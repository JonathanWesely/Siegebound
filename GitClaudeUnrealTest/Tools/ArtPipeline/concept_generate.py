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

Artefact law (CONVENTIONS.md SC-§39.1, added TASK-864):
    "The API returned 200" / "the file was written" / "the call did not throw" are
    NOT success. This tool VALIDATES the artefact it produced before it reports
    success, and it NEVER destroys a prior good roll with an unvalidated one:

      write (to a temp file BESIDE the target)  ->  validate (re-read from disk)
      ->  swap (os.replace) only if the artefact passed

    A degenerate frame (fully black / fully uniform / no lit content) is REJECTED
    with exit code 6, the existing Inbox/<CardID>.png is left byte-for-byte
    untouched, and the rejected frame is kept under Inbox/_rejected/ for
    inspection rather than discarded. Rationale, recorded because it was measured:
    the pre-TASK-864 tool printed "SUCCESS -> Witch.png (3,129 bytes)" for a fully
    black 1024x1024 frame and replaced an 837,664-byte good roll with it.

    The guard ships with its own POSITIVE CONTROL (SC-§39 / SHIP-§9): `--check`
    runs the degeneracy assessment against synthesised degenerate AND synthesised
    legitimately-dark-but-valid fixtures, and FAILS if the guard does not go red
    on the former or does go red on the latter. A guard only ever observed passing
    is indistinguishable from no guard. The wider corpus sweep lives in the
    sibling `test_concept_guard.py`.

Exit codes (0-5/64 identical to trellis_generate.py so the orchestration reads
them 1:1; 6 is NEW in TASK-864 and is unused by the sibling tools):
    0  success (or --check passed)
    1  generic failure (network/generation/output errors after retries; also a
       --check that detects the degeneracy guard itself is broken)
    2  HF_TOKEN not set in the environment
    3  quota / rate-limit exhausted (surfaced verbatim + resume guidance)
    4  API drift: the model/endpoint this client targets is gone or changed shape
    5  prompts missing/unreadable (concept_prompts.json absent, malformed, or the
       requested CardID has no valid prompt entry)
    6  DEGENERATE ARTEFACT: the model answered, but the frame it returned has no
       usable content (black / uniform / unlit). The call SUCCEEDED and the model
       is FINE - the roll is not. Distinct from 5 ("input missing", nothing to
       read) and from 4 ("API drift", the endpoint changed shape). The existing
       Inbox PNG is untouched; re-roll with a DIFFERENT --seed.
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
# Rejected (degenerate) frames are kept here instead of being deleted, so the
# operator can LOOK at what came back. Inside Inbox/ => gitignored like Inbox/.
REJECTED_DIR_NAME = "_rejected"
PROMPTS_PATH = SCRIPT_DIR / "concept_prompts.json"

DEFAULT_SEED = 0
DEFAULT_TIMEOUT_MINUTES = 5.0
DEFAULT_ATTEMPTS = 3
BACKOFF_BASE_SECONDS = 10       # 10s, 30s, 90s ...

# CardIDs are PascalCase alnum (cards.csv row names). Enforced so a CardID can
# never smuggle a path separator / traversal into the Inbox write target.
CARD_ID_RE = re.compile(r"^[A-Za-z][A-Za-z0-9]*$")

