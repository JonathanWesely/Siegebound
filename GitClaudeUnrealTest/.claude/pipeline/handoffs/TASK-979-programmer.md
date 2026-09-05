# TASK-979 — [COMBAT-NOTICE] handoff (gameplay-programmer)

**Row:** unit notice range `600` → `2000`, as a **per-unit channel** — not a number change.
**Date:** 2026-09-04 · **Gate:** `TASK-985` · **Ship host:** `TASK-987`
**Inputs read:** the TASK-979 board row **whole, including all four amendments** · `qa/TASK-996.md` · `handoffs/TASK-978-programmer.md` findings as relayed on the board.

> ⛔ **`TL-§5c` — NOTHING WAS COMPILED OR EXECUTED FOR THIS ROW.** No build, no editor, no MCP, no PIE, no Git. Every number below is either a **source read** (reproducible from the `file:line` given) or a **declared** count. **No executed suite number appears in this document.**

---

## §0 — INSTRUMENTS (`SC-§39`), AND THE ONE RULE I FOLLOWED WITHOUT EXCEPTION

| instrument | control | result |
|---|---|---|
| `Read` | every character-exact claim, every edit target | ✅ authoritative |
| `Grep`/`grep` | **location only** — never quoted as source | ✅ per ⭐ `SC-§38a` |
| negative control | `grep "> LeashRange"` after the fix → **0** | ✅ scanner alive (it returned 2 hits for `> GetEffectiveLeashRangeUU()` in the same sweep) |
| CDO template check | `UClass::GetDefaultObject<T>()` exists at `Engine/.../Class.h:4624` and the project already uses it (`Barracks.cpp:126`, `SiegePlayerController.cpp:4852`) | ✅ not invented |
| UHT precedent | `static constexpr float` inside a UCLASS body: `GoldNode.h:352` | ✅ shipped precedent |

⭐ **`SC-§38a` honoured:** no source text in this document was taken from `Grep` output. Every quoted line was re-`Read`.

---

## §1 — THE CHANNEL: WHAT I BUILT AND WHERE THE PER-UNIT VALUE IS READ

**One constant, one column, one resolver, one binding.**

| piece | where | what |
|---|---|---|
| the constant | `SummonedUnit.h:842` | `static constexpr float UnitEngagementRadiusUU = 2000.f;` — **public**, so `TASK-980` and the tests read the symbol instead of re-typing the number |
| the default | `SummonedUnit.h:1227` | `float AggroRadius = UnitEngagementRadiusUU;` — ⛔ **not** a second `2000.f` literal |
| the column | `CardRow.h` (`FCardRow::NoticeRange`, immediately after `MinRange`) | `float NoticeRange = 0.0f;` — **0/blank = "use the class default"**. `TASK-993` puts `3600` in the Longbowman cell; ⛔ no other row gets a cell |
| the resolver | `SummonedUnit.cpp:4396` | `static float ASummonedUnit::ResolveNoticeRadiusUU(float ClassDefaultRadiusUU, float RowNoticeRangeUU)` |
| the binding | `SummonedUnit.cpp:1302` | `AggroRadius = ResolveNoticeRadiusUU(GetClassDefaultEngagementRadiusUU(), Row->NoticeRange);` — **beside** the shipped `AttackRange = Row->Range;` at `:1288`, exactly as item (6d-ii) ruled |
| the accessor `TASK-980` consumes | `SummonedUnit.h` — `float GetEngagementRadiusUU() const { return AggroRadius; }` | the **per-unit** value. ⛔ Deliberately **not** named "ceiling" — item (6d-iii) is explicit that the *name* invites the `min` |

### ⛔⛔ WHERE THE PER-CLASS VALUE IS READ — AND IT IS **NOT** THE BASE CDO

`SummonedUnit.cpp:4454`, `ASummonedUnit::GetClassDefaultEngagementRadiusUU()`:

```cpp
	if (const UClass* const MyClass = GetClass())
	{
		if (const ASummonedUnit* const ClassDefaults = MyClass->GetDefaultObject<ASummonedUnit>())
		{
			return ClassDefaults->AggroRadius;
		}
	}
	return AggroRadius;
```

- `GetClass()->GetDefaultObject<>()` ⇒ **this instance's class**. `AMinerUnit`'s CDO answers `0`, `ASorcererUnit`'s answers `0`, `ASummonedUnit`'s answers `2000`, and a `BP_Unit_*` that overrode the property answers *its own* number.
- ⛔ `GetDefault<ASummonedUnit>()` appears **0 times on a code line** in `SummonedUnit.cpp` (the two textual hits are both comment lines warning against it — `:1298`, `:4456`). Asserted structurally in `Siegebound.Notice.TheDefaultIsTheEngagementRadiusAndTheSealedClassesAreStillZero` item (f).
- The template form is safe here (it `check()`s `IsA<T>`, and `this` is always an `ASummonedUnit`); the `SiegePlayerController.cpp:5450` warning about it applies to classes that *might not* be `T`, which is not this call.

### ⛔ THE `2000` IS A **DEFAULT**. IT IS NOT A CAP, AND THE REFUSAL IS STRUCTURAL

`ResolveNoticeRadiusUU` contains **zero `FMath::Min` and zero `Clamp`** — asserted, not asserted-about (`SiegeUnitNoticeRangeTest` test 2 item (f) extracts the function body and counts). Order of the three branches:

1. `!(ClassDefaultRadiusUU > 0.f)` ⇒ return the class default. **The seal, first, deliberately.**
2. `RowNoticeRangeUU > 0.f && FMath::IsFinite(...)` ⇒ **return the row value, unmodified.** A `3600` cell comes back out as `3600`.
3. otherwise ⇒ the class default (the sparse case).

A row of `3600` survives, and `Siegebound.Notice.APerUnitNoticeRangeAboveTheDefaultSurvivesTheChannel` asserts it **as a property** (`Resolve(2000, 3600) > 2000`) as well as a value, plus a **synthetic `2000 × 2`** so a clamp cannot hide behind an allow-list of known card numbers. Every one of those rows goes **RED** against `min(Row, 2000)`.

---

## §2 — ⛔⛔ THE LEASH INVERSION: BOTH SITES, AND WHAT I SIZED IT AGAINST

**Both sites fixed. Neither reads the raw member any more.**

| site | file:line (post-diff) | body |
|---|---|---|
| 1 of 2 | `SummonedUnit.cpp:1734` | `UpdateState` |
| 2 of 2 | `SummonedUnit.cpp:1979` | `UpdateStateStandardCommanded`, the `Attack`/`default` case |

Both now compare against `GetEffectiveLeashRangeUU()`. **Measured after the edit:** `> LeashRange` ⇒ **0** occurrences in `SummonedUnit.cpp`; `> GetEffectiveLeashRangeUU()` ⇒ **exactly 2**. Both counts are asserted in test 3 item (e), so a future third drop site that forgets the accessor turns a row red.

### The shape of the fix — an ORDERING, derived, not a typed number

`SummonedUnit.cpp:4425`:

```cpp
	return FMath::Max(LeashRangeUU, NoticeRadiusUU * SafeMultiplier);
```

with `LeashMarginMultiplier = 1.5f` (`SummonedUnit.h`, `EditAnywhere`, `ClampMin = "1.0"`), and the resolver floors any margin `<= 1` at exactly `1.0`.

⭐⭐ **SIZED AGAINST `3600`, NOT `2000` — AND IT NEEDS NO SECOND NUMBER TO BE.** Because the leash is computed **from the unit's own notice radius**, the Longbowman's `3600` yields `5400` automatically, and a future card larger than `3600` inherits the ordering with zero edits. Test 3 item (c) asserts `EffectiveLeash > Notice` at **four** radii: the historic `600`, the new default `2000`, the excepted `3600`, and a synthetic `8000`.

