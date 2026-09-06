# TASK-1087 — [CHAR-TILES] handoff (art-director, 2026-09-06)

Marker `TASK-1087-CHAR-TILES`. Law: `CHAR-§1` · `CHAR-§2`.
No editor, no MCP, no network, no credits spent. Nothing staged, nothing committed.

## 0. Source, and what it actually is

| | |
|---|---|
| path | `Tools/ArtPipeline/Inbox/MainCharacter.png` |
| `sha256` BEFORE | `77f845dbd1e043b137f98d13e26ab2338cc65b435b843ce7c25ba9ebcc324402` |
| `sha256` AFTER | `77f845dbd1e043b137f98d13e26ab2338cc65b435b843ce7c25ba9ebcc324402` — **IDENTICAL. The original was never opened for write** (`CHAR-§2`, spec cl. 5). |
| canvas | **1448 × 1086 px**, RGB, no alpha |
| layout | 3 horizontal bands; **16 captioned cells**; the HELMET cell stacks **4** sub-images ⇒ **19 distinct sub-images** |

**Grid was MEASURED, not eyeballed** — divider lines found by column/row profiling
(dividers are a light grey 146–164 against a 122–134 ground), then each cell's identity
confirmed by checking its **caption's pixel centre against the cell's geometric centre**.
All 16 agreed to within 27 px (most within 4 px). Divider positions:

- band-1 art rows `0–606`, captions `614–626`; column dividers at `616–618`, `797–798`, `1315–1317`
- band-2 art rows `640–838`, captions `853–862`; dividers at `189–190`, `389–390`, `580–581`, `824–826`, `1004–1005`, `1224–1226`
- band-3 art rows `881–1044`, captions `1058–1068`; dividers at `612–616`, `796–800`, `989–992`, `1172–1176`
- HELMET column `x 1318–1447`, internal row dividers at `166–169`, `314–316`, `455–456`, `603–606`

## 1. Tile-by-tile manifest — all 16 cells / 19 sub-images

`INPUT` = fed to the generator · `CHECKLIST` = acceptance reference only, never geometry input · `EXCLUDED` = must not enter the pipeline at all.

