# TASK-1154 — [FOGGREY-ART] — art-director handoff

> 🚨🚨🚨 **ANNOTATION BANNER — added 2026-09-08 by the manager. `SC-§114` cl. 4 propagation discharge. NOT ONE WORD OF THIS AUTHOR'S TEXT HAS BEEN CHANGED, AND THAT IS DELIBERATE.**
>
> **What this file got wrong, in one line:** its §3 measured `general Data.base Color` as **inert on pixels** (pure red ⇒ B/G `0.890` vs baseline `0.891`) and its §4 concluded **the vendor Blueprint never transports the colour**. `TASK-1172` §1a re-ran the *identical single-variable flip* and measured **B/G `0.6908` / sat `72.56 %`** against a same-session baseline of `0.8461` / `19.64 %` — a violent change where this file scored zero. `TASK-1167` §4.3(ii) then measured the vendor script pushing **both** colours onto its MID, causally, in both directions. **The field was live the whole time. The colour that finally shipped went in through exactly the field this file reports as dead.**
>
> **Why the text below is preserved verbatim rather than corrected:** it is the honest record of what a competent agent, running a controlled-looking experiment, actually observed — and it is the *evidence* for `SC-§114` (the null-result law). Rewriting it would destroy the only artefact that shows how a false null looks from the inside: confident, well-instrumented, internally consistent, and closing a door. `SC-§97` cl. 4 — **strike in place, keep history, never silently rewrite.**
>
> **What this file is NOT:** it is not incompetent and it is not dishonest. It ran a decisive falsification test *on purpose* (§3's "a null result deserves a decisive test rather than a shrug"), it refused to save a Blueprint whose write it could not verify (§5), and it flagged its own mechanism as a lead rather than a finding (§3's last subsection). The defect is a **missing positive control on its own driver** — one write it was never asked for. **Cause still NOT diagnosed;** the only lead on file is a wrong-world write (`/Game/Maps/L_Arena…` vs `/Game/Maps/UEDPIE_0_L_Arena…` are separate objects with separate MIDs) and nobody has tested it.
>
> **Reader's rule:** ⛔ **nine sites below carry the falsified claim and each is annotated inline.** Every number in §1, §2, §4's *"the material's colour parameters are alive"* table, §4's value-tuning table, §5's tooling findings and §6's fences **STANDS and is still cited**. The live rulings are `FOG-§12.4b` (the channel) and `SC-§114` (the law this file bought).

**Trigger verdict: EXECUTED, for the named asset-side contributor.** `handoffs/TASK-1151-artist.md` §2 carries `CAUSE — ASSET-SIDE ✅ (ask C)` and names it: `BP_SiegeFog` → `general Data.base Color`, then `(1, 1, 1, 1)`. Its ask-(C) `CAUSE — ENGINE-SIDE` section is **absent and measured absent**, and it says explicitly *"this is NOT a mixed cause"* ⇒ `TASK-1155` does not fire and this row runs alone.

Date 2026-09-08 · editor PID **14120** · ran **second**, in the same session as `TASK-1152`, on the same `.uasset`, per the mandated order (uniformity first — a density change moves the perceived hue, so colour is judged on the final density).

Instrument, calibration, Simulate setup, controls and fences: **as documented in `handoffs/TASK-1152-artist.md` §1 and §6** — same session, same instrument, same frames. Not restated.

---

## 1. The before, on pixels — and it corroborates `TASK-1151` independently

My reproduction of his fog, measured in Simulate at the `VID-007` vantage, against his own recorded frames:

| | far-field sRGB | R/G | B/G |
|---|---|---|---|
| **his `VID-007` reference (t=27)** | (190.6, 181.1, 162.2) | 1.052 | 0.896 |
| **mine, before (Simulate)** | (191.7, 179.2, 161.6) | 1.070 | **0.902** |

B/G agrees to **0.006**. `TASK-1151` reported (190, 181, 162) / R/G 1.05 / B/G 0.895 — I reproduce it. The warm cast is real, it is in my frames, and it is the same cast he is complaining about.