⭐⭐ **AND THE MULTIPLIER IS MEASURED, NOT INVENTED — this is the strongest thing in the diff.** The game shipped `LeashRange 900` against `AggroRadius 600`, which is **exactly `1.5`**. So:

- `ResolveEffectiveLeashRangeUU(900, 600, 1.5) == max(900, 900) == 900` — **bit-identical to what shipped**. Fed the pre-ruling numbers, the new mechanism returns the old behaviour, so this is provably a **repair**, not a balance change riding a ruling about a different number. Asserted at `Exact` tolerance in test 3 item (b), and item (a) pins `LeashMarginMultiplier == 900.f / 600.f`.

⚠️ **THE CONSEQUENCE, STATED (`HIGH-§1`, 🧑 `J-F27`):** the effective leash becomes **3000** for a default unit and **5400** for the Longbowman. A melee unit now disengages at 3000 uu instead of 900. That is forced arithmetic rather than taste — his `2000` notice makes *any* leash below 2000 inert by construction, and a leash *equal* to notice would thrash at the boundary. It is flagged for him; it does not block.

⛔ **THE GROUPED LANE IS UNTOUCHED.** `UpdateStateGrouped` still has no distance-from-self drop path (his ruling: *"commanded units DO NOT LOSE THEIR COMMANDS"*). Asserted: `LeashRange` ⇒ **0** code occurrences in that body (test 3 item (e), third row).

---

## §3 — ⛔⛔ ITEM (6f): WHAT I DID WITH `SiegeFogClampTest.cpp:1021` AND `:1063–1068`

**Disposition: RE-DERIVED IN PLACE, and INVERTED. Not renumbered, not re-signed, not deleted.** `SiegeFogClampTest.cpp` still declares **9** tests — I added no new macro there, because the fix is to the *argument*, not to the count.

**(1) The operand at `:1021` no longer exists as a literal.** The `SiteRadii` table's unit row now reads `UnitNoticeRadiusUU`, taken from `GetDefault<ASummonedUnit>()->AggroRadius` behind a null self-check that `AddError`s and returns `false`. ⛔ I did **not** hand-type `2000.f` — the board names that as *"the identical defect one change later"*, and it is: a literal asserted against itself is green forever.

**(2) The `:1063–1068` claim is replaced by three rows that make the OPPOSITE claim**, because the old claim's premise (`600 < 609.6`, an arithmetic coincidence) is gone:

- **(a) the premise, asserted separately** — `UnitNoticeRadiusUU > Tuning.FogVisionCeilingUU`. Its message says in terms that if this goes red, the argument must be re-derived *again*, not the numbers re-signed.
- **(b) the value** (spec item 5(c)) — `EffectiveVisionRadius(UnitNoticeRadiusUU, /*fog=*/true, Tuning) == Tuning.FogVisionCeilingUU`, `Exact`.
- **(c) the cost** — `EffectiveVisionRadius(..., true, ...) < UnitNoticeRadiusUU`, which is **literally the funnel's own cut predicate** (`SiegeCombatStatics.cpp:207`). A green row here is the statement that the per-candidate loop now runs.

**⭐ WHY MY REPLACEMENT CANNOT PASS AGAINST THE DEFECT — the argument, not an assurance.** The retired row could not fail because **no shipped value reached it**: it compared a literal `600.f` to a literal `600.f` through a function that returns its argument unchanged below the ceiling. Both operands were constants of the test. My replacement takes **one operand from the shipped CDO and the other from the shipped tuning struct**, and asserts a relation *between* them:

- revert `AggroRadius` to `600` ⇒ **(a) fails** (600 < 609.6), **(b) fails** (`min(600, 609.6) = 600 ≠ 609.6`), **(c) fails** (600 is not < 600). Three red rows, and (a)'s message names the reason.
- add a per-site clamp at the acquisition call instead of the chokepoint ⇒ still caught, by test 6's separate uniqueness assertion (untouched).
- raise the fog ceiling above the notice radius ⇒ **(a) fails first**, with a message telling the reader the claim itself needs re-deriving. That is the intended behaviour, not a fragility.

There is **no path** on which the shipped notice radius changes and these rows stay green, because the shipped notice radius **is** an operand.

**(3) I also added the excepted-card rows (d) in the same test**, fed through the shipped resolver rather than typed into the fog call: `EffectiveVisionRadius(Resolve(2000, 3600), fog OFF) == 3600` and `(fog ON) == 609.6`. ⛔ If anyone ever adds `min(Row, 2000)`, the resolver returns `2000` and **both** rows go red at once — the anti-clamp assertion crossed with the fog claim, in the file that owns fog.

---

## §4 — ⚠️ ITEM (6e): **BOTH** HALVES OF THE COST. THEY ARE DIFFERENT CLAIMS

**I confirmed the enumeration myself in one `Read`, as item (6)(b) required — not relayed.**
`SiegeCombatStatics.cpp:32`:

```cpp
void FSiegeCombatStatics::GatherTeamAgentsFiltered(const UWorld* World, ETeamId ViewerTeam, bool bWantHostile, TArray<AActor*>& Out)
```
`:48`:
```cpp
	UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);
```

**Half one — the ENUMERATION delta is `1.00×`.** The signature takes **no radius and no position**; a radius is not merely unused, it is *unrepresentable* at this seam. Widening the reach does not widen the search. ⛔ The `11.1×` figure is an **AREA quoted as a COST** and is **REFUTED**. It appears at **0 occurrences** anywhere in `Source/` — I did not write it, and the shipped comment beside `UnitEngagementRadiusUU` says `1.00×` with its mechanism.

**Half two — the TOTAL GATHER COST IS NOT UNCHANGED.** `SiegeCombatStatics.cpp:207`:

```cpp
		if (EffectiveRadiusUU < Vision->RequestedRadiusUU)
```

- at `AggroRadius = 600`: `min(600, 609.6) = 600`, and `600 < 600` is **false** ⇒ the `RemoveAll` loop is **provably skipped** and costs **zero** collision queries.
- at `2000`: `min(2000, 609.6) = 609.6 < 2000` is **true** ⇒ the loop **RUNS**: `ActorGetDistanceToCollision` (`:230`) **per candidate, per unit, at 4 Hz, whenever fog is up** — work that has **never executed on this path in this game**.

⚖️ **A refutation is not a licence.** Both halves are written into the shipped comment on `UnitEngagementRadiusUU`; neither the word *"free"* nor the figure `11.1×` appears anywhere in the diff.

**Half three — the GAMEPLAY load is SEPARATELY UNMEASURED (item 6c).** Far more units simultaneously engaged ⇒ pathing/AI/animation. **Not measured. Not guessed. It needs a profiled match.** *"Probably fine"* is not an answer; *"not measured"* is, and that is what the code says.

---

## §5 — THE OTHER SPEC ITEMS

