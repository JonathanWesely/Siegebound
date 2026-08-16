# TASK-598 — [WR-44] THE LIVE ADJUDICATION — worst-column traces at BOTH castles (build-master handoff)

**Status: COMPLETE — evidence only. Read-only throughout (W10-R2): nothing saved, nothing compiled, nothing committed, nothing added to the index, no asset edited, no PIE entered.**
Date: 2026-08-16. Editor: Jonathan's relaunched session, PID 17704, MCP `http://127.0.0.1:8000/mcp` (re-verified answering at dispatch). Session open/close grant used only to load `L_Arena`; the editor was NEVER closed (W7-R2 — his session stays his).

---

## THE ONE-LINE VERDICT

**TASK-597's headless SINK MAP is CONFIRMED LIVE at BOTH castle actors at every worst column, to ≤ 0.04 uu; hypothesis (b) placement is REFUTED LIVE at 0.00 uu measured offset; hypothesis (c) is CONFIRMED LIVE — at S1/S2/S5 the castle has NO collision anywhere in the walkable range and the support is the `ArenaGround` slab top at exactly z = 0; the live `SM_Castle` BodySetup carries exactly 25 KBoxElem / 0 convex, byte-identical to the manifest.** No discrepancy of any kind was found between the headless map and the live world. The repair sketch in `handoffs/TASK-597-artist.md` §6 stands unchanged.

---

## 0. HARD-FENCE COMPLIANCE, WITH THE HASHES (the never-save law)

| | SHA256 `Content/Maps/L_Arena.umap` |
|---|---|
| **ENTRY** (before any editor touch) | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |
| **EXIT** (after all traces) | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |

**IDENTICAL — `L_Arena` was opened and traced, never saved.** Matches the pipeline's canonical ledger value (`B3DBC5D9…F8268`, recorded at TASK-507/570 et al.).
⚠️ **Shorthand slip in the board spec, flagged per the wrong-name-replication lesson:** TASK-598's names row says *"hash `b3bd…` lane"* — the canonical hash begins **`B3DB`**, not `b3bd` (transposition). Every full recorded hash agrees; only the four-char shorthand in that one board line is wrong. Nobody should ever verify against a four-char prefix.

