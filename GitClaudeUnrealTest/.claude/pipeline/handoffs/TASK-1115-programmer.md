# TASK-1115 — GFX-PANEL — handoff (gameplay-programmer)

**Marker:** `TASK-1115-GFX-PANEL` · **Date:** 2026-09-07 · **Gate:** `TASK-1116` (qa-reviewer) · **Host:** ⚠️ see §9 F-6 (the row's `names:` says `TASK-1117`; the row's own cycle-correction re-homes it to `TASK-1124`, and `TASK-1117` has already shipped and closed)
**Spec source:** the board row cl. (1)-(8) + riders (3a)-(3e) · `handoffs/TASK-1113-programmer.md` (incl. § LOOP 1) · `qa/TASK-1114.md` (both loops) · `handoffs/TASK-1112-programmer.md` §2.
**Law:** `GFX-§2` (all six conditions) · `GFX-§5` · `GFX-§6` · `GFX-§9` · `GFX-§10` (ratified) · `SC-§79` `SC-§83` `SC-§94` `SC-§101`.

**Dispatch fences, checked before the first keystroke, not assumed:** `TASK-937` and `TASK-1117` are BOTH reported done. `TASK-1117` committed `42734b7` with a **measured** suite of `517 / 0` (`handoffs/TASK-1117-buildmaster.md` §; "17 builds, all green", `Test Started / Test Completed 517 / 517`). ⇒ no live compile window to poison.

---

## 0. WHAT A PLAYER CAN NOW DO

Main Menu → **Settings** → a new **Graphics** button above Back → a scrolling panel with ~~**19 rows**~~ ✅ **18 rows**:

> 🚨 **CORRECTED IN PLACE 2026-09-07 BY THE MANAGER — `19` was never true.** `qa/TASK-1116.md` counted **18** (WARN-7), and **the table immediately below this line lists 18**: 1 + 1 + ten (rows 3–12) + 13 + 14 + 15 + 16 + 17 + Back = 18. The number is corrected here *and* at §8 step 4 because §8 is 🧑 Jonathan's script and a miscount there produces a **false failure on the only human check this feature gets**. Struck rather than deleted so a reader who remembers `19` finds its resolution instead of re-deriving it.


| # | Row | Control | Applies |
|---|---|---|---|
| 1 | Auto-Detect Quality | button + "this takes a moment" line | immediately |
| 2 | Overall Quality | 5-detent slider + live level name / `Custom` | immediately |
| 3-12 | View Distance · Anti-Aliasing · Shadows · Global Illumination · Reflections · Post Processing · Textures · Effects · Foliage · Shading | 5-detent slider + live level name each | immediately (Foliage: **next match**, and the row says so) |
| 13 | Resolution Scale | **continuous** 50-100 % slider + live `%` | immediately, **on release** |
| 14 | Screen Resolution | `<` value `>` stepper | **staged** — see §4 |
| 15 | Window Mode | `<` value `>` stepper | **staged** — see §4 |
| 16 | V-Sync | checkbox | immediately |
| 17 | Frame Rate Limit | `<` 30/60/90/120/144/165/240/Unlimited `>` stepper | immediately |
| 18 | Back | button | — |
| (+ title, status line and four hint lines) | | | |

**Zero `.uasset`. Zero `Config/`. Zero editor. Zero MCP. Zero git.** `/Game/UI/WBP_MainMenu` was never opened.

---

## 1. FILES WRITTEN (5 — nothing else)

