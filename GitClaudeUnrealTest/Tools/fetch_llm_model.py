#!/usr/bin/env python3
# fetch_llm_model.py
# ---------------------------------------------------------------------------
# Downloads a GGUF model from Hugging Face into <ProjectRoot>/Models/ for the
# in-match LLM command assistant (LLM-ASSISTANT batch, TASK-409).
#
# WHY THIS EXISTS:
#   The weights are ~2.5 GB and NEVER enter git (CONVENTIONS section 7 -- both
#   `*.gguf` and `/Models/` are gitignored), and they never go in Content/
#   either, because a non-.uasset blob there would be cook-ignored. So every
#   machine has to fetch them, and "download it by hand from a browser" is not a
#   reproducible pipeline step. This is that step.
#
# DESIGN NOTES:
#   - STDLIB ONLY. No `huggingface_hub`, no `requests`, no pip install. The
#     ArtPipeline .venv is disposable and this must run bare from any shell.
#   - RUN BARE. Norton's TLS interception is already excluded for huggingface.co
#     and proven; do NOT reintroduce an SSL_CERT_FILE hack.
#   - RESUMABLE. A 2.5 GB download that dies at 90% must not start over: the
#     partial lands in <file>.part and is continued with an HTTP Range request.
#   - CHECKSUM-VERIFIED. HF stores the sha256 as the LFS oid; we fetch it from
#     the API and verify after download. A corrupt/truncated GGUF otherwise
#     fails much later, deep inside llama.cpp, with a far worse error.
#   - EXIT-CODE LAW. Non-zero on any failure, AND an explicit verdict line, so
#     a caller can judge on log text rather than trusting the exit code alone.
#
# SECURITY -- HF_TOKEN IS ENV-ONLY AND MUST NEVER BE PRINTED:
#   The token is read from the environment (it lives in HKCU on this machine and
#   a Windows rollback wipes it). It is never echoed, never written to a log,
#   and never included in a report. It is also STRIPPED ON CROSS-HOST REDIRECT
#   so it cannot leak to a non-Hugging-Face CDN.
#
# USAGE:
#   python Tools/fetch_llm_model.py --check
#   python Tools/fetch_llm_model.py --repo <org/repo> --file <name.gguf>
#   python Tools/fetch_llm_model.py --repo <org/repo> --file <name.gguf> --dest D:/Models
#
# NOTE ON MODEL CHOICE (CONVENTIONS section 7): the spike default is
#   Qwen3.5-4B-Instruct Q4_K_M (Apache 2.0). Every purpose-built small
#   function-calling model -- xLAM-2, Hammer 2.1, Arch-Function -- is
#   non-commercial and BANNED. Verify the EXACT quant repo's model card, not the
#   family's, and record the licence line verbatim.
# ---------------------------------------------------------------------------

from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path
from typing import Optional

HF_HOST = "huggingface.co"
# Hosts allowed to receive the Authorization header. Matched exact-or-subdomain
# by _may_send_auth() -- NEVER as a bare suffix, because a bare
# host.endswith("huggingface.co") also accepts "evilhuggingface.co".
ALLOWED_AUTH_HOSTS = ("huggingface.co", "hf.co")
CHUNK = 1024 * 1024
USER_AGENT = "siegebound-fetch-llm-model/1.0 (stdlib urllib)"

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_DEST = PROJECT_ROOT / "Models"


# ---------------------------------------------------------------------------
# HTTP plumbing
# ---------------------------------------------------------------------------

def _may_send_auth(host: Optional[str]) -> bool:
    """True iff `host` is an allowed host itself, or a subdomain of one.

    Subdomains MUST still match: HF redirects large files to its own CDN
    (cdn-lfs*.huggingface.co), and those requests have to keep the bearer token
    or the download 401s. Look-alike registrations that merely END WITH an
    allowed name -- evilhuggingface.co, myhf.co -- must not.
    """
    host = (host or "").lower()
    return any(host == allowed or host.endswith("." + allowed)
               for allowed in ALLOWED_AUTH_HOSTS)


class _TokenStrippingRedirectHandler(urllib.request.HTTPRedirectHandler):
    """Drops the Authorization header when a redirect leaves Hugging Face.

    urllib replays request headers across redirects. HF redirects large files to
    a CDN, so without this the bearer token would be handed to whatever host the
    redirect names. The token is a secret; it goes to HF or nowhere.
    """

    def redirect_request(self, req, fp, code, msg, headers, newurl):
        new_req = super().redirect_request(req, fp, code, msg, headers, newurl)
        if new_req is None:
            return None
        host = (urllib.parse.urlparse(newurl).hostname or "").lower()
        if not _may_send_auth(host):
            # `Authorization` may be stored under either casing depending on how
            # it was added; clear both views.
            new_req.headers.pop("Authorization", None)
            new_req.unredirected_hdrs.pop("Authorization", None)
        return new_req


_OPENER = urllib.request.build_opener(_TokenStrippingRedirectHandler)


