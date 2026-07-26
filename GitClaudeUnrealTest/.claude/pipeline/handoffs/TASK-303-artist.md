# TASK-303 handoff — `T_CardArt_Wizard` card art (art-director)

**Status: 2D generation COMPLETE — PNG staged, verified 512×512. UE import NOT done (deliberate).**
Per orchestrator instruction, this was a NO-EDITOR task: stopped at the PNG-on-disk boundary. The
Unreal editor is occupied by the in-flight deck-builder fix; the editor-gated import serializes and
is dispatched separately. **No Unreal editor, no Unreal MCP (`mcp__unreal-mcp__*`), no deck-builder
widget, and no Git were touched.** Only the Blender MCP (my art tool) was used.

## Deliverable
- **Staged source PNG:** `Content/RawAssets/CardArt/Wizard.png`
- **Verified:** RGB, **512×512** (confirmed two ways — PNG-header readback inside Blender AND PIL
  outside Blender: `mode RGB size (512, 512)`). 8-bit, opaque (no alpha), no baked-in text.
- **Pending import target (for the separate editor-gated dispatch):** import as
  `/Game/UI/CardArt/T_CardArt_Wizard` (Content/UI/CardArt/) — **Texture Group = UI, sRGB ON,
  default compression** (identical to the 22 originals, TASK-078). The DT_Cards `CardArt` cell
  (TASK-299) already points at `/Game/UI/CardArt/T_CardArt_Wizard.T_CardArt_Wizard`, so the import
  closes the current text-only-fallback gap. No `.uasset` exists yet (confirmed absent).

## How the roster style was matched (method + framing)
The existing 28 `T_CardArt_*` are NOT AI illustrations — they are **Blender EEVEE renders of custom
low-poly "board-game token" primitive figures on a per-card color-keyed studio backdrop** (recipe in
the TASK-077 handoff). I reproduced that exact pipeline rather than cropping the full-body Meshy
concept:
- **Engine/output:** EEVEE, 512×512 @ 100%, PNG RGB 8-bit, **View Transform = Standard, Look = None**
  (the roster's punchy-saturated-key setting for small-size readability).
- **Shared studio rig:** floor + back-wall planes in the card's key color; camera 65 mm, slight high
  angle, tracked to subject center; **warm key + cool fill + white rim (no shadow) + a backdrop halo
  spot (no shadow, aimed over the head)** → the signature radial glow behind the subject. Emissive
  accents via Principled emission (the roster's torch/halo/orb/gem-glow technique).
- **Framing = full-figure token, centered, filling the frame with a strong silhouette**, clear
  headroom top-center + a floor strip bottom for the runtime DisplayName/cost overlay (matches
  Archer/Cleric/Ogre/Longbowman etc.). One dominant subject.
- **Team-agnostic palette** (no team-blue/red field): blue-grey robe, tan hood-lining collar, gold
  accents, warm-orange fire — plus an **arcane-plum key** chosen to be distinct from every used key
  (it is warmer/more magenta than DeepMine's deep violet, WarBanner's royal violet, and Knight's
  indigo slate) AND complementary to the orange fireball, so the fireball reads as the pop.

## Consistency with the approved concept (`Content/RawAssets/Concepts/Wizard.png`)
Token-tier translation of the concept's identity so it reads at ~150 px: hooded robe, **big bushy
beard**, **glowing amber eyes** under the hood, a **staff topped with a red emissive orb** in one
hand, and a **bright flaming fireball in the raised casting hand** as the dominant subject — the
fire-mage silhouette. Fire-caster identity (the batch's whole point) is unmistakable at card size.

## Notes for the separate editor-gated import dispatch
- Full-bleed opaque 512×512 square, no alpha/border — safe edge-to-edge on the card face; overlay
  text lands on the top-headroom / bottom-floor strips (art has no baked text).
- Exact-casing: file is `Wizard.png` (matches the `Wizard` cards.csv row / CardID character-for-char).
- After import: set LODGroup = TEXTUREGROUP_UI, sRGB = true, compression = TC_Default; verify
  512×512 by post-import readback; save. Then add BOTH the PNG (raw-asset rule) and the new
  `Content/UI/CardArt/T_CardArt_Wizard.uasset` to the batch commit (build-master).
- Roster CardArt count is now 29 PNGs (28 prior + Wizard).
