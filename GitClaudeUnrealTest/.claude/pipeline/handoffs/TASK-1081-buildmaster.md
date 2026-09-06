# TASK-1081 — [FIELD-BASELINE] — build-master handoff

**Marker:** `TASK-1081-FIELD-BASELINE` · **Date:** 2026-09-06 · **Status:** done
**THIS TASK CHANGED NOTHING.** No `.uasset` write, no `.ini`, no texture downsize, no pool change, no commit, no push.

Instruments: Unreal MCP (editor PID 17008, `L_Arena` loaded) · engine runtime log · `nvidia-smi` (external, independent of UE) · `git` · `sha256sum`.

---

## 0. State reconciliation — TWO relayed facts were WRONG

| Relayed to me | Measured | Verdict |
|---|---|---|
| `HEAD` = `05c3fc0`, **ahead 30, NOT pushed** | `HEAD` = **`09b89cd`** *"fog works!"*, **`main` 0/0 vs `origin/main`** | ⚠️ **Jonathan self-committed AND PUSHED.** `05c3fc0` is an ancestor. The 30-commit backlog is discharged. (Known pattern: *jonathan-self-commits-milestones*.) |
| `Trees` layer is `Tree_Pack_1`; `Megaplant_Library` appears ZERO times | **Both confirmed** | ✅ The manager was right and **my own earlier folder-name relay was wrong.** Corrected below. |

