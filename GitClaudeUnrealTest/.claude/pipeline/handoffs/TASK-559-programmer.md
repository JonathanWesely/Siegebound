# TASK-559 handoff — [WR-5] `ACommanderNpc`, the avatar body + the war table (gameplay-programmer, 2026-08-15)

- **Gate:** `.claude/pipeline/qa/TASK-565.md` — ⛔ **this task names TASK-565 as its gate** (RULING 9; build-master refuses to commit without this naming).
- **Compile:** TASK-566. **Commit:** TASK-570. ⛔ **This task opened NO compile of its own.**
- **Status:** `ready-for-qa`.
- **Discipline honoured:** ⛔ no compile · no build · no Git · no editor · no MCP · no PIE · no `Content/` asset · no `.csv` · no `Build.cs` · no `Tests/` · no existing source file touched at all. ⛔ **No token figure is quoted, derived or reasoned from anywhere in this handoff** (`AS-§12g`, batch-wide ban).
- **Files created — exactly the two in `names:`, both NEW:**
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\CommanderNpc.h`
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\CommanderNpc.cpp`
- ⛔ **NOT touched (the spec's list, verified by the greps in §4):** `SiegeAssistantComponent.{h,cpp}` · `SiegeAssistantConsoleWidget.{h,cpp}` · `SiegeAssistantSnapshot.{h,cpp}` · `SiegeAssistantGrammar.{h,cpp}` · `SiegePlayerState.{h,cpp}` · `SiegePlayerController.{h,cpp}` · `Castle.{h,cpp}` · `Torch.{h,cpp}` · `SummonedUnit.{h,cpp}` · `ScatterConfig.h` · `L_Arena` · any `Content/` asset · any Zone builder · any `.csv`.
- **Assets referenced by path only (none created, none required to exist):** `/Game/Characters/SK_Sorcerer` (ships today) · `/Game/Meshes/SM_WarTable` (TASK-556 authors, TASK-566 imports — **absent today, and the class is written to survive that**) · `/Game/Characters/ABP_Footman` (ships today) · BP target `/Game/Blueprints/BP_CommanderNpc` (TASK-568).

---

## 1. ⭐ THE RULING THIS CLASS EXISTS TO OBEY, AND THE CHECKABLE FORM OF IT

Jonathan: *"add an NPC character that you can talk to that will be the avatar for the AI model"* … *"the console still works anywhere."*

⇒ **`ACommanderNpc` is a BODY and a TABLE. It holds no conversation state, no prompt, no model handle, and no reference into `USiegeAssistantComponent`.** It does not gate the console, and it does not `#include` anything that could reach the console's open path.

**⛔ THE CLAIM IS STATED SO A REVIEWER CAN RUN IT INSTEAD OF TRUSTING IT** — command, raw count, classification:

```
$ grep -n "Assistant\|Prompt\|ZoneA\|Llama\|Gbnf\|GBNF\|SpendGold" \
      Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.h \
      Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.cpp
CommanderNpc.h:25   *  OBEY (WR-§5). There is ONE assistant: one USiegeAssistantComponent, one
CommanderNpc.h:29   *  ⛔ THE CHECKABLE FORM OF THAT CLAIM, stated so a reviewer can run it rather
CommanderNpc.h:194  *  FSiegeAssistantRegionStatics::IsPointInRegion precedent); TASK-563 is
CommanderNpc.h:280  *  ASiegePlayerState::SpendGold, and never touches gold in any direction.
CommanderNpc.cpp: (no hits)
```
**RAW COUNT: 4 hits, 1 file. ALL FOUR ARE COMMENT PROSE.** The mechanical proof that none is code:

```
$ grep -vE "^\s*(\*|//|/\*)" CommanderNpc.h CommanderNpc.cpp \
    | grep -nE "Assistant|Prompt|ZoneA|Llama|SpendGold|GetFirstPlayerController|HasAuthority|bReplicates|SetReplicates|ITeamAgent|GetTeamId"
(no output — exit 1)
```
**⇒ ZERO code lines, ZERO `#include`s, ZERO symbols.** ✅ **This task spends ZERO prompt characters and touches no Zone builder; `ZoneA` cannot have moved** (TASK-564 asserts it against the untouched `Tests/SiegeAssistantZoneATest.cpp`).

📌 **The gold line, stated separately because `WR-§7` says it will look like a violation to someone reading fast:** this class **HOLDS** `EnemyRevealCost` and **never spends it**. There is no `SpendGold` call, no `ASiegePlayerState` include, no balance read, and no RPC. The spend is TASK-563's, on the authority, initiated by the player's own click.

---

## 2. ⛔⛔ THE `ITeamAgent` DIAGNOSIS — THE SPEC MADE IT CONDITIONAL, AND THE CONDITION FAILS. **THIS IS THE FINDING QA SHOULD READ FIRST.**

The spec (2): *"expose `GetTeamId()` via the existing `ITeamAgent` pattern **only if that is what the shipped interface expects; ⛔ diagnose before implementing, do not assume**."*

**I diagnosed. The shipped interface does NOT expect it, and implementing it would have been a real, shipping defect.**

**LEG 1 — in this codebase, "is an `ITeamAgent`" means "is a legal combat target." Eight shipped call sites, all full-world `GetAllActorsWithInterface(UTeamAgent::StaticClass())`:**

```
$ grep -n "GetAllActorsWithInterface\|UTeamAgent::StaticClass" Source/GitClaudeUnrealTest/Siegebound/*.cpp
HeroCharacter.cpp:404       (melee cone — every enemy ITeamAgent in range takes damage)
SummonedUnit.cpp:1536       (AcquireTarget)
SummonedUnit.cpp:2053
Tower.cpp:220
Tower.cpp:378
SpellLibrary.cpp:64
SpellLineSweep.cpp:133
SiegeCheatManager.cpp:130
```
**RAW COUNT: 8 hits over 6 files** (a 9th hit, `SiegeStuckStatics.cpp:36`, is a COMMENT about the cost of these scans — classified and excluded).

**LEG 2 — AND THIS IS THE HALF THAT MAKES IT WORSE THAN "THE NPC GETS SHOT."** `ASummonedUnit::IsTargetAlive` (`SummonedUnit.cpp:3528-3556`) ends:

```cpp
	// unknown ITeamAgent types (M2 towers/walls) have no death API yet — treat as alive
	return true;
```
`ACommanderNpc` has no HP, no death API and `SetCanBeDamaged` semantics that never resolve. ⇒ an `ITeamAgent` commander would be a **PERMANENTLY-ALIVE, UNKILLABLE AGGRO SINK standing in the grand hall**: enemy units would path into the enemy castle, attack it forever, and **never re-target** — a stall that presents at playtest as *"units stop attacking the castle"* and gets attributed to pathing or to the 9× castle, not to this class.

**LEG 3 — THE PROJECT ALREADY WROTE THIS RULING DOWN, ABOUT THE SAME HAZARD, IN LAW.** `GoldNode.h:78-82`:
> *"NOT a combatant: deliberately does NOT implement `ITeamAgent` — unit acquisition scans `ITeamAgent` actors (TASK-004), so implementing it would make enemy units target the mine … the old ownership-metadata `Team`/`GetTeam` is REMOVED."*

**⇒ DECISION, AND IT IS A DECLARED DEPARTURE FROM THE SPEC'S SUGGESTED NAME (`SC-§15`):**
- ⛔ `ACommanderNpc` **does NOT implement `ITeamAgent`.**
- ✅ The accessor is **`ETeamId GetCommanderTeam() const`**, ⛔ **not `GetTeamId()`**. The rename is itself the control: a non-virtual `GetTeamId()` on a class that does not implement the interface is a standing invitation to write `Cast<ITeamAgent>(Npc)->GetTeamId()` and get a null cast — exactly the ambiguity `AGoldNode` removed its accessor to kill.
- **⚠️ RELAY THIS NAME TO TASK-563 AND TASK-564.** It is the one symbol in this task whose name differs from the spec text. `InteractRadius`, `EnemyRevealCost` and `IsPlayerInRange` are **character-for-character as specced.**

---

## 3. 📌 M8 DECLARATION — **TIER C**, DECLARED, ⛔ NOT COPIED BOILERPLATE (`WR-§8`)

⛔ **`ACommanderNpc` is NET RELEVANCY TIER C — NOT REPLICATED.** Declared in a header comment (`CommanderNpc.h`, class doc) **and** at the declaration site in the constructor (`CommanderNpc.cpp`). "Tier not declared is a QA FAIL, and Tier C is a DECLARATION, not an exemption."

| M8 question | answer in this file |
|---|---|
| new replicated property | ⛔ **none** |
| new replicated class | ⛔ **none** — `bReplicates` stays at the `AActor` default (`false`) and is **never touched**; the only occurrence of the token is the declaring comment |
| new relevancy tier | ⛔ **none** |
| new RPC | ⛔ **none in this file.** The batch's two (`ServerRequestEnemyReveal` / `ClientReceiveEnemyReveal`) are TASK-563's, on `ASiegePlayerController`, because gold is authority-owned |
| `HasAuthority()` guard needed | ⛔ **no mutation of gameplay truth exists here to guard.** The engine authority query appears **nowhere** in the TU (grep in §1) |
| `GetFirstPlayerController()` | ⛔ **zero uses** (grep in §1) |

**THE STRUCTURAL REASON TIER C IS CORRECT, not just cheap:** `ACastle::BeginPlay` runs on the server **and** on the client, so TASK-562 spawns **one local commander per machine** and pushes the castle's already-replicated `Team` into it. The NPC is a pure local projection of replicated truth — the `AAncientGround` / `AGoldNode` precedent, verbatim.

---

## 4. ⛔ `SC-§33` — THE PASTED CALL-SITE GREP (the gate re-runs it)

**FIRST, THE SCOPE FINDING, because it decides what the obligation even is:** `SC-§33` binds *"a DEFAULTED parameter added to a function that ALREADY HAS CALL SITES"* and its own scope note says it *"does **not** bind a brand-new function."* **TASK-559 is NEW FILES ONLY: it adds no parameter to any existing function, defaulted or otherwise, and it edits no existing file.** ✅ **The obligation is therefore discharged by evidence rather than by classification — here is the evidence anyway, because a sweep that finds nothing is a RESULT and is reported as one (`SC-§22`).**

**(a) NO DEFAULTED PARAMETER EXISTS IN THE NEW PUBLIC SURFACE.**
```
$ grep -nE "^[^/*]*\b(void|bool|float|int32|ETeamId|static ACommanderNpc\*|virtual void)\b.*\(.*=.*\)" \
      Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.h
(no output — exit 1)
```
**RAW COUNT: 0.** The two private helpers `ResolveAvatarMesh(bool)` / `ResolveWarTableMesh(bool)` take a **REQUIRED** bool — *"the compiler audits that one for you, which is precisely the property a default trades away."* Both call sites pass it explicitly with an inline named-argument comment (`/*bWarnIfMissing=*/`).

**(b) EVERY NEW SYMBOL, SWEPT OVER THE WHOLE `Source/` TREE.**
```
$ grep -rn "ACommanderNpc\|InitCommanderNpc\|GetCommanderTeam\|IsPlayerInRange\|GetInteractRadius\|GetEnemyRevealCost\|FindCommanderNpcForTeam\|InteractRadius\|EnemyRevealCost\|AvatarMesh\|WarTableMesh\|AvatarAnimClassAsset\|ResolveAvatarMesh\|ResolveWarTableMesh\|CommanderTeam" Source/ | wc -l
93
$ grep -rln  (same pattern)  Source/
Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.cpp
Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.h
```
**RAW COUNT: 93 hits / 2 files — and both files are the two this task created.** Per-symbol, hits outside my two files:

| symbol | total hits | hits OUTSIDE `CommanderNpc.{h,cpp}` | classification |
|---|---|---|---|
| `ACommanderNpc` | 15 | **0** | (iii) future caller **TASK-562** (spawn) + **TASK-563** (read) — named, not editable by me |
| `InitCommanderNpc` | 3 | **0** | (iii) sole future caller **TASK-562** |
| `GetCommanderTeam` | 4 | **0** | (iii) future readers **TASK-562/563** — ⚠️ **name differs from the spec, see §2** |
| `IsPlayerInRange` | 3 | **0** | (iii) sole future caller **TASK-563** (the map gate) |
| `GetInteractRadius` | 1 | **0** | (iii) **TASK-563** |
| `GetEnemyRevealCost` | 1 | **0** | (iii) **TASK-563** (the authority reads it before `SpendGold`) |
| `FindCommanderNpcForTeam` | 2 | **0** | (ii) **ships with zero call sites by design** — legal per `SC-§33`'s own scope note; see §7 |
| `InteractRadius` | 10 | **0** | own UPROPERTY |
| `EnemyRevealCost` | 7 | **0** | own UPROPERTY |
| `AvatarMesh` / `WarTableMesh` | 28 / 24 | **0** | own components |
| `AvatarAnimClassAsset` · `ResolveAvatarMesh` · `ResolveWarTableMesh` · `CommanderTeam` | 4 / 4 / 4 / 6 | **0** | own members/helpers |

⚠️ **ONE NAME I SWEPT AND CLASSIFIED RATHER THAN ASSUMED — `SceneRoot`: 25 hits, 20 of them outside my files** (`AncientGround` · `BattlefieldScatter` · `CaptureZone` · `Projectile` · `SpellLineSweep`). **Not a collision:** each is that class's OWN `CreateDefaultSubobject` name, which is per-class scoped. Reported because a bare grep on it looks alarming and `SC-§29`'s lesson is that a citation is a lower bound.

---

## 5. ⚠️ THE NUMBERS — WHICH I INVENTED, WHICH I TRACED, AND WHY THE INVENTED ONES ARE LEGAL

| number | status | where it lives | justification |
|---|---|---|---|
| **`InteractRadius = 400` uu** | ⚠️ **INVENTED — DECLARED AS ONE IN THE HEADER, per spec (3)** | `CommanderNpc.h` UPROPERTY doc | The `WR-§5` distinction is **pasted verbatim into the doc comment**: `AS-§21.4` forbids inventing a radius for a **PLACE SYMBOL**, because that radius becomes a **semantic claim the model reasons over** and silently **mis-selects units**. An **INTERACTION** radius is a **UI affordance the player feels directly**, tunes by walking, and **can never mis-select anything**. ⇒ permitted; **FLAGGED to Jonathan as a feel tunable.** ⛔ Written down in the code so nobody cites `AS-§21.4` to block this, **and so nobody cites this to justify a place radius.** |
| **`CommanderWarTableForwardOffset = 200` uu** | ⚠️ **INVENTED — DECLARED** | `CommanderNpc.cpp` anon namespace | Where the table sits relative to the commander (actor +X, floor plane). ~2 m = "arm's-length plus" for a person at a table, and **comfortably inside the 400 uu gate**, so a player who has walked to the table is in range **by construction**. Set as the component template's relative location ⇒ **`BP_CommanderNpc` (TASK-568) is the tuning surface**, not a runtime write. **FLAGGED** for the integration pass once `SM_WarTable`'s real footprint exists. |
| **`CommanderAvatarYawOffset = -90`** | ✅ **TRACED, ⛔ NOT INVENTED** | `CommanderNpc.cpp` anon namespace | The fleet's ONE baked forward. `ASiegePlayerController::GhostYawOffset = -90.f` (`SiegePlayerController.h:1171`) and `ASummonedUnit::SkeletalVisualYawOffset = -90.f` (`SummonedUnit.h:659`), both derived in `SummonedUnit.cpp:415-424` from the shared `SK_Footman_Skeleton` arriving UE-local **+Y** (measured in-engine, TASK-326). ⛔ **Without it the commander stands SIDEWAYS to the room TASK-562 aims him at** — a silent visual bug nobody would trace to this file. |
| **`EnemyRevealCost = 30`** | ✅ **JONATHAN'S OWN NUMBER** | `CommanderNpc.h` UPROPERTY | *"You can pay 30 gold to reveal all enemy locations."* `int32`, `EditDefaultsOnly`, `ClampMin 0`, doc comment ends `// GDD §3.15` (the section **TASK-572** will write) and the declaration carries the same trailing token — mirroring the shipped `ObstaclePlacementClearance` / `// GDD §5 (M4.5)` style, ⛔ no new convention invented. ⛔ **A UPROPERTY default, NEVER a `cards.csv` column** (§3.0). |

### `SC-§34` — the one ledger row this task owns

The batch's defining-dimension change is the 9× castle (TASK-557's ledger). **This task adds exactly one constant that a reader could think should scale with it, and it is marked (ii):**

