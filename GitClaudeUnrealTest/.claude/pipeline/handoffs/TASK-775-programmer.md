# TASK-775 — [CONTACT-0] DIAGNOSE-FIRST — the climb diagnosis

**Agent:** gameplay-programmer · **Date:** 2026-09-01
**Law read in full before any measurement:** `CONTACT-§0..§9` (CONVENTIONS.md:5792-5969), plus `TOWER-§8.3` (`:5341-5364`), `TOWER-§8.4(A)` (`:5368-5370`), `TOWER-§8.5a` (`:5421-5453`), `RECALL-§4 R-2` (`:5654`).

## ⛔ FILES WRITTEN: THIS ONE, AND THE BOARD STATUS LINE. NOTHING ELSE.

⛔ No source, no test, no asset, no `CONVENTIONS.md` edit, no compile, no Git, no editor. **Zero lines of shipped code changed by this task.**

---

## ⚠️⚠️ READ THIS BEFORE TRUSTING ANY `SummonedUnit.cpp` LINE NUMBER IN OUR RECORD

**TASK-776's statics lift LANDED WHILE THIS INVESTIGATION WAS RUNNING.** Measured: `SummonedUnit.cpp`
went **4,339 → 4,165 lines (−174)**, and `SiegeLadderClimbStatics.{h,cpp}` now exist (untracked at the
time of writing; `git status` confirms `M SummonedUnit.cpp`, `M SummonedUnit.h`, `M SiegeLadderClimbTest.cpp`,
`?? SiegeLadderClimbStatics.{h,cpp}`).

⇒ ⛔ **EVERY `SummonedUnit.cpp` LINE NUMBER IN `CONTACT-§1` AND ON THE BOARD IS NOW STALE BY ~174 LINES.**
The law's `SummonedUnit.cpp:3075-3077` (the `bProjectDestinationToNavigation = true` cite) is **now
`:2901-2902`**. The three `AClimbableTower` comments cited as `:784`/`:3901`/`:3951` are **now
`:610`/`:3727`/`:3777`**.

**Every line number below is re-derived against the CURRENT (post-lift) files.** Where the law cites a
pre-lift number I give both. ⚖️ *Flagged rather than silently renumbered, because a diagnosis whose
citations do not resolve is a diagnosis nobody can re-derive.*

---

# Q1 — DOES A `ASummonedUnit` ORDERED ONTO THE PLATFORM ACTUALLY CLIMB TODAY, AND IF NOT, WHY?

## ⚖️ VERDICT: THE LEADING HYPOTHESIS IS **REFUTED AS STATED** — AND THE ANSWER IS **BOTH**, SPLIT BY WHO IS DRIVING.

The manager's hypothesis has two clauses. **One is confirmed exactly. The other is false, and I found
the counter-example at source.**

### ✅ CLAUSE 1 — "the only non-test `ClimbableTower` references are comments" — **CONFIRMED, and it is stronger than 4**

Grepped `Source/` for `ClimbableTower`, excluding `ClimbableTower.{h,cpp}` itself and `Siegebound/Tests/`.
**Result: 10 references, and ⛔ every single one is a comment. ⛔ ZERO executable references.**

| File:line | Kind |
|---|---|
| `SiegeGhostPawn.h:242` | comment |
| `SummonedUnit.cpp:610` · `:3727` · `:3777` | comments (the law's `:784`/`:3901`/`:3951`, pre-lift) |
| `SummonedUnit.h:85` · `:386` · `:461` · `:1008` · `:1052` · `:1075` | comments |

⭐ The manager's count of **4** was an undercount (it missed `SummonedUnit.h`'s six), but the count was
never the claim — **the claim was "zero code", and zero code is exactly right.**

### ⛔⛔ CLAUSE 2 — "NOTHING IN THE GAME EVER PRODUCES A DESTINATION ON THE DECK" — **REFUTED. THERE IS ONE, IT IS REACHABLE TODAY, AND A PLAYER CAN FIRE IT WITH A MOUSE.**

**I traced every goal-producing site that can reach `AI->MoveToActor` / `MoveToLocation`. There are
exactly six call sites in the whole project.** Full table:

| # | Call site | Goal | Can it land on the deck? |
|---|---|---|---|
| 1 | `SummonedUnit.cpp:2762` `MoveToActor(Goal, …)` | `CurrentTarget` — an enemy **pawn** | ⛔ **No** — only if a pawn were already on the deck, which requires a climb. Bootstrap-impossible today |
| 2 | `SummonedUnit.cpp:2789` `MoveToLocation(MarchPoint, …)` **`bProjectDestinationToNavigation = false`** | `ResolveStructureMarchPoint` = nearest point on the structure's **ECC_Pawn-blocking collision** to this unit, nav-projected | ⛔ **No** — a wall-face point at the unit's own height; ground |
| 3 | `SummonedUnit.cpp:2795` `MoveToActor(Goal, …)` | structure actor origin (legacy degrade) | ⛔ **No** — actor origin, ground |
| 4 | ⭐⭐ `SummonedUnit.cpp:2901-2902` `MoveToLocation(Point, …)` **`bProjectDestinationToNavigation = true`** | `EnterAdvanceToLocation(Point)` — **three producers, one of which is player-authored (below)** | ✅⛔ **YES — see the chain** |
| 5 | `MinerUnit.cpp:921-922` `MoveToLocation(Point, …)` `bProject… = true` | miner order point / castle-interior anchor | ⛔ **No** — and ⛔ `AMinerUnit` is not on the tower path at all |
| 6 | `MinerUnit.cpp:986` `MoveToActor(Node, …)` | `AGoldNode` | ⛔ **No** — ground node |

