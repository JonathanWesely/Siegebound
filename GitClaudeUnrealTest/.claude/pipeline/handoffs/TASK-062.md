# TASK-062 Handoff — BP_Unit_* for the 7 Set II units (editor/MCP)

- author: gameplay-programmer
- date: 2026-07-04 (M4 editor-wave task 2/4; serial editor access)
- status: ready-for-qa — 7 BPs created, reparented to ASummonedUnit, CDO+component overrides set, compiled clean (warnings-as-errors), saved; per-unit DT_Cards stat verification passed in Simulate. No Git, no compile, no TASKBOARD edit.
- editor: left UP (PID 6172, 65861ce DLL). MCP stable throughout — no BP/graph/object/scene/PIE hiccups. Did NOT boot/close/bounce it. L_Arena left UNSAVED and content-identical (temp verify actors added then removed).

## What was built

Seven data-only child BPs in `/Game/Blueprints/Units/`, each cloning the BP_Unit_Footman/BP_Unit_Archer recipe (handoffs/TASK-010.md, TASK-034.md). Recipe verified against the live BP_Unit_Footman CDO before applying: component addressing on each CDO is `:VisualMesh` (inherited UStaticMeshComponent) + `:CollisionCylinder` (the inherited ACharacter capsule). Per unit: CardID = suffix, Team = Blue (explicit CDO stamp, matches C++ default + Footman precedent), VisualMesh StaticMesh + OverrideMaterials[0] = `/Game/Materials/Instances/MI_TeamColor_Blue`, VisualMesh RelativeLocation Z = -HalfHeight (feet-center mesh dropped to capsule bottom), VisualMesh RelativeRotation yaw = **-90** (the TASK-014 import-fix: meshes model facing -Y, import +Y → -90 yaw faces actor +X), capsule sized to the art-handoff mesh height.

