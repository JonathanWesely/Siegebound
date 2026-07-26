# TASK-274 handoff — Command state + T/R/E input + HOLD reticle pick

**Status:** ready-for-qa
**Branch:** m7.6-arena10x
**Assignee:** gameplay-programmer
**Law:** CONVENTIONS "Unit commands (Shield Wall stances) — ATTACK / HOLD / DEFEND (W1, 2026-07-23)"

This is the PLAYER-CONTROLLER (command-issuing) half of the Shield Wall feature. TASK-275
(ASummonedUnit / ACastle) consumes the public API below. NO compile, NO Git (build-master, TASK-277).

## Files created / edited
- **NEW** `Source/GitClaudeUnrealTest/Siegebound/UnitCommand.h` — header-only `UENUM(BlueprintType) enum class ESiegeUnitCommand : uint8 { Attack, Hold, Defend };` (the `TeamId.h` pure-data-type exception; includes `UnitCommand.generated.h`). Nothing else in it, so both the controller and the unit include it cheaply.
- **EDIT** `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`
- **EDIT** `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`

## EXACT public API TASK-275 consumes (nail these — do not rename)
All on `ASiegePlayerController`, all `BlueprintPure`/`const` except the setter:
```cpp
ESiegeUnitCommand GetCurrentCommand() const;   // the latched stance
bool              HasIssuedCommand() const;     // false until first key — TASK-275's legacy-body gate
FVector           GetHoldLocation() const;      // confirmed HOLD ground point
float             GetHoldRadius()   const;      // default 1500 uu (EditDefaultsOnly, ClampMin 0)
void              SetUnitCommand(ESiegeUnitCommand NewCommand); // BlueprintCallable: latch + bHasIssuedCommand=true + broadcast
```
Delegate (declared at file scope in the header, above the UCLASS):
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitCommandChanged, ESiegeUnitCommand, NewCommand);
UPROPERTY(BlueprintAssignable, Category="Siegebound|Commands") FOnUnitCommandChanged OnUnitCommandChanged; // HUD (TASK-276) binds
```
Authoritative state (private): `ESiegeUnitCommand CurrentCommand = Attack;`, `bool bHasIssuedCommand = false;`, `FVector HoldLocation = ZeroVector;`. Tunable (protected, `Siegebound|Commands`): `float HoldRadius = 1500.f;`.

**TASK-275 pattern:** each `ASummonedUnit::UpdateState` (Standard body only) reads `GetPlayerController(0)` → cast `ASiegePlayerController` → if `Team == Blue (local player) && PC->HasIssuedCommand()`, branch on `PC->GetCurrentCommand()`; else legacy body. Units read LIVE state each tick, so a newly-spawned unit adopts the current stance automatically — no need to bind `OnUnitCommandChanged` (that's the HUD's job).

**DefendRadius is NOT here.** Per CONVENTIONS it is a NEW `ASummonedUnit` member (`float DefendRadius = 2500`), owned by TASK-275. I deliberately did not add it to the controller. `SpawnBoxHalfExtent` already exists on the controller (TASK-261, default (840,840)) — TASK-275 adds the matching `ACastle::SpawnBoxHalfExtent` + `IsPointInSpawnBox` (3-way paired-tunable).

## Input (soft-resolved, null-safe — TASK-273 assets)
Mirrors the `IA_Card*` pattern EXACTLY. Constructor sets the soft paths; `SetupInputComponent` resolves via the existing `ResolveInputAction` and binds `ETriggerEvent::Started`. A missing asset skips only its own binding, logs once, never crashes.

| Handler | Action asset (soft path) | Key (TASK-273 maps in IMC_Hero) | Behavior |
|---|---|---|---|
| `OnCmdAttackPressed` | `/Game/Input/Actions/IA_CmdAttack` | **T** | immediate `SetUnitCommand(Attack)` |
| `OnCmdHoldPressed`   | `/Game/Input/Actions/IA_CmdHold`   | **R** | `BeginHoldTarget()` (ground pick) |
| `OnCmdDefendPressed` | `/Game/Input/Actions/IA_CmdDefend` | **E** | immediate `SetUnitCommand(Defend)` |

Slots: `TObjectPtr<UInputAction> CmdAttackAction/CmdHoldAction/CmdDefendAction`; soft fields: `TSoftObjectPtr<UInputAction> CmdAttackActionAsset/CmdHoldActionAsset/CmdDefendActionAsset`.

**TASK-273 asset names it MUST match (character-for-character):** `IA_CmdAttack`, `IA_CmdHold`, `IA_CmdDefend` at `/Game/Input/Actions/`, Digital(bool); mapped in `/Game/Input/IMC_Hero` to T/R/E.

**ATTACK/DEFEND** are immediate stance latches (they do not use the cursor, so they fire even during placement/spell targeting — orthogonal). They are ignored after match end, and if a HOLD pick is mid-flight they `CancelHoldTarget()` first (abandon the pick, no stance change) then latch.

## HOLD-pick flow + how it guards the placement/spell modes
`BeginHoldTarget()` → per-frame `UpdateHoldReticle()` (in `PlayerTick`) → LMB `ConfirmHoldTarget()` / RMB-Esc `CancelHoldTarget()`.
- **Reuses the spell reticle machinery** (no new reticle system): `SpawnSpellReticle()`/`DestroySpellReticle()` manage the shared `SpellReticleActor`; the ground trace is the shared `TraceCursorToGround`. The decal ring is resized to `HoldRadius` in `BeginHoldTarget` (SpawnSpellReticle sizes to a spell's AoERadius; a HOLD pick has no row) so the player sees the actual hold disc. Null-safe: missing `M_SpellReticle` ⇒ no visual, the pick still works off the trace.
- **Disjoint scratch state** (`bInHoldTargetMode`, `bHoldSurfaceValid`, `HoldPickLocation`, `HoldHero`) — kept separate from the placement (`Pending*`) and targeting (`Targeting*`) scratch, following the codebase's no-cross-contamination discipline. `HoldLocation` (the committed point) is written only on confirm.
- **Confirm:** a trace-miss (cursor on the sky, `!bHoldSurfaceValid`) refuses free and STAYS in mode (a different point can succeed — the placement/targeting trace-miss precedent). A surface hit → `HoldLocation = HoldPickLocation`, exit the mode, then `SetUnitCommand(Hold)` + broadcast.
- **Cancel** (RMB/Esc via the polled `PlayerTick` branch AND the `IA_CancelPlace` binding — double-cover): exits with NO stance change.
- **Hero melee suppression** while the pick owns the LMB (via `HoldHero`, mirroring `PlacementHero`/`TargetingHero`) so the confirm click doesn't also swing; released BEFORE any early-out on every exit path (`CancelHoldTarget`).
- **Mutual exclusion (extend, don't fork):** `EnterPlacementMode` and `EnterTargetingMode` each gained a `bInHoldTargetMode` silent-ignore check; `BeginHoldTarget` ignores while `bInPlacementMode` OR `bInTargetingMode` is live. This is the codebase's existing mutual-IGNORE pattern extended to a third mode (the CONVENTIONS phrase "entering one cancels the others" is realized as the established mutual-ignore, which preserves the in-progress mode rather than abruptly destroying its ghost/reticle). `ApplyCursorInputState` now counts `bInHoldTargetMode` as a cursor owner.
- **Teardown/reset coverage:** `EndPlay`, `OnUnPossess`, `HandleHeroDied`, `HandleMatchEnd`, `HandleMatchReset` all now defensively `CancelHoldTarget()` (the shared reticle would otherwise leak, since `ExitTargetingMode` early-outs when `bInTargetingMode` is false).

## Play Again reset (step 5)
`HandleMatchReset` now: defensive `CancelHoldTarget()` (top, alongside the existing ExitPlacement/ExitTargeting), then near the end `CurrentCommand=Attack; bHasIssuedCommand=false; HoldLocation=ZeroVector; OnUnitCommandChanged.Broadcast(Attack)`. The HUD (TASK-276) re-checks `HasIssuedCommand()` (now false) on that broadcast and shows nothing.

## E/R/T conflict check (code side)
Grepped `Source` for `EKeys::E/R/T` — **zero** matches. The only letter-key gameplay binding is Rally = Q (bound via `IA_Rally`, `HeroCharacter.cpp:182`). No code conflict. IMC_Hero is binary — TASK-273 (art-director) confirms in-editor that T/R/E are unbound before mapping.

## What QA should scrutinize
- **Shadow scan:** new locals are `Hero`, `Hit`, `bSurfaceHit`, `ConfirmedHoldLocation`, `ReticleDecal`, param `NewCommand` — none shadow an inherited reflected member (Owner/PlayerState/Instigator/Controller/Slot). (`Hero`/`Hit` are already existing local names in this class.)
- **Complete-type includes:** `UDecalComponent` (`Components/DecalComponent.h`), `ADecalActor` (`Engine/DecalActor.h`), `AHeroCharacter` (`Siegebound/HeroCharacter.h`) all already `#include`d in the .cpp before my dereferences. `ESiegeUnitCommand` via `UnitCommand.h` in the header. No new include needed; verify anyway.
- **Null-safety:** every new lookup guarded (`GetPawn` cast, `SpellReticleActor`, `GetDecal()`, `IsValid(HoldHero)`); missing IA_Cmd assets and missing `M_SpellReticle` both degrade gracefully.
- **Mode exclusivity + melee-suppression release** on every hold exit path (confirm/cancel/teardown/reset).
- **No existing binding disturbed:** cards 1-6, IA_UICursor (Left Alt), IA_CancelPlace (RMB/Esc), LMB attack, and all M5/placement/targeting flows are byte-for-byte additive.

