# TASK-799 — Was the 01:20.5 ascent commanded? — READ-ONLY DIAGNOSIS

**Agent:** gameplay-programmer · **Status:** terminal `done` (`CONTACT-§10.2`, the TASK-775 precedent — no QA gate)
**Law:** `VIS-§4a` · `CONTACT-§4`/`§4.1`/`§4.2`/`§4.3`/`§11.5` · `CONTACT-§7b` · `SC-§20`
**Files written:** THIS FILE ONLY. ⛔ Zero `Source/`, zero `Tools/`, zero `Content/`, zero `CONVENTIONS.md`, no compile, no editor, no MCP, no Git.
**C++ suite:** **279**, unchanged. I added no test and removed none.

> ⚠️ Every `file:line` below was re-grepped against the working tree today. `SummonedUnit.cpp` line numbers in older documents are stale by ~174 after TASK-776's lift; nothing here is inherited from a prior cite.

---

## ⛔ THE ONE-WORD ANSWER

**Does the contact predicate gate on the unit having a move order, a destination, a path, or an intent to reach the deck?**

# ⛔ NO.

**But — and this is the finding that matters more — the 01:20.5 archer *was* under an order anyway. The board's premise is refuted.** See §2.

---

## 1. THE PREDICATE, TERM BY TERM, WITH CITES

`AClimbableTower::TryBeginContactClimb` (`ClimbableTower.cpp:769-889`) is the ONE entry point for both climber classes. It composes into `EvaluateContactEntry` (`ClimbableTower.cpp:719-767`), which is `WantsToClimb` (the three contact terms) followed by `EvaluateLadderEntry` (the three eligibility terms). **Order is contractual and deliberately not the tidy one** — `ClimbableTower.cpp:727-733` explains why the cheap refusals may not run first.

### Pre-gate — in `TryBeginContactClimb` itself

| # | Term | Cite |
|---|---|---|
| P0 | Non-null `Climber` **and** non-null `LadderLink` ⇒ else `NoLadder` | `ClimbableTower.cpp:771-774` |
| P1 | Which endpoint: **by Z**, never 2D distance — the two discs overlap in a 400 uu lens at R350 | `ClimbableTower.cpp:794` → `SiegeLadderClimbStatics.cpp:207-224` |
| P2 | Cheap 2D proximity to the nearer endpoint ≤ `LadderContactRadiusUU`; failure calls `ForgetContact` (which IS the K-C re-arm) | `ClimbableTower.cpp:794-802`, `ForgetContact` `:917-931` |

### The three contact terms — `FSiegeLadderContactStatics::WantsToClimb` (`SiegeLadderClimbStatics.cpp:226-315`)

| # | Term | Rule as shipped | Cite |
|---|---|---|---|
| **T1** | **PROXIMITY** | `DistSquared2D(pawn, endpoint) <= 350²`. Negative radius clamps to 0 ("never fires"), never to "fires everywhere" | value `ClimbableTower.h:653` (`LadderContactRadiusUU = 350.f`); test `SiegeLadderClimbStatics.cpp:243-244`; refusal `:281-285` |
| **T2** | **INTENT** | **two** conjuncts: `PawnVelocity.Size2D() >= MinContactSpeedUU` **AND** `dot(vel.Normal2D, (endpoint − pawn).Normal2D) >= LadderContactIntentCos` | `SiegeLadderClimbStatics.cpp:251-255`; `MinContactSpeedUU = 1.f` `SiegeLadderClimbStatics.h:463`; `LadderContactIntentCos = 0.5f` (60° half-cone) `ClimbableTower.h:669`; refusal `:287-291` |
| **T3** | **DWELL** | T1∧T2 continuous for `LadderContactDwellSeconds` **at ONE endpoint** — swapping ends resets the clock; negative delta clamped so the accumulator is monotonic | `SiegeLadderClimbStatics.cpp:296-309`; `LadderContactDwellSeconds = 0.35f` `ClimbableTower.h:685` |
| **T3b** | **K-C RE-ARM LATCH**, tested **before** the dwell | a pawn that just finished a climb here stays `Disarmed` until it changes ends, leaves the radius, or steers out of the cone | `SiegeLadderClimbStatics.cpp:263-279`; `Disarm` `:317-326` |

