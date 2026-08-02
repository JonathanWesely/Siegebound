# TASK-375 — [AG-A8] `BP_Unit_Sorcerer` — parent `ASorcererUnit` + the static `VisualMesh` ritual

**Agent:** art-director · **Date:** 2026-08-02 · **Status: ready-for-integration.**

**Editor/MCP:** up throughout. `IsPIERunning` = **false** before and after. No asset editors were open at start.
**Discipline honored:** one Blueprint touched · **never save-all** (single explicit path list) · **`L_Arena` NEVER saved**
· no Git command run by me · no C++ · no gameplay code · no `ABP_Sorcerer` · editor never closed.

---

## 0. Headline

✅ **`/Game/Blueprints/Units/BP_Unit_Sorcerer` created FRESH with parent `ASorcererUnit` in ONE step — no
duplicate, and no reparent at all.** `BlueprintTools.create` accepted the actor class directly as `asset_type`, so
the recorded duplicate+reparent corruption pattern was never even approached.
🎯 **THE FACING RITUAL — yaw is `+90`, NOT the fleet's `-90`, and it is PROVEN in the viewport by a positive cue
(the stone mask), not by absence of one.** Full evidence in §3, four captures committed alongside this note.
📏 **Capsule half-height READ off this Blueprint = `88.0` → `VisualMesh.RelativeLocation.Z = -88.0`.** Both numbers
reported as required. The banned "== 90" assumption was never used.
🔴 **FLAG, NOT FIXED (C++ concern, separate task): `SK_Sorcerer` WILL march backwards at runtime**, and so will the
**placement ghost**. Both yaws are C++-owned constants I am forbidden to touch. Proven in the viewport (§4).
✅ Compiled with **`warnings_as_errors = True`** — zero errors *and* zero warnings. Saved, `is_dirty = false`.

---

## 1. What shipped

| | |
|---|---|
| Asset | `/Game/Blueprints/Units/BP_Unit_Sorcerer` |
| On disk | `Content/Blueprints/Units/BP_Unit_Sorcerer.uasset` (40,413 B) |
| Parent class (readback) | **`/Script/GitClaudeUnrealTest.SorcererUnit`** ✅ |
| Generated class | `BP_Unit_Sorcerer_C` |
| Dependencies | `/Script/GitClaudeUnrealTest`, `/Game/Meshes/SM_Sorcerer`, `/Game/Materials/Instances/MI_TeamColor_Blue` — nothing dangling |

`/Game/Blueprints/Units` now holds **13** `BP_Unit_*` assets. Project-wide `find_assets("/Game","Sorcerer")` returns
**exactly 12** (the 11 pre-existing TASK-371/372/373 assets + this one) — **zero strays**. `ABP_Sorcerer` does **not**
exist and was not created (`ResolveSkeletalVisual` falls back to `ABP_Footman`).

## 2. Hard readback — every acceptance criterion, post-save

| Criterion | Required | **Readback** | |
|---|---|---|---|
| Parent | `ASorcererUnit` (NOT `ASummonedUnit`) | `/Script/GitClaudeUnrealTest.SorcererUnit` | ✅ |
| `CardID` | `Sorcerer` | **`Sorcerer`** | ✅ |
| `VisualMesh.StaticMesh` | `/Game/Meshes/SM_Sorcerer` | `/Game/Meshes/SM_Sorcerer.SM_Sorcerer` | ✅ |
| **`VisualMesh` yaw** | hand-authored | **`+90`** (pitch 0, roll 0) | ✅ **§3** |
| **`VisualMesh` Z** | `-(READ capsule half-height)` | **`-88.0`** | ✅ **§3b** |
| **Capsule half-height READ** | read, never assumed | **`88.0`** (radius `34`) | ✅ |
| `VisualMesh` scale | 1,1,1 | `(1,1,1)` | ✅ |
| `VisualMesh` slot 0 | Blue design-time placeholder | `[MI_TeamColor_Blue]` | ✅ fleet 11/12 |
| `bVisible` / `CastShadow` | true / true | `true` / `true` | ✅ |
| **`SkeletalVisualMesh` transform** | **MUST NOT be hand-authored** | **`(0,0,0)` loc, `(0,0,0)` rot — UNTOUCHED** | ✅ |
| `SkeletalVisualYawOffset` | C++ default, absolute | **`-90`**, not overridden, **no `+=` anywhere** | ✅ |
| Compile | clean | **`warnings_as_errors=True` passed** | ✅ |
| Save | explicit path list | `is_dirty` `true` → **`false`** | ✅ |

