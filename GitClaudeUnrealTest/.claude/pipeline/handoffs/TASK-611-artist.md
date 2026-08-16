# TASK-611 — [WR-45] THE FLOOR-SINK REPAIR, R1 — 25 → 35 manifest hulls, applied same-path to all four SM_Castle* (art-director handoff)

**Status: COMPLETE — applied, saved, readback-verified at max deviation 0.000, per-stage invariance IDENTICAL, live-probed at BOTH castles.**
Date: 2026-08-16. Editor: Jonathan's live session (PID 17704), MCP `http://127.0.0.1:8000/mcp` — **never closed, no PIE entered, no `M`, no console sentence, no input of any kind** (the TASK-571+552 latch and TASK-579's first-open instrument are untouched by construction). No `Build.bat`, no Blender, no git, no `.py` committed.
Laws: `WR-§1` · `W6-R2` · `TL-§1`/`TL-§2` · W10-R5..R7 · `SC-§15` (departures declared in §5) · the never-save law.

---

## 0. HARD-FENCE COMPLIANCE

| | SHA256 `Content/Maps/L_Arena.umap` |
|---|---|
| ENTRY (before any write) | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |
| EXIT (after all writes + probes) | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |

**IDENTICAL — the map was probed, never dirtied, never saved** (`is_dirty("/Game/Maps/L_Arena")` = false before, during and after; the editor sits on `L_Arena` exactly as TASK-598 left it). Byte-identity is what TASK-612's item (4) hashes against.

**Files changed — exactly the FIVE W10-R7 artifacts, nothing else:**

| artifact | change | on-disk after save |
|---|---|---|
| `Tools/ArtPipeline/pipeline_manifest.json` | `assets.Castle.ucx.boxes` 25 → 35 + ASCII `_comment_task611` record (original `_comment` preserved) | edited 2026-08-16 |
| `Content/Meshes/SM_Castle.uasset` | collision re-applied | 1,649,990 B, 12:35:25 |
| `Content/Meshes/SM_Castle_Crumble01.uasset` | collision re-applied | 1,590,738 B, 12:35:25 |
| `Content/Meshes/SM_Castle_Crumble02.uasset` | collision re-applied | 1,589,792 B, 12:35:26 |
| `Content/Meshes/SM_Castle_Crumble03.uasset` | collision re-applied | 1,589,165 B, 12:35:26 |

