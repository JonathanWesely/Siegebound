# TASK-123 — WBP_UnitHealthBar: diagnose + repair the 1b "fill never drops" bug (editor/MCP)

**Agent:** art-director
**Date:** 2026-07-10
**Editor/MCP:** up (127.0.0.1:8000). Live PIE run in L_Arena performed and cleanly stopped.
**Verdict:** **ROOT CAUSE IS NOT ASSET-SIDE.** `WBP_UnitHealthBar` is verified correct in every dimension, and every runtime prerequisite for the fill to drop is satisfied in the current freshly-compiled build. **No asset change was made** (nothing to fix on the widget). One concrete, readable **C++/runtime anomaly** surfaced (hero overhead bar hidden) — routed to gameplay-programmer, NOT my lane. The "fill visibly drains" pixel confirmation could not be executed headlessly (documented below) and remains a human WATCH.

Per STEP 2 of the spec: an asset-side cause would be mine to fix; a C++/data cause means STOP + report. **This is the latter.** I did not paper over anything with an asset workaround.

---

## What the discriminators said — every candidate cause ELIMINATED

### Asset-side candidates — all eliminated by direct readback (no guessing)

1. **Reparent — CORRECT.** `get_parent(WBP_UnitHealthBar)` = `/Script/GitClaudeUnrealTest.UnitHealthBarWidget`.
2. **EventGraph wiring — CORRECT.** `read_graph_dsl(EventGraph)`:
   - `OnHPChanged(CurrentHP, MaxHP)` → `if (MaxHP > 0)` → `Bar.SetPercent(CurrentHP / MaxHP)`
   - `SetTeamColor(R,G,B)` → `Bar.SetFillColorAndOpacity(MakeLinearColor(R,G,B,1.0))`
   - Both events target the **same single** `GetBar` variable (the ProgressBar named `Bar`).