### The three eligibility terms — `EvaluateLadderEntry` (`ClimbableTower.cpp:272-313`), reached ONLY after T1-T3 pass

| # | Term | Cite |
|---|---|---|
| **T4** | **IDENTITY** — the climber must implement `ILadderClimber` (a capability, not a class) | resolved `ClimbableTower.cpp:812`, passed `:821`, judged `:285-288` |
| **T5** | **TEAM** — `CanTeamAscend(TowerTeam, ClimberTeam)`, the same predicate the nav-link path calls; no hero exemption | `ClimbableTower.cpp:294-297`; team read via `ITeamAgent` at `:807`, `:820` |
| **T6** | **OCCUPANCY** — `IsLadderSlotOccupied()`, one climber at a time | `ClimbableTower.cpp:307-310`, fed from `:822` |

### ⛔ THE ABSENCE, STATED PLAINLY

**There is no term reading a move order, a command group, a `CurrentMoveGoal`, an `EPathFollowingStatus`, an AI state, a destination, or any intent to reach the deck. Not in the pre-gate, not in `WantsToClimb`, not in `EvaluateLadderEntry`.** The full input set of the predicate is: two world points, the pawn's location, the pawn's **velocity**, four floats, two `ETeamId`s, and two bools (is-a-climber, is-ladder-busy).

**This is not an argument from grep-absence** (`VIS-§1`). It is positive, at the caller:

- `ASummonedUnit::TryContactClimbAtNearestLadder` (`SummonedUnit.cpp:3665-3782`) authors **not one float and not one gate**, and says so: *"⛔⛔ AND THIS IS THE ONLY DECISION THIS FUNCTION MAKES. There is ⛔ no team test, ⛔ no eligibility test and ⛔ no proximity THRESHOLD here … ⇒ ⛔ NOT ONE FLOAT IS AUTHORED IN THIS FUNCTION; the nearest tower is asked unconditionally"* (`:3716-3722`). It does exactly two things: pick the nearest tower by 2D distance to the nearer endpoint (`:3723-3751`), and ask (`:3780-3781`).
- The call site is `SummonedUnit.cpp:1472`, and its **placement is the point**: it sits **above** the follow hoist, **above** the profile dispatch, and **above** the sidestep-lease early-out — *"so Standard, Siege, Support, Follow and Hold/Ambush are all covered from here and no ask has to be scattered into the individual bodies"* (`:1456-1462`).
- It rides `StateTimerHandle` at `StateCheckInterval = 0.25f` (`SummonedUnit.h:963`), **not `::Tick`** — `CONTACT-§11.5`'s cadence trap, restated at `SummonedUnit.cpp:3657-3661`.

### ⇒ WHICH UNIT STATES CAN REACH THE POLL (spec item 3, quantified)

**Everything except five conditions.** The only fence above `:1472` is `SummonedUnit.cpp:1427`:

```
if (bDead || !bStatsLoaded || bAIFrozen || bSpellFrozen || IsClimbing()) return;
```

⇒ reachable in **Idle, Advance/marching, Attack, Follow, Hold, Ambush, Siege, Support, mid-sidestep-rescue, and stuck**. A unit with a goal, a unit with none, and a unit mid-attack are all equally exposed. One class-level exemption exists: `AMinerUnit` seals `StateCheckInterval = 0`, so it has no state poll and therefore never asks (`SummonedUnit.h:1684`).

### ✅ BUT A STATIONARY UNIT CANNOT BE TAKEN — the categorically worse finding is REFUTED

**T2's first conjunct is a speed floor and the second normalises velocity.** A pawn standing still has `Size2D() == 0 < 1.0` and a zero heading vector, so `dot == 0 < 0.5` ⇒ `NotHeadingIn`, on every sample, forever. `SiegeLadderClimbStatics.cpp:248-250` states the design intent: *"a pawn standing still and a pawn standing exactly ON the endpoint both yield a dot of 0 … with ⛔ no special case to get wrong."*

Confirmed at law: `CONTACT-§11` (CONVENTIONS:6258) — *"THE PREDICATE READS ⛔ VELOCITY, AND A PAWN PRESSED AGAINST GEOMETRY HAS NONE … ⇒ the trigger fires during the ⛔ APPROACH, ⛔ never while 'leaning on' the ladder."*

