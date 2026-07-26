# TASK-290 handoff → TASK-291 (build-master)

**M7.6 Phase 5 — Vista-ring + POI + neutral gold-node-prop asset prep**
Branch: `m7.6-arena10x` · Editor on L_MainMenu (PIE not running) · art-director · 2026-07-24

Assets are PREPPED and SAVED. TASK-291 does ALL `L_Arena` placement + the fog/light polish + the branch commit. I did **not** edit `L_Arena` (still on L_MainMenu) and touched **no** Git. Everything below is ADDITIVE — new duplicated assets, no shared scatter donor was edited in place.

---

## 1. Assets created (all in `/Game/Meshes/`, all SAVED)

### Vista-ring silhouette palette (4 meshes)
Far distant backdrop only. Big silhouettes; a mix of grey rocky peaks + green grassy foothills reads as a believable mountain/foothill ring.

| Asset | Donor (duplicated from, NOT edited) | Look | Native bounds (uu) | Suggested placement scale |
|---|---|---|---|---|
| `SM_Vista_01` | `/Game/Realistic_Rocks/Meshes/SM_Rock_20` | tall grey craggy **peak/spire** | 322×269×**504** | 10–14× (→ ~50–70 m tall) |
| `SM_Vista_02` | `/Game/Meshes/SM_Hill_03` (ridge) | **green grassy ridge/massif** (wide) | 2200×1700×428 | 6–9× (already wide) |
| `SM_Vista_03` | `/Game/Meshes/SM_Hill_02` (hill) | **green grassy conical foothill** | 2200×2200×400 | 6–9× |
| `SM_Vista_04` | `/Game/Realistic_Rocks/Meshes/SM_Rock_11` | rounded grey **craggy massif** | 283×273×292 | 12–16× (→ ~40–50 m tall) |

Distribute grassy foothills (02/03) lower/nearer and rocky peaks (01/04) taller/farther, or intermix, and vary yaw so repeats don't read as clones.

### POI landmark palette (4 meshes → place 6–10 instances)
In-field visual landmarks, no gameplay. Medium scale.

| Asset | Donor | Look | Suggested placement scale |
|---|---|---|---|
| `SM_POI_01` | `/Game/Realistic_Rocks/Meshes/SM_Rock_31` | flat **rubble / rock-cluster outcrop** ("rocky camp") | 3–4× |
| `SM_POI_02` | `/Game/Realistic_Rocks/Meshes/SM_Rock_33` | low **rock cluster** (variety) | 3–4× |
| `SM_POI_03` | `/Game/Realistic_Rocks/Meshes/SM_Rock_11` | craggy **boulder landmark** | 2.5–3.5× |
| `SM_POI_04` | `/Game/Realistic_Rocks/Meshes/SM_Rock_20` | upright **rock monolith** | 2–3× |

### Neutral gold-node visual props (1 mesh → place 2 instances)
| Asset | Donor | Look | Suggested placement scale |
|---|---|---|---|
| `SM_GoldNodeProp` | `/Game/Meshes/SM_GoldNode` | bright emissive **gold crystal cluster** (keeps `M_GoldGlow`, slot `GoldNodePBR`) | 1.5–2× |

**VISUAL-ONLY** — the neutral-node CAPTURE mechanic is explicitly NOT designed (Standing-backlog hook). These are dressing, NOT `AGoldNode` gameplay actors and NOT `ACaptureZone`. Place as plain `StaticMeshActor`s.

---

## 2. Flags — what I baked vs what TASK-291 must set