| File | Lines | What |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.h` | **723** (new) | the panel's contract: 30 group members + 25 others, all `BindWidgetOptional`; 8 public pure statics; the two test seams |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.cpp` | **1917** (new) | the code-authored tree, the ten-row loop, every handler |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeGraphicsMenuTest.cpp` | **1126** (new) | **12** automation tests + the `SC-§83` mutation table (M1-M13) |
| `Source/GitClaudeUnrealTest/Siegebound/SettingsMenuWidget.h` | 264 → **303** | `GraphicsButton` + `GraphicsLabelText` + `GraphicsPressed()` + `HandleGraphicsClicked()` |
| `Source/GitClaudeUnrealTest/Siegebound/SettingsMenuWidget.cpp` | 545 → **658** | the entry button, built ABOVE `BackButton`; bind/unbind; the navigation body |

⛔ **ZERO** `Content/**` · **ZERO** `Config/**` · **ZERO** `.uasset` · **ZERO** `CONVENTIONS.md` · **ZERO** `TASKBOARD.md` beyond my own single `- status:` line · **ZERO** `SiegeGraphicsSettingsSubsystem.{h,cpp}` · **ZERO** `Tests/SiegeGraphicsSettingsTest.cpp` (that file is `TASK-1118`'s by the board's `names:`; a **separate** test file is why we cannot collide) · **ZERO** `BattlefieldScatter.*` · **NO compile, NO editor, NO MCP, NO git.**

⚠️ **I did NOT touch the confirm-toggle row in `SettingsMenuWidget`** (the board fences it). The diff there is additive: one text constant, one member pair, one construct block, one bind, one unbind, two functions, one include.

---

## 2. THE PANEL'S STRUCTURE, AND HOW THE TREE ENTRY IS ADDED

### 2.1 The tree

```
BackdropBorder            UBorder      ← WidgetTree->RootWidget, ESlateVisibility::Visible
  └ PanelSizeBox          USizeBox     WidthOverride 1040
      └ RootScrollBox     UScrollBox   ← board cl. (4)
          └ RootPanel     UVerticalBox
              TitleText · StatusText
              AutoDetectButton{AutoDetectLabelText} · AutoDetectHintText
              OverallQualityRow  [ label | slider | value ]
              <Group>Row × 10    [ label | slider | value ]   ← ONE LOOP
                  (+ ShadowQualityHintText, + FoliageQualityHintText)
              ResolutionScaleRow [ label | slider | value ] + ResolutionScaleHintText
              ScreenResolutionRow[ label | < | value | > ]
              WindowModeRow      [ label | < | value | > ]
              DisplayHintText
              VSyncCheckBox{VSyncLabelText}
              FrameRateLimitRow  [ label | < | value | > ]
              BackButton{BackLabelText}
```

**`GFX-§2(c)` — the order.** `RebuildWidget()` is three lines and they are the shipped `USettingsMenuWidget.cpp:92-94` sequence: `Initialize(); ConstructGraphicsTree(); return Super::RebuildWidget();`. ⛔ **This is the one thing in this row that NO test can see** — see §6.

**`GFX-§2(b)` — the escape hatch.** `ConstructGraphicsTree()` returns at its second guard when `WidgetTree->RootWidget != nullptr`. That is also what makes a second call a no-op; the per-row `UHorizontalBox`es are **not** individually null-guarded, so the escape hatch is load-bearing and the header says so.

**`GFX-§2(f)` — the backdrop is hit-test VISIBLE.** `BackdropBorder->SetVisibility(ESlateVisibility::Visible)`, with the reasoning in place. This panel stacks on the live settings panel, which stacks on the live main menu, and neither is removed ⇒ a `HIT_TEST_INVISIBLE` plate would ship a live click-through into **QUIT** while the panel looks modal. Asserted by test 1.

### 2.2 The entry in the settings tree (board cl. 6)

`USettingsMenuWidget::ConstructSettingsTree()` gains one block, inserted **before** the `---- BackButton ----` block exactly where `TASK-1112` §2 said to put it, in the shipped button idiom (**font 28**, `FMargin(24,12,24,12)`, `HAlign_Fill`) so Back stays last. `GraphicsPressed()`:

```cpp
USiegeGraphicsMenuWidget::CreateAndAddToViewport(GetOwningPlayer(), nullptr, /*ZOrder*/ 20);
```

`ZOrder 20` is **above** the settings panel's own 10, and the settings panel is **not** removed — the `TASK-438` shape cloned one level deeper. The `nullptr` class argument is the shipping state, not an oversight: `/Game/UI/WBP_GraphicsMenu` is RESERVED and UNAUTHORED, so it falls back to the C++ class. If that asset is ever authored, that one argument is the only line that changes.

---

## 3. THE TEN GROUPS — ONE LOOP, NOT TEN BLOCKS

```cpp
for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
{
    BuildQualityGroupRow(GroupName);
}
```

The list is the **facade's own**, in `GFX-§8` Tier-B order. The panel never re-decides the list, the spelling or the count: a group added there appears here for free.

**Every widget name is DERIVED from the canonical group name** (`FName(*(Base + TEXT("Slider")))`), so `GFX-§10`'s triple is true by construction for all ten rows, and `EffectsQualitySlider` / `PostProcessQualitySlider` are what actually gets built. ⛔ A repo grep for `VisualEffect` or `PostProcessing` in my three new files returns **zero**.

**The one place a name is paired with its three members** is `ResolveGroupRow(FName)`, and it uses a **local token-pasting macro** (`#undef`'d in the same function). That is a correctness device, not brevity: the group constant `GroupName_##Token` and the three members `##Slider` / `##LabelText` / `##ValueText` all come from the *same identifier*, so `ShadowQuality` **cannot** be paired with `ReflectionQualityValueText` — the pairing is not written anywhere to get wrong. Ten hand-written blocks would be thirty chances at a silent mis-wire, and a mis-wired row is invisible at runtime: the handle moves, a setting changes, and it is the wrong setting.

