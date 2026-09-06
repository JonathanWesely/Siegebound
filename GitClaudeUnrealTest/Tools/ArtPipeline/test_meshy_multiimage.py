#!/usr/bin/env python3
"""Gate for meshy_generate.py's --mode multiimage (TASK-1088, law CHAR-3).

WHAT THIS FILE IS FOR, stated before the tests so the reason survives:

    A new mode is the easiest place to quietly drop an inherited law. The mode
    added by TASK-1088 rides on the SAME create/poll/download/validate/commit
    tail as the shipped modes, so the risk is not that the guard is missing -
    it is that the new mode reaches that tail with the wrong payload, or does
    not reach it at all and degrades to something that still returns 0.

    So the assertions here are aimed at the failures that can ACTUALLY happen
    in this diff (SC-79), not at restating that the tool works:

      1. ORDER - the payload carries EVERY requested view, in the REQUESTED
         order. Asserted by decoding each data URI back to bytes and matching
         its sha256 against the file at the same index, so a payload that
         merely has the right COUNT fails.
      2. NO SILENT DOWNGRADE - a missing view is exit 5 and NOTHING is sent.
         The single most valuable assertion in the file: a run that dropped
         _Back.png and generated anyway would spend credits, print SUCCESS,
         return 0, and be wrong in a way nothing downstream could see.
      3. NO SILENT FALLBACK - a 403/404 from the multi-image endpoint is exit
         4 and says so; it never re-runs as --mode image3d. And the shipped
         modes' behaviour for those statuses is UNCHANGED (controlled here, so
         "scoped to multiimage" is a measurement rather than a claim).
      4. THE INHERITED LAWS still hold on the new path - the redactor scrubs,
         the degeneracy guard still rejects, writes stay confined to Cache/.

    Both directions are controlled where it matters: a mode that refused every
    run would pass (2) trivially, so (1) and (3) assert that a COMPLETE view
    set does reach the endpoint with the right payload.

No network, no MESHY_TOKEN, no credits, no engine, no Git. Nothing under
Tools/ArtPipeline/Inbox/ or Cache/ is touched: INBOX_DIR and CACHE_DIR are
redirected to a temp directory for every case that runs the real _run_mode.

Run:
    uv run test_meshy_multiimage.py     (or: .venv/Scripts/python.exe ...)

Exit codes:
    0  every assertion passed
    1  at least one assertion failed (the ledger names which)
"""

from __future__ import annotations

import argparse
import base64
import contextlib
import hashlib
import io
import json
import shutil
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

import meshy_generate as mg  # noqa: E402 - deliberate: needs the path above

_RESULTS: list[tuple[bool, str, str]] = []


def check(passed: bool, label: str, detail: str = "") -> bool:
    _RESULTS.append((bool(passed), label, detail))
    mark = "PASS" if passed else "FAIL"
    print(f"  [{mark}] {label}" + (f"  -- {detail}" if detail else ""), flush=True)
    return bool(passed)


def section(title: str) -> None:
    print(f"\n=== {title} ===", flush=True)


# ---------------------------------------------------------------------------
# Fixtures / sandbox
# ---------------------------------------------------------------------------

ASSET = "TestKnight"

# Built by concatenation ON PURPOSE: a token-shaped literal must not exist in a
# tracked file even when it is synthetic. This still matches the redactor's
# msy_[A-Za-z0-9]{15,} pattern at RUNTIME, which is the point of the fixture.
_PREFIX = "msy" + "_"
FAKE_KEY = _PREFIX + "TASK1088SyntheticDoubleNotACredential"
SHAPED_BUT_UNREGISTERED = _PREFIX + "neverRegisteredButShaped0000"


def make_args(**overrides) -> argparse.Namespace:
    """The parser's real defaults, so a test cannot pass against a shape the
    CLI never produces."""
    base = dict(
        asset=ASSET,
        mode="multiimage",
        views=mg.MULTIVIEW_DEFAULT_VIEWS,
        ai_model=mg.DEFAULT_AI_MODEL,
        topology=mg.DEFAULT_TOPOLOGY,
        target_polycount=mg.DEFAULT_POLYCOUNT,
        no_pbr=False,
        donor="auto",
        no_preserve_uv=False,
        timeout_minutes=mg.DEFAULT_TIMEOUT_MINUTES,
        poll_seconds=mg.DEFAULT_POLL_SECONDS,
        attempts=1,
        check=False,
    )
    base.update(overrides)
    return argparse.Namespace(**base)


