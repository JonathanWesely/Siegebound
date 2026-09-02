# TASK-745 — [MARK-2] `UWarMapWidget`: place · wheel-resize · right-click delete · the centred number

**Author:** gameplay-programmer · **Status:** ready-for-qa · **Date:** 2026-09-01
**Law:** `MARK-§0..§6` · `WR-§6` · `WR-§9` · `WM-§4` · `WM-§8d` · `WM-§8e` · `HIGH-§1` · `SC-§15` · `SC-§32` · `SC-§33` · `SHIP-§9c` · `W4-R1`

**Jonathan's directive, verbatim:** *"click on anywhere on the map, to create a circle on the map at that location. you can hover the mouse over it and use the mouse wheel scroll to make that circle larger or smaller. You can right click to delete that circle. Every time you make a new circle, it gets its own number in the middle of it."*

---

## A. FILES TOUCHED — three, all inside the fence

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h` | SOLE owner. New: `FSiegeWarMapMarkCircle`; 6 pure statics on `FSiegeWarMapProjection`; `MakeMarkPickSymbol` / `GetMarkRingColor` / `BuildMarkCircles` / `GetMapMarkCount`; `NativeOnMouseWheel` / `NativeOnMouseMove` / `NativeOnMouseLeave`; 8 `EditDefaultsOnly` mark tunables; 3 private `Try*` helpers + `GetMarkSubsystem`; `HoveredMarkNumber`. Class comment gains **§8** and §2 gains the declared `WR-§9`-outcome-1 amendment. |
| `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp` | SOLE owner. Implementations, the ring painter, the mark chrome, the mark colour, two new paint layers, the right-click arm, the mark arms of the left-click, the three new event overrides. |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp` | Extended in place (⛔ no second war-map test file). Tests **28–35**. |

⛔ **NOT touched:** `SiegeMapMark.h` / `SiegeMapMarkSubsystem.{h,cpp}` (744) · `SiegeAssistantSnapshot.{h,cpp}` (746) · `HeroCharacter.{h,cpp}` (748) · `SiegeGameMode` / `SiegePlayerController` (750) · `SummonedUnit.{h,cpp}` / `ClimbableTower.{h,cpp}` (tower wave) · `SiegeAssistantConsoleWidget.{h,cpp}` (`ComposeAppendedInput` is **consumed, byte-unchanged**).
⛔ **No compile, no editor, no MCP, no Git.**

**⭐ TASK-744 LANDED WHILE I WAS AUTHORING, AND I RE-VERIFIED THE REGISTRY AT THE SOURCE RATHER THAN TRUSTING THE BOARD:** `FSiegeMapMark{ int32 Number; FVector2D WorldXY; float RadiusUU; static FString MakeSymbol(int32); }` (`SiegeMapMark.h:59-68`, `MakeSymbol` is `inline` at `:116`, returns `FString()` below `FirstMarkNumber = 1`) and the five subsystem methods (`SiegeMapMarkSubsystem.h:102-110`). ⛔ **Zero divergence — I compile against the pinned shapes character-for-character and changed nothing in them.**

---

## B. THE THREE INPUT PATHS, AS WIRED

| Gesture | Slate override | What runs | Precedence |
|---|---|---|---|
| **LEFT click, empty map** ⇒ **PLACE** | `NativeOnMouseButtonDown` | `TryPlaceMarkAtLocal` → `LocalToMapUV` → `MapUVToWorld` → `AddMark(WorldXY, RadiusUU, Out)` | after markers, after marks |
| **LEFT click, on a mark** ⇒ **NAME IT** | `NativeOnMouseButtonDown` | `FindMarkIndexAtLocal` → `MakeMarkPickSymbol(N)` → `OnPlacePicked.Broadcast` | after markers |
| **RIGHT click, on a mark** ⇒ **DELETE** | `NativeOnMouseButtonDown` (right arm, **before** the non-left absorb) | `TryDeleteMarkAtLocal` → `RemoveMark(Number)` | marks only |
| **WHEEL over a mark** ⇒ **RESIZE** | `NativeOnMouseWheel` | `TryResizeMarkAtLocal` → `StepMarkRadiusPx` → `SetMarkRadius(Number, uu)` | marks only |
| *(hover highlight)* | `NativeOnMouseMove` / `NativeOnMouseLeave` | `HoveredMarkNumber` — **cosmetic only** | — |

