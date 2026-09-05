# TASK-984 — `T_CardArt_BrightSun` (+ TASK-842 `T_CardArt_Fog`) — art-director handoff

**Status: ready-for-integration.** Both faces of the anti-card pair were made in ONE batch, in ONE
Blender scene, because that is what the spec's hazard clause asks for. This file is the full batch
record; `handoffs/TASK-842-artist.md` is the Fog-side pointer to it.

**Law honoured:** "Card artwork (hand UI)" · `FOG-§10.1` · `SC-§39` · `SC-§40` cl. 10/13.

---

## 0. Deliverables — proven by read-back, never by a return value (`SC-§39`)

| what | path |
|---|---|
| Card texture | **`/Game/UI/CardArt/T_CardArt_BrightSun`** (`Content/UI/CardArt/T_CardArt_BrightSun.uasset`) |
| Card texture | **`/Game/UI/CardArt/T_CardArt_Fog`** (`Content/UI/CardArt/T_CardArt_Fog.uasset`) |
| Source render | `Content/RawAssets/CardArt/BrightSun.png` |
| Source render | `Content/RawAssets/CardArt/Fog.png` |
| Render script | session scratchpad `cardart_fog_sun.py` (see §6 — `cardart_render.py` still does not exist) |

**Verified in the editor, by property read-back and not by the import call's success:**

| | `T_CardArt_Fog` | `T_CardArt_BrightSun` | shipped `T_CardArt_Witch` (control) |
|---|---|---|---|
| `get_size` | **512 × 512** | **512 × 512** | 512 × 512 |
| `SRGB` | `true` | `true` | `true` |
| `LODGroup` | `TEXTUREGROUP_UI` | `TEXTUREGROUP_UI` | `TEXTUREGROUP_UI` |
| `CompressionSettings` | `TC_Default` | `TC_Default` | `TC_Default` |
| `MipGenSettings` | `TMGS_FromTextureGroup` | `TMGS_FromTextureGroup` | `TMGS_FromTextureGroup` |

`exists("/Game/UI/CardArt/T_CardArt_Fog")` → **true**; same for `T_CardArt_BrightSun`. Both returned
**false** before I started, so **both are clean first imports** and the recorded
`import_file`-cannot-overwrite trap (CONVENTIONS "Card artwork" cl. 94-97) **never arose** — no
delete+re-import was performed, and that fenced exception was not exercised or extended.

`load_asset` on the **exact full object path** `TASK-983` will write into the CSV
(`/Game/UI/CardArt/T_CardArt_BrightSun.T_CardArt_BrightSun`) resolves and returns that same path;
`get_asset_class` = `Texture2D`.

**sha256, on the bytes on disk:**

```
.uasset  T_CardArt_Fog        513322e7daa7d94fe828a75c45faae3cd9315f675d49ebc145764877e79e2128   (231,798 B)
.uasset  T_CardArt_BrightSun  15570b043fe0ab7fd1205b54d582a1fdbdf796187fce1ee85072edcd8d70a387   (242,724 B)
.png     Fog.png              62467573e3a5678712d6fe6417df1da2d933addfa0367e6be09e13e5e2d04b62
.png     BrightSun.png        2671f9b5f41ceaf0c93a5b209a3d54db426a6d5087a500cf7a9a2f45bd4417b0
```

⚠️ **`get_referencers` is EMPTY on both, and that is correct today** — `TASK-983` has not yet written
the `BrightSun` row and `TASK-840` has not yet written the `Fog` row. The soft path resolving is the
part I *can* prove now, and I proved it. The referencer check belongs to whoever lands those rows.

---

## 1. ⛔⛔ THE DEFECT THAT SHIPPED TWO WRONG FACES WHILE REPORTING CORRECT NUMBERS

> ### **`Image.pixels` on an 8-bit sRGB PNG returns the RAW ENCODED bytes. Blender does NOT linearise them.**

My key solve treated that reading as **linear** and drove the corners to `s2l(target)`. Both faces
came out **about two stops dark** — and the script printed corner values that **matched target on
every iteration**, because it was converting its own wrong reading back with the inverse function.
Two errors that cancel in the report and do not cancel in the image.

