#!/usr/bin/env python3
"""Stage 1.5 / Stage-1-alternative of the Siegebound art pipeline: Meshy client (TASK-198).

Second engine per CONVENTIONS.md "Meshy second engine (M7.5)". Two modes:

  --mode retexture <CardID>
      Upload the dense donor Cache/<CardID>/trellis_raw.glb (or meshy_raw.glb)
      plus the style reference Inbox/<CardID>.png to the Meshy Retexture API
      -> download Cache/<CardID>/meshy_retex.glb.

  --mode image3d <CardID>
      Upload the concept Inbox/<CardID>.png to the Meshy Image-to-3D API
      -> download Cache/<CardID>/meshy_raw.glb.

Both modes MERGE engine provenance into Cache/<CardID>/state.json (engine,
Meshy task id, input shas, consumed credits) without clobbering the existing
TRELLIS success record. Meshy output NEVER lands in Content/ - everything
re-enters through Stage 2 (refine_trellis_glb.py); the Stage-2/3 laws are
untouched regardless of engine (the INVARIANT).

Security law (CONVENTIONS.md "Meshy second engine (M7.5)" secret law):
    The API key is ENV-ONLY. Canonical variable: MESHY_TOKEN  (RECORDED
    DEVIATION from the board-specced MESHY_API_KEY - Jonathan set the real
    HKCU user var as MESHY_TOKEN, 2026-07-18; actual name is law. The
    specced MESHY_API_KEY is still accepted as a documented fallback alias.)
    Resolution order:
      1. process env MESHY_TOKEN
      2. process env MESHY_API_KEY (alias)
      3. Windows-only: HKCU\\Environment user values MESHY_TOKEN then
         MESHY_API_KEY via winreg - covers shells spawned before the user
         var was set (the HF_TOKEN/HKCU rollback precedent). The value read
         this way is registered with the redactor exactly like an env read.
    The key is never accepted on argv, never written to any file (state.json
    carries no credential material), and never echoed or logged - every line
    this script prints passes through a redactor that scrubs the live key
    value AND anything matching the generic msy_ key shape.

TLS law: verification is NEVER disabled. ssl.create_default_context() honors
SSL_CERT_FILE, so the proven combined-CA-bundle fallback (TASK-185/195
Norton precedent) works unchanged if Norton MITMs *.meshy.ai; surface
TLS/cert failures verbatim and escalate for a Norton exclusion instead.

Artefact law (CONVENTIONS.md SC-39.1, TASK-876):
    A tool that produces an artefact VALIDATES that artefact before reporting
    success. This script ALREADY staged its download (.part -> replace), so a
    FAILED download could never destroy the prior artefact - but nothing
    inspected the file between the completed download and the swap, so a
    SUCCESSFUL download of a degenerate payload destroyed it just as
    thoroughly. TASK-876 MEASURED exactly that, by driving this function with a
    scripted CDN socket and zero credits: a 132-byte 0-mesh GLB (a WELL-FORMED
    container with an empty `meshes` array) replaced a real 23,434,424-byte
    Knight mesh, and the tool printed `SUCCESS: ... (132 bytes)` and returned 0.

    ⇒ Staging protects against a FAILED transfer. Only validation protects
    against a SUCCESSFUL transfer of a degenerate payload. The staging was
    correct and is UNCHANGED. What was added is PHASE 2:

        PHASE 1  download   -> <name>.glb.part   (pre-existing; dest untouched)
        PHASE 2  validate_glb_file(.part)         (NEW; dest still the old file)
        PHASE 3  commit_staged()                  os.replace, only if PHASE 2 passed

    A rejected GLB is QUARANTINED to Cache/<CardID>/_rejected/, never deleted.
    Cache/ is gitignored, so quarantined artefacts never enter git.

Exit codes (board-spec contract, mirrors trellis_generate.py):
    0  success (or --check passed)
    1  generic failure (network/task/download errors after retries)
    2  API key not set (MESHY_TOKEN / MESHY_API_KEY alias, env or HKCU)
    3  credits/quota exhausted (HTTP 402 or a credit-worded task error) -
       an EXPECTED PAUSE, surfaced verbatim, never retried, never faked
    4  API drift: endpoint/response no longer matches the documented schema
    5  input missing (donor GLB and/or style/concept PNG) or unreadable
    6  DEGENERATE ARTEFACT: the task SUCCEEDED and the download completed, but
       the GLB is unreadable, a truncated container, mesh-less, geometry-less
       or collapsed to a plane/point. The prior artefact is UNTOUCHED and the
       rejected GLB is in Cache/<CardID>/_rejected/. Credits were already spent
       on the task, so this is reported, never silently retried.
       (Same value and meaning as trellis_generate.py and concept_generate.py.)
    64 CLI usage error (argparse default of 2 is remapped so that exit code
       2 uniquely means "API key unset")
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
import re
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

API_BASE = "https://api.meshy.ai"
EP_BALANCE = "/openapi/v1/balance"
EP_RETEXTURE = "/openapi/v1/retexture"
EP_IMAGE3D = "/openapi/v1/image-to-3d"

ENV_CANONICAL = "MESHY_TOKEN"       # recorded deviation: Jonathan's real HKCU var
ENV_ALIAS = "MESHY_API_KEY"         # board-specced name, kept as documented alias

SCRIPT_DIR = Path(__file__).resolve().parent
INBOX_DIR = SCRIPT_DIR / "Inbox"
CACHE_DIR = SCRIPT_DIR / "Cache"

DEFAULT_AI_MODEL = "latest"
DEFAULT_POLYCOUNT = 300_000         # image3d dense-donor target (API max, standard)
DEFAULT_TOPOLOGY = "triangle"
DEFAULT_TIMEOUT_MINUTES = 30.0
MIN_TIMEOUT_MINUTES = 5.0
DEFAULT_POLL_SECONDS = 10.0
DEFAULT_ATTEMPTS = 3
BACKOFF_BASE_SECONDS = 20           # 20s, 60s, 180s ...

CREATE_HTTP_TIMEOUT = 600.0         # data-URI upload of a ~30 MB donor
POLL_HTTP_TIMEOUT = 60.0
DOWNLOAD_HTTP_TIMEOUT = 600.0

TERMINAL_STATUSES = ("SUCCEEDED", "FAILED", "CANCELED")

# --- Artefact guard (SC-39.1, TASK-876) ------------------------------------
# Kept byte-identical in meaning to trellis_generate.py's guard so the two
# engines cannot drift apart on what "a usable mesh" means. See the FINDINGS
# note in handoffs/TASK-876-programmer.md: the right long-term home is a shared
# Tools/ArtPipeline/artefact_guard.py, which is outside this task's fence.
EXIT_DEGENERATE = 6                  # measured absent from BOTH siblings
REJECTED_DIR_NAME = "_rejected"

GLB_MAGIC = 0x46546C67               # 'glTF' little-endian
GLB_CHUNK_JSON = 0x4E4F534A          # 'JSON'
GLB_CHUNK_BIN = 0x004E4942           # 'BIN\0'
GLB_HEADER_BYTES = 12

# THRESHOLDS ARE DERIVED, NOT PICKED (SC-40 cl. 10):
#   MIN_MESH_VERTICES / MIN_MESH_TRIANGLES = the tetrahedron, the smallest
#   closed solid in 3-space. A floor of "is it a solid at all", not a quality
#   bar - quality is Stage 2's tri_budget.
#   FLAT_ASPECT_FLOOR = a relative epsilon; a collapsed mesh has aspect EXACTLY
#   0.0, and 1e-4 is ~3 orders of magnitude above float32 noise at unit scale.
# MEASURED HEADROOM over the 126 real GLBs under Cache/ (TASK-876 sweep, of
# which 33 are this script's own meshy_raw.glb / meshy_retex.glb output):
#   vertices  11,820 (Sapper_gameready_nobomb) -> 2,955x above the floor
#   triangles  9,461 (Sapper_Attack)           -> 2,365x
#   aspect    0.2652 (Footman_gameready)       -> 2,652x
#   126/126 accepted by this guard. The floors are ~3 orders of magnitude below
#   the worst REAL asset, so they gate "is this a solid at all", never quality.
MIN_MESH_VERTICES = 4
MIN_MESH_TRIANGLES = 4
FLAT_ASPECT_FLOOR = 1e-4

# BYTE SIZE IS DELIBERATELY *NOT* A GATE. `SUCCESS: {dest} ({size:,} bytes)` is
# precisely the line that reported a destroyed 20 MB mesh as a success in the
# TASK-876 repro. Size stays a printed diagnostic and decides nothing.

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

# Only these two are worth a re-request: they describe the TRANSPORT, and the
# CDN can serve the same signed URL correctly on a second attempt. A
# content-shaped rejection (no-mesh / no-geometry / degenerate-bounds) is a
# fact about the model Meshy generated - the same URL returns the same bytes,
# so retrying spends time for a guaranteed repeat.
RETRYABLE_GLB_CHECKS = frozenset({CHECK_UNREADABLE, CHECK_TRUNCATED})

# ---------------------------------------------------------------------------
# Secret-safe output. EVERY print in this script goes through redact().
# ---------------------------------------------------------------------------

_REDACT_VALUES: list[str] = []
# Generic Meshy key shape (observed prefix msy_, TASK-198; mirrors the
# guard-secrets hook pattern).
_MESHY_KEY_RE = re.compile(r"msy_[A-Za-z0-9]{15,}")
_REDACTED = "[meshy-key-redacted]"


def _register_secret(value: str) -> None:
    """Remember a secret so redact() can scrub it from ALL output paths."""
    if value and value not in _REDACT_VALUES:
        _REDACT_VALUES.append(value)


def redact(text: object) -> str:
    out = str(text)
    for value in _REDACT_VALUES:
        out = out.replace(value, _REDACTED)
    return _MESHY_KEY_RE.sub(_REDACTED, out)


def say(msg: str) -> None:
    print(f"[meshy] {redact(msg)}", flush=True)


def warn(msg: str) -> None:
    print(f"[meshy][WARN] {redact(msg)}", file=sys.stderr, flush=True)


def fail(msg: str) -> None:
    print(f"[meshy][ERROR] {redact(msg)}", file=sys.stderr, flush=True)


# ---------------------------------------------------------------------------
# Error classes
# ---------------------------------------------------------------------------


class ApiDriftError(RuntimeError):
    """The API no longer matches the documented endpoint/response schema."""


class InputError(RuntimeError):
    """An input file is unreadable/corrupt (exit 5, like a missing input)."""


class InsufficientCreditsError(RuntimeError):
    """Credits/quota exhausted (HTTP 402) - an expected pause, not a failure."""


class MeshyHttpError(RuntimeError):
    """Non-2xx HTTP response; carries status + verbatim body for surfacing."""

    def __init__(self, status: int, body: str, context: str):
        self.status = status
        self.body = body
        self.context = context
        super().__init__(f"{context}: HTTP {status}: {body}")


class DegenerateArtefactError(RuntimeError):
    """The download completed, and what arrived is not a usable mesh (exit 6)."""

    def __init__(self, checks: list[str], stats: dict, path: Path | None = None):
        self.checks = list(checks)
        self.stats = stats
        self.path = path
        super().__init__(", ".join(self.checks) or "degenerate")


def _looks_like_credit_error(text: str) -> bool:
    lowered = text.lower()
    return "insufficient credit" in lowered or (
        "credit" in lowered and ("exhaust" in lowered or "enough" in lowered)
    )


def _print_credit_guidance(verbatim: str) -> None:
    # Spec: surface credit exhaustion VERBATIM - an expected pause, never faked.
    fail("Meshy credits exhausted. API message, verbatim:")
    fail(f"    {verbatim}")
    fail("Guidance:")
    fail("  - This is an EXPECTED PAUSE (exit 3), not a code failure - record it in")
    fail("    the task handoff and resume when the subscription window resets or")
    fail("    Jonathan tops up credits at meshy.ai.")
    fail("  - Check remaining credits any time with: uv run meshy_generate.py --check")


# ---------------------------------------------------------------------------
# API key resolution (ENV-ONLY, with the documented Windows HKCU fallback)
# ---------------------------------------------------------------------------


def resolve_api_key() -> tuple[str | None, str]:
    """Return (key, source-description). The key value is NEVER printed.

    Order: process env MESHY_TOKEN -> process env MESHY_API_KEY (alias) ->
    HKCU\\Environment MESHY_TOKEN / MESHY_API_KEY (Windows only; covers shells
    spawned before the user var existed - the HF_TOKEN/HKCU precedent).
    """
    for name in (ENV_CANONICAL, ENV_ALIAS):
        value = os.environ.get(name)
        if value:
            return value, f"process env {name}"
    if sys.platform == "win32":
        try:
            import winreg

            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment") as env_key:
                for name in (ENV_CANONICAL, ENV_ALIAS):
                    try:
                        value, _type = winreg.QueryValueEx(env_key, name)
                    except FileNotFoundError:
                        continue
                    if value:
                        return str(value), f"HKCU user environment {name}"
        except OSError as exc:
            warn(f"HKCU environment read failed ({type(exc).__name__}); "
                 "falling through to 'key unset'.")
    return None, "unset"


def require_api_key() -> str | None:
    """Resolve the key or print exit-2 setup guidance. Returns None if unset."""
    key, source = resolve_api_key()
    if not key:
        fail(f"{ENV_CANONICAL} is not set (checked process env {ENV_CANONICAL} and")
        fail(f"{ENV_ALIAS}, then the HKCU user environment on Windows).")
        fail("Setup (one-time): Windows Settings > System > About > Advanced system")
        fail(f"settings > Environment Variables > add user variable {ENV_CANONICAL}")
        fail("with your Meshy API key (meshy.ai > API settings), then open a NEW")
        fail("terminal. The key lives ONLY in the environment: never put it in a")
        fail("file, on a command line, or in chat.")
        return None
    _register_secret(key)
    say(f"API key resolved from {source} (value redacted everywhere).")
    return key


# ---------------------------------------------------------------------------
# HTTP layer (stdlib urllib; TLS verification always ON; SSL_CERT_FILE honored)
# ---------------------------------------------------------------------------


def _ssl_context():
    import ssl

    # create_default_context() honors SSL_CERT_FILE / SSL_CERT_DIR, so the
    # proven Norton combined-CA-bundle fallback (TASK-185/195) works unchanged.
    # Verification is NEVER disabled (secret law / TLS law).
    return ssl.create_default_context()


def api_request(
    method: str,
    path: str,
    api_key: str,
    payload: dict | None = None,
    timeout: float = POLL_HTTP_TIMEOUT,
) -> dict | list:
    """One authenticated JSON request. Raises typed errors; bodies verbatim.

    Returns the parsed JSON (object OR array - the task-list endpoints return
    a bare array); callers assert the shape they need.
    """
    import urllib.error
    import urllib.request

    url = API_BASE + path
    data = None
    headers = {"Authorization": f"Bearer {api_key}"}
    if payload is not None:
        data = json.dumps(payload).encode("utf-8")
        headers["Content-Type"] = "application/json"
    request = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(
            request, timeout=timeout, context=_ssl_context()
        ) as response:
            body = response.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as exc:
        error_body = exc.read().decode("utf-8", errors="replace")
        if exc.code == 402 or _looks_like_credit_error(error_body):
            raise InsufficientCreditsError(
                f"HTTP {exc.code}: {error_body}"
            ) from None
        raise MeshyHttpError(exc.code, error_body, f"{method} {path}") from None
    try:
        parsed = json.loads(body)
    except json.JSONDecodeError:
        raise ApiDriftError(
            f"{method} {path} returned non-JSON body (first 300 chars): {body[:300]}"
        ) from None
    if not isinstance(parsed, (dict, list)):
        raise ApiDriftError(
            f"{method} {path} returned JSON {type(parsed).__name__}, "
            "expected object or array."
        )
    return parsed


def _as_object(result: dict | list, context: str) -> dict:
    if not isinstance(result, dict):
        raise ApiDriftError(
            f"{context} returned JSON {type(result).__name__}, expected object."
        )
    return result


def call_api(
    method: str,
    path: str,
    api_key: str,
    payload: dict | None = None,
    timeout: float = POLL_HTTP_TIMEOUT,
    attempts: int = DEFAULT_ATTEMPTS,
    context: str = "",
) -> dict:
    """api_request with exponential backoff.

    - Credit exhaustion (402) is NOT retried - retrying spends nothing but
      time; it propagates for the exit-3 expected-pause path.
    - 400/401/404 are NOT retried (request/auth/id problems do not heal);
      they propagate with the verbatim body.
    - Everything else (transport errors, 429 rate limit, 5xx) retries up to
      `attempts` with backoff 20s -> 60s -> 180s.
    """
    label = context or f"{method} {path}"
    last_error: Exception | None = None
    for attempt in range(1, attempts + 1):
        try:
            return api_request(method, path, api_key, payload, timeout)
        except InsufficientCreditsError:
            raise
        except MeshyHttpError as exc:
            if exc.status in (400, 401, 404):
                raise
            last_error = exc
        except ApiDriftError:
            raise
        except Exception as exc:  # noqa: BLE001 - urllib raises many types
            last_error = exc
        if attempt < attempts:
            delay = BACKOFF_BASE_SECONDS * (3 ** (attempt - 1))
            warn(f"{label} failed "
                 f"({redact(f'{type(last_error).__name__}: {last_error}')}); "
                 f"retrying in {delay}s (attempt {attempt}/{attempts})")
            time.sleep(delay)
    raise RuntimeError(
        f"{label} failed after {attempts} attempts: "
        f"{redact(f'{type(last_error).__name__}: {last_error}')}"
    )


def _strip_query(url: str) -> str:
    """Signed download URLs carry auth-ish query params - log host+path only."""
    return url.split("?", 1)[0]


# ---------------------------------------------------------------------------
# THE ARTEFACT GUARD (SC-39.1 cl. 1 + cl. 3, TASK-876)
#
# Deliberately kept the SAME SHAPE as trellis_generate.py's guard so the two
# mesh engines cannot drift apart on what "a usable mesh" means. See the
# FINDINGS section of handoffs/TASK-876-programmer.md: the right long-term home
# is one shared Tools/ArtPipeline/artefact_guard.py, and creating a new module
# is outside this task's fence, so it is REPORTED rather than done.
# ---------------------------------------------------------------------------


def measure_glb(data: bytes) -> dict:
    """Parse a GLB byte string into the statistics the checks are computed on.

    PURE: no I/O, and no exceptions for degenerate input - a malformed container
    is RECORDED in the stats (`unreadable_reason` / `truncated_reason`) so that
    assess_degeneracy() stays a total function of the stats. That split is what
    lets the positive control drive every check deterministically.
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
        # The transfer completed but the payload is not the whole file. THIS is
        # the truncation `.part` staging cannot see: the stream simply ended
        # early, `copyfileobj` returned normally and nothing raised.
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
            except Exception as exc:  # noqa: BLE001 - any decode failure is one verdict
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

    Reading from disk (rather than trusting the bytes we just streamed) is what
    catches the `save_asset` shape named beside this defect in SC-39.1: a write
    that "succeeded" while producing a zero-byte or unreadable file.

    `quiet` suppresses the skip-rule warning only; it never changes a verdict.
    The positive control sets it, because several fixtures are SUPPOSED to be
    unparseable and a warning per fixture would bury the real output.

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
#
# The STAGING half was ALREADY CORRECT here and is UNCHANGED (board TASK-876
# item 2 - do not re-plumb a path that already exists). What was missing is
# PHASE 2. Measured, not assumed: the TASK-876 repro drove the PRISTINE tool
# with a scripted CDN and the `.part` staging did NOT save the prior artefact,
# because the download SUCCEEDED - a 132-byte 0-mesh GLB replaced a real
# 23,434,424-byte mesh and the tool printed SUCCESS and returned 0. Staging
# protects against a FAILED transfer; only validation protects against a
# SUCCESSFUL transfer of a degenerate payload.
# ---------------------------------------------------------------------------