- **(3) The two class contracts are ASSERTED, never changed.** `MinerUnit.cpp:72` and `SorcererUnit.cpp:23` are **byte-untouched** — I opened neither for editing. Asserted three ways in test 1: the two CDOs still read `0`; **their class-default accessor** reads `0` (the anti-base-CDO tripwire); and `ASummonedUnit`'s reads `2000` as the positive control.
- **(4) Fog is free and I wrote no fog code for it.** `AcquireTarget` still hands `AggroRadius` to `FSiegeVisionQuery::SeeingFrom(MyLocation, AggroRadius)` — that line is **unchanged**. `min(2000, 609.6) = 609.6` arrives through the one chokepoint. **Zero** new fog reads at any site.
- **(5) Tests** — (a) CDO default = the constant ✅ · (b) Miner/Sorcerer still `0` ✅ · (c) fog ON ⇒ `609.6` ✅ (`SiegeFogClampTest` test 7 row (b)) · (d) fog OFF ⇒ bit-identical ✅ (the `SiteRadii` loop, now CDO-driven so it tracks the shipped value).
- **(6a/6d-iv)** — §2 above.
- **(6b) The commanded-lane bound is built, as the GENERAL FORM.** In `AcquireEnemyNearPoint` (`SummonedUnit.cpp:2375`): `const float NoticeRadiusUU = GetEngagementRadiusUU();` at `:2429`, applied as `if (Distance > NoticeRadiusUU) continue;` at `:2464`, measured **from self** with the same bounds-aware metric `AcquireTarget` uses. ⛔ The `SeeingFromUnbounded` vision query is **unchanged** — `TASK-980` routes this **existing read** through the fog accessor, exactly as its spec requires; it does not have to add the general form. ⛔ The **zone disc survives beside it** (two terms, not a replacement). ⛔ The **order assignment is untouched** — this cuts acquisition only; the unit stays assigned, holds station and goes blind.
- **(6c) The `bRanged` fence held.** `bRangedAttack` ⇒ **0** code occurrences in `AcquireEnemyNearPoint`'s body, asserted in test 4. `CrystalTower` ships `bRanged = FALSE` at `Range 800` and would have escaped such a gate.
- **(6d-v) The stale comment at the old `SummonedUnit.h:1037` is rewritten**, both false clauses named explicitly (it is `2000`, and it *is* a card stat now).
- **(6d-vii) / the fences:** ⛔ `cards.csv` and `DT_Cards` **untouched** (`TASK-993`'s). ⛔ `SiegeFogStatics.{h,cpp}` and `Tests/SiegeFogTest.cpp` **never opened, never edited** — `TASK-981` owns them. ⛔ No compile, no editor, no MCP, no Git.

---

## §6 — 🧑 THE NUMBERS HE HAS NOT SEEN

- **`J-F28` — the commanded-circle blindness.** `USiegePlayerController::GroupRadiusMax = 5000.f` (`SiegePlayerController.h:2076`, **read, ⛔ never retuned**). A legal maximum guard circle is **2.5×** wider than the `2000` default ⇒ a unit at its centre covers `2000²/5000² = 16%` of the circle's area and is **blind to 84% of it in CLEAR WEATHER**. This lane had **no** notice bound before, so the bound is being **introduced**, not tightened. The test emits the derived figure via `AddInfo` and goes red if `GroupRadiusMax` moves, so the number cannot rot silently.
- **`J-F27` — the leash.** 900 ⇒ **3000** (default) / **5400** (Longbowman). See §2.
- **`J-F29` — the Archer/Wizard dead band. ⭐ AND THE BOARD'S FRAMING OF IT IS BACKWARDS; see §7 finding (5).**

---

## §7 — ⚠️ FINDINGS NEITHER THE BOARD NOR THE DISPATCH ANTICIPATED

**(0) ⛔⛔ A SELF-REVIEW PASS CAUGHT THREE DEFECTS IN MY OWN DIFF, AND ONE OF THEM WOULD NOT HAVE COMPILED. RECORDED IN FULL, BECAUSE THE THIRD ONE IS THE INTERESTING ONE.**
Before writing this handoff I ran a second, adversarial read of the diff against the shipped access levels and idioms. It found:
- ⛔ **BLOCKER (would not compile):** `AggroRadius`, `LeashRange` and `LeashMarginMultiplier` are **`protected`** (⚠️ **COORDINATES CORRECTED — see §11(C); the figures originally written here (`public:` 192 → `protected:` 1101 → `private:` 1696, members 1230/1243/1256) were ⛔ WRONG BY ~10 LINES.** Measured at QA's instant: `public:` **200** → `protected:` **1111** → `private:` **1706**, members **1240/1253/1266**. Measured **after** the QA-loop-1 repair: `public:` **200** → `protected:` **1129** → `private:` **1724**, members **1258/1271/1284**), and there are **no `friend` declarations**. My first draft read them raw from both test files — **9 sites**, every one an `error C2248`. ⚠️ **Root cause worth keeping:** I copied the CDO-read pattern from `SiegeBuildingStackTest.cpp:119`, where it is legal *because* `ABuilding::MaxStackHeightMultiplier` is **public** (`Building.h:186`, under `public:` at `:65`). ⇒ ***a pattern copied from a file where it is correct can be illegal in the file you paste it into, and the difference is one access specifier 800 lines away.*** Every other non-member CDO read in this codebase already goes through an accessor (`UnitCDO->GetCapsuleComponent()`, `Barracks.cpp:128`; `GetPermanentDamageBonusPerStack()`, `DeckBuilderWidget.cpp:1219`). **Fixed:** all five `->AggroRadius` reads now use the already-public `GetEngagementRadiusUU()`, and two public inline getters were added beside `GetEffectiveLeashRangeUU()` — `GetLeashRangeFloorUU()` and `GetLeashMarginMultiplier()`.
- ⛔ **A TAUTOLOGY THE FIX WOULD HAVE CREATED.** Once both operands go through the accessor, test 1(e)'s `GetEngagementRadiusUU() == AggroRadius` becomes `x == x` — **permanently green against every possible implementation, including `return UnitEngagementRadiusUU;`**. ⇒ **This is `SC-§60` appearing *inside the fix for `SC-§60`*.** Re-derived as a **difference between two classes** (`base != miner`), which a constant cannot produce — and that row is now the one `TASK-980` depends on, since its firing seam consumes that accessor.
- **NIT:** a `FString::Printf` in the re-derived fog row passed `UnitNoticeRadiusUU` to a format string with no specifier — the message promised a number it never printed. Fixed with a `%.1f`.
⇒ ⚖️ ***None of the three was visible from reading the diff for correctness; all three needed the diff read against the surrounding file.*** Declared so `TASK-985` knows the diff has already been adversarially read once, and by what.

**(0a) ⛔⛔ AND THE ONE I AM LEAST COMFORTABLE WITH: MY OWN DIFF CREATED A STALE COMMENT 30 LINES ABOVE THE CODE THAT FALSIFIED IT, AND MY FIRST PASS DID NOT SEE IT.**
`AcquireEnemyNearPoint`'s shipped `TASK-838` paragraph said: *"Handing over some other number (`AggroRadius`, `AttackRange`) would narrow a commanded unit's pick ⛔ WITH FOG OFF, i.e. a shipped behaviour change wearing a fog card's commit message."* **My item-(6b) bound does exactly that, and I left the sentence sitting above it.** ⇒ **Amended, not deleted** (struck with its reason): the observation was *correct*, Jonathan then ruled the clear-weather narrowing **IN** by name, **and its real warning is honoured** — which is *why* the bound is a site-local cut on a **combat** row with the fog query (`SeeingFromUnbounded`) left untouched, rather than a change to what this site hands the funnel.
⇒ ⚖️ ***This is finding (1) below happening to ME, in the same file, in the same hour I wrote finding (1).*** The generalisation is stronger than I first wrote it: **a comment that documents why something was NOT done becomes false the moment someone does it, and it is invisible to every test, every grep for the changed value, and to the author's own review of their own diff.**

**(1) ⛔⛔ THE FALSE CLAIM WAS IN *THREE* PLACES, NOT TWO — AND THE THIRD IS SHIPPED CODE.**
> ⛔⛔ **AMENDED AT QA LOOP 1 — THIS COUNT WAS ITSELF WRONG. IT WAS ⛔ NINE, NOT THREE.** QA measured five (adding `SummonedUnit.h:125`, `:131`, `:1895`); a claim-shaped census then found **three more that neither the report nor the dispatch named** (`SummonedUnit.h:150`, `SiegeFogClampTest.cpp:1047`, and a pre-existing stale `Archer 700`), plus the WARN-3 site. ⇒ ⚖️ ***The section below was RIGHT about the mechanism and WRONG about the number, and it undercounted in exactly the way it warned about.*** **Full ledger in §11(A).**
The board named `SummonedUnit.h:1037` (item 6d-v) and `Tests/SiegeFogClampTest.cpp:1063–1068` (item 6f). There is a **third**: `ASummonedUnit::AcquireTarget`'s own paragraph in `SummonedUnit.cpp` asserted *"AggroRadius is the GDD §3.8 PROFILE CONSTANT 600 … so this site already sits INSIDE the 609.6 ceiling and fog does not narrow it."* That is false in **both** clauses on this diff, and the retired test's rationale is **near-verbatim the same sentences** — i.e. the stale claim propagated by **copy**, from shipped code into a test's justification. ⇒ ⚖️ **Retiring the test alone would have left two live copies of the refuted claim, one of them in the engine.** I rewrote it, and recorded there that the old paragraph's *closing* sentence — *"it must stay correct if `AggroRadius` is ever retuned above the ceiling"* — is exactly why **no rewiring was needed**: the site kept handing the query over even while explaining that fog could not bite it.
⇒ **Generalises:** `SC-§60` says a green test can certify a defect. This case says **a comment can too, and it travels by copy-paste into the test that then certifies it.** A census that greps for the stale *number* misses this; only a search for the stale *claim* finds all three.

**(2) ⛔⛔ ITEM (3)'s PRESCRIBED TEST CANNOT SEE THE DEFECT ITEM (6d-ii) CREATES.**
Item (3) says *"Assert both are still `0` in a test."* A CDO assertion (`GetDefault<AMinerUnit>()->AggroRadius == 0`) is **permanently green regardless**: `LoadStatsAndStart` never writes the CDO — it writes the **spawned instance**. So the obvious implementation (`AggroRadius = Row->NoticeRange` straight over the member, or a resolver that consults the row before the seal) would leave **every spawned miner running at 2000** while item (3)'s own test stayed green. This is the same shape as item (6f), one level up. ⇒ That is why the seal lives **inside the pure resolver** and is asserted on the **function** (`ResolveNoticeRadiusUU(0, 3600) == 0`), which *can* fail, in addition to the CDO rows, which cannot.

**(3) ⚠️ A LITERAL `2000` CENSUS OVER `Source/` RETURNS MORE THAN ONE, THROUGH NO FAULT OF THIS DIFF. Pre-declared so `TASK-985` item (1a) does not file a false BLOCKER.** Measured on code lines, shipping source only:
`SummonedUnit.h:842` (**mine — the constant**) · `Castle.h:359` and `Castle.h:885` (`MaxHP` / `CurrentHP` = 2000 **hit points**) · `GitClaudeUnrealTestCharacter.cpp:35` and `Variant_SideScrolling/SideScrollingCharacter.cpp:43` (`BrakingDecelerationWalking`). ⇒ **Different quantities that share a round number.** The one-literal claim is *"exactly one notice/engagement `2000`"*, and I wrote it that way in the shipped comment rather than leaving a claim a grep would falsify.

**(4) ⭐⭐ THE LEASH MARGIN WAS ALREADY IN THE DATA: `900 / 600 = 1.5` EXACTLY.** The board asked me to *"state the new number and its consequence"*. I found there is a better answer than a number: the **shipped ratio** was already 1.5, so a margin-based leash **reproduces the shipped 900 bit-identically at the old 600** while holding the ordering at 2000 and 3600. The fix is therefore demonstrably a repair rather than a balance change — and there is no second number for anyone to maintain when `TASK-993` lands.

**(5) ⛔⛔ `J-F29`'s DEAD BAND IS **NOT CREATED** BY THIS ROW — IT EXISTS TODAY, AND THIS ROW **SHRINKS IT 15×**. THE BOARD FRAMES IT BACKWARDS.**
Item (6d-iii-a) presents *"at notice 2000 and firing 2100 their top 100 uu is DEAD"* as a consequence left open by this row. Measured against the **shipped** numbers instead:
- **today:** `Archer`/`Wizard` fire at `2100` and acquire at `600` ⇒ **1500 uu of their firing range is unusable**, and a target that drifts past `900` is dropped by the leash entirely ⇒ their real engagement envelope is **~900**, not 2100.
- **after this row:** acquisition `2000` ⇒ the dead acquisition band is **100 uu**, and the effective leash `3000` means a retained target can be fired on across the **whole** `2100`.
⇒ ⚖️ **`TASK-979` improves `J-F29` from a 1500-uu defect to a 100-uu rounding artefact.** ⛔ I still **left the Archer/Wizard value pending** exactly as ordered — no cell, no guess. But when it is put to Jonathan it should be put as *"we shrank this by 15×; do you want the last 100 uu?"*, not as *"this row leaves your two cards nerfed."* The `−4.8%` framing measures against the card's `Range` column; the player-visible number was never `2100`.

**(6) ⚠️ `GroupRadiusMax` IS `protected` (`SiegePlayerController.h`, last access specifier before `:2076` is `protected:` at `:1645`).** The board's `names:` line says to **quote** it. A test cannot read it via a CDO, so test 4 quotes it with a **source-text probe** (`GroupRadiusMax = 5000.f` ⇒ exactly 1) and derives the 84% figure from that. Flagged for `TASK-980`/`TASK-985`: if either intends to *read* it, it needs an accessor first.

**(7) The new commanded-lane bound incidentally seals `AMinerUnit`/`ASorcererUnit` at that site too** (`AggroRadius 0` ⇒ every candidate cut). **No behaviour change** — `UpdateStateGrouped`'s guard 2 (`CanEverAttack()`) already forces `CurrentTarget` null before `AcquireEnemyNearPoint` is reached for them — but it is now defence-in-depth rather than a single point, and it is written down so it is not later mistaken for dead code.

---

## §8 — SUITE COUNT (`TL-§5b` / `TL-§5c`)

- **445 declared** across `Source/GitClaudeUnrealTest/Siegebound/Tests/` at my instant (census of `IMPLEMENT_*_AUTOMATION_TEST` macro lines).
- **This row's delta: `+4` declared** — all four in the new `Tests/SiegeUnitNoticeRangeTest.cpp`. `SiegeFogClampTest.cpp` stays at **9 declared**: item (6f) was discharged by **re-deriving test 7's rows in place**, which is a change of argument, not of count.
- ⛔ **Nothing was executed.** 445 is a **declared** count, not a pass count. `TASK-987` derives the executed number at its own instant.

---

## §9 — FILES TOUCHED

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | `UnitEngagementRadiusUU`; `ResolveNoticeRadiusUU`; `ResolveEffectiveLeashRangeUU`; `GetEngagementRadiusUU`; `GetEffectiveLeashRangeUU`; `GetLeashRangeFloorUU`; `GetLeashMarginMultiplier`; `GetClassDefaultEngagementRadiusUU`; `AggroRadius` default + rewritten doc; `LeashRange` doc; new `LeashMarginMultiplier` |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | four new definitions; the `LoadStatsAndStart` binding; two leash sites; the `AcquireTarget` stale paragraph; the `AcquireEnemyNearPoint` notice bound **+ its own stale `TASK-838` clause (§7 finding 0a)** |
| `Source/GitClaudeUnrealTest/Siegebound/CardRow.h` | `FCardRow::NoticeRange` |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogClampTest.cpp` | include; test 7 CDO read + the re-derived/inverted rows + the excepted-card rows |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeUnitNoticeRangeTest.cpp` | **NEW** — 4 tests |

**⛔ NOT touched, deliberately:** `MinerUnit.cpp` · `SorcererUnit.cpp` · `SiegePlayerController.{h,cpp}` · `Docs/Data/cards.csv` · `/Game/Data/DT_Cards` · `SiegeFogStatics.{h,cpp}` · `Tests/SiegeFogTest.cpp` · `SiegeCombatStatics.{h,cpp}`.

**⭐ No `names:` fence was breached.** Every file I edited is named on the row. `CardRow.h` is the one file not literally listed — item (6d-vii) orders *"You ship the CHANNEL, the ⛔ `FCardRow` FIELD and the BINDING"*, so the field is explicitly in scope and `CardRow.h` is where `FCardRow` lives. Declaring it loudly rather than quietly, per the dispatch.

---

## §10 — WHAT QA (`TASK-985`) SHOULD SCRUTINISE HARDEST

1. **The per-class read.** `SummonedUnit.cpp:4461–4466`. Confirm it is `GetClass()->GetDefaultObject<>()` and **not** `GetDefault<ASummonedUnit>()`. This is the `Building.cpp:302` defect's second chance.
2. **The seal ordering inside `ResolveNoticeRadiusUU`.** The class-default test must come **first**. If a reviewer "simplifies" it to test the row first, a card cell can un-seal a miner and **item (3)'s own CDO test stays green** (§7 finding 2).
3. **That my re-derived fog rows can actually fail.** Mentally revert `AggroRadius` to `600` and check that rows (a), (b) and (c) of `SiegeFogClampTest` test 7 all go red. If any stays green, I reproduced the defect I was sent to remove.
4. **Both leash sites, by count not by eye.** `> LeashRange` must be 0; `> GetEffectiveLeashRangeUU()` must be 2.
5. **The `2000` census.** Please read §7 finding (3) before item (1a) — a bare grep returns 5 hits and 4 of them are pre-existing and unrelated.
6. **`FCardRow::NoticeRange` has no CSV column yet.** That is intended (`TASK-993` owns the data) and follows the shipped `SpellDelivery` precedent; a `DT_Cards` reimport may log a benign missing-column notice. Every row deserializes `0` ⇒ every unit resolves to its class default ⇒ **the column's existence changes no behaviour today**.
7. **⭐ THE `AttackRange` CENSUS RECONCILES TO A DIFFERENT TOTAL, AND ZERO CONSUMERS MOVED. Pre-declared so `TASK-978`'s table is not read as broken.** Module-wide occurrences: **33 → 38**. **Every one of the +5 is comment/doc prose** (`CardRow.h`'s `NoticeRange` doc citing `AttackRange = Row->Range`; the `LoadStatsAndStart` binding comment; the struck clause in §7 finding (0a); the new test file's header). ⛔ **The executable side is untouched: all 10 `SummonedUnit.cpp` read lines from the census are present** (now at `:1762 :1932 :2002 :2107 :2650 :2820 :2916 :3228 :3817 :4032`, with `:3228` still carrying two reads on one line), plus the `:1288` write, and **`Tower.cpp` is byte-untouched at 6**. ⇒ **0 new `AttackRange` consumers.** `TASK-980`'s consumer table is unaffected.
8. **Compile risk I could not retire without a build (`QUIET-MODULE`):** `float AggroRadius = UnitEngagementRadiusUU;` is a UPROPERTY initialised from a named constant. Precedent exists (engine: `int32 X = DirectLink::InvalidStreamPort;` in a UPROPERTY; project: `static constexpr float` inside a UCLASS at `GoldNode.h:352`), and UHT does not evaluate NSDMIs — but it is the one construct in this diff with no in-project precedent for the *combination*. If UHT objects, the one-line fallback is to move the assignment into `ASummonedUnit`'s constructor and leave the header declaration bare; ⛔ do **not** "fix" it by typing `2000.f`.

---

# §11 — QA LOOP 1 REPAIR (`qa/TASK-979.md` → FAIL: 1 BLOCKER · 5 WARN · 4 NIT)

**Date:** 2026-09-04 · **Loop 1 of max 3** · ⛔ **NO compile, NO editor, NO MCP, NO Git** (`QUIET-MODULE`; `TASK-987` owns the wave's one compile).
⛔ **`TASK-981`'s files (`SiegeFogStatics.{h,cpp}`, `Tests/SiegeFogTest.cpp`) were NOT opened and NOT edited.** ⛔ **No `names:` fence breach** — every file touched is on the row (board `:18063`).

> ⛔ **THE REPAIR CANNOT MOVE A NUMBER.** Every edit is comment text, with **one declared exception**: one `TEXT(...)` **label** in a test's message array (§11(D)). ⛔ Zero logic, zero new symbols, zero macro-count change, zero operand change.

## §11(A) — ⛔ THE BLOCKER: HOW I FOUND THE LIVE ONES, AND WHY IT WAS NINE

⭐ **`SC-§47` applied literally: I did not re-read the paragraphs I wrote.** Re-reading a wrong description agrees with the author every time. Instead I enumerated the **behaviours the diff changed** and censused the **claim shapes** that assert them, adjudicating every hit with `Read` (`SC-§38a`):

| behaviour changed | claim shape censused |
|---|---|
| `AggroRadius` 600 → per-unit `UnitEngagementRadiusUU` | `\b600\b`, `aggro`, `Aggro` over both files |
| `LeashRange` 900 → `GetEffectiveLeashRangeUU()` | `\b900\b`, `leash`, `Leash` |
| `AcquireEnemyNearPoint` gained a from-self bound | `instead of AggroRadius`, `AggroRadius-from-self`, `UNBOUNDED` |

⇒ **the claim-census returns hits a value-grep cannot**, because sites 6, 7 and 9 below contain **no number at all** or none that matches.

| # | site | claim | who found it | state |
|---|---|---|---|---|
| 1 | `SummonedUnit.h` AggroRadius doc | "profile constant 600" | board item (6d-v) | fixed pre-review |
| 2 | `SiegeFogClampTest.cpp:1063-1068` | "600 … untouched by fog" | board item (6f) | fixed pre-review |
| 3 | `SummonedUnit.cpp` `AcquireTarget` para | "PROFILE CONSTANT 600 … fog does not narrow it" | ⭐ me, §7(1) | fixed pre-review |
| 4 | `SummonedUnit.h:125` | "within AggroRadius (**600**)" | 🔍 **QA** | ⭐ **FIXED NOW** |
| 5 | `SummonedUnit.h:131` | "beyond **LeashRange (900)**" | 🔍 **QA** | ⭐ **FIXED NOW** |
| 6 | `SummonedUnit.h:1891-1897` | "a 2D disc **instead of AggroRadius-from-self**" | 🔍 **QA** (the BLOCKER) | ⭐ **FIXED NOW** |
| 7 | `SummonedUnit.h:150` | "state machine (**aggro 600, leash 900**, … Archer **700**)" | ⛔⛔ **ME — named by NEITHER the report NOR the dispatch** | ⭐ **FIXED NOW** |
| 8 | `SiegeFogClampTest.cpp:1028-1031` | "tracks a Blueprint override and a bound card row" | 🔍 QA (WARN-3) | ⭐ **FIXED NOW** |
| 9 | `SiegeFogClampTest.cpp:1047` | "the **UNBOUNDED** sentinel (**AcquireEnemyNearPoint**…)" | ⛔⛔ **ME — named by NEITHER** | ⭐ **FIXED NOW** |

⇒ ⚖️ ***The BLOCKER's own lesson recurred INSIDE the report that raised it: QA's list of three was also short by two.*** ⭐ Sites 7 and 9 are why the method matters — **site 9 contains no number**, so no value-grep at any threshold could reach it, and **site 7 sits in the same 30-line narrative block as sites 4 and 5**, which QA read and I had read twice.

### ⛔ SITE 6 — WHAT THE DECLARATION NOW SAYS (`SummonedUnit.h:1909-1959`)
The doc states the gate as **TWO NAMED TERMS** — (1) the 2D disc on `Center`, (2) `GetDistanceToTarget(self, candidate) <= AggroRadius` from self — records that the line **used to say the opposite**, and tells `TASK-980` in terms that it **routes this existing read** and must **not** re-add the bound. ⭐ It also records **why the class hid**: the stale doc was in a **different file** from the definition that falsified it.

### ⚠️ SITE 7 CARRIED A DEFECT OLDER THAN THIS ROW — DECLARED, NOT LAUNDERED
`"Archer 700"` — `Docs/Data/cards.csv` line 3 has `Range = **2100**`, and has for a long time. ⛔ **Not caused by `TASK-979`.** I removed the three numbers rather than re-typing 2100, so the line now names the **members** and cannot rot again. **Declared so the correction is not read as this row's scope creep.**

## §11(B) — ⚖️ MY RULING ON WARN-1 (THE DEFEND STANCE): ⛔ **DECLINED AS A CODE CHANGE, RESOLVED AS A RECORD** (`SC-§40`)

**Measured myself, every operand read at source — ⛔ not relayed from the report:**

| operand | site | value |
|---|---|---|
| `DefendRadius` | `SummonedUnit.h:1327` | **1281.f** |
| DEFEND disc | `SummonedUnit.cpp:2626` `CastleHalfWidth + DefendRadius` | ≈3656.85 + 1281 = **≈4937.9 uu** (9× castle) |
| callers of `AcquireEnemyNearPoint` | `:1928` DEFEND · `:2059` · `:2097` · `:2101` | **4** |
| castle footprint | `SummonedUnit.cpp:5572` | **≈7313.7 × 7384.5 uu** |

- **Ratio:** `4937.9 / 2000` = **2.469×** ⇒ blind area from the disc centre = `1 − (2000/4937.9)²` = **83.6%**. ⭐ Independently within 0.4% of the grouped lane's 84% at `GroupRadiusMax 5000` — **two different operands, one figure**, which is why I trust it.
- **Opposite faces:** `2 × 3656.85` = **≈7313.7 uu** = **3.66×** the default notice radius.

### ⛔ THE RULING, AND THE HONEST BOUNDARY OF IT
✅ **The regression is REAL and I have written it into the code, into `J-F28`, and onto the `TASK-987` regression watch.** ⛔ **I did NOT change behaviour, and the refusal is reasoned, not lazy:**

1. **Exempting DEFEND breaks the thing item (6b) exists to guarantee.** Jonathan ruled the bound a property of the **engagement radius** — *"due to fog OR ANYTHING"*. `TASK-980` then routes this same read through the fog accessor. An exemption would make **one stance behave differently in clear weather than in fog**, which board item (7c) grades **BLOCKER** in those exact terms.
2. **The real repair is MOVEMENT, and item (6b) cuts ACQUISITION ONLY.** A defender that cannot notice the far face should be **re-pointed at the battered face**; that is a change to the DEFEND goal on a commanded lane, unboarded, and squarely the "silent rebalance riding a fix cycle" this batch has paid for repeatedly.
3. ⭐ **TASK-574's actual repair is INTACT, and this is the part the WARN understates.** `TASK-574`'s defect was that the disc lay **entirely inside the keep** — besiegers at the gate were unreachable **at any range, from any position**. This bound is a **reach** limit: a defender **standing at the battered face still acquires there**, and the fallback (`EnterAdvance(OwnCastle)`) is untouched, so **nothing bricks and no unit idles**. The new failure is strictly narrower: *"a defender does not notice a besieger on a face it is not standing on."*
4. **The fall-back is benign** and the magnitude matches the figure already declared for the grouped lane.

⇒ ⚖️ **This is a WARN I am resolving as a DECLARED CONSEQUENCE, not one I am leaving silent.** ⛔ If 🧑 Jonathan wants defenders to sweep their own walls, that is a **boardable movement row** — `TASK-980`-adjacent, not smuggled into a comment fix.

⚠️ **AND THE FOURTH CALLER THE REPORT DID NOT SIZE:** `:2059` is **not a zone tier** — it is the HOLD **position→attack MONOTONE UPGRADE**. Term (2) gates it too ⇒ **a held position-tier target can no longer be upgraded to an attack-zone enemy beyond this unit's reach.** Recorded at the declaration.

## §11(C) — ✅ THE CORRECTED COORDINATE, AND EVERY OTHER ONE RE-DERIVED

⛔ **The dispatch is right and my figure was wrong.** `protected:` was cited as **1101**; it measured **1111** at QA's instant. ⭐ Root cause is the `SC-§38a` family: I wrote the number from a **located** position, never an **adjudicated** one.

⚠️ **It has since moved AGAIN — to 1129 — because this repair inserted 18 comment lines above it.** ⇒ ⭐ **that is the argument against citing coordinates at all**, so the table below is anchored on **symbols**, which do not rot:

| symbol | file | line (post-repair) |
|---|---|---|
| `public:` / `protected:` / `private:` | `SummonedUnit.h` | **200 / 1129 / 1724** |
| `static constexpr float UnitEngagementRadiusUU = 2000.f;` | `SummonedUnit.h` | **852** |
| `float AggroRadius = UnitEngagementRadiusUU;` | `SummonedUnit.h` | **1258** |
| `float LeashRange = 900.f;` / `float LeashMarginMultiplier = 1.5f;` | `SummonedUnit.h` | **1271 / 1284** |
| `GetEngagementRadiusUU` / `GetEffectiveLeashRangeUU` | `SummonedUnit.h` | **945 / 952** |
| `GetLeashRangeFloorUU` / `GetLeashMarginMultiplier` | `SummonedUnit.h` | **961 / 962** |
| `GetClassDefaultEngagementRadiusUU` (decl / def) | `.h` / `.cpp` | **981 / 4485** |
| `ResolveNoticeRadiusUU` (decl / def) | `.h` / `.cpp` | **882 / 4427** |
| `ResolveEffectiveLeashRangeUU` (decl / def) | `.h` / `.cpp` | **933 / 4456** |
| `AcquireEnemyNearPoint` (decl / def) | `.h` / `.cpp` | **1957 / 2375** |
| the binding | `SummonedUnit.cpp` | **1302** (beside `AttackRange = Row->Range` at **1288**) |
| the two leash drop sites | `SummonedUnit.cpp` | **1734 / 1979** |
| the notice bound (read / cut) | `SummonedUnit.cpp` | **2460 / 2495** |
| `ResolveDefendEngagementRadius` (def / the sum) | `SummonedUnit.cpp` | **2559 / 2626** |

✅ **Counts re-measured AFTER the repair:** `> LeashRange` ⇒ **0**; `> GetEffectiveLeashRangeUU()` ⇒ **2**. **Unchanged — the repair touched no drop site.**

## §11(D) — THE OTHER FINDINGS, DISPOSED INDIVIDUALLY

- **WARN-2 (acquisition vs retention under fog) — ⛔ NOT "fixed", ROUTED.** Written into `ResolveEffectiveLeashRangeUU`'s doc as a **rider on `J-F27`**: the 3000 is a **clear-weather** number; under fog a unit acquires at 609.6 and chases to 3000 (**≈2390 uu on sight it does not have**, ~4.9× vs the pre-diff 1.5×). ⛔ I did **not** clamp retention — it collides head-on with *"commanded units DO NOT LOSE THEIR COMMANDS"* and with the deliberately leash-free grouped lane. ⇒ **needs a ruling; routed to the manager for `TASK-980`'s row.** I agree with QA that a silent fix here would have been the wrong move.
- **WARN-3 — ✅ FIXED.** The sentence claiming the base-CDO read *"tracks a Blueprint override and a bound card row"* is replaced with what is **true**: it is the shipped **class default**; a BP override lives on a *different* class's CDO; a card row is written to the **spawned instance**. ⭐ The claim the row actually needs requires neither.
- **WARN-4 — ✅ CONCEDED.** QA is right: the board is **INCOMPLETE, not inverted**. Item (6d-iii-a) never claims this row creates the 100-uu band; it omits the **1500-uu baseline**. ⛔ **"BACKWARDS" was too strong and I withdraw it.** The arithmetic (1500 → 100, 15×) stands and QA confirmed it independently. ⚠️ Add QA's rider when it goes to 🧑 Jonathan: **the 15× is a CLEAR-WEATHER figure** — under fog both cards sit at 609.6 before and after.
- **WARN-5 (this file is not the board's gate) — ✅ ACKNOWLEDGED, no action mine.** `TASK-985` gates `979`+`980` together; this file is the `979` half.
- **NIT-1 — ✅ FIXED**, §11(C).
- **NIT-2 (`AcquireTarget` reads the raw member) — ⛔ DELIBERATELY NOT TOUCHED.** QA is right that `SiegeAcquisitionFunnelTest.cpp:786` pins that body to contain the token `AggroRadius`, so a "consistency" rewrite would turn a **pre-existing** row RED. ⛔ Left exactly as it is.
- **NIT-3 / NIT-4 — no action.** Both are correct as written; NIT-3 is a build-only question for `TASK-987`.

### ⛔ THE ONE NON-COMMENT LINE IN THIS REPAIR, DECLARED RATHER THAN GLOSSED
`SiegeFogClampTest.cpp:1057` — the `TEXT(...)` **label** in the `SiteRadii` array changed from *"the UNBOUNDED sentinel"* to *"the UNBOUNDED **gather** sentinel (AcquireEnemyNearPoint's **vision query**…)"*. ⛔ **It is a message string, not an operand:** the row's value is still `TNumericLimits<float>::Max()` and `Site.Label` is still `const TCHAR*`. **The assertion is behaviourally byte-identical.** ⇒ I will **not** claim this repair is literally "comment-only"; it is **"comment-only plus one test message label"** — and that distinction is exactly the overstatement WARN-3 caught me making.

## §11(E) — ⭐ WHAT THE REPORT AND THE DISPATCH **BOTH** MISSED

1. ⛔⛔ **`SummonedUnit.h:150` and `SiegeFogClampTest.cpp:1047`** — two more live stale claims (§11(A) sites 7 and 9). **Site 9 contains no number**, so it was unreachable by any value-grep.
2. ⚠️ **The pre-existing `Archer 700`** vs `cards.csv`'s `2100` — a stale claim **older than this row**, found only because the claim-census swept the whole sentence rather than the changed value.
3. ⚠️ **`:2059` is the MONOTONE UPGRADE, not a tier** — the report called all three grouped callers "tiers" and did not state the upgrade consequence.
4. ⚠️ **`SummonedUnit.cpp:5572-5575` — a claim that is STILL TRUE but whose MARGIN this row cut by 98%. ⛔ Declared and deliberately NOT edited.** It says the castle origin *"would never come within Range/AggroRadius of a unit standing at its walls"*, justifying the closest-point metric. Half-width **3656.85** vs notice: at the old 600 the margin was **3056.85 uu**; at 2000 it is **1656.85**; at the Longbowman's **3600** it is **56.85 uu**. ⇒ ⭐ **still true, and ONE `NoticeRange` cell above ≈3657 falsifies it.** ⛔ Not rewritten (it is not false, and `TASK-993` owns the cell that would make it so) — but **`TASK-993`/`TASK-985` must know a 3657+ notice cell retires this comment's reasoning.**
5. ⚠️ **False positives I checked and REFUSED to "fix"** (`SC-§40` — a refused edit is a measurement): `SummonedUnit.h:2320` *"UNREACHABLE for Cavalry (600)"* is a **movement speed**, and `:1354`'s `600.f` is a **projection-extent Z**. ⛔ Same round number, different quantity — the §7(3) class, one level down.

## §11(F) — SUITE COUNT (`TL-§5c`)

- ⭐ **445 DECLARED across 34 files**, re-censused at this instant, **post-repair**. ⛔ **Unchanged by this repair — it added zero `IMPLEMENT_*_AUTOMATION_TEST` macros.**
- ✅ Reconciles exactly with QA's independent census (445 / 34) and with `TASK-985`'s boarded figure.
- ⛔ **NOTHING WAS EXECUTED.** 445 is a **declared** count, not a pass count. ⚠️ `TASK-981` may move it; **`TASK-987` derives the executed number at its own instant.**

## §11(G) — FILES TOUCHED IN THIS REPAIR

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | 4 comment blocks: the class-header `Acquire`/`Reacquire` lines · the ranged-delivery line · `AcquireEnemyNearPoint`'s declaration doc (the BLOCKER) · the `J-F27` fog rider on `ResolveEffectiveLeashRangeUU` |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | 1 comment block: the `J-F28` paragraph, now carrying the DEFEND caller + ≈4937.9 uu + the declined-fix reasoning |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogClampTest.cpp` | WARN-3's justification rewritten · the `UNBOUNDED` label + a 3-line comment (§11(D)) |
| `.claude/pipeline/handoffs/TASK-979-programmer.md` | §7(0) and §7(1) corrected in place · this §11 |

⛔ **NOT touched:** `MinerUnit.cpp` · `SorcererUnit.cpp` · `CardRow.h` · `SiegeFogStatics.{h,cpp}` · `Tests/SiegeFogTest.cpp` · `Tests/SiegeUnitNoticeRangeTest.cpp` · `SiegePlayerController.{h,cpp}` · `cards.csv` · `DT_Cards` · `SiegeCombatStatics.{h,cpp}`.

---

# §12 — RECOVERY BY TRANSCRIPT REPLAY (2026-09-04)

**Verdict: ✅ BYTE-IDENTICAL. The board's pre-positioned criterion (a) is MET.**
`SummonedUnit.{h,cpp}` have been restored to the exact artifact that received the LOOP-2 PASS.
⛔ Nothing was reconstructed, inferred, re-authored, or fuzzy-matched.

## §12(A) — WHAT WAS DONE

The lost work was replayed from the **literal `Edit` tool-call payloads** preserved in two agent
transcripts, applied onto the `HEAD (84eec02)` state in strict chronological order:

| # | source | edits | role |
|---|---|---|---|
| 1 | `agent-ae9d8a8316b2b9045.jsonl` | **13** (2 × `.h`, 7 × `.cpp`, then 3 × `.h`, 1 × `.cpp`) | the ORIGINAL implementation |
| 2 | `agent-a43f7c8aff84d7c1b.jsonl` | **7** (6 × `.h`, 1 × `.cpp`) | the LOOP-1 REPAIR |

- All 20 were `Edit` (⛔ zero `Write`, zero `MultiEdit`) ⇒ no full-file replacement was involved.
- All 20 recorded `is_error: false` at authoring time ⇒ no failed edit was replayed.
- **Every one of the 20 `old_string`s matched EXACTLY ONCE** in the evolving buffer. Not once did a
  match count come back `0` or `≥2`, so no site was ambiguous and no judgement call was made.
- Replay was **all-or-nothing**: nothing reached disk until all 20 matched.
- Line endings: the payloads are 100% LF, the files 100% CRLF (no BOM). Reproduced the Edit tool's
  own normalisation — LF buffer in, CRLF out. Result is 100% CRLF, matching HEAD's convention.

Restored sizes: `SummonedUnit.h` 183,139 B (`sha256 cd17a64f…`) · `SummonedUnit.cpp` 296,560 B
(`sha256 26e8ec9d…`). Diff vs HEAD: **+514 / −33 across the two files.**

## §12(B) — HOW BYTE-IDENTITY WAS PROVEN (⛔ not assumed — `SC-§39`)

Both agents ran verification `grep`s **after their final edit**, and the transcripts preserve those
**line-numbered outputs — the real qa-passed file's own answers.** Re-running the identical commands
against the restored files reproduces them character for character:

1. **Post-REPAIR (= the qa-passed final state).** Two full grep outputs, **28 coordinates**, exact:
   - `SummonedUnit.h` `\b(600|900)\b` → lines 157, 809, 840, 844, 845, 892, 898, 899, 900, 912, 919,
     1231, 1262, 1265, 1267, 1271, 1277, 1279, 1354, 1530, 2320 — all 21 identical, text and number.
   - `SummonedUnit.h:1916` (the `AggroRadius-from-self` corrective) — identical.
   - `SummonedUnit.cpp:1288, 1290, 2559, 2626` and `SummonedUnit.h:1239, 1327` — identical.
   ⭐ A single byte inserted or dropped anywhere above those points would have shifted the numbers.
2. **Post-ORIGINAL state.** The 7 `grep -c` probes and the brace/paren census reproduce exactly:
   `> LeashRange` 0 · `> GetEffectiveLeashRangeUU()` 2 · `Row->NoticeRange` 1 ·
   `GetEngagementRadiusUU()` 1 · `Distance > NoticeRadiusUU` 1 · `ResolveNoticeRadiusUU(` 2 ·
   `GetClassDefaultEngagementRadiusUU()` 2 · `.cpp` braces 0 / parens −3 / `TEXT(` 62 ·
   `.h` braces 0 / parens 0 / `TEXT(` 0.

## §12(C) — PIN VERIFICATION (the surviving untracked test)

`Tests/SiegeUnitNoticeRangeTest.cpp` survived (untracked). Its `CountOccurrencesInCode` and
`ExtractFunctionBody` were ported to Python **verbatim** (comment-line skipping, case-sensitive,
signature → first `\n}`) and every source-text assertion evaluated against the restored files:

| pin | expected | got |
|---|---|---|
| `GetDefault<ASummonedUnit>()` in `.cpp` | 0 | ✅ 0 |
| resolver body `FMath::Min` / `Clamp` | 0 / 0 | ✅ 0 / 0 |
| resolver body `FMath::IsFinite` | > 0 | ✅ 1 |
| `> GetEffectiveLeashRangeUU()` | 2 | ✅ 2 |
| `> LeashRange` | 0 | ✅ 0 |
| `UpdateStateGrouped` body `LeashRange` | 0 | ✅ 0 |
| `LoadStatsAndStart`: `Row->NoticeRange` / `ResolveNoticeRadiusUU(` / `GetClassDefaultEngagementRadiusUU()` | 1 / 1 / 1 | ✅ 1 / 1 / 1 |
| `AcquireEnemyNearPoint`: `GetEngagementRadiusUU()` / `Distance > NoticeRadiusUU` / `DistSquared2D` / `bRangedAttack` | 1 / 1 / >0 / 0 | ✅ 1 / 1 / 1 / 0 |
| header `float AggroRadius = UnitEngagementRadiusUU;` | 1 | ✅ 1 |

Also confirmed directly:
- `static constexpr float UnitEngagementRadiusUU = 2000.f;` at `.h` **h:852, `public:`** — the symbol `TASK-980` consumes.
- `ResolveNoticeRadiusUU` is bound in `LoadStatsAndStart` **beside the shipped `AttackRange = Row->Range`** (`.cpp:1288`).
- The per-class read is `GetClass()` → `GetDefaultObject<ASummonedUnit>()`, written as a **null-checked
  two-step** (`MyClass->GetDefaultObject<ASummonedUnit>()`, `.cpp:4494`), ⛔ not a bare chain — and
  `GetDefault<ASummonedUnit>()` occurs **0** times.
- **⛔ ZERO `FMath::Min`, ZERO `Clamp` in the resolver.** Its shape confirms `2000` is a DEFAULT, not a
  cap: a positive finite row value **returns outright** (the Longbowman's 3600 survives), and the
  `!(ClassDefaultRadiusUU > 0.f)` guard preserves the Miner/Sorcerer zero-seals.
- `GetEffectiveLeashRangeUU()` = `ResolveEffectiveLeashRangeUU(LeashRange, AggroRadius, LeashMarginMultiplier)`
  = `FMath::Max(Leash, Notice × 1.5)` (`LeashMarginMultiplier = 1.5f`, h:1284), read at **both** former
  drop sites (`.cpp:1734`, `.cpp:1979`).
- Access levels: all 7 accessors/statics `public:`; data members `AggroRadius` (h:1258) and
  `LeashRange` (h:1271) `protected:`. **Zero raw member reads remain in `Tests/`.**
- The repair's stale-claim fixes are present: h:157 now reads *"the three numbers this line used to
  carry — 'aggro 600, leash 900, Archer 700' — were ALL stale"*; the struck `TASK-838` clause has
  **0** occurrences.

## §12(D) — WHAT THE TRANSCRIPTS REVEALED THAT THE HANDOFFS DID NOT

1. ⭐ **The destroyed diff was NOT only `TASK-979`'s.** The `git checkout` at **10:00:41** was run by the
   **`TASK-980` Part (A)** agent (`agent-ae2dfad0ea8fb278c`) to undo **its own 4 edits** made at
   09:57:00–09:58:48. Those 4 (a `SiegeFogStatics.h` include + the `FOG-§9.6` fog-band reach ruler,
   built on `979`'s `GetClassDefaultEngagementRadiusUU()`) were **deliberately discarded by their own
   author** and are therefore ⛔ **NOT replayed here.** `TASK-980` re-authors them from scratch.
2. ⚠️ **A transcript-duplication artifact that could have corrupted the replay.** A **third** file,
   `agent-afc69a22514a06dbf.jsonl`, carries the *same* TASK-979 dispatch prompt, the *same* start
   timestamp, and 11 SummonedUnit edits — which reads at first glance like a **parallel double-dispatch
   whose edits interleaved.** It is not: all 11 payloads are **byte-identical with identical
   timestamps** to `ae9d8a83`'s first 11; it is a truncated mirror recording that stops at 08:43:24
   and misses the final two edits (08:50:20, 08:51:49). The same pairing exists for `TASK-777`
   (`a52435d1` / `ab8be00d`). ⛔ **Replaying the mirror instead of `ae9d8a83` would have silently
   dropped two edits and produced a near-miss.**
3. ✅ **No non-`Edit` writes.** Both agents were swept for `Bash`/`PowerShell` writes to the two files:
   every apparent hit is a `>` inside a quoted `grep` pattern (`"> LeashRange"`), plus one `cat >>`
   onto **this handoff**. The 20 Edits are the complete authorship record.
4. ✅ **The repair's third file was never lost.** `Tests/SiegeFogClampTest.cpp` (§11(D)/WARN-3) is still
   `M` in the worktree — the checkout named only the two source files. TASK-979's artifact is
   therefore complete: restored `SummonedUnit.{h,cpp}` + untouched `SiegeFogClampTest.cpp` +
   `CardRow.h`'s `NoticeRange` + this handoff.

## §12(E) — STATE, AND WHAT IS OWED

- **Files written:** `SummonedUnit.h`, `SummonedUnit.cpp` — ⛔ and nothing else. Every other live diff
  (`SiegeCombatStatics.cpp`, `SiegeFogStatics.*`, `SiegeFogTest.cpp`, `cards.csv`, `CardRow.h`,
  `SiegeFogClampTest.cpp`, both untracked test files) is **untouched**; verified via `git status`.
- ⛔ **NOT compiled** (`QUIET-MODULE`). The module should now build again — the restored symbols are
  exactly the ones `SiegeUnitNoticeRangeTest.cpp` (~20 sites) and `SiegeFogClampTest.cpp:1039`/`:1165`
  were failing to resolve — but that is `TASK-987`'s to prove, ⛔ not an assertion of this note.
- ⛔ **No Git state was altered.** Read-only `status`/`diff` only.
- 🙋 **The board row still reads `UNRECOVERABLE` and must be updated by the manager**, who owns
  `TASKBOARD.md` and the ruling. Evidence above satisfies criterion **(a)**.
