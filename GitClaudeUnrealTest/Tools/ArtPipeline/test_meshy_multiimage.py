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
      5. THE PINNED-THREE FENCE (TASK-1095, CHAR-2) - --views may ADD a view
         but may never DROP Front, Side or Back. The test that earns that
         row is `--views Front,Side,ThreeQuarter`: THREE views, so a count
         floor passes it, and it never sends Back. It must be REFUSED (exit
         64, naming --mode image3d) at the parser AND on the deciding path,
         with no network and no task. The other direction is controlled: a
         4-view superset and any permutation still run.
      6. THE SUCCESS LINE carries the view count and order READ FROM THE SET
         SENT - the request body and the resolved provenance - never from
         argv (SC-94 cl. A). Controlled by handing it an argv that disagrees
         with the body: the line must report the body.
      7. THE STATE-FILE FENCE (TASK-1096) - state.json / state_failed.json go
         through the SAME _require_inside_cache() as the GLB. A traversal-
         shaped CardID (../../Content/X) is refused BEFORE any file or
         directory is written ANYWHERE, in all three modes; a normal CardID
         still writes both files where it always did. PRE-EXISTING - it
         predates the multi-view work and is not a regression from it.

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


# ---------------------------------------------------------------------------
# 11-16. THE PINNED-THREE FENCE and the SUCCESS-line instrument (TASK-1095)
# ---------------------------------------------------------------------------


class RecordingRun:
    """Stands in for _run_mode: records the args it was handed, returns 0."""

    def __init__(self) -> None:
        self.args: argparse.Namespace | None = None
        self.calls = 0

    def __call__(self, args) -> int:
        self.calls += 1
        self.args = args
        return 0


def run_main(argv: list[str], run_mode) -> tuple[int | None, str, str]:
    """mg.main(argv) with the run replaced by `run_mode` and every remote
    call tripwired. Returns (code, kind, text): kind is 'usage' for the
    parser's SystemExit, 'return' for a plain return, and 'reached' when a
    Tripwire standing in for _run_mode was hit - i.e. the CLI fence FAILED
    and the run would have started. A real _run_mode is never reached from
    here, so MESHY_TOKEN is never read."""
    with patched(_run_mode=run_mode, api_request=Tripwire(),
                 create_task=Tripwire(), require_api_key=lambda: FAKE_KEY):
        with captured_output() as (out, err):
            try:
                code, kind = mg.main(argv), "return"
            except SystemExit as exc:
                code, kind = exc.code, "usage"
            except AssertionError:
                code, kind = None, "reached"
    return code, kind, out.getvalue() + err.getvalue()


def _no_state_written(box: Sandbox) -> tuple[bool, str]:
    asset_dir = box.cache / ASSET
    wrote = (sorted(p.name for p in asset_dir.glob("*"))
             if asset_dir.exists() else [])
    return (not wrote), (f"found {wrote}" if wrote else "asset dir empty or absent")