def _require_inside_cache(path: Path) -> Path:
    """Write confinement: this script only ever writes under Cache/."""
    resolved = Path(path).resolve()
    root = Path(CACHE_DIR).resolve()
    if not (resolved == root or root in resolved.parents):
        raise RuntimeError(f"Refusing to write outside Cache/: {resolved}")
    return resolved


def commit_staged(staged: Path, dest: Path) -> None:
    """PHASE 3 - atomically swap a VALIDATED staged file into place.

    THE ONLY place in this script that writes the output GLB. Call it ONLY
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

    A rejected mesh has ALREADY CONSUMED Meshy credits; deleting it deletes the
    evidence of what those credits bought. Best-effort by contract: a quarantine
    failure must never turn a clean rejection into a crash, and must never leave
    the temp file behind.
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


def download_file(url: str, dest: Path, attempts: int) -> tuple[Path, dict]:
    """PHASE 1+2 - stream a (signed) result URL to `<dest>.part` and VALIDATE it.

    Returns (STAGED path, the validated stats). It deliberately does NOT return
    a byte size and deliberately does NOT touch `dest`: reporting a size was the
    entire evidence base of the defect this function used to carry, and the swap
    now belongs to the caller so it can exist at exactly one guarded call site.

    A TRANSPORT-shaped rejection (unreadable / truncated) is retried, because a
    CDN can serve the same signed URL correctly on a second attempt. A
    CONTENT-shaped rejection (no-mesh / no-geometry / degenerate-bounds) is NOT:
    the same URL returns the same bytes, so a retry spends time for a guaranteed
    repeat. Raises DegenerateArtefactError carrying the staged path.
    """
    import urllib.request

    staged = dest.with_suffix(dest.suffix + ".part")
    last_error: Exception | None = None
    last_degenerate: DegenerateArtefactError | None = None
    for attempt in range(1, attempts + 1):
        try:
            say(f"Downloading {_strip_query(url)} -> {staged.name} "
                f"(attempt {attempt}/{attempts})")
            request = urllib.request.Request(url)
            with urllib.request.urlopen(
                request, timeout=DOWNLOAD_HTTP_TIMEOUT, context=_ssl_context()
            ) as response, open(staged, "wb") as sink:
                shutil.copyfileobj(response, sink)
            # PHASE 2: dest is STILL the previous good artefact at this point.
            stats = validate_glb_file(staged)
            say(f"Downloaded and validated: {describe_glb(stats)}")
            return staged, stats
        except DegenerateArtefactError as exc:
            last_degenerate = exc
            if set(exc.checks) & RETRYABLE_GLB_CHECKS and attempt < attempts:
                delay = BACKOFF_BASE_SECONDS * (3 ** (attempt - 1))
                warn(f"Downloaded GLB failed {', '.join(exc.checks)} "
                     f"(transport-shaped); re-requesting in {delay}s")
                time.sleep(delay)
                continue
            raise
        except Exception as exc:  # noqa: BLE001
            last_error = exc
            if attempt < attempts:
                delay = BACKOFF_BASE_SECONDS * (3 ** (attempt - 1))
                warn(f"Download failed "
                     f"({redact(f'{type(exc).__name__}: {exc}')}); "
                     f"retrying in {delay}s")
                time.sleep(delay)
    if last_degenerate is not None:
        raise last_degenerate
    _discard_staged(staged)
    raise RuntimeError(
        f"Download failed after {attempts} attempts: "
        f"{redact(f'{type(last_error).__name__}: {last_error}')}"
    )


# ---------------------------------------------------------------------------
# Input validation + data URIs
# ---------------------------------------------------------------------------


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as source:
        for chunk in iter(lambda: source.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_image(path: Path, role: str) -> dict:
    """Verify a PNG/JPG is readable; return metadata incl. sha256."""
    if not path.is_file():
        raise FileNotFoundError(
            f"{role} image not found: {path}\n"
            f"Drop the PNG at Inbox/{path.name} (Inbox law, CONVENTIONS.md)."
        )
    from PIL import Image  # local import: keeps --help dependency-free

    try:
        with Image.open(path) as img:
            img.verify()  # detects truncated/corrupt files
        with Image.open(path) as img:
            width, height = img.size
    except Exception as exc:  # noqa: BLE001 - Pillow raises various decode errors
        raise InputError(
            f"{role} image unreadable: {path} ({type(exc).__name__}: {exc})"
        ) from None
    return {
        "file": path.name,
        "width": width,
        "height": height,
        "sha256": _sha256(path),
    }


def image_data_uri(path: Path) -> str:
    suffix = path.suffix.lower()
    mime = "image/jpeg" if suffix in (".jpg", ".jpeg") else "image/png"
    encoded = base64.b64encode(path.read_bytes()).decode("ascii")
    return f"data:{mime};base64,{encoded}"


def model_data_uri(path: Path) -> str:
    # Documented Meshy shape for model uploads (docs.meshy.ai, retexture).
    encoded = base64.b64encode(path.read_bytes()).decode("ascii")
    return f"data:application/octet-stream;base64,{encoded}"


def resolve_donor_glb(asset_dir: Path, preference: str) -> Path:
    """Pick the dense donor for retexture: trellis_raw.glb (default) or
    meshy_raw.glb, per CONVENTIONS Stage 1.5."""
    trellis = asset_dir / "trellis_raw.glb"
    meshy = asset_dir / "meshy_raw.glb"
    ordered = {
        "auto": (trellis, meshy),
        "trellis": (trellis,),
        "meshy": (meshy,),
    }[preference]
    for candidate in ordered:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(
        f"No donor GLB for retexture under {asset_dir} "
        f"(looked for: {', '.join(p.name for p in ordered)}). "
        "Run trellis_generate.py (or --mode image3d) first."
    )


# ---------------------------------------------------------------------------
# Task lifecycle
# ---------------------------------------------------------------------------


def create_task(endpoint: str, payload: dict, api_key: str, attempts: int) -> str:
    result = _as_object(
        call_api(
            "POST", endpoint, api_key, payload,
            timeout=CREATE_HTTP_TIMEOUT, attempts=attempts,
            context=f"POST {endpoint} (create task)",
        ),
        f"POST {endpoint}",
    )
    task_id = result.get("result")
    if not isinstance(task_id, str) or not task_id:
        raise ApiDriftError(
            f"POST {endpoint} returned no 'result' task id "
            f"(body keys: {sorted(result.keys())})."
        )
    return task_id


def poll_task(
    endpoint: str,
    task_id: str,
    api_key: str,
    timeout_minutes: float,
    poll_seconds: float,
    attempts: int,
) -> dict:
    """Poll GET {endpoint}/{id} until a terminal status or wall-clock timeout."""
    deadline = time.monotonic() + timeout_minutes * 60.0
    last_progress: object = None
    while True:
        task = _as_object(
            call_api(
                "GET", f"{endpoint}/{task_id}", api_key,
                attempts=attempts, context=f"GET {endpoint}/{task_id} (poll)",
            ),
            f"GET {endpoint}/{task_id}",
        )
        status = task.get("status")
        if status not in ("PENDING", "IN_PROGRESS") + TERMINAL_STATUSES:
            raise ApiDriftError(
                f"Task {task_id}: unknown status {status!r} "
                f"(task keys: {sorted(task.keys())})."
            )
        progress = task.get("progress")
        if status == "PENDING":
            queued = task.get("preceding_tasks")
            say(f"Task {task_id}: PENDING"
                + (f" ({queued} task(s) ahead in queue)" if queued else ""))
        elif progress != last_progress or status in TERMINAL_STATUSES:
            say(f"Task {task_id}: {status} (progress {progress})")
        last_progress = progress
        if status in TERMINAL_STATUSES:
            return task
        if time.monotonic() >= deadline:
            raise RuntimeError(
                f"Task {task_id} still {status} after {timeout_minutes:.0f} min "
                f"(progress {progress}). Re-run later; the task may still finish "
                "server-side - its id is recorded in state_failed.json."
            )
        time.sleep(poll_seconds)


def assert_succeeded(task: dict) -> None:
    """Raise the right typed error for FAILED/CANCELED tasks, verbatim."""
    status = task.get("status")
    if status == "SUCCEEDED":
        return
    error = task.get("task_error") or {}
    message = error.get("message") if isinstance(error, dict) else str(error)
    verbatim = message or f"(no task_error message; status {status})"
    if _looks_like_credit_error(str(verbatim)):
        raise InsufficientCreditsError(str(verbatim))
    raise RuntimeError(
        f"Meshy task {task.get('id')} ended {status}. task_error, verbatim: "
        f"{verbatim}"
    )


def fetch_balance(api_key: str) -> float | None:
    """GET /openapi/v1/balance; non-fatal (returns None on any failure)."""
    try:
        result = _as_object(api_request("GET", EP_BALANCE, api_key), EP_BALANCE)
    except Exception as exc:  # noqa: BLE001 - balance is informational only
        warn(f"Balance check failed (non-fatal): "
             f"{redact(f'{type(exc).__name__}: {exc}')}")
        return None
    balance = result.get("balance")
    if isinstance(balance, (int, float)):
        return float(balance)
    warn(f"Balance response had no numeric 'balance' "
         f"(keys: {sorted(result.keys())}).")
    return None


# ---------------------------------------------------------------------------
# State file (merge law: never clobber the TRELLIS success record)
# ---------------------------------------------------------------------------


def _utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


def _load_state(asset_dir: Path) -> dict:
    state_path = asset_dir / "state.json"
    if state_path.is_file():
        try:
            existing = json.loads(state_path.read_text(encoding="utf-8"))
            if isinstance(existing, dict):
                return existing
            warn("Existing state.json is not a JSON object; preserving it as "
                 "'pre_meshy_state'.")
            return {"pre_meshy_state": existing}
        except json.JSONDecodeError:
            warn("Existing state.json is unparsable; it will be preserved as "
                 "state_pre_meshy.json and a fresh state written.")
            shutil.copyfile(state_path, asset_dir / "state_pre_meshy.json")
    return {}


def write_success_state(asset_dir: Path, engine: str, meshy_block: dict) -> None:
    """MERGE Meshy provenance into state.json (success only).

    The trellis_generate.py record (concept, params, output_glb...) is kept
    verbatim; this adds/replaces top-level 'engine' plus a 'meshy' block, and
    shelves any previous meshy block into 'meshy_history'. Failure paths
    write state_failed.json instead (trellis QA WARN-2 scheme) so a failed
    run can never clobber the success record of an artifact that survived.
    """
    state = _load_state(asset_dir)
    previous = state.get("meshy")
    if isinstance(previous, dict):
        state.setdefault("meshy_history", []).append(previous)
    state["engine"] = engine
    state["meshy"] = meshy_block
    asset_dir.mkdir(parents=True, exist_ok=True)
    dest = asset_dir / "state.json"
    dest.write_text(json.dumps(state, indent=2, default=str), encoding="utf-8")
    say(f"State merged: {dest} (engine={engine})")


def write_failed_state(asset_dir: Path, record: dict) -> None:
    asset_dir.mkdir(parents=True, exist_ok=True)
    dest = asset_dir / "state_failed.json"
    dest.write_text(json.dumps(record, indent=2, default=str), encoding="utf-8")
    say(f"Failure record written: {dest} (state.json untouched)")


# ---------------------------------------------------------------------------
# Modes
# ---------------------------------------------------------------------------


# ---------------------------------------------------------------------------
# The guard's POSITIVE CONTROL (SC-39 / SC-39.1 cl. 4 + cl. 6 / SHIP-9)
#
# It lives HERE and runs inside --check, not in a separate file somebody has to
# remember to run. Every fixture is a REAL FILE ON DISK driven through
# validate_glb_file(), so the on-disk checks are controlled exactly like the
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

    SYNTHESISED, never a real artefact and never a re-request: proving the guard
    goes RED must cost zero Meshy credits (SC-39.1 cl. 4).
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

    OFFLINE and KEYLESS by construction, so it runs before the API-key check
    and cannot be skipped by an unset MESHY_API_KEY - which is the state this
    machine is actually in (TASK-876). A control that only runs when a
    credential is present is a control with a known failure mode.
    """
    with tempfile.TemporaryDirectory(prefix="meshy-guard-") as scratch:
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


