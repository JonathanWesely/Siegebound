# QA Report — TASK-848 (WF-GATE-B, the funnel gate)

**Verdict: PASS** — 0 BLOCKER · 4 WARN · 4 NIT
**Date:** 2026-09-02 · **Reviewer:** qa-reviewer · **Mode:** read-only (no edits, no engine, no Git)

---

## `SC-§29` COVERAGE LEDGER

| this gate covers | it does NOT cover |
|---|---|
| ⭐ **`TASK-828` — and nothing else** (the acquisition funnel: `SiegeCombatStatics.{h,cpp}` + the 9 routed sites + `Tests/SiegeAcquisitionFunnelTest.cpp`) | `TASK-827` / `TASK-837` (⇒ `qa/TASK-847.md`) · `829`/`830`/`831` (⇒ `qa/TASK-849.md`) · `838`+ (⇒ `qa/TASK-850.md`) · the compile (`QUIET-MODULE`) · art |

**Subject files read in full or in the relevant span:** `SiegeCombatStatics.h`, `SiegeCombatStatics.cpp`, `SummonedUnit.cpp` (sites 1–2 + `IsTargetAlive`), `Tower.cpp` (sites 3–4 + `IsAcquirableEnemy`), `HeroCharacter.cpp` (site 8 + Rally), `SpellLibrary.cpp` (site 6, all three resolvers), `SpellLineSweep.cpp` (site 7), `SiegeCheatManager.cpp` (site 9), `Tests/SiegeAcquisitionFunnelTest.cpp` (all 1,072 lines), plus `TeamId.h`, `SiegeInvisibilityStatics.{h,cpp}`, `SiegeFogStatics.h`, `GoldNode.h` and the five pre-existing tests that text-probe the six edited files.

### ⭐ THE WINDOW WAS OPEN AND THIS GATE RAN INSIDE IT

Measured, not assumed: `GatherTeamAgentsFiltered` (`SiegeCombatStatics.cpp:25-69`) contains **zero** consults of `FSiegeInvisibilityStatics`, **zero** of `FSiegeFogStatics`, **zero** `bIsInvisible` and **no radius parameter anywhere in the funnel's signature**. `TASK-829` and `TASK-838` had **not** landed when this review was taken, so *"zero behaviour change"* was still a checkable claim and it was checked. ⛔ **A re-run of this gate after either lands cannot reproduce this evidence.**

---

## THE THREE THAT MATTER MOST (spec items a / b / c)

### (a) ⭐⭐ THE BINDING GREP — **GREEN, measured independently at 4 needles**

| needle | scope | required | **measured by QA** |
|---|---|---|---|
| `UGameplayStatics::GetAllActorsWithInterface(` | ALL of `Source/` (**265** `.h`/`.cpp`, tests included) | 1 | **1** — `SiegeCombatStatics.cpp:41` ✅ |
| bare `GetAllActorsWithInterface(` | shipping source (excl. `Tests/`) | 1 | **1** — same line ✅ |
| bare token, `Tests/` lane | the NAMED exemption | 1 | **1** — `SiegeGhostPawnTest.cpp:116`, a `TEXT()` failure-message literal, **not a call** ✅ |
| `UTeamAgent::StaticClass()` | shipping source | 1 | **1** — `SiegeCombatStatics.cpp:41` ✅ |

**Every other hit in the tree is prose, and I checked each one against the scanner's own rule** (`Trimmed.StartsWith("//" / "* " / "*/" / "/*")`): `CommanderNpc.h:97`, `Projectile.h:76`, `SiegeCombatStatics.h:33/36/65/169`, `SiegeGhostPawn.h:39`, `SiegeGameMode.h:94`, `SiegeStuckStatics.cpp:36`, `SpellLibrary.cpp:62` — all doc-comment continuations, all skipped. ⇒ the comment-skip is **load-bearing, not decorative**: without it `SiegeCombatStatics.h` and `SiegeGhostPawn.h` alone would push the qualified lane to 3 and the file would have to stop explaining the law it enforces.

