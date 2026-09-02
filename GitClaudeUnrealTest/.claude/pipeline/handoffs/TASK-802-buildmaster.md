# TASK-802 — [GATE-2] build-master: ONE compile · the suite · ONE commit

**Outcome: ✅ compile GREEN · ✅ suite `301/301` (0 fail) · ✅ ONE commit on `main` · ⛔ NOT pushed · ⛔⛔ THE PIE HALF IS ⛔ NOT DONE AND IS ⛔ OWED TO JONATHAN (§5).**

Input gate: `qa/TASK-801.md` = **PASS, 0 blockers**, 2 declared-and-accepted WARNs, 1 deferred NIT, after one qa-loop that caught two real blockers.

---

## 1. ⭐ MEASURED FIRST — git state, before anything was touched

| probe | measured | expected | |
|---|---|---|---|
| `git rev-parse HEAD` | `c6ee4a7d8d0d0a25a079f6b23eb0f832dd6b1da4` | `c6ee4a7` | ✅ |
| `git rev-list --left-right --count origin/main...HEAD` | `0	5` ⇒ **5 ahead / 0 behind** | 5 ahead / 0 behind | ✅ |
| branch | `main` | `main` | ✅ |
| ⛔ Jonathan self-commit to avoid duplicating? | ⛔ **none** — HEAD did not already carry this work | — | ✅ |

⚠️ **The unpushed debt is now 6 ahead / 0 behind.** ⛔ Still not pushed — no one asked.

⛔ **`Tools/ArtPipeline/build_watchtower.py` was ⛔ NOT in the dirty set at any point.** `LADDER_OUTWARD_SHIFT = 10.0` is intact at `:137`, sockets at `:146-147` unchanged. ⛔ **Not staged, ⛔ not reverted, ⛔ not opened** (`CONTACT-§14.4`: correct on the standoff axis, exactly `0.000 uu` on Y — a revert would have been a FAIL, not a cleanup).

---

## 2. ✅ THE COMPILE — `Result: Succeeded`, parsed from the log

⛔ **The exit code was ⛔ not trusted** (standing Build.bat law: it returns `0` on a FAILED build). The log was parsed.

```
Result: Succeeded
```

- `grep -n "Result:"` over the full log returns **exactly one line**, line 44: **`Result: Succeeded`**.
- `error C` / `error LNK` / `Result: Failed` census: ⭐ **0**. Warning census: ⭐ **0**.
- 19 actions, 24.50 s total. `[Adaptive Build] Excluded from unity file: CombatantHealthBarComponent.cpp, HeroCharacter.cpp, SummonedUnit.cpp, SiegeHealthBarOcclusionTest.cpp, SiegeHeroCameraTest.cpp` ⇒ ⭐ **all five changed translation units compiled as their own TUs, ⛔ not hidden inside a unity blob.**
- ⛔ **Not SAC** (`0x800711C7` in ~2 s did not occur — it linked and wrote `UnrealEditor-GitClaudeUnrealTest.dll`). ⛔ **Not Live Coding** (no mutex; the editor was down).

⚠️ **This was the FIRST compile for BOTH diffs** — same UBT module, one compile, and nothing in either diff had ever been built. It came back clean on the first attempt with zero warnings.

---

## 3. ✅ THE SUITE — **`301 / 301`, 0 fail. Declared == actual.**

**Static census on disk** (`^IMPLEMENT_*_AUTOMATION_TEST` at column 0, across `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`): **301 across 23 files.** ⭐ The `_SIMPLE_` needle and the wider `_[A-Z_]*_` needle return the **same 301** ⇒ ⛔ no complex/custom macros exist.

**Headless run** — ⛔ no GUI, ⛔ no PIE, ⛔ no map load:
`-ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended -nopause -nosplash -NoLiveCoding`

| | count |
|---|---|
| `Result={Success}` | ⭐ **301** |
| `Result={Fail}` | ⭐ **0** |

**The arithmetic the gate demanded, reproduced:** `301 − 10 − 12 = 279` = the `qa/TASK-779.md` baseline **exactly** ✅

**All five anchors — census on disk AND executed count in the log, both:**

| suite | census | executed | |
|---|---|---|---|
| `SiegeHealthBarOcclusionTest` | 12 | **12** | ✅ (was 8; the qa-loop grew it `+4`) |
| `SiegeHeroCameraTest` | 10 | **10** | ✅ |
| `SiegeLadderClimbTest` | 17 | **17** | ✅ anchor |
| `SiegeClimbableTowerTest` | 14 | **14** | ✅ anchor |
| `SiegeHeroLadderClimbTest` | 24 | **24** | ✅ anchor |

⛔ **297 was the superseded round-0 number and was ⛔ never asserted.** The gate's `301` is what was measured, two independent ways.

