# TASK-978 — the `AttackRange` / `AggroRadius` census (DIAGNOSE-ONLY)

**Agent:** gameplay-programmer · **Date:** 2026-09-04 · **Status:** ready-for-qa
**Writes:** this file only. ⛔ Zero `Source/` edits, zero tests, zero assets, zero law, zero engine/MCP, zero Git writes. `git status` re-read at completion confirms no tracked source file changed.

---

## §0 — INSTRUMENTS, AND WHAT EACH ONE IS WORTH (`SC-§39`)

⚠️ **Repo root is `C:/GitProjects/GitHub/GitClaudeUnrealTesting`, one level ABOVE the `.uproject`** (`SC-§56`). Every path below is written repo-relative from `GitClaudeUnrealTest/`. I used filesystem `grep`, not `git grep` with a pathspec, so the silent-`--` trap could not fire; I state the root anyway because it is the thing that has bitten five times.

| instrument | control run | result |
|---|---|---|
| `grep -rn` over `Source/` | **positive:** `class GITCLAUDEUNREALTEST_API ASummonedUnit` → 1 hit at `SummonedUnit.h:188` | ✅ instrument live |
| same | **negative:** `ZzQqAttackRangeNotARealSymbol` → **0** | ✅ not hallucinating hits |
| same | **substring trap:** `[A-Za-z_]*AttackRange[A-Za-z_]*` → the only distinct token is `AttackRange`; same for `AggroRadius` | ✅ **no `GetAttackRange()` accessor exists**, so a name census is complete for direct reads |
| same | **recompute trap:** `Row(\.\|->)Range` swept separately, because a site that re-reads the ROW bypasses a member-name census | ✅ 2 writes + 1 tower validation + 10 UI-text reads in `DeckBuilderWidget.cpp`; **no second firing path** |
| `.uasset` binary scan (Python, ASCII **and** UTF-16LE) over 4,379 files | **positive:** `SummonedUnit` → 12 `BP_Unit_*.uasset`; **negative:** `AggroRadius` → 0, `AttackRange` → 0 | ✅ **no Blueprint overrides either default** |

⚠️ **The `.uasset` result carries a stated residual.** My first attempt used `grep -rl` over `Content/` and returned **0 for the positive control too** — an uncontrolled instrument that would have produced a confident false "no overrides". I discarded it and rebuilt with an explicit control. The rebuilt scan is sound for *uncompressed* name-table entries; a fully compressed package could still hide a name. Confidence: high, not absolute.

⚠️ **`SC-§55`:** my session-start `gitStatus` snapshot was stale — it showed a castle-era tree. Re-read live: `HEAD = 84eec02`, branch `main`.

⚠️⚠️ **A CONCURRENCY HAZARD I HIT MID-CENSUS, DECLARED BECAUSE IT AFFECTS WHAT MY READS ARE WORTH.** `SiegeFogStatics.{h,cpp}` and `Tests/SiegeFogTest.cpp` **changed under me while I was reading them** — `TASK-981` (the Beer-Lambert falloff row) is editing them in parallel. I verified this is not my own writing (I called `Write` once, on this file, and `Edit` once, on my own board row) and, more importantly, I checked **whether the function my census depends on moved**: `git diff -U0` shows **every hunk is inside `FogDensityAt`**; **`EffectiveVisionRadius` is untouched** and appears in the diff only inside new comments. ⇒ ⭐ **§3(b) and §6 stand.** ⛔ But QA should re-confirm rather than inherit this: a census read from a working tree another agent is writing is only as good as its last check.

---

## §1 — ⛔⛔ THE HEADLINE: THE `AttackRange` CONSUMER CENSUS

### **14 executable read SITES · 9 distinct CONSUMERS. The two numbers DIFFER, and the difference is the whole point** — five sites are one consumer (the state-machine range test), so a per-site fix list is not a per-consumer fix list.

