# TASK-218 handoff — M7.6 Phase-0 integration: compile + 10× actor pass + nav rebuild + DA extents + PIE sanity + branch commit (build-master, 2026-07-19)

Branch: `m7.6-arena10x` (checked out from base `a33aba6`, zero-delta checkout; dirty M7.5 tree carried over untouched per TASK-214 lane law). Editor was closed AND relaunched under Jonathan's explicit session grant (relayed 2026-07-19: "I give you permission to close and reopen the editor as needed"). Graceful close both times conceptually; the one actual close used CloseMainWindow (clean exit, no modal, no force-kill). **Editor left RUNNING with L_Arena loaded — W1 is one click.**

## 1) Compile
Clean: `Build.bat GitClaudeUnrealTestEditor Win64 Development` → Succeeded in 20.3 s (9 actions; ScatterConfig.cpp, BattlefieldScatter.cpp, SiegeBotController.cpp + module/link). No Smart-App-Control interference.

## 2) Actor pass (plan §1b — all values verified by post-set readback)
| Actor (label) | Old | New (readback) |
|---|---|---|
| Castle_Blue / Castle_Red | X ∓8,000 | **X ∓25,000** (Red keeps yaw 180) |
| CastleAnchor_Blue/_Red (TargetPoints) | X ∓8,000 | **X ∓25,000** |
| GoldNode_Blue/_Red | X ∓7,200 | **X ∓24,200** |
| PlayerStart | (−6,800,0,100) | **(−23,800,0,100)** |
| ArenaGround | scale (180,80,1) | **(560,250,1)**, loc Z −50 kept → bounds X ±28,000 Y ±12,500, **top Z=0 exact** |
| ArenaBoundary_East/West | X ±8,800, Y-scale 80 | **X ±27,500, Y-scale 250** |
| ArenaBoundary_North/South | Y ±4,000, X-scale 180 | **Y ±12,500, X-scale 560** |
| NavMeshBounds_Arena | scale (90,40,12) | **(280,125,12)** |
| SkyDome_Gradient | scale 900 | **3,000** |
| TeamFill_Cool_Blue / _Warm_Red | X ∓4,000, atten 6,500 | **X ∓12,500, AttenuationRadius 20,000** |
| ExponentialHeightFog | 0.0436 / 0 / 1.0 | **FogDensity 0.008 / StartDistance 10,000 / FogMaxOpacity 0.85** |
| KillZ / PP volume / hero camera / CenterlineMarker | — | untouched (keep-list) |

Ground material tiling: visually world-aligned (no ×3.125 stretch; per-world-unit repeat unchanged). The small-period repetition IS visibly obvious across the huge field — polish-task note, NOT fixed here (per spec). Also noted: the TeamFill rect footprint reads as a pale patch on the ground — W1 feel item (fallback lever in plan: delete + tint ground by X-sign).

## 3) Recast actor mirror + navigation rebuild (the drama — READ THIS)
- Actor params verified == TASK-217 ini table: TileSizeUU 2000, NavMeshResolutionParams[1]=(32,20,35), bFixedTilePoolSize True, TilePoolSize 1024, bDoFullyAsyncNavDataGathering True, RuntimeGeneration Dynamic. (The serialized actor carried no config-delta, so the new ini flowed onto it at editor boot — mirror check = exact match; ini/actor law satisfied.)
- Boot log confirmed the stale bake died: "Navmesh RecastNavMesh-Default will be loaded empty… built with different navmesh settings" (old quantization 0.052632/cell 19, old tile 988 vs 1984).
- **GOTCHA (memory-worthy):** MCP `set_actor_transform` on the NavMeshBoundsVolume does NOT fire PostEditChangeProperty → `UNavigationSystemV1::OnNavigationBoundsUpdated` never ran → nav bounds stayed registered at the OLD footprint (navmesh bounds read ±9,920×±3,968 = old arena tile-rounded). First two PIE runs: Blue→Red path NEVER found, 5-cull force-clear Error, bot 100% paralyzed (spawn projection at X≈23,250 was outside the navmesh).
- **Fix sequence that works (repeatable):** (1) toggle a hook property on the volume — `SupportedAgents.bSupportsAgent30` false→true — fires OnNavigationBoundsUpdated (ANavMeshBoundsVolume::PostEditChangeProperty hooks BrushBuilder/SupportedAgents/RelativeLocation-Rotation-Scale3D); (2) re-set `TilePoolSize=1024` on the RecastNavMesh actor — Generation-category PostEditChange fires `RebuildAll()` (verified in 5.8 source; do NOT nudge TileSizeUU — it re-snaps CellSize and would desync the ini); (3) editor was background-throttled — temporarily set `bThrottleCPUWhenNotForeground=false` so the async build completes in ~1–2 min (RESTORED to default true afterwards). Do NOT `time.sleep` inside MCP tool-scripts — they run on the game thread and freeze the build you're waiting for.
- Result: navmesh bounds ±29,760×±13,888 (full volume, tile-rounded), saved into L_Arena — umap grew 122,321 → 298,603 bytes (~176 KB of serialized tiles). Build > Navigation owed-item satisfied without Jonathan's click.

