# TASK-1124 — [GFX-SHIP-2] build-master handoff — THE PLAYABLE CHECKPOINT

⛔ **THIS FILE IS BORN OUTSIDE ITS OWN COMMIT** (`TL-§5e` cl. 7, the MINTED row). It carries
`61702e1`, which did not exist when the commit was made. **Expected, bounded at one, never
amended — the NEXT commit host takes it under cl. 7a.**

**Commit: `61702e1`** · **HEAD before: `42734b7`** (⭐ `TASK-1117`) · `main` **4 ahead of
`origin/main`** · ⛔ **NOT PUSHED.**

---

## 0. THE SENTENCE THIS ROW EXISTS FOR

🧑 Jonathan can now open **Settings → Graphics** and the sliders move. The facade shipped in
`42734b7` with no menu on top of it; this is the commit that puts one there.

---

## 1. THE RENAME RE-GREP — RUN **BEFORE** ANY BUILD (board's leading duty)

✅ **PASSED AT MY OWN INSTANT. No fallback was needed and no row was sent back.**

| what | where | found |
|---|---|---|
| declaration | `SiegeGraphicsSettingsSubsystem.h:710` | ✅ `float GetFoliageQualityScale() const;` |
| definition | `SiegeGraphicsSettingsSubsystem.cpp:1098` | ✅ |
| the call | `BattlefieldScatter.cpp:406` | ✅ `FoliageCullScaleCached = Graphics->GetFoliageQualityScale();` |
| old name surviving | 2 deliberate prose sites + 1 header mention | ✅ `BattlefieldScatter.cpp:401` (the "⛔ NAME:" comment), `facade.h:677` ("RENAMED FROM"), `SiegeGraphicsMenuWidget.h:135` — **zero code hits** |

⇒ `TASK-1118` and `TASK-1122` were built **together**, as the coupling required. The
mis-attribution trap never fired.

---

## 2. COMPILE

```
Result: Succeeded
```

⛔ **Parsed from the log, never from `$LASTEXITCODE`** — every run returned `EXITCODE=0` and that
fact carries **no information** (the `Build.bat` exit-code law). **Smart App Control did not fire:**
no `0x800711C7`, no ~2 s death, in any of the 53 builds.

**53 builds, all green** = 1 baseline + 46 mutation builds + 1 for the cl. (5c) assertion + 1 final
post-restore build (plus 4 re-runs of `M22`/bookkeeping). Editor was **closed by name** before the
first build (`taskkill /F /IM UnrealEditor.exe`, PID **26708**, standing grant) — required, because
the diff adds two new `UCLASS`es (`USiegeGraphicsMenuWidget`, `USiegeFrameRateCounterWidget`) and
Live Coding cannot help with those.

**`Source/` was confirmed quiet before the first build** (UBT globs every `.cpp`): newest write
anywhere under `Source/` was `1788850366`, **~32 minutes** before my first build. Nothing landed
during the run.

### 🚨 `SC-§99` — THE PARSER CONTROL, AND THE HONEST NEGATIVE

⛔ **I DID NOT GET AN UNINTENDED FAILURE, AND I DID NOT MANUFACTURE ONE.** All 46 mutation edits
applied cleanly on the first attempt (the dry-run in §4 is why), so **no `Result: Failed` was
produced by this run.** Per cl. 4 this is a **harvesting rule, not a break-the-build rule**, and
per cl. 6 *"I did not get one"* is the **compliant outcome**, not a gap.

⚠️ **What I have instead, named as the weaker instrument it is:** the parse expression was fed the
synthetic `Result: Failed (OtherCompilationError)` fixture left in the session scratchpad by
⭐ `TASK-1117` and correctly rejected it. **That is a control on the READER (`SC-§96`), not on the
BUILD (`SC-§99`).** It proves my `grep` can say "Failed"; it proves **nothing** about whether UBT
would have told me. The identical expression was harvested against a **real** `Result: Failed` by
⭐ `TASK-937` — ⛔ **that is a citation, not my measurement** (`SC-§40` cl. 1).

⇒ **The strongest true statement: 53 greens resting on a parse expression proven alive, on a
toolchain path this run never observed failing.**

---

## 3. SUITE — EXECUTED AND BOUNDED (`SC-§87`), NOT CENSUSED

