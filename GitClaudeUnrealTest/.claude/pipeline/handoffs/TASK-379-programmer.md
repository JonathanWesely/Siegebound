# TASK-379 — [AG-FU1] Two public getters on `ASummonedUnit` + delete the `SiegeCheatManager` reflection block + interpolate the Sorcerer rule line

**Agent:** gameplay-programmer · **Date:** 2026-08-02 · **Status → `ready-for-qa`** (QA: TASK-380)
**Spec:** TASKBOARD `#### TASK-379` · **Authority:** `qa/TASK-365-report.md` "THE SIMPLIFICATION VERDICT"
**No compile, no Git, no editor, no MCP.** Three files touched, all code+comment only.

---

## Files touched

| File | +/− | What |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | +34 / −0 | The two public getters (declaration-only; no `.cpp` change — both are inline) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.cpp` | +14 / −33 = **−19** | Reflection block, constant and include deleted |
| `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` | +45 / −21 | Sorcerer rule line now interpolates both magnitudes |

**`SummonedUnit.cpp` was NOT touched** (both getters are inline in the header) — TASK-396's body work lands on a
clean `.cpp`. **`SiegeCheatManager.h` was NOT touched**: its `SetTestDamageBoost` doc already says *"each unit's
OWN PermanentDamageBonusPerStack"*, which is still exactly true. `SiegePlayerController.*` and `MinerUnit.*`
(TASK-395/397) untouched, as instructed. `CombatantHealthBarComponent.cpp:195`'s comment deliberately left dead
per the spec's explicit "DO NOT".

---

## 1. `SummonedUnit.h` — the two getters (**PUBLIC**, verified)

Inserted immediately after `GetPermanentDamageMultiplier()` (now `:448`) and **before** `protected:`.

```cpp
UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
float GetPermanentDamageBonusPerStack() const { return PermanentDamageBonusPerStack; }   // :467

UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
int32 GetMaxPermanentDamageStacks() const { return MaxPermanentDamageStacks; }           // :482
```

**Access-level proof for QA criterion (a)** — `grep -n "^public:\|^protected:\|^private:"` on the file returns
`121:public:` · **`484:protected:`** · `745:private:`. Both getters sit at `:467` / `:482`, i.e. inside
`121 … 484`. They are **not** near `ShouldHoldDeathAnim()` (the `protected:` trap QA named). Character-for-character
identical to the pinned shape in the SIMPLIFICATION VERDICT; each carries a doc comment in the house style of the
getter above it, flagging both properties as CONVENTIONS §4 balance levers.

The properties themselves stayed `protected:` at `:686` / `:696` — **line numbers shifted by +34** from the
`:652` / `:662` the QA report cites. Anything quoting those two numbers is now stale.

---

## 2. `SiegeCheatManager.cpp` — reflection retired, **−19 lines net**

All four deletions the spec named are done:

- `#include "UObject/UnrealType.h"` (was `:13`) — **removed**, and it was the file's only consumer:
  `grep "FProperty\|FFloatProperty\|CastField\|FindPropertyByName\|StaticStruct\|PropertyValue"` over
  `SiegeCheatManager.{h,cpp}` now returns **zero code hits**.
- `const FName SiegeCheatPerStackPropertyName` + its doc block (was `:54–60`) — **removed**.
- The `CastField<FFloatProperty>` / `FindPropertyByName` resolve **and** its `Error` "refusing to guess" early
  return (was `:500–508`) — **removed**.
- The read is now `const float PerStack = Unit->GetPermanentDamageBonusPerStack();` — **still inside the
  `for (ASummonedUnit* Unit : Targets)` loop, i.e. still PER-INSTANCE**, not a CDO read. A per-Blueprint
  override is honoured exactly as before.

**QA measured −15; the real figure is −19** because I also compressed the 8-line "WHY REFLECTION AND NOT…"
rationale into a 7-line record of the deletion. No extra code was removed.

### ⚠️ Two consequential details QA should confirm rather than assume