⭐ **The runtime-composed needle is correct and I verified it cannot self-count.** `BareNeedle = "GetAllActors" + "WithInterface("`. The gate's own file spells the token twice on *code* lines — `:196` (the test's pretty name, `…WithInterfaceAppearsExactlyOnce…`) and `:561` (a `TEXT()` message) — and **neither is followed by `(`**, so both are correctly outside the needle. The `Tests/` lane therefore reads exactly 1 (`GhostPawnTest:116`) rather than 3. The self-check at `:226` (`BareNeedle == "GetAllActorsWithInterfac" + "e("`) means a typo'd needle cannot make the gate pass by finding nothing. ✅

### (b) / (b1) ⭐⭐ ZERO BEHAVIOUR CHANGE — and the NEW TEST FILE is **CHARACTERIZATION, not a smuggled feature**

I applied the manager's corrected discriminating question to each of the 9 tests — *would this have passed against the pre-refactor tree?*

| # | test | pins OLD behaviour? | classification |
|---|---|---|---|
| 1 | grep gate | ⛔ no — it pins the **refactor's own acceptance criterion** | ✅ acceptance gate (the spec's item 3 shipped as a test) |
| 2 | `IsHostileTeam` 2×2 truth table | ✅ **yes** — `!=` is the term all nine sites wrote inline | ✅ characterization, **executed** |
| 3 | `Out.Reset()` + null-world | ✅ yes — the engine's own null behaviour, preserved | ✅ characterization, **executed** |
| 4 | 9 sites routed / 10 hostile + 1 friendly | ⛔ no — structural, by construction | ✅ acceptance gate |
| 5 | no site kept its own team filter | ⛔ no — structural | ✅ acceptance gate |
| 6 | 13 anti-over-lift probes + no-sort | ✅ **yes** — every probed term predates the refactor | ✅ **the strongest characterization in the file** |
| 7 | `ApplyRadialDamage` friendly-fire | ✅ yes (the guarantee) + structural (the gather) | ✅ characterization |
| 8 | Battle Cry uses the friendly lane | ✅ **yes** — `ResolveAllyBuff` bought friendlies before and buys friendlies now | ✅ characterization |
| 9 | no veil/fog token in the 5 call-site files | ✅ yes — trivially true pre-refactor, **and it is written to stay true after 829/838** | ✅ permanent law |

⛔ **Nothing in the file asserts invisibility, fog, suppression or a clamp as a *behaviour*.** The only mentions of `bIsInvisible` / `FSiegeInvisibilityStatics` / `IsVisibleTo(` / `FSiegeFogStatics` / `EffectiveVisionRadius` are **absence** assertions (tests 7 and 9). ⇒ **(b1) satisfied: this is exactly the file a behaviour-neutral refactor should ship, and failing it for existing would have been the wrong call.**

**No pre-existing test was edited, and none is now red.** I could not diff (no Git by design), so I checked the stronger thing — every pre-existing test that reads the six edited files, against the post-refactor text:

- `SiegeHeroLadderClimbTest.cpp:1198` extracts `void AHeroCharacter::DoMeleeAttack()` and requires `IsClimbing()` **exactly once** → still exactly once, `HeroCharacter.cpp:491`. ✅
- `SiegeClimbableTowerTest.cpp:2050` requires `OnLadderClimbEnded.Broadcast(` **once** in `HeroCharacter.cpp` → untouched by the melee lift. ✅ (its `#include` probes at `:1917-1919` are about **`ClimbableTower.cpp`**, which this task never opened. ✅)
- `SiegeHealthBarOcclusionTest.cpp:291-296` probes `CreateDefaultSubobject<UCombatantHealthBarComponent>` → unaffected. ✅
- `SiegeRecallTest.cpp:435`, `SiegeGhostPawnTest.cpp` → **reflection** lanes (`FindFunctionByName`, `ImplementsInterface`, `Cast<ITeamAgent>` on CDOs), untouched by a call-body lift. ✅
- No test outside the new file greps `GetAllActorsWithInterface`, `Cast<ITeamAgent>(Candidate)` or `GatherTeamAgents(` in the six edited files. ✅

**Each of the nine sites kept its own filtering — verified by reading the loop, not by trusting the probe:**

