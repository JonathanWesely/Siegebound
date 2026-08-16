# TASK-560 handoff — [WR-6] `UWarMapWidget`, the C++ painter, the markers, the dots (gameplay-programmer, 2026-08-15)

- **Gate:** `.claude/pipeline/qa/TASK-565.md` — ⛔ **this task names TASK-565 as its gate** (build-master refuses to commit without this naming).
- **Compile:** TASK-566. **Commit:** TASK-570. ⛔ **This task opened NO compile of its own.**
- **Status:** `ready-for-qa`.
- **Discipline honoured:** ⛔ no compile · no build · no Git · no editor · no MCP · no PIE · no `Content/` asset · no `.csv` · no `Build.cs` · no `Tests/` · **no existing source file touched at all.** ⛔ **No token figure is quoted, derived or reasoned from anywhere in this handoff or in either file** (`AS-§12g`, batch-wide ban). Every size claim below is in **chars** or **bytes**.
- **Files created — exactly the two in `names:`, both NEW:**
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\WarMapWidget.h` (656 lines)
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\WarMapWidget.cpp` (858 lines)
- ⛔ **NOT touched (the spec's list, verified by §5's greps):** `SiegeAssistantSnapshot.{h,cpp}` · `SiegeAssistantComponent.{h,cpp}` · `SiegeAssistantConsoleWidget.{h,cpp}` · `SiegeAssistantGrammar.{h,cpp}` · `SiegeAssistantCommand.{h,cpp}` · `ScatterConfig.h` (TASK-557 owns it) · `SiegePlayerController.{h,cpp}` · `SiegePlayerState.{h,cpp}` · `Castle.{h,cpp}` · any Zone builder · any `.csv` · any `Content/` asset · `Build.cs`.
- **Assets referenced by path only (none created, none required to exist today):** UMG target **`/Game/UI/WBP_WarMap`** (TASK-568 builds it FRESH — **absent today, and the class is written to be fully functional without it**) · **`/Game/Data/DA_BattlefieldScatter`** (ships today).

---

## 1. ⭐ THE HEADLINE — WHAT WAS BUILT, IN THE SHAPE THE RULING DEMANDS

**CLICK → SYMBOL. ⛔ NEVER CLICK → COORDINATE.** The whole class is arranged so that the only thing that can leave it is one of the seven `PlaceVocabulary` symbols:

```
OnPlacePicked : DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWarMapPlacePicked, FName, PlaceSymbol)
```

⛔ **That `FName` is the entire outbound surface.** No coordinate, no dot, no count, no marker rect and no arena figure crosses it. ⇒ ✅ **This task spends ZERO prompt characters**, touches no Zone builder, adds no `who` shape, no grammar alternation, no schema field and no place symbol — so **`ZoneA` cannot have moved from its named, dated baseline of 5658 chars (2026-08-05)**, and TASK-564 asserts that against a `Tests/` tree this task never opened.

⭐⛔ **THE FILE CONTAINS NO PLACE SYMBOL LITERAL AT ALL.** The marker list is **read** from `USiegeAssistantSnapshot::GetPlaceNames()` and each position from `ResolvePlace`. ⇒ This file *physically cannot* add an eighth place, and it picks up any future vocabulary change with no edit. (Grep proof in §5.)

⭐ **A MARKER'S HIT BOX IS A WIDGET RECT — `MarkerHitHalfSizePx`, in local pixels, on a panel.** ⛔ Not a world radius. `AS-§21.4` is honoured exactly rather than argued around: no number describing world space is invented anywhere in the pair.

---

## 2. ⛔⛔ THE TASK'S NAMED FINDING — READ THIS BEFORE ANYTHING ELSE (`SC-§15`; spec item (3) calls reporting it *"a successful outcome"*)

### ✅ The read-only route EXISTS and is the one taken — no `Capture()` anywhere

```
GetOwningPlayer()  →  ASiegePlayerController::GetAssistantComponent()   (SiegePlayerController.h:660, public)
                   →  USiegeAssistantComponent::GetTurnSnapshot()       (SiegeAssistantComponent.h:903, public, const)
                   →  GetPlaceNames() / ResolvePlace()                  (SiegeAssistantSnapshot.h:574 / :700, const)
```

**Proof, command + raw result:**