def run_check() -> int:
    """--check: key + reachability + schema smoke test. Spends NO credits.

    Deviation from trellis_generate.py's tokenless --check (documented): the
    Meshy API has no unauthenticated surface, and the board acceptance pins
    '--check passes with key present / exits 2 without' - so --check REQUIRES
    the key. It calls only free read endpoints (balance + task lists).
    """
    # STEP 1, and deliberately FIRST - AHEAD of the key check, not merely ahead
    # of the network calls. MESHY_API_KEY is unset on this machine, so a control
    # placed after require_api_key() would exit 2 and NEVER RUN (SC-39.1 cl. 6:
    # the control belongs in the ALWAYS-RUN tier).
    if run_guard_self_test() != 0:
        return 1

    api_key = require_api_key()
    if not api_key:
        return 2

    say(f"--check: probing {API_BASE} (free read endpoints; no credit spend)")
    try:
        result = _as_object(api_request("GET", EP_BALANCE, api_key), EP_BALANCE)
        balance = result.get("balance")
        if not isinstance(balance, (int, float)):
            raise ApiDriftError(
                f"GET {EP_BALANCE} returned no numeric 'balance' "
                f"(keys: {sorted(result.keys())})."
            )
        say(f"  {EP_BALANCE} - OK (credits remaining: {balance})")
        for endpoint in (EP_RETEXTURE, EP_IMAGE3D):
            probe = api_request("GET", f"{endpoint}?page_size=1", api_key)
            # Live API returns a bare JSON array for task lists (observed
            # 2026-07-18); tolerate an object wrapper too.
            if isinstance(probe, list):
                count = len(probe)
            else:
                count = len(probe.get("result", probe.get("data", [])) or [])
            say(f"  {endpoint} - OK (list endpoint reachable, "
                f"{count} task(s) in first page)")
    except InsufficientCreditsError as exc:
        # Free reads should never 402; if one does, surface it verbatim.
        _print_credit_guidance(str(exc))
        return 3
    except ApiDriftError as exc:
        fail(str(exc))
        return 4
    except MeshyHttpError as exc:
        if exc.status == 404:
            fail(f"API drift: {exc.context} returned 404 - documented endpoint "
                 f"missing. Body, verbatim: {exc.body}")
            return 4
        fail(f"{exc.context} failed. HTTP {exc.status}, body verbatim: {exc.body}")
        if exc.status == 401:
            fail("The key resolved but Meshy rejected it (401) - regenerate the "
                 "key at meshy.ai and update the MESHY_TOKEN user variable.")
        return 1
    except Exception as exc:  # noqa: BLE001
        fail(f"API unreachable: {type(exc).__name__}: {exc}")
        fail("If this is an SSL/certificate error: Norton TLS interception on "
             "*.meshy.ai - point SSL_CERT_FILE at the combined CA bundle "
             "(TASK-185/195 playbook) or add the Norton exclusion. Never "
             "disable verification.")
        return 1
    say("--check PASSED: key valid, API reachable, documented endpoints present.")
    return 0