Runner: `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi
-unattended -nopause -nosplash -NoLiveCoding -log -abslog=…`, wrapped in
`timeout --signal=KILL`, bound **1500 s** for the baseline/final and **900 s** per mutation.
⛔ **No run was ever killed.** Wall clock per full run: **~40 s.**

| | baseline (shipped tree) | FINAL (= what is in `61702e1`) |
|---|---|---|
| `Result={Success}` | **552** | **552** |
| `Result={Fail}` | **0** | **0** |
| `Test Started` / `Test Completed` | 552 / 552 | 552 / 552 |
| log | 6,990 lines / 1,180,252 B | 6,989 lines / 1,180,209 B |

⛔ **`N` was read FIRST, is non-zero and plausible, and `Started == Completed == Success + Fail`**
(`SC-§95` cl. 1: a run that starts zero tests is indistinguishable from a green one). **Nothing
hung.**

### RECONCILIATION — RESIDUAL ZERO

| source | Δ | evidence |
|---|---|---|
| **MEASURED** baseline | **517** | ⭐ `TASK-1117` @ `42734b7` |
| ⭐ `TASK-1115` — the panel | **+12** | censused by `qa/TASK-1116.md` |
| ⭐ `TASK-1118` — the countdown | **+7** | censused by the author |
| ⭐ `TASK-1122` — the cull band | **+9** | censused by the author |
| ⭐ `TASK-1120` — the readout | **+7** | censused by `qa/TASK-1121.md` |
| | **= 552** | **measured 552. Residual 0.** |

⚠️ **`qa/TASK-1121.md`'s `552` across 46 files is a DECLARATION CENSUS and my `552` is an
EXECUTION.** They agree, which is corroboration — ⛔ **it is not the source.** ⛔ I never quote
`529`, `536`, `545` or `306` as measured; the only prior measured figure is `517 / 0` at `42734b7`.

⚠️ `Saved/Config/WindowsEditor/GameUserSettings.ini` — **untracked and unstaged**; suppression did
not leak into the commit. (I recorded its md5 `88dd4565e3d904530503144bc667e4c5` at the end rather
than diffing it across every run; declared as the weaker check it is.)

---

## 4. MUTATIONS — **46 NAMED, 46 EXECUTED, 0 DEFERRED**

Each its own compile. Each **restored byte-exact** before the next began — ⛔ **never two open at
once**, per the coordinator's rule and because these files are *uncommitted*, so `git checkout`
could not have saved them.

**The instrument was controlled before it was trusted** (`SC-§96`): a **dry run** applied all 46
onto pristine copies with **no compile**, and asserted for each that (a) its anchor matched
**exactly once** and (b) the file actually changed. **46/46 OK, zero inert mutations, restore
byte-exact.** That is why no mutation edit failed later — and it is also why `SC-§99` had nothing
to harvest.

**Restore proof:** 17 pristine files under sha256; after **every** batch the live tree was compared
against the manifest. ✅ **All 17 matched, every time, including after the final batch.**

### 4.1 ⭐ `TASK-1115` — the panel (M1–M13)

