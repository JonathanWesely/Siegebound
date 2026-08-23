# TASK-629 — [GH-4] THE INTERIOR REDESIGN GEOMETRY (art-director handoff)

**Status: COMPLETE — Cache-side only, ready for TASK-630 (UV/bake), TASK-631 (manifest v3), TASK-632 (crumble trio), TASK-634 (anchor audit).**
Fully headless Blender 5.1.2 `--background --factory-startup`. **No Content/ write, no manifest edit, no editor/MCP, no git, no Meshy (GH-R4: zero spend).**
Laws: GH-R1/R4/R7 · `WR-§1` · `TL-§2` · `SC-§15` · H1 default. Date: 2026-08-18.

---

## THE ONE-LINE RESULT

The castle interior is rebuilt as **one contiguous hollow volume** — the retained gate arch opens through a threshold (risers **2.6 / 13 / 13 uu**) into a clean corridor and a **2910 × 1140 grand hall, flat floor z 174.0 end-to-end, flat ceiling z 2160 (1986 uu clear)** — while the exterior is **vertex-identical** where kept: bounds `7313.576 × 7384.367 × 8082.610`, min-Z `−0.1657`, both byte-equal to the anchor gate, and `Content/RawAssets/Castle.fbx` is **untouched on disk** (sha256 `8b96f2b6…ae7b2` re-verified).

---

## 1. DELIVERABLES (all under `Tools/ArtPipeline/Cache/Castle/`)

| artifact | path / id |
|---|---|
| **the redesigned mesh** | `castle_redesign_v1.blend` — sha256 `71571edb20d01c9933ed19d7a2ed32863ecb74293dc59af0c0c2c6241f5d44ac` (object `SM_Castle`, 23,068 polys / **26,517 tris** / 13,052 verts; the 25 historical `UCX_*` imports kept in hidden collection `UCX_historical` — `TL-§2`: they remain historical, the manifest stays sole collision authority) |
| the build (reproducible) | `redesign_v1_build.py` — `--stage survey|survey2|survey3|build`; every number below is measured by it, not asserted |
| the readbacks | `redesign_report.json` (+ `redesign_survey{,2,3}.json` — the measurement base that sized every volume) |
| eyeballed previews | `previews_redesign/` — `before_*`/`after_*` × {ext_front_west, ext_back_east, ext_top, int_hall_to_door, int_hall_to_north, int_mouth_to_hall, int_floor_east} + `diffbase_*` (the diff baselines) |

### ⛔ NOT TOUCHED
`Content/**` (including `Castle.fbx` and every texture — byte-identical, hash re-verified) · `pipeline_manifest.json` · any `.uasset` · `L_Arena` · git · TASKBOARD.

---

## 2. THE DESIGN, AS BUILT (all UE units, conformed space — 631's derivation input)

```
DOORWAY (the retained gate aperture; GH-R1)
  arch cutter: centre x 18, half-width 903, spring z 1410, apex z 2313,
               y −2100 … −1020  (old gate_arch 900/1410/2310 grown +3 uu — see §5.2)
  facade mouth measured at y ≈ −1780 (facade first-hit −1751…−1793 across the front)
  jamb wall planes: x −885 / +921

THRESHOLD (the doorway sill — WR-§1 ≤40-uu law, the TASK-611 half-riser pattern)
  walk channel x −462 … 498:
    apron (kept exterior, visual ramp unchanged) arrives ≈ 145.4 at the mouth
    landing 1: z 148.0   y mouth … −1610
    landing 2: z 161.0   y −1610 … −1460
    hall floor: z 174.0  y ≥ −1460
  measured risers on the centre route: +2.56 / +13.0 / +13.0  (max 13.0 ✅ ≤ 40)
  flanks inside the tunnel (|x−18| > 480): flat 174.0 (matches the shipped plateau read)

CORRIDOR
  arch: centre x 18, half-width 753, spring 1410, apex 2163, y −1140 … 510
  wall planes x −735 / +771 (old ±750 grown +3; the 1560-uu collision gap
  x −762…+798 still brackets the visual walls exactly as shipped)

GRAND HALL (one volume, no partitions/columns/annex — H1 default)
  x −1920 … 990   (2910 wide — hall_main width retained)
  y  240 … 1380   (1140 deep — old 720 grown 58%)
  floor z 174.0 flat end-to-end (measured 174.00–174.69, see §4)
  ceiling z 2160 flat → clear height 1986 (measured 1986.0–2112.5) ✅ ≥ 1560
  MUST-CONTAIN check: hall_main envelope (−1920…990, 270…990, 174…1734) ⊂ hall ✅
```