def _finish_task_common(
    args: argparse.Namespace,
    endpoint: str,
    payload: dict,
    api_key: str,
    output_name: str,
    engine: str,
    inputs_block: dict,
    asset_dir: Path,
    state_record: dict,
) -> int:
    """Shared create -> poll -> download -> provenance tail for both modes."""
    balance_before = fetch_balance(api_key)
    if balance_before is not None:
        say(f"Credits before run: {balance_before}")

    task_id = create_task(endpoint, payload, api_key, args.attempts)
    say(f"Task created: {task_id}")
    state_record["task_id"] = task_id

    task = poll_task(
        endpoint, task_id, api_key,
        timeout_minutes=args.timeout_minutes,
        poll_seconds=args.poll_seconds,
        attempts=args.attempts,
    )
    assert_succeeded(task)

    model_urls = task.get("model_urls") or {}
    glb_url = model_urls.get("glb") if isinstance(model_urls, dict) else None
    if not glb_url:
        raise ApiDriftError(
            f"Task {task_id} SUCCEEDED but has no model_urls.glb "
            f"(model_urls keys: "
            f"{sorted(model_urls.keys()) if isinstance(model_urls, dict) else model_urls})."
        )
    dest = asset_dir / output_name

    # ---- stage -> validate -> swap (SC-39.1 cl. 3) ------------------------
    # PHASE 1+2 happen inside download_file(): the payload lands in
    # <dest>.part and is validated FROM DISK. Whatever happens in there, the
    # previous artefact at `dest` is still untouched.
    try:
        staged, glb_stats = download_file(glb_url, dest, args.attempts)
    except DegenerateArtefactError as exc:
        quarantined = quarantine_staged(exc.path or dest, asset_dir.name, dest)
        state_record["status"] = "degenerate-artefact"
        state_record["finished_utc"] = _utc_now()
        state_record["task_id"] = task_id
        state_record["failed_checks"] = exc.checks
        state_record["artefact_stats"] = exc.stats
        state_record["quarantined_to"] = str(quarantined) if quarantined else None
        state_record["prior_artefact_preserved"] = dest.is_file()
        state_record["consumed_credits"] = task.get("consumed_credits")
        write_failed_state(asset_dir, state_record)
        fail(f"DEGENERATE ARTEFACT: task {task_id} SUCCEEDED and the download "
             f"completed, but the GLB is not a usable mesh. Failed checks: "
             f"{', '.join(exc.checks)}.")
        fail(f"  measured: {describe_glb(exc.stats)}")
        if exc.stats.get("unreadable_reason"):
            fail(f"  reason: {exc.stats['unreadable_reason']}")
        if exc.stats.get("truncated_reason"):
            fail(f"  reason: {exc.stats['truncated_reason']}")
        if dest.is_file():
            fail(f"  The PREVIOUS {dest.name} is UNTOUCHED "
                 f"({dest.stat().st_size:,} bytes) - nothing was destroyed.")
        else:
            fail(f"  No previous {dest.name} existed; nothing was written.")
        if quarantined:
            fail(f"  The rejected GLB is kept for inspection: {quarantined}")
        fail(f"  Credits for task {task_id} were ALREADY SPENT "
             f"({task.get('consumed_credits')}); this is reported, never "
             "silently re-requested.")
        return EXIT_DEGENERATE

    # PHASE 3: the ONLY write to the output GLB in this script, reached only
    # because PHASE 2 raised nothing.
    try:
        commit_staged(staged, dest)
    except OSError as exc:
        # A locked destination (an open viewer on Windows) must not escape as a
        # traceback and must not leave the staged file behind.
        _discard_staged(staged)
        state_record["status"] = "failed"
        state_record["finished_utc"] = _utc_now()
        state_record["error"] = redact(f"commit failed: {type(exc).__name__}: {exc}")
        write_failed_state(asset_dir, state_record)
        fail(f"Validated GLB could not be swapped into place: "
             f"{type(exc).__name__}: {exc}. The previous {dest.name} is "
             "untouched; close anything holding the file and re-run.")
        return 1
    size = dest.stat().st_size

    consumed = task.get("consumed_credits")
    balance_after = fetch_balance(api_key)
    meshy_block = {
        "mode": state_record["mode"],
        "task_id": task_id,
        "endpoint": endpoint,
        "api_base": API_BASE,
        "ai_model": args.ai_model,
        "params": state_record["params"],
        "inputs": inputs_block,
        "output_glb": str(dest),
        "output_glb_bytes": size,
        "output_glb_sha256": _sha256(dest),
        "artefact_stats": glb_stats,
        "artefact_checks_passed": list(ALL_GLB_CHECKS),
        "consumed_credits": consumed,
        "credits_before": balance_before,
        "credits_after": balance_after,
        "started_utc": state_record["started_utc"],
        "finished_utc": _utc_now(),
        "status": "success",
    }
    write_success_state(asset_dir, engine, meshy_block)
    credits_note = (
        f" Credits consumed: {consumed}"
        + (f" (balance {balance_before} -> {balance_after})"
           if balance_before is not None and balance_after is not None else "")
        + "." if consumed is not None else ""
    )
    # The measurement beside the word SUCCESS is now the measurement that was
    # CHECKED, not decoration (SC-39.1's durable sentence). `{size:,} bytes`
    # alone is exactly what reported a destroyed 23 MB mesh as a success.
    say(f"SUCCESS: {dest} - validated: {describe_glb(glb_stats)}.{credits_note} "
        "Next: Stage 2 headless refine consumes this donor (INVARIANT: "
        "Meshy output never lands in Content/ directly).")
    return 0


