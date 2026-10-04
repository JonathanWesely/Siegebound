<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ DEATH, THE GHOST, AND THE 3-MINUTE TIMER (2026-09-01) — namespace **GHOST-§**

📌 **BORN WITH ITS NAMESPACE PREFIX (`GHOST-§N`).**

**Trigger — Jonathan's directive, verbatim:** *"I also want to add a respawn timer. If the player dies at any point during the match, they are dead for 3 minutes. During this time they instead get a ghost creature that they can control that is similar to their original body where it can command units, access the AI commander, and look at the map, it just cannot attack or be attacked. The enemy should be able to see this ghost as well. When the 3 minutes are up they respawn back at the castle."*

### GHOST-§0 ⭐⭐⭐ THE MEASUREMENT HE MUST SEE BEFORE ANYONE BUILDS THIS — **THE SHIPPED DEATH PENALTY IS 5 SECONDS. HE IS ASKING FOR 180. THAT IS 36×.**

> ### ⚠️⚠️ **`HeroRespawnDelay` SHIPS AT **5 s** (`SiegeGameMode.h:48`, GDD §3.1 *"back within 5-6 s"*). HIS ASK IS **180 s**. ⛔ THE NUMBER IS HIS AND ⛔ IT IS ⛔ NOT CHANGED BY ANY AGENT — BUT HE IS ⛔ NOT TOLD "3 MINUTES" IN ISOLATION. HE IS TOLD **36×**, BESIDE A MEASURED TIME-TO-LOSE.**

- ⚠️⚠️ **AND THE SECOND HALF OF THE FLAG, WITH ITS RECORD CORRECTED RATHER THAN REPEATED:** this pipeline has recorded that **an unattended castle dies at ~T+4 min** — `TASKBOARD.md:9621` (CR-R5, the note that bought `SIE-§1`): *"SIE runs the full AI war unattended; **a castle dies ~T+4min** and takes its commander with it."*
  - ⛔⛔ **THE COMMONLY-REPEATED "~3 MINUTES" IS A MIS-READING AND IS CORRECTED HERE: the 3-minute figure in `SIE-§1` is the *READ WINDOW*** (*"every live read lands in the first ~3 minutes of its session or the session restarts"*) — **a deliberate SAFETY MARGIN set BELOW the castle-death time. ⛔ It is ⛔ NOT the castle-death time.** ⚖️ **Reporting "3-minute respawn vs 3-minute raze" would be manufacturing alarm out of a margin, and `AS-§12g` forbids exactly this class of number-laundering.**
  - ⚠️ **AND THE ~4 MIN FIGURE IS ITSELF WEAK EVIDENCE, STATED SO IT IS NOT LEANED ON:** it is an **incidental observation from an unattended SIE session**, ⛔ **never a deliberate measurement**, and it **pre-dates `HIGH-§` (×3 ranged range + the elevation damage bonus)** — **both of which make the attacker strictly stronger.** ⇒ ⛔ **the true current figure is UNKNOWN and is very likely LOWER.**
- ⇒ ⭐⭐ **THEREFORE THE RAZE TIME IS RE-DERIVED AT THE CURRENT BUILD AND PUT BESIDE HIS NUMBER. ⛔ THE MEASUREMENT BLOCKS ⛔ NOTHING — his 180 s ships either way and the task is a one-line retune if he rules.** ⚖️ **A measured comparison is worth more than an opinion, and he can rule in one word.**
- ✅ **THE LEVERS, NAMED FOR HIM SO THE RULING IS ONE WORD AND ⛔ NOT AN ESSAY:** **(a)** a shorter timer · **(b)** a timer that SCALES with match time (short early, long late) · **(c)** ⭐ **the ghost retains more influence — and `G-5` below is the concrete, already-identified lever** · **(d)** castle durability · **(e)** ⛔ **do nothing — a 3-minute death IS meant to be near-fatal.** ⚖️ **(e) is a legitimate answer and is listed so the question does not read as lobbying.**
- 📌 **AND THE STANDING PARKED DATUM IS SPENT BY THIS, ⛔ not duplicated:** the board's **JR4** row parks *"the AI-razes-undefended-castle balance datum (659 §5.2)"*. ⇒ **this measurement DISCHARGES it; ⛔ do not board a second one.**