**The goal producers behind #1/#3 (`EnterAdvance(AActor*)`), all ground-level:** `CurrentTarget`
(`:1600`, `:1807`, `:1931`, `:2394`) · `FindNearestEnemyCastle()` (`:1603`, `:1810`, `:2367`) ·
`OwnCastle` (`:1768`) · `FollowGoal` = `HealTarget` or `FindNearestFriendlyCombatUnit()` (`:2420`, `:2435`).

**The three goal producers behind #4 (`EnterAdvanceToLocation`):**
- `SummonedUnit.cpp:3234` — `SidestepGoal` from `FSiegeStuckStatics::ComputeSidestepGoal`, a lateral
  offset from the unit's **own** location. ⛔ Ground.
- `SummonedUnit.cpp:2025` / `:2077` — FOLLOW: `Anchor->GetActorLocation() + GroupStationOffset`, the
  hero's location. ⛔ Ground (the hero cannot get on the deck — `C-1`).
- ⭐⭐ `SummonedUnit.cpp:1941` / `:1948` — **HOLD tier 3: `Group.PositionCenter + GroupStationOffset`.
  THIS IS THE ONE.**

### ⭐⭐ THE CHAIN THAT PUTS A UNIT'S DESTINATION ON THE DECK — every link measured at source

| Step | File:line | What it does |
|---|---|---|
| 1 | `SiegePlayerController.cpp:3931-3936` | `TraceCursorToGround` = `GetHitResultUnderCursor(**ECC_Visibility**, bTraceComplex=false)` + `bBlockingHit` |
| 2 | ⭐ `Building.cpp:47` | `VisualMesh->SetCollisionProfileName(**BlockAll**)` ⇒ ⛔ **the tower blocks `ECC_Visibility`.** `AClimbableTower : ABuilding` inherits it — the ctor (`ClimbableTower.cpp:111-133`) touches collision ⛔ nowhere |
| 3 | `TASK-737-artist.md` §3, hull **07** | the deck slab's **+Z face IS z = 1200**, `X/Y ∈ [−300,+300]` ⇒ ⛔ **a cursor on the deck returns `ImpactPoint.Z ≈ 1200`** |
| 4 | `SiegePlayerController.cpp:2937-2940` | `GroupPickLocation = Hit.ImpactPoint` — the header calls it *"the SURFACE under the cursor … never the Z=0 plane"* (`SiegePlayerController.h:1660-1661`) |
| 5 | `SiegePlayerController.cpp:3094` → `:3213` | `GroupPickPositionCenter = GroupPickLocation` → `NewGroup.PositionCenter = PositionCenter`. ⛔ **No nav-projection and no Z clamp on the centre itself** |
| 6 | `SiegePlayerController.cpp:3260-3281` | per-unit sunflower `Station`, then `ProjectPointToNavigation(Station, Projected, {r/2, r/2, **200**})` — a **±200 uu Z extent**, which comfortably reaches the deck poly from a z-1200 click |
| 7 | `SummonedUnit.cpp:1941-1948` | `UpdateStateHold` tier 3 → `EnterAdvanceToLocation(Station)` |
| 8 | `SummonedUnit.cpp:2901-2902` | `MoveToLocation(Point, …, **bProjectDestinationToNavigation = true**, …, bAllowPartialPath = true)` |

⇒ ⭐⭐ **A player can circle their units (`ESiegeGroupCommandType::Hold`, `UnitCommand.h:85-90`), point
the position circle at the tower deck, and confirm — and every member of that group is handed a
destination on the deck poly.** A path to a deck-poly goal has **exactly one** legal route: the ladder
link. `HandleLadderLinkReached` fires, `Cast<ASummonedUnit>` succeeds (`NewGroup.Members` is typed
`ASummonedUnit*`), and the unit climbs.

⚠️ `Ambush` reaches the same `PositionCenter` through `AcquireEnemyNearPoint(Group.PositionCenter,
Group.PositionRadius)` (`SummonedUnit.cpp:1919`); `Follow` does not use it.

### ⚖️ SO: BUG OR DESIGN GAP? — **PLAINLY, IT IS BOTH, AND CONFLATING THEM IS THE TRAP**

