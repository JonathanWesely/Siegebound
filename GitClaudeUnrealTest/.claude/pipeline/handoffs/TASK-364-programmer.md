# TASK-364 — Sorcerer deck-builder description (CardID-keyed glossary rule line)

**Agent:** gameplay-programmer · **Status:** ready-for-qa · **Date:** 2026-08-01
**No compile, no Git, no editor, no MCP** — as specced.

## File touched (ONE)

`Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` — **+49 lines, no deletions, no logic changes.**
`DeckBuilderWidget.h` is **UNTOUCHED**: no new function is needed (`AppendRuleLines` already exists and is
already declared); the `names:` block lists the pair as the ownership scope, not as a required edit.

Nothing else in the repo was touched. `SummonedUnit.{h,cpp}` (TASK-360), `HealthBarProvider.h` /
health-bar files (TASK-362) and `BattlefieldScatter` (TASK-358) were **read only**.

## The composed Sorcerer panel, VERBATIM (QA: diff this against the code)

`GetCardDescription("Sorcerer")` now returns exactly:

```
Unit · Cost 60 gold · Max 2 per deck

Health: 70
Move speed: 350 units per second

It never attacks - no order will make it strike, and an enemy walking into it is ignored - so it deals no damage of its own. It still takes your unit orders like anything else you play, which is how you walk it onto an ancient ground.
While it stands inside an ancient ground, every friendly unit that fights standing in that same ground hits harder for each second it spends there. The gain is permanent - kept in full when that unit walks back out, and lost only when it dies - and it stacks up second after second to a hard ceiling. A second sorcerer in the same ground builds it twice as fast. Units that never attack - miners, healers and sorcerers themselves - gain nothing.
```

Separator is the CONVENTIONS middle dot (U+00B7) emitted by the existing `IdentitySeparator()`, never a typed glyph.
Blocks are joined `\n\n`, the two rule lines `\n` — the existing `GetCardDescription` join, unchanged.

> ⚠️ **The board's spec quotes the "before" panel as `… / Move speed: 350`.** The real shipped stat line is
> **`Move speed: 350 units per second`** (`AppendStatLines`, the `Speed > 0` branch). The spec's quote was
> abbreviated; the panel above is the real composition. Not a defect in either place — flagged so QA does not
> chase a phantom mismatch.

## What was added

1. **`SiegeboundCardGlossary::SorcererRole`** (`const TCHAR[]`) — the never-attacks / still-commandable clause.
2. **`SiegeboundCardGlossary::SorcererGroundBoost`** (`const TCHAR[]`) — the ancient-ground boost rule.
3. **`GlossaryCardID_Sorcerer(TEXT("Sorcerer"))`** in the anonymous namespace, beside its six neighbours.
4. **One `else if (CardID == GlossaryCardID_Sorcerer)` branch** in `AppendRuleLines`, inserted in the existing
   role chain immediately after the `Masons` branch.

## TRUTH-LAW VERIFICATION — claim by claim, against code that is NOW ON DISK

TASK-359/360 landed while this task was in flight, so **every clause below is verified against the shipped
implementation, not against the spec.** File:line for each:

| Clause in the player-facing text | Verified at |
|---|---|
| "It never attacks" | `SorcererUnit.h/.cpp` `CanEverAttack() → false`; three guards: `SummonedUnit.cpp:2050` (`EnterAttack`), `:1583` (`UpdateStateGrouped`), `:2307` (`PerformAttack`) |
| "no order will make it strike" | `SummonedUnit.cpp:1583` — the grouped path forces `CurrentTarget = nullptr` |
| "an enemy walking into it is ignored" | `SorcererUnit.cpp` ctor `AggroRadius = 0.f` ⇒ `AcquireTarget` can never return a candidate |
| "it deals no damage of its own" | `cards.csv` Sorcerer `Damage 0` + the seal above |
| "It still takes your unit orders like anything else you play" | `Profile Standard` in `cards.csv` ⇒ `IsGroupCommandEligible()`; `SorcererUnit.cpp` ctor deliberately **leaves `StateCheckInterval` at 0.25 s** "because it is commandable" |
| "While it stands inside an ancient ground" | `AncientGround.cpp:184` `IsPointInZone` gate on the empowerer count |
| "every friendly unit … in that same ground" | `AncientGround.cpp:223` — `Grant = SorcererCount[TeamBucketIndex(Unit->GetTeamId())]`, i.e. **own team only**; occupants collected in-zone at `:184`, per-instance zone ⇒ same ground by construction |
| "**that fights**" (the eligibility qualifier) | `AncientGround.cpp:197` `CanReceiveDamageBoost()` → `SummonedUnit.cpp:793-795` (`CanEverAttack && AttackDamage > 0 && Profile != Support`) |
| "hits harder" (damage only — not HP, not speed) | `ComputeOutputDamage` + `ApplyDetonation` compose points (TASK-360) |
| "for each second it spends there" | `AncientGround.cpp:108` — 1 Hz looping timer at `BoostTickInterval` |
| "kept in full when that unit walks back out" | **No removal path exists** — nothing in `AncientGround.cpp` ever subtracts or clears; the only clear is on death |
| "lost only when it dies" | `SummonedUnit.cpp:2802` — `ClearPermanentDamageStacks()` in the death choke |
| "stacks up second after second to a hard ceiling" | `AddPermanentDamageStacks` clamps to `MaxPermanentDamageStacks` |
| "A second sorcerer … builds it twice as fast" | `AncientGround.cpp:193/223` — one stack per friendly sorcerer per tick |
| "miners, healers and **sorcerers themselves** gain nothing" | `AncientGround.cpp:189-195` (empowerers `continue` before eligibility — never self-boost) **and** `SummonedUnit.cpp:785-789`, whose own comment names exactly the Sorcerer / Miner / Cleric |

