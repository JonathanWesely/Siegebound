# TASK-760 — the ladder-climb driver is now SELF-HEALING (gameplay-programmer)

**Status:** `ready-for-qa` · 2026-09-01
**Source:** `qa/TASK-741.md` **W-1** (verdict PASS, 0 blockers — this was its deepest WARN)
**Fence honoured:** `SummonedUnit.h` · `SummonedUnit.cpp` · `Tests/SiegeLadderClimbTest.cpp` and **nothing else**.
⛔ No compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ `ClimbableTower.{h,cpp}` untouched · ⛔ no mesh/anim/`ABP_Footman`.

---

## ⭐ TEST-COUNT DELTA — **+1 test** (13 → **14**) in `Tests/SiegeLadderClimbTest.cpp`

QA pinned 13 and TASK-742 stops if the count moves unexpectedly. **My delta is exactly +1.**
New test 14: `FSiegeLadderClimbSelfHealingDriverTest` →
`Siegebound.LadderClimb.AnExternallyKilledActorTickIsRestoredByTheStatePollSoTheWatchdogIsNeverStrandedOnADeadDriver`
⛔ No existing test was edited, renamed, deleted or re-ordered. Tests 1–13 are byte-identical.

---

## 1. THE LINE, AS PLACED

`SummonedUnit.cpp:1552-1582` — the **first statement** of `UpdateState()` (`:1550`), above the fence QA cited at
`:1567` (now `:1599` after the insert). The line itself is `:1579-1582`:

```cpp
void ASummonedUnit::UpdateState()
{
	// ══ THE SELF-HEALING CLIMB DRIVER (TASK-760, closing TASK-741's W-1) ═════════════════════
	// …
	if (LadderClimb.bActive)
	{
		RefreshActorTickEnabled();
	}

	// bAIFrozen is defense-in-depth (TASK-028): …
	// ══ THE TRAVERSAL FENCE (TASK-738, TOWER-§8.5) …
	if (bDead || !bStatsLoaded || bAIFrozen || bSpellFrozen || IsClimbing())
	{
		return;
	}
```

**Why FIRST and not merely "above the fence":** the fence early-outs on `IsClimbing()`, so one line lower it is
**dead code for the exact case it rescues**. It also sits above `!bStatsLoaded`, which `BeginLadderClimb`
deliberately does **not** test (`TOWER-§8.4(B)` pins four refusal reasons) — so an armed climb can never be
fenced out of its own rescue.

**⛔ `RefreshActorTickEnabled()`, never a bare `SetActorTickEnabled(true)`** — as specified.

---

## 2. ⭐ MY VERIFICATION THAT THE 0.25 s POLL RUNS INDEPENDENTLY OF THE ACTOR TICK

I re-derived QA's claim rather than relaying it. Four measured facts:

1. **It is a world timer, not a tick function.** `SummonedUnit.cpp:1487` (`BeginPlay`) and `:1211` (`EndSpellFreeze`)
   arm `StateTimerHandle` via `GetWorldTimerManager().SetTimer(..., StateCheckInterval, bLoop=true)`. `FTimerManager`
   is ticked by `UWorld`, and it reads nothing from `AActor::PrimaryActorTick`. `SetActorTickEnabled` writes only
   `PrimaryActorTick`'s tick-function state (`AActor::SetActorTickEnabled` → `FTickFunction::SetTickFunctionEnable`).
   ⇒ **the two are structurally disjoint.** The hazard cannot reach the healer.
2. **`BeginLadderClimb` clears `AttackTimerHandle` ONLY** (`:3846`). It never touches `StateTimerHandle`.
   ⇒ the poll keeps landing for the whole climb.
3. **`StateTimerHandle` is cleared at exactly four sites, and every one is itself a climb exit** — `EndPlay` `:772`
   (exit 7), `FreezeAI` `:853` (exit 5), `ApplyFreeze` `:1130` (exit 6), `HandleDeath` `:4220` (exit 4).
   All of them have *already*
   ended the climb through the one teardown, so `LadderClimb.bActive` is false there and the self-heal is a no-op.
   ⇒ **there is no state in which the poll is dead AND a climb is still armed** (except (5) below).
4. **The rate is shipped and positive.** `SummonedUnit.h:1236` `UPROPERTY(EditAnywhere, …, ClampMin="0.05") float
   StateCheckInterval = 0.25f;` ⇒ at the pinned line's 14.14 s watchdog budget the poll gets **56 chances to heal**.
   Worst-case healing latency = **one poll ≈ 0.25 s**, i.e. ~0.25 s of a stalled climb, then the driver resumes and
   the watchdog is live again.

