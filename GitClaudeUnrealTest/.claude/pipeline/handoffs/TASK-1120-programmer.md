# TASK-1120 — GFX-FPS — handoff (gameplay-programmer)

**Marker:** `TASK-1120-GFX-FPS` · **Date:** 2026-09-07 · **Gate:** `TASK-1121` (qa-reviewer) · **Host:** `TASK-1124`
**Spec source:** the board row cl. (1)–(6) · `handoffs/TASK-1115-programmer.md` · `handoffs/TASK-1118-programmer.md` (incl. § LOOP 1) · `qa/TASK-1116.md` · `qa/TASK-1119.md` (both loops)
**Law:** `GFX-§7` (both clauses) · `GFX-§3` (the ONE named exception) · `GFX-§10` (three pinned names) · `GFX-§2` · `GFX-§11` · `SC-§33` `SC-§79` `SC-§83` + Addendum B · `SC-§87` `SC-§90` `SC-§91` `SC-§94` `SC-§101`.

🚨 **NO WITNESSED RED.** Nothing in this row has been compiled, executed or rendered. Every mutation in §6 is a **derived prediction**, and this lane holds standing proof that a confident prediction can be wrong (`TASK-1113`'s M4; `TASK-1119` loop 0 re-derived four of seven rows differently; `TASK-1119` loop 1 then overturned its own WARN-1). The compiler and the suite are `TASK-1124`'s.

---

## 0. WHAT 🧑 JONATHAN SEES, AND WHERE

**In the Graphics panel** (Main Menu → Settings → Graphics), at the very top of the column, above Auto-Detect:

> **`62 FPS  ·  16.1 ms`**
>
> ☐ Show FPS counter during a match
> *The number above is measured HERE, in the menu — the battlefield costs far more. Turn this on to see the real frame rate in the top-right corner during a match, which is where these settings should be judged.*

**In a match** (`L_Arena`), when that box is ticked: the same two numbers on a dark plate in the **top-right corner**, updating twice a second, for the whole match.

⛔ **THE HINT LINE IS NOT DECORATION — IT IS `GFX-§7`'s ARGUMENT ON SCREEN.** Without it a player reasonably assumes the number at the top of the panel is the number to tune against, and tunes a battlefield against a menu.

---

## 1. 🚨 THE LOAD-BEARING HALF: HOW IT IS OBSERVABLE IN `L_Arena`, NOT ONLY IN THE MENU

This is the clause the row exists for, so it is answered first and mechanically.

| Question | Answer | Site |
|---|---|---|
| What creates it? | `ASiegePlayerController::TryInitFrameRateCounter()`, called from `BeginPlay` **immediately after `TryInitHUD()`** | `SiegePlayerController.cpp:348`, `:409` |
| Is that the controller `L_Arena` actually uses? | ✅ **Yes, measured** — `ASiegeGameMode` sets `PlayerControllerClass = ASiegePlayerController::StaticClass()` | `SiegeGameMode.cpp:38` |
| What is it? | `USiegeFrameRateCounterWidget`, a code-authored `UUserWidget` added to the **viewport** at **ZOrder 30** | `SiegeGraphicsMenuWidget.h` (2nd UCLASS) |
| Why ZOrder 30? | Above the HUD (`0`), the victory screen (`10`) and the war map (`25`). ⛔ A counter the victory screen covers vanishes at the exact moment a player compares this match's frame rate to the last one's | `SiegePlayerController.h` `FrameRateCounterZOrder` |
| Can it eat a click? | ⛔ **No.** Its root Border **fills the viewport** (that is how it aligns to the corner), so it is `ESlateVisibility::HitTestInvisible` — "neither I nor my children are hit-testable". ⚠️ This is the **opposite** of `GFX-§2(f)`'s ruling for the menu backdrop, which is `Visible` *so that* it absorbs clicks. Same enum, opposite requirement | `ConstructCounterTree()` |
| Server-side copies? | ⛔ `IsLocalController()` guard, like the HUD. A server copy of a remote client's PC creates nothing | `TryInitFrameRateCounter()` |
| Teardown | `EndPlay` → `RemoveFromParent()` → the widget's own `NativeDestruct` unbinds the delegate and clears the timer | `SiegePlayerController.cpp:532` |

⇒ **The number Jonathan tunes against is measured with ~340 scatter trees, ~24k grass instances, volumetric fog and Lumen all on screen.** The panel readout is the sanity check; this is the instrument.

**The in-match HUD site, named as the board requires:** `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.{h,cpp}` — one new private method, one ZOrder constant, one `UPROPERTY(Transient)` member, one forward declaration, one include, one `BeginPlay` call, one `EndPlay` block. ⛔ Nothing else in that 7,300-line file is touched.

---

## 2. THE MEASUREMENT — AND THE COST OF THE COUNTER ITSELF

### 2.1 What was rejected, and why (`SC-§91` — measured from the engine source, not inferred)

| Option | Verdict |
|---|---|
| **(a) Tick and count frames ourselves** | ⛔ **Rejected by the board in so many words.** "A tick that exists only to print a number is a cost the number then reports." |
| **(b) `GAverageFPS` / `GAverageMS`** | ⚠️ **Real, and rejected — recorded so nobody re-proposes it.** They exist and are maintained every frame whether we look or not (`UnrealEngine.cpp:798-799`, `:824`, `:829` — an EMA with a `0.1` smoothing factor; the numbers `stat fps` prints). ⛔ **But they are declared `extern ENGINE_API` in PRIVATE `.cpp` files only** — I grepped `Runtime/Engine/Public/` and **no public header declares either** — so using them means re-declaring an engine global by hand, and inheriting a smoothing constant we did not choose (≈10 frames ≈ 0.16 s, not the ~0.5 s the board asked for). |
| **(c) `stat fps` / `stat unit`** | ⛔ **Rejected: not a shippable player feature.** The stats subsystem is compiled out of a Shipping build. It is also a debug overlay, not a menu-toggled preference. |
| **(d) ✅ `GFrameCounter`** | **TAKEN.** `CoreGlobals.h:532`, `extern CORE_API uint64` — **public**. Incremented **once per engine tick, unconditionally**, at the end of `FEngineLoop::Tick` (`LaunchEngineLoop.cpp:6130-6131`, comment *"Increment global frame counter. Once for each engine tick."*), in **every build configuration**. |

### 2.2 The arithmetic

`FSiegeFrameRateSample` (a plain, non-reflected struct at the top of `SiegeGraphicsMenuWidget.h`):

- On each timer firing: read `GFrameCounter` and `FPlatformTime::Seconds()`, subtract the previous pair, publish `frames ÷ seconds` and `seconds × 1000 ÷ frames`, then move the anchor.
- ⛔ **It is a box average over the window, not an EMA** — over 0.5 s it is *exactly* "frames drawn ÷ seconds elapsed", which is what a player means by FPS. An EMA would be smoother and would lie about the size of a stutter.
- ⛔ **Both numbers come from the same division**, so ms is exactly `1000 / fps` and the two units cannot disagree.
- ⛔ **Three refusals, and none is hypothetical:** `SecondsElapsed <= 0`, `!FMath::IsFinite(SecondsElapsed)`, and **`FramesElapsed == 0`** — a minimised window, an alt-tab, a hitch longer than the window. Dividing by zero frames puts `inf` on the HUD **at exactly the moment the player is staring at it to find out what went wrong**. On refusal **both outputs are left untouched** and the caller keeps its last honest text: "0 FPS" and "no reading" are different claims and only one of them is true.
- ⛔ **A backwards counter is refused, never wrapped** — unsigned `Now - Last` on an out-of-order pair would give ~1.8e19 frames and print a spectacular lie.
- ⭐ **The anchor advances even on a refusal.** Otherwise one stalled half-second would leave a stale anchor and **halve the readout for as long as the panel stayed open**. `M25` pins exactly this.

### 2.3 The cost, stated as a number

| State | Per-frame work | Per-second work |
|---|---|---|
| Counter **OFF** (the default) | **zero** | **zero** — `Collapsed` **and** no timer at all. Slate skips a collapsed widget in layout *and* paint. |
| Counter **ON** | **zero** | 2 timer firings × (2 global reads + 1 division + 1 `SetText`) |
| Graphics panel open | **zero** | 2 timer firings, same body |

⛔ **The widget never ticks in either state**, and that is structural: `UCLASS(meta = (DisableNativeTick))` plus no Blueprint tick and no animations. (`UserWidget.h:117-128`: `EWidgetTickFrequency::Auto` already means "only if a BP tick, a latent action or an animation needs it"; the meta closes the native half. `UUserWidget` itself carries the same meta at `UserWidget.h:279`.)

⇒ **Turning the counter on does not measurably change the thing it counts.** That is the property that makes the number honest, and it is why option (a) was refused.

---

## 3. THE TOGGLE AND ITS PERSISTENCE — THE RECIPE, ALL FIVE STEPS

`GFX-§3`'s **one named exception**: this preference is a UI choice, not hardware, so it lives in the **profile-scoped** `USiegeSettingsSaveGame` and follows the player across machines with their account — not in `UGameUserSettings`.

| Step | Site | Note |
|---|---|---|
| 1 | `SiegeSettingsSaveGame.h` | `UPROPERTY() bool bShowFrameRateCounter = false;` — ⛔ **default OFF** (board cl. 4) |
| 2 | `SiegeSettingsSubsystem.{h,cpp}` | `SettingName_ShowFrameRateCounter` == `FName(TEXT("bShowFrameRateCounter"))` |
| 3 | `SiegeSettingsSubsystem.{h,cpp}` | `IsFrameRateCounterEnabled()` (BlueprintPure) + `SetFrameRateCounterEnabled()` (BlueprintCallable), a **one-line forward** to `ApplyBoolSetting` |
| 4 | `LoadSettingsFromSlot()` | one `ApplyBoolSetting(...)` line, reading the same `Source` |
| **5** | `SaveSettingsToSlot()` | 🚨 `SaveObj->bShowFrameRateCounter = bShowFrameRateCounter;` |

### 🚨⭐ THE RECIPE WAS WRONG, AND FOLLOWING IT EXACTLY IS HOW I FOUND OUT

The board told me to follow the *"HOW TO ADD SETTING #2"* block **"EXACTLY, all four steps"**. I did — and **four steps ship a setting that never persists.**

`SaveSettingsToSlot()` copies the in-memory values onto a **freshly constructed** SaveGame object **field by field**. A steps-1-4 setting compiles, runs, broadcasts, updates the UI and reports a **successful** save — while writing its C++ default to disk every single time. The preference evaporates on the next launch, with **no error and nothing in the log to point at**. ⛔ That is not "unfinished"; it is **wrong while appearing correct** (`SC-§94` cl. A).

- The recipe in `SiegeSettingsSubsystem.h` is now **five steps**, with the failure described so the next author does not have to rediscover it. That file is in my `names:` WRITES list.
- The save log line now names **both** fields, so a reader can see what actually reached disk.
- ⛔ **NO version field and NO migration code** were added — tagged-property serialization already covers it, and adding either would be the defect (board cl. 3).
- The test that catches it is `Siegebound.Settings.FrameRateCounterSaveLoadRoundTrip`, and the shape that makes it catchable is **"load into a SECOND store"**. A test that set the value and re-read the *same* store would pass with step 5 missing — a test that cannot fail for the reason it exists (`TASK-1123` `M8`'s lesson, applied before the gate had to apply it to me).

### The toggle's behaviour

- **Seed, then bind** (`qa/TASK-005` major-2): `RefreshFrameRateCounterRow()` runs before `OnCheckStateChanged` is bound.
- Ticking it calls `SetFrameRateCounterEnabled` — ⛔ **the only write**; the subsystem owns the no-op check, the disk write and the broadcast.
- Both the panel row **and** the in-match counter subscribe to `OnSettingsChanged` and **filter on the token**, so the counter appears/disappears live and the confirm toggle can never move either of them.
- ⛔ **One polarity site for the whole feature:** `USiegeFrameRateCounterWidget::ShouldShowFrameRateCounter(const USiegeSettingsSubsystem*)`. The panel's checkbox and the in-match counter call the *same* predicate, so a checkbox saying "on" while the counter decided "off" is impossible. `nullptr ⇒ false` — the **opposite** polarity to `IsAssistantConfirmEnabled()`'s documented `true` fallback, and for the mirror-image reason: a lookup failure must buy *less* debug UI, never more.

---

## 4. FILES TOUCHED

| File | What |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeSettingsSaveGame.h` | +1 `UPROPERTY` (default `false`) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeSettingsSubsystem.h` | +1 FName, +1 getter, +1 setter, +1 member; **recipe corrected to 5 steps** |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeSettingsSubsystem.cpp` | FName definition, getter, setter, load line, **save line**, 2 log lines widened |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.h` | `FSiegeFrameRateSample`; panel readout + toggle members and API; **new `USiegeFrameRateCounterWidget` UCLASS** |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.cpp` | sampler impl; readout row builder; timer arm/disarm; settings resolver + handlers; the whole counter widget |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.{h,cpp}` | ⭐ **the in-match HUD site** — see §1 |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeSettingsTest.cpp` | +3 tests |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeGraphicsMenuTest.cpp` | +4 tests + a scratch-settings harness |

⛔ **ZERO `Content/**` · ZERO `Config/**` · ZERO `.uasset` · ZERO `CONVENTIONS.md` · no compile, no editor, no MCP, no git, no push.**
⛔ **`BattlefieldScatter.*` NOT TOUCHED** and `GetFoliageQualityScale()` **not reverted** — `TASK-1118`/`TASK-1122`'s mutual compile-time coupling is intact and my diff joins theirs in one commit.

---

## 5. TESTS — **+7**

⛔ **I quote a DELTA, not a total.** The reviewer's own census at his instant was **545** `IMPLEMENT_*_AUTOMATION_TEST` declarations across 46 files (`qa/TASK-1119.md` §6 item 5), built on the **measured** `517 / 0` at `42734b7`. ⇒ **this row adds 7**, floor `545 + 7 = 552`. ⛔ Never `529`, `536`, `514` or `306`.

| # | Test | Bound (`SC-§87`) |
|---|---|---|
| 1 | `Siegebound.Settings.FrameRateCounterSaveLoadRoundTrip` | pure in-process; scratch slot, guarded both ways |
| 2 | `Siegebound.Settings.FrameRateCounterDefaultsOffAndAgree` | same |
| 3 | `Siegebound.Settings.FrameRateCounterDelegateNamesTheRightSetting` | same |
| 4 | `Siegebound.GraphicsMenu.FrameRateSampleMathAndRefusals` | **pure statics — no world, no widget, no clock, no frame** |
| 5 | `Siegebound.GraphicsMenu.FrameRateReadoutRowAndToggleExist` | scratch panel, no Slate |
| 6 | `Siegebound.GraphicsMenu.FrameRateToggleSeedsWritesAndFilters` | scratch panel + scratch settings store |
| 7 | `Siegebound.GraphicsMenu.InMatchCounterFollowsThePreference` | scratch counter widget + scratch settings store |

⛔ **Hermeticity is mechanical, not promised.** The two test files use **different** scratch slot strings (`SiegeSettings_AutomationScratch` and `SiegeSettings_GraphicsMenuScratch`), each with its own delete-on-entry/exit guard, and both are asserted `!=` the shipped `SiegeSettings` slot. No test run can read, write or delete the player's real `Saved/SaveGames/SiegeSettings.sav`.

### ⭐ THE INSTRUMENT LESSON, APPLIED WHERE IT HAD TEETH

`qa/TASK-1119.md` § LOOP 1 (`M21`) and `qa/TASK-1114.md` both found **counters that read identically under a correct and a broken branch**. Everything discriminating in these seven tests therefore asserts **state**:

- the delegate is asserted by **`LastBroadcastSettingName`**, not by `SettingsChangeBroadcastCount`. `M28` (a setter that forwards the *wrong* FName) reddens **two NAME rows and zero count rows** — the count is 3 either way. The tally is kept only as corroboration and is labelled as such in the file.
- the counter is asserted by its **`ESlateVisibility` enum**, not by an "apply ran" tally.
- the preference is asserted by **the store's value**, not by "the handler was called".

### ⛔ WHAT THESE TESTS CANNOT DETECT (`SC-§79`, stated so the gate does not have to find it)

1. **That "visible ⇒ the sampling timer is armed".** A bare `NewObject` widget has **no world** ⇒ no `FTimerManager`, so `ApplyFrameRateCounterPreference()` takes its no-timer branch and `IsCounterTimerArmed()` reports `false` in **both** states. An assertion that the counter is armed when shown **could not fail here**, and `TASK-1123`'s `M8` is this lane's standing proof that such a row is worse than none — so **it is not written**. The `SetTimer` → callback edge is engine behaviour used identically at a dozen shipped sites in this project (`HeroCharacter.cpp:742`, `GoldNode.cpp:417`, `BattlefieldScatter.cpp:2018`, `CaptureZone.cpp:95`) and is a **declared unproven premise**, exactly as `TASK-1118` records for its countdown.
2. **That the number is right on a real machine.** The sampler is driven with **fabricated** counter/clock pairs on purpose — that is what makes the arithmetic deterministic and the mutations meaningful. That `GFrameCounter` advances once per frame is read from engine source (§2.1), not from a run.
3. **Anything pixel.** Legibility over grass, clearance from the card bar, whether it is genuinely in the corner — `GFX-§2(e)`'s pixel/human check, owed to `TASK-1125`. **The `SC-§94` pixels rung is entirely unspent and the new log lines have never been observed.**
4. **Board cl. (5)'s literal "an absent field in an older `.sav`".** Not producible in-process — see **F-10**.

---

## 6. NAMED MUTATIONS — Addendum B form

🚨 **Every row is a DERIVED PREDICTION. Nothing was executed.**

| # | Mutation | Predicted red |
|---|---|---|
| **M23** | In `ComputeOverWindow`, **swap** the two output assignments | **≥ 6 rows**, incl. *"30 frames / 0.5 s = 60 FPS"* and *"…and publishes 60 FPS"*. The 60 fps / 30 fps pairs were chosen because the two numbers are far apart in **both** directions, which is what makes a swap unsurvivable. |
| **M24** | Delete the `FramesElapsed == 0` refusal | **≥ 5 rows**, incl. *"⛔ ZERO FRAMES in a window is REFUSED"* and *"A refused window leaves OutFramesPerSecond untouched"*. (Mutated: `Fps` becomes `0`, `Ms` becomes `inf`.) |
| **M25** ⭐ | In `Advance`, move the two anchor-update lines **below** the `ComputeOverWindow` early-return (advance only on success) | **EXACTLY 1 row** — *"⭐ …so it publishes 60 FPS again, not 30"*. Narrow on purpose, and **named as narrow**: the neighbouring `TestTrue` stays GREEN because 30 frames over 1.0 s is still an *honest* window — just the wrong one. |
| **M26a** | `ConstructCounterTree`: `HitTestInvisible` → `Visible` | **EXACTLY 1 row** — *"⛔ The counter root is HitTestInvisible as constructed"*. ⚠️ Only the constructed state; `ApplyFrameRateCounterPreference` would still write the correct value. |
| **M26b** | `ApplyFrameRateCounterPreference`: `HitTestInvisible` → `Visible` | **EXACTLY 2 rows** — *"Preference ON ⇒ the counter is shown, and shown HitTestInvisible"* and *"⛔ An unrelated setting's broadcast does NOT move the counter"*. ⛔ **This is the one that ships the click-eating bug**; run it if you run only one of the pair. |
| **M27** 🚨 | Delete `SaveObj->bShowFrameRateCounter = …` from `SaveSettingsToSlot()` (= ship the recipe's 4 steps) | **≥ 3 rows**, incl. *"save(counter=true) → a SEPARATE store loads true (recipe step 5 is present)"*. ⚠️ **The other-direction row stays GREEN** (disk holds `false` because the field was never written) — which is precisely why the `true` direction had to exist. |
| **M28** ⭐⭐ | `SetFrameRateCounterEnabled` forwards `SettingName_AssistantConfirmBeforeExecute` | **EXACTLY 2 rows, BOTH NAME rows** — *"⭐ The broadcast names the FRAME-COUNTER setting…"* and *"⭐ And moving the counter back names the counter again"*. ⛔ **Every count-based assertion stays GREEN** (3 broadcasts either way). This is `M21`'s lesson made executable. |
| **M29** | Delete the token filter from `USiegeGraphicsMenuWidget::HandleSettingsChanged` | **EXACTLY 1 row** — *"⛔ An UNRELATED setting's broadcast does NOT re-seed this row (the token filter)"*. |
| **M30** | Delete the token filter from `USiegeFrameRateCounterWidget::HandleSettingsChanged` | **EXACTLY 1 row** — *"⛔ An unrelated setting's broadcast does NOT move the counter"*. |
| **M31** | `ShouldShowFrameRateCounter`: `nullptr ⇒ true` | **≥ 3 rows**, incl. *"⛔ No settings store ⇒ the counter does NOT show (fail-safe OFF)"*. ⚠️ *"…and no timer is armed"* stays **GREEN** — the harness has no world (see §5 blind spot 1). Named so the gate does not read that green as coverage. |
| **M32** | `USiegeSettingsSaveGame::bShowFrameRateCounter` default → `true` | **≥ 5 rows**, incl. *"⛔ The SaveGame's compiled default is OFF"* and *"The subsystem's compiled default AGREES with the SaveGame's"*. ⚠️ *"⛔ A fresh store defaults to OFF"* in the counter test stays **GREEN** — it reads the subsystem member, which the mutation does not touch; the **drift guard** is what catches it. |
| **M33** | Delete the `ApplyBoolSetting(bShowFrameRateCounter, …)` line from `LoadSettingsFromSlot()` (recipe step 4) | **≥ 5 rows** across both settings tests, incl. *"save(counter=true) → a SEPARATE store loads true"* and *"⛔ A FOREIGN-class slot falls back to the C++ default (counter OFF)"*. |
| **M34** ⛔ | Delete `DisarmFrameRateReadout()` from `NativeDestruct()` | **ADMITTED ZERO-RED.** No test builds a world, so there is no timer to leave running, and no test reaches `NativeDestruct` at all. The property is proved by **counting the two call sites by reading**, not by a test. Written down before the gate had to find it (the `TASK-1122` `M2` precedent). |
| **M35** ⛔ | Delete the `ClearTimer` before `SetTimer` in `ArmFrameRateReadout()` | **ADMITTED ZERO-RED**, same root cause. |

---

## 7. FLAGGED DECISIONS FOR ⭐ `TASK-1121`

**F-1 — The in-match counter is a NEW UCLASS, `USiegeFrameRateCounterWidget`, declared inside `SiegeGraphicsMenuWidget.{h,cpp}` rather than in a new file pair.** `GFX-§10` pins no name for it (it pins the panel, the facade, the log category and three FPS tokens). The board's `names:` list gives me the widget pair plus *"the in-match HUD site (named in your handoff)"* — not a new file pair. Precedent: `SiegeControlsHelpWidget.h` declares **three** UCLASSes in one pair. Everything about the FPS instrument therefore lives in the graphics lane's own files. **Route the name for pinning if the manager wants it in `GFX-§10`.**

**F-2 — The in-match HUD site is `ASiegePlayerController::TryInitFrameRateCounter()`**, plus a forward declaration, an include, a ZOrder constant, a `UPROPERTY(Transient)` member, one `BeginPlay` call and one `EndPlay` block. Named here as the board requires. It is **eager**, unlike the console / war map / help overlay, which are lazy — argued in the header: those are opened by a keypress and have an event to hang on; this one is driven by a persisted preference that may already be on at match start, and a lazy path would need a **second** creation site for the mid-match toggle, which is the shape that ships *"works on a fresh match, silently dead after Play Again"* (`SC-§94`; `TASK-1122` found that exact defect in the scatter's reuse path).

**F-3 — Tests went to `Tests/SiegeSettingsTest.cpp` (3) and `Tests/SiegeGraphicsMenuTest.cpp` (4), and `Tests/SiegeGraphicsSettingsTest.cpp` got ZERO** — which is what the board's `names:` line asked for. This is `qa/TASK-1119.md` **F-6** applied unchanged (*"the harness is there … duplicating it would be strictly worse"*). The settings-store harness — `MakeScratchStore`, the throwaway `UGameInstance`, the scratch slot **name** and `FScratchSlotGuard` — is in `SiegeSettingsTest.cpp`; duplicating it would give one slot string **two independent owners and two independent guards**. The widget harness (`MakeScratchPanel`, `FindPinned`) is in `SiegeGraphicsMenuTest.cpp`. The small scratch-settings helper I *did* add to the menu test file deliberately uses a **different slot string**, so the two files never share disk state.

**F-4 — 🚨 I CORRECTED SHIPPED DOCUMENTATION: the *"HOW TO ADD SETTING #2"* recipe is now FIVE steps.** See §3. The file is in my WRITES list, the defect is real and I hit it by following the instruction literally, and leaving a four-step recipe in place would guarantee the next author ships a non-persisting setting. ⛔ I did **not** touch `CONVENTIONS.md` (`SC-§82`) — if the law quotes the four-step recipe anywhere, **that quotation is now wrong and is the manager's to fix.**

**F-5 — `ShowFrameRateCounterCheckBox` is deliberately NOT in `SetAllControlsEnabled`'s list.** Every control in that list writes through the **graphics facade**, and that function's one caller is "the facade did not resolve". This row writes through `USiegeSettingsSubsystem` — a different store, `GFX-§3`'s named exception — so a dead facade says nothing about it, and greying it would be the panel disabling a control it has no authority over. Its enabled state has exactly **one** writer, `RefreshFrameRateCounterRow()`. ⇒ this **narrows** `TASK-1119` F-9's *"every interactive control in one call"* to *"every control the FACADE owns, in one call"* — which I believe is what it always meant, but it is a change to a blessed invariant and so it is flagged, not assumed. Two assertions pin the consequence (*"A missing SETTINGS store does not disable Back"* / *"…nor any control the GRAPHICS facade owns"*).

**F-6 — `FrameRateReadoutText` appears in TWO widget trees** — the panel's and the counter's. No collision: `FName` uniqueness is **per `UWidgetTree`**. One word for one thing beats two words for one thing, and `GFX-§10` pins the word. If the gate wants two names, say so and I will rename the counter's.

**F-7 — ⚠️ THE PANEL READOUT SCROLLS WITH THE COLUMN.** It sits at the top, above Auto-Detect, and the panel opens scrolled to the top — but dragging the Shadow slider two thirds of the way down scrolls the number off. **Declared, not glossed.** My argument for accepting it: this is the *secondary* half of `GFX-§7` by the law's own words, the in-match counter is the tuning instrument, and the on-screen hint tells the player exactly that. **If the gate disagrees, the fix has a known shape** — a `HeaderPanel` `UVerticalBox` inserted between `PanelSizeBox` and `RootScrollBox` holding `TitleText` / `StatusText` / `FrameRateReadoutText`, with the ScrollBox below. I declined to make that spine change as the **third** author in this file without a ruling.

**F-8 — ZOrder 30, chosen against the three ZOrders I could find** (HUD `0`, victory `10`, war map `25`). ⛔ **Not a census** — I did not enumerate every `AddToViewport` in the project. If a higher one exists, the counter can be covered.

**F-9 — `GAverageFPS` / `GAverageMS` were measured and REJECTED** (§2.1 option b). Recorded so a reviewer does not raise it as a missed simplification: they are private-header globals with a smoothing constant we did not choose.

**F-10 — Board cl. (5)'s "an ABSENT field in an older `.sav` loads at the C++ default" is NOT PROVABLE IN-PROCESS, and I substituted rather than faked it.** Producing a `.sav` carrying `bAssistantConfirmBeforeExecute` and **not** `bShowFrameRateCounter` requires a build in which the second `UPROPERTY` does not exist; any file the suite can write carries both tags. What test 2 proves instead is **every reachable neighbour**: the CDO default, the two compiled defaults agreeing, the **missing-slot** fallback and the **foreign-class** fallback — all three of which funnel through the same `Source = Loaded ? Loaded : GetDefault<>()` line the absent-field case would use. UE's tagged-property behaviour itself is an **accepted premise**. ⚠️ **A test asserting the absent-field case would have been a test that cannot fail** — the value is `false` whether the tag is skipped or read.

**F-11 — Non-ASCII inside `TEXT()` literals is SAFE in this project, and I measured it rather than avoiding it.** These files carry **no BOM**, which normally means MSVC decodes UTF-8 multi-byte sequences as the system codepage and ships mojibake. ⛔ But UBT passes **`/utf-8` unconditionally** (`VCToolChain.cs:708`) and suppresses C4819 (`:711`). ⇒ the `·` in my readout string, and `TASK-1118`'s em dashes in `DisplayHint` / `CountdownUnavailable` / `AutoDetectBlocked`, and `SettingsMenuWidget.cpp:43`'s, are all fine. **Recorded so nobody raises it as a defect on this row or the previous one.**

**F-12 — `UCLASS(meta = (DisableNativeTick))` on the counter.** Belt-and-braces: `EWidgetTickFrequency::Auto` already means "no tick" for a direct `UUserWidget` subclass with no BP tick and no animations, and `UUserWidget` itself carries the same meta. It makes the "costs nothing per frame" claim **structural** rather than inherited.

**F-13 — "Visible ⇒ armed" is not asserted, and two mutations are admitted zero-reds** (`M34`, `M35`). All three have the same root cause: a headless widget has no world. Declared rather than papered over with unfailable rows.

---

## 8. FOR THE HOST ⭐ `TASK-1124`

1. ⛔ **Report the suite as an EXECUTED `N / M`** (`SC-§95`). My contribution is **+7**; the floor off the reviewer's own 545-declaration census is **552**. ⛔ Do not quote a total from any handoff, including this one, as measured.
2. ⭐ **If you run only one mutation from §6, run `M26b`** — it is the one that ships a viewport widget swallowing every click in the match.
3. **`M25`, `M26a`, `M28`, `M29`, `M30` are predicted to redden EXACTLY the named rows and nothing else.** Report ACTUAL vs PREDICTED; four of `TASK-1118`'s seven predictions came out different when the gate re-derived them, and I want to know if any of mine inverts.
4. ⛔ **This row does not compile alone**, and neither do `TASK-1118`/`TASK-1122` — same commit, as the board says. My diff adds an include of `SiegeGraphicsMenuWidget.h` to `SiegePlayerController.cpp`, so **a compile error in the graphics widget pair now surfaces in the controller's TU too**; do not misattribute it.
5. **New log lines to look for on a first run** (⛔ none has ever been observed): `[SiegeSettings] … bShowFrameRateCounter=…` at init and at every save · `[FrameRateCounter] In-match FPS/frame-time counter ON …` when the pref is on · `ASiegePlayerController '…': in-match FPS/frame-time counter created at ZOrder 30 …` at every match start.

## 9. FOR 🧑 JONATHAN'S PIXEL CHECK (`TASK-1125`)

1. Settings → **Graphics**: a number at the top that **moves**, showing both `FPS` and `ms`. It should read dashes for about a second first — that is correct, not a bug.
2. Tick **"Show FPS counter during a match"**, press Back twice, start a match.
3. **Top-right corner: the counter is there, over the battlefield.** ⭐ **The number should be visibly WORSE than the menu's** — that is the entire point of the feature.
4. **Click somewhere the counter is not, then click *through where it is*.** Placement and orders must work normally. (If a click near the top-right corner is swallowed, that is `M26b` and it is the one real risk in this row.)
5. Quit to menu, untick the box, start a match: **the corner is empty.**
6. Restart the game with the box ticked and start a match: **it is still there.** That is recipe step 5 doing its job.
