# TASK-562 — [WR-8] `ACastle` FURNISHING: spawn and own the torches + the NPC

**Agent:** gameplay-programmer · **Date:** 2026-08-15 · **Status → `ready-for-qa`**
**Gate:** ⛔ **`qa/TASK-565.md`** (this task is one of the nine that gate names) · **Compile:** TASK-566 · **Commit:** TASK-570
**Law worked to:** CONVENTIONS **`WR-§0`..`WR-§9`** (esp. `WR-§4` placement/teardown, `WR-§5` placement, `WR-§8`) · **`SC-§15`** · **`SC-§18c`** · **`SC-§22`** · **`SC-§33`** · **`SC-§34`** + "Castle 3× HOLLOW"

⛔ **NONE PERFORMED: no compile · no build · no Git · no editor · no MCP · no PIE · no `Content/` asset · no `.csv` · no `Build.cs` · no `Tests/` · no `L_Arena`.** File-only, as dispatched. The Unreal editor was running this session and was **not touched**.
⛔ **NO TOKEN FIGURE IS QUOTED, DERIVED OR REASONED FROM ANYWHERE IN THIS DOCUMENT** (`AS-§12g`, batch-wide ban).

---

## 0. FILES — EXACTLY THE TWO IN THE `names:` BLOCK, AND ⛔ TASK-557'S EIGHT INITIALISERS ARE INTACT

