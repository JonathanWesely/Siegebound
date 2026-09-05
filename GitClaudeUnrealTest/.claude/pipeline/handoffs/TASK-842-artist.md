# TASK-842 — `T_CardArt_Fog` — art-director handoff

**Status: ready-for-integration.**

⭐ **The full record is `handoffs/TASK-984-artist.md`.** `Fog` and `BrightSun` are anti-cards and were
made in **one batch, in one Blender scene, sharing one field and one camera** — which is what
`TASK-984`'s hazard clause asks for. Splitting the record would have split the pair measurement that
is the point of it. This file carries the Fog-specific facts; everything else is cited, not restated.

---

## Deliverables — proven by read-back, not by the import call

| what | path |
|---|---|
| Card texture | **`/Game/UI/CardArt/T_CardArt_Fog`** (`Content/UI/CardArt/T_CardArt_Fog.uasset`) |
| Source render | `Content/RawAssets/CardArt/Fog.png` |

- `exists("/Game/UI/CardArt/T_CardArt_Fog")` → **true** (it returned **false** before this task, so
  this is a **clean first import** — no delete+re-import, the `import_file` overwrite trap never arose).
- `get_size` → **512 × 512** · `SRGB` **true** · `LODGroup` **TEXTUREGROUP_UI** ·
  `CompressionSettings` **TC_Default** · `MipGenSettings` **TMGS_FromTextureGroup** — identical to
  shipped `T_CardArt_Witch`, read back as a control.
- `load_asset("/Game/UI/CardArt/T_CardArt_Fog.T_CardArt_Fog")` resolves; `get_asset_class` = `Texture2D`.
- sha256 `.uasset` **`513322e7daa7d94fe828a75c45faae3cd9315f675d49ebc145764877e79e2128`** (231,798 B)
- sha256 source PNG **`62467573e3a5678712d6fe6417df1da2d933addfa0367e6be09e13e5e2d04b62`**
- ⚠️ `get_referencers` is **empty, and correctly so** — `TASK-840` has not yet written the `Fog` row.

---

## Key colour — the spec's warning was right, and the measurement inverted the obvious answer

The row warned that a grey/white fog key is *"exactly the kind of assumption that failed on the
green-teal arc"*. **Measured: it is worse than assumed.** The near-neutral band **at low value** is
the most crowded region on the wheel and owns the roster **minimum** (Ogre↔PlateArmor **4.07**) —
Sapper C\* 1.6, Wall 2.9, Ogre 4.6, BombTower 8.4, PlateArmor 9.1 all sit in it.

⭐ **But the crowding is a LOW-VALUE phenomenon, and no shipped key is a pale desaturated one.** Every
roster neutral is dark (val 0.225–0.361). Sweeping the cold low-saturation region at *high* value
returns the freest space on the board. ⇒ **The fog card gets to be literally the colour of fog** —
not despite the measurement but because of it, once taken on the right axis.

**Shipped key, re-measured on the shipped bytes:** `rgb8` **(139, 156, 168)**, hsv (204.8, 0.173,
0.659), L\* 63.4, C\* 9.0.

- **min-ΔE2000 to all 32 shipped = 19.41 → 100th percentile** (roster NN median 8.23, **max 17.75**).
  It exceeds every shipped card's own nearest-neighbour distance. Nearest: Cleric 19.41, Footman 19.67.
- Backdrop-mean Lab chroma **9.10** — inside the roster range (1.59–50.64), p16.
- ⚠️ Hue separation to the nearest shipped hue is **1.8°**, and that is **not** the hazard it looks
  like: at C\* 9.0 hue is perceptually inert and the separation is carried by **lightness** (L\* 63.4
  against roster neutrals at L\* 24–45). Confirmed by eye at 150 px.

**Instrument control** (reproduced before use): min **4.07 Ogre↔PlateArmor**, p10 **4.13**, max
**12.53 Pickpocket↔Archer**, **5 of 29** clear ΔE 10, Sorcerer key **(29,79,69)** and Sorcerer↔Archer
**10.14** — all exact against the law. Sampling is **64 px top-two-corner medians**.

**Pair separation vs `T_CardArt_BrightSun`: ΔE2000 32.41, hue 149.2°** — 3.9× the roster median
neighbour distance, 1.8× its maximum. See `TASK-984-artist.md` §2.

---

## Acceptance — frame-wide re-derivations (the law's figure-mask numbers do not apply)

The law's recorded 0.1786 / 0.230% were measured on a **mesh figure mask**; a spell card has no
`SM_<CardID>` and this face is atmospheric, so there is no figure to mask. Frame-wide equivalents
were re-derived over all 32 shipped faces. Full table in `TASK-984-artist.md` §3.

| metric | Fog | corpus (n=32) | verdict |
|---|---|---|---|
| frame median | 0.699 | 0.239 … **0.690** | ⚠️ **OUTSIDE by 0.009** — declared |
| clip ≥ 0.996 | **0.687%** | 0.000 … 10.963% | INSIDE, p78 |
| p99.5 | 0.997 | 0.561 … 1.000 | INSIDE, p78 |
| contrast | 0.474 | 0.176 … 0.840 | INSIDE, p81 |
| backdrop chroma | 9.10 | 1.59 … 50.64 | INSIDE, p16 |

⚠️ **The one excursion, declared rather than dressed up:** frame median 0.699 against a corpus max of
0.690. It is the direct, unavoidable consequence of the pale key that earned the 100th percentile —
this is the brightest card on the board because it is the only pale-keyed one. Same fact, twice.

👁️ **Rendered and looked at.** The clip map caught a real defect: the halo first clipped to a **hard
white hole covering 11.09% of frame** — *above* the corpus max — and read as a sticker. Softened to
**0.687%**; it now reads as a sun diffused through fog. A numeric pass was never acceptance on its own.

---

## The face

A castle tower, a crenellated wall, four pikes with a banner, and two further towers **drowning in a
lit fog bank**, with a soft veiled light behind the castle. Depth is erased with distance — the near
wall is veiled, the mid tower is a ghost, and **the horizon ridge is gone entirely**. That erasure is
the mechanic drawn: it is the same field `BrightSun` reveals.

CYCLES (never EEVEE) · 512×512 · Standard/None · 65 mm / 36 mm · 640 samples · matte backdrop
(TRAP 2) · no baked-in text · team-agnostic · full-bleed opaque.

📌 **For the overlay:** this is the **palest card on the board**. White DisplayName/cost text will
need its contrast strip or shadow — permitted by the face-composition law, but worth knowing up front.

⚠️ **Deviation declared for Jonathan's eye:** the 32 shipped faces are one object on a plain backdrop;
this pair are landscapes, because a map-wide spell has no single object to be. Overrulable with a
re-render of the same script. See `TASK-984-artist.md` §5.

---

## Not touched (fence)

⛔ `Content/FogArea/**` never opened, edited or saved (vendor) · ⛔ no `cards.csv` / `DT_Cards` (the
`Fog` row is `TASK-840`) · ⛔ no fog visual (`TASK-841`) · ⛔ no C++ · ⛔ **no Git** · ⛔ `L_Arena`
never opened; only the texture was saved, by explicit path.