| site | own filtering still in place |
|---|---|
| `SummonedUnit::AcquireTarget` `:1654` | `IsTargetAlive`, `AggroRadius`, pawn/non-pawn bucketing, `TieBreakDistance` ✅ |
| `SummonedUnit::AcquireEnemyNearPoint` `:2203` | `IsTargetAlive`, `DistSquared2D` disc, bucketing ✅ |
| `Tower::AcquireTarget` `:209` | `IsAcquirableEnemy` (§3.7 class gate + liveness), `RangeSq`, `MinRangeSq` blind spot ✅ |
| `Tower::FireChainZapAt` `:364` | `IsAcquirableEnemy`, `!= PrimaryTarget`, `ChainBounceRadius` from the **previous** target ✅ |
| `ApplyRadialDamage` `:82` | `ActorGetDistanceToCollision` closest-point + origin fallback, `Distance > Radius` ✅ |
| `SpellLibrary::ResolveFreeze` `:285` | `Row.AoERadius`, the ruling-5 type exclusions, per-type liveness ✅ |
| `SpellLibrary::ResolveTopTargetsDamage` `:343` | castle exclusion, per-type liveness + current HP, `Row.AoERadius`, `StableSort`, `MaxTargets` ✅ |
| `SpellLineSweep::ApplyLineEffectUpTo` `:119` | `AppliedTargets` once-only, `ClosestPointOnSegment` + `LineHalfWidth` ✅ |
| `HeroCharacter::DoMeleeAttack` `:464` | `MeleeRange`, `MinCosAngle` cone, closest-point measure ✅ |
| `SiegeCheatManager::FindNearestEnemy` `:135` | `IsCombatActorAlive` ✅ |

⭐ **ORDER PRESERVATION — CHECKED AT EVERY SITE, and the claim holds.** The programmer's argument is that every caller breaks ties by **strict improvement**, so the first candidate wins and a sort in the funnel would re-pick targets at nine sites without changing one result set. I checked each caller for a first-match/early-`break` shape that would make the claim *stronger* than stated, and for a stable-sort that would make it load-bearing in a different way:

- `SummonedUnit.cpp:1699/1705`, `:2245/:2251` — `Distance < BestPawnDist` / `< BestOtherDist` (strict) ✅
- `Tower.cpp:258` — `DistSq >= BestDistSq → continue` (strict) ✅ · `Tower.cpp:429` — `DistSq < NextDistSq` (strict) ✅
- `SiegeCheatManager.cpp:154` — `DistSq < BestDistSq` (strict) ✅
- `ApplyRadialDamage`, `DoMeleeAttack`, `ResolveFreeze`, `ApplyLineEffectUpTo`, `ResolveAllyBuff` — **apply-to-all** loops with no selection: order affects only the sequence of `ApplyDamage`/`ApplyFreeze` calls, which is the enumeration order it always was ✅
- `ResolveTopTargetsDamage` — a **`StableSort`** on (HP desc, distance asc): ties in *both* keys resolve by input order ⇒ the funnel's order guarantee is what keeps this deterministic, and it is preserved ✅
- ⛔ **No site takes a first match and breaks**, and no site reverses. The funnel appends in loop order, `Sort(` count 0, `Out.Add(Candidate)` count 1, `Out.Reset()` count 1 — all four verified by reading `SiegeCombatStatics.cpp:29-68`. ✅

**Predicate identity, term for term:** the funnel is `IsValid(Candidate)` → `Cast<ITeamAgent>` → `IsHostileTeam(ViewerTeam, GetTeamId()) != bWantHostile`. The two `SummonedUnit` sites folded `IsValid` into `IsTargetAlive`, and I confirmed `IsTargetAlive` **opens with `IsValid(Target)`** (`SummonedUnit.cpp:4335`) ⇒ the funnel's guard is a redundant superset there, **never additive**. `Tower::IsAcquirableEnemy` also opens with `IsValid` (`:272`). ✅

