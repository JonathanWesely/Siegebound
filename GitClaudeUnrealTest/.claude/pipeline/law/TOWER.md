<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ THE CLIMBABLE TOWER — a 30-gold building ranged units ascend for height (2026-08-30) — namespace **TOWER-§**

📌 **BORN WITH ITS NAMESPACE PREFIX (`TOWER-§N`).** Cite as `TOWER-§3`, never a bare `§3`.

**Trigger — Jonathan's directive, verbatim (2026-08-30):** *"Also, lets add a climable tower as a building card, make the cost 30 gold, and any ranged unit can climb it, so you will have to rig a climbing animation for all the ranged units."*

### TOWER-§0 ⭐⭐ THE MEASUREMENT THAT RESIZES THE WHOLE ASK — **THE RANGED UNITS DO NOT HAVE THREE RIGS. THEY HAVE ONE.**

> ### ⭐ **"RIG A CLIMBING ANIMATION FOR ALL THE RANGED UNITS" COSTS **ONE** ANIMATION, ⛔ NOT THREE — AND THIS WAS VERIFIED IN THE CONTENT TREE, ⛔ NOT ASSUMED.**

- **`SK_Archer`, `SK_Longbowman`, `SK_Wizard` are all bound to the SINGLE shared skeleton `/Game/Characters/SK_Footman_Skeleton`** (the 21-bone SiegeBiped rig), and **the entire rigged fleet shares ONE AnimBlueprint, `ABP_Footman`** (`SummonedUnit.cpp:60`; CONVENTIONS *"`ABP_Footman` is the ENTIRE rigged fleet's locomotion asset"*; corroborated at TASK-297/305). **The retarget chain already exists and is shipped: `IK_MeshyBiped` → `RTG_MeshyBiped_to_SiegeBiped` → `IK_SiegeBiped`.**
- ⇒ ⭐ **ONE clip, authored once on the shared rig, named `A_SiegeBiped_Climb` per the existing shared-skeleton naming clause** (CONVENTIONS *"Shared-skeleton locomotion retargeted across several units may instead be authored once as `A_SiegeBiped_<Action>`"*), **covers all three units.** ⛔ **A per-unit `A_Archer_Climb` / `A_Longbowman_Climb` / `A_Wizard_Climb` set is REFUSED as redundant** — three clips on one identical rig for three near-identical silhouettes.
- ⚖️ **THE HONEST RESTATEMENT OF THE COST, AND IT INVERTS THE OBVIOUS ASSUMPTION: the RIG is the CHEAP half. The EXPENSIVE half is the ASCENT MECHANIC** — a scripted ladder-mover, its state in the shared ABP, its interruption/death/destruction cases, and its interaction with `ACharacter` movement and the navmesh. ⭐ **That distinction is what lets a playable tower ship without a rig, and it is the reason this namespace is decomposed the way it is.**

### TOWER-§1 ⛔⛔ THE SHIP-FIRST LAW — **A RIG MAY NOT BLOCK A PLAYABLE TOWER**

> ### ⚖️ **THE TOWER'S REASON TO EXIST IS ELEVATION, AND `HIGH-§` DELIVERS ELEVATION WITH ⛔ ZERO ANIMATION WORK. THEREFORE THE CARD, THE STRUCTURE, THE ASCENT AND THE OCCUPANCY SHIP AND ARE PLAYTESTED FIRST; THE BESPOKE CLIMB CLIP FOLLOWS.**

- ⛔ **THE RIG IS ⛔ NOT CANCELLED, ⛔ NOT DEFERRED INDEFINITELY, AND ⛔ NOT QUIETLY DROPPED. He asked for it; it stays the target.** ✅ **The only question this law settles is WHAT SHIPS FIRST**, and it is boarded as a real task with a real cost so it cannot be forgotten.
- ⭐ **THE SHIP-FIRST SHAPE IS A WALKABLE RAMP, AND IT IS ⛔ NOT A HACK:** a unit **walks up** a ramp using `A_<CardID>_Walk`, **which all three ranged units already have**. It reads correctly — *a unit walking up a tower ramp IS climbing the tower* — it needs **no new animation, no ascent mechanic, no ABP edit, and no `ACharacter` movement override**, and `HIGH-§` grants the damage bonus from the unit's genuinely-higher `GetActorLocation().Z` with **zero tower-awareness** (`HIGH-§3`).
- ⛔ **AN "INTERIM GARRISON/TELEPORT" IS REFUSED AS THE FIRST SHIP.** A unit snapping to a platform is the thing that would look broken; **the ramp looks correct on day one** and — ⭐ **decisively** — **the ramp is not thrown away when the climb clip lands.** ⚖️ *Ship-first work that must be deleted to make room for the real feature is a detour; ship-first work the real feature builds on is a foundation.*

### TOWER-§2 ⛔⛔ THE RAMP IS **ARITHMETIC**, ⛔ NOT AN ARTIST'S GUESS — DERIVED FROM THIS PROJECT'S OWN NAV CONFIG

**All four agent figures are the project's own, quoted from `Config/DefaultEngine.ini:287-290` — *"AgentMaxStepHeight stays at the engine/project default 35.0"*, *"Agent radius/height/slope untouched (engine defaults 34/144/44)"* — plus `NavMeshResolutionParams[1]=(CellSize=32.0, CellHeight=20.0, AgentMaxStepHeight=35.0)` at `:340`.**

| Nav parameter | Value | What it forces on the mesh |
|---|---|---|
| `AgentMaxSlope` | **44°** | ~~⛔ **The ramp must be under 44° or it is not walkable at all**~~ ⛔⛔ **STRUCK 2026-08-30 — 44° IS ⛔ NOT THE CEILING. THE REAL CEILING IS `atan(20/32)` = 32.005°** (`TOWER-§2a`). This setting only *marks* triangles walkable; Recast's ledge filter then runs **unconditionally** in voxels and shreds anything steeper. |
| `AgentMaxStepHeight` | **35 uu** | ~~⛔⛔ **STAIRS ARE REFUSED. A 35 cm riser is unusably small at this scale — a stepped mesh will simply not generate a nav surface.**~~ ⛔ **THE REFUSAL STANDS; THE REASON WAS WRONG AND IS STRUCK** — a 35 uu riser most likely *would* generate (`rcFilterLowHangingWalkableObstacles` re-marks it). **Stairs are refused on COST: ~35 steps for 1,200 uu of rise** (`TOWER-§2a`). ⇒ **A SMOOTH RAMP, always.** |
| `AgentRadius` | **34 uu** | needs > 68 uu of clear width; with `CellSize 32` voxelization, ⇒ ~~⭐ **RAMP WIDTH ≥ 200 uu**~~ **≥200 is a VALID FLOOR, REFINED ⛔ not refuted: the shipped deck is ⭐ 300 uu** — ledge + erosion cost **128 uu** (2 cells/side) whatever the width (`TOWER-§2a`) |
| `AgentHeight` | **144 uu** | ⇒ ⭐ **vertical clearance over the walking surface ≥ 200 uu** — ⛔ **but ⛔ NOT because of the 144: `ASummonedUnit`'s REAL capsule is 176 uu tall** and the nav agent is **shorter than the actual unit** (`TOWER-§2a`) |