⇒ **A unit ordered to hold at a ladder foot is safe. Only a MOVING unit, heading within 60° of the bearing to an endpoint, continuously for 0.35 s, can be taken.** *(Confidence: HIGH — read directly off the shipped conjunction.)*

⚠️ **One residual I am naming rather than asserting.** `MinContactSpeedUU = 1.f` is **0.33 % of a 300 uu/s walk** — an extremely weak floor for "is this pawn actually moving." `bUseRVOAvoidance` is false, so units physically jostle and depenetrate (`ClimbableTower.cpp:300-301`), and depenetration nudges plausibly exceed 1 uu/s. What actually protects a stationary crowd at a ladder foot is **T3's 0.35 s of CONTINUOUS in-cone motion**, not the speed floor — a jostle would have to push a unit toward the same endpoint for 0.35 s unbroken. *(Confidence: MEDIUM. I did not measure jostle velocities; that would need PIE.)*

---

## 2. ⭐⭐ COULD ESCORT/FOLLOW ALONE PRODUCE THIS? — YES, AND THE BOARD'S PREMISE IS REFUTED AT SOURCE

> `VIS-§4a` states: *"the first `HOLD set: 6 unit(s)` banner is at 01:32.7, 12 s later ⇒ ⛔ that ascent was ⛔ NOT a commanded move."*

**⛔ That inference does not hold. The absence of a banner before 01:20.5 is evidence of nothing.**

### (a) Every Blue combat unit spawns ALREADY FOLLOWING — unconditionally

`ASummonedUnit::BeginPlay` calls `TryAutoEnrollInFollowGroup()` (`SummonedUnit.cpp:1311`). That function's own header comment is the default-stance law (`:1320-1329`):

> *"THE DEFAULT-STANCE LAW (CONVENTIONS §2; Jonathan-confirmed …): **every follow-eligible Blue unit spawns FOLLOWING, on EVERY spawn path**, and nothing player-side auto-engages any more … **Enrollment is UNCONDITIONAL**: a unit spawned after the player pressed T still spawns following."*

An Archer is `ECardProfile::Standard` ⇒ `CanFollowHero()` is true (`SummonedUnit.cpp:2152-2160`) ⇒ `IsFollowCommandEligible()` is true for any live, unfrozen Blue unit (`:2171-2179`). **The archer was follow-eligible and therefore enrolled at spawn.**

### (b) That enrollment is SILENT — it prints no banner, by construction

`ASiegePlayerController::EnrollInDefaultFollowGroup` (`SiegePlayerController.cpp:3552` onward) contains **no `BroadcastCommandPrompt` call**. Every banner site in that file is at `:2919`, `:2924`, `:3084`, `:3104`, `:3170`, `:3358`, `:3505`, `:3817` — none inside the enroll function. `:3505` is `"FOLLOW set: %d unit(s)"`, and it lives in the **explicit C-press confirm** path only.

⇒ ⭐⭐ **The 01:32.7 `HOLD set` banner is the first *EXPLICIT* order of the match. It is not the first order. The units carried a standing FOLLOW order from the moment they spawned, and that order never prints.**

### (c) A follow order is a real, moving destination — and after the hero climbed, it pointed AT THE DECK

`ASummonedUnit::UpdateStateFollow` (`SummonedUnit.cpp:1983-2109`):

- anchor = `PC->GetFollowAnchor()` = **the live hero pawn**, re-resolved every call (`SiegePlayerController.cpp:3635-3660`);
- `Station = Anchor->GetActorLocation() + GroupStationOffset` — *"recomputed here every tick precisely because the anchor moves"* (`:2053-2056`);
- arrival tolerance is `HoldArrivalTolerance` **150 uu, 2D** (`:2061`); re-path band is `PC->FollowRepathTolerance` **250 uu** (`:2091-2099`);
- otherwise ⇒ `EnterAdvanceToLocation(Station)` (`:2108`).