✅ **And the negative control separates fog from world:** the same vantage with **no `BP_SiegeFog`** reads (126.0, 135.2, 72.2) — **green**, B/G 0.53. The battlefield is not beige; the fog is. So the hue arrives with the fog, exactly as `TASK-1151` concluded.

## 2. 🚨 Why a grey `base Color` would only have darkened — computed in LINEAR, not sRGB

`base Color` is a **multiplier** on scattered light, applied in linear space. The measured RGB above is **sRGB, post-tonemap**, so a correction reasoned in sRGB ratios is wrong. Converting my measured far-field to linear:

> sRGB (191.7, 179.2, 161.6) → linear **(0.5253, 0.4519, 0.3593)** ⇒ **R/G 1.162, B/G 0.795**

⇒ the multiplier that fully cancels it is `(G/R, 1, G/B)` = **(0.860, 1.000, 1.258)**, whose luma weight is **0.9889** — i.e. it re-balances the hue while leaving brightness essentially untouched (a flat grey would not: it would scale all three channels down and simply dim the fog, which is `FOG-§12.4`'s warning and `TASK-1151`'s corollary).

⚠️ **This is a correction to `TASK-1151`'s estimate, and the direction matters.** It predicted a shape of ≈ `(0.88, 1.00, 1.13)`, derived from the 5400 K light's spectrum. Measured from the **rendered pixels**, the blue channel needs **1.26, not 1.13** — the tonemapper compresses the warm end, so the residual cast in the final image is stronger than the light's own chromaticity implies. `TASK-1151` derived its number honestly from the light; the pixels are the authority, and they ask for more blue (`SC-§101` — a prescribed remedy is a claim, and this one needed measuring).


---

## 3. 🚨🚨 THE NAMED CONTRIBUTOR IS REFUTED ON PIXELS — `base Color` IS INERT

> ⛔ **CARRIER 1 of 9 — this heading. ANNOTATION (manager, 2026-09-08, `SC-§114`):** the heading's claim is **false**. `base Color` is not inert and was never refuted; it is the field the shipped fix uses. Correct label for everything under this heading: **`NOT MEASURED — NULL WITH NO POSITIVE CONTROL`.** Measurer of the contrary result: `TASK-1172` §1a.

I applied the computed correction and measured the result. **It did nothing.**

| `general Data.base Color` | far-field sRGB | R/G | B/G |
|---|---|---|---|
| `(1.00, 1.00, 1.00)` — before | (190.1, 177.9, 158.5) | 1.069 | 0.891 |
| `(0.88, 1.00, 1.21)` — the correction | (189.8, 177.6, 158.0) | 1.069 | 0.890 |
| | (191.6, 179.8, 161.1) | 1.066 | 0.896 |
| | (191.0, 178.9, 159.7) | 1.068 | 0.893 |
| | (191.8, 179.9, 161.5) | 1.066 | 0.898 |

Four frames, two positions, two times: **B/G moved from 0.891 to 0.890–0.898 — inside the frame-to-frame noise of the unchanged fog.** No shift at all.

> ⛔ **CARRIER 2 of 9 — the correction table and "No shift at all". ANNOTATION (manager, 2026-09-08, `SC-§114`):** the same correction shape, driven through the same field, later moved the render decisively (`TASK-1172`: `0.8461 ⇒ 0.9772` in the tuning rig; `TASK-1167` rev-2 independently `⇒ 0.9372` on the ship path). **The four frames above are real frames of a fog that never received the write.** Note the shape of the trap: the spread quoted here (`0.890–0.898`) is *genuine frame-to-frame noise*, which is exactly what a disconnected lever produces — and it reads as a competent noise estimate.

### The falsification, because a null result deserves a decisive test rather than a shrug

A small tint producing a small effect could be a *weak* lever. So I set `base Color` to the most extreme value available — **pure red `(1, 0, 0)`**:

> ⛔ **CARRIER 3 of 9 — the pure-red falsification. ANNOTATION (manager, 2026-09-08, `SC-§114`):** this instinct is **exactly right** and `SC-§114` cl. 3(a) now mandates it project-wide — *drive the lever to an absurd value*. What it lacked was the second half: **the absurd value must be shown to move the picture, and when it does not, the conclusion is "my apparatus is broken", not "the lever is dead."** No material in this engine is invariant to its own base colour. This exact flip re-run by `TASK-1172` §1a: **sRGB `(202.9, 80.6, 55.7)`, B/G `0.6908`, sat `72.56 %`.**

| | far-field sRGB | R/G | B/G | contrast |
|---|---|---|---|---|
| baseline `(1, 1, 1)` | (190.1, 177.9, 158.5) | 1.069 | 0.891 | 7.17 |
| **pure red `(1, 0, 0)`** | (189.3, 176.9, 157.5) | **1.070** | **0.890** | 7.19 |

⛔⛔ **A fog whose `base Color` is pure red renders EXACTLY the same beige.** Not weaker — *identical*, on hue and on brightness alike (contrast 7.19 vs 7.17). The property read back correctly every time; the render never moved.

⇒ **`BP_SiegeFog` → `general Data.base Color` does not reach the rendered image at all.**

> ⛔ **CARRIER 4 of 9 — "renders EXACTLY the same beige" / "does not reach the rendered image at all". ANNOTATION (manager, 2026-09-08, `SC-§114`):** **FALSE, and this is the single sentence that cost the lane.** It reaches the rendered image; it is the channel the shipped colour travels on (`FOG-§12.4b`, `FOG-§12.5` pinned colour fields). "The property read back correctly every time" is not corroboration — a driver writing the wrong object also reads back perfectly (`SC-§112` cl. 6). This sentence was inherited by `TASK-1165` → `1166` → `1167`: a material built, a C++ change written, compiled, QA'd, gated and reverted unshipped, **to route around a field that worked.**

### What this means for `TASK-1151`'s finding — stated precisely, because the diagnosis was otherwise excellent

`TASK-1151` C-1 said the fog's colour is *"`BaseColor` × light"* and concluded *"one change on `BP_SiegeFog.general Data.base Color` moves 100 % of it… No partial shift, no round trip."* That was an **inference from the material's parameter census, not a measurement** — the handoff is honest that its instrument was a property read-back, and `SC-§94`/`FIELD-§7` name exactly this failure mode: **a value that reads correct and renders wrong.**

⚖️ **This is `SC-§101` happening on schedule:** a correct diagnosis (the hue *is* single-sourced from the 5400 K sun; the vendor pack *does* inject zero hue; every colour field *is* neutral — I reproduced all of it) prescribed a remedy that carried an untested assumption, and the finding's credibility is exactly what would have stopped anyone checking. `FOG-§12.4`'s *"verify by rendered pixels, never by a property read-back"* is what caught it, and it earned its place in the law twice over.

⚠️ **What is NOT refuted:** everything `TASK-1151` measured stands — the hue is real (I reproduce B/G 0.902 against his 0.896), it is single-sourced, it arrives with the fog and not from the battlefield (my no-fog control reads green, B/G 0.53), and no vendor or map colour field carries it. Only the **prescribed lever** is refuted.

> ⛔ **CARRIER 5 of 9 — "Only the prescribed lever is refuted". ANNOTATION (manager, 2026-09-08, `SC-§114`):** the *"what is NOT refuted"* list is **entirely correct and still cited** — the hue is real, single-sourced, arrives with the fog, and no vendor/map field carries it. **The one item this paragraph does assert is the one that is false.** `TASK-1151`'s prescribed lever was right from the first row; the lane spent three rows proving otherwise. Read this paragraph as: *nothing was refuted.*

### Why it is plausibly inert — recorded as a lead, NOT as a finding