```
$ grep -nE "(->|\.|::)\s*(Capture|EnsureSnapshot|CaptureTurnSnapshot)\s*\(" WarMapWidget.h WarMapWidget.cpp
(no output — exit 1)
```
**⇒ ZERO invocations of `Capture`, `EnsureSnapshot` or `CaptureTurnSnapshot`.** Opening the map triggers no survey of any kind.

### ⚠️⚠️ THE RESIDUAL, AND IT IS A REAL PLAYER-VISIBLE GAP — ESCALATED, ⛔ NOT CODED AROUND

> **`GetTurnSnapshot()` returns null until the player's FIRST console sentence.** `Snapshot` is allocated in `USiegeAssistantComponent::EnsureSnapshot()` (SiegeAssistantComponent.cpp:3215-3224), which is reached only from the turn path, and the accessor's own doc says **"⚠️ Null before the first capture."**
>
> ⇒ **A player who opens the war map before ever typing into the console sees ally dots and (if paid) enemy dots, but NO PLACE MARKERS — and therefore has nothing to click.** The map self-heals the instant one sentence is sent.

⛔ **I did not fix it, and the refusal is deliberate — every available fix crosses a boundary this task may not cross:**

| candidate fix | why it is refused |
|---|---|
| call `Capture()` when the map opens | ⛔ **The exact side effect `WR-§6` forbids in terms.** It would also re-survey mid-conversation and answer a different question from the one the model was asked. |
| give the map its own second snapshot and capture into that | ⛔ A parallel survey — the registry/cache/dirty-flag class `SiegeAssistantSnapshot.h` §4 says is **"rejected on sight"**, plus a second copy of `ResolvePlace`'s answers that can drift from the executor's. |
| prime the component with one capture at `BeginPlay` | ⛔ An edit to `SiegeAssistantComponent.{h,cpp}`, which this task's `names:` block lists as **NOT TOUCHED**. |

🚩 **OWNER QUESTION FOR THE MANAGER — three candidates, none of them me:**
1. **TASK-563** (owns `SiegePlayerController.{h,cpp}`): on the map's open path, if the assistant has no snapshot yet, do nothing / or ask the component to prime one — but that still needs an assistant-side entry point.
2. **A new task on `SiegeAssistantComponent.{h,cpp}`** adding a *display-only* prime that is explicitly not a turn (the honest fix, and the only one that puts the change where the ownership is).
3. **Ship as-is and put it on Jonathan's playtest sheet as an eighth `WR-§9` designed outcome.** ⚠️ My recommendation *only if* (2) is judged too expensive: it is cheap to explain and impossible to mistake for a crash.

**Degradation is observable rather than mysterious:** one `Log`-level line, latched once, naming the cause and pointing at this handoff; plus an on-screen chrome line — *"Place markers appear once you have sent an order in the console."* — shown when the player clicks an empty map that has no markers.

---

## 3. ⚠️ DECLARED DEPARTURES FROM THE SPEC (`SC-§15`) — THREE, ALL NAMED RATHER THAN SILENT

### D-1 ⛔⛔ `NativePaint`, NOT `NativeOnPaint` — **THE SPEC'S SPELLING DOES NOT EXIST IN UE 5.8**

Spec item (1) says *"`NativeOnPaint` / `FSlateDrawElement`"*. Verified against the engine on this machine:

```
$ grep -n "NativeOnPaint\|virtual int32 NativePaint" \
      "C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/UMG/Public/Blueprint/UserWidget.h"
1592:	UMG_API virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
        const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const;
```
**ZERO hits for `NativeOnPaint` in that header.** The `NativeOn…` prefix belongs to the **input** events (`NativeOnMouseButtonDown`, `NativeOnKeyDown`, …); the paint override is `NativePaint`.

⚖️ **Why the departure is the fix and not a liberty:** overriding the spelled name would have compiled cleanly as a **brand-new function the engine never calls**. The map would have rendered nothing while every property readback looked correct — the precise silent-failure shape this project's UMG history warns about. ✅ **`FSlateDrawElement` — the load-bearing half of the instruction — is used exactly as specified.**

### D-2 ⭐ THE ARENA FALLBACK IS THE **CDO**, NOT A "named fallback constant"

`WR-§6` says *"Config absent ⇒ a named fallback constant + log once."* I shipped a named fallback **function** reading `GetDefault<USiegeScatterConfig>()->ArenaHalfExtent`.

