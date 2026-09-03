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

Artefact law (CONVENTIONS.md SC-39.1, TASK-876):
    A tool that produces an artefact VALIDATES that artefact before reporting
    success, and a write NEVER destroys the prior good artefact before the new
    one validates. This script therefore runs stage -> validate -> swap:

        PHASE 1  stage_glb()      copy the Space's GLB to .trellis_raw.glb.tmp-<pid>
                                  BESIDE the destination; the destination is untouched
        PHASE 2  validate_glb_file()  re-read the staged file FROM DISK and assess it
        PHASE 3  commit_staged()  os.replace - reached ONLY when no check failed

    A rejected GLB is QUARANTINED to Cache/<AssetName>/_rejected/ (never deleted,
    never overwritten by a later rejection) so the evidence survives for
    inspection. Cache/ is gitignored, so quarantined artefacts never enter git.

Exit codes:
    0  success (or --check passed)
    1  generic failure (network/generation/output errors after retries)
    2  HF_TOKEN not set in the environment
    3  ZeroGPU quota exhausted (error surfaced verbatim + reset guidance)
    4  API drift: required endpoints/schema not found on the Space
    5  input concept image missing or unreadable
    6  DEGENERATE ARTEFACT: the Space returned a GLB that is unreadable, a
       truncated container, mesh-less, geometry-less or collapsed to a
       plane/point. The call SUCCEEDED and the Space is fine - the roll is not.
       The prior trellis_raw.glb is UNTOUCHED; the rejected GLB is in
       Cache/<AssetName>/_rejected/. Re-roll with a different --seed.
       (Same value and meaning as concept_generate.py's exit 6 - one family.)
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
import struct
import sys
import tempfile
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

# --- Artefact guard (SC-39.1, TASK-876) ------------------------------------
EXIT_DEGENERATE = 6                  # measured absent from BOTH siblings; see docstring
REJECTED_DIR_NAME = "_rejected"

GLB_MAGIC = 0x46546C67               # 'glTF' little-endian
GLB_CHUNK_JSON = 0x4E4F534A          # 'JSON'
GLB_CHUNK_BIN = 0x004E4942           # 'BIN\0'
GLB_HEADER_BYTES = 12

# THRESHOLDS ARE DERIVED, NOT PICKED (SC-40 cl. 10). A derived floor cannot go
# stale when the data moves, which a corpus-fitted one does.
#   MIN_MESH_VERTICES / MIN_MESH_TRIANGLES = the tetrahedron - the smallest
#   closed solid that exists in 3-space. This is the floor of "is it a solid at
#   all", NOT a quality bar; quality is Stage 2's tri_budget.
#   FLAT_ASPECT_FLOOR = a relative epsilon. A collapsed mesh has an aspect of
#   EXACTLY 0.0 (every POSITION on one plane), so any value in (0, corpus-worst)
#   separates them; 1e-4 sits ~3 orders of magnitude above float32 coordinate
#   noise at unit scale and is nowhere near a real asset.
# MEASURED HEADROOM over the 126 real GLBs under Cache/ (TASK-876 sweep, of
# which 19 are this script's own trellis_raw.glb output):
#   vertices   worst real = 11,820   -> 2,955x above the floor
#   triangles  worst real =  9,461   -> 2,365x above the floor
#   aspect     worst real =  0.2652  -> 2,652x above the floor
#   126/126 parsed clean, 126/126 accepted by this guard (positive control:
#   the parser CAN read a real GLB, so a rejection means something).
MIN_MESH_VERTICES = 4
MIN_MESH_TRIANGLES = 4
FLAT_ASPECT_FLOOR = 1e-4

# BYTE SIZE IS DELIBERATELY *NOT* A GATE, and this is the whole point of the
# task: the defect this guard replaces reported SUCCESS on `output_glb_bytes`
# alone. The TASK-876 repro destroyed a 20,380,016-byte good mesh with a
# 428-byte one, so a size floor would have caught THAT case - but a degenerate
# GLB can be arbitrarily large (a 300k-vertex mesh collapsed to a plane weighs
# megabytes) and a legitimate simple prop can be tiny. Size stays a PRINTED
# DIAGNOSTIC and decides nothing.

CHECK_UNREADABLE = "unreadable-artefact"
CHECK_TRUNCATED = "truncated-container"
CHECK_NO_MESH = "no-mesh"
CHECK_NO_GEOMETRY = "no-geometry"
CHECK_FLAT_BOUNDS = "degenerate-bounds"

ALL_GLB_CHECKS = (
    CHECK_UNREADABLE,
    CHECK_TRUNCATED,
    CHECK_NO_MESH,
    CHECK_NO_GEOMETRY,
    CHECK_FLAT_BOUNDS,
)

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
# Artefact validation: is the GLB the Space handed back actually a MESH?
# (SC-39.1 cl. 1 - "the API returned 200" is not success)
#
# Pure stdlib: a GLB is a 12-byte header + length-prefixed chunks, and the
# JSON chunk is the whole glTF document. No new dependency, and deliberately
# NOT a full glTF validator - this answers one question only: "did the Space
# hand back a mesh, or a shell?"
#
# DECLARED SKIP RULES (SC-39's "a test that measures declares its skip rules"):
#   - the geometry and bounds checks are NOT evaluated when the document has
#     ZERO primitives. They have no domain there, and `no-mesh` already fires.
#   - the bounds check is NOT evaluated when no POSITION accessor supplies
#     min/max. glTF requires them and 126/126 real GLBs carry them, so this is
#     a defensive skip, not an expected path; validate_glb_file() WARNS on it
#     rather than passing silently.
# ---------------------------------------------------------------------------


class DegenerateArtefactError(RuntimeError):
    """The artefact was produced, and it is not a usable mesh (exit 6)."""

    def __init__(self, checks: list[str], stats: dict, path: Path | None = None):
        self.checks = list(checks)
        self.stats = stats
        self.path = path
        super().__init__(", ".join(self.checks) or "degenerate")


def measure_glb(data: bytes) -> dict:
    """Parse a GLB byte string into the statistics the checks are computed on.

    PURE: no I/O, no exceptions for degenerate input - a malformed container is
    RECORDED in the stats (`unreadable_reason` / `truncated_reason`) so that
    assess_degeneracy() remains a total function of the stats. That split is
    what lets the positive control drive every check deterministically.
    """
    stats: dict = {
        "bytes": len(data),
        "declared_bytes": None,
        "version": None,
        "meshes": 0,
        "primitives": 0,
        "vertices": 0,
        "triangles": 0,
        "images": 0,
        "materials": 0,
        "bounds_known": False,
        "extent": None,
        "max_extent": None,
        "min_extent": None,
        "aspect": None,
        "unreadable_reason": None,
        "truncated_reason": None,
    }

    if len(data) < GLB_HEADER_BYTES:
        stats["unreadable_reason"] = f"file is {len(data)} bytes; a GLB header is 12"
        return stats
    magic, version, declared = struct.unpack("<III", data[:GLB_HEADER_BYTES])
    stats["version"] = version
    stats["declared_bytes"] = declared
    if magic != GLB_MAGIC:
        stats["unreadable_reason"] = (
            f"not a GLB: magic 0x{magic:08X}, expected 0x{GLB_MAGIC:08X} ('glTF')"
        )
        return stats
    if declared != len(data):
        # The download/copy completed but the payload is not the whole file.
        # This is the truncation a `.part` swap CANNOT see: the stream simply
        # ended early and nothing raised.
        stats["truncated_reason"] = (
            f"header declares {declared:,} bytes, file is {len(data):,}"
        )

    document = None
    offset = GLB_HEADER_BYTES
    while offset + 8 <= len(data):
        chunk_length, chunk_type = struct.unpack("<II", data[offset:offset + 8])
        body = data[offset + 8:offset + 8 + chunk_length]
        if len(body) < chunk_length:
            if chunk_type == GLB_CHUNK_JSON:
                stats["unreadable_reason"] = (
                    f"JSON chunk truncated: declared {chunk_length:,} bytes, "
                    f"{len(body):,} present"
                )
                return stats
            stats["truncated_reason"] = stats["truncated_reason"] or (
                f"chunk 0x{chunk_type:08X} truncated: declared {chunk_length:,} "
                f"bytes, {len(body):,} present"
            )
            break
        if chunk_type == GLB_CHUNK_JSON and document is None:
            try:
                document = json.loads(body.decode("utf-8"))
            except Exception as exc:  # noqa: BLE001 - any decode failure is the same verdict
                stats["unreadable_reason"] = (
                    f"JSON chunk is not decodable glTF: {type(exc).__name__}: {exc}"
                )
                return stats
        # chunkLength already includes the mandatory 4-byte padding; round up
        # anyway so a non-conforming writer cannot desynchronise the walk.
        offset += 8 + chunk_length + ((4 - chunk_length % 4) % 4)

    if not isinstance(document, dict):
        stats["unreadable_reason"] = "no JSON chunk in the GLB container"
        return stats

    accessors = document.get("accessors") or []
    meshes = document.get("meshes") or []
    stats["meshes"] = len(meshes)
    stats["images"] = len(document.get("images") or [])
    stats["materials"] = len(document.get("materials") or [])

    low = [float("inf")] * 3
    high = [float("-inf")] * 3
    for mesh in meshes:
        for primitive in (mesh.get("primitives") or []):
            stats["primitives"] += 1
            # mode 4 == TRIANGLES (the glTF default). Point/line primitives
            # contribute vertices but no triangles, which is exactly right.
            mode = primitive.get("mode", 4)
            position = (primitive.get("attributes") or {}).get("POSITION")
            vertex_count = 0
            if isinstance(position, int) and 0 <= position < len(accessors):
                accessor = accessors[position]
                vertex_count = int(accessor.get("count") or 0)
                stats["vertices"] += vertex_count
                minimum, maximum = accessor.get("min"), accessor.get("max")
                if (isinstance(minimum, list) and len(minimum) == 3
                        and isinstance(maximum, list) and len(maximum) == 3):
                    stats["bounds_known"] = True
                    for axis in range(3):
                        low[axis] = min(low[axis], float(minimum[axis]))
                        high[axis] = max(high[axis], float(maximum[axis]))
            indices = primitive.get("indices")
            if isinstance(indices, int) and 0 <= indices < len(accessors):
                element_count = int(accessors[indices].get("count") or 0)
            else:
                element_count = vertex_count
            if mode == 4:
                stats["triangles"] += element_count // 3

    if stats["bounds_known"]:
        extent = [high[axis] - low[axis] for axis in range(3)]
        stats["extent"] = extent
        stats["max_extent"] = max(extent)
        stats["min_extent"] = min(extent)
        stats["aspect"] = (
            (stats["min_extent"] / stats["max_extent"]) if stats["max_extent"] > 0 else 0.0
        )
    return stats


def assess_degeneracy(stats: dict) -> list[str]:
    """PURE: stats -> the list of checks that FAILED (empty list == usable).

    Order is stable so the positive control can assert an EXACT SET rather than
    a boolean - with a boolean, a dead check hides behind a live one.
    """
    failed: list[str] = []
    if stats.get("unreadable_reason"):
        # Nothing else can be judged about bytes we could not parse.
        return [CHECK_UNREADABLE]
    if stats.get("truncated_reason"):
        failed.append(CHECK_TRUNCATED)
    if stats.get("primitives", 0) <= 0:
        # SKIP RULE: geometry and bounds have no domain without a primitive.
        failed.append(CHECK_NO_MESH)
        return failed
    if (stats.get("vertices", 0) < MIN_MESH_VERTICES
            or stats.get("triangles", 0) < MIN_MESH_TRIANGLES):
        failed.append(CHECK_NO_GEOMETRY)
    if stats.get("bounds_known"):
        if not stats.get("max_extent") or stats["max_extent"] <= 0.0:
            failed.append(CHECK_FLAT_BOUNDS)
        elif stats.get("aspect", 0.0) < FLAT_ASPECT_FLOOR:
            failed.append(CHECK_FLAT_BOUNDS)
    return failed


def describe_glb(stats: dict) -> str:
    """One-line human summary; byte size appears here as a DIAGNOSTIC only."""
    if stats.get("unreadable_reason"):
        return f"{stats['bytes']:,} bytes, unreadable ({stats['unreadable_reason']})"
    extent = stats.get("extent")
    extent_text = (
        "x".join(f"{value:.3f}" for value in extent) if extent else "bounds unknown"
    )
    return (
        f"{stats['bytes']:,} bytes, {stats['meshes']} mesh(es)/"
        f"{stats['primitives']} primitive(s), {stats['vertices']:,} verts, "
        f"{stats['triangles']:,} tris, bounds {extent_text}, "
        f"aspect {stats.get('aspect') if stats.get('aspect') is None else round(stats['aspect'], 4)}, "
        f"{stats['images']} image(s)"
    )


def validate_glb_file(path: Path, quiet: bool = False) -> dict:
    """PHASE 2 - re-read the artefact FROM DISK and assess it.

    Reading from disk (rather than trusting the bytes we just wrote) is what
    catches the `save_asset` shape named beside this defect in SC-39.1: a write
    that "succeeded" while producing a zero-byte or unreadable file.

    `quiet` suppresses the skip-rule warning only; it never changes a verdict.
    The positive control sets it, because several of its fixtures are SUPPOSED
    to be unparseable and a warning per fixture would bury the real output.

    Raises DegenerateArtefactError; returns the stats on success.
    """
    try:
        data = path.read_bytes()
    except OSError as exc:
        # A file that vanished, was never created, or cannot be read at all.
        # CONTROLLED by run_guard_self_test()'s missing-file fixture - deleting
        # this except clause turns the control RED (SC-39.1 cl. 6).
        stats = {"bytes": -1, "unreadable_reason": f"{type(exc).__name__}: {exc}"}
        raise DegenerateArtefactError([CHECK_UNREADABLE], stats, path) from None
    stats = measure_glb(data)
    failed = assess_degeneracy(stats)
    if not failed and not stats.get("bounds_known") and not quiet:
        warn(f"{path.name}: no POSITION accessor carries min/max, so the "
             "degenerate-bounds check was SKIPPED (glTF requires them; every "
             "one of the 126 reference GLBs has them). Inspect this mesh.")
    if failed:
        raise DegenerateArtefactError(failed, stats, path)
    return stats


# ---------------------------------------------------------------------------
# stage -> validate -> swap. The overwrite exists at EXACTLY ONE call site.
# ---------------------------------------------------------------------------


def _require_inside_cache(path: Path) -> Path:
    """Write confinement: this script only ever writes under Cache/."""
    resolved = Path(path).resolve()
    root = Path(CACHE_DIR).resolve()
    if not (resolved == root or root in resolved.parents):
        raise RuntimeError(f"Refusing to write outside Cache/: {resolved}")
    return resolved


def stage_glb(source: Path, dest: Path) -> Path:
    """PHASE 1 - copy the Space's GLB to a temp file BESIDE dest.

    dest is NOT touched. The caller MUST commit_staged(), quarantine_staged()
    or _discard_staged() the result.
    """
    dest = _require_inside_cache(dest)
    dest.parent.mkdir(parents=True, exist_ok=True)
    staged = dest.parent / f".{dest.name}.tmp-{os.getpid()}"
    try:
        shutil.copyfile(source, staged)
    except Exception:
        _discard_staged(staged)
        raise
    return staged


def commit_staged(staged: Path, dest: Path) -> None:
    """PHASE 3 - atomically swap a VALIDATED staged file into place.

    THE ONLY place in this script that writes trellis_raw.glb. Call it ONLY
    after validate_glb_file() returned without raising.
    """
    _require_inside_cache(dest)
    os.replace(str(staged), str(dest))  # atomic on the same filesystem


def _discard_staged(staged: Path) -> None:
    if staged.exists():
        try:
            staged.unlink()
        except OSError:
            pass


def quarantine_staged(staged: Path, asset: str, dest: Path) -> Path | None:
    """Move a REJECTED staged GLB to Cache/<asset>/_rejected/ - never delete it.

    A rejected mesh costs real GPU quota to produce; deleting it deletes the
    evidence. Best-effort by contract: a quarantine failure must never turn a
    clean rejection into a crash, and must never leave the temp file behind.
    """
    try:
        stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        folder = _require_inside_cache(dest).parent / REJECTED_DIR_NAME
        target = folder / f"{dest.stem}.{stamp}{dest.suffix}"
        # Two rejections inside the same second (a retry loop does exactly
        # that) must not overwrite each other.
        suffix = 1
        while target.exists():
            target = folder / f"{dest.stem}.{stamp}-{suffix}{dest.suffix}"
            suffix += 1
        _require_inside_cache(target)
        folder.mkdir(parents=True, exist_ok=True)
        os.replace(str(staged), str(target))
        return target
    except Exception as exc:  # noqa: BLE001 - quarantine is a convenience
        warn(f"{asset}: could not quarantine the rejected GLB ({exc}); discarding "
             "it. The existing artefact is still untouched.")
        _discard_staged(staged)
        return None


# ---------------------------------------------------------------------------
# The guard's POSITIVE CONTROL (SC-39 / SC-39.1 cl. 4 + cl. 6 / SHIP-9)
#
# It lives HERE and runs inside --check, not in a separate file somebody has to
# remember to run. Every fixture is a REAL FILE ON DISK driven through
# validate_glb_file(), so the on-disk check is controlled exactly like the
# in-memory ones - clause 6 exists because concept_generate.py's fifth check
# was the one whose fixture cost four more lines, and this is those four lines.
# ---------------------------------------------------------------------------


def _synth_glb(
    vertices: int,
    triangles: int,
    bounds_min=(0.0, 0.0, 0.0),
    bounds_max=(1.0, 1.0, 1.0),
    meshes: int = 1,
    declared_delta: int = 0,
    json_override: bytes | None = None,
) -> bytes:
    """Build a syntactically valid GLB 2.0 carrying the requested degeneracy.

    SYNTHESISED, never a real artefact and never a re-roll: proving the guard
    goes RED must cost zero quota (SC-39.1 cl. 4).
    """
    position_bytes = vertices * 12
    index_bytes = triangles * 3 * 2
    index_bytes += (4 - index_bytes % 4) % 4
    buffer = bytes(max(position_bytes + index_bytes, 4))

    document = {
        "asset": {"version": "2.0"},
        "scene": 0,
        "scenes": [{"nodes": list(range(meshes))}],
        "nodes": [{"mesh": index} for index in range(meshes)],
        "meshes": [
            {"primitives": [
                {"attributes": {"POSITION": 0}, "indices": 1, "mode": 4}
            ]} for _ in range(meshes)
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": vertices,
             "type": "VEC3", "min": list(bounds_min), "max": list(bounds_max)},
            {"bufferView": 1, "componentType": 5123, "count": triangles * 3,
             "type": "SCALAR"},
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": max(position_bytes, 1)},
            {"buffer": 0, "byteOffset": position_bytes,
             "byteLength": max(index_bytes, 1)},
        ],
        "buffers": [{"byteLength": len(buffer)}],
        "images": [{"uri": "synthetic.png"}],
        "materials": [{"name": "synthetic"}],
    }
    payload = json_override if json_override is not None else json.dumps(
        document, separators=(",", ":")
    ).encode("utf-8")
    payload += b" " * ((4 - len(payload) % 4) % 4)
    body = (
        struct.pack("<II", len(payload), GLB_CHUNK_JSON) + payload
        + struct.pack("<II", len(buffer), GLB_CHUNK_BIN) + buffer
    )
    total = GLB_HEADER_BYTES + len(body)
    return struct.pack("<III", GLB_MAGIC, 2, total + declared_delta) + body


