# TASK-618 — [CR-1] FLOOR-COVERAGE MANIFEST REPAIR — RB-1..4 + the 617 amendments, 35 → 61 hulls (art-director handoff)

**Status: COMPLETE — MANIFEST AUTHORED + FILE-SIDE VERIFIED + APPLIED IN THE GRANTED WRITE-WINDOW (readbacks §6, all perfect).** The apply ran in Jonathan's live editor session during the orchestrator-granted window (Jonathan stepped away and said go), over the TASK-611 live MCP property-write lane, one gated Programmatic-toolset run: precheck identity (all four meshes carried exactly the shipped 35, deviation 0.000) → staged clear-then-fill → same-run readback gate → explicit four-path save. **Written this task: `Tools/ArtPipeline/pipeline_manifest.json` (`ucx` `_comment_task618` + 26 new `boxes` entries; original 35 byte-identical, zero non-ASCII bytes added, file remains cp1252-decodable per the `:137` law) + the four mesh .uassets (§6a) + this file. Nothing else.** No Blender export/save (read-only import), no PIE, no compile, no Git, no console/`M`, `L_Arena` untouched (`is_dirty` false before, after the apply, and after the save; `git status Content/Maps/` clean), `rescale_refined_fbx.py` never executed, `reimport_meshes.py` never executed. TASKBOARD not edited (orchestrator's instruction; status flip is the orchestrator's).
Laws: CF-R2..R5 · CR-R2 · `W6-R2` · `WR-§1` · `TL-§2` · SC-§15 (declared departures) · the never-save law. Date: 2026-08-17.

---

## 0. THE ONE-LINE RESULT

**`ucx.boxes` 35 → 61 (+26): 3 bay seals + 2 gate plateaus + 1 shoulder step + 20 skirt-toe ring blockers.** File-side re-derivation over all 30,537 TASK-615 grid columns: **every previously-sunk reachable column now reads supported within +14.5 of visual, BLOCKED, or sealed-unreachable — under BOTH reachability baselines (mine and the CSV flag) — except the three pre-declared tread-side ledger classes (lip +22.63 · shelf flank +48.73 · gate channel +42.74, all < 50 = below `HeroMaxStepHeight`).** The 13,233-column half-body walk-in field, both shell bays, the chest-seam ring, the evidence berm at (±1600, −2900) and the +198 west shoulder are all closed. Interior corridor/hall/annex: zero collision change outside the RB footprints; HALL flush, S6 +13.1, F1 float −16.5 all unchanged.

## 1. INSTRUMENT CHAIN (nothing trusted un-reproduced)

| gate | result |
|---|---|
| `Content/RawAssets/Castle.fbx` byte identity | **1,217,612 B** (= TASK-597/615 record), sha256 `8b96f2b6a1c8849084860798c2d2e2fccffdd30340c77195669bb1c8242ae7b2` (pinned here for the record) |
| Blender 5.1 `--background --factory-startup`, conformed-space import (`rescale_refined_fbx.py:102-127` recipe), read-only | §1 anchor gate reproduced **verbatim**: dims 7313.576 × 7384.367 × 8082.611 · min-Z −0.1657 · tris 28,698 · verts 14,057 · 25 historical UCX_ (excluded; manifest = sole authority, `TL-§2`) |
| BVH multi-hit down-scan vs `TASK-615-grid.csv` `visual_z` | **514/514 columns exact** (14 named incl. (−2030,−1610) 513.97 · (2765,−2030) 518.27 · (±1295,−1155) 164.71/163.86 · HALL 174.00 · F1 128.48 + 400 random with-visual ≤0.5 uu + 100 no-visual confirmed) |
| Collision model vs CSV, shipped 35-box set | **0 mismatches / 30,537 rows** on support name, support top (≤0.5) AND blocked flag — my re-derivation IS the TASK-615 instrument on the only side that changed |
| Reachability (coarse 70-lattice directed flood-fill, climb ≤ 50, drop unlimited, BLOCKED impassable) | my fill ⊆ CSV flag (625 coarse cells CSV-reach that mine calls unreachable, 0 opposite — mine is strictly conservative). **Gates below were run under BOTH baselines; identical class-verdicts.** |

**Declared method note (SC-§15):** the spec's "BVH re-scan of the NEW set" is realized as: byte-identity + exact visual reproduction (above) ⇒ the CSV visual field is valid cache; the NEW collision + reachability were then re-derived over the full grid in the proven-exact collision model. The visual side cannot change (no mesh byte touched); re-scanning it would reproduce the same 30,537 numbers.

## 2. THE DESIGN TABLE — 26 new hulls (mesh-local, all bottoms −60 per the berm precedent)