**Baked into every one of the 9 assets (done, verified):**
- **Collision REMOVED** (`remove_collisions`) → every instance is **NoCollision by default**, generates **no navigation collision**. ⇒ **TASK-291 needs NO nav rebuild** for these props (they can't affect the RecastNavMesh). This is why I duplicated instead of referencing the shared donors — stripping the donor's collision would have broken the in-field scatter.
- **Nanite OFF** on all (verified `is_nanite_enabled == false`). See the Nanite decision below.

**Per-INSTANCE flags TASK-291 sets on each placed component (these are component-level, not asset-level — I could not bake them):**

For the **vista-ring** instances (CONVENTIONS "M7.6" Nanite vista amendment), set on the `StaticMeshComponent`:
- `CastShadow = false`
- `bVisibleInRayTracing = false`  (`SetVisibleInRayTracing(false)`)
- `bCanEverAffectNavigation = false`
- Collision already None (baked) — leave `NoCollision`.
- Never a gameplay actor; never inside the play bounds.

For the **POI** and **gold-node-prop** instances: NoCollision is already baked; `bCanEverAffectNavigation=false` is implied by no-collision but set it explicitly to be safe. Shadows can stay ON for POIs (they're in-field, grounded) — your call; gold-node props are emissive, shadow irrelevant.

### Nanite decision (FLAGGED — default now, upgrade later)
The amendment reserves **Nanite ON** for real high-poly Megascans cliffs. **We have NO Megascans/cliff/mountain meshes** in the project (searched whole project: `Cliff` = none; `Mountain` = only a Landmass LayerInfo). The vista meshes here are **repurposed low-poly donors** (~18k tris each) — the task explicitly makes Nanite **optional** for placeholders ("your call, but never on a shared donor"). I left them **Nanite OFF** because low-poly meshes gain nothing from Nanite. Keep them OFF at placement.

---

## 3. Suggested placement layout (parametric — TASK-291 finalizes against live bounds)

### ⚠ Bounds note — READ FIRST
The task/TASK-291 spec cites a "**±12,000 play bounds**", but documented branch geometry has **castles at X≈±25,000** (spawn-box appendix: Castle_Red≈+25,000; plan file: retired gold-node fallback discs at ±24,200) with **centerline X=0** and **`CaptureZone_Center` at (0,0,0)**. So ±12,000 is NOT the X half-extent — most plausibly it is the **Y half-width** of the play field (field ≈ 50,000 long on X × ≈24,000 wide on Y). **The editor is on L_MainMenu**, so I did not read the live `NavMeshBoundsVolume` / ground-slab extents. **TASK-291: when L_Arena is loaded, read the actual `NavMeshBoundsVolume` bounds + ground-slab bounds and clamp the vista ring OUTSIDE them, and nudge any POI/gold-prop off a scattered mine/hill.** The coordinates below are deliberately generous so they stay outside the field even if Y is wider than ±12,000.

### Vista ring (~14–18 instances, drawn from `SM_Vista_01..04`)
A rectangular ring on the far perimeter, **well outside** the field. Suggested perimeter box: **X = ±34,000**, **Y = ±20,000** (both comfortably beyond castles ±25,000 and the ±12,000 Y edge). Base Z at ground level (≈0); sink slightly if the base would read as floating — they are backdrop, no collision, viewed from afar so ground underneath is not required.

Suggested positions (yaw random; scale per the table above):
- Far X-ends (behind each castle): `(−34000, 0)`, `(−32000, ±9000)`, `(+34000, 0)`, `(+32000, ±9000)` — 6, peaks (Vista_01/04).
- Long sides: `(±22000, +20000)`, `(0, +20000)`, `(±22000, −20000)`, `(0, −20000)` — 6, mix ridges (Vista_02/03) + peaks.
- Corners: `(±30000, ±18000)` — 4, tall peaks (Vista_01).
Vary the ring radius ±3,000 per instance so the silhouette is irregular, not a clean rectangle.

### POI landmarks (8 instances suggested — 6–10 acceptable)
Off the central lane (units march near Y=0) and clear of castle keep-clears. All at **|Y| ∈ [3,500, 7,000]** (off-lane) and **≥7,000 uu from either castle**; none near the origin capture zone. Symmetric pairs for visual balance (branch scatter is asymmetric-random, so exact symmetry is not required):

| # | Asset | Position (X, Y) | Scale |
|---|---|---|---|
| 1 | `SM_POI_03` | (−15000, +6000) | 3 |
| 2 | `SM_POI_01` | (−9000, −6500) | 3.5 |
| 3 | `SM_POI_04` | (−19000, −3500) | 2.5 |
| 4 | `SM_POI_02` | (−5500, +7000) | 3 |
| 5 | `SM_POI_03` | (+15000, −6000) | 3 |
| 6 | `SM_POI_01` | (+9000, +6500) | 3.5 |
| 7 | `SM_POI_04` | (+19000, +3500) | 2.5 |
| 8 | `SM_POI_02` | (+5500, −7000) | 3 |

These do NOT touch the castle↔castle path (near Y=0) and are NoCollision, so **traversability is unchanged** and no nav rebuild is needed. Clamp |Y| down if the live field is narrower than ±12,000.

### Neutral gold-node props (2 instances)
Mid-field, flanking the contested center (thematically near `CaptureZone_Center` at origin), OFF the lane:
- `SM_GoldNodeProp` at **(0, +5000)**, scale 1.5–2×
- `SM_GoldNodeProp` at **(0, −5000)**, scale 1.5–2×

Alternative if you prefer them reading as "prizes between the lines": `(−4000, +4000)` and `(+4000, −4000)`. Emissive gold draws the eye to mid — good contested-treasure read.

---

## 4. Flags / upgrades for Jonathan (morning)

1. **Dedicated Nanite Megascans cliffs = the eventual vista upgrade.** Current vistas are repurposed low-poly rock/hill placeholders (Nanite OFF). When real Megascans cliff/mountain meshes are acquired (Fab/Quixel), duplicate them to `SM_Vista_*`, set **Nanite ON** + the same per-instance vista flags, and swap. Non-breaking (they're pure dressing, no gameplay refs).
2. **Pre-staged Nanite candidate already in-project:** `/Game/Fab/Rocks/highpoly_rocks_free_download/StaticMeshes/highpoly_rocks_free_download` is a genuine **1.4M-tri** high-poly rock formation (bounds ~60,000×71,000×11,800). I did NOT process it (1.4M-tri Nanite build in the live editor is heavy/risky), but it is the ready-made "real Nanite vista" upgrade — duplicate it into a `SM_Vista_*`, enable Nanite, scale to taste.
3. **FAB-005 watchtowers = POST-purchase slot-in** (Jonathan's Fab purchase not yet done). Do NOT block Phase 5 on them. When fulfilled, they drop in as extra POI landmarks (visual-only) with the same NoCollision treatment.
4. **Perf:** ~14–18 vista instances × ~18k tris (Nanite OFF) ≈ 250–320k tris on the far ring, drawn every frame (no cull benefit configured). Acceptable for a distant static ring; if the W-gate fps watch flags it, add cull distances or switch to the Nanite highpoly donor (#2).

---

## 5. Prep manifest (for `git diff --stat` sanity at TASK-291 commit)
New `.uasset`s under `Content/Meshes/` (branch-owned dressing, expected in the commit alongside `L_Arena.umap`):
```
SM_Vista_01  SM_Vista_02  SM_Vista_03  SM_Vista_04
SM_POI_01    SM_POI_02    SM_POI_03    SM_POI_04
SM_GoldNodeProp
```
No source-code / DA change from me. Raw FBX rule N/A (these are in-editor duplicates of already-imported assets, not new FBX imports).
