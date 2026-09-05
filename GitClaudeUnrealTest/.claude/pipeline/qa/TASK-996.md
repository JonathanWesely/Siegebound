# QA Report — TASK-996
**Gate over:** `TASK-978` ALONE (the `AttackRange`/`AggroRadius` census) · **Input:** `handoffs/TASK-978-programmer.md`
**Date:** 2026-09-04 · **Agent:** qa-reviewer · **Writes:** this file + the board status line + one Slack post. ⛔ Zero source edits, zero engine, zero Git.

## Verdict: **PASS** — 0 BLOCKER · 4 WARN · 5 NIT

⛔ **`TL-§5c` — NOTHING WAS COMPILED OR EXECUTED FOR THIS ROW.** `TASK-978` changed zero source, and I ran no build and no suite. Every statement below is a **source read**, reproducible from the `file:line` given. **No executed number appears in this report.** The suite total is **not stated** — not remembered, not derived, not implied.

---

## §0 — INSTRUMENTS (`SC-§39`), AND THE ONE THAT LIED TO ME

| instrument | control | result |
|---|---|---|
| `Grep` over `Source/` | **positive:** `class GITCLAUDEUNREALTEST_API ASummonedUnit` → 1 hit, `SummonedUnit.h:188` | ✅ live |
| same | **negative:** `ZzQqAttackRangeNotARealSymbol` → **0** | ✅ not hallucinating |
| same | **accessor trap:** `GetAttackRange\|SetAttackRange` → **0** | ✅ no accessor exists ⇒ a name census IS complete for direct reads |
| `Read` | used for every character-exact adjudication | ✅ authoritative |

⚠️⚠️ **AN INSTRUMENT DEFECT I HIT, DECLARED BECAUSE IT NEARLY PRODUCED A FALSE BLOCKER.** `Grep`'s `content` output **misrendered the C++ comment marker `//` as `\`** on `SiegeFogStatics.h:247` and `:304`, e.g. it printed `float FogVisionCeilingUU = 609.6f; \ FOG-§1: …`. A stray `\` there would be a **compile-breaking token**, and I was one step from filing it as a BLOCKER against a file `TASK-981` is editing. `Read` on the same two lines shows a correct `// FOG-§1: …`, and a negative grep for `; \ ` returns **0**. ⇒ ⭐ **`Read` is authoritative for character-exact claims; `Grep` content output is NOT.** Recorded as a standing instrument caveat, not as a finding against anyone.

---

## §1 — THE SIX SPEC ITEMS, ADJUDICATED AT SOURCE

### (1) ⭐⭐ THE `11.1×` REFUTATION — **`TASK-978` IS RIGHT. THE THREE AMENDED ROWS ARE RIGHT.**

**CONFIRMED, read myself, not relayed.** `SiegeCombatStatics.cpp:32`:

```cpp
void FSiegeCombatStatics::GatherTeamAgentsFiltered(const UWorld* World, ETeamId ViewerTeam, bool bWantHostile, TArray<AActor*>& Out)
…
:48   UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);
```

- The **signature takes no radius and no position.** A radius is not merely unused — it is **unrepresentable** at this seam.
- `GatherHostileAgents` (`:147`) calls it **unconditionally at `:150`**, before any radius is consulted.
- The radius is applied **afterwards** at `:207`/`:216`.

⇒ ⭐ **The enumeration is `O(all actors)` and radius-independent. `2000² / 600² = 11.1×` is a DISC AREA and describes no work this code performs. The enumeration-cost delta of `TASK-979` is `1.00×`.** ✅ **`FOG-§9.6`, `TASK-979` item (6)(b) and `TASK-985` item (7) were amended CORRECTLY. Three rows stand.**

### (2) ⭐⭐ THE `LeashRange` INVERSION — **CONFIRMED, ORDER VERIFIED RATHER THAN INFERRED**

Declarations (`Read`, exact lines):
- `SummonedUnit.h:1039` — `float AggroRadius = 600.f;` ✅
- `SummonedUnit.h:1043` — `float LeashRange = 900.f;` ✅

Read sites: **exactly two**, `:1713` and `:1943` ✅ (complete — my own `AggroRadius`/leash sweep found no third).

