# TASK-267 handoff — gameplay-programmer

**Branch:** `m7.6-arena10x`
**Status:** ready-for-qa
**Scope honored:** `SiegeBotController.{h,cpp}` ONLY. No compile, no Git (folds into the next SiegeBotController-touching build-master integration per the board's `integration:` note).

## What changed

### 1. Rule-2 fall-through (the core fix)
The unconditional `return`s that formerly abandoned the decision tick on a rule-2a (Miner) / rule-2b (Deep Mine) spawn failure are GONE. Control flow is now:

- **On a CONFIRMED play** (`ComputeValidBotSpawnPoint` succeeded AND `SpawnBotCardActor` returned non-null AND `ConfirmPlayFromHand` ran): `return` inside the `if (SpawnBotCardActor(...))` success branch — the rule fired and owns the tick, exactly as before.
- **On ANY failure** (no valid spawn point, OR `SpawnBotCardActor` returned null): NO return — falls through to 2b, then rules 3/4/5 in the same tick.

This kills the permanent re-stall: the old structure left the Miner in hand, `AliveMinerCount` at 0, rule 2's precondition satisfied, and rules 3/4/5 unreachable for the rest of the match. The failure path spends no gold and confirms no card, so falling through strictly ADDS reachable behavior (manager ruling).

New `.cpp` line refs (post-edit): rule-2a success return `:581`, rule-2a fall-through else `:584–597`; rule-2b success return `:652`, rule-2b fall-through else `:655–667`.

### 2. Streak-log guard — `bRule2SpawnFailureLogged` (the one new identifier)
- Declared `private`, transient (NOT a UPROPERTY, not editor-exposed), default-initialized `false` — `SiegeBotController.h` (added after `bLoggedMineLockout`).
- The two rule-2 failure lines were promoted **Verbose → Log** (they were invisible at default verbosity — QA WARN-3), but each is now emitted **at most once per contiguous failure streak**: `if (!bRule2SpawnFailureLogged) { bRule2SpawnFailureLogged = true; UE_LOG(... Log ...); }`.
- **Cleared** on: (a) a successful rule-2 spawn — set to `false` right after `ConfirmPlayFromHand` in BOTH 2a and 2b success branches (`.cpp:580`, `.cpp:651`); (b) match reset — `ResetBot()` sets it `false` (`.cpp:1582`).
- 2a and 2b **share** the one latch (spec wording: "the two failure lines"), so a single tick where 2a fails then 2b also fails logs once, not twice.
- Semantics note (resolved discrepancy): the board point (3) says "cleared on the next successful rule-2 spawn AND on match reset" — I implemented exactly that. The dispatch prompt's looser "or the bot does something else" (i.e. clear when any other rule fires) was NOT implemented, because clearing on every other-rule fire would RE-log on the next rule-2 failure and defeat the anti-spam goal. The board is the contract; QA please confirm this reading.

### 3. Decision-trace law preserved
- A FAILED rule-2 emits only a `LogGitClaudeUnrealTest` diagnostic — never a `LogSiegeBot` fired-rule line. The `LogSiegeBot` lines still fire only inside the `SpawnBotCardActor` success branch, immediately before the success `return`, and their text/format is **byte-unchanged**.
- Rule priority order is untouched: 2a still precedes 2b precedes rule 3; a rule-1 fall-through still cannot reach rule 2 (rule 2 is guarded by `if (!NearestIntruder)` and `NearestIntruder` is computed once per tick).

## Rules 1/3/4/5 audit result

| Rule | Shape | Shares the abandon-the-tick trap? | Action |
|------|-------|-----------------------------------|--------|
| **1 DEFEND** | `if (ComputeValidBotSpawnPoint) {...} else {Verbose} return;` — return OUTSIDE the success branch | **YES** — identical to rule 2. With an intruder present + an affordable defensive card, a persistent spawn failure `return`ed and starved rules 3/4/5 (exactly the Fireball/Lightning/attack the bot wants when it CANNOT place a blocker). Continuing does NOT double-spend (no card confirmed on failure; rule 2 stays skipped while the intruder stands). | **FIXED** — fall-through. `.cpp:481` success return, `.cpp:484–496` else + no-return comment. Verbose failure log left at Verbose (the Log-promotion + latch are rule-2-specific per the spec; a silent fall-through here is anti-spam-correct). |
| **3 SPELLS (3a/3b)** | `if (ResolveSpell) {confirm} else {Verbose} return;` — NOT a spawn; the `return` is on a `ResolveSpell` **refusal** | **NO (different failure mode).** A whiff still returns `true` (spell resolved, spent) and goes down the success/return path — legitimate firing. The `ResolveSpell == false` branch is a degenerate-state tripwire ("should be unreachable"), not a transient spawn miss, so it does not stall in normal play. Forcing fall-through here would change spell rule-flow semantics — the exact kind of change QA said needs a ruling. | **REPORTED, not changed.** Recommend a separate manager ruling if we ever want spell-resolve refusals to fall through too. Not load-bearing (no double-spend — gold isn't moved on refusal), just out of this task's scope. |
| **4 ATTACK** | `if (ComputeValidBotSpawnPoint) {...} else {Verbose} return;` — return OUTSIDE the success branch | **YES** — identical to rule 2. `Gold >= AttackBankThreshold` stays satisfied (gold only accrues), so a persistent attack-spawn failure `return`ed and starved rule 5 (Cycle). Continuing does NOT double-spend (rule 5 only cycles UNPLAYABLE cards; the rule-4 unit is playable, so rule 5 won't re-pick it). | **FIXED** — fall-through. `.cpp:815` success return, `.cpp:818–828` else + no-return comment. Verbose log left at Verbose (same rationale as rule 1). |
| **5 CYCLE** | `if (DiscardFromHand) {charge} return;` inside `if (CardIndex != NONE && Gold >= fee)` | **NO.** Rule 5 is the LAST rule — its `return` is equivalent to reaching the end-of-function "no rule fired" tail. Nothing follows it to be starved. | **No change** (no trap). |

Sites I changed: **rule 1, rule 2a, rule 2b, rule 4** (all four `ComputeValidBotSpawnPoint`-else spawn-failure returns) + the `ResetBot` latch clear + the header field. Rules 3 and 5 untouched.

## What QA should scrutinize
- **Brace balance / structure** of the four rewritten `if/else` blocks (I verified `{`=`}`=193 post-edit and re-read all four regions; the success `return` sits inside `if (SpawnBotCardActor)` in every case).
- **The `SpawnBotCardActor`-returns-null path now falls through too** (previously it `return`ed). This is intentional and consistent with the ruling (no gold spent, no card confirmed), but it is a behavior delta beyond the literal "no valid spawn point" wording — flagging it explicitly. `SpawnBotCardActor` still logs the missing/incompatible-BP case internally; I did not tie that to the streak latch.
- **The latch clear-semantics choice** (board wording vs dispatch wording) — see §2 note.
- **Rules 1 & 4 scope call** — I fixed them under board point (4) ("fix any identical instance found in the others under the same ruling, and LIST every site you changed"). If QA reads the scope more narrowly (rule-2-only), rules 1 & 4 are the only extra sites and each is a pure fall-through with no state mutation on the failure path.
- Untouched by design: TASK-265 anchor-clamp / `UnitSpawnClearance` / spawn-Z logic, the TASK-262 spawn gate, `AttackBankThreshold`, the `bLoggedMineLockout` latch, and all `LogSiegeBot` fired-rule text.

## Files touched
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h` — added `bool bRule2SpawnFailureLogged = false;` (private, transient, documented).
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp` — rule 1, 2a, 2b, 4 fall-through + rule-2 streak-log guard + `ResetBot` latch clear.

## Acceptance mapping
- Forcing rule-2 spawn to fail (e.g. absurd `UnitSpawnClearance` in PIE) → the bot now continues to rules 3/4/5 in the same tick and keeps playing for the rest of the match. ✔ (control-flow verified; not compiled/PIE'd — that's build-master's fold-in.)
- `LogSiegeBot` = one line per FIRED rule, format byte-unchanged. ✔
- Rule-2 failure line visible at default verbosity, once per streak not per tick. ✔
