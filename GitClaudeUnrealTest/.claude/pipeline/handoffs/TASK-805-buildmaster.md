# TASK-805 — [MERGE-3] build-master: ONE compile · the suite · ONE commit

**Outcome: ✅ compile GREEN (`Result: Succeeded`) · ✅ suite `306`, declared == actual, all five anchors exact · ✅ ONE commit on `main` · ⛔ NOT pushed · ⛔⛔ THE PIE HALF IS ⛔ NOT DONE AND IS ⛔ OWED TO JONATHAN (§5) — and `TASK-802`'s five rows ride forward UNPAID with it.**

---

## 1. MEASURED FIRST — the state I branched on

| probe | expected | measured | ✅ |
|---|---|---|---|
| `git rev-parse HEAD` | `8910c17` | **`8910c174276d147b8f71ab65c8f5daf2fe6bf95e`** | ✅ |
| ahead / behind `origin/main` | 6 / 0 | **6 ahead / 0 behind** | ✅ |
| branch | `main` | **`main`** | ✅ |
| ⛔ Jonathan self-commit to avoid duplicating? | ⛔ none | **⛔ none** — HEAD did not already carry this work | ✅ |
| editor down? | DOWN | **DOWN** — `Get-Process UnrealEditor,UnrealEditor-Cmd,UnrealBuildTool` returned **nothing** | ✅ |

⛔ **The editor was ⛔ never opened.** `L_Arena` stays at `9ccd54ef…0e58`, never saved.
⭐ Dirty tree exactly matched the expected cargo — **6 modified, 2 untracked**, ⛔ no strays.

---

## 2. ✅ THE COMPILE — `Result: Succeeded`

```
Result: Succeeded
Total execution time: 20.61 seconds
```

⛔ **PARSED FROM THE LOG, ⛔ NOT FROM THE EXIT CODE** (the `Build.bat`-exit-code law). For the record the exit code was **`0`** — but it is `0` on a failed build too, so it carried ⛔ no weight here. The verdict is the `Result:` line at log line 42.

- **0 errors, 0 warnings** — a case-insensitive `error` census across the whole log returned **0**.
- ⛔ **Not a no-op.** 18 actions ran, and **all four edited translation units rebuilt by name**: `HeroCharacter.cpp` · `SiegeLadderClimbStatics.cpp` · `SiegeLadderClimbTest.cpp` · `SummonedUnit.cpp` — then `UnrealEditor-GitClaudeUnrealTest.lib` and `.dll` **relinked**.
- ⛔ **SAC did ⛔ not fire** — no `0x800711C7`, and the build ran 20.6 s (the SAC signature is a ~2 s failure).
- ⛔ **Live Coding did ⛔ not fire** — no mutex contention; the editor was down.

⇒ ⛔ **Nothing to append to `qa/TASK-804.md`; nothing routed back.**

---

## 3. ✅ THE SUITE — **`306`, and the anchor that MOVED is exact**

⛔ **Re-measured on disk, ⛔ never by adding the declared delta.**

| probe | result |
|---|---|
| `^IMPLEMENT_` across `Siegebound/Tests/*.cpp` | **306** |
| `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` | **306** ⇒ ⭐ one macro form only |
| `^IMPLEMENT_COMPLEX_*` | **0** ⇒ ⛔ no N-per-macro form to break the count |
| test files | **23** |
| `IMPLEMENT_` anywhere in `Source/` outside `Tests/` | **1** — `IMPLEMENT_PRIMARY_GAME_MODULE`, correctly outside the census |

**Anchors — all five, including the one that moved:**

| anchor | declared | on disk | ✅ |
|---|---|---|---|
| `SiegeHealthBarOcclusionTest` | 12 | **12** | ✅ |
| `SiegeHeroCameraTest` | 10 | **10** | ✅ |
| `SiegeClimbableTowerTest` | 14 | **14** | ✅ |
| `SiegeHeroLadderClimbTest` | 24 | **24** | ✅ |
| ⭐ `SiegeLadderClimbTest` | **17 → 22** | **22** | ✅ **TASK-803's `+5` landed exactly where it was declared** |

⇒ **declared `306` == actual `306`.** ⛔ The superseded `301` did ⛔ not appear anywhere.

⚠️ **One trivial correction to `qa/TASK-804.md` §9, ⛔ not a finding:** it cites `IMPLEMENT_PRIMARY_GAME_MODULE` at `GitClaudeUnrealTest.cpp:8`; it is at **`:6`**. Same fact, same exclusion, ⛔ no effect on the census.

⭐ **A note on scope, so nobody re-derives it in alarm:** a `*Test*.cpp` glob outside `Tests/` returns **4 files** — `GitClaudeUnrealTest{,Character,GameMode,PlayerController}.cpp`. They are ⛔ **project-name collisions** (the project is literally named `GitClaudeUnrealTest`), ⛔ not stray tests; none carries an `IMPLEMENT_` macro. QA's "no test file outside `Tests/`" stands.

---

## 4. ✅ THE ADDITIVE FENCE — re-confirmed independently on the diffstat