**⚠️ (5) THE ONE DECLARED NON-COVERAGE — `AMinerUnit`.** `MinerUnit.cpp:62` seals `StateCheckInterval = 0`, and
`SetTimer` with rate 0 arms nothing ⇒ **a miner has no state poll and therefore no self-heal.** ⛔ NOT fixed:
`MinerUnit.{h,cpp}` is outside this task's fence, and `qa/TASK-741.md` **W-5** already records that a miner should
never be admitted to a ladder (no gold node on a deck ⇒ no path terminates there) and that its failure mode is a
double-drive the watchdog **drops** rather than a hang. **Asserted in test 14(b)** so the limit is *known* rather
than discovered.

---

## 3. ⚖️ ONE DECLARED DEVIATION FROM "ONE LINE, CHANGE NOTHING ELSE" — please rule on it

**What I also changed:** the *decision* inside `RefreshActorTickEnabled` is now a pure static.

```cpp
// SummonedUnit.h:302, inside FSiegeLadderClimbStatics, immediately after IsAttackAllowed
static bool WantsActorTick(bool bLungeActive, bool bClimbActive) { return bLungeActive || bClimbActive; }

// SummonedUnit.cpp:3795 — RefreshActorTickEnabled, was: SetActorTickEnabled(bLungeActive || LadderClimb.bActive);
SetActorTickEnabled(FSiegeLadderClimbStatics::WantsActorTick(bLungeActive, LadderClimb.bActive));
```

**Why it was unavoidable, and it is the reason the test can fail at all.** The spec's hard requirement was a test
that *can* fail. I proved to myself that no such test exists without this:

- `UpdateState()`, `RefreshActorTickEnabled()`, `LadderClimb` and `bLungeActive` are **all `private:`** — a test
  cannot call or arm any of them.
- `GetMutableDefault<ASummonedUnit>()` is useless here: `AActor::SetActorTickEnabled` **early-outs on `IsTemplate()`**,
  so the CDO's tick flag never moves. Any assertion against it would be the exact tautology this project has lost
  two loops to.
- A live instance is impossible: this file's own doctrine (`:36-51`) measures it, and I re-confirmed it —
  `BeginLadderClimb` reaches `GetWorldTimerManager()` ⇒ `GetWorld()->GetTimerManager()`, which **crashes** a
  world-less unit rather than failing it. There is **no `SpawnActor` and no `UWorld::CreateWorld` anywhere** in
  `Siegebound/Tests/`, and no `NewObject` of an *actor* either.

⇒ Without the extraction the only available test is a test-authored model asserting against itself. **With** it, the
shipped predicate does the deciding and the test names both forbidden implementations.

**Why it is safe, and why I claim it is not scope creep:**
- **Behaviour-identical by inspection:** same expression, same two operands, same order, no new term, no new state.
- **Exact in-file precedent, three lines above it:** `IsAttackAllowed(bool, bool)` was lifted into this same struct
  for this same reason — the actor's three guard points stay *wiring*, the decision becomes a truth table (tested
  by test 8). `WantsActorTick` is that pattern applied a second time.
- **`SetActorTickEnabled` still has EXACTLY ONE writer in the whole `Source/` tree** (`SummonedUnit.cpp:3795`) —
  QA's grep result is preserved, not weakened. I re-grepped: confirmed.
- ⛔ No new `UPROPERTY`, no designer surface, no new timer, no new call site, no header include added.

**If QA rules against it:** deleting `WantsActorTick` and restoring `SetActorTickEnabled(bLungeActive ||
LadderClimb.bActive)` is a two-line revert, and test 14 sections (c)/(d) would have to be deleted with it —
leaving (a)/(b), which still hold. The self-heal line itself is unaffected either way.

---

## 4. THE TEST — WHAT IT WOULD CATCH

`Tests/SiegeLadderClimbTest.cpp:1166-1318`, five sections. Modelled on test 12 (names its traps explicitly).

