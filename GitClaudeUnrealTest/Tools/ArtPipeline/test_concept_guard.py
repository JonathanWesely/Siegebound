#!/usr/bin/env python3
"""Gate for concept_generate.py's degeneracy guard (TASK-864, law SC-§39.1).

`concept_generate.py --check` carries the guard's inline positive control - it
proves the guard goes RED on synthesised degenerate frames and stays green on
legitimately dark ones, and it runs every time anyone smoke-tests the tool. This
file is the WIDER gate that does not belong in a per-run smoke test:

    1. NEGATIVE CONTROL AT SCALE - every real shipped concept PNG on disk
       (Tools/ArtPipeline/Inbox/*.png + Content/RawAssets/Concepts/*.png) must be
       ACCEPTED. Zero false positives on the whole corpus, or the guard is the
       "rejects everything" defect wearing different clothes.
    2. POSITIVE CONTROL, PER CHECK - re-runs the shared fixtures and asserts the
       EXACT set of checks that fired, so no check can quietly stop working
       behind another one that still fires.
    3. END-TO-END, THROUGH THE REAL generate_one() - with a scripted stand-in for
       InferenceClient, so the ORDER of operations is observed rather than read:
         - a degenerate roll returns exit 6 and leaves the previous PNG
           BYTE-FOR-BYTE intact, even under --force;
         - the previous PNG is still the OLD one AT THE MOMENT validation runs
           (the direct proof that the swap is downstream of the check, not
           upstream of it);
         - a valid roll under --force DOES replace the previous PNG, so the
           guard is not simply refusing everything;
         - a pinned seed is not retried (identical input reproduces identically -
           TASK-833 measured seed 71031 going black twice);
         - an unpinned seed IS retried, because each attempt draws a fresh seed;
         - a rejected frame is quarantined, not silently discarded.
    4. EXIT-CODE HYGIENE - 6 does not collide with the shipped family's
       0/1/2/3/4/5/64.

No network, no HF quota, no HF_TOKEN, no engine, no Git. Nothing under
Tools/ArtPipeline/Inbox/ is written: INBOX_DIR is redirected to a temp directory
for every end-to-end case, and the real corpus is only ever READ.

Run:
    uv run test_concept_guard.py            (or: .venv/Scripts/python.exe ...)

Exit codes:
    0  every assertion passed
    1  at least one assertion failed (the ledger names which)
    5  the negative-control corpus could not be found - the run is VOID, not
       green: a no-false-positive claim from an instrument with no input is
       exactly the blind measurement SC-§39 exists to forbid
"""

from __future__ import annotations

import argparse
import hashlib
import shutil
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

import concept_generate as cg  # noqa: E402 - deliberate: needs the path above

REPO_ROOT = SCRIPT_DIR.parents[1]
CORPUS_DIRS = [
    SCRIPT_DIR / "Inbox",
    REPO_ROOT / "Content" / "RawAssets" / "Concepts",
]

# ---------------------------------------------------------------------------
# Minimal ledger (house style: no pytest dependency, one readable transcript)
# ---------------------------------------------------------------------------

_RESULTS: list[tuple[bool, str, str]] = []


def check(passed: bool, label: str, detail: str = "") -> bool:
    _RESULTS.append((bool(passed), label, detail))
    mark = "PASS" if passed else "FAIL"
    print(f"  [{mark}] {label}" + (f"  -- {detail}" if detail else ""), flush=True)
    return bool(passed)


def section(title: str) -> None:
    print(f"\n=== {title} ===", flush=True)