**⭐ THE SYMBOL RIDES THE SHIPPED SEAM WITH ⛔ ZERO CONTROLLER CHANGE.** A mark pick broadcasts on the *existing* `FOnWarMapPlacePicked(FName)`; `ASiegePlayerController::HandleWarMapPlacePicked` (`:4981`) moves the symbol **opaquely** — its own comment says it "knows nothing about `PlaceVocabulary` and must not learn" — into `AppendToInput` → `ComposeAppendedInput`. ⇒ `circle_2` flows to the input box through code I did not touch, and **the player still presses Enter himself**.

**⛔⛔ MARKERS OUTRANK MARKS, IN BOTH THE HIT ORDER AND THE PAINT ORDER, AND THE TWO ARE THE SAME ORDER.** Spec item (7) forbids disturbing "the seven place markers' hit-testing and symbol insertion" — and a mark is an arbitrarily large disc the player can drop anywhere, so if marks won, one big circle over `own_castle` would make that marker **permanently unclickable**. Markers are tested first and drawn last (topmost). ⚠️ **Declared consequence:** you cannot place a mark *inside* an existing mark — that click names the outer one. Delete it, or place elsewhere. Playtest-sheet material, ⛔ not a defect.

**Layer order** (each shipped layer moves up, ⛔ order preserved — the TASK-684/685 precedent):
`Elevation(+1) < **MarkRing(+2)** < PoiIcon(+3) < Ally(+4) < Enemy(+5) < **MarkNumber(+6)** < Marker(+7) < Label(+8)`
Two slots, split on purpose: the **ring** is ground annotation and must not hide units; the **number** is the mark's identity — the name the player speaks — and must not be buried under a dot.

---

## C. HOW THE NUMBER IS CENTRED AND STAYS LEGIBLE AT ANY RADIUS

**Centred** — measured, ⛔ not estimated. `FSlateApplicationBase::Get().GetRenderer()->GetFontMeasureService()->Measure()` gives the glyph box; the placement rule is the pure static `FSiegeWarMapProjection::CentreTextTopLeft(Centre, MeasuredSize) = Centre − Size·0.5`. ⚖️ It is a *function* for `W4-R1`'s reason: "in the middle of it" is Jonathan's contract, and a contract inlined in a `const` paint pass is unreadable by any test. Test 33(a) asserts `TopLeft + Size·0.5 == Centre` exactly, at three centres × three box sizes.
⚠️ **Contrast with the seven place markers, whose labels are deliberately LEFT-ANCHORED** because centring "needs the font measure service" (the shipped comment). This feature pays that cost because the spec demands the centre. The measure-service guard (`IsInitialized()`) is **defensive and structurally unreachable from a paint pass** — `NativePaint` only runs under a live Slate app — and the comment says so plainly rather than dressing it up.

**Legible at any radius** — two independent mechanisms:
1. **Size follows the ring:** `MarkNumberFontSizePx(Radius, 0.6, 10, 28)` — a pure static, swept by test 33(b) across all 201 radii in `[12, 240]`: **0** below the floor, **0** above the ceiling, and the size genuinely *moves* (10 → 28), which catches a broken fraction that pinned every circle to the floor and would otherwise pass both clamps.
2. **Contrast comes from an OUTLINE, ⛔ not from the fill** — and that is **this file's own measured finding**, applied rather than re-derived: *"the three overlays that fail [3:1] are EXACTLY the three drawn with NO OUTLINE… no flat colour can clear 3:1 against BOTH ends of a 14.74:1 background by luminance alone, so the repair is an OUTLINE, ⛔ not another re-tint."* The digit uses `FFontOutlineSettings(2, MarkerOutlineColor)` with `MarkerLabelColor` as fill — **the identical pairing that puts the marker glyph at 12.22:1** — so it reads over the ramp's dark end, its light end, a castle shell clamped white, and its own ring alike. ⭐ **Zero new colour constants for the number**, "Bold" typeface (a lone digit has no word-shape to carry it at 10 pt).