⚖️ **`SC-§34` prefers this outright** — *"THE STRUCTURAL ESCAPE, PREFERRED WHERE AVAILABLE: DERIVE AT RUNTIME FROM THE ASSET INSTEAD OF TRANSCRIBING A NUMBER."* A transcribed `(26000, 12000)` here is verbatim the stale-derived-constant defect `SC-§34` exists to prevent: a legal number in the right type that compiles, passes every test, and quietly draws the wrong battlefield the day the arena changes size. ✅ **There is not one hand-typed arena dimension in the pair** (§5 grep).
- **Zero-divide guard:** `FSiegeWarMapProjection::MinArenaHalfExtentUu = 1.f`, applied **before** every division — copied from, not re-decided against, the shipped `FMath::Max(ScatterConfig->ArenaHalfExtent.X, 1.f)` at `BattlefieldScatter.cpp:566` and `:1352`.
- ✅ **Verified the number did not move under me:** TASK-557 is editing `ScatterConfig.h` in parallel; `git diff -- ScatterConfig.h | grep -c "^[+-].*ArenaHalfExtent = "` → **0**. `ArenaHalfExtent` is untouched (correctly — `WR-§2` row 13: the arena did not grow, only the castle).

### D-3 📌 A NEW LOG CATEGORY `LogSiegeWarMap` (an addition over the `names:` block)

Declared in `WarMapWidget.h`, defined in the `.cpp` — the shipped per-feature idiom (`LogSiegeInputLayout`, `LogSiegeNavDiag`, `LogSiegeSettings`, `LogSiegeFeedback`). ⚖️ **Reason it is not `LogSiegeAssistant`:** the map is a display, not part of the prompt lane, and TASK-565 re-runs coupling sweeps — a shared category would bury map noise inside the one log a reviewer reads to audit the airlock.

---

## 4. ⛔ `SC-§33` — THE TRAILING-DEFAULT LAW, DISCHARGED WITH THE PASTED CALL-SITE GREP

**Verdict up front: `SC-§33` does not fire, and here is the run rather than the assertion.**

**(a) No existing function signature was modified at all.** The task is NEW FILES ONLY:
```
$ git status --porcelain | grep WarMapWidget
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h
```
Both **untracked (`??`)**. ⛔ Not one ` M` line in the whole tree belongs to this task (the modified files are TASK-557's five and TASK-561's console pair, plus art).

**(b) This task DOES introduce two trailing defaults, on ONE brand-new function**, and they are declared rather than hidden:
```cpp
static UWarMapWidget* CreateAndAddToViewport(
    APlayerController* OwningController,
    TSubclassOf<UWarMapWidget> MapClass = nullptr,   // trailing default #1
    int32 ZOrder = 0);                                // trailing default #2
```
⚖️ **`SC-§33`'s own scope clause excludes it, quoted:** *"this binds a DEFAULTED parameter added to a function that ALREADY HAS CALL SITES. ⛔ It does not bind a brand-new function (`FSiegeAssistantRegionStatics::IsPointInRegion` shipped with zero call sites by design and owes nothing here)."* This function is the same shape and is **copied from `USiegeAssistantConsoleWidget::CreateAndAddToViewport`**, which carries the identical two defaults.

**(c) THE SWEEP, RUN ANYWAY, WITH THE COMMAND AND THE RAW COUNT.** Every symbol this task introduces, across the entire `Source/` tree:
```
$ grep -rn "UWarMapWidget::\|FSiegeWarMapProjection::\|OpenMap(\|CloseMap(\|ToggleMap(\|ReceiveEnemyReveal(\
\|ClearEnemyReveal(\|BuildMarkerRects(\|WorldToMapUV(\|ComputeMapRectLocal(\|MapUVToLocal(\
\|FindMarkerIndexAtLocal(\|ResolveArenaHalfExtent(\|RefreshAllyDots(\|SetAllyRefreshTimerEnabled(\
\|GetReadOnlySnapshot(" Source/ --include=*.h --include=*.cpp --include=*.cs | wc -l
68

$ (same grep, -l)
Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp
Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h
```
**RAW COUNT: 68 hits, exactly 2 files — both new, both mine.**
**Classification: all 68 are (i) declarations and definitions inside the new pair. ZERO pre-existing call sites exist for any of them, so there is nothing to update, nothing to leave at a default and nothing to name to another task.** ✅ *A sweep that finds every call site already correct is a RESULT and is reported as one* (`SC-§22`).