| # | constant | disposition |
|---|---|---|
| A | **`InteractRadius = 400`** | ⛔ **(ii) DELIBERATELY UNCHANGED — HUMAN-SCALE.** `SC-§34`'s human-scale exemption, `WR-§1`'s clause: *"a number keyed to a BODY — a step, a doorway threshold, a capsule, an INTERACTION RANGE — is NOT multiplied by 3, and multiplying it is the defect."* This is a person walking up to a table. The reason is written into the UPROPERTY doc so the next reader of the 9× castle does not "fix" it. |
| B | **`CommanderWarTableForwardOffset = 200`** | ⛔ **(ii) DELIBERATELY UNCHANGED — HUMAN-SCALE**, same clause: it is arm's reach, not castle geometry. |

---

## 6. WHAT THE CLASS ACTUALLY IS (the shape, for the reviewer)

`ACommanderNpc : public AActor` — ⛔ **not `APawn`/`ACharacter`, and that is load-bearing.** A Pawn brings a **capsule** — a blocking primitive that would carve the castle's interior navmesh in the grand hall the units now spawn into — plus an auto-possession surface and a controller the AI would have to learn to ignore. The NPC never moves, fights or pathfinds.

- `SceneRoot` (`USceneComponent`) — the actor origin **is** TASK-562's anchor point.
- `AvatarMesh` (`USkeletalMeshComponent`) — soft-resolves `/Game/Characters/SK_Sorcerer`, null-safe, Movable (**required**, not cosmetic: a Static component refuses `SetSkeletalMeshAsset` after `BeginPlay`, which would break the deferred resolve). Yaw `-90` applied in the constructor as the component template's rotation, so a BP override wins cleanly; ⛔ **deliberately not re-applied at runtime, which would stomp that override.**
- `WarTableMesh` (`UStaticMeshComponent`) — soft-resolves `/Game/Meshes/SM_WarTable`, null-safe, Movable for the same reason. `SM_WarTable`'s origin is its **floor-contact plane** (TASK-556 spec) and `SK_Sorcerer` is **feet-origin**, so both sit on the floor at relative `Z = 0` with **no per-asset Z fudge**.
- Resolves **twice** — `OnConstruction` (**silent**, because it re-runs on every property tweak while an artist nudges the anchor) and `BeginPlay` (**warns once**). The `AGoldNode` discipline exactly.
- `PrimaryActorTick.bCanEverTick = false`. **No timer, no tick, no overlap event, no delegate.** The proximity gate is **polled by the caller** (TASK-563) through `IsPlayerInRange`, which keeps the cost on the one client that can open a map instead of on two actors every frame.
- `IsPlayerInRange` — the house arena metric, `static_cast<float>(FVector::DistSquared2D(...)) <= FMath::Square(InteractRadius)`, Z ignored (so the raised interior floor never makes the table unreachable). **A non-positive radius answers `false` for every point** — an empty disc contains nothing, and it is also how a designer disables the gate on an instance.