| Driver | Verdict | Why |
|---|---|---|
| **The autonomous AI / bot / cards / intents** | ⛔⛔ **DESIGN GAP, ⛔ NOT A BUG** | ⛔ **Nothing in the game ever *chooses* the deck.** Every autonomous goal is an enemy pawn, a castle, an own castle, a heal target or a stuck-sidestep — all ground. `CONTACT-§7 K-2` stands **exactly as written**, and `CONTACT-§6` binds: ⛔ **I propose no garrison order, no `tower` symbol, no intent, no bot rule, no card behaviour.** ✅ Recorded and stopped |
| ⭐ **The player's HOLD / Ambush group command** | ⛔ **NOT a design gap — the path EXISTS TODAY** | ⇒ whether it actually climbs is an **untested runtime question**, ⛔ not an absent feature. **That is why Q2 is ⛔ NOT moot and I answered it.** |

### ⛔ ONE SENTENCE OF THE LAW IS FALSE AND I AM SAYING SO RATHER THAN INHERITING IT

`CONTACT-§1` C-3 concludes: *"⛔ no amount of repairing the link would ever have shown a symptom"* and
the board says *"⛔ no runtime test would ever have shown a symptom."* ⛔ **Both are false.** A HOLD
order clicked on the deck is a runtime test, it is reachable with a mouse today, and it would show a
symptom. ⚖️ *Recording this because `CONTACT-§1`'s own header claims every row was read at the source
before the law was written — this row was reasoned from a grep, and the grep was of the wrong noun
(`ClimbableTower` references) for the question (deck destinations).*

⭐ **It does ⛔ NOT change what the batch builds.** `C-1`, `C-2` and the animation waiver are untouched,
and `K-2`'s ruling (⛔ no garrison order) is **correct on its merits**, not merely on its premise.

---

# Q2 — THE OTHER CANDIDATE CAUSES — ⭐ LIVE, ⛔ NOT MOOT (Q1 found a real reason to path there)

### (a) DO `LadderFoot` / `LadderTop` RESOLVE AT RUNTIME? — ✅ **THEY SHOULD, AND THE FALLBACK IS ⛔ NOT SILENT**

- **The sockets EXIST on the shipped asset.** `Content/Meshes/SM_WatchTower.uasset`'s name table
  contains `LadderFoot`, `LadderTop`, `StaticMeshSocket`, `SocketName` (binary name-table scan, ⛔ no
  editor needed).
- **The BP is wired.** `Content/Blueprints/Buildings/BP_Building_WatchTower.uasset` references
  `/Game/Meshes/SM_WatchTower` (×1), `ClimbableTower` (×9), `VisualMesh` (×1).
- ⇒ `ResolveLadderSocketRelative` (`ClimbableTower.cpp:272-298`) should take the **socket** path. Its
  four bail conditions (`:281`) — null mesh, null `GetStaticMesh()`, null owner, `!DoesSocketExist` —
  are all satisfied by the shipped pair.
- ⛔ **AND THE FALLBACK IS NOT SILENT: `ClimbableTower.cpp:331-342` logs exactly ONE `Warning` naming
  which socket was missing and whether the line was degenerate.** `TOWER-§8.4(A)`'s "silent except one
  warning" is honoured.
- ⇒ ⛔ **NOT a candidate cause.** ⭐ **Runtime confirmation is free and costs one log grep in TASK-780's
  PIE: if that Warning is ABSENT, the sockets resolved.**

### (b) LINK REGISTRATION + BOTH ENDPOINTS ON REAL GENERATED NAVMESH POLYS — **`DEFERRED — needs PIE`**

⛔ **I did ⛔ not fake this and I did ⛔ not port-check.** The editor is DOWN and the orchestrator owns
its lifecycle. **This is TASK-780's named PIE row.**

⭐ **BUT I FOUND A SOURCE-LEVEL RISK WORTH CARRYING INTO THAT ROW, AND IT IS THE CASTLE-FLOOR DEFECT CLASS:**

> `Config/DefaultEngine.ini:281` sets `RuntimeGeneration=Dynamic` — ✅ correct, and required, since a
> tower is **runtime-spawned** by a card play. ⚠️⚠️ **But the ini's OWN comment, `:291-293`, warns:**
> *"the serialized RecastNavMesh actor in L_Arena carries its OWN copies of these params — TASK-218 must
> mirror them on the actor and run Build > Navigation, or the stale bake silently keeps the old values
> (this ini applies to freshly spawned nav data only)."*

