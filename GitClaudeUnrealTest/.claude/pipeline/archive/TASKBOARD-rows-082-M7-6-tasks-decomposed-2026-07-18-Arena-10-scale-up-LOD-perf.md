<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-249 — M_HillGrass: tri-planar/slope-blend grass material + hill-layer wire (art, editor)
- assignee: art-director
- status: **done** (2026-07-22 — COMMITTED at the final overnight branch fold **`f49d8b1`** on m7.6-arena10x (M_HillGrass new + DA wire; LFS-pointer clean). ← was: **ready-for-integration** (2026-07-22 overnight — M_HillGrass authored STOCK-NODES (world-aligned tri-planar 3-plane blend + M_BattlefieldGround's exact macro-tint recipe at the MI's live values + SmoothStep slope blend to desaturated dirt on steep faces; ~1 s compiles, param-only iteration) + hill layer `OverrideMaterial` wired in DA_BattlefieldScatter + opt-ins set (Trees/Rocks/Grass/Plants `bAllowOnHills=true`, slope 35 default). VERIFIED over 3 re-seeded runs (seeds 471742529/53151321/1677445121): hills GRASSY + continuous with the field up close and at gameplay cam, zero slope smear, props visibly ON hill surfaces (TASK-250 NIT-2 same-frame-bodies assumption CONFIRMED), 100% fill every layer every seed (NIT-1 clear), Traversability CONFIRMED 0 culls ×3. ⚠ FOUND+FIXED: fresh material needed `bUsedWithInstancedStaticMeshes=true` — without it runtime HISM SetMaterial silently renders the default WorldGridMaterial (gray); flag law candidate for CONVENTIONS. Component-level ground truth verified (PIE HISM OverrideMaterials[0]=M_HillGrass). Saves scoped: M_HillGrass (new, SCC auto-staged AM) + DA only; SM_Hill donors + L_Arena untouched. Before/after captures: Tools/ArtPipeline/Cache/HillGrass/w1prep/. handoffs/TASK-249.md)
- blocked-by: TASK-250 (OverrideMaterial field compiled) for the WIRE step only
- parallel-safe: no (editor; coordinate with Jonathan — he is playing)
- spec: >
    Author `/Game/Materials/M_HillGrass`: WORLD-ALIGNED TRI-PLANAR (or slope-blend) grass matching `M_BattlefieldGround`'s
    look — grass on walkable faces, rock/dirt blend on steep faces; straight XY projection is FORBIDDEN (smears on slopes;
    CONVENTIONS W1-PREP law). Stock nodes (Custom-HLSL law). Then WIRE: set the hill layer's `OverrideMaterial` to
    M_HillGrass in DA_BattlefieldScatter (branch-owned — legal on this lane) once TASK-250 compiles; verify on a re-seeded
    scatter. ACCEPTANCE = Jonathan's screenshot bar: hills read as grassy terrain CONTINUOUS with the ground at gameplay
    camera + up close on a climb; steep faces don't smear; main-lane SM_Hill assets untouched. Captures for his verdict.
    Post in 🎨 Art.
- names: >
    `/Game/Materials/M_HillGrass` (+ MI if parameterized); DA_BattlefieldScatter hill layer `OverrideMaterial`. Law:
    CONVENTIONS "W1-PREP additions", Custom-HLSL law, M7.6 branch-ownership (DA is branch-owned — this IS the branch lane).

#### TASK-250 — Scatter-on-hills + OverrideMaterial field (C++, branch)
- assignee: gameplay-programmer
- status: **done** (2026-07-22 — COMMITTED at the final overnight branch fold **`f49d8b1`** on m7.6-arena10x (ScatterConfig.h + BattlefieldScatter.{h,cpp}; compiled 0 err/0 warn at the pre-W1 bounce). ← was: qa-passed (2026-07-22 — qa/TASK-250-qa.md PASS 0 blockers 0 warns; diagnosis independently verified, RNG-stream regression trap CLEARED (existing seeds byte-identical), LineTraceComponent per-instance-body claim engine-verified, traversability laws untouched; 3 nits incl. the same-frame-bodies runtime assumption → visual confirm at TASK-249's first re-seeded wire. Pre-W1 bounce clear to compile. COMPILE DONE at the 2026-07-22 pre-W1 bounce: Build.bat SUCCEEDED 0 err/0 warn, 17s incremental — DLL live for TASK-249's wire step; the 249/250/251 branch-commit fold still pending)
- blocked-by: none
- parallel-safe: yes (branch C++ files; no editor)
- spec: >
    On m7.6-arena10x: (1) DIAGNOSE why nothing spawns on hills (expected: the hill layer's footprint/MinSpacing acts as an
    exclusion zone + placement traces resolve before/around hills) — record the actual mechanism in the handoff. (2) ENABLE
    grass/plants/rocks/trees to place ON hill surfaces: order placement so hills exist BEFORE dependent layers trace; traces
    accept elevated ground Z (hill surface = valid ground); per-layer `bAllowOnHills` + `MaxPlacementSlopeDeg` (default ~35°,
    UPROPERTY tunables) so props sit sanely; keep-clear/corridor/traversability laws UNCHANGED (the castle-to-castle
    guarantee is non-negotiable). (3) ADD `FScatterLayer.OverrideMaterial` (TSoftObjectPtr<UMaterialInterface>, null = donor
    materials) applied in ResolveComponentForMesh() + the proxy path — TASK-249 consumes it. ACCEPTANCE: re-seeded scatter
    places props on hill tops/flanks within the slope limit; no corridor/keep-clear regressions; OverrideMaterial applies
    when set and no-ops when null. QA implied (shadow + include scans). Compile rides the pre-W1 build-master bounce. Post
    in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h`, `BattlefieldScatter.{h,cpp}` (branch only). Law: CONVENTIONS
    "W1-PREP additions", "Battlefield & procedural terrain (M6.5)" traversability guarantee.

#### TASK-251 — Hill size variance: DA scale range + nav-Z cap check (art, editor/DA)
- assignee: art-director
- status: **done** (2026-07-22 — COMMITTED at the final overnight branch fold **`f49d8b1`** on m7.6-arena10x (rides the same DA save as TASK-249). ← was: **ready-for-integration** (2026-07-22 overnight — Hill layer ScaleRange 0.9–1.3 → **0.4–2.5**, count 6 → **8** (modest bump; single uniform range = giants stay rare by draw luck ~1–2/seed). NAV-Z CAP RECORDED: tallest donor SM_Hill_02 = 400 uu → 2.5× crown = 1,000 uu ≤ 1,200 nav-Z with 200 uu margin (ceiling would be 3.0×; 2.5 is the margin-safe cap) — NO nav-volume bump, NO nav rebuild owed. Verified same 3 seeds as TASK-249: Hill 8/8 fill every seed, Traversability CONFIRMED 0 culls with giants present, spread reads dramatic (0.4 bump → 2.5 landmark in one wide frame), elevated prop placement follows scale (live instance bodies). Residual: direct navmesh spot on a 2.5× crown not machine-checkable — indirect 0-cull pass ×3; Jonathan's W1 climb is the human check. Same DA save as 249. Captures in Cache/HillGrass/w1prep/. handoffs/TASK-251.md)
- blocked-by: none (independent of TASK-250; serialize with TASK-249 in the editor)
- parallel-safe: no (editor/DA — single editor)
- spec: >
    Widen the hill layer's scale range SUBSTANTIALLY per Jonathan: today 0.9–1.3 → target ≈0.4–2.5+ ("much bigger and much
    smaller"), art-director tunes the distribution (consider count/bands so giants stay rare and read as landmarks).
    CONSTRAINT (CONVENTIONS W1-PREP law): uniform scale preserves the ≤30° climbability (angles scale-invariant — good),
    BUT compute the tallest crown at max scale from the SM_Hill_01-03 heights and CAP max scale so it stays under the
    NavMeshBoundsVolume Z (±1,200) — OR bundle the nav-volume Z bump into this task (then a nav rebuild is OWED: editor
    Build > Navigation + resave, the MCP-no-nav-tool note). Re-seed several times; verify giants and minis both climb and
    that units path over them (spot navmesh coverage on the tallest crown). ACCEPTANCE: visibly dramatic size range in a
    re-seeded match, climbability intact, nav covers the tallest crown, DA saved; captures for Jonathan. Post in 🎨 Art.
- names: >
    DA_BattlefieldScatter hill layer (scale min/max, count/bands) + NavMeshBoundsVolume Z only if bumped. Law: CONVENTIONS
    "W1-PREP additions" (nav-Z cap), "Climbable terrain (M6.6)" (≤30° faces).

#### TASK-253 — [T-A] AGoldNode → neutral depleting claimable mine (C++, branch) [DISPATCH FIRST]
- assignee: gameplay-programmer
- status: done (2026-07-23 — INTEGRATED in THE W1 BUILD, branch commit bfa2ecf; batch compiled 0 err/0 warn, 9-point PIE + float-fix suite verified per handoffs/TASK-258.md) ← was: qa-passed (2026-07-22 — qa/TASK-253-qa.md PASS 0 blockers 0 warns; laws byte-faithful ×6, claim atomicity wedge-free (phantom ≤1s self-heal), Deplete re-entrancy structurally safe, all 5 deviations adjudicated ACCEPT (constexpr cadence = the STRONGER law reading), pinned contracts grep-verified exhaustive. 2 nits → merge-gate cross-note pairs the cadence; tier-2 phantom transient noted for 254's review. 254/255/256 clear to build)
- blocked-by: none
- parallel-safe: yes (GoldNode.{h,cpp} only)
- spec: >
    Per the plan §AGoldNode: REMOVE Team/GetTeam (all 3 callers rewritten in-feature — flag each); KEEP the standing laws
    (NoCollision, never affects nav, not damageable, no ITeamAgent, soft SM_GoldNode resolve). ADD: GoldReserve (default 300)
    + latched initial; DrainPerMinerPerSecond=1 (PAIRED-TUNABLE LAW ≡ ASiegePlayerState::MinerGoldPerTick — cross-note both
    headers); TOptional<ETeamId> OccupyingTeam; arrived-miner weak-ptr registry; bDepleted; 1 s drain timer while occupied
    (reserve −= n×1/s; at 0 → evict all via NotifyMineDepleted + broadcast + gauge dims). API: InitMine(Reserve),
    CanTeamMine, TryRegisterArrivedMiner (atomic claim on 0→1), UnregisterArrivedMiner (release on last), static
    FindBestMineFor(World, Team, From) — THE single finder (tier-1 nearest CanTeamMine; tier-2 nearest enemy-occupied
    non-depleted wait target; null = all depleted). Gauge: lazy MID on slot-0 M_GoldGlow driving GlowIntensity
    Lerp(0.05,1.0,Reserve/Initial) — intensity modulation, never material replacement. Delegates OnMineDepleted/
    OnMineReserveChanged (HUD backlog hook — nothing binds this pass). ACCEPTANCE: API complete + null-safe; laws held;
    compiles deferred to TASK-258. QA implied (LAWS-UNCHANGED review + shadow/include scans). Post in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/GoldNode.{h,cpp}` (branch). Law: CONVENTIONS "Mirrored depleting mines" (paired-
    tunable, gauge, laws-held), M7.6 ownership extension.

#### TASK-254 — [T-B] MinerUnit retarget / wait / evict (C++, branch)
- assignee: gameplay-programmer
- status: done (2026-07-23 — INTEGRATED in THE W1 BUILD, commit bfa2ecf; economy verified live in PIE: miner claim + drain 20→0 + depletion evict + retarget) ← was: qa-passed (2026-07-22 — qa/TASK-254-255-256-qa.md PASS 0 blockers; invariant table verified at every seam, no-churn upgrade-only ADJUDICATED FAITHFUL, phantom-window benign. Feeds TASK-258) ← was: ready-for-qa (2026-07-22 — MinerUnit.{h,cpp} reworked per plan §1b: FindNearestSameTeamGoldNode deleted → SeekBestMine over AGoldNode::FindBestMineFor; UpdateMining retarget gate (shared dead-target predicate, tier-1-upgrade-only no-churn rule) + at-ring TryRegisterArrivedMiner (success = M2 arrival block code-identical / failure = WAIT MODE at the ring with per-poll auto-claim retries) + finder-null idle; pinned `NotifyMineDepleted(AGoldNode*)` declared+implemented (per-tenure un-arrive, no mine call-back); EndPlay(Destroyed) unregisters from the mine BEFORE the player-state bookkeeping; lost-node Error → Log; FreezeAI untouched; NO compile (TASK-258 batch). Invariant table (R/A/I/M, every path) self-audited in handoffs/TASK-254.md)
- blocked-by: TASK-253 (API)
- parallel-safe: yes (MinerUnit.{h,cpp}; disjoint from 255/256)
- spec: >
    Per the plan §MinerUnit: FindNearestSameTeamGoldNode DIES → AGoldNode::FindBestMineFor (team filter gone; occupancy
    replaces it). UpdateMining poll (0.25 s cadence UNCHANGED): retarget gate (null/stale/depleted target, or a better
    tier-1 exists while un-arrived → re-find; NO churn between equal options); at ring → TryRegisterArrivedMiner — success =
    the existing arrival block VERBATIM; failure = WAIT MODE (stand at ring, poll retries, auto-claim when freed); finder
    null → idle-in-place + poll retry (all-depleted endgame = intended income death). NEW NotifyMineDepleted: un-arrive
    (RemoveMinerIncome + clear bArrivedAtNode — the one-way latch becomes PER-TENURE, header doc rewritten), clink off,
    null target → re-seek. EndPlay(Destroyed): unregister from mine BEFORE existing bookkeeping (income ⊆ alive invariant;
    no double-Remove after evict). Lost-node Error demoted to Log. FreezeAI untouched; post-match drain quirk accepted +
    commented. ACCEPTANCE: all transitions per spec; invariants documented at the seams. QA implied (BOOKKEEPING-INVARIANTS
    review). Post in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.{h,cpp}` (branch). Law: CONVENTIONS "Mirrored depleting mines", M6.5/
    M2 income laws (SetGold choke-point untouched).

#### TASK-255 — [T-C] Scatter mines pass: mirrored placement + hill parity + clearance + traversability (C++, branch)
- assignee: gameplay-programmer
- status: done (2026-07-23 — INTEGRATED in THE W1 BUILD, commit bfa2ecf; determinism/mirror/hill-parity/clearance all PASS in live PIE per handoffs/TASK-258.md) ← was: qa-passed (2026-07-22 — qa/TASK-254-255-256-qa.md PASS 0 blockers; draw-audit confirmed (2/attempt, zero elsewhere), cross-pair spacing PROVEN complete (opposite-sign X ⇒ |Xi|+|Xj|≥spacing ∀Y), reindex trap avoided via batch RemoveInstances, Hit.Item engine-confirmed; 1 nit = fallback slots unspaced vs drawn primaries (Error-flagged degenerate only). Feeds TASK-258) ← was: ready-for-qa (2026-07-22 — full §2 algorithm on the branch: Scatter|Mines config block (3/3000/600/300/600/30°/null⇒AGoldNode) + GoldNodeKeepClearRadius removed; RebuildKeepClearZones gold-node block DELETED incl. the ±24,200 phantom discs; PlaceMines(Seed) after pass-2/before nav poll on dedicated FRandomStream(Seed^0x4D494E45) — 2 draws/attempt X-then-Y, zero draws anywhere else; half-draw |X|≥max(600,spacing/2), spacing vs primaries (twin+cross covered by construction), keep-clear discs at P AND P′, NO corridor test (ruling), FindHillSurfaceAt (Hit.Item+comp; ResolveHillAwareGroundZ now delegates), 30° gate both points, hill parity via same-comp mirrored AddInstance + footprint un-bury + re-trace + reject-if-no-fit (clone rolled back), RemoveBlockingInstancesInDisc (nav-relevant, lockstep, HILLS EXEMPT, grass untouched) at both points, tracked SpawnActor pair + InitMine, ≤48 attempts then deterministic fallback slot (−13,000, {−3,000,0,+3,000}) w/ Error log, MinesPass reproducibility line; ClearScatter destroys SpawnedMines; ValidateTraversability: Blue→each-mine path checks + widening per-mine disc culls in the existing attempts machinery + RegroundMines after EVERY cull. NO compile (TASK-258 batch). Draw-sequence table + 10 adjudicated deviations in handoffs/TASK-255.md)
- blocked-by: TASK-253 (API), TASK-250 landed (same files — serialize; builds ON ResolveHillAwareGroundZ)
- parallel-safe: yes (ScatterConfig.h + BattlefieldScatter.{h,cpp}; disjoint from 254/256)
- spec: >
    Per the plan §Scatter (first SpawnActor capability): config block Scatter|Mines (MineCountPerSide=3, MineMinSpacing=3000,
    MineClearanceRadius=600, MineGoldReserve=300, MineEdgeMargin=600, MineMaxSlopeDeg=30, MineClass null⇒AGoldNode); REMOVE
    GoldNodeKeepClearRadius; RebuildKeepClearZones: DELETE the whole gold-node block incl. the hardcoded ±24,200 fallback
    discs (phantom-disc trap) → keep-clears = castles + PlayerStart only. PlaceMines(Seed) after pass-2, BEFORE
    StartNavSettlePoll (injected hills must carve nav pre-validation); DEDICATED FRandomStream(Seed XOR 0x4D494E45), all
    draws in fixed order (seed-order law). Per mine (≤2× attempts then deterministic fallback slot — the economy NEVER
    ships short): draw on the Blue half |X|≥1,500; spacing vs prior primaries; keep-clear discs tested at BOTH P and P′
    (PlayerStart isn't mirrored); NO corridor test (ruling — NoCollision keeps traversability safe). Hill resolve both
    points via NEW FindHillSurfaceAt (extends ResolveHillAwareGroundZ to surface Hit.Item + component); slope ≤30°. HILL
    PARITY (ruling either⇒both): clone the hill instance transform mirrored (−x, yaw+180) onto the SAME HISM (auto-registers
    as hill surface), clearance-delete blockers in the clone's footprint (un-bury), re-trace; reject the candidate if the
    clone can't fit its keep-clear disc. Clearance-delete via NEW RemoveBlockingInstancesInDisc (disc sibling of
    CullCorridorBlockers — nav-relevant comps, visual+proxy lockstep, HILL-SURFACE COMPS EXEMPT, grass untouched) at both
    points r=600. Spawn the tracked AGoldNode PAIR + InitMine(300); log a reproducibility line (seed + pairs + onHill).
    ClearScatter destroys SpawnedMines (Play-Again lifecycle). ValidateTraversability extension: path-query Blue anchor →
    each mine; failure → widening clearance cull per the existing MaxReachabilityAttempts machinery; RegroundMines() after
    EVERY defensive cull (no floating mines). Do NOT collide with the pending Phase-1 cull-field additions. ACCEPTANCE:
    deterministic (same seed ⇒ identical MinesPass log), exact mirror pairs, parity + clearance + reachability logic per
    spec. QA implied (DETERMINISM review: every draw from MineStream, fixed order). Post in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h`, `BattlefieldScatter.{h,cpp}` (branch). Law: CONVENTIONS
    "Mirrored depleting mines" (stream, defaults), "Battlefield & procedural terrain (M6.5)" traversability guarantee.

#### TASK-256 — [T-D] Bot: mine-aware economy rules (C++, branch)
- assignee: gameplay-programmer
- status: done (2026-07-23 — INTEGRATED in THE W1 BUILD, commit bfa2ecf; bot Rule 2 miner-buys anchored on FindBestMine observed live, target 3) ← was: qa-passed (2026-07-22 — qa/TASK-254-255-256-qa.md PASS 0 blockers; pre-clamp deviation RATIFIED as necessary correction (ComputeValidBotSpawnPoint rejects-not-clamps — unclamped would stall 2a forever), rules 1/3-5 byte-identical, 216/252 state undisturbed. Feeds TASK-258) ← was: ready-for-qa (2026-07-22 — rules 2a/2b anchored on FindBestMineFor; GetGoldNodeRedLocation + GoldNodeRedFallbackLocation deleted; null-finder ⇒ 2a skipped, latch-logged once per state change; handoffs/TASK-256.md)
- blocked-by: TASK-253 (API)
- parallel-safe: yes (SiegeBotController.{h,cpp}; disjoint from 254/255 — TASK-252's BotDecks edit is committed, no overlap conflict)
- spec: >
    Per the plan §Bot: DELETE GetGoldNodeRedLocation + GoldNodeRedFallbackLocation. Rules 2a/2b anchor on
    FindBestMineFor(BotTeam, castle); FINDER NULL ⇒ SKIP rule 2a (never buy a doomed miner); 2b falls back to castle-offset.
    Spawn point = mine + approach offset via the existing own-half clamp (cross-field walks are CORRECT behavior — playtest
    watch, not a bug). Comment sweep for the dead constants. ACCEPTANCE: bot buys miners only when a mine is reachable,
    anchors on the finder, LogSiegeBot lines intact. QA implied. Post in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.{h,cpp}` (branch). Law: CONVENTIONS "Mirrored depleting mines",
    M3 bot ordered-rules law (one LogSiegeBot line per fired rule).

#### TASK-257 — [T-E] Editor: TeamFill lights ×2 + castle nodes ×2 DELETED, M_GoldGlow GlowIntensity, DA Mines block (art, editor, branch)
- assignee: art-director
- status: done (2026-07-23 — DA population COMPLETED inside the TASK-258 window: after the batch compile + editor relaunch, DA_BattlefieldScatter Scatter|Mines block written + confirmed field-by-field via MCP ObjectTools [MineCountPerSide 3, MineMinSpacing 3000, MineClearanceRadius 600, MineGoldReserve 300, MineEdgeMargin 600, MineMaxSlopeDeg 30, MineClass None⇒AGoldNode], saved not-dirty. Pre-compile parts had LANDED on the branch 2026-07-22: TeamFill_Cool_Blue/TeamFill_Warm_Red (RectLight_0/1) + GoldNode_Blue/Red (GoldNode_0/1, ±24,200) DELETED from L_Arena, find_actors sweep = 0 matches, L_Arena saved not-dirty; M_GoldGlow `GlowIntensity` authored per the 253 flag as a NEW default-1.0 scale — EmissiveColor ← NEW Multiply_3(A: original Multiply_2 chain untouched, B: ScalarParameter GlowIntensity=1.0), stock nodes, recompiled clean, neutrality verified, saved not-dirty; before/after captures in Tools/ArtPipeline/Cache/TASK-257/) ← was: done-pending-DA-handover ← was: backlog
- blocked-by: TASK-255 (config shape final — the DA half needs the compiled fields), editor+MCP up; coordinate with Jonathan (active)
- parallel-safe: no (editor; the deletions + material param may PRE-RUN before the compile — the DA population happens post-compile inside TASK-258's window)
- spec: >
    Branch lane, three parts — MUST land with-or-before the integration compile (NEVER ship the 8-node hybrid economy):
    (1) L_Arena: DELETE TeamFill_Cool_Blue + TeamFill_Warm_Red (RectLight_0/1 — team mood lighting removed outright) and
    DELETE GoldNode_Blue + GoldNode_Red (same session as the C++ integration). (2) M_GoldGlow: add scalar param
    `GlowIntensity`, DEFAULT = the current emissive multiplier (undriven ⇒ byte-identical look; stock node, seconds compile;
    Custom-HLSL ban; bUsedWithInstancedStaticMeshes check N/A — actor mesh, not HISM). (3) DA_BattlefieldScatter: populate
    the Scatter|Mines block per the CONVENTIONS defaults (3/3000/600/300/600/30°/null class) AFTER TASK-258's compile makes
    the fields visible — coordinate the same editor session. ACCEPTANCE: lights + castle nodes gone, param default-neutral,
    DA block populated post-compile, all saved not-dirty. Post in 🎨 Art.
- names: >
    `/Game/Maps/L_Arena` (RectLight_0/1 + GoldNode_Blue/Red DELETIONS), `/Game/Materials/M_GoldGlow` (GlowIntensity),
    DA_BattlefieldScatter Mines block. Law: CONVENTIONS "Mirrored depleting mines", M7.6 ownership (all four targets are
    branch-owned), Custom-HLSL ban.

#### TASK-258 — [T-F] Mines integration: compile + 9-point PIE suite + branch commit = THE W1 BUILD (build)
- assignee: build-master
- status: done (2026-07-23 — THE W1 BUILD shipped: branch commit bfa2ecf (NOT pushed). Compile 0 err/0 warn; DA Mines block populated+saved; 9-point PIE + TASK-259 float-fix suite results in handoffs/TASK-258.md — determinism/mirror/hill-parity/clearance/depletion/float-fix all PASS, points 7 (all-depleted endgame) + contested/HUD-gauge + in-match Play-Again button deferred to Jonathan's live W1 (TASK-219 gate). Watches: hill-twin height asymmetry, bot all-attack openings, Ogre-near-hill capsule lift, M7 miner art debt. Editor left RUNNING on L_Arena.) ← was: backlog
- blocked-by: TASK-253, 254, 255, 256, TASK-259 (all qa-passed), TASK-257 (deletions + param landed; DA populated inside this window)
- parallel-safe: no (single editor + Git; branch direct-commit lane)
- spec: >
    Compile the mines batch on the branch (editor-bounce; TASK-257's DA population slots after the compile, same window —
    also clear the standing PRE-W1 BOUNCE CHECKLIST items: stray M_T245_WpoTest.uasset disk-delete + the TASK-247/248 docs
    fold if still pending). Then the 9-POINT PIE SUITE (plan §T-F, all must pass): (1) determinism — same seed ⇒ identical
    MinesPass log ×2; (2) mirror fairness — every pair (−X,Y), equal distances from logs; (3) hill parity — seed-hunt an
    on-hill mine, twin hill injected, nothing buried; (4) clearance — no blockers within 600, miners arrive at all 6;
    (5) occupancy — claim / wait-at-ring / auto-claim-on-free; (6) depletion — low-reserve override: drain → evict →
    un-arrive on HUD rate → retarget → gauge dims; (7) all-depleted endgame winnable (base + overtime + DeepMine);
    (8) Play-Again ×3 — exactly 6 fresh mines, zero leaks, no phantom keep-clear discs; (9) bot reaches 3 arrived miners on
    3 seeds + stops buying when locked out (cross-field walks = correct, watch-listed). COMMIT on the branch (push-pending —
    Jonathan decides; DO NOT push). THIS IS THE W1 BUILD (hills + mines + no team lighting) — hand to TASK-219 with the
    watch list (mine-lock, cross-field walks, all-depleted pacing → reserve 450 lever). Post results + hash in 🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x` compile + commit. Law: CLAUDE.md hard gates, CONVENTIONS "Mirrored depleting mines", W-gate law
    (TASK-219 = the acceptance gate).

#### TASK-259 — [W1 BLOCKER] Floating-units fix: SkeletalVisualMesh Z-offset in ResolveSkeletalVisual (C++, branch)
- assignee: gameplay-programmer
- status: done (2026-07-23 — INTEGRATED in THE W1 BUILD, commit bfa2ecf; VERIFIED live in PIE: Ogre SkeletalVisualMesh.Z −145 feet@z1.99, Archer −90 feet@z2.28, Knight/Cleric roster spot-sweep grounded) ← was: qa-passed (2026-07-23 — qa/TASK-259-qa.md PASS 0 blockers; cache-ordering verified across all spawn paths, no-op-for-9 structurally guaranteed (feet-at-pivot law + measured equality), Archer 0→-90 / Ogre 0→-145 correct. NIT: TASK-258 PIE spot-sweeps full-roster grounding. Feeds/unblocks TASK-258)
- blocked-by: none (independent one-liner; finishes-alongside TASK-258 so the W1 build carries it)
- parallel-safe: yes (SummonedUnit.cpp — coordinate ordering with any other SummonedUnit toucher; TASK-254 is MinerUnit, disjoint)
- spec: >
    Fix per handoffs/DIAG-floating-units.md SESSION-2: `ASummonedUnit::ResolveSkeletalVisual` never sets the
    SkeletalVisualMesh Z-offset — it trusted per-unit BP authoring from TASK-159, so Archer + Ogre (first-imported later in
    TASK-242/243 with ZERO BP changes) kept Z=0 and their mesh floats one capsule-half above the grounded capsule (Archer
    +90, Ogre +145). FIX: one-line `SetRelativeLocation(VisualMeshBaseRelativeLocation)` on SkeletalVisualMesh AFTER the SK
    swap — corrects both, NO-OP for the 9 already-correct units (they authored the same offset), closes the recurrence trap
    permanently (future first-imports need no BP authoring). Does NOT address the SECONDARY Ogre-near-hill spawn-lift (see
    the watch below — separate item, not this change). ACCEPTANCE: Archer + Ogre feet meet the ground in PIE; the 9 correct
    units byte-unchanged in position; no crash on the static-fallback path. QA implied (confirm no-op for the authored-offset
    units; shadow/include scans). Compile rides TASK-258. Post in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` (`ResolveSkeletalVisual`, branch — JOINS the branch touched-
    files set for the merge gate). Law: CONVENTIONS "Skeletal rig & animation workstream (M7)" (SkeletalVisualMesh swap
    contract), M7.6 ownership.

**WATCH (recorded 2026-07-22, follow-up to TASK-259 — separate item, NOT fixed by the mesh Z fix):** the SECONDARY Ogre-near-hill spawn-lift (DIAG-floating-units.md SESSION-1 Rank-2) — nav-projection/collision-adjust raising the big Ogre CAPSULE (not the mesh) when it spawns near a hill. The TASK-259 mesh fix removes the dominant +145 mesh offset; this residual capsule lift is a smaller, separate item. If it reads badly at W1 → register a gameplay-programmer task (spawn nav-projection extent / collision-adjust tuning for tall capsules); until then it's a watch, not a task. **UPDATE 2026-07-23:** TASK-265 (appendix 3a) attacks the same class of issue from the stacking side and adds the `bLogSpawnZDiagnostic` chosen-Z/ground-Z/delta line — TASK-266's PIE evidence is what decides whether ANY residual capsule lift remains on this watch. A residual traced to the shared `SpawnUnitSwarm` lift or to the tall-Ogre capsule stays HERE (out of TASK-265's scope by spec).

#### TASK-264 — [S-E] Integration: compile batch + DELETE centerline actor + place CaptureZone_Center + PIE capture-suite + branch commit (build)
- assignee: build-master
- status: **done** (2026-07-23, build-master — compile GREEN (13.46 s, editor bounce cleared the Live Coding lock); `CenterlineMarker` (DecalActor_0, M_CenterlineStripe) DELETED from L_Arena — 0 DecalActors remain; `CaptureZone_Center` placed @ (0,0,0), ZoneHalfExtent (840,840), decal MID resolves M_CaptureZone gray (0.5,0.5,0.5). PIE suite: (c) Blue-alone→Blue + decal (0.05,0.30,1.00) PASS; (d) contested→**Neutral** + gray PASS (confirms the 2026-07-23 neutralize ruling); (f) Red-alone→Red + decal (1.00,0.10,0.05) PASS, and the bot demonstrably staged 2 units mid-field only while Red held the zone; (b) bot SpawnBoxHalfExtent 840 live, new spawns cluster X 24032–24448 inside the Castle_Red box PASS; (a) player SpawnBoxHalfExtent 840 live + PlacementMaxX fully retired, ring = 420-uu-wide flat band at Z=0 (non-empty, off-plinth, ~2.12 M uu²) — live click-refusal is a W1 WATCH (locked desktop, no SendInput); (e) Neutral state observed twice, gate is `IsPointInZone && CaptureOwner==TeamToState(Team)` which never matches Neutral (structural); (g) 3× fresh match starts all read Neutral + gray (no leak) — the in-place PlayAgain button press is a W1 WATCH (no MCP console/exec route). L_Arena saved not-dirty. FOLLOW-UPS RAISED: bot units stacking/floating ~215–232 uu above ground at the Red spawn edge (TASK-216's 1,500–2,000 castle-relative attack spawn vs TASK-262's 840 box — the wave gets clamped to the box edge and piles up).)
- blocked-by: TASK-260, TASK-261, TASK-262 (all qa-passed), TASK-263 (done)
- parallel-safe: no (single editor + Git; branch direct-commit lane)
- spec: >
    On m7.6-arena10x: (1) COMPILE the spawn-box/capture batch (editor-bounce). (2) WHITE-LINE REMOVAL — locate the L_Arena
    level actor whose material is `M_CenterlineStripe` (a decal/plane; grep of Content confirms it is NOT C++) via
    find_actors / material reference, and DELETE it; verify the centerline is gone in a PIE frame and L_Arena saves
    not-dirty. (3) PLACE the `ACaptureZone` instance `CaptureZone_Center` at (0,0,0) in L_Arena (default ZoneHalfExtent);
    confirm its decal reads at origin. (4) PIE CAPTURE-SUITE (all must pass): (a) player spawn shrunk — can place inside the
    Blue castle box, REFUSED outside it and on the plinth, the old whole-half is gone; (b) bot spawn shrunk — mirror around
    Castle_Red; (c) capture — send ONE Blue unit into the mid zone with zero Red units ⇒ CaptureOwner→Blue, decal turns
    blue, player can now place in mid; (d) contested — Blue+Red units both inside ⇒ CaptureOwner→Neutral, decal turns gray, nobody can place in mid until re-captured (per Jonathan's 2026-07-23 ruling; the old "sticky/unchanged" assertion is retired); (e) neutral
    ⇒ nobody can place in mid; (f) Red mirror — bot captures + stages in mid; (g) Play-Again ×3 ⇒ CaptureOwner resets to
    Neutral, no leaks. COMMIT on the branch (push-pending — Jonathan decides; DO NOT push). This build folds into the
    pre-W1 bounce ahead of Jonathan's W1 look (TASK-219 = acceptance gate). Post results + hash in 🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x` compile + commit; `L_Arena` (centerline actor DELETE + `CaptureZone_Center` place). Law:
    CLAUDE.md hard gates, CONVENTIONS "W1-PREP additions 3", W-gate law (TASK-219 = the acceptance gate).

#### TASK-266 — [S3a-B] Integration: compile + PIE spawn re-verify + branch commit (build)
- assignee: build-master
- status: **done** (2026-07-23, build-master — commit `1e4bd19` on `m7.6-arena10x`, NOT pushed. Compile GREEN (13.5 s, `Result: Succeeded`); `git diff --stat` confirms ONLY `SiegeBotController.{h,cpp}` changed (QA note 2 satisfied). PIE re-verified over **4 matches** on L_Arena with `LogGitClaudeUnrealTest` + `LogSiegeBot` raised to **Verbose** BEFORE PIE (QA WARN-3). **(a) NO STACK — PASS:** 11 consecutive spawn anchors in one match spread across Y ∈ [−800, +698] all at X = 24 200 (vs the pre-fix single pinned point); 29 concurrent units sampled, **ZERO identical-XY pairs** — the pre-fix signature (two units at exactly (24192, −192)) is gone. Min pairwise 2D = 60.12 uu, and that pair is two members of the SAME `MilitiaMob x4` swarm clumped in melee at the enemy castle — intra-swarm placement is `SpawnUnitSwarm`'s business (off-limits to TASK-265), not a bot spawn-point collision; no two independently-spawned units were ever co-located. **(b) NO FLOAT — PASS, instant AND settled (QA WARN-1 satisfied):** every `[SpawnZ]` line read `chosen Z 20.0, ground Z 0.0 (trace hit), delta 20.0, FLOAT above ground **2.1**`; an INDEPENDENT settled-state readback (own downward ground traces, minutes after spawn, at marched-to positions far from spawn) gives capsule bottoms **2.13–2.15 uu** above traced ground, and actor Z = capsule half-height + 2.15 for all 7 unit types (MilitiaMob 76.65/74.5, Sapper 86.65/84.5, Miner 88.65/86.5, Archer+Footman 92.15/90.0, Knight+Pikeman 97.15/95.0, Cavalry 106.15/104.0). 2.1 uu is CharacterMovement's standard ground offset. **Pre-fix 215–232 uu ⇒ CLOSED.** No hill-adjacent false positive seen (all traces hit the flat floor at Z = 0.0). **(c) MARCH — PASS:** rule 4 fired 15+ times, always `castle-front (24200, …)` — inside the box [24 160, 25 840], 800 uu from castle centre so off the 420 plinth; units crossed the full ~49 000-uu arena and destroyed Castle_Blue every match. NO attack unit ever spawned at mid (the QA-defined defect signature). **(d) ECONOMY — PASS (headline):** `Rule 2 (Economy): played Miner 'Miner' (cost 8) toward mine 'GoldNode_3' at (24200, -800, 20) — miners now 1/3, gold 10->2.` and rule 4 fired TWICE AFTER it in the same match ⇒ ladder no longer stalls. With Verbose live, **ZERO** `found no valid spawn point` lines across all 4 matches (all 6 Verbose failure sites silent) ⇒ no spawn attempt failed at all, so the mine-Z vs `NavProjectionExtent.Z`=1 000 suspect never triggered. **(e) MID-ZONE — precondition PROVEN, live mid-spawn NOT observed:** `CaptureZone_Center` reached `CaptureOwner: Red` in 2 of 4 matches (Red units marching through mid capture it) and latches when empty; but no rule-2 economy play happened to fire during Red ownership, so a mid-field bot spawn was not directly seen. The clamp's INERT path WAS observed and is correct — at t ≈ 1 s the zone was Neutral and the rule-2 miner anchor clamped to the castle box at (24 200, −800), byte-identical to the literal spec, exactly as QA predicted. No Deep Mine at mid (the bot never drew/played one). Not a defect — needs a live opponent to exercise. **(f) PLAYER — PASS:** `SiegePlayerController.{h,cpp}` untouched per the diff; TASK-261 castle-box behavior unregressed. **(g) TRACE INTACT:** every `LogSiegeBot` line is one per FIRED rule, formats unchanged; the new diagnostic is on `LogGitClaudeUnrealTest` only. **NEW W1 WATCH (not a defect, out of TASK-265 scope) — BOT IDLES ON A BIG GOLD BANK:** against a fully passive opponent the bot banked **328 gold** while playing nothing for ~3.5 min. Root cause read directly off the live `DeckComponent`: its hand was `[Wall, BombTower, CrystalTower, Wall, ArrowTower, BallistaTower]` — **six buildings, zero units/miners/spells** — so rule 1 has no intruder to defend against, rules 2/3/4 have no eligible card, and rule 5 never cleared the dead hand. Deck-composition/rule-gating behavior of the 'Bot Defensive Economy' deck, NOT a spawn defect (proven by the zero Verbose spawn-failure lines). Should self-resolve when Jonathan actually attacks (rule 1 then drains the towers), but if the W1 bot ever looks asleep, THIS is why — candidate follow-up: let rule 5 discard a hand with no playable card.)
- blocked-by: TASK-265 (must be qa-passed)
- parallel-safe: no (single editor + Git; branch direct-commit lane)
- spec: >
    On `m7.6-arena10x`: (1) COMPILE (editor bounce as usual). (2) PIE RE-VERIFY — the TASK-264 follow-up, all must pass:
    (a) NO STACK — log/readback ≥ 6 consecutive bot unit spawns, no two within `UnitSpawnClearance` (150) 2D, no identical XY;
    (b) NO FLOAT — spawned bot units sit on the ground; report the `bLogSpawnZDiagnostic` chosen-Z / ground-Z / delta values
    verbatim in the handoff (this is the evidence that closes the ~215–232 uu float, and it feeds the Ogre-near-hill WATCH if
    a residual remains); (c) IN-BOX — every new bot spawn X is inside [Castle_Red.X − 840, Castle_Red.X + 840] and off the
    plinth, and the units MARCH out (not idle); (d) ECONOMY RESTORED — grep `LogSiegeBot` for a "Rule 2 (Economy)" line
    (miner and/or Deep Mine) within a couple of minutes of match start, and confirm rules 3/4/5 still fire (the ladder no
    longer stalls); (e) NO REGRESSION of TASK-264 — the player can still place only inside the Blue castle box, and with Red
    holding `CaptureZone_Center` the bot still stages mid-field; (f) decision trace intact — `LogSiegeBot` lines are one per
    FIRED rule, format unchanged; (g) **WATCH — report, do NOT fail the task on it (added 2026-07-23, QA WARN-4 / FLAGGED item
    (iv)):** with Red HOLDING the zone, rule 2b may now build a Deep Mine AT MID. If one appears, note it in the handoff and say
    whether it visibly obstructs traffic through the centre corridor. Also per QA: an *attack* unit (rule 1 / rule 4) spawning at
    mid IS a defect — report that one. (3) COMMIT on the branch (DO NOT push — Jonathan decides). If any check fails, append to the
    QA report and route back per CLAUDE.md rule 6. This is the last build before Jonathan's W1 look (TASK-219 = the
    acceptance gate). Post results + hash in 🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x` compile + commit. Law: CLAUDE.md hard gates, CONVENTIONS "W1-PREP additions 3" appendix 3a,
    W-gate law (TASK-219 = the acceptance gate).

**FLAGGED for Jonathan (defaults chosen, nothing blocks):** (i) the bot wave now starts ~1,750 uu further back (inside its box) — ~4 s more march on a 45,000-uu field, and it is the same rule the player lives under; (ii) `BotCastleSpawnOffset` 1,750 becomes inert while the box is 840 — the property is KEPT so ruling #1's knob returns intact if the box grows; (iii) if he'd rather the waves keep the forward start, the honest lever is a BIGGER box (`SpawnBoxHalfExtent`, still FLAGGED tunable from TASK-262) which grows the PLAYER box symmetrically — a balance change, his call, not shipped by default; (iv) **AMENDED 2026-07-23 (QA WARN-4 — the original wording is conditionally FALSE as shipped):** Deep Mines build inside the castle box *while Red holds no capture zone* — but when Red **OWNS** `CaptureZone_Center`, rule 2b may instead build the Deep Mine **AT MID**, inside the zone (buildings are exempt from `UnitSpawnClearance` but NOT from the anchor clamp, and a held zone is an eligible spawn region under the accepted deviation). The castle-box case remains mechanically irrelevant (a Deep Mine needs no mine). **Manager balance verdict on the MID case: not a balance concern, and NOT a W1 blocker — but a W1 WATCH, not "irrelevant".** Reasoning: (1) it moves value in the PLAYER's favor — an unshielded bot building at the centerline is raidable by anything the player fields, whereas the castle-box version sits behind the bot's whole army; (2) it cannot lock the objective — capture eval counts UNITS only (`ASummonedUnit`/`AHeroCharacter`; buildings/castles/gold-nodes EXCLUDED, TASK-260 law), so a Deep Mine at mid contributes ZERO to holding the zone, and it does not persist ownership if the bot's units leave; (3) it is design-consistent (a raidable mid-field economy is exactly the tension the capture zone exists to create). Two things worth Jonathan's eyes at W1, both non-blocking: **(a) legibility** — an enemy building materializing at the dead centre of the map is surprising the first time and he should see it deliberately rather than as a bug report; **(b) nav pinch** — a building footprint inside the ±840 zone sits in the middle of the 1,000-half-width centre corridor; the arena is wide enough that units should path around, but build-master should note in TASK-266 whether a mid Deep Mine visibly obstructs traffic. No code change requested; if Jonathan dislikes it, the honest lever is a building-only exclusion in the clamp (≈5 lines) — do NOT pre-emptively add it. (v) the W1 bot will play a visibly DIFFERENT (more economic) game once rules 2a/2b work again.

#### TASK-267 — [S3a-C] Bot ordered-rules ladder: a failed rule-2 spawn must FALL THROUGH, not abandon the tick (C++, robustness — NOT a W1 blocker)
- assignee: gameplay-programmer
- status: done (INTEGRATED at TASK-283 batch, build-master 2026-07-24, commit on m7.6-arena10x — compile GREEN; PIE: bot reached + fired Rule 4 (Knight, gold 36→18), i.e. rules 1/2a/2b/3 fell THROUGH to rule 4 with no tick-abandon / ladder-stall (the fix); clean load. --- Prior QA 2026-07-24 PASS, 0 blockers / 3 non-blocking NITs, `qa/TASK-267.md`. Double-spend check CLEAN — traced selectors vs cards.csv: no failure path mutates state, rule-1 fall-through leaves rule-2 skipped [NearestIntruder guard], rule-4 fall-through only cycles unplayable TYPES so a failed Unit is never re-picked, a failed Miner [Economy] is unselectable by 4/5/2b; ≤1 confirm/tick. Latch + priority order + fired-rule trace preserved; TASK-265/262/279 untouched. AWAITS the next SiegeBotController-touching build [batch with TASK-283]. Original note: 2026-07-24 — gameplay-programmer: rule-2a/2b spawn-failure `return`s replaced with FALL-THROUGH; `bRule2SpawnFailureLogged` streak latch added [set on first rule-2 spawn failure, cleared on a successful rule-2 spawn AND in ResetBot], the two rule-2 failure lines promoted Verbose→Log. AUDIT: rules 1 & 4 shared the IDENTICAL `ComputeValidBotSpawnPoint`-else abandon-the-tick pattern and were fixed the same way [fall-through, no double-spend — their Verbose logs left as-is per the rule-2-only log-promotion scope]; rule 3 [spell ResolveSpell refusal] and rule 5 [last rule] do NOT share the trap — reported, not touched. File-only, no compile, no Git. Handoff: `.claude/pipeline/handoffs/TASK-267.md`. Was: UNBLOCKED/dispatchable.)
- blocked-by: none — **both gates CLEARED 2026-07-24** (TASK-266 done + committed @ `1e4bd19`; Jonathan's W1 sign-off GIVEN, TASK-219). NOTE: sole owner of `SiegeBotController.{h,cpp}` when it runs — do not dispatch concurrently with any other SiegeBotController-touching task.
- parallel-safe: no (sole owner of `SiegeBotController.{h,cpp}` when it runs)
- origin: QA WARN-2 on TASK-265 (`qa/TASK-265.md`), which explicitly ruled this OUT of TASK-265 scope ("it changes rule-flow semantics and needs a ruling") and asked the manager to open a follow-up so it is not lost. Independently confirmed by the programmer in `handoffs/TASK-265.md`.
- spec: >
    **The latent defect.** The `return` at `SiegeBotController.cpp:581` (and its rule-2b mirror at `:642`) sits **OUTSIDE** the
    `ComputeValidBotSpawnPoint` success branch: when the spawn point cannot be found, rule 2 returns from the decision tick
    entirely instead of falling through to the remaining rules. Because `ConfirmPlayFromHand` never runs, the Miner stays in
    hand ⇒ `AliveMinerCount` stays 0 ⇒ rule 2's own precondition stays satisfied ⇒ it re-fails and re-returns every tick.
    **The stall is PERMANENT for the rest of the match — rules 3/4/5 are never evaluated again.** TASK-265 removed today's
    *cause* (out-of-box anchors) but NOT this *structure*: any FUTURE spawn failure — a box crowded past the new 150-uu
    `UnitSpawnClearance`, a nav-projection miss (e.g. the mine-Z vs `NavProjectionExtent.Z`=1,000 suspect on TASK-266), a
    future box/geometry change — re-stalls the entire ladder by the identical mechanism, and it presents as "the bot stopped
    playing" with no error.
    **MANAGER RULING (made here so this needs no second round-trip): FALL THROUGH.** On a rule-2a/2b spawn failure the bot
    logs and CONTINUES to the next rule in the ordered ladder in the same tick. No gold is spent and no card is confirmed on
    the failure path, so no state is mutated — the change strictly ADDS reachable behavior and cannot make the bot play worse.
    (1) Replace the two early `return`s with fall-through control flow. Do NOT reorder, merge or re-prioritize the rules —
    the M3 ordered-rules priority is unchanged; only the abandon-the-tick behavior goes.
    (2) **Decision-trace law is inviolate:** a rule that attempted and FAILED to spawn did NOT fire, so it must NOT emit a
    `LogSiegeBot` fired-rule line, and the surviving one-line-per-FIRED-rule format stays byte-unchanged. The rule that
    subsequently fires in the same tick emits its normal single line.
    (3) **Kill the silent failure (QA WARN-3):** the two failure lines at `:577–579` / `:638–640` currently log at `Verbose`
    on `LogGitClaudeUnrealTest` — invisible at default verbosity. Promote them to `Log`, but emit at most ONCE per contiguous
    failure streak via a private transient bool `bRule2SpawnFailureLogged` (set on the first failure, cleared on the next
    successful rule-2 spawn AND on match reset) so a persistent failure cannot spam the 2-s decision cadence.
    (4) **AUDIT, then report before fixing beyond rule 2:** check rules 1, 3, 4 and 5 for the same
    return-outside-the-success-branch shape. Rule 2 is in scope unconditionally; fix any identical instance found in the
    others under the same ruling, and LIST every site you changed in the handoff. If a rule's early return turns out to be
    load-bearing (i.e. continuing would double-spend or double-play), STOP and report — do not force the pattern.
    NOT IN SCOPE: the anchor clamp / `UnitSpawnClearance` / spawn-Z work from TASK-265 (shipped), `CaptureZone.{h,cpp}`,
    `SiegePlayerController.{h,cpp}`, card costs, the rule priority order itself.
    ACCEPTANCE: with rule-2 spawning artificially forced to fail (e.g. `UnitSpawnClearance` set absurdly high in PIE), the bot
    still evaluates and fires rules 3/4/5 and keeps playing for the rest of the match; the `LogSiegeBot` decision trace is
    one line per FIRED rule with the format byte-unchanged; the rule-2 failure line is visible at default verbosity and
    appears once per streak, not once per tick. QA implied (rule-flow diff, decision-trace law, no state mutation on the
    failure path). Post in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.{h,cpp}`. ONE new identifier, EXACT:
    `bRule2SpawnFailureLogged` (private transient bool, NOT a UPROPERTY, not editor-exposed). Existing names consumed as-is:
    `ComputeValidBotSpawnPoint`, `ConfirmPlayFromHand`, `AliveMinerCount`, `LogSiegeBot`, `LogGitClaudeUnrealTest`.
    Law: M3 bot ordered-rules law (priority order UNCHANGED, one-line-per-FIRED-rule decision trace inviolate), CONVENTIONS
    "W1-PREP additions 3" appendix 3a, M7.6 branch-ownership extension.
- integration: NO dedicated build task is opened now. When TASK-267 is dispatched it FOLDS INTO the next
  `SiegeBotController`-touching branch integration task, which the manager issues at that time (this is the explicit
  accounting for the "every chain ends with a build-master task" rule — the chain is deliberately left open until dispatch).

#### TASK-214 — BRANCH FIRST: cut m7.6-arena10x off main before any batch file change (build)
- assignee: build-master
- status: **done** (2026-07-18 — branch `m7.6-arena10x` cut at main HEAD `a33aba6` via pure ref creation, no checkout; working tree verified untouched; lane strategy in handoffs/TASK-214.md)
- blocked-by: none
- parallel-safe: no (Git)
- spec: >
    Cut branch `m7.6-arena10x` off current `main` HEAD (VERIFY git state first — Jonathan self-commits; record the base hash).
    NO file changes in this task. Record in the handoff: branch base hash + the ownership law (branch exclusively owns
    L_Arena.umap + DA_BattlefieldScatter; M7.5 never touches them; branch = rollback). Do NOT push. ACCEPTANCE: branch exists
    locally at the recorded base; main untouched. Post base hash in 🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x` (manager relabel of the plan's `m8-arena10x`). Law: CONVENTIONS "Arena 10× scale-up & LOD/perf
    (M7.6)" (branch ownership), milestone-branch workflow memory.

#### TASK-215 — CONVENTIONS: M7.6 law block (Nanite amendment, LOD law, cull-field naming, W-gate + branch ownership) (manager)
- assignee: manager
- status: **done** (2026-07-18 — CONVENTIONS "Arena 10× scale-up & LOD/perf (M7.6)" section written + live: Nanite vista-only amendment (ruling #4), classic-LOD law, TASK-220 sequencing law, FScatterLayer CullStartDistance/CullEndDistance/bCastShadows naming, branch-ownership + W-gate laws. This decomposition is the deliverable.)
- blocked-by: none
- parallel-safe: yes
- spec: >
    Write the M7.6 law before task issue (done): scoped Nanite exception, lod_group/castle-reduction/SK-LOD/URO law, scatter
    cull-field names, branch ownership, W-gate law, cross-batch sequencing law.
- names: >
    CONVENTIONS.md "Arena 10× scale-up & LOD/perf (M7.6)".

#### TASK-218 — Phase-0 integration: compile + L_Arena 10× actor pass + nav rebuild + DA extents + PIE sanity + branch commit (build)
- assignee: build-master
- status: **done** (2026-07-19 — commit `42011de` on `m7.6-arena10x`; compile clean 20.3s; §1b actor pass exact w/ readback; Recast actor==ini verified; FULL nav rebuild baked into L_Arena (umap 122KB→292KB) after fixing the MCP transform/PostEditChange nav-bounds gotcha (see handoffs/TASK-218.md §3); DA (26k,12k)/1000/1500-600-800, densities untouched; PIE: Traversability CONFIRMED ×2 (0 culls), bot waves castle-front X=23,250 marching + miner loop, full A→B march proven via castle kill; editor closed/reopened under Jonathan's explicit grant, left RUNNING with L_Arena for W1. W1 checklist in handoffs/TASK-218.md. Follow-ups for manager: ground-tiling polish, TeamFill footprint, ISM material usage-flag resaves, first-contact ≈2.5 min (not ~9) pacing flag)
- blocked-by: TASK-216 (qa-passed), TASK-217 (qa-passed), editor+MCP up
- parallel-safe: no (single editor + Git; coordinate with Jonathan if he is active — editor-close is HIS choice)
- spec: >
    On the branch: (1) COMPILE the Phase-0 C++ (editor-bounce per learnings). (2) EDITOR PASS per plan §1 table (numeric
    truth): castles/anchors ±25,000, GoldNodes ±24,200, PlayerStart (−23,800,0,100), ArenaGround scale (560,250,1) top Z=0
    (VERIFY ground-material UVs world-aligned, else ×3.125 tiling), walls E/W ±27,500 Y-scale 250 / N/S ±12,500 X-scale 560,
    NavMeshBoundsVolume (280,125,12), SkyDome scale 3,000, TeamFill RectLights ±12,500 atten 20,000 (fallback if Lumen
    spikes: delete + tint ground by X-sign), ExponentialHeightFog 0.008/10,000/~0.85; KillZ/PP/camera KEEP. (3) MIRROR the
    TASK-217 param table onto the RecastNavMesh actor (ini/actor match check), then Build > Navigation + resave (editor-
    python route; else flag Jonathan's one click in 🚨) — kills the stale serialized bake. (4) DA_BattlefieldScatter:
    extents (26,000,12,000), corridor 1,000, keep-clears — DENSITIES UNCHANGED at Phase 0. (5) PIE SANITY: full A→B march
    completes (units + hero reach both castles), bot marches FROM its castle, Play Again ×3 clean, miner loop intact,
    Message Log clean. (6) COMMIT ON THE BRANCH (explicit pathspecs, no push). ACCEPTANCE: all six recorded + hash; W1
    WATCH checklist posted for Jonathan. Post in 🔧 Build & Git.
- names: >
    `/Game/Maps/L_Arena` actors per plan §1; `DA_BattlefieldScatter`; RecastNavMesh actor. Branch commits only. Law:
    CONVENTIONS "M7.6" (ownership, W-gate), "World axes" (main's law untouched until merge).

