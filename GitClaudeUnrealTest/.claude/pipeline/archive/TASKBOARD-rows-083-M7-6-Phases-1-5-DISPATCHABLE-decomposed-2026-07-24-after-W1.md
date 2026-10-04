<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-284 — [P1] Scatter cull bands + grid-hash MinSpacing accelerator (C++, branch)
- assignee: gameplay-programmer
- status: done (INTEGRATED at TASK-286 batch, build-master 2026-07-24, commit on m7.6-arena10x — compile GREEN; PIE runtime proof: clean GenerateScatter (seed 774069313), all 7 layers placed EXACT target counts (Boulders 6 / Hill 8 / Slabs 8 / Trees 70 / Rocks 60 / Grass 2500 / Plants 400) + 6 mines / 0 culls + Traversability CONFIRMED ⇒ grid-hash accelerator equivalence holds at runtime, no count-change; unpopulated-DA cull default = full render (today's look). L_Arena/DA/BattlefieldScatter.h verified UNTOUCHED. --- Prior QA 2026-07-24 PASS, 0 blockers / 1 non-blocking NIT [negative-scale, out of scope], `qa/TASK-284.md`. Grid-hash equivalence SOUND — cell size MinSpacing+2×MaxLayerR provably covers max rejection dist ⇒ 3×3 block catches every conflict; same inequality, 0 FRandomStream draws, Add 1:1 incl. mirror twin, keep-clear O(1) untouched ⇒ byte-identical layout per fixed seed. Cull fields (CullStart/End int32, bCastShadows bool default true) added to FScatterLayer, applied at BOTH HISM sites pre-Register; unpopulated DA byte-identical to today. Proxy SetCastShadow(false)-hardcoded ACCEPTED (invisible-trunk floating-shadow guard). Runtime fixed-seed layout diff = TASK-286 PIE proof. Pairs w/ TASK-285 for TASK-286 build.)
- blocked-by: none (W1 sign-off cleared the phase gate; owns BattlefieldScatter/ScatterConfig on the branch)
- parallel-safe: yes (disjoint from TASK-285's `SummonedUnit`; disjoint from TASK-282's `SummonedUnit`; TASK-288's Tools/ file)
- spec: >
    On `m7.6-arena10x`. Two perf structures that make the Phase-3 density fill (≈4.9× instances) affordable.
    (1) **Cull bands.** IMPLEMENT the CONVENTIONS-declared `FScatterLayer` fields `CullStartDistance` / `CullEndDistance`
    (uu; 0 = never culled) + `bCastShadows` — first CONFIRM whether the W1-PREP `OverrideMaterial` pass (TASK-249) already
    added any of them and add only what is missing. APPLY them at component build via `SetCullDistances` / `SetCastShadow`
    in `ResolveComponentForMesh()` AND the tree collision-proxy path (both places a HISM/component is created). Per-layer
    cull bands + shadow flags remain DATA in `DA_BattlefieldScatter` (populated at Phase 3) — this task ONLY wires the code
    path; do NOT change any density here. Defaults if a field is unset in data: `CullEndDistance=0` (never culled — safe,
    Phase 3 sets the real bands), `bCastShadows=true` for hills, `false` for grass/plants (CONVENTIONS).
    (2) **Grid-hash spacing accelerator.** Replace the naive/O(n²) MinSpacing + keep-clear proximity checks in the scatter
    placement loop with a uniform spatial-hash grid (cell size ≈ the largest MinSpacing/keep-clear radius in use) so
    MinSpacing/keep-clear queries stay ~O(1) at the denser instance counts. **Determinism is INVIOLATE:** the seed-order law
    holds — the same seed must produce byte-identical placement (same draws, same order, same accept/reject outcomes); the
    hash only accelerates the query, it must not change any result or draw order. NOT IN SCOPE: densities (Phase 3), LODs
    (Phase 4), the mine stream, L_Arena, SummonedUnit.
    ACCEPTANCE: cull fields present + applied in both component paths; a fixed seed produces byte-identical placement vs the
    pre-refactor build (prove it — log/compare the placement set for one seed before/after); no density change. QA implied
    (determinism review, shadow/include scans, null-safety). Post in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.{h,cpp}`, `ScatterConfig.h` (`FScatterLayer`). Fields EXACT:
    `CullStartDistance`, `CullEndDistance`, `bCastShadows` (CONVENTIONS scatter cull-field naming). Consumed as-is:
    `ResolveComponentForMesh`, `SetCullDistances`, `SetCastShadow`, the layer/stream structures. Grid-hash helper is internal
    (name at implementation, non-UPROPERTY). Law: CONVENTIONS "Arena 10× scale-up & LOD/perf (M7.6)" scatter cull-field
    naming + seed-order law, "Battlefield & procedural terrain (M6.5)".

#### TASK-285 — [P2] Unit URO / OnlyTickPoseWhenRendered block (C++, branch)
- assignee: gameplay-programmer
- status: done (INTEGRATED at TASK-286 batch, build-master 2026-07-24, commit on m7.6-arena10x — compile GREEN; PIE clean load + short match runs (units march/attack — Rule 4 Cavalry marching; error scan clean, no ensures/Accessed-None); `SummonedUnit.h` verified UNTOUCHED (constructor-only); off-screen pose correctness QA-proven structural. --- Prior QA 2026-07-24 PASS, 0 blockers, `qa/TASK-285.md`. Both URO flags set on `SkeletalVisualMesh` (cosmetic subobject, not ACharacter Mesh); pose-independence VERIFIED — grep 0 AnimNotify/RootMotion, damage applied DIRECTLY on the cadence timer (ApplyDamage/FireProjectileAt w/ explicit damage, no socket/bone reads), movement CMC, aggro timer-driven; SetVisibleInRayTracing correctly absent; TASK-282/020 paths disjoint. Awaits TASK-286 build w/ TASK-284.)
- blocked-by: none. ⚠ FILE-OVERLAP: shares `SummonedUnit.{h,cpp}` with the overnight ATTACK-bug fix TASK-282 — SERIALIZE on the file (do TASK-282 first, or whichever is in flight completes before the other opens the file). Not concurrent-safe with TASK-282.
- parallel-safe: yes vs TASK-284 (disjoint files); NO vs TASK-282 (same file — serialize)
- spec: >
    On `m7.6-arena10x`. Make off-screen units cheap at 10× scale. On the unit's `SkeletalVisualMesh` (configured around
    `SummonedUnit.cpp:119`) set `VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered`
    and `bEnableUpdateRateOptimizations = true` (CONVENTIONS SK-unit URO law). CONFIRM before shipping that no gameplay-
    critical logic depends on per-tick pose while off-screen — movement, aggro, target acquisition, and attack timing are
    driven by the AI/state machine and cadence timers, NOT the anim pose; verify this in source and state it in the handoff.
    If any critical path DOES read the pose off-screen, scope the URO so it does not regress (flag it). The pre-approved
    EMERGENCY perf lever `SetVisibleInRayTracing(false)` on unit meshes is NOT applied in this task by default (perf-watch
    shortfall only — reserve for W2/W3 if Jonathan calls for it). NOT IN SCOPE: the ATTACK/DEFEND/HOLD command logic
    (TASK-282's territory), LODs (Phase 4), scatter.
    ACCEPTANCE: the two flags set on `SkeletalVisualMesh`; no behavior regression in march/aggro/attack (state-driven, proven
    in the handoff); off-screen anim pop acceptable at gameplay cam. QA implied (shadow/include scans, confirm no off-screen
    pose dependency). Post in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}` (`SkeletalVisualMesh`, ~:119). Consumed as-is:
    `EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered`, `USkeletalMeshComponent::VisibilityBasedAnimTickOption`,
    `bEnableUpdateRateOptimizations`. Law: CONVENTIONS "Arena 10× scale-up & LOD/perf (M7.6)" SK-unit URO/anim-tick law.

#### TASK-286 — [P1∥2] Integration: compile + PIE + branch commit (build)
- assignee: build-master
- status: done (BUILD-MASTER 2026-07-24 — BATCHED M7.6 Phase 1 (TASK-284 scatter cull-bands + grid-hash) + Phase 2 (TASK-285 unit URO) integrated on m7.6-arena10x, commit `2249444` (no push). Jonathan awake: Step-0 PIE idle; save-all (`save_assets([])`=true) → graceful close (no dialog wedge, no force-kill); recompiled GREEN (~15s); relaunched on L_Arena. VERIFY — (TASK-284) clean GenerateScatter seed 774069313, all 7 layers placed EXACT target (6/8/8/70/60/2500/400) + 6 mines / 0 culls + Traversability CONFIRMED ⇒ grid-hash runtime-equivalent, no count-change; cull bands unpopulated-DA default = full render. (TASK-285) clean load + match runs (Rule 4 Cavalry marching), no ensures/Accessed-None; off-screen correctness QA-structural. `L_Arena`/`DA_BattlefieldScatter`/`BattlefieldScatter.h`/`SummonedUnit.h` all UNTOUCHED (verified). PERF: NOT machine-capturable (no console-exec/stat-read via MCP; detached-editor tick throttled while unfocused → the log's ~5 fps is that throttle, not gameplay; real fps = Jonathan gameplay-cam WATCH — this batch is perf INFRA pre-density for Phase 3/W2). Committed: `SummonedUnit.cpp`, `BattlefieldScatter.cpp`, `ScatterConfig.h`, board + `handoffs/qa` TASK-284/285. DeckBuilderWidget + WBP_DeckBuilder (parked TASK-268) stayed unstaged. No push. Was: backlog.)
- blocked-by: TASK-284 (qa-passed) + TASK-285 (qa-passed)
- parallel-safe: no (single editor + compiler + Git)
- spec: >
    On `m7.6-arena10x`: (1) COMPILE TASK-284 + TASK-285 (editor bounce — Jonathan's close/reopen grant covers the session).
    GREEN, report time + `Result: Succeeded`; failure → append to the relevant QA report, route back (counts as a QA loop).
    (2) VERIFY `git diff --stat` shows ONLY `BattlefieldScatter.{h,cpp}` / `ScatterConfig.h` / `SummonedUnit.{h,cpp}` — the
    branch-owned `L_Arena.umap` + `DA_BattlefieldScatter` MUST be UNTOUCHED (Phase 3 owns density); if anything else moved,
    STOP and report. (3) PIE SANITY: full A→B march completes, a fixed seed produces the SAME scatter layout as before the
    refactor (determinism proof — the same seed/positions), units cull + animate-when-rendered correctly (spot-check an
    off-screen unit still marches/fights), Play Again ×3 clean, Message Log clean. (4) CAPTURE MACHINE PERF baseline where
    MCP allows (this is the first structured perf number post-W1) into the handoff. (5) COMMIT on the branch, NO push. Post
    hash + perf numbers in 🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x` compile + commit (NO push). Law: CLAUDE.md hard gates, CONVENTIONS "M7.6" (branch ownership,
    seed-order law).

#### TASK-287 — [P3] DA_BattlefieldScatter density fill + cull bands/shadow data + nav rebuild + PIE + W2 machine capture + branch commit (build)
- assignee: build-master
- status: done (BUILD-MASTER 2026-07-24 — Phase 3 density fill applied to `DA_BattlefieldScatter` (data-only, NO compile), commit `8fe4991` (no push). Jonathan ruling "default the numbers, tune at W2". APPLIED ≈4.9× (total 3052→15000): Trees 70→340, Rocks 60→300, Boulders 6→30, Hill 8→40, Slabs 8→40, Grass 2500→12250, Plants 400→2000. Cull bands (uu): Grass 6000/9000, Plants 8000/12000, Rocks 14000/20000, Trees 24000/32000, Boulders/Hill/Slabs 0/0 (never). Shadows: obstacle layers ON, Grass/Plants OFF. Edit via ProgrammaticToolset server-side deepcopy, readback-verified ALL soft refs preserved (mesh counts + Hill `M_HillGrass` + Trees `Cylinder` proxy intact; DA saved is_dirty=false). TRAVERSABILITY PASS ×3 fresh seeds (494877441 / 197269377 / 1159629313): CONFIRMED 0 culls, counts on target (Hill 28-30/40, Grass ~12246/12250 = normal rejection-crowding); bot plays FULL ladder at density (Rule 2 Economy Deep Mine+Miners, Rule 4 Attack Cleric/Knight). ⚠ W2 WATCH: intermittent NON-FATAL large-world matrix-precision ensure (`DoubleFloat.cpp:19` / `Matrix.h:468` "precision loss converting matrix to GPU format / view transform") on 2/3 dense PIE generates — a rendering transient at 10× coords, gameplay+traversability fully intact, flagged for Jonathan's gameplay-cam. NAV: editor-python NOT MCP-reachable → nav NOT rebuilt/resaved, `L_Arena` NOT committed; runtime-Dynamic RecastNavMesh regenerates at PIE (non-breaking, all 3 runs confirmed); Jonathan's manual Build>Navigation click flagged in 🚨 Blockers. PERF not machine-capturable = W2 gameplay-cam WATCH. Committed: `DA_BattlefieldScatter.uasset` + board + `handoffs/TASK-287.md`. DeckBuilderWidget/WBP_DeckBuilder (parked) stayed unstaged. No push. Full table in handoffs/TASK-287.md. Was: backlog.)
- blocked-by: TASK-286 (P1∥2 integrated — the cull fields + grid-hash MUST exist before the denser fill is affordable)
- parallel-safe: no (single editor + Git; edits branch-owned `DA_BattlefieldScatter` + rebuilds nav)
- spec: >
    On `m7.6-arena10x`, in the editor. Populate `DA_BattlefieldScatter` with the §3 density fill for the 10× field: per-layer
    instance COUNTS scaled up for the wider ±12,000 field, plus per-layer `CullStartDistance`/`CullEndDistance` cull bands and
    `bCastShadows` flags (the TASK-284 code fields). **⚠ EXACT COUNTS ARE FLAGGED (plan overwritten — see the block's
    numeric-source flag):** DEFAULT heuristic overnight = scale the current Phase-0 per-layer densities to ≈4.9× total
    instances (milestone entry) distributed by the existing per-layer proportions, with cull bands set so distant instances
    stop rendering (e.g. grass `CullEndDistance` short, trees/rocks mid, hills never culled + shadows ON, grass/plants shadows
    OFF). RECORD the exact numbers you applied in the handoff as a table for Jonathan's W2 review — these ARE the W2 levers
    (grass count, cull distances, RT-off units). Do NOT touch cull-field CODE (TASK-284) or unit code. After the density
    change: re-run Build > Navigation + resave (editor-python route; else flag Jonathan's one click in 🚨) so the nav bake
    reflects any new blockers, and verify the NON-NEGOTIABLE castle↔castle traversability guarantee still holds (0 culls, or
    the widening-cull machinery resolves it). PIE: full A→B march, Play Again ×3, Message Log clean. **W2 MACHINE CAPTURE:**
    capture fps/`stat unit`/instance counts at several field positions into the handoff — this is the perf number Jonathan
    reviews at W2 in the morning; the human feel-verdict is HIS and does NOT block Phase 4 overnight. COMMIT on the branch,
    NO push. Post hash + the density table + perf numbers in 🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x`; `DA_BattlefieldScatter` (branch-owned); RecastNavMesh actor / L_Arena nav bake. Law: CONVENTIONS
    "M7.6" (scatter cull-field naming, W-gate law — W2 is a Jonathan morning watch), "Battlefield & procedural terrain (M6.5)"
    (traversability guarantee), plan §3 density table (recorded numeric truth = the applied table in this handoff).

#### TASK-289 — [P4] LOD apply on the branch: LargeProp LOD line + SK-LOD regen + donor LOD audit + PIE + W3 machine capture + branch commit (build)
- assignee: build-master
- status: done (BUILD-MASTER 2026-07-24 — Phase 4 LOD: conflict-free parts done + 2 blockers FLAGGED, commit `9fa6b92` (no push). NO compile (editor + audit + git). (1) LargeProp LOD line ALREADY PRESENT in `Tools/reimport_meshes.py` (TASK-220 `DEFAULT_LOD_GROUP='LargeProp'`) — no port needed. (2) SK-unit LOD apply DEFERRED — **BLOCKER A:** `regenerate_lod` is editor-python, NOT MCP-reachable (SkeletalMeshTools has NO LOD-gen tool; ProgrammaticToolset can't run `unreal.` API); **BLOCKER B:** the 11 SK fleet meshes are M7.5 retexture territory (TASK-201/202, gated on undone TASK-200) → committing branch LODs on them = binary merge conflict → per the cross-batch ruling did NOT touch. URO flags already live via TASK-285 C++. SK baseline readback: all 11 `SK_` = LOD0-only (14k-22k verts). Recommend applying SK-LODs on MAIN after M7.5 retexture via the TASK-288 recipes. **[FOLLOW-UP FILED 2026-07-25: this SK-LOD apply is now TASK-297 (build-master, on main via the editor-python PythonScriptPlugin remote-exec lane — both deferral blockers cleared: M7.5 retexture merged to main `4c680bb`; TASK-288 recipes committed via TASK-289b `1df47b6`).]** (3) DONOR AUDIT (read-only): Rocks/Boulders/Slabs = 6 LODs, Grass/Plants = 5 LODs (Fab — cover the 12250+2000 dominant counts); ⚠ **TREES = LOD0-only (340 blocking inst × ~2450 tris = the #1 W3 lever, no reduction)** + Hill LOD0-only (minor) → RECOMMEND follow-up to generate Tree/Hill donor LODs (`generate_lods`, MCP-reachable, conflict-free — Trees ≠ M7.5 fleet). (4) PIE: Traversability CONFIRMED (scatter byte-identical to TASK-287 `8fe4991` — its ×3 stands); the TASK-287 LWC matrix-precision ensure still appears intermittently (unchanged by this task). (5) W3 fps NOT machine-capturable = Jonathan gameplay-cam WATCH; objective LOD evidence = the audit table in `handoffs/TASK-289.md`. Committed: board + `handoffs/TASK-289.md` (docs only — no asset/tooling change). DeckBuilderWidget/WBP_DeckBuilder + all M7.5 `SK_`/`SM_` fleet untouched/unstaged. No push. Blockers in 🚨 Blockers. Was: backlog.)
- blocked-by: TASK-287 (P3 committed — density is in, so LODs are measured against the real instance load) + TASK-288 (qa-passed — the rig LOD step exists)
- parallel-safe: no (single editor + Git)
- spec: >
    On `m7.6-arena10x`. Apply the fleet LOD architecture and MEASURE at W3. (1) Ensure the branch's `Tools/reimport_meshes.py`
    carries the TASK-220 `lod_group='LargeProp'` line (already qa-passed on main — CHERRY-PICK it into the branch if the branch
    base predates it; verify with a diff). (2) Regenerate SK LODs on EXISTING rigs via the editor-python `regenerate_lod`
    helper (LOD1 50% @ 0.4 / LOD2 20% @ 0.15) + set the URO flags — **⚠ CROSS-BATCH FLAG:** the SM_/SK_ fleet meshes are
    M7.5's main-lane retexture territory (TASK-201/202, themselves gated on Jonathan's TASK-200 A/B eyeball, NOT yet done).
    To avoid a merge conflict, DEFAULT overnight = apply LODs ONLY in a way that does not fight M7.5 (regenerate LODs on the
    branch's current mesh assets; the merge inherits M7.5's retexture+LOD from main at Phase 6). If reimporting the fleet on
    the branch would stomp M7.5-owned assets, DO NOT — FLAG it for Jonathan's morning and apply what is conflict-free
    (LargeProp line present in the branch tooling, SK-LOD regen, donor LOD audit). (3) DONOR LOD AUDIT: read-only check that
    the scatter donor SMs carry LOD chains; list any missing in the handoff. (4) PIE: full A→B march, Play Again ×3, Message
    Log clean. **W3 MACHINE CAPTURE:** capture fps/`stat unit`/draw-call/LOD-transition numbers vs the TASK-287 baseline
    (expect strict improvement) into the handoff — Jonathan's human W3 feel-verdict is deferred to morning, does NOT block
    Phase 5 overnight. (5) COMMIT branch-owned + tooling changes on the branch, NO push. Post hash + before/after perf in
    🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x`; `Tools/reimport_meshes.py` (TASK-220 line), the `regenerate_lod` editor-python helper. Law:
    CONVENTIONS "M7.6" classic-LOD law (SM LargeProp / castle explicit / SK LOD + URO), Nanite vista amendment (vista is
    Phase 5, not here), sequencing law (TASK-220 line), branch-ownership. FLAG the M7.5 SM_/SK_ fleet-reimport coordination.

#### TASK-291 — [P5] Place vista ring + POIs + gold-node props + fog/light polish into L_Arena (seeded one-shot) + branch commit (build)
- assignee: build-master
- status: done (BUILD-MASTER 2026-07-24 — Phase 5 dressing PLACED in L_Arena + fog polish + committed, commit `10bb474` (no push). **⭐ M7.6 Phase 1–5 ENGINEERING COMPLETE.** Editor was on L_MainMenu (TASK-290 prep) → loaded L_Arena first. Live nav bounds ±28000/±12500 → vista ring safely outside. Placed 26 StaticMeshActors (0 fails): 16 vista (ring ±34000/±20000, peaks 12-14× / ridges 8×; per-instance `castShadow=false`/`bVisibleInRayTracing=false`/`bCanEverAffectNavigation=false` all SET ×16), 8 POI (off-lane |Y| 3.5-7k, ≥~7k from castles, scale 2.5-3.5×), 2 gold props at (0,±5000) (VISUAL-only StaticMeshActors, NOT AGoldNode/ACaptureZone). Fog polish: `ExponentialHeightFog` density 0.008→0.012 + maxOpacity 0.85→0.92 (startDistance 10k kept = vista depth); DirectionalLight left as-is. TRAVERSABILITY UNCHANGED — dressing NoCollision + bCanEverAffectNav=false; PIE CONFIRMED 0 culls across fresh seeds (2138636033 / 1633272449). ⚠ W-GATE FINDING: `LogUnrealMath InverseFast non-invertible→NaN` now fires at PIE first-frame (large-world precision family with the TASK-287 LWC ensure, EXACERBATED by the extreme vista coords — non-fatal, gameplay + traversability intact; the LWC ensure predates the vistas so a fully clean log needs an LWC project setting, not just dressing tweaks; mitigation = pull ring in / cap scale / LWC tile setting, Jonathan's call). Also a pre-existing `InputMode:UIOnly` HUD-focus error, unrelated to dressing. BRANCH-OWNED-ONLY diff verified: `L_Arena.umap` + 9 `SM_` dressing `.uasset` ONLY — no fleet code / DA / SummonedUnit / SiegeBotController / gameplay source touched. Committed: L_Arena.umap + 9 .uasset (LFS) + board + `handoffs/TASK-291.md`. DeckBuilderWidget/WBP_DeckBuilder + all M7.5 SK_/SM_ fleet untouched/unstaged. No push. ⚠ **Phase 6 (capstone playtest + MERGE-TO-MAIN) is Jonathan's gate — NOT started here.** Was: backlog.)
- blocked-by: TASK-290 (assets prepared)
- parallel-safe: no (single editor + Git; edits branch-owned L_Arena)
- spec: >
    On `m7.6-arena10x`, in the editor. Place the TASK-290 assets into `L_Arena` via editor-python as a SEEDED one-shot (a
    fixed layout — the vista/POI dressing does NOT re-seed per match; only the scatter does): a vista ring of cliff/mountain
    silhouettes around the far perimeter (outside the ±12,000 play bounds, no collision/nav), 6–10 POI landmark props inside
    the field (clear of the corridor + keep-clears — do NOT block the castle↔castle path), 2 neutral gold-node visual props
    mid-field, and a fog/light polish pass (ExponentialHeightFog + light tuning for the vista depth — no collision/nav
    change). Verify: castle↔castle traversability UNCHANGED (POIs/vista do not obstruct), nav bake still valid (vista/POI are
    no-nav; re-run Build>Navigation only if a POI has collision — prefer NoCollision POIs), PIE full A→B march, Play Again ×3,
    Message Log clean. **Branch-owned-only diff:** `git diff --stat` should show `L_Arena.umap` (+ any new dressing .uassets
    from TASK-290) — the fleet code/DA untouched unless intended; STOP + report on any unexpected change. COMMIT on the branch,
    NO push. **This is the last OVERNIGHT phase — do NOT proceed to Phase 6** (capstone playtest + MERGE GATE = Jonathan's,
    morning). Post hash + a screenshot/description of the dressed field in 🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x`; `/Game/Maps/L_Arena` (branch-owned). Law: CONVENTIONS "M7.6" (Nanite vista amendment, branch
    ownership, W-gate/merge law — merge is Phase 6, Jonathan's), "World axes (arena contract)" (main's law untouched until
    merge), milestone Standing backlog (neutral-node capture hook).

#### TASK-292 — [M7.6 W-gate follow-up] DIAGNOSE + fix the LWC render-precision ensure + InverseFast NaN at PIE first-frame (gameplay-programmer diagnosis → build-master L_Arena fix)
- assignee: gameplay-programmer (diagnosis) → build-master (applies the L_Arena property fix + verifies + commits)
- status: done — VISTA FIX APPLIED + VERIFIED; ⚠ RESIDUAL scatter-DF NaN → follow-up (BUILD-MASTER 2026-07-24, commit `b513f09` (no push). Set `bAffectDistanceFieldLighting=false` + `bAffectDynamicIndirectLighting=false` on ALL 26 dressing components (16 vista `StaticMeshActor_7..22` + 8 POI + 2 gold; readback-verified 26/26 off; used label `GoldProp` so NO gameplay `GoldNode` touched); saved L_Arena. FRESH-EDITOR RESTART verify (my PID 16232 had crashed → MCP reconnected to a live editor; I save-all'd + graceful-closed it, relaunched fresh PID 8644 on L_Arena, which loaded the saved flags): **PIE gen 1 (seed 1857084801) Message Log CLEAN — the OriginX ensure (`DoubleFloat.cpp:19`) + InverseFast NaN (`Matrix.h:468`) BOTH GONE**, traversability CONFIRMED 0 culls. **BUT gen 2 (seed 1529263745) the ensure + NaN RETURNED** (right after GenerateScatter) → the VISTA source is eliminated, but a RESIDUAL comes from the SCATTER layers' DF participation (the pre-existing TASK-287 source, diagnosis §6.3 predicted; seed-dependent = intermittent). ⚠ **FALLBACK (b) = drop DF flags on the scatter HISM layers = a `BattlefieldScatter.cpp` CODE change** (no `FScatterLayer` DF field exists) → **HANDED BACK to gameplay-programmer as follow-up (TASK-292c)**. Fallback (a) pull-vista-ring-in is INAPPLICABLE (vistas already out of DF). NO ini/project setting touched (diagnosis §2 ruled out). Committed: `L_Arena.umap` (LFS) + `handoffs/TASK-292.md` (diagnosis) + `handoffs/TASK-292b.md` (this result) + board — branch-owned-only (no code/DA/fleet); DeckBuilder/WBP parked untouched; no push. ⚠ **The ensure/NaN is NOT fully eliminated until the scatter-DF follow-up lands** (W-gate flag). --- Was: ready-for-integration (diagnosis, handoffs/TASK-292.md).
- blocked-by: none (follow-up to TASK-291 `10bb474`)
- parallel-safe: no (build-master edits branch-owned L_Arena + PIE-verifies)
- spec: >
    SYMPTOM (reproduced, `Saved/Logs/GitClaudeUnrealTest.log` ~L2645/L2704): at PIE first-frame on L_Arena the Message Log
    shows `EnsureFailed: OriginX <= OriginMax … precision loss while converting matrix to GPU format` (`DoubleFloat.cpp:19`)
    immediately followed by `TMatrix InverseFast … non-invertible matrix → NaN` (`Matrix.h:468`), the InverseFast then
    repeating every frame. Non-fatal; gameplay + traversability intact.
    ROOT CAUSE (EVIDENCE): (1) Engine source — `OriginMax = UE_DF_FLOAT_MAX_VALUE = (1<<23)*0.25 − 1 = 2,097,151 uu ≈ 21 km`;
    the whole scene sits within ±37,000 uu (0.37 km) = **56× under the limit** → the ensure is NOT a magnitude/tile overflow;
    `NaN <= 2097151` is `false`, so it can only fire on a NaN. `Matrix.h:468` is `ErrorEnsure`, called by `InverseFast` on a
    non-invertible matrix — the exact NaN source. **The two errors are ONE bug.** (2) All 16 vista actor transforms are
    UNIFORM invertible scales (8/12/13/14×), clean positions (≤±34k) + bounds → no degenerate actor transform. (3) The vista
    components have `CastShadow=false`/`bVisibleInRayTracing=false`/`bCanEverAffectNavigation=false` SET, **but
    `bAffectDistanceFieldLighting=true` + `bAffectDynamicIndirectLighting=true` were LEFT ON** (the TASK-290 Nanite-vista
    amendment omitted them). The meshes are NON-Nanite (~320×270×504 local) scaled 8–14× → ~4000×3500×6500 world; with
    `r.GenerateMeshDistanceFields=True`, Lumen GI (`r.DynamicGlobalIlluminationMethod=1`), and the DirectionalLight's
    `DistanceFieldShadowDistance=30000`, these huge distant backdrop meshes feed the distance-field + Lumen-GI scene, whose
    per-mesh capture/DF matrix goes singular → the NEW-since-vista InverseFast NaN. BOTH build-master hypotheses REFUTED:
    an LWC/world-tile ini setting cannot change a hardcoded engine constant and does not address a NaN; the vista transforms
    are not degenerate.
    FIX (build-master, L_Arena, NO code/compile): on the 16 vista `StaticMeshActor_7..22` (and recommended on the 8 POI +
    2 gold-node props — all pure dressing) set `bAffectDistanceFieldLighting=false` + `bAffectDynamicIndirectLighting=false`;
    save L_Arena, restart editor, PIE, confirm the Message Log is clean (no `OriginX` ensure, no `InverseFast` NaN) and that
    traversability/rendering/perf are unregressed. FALLBACK if either error persists (documented in the handoff): pull the ring
    inside `DistanceFieldShadowDistance` (≤~28,000) and/or drop the DF flags on the scatter layers. Post in ⚙️ Dev & QA.
- names: >
    `L_Arena` vista/POI/gold-prop `StaticMeshComponent` props `bAffectDistanceFieldLighting` + `bAffectDynamicIndirectLighting`
    (build-master lane). Diagnosis evidence: `Config/DefaultEngine.ini` (r.* render settings), UE_5.8 `DoubleFloat.cpp:10-25` /
    `Matrix.h:465-469`. Law: CONVENTIONS "M7.6" Nanite vista amendment (this fix ADDS the two DF/GI-off flags to the
    vista-dressing law), branch-ownership (L_Arena is branch-owned), W-gate.

#### TASK-293 — [M7.6 P5 polish] Fill the vista-ring sky-gaps in L_Arena (continuous layered backdrop) + branch commit (build)
- assignee: build-master
- status: done (BUILD-MASTER 2026-07-25 — commit `c35f146` (L_Arena+handoff+board; no push). RE-DO of a stalled attempt that persisted nothing. ONE deterministic editor-python placement script (fixed LCG seed 20260725, rounded-rectangle ring projection so nothing lands inside the play box), run synchronously + SAVED L_Arena immediately (git-verified umap modified 122KB→521KB BEFORE screenshots/PIE). MCP client was wedged all task → drove everything via the raw-HTTP MCP fallback (dual-channel SSE). Placed **62 fill instances** (`VistaFill_01..62`, 0 spawn fails) across 3 radial depth bands, all OUTSIDE nav (X±28000/Y±12500) + boundary walls: FRONT tree-line/low-ridge (RX31000/RY17000, 20) · MID main ridge interleaving the existing 16 (RX34000/RY20500, 24) · BACK tall peaks behind gaps (RX36500/RY22500, 18). Composition (existing assets only): **34 rocks** (20 peak `SM_Vista_01/04` + 14 ridge `SM_Vista_02/03`, scale 9–15×) + **13 hills** (`SM_Hill_01/02/03`, 6–10×) + **15 trees** (`SM-Mobile_Tree_1..12`, 7–11×), varied yaw + radial jitter → layered ridgelines, silhouettes overlap. ⚠ FULL vista flag set (`CastShadow`/`bVisibleInRayTracing`/`bCanEverAffectNavigation`/`bAffectDistanceFieldLighting`/`bAffectDynamicIndirectLighting` all=false) SET + readback-verified on ALL 62 → **verified_off=62/62** (matches proven `Vista_01..16`). Gotcha: `set_properties` `values` is a JSON-encoded STRING not an object (object = silent no-op). SCREENSHOTS (sent, unstaged in handoffs/): top-down shows a CONTINUOUS 360° ring; two low/outward shots at former 66°/294° gaps show the horizon fully backed (no drop-off void). LWC CLEAN — ONE PIE (seed 901016449): `InverseFast` NaN + `OriginX<=OriginMax` ensure + `non-invertible` = 0 across whole session log (DF-off held with +62 instances); `Traversability CONFIRMED — Blue→Red + 6 mine path(s) (after 0 cull(s))` current run. Branch-owned diff = `L_Arena.umap` ONLY (+ board + handoff); NO code/DA/fleet; DeckBuilderWidget/WBP_DeckBuilder (parked) unstaged; LFS ok; no push. Existing 16 vistas + 8 POI + 2 gold props untouched. Follow-up (report-only): a thin sky saddle between two MID crests is visible ONLY from a 130 m oblique, NOT from gameplay-height low shots — a couple more BACK peaks close it later if a pixel-check flags it (existing assets suffice). Full detail in handoffs/TASK-293.md. Was: re-do/stalled.)
- blocked-by: none (follow-up to TASK-291 `10bb474` / TASK-292c LWC closure)
- parallel-safe: no (build-master edits branch-owned L_Arena + PIE-verifies + commits)
- spec: >
    Jonathan's screenshot: the TASK-291 16-instance vista ring is too SPARSE — ~48° angular sky-gaps on the long (±Y) sides,
    and through the gaps the ground plane ends and drops to sky/void at the horizon. Fill the ring into a CONTINUOUS, varied,
    layered backdrop (rocks + trees + hills) so no gap shows the world edge, from EXISTING assets only (no new art). ~40–80
    instances, 2–3 radial depths, silhouettes touch/overlap, all OUTSIDE the play bounds. EVERY new instance gets the full
    vista flag set (readback-verified). SAVE L_Arena immediately (crash-robust), then screenshots (top-down + low-at-gap) +
    ONE PIE (LWC ensure/NaN gone, traversability 0 culls). Commit `L_Arena.umap` + docs, explicit pathspecs; DeckBuilder parked;
    no push. Post in 🔧 Build & Git.
- names: >
    `L_Arena` `VistaFill_01..62` StaticMeshActors from `/Game/Meshes/SM_Vista_01..04`, `/Game/Meshes/SM_Hill_01..03`,
    `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_1..12`; component vista flags per TASK-290/292 law. Branch-owned
    L_Arena (M7.6 branch-ownership). Law: CONVENTIONS "Arena 10× scale-up & LOD/perf (M7.6)" (vista dressing + flag set), W-gate.

---

#### TASK-294 — [M7.6 P5 dressing fix] Reposition ALL vista-ring objects so NONE overhang the walkable area (no-overhang, occlusion preserved) + branch commit (build)
- assignee: build-master
- status: done (BUILD-MASTER 2026-07-25 — commit `238a2da` (L_Arena+handoff+board; no push). Vista meshes are NoCollision but scaled 6–15× so their GEOMETRY overhung inward past the walkable boundary even though origins sat outside → units pathed through the overhanging parts. Read pass: nav ±28000/±12500, 78 dressing actors (16 `Vista_*` + 62 `VistaFill_*`, label regex `^(Vista|VistaFill)_\d+$` so POIs/gold/scatter/walls excluded); world AABB half-extents up to ~17,600 uu → **52 of 78 overhung**, worst `VistaFill_23` reached **8,351 uu INTO the field**; ground-plane probe: +X/−X & corners ~44–48k, short ±Y sides ~30–32k. APPROACH: per object took the live world AABB (accounts for yaw+scale), computed the outward radial unit from center, solved the MINIMAL outward translation so the translated AABB clears the nav rect expanded by **margin 2500 uu** (separation on the natural first-clearing axis; bigger meshes move farther); z/yaw/scale kept → layered bands + yaw variety preserved. The 26 already-clearing objects were LEFT (moving them would reduce occlusion); only 52 overhangers moved. ONE atomic write: **moved 52/52 (0 fail)**, SAVED L_Arena immediately (git-verified umap modified on disk BEFORE screenshots/PIE). **No-overhang proof** (independent fresh re-scan): **0/78 overhang, MIN clearance across all 78 = 2,499 uu**. **Occlusion preserved BY CONSTRUCTION** — huge meshes mean inner faces land at ~30,500(X)/~15,000(Y), still far inside the ground edge (44k/30k); tightest inner-face-vs-ground margin +2,846 uu (`VistaFill_26`); NO object's inner face exceeds ground → **no void revealed, no taller vistas / no ground-extension needed**. SCREENSHOTS (sent, unstaged in handoffs/): `TASK-294-topdown` (ring outside nav rect), `-low-posY`/`-low-negY` (tight short sides, solid wall backs horizon, no void), `-low-posX`, `-low-corner` (tightest corner — rocks/trees/hills above the wall, no gap), `-oblique-posY` (continuous ring). Vista flags PRESERVED — readback all 78 → `flags_ok=78/78`, `flags_fixed=0` (`CastShadow`/`bVisibleInRayTracing`/`bCanEverAffectNavigation`/`bAffectDistanceFieldLighting`/`bAffectDynamicIndirectLighting` all=false). LWC CLEAN — ONE PIE: `InverseFast` NaN + `OriginX` ensure + `non-invertible` = 0 whole session; `Traversability CONFIRMED — Blue→Red + 6 mine path(s) (after 0 cull(s))`, 7 scatter layers on target. Branch-owned diff = `L_Arena.umap` ONLY (+ board + handoff); NO code/DA/fleet; DeckBuilderWidget/WBP_DeckBuilder (parked) unstaged; LFS ok; no push. Existing scatter/POIs/2 gold props untouched. Full detail + follow-ups in handoffs/TASK-294.md. Was: backlog.)
- blocked-by: none (follow-up to TASK-293 `c35f146`)
- parallel-safe: no (build-master edits branch-owned L_Arena + PIE-verifies + commits)
- spec: >
    Reposition ALL vista-ring objects (16 `Vista_*` + 62 `VistaFill_*`) so NONE overhang the walkable area (units must not walk
    into/through any geometry) while STILL occluding the ground drop-off from the gameplay cam. Keep arena/playable/nav size
    EXACTLY the same — only vista dressing moves. Read live nav + visual-ground extents; per object compute scaled world bounds,
    push radially OUTWARD so the inner geometry edge clears the walkable boundary by ≥1500–2000 uu; keep the layered/varied look;
    preserve full vista flags. SAVE L_Arena immediately (crash-robust). Verify: no-overhang (min clearance), occlusion (top-down +
    low-outward screenshots, no void), LWC clean (one PIE) + traversability 0 culls. If pushing out reveals a gap → taller vistas
    or FLAG a ground-extent conflict. Commit `L_Arena.umap` + docs, explicit pathspecs; DeckBuilder parked; no push. Post 🔧 Build & Git.
- names: >
    `L_Arena` `Vista_01..16` + `VistaFill_01..62` StaticMeshActors (branch-owned L_Arena, M7.6 branch-ownership). No new
    identifiers. Law: CONVENTIONS "Arena 10× scale-up & LOD/perf (M7.6)" (vista dressing + flag set), W-gate.

#### TASK-295 — [M7.6 P5 dressing] Decorative backdrop ground apron fills the field-edge→vista void in L_Arena + branch commit (build)
- assignee: build-master
- status: done (BUILD-MASTER 2026-07-25 — commit `709fb10` (L_Arena+handoff+board; no push). Jonathan's screenshot: from the angled/RTS cam a VOID horizon band sits between the field edge and the vista ring → vistas read as "floating." Read pass matched the field ground: **`ArenaGround`** = scaled `/Engine/BasicShapes/Cube.Cube` + override material **`/Game/Materials/Instances/MI_BattlefieldGround`** (M6.5 tri-planar grass), **top surface Z=0**, bounds X±28000/Y±12500 — the field ground ENDS EXACTLY at the nav boundary; beyond it is void. Added ONE decorative apron **`BackdropApron`** (`StaticMeshActor_33`): SAME Cube mesh + SAME `MI_BattlefieldGround` instance (tri-planar → grass tiles continuously across the field/apron boundary, no UV seam, exact color match), loc (0,0,−55) scale (800,640,1) → half-extents **X±40000 / Y±32000**, **top surface Z=−5** (5 uu BELOW field top → underlaps the field, opaque field hides the overlap, sub-pixel step = no z-fight/no seam; extent reaches PAST the vista bases so the apron's far edge is tucked UNDER the ring meshes on every side = hidden). **No new art asset** (reused engine Cube + existing MI). ⚠ FLAGS readback-verified all=false: `CastShadow`/`bVisibleInRayTracing`/`bCanEverAffectNavigation`/**`bAffectDistanceFieldLighting`**/**`bAffectDynamicIndirectLighting`** — the two DF/GI flags are the load-bearing guard vs the TASK-292/292c far-coord LWC NaN (held; apron still receives lighting → renders lit green). Collision `collisionProfileName=NoCollision` SET; derived `collisionEnabled` stays `QueryAndPhysics` (KNOWN TASK-293 toolset limit — not writable; non-issue: nav-off + below field surface + out of reach; traversability proves walkable area unchanged). ONE atomic script spawned+flagged+material+SAVED L_Arena, git-verified umap modified on disk BEFORE screenshots/PIE. **Gap-filled proof** (screenshots sent, unstaged in handoffs/): `TASK-295-eye-posX-ingap`/`-eye-posY-ingap` (eye-level in the former gap — apron grass runs unbroken up to+under the vista rock/hill bases, no void, vista grounded; ±Y = widest former gap, +X = long side), `-oblique-posY` (continuous field→vista, no sky-gap), `-topdown` (grass skirt surrounds field, reaches ring on all sides), `-rts-posY`/`-rts-negY`/`-rts-posX` (RTS-angle over the wall, continuous grass). **LWC CLEAN + traversability — ONE fresh PIE (seed 710563009, polled for a fresh line):** `[23.36.29] Traversability CONFIRMED — Blue→Red castle path + 6 mine path(s) (after 0 cull(s))`; `InverseFast` NaN + `OriginX` ensure + `non-invertible` = 0 whole session (DF-off held); 7 scatter layers on target. Branch-owned diff = `L_Arena.umap` ONLY (+ board + handoff); NO code/DA/fleet/gameplay; DeckBuilderWidget/WBP_DeckBuilder (parked) unstaged; LFS ok; no push. Playable ground/collision/nav/scatter/vistas/POIs/gold props untouched. Full detail + follow-ups in handoffs/TASK-295.md. Was: backlog.)
- blocked-by: none (follow-up to TASK-294 `238a2da`)
- parallel-safe: no (build-master edits branch-owned L_Arena + PIE-verifies + commits)
- spec: >
    Add DECORATIVE backdrop ground to fill the visible void band between the playable field edge and the vista ring (VISUAL
    ONLY, behind the nav barrier — units can't reach it); keep the playable/walkable/nav area EXACTLY unchanged. Match the field
    ground's MATERIAL + Z so the apron blends seamlessly (no color/height seam). Big flat plane at the field Z (or slightly below)
    using the same grass material, extending from the field edge out PAST the vista bases so its far edge hides behind the ring.
    Apron flags: NoCollision + `bCanEverAffectNavigation=false` (must NOT change walkable area) + `bAffectDistanceFieldLighting=false`
    + `bAffectDynamicIndirectLighting=false` (far-coord LWC guard) + `CastShadow=false`; readback-verify. SAVE L_Arena immediately
    (crash-robust). Verify: gap-filled screenshots at gameplay-cam height, playable-area unchanged (PIE A→B traversability 0 culls),
    LWC clean (one PIE). Commit `L_Arena.umap` (+ any new apron mesh) + docs, explicit pathspecs; DeckBuilder parked; no push. Post 🔧 Build & Git.
- names: >
    `L_Arena` `BackdropApron` StaticMeshActor (branch-owned L_Arena, M7.6 branch-ownership). Reuses `/Engine/BasicShapes/Cube`
    + `/Game/Materials/Instances/MI_BattlefieldGround` — no new identifiers. Law: CONVENTIONS "Arena 10× scale-up & LOD/perf (M7.6)".

---