**Spawn-path contract VERIFIED, not assumed** — the composed string
`/Game/Blueprints/Units/BP_Unit_Sorcerer.BP_Unit_Sorcerer_C`:
- `load_asset` resolves it, and
- it appears in `search_subclasses(ASummonedUnit)` ⇒ **`IsChildOf(ASummonedUnit)` passes by inheritance.**

**⇒ NO C++ / spawn-path change is needed. Confirmed, not inferred.**

### 2a. Fleet conformance — I read all 12 shipped BPs first, then matched them

Rather than work from the description, I dumped every shipped `BP_Unit_*`. The static-`VisualMesh` ritual is
**exact on 12/12**: `RelativeLocation.Z == -CapsuleHalfHeight`, scale `(1,1,1)`, `bVisible` true, `CastShadow` true
(Ogre −145/145 · Cavalry −104/104 · Knight −95/95 · Pikeman −95/95 · Longbowman −92/92 · Cleric −91/91 ·
Footman −90/90 · Archer −90/90 · **Wizard −88/88** · Miner −86.5/86.5 · Sapper −84.5/84.5 · MilitiaMob −74.5/74.5).
`BP_Unit_Wizard` has since been brought in line by TASK-334 — it now measures yaw `-90` / Z `-88`, so the
"one outlier" note in CONVENTIONS §493 is **stale on the yaw/Z axis** and can be retired.
`OverrideMaterials = [MI_TeamColor_Blue]` on **11/12** (Wizard is `[]` — still the outlier there). I followed the 11.
`HPBarWidget` is `(0,0,0)` on all 12 — its height is C++-owned (`BarHeightZ`), nothing to author.

---

## 3. 🎯 THE FACING RITUAL — `+90`, and how it was proven

### 3a. Three independent lines, all agreeing — and I did not stop at the two I was handed

**(1) Prior evidence (given):** TASK-371's controlled 4-way thumbnail (`SM_Footman`/`SM_Cleric`/`SM_Wizard` front,
`SM_Sorcerer` back) and TASK-372's cue-by-cue static/skeletal corroboration.

**(2) An independent axis derivation I ran from the C++ + the FBX data, which neither prior task did.**
`ResolveSkeletalVisual`'s comment fixes the fleet contract: the fleet bakes front on **Blender −Y**, which arrives as
**UE-local +Y**, and actor forward is `+X`, so `Rot(θ)·(0,1,0) = (1,0,0) ⇒ θ = -90`. TASK-373 recorded that
**`Sorcerer.fbx`'s front faces Blender +Y**, and TASK-371 §4 independently measured that the FBX import **mirrors Y**
(Blender −39.21/+39.16 ↔ UE −39.157/+39.212). Compose them: the Sorcerer's front arrives at **UE-local −Y**, so
`Rot(θ)·(0,-1,0) = (sinθ, -cosθ) = (1,0,0) ⇒ **θ = +90**`. This is a *derivation*, not a thumbnail impression, and it
lands on the same number.

**(3) THE VIEWPORT CHECK — a positive cue, with a control, as instructed.**
`CaptureAssetImage` **does not support Blueprints** ("Asset type does not support image capture"), so a thumbnail
route was unavailable. I placed temporary actors in the live level instead, at **actor yaw `0`** so that
**actor-forward is world `+X`**, and captured with explicit camera transforms (Jonathan's viewport camera was never
moved — `CaptureViewport` takes a `captureTransform`).

**Control first.** `BP_Unit_Footman` (known-good, yaw `-90`), camera at `+X` looking `-X`:
→ **face, blue helm, spear, kite shield, lion tabard.** So this camera pose = "standing in the unit's path".
Capture: `TASK-375-facing-control-Footman-front.png`.

