# TASK-763 — `BP_SiegeGhostPawn` — art-director handoff

**Status:** `ready-for-integration`
**Law:** `GHOST-§3` `G-1`/`G-2`/`G-3`/`G-4` · `GHOST-§4` · `GHOST-§5` · `SC-§35` · `§25b`
**Closes:** `qa/TASK-753.md` **W-1** (*"the ghost is INVISIBLE and IMMOBILE"*).
**Suite delta: ⛔ ZERO.** Pure asset wiring — ⛔ no C++, ⛔ no compile, ⛔ no Git, ⛔ no PIE, ⛔ no `.umap`, ⛔ no Blender, ⛔ no new geometry.

---

## ⭐ THE ASSET

**`/Game/Blueprints/BP_SiegeGhostPawn`** — file `Content/Blueprints/BP_SiegeGhostPawn.uasset` (27,089 B)
Blueprint compiled with **`warnings_as_errors = true`** (it raises on warnings; it did not raise). `is_dirty == false` after save.

### ⚠️ PARENT CLASS — READ BACK, ⛔ not assumed
`get_parent` ⇒ **`/Script/GitClaudeUnrealTest.SiegeGhostPawn`** — measured **twice**, before *and* after the compile.
⭐ This is the check the brief demanded: a Blueprint that silently fell back to `ACharacter`/`APawn` **opens perfectly with all its behaviour missing.** It did not fall back.

---

## ⭐⭐ THE SEVEN SLOTS — ENUMERATED AT THE REFLECTED CLASS, ⛔ NOT FROM A REMEMBERED LIST

I did **not** work from the three-slot list. I ran `ObjectTools.list_properties` against
`/Script/GitClaudeUnrealTest.SiegeGhostPawn` and classified **every** property the class declares:

| # | slot | type | BEFORE | AFTER |
|---|---|---|---|---|
| 1 | `GhostMappingContext` | `UInputMappingContext` | `None` | **`/Game/Input/IMC_Hero`** |
| 2 | `MoveAction` | `UInputAction` | `None` | **`/Game/Input/Actions/IA_Move`** |
| 3 | `LookAction` | `UInputAction` | `None` | **`/Game/Input/Actions/IA_Look`** |
| 4 | `MouseLookAction` | `UInputAction` | `None` | **`/Game/Input/Actions/IA_MouseLook`** |
| 5 | `GhostMesh` | `TSoftObjectPtr<USkeletalMesh>` | `None` | **`/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple`** |
| 6 | `GhostMaterial` | `TSoftObjectPtr<UMaterialInterface>` | `None` | **`/Game/Materials/MI_Ghost_Translucent`** |
| 7 | `GhostIdleAnimation` | `TSoftObjectPtr<UAnimationAsset>` | `None` | **`/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle`** |

**The BEFORE column is measured**, off `/Script/GitClaudeUnrealTest.Default__SiegeGhostPawn`: all seven read `None`, confirming W-1 at the class rather than trusting the report.

### ⛔ AND THE THREE PROPERTIES THAT ARE ⛔ NOT SLOTS — stated so nobody "finishes the job" later
`CameraBoom` · `FollowCamera` · `GhostTeam` are **`VisibleAnywhere`, ⛔ not `EditDefaultsOnly`** — two C++-constructed components and the runtime team field. ⛔ **They are not designer slots and were deliberately left alone.** `GhostTeam` reads `Blue` on the CDO, which is its **declaration default**, ⛔ not an assignment by me — `InitializeGhost()` owns it at runtime (`TASK-758` ordering).

⇒ **7 fillable slots, 7 filled, 0 remaining.**

---

## ⭐ VERIFIED BY READ-BACK — ⛔ never by a call returning success

`set_properties` returned `true`. **That was ignored as evidence.** Four independent confirmations:

1. **CDO read-back before compile** — all 7 correct.
2. **CDO read-back AFTER compile** — all 7 survive (a recompile rebuilds the CDO; this is where a silent reset would show).
3. **CDO read-back AFTER save** — all 7 still correct.
4. ⭐ **On-disk dependency read-back** (`get_dependencies` on the saved package) — the strongest instrument, because it reads the **serialized package**, not editor memory:
   ```
   /Script/GitClaudeUnrealTest, /Game/Input/IMC_Hero,
   /Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple,
   /Game/Input/Actions/IA_Look, /Game/Input/Actions/IA_MouseLook,
   /Game/Input/Actions/IA_Move, /Game/Materials/MI_Ghost_Translucent,
   /Game/Characters/Mannequins/Anims/Unarmed/MM_Idle
   ```
   ⇒ **exactly the seven targets + the code module. ⛔ Nothing missing, ⛔ nothing extra.**

**Every target's class was verified before wiring** (⛔ a name that resolves is not a name of the right type):
`InputMappingContext` · `InputAction` ×3 · `SkeletalMesh` · **`MaterialInstanceConstant`** · **`AnimSequence`**.

### ⭐ THE ONE THING TASK-756 ASSERTED THAT I MEASURED — skeleton binding
`MM_Idle` is only playable on `SKM_Quinn_Simple` if they share a skeleton; a mismatch means `PlayAnimation` silently does nothing and the ghost stands in reference pose. Read off **both assets**:

| asset | `skeleton` |
|---|---|
| `SKM_Quinn_Simple` | `/Game/Characters/Mannequins/Meshes/SK_Mannequin` |
| `MM_Idle` | `/Game/Characters/Mannequins/Meshes/SK_Mannequin` |

✅ **Identical ⇒ it binds.** And `MM_Idle` is an **`AnimSequence`**, ⛔ not an Anim Blueprint — `SC-§35` single-node playback honoured; ⛔ no `TryGetPawnOwner()` exists to resolve wrongly.

---

## ⛔ THE HARD CONSTRAINTS — HELD, AND ⭐ MEASURED ON THE BLUEPRINT'S OWN CDO

⚠️ Measured on `BP_SiegeGhostPawn`'s CDO capsule (`Default__BP_SiegeGhostPawn_C:CollisionCylinder`), ⛔ not on the C++ parent — the BP is what spawns, so the BP is what had to be proved:

| constraint | measured | verdict |
|---|---|---|
| capsule `objectType` | **`ECC_Pawn`** | ✅ ⛔ **NOT** `WorldStatic` ⇒ stays out of `AProjectile::FindTerrainHit`'s object trace. **No projectile shield for a dead player.** |
| `Pawn` response | `ECR_Ignore` | ✅ no body-block wall |
| `WorldStatic` response | not overridden ⇒ **`ECR_Block`** | ✅ walks the ground, walls still stop it (`G-2`) |
| `SiegeTeamBlue` / `SiegeTeamRed` | `ECR_Ignore` | ✅ ⛔ no combatant-body channel |
| `collisionEnabled` | `QueryAndPhysics` | ✅ does not sink through the floor |
| capsule size | radius **42**, half-height **96** | ✅ the hero's own |
| `ITeamAgent` | ⛔ **NOT implemented** | ✅ see below |
| `primaryActorTick.bCanEverTick` | **`false`** | ✅ byte-identical to the native CDO |
| `netCullDistanceSquared` | **3,600,000,000** | ✅ Tier B (600 m) inherited intact |
| `bReplicates` | `true` | ✅ inherited from `APawn`, ⛔ not re-set |

### ⛔⛔ `ITeamAgent` — WHY IT CANNOT HAVE BEEN ADDED
The untargetability of the ghost is a **structural omission**, and I did nothing that could undo it:
- the **parent** does not implement it (`GHOST-§1`, and `FSiegeGhostPawnNotATeamAgentTest` guards it);
- the `BlueprintTools` toolset exposes **no interface-adding tool at all** — I enumerated all 53 tools;
- my complete set of write operations on this asset was exactly four: `create` → `set_properties` (the 7) → `compile_blueprint` → `save_assets`.
- the on-disk dependency list carries **no interface asset**.

⇒ ⭐ **all eight `GetAllActorsWithInterface(UTeamAgent…)` acquisition sites — including every AoE — still cannot see the ghost.**

### ⛔ NO GAMEPLAY IN THE GRAPH
`list_variables` ⇒ **`[]`** (⛔ zero variables added). Graphs read back via `read_graph_dsl`:
- `UserConstructionScript` ⇒ `(fn ConstructionScript ())` — **empty**
- `EventGraph` ⇒ UE's three **default disconnected stubs** (`EventBeginPlay`, `EventActorBeginOverlap`, `EventTick`), ⛔ **no connections, no logic**.