| BP | Parent (C++) | CardID | VisualMesh | Capsule HH / R | VisualMesh RelLoc Z / yaw | Mesh height (art) |
|----|--------------|--------|-----------|----------------|---------------------------|-------------------|
| BP_Unit_MilitiaMob | ASummonedUnit | MilitiaMob | /Game/Meshes/**SM_MilitiaMob** | 74.5 / 30 | -74.5 / -90 | 149 (TASK-065) |
| BP_Unit_Pikeman    | ASummonedUnit | Pikeman    | /Game/Meshes/SM_Pikeman        | 95 / 40   | -95 / -90   | 190 (TASK-065) |
| BP_Unit_Sapper     | ASummonedUnit | Sapper     | /Game/Meshes/SM_Sapper         | 84.5 / 40 | -84.5 / -90 | 169 (TASK-065) |
| BP_Unit_Cavalry    | ASummonedUnit | Cavalry    | /Game/Meshes/SM_Cavalry        | 104 / 45  | -104 / -90  | 208 (TASK-065) |
| BP_Unit_Longbowman | ASummonedUnit | Longbowman | /Game/Meshes/**SM_Longbowman** | 92 / 40   | -92 / -90   | 184 (TASK-066) |
| BP_Unit_Cleric     | ASummonedUnit | Cleric     | /Game/Meshes/SM_Cleric         | 91 / 40   | -91 / -90   | 182 (TASK-066) |
| BP_Unit_Ogre       | ASummonedUnit | Ogre       | /Game/Meshes/SM_Ogre           | 145 / 60  | -145 / -90  | 290 (TASK-066) |

- **All 7 parent ASummonedUnit** (per the task spec + names). Siege/Support/keyword behavior is 100% data-driven inside ASummonedUnit (Profile + bCharge/bSlayer/bSuicide/bRanged columns bound at BeginPlay, TASK-054/055) — NO subclasses for these 7 (unlike AMinerUnit).
- **Custom meshes wired, NOT the fallbacks:** SM_MilitiaMob (TASK-065 modeled a distinct peasant, not reused SM_Footman) and SM_Longbowman (TASK-066 modeled a distinct longbow archer, not reused SM_Archer). Both confirmed present with the MI_TeamColor_Blue slot before wiring; both confirmed on the CDOs after compile.
- **Nothing stat-like on any BP** — HP/Speed/Damage/Range/Cadence/Profile/keyword flags are all Transient and read 0/None on the editor CDO; every value binds from DT_Cards at BeginPlay (verified in Simulate below). No SwarmCount/AoERadius/etc. authored on the BP.
- **Capsule HalfHeight = mesh height / 2** (feet at capsule bottom, head at top; VisualMesh Z = -HalfHeight — the exact Footman/Archer/Knight/Miner convention). Radius = torso-only per the family convention (weapon/mount overhang deliberately spills past the capsule, same as Footman's 147-wide mesh in a radius-40 capsule): 40 house value for the normal soldiers (Pikeman/Sapper/Longbowman/Cleric); 30 for MilitiaMob (distinctly small/thin swarm unit — thin 34.3u depth, and 4 spawn per card so a tighter footprint avoids swarm overlap); 45 for Cavalry (heavy mounted, Knight-bulky tier); 60 for Ogre (roster's bulkiest brute — sized to the torso, NOT the 226.7u club-inclusive extent per the TASK-066 note). Not tied to weapon/mount-inclusive X/Y extents on purpose.
- Assets saved to disk (`Content/Blueprints/Units/BP_Unit_{MilitiaMob,Pikeman,Sapper,Cavalry,Longbowman,Cleric,Ogre}.uasset`); targeted `save_assets` only; all 7 confirmed not-dirty. Donors + L_Arena not re-saved.

## Compile

All 7 compiled via BlueprintTools.compile_blueprint with `warnings_as_errors: true` — no exception raised on any (the tool raises on error/warning). Every CDO + component override persisted through compile (re-read on the disk-backed CDO after compile; all values in the table above verified post-compile).

## Spawn + stat verification (Simulate-In-Editor, L_Arena)

Method = TASK-034 precedent: placed one instance of each BP in editor L_Arena (x=-900, spread on Y, snap-to-ground), ran Simulate (`bSimulate=true`), read the PIE-world (`UEDPIE_0_L_Arena`) instances, StopPIE, removed the 7 temp editor actors. **L_Arena left UNSAVED / content-identical** (add+remove pair; if the editor ever prompts to save L_Arena, discarding is correct).

Runtime values, all resolved from `/Game/Data/DT_Cards` (nothing hardcoded — the editor CDOs read 0/None for these Transient fields):

| Unit | CardID/Team | MaxHP | MaxWalkSpeed | Dmg/Range/Cadence | Profile | Keyword flags (bound) | Spec target | Result |
|------|-------------|-------|--------------|-------------------|---------|-----------------------|-------------|--------|
| MilitiaMob | MilitiaMob / Blue | **25** | **400** | 6 / 120 / 1.0 | Standard | (none; SwarmCount is play-time, not a unit field) | 25HP/400 | PASS |
| Pikeman    | Pikeman / Blue    | **100** | **350** | 30 / 120 / 1.5 | Standard | **bSlayer=true** | 100HP/350 | PASS |
| Sapper     | Sapper / Blue     | **60**  | **500** | 80 / 120 / 1.0 | **Siege** | **bSuicide=true, AoERadius=250** | 60HP/500 Siege | PASS |
| Cavalry    | Cavalry / Blue    | **140** | **600** | 20 / 120 / 1.0 | Standard | **bCharge=true** | 140HP/600 | PASS |
| Longbowman | Longbowman / Blue | **70**  | **300** | 18 / **1200** / 1.5 | Standard | **bRangedAttack=true** | 70HP/300 ranged | PASS |
| Cleric     | Cleric / Blue     | **90**  | **350** | 8 (heal rate) / 400 / 1.0 | **Support** | (none) | 90HP/350 Support | PASS |
| Ogre       | Ogre / Blue       | **500** | **250** | 35 / 120 / 1.5 | **Siege** | (none) | 500HP/250 Siege | PASS |

Every HP/speed/damage/range/cadence/Profile/keyword value matches `Docs/Data/cards.csv` (the 22-row Set II table reimported by TASK-061) character-for-character. Profiles bind correctly (Siege = Ogre/Sapper, Support = Cleric, Standard = the four combat units). Keyword columns bind correctly (bCharge Cavalry, bSlayer Pikeman, bSuicide+AoE250 Sapper, bRanged Longbowman).

**Mesh facing (-90 yaw):** confirmed on all 7 CDOs post-compile (RelativeRotation yaw -90) — the standard SM_Footman-family import fix; structurally identical to the PIE-verified Footman.

### Verification-run notes (for QA — nothing to action)
- **The Simulate world runs a LIVE match** (SiegeGameMode + the M4 bot auto-run with no one defending Blue), so it spawned Red waves and stomped fast. On the first 3-second warmup pass the fragile Blue verify units (MilitiaMob 25HP, Cleric 90HP) were killed and the Sapper suicide-detonated before I could read them. I re-ran with `warmupSeconds=0` and a tight read so BeginPlay stat-binding was captured before combat reached them — that gave the clean full table above. The Sapper's MaxWalkSpeed specifically needed one extra warmup-0 pass (it detonates near-instantly as a Siege+suicide unit); 500 confirmed. This is a verification-harness artifact, NOT a BP defect — stat-binding is synchronous at BeginPlay and independent of the match; every value read equals the DT_Cards row.
- All units read State=Idle/Advance in the noisy sim (fast bot stomp → likely a mid/late match-end freeze). Targeting/combat behavior is NOT the acceptance boundary here (see deferral below).

## Deferred to TASK-069 (integrated PIE)

The board's FULL TASK-062 acceptance needs enemies/waves + a fair match that don't exist in a single-side Simulate:
- **Siege targeting** — Ogre/Sapper ignoring units/hero and pathing to buildings→castle (Profile=Siege confirmed here; the walk-the-lane behavior needs a real match).
- **Cleric heal** — following + healing a *damaged* friendly and never attacking (Profile=Support confirmed; a lone Cleric with no damaged friendly just idles).
- **Charge / Slayer 2×** — Cavalry's primed-after-2s-move ×2 and Pikeman's ×2 vs MaxHP≥150 (flags bound; the damage-multiplier proof needs the right target types in combat).
- **Militia Mob → 4 units** — SwarmCount 4 is a play-time spawn behavior (TASK-059 controller path), not a per-unit BP field; the BP is a single unit and correctly carries no SwarmCount.
- **No-friendly-fire across the roster.**

These belong to TASK-069's expanded-roster integrated PIE per this task's verification boundary. Everything verifiable on this editor (create, reparent to ASummonedUnit, spawn-without-error, DT_Cards stats incl. Profile + keyword flags, -90 yaw, custom-mesh wiring) is PASS.

## Constraints honored
- No Git, no compile, no TASKBOARD edit (orchestrator owns the board). Targeted saves only; donors/L_Arena not re-saved (L_Arena left unsaved + content-identical). Editor left UP with MCP reachable for TASK-063/064/069. Donors not modified. The 7 new `.uasset` are on disk (untracked / possibly auto-staged by the UE Git provider) for TASK-069 to commit.
- Did NOT chase any benign `Missing RowStruct` log (TASK-061 closed the 22-row table; none seen this run).

## Notes for QA
- Radius deviations from the uniform Footman-40: MilitiaMob 30, Cavalry 45, Ogre 60 — all justified above (small swarm / heavy mount / bulk brute), same rationale family TASK-034 used for Knight's 45. Flag if the convention should be forced uniform 40 (gameplay-neutral: the capsule is torso collision, weapon/mount overhang already spills by design).
- Cleric's row `Damage` 8 is the heal RATE (HP/s), consumed by the Support heal path — it is not an attack; Cleric never enters Attack (Profile=Support).
- MilitiaMob and Longbowman VisualMesh point at the CUSTOM SM_MilitiaMob / SM_Longbowman (confirmed on the CDOs), per the TASK-065/066 handoffs — NOT SM_Footman / SM_Archer.