**Then `BP_Unit_Sorcerer` at yaw `+90`, identical relative pose:**

| cue | front camera (`+X` → `-X`) | back camera (`-X` → `+X`) |
|---|---|---|
| **stone mask (eye sockets + muzzle)** | **PRESENT** ✅ | absent — smooth dome |
| **rune monolith** | **PRESENT**, planted at screen-right | absent |
| **central knotted sash** | **PRESENT** | dark spine strap instead |
| ribbed cowl mass | behind the mask | fills the shoulders |
| blue `TeamRegion` | shoulder slabs | shoulder slabs |

Captures: `TASK-375-facing-static-yaw-plus90-FRONT-mask-monolith.png` /
`…-BACK-control.png`.

**⇒ THE CUE THAT PROVED IT: the faceless stone MASK — two dark eye sockets and the muzzle — is presented toward the
unit's direction of travel at yaw `+90`.** That is a *positive* identity cue, exactly as the brief demanded, and it
is corroborated by the **monolith** (front-only) and the **central sash** (front-only). The reciprocal capture is
simultaneously the **disconfirmation of `-90`**: rotating the mesh 180° swaps which side faces `+X`, so the back
image *is* what yaw `-90` would have put in the travel direction — a maskless dome with the monolith hidden. The
unit would have marched backwards.

**`-90` is therefore not merely "suspect" — it is measured wrong for this asset. `+90` is shipped.**

### 3b. The Z — both numbers, as required

**READ capsule half-height on this Blueprint: `88.0`** (radius `34`). **Applied `VisualMesh.RelativeLocation.Z = -88.0`.**
Read off `CollisionCylinder` immediately before use and re-read after the save; the "half-height == 90" assumption was
never used anywhere.

⚠️ **Worth a manager eye (recorded, deliberately NOT acted on): `88 / 34` is the `ACharacter` constructor default.**
Neither `ASummonedUnit` nor `ASorcererUnit` resizes the capsule, and **11/12 shipped units hand-author theirs to match
their mesh** — only `BP_Unit_Wizard` ships at the raw default, which is what a fresh BP inherits. `SM_Sorcerer` is
**181.81 UE tall / 90.98 wide**, so a mesh-matched capsule would be ≈ **91 / 40** (the Cleric's exact profile, and the
Cleric is the same class of robed humanoid). I did **not** set it: the task spec says only "read the half-height and
negate it", capsule size is collision/nav/spacing (gameplay-adjacent, no ruling), and TASK-340's own fence says
"**read** `GetScaledCapsuleHalfHeight()`, never assume". The BP is fully self-consistent as shipped — feet land
exactly on the floored capsule bottom either way, because Z tracks whatever the capsule is. **If the manager wants it
mesh-matched, it is a one-line follow-up: set the capsule, then re-derive Z from the new value.**

---

## 4. 🔴 FLAG — the SKELETAL visual and the placement GHOST will both face backwards. NOT fixed here.

Both are C++-owned absolute constants; touching either from a Blueprint is an automatic QA FAIL, and fixing them is a
C++ concern and a separate task. **Flagged with evidence rather than asserted.**

**Proven in the viewport.** I placed a `SkeletalMeshActor` of `SK_Sorcerer` at world yaw **`-90`** — which is exactly
the runtime configuration `ResolveSkeletalVisual` produces (`SkeletalVisualYawOffset = -90.f`, absolute, on an actor
whose yaw follows movement) — plus `SK_Footman` at the same yaw as the control, and captured both from the travel
direction:

| | side facing the direction of travel |
|---|---|
| `SK_Footman` (control) | **FRONT** — face, blue hood, spear, shield, lion tabard ✅ |
| **`SK_Sorcerer`** | **BACK** — featureless dome, **no mask**, ribbed cowl across the shoulders 🔴 |

The reciprocal capture of the same actor shows the **mask with both eye sockets and the muzzle** on the *opposite*
side — so this is a genuine 180°, established by a positive cue on both sides rather than by "no face visible" (which
TASK-372 correctly warned is non-decisive for a faceless-mask unit).
Capture: `TASK-375-SK-runtime-yaw-minus90-BACKWARDS-flag.png` (left = travel direction, right = reciprocal).