---

## D. ⚠️ DECLARED DEVIATIONS / AMENDMENTS — three, none silent (`SC-§15`)

### D-1. `WR-§9` OUTCOME 1 IS AMENDED **FOR THE LEFT BUTTON ONLY**, BY JONATHAN'S OWN DIRECTIVE
*"A click on empty map area does nothing"* was law. His sentence overrides it: an empty-map **left** click now places a numbered mark.
✅ **Why it is cheap and why the airlock is untouched:** the click still emits a **SYMBOL**, never a coordinate — `MARK-§1`'s *"`WR-§6`'s click→symbol trick, applied a second time and for the same reason."*
⭐ **THE RETIRED BEHAVIOUR IS ⛔ NOT DELETED — IT IS A REACHABLE DEGRADE PATH.** The shipped empty-click arm (the `W691-3` two-arm discriminator, `EmptyClickHintText` / `NoSnapshotStatusText`) now runs when the mark lane is unavailable: **no owning local player ⇒ no subsystem**, or **the click landed outside the drawn map rect** (the letterbox). ⇒ the old code still has a real reason to run and is still auditable. ⛔ **The RIGHT button still does nothing on empty map**, so half of outcome 1 stands verbatim.
📌 **Manager action owed:** land the amendment in `CONVENTIONS.md` (`WR-§9` row 1). I did not edit it — that is not mine to write.

### D-2. THE STORE IS WORLD uu, THE TUNABLES ARE WIDGET PX — RECONCILED AT **ONE** NAMED CROSSING
Two laws touch here and both are obeyed:
- the **pinned registry** stores `FSiegeMapMark::RadiusUU` in **world uu**, and 744/746 compile against that field;
- **`MARK-§4`** requires the wheel's tunables to be **widget-space**, separately named, ⛔ never the world-space `GroupRadiusWheelStep`/`Min`/`Max` numbers.

⇒ the wheel steps and clamps in **px**; `FSiegeWarMapProjection::MapLocalPxToWorldRadius` / `MapWorldRadiusToLocalPx` are the **only** crossing. ⛔ The controller's 100 / 200 / 5000 uu appear nowhere in this file, in any form.
⭐ **World-space storage is also right on its own merits:** a mark denotes GROUND. A pixel radius would mean a *different amount of ground* on a different monitor or after a window resize — so `circle_1` would quietly denote different ground than the one the player drew, which is `M-1`'s failure arriving through geometry instead of bookkeeping.
⭐ **The single scalar is PROVEN, not assumed:** `ComputeMapRectLocal` builds a rect of exactly the arena's aspect ⇒ `RectSize.X/(2·HalfX) == RectSize.Y/(2·HalfY)`. Test 31(a) asserts that equality on four panel shapes, so a future edit that broke the aspect preservation is caught **here** rather than shipping subtly-oval marks whose hit test is still a circle.
⚠️ **Honest consequence:** because the clamps are px and the store is uu, a mark whose radius sits outside `[Min, Max]` after a panel resize is pulled back into the window on the next notch (test 32(c)). That is inherent to `MARK-§4`'s widget-space instruction and is stated rather than hidden.

### D-3. THE WHEEL IS **ABSORBED** WHILE THE MAP IS OPEN, EVEN ON A MISS
⛔ **INERT still means INERT: a wheel event not over a mark changes NO state anywhere.** Absorbing is about the *event*, not the *state*.
⚖️ **Same fail-safe direction and same argument as the shipped `NativeOnMouseButtonDown`**, which already absorbs every mouse **button** while the map is up: the map fills the screen, so letting input fall through drives something the player cannot see. `MARK-§4`'s own wording contemplates it (*"`UWarMapWidget` while the map is open and the cursor is over **it**"*) and it adds ⛔ **no third consumer** — it is the same one consumer declining to act.
⚠️ **The alternative is recorded so it is not re-argued:** returning `Unhandled` on a miss would let the notch reach the controller's poll. Refused because it makes the map's **wheel** behave differently from the map's **clicks** — same gesture, same pixels, falling through or not depending on which button — and that inconsistency is how a player learns to distrust a UI.