**Declared inert ordering changes — I re-derived each rather than accepting it:** `Candidate == this` / `Target == this` now runs after the team filter (self is always same-team ⇒ already dropped) ✅ · `AppliedTargets.Contains()` on the pre-filtered set (the set only ever held enemies) ✅ · null-`World` no longer reaches the engine's `LogAndReturnNull` (**the single observable difference in the whole pass**, and it is one fewer spurious warning; all callers guard `World` first — `SummonedUnit.cpp:1657/:2206`, `Tower.cpp:212/:367`, `HeroCharacter.cpp:497`, `SpellLineSweep.cpp:122`, `SiegeCheatManager.cpp:137`, `ApplyRadialDamage:92`) ✅

### (c) ⭐⭐ `ApplyRadialDamage` — **FRIENDLY FIRE IS STILL IMPOSSIBLE, STATED EXPLICITLY AS THE SPEC DEMANDS**

- The friendly-fire **authority did not move**: it was the **team filter on the candidate set** (`SiegeCombatStatics.cpp:118-119`, `GatherHostileAgents(World, Team, …)`) and it is still the team filter on the candidate set. It is deliberately **not** the receiver's instigator chain — `DamageCauser` is still `nullptr` (`:141`), which is exactly why a tower-fired projectile with no resolvable instigator still cannot blast its own side. ✅
- The candidate set is **element-for-element** what it was: same `IsValid` + `Cast<ITeamAgent>` + `GetTeamId() != Team` triple, same enumeration order, no sort. ✅
- **`AGoldNode` still opts out — verified at the class declaration, not from the comment:** `GoldNode.h:100` is `class AGoldNode : public AActor` with **no `ITeamAgent`** in its base list (and `:78-79` says so on purpose). ⇒ mining nodes are still never caught by any blast. ✅
- The closest-point measure (`:128`), the origin fallback (`:132`), the radius gate (`:134`) and the routing through the target's own `TakeDamage` (`:141`, so Siege-typed = 200% vs castle/buildings) are **byte-identical**. ✅
- ⛔ **Zero veil hook shipped here, which is correct:** `WITCH-§2` / `J-W2` rules this lane EXEMPT and `TASK-829` owns implementing that explicitly. Test 7's `bIsInvisible == 0` row is written to be **inverted** by 829 rather than deleted — the right shape. ✅

---

## ⚖️ THE TWO DECLARED JUDGMENT CALLS — RULED, ON THE RECORD

### RULING 1 — ⭐⭐ `GatherFriendlyAgents` is **IN SCOPE and CORRECT**. The measurement is real; I verified it at source before accepting the conclusion.

The invitation to overrule was taken seriously, and the underlying fact was checked first, because the whole justification collapses if Battle Cry does not read that gather:

- `SpellLibrary.cpp` has **one** enumeration feeding **three** resolvers — `ResolveFreeze` (`:285`, hostile), `ResolveTopTargetsDamage` (`:343`, hostile), **`ResolveAllyBuff` (`:460`, Battle Cry)**.
- `ResolveAllyBuff:476-499` iterates the gathered roster and buffs `Cast<ASummonedUnit>(Candidate)` within `Row.AoERadius` — **with no team term of its own**, because the gather supplies it. ⇒ routed through `GatherHostileAgents` it would iterate **only enemies**, and `ApplyCombatBuff` would land on **nobody the player owns**. **A shipped card silently doing nothing, with a green suite and a commit message reading "pure refactor" — the finding is exactly as described.** ✅
- **The split (two named lanes) beats the `bool bHostile` alternative for a structural reason, not a stylistic one:** `TASK-829` must change one lane and must be **unable** to change the other by editing a shared branch (`WITCH-§2` lane 4 — friendly acquisition is NEVER veil-suppressed). A shared public bool makes that a matter of care; two entry points make it a matter of structure.
- ⭐ **And the shared *private* helper is safe from the same trap, which I checked rather than assumed:** if 829 puts the veil consult inside `GatherTeamAgentsFiltered` un-guarded, `FSiegeInvisibilityStatics::IsVisibleTo` returns **`true` unconditionally for a same-team viewer** (`SiegeInvisibilityStatics.cpp:26-29`, and that early return is placed above the veil test on purpose). ⇒ the friendly lane cannot be suppressed even by the careless wiring. The header's forward-note at `SiegeCombatStatics.h:69-70` is **accurate, not aspirational**. ✅
- The grep gate still reads **one**: both public lanes funnel into the single private enumeration. ✅
- Consistent with the manager's pre-ruling (board `:13879` (B)) and with `FOG-§7` row 5, now law. **UPHELD.**