⛔ **I did not take this on QA's word; the diffstat proves it structurally:**

```
 SiegeLadderClimbStatics.cpp   |  57 ++            ← 0 deletions
 SiegeLadderClimbStatics.h     |  95 +++           ← 0 deletions
 HeroCharacter.cpp             |  73 +-
 SummonedUnit.cpp              |  63 +-
 Tests/SiegeLadderClimbTest.cpp| 732 +++++++++     ← 0 deletions
 TASKBOARD.md                  |   2 +-
 6 files changed, 1017 insertions(+), 5 deletions(-)
```

- ⭐ **`SiegeLadderClimbStatics.{h,cpp}` are PURE ADDITIONS — 0 deleted lines.** ⇒ `ClimbDirection` cannot have changed; the file it lives in has no deletions at all.
- ⭐ **5 total deletions − 1 in `TASKBOARD.md` = 4 source deletions**, ⛔ exactly the 4 the programmer declared (2 mandated swaps + 2 `ArrivalTarget` hoists).

### ⛔⛔ `Tools/ArtPipeline/build_watchtower.py` — NOT cargo, and NOT reverted

`LADDER_OUTWARD_SHIFT = 10.0` at `build_watchtower.py:137`, **verified on disk, unchanged**, and the file is ⛔ **not dirty** (`git status` on `Tools/` returned empty). ⇒ ⛔ **nothing to exclude, because nothing was touched.** Per `CONTACT-§14.4` the constant is **inert on `Y` (exactly `0.000 uu`) — ⛔ inert, ⛔ NOT "insufficient"** — and **measured-good on the standoff axis it was actually ruled for**. ⛔ **A revert would be a FAIL, not a cleanup.**

---

## 5. ⛔⛔ THE PIE ROWS — ⛔ OWED, ⛔ NOT OBSERVED, ⛔ NOT SIMULATED

⛔ **No PIE ran this pass.** The editor was **DOWN** by the orchestrator's own hand and I was ⛔ instructed not to open it. ⛔ **Independently of that, every row below needs a HUMAN AT THE KEYBOARD** — the pawn must be *walked in deliberately off-line*. The editor's remote interface can start and stop a play session but has ⛔ **no key, axis or pawn-input tool in any of its 19 toolsets.**

⇒ ⛔ **Nothing below is observed. Nothing below is waived. A green `306` is ⛔ NOT a substitute for any of it.**

### ⭐⭐ STEP 1 OF THE OWED SESSION — RUN THIS ⛔ BEFORE ANYTHING ELSE

```
Log LogGitClaudeUnrealTest Verbose
```

*(or launch with `-LogCmds="LogGitClaudeUnrealTest Verbose"`)*

⛔⛔ **WITHOUT IT THE ENTIRE INSTRUMENT PRINTS NOTHING.** `GitClaudeUnrealTest.h:8` declares `DECLARE_LOG_CATEGORY_EXTERN(LogGitClaudeUnrealTest, Log, All)` — the compile-time `All` means the lines **survive the build**, but the **default runtime verbosity is `Log`**, so every `Verbose` line is **suppressed at runtime**. I re-checked `Config/` myself: **zero** matches for `LogGitClaudeUnrealTest` and **zero** for `[Core.Log]`. ⇒ nothing in the repo raises it.

⛔⛔ **AND THE TRAP IS THE FAILURE MODE, NOT THE MISSING LINE: AN EMPTY LOG READS AS A ZERO OFFSET — i.e. A ⛔ FALSE PASS.** ⭐ **A SILENT LOG IS ⛔ NOT EVIDENCE OF A CENTRED CLIMB.** It is evidence of nothing at all.

**Canary — confirm this ⛔ known-good Verbose line appears before trusting ⛔ any absence:**
`HeroCharacter.cpp:1851` → `"AHeroCharacter '%s': ladder climb STARTED — …"`. If the canary is silent, the category is still suppressed and ⛔ **every** reading in the session is void.

⚠️ Both new instrument lines are `Verbose` — **entry offset *and* arrival pop**. There is no `Log`-level fallback for either.

### The rows owed for `TASK-805` (the reason this task existed)

- **(a) ⭐ `Y` AT ENTRY — the whole measurement.** Read the Verbose **ENTRY OFFSET** line per climb; compare against **±24.0** (hero, `r 42`) and **±32.0** (unit, `r 34`). Line prints signed cross-track, stiles at ±66.0.
- **(b) ⭐ THE ARRIVAL POP** (`CONTACT-§14.3`) — read the **ARRIVAL POP** line (`horizontal` = the residual cross-track). ⛔ **Never looked for before.** Simulated expectation ≈ **0.5 uu**; ⛔ report the number either way.
- **(c) ⛔ OVER ~6 APPROACHES** at different side-offsets **and speeds** — ⛔⛔ **the finding is a DISTRIBUTION, ⛔ not one number.** ⭐ Walk in deliberately off-line; that is the point.
- **(d) ✅ THE CONVERGENCE, ON PIXELS** — the pawn must **visibly ride between the stiles** by mid-climb, ⛔ **and the climb must still ARRIVE.** A cross-track term that breaks arrival is ⛔ worse than the defect.