**Junk-void fills (union, all measured sky-safe / buried):** F1 arcade-slot (x −2070…1290, y 1140…1560, z 150…2170) · F2 annex void (900…1740, 480…1080, 60…1900) · F3 back-wall tunnels (±480, 1150…1590, 1650…1860) · F45 passage sub-floor (−880…916, −1780…520, 60…174.5) · F6 west floor-tube (−2100…−1800, 475…660, 100…300) · F7 court under-roof (−1950…−1350, 230…630, 55…174.6) · F8 floor plate (−1870…940, 290…1360, 20…174.7) · F9 west wall plate (−2160…−1870, 235…1340, 25…2165) · F10/F11 corridor flank plates (−1005…−700 / 700…1085, −1783…528, 28…2168). Exact params live in the script header — **H1 amendments are one-constant edits + a re-run.**

---

## 3. GH-R7 ANCHOR CONTAINMENT — MEASURED

| anchor | result |
|---|---|
| **`CommanderNpcAnchor` (−465, 810, 174)** | interior, floor 174 under it; **min horizontal clearance 570.0** (probed 24 directions × z 300/600/954) ✅ ≥ 300. War table at +200 fwd → y ≈ 610, 370 clear ✅ |
| **`GetInteriorAnchorLocation` target (local 0,0)** | interior: floor 174.0 below, ceiling 2162 above ✅ |
| **gate aperture** | reproduced as-built (§5.2 table); `GateBlockerExtent (900,405,678)` / `RelativeLocation (18,−1575,852)` still span it ✅ |
| **torch anchors 1–6** | ⛔ **ALL SIX now off-wall** (expected case, GH-R7): measured distance to nearest wall face at z 954: anchors 1–3 (y 990 wall is GONE — hall extends to 1380): 390 each · anchor 4 (y 270 → wall now 240): 46.8 · anchor 5 (annex filled): 60 · anchor 6 (corridor wall −732 → −735): 41.4. **TASK-634 re-derives.** Suggested new sites (walls verified planar): north wall y 1380 at x −1435/−465/505 yaw −90 · south wall y 240 at x −1435 yaw +90 · east wall x 990 at (990, 810) yaw 180 · corridor west wall x −735 at (−735, −315) yaw 0 — mount z 954 has flat wall everywhere (walls are vertical 174→2160). |

---

## 4. THE MEASURED READBACK TABLE

| readback | measured | law / expectation | verdict |
|---|---|---|---|
| bounds | **7313.576 × 7384.367 × 8082.610** | identical to source | ✅ Δ0 |
| min-Z | **−0.1657** | identical | ✅ Δ0 |
| tris | **26,517** (was 28,698) | ≤ 30k | ✅ |
| verts | 13,052 (10,377 of them at byte-identical source positions) | — | ✅ |
| **exterior faces** | **19,331 — every vertex of every one matches the imported snapshot to <0.001 uu** | GH-R1 vertex-stable | ✅ |
| interior/new faces | **3,737**, tagged `interior_redesign` (BOOLEAN face attribute in the .blend), flat-shaded, **no UVs yet (630 owns)** | enumerated for 630 | ✅ |
| material slots | **`[TeamRegion, CastlePBR]`** exactly 2, order kept (a boolean-appended empty 3rd slot was detected and popped) | slot contract | ✅ |
| TeamRegion faces | **4,318** vs shipped 4,191 (+127 = high-roof membranes > z 2300, invisible) | ≈ shipped | ✅ (+3%) |
| floor flatness (1,612 down-casts, hall + corridor) | **174.00 min / 174.69 max, 2 columns > 0.5 off** (fill-top ≤ 0.7 proud at (500/560, −1420)) | flat 174 | ✅ |
| threshold channel probes | **0 off-spec** (148/161 landings exact) | — | ✅ |
| **max approach riser (centre route)** | **13.0** (2.56 at the sill seam) | ≤ 40 | ✅ |
| hall clear height | **1986.0 – 2112.5** | ≥ 1560 | ✅ |
| wall thickness (march-probes) | west 240 · west-hi 505 · east 750 · east-hi 597 · north 384 · north-hi 724 · south 1401 · corridor W **276** / E **314** · floor slab 154 · roof over ceiling 215–2495 | ≥ 150 | ✅ all |
| commander clearance | **570.0** | ≥ 300 | ✅ |
| aperture (visual open span by z) | z300: −672…668 (**identical to pre-redesign**) · z800–1400: −882…918 · z1800: −792…818 · z2100: −562…598 · z2250: −302…338 | as-built ±3 | ✅ (§5.2) |
| exterior render diff (same camera, before/after) | back-east **0.52%** px · top-down **2.12%** px (= the declared slot floors) · front-west 3.00% px (dominated by the through-door view, which now shows a lit floor instead of the dark dip) | ≈ 0 outside declared | ✅ |
| top-down heightfield diff (60-uu grid, 12k columns) | **129 changed columns: 128 inside the passage strip** (the F45 slot-floor raises + threshold, §5.1) **+ exactly 1 outside** at (60, 1860) +53 (§5.4) | 0 outside declared | ⚠️ 1, declared |

