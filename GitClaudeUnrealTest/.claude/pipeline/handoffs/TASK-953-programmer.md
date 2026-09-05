# TASK-953 — `BP_Unit_Wizard`'s empty `OverrideMaterials`: does a shipped unit render untinted?

**Agent:** gameplay-programmer · **Date:** 2026-09-03 · **Status:** ready-for-qa
**Deliverable:** this document. ⛔ **THERE IS NO DIFF.** Zero writes to `Source/**`, `Content/**`, tests, law, or Git. No compile. No PIE. No save/restore prompt was presented or accepted.
**Law:** `SC-§40` cl. 3 + cl. 9 · `SC-§39` · `SC-§37` · `SC-§49` · input `handoffs/TASK-946-artist.md` §3.3

---

## 0. THE VERDICT — one of the three shapes the row named

> ### ✅ **`STRUCTURALLY COVERED` — `ApplyTeamMaterial` reaches it on every path.**

⛔ **NOT** `LIVE`. ⛔ **NOT** `INERT BY COINCIDENCE OF DATA`.

⭐⭐ **And the measurement went one step further than the row asked, in the direction that dissolves the premise: the empty array is not a deviation that happens to be harmless — it is the ONLY unit *not* carrying a redundant restatement of a value the mesh asset already holds. `SM_Wizard`'s own slot 0 IS `MI_TeamColor_Blue`. The other 13 BPs override slot 0 with the SAME material the slot already resolves to. Those 13 overrides are NO-OPS.**

⇒ 🧑 **WHAT A PLAYER SEES: NOTHING. No pixel differs, on any surface, in either team colour.** A Red-spawned Wizard reads red exactly like every other red unit. There is no wrong-colour, no-colour, or enemy-colour outcome. ⛔ **This is not a gameplay defect and not a cosmetic one.**

⛔ **RECOMMENDATION: NO REPAIR. Do not author the override.** Reasoning in §7.

---

## 1. ⛔ (2)(a) RE-MEASURED MYSELF — with the positive control on the SAME call (`SC-§39`)

⛔ I inherited nothing. Both rows below are values read out of the live editor (PID 40528) in one round trip, read-only.

| CDO property read | ⭐ **VALUE READ BACK** |
|---|---|
| `BP_Unit_Wizard…VisualMesh.OverrideMaterials` | **`[]`** — count **0** |
| `BP_Unit_Cleric…VisualMesh.OverrideMaterials` (⭐ **POSITIVE CONTROL**) | **`[/Game/Materials/Instances/MI_TeamColor_Blue]`** — count **1** |

⇒ ⭐ **The instrument returns BOTH answers on the same call. The `0` is a measurement, not an unreadable array** (`SC-§39`; the recorded `sha256`-join-prints-a-perfect-`0` and `git show`-prints-three-confident-zeros failures are the species this control exists to exclude).

**Instrument:** `ObjectTools.get_properties` on `/Game/Blueprints/Units/BP_Unit_<U>.Default__BP_Unit_<U>_C.VisualMesh`.

### 1.1 ⛔ The same call re-confirms the struck facing claim — the Wizard's facing IS fine

| | Wizard | Cleric |
|---|---|---|
| `RelativeRotation` | **yaw `−90`** | yaw `−90` |
| `RelativeLocation.Z` | **`−88`** | `−91` |
| `StaticMesh` | `/Game/Meshes/SM_Wizard` | `/Game/Meshes/SM_Cleric` |

⇒ ⛔ **`TASK-946` §3.3's correction is independently reproduced. I did not go looking for the wrong defect.**

---

## 2. ⛔⛔ THE MEASUREMENT THAT DECIDES IT — **THE MESH ASSET ALREADY CARRIES THE TEAM COLOUR**

⭐ **This is the question nobody in the chain asked, and it is the one that answers the row.** An override array is only meaningful relative to *what the slot resolves to without it.*

**Fleet census, all 14 units, read off the STATIC MESH ASSETS** (`StaticMeshTools.get_material_slots` + `get_material`):