# ---------------------------------------------------------------------------
# Degeneracy thresholds (TASK-864). MEASURED, not guessed - see the ledger below.
# ---------------------------------------------------------------------------
# All statistics are taken on the 8-bit sRGB luminance channel (PIL "L"),
# normalised to 0.0-1.0. Every figure below was measured on 2026-09-02 over the
# 49 real shipped concepts (Tools/ArtPipeline/Inbox/*.png +
# Content/RawAssets/Concepts/*.png) and a set of synthesised fixtures. Re-derive
# with `uv run test_concept_guard.py`, which prints the same ledger.
#
#                              real corpus       synth degenerate    synth dark-
#                              (n=49, WORST)     (WORST i.e. least   but-VALID
#                                                obviously bad)      (WORST)
#   p99.9 luminance            0.741             0.031               0.435
#   luminance stddev           0.084             0.0069              0.035
#
# PICKED: each floor sits near the geometric mid-point of the gap it straddles,
# so the guard has a comparable margin against BOTH failure directions
# (SC-§39's "a count-based guard has two failure directions").
#
# DELIBERATELY NOT USED AS GATES, and why - both were the obvious first guesses:
#   * PNG BYTE SIZE. The 12,444-byte black frame in TASK-833 makes a size floor
#     look attractive, but it does not generalise IN EITHER DIRECTION: a
#     near-black NOISE frame compresses to 610 KB (passes a size floor while
#     being 100% degenerate), and a legitimately dark frame compresses to 7 KB
#     (fails a size floor while being perfectly usable). Size is reported as
#     diagnostic context only.
#   * DISTINCT LUMINANCE LEVELS. Inverted on the hardest case: the degenerate
#     noise fixture occupies 13 levels while a legitimate very-dark frame
#     occupies 3. Reported as context; never gated on.
#
# The 99.9th percentile (not max) is used so a handful of stuck/hot pixels cannot
# talk the guard out of a rejection: at 1024x1024 it demands ~1,049 lit pixels
# (a ~32x32 patch), which is far less than any usable TRELLIS reference has.
MIN_HIGHLIGHT_P999 = 0.12   # "is ANYTHING in this frame lit?"
MIN_LUMA_STDDEV = 0.010     # "does this frame have ANY structure?" (catches
                            # uniform frames of any brightness, incl. white/grey)
NEAR_BLACK_LEVEL = 8        # 8/255 - the "near black" bucket, diagnostic only
MIN_ARTEFACT_EDGE_PX = 64   # smaller than this is not a concept, it is a glitch

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


class DegenerateArtefactError(RuntimeError):
    """The model answered, but the frame it returned has no usable content.

    NOT an API failure and NOT a missing input - the call succeeded and the model
    is fine. Carries its own exit code so the orchestration can tell "re-roll with
    a different seed" apart from "the endpoint moved" (4) and "there is nothing to
    read" (5).
    """


# Exit code for DegenerateArtefactError. 6 is unused by trellis_generate.py and
# meshy_generate.py (both stop at 5 + 64), so the shipped family stays 1:1.
EXIT_DEGENERATE = 6


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
# Artefact validation - the degeneracy guard (TASK-864, law SC-§39.1)
# ---------------------------------------------------------------------------


def _percentile_from_histogram(hist: list[int], total: int, fraction: float) -> float:
    """Luminance (0.0-1.0) at the given cumulative fraction of an "L" histogram."""
    if total <= 0:
        return 0.0
    target = fraction * total
    cumulative = 0
    for level, count in enumerate(hist):
        cumulative += count
        if cumulative >= target:
            return level / 255.0
    return 1.0


def measure_image(image) -> dict:
    """Compute the degeneracy statistics for a PIL image. Never raises on shape."""
    from PIL import ImageStat

    luma = image.convert("L")
    total = luma.width * luma.height
    hist = luma.histogram()
    try:
        stat = ImageStat.Stat(luma)
        mean = stat.mean[0] / 255.0
        # A 1-pixel image (or any pathological one) can make PIL's variance go
        # very slightly negative; treat an unmeasurable spread as ZERO, which
        # fails the structure check. The failsafe direction is REJECT.
        stddev = stat.stddev[0] / 255.0
    except Exception:  # noqa: BLE001 - statistics must never mask the artefact
        mean = 0.0
        stddev = 0.0

    # An RGBA frame whose alpha is entirely zero is "written but empty": the
    # luminance channel cannot see it, because convert("L") ignores alpha.
    alpha_max = None
    if "A" in image.getbands():
        alpha_max = image.getchannel("A").getextrema()[1] / 255.0

    return {
        "width": image.width,
        "height": image.height,
        "mean": mean,
        "stddev": stddev,
        "p999": _percentile_from_histogram(hist, total, 0.999),
        "max": (luma.getextrema()[1] / 255.0) if total else 0.0,
        "levels": sum(1 for count in hist if count > 0),
        "near_black_fraction": (sum(hist[: NEAR_BLACK_LEVEL + 1]) / total) if total else 1.0,
        "alpha_max": alpha_max,
    }