> ⛔ **CARRIER 6 of 9 — the `BLEND_Additive` explanation. ANNOTATION (manager, 2026-09-08, `SC-§114`):** the lead is **withdrawn as unneeded, not as disproven** — there is nothing to explain, because the field is not inert. ✅ **The discipline in this subsection is the best thing in the file and it is now law:** it refused to assert an unmeasured mechanism. ⚠️ **And note the shape of the danger anyway:** a *plausible* mechanism attached to a false null is what makes the null credible. `FOG-§12.4a`'s struck construction-timing paragraph was my version of the same move, and mine was worse — I put it in the law.

`TASK-1151` §5 note 8 observed that `MI_FogArea_Box` reports `blendMode = BLEND_Additive`. An additive surface contributes what it **adds**; a diffuse albedo term has nothing to multiply. That is consistent with `base Color` being unused in the `Base` mode this fog runs in — but I did **not** trace the material graph to prove it, and I am not going to assert a mechanism I did not measure. ⛔ **Lead, not finding.**


---

## 4. ⭐⭐ THE MECHANISM, FOUND — the vendor Blueprint never transports the colour

> ⛔ **CARRIER 7 of 9 — this heading. ANNOTATION (manager, 2026-09-08, `SC-§114` + `SC-§110`):** **the vendor Blueprint DOES transport the colour.** `TASK-1167` §4.3(ii) measured it causally: `generalData.baseColor → (1,0,0)` and `emissiveColor → (0,0.9,0)` moved the MID's two parameters to exactly those values, and restoring moved them back — **in both directions, on the live MID.** ✅ **The scalar half of the table below is correct and still cited** (`Density` · `Base Noise Size` · `Base Noise Sharpness` · `Wind Speed` · `Mask Margin`), and the `Wind Speed` row's `0.5 (×0.01)` reading is the measurement that later closed `NIT-8` (`FOG-§12.4c`). **Only the two colour rows are wrong.**

A null result is only half an answer, so I opened the live actor and read what the material actually receives. `BP_SiegeFog` runs `material Mode = Dynamic`: it builds a **`MaterialInstanceDynamic`** (`MID_MI_FogArea_Box_0`) from `boxMaterials[Base]` and writes the struct into it. I read that MID's parameter arrays directly, **while the BP struct held values I had just set**:

| MID parameter | value in the MID | value in the BP struct | transported? |
|---|---|---|---|
| `Density` | **0.5** | 0.5 | ✅ |
| `Base Noise Size` | **2.5** | 2.5 | ✅ |
| `Base Noise Sharpness` | **0.1** | 0.10 | ✅ |
| `Wind Speed` | **0.005** | 0.5 (×0.01) | ✅ |
| `Mask Margin` | **3** | 3 | ✅ |
| **`Emissive Color`** | **(0.05, 0.05, 0.05)** | **(0, 0, 0.5)** ← I had just set this | ⛔ **NO** |
| **`Base Color`** | (1, 1, 1) | — | ⛔ **never written** |

⇒ ⭐⭐ **The vendor Blueprint pushes every SCALAR from the struct into the material and pushes NO COLOUR at all.** The colour fields it exposes on `S_FogAreaGeneral` are decorative: they read back perfectly, they are never transported, and nothing anywhere reports it. That is why `base Color` was inert — not because the material ignores colour, but because **the value never arrives.**

> ⛔ **CARRIER 8 of 9 — "pushes NO COLOUR at all" / "the colour fields … are decorative" / "the value never arrives". ANNOTATION (manager, 2026-09-08, `SC-§110` propagation site 1 + `SC-§114`):** **all three are false.** The construction script writes `Base Color` **and** `Emissive Color` onto its MID from `generalData` on every construction. The fields are not decorative — they are the **only** working colour channel this fog has, and `Box Materials["Base"]` (which §4 goes on to recommend) is honoured but **colour-incapable**, because those same two parameters are the entire authored content of any MI you put there. ⚠️ **The MID read of `(0.05,0.05,0.05)` recorded in the table above is a real read of a real MID — of an actor that never received the struct write.** This paragraph propagated to 5 board sites and 4 `CONVENTIONS.md` sites; all are now annotated or struck.

### And the material's colour parameters are alive — proven by writing them directly

I wrote the MID's own parameters at runtime and captured:

