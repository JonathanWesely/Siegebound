# TASK-077 handoff — Card artwork: 22 card illustrations rendered (art-director)

**Status: complete — 22/22 rendered, verified 512×512, on disk under `Content/RawAssets/CardArt/`.**
File-side only per spec: no Unreal editor work, no .uasset mutation, no Git. Import is TASK-078.

## What was built

All 22 illustrations were rendered in the live Blender (MCP bridge) with a single shared studio rig
authored as a Python module (session scratchpad `card_rig.py`, executed via `execute_blender_code`).
Subjects are custom stylized low-poly primitive compositions (chunky "board-game token" style) — the
blockout FBX donors were NOT imported (untextured gray donors would have needed materials anyway;
building compositions from primitives kept one consistent style family across units, buildings, AND
the icon-only cards). Style law honored: one dominant subject filling the frame, strong silhouette,
distinct per-card background color key, team-agnostic palette (no saturated team blue/red fields),
zero baked-in text. Every render was eyeballed at full size; the weak ones were reworked (see below).

## Render settings (identical across all 22)

- Engine: EEVEE (`BLENDER_EEVEE`, Blender 5.1), 48 samples, 512×512 @ 100%, PNG RGB 8-bit
- View transform: Standard (punchy saturated keys for small-size readability), look None
- Rig per card: backdrop plane + floor in the card's key color; camera 65 mm, slight high angle,
  track-to subject center; framing auto-computed from per-card subject height