def sha256_of(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


# ---------------------------------------------------------------------------
# Test doubles
# ---------------------------------------------------------------------------


class ScriptedClient:
    """Stands in for huggingface_hub.InferenceClient.

    Returns the scripted frames in order (repeating the last one once exhausted)
    and counts the calls, so retry behaviour is observed rather than assumed.
    """

    def __init__(self, frames: list) -> None:
        self._frames = frames
        self.calls = 0

    def text_to_image(self, prompt, **kwargs):
        frame = self._frames[min(self.calls, len(self._frames) - 1)]
        self.calls += 1
        return frame


def make_args(**overrides) -> argparse.Namespace:
    base = dict(
        force=True, seed=None, width=1024, height=1024, steps=40,
        attempts=1, timeout_minutes=5.0,
    )
    base.update(overrides)
    return argparse.Namespace(**base)


ENTRY = {"prompt": "a test subject", "negative_prompt": None, "seed": None}


def fixture_named(name: str):
    for fixture_name, image, _expected, _why in cg.build_guard_fixtures():
        if fixture_name == name:
            return image
    raise KeyError(f"no guard fixture named {name!r}")


class Sandbox:
    """A temp Inbox with INBOX_DIR redirected at it. Never touches the real one."""

    def __init__(self, seed_with: Path | None = None, card_id: str = "TestCard") -> None:
        self.card_id = card_id
        self.root = Path(tempfile.mkdtemp(prefix="task864-"))
        self.inbox = self.root / "Inbox"
        self.inbox.mkdir()
        self._saved = cg.INBOX_DIR
        cg.INBOX_DIR = self.inbox
        self.dest = self.inbox / f"{card_id}.png"
        self.seeded_sha = None
        if seed_with is not None:
            shutil.copy2(seed_with, self.dest)
            self.seeded_sha = sha256_of(self.dest)

    def close(self) -> None:
        cg.INBOX_DIR = self._saved
        shutil.rmtree(self.root, ignore_errors=True)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
        return False


# ---------------------------------------------------------------------------
# 1. Negative control at scale - the real shipped corpus
# ---------------------------------------------------------------------------


def test_real_corpus() -> int:
    from PIL import Image

    section("1. NEGATIVE CONTROL - every real shipped concept must be ACCEPTED")
    files: list[Path] = []
    for directory in CORPUS_DIRS:
        if directory.is_dir():
            files.extend(sorted(directory.glob("*.png")))
        else:
            print(f"  (corpus dir absent: {directory})")

    if not files:
        print("\n  VOID: no real concept PNG was found in any corpus directory.")
        print("  A 'no false positives' claim from an instrument with nothing to")
        print("  measure is not a green result (SC-§39). Restore the corpus and")
        print("  re-run; exiting 5.")
        return 5

    worst = {"p999": (1.0, ""), "stddev": (1.0, "")}
    false_positives: list[str] = []
    for path in files:
        with Image.open(path) as image:
            image.load()
            stats = cg.measure_image(image)
        fired = cg.assess_degeneracy(stats)
        if fired:
            false_positives.append(
                f"{path.name} -> {[name for name, _ in fired]} "
                f"({cg.describe_stats(stats)})"
            )
        for key in worst:
            if stats[key] < worst[key][0]:
                worst[key] = (stats[key], path.name)

    check(
        not false_positives,
        f"{len(files)} real concept PNGs assessed, 0 rejected",
        "; ".join(false_positives) if false_positives else
        f"corpus worst p99.9={worst['p999'][0]:.4f} ({worst['p999'][1]}), "
        f"worst stddev={worst['stddev'][0]:.4f} ({worst['stddev'][1]})",
    )

    # The margins are the reason the thresholds are defensible; print them so the
    # next reader re-derives rather than trusting the constants' comment.
    print(f"  margin: p99.9  floor {cg.MIN_HIGHLIGHT_P999:.4f} vs corpus worst "
          f"{worst['p999'][0]:.4f}  ({worst['p999'][0] / cg.MIN_HIGHLIGHT_P999:.1f}x headroom)")
    print(f"  margin: stddev floor {cg.MIN_LUMA_STDDEV:.4f} vs corpus worst "
          f"{worst['stddev'][0]:.4f}  ({worst['stddev'][0] / cg.MIN_LUMA_STDDEV:.1f}x headroom)")
    return 0


# ---------------------------------------------------------------------------
# 2. Positive control - the shared fixtures, per check
# ---------------------------------------------------------------------------


def test_fixtures() -> None:
    section("2. POSITIVE CONTROL - each check fires on its own fixture")
    fired_somewhere: set[str] = set()
    for name, image, expected, why in cg.build_guard_fixtures():
        stats = cg.measure_image(image)
        fired = {check_name for check_name, _ in cg.assess_degeneracy(stats)}
        fired_somewhere |= fired
        check(
            fired == expected,
            f"fixture '{name}' fires exactly {sorted(expected) or ['(nothing)']}",
            f"got {sorted(fired) or ['(nothing)']} - {why}",
        )

    every_check = {cg.CHECK_SIZE, cg.CHECK_UNLIT, cg.CHECK_FLAT, cg.CHECK_TRANSPARENT}
    check(
        every_check <= fired_somewhere,
        "every degeneracy check is exercised by at least one fixture",
        f"never fired: {sorted(every_check - fired_somewhere) or 'none'}",
    )


# ---------------------------------------------------------------------------
# 3. End-to-end through the real generate_one()
# ---------------------------------------------------------------------------


def test_force_is_not_destructive() -> None:
    section("3a. --force does NOT destroy a good roll when the new one is degenerate")
    good = next(
        (p for d in CORPUS_DIRS if d.is_dir() for p in sorted(d.glob("*.png"))), None
    )
    if good is None:
        check(False, "a real concept PNG was available to stand in as the prior roll")
        return

    with Sandbox(seed_with=good) as box:
        client = ScriptedClient([fixture_named("pure-black")])
        rc = cg.generate_one(client, box.card_id, ENTRY, make_args(force=True, seed=71031))
        check(rc == cg.EXIT_DEGENERATE,
              f"a black frame returns exit {cg.EXIT_DEGENERATE} (degenerate), not 0",
              f"got {rc}")
        check(box.dest.exists(), "the previous PNG still exists")
        check(box.dest.exists() and sha256_of(box.dest) == box.seeded_sha,
              "the previous PNG is BYTE-FOR-BYTE unchanged",
              f"before={box.seeded_sha[:16]} after="
              f"{sha256_of(box.dest)[:16] if box.dest.exists() else 'GONE'}")
        rejected = list((box.inbox / cg.REJECTED_DIR_NAME).glob(f"{box.card_id}.*.png"))
        check(len(rejected) == 1,
              "the rejected frame was quarantined for inspection, not discarded",
              f"{[p.name for p in rejected]}")
        leftovers = list(box.inbox.glob(".*.tmp-*"))
        check(not leftovers, "no staging temp file was left behind",
              f"{[p.name for p in leftovers]}")


def test_validation_runs_before_the_swap() -> None:
    section("3b. ORDER OF OPERATIONS - the swap happens AFTER validation, observed")
    good = next(
        (p for d in CORPUS_DIRS if d.is_dir() for p in sorted(d.glob("*.png"))), None
    )
    if good is None:
        check(False, "a real concept PNG was available to stand in as the prior roll")
        return

    with Sandbox(seed_with=good) as box:
        seen: list[str | None] = []
        original = cg.validate_png_file

        def spy(path: Path):
            # Snapshot the TARGET at the exact moment validation is invoked.
            seen.append(sha256_of(box.dest) if box.dest.exists() else None)
            return original(path)

        cg.validate_png_file = spy
        try:
            # A VALID frame, so the run commits - which is the interesting case:
            # "validate then overwrite" and "overwrite then validate" both end
            # with the new file in place, and only this snapshot tells them apart.
            client = ScriptedClient([fixture_named("typical-concept")])
            rc = cg.generate_one(client, box.card_id, ENTRY, make_args(force=True))
        finally:
            cg.validate_png_file = original

        check(rc == 0, "a valid frame succeeds (exit 0)", f"got {rc}")
        check(len(seen) == 1, "validation ran exactly once", f"{len(seen)} call(s)")
        check(seen and seen[0] == box.seeded_sha,
              "at validation time the target still held the PREVIOUS roll",
              "the overwrite is downstream of the check, not upstream of it")
        check(box.dest.exists() and sha256_of(box.dest) != box.seeded_sha,
              "after a PASSING validation the target was replaced",
              "proves the guard is not simply refusing every roll")


def test_valid_frame_still_commits_without_prior() -> None:
    section("3c. a valid frame with NO prior file is written normally")
    with Sandbox() as box:
        client = ScriptedClient([fixture_named("typical-concept")])
        rc = cg.generate_one(client, box.card_id, ENTRY, make_args(force=False))
        check(rc == 0, "exit 0", f"got {rc}")
        check(box.dest.exists() and box.dest.stat().st_size > 0,
              "the PNG was written")


def test_degenerate_first_roll_writes_nothing() -> None:
    section("3d. a degenerate FIRST roll leaves no bogus PNG behind")
    with Sandbox() as box:
        client = ScriptedClient([fixture_named("near-black-noise")])
        rc = cg.generate_one(client, box.card_id, ENTRY, make_args(force=False, seed=71031))
        check(rc == cg.EXIT_DEGENERATE, f"exit {cg.EXIT_DEGENERATE}", f"got {rc}")
        check(not box.dest.exists(),
              "Inbox/<CardID>.png was NOT created from a degenerate frame",
              "a bad roll must not become the artefact the next stage consumes")


def test_pinned_seed_is_not_retried() -> None:
    section("3e. RETRY POLICY - a pinned seed is not retried; an unpinned one is")
    with Sandbox() as box:
        client = ScriptedClient([fixture_named("pure-black")])
        rc = cg.generate_one(
            client, box.card_id, ENTRY, make_args(force=True, seed=71031, attempts=3)
        )
        check(rc == cg.EXIT_DEGENERATE, "pinned seed -> exit 6", f"got {rc}")
        check(client.calls == 1,
              "pinned seed made exactly ONE call (no wasted quota on a "
              "deterministic re-roll)",
              f"{client.calls} call(s)")

    with Sandbox() as box:
        client = ScriptedClient([fixture_named("pure-black")])
        rc = cg.generate_one(
            client, box.card_id, ENTRY, make_args(force=True, seed=None, attempts=3)
        )
        check(rc == cg.EXIT_DEGENERATE, "unpinned seed -> exit 6 after retries", f"got {rc}")
        check(client.calls == 3,
              "unpinned seed retried the full attempt budget (each draws a fresh seed)",
              f"{client.calls} call(s)")

    with Sandbox() as box:
        # A transient degenerate roll followed by a good one must SUCCEED.
        client = ScriptedClient([fixture_named("pure-black"), fixture_named("typical-concept")])
        rc = cg.generate_one(
            client, box.card_id, ENTRY, make_args(force=True, seed=None, attempts=3)
        )
        check(rc == 0, "unpinned seed recovers when a later attempt is good", f"got {rc}")
        check(box.dest.exists(), "the recovered roll was written")


def test_unreadable_artefact_is_caught() -> None:
    section("3f. a write that produces an UNREADABLE file is caught, not reported OK")
    with Sandbox() as box:
        original = cg.stage_png

        def stage_empty(image, dest):
            staged = original(image, dest)
            staged.write_bytes(b"")   # simulate the save_asset failure mode
            return staged

        cg.stage_png = stage_empty
        try:
            client = ScriptedClient([fixture_named("typical-concept")])
            # attempts=2 with a PINNED seed on purpose: an unreadable artefact is
            # a write failure, not a model output, so the "pinned seeds are
            # deterministic, do not retry" rule must NOT suppress the retry here.
            rc = cg.generate_one(
                client, box.card_id, ENTRY, make_args(force=True, seed=1, attempts=2)
            )
        finally:
            cg.stage_png = original
        check(rc == cg.EXIT_DEGENERATE,
              "a zero-byte artefact is rejected even though the image was valid",
              f"got {rc}")
        check(client.calls == 2,
              "an unreadable artefact IS retried despite the pinned seed",
              f"{client.calls} call(s) - a disk failure says nothing about the seed")
        check(not box.dest.exists(), "nothing was committed")


def test_confinement_still_holds() -> None:
    section("3g. the Inbox confinement guard still refuses to escape")
    with Sandbox() as box:
        escaped = box.root / "elsewhere.png"
        try:
            cg.stage_png(fixture_named("typical-concept"), escaped)
            check(False, "staging outside Inbox/ was refused", "it was ALLOWED")
        except RuntimeError as exc:
            check("Refusing to write outside Inbox/" in str(exc),
                  "staging outside Inbox/ was refused", str(exc))
        check(not escaped.exists(), "no file was created outside Inbox/")


def test_run_all_names_degenerate_cards() -> None:
    section("3h. --all keeps going past a degenerate card and NAMES it")
    with Sandbox(card_id="GoodCard") as box:
        stub_prompts = {
            "BadCard": {"prompt": "x", "negative_prompt": None, "seed": 71031},
            "GoodCard": {"prompt": "y", "negative_prompt": None, "seed": 1},
        }
        frames_by_card = {
            "BadCard": fixture_named("pure-black"),
            "GoodCard": fixture_named("typical-concept"),
        }

        class PerCardClient:
            def text_to_image(self, prompt, **kwargs):
                # BadCard sorts first, so the batch must survive it to reach GoodCard.
                return frames_by_card["BadCard" if prompt.startswith("x") else "GoodCard"]

        saved = (cg.require_token, cg.build_client, cg.load_prompts)
        cg.require_token = lambda: "not-a-real-token"
        cg.build_client = lambda token, timeout: PerCardClient()
        cg.load_prompts = lambda path: stub_prompts
        try:
            rc = cg.run_all(make_args(force=True, attempts=1))
        finally:
            cg.require_token, cg.build_client, cg.load_prompts = saved

        check(rc == cg.EXIT_DEGENERATE,
              f"--all returns {cg.EXIT_DEGENERATE} when any card was degenerate",
              f"got {rc}")
        check((box.inbox / "GoodCard.png").exists(),
              "the batch continued past the bad card and generated the good one",
              "a per-card degeneracy must not stall the whole batch")
        check(not (box.inbox / "BadCard.png").exists(),
              "the degenerate card produced no PNG")


# ---------------------------------------------------------------------------
# 4. Exit-code hygiene
# ---------------------------------------------------------------------------


def test_exit_code_hygiene() -> None:
    section("4. EXIT CODES - 6 is distinct from the shipped family")
    family = {0: "success", 1: "generic failure", 2: "token unset", 3: "quota",
              4: "API drift", 5: "input missing", 64: "CLI usage"}
    check(cg.EXIT_DEGENERATE not in family,
          f"exit {cg.EXIT_DEGENERATE} does not collide with {sorted(family)}",
          f"family: {family}")
    check(f"    {cg.EXIT_DEGENERATE}  " in (cg.__doc__ or ""),
          f"exit {cg.EXIT_DEGENERATE} is documented in the module exit-code table")
    epilog = cg.build_parser().epilog or ""
    check(f"{cg.EXIT_DEGENERATE} DEGENERATE" in epilog,
          f"exit {cg.EXIT_DEGENERATE} is documented in `--help`")


# ---------------------------------------------------------------------------
# 5. META-CONTROL - can the guard's own control go red?
# ---------------------------------------------------------------------------


def test_the_control_itself_can_go_red() -> None:
    """The control that proves the guard is only worth as much as its own ability
    to fail. This is SC-§39 applied one level up: a self-test never observed
    failing is indistinguishable from no self-test, which is precisely the shape
    of the defect TASK-864 fixes.
    """
    import contextlib
    import io

    section("5. META-CONTROL - the guard's own self-test is proven able to FAIL")
    original = cg.assess_degeneracy

    def run_quietly(fn):
        # Both streams: the deliberately-broken guard below prints a full failure
        # report, and a PASSING assertion must not look like a crashed run.
        with contextlib.redirect_stdout(io.StringIO()), \
                contextlib.redirect_stderr(io.StringIO()):
            return fn()

    # Direction A: a guard that never rejects anything (the TASK-864 defect
    # itself, re-injected). The control must notice.
    cg.assess_degeneracy = lambda stats: []
    try:
        problems = run_quietly(cg.run_guard_self_test)
    finally:
        cg.assess_degeneracy = original
    check(bool(problems),
          "a guard that NEVER rejects is caught by the control",
          f"{len(problems)} disagreement(s) reported")

    # Direction B: a guard that rejects everything - the failure mode a careless
    # fix for direction A would introduce.
    cg.assess_degeneracy = lambda stats: [(cg.CHECK_FLAT, "always fires")]
    try:
        problems = run_quietly(cg.run_guard_self_test)
    finally:
        cg.assess_degeneracy = original
    check(bool(problems),
          "a guard that ALWAYS rejects is caught by the control",
          f"{len(problems)} disagreement(s) reported")

    # And the control is wired into --check, not merely available: a broken guard
    # must fail the whole smoke test, not just log a note.
    cg.assess_degeneracy = lambda stats: []
    try:
        rc = run_quietly(
            lambda: cg.run_check(argparse.Namespace(probe=False))
        )
    finally:
        cg.assess_degeneracy = original
    check(rc == 1,
          "`--check` FAILS (exit 1) while the guard is broken",
          f"got {rc}")

    # Sanity: with the real guard restored, --check passes again. Without this
    # the two assertions above could both be satisfied by a permanently red run.
    rc = run_quietly(lambda: cg.run_check(argparse.Namespace(probe=False)))
    check(rc == 0, "`--check` passes again once the real guard is restored",
          f"got {rc}")


# ---------------------------------------------------------------------------


def main() -> int:
    print("TASK-864 - concept_generate.py degeneracy guard gate")
    print(f"tool: {SCRIPT_DIR / 'concept_generate.py'}")
    print(f"thresholds: MIN_HIGHLIGHT_P999={cg.MIN_HIGHLIGHT_P999} "
          f"MIN_LUMA_STDDEV={cg.MIN_LUMA_STDDEV} "
          f"MIN_ARTEFACT_EDGE_PX={cg.MIN_ARTEFACT_EDGE_PX}")

    corpus_rc = test_real_corpus()
    if corpus_rc:
        return corpus_rc
    test_fixtures()
    test_force_is_not_destructive()
    test_validation_runs_before_the_swap()
    test_valid_frame_still_commits_without_prior()
    test_degenerate_first_roll_writes_nothing()
    test_pinned_seed_is_not_retried()
    test_unreadable_artefact_is_caught()
    test_confinement_still_holds()
    test_run_all_names_degenerate_cards()
    test_exit_code_hygiene()
    test_the_control_itself_can_go_red()

    failures = [(label, detail) for ok, label, detail in _RESULTS if not ok]
    print(f"\n{'=' * 72}")
    print(f"{len(_RESULTS) - len(failures)}/{len(_RESULTS)} assertions passed.")
    if failures:
        print("FAILED:")
        for label, detail in failures:
            print(f"  - {label}  ({detail})")
        return 1
    print("ALL GREEN - the guard rejects degenerate frames, accepts every real "
          "concept, and never overwrites a good roll with a bad one.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
