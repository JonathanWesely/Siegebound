# TASK-1118 — GFX-REVERT — handoff (gameplay-programmer)

**Marker:** `TASK-1118-GFX-REVERT` · **Date:** 2026-09-07 · **Gate:** `TASK-1119` (qa-reviewer) · **Host:** `TASK-1124`
**Spec source:** the board row cl. (1)–(7) + routed riders (8), (8-WIDENED), (9), (10a), (10b) · `handoffs/TASK-1115-programmer.md` (the seam + the reconciliation note) · `handoffs/TASK-1113-programmer.md` incl. § LOOP 1 (the facade) · `qa/TASK-1116.md` · `qa/TASK-1114.md` (both loops) · `handoffs/TASK-1112-programmer.md`.
**Law:** `GFX-§4` (all four clauses) · `GFX-§2` (all six conditions) · `GFX-§5` · `GFX-§9` (NAME RULING) · `GFX-§10` (four pinned names) · `GFX-§11` · `SC-§79` `SC-§83` `SC-§87` `SC-§94` `SC-§101` + Addendum B.

🚨 **NO WITNESSED RED.** Nothing in this row has been compiled, executed or rendered. Every mutation below is a **derived prediction**, and this lane already holds two proofs that a confident prediction can be wrong: `TASK-1113`'s **M4** (did not derive) and its **M13** (derived over-broad, then *measured* narrower by `TASK-1117`). The compiler and the suite are `TASK-1124`'s.

---

## 0. WHAT A PLAYER NOW EXPERIENCES

Press `>` on **Screen Resolution** or **Window Mode**:

1. The picture **changes immediately** — provisionally, and **nothing is written to disk**.
2. A plate appears directly under the two steppers (and scrolls itself into view): **"Keep these settings? Reverting in 10 seconds."**, counting down every second, with **Keep** and **Revert**.
3. **Keep** → the mode stays and *only now* is it saved. **Revert** → the previous mode comes back.
4. **Nobody touches anything** → after ten seconds the old mode comes back **by itself**. That is the row: a player whose monitor cannot show the mode they picked sees a black screen, cannot find Keep, cannot find Back — and gets their game back anyway.
5. Pressing `>` again **restarts** the ten seconds. Pressing `>` until the value wraps back to the mode they started in **cancels the question** rather than asking about a change that no longer exists.

Quality sliders, the resolution scale and the frame-rate ladder are **untouched by all of this** — they still apply and save immediately (board cl. 3).

---

## 1. FILES WRITTEN (5 — nothing else)

| File | What |
|---|---|
| `Source/.../Siegebound/SiegeGraphicsMenuWidget.h` | the countdown's state machine in the class comment · 4 pinned + 2 derived widgets · 5 internals · 4 public entry points · 1 pure static · 1 automation seam · 4 state members |
| `Source/.../Siegebound/SiegeGraphicsMenuWidget.cpp` | the implementation · the prompt's tree · the two steppers' seam filled · `Back`/teardown reconciled · **two player-facing strings corrected** (§6) |
| `Source/.../Siegebound/SiegeGraphicsSettingsSubsystem.h` | **(10a)** rename + doc · **(10b)** counter doc widened |
| `Source/.../Siegebound/SiegeGraphicsSettingsSubsystem.cpp` | **(10a)** the definition |
| `Source/.../Siegebound/Tests/SiegeGraphicsMenuTest.cpp` | **7 new tests** · M14–M20 · the four routed prediction corrections |
| `Source/.../Siegebound/Tests/SiegeGraphicsSettingsTest.cpp` | **(10a)** 8 sites · the **M13** comment corrected from `TASK-1117`'s witnessed red |

⛔ **ZERO** `Content/**` · **ZERO** `Config/**` · **ZERO** `.uasset` · **ZERO** `CONVENTIONS.md` · **ZERO** `TASKBOARD.md` beyond my own single `- status:` line · **NO compile, NO editor, NO MCP, NO git.**
⛔ **`TASK-1122`'s files were not touched.** `BattlefieldScatter.{h,cpp}` and `Tests/SiegeScatterCullBandTest.cpp` are its dirt — **read once** (§7's grep) and never written.

⚠️ **ONE `names:` DEVIATION, DECLARED.** The board's `names:` line says `Tests/SiegeGraphicsSettingsTest.cpp`, written before `TASK-1115` existed. The countdown lives in the **widget**, and the widget's harness (`MakeScratchPanel`, `FindPinned`) lives in `Tests/SiegeGraphicsMenuTest.cpp`. The seven new tests are therefore **in the menu test file**; duplicating that harness into the settings test file would have been the worse of the two. I still wrote `SiegeGraphicsSettingsTest.cpp` — for (10a) and the M13 correction, which are its own.

---

## 2. THE STATE MACHINE

```
StepScreenResolution / StepWindowMode      (stage — TASK-1115's code, unchanged)
    └─> BeginVideoModeConfirmation(Graphics)
            ├─ Graphics null            ⇒ ShowPanelUnavailable, return
            ├─ !IsVideoModeChangePending()
            │      ⇒ DisarmVideoModeCountdown(), return       ← cancels a stale prompt
            ├─ !CanArmVideoModeCountdown()                     ← ⛔ no timer ⇒ NO APPLY
            │      ⇒ Warning + StatusText, return              (staged-only, Back discards)
            ├─ !ApplyVideoModeProvisional()  ⇒ Warning, disarm, return
            └─ ArmVideoModeCountdown()

ArmVideoModeCountdown()   Remaining = 10.0 · Active = true · repaint · show prompt ·
                          ScrollWidgetIntoView · ClearTimer then SetTimer(1 s, LOOPING)

TickVideoModeCountdown()  !Active ⇒ return                     ← the stranded-callback guard
                          Remaining -= 1.0
                          > 0 ⇒ repaint, return
                          = 0 ⇒ repaint "Reverting now…", Warning, RevertSettingsPressed()

KeepSettingsPressed()     disarm → (guard: pending?) → Graphics->ConfirmVideoModeChange()
RevertSettingsPressed()   disarm → DiscardStagedVideoMode(Graphics)
BackPressed()             disarm → DiscardStagedVideoMode(Graphics) → RemoveFromParent()
NativeDestruct()          UnbindAll → disarm → DiscardStagedVideoMode(Graphics)
```

