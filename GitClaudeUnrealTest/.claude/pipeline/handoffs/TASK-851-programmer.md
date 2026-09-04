# TASK-851 — THE BOT'S THREAT READ HONOURS THE VEIL — programmer handoff

**Status:** `ready-for-qa` · **Gate:** `TASK-849` · **Law:** `WITCH-§8` (⚠️ **amended by measurement — see §3**), `WITCH-§1`/`§2`/`§6`/`§7`, `J-W2`/`J-W10`, `SC-§38`, `SC-§39`, `TL-§5b`/`§5c`
**Blocker consumed:** `TASK-829` (the flag, the break door, and the `IsAgentVisibleTo` seam)
**Compile / editor / MCP / Git:** ⛔ none touched. The editor was live under two art tasks for this whole task.

---

## 1. THE ANSWER TO THE QUESTION I WAS ASKED, FIRST

> *"Say plainly how many scans you changed and whether three was the right number."*

**I changed THREE scans. ⛔ But "three" was not the right number, and it is not the same three.**

`WITCH-§8`'s table lists **three scans across two functions** and boards **two** of them for suppression. I re-measured by symbol (`SC-§38`) and found **FIVE unit-perception scans across FOUR functions**. The corrected census:

| # | function | iterator | category | ruling | shipped |
|---|---|---|---|---|---|
| 1 | `FindNearestEnemyIntruderOnBotHalf` — units arm | `ASummonedUnit` | **threat read** (rule 1 DEFEND) | suppress | ✅ consult |
| 2 | `FindNearestEnemyIntruderOnBotHalf` — hero arm | `AHeroCharacter` | threat read | **leave** — `J-W10` | 💬 named comment, ⛔ no code |
| 3 | `FindFireballClusterTarget` | `ASummonedUnit` | **aiming** (rule 3a) | suppress | ✅ consult |
| **4** | ⭐⭐ **`FindLightningTowerTarget`** | `ASummonedUnit` | **aiming** (rule 3b) | **suppress** | ✅ consult — **⛔ NOT IN `WITCH-§8`'s TABLE** |
| **5** | `IsBotHalfPointClear` | `ASummonedUnit`, **both teams** | **physical occupancy** | **leave** — `J-W2` | 💬 named comment, ⛔ no code |

⇒ **two of the three I changed were the boarded ones; the third is a site the law did not know about.** Non-unit iterators in the file (`ATower` ×1, `ACastle` ×2, `ACaptureZone` ×2, `ABuilding` ×1) are untouched — none of those classes is veilable (`WITCH-§6`, `WITCH-§7`).

---

## 2. WHAT A PLAYER WILL ACTUALLY OBSERVE

**A witch veils a friendly push mid-approach on the bot's half. From that moment:**