Also: ⛔ no `Build.bat`, ⛔ no git operation (the concurrent ACCOUNTS-lane tree changes at exit are the programmer's, untouched), ⛔ no PIE session was ever started — static editor-world queries sufficed, so **no `M` press, no console sentence, no input of any kind existed to threaten the latch: the one-shot latch stays UNSPENT and TASK-579's/571's instruments are untouched by construction.** Git at dispatch: HEAD `7a4bf39`, main 0/0 vs origin.

Sequence note: the editor was on `/Game/Maps/L_MainMenu` at dispatch; I announced on Slack, then loaded `/Game/Maps/L_Arena` via MCP under the session grant, and left the editor open on it.

## 1. THE TWO LIVE INSTRUMENTS (and the calibration that separates them)

- **Visual surface = `trace_world` down-trace.** Measured to be a **COMPLEX (per-poly) trace**: at F1 it hit **127.34** — the *visual* corridor dip — and passed through the 174 collision slab; at S6 it passed straight through tread_02's 58 top to the ground. So its first hit IS the render surface. (SM_Castle `CollisionTraceFlag = CTF_UseDefault`, so complex traces use per-poly — consistent.)
- **Support surface = native physics overlap** (`find_actors` with `ObjectTypeQuery1`/WorldStatic, 4×4×1 uu probe slab), which tests **SIMPLE collision — the shapes a walking capsule actually stands on.** Support tops bracketed at ±0.6 uu (probe pairs above/below), with a binary-search fallback that no column needed.
- **F1 is the polarity control and it PASSED on both instruments:** visual 127.34 under collision 174.0 = **−46.66 FLOAT** — the instruments see both signs, and the corridor cannot be what Jonathan felt (the polarity trap held live).

## 2. THE LIVE WORST-COLUMN TABLE — BOTH CASTLES

Actors: `Castle_0` = **Castle_Blue**, world (−25000, 0, 0) · `Castle_1` = **Castle_Red**, world (+25000, 0, 0) — both yaw 0, scale 1, `CastleMesh` root at actor origin, so mesh-local → world is pure ±25000 x-translation. **Every number below was measured independently at BOTH castles and came out IDENTICAL to the hundredth**, so one table serves both. z in mesh-local uu (= world z).

| # | (x, y) local | live visual z (complex trace) | live support z (overlap top) | **live Δ** | live support identity | TASK-597 headless claim | verdict |
|---|---|---|---|---|---|---|---|
| **S1** | (1050, −2350) | 496.73 | 0.0 | **+496.73 SINK** | `ArenaGround` — castle hull **ABSENT** (probes at z −30/50/150/300/450 all negative) | +496.7, ARENA_GROUND | ✅ CONFIRMED |
| **S2** | (−1050, −2350) | 490.18 | 0.0 | **+490.18 SINK** | `ArenaGround` — castle hull ABSENT (same probe set) | +490.2, ARENA_GROUND | ✅ CONFIRMED |
| **S3** | (990, −2350) | 467.59 | 87.0 | **+380.59 SINK** | `Castle_N` — `approach_tread_03` top (bracket 86.4/87.6) | +380.6, tread_03 | ✅ CONFIRMED (the "377" site) |
| **S4** | (810, −2170) | 410.74 | 116.0 | **+294.74 SINK** | `Castle_N` — `approach_tread_04` top | +294.7, tread_04 | ✅ CONFIRMED |
| **S5** | (1050, −2920) | 116.33 | 0.0 | **+116.33 SINK** | `ArenaGround` — castle hull ABSENT | +116.3, ARENA_GROUND | ✅ CONFIRMED |
| **S6** | (0, −2900) | 85.82 | 58.0 | **+27.82 SINK** | `Castle_N` — `approach_tread_02` top | +27.8, tread_02 | ✅ CONFIRMED (centre-line designed sink) |
| **F1** | (0, −500) | 127.34 | 174.0 | **−46.66 FLOAT** | `Castle_N` — `floor_slab_hall` top | −46.7, floor_slab_hall | ✅ CONFIRMED (polarity control) |
| GND | (0, −4000) | 0.0 (ground) | 0.0 | — | `ArenaGround` reference | ground z ≈ 0 assumption | ✅ CONFIRMED |

Hull identification: KBoxElem names are unset (`None`) in the live BodySetup, so hulls are identified by geometry — valid because the live boxes are numerically identical to the named manifest set (§4). The support actor under S1/S2/S5 is **`ArenaGround`** (`StaticMeshActor_1`, an Engine-Cube slab, top exactly z = 0, spanning x ±28000 / y ±12500), with `BackdropApron` 5 uu beneath it — the support is the ground plane, ⛔ not "nothing", ⛔ not an unexpected hull.

**Cross-instrument check:** the live visual hits reproduce TASK-597's Blender BVH numbers to 0.01–0.04 uu at all seven columns, at both castles — two fully independent measurement stacks (Chaos complex trace in-editor vs headless BVH on the FBX) agreeing on the shipped artifact.

## 3. HYPOTHESIS (b) — ⛔ REFUTED, FINAL, LIVE, BOTH CASTLES (spec item 2)

- **Measured placement offset = 0.00 uu.** Both castle actors sit at world z = 0 with identity rotation/scale; every collision top (58 / 87 / 116 / 174) and every visual surface landed at EXACTLY its mesh-authored z in world space. An actor-transform error would shift visual and collision hits equally and visibly — none exists.
- **Arena ground top beneath both castles = 0.0 exactly** (bracketed ±0.6, both castles); the mesh's own base min-Z is −0.166 (TASK-597 §1b) ⇒ mesh-base-to-ground delta ≤ 0.17 uu, now live-confirmed.
- **Colliding base z:** the live hull bottoms reach **−60.0** (tread_01 bottom bracketed −59.4 present / −60.6 absent, both castles) — the designed embedment TASK-597 predicted from the manifest, NOT a placement error. NOTE for future bounds readers: the full-actor `get_actor_bounds` AABB reads min z = **−32** and max z = 9450 (7313.58 × 7384.37 x/y — TASK-569's dims re-verified); the −32 and the 9450 come from the actor's OTHER components (the AABB spans all components, not only colliding ones), so neither is a collision/placement signal. TASK-569's "min Z = 0" row read the mesh, and the mesh does sit at 0.
- Mechanism (recorded by 597, confirmed live): a transform moves visual and collision TOGETHER — it algebraically cannot produce the DIFFERENTIAL deltas measured above.

## 4. THE COLLISION AUTHORITY — RELAY → ✅ CONFIRMED AT THE LIVE BodySetup (spec's readback demand)

Live readback of `/Game/Meshes/SM_Castle.SM_Castle:BodySetup_0`:
- **`boxElems: 25` · `convexElems: 0`** · sphere/sphyl/taperedCapsule/levelSet/skinnedLevelSet/mLLevelSet/skinnedTriangleMesh all **0** · `CollisionTraceFlag: CTF_UseDefault`.
- **All 25 live boxes are numerically identical to `pipeline_manifest.json` `ucx.boxes`, in the same order** (spot-anchors: box[20] = tread_01 center (15, −3538.5, −15.5) size (1950, 183, 89) ⇒ top 29 / bottom −60 ✓; box[19] = floor_slab_hall (−120, 15, 57)/(3960, 2370, 234) ⇒ top 174 ✓; box[6] = gate_lintel (18, −1560, 1980)/(1560, 780, 900) ✓; all rotations zero ✓).
- ⇒ **TASK-566 R6a's relayed "25 manifest-authored KBoxElem / 0 convex" is now a MEASURED live fact, not a relay** — the `TL-§2` manifest-by-choice path is the live authority, and TASK-597's sink map is valid under it. Both castles share this one BodySetup (both `CastleMesh` components reference the same asset — verified), so **one mesh/manifest repair fixes both castles identically.**

## 5. WHAT THIS CHANGES IN THE REPAIR SKETCH (TASK-597 §6) — NOTHING; TWO CONFIRMATIONS STRENGTHEN IT

1. **R1/R3 (manifest-only: tread splits + berm blocker boxes + tread widening) remains the right shape** — the live world matches the headless model exactly, the ground plane is exactly at 0, and the C1/C2/C3 volumes are where §6 says they are. No number in §6 needs revision.
2. **One repair, two castles:** identical live readings at Blue and Red confirm the single-asset repair covers both placements with no per-instance work.
3. **No code-side cause exists** — placement, spawn logic, and the map are all innocent live; the owner-by-evidence remains **art-director** (W10-R1's gated one-liner). `L_Arena` needs NO edit (flag F2's default holds: repair at the mesh/manifest, never the map).

## 6. SCOPE-FENCE / INSTRUMENT LEDGER

- **Writes:** this handoff + the TASK-598 board status flip. Nothing else anywhere.
- **Not touched:** `L_Arena` (hash-proven §0) · `SM_Castle.uasset` · `Castle.fbx` · `pipeline_manifest.json` (read-only) · the crumble trio · Git index · any `Content/` byte.
- **PIE: never entered.** Latch UNSPENT; no `M`, no console, no input injection (none exists on this MCP surface and none was improvised).
- **Editor:** left OPEN on `L_Arena`, Jonathan's session, never closed (W7-R2). MCP tell-don't-fake: MCP was reachable throughout; every number above is a live tool readback.

**Next:** the manager specs the FLOOR-SINK REPAIR from `handoffs/TASK-597-artist.md` §6 + this file (W10-R1 gate now fully satisfied: headless evidence + live confirmation, both castles).
