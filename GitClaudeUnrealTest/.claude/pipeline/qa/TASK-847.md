# QA Report — TASK-847 (WF-GATE-A: the statics gate)

**Gate task:** TASK-847 · **Reviewer:** qa-reviewer · **Date:** 2026-09-03
**File named for the GATE task** (`SC-§29`; `qa/TASK-506.md` / `qa/TASK-525.md` / `qa/TASK-537.md` precedent) — ⛔ never for a subject task.

## ⭐ `SC-§29` COVERAGE LEDGER — this gate covers the tasks it NAMES

| task | subject | verdict | evidence |
|---|---|---|---|
| **TASK-827** | `SiegeInvisibilityStatics.{h,cpp}` + `Tests/SiegeInvisibilityTest.cpp` | ✅ **PASS** — 0 blockers | this file |
| **TASK-837** | `SiegeFogStatics.{h,cpp}` + `Tests/SiegeFogTest.cpp` | ✅ **PASS** — 0 blockers | this file |

⛔ **NOT covered by this gate:** `TASK-828` (→ `TASK-848`) · any wiring/call-site question (→ `TASK-849`/`TASK-850`) · the compile (`QUIET-MODULE` — a compile finding is a build-time event) · art · Lane A.

**Verdict: PASS (both subjects) — 0 BLOCKERS · 3 WARN · 3 NIT**

---

## 0. THE ZERO-BEHAVIOUR-CHANGE CLAIM — VERIFIED, NOT ASSUMED