**(d) Named forward: TASK-563 will become `CreateAndAddToViewport`'s first external caller.** ⚠️ It must pass the `WBP_WarMap` class explicitly once TASK-568 lands, or the map silently falls back to the bare C++ class — functional, but with no background panel and no buttons.

---

## 5. ⛔ PURITY SWEEPS — THE AIRLOCK CLAIM, RUN RATHER THAN ASSERTED

**Sweep 1 — comment-stripped scan for every forbidden symbol.**
```
$ grep -vE "^\s*(\*|//|/\*)" WarMapWidget.h WarMapWidget.cpp | grep -nE \
  "Capture\(|BuildZone|ComposeTurn|GBNF|Grammar|SpendGold|EnemyRevealCost|SetPause|TimeDilation|\
GetFirstPlayerController|26000|12000|own_castle|ancient_ground|nearest_mine"
521:WarMapWidget.cpp:  TEXT("Deliberately NOT fixed here: forcing a Capture() is the exact side effect the WAR-ROOM law forbids. ...")
```
**RAW COUNT: 1 hit. Classification: (ii) it is a LOG STRING LITERAL, not a call** — it lives inside `TEXT(...)` and is the diagnostic that explains the §2 finding. The stricter invocation grep in §2 returns **zero**. ⇒ **ZERO code references to `Capture`, any Zone builder, the grammar, `SpendGold`, `EnemyRevealCost`, pause, time dilation, `GetFirstPlayerController`, or any place-symbol literal.**

**Sweep 2 — the remaining hits are all comment prose, enumerated so QA's own grep lands on the explanation rather than on a surprise:**

| pattern | hits | where | classification |
|---|---|---|---|
| `Capture(` | 8 | `.h` 242/244/248/259/474 · `.cpp` 463/465/481 | (ii) prose stating the prohibition + 1 log string |
| `own_castle` / `ancient_ground` / `nearest_mine` / `"mid"` / `"hero"` | 2 | `.h` 225/227 | (ii) prose quoting the `DEV-01` reasoning. ⛔ **Zero in `.cpp`; zero in code.** |
| `zoneA_tok` | 1 | `.h` 208 | (ii) prose naming the symbol to say it **has never been printed**. ⛔ **No token FIGURE is quoted** — the ban is on the number, and there is no number. |
| `SpendGold` / `EnemyRevealCost` | 5 | `.h` 50/306/307 · `.cpp` 848/849 | (ii) prose naming TASK-563 as the owner |
| `26000` / `12000` | 3 | `.h` 96/606 · `.cpp` 495 | (ii) prose — two of them say **"NOT A TRANSCRIBED (26000, 12000)"**. ⛔ **Zero numeric literals in code.** |
| shipped team colour literals | 0 | — | ✅ read from `GetDefault<UCombatantHealthBarComponent>()`, never re-typed |

**Sweep 3 — actor iteration.**
```
$ grep -n "TActorIterator" WarMapWidget.cpp
576:	for (TActorIterator<ASummonedUnit> UnitIt(World); UnitIt; ++UnitIt)
588:	for (TActorIterator<AHeroCharacter> HeroIt(World); HeroIt; ++HeroIt)
$ grep -nE "#include.*(Castle|AncientGround|GoldNode|CaptureZone)\.h" WarMapWidget.h WarMapWidget.cpp
(no output)
```
**Exactly two iterators, both in `RefreshAllyDots`, both asking "where are MY units" — a question no shipped finder answers.** ⛔ **No place iterator, and no include that could reach one.** `ResolvePlace` remains the single owner of place → position.

---

## 6. WHAT EACH SPEC ITEM PRODUCED