---

## 7. ⚠️ DECLARED DEPARTURES AND ADDITIONS (`SC-§15`) — every one of them, named

1. **`GetCommanderTeam()` instead of `GetTeamId()`, and NO `ITeamAgent`.** — §2. The spec conditioned it on a diagnosis; the diagnosis says no. ⭐ **The most important line in this handoff to relay onward.**
2. **`static ACommanderNpc* FindCommanderNpcForTeam(UWorld*, ETeamId)` — AN ADDITION over the spec's `names:` block.** Reason: the house law puts a finder on the class being **found** (`AGoldNode::FindBestMineFor` · `AAncientGround::FindNearestAncientGround` · `ACastle::FindNearestCastleForTeam`), and its consumer is already on the board — TASK-563 must locate the own-team NPC. Shipping it here keeps a `TActorIterator` out of `ASiegePlayerController`. ⚠️ **Shipping with zero call sites is legal and intended** (`SC-§33`'s scope note; the `FSiegeAssistantRegionStatics::IsPointInRegion` precedent). **TASK-563 is free to ignore it.**
3. **A `SceneRoot` component the `names:` block does not list.** House-standard (`AAncientGround`), and it is what makes the actor origin the clean anchor pivot for TASK-562.
4. **⛔ NoCollision + `SetCanEverAffectNavigation(false)` on BOTH meshes.** The spec is silent on collision, so I chose, and I am declaring it. ⚖️ **The reason: this prop stands INSIDE the castle interior, which is navigable space the units now spawn into and the hero walks through. A blocking prop there is a traversability hazard.** The `AGoldNode` "blocks NOTHING" posture, applied for the same reason.
   ⇒ ⚠️ **A DESIGNED, FLAGGED CONSEQUENCE, NOT A BUG — and I recommend it be added to `WR-§9`'s playtest sheet as outcome 8: THE PLAYER CAN WALK THROUGH THE WAR TABLE.** Making it solid is one line (a blocking profile on `WarTableMesh`) and is **Jonathan's call** — deliberately not taken unasked, because it trades a cosmetic wrinkle for a navmesh risk. ⛔ **I did not edit `CONVENTIONS.md`; that is the manager's file.**