Totals, declared so QA can reproduce: **33** textual occurrences of `AttackRange` in the module; **5** are in `Tests/`; of the remaining 28, **9** are comments, **2** are member declarations, **2** are writes, **1** is a `UE_LOG` format argument, leaving **14 executable reads**.

| # | `file:line` | enclosing symbol | consumer | class | ⛔ what a blind clamp would do |
|---|---|---|---|---|---|
| 1 | `SummonedUnit.cpp:1740` | `ASummonedUnit::UpdateState` | state-machine "in range → `EnterAttack`" | ✅ **FIRING** | the intended effect |
| 2 | `SummonedUnit.cpp:1900` | `UpdateStateStandardCommanded` (defend branch) | same | ✅ **FIRING** | the intended effect |
| 3 | `SummonedUnit.cpp:1966` | `UpdateStateStandardCommanded` (main branch) | same | ✅ **FIRING** | the intended effect |
| 4 | `SummonedUnit.cpp:2071` | `UpdateStateGrouped` | same | ✅ **FIRING** | the intended effect |
| 5 | `SummonedUnit.cpp:2559` | `UpdateStateSiege` | ⚠️ same test, but it triggers the **Sapper SUICIDE detonation** | ✅ **FIRING** | inert (Sapper `Range` 120 < 609.6 < 2000) — but it **must still be routed**, or the exemption becomes a list |
| 6 | `SummonedUnit.cpp:3941` | `ASummonedUnit::PerformAttack` | ⭐ **the actual shot gate**, on the attack timer | ✅ **FIRING** | the intended effect — ⛔ **this is the one that stops the arrow** |
| 7 | `SummonedUnit.cpp:2729` | `FindNearestDamagedFriendly` | Cleric heal-target candidate filter | ⛔ **HEAL** | shrinks the Cleric's heal reach. Inert on shipped data (400 < 609.6) ⇒ **`SC-§40` cl. 10: a data-drawn test cannot tell a correct exemption from a broken one** |
| 8 | `SummonedUnit.cpp:2825` | `ASummonedUnit::PerformHeal` | per-tick heal-target re-validate | ⛔ **HEAL** | same, second site — ⛔ **easy to miss; it is the `\|\|` tail of a 6-term validity chain** |
| 9 | `SummonedUnit.cpp:3137` | `ResolveWitchPositionCircle` | ⛔⛔ **THE WITCH'S VEIL RADIUS** (two reads on one line) | ⛔⛔ **VEIL RADIUS** | ⛔⛔ **THE FOG CARD SILENTLY SHRINKS THE INVISIBILITY CARD.** Two live callers (`:3098`, `:3378`). Verified at source — the dispatch's warning is **CONFIRMED, not inherited** |
| 10 | `SummonedUnit.cpp:3726` | `ASummonedUnit::EnterAdvance` | `MoveToActor(Goal, FMath::Max(AttackRange * 0.8f, 40.f))` | ⛔ **APPROACH** | changes how close every ranged unit **walks**, with fog up and **nothing in any log** |
| 11 | `Tower.cpp:235` | `ATower::AcquireTarget` | reach handed to `SeeingFrom` | ⚠️ **ACQUISITION** | already fog-clamped inside the funnel — ⛔ **a second clamp here is `FOG-§7`'s one-chokepoint violation** |
| 12 | `Tower.cpp:240` | `ATower::AcquireTarget` | `RangeSq` — ⭐ **the tower's ONLY range gate** | ✅ **FIRING ≡ ACQUISITION** | see §5: for towers these are **one** gate, not two |
| 13 | `Tower.cpp:100` | `ATower::OnStatsLoaded` | `MinRange >= AttackRange` data-sanity warn | ⛔ **VALIDATION** | would fire a spurious "blind spot covers the whole range" warning under fog |
| 14 | `Tower.cpp:104` | `ATower::OnStatsLoaded` | `UE_LOG` format argument | ⛔ **DIAGNOSTIC** | prints a clamped number as if it were the authored one |