⚠️ **The Tick stub was not taken on trust** — a *used* Tick event flips `bCanEverTick` on the generated class, which would have quietly undone the C++'s `PrimaryActorTick.bCanEverTick = false`. **Measured: `bCanEverTick == false` on the BP CDO, identical to the native CDO.** ⇒ the stubs are inert and the ghost still ticks nothing.

---

## ⚠️ `§25b` — CHECKED, AND IT IS ⭐ **CLEAN**. ⛔ THE SIZE CHECK WOULD HAVE CRIED WOLF.

The editor's SCC auto-staged the file (`A ` in `git status`). I verified **oid-vs-worktree-sha256, ⛔ never by size** — and this run is a live demonstration of *why* that law is worded that way:

| instrument | value | reading |
|---|---|---|
| index blob **size** | **130 B** | ⛔ **vs 27,089 B worktree — would look like a catastrophic mismatch** |
| index blob's own sha256 | `5dd687d8…a7d8` | ⛔ also "mismatches" — it is the sha of the *pointer text* |
| ⭐ **LFS pointer's declared `oid sha256`** | **`8c5fef7ecc07c0a1156a2bf70c608742c2e3245718bb98e3fb7d06c1c313a814`** | ✅ |
| ⭐ **worktree file sha256** | **`8c5fef7ecc07c0a1156a2bf70c608742c2e3245718bb98e3fb7d06c1c313a814`** | ✅ **IDENTICAL** |

`*.uasset` is `filter=lfs` (repo-root `.gitattributes:1`), so a 130-byte index blob is the **correct** LFS pointer, ⛔ not a stale pre-wiring blob.
✅ **The staged entry IS the wired Blueprint. ⛔ No re-`git add` is needed for this file.** ⛔ I touched no Git.

---

## ⚖️ `GhostPawnClassAsset` — ⛔ NOT SET BY ME, AND IT IS ⛔ NOT A CDO EDIT ANYONE CAN MAKE SAFELY

⭐ **This is the one thing still standing between this Blueprint and a visible, mobile ghost.** Measured:

- `ASiegeGameMode::GhostPawnClassAsset` is `UPROPERTY(EditDefaultsOnly, …)` — ⛔ **not `config`**, so there is **no ini that can hold it**.
- `Config/DefaultEngine.ini:12` ⇒ `GlobalDefaultGameMode=/Script/GitClaudeUnrealTest.SiegeGameMode` — the shipped game mode is the **raw C++ class**.
- ⛔ **There is NO `BP_SiegeGameMode`** anywhere in `Content/` (the only game-mode BP is `BP_MenuGameMode`, plus template/demo ones).

⇒ ⛔ **A native-class CDO edit has nowhere to persist**, so I correctly did **not** reach into it. **It needs one line of C++** — and the house precedent is sitting three lines above it (`SiegeGameMode.cpp:47`, `HeroPawnClassAsset`), which is a **constructor assignment**, ⛔ not an editor edit:

```cpp
GhostPawnClassAsset = TSoftClassPtr<ASiegeGhostPawn>(FSoftObjectPath(TEXT("/Game/Blueprints/BP_SiegeGhostPawn.BP_SiegeGhostPawn_C")));
```

replacing the commented-out `// GhostPawnClassAsset = <unset>;` at `SiegeGameMode.cpp:57`.
⭐ The `_C` suffix is **verified**, ⛔ not guessed: `get_asset_class` ⇒ **`BP_SiegeGhostPawn_C`**.
⚠️ **Until that line lands, `ResolveGhostPawnClass()` falls back to the raw C++ class and the ghost is STILL invisible and immobile** — the Blueprint exists but nothing spawns it. ⛔ **This task is not "done" for the player until that one line ships** (gameplay-programmer, ⛔ not mine).

---

## ⚖️ TEAM COLOUR — ⛔ NOT DECIDED, ⛔ NOT INVENTED. WIRED SO IT STAYS ONE PROPERTY.

Per the brief, this is **Jonathan's call** and I made none. The ghost wears the **neutral** `MI_Ghost_Translucent`.