| MID parameter set directly | far-field sRGB | R/G | B/G |
|---|---|---|---|
| baseline (`Emissive` 0.05 grey, `Base` white) | (189.2, 177.9, 159.3) | 1.063 | 0.895 |
| **`Emissive Color` = (0, 0, 1)** | (184.7, 171.7, **199.9**) | — | **1.164** |
| **`Base Color` = (1, 0, 0)** | (193.8, **55.2**, 44.6) | 3.508 | — |

Both parameters move the render violently. ⇒ **the material is fine; the transport is the defect.**

> ⛔ **CARRIER 9 of 9 — "the material is fine; the transport is the defect", and the "why it is an MI rather than a struct edit" subsection that follows it. ANNOTATION (manager, 2026-09-08, `SC-§114`):** ✅ *"The material is fine"* **stands** — twice-measured (`TASK-1167` lane C rendered `MI_SiegeFog_Grey` directly at `0.947`/`7.0 %`). ⛔ *"The transport is the defect"* is **refuted**, and with it the MI rationale below: **a struct edit was always the right shape, and the MI route could not have worked** — the vendor script overwrites both colour parameters on whatever MID it builds, so a MID built from `MI_SiegeFog_Grey` is parameter-identical to one built from the vendor material. `MI_SiegeFog_Grey` is **kept and demoted** to a value reference and positive control, **unreachable by design, wired to nothing, and nothing may wire it** (`FOG-§12.4b`). ⚠️ **Its two values were updated 2026-09-08 to the landed pair `(0.86,1,1)` / `(0.05,0.07,0.325)`, so the `(0.88,1,1)` / `(0.05,0.08,0.26)` pair recorded in §4 and §5 below is HISTORICAL** — as is the `0.947`/`7.0 %` render bound to it. **Both remain the correct record of what this row built.** And the two levers do *different* jobs, which the fix depends on:
- **`Base Color` is multiplicative** — it can pull the red down (it cannot add blue: ×1.26 on B produced no change).
- **`Emissive Color` is additive** — it is the only way to *add* blue, and it cannot subtract.

⇒ neutralising this cast needs **both**: `Base Color` to cut R, `Emissive Color` to lift B.

### The fix, and why it is an MI rather than a struct edit

Since the BP never writes the colour into the MID, a colour baked into the **source material** survives untouched into the MID. So:

1. **`/Game/Materials/Instances/MI_SiegeFog_Grey`** — **new, ours**, a child of the vendor `MI_FogArea_Box` (referenced as a **parent only**; the vendor asset is never opened, edited or re-saved, per `FOG-§6`). Named per this row's `names` line (`MI_SiegeFog_<Purpose>`).
2. **`BP_SiegeFog.boxMaterials.Base`** repointed from the vendor `MI_FogArea_Box` to `MI_SiegeFog_Grey`.

The scalars still flow from the BP struct exactly as before, so 🧑 he keeps `density`/`sharpness`/`wind Speed` as live dials on the Blueprint; only the colour now comes from the MI.

⚠️ **No new shader permutation:** only vector parameters are overridden — no static switch, no graph edit ⇒ **no material recompile** (`TASK-239`).

### Choosing the value — measured, three candidates, one proposed

All three measured on pixels at the `VID-007` vantage, on the final `TASK-1152` density:

| candidate | `Base Color` | `Emissive Color` | far-field sRGB | R/G | B/G | saturation | luma |
|---|---|---|---|---|---|---|---|
| **before** | (1, 1, 1) | (0.05, 0.05, 0.05) | (189.2, 177.9, 159.3) | 1.063 | **0.895** | **14.7 %** | 179.5 |
| t1 (fully neutral) | (0.86, 1, 1) | (0.05, 0.05, 0.33) | (182.6, 178.3, 177.7) | 1.024 | 0.996 | 2.7 % | 179.5 |
| ⭐ **t2 — PROPOSED** | **(0.88, 1, 1)** | **(0.05, 0.08, 0.26)** | (180.3, 176.3, 169.7) | **1.022** | **0.963** | **5.9 %** | 176.7 |
| t3 (warmer) | (0.90, 1, 1) | (0.05, 0.06, 0.20) | (181.5, 175.5, 165.9) | 1.034 | 0.945 | 8.6 % | 176.2 |