def test_fence_refuses_three_views_that_drop_a_pinned_one() -> None:
    section("11. THE ROW'S TEST: --views Front,Side,ThreeQuarter is REFUSED "
            "(three views - a COUNT check passes it)")
    narrowed = ("Front", "Side", "ThreeQuarter")
    # Every crop the request names EXISTS, so resolution alone would succeed,
    # and the request has exactly as many views as the pinned set, so a
    # cardinality floor is satisfied. Only an IDENTITY check can stop this.
    with Sandbox(views=("Front", "Side", "Back", "ThreeQuarter")) as box:
        check(
            all(box.paths[v].is_file() for v in narrowed),
            "CONTROL: every requested crop exists on disk (resolution alone "
            "would pass)",
            ", ".join(box.paths[v].name for v in narrowed),
        )
        check(
            len(narrowed) == len(mg.MULTIVIEW_DEFAULT_VIEWS),
            "CONTROL: the request has exactly as many views as the pinned set",
            f"{len(narrowed)} == {len(mg.MULTIVIEW_DEFAULT_VIEWS)} - a count "
            "floor cannot see this case",
        )
        # (i) the parser-level fence
        msg = ""
        try:
            mg.parse_views(",".join(narrowed))
            check(False, "parse_views REFUSES Front,Side,ThreeQuarter",
                  "it was ACCEPTED")
        except ValueError as exc:
            msg = str(exc)
            check(True, "parse_views REFUSES Front,Side,ThreeQuarter",
                  msg.splitlines()[0][:80])
        check("Back" in msg, "the refusal NAMES the dropped pinned view (Back)")
        check(
            "--mode image3d" in msg,
            "the refusal names --mode image3d as the legitimate single-view "
            "route",
        )
        # (ii) the CLI: exit 64, and the run is never entered
        run = Tripwire()
        code, kind, text = run_main(
            ["--mode", "multiimage", ASSET, "--views", ",".join(narrowed)], run
        )
        check(
            kind == "usage" and code == 64,
            "main() exits 64 (CLI usage) for Front,Side,ThreeQuarter",
            f"{kind} {code}",
        )
        check(not run.calls, "the run was never entered from the CLI",
              f"{len(run.calls)} call(s) - must be 0")
        check("Back" in text and "image3d" in text,
              "the CLI message names Back and --mode image3d")
        # (iii) THE DECIDING PATH: _run_mode itself, handed the same set,
        #       refuses before the network. A caller that skipped main()
        #       gains nothing.
        wire = Tripwire()
        finish = CapturedFinish()
        with patched(api_request=wire, create_task=wire,
                     _finish_task_common=finish,
                     require_api_key=lambda: FAKE_KEY):
            with captured_output() as (out, err):
                rc = mg._run_mode(make_args(views=narrowed))
        text = out.getvalue() + err.getvalue()
        check(rc == 64, "_run_mode returns 64 for Front,Side,ThreeQuarter",
              f"got {rc}")
        check(
            finish.calls == 0,
            "no task was created (the shared tail was never entered)",
            f"{finish.calls} call(s) - must be 0",
        )
        check(not wire.calls, "the network was never reached",
              f"{len(wire.calls)} call(s) - must be 0")
        check("Back" in text and "--mode image3d" in text,
              "the deciding-path message names Back and --mode image3d")
        check(
            "asked for only" not in text,
            "the old cardinality warn is GONE - replaced, not stacked under "
            "the fence",
        )
        ok, detail = _no_state_written(box)
        check(ok, "no state.json / state_failed.json was written for a "
                  "refused set", detail)


def test_fence_refuses_narrowed_sets() -> None:
    section("12. a NARROWED --views (Front,Side / Front / Side,Back) is exit "
            "64, no network, no task")
    for raw, dropped in (
        ("Front,Side", ("Back",)),
        ("Front", ("Side", "Back")),
        ("Side,Back", ("Front",)),
    ):
        msg = ""
        try:
            mg.parse_views(raw)
            check(False, f"parse_views refuses {raw!r}", "it was ACCEPTED")
        except ValueError as exc:
            msg = str(exc)
            check(True, f"parse_views refuses {raw!r}",
                  msg.splitlines()[0][:70])
        check(
            all(d in msg for d in dropped),
            f"{raw!r}: the refusal names every dropped view",
            ", ".join(dropped),
        )
    with Sandbox() as box:
        run = Tripwire()
        code, kind, text = run_main(
            ["--mode", "multiimage", ASSET, "--views", "Front,Side"], run
        )
        check(kind == "usage" and code == 64,
              "main() exits 64 for --views Front,Side", f"{kind} {code}")
        check(not run.calls, "the run is never entered from the CLI",
              f"{len(run.calls)} call(s) - must be 0")
        check("--mode image3d" in text,
              "the CLI message names --mode image3d")
        wire = Tripwire()
        finish = CapturedFinish()
        with patched(api_request=wire, create_task=wire,
                     _finish_task_common=finish,
                     require_api_key=lambda: FAKE_KEY):
            with captured_output():
                rc = mg._run_mode(make_args(views=("Front", "Side")))
        check(rc == 64, "_run_mode returns 64 for (Front, Side)", f"got {rc}")
        check(finish.calls == 0 and not wire.calls,
              "no task, no network for the narrowed set",
              f"{finish.calls} tail call(s), {len(wire.calls)} wire call(s)")
        ok, detail = _no_state_written(box)
        check(ok, "no state file was written for the narrowed set", detail)
    # A case slip is refused too - and the message says which spelling.
    try:
        mg.parse_views("front,Side,Back")
        check(False, "a case-slipped view ('front') is refused",
              "it was ACCEPTED")
    except ValueError as exc:
        check(
            "'front' is not 'Front'" in str(exc),
            "a case-slipped view ('front') is refused AND the message names "
            "the exact spelling",
            [ln for ln in str(exc).splitlines() if "exact" in ln][:1],
        )


