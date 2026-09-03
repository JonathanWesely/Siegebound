# TASK-832 — [W-5] `MI_Unit_Invisible` — REFRACTION, ⛔ NOT ALPHA — art-director handoff

**Status:** `ready-for-integration`
**Law:** `WITCH-§5` · `WITCH-§6` (the veil-material row) · `SC-§39.1` (a success return is not evidence)
**Suite delta: ⛔ ZERO.** Pure asset authoring. ⛔ No C++, ⛔ no compile, ⛔ no Git, ⛔ no PIE, ⛔ no `DT_Cards`, ⛔ no mesh, ⛔ no re-bake, ⛔ no rig, ⛔ no `.umap` written.

---

## ⭐ THE HEADLINE: REFRACTION WAS ACHIEVABLE — BUT ⛔ NOT BY THE ROUTE THE SPEC ASSUMED, AND THE FIRST ROUTE FAILED ⛔ SILENTLY

✅ **`/Game/Materials/MI_Unit_Invisible` ships.** A veiled unit is **much lighter than base colour, see-through, and the arena behind it is visibly torn and smeared** — measured at **13× the background noise floor**, not judged by eye.

⛔ **`WITCH-§5`'s escape clause FIRED and I am saying so, as instructed.** Measured in-editor before any edit:

| what | measured | means |
|---|---|---|
| `M_HeroSpirit` `MP_Refraction` | **disconnected** (`get_property_input` → `expression: None`) | ⛔ **the master carried NO refraction input** |
| `M_HeroSpirit.RefractionMethod` | **`RM_None`** | ⛔ refraction was **switched off at the material level too** |
| `RefractionMethod` in `FMaterialInstanceBasePropertyOverrides` | ⛔ **ABSENT** (read off `MI_Ghost_Translucent`'s override struct — 15 override flags, ⛔ no refraction among them) | ⛔⛔ **refraction can ⛔ NOT be enabled per-instance. The master ⛔ must be edited — there is no instance-only path.** ⭐ This is the fact that forces the amendment; it is ⛔ not a preference |

⇒ **I extended the master, as `WITCH-§5` directs** (*"the artist ⛔ SAYS SO and the master is ⛔ EXTENDED"*) — ⛔ and I did ⛔ not downgrade to alpha. 🧑 **This is the manager amendment the board's item (4) anticipated. It is one word either way; the extension is on disk and provably inert for every pre-existing instance (§4).**

---

## 1. THE SHIPPED ASSET

**`/Game/Materials/MI_Unit_Invisible`** — file `Content/Materials/MI_Unit_Invisible.uasset`
Read back **independently after saving**, ⛔ not trusted from the save's `true`:
`class MaterialInstanceConstant` · **dependencies = exactly `[/Game/Materials/M_HeroSpirit]`** · **referencers = `[]`**.

⭐ The board's pinned name is preserved character-for-character and the parent is the one `WITCH-§5` mandates. ⛔ `M_Ghost` was never touched, ⛔ `MI_Ghost_Translucent` was never re-parented or reused.

| param | value | why |
|---|---|---|
| `VeilRefractionStrength` | **0.25** | ⭐ **the veil's whole point.** IOR swings 0.75–1.25 across the body ⇒ the ground/castle behind is bent and smeared |
| `VeilWobbleScale` | **14.0** | ripple frequency in viewport UV — tuned so ~1 cycle crosses a unit at play distance; higher = shattered, lower = a single soft lens |
| `VeilWobbleSpeed` | **0.6** | slow crawl. ⭐ **Motion is what the eye catches when a unit is 60 px tall** |
| `CoreOpacity` | **0.14** | see-through enough that the distortion is the read, ⛔ not the body |
| `CoreWhiten` | **0.70** | ⛔ *"much lighter than base colour"* — his words |
| `RimPower` / `RimBoost` | 3.5 / **1.2** | ⛔ **deliberately far below the ghost's 6.0** — a veiled unit is ⛔ hiding, ⛔ not glowing. The rim exists only to keep the silhouette legible |
| `RimOpacityBoost` | 0.50 | firms the outline so it reads as a **person**, ⛔ not a smudge |
| `PulseSpeed` / `PulseAmount` | **0.0 / 0.0** | ⛔ **the breathing pulse is the GHOST's signature and is switched OFF here.** Sharing a master must ⛔ not mean sharing a look |
| `TeamColor` | **(0.78, 0.76, 0.86)** pale silver-lilac | ⛔ **not** the ghost's cyan `(0.45,0.78,1.00)`, ⛔ not green/red (the placement-ghost valid/invalid states). ⭐ **This param is the per-team lever if integration ever wants one** — a MID + `SetVectorParameterValue`, the shipped `ACaptureZone` idiom |

---

## 2. ⛔⛔ THE TRAP THAT COST THE MOST, RECORDED SO NOBODY PAYS IT TWICE: **`RM_2DOffset` COMPILES, SAVES, READS BACK CORRECTLY — AND REFRACTS ⛔ NOTHING**

I built the first version on **`RM_2DOffset`**. It is the *obvious* choice on an **Unlit** master, because 2D-offset refraction needs ⛔ no Normal input and Unlit has none. Every instrument said it worked: `set_properties` returned `true`, `get_properties` read back `RM_2DOffset`, `recompile` did ⛔ not raise, the graph verified pin-by-pin.

⛔ **It produced ZERO refraction. In FOUR configurations.** Measured, ⛔ not eyeballed — a region-restricted frame diff, body box vs two background boxes:

| config | BODY mean Δ | BG mean Δ | verdict |
|---|---|---|---|
| `RM_2DOffset`, AfterDOF, opacity 0.10, strength 2 | 9.99 | 8.05 / 10.11 | ⛔ **body ⛔ NOT elevated ⇒ nothing** |
| `RM_2DOffset`, BeforeDOF, opacity 0.90, strength 5 | 7.40 | 9.95 / 12.00 | ⛔ body **BELOW** background ⇒ nothing |
| `RM_2DOffset`, BeforeDOF, **DefaultLit**, strength 5 | 7.05 | 6.89 / 9.36 | ⛔ nothing |
| ⭐ **`RM_IndexOfRefraction`, AfterDOF, Unlit, strength 3** | **68.42** | 17.08 / **10.40** | ✅ **6.6× elevation, confined to the body** |
| ⭐ **shipped values, strength 0.25** | **110.23** | 18.63 / **8.51** | ✅ **13×** |

⇒ ⛔⛔ **`RM_2DOffset` is a silent no-op here. Use `RM_IndexOfRefraction`.** ⚠️ It is the ⛔ same species as `GHOST-§5`'s `bUsedWithSkeletalMesh` defect: **the asset is correct, every return value is green, and the pixels are empty.** ⭐ Only a **frame diff against a strength-0 control** could tell the difference — a thumbnail could not, and neither could I.

### ⚠️ AND TWO HYPOTHESES I HELD CONFIDENTLY THAT WERE ⛔ WRONG — refuted by controlled flips, ⛔ not by argument
- ⛔ *"`MSM_Unlit` blocks refraction"* — **FALSE.** Flipping ⛔ only the shading model Lit→Unlit with everything else fixed changed the body by **4.65**, ⛔ below the background noise (8.29 / 8.68). ⭐ **The Unlit master is safe; the ghost's legibility-in-the-dark design does ⛔ not have to be sacrificed.**
- ⛔ *"`MTP_AfterDOF` separate translucency blocks refraction"* — **FALSE.** The shipped result runs in the ghost's ⛔ original `MTP_AfterDOF`. ⇒ ⭐ **`TranslucencyPass` was restored and is ⛔ unchanged.**

---

## 3. ⛔⛔⛔ THE NEAR-MISS THAT WOULD HAVE SHIPPED A REGRESSION INTO THE GHOST — **IOR's NEUTRAL IS `1.0`, ⛔ NOT `0`**

I defaulted `VeilRefractionStrength` to **0.0** specifically so every pre-existing instance would be untouched. **That reasoning was wrong and the pixels caught it.**

⛔ In `RM_IndexOfRefraction` the Refraction input is an **index of refraction**. `0` is ⛔ not "off" — it is a **violently refractive** value. At strength 0 the test figure was ⛔ still heavily distorted (`handoffs/TASK-832-C` was shot to prove the opposite and disproved it instead).

⇒ ⛔ **`MI_Ghost_Translucent`, which inherits the parameter, would have silently acquired a refraction nobody asked for** — a live change to a shipped surface, invisible in every success return.

✅ **THE FIX — a `+1.0` bias node**, so the graph is `Refraction = 1.0 + Sine(...) × VeilRefractionStrength`:
⇒ **strength 0 ⇒ IOR exactly 1.0 ⇒ mathematically zero bend.**

⭐ **Verified on pixels, ⛔ not on arithmetic:** `TASK-832-C` (strength 0, everything else identical) shows a **clean, undistorted** figure with the castle's wall band running perfectly straight behind it.
⚖️ *⛔ A defaulted parameter is ⛔ not automatically a neutral parameter. ⛔ Neutrality is a property of the CONSUMING MODE, and it has to be measured in the mode that consumes it.*

---

## 4. ✅ THE GHOST IS PROVABLY UNTOUCHED — THREE INDEPENDENT INSTRUMENTS

| instrument | result |
|---|---|
| ⭐ **sha256 of `MI_Ghost_Translucent.uasset`** | `fbeb1f5f413851a77139f720…` **before** → `fbeb1f5f413851a77139f720…` **after** ⇒ ⛔ **byte-identical, never written** |
| its own parameters, read back | `CoreOpacity 0.16` · `RimBoost 6` · `PulseAmount 0.12` · `TeamColor (0.45, 0.78, 1.00)` = ⭐ **exactly `TASK-756`'s recorded values** · `VeilRefractionStrength` **0** (inherited) ⇒ IOR 1.0 |
| 👁️ **PIXELS — `handoffs/TASK-832-D`** | the ghost material and the veil rendered **side by side on the same mesh**: the ghost is a **clean, crisp** translucent figure; the veil beside it is visibly scrambling the background. ⛔ **No refraction leaked onto the ghost** |

⛔ `MI_HeroSpirit_Blue` / `MI_HeroSpirit_Red` (the two inert `TASK-756` proposals) are likewise unwritten and inherit IOR 1.0.

---

## 5. 👁️ THE QUESTION THAT DECIDES THE FEATURE — *can the owner tell at a glance, without it reading as dead?*

### ✅ ON A NORMAL UNIT: **YES.** — `handoffs/TASK-832-A`, four figures at ~1600 uu
The control footman is a **solid dark silhouette**; the veiled footman beside it is a **pale, see-through, shimmering** figure. The difference is unmissable at play distance.

### ✅ AND IT DOES ⛔ NOT READ AS DEAD OR REMOVED — ⭐ and the reason is structural, not cosmetic
- ⛔ A **removed** unit leaves nothing. This one holds a **full, sharp, correctly-proportioned silhouette**.
- ⛔ A **dying/spawning** unit in this project's language **fades** — a uniform alpha ramp. This one is ⛔ not fading: it is **actively bending the ground behind it**, which is a thing only a *present, solid* object does.
- ⇒ ⭐⭐ **That is exactly why the row insisted on refraction over alpha, and the distinction survives contact with the arena.** An alpha-only version would have been indistinguishable from a death fade at this distance.

### ⚠️⚠️ ON THE METALLIC WITCH: **MUCH WEAKER — and the cause is `TASK-899`, not this material**
Rough per-figure luminance over the same frame (boxes include surrounding grass, so these are **indicative, ⛔ not photometric**):

| figure | mean L | **internal contrast (σ)** |
|---|---|---|
| control **footman** | 147.9 | **20.05** |
| veiled footman | 159.0 | **8.66** ⇒ ⭐ **σ collapses −57 %** |
| control **witch** | 143.3 | **10.84** ⇒ ⛔ **already flat and pale** |
| veiled witch | 156.1 | **11.53** ⇒ ⛔ **σ essentially unchanged** |

⇒ ⭐ **The *"much lighter"* half of the veil works on both (+11 vs +13 luminance).** ⛔ **The *"flattens into a pale wash"* half — which is most of what the eye actually catches — is nearly a no-op on her, because at 95.2 % metallic she is *already* a low-contrast pale chrome figure reflecting the sky.**
⛔ **This is ⛔ NOT mine to fix** (`TASK-899` / `J-W15` is Jonathan's, ship-as-is by default). ⭐ **But it is a real consequence of that ruling reaching a second lane**, and it is recorded here so the two are decided together rather than separately. 👁️ **`TASK-832-A` shows it directly: the two rightmost figures are the veiled and unveiled witch, and they are hard to tell apart.**

### ⚠️ THE THIRD THING FOR HIS EYE — **the veil and the HERO'S GHOST are now visual siblings**
They share a master and both read as *"pale translucent person"* (`TASK-832-D`). Distinguishers that ⛔ do exist: the ghost is **cold cyan, clean-edged, brightly rimmed and pulsing**; the veil is **neutral silver-lilac, refracting, dim-rimmed and static**. Their **contexts never overlap** (the ghost appears only when your hero dies; the veil only on your own living units). ⛔ Flagged rather than "fixed", because tuning them apart is a taste call and the levers are all instance parameters.

---

## 6. ⚠️ THE ONE PLACE HIS WORD AND THE SHIPPED PIXELS ⛔ DIVERGE — SAID PLAINLY

🧑 His sentence is *"the objects you see through them are going to be **blurred**."*
⛔ **What ships is DISTORTION/SMEAR, ⛔ not a true optical blur.** At play distance it reads as blur; up close (`TASK-832-B`) it reads as **torn and displaced**, ⛔ not softened.

⭐ **And UE 5.8 has the real thing — measured, ⛔ not assumed:**
- **`r.Substrate = 1`** (Substrate is ⛔ ON in this project)
- **`r.Refraction.Blur = 1`** — its own help text: *"Enables rough refractions, i.e. **blurring of the background**. Only enabled when Substrate is enabled."*
- `r.RefractionQuality = 2`, `r.DisableDistortion = 0`

⛔ **Why I could not reach it:** Substrate rough refraction is driven by the surface's **Roughness**. ⛔ **`M_HeroSpirit` is `MSM_Unlit` and Unlit has no Roughness input at all.** ⇒ true blur would require changing the master's **shading model** — ⛔ a far heavier edit than adding a refraction input, and one that would land directly on the ghost's deliberate *"Unlit so it stays legible in the dark moonlit arena at range"* design (`TASK-756` / `GHOST-§4`).

🧑 ⇒ **A REAL OPTION, ⛔ NOT BOARDED, offered for a manager/Jonathan call:** a **separate lit master** for the veil would unlock genuine `r.Refraction.Blur` background blur. ⛔ **`WITCH-§5` forbids a new master** (*"⛔ never a new master"*), so this needs an explicit amendment and is ⛔ deliberately ⛔ not something I did on my own authority. ⚠️ **My honest read: the current smear already satisfies the design intent at gameplay distance, and a second master is ⛔ not obviously worth its cost — but the choice is his, and he should know the engine can do the literal thing he asked for.**

---

## 7. ⚠️ THE TRANSLUCENCY COST, STATED HONESTLY (board item 5, `J-W8`)

- ⛔ **A fixed, unavoidable cost was taken the moment `MP_Refraction` was connected:** `RefractionMethod != RM_None` + a connected input ⇒ the material is **distorting**, which allocates **a full-screen framebuffer** (`r.DisableDistortion`'s own help: *"Saves a full-screen framebuffer's worth of memory"*). ⚠️ **This is ⛔ NOT proportional to unit count — it is paid once, as soon as one distorting material is visible.** ⚠️ Relevant given this project's recorded VRAM findings.
- ⛔ **Every `M_HeroSpirit` instance now enters the distortion pass**, including the ghost — even though its **visual** output is provably unchanged (§4). ⭐ The ghost is **one pawn, only after the hero dies**, so the marginal draw cost is negligible; the framebuffer is the real line item.
- ⭐ **The per-unit cost is bounded by the DESIGN, not by the fleet:** his own sentence is *"The witch can only make ⛔ one unit at a time invisible."* ⇒ **veiled units on screen ≈ the number of live witches**, ⛔ not the fleet size. ⛔ This is ⛔ not a 50,000-uu-field-wide translucency load.
- ⚠️ **It compounds with `FOG-§`'s translucent pass (`J-W8`)** — both are screen-space translucency work and both scale with **covered screen area**. ⛔ Fog covers the whole screen; the veil covers a few unit silhouettes. ⇒ ⛔ **fog dominates; the veil is the cheap one of the pair.** ⛔ Neither was profiled — ⛔ I am reasoning from what each pass touches, and I am saying so rather than quoting a number I did not measure.
- ⚠️ **Known translucency sorting artefacts apply** (`WITCH-§5` predicted them): the mesh's own front/back faces and the refraction interact at silhouette edges. `TwoSided = false` on the master already halves this and was ⛔ left as-is.

---

## 8. 📌 FOR INTEGRATION (build-master) — ⛔ NOT DONE BY ME, and one of these is a real trap

⛔ **Nothing references `MI_Unit_Invisible` — `get_referencers` = `[]`.** The wiring seam is named in the shipped code but deliberately empty: `SummonedUnit.cpp:2879`, on `BreakInvisibility`'s true edge — *"📌 The MATERIAL swap (`MI_Unit_Invisible`, `WITCH-§5`) hangs off this same edge and is ⛔ NOT `TASK-829`'s — it is the art/render lane."*

1. ⛔⛔ **APPLY IT TO ⛔ EVERY SLOT, ⛔ NOT SLOT 1.** ⚠️ **This is the trap.** Units carry **two** slots, `[TeamRegion, <Card>PBR]`, and `BeginPlay` writes `MI_TeamColor_<Team>` to **slot 0**. A slot-1-only swap leaves slot 0 **fully opaque**.
   ⇒ ⛔ On a normal unit that is a 1.7–4.1 % opaque speck. ⛔⛔ **On the WITCH `team_region` is 18 % and it is her HAT BRIM (`TASK-833`)** — a slot-1 swap would leave **an opaque chrome hat floating over a ghostly body.** ⭐ A **MID over the whole body** is the correct shape — the same conclusion `TASK-756` reached for the ghost, for the same reason.
2. ⚠️ **Applying it destroys the team read** (slot 0 is gone). ⛔ Acceptable — it is the **owner's own** unit. ⭐ If a team tell is ever wanted, `TeamColor` is already exposed; ⛔ do ⛔ not add a second material.
3. ⚠️ **Restoring on break** = clear the overrides **and re-apply `MI_TeamColor_<Team>` to slot 0**, ⛔ or the unit comes back untinted.
4. ⚠️ **`WITCH-§3` makes the veil permanent-until-broken**, so the swap is a **one-shot on each edge**, ⛔ never a per-tick write.

---

## 9. ⚠️ WHAT I COULD ⛔ NOT VERIFY — stated rather than glossed

- ⛔⛔ **NOT PROVEN ON A SKELETAL MESH IN PIE.** `TASK-835` §7 requires exactly this, and `GHOST-§5`'s defect class is invisible in-editor. ⛔ **I tested on STATIC mesh actors** (`SM_Footman` / `SM_Witch`), because `SK_Witch` does ⛔ not exist and units currently spawn static (`TASK-833` §6). ⭐ **Mitigation: the flag that governs that defect — `bUsedWithSkeletalMesh` — is `true` on the master and was read back after every edit, and the ghost already exercises this master on `SKM_Quinn_Simple`.** ⛔ But the PIE gate is ⛔ still open and it is ⛔ not mine to close.
- ⛔ **No PIE, no packaged build.** The `MTP_AfterDOF` + Substrate + distortion combination was verified in the **editor level viewport** only.
- ⚠️ **The luminance figures in §5 are indicative** — hand-placed boxes including background, ⛔ not calibrated photometry. ⭐ The **refraction** numbers (§2) are sound: they are self-referential region diffs with two background controls.
- ⚠️ **A single still cannot show the wobble.** `VeilWobbleSpeed = 0.6` animates; ⛔ every capture here is one frame. ⭐ **Motion is a large part of the at-a-glance read and Jonathan will see more than these PNGs do.**

---

## 10. 🪤 ENVIRONMENT — an editor hazard hit and cleared, recorded because it will recur

⛔ **The editor (PID 11892) was wedged on a modal `Restore Packages` dialog** and MCP `:8000` accepted connections but ⛔ never answered — ⚠️ **a listening port is ⛔ not liveness, exactly as briefed.**
⛔ The restore offered **`BP_FogArea`** and **`WBP_CombatantHealthBar`** autosaves. ⛔ **Both had to be DECLINED** — `WBP_CombatantHealthBar` is the ⛔ **cancelled** cast-bar work (`WITCH-§9.0`), so restoring it would have resurrected a deliberately-killed deliverable.
✅ Cleared by this project's own precedent: killed the editor, **parked** `Saved/Autosaves/PackageRestoreData.json` → `.bak-2026-09-03-task832-declined-fogarea-castbar` (joining ~10 prior `.bak-*-forcekill` files), relaunched clean.

✅ **`L_Arena` never-save law honoured, verified by disk mtime, ⛔ not by a save prompt:**
- ⛔ **Autosave was DISABLED for the whole test** and restored to `true` afterwards — ⛔ specifically so no autosave could write an `L_Arena_Auto*` and re-arm the very dialog above.
- 4 temporary `ZZTEST_*` actors were placed in `L_Arena` and **all 4 deleted** (`find_actors("ZZTEST")` → **0**).
- ⭐ **`Content/Maps/L_Arena.umap` `LastWriteTime` = 2026-08-27 3:05 PM — untouched.**
- ⭐ **A sweep of ALL `Content/**/*.uasset|umap` written in the last 75 minutes returns EXACTLY TWO FILES, both mine.**

---

## 11. ✅ SAVE PROOF — sha256, ⛔ never `is_dirty`, ⛔ never a success return

⛔ `AssetTools.is_dirty` was ⛔ not consulted (it is blind — `true` for every asset that exists). `save_assets` returned `true`; ⛔ **that was ⛔ not accepted as evidence.** Saved by **explicit path**, ⛔ never an empty list.

| file | before | after | verdict |
|---|---|---|---|
| `Content/Materials/M_HeroSpirit.uasset` | `0201a7e35d46f044a32474e1…` | **`4879a679c996213aaffd68c5…`** | ✅ **CHANGED — written** |
| `Content/Materials/MI_Unit_Invisible.uasset` | ⛔ did not exist | **`fea6e0d41aa8c573c2636bbb…`** | ✅ **CREATED** |
| `Content/Materials/MI_Ghost_Translucent.uasset` | `fbeb1f5f413851a77139f720…` | `fbeb1f5f413851a77139f720…` | ✅ ⛔ **UNCHANGED — control** |

### The master after the edit — read back, ⛔ every original property intact
`MD_Surface` · `BLEND_Translucent` · **`MSM_Unlit`** · **`bUsedWithSkeletalMesh = true`** · `TwoSided = false` · **`TranslucencyPass = MTP_AfterDOF`**
⇒ ⭐ **the ONLY changed property is `RefractionMethod`: `RM_None` → `RM_IndexOfRefraction`.**
Graph **25 → 37 nodes** (12 added), parameter groups `["01 Team", "02 Spirit", **"03 Veil"]`, `MP_Refraction` → `Add_4` (the bias node). ⛔ **Stock nodes only — the Custom-HLSL ban is honoured; there is no Custom node in the graph.** `recompile` raises on failure and did ⛔ not raise.

**New master parameters (group `03 Veil`), all defaulting to ⛔ ZERO EFFECT:**
`VeilRefractionStrength` **0.0** (⇒ IOR 1.0) · `VeilWobbleScale` 22.0 · `VeilWobbleSpeed` 0.9

---

## 12. 👁️ EVIDENCE — real renders in the real arena, ⛔ not asset-editor spheres

⚠️ **The material thumbnail is a USELESS instrument here and I am recording why:** it renders against a **flat grey backdrop**, and ⛔ **distorting a uniform colour produces a uniform colour.** ⛔ A thumbnail would have shown a clean sphere whether refraction worked or not — and for the first four configurations, it did ⛔ not work.

| file | shows |
|---|---|
| `handoffs/TASK-832-A-gameplay-distance-veiled-vs-control.png` | ⭐ **THE ONE TO LOOK AT.** ~1600 uu. L→R: **control footman · veiled footman · veiled witch · control witch** |
| `handoffs/TASK-832-B-refraction-closeup-castle-warped.png` | the castle visibly **bent and smeared** through the body — the refraction proof |
| `handoffs/TASK-832-C-control-strength0-no-refraction.png` | ⭐ **the control that caught the IOR-neutral bug.** strength 0 ⇒ clean figure, straight castle |
| `handoffs/TASK-832-D-ghost-unchanged-beside-veil.png` | **ghost · veiled footman · veiled witch · control witch** — the ghost renders **clean**, the veil scrambles |

---

## 13. 🧑 WHAT NEEDS A DECISION

| # | who | question |
|---|---|---|
| **1** | 📋 **manager** | ⭐ **Ratify the `M_HeroSpirit` extension** (`WITCH-§5`'s escape clause). ⛔ There was **no instance-only path** — `RefractionMethod` is absent from `BasePropertyOverrides`. The edit is on disk and provably inert for all 3 pre-existing instances |
| **2** | 🧑 **Jonathan** | 👁️ **`TASK-832-A`: can you tell the veiled unit at a glance, and does it read as *hidden* rather than *dead*?** ⭐ **The one question that decides the feature** |
| **3** | 🧑 **Jonathan** | ⚠️ **The veil barely shows on the WITCH** because `TASK-899` left her 95.2 % metallic. ⛔ Not mine to fix — but it should be decided **together with `J-W15`**, not separately |
| **4** | 🧑 **Jonathan** | ⚠️ **True background BLUR is reachable** (`r.Refraction.Blur = 1`, Substrate on) but needs **Roughness** ⇒ a **lit** master ⇒ an amendment to `WITCH-§5`'s *"never a new master"*. ⛔ Deliberately not done unilaterally |
| **5** | 🧑 **Jonathan** | ⚠️ **Veil vs the hero's GHOST read as siblings** (`TASK-832-D`). Contexts never overlap; levers are all instance params if he wants them pushed apart |
| **6** | 📋 **manager** | ⚠️ **The PIE-on-a-skeletal-mesh gate (`TASK-835` §7) is still OPEN** and cannot be closed by me — ⛔ `SK_Witch` does not exist and units spawn static |
| **7** | 📋 **manager** | ⛔ **§8's whole-body-override requirement is integration law, not a suggestion** — a slot-1-only swap puts an **opaque chrome hat** on a veiled witch |