def write_png(path: Path, colour: tuple[int, int, int], size: int = 64) -> Path:
    """A real, decodable PNG - validate_image() opens these with Pillow, so a
    stub byte string would fail for the wrong reason."""
    from PIL import Image

    path.parent.mkdir(parents=True, exist_ok=True)
    Image.new("RGB", (size, size), colour).save(path, format="PNG")
    return path


def sha256_of(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Sandbox:
    """Redirects INBOX_DIR and CACHE_DIR at the module so the real pipeline
    directories are never read or written by this gate."""

    def __init__(self, views: tuple[str, ...] = mg.MULTIVIEW_DEFAULT_VIEWS):
        self.views = views
        self.root: Path | None = None
        self._saved: tuple[Path, Path] | None = None

    def __enter__(self) -> "Sandbox":
        self.root = Path(tempfile.mkdtemp(prefix="task1088_"))
        self.inbox = self.root / "Inbox"
        self.cache = self.root / "Cache"
        self.inbox.mkdir(parents=True, exist_ok=True)
        self.cache.mkdir(parents=True, exist_ok=True)
        self._saved = (mg.INBOX_DIR, mg.CACHE_DIR)
        mg.INBOX_DIR = self.inbox
        mg.CACHE_DIR = self.cache
        # Each view gets a DISTINCT colour, so two views can never be confused
        # for one another by sha256.
        palette = [(200, 20, 20), (20, 200, 20), (20, 20, 200), (200, 200, 20)]
        self.paths: dict[str, Path] = {}
        for index, view in enumerate(self.views):
            self.paths[view] = write_png(
                self.inbox / f"{ASSET}_{view}.png", palette[index % len(palette)]
            )
        return self

    def __exit__(self, *exc) -> None:
        if self._saved:
            mg.INBOX_DIR, mg.CACHE_DIR = self._saved
        if self.root and self.root.exists():
            shutil.rmtree(self.root, ignore_errors=True)


class Tripwire:
    """Stands where the network is. Any call is a defect, and it says which."""

    def __init__(self) -> None:
        self.calls: list[tuple] = []

    def __call__(self, *args, **kwargs):
        self.calls.append((args, kwargs))
        raise AssertionError(
            "NETWORK REACHED: the run should have stopped before any request."
        )


class CapturedFinish:
    """Stands in for _finish_task_common: records exactly what the mode would
    have sent, and returns 0 without a task, a download or a credit."""

    def __init__(self, raise_error: Exception | None = None) -> None:
        self.raise_error = raise_error
        self.seen: dict | None = None
        self.calls = 0

    def __call__(self, args, endpoint, payload, api_key, output_name, engine,
                 inputs_block, asset_dir, state_record):
        self.calls += 1
        self.seen = dict(
            endpoint=endpoint, payload=payload, output_name=output_name,
            engine=engine, inputs_block=inputs_block, asset_dir=asset_dir,
            state_record=state_record, api_key=api_key,
        )
        if self.raise_error is not None:
            raise self.raise_error
        return 0


@contextlib.contextmanager
def patched(**attrs):
    """Temporarily replace module attributes; always restored."""
    saved = {name: getattr(mg, name) for name in attrs}
    for name, value in attrs.items():
        setattr(mg, name, value)
    try:
        yield
    finally:
        for name, value in saved.items():
            setattr(mg, name, value)


@contextlib.contextmanager
def captured_output():
    out, err = io.StringIO(), io.StringIO()
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        yield out, err


def decode_data_uri(uri: str) -> bytes:
    head, _, b64 = uri.partition(",")
    assert head.startswith("data:image/") and head.endswith(";base64"), head
    return base64.b64decode(b64)


# ---------------------------------------------------------------------------
# 1. ORDER - every view, in the order it was asked for
# ---------------------------------------------------------------------------


def test_payload_carries_every_view_in_declared_order() -> None:
    section("1. the payload carries every view, in the declared order")
    with Sandbox() as box:
        resolved = mg.resolve_multiview_images(ASSET, mg.MULTIVIEW_DEFAULT_VIEWS)
        check(
            [v for v, _p in resolved] == list(mg.MULTIVIEW_DEFAULT_VIEWS),
            "resolve_multiview_images returns the requested order",
            str([v for v, _p in resolved]),
        )
        payload = mg.build_multiimage_payload({"ai_model": "latest"}, resolved)
        urls = payload.get("image_urls")
        check(
            isinstance(urls, list) and len(urls) == 3,
            "payload carries image_urls with 3 entries",
            f"{type(urls).__name__} "
            f"len={len(urls) if isinstance(urls, list) else 'n/a'}",
        )
        check(
            "image_url" not in payload,
            "payload does NOT carry the single-image key image_url",
            "a payload with both would be ambiguous to the API",
        )
        # THE ORDER ASSERTION: index i must be the file for view i, byte for
        # byte. A payload with the right count but shuffled contents fails.
        mismatches = []
        for index, (view, path) in enumerate(resolved):
            sent = hashlib.sha256(decode_data_uri(urls[index])).hexdigest()
            if sent != sha256_of(path):
                mismatches.append(f"index {index} ({view})")
        check(
            not mismatches,
            "each image_urls[i] is byte-identical to the view at index i",
            ("mismatched: " + ", ".join(mismatches)) if mismatches else
            "sha256-matched Front->0, Side->1, Back->2",
        )
        # ...and the order follows the REQUEST, not the directory listing.
        reversed_views = tuple(reversed(mg.MULTIVIEW_DEFAULT_VIEWS))
        rev = mg.build_multiimage_payload(
            {}, mg.resolve_multiview_images(ASSET, reversed_views)
        )
        check(
            rev["image_urls"] == list(reversed(urls)),
            "reversing --views reverses image_urls",
            "order is driven by the request, not by the filesystem",
        )
        check(
            len(set(urls)) == 3,
            "the three encoded views are distinct payloads",
            "guards against every slot encoding the same file",
        )


# ---------------------------------------------------------------------------
# 2. NO SILENT DOWNGRADE - a missing view stops before the network
# ---------------------------------------------------------------------------


def test_missing_view_is_exit_5_and_sends_nothing() -> None:
    section("2. a missing view is exit 5 and NOTHING is sent")
    with Sandbox() as box:
        removed = box.paths["Back"]
        removed.unlink()
        wire = Tripwire()
        finish = CapturedFinish()
        with patched(api_request=wire, create_task=wire,
                     _finish_task_common=finish,
                     require_api_key=lambda: FAKE_KEY):
            with captured_output() as (out, err):
                rc = mg._run_mode(make_args())
        text = out.getvalue() + err.getvalue()
        check(rc == 5, "exit code is 5 (input missing)", f"got {rc}")
        check(
            not wire.calls,
            "the network was never reached",
            f"{len(wire.calls)} call(s) - must be 0",
        )
        check(
            finish.calls == 0,
            "no task was created (create/poll/download never entered)",
            f"{finish.calls} call(s) - must be 0",
        )
        check(removed.name in text, "the message names the missing file",
              removed.name)
        check(
            "MISSING" in text and "exit 5" in text,
            "the message says MISSING and names the exit code",
        )
        # Two views WERE present. A downgrade would have proceeded with them.
        check(
            f"{ASSET}_Front.png" in text and f"{ASSET}_Side.png" in text,
            "the message lists the views that WERE present",
            "so the operator can see exactly what a downgrade would have used",
        )
        asset_dir = box.cache / ASSET
        wrote = (sorted(p.name for p in asset_dir.glob("*"))
                 if asset_dir.exists() else [])
        check(
            not wrote,
            "no state.json / state_failed.json was written for a missing input",
            f"found {wrote}" if wrote else "asset dir empty or absent",
        )


# ---------------------------------------------------------------------------
# 3. THE COMPLETE SET DOES REACH THE MULTI-IMAGE ENDPOINT
#    (the other direction of the control: a mode that refused everything would
#     sail through test 2)
# ---------------------------------------------------------------------------


def test_complete_set_reaches_the_multi_image_endpoint() -> None:
    section("3. a complete view set reaches the multi-image endpoint")
    with Sandbox() as box:
        finish = CapturedFinish()
        wire = Tripwire()
        with patched(_finish_task_common=finish, api_request=wire,
                     require_api_key=lambda: FAKE_KEY):
            with captured_output():
                rc = mg._run_mode(make_args())
        check(rc == 0, "the run returns 0", f"got {rc}")
        seen = finish.seen or {}
        check(
            seen.get("endpoint") == mg.EP_MULTIIMAGE,
            "the endpoint is the multi-image one",
            str(seen.get("endpoint")),
        )
        check(
            seen.get("endpoint") != mg.EP_IMAGE3D,
            "the endpoint is NOT the single-image one",
            "no silent substitution of --mode image3d",
        )
        check(
            seen.get("output_name") == "meshy_raw.glb",
            "the output is Cache/<CardID>/meshy_raw.glb (Stage 2's donor)",
            str(seen.get("output_name")),
        )
        check(
            seen.get("engine") == "meshy-mi23d",
            "state.json is tagged with a DISTINCT engine for multi-view",
            f"{seen.get('engine')} (image3d is meshy-i23d)",
        )
        payload = seen.get("payload") or {}
        check(
            len(payload.get("image_urls", [])) == 3,
            "three images were in the payload handed to the shared tail",
        )
        inputs = seen.get("inputs_block") or {}
        check(
            inputs.get("view_order") == list(mg.MULTIVIEW_DEFAULT_VIEWS),
            "provenance records the view order",
            str(inputs.get("view_order")),
        )
        shas = [m.get("sha256") for m in inputs.get("view_images", [])]
        check(
            len(shas) == 3 and all(shas) and len(set(shas)) == 3,
            "provenance carries a sha256 for EVERY input image",
            f"{len(shas)} sha(s), {len(set(shas))} distinct",
        )
        expected = [sha256_of(box.paths[v]) for v in mg.MULTIVIEW_DEFAULT_VIEWS]
        check(
            shas == expected,
            "the recorded shas are the files that were actually sent",
        )
        check(
            str(seen.get("asset_dir", "")).startswith(str(box.cache)),
            "the run writes under Cache/, never Content/",
            str(seen.get("asset_dir")),
        )


# ---------------------------------------------------------------------------
# 4. NO SILENT FALLBACK on a refused endpoint - and the shipped modes are
#    UNCHANGED for the same statuses
# ---------------------------------------------------------------------------


def test_refused_endpoint_is_loud_exit_4_never_a_fallback() -> None:
    section("4. a refused multi-image endpoint is a loud exit 4")
    for status in (403, 404):
        with Sandbox():
            err_obj = mg.MeshyHttpError(
                status, '{"message":"not available"}',
                f"POST {mg.EP_MULTIIMAGE} (create task)",
            )
            finish = CapturedFinish(raise_error=err_obj)
            with patched(_finish_task_common=finish,
                         require_api_key=lambda: FAKE_KEY):
                with captured_output() as (out, err):
                    rc = mg._run_mode(make_args())
            text = out.getvalue() + err.getvalue()
            check(rc == 4, f"HTTP {status} -> exit 4 (API drift)", f"got {rc}")
            check(
                "image3d" in text and "fell back" in text,
                f"HTTP {status}: the operator is told NOTHING fell back to "
                "single-image",
            )
            check(
                '{"message":"not available"}' in text,
                f"HTTP {status}: the API body is surfaced verbatim",
            )
            check(
                finish.calls == 1,
                f"HTTP {status}: the mode was not retried under another mode",
                f"{finish.calls} call(s)",
            )

    # THE CONTROL: the shipped modes keep their shipped exit code for 403/404.
    # Without this, "scoped to multiimage" would be a claim, not a measurement.
    with Sandbox() as box:
        write_png(box.inbox / f"{ASSET}.png", (10, 10, 10))
        err_obj = mg.MeshyHttpError(
            404, '{"message":"nope"}', f"POST {mg.EP_IMAGE3D} (create task)"
        )
        finish = CapturedFinish(raise_error=err_obj)
        with patched(_finish_task_common=finish,
                     require_api_key=lambda: FAKE_KEY):
            with captured_output():
                rc = mg._run_mode(make_args(mode="image3d"))
        check(
            rc == 1,
            "CONTROL: --mode image3d still exits 1 on a 404 (shipped, unchanged)",
            f"got {rc}",
        )


# ---------------------------------------------------------------------------
# 5-7. THE INHERITED LAWS
# ---------------------------------------------------------------------------


def test_redactor_still_scrubs_on_the_new_path() -> None:
    section("5. the secret law holds on the new path")
    mg._register_secret(FAKE_KEY)
    check(
        FAKE_KEY not in mg.redact(f"Authorization: Bearer {FAKE_KEY}"),
        "a registered key is scrubbed by redact()",
    )
    check(
        SHAPED_BUT_UNREGISTERED not in
        mg.redact(f"key={SHAPED_BUT_UNREGISTERED}"),
        "the generic key shape is scrubbed even when never registered",
        "covers a key pasted on argv before any secret exists",
    )
    with captured_output() as (out, err):
        mg.say(f"leaking {FAKE_KEY}")
        mg.warn(f"leaking {FAKE_KEY}")
        mg.fail(f"leaking {FAKE_KEY}")
    printed = out.getvalue() + err.getvalue()
    check(
        FAKE_KEY not in printed and printed.count(mg._REDACTED) == 3,
        "say/warn/fail all route through the redactor",
        f"{printed.count(mg._REDACTED)} redactions in 3 lines",
    )
    # ...and the key never enters the payload or the provenance block.
    with Sandbox():
        finish = CapturedFinish()
        with patched(_finish_task_common=finish,
                     require_api_key=lambda: FAKE_KEY):
            with captured_output():
                mg._run_mode(make_args())
        seen = finish.seen or {}
        blob = json.dumps(
            {"payload": seen.get("payload"), "inputs": seen.get("inputs_block"),
             "state": seen.get("state_record")}, default=str
        )
        check(
            FAKE_KEY not in blob,
            "the key is absent from the payload, inputs and state record",
            "state.json carries no credential material",
        )
        check(
            seen.get("api_key") == FAKE_KEY,
            "CONTROL: the key IS handed to the request layer",
            "it must still reach the API - just never be recorded or printed",
        )


def test_degeneracy_guard_still_rejects() -> None:
    section("6. the SC-39.1 artefact guard still rejects a degenerate GLB")
    with captured_output():
        rc = mg.run_guard_self_test(verbose=False)
    check(rc == 0, "the shipped positive control passes", f"got {rc}")

    fixtures = list(mg.guard_fixtures())
    root = Path(tempfile.mkdtemp(prefix="task1088_glb_"))
    try:
        should_reject = [(n, p) for n, p, e in fixtures if e and p is not None]
        should_accept = [(n, p) for n, p, e in fixtures if not e and p is not None]
        rejected = 0
        for name, payload in should_reject:
            path = root / f"bad_{name}.glb"
            path.write_bytes(payload)
            try:
                with captured_output():
                    mg.validate_glb_file(path, quiet=True)
            except mg.DegenerateArtefactError:
                rejected += 1
        check(
            should_reject and rejected == len(should_reject),
            "every degenerate fixture is still REJECTED by validate_glb_file",
            f"{rejected}/{len(should_reject)} rejected",
        )
        accepted = 0
        for name, payload in should_accept:
            path = root / f"ok_{name}.glb"
            path.write_bytes(payload)
            try:
                with captured_output():
                    mg.validate_glb_file(path, quiet=True)
                accepted += 1
            except mg.DegenerateArtefactError:
                pass
        check(
            should_accept and accepted == len(should_accept),
            "COUNTER-CONTROL: legitimate fixtures are still ACCEPTED",
            f"{accepted}/{len(should_accept)} - a reject-everything guard "
            "fails here",
        )
    finally:
        shutil.rmtree(root, ignore_errors=True)


def test_writes_stay_confined_to_cache() -> None:
    section("7. write confinement (THE INVARIANT: nothing near Content/)")
    with Sandbox() as box:
        try:
            mg._require_inside_cache(box.root / "Content" / "RawAssets" / "x.glb")
            check(False, "a Content/ destination is refused", "it was ALLOWED")
        except RuntimeError as exc:
            check(True, "a Content/ destination is refused", str(exc)[:70])
        inside = mg._require_inside_cache(box.cache / ASSET / "meshy_raw.glb")
        check(
            str(inside).startswith(str(box.cache.resolve())),
            "a Cache/ destination is allowed",
        )


# ---------------------------------------------------------------------------
# 8-10. CLI contract
# ---------------------------------------------------------------------------


def test_views_parsing_rejects_what_the_endpoint_cannot_take() -> None:
    section("8. --views parsing matches the documented endpoint limits")
    check(
        mg.parse_views("Front, Side ,Back") == ("Front", "Side", "Back"),
        "whitespace is trimmed and order preserved",
    )
    check(mg.MULTIVIEW_MAX_IMAGES == 4, "the documented 1-4 image cap is pinned")
    for raw, why in (
        ("", "an empty view list"),
        (",,", "a list of separators only"),
        ("A,B,C,D,E", "five views (over the endpoint's cap of 4)"),
        ("Front,Front,Back", "a repeated view"),
    ):
        try:
            mg.parse_views(raw)
            check(False, f"{why} is rejected", f"{raw!r} was ACCEPTED")
        except ValueError as exc:
            check(True, f"{why} is rejected", str(exc)[:60])


def test_check_probes_the_multi_image_endpoint() -> None:
    section("9. --check is the standing preflight for this endpoint")
    seen: list[str] = []

    def recorder(method, path, api_key, payload=None, timeout=None):
        seen.append(path)
        if path == mg.EP_BALANCE:
            return {"balance": 1234}
        return []

    with patched(api_request=recorder,
                 resolve_api_key=lambda: (FAKE_KEY, "test double")):
        with captured_output():
            rc = mg.run_check()
    probed = " ".join(seen)
    check(rc == 0, "--check returns 0 against a healthy API double", f"got {rc}")
    check(
        mg.EP_MULTIIMAGE in probed,
        "--check probes the multi-image endpoint",
        "so a plan/endpoint regression is found BEFORE credits are spent",
    )
    check(
        mg.EP_BALANCE in probed and mg.EP_IMAGE3D in probed
        and mg.EP_RETEXTURE in probed,
        "CONTROL: --check still probes balance, retexture and image3d",
    )


def test_mode_and_exit_code_contract() -> None:
    section("10. the mode list and the exit-code contract")
    parser = mg.build_parser()
    modes = None
    for action in parser._actions:  # noqa: SLF001 - reading the real parser
        if action.dest == "mode":
            modes = tuple(action.choices or ())
    check(
        modes == ("retexture", "image3d", "multiimage"),
        "--mode offers all three modes",
        str(modes),
    )
    args = parser.parse_args(["--mode", "multiimage", ASSET])
    check(
        args.views == ",".join(mg.MULTIVIEW_DEFAULT_VIEWS),
        "--views defaults to the CHAR-2 pinned pattern",
        str(args.views),
    )
    check(mg.EXIT_DEGENERATE == 6, "exit 6 still means degenerate artefact")
    check(
        mg.ENGINE_BY_MODE["multiimage"] not in
        (mg.ENGINE_BY_MODE["image3d"], mg.ENGINE_BY_MODE["retexture"]),
        "the multi-view engine tag cannot be confused with the others",
        str(mg.ENGINE_BY_MODE),
    )


def main() -> int:
    print("TASK-1088 - meshy_generate.py --mode multiimage gate")
    print(f"tool: {SCRIPT_DIR / 'meshy_generate.py'}")
    print(f"endpoint under test: {mg.EP_MULTIIMAGE}")
    print(f"pinned views: {', '.join(mg.MULTIVIEW_DEFAULT_VIEWS)} "
          f"(max {mg.MULTIVIEW_MAX_IMAGES})")

    test_payload_carries_every_view_in_declared_order()
    test_missing_view_is_exit_5_and_sends_nothing()
    test_complete_set_reaches_the_multi_image_endpoint()
    test_refused_endpoint_is_loud_exit_4_never_a_fallback()
    test_redactor_still_scrubs_on_the_new_path()
    test_degeneracy_guard_still_rejects()
    test_writes_stay_confined_to_cache()
    test_views_parsing_rejects_what_the_endpoint_cannot_take()
    test_check_probes_the_multi_image_endpoint()
    test_mode_and_exit_code_contract()

    failures = [(label, detail) for ok, label, detail in _RESULTS if not ok]
    print(f"\n{'=' * 72}")
    print(f"{len(_RESULTS) - len(failures)}/{len(_RESULTS)} assertions passed.")
    if failures:
        print("FAILED:")
        for label, detail in failures:
            print(f"  - {label}  ({detail})")
        return 1
    print("ALL GREEN - every requested view reaches the multi-image endpoint in "
          "order, a missing view stops before the network, and a refused "
          "endpoint never becomes a quiet single-image run.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