| M | predicted | **ACTUAL** | verdict |
|---|---|---|---|
| **M1** | `DetentSliders…` (11 assertions) and **nothing else** | that test, **11** assertions | ✅ **EXACT** |
| **M2** | the same test's final `TestFalse`, **only** that one | that test, **1** assertion (the scale-bar row) | ✅ **EXACT** |
| **M3** | `ResolutionScaleWritesOnCommit…`, the 50-drag row | that test, **5** assertions (50 saves / 50 applies / 50 broadcasts) | ✅ WIDER-IN-TEST |
| **M3b** | the same test's **BINDING half only** | that test, **6** — binding row **+** the behaviour rows | ⚠️ **WIDER, and the sub-claim is refuted:** a bound dynamic delegate really is *invoked*, so binding the write handler also produces the writes. Contains the predicted row ⇒ confirmation. |
| **M4** | gate NIT-3: **FOUR** rows in `DiscardStagedVideoModeRevertsExactlyOnce` | **exactly 4** there, **+2** in `BackDuringCountdownRevertsExactlyOnce` | ✅ gate's correction **EXACT**; wider only by a `TASK-1118` test that did not exist when the prediction was written |
| **M5** | gate NIT-4: **FIVE** rows in `BackReleasesAutoDetectAndEverySave` | **exactly 5** there, **+5** across two more tests | ✅ gate's correction **EXACT**; positive-control block stayed GREEN as promised |
| **M6** | `CustomLabelIgnoresInvisibleInputs`, the two ⭐⭐ rows; fixture self-check GREEN | that test, 3 assertions; **self-check stayed GREEN** | ✅ CONFIRMED |
| **M7** | gate NIT-6: `LevelSliderValueRoundTrip` **only** — the author's hedge does **not** derive | exactly that test, 3 assertions. **`GroupCommitWritesOnlyTheMovedGroup` stayed GREEN** | ✅ **gate RIGHT, author's hedge REFUTED by execution** |
| **M8** | `StepperIndexWrapsBothWays` | exactly that, 1 assertion | ✅ **EXACT** |
| **M9** | `NextMatchStartNoticeIsScatterDrivenOnly` | exactly that, 3 assertions | ✅ CONFIRMED |
| **M10** | gate WARN-5: the `TestNotEqual` **only** | exactly that, **1** assertion | ✅ **gate's correction EXACT** |
| **M11** | `TreeBuildsEveryPinnedRow`'s `GFX-§2(f)` row | exactly that, 1 assertion | ✅ **EXACT** |
| **M12** | `TreeBuildsEveryPinnedRow` pairing **and** `GroupCommit…`'s loop | `TreeBuildsEveryPinnedRow` **+** `DetentSliders…`; ⛔ **`GroupCommit…` stayed GREEN** | ⚠️ **wider AND narrower — a mixed finding, see §4.5** |
| **M13** | `NullSubsystemDisablesEveryControlButBack`'s ⭐ row | 🚨 **NOTHING. `552 / 0`.** | 🚨 **FINDING — §4.5** |

### 4.2 ⭐ `TASK-1118` — the countdown (M14–M22)

| M | predicted | **ACTUAL** | verdict |
|---|---|---|---|
| **M14** | gate WARN-2: exactly one — "APPLIED PROVISIONALLY" | that row **+2** in `SteppingBack…` | ✅ WIDER; the "exactly one apply on the revert" row **stayed GREEN** exactly as the gate corrected |
| **M15** | ≥4, all in `VideoModeCountdownExpiryReverts`, four named | that test only, **6** assertions incl. **all four named**; nine-tick block GREEN | ✅ CONFIRMED |
| **M16** | gate WARN-1: **exactly 2**, both immediately after `BackPressed()` | **exactly those 2**; stranded-tick rows GREEN | ✅ **EXACT** |
| **M17** | ≥2 in `NoTimerNoApplyAndQualityNeverArms`, both named | **exactly those 2** | ✅ **EXACT** |
| **M18** | ≥1 incl. the "NOT APPLIED" row | 3 tests, 4 assertions incl. it | ✅ WIDER |
| **M19a** | ≥1 incl. "a QUALITY change arms NO confirmation"; frame-rate row **GREEN** | that test, **2** — incl. **the frame-rate row** | ⚠️ WIDER; the *"stays green"* sub-claim is **refuted** |
| **M19b** | ≥1 incl. "reaches NO provisional display apply" | 2 tests, 3 assertions incl. it | ✅ WIDER |
| **M20** | gate WARN-4: **exactly 2**; stranded-tick row **not** among them | **exactly those 2** | ✅ **EXACT** — and see §4.4 |
| **M21** 🚨 | ≥4, "**exactly four**"; apply-count row **GREEN** by coincidence | 🚨 **FIVE** assertions; **apply-count row GREEN** | ✅ **REDDENED (the fix is real)** · ⭐ **board cl. (5b) now MEASURED** · ⭐ **`SC-§104` cl. 1 CONFIRMED BY EXECUTION** |
| **M22** | **exactly 4**; both prompt rows **GREEN** | **exactly 4** on the pristine tree; both prompt rows GREEN. **FIVE** after cl. (5c) | ✅ **EXACT**, then deliberately widened — §5c |

### 4.3 ⭐ `TASK-1120` — the readout (M23–M35) and ⭐ `TASK-1122` — the cull band (M1–M8, run as S1–S8)