⇒ ⛔⛔ **If `L_Arena`'s serialized `RecastNavMesh` actor still carries `RuntimeGeneration=Static`, a
runtime-spawned tower generates ⛔ NO deck poly at all, `LadderTop` lands on nothing, and the link is
dead by construction — with every readback in `ConfigureLadderLink` correct and ⛔ not one warning
logged.** ⭐ **That is exactly the failure shape `TOWER-§8.3` names ("every readback correct, nothing
can use it"), and it is invisible to source.**

**⇒ THE PIE ROW I RECOMMEND FOR TASK-780, stated so it is checkable:** with a tower placed, confirm
(1) `L_Arena`'s `RecastNavMesh` actor's `RuntimeGeneration` is **Dynamic**; (2) a nav poly exists at
`LadderFoot` world XY (ground, tower-local x ≤ −364); (3) a nav poly exists on the deck (tower-local
X ∈ [−236,+236], z ≈ 1200); (4) `ConfigureLadderLink`'s socket Warning is **absent**.

### (c) IS `CanTeamAscend` REFUSING SOMETHING IT SHOULD ADMIT? — ⛔ **NO**

`ClimbableTower.cpp:189-199`:
```
return SiegeTeamObjectChannel(ClimberTeam) != AscentBlockedChannel(TowerTeam);
```
with `AscentBlockedChannel = SiegeEnemyTeamObjectChannel(TowerTeam)` (`:180-187`). An own-team climber's
channel is never the tower's enemy channel ⇒ **admitted**. ⛔ No capacity term, ⛔ no unit-type term —
the comment at `:194-197` says so and the shipped test proves it across all four team pairs plus a
20-iteration determinism loop (`SiegeClimbableTowerTest.cpp:406-441`). ⇒ ⛔ **NOT a candidate cause.**

### (d) IS `ShouldLinkAllowPathfinding` REFUSING A PATH IT SHOULD ALLOW? — ⛔ **NO — IT FAILS OPEN BY CONSTRUCTION**

`ClimbableTower.cpp:237-270`. Querier not a `AController` ⇒ `return true` (`:249-255`). Pawn not an
`ITeamAgent` ⇒ `return true` (`:259-265`). **Only a fully resolved enemy pawn is refused**, via the same
`CanTeamAscend`. ⛔ It is structurally incapable of stranding an own-team unit. ⇒ ⛔ **NOT a candidate cause.**

### ⭐ (e) THE ONE I ADD, BECAUSE IT IS THE ANSWER TO THE **HERO** HALF AND IT IS AT SOURCE

`HandleLadderLinkReached` (`ClimbableTower.cpp:366-453`) resolves its climber through
`ResolvePathFollowingPawn` → `Cast<UPathFollowingComponent>(PathComp)` (`:60-69`, called `:376`) and then
`Cast<ASummonedUnit>` (`:377`). ⇒ ✅ **`CONTACT-§1` C-2 CONFIRMED at source, on both halves:** a
player-driven hero raises ⛔ no `UPathFollowingComponent` move, so the delegate is ⛔ never fired for it;
and the `Cast<ASummonedUnit>` would yield `nullptr` anyway ⇒ `EvaluateLadderEntry` returns
`NotASummonedUnit` (`:207-210`) — which the code comment already names: *"most obviously the player's own
hero, which paths nowhere but is a pawn all the same."*

---

# Q3 ⛔⛔ THE HERO'S CAPSULE — **MEASURED. AND IT ⛔ FAILS. `TOWER-§8.5a` IS VOID FOR THE HERO.**

## ⛔⛔ THE LAW'S PREMISE IS WRONG: **THE PROJECT ⛔ DOES CALL `InitCapsuleSize` — IN THE HERO'S OWN DIRECT BASE CLASS.**

`TOWER-§8.3` (`CONVENTIONS.md:5357`) and `CONTACT-§3.3` #4 (`:5867`) both assert *"the project never
calls `InitCapsuleSize`"* and conclude the hero is therefore the engine default **r 34**.

⛔ **Grepped the whole of `Source/`. That is false.** The hero's inheritance chain is:

```
AHeroCharacter  (HeroCharacter.h:319)
  : public AGitClaudeUnrealTestCharacter  (GitClaudeUnrealTestCharacter.h:22)
      : public ACharacter
```

and `AGitClaudeUnrealTestCharacter`'s constructor — **the hero's own direct base** — contains:

```
// GitClaudeUnrealTestCharacter.cpp:18
GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
```

⇒ ⭐⭐ **`R_hero = 42.0 uu` · `HalfHeight_hero = 96.0 uu`.** ⛔ Neither 34/88 nor a fallback.

### ✅ THREE INDEPENDENT CORROBORATIONS, ⛔ so this does not rest on one line

1. ⭐ **`Castle.cpp:185-188` already writes the number down:**
   `constexpr float HeroCapsuleRadius = 42.f;` — commented *"Hero capsule radius (Ø84×192 — the GH-R9
   measured capsule, 656 §2's instrument)"* — and it is **load-bearing**: `HeroStopLaneX = 3657.5 + 42 =
   3699.5`, which TASK-656 **measured on pixels** (capsule FREE at 3710, BLOCKED at 3690). ⇒ **42 is not
   a transcription; it has been measured in the world and matched.** Ø84×192 = r 42 / half-height 96. ✅
2. **`SiegeGhostPawn.cpp:51-53`** sizes the ghost `InitCapsuleSize(42.f, 96.f)` with the comment *"The
   hero's own capsule size (`AGitClaudeUnrealTestCharacter` ctor: `InitCapsuleSize(42.f, 96.f)`)"*.
3. **`AHeroCharacter` never resizes it.** Grepped `HeroCharacter.{h,cpp}` for `Capsule`: five hits, all
   `GetCapsuleComponent()` for **`SetCollisionObjectType`** (`:121-123`, `:212-214`, `:225-227`) and one
   `SetupAttachment` (`:90`). ⛔ No `InitCapsuleSize`, ⛔ no `SetCapsuleSize`, ⛔ no `SetCapsuleRadius`.

### ✅ AND `BP_HeroCharacter` OVERRIDES ⛔ NOTHING — read at the asset, ⛔ not assumed

`Content/Blueprints/BP_HeroCharacter.uasset` (39,225 bytes) — **full name-table dump** (the editor is
DOWN, so this is a binary name-table read; an uncooked `.uasset` only carries a name for a property tag
it actually serializes, so **absence of the name is absence of the override**):

- ✅ Parent confirmed: `HeroCharacter`, `Default__HeroCharacter`, `ParentClass` all present.
- ✅ The capsule is present only as **class/subobject identity**: `CapsuleComponent`, `CollisionCylinder`.
- ⛔⛔ **`CapsuleRadius` — ABSENT. `CapsuleHalfHeight` — ABSENT. `InheritableComponentHandler` — ABSENT.**

⭐ `InheritableComponentHandler` is the **only** mechanism by which a Blueprint stores an override on a
**native/inherited** component such as `ACharacter`'s `CollisionCylinder`. Its absence from the package
means **⛔ the BP carries no override of any inherited component at all**, so the capsule is exactly the
C++ CDO value.

⇒ ⛔⛔ **`R_hero = 42.0`. This is a MEASUREMENT, ⛔ not the substituted default the spec warned against.**

## ⛔⛔ THE GATE — AND IT FAILS BY **4.376 uu**

`TOWER-§8.3` (`CONVENTIONS.md:5357`) defines the standoff as **"≥ 56 uu clear between the CAPSULE SURFACE
and the body face along the whole line"**, measured against **r 34**. TASK-737 measured **59.624 uu**
worst case (`handoffs/TASK-737-artist.md:28`, and §5: *"worst body clearance 59.62 uu (plinth, at t = 245)"*).
A wider capsule eats the excess one-for-one:

| Quantity | Value |
|---|---|
| TASK-737 worst-case standoff (vs r 34) | **59.624 uu** |
| `R_hero` | **42.0 uu** |
| Excess over the measured radius | `42.0 − 34 =` **8.0 uu** |
| ⭐ **Residual standoff** `59.624 − (R_hero − 34)` | ⛔⛔ **51.624 uu** |
| Required | **≥ 56.0 uu** |
| ⛔ **Deficit** | ⛔⛔ **4.376 uu SHORT** |
| Max radius that would pass | 37.624 uu — **the hero exceeds it by 4.376** |

## ⇒ ⛔⛔⛔ **`TOWER-§8.5a` IS VOID FOR THE HERO. `§8.5`'s OUTRIGHT REFUSAL APPLIES IN FULL. THIS IS FOR-JONATHAN ROW `K-1`, AND IT IS A HARD GATE ON TASK-778.**

⚠️ **AND 51.624 IS AN ⛔ UPPER BOUND, ⛔ NOT THE TRUE FIGURE.** TASK-737 swept a capsule of **r 34 /
half-height 88**. The hero is **also 8 uu taller per end (96)**, so its swept volume reaches further
along the line at both caps and can only encounter geometry the shorter capsule missed. ⇒ ⛔ **A true
re-measure with r 42 / hh 96 can come out **worse** than 51.624. It cannot come out better.** The gate
fails on the generous number.

### ⛔ WHAT I DID ⛔ NOT DO ABOUT IT (`CONTACT-§3.3` #4, `CONTACT-§0`, and the dispatch, all agreeing)

⛔ **I did NOT shrink the hero's capsule** — it is a combat and collision dimension, and `Castle.cpp:188`
proves it is already load-bearing for a **measured, shipped** castle stop lane (change it and TASK-656's
pixel-matched 3699.5 moves). ⛔ **I did NOT re-author the mesh** (`CONTACT-§0` fence). ⛔ **I did NOT
ship the window anyway.** ⇒ **It is `K-1`, with the measured number beside it, and it is Jonathan's word.**

### THE OTHER THREE RE-DERIVATIONS, REPORTED AS NUMBERS (`CONTACT-§3.3` #1/#2/#3)

The pinned line, from `ClimbableTower.cpp:44-45` and confirmed on the asset by TASK-737:
`LadderFoot (−450, 0, 0)` → `LadderTop (−150, 0, 1200)`; `Δ = (300, 0, 1200)`;
**length = 1,236.9317 uu**; `sin θ = 1200 / 1236.9317 =` **0.9701425** (θ = 75.9638°).

| # | Re-derivation | Unit (hh 88) — shipped | ⭐ **Hero (hh 96)** |
|---|---|---|---|
| **1** | Endpoint lift, surface → capsule centre (`§8.5a` cl. 6) | **88.0 uu** | ⭐ **96.0 uu** |
| **2** | Deck-breach ceiling, **Z** = `3 × HalfHeight` | 264.0 uu | ⭐ **288.0 uu** |
| **2** | …converted by the line's OWN slope = `Z / sin θ` | 272.126 uu | ⭐ **296.863 uu** |
| **2** | …as a % of the 1,236.9 uu ascent | **22.00 %** | ⭐ **24.00 %** |
| **3** | Watchdog budget, raw traversal at `LadderClimbSpeedUU = 350` (`SummonedUnit.h:1113`) | 3.5341 s | **3.5341 s** (line length is unchanged — both endpoints lift equally) |

✅ **My method reproduces the law's shipped unit row exactly** (`§8.5a` cl. 2 states *"264 uu of Z ⇒
272.1 uu of line = 22.0%"`; I compute 272.126 and 22.00%), which is the check that the hero row is
derived the same way and ⛔ not a fresh invention.
⭐ **Clause 2's formula does survive the larger capsule by its own terms** — the window simply grows to
24.00%. ⛔ **But that is MOOT while `§8.5a` is void**, and I am ⛔ not presenting a surviving sub-clause
as if the exception survived.