| unit | mesh slots | mesh slot-0 material (⛔ **on the asset**) | BP `OverrideMaterials` |
|---|---|---|---|
| Footman | `[TeamRegion, FootmanPBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |
| Archer | `[TeamRegion, ArcherPBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |
| Knight | `[TeamRegion, KnightPBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |
| Pikeman | `[TeamRegion, PikemanPBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |
| Cavalry | `[TeamRegion, CavalryPBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |
| Longbowman | `[TeamRegion, LongbowmanPBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |
| MilitiaMob | `[TeamRegion, MilitiaMobPBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |
| Ogre | `[TeamRegion, OgrePBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |
| Sapper | `[TeamRegion, SapperPBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |
| Cleric | `[TeamRegion, ClericPBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |
| **Wizard** | `[TeamRegion, WizardPBR]` | ⭐ **`MI_TeamColor_Blue`** | ⛔ **0 · `[]`** |
| Sorcerer | `[TeamRegion, SorcererPBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |
| Miner | `[TeamRegion, MinerPBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |
| Witch | `[TeamRegion, WitchPBR]` | `MI_TeamColor_Blue` | 1 · `MI_TeamColor_Blue` |

⭐⭐ **14/14 — the mesh asset's slot 0 is `TeamRegion` and is assigned `MI_TeamColor_Blue` ON THE ASSET.**

⇒ ⛔⛔ **`UStaticMeshComponent::GetMaterial(0)` returns `OverrideMaterials[0]` when the entry is valid, ELSE the mesh asset's own material.** For the 13, both are the **same `UMaterialInterface*`**. For the Wizard, the fallthrough yields **the same pointer**. ⇒ ⭐ **IDENTICAL DRAW CALL. IDENTICAL PIXELS.**

⚖️ ***The 13 "authored team colours" do not colour anything. They restate a value the mesh already holds. The Wizard is not the unit missing a team colour — it is the unit not carrying the restatement.***

### 2.1 ⛔ THIS REFUTES A SENTENCE IN THE INPUT HANDOFF — recorded so it does not propagate

`handoffs/TASK-946-artist.md` §3.3 line 100 states the empty array *"means the Wizard renders untinted in the editor viewport and in any preview that does not run `BeginPlay`."*

⛔ **MEASURED FALSE.** The editor viewport renders the **mesh asset's** materials, and `SM_Wizard`'s slot 0 is `MI_TeamColor_Blue`. **The Wizard renders team-blue in the viewport exactly like the other 13.**

⚖️ ⭐ **This is the SAME species the artist itself caught one paragraph earlier, one turn later: a confident, reasonable, unmeasured consequence attached to a correctly-measured fact.** The *finding* (empty array, only one of 14) is **exactly right** and was worth the row; the *consequence* was inferred from the array alone without reading what the slot falls back to. ⛔ **No blame attaches — it was flagged, not fixed, and boarded as a question rather than a repair, which is precisely why the wrong half cost nothing.**

---

## 3. ⛔ (2)(b)+(c) THE RUNTIME TRACE — BY SYMBOL, AND **BOTH BRANCHES OF THE FORK ARE COVERED**

### 3.1 ⭐⭐ (2)(c) THE FORK ANSWER: **`SK_Wizard` DOES RESOLVE TODAY.**

Measured **three independent ways**, with controls in both directions:

| instrument | Wizard | control |
|---|---|---|
| on disk | `Content/Characters/SK_Wizard.uasset` — **6,402,375 B, 2026-07-26** | `SK_Footman.uasset` present (pos.) |
| `git ls-files` (tracked) | **present** | 15 `Content/Characters/SK_*` tracked |
| ⭐ **`ObjectTools.get_class` on the EXACT path `ResolveSkeletalVisual` composes** — `/Game/Characters/SK_Wizard.SK_Wizard` | ⭐ **`/Script/Engine.SkeletalMesh`** | `SK_Cleric` ✅ · `SK_Sorcerer` ✅ · ⛔ **`SK_Witch` → `"is not valid Object"` (NEGATIVE CONTROL)** |

⇒ ⭐ **The resolver is shown able to FAIL on a real absent sibling and SUCCEED on the Wizard.** `CardID` read off the Wizard CDO = **`Wizard`**, so the composed path `/Game/Characters/SK_Wizard.SK_Wizard` is the one the code actually builds (`SummonedUnit.cpp:384-387`) — ⛔ not merely the one I typed.

⇒ **At runtime the Wizard's static `VisualMesh` is HIDDEN** (`SetVisibility(false)`, `SummonedUnit.cpp:512`) and the **skeletal mesh is the visual**.

### 3.2 The call chain, by symbol

| # | site | what happens for the Wizard |
|---|---|---|
| 1 | `BeginPlay` → **`ApplyTeamMaterial()`** — `SummonedUnit.cpp:277` | ⛔ **UNCONDITIONAL.** `GetActiveVisualMesh()` returns the static `VisualMesh` (`bUsingSkeletalVisual` still false) — **non-null by construction** (`CreateDefaultSubobject`, `:176`), so the `if (!ActiveMesh) return;` early-out at `:337` cannot fire ⇒ **`VisualMesh->SetMaterial(0, MI_TeamColor_<Team>)`** (`:354`) |
| 2 | `BeginPlay` → `LoadStatsAndStart()` — `:285` | — |
| 3 | → **`ResolveSkeletalVisual()`** — `:1323` | `SK_Wizard` resolves ⇒ skeletal visible `:509`, **static hidden `:512`**, `bUsingSkeletalVisual = true` `:514` |
| 4 | → `if (bUsingSkeletalVisual) **ApplyTeamMaterial()**` — `:1324-1326` | **`SkeletalVisualMesh->SetMaterial(0, MI_TeamColor_<Team>)`** |

⇒ ⭐ **The Wizard is team-tinted TWICE per spawn, once on each component, on whichever one is active.**

**On which component?** `ApplyTeamMaterial` writes **slot 0 of `GetActiveVisualMesh()`** (`:336`, `:354`) — ⛔ never a hard-coded component. **Slot 0 only** — confirmed at source; the veil loop is the only whole-slot writer.

**Is `Team` correct by then?** ⛔ Yes, and it is measured, not assumed: both shipped spawn paths are `SpawnActorDeferred → InitUnit → FinishSpawning`, so `InitUnit` sets `Team` **before** `BeginPlay` and its own re-apply at `:676` is gated `if (HasActorBegunPlay())` — **zero reachable call sites today**, per the shipped comment at `:659-664`. ⇒ the single `BeginPlay` apply lands the correct team.

### 3.3 ⛔⛔ WHY THIS IS `STRUCTURALLY COVERED` AND **NOT** `INERT BY COINCIDENCE` (`SC-§37`)

⚠️ **The row explicitly offered me the trap: *"it is covered only because `SK_Wizard` resolves."* ⛔ That is NOT the case, and the distinction is the whole point of the row.**

⭐ **Step 1 above runs BEFORE the fork and does not depend on its outcome.** If `SK_Wizard` were deleted tomorrow, `ResolveSkeletalVisual` would return at its `!SkeletalAsset` guard (`:388-391`), the static `VisualMesh` would stay visible — **and `BeginPlay:277` would already have tinted it.** ⇒ ⛔ **Both branches of the fork are covered by the SAME unconditional call.** The coverage is a property of the call graph, not of which assets happen to exist.

⭐ **And there is a SECOND, independent layer** (§2): even a surface that never runs `BeginPlay` at all gets the tint from the mesh asset. ⇒ **two independent mechanisms, either one sufficient.**

### 3.4 ⭐ THE SKELETAL HALF — the same non-finding, one order of magnitude larger

| read | Wizard | Cleric (control) |
|---|---|---|
| `SK_<CardID>` asset slot 0 | `TeamRegion` → **`MI_TeamColor_Blue`** | `TeamRegion` → `MI_TeamColor_Blue` |
| `SK_<CardID>` asset slot 1 | `WizardPBR` → `MI_Wizard_PBR` | `ClericPBR` → `MI_Cleric_PBR` |
| ⭐ **CDO `SkeletalVisualMesh.OverrideMaterials`** | ⛔ **`[]`** | ⛔ **`[]`** |

⇒ ⛔⛔ **On the component that IS the runtime visual for every rigged unit, NOBODY authors an override — 0/14, the Cleric included.** The authored static override that 13 units carry sits on a component that is **hidden at spawn**.

⚖️ ***The property the artist was about to copy onto 12 units lives on the component the player never sees.***

---

## 4. ⛔ (2)(d)+(e) EVERY SURFACE THAT RENDERS A UNIT WITHOUT `BeginPlay` — swept, and there is exactly ONE

| surface | what it actually renders | reads `BP_Unit_*.OverrideMaterials`? |
|---|---|---|
| ⭐ **placement ghost** (the only one) | a bare engine **`AStaticMeshActor`** (`SiegePlayerController.cpp:2896`, `.h:3334`), mesh = the **raw `/Game/Meshes/SM_<CardID>` asset** via `ResolveGhostMesh` (`:4877-4901`) — ⛔ **never the Blueprint** | ⛔ **NO** |
| deck-builder / card hand / card face | ⛔ **pre-rendered `UTexture2D`** `T_CardArt_<CardID>` off the `DT_Cards` row (`CardRow.h:197-204`; `DeckBuilderWidget.cpp:743`, `CardHandWidget.cpp:474`). **32 on disk, `T_CardArt_Wizard` present.** ⛔ Zero `SceneCapture` / `RenderTarget` in all of `Source/` | ⛔ **NO** |
| war map | 2D `FSlateDrawElement` dots; `TActorIterator<ASummonedUnit>` reads **positions only** (`WarMapWidget.cpp:1779-1799`) | ⛔ **NO** |
| editor viewport / BP preview | the **mesh asset's** materials ⇒ team-blue per §2 | n/a — **tinted anyway** |
| deferred / hidden unit previews | ⛔ **none exist.** Every `SpawnActorDeferred<ASummonedUnit>` (`Barracks.cpp:141`, `SiegePlayerController.cpp:4859`) is a **real spawn** with `InitUnit` + `FinishSpawning` ⇒ `BeginPlay` runs. `bDeferConstruction`: **zero hits in `Source/`** | n/a |

⭐⭐ **AND THE GHOST IS TEAM-COLOUR-BLIND BY CONSTRUCTION — verified at source myself, not taken on report:**

```cpp
// SiegePlayerController.cpp:2929-2943
if (UStaticMesh* Mesh = ResolveGhostMesh(PendingCardID)) { GhostMesh->SetStaticMesh(Mesh); }   // :2931
if (UMaterialInterface* GhostMaterial = GhostMaterialAsset.LoadSynchronous()) {
    GhostMID = UMaterialInstanceDynamic::Create(GhostMaterial, this);
    if (GhostMID) {
        for (int32 SlotIndex = 0; SlotIndex < GhostMesh->GetNumMaterials(); ++SlotIndex)
        { GhostMesh->SetMaterial(SlotIndex, GhostMID); }                                        // :2939-2942
    }
}
```

⇒ **EVERY slot is blanketed with the `M_Ghost` MID** (`/Game/Materials/M_Ghost`, set `:216`), and `SetStaticMesh` precedes the loop so `GetNumMaterials()` is the Wizard's real count of 2. ⛔ **Identical for all 14 units. The ghost is green/red/blue, never `MI_TeamColor`.** The only fallback (`M_Ghost` fails to load, `:2945`) drops to the **`SM_Wizard` asset's own** materials — **still `MI_TeamColor_Blue`**, still not the BP array.

**Complete `MI_TeamColor` assignment census across `Source/`** — 4 functions, all on live begun-play actors: `ASummonedUnit::ApplyTeamMaterial` (`SummonedUnit.cpp:348-355`), `ABuilding::ApplyTeamMaterial` (`Building.cpp:166-172`), `ACastle::ApplyTeamVisuals` (`Castle.cpp:1072-1090`), `AProjectile::ApplyTeamVisuals` (`Projectile.cpp:156-176`). ⛔ **No UI, widget, preview, ghost, or war-map path ever assigns a team-colour material.**

---

## 5. ⛔ THE VEIL ROUND TRIP — the one place an authored fallback COULD have mattered

⚠️ **This was the real risk and it deserved checking rather than waving through:** `ClearVeilMaterial` nulls every slot, and a unit with **no authored override** has, on the face of it, nothing to fall back to.

```cpp
// SummonedUnit.cpp:3026-3030
const int32 SlotCount = ActiveMesh->GetNumMaterials();
for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex) { ActiveMesh->SetMaterial(SlotIndex, nullptr); }
// :3043
ApplyTeamMaterial();
```

⇒ **Covered twice over.** The shipped comment at `:3016-3021` states the mechanism (and says it was measured, not assumed): `SetMaterial(i, nullptr)` writes a **null entry**, and both `FStaticMeshComponentHelper::GetMaterial` and `FSkinnedMeshComponentHelper::GetMaterial` test `OverrideMaterials.IsValidIndex(i) && OverrideMaterials[i]` and **fall through to the asset's own material** — which is `MI_TeamColor_Blue` (§2). Then `:3043` calls `ApplyTeamMaterial()` **unconditionally**.
⭐ **An empty array and a null-entry array are the same thing to both readers.** ⇒ the Wizard's restore is byte-equivalent to the Cleric's.

---

## 6. ⛔ LEGACY OR RECENT? — **LEGACY. ~5.5 weeks, and it never changed.**

`Content/Blueprints/Units/BP_Unit_Wizard.uasset` has **exactly two commits, ever**:

| commit | date | subject |
|---|---|---|
| `0d717c0` | **2026-07-26** | TASK-298/305 + TASK-307/308: Wizard AoE fireball unit (SM/SK/BP/card/anims/LODs…) — ⛔ **the BP is born here, with `[]`** |
| `390aff3` | **2026-07-27** | **TASK-334: FACE-wizstatic — `BP_Unit_Wizard` static `VisualMesh` restored to fleet convention (yaw -90, Z -88)** — 1 asset + 1 board file, 4 lines |

⇒ ⛔ **The empty array has been on disk since the day the Wizard was created and has never been touched.** Working tree: `git status` on `Content/Blueprints/Units/` shows **only `A  BP_Unit_Witch.uasset`** — ⛔ the Wizard is **clean**, so nothing in this session or the parallel lanes moved it.

⭐⭐ **AND THE COMMIT THAT EXPLAINS THE WHOLE CITATION-ROT CHAIN IS `390aff3`.** Its own subject says *"restored to fleet convention"* — it fixed **yaw and Z on 2026-07-27** and left `OverrideMaterials` alone. ⇒ ⭐ **`TASK-334` was RIGHT to leave it** (§2 shows there was nothing to fix), **but the law paragraph describing the pre-`390aff3` state was never updated** — which is exactly how a true-in-July sentence survived into a September dispatch in the present tense. ⚖️ *`SC-§38` cl. 4 with a commit hash attached: the repair landed, the description did not.*

### 6.1 ⛔ THE LAW SITES — located BY GREP, not by the citation I was handed

⛔ **I was told to verify every cited law by grep. I did, and the relay I was given was correct in substance and imprecise in wording:**

- ⛔ **`WITCH-§6` (`CONVENTIONS.md:8056-8093`) contains the token `Wizard` TWICE (`:8079`, `:8080`) — so "no `Wizard` token" is literally false — but ⭐ BOTH hits are the CORRECTION, already citing `TASK-953`. ⛔ The STALE CLAIM is genuinely NOT there.** *(awk range validated against both anchors; positive control: `Witch` appears 9× in the same range, so the extractor was not returning empty.)*
- ✅ **The stale sentence lives at the `"Unit mesh facing"` static-`VisualMesh` bullet (`CONVENTIONS.md:4383`+, item 10) — ⛔ AND IT IS ALREADY STRUCK.** It is wrapped in `~~ ~~` and followed by the amendment (items 11–15) recording yaw `−90` / Z `−88`, the empty-array finding, and the `TASK-953` reference. ⇒ 📌 **The `CONVENTIONS.md` amendment `TASK-946` §8 asked for HAS LANDED. Nobody needs to re-do it.**

### 6.2 ⚠️ A THIRD SITE, IN THE SOURCE — reported, and deliberately NOT called stale

`SummonedUnit.cpp:409-412` reads: *"`BP_Unit_Wizard`'s static `VisualMesh.Z` was never offset … so the copy propagated 0 and the Wizard floated exactly like the old Archer/Ogre."*

⛔ **This is NOT a defect and I am not asking anyone to touch it.** It is **past tense**, it explains why the `TASK-259` v1 fix was insufficient, and it is **historically accurate** — the Z *was* 0 until `390aff3` the next day. ⚠️ **I flag it only because a skimming reader could take the clause as present tense**, and because the row asked me to locate every site rather than the ones I was pointed at. ⭐ **I measured today's value (`−88`) rather than inferring the comment's staleness from the law's.**

---

## 7. ⛔ RECOMMENDATION — **NO REPAIR**, and the reverse direction is also refused

⛔ **Do NOT author `OverrideMaterials[0] = MI_TeamColor_Blue` on `BP_Unit_Wizard`:**

1. ⛔ **It would change zero pixels** — measured, §2. A repair with no observable effect is indistinguishable from a no-op that future readers must re-derive.
2. ⛔ **It would ADD a third-layer restatement of a value the asset already holds** — the "second source of truth about something already derivable" shape this file's own comments refuse (`SummonedUnit.cpp:3009-3010`).
3. ⭐⭐ **If either state is the correct one, it is the WIZARD'S.** The day someone re-points a mesh's `TeamRegion` slot at a different instance, the 13 authored overrides would **silently mask it** while the Wizard **correctly follows**. ⛔ **The outlier is the one that behaves right.**

⛔ **And I am equally NOT recommending stripping the other 13.** That is a 13-asset `Content/` churn, under LFS, for zero pixels, with real re-save risk and no gain. ⚖️ *The correct outcome of this row is a RECORD, not an edit.*

📌 **What I would board, if the manager wants anything at all:** a one-line law note under the `"Unit mesh facing"` amendment stating **"`SM_<CardID>` slot 0 (`TeamRegion`) is `MI_TeamColor_Blue` on the ASSET, 14/14 — the BP-level `OverrideMaterials[0]` is a redundant restatement and its absence on the Wizard is correct"** — so the next census does not re-open this. ⛔ **I did not write it (law is not mine and this row forbids writes).**

---

## 8. ⛔⛔ WHAT I COULD **NOT** DETERMINE — stated as unknowns, not as passes (`SC-§40` cl. 1)

1. ⛔⛔ **NO PIXELS. NO PIE. This is an ASSET + SOURCE reading, start to finish.** The sentence *"a red Wizard reads red"* is **TRACED, never SEEN.** MCP has no input lane and this row forbids starting PIE. ⇒ 🧑 **`TASK-954`'s sitting could confirm it in one glance** — if Jonathan has a bot Wizard on screen, the question is closed by eye in a second. ⛔ **I did not close it and I am not claiming it.**
2. ⛔ **PACKAGED/COOKED behaviour is UNMEASURED.** §2 rests on the **editor's** view of the mesh assets. A cook that reassigns or strips asset-level material slots would break the second layer (the first — `ApplyTeamMaterial` — would still hold). ⛔ I have no cooked-build evidence and did not produce any.
3. ✅ **BP-side material assignment — GAP CLOSED, not declared.** I worried a construction script or graph node could assign a material invisibly to a CDO read. **`AssetTools.get_dependencies` settles it:** Wizard → `[/Script/GitClaudeUnrealTest, /Game/Blueprints/BP_Projectile_Fireball, /Game/Meshes/SM_Wizard]` — ⛔ **NO material dependency of any kind.** Controls: Cleric and Witch each return `MI_TeamColor_Blue`. ⇒ ⭐ **the instrument returns a material for 2 of 3 and not for the Wizard — there is no BP-side material assignment anywhere in `BP_Unit_Wizard`.** *(The extra `BP_Projectile_Fireball` is the Wizard's own AoE projectile class, not a material.)*
4. ✅ **Do the two team instances actually differ? — CHECKED, since the whole "tell them apart" premise rests on it.** Same parent `M_TeamColor`, same `TeamColor` param GUID; **Blue `(0.05, 0.30, 1.00)` vs Red `(1.00, 0.10, 0.05)`.** ⇒ strongly distinct.
5. ⚠️ **I did not read `BP_Unit_Wizard`'s event graph directly** — `read_graph_dsl` needs a `graph` argument I did not have the schema for. ⛔ **Item 3's dependency measurement covers the material question completely**, but I am declaring the graph itself unread rather than implying I opened it.
6. ⛔ **`TASK-952`'s subjects (Sorcerer parent, Cleric rotation) were NOT investigated** — out of fence, disjoint by instruction. ⛔ The capsule question (`TASK-955`) likewise untouched, though §1.1's reads happen to corroborate the Wizard's `88`.

---

## 9. 🔍 WHAT QA SHOULD SCRUTINISE

1. ⭐⭐ **§2 is the load-bearing claim and everything else is decoration.** If `SM_Wizard`'s slot-0 asset material is NOT `MI_TeamColor_Blue`, the verdict weakens from *"the premise dissolves"* to *"covered at runtime, untinted in the editor viewport"* — ⛔ **still not `LIVE`** (§3 is independent), but the recommendation would change. **Re-run `StaticMeshTools.get_material` on `/Game/Meshes/SM_Wizard` slot `TeamRegion`.**
2. ⛔ **Check I did not confuse `GetNumMaterials()` with `OverrideMaterials.Num()`.** I did not rely on `GetNumMaterials()` for this verdict at all — `TASK-946` traced it for the veil loop; here the relevant reader is `GetMaterial(i)`'s valid-index-and-non-null test (§5). ⭐ **Confirm that distinction is real and that I did not inherit `TASK-946`'s finding in place of making my own.**
3. ⛔ **§3.3 is the `SC-§37` claim and it is the one worth attacking.** Is `BeginPlay:277`'s apply *genuinely* independent of the fork? Attack it: find a path where `ResolveSkeletalVisual` runs **before** `BeginPlay:277`, or where `GetActiveVisualMesh()` returns null. I claim neither exists (`VisualMesh` is a `CreateDefaultSubobject`, `:176`).
4. ⚠️ **Every count in §2/§4 came from ONE `ProgrammaticToolset` script.** A script that silently swallowed failures would print plausible uniformity. ⭐ **It did not swallow: an earlier run of the same script DIED on `SK_Witch` rather than returning a tidy null** — that crash is my evidence the harness surfaces failures instead of hiding them, and the per-unit slot names differ (`FootmanPBR` / `WizardPBR` / …), which a canned response would not produce.
5. ⛔ **§6's "two commits, ever" — re-run `git log -- <path>`.** My conclusion that this is legacy rather than a fresh regression rests entirely on it.

---

## 10. FENCE — what I touched

- ⛔ **ZERO writes** to `Source/**`, `Content/**`, tests, `CONVENTIONS.md`, or any board row **except TASK-953's own status line**.
- ⛔ **No compile. No PIE. No Git write** — `git log` / `git show --stat` / `git status` / `git ls-files` only.
- ⛔ **Editor (PID 40528): READ-ONLY.** Only `get_properties`, `get_class`, `get_material`, `get_material_slots`, `get_dependencies` were called. ⛔ **No `set_*`, no `save_assets`, no level load.** ⛔ **No save or restore prompt was presented; none was accepted. `L_Arena` was never opened or touched.**
- ⚠️ **`is_dirty` was NEVER called** — the standing warning is that it returns `true` for every extant asset. ⛔ Nothing here is gated on it.
- ⛔ `TASK-952`'s and `TASK-955`'s subjects deliberately untouched.
