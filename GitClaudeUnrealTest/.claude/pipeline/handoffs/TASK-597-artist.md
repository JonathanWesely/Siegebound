# TASK-597 — [WR-43] THE FLOOR-SINK DIAGNOSIS — measured SINK MAP + three-hypothesis adjudication (art-director handoff)

**Status: COMPLETE — evidence only. ⛔ NOTHING WAS REPAIRED (W10-R1). ⛔ NOTHING WAS WRITTEN outside this file (W10-R2).**
Fully headless: no Unreal editor, no MCP-Unreal, no PIE, no compile, no Git, no network. Blender **5.1.2** `--background --factory-startup`, **import-only** — zero exports, zero saves.
⛔ **`rescale_refined_fbx.py` was NEVER executed in any mode** (W10-R3; `qa/TASK-594.md` WARN-1 — its 27× guard catches the double-run but NOT a wrong-`--factor` run). Only its conformed-space import recipe (`rescale_refined_fbx.py:102-127`) was **copied into a scratchpad-only script**, never run from the tool.
Laws: `WR-§1` · `WR-§2` rows 8/11 · `TL-§2` · `SC-§34` · the RELAYED-DIAGNOSIS LAW · W10-R1..R4.
Date: 2026-08-16.

---

## THE ONE-LINE VERDICT