def guard_fixtures() -> list[tuple[str, bytes | None, tuple[str, ...]]]:
    """(name, GLB bytes or None, the EXACT set of checks that must fire).

    A payload of None means THE FILE IS NEVER CREATED - that fixture exercises
    validate_glb_file()'s OSError branch, which is a DISTINCT raise site from
    the byte-level ones and is otherwise never reached by any fixture.

    EVERY check owns a fixture where it fires ALONE (SC-39.1 cl. 6):
        unreadable-artefact  <- missing-file / empty-file / not-a-glb /
                                json-chunk-garbage
        truncated-container  <- truncated-container
        no-mesh              <- no-mesh
        no-geometry          <- single-triangle
        degenerate-bounds    <- flat-plane

    AND BOTH DIRECTIONS ARE CONTROLLED - a guard that rejects everything must
    fail this suite just as loudly as one that accepts everything:
        minimal-tetrahedron has FEWER vertices (4) than the REJECTED flat-plane
        (6), so nothing keyed on size or count alone can pass one and fail the
        other. thin-but-legitimate is 26x thinner than the thinnest real asset
        in the 126-GLB corpus (aspect 0.010 vs 0.265) and must still be
        ACCEPTED, which is what forces the bounds check to key on "collapsed"
        rather than on "thin".
    """
    return [
        # ---- must be REJECTED ------------------------------------------------
        ("missing-file", None, (CHECK_UNREADABLE,)),
        ("empty-file", b"", (CHECK_UNREADABLE,)),
        ("not-a-glb", b"\xab" * 64, (CHECK_UNREADABLE,)),
        ("json-chunk-garbage",
         _synth_glb(64, 32, json_override=b"<<<not json at all>>>"),
         (CHECK_UNREADABLE,)),
        ("truncated-container",
         _synth_glb(64, 32, declared_delta=64), (CHECK_TRUNCATED,)),
        ("no-mesh", _synth_glb(0, 0, meshes=0), (CHECK_NO_MESH,)),
        ("single-triangle", _synth_glb(3, 1), (CHECK_NO_GEOMETRY,)),
        ("flat-plane",
         _synth_glb(6, 4, bounds_min=(0.0, 0.0, 0.0), bounds_max=(1.0, 1.0, 0.0)),
         (CHECK_FLAT_BOUNDS,)),
        # ---- must be ACCEPTED (the other direction) --------------------------
        ("minimal-tetrahedron", _synth_glb(4, 4), ()),
        ("thin-but-legitimate",
         _synth_glb(2048, 1024, bounds_min=(0.0, 0.0, 0.0),
                    bounds_max=(1.0, 1.0, 0.01)), ()),
        ("dense-healthy",
         _synth_glb(100_000, 60_000, bounds_min=(-0.5, -0.5, -0.5),
                    bounds_max=(0.5, 0.5, 0.5)), ()),
    ]


