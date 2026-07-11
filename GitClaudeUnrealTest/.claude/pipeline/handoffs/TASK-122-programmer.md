# TASK-122 handoff — gameplay-programmer

**Status:** ready-for-qa
**Scope:** C++ only (`HealthBarComponent.cpp/.h`) + a read-only Unreal MCP diagnosis of `WBP_UnitHealthBar`. No editor mutation, no compile, no Git, no .uasset save.

---

## STEP 1 — Read-only diagnosis (recorded, with evidence)

The task named `BarWidget == nullptr` (a missing reparent of `WBP_UnitHealthBar`) as the PRIME SUSPECT. I verified all four points against the live editor via `BlueprintTools` (read-only) and the source tree. **The prime suspect is REFUTED — the widget asset is HEALTHY.**

**(b) Reparent — HEALTHY.** `BlueprintTools.get_parent` on `/Game/UI/WBP_UnitHealthBar` returns:
```
{"refPath":"/Script/GitClaudeUnrealTest.UnitHealthBarWidget"}
```
So `Cast<UUnitHealthBarWidget>(GetWidget())` at `HealthBarComponent.cpp:80` returns NON-NULL. `BarWidget` is not null. The "bar appears but never fills" null-cast theory does not hold.

**(a) Graph wiring — HEALTHY.** `BlueprintTools.read_graph_dsl` on `WBP_UnitHealthBar:EventGraph` returns a real, wired graph (not just FNames in the name table):
```
(event Siegebound|UI|EventOnHPChanged (CurrentHP MaxHP)
  (if (> MaxHP 0.0)
    (Progress|SetPercent (Variables|WBP_UnitHealthBar|GetBar) (/ CurrentHP MaxHP))))

(event Siegebound|UI|EventSetTeamColor (R G B)
  (Progress|SetFillColorAndOpacity (Variables|WBP_UnitHealthBar|GetBar) (Utilities|Struct|MakeLinearColor R G B 1.0)))
```
`OnHPChanged` guards `MaxHP > 0` and drives `ProgressBar SetPercent(CurrentHP / MaxHP)` on the widget-tree ProgressBar `Bar` (the `GetBar` accessor resolved cleanly — no broken node — confirming the ProgressBar exists and is the SetPercent target). `SetTeamColor` drives `SetFillColorAndOpacity`. Both BIEs are correctly implemented. (`list_variables` returns `[]` — expected; those are user member vars, not the widget-tree `Bar` element.)

**(c) Component construction — HEALTHY.** `UHealthBarComponent` named `HPBarWidget` is created in-constructor on all three families:
- `SummonedUnit.cpp:83` — `CreateDefaultSubobject<UHealthBarComponent>(TEXT("HPBarWidget"))`
- `Building.cpp:54` — same
- `HeroCharacter.cpp:52` — same

`ASummonedUnit`/`ABuilding`/`AHeroCharacter` each declare `TObjectPtr<UHealthBarComponent> HPBarWidget` (subclasses AMinerUnit/ATower/ABarracks/ADeepMine inherit it). `ACastle` correctly uses a plain `UWidgetComponent` + its own `FOnCastleHPChanged` push model — untouched.

**(c) Soft class — HEALTHY.** `HealthBarComponent.cpp:48` sets `HealthBarWidgetClass = /Game/UI/WBP_UnitHealthBar.WBP_UnitHealthBar_C` and `BeginPlay` resolves it null-safe. Matches CONVENTIONS names block.

### Root-cause conclusion
There is **no widget-asset defect and no C++ null-cast defect**. The bar was reaching the code path where, at full HP, it was HIDDEN (old hide-at-full gate `Current < Max - epsilon`) and `OnHPChanged` was NEVER pushed until damage; when a unit did take damage it usually did so in a burst just before dying, so the full→damaged transition was never seen on screen — reading exactly as "the bars don't drop," in contrast to the always-visible castle bar (a separate `FOnCastleHPChanged` push-model system). **The reported 1b symptom is a consequence of the 1a hide-at-full behavior, not a separate broken-fill bug.**

---

## 1a / 1b independence verdict

**Not independent in this instance — 1b was a symptom of 1a.** The widget renders correctly; once the bar is visible at full HP and polled every ~0.15 s, `OnHPChanged` drives `SetPercent` continuously and the fill visibly drops. Removing hide-at-full (1a) is therefore expected to resolve the perceived "fill never drops" (1b).