### RULING 2 — routing `SiegeCheatManager.cpp` was **IN SCOPE**. ✅

- The file is named in the board's `names:` line **and** in spec item (2)'s nine sites (`SiegeCheatManager.cpp:130`). The "six source files" phrase at `:13877` is a **parallel-safety** statement (why this task is `parallel-safe: no`), not a fence.
- The declared fence (`SiegePlayerController`, `CardHandWidget`, `WBP_CardHand`, `Building`, `ClimbableTower`, `SiegeControlsHelpWidget`, `IMC_Hero`) does **not** contain it, and no live task owns it.
- The binding acceptance gate is *"appears in `Source/` **exactly once**"*. Leaving the cheat un-routed would have produced a second hit that then had to be exempted by name — legal under `WITCH-§1`, but **strictly weaker**. The programmer's reason is sound and is written at the call site (`:126-134`): a cheat that saw a different candidate set from the game is an instrument that lies, and `SummonTestUnit`/`ApplyTestDamage` exist precisely to verify **shipping** behaviour headlessly.
- ⭐ **Compatibility with `FOG-§7` row 4 checked:** routing does **not** subject the cheat to `TASK-838`'s clamp, because the clamp needs a **radius** the funnel signature does not take, so it must arrive **at the call site** — and the cheat's call site passes none. The exemption stays expressed by *which call the site writes*, exactly as `FOG-§7`'s structural law requires. ✅ (See **W-2**: the CONVENTIONS row needs a word changed so nobody "restores" the exemption by re-adding an enumeration.)

### RULING 3 — the redundant `Candidate == this` / `Target == this` guards: **KEEP them.** Provably inert today (self is always same-team, so the gatherer already dropped it), commented as such at all three sites, one pointer compare on a 0.25 s poll, and load-bearing again the day "hostile" is redefined. ⛔ I do **not** rule dead code out here.

### RULING 4 — `ETeamId` vs the spec's `ESiegeTeam`: **the correction is right and is not a liberty.** Measured: **zero** `ESiegeTeam` anywhere in `Source/` (the only hits in the repo are the board/CONVENTIONS/handoff paragraphs recording the correction). The shipped enum is `ETeamId` (`TeamId.h:14`, `Blue`/`Red`), and every signature, test and call site in the diff uses it. `WITCH-§1`'s phantom-symbol law is satisfied — **nothing in the landed code or tests carries the phantom name.** ✅

### RULING 5 — not compiled (`QUIET-MODULE`): **out of this gate's scope by the spec**, and I did not fail it for that. Static checks I could make instead: all six routed files carry `#include "Siegebound/SiegeCombatStatics.h"` (`SummonedUnit.cpp:37`, `Tower.cpp:14`, `HeroCharacter.cpp:36`, `SpellLibrary.cpp:17`, `SpellLineSweep.cpp:15`, `SiegeCheatManager.cpp:21` — the four new ones carry a TASK-828 rationale comment and sit in alphabetical order) · `ITeamAgent::GetTeamId()` is `const` (`TeamId.h:39`) so the funnel's `const ITeamAgent*` compiles · the header's `ETeamId` comes from its own `#include "Siegebound/TeamId.h"` · the test file's macros/flags (`IMPLEMENT_SIMPLE_AUTOMATION_TEST` + `EditorContext | EngineFilter`) are byte-identical to the 28 sibling test files. ⛔ **Static verification is not a compile and must not be reported as one.**

---

## SITE COUNT + CENSUS — RE-MEASURED INDEPENDENTLY (`TL-§5b`)

**Nine enumerations, eleven team-filter consumers, ten hostile + one friendly — confirmed by my own count, line by line:**