**The sink is REAL, MESH-SIDE, and has TWO measured mechanisms: (a) the approach treads ride 0–29 uu BELOW the visual ramp everywhere on the centre route by construction (median sink +24.2 uu over 6,396 walkable columns), and (c) — the dominant class — 1,503 walkable-looking flank/berm columns have NO castle hull under them at all, so the support is the arena ground at z ≈ 0 and the sink runs 48 → 497 uu.** Hypothesis (b), castle placement, is **REFUTED at ≤ 0.17 uu** (mechanism + numbers in §3b; final live word is TASK-598's). **The map (`L_Arena`) is innocent; the repair belongs at the mesh/manifest** — exactly flag F2's default.

---

## 1. SOURCES + RE-VERIFICATION (the RELAYED-DIAGNOSIS LAW: nothing inherited)

### 1a. `rescale_report.json` — the board's §2 claim CONFIRMED, then mined
`Tools/ArtPipeline/Cache/Castle/rescale_report.json` (18,107 B, mtime 2026-08-15 17:02:35) does hold the full approach chain, the 63-sample visual ramp profile, the per-tread float/sink scan, the through-route floor table, and the gate/cavity scans. Mined throughout §2.

### 1b. The FBX re-measured at the artifact — every anchor agrees with the shipped readback
`Content/RawAssets/Castle.fbx` (1,217,612 B, mtime 2026-08-15 17:02:34 — **byte-count identical to TASK-555 §1's shipped figure; file untouched by this task**), imported read-only into conformed (UE/manifest) space:

| anchor | shipped readback | **re-measured now** | |
|---|---|---|---|
| dims | 7313.576 × 7384.367 × 8082.610 | **7313.576 × 7384.367 × 8082.611** | ✅ |
| min-Z | −0.166 | **−0.1657** | ✅ |
| tris / welded verts | 28,698 / 14,057 | **28,698 / 14,057** | ✅ |
| UCX hulls | 25 | **25** | ✅ |

### 1c. ⚠️ The "377" prior is NOT in `rescale_report.json` — re-measured at the artifact, and it re-verifies
`handoffs/TASK-555-artist.md` §2e's *"max SINK 377, worst flank column"* appears **nowhere in the report JSON** (its per-tread `sink_max` peaks at 242.98, tread_04). Re-measured on my own grid: **+380.59 uu at mesh-local (990, −2350)** — visual 467.59 over `approach_tread_03` top 87.0. **Same site, same class, same magnitude within grid resolution ⇒ the prior is CONFIRMED as a prior, and it is not even the worst column (see §2c).**

### 1d. ⭐ WHICH COLLISION AUTHORITY IS LIVE — measured, not assumed (spec (2)(iii))
- **The live `SM_Castle` carries 25 `KBoxElem` / 0 convex** — `handoffs/TASK-566-buildmaster.md` R6(a) run-2 readback: `hulls=0 boxes=25`, authored by `Tools/reimport_meshes.py::_apply_box_collision()` from `pipeline_manifest.json` `ucx.boxes`. ⇒ **the `TL-§2` manifest-by-choice path is the live one**, not the FBX convex import that TASK-567's throwaway probe demonstrated possible. *(That readback is a written record, i.e. a relay — TASK-598's live `BodySetup` readback is the confirmation lane.)*
- **And the two authorities are numerically IDENTICAL:** all 25 FBX-embedded `UCX_SM_Castle_00..24` AABBs vs the 25 manifest boxes — **max deviation 0.000 uu** (center and size, every hull). ⇒ **this sink map is valid under either authority; nothing below turns on the choice.**

---

## 2. THE SINK MAP

**Method.** Conformed-space BVH ray-scan of the render mesh + the 25-box collision set. Per column `(x, y)` (mesh-local uu; each castle actor's transform maps these to world): **visual-top z** = highest upward-facing surface (face normal z > 0.3) under a zone cap (approach 520, interior 400 — everything above is wall/roof); **collision-top z** = highest covering box top ≤ visual + 50 (`HeroMaxStepHeight`), else **ARENA_GROUND (z ≈ 0 — assumption per TASK-569's terrain readback; TASK-598 traces confirm)**; columns under a hull straddling head-height = BLOCKED (unwalkable, excluded). **Signed delta = visual − collision: + = SINK, − = FLOAT** (the board's polarity convention). Walkable-visual = face ≤ 45° from +Z. Grid: approach x −1500…1500, y −3700…−1150, step 30 (≈ agent radius 34); interior x −2100…1860, y −1150…1200, step 60. *Point-column caveat: a hero capsule (r 34) shifts the tread flush-lines by ≤ 34 uu of phase; amplitudes are unaffected, and at the flank classes there is no hull within reach at all.*

### 2a. The centre line (x = 0) — the exact route a player walks

| y | visual z | collision z | **delta** | support |
|---|---|---|---|---|
| −3600 | 48.5 | 29.0 | **+19.5** | approach_tread_01 |
| −3500 | 55.1 | 29.0 | **+26.1** | approach_tread_01 |
| −3400 | 60.4 | 58.0 | +2.4 | approach_tread_02 |
| −3200 | 70.6 | 58.0 | **+12.6** | approach_tread_02 |
| −2900 | 85.8 | 58.0 | **+27.8** | approach_tread_02 |
| −2800 | 90.9 | 87.0 | +3.9 | approach_tread_03 |
| −2500 | 107.0 | 87.0 | **+20.0** | approach_tread_03 |
| −2400 | 112.3 | 87.0 | **+25.3** | approach_tread_03 |
| −2300 | 118.5 | 116.0 | +2.5 | approach_tread_04 |
| −2000 | 134.3 | 116.0 | **+18.3** | approach_tread_04 |
| −1800 | 144.9 | 116.0 | **+28.9** | approach_tread_04 |
| −1600 | 128.0 | 145.0 | −17.0 (float) | approach_tread_05 |
| −1300 | 128.6 | 145.0 | −16.4 (float) | approach_tread_05 |
| −1000 | 127.7 | 174.0 | −46.3 (float) | floor_slab_hall (corridor) |
| −300 | 127.3 | 174.0 | −46.7 (float) | floor_slab_hall (corridor) |
| +400 … +1200 | 174.0 | 174.0 | **0.0 (flush)** | floor_slab_hall (hall) |

⭐ **The saw-tooth is BY CONSTRUCTION:** TASK-555 §2c set each tread's y-boundary at the *measured flush point where the visual apron crosses that tread's top, "so no tread top ever rises above the mesh."* That choice guarantees **zero float on treads 01–04 by making the collision sit AT-OR-BELOW the visual across every tread** — the player's feet are 0…29 uu inside the visual ramp for the entire ~2,460-uu climb, resetting to ~0 at each riser and growing back to ~29. **A no-float design IS a permanent-sink design; that trade was never stated in those words, and this is the defect the player feels underfoot the whole way up.**

### 2b. Per-support aggregate — approach zone, 6,396 walkable columns

| support | n | median Δ | p90 Δ | **max SINK** | at (x, y) |
|---|---|---|---|---|---|
| **ARENA_GROUND (no hull!)** | **1,503** | **+110.1** | +216.4 | **+496.73** | (1050, −2350) |
| approach_tread_01 | 396 | +24.2 | +28.1 | +28.4 | (−90, −3460) |
| approach_tread_02 | 1,251 | +14.5 | +26.5 | +28.9 | (960, −2890) |
| approach_tread_03 | 1,133 | +17.2 | +54.9 | **+380.59** | (990, −2350) |
| approach_tread_04 | 978 | +28.6 | +58.0 | **+294.74** | (810, −2170) |
| approach_tread_05 | 1,068 | −15.3 (float) | +29.0 | +29.0 | (−450, −1780) |
| floor_slab_hall (edge) | 67 | −44.3 (float) | 0.0 | 0.0 | — |

Zone totals: **median +24.24** · sink > 5 uu: **5,157** cols · > 29: **2,289** · > 50: **1,894** · > 100: **997** · max float −49.7 (hall-slab edge at y −1150).
**Every sink > 100 lies at |x| ≥ 720** (y −3190…−1930): breakdown ARENA_GROUND 929 · tread_04 54 · tread_03 14. **The centre route (|x| < 720) never exceeds ~29.**

### 2c. The flank classes — where the visual apron outruns the collision

Visual apron/skirt width vs tread x-span (walkable-visual columns, v > 5):

| y | visual x-extent | tread x-span | beyond-span cols | **max sink beyond** |
|---|---|---|---|---|
| −3610 | −1500…1500* | −960…990 | 31 | +48.4 |
| −3250 | −1500…1500* | −960…990 | 34 | +97.4 |
| −2890 | −1500…1500* | −960…990 | 34 | +118.4 |
| −2530 | −1500…1500* | −960…990 | 33 | +140.3 |
| −2350 | −1440…1500* | −960…990 | 23 | **+496.7** |
| −2260 | −1500…1080 | −882…918 | 12 | +486.1 |
| −1990 | −1110…1170 | −882…918 | 14 | +218.9 |
| −1900…−1180 | −750…780 | −882…918 | 0 | — (gate passage: walls flank; collision WIDER than visual, no beyond-span) |

\* the grid stops at ±1500 — the walkable skirt reaches AT LEAST there; outer edge unmeasured.

- **C1 — the flat ground shelf** (y ≈ −3610…−2530, |x| ≈ 990…≥1500): near-FLAT walkable surface (face angle ~3°, e.g. (1050, −2920) v 116.3, ang 2.9°) with **no hull beneath — sink +48 → +140**, growing as the ramp climbs. A player approaching off-centre walks on invisible arena ground straight *through* the apron skirt, buried to the waist.
- **C2 — the berm/mound tops** (y ≈ −2440…−1990, |x| ≈ 990…1470): walkable-angle masonry tops at z 380–497, **ground support ⇒ sink +219 → +496.73** — full burial, the player vanishes inside the mound.
- **C3 — in-span berm encroachment** (treads 03/04, |x| ≥ 720): the berm geometry leans INSIDE the tread footprint — visual up to 467.6/410.7 over tread tops 87/116 ⇒ **sink +380.59 / +294.74 while standing on a real tread.** *(This is TASK-555's recorded 377-site, re-verified.)*

### 2d. Interior control — zone B, 1,746 walkable columns: **ZERO sink anywhere**

`sink_max = 0.00` across the gate passage, corridor and hall. The interior is **FLOAT-or-flush only**: corridor −45…−47 (max −49.9 at (720, 350)), tread_05 −15…−17 designed float, hall **flush at 174.0**. Hall-pocket probes ((750…950, 300…350)): visual 103–174, all BLOCKED by `keep_front_east` standing in the carve footprint (TASK-555 §4b, inherited) — float-polarity territory. ⇒ ⛔ **Per the board's polarity trap: the corridor 174-over-128 and the pocket CANNOT be what Jonathan saw — they are the OPPOSITE defect.** Sink exists ONLY on the approach.

---

## 3. THE THREE HYPOTHESES, ADJUDICATED

### (a) mesh collision-vs-visual mismatch after scale — ✅ **CONFIRMED, two distinct sub-mechanisms, quantified**
1. **The designed saw-tooth** (§2a): treads flush at their uphill edge, 0–29 uu below the visual everywhere else — median +24.2, ubiquitous, centre-route. *Strictly this is a re-derivation design consequence, not a scale error: the same flush-point rule at 1× produced the same class at a third the amplitude (1× medians ~15 per the report's own float/sink scan).* The ×3 scale tripled the amplitude into the visible range.
2. **The flank mismatch** (§2c C3): berm visual inside the tread span, sink to +380.6. The recorded 377 prior confirmed, ⛔ not the verdict — it is not even the worst column.

### (b) `ACastle` spawn-Z / placement offset — ⛔ **REFUTED (final live confirmation = TASK-598)**
- **Mechanism:** an actor transform moves visual and collision TOGETHER — it algebraically cannot produce a *differential* sink (a delta between two things it translates identically). And a spawn-Z error cannot persist under gravity while walking: the capsule settles onto whatever collision exists.
- **Numbers:** mesh's own base min-Z = **−0.166** (re-measured §1b); `handoffs/TASK-569-buildmaster.md` row (a) recorded the live `Castle_0` at bounds 7313 × 7384 with **min Z = 0** on the terrain ("no float, no sink"). ⇒ **placement delta ≤ 0.17 uu** — three orders of magnitude below the smallest measured sink class. *(Note for 598: the manifest hull bottoms reach z −60 (e.g. tread_01 bottom), so a colliding-bounds trace may legitimately read below 0 — that is hull embedment, not placement error.)*

### (c) arena ground-plane interaction — ✅ **CONFIRMED — and it is the DOMINANT class by depth**
**1,503 walkable-visual columns carry NO castle hull: their support is the arena ground at z ≈ 0.** Named in full in §2c (C1 + C2): the flat shelf +48…+140 and the berm tops +219…+497. 929 of the 997 sink-greater-than-100 columns are this class. **This is not an edge case: at |x| > ~990, more than a third of the visual approach width, the castle's approach has zero collision and the ground plane is doing all the work.**

---

## 4. THE CAUSE, IN ONE PARAGRAPH

Jonathan walked the approach. On the exact centre line his feet were 0–29 uu inside the visual ramp for the entire climb (median +24 — ankle-to-shin on a 176-uu character, resetting and regrowing across each of six treads), because the tread chain was authored flush-at-the-uphill-edge, which guarantees at-or-below-visual everywhere else; the moment he drifted more than ~720 uu off-centre the sink deepened to +55…+380 (berm encroachment over real treads), and beyond the tread span (|x| ≳ 990 — over a third of what the eye reads as walkable apron) there is no collision at all, so he stood on the invisible arena ground buried +48…+497 uu — waist-deep to fully vanished. *"The floor is not low enough"* is the precise phenomenology of both mechanisms: **the VISUAL floor rides above the SUPPORT everywhere on the approach**. The interior cannot contribute (zero sink measured; float/flush only), and placement is refuted at ≤ 0.17 uu.

---

## 5. WORST-COLUMN LIST FOR TASK-598 (mesh-local uu — apply each castle actor's transform; trace down from ~600 above the visual z)

| # | (x, y) | visual z | collision z | Δ | support (headless claim to confirm/refute) | class |
|---|---|---|---|---|---|---|
| **S1** | (1050, −2350) | 496.7 | 0.0 | **+496.7** | ARENA_GROUND — no castle hull | C2 berm, east |
| **S2** | (−1050, −2350) | 490.2 | 0.0 | **+490.2** | ARENA_GROUND — no castle hull | C2 berm, west |
| **S3** | (990, −2350) | 467.6 | 87.0 | **+380.6** | `approach_tread_03` | C3 in-span (the "377" site) |
| **S4** | (810, −2170) | 410.7 | 116.0 | **+294.7** | `approach_tread_04` | C3 in-span |
| **S5** | (1050, −2920) | 116.3 | 0.0 | **+116.3** | ARENA_GROUND — no castle hull | C1 flat shelf (ang 2.9°) |
| **S6** | (0, −2900) | 85.8 | 58.0 | **+27.8** | `approach_tread_02` | centre-line designed sink |
| **F1** | (0, −500) | 127.3 | 174.0 | **−46.7** | `floor_slab_hall` | corridor FLOAT — polarity control |

S1/S2/S5 are the hypothesis-(c) columns: **report what the support actually is** (ground plane / nothing / an unexpected hull). F1 should trace to 174 and prove the instrument can see both polarities.

---

## 6. COSTED REPAIR SKETCH — ⛔ INPUT to the manager's spec, NOT authorization (W10-R1)

**Owner by the evidence: art-director** (mesh/manifest + same-path reimport). No code-side cause was found; nothing here needs C++.

| option | what | fixes | cost | crumble-trio implication (`WR-§1`/W6-R2) |
|---|---|---|---|---|
| **R1 — manifest-only, cheapest** | (i) split each 29-uu tread into two 14.5-uu treads (6 → 12 risers; all ≪ 40; depths 91–313 still > agent Ø 68) → centre sink halves to ≤ 14.5; (ii) **berm blocker boxes** (~4–8) filling the C2/C3 berm volumes to their visual tops → burial becomes a wall, matching the visual read of raised masonry; (iii) widen treads 01–03 from ±960/990 toward the measured flat shelf (≈ ±1470, the C1 strip at face ≈ 3°) → the shelf becomes real floor. Hull count 25 → ~37. | C1, C2, C3 fully; halves (a) | manifest `ucx.boxes` edit + same-path reimport (`_apply_box_collision`), **no Blender, no texel, no visual change, no bake gates** | ⛔ **IMPLICATED** — pristine-collision change ⇒ re-derive `SM_Castle_Crumble01/02/03` to IDENTICAL collision, per-stage readback |
| **R2 — visual re-derivation** | lower/step the visual approach surface onto the treads (the "floor-fill" cutter TASK-555 §4c says the pipeline lacks) — the literal reading of *"the floor is not low enough"* | (a) fully | Blender vertex-z edit + FBX re-export + reimport; UVs intact if z-only; previews + gates re-run; heaviest | collision untouched, BUT the trio go visually stale at the approach ⇒ visual re-derive of all three |
| **R3 — recommended** | **R1 now; hold R2** unless the residual ≤ 14.5-uu saw-tooth still reads on screen | — | — | as R1 |
| **F3 rider** | tread_05 → y +270, `floor_slab_hall` starts there ⇒ corridor float 46 → 17 | corridor float (NOT his finding) | free in the same reimport | manager default: TAKEN if approach hulls re-derive |

Numbers for the spec: shelf extent per §2c table; berm volumes bounded by y −2440…−1930, |x| 720…1470, tops 380–497; tread splits derive from the report's 63-sample visual profile (already on disk).

---

## 7. SCOPE-FENCE COMPLIANCE (W10-R2/R3)

- **Writes:** this file only. Script + raw grid JSON live in the session scratchpad (ephemeral, outside the repo) — **every load-bearing number is IN this handoff.**
- **Not touched:** `Content/RawAssets/Castle.fbx` (read-only import; byte count 1,217,612 unchanged) · `SM_Castle.uasset` · `pipeline_manifest.json` (read, `encoding="utf-8"`) · `L_Arena` · any `Content/` path · the crumble trio · Git · editor/MCP-Unreal.
- **`rescale_refined_fbx.py`: read, never executed.** No mode of it ran, write-capable or otherwise.
- **Reproduce:** import `Castle.fbx` per the conformed-space recipe at `rescale_refined_fbx.py:102-127` (axis_forward −Z, axis_up Y, bake matrix_world, Y-mirror + winding flip, ×100 m→uu), BVH downward multi-hit scan on the grids of §2, collision from manifest `ucx.boxes` (center + full size). Verification anchors in §1b must reproduce first.

**Next:** TASK-598 (build-master, editor-gated) confirms S1–S6/F1 live at BOTH castle actors; the manager specs the repair from this file + 598's.