When the hero stood on the deck, the station moved onto the deck. `LadderFoot` is actor-local `(−460, 0, 0)` and `LadderTop` `(−160, 0, 1200)` (`TOWER-§8.3`, CONVENTIONS:5364-5365) ⇒ ~300 uu of 2D separation between a follower at the foot and a hero above the top — **past the 150 uu arrival tolerance, so the follower issues a move.**

### (d) And that station is REACHABLE, because the deck is navmesh and the ladder is a nav link

`TOWER-§8.3` (CONVENTIONS:5365) on the `LadderTop` socket: *"⛔ **MUST sit on generated DECK navmesh.** A 600 uu deck loses 64 uu per side to ledge-nulling + erosion ⇒ the surviving poly is X ∈ [−236, +236]."* The link's pathfinding gate passes own-team queriers (`AClimbableTower::ShouldLinkAllowPathfinding`, `ClimbableTower.cpp:315-348`).

### ⇒ ⭐⭐ THERE IS A SECOND, FULLY-ORDERED ENTRY PATH THE ANALYST DID NOT HAVE

`AClimbableTower::HandleLadderLinkReached` (`ClimbableTower.cpp:444-551`) — the nav-link path. It is **not** legacy; `CONTACT-§4.2` row `K-E` ruled both ship, permanently:

> *"⭐⭐ **THEY ANSWER DIFFERENT QUESTIONS AND ⛔ NEITHER SUBSTITUTES FOR THE OTHER: the LINK is how the AI PLANS A ROUTE through the ladder** … **· CONTACT is how a BODY STARTS CLIMBING.**"*

**⇒ Escort-follow alone fully explains the 01:20.5 ascent without invoking the contact trigger at all — and it explains it as a genuinely COMMANDED move:** standing default FOLLOW order → hero moves to deck → station moves to deck → move request → path routes through the ladder link → `HandleLadderLinkReached` → climb. *(Confidence: HIGH on every link in that chain being live and shipped; MEDIUM that it is what actually fired — see §3.)*

---

## 3. ⚖️ WHICH IS IT? — GENUINELY AMBIGUOUS, AND THE AMBIGUITY IS THREE-WAY, NOT TWO

Both mechanisms were armed and both would have fired on these frames:

- **(A) LINK path — ordered.** Follow station on the deck → path through the link → `HandleLadderLinkReached`.
- **(B) CONTACT path — order-blind predicate.** The follower's own approach to the ladder foot satisfies proximity + intent + dwell on the way in.

### ⭐ The dichotomy `VIS-§4a` poses is partly a FALSE one for *this* event

Reading (i) "benign escort-follow" and reading (ii) "incidental grab" are framed as alternatives. **For 01:20.5 they share a cause.** In both, the archer was standing in the cone *because a follow order had walked it there behind the hero*. So even a contact-path admission at 01:20.5 is **escort-caused, not incidental** — the "grab" would have taken a unit that was heading for that exact spot under orders. ⇒ **VID-004's 01:20.5 is not evidence for the abduction defect under either reading.**

### ⚠️ One timing observation that leans toward (B) — declared a HYPOTHESIS (`SC-§20`), ⛔ not a measurement

- Hero and unit climb at **the one shipped rate, 350 uu/s** — `SummonedUnit.h:1132`; the hero deliberately owns no rate of its own and resolves that same value (`HeroCharacter.h:1002-1007`: *"⛔ THERE IS DELIBERATELY ⛔ NO `LadderClimbSpeedUU`"*). 1200 uu / 350 ≈ **3.43 s** per ascent.
- The archer broke the deck plane at **01:20.5** ⇒ its climb began ≈ **01:17.1**. `VIS-§5` records the hero **still mid-ladder at 01:17.0**. Occupancy is strictly one-at-a-time (`ClimbableTower.cpp:307-310`) ⇒ the archer entered essentially **the instant** the hero released the slot.
- ⭐ **Instant re-entry on release is a signature of the CONTACT path specifically.** `SiegeLadderClimbStatics.cpp:311-313`: *"⛔ THE DWELL IS **NOT** CLEARED HERE … so a pawn the caller refuses (a busy ladder) **enters the moment the ladder frees**."* A LINK-path agent refused as `LadderBusy` is instead **handed back to path following** (`ClimbableTower.cpp:481-497`) and must re-path and re-approach — which is not instant.
- ⛔ **CAVEATS THAT KEEP THIS A PREDICTION.** The 01:17.1 start is reconstructed from a nominal 350 uu/s over a nominal 1200 uu, ignoring `TOWER-§8.5a`'s capsule lift (which lengthens the line) and the analyst's declared **0.4–0.5 s frame sampling with no `--run` burst**. `CONTACT-§13`'s closing clause binds me as it binds the manager: **a reconstruction is a PREDICTION, never the acceptance.** *(Confidence: LOW-MEDIUM.)*