def _run_mode(args: argparse.Namespace) -> int:
    """Dispatch retexture/image3d with the shared failure->exit-code tail."""
    api_key = require_api_key()
    if not api_key:
        return 2

    asset = args.asset
    asset_dir = CACHE_DIR / asset
    asset_dir.mkdir(parents=True, exist_ok=True)

    state_record: dict = {
        "asset": asset,
        "mode": args.mode,
        "engine": "meshy-retex" if args.mode == "retexture" else "meshy-i23d",
        "api_base": API_BASE,
        "started_utc": _utc_now(),
        "params": {},
        "status": "in-progress",
    }

    try:
        if args.mode == "retexture":
            donor = resolve_donor_glb(asset_dir, args.donor)
            style_path = INBOX_DIR / f"{asset}.png"
            style_meta = validate_image(style_path, "Style reference")
            donor_sha = _sha256(donor)
            say(f"Donor: {donor.name} ({donor.stat().st_size:,} bytes, "
                f"sha256 {donor_sha[:12]}...)")
            say(f"Style ref: {style_path.name} "
                f"({style_meta['width']}x{style_meta['height']}, "
                f"sha256 {style_meta['sha256'][:12]}...)")
            params = {
                "ai_model": args.ai_model,
                "enable_original_uv": not args.no_preserve_uv,
                "enable_pbr": not args.no_pbr,
                "target_formats": ["glb"],
            }
            state_record["params"] = params
            inputs_block = {
                "donor_glb": donor.name,
                "donor_glb_sha256": donor_sha,
                "style_image": style_meta["file"],
                "style_image_sha256": style_meta["sha256"],
            }
            say("Encoding donor + style ref as data URIs and creating the "
                "retexture task (upload may take a while)...")
            payload = dict(params)
            payload["model_url"] = model_data_uri(donor)
            payload["image_style_url"] = image_data_uri(style_path)
            return _finish_task_common(
                args, EP_RETEXTURE, payload, api_key,
                output_name="meshy_retex.glb", engine="meshy-retex",
                inputs_block=inputs_block, asset_dir=asset_dir,
                state_record=state_record,
            )

        # ---- image3d ------------------------------------------------------
        concept_path = INBOX_DIR / f"{asset}.png"
        concept_meta = validate_image(concept_path, "Concept")
        say(f"Concept: {concept_path.name} "
            f"({concept_meta['width']}x{concept_meta['height']}, "
            f"sha256 {concept_meta['sha256'][:12]}...)")
        params = {
            "ai_model": args.ai_model,
            "topology": args.topology,
            "target_polycount": args.target_polycount,
            "should_remesh": True,
            "should_texture": True,
            "enable_pbr": not args.no_pbr,
        }
        state_record["params"] = params
        inputs_block = {
            "concept_image": concept_meta["file"],
            "concept_image_sha256": concept_meta["sha256"],
        }
        payload = dict(params)
        payload["image_url"] = image_data_uri(concept_path)
        return _finish_task_common(
            args, EP_IMAGE3D, payload, api_key,
            output_name="meshy_raw.glb", engine="meshy-i23d",
            inputs_block=inputs_block, asset_dir=asset_dir,
            state_record=state_record,
        )

    # Failure paths write state_failed.json, NEVER state.json (trellis QA
    # WARN-2 scheme): a surviving artifact keeps its success provenance.
    except (FileNotFoundError, InputError) as exc:
        fail(str(exc))
        return 5
    except InsufficientCreditsError as exc:
        state_record["status"] = "credits-blocked"
        state_record["finished_utc"] = _utc_now()
        state_record["error"] = redact(str(exc))
        write_failed_state(asset_dir, state_record)
        _print_credit_guidance(str(exc))
        return 3
    except ApiDriftError as exc:
        state_record["status"] = "api-drift"
        state_record["finished_utc"] = _utc_now()
        state_record["error"] = redact(str(exc))
        write_failed_state(asset_dir, state_record)
        fail(str(exc))
        return 4
    except MeshyHttpError as exc:
        state_record["status"] = "failed"
        state_record["finished_utc"] = _utc_now()
        state_record["error"] = redact(str(exc))
        write_failed_state(asset_dir, state_record)
        fail(f"{exc.context} failed. HTTP {exc.status}, body verbatim: {exc.body}")
        return 1
    except Exception as exc:  # noqa: BLE001
        state_record["status"] = "failed"
        state_record["finished_utc"] = _utc_now()
        state_record["error"] = redact(f"{type(exc).__name__}: {exc}")
        write_failed_state(asset_dir, state_record)
        fail(f"Run failed: {type(exc).__name__}: {exc}")
        if "SSL" in str(exc) or "certificate" in str(exc).lower():
            fail("SSL/certificate failure: likely Norton TLS interception on "
                 "*.meshy.ai - point SSL_CERT_FILE at the combined CA bundle "
                 "(TASK-185/195 playbook) or add the Norton exclusion. Never "
                 "disable verification.")
        return 1


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