| # | caption | what it shows | crop (x0,y0,x1,y1) | role | why |
|---|---|---|---|---|---|
| 1 | `FRONT` | full body, T-pose, front. Great helm, mail, white tattered surcoat + cloak, red cross on chest, **scabbarded sword at left hip**, hands empty | 0,0,616,607 | ✅ **INPUT** | orthographic body view |
| 2 | `SIDE` | full body, profile. Cloak sweep, **scabbarded sword down the hip/leg**, arm extended to camera (T-pose foreshortened), hand empty | 621,0,796,607 | ✅ **INPUT** | orthographic body view |
| 3 | `BACK` | full body, T-pose, back. **Large red cross on the tattered cloak**, hood down, both hands empty, sword NOT visible (cloak occludes) | 801,0,1315,607 | ✅ **INPUT** | orthographic body view |
| 4a | `HELMET` (1/4) | great helm front — gold cruciform face reinforcement, twin sight slits, grid of breath holes, mail aventail | 1320,2,1446,164 | 🔶 CHECKLIST | head close-up: wrong scale for a body slot |
| 4b | `HELMET` (2/4) | great helm ¾ — gold cross on side panel, single slit, breath holes | 1320,172,1446,312 | 🔶 CHECKLIST | as above |
| 4c | `HELMET` (3/4) | great helm profile — gold cross, slit, breath-hole cluster | 1320,319,1446,453 | 🔶 CHECKLIST | as above |
| 4d | `HELMET` (4/4) | great helm rear — **plain, no cross**; vertical gold seam strip, gold base band, mail below | 1320,459,1446,601 | 🔶 CHECKLIST | as above |
| 5 | `GAUNTLET` | articulated steel gauntlet, lamed fingers, gold-edged cuff | 7,642,187,837 | 🔶 CHECKLIST | **a hand close-up in a body slot tells the solver the character IS a hand** (`CHAR-§2`) |
| 6 | `SHOULDER` | pauldron + surcoat shoulder: **red cross on white**, gold roundel clasp with cross, mail beneath | 193,642,387,837 | 🔶 CHECKLIST | part, not whole |
| 7 | `CHEST` | torso front: bold **red cross**, two gold cross-roundel clasps, brown belt w/ gold buckle | 393,642,578,837 | 🔶 CHECKLIST | part, not whole |
| 8 | `HIP / TASSETS` | double belt rig, gold cross strap-ends, mail skirt, gold-trimmed surcoat split | 584,642,822,837 | 🔶 CHECKLIST | part, not whole |
| 9 | `LEG` | cuisse + poleyn, gold-edged plate over mail | 829,642,1002,837 | 🔶 CHECKLIST | part, not whole |
| 10 | `SABATON` | pointed lamed sabaton + greave, gold trim, rivets | 1008,642,1222,837 | 🔶 CHECKLIST | part, not whole |
| 11 | `REAR DETAIL` | upper back: white hood thrown back, **mail coif/aventail**, **2 gold cross roundels**, **X of riveted leather straps** | 1229,642,1442,837 | 🔶 CHECKLIST | part, not whole |
| 12 | `SWORD (SIDE)` | full arming sword: straight double-edged blade, gold cruciform guard, leather grip, cross pommel | 8,883,610,1043 | 🔶 CHECKLIST ⚔️ | **a prop, not the character** — `J-C1` |
| 13 | `SWORD DETAIL` | hilt close-up: cross-embossed round pommel, wrapped grip, gold quillons | 619,883,794,1043 | 🔶 CHECKLIST ⚔️ | prop — `J-C1` |
| 14 | `SHIELD (FRONT)` | heater shield, weathered white face, **red cross**, studded rim | 803,883,987,1043 | 🔶 CHECKLIST ⚔️ | prop — `J-C1` |
| 15 | `SHIELD (BACK)` | shield reverse: vertical planks, leather straps/enarmes, central grip | 995,883,1170,1043 | 🔶 CHECKLIST ⚔️ | prop — `J-C1` |
| 16 | `CLOAK DETAIL` | tattered hem close-up: fraying, torn strips, mud staining, weave | 1179,883,1441,1043 | 🔶 CHECKLIST | material/texture reference |
| — | the 16 caption strings | `FRONT`…`CLOAK DETAIL`, dark serif text on light grey | rows 614–626 / 853–862 / 1058–1068 | ⛔ **EXCLUDED** | README: *"no … text"*. Every crop above is bounded to art rows, so no caption can leak. |

**`CHAR-§1` inventory reconciliation — EXACT MATCH, 16/16, nothing extra, nothing missing.**
One layout nuance worth recording, **not** a discrepancy: `CHAR-§1` groups `CLOAK DETAIL`
with the DETAIL band, and it is a detail tile — but it physically sits in the **bottom
(props) band**, right of `SHIELD (BACK)`. Classification correct, position differs.
**No tile was found that `CHAR-§1` did not list.**

## 2. Generator inputs — the three body views

| file | output | source window | figure bbox (source) | figure h | fills |
|---|---|---|---|---|---|
| `MainCharacter_Front.png` | 1024×1024 | 640×640 @ (−17,−8) | x 11–595, y 17–606 | 590 px | 92.2 % of frame height |
| `MainCharacter_Side.png` | 1024×1024 | 640×640 @ (394,−11) | x 637–791, y 16–602 | 587 px | 91.7 % |
| `MainCharacter_Back.png` | 1024×1024 | 640×640 @ (740,−10) | x 812–1307, y 18–603 | 586 px | 91.6 % |

