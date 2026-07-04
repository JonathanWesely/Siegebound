# QA Report — TASK-030 (Placement v2: navmesh projection, building clearance, generalized spawn, miner cap)

Verdict: **PASS**
Reviewer: qa-reviewer
Date: 2026-07-04
Counts: **0 BLOCKER · 2 WARN · 3 NIT**

Files reviewed (pre-compile; TASK-039 owns the batch build):
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` / `.cpp` (placement v2, layered on the TASK-023 qa-passed baseline)
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp` (out-of-names-block edit — WARN-1 closure)
- `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs` (out-of-names-block edit — `+NavigationSystem`)

Cross-task dependency headers read to confirm exact signatures: `SiegePlayerState.h`, `Building.h`, `DeckComponent.h`, `SummonedUnit.h`, `HeroCharacter.h`, `Castle.h`. Engine headers read to confirm the NavigationSystem API surface: `UE_5.8/.../NavigationSystem.h`.

---

## Rulings on the three flagged items

### 1. SiegeGameMode.cpp edit — drop the redundant game-mode-side `ResetDeck` (+ dead `else` + unused include). **APPROVED — clean strict-subtraction; §3.9 Play Again semantics preserved.**
- `PlayAgain` step 6 (cpp:505-517) now only calls `SiegePC->HandleMatchReset()` per controller; the direct `FindComponentByClass<UDeckComponent>()->ResetDeck()` and its dead `else` warn are gone, and `#include "Siegebound/DeckComponent.h"` is removed.
- Verified **no orphan reference**: the only remaining `DeckComponent`/`ResetDeck` tokens in the file are in comments (cpp:507, 509). Nothing live uses `UDeckComponent`, so dropping the include cannot fail to compile.
- Verified the removal does **NOT** break TASK-024 (already qa-passed): `FreezeWorldAtMatchEnd` (units frozen / towers silenced / projectiles cleared / `PauseIncome` / `StopClock`) is untouched (cpp:168-247), and `PlayAgain` still performs the full economy+clock reset — `ResetClock` (3b, cpp:467-470), `ResetEconomy`/`ResetGold`/`ResumeIncome` (4, cpp:485-496). `HandleMatchReset` is now the single §3.9 deck-reset entry point; `DeckComponent->ResetDeck()` fires there null-safe (controller cpp:773-776). Exactly one fresh 6-card deal per PlayAgain.

### 2. Build.cs `+"NavigationSystem"`. **APPROVED — correct and necessary; no duplicate / no ordering issue.**
- TASK-030 is the first direct `UNavigationSystemV1` consumer (`GetCurrent`, `GetDefaultNavDataInstance`, `ProjectPointToNavigation`), previously only transitive via `AIModule`. Declaring the dependency explicitly is correct.
- Single entry (Build.cs:18), placed after `AIModule`; module-name order is irrelevant to UBT. No duplicate. Coexists cleanly with the existing set including TASK-016's `Niagara`.

### 3. `CastlePlinthClearance` keep-out box (420.f 2D half-extent per `ACastle`). **APPROVED — acceptable belt-and-braces; not over-reach. WARN-watch for PIE.**
- It **strengthens** the spec's navmesh-projection rule (§3.5 / point 1) rather than contradicting it: it guarantees plinth-rim navmesh islands can never validate a point nothing can path to. It is a commented, `ClampMin=0` mechanic `UPROPERTY` (h:406-407), not a hardcoded stat (CONVENTIONS mechanic-rule registry).
- Geometry sanity: Blue castle at X=-2000 (CONVENTIONS world axes), Blue half is X<=0. A 420 half-extent box removes X∈[-2420,-1580], Y∈[-420,420] — a small footprint hard against the back wall; the vast majority of the Blue half stays placeable. Red castle's box is irrelevant (already excluded by X<=0). Not over-restrictive for M2.
- See WARN-2: confirm at TASK-040 PIE that 420 does not clip legitimate back-line building placement.

---

## Findings

