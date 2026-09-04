# TASK-814 — build-master handoff (Lane B commit gate)

**Date:** 2026-09-03 · **Agent:** build-master · **Result:** ✅ **COMMITTED — `d287102`** (8 paths, after a ruled subtraction)
**Law:** `TL-§5c` (executed suite) · `TL-§5b` · `QUIET-MODULE` · Build.bat-exit-code law · `§25b` · `VIS-§4` · `SC-§39` / `SC-§40`

---

## 0. ⭐ HEADLINE

**Commit `d287102` on `main`, 8 files, 6369 insertions / 17 deletions. NOT pushed** — `main` is now **8 ahead** of `origin/main`.

This row ran in **two passes**, and the record keeps both because the first is the reason the second is safe.

1. ⛔ **I stopped before committing** and escalated: the **ruled 9-path set could not compile at `HEAD`**.
2. ✅ **The manager ruled in favour of the measurement** — `Tests/SiegeAcquisitionFunnelTest.cpp` was **dropped from item (5)** and re-homed onto **`TASK-835`**, beside the `SiegeCombatStatics.{h,cpp}` unit that `qa/TASK-848.md` gates it with. I then committed **the eight**.

⭐⭐ **THE SUBTRACTION WAS THE BOARD'S, NOT MINE.** I measured the break, refused to make the edit myself, and reported. That refusal is recorded by the manager as the correct call, and the dispatch's own `H1` — *"it must be `git add`-ed or this commit ships the code WITHOUT its 9 tests"* — is **recorded as WRONG in the manager's words**: those tests cover code that is **not in this commit at all**, so adding the file protected no Lane B coverage and only broke the build.

---

## 1. ⭐ THE EXECUTED SUITE — `TL-§5c` DISCHARGED. I AM THE FIRST EXECUTION.

Every gate on this batch (`816`, `862`, `869`, `878`) declared its numbers with **no shell**. These are measured.

### Compile — the `Result:` line, verbatim, parsed in BOTH directions

```
Result: Succeeded
```

⛔ **The exit code was `0` and I gave it no weight** (Build.bat returns 0 on failure). Exactly **one** `Result:` line exists in the log; `Result: Failed` appears **0** times; `error`/`failed` appear **0** times. 46/46 actions, 30.81 s.

⭐ **The compile covered the one thing `qa/TASK-869.md` §7(3) said it could not check:** `SiegeAcquisitionFunnelTest.cpp` (`+292` lines, new fixture helper `CountOccurrencesIncludingComments`) **compiled clean** at `[18/46]` — so the file is *sound*, it is simply not committable **here**. Also clean: `SiegeBuildingStackTest.cpp` `[22/46]`, `SiegeCastBarTest.cpp` `[23/46]`, `SiegeAssistantSelectionTest.cpp` `[27/46]`, `SiegeLadderClimbTest.cpp` `[35/46]`, `Building.cpp` `[6/46]`, `ClimbableTower.cpp` `[2/46]`.

### The suite — the `Result={}` pair, verbatim

Command: `-ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended`

| count | value |
|---|---|
| `Result={Success}` | ⭐ **426** |
| `Result={Fail}` | ⭐ **0** |

⛔ **`426 / 426`, zero failures.** `grep -o "Result={[A-Za-z]*}"` returns **exactly one variant across the whole log — `426 Result={Success}`**. There is no third state hiding.

**`SC-§39` positive control — the instrument is PROVEN READABLE, not merely silent.** The log carries **426** `Result={...}` lines and my independently-taken census is **also 426**. Two different tools agreeing on the same number rules out an unreadable instrument. ⇒ the `Result={Fail}` reading of `0` is a **measured zero, not an empty log**.

### ⭐⭐ The NAMED ladder row — acceptance for TWO tasks (item 2-pre-c)

```
Test Completed. Result={Success} Name={ClimbDirectionIsByteIdenticalAndItsThreeOtherReadersStillReadTheLine}
Path={Siegebound.LadderClimb.ClimbDirectionIsByteIdenticalAndItsThreeOtherReadersStillReadTheLine}
```

⭐ Reported as the **named row**, not as a total — a total can hide a swap. This **closes `TASK-855`** and is the **first and only execution `TASK-853`'s ladder fix has ever had**. `TASK-853` is accepted on this line, and the file ships in this commit.

### Census (`TL-§5b`) — my own, and it RECONCILES

Pattern `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` scoped to `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`:

> ⭐ **426 across 31 files**