**Consequence:** the moment `SK_Sorcerer` resolves at BeginPlay it becomes the runtime visual and the static
`VisualMesh` is hidden — so **in normal play the Sorcerer will march, cast and die facing backwards**, and my correct
`+90` static value is only visible on the null-safe fallback path. **The static ritual is done right; the runtime
result still needs the C++ task.**

**A THIRD seam, same root cause, also C++-owned:** `ASiegePlayerController::GhostYawOffset = -90.f`
(`SiegePlayerController.h:825`) rotates the placement **ghost**, which uses the static `SM_<CardID>`. So the
**drag-to-place ghost will also show the Sorcerer's back**. CONVENTIONS §486 notes the ghost "has never mis-faced
even for units whose BP yaw is 0" — true, because it carries its own offset, but that reasoning assumes the fleet
bake, which this asset does not follow.

**Recommended shape of the fix (for the manager / programmer — NOT authored by me):** all three seams
(`SkeletalVisualYawOffset`, the static `VisualMesh` ritual, `GhostYawOffset`) are the same fact — *this asset's baked
forward is 180° from the fleet*. `SkeletalVisualYawOffset` is already the documented `EditDefaultsOnly` **exception
hatch** for "a genuinely differently-baked mesh", which is precisely this case; per CONVENTIONS §489 overriding it is
a **NON-DEFAULT requiring an explicit manager ruling**, so I have not touched it. The alternative — re-exporting
`Sorcerer.fbx` 180° so the asset joins the fleet bake — would fix all three seams at once and cost this Blueprint's
`+90` (it would go back to `-90`), but it invalidates `SM_Sorcerer`, `SK_Sorcerer`, the four anim clips and the card
render. **Manager's call; I have deliberately made no move on either.**

---

## 5. Level hygiene

Four temporary actors were placed in the loaded level (`/Game/Maps/L_Arena`) purely for the §3/§4 captures and
**all four were removed**: `remove_from_scene` returned `true` ×4, and a follow-up `find_actors` sweep on `TMP_`,
`Sorcerer` and `SkeletalMeshActor` returns **`[]` on all three** — zero leftovers.

**`L_Arena` was NEVER saved.** It is left dirty-in-memory from the place/remove round trip, which is expected and
harmless — the net actor set is unchanged, and discarding it costs nothing. **Do not save it to "clean it up".**
Only `BP_Unit_Sorcerer` was saved, by explicit single-path list.

### 5a. ⚠️ One asset I did NOT touch reads dirty — reported, deliberately NOT saved

A post-run dirty sweep found **`/Game/Materials/Instances/MI_Sorcerer_PBR` = `is_dirty true`**. I never opened,
edited or saved it. I checked whether this was a general editor artifact and it is not: of the **21** `MI_*_PBR`
instances in `/Game/Materials/Instances`, **exactly 1 is dirty — this one** (`MI_Wizard_PBR`, `MI_Cleric_PBR`, and
the other 18 are all clean).

**It is derived state, not an edit — verified by readback rather than assumed.** The MI's authored content is
identical to TASK-371's recorded values: parent `/Game/Materials/M_AssetPBR`; `BaseColor` → `T_Sorcerer_D`;
`Normal` → `T_Sorcerer_N`; `ORM` → `T_Sorcerer_ORM`; dependencies exactly
`[M_AssetPBR, T_Sorcerer_D, T_Sorcerer_N, T_Sorcerer_ORM]`. **Nothing authored changed.**

**Most likely cause:** my §3/§4 viewport captures were the first time `SM_Sorcerer` / `SK_Sorcerer` were actually
*rendered in a level* in this editor session, so the material's shader map got cached onto the instance and marked
it dirty. Every other `MI_*_PBR` has been rendered in `L_Arena` many times already, which fits the 1-of-21 pattern.