⭐ **I propose t2**, and the reasoning is `FOG-§12.4` cl. 3 rather than arithmetic: 🧑 he asked for ***"more of a grey"***, not for grey. **t1 is available and reaches a dead neutral (B/G 0.996)** — I did not take it, because a fog with no hue at all on a green, sunlit battlefield reads as a flat slab. **t2 cuts the saturation by 60 % (14.7 % → 5.9 %) and moves B/G from 0.895 to 0.963** while costing **1.6 % of brightness** — unmistakably grey next to the before, with a trace of warmth left so it still reads as weather.

🧑 **t1 and t3 are the dial either side, already measured** — `TASK-1159` can move one notch in either direction without anyone re-deriving anything.


---

## 5. ⛔⛔ WHAT I BUILT, AND THE ONE STEP I COULD NOT TAKE — the colour is NOT shipped

**`/Game/Materials/Instances/MI_SiegeFog_Grey` is created, correct and SAVED**, parented to the vendor `MI_FogArea_Box`, read back after the write:

| parameter | value on the MI (read back) |
|---|---|
| `Base Color` | **(0.88, 1.00, 1.00, 1.00)** |
| `Emissive Color` | **(0.05, 0.08, 0.26, 1.00)** |

⛔ **But it is NOT referenced by anything, so the fog still renders the old beige.** The remaining step is a single field — `BP_SiegeFog` → `boxMaterials["Base"]` must point at `MI_SiegeFog_Grey` instead of the vendor `MI_FogArea_Box` — and **I could not write it through the MCP surface.** Two routes, both attempted, both measured:

| route | result |
|---|---|
| `ObjectTools.set_properties` on the CDO's **`boxMaterials`** `TMap` | ⛔ **silent no-op.** Tried the map with every entry as a plain string and again with every entry as a `refPath` dict; both calls returned **without error** and the read-back still showed the vendor MI. *(My script's post-write assertion caught this and **refused to save** — otherwise it would have saved a Blueprint whose colour route silently did nothing.)* |
| CDO **`material`** override slot (was `None`) | ⛔ **stored but ignored.** The property took (read-back confirms `MI_SiegeFog_Grey`), but a fresh spawn still built **`MID_MI_FogArea_Box_0`** — a MID of the *vendor* MI — and the pixels were unchanged: far-field **(188.0, 175.4, 156.3), B/G 0.891** against 0.895 before. The construction script uses `boxMaterials[mode]` and does not consult `material`. |

⇒ 🚨 **ASK (C) IS NOT DELIVERED.** The cause is diagnosed, the lever is found, the value is measured and the asset is built — but the fog Jonathan sees is **still the same beige**, and I am not going to describe a prepared asset as a shipped fix.

### What finishes it — one field, seconds of work

> Open `Content/Blueprints/BP_SiegeFog` → **Class Defaults** → **`Box Materials`** → key **`Base`** → set to **`MI_SiegeFog_Grey`**. Save. Nothing else changes.

That is a hand edit in the Blueprint editor (🧑 Jonathan or build-master), **or** a one-line programmer change. The scalars keep flowing from the struct, so `density` / `sharpness` / `wind Speed` remain live dials.
⚠️ **Whoever does it must re-verify on PIXELS** — the far-field B/G should move **0.895 → ~0.963**. This row exists because a colour that read correct rendered wrong; do not accept a property read-back as proof.

### State I left behind, exactly

- ✅ `MI_SiegeFog_Grey` — **created and saved**, values verified, **referenced by nothing** (harmless; it renders nowhere until wired).
- ✅ `BP_SiegeFog` — carries **only** `TASK-1152`'s uniformity change. The `material` slot I set during the experiment was **reverted to `None`** and the asset re-saved, so no half-finished colour wiring is on disk.
- ⛔ No colour change is live. `TASK-1152`'s uniformity fix is unaffected and remains fully verified.