| file | `FSiegeCombatStatics::GatherHostileAgents(` (code lines) | friendly |
|---|---|---|
| `SummonedUnit.cpp` | `:1669`, `:2215` = **2** (`:1663`, `:3187`, `:3829` are `//` prose, and none carries the `(`) | 0 |
| `Tower.cpp` | `:225`, `:387` = **2** | 0 |
| `SpellLibrary.cpp` | `:298`, `:366` = **2** | `:477` = **1** |
| `SpellLineSweep.cpp` | `:141` = **1** | 0 |
| `HeroCharacter.cpp` | `:556` = **1** (`:552` is prose) | 0 |
| `SiegeCheatManager.cpp` | `:142` = **1** | 0 |
| `SiegeCombatStatics.cpp` (in-class) | `:119` = **1** | 0 |
| **total** | **10** ✅ | **1** ✅ |

**Is there a tenth `GetAllActorsWithInterface`? No** — the four-needle grep above is exhaustive over all 265 files. **Is the 9→11 gap fully explained? Yes** — one enumeration (`SpellLibrary.cpp`) with three consumers, and the third wants friendlies. See **W-1** for the honest boundary of that claim.

**Suite census, published with its scope (`TL-§5b`), and the delta is what I judged:**
`^IMPLEMENT_SIMPLE_AUTOMATION_TEST(` (anchored — ⛔ **not** a bare `^IMPLEMENT_` grep, which would also count `IMPLEMENT_PRIMARY_GAME_MODULE` and misreport the file count) over all of `Source/`: **376 tests across 29 files**, all under `Siegebound/Tests/`; **0** `IMPLEMENT_COMPLEX_`/`CUSTOM_`/`NETWORKED_`.
**`TASK-828`'s own contribution: `SiegeAcquisitionFunnelTest.cpp` = 9 ⇒ delta `+9`, exactly as declared.** ✅ Cross-check of the parallel lanes in the same census: `SiegeInvisibilityTest.cpp` = **8** (`TASK-827` declared +8 ✅) and `SiegeFogTest.cpp` = **9** (`TASK-837` declared +9 ✅). ⛔ **No absolute from any spec was used as a target**; reconciliation of the wave total belongs to `TASK-835`/`TASK-843`.

**`FOG-§7` signature check (`TASK-828(1b)`):** the funnel takes `(const UWorld*, ETeamId, TArray<AActor*>&)` — **no radius, no caller-kind, no enum, and not one branch that inspects its caller.** ⇒ `TASK-838` can clamp row 1 (vision) **at the call site** while rows 2/3/5 stay unclamped, and the `switch (CallerKind)`-inside-the-funnel AUTOMATIC FAIL is **absent**. The taxonomy is not foreclosed. ✅

---

## MUTATION SUITE — ALL 8 RE-DERIVED; **none is trivially detectable for the wrong reason**

| # | mutant | which assertion goes red | verdict |
|---|---|---|---|
| 1 | a tenth enumeration | test 1 lane A (whole tree) **and** lane B (bare/unprefixed) **and** the `UTeamAgent::StaticClass()` lane | ✅ right reason; needle composed at runtime so the gate sees its own file |
| 2 | the team filter deleted | ⭐ **not** test 2 (which tests the predicate in isolation) — it is **test 5's `IsHostileTeam(` == 2** in `SiegeCombatStatics.cpp` that falls to 1 | ✅ discriminates, and by a *different* assertion than the obvious one — checked, not assumed |
| 3 | Battle Cry re-routed to hostile | test 8 (`GatherFriendlyAgents(` == 1 **and** `GatherHostileAgents` == 0 in the slice) **and** test 4's friendly total | ✅ double-covered — the one real regression available |
| 4 | a `Sort()` in the funnel | test 6 `Sort(` == 0 on the extracted funnel body | ✅ (catches `Sort(`/`StableSort(`/`Algo::Sort(`; see **NIT-2**) |
| 5 | melee range gate removed | test 6 probe `MeleeRange` in `DoMeleeAttack` | ✅ |
| 6 | the blast gathering friendlies | test 7 (`GatherHostileAgents(World, Team, HostileAgents)` == 1 → 0, **and** `GatherFriendlyAgents` == 0 → 1) | ✅ |
| 7 | a per-site veil check added | test 9, over the five call-site files, with **real** shipped symbol names | ✅ |
| 8 | the comment-skip broken | the `SiegeGhostPawn.h` control (must count **0** on code lines) paired with `Contains()` proving the prose is there | ✅ a control on the **instrument**, which is the one most suites omit |