def test_fence_accepts_supersets_and_permutations() -> None:
    section("13. the OTHER direction: a 4-view superset and any order still "
            "PASS (the flag survives)")
    four = ("Front", "Side", "Back", "ThreeQuarter")
    check(
        mg.parse_views(",".join(four)) == four,
        "parse_views accepts a 4-view superset in the order given",
        str(four),
    )
    check(
        mg.parse_views("ThreeQuarter,Front,Side,Back")
        == ("ThreeQuarter", "Front", "Side", "Back"),
        "the 4th slot may come FIRST - the fence is a set check, not a "
        "prefix check",
    )
    check(
        mg.parse_views("Back,Front,Side") == ("Back", "Front", "Side"),
        "a permutation of the pinned three is accepted in the order given",
    )
    with Sandbox(views=four) as box:
        finish = CapturedFinish()
        wire = Tripwire()
        with patched(_finish_task_common=finish, api_request=wire,
                     require_api_key=lambda: FAKE_KEY):
            with captured_output():
                rc = mg._run_mode(make_args(views=four))
        check(rc == 0, "a 4-view run returns 0", f"got {rc}")
        check(finish.calls == 1,
              "the 4-view run reaches the shared tail exactly once",
              f"{finish.calls} call(s)")
        seen = finish.seen or {}
        urls = (seen.get("payload") or {}).get("image_urls") or []
        check(len(urls) == 4, "four images are in the payload", f"{len(urls)}")
        check(
            (seen.get("inputs_block") or {}).get("view_order") == list(four),
            "provenance records all four, in order",
            str((seen.get("inputs_block") or {}).get("view_order")),
        )
        mismatches = (
            [f"index {i} ({v})" for i, v in enumerate(four)
             if hashlib.sha256(decode_data_uri(urls[i])).hexdigest()
             != sha256_of(box.paths[v])]
            if len(urls) == 4 else ["payload short"]
        )
        check(
            not mismatches,
            "each of the four image_urls[i] is the view at index i, byte "
            "for byte",
            ("mismatched: " + ", ".join(mismatches)) if mismatches
            else "sha256-matched 0..3",
        )
        finish2 = CapturedFinish()
        with patched(_finish_task_common=finish2, api_request=wire,
                     require_api_key=lambda: FAKE_KEY):
            with captured_output():
                rc2 = mg._run_mode(make_args(views=("Back", "Front", "Side")))
        order2 = ((finish2.seen or {}).get("inputs_block") or {}).get("view_order")
        check(
            rc2 == 0 and order2 == ["Back", "Front", "Side"],
            "a permuted pinned set runs and keeps the REQUESTED order",
            f"rc {rc2}, order {order2}",
        )


def test_default_views_unchanged_through_main() -> None:
    section("14. the DEFAULT (no --views) is still the pinned three and "
            "still exit 0")
    with Sandbox():
        run = RecordingRun()
        code, kind, _text = run_main(["--mode", "multiimage", ASSET], run)
        check(kind == "return" and code == 0,
              "main() with no --views returns 0", f"{kind} {code}")
        got = tuple(getattr(run.args, "views", ()) or ())
        check(
            run.calls == 1 and got == mg.MULTIVIEW_DEFAULT_VIEWS,
            "the run receives exactly the CHAR-2 pinned three, in order",
            str(got),
        )
        run2 = RecordingRun()
        code2, kind2, _ = run_main(
            ["--mode", "multiimage", ASSET, "--views", "Front,Side,Back"], run2
        )
        got2 = tuple(getattr(run2.args, "views", ()) or ())
        check(
            kind2 == "return" and code2 == 0 and got2 == mg.MULTIVIEW_DEFAULT_VIEWS,
            "--views Front,Side,Back (the default, spelled out) is identical",
            f"{kind2} {code2} {got2}",
        )