---

## 5. `SC-§15` DECLARED DEPARTURES (each measured, none hidden)

1. **The passage sky-slot floors now read 174.** The corridor/tunnel roof has small open-sky slots (14 columns, x −330…330, y −1160…−1040 + two roof holes at (−300,−1620)/(240,−1140)); their floors were the 127–130 dip — the on-route disfigurement itself. F45 fills them to the 174 plane. Visible ONLY straight down those slots (the 2.12% top-down diff); this IS the directive's clean floor.
2. **Doorway jamb regularized ≤ 3 uu.** Re-cutting with the old cutter profiles grazed the old cut surfaces coplanar (boolean film storm), so both arches grew +3 uu (900→903, 750→753). The z300 aperture row is **identical** to pre-redesign; upper rows grow ≤ 10 uu per side. Clearance arguments still cite TASK-555's collision numbers — unchanged.
3. **Interior junk voids filled (invisible from outside, render-diff-proven):** the east arcade through-slot now ends at a wall ~2,070 uu deep (was a through-void to x −1970) · the two back-wall tunnels at z 1700–1800 end at y 1590 (mouths kept, still read as deep holes) · the old east annex + its low tube are solid (annex is NOT in the must-contain; H1 one-volume default) · the gashed 16–32-uu corridor shell walls are backed by solid plates (the shard-mess in the before renders is gone).
4. **One top-down column at (60, 1860) reads +53** (a reconstruction patch tenting over a pre-existing hole in the back-wall crest skin). One 60-uu column, back wall top. Flagged for the 630 bake eyeball; trivially flattened there if it reads.
5. **Mesh hygiene, stated:** 196 residual open film edges + 248 inherited non-manifold edges remain, all buried inside masonry (the Meshy donor was never manifold); watertightness of the walkable interior is probe-verified (0 floor/threshold holes). Signed volume +51,589 m³.

---

## 6. NOTES FOR THE TASKS DOWNSTREAM

- **TASK-630 (UV + bakes):** the interior set = faces with `interior_redesign == True` (3,737). Exterior UV islands untouched (their verts/loops are source-identical). Slots exactly `[TeamRegion, CastlePBR]`. New faces currently carry CastlePBR (except ~127 high membranes) and **no meaningful UVs**. The sill seam (ragged strip where the kept apron meets landing 1, y ≈ −1780) and the arch-crown rim fragments at the mouth are jamb geometry — texture them as rough stone, don't chase the topology.
- **TASK-631 (manifest v3):** §2 has every plane. Interior slab: `floor_slab_hall` becomes y −1460…1380 span (top 174) across x −1920…990 (hall) + channel width in the corridor; channel treads: top 148 (y −1780…−1610) and 161 (−1610…−1460), x −462…498; flank plateaus 174 from y −1780; hall walls at x −1920/990, y 240/1380, ceiling 2160; corridor walls −735/771. Exterior approach hulls (treads 01..09, toe ring, seals): **base on the POST-hotfix set (GH-R3)** — my exterior is vertex-identical so they stay valid.
- **TASK-632 (crumble trio):** derive from `castle_redesign_v1.blend`; the interior is 11 axis-aligned volumes + 2 arch prisms (script header), so per-stage derivation is mechanical. `W6-R2`: collision identical per stage, both slots `MI_Castle_Crumble0N`.
- **TASK-634 (anchors):** §3 table + suggested torch sites. `HallThirdX` etc. in `Castle.cpp:104-124` still describe x thirds correctly; `HallMaxY` 990 and `AnnexMaxX`/corridor constants are now stale — the hall north wall is 1380, the annex is gone.
- **FBX export:** deliberately NOT done here (spec fence). Whoever exports (630/632 chain) must re-apply the `ue_handedness_precomp` mirror on export and embed the v3 manifest hulls, not the historical 25.

---

## 7. WHAT I DID NOT DO, ON PURPOSE

⛔ No Meshy call (GH-R4 — no cost sheet needed; the Blender-side rebuild sufficed). ⛔ No FBX/Content write, no manifest edit, no editor, no MCP, no git, no board edit. ⛔ Did not widen the corridor past +3 uu (H1's "(widened)" sketch would have trimmed the sky-slot rims — GH-R1 outranks it; say the word and it's a one-constant re-run). ⛔ Did not flatten the (60,1860) crest tent or the 2 proud fill columns (≤0.7 uu) — declared instead, one re-run away.
