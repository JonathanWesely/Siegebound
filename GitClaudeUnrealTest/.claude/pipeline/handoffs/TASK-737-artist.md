# TASK-737 — [LADDER-1] `SM_WatchTower` re-authored: ladder + compact base + the SAME platform

**Agent:** art-director · **Date:** 2026-09-01 · **Law:** `TOWER-§8.2` / `§8.3` / `§8.4(A)` / `§2a` (nav constraints) / "Static-mesh SOCKET names"

**Status: mesh + collision + sockets AUTHORED, MEASURED, and VERIFIED IN-ENGINE by a scratch import.
The same-path reimport of `/Game/Meshes/SM_WatchTower` is deliberately NOT done — it is TASK-742's
editor step (board spec item 6) and RULING 3 forbids landing the mesh before the traversal code.**

⭐ **ONE DECLARED DEVIATION, and it is geometric, not stylistic — §5. Nothing was quietly adjusted.**

---

## 1. As-built vs the PINNED geometry — every row measured, none asserted

Measured by raycasting and exact signed-distance against the **exported** hull soup and render mesh,
then **re-measured from the FBX after a clean re-import**, then **read back out of Unreal**.

| Pinned (`TOWER-§8.3`) | Required | **AS BUILT** | Verdict |
|---|---|---|---|
| Platform rise | 1,200 uu | **1,200.0** — deck collision top | ✅ **SURVIVES** |
| Deck | 600 × 600 @ z 1200 | **600 × 600 @ 1200.0** | ✅ **SURVIVES** |
| Body footprint | 600 × 600, X/Y ∈ [−300,+300] | ground collision half-extent **[300.0, 300.0]** | ✅ EXACT |
| `LadderFoot` | (−450, 0, 0) | **(−450.0, 0.0, 0.0)** | ✅ EXACT |
| `LadderTop` | (−150, 0, 1200) | **(−150.0, 0.0002, 1200.0)** | ✅ EXACT (0.2 µu float noise) |
| Climb line length | 1,236.9 uu | **1,236.9317** | ✅ (= √(300²+1200²), the law's rounding) |
| Lean | 76.0° | **75.9638°** | ✅ (= atan(4); the law's 76.0 is its rounding) |
| Ladder clear width | ≥ 120 uu | **132.0** clear / 172.0 overall | ✅ |
| Standoff (capsule↔body) | ≥ 56 uu | **59.624** worst case, over the whole line | ✅ (⚠️ deck slab: §5) |
| Deck parapet | outside the deck, if any | **NONE — and that is forced, §6** | ✅ |
| `MI_WatchTower_PBR` | reused, not re-authored | **untouched, not opened** | ✅ |
| Paths / names | in place, same | `SM_WatchTower` · `WatchTower.fbx` | ✅ |

**Deck flatness — the number the height-damage contract rests on:** collision deviation from z = 1200
is **max 0.000191 uu / mean 0.000038 uu over 3,600 samples, ZERO ray misses**, and **0.000191 max**
across the surviving nav polygon X,Y ∈ [−236, 236]. That is float32 storage noise: the deck plane is
the hull's own +Z face, so the deviation is zero *by construction* and the measurement confirms the
construction survived both export and the UE import.

**Socket arithmetic re-checked against what the numbers are FOR** (not just that they match):
ground nav starts at **x ≤ −364** (600×600 carve + 2 cells of erosion) ⇒ `LadderFoot` at −450 clears
it by **86.0 uu**; deck nav poly is **X ∈ [−236, +236]** ⇒ `LadderTop` at −150 sits **86.0 uu** inside
it. **Both 86s are as the law derives them.**

---

## 2. ⭐ THE SOCKETS — shipped on the mesh, and named exactly

| Socket | Local coords, read back from the FBX | Parent |
|---|---|---|
| **`LadderFoot`** | **(−450.0, 0.0, 0.0)** | `SM_WatchTower` |
| **`LadderTop`** | **(−150.0, 0.0002, 1200.0)** | `SM_WatchTower` |

In the FBX the nodes are **`SOCKET_LadderFoot`** / **`SOCKET_LadderTop`**, parented to the single
render node; **UE strips the `SOCKET_` prefix**, so the shipped socket names are exactly `LadderFoot`
and `LadderTop` — PascalCase, no prefix, no underscore, as `TOWER-§8.4(A)` pins them.

⭐ **Why the handedness mirror cannot corrupt them:** the export applies the shipped TASK-348 pre-comp
(mirror-Y + winding flip). **Both sockets sit on y = 0, where a Y negation is a no-op**, and x/z are
untouched by it. That is not luck — it is why the pinned coordinates are safe to ship through this
export path at all.

⚠️ **THE ONE ASSERTION I COULD NOT MAKE, STATED PLAINLY RATHER THAN GLOSSED (§8).**

---

## 3. ⭐ Collision — hand-authored, and READ BACK OUT OF UNREAL

**8 hand-placed exact convex primitives, exported as `UCX_SM_WatchTower_00..07`. There is NO convex
decomposition anywhere in the chain** — not in Blender, not at import.

**Read back from a real UE import of this exact FBX** (`bodySetup.aggGeom`):

```
convexElems  8      boxElems []   sphereElems []   sphylElems []
bIsGenerated false  on ALL 8      <- hand-authored, never decomposed
```

| # | Role | Verts | Bounds read back from UE |
|---|---|---|---|
| 00 | **plinth — THE 600×600 ground nav carve** | 8 | X[−300,300] Y[−300,300] Z[0,160] |
| 01 / 02 | keep wall +Y / −Y | 8 | X[±276] Y[196,276] Z[160,1160] |
| 03 | keep wall +X (east) | 8 | X[196,276] Z[160,1160] |
| 04 / 05 | west bay pier +Y / −Y | 8 | X[−276,−196] Y[130,196] Z[160,1160] |
| 06 | interior fill wedge (45°) | 6 | X[−276,196] Z[160,632] |
| 07 | **deck slab — its +Z face IS z = 1200** | 8 | X[−300,300] Y[−300,300] **Z[1160, 1200]** |

**⛔ THE LADDER HAS ZERO HULLS, AND THAT IS THE DESIGN, NOT AN OMISSION.** Both halves of the
dispatch's warning are satisfied *by the same decision*: (a) the ladder is **not walkable** — it is
traversed by a nav link, and at 76° Recast would shred it anyway (2.4× the 32.005° ceiling); (b) a
hull at its foot would **carve ground nav in the exact cell a unit must stand in to start climbing.**
Measured: **no collision anywhere over `LadderFoot`'s footprint** (72 samples across a 34 uu radius
at 5 heights, zero hits; nearest collision surface **116 uu** away), and the westmost collision
vertex in the whole asset is **x = −300**.

**Interior fill (hull 06) earns its place:** without it the plinth top inside the shaft is a
496 × 416 flat floor 160 uu up — an **unreachable nav island**. A 45° top plane is 1.4× over the
ledge-filter ceiling, so no span survives on it. Its closest approach to the climbing capsule is
**82.9 uu**.

---

## 4. ⭐ The shape the pinned numbers FORCE — stated because it is the whole design

The climb line crosses the body's west face plane (x = −300) at **z = 600**, so **for the top half of
the ascent the climb line is inside the body's plan footprint.** A solid 600×600×1200 block therefore
**cannot** satisfy the ≥56 uu standoff — it holds only to **z = 240**.

⇒ The resolution that keeps **every** pinned number is a **hollow keep with an open west bay**
(|y| < 130, 260 uu wide, full height): the ladder leans *through* the bay and rises inside it. The
ground carve stays a full solid 600×600 plinth, which is exactly what `LadderFoot`'s 86 uu is
computed from. **No pinned number was moved to get here.**

---

## 5. ⚠️⚠️ THE ONE DECLARED DEVIATION — the climb line's last 40.8 uu is inside the deck slab

**Measured:** the capsule's clearance to hull 07 is **−53.95 uu**; the climb line itself is inside the
deck slab for its final **40.832 uu — 3.30 % of the 1,236.9 uu ascent**.

⛔ **This is NOT fixable in the mesh, and I did not pretend otherwise or quietly move a coordinate.**
`LadderTop` is pinned **150 uu inside a solid 600×600 deck** and is approached from **below at 76°**.
Therefore the final stretch of **any** straight climb line ends inside **any** solid deck. I checked
the three escapes and all fail against the law:

- **A deck hatch/notch at the arrival** — any opening containing the line's last stretch also contains
  the socket, leaving `LadderTop` over a hole. That is precisely the castle-floor defect class the
  spec names: every readback correct, nothing can use it. ⛔ Refused.
- **A thinner slab** — reduces but cannot remove it; the capsule (176 uu tall) straddles the slab from
  a bottom-Z of 964 upward regardless of thickness. The slab is already a lean **40 uu**.
- **Moving `LadderTop` outward** — ⛔ forbidden, and it is the navmesh arithmetic itself.

⭐ **WHAT THE CODE LANE NEEDS TO KNOW (TASK-734/738), because this is the one place the mesh constrains
them:** a **swept** move along the final ~41 uu will **block against the deck slab and the climb will
stall just below the deck** — a feature-breaking failure that looks like nothing is wrong. The normal
remedy is the standard one for scripted traversal: drive the last stretch with a **non-swept /
teleport-style move** (or disable collision for the ascent), which `MOVE_Flying` interpolation
supports. **Flagging, not prescribing — this is their call and their file.**

Everything else on the tower clears the standoff with margin: **worst body clearance 59.62 uu**
(plinth, at t = 245) and **worst render-dressing clearance 96.0 uu**.

---

## 6. ⭐ What I would have changed for art reasons — and did NOT

- **A parapet / merlons.** The old tower had them and Jonathan liked that tower. ⛔ **Not shipped, and
  it is forced both ways:** `TOWER-§8.3` forbids a rail carved **out of** the deck (it seeds erosion
  from its own face), and a rail **outside** the deck would breach the pinned 600×600 footprint and
  the 750 uu total span. **Declared rather than smuggled in.** The team colour therefore lives in a
  **flush inlay** in the deck top, which costs zero nav.
- **`LadderFoot` is 13.6 uu outside the render bounds** (mesh min x = −436.418; total span **736.4 uu**,
  inside the pinned ~750). This is correct, not drift: the ladder plane is offset 22 uu *toward* the
  tower so the climbing unit is **outboard** of the stiles — a unit stands *at* the foot of a ladder,
  not inside it. I did not stretch the ladder to −450 just to make the socket sit within the bounds.
- **Two things I tried, rendered, and rejected on the picture** (both are in the script's comments so
  nobody re-adds them): a **team-coloured corbel band** — read as a blue slab hanging under the deck;
  and **full-height corner quoins** — they framed each wall into one recessed panel and the side
  elevation read as a **fridge door**. Replaced with horizontal banding (two string courses) and a
  rhythm of window slits, which is what actually gives a 600 × 1200 shaft its scale.

**Cosmetic residual, declared:** the ladder head projects **108.7 uu above the deck** over a
~39 × 168 uu patch at x ∈ [−133, −94]. It is **render-only with zero collision**, so it cannot enter
voxelization or cost a nav cell, and a unit standing on `LadderTop` clears it by **37.1 uu**. A unit
that walks directly under it will visually clip it. **Kept deliberately: from the top-down RTS camera
it is the only cue that says "the ladder arrives here."**

---

## 7. ⛔ THE TRAP — verified still closed, and its stale rows repaired

`Tools/reimport_meshes.py::_category_of()` defaults an unknown CardID to `"unit"`, whose branch runs
`remove_collisions()` + `set_convex_decomposition_collisions()` — it would **delete all 8 authored
hulls and report DONE.** Simulated against the shipped function:

```
WatchTower                  -> category=building  boxes=[]  -> _apply_box_collision
                               -> WARN + LEAVES imported collision      [TRAP CLOSED]
CONTROL (no manifest entry) -> category=unit -> AUTO-DECOMPOSITION      [the trap]
```

⚠️ **But the entry had gone STALE in a way that would have misfired:** its `_acceptance` still said
*"expect `convex_hull_count == 14`"* and its dims/origin still described the ramp. A gate reading that
against the new mesh either raises a false STOP or, worse, invites someone to "repair" it. **Updated**
(`WatchTower` is the **only** asset entry that changed; all 27 assets and every non-asset key are
byte-identical otherwise; CRLF preserved, ASCII-only, valid JSON): hull count **14 → 8**, new dims /
origin / walkable-surface rows, and a new **`sockets`** block recording both pinned coordinates and
why they are navmesh arithmetic. `category: "building"` and `ucx.boxes: []` are **untouched**.

⛔ **No bulk reimport sweep was run.**

---

## 8. ⚠️ WHAT IS STILL OWED — one assertion MCP cannot express

**Everything below was read back out of Unreal from a real import of this FBX** (into a scratch path,
then deleted — `/Game/Meshes/SM_WatchTower` was **never touched** and is **not dirty**):

| Read back from UE | Value |
|---|---|
| convex hulls / boxes | **8 / 0** |
| `bIsGenerated` | **false on all 8** |
| deck collision top | **z = 1200 exactly** |
| bounds | X[−436.418, 300] Y[±300] Z[0, 1308.656] |
| triangles | **836** (budget 20,000) |
| material slots | **["TeamRegion", "WatchTowerPBR"]** — order correct |
| Nanite | **false** ✅ |

⚠️ **`UStaticMesh::Sockets` is not a reflected-editable property, so the Unreal MCP surface cannot read
socket names or transforms.** I proved them at the file level instead, with an **independent**
verifier that re-imports the shipped FBX and restates the contract from scratch rather than importing
the build script's own constants (`verify_watchtower_fbx.py`, **13/13 rows pass**). **The engine-side
socket assertion is genuinely owed and belongs to TASK-742's import step:**

> After the same-path reimport, confirm `SM_WatchTower` has **exactly two** sockets named
> **`LadderFoot`** and **`LadderTop`** (Socket Manager, or `len(sm.sockets) == 2` /
> `[s.socket_name for s in sm.sockets]`), at **(−450,0,0)** and **(−150,0,1200)**.
> ⚠️ **A missing or misnamed socket is SILENT** — `TOWER-§8.4(A)` makes the code degrade open, so the
> tower still works and logs one warning. Nobody would notice from play.

---

## 9. ⛔ Why the same-path reimport was NOT done here

Board spec item **(6)**: *"Same-path reimport is TASK-742's editor step; ⛔ never claimed done here."*
And **RULING 3**: the re-authored mesh **deletes the ramp** while the traversal code **replaces it**,
so they may not land separately — `main` must never hold a tower nobody can ascend. **Reimporting now
would create exactly that intermediate state** in the working tree. The recipe is mechanical:

1. Reimport `Content/RawAssets/WatchTower.fbx` **over** `/Game/Meshes/SM_WatchTower` — ⛔ **same path,
   never delete-and-recreate** (`BP_Building_WatchTower` and the placement ghost resolve it).
2. Reimport the three PNGs over `/Game/Textures/T_WatchTower_{D,N,ORM}` — **`_D` sRGB ON**, **`_N`
   normal-map**, **`_ORM` LINEAR / sRGB OFF**. ⚠️ **Read the sRGB flag back** — "the ORM imported
   sRGB" is one of this lane's three recorded silent defects.
3. Slots stay `[TeamRegion → MI_TeamColor_Blue, WatchTowerPBR → MI_WatchTower_PBR]`; **Nanite OFF**.
4. ⛔ **Read back: 8 hulls, `bIsGenerated == false` ×8, box_count 0, deck z 1200, both sockets.**
   **Zero hulls is a STOP.**

⚠️ **The textures MUST be reimported, and this is a declared consequence rather than a re-authoring.**
`MI_WatchTower_PBR` is **reused and was never opened** — same asset, same parent, same three parameter
bindings, same three texture paths. But `T_WatchTower_{D,N,ORM}` are a **baked UV atlas**, not tiling
library textures: a re-authored mesh has a new UV layout, so the old atlas would sample as garbage.
They were re-baked with the **identical** palette, shader recipe, delight profile and resolution.
Gates: albedo floor **0.3837 vs 0.2536 PASS** · ORM metal max **0.0** · anti-bleach operative guard
**not tripped** · retention 1.3499 with the AO-divide component 1.4927 ⇒ **residual 0.8904**, i.e. the
palette is within ~11 % of its declared intent (same regime as TASK-727's declared 1.4013).

---

## 10. Files

**Source (checked into Git alongside the .uasset):**
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\WatchTower.fbx`
  — 62,076 bytes, sha256 `07d0b50849d733cb7e514569…`, **11 nodes** (1 render + 8 UCX + 2 SOCKET)
- `…\Content\RawAssets\Textures\WatchTower\T_WatchTower_{D,N,ORM}.png` — 2048², re-baked

**Tooling (`Tools/**/*.py` is CODE ⇒ the tooling QA gate applies):**
- `…\Tools\ArtPipeline\build_watchtower.py` — re-authored in place; every dimension a named constant,
  the pinned values are literals and all measurements are taken *against* them
- `…\Tools\ArtPipeline\verify_watchtower_fbx.py` — **NEW**, the 13-row round-trip gate
- `…\Tools\ArtPipeline\pipeline_manifest.json` — `WatchTower` entry only (§7)

**Evidence (gitignored cache):** `…\Tools\ArtPipeline\Cache\WatchTower\{watchtower_report.json,
roundtrip_report.json, previews/*.png}` — all five previews **eyeballed before anything entered the
editor**: hero, side elevation, ladder detail, top-down, collision hulls.

**Reproduce:**
```
blender.exe --background --factory-startup --python-exit-code 1 --python Tools/ArtPipeline/build_watchtower.py
blender.exe --background --factory-startup --python-exit-code 1 --python Tools/ArtPipeline/verify_watchtower_fbx.py
```

---

## 11. Integration notes

- **Pivot unchanged in kind:** origin at the **centre of the tower at ground level**, `min_z = 0.0`
  exactly. **Scale 1.0.** ⚠️ **The ladder is now on −X** (the old ramp ran along +X) — anything that
  assumed "local +X is down the ramp" is stale.
- **Footprint collapsed 3,058 → 736 uu** (span, X). `TOWER-§7`/TASK-735 needs **no spec edit** — it
  takes the footprint from the mesh, which is the dividend of that rule.
- **Mesh:** 836 tris · UV layer exactly `UVMap` · Y-symmetric (**worst mismatch 0.0 uu, 0 misses**, so
  the export mirror is a proven geometric no-op) · **Nanite OFF** · team area fraction **0.0283**
  (cap 0.4), selector = deck-top border ring + the deck's 40 uu outer rim.
- ⛔ **Fences honoured:** no `BP_Building_WatchTower`, no C++, no compile, no Git, no card art
  (TASK-740's), no `MI_WatchTower_PBR` edit, no `L_Arena`/`.umap` touched or saved, no editor bounce,
  no `Content/RawAssets/CardArt/` or `/Game/UI/CardArt/` write, no Fab browsing.