- **[WARN]** SiegePlayerController.cpp:864/1045 — Ghost preview spawns with `GhostYawOffset` (-90°, SM_<CardID> facing convention) but confirmed **buildings** spawn with `FRotator::ZeroRotator`. For a long/asymmetric building mesh (e.g. Wall) the preview will face differently than the placed actor. Cosmetic only; building meshes/BPs do not exist yet (TASK-035/037/038). Suggested: give TASK-035 BP authors awareness, or add per-type ghost yaw when the wall mesh lands. (Programmer pre-flagged; concur.)
- **[WARN]** SiegePlayerController.h:406-407 / .cpp:1261-1289 — `CastlePlinthClearance = 420.f`: verify at TASK-040 PIE that the keep-out box does not over-restrict legitimate Blue-half building/unit placement near the own castle. Ruled acceptable for pre-compile; this is a runtime tuning watch, not a code defect.
- **[NIT]** SiegeGameMode.cpp:510 — comment cites `handoffs/TASK-030.md`; the actual handoff file is `handoffs/TASK-030-programmer.md`. Doc-only.
- **[NIT]** SiegePlayerController.cpp:905-911 — unit spawn lift reads `ActorClass->GetDefaultObject<ASummonedUnit>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()`; a BP capsule-size override is not always reflected on the CDO, so the initial ground lift can be slightly off. Self-corrects via CharacterMovement; pre-existing M1/TASK-007 idiom, cosmetic.
- **[NIT]** SiegePlayerController.h:162-169 — the `ExitPlacementMode` doc-comment's exit-path list predates the two new confirm-time refusal exits (they ARE itemized in the class-level comment, h:88-94 + cpp:822-848). Doc completeness only.

No BLOCKERs. No deprecated UE 5.8 APIs. No null-safety gaps. No header/cpp asymmetry.

---

## Standard-check confirmations

- **Deprecated/removed APIs:** none. All NavigationSystem calls verified against the installed UE 5.8 header:
  - `UNavigationSystemV1::GetCurrent(UWorld*)` — NavigationSystem.h:1177.
  - `GetDefaultNavDataInstance()` const (public, returns MainNavData) — NavigationSystem.h:759.
  - `ProjectPointToNavigation(Point, FNavLocation&, Extent)` — the non-const inline overload (NavigationSystem.h:712) is unambiguously selected for the non-const `NavSys` and forwards to :718. No ambiguity, no deprecation.
- **Cross-task contracts resolve with exact signatures:**
  - `ASiegePlayerState::CanAddMiner()` (h:143), `GetAliveMinerCount()` (h:139), `CanAfford(int32)`→bool (h:81), `SpendGold(int32)`→bool (h:88), `PauseIncome/ResumeIncome/ResetGold/ResetEconomy` — TASK-024. ✔
  - `ABuilding::InitBuilding(ETeamId, FName)` (h:78), `IsBuildingDestroyed()`→bool (h:98), `GetActorLocation()` inherited — TASK-027. ✔
  - `UDeckComponent::GetHandCardID(int32)`→FName (h:108), `ConfirmPlayFromHand(int32)`→bool (h:128), `DiscardFromHand(int32)`→bool (h:137), `ResetDeck()`/`BuildAndShuffle()` — TASK-022. ✔
  - `ASummonedUnit::InitUnit(ETeamId, FName)` (h:117) + `GetCapsuleComponent()`; `AHeroCharacter::GetTeamId/IsDead/SetMeleeSuppressed/OnHeroDied`; `ACastle`; `ECardType`. ✔