def _accepted_glb_bytes() -> bytes:
    for _name, payload, expected in mg.guard_fixtures():
        if not expected and payload is not None:
            return payload
    raise RuntimeError("guard_fixtures() offers no must-ACCEPT fixture")


@contextlib.contextmanager
def offline_task_tail(glb_bytes: bytes):
    """Stubs every REMOTE call _finish_task_common makes so the REAL tail
    (validate -> commit -> state.json -> the SUCCESS line) runs with no
    network, no key use and no credits."""

    def fake_download(url, dest, attempts):
        staged = dest.with_suffix(dest.suffix + ".part")
        staged.write_bytes(glb_bytes)
        return staged, mg.validate_glb_file(staged, quiet=True)

    with patched(
        api_request=Tripwire(),
        fetch_balance=lambda api_key: 100.0,
        create_task=lambda endpoint, payload, api_key, attempts: "task-offline-1",
        poll_task=lambda endpoint, task_id, api_key, **kw: {
            "id": task_id, "status": "SUCCEEDED",
            "model_urls": {"glb": "https://example.invalid/offline.glb"},
            "consumed_credits": 30,
        },
        download_file=fake_download,
    ):
        yield


def _success_line(text: str) -> str:
    for line in text.splitlines():
        if "SUCCESS:" in line:
            return line
    return ""


def _views_clause(line: str) -> str:
    """The `Views sent: N (...)` clause of a SUCCESS line, or ''."""
    start = line.find("Views sent:")
    if start < 0:
        return ""
    end = line.find(")", start)
    return line[start:end + 1] if end > 0 else line[start:]