Two thunks drive all ten sliders (`USlider`'s delegates carry no sender): the value-changed thunk loops and refreshes every level label; the commit thunk loops, **snapshots** the disagreeing groups, then writes them.

**The eleventh group is absent.** `LandscapeQuality` has no `[LandscapeQuality@N]` section at any level, no auto-detect threshold table and no Landscape in this project ⇒ the slider would move and change nothing. Test 1 asserts `LandscapeQualitySlider` does not exist.

---

## 4. THE FIVE RIDERS — WHAT I DID FOR EACH

### (3a) — the write is on COMMIT, never `OnValueChanged` ✅

- `OnValueChanged` → `Handle*ValueChanged` → **updates the label and returns.** No facade call, no apply, no save, no broadcast.
- `OnMouseCaptureEnd` **and** `OnControllerCaptureEnd` → `Handle*Committed` → the one write.
- ⚠️ **Both capture-end events, and the second is not belt-and-braces.** Measured at `SSlider.cpp:280-289`: a gamepad/keyboard user never produces a *mouse* capture end — `SSlider` commits on `OnFocusLost` and then fires `OnControllerCaptureEnd`. Binding only the mouse event would silently drop every pad-driven change. (`USlider::RequiresControllerLock` defaults to **true** — `Slider.cpp:28` — so nav keys cannot move a slider *without* a capture, which is what closes the last hole.)
- The detented group sliders take the same rule ("not exempt in principle, only in degree").
- **Proved BOTH halves** (test 6): the *binding* by `FMulticastScriptDelegate::Contains` (the write handler is **not** on `OnValueChanged`), and the *behaviour* by driving 50 simulated drag frames and asserting `SaveSettingsCallCount == 0`, then one commit and asserting `== 1`. Neither half alone is evidence.

### (3b-WIDENED) — `Back` must revert a staged video mode 🚨 ✅

**The mechanism, restated so the gate can check I understood it:** `SetScreenResolution` / `SetWindowMode` **stage** only, and `IsVideoModeChangePending()` is true from the staged-vs-`LastConfirmed` comparison **alone** — so **one stepper press** is enough. While it is true the facade refuses (a) **every save** (`RequestSaveSettings`, `cpp:1199-1207`) and (b) **`AutoDetectQuality()`** (`cpp:522-529`, added by `TASK-1113` LOOP 1). Both refusals are **correct**. Nothing below the widget knows the panel closed.

**The fix — one function, two call sites, exactly one revert:**

```cpp
bool USiegeGraphicsMenuWidget::DiscardStagedVideoMode(USiegeGraphicsSettingsSubsystem* Graphics)
{
    if (Graphics == nullptr)                     { return false; }
    if (!Graphics->IsVideoModeChangePending())   { return false; }   // ⛔ the guard
    Graphics->RevertVideoModeChange();
    return true;
}
```

Called from **`BackPressed()`** (before `RemoveFromParent()`, so the window closes synchronously on the click) and from **`NativeDestruct()`** (the catch-all: a level change, the owning panel being removed, anything). On the Back path the second call finds the window closed and returns `false` **without reverting** ⇒ **exactly one** `RevertVideoModeChange()`, guaranteed by **state**, not by ordering.

⛔ **It calls the facade's revert and nothing else.** `TASK-1118` cl. (9)'s read-to-the-terminator duty is discharged here: I re-read `RevertVideoModeChange` (`SiegeGraphicsSettingsSubsystem.cpp:900-925`) after `TASK-1113`'s loop-1 rewrite and it still does `RevertVideoMode()` **then** `ApplyResolutionSettings(false)`, in that order. I did not unwrap it and I did not duplicate it.

**Proved twice.** Test 4 calls the function twice and asserts `ApplyResolutionSettingsCallCount` goes `0 → 1 → 1`. Test 5 drives it through the panel's own `BackPressed()` **with a positive control first**: it asserts the soft-lock is REAL (Auto-Detect refused, refusal counter 1; an unrelated quality change reaching zero saves) and only then asserts Back releases both. ⛔ **The instrument is `RefusedSaveWhileVideoModePendingCount`, not the bool `AutoDetectQuality()` returns** — under the automation seam that bool is `false` on a broken build and a fixed one alike, which is exactly why the board's cl. (8-WIDENED) names the counter.

🚨 **RECONCILIATION NOTE FOR `TASK-1118` (it lands second, so it owns this):** the `Back`/teardown discard is **already here**. `TASK-1118` should **call `DiscardStagedVideoMode` or delete it and own both sites — never add a third**. Its seam for the provisional apply + countdown is marked in the body of `StepScreenResolution()` and `StepWindowMode()` with a `⇒ TASK-1118: THIS IS YOUR SEAM` comment.

### (3c) — `Custom` from the visible groups ✅

`IsCustomForDisplay(Graphics)` forwards to `IsOverallQualityCustomAcrossVisibleGroups()` and is the **only** question the panel labels from. ⛔ No hand-rolled second Custom rule (the `TASK-931` "N hides" defect). The helper the board required **does** ship on the facade, so nothing was routed back.

When not Custom, the displayed level is the visible groups' shared level — deliberately **not** `GetOverallScalabilityLevel()`, which can read `-1` because of the invisible eleventh group *or* because the resolution scale drifted off the preset's canonical value (`Scalability.cpp` `GetSingleQualityLevel`, which I read: it requires `GetRenderScaleLevelFromQualityLevel(Target) == ResolutionQuality` **and** all eleven groups equal).

When Custom, the handle sits at the rounded mean of the ten visible levels and the **label** states the truth — `GFX-§5`'s "the label always states the truth". A handle has to be somewhere; the mean is the least misleading place.

Test 8 builds the disagreement (ten visible groups at High + a 63 % scale), asserts the **engine** says Custom, asserts the **panel** does not, and then asserts a *genuine* disagreement among the visible ten *does* read Custom — so the test cannot pass on a panel that never says Custom at all.

### (3d) — a preset press moves the resolution scale, and that is correct ✅

Not suppressed, not "fixed". `HandleOverallSliderCommitted()` calls `SetOverallScalabilityLevel` and then re-seeds **every** row (the facade's conditional second broadcast reaches `HandleGraphicsSettingsChanged` → `RefreshAllRows()`, and there is a belt-and-braces refresh for the no-op case where nothing broadcasts). `AutoDetectPressed()` does the same, because a benchmark moves eleven values. The comment at the call site names `Scalability.cpp:1047` and says **a stale resolution-scale row is the real defect here**.

### (3e) — no `FIntPoint` in a BIE parameter ✅

`OnGraphicsRowChanged(const FString& SettingName, int32 IntValue, const FString& DisplayValue)` — `FString`/`int32` only. The resolution crosses as its **stepper index** plus the facade's already-composed `"1920 x 1080"` label; window mode crosses as `int32`. The panel consumes `GetSupportedScreenResolutionLabel` / `SetScreenResolutionByIndex` / `FindCurrentScreenResolutionIndex` and **never** the `FIntPoint` overloads. There is no `UENUM` anywhere in this pair.

---

## 5. THE "APPLIES AT NEXT MATCH START" TEXT (board cl. 5 / `GFX-§9`)

**Shipped, verbatim, on the Foliage row:**

> *"Grass and trees on the battlefield are built when a match starts, so a change here appears at the NEXT match — not the one in progress."*

**And the Shadows row carries `GFX-§9`'s ruled fog line**, which is a **live readout**, not a decoration — it re-composes from `ShouldEnableVolumetricFog()` on every refresh:

> *"Also drives volumetric fog, which the engine turns OFF at Low and Medium. Currently: fog ON."* / *"…Currently: fog OFF."*

⇒ that shipped getter now has a **real caller** (`SC-§36.1`), and test 10 asserts the line follows the facade from Shadows=Low to Shadows=Epic.

⚠️ **THIS IS A DECLARED DEVIATION FROM cl. (5)'s WORDING — see §9 F-1.** cl. (5) names "Foliage, View Distance and Effects"; I ship the notice on **Foliage only**, measured rather than conformed (`SC-§101`).

Other player-facing text shipped: the Auto-Detect stall warning (`GFX-§6`'s other half), the resolution-scale explanation ("the biggest single performance win… menus and text stay sharp"), the display-staging line (§4 below), a panel status line, and the unavailable line.

---

## 6. TESTS — `Tests/SiegeGraphicsMenuTest.cpp`

**12 tests**, hermetic (scratch `UGameUserSettings` + engine-apply suppression in one call; the panel is `Initialize()`d and its tree built **without** taking a Slate widget — `UUserWidget::Initialize()` is headless-safe for a native class: no `UWidgetBlueprintGeneratedClass` ⇒ `InitializeNativeClassData()` is an empty virtual, `WidgetTree` is `NewObject`'d, and `NativeOnInitialized` is skipped because `PlayerContext` is invalid).

`TreeBuildsEveryPinnedRow` · `DetentSlidersSnapAndTheContinuousOneDoesNot` · `LevelSliderValueRoundTrip` · **`DiscardStagedVideoModeRevertsExactlyOnce`** · **`BackReleasesAutoDetectAndEverySave`** · **`ResolutionScaleWritesOnCommitNotOnValueChange`** · `GroupCommitWritesOnlyTheMovedGroup` · **`CustomLabelIgnoresInvisibleInputs`** · `NextMatchStartNoticeIsScatterDrivenOnly` · `ShadowHintTracksVolumetricFog` · `StepperIndexWrapsBothWays` · `NullSubsystemDisablesEveryControlButBack`.

Every pinned child is looked up **by its name through the `WidgetTree`**, never through the C++ member, so a member holding a wrongly-named widget fails.

### `SC-§83` — the named mutations, and ⛔ **NO WITNESSED RED**

The full table (M1-M13) is at the bottom of the test file next to the code it mutates. Headline pairs:

| | mutation | predicted RED |
|---|---|---|
| **M1** | delete `OutSlider->MouseUsesStep = bDetented;` | `DetentSliders…` (11 assertions) — and **nothing else**, which is the point |
| **M3** | move the write into `HandleResolutionScaleValueChanged` | `ResolutionScaleWritesOnCommit…` — "50 drag frames, 0 saves" would read 50 |
| **M3b** | bind the write to `OnValueChanged` but leave the bodies alone | the same test's **binding** half only |
| **M4** | delete the `IsVideoModeChangePending()` guard from `DiscardStagedVideoMode` | `DiscardStagedVideoModeRevertsExactlyOnce` — ⚠️ **predicted weaker variant: only the last two rows redden**, which is why the test asserts the counter twice rather than the bool twice |
| **M5** | delete the discard from `BackPressed()` | `BackReleasesAutoDetectAndEverySave` — ⚠️ the positive-control block above it stays GREEN, which is what proves the red is about the fix |
| **M6** | `IsCustomForDisplay` → `IsOverallQualityCustom()` | `CustomLabelIgnoresInvisibleInputs` — ⚠️ the fixture self-check stays green, so "the panel regressed" is distinguishable from "the engine changed" |
| **M11** | backdrop → `HitTestInvisible` | `TreeBuildsEveryPinnedRow` — the live cost is a click-through into QUIT and **no other test sees it** |

⛔ **NO WITNESSED RED. None of M1-M13 was executed.** This row does not compile and does not run the suite. **Every mutation is a derived prediction from reading the code, not an observed transition** — and `TASK-1113`'s **M4** is the standing proof that a confident prediction in a file like this can be wrong (`SC-§90`: plausibility is not a measurement).

**Suite: NOT RUN by this row.** The last real executed baseline is **`517 / 0`** (`TASK-1117`, `42734b7`). This row adds **12** tests, so the expected new total is **`529`**, and that number is a **PREDICTION** until the host executes it under an `SC-§87` bound. ⛔ Do not quote `529` as measured. ⛔ Never `514`, `511` or `306`.

### `SC-§79` — what these tests **cannot** detect (also written into the file)

1. ⭐ **`GFX-§2(c)`, the ORDER.** Nothing here proves `ConstructGraphicsTree()` runs **before** `Super::RebuildWidget()`. Building it after would leave every assertion above green while shipping a silently EMPTY panel. ⇒ **a three-line code read at `RebuildWidget()`, and a QA criterion on `TASK-1116`.**
2. Whether anything is **visible, legible, on-screen or clickable**. No Slate tree is built. `GFX-§2(e)`: pixels or a human. See §8.
3. Whether a moved setting changed a **rendered frame** (`GFX-§9` / `SC-§94`) — entirely unspent.
4. Whether the sliders respond to a real **drag**; `SSlider`'s input path is Slate's.
5. That `NativeDestruct()` is actually reached by a real `RemoveFromParent()` — engine behaviour, asserted nowhere. Test 4 proves the *predicate*, not the *reachability*.

---

## 7. TWO ENGINE FINDINGS NOBODY IN THIS LANE HAD, AND BOTH ARE LIVE

### F-A 🚨 `USlider::MouseUsesStep` DEFAULTS TO `false` — `Slider.cpp:27`

`SSlider::PositionToValue` snaps to `StepSize` **only inside `if (bMouseUsesStep)`** (`SSlider.cpp:455-470`). ⇒ **`StepSize` alone affects keyboard / gamepad navigation ONLY.** A mouse drag on a default `USlider` returns a continuous value.

**Shipped as `GFX-§5` is written — "`USlider`, `StepSize 0.25` ⇒ 5 detents" — and nothing else, the ten quality sliders would have glided smoothly through 0.37 and 0.61: exactly the "fake continuity over 5 states" that ruling exists to forbid.** It would have looked correct in every code review, every property read-back and every MCP query; only a human dragging the handle would ever have seen it.

Fixed by `OutSlider->MouseUsesStep = bDetented;` (there is **no public setter** — it is a plain public `UPROPERTY` with no `Setter=`, read by `USlider::SynchronizeProperties` at `Slider.cpp:76` when the Slate widget is built, i.e. after this assignment). Asserted eleven times by test 2, and asserted **false** on the resolution-scale bar, because snapping the one genuinely continuous control would be the same lie in reverse. Mutation **M1**.

### F-B 🚨 `USlider::SetValue` DOES RE-ENTER `OnValueChanged`

`USlider::SetValue(float)` calls `HandleOnValueChanged(InValue)` when the cached `Value` differs, which **broadcasts**. This is the **opposite** of `UCheckBox::SetIsChecked` (the shipped `USettingsMenuWidget` comment correctly records that the check box does *not* re-enter). ⇒ **a programmatic seed re-enters the panel's own handler.**

Harmless today (the value-changed thunks only touch labels) but one edit away from a seed that writes what it read — i.e. from opening the menu costing an ini write. Handled twice: `SeedAndBind()` **unbinds first**, and `RefreshAllRows()` sets `bSuppressRowEcho` around every push, which the thunks early-return on.

---

## 8. `GFX-§2(e)` — 🧑 JONATHAN'S PIXEL CHECKLIST (this is `TASK-1125`'s script)

⛔ **Nothing below is verified by me. There is no `.uasset` to read back and MCP read-back has passed on visually-broken UMG on this project before.**

1. **Main Menu → Settings.** Is there a **Graphics** button, above **Back**, in the same size/typeface as Back?
2. **Press Graphics.** Does a panel open **on top** of Settings — and is Settings still visible dimmed behind it, not replaced?
3. **Click the dark area** to the left/right of the panel. ⛔ **Nothing behind it may respond.** (This is the `GFX-§2(f)` click-through into QUIT.)
4. **Scroll to the bottom.** Are all ~~19~~ ✅ **18** rows reachable? Is **Back** the last one? (🚨 **CORRECTED 2026-09-07 BY THE MANAGER from `qa/TASK-1116.md` WARN-7 — the count in this step was wrong by one and would have made you report a missing row that does not exist. Count **18**. The binding copy of this checklist is TASKBOARD `TASK-1125`; where the two disagree, `TASK-1125` wins.**)
5. **Count the quality sliders: there must be exactly TEN**, labelled View Distance, Anti-Aliasing, Shadows, Global Illumination, Reflections, Post Processing, Textures, Effects, Foliage, Shading. Each has a word beside it (Low / Medium / High / Epic / Cinematic).
6. ⭐ **Drag one quality slider slowly.** Does it **click between five positions**, or does it glide smoothly? ⛔ **Gliding = finding F-A regressed.** Does the word beside it change *while* you drag?
7. **Let go.** Does the picture change? (Shadows and Global Illumination are the most visible.)
8. **Drag Resolution Scale down to ~60 %.** The number should track the handle **while dragging**; the picture should change **when you let go**, not before.
9. **Press an Overall Quality detent.** ⭐ **Expect the Resolution Scale row to jump too — that is correct**, not a bug.
10. **Set two group sliders to different levels.** Does Overall Quality read **Custom**?
11. **Press Auto-Detect.** Does it pause a moment and then move the sliders? Afterwards, does Overall Quality still read a sensible value rather than being stuck on **Custom** forever?
12. **Set Shadows to Low.** Does the line under it change to "*Currently: fog OFF*"? Set it to Epic — "*fog ON*"?
13. **Look at the Foliage row's line.** It must say the change appears at the **NEXT match**.
14. 🚨 **THE SOFT-LOCK CHECK, THE ONE THAT MATTERS MOST:** press **`>` on Window Mode once**, then press **Back**, then go back into **Graphics**. Now press **Auto-Detect** — *it must work* — and drag a **quality slider** — *it must stick*. ⛔ If either does nothing, cl. (3b-WIDENED) is not fixed.
15. **Back** returns to Settings, and **Back** again returns to the Main Menu.
16. **Restart the game.** Are your quality settings still there? Is your **display mode** back to what it was (because you never confirmed it)?

---

## 9. FLAGGED DECISIONS FOR THE GATE (`TASK-1116`)

| # | Decision | Why it needs a ruling |
|---|---|---|
| **F-1** ⭐ | **The next-match-start notice is on FOLIAGE ONLY**, not on the three rows board cl. (5) names. | cl. (5) predates the `GFX-§9` correction of 2026-09-07, which **STRUCK** two of the three project levers. Measured: **View Distance** — `[ViewDistanceQuality@N]` sets `r.ViewDistanceScale` (0.4 @0 → 1.0 @3), which scales HISM cull distances **immediately**, and `GetViewDistanceScale()` is `1.0` at every level *by measurement*, so there is no project-side deferral; saying "next match" there would **undersell a control that already works this frame**. **Effects** — stock scalability CVars, immediate; the fog lever `GFX-§9` once bound to it was struck for being the wrong group **and** the wrong symbol. **Foliage** — the one surviving Tier-D lever, read inside `RunScatterPasses` (`BattlefieldScatter.cpp:363`), once per match. `SC-§101`: I treated the prescribed list as a claim and measured it. **Wanted: confirm, or tell me to ship cl. (5)'s literal three.** |
| **F-2** ⭐ | **The display steppers STAGE and do not apply.** No `ApplyVideoModeProvisional()` in this row. | `GFX-§4` requires a provisional apply to be paired with the 10-second auto-reverting countdown, and that countdown is `TASK-1118` cl. (1)'s. Applying a mode with **no countdown in the tree** is the exact permanent lockout `GFX-§4` exists to prevent: if the new mode is unviewable the player cannot find Back, so my teardown revert never runs. ⇒ **the honest interim is to stage, say so, and discard on leave.** `DisplayHintText` says exactly that and stays true after `TASK-1118` lands. ⚠️ **If `TASK-1118` does NOT ship in the same commit as this row, a player can change Screen Resolution and see nothing happen.** That is a real, temporary, *safe* gap and the gate should decide whether it is acceptable to commit alone. |
| **F-3** | The Foliage hint promises a next-match effect whose **consumer (`TASK-1122`) has not landed**. | Today the Foliage group's project-side lever has no reader, so the sentence is ahead of its consumer. It is true of the mechanism (`SetCullDistances` really is called inside the build path) and becomes true end-to-end when `TASK-1122` lands — which shares the same host. **Wanted: a yes, or an instruction to hold the line until `1122` is in.** |
| **F-4** | A **token-pasting macro** in `ResolveGroupRow`, `#undef`'d in the same function. | It is the only construction under which a group name **cannot** be mis-paired with a neighbour's widgets. Ten hand-written blocks are thirty chances at a defect that is invisible at runtime. If house style forbids the macro, the alternative is thirty hand-typed member names and a test that only *probably* catches a swap. |
| **F-5** | Seven `public` members that would normally be `protected`: the nine input thunks, `SeedAndBind`, `UnbindAll`, `ConstructGraphicsTree`, and the two `…ForAutomationTests` seams. | A slider cannot be dragged headlessly. Without these the suite could prove **neither** half of cl. (3a) and **none** of cl. (3b). Each carries a doc comment saying why. The two `…ForAutomationTests` names follow the facade's own idiom. |
| **F-6** ⚠️ | **The row's own `names:` line says `HOST: TASK-1117`, and `TASK-1117` has already shipped and closed** (`42734b7`, `TASK-1113` alone). | The row's status/cycle-correction block says the panel's integration was **re-homed to `TASK-1124`**, but the `names:` line was not updated with it. I did not edit any board line but my own. **The manager should reconcile it**, or a host reading `names:` literally will look for a row that is done. |
| **F-7** | The panel adds **three unpinned widget names** `GFX-§10` does not list: `PanelSizeBox`, `RootScrollBox`, `StatusText` (plus `<Base>Row` containers, `<Base>Prev/NextButton`, `<Base>Glyph`, and the four hint texts). | `RootScrollBox` is board cl. (4)'s requirement and `GFX-§10` predates the row count. All are `BindWidgetOptional` with stable names so a future `WBP_GraphicsMenu` can adopt them. **Wanted: fold them into `GFX-§10` or say they stay unpinned.** `CONVENTIONS.md` is the manager's (`SC-§82`), so I did not add them myself. |
| **F-8** | `AutoDetectPressed()` **repurposes `StatusText`** to show the display-staging line when the facade refuses the benchmark. | It is the only surface on the panel that can explain a button that just did nothing. The alternative — a silent no-op — is the failure mode this whole row exists to remove. Slightly odd that a *display* sentence appears in a *status* slot; call it if you disagree. |

---

## 10. WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐⭐ **`GFX-§2(c)` — READ THE THREE LINES.** `SiegeGraphicsMenuWidget.cpp`, `RebuildWidget()`: `Initialize(); ConstructGraphicsTree(); return Super::RebuildWidget();`. ⛔ **No test in this diff can see this**, and getting it wrong ships an empty panel that passes everything else.
2. ⭐ **The `Back` path has exactly ONE revert. Counted, not asserted:** `grep -n "RevertVideoModeChange" SiegeGraphicsMenuWidget.cpp` ⇒ **3 hits, of which exactly ONE is a call** (`:1793`, inside `DiscardStagedVideoMode`, **below** its `IsVideoModeChangePending()` guard); the other two are `:1775` and `:1789`, both comments. `grep -n "DiscardStagedVideoMode" SiegeGraphicsMenuWidget.cpp` ⇒ **7 hits: 1 definition (`:1765`), 2 CALLS (`:914` `NativeDestruct`, `:1302` `BackPressed`), 4 comments.** If the call counts are anything but 1 and 2, cl. (3b) is broken in one direction or the other.
3. ⭐ **The write is not on `OnValueChanged`.** In `SeedAndBind`, check that `Handle*Committed` appears **only** on `OnMouseCaptureEnd` / `OnControllerCaptureEnd`, and that the three `Handle*ValueChanged` bodies contain **no** `Graphics->Set…` call at all.
4. **The canonical spelling.** `grep -nE "VisualEffect|PostProcessing"` across my three new files ⇒ **exactly ONE hit, `SiegeGraphicsMenuWidget.h:525`, and it is the PROHIBITION ITSELF** ("⛔ NEVER `VisualEffect*`, ⛔ NEVER `PostProcessing*`"). ⛔ **Zero in code.** `GFX-§10` is ratified; either spelling outside the facade's four commented sites is a fail, not a nit.
5. **`FIntPoint` never reaches the BIE.** `grep -n "FIntPoint" SiegeGraphicsMenuWidget.{h,cpp}` ⇒ **5 hits, ALL of them comments** (`.h:63`, `:65`, `:201`, `:202`; `.cpp:1192`) — every one explaining why the type is absent. ⛔ **Zero in code, and in particular zero in any `BlueprintImplementableEvent` signature.**
6. **Null-safety on every click path.** Every handler begins by resolving and bails to `ShowPanelUnavailable()`. Test 12 drives all ten paths with a null subsystem. ⚠️ Check that **Back stays enabled** there — the backdrop is modal, so a dead Back is a trap.
7. **F-A is the finding I am most confident about and least able to prove.** It is a source read of `Slider.cpp:27` and `SSlider.cpp:455`, not an observation. If it is wrong, one line is wrong; if it is right and had been missed, `GFX-§5` would have shipped as prose.
8. ⚠️ **Nothing here has been compiled or executed.** The three most likely compile risks, since I could not check them: (i) `TObjectPtr<Derived>` → `UWidget*` in the aggregate initializer inside `SetAllControlsEnabled` (I used `.Get()` on every entry precisely to avoid it); (ii) the nested plain `struct FGroupRowWidgets` inside a `UCLASS` body — UHT accepts these (precedent: `UNavCollisionBase::FDelegateInitializer`, `NavCollisionBase.h:27`), but it is the sort of thing that fails at header-tool time rather than compile time; (iii) `FMulticastScriptDelegate::Contains(const UObject*, FName)` in the test file (`ScriptDelegates.h:1206`).
9. **Not mine:** the compile, the executed `N / M`, the commit. I hold no compiler and no `git` this row.

---

## 11. SLACK

Posted to ⚙️ Dev & QA (`C0BF0QZP3CN`, thread `1783116269.740549`), prefix `⚙️ GAMEPLAY-PROGRAMMER:` — the row count as built, confirmation the tree is built **before** `Super::RebuildWidget()`, and this handoff's path.