class _Parser(argparse.ArgumentParser):
    """argparse exits 2 on usage errors by default; remap to 64 (EX_USAGE) so
    exit code 2 UNIQUELY means 'API key unset' (board-spec contract)."""

    def error(self, message: str):  # noqa: D102 - argparse override
        self.print_usage(sys.stderr)
        # Usage messages mirror raw argv back; redact() so a key accidentally
        # pasted on the command line is never echoed - the generic msy_
        # pattern works even before any secret is registered.
        self.exit(64, f"{self.prog}: error: {redact(message)}\n")


def build_parser() -> argparse.ArgumentParser:
    parser = _Parser(
        prog="meshy_generate.py",
        description=(
            "Meshy second engine for the Siegebound art pipeline (Stage 1.5 "
            "retexture / Stage-1-alternative image-to-3D). Outputs "
            "Cache/<CardID>/meshy_retex.glb or meshy_raw.glb + state.json "
            "provenance; Stage 2 consumes the result unchanged."
        ),
        epilog=(
            f"The API key is read from the environment ONLY - canonical "
            f"{ENV_CANONICAL} (alias {ENV_ALIAS}; HKCU user-env fallback on "
            "Windows) - never accepted as an argument, never written or "
            "logged. Exit codes: 0 success, 1 failure, 2 key unset, "
            "3 credits exhausted (expected pause), 4 API drift, 5 input "
            "missing, 64 CLI usage error. Examples: "
            "`uv run meshy_generate.py --mode retexture Ogre`, "
            "`uv run meshy_generate.py --check`."
        ),
    )
    parser.add_argument(
        "asset",
        nargs="?",
        metavar="CardID",
        help="Asset/CardID (PascalCase, Inbox law). retexture reads "
             "Cache/<CardID>/trellis_raw.glb + Inbox/<CardID>.png; image3d "
             "reads Inbox/<CardID>.png. Required unless --check.",
    )
    parser.add_argument(
        "--mode", choices=("retexture", "image3d"),
        help="retexture: Stage 1.5 - repaint the dense donor GLB using the "
             "concept as style ref -> meshy_retex.glb. image3d: Stage-1 "
             "alternative - concept PNG -> meshy_raw.glb. Required unless "
             "--check.",
    )
    parser.add_argument(
        "--ai-model", default=DEFAULT_AI_MODEL,
        help=f"Meshy model id: meshy-5, meshy-6, or latest (default "
             f"\"{DEFAULT_AI_MODEL}\"). Recorded in state.json.",
    )
    parser.add_argument(
        "--donor", choices=("auto", "trellis", "meshy"), default="auto",
        help="retexture only - which dense donor to upload: auto (default; "
             "trellis_raw.glb, else meshy_raw.glb), trellis, or meshy.",
    )
    parser.add_argument(
        "--no-preserve-uv", action="store_true",
        help="retexture only - let Meshy re-UV the donor instead of painting "
             "into its existing UV layout (enable_original_uv=false).",
    )
    parser.add_argument(
        "--no-pbr", action="store_true",
        help="Skip PBR map generation (metallic/roughness/normal). Default is "
             "PBR ON - Stage 2 bakes ORM from the donor.",
    )
    parser.add_argument(
        "--topology", choices=("triangle", "quad"), default=DEFAULT_TOPOLOGY,
        help=f"image3d only - output topology (default {DEFAULT_TOPOLOGY}).",
    )
    parser.add_argument(
        "--target-polycount", type=int, default=DEFAULT_POLYCOUNT,
        help=f"image3d only - dense-donor polycount target (default "
             f"{DEFAULT_POLYCOUNT}; Stage 2 does the real budget cut).",
    )
    parser.add_argument(
        "--timeout-minutes", type=float, default=DEFAULT_TIMEOUT_MINUTES,
        help=f"Wall-clock cap on task polling (default {DEFAULT_TIMEOUT_MINUTES:g}; "
             f"minimum {MIN_TIMEOUT_MINUTES:g}).",
    )
    parser.add_argument(
        "--poll-seconds", type=float, default=DEFAULT_POLL_SECONDS,
        help=f"Seconds between task polls (default {DEFAULT_POLL_SECONDS:g}).",
    )
    parser.add_argument(
        "--attempts", type=int, default=DEFAULT_ATTEMPTS,
        help=f"Retry attempts per HTTP call with exponential backoff (default "
             f"{DEFAULT_ATTEMPTS}). Credit exhaustion (402) is never retried.",
    )
    parser.add_argument(
        "--check", action="store_true",
        help="Smoke test: FIRST run the degeneracy guard's positive control "
             "(offline AND KEYLESS, so it runs even with no API key set; "
             "synthetic GLB fixtures giving every check a case where it fires "
             "ALONE, plus must-ACCEPT cases so a reject-everything guard fails "
             "too - the live fixture count is printed), then resolve the key "
             "(exit 2 if unset) and hit free read endpoints (balance + task "
             "lists) to verify auth + schema. A broken guard exits 1 and BLOCKS "
             "generation. No credit spend.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)

    if args.timeout_minutes < MIN_TIMEOUT_MINUTES:
        warn(f"--timeout-minutes {args.timeout_minutes} is below the "
             f"{MIN_TIMEOUT_MINUTES:g}-minute floor; clamping.")
        args.timeout_minutes = MIN_TIMEOUT_MINUTES
    if args.poll_seconds < 2.0:
        warn("--poll-seconds must be >= 2 (rate-limit courtesy); using 2.")
        args.poll_seconds = 2.0
    if args.attempts < 1:
        warn("--attempts must be >= 1; using 1.")
        args.attempts = 1

    if args.check:
        return run_check()
    if not args.mode:
        build_parser().error("--mode {retexture,image3d} is required unless --check.")
    if not args.asset:
        build_parser().error("CardID is required unless --check.")
    return _run_mode(args)


if __name__ == "__main__":
    sys.exit(main())
