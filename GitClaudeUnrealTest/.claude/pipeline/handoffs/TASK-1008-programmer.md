# TASK-1008 — [FOG-RETAIN] fog stops binding acquisition alone: retention, firing and the commanded notice bound all pass through one unit-side ceiling — plus the guard extension that would otherwise have certified this diff without reading it

**Status:** ready-for-qa · **Gate:** ⭐ `TASK-1039` · **Author:** gameplay-programmer · 2026-09-05
**Inputs consumed:** `handoffs/TASK-1007-programmer.md` (the seam) · `qa/TASK-1009.md` **W-5** (the binding instruction) + W-8 · `handoffs/TASK-980-programmer.md` §2/§4(a)(c)(f) · `handoffs/TASK-978-programmer.md` (the census; its §4(c) item 2 **refuted**, not followed) · `qa/TASK-1006.md` item (5) (**re-confirmed**, not relayed)

---

## §0 — ⭐⭐ THE HEADLINE, AND THE HALF THAT MATTERS MOST IS THE SECOND ONE

1. **THE WIRING.** Nine unit-side reaches now pass through **one** door,
   `ASummonedUnit::ApplyFogVisionCeilingUU(float RequestedReachUU) const`, which forwards to
   `TASK-1007`'s seam and does nothing else. **Six firing gates + two leash drops + one commanded
   notice bound.** With the fog down every one is **bit-identical** to the code that shipped; with
   it up every one is `min(own reach, 609.6)`.
2. ⛔⛔ **THE GUARD.** `Tests/SiegeAcquisitionFunnelTest.cpp` test 9 was **measured GREEN over this
   entire finished diff** with its unextended token list — `qa/TASK-1009.md` W-5 confirmed
   empirically rather than predicted. It has been **extended** (never relaxed, never relocated),
   carved out **only** through a derived authorised-chokepoint table, capped at **exactly one** call
   per file inside the named function, **and seen RED against three mutations.**

⇒ ⚖️ ***The clamp is a two-line change. The reason this row is large is that the thing which was
supposed to be watching it could not see it, and a guard that passes a change by accident does not
merely fail to catch — it certifies.***

---

## §1 — WHAT CHANGED, BY FILE