**THE ORDER, WHICH IS THE CLAIM, VERBATIM FROM `UpdateState`:**
```cpp
:1713   if (CurrentTarget && (!IsTargetAlive(CurrentTarget) || GetDistanceToTarget(MyLocation, CurrentTarget) > LeashRange))
:1715   {   CurrentTarget = nullptr;   }
        // :1717–:1720 are COMMENTS ONLY — no return, no branch, no early-out
:1721   if (AActor* Acquired = AcquireTarget())
:1723   {   CurrentTarget = Acquired;   }
```
⇒ ⛔ **DROP-THEN-REACQUIRE IN ONE UNINTERRUPTED PASS. CONFIRMED.** I checked `:1717–:1720` specifically for an intervening `return`/branch that would have refuted the claim — there is none, they are comment lines.

`AcquireTarget()` reaches with `AggroRadius` at **both** `:1779` (`SeeingFrom(MyLocation, AggroRadius)`) and `:1803` (`if (Distance > AggroRadius) continue;`) ⇒ raising it to `2000` widens **re-acquisition** to 2000. With fog off, a target at 1,500 uu is released at `:1713` and re-taken at `:1721` in the same 0.25 s poll. ⭐ **The leash stops releasing anything the unit can still see. `TASK-978` §3(c) is CONFIRMED and `TASK-979` item (6a) is correctly boarded.**

The identical shape recurs in `UpdateStateStandardCommanded`: drop `:1943` → re-acquire `:1948`. ✅

### (3) THE GROUPED-LANE GAP — **CONFIRMED**

`UpdateStateGrouped` (`:1979`). The comment is at **`:1988–1989`** (spans two lines; `TASK-978` cited `:1988`), verbatim:
> *"There is deliberately NO LeashRange here: the zones ARE the leash for HOLD, and AMBUSH's whole point is the unbounded chase."*

**Complete drop census in that body:** `!IsTargetAlive` (`:1993`) · zone membership, **`Hold` only** (`:2000–2016`) · the climb/never-attack seal (`:2053–2056`). ⇒ ⛔ **No distance-from-self drop path exists. CONFIRMED.**

### (4) THE VEIL RADIUS + BOTH CLERIC SITES — **CONFIRMED, AND SITE 9 IS STRONGER THAN `TASK-978` STATED**

- `:2729` — `if (Distance > AttackRange) continue;` in `FindNearestDamagedFriendly` ✅
- `:2825` — the `||` tail of the `PerformHeal` re-validate chain ✅
- `:3137` — `OutRadius = (AttackRange > 0.f) ? AttackRange : WitchVeilRadiusFallbackUU;` — ✅ **two reads on one line**, exactly as claimed. Callers **`:3098` and `:3378`** ✅ (my own grep: exactly two call sites).

⭐⭐ **CORROBORATION `TASK-978` DID NOT CITE, AND IT MAKES `TASK-985`'s NEGATIVE CONTROL STRONGER.** Two independent facts:
1. `Docs/Data/cards.csv` line 33, the Witch's own `Notes` cell, states it in the **data**: *"Range 400 is the ungrouped veil radius **not an attack range** (WITCH-4)."*
2. `ResolveWitchPositionCircle` returns the `AttackRange`-derived radius on **every early-out path**, and `:3155–3163` records that **`Follow` is the SPAWN DEFAULT for every follow-eligible Blue unit, and the witch is follow-eligible.** ⇒ a freshly played witch takes the `AttackRange` fallback.

⇒ ⛔ **The `AttackRange` read at `:3137` is the COMMON case for a fresh witch, not a rare edge.** Routing site 9 through a fog-clamped accessor is a live Witch nerf. **`TASK-985` item (2) is correctly built.**

### (5) `bRanged` / `CrystalTower` + THE 3-D METRIC — **BOTH CONFIRMED**

**`CrystalTower`** (`cards.csv:29`): `CardType=Building`, `Range=800`, `bRanged=FALSE`, `ChainTargets=3` ✅ exactly as stated. And `SummonedUnit.cpp:1290` binds `bRangedAttack = Row->bRanged;` ⇒ ⛔ **a `if (bRangedAttack)` firing gate WOULD let `CrystalTower` escape. The hole is real.** The tower lane is gated by the funnel today, so it is safe now — the warning is correctly aimed at future code.