Both handoffs claim "3 new files, zero existing files edited". Verified at source (I hold no Git access by design; this is a symbol-level verification, and a diff remains build-master's):

- `grep -n "SiegeFogStatics|SiegeInvisibilityStatics"` over `Source/` returns hits in **exactly 6 files**: the four new subject files, their two test files — plus **two files that belong to TASK-828, not to either subject**: `SiegeCombatStatics.h:67-71` (forward-pointer comments naming both seams for TASK-829/838) and `Tests/SiegeAcquisitionFunnelTest.cpp:1020-1064` (TASK-828's *absence* guard, which asserts `bIsInvisible` / `FSiegeInvisibilityStatics` / `IsVisibleTo(` / `FSiegeFogStatics` / `EffectiveVisionRadius` occur **zero** times in all five acquisition call-site files).
- ⇒ **not one shipped code path calls either module.** The only references are comments and an absence assertion. ✅ **A landed file is not a landed feature, and that is the correct state.**
- ⭐ Incidental corroboration: TASK-828's guard cites `SiegeInvisibilityStatics.h:338` and `SiegeFogStatics.h:299` — **both exact at source** (`IsVisibleTo` decl and `EffectiveVisionRadius` decl respectively).

**Suite deltas (`TL-§5b` — the declared DELTA, never an absolute):** counted by reading the files, not by a bare `^IMPLEMENT_` grep:
`SiegeInvisibilityTest.cpp` = **8** tests ✅ (`+8` as declared) · `SiegeFogTest.cpp` = **9** tests ✅ (`+9` as declared). Both match. ⛔ No absolute is asserted here; the tree read `376` macro occurrences across 29 files while I was reviewing and that number is stale already.

---

## 1. TASK-827 — THE CLAIMS, VERIFIED RATHER THAN ACCEPTED

| # | claim | verdict | measured |
|---|---|---|---|
| 1 | **"Permanently" is enforced by SHAPE** — no function takes a time parameter | ✅ **TRUE** | The header declares exactly six members (`h:338`, `:348`, `:373`, `:381`, `:389`, `:392`). **Not one takes a `float`, a `DeltaSeconds`, a `Duration`, a handle or any clock.** No `RestoreVeil` / `RefreshVeil` / `TickVeil` exists in either file. The struct holds **no state**, so nothing could hold a deadline. ⇒ **a cooldown is genuinely unrepresentable**, not merely unwritten. |
| 2 | `Reason` is a label, not a condition | ✅ **TRUE** | `cpp:60` — `(void)Reason;`, and the transition at `cpp:68-78` is reached identically for all six. Test 4 breaks with **six different reasons** and pins that none re-fires ⇒ a future `switch (Reason)` in the transition goes red. |
| 3 | `ApplyBreak` returns **true exactly once**, on the true→false edge | ✅ **TRUE** | `cpp:68-78`: `if (!bIsInvisible) return false;` → `bIsInvisible = false;` → `return true;`. **Monotone + idempotent.** ⇒ the "was visible" cache `WITCH-§6` bans is unnecessary, and **no cache crept in** — both `.h` and `.cpp` are free of statics, globals and mutable file scope. |
| 4 | `IsVisibleTo` checks same-team **first and unconditionally** | ✅ **TRUE** | `cpp:26-29` is the first statement in the body; the veil is not consulted until `cpp:34`. The ordering is structural, not stylistic — pinned by test 2 with its cross-team discriminator so a `return true` cannot satisfy it. |
| 5 | Break-set **closed at six**; `ToString` has **no `default:`** | ✅ **TRUE** | `cpp:93-101` — six case arms, **no `default:` label**, fall-through to `UnrecognisedReasonToken()` at `cpp:103`. Test 7 (`Tests/…:434-438`) casts `VeilBreakReasonCount` (6) and asserts index-6 is still the unrecognised token. |
| 6 | `Walk` / `TakeDamage` / `Order` absent **and asserted absent** | ✅ **TRUE** | Fixture `ForbiddenReasonNames` (`Tests/…:81-86`); asserted per-name against all six shipped names at `Tests/…:442-451`. |
| 7 | The **control-veil** discipline is really present in tests 4, 5, 8 | ✅ **TRUE** | 4: control veiled at `:272`, checked at `:305` after **seven** breaks (1 + 6) — the count in the message is right. 5: control at `:324-330`, checked at `:359`. 8: control at `:475`, checked at `:506`. Preconditions are `AddError` + `return false` in 4 and 8, and per-iteration `TestTrue` + `continue` in 5 (`:343-347`) so a trivially-passing assertion is never emitted. |
| 8 | The three `WITCH-§3a` wiring traps are **real** | ✅ **TRUE — all three, semantically** | See §1.1. ⚠️ Their **line numbers** have drifted — WARN-1. |
| 9 | Capture-by-presence: the **code** matches the stated `J-W9` default | ✅ **TRUE** | There is **no `Capture` enumerator**, so a capture break is unpassable; the accepted leak is recorded in the header's inverse ledger (`h:265-272`) in the same words as `WITCH-§7 J-W9`. ⛔ Not re-litigated here — the design ruling is Jonathan's. |

### 1.1 The three traps, re-measured at source (this was the highest-value re-check)

- ⭐⭐ **SAPPER — CORRECT.** `ASummonedUnit::ApplyDetonation` is defined at **`SummonedUnit.cpp:4183`** and its blast is `FSiegeCombatStatics::ApplyRadialDamage(...)` at **`:4212`**. It is entered from **`Detonate()` at `:4179`** (contact, guarded by `bSuicide` at `:2417-2419` in `UpdateStateSiege`) and from **`HandleDeath()` at `:4230`**. ✅ **It is genuinely NOT reached through `PerformAttack`.** The finding stands.
- ⭐⭐ **SORCERER — CORRECT, and the line is exact.** `AncientGround.cpp:233` is literally `if (Unit->IsAncientGroundEmpowerer())` — the sorcerer is **counted and `continue`d** (`:237-238`), never added to `Occupants`; the grant `Unit->AddPermanentDamageStacks(Grant)` is in a **separate loop** at **`:270`** over the *recipients*. ✅ Breaking at `:270` would un-veil exactly the wrong actors.
- ⭐⭐ **MINER — CORRECT, and the line is exact.** `MinerUnit.cpp:514` is literally `if (Node->TryRegisterArrivedMiner(this))`; income is `AddMinerIncome()` at `:541`; and the payout really is off-stack — `ASiegePlayerState::HandleGoldTick` → **`SiegePlayerState.cpp:238` = `SetGold(Gold + TickGrant);`** ✅ (exact).

### 1.2 The absence gate — does it actually discriminate? (measured, case by case)

| future edit | test 7 result |
|---|---|
| 7th enumerator **with** a matching `ToString` arm | 🔴 **RED** — index 6 returns a real name ✅ the designed bite |
| enumerator **reordered** | 🔴 RED — declaration-order rows (`:422-429`) ✅ |
| enumerator **renamed** | 🔴 RED — hand-written name list ✅ |
| set **shrunk** to five | 🔴 RED — count row + index-5 lookup ✅ |
| `Walk`/`TakeDamage`/`Order` added under those names | 🔴 RED — by-name refusal rows ✅ |
| 7th enumerator with **no** `ToString` arm | 🟢 green — **NIT-1**, and see the mitigation there |

---

## 2. TASK-837 — THE CLAIMS, VERIFIED RATHER THAN ACCEPTED

| # | claim | verdict | measured |
|---|---|---|---|
| 1 | `304.8` / `609.6` each appear **exactly once** in `Source/` | ✅ **TRUE — I re-ran the grep myself** | The only **code literals** are `SiegeFogStatics.h:110` (`FogVisionOnsetUU = 304.8f`) and `:153` (`FogVisionCeilingUU = 609.6f`). Every other hit in `Source/` is **prose in a comment**. ⛔ Neither `300`, `600`, `10` nor `20` appears as a **fog distance** anywhere: the only `300`/`600` in the fog files are the *refusal* rows (`Tests/…:178-185`) and the Ballista `MinRange` tripwire (`Tests/…:526`). ✅ And the **test file carries no literal of either number** — everything derives from `Feet × CentimetresPerFoot` (`Tests/…:86-90`), per `SC-§37`. |
| 2 | Falloff is **quadratic ease-in (`t²`)**, exponent tunable | ✅ **TRUE** | `cpp:54-64`: `T = (d − Onset)/(Ceiling − Onset)` then `Clamp(Pow(T, Exponent), 0, 1)` with `Exponent = FogDensityExponent` (`h:189`, default `2.f`). One uu past the onset: `(1/304.8)² = 1.0765e-05` ⇒ **imperceptible by design**, exactly as `FOG-§7a` and the manager's downstream note say. The test asserts `< 0.001` (`Tests/…:245`), which is the correct instrument: it kills linear (0.00328) without pinning an unobservable value. ⭐ **The rewritten `TASK-843` pixel gate is right and the original would have failed a correct build.** |
| 3 | **Hard cut, not asymptote** | ✅ **TRUE** | `cpp:48-51` — `if (d >= Ceiling) return 1.f;` **exactly**. Test 2(b) asserts `Exact` (tolerance `0.f`) **at** the ceiling and at five beyond-distances (610 / 900 / 2100 / 3600 / 50000), plus a never-exceeds-1 row. ⇒ **every asymptote fails, and it fails on an exact comparison rather than a tolerance.** |
| 4 | `EffectiveVisionRadius` returns the request **bit-identically** with fog off | ✅ **TRUE — verified as bit-identity, not approximation** | `cpp:73-76` is the **first** statement: `if (!bFogActive) { return RequestedRadiusUU; }` — the tuning is not even read. Test 5 asserts with `Exact = 0.f` over nine ranges incl. 0 and 50,000 (`Tests/…:454-474`), so `|actual − expected| > 0` fails. ⇒ safe to call **unconditionally**; a fog regression stays attributable to fog. |
| 5 | The anti-triviality counters really bite | ✅ **TRUE — I recomputed both sweeps** | Test 4 (0→1200 uu, 201 samples @ 6 uu): **51 clear · 51 ramp · 99 opaque** ⇒ the `RampSamples >= 20` row is satisfied with margin, and a curve sampled only inside the bubble would fail *the sweep test itself*. Test 7 (0→4000 uu, 101 samples @ 40 uu): **16 pass-through · 85 clamped**, both ≥ 10 ⇒ the invented-floor branch is genuinely exercised. |
| 6 | The midpoint discriminator separates **all three** alternatives | ✅ **TRUE** | At `t = 0.5`: quadratic **0.25** · linear **0.50** · smoothstep `3t²−2t³` = **0.50** · Beer-Lambert ≈ **0.63**. Tolerance `1e-4` ⇒ each alternative misses by **2,500×** the tolerance. And the discrimination is not one row: 3(b) adds `0.0625` vs `0.25` and `0.5625` vs `0.75`; 3(c)'s **three-equal-steps** row (`:344-352`) is what reverses under a *concave* curve, catching Beer-Lambert on a second, independent property; 3(d) proves the rows measure the **exponent** and not the boundary code by retuning to `1.0` and reading `0.5`. |
| 7 | **No team parameter anywhere in fog** | ✅ **TRUE** | `grep "Team\|Viewer\|Controller\|AActor\|APawn\|ETeamId\|Owner\|Player"` over `SiegeFogStatics.h` returns **two comment lines and zero code**; the `.cpp` names no engine type beyond `FMath`. ⇒ **asymmetric fog is unrepresentable**, and no trivially-true "both teams agree" test was written (`FOG-§7a` forbids it). |
| 8 | Totality / degenerate inputs fail toward **no fog**, never no vision | ✅ **TRUE, with one bounded exception — WARN-3** | `cpp:22-25` non-finite → clear · `cpp:32-35` `Ceiling <= Onset` → hard step (**this is the divide-by-zero guard, and it precedes the division — verified, not defence in depth**) · `cpp:61` `IsFinite(x) && x > 0.f` correctly rejects **NaN**, which `x <= 0.f` would not · `cpp:84` non-finite/negative ceiling → request unchanged. The single deliberate inversion (`IsVisibleThroughFog` on a NaN **distance** → `false`, `cpp:101-104`) is correct and is argued at `h:332-335`. |

### 2.1 The three declared deviations — ruled explicitly, as required

- **(i) `FSiegeFogTuning` is a reflected `USTRUCT` in a "plain C++ statics" file — ✅ ACCEPTABLE. I concur with the manager's pre-ruling, and now on a *measured* precedent rather than a citation.** `SiegeStuckStatics.h` is structurally identical, line for line in shape: `CoreMinimal.h` → `SiegeStuckStatics.generated.h` → `USTRUCT(BlueprintType) struct FSiegeStuckTuning` with `EditDefaultsOnly` float members (`:135-163`) → `class GITCLAUDEUNREALTEST_API FSiegeStuckStatics` (`:165`). `SiegeFogStatics.h` mirrors it exactly (`:11`, `:13`, `:79-190`, `:209`). `NAV-§7` instructs QA not to flag it. ⛔ Not a one-class-per-header violation and not a new pattern. The `.generated.h` is correctly the **last** include.
- **(ii) `FogDensityExponent = 2.f`, a third tunable — ✅ IN SCOPE, now law (`FOG-§7a`). No deletion requested.** It is a *shape*, not a distance, so it cannot carry `FOG-§1`'s 30× trap, and test 3(d) makes the "one-word retune to linear" claim an assertion rather than a promise.
- **(iii) plain `enum` not `UENUM` (TASK-827) — ✅ CORRECT.** The `ESiegeLadderExit` / `ESiegeStuckAction` precedent, plus `WITCH-§6`'s M8 clause: an unreflected type cannot be accidentally replicated, and a naive replicated veil flag leaks the veiled unit's position to the enemy client's renderer. If TASK-829 needs Blueprint exposure it is a deliberate amendment with the leak re-argued.

---

## 3. THE CROSS-CUT — DID EITHER MODULE ACQUIRE THE OTHER'S SHAPE?

**No. The disagreement is intact in both directions, and it is structural on both sides.**

| property | invisibility | fog |
|---|---|---|
| team parameter | ✅ **`IsVisibleTo(ETeamId Viewer, ETeamId Target, bool)`** — present, because the feature **is** asymmetric | ⛔ **zero** functions take a team, viewer, controller or actor — because the feature **is** symmetric |
| distance / radius term | ⛔ **none** — no `float` appears in any of the six signatures | ✅ `float DistanceUU` / `RequestedRadiusUU` — the whole content |
| time term | ⛔ **none** (the "permanently" law) | ⛔ none (`bFogActive` is a bool the caller owns; duration is TASK-839's) |
| cross-include | ⛔ `SiegeInvisibilityStatics.h` includes only `CoreMinimal.h` + `Siegebound/TeamId.h` | ⛔ `SiegeFogStatics.h` includes only `CoreMinimal.h` + its own `.generated.h` |
| phantom `ESiegeTeam` | ⛔ **absent** | ⛔ **absent** |

⭐ **`ESiegeTeam` returns ZERO hits across all of `Source/`.** The shipped enum is `ETeamId` (`TeamId.h:14`), which is what TASK-827 uses and what TASK-837 sidesteps entirely by taking no team at all. ✅ `WITCH-§1`'s reason for existing is honoured by both.

⇒ **Neither drifted toward the other**, and the reason the manager put them under one pair of eyes is discharged: `FOG-§7a`'s "the two signatures disagree on purpose" is true of the shipped code, not just of the law.

---

## Findings

- **[WARN] `CONVENTIONS.md` `WITCH-§3a` / `SiegeInvisibilityStatics.h:107-117`, `:129`, `:199` — the `SummonedUnit.cpp`, `Tower.cpp` and `HeroCharacter.cpp` line citations have DRIFTED and are now wrong by +1 to +5 in the current tree.** Measured, cited → actual:
  `SummonedUnit.cpp` — detonation blast **4209 → 4212** (4209 is now a *comment* line) · `ApplyDetonation` def 4180 → **4183** · compose 4202 → **4205** · contact entry 2418 → **2419** · death entry 4227 → **4230** · `HandleDeath` def 4213 → **4216** · melee `ApplyDamage` 3050 → **3051** · ranged `FireProjectileAt` 3031 → **3032** · `PerformHeal` 2662 → **2663** · `ApplyHealing` (receiver) 2665 → **2666**.
  `Tower.cpp` — `FireProjectileAt` 307 → **312** · `FireChainZapAt` 359 → **364**. `HeroCharacter.cpp` — `DoMeleeAttack` 463 → **464** · `HandleDeath` 831 → **832**.
  ⭐ **This is NOT an error in the diff under review.** The cause is measured: **TASK-828 landed in parallel and edited exactly these files** (it added `#include "Siegebound/SiegeCombatStatics.h"` at `HeroCharacter.cpp:36`, `Tower.cpp:14`, `SpellLineSweep.cpp:15`, `SiegeCheatManager.cpp:21` and replaced the enumerations at `SummonedUnit.cpp:1669`/`:2215`, `Tower.cpp:225`/`:387`, `HeroCharacter.cpp:556`). TASK-827 measured a tree that has since moved under it, which is the same `TL-§5b` hazard applied to line numbers instead of suite totals.
  ✅ **The exact citations are:** `AncientGround.cpp:233` · `:270` · `MinerUnit.cpp:514` · `:541` · `SiegePlayerState.cpp:238` · `SpellLineSweep.h:103` · `TeamId.h:14` · `SiegeInvisibilityStatics.h:338` · `SiegeFogStatics.h:299`. ⇒ **two of `WITCH-§3a`'s three rows are exact; only the Sapper row drifted.**
  **Suggested fix (manager + TASK-829/849, not TASK-827):** every `WITCH-§3a` row already quotes its expression (`Node->TryRegisterArrivedMiner(this)`, `Unit->AddPermanentDamageStacks(Grant)`, `FSiegeCombatStatics::ApplyRadialDamage`), so the citations are self-correcting — **make the SYMBOL the primary key and the line a hint**, and have TASK-829 locate by symbol and TASK-849 verify by symbol. ⛔ A line-pinned law in a file three live lanes are editing will keep drifting; `FOG-§7`'s site table (`SummonedUnit.cpp:1663`, `Tower.cpp:220`, `HeroCharacter.cpp:551`, `SpellLineSweep.cpp:133`, `SiegeCheatManager.cpp:130`) has already drifted the same way and is `TASK-848`'s to re-pin.
- **[WARN] `handoffs/TASK-827-programmer.md` "What QA should scrutinise" item 3 vs `SiegeInvisibilityStatics.cpp:16-34` — the claim that a swapped `IsVisibleTo(Viewer, Target, …)` call "WOULD be caught by TASK-829's tests" is FALSE, and the reason matters more than the error.** The predicate is `(Viewer == Target) || !bTargetIsInvisible` — **exactly symmetric under exchange of its two team arguments for all 8 inputs** (rows 4 and 6 both expect `false`, so they do not discriminate a swap; they mirror it). ⇒ a swapped call site is **undetectable by any test that can be written today**. That is harmless *now* precisely because the swap cannot change an answer — but the mitigation on record is wrong, and **the moment an asymmetric term is added** (a "detector" unit that sees through veils, a per-team reveal, anything that consults `ViewerTeam` alone) **the swap becomes a silent defect with no guard and a handoff saying it is guarded.** Suggested fix: correct the record; have TASK-829 write the call site with named-argument comments (`/*ViewerTeam=*/`, `/*TargetTeam=*/`) as the cheap structural guard, and require any future asymmetric term to arrive with a swap-detecting row.
- **[WARN] `SiegeFogStatics.cpp:84` — `FogVisionCeilingUU == 0.f` is unguarded, editor-reachable, and blinds the whole army under fog.** The guard is `Ceiling < 0.f` (strict), and `meta = (ClampMin = "0")` on `h:152` makes `0` the **editor's floor value** — a designer who drags the slider to its stop, or types `0`, gets `min(Range, 0) == 0` for every acquisition ⇒ `IsVisibleThroughFog` false for every `d > 0` ⇒ **all combat stops while fog is up**. That is the exact outcome `cpp:80-83`'s own comment says must never happen ("a negative ceiling … would clamp every unit in the game to a negative range and stop all combat"), reached through the one input adjacent to the guarded one. ⛔ **Not a blocker:** it cannot occur with the shipped defaults, no caller exists yet (TASK-838 has not landed), and "ceiling 0 = see nothing" is a defensible reading. Suggested fix, for TASK-838/`TASK-850` to rule rather than for TASK-837 to patch blind: either raise the ceiling's `ClampMin` to a non-blinding floor **or** state on the record that `0` means "blind" deliberately — and if the latter, say so in the tunable's comment beside the consequence, per `HIGH-§1`.
- **[NIT] `SiegeInvisibilityStatics.h:381` + `.cpp:93` — the census tripwire has one named blind spot, and the file's own design already covers it.** `VeilBreakReasonCount` is a hand-maintained `constexpr` with no compile-time tie to the enum's cardinality, so a 7th enumerator added **without** a `ToString` arm leaves index 6 unrecognised and test 7 green. ⭐ **But that is precisely the case the missing `default:` hands to the compiler:** with no default label, an unhandled enumerator raises MSVC **C4062** / clang **`-Wswitch`**. ⇒ the runtime gate covers "added with an arm" and the compiler covers "added without one"; between them the closed set is enforced. Recorded so the residual is visible, ⛔ **no change requested** — a sentinel `Num` enumerator would be a seventh enumerator, which is the thing being prevented.
- **[NIT] `Tests/SiegeFogTest.cpp:243-244` — the comment's arithmetic is wrong by 100×.** "linear … reads 0.0033 here, ~3× the quadratic's 0.0000108" — the true ratio is **~305×**, and `FOG-§7a` itself says 300×. The assertion (`< 0.001`) is correct and does kill linear; only the prose is wrong. In a project whose comments are treated as law, fix the number.
- **[NIT] `Tests/SiegeFogTest.cpp:96` — the stated float-rounding gap understates the measured one by ~4×, and the tolerance headroom is tighter than the comment implies.** Measured in IEEE-754 binary32: `10.f*30.48f = 304.80001831…` vs `304.8f = 304.79998779…` ⇒ gap **3.05e-05**; `20.f*30.48f = 609.60003662…` vs `609.6f = 609.59997559…` ⇒ gap **6.10e-05** — not "~1.5e-5". Against `Tolerance = 1e-4` the ceiling row (`:163-164`) passes with only **1.6× headroom**. ⛔ Nothing fails today, and every other row is unaffected (`:195-196` compares two *shipped* values and is bit-exact). But anyone who "tightens" `Tolerance` below `6.2e-5` turns a correct build red — the same class of self-inflicted wound the `TASK-843` pixel gate just avoided. Suggested fix: correct the number and add "⛔ do not tighten below 1e-4" beside it.

---

## Notes for build-master

1. ✅ **Both subjects PASS. `TASK-835` and `TASK-843` may count `qa/TASK-847.md` as satisfied for `TASK-827` and `TASK-837` respectively** — and for **those two tasks only** (`SC-§29`). `TASK-828` still needs `TASK-848`; Lane W still needs `TASK-849`; Lane F still needs `TASK-850`.
2. ⛔ **Nothing here is a compile result.** Neither task compiled (`QUIET-MODULE`), and per this gate's spec a compile finding is a build-time event, not a reason to fail this gate. **Two compile-shape checks I did make, so the build is not surprised:** (a) `SiegeFogStatics.h`'s reflected-struct-plus-plain-statics layout is structurally identical to the shipped, already-compiling `SiegeStuckStatics.h`, with the `.generated.h` last; (b) both test files use `EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter`, which is the idiom in **all 29** shipped test files. ⚠️ Expect a UHT pass on the **new** `SiegeFogStatics.generated.h` — this is the first new reflected header in the batch, so an editor bounce (not Live Coding) is the safe compile path.
3. 📌 **Assert DELTAS, never an absolute** (`TL-§5b`): TASK-827 `+8`, TASK-837 `+9`, both **verified by reading the files**. ⛔ Do not gate on `338`/`344`/`361`/`370`/`371` — all stale — and note a bare `^IMPLEMENT_` grep over `Tests/` also matches `IMPLEMENT_PRIMARY_GAME_MODULE` elsewhere in the module and will miscount the file total.
4. ⚠️ **WARN-1 is the one to carry forward**, and it is not this batch's to fix: `WITCH-§3a`'s Sapper row and `FOG-§7`'s site table are line-pinned against files three live lanes are editing, and both have already drifted. **TASK-829/838 must locate by symbol; TASK-849/850 must verify by symbol.** The correct sites are re-measured in §1.1 above and are current as of this review.
5. ⚖️ **WARN-3 (`FogVisionCeilingUU == 0`) is a ruling to be made before `TASK-838` wires the clamp into the funnel every attack routes through** — it is inert today and must not block this batch, but it should not be discovered by a designer.

**Reviewed, not edited.** ⛔ No code, engine, MCP or Git action was taken by this gate.