**Why ONE repeating timer and not two.** The repaint and the expiry are the same event at different counts. Two timers would be two things to cancel on four exits, and the one eventually missed would be the one that fires at a player who has already left.

**Why the tick decrements by the period, not by delta time.** Ten ticks is then exactly ten seconds and the visible number never skips one. `10.0f − 1.0f` ten times is exact in binary float, so the `> UE_KINDA_SMALL_NUMBER` boundary lands on the tenth tick and not the ninth or eleventh.

**Why `Active` goes down BEFORE the handle is cleared** (`DisarmVideoModeCountdown`, first two lines): a tick can already be queued when the player presses Keep. Clearing the handle first and the flag second leaves a window in which that queued callback runs against an "active" countdown and **reverts the mode the player just chose to keep**. Asserted, not assumed — Test 14's last block.

---

## 3. ⭐⭐ THE RECONCILIATION — HOW THE `Back` PATH STAYED AT EXACTLY ONE REVERT

`TASK-1115` landed first and left the instruction *"call this one, or delete this one and own both — never add a third"*. **I called that one.** `BackPressed()`'s revert line is **byte-for-byte what `TASK-1115` shipped**; what I added above it is `DisarmVideoModeCountdown()`, which stops a timer and hides a widget and **reverts nothing**.

**The invariant is `RevertVideoModeChange()` call sites, and it is COUNTED, not argued:**

```
grep -n "RevertVideoModeChange" SiegeGraphicsMenuWidget.cpp
  → 4 hits: :1849 :2316 :2330 = COMMENTS · :2334 = THE ONE CALL
    (inside DiscardStagedVideoMode, below its IsVideoModeChangePending() guard)

grep -n "DiscardStagedVideoMode" SiegeGraphicsMenuWidget.cpp
  → 1 definition (:2306) · 3 CALLS · 6 comments
      :1117  NativeDestruct()          teardown catch-all   (TASK-1115)
      :1535  BackPressed()             the Back path        (TASK-1115)
      :1853  RevertSettingsPressed()   the button + expiry  (TASK-1118)

grep -c "GameUserSettings\|ApplyResolutionSettings" SiegeGraphicsMenuWidget.cpp
  → 6, ALL SIX COMMENTS. ⛔ Zero direct engine calls: GFX-§3's seam clause holds.
```

**⇒ ONE revert function, THREE callers, and still ONE revert on the Back path.** The arithmetic is `TASK-1115`'s and is unchanged, because the guard is on **state**: `BackPressed`'s call reverts and closes the window, and `NativeDestruct`'s call then finds nothing pending and returns `false`. `RevertSettingsPressed` is **not on the Back path** — `Back` does not call it and cannot: pressing Back is not pressing Revert.

**The third caller is not the "third site" cl. (8) forbids.** That clause governs *the Back path*, and the count there is 1. A revert button that could not call the class's one revert function would have to hand-roll a second — which is the thing actually being prohibited.

⚠️ **AND A HONEST LIMIT ON THAT PROOF.** A *redundant* `DiscardStagedVideoMode` call anywhere reddens **no test**, because the guard makes a second call a no-op by design. "Exactly one revert" is proved by the **greps above**, not by a mutation. A test cannot see a call that does nothing. This is recorded in the test file's own mutation table under "what no mutation here can reach", clause (ii).

---

## 4. THE EXPIRY PATH, AND WHY IT CANNOT DOUBLE-APPLY

**Clause (9), discharged negatively and exactly.** I re-read `RevertVideoModeChange` **to its terminator** (`SiegeGraphicsSettingsSubsystem.cpp:900–925`) after `TASK-1113`'s loop-1 rewrite: it still does `Settings->RevertVideoMode()` **then** `ApplyResolutionSettings(false)`, in that order, both under the `⛔ DO NOT DELETE THIS` marker, then `CloseVideoModeWindow`. **The loop-1 rewrite did not move it.**

- I did **not** unwrap it — there is no `UGameUserSettings::RevertVideoMode()` call in the widget (see the grep: zero engine calls at all).
- I did **not** duplicate it — there is no `ApplyResolutionSettings` after it, anywhere.
- The expiry does not have its own revert body. `TickVideoModeCountdown` calls **`RevertSettingsPressed()`**, the same function the button calls, which calls the same `DiscardStagedVideoMode`. One body ⇒ a second apply cannot be added to "just the expiry".

Counted by the tests: after an expiry, `ApplyResolutionSettingsCallCount` reads **exactly 1** since the reset (Test 13 step 5). After `Back` mid-countdown, **exactly 1** (Test 16). After the button, **exactly 1** (Test 15).

**Re-entrancy.** `ClearTimer` is called from inside the timer's own callback (tick → revert → disarm). `FTimerManager` defers destruction of the executing timer, so this is legal; the shipped project does the same shape at `BattlefieldScatter.cpp:2011/:2018`.

---

## 5. ⭐ THE ONE DESIGN ADDITION NOBODY ASKED FOR — AND WHY IT IS THE SPEC, NOT EXTRA

**`CanArmVideoModeCountdown()` — no timer manager ⇒ the display change is STAGED and NOT APPLIED.**

`GFX-§4` pairs the provisional apply with the auto-revert because **the apply is only survivable if the revert is guaranteed**. If the widget cannot get a timer, applying anyway ships the permanent lockout through a door nobody is watching. So the panel refuses to apply, logs a `Warning`, says so in `StatusText`, and leaves the change staged — which is `TASK-1115`'s already-shipped, already-tested, safe interim, released by `Back`.

This is unreachable in a running game (a viewport-added widget has a world). It is **highly** reachable in automation, which is exactly why it is testable — and it is the reason the automation seam (`SetVideoModeCountdownDrivenManuallyForAutomationTests`) exists rather than being a convenience: without it every countdown test would silently have been measuring this refusal branch while looking like it covered the expiry.

**The order of the two checks is the safety property**, and it is commented as such: the ability to auto-revert is established **before** anything reaches the display. `M18` is the mutation for the reversal alone, separate from `M17`, because *moving* the guard is the edit a future reader is far likelier to make while tidying than *deleting* it — the function still looks guarded.

---

## 6. TWO PLAYER-FACING STRINGS CORRECTED (both routed, neither invented)