- **Null-safety on composed soft paths:** `ResolveCardActorClass` (cpp:1132) — `/Game/Blueprints/Units/BP_Unit_<CardID>_C` or `/Game/Blueprints/Buildings/BP_Building_<CardID>_C`, `LoadSynchronous` + `IsChildOf(RequiredBase)`; missing/incompatible → nullptr → `RefuseCardPlay("Card actor unavailable")` + `ExitPlacementMode`, **NO gold spent** (confirm-path resolve precedes any `SpendGold`). Ghost mesh `/Game/Meshes/SM_<CardID>` → engine Sphere → invisible (cpp:1179). `M_Ghost` MID, HUD/Victory widgets, all IA_* soft paths, DT_Cards — every load null-checked; every failure logs and degrades, none crash.
- **Melee-suppression law (qa/TASK-003 warning 2):** all 10 exit paths funnel through `ExitPlacementMode`, which releases `SetMeleeSuppressed(false)` **before** the `!bInPlacementMode` early-out (cpp:630-639): confirm-success (972), confirm miner-cap refusal (828, NEW), confirm missing-BP refusal (846, NEW), cancel action (495), polled RMB/Esc (196), match end (660), hero death (509), unpossess (233), match reset (748), EndPlay (111). The two deliberate stay-in-mode refusals (invalid point; SpendGold-refused-at-confirm) correctly keep suppression on. Law holds.
- **Header/cpp symmetry:** every method declared in the header has exactly one matching definition; no orphan declarations, no stub/undeclared definitions. Complete.
- **Miner cap net-zero:** gated at ENTRY before any gold/mode state (cpp:585-592) and re-gated at CONFIRM (cpp:822-830) with `ExitPlacementMode` + hand card retained; `PlayHandSlot` clears `PendingHandSlot` on refused entry (cpp:383-387). Exact string "Miner limit reached". Zero gold movement.
- **Confirm gold ordering:** SpawnActorDeferred → SpendGold (destroy half-spawned actor on refusal) → Init(Team, CardID) + FinishSpawning → `ConfirmPlayFromHand` — gold is the last gate; card leaves the hand only on commit (M1/TASK-007 idiom, preserved for both Unit and Building).
- **CONVENTIONS / names-block match:** spawn paths, ghost path, `M_Ghost`/`GhostColor`, `BuildingClearance` UPROPERTY, Blue half X<=0 — all character-for-character with the TASK-030 names block and CONVENTIONS §"Blueprint subclasses"/§"Per-card visual assets"/§"World axes".
- **No hardcoded stats:** Cost/CardType/HP all from DT_Cards; `BuildingClearance` (200), `CastlePlinthClearance` (420), `NavProjectionExtent`, `DiscardCost`, `MinerCardID` are commented mechanic UPROPERTYs (CONVENTIONS registry), not CSV columns.

## TASK-023 baseline preservation (confirmed intact, not clobbered)
`PlayHandSlot` (298), `DiscardHandSlot` (410), `OnCardRefused`/`RefuseCardPlay` (1291)/`BroadcastRefusal` (1301), `DeckComponent` default subobject (ctor 45), input slots `Card2Action..Card6Action`+`UICursorAction` with `ClearUICursorHold`/`ApplyCursorInputState`, the empty-slot discard pre-check BEFORE `SpendGold` (cpp:445-453, qa/TASK-022 WARN-1 guard), and the key-1 empty-hand Footman fallback (`OnCard1Pressed`, 238) — all present and behavior-consistent with qa/TASK-023-report.md. Placement v2 is layered strictly behind the `EnterPlacementMode(FName)` entry + `PendingHandSlot` confirm contract, per the TASK-023 seam.

## Carry-forward WARN closures (all three genuinely closed)
- **qa/TASK-023 WARN — defensive `ExitPlacementMode` in `HandleMatchReset`: CLOSED.** First statement of `HandleMatchReset` (cpp:748), ahead of `ResetDeck`; a mid-placement reset can no longer rebuild the hand under a live `PendingHandSlot`.
- **qa/TASK-027 WARN-2 — deferred `InitBuilding` gets a REAL CardID: CLOSED.** `Building->InitBuilding(Team, PendingCardID)` (cpp:896), `PendingCardID` set from the played card in `EnterPlacementMode`; never `NAME_None`.
- **qa/TASK-024 WARN-1 — PlayAgain double `ResetDeck`: CLOSED.** Game-mode-side reset dropped (ruling 1 above); `HandleMatchReset` is the sole reset; TASK-024 freeze / economy / clock reset all preserved.

## Notes for build-master (on PASS)
- Two files outside the TASK-030 names block were edited and are approved: `SiegeGameMode.cpp` (strict subtraction) and `Build.cs` (`+NavigationSystem`). `Build.cs` changed, so TASK-039's compile must be a full rebuild of the editor target (module dependency delta), not an incremental hot-reload.
- Expected designed-degradation logs until the parallel editor tasks land (all null-safe, none fatal): 6 input-action warns (IA_Card2..6/IA_UICursor → TASK-032), per-card ghost-mesh fallback logs (→ TASK-014/037/038), missing BP-class refusals (→ TASK-034/035), missing WBP/M_Ghost logs (→ TASK-011/012), one no-navmesh degrade-open warn only if a world lacks nav data (never in L_Arena).
- No engine/MCP or Git action performed by QA. Code was not compiled — this is the pre-compile safety gate.
