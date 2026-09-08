# TASK-1132 [WARMAP-VEIL] — programmer handoff

**Status: technically complete — ⚖️ AWAITING A 🧑 JONATHAN RULING ON DIRECTION.**
Gate: `TASK-1133`. Host: `TASK-1134` (or `TASK-937` by adoption).
Date: 2026-09-07. Compiler held: **none** — ⛔ NO WITNESSED RED (see §4).

---

## 0. ⚖️⚖️ READ THIS FIRST — THE DIRECTION IS **NOT** SETTLED, AND NO AGENT MAY SETTLE IT

The code below is written. **Whether it should ship is a design question nobody has ruled.**
Stated in both directions, with the balance facts I could actually measure.

### The two readings

| | reading | the case for it |
|---|---|---|
| **A — THE VEIL WINS** (what the diff does; the board's shipping default) | a veiled enemy unit is absent from the paid reveal | 🧑 his own sentence (§0.1) says *enemy players*, not just enemy AI |
| **B — GOLD PIERCES THE VEIL** | the 30-gold reveal is an **intended counter** to invisibility, exactly as AoE is | you pay gold, you get intel; that is counter-play, and reading A deletes it |

### 0.1 ⭐⭐ THE ONE PIECE OF EVIDENCE THAT COMES CLOSEST TO SETTLING IT — 🧑 HIS OWN WORDS

`CONVENTIONS.md:9298`, Jonathan verbatim, quoting his own witch spec:

> *"Invisible units are **completely invisible to enemy AI and enemy players**."*

That sentence names **enemy players** as a separate clause from enemy AI, and the war map is a
player instrument. It is the strongest evidence on the board for reading **A**.

⚠️ **But I will not call it decisive, and here is the honest reason.** His sentence describes what
an invisible unit *is*; it does not enumerate exceptions, and he was not asked about the war map.
It is measurable that the **war map shipped FIRST** (the `WR-§` batch — this test file's own header
pins its Zone-A baseline at **2026-08-05**) and that the **witch sentence came later** (`WITCH-§`
added **2026-09-03**), so he wrote it with the map already in the game — which strengthens A but
does not prove he had the map in mind. ⇒ **evidence, not a ruling.**

### 0.2 THE BALANCE FACTS, MEASURED AT SOURCE (not recalled)

| fact | measured value | where |
|---|---|---|
| Witch card cost | **50 gold**, `MaxCopies 2` | `Docs/Data/cards.csv` row 33 |
| Enemy-reveal cost | **30 gold** | `CommanderNpc.h:311` `EnemyRevealCost = 30` (GDD §3.15) |
| Veil throughput | **ONE unit at a time**, 3 s **interruptible** cast, permanent until the unit acts | `cards.csv` row 33 Notes + his verbatim |
| Witch in the default deck? | **`DeckCount = 0`** — she is **not** in the starter deck | `cards.csv` row 33 |
| Reveal persistence | **one-shot, FROZEN at purchase**; does not track; **REPLACES** on re-purchase (never accumulates); **cleared on map close** | `WarMapWidget.cpp` `ReceiveEnemyReveal` / `ClearEnemyReveal` |
| Reveal friction | gated behind walking to the own-team `ACommanderNpc` | `PerformEnemyReveal` → `FindOwnTeamCommanderNpc` |
| ⭐ Does the reveal already pierce **FOG**? | **YES — zero fog terms in `PerformEnemyReveal`, and zero in all of `WarMapWidget.cpp`** | measured |
| ⭐⭐ Does the veil already have a **ruled, shipped counter**? | **YES** | `WITCH-§2` lane 2 |

### 0.3 THE TWO FACTS THAT ACTUALLY MOVE THE ARGUMENT

**⭐⭐ FOR READING A — the veil is *already* non-auto-win, by a ruled counter, so suppressing the
map does not create an uncounterable card.** `WITCH-§2` lane 2 (🧑 `J-W2`), verbatim:

> *"AoE / `ApplyRadialDamage` — **NOT SUPPRESSED**… a blast is not an act of seeing. It also gives
> the card a **real counter** (Fireball/Sapper/BombTower flush a veiled push) ⇒ the 50-gold card is
> **not an auto-win**."*

⇒ the "reading A removes the only counter-play" worry is **measurably false**: an authored,
ruled, shipped counter already exists and this diff does not touch it. The interrupt (his own
3-second-cast clause) is a second one.

**⛔ FOR READING B — the numbers are lopsided, and this is the sharpest fact against A.**
30 < 50; the reveal is **repeatable** (his own *"pay another 30 gold to reveal the NEW locations"*)
while the witch is capped at 2 copies veiling **one unit at a time** over a 3-second interruptible
cast. Under reading B a player hard-counters a 50-gold investment for 30, repeatedly, with no
interaction — which is the shape of a dominant answer, not a counter.

### 0.4 ⛔ AND THE LAW GENUINELY DOES **NOT** COVER IT — the coordinator is right

`WITCH-§2`'s four ruled lanes are **ACQUISITION · AoE · locked projectiles · FRIENDLY acquisition**.
There is **no INTEL lane and no UI lane.** The war map is neither acquisition nor a blast. ⇒ the
paid reveal is **outside every clause that exists**, and no shipped card text mentions it either
(the Witch's `Notes` column describes the cast and the radius, and says nothing about detection).

**⇒ Nothing in the tree settles this. It needs 🧑 Jonathan.**

### 0.5 ⭐ IT IS ALREADY BOARDED — no new row is needed

**`TASK-1135` (🧑 `J-W18`) item (6) already carries this exact question**, in both directions,
with reading A as its stated proceeding default and reading B described as *"a legitimate design
(gold as counterplay)"*. This handoff supplies the balance facts that row was missing.

**If he rules B, it is a ONE-LINE INVERSION at this exact site** — negate the term in the guard at
`SiegePlayerController.cpp:7146`. ⛔ **I built no toggle**, per item (8).

⚠️ **Second-order consequence if he rules A, named because it is easy to miss:** the reveal
already pierces **fog** (§0.2). Ruling A makes the war map honour one concealment system and not
the other, which is defensible (fog is weather, the veil is a card) but is now a **visible
inconsistency** rather than a latent one. ⛔ Out of scope here; flagged for `TASK-1135`.

---

## 1. THE GUARD — `file:line`

**`Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp:7145-7146`**, inside
`ASiegePlayerController::PerformEnemyReveal()`, folded into the unit survey's **existing**
`continue` guard — ⛔ no new branch, ⛔ no new predicate, ⛔ no new local:

```cpp
if (!IsValid(Unit) || Unit->IsUnitDead() || Unit->GetTeamId() != EnemyTeam
    || !FSiegeCombatStatics::IsAgentVisibleTo(OwnTeam, Unit))
{
    continue;
}
```

**The veil term is LAST on purpose:** `||` short-circuits, so the three cheap filters run first and
the predicate is only ever asked about a **live enemy** unit — the only case where it can answer
anything but `true`.

### 1.1 The two other edits in the same file, both required, both named

| edit | line | why it is not scope creep |
|---|---|---|
| `#include "Siegebound/SiegeCombatStatics.h"` | `:39` | **measured: the header was not included.** The row said "one line"; an IWYU include is the cost of the call. Placed in the existing alphabetical `Siegebound/` block (`SiegeCheatManager` → **`SiegeCombatStatics`** → `SiegeControlsHelpWidget`). ⚠️ Its trailing comment deliberately writes `FSiegeCombatStatics` **without** the `IsAgentVisibleTo(` token, so it cannot be miscounted by any qualified-needle probe — the exact trap `SiegeInvisibilityTest.cpp:1853` documents. |
| the block comment above the loop | `:7101-7131` | **required by item (5)** — my diff makes the old *"mirrors `RefreshAllyDots` **exactly**"* sentence **false**. **Corrected, never deleted.** |

**The comment correction** names the asymmetry, names why it is one-sided (own-team is always
visible ⇒ a consult in `RefreshAllyDots` would be dead code implying a player can lose sight of his
own army), names why the hero loop is exempt (`J-W10`/`WITCH-§6`: `ASummonedUnit` is the only
veilable class), carries the 🧑 `TASK-1135` consequence — and is **falsifiable in both directions**:
it claims *exactly ONE consult here and exactly ZERO in `RefreshAllyDots`*, and **test 36 asserts
both numbers.** ⇒ the prose cannot rot without a red.

---

## 2. ⭐⭐ THE SEAM — RULING **UPHELD**, AND I FOUND A **FOURTH** GROUND THE MANAGER DID NOT HAVE

Used **`FSiegeCombatStatics::IsAgentVisibleTo(OwnTeam, Unit)`** — the shipped predicate.
⛔ **NOT** `TASK-931`'s `IsAgentVisibleToLocalViewer` adapter. The row's three grounds all verified:

- **(a) confirmed** — `const ETeamId OwnTeam = SiegeState->GetTeam();` sits at `:7086`, **eighteen
  lines** above the loop as stated. Calling the adapter would discard a known value and re-derive
  it from the world.
- **(b) confirmed** — the adapter resolves the **local** viewer; this is a controller method already
  serving **this** controller's player.
- **(c) confirmed** — `IsAgentVisibleTo` is in `HEAD`; the adapter is uncommitted. ⇒ **this row has
  no dependency on `TASK-931` / `TASK-937`.**

### ⭐⭐ (d) THE NEW, MEASURED GROUND — THE ADAPTER SEAM WOULD HAVE TURNED `TASK-931`'s **OWN PIN** RED

`Tests/SiegeInvisibilityTest.cpp:3583` (test 34, `TASK-931`'s, uncommitted) pins
**tree-wide `FSiegeCombatStatics::IsAgentVisibleToLocalViewer(` == 3**, and its own failure message
reads: *"⛔ A FOURTH hit is a render surface nobody ruled in writing — board it and rule it,
⛔ **never bump this number**."*

**Measured now, with the diff in the tree: the tree-wide adapter count is 3.** Had I taken the
adapter seam it would read **4** and go RED — a QA blocker that neither the manager nor I could
have predicted from the row alone. ⇒ **the ruling was not merely reasonable, it was necessary.**

⛔ **Nothing contradicts the ruling. I did not switch seams and had no reason to.**

---

## 3. THE TEST — `Tests/SiegeWarMapTest.cpp`, **test 36**

`Siegebound.WarMap.ThePaidEnemyRevealHonoursTheVeil`
(`FSiegeWarMapEnemyRevealHonoursTheVeilTest`), appended at the end of the existing file.
⛔ No new file. ⛔ `Tests/SiegeInvisibilityTest.cpp` byte-untouched by me.

**Mechanism:** source-text structural probes over `PerformEnemyReveal` and `RefreshAllyDots`, using
the house helpers (`LoadProjectSource` / `CountOccurrencesInCode` / `ExtractFunctionBody`) copied
**verbatim** from `SiegeFogRefusalTest.cpp`, plus a `TextBefore` ordering helper. Two new includes
(`Misc/FileHelper.h`, `Misc/Paths.h`).

### 3.1 ⛔ `SC-§79` — WHAT THIS TEST CAN AND CANNOT DETECT, STATED PLAINLY

| ✅ **CAN detect** | ⛔ **CANNOT detect** |
|---|---|
| the consult **absent** | that a **real** veiled unit in a **real** match is really absent from a **real** `EnemyWorldXY` |
| the consult in the **wrong loop** (hero not unit) | |
| the consult **after** the `Emplace` and therefore unreachable | |
| the arguments **SWAPPED** (`EnemyTeam` — always true ⇒ a silently dead guard) | |
| a **second, hand-rolled** veil rule at this site | |
| the ally survey **growing** a consult it must never have | |

⚠️ **The end-to-end behavioural claim is NOT executed, and is not pretended to be** (`SC-§32`).
`PerformEnemyReveal` needs authority, a world, an `ASiegePlayerState`, an own-team `ACommanderNpc`
and gold, and there is **not one `SpawnActor` or `UWorld::CreateWorld` anywhere under
`Siegebound/Tests/`** — a house rule this file's own header already states for the whole gold lane.
⇒ **it belongs to a PIE row / Jonathan's playtest, and I have not claimed it.**

**What IS proved is a REACHABILITY claim over the shipped source** — row (d) below — which is the
strongest available form of *"a veiled enemy cannot reach `EnemyWorldXY`"* in a headless test.

### 3.2 The rows

| row | assertion | class |
|---|---|---|
| (a) | **self-check/census** — exactly **2** `EnemyWorldXY.Emplace(` (unit + hero) | positive control; a 3 = a new position source nobody ruled |
| **(b)** | **exactly ONE** `FSiegeCombatStatics::IsAgentVisibleTo(` in the body | ⭐ **the authored guard** |
| **(c)** | it asks `IsAgentVisibleTo(OwnTeam,` **once** and `IsAgentVisibleTo(EnemyTeam,` **zero** times | ⭐ **the argument SWAP** |
| **(d)** | before the **first** `Emplace`: **1** consult **and 1** `continue;` | ⭐⭐ **REACHABILITY — the row that is about the array, not about a flag** |
| (e) | after the first `Emplace`: **zero** consults | the hero loop's exemption made **decided** rather than merely absent |
| (f) | `bIsInvisible` · `IsInvisible()` · `FSiegeInvisibilityStatics` · `ESiegeVeilPolicy` · `MI_Unit_Invisible` all **zero** | item (3): ⛔ ZERO NEW HIDES |
| (g) | **1** `SpendGold(`, **1** `ClientReceiveEnemyReveal(` | item (4c): the gold lane's shape. ⚠️ **named as WEAK** — inherited behaviour, `SC-§83`'s easier half |
| (h) | `RefreshAllyDots`: **2** `AllyDotsWorldXY.Emplace(` (control) and **zero** consults | the other half of the asymmetry, pinned **over the second file's own text** |

### 3.3 ⭐ THE NAMED MUTATIONS (`SC-§83` — aimed at the **authored** guard, not an inherited pin)

- **`MUT-1132-A`** — **delete `|| !FSiegeCombatStatics::IsAgentVisibleTo(OwnTeam, Unit)`** from the
  unit loop's `continue` guard (i.e. restore the pre-fix filter exactly).
- **`MUT-1132-B`** — **swap the argument**: `IsAgentVisibleTo(OwnTeam, Unit)` →
  `IsAgentVisibleTo(EnemyTeam, Unit)` (the *silently dead guard* — compiles, reviews clean, leaks).
- **`MUT-1132-C`** *(described, not simulated)* — move the consult **below** the `Emplace`.
  Row (d1) is the row that catches it.

---

## 4. 🚨 **NO WITNESSED RED** — and exactly what I *did* measure instead

**NO WITNESSED RED.** **NOT MEASURED** by a test runner. ⛔ I hold no compiler, the editor holds the
DLL, and I did not compile (row item (9)). **The RED obligation passes to `TASK-1134`** (or to
`TASK-937` on adoption): apply `MUT-1132-A`, paste the verbatim red, **revert**, re-run green.
**A green-by-declaration is not a red.**

⚠️ **What I did do is a genuine measurement and must not be read as a substitute for the red:** I
re-implemented the shipped `CountOccurrencesInCode` / `ExtractFunctionBody` **faithfully** (same
comment-line skip set, same first-column-`}` body end) and ran it against the **real files**.

- **All 16 asserted counts: GREEN. 0 mismatches.**
- **`MUT-1132-A` ⇒ 3 of 7 structural rows RED** — (b) 1→0, (c1) 1→0, (d1) 1→0.
- **`MUT-1132-B` ⇒ 2 of 7 structural rows RED** — (c1) 1→0, (c2) 0→1.

⇒ **the pin is not vacuous** — but that is *my replica* of the counter agreeing with itself, ⛔ not
the automation runner. `TASK-1134` still owes the real transcript.

**`TL-§5c` intended delta: +1 test.** Executed baseline **498 / 0** ⇒ expected **499 / 0**.
(⛔ The long-quoted `306` is a stale static macro census — not quoted here.)

---

## 5. ⭐ THE NINE-PIN VERDICT — RE-MEASURED MYSELF, ALL QUIET, **ZERO EDITED**

I re-measured **every** pin on the symbol against my diff rather than assuming. **13 hits found**
(the row said nine; the extra four are the two `ExtractFunctionBody` *anchors* and `TASK-931`'s two
new adapter pins, which did not exist when the row was written):

| pin | scope | pinned | measured |
|---|---|---|---|
| P1 `SiegeFogClampTest:877` | `GatherHostileAgents` body | 1 | **1 QUIET** |
| P2 `SiegeInvisibilityTest:1453` | `GatherHostileAgents` body | 1 | **1 QUIET** |
| P3 `:1493` | anchor on the predicate's own definition | found | **found QUIET** |
| P4 `:1638` | `GatherFriendlyAgents` body | 0 | **0 QUIET** |
| P5 `:1862` | **file-scoped `SiegeBotController.cpp`** | 3 | **3 QUIET** |
| P6-P8 `:1901` (loop ×3) | the three bot aiming bodies | 1 each | **1/1/1 QUIET** |
| P9 `:1976` | `FindNearestEnemyIntruderOnBotHalf` | 1 | **1 QUIET** |
| P10 `:2002` | spawn-clearance body | 0 | **QUIET — file untouched** |
| P11 `:2017` | anchor on the predicate definition | found | **found QUIET** |
| P12 `:3535` | adapter body (`TASK-931`) | 1 | **1 QUIET** |
| **P13 `:3583`** | **tree-wide `IsAgentVisibleToLocalViewer(`** | **3** | **3 QUIET** — see §2(d) |

**⇒ 0 pins red. 0 pins edited.** The reason is structural and worth stating: **every pin on this
symbol is scoped to `SiegeCombatStatics.cpp` or `SiegeBotController.cpp`, and my diff touches
neither.** ⭐ **CONTROL, measured: there is NO tree-wide pin on `FSiegeCombatStatics::IsAgentVisibleTo(`
at all** — its tree-wide count moved **5 → 6** with my call and **nothing asserts that number**.
(Contrast P13, which *is* tree-wide — which is precisely why the adapter seam would have failed.)

---

## 6. ⭐ WHAT ELSE THE SURVEY PUBLISHES — asked, measured, answered: **NOTHING the one `continue` misses**

| surface | leaks a veiled unit? | why |
|---|---|---|
| **position** | ✅ **FIXED** by this guard | the defect |
| **count** — the purchase log, the truncation warning, and the widget's `OnWarMapRevealStateChanged(bool, int32)` BP event | ✅ **covered, no widening needed** | all three are **derived from `EnemyWorldXY.Num()`**; a unit that never enters the array cannot be counted |
| **icon / type / team / id / health** | ✅ **impossible by construction** | the payload is `TArray<FVector2D>` — bare `(X, Y)`. It cannot carry a class, a team or an id, so it cannot leak *which* unit |
| **ping / mark** | ✅ none | map marks are player-authored (`TASK-744`), not derived from enemy actors |
| **ordering / inference** | ✅ none | positionally anonymous; a removed dot is indistinguishable from a dead unit — which favours the veil |
| **the enemy HERO's dot** | ⛔ still published, **correctly** | `J-W10`: the hero is not veilable. Same array, deliberate |

**⇒ ONE `continue` covers the whole surface. I did not widen the diff.**

### 6.1 The adjacent surveys — census of every `TActorIterator<ASummonedUnit>` in the file

Measured: **three** in `SiegePlayerController.cpp`.

| site | what it is | verdict |
|---|---|---|
| `:7134` | **the enemy reveal** | ⭐ **mine — fixed** |
| `:3892` | group-pick selection (`IsFollowCommandEligible` / `IsGroupCommandEligible`) | ✅ **clean** — own-team only; its own comment records that it *"excludes… every Red/bot unit"*. Not an enemy-facing read |
| `:5911` | placement obstacle clearance | ✅ **clean, and must stay so** — filters `GetTeamId() != OwnTeam`, **and** it is the **physical spawn-clearance** class that `WITCH-§8` deliberately does **not** suppress (pinned at `SiegeInvisibilityTest.cpp:2002` == 0) |

**⇒ the enemy-reveal survey was the ONLY enemy-facing unit survey in this file.** There is no
second leak of this class here.

`UWarMapWidget::RefreshAllyDots` (`:1779`) — verified `!= LocalTeam`, **own team only** ⇒ a consult
there is dead code. Item (4)(b) **holds, measured, not assumed.**

---

## 7. ⛔ FLAGGED DECISIONS FOR GATE `TASK-1133`

1. ⚖️⚖️ **THE DIRECTION IS UNRULED (§0).** The diff implements reading **A**. If the gate believes
   reading B, that is a 🧑 Jonathan question routed to `TASK-1135` — ⛔ **not a code finding**, and
   the code is correct under A either way. **Please do not fail the row over the direction.**
2. **The diff is 3 edits, not 1**, in the one named file: the guard, a **required** IWYU include,
   and the **mandatory** comment correction (item (5)). Named so their presence is not read as creep.
3. **NO WITNESSED RED** (§4). The red is `TASK-1134`'s. My replica simulation is offered as
   evidence the pin is **not vacuous**, ⛔ never as the transcript.
4. **The behavioural end-to-end is NOT covered** and is declared, not hidden (§3.1). If the gate
   wants it, it is a **PIE row** and needs boarding — this file cannot host it (house rule: no
   `SpawnActor` under `Siegebound/Tests/`).
5. **Row (g) is deliberately weak** and says so in place — a shape pin on inherited behaviour
   (`SC-§83`'s easier half), kept only because the gate checks the diff in both directions.
6. **Test numbering**: the file had 35 tests; mine is **36**, and the shipped comment at
   `SiegePlayerController.cpp:7124` cites *"Test 36"* by number. A future insertion that renumbers
   would make that citation stale. Accepted knowingly — the test's **name**
   (`ThePaidEnemyRevealHonoursTheVeil`) is the durable handle.
7. **Untouched, deliberately** (item 4): the **hero loop**, `RefreshAllyDots`, all gold/RPC/widget/
   `FSiegeWarMapProjection` code, and **all four of `TASK-931`'s files**.
8. **Coexistence**: `TASK-931`'s diff is uncommitted in the same tree. **Measured `git status`:
   zero file overlap** — mine is `SiegePlayerController.cpp` + `Tests/SiegeWarMapTest.cpp`; its is
   `SummonedUnit.cpp`, `CombatantHealthBarComponent.{h,cpp}`, `SiegeCombatStatics.{h,cpp}`,
   `Tests/SiegeInvisibilityTest.cpp`. ⚠️ **Also in the tree and MINE NEITHER:** untracked
   `SiegeGraphicsSettingsSubsystem.{h,cpp}` + `Tests/SiegeGraphicsSettingsTest.cpp` — named under
   `§25b` cl. R and **left**.
9. **`blocked-by` stays NONE** — the ruling held, so no dependency on `TASK-931` was taken (§2).

---

## 8. FILES TOUCHED

- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` — include `:39`; comment
  correction `:7101-7131`; **the guard `:7145-7146`**
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp` — 2 includes; fixture
  `SiegeWarMapVeilTestFixture`; **test 36**
- `.claude/pipeline/handoffs/TASK-1132-programmer.md` — this file

⛔ Zero `Content/`. ⛔ Zero editor. ⛔ Zero MCP. ⛔ Zero Git. ⛔ Zero compile.