# The four independent degeneracy checks. Each one has its OWN isolated fixture
# in build_guard_fixtures(), so no check can silently stop working behind another.
CHECK_SIZE = "degenerate-size"
CHECK_UNLIT = "no-lit-content"
CHECK_FLAT = "no-structure"
CHECK_TRANSPARENT = "fully-transparent"
CHECK_UNREADABLE = "unreadable-artefact"


def assess_degeneracy(stats: dict) -> list[tuple[str, str]]:
    """Return the FAILED checks as (name, detail); an empty list means usable.

    `detail` is self-explaining - the value measured and the threshold it missed -
    so the operator never has to open this file to understand a rejection.
    """
    failed: list[tuple[str, str]] = []
    if stats["width"] < MIN_ARTEFACT_EDGE_PX or stats["height"] < MIN_ARTEFACT_EDGE_PX:
        failed.append((
            CHECK_SIZE,
            f"image is {stats['width']}x{stats['height']}; each edge must be "
            f">= {MIN_ARTEFACT_EDGE_PX}px to be a usable concept",
        ))
    if stats["p999"] < MIN_HIGHLIGHT_P999:
        failed.append((
            CHECK_UNLIT,
            f"99.9th-percentile luminance {stats['p999']:.4f} < "
            f"{MIN_HIGHLIGHT_P999:.4f} floor - essentially nothing in this frame "
            "is lit; this is the fully-black-frame case",
        ))
    if stats["stddev"] < MIN_LUMA_STDDEV:
        failed.append((
            CHECK_FLAT,
            f"luminance stddev {stats['stddev']:.4f} < {MIN_LUMA_STDDEV:.4f} floor "
            "- the frame is near-uniform, i.e. a flat fill rather than a subject "
            "on a background",
        ))
    if stats["alpha_max"] is not None and stats["alpha_max"] <= 0.0:
        failed.append((
            CHECK_TRANSPARENT,
            "every pixel has alpha 0 - the file exists but there is nothing "
            "visible in it (the luminance channel cannot see this)",
        ))
    return failed


def describe_stats(stats: dict) -> str:
    """One-line measurement summary, printed on BOTH success and rejection."""
    alpha = "" if stats["alpha_max"] is None else f" alphaMax={stats['alpha_max']:.3f}"
    return (
        f"{stats['width']}x{stats['height']} mean={stats['mean']:.4f} "
        f"stddev={stats['stddev']:.4f} p99.9={stats['p999']:.4f} "
        f"max={stats['max']:.4f} levels={stats['levels']} "
        f"nearBlack={stats['near_black_fraction']:.4f}{alpha}"
    )


def validate_png_file(path: Path) -> tuple[dict, list[tuple[str, str]]]:
    """Re-READ the staged PNG from disk and assess it.

    Reading it back (rather than assessing the in-memory object we just saved) is
    deliberate: it is the only way this tool can catch a write that reported
    success while producing an unreadable or empty file - the save_asset failure
    mode named alongside this one in SC-§39.1.

    Raises DegenerateArtefactError if the file cannot be opened or decoded.
    """
    from PIL import Image

    try:
        size = path.stat().st_size
    except OSError as exc:
        raise DegenerateArtefactError(
            f"the artefact was reported written but is not on disk: {path} ({exc})"
        ) from None
    if size <= 0:
        raise DegenerateArtefactError(
            f"the artefact was written as a ZERO-BYTE file: {path}"
        )
    try:
        with Image.open(path) as image:
            image.load()           # force a full decode, not just the header
            stats = measure_image(image)
    except Exception as exc:  # noqa: BLE001 - any decode failure is degeneracy
        raise DegenerateArtefactError(
            f"the artefact was written but does not decode as an image: {path} "
            f"({type(exc).__name__}: {exc})"
        ) from None
    stats["bytes"] = size
    return stats, assess_degeneracy(stats)