| M | predicted | **ACTUAL** | verdict |
|---|---|---|---|
| **M23** | ≥6 incl. two named | **6**, both named present | ✅ CONFIRMED (gate's "6 by my count" exact) |
| **M24** | ≥5 incl. two named | **6**, both named present | ✅ WIDER (gate's "6" exact) |
| **M25** | **EXACTLY 1** | **exactly 1** | ✅ **EXACT** |
| **M26a** | **EXACTLY 1** | **exactly 1** | ✅ **EXACT** |
| **M26b** ⭐ | **EXACTLY 2**, both named | **exactly those 2** | ✅ **EXACT** — the click-eating bug is now a measured red |
| **M27** 🚨 | ≥3 incl. "recipe step 5 is present" | **4**, incl. it; the other-direction row **GREEN** as predicted | ✅ CONFIRMED — **`SC-§105`'s defect made executable, and it reddens** |
| **M28** ⭐⭐ | **EXACTLY 2, both NAME rows**; every count row GREEN | **exactly those 2**; no count row moved | ✅ **EXACT** — `SC-§104`'s lesson, executable and holding |
| **M29** | **EXACTLY 1** | **exactly 1** | ✅ **EXACT** |
| **M30** | **EXACTLY 1** | **exactly 1** | ✅ **EXACT** |
| **M31** | ≥3 incl. the fail-safe row; "no timer armed" GREEN | 3 across 2 tests incl. it; timer row GREEN | ✅ CONFIRMED |
| **M32** | ≥5 incl. **two** named rows | **5** — but ⛔ **the drift-guard row is NOT among them** | ⚠️ **FINDING — §4.5** |
| **M33** | ≥5 across both settings tests, two named | **6**, both named present | ✅ WIDER |
| **M34** ⛔ | **ADMITTED ZERO-RED** | **`552 / 0`** | ✅ **the declared result** (§5e-i) |
| **M35** ⛔ | **ADMITTED ZERO-RED** | **`552 / 0`** | ✅ **the declared result** (§5e-i) |
| **S1** | ≥3, three named tests | **exactly those 3** | ✅ **EXACT** |
| **S2** | author: ⛔ **DECLARED ZERO-RED**. Gate WARN: **≥1, the `(4000,0)` case** | 🚨 **1 test, 5 assertions** — *"A zero end keeps its non-zero start untouched"* (`4000` → `0`) | 🚨 **GATE RIGHT, AUTHOR REFUTED — §4.4** |
| **S3** | ≥1 `NeverCulled…` (gate: 2) | **3** tests | ✅ WIDER |
| **S4** | ≥1 Floor (gate: Floor + Monotone) | **exactly those 2** | ✅ **EXACT vs the gate** |
| **S5** | ≥1, **and only the authored HARD-POP case** | that test, 3 assertions, **all hard-pop**; the three real layers GREEN | ✅ **EXACT incl. the refinement** |
| **S6** | ≥1 Floor | that test, 2 assertions | ✅ CONFIRMED |
| **S7** | ≥1 Sqrt (gate: "may add UB noise in Floor — revert at once") | Sqrt **+** Floor's "never gains a negative band" | ✅ CONFIRMED, **UB noise exactly as anticipated**; restored immediately |
| **S8** | author: ≥1. **Gate WARN-1: ZERO** | **`552 / 0`** | 🚨 **GATE RIGHT, AUTHOR REFUTED — §4.4** |

**Tally: 46 executed · 42 red · 4 green · 0 deferred · 0 unexplained.**

### 4.4 ⭐ THE THREE RESULTS WORTH KEEPING

1. **`M20` now reds, and before the loop-1 fix it had NO RED AT ALL.** That is not a better test —
   it is the **executed proof that the wrap-around branch became reachable.** ⛔ A mutation with no
   red is evidence about **reachability** before it is evidence about an assertion, and this is that
   sentence being paid off by a machine rather than argued.
2. **`M21` reddened, and the apply-count row stayed GREEN.** `SC-§104` cl. 1 predicted precisely
   this: `ModeCount−1` provisional applies **+ one more provisional** equals `ModeCount−1` **+ the
   closing revert's**. ⛔ **The tally is identical on both branches and the state rows are not.**
   The law was written from a reading; it is now **measured**.
3. **The gate beat the author twice, in opposite directions, on the same file family.** `S2` was
   self-declared a no-red and **reddens** (the gate's `(4000,0)` case); `S8` was claimed as a red and
   is a **zero-red** (the gate's `Clamp` collapse). ⛔ **Both times the party with no compiler was
   right and the party who wrote the code was wrong** — `SC-§83`'s addendum, paid again: *mutation
   coverage is a reading problem, and the author is the worst-placed party to see it.*

### 4.5 🚨 TWO FINDINGS — PREDICTED REDS THAT DID NOT ARRIVE

⛔ Reported under Addendum B's asymmetric rule: **a narrower set, or one missing a predicted row, is
a FINDING.** ⛔ **Neither is a defect in shipped behaviour. Both are defects in a CLAIM.** I
corrected each **where it is written** and repaired **no code**.

**(a) `M13` — predicted RED, measured GREEN, and the cause is structural.**
`BackButton` is **deliberately absent** from `SetAllControlsEnabled`'s `Controls[]` array. ⇒ Back is
**never disabled**, so `ShowPanelUnavailable`'s re-enable is a no-op with respect to the pin, and the
⭐ assertion *"Back is STILL ENABLED"* is **true on both branches**. ⛔ **`SC-§104` cl. 5(a), a fourth
independent instance, in a fresh place.** ✅ **The player really can always leave the dead panel** —
what was false is the claim that this pin proves it. The re-enable is defence-in-depth against a
future edit that adds `BackButton` to `Controls[]` — the same shape as the scatter's `BaseEndUU<=0`
sentinel — so ⛔ **it must not be deleted.** Corrected in the M13 table entry; ⚖️ **routed to the
manager**: if a real pin is wanted, it must assert on `SetAllControlsEnabled`'s *membership*, not on
Back's enabled state.

**(b) `M32` — 5 reds, but one of its TWO named rows never moved.**
The absent row is `SiegeSettingsTest.cpp:619`, *"The subsystem's compiled default AGREES with the
SaveGame's (drift guard)"*. Cause: `MakeScratchStore()` runs `Initialize() → LoadSettingsFromSlot()`,
and with no slot on disk `Source` **is the SaveGame CDO** — so moving the SaveGame default moves the
subsystem's live value **with it**. ⛔ **The two sides of the comparison are not independent
witnesses; they are one witness read twice** (`SC-§105` cl. 3). ⇒ **the drift guard cannot detect
the drift it is named for.** ⚖️ **Routed, NOT repaired** — a real fix reads the subsystem **CDO**
(`GetDefault<USiegeSettingsSubsystem>()`) *before* any load, and that is a `Source/**` test edit with
no gate on it, which is exactly what ⭐ `TASK-1140` is blocked on me to prevent.

**(c) minor, folded in:** `M12` reddened `TreeBuildsEveryPinnedRow` **and** `DetentSliders…` while
`GroupCommitWritesOnlyTheMovedGroup` — a **named** predicted row — stayed **GREEN**. Same class as
(a), lower stakes; recorded here rather than corrected, because the mutation's headline row *did*
red and the pairing property *is* covered.

---

## 5. THE CHECKLIST — (5a)–(5e), EACH DISPOSED

| # | disposition |
|---|---|
| **(5a)** | ✅ **DONE.** The false comment is corrected in `SiegeGraphicsMenuWidget.cpp` (now ~`:1938`, **not** the board's `:1654-1655` — line numbers moved when `TASK-1120` landed). It read *"nothing the player can see moves"*; ⛔ **that is false and it matters**: the *staged* mode equals the confirmed one by the time the branch fires, but the mode **on screen** is the previous press's provisional apply, so the discard really does change the picture — **and it must**, because the wrapping press is the player saying *"put it back"*. Compile-only burden, no decoy, no control owed. Verified by the FINAL build. |
| **(5b)** | ✅ **DONE, AND MEASURED RATHER THAN ASSERTED.** `M21`'s entry claimed *"EXACTLY four"* while **enumerating five**. I executed it: **five**. Corrected in place in the test file's table with the measurement attached. ⚠️ **The same undercount also stands in `handoffs/TASK-1118-programmer.md` § L1.2** — that is another agent's document, so per `SC-§97` cl. 4 the annotation is the **manager's**; ⚖️ **routed**. |
| **(5c)** | ✅ **DONE, WITH A WITNESSED RED.** Added **one assertion above the reset** in Test 18 pinning that the wrap-around's `CloseVideoModeWindow` **flushed the deferred save**. ⛔ Written as a **difference** (`SaveSettingsCallCount > SavesBeforeWrap`, snapshotted before the wrapping press), **not** as a bare `>= 1` — a total would have been satisfied by any earlier save and green on both branches, which is the very disease `SC-§104` names. **Measured: green on the shipped tree (`552 / 0`), and `M22` widened from FOUR reds to FIVE with the new row among them.** ⇒ `SC-§83`'s *"at least one observed RED per authored pin"* is **satisfied for this pin**. |
| **(5d)** | ✅ **HONOURED.** I touched the `M21` mutation and I asserted **STATE, never tallies**: the five reds are the branch predicate, the remainder, the facade window, the Auto-Detect refusal and the save — ⛔ and the **apply-count** row, the one tally in the set, is exactly the row that **stayed green**. `SC-§104` is no longer a derivation in this lane; it is a measurement. |
| **(5e-i)** | ✅ **CARRIED, NOT EVAPORATED.** `qa/TASK-1121.md` §4 F-13 **RULED: KEEP `M34`/`M35`, labelled.** Quoted grounds: *"a prediction row equal on both branches is STRUCK; a mutation with no red anywhere is KEPT AND LABELLED — the first is a false claim, the second is a true disclosure."* I ran both anyway (cheap): **`552 / 0` both times.** ⛔ Reported as **the declared result they are, not as failures**, and their green is **not** quoted as coverage. Recorded on `TASK-1120`'s board line so it survives this handoff. |
| **(5e-ii)** | ✅ **UNTOUCHED, AS ORDERED.** F-7 (the panel readout scrolling off) is ruled and homed on ⭐ `TASK-1125` item (8). ⛔ **I changed nothing about the panel's spine** — no `HeaderPanel`, no re-parent, no scroll-box change. |
| **(5e-iii)** | ✅ **NAMED, NOTHING REPAIRED.** `qa/TASK-1121.md` §7(3): `SaveSettingsToSlot()`'s own census is **`N = 0`** — every `USiegeSettingsSaveGame` field is in the copy list. ⇒ ⭐ **`TASK-1143` closed as unnecessary** on the board. ⚠️ **The gate found the same species ONE FUNNEL OVER and it is NOT mine:** `bShowFrameRateCounter` is absent from `SiegeCloudSync.cpp`'s `ACC-§13` payload while `SiegeSettingsSaveGame.h:87-92` promises it follows the account across machines. Different file, outside `1120`'s WRITES list, **a product call for the manager** (ship the key, or soften the sentence). ⚖️ **Routed. I repaired nothing.** |

---

## 6. THE COMMIT

**Pathspec derived at my own instant**, `git status --short --untracked-files=all --ignored --
.claude/pipeline/` for the documents half. ⛔ **Every `??` staged by explicit path** (`TL-§5e`
cl. 7a-iv) — ⛔ **no `-A`, no `.`, no bare directory.** `.git/index.lock` checked absent first.

**29 files:** 17 `Source/**` (13 modified + 4 new) · `CONVENTIONS.md` · `TASKBOARD.md` ·
6 handoffs · 4 QA reports.

```
$ git show --stat HEAD
 GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md                     |   96 +-
 GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md                       |  347 +-
 .../pipeline/handoffs/BOARD-STALENESS-audit.md                          |  220 ++
 .../pipeline/handoffs/TASK-1115-programmer.md                           |  293 ++
 .../pipeline/handoffs/TASK-1117-buildmaster.md                          |  367 +++
 .../pipeline/handoffs/TASK-1118-programmer.md                           |  404 +++
 .../pipeline/handoffs/TASK-1120-programmer.md                           |  233 ++
 .../pipeline/handoffs/TASK-1122-programmer.md                           |  204 ++
 GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1116.md                    |  166 +
 GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1119.md                    |  332 ++
 GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1121.md                    |  172 +
 GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1123.md                    |  140 +
 .../Siegebound/BattlefieldScatter.cpp                                   |  287 +-
 .../Siegebound/BattlefieldScatter.h                                     |  174 +
 .../Siegebound/SettingsMenuWidget.cpp                                   |  113 +
 .../Siegebound/SettingsMenuWidget.h                                     |   39 +
 .../Siegebound/SiegeGraphicsMenuWidget.cpp                              | 3363 ++++++++++++
 .../Siegebound/SiegeGraphicsMenuWidget.h                                | 1415 ++++++
 .../Siegebound/SiegeGraphicsSettingsSubsystem.cpp                       |   19 +-
 .../Siegebound/SiegeGraphicsSettingsSubsystem.h                         |   72 +-
 .../Siegebound/SiegePlayerController.cpp                                |   73 +
 .../Siegebound/SiegePlayerController.h                                  |   51 +
 .../Siegebound/SiegeSettingsSaveGame.h                                  |   31 +
 .../Siegebound/SiegeSettingsSubsystem.cpp                               |   56 +-
 .../Siegebound/SiegeSettingsSubsystem.h                                 |   62 +-
 .../Siegebound/Tests/SiegeGraphicsMenuTest.cpp                          | 2508 +++++++++
 .../Siegebound/Tests/SiegeGraphicsSettingsTest.cpp                      |   34 +-
 .../Siegebound/Tests/SiegeScatterCullBandTest.cpp                       |  643 ++++
 .../Siegebound/Tests/SiegeSettingsTest.cpp                              |  291 ++
 29 files changed, 12100 insertions(+), 105 deletions(-)
```

⛔ **VERIFIED AGAINST THE COMMIT, NOT THE INDEX.** 29 staged == 29 committed == 29 derived. **No
stray.** Index empty afterwards. ⛔ **NOT PUSHED** — `main` is **4 ahead** of `origin/main`.

### The fences, proven — with a **positive control on the filter first** (`SC-§96` cl. 3a)

The filter was shown able to **match** before it was trusted to return zero: `Source/` → **17**,
`.cpp` → **10**. Then: `.uasset` **0** · `.umap` **0** · `Config/` **0** · `Content/` **0** ·
`Tools/` **0** · `Saved/` **0** · `Intermediate/` **0** · `Binaries/` **0** · `testvideo/` **0**.
⛔ **`L_Arena.umap` was never opened and never saved** — the editor was closed for the whole build
and mutation run and only relaunched after the commit.

### `TL-§5e` cl. 7a — the standing sweep, with its classifications

- ⛔ **`--ignored` was used** (cl. 7a-ii(a)): **zero `!!` lines** under `.claude/pipeline/`. Nothing
  to route to the manager. The on-disk-vs-tracked control (7a-ii(b)) is **not owed** — it is
  mandatory only on a **zero** derivation, and mine returned **12 lines**.
- ⛔ **cl. 7a-iii, PRECEDENCE, and I recorded which limb applied:** the table would exclude
  `CONVENTIONS.md`/`TASKBOARD.md`, but **my own row names both** — cl. (2) and cl. (2c) — so **the
  row overrides the table** and I took them. ⭐ `CONVENTIONS.md` is cl. (2c)'s whole point: the
  five-step recipe correction existed **only in the working tree** and was one `git restore` from
  being a falsehood again.
- ✅ **ORPHAN TAKEN:** `handoffs/BOARD-STALENESS-audit.md` — a build-master read-only sweep with
  **no `TASK-###` row and therefore no named host**, so cl. 7a-v's exception does **not** apply
  (it requires a host ID **and** an uncommitted host). A genuine orphan; swept.
- ✅ **`handoffs/TASK-1117-buildmaster.md` taken** — the one-cycle MINTED lag is the **expected**
  state, staged normally, ⛔ **not reported as a finding**, exactly as the row instructed.
- ✅ **ALREADY SWEPT ⇒ NOT CANDIDATES, named with the commit that took them** (cl. 7a-i):
  `handoffs/TASK-1091-artist.md`, `Tools/ArtPipeline/pipeline_manifest.json` and the three
  `playtest-evidence/2026-09-06/MainCharacter_preview_*.png` were all taken by **`9daa641`**
  (⭐ `TASK-1094`). `Content/RawAssets/MainCharacter.fbx` is **neither on disk nor in `HEAD`** —
  superseded by the art lane, outside my fences either way.

### 🚨 A DEAD READER, CAUGHT BY ITS OWN CONTROL — `SC-§96` / `SC-§102`

⛔ **My first `git cat-file -e HEAD:<path>` sweep returned `NO` for every path — and it was WRONG.**
The git root is **one level above** the project directory (`git rev-parse --show-prefix` =
`GitClaudeUnrealTest/`), so every unprefixed lookup missed. ⛔ **I only knew because I had included a
control that MUST return YES, and it returned NO.** Re-run with the prefix and controlled in **both**
directions (`CONVENTIONS.md` → YES, `qa/TASK-1116.md` → NO), the answers inverted completely.
⚖️ ***An unread value compares unequal to everything, so a broken reader reports maximum alarm and
looks exactly like a catastrophic finding.*** Recorded because it fired **on this row**, not
because it is quoted in the law.

---

## 7. STATE LEFT BEHIND

- ✅ **Working tree: clean except `TASKBOARD.md`** — my own six status lines, which carry the hash
  and therefore **cannot** be inside the commit that produced it. **Expected dirt, bounded, mine.**
- ✅ **This file** — `handoffs/TASK-1124-buildmaster.md`, **MINTED**, outside its own commit,
  bounded at one, ⛔ **never amend it.** The next commit host takes it under cl. 7a.
- ⛔ **ARRIVED class: none.** No `.claude/pipeline/` document landed during my staging window.
- ✅ **Editor RELAUNCHED, PID `14188`.** **MCP on `http://127.0.0.1:8000/mcp` answering** (HTTP 405
  to a bare GET = the server is up and rejecting the wrong verb). ⛔ **`L_Arena` was never saved**;
  the editor was killed by name under the never-save law and nothing was dirty when it went.
- ✅ Nothing staged that was not committed. No `.git/index.lock`.
- ⛔ **NOT PUSHED.** `main` **4 ahead** of `origin/main`.

**Budget vs actual:** budgeted 46 mutations × ~90 s ≈ **70 min** against a ~2 h window, decided
**before** starting; spent **~75 min** on the mutation set (compile ~40 s + suite ~40 s per cycle,
faster than the 2-hour worst case because the suite runs in **40 seconds**, not the 15 minutes the
`1500 s` bound allows for). **No cut was needed and nothing was deferred.**

---

## 8. ROUTED TO THE MANAGER (not mine to change — `SC-§82`)

1. 🚨 **`M13` is not a pin** (§4.5a) — a real one must assert `BackButton`'s **absence from
   `Controls[]`**, not Back's enabled state. Needs a row; ⭐ `TASK-1140` is the natural neighbour.
2. 🚨 **The `SiegeSettingsTest.cpp:619` drift guard cannot detect drift** (§4.5b) — it must read the
   subsystem **CDO** before any load. Needs a row.
3. ⚠️ **`handoffs/TASK-1118-programmer.md` § L1.2 still says `M21` is "exactly four"** — measured
   five. `SC-§97` cl. 4 makes the downstream annotation yours.
4. ⚠️ **`SiegeCloudSync.cpp`'s `ACC-§13` projection omits `bShowFrameRateCounter`** while the header
   promises it travels (§5e-iii). **A product call:** ship the key, or soften the sentence. ⛔ Do not
   leave both as they are.
5. ⚠️ **`Tools/run_suite_bounded.ps1` DOES NOT EXIST** although `TL-§6` names it as *"the ONE
   sanctioned executor"* and makes it a **tracked** tool. I ran from a **session-scratchpad** copy
   inherited from ⭐ `TASK-1117` — ⛔ **precisely the unversioned-instrument state `TL-§6` was
   written to end**, and it survived only because that scratchpad happened not to be cleared.
   `Tools/**` is fenced from me, so I could not land it. **Needs a row.**
6. ⭐ **Three laws were confirmed by execution today, not by reading** — `SC-§104` cl. 1 (`M21`'s
   coincidence), `SC-§105` (`M27`), and `SC-§83`'s addendum twice over (`S2`/`S8`). Worth recording
   that they now have measurements behind them.

---

## 9. WHAT THIS COMMIT DOES **NOT** MEAN (`SC-§94` cl. B)

⛔ **I hold a compiler, a suite and `git`. I do not hold pixels.** ⛔ **NO PIXEL OF THIS PANEL, THIS
PROMPT OR THIS COUNTER HAS EVER BEEN RENDERED** — not by the authors, not by the gates, not by me.
`552 / 0` says every assertion those four rows wrote is true of the built code. It does **not** say
the panel appears, that it is legible, that a slider clicks between five positions under a real
mouse, that ten ticks take ten wall-clock seconds, or that a single moved setting changes a rendered
frame.

🚨 **And the sentence both gates asked to have carried forward: THE AUTO-REVERT HAS STILL NEVER
FIRED AGAINST A REAL `FTimerManager`.** Every one of its ten ticks is driven by hand through the
automation seam, because a bare `NewObject` widget has no world. `M15` proves the **expiry body**
reverts; ⛔ **nothing proves the timer ever calls it.** That is `GFX-§4`'s central promise and it is
**argued, not witnessed**.

⇒ **All of it is owed to ⭐ `TASK-1125`, and the cheapest three are:** change Window Mode, touch
nothing, and watch the old mode come back on its own · press `>` on Window Mode all the way round and
watch the prompt vanish while the picture ends where it started · tick the FPS box, start a match,
and click **through** where the counter is.