def _token() -> Optional[str]:
    for name in ("HF_TOKEN", "HUGGING_FACE_HUB_TOKEN", "HUGGINGFACEHUB_API_TOKEN"):
        value = os.environ.get(name)
        if value and value.strip():
            return value.strip()
    return None


def _request(url: str, *, method: str = "GET", data: Optional[bytes] = None,
             extra_headers: Optional[dict] = None) -> urllib.request.Request:
    headers = {"User-Agent": USER_AGENT}
    if data is not None:
        headers["Content-Type"] = "application/json"
    tok = _token()
    if tok:
        headers["Authorization"] = f"Bearer {tok}"
    if extra_headers:
        headers.update(extra_headers)
    return urllib.request.Request(url, data=data, headers=headers, method=method)


def _fail(message: str) -> None:
    print(f"ERROR: {message}", file=sys.stderr)


# ---------------------------------------------------------------------------
# Metadata
# ---------------------------------------------------------------------------

def fetch_file_metadata(repo: str, filename: str, revision: str) -> dict:
    """Returns {'size': int, 'sha256': str|None} for one file in an HF repo."""
    url = f"https://{HF_HOST}/api/models/{repo}/paths-info/{urllib.parse.quote(revision)}"
    payload = json.dumps({"paths": [filename]}).encode("utf-8")
    try:
        with _OPENER.open(_request(url, method="POST", data=payload), timeout=60) as resp:
            entries = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        if exc.code in (401, 403):
            raise RuntimeError(
                f"access denied to '{repo}' (HTTP {exc.code}). If the repo is gated, "
                f"accept its licence on Hugging Face and set HF_TOKEN. "
                f"HF_TOKEN is currently {'SET' if _token() else 'NOT SET'}."
            ) from exc
        if exc.code == 404:
            raise RuntimeError(f"repo or revision not found: '{repo}' @ '{revision}'") from exc
        raise RuntimeError(f"metadata request failed: HTTP {exc.code} {exc.reason}") from exc
    except urllib.error.URLError as exc:
        raise RuntimeError(f"network error reaching {HF_HOST}: {exc.reason}") from exc

    for entry in entries:
        if entry.get("path") == filename:
            lfs = entry.get("lfs") or {}
            return {
                "size": int(lfs.get("size") or entry.get("size") or 0),
                "sha256": lfs.get("oid"),
            }
    raise RuntimeError(f"'{filename}' is not in '{repo}' @ '{revision}'")


# ---------------------------------------------------------------------------
# Download
# ---------------------------------------------------------------------------

def _sha256(path: Path, *, label: str) -> str:
    digest = hashlib.sha256()
    total = path.stat().st_size
    done = 0
    with path.open("rb") as handle:
        while True:
            block = handle.read(CHUNK)
            if not block:
                break
            digest.update(block)
            done += len(block)
            if total:
                print(f"\r  {label}: {100.0 * done / total:5.1f}%", end="", flush=True)
    print()
    return digest.hexdigest()


def download(repo: str, filename: str, revision: str, dest_dir: Path,
             *, force: bool, verify: bool) -> Path:
    dest_dir.mkdir(parents=True, exist_ok=True)
    final_path = dest_dir / Path(filename).name
    part_path = final_path.with_suffix(final_path.suffix + ".part")

    meta = fetch_file_metadata(repo, filename, revision)
    expected_size = meta["size"]
    expected_sha = meta["sha256"]
    print(f"  remote size   : {expected_size / 1e9:.2f} GB ({expected_size} bytes)")
    print(f"  remote sha256 : {expected_sha or '(unavailable)'}")

    if final_path.exists() and not force:
        actual = final_path.stat().st_size
        if expected_size and actual != expected_size:
            print(f"  existing file is {actual} bytes, expected {expected_size} -- refetching")
            final_path.unlink()
        elif verify and expected_sha:
            print("  verifying existing file...")
            if _sha256(final_path, label="verify") == expected_sha:
                print(f"  already present and verified: {final_path}")
                return final_path
            print("  checksum mismatch -- refetching")
            final_path.unlink()
        else:
            print(f"  already present: {final_path}")
            return final_path

    url = (f"https://{HF_HOST}/{repo}/resolve/{urllib.parse.quote(revision)}/"
           f"{urllib.parse.quote(filename)}")

    resume_from = part_path.stat().st_size if part_path.exists() else 0
    if resume_from and expected_size and resume_from >= expected_size:
        resume_from = 0
        part_path.unlink()

    headers = {"Range": f"bytes={resume_from}-"} if resume_from else None
    if resume_from:
        print(f"  resuming from {resume_from / 1e9:.2f} GB")

    try:
        with _OPENER.open(_request(url, extra_headers=headers), timeout=120) as resp:
            # A 200 to a Range request means the server ignored it: restart.
            if resume_from and resp.status == 200:
                print("  server ignored Range -- restarting from 0")
                resume_from = 0
            mode = "ab" if resume_from else "wb"
            written = resume_from
            with part_path.open(mode) as out:
                while True:
                    block = resp.read(CHUNK)
                    if not block:
                        break
                    out.write(block)
                    written += len(block)
                    if expected_size:
                        print(f"\r  downloading: {100.0 * written / expected_size:5.1f}% "
                              f"({written / 1e9:.2f}/{expected_size / 1e9:.2f} GB)",
                              end="", flush=True)
            print()
    except urllib.error.HTTPError as exc:
        if exc.code == 416 and part_path.exists():
            part_path.unlink()
            raise RuntimeError("stale partial download discarded -- rerun to restart") from exc
        raise RuntimeError(f"download failed: HTTP {exc.code} {exc.reason}") from exc
    except urllib.error.URLError as exc:
        raise RuntimeError(f"download failed: {exc.reason} (partial kept for resume)") from exc

    actual_size = part_path.stat().st_size
    if expected_size and actual_size != expected_size:
        raise RuntimeError(
            f"size mismatch: got {actual_size}, expected {expected_size} "
            f"(partial kept at {part_path} -- rerun to resume)")

    if verify and expected_sha:
        print("  verifying checksum...")
        actual_sha = _sha256(part_path, label="verify")
        if actual_sha != expected_sha:
            raise RuntimeError(f"CHECKSUM MISMATCH: got {actual_sha}, expected {expected_sha}")
        print("  checksum OK")
    elif verify:
        print("  WARNING: no remote sha256 available -- integrity NOT verified")

    part_path.replace(final_path)
    return final_path