### ⛔ CARRIED FORWARD FROM `TASK-802` — still owed, still unpaid

1. **`B-1` regression** — a dead unit's bar stays hidden (⛔ no 0-HP bar resurrected over a corpse).
2. **`B-2` regression** — a camera buried in geometry is detected on the hit.
3. **`V2` pixel row** (hero camera).
4. **`V3` pixel row** (health bars).
5. **The turn-on-the-spot row.**

⛔ **`TASK-798` and `TASK-806` are ⛔ JONATHAN'S — ⛔ I may not close either, and this pass closed neither.**

---

## 6. ⭐ QA's OTHER THREE WARNS — carried, ⛔ not dropped

- **`W-2` — the withdrawn "Y at 3–4 points" row ⛔ STAYS WITHDRAWN, but ⛔ its REASON EXPIRES WITH THIS FIX.** The proof that all four samples return an identical number was a proof about the **pre-fix** driver — its whole content was *that there was no cross-track term*. ⭐ **Post-fix those samples would trace the CONVERGENCE CURVE instead of repeating one number.** The withdrawal survives on a **different** ground: the shipped two-point instrument captures both **ends** of that curve. ⇒ ⛔⛔ **the pixel row (d) is now LOAD-BEARING, ⛔ not decorative, and ⭐ EVERY ENTRY OFFSET MUST BE PAIRED WITH ITS OWN ARRIVAL POP, PER CLIMB.** ⛔ The arrival-pop number is ⛔ **not** evidence about *where* convergence completed — that is the pixel row's job.
- **`W-3` — the convergence numbers are the ⛔ STEERING LAW's, ⛔ not the movement component's.** The swept call goes through `AddMovementInput` → acceleration → `MaxFlySpeed`; the commanded vector reaches **0.88 lateral** on the first frames of a worst-case entry, so along-track climb rate drops up to **~52.5 %** there and real convergence **LAGS** the derived bound (watchdog headroom covers it: +6.6 % path cost against a 300 % allowance). ⇒ ⛔ **A green `306` does ⛔ NOT prove a pawn converges in PIE.** ⭐ Expect the PIE residual to **lag** the simulated `~0.5 uu`: **a few uu is a PASS; tens is a finding.**
- **`W-4` — READER 2 (the deck-breach step, `HeroCharacter.cpp:2119` · `SummonedUnit.cpp:4078`) is ⛔ UNCOVERED BY ALL 306 TESTS, ⛔ PERMANENTLY.** Structurally protected by `const` + the branch `return`, and verified by eyeball — but **a diff read is not a test**: any future edit changes arrival/lift semantics with **no compile error and no test failure**. The `static_assert`s pin the *signature*, ⛔ not the *call site*. ⇒ ⛔ **Every future ladder task must re-run this eyeball**; `19(e)` is the row that says so. ⛔ Not fixable headlessly — neither driver instantiates without a `UWorld`.

---

## 7. ⭐⭐ THE FINDING WORTH RECORDING — TWO BUGS WERE ONE

**"Climbs off-centre" and "gets grabbed walking past" were ⛔ ONE defect, ⛔ not two rows.**

The trigger admits from a **350 uu disc** while the driver had **no cross-track term** — so a pawn admitted off-line **stayed off-line**, all the way up. At unit speed that is **±247.2 uu** against a **24 uu** margin, and it was **invisible** because the ladder has **zero collision**: nothing ever pushed the pawn back onto the line, and nothing ever complained. One admission geometry, one missing term, two symptoms.

Fixed **additively**, with a closed-loop steer — ⛔ no existing behaviour deleted.

⭐ **And the consequence worth recording: the one-frame arrival pop falls `277.8 → ~0.5 uu` ⛔ WITHOUT touching the unswept snap.** The snap is untouched and still unswept — ⛔ **the error is simply GONE BY ARRIVAL.** That is a strictly better outcome than making the snap safe, because there is no longer anything for it to hide.

---

## 8. ✅ THE COMMIT — explicit paths only, ⛔ never `git add -A`, ⛔ NOT pushed

**Code cargo (5):** `SiegeLadderClimbStatics.h` · `SiegeLadderClimbStatics.cpp` · `HeroCharacter.cpp` · `SummonedUnit.cpp` · `Tests/SiegeLadderClimbTest.cpp`

⚠️ **`HeroCharacter.h` and `SummonedUnit.h` were named as cargo but are ⛔ NOT DIRTY** — TASK-803 needed no header change in either (the new statics live in `SiegeLadderClimbStatics.h`). ⛔ Nothing was staged that had not changed.

**Pipeline record:** `TASKBOARD.md` (status flips for `803`/`804`/`805` — ⛔ nothing else) · `handoffs/TASK-803-programmer.md` · `handoffs/TASK-805-buildmaster.md` · `qa/TASK-804.md`

⛔ **`Tools/ArtPipeline/build_watchtower.py`: ⛔ NOT staged, ⛔ NOT reverted, ⛔ NOT opened.**
⛔ **No push.** `main` sits **7 ahead / 0 behind** `origin/main` after this commit.