Caveat I cannot close from source+MCP inspection alone: a runtime-only issue (e.g. Slate deferring the Screen-space widget's creation past the one BeginPlay cast, leaving `BarWidget` null at runtime even though the reparent is correct) would still starve the fill. To defend against exactly that failure mode I hardened the C++ (below). **TASK-123/124 PIE verification must confirm the fill actually moves** — that is the only thing this static analysis can't prove.

---

## STEP 2 + 3 — C++ changes

**Files:** `Source/GitClaudeUnrealTest/Siegebound/HealthBarComponent.cpp`, `HealthBarComponent.h` — nothing else.

1. **1a — always-visible show-gate.** `PollHealth` show-gate changed from
   `bShowHealthBar && bAlive && (Current < Max - HealthBarFullEpsilon)` → `bShowHealthBar && bAlive`.
   The bar now shows the entire time the owner is alive + opted-in, full HP included; hides only on `!IsHealthBarActorAlive()` / destruction or when opted out.
2. **Retired the epsilon.** Removed the now-unused `HealthBarFullEpsilon` anonymous-namespace constant (and its comment) — CONVENTIONS explicitly retires it from the show/hide decision; leaving it would be dead code.
3. **`bShowHealthBar` opt-out — KEPT** (default `true`, `EditDefaultsOnly`), per FLAG 3 / manager ruling. It gates whether a type carries a bar at all (miner clutter), orthogonal to when a bar shows. Not removed.
4. **1b — fill driven every poll on a NON-NULL widget.** After 1a, "shown" == "alive", so `OnHPChanged(Current, Max)` is now pushed on every poll starting from the first (full HP → `SetPercent 1.0`), then continuously as HP drops. Added a **self-healing `BarWidget` re-resolve**: if `BarWidget` is ever null at poll time it re-runs `Cast<UUnitHealthBarWidget>(GetWidget())` before the push, so a deferred Screen-space widget creation can't permanently starve the fill (the runtime-only failure mode the static diagnosis can't rule out). Normal path: `BarWidget` is set at BeginPlay, the `if(!BarWidget)` is false forever → zero overhead.
5. **Read Current/Max at the push site only** (they no longer feed the show decision).
6. Updated the two header doc comments and the constructor `SetVisibility(false)` comment to the always-visible law.

**Preserved (unchanged):** null-safety (missing widget class ⇒ silent no-bar, logged once, timer never armed — `BeginPlay` early-return); one-time team tint via `ITeamAgent::GetTeamId` (`BlueBarColor`/`RedBarColor` data, never hardcoded); hide-on-death via the `IsHealthBarActorAlive()` poll; `EndPlay` `ClearTimer`; `MinPollInterval` timer floor; DrawSize / Screen space / NoCollision. **Zero combat/stat/damage change** — the component only reads HP getters.