**RB-1 bay seals (3)** — "the gate stays the ONLY opening" now holds for collision:
| name | x | y | top | covers |
|---|---|---|---|---|
| `bay_seal_west_01` | −2424..−990 | −1170..−60 | 2430 | B-II west bay (1,117 cols, median +157) |
| `bay_seal_east_01` | 990..3360 | −1170..360 | 2430 | B-III east bay south (walk-in field, max +511) |
| `bay_seal_east_02` | 1860..3360 | 360..1170 | 2430 | B-III north of `hall_east_front`/`keep_east`; west face 1860 = slab east edge — **the carved annex (x 840..1680, y 570..990) is untouched and stays reachable from the hall** |

(East shipped as **2 of the allowed 2–3**; the third box would only have covered slab-supported slivers under uncarved masonry, capsule-unreachable — declared, not owed.)

**RB-3 gate-mouth plateaus (2)** — manager's pin verbatim: `gate_plateau_01/02`, x ∓918..∓630 / ±630..±918, y −2344.5..245, **top exactly 174.0** (z-span −60..174, same section as `floor_slab_hall`). The measured +58 at (±700,−2100) and the +29 tread_09 flank strips inside x ≥ 630 read **flush 174** (they are the two INTENDED walk-on surfaces among the 26 — mount from tread_08/09 at risers 43.5/29).

**RB-4 shoulder step (1)** — `shoulder_step_01`: `berm_block_04` extended south, x −1470..−720, y −2530..−2440, **top 300** = the measured (−1225,−2450) visual 299.75 → B-V's +198.25 reads supported-at-visual (−0.25) and is unmountable (rise 198.5 from tread_06).
⭐ **Declared departure: `shoulder_step_02` (east, spec-optional) NOT built.** Evidence: (i) east shoulder max is **+48.73** — inside the declared shelf-flank family, below the 50-uu step; (ii) a 150-top step is mountable from tread_06 (riser 48.5) and **chains onto the flank toe ring at a 2.5-uu step**, reopening an elevated escape ledge over the skirt — the exact defect class the mount audit (below) exists to kill; a taller top would stand >50 above the +47 knoll it covers. Also measured: (1225,−2450) visual is actually **311.97** but unreachable under both baselines (the east berm face blocks the corner-cut) — noted for 619.

**RB-2 skirt toe ring (20)** — `skirt_toe_01..20`, derived column-exact from the grid's ≥35-uu walkable-visual contour (the fringe 14.5..35 is only 31 columns — the skirt rises fast, so the 35-toe ≈ the 14.5-toe). Straight bands + 4 corner staircases (DP-optimized y-bands; **axis-aligned law**: the apply lane writes `KBoxElem` with no rotation, so the four ~2,100-uu toe diagonals must be stepped). Inner faces pinned at |x| = 1470 flush with the widened treads. ⭐ Covers the **evidence berm (±1600,−2900), Δ +98/+99 (CR-R2)** — post-repair those columns read BLOCKED (`skirt_toe_09`/`_13` faces = the mound toe).

| name | zone | x | y | top | covered target / vmax |
|---|---|---|---|---|---|
| 01 | E side | 3290..3657.5 | −1929..2170 | 506.5 | 828 / 506.5 |
| 02 | W fat | −3657.5..−2415 | −1170..1400 | 515.5 | 2150 / 515.1 |
| 03 | W thin | −3657.5..−3290 | −1929..2170 | 138.5 | 816 / 138.5 |
| 04 | back W | −1592.5..−1067.5 | 2537.5..3692.5 | 361 | 383 / 360.9 |
| 05 | back C | −1067.5..1067.5 | 2152.5..3692.5 | 513.5 | 2355 / 513.2 |
| 06 | back E | 1067.5..1592.5 | 2537.5..3692.5 | 355 | 393 / 354.8 |
| 07–10 | FE stairs S→N | 1470..{2047.5, 2677.5, 3132.5, 3517.5} | −3692.5..−1592.5 (4 bands) | 109 / 123.5 / 152.5 / 518.5 | 89+324+622+625 |
| 11–14 | FW stairs S→N | mirrored −{1872.5..3552.5}..−1470 | −3692.5..−1592.5 (4 bands) | 109 / 123.5 / 152.5 / 518 | 61+223+704+715 |
| 15–17 | BE stairs S→N | 1540..{3377.5, 3027.5, 2362.5} | 2152.5..3552.5 (3 bands) | 515 / 141 / 92 | 214+553+163 |
| 18–20 | BW stairs S→N | mirrored | 2152.5..3552.5 (3 bands) | 516 / 405.5 / 94 | 138+613+186 |