**`GetDistanceToTarget` (`:5385`)** — `ActorGetDistanceToCollision(From, ECC_Pawn, OutClosestPoint)`, fallback `FVector::Dist(From, OutClosestPoint)`. **No `Size2D`, no `.Z = 0`, no `DistXY`.** ✅ **3-D CONFIRMED**, and identical to the funnel's own cut at `SiegeCombatStatics.cpp:230`. `FOG-§9.8c`/`§9.8d` stand.

### (6) THE DECLARED RESIDUAL — **RE-CONFIRMED AT MY OWN INSTANT, NOT INHERITED**

`SiegeFogStatics.cpp:86` `EffectiveVisionRadius` is **still exactly the shape §3(b) describes**, read now, after `TASK-981`'s edits:
- `:92–95` fog-off ⇒ returns the request untouched;
- `:114` totality guard (`!IsFinite(Ceiling) || Ceiling <= 0.f || !IsFinite(Request)`) ⇒ returns the request;
- **`:122` `return FMath::Min(RequestedRadiusUU, Ceiling);`** — ⭐ **still a `min`, not a clamp.**

⇒ `min(600, 609.6) == 600` ⇒ ⛔ **the shipped acquisition clamp IS inert on units by arithmetic. CONFIRMED.** `TASK-981` moved nothing this census depends on.

---

## §2 — MY OWN COUNTS (`SC-§39` — derived, not relayed)

| quantity | `TASK-978` declared | **my independent count** | |
|---|---|---|---|
| `AttackRange` occurrences, module-wide | 33 | **33** (9 files) | ✅ |
| in `Tests/` | 5 | **5** (Invisibility 1 + FogClamp 3 + ClimbableTower 1) | ✅ |
| writes | 2 | **2** (`SummonedUnit.cpp:1288`, `Tower.cpp:61`) | ✅ |
| member declarations | 2 | **2** (`SummonedUnit.h:2301`, `Tower.h`) | ✅ |
| comments | 9 | **10** | ⚠️ NIT-1 |
| **executable read LINES** | **14** | **14** | ✅ |

⭐⭐ **THE TABLE IS COMPLETE — THE FINDING THAT MATTERS MOST.** I enumerated the read lines independently and got **exactly** `TASK-978`'s fourteen, at the same line numbers:
`SummonedUnit.cpp` **1740 · 1900 · 1966 · 2071 · 2559 · 2729 · 2825 · 3137 · 3726 · 3941** + `Tower.cpp` **100 · 104 · 235 · 240**.
⇒ ⛔ **ZERO missed consumers.** Spot-verified at source: `:3726` `MoveToActor(Goal, FMath::Max(AttackRange * 0.8f, 40.f), true)` ✅ · `:3941` returns rather than drops, comment verbatim ✅ · `:2559` gates the `bSuicide` `Detonate()` ✅ · `Tower.cpp`'s 6 occurrences leave **no `AttackRange` read in the fire path**, so §5's "acquisition IS firing for towers" is confirmed **by a complete census**, i.e. the absence is measured (`SC-§40`).

Also confirmed: `LoadStatsAndStart` (`:1278–1305`) binds HP/Damage/**AttackRange**/Cadence/**bRangedAttack**/Profile/keywords/Speed and ⛔ **never `AggroRadius`** ✅ · `BindStats` → **0 hits**, the name is informal ✅ · assignments to `AggroRadius` are **exactly 2**, both `0.f` (`MinerUnit.cpp:72`, `SorcererUnit.cpp:23`) ✅ · `cards.csv` header column 0 **is empty** ✅ · the `bRanged=true` population is **exactly 6** rows and **exactly 3** exceed 2000 (Longbowman 3600, Archer 2100, Wizard 2100), **no tower touched** ✅.

---

## Findings

