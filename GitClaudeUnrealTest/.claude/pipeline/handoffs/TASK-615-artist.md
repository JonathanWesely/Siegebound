# TASK-615 — [CF-3] THE FULL-COVERAGE SINK GRID — the instrument upgrade from ROUTES to the WHOLE WALKABLE SET (art-director handoff)

**Status: COMPLETE — evidence only. NOTHING WAS REPAIRED (CF-R1). NOTHING WAS WRITTEN outside this file + `TASK-615-grid.csv` (W10-R2/R3).**
Fully file-only: no Unreal editor, no MCP-Unreal, no PIE, no compile, no Git, no network. Blender **5.1.2** `--background --factory-startup`, **import-only** — zero exports, zero saves. `rescale_refined_fbx.py` NEVER executed in any mode (the 27x hazard stands); only its conformed-space import recipe (`:102-127`) was copied into a scratchpad-only script.
Laws: CF-R1..R3 · `WR-§1` · `TL-§2` · W10-R2/R3 · `SC-§34` · the RELAYED-DIAGNOSIS LAW.
Date: 2026-08-17.

---

## THE ONE-LINE VERDICT

**The walkable-LOOKING visual set is ~22,500 columns; castle hulls support only ~6,650 of them (29.5%). The other 15,887 (70.5%) stand on the arena ground at z = 0 under castle visual up to 518 uu overhead — a contiguous sunk field that wraps the ENTIRE castle (front flanks beyond |x| 1470, both sides, the back) plus two hull-free bays INSIDE the shell, and 13,233 half-body-or-worse columns are plainly walk-in reachable from open field in every compass direction.** The routes instrument (S1–S6/F1) measured the one lane that is actually covered; Jonathan stood in the other 70%. CF-R2's 174-seam prediction is **CONFIRMED as a mechanism and magnitude, amended in location** (§3). The centre walk lane itself still holds the TASK-612 gate (max +14.9 at |x| ≤ 300). Two NEW on-route findings: the gate-mouth 174-plateau flanks (+29…+58, partly a TASK-611/F3 regression) and the west berm south shoulder (+198). Repair stays collision/manifest-side — **no region found requires R2** (§6).

---

## 1. INSTRUMENT + RE-VERIFICATION (nothing inherited)