---

## 4. ⭐⭐ THE TWO FINDINGS THIS BATCH RESTS ON — both are CALL-GRAPH defects neither author could see from inside their own fence

⚖️ Recorded here because the diffs read as correct in isolation and the record should say why they were not.

### `B-1` — a unit's death wrote bar visibility **RAW** instead of through the latch
`ASummonedUnit::HandleDeath` called `HPBarWidget->SetVisibility(false)` directly, so the owner-intent latch `bBarShownByOwner` stayed **`true`**. The occlusion poll `TASK-791` had just added would therefore recompute "visible" ≈150 ms later and **pop a 0-HP bar back onto every corpse for the whole death-anim hold (up to ~2 s)** — ⭐ **the exact regression the fix's own reasoning claimed to prevent.** Repaired at the one granted line, `SummonedUnit.cpp:4259` → `HPBarWidget->HideBar();`, carrying a six-line comment explaining why the two calls were equivalent *before* `TASK-791` and are not now. ⭐ **The comment is the deliverable, not the line.**
⭐ The census that closed it: `AHeroCharacter` has a **real** corpse window (hidden, not destroyed) and already used `HideBar()` ⇒ the latch was never a theory, it was already carrying a live case; `ABuilding` never writes its bar's visibility at all and `Destroy()`s at `Building.cpp:370` ⇒ no corpse window. `ASummonedUnit` was the **one** owner that had opted out.

### `B-2` — the guard against "trace starts inside geometry" was **INERT for raycasts**
`bFindInitialOverlaps = false` is **sweep-only**: its sole engine consumer sits behind `if (!bIsSweep) { return Block; }` (`CollisionQueryFilterCallback.cpp:211-220`), and `SceneQuery.cpp:522` sets `bIsSweep == false` for a ray ⇒ **the engine returns a blocking hit before ever reading the flag.** With `TASK-790` parking the camera up to **150 uu inside the tower by design** at every ladder approach, every unit's ray would start inside the same hull ⇒ ⭐ **the entire roster's bars would have blinked off together**, at the exact moment and place the pair was fixing. Repaired by detecting the burial **on the hit** via `bStartPenetrating` — which **is** populated on the ray path (`CollisionConversions.cpp:366`, on the shared path; `:552-553` explicitly instantiates `ConvertTraceResults<FHitRaycast>`) — through a new third pure static `ComputeOcclusionFromTraceResult`.
⭐ **The shipped instrument is query-kind agnostic, which the flag was not** — that asymmetry is precisely what made `B-2` a shipped defect rather than a design choice.

---

## 5. ⛔⛔ THE PIE ROWS — **⛔ NOT RUN, ⛔ NOT WAIVED, ⛔ NOT SIMULATED. ALL FIVE ARE OWED TO JONATHAN.**

### ⛔ WHY THEY WERE NOT RUN — and why this is ⛔ not the "compile is clean, skip it" waiver `SC-§35` forbids

1. **The editor was DOWN by the orchestrator's own hand** and verified so (`L_Arena` at `9ccd54ef…0e58`, never saved this session). The dispatch fenced it: ⛔ do not open it.
2. ⭐⭐ **AND THE DECISIVE REASON, WHICH WOULD STAND EVEN IF IT WERE UP: ⛔ all five rows require a HUMAN AT THE KEYBOARD** — walking to a ladder, turning on the spot, killing a unit. **The editor's remote interface can start and stop a play session but has ⛔ NO key, ⛔ no axis and ⛔ no pawn-input tool in ⛔ any of its 19 toolsets.** ⇒ ⛔ **there is no honest way to drive a pawn from here.**
3. ⇒ ⛔ **A property readback is ⛔ not an answer to a pixel row (`AS-§6 A(e)`), and a simulated one is worse than a deferred one.**

⛔ **`SC-§35` is honoured by RECORDING these as unobserved, ⛔ not by claiming them.** The law exists because 1,806 Blueprint runtime errors once compiled clean and cleared two QA gates.

### ⭐ THE FIVE ROWS, VERBATIM