**(a) `DisplayHint` — `qa/TASK-1116.md` WARN-2 ruled it FALSE-ON-SCREEN until this row.** It read *"Display changes are applied when you confirm them"*, which was the honest description of a panel that staged and applied nothing. **With the countdown in the tree the mechanic is the other way round** — applied immediately, **undone** unless confirmed — so leaving the old wording would have replaced one false sentence with another pointing the opposite way. It now reads:

> *"Display changes take effect straight away and then ask you to keep them. If you do not confirm within 10 seconds — or if you cannot see the screen at all — the old mode comes back by itself. Leaving Graphics also undoes an unconfirmed change."*

**⇒ WARN-2 is discharged by CODE, not by ship order.** The panel is now truthful standalone on this sentence.

**(b) `AutoDetectBlocked` — `qa/TASK-1116.md` WARN-3.** The refusal message set `StatusText` to `DisplayHint` (a sentence about *display* changes) at a player who had pressed **Auto-Detect**. The gate ruled the mechanism (F-8) right and the string wrong. **My row also invalidates the advice**: the way out is no longer "press Back", it is the Keep/Revert prompt now on screen. New constant, naming the thing refused and the current way out.

⚠️ **The other five `TASK-1116` WARNs are NOT mine and were not touched**: WARN-1 (Foliage/`TASK-1122`), WARN-4 (FPS counter/manager), WARN-5 + NIT-3/4/5/6 (adopted as **comment-only** corrections to the mutation table — see §8), WARN-7 (handoff/board counts — manager's).

---

## 7. 🚨 HOST-CRITICAL FINDING — `TASK-1122` ALREADY CALLS A NAME THAT DID NOT EXIST UNTIL THIS ROW

Rider **(10a)** renamed `GetFoliageDensityScale()` → **`GetFoliageQualityScale()`** across the three files I own (13 sites: `.h` ×4 incl. 3 prose, `.cpp` ×1, settings test ×8) plus a fourth prose mention in `SiegeGraphicsMenuWidget.h:32` the clause did not name (it postdates the clause).

**Then I greped the tree rather than assuming zero callers, and `TASK-1122` has landed TWO CALLS to the NEW name in `BattlefieldScatter.cpp` — a symbol that did not exist before this edit.**

```
grep -rn "GetFoliageDensityScale" Source/   → 2 hits, BOTH deliberate prose about the rename
grep -rn "GetFoliageQualityScale" Source/   → BattlefieldScatter.cpp ×2  ← TASK-1122's consumer
                                              + my 13 sites + the widget prose
```

⇒ 🚨 **`TASK-1118` AND `TASK-1122` ARE NOW MUTUALLY LOAD-BEARING AT COMPILE TIME. Before this edit, `TASK-1122`'s `BattlefieldScatter.cpp` could not have compiled — it called a symbol with no declaration.** `TASK-1124` must build them **together**, or revert them **together**. `TASK-1122` did the right thing (its board row instructed the new name); the row that owed the declaration is this one, and it is now paid. **I did not touch its files** — this is a read-only census.

Rider **(10b)**: `.h`'s `RefusedSaveWhileVideoModePendingCount` doc said *"how many **saves** were refused"*; it also counts an `AutoDetectQuality` **benchmark** refusal. Widened to "operations", with both sites named — and with a paragraph saying **why this counter and not a boolean** is the lane's instrument, since three separate tests now depend on that reasoning.

---

## 8. TESTS — 7 NEW, ALL IN `Tests/SiegeGraphicsMenuTest.cpp`

| # | Test | What can fail (SC-§79) |
|---|---|---|
| 13 | `VideoModeCountdownExpiryReverts` | ⭐⭐⭐ **the expiry does not revert** — the permanent lockout · a save before Keep · firing early · a stranded tick after expiry |
| 14 | `KeepPersistsTheModeAndSavesOnlyThen` | Keep rolls back · Keep saves nothing · **a queued tick undoes the mode the player kept** · a spurious save on an empty Keep |
| 15 | `RevertRestoresTheModeBeforeAnyChange` | Revert restores the *intermediate* mode · a second press does not restart the timer · a double apply |
| 16 | `BackDuringCountdownRevertsExactlyOnce` | ⭐⭐⭐ **cl. (8): two reverts on the Back path** · a timer outliving the panel · the soft-lock surviving Back |
| 17 | `NoTimerNoApplyAndQualityNeverArms` | ⭐⭐ applying with no auto-revert · **cl. (3):** a quality change arming a prompt · the frame-rate ladder arming one |
| 18 | `SteppingBackToTheStartingModeCancelsTheCountdown` | a prompt left up over a change that no longer exists · an apply spent on a no-op press |
| 19 | `CountdownTextNamesTheNumberAndIsSingularAtOne` | a prompt with no number · "1 seconds" · counting into the negatives |

Plus **4 new assertions in Test 1** (`TreeBuildsEveryPinnedRow`): the four `GFX-§10` pinned names looked up **through the tree**, and the prompt **Collapsed** on a freshly built panel.

**Positive controls first, on the right instrument.** Tests 13 and 16 both prove the soft-lock is **real** (Auto-Detect refused **and counted**) before asserting it is released — and both assert **`RefusedSaveWhileVideoModePendingCount`**, never the bool `AutoDetectQuality()` returns. Under the automation seam that bool is `false` on a broken build and a fixed one alike; only the counter discriminates. That instrument choice was `TASK-1114`'s blocker and `TASK-1115`'s success, and it is inherited here deliberately.

**`SC-§87`:** every test is bounded — the longest loop is 10 iterations (Test 13's ticks) and 50 (inherited). Nothing waits on wall-clock time; the countdown is driven by hand.

### Named mutations (`SC-§83`) — **Addendum B form**, all seven predictions

| M | Mutation | Predicted red |
|---|---|---|
| **M14** | delete `ApplyVideoModeProvisional()` from `BeginVideoModeConfirmation` (still arm) | **≥ 3 rows**, including Test 13's "APPLIED PROVISIONALLY" row and Test 15's "EXACTLY ONE display apply on the revert" |
| **M15** ⭐⭐⭐ | delete the `RevertSettingsPressed()` call at zero in `TickVideoModeCountdown` | **≥ 4 rows, all in Test 13**, including "THE COUNTDOWN EXPIRED … AND REVERTED", "back to what it was", "EXACTLY ONCE", "the prompt is gone". The nine-tick block **stays green**. ⛔ **This is the mutation that ships the permanent lockout** |
| **M16** ⭐⭐ | delete `DisarmVideoModeCountdown()` from `BackPressed()` | **≥ 3 rows in Test 16**, including "with the countdown DISARMED", "a stranded timer callback after Back reverts NOTHING", "still exactly one revert". ⚠️ the ONE-revert row *immediately* after `BackPressed()` **stays green** — the count only goes wrong once a tick arrives, which is why the test fires the timer by hand afterwards |
| **M17** ⭐⭐ | `CanArmVideoModeCountdown()` returns `true` unconditionally | **≥ 2 rows in Test 17**, including "NOT APPLIED" and "no countdown was armed" |
| **M18** | swap the order: apply **before** the can-arm check | **≥ 1 row**, including Test 17's "NOT APPLIED" |
| **M19** | call `BeginVideoModeConfirmation` from `HandleGroupSliderCommitted` | **≥ 2 rows in Test 17**, including "a QUALITY change arms NO confirmation" and "reaches NO provisional display apply" |
| **M20** | drop the disarm from the not-pending branch | **≥ 1 row**, including Test 18's "the countdown is CANCELLED". ⚠️ predicted **narrow by construction** — the facade's state is already right in that case, so only the prompt is wrong. A second red means the disarm does more than hide a widget |

### Four routed prediction corrections, **adopted not invented** (`qa/TASK-1116.md`)

Corrected **in place** in `TASK-1115`'s table, because a table with a known-false row is worse than none — the next reader runs it, counts the "wrong" reds and doubts the *code*:
**M4** → four reds, not two (NIT-3) · **M5** → five, not four (NIT-4) · **M7** → the hedge **does not derive** (NIT-6) · **M10** → the `TestNotEqual` **only** (WARN-5). ⚠️ All four remain predictions; the gate did not execute them either. And the **M13** comment in `SiegeGraphicsSettingsTest.cpp` is corrected from `TASK-1117`'s **witnessed** red — the only measured row in this lane.

### Suite arithmetic — ⛔ A PREDICTION RESTING ON A PREDICTION

`517 / 0` **MEASURED** at `42734b7` (`TASK-1117`) **+ 12** (`TASK-1115`, censused, its `529` **never measured**) **+ 7** (mine, censused) = **536**, ⛔ **UNMEASURED**. ⛔ Never `529`, `517`, `514`, `511`, `306`. `TASK-1122` adds its own and I have not counted them.

### What these tests CANNOT see (declared, not buried)

1. 🚨 **That `FTimerManager` actually calls `TickVideoModeCountdown()`.** A bare `NewObject` widget has **no world**, so no test here creates a timer manager; the ticks are driven by hand. Deleting the `SetTimer` call reddens **nothing in this file**. The mechanism is used identically at a dozen shipped sites (`HeroCharacter.cpp:742`/`:1272`, `GoldNode.cpp:417`, `BattlefieldScatter.cpp:2018`/`:2438`, `CaptureZone.cpp:95`) — but that is precedent, not proof. ⇒ **a code read at `ArmVideoModeCountdown` (QA) and a pixel check (`TASK-1125`: change Window Mode, touch nothing, watch it come back).** This is the single most load-bearing unproven line in the row.
2. A redundant discard call — see §3.
3. Anything visible: whether the prompt is legible, on screen, above the fold, or reachable by a mouse. `GFX-§2(e)`.
4. Whether ten ticks take ten wall-clock seconds.

---

## 9. FLAGGED DECISIONS FOR `TASK-1119`

| # | Decision | Reasoning |
|---|---|---|
| **F-1** ⭐⭐ | **`RevertSettingsPressed` is a THIRD caller of `DiscardStagedVideoMode`** (Back, teardown, Revert/expiry). | cl. (8)'s invariant is *one revert **on the Back path***, and that count is still 1 — proved by grep in §3. The alternative was a hand-rolled second revert in the countdown, which is what the clause actually forbids. **Rule: is "three callers, one Back-path revert" the right reading of cl. (8)?** I believe yes and `TASK-1115`'s own header sanctions it (*"call this one"*), but it is the row's biggest judgement call. |
| **F-2** ⭐⭐ | **`qa/TASK-1116.md` WARN-6 declined: I did NOT gate the facade's revert-apply on "a provisional apply actually happened".** | The gate asked me to *consider* it. After this row, **every** revert on the panel's paths follows a provisional apply, so the condition would be constant-true in practice — while adding a branch that can **silently skip the one line that makes a revert real** (`SC-§94` cl. A). The residual case WARN-6 names (staged-but-never-applied, then Back) now only arises on the unreachable no-timer branch, where `RevertVideoModeChange` returns `false` at the same null check anyway. **One-place invariants stay one-place** (manager's RULING B). |
| **F-3** ⭐ | **The no-timer branch leaves the mode STAGED rather than discarding it**, so the pending state survives until `Back`. | Discarding there would add a **fourth** `DiscardStagedVideoMode` call site to handle a branch unreachable in a running game, against an explicit "do not add sites". The staged state is `TASK-1115`'s shipped, tested, `Back`-released interim. **Asserted** in Test 17. |
| **F-4** | **`DisplayHint` rewritten** (§6a) — I edited a string `TASK-1115` shipped. | WARN-2 ruled it false-on-screen; my row inverts the mechanic, so the old wording becomes false in the *other* direction. Discharging a routed WARN by code beats discharging it by ship order. **Rule: is the new sentence right?** |
| **F-5** | **`AutoDetectBlocked` added** (§6b) — WARN-3's proposed fix, which nobody was named to execute. | The old advice ("press Back") is **wrong after this row**; the way out is now Keep/Revert. It had to change for correctness, not tidiness. |
| **F-6** | **Tests live in `SiegeGraphicsMenuTest.cpp`, not the board's `SiegeGraphicsSettingsTest.cpp`.** | The harness is there and the code under test is the widget. §1. |
| **F-7** | **The prompt sits inside `RootScrollBox`, under the steppers, plus `ScrollWidgetIntoView`** — not as an overlay pinned above the scroll. | An overlay would mean restructuring `TASK-1115`'s shipped spine (`BackdropBorder → PanelSizeBox → RootScrollBox`). Siting it where the player just clicked, and scrolling to it, covers the case that matters; the case it does not cover (player cannot see at all) is covered by the **timer**, not the prompt. **Rule: acceptable, or does the prompt need to be modal?** |
| **F-8** | **`KeepSettingsLabelText` / `RevertSettingsLabelText` are unpinned**, derived by the file's `<Base>Button` / `<Base>LabelText` rule. | `GFX-§10` pins the two buttons and not their labels. Consistent with `TASK-1116` F-7's ruling that derived names stay unpinned. |
| **F-9** | **Keep/Revert are in `SetAllControlsEnabled`'s disabled list.** | "Every interactive control in one call" stays true. Reachable only if the subsystem vanished mid-countdown, where the timer still reverts (the safe outcome) and Keep would have failed at the facade's null check anyway. |
| **F-10** | **`TierDFoliageDensityTable` (a test NAME) still says "Density".** | (10a) named the function + three prose mentions; a test-name change is a **suite-listing** change and the clause said *zero behaviour change*. Declared so it is a decision rather than an oversight. **Manager's if it wants the name too.** |
| **F-11** 🚨 | **`TASK-1118` and `TASK-1122` must now ship or revert TOGETHER** — §7. | Measured, not predicted: `BattlefieldScatter.cpp` calls `GetFoliageQualityScale()` ×2, a symbol this row created. **This is `TASK-1124`'s to act on.** |

---

## 10. WHAT THE HOST (`TASK-1124`) MUST DO

1. ⛔ **Compile with the editor CLOSED** and parse the log for `Result:` — never `$LASTEXITCODE`.
2. **Build `TASK-1118` + `TASK-1122` together** (F-11). And `TASK-1116` §7 already ruled: do not commit `TASK-1115` alone — with this row landed, its WARN-2 sentence is **fixed in code**, so the remaining condition is WARN-1 (Foliage), which is `TASK-1122`'s.
3. Report the suite as an **executed `N / M`**. `536` is a prediction resting on an unverified `529`; only your run counts (`SC-§95`).
4. If you exercise the mutations: **M15** and **M16** are the two that matter. M16's red does **not** appear until the by-hand ticks after `BackPressed()` — if it reddens *immediately* after Back, my reading of the disarm is wrong and I want to know.
5. ⛔ Do not "tidy" the two-line order at the top of `DisarmVideoModeCountdown` or the two-check order in `BeginVideoModeConfirmation`. Both are commented as load-bearing; both are the row's safety properties.

---

# § LOOP 1 — the gate `TASK-1119` FAILED this row (1 BLOCKER · 5 WARN · 5 NIT)

**Instant:** 2026-09-07 · **Report:** `.claude/pipeline/qa/TASK-1119.md` · **Loop 1 of a limit of 3.**
🚨 **NO WITNESSED RED.** Nothing in this loop was compiled, executed or rendered. Every red below is a **derived prediction**, exactly like loop 0's. `TASK-1124` holds the compiler.

## L1.0 ⭐⭐ THE BLOCKER, AND THE ONE SENTENCE THAT MATTERS

**The gate is right, and I re-derived it independently before touching anything.** `IsVideoModeChangePending()` is
`bVideoModeChangePending || SiegeGraphicsInternal::HasUnconfirmedVideoModeDifference(ResolveSettings())`
(`SiegeGraphicsSettingsSubsystem.cpp:885-888`). `ApplyVideoModeProvisional()` sets that bool `true` (`facade.cpp:870`) and **only `CloseVideoModeWindow` clears it** (`facade.cpp:927`), whose only callers are `ConfirmVideoModeChange` and `RevertVideoModeChange`. ⇒ **the moment one press applies, the left-hand operand is stuck true**, the cancel branch can never be entered, and press 3 of 3 on Window Mode re-applied and restarted the ten seconds.

⛔ **I read a name as if it were a comparison, and then wrote three artefacts from that reading — a comment, a handoff paragraph and a test — all of which agreed with each other and none of which agreed with the code.** The one durable lesson is the gate's and I am recording it in my own words: **`SC-§97` cl. 7's read-to-the-terminator duty applies to PREDICATES, not just to functions with side effects.** I discharged it perfectly against the facade's *revert* (cl. 9) and not at all against the facade's *predicate*, because the revert looked dangerous and the predicate looked like a question.

## L1.1 THE FIX — five files, and it is two halves, not one

⚠️ **`SC-§101`: the gate's diagnosis is measured; the fix SHAPE below is MY hypothesis.** My acceptance is **M20 and M21 reddening under the named mutations**, not conformance to what the gate sketched. Two places I deliberately deviated from its sketch are named in **L1.4**.

### (a) The facade grows the un-latched half as its own name — `SiegeGraphicsSettingsSubsystem.{h,cpp}`

```cpp
// facade.cpp:885-896
bool USiegeGraphicsSettingsSubsystem::IsVideoModeChangePending() const
{
    return bVideoModeChangePending || HasUnconfirmedVideoModeDifference();   // <- the || made VISIBLE
}

bool USiegeGraphicsSettingsSubsystem::HasUnconfirmedVideoModeDifference() const
{
    return SiegeGraphicsInternal::HasUnconfirmedVideoModeDifference(ResolveSettings());
}
```

`UFUNCTION(BlueprintPure)`, declared at `facade.h:601`. The file-static is now computed in **one** place and `IsVideoModeChangePending()` ors the latch onto it rather than repeating it. The header on **both** functions now says in capitals that the first is *an `||` with a latch on the left, not a comparison*, and that the two are **not interchangeable in either direction** — with the reason for each direction.

### (b) The branch asks the question it means to ask — `SiegeGraphicsMenuWidget.cpp:1660-1672`

```cpp
if (!Graphics->HasUnconfirmedVideoModeDifference())
{
    DisarmVideoModeCountdown();
    DiscardStagedVideoMode(Graphics);
    UE_LOG(LogSiegeGraphics, Verbose, TEXT("[GraphicsMenu] The display change came back to the confirmed mode - the confirmation window is closed and any running countdown is cancelled."));
    return false;
}
```

🚨 **THE DISCARD IS THE HALF A "JUST FIX THE PREDICATE" PATCH WOULD HAVE MISSED, AND IT IS THE MORE DANGEROUS HALF.** With the predicate fixed and the discard absent, the branch would hide the prompt and **leave the latch up with nothing left to lower it** — `bVideoModeChangePending` stays `true`, the facade refuses **every save and Auto-Detect for the rest of the session**, and the panel looks completely correct while doing it. That is cl. (8-WIDENED)'s soft-lock arriving through a door opened by the fix for a different defect. `DiscardStagedVideoMode` reverts to a mode **identical** to the staged one (nothing visible moves), closes the window and flushes any deferred save. **It is pinned by M22.**

### THE CORRECTED COMMENT TEXT (verbatim, `SiegeGraphicsMenuWidget.cpp:1635-1667`)

```
// -- (1) IS THERE ANYTHING LEFT TO CONFIRM? -------------------------------
// A stepper press can land the player back on the mode they started in -
// three presses of ">" on Window Mode wraps all the way round. Leaving the
// prompt up over that would offer "Revert" for a change that no longer exists
// and "Keep" for a mode that was never left.
//
// 🚨 THE PREDICATE HERE IS HasUnconfirmedVideoModeDifference(), ⛔ NOT
// IsVideoModeChangePending(). The latter is `bVideoModeChangePending || <that
// difference>` and the left-hand half is a LATCH: the first provisional apply
// below raises it and only the facade's CloseVideoModeWindow lowers it.
// Reading it here made this branch DEAD from the second press onward - the
// wrap-around press would see "pending", re-apply the mode the player is
// already on and RESTART the ten seconds instead of putting the question away
// (qa/TASK-1119.md BLOCKER-1: a name read as if it were a pure comparison).
//
// ⛔ AND THE DISCARD IS NOT OPTIONAL - DISARMING ALONE WOULD BE A NEW SOFT-
// LOCK. The latch would stay up with no prompt and no countdown left to lower
// it, so the facade would refuse every save and Auto-Detect for the rest of
// the session (cl. 8-WIDENED's failure, arriving through a new door).
// DiscardStagedVideoMode reverts to a mode IDENTICAL to the staged one -
// nothing the player can see moves - closes the window and flushes any save
// deferred while it was open.
//
// Order: disarm THEN discard, the same order Keep, Revert, Back and the
// teardown all use. The countdown is dead before any state moves.
```

### THE CORRECTED HANDOFF SENTENCE — **struck in place, not deleted**

§0 item 5 above reads, and I am leaving it on the page so the next reader can see what was believed:

> ~~5. Pressing `>` again **restarts** the ten seconds. Pressing `>` until the value wraps back to the mode they started in **cancels the question** rather than asking about a change that no longer exists.~~

⛔ **FALSE AS WRITTEN AT LOOP 0 — the second sentence described a branch that could not run.** It is now true, and it does more than it claimed:

> **5. Pressing `>` again restarts the ten seconds. Pressing `>` until the value wraps back to the mode the player started in cancels the question — the countdown is disarmed, the prompt goes away, AND the facade's confirmation window is closed, so the save refusal that came with it is released. At loop 0 this branch was gated on `IsVideoModeChangePending()`, whose left operand is latched by the first provisional apply, so from press two onward the branch was dead and the wrapping press re-applied and restarted the ten seconds instead.**

⚠️ **One consequence of the fix I am declaring rather than burying:** the wrap-around now **flushes a save** (`CloseVideoModeWindow` → `RequestSaveSettings`). What it writes is the **last confirmed** mode — the one the player is looking at — which is precisely what `Revert` and `Back` already write today on the identical path. `GFX-§4` cl. (4) forbids saving an **unconfirmed** mode; after the wrap there is no unconfirmed mode left, which is the whole reason the branch fired. Test 18 asserts zero saves **before** the wrapping press, and the release **after** it.

## L1.2 ⭐ THE PREDICTIONS, ADDENDUM B FORM — M20's new red, plus two new mutations

**⛔ Every row below is DERIVED, NOT WITNESSED.** The two new ones exist because M20's absent red was never a bad prediction — it was **BLOCKER-1 seen from the mutation side**, and a mutation with no red is evidence about **reachability** before it is evidence about an assertion.

| M | mutation | prediction (Addendum B) |
|---|---|---|
| **M20** ⛔ *was NO RED* | drop `DisarmVideoModeCountdown()` from the wrap-around branch | **≥ 2 rows**, and I expect exactly two, both in `SteppingBackToTheStartingModeCancelsTheCountdown`: **"the branch reads the DIFFERENCE, not the LATCH … CANCELLED"** and **"the remainder is zeroed with it"**. ⚠️ The stranded-tick row is **not** among them — an un-disarmed countdown still holds 10.0 s and two ticks reach 8.0. Everything the **facade** holds stays right, because the discard still runs. |
| **M21** 🚨⭐⭐⭐ *new — the fix's own mutation* | change the branch condition back to `!Graphics->IsVideoModeChangePending()` | **≥ 4 rows**, and I expect exactly four, all in Test 18: **"the branch reads the DIFFERENCE, not the LATCH"** (re-armed ⇒ active) · **"the remainder is zeroed with it"** (re-armed ⇒ 10.0) · **"AND THE FACADE'S WINDOW IS CLOSED"** (latch still up) · **both release rows** — "Auto-Detect is no longer refused" (counter 1) and "a quality change SAVES again". ⛔ **The apply-count row STAYS GREEN and that coincidence is worth knowing: `ModeCount−1` provisional applies + one MORE provisional apply is the same number as `ModeCount−1` + the closing revert's apply.** A count alone cannot tell the fixed branch from the broken one. |
| **M22** ⭐⭐ *new — the second half* | delete `DiscardStagedVideoMode(Graphics)` from the wrap-around branch, keep the disarm | **≥ 4 rows**, exactly four in Test 18: **"AND THE FACADE'S WINDOW IS CLOSED"** · the apply count (`ModeCount−1`, not `ModeCount`) · **"Auto-Detect is no longer refused"** · **"a quality change SAVES again"**. ⛔ **The two prompt rows stay GREEN, which is the danger: the panel LOOKS completely correct while every save is refused for the session.** |

## L1.3 EVERY WARN AND NIT — acted on, or declined with a reason

| # | verdict | what I did, and why |
|---|---|---|
| **BLOCKER-1** | ✅ **FIXED (a)** | L1.0–L1.2. I took the gate's **(a) CODE** over **(b) TEST+PROSE** for its own stated reason — (b) ships a prompt asking about a change that no longer exists — and because (b) would have left the wrap-around **latching the save refusal until the panel closes**, which is worse than the defect it replaces. |
| **WARN-1** (M16 over-broad + the instruction inverted) | ✅ **FIXED, and I found the gate's suggested strengthening does NOT work** | M16 restated as **≥ 2 rows, both IMMEDIATELY after `BackPressed()`**, and the second row is **new**: `TestEqual(GetVideoModeCountdownSecondsRemaining(), 0.0f)` — the disarm zeroes the remainder as well as lowering the flag, so a missing disarm reads `10.0` on the spot. ⛔ **The gate proposed driving TEN ticks after Back to make the stranded-tick half load-bearing. I derived that it would still not redden:** at zero the stranded tick calls `RevertSettingsPressed` → `DiscardStagedVideoMode`, whose `IsVideoModeChangePending()` guard finds the window **already closed by Back's own revert** and returns `false`. The second revert cl. (8) forbids is prevented **one layer down**, so **no tick count** can make those rows load-bearing against M16. Recorded in the test file's clause (ii). **And the author's own question is answered: yes, it reddens immediately, and only there.** |
| **WARN-2** (M14 over-broad) | ✅ **FIXED (comment)** | Restated as **≥ 1 row, exactly one**: `test:1177-1178`. The named Test 15 row stays green because a counter reset precedes it. ⭐ The corollary is recorded in the table because it is worth more than the correction: **exactly one assertion in that file would notice the provisional apply going missing.** |
| **WARN-3** (M19 inert) | ✅ **RE-SPECIFIED** | Kept the original **as a recorded negative** (it takes the cancel branch and returns; the branch's own harmlessness is what made a mutation aimed past it look alive), and replaced it with a **discriminating pair, one per property Test 17(b) asserts**: **M19a** call `ArmVideoModeCountdown()` from `HandleGroupSliderCommitted` ⇒ the "arms NO confirmation" row; **M19b** call `ApplyVideoModeProvisional()` from the same handler ⇒ the "reaches NO provisional display apply" row. |
| **WARN-4** (M20 no red) | ✅ **FIXED** | Free with BLOCKER-1, plus the new remainder assertion widens it to two. L1.2. |
| **WARN-5** (`CountdownUnavailable` says "Nothing was changed" beside the changed value) | ✅ **FIXED** | Now: *"…Nothing on your screen was changed — your choice is staged, and leaving Graphics will discard it."* True of **both** halves: the picture did not change, the choice did. The comment names why (`StepWindowMode` stages **before** the call and `RefreshAllRows()` runs **after** it). |
| **NIT-1** (`DisplayHint` hardcodes "10 seconds") | ✅ **TIED DOWN AT BOTH ENDS, not composed** | I did **not** make the hint a runtime-composed string: that block is deliberately a table of plain literals and turning one entry into a function to defend a number `GFX-§4` pins would cost more than it buys. Instead **both ends now carry the ⛔ note** — the string's doc and `VideoModeConfirmSeconds`'s declaration each name the other and say that **nothing in the suite or the compiler can see them disagree**. Declared as a judgement, not a silent skip. |
| **NIT-2** ("×2" is a hit count) | ✅ **CORRECTED** | §7 and F-11 above said `BattlefieldScatter.cpp` calls `GetFoliageQualityScale()` **×2**. ⛔ **Measured again this loop: it is ONE CALL (`:406`) + ONE COMMENT (`:401`).** The ship-together ruling is **unaffected** — one call is enough — but the host acts on this artefact, so: **one call.** |
| **NIT-3** (asset-authored tree never gets the `Collapsed` default) | ✅ **FIXED** | Added the collapse to **`SeedAndBind()`**, which runs on **both** tree paths (`BuildVideoModeConfirmRow` is skipped wholesale by `ConstructGraphicsTree`'s condition-(b) guard). `SeedAndBind` has exactly **one** caller, `NativeConstruct`, so nothing live can be hidden by it. ⭐ **And I rewrote the invariant rather than just the number:** the header no longer counts writers, it states the property that matters — **four writers, and exactly ONE of them can make the prompt Visible.** The three `WRITER n OF 3` comments are renumbered to `OF 4`. |
| **NIT-4** ("4 new assertions in Test 1"; it is 6) | ✅ **CORRECTED** | §8 above says 4. ⛔ **It is 6** (`test:303`, `:304`, `:305`, `:306` pinned-name lookups · `:314-315` the Collapsed check · `:317-318` "no countdown on an untouched panel"). Wider than claimed. |
| **NIT-5** (`AutoDetectBlocked`'s advice false on two branches) | ✅ **FIXED** | Now: *"…Choose Keep or Revert above — or wait for it to revert by itself, **or leave Graphics** — then try again."* True on every path it can appear on, including the two staged-with-no-countdown branches. The ⛔ note says which two and forbids dropping the clause. |
| **The `SetTimer` assertion the loop asked for** | ⛔ **DECLINED, and this is the honest answer** | **No cheap honest assertion exists.** "The handle is valid after arming" cannot be reached: the automation seam that makes every other countdown test possible **returns before the `SetTimer` block**, and the only other way in needs the world a `NewObject` widget does not have. Adding a handle getter would buy exactly one assertion — *"the handle is INVALID while driven manually"* — which is a **green row proving the seam works and nothing about the real timer**, i.e. an artefact that reads as coverage and is not. I did not contrive a world. ⛔ **The observation stays owed to 🧑 Jonathan at `TASK-1125`: change Window Mode, touch nothing, watch it come back.** Recorded in the test file's clause (i) in those words. |
| **F-2 / F-3 / F-6 / F-7 / F-8 / F-9 / F-10 (upheld)** | — | Ruled upheld by the gate; **untouched**, per the instruction not to churn what verified clean. F-10 stays routed to the manager. |

## L1.4 ⚠️ WHERE I DEVIATED FROM THE GATE'S SKETCHED FIX (`SC-§101` — the shape was mine to choose)

1. **Order inside the branch.** The gate wrote *"call `DiscardStagedVideoMode(Graphics)` … and then disarm."* I do **disarm then discard**, because that is the order **all four** other exits use (`Keep`, `Revert`, `Back`, `NativeDestruct`) and a single house order is what makes *"the countdown is dead before any state moves"* checkable at a glance. Behaviourally identical — both run on the game thread inside one call, and a timer callback cannot interleave between the two statements.
2. **`IsVideoModeChangePending()` was NOT replaced everywhere.** It is still the predicate at the save refusal (`facade.cpp:1199`), at `AutoDetectQuality`'s guard (`:522`) and in `DiscardStagedVideoMode`'s guard (`widget:2378`) — and each of those now carries a ⛔ note saying the **latched** one is required there. **The two predicates are not interchangeable in either direction**, and swapping the discard's guard to the un-latched one would strand the facade's window exactly as M22 describes. That is the trap this fix opens and closes in the same edit.
3. **M16 was strengthened differently from the gate's suggestion** — see WARN-1 above; the suggestion derives green.

## L1.5 THE COUNTS, RE-MEASURED THIS LOOP (not carried forward)

```
DiscardStagedVideoMode  in SiegeGraphicsMenuWidget.cpp -> 1 definition (:2370) + 4 CALLS
      :1140 NativeDestruct · :1573 BackPressed · :1668 the wrap-around branch · :1917 RevertSettingsPressed
RevertVideoModeChange   in SiegeGraphicsMenuWidget.cpp -> 4 hits, STILL EXACTLY ONE CALL (:2406)
GameUserSettings|ApplyResolutionSettings in the widget -> 6 hits, ALL SIX COMMENTS
BeginVideoModeConfirmation                             -> STILL EXACTLY 2 CALL SITES (:2151, :2188)
HasUnconfirmedVideoModeDifference over Source/         -> 1 decl + 1 def + 1 facade caller + 1 widget caller + 2 test uses
                                                          ZERO uses at the save refusal or in the discard's guard
IMPLEMENT_*_AUTOMATION_TEST over Source/                -> 545
```

🚨⭐⭐ **THE BACK PATH IS STILL EXACTLY ONE REVERT, and the fourth caller does not change it.** The wrap-around branch is reached from a **stepper press**, not from Back; and if it has fired, it has already closed the window, so a subsequent `Back` finds the guard shut and reverts **zero** times — correct, because there is nothing staged. cl. (8)'s invariant is **one revert per Back press**, not one call site, and what it forbids is a **hand-rolled second revert body** — which is exactly what calling this one function from four places avoids (the gate's own F-1 ruling).

## L1.6 SUITE — ⛔ THE ONLY MEASURED BASELINE IS `517 / 0`

**`545`** `IMPLEMENT_*_AUTOMATION_TEST` declarations across 46 files, **censused in the working tree this loop** and matching the gate's independent count exactly: `545 = 517` **MEASURED** at `42734b7` `+ 12` (`TASK-1115`) `+ 7` (mine) `+ 9` (`TASK-1122`'s `Tests/SiegeScatterCullBandTest.cpp`).
⛔ **A declaration census is not a run.** ⛔ **Never quote `536`, `529`, `514`, `511` or `306` as measured.** This loop adds **zero new tests** — only assertions — so the number is unchanged from the gate's.
⇒ **Expect `545 / 545` once this loop lands**, where the gate expected `544 / 545` on the loop-0 bytes.

## L1.7 FILES WRITTEN THIS LOOP (5) — and the fences

| File | What changed in loop 1 |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsSettingsSubsystem.h` | `HasUnconfirmedVideoModeDifference()` declared (`:601`) · `IsVideoModeChangePending()`'s doc now names the **latch** in capitals and points at the sibling |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsSettingsSubsystem.cpp` | the new definition (`:890`) · `IsVideoModeChangePending()` now ors the latch onto it (behaviour identical, one computation site) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.h` | the state-machine diagram's branch line · the writers invariant (four writers, **one** can show it) · the `VideoModeConfirmSeconds` ⛔ tie-down · the `DiscardStagedVideoMode` caller note (**four**, Back path still one) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.cpp` | **the blocker fix** (`:1660-1672`) · the corrected comment · the discard guard's ⛔ "keep the latched predicate here" note · `CountdownUnavailable` (WARN-5) · `AutoDetectBlocked` (NIT-5) · `DisplayHint`'s NIT-1 note · `SeedAndBind`'s collapse (NIT-3) · writer renumbering |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeGraphicsMenuTest.cpp` | **Test 18 rewritten** (positive controls before the wrap; both halves of the fix pinned; the release asserted on the counter) · one assertion added to Test 16 (WARN-1) · mutation table: M14/M16/M19/M20 corrected in place, **M21 + M22 added**, clauses (i) and (ii) extended |

⛔ **ZERO** `Content/**` · **ZERO** `Config/**` · **ZERO** `.uasset` · **ZERO** `CONVENTIONS.md` · **ZERO** `TASKBOARD.md` beyond my own single `- status:` line · **NO compile, NO editor, NO MCP, NO git.**
⛔ **`Tests/SiegeGraphicsSettingsTest.cpp` was NOT touched this loop** (loop 0's rider work stands unchanged). ⛔ **`TASK-1122`'s files were not touched, and the `GetFoliageQualityScale` rename was NOT reverted** — `BattlefieldScatter.cpp:406` still needs it, and `TASK-1124` must build the two rows **together**.

## L1.8 WHAT THIS LOOP STILL DOES NOT PROVE

1. 🚨 **That `FTimerManager` ever calls `TickVideoModeCountdown()`.** Unchanged, and deliberately not closed here — see the decline in L1.3. **Deleting the `SetTimer` call would still redden nothing.** Owed to `TASK-1125`.
2. That any of M20/M21/M22 actually reddens. They are derivations; `TASK-1124` holds the compiler, and **M21 is the one to run if only one is run** — if it does not redden, this fix is not doing what this row believes it is doing.
3. Anything visible: whether the prompt is legible, above the fold, or reachable by a mouse (`GFX-§2(e)`, F-7's residual).
4. That ten ticks take ten wall-clock seconds.
