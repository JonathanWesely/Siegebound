# TASK-106 Handoff — Set III card art: 6 illustrations + import (art-director)

**Status: COMPLETE — 6/6 rendered (Blender MCP, shared EEVEE studio rig), 6/6 imported to
`/Game/UI/CardArt/`, settings + size + casing readback-verified, saved, not-dirty.**
Editor state: `IsPIERunning` = false before any editor mutation; editor left UP. Editor mutations
kept to the import/verify steps only (no other Content/ touched). No Git run, no TASKBOARD edit.

## Pipeline

TASK-077 pipeline reused: subjects are custom stylized low-poly primitive compositions
("board-game token" family) built in the live Blender (bridge port 9876) and rendered through a
reconstruction of the shared M4 studio rig — backdrop+floor in the per-card key color, 65 mm
camera slight high angle track-to, warm key / cool fill / soft white corner-wash rim / backdrop
halo spot in lightened key color, EEVEE 48 samples, 512×512 PNG RGB 8-bit, view transform
Standard. Spell cards use the M4 iconographic-prop approach (donor-less concepts). Every render
was eyeballed at full size AND on a 150 px contact sheet; weak passes were reworked (below).

## Deliverables (all readback-verified post-save)

| CardID | PNG source (checked-in) | Texture asset | Size | LODGroup | sRGB | Compression | Dirty |
|---|---|---|---|---|---|---|---|
| Fireball | Content/RawAssets/CardArt/Fireball.png | /Game/UI/CardArt/T_CardArt_Fireball | 512×512 | TEXTUREGROUP_UI | true | TC_Default | no |
| FrostNova | Content/RawAssets/CardArt/FrostNova.png | /Game/UI/CardArt/T_CardArt_FrostNova | 512×512 | TEXTUREGROUP_UI | true | TC_Default | no |
| Lightning | Content/RawAssets/CardArt/Lightning.png | /Game/UI/CardArt/T_CardArt_Lightning | 512×512 | TEXTUREGROUP_UI | true | TC_Default | no |
| BattleCry | Content/RawAssets/CardArt/BattleCry.png | /Game/UI/CardArt/T_CardArt_BattleCry | 512×512 | TEXTUREGROUP_UI | true | TC_Default | no |
| Pickpocket | Content/RawAssets/CardArt/Pickpocket.png | /Game/UI/CardArt/T_CardArt_Pickpocket | 512×512 | TEXTUREGROUP_UI | true | TC_Default | no |
| CrystalTower | Content/RawAssets/CardArt/CrystalTower.png | /Game/UI/CardArt/T_CardArt_CrystalTower | 512×512 | TEXTUREGROUP_UI | true | TC_Default | no |

Verification detail:
- Size verified twice per asset: live `TextureTools.get_size` (512×512, no async-compile 32×32
  placeholders at read time) AND asset-registry `Dimensions` tag ("512x512") — all 6.
- Properties readback (`ObjectTools.get_properties`) post-set, and registry tags post-save,
  both show TEXTUREGROUP_UI / SRGB True / TC_Default on all 6. `AssetImportData` on each points
  at the correct `../../RawAssets/CardArt/<CardID>.png` source.
- Exact casing: PNG filenames checked character-for-character (`-ceq`) against cards.csv row
  names on a real directory listing; `find_assets` on /Game/UI/CardArt returns exactly **28**
  assets (22 M4 + these 6), each new path string-exact `/Game/UI/CardArt/T_CardArt_<CardID>`.
- Visual spot check: in-editor `CaptureAssetImage` of T_CardArt_CrystalTower matches the source
  PNG (sRGB behaving).

## Per-card content + color key (all keys distinct within Set III AND vs the M4 22)