## What I explicitly did NOT change
- `ACastle` / `FOnCastleHPChanged` / `UCastleHealthBarWidget` / its `HPBarWidget` — the castle push-model bar works and is a separate system.
- `WBP_UnitHealthBar` and any other `.uasset` — no editor mutation, no save, no "Refresh Nodes". Read-only inspection only.
- The 4 pre-existing working-tree modifications (`TASKBOARD.md`, `handoffs/TASK-120.md`, `WBP_CastleHealthBar.uasset`, `WBP_MainMenu.uasset`) and `CONVENTIONS.md` (manager's reversal) — untouched.
- No compile, no Git.

## QA scan notes (mandatory for this task)
- **Inherited-reflected-member shadow scan:** clean. New/kept locals (`Target`, `bAlive`, `bShouldShow`, `LoadedWidgetClass`, `TeamAgent`, `BarColor`, `World`) shadow no inherited reflected UPROPERTY (no `Owner`/`Instigator`/`Controller`/`PlayerState`/`Slot` collisions).
- **Complete-type-include scan:** clean. Every dereference/`Cast<>` targets a type whose full header is already included — `UUnitHealthBarWidget` (`Siegebound/UnitHealthBarWidget.h`, l.12), `UUserWidget` from `GetWidget()` (`Blueprint/UserWidget.h`, l.5), `IHealthBarTarget` (`Siegebound/HealthBarTarget.h`, l.10), `ITeamAgent`/`ETeamId` (`Siegebound/TeamId.h`, l.11). No new type usage introduced (the added re-resolve reuses the exact `Cast<UUnitHealthBarWidget>(GetWidget())` already at l.80).

## For TASK-123 (art-director, WBP side)
Diagnosis says `WBP_UnitHealthBar` is already correctly reparented and wired — **no repair is strictly required**. TASK-123's job narrows to VERIFICATION under the always-visible law once TASK-124 phase-A has compiled this change:
- CONFIRM in PIE the bar is visible at full HP and the fill drops live as HP falls (the one thing static analysis can't prove — closes the 1b caveat).
- CONFIRM the ProgressBar `Bar` fill visually reflects `SetPercent` (i.e. the fill image is a horizontal-fill bar, not a static/full-only decal) and the team tint (`SetFillColorAndOpacity`) reads.
- If — and only if — PIE shows the fill still not moving with this C++ in, the runtime-only deferred-widget theory would be live; but the self-healing re-resolve added here should cover it. Do NOT touch `WBP_CastleHealthBar`.

---

# QA LOOP 1 (2026-07-10) — round-1 was WRONG; runtime diagnosis + real fix

**What round 1 got wrong.** I concluded the WBP was healthy and "1b was a symptom of 1a." Jonathan PIE'd the compiled build: bars are now always-visible (1a shipped) but **still do not drop, and are not team-tinted on either team** (1b NOT fixed). My "symptom of 1a" verdict is refuted.

**The engine fact both prior verdicts missed.** `UWidgetComponent::SetWidgetClass` (engine `WidgetComponent.cpp:2361`) sets `WidgetClass` **unconditionally** but only runs `CreateWidget`+`SetWidget` when `HasBegunPlay()` is true at that moment. So reading `WidgetClass == WBP_UnitHealthBar_C` on the live component (which is all art-director/I could read) does **NOT** prove the widget exists — a set class with a null `GetWidget()` is possible, and yields EXACTLY visible+frozen+untinted: the component's own `SetVisibility` runs regardless, while `OnHPChanged` and `SetTeamColor` both sit behind `if (BarWidget)` and guard-skip on a null cast.

**Runtime measurements I actually took (fresh PIE via MCP; all recorded in qa/TASK-122-report.md "QA LOOP 1"):**
- `get_parent` = `UnitHealthBarWidget` (reparent OK); `read_graph_dsl` shows `OnHPChanged→SetPercent` + `SetTeamColor→SetFillColorAndOpacity` (graph OK).
- Live hero `HPBarWidget`: `WidgetClass` set, `bShowHealthBar`=true, `Space`=Screen, `bVisible`=true; hero Team=Blue, 200/200.
- Authored `Bar`: Percent=1, FillColorAndOpacity + fillImage.tintColor = white (nothing bakes red / freezes fill).
- Output log: **zero "Accessed None"** (graph not hitting a null `Bar`).
- **Blocked probes (tooling limits):** `get_properties` cannot serialize the transient `Widget`/`BarWidget` pointers, and `CurrentHP` is not settable — so a direct null/non-null read of `BarWidget` and a live damage probe were impossible. This is WHY a runtime log is required (Attempt B).

**1a/1b independence verdict (corrected):** they are **independent**. 1a (always-visible) shipped and works. 1b (fill never drops + no tint) is a SEPARATE defect whose leading, measurement-consistent cause is a **runtime-null `BarWidget`** — `SetWidgetClass`'s construction side-effect not firing/persisting — i.e. **C++-side, not a WBP defect.** Not yet 100% proven (unreadable transient pointer), so I added a one-pass log to confirm.

**Fix + instrumentation this loop (HealthBarComponent.cpp/.h only):**
1. **Explicit widget construction** (`BeginPlay`): after `SetWidgetClass`, if `GetWidget()` is null, `SetWidget(CreateWidget<UUnitHealthBarWidget>(World, LoadedWidgetClass))`. Removes all dependence on the conditional side-effect. Null-safe (null World skips). This is the actual candidate FIX.
2. **`ApplyTeamTint()` helper** (round-1 WARN-1, pre-authorized): tint factored out, called from BeginPlay AND the poll re-resolve, idempotent via `bTeamTintApplied` — a late-resolved widget is never left untinted.
3. **`[TASK122DIAG]` throttled logs** (Attempt B): BeginPlay logs `side-effect-created=YES/NO`, actual GetWidget class, `BarWidget-cast=VALID/NULL`; poll logs `alive/Current/Max/BarWidget/PUSHED|SKIPPED` (first 3 polls + every 40th). **TEMPORARY — remove once confirmed.** They record the state BEFORE the fallback acts, so the log still reveals the true cause even though the fallback also fixes it.

**Does this need a compile before retest? YES.** Per the coordinator's Attempt B: build-master (TASK-124 phase-A) compiles, then ONE PIE pass, then read the `[TASK122DIAG]` log:
- `side-effect-created=NO` → widget was never constructed → the explicit fallback now creates it → bars drop → **C++-side, closed** (follow-up trims the diag logs).
- `side-effect-created=YES` + `BarWidget-cast=VALID` + poll shows Current dropping but bar frozen → **WBP-render defect → TASK-123 (art-director)**.

**Not changed / not touched:** `ACastle`/`FOnCastleHPChanged`/`WBP_CastleHealthBar`; no `.uasset` saved (read-only MCP inspection + a diagnostic PIE that I stopped); `bShowHealthBar` kept; no combat/stat/HP math touched; the 4 pre-existing working-tree mods + CONVENTIONS.md left alone. Shadow-scan + complete-type-include scan re-run CLEAN on the loop-1 change.

---

# QA LOOP 2 (2026-07-10) — loop-1 theory ALSO refuted; the fault is the last hop (Percent → pixels)

**Loop-1 was wrong too.** art-director proved (runtime, not graph readback) that `OnHPChanged`/`SetTeamColor` are genuine event overrides that EXECUTE, `SetPercent` runs, and `Bar.Percent` (the UPROPERTY) drops 0.85→0.775→0.7 under damage and holds — and my own `[TASK122DIAG]` shows `BarWidget=VALID -> OnHPChanged PUSHED` with falling values. So `BarWidget` is NOT null (loop-1 refuted) and the C++→BIE→`Bar.Percent(UPROPERTY)` chain is proven end to end. **The only unproven link is `Percent` UPROPERTY → rendered pixels** — the screen-space `UWidgetComponent` presentation layer. That is Source/, i.e. mine. The WBP is exonerated (do not send back to art).

## Castle diff (measured — the coordinator's InitWidget-order hypothesis is REFUTED by the control)
- **InitWidget order is IDENTICAL.** `ACastle::BeginPlay` calls `Super::BeginPlay()` (runs `InitWidget()` with `WidgetClass` still null) FIRST, THEN `InitHPBarWidget()`→`SetWidgetClass()` — exactly our order (`Castle.cpp:60-101`). So the widget-class-before-`Super` idea is not the differentiator; the known-good castle does the same thing.
- **The ONE structural difference: the castle never hides its component.** `ACastle::HPBarWidget` is VISIBLE from construction and renders; ours was the only screen-space bar that called `SetVisibility(false)` in the constructor and then toggled visible from the poll.
- **Engine mechanism (source):** a screen-space `UWidgetComponent` is painted by being added to `FWorldWidgetScreenLayer` in `UWidgetComponent::UpdateWidgetOnScreen()` (`WidgetComponent.cpp:1308-1331`), called at the END of `TickComponent`, gated on the component `IsVisible()`. For **Screen** space `UpdateWidget()` is a near-no-op (its body is entirely under `if (Space==World)`), and nothing syncs the component's `SetVisibility` to the widget's own Slate visibility.
- **CDO read (read-only MCP, `Default__HealthBarComponent`):** `Space=Screen`, `DrawSize=90x12`, `TickMode=Enabled`, `bVisible=false`, `bWindowFocusable=true`. **`TickMode=Enabled` is important:** the tick self-disable-when-invisible path (`WidgetComponent.cpp:1264`, gated on `TickMode != Enabled`) NEVER fires here, so the component ticks continuously and `UpdateWidgetOnScreen` re-evaluates every tick — which *weakens* the visibility-toggle theory (a continuously-ticking component should re-add on `SetVisibility(true)`). `bVisible=false` confirms the old hidden default.

## The five measurements (instrumented into `LogDiag`; I could NOT run them — needs a compile + PIE)
Editor tooling cannot serialize the transient `Widget`/`BarWidget`/Slate pointers, so I added `UHealthBarComponent::LogDiag()` (BeginPlay + throttled poll) capturing, per actor:
1. `BarWidget->GetCachedWidget().IsValid()` — is the UserWidget's Slate built at all? (`widgetSlate`)
2. the `Bar` ProgressBar (via `BarWidget->GetWidgetFromName("Bar")`): `Cast<UProgressBar>` non-null (`Bar=`), its `GetCachedWidget().IsValid()` (`BarSlate` — **if 0, `SetPercent` writes the UPROPERTY and paints nothing; the exact gap, since `GetPercent()` reads the UPROPERTY not Slate**), and live `GetPercent()` (`BarPercent`).
3. `GetWidget()` object name + `GetWidget()==BarWidget` (`identity` — is the on-screen instance the same one we push `SetPercent` to?).
4. `IsInViewport()`, component `IsVisible()`, `GetOwner()->IsHidden()`, `IsComponentTickEnabled()` — presentation/registration state.
5. `Space`, `DrawSize` (dumped live for ours; castle equivalents read from CDO/source: Screen, 256x32).

## Root cause — HONESTLY UNDETERMINED from static analysis
I will not assert a fourth confident cause. The chain is proven up to `Bar.Percent(UPROPERTY)`; the `Percent → SMyProgressBar Slate → pixels` hop cannot be observed without the runtime log. **Leading candidates the log will discriminate:** (a) `BarSlate=0` — the `Bar` ProgressBar's Slate isn't live when `SetPercent` runs (item 2); (b) `identity=0` — the painted `GetWidget()` is a different instance than the `BarWidget` we push (item 3). The visibility-toggle diff is a WEAK candidate given `TickMode=Enabled`, but it is the only structural difference from the working castle.

## Change this loop (`HealthBarComponent.cpp/.h`)
1. **Instrumentation (primary):** `LogDiag()` capturing the five measurements above, at BeginPlay and throttled in the poll. Marked `[TASK122DIAG]`, TEMPORARY (TASK-128 strips them + the orphaned `DiagPollCount`).
2. **Candidate fix (labeled, low-risk, castle-aligned — NOT asserted as the cause):** removed the constructor `SetVisibility(false)`; the component is now visible from construction like the castle, and BeginPlay sets the initial shown state from `bShowHealthBar` (`bBarShown = bShowHealthBar; SetVisibility(bShowHealthBar)`), so an alive opted-in bar is continuously visible (added to the screen layer on its first tick) and never does the hide→show toggle the castle avoids. Harmless if it is not the cause, and it does NOT mask the LogDiag (if the fault is item 2/3, the log still shows `BarSlate=0` / `identity=0`).

**Kept:** always-visible law, `bShowHealthBar`, `ApplyTeamTint()`, null-safety, `EndPlay` timer cleanup, hide-on-death. **Not touched:** `ACastle`/`FOnCastleHPChanged`/`UCastleHealthBarWidget`; no `.uasset`. Shadow-scan + complete-type-include scan CLEAN (added `Components/ProgressBar.h` for `UProgressBar`, `GameFramework/Actor.h` for `GetOwner()->IsHidden()`).

## Compile needed? YES.
build-master (TASK-124 phase-A) compiles, runs ONE PIE pass under combat, and reads the `[TASK122DIAG] LogDiag` lines:
- `BarSlate=1` + `identity=1` + the candidate fix made it paint (human WATCH: fill drains) → **closed** (then TASK-128 strips diagnostics).
- `BarSlate=0` → the `Bar` Slate isn't live when pushed → the fix is to force/verify the Slate is constructed before/at the push (loop 3, C++-side).
- `identity=0` → two widget instances → reconcile which instance is on screen (loop 3, C++-side).
- The final "fill visibly drains, blue/red, grey track, legible" is a **human WATCH for Jonathan** (screen-space Slate is uncapturable — per OVERNIGHT-AUTH §3); record it as OPEN, do not fake it.

**What I could NOT determine:** whether the `Bar` ProgressBar's Slate is live, and whether the on-screen widget is the same instance I push — both require the runtime LogDiag I cannot execute. If loop 3 does not close it, escalate to 🚨 Blockers per OVERNIGHT-AUTH §5.