| term | tests | files |
|---|---|---|
| `HEAD` (`bf0cd9e`) | 306 | 23 |
| 8 new untracked files (funnel 9 · stack 10 · keylabel 5 · castbar 15 · fogclamp 8 · fog 9 · invis 32 · placement 28) | +116 | +8 |
| `SiegeControlsHelpTest.cpp` 13→16 | +3 | — |
| `SiegeAssistantSelectionTest.cpp` 38→39 | +1 | — |
| **total** | ⭐ **426** | ⭐ **31** |

⚠️ **The `+1` over `qa/TASK-862.md`'s declared `425/31` is accounted for, not narrated away:** it is `TASK-874`'s single added test. `qa/TASK-869.md`'s `410/30` + castbar `15/1` = `425/31`, + `874`'s `1` = **426**. All three declared figures reconcile to my executed one.

⛔ **The `^IMPLEMENT_` trap was checked and does NOT fire on this tree today:** bare and scoped both return `426/31`. (QA reproduced `393/30` vs `392/29` on an earlier tree; that divergence is gone. **Scoped is still the pattern I used and the one that must stay in the law** — the trap's absence today is a property of this tree, not a repeal.)

---

## 2. ⛔⛔ THE STOP THAT PRODUCED THE RULING — measured, not reasoned

`Tests/SiegeAcquisitionFunnelTest.cpp` makes these calls with explicit `FSiegeCombatStatics::` qualification:

| call site | symbol | occurrences in **`HEAD`**'s `SiegeCombatStatics.h` |
|---|---|---|
| `:437 :441 :446 :451 :461` | `FSiegeCombatStatics::IsHostileTeam(` | ⛔ **0** |
| `:496` | `FSiegeCombatStatics::GatherHostileAgents(` | ⛔ **0** |
| `:504` | `FSiegeCombatStatics::GatherFriendlyAgents(` | ⛔ **0** |
| (referenced) | `GatherTeamAgentsFiltered` · `SeeingFrom` · `ReadFogState` | ⛔ **0** each |

These are **real executed C++ calls**, not the token-counting strings test 9 uses. `HEAD` **has** the file — it does not have this API. Worktree counts: 2/7/2/1/4/1.

**The closure that made "just add the one file" impossible:**

```
Tests/SiegeAcquisitionFunnelTest.cpp
  └─ SiegeCombatStatics.{h,cpp}        ⛔ modified, another lane's
       ├─ SiegeInvisibilityStatics.h   ⛔ UNTRACKED  (TASK-827 → TASK-835)
       ├─ SiegeFogStatics.h            ⛔ UNTRACKED  (TASK-837 → TASK-843)
       └─ SummonedUnit.{h,cpp}         ⛔ modified (+1243)
```

⭐⭐ **The file was never Lane B's.** `qa/TASK-848.md` gates `TASK-828` as **one unit**: *"the acquisition funnel: `SiegeCombatStatics.{h,cpp}` + the 9 routed sites + `Tests/SiegeAcquisitionFunnelTest.cpp`"*. It rode onto `TASK-814` only because `TASK-868` fixed a red row **inside** it that was blocking **my** suite — the row was fixed, but **the file's home lane travelled with it and nobody re-checked the dependency.**

⭐ **The general lesson, worth keeping:** *a test file follows the code it guards, not the task that repaired it.* The repair (`868`) and the gate (`848`) were never the same row, and that gap is what put an uncompilable file into a ruled pathspec.

### ✅ Why the remaining 8 were provably safe

Checked every new declaration in every excluded header (`HealthBarProvider.h`, `CombatantHealthBarComponent.h`, `CombatantHealthBarWidget.h`, `SummonedUnit.h`, `SiegeControlsHelpWidget.h`, `CardHandWidget.h`, `DeckComponent.h`, `Tower.h`) against the 8:

- ⭐ **`SiegeControlsHelpWidget.h` gained ZERO new declarations** — `TASK-823`'s rows are `.cpp`-only. ⇒ `SiegePlayerController.cpp`'s include of it is safe and **`4e` was never triggered. I never needed `811`'s file.**
- **`CardHandWidget`'s `GetSlotKeyLabel` / `ComposeSlotKeyLabel`: 0 references** in the 8 ⇒ **the "you MAY adopt `CardHandWidget`" permission was neither needed nor used.**
- `Building.cpp` uses only `UCombatantHealthBarComponent` (present at `HEAD`). Building does **not** override the new `GetCastProgressPercent`/`IsCastInProgress` — non-pure defaults.
- `CanEverAttack()` (used by `SiegeLadderClimbTest.cpp`) **exists at `HEAD`** (7 hits) — its *body* changed, its declaration did not.
- `MinerCardID`, `PositionRadius` are declared in `SiegePlayerController.h` / `UnitCommand.h`, present at `HEAD`. `OutRadius` is a parameter of Lane B's **own** new `TryGetPlacementFootprintRadius`, shipping in this commit.

---

## 3. ✅ THE COMMIT — `d287102`

**Final pathspec, exactly as executed (8 paths, explicit, from the repo root):**

```
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Building.h
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Building.cpp
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeBuildingStackTest.cpp
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegePlacementTest.cpp
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeLadderClimbTest.cpp
```

**Differences from the item (5) `names:` line — both ruled by the manager, neither chosen by me:**

| path | disposition |
|---|---|
| `Tests/SiegeAcquisitionFunnelTest.cpp` | ⛔ **REMOVED** → re-homed to `TASK-835` (the compile break) |
| `Tests/SiegeAssistantSelectionTest.cpp` | ⛔ **NOT ADDED** → `TASK-874`'s **ungated** diff; own host `TASK-906` behind `TASK-889` |
| `Tests/SiegeCardHandKeyLabelTest.cpp` | ⛔ **NOT ADDED** → went to `TASK-811` (which takes `CardHandWidget` whole) |
| `CombatantHealthBar*` / `HealthBarProvider.h` | ⛔ **NOT ADDED** → `TASK-835`'s |

⭐ **`H1` verified against `git status`, never the `names:` line — and it was load-bearing.** Of the 8, **two were `??` untracked** (`SiegeBuildingStackTest.cpp`, `SiegePlacementTest.cpp`) and would have been silently missed by any `commit -a`; **none of the 8 was staged.** A bare index-wide commit would have shipped **zero Lane B code and seven art assets.**

**Verified after the fact:** the commit contains **exactly 8 files** and **0 `Content/` paths**.

### What the commit message says, and what it deliberately does NOT

⛔⛔ **THREE OF THE SEVEN SUBJECTS I WAS ASKED TO "COVER HONESTLY" ARE NOT IN THIS COMMIT, AND I MEASURED THAT BEFORE WRITING THE MESSAGE RATHER THAN AFTER.** A grep of the 8 files returns **zero** hits for the cast-bar data path, **zero** for `TASK-823`'s help rows, **zero** for the funnel guard. **Writing them in would have been the exact defect `HELP-§2` exists to prevent — a shipped record teaching something false — committed by the row that knew better.** The message names each omission and where it actually lands.

**In the commit:** tower stacking (`812`) · the Watch Tower exclusion · the third ghost state (`813`) · the placement footprint wheel (`815`) · the scaled-bounds footprint work (`735`) · the ladder test fix (`853`/`855`) · **and Lane A's discard-all C++ (`819`/`820`) riding inside `SiegePlayerController.{h,cpp}`, gated by `TASK-822` PASS** — a commit takes whole files, and that half was verified purely additive.

**Named as absent:** the help rows (`TASK-811`'s, with the asset that makes them true) · the funnel guard (`TASK-835`'s) · the cast bar's **data path** (`TASK-835`'s).