- **[WARN-1]** `handoffs/TASK-978-programmer.md:205` (§6) — **"Raising `600 → 2000` adds ZERO scan cost" is TOO STRONG, and the headline `1.00×` is nonetheless CORRECT.** §6 groups the fog `RemoveAll` cut with the site's own `:1803` compare as work *"already performed on every candidate at 600"*. That is true of `:1803` and **false of the fog cut**. `SiegeCombatStatics.cpp:207` runs the cut **only** `if (EffectiveRadiusUU < Vision->RequestedRadiusUU)`. Today, fog up: `min(600, 609.6) = 600`, and `600 < 600` is **false** ⇒ ⭐ **the loop is SKIPPED ENTIRELY and costs zero collision queries.** After `TASK-979`: `min(2000, 609.6) = 609.6 < 2000` is **true** ⇒ the loop **RUNS**, executing `ActorGetDistanceToCollision` (`:230`) **per candidate, per unit, at 4 Hz, whenever fog is up** — work that has **never** run on this path. ⇒ **Enumeration delta `1.00×` STANDS; total gather cost under fog does NOT.** ⛔ The code's own comment at `:198–206` names the very property being removed (*"a ceiling above this site's own reach … skips the loop entirely, costs zero distance queries"*). *Suggested fix: not `TASK-978`'s to fix — route to `TASK-979`'s honesty clause (6)(c) and to `TASK-987`'s regression watch.*
- **[WARN-2]** `TASKBOARD.md:17991` — ⛔⛔ **THE `11.1×` SURVIVES AS A LIVE INSTRUCTION TO WRITE IT INTO SHIPPED SOURCE.** `TASK-979` item **(2)** still orders, under `HIGH-§1`: *"The comment must say, in substance: … **the search area is 11.1× the old one** …"* — i.e. into a permanent comment beside `AggroRadius`. The amendment landed on item **(6)(b)** (`:17996`, correctly struck and refuted) and **missed item (2) in the same row**. ⇒ **(6)(b) only misleads a handoff; (2) puts the refuted figure in the engine.** *Suggested fix: manager amends `TASK-979` item (2) **before** `979` is dispatched.*
- **[WARN-3]** `TASKBOARD.md:18468` — `TASK-988` / `J-F21`, the sheet **Jonathan reads**, still says the *"ONE thing still owed on this row … is the `11.1×` search-area cost (`2000²/600²`), MEASURED on `TASK-978` item (5)"*. It is **no longer owed** — it was measured and **refuted**. As written it tells Jonathan an `11.1×` cost is an open concern on a ruling he already closed. *Suggested fix: manager replaces with the `1.00×` result.*
- **[WARN-4]** ⭐⭐ **`SC-§60`, APPLIED — A SHIPPED GREEN TEST WHOSE ANTI-FAKE ARGUMENT `TASK-979` INVALIDATES, AND WHICH NO ROW NAMES.** `Tests/SiegeFogClampTest.cpp` (`Siegebound.Fog.EveryVisionSiteIsBitIdenticalWithFogOff`):
  - `:1021` hardcodes `{ TEXT("ASummonedUnit::AcquireTarget — AggroRadius …"), 600.f }`;
  - `:1063–1068` asserts `EffectiveVisionRadius(600.f, /*bFogActive=*/ true, Tuning) == 600.f` under the stated rationale *"…the unit AggroRadius of 600, because it already sits INSIDE the 609.6 ceiling … **this site is essentially untouched by fog**."*

  `TASK-979` makes that description **false for the shipped game** — `J-F21` exists precisely to make fog bite on unit notice (`609.6`). ⛔⛔ **But the test STAYS GREEN, because it passes the literal `600.f` rather than `ASummonedUnit`'s actual `AggroRadius`.** ⇒ **a permanently-green test asserting an obsolete design property as correct behaviour — the exact tower-stacking shape `SC-§60` was written from.** Worse: `TASK-979` item (5)(c) adds a test asserting the effective notice radius **is** `609.6`; land it without retiring this one and the suite holds **two green tests giving contradictory accounts of the same site**, both passing because both assert literals. ⇒ **the argument must be RE-DERIVED, not the number re-signed.** ⚠️ **Root cause: `TASK-978` subtracted the 5 `Tests/` occurrences and never examined them** — for a census whose purpose is to tell `979`/`980` what their change touches, the test surface is exactly where `SC-§60` failures live. *Suggested fix: `TASK-979` must name `SiegeFogClampTest.cpp:1021` and `:1063–1068` explicitly and re-derive both; `TASK-985` gates that it did.*