**Writes (not reads, listed so the census is closed):** `SummonedUnit.cpp:1288` `AttackRange = Row->Range;` · `Tower.cpp:61` `AttackRange = Row.Range;`.

### ⇒ The routing verdict for `TASK-980`
- **Route through the effective-firing accessor: sites 1–6** (ASummonedUnit) **and site 12** (ATower) — ⛔ *only if* `TASK-980` is licensed to touch `Tower.cpp`; see §5, because the tower's answer is not the unit's answer.
- **Leave reading `AttackRange` RAW: sites 7, 8, 9, 10, 13, 14.** ⛔ **Site 9 is the blocker `TASK-985` exists to catch.**

📌 **A defect found, written down, not fixed (per item 7):** `SiegeFogStatics.h:355` cites the firing gates as *"`:1851, :1956, :2409`"*. **All three line numbers are stale** — the gates now live at `:1740`, `:1966`, `:2071`. `SC-§38` in miniature. This is a comment-only drift and belongs to a manager row, not to me.

---

## §2 — THE PER-UNIT RANGE TABLE (machine-read column, `SC-§45`)

⚠️ **A trap that would have silently corrupted this table:** `Docs/Data/cards.csv`'s **first column header is EMPTY** — the `CardID` column is unnamed. A `DictReader` keyed on `'CardID'` returns `''` for every row and prints a table of blanks that *looks* well-formed. My first read did exactly that; I caught it and re-keyed on the real (empty-string) column. **Any agent re-deriving this table must key on `list(row.keys())[0]`.**

### (a) `bRanged = true` — the population a `bRangedAttack`-gated ceiling would touch

| card | `CardType` | `Range` | `MinRange` | ⇒ under **2000** | ⇒ under **609.6** (fog) |
|---|---|---|---|---|---|
| ⛔⛔ **Longbowman** | Unit | **3600** | 0 | ⛔ **2000 — −44.4 % NERF** | −83.1 % |
| ⚠️ **Archer** | Unit | **2100** | 0 | ⚠️ **2000 — −4.8 %** | −71.0 % |
| ⚠️ **Wizard** | Unit | **2100** | 0 | ⚠️ **2000 — −4.8 %** | −71.0 % |
| BallistaTower | Building | 1400 | **300** | ✅ unchanged | −56.5 % ⇒ a **300–609.6 annulus** |
| ArrowTower | Building | 900 | 0 | ✅ unchanged | −32.3 % |
| BombTower | Building | 800 | 0 | ✅ unchanged | −23.8 % |

### ⇒ **`FOG-§9.6`'s pre-read is CONFIRMED at source, not relayed.** Exactly three cards are cut by `2000`; one materially; **no tower is touched.**

### (b) ⚠️ THE `CrystalTower` DISCREPANCY — RESOLVED, AND IT IS A REAL TRAP

**`CrystalTower` ships `bRanged = FALSE`, `Range = 800`, `ChainTargets = 3`.** `FOG-§2`'s older table is **correct**; the `bRanged` census missed it because **`bRanged` is the projectile-DELIVERY flag, not an "is ranged" flag** — a chain tower zaps instantly and fires no projectile, so the column is legitimately false.

⛔ **Consequence `TASK-980` must not step on:** if the `2000` ceiling (or the fog firing clamp) is gated on `bRangedAttack`, **`CrystalTower` escapes it**. At `Range 800` the `2000` ceiling is moot — but the **fog** clamp is not: an ungated CrystalTower would keep firing to 800 uu under fog while every other tower is cut to 609.6. ⭐ **The tower lane is gated by the funnel, not by `bRanged`, so it is safe today** — but the moment anyone writes `if (bRangedAttack)` in the firing lane, this card is the hole.

### (c) melee + support — unchanged by BOTH numbers