⚠️ **On the cast bar — the second reason the wording matters.** Jonathan has since **ruled the widget half out**: *"I do not think it is neccesary, just add an animation that the witch does during the casting time."* `TASK-900` is boarded to remove the cast-row geometry. ⇒ **`TASK-860`'s data path is reviewed, inert, and has no consumer and will never get one.** It is not in this commit, but the next reader of `CombatantHealthBarComponent` should know the code is **deliberately orphaned pending `TASK-900`**, not unfinished.

---

## 4. ⚠️⚠️ `§25b` AUTO-STAGE — FIRE #5 IS **7 ASSETS, NOT 2** — AND ALL SEVEN ARE UNTOUCHED

The dispatch named two. **The index actually holds seven**, staged by the editor's SCC, staged by no task:

```
A  Content/Input/Actions/IA_DiscardAll.uasset          (Lane A — TASK-811's)
A  Content/Materials/Instances/MI_Witch_PBR.uasset     (Lane W — TASK-835's)
A  Content/Meshes/SM_Witch.uasset                      (Lane W — TASK-835's)
A  Content/Textures/T_Witch_D.uasset                   (Lane W — TASK-835's)
A  Content/Textures/T_Witch_N.uasset                   (Lane W — TASK-835's)
A  Content/Textures/T_Witch_ORM.uasset                 (Lane W — TASK-835's)
A  Content/UI/CardArt/T_CardArt_Witch.uasset           (Lane W — TASK-835's)
```