### ⭐ THE MEASUREMENT THAT SEPARATES THEM — cheap, zero-code, already in the shipped log

The two paths log **asymmetrically**, which is accidental but decisive:

| Path | On successful start | Cite |
|---|---|---|
| **CONTACT** | `"AClimbableTower '%s': CONTACT climb started for '%s' (ascending)."` | `ClimbableTower.cpp:884-886` |
| **LINK** | ⛔ **nothing** — only a refusal (`:494-496`) or a declined `BeginLadderClimb` (`:547-549`) log | `ClimbableTower.cpp:444-551` |
| both | `"climb ended for '%s' (reached top: …)"` | `ClimbableTower.cpp:570-572` |

⇒ **Run PIE on `L_Arena` with a Watch Tower placed and `LogGitClaudeUnrealTest` at Verbose. Per unit ascent:**
- a `CONTACT climb started` line ⇒ **path (B)**, the order-blind predicate;
- a `climb ended` line for a pawn with **no** preceding `CONTACT climb started` ⇒ **path (A)**, the ordered nav link.

**That settles `VIS-§4a` outright, in one session, with no diff.** Recommend it as a named row in `TASK-802`'s PIE session (the `TASK-775` `Q2(b)` deferral precedent).

⚠️ If the manager wants it airtight rather than inferential, the cheapest possible diff is **one Verbose line** at `ClimbableTower.cpp:529` mirroring the contact one ("LINK climb started"), making both paths self-identifying. ⛔ **I have not written it.** Naming an option is not taking it — that is a manager decision.

---

## 4. 🧑 IF IT IS UNCOMMANDED-BY-DESIGN — IS THAT WANTED? **JONATHAN'S CALL. ⛔ I DO NOT RULE IT.**

Both sides, with the cost stated.

### ✅ FOR — it is a literal transcription of his sentence

Jonathan asked for *"any units can climb the ladder by simply walking up to it and walking against it."* The three terms are that sentence, word for word: **walk** (T2 speed floor), **toward it** (T2 cone), **keep doing it** (T3 dwell). `CONTACT-§4.2` row `K-A` already ruled **AUTO — no key, no prompt, no UI** on those exact words. **An order term would break the sentence**: a unit walking at a ladder with no order would stand there doing nothing, which is the behaviour he described as wrong.

⭐ And it is narrower than "walks close enough while facing the ladder" sounds. T2 measures the bearing **pawn → endpoint**, which **swings** as a pawn passes — `CONTACT-§11.4` (CONVENTIONS:6240) gives the closed form `(√(R²−d²) − d/tan 60°)/v`: **0.500 s head-on vs 0.180 s at d = 100 uu.** The 0.35 s dwell sits **between** them by construction. ⇒ a unit walking *past* is refused; only a unit walking *at* it is taken. That is a real discrimination, not a hope.

### ⛔ AGAINST — the cost, measured rather than feared

The same permissiveness **is** the abduction window: at `v = 300` the half-window is **±247.2 uu** at `R = 350` (`CONTACT-§7b`, CONVENTIONS:6122 — measured by TASK-786; ⛔ super-linear, so a `0.4·R` model understates it at every radius).

⇒ **Any own-team unit whose march line passes within ±247 uu of a ladder endpoint, heading roughly toward it, is taken — with no order, no consent, and no player awareness.** And `VIS-§4`.3 (CONVENTIONS:6425) records that this is **largely unobservable in VID-004**, because from 01:16 the camera is locked to the hero atop the tower: *"an abduction could have occurred repeatedly and left ⛔ no pixel."*

### ⚖️ THE HONEST FRAMING FOR HIS DECISION