⭐ **The `IsVisibleToViewer` / `FogVisionCeilingUU` correction is REAL and COMPLETE.** The names test 9 actually ships resolve to shipped declarations at the cited lines: `IsVisibleTo(` → **`SiegeInvisibilityStatics.h:338`** and `EffectiveVisionRadius` → **`SiegeFogStatics.h:299`** — the citations are exact, and `FSiegeInvisibilityStatics`, `FSiegeFogStatics`, `bIsInvisible` all exist. Measured across the five call-site files: **all five tokens count 0 today**, and the positive control (`FSiegeCombatStatics::Gather` > 0) is satisfied in each. ⇒ **the guard can fire.** A phantom name here would have been the most expensive kind of green, and it is not present.

---

## Findings

- **[WARN] W-1 — `handoffs/TASK-828-programmer.md` §2 / `SiegeCombatStatics.h:41` — *"I looked for a tenth enumeration and did not find one"* is true **only for `GetAllActorsWithInterface`**, and a future reader will read it as "no other hostile acquisition exists".** Measured: `SiegeBotController.cpp:981` (`TActorIterator<ASummonedUnit>` + `GetTeamId() != EnemyTeam`), `:1004` (`TActorIterator<AHeroCharacter>`, same term) and `:1047` (`== EnemyTeam`) are **hostile perception scans outside the funnel**, and `SummonedUnit::FindNearestEnemyCastle` (`:1739`) is an enemy-selecting `TActorIterator<ACastle>` — so "none of the 44 `TActorIterator` sites is an `ITeamAgent` acquisition" is imprecise. ⛔ **This is NOT a `TASK-828` defect and NOT a reason to fail:** none was among the nine, `FOG-§6` explicitly records `SiegeBotController.{h,cpp}` as **untouched (`J-F4` deferred)**, and castles cannot be veiled. ⇒ **Routed to the manager for `TASK-829`/`849`:** once the veil lands, a veiled unit will still be **visible to the bot's card-play threat read**, which is a plausible reading of *"completely invisible to enemy AI"*. Suggested fix: one sentence in `WITCH-§2` naming the bot lane as a declared residual (or a `J-W` ruling), so 849 is not left to discover it.
- **[WARN] W-2 — `CONVENTIONS.md` `FOG-§7` row 4 says the cheat lane is *"exempt BY NAME in a comment"*; the diff **ROUTES** it instead (correctly — see Ruling 2).** As written, a future task honouring that table row would **re-add a second `GetAllActorsWithInterface`** and break the binding gate. Suggested fix (manager's edit, not the programmer's): amend row 4 to *"ROUTED through `GatherHostileAgents`; unclamped because the clamp arrives as a call-site radius the cheat does not pass (`TASK-828`)"*.
- **[WARN] W-3 — `Tests/SiegeAcquisitionFunnelTest.cpp:724-752` — the 13 anti-over-lift probes cover 6 of the 9 sites; three have no body probe:** `ResolveFreeze` (`Row.AoERadius` + the ruling-5 type exclusions), `ResolveTopTargetsDamage` (castle exclusion + `Row.AoERadius` + `MaxTargets`) and `FindNearestEnemy` (`IsCombatActorAlive`). I verified all three **by hand at source** and their own filtering is intact, so this is a **coverage** gap, not a defect — but a future "consolidation" could lift those three filters and **no test would go red**. Suggested fix: three more rows in `Probes[]` (the anonymous-namespace resolvers need the `Find`-slice idiom test 8 already uses, not `ExtractFunctionBody`).
- **[WARN] W-4 — declared residual, restated because the gate should not be read as more than it is: no test drives the funnel over a populated world.** The house rule (no `UWorld::CreateWorld` / `SpawnActor` in `Siegebound/Tests/`, `SiegeLadderClimbTest.cpp:39`) means the filter's *runtime* behaviour is pinned by (i) the pure `IsHostileTeam` truth table, (ii) the null-world contract and (iii) source-text structure. A mutation that kept the shape but broke the term (e.g. `if (false)` around the filter) would survive the suite. The file declares this at `:42-49`. ⇒ **the live acquisition regression, if one existed, could only surface in `TASK-835`'s PIE/human pass** — say so there rather than treating 376 green as coverage of combat.
- **[NIT] N-1 — `Tests/SiegeAcquisitionFunnelTest.cpp:90` names `SummonedUnit.cpp` among the files that mention the forbidden token in prose. It does not** (measured: `CommanderNpc.h`, `Projectile.h`, `SiegeCombatStatics.h`, `SiegeGhostPawn.h`, `SiegeGameMode.h`, `SiegeStuckStatics.cpp`, `SpellLibrary.cpp`). Harmless — but it is a prose claim in the one file whose entire discipline is that prose must be true.
- **[NIT] N-2 — the no-sort guard is one needle short of its own claim.** `Sort(` catches `Sort`/`StableSort`/`Algo::Sort`, but **not** `Algo::Reverse(`, a reverse iteration or an index-descending loop — each of which would change the same tie outcomes at nine sites. Suggested fix: add `Reverse` (and optionally `--` on the loop) to the funnel-body ban list.
- **[NIT] N-3 — `Tower.cpp:217-222` justifies keeping `IsAcquirableEnemy`'s team term with *"the chain-bounce search calls the same gate on a candidate it did not gather"* — but after this refactor **both** callers (`:242` and `:400`) feed from the same gather, so the term is now redundant in both, not just idempotent in one.** ⛔ **Keeping it is right** (it is the §3.7 class gate's home and it is defence in depth); only the sentence is stale.
- **[NIT] N-4 — `Tests/SiegeGhostPawnTest.cpp:107-111` still says *"eight enumerations"* with pre-refactor line numbers (`SummonedUnit.cpp:1717`, `HeroCharacter.cpp:404`).** Pre-existing prose in a file this task **correctly did not touch** (editing it would have been the automatic FAIL under (b)). Flagged only so a later task fixes it deliberately — it now describes a world with nine enumerations that no longer exists.