- **Sources:** `Content/RawAssets/Castle.fbx` (1,217,612 B, byte-identical to the TASK-597 record; read-only import) · `pipeline_manifest.json` `assets.Castle.ucx.boxes` (**35 hulls**, read `encoding="utf-8"`) · the two evidence PNGs (read first-hand).
- **Anchor gate, all reproduced before any scan was trusted:** dims 7313.576 × 7384.367 × 8082.611 · min-Z −0.1657 · tris 28,698 · verts 14,057 · 25 historical FBX UCX_ objects (excluded from the BVH; the manifest array is the sole authority per `TL-§2`).
- **Spot gate — all 8 live-trace anchors reproduced to ≤ 0.8 uu** (vs TASK-598/611/612): S1 496.73 · S2 490.18 · S3 467.59 · S4 410.74 · S5 116.33 (Δ +43.8) · S6 85.82 (Δ +13.3) · F1 127.34 (Δ −17.7) · HALL 174.00 (Δ 0.0). The instrument sees exactly what the live traces saw.
- **Method (TASK-597's, upgraded to a SET):** conformed-space BVH multi-hit down-scan of the render mesh. Per column: visual-top = highest upward face (n_z > 0.3) under zone cap (interior 400 / elsewhere 520); walkable-visual = face ≤ 45°; collision-top = highest covering manifest box top ≤ visual + 50 (`HeroMaxStepHeight`), else ARENA_GROUND z = 0; BLOCKED = covering box straddling the standing volume (support+50 … support+180); **signed Δ = visual − collision, + = SINK**.
- **Coverage:** coarse grid step **70** (≤ hero Ø 68 + margin) over the FULL mesh footprint x −3710…3710, y −3780…3780 = 11,663 columns; refined to step **35** wherever |Δ| > 14.5 or the support identity changes vs a 4-neighbour = 18,874 fine columns. **Total 30,537 columns — full CSV beside this file: `TASK-615-grid.csv`** (2,084,610 B; header `x,y,res_uu,zone,visual_z,visual_nz,walkable,support,support_top_z,delta_signed(+sink),blocked_by,reachable`).
- **Reachability:** directed flood-fill on the coarse lattice from open-field boundary columns (climb ≤ 50, drop unlimited, BLOCKED impassable). ⚠️ Point-column BFS — it cannot see capsule radius 34; every named region below was checked by hand against hull spans for a ≥ 68-uu opening, and the one place point-reach leaked through a 24-uu crack (the north slot, §2 B-VI) is called out as SEALED. Live confirmation = TASK-617.

## 2. THE REGION LIST — every coverage gap, named

Declared residuals first, CONFIRMED still as declared: **D1 lip strip** max +22.63 at (350, −3570) (declared 14.5–22.1; grid-resolution consistent) · **D5 shelf** flank maxes +44.6/+47.1 on treads 04/05 (declared ≤ ~46) · **strict centre lane |x| ≤ 300, y −3556…990: max +14.94** at (−280, −2345) — S6 at x=0 reproduces 13.3, **the TASK-612 gate number stands on the lane it measured**. The carved interior proper (corridor + hall + annex over slab/treads) has **zero sink** — flush/float only, exactly TASK-597 §2d.

| # | region | extent (mesh-local) | n cols | Δ (sink) | support | reachable | class |
|---|---|---|---|---|---|---|---|
| **B-I** | **THE PERIMETER SKIRT RING** — walkable-looking castle skirt/mound visual with NO hull, wrapping the whole building: front flanks \|x\| > 1470, BOTH SIDES, the BACK | contiguous ring, x ±3660, y −3700…+3690 | **~15,900** ground-supported walkable cols; **13,698 > 90 uu (13,233 reachable); 776 > 180 (738 reachable)** | 0 → **+518.27** (at (2765, −2030)); median +120, p90 +163 | ARENA_GROUND | walk-in from EVERY direction | D7 declared only the front-flank slice out-of-repair-bounds; the sides/back were **never measured by any instrument before this grid** |
| **B-II** | **WEST SHELL BAY** — hull-free INSIDE the shell (no west perimeter hull below keep_west's y −60) | x −2424…−990, y −1170…−60 | 1,117 | median **+157**, max +241 | ARENA_GROUND | **ALL** — straight walk-in from the west field (no hull crosses y −1170…−60 until spine_west x −990) | UNDECLARED FINDING |
| **B-III** | **EAST SHELL BAY (the arcade side)** — the east perimeter has NO hull below y 1170; `arcade_seal` seals only y 1170…1356 | x 990…3360, y −1170…1170 | 3,256 | median **+158**, max **+511** (at (3255, −1120)) | ARENA_GROUND | 3,244 — walk-in through the visual east wall / around tower_front_east | UNDECLARED FINDING — "the gate stays the ONLY opening" is true of the CARVE, false of the collision |
| **B-IV** | **GATE-MOUTH / CORRIDOR 174-PLATEAU FLANKS — ON the legal route.** The carve floor (z 174) flanks the natural 128-dip channel inside the arch/corridor; supports below are treads 07/08/09 (116/130.5/145) | x −735…770 (flank strips ~±540…±900 wide at the arch), y −2310…+245 | 565 | **+14.5…+58**; max +58 at (±700, −2100); +29 over the whole tread_09 flank strip | treads 07/08/09 | yes — it IS the entrance | UNDECLARED; **partly a TASK-611 regression**: the tread split lowered passage support 145 → 116/130.5 south of y −2072 (+29 → +58 there), and the F3 slab pull-back exposed tread_09 (145) under 174-visual at y −1170…245 (was FLUSH under the old slab) |
| **B-V** | **WEST BERM SOUTH SHOULDER** — the mound face south of berm_block_04 rides walkable-angle over widened tread_06 | (−1225 ± 60, −2450 ± 40) | 4 (> 47) | max **+198.25** at (−1225, −2450), vis 299.75 | approach_tread_06 | yes (SW corner-cut path) | UNDECLARED — D7 bounded this shoulder at ~39; east mirror is only +47 |
| **B-VI** | NORTH SLOT (recorded for completeness) | x −1930…990, y 1170…1380 | 384 | vis 127–151 over ground | ARENA_GROUND | **NONE — SEALED** (hall_back/wall_back/keep_west/arcade_seal enclose it; only 24-uu cracks, capsule Ø 68 cannot pass) | no player exposure; the closest literal "interior 174-over-ground" instance, and it is unreachable |

**The seam-174 ring (the chest-seam class):** 1,089 standable ground-supported columns under walkable visual **150–200 uu** — the exact contour where a ~180-uu character reads a floor seam at chest/neck — ALL reachable, forming a band through B-I/B-II/B-III (examples: (−1295, −1155) vis 164.7 · (1295, −1155) vis 163.9 · (−1820, −2170) vis 175.6 · (2030, −1890) vis 176.4).

**Profile witness (y = −2900):** tread_04 ends at x 1470 (Δ +38.5) → x 1500 vis 101.2 **Δ +101.2 on ground** → the half-body band begins ONE STEP off the widened treads. **Jonathan's "~90+ uu" number is the natural reading of the first contour off the tread edge.**

## 3. CF-R2's NAMED PREDICTION — adjudicated

**The MECHANISM and MAGNITUDE are CONFIRMED; the LOCATION is amended.** A hull-free column supported by arena ground (z ≈ 0) under visual ~174 does put the seam at chest height — but it does NOT occur on the carved interior floor: slab + treads cover the corridor/hall/annex completely (verified column-by-column; the interior proper has zero sink). The prediction's only literal interior instance (the north slot, vis 127–151) is SEALED and unreachable. **The realized chest-seam region is the 150–200-uu contour of the SKIRT + the two SHELL BAYS** — 1,089 reachable columns whose numbers (155–175 over ground on a ~180 character) match the pixel exactly.

**How the player physically reaches it (flood-fill path, support 0 the whole way):** e.g. open field (−3710, −1890) → walk due east along y ≈ −1890 → (−2030, −1610), sink +514 — nothing blocks: the front wall band ends at tower_front_west x −3300, no west perimeter hull exists below keep_west's y −60, and the skirt visual has no collision. The same walk works on every side; entering the WEST BAY needs only a straight line from the west field at y −1150…−60. No jumping, no falling, no exploit — level ground all the way, visual mounting overhead.

## 4. THE SCREENSHOT RECONSTRUCTION (best-effort; the live word is TASK-617's)

- **Shot 1 (`entrance-berm-halfbody-sink.png`):** two candidate columns. (a) **On the centre lane behind a NON-castle mound** at ~(0…300, −3500): grid says vis 48–60, Δ ≤ +22 — if the live trace at that column shows support ≈ visual while the pixel shows half-body, the berm is not castle geometry. (b) **On the front skirt at \|x\| ≥ 1500, y −3100…−2600**: Δ +91…+114 (half-body exactly), 1,594 reachable columns.
- **⭐ The berm-from-the-FBX answer (spec (3)): NO castle visual surface at the threshold matches the smooth berm profile.** In the centre threshold band (\|x\| ≤ 700, y −2200/−1600) the castle visual's maximum rise over support is **+41.6 uu** and the approach centre is a 3°-planar ramp — no smooth mound of ≥ 90 uu crosses the camera-doorway axis anywhere in the mesh. The only mound-profile castle surfaces near the gate are the C2 berms at \|x\| 720…1470, which flank, not cross, the lane. **This is lane A's strongest corroboration from the mesh side** — and the shot's berm is grass-coloured against the castle's pale-beige albedo, with the character's feet on grass at the bottom of frame.
- **Shot 2 (`interior-dark-floorseam.png`):** the seam-at-chest + grass-underfoot + doorway-with-blue-bar sightline is the **seam-174 ring**, most plausibly the WEST BAY south edge / west front skirt: primary column **(−1295, −1155), vis 164.7 over ground** (mirror (1295, −1155) vis 163.9; alternate (−1820, −1120) vis 150.5). He is standing INSIDE the shell footprint on arena grass, eye just below the 174 floor plane, looking at the gate from the wrong side of the collision — which also feeds lane C (a commander on the 174 slab is invisible below the waist from there) and lane D (no torch pool reaches the bays).

## 5. WHAT THE ROUTES INSTRUMENT COULD NOT SEE (CF-R2 discharged)

TASK-597/598/611/612 measured ~6,400 approach-route columns and 7 S/F anchors — all inside the one covered lane. The full set is ~22,500 walkable-visual columns; **the instrument had never touched 70% of it, and the untouched 70% is exactly where Jonathan stood.** The routes verdicts remain true ON the routes (re-confirmed at every anchor this task); they were silent, not wrong, about the set.

## 6. COSTED REPAIR SKETCH — ⛔ INPUT to the manager's spec, NOT authorization (CF-R1)

Owner by the evidence: art-director (manifest-only, same-path `_apply_box_collision()` write set — the R1 pattern; NEVER delete+recreate). **Zero compile. No region found is unfixable collision-side ⇒ nothing here triggers the held R2** (G2 stays exactly where the manager put it — Jonathan's eye after the lighting pass).

| option | what | fixes | est. hulls |
|---|---|---|---|
| **RB-1 bay seals** | fill WEST BAY (1 box: x −2424…−990, y −1170…−60) + EAST BAY (2–3 boxes: x 990…3360, y −1170…1170, skipping nothing — no carve enters either bay) to wall height, honouring the design line "the gate stays the ONLY opening" | B-II, B-III, and most of the seam-174 ring's reachable interior instances | +3–4 |
| **RB-2 skirt toe ring** | perimeter blocker ring at the skirt's ~35-uu visual contour (walk boundary moves to the mound toe; the mound reads as raised masonry = the berm_block precedent) — ~10–16 axis-aligned boxes around the sides/back/front flanks | B-I (the 13,698-column half-body+ field becomes unwalkable wall) | +10–16 |
| **RB-3 gate-mouth plateaus** | two flank boxes top 174 over the carve-floor strips (x ±~630…918, y −2344.5…+245; centre channel \|x\| ≲ 480 untouched — its visual is the 128 dip, already float) — lateral riser vs treads 29…58, declare it (edge, not on the walk line) | B-IV (the on-route +29…+58) | +2 |
| **RB-4 shoulder step** | extend berm_block_04 (and optionally 01) south to y −2530 with one lower step (top ~300 west / ~150 east) | B-V (+198) | +1–2 |

Total 35 → **~51–59 hulls**. ⛔ **The crumble-trio invariance re-derivation is IMPLICATED** (`W6-R2`): any pristine-collision change re-applies the identical set to SM_Castle_Crumble01/02/03 with per-stage readback as the commit gate — same as TASK-611. Nav implications: bay seals + skirt ring change the navmesh ring around both castles; the `NAV-§4`/`NAV-§12` settled-only Blue→Red + 6-mine-paths check is the regression gate. The scatter/berm visual itself is lane A's repair, not this one.

## 7. TASK-617 LIVE VERIFICATION LIST (both castles; trace instrument = TASK-598's, unchanged)

1. **Grid worst columns** (trace down + WorldStatic overlap ±0.6): (−2030, −1610) exp. support 0 / vis 514 · (2765, −2030) exp. 0 / 518.3 · east bay (3255, −1120) exp. 0 / 511.3 · west bay (−1295, −1155) exp. 0 / 164.7 · shoulder (−1225, −2450) exp. tread_06 72.5(top 101.5)/ vis 299.75 · gate-mouth (±700, −2100) exp. tread_07 116 / vis 174 (Δ +58) · corridor flank (−315, −1120) exp. tread_09 145 / vis 174 (Δ +29) · controls S6 (0, −2900) +13.3 and HALL (0, 700) flush.
2. **⭐ The chest-seam adjudication (the CF-R2 prediction):** trace at BOTH shot-2 reconstruction columns (±1295, −1155) — expected hit z ≈ 0 on `ArenaGround`, visual ~164 — seam at chest on the hero. Confirm the amended location (bays/skirt ring, NOT the carved interior; interior stays flush).
3. **The entry-path walk-trace:** trace the flood-fill line y = −1890, x −3710 → −2030 (or y −1150 into the west bay): support must stay ~0 the whole way while visual climbs — that IS "how a player physically reaches the sunk region".
4. **Shot-1 lane-A cross-check:** trace at (0…300, −3500) — expected Δ ≤ +22. If the pixel shows half-body there, the berm is non-castle: dump scatter HISM instances in x −900…900, y −3700…−2900 (joint with TASK-613's checklist).
5. **Capsule-vs-point check:** confirm a real capsule (r 34) can walk into the west/east bays (openings are 100s of uu — expected YES) and canNOT enter the north slot (24-uu cracks — expected NO).
6. **Sightline feed to lane C:** from (−1295, −1155) eye height, is the commander (on the 174 slab) occluded below the waist by the floor plane; blue-bar world position vs this viewpoint.

## 8. SCOPE-FENCE COMPLIANCE (W10-R2/R3 verbatim)

- **Writes: this file + `TASK-615-grid.csv`. Nothing else.** Scratchpad script + summary JSON live outside the repo (ephemeral); every load-bearing number is IN this handoff.
- **Not touched:** `Castle.fbx` (read-only import, byte count 1,217,612 unchanged) · `pipeline_manifest.json` (read-only) · any `Content/` path · the crumble trio · `L_Arena` · TASKBOARD.md · Git · editor/MCP-Unreal · no FBX export, no reimport, no manifest edit. `rescale_refined_fbx.py` read, never executed.
- **Reproduce:** import `Castle.fbx` per the conformed-space recipe (`rescale_refined_fbx.py:102-127`: axis −Z/Y, bake matrix_world, Y-mirror + winding flip, ×100 m→uu), BVH multi-hit down-scan on the §1 grids, collision from manifest `ucx.boxes`. The §1 anchor + spot gates must reproduce first.

**Next:** TASK-617 adjudicates §7 live; the manager specs the FLOOR-COVERAGE repair from this file + 613/614/616 + 617's.