Footman · Knight · MilitiaMob · Pikeman · Sapper · Cavalry · Ogre = **120** · Cleric = **400** · Witch = **400**. All sit below `609.6`, so `min()` returns them bit-identically in both states. ⇒ ⭐ **`FOG-§9.6`'s "melee unaffected by the number" is true for the RANGE; §4 shows it is NOT true for the BEHAVIOUR.**

### (d) 🧑 ⚠️ WHAT JONATHAN HAS NOT BEEN TOLD, AND SHOULD BE, IN ONE SENTENCE
> **Longbowman loses 44.4 % of its reach (3600 → 2000) and Archer/Wizard lose 4.8 % each. No tower changes. Longbowman is the card the elevation batch's ×3 created, so the `2000` ceiling is very nearly a Longbowman-only nerf.**

⭐ **But read §3(c) before he rules — the 3600 is mostly unreachable *today*, which changes what the nerf actually costs him.**

---

## §3 — THE ACQUISITION SIDE

### (a) ⛔ CLAIM 1 — **CONFIRMED, and the arithmetic holds**

**`SummonedUnit.h:1039` — `float AggroRadius = 600.f;`**, `UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))`, commented *"GDD §3.8 profile constant: 600 — not a card stat"*.

- ⛔ **It is NEVER assigned from the card row.** The stat-binding function is **`ASummonedUnit::LoadStatsAndStart` (`SummonedUnit.cpp:1242`)** — ⚠️ **there is NO symbol named `BindStats` anywhere in the module** (0 hits; the dispatch's name for it is informal). It sets `MaxHP`, `AttackDamage`, **`AttackRange = Row->Range` (`:1288`)**, `AttackCadence`, `bRangedAttack`, `Profile`, `AoERadius`, `bCharge`. **`AggroRadius` is absent.**
- **The complete set of assignments to `AggroRadius` in the entire module is 2**, both to `0.f`: `MinerUnit.cpp:72`, `SorcererUnit.cpp:23`. **No other class overrides it. No Blueprint overrides it** (§0 scan).
- ⇒ ⛔ **`min(600, 609.6) == 600`. The shipped acquisition clamp was INERT ON UNITS BY ARITHMETIC.** ✅ **Confirmed.**

### (b) The gather sites and what each hands over

| site | query | radius handed over | fog-clamped? |
|---|---|---|---|
| `SummonedUnit.cpp:1779→1782` `AcquireTarget` | `SeeingFrom` | **`AggroRadius`** (600 → 2000) | ✅ **yes** |
| `SummonedUnit.cpp:2362→2365` `AcquireEnemyNearPoint` | **`SeeingFromUnbounded`** | `TNumericLimits<float>::Max()` | ⚠️ **yes, but to the ceiling only** — it has no reach of its own |
| `Tower.cpp:235→238` `ATower::AcquireTarget` | `SeeingFrom` | **`AttackRange`** (row `Range`) | ✅ **yes** |
| `Tower.cpp:420→423` `FireChainZapAt` | **`SeeingFromUnbounded`** | `Max()` | ⚠️ same |
| `HeroCharacter.cpp:581→584` | `SeeingFrom` | `MeleeRange` | ✅ yes (inert — melee) |
| `SpellLibrary.cpp:298`, `:366` · `SpellLineSweep.cpp:141` · `SiegeCheatManager.cpp:142` | **no `Vision` argument at all** | — | ⛔ **NOT clamped — structurally cannot be.** ✅ This is what makes 🧑 `J-F9`'s hero-spell exemption **free** |

⭐ **The exemption is expressed by the ARGUMENT, not by a branch** — `SiegeCombatStatics.cpp:183` is a bare `if (Vision)`. `FOG-§7`'s structural law is genuinely honoured in the shipped code; `TASK-980` should copy this shape, not invent one.

### (c) ⛔⛔ THE FINDING NOBODY BOARDED: **`TASK-979` SILENTLY NEUTERS `LeashRange`**

**`SummonedUnit.h:1043` — `float LeashRange = 900.f;`** *"a target beyond this distance is dropped and Advance resumes"*. It is read at exactly **two** sites: `SummonedUnit.cpp:1713` (`UpdateState`) and `:1943` (`UpdateStateStandardCommanded`).

**Today `LeashRange (900) > AggroRadius (600)`, so the leash is a real leash:** a target chased past 900 is released and cannot be re-acquired, because re-acquisition only reaches 600.

⛔ **After `TASK-979` raises `AggroRadius` to 2000, the ordering INVERTS.** `UpdateState` runs drop-then-reacquire in one pass:
```
:1713   if (dist > LeashRange 900)  CurrentTarget = nullptr;   // released
:1721   if (AActor* A = AcquireTarget())  CurrentTarget = A;    // re-acquired, radius now 2000
```
⇒ **a target at 1,500 uu is dropped and immediately re-taken in the same 0.25 s poll.** The leash stops releasing anything it can still see. ⭐ **This is not a bug in `TASK-979`'s diff — it is a consequence of its number, and it is invisible in any test that only asserts the CDO default is 2000.**

⚠️ **Two live consequences:**
1. **With fog OFF:** units chase essentially without a leash out to 2000 uu. `LeashRange` becomes near-dead surface (`SC-§40` cl. 2 territory).
2. ⭐ **With fog ON this INVERTS AGAIN and works in our favour** — see §4.

📌 **Not my row to fix.** Recorded for the manager: either `LeashRange` moves with `AggroRadius`, or it is consciously retired, or `TASK-979` ships with the consequence written beside the number (`HIGH-§1`).

### (d) ⭐ AND THIS RESIZES THE NERF IN §2(d)
**A Standard, uncommanded Longbowman cannot fire at 3,600 uu today.** It acquires only within `AggroRadius` 600 and is leash-dropped beyond 900, so its held target is bounded by **900**, not 3600. Its authored `3600` is reachable **only** through the zone-command lane (`UpdateStateGrouped`, which has *no* leash — `:1988`) or by a target that walked outward while held. ⇒ ⛔ **The `2000` ceiling's real-world bite is far smaller than −44.4 % suggests for uncommanded play, and concentrated in commanded play.** He should hear both numbers, not just the −44.4 %.

---

## §4 — ⛔⛔ DROP vs KEEP: THE MECHANISM, AND WHAT "DROP" WOULD COST

**Jonathan has ruled DROP** (*"fog should make units drop existing targets if they are outside the 609 range"*). Reporting the shipped mechanism against that ruling, per unit class of caller.

### (a) Where a held target is dropped by DISTANCE today — the complete list

| body | distance drop? | `file:line` |
|---|---|---|
| `UpdateState` (Standard, uncommanded) | ✅ `> LeashRange` (900) | `:1713` |
| `UpdateStateStandardCommanded` | ✅ `> LeashRange` (900) | `:1943` |
| ⛔ **`UpdateStateGrouped`** | ⛔ **NONE — deliberately** | `:1988` *"There is deliberately NO LeashRange here: the zones ARE the leash for HOLD, and AMBUSH's whole point is the unbounded chase"* |
| `UpdateStateSiege` | n/a — **re-picks its structure every poll** (`:2545–2550`), holds nothing | — |
| `ATower::ScanAndFire` | n/a — **re-acquires every single shot** (`:179`), holds nothing | — |

⭐ **`ASummonedUnit::PerformAttack:3941` does NOT drop.** Out of range ⇒ it `return`s and skips the shot; the comment says so verbatim (*"drifted out of range between checks — no hit, the state check re-chases"*). **Firing and dropping are already two different mechanisms in the shipped code.**

### (b) The two cases the dispatch asked me to measure

**Case A — acquired at 1,500 uu, then fog lands.**
- **Standard / Commanded:** ⛔ **not reachable today** — 1,500 > `LeashRange` 900, so the target was already released before fog was ever in play. *After* `TASK-979` it becomes reachable (§3c), and then: the leash drops it at `:1713`, and re-acquisition at `:1721` is fog-clamped to 609.6 and **fails** ⇒ ⭐ **the drop STICKS, for free, with no new code.**
- ⛔ **Grouped (zone-commanded):** the target is **KEPT**. No distance leash, and the zone gather uses `SeeingFromUnbounded`, so the unit re-validates only on *zone* membership and *aliveness*. ⇒ ⛔ **A grouped Longbowman keeps firing at 1,500 uu with fog up, indefinitely.** This is the gap.

**Case B — held target walks out to 1,200 uu while fog is up.** Identical outcome to Case A: Standard/Commanded drop it via the leash and cannot re-take it; **Grouped keeps it.**

**⚠️ The band nobody has named: `(609.6, 900]`.** A Standard unit holding a target at, say, 700 uu is **inside** the leash (so not dropped) and **outside** the fog ceiling (so it must not fire). Today it would hold the target and `EnterAdvance` toward it — which is arguably *good* behaviour, but it is **KEEP, not DROP**, and it contradicts his ruling. ⇒ **This band is the only place where Standard units need new code.**

### (c) ⇒ WHAT IMPLEMENTING **DROP** ACTUALLY COSTS — the seam, not a suggestion

1. ✅ **Towers + Siege: FREE.** They hold nothing; the funnel clamp already blinds them.
2. ⭐ **Standard / Commanded: ONE expression at TWO existing sites** (`:1713`, `:1943`). The validity path already exists and already drops by distance — **reuse it, do not add a tick.** The leash term becomes a `min` of `LeashRange` and the effective (fog-aware) reach. ⛔ **It must call the same accessor `TASK-980` introduces**, not read a fog symbol here (`FOG-§7`).
3. ⛔⛔ **Grouped: THIS IS A DECISION, NOT AN EDIT.** There is no distance-drop term to extend, and adding one contradicts an explicit shipped design law quoted at `:1988`. ⭐ **I am not inventing a proceeding default for it.** It is a question for the manager, and it is the single most likely way a "DROP" implementation ships looking correct while leaving the loudest case — commanded ranged units — untouched. **This is exactly `SC-§37`'s shape:** a fix at sites 1–2 passes every plausible test while the grouped lane keeps shooting through fog.

### (d) ⚠️ MELEE, AS THE DISPATCH REQUIRED

Melee `AttackRange` is **120**, far inside 609.6, so **melee firing is untouched in both states**. Melee *notice* is affected (2000 → 609.6). The sharp case — **a melee unit mid-charge across 1,500 uu when fog lands**:
- It drops the target (post-`TASK-979`, per §4b).
- `AcquireTarget` returns nothing (fog-clamped).
- `Goal` falls back to **`FindNearestEnemyCastle()`** (`:1730`) ⇒ ⭐ **the unit does NOT stop — it reverts to the castle march.** Fog redirects the army; it does not freeze it.
- ⭐ **Cavalry keeps its charge.** `TrackChargeMovement` (`:4036`) only resets `ChargeMoveElapsed`/`bChargePrimed` on `Idle` or on a stall; a drop that lands in `Advance` **preserves** the primed charge (`:4049–4072`).
- ⚠️ **The exception:** if there is no standing enemy castle the body calls `EnterIdle()` (`:1736`), which **does** reset the charge. Narrow, but real.

---

## §5 — TOWERS: ACQUISITION *IS* FIRING (a distinction `TASK-980` must not import from the unit lane)

`ATower::ScanAndFire` (`Tower.cpp:155`) calls `AcquireTarget()` **every cadence tick** (`:179`) and fires at whatever comes back. `ATower::AcquireTarget` bounds candidates with `RangeSq` (`:240`) and **there is no second range check anywhere in the fire path** (`FireProjectileAt:336`, `FireChainZapAt:388` — neither reads `AttackRange`).

⇒ ⭐ **For towers there is exactly ONE gate, it is already inside the fog funnel, and DROP is automatic.** ⛔ **`TASK-980` should touch `Tower.{h,cpp}` only if it wants the `2000` ceiling on towers — and §2 shows no tower reaches 2000, so the correct answer is almost certainly to leave `Tower.cpp` alone entirely.** Adding the accessor there would be a second clamp on an already-clamped path.

---

## §6 — ⛔ THE PERFORMANCE READ: **THE 11.1× IS NOT A COST. IT IS AN AREA.**

**Measured, at source.** `FSiegeCombatStatics::GatherHostileAgents` (`SiegeCombatStatics.cpp:147`) delegates to `GatherTeamAgentsFiltered` (`:32`), whose enumeration is:

```
UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);
```

### ⇒ ⛔⛔ **THE ENUMERATION IS `O(all actors)` AND TAKES NO RADIUS ARGUMENT. IT IS NOT SPATIALLY INDEXED, AND ITS COST IS COMPLETELY INDEPENDENT OF `AggroRadius`.**

The radius is applied **afterwards**, twice: the fog cut (`RemoveAll`, `:216`) and the site's own `if (Distance > AggroRadius) continue;` (`SummonedUnit.cpp:1803`) — a compare **already performed on every candidate at 600**.

⇒ ⭐ **Raising `600 → 2000` adds ZERO scan cost.** The `2000²/600² = 11.1×` figure describes a disc's area; it does **not** describe any work this code does. **The measured enumeration-cost delta of `TASK-979` is 1.00×.**

**Call count per unit per poll** (`StateCheckInterval = 0.25 s`, `SummonedUnit.h:1090` ⇒ **4 Hz**), counting full world enumerations:

| profile / path | enumerations per poll |
|---|---|
| Standard, uncommanded, **has** target | **1** (`AcquireTarget` → `GetAllActorsWithInterface`) |
| Standard, uncommanded, **no** target | **2** (+ `FindNearestEnemyCastle`, `TActorIterator<ACastle>` `:1850`) |
| Standard, commanded / grouped | **1–3** (`AcquireEnemyNearPoint` ×1–2 at `:2061`,`:2065` + castle fallback) |
| Siege | **2** (`TActorIterator<ABuilding>` `:2675` + castle `:2432`) |
| Support (Cleric/Witch) | **1–2** (`TActorIterator<ASummonedUnit>` `:2707` / `:2757` / `:3255`) |
| `ATower` | **1 per `AttackCadence`**, not per poll |

⇒ **Bounded at 3 world scans per unit per 0.25 s, unchanged by this batch.**

⚠️ **The real cost of `600 → 2000` is a GAMEPLAY cost, not a scan cost:** far more units hold a live target, so more units path, chase and swing simultaneously. That is movement/AI/animation load, and I did **not** measure it — it needs a profiled match, not a code read. **Stated as unmeasured rather than guessed.**

---

## §7 — ⛔ `GetDistanceToTarget` IS **3-D**. (item 4 / 🧑 `J-F22`)

`ASummonedUnit::GetDistanceToTarget` (`SummonedUnit.cpp:5385`) is:
```
Target->ActorGetDistanceToCollision(From, ECC_Pawn, OutClosestPoint);   // 3-D, closest point on collision
...fallback: FVector::Dist(From, OutClosestPoint);                       // 3-D
```
**No `Size2D()`, no `.Z = 0`, no `DistXY` anywhere in the function.** The fog funnel's own cut (`SiegeCombatStatics.cpp:230`) uses the **identical** call, so the two metrics agree term for term.

### ⇒ **The shipped answer to his question is "through the air."** A unit on a ×2 Watch Tower (deck **2,400 uu**) is **2,400 uu** from a target directly below — **far outside 609.6** ⇒ ⛔ **it CANNOT shoot down through the fog.** That matches his *"in any direction"*. ⭐ **No change is needed to satisfy him; the code already does what he described.** Reported, not altered.

⚠️ **Two riders.** (1) The metric is **closest-point-on-collision**, not origin-to-origin — so a big actor is "closer" than its pivot suggests; that is deliberate and documented. (2) `UpdateStateGrouped`'s **zone** tests are explicitly **2-D discs** (`:1998–2000`, `:2370`). ⇒ **the game already mixes 2-D and 3-D deliberately**, and `J-F22` should be answered as "the *range* test is 3-D; the *zone* test is 2-D," not as one global answer.

---

## §8 — `BrightSun` HEIGHT CEILING (item 6 — read from `TASK-941`, not re-derived)

- **Stack ceiling: ×2, final and shipped.** `AClimbableTower::MaxStackHeightMultiplier = 2` (`ClimbableTower.cpp:223`); `TASK-941` narrowed ×2…×5 itself (break-even `40n ≤ CapsuleHalfHeight` ⇒ `n ≤ 2.2`). "Stacking" is a Z-scale on one tower, **not** N towers stacked.
- **Max deck / climb-top height: `2400 uu`** (`TASK-941-programmer.md:321`, *"a **2,400 uu** tower"*; per-tower deck `1200`).
- ⇒ ### **Maximum prevention window = `120 + 60 × floor(2400 / 1524)` = `120 + 60 × 1` = `180 s` (3 minutes).**
- ⭐ **`FOG-§10.2`'s `J-F14` amendment is CONFIRMED on the measured number:** the 50-ft step makes the stacked-tower combo **+1 minute**, not +3. Uncapped is safe.

⚠️ **Caveats that could move it:** `2400` is the **deck** plane and excludes the hero capsule half-height (`+96` ⇒ 2,496 at capsule centre — a derivation, not a quoted figure); the tower's *render* top is ~2,617 (railing), which is **not** standable; and `2400` is height above the **tower actor**, so a tower placed on a hill adds that hill's elevation. **None of these crosses the `1524` step boundary** — `floor()` stays `1` from 1,524 up to 3,047 — so ⭐ **the answer `180 s` is robust to every one of these caveats.**

---

## §9 — ⛔ WHAT I COULD NOT DETERMINE

1. **Whether any `.uasset` overrides `AggroRadius`/`AttackRange` inside a COMPRESSED package.** My scan is controlled and found none, but it reads raw bytes. A definitive answer needs the editor, which is out of scope (`TASK-984` holds it).
2. **The gameplay-load cost of `600 → 2000`.** I proved the *scan* cost is unchanged (§6). The downstream pathing/AI/animation load from many more simultaneously-engaged units is real and **requires a profiled match** — I did not measure it and will not guess.
3. **Whether `UpdateStateGrouped` should get a fog drop term** (§4c item 3). This contradicts a shipped design law and is a **ruling**, not a measurement.
4. **Runtime confirmation of anything here.** This is a pure source read — no compile, no PIE, no editor. Every claim is textual and every one is reproducible from the `file:line` given.

---

## §10 — WHAT QA SHOULD SCRUTINISE HARDEST

1. ⛔⛔ **§1 site 9 (`SummonedUnit.cpp:3137`) — the Witch's veil radius.** If `TASK-980` routes it, the fog card nerfs the invisibility card. `TASK-985`'s negative control.
2. ⛔ **§1 sites 7 and 8 — BOTH Cleric heal sites.** Site 8 hides in a 6-term `||` chain and is the one a census skips.
3. ⛔ **§3(c) — the `LeashRange` inversion.** Not on any board row today.
4. ⛔⛔ **§4(c) item 3 — the grouped-unit DROP gap.** The most likely silent failure of the whole batch.
5. ⛔ **§6 — the 11.1× refutation.** `TASK-979` item (6)(b) instructs the implementer to *quote* this number. **The measured answer is 1.00×, and it contradicts the board's own framing.** Please check my reading of `GatherTeamAgentsFiltered` before it is relied on.
6. **§2(b) — `CrystalTower`.** Confirm `bRanged=false` is the delivery flag, not an "is ranged" flag, before anyone gates the firing lane on it.