| spec item | what shipped |
|---|---|
| **(1)** C++ painter; hit test against the same rects | `NativePaint` + `NativeOnMouseButtonDown` **both call the one `BuildMarkerRects`** against the geometry Slate hands them. ⛔ No cached rects, no second geometry, no UMG child widget per marker. Drawn with `FSlateDrawElement::MakeBox` / `MakeText` on a texture-less `FSlateColorBrush`. |
| **(2)** markers = the seven, at `ResolvePlace` positions | Read from `GetPlaceNames()` in fixed vocabulary order; each position from `ResolvePlace` (return value checked — it leaves the out-param **untouched** on failure, so an unresolved place simply has no marker). Empty-map click ⇒ one chrome hint, nothing else. |
| **(3)** snapshot hazard | §2 above. Read-only route taken; residual reported as a FINDING. |
| **(4)** projection from `ArenaHalfExtent`, pure + testable | `FSiegeWarMapProjection` — a plain static library (the `FSiegeAssistantRegionStatics` / `FSiegeCombatStatics` shape), **four pure functions, no world, no actor, no state**: `WorldToMapUV` · `ComputeMapRectLocal` · `MapUVToLocal` · `FindMarkerIndexAtLocal`. All four headlessly testable by TASK-564. |
| **(5)** ally dots @ `AllyDotRefreshInterval` 0.25 s, `BlueBarColor` | `AllyDotRefreshInterval` (EditDefaultsOnly, default `0.25`, ClampMin 0.05). Colour = `GetDefault<UCombatantHealthBarComponent>()->BlueBarColor`. Own-team `ASummonedUnit` + `AHeroCharacter`, alive only, team read from `ASiegePlayerState::GetTeam()` — ⛔ **never guessed** (no `PlayerState` ⇒ empty list, not a Blue default). |
| **(6)** enemy dots, held and cleared on close | `ReceiveEnemyReveal` / `ClearEnemyReveal`; colour `RedBarColor`; `CloseMap()` calls `ClearEnemyReveal()` **unconditionally, behind no flag**; `NativeDestruct` clears again. ⛔ Nothing here requests, prices, validates or spends. |
| **(7)** no pause; no new spatial primitive | ⛔ No `SetPause`, no time dilation, no input-mode change anywhere in the pair (Sweep 1). No place symbol, grid cell, snap radius, coordinate field or `who` shape added. |
| **degrade without the BP** | `CreateAndAddToViewport` falls back to `UWarMapWidget::StaticClass()`; `SObjectWidget::OnPaint` routes `NativePaint` regardless of tree contents (`SObjectWidget.cpp:146`), so **markers and dots still draw and still hit-test with no `WBP_WarMap` at all.** All three optional children are `BindWidgetOptional`. |

---

## 7. 🚩 WHAT QA SHOULD SCRUTINISE HARDEST

