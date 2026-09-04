# TASK-946 — `BP_Unit_Witch`, the actor Blueprint — ART HANDOFF

**Agent:** art-director · **Date:** 2026-09-03 · **Status:** ready-for-integration
**Law:** ⭐⭐ `WITCH-§6` (the new ACTOR-BLUEPRINT row) · `WITCH-§5` (application clause) · ⭐⭐ `SC-§50` · `SC-§39.1` · `BP_` naming law
**Integration:** `TASK-949` · **Gate that must flip RED→GREEN:** `TASK-947`

---

## 1. WHAT SHIPPED — ⛔ EXACTLY ONE ASSET

| asset | path on disk | /Game/ path |
|---|---|---|
| actor Blueprint | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\Blueprints\Units\BP_Unit_Witch.uasset` | `/Game/Blueprints/Units/BP_Unit_Witch` |

⛔ **No mesh, no texture, no material, no re-bake, no `DT_Cards`, no `Source/**`, no `L_Arena`, no Git.**
⛔ **No `RawAssets` source file exists or should exist** — a Blueprint is authored in-engine, not exported from Blender. This is the one art deliverable with no `Content/RawAssets/` counterpart.

⭐ **The composed spawn path the code builds resolves:** `load_asset("/Game/Blueprints/Units/BP_Unit_Witch.BP_Unit_Witch_C")` → `{"refPath":"/Game/Blueprints/Units/BP_Unit_Witch.BP_Unit_Witch_C"}`. **That is the code contract in `ResolveCardActorClass`, exercised directly rather than inferred from the asset existing.**

---

## 2. ⭐⭐ ITEM (5) — THE READ-BACK. ⛔ EVERY LINE IS A **VALUE READ FROM THE SAVED ASSET**, ⛔ NOT A "DONE".

Every write returned `true`. **I accepted none of them.** After the compile *and* after the save I ran **separate, independent** read calls:

| contract | ⭐ VALUE READ BACK | instrument |
|---|---|---|
| the path | `/Game/Blueprints/Units/BP_Unit_Witch` → `exists` **`true`** | `AssetTools.exists` |
| the asset class | **`BP_Unit_Witch_C`** | `AssetTools.get_asset_class` |
| ⭐ **the composed spawn class** | **`/Game/Blueprints/Units/BP_Unit_Witch.BP_Unit_Witch_C`** — loads | `AssetTools.load_asset` |
| ⭐ **the parent class** | **`/Script/GitClaudeUnrealTest.SummonedUnit`** | `BlueprintTools.get_parent` |
| ⭐ **the component NAME** | **`VisualMesh`** (addressed as `…Default__BP_Unit_Witch_C.VisualMesh`; the read succeeded, so the name resolved) | `ObjectTools.get_properties` |
| the mesh reference | **`/Game/Meshes/SM_Witch.SM_Witch`** | same |
| ⭐ **the yaw** | **`{"pitch":0,"yaw":-90,"roll":0}`** | same |
| ⭐ **the Z** | **`{"x":0,"y":0,"z":-91}`** | same |
| the scale | `{"x":1,"y":1,"z":1}` | same |
| slot-0 authored override | **`[/Game/Materials/Instances/MI_TeamColor_Blue]`** | same |
| ⭐ **the capsule (the Z's source)** | **`CapsuleHalfHeight 91`, `CapsuleRadius 40`** | `ObjectTools.get_properties` on `CollisionCylinder` |
| `CardID` | **`Witch`** | `ObjectTools.get_properties` on the CDO |
| `Team` | `Blue` (inherited default — the fleet value) | same |
| `CardTableAsset` | `/Game/Data/DT_Cards.DT_Cards` (inherited) | same |
| `SkeletalVisualYawOffset` | `-90` (inherited, **not** hand-authored — see §4) | same |
| `SkeletalVisualMesh` | mesh `None`, loc `0,0,0`, rot `0,0,0`, `bVisible false` — **untouched** | same |
| dependencies | **`[/Script/GitClaudeUnrealTest, /Game/Materials/Instances/MI_TeamColor_Blue, /Game/Meshes/SM_Witch]`** | `AssetTools.get_dependencies` |
| event graph | `(event EventBeginPlay) (event Collision\|EventActorBeginOverlap) (event EventTick)` — the **stock disconnected stubs only**, byte-for-byte what `BP_Unit_Cleric` carries | `BlueprintTools.read_graph_dsl` |
| BP variables | **`[]`** | `BlueprintTools.list_variables` |
| referencers | **`[]`** — nothing points at it yet, which is correct: `TASK-949` integrates | `AssetTools.get_referencers` |

⚠️ **`Z = −CapsuleHalfHeight` is satisfied as an identity, not a coincidence: `−91` against a capsule I read as `91`.** The banned *"half-height == 90"* assumption was never used — see §3 for how 91 was derived.

**Compile:** `compile_blueprint` with **`warnings_as_errors: true`** → no error raised. Property values were re-read **after** the compile and were unchanged, so the compile did not reset the CDO.

---

## 3. ⭐⭐ HOW I VERIFIED IT MATCHES THE FLEET — ⛔ 13 BLUEPRINTS READ **BEFORE** I CREATED ANYTHING

I read every shipped `BP_Unit_*` out of the editor first. **Nothing below is a convention I invented; every number is a census.**

### 3.1 The parent-class census

| parent | units |
|---|---|
| **`ASummonedUnit`** | Footman, Archer, Knight, Pikeman, Cavalry, Longbowman, MilitiaMob, Ogre, Sapper, Cleric, Wizard — **11** |
| `ASorcererUnit` | Sorcerer — 1 |
| `AMinerUnit` | Miner — 1 |

⇒ **`ASummonedUnit` is the majority AND the law's value AND correct** (there is no `AWitchUnit`; the veil lives inside `ASummonedUnit`). **Read back as `/Script/GitClaudeUnrealTest.SummonedUnit`.**

### 3.2 The `VisualMesh` transform census — ⭐ THE RITUAL IS **13/13**, AND THE Z RULE IS **EXACT**

| unit | mesh Z-max (measured) | capsule half-height | `VisualMesh` Z | yaw | slot-0 override |
|---|---|---|---|---|---|
| MilitiaMob | 148.745 | 74.5 | **−74.5** | −90 | Blue |
| Sapper | 168.613 | 84.5 | **−84.5** | −90 | Blue |
| Miner | 172.916 | 86.5 | **−86.5** | −90 | Blue |
| Footman | 179.624 | 90 | **−90** | −90 | Blue |
| Archer | 179.919 | 90 | **−90** | −90 | Blue |
| **Cleric** | **181.886** | **91** | **−91** | −90 | Blue |
| Sorcerer | 181.882 | 88 *(engine default)* | **−88** | −90 | Blue |
| Wizard | 183.249 | 88 *(engine default)* | **−88** | −90 | **`[]` ⚠️** |
| Longbowman | 183.958 | 92 | **−92** | −90 | Blue |
| Knight | 188.915 | 95 | **−95** | −90 | Blue |
| Pikeman | 189.657 | 95 | **−95** | −90 | Blue |
| Cavalry | 208.029 | 104 | **−104** | −90 | Blue |
| Ogre | 289.496 | 145 | **−145** | −90 | Blue |
| ⭐ **Witch (shipped)** | **181.895** | **91** | **−91** | **−90** | **Blue** |

- ⭐ **`VisualMesh.Z == −CapsuleHalfHeight` holds 13/13, exactly, with no rounding slack.**
- ⭐ **yaw `−90` holds 13/13.** No exception hatch taken (`TASK-833` §5 measured that her Blender `pre_rotate_z_deg = 180` **is** the compensation).
- ⭐ **The capsule rule, derived from the 11 units that author one: `CapsuleHalfHeight = mesh-Z-max / 2`, snapped to the nearest 0.5.** Check: MilitiaMob 74.37→74.5 · Sapper 84.31→84.5 · Miner 86.46→86.5 · Footman 89.81→90 · Archer 89.96→90 · Cleric 90.94→**91** · Longbowman 91.98→92 · Knight 94.46→95 · Pikeman 94.83→95 · Cavalry 104.01→104 · Ogre 144.75→145. **11/11.**
  ⇒ **Witch: `181.89451599 / 2 = 90.947` → `91`.** ⛔ **Not assumed — computed from the mesh bounds I read off `SM_Witch` in the editor**, and it lands on the identical value as the **Cleric, whose mesh is within 0.01 uu of hers.**
- **Radius `40`** — the fleet's dominant value (**7 of the 11** that author one: Footman, Archer, Pikeman, Longbowman, Sapper, Cleric, Miner), and the Cleric's exact value. ⭐ **Radius is deliberately NOT derived from bounds, and the fleet proves it: Pikeman's mesh is 95.9 wide (a pike) and gets 40; Longbowman's 59.0 (a bow) gets 40; MilitiaMob's 59.3 gets 30.** Props and overhangs are excluded fleet-wide ⇒ **the Witch's 59.6 half-extent is her HAT BRIM and is correctly excluded too.** A brim-derived radius would have made her wider than the Ogre.

### 3.3 ⛔⛔ A MEASURED CORRECTION TO THE ROW I WAS DISPATCHED WITH — ⛔ PLEASE PROPAGATE IT

> The board and `TASK-833` §8 item 3 both say: *"`BP_Unit_Wizard` is the ONE unit that MISSED this ritual and sits at **yaw 0 / Z 0**."*

⛔ **That is FALSE as of today's tree, and I did not take it on trust — I read the asset.** `BP_Unit_Wizard`'s `VisualMesh` reads **`Z = −88`, `yaw = −90`** — **it obeys the ritual**, with `−88` correct against its own un-overridden capsule of `88`.

✅ **Wizard's REAL deviation is a different one, and I found it only because I censused all thirteen: `OverrideMaterials` is `[]` — it is the ONLY unit of the 13 with no authored slot-0 `MI_TeamColor_Blue`.** (Harmless at runtime — `ApplyTeamMaterial` writes slot 0 at `BeginPlay` regardless — but it means the Wizard renders untinted in the editor viewport and in any preview that does not run `BeginPlay`.)

⚠️ **Two secondary Wizard/Sorcerer notes:** both sit at the **engine `ACharacter` default capsule `88/34`**, i.e. they are the two units that never authored one. Against their own meshes (181.9 / 183.2) the fleet rule would give **91 / 91.5**, so both capsules are ~3 uu short and their heads poke above the capsule. ⛔ **I did not touch them** — out of fence, and flagged here rather than fixed.

⚖️ **Why this matters beyond pedantry:** the row told me to avoid the Wizard *as a template because of a defect it does not have*, while the defect it **does** have is exactly the property I was about to copy 12 other units on. **`SC-§38`'s lesson in a new costume: a stale claim in a dispatch is an unchecked assertion, and re-measuring it is what turned a warning into a finding.**

### 3.4 `SkeletalVisualMesh` — ⛔ DELIBERATELY UNTOUCHED, and the fleet is SPLIT on this

`SummonedUnit.h`'s `SkeletalVisualYawOffset` doc is explicit: ***"A `BP_Unit_<Unit>` MUST NOT hand-author `SkeletalVisualMesh` rotation"*** — `ResolveSkeletalVisual` **overwrites** the yaw from the C++ constant, and an authored value is at best redundant.

Measured: **Sorcerer and Wizard are at `0,0,0` / `0,0,0` (law-compliant)**; **Cleric and Footman carry a hand-authored `Z −90 / yaw −90` (legacy, pre-`TASK-326/327`)**.
⇒ ⭐ **I left the Witch's completely untouched.** She is compliant with the *current* law, not with the legacy pattern. **This is the one place I deliberately did NOT copy the Cleric**, and this paragraph is why.

### 3.5 Structural parity, checked rather than assumed

- **Dependency set is shape-identical to the Cleric's:** Cleric `[/Script/GitClaudeUnrealTest, MI_TeamColor_Blue, SM_Cleric]` vs Witch `[/Script/GitClaudeUnrealTest, MI_TeamColor_Blue, SM_Witch]`.
- **Event graph identical** to the Cleric's (three stock disconnected stubs, no logic). **Zero BP variables.**
- ⛔ **I created it fresh from `ASummonedUnit` rather than duplicating a fleet member — on purpose.** Duplicating the Cleric would have inherited its **law-violating hand-authored `SkeletalVisualMesh` rotation** (§3.4); duplicating the Sorcerer would have landed the **wrong parent** (`ASorcererUnit`). A fresh create is the only route where the *only* overrides on the asset are the ones I can name.
- `find_assets` on `/Game/Blueprints/Units` now returns **14** `BP_Unit_*`, the Witch among them. ⛔ Before creation it returned 13 and `exists` on the Witch was **`false`** (negative control), while `exists` on `BP_Unit_Sorcerer` was **`true`** (positive control).

---

## 4. ⭐⭐ DOES THE SLOT LAYOUT MATCH WHAT THE VEIL WIRING ITERATES? — ✅ **YES, AND I TRACED IT RATHER THAN ASSUMING IT**

**The question is not "does my BP have 2 slots" — it is "what number does `ApplyVeilMaterial`'s loop bound actually evaluate to on my component".** `TASK-923` ships:

```
const int32 SlotCount = ActiveMesh->GetNumMaterials();
for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex) { ActiveMesh->SetMaterial(SlotIndex, ResolvedVeil); }
```

Traced, link by link:

1. **`GetActiveVisualMesh()` returns my `VisualMesh`, not the skeletal one.** It returns `SkeletalVisualMesh` only when `bUsingSkeletalVisual && SkeletalVisualMesh`, and `bUsingSkeletalVisual` latches only inside `ResolveSkeletalVisual` when `/Game/Characters/SK_Witch` resolves. **`SK_Witch` does not exist** (`TASK-833` §6, still true). ⇒ the veil paints **`VisualMesh`**.
2. ⭐ **`GetNumMaterials()` reads the MESH ASSET's slot count, not my override array.** Read from the engine source rather than recalled — `Engine/Source/Runtime/Engine/Private/Components/StaticMeshComponent.cpp:2858`:
   ```
   int32 UStaticMeshComponent::GetNumMaterials() const
   {
       // @note : you don't have to consider Materials.Num()
       if(GetStaticMesh()) { return GetStaticMesh()->GetStaticMaterials().Num(); }
       else { return 0; }
   }
   ```
3. **`SM_Witch`'s slots, read back from the asset: `["TeamRegion", "WitchPBR"]`** — `TeamRegion` → `MI_TeamColor_Blue`, `WitchPBR` → `MI_Witch_PBR`.

⇒ ⭐ **`SlotCount == 2`. The loop runs `i = 0` and `i = 1`. BOTH slots are painted — `WITCH-§5`'s application clause is satisfied, and the *"opaque chrome hat floating over a ghostly body"* cannot occur.**

**Two things this trace establishes that a slot count alone would not:**

- ⛔⛔ **My BP's single-entry `OverrideMaterials` array does NOT shrink the loop.** This was the live risk: I author **one** override (slot 0) while the mesh has **two** slots, and if `GetNumMaterials()` had been `OverrideMaterials.Num()` the veil would have painted **slot 0 only** — leaving her `MI_Witch_PBR` **body fully opaque under a ghostly hat**, i.e. the law's failure inverted. **It does not, and line 2860's own comment says why.**
- ⛔ **The mesh binding is load-bearing for the VEIL, not only for the look.** `GetNumMaterials()` returns **0** on a null `StaticMesh` ⇒ a Witch BP with an unbound mesh would run the veil loop **zero times, paint nothing, and log nothing** — `ApplyVeilMaterial` would return normally and `GrantInvisibility` would still report success (`TASK-923` §4e, deliberately). **A silent no-op that reviews as a pass.** My binding is read back as `/Game/Meshes/SM_Witch.SM_Witch`.

**The restore direction also checks out.** `ClearVeilMaterial` sets every slot to `nullptr` — which destroys my authored `OverrideMaterials[0]` at runtime — then calls the shipped `ApplyTeamMaterial()`, which writes `MI_TeamColor_<Team>` back to slot 0 from the **current** `Team`. Slot 1 falls through to the **asset's** `MI_Witch_PBR`. ⇒ **the authored Blue override is an editor-preview convenience only; its runtime destruction is immediately and correctly overwritten.** ⛔ **I did not hand-assign a team colour anywhere, did not reorder, did not blank.**

---

## 5. PROOF OF SAVE — ⛔ SHA256 + BYTES ON DISK, ⛔ NOT A RETURN VALUE

| step | evidence |
|---|---|
| before | `Test-Path …\BP_Unit_Witch.uasset` → **`False`**. `git log --all` on the path was empty in `STACK-BUGS` §2.2 — **never tracked in any commit, ever** |
| `save_assets(["/Game/Blueprints/Units/BP_Unit_Witch"])` | returned `true` — ⛔ **recorded, not trusted** |
| after | **sha256 `b4f375305aea3ea812e25fb0e30bfabd39f82d21a64fe52b02298c3223fec2ab`**, 39,609 bytes, `LastWriteTime 2026-09-03 19:02:41` |

⚠️ **Stated precisely rather than inflated:** the transition I can prove is **ABSENT → PRESENT**, which is a genuine machine-checkable state change with a definite before-state — **not** a same-file hash *delta*. I did not manufacture a throwaway edit to produce one.

**So I corroborated the bytes independently.** Binary probe with `grep -a` (⛔ **`strings` is not installed in this shell and prints empty for every file** — `STACK-BUGS` §1.3's recorded instrument defect; I did not repeat it):

| needle | in `BP_Unit_Witch.uasset` | in `BP_Unit_Sorcerer.uasset` (cross-control) |
|---|---|---|
| `SM_Witch` | **2** | **0** |
| `SM_Sorcerer` | **0** | **2** |
| `MI_TeamColor_Blue` | **2** | — |
| `SummonedUnit` | **8** | — |
| `SorcererUnit` | — | **8** |
| `VisualMesh` | **2** | — |
| `CollisionCylinder` | **1** | — |
| `Nonexistentassetxyz` (negative) | **0** | **0** |

⇒ ⭐ **the wiring is in the bytes on disk, not merely in editor memory**, and the instrument is shown able to return both non-zero and zero on the same needles.

⚠️ **`is_dirty` returned `false`.** ⛔ **Reported, gated on nothing** — the standing warning is that this instrument returns `true` for every extant asset, so a `false` here is unexplained and I treat it as noise, not evidence.

⚠️ **LFS — verified by OID-vs-SHA256, ⛔ never by size.** `git check-attr filter` → `lfs`. The pointer blob reads `oid sha256:b4f375305aea3ea812e25fb0e30bfabd39f82d21a64fe52b02298c3223fec2ab` — **identical to the working-tree sha256 above.** (Size 39609 matches too, but the oid is the proof.)

**Pixel check.** I opened the Blueprint editor and captured the running editor: the witch stands **upright and grounded on the capsule**, her **wide flat brim solid team-blue** over the dark PBR body, header reads **"Parent class: Summoned Unit"**, the Details panel shows `VisualMesh` Z **−91** and `SkeletalVisualMesh` at all zeros, and the status bar reads **"All Saved"**. Re-hashing after opening the editor returned the **identical** sha256 ⇒ opening it mutated nothing.

---

## 6. ⛔⛔ TWO THINGS FOR THE BUILD-MASTER, ONE OF THEM URGENT

### 6.1 ⛔ THE FILE ARRIVED **ALREADY STAGED**, AND ⛔ IT WAS NOT ME

`git status` reports **`A  Content/Blueprints/Units/BP_Unit_Witch.uasset`** — staged-added. ⛔ **I ran no writing Git command; every Git call I made was read-only (`status`, `ls-files`, `cat-file`, `check-attr`).** The editor's **Revision Control provider auto-added it on save** (the status bar shows a live "Revision Control" connection, and the session-start tree already carried other agents' assets in the same staged state).

⇒ 🔧 **Build-master: this is pre-staged state you did not create either. Confirm it against your own `git status` before you build a pathspec** (`SC-§40` cl. 1) — **and be aware the same provider may have staged other lanes' assets.** ⛔ I did not unstage it; that would itself be a Git write.

### 6.2 My total `Content/` footprint is **exactly one file**

`git status --short` shows **no other `Content/` change** attributable to me. Specifically:
- ⛔ **`L_Arena` was never opened, never modified, never saved.** It was already the loaded level when I arrived (from a prior session) and both status bars read "All Saved". ⛔ **No restore prompt and no save prompt was presented to me, so none was accepted.**
- ⛔ **`BP_FogArea` untouched.** `Content/FogArea/` shows as untracked but **every file in it timestamps `2026-09-02 20:04`** — a full day before this session. ⛔ Not mine; verified by mtime rather than asserted.
- ⛔ **`save_assets` was called ONCE, with exactly one explicit path. ⛔ Never an empty list.**

### 6.3 Integration notes

1. **Pivot / scale:** inherited fleet convention. `VisualMesh` at `Z −91`, yaw `−90`, scale 1. Capsule `91 / 40`. Feet land on the capsule bottom; ⛔ no special handling.
2. **Material slots:** `[TeamRegion, WitchPBR]`, in that order, **on the mesh asset — the BP does not restate them.** Slot 0 is the `BeginPlay` team-recolour target and works unchanged; ⛔ **no code needed.**
3. ⭐ **The `TASK-947` roster gate should now flip RED → GREEN on the `Witch` row, and that transition is the real integration proof.** ⛔ If it stays red, the fault is on the composer side, not the asset side — the class path is proven to load (§1).
4. ⛔ **Nothing else in the Witch chain needs anything from me.** Full editor sweep for "Witch" returns exactly: `T_Witch_D`, `T_Witch_N`, `T_Witch_ORM`, `MI_Witch_PBR`, `SM_Witch`, `T_CardArt_Witch`, and now `BP_Unit_Witch`. **The three extra hits are engine/plugin assets matching "S-witch"/"RT-Switch" as a substring** (`MF_Switch4_Vec2`, `MF_Switch4_Vec3`, `MF_RTSwitch`) — the identical three `TASK-833` recorded.

---

## 7. 🧑 WHAT NEEDS JONATHAN'S EYE

1. ⭐⭐ **The one thing only a live match can settle: does she read as a witch at gameplay camera distance, standing next to the Sorcerer and Cleric?** I verified silhouette distinctness on the concept, on four Stage-2 previews, and now in the BP viewport — ⛔ **never in PIE at real distance among real units.** MCP has no input lane and I did not start PIE.
2. ⚠️ **She spawns STATIC and UNANIMATED, and her 3-second interruptible cast therefore has NO visual tell.** ⛔ Declared, ⛔ not introduced by me — `TASK-833` §6 / `WITCH-§9`, and `WITCH-§9.0` records that **he already ruled the rig+animation is the shipped tell** and cancelled the cast bar. **`SK_Witch` still does not exist.** ⛔ Out of scope for this row; naming it so it is not mistaken for something this Blueprint fixed.
3. **The dark-chrome bake (95.2% metallic) and the 18% hat-brim team region are `TASK-899` / `TASK-833` §7b — his ruling, ship-as-is by default.** ⛔ I changed neither, and §4 shows the 18% brim is now *correctly* covered by the veil rather than left opaque.
4. 🔧 **A capsule question he may want to rule on, ⛔ NOT a blocker:** the Witch ships at `91 / 40` by the fleet's own measured rule. **Sorcerer and Wizard sit at the engine default `88 / 34`** and are the only two that never authored one — ⇒ **the Witch is now 3 uu taller and 6 uu wider in the capsule than the two casters she stands next to.** ⛔ **I matched the RULE (11/11) rather than the two units that skipped it**, and I am flagging it rather than quietly picking. If he prefers caster-group uniformity, the lever is `CapsuleHalfHeight` + `CapsuleRadius` on this BP, and **`VisualMesh.Z` must move with it** — they are one number.

---

## 8. 📌 MEASURED FOLLOW-UPS — ⛔ NAMED, ⛔ NOT BOARDED, ⛔ NOT BUILT

- ⛔ **`BP_Unit_Wizard` has no authored slot-0 `MI_TeamColor_Blue`** — the only one of 14. Cosmetic (editor/preview only). §3.3.
- ⛔ **`BP_Unit_Sorcerer` and `BP_Unit_Wizard` never authored a capsule** and sit ~3 uu short against the fleet rule. §3.3.
- ⛔ **`BP_Unit_Cleric` and `BP_Unit_Footman` hand-author `SkeletalVisualMesh` rotation**, which `SummonedUnit.h` explicitly forbids. Inert today (`ResolveSkeletalVisual` overwrites the yaw), but it is exactly the kind of authored-value-that-looks-authoritative the header warns about. §3.4.
- ⛔ **`WITCH-§6`'s new row and `TASK-833` §8 item 3 both carry the stale *"Wizard sits at yaw 0 / Z 0"* claim.** ⇒ 📌 **a `CONVENTIONS.md` amendment is owed** — the sentence is currently *false against the tree* and would send the next reader looking for the wrong defect. **Manager's row, ⛔ not a self-serve edit by me.**

⛔ **I did not board any of these and I did not fix any of them.**
