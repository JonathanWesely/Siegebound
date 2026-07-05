# TASK-063 Handoff — BP_Building_* for the 4 Set II buildings (editor/MCP)

- author: gameplay-programmer
- date: 2026-07-04
- status: complete — 4 building BPs created in `/Game/Blueprints/Buildings/`, cloned from the BP_Building_ArrowTower/Wall recipe (TASK-035). All compiled clean (warnings-as-errors), CDO-verified, SIE-verified (parent + DT_Cards stat bind + collision + nav), saved to disk. No Git (TASK-069 commits). No compile of C++ (code is in 65861ce). TASKBOARD.md NOT edited (orchestrator owns the board).
- editor: left UP (PID 6172, 65861ce DLL — never booted/closed). MCP stable throughout: BP create/compile, Object get/set properties, Scene add/remove, PIE Simulate start/stop, Asset save, Log reads — zero socket hiccups. **No UMG ops performed** (the op class that has historically killed the MCP socket was avoided entirely).

## What was built — 4 data-only child BPs

Each is a data-only child of a 65861ce-compiled C++ class. **Nothing stat-like is set on any BP** — every HP/damage/range/cadence/interval/lifetime/income value binds from `/Game/Data/DT_Cards` at BeginPlay (verified in SIE below). `MuzzleOffset` left at the C++ default `(0,0,200)` on the two towers (cosmetic feel value, not a stat; matches the TASK-035 ArrowTower recipe which likewise left it at default).

| BP | Parent (C++) | CardID | VisualMesh StaticMesh | Slot 0 material | Collision profile | nav |
|----|--------------|--------|-----------------------|-----------------|-------------------|-----|
| `/Game/Blueprints/Buildings/BP_Building_BombTower` | `ATower` (`/Script/GitClaudeUnrealTest.Tower`) | `BombTower` | `/Game/Meshes/SM_BombTower` | `MI_TeamColor_Blue` | `BlockAll` (blocks Pawns) | `bCanEverAffectNavigation=true` |
| `/Game/Blueprints/Buildings/BP_Building_BallistaTower` | `ATower` (`/Script/GitClaudeUnrealTest.Tower`) | `BallistaTower` | `/Game/Meshes/SM_BallistaTower` | `MI_TeamColor_Blue` | `BlockAll` (blocks Pawns) | `bCanEverAffectNavigation=true` |
| `/Game/Blueprints/Buildings/BP_Building_Barracks` | `ABarracks` (`/Script/GitClaudeUnrealTest.Barracks`) | `Barracks` | `/Game/Meshes/SM_Barracks` | `MI_TeamColor_Blue` | `BlockAll` (blocks Pawns) | `bCanEverAffectNavigation=true` |
| `/Game/Blueprints/Buildings/BP_Building_DeepMine` | `ADeepMine` (`/Script/GitClaudeUnrealTest.DeepMine`) | `DeepMine` | `/Game/Meshes/SM_DeepMine` | `MI_TeamColor_Blue` | `BlockAll` (blocks Pawns) | `bCanEverAffectNavigation=true` |

### Parent-class decisions (recorded per TASK-056 / TASK-057 handoffs)
- **BombTower & BallistaTower → `ATower` directly, NO subclass.** TASK-056 chose plain row-driven `ATower`: `AoERadius > 0` (BombTower 250) makes the shot an AoE projectile; `MinRange > 0` (BallistaTower 300) reshapes acquisition into a blind-spot ring. Both come from DT_Cards cells, so no new class was needed. Matches the ArrowTower parenting.
- **Barracks → `ABarracks`** (TASK-057); **DeepMine → `ADeepMine`** (TASK-057). Both subclass `ABuilding`.