**⭐ MOUNT-PROOF LAW (new, this task):** every toe-box top ≥ (max adjacent reachable support) + 51 — a ≤50-uu riser would let the hero climb the blocker and ride it as a floating ledge (caught on toe_07..09/11..13, whose visual-max tops of 84–143.5 were within 50 of tread tops 58–101.5; raised to 109/123.5/152.5). **Verified by flood-fill: ZERO reachable coarse columns stand on any toe box or seal; the only reachable new surfaces are the two plateaus (intended).**

## 3. FILE-SIDE RE-DERIVATION — the gates

**Gate A (every previously-sunk reachable walkable column), both baselines:**
| baseline | fixed ≤14.5 | BLOCKED | sealed-unreachable | residual (all in declared classes) |
|---|---|---|---|---|
| my fill | 267 | 13,057 | 2,350 | 1,146 |
| CSV flag | 237 | 13,171 | 2,341 | 1,144 |

Residual = **exactly** the three pre-declared tread classes, zero OTHER (full ledger §4). The whole ground-supported sunk set — B-I ring 15,456 cols (incl. all 13,233 reachable half-body+), B-II, B-III, the 1,089-col chest-seam ring, the 31 fringe cols — is closed.

**Gate B (no legal walkable area lost):** zero regressions outside these declared absorption classes: bay-interior enclosure 277 cols (intended sealing) · toe-face edging ≤105 uu 1,104 · corner-step over-cover 105–210 uu 161 · **>210 uu: 17 cols, max 315** (axis-aligned staircase corners over flat field, all hugging the mound) · **two dead field pockets behind the back shoulders** (74 cols at x ±1085..1470, y 2170..2537, enclosed between back_C/E|W faces and `wall_back`) — **capsule-inaccessible anyway: the only entry alley is 52.5 uu < Ø68**. No DELTA regressions, no unreach regressions elsewhere. Interior-zone collision changes: 2,554 columns, **0 outside the RB footprints**; corridor→hall→annex connectivity re-verified reachable (both per-castle clip fills).