**No clause states anything the code does not do.** The one deliberately *narrowed* claim is "every friendly unit
**that fights**" — the dispatch brief said "every friendly unit", but `CanReceiveDamageBoost` excludes miners,
Support healers and other sorcerers, so the unqualified wording would have been false. The final sentence names
the exclusions explicitly rather than leaving the qualifier to do the work alone.

## ⚠️ THE ONE JUDGEMENT CALL — magnitudes are QUALITATIVE, and this is now PROVEN, not assumed

CONVENTIONS §8 ("Ancient Grounds + Sorcerer"): *magnitudes that exist as UPROPERTY mechanic rules are
**interpolated from those properties or stated qualitatively** — never a hardcoded number that can drift.*

Interpolation was the first choice and **is not available**: TASK-360 landed
`PermanentDamageBonusPerStack` at **`SummonedUnit.h:652`** and `MaxPermanentDamageStacks` at **`:662`** — both
inside the **`protected:`** block that opens at **`:450`**. A `GetDefault<ASummonedUnit>()->…` read from this
widget **would not compile**. (Predicted before TASK-360 landed, from the shipped pattern that *every*
EditAnywhere tunable on that class — `AggroRadius`, `DefendRadius`, `StateCheckInterval` — is protected; then
confirmed on disk.)

So the string carries **no number at all** — "hits harder … for each second", "stacks up … to a hard ceiling",
"twice as fast". The real values (`0.05` ⇒ +5%/s per sorcerer, `80` ⇒ the +400% ceiling) are recorded in the
declaration's comment with the exact re-interpolation instruction for the day they become public.

**Sibling precedent for the same call:** `CombatantHealthBarComponent.cpp:195` (TASK-362, this batch) also needed
the cap and also put it in a **comment** rather than coupling to the member.

**QA — the two ways to disagree, and what each would cost:**
- *"Bake the numbers with a `// mirrors` comment, like the other 15 clauses."* That is the file's older
  GLOSSARY-MIRROR RULE. I did not follow it here because the 2026-08-01 §8 clause is **newer and specific to this
  card**, and the task spec restates it verbatim. If QA rules the mirror rule wins, the fix is one string edit.
- *"Interpolate anyway."* Not possible without an access-level change to `SummonedUnit.h`, which is **TASK-360's
  file and explicitly off-limits to me this batch.** If QA wants the numbers live, the correct route is a
  follow-up task making those two properties public (or adding public getters), then a one-line edit here.

## Deliberate deviations from the letter of the `names:` block — both justified

1. **A second glossary symbol, `SorcererGroundBoost`.** The block pins `SorcererRole` + `GlossaryCardID_Sorcerer`;
   both exist, spelled exactly as pinned. The extra constant follows the file's shipped multi-clause-role shape
   (`StructureRole` + `TowerRole`; `UpgradeSharpenedBlade` + `UpgradeTailFmt`) — the seal is a permanent property
   of the unit, the boost is conditional on where it stands, and one run-on sentence would bury the second.
2. **No `Fmt` suffix.** The file's convention is `…Fmt` **only** for `Printf` format strings. Both new constants
   are plain `const TCHAR[]` because the magnitudes are qualitative — so `SorcererRole` keeps its pinned name
   *and* obeys the naming convention, with no conflict.

## Two comment-only edits QA will see in the diff (zero effect on any description)

- The glossary header said *"all **28** descriptions"*; `cards.csv` now has **30** rows (it was already stale by
  one before this batch — the Wizard). Rewritten to *"EVERY card description"* so it can never go stale again.
- The GLOSSARY-MIRROR RULE paragraph now records the one documented exception above, because as written it
  asserted a rule these two new constants deliberately do not follow.

## Blast radius — why no other card can have changed

- The new branch is an **`else if` inside the existing role chain**, after `Miner`/`DeepMine`/`Masons` and before
  the `HeroUpgrade`/`Building` branches. It is reachable **only** for `CardID == "Sorcerer"`; the chain's existing
  order and conditions are byte-identical.
- No existing string, format, helper, stat branch, keyword branch, spell branch, profile branch or scaling branch
  was edited.
- For the Sorcerer row itself, **nothing else in `AppendRuleLines` fires**: all keyword columns are default,
  `SpellEffect None`, `Profile Standard`, `bRanged false`, `Damage 0` ⇒ no keyword, spell, delivery, profile or
  castle-scaling line. Its rules block is exactly the two new lines.
- `AppendStatLines` needed **no change**: the melee branch requires `Row.Damage > 0` *inside* a `Row.Range > 0`
  block and both are 0, so it never emitted a melee lie — confirming the manager's finding.

## Scrutiny list for QA

1. Read the two strings against the table above and reject any clause you cannot land on a file:line.
2. Confirm the ASCII-only-string-literal law: both new literals use `-` and plain ASCII throughout. The non-ASCII
   characters (⚠️ ⇒ —) appear **only in comments**, which is established house style in this module
   (`SummonedUnit.cpp` alone has 306 such lines; the file has no BOM and compiles today).
3. Confirm the `else if` placement cannot shadow or be shadowed by another CardID branch.
4. Rule on the qualitative-magnitude call above — that is the single decision worth a second opinion.
5. If a headless `GetCardDescription` dump over all 30 CardIDs is run at TASK-366/377, the 29 non-Sorcerer strings
   must be **byte-identical** to the pre-change build.