# ---------------------------------------------------------------------------
# Safe, confined Inbox write: STAGE -> VALIDATE -> SWAP
# ---------------------------------------------------------------------------
# ORDER IS THE WHOLE FIX (SC-§39.1 cl. 3). The staged file is written BESIDE the
# target and the target is not touched until validation passes, so a rejected
# roll costs a delay and never the previous good artefact.


def inbox_path_for(card_id: str) -> Path:
    return INBOX_DIR / f"{card_id}.png"


def _require_inside_inbox(path: Path, allow_subdir: bool = False) -> Path:
    """Confinement guard: refuse any write that escapes Inbox/.

    Defence in depth on top of CARD_ID_RE. `allow_subdir` is granted ONLY to the
    quarantine directory, whose name is a module constant and never user input.
    """
    resolved = path.resolve()
    inbox = INBOX_DIR.resolve()
    inside = (
        resolved.parent == inbox
        if not allow_subdir
        else resolved.parent in (inbox, inbox / REJECTED_DIR_NAME)
    )
    if not inside:
        raise RuntimeError(f"Refusing to write outside Inbox/: {resolved}")
    return resolved


def stage_png(image, dest: Path) -> Path:
    """PHASE 1 - write the image to a temp file BESIDE dest. dest is NOT touched.

    Returns the staged path. The caller MUST either commit_staged() it or dispose
    of it; nothing here can overwrite an existing artefact.
    """
    dest = _require_inside_inbox(dest)
    dest.parent.mkdir(parents=True, exist_ok=True)
    staged = dest.parent / f".{dest.name}.tmp-{os.getpid()}"
    try:
        image.save(str(staged), format="PNG")
    except Exception:
        _discard_staged(staged)
        raise
    return staged


def commit_staged(staged: Path, dest: Path) -> None:
    """PHASE 3 - atomically swap a VALIDATED staged file into place.

    Call this ONLY after validate_png_file() came back with no failed checks.
    """
    _require_inside_inbox(dest)
    os.replace(str(staged), str(dest))  # atomic on the same filesystem


def _discard_staged(staged: Path) -> None:
    if staged.exists():
        try:
            staged.unlink()
        except OSError:
            pass


def quarantine_staged(staged: Path, card_id: str) -> Path | None:
    """Move a REJECTED staged frame to Inbox/_rejected/ instead of deleting it.

    Best-effort by contract: a quarantine failure must never turn a clean
    rejection into a crash, and must never leave the temp file behind.
    """
    try:
        stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        folder = INBOX_DIR.resolve() / REJECTED_DIR_NAME
        target = folder / f"{card_id}.{stamp}.png"
        # Two rejections inside the same second (a retry loop does exactly that)
        # must not overwrite each other - losing evidence is the habit this whole
        # task exists to break.
        suffix = 1
        while target.exists():
            target = folder / f"{card_id}.{stamp}-{suffix}.png"
            suffix += 1
        _require_inside_inbox(target, allow_subdir=True)
        folder.mkdir(parents=True, exist_ok=True)
        os.replace(str(staged), str(target))
        return target
    except Exception as exc:  # noqa: BLE001 - quarantine is a convenience
        warn(f"{card_id}: could not quarantine the rejected frame ({exc}); "
             "discarding it. The existing artefact is still untouched.")
        _discard_staged(staged)
        return None


def _utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


# ---------------------------------------------------------------------------
# The guard's POSITIVE CONTROL (SC-§39 / SHIP-§9)
# ---------------------------------------------------------------------------
# These fixtures are the reason this guard is allowed to exist. They are defined
# HERE, not in the test file, so that `--check` and test_concept_guard.py assert
# against the SAME set - a control that lives only in a test nobody runs is not a
# control. Every statistic is scale-invariant (mean/stddev/percentile), so the
# small default size behaves exactly like a 1024x1024 frame.


