# QA Report — TASK-044
Verdict: PASS

Team-driven visuals: `ApplyTeamMaterial` on `ASummonedUnit` + `ABuilding` at BeginPlay (VisualMesh slot 0 → `MI_TeamColor_<Team>`); `AMinerUnit` covered via `Super::BeginPlay`; idempotent re-apply in `InitUnit`/`InitBuilding` (HasActorBegunPlay branch); function-local static soft-ref cache, null-safe. Also carries the TASK-043 WARN closure (`bWarnedNoTeamPlayerState`) in `MinerUnit`. Reviewed pre-compile (not built).

Counts: 0 BLOCKER, 1 WARN, 2 NIT.

## Findings

- [WARN] MinerUnit.cpp:365-368 — `ResolveOwningPlayerState`'s `bWarnedNoTeamPlayerState` one-shot short-circuits the resolver to `return nullptr` WITHOUT re-calling `GetPlayerStateForTeam`, so once latched it stops resolution *retries*, not only the log. This is a behavioral change vs TASK-043 (which re-queried every poll). Verified BENIGN: the latch only fires on the live-GameState + no-team-PS path, which never occurs in the designed M3 flows — each team's `ASiegePlayerState` is created before any miner of that team can spawn (Blue PS at login, well before any card-played miner; the bot's Red PS is created synchronously with the bot at match start, before its 2 s decision loop can ever play a Miner). GameState is always created before any PlayerState, so there is no "live GameState but late-appearing team PS" window a real miner's poll could observe. A genuinely mis-teamed miner (a team with no economy) correctly latches and idles without income — the intended degraded behavior. The no-GameState-yet branch (`bWarnedNoOwnerState`, lines 341-348) is untouched and keeps retrying. Accepted as the correct closure of the TASK-043 WARN; flagged only so the record is honest that the guard gates resolution, not merely logging.

- [NIT] SummonedUnit.cpp:144-145 / Building.cpp:79-80 — the two `MI_TeamColor_*` paths resolve through function-local `static const TSoftObjectPtr` (weak). Once `SetMaterial(0, …)` runs, the SM component's `OverrideMaterials[0]` hard-refs the instance and keeps it resident, so in practice it never unloads; a full GC of every team actor followed by a respawn could trigger one synchronous re-load hitch. Matches the spec's "cached static resolve" wording and the QA-blessed AProjectile precedent. A `UPROPERTY` hard cache would be GC-proof at the cost of 2 UPROPERTYs × 2 classes — programmer chose the lighter static per spec. Acceptable.

- [NIT] SummonedUnit.cpp:144-145 / Building.cpp:79-80 — the MI instance paths are hand-duplicated across the two cpp files (no shared constant). Programmer already flagged this in the handoff; mirrors the existing `TryGetDamageTeam` hand-mirror precedent. Keep the two path pairs in sync by hand if the MI paths ever move.

## Verification detail

**All 3 actor types color correctly (Red→red, Blue→blue).**
- `ASummonedUnit::ApplyTeamMaterial` called at end of `BeginPlay` (SummonedUnit.cpp:122) — covers units.
- `AMinerUnit::BeginPlay` calls `Super::BeginPlay()` FIRST (MinerUnit.cpp:69), which runs the base apply with the miner's own `Team` — miners covered, no material code duplicated into the miner. `ApplyTeamMaterial` is private on the base but invoked from the base's own BeginPlay, so no access issue. `AMinerUnit` does not override `InitUnit`, so it inherits the same re-apply.
- `ABuilding::ApplyTeamMaterial` called at top of `BeginPlay` after `Super` (Building.cpp:58) — covers buildings.
- Selection `(Team == ETeamId::Red) ? RedTeamMaterial : BlueTeamMaterial` (SummonedUnit.cpp:147 / Building.cpp:82) is correct. Both `MI_TeamColor_Blue.uasset` and `MI_TeamColor_Red.uasset` exist in `Content/Materials/Instances/`; paths match CONVENTIONS.md:61.

**No spawn-timing hole.**
- Deferred path (real spawn, SiegePlayerController.cpp:865-943 for units/buildings; the TASK-046 bot reuses these rules): `SpawnActorDeferred → Init*(Team, CardID) → FinishSpawning`. `Init*` sets `Team = InTeam` (SummonedUnit.cpp:165 / Building.cpp:91) while `HasActorBegunPlay()` is false, so the in-`Init` re-apply is skipped; the single BeginPlay apply then runs with the correct Team. No stale/default apply.
- Plain `SpawnActor` + `Init*` path: BeginPlay applies with default Blue, then `Init*` sets the real Team FIRST and the `HasActorBegunPlay()` branch re-applies for the corrected Team. Ordering is correct in both `InitUnit` and `InitBuilding` (Team assignment precedes the re-apply). A bot Red unit can never render blue.

**Blue-side byte-for-byte no-op.** For a Blue actor the resolver returns the identical `MI_TeamColor_Blue` and `SetMaterial(0, …)` re-applies the same `UMaterialInterface` — no MID, no render change (an override entry pointing at the same material renders identically). Only ElementIndex 0 is touched: the -90° yaw VisualMesh pose, the TASK-020 melee/lunge path, the TASK-028 ranged path, and TASK-042's `ApplyMoveSpeedBuff` are all untouched. TASK-042 confirmed undisturbed — `ApplyMoveSpeedBuff`/`EndMoveSpeedBuff` intact (SummonedUnit.cpp:235-294); BeginPlay's only addition is the `ApplyTeamMaterial()` call; EndPlay still clears the buff timer; FreezeAI still calls `EndMoveSpeedBuff`.

**TASK-043 WARN closure sound.** Guard sits AFTER the `!SiegeGameState` early return (return at MinerUnit.cpp:348-349; guard at 365-368), so the no-GameState-yet retry is unaffected. `GetPlayerStateForTeam` (SiegeGameState.cpp:96-98) logs unconditionally on the not-found path, so latching before the re-call turns ~4 lines/sec into exactly one. Resolution-suppression nuance captured as WARN above — benign in all designed flows.

**Null-safety / shadow / API.**
- `VisualMesh` null-checked at top of both applies; `LoadSynchronous()` result guarded (`if (UMaterialInterface* ResolvedTeamMat = …)`), so a missing asset leaves the authored slot — never a crash. Slot index 0 always valid for `SetMaterial`.
- C4458: locals `TeamMat`, `ResolvedTeamMat`, `BlueTeamMaterial`, `RedTeamMaterial`, `OwnerState` shadow no inherited reflected UPROPERTY (Owner/PlayerState/Instigator/Controller/Slot). New member `bWarnedNoTeamPlayerState` is distinct from base `bWarnedNoAIController` and the sibling `bWarned*` guards — no shadow.
- UE 5.8: `UStaticMeshComponent::SetMaterial`, `TSoftObjectPtr::LoadSynchronous`, `HasActorBegunPlay` all current (non-deprecated). Includes: `Materials/MaterialInterface.h` added to both cpp — required for the full type. Header/cpp decls consistent (private `void ApplyTeamMaterial()` in both headers). `ApplyTeamMaterial` correctly not a UFUNCTION (not reflected/BP-called); new guard bool matches the sibling plain-bool pattern.

## Notes for build-master (if PASS)
- Clean for the TASK-051 M3 batch compile (TASK-042..047). No new UPROPERTYs, no new includes beyond `Materials/MaterialInterface.h` in the two cpp. Pre-compile shadow scan clean.
- The two `MI_TeamColor_*` paths live hand-duplicated in SummonedUnit.cpp and Building.cpp (NIT above) — no action, just awareness if paths ever move.