### Component addressing / how the recipe was applied
Each child CDO has exactly the single native root `VisualMesh` (from `ABuilding`) — no extra components. Per BP, three MCP `set_properties` targets:
- CDO (`…Default__BP_Building_<X>_C`): `CardID` = `<X>`. `Team` left at the inherited C++ default `Blue` (reads `Blue`) — the task `names:` block lists no Team; player placements set Team via `InitBuilding(Blue,…)` at spawn (TASK-030), and `ABuilding::ApplyTeamMaterial` recolors slot 0 to the actual team at BeginPlay (TASK-044), so a Red bot instance reuses the same BP with no Red duplicate.
- `…Default__BP_Building_<X>_C:VisualMesh`: `StaticMesh` = `SM_<X>`, `OverrideMaterials[0]` = `MI_TeamColor_Blue`, `bCanEverAffectNavigation` = `true`.

### Collision / navigation (§3.7) — how the requirement is met
- The **base `ABuilding` C++ constructor** (Building.cpp L36/44) already does `SetCollisionProfileName(BlockAll)` + `SetCanEverAffectNavigation(true)`, inherited by `ATower`/`ABarracks`/`ADeepMine` and thus by all 4 BPs.
- I **additionally pinned `bCanEverAffectNavigation=true` on each BP's VisualMesh CDO** (the TASK-035 Wall precedent) so the §3.7 nav-carve intent lives in the asset itself and survives any future base/template flip.
- Verified on the saved CDOs: `collisionProfileName=BlockAll`, `objectType=ECC_WorldStatic`, `collisionEnabled=QueryAndPhysics` on all three parent-chain branches (BombTower=ATower branch, Barracks=ABarracks branch, DeepMine=ADeepMine branch — BallistaTower shares the ATower branch). Units/hero physically stop, and reach-tests (hero melee / unit attacks / projectiles, all query ECC_Pawn) hit the buildings. **`bCanEverAffectNavigation=true` confirmed `true` on all 4** — walls/buildings carve the dynamic navmesh (§3.7).

## Compile
All 4 `compile_blueprint` calls ran with `warnings_as_errors: true` → no error raised (returnValue null). CDO/component overrides persisted (re-read post-compile; every value in the tables above verified on the disk-backed CDO).

## Runtime verification (Simulate-In-Editor, L_Arena)

Method (the TASK-035 pattern): placed one instance of each BP in editor L_Arena, `StartPIE(bSimulate=true)`, read the PIE-world (`UEDPIE_0_L_Arena`) instances via ObjectTools, `StopPIE`, removed the temp editor actors. **L_Arena left UNSAVED / content-identical** (every add was paired with a remove; discard if ever prompted).

Two SIE passes were run:

**Pass 1 (midfield placement)** — all 4 stats bound from DT_Cards exactly:

| Instance | CardID/Team | MaxHP | Damage / Range / Cadence | AoERadius / MinRange | Spawner triple | Income | Spec target | Result |
|----------|-------------|-------|--------------------------|----------------------|----------------|--------|-------------|--------|
| BombTower | BombTower / Blue | 180 | 25 / 800 / 2.5 | 250 / 0 | — | — | 180HP, 25dmg, 800, 2.5, AoE250 | **PASS** |
| BallistaTower | BallistaTower / Blue | 120 | 45 / 1400 / 3.0 | 0 / 300 | — | — | 120HP, 45dmg, 1400, 3.0, Min300 | **PASS** |
| Barracks | Barracks / Blue | 250 | — | — | Footman / 8s / 60s | — | 250HP + Footman/8/60 | **PASS** |
| DeepMine | DeepMine / Blue | 200 | — | — | — | DeepMineIncome 2 | 200HP, +2/s | **PASS** |

All 4 spawned without error. Note: in Pass 1 the buildings were Blue and placed at contested **midfield** in a live-simulating match (the world keeps ticking through the slow MCP reads, so a full match played out — a match-end freeze even fired). Enemy units correctly battered these destructible Blue buildings, so their `CurrentHP` read `0` by inspection time. This is expected combat behavior (buildings ARE destructible, §3.7), not a bind defect — `MaxHP` bound to the exact spec value proves `CurrentHP=MaxHP` was set at spawn (adjacent lines in `ABuilding::LoadStats`).

**Pass 2 (elevated, out-of-combat placement, Z=2000)** — clean healthy read to remove that ambiguity. Ground units can't reach a floating building and towers never target buildings (positive class gate), so no damage lands:

| Instance | MaxHP / CurrentHP | Result |
|----------|-------------------|--------|
| BombTower | 180 / 180 | **healthy** |
| BallistaTower | 120 / 120 | **healthy** |
| Barracks | 250 / 250 | **healthy** |
| DeepMine | 200 / 200 | **healthy** |

**Bonus signals observed in the log (LogGitClaudeUnrealTest):**
- Match-end freeze line reported `…1 barracks frozen, 2 tower(s) silenced…` during Pass 1 → my `ABarracks::FreezeAI()` was correctly caught by `FreezeWorldAtMatchEnd` (TASK-057 step 2b) and my two towers silenced by the ATower sweep. (Full spawn/self-destruct behavior still deferred — see below.)
- The `ASiegeBotController` referenced `BP_Building_BombTower/BallistaTower/Barracks_C` via the composed soft-class path (earlier "missing card class" warnings were from PRIOR sessions before these BPs existed; a DeepMine even got bot-played this session), confirming the CONVENTIONS composed paths resolve to these new BPs.

**Error scan:** a log query for `Error.*(BombTower|BallistaTower|Barracks|DeepMine|Building)` returned **empty** — zero errors. The only building warnings are the **known-benign `ABuilding … row 'DeepMine' has CardType 2 (Economy), expected Building`** warning, which TASK-057's handoff explicitly flagged (HP still binds regardless of CardType; suppression would require editing the frozen Building.cpp). No `Missing RowStruct` noise (TASK-061 closed the table, as noted).

## Deferred to TASK-069 (integrated PIE)
Full behavior needs a live match with the play/spawn routing (TASK-059) and enemy waves — per the task's explicit deferral:
- **Bomb Tower** AoE splash on a unit cluster every 2.5 s (AoERadius 250 carried on the projectile).
- **Ballista Tower** single-target at long range with the 300 blind spot (a target < 300 ignored, re-scan next cadence).
- **Barracks** spawns a Footman every 8 s and self-destructs at 60 s (this task saw only the timers arm + the freeze hook fire; 8 s/60 s are longer than the SIE window).
- **Deep Mine** raises the owner's rate +2/s on placement. `DeepMineIncome=2` bound and the mine registered via `ADeepMine::TryRegisterIncome` (the CardType warning per-mine confirms the class ran BeginPlay), but the actual gold-rate delta is observable only in a full match — TASK-069.

## Scope touched
Created `BP_Building_BombTower.uasset`, `BP_Building_BallistaTower.uasset`, `BP_Building_Barracks.uasset`, `BP_Building_DeepMine.uasset` (all saved, clean/not-dirty) + this handoff. No C++ compiled, no Git, no TASKBOARD edit, no donor modified (BP_Building_ArrowTower/Wall untouched), no L_Arena re-save. The 4 new `.uasset` are on disk (untracked / may be auto-staged by the UE Git provider) for TASK-069 to commit.

## Notes for QA / build-master
- Verify on a fresh editor load that each CDO still reads: parent (`ATower`/`ATower`/`ABarracks`/`ADeepMine`), CardID (BombTower/BallistaTower/Barracks/DeepMine), VisualMesh mesh `SM_<CardID>` + slot0 `MI_TeamColor_Blue`, `collisionProfileName=BlockAll`, `bCanEverAffectNavigation=true`.
- The DeepMine `CardType Economy expected Building` warning is EXPECTED and benign (TASK-057). The play/spawn routing that sends the Economy-typed DeepMine card down the building path is TASK-059's concern.
- `find_actors` by label omitted the Barracks from its result list in Pass 1 (a query quirk under a live-ticking match); the direct refPath `get_properties` returned the Barracks alive with correctly-bound stats, so the actor was present and healthy — noted only so QA doesn't read the find-list omission as a spawn failure.
- `MuzzleOffset` intentionally left at the C++ default on both towers (cosmetic; tune later if SM_BombTower/SM_BallistaTower heights warrant, as the ATower comment invites).