✅ **Re-derivation 1 is already implemented correctly for the unit and needs ⛔ no repair:**
`SummonedUnit.cpp:3650-3652` reads `GetCapsuleComponent()->GetScaledCapsuleHalfHeight()` and uses
`SiegeSpawn::DefaultCapsuleHalfHeight` **only as a null-capsule fallback** — exactly as `CONTACT-§3.3` #1
requires. ⭐ A hero driver copying that shape reads **96** for free.

⚠️ **INCIDENTAL, FLAGGED ⛔ NOT FIXED (out of scope, no task owns it):** `SiegeSpawn::DefaultCapsuleHalfHeight
= 88` (`SiegeSpawnConstants.h:9`) is used as a **spawn-placement** half-height at `Barracks.cpp:125` and
`SiegePlayerController.cpp:4179`. The hero's real half-height is **96**. ⛔ I did not chase whether either
site can ever place the hero — ⭐ **naming it because the constant's comment calls itself a "fallback" and
this diagnosis just proved that fallback is 8 uu wrong for one specific pawn.**

---

# Q4 — THE RECALL SEAM: DOES `RECALL-§ R-2`'s MOVEMENT-CANCEL READ **INPUT** OR **VELOCITY**?

## ⭐ **NEITHER. IT READS ⛔ POSITION — displacement from an anchor captured at channel start.**

