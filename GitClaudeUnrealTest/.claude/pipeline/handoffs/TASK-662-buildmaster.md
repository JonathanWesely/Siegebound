# TASK-662 — [ROT-1] THE CASTLE ROTATION + THE ORDERED `L_Arena` SAVE (build-master handoff)

**Status: COMPLETE — both castles rotated, facing verified on pixels BEFORE the save, nav settled, the ONE ordered save executed on an enumerated one-package set, the ROT-§2 ledger filled.** Date: 2026-08-27.
Editor: UP throughout, PID **28772** (as left by TASK-659 — never bounced, never closed). MCP `http://127.0.0.1:8000/mcp` green throughout (raw JSON-RPC lane, session scratchpad `mcp_client.py`). Level `/Game/Maps/L_Arena` loaded at entry and exit.
Fences honoured: ⛔ no console sentence, no `M` (latch UNSPENT) · ⛔ no pawn input (CF-R3; the one game-world occupancy was SIMULATE — no pawn spawned or possessed) · ⛔ no compile · git READ-only (the commit is TASK-667's) · TASKBOARD untouched · the user's viewport camera never moved (all captures via `CaptureViewport.captureTransform`).
Laws: ROT-§0..§4 · SIE-§1 (logs read at session start; SIE measurements at T+0:23..T+1:57 of a fresh session) · the never-save law (the ROT-§0.1 exception spent EXACTLY ONCE, below; the law RESUMES IN FULL from the moment of that save, under the new ledger) · the MCP tell-don't-fake law.

---

## 0. THE ONE-LINE RESULT

**Blue's gate now faces world +X and Red's gate world −X — both gates face the battlefield centre, proven on pixels from the centreline before the save — and `L_Arena` was saved ONCE, the save set being exactly the one enumerated package. THE NEW CANONICAL LEDGER: `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58`.**

## 1. THE HASH RETIREMENT RECORD (ROT-§2)

| event | `Content/Maps/L_Arena.umap` SHA256 |
|---|---|
| **ENTRY** (pre-change proof) | `b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268` == the retiring ledger, exact — the map was byte-identical to the known state before the ordered change |
| **EXIT** (post-save, re-verified after the ledger write) | `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58` — **THE NEW CANONICAL LEDGER**, recorded in CONVENTIONS ROT-§2 (the one sanctioned build-master line; nothing else in the file touched) |

The old ledger is RETIRED as of the 22:05:16 save event. Pre-save citations are historical record. ROT-§2 is the living ledger.
Git at entry and exit: HEAD `bd222c2` (expected, no drift; Jonathan had not self-committed). Post-save porcelain shows exactly ` M Content/Maps/L_Arena.umap` + this wave's pipeline docs — the commit is TASK-667's cargo.

## 2. MEASURED, THEN SET (ROT-§1 — measure-before-pinned, honoured in both directions)

**Pre-rotation live reads (== the 656 §1 ground truth):**
- `Castle_0` (label `Castle_Blue`): loc (−25000, 0, 0), rot (pitch 0, yaw **0**, roll −0), scale 1 · AABB x −28657.15..−21343.57 (half **3656.79**), y −3693.46..+3690.91 (half **3692.18**), z −32..9450
- `Castle_1` (label `Castle_Red`): loc (+25000, 0, 0), rot (pitch 0, yaw **0**, roll 0), scale 1 · AABB mirrored, same extents
- **Facing proof on pixels BEFORE deriving targets:** captures from due south of each castle (camera at (∓25000, −6600, 700) looking +Y) show the gate archway dead-centre — **both gates face world −Y** (`TASK-662-pre-blue-southgate.png` / `TASK-662-pre-red-southgate.png`). The 617 C1 datum reproduced on this session's own pixels.
- **Derived targets from the measured convention:** gate local (0,−1,0) → world (sin θ, −cos θ, 0). Blue needs +X (toward centre) ⇒ **yaw +90**; Red needs −X ⇒ **yaw −90**. The ROT-§1 arithmetic confirmed by measurement — no sign correction needed.

**Set + readback (all five components, per actor):**
- `Castle_0`: loc (−25000, 0, 0) UNCHANGED · pitch 0 · roll −0 · scale 1 · yaw **+90** (readback 89.999999999999986 — double representation of exactly +90)
- `Castle_1`: loc (+25000, 0, 0) UNCHANGED · pitch 0 · roll 0 · scale 1 · yaw **−90** (readback −90.000000000000014)
- ⛔ Only these two actors were written. No other property write of any kind this task.

## 3. FACING VERIFIED BEFORE SAVING (spec step 3 — pixels are the authority)

**(a) Pixels from the centreline:** `TASK-662-post-blue-from-centre.png` (cam (−19000, 0, 700) looking −X) and `TASK-662-post-red-from-centre.png` (cam (+19000, 0, 700) looking +X) — **each gate archway faces the camera dead-centre**: arch + tan apron + paved treads + flanking knolls, the full 656-documented gate composition, now presented to the battlefield centre. Blue's mouth is sunlit, Red's sits in its own shadow — exactly what a west-facing (−X) mouth under the SE sun (light yaw 145) must do; the shading itself corroborates the signs. Plus `TASK-662-post-blue-spawnline-M1.png` — **the 656 M1 camera line ((−20900, 0, 260) yaw 180), where VID-001's hero hit the invisible seal, now frames the OPEN mouth**: the before/after pair with 656's `TASK-656-match-eastlip-M1.png` is the discoverability fix in two frames. (`TASK-662-post-{blue,red}-banners.png` are editor-world close-ups at (∓20400, 0, 380) — mouths head-on; F1 props absent there because they are BeginPlay-spawned, see §4.)
**(b) Bounds — the extent swap:** post-rotation AABBs read Blue x −28690.91..−21306.54 (half **3692.18**), y −3657.15..+3656.43 (half **3656.79**); Red the exact 180°-mirror (y-extents mirrored-negated, as ROT-§1's symmetry proof predicts). **X/Y half-extents swapped 3656.79 ↔ 3692.18, byte-exact against the pre-rotation reads.** The gate-corridor void now opens along world X toward the centreline on both castles (pixels in (a) show the open mouth on that axis; the old sealed east face is now the world-Y flank).
**(c) `GateBlockerVolume`:** present on both castles (component census identical: CastleMesh · HPBarWidget · GateBlockerVolume · InteriorNavModifier · HitFlashComponent), attached under `CastleMesh` (the root component) — **it rides the actor transform by construction**. Editor-world readback pre AND post rotation: RelativeLocation (0,0,0), RelativeRotation (0,0,0), BoxExtent 32³ — the ctor-inert state, byte-unchanged (it arms at BeginPlay castle-local, the 659 §3b record). Armed-centre arithmetic at the new yaws: castle loc + R(yaw)·(18, −1575, 852) ⇒ Blue **(−23425, +18, 852)**, Red **(+23425, −18, 852)** — both land CENTRE-side of their castles, on the new gate axis. ✓

## 4. THE SIE PROOF SESSION (pre-save, one session, measurements T+0:23..T+1:57 — SIE-§1 clean)

One Simulate session (`bSimulate=true`, no pawn, no console; started 22:02:01, stopped ~T+2:10, `IsPIERunning` false after). Purpose: the ROT-§3 nav settle over the ROTATED footprints + the BeginPlay-spawned furnishing proof the editor world cannot show.

**Nav — the definitive gate, on the rotated castles (ROT-§3):**
- `nav-config`: standard (`RecastNavMesh-Default`, cellSize 32, tileSizeUU 2000, agentRadius 35, poolCap 1024, **runtimeGen=Dynamic**)
- `nav-build [at-confirmation]: remaining=0 running=0 dirtyAreas=0 hasDirty=false activeTiles=446` — **twice** (22:02:03.701 and .767; activeTiles 446 vs 431/433 in pre-rotation sessions — the tile set re-formed over the rotated footprints and settled)
- `Traversability CONFIRMED (nav settled: 0 pending) — 0 tile task(s) pending; Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s)) [definitive: OnNavigationGenerationFinished]` — **the definitive line, reproduced twice**. **0 castle-lane culls.** No manual Build > Navigation click was needed (JR1 not invoked). Notably the mine-reachability terminal fallback (659 §5.1's parked row) did NOT fire this session — all 6/6 mine paths confirmed with 0 culls; reported as observed, the row stays parked and 663 re-observes.
- Nav residual declaration: `RecastNavMesh-Default` lives IN the `L_Arena` persistent level (actor census) — **no separate nav package exists**; runtime regen (Dynamic) rebuilds over the rotated footprints at every BeginPlay, per the standing JR1 default.

**Furnishings rotated WITH their castles (the F1 set is castle-local, BeginPlay-spawned):**
- Log, both castles, this session: `F1 discoverability set — 2/2 banners, 13/13 path segments, 10/10 toe rocks spawned` + `furnished — 6 of 6 torch anchors spawned (cap 6), commander spawned` — the byte-form intact.
- Pixels: `TASK-662-sie-blue-mouth-wide.png` / `TASK-662-sie-red-mouth-wide.png` (cam (∓19200, 0, 1050) from centre-field) — **both crimson chevron banners frame each mouth as seen from the NEW approach**, trample ribbon running toward the camera, torches lit in the arch, war table in the hall; Red the mirror in its own shade. Close-ups: `TASK-662-sie-{blue,red}-banners.png`.
- Session hygiene: captures at T+1:57 — well before any war-decay horizon; both castles pristine in every frame.

## 5. THE ORDERED SAVE (ROT-§0.1 — the heart of the task)

**The enumerated dirty set** (FULL sweep of every `/Game` package — 3252 assets → 3194 saveable packages probed via `is_dirty`, batched server-side; the 58 skipped entries are `__ExternalObjects__`/`__ExternalActors__` OFPA residue of UNLOADED marketplace maps (Fire_Magic/Ice_Magic/ThirdPerson/Variant_*), which error on probe and cannot be part of any save):

| # | dirty package | justification |
|---|---|---|
| 1 | `/Game/Maps/L_Arena` | The two ordered Castle yaw writes live in this package; `RecastNavMesh-Default` lives in this same persistent level, so all in-level nav state rides inside this ONE package |

**Nothing else was dirty. Zero unexplained lines.** Baseline sweep (pre-rotation, same instrument): 3194 probed, **0 dirty** — the world was pristine before the ordered change, so every byte of the save is the rotation's. The sweep was taken AFTER the SIE session (proving SIE added no dirt), immediately before saving.

**The save:** explicit per-package `save_assets(["/Game/Maps/L_Arena"])` — never save-all. `LogSavePackage` 22:05:16: one Content write, `Content/Maps/L_Arena.umap`. `is_dirty` false after. (For the record: the editor's own periodic autosave had fired at 21:59:24 into `Saved/Autosaves/` — outside `Content/`, gitignored, not a Content-package save; the 22:05:16 event is the ONE save under the law.)

**The never-save law RESUMED IN FULL at 22:05:16, under the ROT-§2 ledger `9ccd54ef…0e58`.**

## 6. POST — THE FRESH LOOK

Re-read after the save + ledger write: `Castle_0` yaw +90 / `Castle_1` yaw −90, locations (∓25000, 0, 0), pitch/roll 0, scale 1 — held. `TASK-662-postsave-blue-from-centre.png`: the gate still faces centre. Level `/Game/Maps/L_Arena` loaded, `is_dirty` false, PIE not running.

## 7. EXIT STATE

Editor **UP PID 28772**, MCP green, level loaded for TASK-663, **dirtiness 0 — decline any save prompt from here on (the exception is SPENT)**. Latch UNSPENT; no console sentence, no `M`, no pawn input ever issued. Viewport camera never moved. For the record: the editor log shows three pre-existing game-world sessions earlier today (21:09 / 21:24 / 21:27, pre-rotation, not this task's) — this task ran ONE Simulate session (22:02).

## 8. FOR TASK-663 / THE ORCHESTRATOR

- 663's entry AND exit hash gate = the NEW ledger `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58` (cite ROT-§2, not this file, as the living ledger).
- The wave anchor datum for 663 §5: with Red's gate on −X, the `ResolveCastleFaceDistance(FVector2D(1,0))` face is now Red's GATE face — record the resolved value live.
- Captures beside this file (12): pre `-pre-{blue,red}-southgate` · post `-post-{blue,red}-from-centre`, `-post-blue-spawnline-M1`, `-post-{blue,red}-banners` (editor world, props absent by design) · SIE `-sie-{blue,red}-mouth-wide`, `-sie-{blue,red}-banners` · `-postsave-blue-from-centre`.
- Deviations from spec: NONE. JR1 not invoked (no manual nav click needed). No sign correction needed (ROT-§1 arithmetic measured true).