1. **`V2` pixels** — walk the hero to the ladder foot and **watch**. Is the world visible through the ≈2 s approach? **Is the 150 uu near-clip into stone acceptable, or does Jonathan want the number moved?** 🧑 ⭐ **HIS CALL, ⛔ NOT OURS.** *(original acceptance)*
2. **`W-1`** — turn on the spot at the ladder foot; look for a **snap** as the traced blocker flips tower→ground. *(original acceptance; up to 150 uu of instantaneous camera translation is possible by construction — a feel row, ⛔ not a correctness bug)*
3. ⭐⭐ **`B-2` — DECISIVE, and a ⛔ REGRESSION CHECK** — with units on the field, walk in until the camera buries in the tower, and watch **every other unit's health bar**. ✅ **They must STAY UP.** ⛔ If the roster's bars vanish together, the zero-distance fail-open is not working in-engine and the engine-source reasoning was wrong.
4. ⭐ **`B-1` — a ⛔ REGRESSION CHECK** — kill one unit in view and watch its bar for ~2 s. ✅ **It must STAY GONE.** ⛔ A 0-HP bar reappearing over the corpse means the latch is still being bypassed somewhere the source probe cannot see.
5. **`V3` pixels** — from the watchtower deck with units on the ground below, confirm **VID-004 @ 02:08's four floating bars are gone** (defect frame `f00079_00s`: four bars over an empty deck — reproduce **that shot**, not an approximation). **Then confirm bars for VISIBLE pawns are still drawn** — ⛔ a cull that hides everything is worse than the defect.

⚖️ **Rows 3 and 4 are regression checks on repairs this gate's qa-loop FORCED. Rows 1, 2 and 5 are the original acceptance.** ⛔ **None of the five is observed.**

### ⛔ ALSO NOT RUN, and correctly so
- **`TASK-800`'s old deferred row** — ⛔ **WITHDRAWN** by the board (`CONTACT-§14.1` proves all four samples return the identical number ⇒ 4 samples, 1 bit). Its replacement needs instrumentation **that does not exist yet**, and writing it is **code** ⇒ ⛔ not mine. Boarded: `TASK-803` writes it, `TASK-805` runs it. ✅ Correctly left alone.
- **The `VIS-§4a` CONTACT/nav-link log census** (spec (3)'s "free one") — ⛔ **needs a live editor and a placed Watch Tower.** Zero code, but ⛔ not zero editor. ⇒ **it rides the same owed PIE session.**
- **Spec (4)'s same-path reimport** — ⛔ **`TASK-793` did NOT land** (no `SM_WatchTower` / `T_WatchTower_{D,N,ORM}` cargo in the tree, measured). ⇒ ⛔ **not swept into this commit; `793` needs its own later integration**, and `CONTACT-§13` still binds it (mesh + atlas import **together or neither**).
- **`TASK-796`/`797`/`798`** — ⛔ Jonathan's, ⛔ out of scope, ⛔ not closed from here.

---

## 6. ⛔⛔ THE LIMIT THAT RIDES THE COMMIT MESSAGE

> ⛔ **A green 301 does ⛔ NOT mean `V2` and `V3` are fixed.**

That a real camera in a real tower produces a real zero-distance hit is ⛔ **not testable headlessly.** It rests on **engine-source reasoning — derived independently by the programmer and by QA, agreeing at five line-exact points** — and it is **owed a PIE row**.

⛔ **`W-5` STANDS, UNCHANGED AND PERMANENT: the runtime tick and the visibility write are uncovered by ⛔ all 301 tests.** The 22 rows across the two new files prove *arithmetic*, *configuration*, *structure*, *fences* and *rules*. They do ⛔ **not** prove that a camera behaves, that a bar hides, or that a corpse stays quiet.

⭐ **A green suite here means only that nothing checkable headlessly is wrong.** The pixels are the verdict (`SC-§35`, `AS-§6 A(e)`).

---

## 7. ✅ THE COMMIT — explicit paths only, ⛔ never `git add -A`, ⛔ NOT pushed

**Code cargo (7):** `HeroCharacter.{h,cpp}` · `CombatantHealthBarComponent.{h,cpp}` · `SummonedUnit.cpp` (⭐ **the one granted line at `:4259` + its comment — diffstat `8 insertions, 1 deletion`, verified line-by-line**) · `Tests/SiegeHeroCameraTest.cpp` · `Tests/SiegeHealthBarOcclusionTest.cpp`

**Pipeline record:** `TASKBOARD.md` (+401, ⭐ additions only) · `CONVENTIONS.md` (+237, ⭐ additions only — the `VIS-§` law + `CONTACT-§13`/`§14`) · `handoffs/TASK-790,791,799,800-programmer.md` · `handoffs/TASK-802-buildmaster.md` · `qa/TASK-801.md`

**Evidence:** `footage/VID-004-contact-climb-confirmed-visual-punchlist.md` + 8 PNGs under `playtest-evidence/2026-09-02/`.
⭐ **Included on precedent + law, ⛔ not on a guess:** `VID-001..003` reports and 22 evidence PNGs are already tracked, and `CLAUDE.md` states promoted evidence PNGs under `.claude/pipeline/playtest-evidence/` **do** enter git. PNGs ride **Git LFS** (`*.png filter=lfs`). ⛔ `testvideo/` was ⛔ never staged.

⛔ **`Tools/ArtPipeline/build_watchtower.py`: ⛔ NOT staged, ⛔ NOT reverted, ⛔ NOT opened.**
