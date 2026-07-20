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

Exit codes (board-spec contract, mirrors trellis_generate.py):
    0  success (or --check passed)
    1  generic failure (network/task/download errors after retries)
    2  API key not set (MESHY_TOKEN / MESHY_API_KEY alias, env or HKCU)
    3  credits/quota exhausted (HTTP 402 or a credit-worded task error) -
       an EXPECTED PAUSE, surfaced verbatim, never retried, never faked
    4  API drift: endpoint/response no longer matches the documented schema
    5  input missing (donor GLB and/or style/concept PNG) or unreadable
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
import sys
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


def download_file(url: str, dest: Path, attempts: int) -> int:
    """Stream a (signed) result URL to dest; returns byte size."""
    import urllib.request

    last_error: Exception | None = None
    for attempt in range(1, attempts + 1):
        try:
            say(f"Downloading {_strip_query(url)} -> {dest.name} "
                f"(attempt {attempt}/{attempts})")
            request = urllib.request.Request(url)
            tmp = dest.with_suffix(dest.suffix + ".part")
            with urllib.request.urlopen(
                request, timeout=DOWNLOAD_HTTP_TIMEOUT, context=_ssl_context()
            ) as response, open(tmp, "wb") as sink:
                shutil.copyfileobj(response, sink)
            tmp.replace(dest)
            return dest.stat().st_size
        except Exception as exc:  # noqa: BLE001
            last_error = exc
            if attempt < attempts:
                delay = BACKOFF_BASE_SECONDS * (3 ** (attempt - 1))
                warn(f"Download failed "
                     f"({redact(f'{type(exc).__name__}: {exc}')}); "
                     f"retrying in {delay}s")
                time.sleep(delay)
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


def run_check() -> int:
    """--check: key + reachability + schema smoke test. Spends NO credits.

    Deviation from trellis_generate.py's tokenless --check (documented): the
    Meshy API has no unauthenticated surface, and the board acceptance pins
    '--check passes with key present / exits 2 without' - so --check REQUIRES
    the key. It calls only free read endpoints (balance + task lists).
    """
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
    size = download_file(glb_url, dest, args.attempts)

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
    say(f"SUCCESS: {dest} ({size:,} bytes).{credits_note} "
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
        help="Smoke test: resolve the key (exit 2 if unset), then hit free "
             "read endpoints (balance + task lists) to verify auth + schema. "
             "No credit spend.",
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