**Float audit (new floats on reachable columns only):** gate plateaus ≤ **50** (the pinned south-apron/flank geometry — off the walk line; the F1 128-dip channel float −16.5 unchanged) · a **9-coarse-cell `berm_block_03/06` knoll** (riser 45 from the plateau top onto the berm's lowest 219 step; contained — no onward climb, drop-only exits; declared). All other floats (slab −46.8 / tread_09 ≤ −25.8 / berm toes) are **inherited unchanged** from the shipped set (float-diff old→new = 0 there).

**Coarse reachable field: 8,406 → 3,594 columns** (−4,812: the sunk field is gone). Per-castle clips near-symmetric (Blue 2,961 / Red 2,972); gate mouth + hall reachable in both.

**Boundary trim (617 §5-2 amendment) — analyzed, VACUOUS for hull count (declared):** both castles carry the same asset at yaw 0, so the band fenced by ArenaBoundary at one castle (|world x| > 27,450 = local |x| > 2,450) is the LIVE mirror band at the other: 2,936 target cols x < −2,450 are fenced at Blue but live at Red; 2,884 x > 2,450 fenced at Red, live at Blue. **No hull may be dropped.** The Red BackdropApron **top −4** base amendment is subsumed by the uniform **−60 bottoms**.

**Final N = 61 vs the expected ~51–59 (+2, declared with the curve):** K-scan of corner-staircase depth — K=(4,3): N 59, >140-uu over-cover 126 cols · K=(5,4): N 63, 93 · K=(6,5): N 67, 82 (diminishing); chosen K=(4,3) + a 3-way back split (+2 boxes) which cut the worst back over-pocket from 385-uu blocked to the (already choked) dead pockets. Cover-all was chosen over hull-count: under-covering the corner wedges would have left 35..90-uu sink strips — the exact reported defect class.

## 4. RESIDUAL LEDGER (updated; every number re-measured this task)

| class | support | n | max Δ | at | lineage |
|---|---|---|---|---|---|
| D1 ground-lip strip | tread_01 | 163 | **+22.63** | (350,−3570) | unchanged (TASK-611 declared 14.5..22.1; grid max identical to TASK-615) |
| shelf flanks (D5 family, incl. east shoulder + gate south apron) | treads_03..06 | 672 | **+48.73** | (−700,−2345) | was ≤ ~47.1 declared; +1.6 at the tread_06/07 border SW of the plateau — same family, < 50 |
| gate channel strips (RB-3 riser leftover) | treads_07..09 | 311 | **+42.74** | (595,−2100) | B-IV declared +29..58; **the +58s are dead** (plateau); leftover = strips |x| 480..630 + corridor flanks x 480..630 at +29 |
| RB-3 plateau apron float | plateaus | ~99 coarse | −50 (float) | x ±630..918, y ≲ −2100 | pinned geometry; off walk-line; F-float class (F1 precedent) |
| berm knoll float | berm_block_03/06 | 9 coarse | −45 (float) | x ±720..918, y ≈ −1975 | new, contained, no exit |
| toe-face edging / corner over-cover | (blocked flat) | 1,104 / 161 / 17 | ≤105 / ≤210 / ≤315 uu deep | ring faces | axis-aligned law; "raised masonry" read; Jonathan's eye = final arbiter (G2 sitting) |
| back dead pockets | (unreachable flat) | 74 | 0 sink | (±1085..1470, 2170..2537) | capsule-choked (52.5 < 68) even without the seal |
| per-box poke-above-visual | — | — | tall boxes stand up to ~470 above their low toe edges (skirt_toe_05 worst); traces/projectiles read solid mound there | inside footprints only | consequence of blocker tops ≥ crest; declared per the berm precedent |

## 5. PREDICTED POST-REPAIR TRACES — TASK-619's expected numbers (mesh-local; identical both castles unless noted)

| column | old (615/617 measured) | **NEW expected** |
|---|---|---|
| (−2030,−1610) | ground 0 / vis 513.97 | support **`skirt_toe_14` top 518** (−4 float, sealed region, walk-in impossible) |
| (2765,−2030) | 0 / 518.27 | support **`skirt_toe_10` top 518.5** (−0.2) |
| east bay (3255,−1120) | 0 / 511.32 (Red: apron −4) | **BLOCKED by `bay_seal_east_01`** (overlap probe hits the seal through z −60..2430) |
| ⭐ chest seam (±1295,−1155) | 0 / 164.7·163.9 | **BLOCKED by `bay_seal_west_01` / `bay_seal_east_01`** — the seam-at-chest world is sealed |
| ⭐ berm (±1600,−2900) | 0 / ~98.5–100.1 | **BLOCKED by `skirt_toe_09`/`_13`** — walk boundary at the mound toe, half-body impossible (CR-R2 discharged) |
| gate-mouth (±700,−2100) | tread_07 116 / vis 174 (Δ+58) | **`gate_plateau_0x` 174 / 174 FLUSH** |
| corridor flank (−315,−1120) | tread_09 145 / 174 (Δ+29) | unchanged +29 — **declared** (inside the untouched centre channel margin; the fix zone starts x ≥ 630 where flush) |
| shoulder (−1225,−2450) | tread_06 101.5 / vis 299.75 (Δ+198) | **`shoulder_step_01` top 300** (−0.25, walled) |
| controls S6 (0,−2900) · HALL (0,700) · F1 (0,−1120) | +13.3 · flush · −16.5 float | **unchanged** (+13.1 grid · flush · −16.5) |
| entry walk-trace y −1890 | support 0 the whole way in | support 0 until the **`skirt_toe_14` west face at local x −3552.5** → BLOCKED. Note: at BLUE that face lies beyond ArenaBoundary_West (world −28,552); the boundary at local −2,450 is hit first. At RED the face is live at world 21,447.5. |
| centre lane |x| ≤ 300 | max +14.94 | unchanged (no new box inside |x| < 630 south of the shoulder row) |

## 6. CRUMBLE-TRIO INVARIANCE RE-DERIVATION (`W6-R2`) — design side; readback = apply gate

- **Law re-derived for the new set:** the identical 61-box array (aperture, lintel, every seal/toe included) applies to `SM_Castle` AND `SM_Castle_Crumble01/02/03` in ONE pass — collision is invariant across crumble stages by design, so a mid-battle stage swap never changes gameplay collision. The three crumble stage meshes share the pristine footprint (same conformed space; visual damage only), so no per-stage geometric exception exists — **verified premise: TASK-611/612 applied and readback-proved the previous 35 identically on all four (612: canonical-JSON equality, deviation 0.000).**
- **Apply lane (from the TASK-611 precedent, NOT the commandlet):** the `_apply_box_collision()` write set executed over live MCP `ObjectTools.set_properties` on each mesh's `BodySetup_0` — staged clear-then-fill (the reflection differ refuses size+content together), `convex/sphere/sphyl/taperedCapsule` cleared, `CollisionTraceFlag` CTF_UseDefault, **save gated on a perfect same-run readback of all four**, explicit four-path save. ⛔ NEVER delete+recreate. ⚠️ **Standing caution re-pinned:** `reimport_meshes.py::main()` must never be pointed at the crumbles — no manifest entries ⇒ `_category_of` falls to "unit" ⇒ ≤4 convex hulls (a W6-R2 violation behind a green run). (If the write-window instead comes as a closed-editor window, the commandlet lane is legal for `SM_Castle` ONLY, with the crumbles still done via MCP after relaunch — but the single-session MCP lane for all four is the proven one.)
- **APPLY RECORD (2026-08-17, the granted write-window):** one `ProgrammaticToolset.execute_tool_script` run — per mesh: `ObjectTools.get_properties` precheck on `…:BodySetup_0` (**all four read exactly 35 boxes, deviation 0.000 vs manifest[:35], `CTF_UseDefault`, 0 convex/sphere/sphyl/taperedCapsule — the freshness+identity gate**) → staged `set_properties` clear-to-empty then fill-61 (both returned true on all four) → same-run readback → `AssetTools.save_assets` with the four explicit paths ONLY (returned true). No delete/recreate, no referencer touched, no other asset saved.
- **PER-STAGE READBACK TABLE — post-apply, post-save, live `BodySetup_0` (the commit gate — ALL PERFECT):**

| mesh | boxElems | convex/sphere/sphyl/tapered | trace flag | max deviation vs manifest (center xyz, size xyz, rotation pyr — every hull) | invariance vs pristine (canonical-JSON, 6 dp, full string equality) | dirty after save |
|---|---|---|---|---|---|---|
| SM_Castle | **61** | 0/0/0/0 | CTF_UseDefault | **0.000** | — (reference) | false |
| SM_Castle_Crumble01 | **61** | 0/0/0/0 | CTF_UseDefault | **0.000** | **IDENTICAL** | false |
| SM_Castle_Crumble02 | **61** | 0/0/0/0 | CTF_UseDefault | **0.000** | **IDENTICAL** | false |
| SM_Castle_Crumble03 | **61** | 0/0/0/0 | CTF_UseDefault | **0.000** | **IDENTICAL** | false |

  **`W6-R2` invariance: the canonical serialisations of all four boxElems arrays are string-IDENTICAL** (61 × [center xyz, size xyz, rotation pyr], 6 dp). `L_Arena` `is_dirty` = **false** at entry, after the writes, and after the save; the four-path save form was used, never save-all. KBoxElem `name` stays `None` on all elems (lane contract; hull identity is by geometry vs the named manifest order).
- **6a. On-disk record (worktree, for TASK-619's `§25b` LFS check):** `git status` shows EXACTLY the four mesh uassets modified (plus the manifest + this handoff from the authoring phase). New sha256:

| file | sha256 (post-apply) |
|---|---|
| `Content/Meshes/SM_Castle.uasset` | `28131fe6f0f1750420ce620d35676be2d73000afe1e2a1b22b61005b6103aae4` |
| `Content/Meshes/SM_Castle_Crumble01.uasset` | `a7e070250c15220cde9059ec60c8599d29249fc461cc5619765950ad6818266d` |
| `Content/Meshes/SM_Castle_Crumble02.uasset` | `e822c51ca2f25f335f57c0c179c18073fe08c894d20fea8b69f00484e4f27d84` |
| `Content/Meshes/SM_Castle_Crumble03.uasset` | `0867bc6d2b3402d6bab6e9196cff3492416eeaac7722b4b556623eff215a86d9` |

- **Nav note for 619:** bay seals + toe ring move the navmesh ring at both castles — `NAV-§4`/`NAV-§12` settled-only Blue→Red + 6-mine-paths is the regression gate (619 §3 owns it).

## 7. WHAT REMAINS (handed to TASK-619)

1. **The apply is DONE** (§6/§6a — readbacks perfect, four assets saved, manifest and assets now agree at 61). TASK-619 runs the independent identity gate (the 612 pattern: live hull set == manifest per stage) + the live re-trace against the §5 expectations + the `NAV-§4`/`NAV-§12` settled-only regression + the wave commit (this commit carries the CASTLE-FINDINGS wave record per the board's COMMIT LAW).
2. ⚠️ Note for 619: the editor session that performed the apply is still Jonathan's (never closed by me); the in-memory and on-disk states match (dirty=false on all four). A fresh-load session for the re-trace is 619's call per its spec ("fresh load of the new assets — batch the bounce with 622's ini-boot if timing allows"); the bounce is Jonathan's hand per the standing law.

**Reproduce:** scratchpad scripts (ephemeral, outside the repo) re-derive everything from `TASK-615-grid.csv` + the manifest; every load-bearing number is in this file. Instrument chain in §1; the §5 table is the live gate's contract.
