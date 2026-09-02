# TASK-756 — [GHOST-5] the ghost's VISUAL — art-director handoff

**Status:** `ready-for-integration` **for the material**; ⛔ **`BP_SiegeGhostPawn` is HARD-BLOCKED** (see §5).
**Law:** `GHOST-§3` `G-1`/`G-4` · `GHOST-§4` · `GHOST-§5` · `SC-§35`
**Suite delta: ⛔ ZERO.** Pure asset authoring, ⛔ no tests added. Stated explicitly so TASK-753/754's one-number reconciliation is ⛔ not thrown.
⛔ No C++, ⛔ no compile, ⛔ no Git, ⛔ no PIE, ⛔ no `.umap`, ⛔ `SiegeGhostPawn.{h,cpp}` untouched, ⛔ `M_Ghost` untouched (verified `is_dirty == false` **after** all my work).

---

## ⭐ THE HEADLINE: ⛔ NOTHING WAS MODELLED. Zero geometry, zero Blender, zero credits.

`G-1`'s *"similar to their original body"* is satisfied **literally** — the ghost wears the hero's **own** shipped mesh. Measured, ⛔ not assumed:

| what | measured value | where |
|---|---|---|
| the hero's body | **`/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple`** | `BP_HeroCharacter.uasset` hard-refs; corroborated by `handoffs/TASK-009.md:52` |
| set in C++? | ⛔ **no** — `HeroCharacter.{h,cpp}` contain **zero** mesh/material code | `GitClaudeUnrealTestCharacter.cpp:49-50` states the BP-owns-the-mesh contract |
| its material slots | exactly **2** — `Quinn_01`, `Quinn_02` | read back off the SkeletalMesh asset |
| skeleton | `SK_Mannequin` (⛔ **not** the `SK_Footman_Skeleton` unit family) | — |

⇒ ⭐ The hero is the **stock UE5 mannequin**, ⛔ not a Siegebound-authored character. A bespoke ghost sculpt would have been pure waste. **The answer was a material, and only a material.**

---

## 1. THE SHIPPED ASSET — ⭐ ONE new material instance, at the board's PINNED name

**`/Game/Materials/MI_Ghost_Translucent`** — file `Content/Materials/MI_Ghost_Translucent.uasset`
Read back: class `MaterialInstanceConstant`, `is_dirty == false`, dependencies = **exactly** `[/Game/Materials/M_HeroSpirit]`.

**Look (render: `handoffs/TASK-756-A-shipped-MI_Ghost_Translucent.png`):** an icy translucent body you can see the background *through*, wrapped in a bright pale-cyan **rim glow** on the silhouette. Unlit, so it is legible in the dark moonlit arena at range — which is what `G-4` actually asks for.

| param | value | why |
|---|---|---|
| `TeamColor` | **(0.45, 0.78, 1.00)** cold spectral cyan-white | ⛔ **NEUTRAL — no team tell**, per board item (4). Deliberately ⛔ **not** green `(0,1,0)` / red `(1,0,0)`: those are the **placement ghost's** valid/invalid states (`SiegePlayerController.h:1541-1547`) and a spirit must never read as a placement state. Also ⛔ not the living hero's grey. |
| `CoreOpacity` | 0.16 | interior is ~16 % opaque ⇒ **you see the arena through the body**. This is the "not alive" cue that survives at distance. |
| `RimPower` / `RimBoost` | 4.0 / 6.0 | tight, bright silhouette edge — the read that survives when the figure is small on screen |
| `RimOpacityBoost` | 0.85 | firms the outline so the shape stays legible |
| `CoreWhiten` | 0.35 | pale spectral core |
| `PulseSpeed` / `PulseAmount` | 0.6 / 0.12 | slow ±12 % **glow** breathe (house precedent: TASK-226, `M_AncientGround`). ⭐ **Pulses EMISSIVE ONLY, ⛔ never opacity** — a pulsing opacity would flicker the ghost toward invisibility and put `G-4` at risk on a timer. |

---

## 2. ⚠️⚠️ THE BOARD'S OWN ESCAPE CLAUSE FIRED — and it fired on MEASUREMENT, not on preference

Board item (2): *"Parent it to a shipped translucent-capable master; **if none exists, ⛔ SAY SO IN THE HANDOFF**."*

✅ **I am saying so. There is NO shipped translucent master that can be applied to a SKELETAL MESH. I measured every one:**

