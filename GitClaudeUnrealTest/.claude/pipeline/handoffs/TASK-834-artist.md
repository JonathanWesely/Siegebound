# TASK-834 — `T_CardArt_Witch` — art-director handoff

**Status: ready-for-integration** · resumed 2026-09-03 after the previous agent was killed mid-task by a network outage.
The previous agent left **no handoff** (`TASK-834-artist.md` did not exist). This file is written from scratch.

---

## 0. What was actually on disk — the dispatch premise was wrong, and correcting it saved the generation quota

I was told *"`Content/RawAssets/Concepts/Witch.png` EXISTS … a concept image exists but was mid-iteration and was never imported"*, and asked to decide **accept / re-tune / regenerate**.

**Measured before generating anything — that file is not this task's asset.** The two are different lanes, and CONVENTIONS keeps them isolated in both directions:

| file | lane | owner |
|---|---|---|
| `Content/RawAssets/Concepts/Witch.png` | TRELLIS/Meshy **mesh** lane — the image-to-3D input | **TASK-833** (shipped `SM_Witch`) |
| `Content/RawAssets/CardArt/Witch.png` | **card-art** lane — this task | TASK-834 |

`Content/RawAssets/CardArt/Witch.png` **did not exist**. `Concepts/Witch.png` is byte-identical in size (837,664) to `Tools/ArtPipeline/Cache/WitchRolls/seed71035.png`, written 22:09–22:10, i.e. *before* the TASK-833 handoff at 22:25 — it is that task's accepted concept roll, not a mid-iteration card face.

⇒ **There was nothing to accept or re-roll.** The M7-tier card face is a **Cycles render of the shipped `SM_Witch` mesh**, not a generated image. **Zero generation quota was spent, and none needed to be.** The previous agent had already moved past generation; its dying words about "clipping" and "a light my scale doesn't touch" were about a **Blender render**, not an image roll.

**What it actually left** (all in the session scratchpad, none in the repo): `cardart_witch.py` (a complete, well-built render script), `witchkey.json` (a key-colour search result), `solved.json` (camera + exposure solve), plus its ΔE2000 machinery. I **reused its render script and framing solve** and rewrote its acceptance targets. Credit where due: its script already coded around both recorded render-script traps correctly.

---

## 1. Why it was stuck — the root cause was never a light

Its `solve.py` drove `light_scale` 1.0 → **0.0924** chasing a clip bar it could never reach. Three separate defects, all measured:

**(a) `light_scale` had no authority.** It scaled only `Key`/`Fill`/`Rim1`/`Rim2`. `Halo` (1500 W), `Wash` (300 W, 96° cone at y=−2.0) and `FloorWash` (420 W, 104° cone at y=−2.6) sit **in front of the figure** aiming past her at the wall/floor, so they lit her front directly and were immune to the knob. Its own last note — *"the source is a light my scale doesn't touch"* — **was correct.**

**(b) ⚠️ THE REAL ROOT CAUSE — `SM_Witch` is baked as a fully metallic asset.** `T_Witch_ORM.png` blue channel (metallic) measured over the model:

| asset | metallic mean | median | p90 | frac > 128 |
|---|---|---|---|---|
| **Witch** | **239.0** | **254** | **255** | **95.2 %** |
| Wizard | 10.7 | 0 | 4 | 4.2 % |
| Sorcerer | 1.0 | 0 | 1 | 0.2 % |

A metal has **no diffuse response**, so under a dark world she renders near-black (figure median **0.017** at gamma 1.0) while the few specular hits saturate — figure **p99.5 = 1.0000 at every exposure tested** — and adding light only grows the hotspots (clip 2.2 % → 20.3 %) without moving the median. **That is the entire phenomenon read as "clipping asymptotes at 4.4 % regardless."** No lighting tune could ever have fixed it.

**(c) Both of its acceptance bars pushed the same way — darker.** Measured against all 31 shipped faces:

- it enforced `clip < 0.5 %` — **13 of 31 shipped cards fail that bar** (roster clip median 0.230 %, p90 3.058 %).
- it targeted figure median **[0.075, 0.150]** — **below** the roster median (0.1786) and far below both clean M7 references (**Sorcerer 0.2062**, **WatchTower 0.2600**).

So it was **converging on a bad face, not failing to converge**. When I first re-solved to its median target, the solver reached `albedo_gamma 0.26` — a range-crushing hack that turned a dark charcoal witch into **white speckled mush** while reporting median 0.204 / clip 2.34 %, *both numerically in family*. **Numbers cannot see that; I only caught it by rendering the clip map and looking.**

⭐ **Lesson worth keeping: an acceptance bar invented for a task, rather than derived from the shipped population, can be satisfied by an image nobody would ship.**

---

## 2. A recorded figure that is an artifact — she does **not** bake 3.8× darker