`sha256`
- Front `e675cabcca7bb738e5be6b2f92e5871471554c5fd6dc8480248c16264a3147f5`
- Side  `4eeaf6848300ff2da9ee05d586712993334a67ef75397e992f493f59a3d2de39`
- Back  `fc799a14ade257f472ba7a9dafcde9108bf8f080c337fc0c60c5bc9b3e138483`

**A uniform 640 px source window was used for all three** so the three views share one
scale and one vertical registration — the three figures are within **4 px** of the same
head-to-toe height in the source (590/587/586), and each is centred on its own bbox, so
helmet crown and sabaton sole land on the same output rows in all three. *A multi-view
solver assumes every input frames the same subject at the same scale* (`CHAR-§2`); this
makes that true by construction rather than by luck. Verified on a triptych with rulers.

**Two defects were found and fixed during verification — both would have shipped silently:**

1. **Divider halo.** The bright divider lines carry a 1-px antialiasing skirt into the
   neighbouring cell (`x=799` reads 137.5 against a 123 ground; `x=796` reads 135.9).
   A naive cell-boundary crop put a **bright 1-px line up the full height** of the BACK
   and SIDE views — exactly the kind of hard edge that background removal latches onto.
   It also faked the BACK figure's bbox 13 px wide, mis-centring the crop. Clean interiors
   are therefore `Front 0–615`, `Side 621–795`, `Back 801–1314`, measured per edge.
2. **The background is not flat — it has a vertical gradient**, ~`123` at the top to
   ~`134` at the bottom of each band. Padding to square with one flat colour left an
   **11-level horizontal seam** across the lower frame. Padding is now built **per row
   from that row's own measured background**, so the gradient continues through the pad.
   Verified: output rows 8 and 1015 are perfectly uniform (max adjacent step **0.0**),
   corners TL=TR and BL=BR exactly, and the only large steps in the frame are the
   figure's own silhouette.

Also verified: **no contact shadow** (background returns to ground level immediately below
the sabatons — min 122–126, no dark ellipse), so nothing gets baked into geometry.

⚠️ **Honest resolution note — do not read "1024" as 1024 of real detail.** The sheet gives
each figure only ~**590 px** of head-to-toe pixels. The 1024² outputs are a **1.6× Lanczos
upscale** of a 640 px window, chosen to match the README's recommended ~1024² square. True
optical resolution is ~640 px. Native 640² would also clear the ≥512 floor if a later row
prefers no resampling; the crop rectangles above regenerate either.

## 3. Detail checklist for the `TASK-1092` approval gate

16 reference crops in `Tools/ArtPipeline/Inbox/MainCharacter_Detail/`, native resolution,
**no resampling** (they are evidence, not input), each inset 2 px to guarantee no divider
or caption bleed. Filename prefix = manifest index above, so a checklist row maps to a file.

`CHAR-§5` wants this **ticked or declared-missing item by item** against the FRONT/SIDE/BACK
previews. The concrete feature to look for is named per row, so the gate is answerable:

| # | file | feature the mesh must show | where to look |
|---|---|---|---|
| 4a | `04a_Helmet_Front.png` | gold **cruciform** face reinforcement; twin sight slits; breath-hole grid | front preview, head |
| 4b | `04b_Helmet_ThreeQuarter.png` | gold cross on the **side** panel | ¾ / side |
| 4c | `04c_Helmet_Side.png` | flat-topped great-helm profile, not a rounded bascinet | side |
| 4d | `04d_Helmet_Back.png` | rear of helm is **plain — no cross**; vertical gold seam only | back |
| 5 | `05_Gauntlet.png` | articulated **lamed fingers**, not mitten blobs | front, hand ends |
| 6 | `06_Shoulder.png` | **red cross on each shoulder** + gold roundel clasp | front + back, shoulders |
| 7 | `07_Chest.png` | **bold red cross on the chest**; belt with gold buckle | front, torso |
| 8 | `08_Hip_Tassets.png` | **double** belt rig; gold cross strap-ends; mail skirt under the split surcoat | front, waist |
| 9 | `09_Leg.png` | gold-edged cuisse + **poleyn** over mail | front/side, thigh–knee |
| 10 | `10_Sabaton.png` | **pointed lamed** sabaton, not a boot | all, feet |
| 11 | `11_Rear_Detail.png` | hood down + mail aventail + **2 gold cross roundels** + **X of leather straps** | back, upper |
| 16 | `16_Cloak_Detail.png` | **tattered/frayed hem with torn strips + mud staining** — not a clean hem | all, cloak edge |
| 12 | `12_Sword_Side.png` | ⚔️ prop — see `J-C1`; **not expected on the body mesh** | — |
| 13 | `13_Sword_Detail.png` | ⚔️ prop — cross pommel, gold quillons | — |
| 14 | `14_Shield_Front.png` | ⚔️ prop — red cross on white heater shield | — |
| 15 | `15_Shield_Back.png` | ⚔️ prop — planks, straps, enarmes | — |

The row `CHAR-§5` names as the point of the exercise — ***"the REAR DETAIL cross is
absent"*** — is **#11**, and it is the likeliest to fail: those roundels are ~12 px across
in the source BACK view. Judge it against `11_Rear_Detail.png`, not against the BACK tile.

**Highest-risk items** (small in the source, so most likely to be lost): #11 roundels,
#4a helmet cross, #8 gold cross strap-ends, #5 finger lames.

## 4. `J-C1` — the weaponry evidence, recorded not resolved