| § | Claim | What a failure means |
|---|---|---|
| **(a)** | **The hazard is real.** With the driver dead, `ElapsedSeconds` stays `0` and `bActive` stays `true` — the clock has exactly one owner, and it is `Advance`, which runs on the actor tick. **Paired self-check:** one call from a *live* driver past the budget ends it as a `Timeout`. | Row 1 flips if the clock is ever moved off `Advance` (e.g. onto the poll) — which is the alternative fix, so the failure correctly demands a re-read rather than a rubber stamp. The self-check is what stops (a) passing because the watchdog is simply broken. |
| **(b)** | **The healer has a driver, and it is shipped.** `StateCheckInterval` is read off the `ASummonedUnit` CDO **by reflection** and must resolve, be **strictly positive**, and land ≥4× inside the watchdog budget. Plus: the `AMinerUnit` CDO's is **0** — the declared non-coverage. | ⭐ **This is the row that catches the real future regression:** if anyone seals or removes `StateCheckInterval` on the base class, the self-heal silently has no driver and the hang comes back. Today nothing in the suite would notice. |
| **(c)** | **The hazard scenario end-to-end.** Arm a climb → tick ON via the shipped predicate → **externally write the flag `false`** → the poll lands → assert the tick is **restored**. | Fails if the composed predicate stops wanting the tick during a climb. |
| **(d)** | **The full 2×2 truth table, both traps named.** `(F,T)⇒true` is the self-heal row — *"a predicate that dropped the climb term returns FALSE here, and the poll would re-assert the very OFF that hung the unit"*. `(F,F)⇒false` is the other — *"a bare `SetActorTickEnabled(true)` returns TRUE here and would fight whatever legitimately disabled the tick"*. Plus the invariant: while climbing the writer wants the tick **regardless of the lunge**. | Either forbidden implementation flips at least one row. A future **third term** that could return false mid-climb fails in this module rather than as a hang in a playtest. |
| **(e)** | **What it does NOT prove, named.** The call site itself (wired into `UpdateState`, above the fence) is a **diff read**, not a suite row — this file's stated `SC-§32` split, with the three private-access / crash reasons spelled out. | Honesty row, test-9 precedent. **⇒ QA must eyeball §1 of this note against the diff; the suite cannot do it.** |

---

## 5. ⛔ EVERYTHING QA PASSED, RE-VERIFIED AS UNDISTURBED

| Item QA passed | State |
|---|---|
| The eight **`§8.5a`** clauses | ✅ untouched — no clause's code was edited |
| Symmetric capsule-centre lift (`Begin`, one `Lift` on `Start` **and** `End`) | ✅ untouched |
| Test **12(a)**'s surviving self-check (foot *and* midpoint still swept) | ✅ untouched — test 12 is byte-identical |
| **Exactly two** `SetActorLocation` sites, both inside `TickLadderClimb` | ✅ re-grepped: `:3978` (arrival snap) and `:4033` (breach drive). No third. |
| `Advance` makes arrival and timeout **mutually exclusive** | ✅ untouched |
| Snap gated on `bReachedTop` | ✅ untouched (`:3977-3979`) |
| `SetActorTickEnabled` = **one writer** in `Source/` | ✅ still one (`:3795`) |
| **No subclass overrides `Tick`** | ✅ re-grepped the unit hierarchy — `SummonedUnit.h:527` is the only one; `UpdateState` is non-virtual with a single declaration |

---

## 6. FOR QA TO SCRUTINISE

1. **⚖️ The `WantsActorTick` extraction (§3) — rule on it.** It is the one thing beyond the single line. I judged a
   provable, precedent-backed, behaviour-identical extraction better than a tautological test, and I have named the
   two-line revert if you disagree.
2. **The placement claim (§1)** — that the self-heal is the *first* statement of `UpdateState` and therefore above
   both `IsClimbing()` and `!bStatsLoaded`. The suite cannot assert this; it is your diff read.
3. **The independence claim (§2)** — four cited facts. Item 3 (every other `StateTimerHandle` clear site is itself a
   climb exit) is the one worth re-walking, because it is what makes "no state has a dead poll and a live climb" true.
4. **The miner non-coverage (§2.5)** — declared, asserted in test 14(b), **not fixed**. Confirm that is the right call
   against W-5 rather than a gap I should have closed.
5. **The test-count delta is +1 (13 → 14).** TASK-742's total must move by exactly one from whatever the ladder
   batch's figure was.

## 7. NOT DONE, ON PURPOSE

⛔ No compile (TASK-742 owns the module's one compile) · ⛔ no editor/MCP (`L_Arena` and `ABP_Footman` are dirty in
memory — nothing of mine went near it) · ⛔ no Git · ⛔ `ClimbableTower.{h,cpp}` untouched · ⛔ `CONVENTIONS.md`
unedited · ⛔ W-2/W-3/W-4/W-5/W-6/W-7/W-8 untouched — TASK-760 is W-1 only.