| master | domain | blend | `bUsedWithSkeletalMesh` | verdict |
|---|---|---|---|---|
| `M_Ghost` | MD_Surface | Translucent | ⛔ **false** | ⛔ **fails on a skeletal mesh** |
| `M_CaptureZone` | **MD_DeferredDecal** | Translucent | false | ⛔ structurally impossible on a mesh |
| `M_CenterlineStripe` | **MD_DeferredDecal** | Translucent | false | ⛔ same |
| `M_SpellReticle` | **MD_DeferredDecal** | Translucent | false | ⛔ same |
| `M_AncientGround` | **MD_DeferredDecal** | Translucent | false | ⛔ same |
| `M_AssetPBR` / `M_TeamColor` | MD_Surface | ⛔ **Opaque** | true | ⛔ opaque cannot make a ghost |

⇒ **Four of the five translucent masters are decals. The fifth (`M_Ghost`) is `bUsedWithSkeletalMesh = false`.**

### ⚠️ AND I BUILT THE `M_Ghost`-PARENTED VERSION FIRST, SO THIS IS EVIDENCE, ⛔ NOT AN ARGUMENT
`handoffs/TASK-756-B-rejected-parented-to-M_Ghost.png` is the real render. It is a **flat, solid-looking pale disc — the checkerboard does NOT show through.** Two measured causes: `M_Ghost` is a 2-node material (`GhostColor`→Emissive, `GhostOpacity`→Opacity, ⛔ no fresnel, ⛔ no depth) and it is `TwoSided = true`, so front+back composite to ~solid. **On a humanoid that reads as a hero painted light blue — i.e. ALIVE.** That is the exact failure my brief names: *if it reads as alive, enemies waste attacks on it and the design reads as a bug.*

⚠️⚠️ **AND THE HARDER DEFECT, WHICH IS A SHIPPING HAZARD, NOT A TASTE CALL:** `bUsedWithSkeletalMesh = false` means the shader permutation for skinned geometry is **not compiled**. In the editor UE often auto-sets the flag and dirties the asset; **in a packaged build it falls back to the DEFAULT MATERIAL.** The project ships packaged zips. ⇒ parenting to `M_Ghost` risks a **grey default-material ghost in the shipped build only** — invisible in every editor check. This is the same silent-asset-defect class as the ORM-as-sRGB precedent.
⛔ **I did NOT "fix" `M_Ghost` by setting its flag** — it is TASK-012's shipped placement-preview contract, ⛔ not in my names list, and flipping it would force a shader recompile of a shipped system.

### ⇒ WHAT I DID, AND THE ⚖️ RULING I OWE THE MANAGER
✅ **The board's PINNED NAME is preserved exactly** — `/Game/Materials/MI_Ghost_Translucent` is the shipping asset and the cross-task contract does ⛔ not drift.
⚠️ **Its PARENT is a new master I authored: `/Game/Materials/M_HeroSpirit`.** Board item (2) says a new master *"is a scope change and it is the manager's call."*

