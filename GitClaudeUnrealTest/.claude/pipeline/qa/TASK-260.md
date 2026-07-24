# QA Report — TASK-260

Verdict: **PASS**
Reviewer: qa-reviewer
Branch: m7.6-arena10x
Date: 2026-07-23
Scope: pre-compile code review of new `ACaptureZone` + the SiegeGameMode PlayAgain reset hook. No compile/commit until PASS (this is it).
Blocker count: **0**

Files reviewed:
- `Source/GitClaudeUnrealTest/Siegebound/CaptureZone.h` (new)
- `Source/GitClaudeUnrealTest/Siegebound/CaptureZone.cpp` (new)
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp` (edited — PlayAgain reset loop, step "3a2", +1 include)

## Acceptance — capture state machine matches Jonathan's ruling EXACTLY
Verified against the board RULING (TASKBOARD TASK-260 line 882) and the handoff, NOT the stale CONVENTIONS line:
- `blue>0 && red==0` → Blue ✓ (EvaluateCapture:195)
- `red>0 && blue==0` → Red ✓ (:199)
- `blue>0 && red>0` → **Neutral** when `bNeutralizeWhenContested` (default) ✓ (:203-212)
- `blue==0 && red==0` → **unchanged** (`NewOwner` seeded from `CaptureOwner`, no branch taken → empty latches last owner) ✓ (:194, :213)
- `bNeutralizeWhenContested` defaults **TRUE** ✓ (CaptureZone.h:172) — sticky survives as the off-state, exactly the ruling.
- `CaptureOwner` inits Neutral ✓ (:209). "Unit" = `ASummonedUnit` (AMinerUnit confirmed a subclass — MinerUnit.h:107, so the `TActorIterator<ASummonedUnit>` covers miners) + `AHeroCharacter`; dead skipped (`IsUnitDead()`/`IsDead()`, both confirmed public); buildings/towers/castles/gold-nodes excluded by type ✓.

## Filter results
- **Deprecated/removed UE 5.8 APIs:** none. `UDecalComponent::SetDecalMaterial` + `CreateDynamicMaterialInstance()`, `DecalSize`, `MarkRenderStateDirty()`, `TActorIterator` (EngineUtils), `TSoftObjectPtr::LoadSynchronous`, `SetVectorParameterValue`, `GetTimerManager().SetTimer/ClearTimer`, `HasAuthority()` — all current and correctly used.
- **Box test:** 2D XY `FMath::Abs(delta) <= HalfExtent`, Z ignored, about `GetActorLocation()` — correct and matches the documented spawn/region-test contract. Inclusive on the boundary; fine for a spawn gate.
- **Null safety:** `EvaluateCapture` null-checks World + `IsValid()` on every iterated unit/hero; missing `/Game/Materials/M_CaptureZone` ⇒ no MID, `ApplyOwnerColorToDecal` no-ops on `!ZoneDecalMID`, mechanic still runs, warned once (`bWarnedMissingMaterial`) ✓; `ZoneDecal` null-guarded in every consumer; `EndPlay` clears the timer under a World guard.
- **Authority + timer:** eval timer armed ONLY under `HasAuthority()` + valid World (BeginPlay:91-99), cleared in EndPlay — no dangling timer, no per-tick work (`bCanEverTick=false`). Consistent with SiegeGameState/SiegeGameMode owning match state locally through M7; M8 replication flagged in the class doc + code comment.
- **UE reflection/GC:** components + MID are `TObjectPtr` UPROPERTYs (GC-safe); delegate is `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams` + `BlueprintAssignable`; `ECaptureState` is a `UENUM(BlueprintType)` declared in the owning header BEFORE the delegate that references it; forward-decl of `ACaptureZone` for the delegate is the standard pattern. Header/cpp fully consistent — all 14 declared members defined, no orphans.
- **Team identity:** reuses `ETeamId` + `ITeamAgent::GetTeamId()` from TeamId.h (the exact identity `ASummonedUnit`/`AHeroCharacter` expose). `TeamToState` maps Blue↔Blue / Red↔Red. No parallel team concept. ✓
- **Public API (TASK-261/262 consumers):** `CanTeamSpawnHere(ETeamId, const FVector&) const`, `IsPointInZone(const FVector&) const`, `GetCaptureOwner() const`, `GetZoneHalfExtent() const`, `ResetCaptureZone()`, delegate `FOnCaptureZoneOwnerChanged` — coherent and const-correct. `CanTeamSpawnHere` folds box-test AND owner-match; Neutral ⇒ false for both teams (correct — nobody spawns in an unheld mid). ✓
- **Change-only broadcast/tint:** `SetCaptureOwner` early-returns when `NewOwner == CaptureOwner`, so decal re-tint + `OnCaptureOwnerChanged.Broadcast` fire ONLY on a real owner change — no 0.5 s spam (verified for contested-on-Neutral and empty-latch cases too). `ResetCaptureZone` broadcasts UNCONDITIONALLY per the reset-path law. ✓
- **Conventions ("W1-PREP additions 3"):** file paths, class/member/enum/delegate names, `ZoneColor` param, `/Game/Materials/M_CaptureZone` soft path, owner colors (Neutral 0.5/0.5/0.5, Blue 0.05/0.30/1.00, Red 1.00/0.10/0.05) all match. Shadow law honored: member is `CaptureOwner`, never `Owner`; no shadows of Owner/Instigator/Controller/PlayerState/Slot. ✓
- **Includes (complete-type law):** DecalComponent, SceneComponent, World, EngineUtils, MaterialInstanceDynamic, MaterialInterface, HeroCharacter, SummonedUnit, TimerManager, GitClaudeUnrealTest(log) all present for every dereferenced type. (MinerUnit intentionally not needed — reached polymorphically via the ASummonedUnit iterator.) ✓

## Findings
- [NIT] CaptureZone.h:41 / CaptureZone.cpp decal recipe — decal projects via relative pitch -90 with `DecalSize=(DecalProjectionDepth, ZoneHalfExtent.X, ZoneHalfExtent.Y)`, mirroring the M_SpellReticle recipe. Correct-by-convention; the ACTUAL floor-projection read is a visual check owned by TASK-263/264, not a code defect.
- [NIT] TASKBOARD TASK-260 `parallel-safe` one-liner says "new files CaptureZone.{h,cpp} only," but the spec BODY explicitly authorizes the Play-Again reset-path hook, which necessarily edits `SiegeGameMode.cpp`. The edit collides with NO sibling task (261=SiegePlayerController, 262=SiegeBotController, 263=material), so parallel-safety holds in practice — but note that `SiegeGameMode.cpp` is now in the branch's touched-file set and is NOT enumerated in the CONVENTIONS "W1-PREP additions 3" lane list. Manager awareness for the Phase-6 merge only; not a code issue.

## Ruling — Flag 1 (PlayAgain reset wiring in SiegeGameMode::PlayAgain)
**KEEP IT IN TASK-260. Correct, null-safe, in-scope.**
- Correct + null-safe: step "3a2" (SiegeGameMode.cpp:585-588) is a `TActorIterator<ACaptureZone>→ResetCaptureZone()` loop placed right after the ResetCastle loop, mirroring it byte-for-byte. TActorIterator yields only valid live actors, so no zone ⇒ clean no-op; `GetWorld()` is used bare exactly as every adjacent step (2/2b/2c/3) does in this already-live-world function. Include added (SiegeGameMode.cpp:14). `ResetCaptureZone` is a defined public method — complete type available.
- Genuinely required, not optional: PlayAgain step 2 destroys every unit BEFORE the next eval, so an empty-zone eval would latch the pre-reset owner (empty = unchanged) and a Blue/Red mid zone would carry into the new match. The forced reset is the only thing that makes TASK-264 PIE test (g) "Play-Again ×3 ⇒ Neutral" pass.
- In-scope for TASK-260, NOT TASK-264: the TASK-260 spec literally says "Play-Again resets CaptureOwner→Neutral (expose a public reset the reset path can call; match how ACastle/AGoldNode reset)." Wiring the reset path is programmer work; build-master writes no code. TASK-264 only TESTS it. This is the correct home.

## Ruling — Flag 2 (stale CONVENTIONS "STICKY default false")
**Code matches the RULING, not the stale doc — confirmed.** The implementation is Contested→Neutral with `bNeutralizeWhenContested` defaulting TRUE, matching the board RULING (TASKBOARD:882) and the handoff. CONVENTIONS "W1-PREP additions 3" line 182 still reads "STICKY = interpretation A ... default false" — that is manager-owned doc drift the manager is reconciling in parallel; it does not gate this code. No action for the programmer.

## Notes for build-master (if PASS)
- Compile rides TASK-264 (editor-bounce batch) with TASK-261/262. This class adds no new module deps.
- **Heads-up on the TASK-264 PIE suite (d):** the board's step (d) still reads "contested — Blue+Red units both inside ⇒ ownership UNCHANGED (sticky)." That expectation is STALE vs Jonathan's ruling — with `bNeutralizeWhenContested` default TRUE the code correctly drives contested → **Neutral**. Update test (d) to expect ownership → Neutral (decal gray, mid unspawnable for both) so the correct code isn't failed against a stale assertion. (Test (c) capture-to-Blue, (e) neutral-unspawnable, (f) Red mirror, (g) Play-Again×3→Neutral are all consistent with the shipped code.)
- Reset path verified: `ASiegeGameMode::PlayAgain` calls `ResetCaptureZone()` on every zone (step 3a2) — test (g) will pass with the shipped wiring.
- CaptureZone_Center placement at (0,0,0) with default ZoneHalfExtent (840,840) needs no BP; a plain ACaptureZone instance at origin is correct (matches the class doc).