- **[NIT-1]** `handoffs/TASK-978-programmer.md:32` — bucket arithmetic has a **compensating off-by-one**: there are **10** comments (not 9), and the `UE_LOG` argument (`Tower.cpp:104`) is **subtracted as a non-read AND listed as row 14** of the 14-read table. The two errors cancel, so the headline **14** and the table are both correct. Cosmetic only.
- **[NIT-2]** `handoffs/TASK-978-programmer.md:57` — the finding is **real** but its own address is **stale**: it cites `SiegeFogStatics.h:355`, which now holds the Beer-Lambert density table. The stale gate citation (*"`:1851, :1956, :2409`"*) actually lives at **`SiegeFogStatics.h:454–455`** — moved by `TASK-981` mid-census, i.e. §0's declared concurrency hazard **materialising in its own report**. ⭐ The substance is confirmed: none of `:1644 :1785 :1851 :1956 :2409` is an `AttackRange` site today; the real ones are `:1740 :1900 :1966 :2071 :2559`.
- **[NIT-3]** `handoffs/TASK-978-programmer.md:43` — `:2825` is a **7-term** validity chain (6 `||`), not "6-term". The point (it hides at the tail) is right.
- **[NIT-4]** `TASKBOARD.md:18000` — `TASK-979` item (6a) says the poll *"RE-ACQUIRES **THE SAME TARGET** at `:1721`"*. `AcquireTarget()` returns the **nearest** hostile, which need not be the same actor. `TASK-978`'s own wording (*"dropped and immediately re-taken"*) is the accurate one. ⚠️ Flagged so no one writes a test asserting *same target* — it would pass or fail for the wrong reason (`SC-§60`).
- **[NIT-5]** `handoffs/TASK-978-programmer.md:152` — the §4(a) row for `UpdateStateGrouped` reads *"NONE — deliberately"*. Precisely, **`Hold` DOES drop** on 2-D zone membership (`:2000–2016`); only **`Ambush`** is drop-free but for death. §4(b) states this correctly in prose, so the table is a simplification, not an error — but `TASK-980` should not read "grouped" as one lane.

---

## Notes for build-master (`TASK-987`)

1. ⛔ **This verdict does NOT gate your commit** (`TASK-996` is read-only, `parallel-safe: yes`). Your row at `TASKBOARD.md:18441` asks only that a verdict **exists** before you compile. **It exists: PASS.**
2. ⭐⭐ **NO FINDING WAS REFUTED. The three rows amended on `TASK-978` (`979`/`980`/`985`) STAND** — I re-read `SiegeCombatStatics.cpp:32` myself and the `1.00×` amendment is correct. **Nothing in this batch needs unwinding on my account.**
3. ⚠️ **WARN-2 is upstream of your compile:** if `TASK-979` is implemented against item (2) **as currently written**, the refuted `11.1×` lands in a shipped source comment. Confirm the manager amended `TASKBOARD.md:17991` before `979`'s diff reaches you.
4. ⚠️ **WARN-1 sharpens your item (3) regression watch:** `AggroRadius 600 → 2000` newly **activates** the funnel's `RemoveAll` collision-query loop for unit acquisition whenever fog is up (`SiegeCombatStatics.cpp:207`). Any fog-plus-many-units slowdown is **this**, and it is a finding, never "flaky".
5. ⚠️ **WARN-4 is the one that can ship green and wrong:** watch that `Tests/SiegeFogClampTest.cpp:1063–1068` was **re-derived**, not merely renumbered. A green suite is **not** evidence here — that test passes literals and cannot fail against the change it describes.
6. ⛔ **`TL-§5c` — I state no suite total.** Nothing was executed for this row. Derive yours at your own instant.

---

## What I could not determine (`SC-§40` — an absence is a measurement)

1. **Runtime behaviour of anything above.** Pure source read: no compile, no PIE, no editor, no MCP. Every claim is textual and reproducible from its `file:line`.
2. **Whether a COMPRESSED `.uasset` overrides `AggroRadius`/`AttackRange`.** `TASK-978` §0 declared this residual as *"high, not absolute"*; **I did not re-run the binary scan and I do not inherit it as proven.** It remains open and needs the editor. ⚠️ Note it cuts **toward** risk, not away: an override would be a further consumer.
3. **The gameplay (pathing/AI/animation) load of `600 → 2000`.** `TASK-978` correctly declares it unmeasured; WARN-1 adds a *second* unmeasured cost (the newly-live collision-query loop). **Neither is guessed here.** Both need a profiled match.