**The site, at file:line:**

- `AHeroCharacter::TickRecall` → `HeroCharacter.cpp:1282`:
  ```
  if (FSiegeRecallStatics::HasLeftAnchor(RecallState, GetActorLocation(), RecallMoveCancelToleranceUU))
  {
      EndRecall(ESiegeRecallExit::CancelledByMovement);   // ⛔ RECALL EXIT 3 of 7
  ```
- The predicate, `HeroCharacter.cpp:1155-1168`:
  ```
  return FVector::DistSquared(State.AnchorLocation, CurrentLocation) > Safe² ;
  ```
  with its own comment: *"**FULL 3D distance, deliberately, not the horizontal projection**: falling off
  the ledge the player was standing on IS leaving the spot."*
- `State.AnchorLocation` is captured at `FSiegeRecallStatics::Begin(NowSeconds, GetActorLocation())` —
  `HeroCharacter.cpp:1101-1112`, called `:1242`.
- Tolerance: **`RecallMoveCancelToleranceUU = 25.f`** (`HeroCharacter.h:896`).
- Driven from `AHeroCharacter::Tick` → `TickRecall(World->GetTimeSeconds())` (`HeroCharacter.cpp:163-165`),
  and `PrimaryActorTick.bCanEverTick = true` (`:61`). ⛔ **Unconditional every frame — ⛔ not input-gated
  and ⛔ not movement-mode-gated.**

## ⇒ ⭐⭐ **WOULD A SCRIPTED `MOVE_Flying` DRIVE CANCEL A RUNNING RECALL? — ✅ YES, AND ⛔ IMMEDIATELY.**

A climb **moves the actor** — 1,236.9 uu of it. `GetActorLocation()` therefore leaves a **25 uu** ball
within the first ~0.07 s at `LadderClimbSpeedUU = 350`. ⛔ **The cancel cannot be evaded by the drive
being scripted, because the check never asks how the pawn moved — only where it is.**

⭐ **THIS IS THE ⛔ FORTUNATE ANSWER, AND IT IS WORTH SAYING WHY:** `CONTACT-§3.4`'s worry was that two
shipped features both say *"movement"* and mean different things by it. ⛔ **They do not.** A
position-based check is **strictly the most inclusive** of the three candidates — it catches input-driven
walking, physics-driven falling **and** scripted interpolation alike. ⇒ **`CONTACT-§3.4`'s second bullet
(*"a climb may ⛔ NOT SURVIVE a recall teleport"*) is already half-served by the shipped code**: the
channel dies the instant the climb starts, so the teleport never arrives to fight the drive.