def _run_fixture_suite(directory: Path) -> list[str]:
    """Drive every fixture through validate_glb_file() from a REAL FILE.

    Returns the list of disagreements (empty == the guard behaves exactly as
    specified). Used both by the control and by the control's own meta-check.
    """
    disagreements: list[str] = []
    for name, payload, expected in guard_fixtures():
        path = directory / f"{name}.glb"
        if payload is None:
            # The missing-file fixture: ensure it does NOT exist.
            if path.exists():
                path.unlink()
        else:
            path.write_bytes(payload)
        try:
            validate_glb_file(path, quiet=True)
            fired: tuple[str, ...] = ()
        except DegenerateArtefactError as exc:
            fired = tuple(exc.checks)
        except Exception as exc:  # noqa: BLE001
            # The guard must REJECT, never explode. An unexpected exception is
            # a disagreement in its own right, reported rather than raised so
            # the control names the fixture instead of dying on it.
            disagreements.append(
                f"{name}: expected {list(expected) or 'ACCEPT'}, but the guard "
                f"RAISED {type(exc).__name__}: {exc}"
            )
            continue
        if fired != tuple(expected):
            disagreements.append(
                f"{name}: expected {list(expected) or 'ACCEPT'}, got "
                f"{list(fired) or 'ACCEPT'}"
            )
    return disagreements