`git status -- Content/` = **exactly one line**, `WBP_CardHand.uasset` (TASK-1079's, host = TASK-1080). Not staged, not touched.

---

## 1. THE LIVE SCATTER ROSTER — `/Game/Data/DA_BattlefieldScatter`

Read from the engine, then **independently corroborated** by the runtime log
(`GenerateScatter … layers=7`, per-layer `meshVariants=`) and by a component census of the live actor
(**59 HISMs** = 12 trees + 12 collision proxies + 2 + 3 + 5 + 10 + 10 + 5 — exact match).

🚨 **SEVEN layers, not five.** The spec anticipated `Trees · Rocks · Hills · Grass · Plants`.
**`Boulders` and `Slabs` are additional**, and the hill layer is named **`Hill`** (singular).

### Layer 0 — `Trees` (the subject of ask 1)
`InstanceCount 340` · `ScaleRange 0.8–1.2` · `MinSpacing 300` · `ZOffset 0` · `FootprintRadius 150`
`bBlocking true` · `RegionBias EdgeBias` · `bAllowOnHills true` · `MaxPlacementSlopeDeg 35`
`CullStart 24000` / `CullEnd 32000` · `bCastShadows true` · `OverrideMaterial None`
`CollisionProxyMesh /Engine/BasicShapes/Cylinder` · `ProxyScale (1.4, 1.4, 17)` · `ProxyZOffset 850`

| idx | asset path |
|----|---|
| 0 | `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_1` |
| 1 | `…/SM-Mobile_Tree_2` |
| 2 | `…/SM-Mobile_Tree_3` |
| 3 | `…/SM-Mobile_Tree_4` |
| 4 | `…/SM-Mobile_Tree_5` |
| 5 | `…/SM-Mobile_Tree_6` |
| 6 | `…/SM-Mobile_Tree_7` |
| 7 | `…/SM-Mobile_Tree_8` |
| 8 | `…/SM-Mobile_Tree_9` |
| 9 | `…/SM-Mobile_Tree_10` |
| 10 | `…/SM-Mobile_Tree_11` |
| 11 | `…/SM-Mobile_Tree_12` |

🚨🚨 **CENSUS CORRECTION THAT CHANGES `TASK-1082`.** The board says the pack was *"measured: `SM_Highpoly_Tree_1..12`"*.
**`Tree_Pack_1` contains TWO complete 12-tree sets, and the rostered one is NOT the one named:**

- `Meches/Highpoly_Tree_1/**SM_Highpoly_Tree_1..12**` — underscore, **1 LOD**, materials `M_Pack1_Leaf` + `M_Pack1_Trunk` — **rostered NOWHERE**
- `Meches/Mobile_Tree_1/**SM-Mobile_Tree_1..12**` — **HYPHEN**, **5 LODs**, materials `M_Pack1_Leaf_Mobile` + `M_Pack1_Trunk_Mobile` — **this is what is live**

Bounds are near-identical between the two sets (h 1381–2094 uu, w 871–1684 uu), i.e. **the same 12 trees in two builds.**
⛔ `TASK-1082` surveying `SM_Highpoly_*` would survey a set the game does not use; the hyphen also breaks naive `SM_` globs.

### Remaining six layers

| # | Layer | n | meshes (count · source) | Scale | Spacing | Bias | Blocking | Hills | Cull S/E | Shadows | OverrideMat |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | `Rocks` | 300 | 10 · `Realistic_Rocks` `SM_Rock_1,5,4,7,8,2,6,10,9,19` | 0.5–1.5 | 350 | Edge | yes | yes | 14000/20000 | yes | None |
| 2 | `Boulders` | 30 | 2 · `SM_Rock_11,20` | 0.8–1.3 | 900 | Edge | yes | no | 0/0 (never) | yes | None |
| 3 | `Hill` | 40 | 3 · `/Game/Meshes/SM_Hill_01,02,03` | 0.4–2.5 | 2000 | Whole | yes | no | 0/0 (never) | yes | **`M_HillGrass`** |
| 4 | `Slabs` | 40 | 5 · `SM_Rock_31..35` | 3.0–5.0 | 1500 | Edge | yes | no | 0/0 (never) | yes | None |
| 5 | `Grass` | 12250 | 10 · `Realistic_Grass_and_plant` `SM_Grass_1,7,3,5,10,14,27,23,17,9` | 0.7–1.5 | 50 | Whole | **no** | yes | 6000/9000 | **no** | None |
| 6 | `Plants` | 2000 | 5 · `SM_Plant_6,3,8,12,15` | 0.7–1.5 | 200 | Whole | **no** | yes | 8000/12000 | **no** | None |

All layers: `bRandomYaw true`, `FootprintRadius 0` (auto) except Trees, `CollisionProxyMesh None` except Trees.
Globals: `ArenaHalfExtent (26000, 12000)` · `SymmetryMode Rotational180` · `CastleKeepClear 4500` · `CorridorHalfWidth 1000` · `PlayerStartKeepClear 800`.

✅ **`Megaplant_Library` = ZERO occurrences** (confirmed). ✅ **`Realistic_Grass_and_plant` already wired TWICE** (`Grass` + `Plants`) — ask 2 is **not an add**.

---

## 2. 🚨 WHERE THE LIGHT-YELLOW BACKGROUND TREES COME FROM

### ✅ ANSWER: candidate **(c) — HAND-PLACED ACTORS in `L_Arena`.** NOT a scatter layer.

**15 `StaticMeshActor`s** carry `SM-Mobile_Tree_*` meshes. Uniform scale **7.54–10.94×** (vs the scatter's 0.8–1.2×), `overrideMaterials` **empty** on all 15, Z=0.

| actor | mesh | X | Y | scale |
|---|---|---|---|---|
| SMA_77 | Tree_12 | 37346 | 5915 | 9.97 |
| SMA_78 | Tree_5 | 35673 | 18176 | 9.78 |
| SMA_79 | Tree_1 | 19439 | 19439 | 9.88 |
| SMA_81 | Tree_12 | 3473 | 21930 | 10.72 |
| SMA_82 | Tree_8 | −3970 | 25068 | 10.44 |
| SMA_83 | Tree_4 | −11321 | 22218 | 8.17 |
| SMA_85 | Tree_11 | −38063 | 19394 | 10.15 |
| SMA_86 | Tree_4 | −34958 | 5537 | 7.97 |
| SMA_87 | Tree_8 | −38697 | −6129 | 9.10 |
| SMA_89 | Tree_12 | −21529 | −21529 | 9.39 |
| SMA_90 | Tree_4 | −9361 | −18373 | 7.68 |
| SMA_91 | Tree_11 | −3397 | −21445 | 10.10 |
| SMA_93 | Tree_8 | 12844 | −25207 | 10.94 |
| SMA_94 | Tree_7 | 21458 | −21458 | 7.54 |
| SMA_95 | Tree_8 | 38041 | −19383 | 9.55 |

**THE DISCRIMINATOR — all 15 are OUTSIDE the playfield box `|X| ≤ 26000 AND |Y| ≤ 12000`, with no exceptions.**
Every row fails on |X| or |Y|. They form a decorative ring alongside 50 `SM_Vista_01..04` backdrop actors
(|X| to 47752, |Y| to 32594) and `SM_SkySphere`. There is **no second tree scatter layer** and **no tree in any outliner folder**.

Pixel corroboration (`TASK-1081-yellow-trees-beyond-wall.png`): the bright-yellow canopies sit **beyond the arena wall**;
the green field in front carries rocks, grass and gold nodes.

### ⇒ 🚨 STOP-AND-REPORT — **THE CONDITION FIRES, ON *SAME MESH*, NOT ON *SAME LAYER***

The dispatch condition was later **broadened by the orchestrator** from *"same layer"* to *"same layer **OR the same mesh**"*.
Measured against the broadened rule, **it fires.** Reported, not worked around:

| test | result |
|---|---|
| Same scatter **layer**? | ❌ **NO** — different systems (scatter HISM vs 15 hand-placed `StaticMeshActor`s) |
| Same **mesh assets**? | 🚨 **YES** — the background ring uses `SM-Mobile_Tree_1,4,5,7,8,11,12`, **7 of the 12** in `Layers[0].Meshes` |
| Would emptying `Layers[0].Meshes` destroy the praised trees? | ❌ **No** — mechanically it cannot reach them |

⛔ **So the array edit is SAFE but POINTLESS**, and that is the finding: the two populations are not different trees,
so *"swap the battlefield ones for lighter ones"* has **nothing to swap to** — they already **are** the light ones.

### ✅ THIS ROW RESOLVES THE MANAGER'S APPARENT CONTRADICTION — **possibility (2), measured**
`TASK-1082` measured `T_Leaf_Pack1` albedo = **RGB(199,176,97), tone 46.1 — light gold**, shared by all 24 meshes, 9% spread.
This row measured, three independent ways (DataAsset · runtime log `layers=7` · 59-HISM census), that the battlefield trees
**are** those meshes — and that the background ring is **the same meshes again**, with **`overrideMaterials` empty on all 15**.

⇒ **Both of the manager's statements are TRUE.** The false premise was the unstated one: *that the dark trees and the light
trees are different assets.* **They are the same asset.** ⛔ Not possibility (1) — the roster read was right. ⛔ Not (3) —
there is no third source: the level contains **no** tree beyond those 15, in no outliner folder, under no other actor.

**Single-frame proof:** `TASK-1081-lane-vantage.png` shows **dark olive canopies and gold canopies in ONE capture**,
all of them the same 7 hand-placed meshes, differing only by **bearing to the sun and distance**.

### ⇒ ⛔ **ASK 1 IS NOT A ROSTER PROBLEM.** It is a lighting/atmosphere problem.
🧑 **His ruling is required.** *"Change the ones on the battlefield, keep the light yellow background ones"* cannot be
executed as worded, because there is only **one** set of trees. The levers that actually exist are **lighting** (his
screenshot is at dusk; the directional light, `SkyAtmosphere`, `SkyLight` and `ExponentialHeightFog` are what make the near
trees read near-black while the far ones read gold), **the missing normal maps on the Mobile materials** (§4), and
**`ScaleRange`/`InstanceCount`** — **not** `Layers[0].Meshes`.

### ⛔ BUT — THE REAL HAZARD, AND IT IS A DIFFERENT ONE
**The two populations share the SAME 7 mesh ASSETS** (`SM-Mobile_Tree_1,4,5,7,8,11,12`).
⇒ **Editing the roster is SAFE. Editing, deleting, re-importing, re-materialising or re-scaling any `SM-Mobile_Tree_*` asset
is NOT — it would change the trees he explicitly praised.** The read-only vendor fence now has a concrete reason behind it.

### ⚠️ FLAGGED FOR HIS RULING — the tone difference may not be an ASSET difference
Same meshes, same materials, no overrides, both populations. The only differences are **scale (≈9×)**, **distance
(20k–47k uu)** and therefore **aerial perspective / height fog**, plus cull (background actors have none).
⇒ **The same asset reads dark near and light-yellow far.** In `TASK-1081-lane-vantage.png` both readings appear in ONE frame.
⛔ This undercuts `TASK-1082` §2's premise that a per-asset "mean canopy luminance" ranking predicts what he sees:
it would rank an asset that is *already* his "light yellow" one as "dark". **`TASK-1082` must measure tone at a
battlefield-representative distance and light, or its ranking will not transfer.**

---

## 3. 🚨 THE VRAM BASELINE

### The instrument, and why `VIS-§8`'s verdict cannot transfer
The banner **`Video memory has been exhausted (N MB over budget). Expect extremely poor performance.`** is the **D3D12
adapter** message — DXGI `QueryVideoMemoryInfo` **whole-GPU local budget, across every process on the machine**.
It is **NOT** the texture-streaming pool. ⇒ reading a value from it as a *pool-size signature* is a category error.

**Machine:** `NVIDIA GeForce RTX 5070 Laptop GPU` — **8151 MiB total**; UE reports `Adapter has 7891MB of dedicated video memory`.
**Configured pool:** `r.Streaming.PoolSize = 800` MB · `PoolSizeForMeshes = -1` (mesh+texture **share** it) · `VRAMPercentageClamp 1024`.

### Measured, converged (`SC-§88` — 10 samples at 2 s, all identical, not one frame)

| state | used | free | note |
|---|---|---|---|
| editor idle, PIE stopped (start) | 6455 MiB | 1437 MiB | Fab `EpicWebHelper` **already on the GPU** |
| **PIE on `L_Arena`, converged** | **7399 MiB** | **493 MiB** | flat across 20 s |
| PIE + viewport captures (peak) | **7498 MiB** | **394 MiB** | **91.9 % of the card** |
| PIE stopped, settled | 6065 MiB | 1827 MiB | |
| session end | 5827 MiB | 2065 MiB | |

**PIE delta = 944 MiB** (vs the 6455 baseline) / **1334 MiB** (vs the 6065 settled floor).

### ✅ THE BANNER REPRODUCES — at a different magnitude
Captured verbatim on screen (`TASK-1081-vram-banner-23MB.png`):
> `Video memory has been exhausted (23.391 MB over budget). Expect extremely poor performance.`

Three readings of **the same instrument**, on **substantially the same content**:

| source | over budget | ratio |
|---|---|---|
| `VIS-§8` | 0.922 MB | 1× |
| **this session** | **23.391 MB** | 25× |
| 🧑 his screenshot | **828.144 MB** | **898×** |

⭐ **THE CONCLUSION.** The overage spans ~900× across sessions whose *scatter content is the same*. It therefore measures
**whole-machine GPU contention at that moment** (browser/Fab, other apps, editor uptime, viewport size), **not the cost of the
battlefield foliage**. ⛔ Do **not** attribute his `828.144 MB` to the trees, and do not treat `TASK-1085`/`TASK-1094`'s
after-reading as a scatter measurement unless the machine state is held constant. **The delta to trust is the PIE delta
(944–1334 MiB), taken back-to-back in one session.**

### ④ THE BUDGET DOWNSTREAM ROWS MAY SPEND — and the honest sentence
**There is very little headroom: 394–493 MiB free at the gameplay vantage on an 8 GB card, ~92 % occupied, and the
over-budget banner already fires today at baseline with no change made.**

But the cost of ask 1 is small **if it stays inside `Tree_Pack_1`**:

| textures (the whole pack — only FOUR, shared by all 24 tree meshes) | dims | format | GPU w/ mips |
|---|---|---|---|
| `T_Leaf_Pack1` | 2048² | DXT5 | 5.33 MiB |
| `T_Trunk_Pack1` | **1414²** | **B8G8R8A8 — UNCOMPRESSED** | **10.17 MiB** |
| `T_Leaf_Pack1_normal` | 2048² | BC5 | 5.33 MiB |
| `T_Trunk_Pack1_normal` | **1414²** | **B8G8R8A8 — UNCOMPRESSED** | **10.17 MiB** |

- **Resident today** (Mobile path uses only the two base maps): **≈ 15.5 MiB.**
- **A Mobile → Highpoly roster swap costs ≈ +15.5 MiB texture (the two normals) + ≈ 5.6 MB mesh ≈ +21 MB** — affordable.
- ⚠️ **`T_Trunk_Pack1` and its normal are UNCOMPRESSED because 1414 is not a multiple of 4** (block compression needs it).
  They cost **~20.3 MiB for what DXT1/BC5 at 2048² would cost ~8 MiB** — a free ~12 MiB if a later row resizes them.
  ⛔ Vendor-pack fence: not mine to change; recorded for `TASK-1084`/`TASK-1085` and his ruling.
- **Cost of one additional *unique* tree species from `Megaplant_Library`: hundreds of MB.**
  Pack sizes on disk: `Tree_Pack_1` **46 MB** (both sets, all 4 textures) vs `Megaplant_Library` **1.2 GB** for **two**
  conifer species (`Tree_Norway_Spruce`, `Tree_Japanese_Cypress`) · `Realistic_Grass_and_plant` 331 MB · `Realistic_Rocks` 2.8 GB.
  ⇒ **variety sourced inside `Tree_Pack_1` is ~free; variety sourced from `Megaplant_Library` is the expensive option
  on a card with 394 MiB free** — and `TASK-1082` §3 already suspects Megaplant cannot deliver "light" anyway.

---

## 4. ⭐ UNPLANNED FINDING — A MEASURED, CHEAP CANDIDATE FOR *"WAY TOO DARK"*

**The two materials on every live battlefield tree carry BROKEN texture references.**

```
M_Pack1_Leaf_Mobile   -> /Game/Tree_Pack_1/Textures/T_Leaf_Pack1
                         /Game/Tree_Pack_16/Textures/dgd     <-- DOES NOT EXIST
M_Pack1_Trunk_Mobile  -> /Game/Tree_Pack_1/Textures/T_Trunk_Pack1
                         /Game/Tree_Pack_16/Textures/fg      <-- DOES NOT EXIST
M_Pack1_Leaf          -> T_Leaf_Pack1  + T_Leaf_Pack1_normal    (both exist)
M_Pack1_Trunk         -> T_Trunk_Pack1 + T_Trunk_Pack1_normal   (both exist)
```

`/Game/Tree_Pack_16/` is absent from disk, from the asset registry, and from the `/Game` root folder listing.
**The engine says so itself**, unprompted, in this session's log:
```
LogScript: Warning: Asset does not exist: /Game/Tree_Pack_16/Textures/dgd
LogScript: Warning: Asset does not exist: /Game/Tree_Pack_16/Textures/fg
```

⚠️ **STATED AS A FACT, NOT A VERDICT (`SC-§94`).** I measured *that the references are broken* and *that the Mobile set also
has no normal maps while the Highpoly set does*. I did **not** prove this causes the darkness — the sampler may be
disconnected vendor litter. **It is cheap for `TASK-1082` to test** and, if it holds, the fix for ask 1 is a
**roster swap to the Highpoly set already sitting in the same pack** — same 12 silhouettes, real normal maps, no broken
refs, ~+21 MB, and **zero new art**. That would satisfy *"too dark"* without spending the variety budget at all.
⛔ Trade-off to put to him: Highpoly is **1 LOD vs Mobile's 5**, across **340 instances** — a real render cost, not free.

---

## 5. ⚠️ OPEN ANOMALY FOR THE MANAGER — declared, not solved

**338 tree instances are confirmed present** (per-HISM: 18/36/36/22/42/22/32/22/30/22/28/28), at correct scale
(sampled transforms ≈ 0.98–1.10), correct meshes, `cullStart 24000 / cullEnd 32000`, shadows on.
**Yet across FOUR captures at four vantages (lane, mid-field side, edge band at |Y| 9500, aerial) I did not positively
image a single battlefield tree**, while background trees appear in three of the four.

I am **not** asserting they fail to render — the instance data proves they exist, and 🧑 he plainly sees them
("all the trees on the battlefield are way too dark"). But 340 instances over a 52 000 × 24 000 field is a mean spacing of
**~1900 uu**, and `EdgeBias` + the 24 000 cull may leave large stretches bare. **Whether the field reads as "wooded" at all
is worth one row**, because if the answer is "sparse", then *adding variety* and *fixing tone* are different fixes and he
may want density (`InstanceCount`) changed too. Evidence: the four PNGs below.

---

## 6. FENCES — ALL HELD

- ⛔ **`save_assets([])` NEVER CALLED.** No save of any kind. No save prompt encountered.
- ✅ **`L_Arena` hash `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` — BEFORE **and** AFTER, byte-identical.**
- ⚠️ `is_dirty` re-measured at session end: **`L_Arena` = true** (dirty in memory — the hazard is live and real),
  `DA_BattlefieldScatter` = **false**, `WBP_CardHand` = **false**.
  ⭐ Refines TASK-1079's finding: my many read-only property reads did **not** dirty `DA_BattlefieldScatter`.
  The read-dirties-asset effect is **not universal** — it was specific to a Blueprint graph read, not to property reads.
- ✅ `git status -- Content/` = **exactly one line**, `WBP_CardHand.uasset`. Nothing beyond the expected. Nothing staged.
- ⛔ No `checkout --` / `restore` / `stash` / `reset` / `clean`. No commit. No push. No `.ini`. No DataAsset write.
- ✅ Vendor packs read-only — rendered/queried only.
- ✅ Fab "My Library" window left **minimised**, untouched.
- ✅ **Editor LEFT OPEN**, PID 17008, PIE **stopped**, `L_Arena` loaded, MCP live.
- Written by this task: this handoff + 4 evidence PNGs under `.claude/pipeline/playtest-evidence/2026-09-06/`
  (pipeline dir, not `Content/`, uncommitted).

**Evidence:**
- `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1081-lane-vantage.png` — dark **and** yellow canopies in one frame
- `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1081-yellow-trees-beyond-wall.png` — the praised trees, beyond the wall
- `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1081-edge-band.png` — edge band at |Y| 9500
- `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1081-vram-banner-23MB.png` — the banner, verbatim

---

## 6b. CROSS-VALIDATION WITH `TASK-1082` — two instruments, agreeing

`TASK-1082` surveyed the packs; this row queried the live engine. They were run independently and **agree on every
overlapping fact**, which is worth recording because agreement across two instruments is the strongest evidence this
lane has produced:

| fact | `TASK-1082` (renderer + controls) | `TASK-1081` (engine/registry/log) | |
|---|---|---|---|
| Dangling refs `Tree_Pack_16/{dgd,fg}` | found in Mobile materials | `exists()` = false ×3 · engine's own `LogScript` warning | ✅ **independently confirmed** |
| Mobile vs Highpoly are the same designs | silhouette IoU | bounds near-identical (h 1381–2094 both sets) | ✅ agree |
| `Tree_Pack_1` is cheap | +2.31 MB meshes, zero extra textures | 46 MB whole pack; only **4** textures for all 24 meshes | ✅ agree |
| `Megaplant_Library` is ruinous | +41.33 MB / spruce part, +96.00 MB / cypress part | **1.2 GB** pack for **two** species | ✅ agree |
| `Megaplant` rostered? | — | **zero occurrences** | ✅ |

⭐ **One consequence of their IoU result that changes §4's recommendation:** since `SM-Mobile_Tree_N` and
`SM_Highpoly_Tree_N` are the **same design**, a Highpoly swap buys **no variety** — so it is **not** a fix for
*"add a large variety"*. It remains worth testing **only** as a *tone* lever, because the Highpoly materials carry the
**two real normal maps** the Mobile ones lack (and no dangling refs), and normal maps change how a dusk sun reads on a
canopy. ⚠️ **Hypothesis, not verdict** — and it costs 1 LOD vs 5 across 340 instances.

⛔ **Neither row can deliver *"a large variety of light trees from those two folders"*:** `Tree_Pack_1` has **12 designs**
(24 assets, one shared gold albedo) and `Megaplant_Library` has **2 dark species at 1.2 GB**. That is the honest answer to
ask 1's second half, and it is 🧑 his call what to do about it.

## 7. FEEDS

- **`TASK-1082`** — ⛔ survey `SM-Mobile_Tree_*` **and** `SM_Highpoly_Tree_*` (24, not 12; note the hyphen).
  ⛔ Tone must be measured at battlefield distance/light, not per-asset in isolation (§2). ⛔ Megaplant = 1.2 GB for 2 dark species.
- **`TASK-1083`** — roster edit is **safe**; asset edit is **not** (§2). Budget: ~+21 MB inside `Tree_Pack_1`; ~394 MiB free total.
  The Highpoly swap is the cheapest candidate on the table (§4), with a 1-LOD-vs-5 cost to declare.
- **`TASK-1084` / `TASK-1085`** — the after-reading must hold machine state constant or it measures the browser, not the trees (§3).
  Free ~12 MiB by fixing the 1414² uncompressed pair, if he licenses a vendor-texture change.
- **manager** — three items need boarding: the `Highpoly`-vs-`Mobile` census correction, the broken `Tree_Pack_16` refs,
  and the sparse-field anomaly (§5). Plus 🧑 the flagged question in §2.