⚖️ **MANAGER: RULE ON `M_HeroSpirit`.** It is one word either way.
- **Keep** ⇒ nothing further to do; the shipped instance already points at it.
- **Refuse** ⇒ then `G-4` cannot be met on a skeletal mesh by any shipped asset, and the fallback is to set `bUsedWithSkeletalMesh = true` on `M_Ghost` (a write to TASK-012's asset, which is **not mine to make**) and accept the flat solid look.

**`M_HeroSpirit` spec** — `Content/Materials/M_HeroSpirit.uasset`, read back after save:
`MD_Surface` · `BLEND_Translucent` · `MSM_Unlit` · **`bUsedWithSkeletalMesh = true`** · `TwoSided = false` (single-sided halves translucent overdraw and the front/back sorting mush on a skinned body).
⭐ **STOCK NODES ONLY — the Custom-HLSL BAN (`CONVENTIONS:3491`) is honoured; there is no Custom node in the graph.** 25 nodes: `Fresnel` → drives both a `Lerp` (pale core → saturated rim) and the opacity ramp; emissive = colour × (CoreGlow + rim×RimBoost) × a `Sine(Time)` pulse.
9 parameters: `TeamColor` (vector) + `CoreWhiten`, `RimPower`, `CoreGlow`, `RimBoost`, `CoreOpacity`, `RimOpacityBoost`, `PulseSpeed`, `PulseAmount`.
Shader **compiles** (`recompile` raises on failure and did not raise).

---

## 3. ⚖️ TEAM COLOUR — ⛔ NOT SHIPPED. Flagged exactly as board item (4) instructs.

⚠️ **My dispatch brief asked for a team tell; the BOARD forbids one here.** The board is the contract, so **the shipped `MI_Ghost_Translucent` carries ⛔ NO team colour.** Flagging instead of shipping, per item (4).

### ⭐ THE MEASUREMENT THE RULING NEEDS — the hero has ⛔ NO body team tint at all
- `AHeroCharacter` has **zero** `SetMaterial` / team-material code. `BP_HeroCharacter` hard-refs **no** `/Game/Materials/` asset. **A Blue hero and a Red hero render identically** (`MI_Quinn_01/02`).
- The hero conveys team by exactly **two** channels: the **overhead health-bar fill** (`CombatantHealthBarComponent`, `BlueBarColor`/`RedBarColor`) and floating **damage numbers** (`USiegeFeedbackLibrary::TeamTint`).
- ⚠️⚠️ **`GHOST-§5` forbids the ghost `IHealthBarProvider` ("it has no health to show"), and the ghost cannot take damage ⇒ no damage numbers either.** ⇒ ⭐ **THE GHOST LOSES BOTH OF THE HERO'S TEAM CHANNELS.** There is no "follow the hero's pattern" available — the hero's pattern is a health bar the ghost is not allowed to have.
- ⇒ **If two ghosts are ever on screen, they are visually identical.** Today that is latent (one human controller); **`GHOST-§6` reserves `GhostTeam` for M8, where both players ghost** and `HandleHeroDied` is already per-controller.

### The two proposal assets — ⛔ UNWIRED, referenced by NOTHING, shipped by nothing
`/Game/Materials/Instances/MI_HeroSpirit_Blue` — `TeamColor` **(0.05, 0.30, 1.00)**
`/Game/Materials/Instances/MI_HeroSpirit_Red` — `TeamColor` **(1.00, 0.10, 0.05)**
⭐ Exact house linear values (`CONVENTIONS:114`, `SiegeFeedbackLibrary::TeamTint`) — ⛔ not invented. Renders: `TASK-756-C-flagged-team-blue.png` / `TASK-756-D-flagged-team-red.png`; they are unmistakable at 256 px, a fair proxy for a small on-screen figure.

📌 **Disclosure:** I authored these **before** reading board item (4), which I regret. They are **inert** (`get_referencers` = `[]` on both). I have **not deleted them** for one reason: something outside my lane has already **staged** them in git (they show `A` in `git status`, and ⛔ I do not touch Git), so deleting the files now would hand build-master a staged-but-missing index entry. ⇒ **build-master can drop both files and their index entries cleanly in one operation if the manager rules against them.**

⚖️ **If the ruling is "add team colour":** it is **one line**, and the shipped house idiom already exists — `ACaptureZone` does exactly this (`CaptureZone.cpp:74` `CreateDynamicMaterialInstance` + `:257` `SetVectorParameterValue`). `M_HeroSpirit` exposes the vector param named **`TeamColor`** precisely so that fix is a copy of shipped code.

### ⚠️⚠️ AND A REAL ORDERING BUG THE FIX MUST AVOID — found while measuring, ⛔ not fixed by me (TASK-749's file)
`ASiegeGhostPawn::BeginPlay` applies `GhostMaterial`. `InitializeGhost(Team)` is called **after** `SpawnActor` per TASK-749's own API note — and **`BeginPlay` fires inside `SpawnActor`.**
⇒ ⛔ **`GhostTeam` is still its default (`Blue`) while the material is being applied.** **Any team tint written in `BeginPlay` would be silently wrong for Red, 100 % of the time.** The tint must live in **`InitializeGhost`**, ⛔ never `BeginPlay`.
⚠️ Note also that `BeginPlay` writes `GhostMaterial` to **every slot** (`SiegeGhostPawn.cpp:245-249`), which **destroys** the two-slot `[TeamRegion, …PBR]` idiom the rest of the project recolours through — so the slot-0 swap used by units/castles is **not** available here. A MID on the whole body is the correct shape.

---

## 4. ⛔ WHAT `ASiegeGhostPawn` MUST BE POINTED AT — the three slots, exactly

Assign on `BP_SiegeGhostPawn` (⛔ build-master / TASK-750's wiring, ⛔ not mine — and see §5, it cannot exist yet):

| slot | value | verified |
|---|---|---|
| `GhostMesh` | **`/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple`** | exists, class `SkeletalMesh` |
| `GhostMaterial` | **`/Game/Materials/MI_Ghost_Translucent`** | exists, class `MaterialInstanceConstant` |
| `GhostIdleAnimation` | **`/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle`** | exists, class **`AnimSequence`** ✅ |

⭐ **`MM_Idle` is a `UAnimSequence`, ⛔ NOT an Anim Blueprint** — exactly what `SC-§35`/`GHOST-§4` require for single-node playback, and it rides the `SK_Mannequin` skeleton that `SKM_Quinn_Simple` uses, so it will bind. ⛔ Do **not** substitute `ABP_Unarmed` (an Anim BP) or `BS_Idle_Walk_Run` (a BlendSpace needing drive values).

---

## 5. ⛔⛔ BLOCKED — `BP_SiegeGhostPawn` CANNOT BE CREATED YET (board item 5)

**`ASiegeGhostPawn` does not exist in the running editor.** Measured:
- `search_subclasses(APawn, "Ghost")` → **`[]`**
- `search_subclasses(APawn, "Siege")` → **`[]`**
- ⭐ **Anti-vacuity self-check — the instrument is proven live:** `search_subclasses(APawn, "Hero")` → **`[/Script/GitClaudeUnrealTest.HeroCharacter, /Game/Blueprints/BP_HeroCharacter.BP_HeroCharacter_C]`**.

⇒ The empty result is a **real negative**: TASK-749's class is written but **not compiled into the editor module**, because **TASK-754 owns this batch's single compile** and it serializes behind the ladder wave. **A Blueprint cannot be parented to a class the editor does not have.**
⛔ This is ⛔ **not** a mistake by anyone — it is the batch's deliberate compile ordering. But it means the board's *"blocked-by: NOTHING"* was correct for the **material** and **not** for item (5).

✅ **This is non-fatal by TASK-749's own design:** `GhostPawnClassAsset` falls back to the raw C++ class, and all three appearance soft-refs are null-safe ⇒ a missing BP is a **degraded-but-alive** ghost that warns, ⛔ never a crash.
⇒ **`BP_SiegeGhostPawn` + the three assignments must be re-boarded as a small follow-up AFTER TASK-754's compile.** All three target assets exist and are verified today, so it is pure wiring.

---

## 6. ⚠️ THE ACCEPTANCE I COULD ⛔ NOT MEET — stated plainly rather than glossed

Board item (3): *"It must read as a ghost FROM THE TOP-DOWN RTS CAMERA AT PLAY DISTANCE … ⛔ not in an asset-editor sphere preview."*

⛔ **I did not meet that bar, and I am not claiming I did.** What I have is **asset-editor sphere renders** — explicitly the instrument the board rejects. Why the real one was impossible **today**:
- The ghost pawn class **does not exist in the editor** (§5) ⇒ there is nothing to spawn and look at.
- **PIE is TASK-750's instrument** (`GHOST-§4`) and the editor is **in use by TASK-739 right now**; starting PIE would disrupt another agent's live work.
- Placing a preview actor would dirty `L_Arena`, and ⛔ **I never save a `.umap`.**

⇒ ⭐ **The play-distance judgement rides along with the PIE session `GHOST-§4` ALREADY OWES TASK-750** — that row *"may NEVER be waived on the grounds that the compile is clean."* **One extra instruction for whoever runs it: look at the ghost from the play camera and confirm it does not read as a living hero.** The tuning is all instance parameters, so any correction is a value change with **no** shader recompile.

---

## 7. Fences held — verified, ⛔ not asserted

⛔ `SiegeGhostPawn.{h,cpp}` · `SiegeGameMode` / `SiegePlayerController` · `SM_WatchTower` · `ABP_Footman` · `A_SiegeBiped_Climb` · any `.umap` — **none touched**.
⛔ `M_Ghost` **read only**; re-verified `is_dirty == false` and its params still `[GhostOpacity, GhostColor]` **after** all my work.
⛔ **`save_assets` was called with an EXPLICIT path list every time — ⛔ never the empty list**, which would have saved every dirty asset in the editor including another agent's live work and the level.
⛔ No Blender (never launched). ⛔ No Git. ⛔ No compile. ⛔ No `Content/RawAssets/` write (nothing was modelled, so there is no source FBX/PNG to check in — the `_D/_N/_ORM` texture law is **not applicable** to a stock-node material with no bake, the `M_Torch` PROP-CLASS precedent).

**Assets created (4 files):**
```
Content/Materials/MI_Ghost_Translucent.uasset            <- SHIPPED
Content/Materials/M_HeroSpirit.uasset                    <- its parent; ⚖️ awaiting the manager's item-(2) ruling
Content/Materials/Instances/MI_HeroSpirit_Blue.uasset    <- ⚖️ UNWIRED proposal (item-4 flag)
Content/Materials/Instances/MI_HeroSpirit_Red.uasset     <- ⚖️ UNWIRED proposal (item-4 flag)
```
