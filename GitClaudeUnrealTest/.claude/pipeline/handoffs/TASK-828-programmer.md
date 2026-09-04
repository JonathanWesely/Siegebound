# TASK-828 — the acquisition funnel (`FSiegeCombatStatics::GatherHostileAgents`)

**Assignee:** gameplay-programmer · **Status:** ready-for-qa · **Law:** `WITCH-§0` / `WITCH-§1` / `WITCH-§2`
**⛔ NOT COMPILED** (`QUIET-MODULE` — three compiles are queued on the live wave). **⛔ No editor, no MCP, no Git.**

---

## 1. The signature TASK-829 and TASK-838 consume

```cpp
// SiegeCombatStatics.h — public
static void GatherHostileAgents (const UWorld* World, ETeamId ViewerTeam, TArray<AActor*>& Out);
static void GatherFriendlyAgents(const UWorld* World, ETeamId ViewerTeam, TArray<AActor*>& Out);
static bool IsHostileTeam(ETeamId ViewerTeam, ETeamId CandidateTeam);

// SiegeCombatStatics.h — private: the SOLE GetAllActorsWithInterface in the project
static void GatherTeamAgentsFiltered(const UWorld* World, ETeamId ViewerTeam, bool bWantHostile, TArray<AActor*>& Out);
```

**⚠️ One correction to the spec, made rather than passed along:** the board writes the parameter as
`ESiegeTeam ViewerTeam`. **`ESiegeTeam` does not exist.** The shipped enum is **`ETeamId`**
(`TeamId.h:14`, `Blue` / `Red`), and CONVENTIONS' team contract names it that way too. Shipped as `ETeamId`.

**Contract:** `Out` is `Reset()` first · a null `World` yields an empty `Out` · **order is the enumeration's
own order, never sorted** · it does **nothing** but enumerate → `IsValid` → `Cast<ITeamAgent>` → team term.

---

## 2. The site count I measured: **nine enumerations — but ELEVEN consumers, and that is the finding**

Nine `GetAllActorsWithInterface` call sites, exactly as `WITCH-§0` recorded. Re-measured at source, not taken
on faith. **But the number that mattered for the refactor is not nine.**

> ⭐⭐ **`SpellLibrary.cpp`'s ONE enumeration fed THREE independent team-filter loops** — `ResolveFreeze`
> (Frost Nova), `ResolveTopTargetsDamage` (Lightning) and **`ResolveAllyBuff` (Battle Cry)**. It reached them
> through a file-local `GatherTeamAgents(World)` helper that returned the **unfiltered** roster.
>
> ⛔ **`ResolveAllyBuff` wants FRIENDLIES** (`Unit->GetTeamId() != CasterTeam → continue`).
> **Routing all nine sites through `GatherHostileAgents` would have made Battle Cry buff NOBODY** — a shipped
> card silently doing nothing, with a green suite and a commit message that said "pure refactor".

⇒ **9 enumerations · 11 team-filter consumers · 10 hostile · 1 friendly.** That is why `GatherFriendlyAgents`
exists. It is not scope creep: it is the same enumerate-and-team-filter step with the term inverted, and it is
what keeps the grep gate at **one** while `WITCH-§2`'s fourth lane (friendly acquisition is **never**
veil-suppressed) stays a **named** lane rather than an unexplained second enumeration.

**I looked for a tenth enumeration and did not find one.** Cross-checked `TActorIterator` (44 sites — all
concrete-class scans: castles, zones, buildings, gold nodes; none is an `ITeamAgent` acquisition) and every
`Cast<ITeamAgent>` in the tree (the survivors are damage-attribution and trace paths, listed in §4).

| # | site | consumers | lane |
|---|---|---|---|
| 1 | `SummonedUnit.cpp` `AcquireTarget` | 1 | hostile |
| 2 | `SummonedUnit.cpp` `AcquireEnemyNearPoint` | 1 | hostile |
| 3 | `Tower.cpp` `AcquireTarget` | 1 | hostile |
| 4 | `Tower.cpp` `FireChainZapAt` | 1 | hostile |
| 5 | `SiegeCombatStatics.cpp` **`ApplyRadialDamage` — EVERY AoE** | 1 | hostile |
| 6 | `SpellLibrary.cpp` (one gather) | **3** — Freeze · Lightning · **Battle Cry** | 2 hostile + **1 friendly** |
| 7 | `SpellLineSweep.cpp` `ApplyLineEffectUpTo` | 1 | hostile |
| 8 | `HeroCharacter.cpp` `DoMeleeAttack` | 1 | hostile |
| 9 | `SiegeCheatManager.cpp` `FindNearestEnemy` | 1 | hostile |