**The feature and the defect are ONE PREDICATE. They cannot be separated by tuning — only traded.** The lever is already named in `CONTACT-§7b`(3): dwell `0.35 → 0.50` narrows the grab window at **zero radius cost, for units only**. It is ⛔ **not** free for the hero — the sprint + Swift Boots case already needs `R ≥ 328.125` against a 0.35 dwell, and raising the dwell raises that floor.

⇒ ⭐ **A per-class dwell (unit 0.50, hero 0.35) is the shape that buys the narrowing without costing him his sprint climb.** ⛔ I am not proposing it as a task and have written no code for it — it is a manager/Jonathan decision made from these numbers.

---

## 5. ⭐ VERDICT

> **"Uncommanded-by-design" is TRUE of the PREDICATE and FALSE of THIS EVENT.**

1. **The predicate requires no order.** Five terms — proximity, intent (speed + 60° cone), dwell, identity, team, occupancy — and **not one of them reads a command, a goal, a path, or a destination.** Confirmed at source, positively, at both the predicate and its caller. *(Confidence: HIGH.)*
2. **A stationary unit cannot be taken.** T2's velocity conjunct refuses it on every sample. The categorically worse version of this finding is **refuted**. *(Confidence: HIGH.)*
3. **The 01:20.5 archer was nevertheless under a standing order** — the unconditional, **silent**, spawn-default FOLLOW stance — and a second, fully-ordered entry path (the nav link to a deck station) was live and sufficient. **The banner timeline cannot see the default stance, so it proves nothing about whether the move was commanded.** *(Confidence: HIGH that the chain is live; MEDIUM that it is what fired.)*
4. ⇒ **VID-004 @ 01:20.5 is NOT evidence of the abduction defect**, under either of the analyst's two readings, because the follow order is what put the archer in the cone.
5. **The abduction defect remains real and remains UNOBSERVED.** ±247.2 uu is measured; VID-004 could not have seen it. It needs `TASK-798`'s own measurement, ⛔ not this frame.

### ⛔ WHAT I COULD NOT DETERMINE FROM SOURCE, SAID PLAINLY (`SC-§20`)

**Which of the two entry paths actually admitted the archer at 01:20.5.** Both were armed; both are consistent with every frame; the timing leans contact-path but rests on a reconstruction I am forbidden to cite as a measurement. **§3's Verbose-log discriminator settles it in one PIE session and requires no code.** A deferred answer with a named instrument is worth more than a guessed one.

---

## FOR THE MANAGER — what is actionable here

- ⚠️ **`VIS-§4a` needs amending.** Its stated inference (*"first HOLD banner at 01:32.7 ⇒ that ascent was NOT a commanded move"*) is **refuted at source** by the silent spawn-default FOLLOW stance (`SummonedUnit.cpp:1320-1329` + no banner in `EnrollInDefaultFollowGroup`). The law should record that **command-state is not observable from banners**, so no future footage row may infer "uncommanded" from banner absence.
- ⭐ **A third entry path belongs in the record.** `VIS-§4a` names two readings; there are **three** mechanisms, and the nav-link path (`ClimbableTower.cpp:444-551`, ruled permanent by `CONTACT-§4.2 K-E`) is the one missing. It is the *benign, ordered* explanation and it was never on the list.
- ▶ **One PIE row for `TASK-802`:** Verbose `LogGitClaudeUnrealTest`, place a tower, record whether each unit ascent carries a `CONTACT climb started` line. Zero code, decisive.
- 🧑 **One question for Jonathan** (`§4`): keep the order-blind predicate (it is his sentence, literally) and accept a ±247 uu accidental-grab corridor, or buy the narrowing with a **per-class dwell (unit 0.50 / hero 0.35)**. ⛔ Not my ruling.
- 🧑 **A second, smaller question** surfaced incidentally: the same default-stance law means **units follow the hero up the tower by design**. If Jonathan does not want his whole squad on a 600 uu deck whenever he climbs, that is a *follow-behaviour* decision, ⛔ not a ladder one — and it would be resolved in `UpdateStateFollow`, not in the contact predicate.

**Fences honoured:** ⛔ no source written · ⛔ no test written (suite stays **279**) · ⛔ no compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ no `CONVENTIONS.md` edit. This file is the only diff.
