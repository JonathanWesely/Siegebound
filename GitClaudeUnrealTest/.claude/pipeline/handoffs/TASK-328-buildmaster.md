# TASK-328 handoff — [FACE-int] facing-fix integration (build-master)

**Date:** 2026-07-27 · **Verdict: VERIFIED — systemic facing fix live in real PIE, 12/12 units at (0,−90,0), no regression, logs clean.**
**Code commit:** the TASK-327 code (QA PASS, `qa/TASK-327.md`) was ALREADY on main in Jonathan's own commit `7bedf58` ("mesh color fixes") — per the Jonathan-self-commits rule it was NOT re-committed or amended. This commit carries board flips + this handoff + verify shots only.

## Compile
- Editor-close gate: Jonathan closed the editor himself (~13:43); no force-kill, no MCP-driven quit.
- `Build.bat GitClaudeUnrealTestEditor Win64 Development` → **Succeeded, 14.79 s** (module recompiled + relinked, `UnrealEditor-GitClaudeUnrealTest.dll` 13:44:06). Zero warnings in the build output.
- Editor relaunched detached; MCP answered on :8000.

## Real-PIE verification on /Game/Maps/L_Arena (NOT Simulate; live match with the bot playing throughout)
Lane: `USiegeCheatManager::SummonTestUnit <CardID> <bRed>` (shipping `SpawnUnitSwarm` path) + UE Python remote-exec readbacks/captures (TASK-221 recipe). PIE cycles: ONE session for everything (no stop/start churn).

### (a) Numeric readback — the load-bearing evidence
All **16** live `ASummonedUnit`s (my 12 Blue + 4 bot-played Red) read at runtime, post-swap:
- `SkeletalVisualMesh.RelativeRotation == (0, −90, 0)` on **every unit** — including **Archer, Ogre, Wizard** (formerly 0). ABSOLUTE overwrite confirmed working; no unit at −180 (additive failure mode absent).
- `SkeletalVisualYawOffset == −90.0` on every instance (new UPROPERTY at its default).
- Grounding untouched: `RelativeLocation.Z` ≈ −(HalfHeight+MeshMinZ) per unit (Ogre −144.79 / Cavalry −103.98 / Wizard −88.07 / MilitiaMob −74.51 …). Capsule half-heights READ LIVE via `GetScaledCapsuleHalfHeight()` (ruling 4): Ogre 145 / Cavalry 104 / Knight+Pikeman 95 / Longbowman 92 / Cleric 91 / Footman+Archer 90 / Wizard 88 / Miner 86.5 / Sapper 84.5 / MilitiaMob 74.5 — matches TASK-326 §1 exactly.
- Attack-state: staged Blue-vs-Red duels — each duelist's ACTOR yaw pointed at its target within ~2° (e.g. Blue Archer yaw 0.66 vs target bearing 0.6°; Red Wizard −178.15 reciprocal), mesh relative constant at −90.

### (b) Visual observations (shots in this folder; "before" = the committed TASK-326 evidence PNGs)
- `TASK-328-verify-march-A.png` — all 12 units line-abreast marching INTO the camera: every unit presents its FRONT (Footman/Knight shields forward, Cavalry horse head-on, Ogre square, Wizard hat/front visible). **Archer, Ogre, Wizard march exactly like the 9 controls. Nobody walks sideways.**
- `TASK-328-verify-march-side-C.png` — Ogre + Wizard (the two slowest, both fixed units) in true profile, bodies aligned with travel direction.
- `TASK-328-verify-attack-A.png` — duels: casters mid-cast with fire-tipped staves POINTED AT their targets; Red Archer advancing facing its target; Blue Ogre charging toward the wall it targets.
- `TASK-328-verify-attack-ranged.png` — round-1 ranged duel mid-fight (fireballs in flight), both teams engaging along the target axis.
- `TASK-328-verify-death-ogres.png`, `TASK-328-verify-death-ranged.png` — six controlled kills (Ogre/Archer/Wizard × both teams): all collapse in place, grounded, sane orientation; no spin/float/slide.
- Regression gate: the 9 controls are UNCHANGED — identical (0,−90,0) numerics they were authored with, and normal front-facing march in march-A. TASK-307 grounding unchanged (Z column). Wizard fireball spawn/flight normal (QA note 5).
- Death also occurred organically in real play (ranged duelists killed each other; the first Blue wave died to Red castle defenses) — no anomalies seen.

### (c) Logs
`ensure` / `Accessed None` / `Fatal` = **0** across the whole session (pattern scan over the full log). Only game-module warnings: pre-existing TASK-015-class `MoveToActor ... failed` nav warnings, produced by MY harness teleporting units to off-lane spots (unreachable nav targets) — not a facing/TASK-327 regression, not new.

## Deviations / notes
- One early Build.bat attempt fired on a false "editor down" signal and was refused by UBT's Live Coding guard (2.7 s, harmless, nothing touched) — recorded in 🔧 Build & Git.
- First capture round was shot from a mis-set view target (distant); superseded by the kept set above. march-front/march-side/attack-1/attack-duels/death-red/death-blue in Saved/Screenshots were NOT copied into handoffs (distant framing), available locally if wanted.
- Aggro observation (report-only, manager's domain): melee units at staged off-lane spots sometimes bypassed each other for structures (Ogres pathing to walls across the map) and idled when nav failed; ranged units engaged unit-vs-unit reliably. Pre-existing behavior surface, unrelated to facing.
- `L_Arena` NOT saved. Boot-resave dirty .uassets left uncommitted per standing law.

## QA WARNs honored (`qa/TASK-327.md`)
- WARN-1 (static-fallback path uncovered): closed by **TASK-334** (this session, own commit).
- WARN-2 (EditDefaultsOnly hatch could be silently re-authored to 0): CONVENTIONS "Unit mesh facing" clause is live (manager, ratified 2026-07-27); the suggested one-time `UE_LOG` guard is left for a future touch of `ResolveSkeletalVisual` — carried, not lost.