def test_success_line_carries_views_sent_not_argv() -> None:
    section("15. the SUCCESS line carries the view COUNT and ORDER - read from "
            "the set SENT, never from argv")
    glb = _accepted_glb_bytes()
    with Sandbox() as box:
        asset_dir = box.cache / ASSET
        asset_dir.mkdir(parents=True, exist_ok=True)
        resolved = mg.resolve_multiview_images(ASSET, mg.MULTIVIEW_DEFAULT_VIEWS)
        params = {"ai_model": "latest"}
        payload = mg.build_multiimage_payload(params, resolved)
        inputs = {
            "view_count": len(resolved),
            "view_order": [v for v, _p in resolved],
            "view_images": [],
        }

        def record(mode: str = "multiimage") -> dict:
            return {
                "asset": ASSET, "mode": mode, "engine": mg.ENGINE_BY_MODE[mode],
                "api_base": mg.API_BASE, "started_utc": mg._utc_now(),
                "params": params, "status": "in-progress",
            }

        def finish(args, body, inputs_block, endpoint=mg.EP_MULTIIMAGE,
                   mode="multiimage"):
            with offline_task_tail(glb):
                with captured_output() as (out, err):
                    rc = mg._finish_task_common(
                        args, endpoint, body, FAKE_KEY, "meshy_raw.glb",
                        mg.ENGINE_BY_MODE[mode], inputs_block, asset_dir,
                        record(mode),
                    )
            return rc, _success_line(out.getvalue() + err.getvalue())

        rc, line = finish(make_args(), payload, inputs)
        check(rc == 0, "the offline tail returns 0", f"got {rc}")
        check(line.startswith("[meshy] SUCCESS:"), "a SUCCESS line was printed",
              line[:60])
        check("Views sent: 3" in line, "the SUCCESS line carries the view COUNT",
              _views_clause(line) or "ABSENT")
        check("(Front -> Side -> Back)" in line,
              "the SUCCESS line carries the view ORDER", _views_clause(line))
        # SC-94 cl. A CONTROL 1: argv claims FOUR views including one that
        # was never sent. The line must report the BODY. An instrument that
        # echoes args.views prints 4 and 'Phantom'.
        rc, line = finish(
            make_args(views=("Front", "Side", "Back", "Phantom")), payload, inputs
        )
        check(
            rc == 0 and "Views sent: 3" in line and "Phantom" not in line,
            "CONTROL (SC-94): argv says 4 views incl. 'Phantom'; the line "
            "reports the 3 that were SENT",
            _views_clause(line) or "ABSENT",
        )
        # CONTROL 2 - the 'delete the operation' test: send TWO images and
        # the instrument must move with the body.
        two = resolved[:2]
        rc, line = finish(
            make_args(), mg.build_multiimage_payload(params, two),
            {"view_count": 2, "view_order": [v for v, _p in two],
             "view_images": []},
        )
        clause = _views_clause(line)
        check(
            rc == 0 and clause == "Views sent: 2 (Front -> Side)",
            "CONTROL: a 2-image body prints exactly 'Views sent: 2 (Front -> "
            "Side)' - the instrument follows the body",
            clause or "ABSENT",
        )
        # CONTROL 3: the shipped image3d SUCCESS line is UNCHANGED.
        concept = write_png(box.inbox / f"{ASSET}.png", (5, 5, 5))
        body3 = dict(params)
        body3["image_url"] = mg.image_data_uri(concept)
        rc3, line3 = finish(
            make_args(mode="image3d"), body3,
            {"concept_image": concept.name, "concept_image_sha256": "n/a"},
            endpoint=mg.EP_IMAGE3D, mode="image3d",
        )
        check(
            rc3 == 0 and line3.startswith("[meshy] SUCCESS:")
            and "Views sent" not in line3,
            "CONTROL: the image3d SUCCESS line carries no Views clause "
            "(unchanged)",
            line3[:70],
        )


def test_clause8_one_liners() -> None:
    section("16. qa/TASK-1089 WARN-4/5/7: poll-404 narrative, retexture "
            "control, --ai-model help")
    # WARN-5: retexture + 403 -> exit 1 (shipped, unchanged); nothing of the
    # multi-image narrative leaks into it.
    with Sandbox() as box:
        (box.cache / ASSET).mkdir(parents=True, exist_ok=True)
        (box.cache / ASSET / "trellis_raw.glb").write_bytes(_accepted_glb_bytes())
        write_png(box.inbox / f"{ASSET}.png", (10, 10, 10))
        err_obj = mg.MeshyHttpError(
            403, '{"message":"forbidden"}', f"POST {mg.EP_RETEXTURE} (create task)"
        )
        finish = CapturedFinish(raise_error=err_obj)
        with patched(_finish_task_common=finish,
                     require_api_key=lambda: FAKE_KEY):
            with captured_output() as (out, err):
                rc = mg._run_mode(make_args(mode="retexture"))
        text = out.getvalue() + err.getvalue()
        check(rc == 1,
              "CONTROL: --mode retexture still exits 1 on a 403 (shipped, "
              "unchanged)", f"got {rc}")
        check(finish.calls == 1,
              "the retexture tail was actually entered (the control is live, "
              "not vacuous)", f"{finish.calls} call(s)")
        check(
            "fell back" not in text and "refused this account" not in text,
            "no multi-image narrative leaks into retexture",
        )
    # WARN-4: a POLL 404 keeps exit 4 (safety) but must not claim the
    # endpoint refused the account (narrative).
    with Sandbox():
        poll = mg.MeshyHttpError(
            404, '{"message":"task not found"}',
            f"GET {mg.EP_MULTIIMAGE}/task-x (poll)",
        )
        finish = CapturedFinish(raise_error=poll)
        with patched(_finish_task_common=finish,
                     require_api_key=lambda: FAKE_KEY):
            with captured_output() as (out, err):
                rc = mg._run_mode(make_args())
        text = out.getvalue() + err.getvalue()
        check(rc == 4, "a poll 404 is STILL exit 4 (safety unchanged)",
              f"got {rc}")
        check(
            "refused this account" not in text,
            "a poll 404 does NOT say the endpoint refused the account",
        )
        check(
            "task not found" in text and "fell back" in text,
            "the verbatim body and the no-fallback sentence still print",
        )
    with Sandbox():
        create = mg.MeshyHttpError(
            404, '{"message":"no route"}', f"POST {mg.EP_MULTIIMAGE} (create task)"
        )
        finish = CapturedFinish(raise_error=create)
        with patched(_finish_task_common=finish,
                     require_api_key=lambda: FAKE_KEY):
            with captured_output() as (out, err):
                rc = mg._run_mode(make_args())
        text = out.getvalue() + err.getvalue()
        check(
            rc == 4 and "refused this account" in text,
            "CONTROL: a CREATE 404 still says the endpoint refused the account",
            f"rc {rc}",
        )
    # WARN-7 and the --views help.
    parser = mg.build_parser()
    ai_help = next((a.help for a in parser._actions if a.dest == "ai_model"), "")  # noqa: SLF001
    check("meshy-7" in ai_help, "--ai-model help lists meshy-7 (the live enum)",
          ai_help[:60])
    views_help = next((a.help for a in parser._actions if a.dest == "views"), "")  # noqa: SLF001
    check(
        "image3d" in views_help and "64" in views_help,
        "--views help names the fence (exit 64) and the image3d route",
    )