| file | disposition |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Castle.h` | **EDITED** — additive |
| `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp` | **EDITED** — additive |

⛔ **NOT touched:** `Torch.{h,cpp}` · `CommanderNpc.{h,cpp}` · `SiegePlayerController.{h,cpp}` · `SiegeBotController.h` · `ScatterConfig.h` · `L_Arena` · any `Content/` asset · any `.csv` · `Tools/ArtPipeline/*` (⚠️ the manifest was opened **READ-ONLY**, §2).

### ⛔ THE FIRST THING I WAS TOLD TO CHECK — TASK-557'S DIFF, RE-VERIFIED AT THE CODE, NOT ASSUMED

`git diff` in this working tree is against `HEAD` (`f205eb5`), which **predates TASK-557**, so 557's lines appear in my diff as additions. **They are 557's, not mine, and every one is still at 557's value.** Verified by direct read-back:

```
$ grep -n "CastleDamageNumberHeightZ = \|SetRelativeLocation(FVector(0.0f, 0.0f, \|SpawnBoxHalfExtent = FVector2D\|GateBlockerRelativeLocation = FVector\|GateBlockerExtent = FVector" Castle.h Castle.cpp
Castle.h:367   FVector2D SpawnBoxHalfExtent = FVector2D(7380.f, 7380.f);
Castle.h:399   FVector GateBlockerRelativeLocation = FVector(18.f, -1575.f, 852.f);
Castle.h:435   FVector GateBlockerExtent = FVector(900.f, 405.f, 678.f);
Castle.cpp:50  constexpr float CastleDamageNumberHeightZ = 9450.f;
Castle.cpp:174 HPBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 9450.0f));
```
✅ **All five in-file values present.** ✅ **`GateBlockerExtent` is `(900,405,678)` — 557's DECLARED DEPARTURE, deliberately left standing; I did not "restore" the specced 780.** ✅ The 3-way pairing law still holds (`ScatterConfig.h:376 = 4500`, `SiegePlayerController.h:1070` and `SiegeBotController.h:422` both `(7380,7380)`) — checked, not edited (spec item 5: *"DO NOT re-touch TASK-557's constants"*).

**Signature-level proof that I changed nothing existing** — every declaration in `Castle.h` at `HEAD` vs now, sorted and diffed:

```
$ diff <(git show HEAD:…/Castle.h | grep -E "^\s+(virtual |static )?[A-Za-z_].*\(.*\)( const)?( override)?;" | …sort) \
       <(grep -E … Castle.h | …sort)
< FVector GateBlockerExtent = FVector(260.f, 135.f, 226.f);            ⟶ TASK-557 (not a signature)
< FVector GateBlockerRelativeLocation = FVector(6.f, -525.f, 284.f);   ⟶ TASK-557 (not a signature)
< FVector2D SpawnBoxHalfExtent = FVector2D(2460.f, 2460.f);            ⟶ TASK-557 (not a signature)
> UClass* ResolveCommanderNpcClass();                                  ⟶ MINE, new, 0 params
> UClass* ResolveTorchClass();                                         ⟶ MINE, new, 0 params
> virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;  ⟶ MINE, engine override
> void DestroyCastleFurnishings();                                     ⟶ MINE, new, 0 params
> void SpawnCastleFurnishings();                                       ⟶ MINE, new, 0 params
```
⇒ **5 declarations added, 0 removed, 0 MODIFIED.** The three `<` lines are member initialisers the regex catches on their `FVector(...)` shape — TASK-557's, and they are the *only* pre-existing lines that differ from `HEAD` in this file.

---

## 1. ⭐⛔ THE HEADLINE — **THE SPEC NAMED THE WRONG LIFECYCLE HOOK, AND FOLLOWING IT LITERALLY WOULD HAVE SHIPPED THE EXACT LEAK IT WARNS ABOUT.** DECLARED DEPARTURE (`SC-§15`)

**Spec item (4), verbatim:** *"Destroy on castle destruction; re-spawn on `ResetCastle` / Play Again — ⭐ mirror the EXISTING `GateBlockerVolume` lifecycle, which already solves this exact problem in this exact file (`HandleDestroyed`'s `SetActorEnableCollision(false)` + `ResetCastle`'s restore). ⛔ Read those two functions before writing yours."*

**I read them. The description is one refactor out of date, and the two instructions in that sentence point at DIFFERENT functions.**

**LEG 1 — the donor moved.** The M8 refactor (TASK-356, doc §3.1) lifted `SetActorEnableCollision` out of `HandleDestroyed` into **`ACastle::ApplyDestroyedState(bool)`**. `HandleDestroyed` now calls `ApplyDestroyedState(true)`; `ResetCastle` calls `ApplyDestroyedState(false)`; and **`OnRep_Destroyed` calls it too.** `Castle.h`'s own comment says why in terms: *"the gate blocker rides the actor state on both machines — addendum §2."*

**LEG 2 — and this is the part that makes it a defect, not a style question. `HandleDestroyed` and `ResetCastle` are BOTH AUTHORITY-ONLY:**

```
$ grep -n "HasAuthority" Castle.cpp
699:  float ACastle::TakeDamage      → if (!HasAuthority()) { … return 0.0f; }   (HandleDestroyed's only caller)
984:  void  ACastle::ResetCastle     → if (!HasAuthority()) { … return; }
1137: void  ACastle::HealOverTime    → if (!HasAuthority()) { … return; }
```
The furnishing is **NET RELEVANCY TIER C — one independent local set on EVERY machine** (`ATorch` / `ACommanderNpc` both declare it; `bReplicates` stays false on both). ⇒ **Hooking the two named functions would have destroyed the SERVER's furniture and left the CLIENT's standing** — 6 torches + 1 commander per castle **per replay, forever, on the machine nobody was watching**. That is `WR-§4`'s *"a leak here is 12 orphan lights per replay"* arriving through the door the spec held open.

**LEG 3 — why it is not even visible without the destroy.** `AActor::SetActorHiddenInGame` and `SetActorEnableCollision` reach the actor's **own components only**; an attached *actor* keeps its own `bHidden`. So a fallen castle does not hide its furniture for free — a destroyed castle would be an invisible ruin with **six lit torches and a commander floating in mid-air**.

**⇒ WHAT SHIPPED: the furnishing lifecycle hangs off `ApplyDestroyedState(bool)`** — `true` ⇒ `DestroyCastleFurnishings()`, `false` ⇒ `SpawnCastleFurnishings()`. **This is the spec's ACTUAL instruction ("mirror the `GateBlockerVolume` lifecycle") honoured exactly; only its stale function names are refused.** The reasoning is pasted into `ApplyDestroyedState`'s doc block in `Castle.h` so the next reader does not undo it.

---

## 2. ⭐ THE ANCHOR TRANSFORMS, AND HOW EACH ONE WAS DERIVED FROM THE 9× INTERIOR (the spec's named deliverable)

### 2.1 The interior I derived against, and ⛔ where the numbers came from

⛔ **PROVENANCE FIRST, because `SC-§22` says a citation is a lower bound and because one candidate source is an in-flight task's working file.**

- **PRIMARY (published, committed):** `handoffs/TASK-348-artist.md`'s carve readbacks for the **3×** castle — `hall_main` box `970×240×520` at `x −640..+330, y +90..+330, z 58..578`; `hall_east` `280×140` at `x +280..+560, y +190..+330`; `gate_corridor` 500-wide arch `y −380..+170`; `gate_arch` `y −700..−340`; interior floor `z 58`.
- **× 3 under `WR-§1`** (*"the SHELL scales"*), which states the two headline results itself: grand hall `970 × 240 → 2910 × 720`, clear height `520 → 1560`, floor `z 58 → 174`.
- **CORROBORATION ONLY, READ-ONLY:** `Tools/ArtPipeline/pipeline_manifest.json`'s `assets.Castle.carve` block, which TASK-555 has already re-derived in-tree to **exactly** these numbers. ⚠️ **It is NOT the source and it was not edited** — reading an in-flight parallel task's working file as a contract is the coupling TASK-558 correctly refused. It agreeing to the unit is a *check*, not an *authority*.

| interior volume | 9× mesh-local extent (origin = ground-centre, `WR-§0`) |
|---|---|
| interior floor | **z 174** |
| `hall_main` (grand hall) | **x [−1920, +990] · y [+270, +990] · z [174, 1734]** = 2910 × 720 × 1560 |
| `hall_east` (annex) | **x [+840, +1680] · y [+570, +990]**, same z |
| `gate_corridor` | 1500 wide about the mesh gate centreline x 18 ⇒ **x [−732, +768] · y [−1140, +510]**, vertical walls to the springline **z 1410** |
| `gate_arch` (through the front wall) | 1800 wide about x 18 ⇒ **y [−2100, −1020]** |

⭐ **THE SPACE CHECK THAT MAKES ALL OF THIS SAFE, and it is the strongest cross-check available in this file:** my anchors are relative to **`CastleMesh`** — the *identical* space as `GateBlockerRelativeLocation`, which is (a) **PIE-verified in engine** at TASK-350 and (b) **re-derived ×3** by TASK-557 on the argument *"it is a MESH-LOCAL offset and the mesh scaled uniformly under it."* Same component, same units, same convention (`−Y` = the gate side, `+Y` = deeper into the keep). If my anchor space were wrong, the shipped gate blocker would be wrong too.

### 2.2 The ONE mount height, derived once

**`TorchWallMountZ = InteriorFloorZ + 0.5 × HallClearHeightZ = 174 + 780 = 954`** — the midpoint of the hall's clear height, which is *also* the hall carve box's own centre z. **Verified against the corridor too**, because that volume is an ARCH and not a box: its walls are vertical from the floor to the springline at 1410, and `174 < 954 < 1410`, so a corridor torch is on flat wall with 456 uu still above it.

⛔ **The attenuation radius is deliberately NOT duplicated into `Castle.cpp`** — it is `ATorch`'s `EditDefaultsOnly` tunable, and a second copy here is precisely the stale-constant hazard `SC-§34` exists for. It is used only as *reasoning*: at its shipped default and a 780-uu drop to the floor, a pool still reaches **≈912 uu horizontally at floor level** (`√(1200² − 780²)`). Every spacing claim below is checked against that figure and **re-checks itself automatically if the tunable moves**, because nothing was transcribed.

### 2.3 ⭐ THE SIX TORCH ANCHORS — pasted, with the derivation per row

Rotation aims the torch **off** the wall: `SM_Torch`'s origin is its **wall-mount face** and the mesh extends along its own **+X** into the room (`WR-§4` / TASK-556). Scale is **1** on every anchor. Every location sits **exactly on** the carve-cutter face, i.e. flush on the wall — an inset would float the torch, and TASK-348's probe confirms the cutter faces *are* the wall surfaces (it hit `hall_back` at exactly the cutter's `y_max`).

| # | volume | wall | mesh-local location | yaw | derivation |
|---|---|---|---|---|---|
| **1** | `hall_main` | north (y +990) | **(−1435, 990, 954)** | **−90** | `HallMinX + 0.5 × 970` — centre of the **1st third** of the hall's 2910-uu length |
| **2** | `hall_main` | north | **(−465, 990, 954)** | **−90** | `HallMinX + 1.5 × 970` — **= the hall's own X centre** |
| **3** | `hall_main` | north | **(+505, 990, 954)** | **−90** | `HallMinX + 2.5 × 970` — centre of the 3rd third |
| **4** | `hall_main` | south (y +270) | **(−1435, 270, 954)** | **+90** | mirrors #1. ⚠️ **The ONLY third-centre that wall has** — the 1500-wide corridor punches through it from x −732 to +768 and swallows the other two |
| **5** | `hall_east` | east (x +1680) | **(+1680, 780, 954)** | **180** | the annex's own Y centre. ⚠️ **Not decoration:** the annex reaches x +1680 and #3's pool stops **≈263 uu short** of that wall at floor level — without this anchor the annex is the one carved volume that is **unlit** |
| **6** | `gate_corridor` | west (x −732) | **(−732, −315, 954)** | **0** | the corridor's mid-length `(−1140 + 510)/2`. One pool spans the full **1650-uu** passage lengthwise |

**Yaw convention, stated so it is checkable:** yaw 0 ⇒ the wall is at the anchor's −X · 180 ⇒ +X · +90 ⇒ −Y · −90 ⇒ +Y. Cross-checked row by row against the wall each anchor sits on.

**COVERAGE ARITHMETIC (all at floor level, pool reach ≈912):**
- Hall length: `[−2347,−523] ∪ [−1377,+447] ∪ [−407,+1417]` = continuous **[−2347, +1417] ⊇ [−1920, +990]** ✅
- Hall depth: a y = 990 torch reaches `y ∈ [78, 1902] ⊇ [270, 990]` ⇒ **one wall lights the full 720-uu depth** ✅
- Annex: #5 covers `x ∈ [768, 2592] · y ∈ [−132, 1692] ⊇ [840,1680] × [570,990]` ✅
- Corridor: #6 covers `y ∈ [−1227, +597] ⊇ [−1140, +510]` lengthwise ✅ — ⚠️ **but only `x ∈ [−1644, +180]` across**, so the passage's **east half (x +180..+768) is lit by falloff and hall spill, not directly.** 🚩 **REPORTED, NOT HIDDEN.** The fix is boarded in the code comment: the mirrored anchor **`(+768, −315, 954)` yaw 180** completes a facing pair. It is out only because `MaxTorchesPerCastle` is **6 by law** (`WR-§4`: 6 ⇒ 12 lights in the level). Adding it = one array entry + one cap bump, both `EditDefaultsOnly`, **no recompile**.

### 2.4 ⭐ THE COMMANDER NPC ANCHOR — `(−465, 810, 174)`, yaw **−90**

- **X = −465** — the grand hall's own centre.
- **Y = 810** — the midpoint of the hall's **northern half** `(630 + 990)/2`, leaving **180 uu** to the back wall.
- **Z = 174** — the interior floor. `SK_Sorcerer` is feet-origin and `SM_WarTable` is floor-contact-origin (TASK-556 spec), so both sit at relative Z 0 with no per-asset fudge (TASK-559).
- **Yaw −90** — he faces **−Y**, i.e. down the hall toward the gate corridor the player walks in through.

⭐ **WHY THE ANCHOR IS THE COMMANDER AND NOT THE TABLE — AND WHY THAT IS THE WHOLE POINT.** `ACommanderNpc` places its war table a fixed distance along the actor's **+X** (`CommanderWarTableForwardOffset`, a file-local constant in *TASK-559's* file). ⛔ **I deliberately did NOT transcribe that number.** Instead the anchor is chosen so the placement is **robust** to it:

| claim | check |
|---|---|
| at TASK-559's shipped offset the table lands at **y ≈ 610** | **within 20 uu of the hall's own centre (630)** ✅ |
| table stays **north of the corridor mouth** (y > 510) | for **any** forward offset **< 300 uu** ✅ |
| table stays **inside the hall** (y > 270) | for **any** forward offset **< 540 uu** ✅ |

⇒ **if that constant is ever tuned, this anchor does not silently go stale** — which is exactly the failure mode `SC-§34` is about, avoided without a cross-file duplicate.

⛔ **CHECKED AGAINST THE TWO PLACES THE SPEC FORBIDS:**
- **gate corridor** occupies `y −1140..+510`; he is at **y 810**, north of it, and north of the doorway overlap too.
- **the approach ramp/stair** is *outside* the shell, below the gate arch at `y ≤ −2100`; he is **2910 uu deeper in and 174 uu up**, on the flat hall floor.

📌 **Composition note, flagged not hidden:** anchor #2 sits on the north wall directly behind him, 780 uu up. That is a deliberate framing (the commander stands at the head of the hall under the central torch) and it costs nothing — this light **casts no shadows**, so there is no silhouette problem, and he is also lit by #1, #3 and #4. If Jonathan dislikes it, both the anchor and the torch are `EditDefaultsOnly`.

📌 **Interaction-gate sanity (TASK-563 consumes it):** at `InteractRadius` 400 measured from the anchor, a player at the corridor mouth (y 510) is 300 away ⇒ **in range**; a player at the hall's south wall (y 270) is 540 away ⇒ **out of range**. ⇒ **you must actually walk into the hall.** ⚠️ Reported as a *consequence*, not a design claim — the radius is TASK-559's flagged feel tunable.

### 2.5 ⭐ POST-HOC CROSS-CHECK AGAINST TASK-555'S **MEASURED** READBACKS — published while I was writing, and it upgrades every number above

TASK-555's handoff landed mid-task and is now a **published** source. ⛔ **It was not my derivation input** (§2.1 — I derived from TASK-348 ×3 under `WR-§1`), which is exactly what makes it a real check: **two independent routes, same numbers.**

| my constant | TASK-555's MEASURED readback off the written FBX | verdict |
|---|---|---|
| interior floor **z 174** | *"interior floor z **174.0** — hall_main + hall_east floor median AND max"* | ✅ **exact** |
| hall clear height **1560** | *"interior clear height **1560.0** (ceiling 1734 − floor 174)"* | ✅ **exact** |
| ⇒ mount height **954** | derived from the two above | ✅ **stands** |
| carve cutters ×3 (gate 1800, corridor 1500, springline 1410, halls 1560) | ledger row H: *"×3 (floor 58→174, gate 600→1800, corridor 500→1500, heights 470/520→1410/1560)"* | ✅ **exact** |

⭐ **AND A STRONGER CONFIRMATION THAN THE RECIPE COULD GIVE — the commander stands on measured COLLISION floor, not on a carve intention.** TASK-555 §2c measures `floor_slab_hall` at **top z 174.0**, spanning **x −2100 … 1860 · y −1170 … 1200**. My NPC anchor `(−465, 810, 174)` is inside that slab **by 450 uu in Y and 1203 uu in X**, at exactly its top face. ✅ **He is standing on real floor.**

**I then checked my anchors against BOTH of TASK-555's flagged interior defects, because `SC-§22` says a citation is a lower bound:**

| TASK-555 finding | delivered-coordinate extent | does any anchor of mine sit in it? |
|---|---|---|
| §4b — `keep_front_east` pier inside the hall carve (floor 108, not 174; ~1.1 % of hall area) | **x 738…990 · y 270…360** — the hall's **south-east corner** | ⛔ **NO.** Nearest is torch #3 at `(505, 990)` — 233 uu west **and** 630 uu north. The **NPC clears it by 1203 uu in X and 450 in Y.** ✅ |
| §4c — the corridor float (visual floor 128 under a 174 collision floor; gate-passage collision floor 145) | the gate passage + corridor | ⛔ **NO EFFECT.** My corridor anchor is a **WALL mount at z 954**; a floor-height artefact cannot reach it. ✅ |

⚠️ **Neither is mine to fix and I touched neither** — §4b is named to TASK-566's PIE nav check and §4c is a costed follow-up on the manager's desk. **Recorded only to show the anchors were checked against the delivered mesh's real defects, not just its intended shape.**

---

## 3. WHAT THE CODE DOES

| element | shipped |
|---|---|
| `TorchAnchors` | `TArray<FTransform>`, `EditDefaultsOnly`, castle-mesh-relative, 6 entries filled **in the constructor from named interior constants**, so the derivation sits beside the arithmetic instead of behind a literal |
| `MaxTorchesPerCastle` | `int32`, `EditDefaultsOnly`, `ClampMin 0`, **6** (`WR-§4`). Applied as `min(TorchAnchors.Num(), max(cap,0))` — adding anchors in a BP can never quietly multiply the level's light count; **0 is a clean kill switch** (and the answer if **D3** ever rules against runtime lights) |
| `CommanderNpcAnchor` | `FTransform`, `EditDefaultsOnly`, castle-mesh-relative |
| `TorchClassAsset` / `CommanderNpcClassAsset` | `TSoftClassPtr`, `EditDefaultsOnly` → `/Game/Blueprints/BP_Torch` · `/Game/Blueprints/BP_CommanderNpc` |
| `SpawnCastleFurnishings()` | spawns at the composed world transform, `AttachToComponent(CastleMesh, KeepWorldTransform)`, then **`Npc->InitCommanderNpc(Team)`**. **Idempotent by construction** — it clears first |
| `DestroyCastleFurnishings()` | destroys and empties — the `ASiegeBattlefieldScatter::ClearScatter` lifecycle verbatim (**destroyed, never pooled**) |
| `SpawnedTorches` / `SpawnedCommanderNpc` | `UPROPERTY(Transient)` tracking, the `SpawnedMines` / `SpawnedAncientGrounds` precedent |
| hooks | `BeginPlay()` spawns · `ApplyDestroyedState(true/false)` destroys/re-spawns · `EndPlay()` destroys (belt, §5.1) |

**The house precedent this mirrors end-to-end is `ASiegeBattlefieldScatter`:** a level-placed actor that, from its own `BeginPlay`, spawns child actors with `SpawnCollisionHandlingOverride = AlwaysSpawn` + `Owner = this`, pushes state into each with an `Init<Thing>` call, tracks them in a `UPROPERTY(Transient)` array, and **destroys rather than pools** them on reset.

### 3.1 Three engine facts I verified in the UE 5.8 source rather than assumed

1. **`FTransform` composition order.** `TransformNonVectorized.h:22` — *"C = A * B … logically first applies A then B."* ⇒ `Anchor * CastleMeshWorld` **is** the world transform. Getting this backwards is silent and catastrophic.
2. ⚠️ **`AlwaysSpawn` — MY FIRST COMMENT WAS WRONG AND I CORRECTED IT.** I wrote that the default would *"nudge the actor off its anchor."* **`Actor.cpp:342` sets `AActor`'s own default to `AlwaysSpawn` already**, so on today's classes the override changes nothing. It stays, with the honest reason written in: the spawned class is a **soft ref somebody will point at a Blueprint**, `SpawnCollisionHandlingMethod` is an `EditDefaultsOnly` property that Blueprint can change — and **`Pawn.cpp:93` defaults `APawn` to `AdjustIfPossibleButDontSpawnIfColliding`**, so anyone who ever "upgrades" `ACommanderNpc` from `AActor` to a Pawn would get a commander that **silently fails to spawn** inside the castle shell. 📌 Recorded because *"I checked and my own claim was false"* is worth more to the gate than a tidy comment.
3. **`TSoftClassPtr` surface** (`SoftObjectPtr.h`): `IsNull()`, `ToString()`, `LoadSynchronous()` returning `UClass*`, and the **explicit** `TSoftClassPtr(const FSoftObjectPath&)` ctor — all present, and the shipped `ASiegeGameMode::HeroPawnClassAsset` uses the identical shape today.

### 3.2 Null-safety — every path, because the assets and Blueprints are **expected** to be absent

| path | behaviour |
|---|---|
| `TorchClassAsset` / `CommanderNpcClassAsset` **cleared** | **silent opt-out** — that piece of furniture is simply absent (the `AttackImpactEffect` `IsNull` pattern, TASK-020) |
| set but **unresolvable** | falls back to the raw C++ class + **ONE** log line, guarded by a per-castle bool (the `ResolveHeroPawnClass` shape). ⚠️ **This is the path that runs today** — see the finding in §6 |
| `TorchAnchors` empty / cap 0 | no torches, no log, no branch taken |
| `SpawnActor` returns null (torch) | `Warning`, **`continue`** — one torch fewer can never unlight the hall |
| `SpawnActor` returns null (NPC) | `Warning`, castle plays exactly as today |
| `CastleMesh` or `GetWorld()` null | early return, nothing spawned |
| a stale/GC'd entry in the arrays | `IsValid()` guard before every `Destroy()` |

⛔ **No `check`, no `ensure`, no crash on any of them.**

---

## 4. 📌 M8 DECLARATION (`WR-§8`) — ⛔ **NOT THE USUAL BOILERPLATE, AND THE UNUSUAL PART IS THE POINT**

| M8 question | answer |
|---|---|
| new replicated property | ⛔ **none** |
| new replicated class | ⛔ **none** — `ACastle` keeps `bReplicates = true` / `bAlwaysRelevant = true` (Tier A) exactly as shipped; `ATorch` and `ACommanderNpc` are **Tier C**, declared in their own headers |
| new relevancy tier | ⛔ **none** |
| new RPC | ⛔ **none** — the batch's two are TASK-563's, on `ASiegePlayerController` |
| `GetFirstPlayerController()` | ⛔ **zero uses** |
| `HasAuthority()` guard added | ⛔ **NONE — DELIBERATELY, AND IT IS THE OPPOSITE OF THE USUAL RULE** |

**⛔⛔ WHY THE MISSING GUARD IS THE CORRECT ANSWER AND NOT AN OVERSIGHT — stated loudly in the class doc *and* here, because it will look like a violation to someone reading fast.** `ACastle::BeginPlay` runs on the server **and** on every client. The furnishing carries **no gameplay truth**: it is a mesh, a shadowless light and two `EditDefaultsOnly` numbers that are class defaults, identical on both machines by construction. Each machine builds its own local set — the `AAncientGround` / `AGoldNode` precedent verbatim. **A `HasAuthority()` guard here would leave every client with an unlit castle and no commander.** ⇒ *"Every mutation site carries a `HasAuthority()` guard"* binds **mutations of gameplay truth**, and this lane mutates none.

**Verification sweep over my own diff (added lines only):**
```
$ git diff -U0 -- Castle.h Castle.cpp | grep '^+' | grep -E "UPROPERTY\(Replicated|DOREPLIFETIME|UFUNCTION\(Server|UFUNCTION\(Client|UFUNCTION\(NetMulticast|bReplicates|bAlwaysRelevant|HasAuthority"
RAW COUNT: 2   — Castle.h (class doc), Castle.h (ApplyDestroyedState doc)
```
✅ **BOTH HITS ARE COMMENT PROSE — the two places I explain why there is no guard. ZERO code lines.**

---

## 5. ⚠️ DECLARED DEPARTURES AND ADDITIONS (`SC-§15`) — every one, named

1. ⭐ **The lifecycle hook is `ApplyDestroyedState`, not `HandleDestroyed`/`ResetCastle`.** §1. **The most important line in this handoff.**
2. **`EndPlay` override — AN ADDITION, and a belt.** `AActor::Destroy` **detaches** attached actors rather than destroying them, so an explicit `ACastle::Destroy()` would strand the furniture in mid-air. Nothing calls it today; this exists so nothing has to remember not to. World teardown makes it a no-op on the ordinary path.
3. **Two `TSoftClassPtr` class fields + two resolver helpers — AN ADDITION over the `names:` block.** The `names:` block lists `TorchAnchors` · `MaxTorchesPerCastle` · `CommanderNpcAnchor`. Spec item (3) says *"a missing **class** … logs once"*, which only means anything if the class is a reference rather than a hard `StaticClass()`. Reason it is a **soft ref to a Blueprint with a C++ fallback**, not a bare `TSubclassOf`: `WR-§4`/`WR-§5` put `BP_Torch` / `BP_CommanderNpc` in `Content/Blueprints/`, and **TASK-558 explicitly parks TASK-556's measured flame-centre offset on `BP_Torch`'s defaults.** If the castle spawned the raw C++ class, **every Blueprint default would be dead on arrival** and someone would have to edit `ACastle` at integration time. This keeps the seam open with **zero later code edits**. ⚠️ QA is entitled to overrule; the fallback if overruled is `TSubclassOf<ATorch>` defaulting to `ATorch::StaticClass()`, and it costs the BP tuning surface.
4. **The fallback log is `Log`, not `Warning`.** The Blueprints' absence is the **expected** state (§6), and a warning that always fires is a warning everyone learns to ignore. A genuinely failed *spawn* is still a `Warning`.
5. **A one-line `Log` summary per furnishing pass** (counts + team + cap). Not in the spec; it exists because **TASK-569's PIE matrix item (e)** — *"torches spawn … and do not survive a Play Again as orphans"* — becomes a log read instead of an eyeball count.
6. **`SpawnCastleFurnishings()` is idempotent by clearing first**, rather than latching a "already furnished?" bool. A latch is one more thing a reset path can desync; a teardown+rebuild cannot double and cannot orphan. Cost is 7 actors on a redundant call, on a path that is already rebuilding a castle.

---

## 6. ⛔ FINDINGS — REPORTED, ⛔ NOT CODED AROUND (`SC-§15`, `SC-§22`)

### F1 — ⚠️ **NO TASK IN THIS BATCH AUTHORS `BP_Torch` OR `BP_CommanderNpc`, AND TASK-558'S MEASURED-OFFSET LANDING SITE IS THEREFORE UNOWNED**

I read the downstream specs rather than assuming: **TASK-566** imports meshes + materials · **TASK-567** the crumble trio · **TASK-568** `IA_WarMap` + `IMC_Hero` + `WBP_WarMap` · **TASK-569** `DA_BattlefieldScatter` + PIE. **TASK-570's expected commit surface lists neither Blueprint.** But `handoffs/TASK-558-programmer.md` §7 says in terms: *"NAMED FORWARD TO TASK-569 / TASK-562: set `BP_Torch → TorchLightRelativeOffset` from TASK-556's published `flame_centre_uu`."*

⇒ **Consequences, all benign, none silent:**
- The **C++ fallback is the shipped behaviour**, and it is complete: `ATorch` derives its light offset from its own mesh bounds (TASK-558 measured the residual at ≈2.6 % of the attenuation radius on today's geometry — *a polish item, not a defect*), and `ACommanderNpc` runs at its law defaults.
- **TASK-556's `flame_centre_uu` currently has nowhere to land.** ⛔ **I did NOT "solve" this by transcribing the number into C++** — that is exactly what TASK-558 refused and `SC-§34` forbids.
- **The ask is one line for whoever owns integration:** create `BP_Torch` (parent `ATorch`) and set `TorchLightRelativeOffset`. My soft ref already points at `/Game/Blueprints/BP_Torch` and will pick it up with **no code change**. ⛔ **Manager/build-master call — I did not board it myself.**

### F2 — 📌 `WR-§9`'s playtest sheet is missing two designed outcomes this task creates

Both belong on Jonathan's sheet so a report of one is read correctly (⛔ I did not edit `CONVENTIONS.md` — that is the manager's file):
- **The gate corridor's east half is dimmer than its west half** (§2.3). Designed at 6 torches; the mirrored anchor is boarded in-code.
- **The war table and commander are walk-through** (no collision, per TASK-559's declared choice #4). TASK-559 already recommended this as outcome 8; I am seconding it from the placement side, because it is *my* anchor that puts the table in a walkway.

### F3 — ✅ The `Team`-timing hazard TASK-559 handed me: **RE-CONFIRMED AT THE CODE, and I state the condition it rests on**

TASK-559 §8: *"`ACastle::Team` is `COND_InitialOnly` … on a client `BeginPlay` can run BEFORE initial replication settles … that argument is a property of the level, not of the code, and it is TASK-562's to re-confirm."*

**Re-confirmed, three legs:**
1. **Every castle is LEVEL-PLACED.** `grep -rn "SpawnActor<ACastle>\|ACastle>" Source/ | grep -v "TActorIterator|FindNearestCastle|Cast<ACastle>"` ⇒ **2 hits, both `TObjectPtr<ACastle>` / `IsA<ACastle>`; ZERO runtime spawns.** ⇒ each machine's own copy of `L_Arena` deserialises the correct `Team` **before any replication arrives**; the rep is the belt `Castle.h:287` documents, not the source.
2. ⭐ **The shipped code already depends on exactly this, ONE HOOK EARLIER.** `PostInitializeComponents` selects the team interior nav area from `Team` **before** `BeginPlay`, and **TASK-350 PIE-verified the result on both instances** (blue open / red closed). A push at `BeginPlay` is strictly safer than something already proven in engine.
3. ⛔ **THE CONDITION, NAMED:** this rests on *"no castle is ever runtime-spawned."* If one ever is, **re-open this** — it is written into `BeginPlay`'s comment, not just here.

### F4 — 📌 Out-of-ownership items I saw and did **not** touch
- TASK-557 §S8's two stale `2460` cross-notes in **`SiegePlayerController.cpp`** — still there, still **TASK-563's** to fix while it is in that file.
- TASK-557's flagged rows **S1** (`PlayerStart` inside the 9× castle), **S2** (`DefendRadius`), **S3/S4/S6/S7** — none is in my `names:` block, none touched, all still owed a manager ruling.

---

## 7. ⛔ `SC-§33` DISCHARGE — THE PASTED CALL-SITE GREPS (the gate re-runs them)

**THE CLAIM: this task adds NO defaulted parameter to any function, new or existing — and changes NO existing signature at all.**

### Leg 1 — the complete new signature surface (5 declarations, §0)
| # | signature | trailing default? |
|---|---|---|
| 1 | `virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;` | **none** — engine override, signature fixed by `AActor` |
| 2 | `void SpawnCastleFurnishings();` | **none** — no parameters |
| 3 | `void DestroyCastleFurnishings();` | **none** — no parameters |
| 4 | `UClass* ResolveTorchClass();` | **none** — no parameters |
| 5 | `UClass* ResolveCommanderNpcClass();` | **none** — no parameters |

### Leg 2 — the mechanical sweep for `=` inside any parenthesised list, added lines only, both owned files
```
$ git diff -U0 -- Castle.h Castle.cpp | grep '^+' | grep -v '^+++' \
    | grep -vE '^\+\s*(\*|//|/\*)' | grep -nE "\w+\s*\([^)]*=[^)=]*\)"
RAW COUNT: 10
```
⛔ **ALL TEN ARE FALSE POSITIVES OF THE REGEX; ZERO IS A PARAMETER DECLARATION:**

| hit | line | classification |
|---|---|---|
| `if (UClass* LoadedClass = TorchClassAsset.LoadSynchronous())` | `Castle.cpp` | **(i)** if-init statement |
| `if (UClass* LoadedClass = CommanderNpcClassAsset.LoadSynchronous())` | `Castle.cpp` | **(i)** if-init statement |
| `if (UClass* TorchClass = ResolveTorchClass())` | `Castle.cpp` | **(i)** if-init statement |
| `if (UClass* CommanderClass = ResolveCommanderNpcClass())` | `Castle.cpp` | **(i)** if-init statement |
| `for (int32 AnchorIndex = 0; AnchorIndex < TorchCount; ++AnchorIndex)` | `Castle.cpp` | **(i)** loop init |
| `UPROPERTY(EditDefaultsOnly, Category = "…\|Furnishing")` **× 4** | `Castle.h` | **(i)** UHT macro, named specifiers |
| `UPROPERTY(EditDefaultsOnly, Category = "…", meta = (ClampMin = "0"))` | `Castle.h` | **(i)** UHT macro |

### Leg 3 — every NEW symbol, swept over the whole `Source/` tree
```
$ grep -rnE "\b(TorchAnchors|MaxTorchesPerCastle|CommanderNpcAnchor|TorchClassAsset|CommanderNpcClassAsset|SpawnedTorches|SpawnedCommanderNpc|SpawnCastleFurnishings|DestroyCastleFurnishings|ResolveTorchClass|ResolveCommanderNpcClass|bLoggedTorchClassFallback|bLoggedCommanderNpcClassFallback)\b" Source/ | grep -v "Siegebound/Castle\."
RAW COUNT: 6 hits, 3 files
  CommanderNpc.h:104   — comment: "…from its CommanderNpcAnchor…"
  Torch.h:23, Torch.h:42, Torch.h:186, Torch.cpp:47, Torch.cpp:143  — comments naming TorchAnchors / MaxTorchesPerCastle
```
✅ **ALL SIX ARE COMMENT PROSE in TASK-558/559's files — their forward references to me. ZERO code collisions, ZERO shadowing.**

### Leg 4 — engine functions I call **at their trailing default**, classified rather than ignored
| call | defaulted parameter left alone | classification |
|---|---|---|
| `AttachToComponent(CastleMesh, KeepWorldTransform)` | `FName SocketName = NAME_None` | **(ii) DELIBERATELY AT THE DEFAULT** — the anchor is a transform in the mesh's own space, **not** a socket; `SM_Castle` has no furnishing sockets and adding one would move the placement into an art asset |
| `TSoftClassPtr::LoadSynchronous()` | `ELoadFlags LoadFlags = LOAD_None` | **(ii) DELIBERATELY AT THE DEFAULT** — matches the four shipped soft resolves already in this file |

⛔ **Neither is a parameter *I* added, so `SC-§33` does not bind either; they are enumerated because the law's lesson is that "byte-identical call sites" is a benefit and never the audit.**

### Leg 5 — the batch's standing criteria, run against MY diff
```
$ git diff -U0 -- Castle.h Castle.cpp | grep '^+' \
    | grep -E "Assistant|SpendGold|ZoneA|Prompt|Llama|Gbnf|GBNF|WarMap|Reveal|GetFirstPlayerController|ServerRequest|ClientReceive"
RAW COUNT: 0
```
✅ **ZERO.** ⇒ **This task spends ZERO prompt characters**, touches no Zone builder, reaches no gold path, and cannot gate the console. ⛔ **`Tests/SiegeAssistantZoneATest.cpp` untouched.** ⛔ **No token figure quoted.** ✅ *A sweep that finds nothing is a RESULT and is reported as one* (`SC-§22`).

---

## 8. `SC-§34` — THE LEDGER ROWS THIS TASK OWNS

⭐ **THIS TASK PRODUCES NO `WR-§2` ROW, AND THAT IS A CONCLUSION, NOT AN OMISSION.** The law's own question is *"if the castle had always been 9×, would this constant have been written differently?"* — **every constant added here was BORN at the 9× scale.** None existed before this batch; none was derived from the old castle's size; **nothing here can be stale.** TASK-557 owns the ledger and I add no row to it, and I re-touched none of its eight initialisers (§0).

| # | constant | disposition |
|---|---|---|
| A | `TorchWallMountZ` = 954, `InteriorFloorZ` = 174, `HallClearHeightZ` = 1560, the hall/annex/corridor extents | **(i) BORN AT 9×** — each computed from `WR-§1`'s own published 9× figures, not scaled from a 3× predecessor |
| B | the 6 torch anchors + the NPC anchor | **(i) BORN AT 9×** — ⛔ **no pre-scale number was copied**, which is the trap `WR-§4`'s *"do NOT copy pre-scale numbers"* names |
| C | `MaxTorchesPerCastle` = 6 | **(ii) NOT SCALED** — a count of light sources, not a dimension. `WR-§4` states it as law |
| D | the mount height 954 | **(ii) NOT A BODY NUMBER — and I checked which it was.** It is keyed to the **ROOM's** clear height (a shell feature that scaled), not to a body reaching up, so it correctly follows the castle. ⚠️ Recorded because the human-scale exemption *looks* like it should apply and does not |
| E | `InteractRadius`, the war-table forward offset | **(iii) NOT MINE** — TASK-559 owns both and marked both **(ii) human-scale, deliberately unchanged**. ⛔ I did not touch, override or duplicate either |
| F | the torch attenuation radius | **(iii) NOT MINE** — TASK-558's `EditDefaultsOnly` tunable. ⛔ **Deliberately NOT duplicated into `Castle.cpp`**; used only as reasoning (§2.2) |

---

## 9. ⚠️ WHAT QA (TASK-565) SHOULD ATTACK — ranked, the top three are the ones I would

1. ⭐⭐ **§1 — IS `ApplyDestroyedState` THE RIGHT HOOK?** This is my one override of the spec's literal text. **Re-run it yourself:** confirm `HandleDestroyed` is only reachable through `TakeDamage`'s authority gate, that `ResetCastle` opens with its own, and that `OnRep_Destroyed` is the client's only route. **If I am wrong, the fix is two lines — but if I am right and it were hooked as specced, every client leaks a full furniture set per replay and nothing logs it.**
2. ⭐⭐ **§2 — THE ANCHOR SPACE AND THE ARITHMETIC.** The whole placement rests on (a) `Anchor * CastleMeshWorld` being the right composition order, and (b) the anchors living in the same space as `GateBlockerRelativeLocation`. **Both are checkable without a compiler**: the engine comment at `TransformNonVectorized.h:22`, and the fact that `GateBlockerVolume` is `SetupAttachment(CastleMesh)` + `SetRelativeLocation`. ⚠️ **If the space is wrong, the torches are inside the masonry and nobody finds out until PIE.**
3. ⭐ **§5.3 — THE SOFT-CLASS ADDITION.** Two fields beyond the `names:` block. I believe it is right (it is the only thing that keeps `BP_Torch`'s defaults alive) and I have given the full reasoning; **it is a judgement call and QA may overrule.**
4. **F1 — the unowned `BP_Torch` / `BP_CommanderNpc`.** Please rule on whether this needs a boarded task before TASK-570, or whether shipping on the C++ fallback is acceptable for this batch. ⛔ I did not board it myself.
5. **The `EndPlay` addition (§5.2).** Cheap belt or unnecessary surface? I judged `AActor::Destroy`-detaches-children to be worth one guarded call.
6. **Pre-compile scans:** forward declarations only in `Castle.h` (`ATorch` / `ACommanderNpc`) with full includes in the `.cpp` — the `BattlefieldScatter.h` ↔ `.cpp` precedent for `TArray<TObjectPtr<AGoldNode>>`. No inherited-member shadowing (§7 leg 3 sweep). No parenthesised `TSoftObjectPtr` local anywhere (most-vexing-parse). Every soft resolve null-checked (§3.2). Braces balance 0 in both files; both are valid UTF-8 with no BOM; every non-ASCII character is inside a comment or a `TEXT()` literal, matching the file's shipped style.

---

## 10. ⛔ WHAT I DID **NOT** VERIFY, AND WHY (`SC-§32` posture — say it rather than imply green)

1. ⛔ **NOTHING HERE HAS BEEN COMPILED.** TASK-566 owns the batch's only compile. Every API claim is a **code reading** — `SpawnActor<T>(UClass*, const FTransform&, const FActorSpawnParameters&)` (`World.h:3823`), `AttachToComponent` (`Actor.h:2015`), `FAttachmentTransformRules::KeepWorldTransform` (`EngineTypes.h:79`), `FSoftObjectPath(const WIDECHAR*)` (`SoftObjectPath.h:94`), the whole `TSoftClassPtr` surface — each read in the UE 5.8 headers **and** matched against a call that compiles in this module today (`DamageNumberActor.cpp:63` and `SummonedUnit.cpp:3207` for the transform-overload spawn; `SiegeGameMode.cpp:44/181` for the soft class). **The compiler has not agreed yet.**
2. ⛔ **NO PIE. The anchors have not been LOOKED AT.** They are arithmetic over a published geometry table, and the geometry they describe **does not exist as an asset yet** (TASK-555 authors the 9× mesh; TASK-566 imports it). ⚠️ **If TASK-555's delivered mesh differs from the manifest recipe, every anchor moves with it** — that is TASK-569's PIE matrix item (f), and both anchor sets are `EditDefaultsOnly` precisely so the correction is a defaults edit, not a recompile.
3. ⛔ **`SM_Torch` and `SM_WarTable` DO NOT EXIST**, so the null-safe path is the one that will run first; the resolved path is untested by anything.
4. ⚠️ **A SUBTLETY I READ THE ENGINE ABOUT AND CANNOT SETTLE WITHOUT A RUN:** `AWorldSettings::NotifyBeginPlay` sets `bBegunPlay` **after** its dispatch loop, so an actor spawned *during* another actor's `BeginPlay` does not get `DispatchBeginPlay` from `PostActorConstruction` — it depends on the live `FActorIterator` reaching the appended entry. ⭐ **The design does not rest on the answer:** `SpawnActor` always runs `OnConstruction`, and **both** `ATorch` and `ACommanderNpc` resolve their assets and apply their tuning there (their `BeginPlay` is a re-assert). The one thing that *must* happen — the **team push** — is my own explicit call, not a `BeginPlay` side effect. ⚠️ **Named so nobody has to rediscover it if a torch ever looks unlit at first frame.** The shipped `ASiegeBattlefieldScatter` spawns from `BeginPlay` the same way.
5. ⛔ **No navmesh claim.** `WR-§3` makes interior navigability TASK-569's **measurement**. What I *can* state is that this task cannot have harmed it: both spawned classes ship `NoCollision` + `SetCanEverAffectNavigation(false)` (verified by grep in their `.cpp`s), so the furniture carves nothing in the one interior units must walk through.
6. ⛔ **No frame-rate figure, no luminance figure, no token figure.** `WR-§4`'s perf clause: no FPS baseline exists for this project, and the perf verdict is Jonathan's eye at TASK-571.

---

## 11. CROSS-TASK NOTES

- **→ TASK-563:** the NPC is spawned per castle and **destroyed/re-spawned across Play Again** ⇒ ⛔ **never cache the pointer across a match reset; always ask `ACommanderNpc::FindCommanderNpcForTeam`.** ⛔ The accessor is **`GetCommanderTeam()`** and `Cast<ITeamAgent>(Npc)` **returns null by design.** ⚠️ TASK-557 §S8 leaves you two stale `2460` cross-notes in `SiegePlayerController.cpp`.
- **→ TASK-564:** the anchors are constructor-filled CDO data, so `ACastle::StaticClass()->GetDefaultObject<ACastle>()` is enough to assert **count == 6**, the Z parity, and that no anchor lies inside the corridor volume — **without a world**. The spawn path itself needs a world + a castle. ⚠️ **If you disagree with any anchor, that is a FINDING, not a silent edit.**
- **→ TASK-566:** compiles this. The two new includes (`Siegebound/Torch.h`, `Siegebound/CommanderNpc.h`) mean `Castle.cpp` now depends on TASK-558's and TASK-559's files — **all three must be in the same compile**.
- **→ TASK-569:** ⭐ **your PIE matrix items (e) and (f) are a log read:** `ACastle '<name>' (<team>): furnished — N of M torch anchors spawned (cap 6), commander spawned`. Expect **two** lines per match (one per castle) and **two more after every Play Again** — and ⛔ **no third set**, which is the orphan check. ⚠️ **Also please action F1** (create `BP_Torch` / `BP_CommanderNpc` and land TASK-556's `flame_centre_uu`) **or confirm it is deliberately deferred.**
- **→ TASK-571 (Jonathan):** flagged for your feel pass — the six torch positions and the 6-torch cap; the commander's spot at the head of the hall; the dimmer east half of the gate corridor (§2.3, one array entry from fixed); and the walk-through war table (F2).