1. **⚠️⚠️ THE §2 FINDING IS THE ONE THAT MATTERS.** Please confirm the refusal is the right call and route the owner question. Everything else here is ordinary widget code; this is the only place a boundary was left deliberately open.
2. **⚠️ `OpenMap()` sets root visibility to `Visible`, NOT the console's `SelfHitTestInvisible`.** The one-word difference is load-bearing: `SelfHitTestInvisible` means *"my children can be clicked, I cannot"*, so `NativeOnMouseButtonDown` would never fire and **every marker would be inert while looking perfectly painted.** ⛔ TASK-568 must not "fix" this on the WBP by setting the root to `SelfHitTestInvisible`.
3. **⚠️ CURSOR/POSTURE IS NOT MINE AND IS A REAL GAP UNTIL TASK-563 CLOSES IT.** A hit test needs a visible cursor and a mouse-capable input mode; the posture owner is `ASiegePlayerController::ApplyCursorInputState`. **This is the same gap TASK-444 flagged for the console** — flagged again here, not fixed, because the controller is TASK-563's file.
4. **⚠️ NOBODY BINDS `OnPlacePicked` YET.** The click seam is a delegate rather than a direct call into the console, deliberately: a direct call would require `#include`ing TASK-561's file and **guessing a method name its own spec calls a HYPOTHESIS (`SC-§20`)**. ⇒ **TASK-563 is the natural binder** — it is the only task that owns both the map instance and the console instance. ⛔ **Until it binds, a click paints a status line and reaches nothing.** Please make that an explicit line item on TASK-563.
5. **⚠️ THE ENEMY-DOT INTERFACE CONTRACT IS WORLD-SPACE `(X, Y)` IN UU** — not map UV, not screen pixels. `TArray<FVector2D>` cannot tell the two apart, and a silent mismatch would put every red dot in a plausible wrong place. **TASK-563's `ClientReceiveEnemyReveal` must send world XY.** Stated in the method's doc comment too.
6. **⚠️ A TIMER, NOT `NativeTick`, AND THE REASON IS AN ENGINE FACT WORTH RE-CHECKING.** `UUserWidget` is declared `meta=(DisableNativeTick)` (`UserWidget.h:279`) and `UpdateCanTick` re-enables native ticking for a `UWidgetBlueprintGeneratedClass` only when `ClassRequiresNativeTick()` says so (`UserWidget.cpp:2358-2361`). A `NativeTick` override would run on the bare C++ class and **could stop running the moment TASK-568 reparents `WBP_WarMap` to it** — working right up until the art lands. The timer is world-owned and immune; it is cleared in **both** `CloseMap` and `NativeDestruct` (a widget torn down while open never reaches `CloseMap`).
7. **⚠️ PAINT LAYERING vs THE WBP'S BUTTONS.** `NativePaint` draws at `LayerId + 1..4`, i.e. **above** every child the WBP supplies. Buttons stay fully **clickable** (Slate hit-tests children first, independently of paint layer), but a dot could draw *over* a button that sits inside the map rect. ⇒ **A layout note for TASK-568: keep the reveal/close buttons outside the central map area,** or accept the overlap. Not a code defect; flagged so it is not discovered as one.
8. **⚠️ THE OPTIONAL-CHILD NAME CONTRACT FOR TASK-568** (all `BindWidgetOptional`, none required): **`RevealButton`** (`UButton`) · **`CloseButton`** (`UButton`) · **`StatusTextBlock`** (`UTextBlock`). Names cloned from `USessionMenuWidget`. A WBP naming none of them still compiles and still works; the BIEs (`OnWarMapOpenStateChanged`, `OnWarMapStatusLine`, `OnWarMapRevealStateChanged`) fire either way.
9. **⚠️ ON-SCREEN CORRECTNESS IS JONATHAN'S PIXEL CHECK, ⛔ NEVER INFERRED FROM A PROPERTY READBACK.** Nothing in this handoff claims the map *looks* right — it has never been rendered. Marker size, dot size, label placement, the letterbox and the chrome colours are all **feel tunables flagged for his pass**.
10. **⛔ AND THE UMG HISTORY LAW IS OBEYED BY CONSTRUCTION:** `WBP_WarMap` must be **BUILT FRESH**, ⛔ never duplicate-and-reparent. That path has silently broken RUNTIME repaint on this project before and cost ~9 wasted fixes. Because the markers and dots are painted in C++, **the WBP is chrome only** — which is precisely why `WR-§6` chose this rendering route.

---

## 8. ⚠️ THE PIXEL NUMBERS THIS FILE INTRODUCES — DECLARED, AND WHY NONE OF THEM IS AN `AS-§21.4` VIOLATION

`MarkerHitHalfSizePx` (18) · `MapPaddingPx` (48) · `MarkerDrawHalfSizePx` (7) · `DotDrawHalfSizePx` (3) · `MarkerLabelGapPx` (6) · `MarkerLabelFontSize` (11).

⚖️ **`WR-§5` already recorded the distinction, for `InteractRadius`, and it applies verbatim:** `AS-§21.4` forbids inventing a radius for a **PLACE SYMBOL**, because that radius becomes a **semantic claim the model reasons over** and a wrong value **silently mis-selects units**. **These are pixel affordances on a panel** — the player feels them directly with the mouse, they can never mis-select anything, and they can never reach a prompt. ✅ Permitted, all flagged as feel tunables. The first two are `EditDefaultsOnly` (tunable with no recompile); the last four are file-static chrome, matching the console's `TranscriptFontSize` / `StatusFontSize` precedent.

⚠️ **One deliberate asymmetry worth noting: the hit rect (18) is larger than the drawn glyph (7).** A marker the player cannot reliably click reads as a broken map; generous targets are the standard fix and cost nothing, because the rect is not a claim about anything.

---

## 9. 📌 M8 DECLARATION (`WR-§8` — ⛔ NOT the last three batches' boilerplate)