- Lights: warm key (area 240 W), cool fill (area 65 W), white rim (area 300 W, no shadow),
  backdrop halo spot (1400 W, no shadow, aimed over the subject's head) → radial glow behind the
  subject in a lightened key color
- Emissive accents (fuse sparks, torch flame, lamp, halo/orb, gold veins, gleam, speed lines) via
  Principled emission

## Per-card content + color key

| CardID | Subject (one dominant) | BG color key |
|---|---|---|
| Footman | Infantryman, raised sword + round wooden shield, steel helm | steel cyan-teal |
| Archer | Hooded archer, big drawn bow with nocked arrow, quiver | forest green |
| Knight | Heavy plate figure, great helm w/ gold plume, kite shield + longsword | indigo slate |
| Miner | Miner in blue-gray overalls, lamp helmet, upright pickaxe, gold pile at feet | ochre amber |
| ArrowTower | Round crenellated stone tower, arrow slit, leaning hero arrow, pennant | warm sand |
| Wall | Crenellated stone wall segment with proud brick relief | neutral dark gray |
| MilitiaMob | Four small peasants: pitchfork, raised torch (lit), club | rust orange |
| Pikeman | Kettle-helm soldier gripping a frame-crossing long pike | olive |
| Sapper | Dark-clad sapper hugging a banded powder keg, lit fuse + sparks | charcoal + ember |
| Cavalry | Horse in profile with armored rider and couched lance | burgundy |
| Longbowman | Capped archer beside a planted man-tall longbow, arrow in hand | light sage |
| Cleric | Ivory-robed figure, glowing halo ring + orb-topped staff | soft teal |
| Ogre | Huge green brute, tusked underbite, spiked club over shoulder | dark slate |
| BombTower | Squat dark tower, giant round bomb on top, lit fuse, ember windows | ember near-black |
| BallistaTower | Stone tower with giant bolt-thrower aimed up-right | dark cyan |
| Barracks | Timber-framed hall, gabled red roof, dark door, gold flag | timber brown |
| DeepMine | Rock mound, timber-framed dark entrance, glowing gold veins, ore cart | deep violet |
| Masons | Half-built staggered brick wall + hero trowel + mallet | terracotta |
| SharpenedBlade | One giant bright upright sword, emissive edge gleam + tip sparkle, whetstone | dark maroon |
| PlateArmor | Steel cuirass with pauldrons + gold rivets on an armor stand | gunmetal blue |
| SwiftBoots | Pair of dark-leather winged boots (white feather fans) + speed lines | bright gold |
| WarBanner | Pole + crossbar with waving gold banner, dark two-disc sigil, gold finial | royal violet |

## Cards that were hard / reworked after the eyeball pass

- **Miner** — first pass hid the pickaxe behind the head and the brown clothes vanished into the
  amber bg; repositioned the pickaxe beside the figure and switched overalls to cool blue-gray.
- **SwiftBoots** (hardest, 2 rework rounds) — boots-only iconography is inherently weak at this tier;
  first pass read as "cylinders on boxes" with wings hidden behind the shafts. Rebuilt: side-fanning
  white feather wings, dark leather vs the gold key, tighter framing, emissive speed lines.
- **WarBanner** — emblem discs originally matched the bg violet and read as HOLES in the cloth;
  recolored dark brown.
- **SharpenedBlade** — fully metallic blade rendered near-black (dark world reflections) and the
  sparkle floated beside the tip; switched to a bright low-metallic blade, sparkle centered on the
  tip apex.
- **ArrowTower / Wall / Ogre / PlateArmor** — minor: arrow clipped at frame edge (repositioned),
  wall washed out (darker stones + brick contrast), Ogre bg too close to its skin and to the other
  green cards (bg → dark slate), pauldrons floated like ears (lowered onto the shoulders).
- Global rig fix during tuning: first light pass was overexposed and the halo spot washed the
  subject with white — energies cut ~3×, halo re-aimed from above the subject, halo/rim shadows off.

## Verification (all files confirmed on disk via Blender image load)

| CardID | File | Dimensions |
|---|---|---|
| Footman | Content/RawAssets/CardArt/Footman.png | 512×512 |
| Archer | Content/RawAssets/CardArt/Archer.png | 512×512 |
| Knight | Content/RawAssets/CardArt/Knight.png | 512×512 |
| Miner | Content/RawAssets/CardArt/Miner.png | 512×512 |
| ArrowTower | Content/RawAssets/CardArt/ArrowTower.png | 512×512 |
| Wall | Content/RawAssets/CardArt/Wall.png | 512×512 |
| MilitiaMob | Content/RawAssets/CardArt/MilitiaMob.png | 512×512 |
| Pikeman | Content/RawAssets/CardArt/Pikeman.png | 512×512 |
| Sapper | Content/RawAssets/CardArt/Sapper.png | 512×512 |
| Cavalry | Content/RawAssets/CardArt/Cavalry.png | 512×512 |
| Longbowman | Content/RawAssets/CardArt/Longbowman.png | 512×512 |
| Cleric | Content/RawAssets/CardArt/Cleric.png | 512×512 |
| Ogre | Content/RawAssets/CardArt/Ogre.png | 512×512 |
| BombTower | Content/RawAssets/CardArt/BombTower.png | 512×512 |
| BallistaTower | Content/RawAssets/CardArt/BallistaTower.png | 512×512 |
| Barracks | Content/RawAssets/CardArt/Barracks.png | 512×512 |
| DeepMine | Content/RawAssets/CardArt/DeepMine.png | 512×512 |
| Masons | Content/RawAssets/CardArt/Masons.png | 512×512 |
| SharpenedBlade | Content/RawAssets/CardArt/SharpenedBlade.png | 512×512 |
| PlateArmor | Content/RawAssets/CardArt/PlateArmor.png | 512×512 |
| SwiftBoots | Content/RawAssets/CardArt/SwiftBoots.png | 512×512 |
| WarBanner | Content/RawAssets/CardArt/WarBanner.png | 512×512 |

Filename casing verified character-for-character against `Docs/Data/cards.csv` row names
(directory listing, not case-insensitive path lookup).

## Integration notes for TASK-078 (import)

- Import each `Content/RawAssets/CardArt/<CardID>.png` → `/Game/UI/CardArt/T_CardArt_<CardID>`
  (Content/UI/CardArt/), all 22.
- Settings per CONVENTIONS "Card artwork (hand UI)": **Texture Group = UI, sRGB ON, default
  compression.** Renders are 8-bit sRGB PNG, RGB (no alpha) — sRGB-on is correct.
- Verify 512×512 by readback post-import; save all.
- Images are full-bleed square compositions (opaque backgrounds, no transparency, no borders) —
  safe to draw edge-to-edge on the card face; DisplayName/cost overlays (TASK-080) sit on top;
  every card keeps clear headroom top-center and floor-strip bottom where overlay text will land.
- No baked text anywhere; all palettes team-agnostic.