def _pseudo_noise_bytes(count: int, low: int, high: int, seed: int) -> bytes:
    """Deterministic 8-bit noise in [low, high]. No numpy, no RNG seeding."""
    state = seed & 0xFFFFFFFF
    span = high - low + 1
    out = bytearray(count)
    for i in range(count):
        state = (1664525 * state + 1013904223) & 0xFFFFFFFF
        out[i] = low + ((state >> 16) % span)
    return bytes(out)


def _checkerboard(size: int, cell: int, dark=(20, 20, 20), light=(240, 240, 240),
                  mode: str = "RGB", alpha: int | None = None):
    """A high-contrast checker: lots of structure AND lots of lit content.

    Used to ISOLATE the checks that are not about brightness or variance, so a
    fixture for (say) the size check cannot accidentally be caught by the flatness
    check instead - which would leave the size check with no control of its own.
    """
    from PIL import Image

    image = Image.new(mode, (size, size))
    px = image.load()
    for y in range(size):
        for x in range(size):
            colour = light if ((x // cell) + (y // cell)) % 2 == 0 else dark
            px[x, y] = colour if alpha is None else (*colour, alpha)
    return image


def build_guard_fixtures(size: int = 256) -> list[tuple[str, object, set[str], str]]:
    """(name, image, expected_failed_check_names, why) for every control case.

    The expectation is the exact SET of checks that must fire - not merely
    "rejected" - so every check owns at least one fixture where it is the ONLY
    one firing. Without that, a broken check hides behind a working one and the
    control goes green while the check is dead (SC-§39's blind-instrument case,
    one level in).

    BOTH directions are represented on purpose. A guard proven only against black
    frames could be `return True`; the legitimately-dark fixtures are what stop
    this from becoming "reject everything", which is the same defect in a new hat.
    """
    from PIL import Image

    fixtures: list[tuple[str, object, set[str], str]] = []
    full = (size, size)

    # ---- MUST BE REJECTED --------------------------------------------------
    fixtures.append((
        "pure-black", Image.new("RGB", full, (0, 0, 0)), {CHECK_UNLIT, CHECK_FLAT},
        "the exact TASK-833 failure: a frame with nothing in it (fires both)",
    ))
    # TASK-833's measured shape: mean ~0.01, ~100% near-black, faint sensor-like
    # noise rather than a perfectly flat fill. This is the HARD degenerate case -
    # it has non-zero variance, so a naive "stddev == 0" test would miss it.
    fixtures.append((
        "near-black-noise",
        Image.frombytes("L", full, _pseudo_noise_bytes(size * size, 0, 5, 71031)).convert("RGB"),
        {CHECK_UNLIT, CHECK_FLAT},
        "TASK-833's measured shape (mean ~0.01, ~100% near-black, faint noise)",
    ))
    # ISOLATES the lit-content check: enough variance to clear the flatness floor,
    # but nothing in the frame is actually lit.
    fixtures.append((
        "dark-noise-unlit",
        Image.frombytes("L", full, _pseudo_noise_bytes(size * size, 0, 20, 4242)).convert("RGB"),
        {CHECK_UNLIT},
        "ISOLATES no-lit-content: real variance, but max luminance ~0.08",
    ))
    fixtures.append((
        "pure-white", Image.new("RGB", full, (255, 255, 255)), {CHECK_FLAT},
        "ISOLATES no-structure: uniform but BRIGHT - the guard is not a darkness test",
    ))
    fixtures.append((
        "flat-mid-grey", Image.new("RGB", full, (128, 128, 128)), {CHECK_FLAT},
        "a mid-brightness flat fill: passes any brightness test, has no content",
    ))
    fixtures.append((
        "fully-transparent",
        _checkerboard(size, max(1, size // 8), mode="RGBA", alpha=0),
        {CHECK_TRANSPARENT},
        "ISOLATES fully-transparent: rich, lit RGB but alpha 0 everywhere, which "
        "the luminance channel cannot see",
    ))
    fixtures.append((
        "tiny-glitch", _checkerboard(16, 4), {CHECK_SIZE},
        "ISOLATES degenerate-size: perfect statistics, but a 16x16 stub is not a "
        "concept",
    ))

    # ---- MUST BE ACCEPTED --------------------------------------------------
    # A dim subject lit by a small highlight. Mean luminance ~0.05: DARKER than
    # anything in the shipped corpus, and it must still pass.
    dark = Image.new("RGB", full, (6, 6, 8))
    dpx = dark.load()
    mid, body_r, glint_r = size // 2, int(size * 0.293), int(size * 0.059)
    for y in range(size):
        for x in range(size):
            dx, dy = x - mid, y - mid
            dist2 = dx * dx + dy * dy
            if dist2 < body_r * body_r:
                falloff = 1.0 - (dist2 ** 0.5) / body_r
                v = int(10 + 70 * falloff)
                dpx[x, y] = (v, int(v * 0.85), int(v * 0.6))
            if dist2 < glint_r * glint_r:
                dpx[x, y] = (210, 200, 170)
    fixtures.append((
        "dark-subject-dim-rim", dark, set(),
        "legitimately dark (mean ~0.05) with a real subject - MUST NOT fire",
    ))

    # The hardest legitimate case: mean luminance ~0.018, i.e. LOWER than some
    # black frames, saved only by a thin lit edge. This fixture is what forces the
    # guard to key on "is anything lit" rather than on mean brightness.
    very_dark = Image.new("RGB", full, (2, 2, 3))
    vpx = very_dark.load()
    silhouette_r, edge_w, edge_h = int(size * 0.254), max(1, int(size * 0.031)), int(size * 0.195)
    for y in range(size):
        for x in range(size):
            dx, dy = x - mid, y - mid
            if dx * dx + dy * dy < silhouette_r * silhouette_r:
                vpx[x, y] = (14, 12, 10)
            if abs(dx) < edge_w and abs(dy) < edge_h:
                vpx[x, y] = (120, 110, 90)
    fixtures.append((
        "very-dark-thin-lit-edge", very_dark, set(),
        "mean ~0.02 - darker than some black frames; only the lit edge saves it",
    ))

    # A stand-in for an ordinary roll: subject on the prompt suffix's flat grey.
    typical = Image.new("RGB", full, (158, 158, 160))
    tpx = typical.load()
    for y in range(size):
        for x in range(size):
            dx, dy = x - mid, y - mid
            if dx * dx + dy * dy < body_r * body_r:
                tpx[x, y] = (70, 90, 130)
            if dx * dx + dy * dy < glint_r * glint_r:
                tpx[x, y] = (250, 245, 230)
    fixtures.append((
        "typical-concept", typical, set(),
        "an ordinary subject-on-grey roll - the everyday case",
    ))
    return fixtures


def run_guard_self_test() -> list[str]:
    """Run every control fixture through the guard; return the DISAGREEMENTS.

    An empty list means the guard both goes RED on degenerate frames and stays
    green on legitimately dark ones. Any entry means the guard is broken and
    must not be trusted with a real roll.
    """
    problems: list[str] = []
    for name, image, expected, why in build_guard_fixtures():
        stats = measure_image(image)
        fired = {check for check, _ in assess_degeneracy(stats)}
        if fired != expected:
            problems.append(
                f"fixture '{name}' fired {sorted(fired) or ['(nothing)']}, "
                f"expected {sorted(expected) or ['(nothing)']} - {why}; "
                f"measured {describe_stats(stats)}"
            )
        say(f"--check: guard control [{'RED  ' if fired else 'green'}] "
            f"{name:24s} {','.join(sorted(fired)) or '-':38s} {describe_stats(stats)}")
    return problems


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
    # A PINNED seed makes the model deterministic, so a degenerate frame from a
    # pinned seed WILL reproduce - retrying it is a guaranteed waste of quota.
    # This is not a guess: TASK-833 recorded seed 71031 returning black TWICE on
    # identical input. With no seed pinned every attempt draws a fresh one, so a
    # retry is worth taking.
    seed_is_pinned = seed is not None
    last_error: Exception | None = None
    for attempt in range(1, attempts + 1):
        say(f"{card_id}: attempt {attempt}/{attempts}")
        started = time.monotonic()
        staged: Path | None = None

        # --- PHASE 1: generate, then STAGE the PNG BESIDE the target. ---------
        # `dest` is NOT touched anywhere in this phase.
        try:
            image = client.text_to_image(positive, **kwargs)
            staged = stage_png(image, dest)
        except Exception as exc:  # noqa: BLE001 - hub raises many HTTP/timeout types
            if staged is not None:
                _discard_staged(staged)
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
            continue

        # --- PHASE 2: VALIDATE the artefact, re-read from disk. ---------------
        # `dest` STILL holds the previous roll while this runs. Nothing has been
        # overwritten, and nothing will be unless `failed_checks` comes back empty.
        try:
            stats, failed_checks = validate_png_file(staged)
        except DegenerateArtefactError as exc:
            stats, failed_checks = None, [(CHECK_UNREADABLE, redact(str(exc)))]

        if not failed_checks:
            # --- PHASE 3: the artefact passed - only NOW is `dest` replaced. --
            commit_staged(staged, dest)
            say(f"{card_id}: SUCCESS -> {dest} ({stats['bytes']:,} bytes) in "
                f"{time.monotonic() - started:.1f}s")
            say(f"{card_id}: validated OK - {describe_stats(stats)}")
            return 0

        # --- REJECTED. `dest` was never opened for writing. -------------------
        quarantined = quarantine_staged(staged, card_id)
        _print_degenerate_report(card_id, dest, stats, failed_checks, quarantined)
        # An UNREADABLE artefact is a write/disk failure, not a model output: the
        # seed says nothing about whether it would recur, so it stays retryable
        # even when the seed is pinned. Only CONTENT degeneracy is deterministic.
        deterministic = seed_is_pinned and not any(
            name == CHECK_UNREADABLE for name, _ in failed_checks
        )
        if deterministic:
            fail(f"{card_id}: seed {seed} is PINNED, so every retry would return "
                 "this same frame - not retrying (TASK-833 recorded seed 71031 "
                 "going black twice on identical input).")
            fail(f"{card_id}: Fix: re-run with a DIFFERENT --seed, or reword the "
                 f"prompt for '{card_id}' in {PROMPTS_PATH.name}.")
            return EXIT_DEGENERATE
        if attempt < attempts:
            warn(f"{card_id}: no seed pinned - re-rolling with a fresh seed "
                 f"(attempt {attempt + 1}/{attempts}).")
            continue
        fail(f"{card_id}: all {attempts} attempts returned a degenerate frame. "
             f"Fix: reword the prompt for '{card_id}' in {PROMPTS_PATH.name}, or "
             "pin a known-good --seed.")
        return EXIT_DEGENERATE

    fail(f"{card_id}: failed after {attempts} attempts: "
         f"{redact(f'{type(last_error).__name__}: {last_error}')}")
    return 1


def _print_degenerate_report(
    card_id: str,
    dest: Path,
    stats: dict | None,
    failed_checks: list[tuple[str, str]],
    quarantined: Path | None,
) -> None:
    """Say exactly what was measured, what it missed, and what was NOT lost."""
    fail(f"{card_id}: DEGENERATE ARTEFACT REJECTED - the model answered, but the "
         "frame it returned has no usable content.")
    if stats is not None:
        fail(f"  measured: {describe_stats(stats)} ({stats.get('bytes', 0):,} bytes)")
    for name, detail in failed_checks:
        fail(f"  FAILED [{name}]: {detail}")
    if dest.exists():
        fail(f"  NOT OVERWRITTEN: {dest} still holds the PREVIOUS roll, byte for byte.")
    else:
        fail(f"  NOT WRITTEN: {dest} does not exist (there was nothing to lose).")
    if quarantined is not None:
        fail(f"  The rejected frame was KEPT for inspection at: {quarantined}")


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
    """--check: TOKENLESS smoke test - deps, client signature, prompts, GUARD.

    Validates that the tool WIRES UP without making any GPU call or spending any
    quota: dependencies import, the HF InferenceClient still exposes the endpoint
    this script drives, concept_prompts.json parses against the schema, the
    degeneracy guard still behaves correctly on its control fixtures, and it
    reports whether HF_TOKEN is present (presence only - never the value, and
    never fails on absence: --check is tokenless by contract).

    Step 5 is the guard's POSITIVE CONTROL and it is the reason --check is worth
    running after any edit to the thresholds: it proves the guard can go RED.

    With --probe it additionally makes a tokenless PUBLIC metadata call
    (model_info) to check the endpoint is reachable - best-effort, informational
    only (a network failure never flips the exit code).
    """
    import inspect

    # ---- 1/5 dependency import -------------------------------------------------
    try:
        from huggingface_hub import InferenceClient
        import PIL  # noqa: F401 - presence check only (Pillow saves the PNG)
    except Exception as exc:  # noqa: BLE001
        fail(f"Dependency import failed: {type(exc).__name__}: {exc}")
        fail("Run `uv sync` in Tools/ArtPipeline (needs huggingface_hub + pillow).")
        return 1
    say("--check: dependencies import OK (huggingface_hub, pillow).")

    # ---- 2/5 offline endpoint/signature drift guard ---------------------------
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

    # ---- 3/5 prompts schema ---------------------------------------------------
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

    # ---- 4/5 degeneracy guard POSITIVE CONTROL (SC-§39 / SHIP-§9) -------------
    # This step is the whole reason TASK-864 is closed rather than merely coded:
    # it proves the guard goes RED on a synthesised degenerate frame AND stays
    # green on a legitimately dark one. A guard only ever observed passing is
    # indistinguishable from no guard - which is the exact defect being fixed.
    guard_problems = run_guard_self_test()
    if guard_problems:
        fail("--check: the DEGENERACY GUARD IS BROKEN - it disagreed with its own "
             "control fixtures. Generation is NOT safe to run: an unverified "
             "verifier is the defect it exists to prevent (SC-§39.1).")
        for problem in guard_problems:
            fail(f"  {problem}")
        fail("Fix: MIN_HIGHLIGHT_P999 / MIN_LUMA_STDDEV / assess_degeneracy() at "
             "the top of concept_generate.py. Re-run `--check` until this passes.")
        return 1
    say("--check: degeneracy guard control PASSED - RED on every degenerate "
        "fixture, green on every legitimately-dark one.")

    # ---- 5/5 token presence (report only; tokenless by contract) --------------
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
    degenerate: list[str] = []
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
        elif rc == EXIT_DEGENERATE:
            # A degenerate roll is a per-card prompt/seed interaction, NOT a
            # systemic outage: keep going so one bad card cannot stall the batch.
            # Every skipped card keeps its previous PNG, untouched.
            degenerate.append(card_id)
            worst = worst or EXIT_DEGENERATE
        else:
            worst = worst or rc  # remember the first generic failure, keep going
    say(f"--all done: {ok} generated, {skipped} skipped, "
        f"{len(card_ids) - ok - skipped} failed.")
    if degenerate:
        # Named, not just counted - otherwise the batch summary buries the very
        # thing the operator has to act on.
        fail(f"--all: {len(degenerate)} CardID(s) returned a DEGENERATE frame and "
             f"were REJECTED (their existing PNGs are untouched): "
             f"{', '.join(degenerate)}")
        fail("--all: re-roll just those with a different --seed, e.g. "
             f"`uv run concept_generate.py {degenerate[0]} --force --seed <new>`.")
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
            "drift, 5 prompts missing, 6 DEGENERATE ARTEFACT (the model answered but "
            "the frame is black/uniform/unlit - the existing PNG is left untouched; "
            "re-roll with a different --seed), 64 CLI usage. Examples: "
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
             "manual drops/edits are never clobbered). --force is NOT destructive on "
             "failure: the new frame is staged and validated first, and a degenerate "
             "roll (exit 6) leaves the existing PNG byte-for-byte intact.",
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