---

## 3. How I established behaviour is unchanged at every site

**⛔ Not by assertion.** Four independent lanes, each of which can fail:

**(a) Term-for-term predicate identity.** The lifted predicate is *exactly* what the sites wrote inline:
`IsValid(Candidate)` → `Cast<ITeamAgent>` non-null → `GetTeamId() != ViewerTeam`. Every site's own version was
read and diffed against it. Two sites (`SummonedUnit` ×2) folded `IsValid` into `IsTargetAlive`, which begins
with `IsValid` — a strict superset, so the gatherer's guard is redundant there, never additive.

**(b) Order preservation — the regression a set-based test cannot see.** Every call site breaks ties by
**strict improvement** (`Distance < BestPawnDist`, `DistSq >= BestDistSq` → skip). The **first** candidate wins
a tie. The gatherer therefore fills `Out` in enumeration order and never sorts; an automation test asserts
`Sort(` appears **0** times in the funnel body and `Out.Add(Candidate)` exactly **once**. A sort here would
change which target nine sites pick **without changing a single result set**.

**(c) Anti-over-lift.** The opposite failure — the funnel swallowing a site's own filter — is asserted
directly: 13 function-body probes require `AggroRadius`, `TieBreakDistance`, `IsTargetAlive`, `DistSquared2D`,
`IsAcquirableEnemy`, `MinRangeSq`, `ChainBounceRadius`, `MeleeRange`, `MinCosAngle`, `LineHalfWidth`,
`AppliedTargets`, `ActorGetDistanceToCollision`, `Distance > Radius` to still be **inside their own sites**;
and the funnel body is asserted to contain **none** of `Radius`, `Distance`, `IsA<`, `IsDead`, `IsUnitDead`.

**(d) Mutation-tested.** I ran **8 deliberate mutations** in memory (never on the tree) and confirmed the
matching assertion goes **red** for each: a tenth enumeration · the team filter deleted · Battle Cry re-routed
to the hostile lane · a `Sort()` in the funnel · the melee range gate removed · the blast gathering friendlies ·
a per-site veil check added · the comment-skip broken. **8/8 discriminate.**

### Ordering changes that are provably inert (say so, don't hide them)

- `Candidate == this` / `Target == this` now runs **after** the team filter instead of before. Self is always
  on its own team, so the gatherer already dropped it. **Kept anyway** (one pointer compare, and it is the kind
  of guard that becomes load-bearing again if "hostile" is ever redefined) — commented as such at both sites.
- `AppliedTargets.Contains()` (line sweep) now runs on the pre-filtered set. `AppliedTargets` only ever
  received enemies (friendlies `continue`d before the `Add`), so membership is unchanged.
- `ATower::IsAcquirableEnemy` still re-applies the team term. **Deliberate, not a leftover:** the chain-bounce
  search calls the same gate on candidates it did not gather. Idempotent; result identical.
- **Null-`World` logging:** previously a null world reached `GetAllActorsWithInterface`, which emits an engine
  `LogAndReturnNull` line and returns empty. The funnel now returns empty **before** that call, so one
  spurious warning is no longer emitted. No caller reaches it with a null world (all six guard `World` first).
  Flagged as the only observable difference in the whole pass, and it is a strict improvement.

### ⚠️ `ApplyRadialDamage` — friendly fire re-verified explicitly, as asked

**Friendly fire remains impossible, and the authority did not move.** It was the **team filter on the candidate
set** before and it is the **team filter on the candidate set** now — deliberately *not* the receiver's
instigator chain, because a tower-fired projectile has no resolvable instigator and would otherwise blast its
own side. `AGoldNode` still opts out by not implementing `ITeamAgent`. The closest-point measure, the radius
gate, and the routing through the target's own `TakeDamage` (so Siege-typed = 200% vs castle/buildings) are all
byte-identical. A dedicated test asserts the blast gathers **hostiles once**, **friendlies zero times**, and
calls `ApplyDamage` exactly once.

---

## 4. The grep gate, as shipped

`Siegebound.Acquisition.GetAllActorsWithInterfaceAppearsExactlyOnceInSource` — five assertions:

| lane | scope | measured |
|---|---|---|
| `UGameplayStatics::GetAllActorsWithInterface(` | **ALL** of `Source/` (265 files, tests included) | **1** ✅ |
| bare `GetAllActorsWithInterface(` | shipping source (excl. `Tests/`) | **1** ✅ |
| bare token, `Tests/` lane | the **named** exemption | **1** — `SiegeGhostPawnTest.cpp:116`, a `TEXT()` failure-message literal, **not a call** ✅ |
| `UTeamAgent::StaticClass()` | shipping source | **1** ✅ (second, independent needle — a call can dodge the function name, not the interface argument) |
| comment-skip **control** | `SiegeGhostPawn.h` | **0** on code lines, while `Contains()` proves the prose is there ✅ |

- Uses the project's **`CountOccurrencesInCode`** (comment-skipping), copied verbatim from
  `SiegeClimbableTowerTest.cpp:490`. This codebase deliberately names refused shapes in its comments —
  `SiegeCombatStatics.h` alone names the forbidden token **four times** in the paragraphs explaining why it may
  appear once — so a naive scan would force the source to stop explaining itself.
- ⭐ **The gate's own needles are composed at runtime** (`"GetAllActors" + "WithInterface("`). Without that it
  would count *itself*, so it would have to exempt its own file — and an instrument blind to the file it lives
  in is exactly where a tenth enumeration gets written. A self-check asserts the composed spelling is correct,
  so a typo'd needle cannot make the gate pass by finding nothing.

**Surviving `Cast<ITeamAgent>` (all verified non-acquisition):** `SummonedUnit.cpp` ×3, `HeroCharacter.cpp` ×2,
`Building.cpp` ×3, `Castle.cpp` ×3 (`TakeDamage` attribution) · `Projectile.cpp`, `ClimbableTower.cpp` ×3,
`CombatantHealthBarComponent.cpp` (target-locked / climber / bar-colour) · `SiegeCheatManager.cpp` ×2 (pawn team
read, trace hit test) · `Tower.cpp` ×1 (`IsAcquirableEnemy`, the §3.7 class gate — site-local, deliberately kept).

---

## 5. Files touched

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h` | +3 public statics, +1 private helper, the `WITCH-§0/§1/§2` rationale, the TASK-829/838 hook notes |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp` | the funnel; `ApplyRadialDamage` routed (site 5) |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | sites 1–2 routed; two stale comments corrected |
| `Source/GitClaudeUnrealTest/Siegebound/Tower.cpp` | sites 3–4 routed; +1 include |
| `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` | site 8 routed; +1 include |
| `Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.cpp` | site 6: local `GatherTeamAgents` **deleted**; 2 hostile + 1 **friendly** routed |
| `Source/GitClaudeUnrealTest/Siegebound/SpellLineSweep.cpp` | site 7 routed; +1 include |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.cpp` | site 9 routed; +1 include |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAcquisitionFunnelTest.cpp` | **NEW** — 9 tests |

**⛔ `SummonedUnit.h`, `Tower.h`, `HeroCharacter.h` were NOT modified** — the lift needed no header change at
any call site. They are in the board's `names:` line; nothing was owed to them.

**Fence check:** `SiegePlayerController.{h,cpp}`, `CardHandWidget.*`, `WBP_CardHand`, `Building.*`,
`ClimbableTower.*`, `SiegeControlsHelpWidget.*`, `IMC_Hero` — **untouched**. Verified against `git status`:
those files are dirty from the **live CARDBAR/STACK wave**, not from me. File-disjointness held exactly as boarded.

### ⚠️ One fence note for the orchestrator, raised not assumed

`SiegeCheatManager.cpp` is named in the board's `names:` line and in spec item (2)'s nine sites, but it is
**not** in the six-file ownership list at TASKBOARD `:13711`. I routed it, because the binding acceptance gate
is *"appears in `Source/` **exactly once**"* and leaving it would have produced a second hit. `WITCH-§1` permits
an exemption "named in a comment"; **I did not take it** — a cheat that saw a different candidate set from the
game is an instrument that lies, and `SummonTestUnit`/`ApplyTestDamage` exist precisely to verify shipping
behaviour headlessly. The reason is written into the source at the call site. No live task owns that file.

---

## 6. Test list (9 new) — **suite delta: `+9`**