---

## Notes for build-master (`TASK-835` / `TASK-843`)

1. ✅ **`qa/TASK-848.md` = PASS covers `TASK-828` and only `TASK-828` (`SC-§29`).** `827`/`837` are `qa/TASK-847.md`; `829`+ are `849`/`850`. Do not read this gate as covering a file — it covers a task.
2. ⛔ **Not compiled.** The three queued compiles still have to happen, and `Build.bat` **returns exit 0 on a failed build** — parse the log for `Result: Failed`, never trust `$LASTEXITCODE`. A compile finding here is a build-time event and comes back through the normal loop, not through this verdict.
3. **Expected suite delta from this task: `+9`** (`Tests/SiegeAcquisitionFunnelTest.cpp`). My census at gate time: **376 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` across 29 files in `Source/`** (anchored grep; `IMPLEMENT_PRIMARY_GAME_MODULE` excluded). Reconcile as `prior + Σ(declared deltas)`; ⛔ do not carry any spec's absolute.
4. ⭐ **Test 1 is a repo-shape gate, not a logic test** — it reads files off disk via `FPaths::ProjectDir()`. It will fail in any environment where `Source/` is not present next to the `.uproject` (a cooked/packaged run). It is `EditorContext | EngineFilter`, which is correct; keep it out of any packaged smoke lane.
5. **Watch `W-4` at integration:** the only lane that can catch a live acquisition regression is a PIE/human pass (a unit acquiring, a tower chaining, a Fireball, and **Battle Cry actually buffing something**). Battle Cry is the single most valuable thing to eyeball in this commit.
6. Nothing here is Git-blocking: 0 blockers, no edits requested of the programmer. **W-1 and W-2 are for the manager** (a `WITCH-§2` residual sentence and a `FOG-§7` row-4 wording fix); W-3/N-1..N-4 are cheap follow-ups that belong to whichever task next opens these files — ⛔ **not** re-work of `TASK-828`.