### GHOST-§1 ⭐⭐ THE HEADLINE DESIGN RULING — **THE GHOST IS A NEW PAWN THAT DOES ⛔ NOT IMPLEMENT `ITeamAgent`, AND THAT ONE FACT DELIVERS "CANNOT BE ATTACKED" FOR FREE**

> ### ✅⭐⭐ **`ASiegeGhostPawn : public APawn` — and it ⛔ DOES NOT implement `ITeamAgent`. ⇒ "CANNOT BE ATTACKED" IS ⛔ NOT A MECHANISM. IT IS A STRUCTURAL PROPERTY, AND IT COSTS ⛔ ZERO LINES OF SUPPRESSION CODE.**

**MEASURED AT TWO INDEPENDENT SOURCES, 2026-09-01 — ⛔ this is a reading, ⛔ not an assumption:**
- **`ASummonedUnit::AcquireTarget` (`SummonedUnit.cpp:1675`)** enumerates candidates with **`UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents)`**.
- **`ATower` (`Tower.cpp:220` and `:378`)** does **the identical thing**.
- ⇒ ⭐ **AN ACTOR THAT DOES NOT IMPLEMENT `ITeamAgent` IS ⛔ NEVER RETURNED TO EITHER, AND THEREFORE CAN ⛔ NEVER BE ACQUIRED BY THE UNIT FLEET OR BY ANY TOWER.**
- ✅ ⇒ ⛔ **WRITE A **TEST** THAT PROVES IT, ⛔ NEVER A MECHANISM.** ⭐ **`TOWER-§9.2`'s "attackable is free" idiom, inverted and applied a second time.** ⚖️ **A structural property cannot be forgotten at a guard point; a flag can.**

> ### ⭐⭐⭐ **AND THE SAME RULING IS THE PARALLELISM ARGUMENT — SAID OUT LOUD BECAUSE IT IS ⛔ NOT A COINCIDENCE:**
> **The REJECTED alternative — a `bIsGhost` flag on `AHeroCharacter` — would force `ASummonedUnit::IsTargetAlive` (`SummonedUnit.h:1952`, `.cpp:3843`) to learn about it.** ⛔ **That file is `TASK-738`'s SOLE-OWNED surface in the LIVE ladder wave** ⇒ **the flag shape is a hard write-collision and ⛔ NOT parallel-safe.**
> ⇒ ⭐ **The better-designed shape and the schedulable shape are the SAME shape.** ⛔ **The flag shape is refused on BOTH grounds and is recorded here so it is ⛔ not re-proposed as a "simpler" fix.**

#### ⭐ AMENDED 2026-09-01 BY TASK-749's OWN MEASUREMENT — **THE RULING IS ⛔ NOT WEAKENED; IT IS *STRONGER* AND MORE LOAD-BEARING THAN THIS SECTION CLAIMED**

- ⭐⭐ **THERE ARE ***EIGHT*** `ITeamAgent` ACQUISITION SITES, ⛔ NOT THE TWO CITED ABOVE** — TASK-749 enumerated them rather than trusting this section's two. ⚠️ **The one that matters most was missing from the law: `FSiegeCombatStatics::ApplyRadialDamage` (`SiegeCombatStatics.cpp:36`) enumerates by the SAME interface ⇒ ⭐ EVERY AoE IN THE GAME.** ✅ **Confirmed at source before this amendment was written** (⛔ not on relay). ⇒ **the ghost is free of splash damage for the same structural reason it is free of targeting, and `AGoldNode`'s shipped non-implementation is the precedent that proves the idiom already carries weight** (`SiegeCombatStatics.cpp:33-34` says so in its own comment).
- ⛔⛔ **THE CONSTRAINT THIS BUYS, AND IT IS A REAL TRAP: THE GHOST CAPSULE'S OBJECT TYPE MAY ⛔ NOT BE RE-TYPED TO `WorldStatic`.** ⚠️ **TASK-749 measured that doing so would insert the ghost into the *projectile terrain trace*** ⇒ ⭐⭐ **a dead player would be handed a PROJECTILE SHIELD — his corpse's ghost soaking shots aimed at his own army.** ⚖️ *The `ITeamAgent` ruling makes the ghost unattackable; a careless object type would make it a fortification.* ⛔ **This is ⛔ NOT a style preference. Any change to `ASiegeGhostPawn`'s collision object type must re-measure the projectile trace and say so.**

