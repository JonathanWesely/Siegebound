#!/usr/bin/env python3
"""Stage 0 of the Siegebound art pipeline: concept-art generation client (TASK-184).

Turns a per-CardID art-direction PROMPT (Tools/ArtPipeline/concept_prompts.json)
into a single-subject concept PNG (Tools/ArtPipeline/Inbox/<CardID>.png) by driving
a Hugging Face text->image model through the HF Inference API. The PNG is the exact
input the TRELLIS Stage-1 client (trellis_generate.py) consumes; Jonathan may
review / replace any generated concept before its mesh wave runs (TASK-167, OPTIONAL).

This tool MIRRORS trellis_generate.py's structure, secret-handling, and exit-code
map (CONVENTIONS.md "Textured mesh law" -> "Stage 0 - concept generation", tooling
law). It is PROMPT-AGNOSTIC: no card design is hardcoded - all art direction lives
in concept_prompts.json (art-director-authored, TASK-185).

Security law (CONVENTIONS.md tooling law, guard-secrets `hf_` pattern):
    HF_TOKEN is ENV-ONLY. It is read from the environment at runtime, is never
    accepted on argv, never written to any file, and never echoed or logged -
    every line this script prints passes through a redactor that scrubs the live
    token value AND anything matching the generic hf_ token shape, so even a
    hostile exception message cannot leak it.

Session law:
    Each run uses ONE InferenceClient. `--all` drives every CardID through that
    single client instance; a single-card run makes exactly one text->image call.

Exit codes (identical to trellis_generate.py so the orchestration reads them 1:1):
    0  success (or --check passed)
    1  generic failure (network/generation/output errors after retries)
    2  HF_TOKEN not set in the environment
    3  quota / rate-limit exhausted (surfaced verbatim + resume guidance)
    4  API drift: the model/endpoint this client targets is gone or changed shape
    5  prompts missing/unreadable (concept_prompts.json absent, malformed, or the
       requested CardID has no valid prompt entry)
    64 CLI usage error (argparse default of 2 is remapped so exit code 2 uniquely
       means "HF_TOKEN unset")
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

# ---------------------------------------------------------------------------
# Model / endpoint (SWAPPABLE - top-of-file constants).
# ---------------------------------------------------------------------------
# The image model driven through the HF Inference API. FLUX.1-dev (TASK-192, M7.5
# art-quality upgrade) replaces FLUX.1-schnell: dev is the full guidance-distilled
# base model (richer detail, better prompt adherence) where schnell was the 4-step
# timestep-distilled turbo variant. LICENSE NOTE: dev is a GATED repo under the
# "FLUX.1 [dev] Non-Commercial License" (schnell was Apache-2.0) - the HF account
# behind HF_TOKEN must have accepted the license once on the model page, or
# token-authed generation can 403. To swap the endpoint (e.g. to SDXL, or a
# different provider):
#   - change MODEL_ID to the new repo id;
#   - set PROVIDER (HF Inference "auto" routing by default; can pin "hf-inference",
#     "fal-ai", "replicate", "together", "nebius", ...);
#   - flip MODEL_SUPPORTS_NEGATIVE_PROMPT True for true-CFG models (SDXL). BOTH
#     FLUX variants are guidance-distilled and ignore negatives (dev's
#     guidance_scale drives an embedded/distilled guidance vector, NOT
#     classifier-free guidance over a negative prompt), so we do NOT forward
#     negatives to them;
#   - retune DEFAULT_STEPS / DEFAULT_GUIDANCE (dev wants ~28-50 steps / guidance
#     ~3.5; schnell wanted ~4 steps / guidance omitted; SDXL ~30 steps / ~7.5).
# No other code needs to change - the CLI, schema, and exit codes are model-agnostic.
MODEL_ID = "black-forest-labs/FLUX.1-dev"
PROVIDER = "auto"
MODEL_SUPPORTS_NEGATIVE_PROMPT = False

# TRELLIS image-to-3D wants a clean single-subject reference: one centered subject,
# plain background, full-body, orthographic-ish. This shared suffix is APPENDED to
# every per-CardID prompt so authors write only the subject description. (Mirrors
# the concept-image guidance in README.md "Concept image guidance".)
PROMPT_SUFFIX = (
    ", single centered subject, full body, orthographic front three-quarter view, "
    "plain flat neutral grey background, even studio lighting, no ground shadow, "
    "no scene, no props, no text, stylized game concept art, clean silhouette"
)
# Baked-in negatives, used ONLY when MODEL_SUPPORTS_NEGATIVE_PROMPT is True (a CFG
# model). Per-entry negative_prompt is appended to this.
NEGATIVE_SUFFIX = (
    "multiple subjects, crowd, background scenery, props, text, watermark, "
    "signature, cropped, cut off, extra limbs, blurry, low quality"
)

# Generation parameters (SWAPPABLE with the model - see the swap note above).
DEFAULT_WIDTH = 1024
DEFAULT_HEIGHT = 1024
# FLUX.1-dev sampling (TASK-192). Dev is NOT the 4-step turbo - it needs a real
# step count. Board-spec window is ~28-50; 40 is the quality-leaning midpoint
# (detail gains flatten past ~30-40; 50 mostly costs more compute/quota). A run
# may still override per-call with --steps for faster/cheaper draft probes.
DEFAULT_STEPS = 40
# Distilled-guidance strength for FLUX.1-dev (model-card recommended value 3.5).
# This drives dev's embedded guidance vector - it is NOT classifier-free guidance,
# so no negative prompt accompanies it. None => omit the kwarg entirely (the old
# schnell posture, which took no guidance at all).
DEFAULT_GUIDANCE: float | None = 3.5

# ---------------------------------------------------------------------------
# Paths / retry / timeout constants
# ---------------------------------------------------------------------------

SCRIPT_DIR = Path(__file__).resolve().parent
INBOX_DIR = SCRIPT_DIR / "Inbox"
PROMPTS_PATH = SCRIPT_DIR / "concept_prompts.json"

DEFAULT_SEED = 0
DEFAULT_TIMEOUT_MINUTES = 5.0
DEFAULT_ATTEMPTS = 3
BACKOFF_BASE_SECONDS = 10       # 10s, 30s, 90s ...

# CardIDs are PascalCase alnum (cards.csv row names). Enforced so a CardID can
# never smuggle a path separator / traversal into the Inbox write target.
CARD_ID_RE = re.compile(r"^[A-Za-z][A-Za-z0-9]*$")

# ---------------------------------------------------------------------------
# Secret-safe output. EVERY print in this script goes through redact().
# ---------------------------------------------------------------------------

_REDACT_VALUES: list[str] = []
# Generic Hugging Face token shape (matches the guard-secrets hook pattern).
_HF_TOKEN_RE = re.compile(r"hf_[A-Za-z0-9]{15,}")
_REDACTED = "[hf-token-redacted]"


def _register_secret(value: str) -> None:
    """Remember a secret so redact() can scrub it from ALL output paths."""
    if value and value not in _REDACT_VALUES:
        _REDACT_VALUES.append(value)


def redact(text: object) -> str:
    out = str(text)
    for value in _REDACT_VALUES:
        out = out.replace(value, _REDACTED)
    return _HF_TOKEN_RE.sub(_REDACTED, out)


def say(msg: str) -> None:
    print(f"[concept] {redact(msg)}", flush=True)


def warn(msg: str) -> None:
    print(f"[concept][WARN] {redact(msg)}", file=sys.stderr, flush=True)


def fail(msg: str) -> None:
    print(f"[concept][ERROR] {redact(msg)}", file=sys.stderr, flush=True)


# ---------------------------------------------------------------------------
# Error classes
# ---------------------------------------------------------------------------


class ApiDriftError(RuntimeError):
    """The model/endpoint this client targets is gone or changed shape."""


class QuotaExceededError(RuntimeError):
    """HF quota / rate-limit exhausted - an expected pause, not a code failure."""


class PromptsError(RuntimeError):
    """concept_prompts.json missing/malformed, or the CardID has no valid entry."""


def _looks_like_quota_error(text: str) -> bool:
    lowered = text.lower()
    markers = (
        "quota",
        "too many requests",
        "rate limit",
        "rate-limit",
        "ratelimit",
        "monthly included credits",
        "insufficient credits",
        "payment required",
        " 402",
        "http 402",
        " 429",
        "http 429",
        "status 429",
    )
    return any(m in lowered for m in markers)


def _looks_like_drift_error(text: str) -> bool:
    lowered = text.lower()
    markers = (
        "not found",
        "does not exist",
        "no such model",
        "repositorynotfound",
        "not supported",
        "no provider",
        "unknown model",
        "410 gone",
        " 404",
        "http 404",
        "status 404",
    )
    return any(m in lowered for m in markers)


def _print_quota_guidance(verbatim: str) -> None:
    fail("HF quota / rate-limit exceeded. Provider message, verbatim:")
    fail(f"    {verbatim}")
    fail("Guidance:")
    fail("  - The message above may include a reset/retry time - record it in the")
    fail("    task handoff (surface it in the 🚨 Blockers Slack thread) and resume")
    fail("    in the next window (re-run the same CardID / --all; existing PNGs are")
    fail("    skipped without --force, so a resume never redoes finished concepts).")
    fail("  - HF PRO (the token in use) has a monthly credit budget; a hard stop")
    fail("    here means the budget is spent for the window, not a code failure.")


# ---------------------------------------------------------------------------
# Prompts schema (concept_prompts.json)
# ---------------------------------------------------------------------------
#   { "_doc": {...}, "version": 1,
#     "prompts": { "<CardID>": { "prompt": str,             # required, non-empty
#                                "negative_prompt": str,    # optional
#                                "seed": int } } }          # optional


def load_prompts(path: Path) -> dict:
    """Read + shallow-validate concept_prompts.json; return the prompts mapping.

    Raises PromptsError (=> exit 5) on missing file, bad JSON, or a missing/
    malformed top-level shape. Per-entry validation happens in validate_entry().
    """
    if not path.is_file():
        raise PromptsError(
            f"Prompts file not found: {path}\n"
            "Author it as concept_prompts.json beside this script "
            "(schema: CardID -> {prompt, optional negative_prompt, optional seed}); "
            "the art-director writes the 16 M7 prompts in TASK-185."
        )
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:  # noqa: BLE001 - json + decode errors
        raise PromptsError(f"Prompts file is not valid JSON: {path}: {exc}") from None
    if not isinstance(data, dict):
        raise PromptsError(f"Prompts file top-level must be a JSON object: {path}")
    prompts = data.get("prompts")
    if not isinstance(prompts, dict):
        raise PromptsError(
            f'Prompts file must have a "prompts" object mapping CardID -> entry: {path}'
        )
    if not prompts:
        raise PromptsError(f'Prompts file "prompts" object is empty: {path}')
    return prompts


def validate_entry(card_id: str, entry: object) -> dict:
    """Validate one CardID entry; return a normalized dict. Raise PromptsError."""
    if not CARD_ID_RE.match(card_id):
        raise PromptsError(
            f"Invalid CardID '{card_id}': must be PascalCase alphanumeric "
            "(the cards.csv row-name / Inbox filename law)."
        )
    if not isinstance(entry, dict):
        raise PromptsError(f"Prompt entry for '{card_id}' must be an object.")
    prompt = entry.get("prompt")
    if not isinstance(prompt, str) or not prompt.strip():
        raise PromptsError(f"Prompt entry for '{card_id}' needs a non-empty 'prompt' string.")
    negative = entry.get("negative_prompt")
    if negative is not None and not isinstance(negative, str):
        raise PromptsError(f"'negative_prompt' for '{card_id}' must be a string if present.")
    seed = entry.get("seed")
    if seed is not None and not isinstance(seed, int):
        raise PromptsError(f"'seed' for '{card_id}' must be an integer if present.")
    return {"prompt": prompt.strip(), "negative_prompt": negative, "seed": seed}


# ---------------------------------------------------------------------------
# Prompt assembly
# ---------------------------------------------------------------------------


def build_positive_prompt(entry: dict) -> str:
    return f"{entry['prompt'].rstrip(' ,')}{PROMPT_SUFFIX}"


def build_negative_prompt(entry: dict) -> str | None:
    """Only meaningful for a true-CFG model; None otherwise (FLUX dev+schnell are
    guidance-distilled and ignore negatives - see MODEL_SUPPORTS_NEGATIVE_PROMPT)."""
    if not MODEL_SUPPORTS_NEGATIVE_PROMPT:
        return None
    parts = [NEGATIVE_SUFFIX]
    if entry.get("negative_prompt"):
        parts.append(entry["negative_prompt"])
    return ", ".join(parts)


# ---------------------------------------------------------------------------
# Safe, confined, atomic Inbox write
# ---------------------------------------------------------------------------


def inbox_path_for(card_id: str) -> Path:
    return INBOX_DIR / f"{card_id}.png"


def atomic_write_png(image, dest: Path) -> None:
    """Write PNG to a temp file then atomically rename; refuse to escape Inbox/."""
    dest = dest.resolve()
    inbox = INBOX_DIR.resolve()
    if dest.parent != inbox:
        # Confinement guard: writes are restricted to Inbox/ (defence in depth on
        # top of the CardID regex).
        raise RuntimeError(f"Refusing to write outside Inbox/: {dest}")
    inbox.mkdir(parents=True, exist_ok=True)
    tmp = dest.parent / f".{dest.name}.tmp-{os.getpid()}"
    try:
        image.save(str(tmp), format="PNG")
        os.replace(str(tmp), str(dest))  # atomic on the same filesystem
    finally:
        if tmp.exists():
            try:
                tmp.unlink()
            except OSError:
                pass


def _utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


# ---------------------------------------------------------------------------
# Generation
# ---------------------------------------------------------------------------


def build_client(token: str, timeout_seconds: float):
    """Construct the single InferenceClient for a run (one client per invocation)."""
    from huggingface_hub import InferenceClient  # local import: keeps --help dep-free

    return InferenceClient(provider=PROVIDER, token=token, timeout=timeout_seconds)


def generate_one(client, card_id: str, entry: dict, args: argparse.Namespace) -> int:
    """Generate + save ONE concept PNG. Returns an exit-code-style int.

    Assumes the token is present, the entry is validated, and `client` is built.
    Skips (returns 0) if Inbox/<CardID>.png already exists and --force is absent,
    so Jonathan's manual drops/edits are never clobbered.
    """
    dest = inbox_path_for(card_id)
    if dest.exists() and not args.force:
        say(f"{card_id}: Inbox/{card_id}.png already exists - skipping "
            "(pass --force to overwrite). Manual drops/edits are preserved.")
        return 0

    seed = args.seed if args.seed is not None else entry.get("seed")
    positive = build_positive_prompt(entry)
    negative = build_negative_prompt(entry)

    # Only forward optional kwargs that are set, so swapping model/provider stays easy.
    kwargs: dict = {
        "model": MODEL_ID,
        "width": args.width,
        "height": args.height,
    }
    if args.steps is not None:
        kwargs["num_inference_steps"] = args.steps
    if DEFAULT_GUIDANCE is not None:
        kwargs["guidance_scale"] = DEFAULT_GUIDANCE
    if seed is not None:
        kwargs["seed"] = seed
    if negative is not None:
        kwargs["negative_prompt"] = negative
    elif entry.get("negative_prompt"):
        warn(f"{card_id}: negative_prompt provided but MODEL_SUPPORTS_NEGATIVE_PROMPT "
             f"is False for {MODEL_ID} (guidance-distilled) - ignoring it.")

    say(f"{card_id}: generating via {MODEL_ID} (provider={PROVIDER}, "
        f"{args.width}x{args.height}"
        + (f", seed={seed}" if seed is not None else "") + ")")

    attempts = args.attempts
    last_error: Exception | None = None
    for attempt in range(1, attempts + 1):
        say(f"{card_id}: attempt {attempt}/{attempts}")
        started = time.monotonic()
        try:
            image = client.text_to_image(positive, **kwargs)
            atomic_write_png(image, dest)
            size = dest.stat().st_size
            say(f"{card_id}: SUCCESS -> {dest} ({size:,} bytes) in "
                f"{time.monotonic() - started:.1f}s")
            return 0
        except Exception as exc:  # noqa: BLE001 - hub raises many HTTP/timeout types
            text = redact(f"{type(exc).__name__}: {exc}")
            if _looks_like_quota_error(text):
                _print_quota_guidance(text)
                return 3
            if _looks_like_drift_error(text):
                fail(f"{card_id}: model/API drift talking to {MODEL_ID}: {text}")
                fail("Fix: update MODEL_ID / PROVIDER at the top of concept_generate.py "
                     "(the model may have moved or the provider dropped it).")
                return 4
            last_error = exc
            if attempt < attempts:
                delay = BACKOFF_BASE_SECONDS * (3 ** (attempt - 1))
                warn(f"{card_id}: attempt failed ({text}); retrying in {delay}s")
                time.sleep(delay)
    fail(f"{card_id}: failed after {attempts} attempts: "
         f"{redact(f'{type(last_error).__name__}: {last_error}')}")
    return 1


# ---------------------------------------------------------------------------
# Token gate (ENV-ONLY, exit 2)
# ---------------------------------------------------------------------------


def require_token() -> str | None:
    token = os.environ.get("HF_TOKEN")
    if not token:
        fail("HF_TOKEN is not set in the environment (required for generation).")
        fail("Setup (one-time): Windows Settings > System > About > Advanced system")
        fail("settings > Environment Variables > add user variable HF_TOKEN with your")
        fail("Hugging Face access token (huggingface.co/settings/tokens), then open a")
        fail("NEW terminal. The token lives ONLY in the environment: never put it in a")
        fail("file, on a command line, or in chat. (--check runs without a token.)")
        return None
    _register_secret(token)
    return token


# ---------------------------------------------------------------------------
# Modes
# ---------------------------------------------------------------------------


def run_check(args: argparse.Namespace) -> int:
    """--check: TOKENLESS smoke test - deps + client signature + prompts schema.

    Validates that the tool WIRES UP without making any GPU call or spending any
    quota: dependencies import, the HF InferenceClient still exposes the endpoint
    this script drives, concept_prompts.json parses against the schema, and it
    reports whether HF_TOKEN is present (presence only - never the value, and
    never fails on absence: --check is tokenless by contract).

    With --probe it additionally makes a tokenless PUBLIC metadata call
    (model_info) to check the endpoint is reachable - best-effort, informational
    only (a network failure never flips the exit code).
    """
    import inspect

    # ---- 1/4 dependency import -------------------------------------------------
    try:
        from huggingface_hub import InferenceClient
        import PIL  # noqa: F401 - presence check only (Pillow saves the PNG)
    except Exception as exc:  # noqa: BLE001
        fail(f"Dependency import failed: {type(exc).__name__}: {exc}")
        fail("Run `uv sync` in Tools/ArtPipeline (needs huggingface_hub + pillow).")
        return 1
    say("--check: dependencies import OK (huggingface_hub, pillow).")

    # ---- 2/4 offline endpoint/signature drift guard ---------------------------
    init_params = inspect.signature(InferenceClient.__init__).parameters
    if "token" not in init_params:
        fail("huggingface_hub drift: InferenceClient.__init__ no longer accepts "
             f"'token' (live params: {', '.join(init_params)}). build_client() passes "
             "token=<HF_TOKEN>, so real generation WOULD fail. Update build_client().")
        return 1
    if not hasattr(InferenceClient, "text_to_image"):
        fail("huggingface_hub drift: InferenceClient has no 'text_to_image' method - "
             "the endpoint this tool drives is gone. Update the client call.")
        return 1
    t2i_params = inspect.signature(InferenceClient.text_to_image).parameters
    for required in ("prompt", "model"):
        if required not in t2i_params:
            fail(f"huggingface_hub drift: text_to_image no longer accepts '{required}' "
                 f"(live params: {', '.join(t2i_params)}). Update generate_one().")
            return 1
    say("--check: InferenceClient signature OK "
        "(accepts token; text_to_image accepts prompt/model).")

    # ---- 3/4 prompts schema ---------------------------------------------------
    if not PROMPTS_PATH.is_file():
        warn(f"--check: {PROMPTS_PATH.name} not found (the art-director authors it in "
             "TASK-185). The tool is wired; generation will exit 5 until it exists.")
    else:
        try:
            prompts = load_prompts(PROMPTS_PATH)
            for cid, entry in prompts.items():
                validate_entry(cid, entry)
        except PromptsError as exc:
            fail(f"--check: prompts schema invalid: {exc}")
            return 5
        say(f"--check: {PROMPTS_PATH.name} OK - {len(prompts)} valid "
            f"prompt entr{'y' if len(prompts) == 1 else 'ies'} "
            f"({', '.join(sorted(prompts))}).")

    # ---- 4/4 token presence (report only; tokenless by contract) --------------
    if os.environ.get("HF_TOKEN"):
        say("--check: HF_TOKEN is present in the environment (value not read/echoed).")
    else:
        say("--check: HF_TOKEN is NOT set - fine for --check; generation would exit 2.")

    # ---- optional live tokenless reachability probe ---------------------------
    if args.probe:
        try:
            from huggingface_hub import model_info

            info = model_info(MODEL_ID)  # public metadata; no token, no GPU
            pipe = getattr(info, "pipeline_tag", "?")
            say(f"--check --probe: {MODEL_ID} reachable (pipeline_tag={pipe}).")
        except Exception as exc:  # noqa: BLE001 - network/gated/etc. is non-fatal here
            warn(f"--check --probe: reachability probe skipped/failed "
                 f"({type(exc).__name__}: {exc}). Offline checks still passed; the "
                 "real generation run (TASK-185) will surface a genuine outage.")

    say(f"--check PASSED: tool wires up (model constant: {MODEL_ID}).")
    return 0


def run_generate(args: argparse.Namespace) -> int:
    """Single-CardID generation."""
    token = require_token()
    if token is None:
        return 2
    try:
        prompts = load_prompts(PROMPTS_PATH)
        if args.card_id not in prompts:
            raise PromptsError(
                f"No prompt for CardID '{args.card_id}' in {PROMPTS_PATH.name}. "
                f"Available: {', '.join(sorted(prompts)) or '(none)'}."
            )
        entry = validate_entry(args.card_id, prompts[args.card_id])
    except PromptsError as exc:
        fail(str(exc))
        return 5

    timeout_seconds = args.timeout_minutes * 60.0
    client = build_client(token, timeout_seconds)
    return generate_one(client, args.card_id, entry, args)


def run_all(args: argparse.Namespace) -> int:
    """Generate every CardID in concept_prompts.json through ONE client."""
    token = require_token()
    if token is None:
        return 2
    try:
        prompts = load_prompts(PROMPTS_PATH)
    except PromptsError as exc:
        fail(str(exc))
        return 5

    timeout_seconds = args.timeout_minutes * 60.0
    client = build_client(token, timeout_seconds)

    card_ids = sorted(prompts)
    say(f"--all: {len(card_ids)} CardID(s): {', '.join(card_ids)}")
    worst = 0
    ok = skipped = 0
    for card_id in card_ids:
        try:
            entry = validate_entry(card_id, prompts[card_id])
        except PromptsError as exc:
            fail(f"{card_id}: bad prompt entry - {exc}")
            worst = worst or 5
            continue
        dest_exists = inbox_path_for(card_id).exists()
        rc = generate_one(client, card_id, entry, args)
        if rc == 0:
            if dest_exists and not args.force:
                skipped += 1
            else:
                ok += 1
        elif rc == 3:
            fail("--all: stopping - quota/rate-limit hit. Re-run --all later; "
                 "completed PNGs are skipped without --force.")
            return 3
        elif rc == 4:
            fail("--all: stopping - model/API drift (systemic). Fix MODEL_ID/PROVIDER.")
            return 4
        else:
            worst = worst or rc  # remember the first generic failure, keep going
    say(f"--all done: {ok} generated, {skipped} skipped, "
        f"{len(card_ids) - ok - skipped} failed.")
    return worst


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


class _Parser(argparse.ArgumentParser):
    """argparse exits 2 on usage errors by default; remap to 64 (EX_USAGE) so
    exit code 2 UNIQUELY means 'HF_TOKEN unset' (board-spec contract)."""

    def error(self, message: str):  # noqa: D102 - argparse override
        self.print_usage(sys.stderr)
        # redact() the message so a token accidentally pasted on the command line is
        # never echoed - the generic hf_ pattern works even before any secret is
        # registered.
        self.exit(64, f"{self.prog}: error: {redact(message)}\n")


def build_parser() -> argparse.ArgumentParser:
    parser = _Parser(
        prog="concept_generate.py",
        description=(
            "Stage 0 of the Siegebound art pipeline: generate a single-subject "
            f"concept PNG from a per-CardID prompt (concept_prompts.json) via {MODEL_ID} "
            "on the HF Inference API. Output: Inbox/<CardID>.png (the TRELLIS Stage-1 "
            "input)."
        ),
        epilog=(
            "HF_TOKEN is read from the environment ONLY (never argv/file/log). Exit "
            "codes: 0 success, 1 failure, 2 HF_TOKEN unset, 3 quota/rate-limit, 4 API "
            "drift, 5 prompts missing, 64 CLI usage. Examples: "
            "`uv run concept_generate.py Knight`, "
            "`uv run concept_generate.py --all --force`, "
            "`uv run concept_generate.py --check`."
        ),
    )
    parser.add_argument(
        "card_id",
        nargs="?",
        metavar="CardID",
        help="CardID (PascalCase); reads its prompt from concept_prompts.json and "
             "writes Inbox/<CardID>.png. Required unless --all / --check / --list.",
    )
    parser.add_argument(
        "--all", action="store_true",
        help="Generate every CardID in concept_prompts.json (through one client).",
    )
    parser.add_argument(
        "--check", action="store_true",
        help="TOKENLESS smoke test: deps import, client signature, prompts schema, "
             "and HF_TOKEN presence. No GPU call, no quota spend. (Add --probe for a "
             "tokenless public reachability check of the model.)",
    )
    parser.add_argument(
        "--probe", action="store_true",
        help="With --check: also make a tokenless public model_info() reachability "
             "call (informational; never changes the exit code).",
    )
    parser.add_argument(
        "--list", action="store_true",
        help="List the CardIDs defined in concept_prompts.json and exit.",
    )
    parser.add_argument(
        "--force", action="store_true",
        help="Overwrite an existing Inbox/<CardID>.png (default: skip if present, so "
             "manual drops/edits are never clobbered).",
    )
    parser.add_argument(
        "--seed", type=int, default=None,
        help="Override the generation seed for this run (default: the entry's seed, "
             "or the model default). Reroll a bad concept by changing this.",
    )
    parser.add_argument(
        "--width", type=int, default=DEFAULT_WIDTH,
        help=f"Output width in px (default {DEFAULT_WIDTH}; square recommended for TRELLIS).",
    )
    parser.add_argument(
        "--height", type=int, default=DEFAULT_HEIGHT,
        help=f"Output height in px (default {DEFAULT_HEIGHT}).",
    )
    parser.add_argument(
        "--steps", type=int, default=DEFAULT_STEPS,
        help=f"num_inference_steps (default {DEFAULT_STEPS}; FLUX.1-dev wants ~28-50).",
    )
    parser.add_argument(
        "--timeout-minutes", type=float, default=DEFAULT_TIMEOUT_MINUTES,
        help=f"Per-call timeout in minutes (default {DEFAULT_TIMEOUT_MINUTES}).",
    )
    parser.add_argument(
        "--attempts", type=int, default=DEFAULT_ATTEMPTS,
        help=f"Retry attempts per generation with exponential backoff (default "
             f"{DEFAULT_ATTEMPTS}). Quota/drift errors are never retried.",
    )
    return parser


def run_list() -> int:
    try:
        prompts = load_prompts(PROMPTS_PATH)
    except PromptsError as exc:
        fail(str(exc))
        return 5
    say(f"{len(prompts)} CardID(s) in {PROMPTS_PATH.name}:")
    for cid in sorted(prompts):
        print(f"  {cid}", flush=True)
    return 0


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)

    if args.attempts < 1:
        warn("--attempts must be >= 1; using 1.")
        args.attempts = 1
    if args.timeout_minutes <= 0:
        warn("--timeout-minutes must be > 0; using the default "
             f"{DEFAULT_TIMEOUT_MINUTES}.")
        args.timeout_minutes = DEFAULT_TIMEOUT_MINUTES

    if args.check:
        return run_check(args)
    if args.list:
        return run_list()
    if args.all:
        if args.card_id:
            build_parser().error("give either a CardID or --all, not both.")
        return run_all(args)
    if not args.card_id:
        build_parser().error("a CardID is required unless --all / --check / --list.")
    if not CARD_ID_RE.match(args.card_id):
        build_parser().error(
            f"invalid CardID '{args.card_id}': PascalCase alphanumeric only.")
    return run_generate(args)


if __name__ == "__main__":
    sys.exit(main())