| CardID | Subject (one dominant) | BG color key |
|---|---|---|
| Fireball | The projectile itself: yellow-hot core ball, layered orange flame taper, deep-red tip, orange sparks, flying up-right diagonal | deep ember umber |
| FrostNova | Grounded ice burst: central hex spike + ring of outward-leaning shards + 2 expanding emissive frost rings + frost disc | cold deep blue |
| Lightning | One bold white-violet zigzag bolt (extruded ribbon) from a dark storm cloud to a glowing impact ring + radiating shards | dark indigo storm |
| BattleCry | Crescent war horn (dark bronze, gold bands, gold bell lip, glowing bell mouth) blasting 3 radiating gold sound rings up-right | deep amber gold |
| Pickpocket | Coin-snatch: tipped leather pouch spilling a pile of gold coins; dark-green gloved hand pinching one lifted coin; sparkle glints | thief green (+ gold) |
| CrystalTower | The TASK-105 SM_CrystalTower mesh itself (live Blender scene reused): stepped stone tower, fanned crystal crown + needle, cyan glow | deep teal |

- **CrystalTower card = the actual mesh:** rendered from the live TASK-105 Blender scene geometry,
  so card and in-game building read as the same object. Card materials: neutral stone (team-agnostic
  — NOT the blue placeholder) + emissive cyan crystals in the M_CrystalGlow color family
  (0.05–0.10, 0.75–0.80, 1.00).
- Team-agnostic law honored: no saturated team-blue/red fields (FrostNova's navy is well below the
  MI_TeamColor_Blue chroma; precedent = Footman's steel cyan-teal).
- NO baked text; full-bleed opaque squares (no alpha); headroom top-center + floor strip bottom kept
  clear for the WBP name/cost overlays, matching the M4 set.

## Reworks after the eyeball pass (style notes)

- **Rig (affects family consistency):** first CrystalTower pass had a hot hard-edged light wedge on
  the backdrop top — A/B diagnosis isolated it to the rim light nearly touching the backdrop at
  large subject scale. Kept (it IS the M4 corner-wash look) but softened: rim clamped in front of
  the backdrop, panel enlarged, energy cut ~2.3×, spread narrowed.
- **Fireball (2 reworks):** emissive shell first blew out to a flat yellow egg swallowing the trail;
  rebuilt as core+layered cones. Then learned the clamp law: with view transform Standard, orange
  emission above ~1.5 total clamps to yellow/white — final emission strengths 0.95–2.2 keep the hue.
- **Lightning (2 reworks):** box-chain bolts read as stick figures/lamp stands; final bolt is a
  single flat tapering zigzag ribbon polygon (from_pydata + solidify) — the iconic ⚡ silhouette.
  Cloud darkened (first pass read friendly-fluffy).
- **BattleCry (2 reworks):** sound rings edge-on read as a tornado swirl; bell axis yawed toward
  camera so rings open to the viewer and the glowing bell interior + gold lip show.
- **Pickpocket (2 reworks):** first coins rendered dull brown (metal 1.0 in a dark scene — lowered
  metalness, added gleam emission); fingers crossed the held coin's center making it a donut — grip
  moved to the coin's top edge, coin enlarged.

## Deferred / notes for downstream

- **DT_Cards row readback: DEFERRED to TASK-109** per orchestrator dispatch — TASK-104 (DT_Cards
  reimport) had not run at TASK-106 time; cards.csv already carries the deterministic
  `/Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>` cells (TASK-097), and the hand UI falls
  back gracefully (text-only face) until the reimport lands. TASK-109 should confirm one Set III
  row's CardArt path resolves live.
- **UMG needs no direct texture references** — runtime path is the TASK-079 resolvers
  (`GetCardArtTexture(CardID)`), driven by DT_Cards soft paths.
- **Import warnings:** none observed via tool returns; each import returned exactly one Texture2D
  (no side artifacts). Format DXT1, no alpha — expected for RGB PNGs.
- **Git state (nothing touched by me):** new on disk for the M5 commit (TASK-109): the 6 PNGs at
  `Content/RawAssets/CardArt/` (untracked) + 6 new
  `Content/UI/CardArt/T_CardArt_<CardID>.uasset` (editor's Git provider may auto-stage on save —
  known behavior, build-master adjudicates).
- Blender scene left with SM_CrystalTower restored visible; card-render materials on it are
  scene-local only (FBX on disk untouched).
