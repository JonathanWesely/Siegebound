# TASK-027 Handoff — Building base + tower (C++) + dynamic navmesh config

- author: gameplay-programmer
- date: 2026-07-03
- status: implementation complete, files-only (no compile, no editor, no Git, no TASKBOARD edit per dispatch; M2 wave 2 batch pattern). Session note: this task was interrupted mid-run by a token-limit termination after Building.h was written; resumed and completed against re-verified disk state (Building.h intact; TASK-028's SummonedUnit.h/.cpp re-checked — `IsUnitDead()` public API unchanged, buildings still fall into `IsTargetAlive`'s unknown-type-alive branch).

## Files

1. `Source/GitClaudeUnrealTest/Siegebound/Building.h` (new) — `ABuilding` (AActor + ITeamAgent)
2. `Source/GitClaudeUnrealTest/Siegebound/Building.cpp` (new)
3. `Source/GitClaudeUnrealTest/Siegebound/Tower.h` (new) — `ATower : ABuilding`
4. `Source/GitClaudeUnrealTest/Siegebound/Tower.cpp` (new)
5. `Config/DefaultEngine.ini` — appended `[/Script/NavigationSystem.RecastNavMesh]` with `RuntimeGeneration=Dynamic` (plus an ASCII comment block). Nothing else in the ini touched.

NOT touched (frozen upstream/sibling contracts, read-only): `CardRow.h`, `Castle.h/.cpp`, `Projectile.h/.cpp`, `DamageTypes.h/.cpp`, `SummonedUnit.h/.cpp` (TASK-028), `DeckComponent.h/.cpp` (TASK-022), `GitClaudeUnrealTest.Build.cs` (no new module deps — GameplayStatics/Timer/Nav-flag APIs are all Engine-level; towers use no Niagara).

## What was built

**ABuilding** (GDD §3.7 base, Wall = this class directly):
- ITeamAgent; `EditAnywhere` `CardID` (FName) + `Team` (ETeamId, default Blue).
- `VisualMesh` (UStaticMeshComponent) is the ROOT: explicit `BlockAll` profile + `SetCanEverAffectNavigation(true)`. No mesh/material set in C++ — BP children assign SM_&lt;CardID&gt; + MI_TeamColor_Blue (TASK-035, CONVENTIONS).
- BeginPlay loads `/Game/Data/DT_Cards` (soft path, identical to ASummonedUnit) → binds `MaxHP/CurrentHP` from the row's HP; missing table/row/CardID = error log, building stands statless. Sanity warnings for HP <= 0 and CardType != Building.
- `TakeDamage`: destroyed/<=0 early-out → same-team ignore via the TryGetDamageTeam chain → `Super` → LISTED damage (NO §3.0 type scaling — that lives only in ACastle, M2 ruling) → 0 HP = single-fire `HandleDestroyed()` → `Destroy()` (crumble FX is M7). Destroy unregisters the mesh from the nav octree, so under Dynamic generation the navmesh heals where a wall died.
- NO timers, NO tick in the base — qa/TASK-021 WARN-1 honored structurally: there is no code path by which ABuilding can start a cadence timer.
- `OnStatsLoaded(const FCardRow&)` protected virtual hook, called once after HP binds (base impl empty) — the single table load is shared with subclasses; also the enforcement point for "only ATower arms a timer".

**ATower** (Arrow Tower):
- `OnStatsLoaded` binds Damage/Range from the row, then the WARN-1 guard: `Row.Cadence <= 0` → warning log, **no timer is ever scheduled** (never clamped into validity); positive cadence floored at 0.05 s (`MinTowerCadence`, the ASummonedUnit `MinAttackCadence` mirror) and ONE looping timer armed. First shot lands one full cadence after stats bind.
- `ScanAndFire` every cadence: acquires the NEAREST alive ENEMY that is a unit or the hero (positive class gate: `ASummonedUnit` incl. subclasses / `AHeroCharacter`; dead-hero-hidden and dying-unit-flag liveness checks) within `Range` (origin distance); no target = idle, loop re-scans next cadence. No persistent target — every shot re-acquires, satisfying the TASK-026 "shooters re-validate at fire time" contract by construction.
- `FireProjectileAt`: `SpawnActor<AProjectile>` at `GetActorLocation() + MuzzleOffset` (EditAnywhere, default (0,0,200), cosmetic) with `Owner = this`, **Instigator deliberately unset** (TASK-026 handoff non-pawn-shooter form), `AlwaysSpawn`; then exactly one `InitProjectile(Team, Target, AttackDamage, USiegeDamageType_Projectile::StaticClass())`.

## Contracts consumed downstream

**TASK-030 (placement v2):**
- Spawn pattern (mirror of the TASK-007 unit pattern):
  `SpawnActorDeferred<ABuilding>(BPClass, Transform)` → `InitBuilding(ETeamId::Blue, CardID)` → `FinishSpawning(Transform)`; plain `SpawnActor` + `InitBuilding` right after also works (late-binds). BP children preset CardID, so passing the same CardID is a harmless confirm; Team always updates.