✅ **VERIFIED AFTER THE COMMIT: all seven are still `A`, byte-for-byte the same index entries. Not one was committed, unstaged, or altered.**

⛔⛔ **I DID NOT UNSTAGE THEM — a considered refusal, since confirmed by the manager and written into the board as item (4a):**
**(a)** I commit by **explicit pathspec**, so nothing can be swept in — the hazard was already neutralised; **(b)** `git reset` would have **destroyed the index oid that `TASK-835` still owes a `§25b` digest comparison against**, and the board states plainly that **nobody has ever run it** (the artist had no Git, the manager has no shell). ⭐ **Unstaging is cheap to redo; a destroyed oid is not recoverable.**

⇒ **`TASK-835` claims all seven explicitly and runs the oid-vs-worktree-sha256 check. They remain UNMEASURED, which is not the same as clean (`SC-§40` cl. 1).**

---

## 5. ✅ THE HAND-FORWARDS QA HAD NO GIT FOR

**`H2` / `H3` — `TASK-868` is EXACTLY ONE FILE. ✅ CONFIRMED.** Its subject is untracked (so absent from `git diff --stat` by construction); `SummonedUnit.{h,cpp}`'s `+1243` belongs to `TASK-829`/`830`. **No production file is attributable to `868`.**

**`H3` — `TASK-860`'s *"zero lines of shipping source changed"*. ⚠️ TRUE BUT NARROWLY SCOPED.** `CombatantHealthBarComponent.{h,cpp}` + `CombatantHealthBarWidget.h` + `HealthBarProvider.h` carry **+543 lines** (`+200 / +241 / +40 / +62`). Per the board these are `TASK-860`'s **lane** files from an earlier network-killed sitting; the claim covers **that sitting only**. **They are `TASK-835`'s to commit** — and `TASK-835` item (2d)(iii) already owes this exact `git diff --stat`.