def run_guard_self_test(verbose: bool = True) -> int:
    """0 == the guard is alive and behaves exactly as its fixtures specify.

    OFFLINE and TOKENLESS by construction, so it runs before any network I/O
    and cannot be skipped by an unreachable Space or an unset credential.
    """
    with tempfile.TemporaryDirectory(prefix="trellis-guard-") as scratch:
        scratch_dir = Path(scratch)
        disagreements = _run_fixture_suite(scratch_dir)
        if disagreements:
            for line in disagreements:
                fail(f"guard control: {line}")
            fail("GUARD CONTROL FAILED - the degeneracy guard does not behave as "
                 "specified. Generation is BLOCKED until this is fixed: an "
                 "unverified guard is indistinguishable from no guard.")
            return 1

        # ---- the control's OWN control: prove this suite can go RED ---------
        # A control only ever seen green is the same defect one level up. Both
        # directions are injected, because a guard that rejects everything is
        # the same bug wearing different clothes.
        original = globals()["assess_degeneracy"]
        try:
            globals()["assess_degeneracy"] = lambda stats: []
            never_rejects = len(_run_fixture_suite(scratch_dir))
            globals()["assess_degeneracy"] = lambda stats: list(ALL_GLB_CHECKS)
            always_rejects = len(_run_fixture_suite(scratch_dir))
        finally:
            globals()["assess_degeneracy"] = original
        restored = len(_run_fixture_suite(scratch_dir))

        if never_rejects <= 0 or always_rejects <= 0 or restored != 0:
            fail(f"guard control is not a live instrument: never-rejects gave "
                 f"{never_rejects} disagreements, always-rejects gave "
                 f"{always_rejects}, restored gave {restored} (expected >0, >0, 0).")
            return 1

    if verbose:
        fixtures = guard_fixtures()
        rejected = sum(1 for _, _, expected in fixtures if expected)
        say(f"--check: degeneracy guard control PASSED - {len(fixtures)} fixtures "
            f"({rejected} rejected, {len(fixtures) - rejected} accepted), all "
            f"{len(ALL_GLB_CHECKS)} checks fire in isolation, and the control "
            f"itself goes RED when the guard is broken ({never_rejects} "
            f"disagreements with a never-rejects guard, {always_rejects} with an "
            "always-rejects one).")
    return 0


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

    # STEP 1, and deliberately FIRST: the degeneracy guard's positive control.
    # Offline, tokenless, and ahead of every network call, so an unreachable
    # Space can never be the reason nobody noticed the guard had died
    # (SC-39.1 cl. 4 + cl. 6 - the control belongs in the ALWAYS-RUN tier).
    if run_guard_self_test() != 0:
        return 1

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

        # ---- stage -> validate -> swap (SC-39.1 cl. 3) ------------------------
        # PHASE 1: the Space's GLB lands BESIDE the destination. Whatever
        # happens next, the previous trellis_raw.glb is still on disk.
        staged_glb = stage_glb(source_glb, dest_glb)
        try:
            # PHASE 2: re-read the staged file FROM DISK and assess it. The
            # destination is STILL the old artefact at this point.
            glb_stats = validate_glb_file(staged_glb)
        except DegenerateArtefactError as exc:
            quarantined = quarantine_staged(staged_glb, asset, dest_glb)
            state["status"] = "degenerate-artefact"
            state["finished_utc"] = _utc_now()
            state["failed_checks"] = exc.checks
            state["artefact_stats"] = exc.stats
            state["quarantined_to"] = str(quarantined) if quarantined else None
            state["prior_artefact_preserved"] = dest_glb.is_file()
            write_state(asset_dir, state, filename="state_failed.json")
            fail(f"DEGENERATE ARTEFACT: the Space returned a GLB that is not a "
                 f"usable mesh. Failed checks: {', '.join(exc.checks)}.")
            fail(f"  measured: {describe_glb(exc.stats)}")
            if exc.stats.get("unreadable_reason"):
                fail(f"  reason: {exc.stats['unreadable_reason']}")
            if exc.stats.get("truncated_reason"):
                fail(f"  reason: {exc.stats['truncated_reason']}")
            if dest_glb.is_file():
                fail(f"  The PREVIOUS {dest_glb.name} is UNTOUCHED "
                     f"({dest_glb.stat().st_size:,} bytes) - nothing was destroyed.")
            else:
                fail(f"  No previous {dest_glb.name} existed; nothing was written.")
            if quarantined:
                fail(f"  The rejected GLB is kept for inspection: {quarantined}")
            fail(f"  Re-roll with a different seed: "
                 f"uv run trellis_generate.py {asset} --seed <new>")
            # NOT retried here on purpose: the same seed reproduces the same
            # mesh, so an automatic retry would spend GPU quota for a
            # guaranteed repeat (concept_generate.py's pinned-seed ruling).
            return EXIT_DEGENERATE

        # PHASE 3: the ONLY write to trellis_raw.glb in this script, reached
        # only because PHASE 2 raised nothing.
        try:
            commit_staged(staged_glb, dest_glb)
        except OSError as exc:
            # A locked destination (an open viewer on Windows) must not escape
            # as a traceback and must not leave the temp file behind.
            _discard_staged(staged_glb)
            state["status"] = "failed"
            state["finished_utc"] = _utc_now()
            state["error"] = redact(f"commit failed: {type(exc).__name__}: {exc}")
            write_state(asset_dir, state, filename="state_failed.json")
            fail(f"Validated GLB could not be swapped into place: "
                 f"{type(exc).__name__}: {exc}. The previous {dest_glb.name} is "
                 "untouched; close anything holding the file and re-run.")
            return 1

        state["status"] = "success"
        state["finished_utc"] = _utc_now()
        state["output_glb"] = str(dest_glb)
        state["output_glb_bytes"] = dest_glb.stat().st_size
        state["artefact_stats"] = glb_stats
        state["artefact_checks_passed"] = list(ALL_GLB_CHECKS)
        write_state(asset_dir, state)
        # The measurement beside the word SUCCESS is now the measurement that
        # was CHECKED, not decoration (SC-39.1's durable sentence).
        say(f"SUCCESS: {dest_glb} - validated: {describe_glb(glb_stats)}. "
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
            "5 concept image missing/unreadable, 6 DEGENERATE ARTEFACT (the "
            "returned GLB is unreadable/truncated/mesh-less/geometry-less/flat - "
            "the previous trellis_raw.glb is UNTOUCHED and the rejected GLB is "
            "kept in Cache/<AssetName>/_rejected/; re-roll with a new --seed), "
            "64 CLI usage error. "
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
        help="TOKENLESS smoke test: FIRST run the degeneracy guard's positive "
             "control (offline; synthetic GLB fixtures giving every check a case "
             "where it fires ALONE, plus must-ACCEPT cases so a reject-everything "
             "guard fails too - the live fixture count is printed), then connect "
             "to the Space and assert the three endpoints exist (writes an "
             "api_schema.json snapshot). A broken guard exits 1 and BLOCKS "
             "generation. No GPU call, no quota spend, no HF_TOKEN needed.",
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