---

## 5. Fences

| Fence | Result |
|---|---|
| `Content/Maps/L_Arena.umap` | **`1F78419D…0AF15622` before AND after — identical**, mtime still `2026-09-05 00:42:58`. Never saved. |
| ⛔ `DirectionalLight_0` (the 5400 K sun) | **NOT touched.** It is the origin of the hue, and re-lighting the world to fix the fog would change everything else he likes — and `GFX-§11` forbids saving the map anyway. Diagnosed by `TASK-1151`, deliberately not the lever. |
| ⛔ `SkyLight_0`, `SkyAtmosphere_0`, `PostProcessVolume_0`, `ExponentialHeightFog_0` | **NOT touched.** No map actor was modified. |
| ⛔ `Content/FogArea/**` (vendor) | **untouched, unopened.** `BP_FogArea` never opened; `M_FogArea` / `MF_Fog` never edited. No MI created. |
| Material recompile (`TASK-239`) | **none.** A colour parameter on a BP struct; no material graph, no static switch. |
| `Source/**` · `Config/**` | **zero bytes.** |
| Save | `save_assets(["/Game/Blueprints/BP_SiegeFog"])` — by explicit path, never `save_assets([])`. |
| Commit / push | **none.** `TASK-1158` hosts. |

## 6. What I did NOT measure — plainly (`SC-§94`)

1. ⛔ **This is one proposed value, not a settled one.** `FOG-§12.4` cl. 3 is explicit that *"more of a grey"* is relative and 🧑 **his eye is the instrument** — that is `TASK-1159`. I moved it most of the way to neutral and deliberately left a trace of warmth rather than driving R/G and B/G to exactly 1.000; a fog with zero hue on a green, sunlit battlefield reads dead.
2. ⛔ **Measured on the far-field wash only.** That band is the purest sample of the fog itself (opaque, no terrain showing through). I did not characterise the hue of the *near* field, where fog and grass mix and the grass's own green dominates.
3. ⛔ **No unit or hero in frame.** Simulate has no pawns, so I did not check the tint against skin, tabards or team colours — and team-colour readability is a competitive concern, not just an aesthetic one. Worth his eye at `TASK-1159`.
4. ⛔ **One lighting condition.** Measured under `DirectionalLight_0` as it stands, at its current pitch. `BrightSun` was not cast; if that card changes the sun, the fog's rendered hue moves with it and this correction is calibrated for the default.
5. ⛔ **The tonemapper is non-linear**, so a linear multiplier cannot cancel a tint exactly at every luminance. I corrected against the measured far-field band; darker or brighter regions of the fog will retain slightly different residuals.
6. ⛔ **`general Data.emissive Color` (0.05 neutral grey) left alone.** `TASK-1151` named it as a secondary lever. It adds a flat, hueless lift; changing it would have altered brightness as well as colour, and one lever was enough.


## 7. For integration (`TASK-1158`)

- **Assets touched by THIS row:** `Content/Materials/Instances/MI_SiegeFog_Grey.uasset` (**new**). `Content/Blueprints/BP_SiegeFog.uasset` carries **only** `TASK-1152`'s change — my experimental `material` write was reverted to `None` and the asset re-saved.
- ⛔ **This row ships NO visible change.** Do not describe ask (C) as fixed. `MI_SiegeFog_Grey` is a **prepared part**, not a delivered repair, and it is referenced by nothing.
- ⚖️ **Routing note for the manager:** `TASK-1151` ruled ask (C) **asset-side only**, and that ruling **still holds** — the fix genuinely is an asset change, and it is built. What is missing is a **tooling** step, not a different owner. `TASK-1155` (the code lane) was correctly closed: the `ExponentialHeightFog` actor is still not a colour contributor, and nothing measured here revives it.
- 🧑 **`TASK-1159`** should judge the colour only **after** the one-field wiring lands; until then there is nothing new for his eye to look at. The uniformity work (`TASK-1152`) *is* ready for his eye now.