**W-5(b) — `TASK-819`'s discard-all block untouched by Lane B. ✅ CLEAN.** `git diff -U0` yields **0 deletion lines matching `Discard`**; Lane A's block is **purely additive**. The file's only 7 deletions are Lane B's own (`SpawnTransform`, the `(6)` clearance comment + gate, the 2-state ghost colour line, `HasBuildingClearance`'s signature, `ClearanceSq`) — legitimately superseded by `735`/`813`/`815`.

**W-5(c) — no shipped gate line re-indented in `UpdatePlacementGhost`. ✅ CONFIRMED.** `bPlacementValid = bValid;` sits at **`:2700`** and `TASK-813`'s block opens on the **next line**, purely additive.

**`TASK-855` item (3a) — zero-diff re-verify. ✅ CONFIRMED.** `git diff --numstat -- SiegeLadderClimbStatics.*` returns **empty**; `git status` returns **empty**. **Byte-unchanged and clean** — `TASK-776`'s byte-identical move is intact.

**Pre-commit staleness re-check. ✅ CONFIRMED MYSELF, not taken on trust.** `find Source -newer build-814.log` and the same over `Content/` both returned **empty** ⇒ **no source or asset file moved between the compile and the commit**, so the `426/0` describes the bytes that were committed.

---

## 6. PIE — ONE ROW ANSWERED BY JONATHAN, THE REST GENUINELY OWED (`VIS-§4`)

✅⭐ **(3c) — ANSWERED, and by Jonathan directly, in his own words:**

> *"opening the war map and right clicking empty ground does not cause it to close, the map seems to function exactly as it should."*

⇒ ⛔ **`Interface.WarMap`'s sentence is the FALSE one** — it claims the map is *"closed by right-click or Escape"*. `Interface.MapMarks` (*"a right-click that hits no circle does nothing at all"*) is **TRUE**. **`TASK-870`'s target is the `Interface.WarMap` row**, and `TASK-872` row (a) can now check the repair against a real observation. ⭐ QA's reading of `NativeOnMouseButtonDown` returning `FReply::Handled()` pointed at the right page; it could not settle it because the answer turned on Slate routing under the live input mode — **an observation, never a deduction.**

⛔⛔ **STILL OWED AND EXPLICITLY NOT PASSED — items (3), (3b), (3f), (3g).** I ran **no** PIE session. Every row is named rather than inferred:

- **(3)** blue ghost on hover · taller-and-not-floating on confirm · health bar shows the higher max · ×5 at **four** upgrades with the **fifth** still buying health (the HUD line fires **at confirm, once** — watch the click, not the hover) · **`WatchTower` shows RED with a refusal** · **the WatchTower ladder still climbs** (`STACK-§2`'s whole argument).
- **(3b)** the three new `TAB` rows readable, and the three wheel meanings reading as **three different things**.
- **(3f)/(3g)** the wheel row **with `Log LogGitClaudeUnrealTest Verbose` enabled first** — ⛔ **an empty log reads as *"the wheel never moved"***, the false-pass shape this project has already bought twice — plus the **spawned-building-matches** check, which is the only instrument on W-6's `BP_Building_*` construction-script risk.

**Reason:** the editor is **DOWN** (verified: no `UnrealEditor` process, **port 8000 clear**) and raising it is the orchestrator's under item (0b), not mine.

---

## 7. STATE OF THE TREE

- ✅ **Commit `d287102` on `main`. ⛔ NOT PUSHED** — `main` is **8 ahead** of `origin/main`.
- ✅ **`TASK-811`'s preconditions: all CLEAN** — `SiegePlayerController.{h,cpp}` · `Building.{h,cpp}` · `ClimbableTower.h` · `Tests/SiegePlacementTest.cpp` · `Tests/SiegeBuildingStackTest.cpp` · `Tests/SiegeLadderClimbTest.cpp`. **`811` may proceed.**
- ✅ **All seven auto-staged assets still staged and untouched.**
- ⛔ **Not one source file was edited by me.** The compile and suite are read-only over the working tree.
- ✅ Editor still **DOWN**, port **8000 clear** — the same-window mitigation for `814`→`811` is **preserved**.
- ✅ Long-standing dirty art (Castle meshes/textures/FBX) and `L_Arena`: **untouched**.

⚠️ **The one thing this commit does NOT prove, stated plainly:** I compiled the **working tree**, never `HEAD`. The new `HEAD` is **reasoned** compilable — on the symbol-closure evidence in §2, which is strong but is not a build. ⭐ The exposure is **one commit on an unpushed local `main`**, and it closes when `TASK-811` lands in this same window.

---

## 8. Follow-up findings for the manager

1. ✅ **RESOLVED BY RULING** — the funnel test re-homed to `TASK-835`. ⭐ **Keep the lesson: a test file follows the code it guards, not the task that repaired it.**
2. ✅ **BOARDED** — `Tests/SiegeAssistantSelectionTest.cpp` → `TASK-906` behind `TASK-889`. It is in my census (the `+1`) and I left it.
3. ✅ **BOARDED** — `Tests/SiegeCardHandKeyLabelTest.cpp` (5 tests) → `TASK-811`. ⚠️ **`811` must actually take it** or `HEAD` silently loses 5 tests.
4. ⚠️ **`§25b` fire #5 is 7 assets, not 2** (§4) — `T_CardArt_Witch.uasset` and the other six remain **digest-UNMEASURED**.
5. ⚠️ **`TASK-900`** — the cast bar's widget half is cancelled by Jonathan; `TASK-860`'s data path is **reviewed, inert and consumer-less** in the tree until `900` removes it.
6. ⚠️ **`qa/TASK-878.md` §F7 stands:** `TASK-874` was never delivered into that verdict — **split or re-open the row**; `TASK-876` must not wait on it.
7. ⚠️ **`TASK-870`/`TASK-872` are now unblocked** by (3c) — target the **`Interface.WarMap`** row. ⛔ A repair on the *other* page is an automatic fail.

---

## 9. Verbatim evidence index

| claim | evidence |
|---|---|
| compile | `Result: Succeeded` (sole `Result:` line; `Result: Failed` ×0) |
| suite | `426 Result={Success}` · `Result={Fail}` ×0 · one variant tree-wide |
| ladder row | `Result={Success} Name={ClimbDirectionIsByteIdenticalAndItsThreeOtherReadersStillReadTheLine}` |
| census | `426` across `31` files, scoped `^IMPLEMENT_SIMPLE_AUTOMATION_TEST`, reconciled from `HEAD` 306/23 |
| the stop | `IsHostileTeam`/`GatherHostileAgents`/`GatherFriendlyAgents` = **0** in `HEAD:SiegeCombatStatics.h`, called at `:437 :496 :504` |
| commit | ✅ **`d287102`** — 8 files, 6369(+)/17(−), 0 `Content/` paths, **not pushed** |
| (3c) | Jonathan: *"…right clicking empty ground does not cause it to close…"* |

Logs: `scratchpad/build-814.log` · `scratchpad/suite-814.log`
