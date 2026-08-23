# TASK-631 — [GH-6] MANIFEST v3 + THE FULL GRID RE-SCAN — collision for the hollow world, file-side proven (art-director handoff)

**Status: COMPLETE — manifest v3 authored (61 → 66 hulls) + full-grid BVH re-scan of the redesigned mesh, all proofs green, NOT applied (TASK-633 owns the apply).**
Fully file-side: Blender 5.1 headless only (`--background --factory-startup`; the live MCP bridge untouched — 30 s cap law respected). **No /Game/ or Content/ write, no editor, no git, no TASKBOARD edit.** cp1252 law: every added manifest byte is pure ASCII; the file re-verified cp1252-decodable; every script `open()` carries `encoding=`.
Laws: GH-R2/R3/R7/R9 · `TL-§2` · `WR-§1` · `W6-R2` · the mount-proof law · `SC-§15`. Base = the SHIPPED 61-hull TASK-618 set (GH-R9: TASK-627 closed no-op, no hotfix delta exists). Date: 2026-08-18.

---

## 0. THE ONE-LINE RESULT

**`ucx.boxes` 61 → 66: 54 hulls retained byte-identical (every exterior family — 629's exterior is vertex-identical, so zero exterior faces moved), 6 interior hulls superseded and removed (`floor_slab_hall`, `spine_west/east`, `keep_front_west/east`, `hall_back`), `approach_tread_09` re-derived in place as the doorway sill landing, 11 new hulls (`hall_floor_01..05`, `hall_wall_01..04`, `gate_jamb_01..02`).** Full-grid proof over 33,184 columns (all 30,537 TASK-615 columns + 2,647 fresh 35-step interior columns): the hollow hall reads **supported flush end-to-end (5,207 open interior walkable columns, delta 0.00..0.82)**, zero hull-free reachable columns, the entry chain **byte-true to 626's measured sequence (worst riser 29.0 vs hero MaxStepHeight 50)** with the interior tail improved to +17.5/+13/+13, mount-proof holds on all 50 blockers, and the 626 §6 cosmetic ledger is adjudicated: **the hall-threshold float is ERASED, the plateau visual-bury strip carries declared.**

## 1. INSTRUMENT CHAIN (nothing trusted un-reproduced)

| gate | result |
|---|---|
| `castle_redesign_v1.blend` anchor (629 §4 readback) | reproduced **verbatim**: dims **7313.576 × 7384.367 × 8082.61** · min-Z **−0.1657** · tris **26,517** · verts 13,052 · slots `[TeamRegion, CastlePBR]` · object matrix identity |
| BVH multi-hit down-scan (the TASK-615 method: visual-top = highest upward face n_z > 0.3 under zone cap 400 interior / 520 exterior; walkable ≤ 45°) | 33,184 columns = 30,537 TASK-615 coordinates + 2,647 fresh 35-step columns over the new interior window (x −1960..1015, y −1855..1400) |
| **Support-model reproduction (the 618 §1 gate)** | my collision model run on the **35-hull set vs `TASK-615-grid.csv`: 0 mismatches / 30,537 rows** on support name AND support top (≤ 0.5) AND blocked flag — the re-derivation IS the TASK-615 instrument |
| Exterior visual identity (GH-R1 cross-check) | **26,274 columns visually identical (≤ 0.5 uu)**; 3,899 changed inside the declared carve/fill footprint; **364 changed outside it — ALL 364 are deleted enclosed-pocket faces (629's interior-classification deletion: west bay + shell pockets), every one BLOCKED or unreachable in BOTH worlds, ZERO reachable-open** (§6 note 1) |
| Exterior spot anchors (615 spot gate) | (−2030,−1610) **513.97** exact · (2765,−2030) **518.27** exact · (1295,−1155) **163.86** exact · S6-adjacent (0,−2905) **85.57/+13.07** exact · (−280,−2345) **+14.94** exact (the old centre-lane max, byte-carried) · (−1295,−1155) visual face deleted (sealed west-bay pocket — expected-changed, blocked under `bay_seal_west_01` in both worlds) |
| Interior probes vs 629 readbacks | (0,−1540) 161.0 · (0,−1295) 174.0 · (0,0) 174.0 · (910,1330) 174.0 · (−1890,1330) 174.0 · (−1890,280) 174.0 — all exact |

## 2. THE DESIGN TABLE — manifest v3 (mesh-local, all new bottoms −60 per the berm precedent)

**Retained byte-identical (54):** `wall_front_west/east` · `tower_front_west/east` · `gatetower_west/east` + `gate_lintel` (the GH-R7 aperture contract: collision gap x −762..+798, lintel 1530, headroom 1356 — untouched) · `keep_west` · `hall_east_front` · `keep_east` · `arcade_seal` · `wall_back` (its y 1380 south face IS the new hall north wall — no new hull needed) · `tower_back_west/east` · `approach_tread_01..08` · `berm_block_01..06` · `bay_seal_west_01`/`bay_seal_east_01..02` · `gate_plateau_01/02` · `shoulder_step_01` · `skirt_toe_01..20`. Zero adjustments owed: 629 proved the exterior vertex-identical.

**Superseded, removed (6):** `floor_slab_hall` (old hall floor y 270..1200) · `spine_west/east` (old corridor walls at −732/768) · `keep_front_west/east` (old hall south walls; keep_front_east intruded the new hall's SE corner x 738..990, y 240..360) · `hall_back` (the old north wall at y 990..1170 — sits ENTIRELY inside the new hall volume).

**Re-derived in place (1) + new (11)** — every face from 629 §2's as-built planes:

| name | x | y | z (top) | role |
|---|---|---|---|---|
| `approach_tread_09` | −462..498 | −1796.5..−1610 | −60..**148** | sill landing 1 (seams with tread_08 at −1796.5; riser +17.5) |
| `hall_floor_05` | −462..498 | −1610..−1460 | −60..**161** | landing 2 (riser +13) |
| `hall_floor_03` | −762..−462 | −1796.5..−1460 | −60..**174** | west threshold flank (629: flanks flat 174) |
| `hall_floor_04` | 498..798 | −1796.5..−1460 | −60..**174** | east threshold flank |
| `hall_floor_02` | −762..798 | −1460..240 | −60..**174** | corridor/passage slab (riser +13 at −1460; erases the old +29 flank strips AND the F1 float) |
| `hall_floor_01` | −1920..990 | 240..1380 | −60..**174** | the grand-hall slab (contains the old `hall_main` envelope + the absorbed north-slot band) |
| `gate_jamb_01` | −990..−735 | −1170..240 | −60..2430 | west corridor wall at the as-built −735 plane |
| `gate_jamb_02` | 771..990 | −1170..240 | −60..2430 | east corridor wall at the as-built +771 plane |
| `hall_wall_01` | −1932..−735 | −60..240 | −60..2430 | hall south wall, west of the corridor (face at y 240) |
| `hall_wall_02` | −2160..−1920 | 240..1380 | −60..2430 | hall west wall at the −1920 plane |
| `hall_wall_03` | 990..1230 | 240..1380 | −60..2430 | hall east wall at the 990 plane |
| `hall_wall_04` | 1230..1860 | 450..1170 | −60..2430 | annex fill (the old annex is SOLID in the redesign — without this hull it is shoot-through visual masonry) |

**Count curve (final N = 66):** 64 rejected (drop `hall_wall_04` + merge the threshold flanks into `hall_floor_02` — leaves the annex shoot-through, and a 174-top over the 148/161 landings floats the walk line); **66 chosen**; 70 rejected (splitting `hall_floor_02` into passage/corridor/alcove strips gains zero covered columns). Axis-aligned law holds (KBoxElem, no rotation). New keys follow the pinned snake_case + two-digit law; no new asset TYPE, so no CONVENTIONS amendment is owed (the spec's own ruling).

## 3. THE PROOFS (spec (2)(a)–(e), all measured, `handoffs/TASK-631-grid.csv` = 33,184 rows)

**(a) Hall floor supported flush end-to-end / zero sink on reachable walkable columns.** Interior open walkable columns: **5,207, delta 0.00 min / 0.82 max** — the only proud columns are 629's declared fill-seam family ((525,−1435) 0.82, (525,−1470) 0.82 …; 629 measured ≤ 0.7 on its sparser probe grid — same seam, denser sampling). Sink audit over the whole grid (reachable ∧ walkable ∧ unblocked ∧ Δ > 14.5): **FINDINGS 0; ground-supported sunk reachable 0.** Every sink > 14.5 falls in exactly the three 618-declared exterior classes with **byte-identical maxima**: D1 lip 159 cols max **+22.63** · shelf flanks 597 max **+48.73** · gate-channel strips 170 max **+42.74** (was 311 — the class SHRANK: its interior members at +29 are erased by the redesign; only the exterior apron strips remain).

**(b) Entry chain unchanged / continuous ≤ 50 arena → hall.** Walk-simulated on v3, lanes x = 0 / ±315: **0 →(+29 @ y −3630)→ 29 →(+14.5 ×7)→ 130.5 →(+17.5 @ −1796.5)→ 148 →(+13 @ −1610)→ 161 →(+13 @ −1460)→ 174.0 hall floor. Worst riser 29.0 vs hero MaxStepHeight 50 (capsule Ø84×192, GH-R9).** Exterior sequence byte-true to 626 §3; the interior tail (old: 145 →+29→ 174) is replaced by the milder 17.5/13/13. Plateau lanes x = ±700: chain to tread_06 101.5, then the south face **+72.5 — BY DESIGN, the 626 row reproduced**; legal mounts: tread_08 → plateau **+43.5** at (±630, −1900) preserved; the old tread_09 → plateau +29 mount is now **flank floor 174 → plateau 174 = 0** (improved); landing2 → flank +13.

**(c) Mount-proof law.** All 50 non-walk-surface hulls audited (top ≥ max adjacent reachable support + 51): **0 violations.** The single declared exception carries exactly as 618 recorded it: the `berm_block_03/06` knoll (top 219 vs plateau 174 = riser 45, mountable), **9 reachable coarse cells, contained — onward climb from 219 is ≥ +192 everywhere** (berm_02/05 top 411, berm_01/04 top 497). Zero reachable cells stand on any toe box or seal.

**(d) Reachability flood-fill (coarse 70 lattice, climb ≤ 50, drop unlimited, BLOCKED impassable, seeded from the field ring).** v3 reachable **3,914** coarse cells vs 61-base **3,676** (same fill code, same mesh): the +238 = the new interior (hall to y 1380, threshold, corridor). The hall is reachable through the gate (probed: (0,−1750) tread_09 148 → (0,−1470) 161 → (0,0)/(0,700)/(−420,1330)/(910,1330)/(−1890,630) all `hall_floor_0x` 174). Sealed regions STAY sealed: both bays (seals byte-identical; the sealed side's pocket faces are what 629 deleted), the back dead pockets, the toe ring. **Two DELIBERATE reachability changes, declared:** (i) the old east annex (x 990..1680, y 570..990) was reachable interior — now solid visual + `hall_wall_04` collision (H1 one-volume default; the annex is not in the GH-R7 must-contain); (ii) the old sealed north slot (y 1170..1380) is absorbed into the hall — now open, reachable, floored at 174 (intended: the hall's north extent). No new floats: float audit findings **0**; carried float classes only — plateau apron float 22 cols (618 class), berm knoll/toe 32, plus 4 toe-face edging columns at (−1855, −3605..−3465) on `skirt_toe_11`'s west face (the 618 edging class; nearest-coarse reach inheritance at the box lip).

**(e) The 626 §6 cosmetic ledger, adjudicated.**
- **Hall-threshold float (~45 uu, y 260..310): ERASED.** Old world (615 CSV visual + 61 hulls): 80 float columns, worst **−46.19** at (−455,280) (visual ~128 dip under slab 174). New world: **every one of those 80 columns reads 174.0/174.0 flush, delta 0.00.** The F1 float control (0,−1120): old −16.52 → **0.00 flush**. The corridor flank +29 control (±315,−1120): → **0.00 flush**.
- **Plateau visual-bury strip (|x|≈700, y −2225..−2120): CARRIED, declared.** 10 columns, support `gate_plateau_0x` 174 under berm-knoll visual up to **+184.68 — old == new byte-exact** (exterior vertex-identical; the berm visual and the plateau top are both contract-pinned: raising the plateau breaks the +43.5/0 mounts, editing the berm visual breaks GH-R1). **0 of the 10 are walkable-classified** (the bury faces are > 45°); 7 reachable. Cosmetic, off the |x| < 630 channel — Jonathan's eye at 636/checkpoint is the arbiter.

**Open-volume guarantee (for 636's hollowness probe):** zero v3 hulls intersect the open hall (−1920..990 × 240..1380 × 174..2160), the corridor (−735..771 × −1140..240 × 174..2100), or the passage channel (−462..498 × −1780..−1140 × 174..1530) — the Ø84×192 capsule sweep is manifest-guaranteed to find nothing. The interior carries **no ceiling lid** (shipped-consistent: the old hall had none; JumpZ 600 from 174 cannot reach 2160).

## 4. PER-REGION PREDICTED RESIDUALS (the post-apply world, both castles)

| class | support | n (this grid) | max Δ | region | verdict |
|---|---|---|---|---|---|
| D1 ground-lip strip | tread_01 | 159 | **+22.63** | y ≈ −3570 | carried, byte-identical (618 §4) |
| shelf flanks (D5) | treads 02..06 | 597 | **+48.73** | tread edges | carried, byte-identical |
| gate-channel strips | treads 07/08 + plateau aprons | 170 (was 311) | **+42.74** | exterior apron |x| 480..630 south of the mouth | carried; **interior members erased** |
| plateau apron float | plateaus | 22 | −50 | y ≲ −2100 flanks | carried (F-float class) |
| berm knoll (float + mount) | berm_block_03/06 | 9 coarse cells | riser 45 | x ±720..918, y ≈ −1975 | carried, contained (+192 onward) |
| toe-face edging floats | skirt_toe_11 | 4 | −44.89 | (−1855, −3605..−3465) | carried (618 edging class) |
| plateau visual-bury | plateaus | 10 | +184.68 (cosmetic) | |x| 630..718, y −2225..−2120 | carried, 0 walkable — ledger item, declared |
| interior proud fill seam | hall_floor_02/04 | ~8 | **+0.82** | (490..560, −1400..−1540) | new-world flush tolerance; 629's declared seam |
| landing-1 south seam | tread_09 | (sub-grid) | float ≤ ~3 | y −1796.5..−1780 channel | support 148 over apron ~145.4 (the measured +2.56 visual seam) |
| mouth-line flank strip | floors 03/04 | (sub-grid) | float ≤ ~29 | y −1796.5..−1780, |x| 462..762 | ≤ 16.5-uu deep strip, fully bridged by Ø84; the alternative was a 173-uu SINK (measured before the fix) |
| deleted pocket faces | (sealed) | 364 | visual-only | bays/shell pockets | blocked/unreachable both worlds, render-invisible (§6 note 1) |

## 5. PREDICTED-TRACE TABLE — TASK-636's contract (the 618 §5 pattern; mesh-local, identical both castles unless noted)

| column | expected support / visual | note |
|---|---|---|
| entry walk x=0: y −3630/−3560/−3445/−3160/−2875/−2610/−2340/−2070/−1796.5/−1610/−1460 | risers +29, +14.5 ×7, **+17.5, +13, +13** → 174 | worst 29.0; 626's exterior chain byte-true |
| (0,−1750) | `approach_tread_09` **148** / vis 148 | the sill landing |
| (0,−1540) | `hall_floor_05` **161** / vis 161 | |
| (0,−1295) | `hall_floor_02` **174** / vis 174 | |
| (0,−1120) F1 control | **174 / 174 FLUSH** | was float −16.5 — ERASED |
| (±315,−1120) corridor flank | **174 / 174 FLUSH** | was +29 — ERASED |
| (0,700) HALL control | `hall_floor_01` 174 / 174 flush | unchanged |
| (−455,805) commander-adjacent | 174 / 174 flush | `CommanderNpcAnchor` (−465,810,174) interior, clearance 570 (629 §3) |
| (910,1330) · (−1890,1330) | `hall_floor_01` 174 / 174 flush | the NEW hall north band (old: hall_back wall / sealed slot) |
| (1300,800) old annex | **BLOCKED by `hall_wall_04`** | deliberate — the annex is solid (H1 one-volume) |
| (±700,−2100) gate mouth | plateau 174 / 174 flush | unchanged |
| (±665,−2170) bury strip | plateau 174 / vis ~300..354 | carried cosmetic (ledger) |
| (0,−2905) S6-adjacent | tread_04 72.5 / 85.57 → **+13.07** | carried control |
| (−280,−2345) | tread_06 101.5 / 116.44 → **+14.94** | the old centre-lane max, carried |
| head-on d1 (walk west from Blue spawn, y=0) | `skirt_toe_01` east face x +3657.5, **+506.5** | DESIGN SEAL, byte-identical (626 §4) |
| d1-Red mirror | `skirt_toe_02` west face −3657.5, +515.5 | byte-identical |
| d3 west band y −1890 | `skirt_toe_03` west face −3657.5, top 138.5 (GH-R8(ii)) | byte-identical |
| wrap d2 y −3650 | `skirt_toe_07/11` face ±2047.5, +109 | byte-identical; field south of y −3692.5 clear |
| plateau south face (±700,−2344.5) | **+72.5** over tread_06 | BY DESIGN; mounts: tread_08→plateau **+43.5** @ (±630,−1900); flank→plateau **0** @ (±630,−1500) (was +29) |
| gate passage | lintel 1530, headroom 1356 ≥ 192; corridor gap **1506** (−735..771); doorway gap 1560 (−762..798) | ≥ Ø84 everywhere |
| (−2030,−1610) · (2765,−2030) | toe_14 518 (−4 float) · toe_10 518.5 (−0.2) | sealed side, unchanged |
| (1295,−1155) | BLOCKED `bay_seal_east_01` (vis 163.86 survives) | unchanged |
| (−1295,−1155) | BLOCKED `bay_seal_west_01`; **the 615-era visual face no longer exists** (629 pocket deletion) | the BLOCK verdict is the contract, not the visual z |

## 6. TASK-633 APPLY CONTRACT (the write-window task)

1. **Order:** ALL visual imports first (SM_Castle + crumbles + textures), THEN the collision apply to all four in the SAME session (GH-R6: the interim FBX/auto-collision state is never committed).
2. **Precheck (freshness + identity gate):** `ObjectTools.get_properties` on each mesh's `BodySetup_0` — all four must read **exactly the shipped 61 boxes, deviation 0.000, `CTF_UseDefault`, 0 convex/sphere/sphyl/taperedCapsule**. The shipped-61 reference is NO LONGER the manifest (it now carries v3): canonical-JSON sha256 (per hull `[center xyz, size xyz, rot 0,0,0]`, 6 dp, array order) of the base-61 = `d1ebfe3bff6ca62082507ed84c514676264db1563071d73e3662556d697e2d9e`. Pre-apply uasset sha256 ×4 = the 618 §6a table (`28131fe6…3aae4` / `a7e07025…266d` / `e822c51c…7d84` / `0867bc6d…86d9`) — if the visual reimport has already run, the uasset shas will differ but the BodySetup must still read the 61.
3. **Apply:** the TASK-611 lane verbatim — staged `set_properties` clear-then-fill on `BodySetup_0` with the **66-box v3 array in manifest order** (⛔ never delete+recreate; ⛔ never `reimport_meshes.py::main()` on the crumbles — the unit-hull trap). Same-run per-stage readback: **canonical v3-66 sha256 = `ebdd54ff7888b38ba7f803c4f27228ebad776a2b4af2fb6c5a963ecc570565ed`, string-IDENTICAL ×4 (`W6-R2`)**. Explicit four-path save only. `L_Arena` untouched, dirty=false pasted entry/exit.
4. Manifest file as written this task: sha256 `ee8e23571aeb5b662eb18846170cc9db1eaa9937cc1417fd21e2c07a24303254` (`Tools/ArtPipeline/pipeline_manifest.json`, `_comment_task631` records the supersession; retained 54 hull text lines byte-identical; file cp1252-decodable, added bytes pure ASCII).

## 7. TASK-632 CRUMBLE DERIVABILITY (preserved)

The v3 set applies IDENTICALLY to `SM_Castle` and all three crumble stages (`W6-R2` invariance — collision never varies across damage stages). 632's stages preserve the pristine footprint in the same conformed space, so no per-stage geometric exception exists; the interior floor/wall planes the v3 hulls pin (174 floor, −735/771 corridor, −1920/990/240/1380 hall, 148/161 landings) are exactly 629's parameterized build volumes — 632 derives visual damage from the same .blend without moving any of them. Per-stage canonical readback (== `ebdd54ff…65ed`) stays the commit gate.

## 8. DECLARED NOTES (`SC-§15` discipline)

1. **364 sub-520-visual columns changed outside the carve/fill footprint** — all are enclosed-pocket faces 629's sky-escape classification deleted (west bay + shell pockets) or hole-closure membranes; every one is BLOCKED or unreachable in BOTH the 61-base and v3 worlds; render-invisible (629's before/after diffs 0.52–3.0% with declared causes). The east-bay face at (1295,−1155) SURVIVED (sky-visible through the arcade) and reproduces 163.86 exactly.
2. **The mouth-line strip** (y −1796.5..−1780): flank floors extended south to seam with tread_08 after the first scan pass measured 7 columns of 173-uu SINK there; the residue is a ≤ 29-uu float on a ≤ 16.5-uu-deep strip, sub-capsule, off-channel (§4).
3. **No interior ceiling hull** — matches the shipped world; declared for 636's probe design.
4. Fill counts and worlds: reachable coarse 3,676 (61-base) → 3,914 (v3); changed columns 61→v3: 5,013, ALL classified (hall 2,733 · corridor/threshold 2,103 · annex-east 119 · jamb walls 40 · fill masonry 11 · mouth 7), **0 unclassified**.

## 9. WRITES THIS TASK (everything else untouched)

- `Tools/ArtPipeline/pipeline_manifest.json` (ucx v3 + `_comment_task631`)
- `.claude/pipeline/handoffs/TASK-631-grid.csv` (33,184 rows, the 615 header)
- this file

**Not touched:** `Content/**` (no FBX export — 633 owns it) · any `.uasset` · `castle_redesign_v1.blend` (read-only) · `L_Arena` · TASKBOARD.md · git · editor/MCP-Unreal. `rescale_refined_fbx.py` never executed. Scratchpad scripts (scan + derive + finalize) are ephemeral outside the repo; every load-bearing number is in this file. Reproduce: open the .blend headless, BVH down-scan the §1 grids, collision from manifest `ucx.boxes`; the §1 gates must reproduce first.