---

## E. ⛔ THE WHEEL LAW — CONFIRMED SEPARATE **AT THE SOURCE**, NOT ASSUMED

| | mechanism | where |
|---|---|---|
| controller | **POLL** — `WasInputKeyJustPressed(EKeys::MouseScrollUp/Down)` | `ApplyGroupPickWheel`, `SiegePlayerController.cpp:2843-2876` |
| gate | its **only** call site is the pick branch | `PlayerTick`, `:647-657` — `if (GroupPickStage != EGroupPickStage::None)` |
| war map | **SLATE EVENT** on a focused widget | `UWarMapWidget::NativeOnMouseWheel` |

⇒ ⛔ **Not one line, not one symbol and not one file of the controller's path is touched**, and its *"inert outside the pick flow"* property is **literally unchanged**. When the map is CLOSED this widget is `Collapsed` and never receives the event at all. **Exactly two consumers; ⛔ no third may be added without amending `MARK-§4` by name.**

---

## F. ⭐ THE MARK COLOUR, ITS SPACE, AND HOW I CHECKED LEGIBILITY

> **`MarkRingColor = FLinearColor(0.52f, 0.035f, 0.54f, 1.00f)` — `// TRUE LINEAR (Slate tint)`, stated at the declaration, per `WM-§8e`'s absolute clause.**

**⛔ THE COLOUR-SPACE HAZARD WAS HANDLED FIRST, NOT LAST.** This is a **Slate tint** ⇒ **TRUE LINEAR** (Slate sRGB-encodes at draw). The elevation ramp's constants a few hundred lines away are **DISPLAY-ENCODED sRGB** in the same `FLinearColor` type because they become the bytes of an `SRGB = true` texture. ⛔ **I did not touch either ramp constant, did not convert anything, and added no `ToFColor`/`sRGBToLinear`/`Pow`.** The one new constant carries its space in a comment on its own line.

**Why magenta:** five things already occupy this map — the **green** ramp, **blue** ally dots + blue castle icons, **red** enemy dots + red castle icons, **gold** mine icons + gold marker glyphs, **deep emerald** ancient grounds (itself just re-tinted by TASK-722 to survive the green). ⇒ magenta is the one strong hue the tactical palette has not spent, it is the **exact complement of green** (maximum hue separation from the background at both ends), and it reads correctly as *the player's annotation* rather than another faction or resource.