5. **`AvatarAnimClassAsset` → `/Game/Characters/ABP_Footman` (the shared locomotion ABP).** Not in the spec. Free: the whole fleet is on `SK_Footman_Skeleton`, so the commander **idles** instead of standing in ref pose, and it is the SAME asset `ASummonedUnit` already falls back to. Unresolved ⇒ no anim instance ⇒ ref pose, never a crash, and **deliberately not warned** (a ref-pose commander is a cosmetic downgrade, not a fault). ⚠️ **The path string is a deliberate DUPLICATE of `SummonedUnit.cpp`'s `SharedLocomotionAbpPath` — not a refactor, because `SummonedUnit.{h,cpp}` is not my file this batch.** The duplication is marked PAIRED in the code: rename the ABP and both sites change.
6. **`VisibilityBasedAnimTickOption = OnlyTickPoseWhenRendered` + `bEnableUpdateRateOptimizations = true`** on the avatar. Mirrors the fleet URO law (TASK-285) and is **safer here than on a unit**: this actor has *no* gameplay behaviour keyed to its pose at all — no root motion, no AnimNotify, no attack cadence, no movement. The only thing that can lag is a visible idle.
7. **⚠️ D5 — A CONFLICT BETWEEN THE BOARD AND THE DISPATCH RELAY, DECLARED PER THE "BOARD IS THE CONTRACT" LAW.** The board spec (2) reads *"Re-using the shipped Sorcerer is a **RULING** (D5) … ⛔ Do not request a new model."* The dispatch relay reads *"D5 — the NPC's body — is a **FLAGGED, UNRULED** decision."* **I followed the BOARD** — and the two are reconcilable: **re-use is the BATCH ruling on cost grounds; what the commander ultimately looks like is still Jonathan's to decide.** ✅ **My assumption, stated as the dispatch required: `SK_Sorcerer` is a placeholder body, chosen because it costs zero credits and zero rig work, and swapping it is ONE soft path (`AvatarMeshAsset`) in `BP_CommanderNpc` touching no logic.** ⛔ **I requested no new model and spent nothing.**