⛔ **REPORTED, ⛔ NOT FIXED**, per the dispatch. ⚠️ **The seam TASK-778 still owns is the OTHER direction**
— `CONTACT-§3.4` bullet 1, *"a hero may ⛔ not START a climb while a recall channel is running"* — which is
`CanBegin`'s hero mapping and is ⛔ **not** provided by anything above. And ⚠️ `H-7` (recall completion as
a climb exit) is now **near-unreachable in practice** rather than impossible: it can only fire if a
completion and a climb-start land on the same frame, ahead of the cancel. ⛔ **Do not delete `H-7` on the
strength of this — an exit you cannot reach is free; an exit you removed is a hang.**

---

# ⭐ THE TWO MEASURED DEFECTS — BOTH VERIFIED, ⛔ NEITHER FIXED

## DEFECT 1 — `ActiveClimber` IS TYPED TO `ASummonedUnit` ⇒ A HERO CLIMBER IS INVISIBLE — ✅ **VERIFIED**

**`ClimbableTower.h:487`:** `TWeakObjectPtr<ASummonedUnit> ActiveClimber;` — confirmed at the exact line
the law cites. The whole surface is typed to match:

| Site | Signature / use |
|---|---|
| `ClimbableTower.h:448` | `void HandleLadderClimbEnded(**ASummonedUnit\*** Unit, bool bReachedTop)` |
| `ClimbableTower.h:451` | `void ReleaseClimber(**ASummonedUnit\*** Unit)` |
| `ClimbableTower.cpp:382` | `const bool bLadderOccupied = ActiveClimber.IsValid();` ← ⛔ **the one-at-a-time rule** |
| `ClimbableTower.cpp:432` | `ActiveClimber = Unit;` (a `ASummonedUnit*`) |
| `ClimbableTower.cpp:168` (in `EndPlay`, `:147`) | `if (ASummonedUnit* const Climber = ActiveClimber.Get()) { Climber->AbortLadderClimb(); … }` ← ⛔ **`H-10`** |

⇒ ✅⛔ **BOTH CONSEQUENCES CONFIRMED EXACTLY AS STATED.** A hero climber (i) never occupies the slot, so
`TOWER-§10 L-1`'s one-at-a-time rule ⛔ cannot see it and a unit may be admitted onto the same line — and
`ClimbableTower.cpp:221-223` records why that is not cosmetic: *"Two capsules interpolated along one line
WILL interpenetrate — `bUseRVOAvoidance` is false, so … depenetration would shove one OFF the climb line,
**in mid-air**"*; and (ii) is invisible to `EndPlay`'s abort ⇒ ⛔⛔ **the tower falls and the hero hangs in
`MOVE_Flying` forever** — which `ClimbableTower.cpp:154-157` already names as *"the one place the ladder
redesign genuinely adds risk … It will HANG IN THE AIR, permanently, in a match that keeps running around it."*

### ⚖️ THE LAW AMENDMENT I AM ⛔ PROPOSING, ⛔ NOT MAKING (`CONTACT-§4.4`, amending `TOWER-§8.4(B)`)

`OnLadderClimbEnded`'s pinned `TwoParams(ASummonedUnit*, bool)` cannot survive the widening.
**Proposed replacement, for the manager to rule in:**

```
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLadderClimbEnded, APawn*, Climber, bool, bReachedTop);
```

with `ActiveClimber` → `TWeakObjectPtr<APawn>` and `ReleaseClimber` / `HandleLadderClimbEnded` retyped to
`APawn*`. ⭐ **`APawn` is the narrowest type that admits both** (`ASummonedUnit` and `AHeroCharacter` share
no closer ancestor — `ASummonedUnit : ACharacter` and `AHeroCharacter : AGitClaudeUnrealTestCharacter :
ACharacter`, so `ACharacter` would also work and is narrower still — ⚖️ **I flag `ACharacter` as the
tighter option and leave the choice to the ruling**). ⛔ **Not `AActor`** and ⛔ **not a second parallel
hero-only slot** (`CONTACT-§4.4`).
⚠️ **Blast radius, measured so the ruling is costed:** `OnLadderClimbEnded` appears at `SummonedUnit.h:386`,
`:1075`, `SummonedUnit.cpp:3777`-region, `ClimbableTower.cpp:434` / `:483`, and is **asserted as a reflected
delegate property by `SiegeLadderClimbTest.cpp:883`** — ⇒ ⛔ **the existing suite touches it, so TASK-776's
"suite delta EXACTLY ZERO" and this amendment must ⛔ not land in the same edit.**
⛔ **`ClimbableTower.{h,cpp}` is TASK-777's sole file — I opened it read-only and changed nothing.**

## DEFECT 2 ⭐⭐ — THE LADDER HAS ⛔ ZERO COLLISION — ✅ **VERIFIED AT THE RECORD, AND THE ARITHMETIC IS INDEPENDENT OF THE SAMPLING**

`handoffs/TASK-737-artist.md` §3, verbatim: **"⛔ THE LADDER HAS ZERO HULLS, AND THAT IS THE DESIGN, NOT AN
OMISSION."** Its eight hulls are enumerated and ⛔ **not one is over the ladder**: `00` plinth (X/Y ±300,
Z 0-160) · `01`/`02` keep walls · `03` east wall · `04`/`05` west bay piers · `06` interior wedge · `07`
deck slab (Z 1160-1200).