**How I checked — arithmetic, then a test that re-does it:**
- Ramp ends read from the shipped seam, ⛔ not transcribed: `BrightnessToRampColor(0)` = `(16,26,16)`, `(1)` = `(160,255,159)` sRGB bytes → linear → **Y_dark 0.00887**, **Y_light 0.81498**, span **14.74:1** — which **reproduces `WM-§8c`'s recorded 14.74:1 exactly**, so my model is the same model the file used.
- Flat-colour ceiling: `(Y+0.05)² = (Y_light+0.05)(Y_dark+0.05)` ⇒ Y = 0.1752, ratio **3.83:1** — **reproduces `WM-§8c`'s recorded 3.84:1**.
- The constant is **tuned to that optimum**: Y = 0.17457 ⇒ **3.81:1 vs the dark end, 3.85:1 vs the light end.** ⭐ Within ~1% of the ceiling on *both* ends — better than any currently-shipping overlay.
- Distinctness (summed linear channels): **1.20 from team blue**, **1.04 from team red** — the blue channel (0.54 vs red's 0.05) is what separates magenta from red and is the largest single-channel gap in the set. Also far from gold (green channel 0.035 vs 0.72) and from deep emerald (every channel).
- **And the ring is still drawn over a dark rim** (`MarkerOutlineColor`, reused ⛔ never re-typed), because 3.84:1 is a *ceiling*, not a comfort — the file's own outline finding, applied.

**⭐⭐ Test 34 MEASURES all of this instead of asserting it in a comment** — and it catches the exact `WM-§8e` error: sRGB-encoding this constant would raise its luminance to ~0.60 and collapse the light-end ratio to ~1.3:1 ⇒ **the invisible colour-space mistake becomes a red test.** Both the ramp ends and the team colours are read through shipped seams, so a future change to either automatically re-checks the overlay that must survive it.

**⚠️ HONEST LIMIT, ⛔ not glossed:** a contrast ratio and a channel distance are **necessary, not sufficient**, and this is **⛔ not a colour-blind simulation** — a deuteranope may read magenta and red closer than the numbers suggest. The shape tells carry that case (see G). **Jonathan's eye is the acceptance.**

---

## G. HOW MARKS ARE DISTINGUISHED FROM THE ORDER ZONES (`MARK-§4`'s named confusion hazard)

Two different things are now called "circles" and both are resized by the wheel. **Three independent tells**, so no single one carries it:

| | group-order pick zones (shipped) | map marks (new) |
|---|---|---|
| fill | **FILLED** ground decal (`M_SpellReticle`) | **HOLLOW** stroked ring — `MakeLines`, ⛔ never a disc |
| number | none, ever | ⭐ **a NUMBER in the middle** — Jonathan's own tell |
| colour | the reticle material's | **magenta**, a hue nothing else on this map or in the team palette uses |
| space | **world** (ground decal, in the 3D scene) | **map** (UI overlay, only while the map is open) |
| meaning | a transient gesture that **ISSUES AN ORDER** | a persistent named place that **issues nothing** |

⛔ **Hollow is the design, not a shortcut:** a filled disc would also hide the elevation, icons and dots underneath — the content the player opened the map to read.
⚠️ **I could not read `M_SpellReticle`'s tint** (a binary asset, and I hold no editor/MCP), so the colour row is stated as "the material's" rather than claimed. The other four rows are structural and hold regardless. **TASK-751 owns the player-facing reconciliation**; this makes the marks visually unambiguous before it gets there.

---

## H. ⛔ THE AIRLOCK — AND THE ONE GENUINELY NEW THING, FLAGGED FOR QA

⛔ **No `Capture()`, no `EnsureSnapshot()`, and the mark lane never reads the snapshot at all** — not even `GetPlaceNames()`. Zone A untouched, the 552 latch untouched, **no token figure anywhere** (`AS-§12g`).
✅ The map's entire outbound surface is still `FOnWarMapPlacePicked(FName)`; the only value that can ride it for a mark is `MakeMarkPickSymbol(N)`.

**⚠️⚠️ THE ONE NEW THING, SAID LOUDLY BECAUSE A REVIEWER SHOULD STOP ON IT: THIS FILE NOW COMPUTES A WORLD COORDINATE.** A placement click runs `LocalToMapUV` → `MapUVToWorld` and stores `FSiegeMapMark::WorldXY`. **Three properties make it safe, all structural:**
1. ⛔ It goes into a **client-local `ULocalPlayerSubsystem`** and nowhere else. Nothing here prints, serializes, replicates or broadcasts it.
2. ✅ **`MARK-§1` sanctions exactly this shape by name:** *"a mark's centre is resolved on the GAME side by `ResolvePlace`, exactly as `nearest_mine` resolves to a world position today without ever printing one."*
3. ⛔⛔ **IT DOES NOT COME FROM THE ELEVATION BAKE — `WM-§8d`'s firewall, honoured exactly.** It comes from the projection pair's exact X/Y inverse, and an `FVector2D` **has no Z**, so the 1,000-uu-clamped display buffer this same class owns is **structurally unreachable** from a mark. ⇒ the `SHIP-§9` fake-instrument class cannot arise here, now or later.

**M8 (`MARK-§6`):** ⛔ no replicated property, ⛔ no RPC, ⛔ no class-tier change. `M-3` (⛔ not enemy-visible) keeps it free, and it is **deliberately the opposite of `GHOST-§ G-4`** — ⛔ do not "make them consistent."
**`M-4`:** `CloseMap` clears the enemy reveal, the ally dots, the POI census and the hover highlight — and ⛔ **deliberately NOT the marks.** That omission is the feature, and `CloseMap` carries a comment saying so.

---

## I. TESTS — `Tests/SiegeWarMapTest.cpp` **27 → 35 (+8)**

| # | name | the assertion that can FAIL |
|---|---|---|
| 28 | `MarkPickSymbolIsTheMakeSymbolSeamByteForForByte`¹ | widget symbol **==** `FSiegeMapMark::MakeSymbol(N)` for 1..9 (`TestEqualSensitive`) **and** == literal `circle_N`; ⛔ never a bare digit; 0/negative ⇒ `NAME_None`; ⭐ **the end-to-end composition**: `"hold"` + circle 2 ⇒ `"hold circle_2 "`, his second sentence, the empty box, and the marker→mark idempotent-separator case |
| 29 | `MarkHitTestIsRadialAndEmptyMapAnswersNothing` | 1,200 sample points on an empty map ⇒ **0** non-`INDEX_NONE` (⭐ *right-click does nothing* + *wheel outside does nothing*); ⭐⭐ **all four bounding-square corners MISS** — a rect hit box dies exactly there; boundary inclusive; last-match-wins; **number ≠ index** (`M-1`); degenerate radii unhittable |
| 30 | `LocalToMapUVInvertsTheRectAndRefusesOutsideIt` | exact round trip incl. corners; five outside points **refused** *and the out-param left untouched*; degenerate rect refuses |
| 31 | `MarkRadiusScaleIsUniformOnBothAxes` | px-per-uu identical on both axes across 4 panel shapes (⇒ circles, not ellipses); px→uu→px exact; all degenerate inputs ⇒ 0, finite, never negative |
| 32 | `MarkWheelStepClampsAndAZeroDeltaChangesNothing` | ⭐ zero delta unchanged; delta 12.0 and 0.05 are **each one step** (sign, ⛔ not magnitude); both clamps; zero step disables; inverted min/max never negative |
| 33 | `MarkNumberIsCentredAndSizedForEveryRadius` | box centre lands **exactly** on the circle centre (3×3 cases); 201-radius sweep: 0 below floor, 0 above ceiling, and the size genuinely moves; degenerate inputs never 0 pt |
| 34 | `MarkRingColorIsLegibleAtBothRampEnds` | ⭐⭐ ≥3:1 at **both** ramp ends, ends read through `BrightnessToRampColor`; the two ratios balanced within 25% of the re-derived ceiling; ≥0.75 channel distance from team blue **and** red (read through the `W4-R5` accessors); opaque; chromatic |
| 35 | `BuildMarkCirclesYieldsNothingWithoutAMarkStore` | junk-prefilled array is **RESET**; empty ⛔ never partial; `GetMapMarkCount()` 0; open/close cycle |

¹ actual id: `Siegebound.WarMap.MarkPickSymbolIsTheMakeSymbolSeamByteForByte`.

**⛔ WHAT THESE CANNOT SEE, NAMED RATHER THAN IMPLIED (`SC-§32`).** `USiegeMapMarkSubsystem` is a `ULocalPlayerSubsystem`, so a `NewObject<UWarMapWidget>` can never reach one ⇒ **no test below adds, deletes or resizes a real mark, and none pretends to.** ⭐ **That is precisely why every rule was put on a pure seam** (`W4-R1`) — the decisions are asserted; the handlers only look up and act. Still owed to PIE / Jonathan's eye: the live `NativeOnMouseButtonDown` / `NativeOnMouseWheel` dispatch (both `protected`, both need a realized Slate widget), the store's allocator and cap (TASK-744's suite owns those), and whether magenta over green reads well to a human.
⚠️ **Test 35's `M-4` leg is declared WEAK in its own comment** — headlessly the count is 0 either way, so it cannot distinguish "persisted" from "never existed". It is a tripwire pointing a reader at `CloseMap`'s comment, ⛔ not proof.

**SUITE COUNT.** `SiegeWarMapTest.cpp` **27 → 35**. Module snapshot at authoring time: **245** `IMPLEMENT_*_AUTOMATION_TEST` across `Siegebound/Tests/` — ⚠️ **that snapshot already contains TASK-744's `SiegeMapMarkTest.cpp` (9) and the in-flight ghost/recall/respawn/ladder files, so it is ⛔ NOT mine to declare as the batch total.** **TASK-753 re-counts; TASK-754 asserts ONE number.** My contribution is unambiguous: **+8**.

---

## J. 🔎 WHAT QA SHOULD SCRUTINISE HARDEST

1. **D-1 / D-2 / D-3** — the three declared amendments. Each is a place where I chose between two defensible readings; each carries its rejected alternative in a comment.
2. **The precedence ruling** (markers > marks, hit AND paint) — spec item (7) is the reason; check I did not weaken the seven markers anywhere.
3. **`GetMarkSubsystem() const` returning a mutable pointer.** Deliberate, and the same shape `GetOwningLocalPlayer() const` has. The painter's only use is `BuildMarkCircles`, which calls `GetMarks()` — a `const` read ⇒ `NativePaint`'s const guarantee is intact. **⛔ No new `mutable` member was added.**
4. **`FString::Printf` and the chrome namespace.** The three mark status lines are **functions**, not `const TCHAR*` constants, because `Printf` takes a `TCheckedFormatString` whose only ctor is `(const CharType (&)[N])` and is `consteval` (`String/FormatStringSan.h:486-520`) — a pointer format **will not compile**. Language constraint, documented at the declaration.
5. **`LocalToMapUV`'s clamp.** There are two clamps in this feature and only one of them would be a violation. The forbidden one accepts an out-of-rect click and pins it to the rim; **mine runs only after containment is proven and removes a one-ulp division overshoot.** The comment distinguishes them explicitly.
6. **Engine API verified against the installed UE 5.8, ⛔ not from memory** (I cannot compile): `NativeOnMouseWheel/Move/Leave` (`UserWidget.h:1615-1618`) · `MakeLines`'s `TArray<FVector2f>`-by-value overload (`DrawElementTypes.h:230`) · `FCoreStyle::GetDefaultFontStyle(FName, float, const FFontOutlineSettings&)` (`CoreStyle.h:51`) · `FFontOutlineSettings(int32, FLinearColor)` (`SlateFontInfo.h:78`) · `FSlateApplicationBase::IsInitialized/Get/GetRenderer` (`SlateApplicationBase.h:133,559,571`, public) · `FSlateRenderer::GetFontMeasureService` (`SlateRenderer.h:419`) · `FSlateFontMeasure::Measure(FStringView, …)` with an `FString` (engine precedent `SlatePasswordRun.cpp:30`) · `ULocalPlayer::GetSubsystem<T>` (`LocalPlayer.h:365`) · `FPointerEvent::GetWheelDelta` (`Events.h:1081`) · `UE_PI` (already used in this module).
7. **`FVector2f` at every Slate boundary** — the file's standing warnings-as-errors discipline, followed in the ring painter and the number pass.
8. **⛔ Nothing in the mark lane touches a snapshot.** Worth a grep: `Capture`, `EnsureSnapshot`, `GetPlaceNames` appear only on the pre-existing marker/status paths.

---

## K. 🚩 FOR JONATHAN'S PLAYTEST SHEET (designed outcomes, ⛔ not defects)

- **You cannot place a circle inside an existing circle** — that click *names* the outer one instead. Delete it, or place elsewhere.
- **A place marker beats a circle on a click.** Drawing a big circle over `own_castle` does not make the castle marker unclickable — the marker still wins, by design.
- **Deleting circle 2 of 3 leaves 1 and 3.** ⛔ Numbers never renumber (`M-1`), and the status line says so out loud when you delete one.
- **Circles survive closing and reopening the map** (⛔ unlike the paid enemy reveal, which never does).
- **Circles are yours alone** — the enemy cannot see them (⛔ deliberately the opposite of the ghost).
- **The wheel does nothing unless the cursor is over a circle**, and the hovered circle's ring thickens so you can see which one you are about to resize.
- **At nine circles a further click refuses with a line naming the count** — it is not a dead click.