# ---------------------------------------------------------------------------
# --check
# ---------------------------------------------------------------------------

def health_check(dest_dir: Path) -> bool:
    ok = True
    print("=== fetch_llm_model.py --check ===")
    print(f"python        : {sys.version.split()[0]}")
    print(f"project root  : {PROJECT_ROOT}")
    print(f"models dir    : {dest_dir}")

    # Presence only. NEVER print the token itself.
    print(f"HF_TOKEN      : {'SET' if _token() else 'NOT SET (fine unless a repo is gated)'}")

    try:
        dest_dir.mkdir(parents=True, exist_ok=True)
        probe = dest_dir / ".write-probe"
        probe.write_text("ok", encoding="ascii")
        probe.unlink()
        print("writable      : YES")
    except OSError as exc:
        print(f"writable      : NO ({exc})")
        ok = False

    try:
        with _OPENER.open(_request(f"https://{HF_HOST}/api/models/gpt2"), timeout=30) as resp:
            print(f"huggingface.co: reachable (HTTP {resp.status})")
    except Exception as exc:  # noqa: BLE001 - a health probe reports every failure kind
        print(f"huggingface.co: UNREACHABLE ({exc})")
        ok = False

    if dest_dir.is_dir():
        models = sorted(dest_dir.glob("*.gguf"))
        print(f"local models  : {len(models)}")
        for model in models:
            print(f"  - {model.name}  {model.stat().st_size / 1e9:.2f} GB")
        for partial in sorted(dest_dir.glob("*.part")):
            print(f"  ! partial: {partial.name}  {partial.stat().st_size / 1e9:.2f} GB")

    print(f"CHECK_VERDICT: {'PASS' if ok else 'FAIL'}")
    return ok


# ---------------------------------------------------------------------------

def main(argv: Optional[list] = None) -> int:
    parser = argparse.ArgumentParser(
        description="Fetch a GGUF model into <ProjectRoot>/Models/ (gitignored).")
    parser.add_argument("--check", action="store_true",
                        help="health probe: env, network, writability, local models")
    parser.add_argument("--repo", help="Hugging Face repo id, e.g. Qwen/Qwen3.5-4B-Instruct-GGUF")
    parser.add_argument("--file", dest="filename", help="file within the repo, e.g. model-Q4_K_M.gguf")
    parser.add_argument("--revision", default="main", help="branch, tag or commit (default: main)")
    parser.add_argument("--dest", type=Path, default=DEFAULT_DEST,
                        help=f"destination directory (default: {DEFAULT_DEST})")
    parser.add_argument("--force", action="store_true", help="refetch even if already present")
    parser.add_argument("--no-verify", action="store_true", help="skip sha256 verification")
    args = parser.parse_args(argv)

    dest_dir = args.dest.resolve()

    if args.check:
        return 0 if health_check(dest_dir) else 1

    if not args.repo or not args.filename:
        parser.print_usage(sys.stderr)
        _fail("--repo and --file are required (or use --check)")
        print("FETCH_VERDICT: FAIL")
        return 2

    print(f"=== fetching {args.repo} :: {args.filename} @ {args.revision} ===")
    try:
        path = download(args.repo, args.filename, args.revision, dest_dir,
                        force=args.force, verify=not args.no_verify)
    except RuntimeError as exc:
        _fail(str(exc))
        print("FETCH_VERDICT: FAIL")
        return 1
    except KeyboardInterrupt:
        _fail("interrupted (partial kept for resume)")
        print("FETCH_VERDICT: FAIL")
        return 130

    size_gb = path.stat().st_size / 1e9
    print(f"  -> {path} ({size_gb:.2f} GB)")
    print("  reminder: this file is gitignored and must never be committed.")
    print("FETCH_VERDICT: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