- Clearance rule: iterate `TActorIterator<ABuilding>` — ACastle is NOT an ABuilding (class-disjoint), so "castles are not buildings for this rule" holds with no special-casing. `GetActorLocation()` for distance; live-but-dying buildings expose `IsBuildingDestroyed()` if you want to skip them.
- Spawned buildings' root is Static-mobility — spawn AT the final placement point; never move one post-spawn.

**TASK-035 (BP children):**
- `/Game/Blueprints/Buildings/BP_Building_ArrowTower` — parent **ATower**, CardID `ArrowTower`, VisualMesh = SM_ArrowTower, slot 0 MI_TeamColor_Blue. Optional: tune `MuzzleOffset` (default Z+200) to SM_ArrowTower's real top.
- `/Game/Blueprints/Buildings/BP_Building_Wall` — parent **ABuilding**, CardID `Wall`, VisualMesh = SM_Wall, slot 0 MI_TeamColor_Blue.
- C++ already sets BlockAll + affects-navigation on VisualMesh; the BPs must NOT weaken collision or nav relevance (acceptance depends on both). Nothing stat-like goes on the BP.

**TASK-024 (game mode v2):** `class ABuilding` exists for PlayAgain's destroy-all (`TActorIterator<ABuilding>` → `Destroy()`; safe — EndPlay clears ATower's timer synchronously and the navmesh heals). See flagged decision 13 for a match-end gap it should also cover.

**TASK-028 note (already complete):** `ASummonedUnit::IsTargetAlive` treats buildings as unknown-ITeamAgent = alive; correct in practice because a dying building flags `bDestroyed` and `Destroy()`s in the same call (IsValid gates it out within the frame). Optional M2b hardening: add a `Cast<ABuilding> → !IsBuildingDestroyed()` branch there when SummonedUnit.h ownership next frees up.

## Config/DefaultEngine.ini — next-boot-only effect

- `RuntimeGeneration=Dynamic` under `[/Script/NavigationSystem.RecastNavMesh]` is read at engine/editor startup. **The currently running playtest editor is unaffected** — it keeps Static generation until restarted. The setting becomes live at the gated M2 compile/editor boot (TASK-039), which is also when anything can first spawn an ABuilding, so no behavior window exists where walls exist but can't carve.
- Build-master caveat for TASK-039/040: the ini sets the class default. If L_Arena's auto-created RecastNavMesh actor (TASK-015) had serialized a per-instance `RuntimeGeneration` override, the instance would win over the ini. Expected NOT to be the case (the navmesh was auto-created with defaults; delta-serialization skips unchanged properties), but if a placed wall does not carve at M2b PIE, check that actor's Runtime Generation property first — fix is a one-click editor change, not code.
- Until the DT_Cards reimport (TASK-031), spawned buildings bind HP = 0 from the stale table (qa/TASK-021 WARN-2 window) — sequencing already covered by that report's build-master notes.

## Flagged decisions — QA must rule on each