**Two independent instruments, and I report both:**
1. **Sampling:** *"no collision anywhere over `LadderFoot`'s footprint (72 samples across a 34 uu radius at
   5 heights, zero hits; nearest collision surface **116 uu** away)."*
2. ⭐ **Arithmetic, which needs ⛔ no sampling at all:** *"the westmost collision vertex in the whole asset
   is **x = −300**"* — and `LadderFoot` is at **x = −450** (`ClimbableTower.cpp:44`, confirmed EXACT on the
   asset by TASK-737). ⇒ ⛔⛔ **The ladder foot stands 150 uu west of ANY collision in the entire mesh.**
   That is not a sample that could have missed something; it is a bound.

⭐ My own check is consistent: a name-table scan of `SM_WatchTower.uasset` finds ⛔ no `UCX_` names (weak
evidence on its own — UE stores imported collision as unnamed `FKConvexElem` aggregate geometry, so I say
plainly that **the load-bearing evidence is TASK-737's measurement, ⛔ not my binary scan**).

⇒ ✅⛔ **CONFIRMED: a pawn walking at the ladder passes straight THROUGH it. There is literally nothing to
walk *against*, and any overlap-or-blocking-hit trigger would ⛔ NEVER fire.** `CONTACT-§4`'s
PROXIMITY + INTENT + DWELL design is ⛔ **not a preference — it is forced**, and `RULING 4` is correct.

### ⚖️ BUT IS IT "THE MOST DIRECT EXPLANATION OF WHAT JONATHAN EXPERIENCED"? — ⭐ **IT EXPLAINS THE FEEL. IT IS ⛔ NOT THE CAUSE. I AM RANKING THEM RATHER THAN AGREEING.**

| Rank | Cause | Why this order |
|---|---|---|
| **1** | ⛔⛔ **`C-1` — the hero has ⛔ NO climb code at all.** `BeginLadderClimb`/`AbortLadderClimb`/`IsClimbing`/`OnLadderClimbEnded` exist ⛔ only on `ASummonedUnit` | ⛔ **DECISIVE AND SUFFICIENT.** ⭐ Give the ladder perfect collision tomorrow and the hero still ⛔ cannot climb — there is no code to run |
| **2** | **`C-2` — the only trigger is a PATHFINDER trigger** the hero can ⛔ never raise (`ClimbableTower.cpp:376-377`) | Even a hero with climb code would ⛔ never be offered one |
| **3** | ⭐ **ZERO COLLISION** | ⇒ **This is what he FELT.** Not a bump, not a stop, not a prompt — ⛔ **nothing at all**, because he walked through the ladder and stopped ~300 uu later on the tower body. ⭐ It explains why his report is *"not climbable"* rather than *"I bump into it and nothing happens"* |

⚖️ *Saying it this way because "the ladder has no collision" is the most **satisfying** finding in this
batch and therefore the easiest to over-credit. It is the phenomenology. `C-1` is the cause, and `C-1` is
what TASK-778 is for.*

---

# 📌 WHAT QA / THE MANAGER SHOULD SCRUTINISE

1. ⛔⛔ **`K-1` FIRES. `R_hero = 42`, residual standoff **51.624 uu** < 56.** TASK-778 is **hard-gated** and
   must **STOP AND ESCALATE** at its own step (0). ⛔ This is ⛔ not a capsule resize and ⛔ not a mesh
   re-author. **It is Jonathan's word.**
2. ⛔ **`CONTACT-§1` C-3's premise and `TOWER-§8.3`/`CONTACT-§3.3`'s *"the project never calls
   `InitCapsuleSize`"* are BOTH factually wrong.** Two law rows want correcting by the manager
   (⛔ I edited ⛔ no law).
3. ⭐ **Q1 is a split verdict, ⛔ not a single one.** ⛔ Do not let it collapse to "design gap" — a HOLD
   order on the deck is reachable **today**, so `K-2`'s ruling must be read as *"the AI should not choose
   to garrison"*, ⛔ **not** as *"nothing can ever go up there."*
4. ⚠️ **`SummonedUnit.cpp` line numbers moved by −174 mid-task** (TASK-776 landed). Any reviewer
   re-deriving `CONTACT-§1`'s cites will find them off; **the mapping is at the top of this file.**
5. ⚠️ **Q2(b) is `DEFERRED — needs PIE`**, with the `L_Arena` `RecastNavMesh` `RuntimeGeneration` risk as a
   named TASK-780 row. ⛔ **A guessed navmesh answer is worse than a deferred one, and I did not guess.**
6. ⚖️ **The `OnLadderClimbEnded` amendment is PROPOSED above, ⛔ not made** — and `ACharacter` may be the
   better base than `APawn`. **TASK-777 owns the file; the manager owns the ruling.**
7. ✅ **`CONTACT-§6` HONOURED:** ⛔ no garrison order, ⛔ no `tower` `where` symbol, ⛔ no new intent, ⛔ no
   bot rule, ⛔ no card behaviour. `MARK-§1`'s zero Zone-A characters and the 5658 byte-freeze are
   **untouched**.