| file | change | tracked? |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | the **one** chokepoint declaration + its doc · **4 prose blocks re-derived** (see §4) | modified |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | the chokepoint **definition** (1 statement) · **9 call sites** wired · **4 prose blocks re-derived** | modified |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAcquisitionFunnelTest.cpp` | **test 9 ONLY** — the token, the authorised-chokepoint table, the derived per-file expectation, the per-entry half, one new immune needle, 3 stale paragraphs repaired | modified |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeUnitNoticeRangeTest.cpp` | ⚠️ **ONE needle + its message** — a **declared deviation**, §9 | modified |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRetentionWiringTest.cpp` | **NEW FILE — 4 tests** | ⛔ **UNTRACKED — the commit host must `git add` it** |

⛔ **Zero edits to** `SiegeCombatStatics.*` (**called, not changed**) · `SiegeFogStatics.*` · `FogVolume.*` ·
`Docs/Data/cards.csv` · `DT_Cards` · `Tower.cpp` · any other test in `SiegeAcquisitionFunnelTest.cpp`.
**No compile, no engine, no MCP, no mutating Git** (read-only `show`/`status`/`diff`/`log` only).

**Diff:** `+439 / −46` across the four tracked files, plus a 724-line new test file. `HEAD = 7e3e883`.

---

## §2 — THE NINE REACH SITES, AND THE FIVE CONSUMERS THAT MUST **NOT** BE ROUTED

⛔ **Every coordinate below was located by SYMBOL. The dispatch's reported line numbers had all
drifted** — the census's `:1740 / :1900 / :1966 / :2071 / :2559 / :3941` were live at
`:1771 / :1945 / :2015 / :2120 / :2689 / :4071` before my diff, and have moved again since.

### Routed (9) — measured, per body, with the house scanner's exact semantics

| # | enclosing symbol | expression | class |
|---|---|---|---|
| 1 | `ASummonedUnit::UpdateState` | `ApplyFogVisionCeilingUU(GetEffectiveLeashRangeUU())` | ⭐ **RETENTION 1 of 2** |
| 2 | `ASummonedUnit::UpdateState` | `ApplyFogVisionCeilingUU(AttackRange)` | FIRING 1 |
| 3 | `UpdateStateStandardCommanded` (ATTACK) | `ApplyFogVisionCeilingUU(GetEffectiveLeashRangeUU())` | ⭐ **RETENTION 2 of 2** |
| 4 | `UpdateStateStandardCommanded` (DEFEND) | `ApplyFogVisionCeilingUU(AttackRange)` | FIRING 2 |
| 5 | `UpdateStateStandardCommanded` (ATTACK) | `ApplyFogVisionCeilingUU(AttackRange)` | FIRING 3 |
| 6 | `UpdateStateGrouped` | `ApplyFogVisionCeilingUU(AttackRange)` | FIRING 4 |
| 7 | `UpdateStateSiege` | `ApplyFogVisionCeilingUU(AttackRange)` | FIRING 5 (**inert on data, routed anyway**) |
| 8 | `ASummonedUnit::PerformAttack` | `ApplyFogVisionCeilingUU(AttackRange)` | ⭐ **FIRING 6 — the one that stops the arrow** |
| 9 | `AcquireEnemyNearPoint` | `ApplyFogVisionCeilingUU(GetEngagementRadiusUU())` | ⭐ **the commanded NOTICE bound** |

⭐ Site 9 was **routed, not added**: the bound is `TASK-979`'s and is unconditional (*"due to fog OR
ANYTHING"*). **`SummonedUnit.cpp` itself told me to do this** — two shipped paragraphs read *"TASK-980
later routes that already-existing bound through the fog-aware accessor"*, and this row is the
surviving half of `TASK-980`'s split. Both sentences are now past tense (§4).

### ⛔ NOT routed (5) — pinned at **zero** ceiling calls, each with the bug it would cause

| enclosing symbol | why raw |
|---|---|
| `FindNearestDamagedFriendly` | the Cleric's heal **candidate filter** — a heal is not an act of shooting |
| `PerformHeal` | the Cleric's per-tick heal **re-validate** — the `\|\|` tail of a six-term chain, the site a census skips |
| `ResolveWitchPositionCircle` | ⛔⛔ **the WITCH'S VEIL RADIUS** (`FOG-§9.10b`) — routing it makes the **fog card silently shrink the invisibility card** |
| `EnterAdvance` | the **approach distance** `MoveToActor(Goal, max(AttackRange × 0.8, 40))` — would change how close every ranged unit *walks* under fog, with nothing in any log |
| `AcquireTarget` | ⭐ **the sharpest of the five.** It is **already** fog-clamped inside the funnel. A unit-side clamp too is arithmetically harmless (`min` is idempotent) and **structurally fatal** — a second guard point on the acquisition path |

### ✅ AMBUSH / HOLD / FOLLOW — exempt **by construction**, with **zero** exemption branches

⛔ **Re-confirmed at source at my own instant, in the CALL GRAPH, not by a name census** (`FOG-§9.11`
records that a name census answers a different question). Measured as byte offsets inside
`UpdateState`'s extracted body:

```
UpdateStateFollow(*FollowGroup);   @10364   →  its `return;` @10401
UpdateStateGrouped(*Group);        @13160   →  its `return;` @13191
the retention line                 @17365
```

⇒ both dispatches `return` **unconditionally before** the leash read, so those lanes **cannot execute
one on any path**. `UpdateStateGrouped`'s own body holds **zero** `LeashRange` and **zero** retention
terms — measured, and asserted. ⛔ **I added no `if`, and an `if` here would have been a second
exception layered on a structural one.** New file test 4(c) pins the ordering, so a future refactor
that lets either dispatch fall through goes red *before* an AMBUSH unit silently acquires a fog leash.

---

## §3 — ⛔⛔⛔ THE GUARD EXTENSION, AND IT WAS **SEEN RED**

### (a) First, the debt measured rather than asserted

I ran test 9's logic with its **unextended** three-token list over my **finished** diff:

```
test 9 with the OLD token list  =>  GREEN (0 failing rows)
```

⇒ ⛔ **`qa/TASK-1009.md` W-5 is confirmed empirically. The guard would have passed nine fog reach
sites without looking at one of them.** That is the measurement, not the prediction.

### (b) The extension — a **described door**, never an exemption

The token list gained `ResolveFogClampedReachUU(`, and the loop's expectation stopped being the
literal `0`. It is now **derived per (file, token) pair** from:

```cpp
struct FAuthorisedFogCeilingChokepoint
{
    const TCHAR* File;                         // one of CallSiteFiles
    const TCHAR* Token;                        // WHICH token this entry authorises — one, never all
    const TCHAR* ChokepointSignature;          // the ONE function that may hold it
    const TCHAR* WhyThisChokepointIsAuthorised;// printed into every message this table produces
};
```

**One entry today.** Twenty (file, token) pairs exist; nineteen derive to `0`, one derives to `1`.
⭐ **That one is the per-file cap**, and it is what makes "one door" a property rather than an
aspiration — a second site would read `2` against a derived `1`.

**Plus the per-entry half**, which is the part that makes the table a *claim* rather than arithmetic:
each entry is re-asserted **inside the one function it names** (`ExtractFunctionBody` by signature; a
stale signature `AddError`s rather than scanning nothing), with a prose-immunity row beside it.

⭐ **An authorised consumer is added by writing down a reason. The number follows.** `handoffs/TASK-980-programmer.md` §4(f), applied verbatim.

### (c) ⛔⛔ SEEN RED — three mutations, run **on disk**, each restored **byte-exact**

`SummonedUnit.cpp` baseline `sha256 = f7534183b2c89ca504b07eb217518a2a46f79c9e7706f8a432afa9f55e943c6c`,
and the **final on-disk hash is the same value** — verified after every restore.

| mutation | funnel **test 9** | new-file **test 2** |
|---|---|---|
| **M1** — a **second seam call at a random reach site** (`UpdateStateSiege` clamps for itself, bypassing the door). *The mutation the board names.* | ⛔ **RED** — file-scope: `got 2, derived-expected 1` | ⛔ **RED** ×2 — per-body `UpdateStateSiege` `got 0, expected 1`; derived total `9 ≠ 10` |
| **M2** — the seam call **MOVES** out of the door into a reach site. **The file still holds exactly one seam call.** | ⛔ **RED** — and **only at the per-entry row**: `'ResolveFogClampedReachUU(' inside ApplyFogVisionCeilingUU: got 0, expected 1` | ⛔ **RED** ×2 |
| **M3** — a **tenth** ceiling call on a **non-firing** consumer (the **Witch's veil radius**) | ✅ green — **correctly**, see below | ⛔ **RED** ×2 — derived total `11 ≠ 10`; `ResolveWitchPositionCircle: got 1 ceiling calls, expected 0` |

⭐⭐ **M2 is the one worth reading.** Its file-scope count is *right* and its placement is *wrong* —
exactly the shape a bare exemption would license — and **only the per-entry row catches it.** Without
that half the table would have been arithmetic.

⚠️ **M3 is declared honestly rather than presented as a gap.** Test 9 is the **one-door** gate: a
`ApplyFogVisionCeilingUU(` call is not a seam call, so it is correctly silent. The **reach census** is
the other gate, and it fires. The two guards divide the work deliberately; **neither alone covers
both mutations**, and QA should confirm it agrees with that division.

### (d) What I did **not** do

⛔ I did not relax the token list. ⛔ I did not relocate the rule to dodge the guard. ⛔ I did not bump a
number. ⛔ I did not rename the registered test (§8 item 1).

---

## §4 — THE PROSE MY OWN DIFF FALSIFIED, RE-DERIVED RATHER THAN PATCHED

⭐ Applying `TASK-1007`'s own ruling (*a row must re-census the file **after** its own diff*), I
censused every fog claim in **both** my files and found **eight** false or newly-incomplete sites.

**`SummonedUnit.h`**
1. **The class-doc state machine summary** — `Attack`/`Reacquire` described unclamped reaches. Now
   states both are fog-ceilinged and that fog is a **standing condition**, not an acquisition filter.
2. **`UnitEngagementRadiusUU`'s doc** — *"the one universal ceiling … applied as a `min` at the ONE
   chokepoint **inside the acquisition funnel**"*. **Short by one place after my diff.** Re-derived to
   name **two** chokepoints and to keep the load-bearing half (*"never one **here**"*) verbatim.
3. **`ResolveEffectiveLeashRangeUU`'s doc** — said the clamp *"is NOT implemented here and MUST NOT
   be … this file is pinned at ZERO fog symbols … the clamp lands on its own row (TASK-1008)."*
   **This row is TASK-1008.** Re-derived: the clamp is **wired**, it is still **not implemented in
   that pure function** (that half must never change), and the zero-fog-symbols sentence is **retired**
   and replaced with what the guard now actually pins.
4. **`AcquireEnemyNearPoint`'s declaration doc** — *"TASK-980 routes THIS EXISTING READ…"* ⇒ done, by
   number, with the intruder/order split written down. Plus a **precision rider** on
   `GetEngagementRadiusUU`'s *"never a global 'ceiling' constant"*, which a reader would otherwise
   read as contradicted by a symbol named `…CeilingUU`.

**`SummonedUnit.cpp`**
5. `AcquireTarget`'s fog paragraph — *"the ceiling is applied at the ONE place, inside the funnel"* ⇒
   narrowed to **ACQUISITION**, plus the rule that decides which sites route where, and **why this
   site is deliberately NOT routed**.
6. The same paragraph's *"the ceiling arrives through the ONE chokepoint"* ⇒ **the ACQUISITION chokepoint**.
7. + 8. `AcquireEnemyNearPoint`'s two *"TASK-980 later routes…"* sentences ⇒ past tense, with the
   fence that the **gather stays unbounded** and the bound must never be re-added beside itself.
9. `UpdateStateGrouped`'s opening — a rider stating that **this row deliberately changed nothing
   there**, so the absence reads as design rather than as an oversight.

⇒ ⚖️ ***The most likely author of the next false sentence is the repair itself — and here it was: four
of the eight were falsified by lines this row wrote.***

---

## §5 — TESTS AND THE SUITE DELTA (`TL-§5b`/`§5c`)

**`Tests/SiegeFogRetentionWiringTest.cpp` — NEW, 4 tests, ⛔ UNTRACKED.**

| # | registered name | asserts |
|---|---|---|
| 1 | `…TheCommandedOrderSurvivesTheFogAndTheIntruderIsStillRefused` | ⭐⭐ **the step ORDER is the assertion.** STEP 1 the order survives (grouped body: zero retention, zero `LeashRange`, zone machinery alive as a positive control, **and** exactly one firing term so an empty diff cannot pass); **then** STEP 2 the intruder is refused — the notice bound routed, the disc surviving, `bRangedAttack` still absent, and the arithmetic **executed** at both fog states. ⛔ The intruder's distance is **derived** (`½(ceiling + notice)` = 2804.8), never picked, with a precondition row so the pair cannot go vacuous if either number is retuned |
| 2 | `…TheCeilingReachesNineSitesAndTheNonFiringConsumersAreUntouched` | the **nine sites from a table with a WHY per row**, asserted **per body**; the tree-wide total **derived** (`1 definition + 9`); and the **five** consumers pinned at zero, each with the bug a clamp would cause **and** a positive control proving the body was really read |
| 3 | `…TheFiringGateCannotExpressTheMeleeDropSoRetentionCarriesIt` | ⛔ the **melee proof, executed**: `min(120, 609.6) == 120` at **both** boolean values, with the Longbowman's `3600 → 609.6` as the discriminator proving the ceiling is live ⇒ therefore **retention** must carry `J-F23`, and both retention sites are pinned |
| 4 | `…TheUnitSideCeilingIsOneDoorAndAmbushIsExemptByConstruction` | one seam call in the file **and inside the door**; the door adds **zero** `FMath::Min`, **zero** `609`, **zero** branches; and the **AMBUSH call-graph ordering** of §2 |

### Delta

- **Last executed:** `481` tests / `0` failures at `7e3e883`.
- **Static census before this row:** `485` / `41` files (`TASK-1007`'s `+4`, uncommitted).
- **This row:** **`+4` tests, `+1` file** ⇒ **`489` tests / `42` files.**
- ⛔ **DECLARED, NEVER EXECUTED.** No compile was run; the fence forbids it. Counts derived by counting
  `IMPLEMENT_SIMPLE_AUTOMATION_TEST(` across `Siegebound/Tests/` (`489`) and its `.cpp` files (`42`).
- ⚠️ **TWO untracked test files must be staged by explicit pathspec** — `SiegeFogReachSeamTest.cpp`
  (`TASK-1007`'s) **and** `SiegeFogRetentionWiringTest.cpp` (mine). `git add -u` and `commit -a` miss
  both, and the suite count would be wrong by eight.

### ⚠️ THE HONEST GAP, STATED SO IT IS NOT DISCOVERED

⛔ **No unit is ticked anywhere in this suite. Nothing here observes a target actually being dropped.**
It observes that the one expression capable of dropping it is present, at both sites, reading the right
reach through the right door, and that the arithmetic that expression performs returns the numbers his
ruling names. There is no `UWorld::CreateWorld` and no `SpawnActor` in `Siegebound/Tests/` (house rule),
so the seam's fog-**on** branch cannot be executed; test 3 executes the **rule** it is structurally
pinned to delegate to, at both values of the one boolean. ⛔ **Green here is "retention is wired to
fog", not "a unit drops its target in a running match".** That second sentence needs a playtest and is
**owed to Jonathan**, not claimed by me.

---

## §6 — ⚠️ THE PERFORMANCE ITEM (`qa/TASK-1009.md` W-8): **MEASURED AND ROUTED, NOT FIXED**

`AFogVolume::Find` is an uncached full-world `TActorIterator<AFogVolume>` per call, and it is reached
once per `ReadFogState`. **Nine call sites do multiply it.** Derived from the call graph, counting
`Find` walks per unit per `0.25 s` state poll (4 Hz):

| lane | before | after | delta |
|---|---|---|---|
| Standard, uncommanded | 1 (`AcquireTarget`) | **3** | +2 |
| Commanded ATTACK | 1 | **3** | +2 |
| Commanded DEFEND | 1 | **3** | +2 |
| **Grouped (worst: upgrade + 2 tiers)** | 3 | **7** | **+4** |
| Siege | **0** | **1** | +1 — *a lane that never paid before* |
| `PerformAttack` | 0 | **1 per attack cadence tick** | +1/cadence |

⇒ worst case **×2.33** on the grouped lane; ≈**1,600** extra full-world actor iterations per second for
100 grouped units.

⛔⛔ **THE NON-OBVIOUS HALF, AND IT INVERTS THE INTUITION: the cost is HIGHEST WHEN THERE IS NO FOG.**
`Find` returns on the **first** valid volume, so with fog up the walk terminates early — but with **no
`AFogVolume` in the world at all**, which is every match until the Fog card is first cast, it walks
**every actor, every time**. The common case is the expensive one.

**What I did:** hoisted the ninth site's evaluation **above** the candidate loop (one ceiling call per
`AcquireEnemyNearPoint` call, not per hostile on the field) — the only reduction available without
caching. ⛔ **I did not cache the returned reach** — that breaks `FOG-§9.6` (the value must be live;
fog can rise or clear between two polls) and would re-collapse three gates onto one stale number.
⛔ **I did not edit `FogVolume.cpp`**: it is not in this row's `names:` and is `TASK-998`'s file.

**Recommendation, for a row that owns that file:** cache **inside `Find`** — a per-world
`TWeakObjectPtr<AFogVolume>` or a `UWorldSubsystem`, invalidated on spawn/destroy. ⚠️ **The absolute
cost is UNMEASURED** (it needs a profiled match, not a code read) and is stated as unmeasured rather
than guessed.

---

## §7 — ⛔ WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐⭐⭐ **The guard division of labour (§3(c), M3).** Test 9 is the **one-door** gate and is correctly
   silent on a tenth *ceiling* call; the **reach census** catches it. I believe that split is right —
   test 9 is about the fog RULE's chokepoint, not about `ASummonedUnit`'s internal call count — but
   **it means neither guard alone covers both mutation classes, and both live in this diff.** If QA
   wants one gate to cover both, say so: it is a table entry, not a rewrite.
2. ⭐⭐ **The `SiegeUnitNoticeRangeTest.cpp` deviation (§9).** One needle, one message, in a file not in
   my `names:`. It was **forced**: the old needle reads `0` against **correct** code. I judged
   "strengthen it in the same commit" strictly better than "leave a known red" or "delete the row".
3. ⭐⭐ **The ninth site.** The dispatch's *"What to wire"* named only RETENTION and FIRING. I routed the
   commanded **notice** bound as well, on three independent grounds: the board's item (1) says *"ALL
   NINE REACH SITES"*; item (6)'s commanded test **requires** the intruder to be refused, which is
   only expressible there; and **the shipped source names this row's predecessor as the router, twice**.
   **If QA rules it out of scope, it is one line to revert — but item (6)'s test goes with it.**
4. ⛔⛔ **The substring hazard my symbol created, found by a before/after needle sweep over every
   `TEXT("…")` in every test that scans this file (819 candidates, 16 moved).** `ApplyFogVisionCeilingUU`
   **contains** `FogVisionCeilingUU`, and `ResolveFogClampedReachUU` **contains** `Clamp`. Three live
   pins take those bare needles: `FogVisionCeilingUU == 0` in `FogVolume.{h,cpp}` and in
   `SiegeGameMode.cpp`; `Clamp == 0` inside `ResolveNoticeRadiusUU`'s body. ✅ **All three are safe
   today — verified at source, body- or file-scoped, none in a file this row touches** — but a future
   row routing a reach through the door **inside one of those scopes** would trip a pin that means
   *"no notice clamping"* / *"no ceiling coupling"*, not *"no fog door"*. **Recorded so it is a finding
   rather than a discovery.**
5. **Every tree-wide fog pin re-measured after my diff** (238 shipping files, `Tests/` excluded exactly
   as `CountAcrossShippingSource` excludes it): `ReadFogState(` **4** · `FSiegeFogStatics::EffectiveVisionRadius(`
   **3** · `EffectiveVisionRadius(` **5** · `FogDensityAt(` **2** · `FSiegeVisionQuery::Seeing` **5** ·
   `GetAllActorsWithInterface` **1**. ⛔ **All unmoved** — my door adds no fog-state read and no vision
   query. ⚠️ Please re-derive rather than inherit: my new test file calls
   `FSiegeFogStatics::EffectiveVisionRadius` five times and is invisible to that pin **only because**
   the scanner skips `Tests/`. I confirmed that skip at source before relying on it.
6. **The melee no-op is the correct answer, not a missing clamp.** `min(120, 609.6) = 120` at all six
   firing gates. A reviewer scanning for "did fog change melee firing?" should expect **no**, and
   should check `RETENTION` instead — that is where `J-F23` lives for melee.
7. **`UpdateStateSiege` is routed although provably inert** (Sapper/Ogre `Range` 120). Deliberate: an
   exemption list would replace a structural rule. It is also the **suicide detonation** trigger.

---

## §8 — ⛔ RAISED, NOT EDITED (`SC-§62` cl. (ii) — the region is another row's, or the change is a different kind)

1. ⚠️ **Test 9's REGISTERED NAME `Siegebound.Acquisition.VeilAndFogSuppressionLiveOnlyInTheFunnel`.**
   The fog clamp now lives at **two** chokepoints, so "only in the funnel" reads narrow. I **did not
   rename it**: an automation test's registered name is referenced from outside the file (I checked —
   4 references, all handoffs/QA records, no config or filter), and renaming is a different kind of
   change from extending a token list. ⭐ I added a comment at the registration telling the reader to
   read "the funnel" as "the chokepoint". **This is the same shape `qa/TASK-1009.md` W-2 routed to
   `TASK-1000` for `SiegeFogClampTest.cpp` — please route it the same way.**
2. ⚠️ **`Tests/SiegeUnitNoticeRangeTest.cpp` test 2's failure message** repeats *"applied as a `min` at
   the ONE chokepoint inside the acquisition funnel (FOG-§7)"* — the **same sentence** my diff
   falsified in `SummonedUnit.h`, where I **did** repair it. It is prose inside a message, not an
   assertion, and it is in a different test from the one my deviation licence covers. **Left alone
   deliberately, to keep that deviation minimal.**
3. ⚠️ **`FogVolume.cpp`'s uncached `Find`** — §6. `TASK-998`'s file, not in my `names:`.
4. 📌 **`SiegeFogStatics.h:355`'s stale firing-gate line numbers** (`:1851, :1956, :2409`) were already
   wrong before this row and are **wrong by more now**. `SiegeFogStatics.*` is explicitly out of scope.
   `SC-§38` in miniature; a manager row.
5. 📌 **`FOG-§9.11`'s own retention bullets cite `SummonedUnit.cpp:2024` and `:1979`** for the grouped
   comment and leash site 2. Both have moved. Law text, not mine to edit.
6. 📌 **`FOG-§9.6b`'s table still shows `2000` for both notice and firing.** `FOG-§9.11` supersedes the
   numbers, and the table says so elsewhere — but the table itself reads current. A manager row.

---

## §9 — FENCES HONOURED, AND ⚠️ **TWO DECLARED DEVIATIONS**

✅ Called `FSiegeCombatStatics::ResolveFogClampedReachUU` and **nothing else** — ⛔ `ReadFogState` stays
`private`, no wrapper, no `friend`, and I never reached past the seam · ✅ retention reuses the **two
existing** drop sites, **no new tick, no new cadence** · ✅ the clamp is a shared **`min`**, never a
collapse of notice into firing · ✅ AMBUSH exempt **by construction**, zero exemption branches · ✅ the
guard **extended**, never relaxed, never dodged, and **seen red** · ✅ located by **SYMBOL** throughout
(every reported coordinate in the dispatch had drifted) · ✅ **no compile, no engine, no MCP, no
mutating Git**; `checkout`/`restore`/`stash`/`reset`/`clean` never invoked · ✅ `TASK-1037`'s row and
`Content/FogArea/` untouched.

⚠️ **DEVIATION 1 — `Tests/SiegeUnitNoticeRangeTest.cpp` (not in `names:`).** My diff **falsifies** test
3(g): its needle `"> GetEffectiveLeashRangeUU()"` reads **0** against correct code once the ceiling
wraps the call. I changed **one needle and its message** to
`"> ApplyFogVisionCeilingUU(GetEffectiveLeashRangeUU())" == 2`, which is **strictly stronger** (it now
pins the effective leash **and** the ceiling at both sites), plus a comment saying why it moved.
⛔ **There is no expression satisfying the board's mandated form that leaves that needle intact**, and
the alternatives were to ship a known red or to delete an assertion. The file is **committed and clean**
at `7e3e883` and no live row owns it (`TASK-1003` finished; `TASK-1037` is git-side on `Content/`).
**A ruling is requested.**

⚠️ **DEVIATION 2 — the board row's `blocked-by:` line as well as `status:`.** It named three blockers,
all discharged, and said the row was *"NOT dispatchable"* — false at the moment it was dispatched.
⛔ The diff is **2 lines, both inside `TASK-1008`'s own row**, verified with `git diff -U0`
(hunk `@@ -19510,2 +19630,2 @@`); every other board hunk was **already dirty before I started**.
⭐ `qa/TASK-1009.md` **accepted this exact deviation** for `TASK-1007` and its **N-4** asked the manager
to widen the fence to say so. Declared, not assumed.

⚠️ **Also not in `names:`: nothing else.** The new test file **is** licensed (`names:` carries *"a test
in `Tests/`"*).

---

## Notes for the commit host

1. ⛔⛔ **STAGE BOTH UNTRACKED TEST FILES BY EXPLICIT PATHSPEC.** From the **repo root**, which sits
   **above** the project dir:
   ```
   git add GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRetentionWiringTest.cpp
   git add GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogReachSeamTest.cpp
   ```
   `git add -u` and `git commit -a` **miss both**. A commit without them ships **eight absent tests** and
   a suite count wrong by eight.
2. **This row's change set:** `SummonedUnit.h` · `SummonedUnit.cpp` · `Tests/SiegeAcquisitionFunnelTest.cpp` ·
   `Tests/SiegeUnitNoticeRangeTest.cpp` (modified) · `Tests/SiegeFogRetentionWiringTest.cpp` (**new**).
   ⚠️ The tree is **also** dirty from `TASK-1007` (`SiegeCombatStatics.{h,cpp}`, `Tests/SiegeFogClampTest.cpp`,
   `Tests/SiegeFogReachSeamTest.cpp`) and from earlier rows. **Stage by pathspec, never `-a`.**
3. **Expected suite after compile: `489` tests across `42` files.** ⛔ **Nothing here has been executed.**
   Last executed was `481 / 0` at `7e3e883`. **A red on first execution is a finding, not a surprise.**
4. **New symbol the compiler sees for the first time:** `ASummonedUnit::ApplyFogVisionCeilingUU` —
   public, non-static, `const`, one `float` parameter, **not** a `UFUNCTION`. No new include (the file
   already includes `Siegebound/SiegeCombatStatics.h`), no `Build.cs` change, no reflection, no GC
   surface. It has **9 callers**, so no unreferenced-static warning.
5. **New test file compile surface:** a named fixture namespace (`SiegeFogRetentionWiringFixture`, the
   house idiom — **no file-scope `using namespace`**; the directive appears only *inside* each
   `RunTest`, the form `qa/TASK-1019.md` **BLOCKER-1** measured as correct — 34 test files use it, **0**
   use file scope, and a file-scope one leaks for the rest of the unity TU). The namespace is **named,
   not anonymous**, so its `CountOccurrencesInCode` / `ExtractFunctionBody` / `LoadProjectFile` cannot
   collide with the identically-named helpers in `SiegeUnitNoticeRangeFixture` or
   `SiegeAcquisitionFunnelFixture` — that is `qa/TASK-1019.md` BLOCKER-2's failure mode, avoided by
   construction. Includes `Siegebound/SummonedUnit.h` and `Siegebound/SiegeFogStatics.h`.
6. **Nothing in this handoff blocks a commit**, provided (1) is honoured and `TASK-1039` passes.