---

## 8. CROSS-TASK NOTES — ⚠️ READ BEFORE THE DOWNSTREAM TASKS ARE DISPATCHED

- **→ TASK-562 (`ACastle` furnishing):** spawn, attach to `CastleMesh`, then call **`Npc->InitCommanderNpc(Team)`**. ⚠️ **`InitCommanderNpc` is a PUSH — do NOT let the NPC derive its team.** ⚠️ **AND A HAZARD I CANNOT FIX FROM MY FILE, REPORTED RATHER THAN CODED AROUND (`SC-§15`):** `ACastle::Team` is `COND_InitialOnly` replicated, and on a **client** `BeginPlay` can run **before** initial replication settles. `Castle.h:287` argues the value is *"level-authored identically on both machines already"*, which makes it safe today — **but that argument is a property of the level, not of the code**, and it is TASK-562's to re-confirm, not mine to assume.
- **→ TASK-563 (controller surface):** the symbols you get are **`ACommanderNpc::FindCommanderNpcForTeam(World, Team)`** → **`IsPlayerInRange(PlayerLocation)`** → **`GetEnemyRevealCost()`** / **`GetInteractRadius()`** / **`GetCommanderTeam()`**. ⛔ **`GetTeamId()` DOES NOT EXIST ON THIS CLASS and `Cast<ITeamAgent>(Npc)` RETURNS NULL — by design (§2).** ⛔ **The proximity gate is the MAP's only. The console open path must stay byte-unchanged** (`WR-§5` RULING 5).
- **→ TASK-564 (tests):** `IsPlayerInRange` is a member on a spawned actor, so testing it needs a world + a spawn; the pure part is one `DistSquared2D` comparison. ⚠️ **If a world-less test is wanted, say so as a FINDING rather than adding a static shim to my file after the fact** — I deliberately did not add one, to keep the surface exactly at spec.
- **→ TASK-566 (import) / TASK-568 (BP):** `/Game/Meshes/SM_WarTable` must land at that exact path. `BP_CommanderNpc` is the tuning surface for the table offset, the avatar yaw and both feel tunables.
- **→ TASK-556 (art):** ✅ I depend on the specced **floor-contact origin (min-Z ≈ 0)** for `SM_WarTable`. If that changes, this class needs a Z offset it currently does not have.

