#!/usr/bin/env python3
"""Stage 1 of the Siegebound art pipeline: TRELLIS.2 generation client (TASK-082).

Turns a concept image (Tools/ArtPipeline/Inbox/<AssetName>.png) into a raw
textured GLB (Tools/ArtPipeline/Cache/<AssetName>/trellis_raw.glb) by driving
the official Hugging Face Space ``microsoft/TRELLIS.2`` through gradio_client.

Security law (CONVENTIONS.md "Textured mesh law", tooling law):
    HF_TOKEN is ENV-ONLY. It is read from the environment at runtime, is never
    accepted on argv, never written to any file (state.json / api_schema.json
    carry no credential material), and never echoed or logged - every line this
    script prints passes through a redactor that scrubs the live token value
    AND anything matching the generic hf_ token shape, so even a hostile
    exception message cannot leak it.

Session law:
    The Space keeps the generated model in gr.State, which is coupled to the
    gradio_client SESSION. The whole preprocess -> image_to_3d -> extract_glb
    sequence therefore runs atomically on ONE Client instance; it is never
    split across script runs or Client objects.

Exit codes:
    0  success (or --check passed)
    1  generic failure (network/generation/output errors after retries)
    2  HF_TOKEN not set in the environment
    3  ZeroGPU quota exhausted (error surfaced verbatim + reset guidance)
    4  API drift: required endpoints/schema not found on the Space
    5  input concept image missing or unreadable
    64 CLI usage error (argparse default of 2 is remapped so that exit code 2
       uniquely means "HF_TOKEN unset")
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import os
import shutil
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

SPACE_ID = "microsoft/TRELLIS.2"
REQUIRED_ENDPOINTS = ("/preprocess_image", "/image_to_3d", "/extract_glb")

SCRIPT_DIR = Path(__file__).resolve().parent
INBOX_DIR = SCRIPT_DIR / "Inbox"
CACHE_DIR = SCRIPT_DIR / "Cache"

DEFAULT_SEED = 0
DEFAULT_RESOLUTION = "1024"          # Space expects the choice as a string
DEFAULT_DECIMATION_TARGET = 500_000
DEFAULT_TEXTURE_SIZE = 2048
MIN_TIMEOUT_MINUTES = 20             # ruling: >= 20-minute timeouts per GPU call
DEFAULT_TIMEOUT_MINUTES = 30
DEFAULT_ATTEMPTS = 3
BACKOFF_BASE_SECONDS = 20            # 20s, 60s, 180s ...

IMAGE_EXTS = (".png", ".jpg", ".jpeg", ".webp", ".bmp")

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
    print(f"[trellis] {redact(msg)}", flush=True)


def warn(msg: str) -> None:
    print(f"[trellis][WARN] {redact(msg)}", file=sys.stderr, flush=True)


def fail(msg: str) -> None:
    print(f"[trellis][ERROR] {redact(msg)}", file=sys.stderr, flush=True)


# ---------------------------------------------------------------------------
# Error classes
# ---------------------------------------------------------------------------


class ApiDriftError(RuntimeError):
    """The Space no longer exposes the endpoints/schema this script expects."""


class QuotaExceededError(RuntimeError):
    """ZeroGPU quota exhausted - an expected pause, not a code failure."""


def _looks_like_quota_error(text: str) -> bool:
    lowered = text.lower()
    return ("quota" in lowered and "gpu" in lowered) or "zerogpu" in lowered


def _print_quota_guidance(verbatim: str) -> None:
    # Ruling 8: surface the Space's quota message VERBATIM (it contains the
    # reset time) and treat this as an expected pause, not a blocker.
    fail("ZeroGPU quota exceeded. Space message, verbatim:")
    fail(f"    {verbatim}")
    fail("Guidance (manager ruling 8):")
    fail("  - The message above includes the quota reset time - record it in the")
    fail("    task handoff and resume in the next window.")
    fail("  - Free ZeroGPU tier is ~5 GPU-min/day (~1-2 assets/day).")
    fail("  - HF PRO (~$9/mo, 40 GPU-min/day) is recommended before the M7 batch.")
    fail("  - Escalate only if blocked for more than 48 h.")


# ---------------------------------------------------------------------------
# API discovery / drift handling
# ---------------------------------------------------------------------------


def discover_api(client) -> dict:
    """view_api() at runtime; assert the three required endpoints exist."""
    api = client.view_api(return_format="dict", all_endpoints=True, print_info=False)
    if not isinstance(api, dict):
        raise ApiDriftError(f"view_api() returned {type(api).__name__}, expected dict")
    named = api.get("named_endpoints") or {}
    missing = [ep for ep in REQUIRED_ENDPOINTS if ep not in named]
    if missing:
        raise ApiDriftError(
            "Space API drift - required endpoint(s) missing: "
            f"{', '.join(missing)}. Endpoints currently exposed: "
            f"{', '.join(sorted(named.keys())) or '(none)'}. "
            "Fallback: Jonathan runs the Space in a browser and drops the GLB at "
            "Cache/<AssetName>/trellis_raw.glb (see README.md); Stage 2 resumes."
        )
    return api


def snapshot_schema(api: dict, dest: Path) -> None:
    """Write the discovered schema beside the output for QA/build-master evidence.

    Contains only public Space metadata - no credential material.
    """
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(
        json.dumps(api, indent=2, default=str, sort_keys=True), encoding="utf-8"
    )
    say(f"API schema snapshot written: {dest}")


def _endpoint_parameters(api: dict, endpoint: str) -> list[dict]:
    return (api.get("named_endpoints", {}).get(endpoint, {}) or {}).get(
        "parameters", []
    ) or []


def _parameter_names(api: dict, endpoint: str) -> list[str]:
    return [
        p.get("parameter_name")
        for p in _endpoint_parameters(api, endpoint)
        if p.get("parameter_name")
    ]


def _find_image_parameter(api: dict, endpoint: str) -> str | None:
    """Locate the image-input parameter of an endpoint by schema inspection."""
    for p in _endpoint_parameters(api, endpoint):
        blob = " ".join(
            str(p.get(key, ""))
            for key in ("component", "python_type", "parameter_name", "type")
        ).lower()
        if "image" in blob or "filepath" in blob or "file" in blob:
            return p.get("parameter_name")
    return None


def _filtered_kwargs(api: dict, endpoint: str, desired: dict) -> dict:
    """Keep only the desired kwargs whose names exist on the live endpoint.

    Any name the Space no longer exposes is a soft API drift: warn loudly, let
    the Space default apply, and rely on the schema snapshot for evidence.
    """
    available = set(_parameter_names(api, endpoint))
    kwargs = {}
    for name, value in desired.items():
        if name in available:
            kwargs[name] = value
        else:
            warn(
                f"API drift: parameter '{name}' not found on {endpoint} "
                f"(live params: {sorted(available)}). The Space default will apply; "
                "value recorded in state.json for reproducibility."
            )
    return kwargs


def _extract_files(result: object, exts: tuple[str, ...]) -> list[str]:
    """Recursively collect file paths with the given extensions from a result."""
    found: list[str] = []

    def visit(node: object) -> None:
        if isinstance(node, str):
            if node.lower().endswith(exts):
                found.append(node)
        elif isinstance(node, dict):
            for value in node.values():
                visit(value)
        elif isinstance(node, (list, tuple)):
            for item in node:
                visit(item)

    visit(result)
    return found


# ---------------------------------------------------------------------------
# Robust endpoint invocation (retries + backoff + quota surfacing)
# ---------------------------------------------------------------------------


def call_endpoint(
    client,
    api_name: str,
    args: tuple,
    kwargs: dict,
    timeout_seconds: float,
    attempts: int,
):
    """submit() + result(timeout) with exponential backoff.

    - Quota errors are NOT retried (retrying spends nothing but time - the
      window has to reset); they raise QuotaExceededError with the verbatim text.
    - Everything else (transport errors, timeouts, transient Space errors)
      retries up to `attempts` with backoff 20s -> 60s -> 180s.
    """
    last_error: Exception | None = None
    for attempt in range(1, attempts + 1):
        say(f"{api_name} - attempt {attempt}/{attempts} (timeout {timeout_seconds:.0f}s)")
        started = time.monotonic()
        job = None
        try:
            job = client.submit(*args, api_name=api_name, **kwargs)
            result = job.result(timeout=timeout_seconds)
            say(f"{api_name} - done in {time.monotonic() - started:.1f}s")
            return result
        except Exception as exc:  # noqa: BLE001 - gradio_client raises many types
            text = redact(f"{type(exc).__name__}: {exc}")
            if job is not None:
                try:
                    job.cancel()
                except Exception:  # noqa: BLE001 - best-effort cancel only
                    pass
            if _looks_like_quota_error(text):
                raise QuotaExceededError(text) from None
            last_error = exc
            if attempt < attempts:
                delay = BACKOFF_BASE_SECONDS * (3 ** (attempt - 1))
                warn(f"{api_name} failed ({text}); retrying in {delay}s")
                time.sleep(delay)
    raise RuntimeError(
        f"{api_name} failed after {attempts} attempts: "
        f"{redact(f'{type(last_error).__name__}: {last_error}')}"
    )


# ---------------------------------------------------------------------------
# Input validation
# ---------------------------------------------------------------------------


def validate_concept(path: Path) -> dict:
    """Verify the concept image is readable; return metadata incl. sha256."""
    if not path.is_file():
        raise FileNotFoundError(
            f"Concept image not found: {path}\n"
            f"Drop the concept PNG at Inbox/{path.name} (guidance in README.md: "
            "single subject, plain/transparent background, 3/4 view, no ground shadow)."
        )
    from PIL import Image  # local import: keeps --help dependency-free

    with Image.open(path) as img:
        img.verify()  # detects truncated/corrupt files
    with Image.open(path) as img:
        width, height = img.size
        mode = img.mode
    if width != height:
        warn(f"Concept image is not square ({width}x{height}) - TRELLIS accepts it, "
             "but a square, subject-filling frame generates best.")
    if min(width, height) < 512:
        warn(f"Concept image is small ({width}x{height}); >=1024px recommended.")
    sha256 = hashlib.sha256(path.read_bytes()).hexdigest()
    return {
        "file": path.name,
        "width": width,
        "height": height,
        "mode": mode,
        "sha256": sha256,
    }


# ---------------------------------------------------------------------------
# State file
# ---------------------------------------------------------------------------


def write_state(asset_dir: Path, state: dict, filename: str = "state.json") -> None:
    """Write run provenance.

    QA WARN-2 scheme: `state.json` always describes the LAST SUCCESSFUL run -
    it is the provenance of the trellis_raw.glb sitting beside it. Failure
    paths (quota / drift / generic) write `state_failed.json` instead, so a
    failed seed re-roll can never clobber the success record of a GLB that
    survived. Stage 2 reads only the GLB; neither file is load-bearing
    downstream.
    """
    asset_dir.mkdir(parents=True, exist_ok=True)
    dest = asset_dir / filename
    dest.write_text(json.dumps(state, indent=2, default=str), encoding="utf-8")
    say(f"State written: {dest}")


def _utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


# ---------------------------------------------------------------------------
# Modes
# ---------------------------------------------------------------------------


def run_check(asset: str | None) -> int:
    """--check: TOKENLESS smoke test - Space reachability + endpoint schema assert.

    Deliberately connects WITHOUT a token (public Space metadata only): this is
    the build-master smoke test and must work on a machine with no HF_TOKEN set.
    No GPU call is made and no quota is spent.
    """
    import inspect

    from gradio_client import Client  # local import: keeps --help dependency-free

    # OFFLINE kwarg-drift guard (TASK-082 QA-loop 2): the authenticated path in
    # run_generate() passes token=<HF_TOKEN> to Client(). gradio_client 2.5.0
    # renamed that kwarg (hf_token -> token) and the old tokenless --check could
    # not see it - the first live run exploded instead. Assert the kwarg exists
    # BEFORE any network I/O so future renames fail --check tokenless + offline.
    client_params = inspect.signature(Client.__init__).parameters
    if "token" not in client_params:
        fail(
            "gradio_client kwarg drift: Client.__init__ no longer accepts 'token' "
            f"(live params: {', '.join(client_params)}). run_generate() passes "
            "token=<HF_TOKEN>, so real generation WOULD fail. Update the Client() "
            "construction in run_generate() to the new auth kwarg name."
        )
        return 1
    say("--check: offline signature assert OK - Client.__init__ accepts 'token'.")

    say(f"--check: connecting to {SPACE_ID} (tokenless; no GPU call, no quota spend)")
    try:
        client = Client(SPACE_ID)
        api = discover_api(client)
    except ApiDriftError as exc:
        fail(str(exc))
        return 4
    except Exception as exc:  # noqa: BLE001
        fail(f"Space unreachable: {type(exc).__name__}: {exc}")
        return 1

    for endpoint in REQUIRED_ENDPOINTS:
        say(f"  {endpoint} - OK (params: {', '.join(_parameter_names(api, endpoint)) or '(none)'})")

    snapshot_dest = (
        CACHE_DIR / asset / "api_schema.json" if asset else CACHE_DIR / "api_schema.json"
    )
    snapshot_schema(api, snapshot_dest)
    say("--check PASSED: Space reachable, all three endpoints present.")
    return 0


def run_generate(args: argparse.Namespace) -> int:
    """Full Stage-1 run: preprocess -> image_to_3d -> extract_glb, atomically."""
    # ---- Token: ENV-ONLY (manager ruling 1). Exit 2 with setup help if unset.
    token = os.environ.get("HF_TOKEN")
    if not token:
        fail("HF_TOKEN is not set in the environment (required for generation).")
        fail("Setup (one-time): Windows Settings > System > About > Advanced system")
        fail("settings > Environment Variables > add user variable HF_TOKEN with your")
        fail("Hugging Face access token (huggingface.co/settings/tokens), then open a")
        fail("NEW terminal. The token lives ONLY in the environment: never put it in a")
        fail("file, on a command line, or in chat. (--check runs without a token.)")
        return 2
    _register_secret(token)

    asset = args.asset
    concept_path = INBOX_DIR / f"{asset}.png"
    asset_dir = CACHE_DIR / asset
    # Working dirs are created on demand (gitignore entries are TASK-084's).
    INBOX_DIR.mkdir(parents=True, exist_ok=True)
    asset_dir.mkdir(parents=True, exist_ok=True)

    try:
        concept = validate_concept(concept_path)
    except FileNotFoundError as exc:
        fail(str(exc))
        return 5
    except Exception as exc:  # noqa: BLE001 - Pillow raises various decode errors
        fail(f"Concept image unreadable: {type(exc).__name__}: {exc}")
        return 5
    say(f"Concept OK: {concept_path} ({concept['width']}x{concept['height']}, "
        f"sha256 {concept['sha256'][:12]}...)")

    timeout_seconds = args.timeout_minutes * 60.0

    from gradio_client import Client, handle_file  # local import (see --help note)
    import gradio_client as _gradio_client_module

    state: dict = {
        "asset": asset,
        "space": SPACE_ID,
        "started_utc": _utc_now(),
        "concept": concept,
        "params": {
            "seed": args.seed,
            "resolution": args.resolution,
            "decimation_target": args.decimation_target,
            "texture_size": args.texture_size,
        },
        "timeout_minutes": args.timeout_minutes,
        "attempts": args.attempts,
        "gradio_client_version": getattr(_gradio_client_module, "__version__", "unknown"),
        "status": "in-progress",
    }

    try:
        # ONE Client for the whole run: the Space parks the generated model in
        # gr.State, which is coupled to THIS client session. Splitting
        # preprocess/generate/extract across Clients or script runs loses the
        # model between calls - never do it.
        say(f"Connecting to {SPACE_ID} (authenticated via HF_TOKEN from env)")
        # gradio_client 2.5.0 renamed the auth kwarg hf_token -> token
        # (TASK-082 QA-loop 2); --check's offline signature assert guards this.
        client = Client(SPACE_ID, token=token)

        api = discover_api(client)
        snapshot_schema(api, asset_dir / "api_schema.json")

        # ---- 1/3 preprocess_image -------------------------------------------------
        image_param = _find_image_parameter(api, "/preprocess_image")
        if image_param is None:
            raise ApiDriftError(
                "/preprocess_image exposes no image-like parameter "
                "(see api_schema.json snapshot)."
            )
        pre_result = call_endpoint(
            client,
            "/preprocess_image",
            args=(),
            kwargs={image_param: handle_file(str(concept_path))},
            timeout_seconds=timeout_seconds,
            attempts=args.attempts,
        )
        pre_images = _extract_files(pre_result, IMAGE_EXTS)
        if pre_images:
            processed_path = pre_images[0]
            say(f"Preprocessed image: {Path(processed_path).name}")
        else:
            processed_path = str(concept_path)
            warn("/preprocess_image returned no image file; passing the original "
                 "concept to /image_to_3d (result shape recorded in state.json).")
            state["preprocess_result_shape"] = redact(repr(pre_result))[:500]

        # ---- 2/3 image_to_3d (GPU) ------------------------------------------------
        gen_kwargs = _filtered_kwargs(
            api, "/image_to_3d", {"seed": args.seed, "resolution": args.resolution}
        )
        gen_image_param = _find_image_parameter(api, "/image_to_3d")
        if gen_image_param and gen_image_param not in gen_kwargs:
            gen_kwargs[gen_image_param] = handle_file(processed_path)
        elif gen_image_param is None:
            # Session-state-fed variant: the Space reads the preprocessed image
            # from gr.State (same Client session), so no image arg is needed.
            say("/image_to_3d has no image parameter - relying on session state "
                "from /preprocess_image (same Client).")
        gen_result = call_endpoint(
            client,
            "/image_to_3d",
            args=(),
            kwargs=gen_kwargs,
            timeout_seconds=timeout_seconds,
            attempts=args.attempts,
        )
        state["image_to_3d_result_shape"] = redact(repr(gen_result))[:500]

        # ---- 3/3 extract_glb (GPU) ------------------------------------------------
        extract_kwargs = _filtered_kwargs(
            api,
            "/extract_glb",
            {
                "decimation_target": args.decimation_target,
                "texture_size": args.texture_size,
            },
        )
        extract_result = call_endpoint(
            client,
            "/extract_glb",
            args=(),
            kwargs=extract_kwargs,
            timeout_seconds=timeout_seconds,
            attempts=args.attempts,
        )

        glb_files = _extract_files(extract_result, (".glb",))
        if not glb_files:
            state["extract_result_shape"] = redact(repr(extract_result))[:500]
            raise RuntimeError(
                "/extract_glb returned no .glb file path "
                "(result shape recorded in state_failed.json)."
            )
        # The Space typically returns (viewer_model, download_file); the last
        # GLB is the downloadable artifact.
        source_glb = Path(glb_files[-1])
        dest_glb = asset_dir / "trellis_raw.glb"
        shutil.copyfile(source_glb, dest_glb)
        state["status"] = "success"
        state["finished_utc"] = _utc_now()
        state["output_glb"] = str(dest_glb)
        state["output_glb_bytes"] = dest_glb.stat().st_size
        write_state(asset_dir, state)
        say(f"SUCCESS: {dest_glb} ({state['output_glb_bytes']:,} bytes). "
            "Next: Stage 2 headless refine (see README.md).")
        return 0

    # Failure paths write state_failed.json, NEVER state.json (QA WARN-2):
    # a prior successful run's state.json must keep describing the surviving
    # trellis_raw.glb even after a failed seed re-roll.
    except QuotaExceededError as exc:
        state["status"] = "quota-blocked"
        state["finished_utc"] = _utc_now()
        state["error"] = redact(str(exc))
        write_state(asset_dir, state, filename="state_failed.json")
        _print_quota_guidance(str(exc))
        return 3
    except ApiDriftError as exc:
        state["status"] = "api-drift"
        state["finished_utc"] = _utc_now()
        state["error"] = redact(str(exc))
        write_state(asset_dir, state, filename="state_failed.json")
        fail(str(exc))
        return 4
    except Exception as exc:  # noqa: BLE001
        state["status"] = "failed"
        state["finished_utc"] = _utc_now()
        state["error"] = redact(f"{type(exc).__name__}: {exc}")
        write_state(asset_dir, state, filename="state_failed.json")
        fail(f"Generation failed: {type(exc).__name__}: {exc}")
        return 1


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


class _Parser(argparse.ArgumentParser):
    """argparse exits 2 on usage errors by default; remap to 64 (EX_USAGE) so
    exit code 2 UNIQUELY means 'HF_TOKEN unset' (board-spec contract)."""

    def error(self, message: str):  # noqa: D102 - argparse override
        self.print_usage(sys.stderr)
        # QA WARN-1: argparse usage messages mirror raw argv back (unrecognized
        # arguments / invalid values). redact() the message so a token
        # accidentally pasted on the command line is never echoed - the generic
        # hf_ pattern works even before any secret is registered.
        self.exit(64, f"{self.prog}: error: {redact(message)}\n")


def build_parser() -> argparse.ArgumentParser:
    parser = _Parser(
        prog="trellis_generate.py",
        description=(
            "Stage 1 of the Siegebound art pipeline: generate a raw textured GLB "
            f"from Inbox/<AssetName>.png via the Hugging Face Space {SPACE_ID}. "
            "Outputs Cache/<AssetName>/trellis_raw.glb + state.json + api_schema.json."
        ),
        epilog=(
            "HF_TOKEN is read from the environment ONLY (never accepted as an "
            "argument, never written or logged). Exit codes: 0 success, 1 failure, "
            "2 HF_TOKEN unset, 3 ZeroGPU quota exhausted, 4 Space API drift, "
            "5 concept image missing/unreadable, 64 CLI usage error. "
            "Examples: `uv run trellis_generate.py Footman --seed 3`, "
            "`uv run trellis_generate.py --check`."
        ),
    )
    parser.add_argument(
        "asset",
        nargs="?",
        metavar="AssetName",
        help="Asset name; reads Inbox/<AssetName>.png, writes Cache/<AssetName>/. "
             "Required unless --check.",
    )
    parser.add_argument(
        "--seed", type=int, default=DEFAULT_SEED,
        help=f"Generation seed (default {DEFAULT_SEED}); recorded in state.json. "
             "Reroll on a bad generation.",
    )
    parser.add_argument(
        "--resolution", default=DEFAULT_RESOLUTION,
        help=f"image_to_3d resolution choice, passed as a string (default "
             f"\"{DEFAULT_RESOLUTION}\").",
    )
    parser.add_argument(
        "--decimation-target", type=int, default=DEFAULT_DECIMATION_TARGET,
        help=f"extract_glb decimation target in triangles (default "
             f"{DEFAULT_DECIMATION_TARGET}; Stage 2 does the real budget cut).",
    )
    parser.add_argument(
        "--texture-size", type=int, default=DEFAULT_TEXTURE_SIZE,
        help=f"extract_glb texture size in px (default {DEFAULT_TEXTURE_SIZE}).",
    )
    parser.add_argument(
        "--timeout-minutes", type=float, default=DEFAULT_TIMEOUT_MINUTES,
        help=f"Per-endpoint-call timeout in minutes (default "
             f"{DEFAULT_TIMEOUT_MINUTES}; hard minimum {MIN_TIMEOUT_MINUTES} - "
             "ZeroGPU queues can be long).",
    )
    parser.add_argument(
        "--attempts", type=int, default=DEFAULT_ATTEMPTS,
        help=f"Retry attempts per endpoint call with exponential backoff "
             f"(default {DEFAULT_ATTEMPTS}). Quota errors are never retried.",
    )
    parser.add_argument(
        "--check", action="store_true",
        help="TOKENLESS smoke test: connect to the Space and assert the three "
             "endpoints exist (writes an api_schema.json snapshot). No GPU call, "
             "no quota spend, no HF_TOKEN needed.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)

    if args.timeout_minutes < MIN_TIMEOUT_MINUTES:
        warn(f"--timeout-minutes {args.timeout_minutes} is below the "
             f"{MIN_TIMEOUT_MINUTES}-minute ruling; clamping to {MIN_TIMEOUT_MINUTES}.")
        args.timeout_minutes = float(MIN_TIMEOUT_MINUTES)
    if args.attempts < 1:
        warn("--attempts must be >= 1; using 1.")
        args.attempts = 1

    if args.check:
        return run_check(args.asset)
    if not args.asset:
        build_parser().error("AssetName is required unless --check is given.")
    return run_generate(args)


if __name__ == "__main__":
    sys.exit(main())