⭐ **The "one-property change later" claim is measured, ⛔ not asserted:**
`MI_Ghost_Translucent` → parent **`/Game/Materials/M_HeroSpirit`**, exposing vector param **`TeamColor`**, currently the neutral cold cyan **(0.45, 0.78, 1.00)**.
⇒ the tint is one `SetVectorParameterValue("TeamColor", …)` at the code seat already marked in `ApplyGhostTeamAppearance()` — the shipped `ACaptureZone` idiom (`CaptureZone.cpp:74` + `:257`). ⛔ **Nothing in this Blueprint blocks or pre-empts that ruling.**

⚠️ **The standing measurement, unchanged:** the hero has **no body team tint at all**; team reads only via the overhead health bar and damage numbers, and `GHOST-§5`/`GHOST-§1` deny the ghost **both**. ⇒ **blue and red ghosts remain indistinguishable.** Latent today (one human controller), **real at M8**. `MI_HeroSpirit_Blue`/`_Red` remain authored and **unwired** — ⛔ I did not wire them, because a per-BP material cannot express a per-team choice anyway; the seat is the C++ one above.

---

## ⚠️ TWO THINGS FOR THE ORCHESTRATOR / MANAGER

1. ⚠️ **TASK-763 HAS ⛔ NO BOARD ROW.** The board's highest task is **TASK-762**; `grep "TASK-763"` ⇒ **0 hits**. This is **W-9 repeating** (*"a dispatch is ⛔ not a board entry"*). ⛔ **I did not create one** — task creation is the **manager's, exclusively** (board write discipline §1), and a self-serve row is exactly what that law forbids. ⇒ **the manager should board it**, carrying the `GhostPawnClassAsset` line as its own follow-up row.
2. ⚠️ **The play-distance judgement is still owed** and I still cannot discharge it — it rides the PIE session `GHOST-§4` owes (**TASK-755 item ⑤**, *"fly the ghost"*), which is now **unblocked for the first time** once the `GhostPawnClassAsset` line lands. **One instruction for whoever runs it: look at the ghost from the play camera and confirm it does not read as a living hero.** All tuning is instance parameters ⇒ any correction is a value change with ⛔ no shader recompile.

---

## Fences held — verified, ⛔ not asserted

⛔ `SiegeGhostPawn.{h,cpp}` · `SiegeGameMode.{h,cpp}` · `SiegePlayerController.{h,cpp}` · `SM_WatchTower` · `A_SiegeBiped_Climb` · `ABP_Footman` · `M_Ghost` · `M_HeroSpirit` · `MI_Ghost_Translucent` — **all read-only, none modified**.
⛔ **`L_Arena` NEVER saved** and never opened for edit. ⛔ **`save_assets` was called with an EXPLICIT one-path list — ⛔ never the empty list.**
⛔ No Git. ⛔ No C++. ⛔ No compile of the game module. ⛔ No editor restart, ⛔ no modal encountered.
⛔ No `Content/RawAssets/` write — ⭐ **zero geometry was authored** (the whole point of TASK-756's mesh reuse), so there is no source FBX/PNG and the `_D/_N/_ORM` texture law is not applicable.

## ⭐ EVERY PATH TOUCHED (`§25b` — one file written, seven read)

**WRITTEN (1):**
```
Content/Blueprints/BP_SiegeGhostPawn.uasset   <- NEW, 27,089 B
    worktree sha256 = 8c5fef7ecc07c0a1156a2bf70c608742c2e3245718bb98e3fb7d06c1c313a814
    staged LFS oid  = 8c5fef7ecc07c0a1156a2bf70c608742c2e3245718bb98e3fb7d06c1c313a814  ✅ MATCH
```
**READ-ONLY (7)** — referenced, ⛔ not modified:
`Content/Input/IMC_Hero.uasset` · `Content/Input/Actions/IA_Move.uasset` · `Content/Input/Actions/IA_Look.uasset` · `Content/Input/Actions/IA_MouseLook.uasset` · `Content/Characters/Mannequins/Meshes/SKM_Quinn_Simple.uasset` · `Content/Materials/MI_Ghost_Translucent.uasset` · `Content/Characters/Mannequins/Anims/Unarmed/MM_Idle.uasset`