**I did not save it** — it is outside this task's scope ("touch only this Blueprint"), and saving would hand
build-master an unexpected file change. **Recommendation: discard it (do not save, do not commit
`MI_Sorcerer_PBR.uasset`).** Flagging it only so nobody sees the dirty marker later and assumes TASK-375 edited it.
All other watched assets are clean: `BP_Unit_Sorcerer` · `BP_Unit_Footman` · `SM_Sorcerer` · `SK_Sorcerer` ·
`SK_Footman` · `MI_TeamColor_Blue` all `is_dirty false`. PIE `false`, zero asset editors open, editor never closed.

---

## 6. For build-master

- **ONE file to commit:** `Content/Blueprints/Units/BP_Unit_Sorcerer.uasset`
  (+ this note and the four `TASK-375-*.png` captures in `.claude/pipeline/handoffs/`).
- **No code, no C++, no Blueprint wiring, no CSV, no widget work needed for this task.** The spawn path already
  resolves the composed class string (§2) and `ASorcererUnit` already sets `CardID` in its constructor, so the BP's
  explicit `Sorcerer` is a deliberate, matching no-op.
- **Do not save `L_Arena`** when committing (§5).
- `DT_Cards` still needs its reimport — that is **TASK-376**, not this task.
- **Integration/verify pass should expect the §4 backwards skeletal facing** and treat it as the *known, flagged*
  state, not a new regression introduced here.

## 7. Not done, on purpose

- `SkeletalVisualMesh` transform — **C++-owned, forbidden** (§2, §4).
- `ABP_Sorcerer` — must never be authored; the shared `ABP_Footman` fallback is the design.
- The capsule resize — no ruling, gameplay-adjacent (§3b).
- Any Git command — none was run.
- Any fix to the three facing seams — flagged for a manager ruling (§4).


---
---

# SUPERSEDED IN PART - 2026-08-02 by **`TASK-375-facing-fix.md`** (read that first)

**Section 3 (`VisualMesh` yaw `+90`) and Section 4 (the three backwards facing seams) are CLOSED and no longer
describe the shipped state.** Nothing in this note was wrong at the time - the `+90` measurement and the Section 4
backwards-skeletal flag were both correct *for the asset as it then stood*, and flagging Section 4 rather than
silently hacking it is what made the source fix possible.

**What changed:** the defect was fixed **at the source**, not at any consumer. `Sorcerer.fbx` was re-exported with
`pre_rotate_z_deg = 180` (Stage 2, ~12 s, zero Meshy credits) so the asset conforms to the fleet, and the
downstream assets were re-landed.

| seam | this note recorded | **now shipped** |
|---|---|---|
| static `VisualMesh` yaw | `+90` | **`-90`** (fleet ritual restored; Z still `-88.0` = -(READ capsule half-height 88.0)) |
| `SkeletalVisualYawOffset` | C++ `-90`, "will march backwards" | **C++ `-90`, UNCHANGED - and now CORRECT.** Proven front-facing in the viewport |
| `GhostYawOffset` | C++ `-90`, "ghost shows the back" | **C++ `-90`, UNCHANGED - and now CORRECT** (same constant, same mesh, same fix) |

**No C++ was touched. No `SkeletalVisualYawOffset` exception hatch was authored. No law was amended** - the
manager ruling was to fix the geometry, not the consumers.

**ROOT CAUSE (Section 4's "recommended shape of the fix" was on the right track but the mechanism is different):**
it is not that this asset was authored 180 degrees off. It is the **TASK-348 `_ue_handedness_precomp` MIRROR-FIX
(2026-07-28)** - the Sorcerer is the **first UNIT exported after it**, so it lands in UE un-mirrored while the
entire pre-fix fleet lands mirrored. Full evidence in `TASK-375-facing-fix.md` Section 1.

**Still accurate and NOT superseded:** Section 2 (every other acceptance readback), Section 3b (capsule
half-height **88.0**, `VisualMesh.RelativeLocation.Z = -88.0`, and the note that `88/34` is the `ACharacter`
default and a mesh-matched capsule would be about 91/40 - **still an open manager call**), Section 5 (level
hygiene), Section 5a (`MI_Sorcerer_PBR` dirty-from-shader-map - it has since been discarded twice and is now
clean), Section 7.