1. **A dangling-reference grep will return one hit, and it is prose.** `SiegeCheatManager.cpp:484` now reads
   *"(CastField<FFloatProperty> + FindPropertyByName + a 'refusing to guess' Error path)"* inside the comment
   that records the deletion. That is deliberate house style and matches the precedent QA itself accepted in
   `qa/TASK-365-report.md` §(G) (*"`bMirrorSymmetric` survives only in explanatory comments"*). **No code
   reference survives.**
2. **The `SkippedCount` warning had a THIRD reference to the deleted constant that the spec did not list**
   (was `:570`, `*SiegeCheatPerStackPropertyName.ToString()` feeding a `%s`). I baked the property name into
   the literal format string and dropped the argument. **The emitted string is byte-identical** — `ToString()`
   on that FName produced exactly `PermanentDamageBonusPerStack`. Format-specifier count and argument count
   still match (now `%d` + one arg).

### Behavior-preservation checks (the "must not change" list)

- **The CEIL + `SiegeCheatStackEpsilon` arithmetic is UNTOUCHED — it does not appear in the diff at all.**
  `RawStacks` / `ClampedStacks` / `CeilToInt64`, the `1.e-4` constant and its full sizing rationale are
  byte-identical. `SetTestDamageBoost 101` still yields 21 stacks → 105%, so Jonathan's most important gate row
  stays performable (QA ruling R9).
- **`SetTestDamageBoost` still routes through `ClearPermanentDamageStacks()` then `AddPermanentDamageStacks(N)`
  — no raw field write** (CONVENTIONS §6). That block is untouched.
- Two comments were re-worded because their premise died with the reflection block: the `Percent <= 0 ⇒ clear
  only` note used to justify its position as *"still works even if the mechanic-rule property cannot be
  resolved"* — a failure mode that no longer exists. **The clear-only path itself did not move and did not
  change**; only the reason recorded for its position did.

---

## 3. `DeckBuilderWidget.cpp` — the Sorcerer line now INTERPOLATES