# ---------------------------------------------------------------------------
# 17. THE STATE-FILE FENCE (TASK-1096) - PRE-EXISTING, all three modes
# ---------------------------------------------------------------------------


def _tree(root: Path) -> set[str]:
    """Every path under `root`, relative - so 'nothing appeared' is a set
    equality over the whole sandbox, not a spot check on the one place a
    write is expected."""
    return {str(p.relative_to(root)) for p in root.rglob("*")}


def test_state_files_are_confined_to_cache() -> None:
    section("17. THE STATE-FILE FENCE (TASK-1096, PRE-EXISTING, all three "
            "modes): a traversal-shaped CardID is refused BEFORE any file or "
            "directory is written ANYWHERE - and a normal CardID still writes "
            "BOTH state files where it always did")
    glb = _accepted_glb_bytes()
    with Sandbox() as box:
        # Nest CACHE_DIR two levels down so the row's exact shape
        # (..\..\Content\X) escapes Cache/ but lands INSIDE the sandbox root.
        # Under a mutation that bypasses the fence the stray mkdir then lands
        # where this test can see it and the sandbox can delete it - never in
        # the real %TEMP%, never in the real Content/. Sandbox.__exit__
        # restores the module's CACHE_DIR.
        deep_cache = box.root / "deep" / "deeper" / "Cache"
        deep_cache.mkdir(parents=True)
        mg.CACHE_DIR = deep_cache
        row_shape = "..\\..\\Content\\X"
        shapes = (
            ("the row's shape, backslashes", row_shape, "multiimage"),
            ("forward slashes", "../../Content/X", "multiimage"),
            ("a real-looking prefix then ..", ASSET + "/../../../Content/X",
             "multiimage"),
            # No '..' at all: pathlib's '/' REPLACES the left operand when the
            # right one is absolute, so CACHE_DIR / "C:\\..." IS "C:\\...".
            ("an ABSOLUTE path", str(box.root / "Content" / "X"), "multiimage"),
            ("the row's shape, --mode retexture", row_shape, "retexture"),
            ("the row's shape, --mode image3d", row_shape, "image3d"),
        )
        for label, evil, mode in shapes:
            target = (deep_cache / evil).resolve()
            # CONTROLS: the shape genuinely escapes Cache/ - and stays inside
            # the sandbox. Without the first, every refusal below tests nothing.
            check(deep_cache.resolve() not in target.parents
                  and target != deep_cache.resolve(),
                  f"CONTROL: {label} resolves OUTSIDE Cache/", str(target))
            check(box.root.resolve() in target.parents,
                  f"CONTROL: {label} stays inside the sandbox", str(target))
            before = _tree(box.root)
            wire, finish = Tripwire(), CapturedFinish()
            with patched(_finish_task_common=finish, api_request=wire,
                         create_task=wire, require_api_key=lambda: FAKE_KEY):
                with captured_output() as (out, err):
                    rc = mg._run_mode(make_args(asset=evil, mode=mode))
            text = out.getvalue() + err.getvalue()
            new = sorted(_tree(box.root) - before)
            check(rc == 1, f"{label}: _run_mode returns 1 (the guard's "
                  "RuntimeError -> the exit 1 the tail already maps it to)",
                  f"got {rc}")
            check(not new, f"{label}: NOTHING appeared anywhere under the "
                  "sandbox (outside Cache/ or inside it)",
                  f"new: {new}" if new else "tree unchanged")
            check(not target.exists(),
                  f"{label}: the escaped path was not created")
            check(not any(deep_cache.iterdir()),
                  f"{label}: Cache/ itself is still empty")
            check(finish.calls == 0 and not wire.calls,
                  f"{label}: no task, no network",
                  f"{finish.calls} tail call(s), {len(wire.calls)} wire call(s)")
            check("Refusing to write outside Cache/" in text
                  and "no state file was written" in text,
                  f"{label}: the refusal names the fence and says no state "
                  "file was written")

        # The CLI path: main() with the REAL _run_mode returns 1 - a clean
        # return, not a traceback and not a SystemExit.
        before = _tree(box.root)
        with patched(api_request=Tripwire(), create_task=Tripwire(),
                     require_api_key=lambda: FAKE_KEY):
            with captured_output():
                try:
                    code, kind = mg.main(["--mode", "image3d", row_shape]), "return"
                except SystemExit as exc:
                    code, kind = exc.code, "usage"
                except Exception as exc:  # noqa: BLE001
                    code, kind = None, f"raised {type(exc).__name__}"
        check(code == 1 and kind == "return",
              "main() returns 1 for the row's shape - a return, not a "
              "traceback", f"{kind} {code}")
        new = sorted(_tree(box.root) - before)
        check(not new, "main(): still nothing anywhere",
              f"new: {new}" if new else "tree unchanged")

        # The WRITERS refuse on their own, independent of _run_mode: the same
        # guard commit_staged() applies to the GLB, at the write site - so a
        # future direct caller cannot escape either.
        escaped_dir = deep_cache / row_shape
        before = _tree(box.root)
        writers = (
            ("write_failed_state",
             lambda: mg.write_failed_state(escaped_dir, {"status": "failed"})),
            ("write_success_state",
             lambda: mg.write_success_state(
                 escaped_dir, mg.ENGINE_BY_MODE["multiimage"],
                 {"status": "success"})),
        )
        for name, call in writers:
            try:
                with captured_output():
                    call()
                check(False, f"{name} refuses an escaped asset_dir", "it WROTE")
            except RuntimeError as exc:
                check("Refusing to write outside Cache/" in str(exc),
                      f"{name} refuses an escaped asset_dir", str(exc)[:70])
        new = sorted(_tree(box.root) - before)
        check(not new, "the direct writer calls created nothing anywhere",
              f"new: {new}" if new else "tree unchanged")

        # (b) A NORMAL CardID still writes BOTH state files where it always
        # did - in ALL THREE modes for the failure record (the shared tail's
        # except-clauses), and via the real offline tail for state.json.
        mg.CACHE_DIR = box.cache
        asset_dir = box.cache / ASSET
        write_png(box.inbox / f"{ASSET}.png", (10, 10, 10))  # concept / style ref
        asset_dir.mkdir(parents=True, exist_ok=True)
        (asset_dir / "trellis_raw.glb").write_bytes(glb)     # retexture donor
        failed = asset_dir / "state_failed.json"
        for mode in ("retexture", "image3d", "multiimage"):
            if failed.exists():
                failed.unlink()
            err_obj = mg.MeshyHttpError(
                500, '{"message":"boom"}', f"POST {mg.EP_MULTIIMAGE} (create task)"
            )
            with patched(_finish_task_common=CapturedFinish(raise_error=err_obj),
                         require_api_key=lambda: FAKE_KEY):
                with captured_output():
                    rc = mg._run_mode(make_args(mode=mode))
            present = failed.is_file()
            record = json.loads(failed.read_text(encoding="utf-8")) if present else {}
            check(rc == 1 and present and record.get("asset") == ASSET
                  and record.get("mode") == mode
                  and record.get("status") == "failed",
                  f"--mode {mode}: state_failed.json is still written at "
                  "Cache/<CardID>/state_failed.json with the run's record",
                  f"rc {rc}, {'present' if present else 'ABSENT'}, "
                  f"mode={record.get('mode')}")
        check(not (asset_dir / "state.json").exists(),
              "a failure still never writes state.json")
        resolved = mg.resolve_multiview_images(ASSET, mg.MULTIVIEW_DEFAULT_VIEWS)
        params = {"ai_model": "latest"}
        with offline_task_tail(glb):
            with captured_output():
                rc = mg._finish_task_common(
                    make_args(), mg.EP_MULTIIMAGE,
                    mg.build_multiimage_payload(params, resolved), FAKE_KEY,
                    "meshy_raw.glb", mg.ENGINE_BY_MODE["multiimage"],
                    {"view_count": len(resolved),
                     "view_order": [v for v, _p in resolved],
                     "view_images": []},
                    asset_dir,
                    {"asset": ASSET, "mode": "multiimage",
                     "engine": mg.ENGINE_BY_MODE["multiimage"],
                     "api_base": mg.API_BASE, "started_utc": mg._utc_now(),
                     "params": params, "status": "in-progress"},
                )
        state = asset_dir / "state.json"
        merged = (json.loads(state.read_text(encoding="utf-8"))
                  if state.is_file() else {})
        check(rc == 0 and state.is_file()
              and merged.get("engine") == mg.ENGINE_BY_MODE["multiimage"],
              "state.json is still written at Cache/<CardID>/state.json with "
              "the merged provenance",
              f"rc {rc}, {'present' if state.is_file() else 'ABSENT'}, "
              f"engine={merged.get('engine')}")
        check((asset_dir / "meshy_raw.glb").is_file(),
              "CONTROL: the GLB path is untouched - meshy_raw.glb committed "
              "beside it")


