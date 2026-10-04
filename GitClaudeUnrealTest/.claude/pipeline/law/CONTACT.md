<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ THE CONTACT CLIMB — the HERO climbs, and a body starts a climb by WALKING INTO the ladder (2026-09-01) — namespace **CONTACT-§**

📌 **BORN WITH ITS NAMESPACE PREFIX (`CONTACT-§N`).** Cite as `CONTACT-§3`, never a bare `§3`.

**Trigger — Jonathan's directive, verbatim (2026-09-01, after playtesting the shipped tower):** *"ok, as of right now the ladder for the tower is not climbable, can we fix that? You do not need to add a climbing animation lets just make sure the playable character and any units can climb the ladder by simplying walking up to it and walking against it."*

### CONTACT-§0 ⛔⛔ THE SCOPE FENCE, FIRST

⛔ **`TOWER-§8.0`'s fence is INHERITED IN FULL and re-stated by reference, ⛔ not re-listed:** `HIGH-§` · the ×3 range · `Cost = 30` · `HP = 250` · `PlatformHeightUU = 1200` · the 600×600 deck · `AClimbableTower : public ABuilding` · `TOWER-§4a`'s fall-damage finding · `TOWER-§4b`'s `bCanWalkOffLedges` ruling. ⛔ **NOT RE-OPENED.**
⛔ **ALSO NOT THIS BATCH:** `TOWER-§8.3`'s pinned geometry (⛔ the mesh is ⛔ NOT re-authored — ⇒ ⭐ **`TOWER-§8.3`'s re-author obligation and `TOWER-§8.5a`'s standoff-voiding condition are ⛔ NOT triggered by anything here**) · the packaged-zip adjudication · the ghost team-colour ruling · TASK-767–774 · Jonathan's other owed sittings.
⭐⭐ **AND THE HAPPIEST FENCE: THERE IS ⛔ NO ART LANE IN THIS BATCH.** ⛔ No mesh, ⛔ no texture, ⛔ no socket, ⛔ no clip, ⛔ no material. **Every deliverable is C++.** ⚖️ *Said out loud because a "make the ladder climbable" ask reads like a modelling problem and is ⛔ not one.*

### CONTACT-§1 ⭐⭐⭐ THE DIAGNOSIS, TAKEN BEFORE ANY DESIGN — **HIS ONE SENTENCE IS ⛔ NOT ONE DEFECT. IT IS ⛔ TWO PROBLEMS OF ⛔ DIFFERENT KINDS, AND ONE OF THEM IS ⛔ NOT A DEFECT AT ALL.**

> ### ⛔⛔ **EVERY CLAIM IN THIS SECTION WAS READ AT THE SOURCE FILE BEFORE THIS LAW WAS WRITTEN — ⛔ NOT TAKEN ON A RELAY (`TOWER-§4a`'s method ruling).** ⭐ **Each row is re-derivable from the cited file and line.**

| # | Claim | ⭐ **MEASURED AT** | Verdict |
|---|---|---|---|
| **C-1** | ⭐⭐ **THE HERO HAS ⛔ NO CLIMB PATH AT ALL — it was ⛔ NEVER BUILT, and it was ruled out ON THE RECORD** | `BeginLadderClimb` / `AbortLadderClimb` / `IsClimbing` / `OnLadderClimbEnded` are declared on **`ASummonedUnit`** and ⛔ nowhere else (`TOWER-§8.4(B)`; ⭐ **re-measured 2026-09-01 post-TASK-776: `BeginLadderClimb` `SummonedUnit.cpp:3632` · `AbortLadderClimb` `:3725` · `IsClimbing` `:3624` · the delegate `SummonedUnit.h:100`, the `UPROPERTY` `:792`, the broadcast `:3781`** — ⚠️ **the struck cites `:3798`/`:3806`/`:3899` were ⛔ BOTH stale (−174) AND ⛔ MIS-PAIRED WITH THE NAMES: 3798 was `IsClimbing`, 3806 was `BeginLadderClimb`, 3899 was `AbortLadderClimb`. ⭐ A cite that was never re-derived is how the mis-pairing survived — see `CONTACT-§10`**). **`AHeroCharacter` is a different class** and `GHOST-§ G-7` states the ruling in its own words: *"`BeginLadderClimb` is on `ASummonedUnit`. The HERO ⛔ NEVER CLIMBS."* Grepped `HeroCharacter.h`: **`Climb` = 0 functional hits** (3 comment mentions only) | ✅⛔ **CONFIRMED — AND IT REFRAMES HIS ASK.** ⭐⭐ **His "playable character" half is ⛔ NOT BROKEN. It is ⛔ ABSENT.** ⚖️ *A missing feature and a broken feature get different tasks, different tests and different acceptance — calling this a "fix" would have sent someone hunting a bug that does not exist.* |
| **C-2** | ⭐⭐ **THE SHIPPED TRIGGER CANNOT ⛔ EVER FIRE FOR A PLAYER — ⛔ not by tuning, ⛔ not by accident, ⛔ structurally** | The one entry point is `AClimbableTower::HandleLadderLinkReached` (`ClimbableTower.cpp:366`), bound to a **`UNavLinkCustomComponent`** move-reached delegate. It resolves the climber via **`ResolvePathFollowingPawn(PathComp)` → `Cast<UPathFollowingComponent>`** (`:59-62`, `:376`) and then **`Cast<ASummonedUnit>`** (`:377`) | ✅⛔ **DECISIVE, ON BOTH HALVES.** **(a)** A player-driven pawn **is not pathing** — it drives input directly ⇒ ⛔ **no `UPathFollowingComponent` move is ever in flight, so the delegate is ⛔ never raised for it.** **(b)** Even if it were, the `Cast<ASummonedUnit>` yields `nullptr` ⇒ `EvaluateLadderEntry` refuses. ⇒ ⛔⛔ **WHAT SHIPPED IS A ***PATHFINDER*** TRIGGER. HE IS ASKING FOR A ***BODY*** TRIGGER. THOSE ARE DIFFERENT MECHANISMS, AND THE SECOND IS ⛔ NOT A TUNING OF THE FIRST.** |
| **C-3** ⚖️ **RESOLVED 2026-09-01 — AND IT CAME BACK ⭐ SPLIT, ⛔ NOT SINGLE** | ⛔ **THE HYPOTHESIS AS WRITTEN IS ⛔ HALF CONFIRMED AND ⛔ HALF REFUTED, AND THE HALVES DIVIDE BY ***WHO IS DRIVING***.** ⛔ **It must ⛔ NOT be collapsed back to one verdict.** | ✅ **CLAUSE 1 — *"only comments reference `ClimbableTower`"* — CONFIRMED, and stronger than the law claimed: 10 non-test mentions, ⛔ ALL comments, ⛔ ZERO executable references** (`SiegeGhostPawn.h:242` · `SummonedUnit.cpp:610`/`:3727`/`:3777` · `SummonedUnit.h:85`/`:386`/`:461`/`:1008`/`:1052`/`:1075`). ⚠️ **The law's count of "three comments + one" was an undercount** — but the count was never the claim; **"zero code" was, and zero code is exactly right.** ⛔⛔ **CLAUSE 2 — *"⛔ ZERO code anywhere computes a destination on the deck"* — ⛔ REFUTED. SEE THE STRUCK BOX BELOW.** | ⚖️⭐ **SPLIT VERDICT — the AI half is a ⛔ DESIGN GAP (`K-2` STANDS, `CONTACT-§6` binds, ⛔ nothing proposed); the PLAYER-COMMANDED half is ⛔ NOT A GAP — the path EXISTS TODAY and is reachable with a mouse.** |

> ### ⛔⛔⛔ **STRUCK 2026-09-01 — `C-3`'s SECOND CLAUSE AND ITS CONCLUSION WERE ⛔ FALSE. MEASURED AND REFUTED BY TASK-775, AND STRUCK ON THE EVIDENCE.**
>
> **THE STRUCK TEXT, QUOTED VERBATIM SO THE REPAIR IS AUDITABLE — ⛔ both the premise and the conclusion go:**
> - ~~*"⛔ ZERO code anywhere computes a destination on the deck. Every unit move is `MoveToActor`/`MoveToLocation` at a **ground-level** goal…"*~~
> - ~~*"⛔ **no amount of repairing the link would ever have shown a symptom.**"*~~ — and the board's phrasing of the same sentence, ~~*"⛔ no runtime test would ever have shown a symptom"*~~.
>
> ⭐⭐ **THERE IS EXACTLY ONE CHAIN THAT PUTS A UNIT'S DESTINATION ON THE DECK, IT IS REACHABLE TODAY, AND A PLAYER CAN FIRE IT WITH A MOUSE.** Every link measured at source by TASK-775 and ✅ **re-verified independently by the manager before this strike was written** (`SiegePlayerController.cpp:3931-3936`, `Building.cpp:47`, `SiegePlayerController.cpp:3256-3282` all read directly):
>
> | # | Link | What it does |
> |---|---|---|
> | 1 | `SiegePlayerController.cpp:3936` | `TraceCursorToGround` = `GetHitResultUnderCursor(**ECC_Visibility**, …)` |
> | 2 | ⭐ `Building.cpp:47` | `VisualMesh->SetCollisionProfileName(**BlockAll**)` ⇒ ⛔ **the tower blocks `ECC_Visibility`**, and `AClimbableTower : ABuilding` inherits it (its ctor touches collision ⛔ nowhere) |
> | 3 | TASK-737 hull **07** | the deck slab's +Z face **IS z = 1200**, X/Y ∈ [−300,+300] ⇒ **a cursor on the deck returns `ImpactPoint.Z ≈ 1200`** |
> | 4 | `SiegePlayerController.cpp:2937-2940` → `:3094` → `:3213` | `GroupPickLocation = Hit.ImpactPoint` → `NewGroup.PositionCenter`. ⛔ **No nav-projection and no Z clamp on the centre** |
> | 5 | `SiegePlayerController.cpp:3260-3281` | per-unit sunflower `Station`, then `ProjectPointToNavigation(Station, …, FVector(_, _, **200**))` — a **±200 uu Z extent**, which reaches the deck poly comfortably from a z-1200 click |
> | 6 | `SummonedUnit.cpp:1941-1948` → `:2901-2902` | HOLD tier 3 → `EnterAdvanceToLocation` → `MoveToLocation(…, **bProjectDestinationToNavigation = true**, …, bAllowPartialPath = true)` |
>
> ⇒ ⭐⭐ **A player can circle a group, point the HOLD position ring at the tower deck, and confirm — and every member is handed a destination ON THE DECK POLY.** A path to a deck-poly goal has **exactly one** legal route: the ladder link. ⚠️ **`Ambush` reaches the same `PositionCenter` via `AcquireEnemyNearPoint` (`SummonedUnit.cpp:1919`); `Follow` does not.**
>
> ### ⚖️ **WHY THE CONCLUSION HAD TO GO TOO, AND ⛔ NOT JUST THE PREMISE** — ⭐ **a player *could* have ordered a unit up there and seen it fail.** ⇒ **a runtime test WOULD have shown a symptom**, so the sentence that excused not running one is ⛔ false. ⚖️ *This was the MANAGER'S framing and the manager's error, and it is struck on TASK-775's evidence rather than on anyone's say-so. The failure was mechanical and worth naming: `C-3` was reasoned from a **grep**, and the grep was of the wrong noun (`ClimbableTower` **references**) for the question (**deck destinations**). `C-1` and `C-2` were read at the source and both survived intact — ⛔ the row that was inferred is the row that broke.*
>
> ✅ **WHAT DOES ⛔ NOT CHANGE, STATED SO THE STRIKE IS ⛔ NOT OVER-READ:** `C-1` · `C-2` · the animation waiver · **and `K-2`'s ruling, which is correct ⛔ ON ITS MERITS and ⛔ not merely on its premise** — ⛔ **nothing in the autonomous AI, bot, cards or intents ever *chooses* the deck** (every autonomous goal is an enemy pawn, a castle, an own castle, a heal target or a stuck-sidestep — all ground). ⇒ ⛔⛔ **`CONTACT-§6` STILL BINDS IN FULL: ⛔ no garrison order, ⛔ no `tower` `where` symbol, ⛔ no intent, ⛔ no bot rule, ⛔ no card behaviour.** ⭐ **The strike widens what we know; it ⛔ does not widen what we build.**

**⇒ ⚖️ THE THREE-WAY RESTATEMENT OF HIS ONE SENTENCE — ⛔ this is what the batch actually builds:**
1. ⭐ **A NEW MECHANISM** — the contact trigger (`CONTACT-§4`), because `C-2` says the shipped one cannot serve a player.
2. ⭐ **A NEW CONSUMER** — `AHeroCharacter` learns to climb (`CONTACT-§2`/`§3`), because `C-1` says it never could.
3. ⚠️ **A MEASUREMENT, ⛔ NOT A REPAIR** — `C-3`. ⛔ **Whatever it finds does ⛔ NOT get "fixed" by inventing a garrison order nobody asked for** (`CONTACT-§6`).

### CONTACT-§2 ⚖️⭐⭐ THE ARCHITECTURE RULING — **A ***STATICS LIFT*** + A ***PER-CLASS DRIVER***. ⛔ NOT A COMPONENT, ⛔ NOT A SHARED BASE CLASS.**

> ### ⚖️ **RULING: the rules move to `SiegeLadderClimbStatics.{h,cpp}`; each pawn class keeps its OWN driver and wires its OWN exits.** ⭐⭐ **AND THE REASON IT IS CHEAP IS A MEASUREMENT, ⛔ NOT A PREFERENCE: THE REFACTOR IS ALREADY ~80% DONE AND NOBODY NOTICED.**

**`FSiegeLadderClimbState` (a POD) and `FSiegeLadderClimbStatics` (`CanBegin` · `Begin` · `ClimbDirection` · `ArrivalTarget` · `ShouldSweep` · `Advance` · `End` · `IsAttackAllowed` · `WantsActorTick`) ALREADY EXIST as a pure, class-free rules module** — read at `SummonedUnit.cpp:136-300` **as of decomposition**. ⛔ **They are merely declared in the WRONG HEADER.** ⇒ **the "shared climb machinery" this ask needs is ⛔ not authored; it is MOVED.**
✅⭐ **LANDED 2026-09-01 (TASK-776). ⇒ 📌 THE CITE ABOVE IS ⛔ HISTORICAL: the module now lives in `Source/GitClaudeUnrealTest/Siegebound/SiegeLadderClimbStatics.{h,cpp}` and `SummonedUnit.cpp` shrank 4,339 → 4,165 lines (−174).** ⛔ **Every `SummonedUnit.cpp` cite written before that commit is stale — see `CONTACT-§10`.**

| Option | ⚖️ Verdict | Why — stated so it is ⛔ not re-proposed as "simpler" |
|---|---|---|
| ⭐ **STATICS LIFT + per-class driver** | ✅⭐⭐ **RULED** | The rules are already pure and already tested headlessly (14 tests, ⛔ not one `SpawnActor` — `SiegeLadderClimbTest.cpp:39`). **Lifting them is a byte-identical move with ⛔ ZERO behaviour change**, it is the shipped `FSiegeStuckStatics` / `FSiegeCombatStatics` idiom **for the third time**, and it leaves `ASummonedUnit`'s shipped, QA-passed driver **behaviourally untouched.** |
| ⛔ **`UActorComponent` (`USiegeLadderClimberComponent`)** | ⛔ **REFUSED** | ⭐⭐ **IT SOLVES THE WRONG HALF.** A component can own the *drive loop* — but ⛔ **it cannot intercept `HandleDeath`, `FreezeAI`, `ApplyFreeze`, `UnPossessed`, `EndRecall` or `EndPlay`**; the owner must call in. ⇒ **the exits — which ARE the risk surface (`TOWER-§8.5`) — stay per-class either way, and the component buys nothing while forcing a rewrite of the project's hottest file.** ⚠️ **And that rewrite would re-plumb a feature Jonathan shipped 3 commits ago for ⛔ zero behaviour change** — the purest form of manufactured regression risk. |
| ⛔ **A shared base class** | ⛔⛔ **REFUSED OUTRIGHT** | Re-parenting `AHeroCharacter` or `ASummonedUnit` re-parents **`BP_HeroCharacter` and the entire shipped unit fleet's Blueprints.** ⚠️ **This project has a recorded, memory-level lesson that a reparented Blueprint can break at RUNTIME while looking perfect at design time.** ⛔ **Not for a 3-second traversal.** |
| ⛔ **Copy-paste the driver onto the hero** | ⛔ **REFUSED** | ⭐ **Two divergent copies of an eight-exit `MOVE_Flying` state machine is exactly how one of them silently loses an exit** — and the failure mode is `TOWER-§8.5`'s: a pawn hanging in the air forever. |

- ⛔⛔ **THE LIFT IS A ***PURE MOVE***. ⛔ NO LOGIC MAY CHANGE IN THE SAME TASK.** ✅ **Renames of the file/include are permitted; ⛔ renaming a function, changing a signature, "tidying" an expression, or altering a default is ⛔ NOT.** ⚖️ *A refactor that also fixes something is a refactor nobody can review.* **The 14 existing tests must pass byte-identically, and that is the lift's own acceptance criterion.**
- ⭐ **`ASummonedUnit` keeps `FSiegeLadderClimbState LadderClimb` as its own member.** ⛔ **There is ⛔ no shared registry, ⛔ no subsystem and ⛔ no singleton** — each pawn owns its own state, exactly as today.

### CONTACT-§3 ⛔⛔⛔ THE HERO'S SAFETY PROPERTIES — **EVERY ONE MUST SURVIVE FOR THE HERO, AND ⛔ NOT ONE OF THEM IS INHERITED BY DEFAULT. THIS IS THE BATCH'S REAL COST.**

> ### ⚠️⚠️ **`MOVE_Flying` IGNORES GRAVITY. AN EXIT THAT FAILS TO RESTORE THE MOVEMENT MODE HANGS THE PAWN IN MID-AIR ***FOREVER*** — and for the HERO that means the PLAYER'S OWN BODY, ⛔ not a replaceable unit.** ⚖️ *The unit version of this bug costs 30 gold. The hero version ends the match.*

#### CONTACT-§3.1 ⛔ THE HERO'S EXITS ARE ***TEN***, AND THEY ARE ⛔ NOT `TOWER-§8.5`'s EIGHT — **ENUMERATE THEM FRESH, ⛔ NEVER MAP THEM ACROSS**