`SorcererGroundBoost` (a `const TCHAR[]`) became **`SorcererGroundBoostFmt`** (a `constexpr TCHAR[]`, matching
the file's existing Fmt-vs-plain naming and storage convention), with two `%s` and two escaped `%%`:

```cpp
constexpr TCHAR SorcererGroundBoostFmt[] = TEXT("While it stands inside an ancient ground, every friendly unit that fights standing in that same ground hits +%s%% harder for each second it spends there. The gain is permanent - kept in full when that unit walks back out, and lost only when it dies - and it stacks up second after second to a hard ceiling of +%s%%. A second sorcerer in the same ground builds it twice as fast. Units that never attack - miners, healers and sorcerers themselves - gain nothing.");
```

Call site (`AppendRuleLines`, the existing `else if (CardID == GlossaryCardID_Sorcerer)` branch):

```cpp
const ASummonedUnit* UnitCDO = GetDefault<ASummonedUnit>();
const float PerStackPercent = 100.f * UnitCDO->GetPermanentDamageBonusPerStack();
const float CeilingPercent = PerStackPercent * static_cast<float>(UnitCDO->GetMaxPermanentDamageStacks());

OutLines.Add(SiegeboundCardGlossary::SorcererRole);
OutLines.Add(FString::Printf(SiegeboundCardGlossary::SorcererGroundBoostFmt,
    *FormatStatValue(PerStackPercent), *FormatStatValue(CeilingPercent)));
```

Plus `#include "Siegebound/SummonedUnit.h"` (complete type required by `GetDefault<ASummonedUnit>()`), and the
file-header GLOSSARY-MIRROR-RULE block's *"ONE DOCUMENTED EXCEPTION (TASK-364)"* paragraph rewritten to record
that the exception is **closed**.

### Why the arithmetic is exact (QA criterion (c) — please re-derive, don't take my word)

I deliberately used **`100.f` FIRST**, matching the operand order of `ASummonedUnit::GetDamageBoostPercent()`,
whose exactness `qa/TASK-365-report.md` "BOUNDARY EXACTNESS" already proved in single precision:
`100.f * 0.05f` → `5.00000007450580596923828125`, which is inside half an ULP of `5.0` (spacing at 5.0 is
`2^-21 ≈ 4.77e-7`) and therefore rounds to **exactly `5.0f`**. Then `5.0f * 80.f` = **exactly `400.0f`**.
`FormatStatValue` takes its `IsNearlyEqual(Value, RoundToFloat(Value))` branch on both and prints via `%d`,
so the panel shows **`5`** and **`400`** — never `5.0`, never `5.000000`, never scientific notation.
`FormatStatValue` also makes a retune to a fractional lever (e.g. `0.075` ⇒ `7.5`) render cleanly rather than
malformed.

**Percent-sign escaping:** each magnitude is `+%s%%` — `%s` consumes the argument, `%%` emits one literal `%`.
Two `%s`, two arguments. Precedent for `%%` in this file: `SpellAllyBuffFmt` (`:135`, *"50%% faster"*).
**Format string is a literal `TEXT(…)` in a named constant, never computed** — the UE 5.8
`TCheckedFormatString` / C7595 trap (TASK-268) does not apply; identical shape to the shipped `UpgradeTailFmt`,
`SuicideFmt`, `SwarmFmt`, `ChainFmt` calls in the same function.

### CDO read here vs INSTANCE read in the cheat manager — deliberate, and they differ on purpose

The deck builder is a **menu screen with no unit in the world**, so the class default is the only available and
the correct authority: it is the value every unit spawns with and the one Jonathan edits when he retunes.
`GetDefault<T>()` on a statically-linked `UCLASS` never returns null, so it is read unguarded (this is the one
new dereference in the task — calling it out explicitly for the null-safety sweep). The **gameplay and cheat**
paths keep reading the **instance**, so a per-Blueprint override still wins there.

---

## THE COMPOSED SORCERER PANEL, VERBATIM — diff this against `handoffs/TASK-364-programmer.md`

`GetCardDescription("Sorcerer")` now returns exactly:

```
Unit · Cost 60 gold · Max 2 per deck

Health: 70
Move speed: 350 units per second

It never attacks - no order will make it strike, and an enemy walking into it is ignored - so it deals no damage of its own. It still takes your unit orders like anything else you play, which is how you walk it onto an ancient ground.
While it stands inside an ancient ground, every friendly unit that fights standing in that same ground hits +5% harder for each second it spends there. The gain is permanent - kept in full when that unit walks back out, and lost only when it dies - and it stacks up second after second to a hard ceiling of +400%. A second sorcerer in the same ground builds it twice as fast. Units that never attack - miners, healers and sorcerers themselves - gain nothing.
```

**The diff against TASK-364's panel is exactly two insertions, both on the second rule line:**

| # | Was | Is |
|---|---|---|
| 1 | `…in that same ground hits harder for each second…` | `…in that same ground hits **+5%** harder for each second…` |
| 2 | `…second after second to a hard ceiling. A second sorcerer…` | `…second after second to a hard ceiling **of +400%**. A second sorcerer…` |

Every other character of the panel — identity line, both stat lines, the whole `SorcererRole` line, the rest of
the boost line, the `\n\n` / `\n` joins, the U+00B7 separator — is **byte-identical**.

### ⚠️ ONE SPEC TENSION I DID NOT RESOLVE SILENTLY — please rule

The board's ACCEPTANCE line says *"the card text renders the identical STRING today"*. **That is not
achievable together with the task's own requirement to interpolate**: today's string contains no numbers at
all, so inserting the derived `+5%` / `+400%` necessarily changes it. I read the two clauses as follows and
implemented accordingly:

- TASK-380's criterion **(d)** says *"no **other** card's description changes by one character"* — implying the
  Sorcerer's does change.
- TASK-379 requires the handoff to quote the panel *"so QA can **diff** it against TASK-364's"* — a diff
  presupposes a difference.
- The SIMPLIFICATION VERDICT states the interpolation *"⇒ '+5%'"* and *"⇒ '+400%'"* explicitly.

So I read "identical STRING" as **"identical everywhere except the two magnitudes the task exists to insert"**,
and I minimised the delta to exactly that: two insertions, zero re-wording. **If QA reads it the other way, the
correct fix is a spec correction, not a code change** — an interpolation that produced today's number-free
string would be a no-op and the task would have no purpose.

### TRUTH LAW (§8) — the two new numbers, verified against code on disk

| New claim | Verified at | ✓ |
|---|---|---|
| `+5%` per second (one sorcerer) | `PermanentDamageBonusPerStack = 0.05f` × one stack per friendly sorcerer per `BoostTickInterval = 1.0f` tick (`AncientGround.cpp:106–109` / `:193/:223`) | ✅ |
| the number is **per second, per sorcerer** | the immediately following, **unchanged** sentence *"A second sorcerer in the same ground builds it twice as fast"* carries the multi-sorcerer case exactly as TASK-364 shipped it — the magnitude attaches to the same clause the qualitative wording occupied | ✅ |
| `+400%` hard ceiling | `MaxPermanentDamageStacks = 80` × 5% = 400%, clamped in `AddPermanentDamageStacks` (`SummonedUnit.cpp:810`) | ✅ |
| "hits harder" is still DAMAGE-only | both compose points (`ComputeOutputDamage:2471`, `ApplyDetonation:2758`); nothing touches HP or speed | ✅ |

Every other clause of both Sorcerer lines is unchanged text and was verified claim-by-claim in
`qa/TASK-365-report.md` §(I) — **that verification still stands, no clause moved.**

---

## Standing pre-compile sweeps (QA criterion (e))

- **Shadowing (C4457/C4458/C4459 = hard errors).** Three new locals total, all in the one `else if` block:
  `UnitCDO`, `PerStackPercent`, `CeilingPercent`. Module-wide grep confirms none of the three names exists
  anywhere else in `DeckBuilderWidget.{h,cpp}`, and none collides with a reflected member of `UUserWidget` /
  `UDeckBuilderWidget`. The `SiegeCheatManager` change **removed** a local (`PerStackProp`) and added none.
  `SummonedUnit.h` added no variables at all.
- **Complete-type includes.** `DeckBuilderWidget.cpp` gained `Siegebound/SummonedUnit.h` — the only new
  dereference in the task. `SiegeCheatManager.cpp` already includes `Siegebound/SummonedUnit.h` (`:18`, still
  present) for the new member call, and lost `UObject/UnrealType.h`, whose only consumers were the three
  deleted lines. No header gained an include.
- **No literal `*/` inside any doc comment** in the three files.
- **Player-facing string literals stay pure ASCII.** `SorcererGroundBoostFmt` contains only ASCII (hyphens,
  `+`, `%`). A `grep -P 'TEXT\("[^"]*[^\x00-\x7F]'` sweep flags only pre-existing `UE_LOG` strings with
  em-dashes (established house style for logs); **my edits introduced no new non-ASCII inside any literal**,
  and the one log line I re-worded kept its pre-existing em-dash unchanged.
- **No new tick, timer, poll or allocation.** All three getters are inline pure reads; the widget work happens
  once per card-details composition, as before.
- **`GetFirstPlayerController()`** — not used, not added.

## What QA should scrutinise hardest

1. **The ACCEPTANCE tension above** — that is the one place I interpreted rather than followed literally.
2. **The third constant reference at the `SkippedCount` warning** that the spec's deletion list omitted, and my
   claim that the emitted string is byte-identical.
3. **The exactness argument** for `5` / `400` — re-derive it; if `100.f` were moved to the right of the
   multiply the values would still be exact at today's tuning, but I chose the order QA already proved.
4. **Whether the two insertion points are the right clauses** for the magnitudes (truth-law placement), given
   the "twice as fast" sentence is what carries the per-sorcerer qualifier.
5. **The unguarded `GetDefault<ASummonedUnit>()`** — the single new dereference.
6. **`SummonedUnit.h` line-number drift (+34)** invalidates the `:652` / `:662` property citations in
   `qa/TASK-365-report.md` and in `DeckBuilderWidget.cpp`'s old comment (that comment is rewritten).

## Downstream

- **TASK-396** (Follow, unit-side) and **TASK-397** — `SummonedUnit.cpp` is untouched and `SummonedUnit.h`
  gained a contiguous 34-line block inside `public:` well away from the state-machine declarations, so the
  merge surface is minimal.
- **TASK-389** (compile + commit): expect a **wide** recompile — `SummonedUnit.h` is included broadly. The
  two additions are new inline `UFUNCTION`s, source-compatible with every existing consumer; no signature
  changed, nothing was removed from any header.