- **The bot stops answering the push.** Rule 1 (DEFEND) no longer fires on those units — no tower placed between them and Castle_Red, no blocking wave spawned castle-front. The bot banks gold and falls through to its economy / attack rules as if the half were empty. *(If the player's **hero** is also on the bot half, the bot **still defends** — that is `J-W10` working, not a leak.)*
- **The bot cannot Fireball them.** Rule 3a's cluster snapshot never contains them, so three veiled units are not a cluster and cannot pad one centred on a visible unit.
- **The bot cannot Lightning them.** Rule 3b no longer counts them as "units adjacent to a tower", so a player tower ringed by veiled defenders does not qualify as a bolt target.
- **⛔ BUT THEY ARE NOT SAFE, AND THAT IS THE DESIGN.** *Choosing where to throw a Fireball is an act of seeing; the explosion is not* (`J-W2`). A Fireball the bot aimed at a **visible** unit standing beside them **still catches them** — `ApplyRadialDamage` asks its gather for `IncludeVeiled` and I did not touch it. Likewise anything the bot already engaged keeps swinging: **hidden ≠ invulnerable**; the veil removes them from *acquisition*, not from the world.
- **They still block the bot's spawn ring** (scan 5). A veiled body is still a body.

**The one-line version for Jonathan:** *the bot walks past your veiled push and spends its gold somewhere else — until something it aimed at a unit it could see happens to land on top of them.*

---

## 3. ⭐⭐ THE FINDING — `FindLightningTowerTarget`, AND WHY IT IS THE WORSE HALF OF THE LEAK

**⚠️ THIS IS THE ONE SCOPE JUDGEMENT IN THE PASS AND IT IS FLAGGED FOR QA BY NAME** (precedent: `TASK-828`'s `GatherFriendlyAgents`, flagged the same way and gated PASS).

`WITCH-§8` was written from a census of **two** functions. There is a third: rule **3b** (Lightning) calls `FindLightningTowerTarget`, which snapshots enemy unit locations with an iterator whose own comment says *"same unit semantics as the Fireball scan"* — and then counts them around each enemy tower to pick where the bolt goes.

**Three reasons I suppressed it rather than only reporting it:**

1. **It is the same category as scan 3, not a new one.** Counting units to choose a cast point *is* the act of seeing `J-W2` names. The sentence that justifies suppressing the Fireball cluster is the identical sentence, applied five lines apart in shape.
2. ⭐ **It was the worse half.** Lightning **resolves** through `SpellLibrary`'s gather, which is **already** veil-suppressed (`FOG-§7` row 3). So an unsuppressed count here would have had the bot spend **40 gold** aiming a bolt at units the resolver **cannot damage** — detecting them *and* whiffing on them. Fireball at least connects; this one was pure loss.
3. **Leaving it ships a bot that honours the veil when it aims one spell and detects veiled units when it aims the other, from the same rule, in the same file.** That reads as "invisibility is broken", which is exactly the outcome `WITCH-§8` overturned the deferral to prevent.

⛔ **It is inside the fence** — `SiegeBotController.cpp` is mine SOLE, and the fence in the spec is a **file** fence, not a function fence.
⚖️ **If the manager/QA rules it out of scope anyway, the revert is 2 lines of code + 1 comment block + 1 test row** — but the bot then ships with a measured, documented hole in the card, and someone must board `TASK-8xx` for it.

⭐ **THE STANDING LESSON, AND it is `WITCH-§8`'s OWN closing line turned back on itself:** *an acquisition surface is sized by **who enumerates units**, never by one API's grep hits* — **or by a count somebody else took**, including when that count is written into law. `WITCH-§1` counted `GetAllActorsWithInterface` and missed the bot; `WITCH-§8` counted the bot's two known functions and missed its third. **Same error, one level down, inside the very section that named it.** Suggested `CONVENTIONS` amendment: add the row to `WITCH-§8`'s table and mark the table *measured 2026-09-03 by TASK-851, 5 scans / 4 functions*.

---

## 4. FILES TOUCHED

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp` | **1** include · **3** veil consults · **2** named-omission comment blocks · 2 stale inline comments corrected (`"not enough player units"` → `"not enough VISIBLE player units"`) |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeInvisibilityTest.cpp` | **+3 tests** (19 → 22) + one fixture path constant (`BotControllerCpp`) |

⛔ **`SiegeBotController.h` was NOT touched** — no signature changed, no member added, no tunable added. The spec's `names:` line permits the header; nothing needed it.
⛔ **Not touched, deliberately:** `SummonedUnit.*` · `SiegeCombatStatics.*` · `SiegeFogStatics.*` (**`TASK-838`'s live lane** — I read `SiegeCombatStatics.cpp` and wrote nothing to it) · `SpellLibrary.cpp` · `Tests/SiegeAcquisitionFunnelTest.cpp` · `CONVENTIONS.md`.

---

## 5. ⚠️ THE `names:` LINE IS STALE AND OBEYING IT WOULD HAVE TURNED THE SUITE RED (`SC-§38`, again)

`TASK-851`'s spec — both §(3) and its `names:` line — says **"call the shipped predicate — `FSiegeInvisibilityStatics::IsVisibleTo(...)`"** with `/*ViewerTeam=*/` / `/*TargetTeam=*/` comments.

⛔ **I did not do that, and doing it would have been a defect.** It is a citation written **before `TASK-829` shipped**. What actually landed is the wrapper `FSiegeCombatStatics::IsAgentVisibleTo(ETeamId, const AActor*)`, and `SiegeInvisibilityTest.cpp`'s own gate asserts:

> `FSiegeInvisibilityStatics::IsVisibleTo(` appears **exactly 2×** across all shipping source (its definition + the one call inside `IsAgentVisibleTo`) — *"⛔ A THIRD hit is a SECOND guard point… ⛔ Do NOT fix a red here by adding a veil check at a new site: … call `IsAgentVisibleTo` (**which is what TASK-851's bot scans do**)."*

⇒ three literal-obedience calls would have made that **5** and turned `AVeiledEnemyIsDroppedByTheOneAcquisitionFunnel` **RED — in order to obey the spec.** Exactly the `FOG-§7` row 4 failure mode `SC-§38` was written for. The wrapper also makes the argument swap **untypeable**, so the named-argument comments the spec asked for are **structurally unnecessary at my call sites** and would have been noise; the symmetric form still exists at exactly one place in the project and still carries them.

---

## 6. THE TWO OMISSIONS, RULED AND NAMED IN SOURCE

**Scan 2 — the hero arm (`J-W10`).** The hero is not veilable. `IsAgentVisibleTo` casts to `ASummonedUnit` and returns `true` for everything else, so a consult there **could never once evaluate false**: dead code that reads as if the hero could be hidden. ⭐ **Test 21 proves this rather than asserting it** — it measures, in the shipped predicate's own body, that the `Cast<ASummonedUnit>` precedes the rule call.

**Scan 5 — the spawn clearance (`J-W2`).** Three independent reasons, all at the site:
1. **Presence is not perception** — the veil hides a unit, it does not make it incorporeal.
2. ⭐ **The loop is NOT team-filtered** — it rejects a point near a live unit of *either* team. `IsAgentVisibleTo` always returns `true` for the viewer's own team, so a consult would suppress **only enemies** and convert a symmetric physics rule into an asymmetric one.
3. ⛔ **It would be an exploit, not a fix** — park a veiled unit in the bot's spawn box and the bot spawns its wave **inside** it, re-opening the identical-XY pile-up `TASK-265` fixed.

⚠️ **Declared residual (real, tiny, not denied):** the bot's ring search silently steps around a veiled body, so a *rejected candidate point* carries a hair of information. It is unobservable to the player — only the chosen point is ever rendered — and closing it costs a physics regression.

---

## 7. TESTS — **DELTA `+3`** (⛔ a delta, never an absolute — `TL-§5b`)

Extended `Tests/SiegeInvisibilityTest.cpp` (**19 → 22 declared in that file**; no new test file, file count stays 29). **Tree at my finish: 392 `IMPLEMENT_` declarations across 29 files.** ⚠️ **Trust the `+3`, not the total** — the brief handed me 389 and other lanes are writing test files concurrently; the residual is theirs, not mine, and `TL-§5c` says report the delta.

| test | fails when |
|---|---|
| `TheBotsThreeAimingScansHonourTheVeil` | any of the three consults is dropped · a **fifth** `TActorIterator<ASummonedUnit>` appears without a ruling · the bot reads the veil **itself** (`bIsInvisible` / `IsInvisible()` / `FSiegeInvisibilityStatics` / `ESiegeVeilPolicy` — all pinned at 0) |
| `TheBotsHeroArmAndSpawnClearanceAreDeliberatelyNotSuppressed` | someone "helpfully" suppresses the hero arm or the spawn clearance · either omission loses its named rationale (`J-W10` / `J-W2`) · ⭐ the shipped predicate stops class-filtering before it asks the rule |
| `TheBotAcquiresWithTheSameRuleAsEveryUnit` | the rule stops hiding cross-team veiled units · **or starts hiding unveiled ones** (the control) · the rule becomes Red-shaped (both teams driven) · the bot reaches into the blast lane |

**⭐ POSITIVE CONTROLS, per `SC-§39` — every zero in this lane is paired with a non-zero taken by the same instrument in the same pass:**
- The file probe asserts `TActorIterator<ASummonedUnit>` **== 4** *before* asserting four different tokens are **0**. A blind scanner fails the first row, so no absence below it can be vacuous.
- The hero-arm proof asserts **both** iterators are inside the extracted body before claiming "one consult for two loops" — otherwise a truncated read would satisfy it.
- The truth table pairs every "hidden" row with an "unveiled ⇒ visible" row; a predicate that answered *hidden* to everything would blind the bot completely and must not pass.

### ⚠️ I MET INSTRUMENT HAZARD #1, IN ITS MIRROR IMAGE — AND IT CHANGED A NEEDLE

`TASK-829` measured that `CountOccurrencesInCode` **skips** lines whose trimmed form starts with `/*`, making a guard **under**-count to zero. My hazard was the same helper **over**-counting:

> the `#include "Siegebound/SiegeCombatStatics.h"` line carries a trailing `//` comment naming `IsAgentVisibleTo`. A **trailing** comment sits on a line whose trimmed form starts with `#include` — **a CODE line.** A bare-token needle therefore reads **4**, not 3.

Measured directly: bare `IsAgentVisibleTo` = **4**; `FSiegeCombatStatics::IsAgentVisibleTo(` = **3**. The shipped needle carries its **open paren** so it counts *calls*, and the test says why beside it. ⚖️ Same family, same lesson: **an instrument's blind spot and its noise are both indistinguishable from a result** — I verified every needle against a positive control before trusting a number, including the ones that came out right.

---

## 8. ⛔ WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐⭐ **The `FindLightningTowerTarget` suppression (§3)** — the one scope judgement. Rule it explicitly, please, rather than letting it pass silently; if it stands, `WITCH-§8`'s table needs the row (a manager docs edit — I did not write to `CONVENTIONS.md`).
2. **Placement inside the two snapshot loops.** I folded the consult into the existing `if (IsValid && !IsUnitDead && GetTeamId == EnemyTeam)` chain with `&&`, so it is **short-circuited** behind the team filter and never runs on friendlies or the dead. Confirm that reading — it is the highest-frequency line I added (it runs per enemy unit per 2 s decision tick, twice).
3. **The early-outs now see the filtered count.** `EnemyLocations.Num() < MinUnits` returns *before* the O(N²) pass in both spell functions, so an all-veiled field costs **less** than before, not more. Confirm that is intended (it is: the bot must not cluster on units it cannot see).
4. **Scan 1's consult sits AFTER the team/dead filter but BEFORE `IsOnOwnHalf`.** Deliberate — the veil check is cheaper than nothing but the ordering is behaviour-neutral (both are pure filters with no side effects). Flag it if you disagree; it is a one-line move.
5. **Compile risks I cannot test** (⛔ no compile in my lane): `SiegeCombatStatics.h` includes only `CoreMinimal` / `SubclassOf` / `TeamId.h` and forward-declares `AActor` — **no cycle** with `SiegeBotController.h`, which I verified by reading. `const ASummonedUnit*` → `const AActor*` at scans 3 and 4 and `ASummonedUnit*` → `const AActor*` at scan 1 are derived-to-base conversions with an added `const`, both implicit.
6. **The stale-comment corrections.** I changed two inline comments from *"not enough player units alive"* to *"not enough **VISIBLE** player units alive"*. Behaviour-free, but they are comment edits inside a diff and you should see them declared rather than find them.
7. **Test 21(c) reads `SiegeCombatStatics.cpp`** (does not write it) to prove the hero-arm omission is dead code. ⚠️ **`TASK-838` is live in that file.** The assertion is structural (`Cast` precedes the rule call inside `IsAgentVisibleTo`) and fails **loudly** via `ExtractFunctionBody` if the signature moves — it cannot silently pass. If `838` reshapes that function, this row is the one to re-read.

---

## 9. ⚠️ DECLARED RESIDUALS

1. **The bot's card-play read is still omniscient under FOG.** `J-F4` stands and is **not** re-litigated here — `TASK-838` is explicitly forbidden this file and I added **zero** fog terms. The asymmetry is deliberate: `WITCH-§8` overturned the *invisibility* half only. ⇒ **any `IsAgentVisibleTo` in `SiegeBotController.cpp` is mine, not `838`'s leakage** — check authorship before reporting a fence break (`TASK-838`'s gate says so by name).
2. **The spawn-clearance information residual** (§6).
3. **Nothing here is playtested.** No `SpawnActor` exists anywhere in `Siegebound/Tests/`, so the wiring is pinned **structurally only**. ⛔ **Green is not "the bot cannot see veiled units"** (`SC-§32`) — that needs a PIE pass with a witch on the field, which cannot exist before `TASK-830`.
4. **`GrantInvisibility()` still has zero callers** (`TASK-829` §4c). Until `TASK-830` lands, **all three of my consults are reachable but can never return false in play.** Stated plainly so nobody reads a quiet playtest as a pass.