- ~~⭐ **THE DERIVED GEOMETRY BUDGET** (platform height **1,200 uu** — `TOWER-§3`): at **40°** (a deliberate 4° margin under the 44° limit) the horizontal run is `1200 / tan(40°) = 1200 / 0.8391 =` **≈1,430 uu**. At a gentler **30°** it is `1200 / 0.5774 =` **≈2,078 uu**. **The mesh task picks inside `[1,430 … 2,080]` and states the slope it used.**~~
  - ⛔⛔ **STRUCK 2026-08-30 (TASK-725's spike, measured at the engine source). 40° IS REFUTED AND THE RANGE IS WITHDRAWN — THE MESH TASK PICKS ⛔ NOTHING. THE SLOPE IS 30°, THE RUN IS 2,078 uu, AND BOTH ARE PINNED IN `TOWER-§2a`.** ⚖️ *The old bullet's arithmetic was right in every digit; the conclusion it licensed — "anything under 44° works, choose your margin" — was wrong, and a correct number reached through a wrong reason fails the moment the reason is tested.*
- ✅⭐ **A SINGLE STRAIGHT EXTERNAL RAMP IS THE RULED SHAPE. A SPIRAL IS REFUSED FOR THE FIRST SHIP.** ~~⚖️ **Why, and it is a measurement not a preference:** `CellHeight = 20` is coarse, and a spiral passes **over itself** — flights closer than ~200 uu vertically risk Recast **merging the spans** into one unwalkable blob. **A straight ramp has ⛔ no self-overlap and therefore ⛔ no span-merge failure mode.**~~
  - ⛔ **THE REFUSAL STANDS; THE SPAN-MERGE REASON IS STRUCK 2026-08-30 AS OVERSTATED** — Recast heightfields are **multi-span by design**, and a turn needs only the same ≥200 uu clearance the straight ramp already needs. ⭐ **THE REAL DISQUALIFIER IS GEOMETRIC AND IT IS WORSE THAN THE ONE WE FEARED: on a helix the INNER EDGE IS STEEPER THAN THE CENTRELINE.** To fit a 300 uu deck the outer radius must be ≥ ~350 uu, so a 30° centreline puts the inner edge **over the 32.005° ceiling** — shredded by the same ledge clause, **on the inside of the curve only, INVISIBLY.** ⚠️ *A failure that shows up as "some units sometimes don't make it up" is far more expensive than one that fails outright.*
- ⛔ **THIS IS NOT A PREDICTION OF SUCCESS.** ⚠️ Whether Recast **actually** generates a walkable surface on a **runtime-spawned** ramp at this arena's coarsened settings is an **open question**, and it is answered by a **cheap diagnose-first spike from config + source arithmetic BEFORE any mesh is authored** — ⛔ never by authoring a tower and hoping. ⚖️ **The spike's ruling is the mesh task's specification.**
  - ✅ **ANSWERED 2026-08-30 (TASK-725): it GENERATES. There is ⛔ NO code path in the generator that distinguishes a building from a hill** — `ABuilding::VisualMesh` (`BlockAll` + `SetCanEverAffectNavigation(true)`, `Building.cpp:47`/`:55`) and the scatter hills register in the same octree and go through the same rasteriser. **A wall "carves" because its faces are 90°, i.e. over `AgentMaxSlope`; a 30° face is under it and is therefore walkable. Same pass, same rules.** ⭐ **The shipped hills are the existence proof, and the whole no-rig path rested on this one claim.**

### TOWER-§2a ⛔⛔ THE GEOMETRY LAW, **MEASURED** (2026-08-30, TASK-725's spike) — ⭐ **AND THE REASONS MATTER MORE THAN THE NUMBERS**

> ### ⚠️⚠️ **RE-SCOPED 2026-09-01 BY `TOWER-§8` — ⛔ NOT STRUCK, ⛔ NOT DELETED, AND ⛔ NOT DEMOTED. READ THIS BOX BEFORE YOU READ THE TABLE.**
>
> **Jonathan replaced this tower's ramp with a LADDER.** ⇒ **The two rows that are ABOUT THIS TOWER'S RAMP — the `30.0°` slope and the `2,078 uu` run — are MOOT FOR `SM_WatchTower`.** There is no longer a ramp on that mesh for them to describe.
>
> ⭐⭐ **EVERYTHING ELSE IN THIS SUB-SECTION SURVIVES AT FULL FORCE, AND IT IS THE MOST VALUABLE THING THIS NAMESPACE PRODUCED.** The **32.005° ceiling**, its derivation, the **176 uu real capsule** vs the nav agent's 144, the **128 uu ledge+erosion tax**, the **solid-wedge / thin-deck** finding, the **flush-junction ≤ 40 uu** rule and the **serialized-`L_Arena` asymmetry** are **findings about THIS PROJECT'S NAVMESH, ⛔ not about one building.** ⇒ ⛔⛔ **THEY BIND EVERY FUTURE RAMP, HILL, SLOPE, PLATFORM AND WALKABLE SURFACE IN SIEGEBOUND, INCLUDING THE LADDER TOWER'S OWN PLATFORM DECK** (`TOWER-§8` sizes that deck from the 128 uu tax in this very table).
>
> ⚖️ ***A measurement is not invalidated by the design that prompted it being replaced.*** **The 10× arena silently dropped this project's walkable ceiling from 46.5° to 32° and nothing has hit it because every shipped hill sits at 27°. That trap is still armed, still undocumented anywhere else, and the next person to author a slope still needs this page.** ⛔ **Deleting it to tidy up after a redesign would re-arm the exact trap it was written to disarm.**
>
> 📌 **Citation rule from here on:** cite `TOWER-§2a` for **the navmesh constraints**; ⛔ do **not** cite it as the Watch Tower's build spec — that is now **`TOWER-§8`**.

> ### ⭐⭐ **THE REAL WALKABLE CEILING IS `atan(20/32)` = **32.005°**, ⛔ NOT THE 44° `AgentMaxSlope` SUGGESTS.**
> ### ⚠️ **`AgentMaxSlope` ONLY *MARKS* TRIANGLES (`rcMarkWalkableTriangles`). `rcFilterLedgeSpans` THEN RUNS ⛔ UNCONDITIONALLY, IN VOXELS, AND IT IS THE BINDING CONSTRAINT.**

- **The mechanism, so it can be re-derived rather than believed:** `rcFilterLedgeSpansImp`'s clause B nulls a span when `(asmax − asmin) > walkableClimb`. `walkableClimb = ceil(AgentMaxStepHeight / CellHeight) = ceil(35/20) =` **2 voxels**. Rise per cell at slope θ is `CellSize × tan(θ) = 32·tan(θ)` uu; span tops are `ceil()`-quantised, so once rise-per-cell exceeds **1 voxel** a step of 2 lands beside a step of 1 ⇒ sum **3 > 2** ⇒ the cell is **nulled**. ⇒ **survives iff `32·tan(θ) ≤ 20`, i.e. θ ≤ 32.005°.**
- ⚠️⚠️ **AND THE PART THAT MAKES THIS A STANDING LESSON, ⛔ NOT A TOWER DETAIL: THE COARSENING FOR THE 10× ARENA SILENTLY DROPPED THE CEILING FROM 46.5° TO 32°, AND NOTHING HAS HIT IT BECAUSE EVERY SHIPPED HILL SITS AT 27°.** At engine-default cells (19/10) `walkableClimb = 4` and the ceiling is `atan(2×10/19) = 46.5°`, so the 44° cap genuinely bound and 40° was genuinely fine. **TASK-217's `CellSize 32 / CellHeight 20` moved it, and the three shipped hills measure 27.54° / 27.18° / 27.48°** (`handoffs/TASK-139-artist.md:28-30`) — comfortably under **both** ceilings. ⚖️ ***A latent constraint with no shipped case near it is invisible until the first case walks into it. The tower was going to be that case.*** ⭐ **The hill law's ≤30° was the right number for a stated reason (the 44° cap) that turns out to be the WRONG reason.**
- ⚠️ **The engine ships a validator for exactly this (`RecastNavMeshGenerator.cpp:5309-5332`) and it stays SILENT here** — it checks only the *worst-case* slope (44° ⇒ `RequiredClimbVx = 2` vs `WalkableClimbVx = 2`), so it does not warn about the **mid-range** failure at 40°. **⛔ Do not treat engine silence as engine approval.**

**THE PINNED BUDGET — ⛔ build targets, ⛔ NOT a range to interpret:**

| Property | **Value** | Why this number, and ⛔ not the obvious one |
|---|---|---|
| **Ramp slope** | ⭐ **30.0°** | 7.6 % margin under the 32.005° ledge ceiling; and it is the angle this project has already built three times |
| **Platform rise** | **1,200 uu** | `T-5` / `PlatformHeightUU` |
| **Ramp run** | **2,078 uu** | `1200 / tan(30°)` |
| **Ramp deck width** | ⭐ **300 uu**, ⛔ **not 200** | **nulling + erosion cost 128 uu** — the outer column dies to ledge clause A (the drop off the open side), then `rcErodeWalkableArea` takes one more (`walkableRadius = ceil(34/32) = 2`). **300 ⇒ 172 uu of surviving corridor; 200 ⇒ 72 uu, one noise cell from nothing.** Hard floor for any corridor at all: **160 uu** |
| **Vertical clearance** | **≥ 200 uu** | ⭐ **driven by `ASummonedUnit`'s REAL 176 uu capsule** (engine `ACharacter` default 34 r / 88 half-height; the project never calls `InitCapsuleSize`, `SiegeSpawnConstants.h:9`), ⛔ **not by the nav agent's 144.** ⚠️ **The nav agent is SHORTER than the actual unit**, so Recast would happily generate nav under a ceiling a unit physically cannot pass |
| **Structure** | ⭐ **SOLID WEDGE**, ⛔ **not a floating plank** | `ABuilding` does ⛔ **not** set `bFillCollisionUnderneathForNavmesh` (the scatter hills do). A thin deck lets Recast generate ground spans **underneath**, and `rcFilterWalkableLowHeightSpans` then nulls the ground within 160 uu of the underside — **punching a ring of missing ground nav around the ramp's low end.** A wedge removes the ground span and the artifact with it |
| **Ramp→platform junction** | ⭐ **FLUSH, co-planar, ⛔ no lip** | **a step > 40 uu (2 voxels) SEVERS the connection.** ⚠️ This is the castle-floor defect class (35→61 hulls): every property readback correct, and nothing can climb it |
| **Parapet, if any** | **OUTSIDE the 300 uu deck** | a rail **carved out of** the deck seeds erosion from its own face and costs another 2 cells per side |

- ⛔ **STAIRS AND THE SPIRAL STAY REFUSED — ⭐ BUT BOTH ORIGINAL REASONS WERE WRONG, AND THE CORRECTED REASONS ARE RECORDED BECAUSE THAT IS THE WHOLE POINT:**
  - **STAIRS** — ~~"a 35 cm riser will not generate a nav surface"~~ is **FALSE**: `walkableClimb` is 40 uu and `rcFilterLowHangingWalkableObstacles` is precisely the pass that re-marks a riser's top as walkable. ⇒ **Refused on COST AND FRAGILITY: 1,200 uu of rise at a sub-35 uu riser needs 35+ steps**, each with a tread deep enough to survive 64 uu/side erosion (≈130 uu) ⇒ a run of **≈4,550 uu, 2.2× the ramp**, for a far more expensive mesh — and at `CellHeight 20` a 34 uu riser is 1.7 voxels, so quantisation makes every step a coin-flip.
  - **SPIRAL** — the span-merge reason is struck (see `TOWER-§2`); it fails because **a helix's inner edge is steeper than its centreline and breaches the 32.005° ceiling INVISIBLY**, on the inside of the curve only.
- ⚖️⭐ **THE STANDING LESSON THIS SUB-SECTION EXISTS TO CARRY, AND IT IS WORTH MORE THAN THE BUDGET ABOVE:** ***a right answer held for a wrong reason fails the moment the reason is tested.*** **All three of this law's original geometry conclusions (ramp-not-stairs · straight-not-spiral · ≥200 width) survived the measurement. ⛔ Not one of their reasons did.** ⇒ **When a claim about engine behaviour is load-bearing, it is read at the engine source or it is not asserted** — `NAV-§5`'s *"an unverified engine assumption is how this feature got a 216-second navmesh in the first place"*, now proven a fourth time.
- 📌 **The serialized-`L_Arena` trap was NEUTRALISED WITHOUT THE EDITOR, and the method is the record-worthy part:** the ini's values may never reach the placed `RecastNavMesh` actor (the reconciliation at `RecastNavMesh.cpp:963-1015` is gated on `IsVoxelCacheEnabled()`, which this project never enables ⇒ **the serialized values win and files alone cannot say which cell size the shipped level runs**). ⭐ **Rather than force the editor open, the spike checked the spec against BOTH configurations: 30° / 300 / 200 / 600×600 generates under BOTH; 40° is a coin-flip on an unresolved question.** ⚖️ *An unresolvable question can be made irrelevant instead of answered — and that asymmetry is on its own sufficient reason to refuse 40°.*
- ⚠️ **THE REJECTED ALTERNATIVE, NAMED SO IT IS NOT RE-PROPOSED (`SC-§15`): flipping `LedgeSlopeFilterMode` to `UseStepHeightFromAgentMaxSlope`** would raise the cap to 4 voxels and make 40° legal. ⛔ **REFUSED: it is a GLOBAL generation change that would re-bake every hill, the castle floor and the whole arena — to buy a steeper ramp nobody asked for.**

### TOWER-§3 📌 THE CARD — NAMES, STATS, AND WHERE EVERY NUMBER CAME FROM

- **CardID `WatchTower`, DisplayName "Watch Tower".** Follows the shipped `<Name>Tower` family (`ArrowTower`, `BombTower`, `BallistaTower`, `CrystalTower`). 🧑 **A rename is CHEAP NOW and EXPENSIVE once art lands** — it is on his sheet as row T-1 for exactly that reason.
- **The pinned asset family** — every name derived from the shipped convention, ⛔ none invented:
  - `Docs/Data/cards.csv` row **`WatchTower`**
  - C++ **`AClimbableTower : public ABuilding`** in `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.{h,cpp}`
  - **`/Game/Blueprints/Buildings/BP_Building_WatchTower`** — ⛔ **the name is FORCED, not chosen**: `SiegePlayerController.cpp:3842` and `SiegeBotController.cpp:1570` both resolve `/Game/Blueprints/Buildings/BP_Building_<CardID>.BP_Building_<CardID>_C`. **A different name = the card silently does nothing.**
  - **`/Game/Meshes/SM_WatchTower`** · **`/Game/Materials/Instances/MI_WatchTower_PBR`** · **`/Game/UI/CardArt/T_CardArt_WatchTower`** · raw `Content/RawAssets/WatchTower.fbx`
- **The stat row, and the derivation of every cell:**

  | Cell | Value | Why |
  |---|---|---|
  | `Cost` | **30** | ⭐ **Jonathan's, verbatim. ⛔ Not tuned by anyone.** |
  | `CardType` | `Building` | his word ("a building card") |
  | `HP` | **250** | ⭐ **Derived, ⛔ not guessed: Barracks is the other 30-gold non-weapon structure and has HP 250. Same cost ⇒ same HP.** |
  | `Damage`/`Range`/`Cadence` | **0 / 0 / 0** | ⛔ **It does NOT auto-fire — he asked for a platform, not a turret.** `Cadence 0` also guarantees no timer ever arms (`ABuilding`'s `qa/TASK-021` WARN-1 law). |
  | `bRanged` | **false** | ⛔⛔ **LOAD-BEARING: `true` would pull it into `HIGH-§4`'s `bRanged && Unit` selector logic and into any future ranged sweep.** It has no attack. |
  | `DeckCount` | **0** | ⛔ **`sum(DeckCount)` must stay EXACTLY 50** (`UNCAP-§`/GDD §3.4). A non-zero cell would break the default deck's legality and force compensating edits elsewhere. |
  | `MaxCopies` | **3** | inert for a Building since `UNCAP-§2` (hero-upgrade stack cap only); mirrors Barracks for consistency |

- ⛔ **THE ROW CARRIES ALL 31 FIELDS AND ⛔ NO COMMA APPEARS INSIDE `Notes`** — `cards.csv` is unquoted, and a stray comma shifts every subsequent column silently.

### TOWER-§4 ⚖️ THE FOUR BEHAVIOUR QUESTIONS — RULED WITH DEFAULTS, ⛔ NONE BLOCKING

| Question | ⚖️ Ruling (proceeding default) | Why — and what it COSTS to have ruled this way |
|---|---|---|
| **Can ENEMIES climb it?** | ⛔ **No — own-team only.** ⭐ **Reuse the SHIPPED team-gating mechanism**, `ECC_SiegeTeamBlue`/`ECC_SiegeTeamRed` (`SiegeNavAreas.h`, the castle 3× hollow law). | ⛔ **No new mechanism is invented** — the castle already solves "enemies cannot walk in here" and it is proven in shipped play. |
| **Ranged-only, or any own unit?** | ✅ **Any OWN-TEAM unit may walk up.** | ⚖️ His *"any ranged unit **can** climb it"* is a **permission**, not an exclusion. And a nav filter admitting 3 CardIDs while rejecting 8 is a **new failure mode** — a melee unit pathing toward a tower it may not enter is exactly the stuck-unit class `NAV-§` exists for. ⭐ **A melee unit on top gains nothing (`HIGH-§4`) and self-selects away, so the outcome is near-identical for a fraction of the risk.** |
| **Capacity?** | ✅ **NO capacity counter. Physical space is the cap** — platform **600×600 uu**, agent radius 34 ⇒ comfortably 4–6 bodies. | ⭐ **An entire occupancy subsystem — a counter, a full/refuse state, its UI, its replication, its desync cases — is DELETED by choosing the ramp.** ⚖️ *The cheapest correct feature is the one whose bookkeeping does not exist.* |
| **What happens to occupants when it dies?** | ✅ **They FALL and survive.** ⛔ No teleport, ⛔ no death, ⛔ no special case. | `ASummonedUnit : ACharacter` ⇒ CharacterMovement handles the fall, and **this project has ⛔ NO fall damage — ⭐ MEASURED, ⛔ not assumed (`TOWER-§4a`)**. ~~⚠️ **The named risk: a unit may land off-navmesh or wedged — and the `NAV-§` stuck watchdog ALREADY covers exactly that** (`SiegeStuckStatics`).~~ ⛔⛔ **STRUCK 2026-08-30 — THAT CLAIM IS FALSE. THE WATCHDOG COVERS NO SUCH THING.** The true residual, and the source measurement that refuted it, are in **`TOWER-§4a`**. ⛔ **No new recovery code** (still correct — for a different reason). 🧑 Row T-4: he may prefer occupants die with the tower — a dramatic choice, and his. |
| **Does it block pathing?** | ✅ **Yes — automatically, with ⛔ zero work.** | `ABuilding` already roots `VisualMesh` with `BlockAll` + `bCanEverAffectNavigation(true)` (`Building.h:43-51`) ⇒ **the body is solid and carves the navmesh like every other building; the ramp is the walkable route.** |

### TOWER-§4a ⛔⛔ THE STRUCK CLAIM — **THE `NAV-§` WATCHDOG DOES ⛔ NOT COVER A BAD LANDING**, AND THE TRUE RESIDUAL IN ITS PLACE (2026-08-30)

> ### ⛔⛔ **THE FALSE CLAIM, QUOTED SO IT CANNOT BE RE-INTRODUCED BY MEMORY:** ~~*"a unit may land off-navmesh or wedged — and the `NAV-§` stuck watchdog ALREADY covers exactly that."*~~
> ### ⚖️ **IT WAS WRITTEN IN TWO PLACES — THIS SECTION'S OWN CELL AND TASK-726'S BOARD SPEC §5 — AND IT WAS WRONG IN BOTH. BOTH ARE STRUCK, ⛔ NOT DELETED.**

**⭐ WHAT WAS MEASURED, AT THE SOURCE, BEFORE THIS REPAIR WAS WRITTEN** (TASK-725 §8 raised it; ⛔ **the manager did not repair on a relay — every line below was re-read in the working tree**):

1. ⛔ **THE LADDER NEVER TELEPORTS AND NEVER NAV-PROJECTS.** `FSiegeStuckStatics` is **pure by construction** — *"No `UWorld`, no `AActor`, no `UObject`, no engine singleton, no allocation, no logging, no RNG, no clock read"* (`SiegeStuckStatics.h:32-36`), and `ComputeSidestepGoal` states outright that its result *"is **NOT** nav-projected and is **NOT** guaranteed reachable, by design"* (`:237-241`).
2. ⛔ **ITS THREE RUNGS ARE THE WHOLE MECHANISM, AND ⛔ NONE OF THEM MOVES A UNIT ANYWHERE IT COULD NOT ALREADY WALK** (`SummonedUnit.cpp:3065-3175`): **Sidestep** = one ordinary `EnterAdvanceToLocation` to a raw geometric point · **WidenAndRepath** = clear three latches so the *same* goal genuinely re-paths · **Abandon** = drop goal + target and **`EnterIdle()`**. ⭐ **The terminal rung is `EnterIdle()` — a stand-down.** ⛔ **There is ⛔ no `SetActorLocation`, ⛔ no `TeleportTo` and ⛔ no `ProjectPointToNavigation` anywhere in the lane** (grepped project-wide; `SummonedUnit.cpp`'s single `ProjectPointToNavigation` at `:2739` projects the **GOAL** — a structure's near wall face, `ResolveStructureMarchPoint`, TASK-349 — ⛔ never the unit's own position).
3. ⭐⭐ **AND THE DECISIVE ONE — THE LADDER NEVER EVEN STARTS FOR A STRANDED UNIT.** Its gate is `bAdvancing = (AI != nullptr) && (AI->GetMoveStatus() != EPathFollowingStatus::Idle)` (`SummonedUnit.cpp:3026`) — *"this unit has an ACTIVE path-following request THIS poll."* **A unit standing off the navmesh has no live request, so `Evaluate` re-anchors and returns `None` for as long as it stands there** — the same clause that (correctly) stops the ladder "rescuing" units told to hold position.

⇒ ⛔⛔ **THE WATCHDOG COVERS A UNIT STUCK **ON** THE NAVMESH. IT DOES ⛔ NOT COVER A UNIT STRANDED **OFF** IT.** ⚖️ *Those are not two degrees of one problem; they are opposite problems. The ladder's whole design assumes a live path request, and a stranded unit has none.*

**✅ THE TRUE RESIDUAL, RECORDED IN THE FALSE CLAIM'S PLACE — ⛔ DECLARED, ⛔ NOT SOLVED:**

- ⚠️ **A dead tower's occupants can land in a footprint whose navmesh has ⛔ NOT YET REGENERATED, and ⛔ NOTHING IN THIS PROJECT CURRENTLY RECOVERS THEM.** `RuntimeGeneration=Dynamic` rebuilds dirtied tiles over a real (short) window; for that window the landing zone carries no poly, and there is no recovery lane.
- ⭐ **WHY IT IS STILL RIGHT TO SHIP NO RECOVERY CODE — and note the reason CHANGED even though the ruling did not:** it is low-likelihood (the landing zone is the ground the tower was *standing on*, which was navmesh before and becomes navmesh again within the rebuild window, and `UNavigationSystemV1` projects a move request's **start** point to the nearest poly), and ⛔ **the fix would be worse than the bug: a nav-projecting teleport is precisely the mechanism `NAV-§` deliberately refuses.**
- ⛔ **NO ARTIFACT MAY CITE THE WATCHDOG AS THE GUARANTEE.** ✅ `ClimbableTower.h` already carries the correction at its source (the occupant clause). **It is a declared residual and a watch item for Jonathan's sitting (TASK-732) — ⛔ never a coverage claim.**

**✅ AND THE OTHER HALF OF `T-4` IS CONFIRMED FREE — RECORDED SO NOBODY RE-OPENS IT:** `Landed` / `OnLanded` / `FallDamage` / `LandingVelocity` / `MOVE_Falling` measure **ZERO hits across `Source/GitClaudeUnrealTest/Siegebound`** (re-grepped 2026-08-30; every hit in the repo lives in untouched Epic template `Variant_*` classes that are ⛔ not ancestors of `ASummonedUnit`). ⇒ ⛔⛔ **THERE IS NO FALL DAMAGE IN SIEGEBOUND. Occupants drop and resume walking. ⛔ No catch, ⛔ no fall damage, ⛔ NO WORK.** ⚠️ *And the watchdog does not interfere with the fall itself either — `Evaluate` re-anchors whenever speed exceeds `MinSpeedSq` (50 uu/s), and a falling unit is far above that.*

> ### ⚖️⛔⛔ **THE METHOD RULING THIS REPAIR CARRIES, BECAUSE IT IS THE THIRD LAW CLAIM THIS WEEK AN IMPLEMENTER REFUTED BY MEASURING:**
> ### **A LAW CLAIM ABOUT ENGINE OR SHIPPED-CODE BEHAVIOUR IS VERIFIED AT THE SOURCE BY WHOEVER WRITES IT. ⛔ A REPAIR IS NEVER MADE ON A RELAY.**
> **`PKG-§7a` has been damaged TWICE by exactly that mistake** — a correction accepted second-hand, then re-corrected. ⭐ **The implementer who refutes a law is doing the law a favour; the law's job is to be re-measured before it is re-written.** ⛔ **"An agent told me it was wrong" is not grounds to edit a law. Reading the source is.**

### TOWER-§4b ⚖️ `bCanWalkOffLedges` STAYS ENGINE-DEFAULT `true` — ⭐ **DELIBERATELY, AND IT IS DESIRABLE** (TASK-726's ruling, ⛔ NOT re-litigable)

- ⛔ **It is ⛔ not `AClimbableTower`'s property to set.** It lives on each unit's `UCharacterMovementComponent`; the tower holds no unit reference, and acquiring one to flip a movement flag is exactly the coupling `TOWER-§` exists to avoid.
- ⛔ **The only place to set it is `ASummonedUnit` — and there it is GLOBAL.** It would change behaviour on **every hill, every rampart and every ledge in the arena**, to fix an aesthetic on one building.
- ⭐⭐ **IT IS THE PRESSURE-RELEASE VALVE, AND THAT IS THE LOAD-BEARING REASON.** `bUseRVOAvoidance` is `false`, so units physically jostle. **A platform with ⛔ no capacity counter, ⛔ no eviction, and no way to be shoved off is a TRAP.** Being walk-off-able is what guarantees ⛔ **no unit is ever permanently stranded up there** — and `T-4` already proves that falling is free. ⚖️ *The capacity subsystem we deliberately did not build is affordable only because the platform leaks.*
- ✅ **The art fix, if Jonathan wants one, is a PARAPET BUILT OUTSIDE THE 300 uu DECK** (`TOWER-§2a`) — ⛔ never carved out of it, and ⛔ never a movement-flag change.

### TOWER-§5 ⛔ SCOPE FENCES

- ⛔ **`ATower` IS NOT TOUCHED.** `AClimbableTower` is a **sibling** under `ABuilding`, ⛔ **not a subclass of `ATower`** — inheriting an auto-fire cadence loop the card explicitly does not want is how a "harmless" base class becomes a bug. **Four shipped tower cards depend on `ATower` and ⛔ none of them may change.**
- ⛔ **NO CHANGE to `ABuilding`**, to any existing `BP_Building_*`, to `ABP_Footman`, to `SK_Footman_Skeleton`, or to any shipped `A_<CardID>_*` sequence. ⚠️ **`SK_Footman_Skeleton` is the whole fleet's rig — a careless save on it silently breaks every unit's ABP binding** (the standing editor-bounce warning).
- 📌 **M8: ⛔ no new replicated property, ⛔ no RPC, ⛔ no class-tier change.** `AClimbableTower` sits at `ABuilding`'s existing tier and inherits its shipped team/HP replication unchanged.

### TOWER-§6 📌 THE FOR-JONATHAN ROWS THIS LAW OWES

| Row | Question | **Proceeding default** |
|---|---|---|
| **T-1** | CardID/name `WatchTower` / "Watch Tower"? | ✅ **Yes** — ⚠️ **rename is cheap NOW, expensive after art** |
| **T-2** | ⭐ **Which ascent ships FIRST — walkable ramp (no new anim) or the climb clip?** | ✅ **Ramp first, clip second.** ⭐ **The clip is ONE animation, not three (`TOWER-§0`)** |
| **T-3** | Enemies climb it? Ranged-only? Capacity? | ⛔ no / ✅ any own unit / ⛔ no counter (`TOWER-§4`) |
| **T-4** | Occupants when it dies — fall, or die with it? | ✅ **Fall and survive** |
| **T-5** | Platform height 1,200 uu (⇒ **+78.7%** damage on flat ground, **×2.44** on a hill)? | ✅ **1,200 uu** — `EditDefaultsOnly`, retunable without code |
| **T-6** ⭐ *(added 2026-08-30, `TOWER-§7`)* | **May a Watch Tower be dropped ON TOP of your own units?** Its footprint is **~2,700 uu** — roughly **10× a wall's** — and the shipped placement rules were sized for a wall | ⛔ **NO — REFUSE THE PLACEMENT** (red ghost, no gold spent), ⛔ **never push the units.** ⚖️ *Moving a unit the placer does not own would be the first code in this project to do so.* 🧑 **One word overrules it** |

### TOWER-§7 ⚠️⚠️ THE OUT-OF-FENCE FINDING — **PLACEMENT VALIDATION IS SIZED FOR A WALL, AND THE TOWER IS ~2,700 uu** (raised by TASK-726, 2026-08-30; boarded as **TASK-735**)

> ### ⛔⛔ **EVERY SHIPPED BUILDING-PLACEMENT GATE IS A **POINT** TEST. THE WATCH TOWER IS THE FIRST STRUCTURE WHOSE FOOTPRINT IS AN ORDER OF MAGNITUDE LARGER THAN THE POINT.**

**⭐ VERIFIED AT SOURCE BEFORE BOARDING (`TOWER-§4a`'s method ruling applies to findings too) — the complete gate list, and what each one measures:**

| Gate | What it actually tests | Sized for |
|---|---|---|
| ground hit + spawn box / captured zone | ⭐ **the cursor POINT** | any card |
| navmesh projection (`NavProjectionExtent`) | ⭐ **the cursor POINT** | any card |
| `MaxPlacementSlopeDegrees = 20.f` | one straight-down trace **at the point** | a wall |
| `ObstaclePlacementClearance = 150.f` | 2D distance **from the point** to `Obstacle`-tagged actors | a wall |
| `BuildingClearance = 200.f` | 2D distance **from the point** to the nearest other `ABuilding` | a wall |
| ⛔⛔ **units** | ⛔ **NOTHING. THERE IS NO UNIT TERM AT ALL** — `EPlacementInvalidReason` is `{ None, Point, Slope, Obstacle, Clearance }` and ⛔ **no member of it names a unit** | — |

- ⇒ ⚠️ **A player can very likely drop a ~2,700 uu tower on top of their own army and ⛔ nothing refuses it.** `BlockAll` geometry materialises around the capsules; `UCharacterMovementComponent` depenetrates them, ⛔ but nothing in this project chooses where they go.
- ⚠️ **AND THE GENERAL FORM, STATED HONESTLY RATHER THAN NARROWED TO THE SYMPTOM: with a point test, a 2,700 uu structure's FAR END is unvalidated** — it may overhang a 40° flank the slope gate never sampled, cross an obstacle the 150 uu radius never saw, intersect a building the 200 uu radius never reached, or extend outside the spawn box entirely. ⭐ **`BuildingClearance = 200` cannot even reach this tower's own half-extent of ~1,350 uu.**
- ⛔⛔ **THE FIX IS THE PLACEMENT RULE, ⛔ NOT `AClimbableTower` AND ⛔ NOT A UNIT SWEEP.** TASK-726 **correctly refused to widen its own fence** to touch this (`SC-§15`), and that refusal is recorded as the right call: a tower class reaching out to reposition units it does not own is exactly the coupling this namespace exists to prevent.
- ⭐ **THE SIZE MUST COME FROM THE MESH, ⛔ NEVER FROM A LITERAL.** A hardcoded `2700` would be wrong for the next large building and silently wrong the day `SM_WatchTower` is re-authored. **The placement ghost already spawns the real `SM_<CardID>` — its bounds ARE the footprint, and every existing small building keeps its shipped behaviour by construction because its bounds are small.**
- ⚠️ **THE RISK THIS FIX CARRIES, NAMED SO IT IS NOT DISCOVERED IN PLAY: a footprint-sized refusal can make a large building feel UNPLACEABLE in a busy spawn box.** ⇒ **The refusal is the OWN-team unit-overlap case + a footprint-aware building clearance, ⛔ not a wholesale re-sampling of every gate across the footprint** (that is declared as a follow-on finding, ⛔ not smuggled into one task), and its threshold ships `EditDefaultsOnly` so 🧑 **T-6** is a one-word retune.
- 📌 **This is ⛔ NOT a speculative fix for an unobserved symptom** (the standing anti-guessing law): the **mechanism** is measured — the validation has **no unit term in its enum** and its two clearances are **200/150 uu against a ~1,350 uu half-extent**. ⚖️ *A missing check is a measurement, not a symptom report.*
- ⚠️ **AMENDED 2026-09-01 (`TOWER-§8`): the tower's footprint drops from ~2,700 uu to ~750 uu (half-extent ~375). ⭐ THIS TASK GETS EASIER, ⛔ NOT OBSOLETE, AND IT IS ⛔ NOT CANCELLED.** The finding was never *"the tower is big"* — it is *"**building placement has NO unit term at all** and its clearances are **point** tests."* **That is true of a 750 uu tower, of a 600 uu one, and of the next large building somebody adds.** ⭐ **What changes is URGENCY, ⛔ not validity:** `BuildingClearance = 200` against a ~375 uu half-extent is now the same order of magnitude instead of **6.75× short**, so the worst symptom (a tower legally intersecting another building) is much less likely. ⇒ **TASK-735 stays boarded, drops below the ladder batch in priority, and its spec is unchanged — ⭐ because it was correctly written to take the footprint FROM THE MESH and never from the literal `2700`, it needs ⛔ no edit at all to be correct against the new mesh.** ⚖️ *That is the dividend of the "size comes from the mesh" rule, collected one day after it was written.*

---

## ⚖️ THE LADDER REDESIGN — the Watch Tower's ascent becomes a climb (2026-09-01) — namespace **TOWER-§8**

**Trigger — Jonathan's directive, verbatim (2026-09-01):** *"ok, I like what you did with the tower, but I was thinking instead of making it a ramp that you walk up it instead has a ladder you climb up"*

> ### ⭐⭐ **READ THE FIRST FIVE WORDS BEFORE ANYTHING ELSE: *"I LIKE WHAT YOU DID."* ⛔ THIS IS ⛔ NOT A REJECTION.**
> ### **THE FEATURE, THE CARD, THE 30-GOLD COST, THE HEIGHT-ADVANTAGE RULE, THE ×3, THE PLATFORM CONCEPT AND THE 1,200 uu RISE ARE ⛔ ALL UNTOUCHED AND ⛔ NOT RE-OPENED.** This namespace changes exactly one thing: **HOW a unit gets to the top.**

### TOWER-§8.0 ⛔⛔ THE SCOPE FENCE, FIRST, BECAUSE A REDESIGN IS WHERE SETTLED DECISIONS GET QUIETLY RE-LITIGATED

⛔ **`HIGH-§` in its entirety · the ×3 range · `Cost = 30` · `HP = 250` · `Damage/Range/Cadence = 0/0/0` · `bRanged = false` · `DeckCount = 0` · CardID `WatchTower` · `PlatformHeightUU = 1200` · the **600×600** deck · `AClimbableTower : public ABuilding` (⛔ never a subclass of `ATower`) · `TOWER-§5`'s fences · `TOWER-§4a`'s fall-damage finding · `TOWER-§4b`'s `bCanWalkOffLedges` ruling — ⛔ NOT RE-OPENED, ⛔ NOT RE-DERIVED, ⛔ NOT RE-TUNED BY THIS NAMESPACE.** Any task that touches one of them without a fresh directive from Jonathan is a **QA blocker on sight**.

### TOWER-§8.1 ⭐⭐ THE MEASURED VERDICT — **A LADDER IS ⛔ NOT A GEOMETRY SWAP. IT IS A NEW TRAVERSAL SUBSYSTEM, AND HERE IS THE PROOF, READ AT THE SOURCE**

> ### ⛔⛔ **A NAVMESH CANNOT PATH A VERTICAL LADDER. ⛔ `UCharacterMovementComponent` CANNOT CLIMB ONE. ⭐ BOTH HALVES WERE VERIFIED AT THE ENGINE AND IN OUR OWN TREE BEFORE THIS LAW WAS WRITTEN — ⛔ NOT TAKEN ON A RELAY (`TOWER-§4a`'s method ruling).**

**The five measurements, each re-derivable from the cited file and line:**

| # | Claim | ⭐ **MEASURED AT** | Verdict |
|---|---|---|---|
| **M-1** | Recast will not generate a walkable surface up a vertical face | `TOWER-§2a`'s own ceiling: `rcFilterLedgeSpans` nulls anything over **`atan(20/32)` = 32.005°**. A ladder is **90°** (this law pins **76.0°**) — **2.4× over the ceiling** | ✅ **CONFIRMED.** ⛔ **No amount of mesh authoring makes a ladder walkable. The ramp was chosen precisely because it dodged this, and that dodge is now spent.** |
| **M-2** | Therefore the AI will not even *consider* the route unless an off-mesh connection exists | `ANavLinkProxy` (`Engine/Source/Runtime/AIModule/Classes/Navigation/NavLinkProxy.h`) ships **`PointLinks` (simple)** + **`SmartLinkComp` (`UNavLinkCustomComponent`)**; `IsNavigationRelevant()` = `PointLinks.Num() > 0 \|\| SegmentLinks.Num() > 0 \|\| bSmartLinkIsRelevant` (`NavLinkProxy.cpp:268-271`) | ✅ **CONFIRMED — a nav link is REQUIRED.** Without one the platform poly is an **island**: no path exists, so a unit ordered to the deck simply never goes. |
| **M-3** | A **simple** link is not enough — it does not drive the unit, it only steers toward the far point | `UPathFollowingComponent::SetMoveSegment` calls `StartUsingCustomLink` **only when `PathPt0.CustomNavLinkId != FNavLinkId::Invalid`** (`PathFollowingComponent.cpp:959-963`). A simple link carries no such id ⇒ **ordinary steering, nothing else** | ✅ **CONFIRMED.** |
| **M-4** | ⭐⭐ **AND ORDINARY STEERING PHYSICALLY CANNOT LIFT A WALKING PAWN** | `UCharacterMovementComponent::ConstrainInputAcceleration` (`CharacterMovementComponent.cpp:8121-8131`): *"walking or falling pawns ignore up/down sliding"* — it returns `FVector::VectorPlaneProject(InputAcceleration, -GetGravityDirection())` whenever `IsMovingOnGround() \|\| IsFalling()` | ✅⛔ **DECISIVE. THE VERTICAL COMPONENT OF THE STEERING VECTOR IS DELETED BY THE ENGINE, EVERY FRAME, BY DESIGN.** A unit handed a link whose far point is 1,200 uu overhead receives a horizontal input of ≈0 and **walks nowhere.** ⇒ **custom movement is REQUIRED; `CharacterMovement` does ⛔ not climb natively, and `EMovementMode` (`EngineTypes.h:1018-1045`) has ⛔ NO climb mode — `MOVE_None/Walking/NavWalking/Falling/Swimming/Flying/Custom`.** |
| **M-5** | ⭐ **And this project has ⛔ NONE of it today** | Grepped `Source/GitClaudeUnrealTest/`: **`NavLink` = 0 hits · `SmartLink` = 0 · `MOVE_Custom` = 0 · `PhysCustom` = 0 · `CustomMovementMode` = 0.** The **only** `SetMovementMode` call in the entire project is `HeroCharacter.cpp:736` → `MOVE_Walking` | ✅ **CONFIRMED — this is greenfield.** ⛔ **Nothing is being extended; a subsystem is being introduced.** |

**⇒ ⚖️ THE HONEST COST RESTATEMENT, AND IT INVERTS `TOWER-§0`'s:**

- ⭐ **`TOWER-§0` said the rig is the cheap half and the ascent mechanic the expensive half. THAT IS STILL TRUE — and the ladder makes the expensive half BIGGER while making the cheap half MANDATORY.**
- ⛔⛔ **THE CLIMB CLIP IS NO LONGER "POLISH, DEFERRABLE, GATED BEHIND HIS SITTING." IT IS REQUIRED, AND THE REASON IS A DIRECT CONSEQUENCE OF THE REDESIGN:** a unit **walking up a ramp** unanimated looks **fine** — the walk cycle is doing honest work. **A unit gliding up a ladder bolt upright reads as BROKEN.** ⇒ ⭐ **`TOWER-§1`'s ship-first law is SPENT: it bought a playable tower months early and it did its job. It does ⛔ not apply to the ladder, because there is no unanimated ladder that looks acceptable.**
- ✅⭐ **THE GOOD NEWS FROM `TOWER-§0` STANDS UNCHANGED AND IS WORTH REPEATING: IT IS STILL **ONE** CLIP, ⛔ NOT THREE.** `SK_Archer` / `SK_Longbowman` / `SK_Wizard` all bind the single `/Game/Characters/SK_Footman_Skeleton`, the whole rigged fleet shares one `ABP_Footman`, and `IK_MeshyBiped → RTG_MeshyBiped_to_SiegeBiped → IK_SiegeBiped` ships. **`A_SiegeBiped_Climb` covers all three.**

### TOWER-§8.2 ⚠️ WHAT THIS OBSOLETES — **STATED OUT LOUD, ⛔ NOT QUIETLY DISCARDED**

| Artifact | Fate | ⚖️ Why, stated honestly |
|---|---|---|
| **`SM_WatchTower`'s ramp wedge** — the 2,078 uu run + the 14 hand-authored hulls tuned to it | ⛔ **RE-AUTHORED** (TASK-737) | There is no ramp any more. **The hulls were tuned to a shape that is being deleted; they go with it.** ⭐ **The platform at 1,200 uu, the 600×600 deck and the whole height-damage contract SURVIVE BYTE-FOR-BYTE** — this is a base-and-access rebuild, ⛔ not a new tower. |
| **`TOWER-§2a`'s 30° / 32.005° derivation** | ⭐ **RE-SCOPED, ⛔ NOT STRUCK, ⛔ NOT DELETED** | The **30° slope** and **2,078 uu run** rows are moot **for this mesh**. ⛔ **Everything else binds every future ramp, hill and slope in the project** — and the deck this ladder lands on is sized from its 128 uu tax. **See the RE-SCOPED box at the head of `TOWER-§2a`.** |
| **`ClimbableTower.cpp:106-107`'s ascent gate** — the elevation shell, and the known-open pivot-vs-mesh tuning item (pivot-centred ±1500 vs mesh x −600→+2458.46) | ⭐⭐ **DISSOLVES ENTIRELY — ⛔ DO NOT BOARD A FIX FOR IT** | The shell existed to stop an enemy **somewhere along a 2,078 uu approach nobody could locate in mesh-local space**. **A ladder has ONE discrete entry point**, so the gate becomes **one predicate at that point** (`TOWER-§8.6`). ⚖️ ***Fixing a tuning item that the next commit deletes is the purest form of wasted work.*** |
| **The owed card-art re-render** (camera on the ramp's blind side) | ⛔⛔ **DO ⛔ NOT SPEND IT TWICE — the OLD one is RETIRED** | The mesh it was going to re-shoot **will not exist**. ⇒ **the re-render is re-boarded against the NEW mesh (TASK-740) and the old obligation is CLOSED, ⛔ not carried.** |
| **`TOWER-§7` / TASK-735** — footprint-aware placement | ✅ **EASIER, ⛔ NOT OBSOLETE. KEPT.** | Footprint **~2,700 → ~750 uu**. ⭐ **The spec needs ⛔ zero edits because it takes the footprint FROM THE MESH.** See the amendment appended to `TOWER-§7`. |
| **`TOWER-§1`'s ship-first law** | ✅ **SPENT, HONOURABLY** | It shipped a playable tower with zero animation and that was the right call. ⛔ **It does not survive contact with a ladder** (`TOWER-§8.1`). |

### TOWER-§8.3 📌⛔ THE PINNED GEOMETRY — **BUILD TARGETS FOR *BOTH* LANES, ⛔ NOT A RANGE TO INTERPRET**

> ### ⭐⭐ **THIS TABLE IS WHY THE ART LANE AND THE CODE LANE ARE `parallel-safe: yes`.** Neither waits for the other: **both build to these numbers**, and the sockets (`TOWER-§8.4`) let integration reconcile them without a re-spec. **This is the same move that let `TOWER-§2a` pin 2,078 uu before any mesh existed.**

**Local space, `SM_WatchTower`'s own origin at the base centre. Project axes per "World axes (arena contract)".**

| Property | ⭐ **Value** | Why this number, ⛔ and not the obvious one |
|---|---|---|
| **Platform rise** | **1,200 uu** | ⛔ **UNCHANGED** — `PlatformHeightUU`, `T-5`, and the whole `HIGH-§` contract (**×1.787** on the flat, **×2.44** from a hill) |
| **Deck** | **600 × 600 uu** at Z **1,200** | ⛔ **UNCHANGED** — `TOWER-§4`'s "physical space is the cap", 4–6 bodies at `AgentRadius 34` |
| **Body footprint** | ⭐ **600 × 600 uu**, X/Y ∈ [−300, +300] | **"Compact base", his word.** The deck's own footprint — ⛔ nothing wider. Total structure span **750 uu** (body + ladder reach) vs the ramp's ~2,700 ⇒ ⭐ **3.6× smaller** |
| **`LadderFoot` socket** | ⛔ ~~**(−450, 0, 0)**~~ ⇒ ⭐⭐ **AMENDED 2026-09-02: `(−460, 0, 0)`** | ⛔ **NOT a style choice — it MUST sit on generated ground navmesh.** The body carves its 600×600 footprint and `rcErodeWalkableArea` takes **2 more cells (64 uu)** ⇒ ground nav starts at **X ≤ −364**. ~~−450 clears it by 86 uu.~~ ⭐ **−460 clears it by 96 uu — the amendment IMPROVES this margin.** ⚖️ **Amended by Jonathan's `K-1` ruling (option `A`); ⛔ the reason is `CONTACT-§7a`'s banner, ⛔ not a modelling preference.** |
| **`LadderTop` socket** | ⛔ ~~**(−150, 0, 1200)**~~ ⇒ ⭐⭐ **AMENDED 2026-09-02: `(−160, 0, 1200)`** | ⛔ **MUST sit on generated DECK navmesh.** A 600 uu deck loses **64 uu per side** to ledge-nulling + erosion (`TOWER-§2a`) ⇒ the surviving poly is **X ∈ [−236, +236]**. ~~−150 is 86 uu inside it.~~ ⭐ **−160 is 76 uu inside it — ⛔ the amendment SPENDS 10 uu of this margin, and it is the only margin it spends.** ⚠️ **Placing it on the deck EDGE is the castle-floor defect class: every readback correct, nothing can use it.** |
| ⭐⭐ **THE TRANSLATION IS ⛔ PURE — the row that makes the amendment cheap** ***(NEW 2026-09-02)*** | **`Δ = LadderTop − LadderFoot = (300, 0, 1200)` — ⛔ BYTE-FOR-BYTE UNCHANGED** | ⇒ ⛔ **length `1236.93169` · lean `76.0°` · `sin θ` · the watchdog budget · the deck-breach window's Z-ceiling AND its % of the line are ⛔ ALL UNCHANGED.** ⭐⭐ **AND THE RUNG PLANE'S −22.0 uu IS DEFINED ***RELATIVE TO THE CLIMB LINE***, so a pure translation preserves it ⇒ ⛔ `A_SiegeBiped_Climb` needs ⛔ NO re-export and the ⭐ UNITS' SHIPPED CLIP IS SAFE.** ⚠️ **That last sentence is a PREDICTION, ⛔ not a measurement — `§8.3`'s three-number checklist below is ⛔ still owed in full, and if the rung plane MOVED the clip re-export is back on.** |
| **Climb line** | ⭐ **ONE STRAIGHT SEGMENT, `LadderFoot` → `LadderTop`.** Length **1,236.9 uu**, lean **76.0°** from horizontal | ⭐⭐ **A LEANING SIEGE LADDER, ⛔ NOT A VERTICAL ONE, AND THE REASON IS ENGINEERING, NOT TASTE:** a vertical ladder forces a **3-segment** traversal (step in · rise · step out), which is three times the code, three times the interruption cases, and three chances to desync from the mesh. **One straight segment cannot desync from itself.** ⭐ It also **animates far better** — a bolt-upright vertical climb is the exact thing that reads as broken — and a ladder leaning on a tower is on-theme for a siege game. **76° is 2.4× over the walkable ceiling, so Recast will never try to walk it** (⭐ that is a feature: ⛔ no ambiguity about which route the AI takes) |
| **Ladder width** | **≥ 120 uu** | `AgentRadius` 34 ⇒ a 68 uu body plus visual margin. ⛔ Not a nav number — nothing walks it — purely so the climbing unit does not visibly clip the stiles |
| ⭐⭐ **RUNG PLANE** ***(NEW 2026-09-01 — ⛔ this is a SECOND, DIFFERENT plane and the whole row exists to say so)*** | ⭐ **mid-plane at −22.0 uu** from the climb line, measured along the in-plane normal pointing AWAY from the tower. **Near face −12, far face −32 ⇒ 20 uu slab.** Stiles **±66 / ±86** ⇒ **132 clear / 172 overall** | ⛔⛔ **THE CLIMB LINE IS ⛔ NOT THE RUNG PLANE, AND CONFLATING THEM HAS ALREADY COST ONE DEFECT.** The climb line is where the **capsule** travels; the rung plane is where the **hands** must grip. ⚠️ **They differ by 22.0 uu because the ladder slab sits INBOARD of the socket line** — a unit stands *outboard* of the stiles. ⇒ **an animator who puts grips on the climb line authors hands that grip 22 uu short of the rungs** (TASK-739 F5, measured off the shipped FBX; the stile figures match TASK-737 exactly, which is what validates the parse). ⚖️ **The clip is re-exported to THIS number (TASK-733b); ⛔ the law is ⛔ NOT bent to absorb the clip** |
| **Standoff** | **≥ 56 uu** clear between the capsule surface and the body face along the whole line ⇒ ⭐⭐ **RESTATED 2026-09-02 IN CAPSULE-INDEPENDENT FORM: `dist(SPINE, geometry) ≥ 98.0 uu`** | ⛔ **The climb is swept movement — a unit dragged through the tower body is a depenetration explosion.** ⚠️⚠️ **THE CAPSULE THIS ROW WAS MEASURED AGAINST IS `r 34 / hh 88`, AND THAT IS THE *UNIT*, ⛔ NOT EVERY PAWN — SEE THE STRUCK CLAUSE DIRECTLY BELOW.** ⇒ ⭐ **THE RESTATEMENT IS THE REPAIR FOR EXACTLY THAT, AND IT IS DERIVED, ⛔ NOT INVENTED — see the box below.** |

> #### ⭐⭐ **THE STANDOFF IS NOW PINNED AS A ***SPINE DISTANCE***, ⛔ NOT AS A CAPSULE CLEARANCE — ADDED 2026-09-02, AND IT IS THE STRUCTURAL FIX FOR THE ROOT CAUSE THAT COST THIS BATCH A WHOLE GATE.**
>
> ⛔⛔ **THE NUMBER `≥ 56 uu` IS ⛔ UNCHANGED AND ⛔ NOT WEAKENED. What changes is ⛔ WHICH BODY IT IS MEASURED AGAINST — because that is the question the old wording never asked, and the omission is what let a `r 42` pawn onto a line certified for `r 34` with ⛔ nobody noticing.**
>
> - ⭐ **THE DERIVATION, one line, from `TOWER-§8.5a`'s spine table:** every UE capsule on this line shares the **same 108 uu vertical spine** (`88 − 34` and `96 − 42` are both **54**) ⇒ a fatter capsule is the thinner one **Minkowski-grown by the radius difference in EVERY direction** ⇒ **clearance ≡ `dist(spine, geometry) − Radius`, an ⛔ EQUALITY.** ⇒ **`clearance ≥ 56` for a radius `R` ⟺ `dist ≥ 56 + R`.**
> - ✅⭐⭐ **PINNING `dist ≥ 98.0` PINS THE GATE FOR THE ⛔ WIDEST PAWN ADMITTED TODAY (`R_hero = 42`) AND IS ⛔ CAPSULE-FREE:** it yields **hero clearance ≥ 56** and **unit clearance ≥ 64** from the same measurement. ⇒ ⭐⭐ **THE ART LANE NO LONGER HAS TO KNOW WHICH PAWN CLIMBS, AND A FUTURE PAWN CANNOT SILENTLY VOID `§8.5a` BY BEING FAT** — it can only do so by exceeding `98 − 56 = 42` uu of radius, which is now a **single checkable number** rather than a re-derivation nobody was told to run.
> - ⛔ **THE DECK SLAB IS ⛔ EXCLUDED FROM THIS ROW AND ALWAYS WAS** (`TOWER-§8.5a`: *"the ONLY static geometry this window can cross is the deck slab itself — the very thing it must"*). ⚠️ **An art handoff reporting `dist ≈ 0` at the deck is reporting the DESIGN, ⛔ not a failure. ⛔ Do not let that reading fail a correct mesh.**
> - 📌 **`TOWER-§8.5a`'s ~~two-sided~~ ⭐ THREE-SIDED voiding condition is ⛔ UNCHANGED IN SUBSTANCE and still binds** (⛔ **side (iii), a RUNTIME TRANSFORM, added 2026-09-03 — ⛔ do ⛔ not read "two-sided" here as live; ⛔ retained struck-through so a reader who remembers the old count finds its amendment instead of re-deriving it**) — ⭐ **but its side (ii) ("any NEW PAWN CLASS admitted to the line") is now cheap to discharge: compare the pawn's radius to `98 − 56 = 42`. ⛔ No re-sweep, ⛔ no art task, ⛔ no re-measure.** ⚖️ *A gate nobody can afford to run is a gate nobody runs.* ⛔ **Side (iii) is ⛔ NOT cheap in the same way — it is discharged by `STACK-§10`, ⛔ not by a radius comparison.**

> #### ⛔⛔ **STRUCK 2026-09-01 — THE PARENTHETICAL *"(`SiegeSpawnConstants.h:9`; the project never calls `InitCapsuleSize`)"* WAS ⛔ FALSE AND IS ⛔ REMOVED FROM THE ROW ABOVE. Measured and refuted by TASK-775.**
>
> **THE STRUCK TEXT, QUOTED SO THE REPAIR IS AUDITABLE:** ~~*"The capsule is **r 34 / half-height 88** (`SiegeSpawnConstants.h:9`; the project never calls `InitCapsuleSize`)."*~~
>
> ⛔ **`InitCapsuleSize` IS CALLED — six times, and one of them is the hero's own direct base class:** `GitClaudeUnrealTestCharacter.cpp:18` → **`GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f)`**. `AHeroCharacter : AGitClaudeUnrealTestCharacter : ACharacter` (`HeroCharacter.h:319`, `GitClaudeUnrealTestCharacter.h:22`) ⇒ ⭐⭐ **`R_hero = 42.0` · `HalfHeight_hero = 96.0`.** ✅ **Corroborated three ways:** `Castle.cpp:185` `constexpr float HeroCapsuleRadius = 42.f;` (**pixel-measured** at TASK-656 — capsule FREE at x 3710, BLOCKED at 3690, matching `HeroStopLaneX = 3657.5 + 42 = 3699.5`) · `SiegeGhostPawn.cpp:53` sizes the ghost to `(42.f, 96.f)` naming the hero's ctor as its source · `AHeroCharacter` never resizes (⛔ no `InitCapsuleSize`/`SetCapsuleSize`/`SetCapsuleRadius` in `HeroCharacter.{h,cpp}`) · and `BP_HeroCharacter` overrides ⛔ nothing (`CapsuleRadius`, `CapsuleHalfHeight` **and** `InheritableComponentHandler` are all ABSENT from the uncooked package's name table — and `InheritableComponentHandler` is the **only** mechanism by which a BP can override a native inherited component, so its absence is the absence of any override).
>
> ⭐⭐ **THE ROOT CAUSE, NAMED SO IT CANNOT RECUR — TWO DIFFERENT 34s WERE CONFLATED.** **34 is the NAV AGENT RADIUS**, an engine default this project genuinely never overrode — `Config/DefaultEngine.ini:290` says so in its own words: *"Agent radius/height/slope untouched (engine defaults 34/144/44)."* ⚖️ **It is ⛔ NOT the capsule radius of any pawn in Siegebound.** Every `TOWER-§` derivation that reasons from *"`AgentRadius` 34"* (the deck's 4–6 bodies, the ladder width, the `LadderFoot`/`LadderTop` erosion margins) is **CORRECT and UNTOUCHED** — those are genuinely nav numbers. ⛔ **ONLY the standoff row is affected, because ⛔ only the standoff sweeps a real CAPSULE.**
>
> 📌 **THE ROW'S NUMBER (`≥ 56 uu`) IS ⛔ NOT CHANGED and the mesh is ⛔ NOT at fault — TASK-737 built to the law it was given and hit 59.624.** ⇒ **The consequence lands entirely in `TOWER-§8.5a`'s voiding box and `CONTACT-§7` `K-1`.**
| **Deck parapet, if any** | **OUTSIDE the 600×600 deck** | ⛔ **UNCHANGED from `TOWER-§2a`** — a rail carved **out of** the deck seeds erosion from its own face and costs another 2 cells per side |
| **`MI_WatchTower_PBR`** | ⛔ **REUSED, not re-authored** | Same material, same texture set. **A redesign of the silhouette is ⛔ not a reason to re-render textures** |

> ### ⚠️⚠️ **THE MESH↔CLIP BINDING — ADDED 2026-09-01, AND IT IS THE SAME SHAPE AS `TOWER-§8.5a`'s VOIDING CONDITION, DELIBERATELY.**
> ⭐⭐ **THE ROOT CAUSE, RECORDED BECAUSE THE RECURRENCE IS THE REAL RISK: the F5 defect existed ⛔ ONLY because `SM_WatchTower` WAS RE-AUTHORED AFTER THE CLIP, AND ⛔ NOTHING TIED THE TWO TOGETHER.** The clip was correct against the law it was given; the mesh was correct against the law it was given; **the law simply did not contain the number where they meet.** ⚖️ *That is a specification failure, ⛔ not an execution failure by either lane, and neither TASK-733 nor TASK-737 is at fault for it.*
> ⇒ ⛔⛔ **BINDING ON EVERY FUTURE `SM_WatchTower` RE-AUTHOR: RE-MEASURE THE RUNG PLANE AND REPORT IT, AND IF IT MOVES, RE-EXPORT `A_SiegeBiped_Climb` TO MATCH.** ⭐ **A mesh change that moves the rungs and leaves the clip alone is a SILENT defect — the hands miss, and ⛔ every readback is still correct.** ⚖️ *A conditional whose condition nobody re-checks is an unconditional one.*
> 📌 **The two lanes' shared checklist is therefore THREE numbers, ⛔ not one: the CLIMB LINE (`LadderFoot`→`LadderTop`) · the ≥56 uu STANDOFF (which licenses `§8.5a`) · the RUNG PLANE (−22.0 uu, which the animation grips).** ⛔ **A re-author handoff that reports fewer than all three is incomplete.**

### TOWER-§8.4 ⛔⛔ THE TWO PINNED CONTRACTS — **the SOCKETS and the API SIGNATURE. ⛔ NEITHER LANE MAY INVENT ITS OWN.**

**(A) THE SOCKETS — the artist↔programmer seam.** `SM_WatchTower` ships exactly two sockets, named per the new **Static-mesh SOCKET names** clause: **`LadderFoot`** and **`LadderTop`**, at the `TOWER-§8.3` coordinates.
- ⭐ **The code reads them at `BeginPlay` and ⛔ NEVER hardcodes the geometry** — `TOWER-§7`'s *"the size must come from the MESH, ⛔ never from a literal"*, applied a second time.
- ⛔ **AND IT DEGRADES OPEN:** socket missing ⇒ fall back to the `TOWER-§8.3` literals, **one** warning naming the missing socket, ⛔ never a broken tower. ⭐ **That fallback is exactly what makes the two lanes parallel-safe: the code is correct before the mesh exists and self-corrects the moment it lands.**

**(B) THE API — the ONE new coupling surface between a tower and a unit.** ⚠️⚠️ **STATED HONESTLY BECAUSE IT IS A REAL LOSS: `TOWER-§`'s founding idea was that the tower is a PLACE that never learns a unit exists. A ladder ENDS that** — something must drive a specific unit up a specific line. ⭐ **What is preserved, and it is the part that actually mattered: `HIGH-§3`'s fence is UNTOUCHED.** The tower still ⛔ never calls `HeightAdvantageMultiplier`, ⛔ never reports elevation, ⛔ never includes a `HIGH-§` header. **A unit on a hill and a unit on a tower at the same Z still deal identical damage, and the damage rule still does not know towers exist.** ⚖️ *The coupling that was refused was gameplay coupling; this is movement coupling, and there is no way to move a unit without touching it.*

⛔ **BOTH programmer tasks compile against this EXACT signature. Neither may change it unilaterally** — an implementer who measures a better shape says so in the handoff and it lands as a **law amendment**, ⛔ never as a silent divergence (the `AS-§21` pinned-signature precedent):

```cpp
// ─── ASummonedUnit, public. Declared by TASK-738; called by TASK-734. ───
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSiegeLadderClimbEnded, ASummonedUnit*, Unit, bool, bReachedTop);

/** Drives a scripted traversal along the ONE straight world-space segment FromWorld -> ToWorld. Returns false
 *  and changes NOTHING if the unit is dead, match-end frozen (bAIFrozen), spell-frozen (bSpellFrozen), or
 *  already climbing.
 *  ⛔ NO Z-ORDERING IS ENFORCED OR IMPLIED, AND THAT IS THE CONTRACT: the link is BothWays (TOWER-§8.7), so a
 *  DESCENT passes the same two points the other way round. Which end is the deck is resolved by Z inside
 *  (TOWER-§8.5a clause 1), never by argument order. ⛔ Do not "fix" this into Foot-then-Top. */
UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit|Climb")
bool BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld);

/** Ends an in-flight climb WHEREVER the unit is: restores the movement mode and lets it drop. Idempotent. */
UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit|Climb")
void AbortLadderClimb();

/** True for the whole ascent. Read by ABP_Footman's climb state and by the TOWER-§9 disarm guards. */
UFUNCTION(BlueprintPure, Category = "Siegebound|Unit|Climb")
bool IsClimbing() const;

/** Broadcast EXACTLY ONCE per successful BeginLadderClimb — on arrival, abort, death, or EndPlay.
 *  The tower's ONLY completion signal, so AClimbableTower needs NO tick (TOWER-§ keeps its no-tick property). */
UPROPERTY(BlueprintAssignable, Category = "Siegebound|Unit|Climb")
FSiegeLadderClimbEnded OnLadderClimbEnded;
```

#### ⚖️ AMENDED 2026-09-01 — **THE TWO PARAMETER NAMES: `LadderFootWorld`/`LadderTopWorld` ⇒ `FromWorld`/`ToWorld`. PROPOSED BY TASK-734, TAKEN BY TASK-738, RULED HERE.**

> ⭐ **This is the `AS-§21` / `§8.4(B)` pinned-signature process working exactly as written: an implementer MEASURED a better shape, said so in the handoff, and it lands as a LAW AMENDMENT — ⛔ never as a silent divergence.**

- ⛔⛔ **THE TRAP IT REMOVES IS REAL, ⛔ NOT COSMETIC.** `Foot`/`Top` **implies a Z-ordering the implementation deliberately does not enforce** — and an implied ordering is an invitation. ⚖️ **A future "tidy-up" that made the names true (assert Foot below Top, or sort the two points by Z) would SILENTLY BREAK CLIMBING DOWN** and manufacture exactly the stranded unit `TOWER-§8.7` exists to prevent. **The names now say what the function does.**
- ✅ **VERIFIED SAFE, ⛔ not assumed: parameter names are NOT part of a function's type.** ⇒ the three compile-time `static_assert`s on member-function-pointer identity are **unaffected**, and TASK-734's call site passes positionally from locals **already named `FromWorld`/`ToWorld`**. ⭐ **Types, count and order are UNTOUCHED — this amendment changes zero bytes of the signature's type.** The reflected UFUNCTION pin names change and **nothing binds them yet** (TASK-739 reads only `IsClimbing`).
- ⭐ **The state's fields moved with them (`Foot`/`Top` ⇒ `Start`/`End`)** for the same reason, and the freedom is now **assertable**: TASK-738's test 13 admits both argument orders, proves the descent direction is the exact negation of the ascent's, and proves each arrives where it was **sent** rather than where Z would sort it.
- ✅ **CONFIRMED AT SOURCE BEFORE THIS AMENDMENT WAS WRITTEN** (⛔ not on relay): `FromWorld`/`ToWorld` are declared at **`SummonedUnit.h:755`** *(was `:1027` — re-measured post-TASK-776)*; the degenerate-line refusal is a **symmetric** `SizeSquared()` test that *cannot* express an ordering, at **`SiegeLadderClimbStatics.cpp:43`** *(was `SummonedUnit.cpp:155` — ⭐ the statics LIFTED, so the cite moved FILE, not just line)*; the deck is resolved by **Z** at **`SiegeLadderClimbStatics.cpp:82`** (`State.bDeckIsAtEnd = (State.End.Z >= State.Start.Z)`) *(was `SummonedUnit.cpp:194`)*. ⛔ **There is no Z-ordering anywhere in the shipped path.**

#### ⚖️ AMENDED 2026-09-01 (second time) — **THE FIRST PARAMETER WIDENS: `ASummonedUnit*` ⇒ ⭐ `ACharacter*`. PROPOSED BY TASK-775, ⚖️ RULED IN `CONTACT-§4.4`, IMPLEMENTED BY TASK-777.**

> ⛔⛔ **THE PINNED BLOCK ABOVE IS ⛔ SUPERSEDED IN EXACTLY ONE TOKEN.** The new pinned line, and ⛔ nothing else about the API changes:
> ```cpp
> DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSiegeLadderClimbEnded, ACharacter*, Climber, bool, bReachedTop);
> ```

- ⛔⛔ **WHY IT HAD TO CHANGE:** `CONTACT-§`'s hero climb makes `AHeroCharacter` a second climber, and the tower's `ActiveClimber` slot + this delegate were both typed to `ASummonedUnit` ⇒ **a hero climber was invisible to `TOWER-§10 L-1`'s one-at-a-time rule AND to `EndPlay`'s abort — tower falls, hero hangs forever** (`CONTACT-§3.1` `H-10`). ⭐ **A ⛔ MEASURED defect (`ClimbableTower.h:487`), ⛔ not a speculative one.**
- ✅ **UNCHANGED: the type name `FSiegeLadderClimbEnded` · the arity (2) · the parameter ORDER · the second parameter (`bool bReachedTop`) · `BeginLadderClimb` · `AbortLadderClimb` · `IsClimbing` — all four other pinned members are ⛔ untouched.**
- ✅ **`HIGH-§3`'s fence and `TOWER-§8.4(B)`'s "movement coupling only" property survive intact** — a wider climber type couples ⛔ no additional gameplay.
- ⚠️ **THE SUITE FEELS IT, AND THAT IS BY DESIGN:** `Tests/SiegeLadderClimbTest.cpp:907-908` pins the first parameter's class and ⇒ **goes RED until updated.** ⭐ **That test doing its job is the evidence the pinning mechanism works — ⛔ it is ⛔ not collateral damage.**

### TOWER-§8.5 ⚖️ THE TRAVERSAL — **`MOVE_Flying` + interpolation along the pinned line. RULED, WITH THE MEASUREMENT THAT LICENSES IT.**

- ✅ ⭐⭐ **`MOVE_Flying` IS THE ONE SHIPPED MODE THAT ACCEPTS VERTICAL MOTION WITHOUT NEW PHYSICS CODE, AND THIS IS MEASURED, ⛔ NOT ASSUMED:** `ConstrainInputAcceleration` plane-projects **only** when `IsMovingOnGround() || IsFalling()` (`CharacterMovementComponent.cpp:8125`). **Flying is neither** ⇒ the vertical component survives. **`MOVE_Flying` also disables gravity, which is exactly what a climb wants.**
- ⛔ **`MOVE_Custom` + a `PhysCustom` override is REFUSED FOR THIS SHIP** — it is a **new physics surface** in a project that has **zero** lines of custom movement (M-5), for a behaviour `MOVE_Flying` already delivers. 🧑 A later upgrade is fine; ⛔ it is not the first ship.
- ⛔⛔ **A RAW `SetActorLocation` / `TeleportTo` LERP *OF THE TRAVERSAL* IS REFUSED OUTRIGHT.** It bypasses the capsule, the sweep and depenetration — a unit driven through the tower body, through other units, or through the deck. ⚖️ *And it is the mechanism `NAV-§` refuses on principle.*
  - ⚠️⚠️ **BUT READ `TOWER-§8.5a` BEFORE RULING AGAINST ANY NON-SWEPT MOVE ON THIS LINE.** ⭐ **This bullet was written on 2026-08-30, BEFORE the mesh existed and BEFORE `LadderTop` was measured to sit 150 uu inside a solid deck slab.** ⛔⛔ **A QA gate that reads this bullet alone and fails the shipped deck-breach window is failing CORRECT code against STALE law** — the exception below is ruled, scoped, and conditional, and it is what makes the feature work at all.
- ⚠️⚠️ **THE NAMED REGRESSION RISK OF THIS ENTIRE REDESIGN, AND IT IS WORSE THAN ANYTHING THE RAMP COULD DO: `MOVE_Flying` IGNORES GRAVITY, SO AN ABORTED CLIMB THAT FAILS TO RESTORE THE MOVEMENT MODE LEAVES A UNIT HANGING IN MID-AIR, FOREVER.** ⇒ ⛔ **EVERY exit path restores the mode and broadcasts `OnLadderClimbEnded` EXACTLY ONCE: arrival · `AbortLadderClimb` · a new order · death (`HandleDeath`) · `FreezeAI` · `ApplyFreeze` · `EndPlay` · ⭐ and the tower being destroyed mid-climb (`ABuilding::HandleDestroyed` destroys the actor, so `AClimbableTower::EndPlay` must abort every climber it started).** **A QA gate that does not check all eight is not a gate.**
- ⭐ **The climb rate is ONE `EditDefaultsOnly` float, and it is Jonathan's exposure lever (`TOWER-§9`):** `LadderClimbSpeedUU = 350.f`. ⚖️ **DERIVED, ⛔ not felt: 350 is the `Speed` cell of the fastest ranged units (Archer, Wizard) in `Docs/Data/cards.csv`** ⇒ **ascending costs a unit exactly the time walking the same distance would** — the ladder adds ⛔ **no speed penalty on top of the disarm**, so the whole cost of the ascent is the vulnerability he ruled and nothing smuggled in beside it.

### TOWER-§8.5a ⚖️⭐⭐ THE DECK-BREACH WINDOW — **THE ONE SCOPED EXCEPTION TO `§8.5`'s LERP REFUSAL. GRANTED 2026-09-01, ⛔ NOT A LOOPHOLE — AND IT IS CONDITIONAL.**

> ### ⚖️ **RULING: GRANTED.** ⭐⭐ **THE LAW PREDATES THE MEASUREMENT THAT REFUTES IT, AND THE MEASUREMENT WINS.** `§8.5`'s refusal was written against a tower nobody had built yet. **TASK-737 then measured the mesh and TASK-738 re-derived it independently, and both land on the same fact: a fully swept climb CANNOT REACH THE DECK.** ⛔ **Refusing the exception does not preserve a safe feature — it ships a feature that silently does nothing.**

**⛔ WHAT WAS MEASURED, ⛔ NOT ASSERTED — the chain that bought this ruling, recorded because the reasoning is the ruling:**

| # | Who | Measurement | Consequence |
|---|---|---|---|
| 1 | **TASK-737** (mesh, art lane) | `LadderTop` is pinned **150 uu inside a solid deck slab** (hull 07, Z[1160,1200]) and approached from below at **76°** ⇒ the climb line's final **40.832 uu (3.30%)** lies **inside deck geometry**. Capsule↔slab clearance **−53.95 uu** | ⭐ **And it ⛔ REFUSED TO PRESCRIBE THE FIX** — correctly. A deck hatch containing the line's last stretch also contains the socket, leaving `LadderTop` **over a hole** = the **castle-floor defect class** (every readback correct, nothing can use it). A thinner slab cannot help: the **176 uu capsule straddles it** from a bottom-Z of 964 regardless of thickness, and the slab is already a lean 40 uu |
| 2 | **TASK-738** (code lane) | **Re-derived it rather than relaying it, and found it WORSE:** the capsule's own traverse of the deck plane is **176 × 1.03078 = 181.4 uu of line** | ⛔⛔ **THE SWEEP DOES NOT STALL AT THE SLAB — IT STALLS ~131 uu BELOW IT**, because the capsule's **top** reaches the slab's underside long before its centre nears it. **The unit stops roughly a capsule-height short of the deck, silently, with all eight `§8.5` exits still perfectly correct** |
| 3 | ⭐⭐ **TASK-738, and NOBODY HAD FLAGGED IT** | **The sockets are SURFACE points (`§8.3`: generated navmesh), but the thing that travels the line is the CAPSULE CENTRE**, which stands one half-height above whatever it is on | ⇒ the first delivery drove the centre to the **bare socket**, so the unit would have arrived with its **feet 88 uu below the deck, buried in the slab**, and depenetration would have dropped it back down the tower. ⚠️⚠️ **THEREFORE A NON-SWEPT STRETCH *ALONE* WOULD NOT HAVE FIXED THE FEATURE — it would have teleported the unit INTO the slab.** ⛔ **Both halves are required, and neither is optional** |

**⚖️ `§8.5`'s THREE NAMED HARMS, ANSWERED ONE BY ONE — ⛔ the exception is granted because each is individually refuted, ⛔ not because the feature is inconvenient:**

| The harm `§8.5` names | Verdict | Why |
|---|---|---|
| *"through the tower body"* | ⛔ **STILL REFUSED — and structurally, ⛔ not by promise** | `TOWER-§8.3` pins **≥56 uu of standoff** between the capsule surface and the body face **along the WHOLE line** (TASK-737 measured **59.624 uu** worst case). ⇒ ⭐⭐ **the ONLY static geometry this window can cross is the deck slab itself** — the very thing it must |
| *"through other units"* | ⛔ **STILL SWEPT for 78% of the line** | `TOWER-§10` L-1 is **one climber at a time**, and a unit already standing on the deck is resolved by the **same depenetration** `TOWER-§4` already relies on for the 4–6 bodies up there |
| *"through the deck"* | ⭐ **NOW MEASURED TO BE *REQUIRED*** | Row 1 above. **This is the harm the law was protecting, and the mesh has since made it the objective** |

**📌 THE EXCEPTION, SCOPED — ⛔ EVERY CLAUSE IS BINDING. A window that breaks any one of them is ⛔ NOT covered by this ruling and `§8.5`'s refusal applies to it in full:**

1. ⭐ **IT RIDES THE LINE'S *ELEVATED END*, RESOLVED BY **Z** — ⛔ NEVER "the last stretch", ⛔ NEVER argument order.** ⚖️ **This is the clause that makes DESCENT work:** climbing down, the unit **starts standing on the deck** and must pass **down** through the slab it is standing on, where a swept move **jams on frame one**. ⭐⭐ **Resolving by Z makes the window a fact about GEOMETRY rather than an ordering constraint on the API** — which is exactly what keeps `TOWER-§8.7`'s `BothWays` link honest (`bDeckIsAtEnd`, ⭐ **re-measured post-TASK-776: `SiegeLadderClimbStatics.cpp:82`; ⛔ was `SummonedUnit.cpp:194`**).
2. 📏 **SIZE CEILING: ≤ 3 × the capsule half-height, measured in **Z**, converted to line length by the line's OWN slope** (⛔ never a hardcoded `sin 76°`). Shipped: **264 uu of Z ⇒ 272.1 uu of line = 22.0%** of the 1,236.9 uu ascent. **The three terms are named and each earns its place:** one half-height because the capsule's **top** breaches first · one for how far the slab hangs **below its own surface** (⚠️ **a declared ASSUMPTION — code cannot read the mesh — and at 88 uu it is 2.2× the ~40 uu that actually ships**) · one because the capsule's **bottom** must clear the deck **surface** before sweeping is safe again.
3. ⛔ **IT IS A CONTINUOUS DRIVE AT THE CLIMB RATE — ⛔ NOT A TELEPORT AND ⛔ NOT A LERP OF THE TRAVERSAL.** Same rate, same straight segment, no visual discontinuity. ⚖️ *A unit that popped 272 uu up a ladder would read as broken — the exact failure `A_SiegeBiped_Climb` exists to prevent.*
4. ⚠️ **VELOCITY IS ZEROED ON EVERY BREACH FRAME.** Otherwise the two drivers fight: `PhysFlying` keeps sweeping the capsule from residual velocity (`BrakingDecelerationFlying` is **0**, so it never decays) and **re-jams it against the slab being stepped through**.
5. ⭐ **THE ARRIVAL SNAP IS PERMITTED ONLY ON A ***REAL*** ARRIVAL.** ⛔⛔ **A timed-out or aborted climb is ⛔ NEVER handed the deck it failed to reach** — it drops from where it actually is. ⚖️ *A watchdog that teleports its casualty to the destination is not a watchdog.*
6. ⭐⭐ **THE ENDPOINT LIFT IS PART OF THE EXCEPTION, ⛔ NOT A SEPARATE CHANGE: both endpoints are lifted from SURFACE space into CAPSULE-CENTRE space by `GetScaledCapsuleHalfHeight()` — ⛔ READ FROM THE CAPSULE, ⛔ NEVER A LITERAL** (the third application of `TOWER-§7`'s *"geometry comes FROM the thing"*; a BP child that resizes its capsule stays correct for free). **The SAME lift at both ends**, so the length, the direction and the watchdog budget are unchanged.
7. ⚠️ **A STALL BEFORE THE WINDOW OPENS MUST FAIL *LOUDLY*.** If the slab is ever thicker than one capsule half-height the sweep jams below the window — and the `§8.5` watchdog must **drop the unit with a warning**, ⛔ never leave it hanging. **That is the second failure the watchdog now catches, and it is why it exists.**
8. ⛔⛔ **THIS EXCEPTION IS SCOPED TO THE LADDER CLIMB LINE AND TO NOTHING ELSE.** ⛔ **It is ⛔ NOT a general licence for `SetActorLocation` anywhere in Siegebound.** `NAV-§`'s refusal of teleport-style movement stands **everywhere else, unweakened**.

> ### ⚠️⚠️ **THE CONDITION THAT CAN VOID THIS RULING — AND IT IS A LIVE OBLIGATION ON THE ART LANE, ⛔ NOT A FOOTNOTE.**
> ⭐ **The exception is licensed by `TOWER-§8.3`'s ≥56 uu standoff and by NOTHING ELSE.** The standoff is the only reason a non-swept window cannot cross the tower **body**. ⇒ ⛔⛔ **ANY RE-AUTHOR OF `SM_WatchTower` MUST RE-MEASURE THE STANDOFF ALONG THE WHOLE LINE AND REPORT IT.** **If the standoff is ever broken, `TOWER-§8.5a` IS VOID and `§8.5`'s outright refusal applies again** — because the window would then be able to drag a capsule through solid tower. ⚖️ *A conditional exception whose condition nobody re-checks is an unconditional one.*

> ### ⛔⛔⛔ **2026-09-01 — THE CONDITION HAS ***FIRED***. `TOWER-§8.5a` IS ⛔ VOID ***FOR THE HERO***, AND `§8.5`'s OUTRIGHT REFUSAL APPLIES TO IT IN FULL.**
>
> ⚠️⚠️ **AND IT FIRED THE WAY NOBODY WAS WATCHING FOR: ⛔ NOT BY A MESH RE-AUTHOR — BY A ***WIDER CAPSULE***.** `SM_WatchTower` is untouched and TASK-737's 59.624 uu still stands **exactly as measured**. ⚖️ *The box above told the ART lane to re-check the condition. ⛔ Nothing told anyone to re-check it when a **different pawn** was admitted to the line — and that is the hole this entry closes.*
>
> ⇒ 📌 **THE VOIDING CONDITION IS HEREBY ⭐ TWO-SIDED, ⛔ NOT ONE:** it must be re-measured **(i)** on any re-author of `SM_WatchTower` *(the original clause, unchanged)* **AND (ii) ⭐ on any NEW PAWN CLASS admitted to the climb line, ⛔ whatever its capsule.** ⛔ **A pawn whose capsule has ⛔ not been measured against this gate may ⛔ not use the window.**
>
> ### ⛔⛔⭐⭐ **SIDE (iii) — ADDED 2026-09-03 BY `TASK-941`'s MEASUREMENT. ⛔ NEITHER EXISTING SIDE COVERS IT, AND THAT IS ⛔ EXACTLY WHY IT IS BEING WRITTEN DOWN.**
> ⇒ 📌 **THE CONDITION IS NOW ⭐ THREE-SIDED:** **(iii) ⛔ ANY ⛔ RUNTIME TRANSFORM APPLIED TO A CLIMBABLE TOWER — ⛔ scale, ⛔ rotation, or ⛔ non-uniform scale — ⛔ whether or not the mesh and the pawn set are untouched.**
> - ⛔⛔ **WHY THE EXISTING TWO SIDES ⛔ MISS IT, IN ONE SENTENCE: ⛔ `SM_WatchTower` is ⛔ NOT re-authored and ⛔ no new pawn is admitted — yet ⛔ EVERY INPUT to the licence MOVES.** ⇒ ⛔ **a reader discharging (i) and (ii) honestly would conclude the licence stands, ⛔ and at `n ≥ 3` it does ⛔ not.**
> - ⭐ **AND THE ⛔ FIRST DISCHARGE OF SIDE (iii) IS ⛔ ALREADY ON FILE AND IT ⛔ PASSES AT `n ≤ 2`:** `handoffs/TASK-941-programmer.md` — ⛔ standoff `103.32 → 112.38 → 115.10 → 116.40 → 117.15` (⛔ **IMPROVES**, clears `≥ 98.0` at ⛔ all five) · ⛔ rung-plane depth `22.000 → 22.649` (⛔ max +3.0 %, closed-form bounded at `22.677`) ⇒ ⛔ **`§8.3`'s mesh↔clip binding does ⛔ NOT fire and `A_SiegeBiped_Climb` needs ⛔ NO re-export.**
> - ⛔⛔ **THE TERM THAT ⛔ ACTUALLY BINDS IS ⛔ NOT THE STANDOFF — IT IS `§8.5a` cl. 2's ⛔ OWN WINDOW, AND IT IS LEGISLATED IN `STACK-§10`.** ⛔ **Read that section before licensing ⛔ any multiplier.**
> - ⚠️ **SIDE (iii) ALSO CARRIES A ⛔ NAV-SIDE OBLIGATION the other two do not:** ⛔ **the off-mesh link is a `UActorComponent`, so a ⛔ root rescale refreshes the ⛔ MESH's octree entry and ⛔ NOT the ⛔ LINK's** ⇒ ⛔ **the AI ascent can fail while the hero's contact climb keeps working.** ⛔ **See `STACK-§10` cl. 5 — ⛔ discharging side (iii) means discharging ⛔ that too.**
>
> **THE ARITHMETIC, DERIVED HERE RATHER THAN RELAYED — and the derivation reproduces TASK-737's published number to three decimals, which is what validates it:**
>
> | Step | Value |
> |---|---|
> | Line `LadderFoot(−450,0,0)` → `LadderTop(−150,0,1200)`, `Δ = (300,0,1200)` | length **1236.93169**, unit **(0.24253563, 0, 0.97014250)** |
> | Worst-case point, TASK-737's `t = 245` | `P = (−390.5788, 0, 237.6849)` |
> | ⭐ **A UE capsule's SPINE is a VERTICAL segment of half-length `HalfHeight − Radius`** | unit **88 − 34 = 54** · hero **96 − 42 = 54** ⇒ ⭐⭐ **IDENTICAL** |
> | Lower spine endpoint | `P − (0,0,54) = (−390.5788, 0, 183.6849)` |
> | Nearest body feature = the **plinth's top-west edge** (hull 00, X/Y ±300, Z 0–160) ⇒ `(−300, 0, 160)` | `√(90.5788² + 23.6849²) =` **93.6242 uu** |
> | Unit clearance `93.6242 − 34` | **59.6242** ✅ **= TASK-737's 59.624. The reconstruction is CORRECT.** |
> | ⛔⛔ **Hero clearance `93.6242 − 42`** | ⛔⛔ **51.6242 uu vs the required ≥ 56 ⇒ SHORT BY 4.3758** |
>
> ### ⭐⭐ **AND ONE CORRECTION TO THE DIAGNOSIS THAT MAKES THE NUMBER ***BETTER-BEHAVED***, ⛔ NOT WORSE: `51.624` IS ⛔ NOT AN UPPER BOUND. IT IS ⛔ EXACT.**
> TASK-775 warned that the hero *"is also 8 uu taller per end, so a true re-measure can only be worse."* ⚖️ **That double-counts, and the fix is worth writing down because it is the whole reason the deficit is a single crisp number.** ⭐ **Both capsules share the SAME 108 uu spine** (54 either side — see the table: `88−34` and `96−42` are both 54, because 42−34 and 96−88 are both 8). ⇒ **the hero's capsule is exactly the unit's capsule Minkowski-grown by 8 uu in EVERY direction**, and clearance is `dist(spine, geometry) − Radius`. ⇒ ⛔ **the hero's clearance is the unit's MINUS EXACTLY 8, at every point on the line and against every hull.** **The "extra reach at the caps" IS the extra radius; counting it twice is the error.** ⇒ ⭐ **`59.624 − (R_hero − 34)` is an ⛔ EQUALITY, ⛔ not a bound, and ⛔ no re-sweep is needed to trust it.**
> ### ✅⚖️⭐⭐ **2026-09-02 — THE FIRED CONDITION IS BEING ⛔ REPAIRED BY CONSTRUCTION, ⛔ NOT WAIVED. JONATHAN RULED `K-1` = OPTION `A`.**
>
> - ⛔⛔ **`TOWER-§8.5a` REMAINS ⛔ VOID FOR THE HERO ***RIGHT NOW*** AND STAYS VOID UNTIL TASK-783's MEASURED HANDOFF LANDS.** ⛔ **A ruling is ⛔ not a measurement, and ⛔ nothing here licenses building against the intended outcome.** ⚖️ *The whole reason this section exists is that a law was trusted ahead of a measurement.*
> - ✅⭐ **WHAT THE RULING CHANGES: the deficit is closed IN THE GEOMETRY** — the climb line translates **10.0 uu west in X** (`CONTACT-§7a`'s ruling banner; sockets `(−460,0,0)` / `(−160,0,1200)`), taking the worst-contact spine distance **93.6242 → 103.330 uu** ⇒ **hero clearance 61.330 ≥ 56.** ⇒ ⭐⭐ **`§8.5a`'s LICENCE IS ⛔ RE-EARNED, ⛔ not excused — the exception goes back to resting on the standoff exactly as written, and ⛔ no clause of it is amended, waived or reinterpreted.**
> - 📌 **THE DISCHARGE IS ⛔ EXACTLY ONE THING AND IT IS AN ART MEASUREMENT: `TOWER-§8.3`'s THREE-NUMBER CHECKLIST, RE-MEASURED OVER THE WHOLE LINE AND RE-REPORTED** (climb line · **the standoff, now `dist ≥ 98.0`** · the rung plane). **TASK-783 owns it.** ⛔ **A handoff reporting fewer than all three is incomplete and does ⛔ NOT lift the void.**
> - ⚠️⚠️ **AND THE ⛔ NON-OBVIOUS HALF: the RUNG PLANE is predicted to be preserved by a pure translation (it is defined relative to the climb line) ⇒ `A_SiegeBiped_Climb` is predicted to need ⛔ no re-export.** ⛔ **PREDICTED. ⛔ NOT MEASURED.** ⇒ **if TASK-783 measures the rung plane OFF −22.0 uu, `§8.3`'s mesh↔clip binding fires and the clip re-export is ⛔ back on** — ⚖️ *that binding exists precisely because "it should not have moved" is what everyone thought last time.*
>
> - ⚠️ **THE ONE CAVEAT, STATED SO THE EXACTNESS IS ⛔ NOT OVER-CLAIMED:** this holds for a capsule whose **centre follows the `§8.3` line**, which is what `§8.3`'s standoff row is defined on (it is an ART build target, measured before clause 6's lift existed). **Clause 6 lifts the hero's DRIVEN centre path 8 uu higher in Z than the unit's** (96 vs 88); against the plinth edge specifically that shift is **away** from the contact, giving `√(90.5788² + 31.6849²) − 42 =` **53.96 uu — deficit ≈ 2.04.** ⇒ ⭐⭐ **IT FAILS ON BOTH LINES, WHICH IS WHY THE VERDICT IS ROBUST** — but ⛔ **the gate's own number is the `§8.3` one, and it is 4.3758.**

**🔍 WHAT A QA GATE MUST DO WITH THIS SECTION — ⛔ stated so the gate cannot fail correct code, and cannot rubber-stamp a bad one either:**
- ⛔ **DO NOT flag the non-swept window as a `§8.5` violation.** It is ruled. **Check the eight clauses above instead**, one by one.
- ✅ **DO flag** a window that is not Z-resolved · that exceeds 3 half-heights · that is a single-step teleport rather than a continuous drive · that skips the velocity zero · that snaps on a **non**-arrival · that uses a **literal** half-height · that is silent when it stalls · or that appears **anywhere other than this climb line**.
- ✅ **DO confirm the self-check survives:** the foot and the midpoint of the line **ARE** swept. ⛔ **A whole-line non-swept traversal is still refused outright, and it is exactly what `§8.5` was written to stop.**

### TOWER-§8.6 ⭐ THE TEAM GATE MOVES — **and it gets STRONGER, CHEAPER, and it DISSOLVES the open tuning item** (`T-3` upheld)

- ✅ **`T-3` STANDS: enemies may ⛔ NOT ascend.** ⛔ The ruling is unchanged; **only the mechanism moves.**
- ⛔ **THE PHYSICAL ELEVATION SHELL IS REMOVED** — `AscentGateVolume` and its three tuning properties (`AscentGateFloorUU` · `AscentGateHeadroomUU` · `AscentGateHalfExtentXY`). ⚖️ **It had exactly one job — stop an enemy partway up a 2,078 uu ramp whose mouth nobody could locate in mesh-local space — and that job no longer exists.** ⭐ **And it could not do the new job anyway: a scripted `MOVE_Flying` traversal is not reliably stopped by a blocking volume.**
- ✅⭐⭐ **THE GATE BECOMES ONE PREDICATE AT THE ONE ENTRY POINT — AND THE PREDICATE ALREADY SHIPS, ALREADY PASSED QA, AND UNTIL NOW HAD ⛔ NO CALLER: `AClimbableTower::CanTeamAscend(ETeamId TowerTeam, ETeamId ClimberTeam)`.** It was written as a *statement* of the collision matrix so `T-3` was assertable headlessly. **The ladder gives it a real caller and makes it the live rule.** ⛔ **Its signature and semantics are FROZEN** (⛔ no capacity term, ⛔ no unit-type term — there are none in the rule); its internals are the implementer's.
- ⭐ **AN OPTIONAL SECOND LAYER, ALLOWED ⛔ NOT REQUIRED:** `INavLinkCustomInterface::IsLinkPathfindingAllowed(const UObject* Querier)` would stop an enemy **pathing** to the ladder at all. ⚠️ **UNVERIFIED BY THIS LAW — nobody has measured what `Querier` actually is here.** ⇒ **the entry predicate is the SHIPPED gate; the pathfinding layer lands only if the implementer MEASURES the querier and says so in the handoff.** ⛔ **Do not assert it on my word.**
- ⭐ **AND NOTE WHAT THIS DOES ⛔ NOT REINTRODUCE:** the castle's `UNavModifierComponent` team-**area** lane stays refused, and `TOWER-§4`'s objection (a) — *"it would make the tower unattackable by enemy melee"* — ⛔ **does not apply to a link-level gate**, because a link excludes a **connection**, ⛔ never the ground around it. ✅ **Enemy melee still walks up to the tower and hits it.**

### TOWER-§8.7 ⛔⛔ THE LINK IS **`ENavLinkDirection::BothWays`** — **DERIVED, ⛔ NOT PREFERRED**

- ⛔ **A one-way ladder makes the deck a DEAD END WITH NO LEGAL PATH OFF IT** ⇒ a unit ordered back to the ground has **no path**, path following never starts, and it stands there permanently. ⚖️ **That is a manufactured stuck unit — the exact `NAV-§` failure class this whole namespace exists to avoid.**
- ⭐ **Under the ramp, descent was free because Recast polys are UNDIRECTED (`TOWER-§2a` / TASK-725 §7.4). ⚠️ A ladder does ⛔ NOT inherit that property — it must be RE-ESTABLISHED EXPLICITLY.** ⇒ **`BothWays`, and it costs one enum value.**
- ✅ **Walking off the deck edge stays legal and free** — `bCanWalkOffLedges` is `true` and stays `true` (`TOWER-§4b`: it is the pressure-release valve that makes a capacity-counter-free platform safe), and **there is ⛔ no fall damage in Siegebound** (`TOWER-§4a`, measured). ⇒ ⭐ **a unit has TWO ways down: climb, or step off and drop. ⛔ Neither costs anything.**

---

## ⚖️ THE CLIMB IS A TACTICAL COMMITMENT — attackable + disarmed (2026-09-01) — namespace **TOWER-§9**

**Jonathan's ruling, verbatim (2026-09-01):** *"they should be attackable while climbing, but they can't attack back."*

> ### ⛔⛔ **THIS IS A ⛔ DECIDED REQUIREMENT, ⛔ NOT A PROCEEDING DEFAULT. IT DOES ⛔ NOT GO ON THE FOR-JONATHAN SHEET, IT IS ⛔ NOT RE-OPENED, AND IT IS ⛔ NOT SOFTENED INTO A "VULNERABILITY MULTIPLIER" HE DID NOT ASK FOR.**

### TOWER-§9.1 ⭐ THE DESIGN REASONING, RECORDED SO IT SURVIVES THE RULE

⭐⭐ **The tower's height bonus now has to be PAID FOR with an exposed, helpless window.** Before this ruling the ×1.787 was free — walk up, shoot harder, no downside. **A climb that cannot answer fire is a real risk/reward trade, and it is what makes the card interesting rather than strictly-better.** ⚖️ *A 30-gold card that only ever improves your position is not a decision; a 30-gold card that asks you to spend a helpless window is.*

### TOWER-§9.2 📌 WHAT EACH HALF ACTUALLY COSTS — **MEASURED IN OUR OWN TREE**

- ✅⭐ **"ATTACKABLE" IS FREE — ⛔ LITERALLY ZERO WORK.** A climbing unit is an ordinary live `ASummonedUnit` at an ordinary world location. **Nothing in enemy target acquisition filters on movement mode, on Z, or on state.** ⇒ ⛔ **do not write code to "make it targetable"; write a test that proves it already is.**
- ⛔ **"CANNOT ATTACK BACK" IS THE ONLY WORK, AND THE SEAM ALREADY EXISTS: `ASummonedUnit::CanEverAttack()` HAS THREE SHIPPED, PROVEN GUARD POINTS** — `EnterAttack()` stands the unit **down to Idle**, `UpdateStateGrouped()` **acquires nothing**, `PerformAttack()` **refuses** (the `ASorcererUnit` / `AMinerUnit` idiom, TASK-360/397). ⭐ **The disarm is a STATE term evaluated at those SAME three points — ⛔ NOT a new suppression mechanism, ⛔ not a fourth guard point, ⛔ not a new attack path.**
- ⚠️ **BUT ⛔ NOT `CanEverAttack()` ITSELF: it is a `const` CLASS-IDENTITY seal** (*"this class can never attack"*), and a climber is a normal unit in a **transient** state. **Overriding it per-instance would tell `ASorcererUnit`'s and `AMinerUnit`'s permanent seal and a 3-second climb apart by nothing at all.** ⇒ **a distinct per-instance state, ⭐ read alongside the existing `bAIFrozen` / `bSpellFrozen` terms that already sit at those very lines** (`SummonedUnit.cpp:2844`, `:2513`, `:1349`).
- ✅ **THE DISARM ENDS THE INSTANT THE UNIT REACHES THE DECK.** ⚖️ **The vulnerability is the CLIMB, ⛔ never a lingering penalty** — a debuff that outlives its cause is a second rule nobody asked for.

### TOWER-§9.3 ⚠️⚠️ THE EXPOSURE, **MEASURED AND HANDED TO HIM RATHER THAN SOFTENED** — and it is the reason this ruling could go wrong in play

**Inputs, all from the shipped `Docs/Data/cards.csv` (post-×3 `Range`), ⛔ nothing estimated:**

| Unit | HP | Damage | Range | Cadence | Speed |
|---|---|---|---|---|---|
| **Archer** | **45** | 10 | 2,100 | 1.2 s | 350 |
| **Longbowman** | **70** | **18** | **3,600** | **1.5 s** | 300 |
| **Wizard** | **45** | 15 | 2,100 | 1.6 s | 350 |

**Climb line = 1,236.9 uu (`TOWER-§8.3`). Shots landed by ONE defending Longbowman = `floor(T / 1.5)` (conservative, favours the climber) … `+1` (if it is already firing when the climb starts):**

| Climb rate | Window **T** | Longbowman shots | Damage | vs **Archer/Wizard (45 HP)** | vs **Longbowman (70 HP)** |
|---|---|---|---|---|---|
| **350 uu/s** ⭐ *(the shipped default — walk speed)* | **3.53 s** | **2–3** | **36–54** | ⚠️ **survives at 9 HP, or DIES** | ✅ survives (16–34 HP) |
| 250 uu/s | 4.95 s | 3–4 | 54–72 | ⛔ **DEAD** | ⚠️ survives at 16 HP, or dies |
| 150 uu/s | 8.25 s | 5–6 | 90–108 | ⛔ **DEAD twice over** | ⛔ **DEAD** |
| 100 uu/s | 12.4 s | 8–9 | 144–162 | ⛔ **DEAD** | ⛔ **DEAD** |

- ⛔⛔ **THE HEADLINE, STATED PLAINLY RATHER THAN BURIED: FOR A CLIMBING ARCHER OR WIZARD TO SURVIVE ONE DEFENDING LONGBOWMAN, THE ASCENT MUST FINISH IN UNDER 3.0 s — WHICH NEEDS A CLIMB RATE OVER 412 uu/s, FASTER THAN THE FASTEST UNIT IN THE GAME RUNS (Footman, 400).** ⇒ **At every plausible climb rate, a single defender holding a Longbowman makes the ladder lethal to the two units most likely to want it.** ⚠️ And the Longbowman is shooting from **3,600 uu** — **4.8× the tower's entire 750 uu footprint** — so it is not even in the fight.
- ✅⭐⭐ **AND THE COUNTERWEIGHT, WHICH IS GENUINELY REASSURING AND WOULD BE DISHONEST TO OMIT: THE LADDER'S EXPOSURE WINDOW IS *SHORTER* THAN THE RAMP'S WAS — BY HALF.** The ramp was **2,078 uu of run = 2,399 uu of surface at 30°**, walked at 350 uu/s = **6.86 s**. The ladder is **3.53 s**. ⇒ **The climber is exposed for 48% less time. What changed is not the duration — it is that it cannot answer.**
- ✅ **A SECOND MITIGATION, MEASURED: THE SHOOTER GETS ⛔ NO HEIGHT BONUS.** `HIGH-§2` grants the bonus **only when the attacker is above the target**, and a climber is **above** the ground shooter. ⇒ **the climber is exposed, ⛔ but not double-punished.**
- ✅ **A THIRD, AND IT IS THE PLAYER'S OWN LEVER: THE CLIMB CAN BE ABORTED** (`TOWER-§10` row L-4). A new order drops the unit **free** — ⛔ no fall damage (`TOWER-§4a`). **A player who sees the arrows coming can bail.**
- ⛔ **NONE OF THIS SOFTENS THE RULING. The lever is `LadderClimbSpeedUU` (one `EditDefaultsOnly` float), the height, or `HIGH-§7` row `R-2` — ⭐ all three are his, and all three are one word.**
- ⚠️ **AND THE QUEUE MULTIPLIES IT** (`TOWER-§10` row L-1): one-at-a-time means **three archers sent to a tower under Longbowman fire queue at the foot and are shot one after another.** ⛔ **Named, ⛔ not fixed** — no observation exists yet, and boarding a repair for an unobserved symptom is guessing.

---

## ⚖️ THE FIVE LADDER BEHAVIOUR RULINGS (2026-09-01) — namespace **TOWER-§10** — proceeding defaults, ⛔ NONE BLOCKING

| Row | Question | ⚖️ **Ruling** | Why — and what it costs to have ruled this way |
|---|---|---|---|
| **L-1** | Two units on one ladder, or **one at a time**? | ✅ **ONE AT A TIME.** The second unit **waits at the foot in its ordinary walking state** — ⛔ no queue object, ⛔ no counter, ⛔ no UI, ⛔ no replication | ⛔ **Two capsules interpolated along one line WILL interpenetrate** — `bUseRVOAvoidance` is `false` (`TOWER-§4b`), so units physically jostle, and depenetration would shove one **off** the climb line mid-air. ⭐⭐ **AND THE OCCUPANCY LIST IS FREE, MEASURED: `UNavLinkCustomComponent` ALREADY tracks `MovingAgents` and exposes `HasMovingAgents()`** (`NavLinkCustomComponent.h:106-107`, `.cpp:200/213`). ⚠️ **VERIFY, ⛔ DO NOT ASSUME: I measured only that `OnLinkMoveFinished` removes an agent and that `OnPathFinished` / `StartUsingCustomLink` call it. Whether a unit DYING mid-climb reaches that path is UNMEASURED — the tower clears its own occupant defensively regardless.** ⚠️ Residual: a waiting unit must keep re-requesting, ⛔ never latch idle (`NAV-§`) |
| **L-2** | Attackable / can it attack mid-climb? | ⛔⛔ **NOT A DEFAULT — JONATHAN RULED IT: attackable, cannot attack back.** See **`TOWER-§9`** | — |
| **L-3** | Descend by ladder, or step off and fall? | ✅ **BOTH.** The link is **`BothWays`** (⛔ **required**, ⛔ not preferred — `TOWER-§8.7`) **and** walking off the edge stays legal | Falling is **free and already ruled** — ⛔ no fall damage anywhere in Siegebound (`TOWER-§4a`, measured). ⛔ A one-way link manufactures a stranded unit |
| **L-4** | Selectable / orderable mid-ascent? | ✅ **Selectable YES. A new order ABORTS the climb and the unit DROPS from wherever it is.** ⛔ **NOT committed-once-started** | ⛔ Hiding a live unit from selection is new UI state nobody asked for. ⭐⭐ **And the abort is what keeps `TOWER-§9` from being a death sentence: it is the player's escape hatch, and it costs nothing because the drop is free.** ⚠️ It is also the traversal's **most likely** exit path ⇒ ⛔ **the movement-mode restore must be bulletproof there first** |
| **L-5** | A unit mid-climb when the **tower dies**? | ✅ **The climb ABORTS and the unit FALLS and survives** — ⛔ no teleport, ⛔ no catch, ⛔ no death, consistent with `T-4` | ⚠️⚠️ **BUT ⛔ NOT FREE ANY MORE, AND THIS IS THE ONE PLACE THE REDESIGN GENUINELY ADDS RISK:** under the ramp the floor vanished and `CharacterMovement` dropped to `MOVE_Falling` **by itself**. **A `MOVE_Flying` climber will ⛔ NOT fall — it will HANG IN THE AIR** unless `AClimbableTower::EndPlay` explicitly aborts it. ⇒ ⛔ **an explicit abort-on-destroy is REQUIRED** (`TOWER-§8.5`). ⚠️ `TOWER-§4a`'s declared residual (landing in a footprint whose navmesh has not regenerated, with ⛔ no recovery lane) **still stands, unchanged and still unsolved by design** |

### TOWER-§6 (AMENDED 2026-09-01) — the FOR-JONATHAN rows this namespace now owes

| Row | Question | **Status / proceeding default** |
|---|---|---|
| **T-2** | Which ascent ships — ramp or climb clip? | ⛔⛔ **CLOSED BY DIRECTIVE 2026-09-01. He chose the ladder ⇒ the clip is REQUIRED, ⛔ no longer deferrable, and `TOWER-§1`'s ship-first law is spent.** ⛔ **Do not ask him this again.** |
| **T-6** | May a tower be dropped on your own units? | ✅ Unchanged (⛔ refuse the placement). ⭐ **Footprint restated: ~750 uu, ⛔ not ~2,700** |
| **T-7** ⭐ *NEW* | ⭐⭐ **The rig is now REQUIRED, ⛔ not polish — and the ×3 range makes a helpless climber very killable.** Retune `LadderClimbSpeedUU`, the height, or `R-2`? | ✅ **Ships at 350 uu/s** (= walk speed, ⇒ ⛔ no speed penalty on top of the disarm). ⚠️ **Full exposure table in `TOWER-§9.3` — read it before answering** |
| **T-8** ⭐ *NEW* | One climber at a time (`L-1`)? | ✅ **One at a time**, second waits at the foot |
| **T-9** ⭐ *NEW* | A leaning **76°** siege ladder (`TOWER-§8.3`) rather than a vertical one? | ✅ **Leaning** — ⭐ **⅓ the traversal code, animates far better, ⛔ still unwalkable by Recast** |
| **T-10** ⭐ *NEW* | Abort-on-new-order (`L-4`) — escape hatch, or should a climb be a commitment? | ✅ **Abortable.** ⭐ **It is the counterweight to the disarm** |
| **T-11** ⭐ *NEW* | ⚠️ **The tower is a REDESIGN of shipped, QA-passed, working content.** Card art is re-rendered against the new mesh; the OLD owed re-render is **retired, ⛔ not spent twice** | ✅ **Confirmed** — the mesh it would have shot will not exist |

---

