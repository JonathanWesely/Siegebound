# TASK-034 Handoff — BP_Unit_Archer / BP_Unit_Knight / BP_Unit_Miner (editor/MCP)

- author: gameplay-programmer
- date: 2026-07-04
- status: complete — 3 BPs created, compiled clean (warnings-as-errors), saved; spawn+stat verified in Simulate. No Git (TASK-040 commits). TASKBOARD.md NOT edited (reported to orchestrator).
- editor: left UP (fresh PID 35508, aafd968 DLL). MCP stable throughout — no hiccups on BP/graph/scene/PIE ops.

## What was built

Three data-only child BPs in `/Game/Blueprints/Units/`, each mirroring the BP_Unit_Footman recipe (handoffs/TASK-010.md). Component addressing on each CDO: `:VisualMesh` (StaticMeshComponent) + `:CollisionCylinder` (the inherited ACharacter capsule; exposed as CapsuleComponent/RootComponent). Recipe applied exactly like Footman: mesh dropped to feet (VisualMesh RelativeLocation Z = -HalfHeight) and yaw -90 import fix (TASK-014/TASK-037: meshes model facing -Y, import +Y → -90 yaw faces actor +X).

| BP | Parent (C++) | CardID | VisualMesh | Slot 0 material | Capsule HH / R | VisualMesh RelLoc / yaw |
|----|--------------|--------|-----------|-----------------|----------------|-------------------------|
| BP_Unit_Archer | ASummonedUnit | Archer | /Game/Meshes/SM_Archer | MI_TeamColor_Blue | 90 / 40 | (0,0,-90) / -90 |
| BP_Unit_Knight | ASummonedUnit | Knight | /Game/Meshes/SM_Knight | MI_TeamColor_Blue | 95 / 45 | (0,0,-95) / -90 |
| BP_Unit_Miner | AMinerUnit | Miner | /Game/Meshes/SM_Miner | MI_TeamColor_Blue | 86.5 / 40 | (0,0,-86.5) / -90 |

- **Team** = Blue on every CDO (explicit, matches C++ default and the Footman precedent).
- **Nothing stat-like set** on any BP — HP/Damage/Range/Cadence/Speed all bind from DT_Cards at BeginPlay (verified below). On the editor CDO the Transient stat fields read 0 as designed.
- **Capsule sizing rationale** (spec: "capsule sized to the mesh"): HalfHeight = mesh height / 2 from TASK-037 handoff dims (Archer 180, Knight 190, Miner 173) so feet sit at capsule bottom and head at capsule top; VisualMesh Z = -HalfHeight. Radius = Footman house value 40 (torso-only; weapon overhang deliberately spills past the capsule, same convention as Footman's 147-wide mesh in a radius-40 capsule), bumped to 45 for the Knight (bulkiest torso/pauldrons, widest mesh at 162 X). Not tied to weapon-inclusive mesh X extents on purpose.
- Assets saved to disk (`Content/Blueprints/Units/BP_Unit_{Archer,Knight,Miner}.uasset`), all three confirmed **not dirty** after save. Targeted save only — donors and L_Arena were not re-saved.

## Compile

All three compiled with `warnings_as_errors: true` — no errors, no warnings raised. CDO/component overrides all persisted through compile (re-read after compile; every value above verified on disk-backed CDO).

## Spawn + stat verification (Simulate-In-Editor, L_Arena)

Method = TASK-010 precedent: placed one instance of each BP in the editor L_Arena (spread on Y at x=0, snap-to-ground), StartPIE `bSimulate=true` warmup 2.5 s (BeginPlay + LoadStatsAndStart run), read the PIE-world (`UEDPIE_0_L_Arena`) instances, StopPIE, removed the three temp editor actors. **L_Arena was left UNSAVED / content-identical** (add+remove pair; if the editor ever prompts to save L_Arena, discarding is correct).

Runtime values, all resolved from `/Game/Data/DT_Cards` (nothing hardcoded):

| Unit | CardID/Team | MaxHP/CurrentHP | MaxWalkSpeed | Damage/Range/Cadence | bRanged | State | Spec target | Result |
|------|-------------|-----------------|--------------|----------------------|---------|-------|-------------|--------|
| Archer | Archer / Blue | 45 / 45 | **350** | 10 / 700 / 1.2 | true | Attack | 45 HP / 350 | PASS |
| Knight | Knight / Blue | 200 / 200 | **300** | 15 / 120 / 1.2 | false | Attack | 200 HP / 300 | PASS |
| Miner | Miner / Blue | 30 / 30 | **350** | 0 / 0 / 0.05* | — | Advance | 30 HP / 350 | PASS |

*Miner Cadence reads 0.05 = the AMinerUnit clamp of the row's Cadence 0 (WARN-1 guard). Miner State = **Advance** (never Attack) — the no-attack-path seal (StateCheckInterval 0 / AggroRadius 0) holds; it acquired no target. Miner bound stats cleanly despite NO GoldNode_Blue in the level yet (nodes come from TASK-036) — it logged/idled the missing-node path and did not crash.

**No friendly targeting (bonus check):** Archer and Knight both acquired `Castle_1`, which I confirmed is **Team=Red** (enemy). Blue units correctly targeted an enemy castle; no same-team acquisition. State machine is live (both reached Attack against the Red castle, which sits within range of origin in the current L_Arena layout).

**Mesh facing (-90 yaw):** confirmed on all three CDOs (RelativeRotation yaw -90) — the standard SM_Footman-family import fix. Visual viewport confirmation of blue tint / feet-on-floor was NOT separately captured this run; the yaw/material/Z values are identical in structure to the PIE-verified Footman, and the mesh-import facing convention is the same TASK-037 export orientation.

## Deferred to TASK-040 (full integrated PIE)

The board's full acceptance for TASK-034 needs actors that do not exist until TASK-036 runs:
- **Archer** engaging from 700 with visible homing projectiles at 1.2 s cadence, and **Knight** meleeing for 15 at 1.2 s, and both dying correctly / never hitting friendlies — needs proper enemy waves and clean spacing (partially demonstrated here: both reached Attack vs a Red castle, but projectile/melee-hit/death was not driven to completion).
- **Miner** walking to `GoldNode_Blue` and raising the gold rate on arrival — needs **GoldNode_Blue** (TASK-036). Verified here only that the Miner binds 30 HP / 350 speed, stays out of Attack, and survives the no-node case.

These belong to the TASK-040 integrated combat/economy PIE per the task's verification boundary. Everything verifiable on this fresh editor (compile, reparent, spawn-without-error, DT_Cards stats, -90 yaw) is PASS.

## Scope touched

Created `BP_Unit_Archer`, `BP_Unit_Knight`, `BP_Unit_Miner` under `/Game/Blueprints/Units/` and this handoff. No C++ compiled, no Git, no TASKBOARD edit, no donor/L_Arena re-save. The 3 new `.uasset` are on disk (untracked / possibly auto-staged by the UE Git provider) for TASK-040 to commit.

## Notes for QA

- `Missing RowStruct` benign log (if seen) is closed per the task brief — rows verified live (all 6 stat rows read correctly at runtime).
- The DisplayName/Cost/MaxCopies/DeckCount columns in DT_Cards are not unit-BP concerns; the unit BPs only consume HP/Damage/Range/Cadence/Speed/bRanged, all confirmed.
- Radius 45 on Knight is the only deviation from Footman's radius 40 — justified by the bulkier tank silhouette; flag if the convention should be uniform 40.