## 4) DA_BattlefieldScatter
ArenaHalfExtent **(26,000, 12,000)**, CorridorHalfWidth **1,000**, keep-clears **1,500/600/800** (all set + readback-verified; keep-clears already inherited the new C++ defaults, now pinned). DENSITIES/COUNTS UNTOUCHED (Phase-3 owns those). Saved. Save discipline: only L_Arena + DA_BattlefieldScatter ever saved (explicit save_assets lists; never save-all).

## 5) PIE machine checks (post-fix runs; two fresh boots = Play-Again ×2 machine proxy)
- Scatter: `GenerateScatter … corridorHalfY=1000`, all 7 layers at OLD densities (70/60/6/6/8/2500/400) — correct for Phase 0.
- Nav poll (TASK-216 code) observed live: pre-fix runs showed the 10 s MaxNavSettleWait cap warnings (poll + cap paths work); post-fix runs settle fast.
- **Reachability: "Traversability CONFIRMED — Blue→Red castle path exists (after 0 cull(s))" in BOTH post-fix boots** (~8 s after scatter; seeds 924020289, 866608961). Zero force-clears post-fix.
- **Bot (ruling #1 live):** attack waves spawn castle-front at **X=23,250** (25,000 − offset 1,750), lane spread within ±900, log "marching (M7.6 ruling #1)" — Ogre/Cleric/Pikeman/Cavalry/Sapper/MilitiaMob/Archer/Knight/Longbowman across matches. Miner: "played Miner toward GoldNode_Red at (23,800,0,20)" — castle-relative economy intact.
- **Full A→B march PROVEN:** in the first post-fix match the bot's waves crossed the entire ~46,750 uu field unopposed, hit the Blue castle (M_HitFlash event 07:30:16) and DESTROYED it (S_CastleDestroyed + S_DefeatMusic 07:31:26). MapCheck 0 errors / 0 warnings; no load errors; only known M7 placeholder no-ops (S_OvertimeSting/M_HitFlash/NS_GoldBurst/S_CastleDestroyed/S_DefeatMusic unresolved — TASK-174/180 land them) and pre-existing material warnings (below).
- True in-game "Play Again" button flow not machine-clickable — on Jonathan's W1 list below.

## 6) Branch commit
Explicit pathspecs only: TASK-216's five Source/ files, Config/DefaultEngine.ini, Content/Maps/L_Arena.umap, Content/Data/DA_BattlefieldScatter.uasset, handoffs/TASK-214/216/217/218.md, qa/TASK-216-217-220-qa.md. Commit hash: see board line / Slack. **Judgment calls, stated:** (a) the QA report is included because the commit cites it; its TASK-220 section is frozen loop-1 history (220 continues on MAIN; trivial doc-merge at Phase-6). (b) TASKBOARD.md + CONVENTIONS.md are EXCLUDED — their churn is predominantly M7.5-lane (TASK-210's consolidated main commit owns it); reconcile at the Phase-6 merge. (c) NO M7.5 files, NO reimport_meshes.py (main-lane TASK-220, mid-QA-loop), NO TASK-220 handoff. Not pushed.

## W1 checklist for Jonathan (TASK-219 — editor is running, L_Arena loaded, just press Play)
1. **FPS at field positions:** blue castle, mid-field (~X 0), red castle, and a corner (~X 20k / Y 10k). First real numbers ever — note them (GDD bar: 60fps@1440p on the 3060 class).
2. **March feel:** summon a unit, watch the full A→B march. MEASURED REALITY: bot units cross castle-front→castle in ~2–2.5 min (speeds ~350–400), NOT the plan's ~9-min first-contact estimate — the bot opens with an attack wave within seconds (starting gold covers it). In the machine run an UNDEFENDED castle fell ~2m40s in. Rule on pacing: too fast? too slow? conga-line feel of Barracks trickle?
3. **Bot behavior:** waves spawn AT the red castle and march (no mid-field materialize), miners work the node, defense feels sane.
4. **Play Again** (the actual button) ×3 — machine side verified 2 fresh-boot cycles clean; the button flow is yours.
5. **Miner loop:** your own miner to the blue node — walk, mine, payout.
6. **Nav feel:** spawn latency after placing buildings (coarser 32/20 cells + 2000 tiles), units cutting corners or hugging odd edges.
7. **Visual notes:** fog density 0.008 at distance (cull-pop hider), TeamFill light footprint patch on the ground (delete/tint fallback exists), ground-texture repetition at scale (polish task candidate), skydome at r150k.

## Follow-ups found (for manager — report only, not fixed here)
- Ground material periodic tiling reads strongly at 10× scale → polish-task candidate (macro variation/detail blend).
- TeamFill RectLight ground footprint visible → W1 judgment; fallback lever already in plan.
- Materials missing InstancedStaticMeshes usage flag (recompile every editor launch + default-material fallback in game for scatter): `/Game/Tree_Pack_1/.../M_Pack1_Trunk_Mobile`, `M_Pack1_Leaf_Mobile`, `/Game/Materials/Instances/MI_BattlefieldGround` → tiny resave task (pre-existing, surfaced by scatter ISM usage).
- Match pacing vs plan: first contact ≈ 2.5 min (not ~9) because bot starting gold funds an instant wave; W1 ruling drives whether that's a knob task (e.g. bot opening-wave delay).
- MCP tooling gotcha (orchestrator memory candidate): `set_actor_transform` bypasses PostEditChange — any nav-relevant volume moved via MCP needs the SupportedAgents-toggle + RebuildAll-nudge + throttle-off sequence documented in §3.