## Out of scope (untouched)
`ASummonedUnit`, `ACastle`, the bot, capture-zone code, `IMC_Hero`/`IA_*` assets, the HUD widget. No compile, no Git.

---

## DELTA — 2026-07-24 compile fix (comment-termination trap)

Build-master's TASK-277 editor-target build failed to compile this header (details appended to `qa/TASK-274.md`). Root cause was a doc-comment that closed itself early. **This is a COMMENT-ONLY fix — zero executable code changed, so the prior QA PASS stands.**

- **`SiegePlayerController.h:1057`** — the `/** … */` doc comment for `bInHoldTargetMode` contained the literal `each Enter*/Begin*`; the embedded `*/` terminated the block comment early, so `bool bInHoldTargetMode = false;` (h:1058) parsed as stray C++ → C2143/C4430/C2059/C4138/C2238 there and `'bInHoldTargetMode': undeclared identifier` (C2065) at 12 sites in the .cpp.
  - **Fix:** reworded to `(each Enter or Begin entry point ignores while any other is live)`. No `*/` or `/*` remains; meaning preserved (the three cursor-pick modes are mutually exclusive).
- **`SiegePlayerController.h:1002`** — grep for the same pattern (across `SiegePlayerController.{h,cpp}`, `SummonedUnit.{h,cpp}`, `Castle.{h,cpp}`, `UnitCommand.h`) turned up one more embedded `*/`: `Pending*/placement` inside a **`//` line comment**. This is inert (a `*/` in a `//` line comment cannot close a block comment and did NOT contribute to the build failure), but it is the same syntactic hazard, so it was hardened to `Pending* placement`. Zero compile/behavior impact.
- **All other grep hits were benign** balanced inline block comments (`/*bLoop=*/ true`, `/*Owner=*/ this`, `/*bPropagateToChildren=*/true`, `/*ZOrder=*/ 10`, etc.) and `SiegePlayerController.h:1005`'s `each Enter*` (followed by a space, not `*/`). None touched.

**QA to scrutinize:** confirm the two edits are comment-only (they are — no token outside a comment moved). Files touched: `SiegePlayerController.h` only. No `.cpp`, no logic, no compile, no Git.
