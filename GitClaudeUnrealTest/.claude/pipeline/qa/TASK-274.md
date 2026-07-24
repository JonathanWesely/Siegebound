# QA Report — TASK-274
Verdict: PASS

Scope reviewed: `Source/GitClaudeUnrealTest/Siegebound/UnitCommand.h` (new),
`SiegePlayerController.h`, `SiegePlayerController.cpp` (edits). Pre-compile review against the
board spec (TASKBOARD "#### TASK-274"), CONVENTIONS "Unit commands (Shield Wall stances)…",
and the programmer handoff `handoffs/TASK-274.md`. No compile, no engine, no Git.

## Findings
No BLOCKERs. No WARNs. Three NITs (informational — no action required for this task).

- [NIT] SiegePlayerController.cpp:767-803 — `OnCmdAttackPressed`/`OnCmdDefendPressed` latch the
  stance even while placement or spell-targeting mode is live (they only cancel an in-flight HOLD
  pick, not the cursor modes). This is intentional and documented (handoff §"ATTACK/DEFEND … fire
  even during placement/spell targeting — orthogonal") and state-safe: `SetUnitCommand` touches only
  `CurrentCommand`/`bHasIssuedCommand` + broadcast, which is disjoint from all placement/targeting
  scratch. CONVENTIONS' "entering one cancels the others" applies to the three CURSOR pick modes,
  which is fully honored. Flagging only so a future playtester knows T/E during a card placement
  silently re-latches the stance (visible via the TASK-276 HUD indicator only).
- [NIT] SiegePlayerController.cpp:756-759 — `SetUnitCommand` always broadcasts, including a
  same-stance re-issue. Intentional per spec ("Idempotent — re-issuing the same stance re-affirms
  the HUD"). No dedupe needed.
- [NIT] SiegePlayerController.h:1049/1052/1055 — `CurrentCommand`/`bHasIssuedCommand`/`HoldLocation`
  are plain (non-UPROPERTY) members, which satisfies the spec's "transient" (not serialized) and is
  GC-safe (no UObject pointers — enum/bool/FVector). BP access is provided through the BlueprintPure
  getters, so no reflection is required on the fields. Correct as written.

## Critical-scrutiny results (the three areas flagged in the task)

### 1. Three-way mode mutual exclusion — AIRTIGHT (verified both directions, all three pairs)
- `EnterPlacementMode` (cpp:849-875) silently ignores while `bInPlacementMode` OR `bInTargetingMode`
  OR `bInHoldTargetMode`.
- `EnterTargetingMode` (cpp:1544-1568) silently ignores while `bInPlacementMode` OR `bInTargetingMode`
  OR `bInHoldTargetMode`.
- `BeginHoldTarget` (cpp:2006-2028) silently ignores while `bInPlacementMode` OR `bInTargetingMode`
  OR `bInHoldTargetMode` (plus `bMatchEnded`).
- All three cross-pairs are covered in both directions, so the shared `SpellReticleActor` can never
  be double-owned. Defense-in-depth also present: `SpawnSpellReticle` early-outs when
  `SpellReticleActor` is already non-null (cpp:1920).

### 2. Melee suppression released on EVERY hold-exit path — VERIFIED (7/7)
The release lives in `CancelHoldTarget` (cpp:2147-2151) and fires BEFORE the `!bInHoldTargetMode`
early-out (cpp:2153), exactly mirroring the ExitPlacementMode/ExitTargetingMode law. Every exit path
funnels through it:
- ConfirmHoldTarget (cpp:2126 → CancelHoldTarget)
- CancelHoldTarget itself (RMB/Esc, binding via OnCancelPlacePressed cpp:743-746, polled via
  PlayerTick cpp:310-314)
- EndPlay (cpp:203)
- OnUnPossess (cpp:411)
- HandleHeroDied (cpp:836)
- HandleMatchEnd (cpp:1018)
- HandleMatchReset (cpp:1118)
`HoldHero` is a SEPARATE `UPROPERTY(Transient) TObjectPtr` record from `PlacementHero`/`TargetingHero`
(cpp/h:1066-1068), and the three modes are mutually exclusive, so at most one hero-record is ever
non-null — a defensive ExitPlacement/ExitTargeting call can never strand or wrongly clear the hold
suppression. `IsValid(HoldHero)` guards the deref; the boolean-flag write is idempotent. No leak
found on any path.

### 3. Shared reticle + disjoint hold scratch — VERIFIED
- `SpellReticleActor` is created only by `SpawnSpellReticle` (guarded against re-spawn) and destroyed
  by `DestroySpellReticle`, which nulls the pointer (cpp:1985-1992). `CancelHoldTarget` calls
  `DestroySpellReticle` (cpp:2162); mutual exclusion guarantees no placement→hold or targeting→hold
  overlap, so no double-destroy and no leak across transitions.
- Hold scratch (`bInHoldTargetMode`/`bHoldSurfaceValid`/`HoldPickLocation`/`HoldHero`) is disjoint
  from the placement `Pending*` and targeting `Targeting*` members (h:1058-1068) and is fully reset on
  every exit (CancelHoldTarget cpp:2158-2160). `HoldLocation` (committed point) is written only in
  ConfirmHoldTarget (cpp:2130) after a valid surface hit, and the candidate is captured into a local
  BEFORE the CancelHoldTarget teardown zeroes `HoldPickLocation` (cpp:2122) — ordering is correct.

### 4. Play Again reset — VERIFIED
`HandleMatchReset` (cpp:1118, 1154-1157) calls `CancelHoldTarget()` up top, then sets
`CurrentCommand=Attack`, `bHasIssuedCommand=false`, `HoldLocation=ZeroVector`, and
`OnUnitCommandChanged.Broadcast(Attack)` — matching how the reset clears the other mode state and
satisfying spec step 5.

## Acceptance / regression checklist
- `UnitCommand.h`: header-only `UENUM(BlueprintType) enum class ESiegeUnitCommand : uint8
  { Attack, Hold, Defend }` with only `#include "CoreMinimal.h"` + `UnitCommand.generated.h` —
  matches the TeamId.h pure-data pattern. PASS.
- Public API signatures/const-correctness (TASK-275 consumers): `GetCurrentCommand() const`,
  `HasIssuedCommand() const`, `GetHoldLocation() const`, `GetHoldRadius() const` all BlueprintPure/const;
  `SetUnitCommand(ESiegeUnitCommand NewCommand)` BlueprintCallable latches + sets
  `bHasIssuedCommand=true` + broadcasts. `FOnUnitCommandChanged` BlueprintAssignable declared at file
  scope above the UCLASS. Exact identifier names match the board "names:" block character-for-character.
  PASS.
- `HoldRadius`: UPROPERTY EditDefaultsOnly, default 1500, ClampMin 0 (h:626-627). PASS.
- `DefendRadius`: correctly NOT on the controller — deferred to TASK-275's ASummonedUnit per
  CONVENTIONS. PASS.
- Input: three soft actions resolved by exact path (`/Game/Input/Actions/IA_CmdAttack|Hold|Defend`,
  cpp:80-82), resolved via the existing `ResolveInputAction` and bound `ETriggerEvent::Started`
  (cpp:231-233, 277-288) mirroring the IA_Card1..6 pattern. Each binding is individually guarded, so a
  missing asset skips only its own binding, logs once (via ResolveInputAction), never crashes.
  `SetupInputComponent` only ADDS bindings — cards 1-6, IA_UICursor, IA_CancelPlace all byte-untouched.
  PASS.
- ATTACK (T)/DEFEND (E) immediate `SetUnitCommand`; HOLD (R) → `BeginHoldTarget()`. PASS.
- `PlayerTick` hold branch (cpp:307-327) is fully gated by `bInHoldTargetMode`; when idle PlayerTick
  returns before any hold/targeting/placement work — no per-tick cost. PASS.
- `ApplyCursorInputState` counts `bInHoldTargetMode` as a cursor owner (cpp:2860). PASS.
- Shadow scan: new locals `Hero`, `Hit`, `bSurfaceHit`, `ConfirmedHoldLocation`, `ReticleDecal` and
  param `NewCommand` — none shadow an inherited reflected member (no `Hero`/`Hit`/etc. on
  AController/APlayerController; `Owner`/`Instigator`/`Controller`/`PlayerState` are untouched).
  `Hero`/`Hit` are pre-existing local names in this class. PASS.
- Complete-type includes: `DecalComponent.h` (cpp:8), `DecalActor.h` (cpp:13), `HeroCharacter.h`
  (cpp:31), `EnhancedInputComponent.h` (cpp:10), `InputAction.h` (cpp:19) all present before the
  dereferences; `UnitCommand.h` included in the header (h:10). PASS.
- Null-safety: `GetPawn` cast guarded, `SpellReticleActor`/`GetDecal()` guarded, `IsValid(HoldHero)`
  before deref, missing `M_SpellReticle` degrades to no-visual pick. PASS.
- UE 5.8 API: `GetHitResultUnderCursor(ECC_Visibility,…)`, `WasInputKeyJustPressed`, `BindAction`
  (payload overload), `SetActorHiddenInGame/Location/Rotation`, `MarkRenderStateDirty` — all current,
  no deprecated/removed APIs. PASS.
- M6/spell/placement flows and TASK-261 spawn-box logic untouched (additive edits only). PASS.

## Notes for build-master (if PASS)
- Compiles clean expected. This is the command-ISSUING half only; the consuming API
  (`GetCurrentCommand`/`HasIssuedCommand`/`GetHoldLocation`/`GetHoldRadius`) is stable for TASK-275,
  and `OnUnitCommandChanged`/`GetCurrentCommand` are stable for the TASK-276 HUD.
- Runtime behavior is byte-identical to today until the player first presses T/R/E (bHasIssuedCommand
  gate), so no regression risk to the current W1 build from merging this half alone.
- The IA_CmdAttack/Hold/Defend + IMC_Hero T/R/E mappings are TASK-273's Content assets; the C++ is
  null-safe if they are absent, but full function requires TASK-273 present at run/integration.

---

## BUILD-MASTER COMPILE FAILURE — 2026-07-24 (TASK-277 editor-target build)

Verdict flips to **qa-failed**. `Build.bat GitClaudeUnrealTestEditor Win64 Development -waitmutex`
returned `Result: Failed (OtherCompilationError)` (~12 s). Root cause is a comment-termination bug in
this task's header. (SAC was clear; this is a genuine source defect, not the machine.)

**Root cause — `SiegePlayerController.h:1057`:** the doc comment for `bInHoldTargetMode` contains the
literal text `each Enter*/Begin*`. The embedded `*/` closes the `/** ... */` block comment early, so
the rest of the line (`Begin* ignores while any other is live). */`) is parsed as C++ and the
`bool bInHoldTargetMode = false;` declaration on line 1058 is never seen. `bInHoldTargetMode` is then
undeclared at every use site in the .cpp.

Errors (deduplicated):
```
SiegePlayerController.h(1057,141): error C2143: syntax error: missing ';' before '*'
SiegePlayerController.h(1057,136): error C4430: missing type specifier - int assumed
SiegePlayerController.h(1057,174): error C2059: syntax error: ')'
SiegePlayerController.h(1057,177): error C4138: '*/' found outside of comment
SiegePlayerController.h(1058,32): error C2238: unexpected token(s) preceding ';'
SiegePlayerController.cpp: error C2065: 'bInHoldTargetMode': undeclared identifier
  at lines 307, 743, 777, 798, 830, 869, 1562, 2022, 2030, 2153, 2158, 2860
```

**Fix (gameplay-programmer):** reword the comment so it contains no `*/` (nor `/*`), e.g.
`(each Enter/Begin mode ignores while any other is live)`. Pure comment edit, zero logic change.
**Re-QA scan:** grep the other new doc comments in this file for the same embedded-`*/` pattern before
re-QA — this is a comment-termination trap that pre-compile review missed once here.

Downstream TASK-277 (compile + PIE + commit) is BLOCKED until this compiles green. Build-master did not
modify any code.