3. **Percent property binding (the leading suspect) — STRUCTURALLY IMPOSSIBLE, ELIMINATED.** `list_variables` = `[]` (no member vars) and `list_graphs` = only `EventGraph` (no auto-generated `GetPercent_*` binding graph); `list_functions` shows no implemented local functions. A UMG property binding on `Percent` needs a float-returning function/variable to bind to — **none exists** — so no per-frame binding can overwrite `SetPercent`. (The WidgetBlueprint's editor-only `Bindings` array is not readable via ObjectTools — it redirects to the CDO — but with zero bindable float sources and no binding graph, an effective binding cannot resolve to anything.)
4. **Wrong `GetBar` target — ELIMINATED.** `WidgetTree.Bar` resolves to a `ProgressBar`; both events drive it; runtime log shows **no "Accessed None"** on the SetPercent target. There is no second/competing visible bar in the graph's drive path (donor `WBP_CastleHealthBar` is a single-ProgressBar widget).
5. **Render/brush defect — no defeating config.** `Bar` design-time: standard `ProgressBar`, `Percent=1.0`, `FillColorAndOpacity={1,1,1,1}` (white design default), `Visibility=HitTestInvisible`, `DrawSize` honored (90×12 on the live component), `enableFillAnimation=false` (fill jumps straight to percent — no animation lag). Fill brush uses the engine `DefaultWhiteGrid_Low` material, which is cosmetic — a standard ProgressBar always masks its fill to the `Percent` width regardless of brush. Nothing here prevents the fill from shrinking with `SetPercent`.

### C++ / data-side candidates — all runtime prerequisites SATISFIED (so no C++ fill bug is manifest here either)

Live PIE in `L_Arena` (bot fielded Hero + Knight ×4 + Miner ×2 + Pikeman + Cavalry; player fielded nothing — no-input headless):

6. **`BarWidget` null (TASK-122 WARN-1) — ELIMINATED.** Both hero and knight `HPBarWidget` components report `WidgetClass = /Game/UI/WBP_UnitHealthBar.WBP_UnitHealthBar_C`. That property is set **only** by `SetWidgetClass(LoadedWidgetClass)` at `HealthBarComponent.cpp:75` — i.e. AFTER the class-resolve guard — and since that class IS a `UUnitHealthBarWidget` subclass, `BarWidget = Cast<UUnitHealthBarWidget>(GetWidget())` is **non-null**. Corroborated: the once-per-run "HealthBarWidgetClass unresolved" log **never fired**.
7. **`MaxHP == 0` data trap — ELIMINATED.** Live nonzero max on every type: Hero 200, Knight 200, Pikeman 100, Cavalry 140, Miner 30. The widget's `if MaxHP > 0` guard passes for all → `SetPercent` is NOT skipped.
8. **Poll is live + always-visible C++ confirmed.** Knight `HPBarWidget.bVisible = true` at FULL HP (200/200). Under the old hide-at-full logic a full-HP unit would be hidden — so this confirms the reversed always-visible C++ (TASK-122) is the running code, the poll timer is armed, and `OnHPChanged(GetHealthCurrent(), GetHealthMax())` is being pushed every poll (it sits unconditionally after the visibility block, guarded only by the now-proven non-null `BarWidget`).
9. **No runtime errors.** `LogBlueprint` Warning/Error during PIE = none. No divide warning, no "Accessed None".

**Chain of consequence:** `Bar` non-null + `MaxHP>0` + poll pushing `OnHPChanged` each frame ⇒ `SetPercent(Current/Max)` executes every poll on a standard ProgressBar ⇒ the fill WILL render at that fraction. There is no asset defect and no manifest C++ fill defect in the current build.

---

## Why I could not POSITIVELY watch the fill pixels drop (honest limitation — not faked)

The decisive "read the live ProgressBar `Percent` after damage" test is **not executable with this MCP toolset**:
- **No headless damage injection.** The `USiegeCheatManager` exec cheats (`ApplyTestDamage`) are not invokable — this MCP build exposes **no UFUNCTION/console/exec tool** (build-master's TASK-120 finding, re-confirmed). `set_properties` on the live actor's `CurrentHP` **failed** ("could not be set" — it's a protected, non-editable UPROPERTY). In a no-input Play-vs-Bot the player fields no defenders/towers, so the bot units advance without taking combat damage — every unit stayed at full HP. So no unit could be brought below full by any means available to me.
- **Live widget `Percent` is unreadable.** The `UWidgetComponent`'s created `Widget` (the live `UUnitHealthBarWidget` instance) is a transient object pointer that ObjectTools refuses to serialize ("could not be read: Widget"; also absent from the component's 158 `list_properties`). So I cannot walk component→Widget→`WidgetTree.Bar` to read the live `Percent`/`FillColorAndOpacity`.
- **Screen-space Slate is uncapturable.** `CaptureViewport` renders an offscreen scene capture (editor viewport, axis gizmo visible) — it does **not** include the game viewport's screen-space UMG bars. This is the standing TASK-112 WATCH.

Therefore the "**fill visibly drains AND is team-tinted, both teams, on hero + unit + tower**" hard gate is **owed to Jonathan's eyes** (or a live-input desktop). It could not be machine-observed. I did not fabricate a pass.

---

## Secondary finding (READABLE, concrete) — route to gameplay-programmer, NOT asset-side

**Hero overhead bar is HIDDEN.** `BP_HeroCharacter_C_0.HPBarWidget.bVisible = false`, persistent across a multi-minute run (re-read twice), while the bot Knight's is `true` — same code, same prerequisites (hero alive at 200/200, `bShowHealthBar=true`, `WidgetClass` resolved, `MaxHP=200`). Because `WidgetClass` is populated (proving the hero's BeginPlay armed the poll timer), the hero's `PollHealth` IS running but evaluating `bShouldShow = bShowHealthBar(true) && bAlive` to **false** — i.e. `IsHealthBarActorAlive()` (`= !AHeroCharacter::IsDead()`) returned **false** for a full-HP hero. That is a C++/hero-state issue in the `IsDead()`/alive path (or a pre-match state), and it **hides** the bar — the OPPOSITE of Jonathan's "visible but frozen" complaint, and hero-only (does not affect units/towers). Not the reported bug, but a real defect worth a ticket. **Not my lane to fix** (Source/ C++).

**No tower/building could be tested:** no `ABuilding` was ever present in the run (towers/walls are built mid-match; the bot built none in this session). So the "tower" leg of the gate is untested here.

---

## What I changed
**Nothing.** `WBP_UnitHealthBar.uasset` was NOT modified or saved (verified correct; no asset fix warranted). Did not touch `WBP_CastleHealthBar` or `WBP_MainMenu` (their pre-existing dirty state is untouched). No Git. My single write attempt (live `CurrentHP`) failed, so PIE state was pristine; PIE stopped clean.

## Recommendation to the orchestrator
1. **Do not treat this as an asset repair.** The widget is correct; leave `WBP_UnitHealthBar` as committed.
2. **Route the two open items to gameplay-programmer** (QA loop, per STEP 2): (a) the **hero `IsDead()`/alive path** producing a hidden hero bar at full HP; (b) a re-examination of whether the "fill never drops" was an artifact of the PRIOR build / imprecise observation (QA's 3-lean) versus a live C++ subtlety — since in THIS freshly-recompiled build all fill-drive prerequisites are provably met.
3. **The fill-visibly-drains + team-tint hard gate stays a human WATCH** owed to Jonathan (headless Slate/read limits above) — do not let it be rubber-stamped.

## Evidence appendix (MCP readbacks)
- `get_parent` → `UnitHealthBarWidget`.
- `read_graph_dsl(EventGraph)` → `OnHPChanged`/`SetTeamColor` bodies as quoted above.
- `list_variables`=[]; `list_graphs`=[EventGraph]; `list_functions`= inherited-only, none implemented.
- `WidgetTree.Bar` props: `Percent=1`, `FillColorAndOpacity={1,1,1,1}`, `Visibility=HitTestInvisible`, `WidgetStyle.fillImage.resourceObject=/Engine/EngineMaterials/DefaultWhiteGrid_Low`, `enableFillAnimation=false`.
- Live `HPBarWidget` (hero & knight): `WidgetClass=/Game/UI/WBP_UnitHealthBar.WBP_UnitHealthBar_C`, `Space=Screen`, `DrawSize={90,12}`, `bHiddenInGame=false`; `bVisible` hero=false / knight=true; hero `bShowHealthBar=true`.
- Live actor HP: Hero 200/200, Knight 200/200, Pikeman 100/100, Cavalry 140/140, Miner 30/30.
- Logs: `[PlayLevel] Compiling WBP_UnitHealthBar before play` (fresh recompile against the new C++); zero `LogBlueprint` warnings/errors; no "Accessed None"; no "HealthBarWidgetClass unresolved".

---

# REOPENED (round 3) — 2026-07-10 — EXECUTION-LEVEL PROOF: the widget is NOT the bug

Prompted by Jonathan's retraction ("the health bar does not seem to drop at all… for characters, hero, and towers, so that was never fixed") and the reopening's prime hypothesis (the BIEs might be **Custom Events**, not true overrides). I stopped proving *structure* and proved *execution*. Result: **the widget's fill pipeline is provably correct end-to-end. There is no widget-asset defect.**

## The coordinator's two required determinations

### 1. Node class of both BIE implementations — GENUINE OVERRIDES (prime hypothesis REFUTED)
Read off the node objects (not the DSL) via `find_nodes` + `get_node_infos` + `get_properties`:
- `OnHPChanged` = object class **`K2Node_Event`**, `type_id = AddEvent|Siegebound|UI|EventOnHPChanged`.
- `SetTeamColor` = object class **`K2Node_Event`**, `type_id = AddEvent|Siegebound|UI|EventSetTeamColor`.
- **Control:** `WBP_CastleHealthBar`'s working `OnHPChanged` = the **same** class `K2Node_Event` and **same** `type_id`.
Neither is a `K2Node_CustomEvent`. Object names are `K2Node_Event_*` (a custom event would be `K2Node_CustomEvent_*`). The "custom event / unbound" hypothesis is dead.

### 2. Did the Print fire? — YES, BOTH (temporary diagnostics, since removed)
Spliced a `PrintString` as the first node inside each event, compiled, PIE'd with real combat:
- `UNITBAR_OHC_FIRED` — logged **every poll, per widget instance** (`LogBlueprintUserMessages [WBP_UnitHealthBar_C_n]`). `OnHPChanged` genuinely executes.
- `UNITBAR_STC_FIRED` — logged **once per widget instance** at init. `SetTeamColor` genuinely executes.
The BP implementations ARE bound to the C++ BIEs and run.

## THE DECISIVE TEST — live `GetPercent` readback (proves SetPercent sticks and the fill drops)
Rewired the OnHPChanged print to output `ToString(ProgressBar::GetPercent(Bar))` at the **start** of each poll — i.e. the ProgressBar's actual stored `Percent`, reflecting the **previous** poll's `SetPercent`. PIE with combat:
- **`WBP_UnitHealthBar_C_0` (= `BP_HeroCharacter_C_0`, confirmed by the BeginPlay diag) printed `0.85 → 0.775 → 0.7` across consecutive polls** as the hero took melee damage (170→155→140 of 200).
- Full-HP units (`_C_1` Cleric 90/90, `_C_2` Knight 200/200) printed **`1.0`**.
**The ProgressBar's `Percent` is set by `SetPercent`, HOLDS the value (no reset, no binding overwrite), and DROPS as HP falls.** This is the ground-truth `Percent` the Slate `SProgressBar` reads when it paints the fill.

## Every fallback hypothesis refuted
1. **Swapped divide operands** — divide is `A/B` with A=`CurrentHP`(pin0), B=`MaxHP`(pin1) = `Current/Max`, byte-identical wiring to the working castle bar. NO.
2. **Percent ignored by the ProgressBar (brush/style)** — `Bar` is `BarFillType=LeftToRight`, `BarFillStyle=Scale`, `bIsMarquee=false`, identical to the castle `Bar`; and `GetPercent` proves the value is honored/stored. NO.
3. **Wrong / second / hidden bar** — the CDO exposes exactly ONE widget property (`bar : ProgressBar`); both events target the same single `GetBar`; no runtime "Accessed None". The bar is visible (so it's not covered). NO.
4. **`if MaxHP > 0` guard mis-wired** — `MaxHP`→`>`→`MakeLiteralFloat 0.0`; with live `Max=200` the branch is taken and `SetPercent` runs (proven by the dropping `GetPercent`). NO.

## Root cause (confirmed): NOT the widget asset
The full chain is proven working: **C++ pushes falling `Current/Max`** (coordinator's `[TASK122DIAG]`) → **`OnHPChanged` fires every poll** (Print) → **`SetPercent(Bar, Current/Max)` runs and the ProgressBar's `Percent` drops and holds** (`GetPercent` 0.85→0.7). The ProgressBar is configured (Scale fill) exactly like the working castle bar. `SetTeamColor` likewise fires and writes the same `Bar`. **The widget computes, stores, and would render a dropping, team-tinted fill.** There is no defect in `WBP_UnitHealthBar` to fix.

The only link not machine-observable headless is `ProgressBar.Percent → on-screen pixels` (Slate; the standing TASK-112 capture WATCH) — and that link works for the castle bar with identical config.

## What I changed
- Applied the temporary diagnostics, PIE-verified, then **removed all of them**: deleted both `PrintString` nodes, the `GetPercent` node, and the orphaned auto-inserted `ToString(Float)` convert node; restored the two original exec links. Verified the graph is byte-for-logic identical to the clean TASK-127 state (11 nodes, DSL matches). Compiled clean, **saved** (`is_dirty=false`).
- **Kept** the TASK-127 grey track `(0.03,0.03,0.03)@0.7` and the C++-driven fill tint (verified still present).
- No `Source/` edits, no Git, `WBP_CastleHealthBar`/`WBP_MainMenu` untouched, `[TASK122DIAG]` C++ logs left in place.

## Recommendation to the orchestrator
Because the widget provably drops its fill `Percent` end-to-end, the reopening's premise (defect inside `WBP_UnitHealthBar`) is refuted at the execution level. Two honest possibilities remain, both OUTSIDE the widget asset / my lane:
1. **The fill DOES drop now** and the report is stale/mistaken (Jonathan already retracted one report; TASK-127 also just fixed the black-on-black contrast). Ask Jonathan to visually re-verify a fresh build.
2. **A presentation-layer defect** in how the **screen-space `UWidgetComponent` (`UHealthBarComponent`, C++) refreshes** the rendered widget — the widget's internal `Percent` is correct but the on-screen texture may not update. That is C++, not the widget asset; per my constraints I STOP and report rather than touch `Source/`. Suggested lead for gameplay-programmer: diff the unit bar's screen-space `UHealthBarComponent` render/redraw settings against `ACastle::HPBarWidget` (the working control).