**Tiles showing weaponry: four dedicated prop tiles — #12 `SWORD (SIDE)`, #13 `SWORD
DETAIL`, #14 `SHIELD (FRONT)`, #15 `SHIELD (BACK)`.** Matches `CHAR-§4`'s count.

**A refinement `CHAR-§4` does not record, found by zooming the tiles — and it changes what
"unarmed" means here.** `CHAR-§4` says *"both hands are empty in FRONT and BACK"*. Confirmed:
hands are empty in all three body views (FRONT open gauntlets, SIDE a closed empty fist,
BACK open gauntlets). **But the body is not weapon-free:**

- **FRONT** — a **scabbarded sword hangs at the left hip**: gold pommel, gold crossguard, brown scabbard running past the knee, slung from the belt.
- **SIDE** — the **same scabbarded sword**, running diagonally down behind the leg.
- **BACK** — **no sword visible at all**; the tattered cloak occludes it completely.

Two consequences, both for 🧑 him and both left undecided here:

1. **The `CHAR-§4` default still holds and needs no change** — *held* weapons are absent, so
   the body ships matching the T-pose and `ABP_Unarmed`, and nothing is fused to a hand.
   **But a generated body will likely carry the hip scabbard as part of its silhouette**,
   because it is drawn on the figure in 2 of 3 input views. That is not the same as
   "unarmed" and should not surprise anyone at `TASK-1092`.
2. ⚠️ **Cross-view inconsistency, flagged for the solver:** the sword is present in FRONT
   and SIDE and absent in BACK. A multi-view reconstruction may resolve that as a partial,
   floating or smeared scabbard. **If the hip sword looks wrong on the preview, this is the
   cause** — an input inconsistency, not a model failure. Deliberately **not** painted out:
   editing his art is not my call, and the alternative (a sword-free body) is equally 🧑 his.

## 5. Files written

WRITES (all under `Tools/ArtPipeline/Inbox/`, **all gitignored** by `.gitignore:25`
`Tools/ArtPipeline/Inbox/*` — correct per spec cl. 7, **not force-added**):

```
MainCharacter_Front.png   1024x1024
MainCharacter_Side.png    1024x1024
MainCharacter_Back.png    1024x1024
MainCharacter_Detail/     16 PNGs, native res, listed below
```

| file | size | sha256 |
|---|---|---|
| `04a_Helmet_Front.png` | 126×162 | `53f3d3567ee210136ede83ae76a602131858fd6564e360cb7e0bcc39591eed7f` |
| `04b_Helmet_ThreeQuarter.png` | 126×140 | `b6650b1ea058863a677fcb7db2fa00cfeea7b13be20d857750f2713f8ba2b9e4` |
| `04c_Helmet_Side.png` | 126×134 | `5063b79759eaf9a9e1605789592a3850cb160daecbb309175ae16931b4fb7843` |
| `04d_Helmet_Back.png` | 126×142 | `decafbe37af771cd6faad9ffa4cbe4bf435731006d3fdec71522e4b5175933e8` |
| `05_Gauntlet.png` | 180×195 | `210cb14a24c992428f53d87c0b06c8f873dcfa989d40ee4fdb2a8e571c939fcf` |
| `06_Shoulder.png` | 194×195 | `dd5f25f34ea43cec8e90cfcdc87180c78d6dbfb0975e75ee90f6420ade58d061` |
| `07_Chest.png` | 185×195 | `3aa6bb5db5f46cc75503a0123199925edd5adb31d8fe81bf7da6379bdade7117` |
| `08_Hip_Tassets.png` | 238×195 | `f99f627c083cccb4cd9b4a5a048a4ba04e507f91c71a399d44be1bcda74ba338` |
| `09_Leg.png` | 173×195 | `47ecefa314feb2fd1e955bef40e28be938976511e0286b5c16db9cfc664f2614` |
| `10_Sabaton.png` | 214×195 | `e592ea60719d300c8b662ebe8aeb1a0c6a5821541f9ae272c793a614af302e93` |
| `11_Rear_Detail.png` | 213×195 | `0e3e7f76e8b4c71f7690acd3715835a1268b208df5f2cacebb8ce03cb700e8ed` |
| `12_Sword_Side.png` | 602×160 | `4ae70819da7c8e519b6408d2972e396d32a3a6b86d56013c3b6c7a834a07e0ab` |
| `13_Sword_Detail.png` | 175×160 | `4c797e11859566d182bcee0e58bc7d15e75b019994c00e112d0acf1612dbe0a6` |
| `14_Shield_Front.png` | 184×160 | `7367d46c112f885b8a929da62cba17fad44cf817e0cf8441eb5d95318667a2e1` |
| `15_Shield_Back.png` | 175×160 | `a5888b858a8ffcf2d172fcb9b331a8593cea4964b2c889865f4938e34811f406` |
| `16_Cloak_Detail.png` | 262×160 | `731b527cc140ce20b2a3d493ce96a12249906cb7c77cec986b4042a4b28e6dc8` |

**NEVER MODIFIED:** `Tools/ArtPipeline/Inbox/MainCharacter.png` — sha verified identical
before and after.

## 6. Notes for downstream

- **`TASK-1091` (generation).** Branch A (`--mode multiimage`) consumes exactly
  `MainCharacter_Front.png` / `_Side.png` / `_Back.png` — the pinned `CHAR-§2` names that
  `TASK-1088` cl. 3 reads. Branch B (single-image `image3d`) should use **`_Front.png`**;
  the `CHAR-§3` loss statement holds and is now measured: **the big red cross on the cloak
  exists only in the BACK view**, so Branch B loses the character's most distinctive
  surface. ⛔ **Never feed a `MainCharacter_Detail/` file as a reconstruction view.**
- The detail tiles remain available as a **retexture style reference** (`CHAR-§2`), which is
  a legal second use — they are excluded from *geometry* input, not from the pipeline.
- **Not mine, seen while verifying:** `Tools/ArtPipeline/meshy_generate.py` reads modified
  in `git status`. That is `TASK-1088`'s lane running in parallel; I did not touch it.
- Expected-dirty set per the batch preamble otherwise unchanged; nothing staged by me.