⛔ NOT touched: `Content/RawAssets/Castle.fbx` (no visual byte anywhere — live visual traces below are byte-identical to TASK-598's) · `L_Arena` · any `.py` · any C++ · `rescale_refined_fbx.py` never executed. Helper/apply scripts lived in the session scratchpad only.

---

## 1. WHAT WAS APPLIED — the 35-box set (the manifest is the authority; this table is its readback)

**Order: boxes 1–19 = the 19 shell hulls, UNCHANGED byte-for-byte** (wall_front_west/east, tower_front_west/east, gatetower_west/east, gate_lintel, spine_west/east, keep_front_west/east, keep_west, hall_east_front, keep_east, hall_back, arcade_seal, wall_back, tower_back_west/east) — **gate aperture (collision gap x −762…+798 = 1560), lintel bottom 1530, every wall face: IDENTICAL to shipped.** Then:

| # | name | center | size | top z | y-span | x-span |
|---|---|---|---|---|---|---|
| 20 | floor_slab_hall | (−120, **735**, 57) | (3960, **930**, 234) | 174 | **270…1200** (was −1170…1200 — the F3 rider) | −2100…1860 |
| 21 | approach_tread_01 | (0, −3595, −15.5) | (2940, 70, 89) | 29 | −3630…−3560 | ±1470 |
| 22 | approach_tread_02 | (0, −3503.5, −8.25) | (2940, 113, 103.5) | 43.5 | −3560…−3447 | ±1470 |
| 23 | approach_tread_03 | (0, −3304.25, −1) | (2940, 285.5, 118) | 58 | −3447…−3161.5 | ±1470 |
| 24 | approach_tread_04 | (0, −3019, 6.25) | (2940, 285, 132.5) | 72.5 | −3161.5…−2876.5 | ±1470 |
| 25 | approach_tread_05 | (0, −2745.25, 13.5) | (2940, 262.5, 147) | 87 | −2876.5…−2614 | ±1470 |
| 26 | approach_tread_06 | (0, −2479.25, 20.75) | (2940, 269.5, 161.5) | 101.5 | −2614…−2344.5 | ±1470 |
| 27 | approach_tread_07 | (18, −2208.5, 28) | (1800, 272, 176) | 116 | −2344.5…−2072.5 | −882…918 |
| 28 | approach_tread_08 | (18, −1934.5, 35.25) | (1800, 276, 190.5) | 130.5 | −2072.5…−1796.5 | −882…918 |
| 29 | approach_tread_09 | (18, −763.25, 42.5) | (1800, 2066.5, 205) | 145 | −1796.5…**+270** | −882…918 |
| 30–32 | berm_block_01/02/03 (east) | x 1095 | x-size 750 | 497 / 411 / 219 | −2440…−2210 / −2210…−2020 / −2020…−1930 | 720…1470 |
| 33–35 | berm_block_04/05/06 (west) | x −1095 | mirrored | 497 / 411 / 219 | same bands | −1470…−720 |

All tread/berm bottoms z −60 (the designed embedment TASK-598 measured). All rotations zero (axis-aligned KBoxElem — the `WR-§1` constraint that makes a ramp inexpressible).

**Derivation (all from the 63-sample centre visual profile in `Cache/Castle/rescale_report.json` — re-interpolated this task, not inherited):** the four ramp risers 29→58→87→116→145 each split at the measured flush crossing of the +14.5 midpoint. Authored boundary vs profile crossing: 58 @ −3447 (Δ0.17) · 72.5 @ −3161.5 (Δ0.24) · 87 @ −2876.5 (Δ0.29) · 101.5 @ −2614 (Δ0.08) · 116 @ −2344.5 (Δ0.71) · 130.5 @ −2072.5 (Δ0.24) · 145 @ −1796.5 (Δ0.17). The four RETAINED boundaries are the shipped TASK-555 flush points exactly. Chain: **10 risers = 29 + 8×14.5 + 29** (see departures D1/D2), every riser ≤ 29 ≤ nominal `AgentMaxStepHeight` 35 with the shipped 6-uu margin, every tread depth ≥ 70 > agent Ø 68, chain contiguous −3630 → +270 → slab 174.

Old→new tread map for TASK-612: old tread_01 ≈ new 01+02 · old 02 ≈ new 03+04 · old 03 ≈ new 05+06 · old 04 = new 07+08 · old 05 = new 09 (+ the F3 extension to +270).

## 2. THE APPLY — mechanism, exactly

- **Write set = the `_apply_box_collision()` write set, executed over the live MCP lane** (spec (5)'s "live MCP python lane"): per mesh, `AggGeom.boxElems` ← the 35 manifest boxes (center/x/y/z, rotation zero, name None — the fields the tool function writes), `convexElems`/`sphereElems`/`sphylElems`/`taperedCapsuleElems` cleared, `CollisionTraceFlag` = `CTF_UseDefault`; then an explicit four-path `save_assets` (never the save-all form). Same-path in-place property write on the existing `BodySetup_0` — **never delete+recreate; every referencer untouched.**
- **Why not the literal function call (departure D4, named):** the live MCP surface has no general-Python lane (the Programmatic toolset is sandboxed to tool orchestration), and the `-run=pythonscript` commandlet requires the project closed — taking Jonathan's open session for it loses to "prefer the running instance." The property-write lane is the **TASK-567 precedent**: the crumble trio's current 25-box collision was itself authored via `ObjectTools.set_properties` on `…:BodySetup_0` and TASK-598 then measured that lane's output **byte-identical to the manifest at the live BodySetup**. `Tools/reimport_meshes.py` was READ, not edited, not executed.
- ⚠️ **Caution recorded for any future commandlet run:** `reimport_meshes.py::main()` must NOT be pointed at the crumbles — the manifest carries no `Castle_Crumble0N` entries, so `_category_of` falls through to "unit" and `_reimport_one` would replace their box set with ≤4 convex decomposition hulls (a W6-R2 violation behind a green run). This task applied the Castle box set to all four explicitly, which is what guarantees invariance.
- The array write is staged (the reflection differ refuses size+content changes together): clear-to-empty, then fill-35 — and **the save was gated on a perfect readback of all four meshes in the same script run** (a partial state could never reach disk; nothing was saved until every count/value/invariance check passed).

## 3. SELF-READBACK (the author's check — TASK-612 runs the independent gate)

Post-apply, post-save, per stage, live `BodySetup_0`:

| mesh | boxElems | convex | sphere | sphyl | trace flag | max deviation vs manifest (center xyz, size xyz, rotation — every hull) | dirty after save |
|---|---|---|---|---|---|---|---|
| SM_Castle | **35** | 0 | 0 | 0 | CTF_UseDefault | **0.000** | false |
| SM_Castle_Crumble01 | **35** | 0 | 0 | 0 | CTF_UseDefault | **0.000** | false |
| SM_Castle_Crumble02 | **35** | 0 | 0 | 0 | CTF_UseDefault | **0.000** | false |
| SM_Castle_Crumble03 | **35** | 0 | 0 | 0 | CTF_UseDefault | **0.000** | false |

**Per-stage invariance (`W6-R2`): the canonical-JSON serialisation of all four boxElems arrays is IDENTICAL** (count, every extent, aperture and lintel included — the shell hulls that define aperture/lintel are byte-unchanged in all four). Invariance is stage-vs-pristine on the NEW set: all four meshes carry it; the historical 25-hull set exists nowhere anymore. KBoxElem `name` stays `None` on all elems (the tool function does not write names; hull identity is by geometry vs the named manifest order, exactly as TASK-598 did).

## 4. LIVE PRE-CHECK — both castles, TASK-598's instrument pair (complex trace = visual · WorldStatic overlap probe ±0.6 = support)

Identical numbers at Castle_0 (Blue, −25000) and Castle_1 (Red, +25000); one table serves both. z mesh-local = world.

| # | (x, y) | live visual z | live support top (bracketed ±0.6) | **live Δ** | vs pinned W10-R5 acceptance |
|---|---|---|---|---|---|
| S1 | (1050, −2350) | **496.73** (= TASK-598 exactly) | **497.0** — berm_block_01 | **−0.27** | ✅ supported-at-visual (was +496.73 on ground) |
| S2 | (−1050, −2350) | 490.18 | 497.0 — berm_block_04 | −6.82 | ✅ structural wall (was +490.18) |
| S3 | (990, −2350) | 467.59 | 497.0 — berm_block_01 | −29.41 | ✅ blocked (was +380.59 on tread_03) |
| S4 | (810, −2170) | 410.74 | 411.0 — berm_block_02 | −0.26 | ✅ supported-at-visual (was +294.74) |
| S5 | (1050, −2920) | 116.33 | 72.5 — approach_tread_04 (widened) | **+43.83** | ✅ SUPPORTED, real floor (was +116.33 on ground) — declared residual, see D5 |
| S6 | (0, −2900) | 85.82 | 72.5 — approach_tread_04 | **+13.32** | ✅ **≤ 14.5** (was +27.82) — the pinned centre-route number |
| F1 | (0, −500) | 127.34 | 145.0 — approach_tread_09 | **−17.66** | ✅ **≈ −17** (was −46.66) — the F3 rider live |
| HALL | (0, 700) | 174.0 | 174.0 — floor_slab_hall | 0.0 | ✅ hall flush, slab intact |
| GND | (0, −4000) | 0.0 | 0.0 — `ArenaGround` | — | ✅ instrument control |

**Negative probes (the change is live, not cached):** (0, −500, 173.4) — the old slab position — now **EMPTY** at both castles (the slab genuinely starts at +270); (1050, −2920, 60) — hull-free in TASK-598's probe set — now **hits the castle** (widening live); (1050, −2350, 300) — mid-berm — hits the castle (the wall is solid through its height). Live visual values are byte-identical to TASK-598's table at every column ⇒ **no visual byte changed** (R1 manifest-only, proven at the instrument).

## 5. SC-§15 DEPARTURES — each named with its mechanism (spec item 7)

- **D1 — the ground-lip riser 0→29 did NOT split (spec said 6→12 risers).** Measured mechanism: the apron front edge (y −3630) already reads visual 36.58…48.3 — no flush point exists at/below 29, and the 43.5 crossing sits at y −3615.8, ~14 uu from the edge. A 14-uu tread risks Recast (CellSize ~19) dropping the intermediate span and quantising the entry climb to 43.5 > the 40-uu effective walkableClimb ⇒ unit entry dies. The 29/43.5 boundary is instead at y −3560 (depth-70 floor). **Consequence, declared: a 56-uu lip strip (y ≈ −3616…−3560) keeps residual centre-route sink 14.5…22.1 uu** (shipped was 17.5…29 there; everywhere else on the centre route the residual is now ≤ 14.5). This is the R1 floor at the lip — geometrically infeasible to do better with axis-aligned treads ≥ 68 deep and an entry step ≤ 35; R2 (visual re-derivation) is the held escape. **S6 and every S-column sit outside this strip.**
- **D2 — the hall riser 145→174 did NOT split.** The interior visual jumps 128.6 → 174.0 in a vertical step at y ≈ 350 (`through_route_floor`) — no 159.5 flush point exists. The 29-uu riser onto the slab stands at y +270, entirely in FLOAT territory (visual below both tops ⇒ zero sink contribution; a short strip y 270…~350 keeps the pre-existing ≤ 46 float class, shortened from 1,440 uu of corridor to ~80).
- **D3 — counts: 9 treads (not 01..12) ⇒ 35 hulls (not ~37).** Pure consequence of D1/D2; the berm allowance (4–8) was used at 6. All spec'd key classes honored: `approach_tread_NN`, `berm_block_NN`.
- **D4 — apply mechanism** (§2): the `_apply_box_collision()` write set over the live `ObjectTools.set_properties` lane (TASK-567 precedent) instead of the literal function call; readback deviation 0.000 is the equivalence proof.
- **D5 — S5 residual +43.83** vs the acceptance's "supported (small delta)": the C1 shelf rides ~0…35 uu ABOVE the centre ramp, so centre-flush treads cannot sit closer to the shelf surface without floating over the centre route. Supported is delivered; "small" is bounded at ≤ ~46 on the widened flanks. R1 cannot close this without per-flank stepped geometry (R2 territory).
- **D6 — berm tops overshoot the local visual where the mound is lower than its band's peak** (S2 −6.8, S3 −29.4; inner faces at exactly |x| = 720 rise above the leaning berm surface near the corridor edge). Capsule-unreachable tops (no route climbs 219+); the walls read as the visible raised masonry — the exact §6 design, accepted by the spec.
- **D7 — flank margins beyond the pinned volumes stay as shipped:** |x| > 1470 (the §2c grid-capped "≥1500" outer skirt) and the berm south shoulder y −2530…−2440 (sink bounded ≤ ~39 by the −2530 row's 140.3 max) — outside the pinned repair bounds, 750+ uu off the walk route.

## 6. DECLARES for TASK-612 (spec item 7 + what the re-trace needs)

- **Nav:** runtime-`Dynamic` ⇒ the grown collision is covered at PIE with NO save; the editor-time navmesh on the open (never-saved) map is irrelevant to the gate. Expect a nav delta vs the frozen editor fingerprint at PIE — that is the collision change working, not a regression; `NAV-§4`/`NAV-§12` settled-only grep still applies at your fresh boot.
- **Replication:** zero impact — static-mesh simple collision, both clients derive identically from the same asset.
- **Entry corridor:** |x| < 720 untouched by the blockers (inner faces exactly ±720); risers 29 + 8×14.5 + 29, all ≤ nominal 35 ⇒ units still enter (verify, per your item 3). Ground step unchanged at 29.
- **Interior:** hall slab (174) intact over the full hall + east annex (y 270…1200); corridor now supported at 145 (float ~17) — nothing walkable lost support; the slab volume removed (y −1170…270 outside corridor width) lay under uncarved solid masonry, capsule-unreachable (TASK-597 §2d's blocked columns).
- **Your invariance gate:** all four meshes must read 35/0 with max dev 0.000 vs `pipeline_manifest.json` `ucx.boxes` **in order** — the manifest array order is the BodySetup order (shell 1–19, slab 20, treads 21–29, berms 30–35). Elem names are `None` by design.
- **Your re-trace acceptance (measured expectations):** S1 −0.27 · S2 −6.82 · S3 −29.41 · S4 −0.26 · S5 +43.83 (supported — the declared D5 residual, not a red item) · **S6 +13.32 (the ≤ 14.5 gate)** · **F1 −17.66 (the ≈ −17 gate)**. The centre-route > 14.5 STOP has one declared exception zone: the D1 lip strip y −3616…−3560 (≤ 22.1), which contains no S-column.
- `L_Arena` hash lane: entry = exit = `B3DBC5D9…F8268` (§0) — your item (4) baseline.

**Next:** TASK-612 (build-master) — independent readback + S1–S6/F1 re-trace + fresh-boot PIE nav/entry sanity + the commit (explicit pathspecs: the five W10-R7 artifacts + handoffs + board; the in-flight ACCOUNTS code lane is NOT this commit's).