The dispatch recorded *"she bakes darker (mean 0.0250 vs the Wizard's 0.0968)"*. Both figures reproduce exactly on my instrument — **but they are whole-texture means, diluted by empty UV-atlas padding**:

| asset | atlas coverage | whole-tex mean | **model-only mean** |
|---|---|---|---|
| Witch | 26.2 % | 0.0250 | **0.0955** |
| Wizard | 66.0 % | 0.0948 | **0.1436** |
| Sorcerer | 72.3 % | 0.3524 | **0.4875** |

`0.0250 / 0.262 = 0.0954`. **Over the model she is 1.5× darker than the Wizard, not 3.8×.** The real gap is to the *Sorcerer* reference (5.1×). Recorded so the next person does not over-compensate exposure on a phantom.

---

## 3. Key colour — measured, with an instrument control

**Instrument control first** (the law's recorded n=29 distribution):

| | mine | law |
|---|---|---|
| min | **3.79 (Ogre↔PlateArmor)** | 4.07 (Ogre↔PlateArmor) |
| p10 | 4.38 | 4.13 |
| median | 7.69 | 7.85 |
| max | **12.53 (Pickpocket↔Archer)** | **12.53 (Pickpocket↔Archer)** |
| Sorcerer key | **(29,79,69) / hsv (168.0, 0.633, 0.310)** | (29,79,69) / (168.0, 0.633, 0.310) |

Same extreme **pairs**, max identical, Sorcerer key reproduced **exactly** ⇒ same instrument. (The previous agent's extractor sampled 4 corners at 24 px and got Sorcerer (25,62,54) / median 7.35 — close, but a different instrument. Only the **top** two corners are free of the floor wash and contact shadow.)

**Shipped key: `rgb8 (143,114,134)`, hsv (319.0, 0.20, 0.56)** — a pale, hazy mauve. Rendered corners measure **(142,114,134)**, within 1 of target.

- **min-ΔE2000 = 18.26 (Knight) → 100th percentile** (roster NN median 7.97, **max 12.05**). The most separated key on the board.
- ΔE to **Wizard 23.27**, to **Sorcerer 40.26** — the three casters are well apart.
- Hue is **24° from the Wizard's 294.5°**, deliberately: the law warns ΔE can be large while two cards still "read purple" at 150 px. A lilac at hue 299 scored similarly but sits **4°** off the Wizard — rejected for that reason.

**Two corrections to the previous agent's pick** (`hue 98, sat 0.85, val 0.46`):
1. `sat 0.85` is **outside the fleet's own saturation range** (0.065–0.717); its own `pickkey.py` had constrained sat ≤ 0.70, so the value that landed in `witchkey.json` did not obey its own constraint.
2. Maximising ΔE under only the fleet's outer sat/val bounds is **degenerate** — the argmax runs to a near-grey (sat 0.105, ΔE 31), which satisfies the metric and fails "distinct per-card **colour** key". A chroma floor removes that corner.

⚠️ **I also added a constraint the law does not currently state, because the first attempt failed on it.** A green at the ΔE argmax (60,117,34) was law-compliant and looked like a **greenscreen**. Measured: its backdrop-mean **Lab chroma was 54.6, above the roster maximum of 52.2** (median 23.3, p90 33.4). The shipped one measures **16.4** — comfortably inside. Green could be vivid-and-distinct or muted-and-generic but not both, because Archer/Longbowman/Pikeman already own muted green; the roster's violets are all *dark*, so a **pale** mauve was wide open.

---

## 4. Deliverables

| what | path |
|---|---|
| Card texture | **`/Game/UI/CardArt/T_CardArt_Witch`** (`Content/UI/CardArt/T_CardArt_Witch.uasset`) |
| Source render | **`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\CardArt\Witch.png`** |

**Verified by read-back, not by return value:** `SRGB = true` · `LODGroup = TEXTUREGROUP_UI` · `CompressionSettings = TC_Default` · **512 × 512** (`get_size`) — identical to shipped `T_CardArt_Sorcerer`. Engine-side thumbnail rendered and **visually confirmed** to be the final image.

**Save proven by sha256 on the `.uasset`, never by `is_dirty`** (blind instrument): `407d00dd…` (246,091 B) → **`29f5eb4d…` (291,189 B)**. Source PNG sha256 `9956d9e9…`.

**Final measured stats:** figure median **0.0853** · clip(figure) **0.67 %** (roster median 0.230 %, p90 3.058 %) · clip(frame) **0.138 %** · p99.5 **1.0** · corners **(142,114,134)** · backdrop mean chroma **16.4**, sat 0.196, val 0.602.

**Recipe** (CYCLES, never EEVEE): 512², Standard/None, 65 mm / 36 mm, 512 samples, `yaw 20°`, `cam_dist 4.3904`, `cam_z 0.8656`, headroom 10.0 % / floor 12.1 %. Slots assigned **in place** — verified `{0: 1312, 1: 13680}` unchanged (TRAP 1 did not fire); backdrop **fully matte** (TRAP 2). Facing **verified by positive identity cues** (robe crossover, lantern, ¾ kick), not assumed.

**Card-lane-only presentation grades — these touch NO shipped asset:** `metal_scale 0.0` (neutralises the ORM defect), `albedo_gamma 0.72`, `albedo_sat 0.55` (the bake's coloured speckle was being *amplified* at the inherited 1.15), `rough_bias 0.08`, `team_scale 0.25`, `floor_mul 0.30`.

---

## 5. ⚠️ Findings that need someone else's task

1. **⚠️ `T_Witch_ORM` metallic = 254/255 over 95.2 % of the model (§1b).** I neutralised it **for the card render only** — I am fenced from mesh/material work, and did not touch `SM_Witch`, `MI_Witch_PBR` or the ORM texture. **This also affects the mesh a player sees in game**: she is currently a rough metal, which is wrong for cloth/felt/leather and will read as dark chrome under level lighting. **Needs its own task.**
2. **The baked albedo carries heavy speckle** ("dalmatian" mottling), visible even at 150 px. Same TRELLIS bake; desaturating to 0.55 made it monochrome and much calmer, but it cannot be removed from the card lane. Same fix-at-source as (1).
3. **`Tools/ArtPipeline/cardart_render.py` still does not exist** — the manager's annotation on this row predicted this. I used a scratchpad script (`w834_render.py`, derived from the previous agent's `cardart_witch.py`), matching the `cardart_watchtower.py` precedent. **Its durable home is `TASK-385`.** My script adds three things worth carrying into it: `--metal-scale`, `--floor-mul`, and **Cycles light linking** for subject/backdrop separation.
4. **`--check` gap confirmed independently.** I ran it and **observed exit code 0** (see §6). The guard exercises **7** RED fixtures (pure-black, near-black-noise, dark-noise-unlit, pure-white, flat-mid-grey, fully-transparent, tiny-glitch) and 3 green ones — **there is no `unreadable-artefact` fixture among them**, matching QA's finding on TASK-865. Not mine to fix (`TASK-879`).
5. **The `.uasset` appeared already git-staged (`A `)** after the editor saved it — I ran no git commands (Git is fenced). Build-master should be aware the working tree was touched by the editor's revision-control integration.

---

## 6. `--check`, observed

```
uv run concept_generate.py --check
[concept] --check: degeneracy guard control PASSED - RED on every degenerate fixture, green on every legitimately-dark one.
[concept] --check: HF_TOKEN is present in the environment (value not read/echoed).
[concept] --check PASSED: tool wires up (model constant: black-forest-labs/FLUX.1-dev).
```
**OBSERVED EXIT CODE: 0.** (Declared-vs-observed mattered here: QA could not execute it. I did.) The token was never read, echoed, logged, or passed on argv.

---

## 7. Does it resolve TASK-831's soft pointer?

**Yes — verified against the exact CSV string, not assumed.**

`Docs/Data/cards.csv` column 23 `CardArt` for row `Witch` = `/Game/UI/CardArt/T_CardArt_Witch.T_CardArt_Witch`.
`load_asset` on that **exact** path returns `refPath: /Game/UI/CardArt/T_CardArt_Witch.T_CardArt_Witch`; `get_asset_class` = `Texture2D`; `get_referencers` = `["/Game/Data/DT_Cards"]`.

⇒ **`ResolveCardArtTexture`'s log-once text-only fallback will no longer fire for the Witch.** I did **not** edit `DT_Cards` or `cards.csv` (TASK-831 shipped the row; fenced).

⚠️ **One process note for the record.** The MCP `import_file` **refuses to overwrite an existing asset**, and no reimport tool exists in this MCP surface — so the same-path overwrite the law requires is **not directly available**. For the second import I deleted and re-imported **at the identical path**. That was safe *here* and is not a precedent: the asset was minutes old, uncommitted, and its sole referencer was a **soft path string** that stays byte-identical, so nothing could break — and I re-verified the referencer came back. **For any card with hard references this would need the `-run=pythonscript` in-place reimport route instead.**

---

## 8. For Jonathan's eye

- **The face itself** — she reads as a hooded caster (wide conical hat, void face, lantern) and sits correctly in the family at 150 px beside Sorcerer / WatchTower / Wizard / Archer / Cleric / Knight / Ogre.
- **She has no face** (recorded, expected — the void under the brim is the mesh, not a render fault).
- **The speckled robes** are the mesh's bake, not the card lighting (§5.2). If that reads as wrong to him, the fix is at the asset, not here.
- **The metallic finding (§5.1) is the one with in-game consequences**, and is the thing most worth his ruling.

## 9. Not touched (fence)
No mesh · no material · no `DT_Cards` / `cards.csv` edit · no code · `concept_generate.py` **not** modified · **no Git**. `L_Arena` never opened or saved; only `/Game/UI/CardArt/T_CardArt_Witch` was saved, by explicit path — never an empty list.