---

## 9. ⚠️ WHAT I DID **NOT** VERIFY, AND WHY (`SC-§32` posture — say it rather than imply green)

1. ⛔ **NOTHING HERE HAS BEEN COMPILED.** File-only by spec; TASK-566 owns the batch's only compile. **Every API claim below is a code READING, not a build result:** `SetSkeletalMeshAsset` / `SetAnimInstanceClass` / `SetMobility` / `SetCanEverAffectNavigation` / `UCollisionProfile::NoCollision_ProfileName` / `VisibilityBasedAnimTickOption` are each used with the same spelling by shipped Siegebound code (`SummonedUnit.cpp`, `BattlefieldScatter.cpp`, `Torch.cpp`) — I matched the shipped usage rather than trusting memory, but **the compiler has not agreed yet.**
2. ⛔ **I have not seen `SK_Sorcerer` in the editor.** I am relying on the shipped `SummonedUnit.cpp:415-424` derivation that the whole rigged fleet shares `SK_Footman_Skeleton` with a **+Y** baked forward. ⚠️ **If the commander renders sideways at the first PIE, the fix is `CommanderAvatarYawOffset`, and it is one constant in one anonymous namespace** — that is the single most likely cosmetic surprise from this file, and it is named here so nobody hunts for it.
3. ⛔ **`SM_WarTable` does not exist yet**, so the null-safe path is the one that will run first. The *resolved* path is untested by anything.
4. ⛔ **No PIE, no MCP, no editor** — the proximity radius has not been walked, and 400 uu is an educated guess flagged for Jonathan's feel pass, not a measurement.
5. ⛔ **`ETeamId` has no `None`.** A commander that never receives `InitCommanderNpc` is **Blue**, per spec. On a Red castle that would make the Red player's map refuse to open. ⚠️ **Fail-open-as-Blue is the spec's default, not my choice** — recorded because it is the failure mode if TASK-562's push ever regresses, and the `CommanderNpcInit` log line (`team=…`, two lines per match) is the diagnostic that catches it.