**It was caught by cross-reading the finished PNG with the SHIPPED-ROSTER instrument** (PIL, raw sRGB
bytes) — the one already proven against the law. The two instruments disagreed: roster instrument
said the Fog key was `(67,88,103)`, the render script said `(140,159,170)`.

**Control, run to settle it rather than to argue it:** loading the shipped `Sorcerer.png` through the
render script's own reader returns `0.11373 / 0.30980 / 0.27059`, and **×255 that is exactly
`(29,79,69)`** — the law's own recorded Sorcerer key, and byte-identical to what PIL reads.

⇒ Fixed: the read stays in sRGB, the **multiplicative update** is done in linear. The solve now
converges **byte-exactly in two iterations** instead of crawling for six. Both faces re-rendered.

⚖️ ***The lesson is `SC-§39` in its sharpest form: the script was not lying about what it measured —
it was measuring the wrong thing, and a self-consistent pipeline cannot detect that about itself.
Only an instrument with a different provenance can.*** The corner numbers were never evidence; the
**cross-read** was.

---

## 2. Key colour — BOTH "obvious" choices were refuted by measurement

**Instrument control first** (the law's n=29 distribution, reproduced before being used):

| | mine | law |
|---|---|---|
| min | **4.07 (Ogre↔PlateArmor)** | 4.07 (Ogre↔PlateArmor) |
| p10 | **4.13** | 4.13 |
| max | **12.53 (Pickpocket↔Archer)** | 12.53 (Pickpocket↔Archer) |
| cards clearing ΔE 10 | **5 of 29** | 5 of 29 |
| Sorcerer key | **(29,79,69) / hsv (168.0, 0.633, 0.310)** | (29,79,69) / (168.0, 0.633, 0.310) |
| Sorcerer↔Archer | **10.14** | 10.14 |

Same extreme **pairs**, same min, same p10, same max, same count, Sorcerer key and its recorded
nearest-neighbour distance reproduced **exactly** ⇒ same instrument. This pinned the sampling at
**64 px top-two-corner medians** (24/32/48 px all give a *different* Sorcerer key — close enough to
look right and wrong).

📌 **One correction to the law's own bookkeeping, offered as a finding, not a fix:** that distribution
reproduces exactly over the **28 legacy faces with Sorcerer EXCLUDED**. Including Sorcerer moves the
max to 11.04 (SwiftBoots↔ArrowTower) and the ≥10 count to 6. The label "n = 29" is consistent with
Sorcerer being the card *judged against* those 28 rather than a member of them — which is also how
the law's own Sorcerer bullet reads. Nothing downstream changes; recorded so the next person who
fails to reproduce "max 12.53" knows why.

### The two refutations

- ⛔ **A grey/white fog key is the worst available choice.** The spec warned this was an assumption
  worth measuring, and it is: the near-neutral band **at low value** is the most crowded region on
  the wheel and owns the roster **minimum** (Ogre↔PlateArmor 4.07). Sapper C\* 1.6 · Wall 2.9 ·
  Ogre 4.6 · BombTower 8.4 · PlateArmor 9.1 all live there.
- ⛔ **A warm-yellow sun key is nearly as bad.** Hue 9°–44° holds **nine** shipped cards
  (SharpenedBlade, Fireball, BombTower, Masons, MilitiaMob, Sapper, BattleCry, Barracks, Miner,
  SwiftBoots, ArrowTower).

### ⭐ But the neutral band is crowded ONLY AT LOW VALUE — and that is what made Fog possible

**No shipped key is a pale desaturated one.** Every roster neutral is dark (val 0.225–0.361). Sweeping
the cold low-saturation region at *high* value returns min-ΔE **19–27 at p100**, the freest space on
the entire board. So the fog card gets to be **literally the colour of fog** — not despite the
measurement, but *because* of it, once the measurement was taken on the right axis.

### The shipped keys, re-measured ON THE SHIPPED BYTES (not on the targets)

| | `T_CardArt_Fog` | `T_CardArt_BrightSun` |
|---|---|---|
| key rgb8 | **(139, 156, 168)** | **(176, 169, 79)** |
| hsv | (204.8, 0.173, 0.659) | (55.7, 0.551, 0.690) |
| L\* / C\* | 63.4 / 9.0 | 68.1 / 47.6 |
| **min-ΔE2000 to all 32 shipped** | **19.41** | **13.57** |
| **percentile vs roster NN** (median 8.23, max 17.75) | **100th** | **97th** |
| nearest shipped key | Cleric 19.41, Footman 19.67 | SwiftBoots 13.57, Longbowman 18.29 |
| hue sep to nearest shipped hue | 1.8° (see note) | **10.0°** |

⚠️ **Fog's 1.8° hue separation is not the hazard it looks like, and I am naming it rather than hiding
it.** At C\* 9.0 hue is nearly meaningless perceptually; the separation is carried by **lightness**
(L\* 63.4 against a roster whose neutrals sit at L\* 24–45), which is exactly what ΔE2000 weighs and
why it returns 19.41. The "far in ΔE but reads alike at 150 px" failure the law warns about is a
*chroma* phenomenon. Confirmed by eye on the contact sheet (§4).

### 🎯 THE PAIR — the spec's actual question

> **ΔE2000(`T_CardArt_Fog`, `T_CardArt_BrightSun`) = 32.41 · hue separation 149.2°**

**Measured on the shipped bytes**, not on the targets and not on the pre-fix renders. That is
**3.9× the roster's median neighbour distance and 1.8× its maximum** — relative to each other these
are the two most separated cards on the board. `T_CardArt_Fog` **exists** (I made it in this batch,
`TASK-842`), so this is a real measurement, not a reserved key.

They also differ on every non-colour axis: cold vs warm, soft vs hard, diffuse vs radial, low-contrast
vs blown-highlight. **Confusing them in hand is not a realistic failure.**

---

## 3. Acceptance stats — FRAME-WIDE re-derivations, and why

⚠️ **The law's recorded figures (figure-median 0.1786, clip(figure) 0.230% / p90 3.058%) are NOT quoted
here and do NOT apply.** They were measured on a **mesh figure mask**. A spell face has no
`SM_<CardID>` and these two are atmospheric — there is no figure to mask. Quoting them would be
comparing different quantities. **I re-derived the frame-wide equivalents over all 32 shipped faces**
so the comparison is like-for-like.

**Corpus, n = 32, frame-wide:**

| metric | min | p10 | median | p90 | max |
|---|---|---|---|---|---|
| frame median | 0.239 | 0.307 | 0.421 | 0.542 | 0.690 |
| clip ≥ 0.996 | 0.000 | 0.000 | 0.112% | 1.182% | 10.963% |
| p99.5 | 0.561 | 0.615 | 0.875 | 1.000 | 1.000 |
| contrast (p95−p5) | 0.176 | 0.213 | 0.271 | 0.766 | 0.840 |
| frame chroma | 2.15 | 7.88 | 23.39 | 30.21 | 40.59 |

| metric | Fog | BrightSun |
|---|---|---|
| frame median | 0.699 — **p100, OUTSIDE by 0.009** ⚠️ | 0.643 — p97, INSIDE |
| clip ≥ 0.996 | **0.687%** — p78, INSIDE | 8.215% — p97, INSIDE |
| p99.5 | 0.997 — p78, INSIDE | 1.000 — p78, INSIDE |
| contrast | 0.474 — p81, INSIDE | 0.859 — **p100, OUTSIDE by 0.019** ⚠️ |
| frame chroma | 9.91 — p19, INSIDE | 36.95 — p97, INSIDE |
| backdrop chroma (corner patches) | 9.10 — p16, INSIDE | **48.23 — p97, INSIDE** |

### ⚠️ Two declared excursions, stated as excursions

1. **Fog frame-median 0.699 vs corpus max 0.690 (+1.3%).** A direct, unavoidable consequence of the
   pale key — the key that earned the **100th percentile**. This is the brightest card on the board
   because it is the only pale-keyed one, and that is the same fact stated twice.
2. **BrightSun contrast 0.859 vs corpus max 0.840 (+2.3%).** A deliberately blown sun disc against
   near-black silhouettes. Fireball and FrostNova already sit at 0.77–0.84 with the same structure.

Neither is dressed as an optimum.

### ⭐ And one excursion that was a MEASUREMENT ARTEFACT, caught before it was reported as real

My first pass measured "backdrop chroma" as the **top 64 ROWS** and got BrightSun at **52.56 vs a
roster max of 51.84 — OUTSIDE**, which is the same shape and nearly the same magnitude as the
greenscreen candidate `TASK-834` rejected (54.6 vs 52.2). Before accepting or fixing it I checked
whether the *definition* was faithful: **for BrightSun that band contains the sun's corona and rays —
the subject, not the backdrop**, while for every shipped card the band is pure backdrop.

Re-measured over the **top-corner patches** — definitionally backdrop, and the exact region the key
instrument samples — BrightSun is **48.23 against a roster max of 50.64 (WatchTower): INSIDE, p97.**

⚖️ ***A metric that includes the subject in the "backdrop" region is not measuring what its name
says. I nearly re-rendered a correct card to satisfy a broken ruler.*** Both numbers are reported
above with their regions named.

---

## 4. 👁️ Rendered and LOOKED AT — the numeric pass was never acceptance on its own

Per the law's escape-hatch clause, every acceptance run renders the artefact **and its clip map** and
a human-equivalent look happens. It earned its keep **four times** in this batch:

1. **The ΔE argmax ran to a bounding-box corner, twice, and both candidates cleared every stated
   numeric bar.** Once to a vivid mint at **hue 165 — 3° from the Sorcerer**, which is precisely the
   "far in ΔE, reads alike at 150 px" failure the Witch's lilac was rejected for. Once, after I
   pinned value, to `rgb8 (68,66,38)` — **a dark olive-brown, for a card called _Bright Sun_.**
   ⇒ Intent must pin the hue band **and** the value half; the search maximises what is left.
   **An optimiser sitting on its bound is a red flag, not a result.**
2. **Fog's halo clipped to a hard white hole** — 11.09% of frame, *above* the corpus max 10.96%, and
   it looked like a sticker. Softened; now **0.687%** (p78) and it reads as a sun diffused through
   fog. The clip map is what showed it.
3. **BrightSun's cloud occluders rendered as flying saucers** — flattened emissive spheres seen
   edge-on. Replaced with soft-edged volumetric density blobs.
4. **A hard-edged disc twice** where a glow was wanted: an emissive *surface* keeps its silhouette
   through fog. Both the fog halo and the sun corona are now **volumetric** glow with a smooth
   density falloff, so there is no silhouette to go hard.

**150 px contact sheet against the shipped spell family** (Fireball / FrostNova / Lightning /
BattleCry / Pickpocket) plus the M7 references: both cards read instantly, read as a **pair**, and
read **apart**. Neither collides with Fireball (dark maroon, orange comet) or FrostNova (saturated
blue, white spikes).

---

## 5. The faces, and one deliberate deviation for Jonathan's eye

**Both cards are ONE scene.** Identical field, identical camera, identical silhouettes — a castle
tower, a crenellated wall, four pikes with a banner, two further towers, a horizon ridge. Only the
atmosphere differs:

- **`Fog`** — the field drowns. Depth is erased: the near wall is veiled, the mid tower is a ghost,
  the ridge is **gone entirely**. A soft veiled light-source glows behind the castle. Cold, pale,
  low-contrast, formless.
- **`BrightSun`** — the same field blazes clear. A white-hot sun with a hard ray burst, the last of
  the fog torn into ground wisps, and **the ridge is now visible** — the reveal *is* the card.

⚠️ **DECLARED DEVIATION, because it is a style call and it is his to overrule:** the 32 shipped faces
are all **one object on a plain keyed backdrop**. These two are **landscapes**. I chose that because
`Fog` and `BrightSun` are the only map-wide, battlefield-scale spells in the deck — "you cannot see
the field" and "the field is revealed" have no single object to be. It also gives the pair a shared
skyline, which is what makes them read as anti-cards at a glance. **If he wants them in the
one-object family style instead, that is a re-render of the same script, not a rebuild.**

**Recipe** (both): **CYCLES, never EEVEE** · 512×512 · Standard / None · 65 mm on 36 mm sensor ·
900 samples (BrightSun) / 640 (Fog) · denoised · 12 volume bounces. Backdrops **fully matte**
(TRAP 2 honoured — Principled's default specular would sheen the backdrop and pin the corner key).
TRAP 1 is not reachable here: `materials.clear()` is never called on a multi-slot mesh.
**No baked-in text. Team-agnostic — no blue, no red.** Full-bleed opaque, safe to draw edge-to-edge.

📌 **For the overlay (`WBP_CardHand` / `WBP_DeckCardTile`):** `Fog` is the **palest card on the
board** (frame median 0.699). White DisplayName/cost text will need its contrast strip or shadow —
allowed by the face-composition law, but worth knowing before it looks wrong. BrightSun's top-centre
is the blown sun, also bright. Both keep their **bottom strip** relatively dark and uncluttered.

---

## 6. ⚠️ Findings that need someone else's task

1. ⛔ **The Blender MCP bridge is DOWN — observed, not inferred.** `mcp__blender__get_scene_info`
   returns *"Cannot connect to Blender at localhost:9876"*; the GUI is not running. **It did not
   block me** — render work is headless by law (the 30 s socket cap), and all of it went through
   `blender.exe --background`. Recorded because **a later task that assumes an interactive Blender
   will hit this**, and the failure will look like a task problem rather than an environment one.
2. ⛔ **`Tools/ArtPipeline/cardart_render.py` STILL DOES NOT EXIST** — third task in a row to report
   it (`TASK-834` §5.3, `TASK-864` §6.3). I used a scratchpad script, matching the
   `cardart_watchtower.py` / `w834_render.py` precedent. **Its durable home is `TASK-385`.** What
   this batch adds and should be carried in: the **sRGB-vs-linear key solve of §1**, the
   **volumetric glow-ball** helper (soft light with no silhouette), and the **height-gradient volume**
   helper (a fog sea with an undulating top).
3. ⛔⛔ **My render script does NOT have the stage→validate→swap discipline `concept_generate.py` just
   gained**, and I am naming it against myself: it writes the final render **straight to
   `Content/RawAssets/CardArt/<CardID>.png`**. When I re-rendered Fog to fix the halo, the previous
   good roll was overwritten in place with no validation gate. It happened to be fine — I read the
   result back — but that is the *exact* shape of the defect `TASK-864` repaired one lane over.
   ⇒ **`cardart_render.py` should be BORN with `TASK-864`'s stage→validate→swap and its degeneracy
   guard**, which is `TASK-864` §6.3's recommendation, now with a second worked example behind it.
4. ⚠️ **`--check`'s guard control has no `unreadable-artefact` fixture — INDEPENDENTLY CONFIRMED, a
   second time.** I ran `uv run concept_generate.py --check` and **OBSERVED EXIT CODE 0**. It
   exercises 10 fixtures (7 RED / 3 green) and each *named* check owns a fixture where it fires
   alone — `no-lit-content`, `no-structure`, `degenerate-size`, `fully-transparent`. **The fifth
   check, `unreadable-artefact`, is never exercised.** This matches `TASK-834` §6 and QA's
   `TASK-865` finding; not mine to fix (`TASK-879`). Token never read, echoed, logged, or on argv.
5. ⚠️ **The `.uasset` files appeared git-staged (`A `) immediately after the editor saved them** —
   the editor's revision-control integration, not me. **I ran no git commands (Git is fenced).**
   Same observation `TASK-834` §5.5 made; build-master should expect the working tree pre-touched.

---

## 7. Not touched (fence)

⛔ No `Content/FogArea/**` — the vendor pack was never opened, edited, or saved, and no save prompt
was answered. ⛔ No mesh, no material, no `SM_`. ⛔ No `cards.csv`, no `DT_Cards` — the `Fog` row
(`TASK-840`) and the `BrightSun` row (`TASK-983`) are theirs. ⛔ No C++. ⛔ No fog visual
(`TASK-841`). ⛔ **No Git.** ⛔ `L_Arena` never opened; only the two textures were saved, by explicit
path — never an empty list. ⛔ No other card's art touched.