### GHOST-§2 ⭐ WHAT ALREADY SHIPS — **THE RESPAWN IS A RETUNE, ⛔ NOT A NEW MECHANIC**

Read at source 2026-09-01, `SiegeGameMode.h:46-51` + `:243-251`:
- ✅ **`FOnHeroDied` → `HandleHeroDied` → a per-controller `HeroRespawnTimers` entry → teleport (PlayerStart, else beside the hero's own castle) → repossess → `ResetHero()` (full HP, input restored).** ⭐ **All of it ships and all of it is QA-passed.**
- ✅ **`HeroRespawnDelay` is already a `UPROPERTY` on `ASiegeGameMode`.** ⇒ ⭐ **HIS 3 MINUTES IS ONE NUMBER: `5.f → 180.f`.**
- ✅⭐ **"WHAT HAPPENS IF THE MATCH ENDS WHILE DEAD" IS ⛔ ALREADY RULED AND NEEDS ⛔ NO NEW RULE:** *"After match end the hero stays down; `PlayAgain()` revives it"* and *"After match end no respawn is scheduled."* ⇒ ⛔ **The ghost inherits this exactly: match end retires the ghost, the end screen shows, `PlayAgain()` restores the hero.** ⛔ **Do ⛔ not invent a second match-end rule.**
- ⛔ **THE `// GDD §3.1` COMMENT BESIDE `HeroRespawnDelay` IS REWRITTEN, ⛔ NOT LEFT LYING.** It currently encodes *"back within 5-6 s"*, which this change makes **false**. ⇒ **it must record WHOSE ruling this is, the new number, and the consequence** — the **`TASK-517` / `HIGH-§1` idiom** (a shipped comment that contradicts the shipped value is the drift defect this project keeps paying for).
- ⚠️ **TIMER POLICY IS QA-BINDING AND UNCHANGED** (`SiegeGameMode.h:53-58`): `PlayAgain()` clears **ONLY** the specific handles this class owns (the `HeroRespawnTimers` map), **BEFORE `ResetGold()`** — ⛔ **never a world-wide clear, ⛔ never another system's timer.** ⚠️ **A 180-second timer makes a leaked handle survive far longer than a 5-second one did** ⇒ **the ghost's own teardown obeys the same policy.**

### GHOST-§3 ⚖️ THE RULINGS — decided, with reasons, ⛔ none blocking

| # | Question | ⚖️ **Ruling (proceeding default)** | Why |
|---|---|---|---|
| **G-1** | Movement speed + vision? | ✅ **THE HERO'S OWN** — ⛔ no flight, ⛔ no wall-pass, ⛔ no extended vision | ⛔ **His words: *"similar to their original body."*** ⭐ Anything else is a scouting buff granted as a reward for dying |
| **G-2** | Collision? | ✅ **BLOCKS `WorldStatic` ONLY** (so it walks the ground and does ⛔ not fall through the world); ⛔ **ignores/overlaps `Pawn`** | ⚠️ **A ghost that BLOCKS pawns is a free body-block wall — a combat mechanic handed to a dead player.** ⚠️ A ghost with NO collision falls through the floor |
| **G-3** | Can it capture, interact, or pick anything up? | ⛔ **NO.** It **issues orders, uses the AI commander, opens the map** — and ⛔ nothing else | ⛔ **His enumeration is CLOSED and it is short.** ⛔ Zone capture by a dead player is a win condition handed to a corpse |
| **G-4** | Enemy-visible? | ✅ **YES** — ⛔ **his explicit words, ⛔ not a default** | ⚠️ **Deliberately the OPPOSITE of `MARK-§ M-3`. ⭐ Say the contrast out loud so nobody "makes them consistent."** ⚠️ Same honest limit as `RECALL-§ R-3`: the bot does ⛔ not look at it, so the tell is for the human observer |
| **G-5** ⭐⭐ | **May the ghost PLAY CARDS?** | ⛔ **NO (default)** — ⚠️⚠️ **BUT THIS IS THE FOR-JONATHAN ROW THAT MATTERS MOST AFTER THE TIMER ITSELF** | ⛔ **Card play is ⛔ NOT in his enumeration, and a closed list is read as closed.** ⚠️⚠️ **BUT: no attacking + no cards + 180 s = the player is a spectator with a chat window for three minutes, while `GHOST-§0` says the castle may fall inside that window.** ⇒ ⭐⭐ **THIS IS THE CHEAPEST LEVER THAT SOFTENS THE PENALTY WITHOUT TOUCHING HIS NUMBER — which is exactly why he is asked rather than told** |
| **G-6** | Hero dies mid-recall? | ✅ **The channel ABORTS** — it is one of `RECALL-§4`'s seven enumerated exits | ⛔ A channel that survives its own caster's death is the `TOWER-§8` hanging-unit class in a new costume |
| **G-7** | Hero dies mid-**climb**? | ⛔⛔ **MOOT — AND SAID SO RATHER THAN TASKED.** ✅ **MEASURED: `BeginLadderClimb` is on `ASummonedUnit` (`TOWER-§8.4(B)`). The HERO ⛔ NEVER CLIMBS.** | ⚖️ **A question whose premise is false gets a stated refutation, ⛔ not a defensive task.** ⛔ **Do ⛔ not write ghost/climb interaction code** |
| **G-8** | While dead, what does the `hero` place symbol resolve to? | ✅ **THE GHOST'S LOCATION** | ⭐⭐ **This costs ⛔ ZERO extra prompt characters — same symbol, different resolution.** ⚖️ **`follow` and `rally` are hero-relative intents** (Zone A: *"follow = they follow the hero"*, *"rally = hero rallies units near him"*) ⇒ **resolving `hero` to a hidden corpse would silently walk the player's army to where he died.** ⇒ ✅ **`rally` and `follow` keep working and the ghost is the anchor** |

### GHOST-§4 ⛔⛔ THE POSSESSION + CURSOR LAW — **THE ONE PLACE THIS FEATURE CAN BOOT THE ARENA INPUT-DEAD**

- ⛔⛔ **CURSOR AND INPUT-MODE OWNERSHIP GOES THROUGH `ApplyCursorInputState()` AND ⛔ NOWHERE ELSE** (`HELP-§5`, verbatim and binding). ⚠️ **A direct `SetInputMode` / `bShowMouseCursor` call is the defect that law exists to prevent — ⛔ it booted the arena input-dead once already and cost a playtest.**
- ⚠️⚠️ **AND THIS FEATURE IS THE MOST DANGEROUS CONSUMER OF IT YET, because it changes the POSSESSED PAWN**: `HandleDeath()` calls **`DisableInput`** (`HeroCharacter.cpp:739` names the mirror) and `ResetHero()` restores it. ⇒ **the ghost possesses BETWEEN those two, so the enable/disable ordering must be exact or the player controls nothing for 180 seconds.**
- ⛔ **The ghost is ADDED to the existing cursor-owner ladder and ⛔ NEVER re-orders it.** The shipped owners keep their exact precedence: placement mode · the Alt-held `IA_UICursor` · `HandleMatchEnd`'s end screen · targeting mode · group-pick · the war map · the assistant console · the `HELP-§` overlay.
- ⚠️ **`ApplyCursorInputState()` normalizes at `BeginPlay`** because input-routing state **survives level travel on the persistent `UGameViewportClient`** (TASK-074, the level-travel law). ⛔ **Dying, ghosting and respawning must leave that composition in the same shape it started in.**
- ⭐⭐ **AND THE INSTRUMENT IS PIE, WITH ⛔ NO SUBSTITUTE (`SC-§35` ruling 5, and it applies by its own terms): this batch SPAWNS A NEW ACTOR CLASS INTO THE WORLD** ⇒ **it owes ONE PIE session with a MESSAGE-LOG READ, and that row may ⛔ NEVER be waived on the grounds that the compile is clean.** ⚠️ **`SC-§35` is the law bought by 1,806 Blueprint runtime errors that compiled clean and cleared TWO QA gates.**
- ⛔⛔⛔ **STANDING LAW, ADDED 2026-09-01 ON TASK-749's MEASUREMENT — ⭐ THIS IS A GENERAL POSSESSION-HAND-OFF TRAP, ⛔ NOT A GHOST-SPECIFIC ONE, AND IT BINDS EVERY FUTURE PAWN SWAP IN THIS PROJECT:**
  > ### ⚠️⚠️ **`IMC_Hero` IS ADDED BY THE HERO ***PAWN***, ⛔ NOT BY THE CONTROLLER.**
  **MEASURED AT SOURCE (`HeroCharacter.cpp:246-280`, `NotifyControllerChanged()` — ✅ re-read and confirmed 2026-09-01 before this line was written, ⛔ not taken on relay): the mapping context is added by `AHeroCharacter` itself, in its own `NotifyControllerChanged`, with its own comment saying `ASiegePlayerController` does ⛔ NOT add contexts the way the template controllers do.**
  ⇒ ⛔⛔ **THEREFORE POSSESSING ANY PAWN THAT DOES NOT ADD ITS OWN CONTEXT LEAVES THE PLAYER CONTROLLING ***NOTHING***.** ⚠️ **A naive death→ghost hand-off does exactly that — and here it would last the FULL 180 SECONDS**, which is `GHOST-§4`'s named catastrophe arriving by a route nobody was watching. ⭐ **TASK-749 solved its own half; the respawn side is TASK-750's.**
  ⇒ 📌 **THE DUTY, BINDING ON EVERY PAWN SWAP: a task that changes the possessed pawn must state, in its handoff, WHICH object adds the mapping context on EACH side of the swap and WHEN.** ⛔ **"The controller handles input" is a false premise in this codebase and may ⛔ never be assumed.**
- ⛔ **`SC-§35`'s ANIM-OWNER LAW BINDS THE GHOST DIRECTLY.** If the ghost carries a `USkeletalMeshComponent`, **any `TSoftClassPtr<UAnimInstance>` assigned to it MUST state, in a comment at the assignment, WHICH OWNER CLASS THE ABP ASSUMES.** ✅ **`ASiegeGhostPawn` is an `APawn`, so `TryGetPawnOwner()` resolves — ⛔ but that must be STATED, ⛔ not left to luck**, and the sanctioned shape for a prop that merely idles is **single-node playback** (`SetAnimationMode(AnimationSingleNode)` + `PlayAnimation(..., /*bLooping=*/true)`), which is **structurally incapable** of that defect class.

### GHOST-§5 NAMING + FILE MAP (the cross-task contract)

| thing | law |
|---|---|
| the pawn | **`ASiegeGhostPawn`** — `Source/GitClaudeUnrealTest/Siegebound/SiegeGhostPawn.{h,cpp}`. ⛔⛔ **DOES ⛔ NOT implement `ITeamAgent`** (`GHOST-§1` — this is the whole design). ⛔ **Does ⛔ not implement `IHealthBarProvider`** (it has no health to show). |
| the timer | **`ASiegeGameMode::HeroRespawnDelay`** — **`5.f → 180.f`**, `EditDefaultsOnly`, **comment rewritten** per `GHOST-§2`. ⛔ **The `HeroRespawnTimers` map, `HandleHeroDied` and the QA-binding timer policy are ⛔ NOT redesigned.** |
| ghost class ref | **`TSoftClassPtr<ASiegeGhostPawn> GhostPawnClassAsset`** on `ASiegeGameMode`, mirroring the shipped **`HeroPawnClassAsset`** pattern (`SiegeGameMode.h:260`): **missing/incompatible ⇒ warn once and fall back to the raw C++ class.** ⛔ **A missing asset ⇒ ⛔ never a crash, ⛔ never a dead 180 seconds with no pawn.** |
| ⛔ untouched | ⛔⛔ **`SummonedUnit.{h,cpp}` · `ClimbableTower.{h,cpp}` · `Tower.{h,cpp}` · `ABP_Footman` — ⛔ NOT TOUCHED BY ANY TASK IN THIS BATCH.** The first two are the LIVE ladder wave's sole-owned surfaces; the ruling in `GHOST-§1` is what makes that possible. |
| ⭐ **the ghost's APPEARANCE — PINNED 2026-09-01, ⛔ the names exist BEFORE the art task** | ⚠️⚠️ **`G-4` MAKES THE MESH FUNCTIONALLY REQUIRED, ⛔ NOT COSMETIC: an unset mesh is an INVISIBLE ghost, which violates his explicit *"the enemy should be able to see this ghost as well."*** TASK-749 correctly ⛔ **refused to invent an asset path the Artist never agreed to** and left the slots unset with a loud `BeginPlay` warning. ⇒ **the three soft-refs resolve to exactly these, and ⛔ nothing else:** **`GhostMesh` ⇒ `/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple`** (⭐ **the HERO'S OWN shipped mesh — ⛔ zero new geometry, and it is the most faithful reading of `G-1`'s *"similar to their original body"***; the `SK_Sorcerer` avatar-reuse precedent) · **`GhostMaterial` ⇒ `/Game/Materials/MI_Ghost_Translucent`** (⭐ **the ONE new asset — a translucent instance, ⛔ not a new master material**) · **`GhostIdleAnimation` ⇒ a `UAnimationAsset`, ⛔ NEVER a `TSoftClassPtr<UAnimInstance>`** (`SC-§35` single-node playback, `GHOST-§4`). |
| ⭐ the ghost BP | **`/Game/Blueprints/BP_SiegeGhostPawn`** — child of `ASiegeGhostPawn`, and the target of `GhostPawnClassAsset`. ⛔ **The C++ class leaves every slot UNSET deliberately; the BP is where they are assigned** (the shipped `BP_HeroCharacter` pattern). ⛔⛔ **AMENDED 2026-09-01 — IT IS ***SEVEN*** SLOTS, ⛔ NOT THREE: see `GHOST-§5a`. The earlier *"the input slots are a separate debt, ⛔ not the art lane's"* wording is ⛔ RETIRED — it is what shipped a still-immobile ghost into the re-board.** |
| ⭐ the ghost material's MASTER | ⚖️ **`/Game/Materials/M_HeroSpirit` — ACCEPTED 2026-09-01 (manager).** The instance's *"parent it to a shipped translucent-capable master"* clause had an explicit escape hatch and **it fired ON MEASUREMENT**: all 5 candidates were measured — **4 are `MD_DeferredDecal`** (structurally impossible on a skeletal mesh) and the 5th, `M_Ghost`, is **`bUsedWithSkeletalMesh = false`** ⇒ ⚠️ **it falls back to the DEFAULT MATERIAL in a PACKAGED build while looking correct in-editor** — the silent-defect class. ⭐ **The artist did exactly what the spec required: said so instead of authoring silently.** Stock nodes only (Custom-HLSL ban honoured), `bUsedWithSkeletalMesh=true`. ⛔ **The pinned instance name did ⛔ not drift.** |

### GHOST-§5a ⛔⛔ **THE BP CARRIES ***SEVEN*** DESIGNER SLOTS AND ⛔ ALL SEVEN ARE ONE DELIVERABLE (added 2026-09-01, from `qa/TASK-753.md` `W-1`)**

> ### ⚠️ **A RE-BOARD THAT COPIES THE ORIGINAL ART SPEC'S THREE APPEARANCE SLOTS SHIPS A ***VISIBLE BUT IMMOBILE*** GHOST — WHICH IS ⛔ NOT AN IMPROVEMENT ON AN INVISIBLE ONE.**

**Verified line by line at `SiegeGhostPawn.cpp:40-172`: ⛔ NONE of the seven is defaulted in the constructor.** Every one is assigned on the BP, and these are the **only** admissible values:

| slot | value | why it is ⛔ not negotiable |
|---|---|---|
| `GhostMappingContext` | **`/Game/Input/IMC_Hero`** | ⛔⛔ **A ghost-ONLY context would silently drop every one of `G-3`'s powers.** The hero's own context is what carries orders, the war map and the assistant console. |
| `MoveAction` | **`/Game/Input/Actions/IA_Move`** | named in the header at `:371`; **null ⇒ the ghost cannot walk** |
| `LookAction` | **`/Game/Input/Actions/IA_Look`** | gamepad look, `:375` |
| `MouseLookAction` | **`/Game/Input/Actions/IA_MouseLook`** | `:379` — the template's twin look bindings; ⛔ not a duplicate of `LookAction` |
| `GhostMesh` | **`/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple`** | `G-1` + `G-4`; ⛔ zero new geometry |
| `GhostMaterial` | **`/Game/Materials/MI_Ghost_Translucent`** | the shipped instance of `M_HeroSpirit` |
| `GhostIdleAnimation` | **`/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle`** | ⭐ a **`UAnimSequence`** on `SK_Mannequin`, which `SKM_Quinn_Simple` rides ⇒ it binds. ⛔⛔ **NEVER `ABP_Unarmed`** (an Anim BP — `SC-§35`) and ⛔ never `BS_Idle_Walk_Run` (a BlendSpace needing drive values). |

- ⚖️ **WHY THIS IS ONE TASK AND ⛔ NOT TWO: the three appearance slots and the four input slots have the SAME owner, the SAME instrument (the BP editor) and the SAME precondition (the class must be compiled).** ⛔ **Splitting them is what produced the gap.**
- ⛔ **THE PRECONDITION IS STRUCTURAL, ⛔ not scheduling: a Blueprint cannot parent to an uncompiled class.** ✅ **Discharged by `4a03e03`'s compile** (measured before it: `search_subclasses(APawn,"Ghost") == []`, with a live anti-vacuity self-check on `"Hero"` proving the negative was real).
- ✅ **`GhostPawnClassAsset` on `ASiegeGameMode` must then point at `/Game/Blueprints/BP_SiegeGhostPawn`, or the raw C++ class keeps spawning and all seven slots stay null.** ⚠️ **The fallback is null-safe and warns — ⛔ it is ⛔ not a substitute for the wire.**
- 🧑 **ACCEPTANCE IS PIXELS, ⛔ NOT A PROPERTY READBACK** (`AS-§6 A(e)`): the ghost must be **seen to MOVE under player input** and **seen by the enemy at play distance**. ⚠️ **It is an `ACharacter` with `UCharacterMovementComponent` ⇒ it WALKS THE GROUND under gravity** (QA ruled the base-class deviation ACCEPT) — ⛔ **do ⛔ not spec it as flying just because Jonathan's sitting note says *"fly the ghost"*; if he wants flight that is a ⚖️ ruling, ⛔ not an implementation detail.**

### GHOST-§6 📌 M8 DECLARATION — ⛔ **DO ⛔ NOT COPY ANOTHER BATCH'S BOILERPLATE; IT IS FALSE HERE**

⛔ **This batch adds a NEW PAWN CLASS that is POSSESSED BY A PLAYER CONTROLLER and that `G-4` requires the ENEMY TO SEE.** ⇒ ⛔ **it is unambiguously a replicated-relevant class in M8 and its TIER MUST BE DECLARED IN A HEADER COMMENT** — the standing duty is *"tier not declared is a QA FAIL"*, and a declaration is ⛔ not an exemption. ⛔ **No RPC and no replicated property are AUTHORED in this batch;** the shape is declared and reserved (`ACC-§8`'s discipline). ⚠️ **`HeroRespawnTimers` is already documented as the M8 per-controller shape (`SiegeGameMode.h:55`) — ⛔ that is not re-designed here.**

---