⛔ **This class adds no replicated property, no new replicated class, no new relevancy tier and no RPC.** A `UUserWidget` is **client-local by construction** — nothing here crosses the wire. ✅ Ally dots need nothing new: own-team actors are already relevant to their own client. ⚠️ The **two RPCs** `WR-§8` declares for this batch (`ServerRequestEnemyReveal` / `ClientReceiveEnemyReveal`) belong to **TASK-563's `ASiegePlayerController`**, because gold is authority-owned. ⛔ **`GetFirstPlayerController` is not used** (Sweep 1). ⚠️ **Honest limitation, recorded rather than hidden:** on a listen server the client already holds the enemy actors under Tier-B relevancy, so the paid reveal is an **economy/UI gate, ⛔ not an anti-cheat boundary and ⛔ not concealment** — this widget simply declines to draw what it was not handed.

---

## 10. ⚠️ FLAGGED AND UNRULED — ⛔ NOT DECIDED HERE

- **D6 frozen vs live enemy dots.** Shipped the **frozen** default (`WR-§7`), and the class is agnostic: if D6 flips to live, TASK-563 re-pushes on an interval and **not one line here changes**.
- **D7 map background art.** ⛔ Not this task. Nothing here draws a backdrop; the WBP's optional background panel is the art seam.
- **D8 whether `SM_POI_01..04` become nameable places.** ⛔ Not assumed. They have no symbol and no `ResolvePlace` entry, so they simply do not appear. ⚠️ If Jonathan says yes, **this file needs no edit at all** — it reads `GetPlaceNames()` — but the cost lands squarely on `PlaceVocabulary`, Zone A's byte freeze, `ResolvePlace`, the grammar and an `AS-§21.4` ruling.
- **D9 whether the map pauses.** ⚠️ **A DOCUMENT TENSION, REPORTED NOT RESOLVED:** the dispatch lists D9 as UNRULED, while `WR-§6`'s pause row and spec item (7) both rule **NO PAUSE** in terms. **I followed the law (no pause)** and am recording the tension so the manager can reconcile the two documents rather than have a later reader discover it.

---
---

# ⛔ APPENDED 2026-08-15 — THE `C2248` REPAIR (spec item **(R)**, ruling `W4-R5`), QA LOOP 1 of 3

⛔ **NOTHING ABOVE THIS LINE WAS EDITED — it is a dated record.** ⭐ **THE FULL REPAIR RECORD IS THE NEW FILE `handoffs/TASK-560-repair-programmer.md`** (board `(R7)` requires a new file; this dispatch said "append" — both are satisfied, and the reconciliation is declared as `SC-§15` D-2 there). This section carries the three items the dispatch names explicitly. **Status → `ready-for-qa`; gate = `qa/TASK-584.md`.** ⛔ **FILE-ONLY: no compile, no Git (zero git commands), no editor (left CLOSED), no MCP, no PIE.**

**Three files. Four lines of new code, two changed lines, one changed comment line, one changed include note.** ⛔ Fields stay `protected`; ⛔ no literal hardcoded; ⛔ `CombatantHealthBarComponent.cpp:110` untouched; ⛔ `WarMapWidget.h` not opened for edit (**0** changed lines) ⇒ the `WarMapWidget` pair's diff is **5 changed lines**, inside the `(R5)` ~8 ceiling.

### 1. The added accessors — `CombatantHealthBarComponent.h`, existing `public:` block, after `HideBar()`, before `protected:`

```cpp
	static FLinearColor GetDefaultBlueBarColor();
	static FLinearColor GetDefaultRedBarColor();
```