1. **API beyond the names block:** `InitBuilding(ETeamId, FName)` (BlueprintCallable), BlueprintPure getters `GetCurrentHP/GetMaxHP/IsBuildingDestroyed/GetCardID`, and the protected virtual `OnStatsLoaded` hook. InitBuilding is REQUIRED — TASK-030's spec says "Team=Blue" and Team is a protected UPROPERTY, so some public setter must exist; it is an exact mirror of `ASummonedUnit::InitUnit` (TASK-007 spawner pattern). Getters mirror the TASK-010/018 PIE-hook precedent; the hook shares one table load and structurally enforces "base starts no timer".
2. **VisualMesh is the root with explicit `BlockAll`** (ACastle precedent, qa/TASK-002). Spec says "blocks Pawns"; BlockAll additionally blocks visibility/camera/cursor traces — accepted consequences: the placement cursor hits building roofs (TASK-030's navmesh projection then refuses those points, matching the castle-roof rule) and the spring arm treats walls like any solid (castle-identical camera behavior).
3. **Mobility left at the component default (Static) for a runtime-spawned actor.** Buildings never move post-spawn (stationary per §3.7); the project has static lighting disabled (`r.AllowStaticLighting=False`); nav-octree registration/unregistration is mobility-independent, so spawn-carve and destroy-heal both work. No warning is emitted for spawning static-mobility actors — only for moving them, which nothing does.
4. **Fourth hand-mirror of the damage-team chain** (`ABuilding::TryGetDamageTeam`): ACastle's and ASummonedUnit's copies are PRIVATE statics in frozen files, so the chain is mirrored verbatim with the keep-in-sync comment — same precedent QA accepted in TASK-004 and TASK-026 ruling 10.
5. **Tower acquisition uses origin-to-origin distance (`FVector::DistSquared`), NOT a fourth mirror of the closest-point helper** — deliberate compliance with qa/TASK-026 NIT-4 ("consolidate, don't re-mirror"). Sound because every legal tower target is a PAWN (capsule radius ~35 uu — noise against a 900 gate), unlike units/projectiles which must range-test 800-uu-wide castles; the projectile's own closest-point impact test still governs the actual hit.
6. **"Targets units/hero" implemented as a POSITIVE class gate** (`Cast<ASummonedUnit>` / `Cast<AHeroCharacter>`), not as an exclude-list. Encodes §3.7 exactly, automatically covers subclasses (AMinerUnit — raidable per §3.3), and can never accidentally acquire a future non-pawn ITeamAgent (spec's "exclude ACastle/ABuilding" is thereby a theorem, not a check).
7. **Cadence guard semantics (qa/TASK-021 WARN-1, binding):** base has no timer code at all; ATower refuses to schedule when `Cadence <= 0` (warned) and NEVER clamps a non-attacking row into a firing one; positive cadences floored at 0.05 s (`MinTowerCadence`, ASummonedUnit mirror).
8. **Always-looping fire timer + fresh acquisition each cadence; first shot at +1 cadence after stats bind.** No persistent target, no cooldown bookkeeping (a tower never "swaps targets mid-cooldown" — it has no cooldown state beyond the loop itself); literal reading of "no target in range = idle, re-scan next cadence". A tower placed next to an enemy fires 1.5 s later, not instantly — spec's "every 1.5 s" satisfied either way.
9. **Statless building (missing table/row/CardID) dies to the first enemy hit** (CurrentHP stays 0) rather than standing invincible — the ASummonedUnit failure-mode precedent; the misconfiguration is already an error log.
10. **No damage-type scaling in ABuilding::TakeDamage** — listed damage from everything, per the M2 ruling and the TASK-026 handoff's explicit instruction ("TASK-027's ABuilding must likewise NOT scale").
11. **Tower shot attribution: `Owner = this` only; `Instigator` deliberately unset** (TASK-026 handoff non-pawn-shooter form — there is no pawn it would be honest to attribute). Receivers resolve tower hits as unattributable-and-apply per their documented contract; friendly-fire safety = enemy-only acquisition (first line) + the projectile's same-team impact gate (qa/TASK-026 ruling 3 backstop — untouched by this task).
12. **No team-visual logic in C++** — TASK-035 sets MI_TeamColor_Blue statically on the BPs, and M2 has no enemy buildings (bot is M3). Carry-forward: the M3 bot needs a team-material pass on ABuilding (mirror of ACastle::ApplyTeamVisuals) before Red can place buildings.
13. **Carry-forward gap for TASK-024 (mirrors qa/TASK-026 WARN-1):** the match-end freeze contract covers units (FreezeAI), income, and clock — but NOT towers. A live tower keeps firing at frozen units after Victory (bounded: single-target shots, enemies frozen in place). Suggested TASK-024 addition: at match end, `TActorIterator<ATower>` → clear/stop firing (needs a tiny public method on ATower at that point, or simply destroy all ABuildings at match end instead of at PlayAgain). Not fixable inside this task's file scope.

## Acceptance mapping (TASK-027 spec)

- ArrowTower-row tower: fires at the nearest enemy inside 900 every 1.5 s for 15 — Range/Cadence/Damage all from the row; projectile-typed so a castle hit would scale to 7.5 castle-side (towers never target castles; the tag stays honest). Beyond 900 ignored (`DistSq > RangeSq`).
- Wall-row ABuilding: 300 HP from the row; BlockAll stops pawn capsules; nav-relevant mesh + Dynamic runtime generation reroutes pathing (live only after the ini takes effect at next boot); dies to 300 cumulative damage → Destroy → navmesh heals.
- Both refuse friendly damage (TryGetDamageTeam same-team early-out, zero broadcasts/side effects).
- Nothing stat-like hardcoded: HP/Damage/Range/Cadence are row-bound; 0.05 timer floor, 200-uu muzzle default, and BlockAll are mechanics/feel constants with GDD/QA comments, not card stats.

## Notes for build-master

- Two new class pairs, normal UBT pickup; no Build.cs/.uproject changes. Compiles independently of TASK-022/028 deliverables (includes of SummonedUnit.h/HeroCharacter.h/Projectile.h/DamageTypes.h/CardRow.h are read-only consumption of already-QA'd or frozen headers).
- Sequence per qa/TASK-021: TASK-031 reimport immediately after the TASK-039 compile, before any PIE — buildings read HP from DT_Cards and a stale table gives 0-HP buildings.
- M2b PIE checks for this task: wall reroute (needs the rebooted editor + Dynamic navmesh — see the ini caveat above), tower cadence/range/damage, friendly-fire refusal, wall death heals pathing, tower death stops firing.