⚠️ **Reported as a count, never an absolute — and the census moved underneath me.** TASK-827 and TASK-837 landed
`SiegeInvisibilityTest.cpp` and `SiegeFogTest.cpp` in parallel while I worked (both new-files-only, zero
collision). My contribution is **exactly +9**; the absolute total is whatever the wave adds up to at gate time.

| # | test | guards |
|---|---|---|
| 1 | `GetAllActorsWithInterfaceAppearsExactlyOnceInSource` | ⭐⭐ the binding grep gate (5 assertions + comment-skip control + needle self-check) |
| 2 | `IsHostileTeamIsExhaustiveAndDiscriminating` | the 2×2 truth table + the hostile/friendly partition — **executed, not scanned** |
| 3 | `GatherResetsOutAndIsNullWorldSafe` | `Out.Reset()` + null-world safety — **a real call into the funnel** |
| 4 | `AllNineEnumerationSitesRouteThroughTheFunnel` | per-file call counts; totals pinned at **10 hostile / 1 friendly** |
| 5 | `NoSiteRetainsItsOwnAcquisitionTeamFilter` | the old inline shape is gone, each zero paired with a per-file positive control |
| 6 | `EverySitesOwnFilteringSurvivedTheLift` | 13 anti-over-lift body probes + the **no-sort / order** guarantee |
| 7 | `ApplyRadialDamageStillCannotFriendlyFire` | ⭐⭐ the AoE lane, explicitly, as spec item (5) required |
| 8 | `BattleCryUsesTheFriendlyLaneNotTheHostileOne` | ⭐⭐ the one real regression this refactor could have shipped |
| 9 | `VeilAndFogSuppressionLiveOnlyInTheFunnel` | ⛔ **permanent law** — no per-site veil/fog check, ever |

**Every assertion can fail** (8/8 mutations discriminate, §3d). Every negative claim carries a positive control
proving the instrument still sees real code in that same file. **No existing test was modified.**

> ⭐ Test 9's tokens are the **real shipped symbol names**, read off TASK-827's and TASK-837's landed headers —
> `FSiegeInvisibilityStatics` / `IsVisibleTo(` / `FSiegeFogStatics` / `EffectiveVisionRadius` / `bIsInvisible`.
> My first draft guessed `IsVisibleToViewer` and `FogVisionCeilingUU`; **neither exists**, so that guard could
> never have fired. Corrected against the source. It is written to survive TASK-829 and TASK-838 **unchanged**
> (it asserts about the five *call-site* files, deliberately excluding `SiegeCombatStatics.cpp`, which is where
> suppression is *supposed* to land) — ⛔ so it must not be relaxed by either.

---

## 7. What QA should scrutinise

1. **⛔ THE ONE THING I MOST WANT A SECOND PAIR OF EYES ON: is `GatherFriendlyAgents` in scope?** It is a second
   public entry point the spec did not ask for. My case: without it, either Battle Cry silently breaks, or a
   second `GetAllActorsWithInterface` survives and the binding gate fails. I judged a *named* friendly lane
   better than either. **If QA disagrees, the alternative is a `bool bHostile` on one function — say so and I
   will change it**, but note the two lanes have different futures (829 changes one and must not be able to
   change the other by editing a shared branch), which is why I split them.
2. **The redundant-but-kept `Candidate == this` / `Target == this` guards** (`SummonedUnit` ×2, `HeroCharacter`).
   Provably dead. Kept and commented. If QA rules dead code out, they are three deletions.
3. **`Tower::IsAcquirableEnemy` still re-checks the team.** Idempotent; kept because the chain-bounce path
   shares the gate. Confirm that reading.
4. **`ApplyRadialDamage`.** Every AoE routes through it. Re-read the friendly-fire argument in §3.
5. **`ETeamId` vs the spec's `ESiegeTeam`** — §1. Confirm the correction is right rather than a liberty taken.
6. **Not compiled.** `QUIET-MODULE`. Balanced braces/parens verified with a real tokenizer on all 9 files;
   every `TestEqual`/`TestTrue`/`TestFalse` overload checked against `UE_5.8/.../AutomationTest.h`. That is
   static verification, ⛔ **not a compile**, and should not be read as one.

## 8. ⛔ Explicitly NOT in this commit

**Zero invisibility. Zero fog. Zero behaviour change.** Nothing consults the veil, nothing clamps a range.
`ApplyRadialDamage` carries **no** exemption hook — `WITCH-§2` / `J-W2` rules it exempt, and TASK-829 owns
implementing that explicitly. A landed funnel is **not** a landed feature.