…each with the `WR-§6` / `W4-R5` doc comment stating they read the CLASS DEFAULTS, that `static` is the mechanism (a static member reads its own class's `protected` members ⇒ the two `UPROPERTY`s stay `protected` and gain no writable surface), that they are ⛔ **not** `UFUNCTION`s, and that zero parameters is deliberate — a team-parameterised accessor would have forced `Siegebound/TeamId.h` into a header that does not include it. ⛔ **No `#include` was added to that header.** And in `CombatantHealthBarComponent.cpp`, appended after `HideBar()`:

```cpp
FLinearColor UCombatantHealthBarComponent::GetDefaultBlueBarColor()
{
	return GetDefault<UCombatantHealthBarComponent>()->BlueBarColor;
}

FLinearColor UCombatantHealthBarComponent::GetDefaultRedBarColor()
{
	return GetDefault<UCombatantHealthBarComponent>()->RedBarColor;
}
```

(each above a comment recording the `static`-reads-`protected` mechanism and the ⛔ deliberate absence of a null branch — `GetDefault<T>()` on a statically-linked native `UCLASS` cannot return null). ✅ **`GetDefault<T>()` needs no new include, verified at the engine, not assumed:** `Actor.h:11` → `Templates/SubclassOf.h:6` → `UObject/Class.h:63` → `UObject/UObjectGlobals.h` (where it is declared at `:2194`).

### 2. The call-site change — `WarMapWidget.cpp`, located by symbol (`SC-§18c`), in `PaintMapContents`

```cpp
-	const UCombatantHealthBarComponent* const PaletteOwner = GetDefault<UCombatantHealthBarComponent>();
-	const FLinearColor AllyColor = PaletteOwner->BlueBarColor;    // C2248
-	const FLinearColor EnemyColor = PaletteOwner->RedBarColor;    // C2248
+	const FLinearColor AllyColor = UCombatantHealthBarComponent::GetDefaultBlueBarColor();
+	const FLinearColor EnemyColor = UCombatantHealthBarComponent::GetDefaultRedBarColor();
```

⭐ **The comment's ARGUMENT survives verbatim** — read from the shipped owner, never re-typed, a future palette change reaches this map with no edit, same structural escape as `ResolveArenaHalfExtent`. **Exactly one line of it moved**, the mechanism sentence: *"The CDO carries UCombatantHealthBarComponent's C++ defaults"* → *"The accessors return the owner's own class defaults (W4-R5)"*. The `#include` was **kept**, its note re-pointed: `// the SHIPPED team palette - GetDefaultBlueBarColor() / GetDefaultRedBarColor()`.

### 3. ⛔ EVIDENCE THAT TASK-579's WORK SURVIVED — post-edit read-back, quoted

- `:87` — `static const TCHAR* NoSnapshotStatusText = TEXT("No place markers yet - send your commander one order in the console and they appear. The console opens over this map, so you do not have to close it.");`
- `:401` + `:413-415` (**the first open**) — `// ⭐ TASK-579 - THE FIRST-OPEN STATUS LINE (WR-§9 row 12).` … `if (GetReadOnlySnapshot() == nullptr)` → `ShowNoSnapshotHint();`
- `:871` + `:876-878` (**the empty click**) — `// ⚠️ TASK-579 MOVED THIS DISCRIMINATOR FROM Markers.Num() TO THE SNAPSHOT, AND IT IS A` … `if (GetReadOnlySnapshot() == nullptr)` → `ShowNoSnapshotHint();`
- `:593` (**the latch rides the EXISTING ally-dot timer**) — `// ⚠️ TASK-579 POINTED THIS AT HandleMapRefreshTimer RATHER THAN AT RefreshAllyDots DIRECTLY.`
- `:910-917` / `:929-935` (**the one-way latch, one writer each way**) — `// ⭐⛔ THE LATCH IS RETIRED BY EVERY LINE, AND RE-ARMED ONLY BY ShowNoSnapshotHint() BELOW,` … `bShowingNoSnapshotHint = false;` … `void UWarMapWidget::ShowNoSnapshotHint()` … `bShowingNoSnapshotHint = true;`
- `WarMapWidget.h` — ⛔ **not edited at all**, so its eleven TASK-579 doc/`StatusTextBlock` sites are untouched by construction.

⭐ **The arithmetic that proves nothing else moved:** every TASK-579 site **above** the palette block kept its original line number; every site **below** it moved by exactly **−1**, the single net line this repair removed. ⛔ A revert, re-order or re-format could not produce a uniform −1.

### 4. The standing properties, restated

⛔ **`SC-§33`:** both new functions take **zero parameters** ⇒ it cannot fire structurally; the sweeps are pasted and classified in the repair handoff (new symbols: **8** hits, **0** outside the two owned pairs, **2** call sites, both new; the protected fields: **20** hits, and ⛔ **no file outside the owner reads either one**, so there is no second `C2248` waiting). 🔒 **Airlock untouched by construction** — `SiegeAssistantSnapshot/Vocabulary/ZoneATest` are **absent from the diff**, Zone A stays byte-frozen at **5658 chars**, ⛔ **no token figure quoted** (`AS-§12g`). ✅ The outbound surface is still **one delegate carrying one `FName`**, and the file still contains **no place-symbol literal**. 📌 **M8: no replicated property, no new class, no new tier, no RPC.**