def main() -> int:
    print("TASK-1088 / TASK-1095 / TASK-1096 - meshy_generate.py --mode multiimage gate")
    print(f"tool: {SCRIPT_DIR / 'meshy_generate.py'}")
    print(f"endpoint under test: {mg.EP_MULTIIMAGE}")
    print(f"pinned views: {', '.join(mg.MULTIVIEW_DEFAULT_VIEWS)} "
          f"(max {mg.MULTIVIEW_MAX_IMAGES}; the pinned three are an IDENTITY, "
          "a 4th may be added, none may be dropped)")

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
    test_fence_refuses_three_views_that_drop_a_pinned_one()
    test_fence_refuses_narrowed_sets()
    test_fence_accepts_supersets_and_permutations()
    test_default_views_unchanged_through_main()
    test_success_line_carries_views_sent_not_argv()
    test_clause8_one_liners()
    test_state_files_are_confined_to_cache()

    failures = [(label, detail) for ok, label, detail in _RESULTS if not ok]
    print(f"\n{'=' * 72}")
    print(f"{len(_RESULTS) - len(failures)}/{len(_RESULTS)} assertions passed.")
    if failures:
        print("FAILED:")
        for label, detail in failures:
            print(f"  - {label}  ({detail})")
        return 1
    print("ALL GREEN - every requested view reaches the multi-image endpoint in "
          "order, a missing view stops before the network, a refused endpoint "
          "never becomes a quiet single-image run, and --views can add a view "
          "but never drop Front, Side or Back - and a state file can never "
          "land outside Cache/.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
