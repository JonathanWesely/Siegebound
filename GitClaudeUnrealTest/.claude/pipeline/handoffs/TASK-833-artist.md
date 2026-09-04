# TASK-833 — `SM_Witch`, the witch unit mesh — ART HANDOFF

**Agent:** art-director · **Date:** 2026-09-02 · **Status:** ready-for-integration
**Law:** `WITCH-§6` (naming/file map) · "Per-card visual assets" · "Textured mesh law (TRELLIS.2 art pipeline)" · "Skeletal rig & animation workstream (M7)"

---

## 1. What shipped

| asset | path | verified by |
|---|---|---|
| static mesh | `/Game/Meshes/SM_Witch` | editor read-back |
| base colour | `/Game/Textures/T_Witch_D` | editor read-back |
| normal | `/Game/Textures/T_Witch_N` | editor read-back |
| packed ORM | `/Game/Textures/T_Witch_ORM` | editor read-back |
| material instance | `/Game/Materials/Instances/MI_Witch_PBR` | editor read-back |

**Source files (raw-asset rule — check these in alongside the .uasset):**

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Witch.fbx`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Textures\Witch\T_Witch_D.png`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Textures\Witch\T_Witch_N.png`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Textures\Witch\T_Witch_ORM.png`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Concepts\Witch.png` (accepted concept, seed 71035)

**Tooling files edited (both are art-lane data, not code):**

- `Tools\ArtPipeline\concept_prompts.json` — new `Witch` entry (prompt, negative_prompt, seed 71035)
- `Tools\ArtPipeline\pipeline_manifest.json` — new `Witch` asset entry

⛔ **I did not touch Git.**

---

## 2. Fleet parity — every number MEASURED off the shipped roster first, then matched

I read `SM_Sorcerer` / `SM_Cleric` / `SM_Wizard` back out of the editor **before modelling anything**, because a witch that is correct in isolation and inconsistent with the roster is a defect.

| property | Sorcerer | Cleric | Wizard | **Witch (shipped)** |
|---|---|---|---|---|
| LOD0 tris | 15000 | 14998 | 15000 | **15000** |
| LOD chain | `[15000,7500,3750,1874]` | `[14998,7498,3750,1874]` | `[15000,7500,3750,1874]` | **`[15000,7500,3750,1874]`** |
| LOD count | 4 | 4 | 4 | **4** |
| Nanite | off | off | off | **off** |
| slots | `[TeamRegion, SorcererPBR]` | `[TeamRegion, ClericPBR]` | `[TeamRegion, WizardPBR]` | **`[TeamRegion, WitchPBR]`** |
| slot 0 material | `MI_TeamColor_Blue` | same | same | **`MI_TeamColor_Blue`** |
| slot 1 material | `MI_Sorcerer_PBR` | `MI_Cleric_PBR` | `MI_Wizard_PBR` | **`MI_Witch_PBR`** |
| convex hulls | 4 | 4 | 4 | **4** (0 box/sphere/sphyl) |
| height (Z) | 181.882 | 181.886 | 183.249 | **181.895** |
| origin | feet-center | feet-center | feet-center | **feet-center, min Z 0.012** |

Texture settings copied from `T_Sorcerer_*` exactly, then re-read to confirm:

| texture | sRGB | compression | LOD group | size |
|---|---|---|---|---|
| `T_Witch_D` | **true** | `TC_Default` | `TEXTUREGROUP_World` | 1024×1024 |
| `T_Witch_N` | **false** | `TC_Normalmap` | `TEXTUREGROUP_WorldNormalMap` | 1024×1024 |
| `T_Witch_ORM` | **false** | `TC_Masks` | `TEXTUREGROUP_World` | 1024×1024 |

`MI_Witch_PBR` parent = `/Game/Materials/M_AssetPBR`; params `BaseColor`/`Normal`/`ORM` bound to the three textures above. `UVMap` confirmed as the UV layer (`uv_layer_ok: true` in the refine report).

---

## 3. Read-back verification (hazard 3 — "a success return is not evidence")

Every write returned `true`. **I did not accept any of them as evidence.** After all writes I ran a **separate, independent** read-back call and confirmed:

- `exists /Game/Meshes/SM_Witch` → `true`; asset class → `StaticMesh`
- LOD count, per-LOD tri counts, Nanite flag, slot names, both slot materials — all as tabled above
- bounds min `[-59.571, -59.184, 0.012]` / max `[59.587, 59.216, 181.895]` — **identical to the refine report's exported bounds**, so nothing shifted through the import
- collision `AggGeom` read off `SM_Witch:BodySetup_0` → **4 `convexElems`, 0 box/sphere/sphyl**
- all three textures: exists, 1024×1024, and the three settings re-read individually
- `MI_Witch_PBR`: parent + all three texture params re-read
- `is_dirty` = **false** on all five assets ⇒ genuinely written to disk, not just in memory
- on-disk `.uasset` files confirmed present via the filesystem
- **in-engine rendered thumbnail captured and viewed** — the brim renders team-blue (slot 0) over the PBR body, proving both material assignments actually render rather than merely being set
- `find_assets` sweep for "Witch" → only my five assets, **no stray materials/textures from the FBX import** (three extra hits are engine/plugin assets matching "S-witch"/"RTS-witch" as a substring, not mine)

**Hazards 1 and 2 did not apply, and here is why rather than an assertion that they didn't:**
- **Hazard 1 (`reimport_meshes.py` defaults unknown CardID to `unit`, strips hand-authored collision):** I **never invoked that script**. `SM_Witch` did not previously exist, so this was a fresh `import_file`, not a same-path reimport. Collision was generated explicitly afterward by me and then read back.
- **Hazard 2 (Interchange `auto_generate_collision=False` drops UCX uids):** units carry `ucx: null` — there is **no UCX geometry in this FBX at all** (authored UCX is the buildings path). The 4 hulls are generated in-editor post-import, which sidesteps the flag entirely. Verified by reading `AggGeom`.

---

## 4. Silhouette — spec item (3), "three robed figures is a legibility failure"

The roster's existing casters, read from their own shipped concept prompts:

- **Sorcerer** — blank stone dome mask, antler stubs, granite slab shoulders, **both hands on a squat pillar**, blocky monolith, boots planted wide.
- **Cleric** — soft hood, **staff held ALOFT** + prayer tome, straight upright robe.
- **Wizard** — hood + beard, fireball in an upturned palm, **arms held OUT TO THE SIDES**.

**Witch** — broad flat circular **hat brim** wider than her shoulders, a short conical crown that **crooks forward** over the front edge, a compact iron **lantern held low** in one hand, the other hand **open and empty, fingers spread**, and a **ragged tattered hem**.

Four distinct axes, all readable at distance:
1. **Only pointed top in the roster.** Cleric and Wizard have round hoods; the Sorcerer a dome.
2. **Only unit with a wide disc from directly above** — and the top-down preview confirms it. This matters more than anything else here because the gameplay camera is top-down; the whole design is built around that being the binding constraint.
3. **Only caster whose prop is held LOW and on one side** — not aloft, not out to the sides, not two-handed in front. Asymmetric.
4. **No weapon of any kind**, and no face — she reads as a caster, and specifically as a *concealment* caster.

**Concept process, recorded because it cost real rolls:** 3 prompt revisions, 6 generations.
- **v1** (1587 chars) — identity landed perfectly but the roll came back **semi-realistic at ~6–7 heads tall**, not the roster's chunky 2.5–3. The FLUX prompt law's "hard style cliff at ≈1600 chars" was the cause: at 1587 I was right against it, and the style block sits at the END of the entry where it gets diluted.
- **v2** (1371 chars) — applied the law's own sanctioned remedy: **moved the style block to the FRONT** and cut description. Style tier fixed immediately. But the "crooked twisted-branch wand held low and angled across her body" reliably generated a **hafted axe/scythe blade** — she read as a *melee* unit, which is a hard fail for this card.
- **v3** (1482 chars, shipped) — **removed the wand entirely** rather than fighting it. The lantern became the single carried object and the free hand went open/empty. This killed the weapon failure at the root and reads *more* caster-like, not less. Also pulled "bruised-plum" out of the robe description, which had been flooding the whole figure purple and diluting the "one colour key" convention.
- Rolls judged on pixels; **seed 71035** chosen over 71032/71034 for: flattest and widest brim (best top-down read *and* best team-region plane), **compact lantern with no long thin dangling chain** (71032's chain would likely have broken or blobbed in the 1.5 uu voxel remesh), properly muted palette, visible planted legs, and no ground-shadow ellipse or floating debris for the image-to-3D stage to reconstruct as junk geometry.

All three prompt versions obey the **FLUX positive-form law** — zero in-prompt negations, verified by regex, not by eye. `negative_prompt` is authored but the tool warns it is **dropped** by FLUX.1-dev (guidance-distilled); it is inert today and I did not rely on it.

---

## 5. Facing — READ, not assumed (spec item 2)

The spec required reading per-asset export facing because it is **generation-scoped** and the fleet flipped on 2026-07-28.

`pre_rotate_z_deg = 180.0`, set from the **Sorcerer entry's own systemic flag** ("every FUTURE unit through Stage 2 will inherit the same +180 need until `_ue_handedness_precomp` is reconsidered for the unit lane"). The Witch is a future unit through Stage 2, so it inherits.

**Then I verified it on pixels rather than trusting the inheritance.** The documented consequence of the 180 is that conformed/preview space fronts `+Y`, so the previews invert. Confirmed by positive identity cues:

- `Cache/Witch/previews/preview_back.png` shows the **FRONT** — the shadow-void face under the brim, the crossed robe front, the sash knot, the lantern presented forward, the crown tip toward the viewer.
- `Cache/Witch/previews/preview_front.png` shows the **BACK** — the rear of the cowl and hat, no face, the lantern from behind.

That inversion **is the compensation, not a defect** — do not "fix" it to 0.0. Net result: her front arrives at UE-local `+Y` exactly like the fleet, so the fleet-wide `-90` yaw ritual is correct for her with no exception hatch needed.

⚠️ Note this character has **no face**, so facing cannot be judged by a face. The reliable cues are **the forward-crooked crown tip** and **which hand holds the lantern**. Recorded because the next person to check her facing will look for a face and not find one.

---

## 6. ⚠️ THE RIG — spec item (4). Cost stated, NOT silently absorbed.

**`SK_Witch` does not exist** (verified: `exists /Game/Characters/SK_Witch` → `false`). Per the SkeletalMeshComponent swap contract, `ASummonedUnit::ResolveSkeletalVisual` composes `/Game/Characters/SK_<CardID>`, fails to resolve, and **falls back to the static `VisualMesh`**. That is null-safe and will not crash — but it means:

> **The Witch will spawn as a STATIC, UNANIMATED mesh. `TASK-830`'s 3-second interruptible cast will have NO visual tell whatsoever.**

That matters more for this card than for any other unit in the roster, because the cast is *interruptible* — the player needs to see it **start**, see it **running**, and see it **break**. A static mesh gives all three states identical pixels. This is a design-visible gap, not a polish item.

**What the rig would cost (a separate deliverable, per the law — flag it, don't absorb it):**

1. `SK_Witch` at `/Game/Characters/SK_Witch`, rigged onto the **shared `SK_Footman_Skeleton`** (the sole skeleton asset; a bespoke skeleton is a non-default needing an explicit manager ruling). Same two-slot material contract as the SM.
2. Anim clips at `/Game/Characters/Anims/`: `A_Witch_{Idle,Walk,Death}` (shared `A_SiegeBiped_{Idle,Walk}` is permitted) — **plus a NEW cast clip**.
3. `ABP_Witch` (or the shared `ABP_SiegeBiped`), plus the M7.6 LOD chain and `VisibilityBasedAnimTickOption`.

**Two named hazards whoever takes the rig must handle — both are recorded, measured defects on this project, not speculation:**

- 🪤 **The cast is a FIFTH action.** The law enumerates exactly `Idle` / `Walk` / `Attack` / `Death`. A 3-second **interruptible channel** with a start, a hold and a break is none of those, and it is not an `Attack`. **Naming it is a manager call, not an art call** — the same shape of decision as `WITCH-§3`'s "never a quiet 7th enumerator". Do not let it get invented at the mesh.
- 🪤 **`rig_character.py` hardcodes "Front is `-Y`" (`:383`) and drives every forward motion toward `-Y`** — toe offset, leg/arm swing, and the attack lunge. This asset ships `pre_rotate_z_deg = 180`. The law is explicit that a disagreement here silently corrupts a rig (**inverted toe offset, inverted leg swing, inverted lunge, and swapped `_l`/`_r` bone sides** — exactly what happened to the shipped Sorcerer) and that **no Blueprint or C++ yaw fix can ever detect or repair it**. The Sorcerer is the precedent that this is survivable — its rig was regenerated from the fixed FBX minutes after the facing fix landed — but **I have not measured what front the exported Witch FBX presents to the rigger, and I am not going to assert it.** Verify it with the law's own `-Y`-camera + `Footman.fbx`-control render **before** shipping a rig.

---

## 7. Declared residuals — the things I could not make good

**(a) ENGINE: she is the one unit on TRELLIS.2, and she bakes darker than the fleet.**

`MESHY_API_KEY` is **unset** in this environment, so the Sorcerer's `meshy-i23d` lane is unavailable to an agent. TRELLIS.2 was available and healthy. This matters because the 2026-07-26 FLEET-REMASTER moved 11 units **off** TRELLIS onto Meshy precisely because TRELLIS meshes read washed-out.

Measured across 4 Stage-2 runs off the same cached GLB (Stage 2 is ~33 s and spends no quota, so iterating was cheap):

| run | gamma / gain | `mean_linear_after` | verdict |
|---|---|---|---|
| 1 | 0.55 / 1.2 (locked fleet profile) | 0.0161 | **near-black body**, no readable material — 6× darker than the Wizard's already-flagged 0.0968 |
| 2 | 0.40 / 1.6 | 0.0348 | better |
| 3 | 0.32 / 2.0 | 0.0625 | ⛔ **saturated RGB confetti speckle across the robes** |
| 4 ✅ | **0.46 / 1.45** | **0.0250** | speckle gone; cowl, sash, cuffs, hem layers and boots all read |

**The informative run is #3.** The lift does not *create* that noise — it **reveals** TRELLIS bake noise that was always buried in the near-black, because a low gamma amplifies darks ~40× while amplifying brights ~2×. So the ceiling here is **bake-noise visibility, not the anti-bleach guard** (`shoulder_compressed_fraction` never exceeded 0.0124 — the guard was never close to binding). **Any future retune must be judged on preview pixels, never on `mean_linear_after` alone.** That reasoning is recorded in the manifest so it is not re-derived.

She is **still darker than her Meshy neighbours.** The real fix is the **engine**, not the levels: a `MESHY_API_KEY` re-run through `meshy_generate.py --mode image3d` at this same path (same-path overwrite — every soft ref survives, no renames) would supersede the whole tune. 🧑 **Jonathan's call.**

**(b) `team_region` is 18% of surface area vs the fleet's 1.7–4.1% band.**

Measured: 1189 faces, 18%, under the 0.35 guard, no warnings. **Deliberate, and here is the reasoning:** on this silhouette the huge brim pushes everything down — the shoulders sit at z ≈ 0.58, and critically **they are occluded from above by the brim**. Copying the Cleric's 0.66–0.84 `shoulder_caps` band would have produced a team region the gameplay camera **cannot see at all**. The brim is the only surface that reads as team colour from a top-down camera, and the in-engine thumbnail confirms it goes solid team colour and reads instantly.

**If Jonathan rules it too much colour**, the lever is recorded in the manifest: raise the `hat_brim` box z-min from 0.66 toward the crown base, shrinking the coloured annulus inward from the rim. **Do not fall back to `shoulder_caps`** — that is the option that looks reasonable and is invisible in play.

*The Archer bare-head lesson is satisfied structurally rather than by a careful box: this character has no face (a shadow void by design) and any head geometry under the brim is vertical, so a `+Z min_dot 0.50` filter cannot reach it. Confirmed on the front/back previews.*

**(c) What I could not verify without a human eye / a running game:**

- Whether she reads correctly **at true gameplay camera distance in a live match** next to the Sorcerer and Cleric. I verified silhouette distinctness on concept pixels, on four Stage-2 preview angles including a genuine top-down, and on the in-engine thumbnail — but never in PIE at real distance among real units. MCP has no input lane and I did not start PIE.
- Whether the **18% team region** is aesthetically right. Measured and flagged; it is a taste call.
- Whether the **residual darkness** is acceptable next to the Meshy fleet in-game rather than in a preview.

---

## 8. Notes for integration (`TASK-835` / build-master)

1. **Pivot / scale:** feet-center, min Z `0.012`, height `181.895`. Same convention as the whole fleet — no special handling.
2. **Material slots:** `[TeamRegion, WitchPBR]` in that order. **Slot 0 is the BeginPlay team-recolor target** (`SummonedUnit.cpp` hardcodes `MI_TeamColor_<Team>` onto slot 0) — this works unchanged, no code needed.
3. ⚠️ **`BP_Unit_Witch` must hand-author the static `VisualMesh` transform: yaw `-90` and Z `= -CapsuleHalfHeight`.** This is a **known latent trap**, not a suggestion: the static `VisualMesh` transform has **no C++ owner** (that derivation is parked debt, TASK-335), so every new `BP_Unit_*` must set it explicitly — and `BP_Unit_Wizard` is the one unit that missed the ritual and sits at yaw 0 / Z 0. **Read the real capsule half-height; the "half-height == 90" assumption is banned** (shipped values range 74.5–145).
4. **Ghost:** the placement ghost resolves `/Game/Meshes/SM_Witch` by string and uses the controller's own `GhostYawOffset = -90`. That path is satisfied by this asset existing at this exact path. **The name is a code contract and must not drift.**
5. **Nothing here needs a `DT_Cards` row, a card-art texture, or the invisibility material** — those are `TASK-831` / `TASK-834` / `TASK-832`. I stayed inside my fence.
6. `L_Arena` was **not opened, not modified, not saved**. I saved only my five new assets by explicit path.

---

## 9. 🪤 Tooling defect found — REPORTED, NOT FIXED

**`Tools/ArtPipeline/concept_generate.py` reports `SUCCESS` on a fully black frame and writes it to disk.**

Observed: a 12,444-byte "successful" 1024×1024 PNG (`mean` 0.01, `max_stddev` 0.13, 100% near-black pixels) against ~800 KB for a real roll. **Reproducible, not transient** — seed 71031 on the v2 prompt returned black **twice** on identical input, so this is a real prompt/seed interaction the tool cannot see.

Two problems, both worth fixing together:
1. **No validation of the returned image.** A mean/variance guard (the check I had to write myself to get through this task) would have caught it instantly.
2. **`--force` is destructive.** The black frame **overwrote a good roll** that then had to be regenerated.

⛔ **I did not fix it:** `Tools/**/*.py` is CODE and needs the tooling QA gate. Flagging for the manager to board.

*(Also worth a line somewhere: the README's Stage-2 example says `--asset <Name>`; the script's real flag is `--card-id`. Minor, but it will cost the next person a few minutes.)*

---

## 10. Environment finding — a new Norton TLS host

`concept_generate.py` failed 3× with `CERTIFICATE_VERIFY_FAILED`. Probed and isolated: **`huggingface.co` passes bare** (the recorded Norton exclusion is live and working), but **`router.huggingface.co` — the inference router `provider=auto` actually calls — is NOT excluded and is being MITM'd.**

Fixed for this run **without weakening the standing law** by scoping `SSL_CERT_FILE` to the existing `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem` for those invocations only (verified: the router went from `ConnectError` to a clean `404`, i.e. TLS handshake succeeded). This is exactly the recorded "SSL_CERT_FILE = fallback for other hosts" case. **The `hf.space` Stage-1 path was unaffected** and ran bare.

🧑 **Suggest adding `router.huggingface.co` (or `*.huggingface.co`) to the Norton exclusions** so the concept lane works bare like the rest.

⛔ **`HF_TOKEN` was read from the environment only** — never echoed, never logged, never passed on argv, never written to any file.