⛔ **Each exit restores the movement mode, clears the state, kills the driver and the watchdog, and broadcasts EXACTLY ONCE. The handoff names all ten and says which test covers each** (`RECALL-§4`'s enumeration duty, third application):

| # | Exit | ⚠️ What is different from the unit's version |
|---|---|---|
| **H-1** | **Arrival** at the far endpoint | — (same shape) |
| **H-2** | **Explicit abort** (`AbortLadderClimb`) | — |
| **H-3** | ⭐ **The player RELEASES the climb input** (`CONTACT-§4` hold-to-climb) | ⛔ **A unit has no such exit.** ⭐ This is the hero's *most frequent* exit and therefore the one that must be bulletproof first (`TOWER-§10 L-4`'s reasoning, re-applied). |
| **H-4** | **Hero death** — `AHeroCharacter::HandleDeath` | ⭐ **PARTIALLY covered ALREADY, and say so rather than double-fixing:** `HeroCharacter.cpp:774` already calls `DisableMovement()` ⇒ the *mode* hazard is met. ⛔ **BUT the climb STATE, the driver tick and the watchdog timer are ⛔ NOT** — a dead, hidden hero still being driven along a ladder line is its own defect. **The abort goes in `HandleDeath` beside `EndRecall(...)` at `:759`, and it is idempotent with H-5.** |
| **H-5** | ⚠️⚠️ **THE GHOST HAND-OFF — `UnPossessed()`** | ⛔⛔ **THE NASTIEST ONE, AND IT IS A ⛔ SEPARATE SEAM FROM H-4.** A hero that dies mid-climb is **un-possessed while in `MOVE_Flying`** and a `ASiegeGhostPawn` takes the controller. ⭐ **`AHeroCharacter::UnPossessed()` is overridden as an INDEPENDENT BELT** — the `SummonedUnit::EndPlay` "independent belt for exit 8" idiom (⭐ **re-measured post-TASK-776: `SummonedUnit.cpp:608-615`, the call itself at `:615`; ⛔ was `:782-786`**), second application. ⚖️ *A possession change for **any** reason must end a climb, and enumerating the reasons is how you miss one.* |
| **H-6** | **Respawn** — `ResetHero()` | ⭐ **ALREADY writes `MOVE_Walking` (`HeroCharacter.cpp:818` — the project's ONLY `SetMovementMode` before this batch).** ⚠️ **But ⛔ do ⛔ NOT lean on it: it is 180 s downstream of the death (`GHOST-§`), so a hero rescued only here has been broken for three minutes.** ⛔ H-4/H-5 are the real exits; this is a backstop. |
| **H-7** | ⛔ **The RECALL channel** | See `CONTACT-§3.4` — a **two-way** rule, ⛔ not one. |
| **H-8** | **Match end** | ⛔ **The hero has ⛔ NO `bAIFrozen`.** See `CONTACT-§3.2` — the equivalent must be **found and named**, ⛔ never assumed. |
| **H-9** | **`EndPlay`** | — |
| **H-10** | ⭐ **THE TOWER DIES MID-CLIMB** (`TOWER-§10 L-5`) | ⚠️⚠️ **`AClimbableTower::EndPlay` aborts every climber it started — but `ActiveClimber` is typed `TWeakObjectPtr<ASummonedUnit>` (`ClimbableTower.h:487`).** ⇒ ⛔⛔ **AS TYPED TODAY, A HERO CLIMBER IS INVISIBLE TO IT AND WOULD ⛔ HANG IN THE AIR WHEN THE TOWER FALLS.** **The occupancy slot MUST be widened** (`CONTACT-§4.4`). |

#### CONTACT-§3.2 ⚠️⚠️ **`CanBegin`'s FLAGS ARE ⛔ UNIT VOCABULARY. THEY ARE ***MAPPED***, ⛔ NEVER PASSED THROUGH.**

`FSiegeLadderClimbStatics::CanBegin(State, bDead, bAIFrozen, bSpellFrozen, …)` (⭐ **re-measured post-TASK-776: `SiegeLadderClimbStatics.cpp:24`; ⛔ was `SummonedUnit.cpp:136` — the cite moved ⛔ FILE, ⛔ not merely line**). ⛔ **`AHeroCharacter` has ⛔ NO `bAIFrozen` and ⛔ NO `bSpellFrozen`.** ⇒ ⛔ **the hero's task MUST name, at file:line, which hero state it passes for each argument, and a mapping it cannot find is a ⛔ DECLARED GAP in the handoff, ⛔ never a `false` typed in to make it compile.** ⚠️ **A hero that can start a climb during the end screen is `GHOST-§4`'s input-dead catastrophe with a new cause.**

#### CONTACT-§3.3 ⛔⛔ THE FOUR RE-DERIVATIONS — **⛔ NEVER INHERIT A NUMBER FROM THE UNIT. ⛔ NOT ONE.**

> ### ⚠️⚠️⚠️ **RE-DERIVATION 4 CAN ***VOID THE FEATURE***. MEASURE IT ⛔ FIRST (TASK-775), ⛔ NOT AT A GATE.**

1. ⭐ **THE ENDPOINT LIFT** — `TOWER-§8.5a` clause 6: both endpoints are lifted from SURFACE space into CAPSULE-CENTRE space by **`GetScaledCapsuleHalfHeight()` READ FROM THE CAPSULE**, ⛔ never a literal. ⇒ **read from the HERO's capsule.** ⛔ **`SiegeSpawn::DefaultCapsuleHalfHeight` (88) is a ⛔ FALLBACK CONSTANT, ⛔ not the hero's dimension** — verified at `SiegeSpawnConstants.h:9`, whose own comment says *"UE default character capsule half-height fallback."*
2. ⭐ **THE DECK-BREACH WINDOW** — `TOWER-§8.5a` clause 2: **≤ 3 × the capsule half-height, measured in Z, converted to line length by the line's OWN slope** (⛔ never a hardcoded `sin 76°`). ⇒ **re-derived from the hero's half-height.** ✅ **The formula is unchanged and the ruling survives a larger capsule by its own terms** — ⭐ **but the resulting Z-ceiling and its % of the 1,236.9 uu line MUST BE REPORTED as numbers**, ⛔ not asserted to be fine.
3. ⭐ **THE WATCHDOG BUDGET** — derived from the (re-lifted) line length and `LadderClimbSpeedUU`, ⛔ never copied.
4. ⛔⛔⛔ **THE `≥56 uu` STANDOFF — AND THIS IS THE ONE THAT KILLED THE HERO HALF. ⛔ MEASURED 2026-09-01. ⛔ IT FAILS.** **`TOWER-§8.5a` is licensed by `TOWER-§8.3`'s ≥56 uu standoff and by ⛔ NOTHING ELSE**, and its own voiding box says so.

   > #### ⛔⛔ **STRUCK — THE PARENTHETICAL *"(⛔ the project never calls `InitCapsuleSize`, and `SiegeSpawnConstants.h` pins only the half-height)"* AND THE INFERENCE THAT THE HERO IS THEREFORE `r 34` WERE ⛔ FALSE.**
   > **STRUCK TEXT:** ~~*"…AGAINST A CAPSULE OF RADIUS 34, which is the engine-default `ACharacter` radius (⛔ the project never calls `InitCapsuleSize`…)."*~~
   > ⛔ **It IS called — on the hero's own direct base:** `GitClaudeUnrealTestCharacter.cpp:18` `InitCapsuleSize(42.f, 96.0f)`. **Full evidence, corroborations and the two-different-34s root cause: `TOWER-§8.3`'s struck box.** ⭐ **`SiegeSpawnConstants.h:9`'s 88 remains correctly described as a FALLBACK — that half of the sentence was right.**

   ⇒ ⭐⭐ **THE FORMULA `59.624 − (R_hero − 34)` SURVIVES INTACT — and TASK-775's measurement puts the hero through it:**

   | Quantity | Value |
   |---|---|
   | `R_hero` / `HalfHeight_hero` — **MEASURED**, ⛔ not defaulted | **42.0 / 96.0** |
   | Excess over the swept radius | `42.0 − 34 =` **8.0** |
   | ⛔⛔ **Residual standoff** | ⛔⛔ **51.624 uu** *(⭐ **EXACT, ⛔ not an upper bound — see `TOWER-§8.5a`'s spine derivation**)* |
   | Required | **≥ 56.0** |
   | ⛔ **Deficit** | ⛔⛔ **4.376 uu SHORT** |
   | Max radius that would have passed | **37.624** — the hero exceeds it by 4.376 |

   - ⇒ ⛔⛔⛔ **`TOWER-§8.5a` IS ⛔ VOID FOR THE HERO AND `§8.5`'s OUTRIGHT REFUSAL APPLIES IN FULL. THIS IS A ⛔ HARD GATE ON TASK-778 AND IT IS ⛔ JONATHAN'S WORD** (`CONTACT-§7` `K-1`, now carrying a **costed options table**).
   - ⭐ **Clause 2's window formula does survive the larger capsule by its own terms** (`3 × 96 = 288 uu` of Z ⇒ `288 / 0.9701425 = 296.863 uu` of line = **24.00%**, against the unit's 264 ⇒ 272.126 ⇒ 22.00%) — ⛔ **but that is MOOT while `§8.5a` is void, and a surviving sub-clause must ⛔ not be presented as a surviving exception.**
   - ⚖️ **STILL BINDING: ⛔ do ⛔ NOT re-author the mesh (`CONTACT-§0` fence) and ⛔ do ⛔ NOT ***quietly*** shrink the hero's capsule** — it is a combat and collision dimension whose value is **pixel-load-bearing** at `Castle.cpp:188`. ⭐ **"Quietly" is the operative word: Jonathan may rule any option in `K-1`'s table; ⛔ no agent may pick one.**
   - ⚠️ **INCIDENTAL, FLAGGED ⛔ NOT TASKED (TASK-775, out of scope, ⛔ no task owns it):** `SiegeSpawn::DefaultCapsuleHalfHeight = 88` is used as a **spawn-placement** half-height at `Barracks.cpp:125` and `SiegePlayerController.cpp:4179`. **The hero's real half-height is 96** ⇒ that fallback is **8 uu wrong for one specific pawn**. ⛔ **Nobody chased whether either site can ever place the hero. Named because the constant calls itself a fallback and this diagnosis just proved the fallback is wrong.**

#### CONTACT-§3.4 ⛔ RECALL × CLIMB — **A ⛔ TWO-WAY RULE, AND BOTH DIRECTIONS SHIP**

- ⛔ **A hero may ⛔ NOT START a climb while a recall channel is running.** (`CanBegin`'s hero mapping, `CONTACT-§3.2`.)
- ⛔ **A climb may ⛔ NOT SURVIVE a recall teleport** — the channel completing teleports the hero home, and a live `MOVE_Flying` drive would fight it and then strand it. ⇒ **the recall completion is exit `H-7`.**
- ✅⭐⭐ **THE THIRD DIRECTION — ⛔ NO LONGER OPEN. ⛔ MEASURED AND ⚖️ RESOLVED 2026-09-01 (TASK-775 Q4), AND THE ANSWER IS ⛔ NEITHER OF THE TWO CANDIDATES THE LAW OFFERED: IT READS ⭐ POSITION.**
  - **The site:** `AHeroCharacter::TickRecall` → **`HeroCharacter.cpp:1282`** calls `FSiegeRecallStatics::HasLeftAnchor(RecallState, GetActorLocation(), RecallMoveCancelToleranceUU)` ⇒ `EndRecall(ESiegeRecallExit::CancelledByMovement)`.
  - **The predicate:** **`HeroCharacter.cpp:1155-1168`** — `FVector::DistSquared(State.AnchorLocation, CurrentLocation) > Safe²`, with its own comment: *"**FULL 3D distance, deliberately, not the horizontal projection**: falling off the ledge the player was standing on IS leaving the spot."* ✅ **Manager-verified at source, ⛔ not on relay.**
  - **The anchor** is captured at `Begin(NowSeconds, GetActorLocation())` (`:1101-1112`, called `:1242`); tolerance **`RecallMoveCancelToleranceUU = 25.f`** (`HeroCharacter.h:896`); driven from `Tick` (`:163-165`) with `bCanEverTick = true` (`:61`) ⇒ ⛔ **unconditional every frame, ⛔ not input-gated and ⛔ not movement-mode-gated.**
  - ⇒ ⭐⭐ **A SCRIPTED `MOVE_Flying` CLIMB CANCELS A RUNNING RECALL — ⛔ IMMEDIATELY.** A climb moves the actor 1,236.9 uu; `GetActorLocation()` leaves a 25 uu ball within **~0.07 s** at `LadderClimbSpeedUU = 350`. ⛔ **The cancel cannot be evaded by the drive being scripted, because the check never asks HOW the pawn moved — only WHERE IT IS.**
  - ⚖️⭐ **RECORD IT AS A ⛔ RESOLVED INTERACTION, ⛔ NOT A CONFLICT.** This clause was written fearing that two shipped features both say *"movement"* and mean different things by it. ⛔ **They do not.** **A position check is strictly the MOST INCLUSIVE of the three candidates** — it catches input-driven walking, physics-driven falling **and** scripted interpolation alike. ⇒ **bullet 2 above is ALREADY HALF-SERVED by shipped code: the channel dies the instant the climb starts, so the teleport never arrives to fight the drive.**
  - ⚠️⚠️ **BUT ⛔ DO ⛔ NOT DELETE `H-7` ON THE STRENGTH OF THIS.** It becomes **near-unreachable in practice** rather than impossible — a completion and a climb-start landing on the same frame, ahead of the cancel, still reaches it. ⚖️ ***An exit you cannot reach is free; an exit you removed is a hang.***
  - ⛔ **AND BULLET 1 IS ⛔ UNCHANGED AND STILL OWED BY TASK-778:** *"a hero may ⛔ not START a climb while a recall channel is running"* is `CanBegin`'s hero mapping and is ⛔ **provided by nothing above.**
- ⛔⛔ **`AS-§6` A-2 IS UNTOUCHED. The climb adds ⛔ NO `Escape` handler of any kind.** Automatic QA fail.

#### CONTACT-§3.5 ⭐ THE WATCHDOG — **THE HERO HAS ⛔ NO 0.25 s POLL, SO IT ⛔ ARMS ITS OWN. MEASURED, ⛔ NOT ASSUMED.**

- ⭐⭐ **MEASURED:** `ASummonedUnit`'s self-healing driver (TASK-760) rides **`StateTimerHandle`**, a looping 0.25 s `FTimerManager` entry that **always** runs. **Grepped `HeroCharacter.cpp` for every `SetTimer`: there are exactly TWO — `RallyCooldownTimerHandle` (`:623`, ONE-SHOT) and `WarBannerAuraTimerHandle` (`:976`, looping but armed ⛔ ONLY while the War Banner upgrade is owned, and PAUSED by `HandleDeath`).** ⇒ ⛔⛔ **THERE IS ⛔ NO UNCONDITIONAL HERO POLL. A HERO DRIVER MODELLED ON THE UNIT'S WOULD SHIP WITH ⛔ NO WATCHDOG AT ALL — and it would look identical in review.**
- ✅ **RULING: the hero arms ONE dedicated looping handle, `LadderClimbWatchdogTimerHandle`, at the same 0.25 s cadence, ⛔ ONLY for the duration of a climb, cleared by ⛔ every one of the ten exits.** ⭐ **One timer, armed for ~3.5 s, ⛔ never a permanent poll added to the hero for a rare feature.**
- ⭐ **AND IT IS STRUCTURALLY SAFER THAN THE UNIT'S, WHICH IS WORTH SAYING:** an `FTimerManager` entry lives on the **world** and ⛔ **cannot be killed by an actor tick-flag write** — so the hazard class TASK-760 had to patch (*"the watchdog rides the very driver it watches"*) ⛔ **does not exist here by construction.** ⛔ **Do ⛔ not "harmonise" the hero onto the unit's tick-flag shape.**
- ⛔ **`TOWER-§8.5a` clause 5 BINDS: a timed-out or aborted climb is ⛔ NEVER handed the deck it failed to reach.** It drops from where it is.

### CONTACT-§4 ⚖️ THE MECHANISM HE ASKED FOR — **PROXIMITY + INTENT + DWELL. ⛔ AND IT IS ⛔ NOT LITERALLY "CONTACT", FOR A MEASURED REASON.**

> ### ⚠️⚠️⭐⭐ **READ THIS FIRST OR YOU WILL BUILD THE WRONG THING: ⛔ THERE IS ⛔ NOTHING TO WALK *AGAINST*. THE LADDER HAS ⛔ ZERO COLLISION.**
> **MEASURED, and it is in our own record — TASK-737's handoff, mirrored on the board: *"⭐ Ladder has ZERO hulls by design; no collision anywhere over `LadderFoot` (nearest surface 116 uu)."*** ⇒ ⛔ **A pawn walking at the ladder passes straight THROUGH it** and only stops ~300 uu later at the solid tower body. ⇒ ⛔⛔ **AN OVERLAP-ON-BLOCKING-HIT TRIGGER, OR ANY DESIGN THAT WAITS FOR THE PAWN TO BE STOPPED BY THE LADDER, WOULD ⛔ NEVER FIRE.** ⚖️ *His words describe the player's INTENT perfectly and the world's GEOMETRY not at all — and the law's job is to say which is which.*

#### CONTACT-§4.1 ⛔ THE PREDICATE — **THREE TERMS, ⛔ ALL REQUIRED, AND IT IS A ***PURE STATIC***

**`FSiegeLadderContactStatics::WantsToClimb(...)` — a pure free function on plain data, ⛔ no world, ⛔ no actor, ⛔ no component.** ⭐ **The `FSiegeLadderClimbStatics` / `FSiegeStuckStatics` precedent, and it is what makes this whole feature headlessly testable in a suite with ⛔ not one `SpawnActor`.**

| Term | Rule | Why this and ⛔ not the obvious thing |
|---|---|---|
| **PROXIMITY** | Pawn within **`LadderContactRadiusUU` (default 150 uu)** of the endpoint, measured **2D (XY)** | ⭐ **The `WR-§5` idiom, applied a third time: THE RADIUS AND THE TEST BOTH LIVE ON THE OWNING ACTOR** (`ACommanderNpc::IsPlayerInRange` reading `InteractRadius`, `CommanderNpc.h:295`) — ⇒ **`AClimbableTower` owns both; the pawn only ASKS.** ⛔ **2D because the endpoint is a SURFACE point and the pawn's origin is its capsule CENTRE** — a 3D test would silently need the `§8.5a` lift to even be satisfiable. |
| **INTENT** | The pawn's horizontal **movement direction** has `dot ≥ LadderContactIntentCos` (**default 0.5, i.e. a 60° half-cone**) with the horizontal direction from the pawn to the endpoint | ⛔ **"Walking toward it", ⛔ not "brushing past".** ⭐ **It reads DIRECTION, ⛔ never a key**, which is the ONLY shape that works identically for a player-driven pawn and an AI-driven one — ⭐ **and `RECALL-§2`/`KBD-§` would forbid a hardcoded key anyway.** |
| ⭐⭐ **DWELL** | The other two terms must hold **CONTINUOUSLY** for **`LadderContactDwellSeconds` (default 0.35 s)** | ⭐⭐ **THIS IS THE WHOLE ANSWER TO "ACCIDENTAL ABDUCTION", AND IT IS LOAD-BEARING FOR THE ***UNIT*** HALF.** ⛔ **Without it, a friendly unit marching past its own tower toward the enemy castle clips the cone for two frames and is yanked 1,200 uu into the air.** ✅ **A pawn merely passing through leaves the cone almost immediately; a pawn deliberately pressing into the ladder holds it trivially.** ⚖️ *Dwell is the cheapest honest translation of "walking **against** it" into a world where there is nothing to press against.* |

#### CONTACT-§4.2 ⚖️ THE FIVE BEHAVIOUR RULINGS — proceeding defaults, ⛔ none blocking

| Row | Question | ⚖️ **Ruling** | Why |
|---|---|---|---|
| **K-A** | Auto-start, or a key press? | ✅⭐ **AUTO. ⛔ No key, ⛔ no prompt, ⛔ no UI** | ⛔ **His words: *"simply by walking against it."*** ⭐ A confirm key is a second mechanism he explicitly did not ask for. |
| **K-B** | How does the player STOP? | ✅⭐⭐ **HOLD-TO-CLIMB: the climb runs while the pawn keeps supplying input INTO the ladder; releasing (or steering away) ends it and the pawn DROPS** | ⭐⭐ **The exact inverse of the start condition, which makes it self-documenting: the verb that starts it is the verb that sustains it.** ✅ **Dropping is FREE — ⛔ no fall damage anywhere in Siegebound** (`TOWER-§4a`, measured). ⛔ **A climb the player cannot abandon is `TOWER-§10 L-4`'s refused shape.** |
| **K-C** | ⚠️ **Descend, and the ⛔ INSTANT-RE-ASCENT LOOP** | ✅ **BOTH ENDPOINTS ARM THE TRIGGER, resolved by **Z** exactly as `TOWER-§8.5a` clause 1 does** — walk into the ladder at the deck ⇒ descend. ⛔⛔ **AND A RE-ARM LATCH IS ⛔ MANDATORY** | ⚠️⚠️ **THE DEFECT THIS PRE-EMPTS IS CERTAIN, ⛔ NOT HYPOTHETICAL: a pawn that finishes a DESCENT is standing at `LadderFoot`, inside the radius, still holding the input that brought it there ⇒ ⛔ it re-climbs INSTANTLY and the player is stuck in a yo-yo.** ⇒ ⛔ **after any climb ends, that endpoint stays DISARMED for that pawn until it either leaves the radius or drops out of the intent cone.** ⭐ Stepping off the deck edge also stays legal and free (`TOWER-§8.7`). |
| **K-D** | Two pawns at once? | ✅ **`TOWER-§10 L-1` STANDS, UNCHANGED AND UN-RE-LITIGATED: ONE AT A TIME.** The second waits in its ordinary walking state | ⛔ **`T-3`'s no-capacity ruling is ⛔ not re-opened.** ⭐ **The mechanism already ships — the tower's single `ActiveClimber` slot — and it needs exactly one change: `CONTACT-§4.4`'s widening.** |
| **K-E** | Does the NAV LINK survive alongside contact? | ✅⭐⭐ **YES — ⛔ BOTH SHIP, AND KEEPING BOTH IS THE POINT** | ⭐⭐ **THEY ANSWER DIFFERENT QUESTIONS AND ⛔ NEITHER SUBSTITUTES FOR THE OTHER: the LINK is how the AI *PLANS A ROUTE* through the ladder** (⛔ delete it and the deck is an unreachable island again — `TOWER-§8.1` M-2) **· CONTACT is how a *BODY STARTS CLIMBING*.** ⛔ **They cannot double-fire: `BeginLadderClimb` returns false when already climbing, and the tower's single occupancy slot refuses the second entrant.** |

#### CONTACT-§4.3 ⛔ THE TEAM GATE IS ⛔ NOT WEAKENED — **`CanTeamAscend` GAINS A SECOND CALLER, ⛔ NOT AN EXCEPTION**

⛔⛔ **`TOWER-§8.6`'s `T-3` STANDS: enemies may ⛔ NOT ascend.** ⇒ **the contact path runs `AClimbableTower::CanTeamAscend(TowerTeam, ClimberTeam)` — the SAME predicate, at the SAME semantics, with its signature still FROZEN.** ⚠️⚠️ **A contact trigger that skipped it would be a ⛔ SILENT BACK DOOR AROUND A JONATHAN RULING, opened by a task whose stated purpose was something else entirely — and the reviewer would have no reason to look.** ⛔ **Stated here so the gate has something to check against.**
⚠️ **AND THE HERO IS AN `ITeamAgent`-BEARING PAWN LIKE ANY OTHER** ⇒ ⛔ **an ENEMY hero may ⛔ not climb your tower either.** ⭐ **Same rule, ⛔ no hero exemption.**

#### CONTACT-§4.4 ⛔⛔ THE OCCUPANCY SLOT MUST BE WIDENED — **A MEASURED, ⛔ NOT SPECULATIVE, DEFECT**

**`ClimbableTower.h:487` declares `TWeakObjectPtr<ASummonedUnit> ActiveClimber`, and `ReleaseClimber(ASummonedUnit*)` / `HandleLadderClimbEnded(ASummonedUnit*, bool)` are typed to match.** ⇒ ⛔⛔ **AS SHIPPED, A HERO CLIMBER CANNOT BE TRACKED AT ALL — which breaks `L-1` (two climbers on one line, and `TOWER-§10 L-1` says they WILL interpenetrate) AND breaks exit `H-10` (the tower falls and the hero hangs).**
✅ **DEFECT CONFIRMED AT SOURCE 2026-09-01 (TASK-775, and ✅ manager-re-verified before ruling):** `ClimbableTower.h:487` `TWeakObjectPtr<ASummonedUnit> ActiveClimber` · `:448` `HandleLadderClimbEnded(ASummonedUnit*, bool)` · `:451` `ReleaseClimber(ASummonedUnit*)` · `ClimbableTower.cpp:382` the busy test `ActiveClimber.IsValid()` · `:432` the assignment · `:168` `EndPlay`'s abort. ⇒ ⛔⛔ **BOTH consequences confirmed exactly as stated: a hero climber never occupies the slot (so `L-1` cannot see it and a unit may be admitted onto the same line — and `ClimbableTower.cpp:221-223` records why that is not cosmetic: *"Two capsules interpolated along one line WILL interpenetrate — `bUseRVOAvoidance` is false … depenetration would shove one OFF the climb line, **in mid-air**"*), and it is invisible to `EndPlay` ⇒ the tower falls and the hero hangs in `MOVE_Flying` forever.**

> ### ⚖️⭐⭐ **THE AMENDMENT, ⛔ RULED 2026-09-01 SO TASK-777 IMPLEMENTS RATHER THAN RE-PROPOSES.** TASK-775 proposed `APawn*` and ⭐ **correctly flagged `ACharacter*` as the tighter option, correctly declined to make the change (that file is TASK-777's), and correctly left the choice to this ruling.**
>
> ### ⚖️ **RULED: ⭐⭐ `ACharacter`, ⛔ NOT `APawn`.** ⛔ **This SUPERSEDES the provisional `TWeakObjectPtr<APawn>` written in this section on 2026-09-01 — and it supersedes it on ⭐ THIS SECTION'S OWN STATED PRINCIPLE, *"the narrowest type that admits both."***
>
> | | |
> |---|---|
> | **Does `ACharacter` admit both?** | ✅ **Yes.** `ASummonedUnit : ACharacter` · `AHeroCharacter : AGitClaudeUnrealTestCharacter : ACharacter` (`HeroCharacter.h:319`, `GitClaudeUnrealTestCharacter.h:22`). **It is their nearest common ancestor.** |
> | ⭐⭐ **AND IT IS ⛔ NOT MERELY NARROWER — IT IS THE TYPE THAT ***CARRIES THE API THE FEATURE REQUIRES***, WHICH IS THE ARGUMENT THAT DECIDES IT** | **A climb needs `GetCharacterMovement()` (for `MOVE_Flying`) and `GetCapsuleComponent()` (for `§8.5a` clause 6's `GetScaledCapsuleHalfHeight()`). ⛔ BOTH ARE `ACharacter` MEMBERS AND ⛔ NEITHER EXISTS ON `APawn`.** |
> | ⛔ **Why `APawn` is actively worse, ⛔ not merely looser** | ⛔⛔ **`APawn` would promise more than the feature can deliver: a bare `APawn` climber is IMPOSSIBLE — it has no movement mode to set and no capsule to lift from.** ⇒ every consumer would immediately `Cast<ACharacter>` anyway, ⭐⭐ **re-introducing the exact failed-cast class (`ClimbableTower.cpp:377`'s `Cast<ASummonedUnit>`) that CAUSED this defect.** ⚖️ *Widening a type until the failure moves is not fixing it.* |
> | ⛔ Not `AActor`, ⛔ not a second slot | ⛔ **UNCHANGED from this section's original ruling.** ⚖️ *Two slots is two `L-1` rules, and the second one is the one nobody tests.* |
> | ⚠️ The ghost | `ASiegeGhostPawn : APawn` is ⛔ **not** an `ACharacter` ⇒ ⭐ **`ACharacter` also makes "a ghost can never climb" true BY TYPE**, which is the `GHOST-§` behaviour we want and would have had to assert separately under `APawn`. ✅ **A free property, named so it is not lost.** |
>
> #### 📌 **THE EXACT SHAPE TASK-777 IMPLEMENTS — ⛔ no re-proposal, ⛔ no re-derivation:**
> ```cpp
> // SummonedUnit.h:100 — ⛔ THE DELEGATE TYPE NAME IS **UNCHANGED**.
> DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSiegeLadderClimbEnded, ACharacter*, Climber, bool, bReachedTop);
> ```
> - ⛔⛔ **THE TYPE NAME STAYS `FSiegeLadderClimbEnded`.** ⚠️ **TASK-775's proposal wrote `FOnLadderClimbEnded`, which would have renamed the TYPE as well as widening it — ⛔ that is a second, unrequested divergence and it is ⛔ REFUSED.** ✅ **Only the first parameter's TYPE changes.**
> - ✅ **The parameter NAME `Unit` → `Climber`** (it is wrong for a hero). ⭐ **Safe by this file's own precedent: parameter names are ⛔ NOT part of a function's type** — established in `TOWER-§8.4(B)`'s `FromWorld`/`ToWorld` amendment and ⛔ not re-argued.
> - ✅ **`ActiveClimber` → `TWeakObjectPtr<ACharacter>`; `ReleaseClimber(ACharacter*)`; `HandleLadderClimbEnded(ACharacter*, bool)`.**
> - ✅ **`SummonedUnit.cpp:3781`'s `OnLadderClimbEnded.Broadcast(this, bReachedTop)` compiles ⛔ UNCHANGED** — `ASummonedUnit*` converts implicitly. ⭐ **Zero edits in the broadcast path.**
>
> #### ⛔⛔ **THE CAPABILITY SEAM — RULED HERE TOO, BECAUSE THE TYPE ALONE DOES ⛔ NOT SOLVE IT AND AN UNRULED SEAM IS A RE-PROPOSAL WAITING TO HAPPEN.**
> `EndPlay`'s abort (`ClimbableTower.cpp:168`) must call **`AbortLadderClimb()` on the climber**, and that is ⛔ **not** an `ACharacter` member. ⇒ ✅ **RULED: a minimal `UINTERFACE`, `ILadderClimber` / `ULadderClimber`, in a NEW pair `Source/GitClaudeUnrealTest/Siegebound/LadderClimber.h`** — ⭐ **the shipped project pattern exactly** (`TeamId.h` → `ITeamAgent`, `HealthBarProvider.h` → `IHealthBarProvider`, both `UINTERFACE(MinimalAPI, NotBlueprintable)`). **Surface: `AbortLadderClimb()` + `IsClimbing() const`, ⛔ nothing else.** Implemented by `ASummonedUnit` (TASK-777 may add the one base-list entry) and by `AHeroCharacter` (TASK-778).
> - ⛔⛔ **IT MAY ⛔ NOT GO IN `SiegeLadderClimbStatics.h` — AND THAT IS ⛔ MEASURED, ⛔ NOT STYLE:** that header states at `:10` *"⛔ No .generated.h: nothing in this pair is reflected, **deliberately**"*. **A `UINTERFACE` would force a `.generated.h` into a deliberately-unreflected pair and silently spend a property TASK-776 was told to preserve byte-for-byte.**
> - ⛔ **A pair of `Cast<ASummonedUnit>` / `Cast<AHeroCharacter>` branches is ⛔ REFUSED** — it hardcodes the class list and is the same failed-cast shape this whole amendment exists to remove.
>
> #### ⚠️⚠️ **THE BLAST RADIUS, MEASURED — AND IT CROSSES A FILE FENCE, WHICH THE BOARD MUST RESOLVE RATHER THAN THE IMPLEMENTER:**
> **`Tests/SiegeLadderClimbTest.cpp:907-908` asserts `UnitParam->PropertyClass == ASummonedUnit::StaticClass()`.** ⇒ ⛔⛔ **THIS AMENDMENT MAKES THAT ASSERTION ***RED***. It is ⛔ not a comment refresh — it is a real edit to a file TASK-776 owns.** ✅ **Resolved on the board by a NARROW, NAMED grant to TASK-777** (TASK-776 has already landed, so ⛔ no parallelism is lost). **Also touched: `:894`'s message text, and `Tests/SiegeClimbableTowerTest.cpp:1061`/`:1067` (TASK-777's own file).**

### CONTACT-§5 ⚖️⚖️ **THE ANIMATION IS WAIVED — BY JONATHAN, EXPLICITLY. ⛔ HIS CALL, AND IT IS HONOURED WITHOUT RE-ARGUMENT.**

> ### ✅ **HIS WORDS, VERBATIM: *"You do not need to add a climbing animation."*** ⛔ **THIS IS A RULING. IT IS ⛔ NOT A DEFAULT, ⛔ NOT NEGOTIABLE BY ANY AGENT, AND ⛔ NOT RE-OPENED.**

- ✅⭐ **WHAT IT BUYS, AND IT IS THE WHOLE REASON THIS BATCH IS SMALL: the wave is ⛔ COMPLETELY UN-BLOCKED FROM `A_SiegeBiped_Climb` AND `ABP_Footman`.** ⛔ **No retarget from `SK_Footman_Skeleton` to the hero's mannequin skeleton · ⛔ no hero ABP state machine · ⛔ no art lane · ⛔ no `SC-§35` anim-owner surface.** ⭐ **The hero climbs playing whatever it already plays.**
- ⚠️ **THE NOTE HE IS OWED, STATED ONCE AND ⛔ NOT RE-ARGUED (`CONTACT-§7` row `K-3`):** `TOWER-§8.1` made the rig **required** on the manager's own reasoning that *"a unit gliding up a ladder bolt upright reads as BROKEN."* ⛔⛔ **THAT WAS MY REASONING, HE HAS OVERRULED IT, AND IT IS HIS CALL.** ⚖️ *Recording the overruled argument beside the ruling is how a decision stays reviewable; re-litigating it is how a manager wastes his time.*
- ✅⭐⭐ **AND THE COST OF CHANGING HIS MIND LATER IS ⛔ NEARLY ZERO, WHICH IS WHY THE WAIVER IS SAFE: `A_SiegeBiped_Climb` IS ALREADY IMPORTED AND COMMITTED** (60 fps, on `SK_Footman_Skeleton`, re-exported onto the `−22.0 uu` rung plane by TASK-733b). ⇒ **wiring it later is a wiring task, ⛔ not a production one.**
- ⛔⛔ **DO ⛔ NOT DELETE IT, ⛔ NOT DEPRECATE IT, AND ⛔ NOT UNWIRE `ABP_Footman`.** ⭐⭐ **THE WAIVER IS A LICENCE ⛔ NOT TO BUILD — IT IS ⛔ NOT AN INSTRUCTION TO REMOVE.** ✅ **UNITS KEEP THEIR SHIPPED CLIMB ANIMATION.** ⚖️ *Deleting working, committed, paid-for content because a later ask did not require it is destroying value and calling it tidiness.*

### CONTACT-§6 ⛔⛔ WHAT THE DIAGNOSIS MAY ⛔ NOT TURN INTO — **THE ANTI-SCOPE-CREEP CLAUSE, WRITTEN ***BEFORE*** THE EVIDENCE ARRIVES SO IT CANNOT BE BENT AFTERWARDS**

⭐⭐ **If TASK-775 proves `C-3` — that ⛔ nothing in the game ever orders a unit onto the deck — the correct response is ⛔ NOT to invent one.**
- ⛔ **⛔ NO garrison order. ⛔ NO "tower" `where` symbol in the assistant grammar** (⚠️ that would spend `MARK-§1`'s hard-won **ZERO** Zone-A characters and break the **5658** byte-freeze). ⛔ **NO new intent, ⛔ no new card behaviour, ⛔ no bot rule.**
- ✅ ⇒ **IT IS RECORDED AS A ⛔ DECLARED DESIGN GAP + A FOR-JONATHAN ROW (`K-2`), and the CONTACT trigger is what serves his actual sentence in the meantime** — a unit walked to the ladder foot climbs it, which is exactly what *"any units can climb the ladder by walking against it"* asks for.
- ⚖️ *An unrequested feature justified by a measurement is still an unrequested feature, and this batch already has enough real work in it.*

### CONTACT-§7 📌 THE FOR-JONATHAN ROWS THIS NAMESPACE OWES — **⭐ every one has a proceeding default that SHIPS, so each is a one-word overrule, ⛔ ⛔ NONE blocking**

| Row | Question | Proceeding default |
|---|---|---|
| ✅⚖️⭐ **K-1 — ⛔ CLOSED. ⚖️ RULED BY JONATHAN 2026-09-02: ⭐ OPTION A — MOVE THE LADDER.** | ⛔⛔ **THE FINDING THAT FIRED IT, KEPT VERBATIM SO THE RULING STAYS REVIEWABLE: `R_hero = 42.0` ⇒ residual standoff **51.624** vs required **≥ 56** ⇒ SHORT BY **4.376 uu** ⇒ `TOWER-§8.5a` WAS ⛔ VOID FOR THE HERO.** ⭐⭐ **He was asked *HOW*, ⛔ never *WHETHER* — he had already ruled the hero climbs.** | ⚖️⭐⭐ **RULED: `A`. The ladder TRANSLATES away from the tower body; the hero is ⛔ NOT shrunk, the sweep is ⛔ NOT excused, and the feature is ⛔ NOT descoped.** ⇒ ⭐ **the deficit is repaired ⛔ IN THE GEOMETRY, so `TOWER-§8.5a`'s licence is ⛔ RE-EARNED rather than waived — see `CONTACT-§7a`'s ruling banner for what that costs and what it buys.** |
| **K-2** | Nothing orders a unit onto the deck (`C-3`). Should the AI ever *choose* to garrison a tower? | ⛔ **NO — ⛔ not built, ⛔ not designed.** Declared gap (`CONTACT-§6`) |
| **K-3** | ⭐ The climb ships **unanimated for the hero** on his waiver. The rig argument is recorded as overruled (`CONTACT-§5`) | ✅ **Ships unanimated. Units keep their clip.** ⛔ Not re-argued |
| **K-4** | Is a climbing HERO also **disarmed** (`TOWER-§9`, which was ruled about UNITS)? | ✅ **YES, disarmed** — ⭐ the `RECALL-§ R-5` state-term-at-a-shipped-guard-point idiom, ⛔ never a new suppression mechanism. ⚠️ **Flagged because it is an EXTENSION of his ruling, ⛔ not his ruling** |
| **K-5** | `LadderContactRadiusUU` ~~**150**~~ ⇒ ~~**300**~~ ⇒ ⭐ **350** *(amended TWICE: `K-6`, then ⭐ **`K-6.1`** — ⛔ read `CONTACT-§7b` before citing any radius)* · `LadderContactIntentCos` **0.5 (60°)** · `LadderContactDwellSeconds` **0.35** | ✅ **All three ship as `EditDefaultsOnly` floats with their consequence written beside them** (`HIGH-§1`). ⚠️ **INVENTED NUMBERS, DECLARED AS SUCH** — feel is his |
| ⛔⛔ **K-6** ⭐ *NEW 2026-09-02 — ⛔ THE ROW THAT KEEPS THE FEATURE FROM SHIPPING INERT* — ⚠️⚠️ **SUPERSEDED SAME DAY BY ⭐ `CONTACT-§7b` (`K-6.1`): the ruling SURVIVES, ⛔ its ARITHMETIC DID NOT, and the shipped number is now ⭐ 350. ⛔ Do ⛔ not cite this cell's `300` or its `±60 → ±125` as live.** | ⛔⛔ **MEASURED: at `LadderContactRadiusUU = 150` the trigger is ⛔ UNREACHABLE BY THE HERO — ⛔ AT ANY CADENCE.** A pawn walking straight at the endpoint is inside the disc for `R / v` seconds; the hero's `WalkSpeed = 500` / `SprintSpeed = 750` (`HeroCharacter.h:736`/`:740`) give **0.300 s / 0.200 s** against a **0.35 s** dwell ⇒ ⛔ **it can ⛔ never satisfy the dwell.** ⚠️ **`K-5`'s numbers were derived against `v = 300` — the SLOWEST unit — and ⛔ never against the pawn the feature was asked for.** | ✅⭐ **PROCEEDING DEFAULT (MINE, DECLARED): `LadderContactRadiusUU` **150 → 300**.** ⛔ **Dwell and cone ⛔ UNCHANGED.** ⇒ sprinting hero **0.400 s** · walking hero **0.600** · Footman(400) **0.750** · Archer/Wizard(350) **0.857** · Longbowman(300) **1.000** — ✅ **all clear 0.35.** ⚠️ **THE COST, MEASURED ⛔ NOT GLOSSED: the abduction window scales with `R` (~0.4·R), so it widens ⛔ ±60 → ±125 uu.** 🧑 **One word retunes any of it — the feel is still yours** |

#### ⭐⭐ CONTACT-§7b — **`K-6.1`: THE RADIUS GOES `300 → 350`, AND THE ⛔ TWO CORRECTIONS TO `K-6`'s ORIGINAL BASIS** *(added 2026-09-02; bought by TASK-778 finding (2) + TASK-784 + TASK-786 — ⭐ every number below was re-derived by the manager before it was written here)*

> ### ⚖️⭐⭐ **`K-6`'s RULING SURVIVES. ⛔ `K-6`'s ARITHMETIC DID ⛔ NOT. ⭐ BOTH HALVES ARE RECORDED, BECAUSE A RULING THAT KEEPS A REFUTED BASIS IS A RULING NOBODY CAN AUDIT.**

**(1) ⛔⛔ THE SHIPPED NUMBER IS ⭐ `LadderContactRadiusUU = 350.f`. `300` ADMITS ⛔ THREE OF THE HERO'S ⛔ FOUR SPEEDS.**

⭐⭐ **FOR THE HERO THE MODEL IS ⛔ EXACT, ⛔ NOT PROBABILISTIC — and that is a MEASURED structural difference, ⛔ not a modelling choice:** the hero's poll is `AHeroCharacter::Tick`, which is **unconditional** (`bCanEverTick` `HeroCharacter.cpp:61`, ticked `:163-165`), so dwell accrues with `DeltaSeconds` and there is ⛔ **no sampling luck at all.** ⚠️ *The unit's 0.25 s `StateTimerHandle` cadence is what made TASK-784's roster analysis a PROBABILITY; ⛔ do ⛔ not carry that framing onto the hero.*

| Hero speed | uu/s | `R/v` at **300** | verdict | `R/v` at ⭐ **350** | verdict |
|---|---|---|---|---|---|
| walk | 500 | 0.600 s | ✅ | 0.700 s | ✅ |
| walk + Swift Boots | 625 | 0.480 s | ✅ | 0.560 s | ✅ |
| sprint | 750 | 0.400 s | ✅ | 0.467 s | ✅ |
| ⛔⛔ **sprint + Swift Boots** | **937.5** | ⛔ **0.320 s** | ⛔ **FAILS AT ⛔ ANY LATERAL OFFSET** | ✅ **0.373 s** | ✅ |

- ⇒ **MINIMUM radius for the hero's full shipped range = `937.5 × 0.35` = ⭐ `328.125 uu`.** ✅ **`350` clears it by `21.875` uu = ⭐ 6.6 %.** ⛔ **`300` is ⛔ short by 28.125 uu and the failure is ⛔ TOTAL for that speed — ⛔ not intermittent, ⛔ not offset-dependent, ⛔ not recoverable by standing still** (a moving pawn that never accrues 0.35 s in the disc ⛔ never triggers, at ⛔ every offset including dead-centre).
- ⚠️⚠️ **AND IT IS THE ⛔ WORST KIND OF DEFECT TO SHIP: a player who sprints with the movement upgrade — the ⭐ most-invested player — is the ⛔ ONLY one who cannot climb.** ⚖️ *The feature would work in every test, for every reviewer, and fail for the person who earned the boots.*
- ⭐ **VERIFY IT, ⛔ DO NOT TRUST IT:** hero speeds are `HeroCharacter.h:999` (`500`) / `:1003` (`750`) — ⚠️ **`K-6`'s original `:736`/`:740` cite is ⛔ STALE** (`CONTACT-§10.1`; correction from TASK-786). The Swift Boots multiplier is `1.25`.
- ⛔ **THE UNIT ROSTER IS ⛔ UNAFFECTED BY THE BUMP** — every unit was already 16/16 at 300 (TASK-786's table); 350 only widens their margin. ⇒ ⭐ **this amendment is bought ⛔ entirely by the hero and costs the units ⛔ nothing but grab-window width (below).**

**(2) ⛔ THE COST, ⛔ RE-MEASURED RATHER THAN RE-ASSERTED — ⛔ AND `K-6`'s OWN COST FIGURE WAS UNDERSTATED ~3×.**

- ⛔⛔ **`K-6` wrote *"the abduction window scales with `R` (~0.4·R), so it widens ±60 → ±125 uu"*. ⛔ THAT MODEL IS REFUTED.** ⭐ **MEASURED (TASK-786's independent closed form, ⭐ agreeing with TASK-784's simulated ±62 → ±204): the true widening at `v = 300` is `±57.8 → ±202.1 uu` — a ⛔ 3.5× widening for a 2× radius.** ⚖️ **The reason is structural and it is why ⛔ no linear model can be right: the dwell only ever eats a ⛔ FIXED `v × 0.35` uu of chord out of a disc whose chord grows with `R`.** ⇒ ⛔ **any `0.4·R` model understates the grab window at ⛔ every radius, and it gets worse as `R` rises.**
- ⛔⛔ **I DID ⛔ NOT PUBLISH A PREDICTED WINDOW FOR `350`** — ⚖️ *`K-6` published one, it was wrong by 3×, and it was wrong because it was a reconstruction shipped as a measurement.* ✅⭐ **TASK-786 MEASURED IT INSTEAD, and this is the ⛔ measured record: the abduction half-window at `v = 300` runs ±57.8 (`R150`) → ±202.1 (`R300`) → ⭐ **±247.2 uu (`R350`)**.** ⚠️ **⛔ A linear scale from ±204 would have guessed ±238 and understated it by ~9 uu — ⛔ super-linear at ⛔ EVERY step, ⛔ not just the doubling.** ⇒ **TASK-780's PIE row — *a unit marching PAST its own tower is ⛔ not abducted* — is what confirms it on pixels, and it is ⛔ more load-bearing at 350 than it was at 300.**
- ⭐⭐ **FRAME-RATE ROBUSTNESS, MEASURED BY TASK-786 UNPROMPTED AND ⛔ NOT PART OF ANY SPEC — ⛔ AND IT DECLARES A REAL FLOOR:** the hero's per-frame accrual holds at **60 fps** (22 samples vs 21 needed) · **30 fps** (11 vs 11) · **20 fps** (7 vs 7), and ⛔ **FAILS at 15 fps.** ⇒ ⚠️ **`350` is ⛔ not unconditional — it is correct down to ~20 fps and the sprint+Boots case is the first to go below that.** ⭐ *Declared here rather than discovered in a low-end playtest.*
- 🧑 **IT IS STILL HIS DIAL.** ⛔ `K-5` makes the feel Jonathan's and `K-6.1` does ⛔ not take it back: **one word retunes the radius, the cone or the dwell.** ⭐ This section exists so the retune is made against ⛔ correct arithmetic.

**(3) ⛔⛔ TASK-786's "FREE LEVER" IS ⛔ FREE FOR ***UNITS*** AND IS ⛔ ***NOT*** FREE FOR THE ***HERO***. ⭐ CORRECTED HERE BEFORE IT REACHES HIM AS AN OPTION.**

TASK-786 offered Jonathan `LadderContactDwellSeconds` **0.35 → 0.50** at *"⛔ ZERO cost in radius"*, on the ground that both values sit in the same 2-poll band of the unit's 0.25 s cadence. ✅ **True for units.** ⛔⛔ **FALSE for the hero, and by the same exactness that bought `K-6.1`:** the hero's requirement is the ⛔ raw product `v_max × dwell` ⇒ `937.5 × 0.50` = ⭐ **`468.75 uu`** — ⛔ **133.9 % of the 350 this section just ruled** (and ⛔ `375` even for plain sprint). ⇒ ⚖️ **the dwell and the radius are ⛔ COUPLED for the hero and ⛔ decoupled for the unit.** ⭐ **THE REAL HEADROOM AT 350 IS `0.35 → 0.373 s` (`350 / 937.5`), ⛔ not 0.50.** ⛔ **If he takes the dwell lever past that, the radius rises with it or the boots stop working again.**

✅⭐⭐ **AND THE CREDIT IS OWED, BECAUSE IT IS THE HABIT THIS FILE WANTS: TASK-786 ⛔ RETRACTED ITS OWN LEVER, ⛔ unprompted, the moment the hero's model reached it** — ⛔ it did ⛔ not defend the earlier figure and ⛔ did not leave it standing for a manager to catch. ⚖️ *A finding withdrawn by its own author costs one paragraph; the same finding reaching Jonathan as an option costs a retune he cannot undo.*

**(4) ⭐⭐ AND THE CORRECTION TO `K-6`'s ORIGINAL PREMISE, WHICH IS THE ⛔ MORE IMPORTANT HALF OF THIS SECTION — `K-6` NAMED THE ⛔ WRONG DEFECT AND STILL REACHED THE ⛔ RIGHT RULING.**

- ⛔⛔ **`K-6` asserted *"at `R = 150` the trigger is UNREACHABLE ⛔ AT ANY CADENCE"* and *"⛔ no unit could ever climb"*. ⛔ REFUTED, MEASURED (TASK-784, reproduced independently by TASK-786's `clamp((R/v − P)/P, 0, 1)` — ⭐ two methods agreeing EXACTLY on all six rows).** **Each poll credits a ⛔ FULL 0.25 s of dwell for ⛔ one instant in the cone**, so the real bar is *"⛔ TWO SAMPLES land in the window"*, ⛔ not *"the pawn is in the disc for 0.35 s"*. **Phase counts at `R = 150`: Ogre 16/16 · Knight 16/16 · Archer 11/16 · Footman 8/16 · Sapper 3/16 · ⛔ Cavalry 0/16.**
- ⇒ ⭐⭐ **THE REAL DEFECT AT 150 WAS ⛔ NOT A DEAD FEATURE — IT WAS ⛔ INTERMITTENCY, AND THAT IS ⛔ WORSE.** ⚖️ ***"Sometimes it works" cannot be reported, cannot be reproduced, and trains a playtester to blame himself.*** ⛔ A dead feature gets fixed in one session; a 8/16 feature gets argued about for a month.
- ✅ **THE RULING IS ⛔ UNCHANGED AND ⛔ STRENGTHENED: the radius had to rise.** ⭐ **At 300 all six roster units are 16/16; at 350 they stay 16/16 with more margin.** ⚖️ **Recorded in full because `K-6` is cited downstream by TASK-779 item (5c) as a ⛔ CONFORMANCE check — ⛔ a gate citing a refuted basis is a gate that can be argued out of.**
- ⚠️ **TWO RESIDUALS DECLARED BY TASK-786 AND ⛔ NOT SILENTLY ABSORBED:** ✅ **hero sprint + Swift Boots (937.5, needed `R ≥ 328.1`) is ⛔ FIXED by this amendment** · ⚠️ **Cavalry + Battle Cry (750 uu/s at the unit's 0.25 s cadence) IMPROVES but does ⛔ NOT CLOSE — ⛔ measured `10/16` phases at 300, ⭐ `14/16` at 350.** Full reliability needs `2 × 0.25 × 750` = **375 uu**, which would push the grab window past **±270**. ⇒ ⛔ **declared, ⛔ not fixed, and ⛔ not a blocker**: it is a buffed-unit corner, and the dial is 🧑 his. ⭐ *Named here so it is found, ⛔ not re-discovered.*
- ✅⭐ **TEST 16 IS RECONCILED BY ⛔ NOT TOUCHING IT, AND THE REASON IS THE INTERESTING PART:** `SiegeLadderClimbTest.cpp`'s test 16 ⛔ never reads the shipped radius for any assertion — it computes `2 × Poll × 600` from poll + roster and drives DERIVED radii. ⇒ ⭐ **its `300` is ⛔ still TRUE; it simply answers a ⛔ DIFFERENT question — *"what does the ROSTER need at the shipped poll?"* — and it ⛔ CANNOT SEE the hero's per-frame requirement, which is HIGHER.** ⛔ **Do ⛔ not "fix" test 16 to 350 and ⛔ do not read its 300 as the whole constraint.**

#### ⛔⛔⛔ CONTACT-§7a — **`K-1`'s COSTED OPTIONS TABLE — 🧑 JONATHAN'S CHOICE, ⛔ AND ⛔ NOT ONE AGENT MAY PICK FOR HIM**

> ### ⭐⭐ **HE HAS ALREADY RULED THAT THE HERO SHOULD CLIMB** (*"lets just make sure the playable character and any units can climb the ladder"*). ⇒ ⛔ **THIS TABLE IS FRAMED AS *HOW*, ⛔ NOT AS *WHETHER*.** ⚖️ **Every cost below is MEASURED or explicitly marked as a manager reconstruction. ⛔ Nothing here is guessed, and where I have an opinion I say it is an opinion.**

**THE DEFICIT, restated so every row is costed against the same number: ⛔ 4.376 uu on `TOWER-§8.3`'s own line** (≈ 2.04 uu on the hero's clause-6-lifted driven line — ⛔ **it fails on both, which is why the verdict is robust**).

> ## ⚖️⭐⭐⭐ **RULED 2026-09-02 BY JONATHAN — ⭐ OPTION `A`. THE TABLE BELOW IS ⛔ CLOSED; ⛔ NO ROW IN IT MAY BE RE-PROPOSED BY ANY AGENT.**
>
> ⛔⛔ **`B`, `C`, `D` and `E` ARE ⛔ REFUSED BY HIS WORD AND ⛔ NOT BY ARGUMENT — ⛔ do ⛔ not re-open them, ⛔ not as "simpler", ⛔ not as a fallback if `A` turns out to cost more than the table says.** ⚖️ *If `A` proves impossible the correct move is to go BACK to him with the measurement, ⛔ never to substitute a row he declined.*
>
> ### 📌 **THE MAGNITUDE — ⛔ AND IT IS ⛔ NOT THE `~4.6` WRITTEN IN ROW `A`. THE PINNED TRANSLATION IS ⭐ `δ = 10.0 uu WEST IN X`.**
>
> ⚠️⚠️ **ROW `A`'s OWN WORDS FORBADE SHIPPING ITS NUMBER: *"⛔ my 4.6 is a reconstruction at the single worst contact, ⛔ not an art measurement — it must ⛔ not be shipped as a build target unverified."* ⭐ Honouring that sentence is what produced this one.** **Re-derived here from `TOWER-§8.5a`'s own spine table, ⛔ not relayed:**
>
> | Step | Value |
> |---|---|
> | Required spine→geometry distance for a **hero** capsule: `dist − 42 ≥ 56` | ⇒ **`dist ≥ 98.0 uu`** |
> | Shipped worst contact (plinth top-west edge, hull 00) | **93.6242 uu** ⇒ ⛔ short |
> | Translating the line west by `δ` moves the worst point west by `δ`: `dist(δ) = √((90.5788 + δ)² + 23.6849²)` | — |
> | ⛔ **Break-even** | **`δ = 4.516`** ⇒ `dist = 98.000`, ⭐ **clearance margin EXACTLY ZERO** |
> | ⚠️ **Row `A`'s `4.6`** | `dist = 98.076` ⇒ ⛔⛔ **clears the gate by 0.076 uu.** ⚖️ *That is ⛔ not a build target, it is a coin flip — and it is the ⛔ only reason this banner exists* |
> | ✅⭐ **PINNED: `δ = 10.0`** | `dist = 103.330` ⇒ **hero clearance 61.330** (margin **+5.33**) · **unit clearance 69.330** (margin **+13.33**) |
> | ✅ **And on the hero's clause-6 LIFTED driven line** (centre 8 uu higher in Z) | `√(100.5788² + 31.6849²) = 105.446` ⇒ clearance **63.446** (margin **+7.45**) ⇒ ⭐⭐ **`δ = 10` clears BOTH lines with >5 uu, which is why the choice is robust in the same way the failure was** |
>
> - ✅ **WHAT `δ = 10` COSTS, MEASURED AGAINST ROW `A`'s OWN NAV FIGURES — and it is ⛔ nothing that matters:** `LadderFoot` **−450 → −460**, its clearance of the `X ≤ −364` ground-nav edge **improves 86 → 96 uu** · `LadderTop` **−150 → −160**, still **76 uu inside** the deck poly's surviving `X ∈ [−236, +236]` (was 86). ⭐ **Total structure span 750 → 760 uu, and `TOWER-§7` absorbs it with ⛔ zero spec edits because it takes the footprint FROM THE MESH.**
> - ⛔⛔ **AND THE ACCEPTANCE IS ⛔ NOT THIS ARITHMETIC. IT IS THE ARTIST'S MEASUREMENT OVER THE ***WHOLE LINE***, AGAINST ***EVERY*** HULL** — every number above is a reconstruction against **one** feature, and `TOWER-§8.5a`'s voiding condition demands the whole line. **If the measured worst case lands below `dist ≥ 98.0`, that is a ⛔ STOP and a finding for the manager, ⛔ never a quiet retune of `δ`.**
> - ⭐⭐ **WHY THIS ⛔ CANNOT BE READ AS "THE MANAGER PICKED AN OPTION": ⚖️ Jonathan ruled WHICH LEVER. The magnitude of a lever he chose is an engineering number with a written derivation, and row `A` explicitly ordered it re-derived before use.** ⛔ **`δ` is ⛔ not a feel number and is ⛔ not on the `K-5` sheet.**

| | Option | ⭐ What it actually costs — **measured** | ⚠️ What it re-opens |
|---|---|---|---|
| **A** | ⭐ **MOVE THE LADDER ~4.6 uu FURTHER FROM THE TOWER BODY** — translate the whole climb line **west in X**: `LadderFoot (−450,0,0) → (−454.6,0,0)`, `LadderTop (−150,0,1200) → (−154.6,0,1200)`, ladder geometry with it | ✅⭐ **CHEAPER THAN IT LOOKS, AND I AM SAYING SO EVEN THOUGH IT WEAKENS MY OWN WARNING: a PURE TRANSLATION leaves `Δ = (300,0,1200)` UNCHANGED** ⇒ ⛔ **lean 76.0°, length 1236.93, `sin θ`, the watchdog budget and the deck-breach window % ALL UNCHANGED.** ⭐⭐ **AND THE RUNG PLANE'S −22.0 uu IS MEASURED ***RELATIVE TO THE CLIMB LINE***, so a translation preserves it ⇒ ⛔ `A_SiegeBiped_Climb` needs ⛔ NO re-export and the ⭐ UNITS' SHIPPED CLIP IS SAFE.** ✅ **Nav margins survive: `LadderFoot` 86 → 90.6 uu clear of the X ≤ −364 ground-nav edge; `LadderTop` 86 → 81.4 uu inside the deck poly's X ∈ [−236,+236].** | ⛔⛔ **IT CREATES AN ART LANE WHERE THERE IS NONE — `CONTACT-§0`'s happiest fence.** `SM_WatchTower` re-author + re-export + re-import. ⛔ **RE-ARMS `TOWER-§8.3`'s re-author obligation AND `§8.5a`'s voiding condition** ⇒ **all THREE `§8.3` numbers must be RE-MEASURED over the WHOLE line and re-reported** (⛔ my 4.6 is a reconstruction at the single worst contact, ⛔ not an art measurement — it must ⛔ not be shipped as a build target unverified). ⛔ **Amends a PINNED coordinate table.** ⛔ **BLOCKS TASK-778 and the whole gate on the art task.** |
| **B** | ⭐⭐ **KEEP THE MOVE ***SWEPT*** AND ***IGNORE THE TOWER*** FOR THE CLIMB'S DURATION** (`UPrimitiveComponent::IgnoreActorWhenMoving`) — ⇒ **the non-swept window is ⛔ NOT NEEDED AT ALL, so the standoff gate stops applying** | ⭐ **ALREADY IN OUR OWN RECORD AS A CO-EQUAL REMEDY WE DID NOT TAKE UP: TASK-737 §5 wrote *"drive the last stretch with a non-swept / teleport-style move ⭐ (or disable collision for the ascent)"* — ⛔ only the first branch was ever ruled.** ✅⭐⭐ **IT ANSWERS `§8.5`'s THREE HARMS MORE STRONGLY THAN `§8.5a` DOES: a swept move that ignores ONE actor ⛔ cannot be dragged through ANYTHING — it still sweeps against other units, other buildings and terrain.** ⇒ **the 4.376 uu deficit becomes MOOT rather than tolerated.** ✅⭐ **PRESERVES `CONTACT-§0`'s NO-ART-LANE FENCE ENTIRELY: ⛔ no mesh, ⛔ no socket, ⛔ no re-measure, ⛔ no clip, ⛔ no pinned-table amendment.** | ⚠️ **A ⛔ NEW MECHANISM ⇒ it needs its own law section and its own QA clauses; it is ⛔ NOT covered by `§8.5a` and must ⛔ not be smuggled in under it.** ⚠️⚠️ **A NEW EXIT HAZARD OF EXACTLY THE `MOVE_Flying` CLASS: a hero left ignoring the tower WALKS THROUGH THE TOWER BODY FOREVER** ⇒ **all TEN exits must revert TWO properties instead of one.** ⚠️ **It DIVERGES the hero from the unit** (unit keeps `§8.5a`; hero uses ignore-and-sweep) — ⚖️ *two mechanisms for one traversal is what `CONTACT-§2` refused for the driver.* ⚠️ **UNMEASURED: whether ignoring the tower also defeats the arrival depenetration that puts the hero ON the deck.** |
| **C** | **SHRINK THE HERO CAPSULE** `r 42 → ≤ 37.6` | ✅ **The smallest possible diff — one literal.** | ⛔⛔ **THE LITERAL IS ON THE EPIC TEMPLATE BASE (`GitClaudeUnrealTestCharacter.cpp:18`), ⛔ NOT ON THE HERO** ⇒ it changes the hero's whole collision profile. ⛔⛔ **AND IT MOVES A PIXEL-MEASURED SHIPPED NUMBER: `Castle.cpp:188` `HeroStopLaneX = 3657.5 + 42 = 3699.5`, which TASK-656 measured ON PIXELS (capsule FREE at 3710, BLOCKED at 3690); at r 37.6 the lane becomes 3695.1 and the castle trample path's east leg moves with it.** ⛔ **Desyncs `SiegeGhostPawn.cpp:53`'s deliberately-matched ghost capsule.** ⚖️ **MY OPINION, LABELLED AS OPINION: trading a measured, pixel-verified castle number to win an argument about a ladder is the worst value in this table. ⛔ It is still YOUR call, and `CONTACT-§3.3` #4 refuses only doing it ***quietly***.** |
| **D** | ⛔ **SHIP THE HERO CLIMB WITHOUT THE WINDOW** — it climbs but cannot reach the deck | ⛔ **The sweep jams ~131 uu BELOW the deck** (`§8.5a` row 2, TASK-738's measurement), **silently, with all exits perfectly correct.** | ⛔⛔ **STRICTLY WORSE THAN E: it spends the entire batch and delivers a VISIBLE defect that reads as broken.** ⛔ **Named only so it is not chosen by accident.** |
| **E** | ⛔ **THE HERO DOES NOT CLIMB** | It is `GHOST-§ G-7`'s previously-shipped ruling (*"The HERO ⛔ NEVER CLIMBS"*) — ⛔ **which your 2026-09-01 directive OVERRULED.** ⭐ **Note the half that still ships: the CONTACT trigger serves UNITS, so *"any units can climb the ladder by walking against it"* is still delivered.** | ⛔⛔ **LAST RESORT, ⛔ NOT A DEFAULT — it contradicts your explicit directive.** |

**📌 WHAT IS ⛔ NOT BLOCKED WHILE THIS ROW SITS:** ✅ TASK-776 (landed) · ✅ TASK-777 (contact trigger + occupancy widening — ⭐ **serves the UNIT half of his sentence on its own**) · ✅ the whole `CONTACT-§4` mechanism. ⛔ **ONLY TASK-778's deck-breach window waits.**

### CONTACT-§8 📌 NAMING + FILE MAP (the cross-task contract)

| thing | law |
|---|---|
| the lifted rules | ✅ **LANDED (TASK-776).** **`Source/GitClaudeUnrealTest/Siegebound/SiegeLadderClimbStatics.{h,cpp}`** — holds `FSiegeLadderClimbState` + `FSiegeLadderClimbStatics`, **moved byte-identically** from `SummonedUnit.cpp:136-300` *(the pre-lift cite, ⛔ historical)*. ⭐ The shipped `SiegeStuckStatics` / `SiegeCombatStatics` naming pattern, ⛔ no new convention needed. ⛔⛔ **THIS PAIR IS DELIBERATELY UNREFLECTED (`SiegeLadderClimbStatics.h:10`: *"⛔ No .generated.h"*) — ⛔ nothing reflected may be added to it** |
| ⭐ **the climber interface** ***(NEW — ruled at `CONTACT-§4.4`)*** | **`Source/GitClaudeUnrealTest/Siegebound/LadderClimber.h`** — `UINTERFACE(MinimalAPI, NotBlueprintable)` **`ULadderClimber` / `ILadderClimber`**, surface ⚠️⚠️ **WIDENED 2026-09-02 FROM ⛔ TWO METHODS TO ⭐ FOUR — see `CONTACT-§12`, the ⚖️ LAW AMENDMENT that owns it. ⛔ THIS ROW'S OLD *"and ⛔ nothing else"* IS ⛔ SUPERSEDED, ⛔ not overridden by a task.** ⇒ **`AbortLadderClimb()` · `IsClimbing() const` · ⭐ `BeginLadderClimb(const FVector&, const FVector&) → bool` · ⭐ `GetOnLadderClimbEnded() → FSiegeLadderClimbEnded&`** — ⛔ **and ⛔ nothing else; the surface is CLOSED again at four.** ⭐ **The shipped project interface pattern** (`TeamId.h` → `ITeamAgent`; `HealthBarProvider.h` → `IHealthBarProvider`). **Authored by TASK-777; implemented by `ASummonedUnit` (777) and `AHeroCharacter` (778); ⭐ WIDENED by TASK-787** |
| ⭐ **the delegate's HOME** ***(NEW — `CONTACT-§12`)*** | **`FSiegeLadderClimbEnded` MOVES `SummonedUnit.h:115` → `LadderClimber.h`** (TASK-787). ⛔ **The TYPE NAME and the SIGNATURE are ⛔ UNCHANGED — `(ACharacter*, Climber, bool, bReachedTop)`, `TOWER-§8.4(B)`'s second amendment ⛔ intact.** ⭐ **A MOVE, ⛔ not an amendment: the delegate is CLIMBER vocabulary, which is the same argument `LadderClimber.h:14` already makes about `AbortLadderClimb()`.** ✅ **Free: `SummonedUnit.h:10` and `HeroCharacter.h:8` ⛔ already include `LadderClimber.h`.** ⛔ **`CONTACT-§10.1` FIRES — the mover lists every invalidated `SummonedUnit.h` cite** |
| ⭐ **the widened delegate + slot** | **`FSiegeLadderClimbEnded(ACharacter*, Climber, bool, bReachedTop)`** (`SummonedUnit.h:100`) · **`AClimbableTower::ActiveClimber` → `TWeakObjectPtr<ACharacter>`** · `ReleaseClimber(ACharacter*)` · `HandleLadderClimbEnded(ACharacter*, bool)`. ⛔ **The TYPE NAME `FSiegeLadderClimbEnded` is ⛔ UNCHANGED** (`TOWER-§8.4(B)`'s second amendment) |
| the contact predicate | **`FSiegeLadderContactStatics::WantsToClimb(...)`**, in the **same** new pair. ⛔ **Pure static on plain data — ⛔ no world, ⛔ no actor, ⛔ no component** |
| the tunables | **`LadderContactRadiusUU` · `LadderContactIntentCos` · `LadderContactDwellSeconds`** on **`AClimbableTower`** (`WR-§5`: the radius and the test live on the owning actor) |
| the hero's members | **`LadderClimb` (`FSiegeLadderClimbState`) · `LadderClimbWatchdogTimerHandle`** on `AHeroCharacter` — ⭐ **+ `OnLadderClimbEnded` (`FSiegeLadderClimbEnded`), added by TASK-787 (`CONTACT-§12`).** ⚠️⚠️ **THE CLIMB-RATE CLAUSE WAS ⛔ WRONG AND IS ⛔ REPAIRED HERE, ⛔ NOT LEFT TO ROT (`CONTACT-§10.1`; found by TASK-778 and DECLARED in-source at `HeroCharacter.cpp:2004-2009` rather than quietly worked around — ⭐ that is the behaviour this file wants).** ~~*"`LadderClimbSpeedUU` is READ FROM **THE TOWER's** shipped value"*~~ ⇒ ⛔⛔ **THE TOWER HAS ⛔ NO SUCH VALUE. ✅ MANAGER-VERIFIED AT SOURCE: `LadderClimbSpeedUU` is `protected`, `= 350.f`, on ⭐ `ASummonedUnit` (`SummonedUnit.h:1129`) and appears ⛔ ZERO times in `ClimbableTower.{h,cpp}`.** ⇒ ✅ **THE REPAIRED CLAUSE: `LadderClimbSpeedUU` is the ⛔ ONE shipped rate, it lives on ⭐ `ASummonedUnit`, and the hero ⛔ READS it — ⛔ NEVER a second copy on the hero and ⛔ never a hero-specific default.** ⚖️ *The INTENT — ⛔ never two speed properties, because two is how they drift — was ⛔ always right and is ⛔ unchanged; only the OWNER was mis-named.* ⚖️⭐ **AND THE "CLEAN LANDING" BOTH 778 AND THE SHIPPED COMMENT PROPOSE — *move the property onto `AClimbableTower`* — IS ⛔ RULED ⛔ OUT** (`CONTACT-§12.4`): ⛔ it would make the rate PER-TOWER, which is a design change ⛔ nobody asked for, and it spends 🧑 Jonathan's `TOWER-§9.3` exposure lever. ⇒ ⭐ **the LAW is repaired to match the CODE, ⛔ not the code to match the law** |
| the occupancy slot | **`AClimbableTower::ActiveClimber`, WIDENED** (`CONTACT-§4.4`) — and the `OnLadderClimbEnded` signature change lands as a ⚖️ **LAW AMENDMENT** to `TOWER-§8.4(B)`, ⛔ never silently |
| ⛔ untouched | ⚠️⚠️ **AMENDED 2026-09-02 — ⛔ THIS ROW WAS MADE ***FALSE*** BY JONATHAN'S `K-1` RULING AND IS REPAIRED RATHER THAN LEFT TO ROT (`CONTACT-§10.1`).** ⛔ **`SM_WatchTower` IS ⛔ NO LONGER UNTOUCHED — it is RE-AUTHORED by TASK-783** (the option-`A` translation). ✅ **STILL untouched: `MI_WatchTower_PBR` · `A_SiegeBiped_Climb` · `ABP_Footman` · `T_CardArt_WatchTower` · `Docs/GDD.md` · `Docs/Data/cards.csv` · `SiegeGameMode.{h,cpp}` · `SiegeGhostPawn.{h,cpp}`.** ⭐ The ghost seam is covered from `AHeroCharacter`'s own side (`H-4`/`H-5`), which is what keeps `SiegeGameMode` out of it |
| ⭐ **the ladder translation** ***(NEW — `K-1` = option `A`)*** | **`Content/RawAssets/WatchTower.fbx` + `/Game/Meshes/SM_WatchTower`** — sockets `LadderFoot (−460,0,0)` / `LadderTop (−160,0,1200)`, ladder geometry translated with them, ⛔ **ladder keeps ZERO collision.** ⛔⛔ **AND `CONTACT-§0`'s HAPPIEST CLAUSE — *"⛔ THERE IS ⛔ NO ART LANE IN THIS BATCH"* — IS ⛔ SPENT: HIS RULING CREATED ONE.** ⚖️ *Row `A` warned it would, in those words, before he chose it. ⛔ It was his call and it is honoured without re-argument* |
| ⭐ **the two call sites** ***(NEW — `SC-§36.1`)*** | **UNIT: `ASummonedUnit`, polled from the shipped 0.25 s `StateTimerHandle` (TASK-784) — ⛔ NOT `Tick`, see `CONTACT-§11.5`.** · **HERO: `AHeroCharacter::Tick` (TASK-778), which ⛔ IS unconditional (`HeroCharacter.cpp:163-165`, `bCanEverTick` `:61`).** ⛔ **`AClimbableTower::TryBeginContactClimb` is the ONE entry point for both — ⛔ neither may re-implement the predicate, and ⛔ neither may skip `CanTeamAscend`** |

### CONTACT-§9 📌 M8 DECLARATION

⛔ **NO replicated property and ⛔ NO RPC are AUTHORED in this batch.** ⚠️ **A climb is authority-relevant movement state on a player-possessed pawn** ⇒ **the M8 shape is DECLARED in a header comment and ⛔ NOT built** (`ACC-§8`'s reserved-not-authored discipline; `WR-§8`'s standing warning that *"nothing to declare"* is false here and must ⛔ not be copied from another batch's boilerplate).

### CONTACT-§10 ⛔⛔ **TWO STANDING PIPELINE RULES BOUGHT BY TASK-775 — ⭐ both GENERAL, ⛔ neither specific to this batch. Cite them from anywhere.**

#### CONTACT-§10.1 ⛔⛔ **LINE-CITE ROT — a cite that silently points at the WRONG line is ⛔ WORSE THAN NO CITE AT ALL** *(the `SC-§36` class: a record that names a location and is not re-derived is a ⛔ DEBT, ⛔ not a wiring)*

⭐⭐ **WHAT HAPPENED, MEASURED:** TASK-776's statics lift landed **while TASK-775 was running**. `SummonedUnit.cpp` went **4,339 → 4,165 lines (−174)** ⇒ ⛔ **every `SummonedUnit.cpp` cite in `CONTACT-§1`, `§2`, `§3.1`, `§3.2`, `§8`, in `TOWER-§8.4(B)`/`§8.5a`, and on the board became wrong in one commit** — ⛔ **while still LOOKING authoritative.** ⚖️ *A stale cite does not fail loudly; it sends the next reader to a plausible wrong line and they trust it.*

- ⛔⛔ **AND IT HID A SECOND, WORSE DEFECT — WHICH IS THE REAL ARGUMENT FOR THIS RULE.** `CONTACT-§1` `C-1` cited `BeginLadderClimb`/`AbortLadderClimb`/`IsClimbing` as `:3798`/`:3806`/`:3899`. ⭐ **Re-derivation showed the shift was a uniform −174 — and that the three numbers had been ⛔ MIS-PAIRED WITH THE NAMES ALL ALONG** (3798 was `IsClimbing`, 3806 was `BeginLadderClimb`, 3899 was `AbortLadderClimb`). ⇒ ⛔ **The mis-pairing survived a full law review because ⛔ nobody re-derived the cite. Staleness is what made it visible.**
- ✅ **THE RULE:** ⛔ **a cite is a MEASUREMENT and expires the moment its file is edited.** **Any task that moves ≥ 50 lines in a file, or moves a symbol BETWEEN files, MUST list in its handoff every law/board cite it invalidated, with the new value.** ⭐ **TASK-775 did exactly this unprompted (the mapping is at the top of `handoffs/TASK-775-programmer.md`) and it is the model.**
- ✅ **AND ⛔ NEVER BULK-SHIFT A CITE BY THE LINE DELTA.** ⭐ **A symbol that moved FILE has no valid line offset at all** (`CanBegin` `SummonedUnit.cpp:136` → `SiegeLadderClimbStatics.cpp:24`; `bDeckIsAtEnd` `:194` → `:82`). ⛔ **Re-grep, ⛔ do not arithmetic.**
- ⚠️ **KNOWN, DECLARED, ⛔ NOT REPAIRED:** this file carries **many** `SummonedUnit.cpp` cites from **older, closed batches**. ⛔ **They were ⛔ NOT swept — a blanket renumber of historical records would itself be an unverified bulk-shift, which is the very thing this rule forbids.** ✅ **Repaired here: every cite in the LIVE `CONTACT-§`/`TOWER-§8` law and on the LIVE board.** ⇒ ⭐ **treat any `SummonedUnit.cpp` line cite dated before 2026-09-01 as UNVERIFIED and re-grep it before relying on it.**
- ⚠️ **ONE STALE CITE LIVES IN ⛔ SOURCE, ⛔ NOT LAW, AND IS BOARDED RATHER THAN FIXED HERE:** `Tests/SiegeLadderClimbTest.cpp:85`'s comment repeats the refuted *"the project never calls `InitCapsuleSize`"*. **Boarded as a named comment-only rider on TASK-777** (`CONVENTIONS.md` is manager-only; source comments are not).

#### CONTACT-§10.2 ⚖️ **A ⛔ DIAGNOSIS-ONLY TASK'S TERMINAL STATE IS `done`, ⛔ NEVER `ready-for-qa` — ⭐ because there is ⛔ nothing to review**

⚠️ **TASK-775 set itself `ready-for-qa` per the programmer's standing convention, having written ⛔ ZERO lines of source.** ⛔ **Spawning `qa-reviewer` on it would have reviewed nothing** — and this board's own TASK-779 line already said so (*"TASK-775 is an input, ⛔ not a reviewed diff"*). ⇒ ⚖️ **the two rules disagreed, and a gate that depends on a human noticing the disagreement is ⛔ not a gate.**

- ✅ **RULED:** a task whose `names:` deliverable is **a handoff and nothing else** (⛔ no source, ⛔ no asset, ⛔ no config) goes **`backlog` → `in-progress` → `done`.** ⛔ **It ⛔ NEVER enters `ready-for-qa`, and the orchestrator ⛔ never spawns QA on it alone.**
- ✅ **Its findings are reviewed as an ⭐ INPUT at the batch's real gate**, named in that gate's `inputs:` list — which is exactly what TASK-779 already does.
- ⭐ **THE TELL, so this is checkable rather than remembered: if a task's `names:` line contains ⛔ no file under `Source/`, `Content/` or `Config/`, `ready-for-qa` is ⛔ WRONG for it.**
- ⚖️ *Recorded as law rather than fixed once on the board, because the next diagnose-first task will have the same standing convention and the same instinct.*

### CONTACT-§11 ⚖️ **THE TASK-777 DISPOSITION — the two declared deviations RULED, and the findings that survive the task** (2026-09-02; ⛔ every claim re-verified at source by the manager before it was written here)

#### CONTACT-§11.1 ⚖️ **D-2 — ⛔ RULED ***IN***. `FSiegeLadderContactStatics` STAYS IN `SiegeLadderClimbStatics.{h,cpp}`.**

⚖️⭐⭐ **THE IMPLEMENTER FOLLOWED THE BOARD OVER A LOSSY DISPATCH SUMMARY, AND THAT IS ⛔ NOT A DEVIATION TO FORGIVE — IT IS THE ⭐ CORRECT PRECEDENCE, AND IT IS HEREBY THE RULE.**

- ✅ **THE BOARD NAMES THAT FILE THREE TIMES** — TASK-777 item (1) (*"in TASK-776's new pair"*), item (9)'s fence line, and **`CONTACT-§8`'s file map** (*"the contact predicate … in the **same** new pair"*). ⛔ **The dispatch prompt's compressed fence omitted it. ⇒ THE FENCE WAS WRONG, ⛔ NOT THE PLACEMENT.**
- ✅⭐ **AND THE ONE PROPERTY THAT COULD HAVE MADE IT WRONG IS ⛔ INTACT — ✅ MANAGER-VERIFIED AT SOURCE, ⛔ not taken on the handoff's word:** `SiegeLadderClimbStatics.h:10` still reads *"⛔ No .generated.h: nothing in this pair is reflected, deliberately"*, and the appended block (`:354` banner, `:451` `FSiegeLadderContactStatics`) adds **a plain struct of pure statics + a plain enum** — ⛔ **nothing reflected, ⛔ no `UINTERFACE`, ⛔ no `UCLASS`, ⛔ no `USTRUCT`.** ⇒ **the deliberate no-`.generated.h` property TASK-776 was told to preserve is preserved.**
- ⭐ **`CONTACT-§4.4`'s PROHIBITION IS ⛔ NOT VIOLATED AND WAS ⛔ NEVER ABOUT THIS:** it forbids a **`UINTERFACE`** in that pair, **and its stated reason is that a `UINTERFACE` would FORCE a `.generated.h`.** ⇒ ⛔ **the prohibition is on REFLECTION, ⛔ not on residency.** ✅ **`ILadderClimber` correctly went to its own `LadderClimber.h` (verified: `LadderClimber.h:62`); the contact statics correctly did not need to.**
- ✅ **THE MITIGATION IS ACCEPTED AND IS BETTER THAN COMPLIANCE WOULD HAVE BEEN:** the addition sits **below a loud attribution banner** so TASK-776's moved region stays **contiguous from the top of the file and byte-diffable**, which is exactly what TASK-779 item (2) needs to do its job. ⭐ *An implementer that anticipated the next gate's instrument and protected it.*
- 📌 ⇒ ⭐⭐ **STANDING RULE, GENERAL: ⛔ WHERE A DISPATCH PROMPT AND THE BOARD DISAGREE, THE ⭐ BOARD WINS AND THE IMPLEMENTER SAYS SO IN THE HANDOFF.** ⚖️ **A dispatch is a COURIER for the board, ⛔ never an amendment to it** — only the manager amends a spec (`TASKBOARD.md` write-discipline rule 1). ⛔ **An implementer who follows a lossy prompt over the board has ⛔ not been obedient; it has silently taken an unauthorised spec edit.**

#### CONTACT-§11.2 ✅ **D-1 — ⛔ RULED ***IN***.** The widened delegate's own doc comment (`SummonedUnit.h:85-113`) read *"⛔ SIGNATURE PINNED CHARACTER-FOR-CHARACTER"* **directly above the signature `CONTACT-§4.4` ordered changed.** ⇒ ⭐ **Leaving it would have shipped `CONTACT-§10.1`'s exact defect — a record that looks authoritative and is false — inside the very file the amendment was announced in.** ✅ **Comment-only; ⛔ no code, ⛔ no `UPROPERTY`, ⛔ no other declaration.** ⚖️ *A pin that outlives its pinning is worse than no pin.*
- ✅ **D-3 accepted as reasoned:** `MinContactSpeedUU = 1.f` is a **private `constexpr` degeneracy floor**, ⛔ **not a fourth `K-5` tunable** — ⛔ not `EditDefaultsOnly`, ⛔ not exposed, ⛔ not on Jonathan's sheet.
- ⭐ **D-4 CARRIED FORWARD ONTO THE BOARD, ⛔ NOT LEFT IN A HANDOFF (`SC-§36.1` clause 3):** `ELadderEntryVerdict::NotASummonedUnit`'s **name** is exact today and becomes **wrong** the moment TASK-778 widens the identity term. ⇒ ⛔ **TASK-778 owns the rename**, and it is a numbered item in its spec.

#### CONTACT-§11.3 ⭐⭐ **THE NEAR-MISS THAT IS WORTH MORE THAN THE FEATURE — ⛔ A CONSTANT BEARING WOULD HAVE ABDUCTED EVERY PAWN THAT WALKED PAST**

⭐ **TASK-777 wanted the intent cone measured against the CLIMB LINE's own constant horizontal direction** — which is *"constant, never degenerate, never flips when the pawn overshoots the socket"*, and reads as strictly better engineering. ⛔⛔ **THE ARITHMETIC KILLED IT, AND THE IMPLEMENTER RAN THE ARITHMETIC BEFORE TRUSTING THE INSTINCT.**

- ⛔ **A constant bearing is satisfied for the ⛔ WHOLE CHORD:** `2·√(R² − d²) / v` = ⛔ **0.745 s at `d = 100`, `R = 150`, `v = 300`** — **more than double the 0.35 s dwell** ⇒ ⛔⛔ **every friendly pawn crossing the disc eastward would have been yanked 1,200 uu into the air, and the dwell term — the ⛔ entire answer to accidental abduction — would have been ⛔ decorative.**
- ✅⭐⭐ **THE LAW'S LITERAL TERM (pawn → ENDPOINT) IS RIGHT, AND ⛔ NOW WE KNOW ***WHY***: THE BEARING ⛔ MUST SWING, AND THAT SWING ⭐ ***IS*** THE DISCRIMINATION.** With the swing, in-cone time is `(√(R² − d²) − d/tan 60°) / v` — **0.500 s head-on, 0.180 s at `d = 100`** ⇒ the dwell separates them. ⛔ **Without it there is nothing to separate.** ⇒ 📌 **`CONTACT-§4.1`'s INTENT row is ⛔ NOT a stylistic phrasing and may ⛔ NOT be "simplified" to the line's own direction. It is shipped in `ClimbableTower.h:582-583`'s comment and it is now law.**
- ⚖️ *Recorded because the refuted version is the one a reviewer would have approved: it is more stable, cheaper, and completely wrong.*

#### CONTACT-§11.4 ⭐ **THE TWO OTHER FINDINGS THAT SURVIVE THE TASK — ✅ both verified at source**

- ⭐⭐ **BOTH ENDPOINTS ARM AND ARE RESOLVED BY ⛔ `Z`, ⛔ NEVER 2D — AND IT IS ⛔ ARITHMETIC, ⛔ NOT TASTE** (`SiegeLadderClimbStatics.cpp:209-211`, read directly): the endpoints are **1,200 uu apart in Z but only 300 uu apart in XY**, so **at a 150 uu radius the two XY discs are ⛔ TANGENT** and **a pawn standing on the GROUND can fall inside the DECK endpoint's disc.** ⇒ **a 2D resolution would send a ground pawn "descending" from a deck it is not on.** ✅ **A strict `<` makes a tie read as the FOOT (an ascent), matching `HandleLadderLinkReached`'s shipped tie-break.** ⚠️⚠️ **AND `K-6` MAKES THIS ⛔ SHARPER, ⛔ NOT MOOTER: at `R = 300` the two discs ⛔ OVERLAP HEAVILY rather than merely touching** ⇒ ⛔ **the Z resolution is now load-bearing at every approach, ⛔ not just at the tangent point. ⛔ Do not weaken it.**
- ⭐⭐ **TEST 10(b) ASSERTED ⛔ ONLY ARITY AND WOULD HAVE SLEPT THROUGH THE AMENDMENT** — the widening changed a parameter's **TYPE** while leaving the **count** at two. ✅ **Now asserts the parameter's type** (`Tests/SiegeClimbableTowerTest.cpp:1176`, `ClimberParam->PropertyClass == ACharacter::StaticClass()`). ⇒ 📌 **`SC-§37`'s SHAPE AGAIN, IN A THIRD COSTUME: ⛔ A CHECK THAT SURVIVES THE CHANGE IT EXISTS TO CATCH.** ⚖️ *An arity assertion on a pinned signature is the delegate equivalent of asserting a colour equals its own literal — it can only fail by accident.*

#### CONTACT-§11.5 ⛔⛔⛔ **THE CADENCE TRAP — ⛔ THE UNIT'S ACTOR TICK IS ***DISABLED*** IN EXACTLY THE STATE A CLIMB MUST START FROM. ⛔ MEASURED 2026-09-02.**

> ### ⚠️⚠️ **THIS IS THE SECOND `SC-§36` TRAP IN ONE FEATURE, AND IT IS ⛔ INVISIBLE: A CONTACT POLL WRITTEN INTO `ASummonedUnit::Tick` COMPILES, REVIEWS CLEAN, AND ⛔ NEVER RUNS.**

- ⛔ **MEASURED AT SOURCE:** `ASummonedUnit` sets `PrimaryActorTick.bCanEverTick = true` (`SummonedUnit.cpp:138`) — ⛔ **but `RefreshActorTickEnabled` (`:3614-3622`) is the ⛔ ONLY WRITER of the flag and its expression is `SetActorTickEnabled(FSiegeLadderClimbStatics::WantsActorTick(bLungeActive, LadderClimb.bActive))`.** ⇒ ⛔⛔ **A unit that is neither lunging nor ALREADY CLIMBING has its actor tick ⛔ OFF — which is ⛔ precisely the state every climb begins in.**
- ⛔⛔ **AND THE OBVIOUS REPAIR IS ⛔ REFUSED, FOR THREE INDEPENDENT REASONS: ⛔ DO ⛔ NOT ADD A THIRD TERM TO `WantsActorTick`.** ① its own comment pins the expression (*"⛔ The expression is UNCHANGED — it is still exactly `bLungeActive || LadderClimb.bActive`, in that order, with no new term"*) ② it is a **pinned pure seam** driven by a headless truth table, and a third parameter is `SC-§33`'s hazard on exactly that shape ③ ⛔⛔ **it would restore a permanent per-frame tick to the ENTIRE FLEET — which is what TASK-738/760 spent two tasks removing.**
- ✅⭐ **RULED: the unit's contact poll rides the shipped `StateTimerHandle`** — `SetTimer(…, &ASummonedUnit::UpdateState, StateCheckInterval, bLoop = true)` at **`SummonedUnit.cpp:1037`**, base `StateCheckInterval` = **0.25 s** (`SorcererUnit.h:53`). ⛔ **No new timer, ⛔ no tick-flag term, ⛔ no tower tick.**
- ✅⭐⭐ **AND ITS CLEARS ARE A ⛔ FREE CORRECTNESS PROPERTY, ⛔ NOT A GAP:** `StateTimerHandle` is CLEARED on death, freeze and stand-down (`:598`, `:679`, `:956`, `:990`). ⇒ **a unit with no live handle is dead / frozen / stood-down — ⛔ every one of which `CanBegin` refuses anyway.** ⭐ *The poll cannot fire in a state the rules forbid, by construction rather than by a guard.*
- ⛔⛔ **DECLARED CONSEQUENCE, ⛔ NOT DISCOVERED LATER: `AMinerUnit` SEALS `StateCheckInterval = 0` (`MinerUnit.cpp:62`), and `SetTimer` with 0 does ⛔ not schedule** ⇒ ⛔ **A MINER HAS ⛔ NO `StateTimerHandle` AT ALL AND THEREFORE ⛔ CAN NEVER CONTACT-CLIMB.** ✅ **Ruled ACCEPTABLE and ⛔ DECLARED rather than fixed** — a miner is a non-combat unit with a structural seal this project has twice refused to re-open, and a miner on a watchtower deck serves ⛔ nothing. ⚖️ *Declaring it is the whole difference between a design decision and a bug report six weeks from now.*
- ⚠️⚠️ **THE CADENCE COSTS RESOLUTION, AND `K-6` IS WHAT PAYS FOR IT:** a 0.35 s dwell sampled at 0.25 s completes at **0.50 s** of real dwell. **At the OLD `R = 150` that is fatal for the whole fleet** (in-cone `R/v`: Footman 0.375 · Archer/Wizard 0.429 · Longbowman 0.500) ⇒ ⛔ **⛔ NO UNIT WOULD EVER HAVE CLIMBED.** ✅ **At `K-6`'s `R = 300` every unit clears it** (0.750 / 0.857 / 1.000 vs 0.500). ⇒ ⭐⭐ **ONE LEVER FIXES BOTH THE HERO'S SPEED AND THE UNIT'S CADENCE, which is why `K-6` moves the RADIUS and ⛔ not the dwell.**
- ⚠️ **AND ONE MORE, NAMED SO IT IS ⛔ NOT MISTAKEN FOR A BUG AT PIE: THE PREDICATE READS ⛔ VELOCITY, AND A PAWN ***PRESSED AGAINST GEOMETRY*** HAS NONE.** The ladder is collisionless, so a pawn that keeps walking passes through it and stops against the **tower body** ~150 uu beyond, at which point its velocity → 0 and the INTENT term ⛔ fails. ⇒ ⭐ **the trigger fires during the ⛔ APPROACH, ⛔ never while "leaning on" the ladder** — which is why the in-cone budget is `R / v` and why `R` is the only lever that buys time. ⚖️ *His words describe leaning; the world only offers approaching. `CONTACT-§4`'s opening box said so first — this is the same fact one layer deeper.*
- 📌 ⇒ ⭐⭐ **THE GENERALISABLE RULE, and it is what `K-5` got wrong: ⛔ CONTACT NUMBERS ARE DERIVED AGAINST THE ***FASTEST PAWN ADMITTED*** AND THE ***COARSEST CADENCE THAT POLLS THEM***, ⛔ never against a representative one.** ⚖️ *`K-5` used `v = 300` — the slowest unit in the game — for a feature whose headline consumer sprints at 750.*
- ⚠️ **RIDER 2026-09-02 — this section's `R = 300` figures are ⛔ SUPERSEDED by `CONTACT-§7b` (`K-6.1` ⇒ ⭐ `350`), and its *"⛔ NO UNIT WOULD EVER HAVE CLIMBED"* at 150 is ⛔ REFUTED (`§7b` item 4: the real defect at 150 was ⛔ INTERMITTENCY — Ogre/Knight 16/16 down to Cavalry 0/16).** ⭐ **Its RULING and its two-samples cadence mechanism are ⛔ UNCHANGED and still bind; only the numbers moved.** ⛔ Left as authored with this rider rather than rewritten — `CONTACT-§10.1`'s discipline applied to a section of this file's own.

## ⚖️⭐⭐⭐ CONTACT-§12 — **THE TWO-SEAM WIDENING: ⛔ THE TOWER MAY ⛔ NO LONGER NAME A CONCRETE CLIMBER CLASS** *(added 2026-09-02; bought by TASK-778's declared blocker; ⛔ every claim below re-verified at source by the manager before it was written. ⇒ owned by ⭐ **TASK-787**.)*

> ### ⚖️⭐⭐ **FIRST, THE RULING THAT MATTERS MOST, AND IT IS ⛔ NOT A TECHNICAL ONE: ⭐ TASK-778's REFUSAL TO SELF-START AROUND THE TOWER WAS ⛔ CORRECT, AND IT IS ⭐ RATIFIED AS LAW.**
>
> **TASK-778 shipped a hero climb that is COMPLETE, TESTED, and ⛔ CANNOT FIRE** — `TryBeginContactClimb` → `EvaluateLadderEntry` returns **`NotAnAdmittedClimber`** because the identity term admits `ASummonedUnit` ⛔ ONLY. ✅ **MANAGER-VERIFIED AT SOURCE: the verdict at `ClimbableTower.cpp:267` · the caller's `Cast<ASummonedUnit>(Climber) != nullptr` at ⭐ `:729` · `CastChecked<ASummonedUnit>` at ⭐ `:747` · the hero's poll + its one-shot Warning at `HeroCharacter.cpp:2137-2174`.** ⚠️ *(⛔ **CITE CORRECTION**: the handoff's `:708`/`:726` are ⛔ STALE — the live lines are **729**/**747**. `CONTACT-§10.1`, third instance in this batch.)*
>
> ⭐⭐ **IT COULD HAVE SHIPPED A WORKING-LOOKING HERO CLIMB IN ⛔ ONE LINE, AND IT REFUSED, AND ITS THREE REASONS ARE THE LAW:**
> 1. ⛔ **A self-start SKIPS `CanTeamAscend`** ⇒ a ⛔ **SILENT BACK DOOR around Jonathan's `T-3` ruling, opened by a task whose stated purpose was something else** (`CONTACT-§4.3`). ⚖️ *An agent overturning a user's ruling from inside an unrelated task is the worst failure this pipeline can produce, and it would ⛔ never have shown up in a diff review.*
> 2. ⛔ **It SKIPS the occupancy slot** (`TOWER-§10` `L-1`) ⇒ two capsules on one line, `bUseRVOAvoidance` false, depenetration shoves one OFF the line **in mid-air**.
> 3. ⛔⛔ **It leaves `ActiveClimber` UNSET** ⇒ **exit `H-10` would ⛔ NOT EXIST** ⇒ **`MOVE_Flying` ignores gravity ⇒ when the tower dies the PLAYER'S OWN BODY hangs in the air FOREVER, in a match that keeps running around it.**
>
> ⇒ ⚖️⭐ **RULED: ⛔ A PAWN MAY ⛔ NEVER DRIVE ITSELF ONTO A LADDER. ⭐ THE PAWN ASKS; THE TOWER DECIDES.** ⛔ **A future task that "just adds a Begin call" to any pawn to make a climb work is ⛔ REFUSED BY THIS SECTION, ⛔ not by an argument.** ⭐ **And the honest report — ask, be refused, warn ONCE, board it — is ⛔ the behaviour, ⛔ not the fallback.**

### CONTACT-§12.1 ⛔⛔ **THE WIDENING NEEDS ⛔ TWO SEAMS, AND A ***START-ONLY*** WIDENING IS ⛔ WORSE THAN ⛔ NO WIDENING AT ALL**

> ### ⚠️⚠️⚠️ **THE SENTENCE THAT SIZES THIS WHOLE SECTION: A START-ONLY WIDENING CONVERTS *"the hero cannot climb"* INTO *"the ⛔ FIRST hero attempt ⛔ DISABLES THE TOWER FOR EVERYONE, for the rest of the match."***

- ⛔ **WHY, MEASURED:** the tower learns a climb ended through **⛔ exactly one channel** — `ASummonedUnit::OnLadderClimbEnded` (`SummonedUnit.h:807`), bound at `ClimbableTower.cpp:764`, unbound in `ReleaseClimber` (`:568`). ⭐ **`AHeroCharacter` has ⛔ NO completion delegate and says so in-source** (`HeroCharacter.cpp:1777-1782`: *"⛔ NO COMPLETION DELEGATE IS BROADCAST, AND THAT IS DECLARED RATHER THAN FORGOTTEN"*). ⇒ **a hero admitted to the slot ⛔ NEVER RELEASES IT** ⇒ every later climber, hero or unit, gets `LadderBusy` ⇒ ⛔ **THE LADDER IS BRICKED.**
- ⇒ ⚖️⭐⭐ **RULED: THE TWO SEAMS ARE BOARDED ⛔ TOGETHER, IN ⛔ ONE TASK, WITH THE CALLER (`SC-§36.1`).** ⛔ **Splitting them is ⛔ not a smaller increment — it is a ⛔ REGRESSION with a green suite.** ⭐ *This is `SC-§36.1`'s first application where the two halves are not "surface + caller" but "surface + its INVERSE", and the law reads the same either way: ⛔ a seam that can be entered and not left is ⛔ half a seam.*
- ✅ **AND THE CREDIT IS OWED PLAINLY: TASK-778 identified ⛔ all three edits — identity, start, completion — named (3) as *"the part that makes this a real design change rather than a one-liner"*, and REPORTED it instead of attempting it.** ⚖️ *That is `SC-§36` clause 3 working exactly as written, from the implementer's side, ⛔ unprompted.*

### CONTACT-§12.2 ⚖️ **SEAM 1 — THE START. ⭐ `ILadderClimber` GAINS `BeginLadderClimb`. ⛔ `CONTACT-§8`'s TWO-METHOD PIN IS ⛔ AMENDED, ⛔ NOT VIOLATED.**

```cpp
// ── LadderClimber.h — ILadderClimber ADDITION (TASK-787) ──────────────────
virtual bool BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld) = 0;
```

- ✅⭐⭐ **THE SIGNATURE IS ⛔ NOT INVENTED — IT IS THE ONE ⛔ BOTH IMPLEMENTERS ⛔ ALREADY SHIP, CHARACTER-FOR-CHARACTER.** ✅ **VERIFIED: `ASummonedUnit::BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld) → bool` (`SummonedUnit.h:770`) · `AHeroCharacter::BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld) → bool` (`HeroCharacter.cpp:1585`).** ⇒ ⭐ **⛔ ZERO signature churn on either implementer; ⛔ neither declaration has to change.**
- ✅ **THE `UFUNCTION` QUESTION IS ⛔ ALREADY ANSWERED BY THE SHIPPED BUILD, ⛔ not by reasoning:** `AbortLadderClimb()` (`SummonedUnit.h:781`, `UFUNCTION(BlueprintCallable)`) and `IsClimbing() const` (`:793`, `UFUNCTION(BlueprintPure)`) ⛔ **already satisfy `ILadderClimber`'s ⛔ non-`UFUNCTION` pure virtuals and compile today.** ⇒ ⭐ **`BeginLadderClimb` is the THIRD application of a pattern this very file already proves.** ⛔ **The interface method stays ⛔ non-`UFUNCTION`** — `LadderClimber.h:34-39`'s reason is unchanged: the implementers declare their own specifiers and a reflected interface method would have to agree with them specifier-for-specifier or fail in UHT for a reason no reader would connect to that file.
- ⛔⛔ **AND `CONTACT-§2` IS ⛔ NOT VIOLATED — ⛔ READ WHAT IT ACTUALLY RULED.** `LadderClimber.h:41-44` says `Begin` is absent because *"`CONTACT-§2` ruled that each pawn class keeps its OWN driver, so STARTING a climb is the pawn's business"*. ⚖️⭐ **THAT SENTENCE IS ⛔ REPAIRED, ⛔ not overridden: `CONTACT-§2` refused a ⛔ SHARED DRIVER — the per-frame interpolation and the exits — and it ⛔ never said the tower may not ASK a pawn to start.** ✅ **PROOF FROM THE SHIPPED CODE: the tower has ⛔ ALWAYS been the caller** (`ClimbableTower.cpp:775`, `Unit->BeginLadderClimb(FromWorld, ToWorld)`; and `:499` on the link path). ⇒ ⛔ **the pin removed ⛔ nothing but the tower's ability to reach a SECOND class. The driver stays per-class, exactly as `§2` ruled.**
- ⇒ ✅ **`ClimbableTower.cpp:747`'s `CastChecked<ASummonedUnit>` becomes a `Cast<ILadderClimber>`, and `:729`'s identity term becomes `Cast<ILadderClimber>(Climber) != nullptr`.**

### CONTACT-§12.3 ⚖️⭐⭐ **SEAM 2 — THE COMPLETION, AND IT IS THE ONE THAT BITES. ⭐ THE DELEGATE MOVES TO THE INTERFACE'S HEADER AND IS REACHED THROUGH AN ACCESSOR.**

```cpp
// ── LadderClimber.h (TASK-787) — the delegate MOVES here from SummonedUnit.h:115.
//    ⛔ TYPE NAME + SIGNATURE UNCHANGED (TOWER-§8.4(B)'s second amendment is INTACT).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSiegeLadderClimbEnded, ACharacter*, Climber, bool, bReachedTop);

// ── ILadderClimber ADDITION (TASK-787) ────────────────────────────────────
virtual FSiegeLadderClimbEnded& GetOnLadderClimbEnded() = 0;
```

- ⭐⭐ **WHY AN ACCESSOR AND ⛔ NOT THE DELEGATE ITSELF: ⛔ AN INTERFACE DECLARES ⛔ NO STATE** (`LadderClimber.h:46`, its own M8 clause). ⇒ **each implementer owns the INSTANCE; the interface owns only the way to reach it.** ⭐ **The unit's instance already exists and is ⛔ untouched** (`SummonedUnit.h:806-807`, `UPROPERTY(BlueprintAssignable, Category = "Siegebound|Unit|Climb")`); **the hero adds one, ⭐ mirroring that specifier**, plus **⛔ ONE broadcast line** placed inside its existing teardown.
- ✅⭐ **THE MOVE IS ⛔ FREE, AND THAT WAS ⛔ CHECKED, ⛔ NOT HOPED:** `SummonedUnit.h:10` and `HeroCharacter.h:8` **⛔ already `#include "Siegebound/LadderClimber.h"`** (both implement `ILadderClimber`). ⇒ ⛔ **no new include anywhere, and ⛔ no heavy `SummonedUnit.h` include is dragged into `HeroCharacter.h`** — ⚖️ *which is the reason a forward declaration was ⛔ not good enough: the hero needs the ⛔ COMPLETE type for a by-value `UPROPERTY`.*
- ✅ **`LadderClimber.h` ⛔ ALREADY carries a `.generated.h`** (it is a `UINTERFACE`) ⇒ ⭐ **it can host a `DECLARE_DYNAMIC_MULTICAST_DELEGATE`.** ⛔⛔ **`SiegeLadderClimbStatics.{h,cpp}` ⛔ CANNOT and ⛔ MUST NOT — that pair is deliberately unreflected (`:10`) and `CONTACT-§11.1` just re-affirmed the property.** ⛔ **Putting the delegate there is a ⛔ BLOCKER.**
- ⭐⭐ **AND THE MOVE IS ⛔ CORRECT, ⛔ not merely convenient: the delegate is ⛔ CLIMBER vocabulary, ⛔ not UNIT vocabulary — which is the ⛔ SAME argument `LadderClimber.h:12-15` already makes about `AbortLadderClimb()`.** ⚖️ *Its doc comment already calls itself "AClimbableTower's ONLY completion signal"; it was only ever in `SummonedUnit.h` because the unit was the only climber.*
- ⛔ **`CONTACT-§10.1` FIRES ON THE MOVER.** Removing ~24 lines from `SummonedUnit.h` shifts every cite below it. ⇒ ⛔ **TASK-787 lists ⛔ every invalidated `SummonedUnit.h` cite with its new value** — at minimum: **`:100`** (cited by `CONTACT-§8` and `TOWER-§8.4(B)`) · **`:115`** · **`:755`** · **`:770`** · **`:807`** · **`:1129`** · **`:197`**. ⛔ **Re-grep, ⛔ do ⛔ not arithmetic.**
- ⇒ ✅ **The tower binds/unbinds through `ILadderClimber::GetOnLadderClimbEnded()` and holds ⛔ ZERO concrete climber classes** — ⭐ **which is the property `CONTACT-§4.4` was created to buy and did ⛔ not finish buying.**

### CONTACT-§12.4 ⛔⛔ **THE FIVE SHAPES THAT ARE ⛔ REFUSED — ⭐ RECORDED SO ⛔ NONE IS RE-PROPOSED AS "SIMPLER"**

| ⛔ Refused shape | ⚖️ Why |
|---|---|
| ⛔ **A `Cast<ASummonedUnit>` / `Cast<AHeroCharacter>` BRANCH PAIR anywhere in the tower** | ⛔ **It is the ⛔ EXACT defect `CONTACT-§4.4` was created to remove**, and `LadderClimber.h:22-26` already records the refusal in these words: ⚖️ ***"Widening a type until the failure moves is not fixing it."*** ⛔ **Including a ⛔ SINGLE `Cast<ASummonedUnit>` guarding a unit-only bind** — that is the class list with one entry hidden |
| ⛔ **A SECOND, HERO-ONLY delegate type** (e.g. `FHeroLadderClimbEnded`) | ⛔ **`TOWER-§8.4(B)`'s second amendment pins the TYPE NAME `FSiegeLadderClimbEnded`; a rename to `FOnLadderClimbEnded` was already PROPOSED and ⛔ REFUSED.** ⭐ The signature was widened to `ACharacter*` in TASK-777 ⛔ precisely so ⛔ one type serves both pawns |
| ⛔ **A SECOND occupancy slot beside `ActiveClimber`** | ⛔ **`TOWER-§10` `L-1` is ⛔ one ladder, ⛔ one climber. Two slots is two rules.** ⭐ TASK-779 item (5) already flags this as a BLOCKER |
| ⛔ **A TICK, TIMER, POLL or INTERVAL on `AClimbableTower`** | ⛔ **TASK-777 armed a test that ERRORS on any such reflected member (`SiegeClimbableTowerTest.cpp:1197`), and ⛔ the no-tick property is `TOWER-§`'s.** ⛔ **The tower must be ⛔ TOLD, ⛔ never poll** |
| ⛔ **Moving `LadderClimbSpeedUU` onto `AClimbableTower`** *(proposed by TASK-778 and by `HeroCharacter.cpp:2008` as "the clean landing")* | ⛔ **RULED OUT.** ① it makes the climb rate **PER-TOWER**, a design change ⛔ nobody asked for — a climb rate is a ⛔ PAWN stat ② it spends 🧑 **Jonathan's `TOWER-§9.3` exposure lever**, which prices the whole climb in Longbowman shots against the ⛔ UNIT's value ③ `CONTACT-§6` anti-scope-creep. ⇒ ⭐ **the LAW was repaired to match the CODE instead** (`CONTACT-§8`, the hero's-members row) |

### CONTACT-§12.5 ✅⭐ **THE INDEPENDENT BELT — THE OCCUPANCY TERM ALSO ASKS `IsClimbing()`. ⛔ A BELT, ⛔ NOT THE MECHANISM.**

`EvaluateLadderEntry`'s occupancy argument is fed today by `ActiveClimber.IsValid()` (`ClimbableTower.cpp:730`). ⇒ ✅ **it additionally requires the held climber to still be CLIMBING, read through `ILadderClimber::IsClimbing()`.**

- ⭐ **WHAT IT BUYS: a slot that ⛔ CANNOT BRICK THE LADDER even if a completion signal is ever missed** — by a future climber class, a mis-ordered broadcast, or a binding that failed. ⇒ ⛔ **the worst case degrades from "the tower is dead for the match" to "one admission is late by one poll".**
- ✅ **IT IS ⛔ FREE AND ⛔ NOT LATE:** the term is evaluated at ⛔ every admission attempt, so a released slot reads free at ⛔ that instant — ⛔ **there is ⛔ no latency versus the eager path.**
- ⛔⛔ **BUT IT IS ⛔ NOT A SUBSTITUTE FOR SEAM 2, AND THAT IS RULED:** without the eager release the tower keeps a **stale `ActiveClimber`** and the two pawn classes get **⛔ DIVERGENT release semantics** — ⚖️ *two mechanisms for one traversal is what `CONTACT-§2` refused for the driver, and the same answer applies here.* ⇒ ⭐ **BOTH ship. This is the shipped house idiom, third application** (`ClimbableTower.cpp:204-209` *"belt-and-braces"* · `H-5 UnPossessed` *"the independent belt"*).
- ✅ **THE BELT IS AS TRUSTWORTHY AS THE DELEGATE, ⛔ MEASURED:** `AHeroCharacter::IsClimbing()` returns `LadderClimb.bActive` (`HeroCharacter.cpp:1582`), whole-struct reset by `FSiegeLadderClimbStatics::End` — which appears **⛔ EXACTLY ONCE** in that file (`:1721`), inside the ⛔ one teardown all ten exits route through. ⭐ **Same for the unit.**

### CONTACT-§12.6 ⚖️ **THE `H-10` ARGUMENT SURVIVES THE WIDENING ⛔ INTACT — ✅ AND IT IS THE ONE PROPERTY TO RE-CHECK AT QA**

- ✅ **`EndPlay` already reaches the climber through `ILadderClimber`** (`ClimbableTower.cpp:225-227`), ⛔ **not through a concrete class** — TASK-777 built that half correctly. ⇒ **the moment `ActiveClimber` can HOLD a hero, `H-10` works with ⛔ no further edit there.**
- ⛔ **THE RE-ENTRANCY ORDER IS ⛔ PINNED AND ⛔ MUST NOT MOVE: BIND BEFORE THE CALL** (`ClimbableTower.cpp:749-764`). A `Begin` that ends synchronously broadcasts ⛔ inside that call; binding afterwards misses it and leaves the stale slot this whole section exists to prevent. ⭐ **`ActiveClimber` is set FIRST for the same reason.**
- ⛔ **`ReleaseClimber` must survive a climber with ⛔ no live binding** (the `Begin`-returned-false path, `:775-786`). ✅ **It is already idempotent by construction** (`RemoveDynamic` on an unbound delegate is a no-op) — ⛔ **do ⛔ not add a "was it bound?" flag.**
- ✅ **`CanTeamAscend` is ⛔ UNTOUCHED and its ⛔ PRECEDENCE is ⛔ UNTOUCHED: IDENTITY → TEAM → OCCUPANCY.** ⛔ **The identity term is WIDENED, ⛔ never removed** — ⚖️ *an `ACharacter` that is not an `ILadderClimber` (the ghost pawn, a future spectator) must still be refused, and refused ⛔ by identity.*
- ✅ **`ELadderEntryVerdict::NotAnAdmittedClimber` is ⛔ ALREADY the right name** — TASK-778 landed the `NotASummonedUnit` rename (board item 0c) ⛔ ahead of the widening that makes it true. ⭐ **⛔ Nothing to rename in TASK-787.**

## ⛔⛔ CONTACT-§13 — **A MESH TRANSLATION IS ⛔ NOT UV-NEUTRAL. ⭐ MEASURED, ⛔ AND IT REFUTES A BOARD ITEM I WROTE.** *(added 2026-09-02; bought by TASK-783, which ⛔ disagreed with its own spec, ⭐ in writing, exactly as instructed)*

> ### ⚖️⭐⭐ **I WROTE, ON THE BOARD: *"⛔ NO texture rebake — the UVs do ⛔ not move."* ⛔ THAT IS ⛔ FALSE, AND THE ARTIST MEASURED IT RATHER THAN OBEYING IT.**

- ⛔ **THE MECHANISM, AND IT IS WHY THE INTUITION FAILS: `uv_atlas()` PACKING IS ⛔ OBJECT-SPACE.** Translating the ladder shifts its projected UVs, **and the packer then re-lays out the ⛔ ENTIRE mesh.** ⇒ ⛔ **`2508 / 2508` loop UVs changed · max UV delta `0.973`.**
- ✅⭐⭐ **AND IT WAS PROVED CAUSAL, ⛔ not asserted: a SAME-MESH control built twice gives `max UV delta 0.0000000000` ⇒ the pipeline is DETERMINISTIC ⇒ ⛔ the repack is caused ⛔ BY THE TRANSLATION, ⛔ not by packer noise.** ⚖️ *A control run is what turns "it changed" into "it changed because of this", and it is the difference between a finding and a guess.*
- ⇒ ⛔⛔ **CONSEQUENCE, ⛔ FORCED: `T_WatchTower_{D,N,ORM}` are a ⛔ BAKED UV ATLAS, ⛔ not tiling library textures.** ⇒ **the three textures were RE-BAKED and ⛔ MUST be reimported ⛔ WITH the mesh.** ⛔ **Honouring my fence's letter would have shipped a repacked mesh against a ⛔ STALE atlas — ⭐ every readback correct, ⛔ the tower sampling garbage.**
- ⚖️⭐ **THE STANDING RULE:** ⛔ **⛔ NO mesh edit — ⛔ including a pure translation — may be declared UV-neutral ⛔ without a measured loop-UV diff ⛔ plus a same-mesh determinism control.** ⛔ **"It is only a translation" is ⛔ NOT evidence.** ⇒ **the mesh and its baked textures import ⛔ TOGETHER or ⛔ neither.**
- ⭐ **AND KEEP ITS SECOND CAUTION, WHICH IS ABOUT ⛔ MY ARITHMETIC AND ⛔ NOT ITS OWN:** TASK-783's measured clearance came in **BELOW** my reconstruction — **`103.3201` vs my `103.330`** — because ⭐ **it minimised over the ⛔ WHOLE LINE against ⛔ EVERY hull, while I minimised against ⛔ ONE feature.** ✅ **Still PASS (+5.3201).** ⚖️ ***A reconstruction is ⛔ OPTIMISTIC BY CONSTRUCTION: it can only find the worst case it thought to look at.*** ⇒ ⛔ **a manager reconstruction is a ⛔ PREDICTION and ⛔ never the acceptance** — `CONTACT-§7a`'s own closing clause said exactly this before the measurement arrived, and the measurement confirmed the clause rather than the number.

---

## ⛔⛔ CONTACT-§14 — **THE MERGED DEFECT: THE ABDUCTION WINDOW AND THE LATERAL CLIMB OFFSET ARE ⛔ ONE DEFECT, ⛔ NOT TWO ROWS** *(added 2026-09-02; bought by `TASK-800`, whose band arithmetic reproduced `TASK-798`'s independently-boarded number to three figures)*

> ### ⭐⭐ **THE MECHANISM, IN ONE SENTENCE: THE CONTACT TRIGGER ADMITS FROM A ⛔ 350 uu 2D ***DISC***, THE CLIMB DRIVER HAS ⛔ NO CROSS-TRACK TERM, AND ⛔ NOTHING SNAPS THE PAWN ONTO THE LINE ⇒ ⛔ A PAWN ADMITTED OFF-LINE STAYS OFF-LINE FOR THE ⛔ ENTIRE CLIMB.**

### §14.1 ⭐⭐ **THE AXIS IS `Y`, AND IT IS PROVEN ⛔ FIVE WAYS — ⛔ not inferred from a screen direction**

`Y` is the axis **across the ladder's clear opening** (`V_AXIS = (0,1,0)`); `X`/`w` is the **standoff** axis. They are orthogonal and they are ⛔ **different defects**.

| # | term | cite | `Y` contribution |
|---|---|---|---|
| 1 | the climb **line** itself | both sockets at `y = 0` ⇒ `_D = (300,0,1200)` ⇒ `U_AXIS.y = 0` | **identically 0 at every `t`** |
| 2 | the capsule-centre **lift** | `SiegeLadderClimbStatics.cpp:65` `FVector Lift(0,0,HalfHeight)` | **pure Z ⇒ 0** |
| 3 | `ClimbDirection` | `SiegeLadderClimbStatics.cpp:102-108` `(End − Start).GetSafeNormal()` | **exactly 0** |
| 4 | **both** movement branches | swept `HeroCharacter.cpp:1867` / `SummonedUnit.cpp:3993`; deck-breach `:1898` / `:4019` — both drive along `Direction` only | **0** |
| 5 | the player's **steer** | `HeroCharacter.cpp:1947-1972` — `DoMove` ⭐ **CAPTURES AND DELIBERATELY DOES ⛔ NOT FORWARD**, ⛔ no `Super::DoMove` | **0** |

⭐ **And `VIS-§1` was honoured: the ⛔ BASE class was checked too** — `AGitClaudeUnrealTestCharacter` declares ⛔ no `Tick`, and its only movement is the `AddMovementInput` inside `DoMove`, which the hero's override **returns before calling** while climbing. ⇒ ⛔ **there is no inherited lateral driver.**

⇒ ⭐⭐ **THE CLIMB IS A RIGID TRANSLATION ALONG A CONSTANT VECTOR WHOSE `Y` IS ZERO, AND ⛔ NOTHING PUTS THE PAWN ON THE LINE** (`Begin` never reads or writes the pawn's location; `TryBeginContactClimb` reads it for three terms and ⛔ never writes it back; ⛔ neither `BeginLadderClimb` contains a `SetActorLocation`/`TeleportTo`). ⇒ ⛔ **`Y(top) == Y(entry)`, EXACTLY.**

### §14.2 ⛔ **THE ADMISSIBLE BAND ⛔ EXCEEDS THE MARGIN AT ⛔ ALL FOUR SHIPPED SPEEDS**

**True clear width `132.0 uu`** — ⭐ **measured as-built off the mesh's own 248 ladder vertices** (`ladder_contract/as_built/ladder_clear_width_uu`), ⛔ not merely derived from `STILE_CY 76`/`STILE_HY 10` ⇒ inner faces `|y| = 66`. ⛔ **The rungs do ⛔ NOT narrow it** — they span the full `|y| ≤ 86`. ⇒ hero (`r 42`) side margin **24.0 uu**; unit (`r 34`) **32.0 uu**.

| approach speed | admissible lateral band `±d` | vs the hero's **24.0 uu** |
|---|---|---|
| 150 uu/s | **±277.8** | 11.58× |
| **300 uu/s (walk)** | **±247.2** | 10.30× |
| 500 uu/s | ±197.4 | 8.23× |
| 750 uu/s (sprint) | **±116.8** | 4.87× |
| **937.5 uu/s (sprint + Swift Boots — the FASTEST shipped)** | **±34.9** | **1.45×** |

⇒ ⛔⛔ **AT ⛔ NO SHIPPED SPEED DOES THE TRIGGER GUARANTEE THE HERO ENTERS THE CLEAR OPENING AT ALL.** Even the fastest approach — where dwell has least time to accumulate and the band is tightest — **overshoots by 45 %.**

⭐⭐ **THE MERGE, AND IT IS THE WHOLE POINT OF THIS SECTION: `±247.2` REPRODUCES `TASK-798`'s INDEPENDENTLY-BOARDED ABDUCTION NUMBER ⛔ TO THREE FIGURES**, from the shipped tunables. ⇒ **the abduction window is ⛔ not merely *"a pawn is grabbed from too far away"* — it is *"a pawn is grabbed from too far away ⭐ AND CARRIES THAT OFFSET UP THE LADDER."* ⇒ ⛔ ONE DEFECT, ⛔ ONE FIX, ⛔ NOT TWO ROWS.**

### §14.3 ⚠️ **WHY IT IS INVISIBLE, AND THE ⛔ NEW CONSEQUENCE NOBODY LOOKED FOR**

- ⛔ **The ladder has ⛔ ZERO collision hulls** (`collision/ladder_hulls = 0`, deliberate) ⇒ a capsule overlapping a stile is ⛔ **not depenetrated, not blocked, not slid.** ⭐ **That is how a real overlap coexists with the analyst's "no frame shows any body-clip" — ⛔ neither observation is wrong.**
- ⚠️⚠️ **THE ARRIVAL POP — ⛔ NEW, ⛔ never looked for.** `SetActorLocation(FSiegeLadderClimbStatics::ArrivalTarget(LadderClimb), /*bSweep=*/false)` at **`HeroCharacter.cpp:1844`** / **`SummonedUnit.cpp:3964`**, and `ArrivalTarget` returns `State.End` — **a point with `y = 0`** (`SiegeLadderClimbStatics.cpp:110-115`). ⇒ ⛔ **on the arrival frame the pawn is teleported laterally by its ENTIRE accumulated `Y` offset — up to ~247 uu — in ⛔ ONE FRAME, ⛔ UNSWEPT.** ⛔ **Unresolvable at `VID-004`'s 0.4–0.5 s sampling cadence**, which is exactly why it is absent from the footage.

### §14.4 ⛔⛔ **THE `10 uu` MOVE IS ⛔ CORRECT ⛔ BUT ⛔ INERT FOR `Y` — ⭐ RECORD BOTH HALVES TOGETHER OR SOMEONE WILL REVERT A CORRECT CHANGE**

⚖️ **THIS CLAUSE EXISTS TO ⛔ PREVENT A REVERT. Read both rows before touching `LADDER_OUTWARD_SHIFT`.**

| half | verdict |
|---|---|
| **for the `Y` cross-track defect** | ⛔⛔ **EXACTLY `0.000 uu`. ⛔ NOT "insufficient" — ⛔ INERT.** `δ = (−10,0,0)` is **X-only**; it moves the line **and** the trigger disc by the same `−10` in `X` and changes the cross-track `Y` error by **zero**. ⇒ ⭐ **the question *"is 10 uu enough?"* is ⛔ MALFORMED: ⛔ NO pure-`X` value answers a `Y` offset — `4.6`, `10`, `100` and `1000` all move it by ⛔ zero.** |
| **for the `K-1` / `TOWER-§8.5a` standoff it was ⛔ ACTUALLY ruled for** | ✅⭐ **MEASURED GOOD, and it is ⛔ REQUIRED.** min spine→body `93.61875 → 103.3201`; **hero clearance `51.61875` (⛔ 4.38125 SHORT of 56) → `61.32015`**; `hero_margin_over_56_uu = +5.32015` ✅; `body_only_HERO_pass_56` ⛔ false → **true**. ⇒ **`TOWER-§8.5a`'s licence is RE-EARNED for the `r 42` capsule.** |

⇒ ⛔⛔ **`TASK-783`'s art work is ⛔ CORRECT and may ⛔ NOT be reverted, revisited or re-scoped.** ⛔⛔ **AND ⛔ NOBODY MAY BELIEVE IT FIXED WHAT JONATHAN SAW: two ⛔ ORTHOGONAL defects on one ladder — ⛔ one is shipped-fixed, the other is ⛔ untouched.**

### §14.5 ⚖️ **THE FIX'S FENCE — ⛔ ADDITIVE ONLY. ⛔ THIS IS LAW, ⛔ not a preference.**

**The shape:** ⛔ **ONE ADDITIVE pure function** in `FSiegeLadderClimbStatics` (e.g. `SteerDirection(State, CurrentWorld)`) returning the unit vector toward a look-ahead point **on the line**, so the pawn converges onto it — plus ⛔ **TWO one-line driver edits** (each driver's ⛔ one movement call). ⛔ **No art, ⛔ no mesh, ⛔ no re-bake, ⛔ no reimport, ⛔ no asset of any kind.**

⛔⛔ **THE TRAP, NAMED SO THE IMPLEMENTER DOES ⛔ NOT FALL IN IT: `ClimbDirection` IS ⛔ ALSO READ BY THREE OTHER SITES** — `Advance`'s **arrival dot test** (`SiegeLadderClimbStatics.cpp:149`), the **deck-breach step** (`HeroCharacter.cpp:1898` / `SummonedUnit.cpp:4019`), and `IsLadderClimbInputHeld`'s **sustain sign test** (`HeroCharacter.cpp:1901-1945`, the dot at `:1944`). ⇒ ⛔ **`ClimbDirection` MUST KEEP ITS ⛔ CURRENT MEANING AT ALL THREE, or arrival and sustain change semantics ⛔ SILENTLY.** ⇒ **the new function is ⛔ ADDITIVE — ⛔ never a redefinition of `ClimbDirection`.**

⭐ **Variant A2, if the pop matters more than purity:** carry the entry cross-track error in `FSiegeLadderClimbState` and lerp it to zero over the first ~150 uu of line ⇒ ⛔ no pop at entry, ⛔ no pop at arrival (**§14.3 disappears for free**), one extra float of state.
⚠️ **`SC-§36.1`: the pure function and its two call sites are ⛔ ONE task, ⛔ not two.**

### §14.6 ⛔ **THE THREE ALTERNATIVES ARE ⛔ DEAD — recorded so ⛔ none is re-proposed as "simpler"**

| alternative | verdict | why |
|---|---|---|
| **change the sockets** | ⛔ **VOID — there is ⛔ nothing to change** | both are ⛔ **already exactly** on the opening's centre-plane (`y = 0`, measured). A socket `Y` offset moves the line ⛔ OFF centre ⇒ ⛔ **strictly worse.** |
| **widen the mesh** | ⛔ **IMPOSSIBLE AT ⛔ ANY COST** | covering the 300 uu/s case needs `2 × (247 + 42) =` **578 uu** of clear width against a west bay that is `2 × 130 =` **260 uu** ⇒ ⛔ **structurally impossible** — ⭐ **and** `CONTACT-§13` makes ⛔ any mesh edit repack the ⛔ entire UV atlas ⇒ full rebake + coupled three-texture reimport. ⇒ ⛔ **highest cost, lowest coverage, and it ⛔ still fails.** |
| **tune the cone alone** (`LadderContactIntentCos`) | ⛔ **MITIGATION, ⛔ NOT A FIX** | `0.5 → 0.98` (**11.5°**) still leaves **±49.1**, ⛔ **2× the margin**, and would make the ladder feel unusable. ⛔ **It also cannot be paid for by shrinking `R`:** `CONTACT-§7b` `K-6.1` raised `R` to `350` on the measurement that sprint + Swift Boots needs `937.5 × 0.35 = 328.125`. |

### §14.7 ⚖️🧑 **WHAT IS ⛔ NOT RULED HERE — ⛔ JONATHAN'S, AND SURFACED ⛔ WITHOUT A RECOMMENDATION**

⭐⭐ **THE ORDER-BLIND PREDICATE IS A ⛔ LITERAL TRANSCRIPTION OF HIS OWN SENTENCE** — *"any units can climb the ladder by simply walking up to it and walking against it"* ⇒ **walk** (speed floor) · **toward it** (60° cone) · **keep doing it** (dwell). ⛔ **AND THE ±247 uu GRAB CORRIDOR IS THE ⛔ SAME PREDICATE.** ⇒ ⛔⛔ **THE FEATURE AND THE DEFECT ARE ⛔ ONE PREDICATE: ⛔ NOT SEPARABLE BY TUNING, ⛔ ONLY ⛔ TRADABLE.**

⚠️ **The dwell lever is ⛔ NOT free for the hero** — sprint + Swift Boots already needs `R ≥ 328.125` against a `0.35` dwell, and raising the dwell raises that floor. ⇒ ⭐ **a ⛔ PER-CLASS dwell (unit `0.50` / hero `0.35`) is the ⛔ shape that buys the narrowing without costing him his sprint climb.** ⛔ **Boarded as ⛔ HIS ruling, ⛔ not applied.**

⛔⛔ **AND THE `26.4 uu` MAGNITUDE IS ⛔ STILL A HYPOTHESIS (`SC-§20`). `TASK-800` ⛔ DECLINED TO CERTIFY IT** and ⛔ nothing here certifies it: it rests on an **eyeballed ~20 %**. ⭐ What ⛔ is added is that `26.4` sits at the ⛔ **LOW END** of a band the mechanism permits up to `±277` ⇒ **the observation is ⛔ not merely possible — it is what the mechanism ⛔ PREDICTS.**

---

