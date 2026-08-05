# TASK-518 — [AX-3] EXCLUSION SCHEMA — struct · grammar · parser

**Agent:** gameplay-programmer · **Date:** 2026-08-04 · **Status:** ready-for-qa
**Names QA gate:** **TASK-525** (`.claude/pipeline/qa/TASK-525.md`)
**Law:** CONVENTIONS `AS-§20.1` (pinned registry) · `AS-§1` · `AS-§2` · `AS-§3` · `SC-§15`

## M8 DECLARATION DUTY

> **adds no replicated property, no new replicated class, no new relevancy tier.**

Reason: `ExcludeKinds` is a `TArray<FName>`, so `FSiegeAssistantCommand` stays
`uint8` / `int32` / `FName`-only and `AS-§3`'s *"M8 P2 takes `FSiegeAssistantCommand`
as-is over the wire"* property survives unchanged.

## Files touched (all four are TASK-518's exclusive ownership this wave)

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantCommand.h` | the constant, the sixth field, the JSON key, the two reason codes, the validator signature, doc corrections |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantCommand.cpp` | the `who` object branch, cross-field check 2, the exclusion invariants |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.cpp` | `exceptlist` + `except` rules, `who` gains the `except` alternative |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.h` | degenerate-input contract updated to name the two new rules |

⛔ **NOT touched:** `SiegeAssistantSnapshot.*` (TASK-517/521) · `SiegeAssistantComponent.*`
(TASK-520/522) · `SiegeAssistantConsoleWidget.*` (TASK-519) · `Tests/**` (TASK-523) ·
`Docs/Data/*.csv` (TASK-524) · `Plugins/SiegeLlama/**` (D4) · `SiegePlayerController.*` ·
`HeroCharacter.*`. **No compile, no Git, no editor/MCP/PIE.** No asset is referenced by
this task — it is pure C++ with no `Content/` surface.

## 1. THE PINNED SYMBOLS — registry version vs. as written

| `AS-§20.1` registry | as written in the source | verdict |
|---|---|---|
| `static constexpr int32 SiegeAssistantMaxExclusionKinds = 3;` | `static constexpr int32 SiegeAssistantMaxExclusionKinds = 3;` | ✅ character-for-character (`SiegeAssistantCommand.h:111`) |
| `TArray<FName> ExcludeKinds;` under `UPROPERTY()` | `TArray<FName> ExcludeKinds;` under `UPROPERTY()` | ✅ character-for-character, **sixth** field, after the five shipped ones in shipped order (`:209`) |
| `inline constexpr const TCHAR* AllExcept = TEXT("all_except");` | `inline constexpr const TCHAR* AllExcept = TEXT("all_except");` | ✅ character-for-character, inside the existing `SiegeAssistantJsonKeys` (`:264`) |
| `inline constexpr const TCHAR* ExcludeArity    = TEXT("exclude_arity");` | `inline constexpr const TCHAR* ExcludeArity = TEXT("exclude_arity");` | ✅ symbol + value identical. **Only difference: the registry's column-alignment padding before `=`.** Each code line now carries its own doc block, so aligning across them is impossible; nothing but whitespace differs (`:358`) |
| `inline constexpr const TCHAR* ExcludeConflict = TEXT("exclude_conflict");` | `inline constexpr const TCHAR* ExcludeConflict = TEXT("exclude_conflict");` | ✅ character-for-character (`:373`) |
| `who ::= selection | except | "all" | "none"` | `who ::= selection | except | "\"all\"" | "\"none\""` | ✅ same rule, same alternative order. The extra escaping is the shipped `GbnfJsonString` emitter making a JSON string terminal — identical to how `"all"`/`"none"` already emitted before this task |
| `SiegeAssistantValidateSelection` KEEPS ITS NAME | name unchanged; **signature gained a trailing defaulted parameter** | ⚠️ see §5 — the pin anticipated a signature change ("keeps its NAME"), but the *shape* was forced by file ownership. **Flagged for TASK-525.** |

## 2. THE NEW GBNF — representative 13-kind board

Board = the 13 commandable kinds in `DT_Cards` row order (the `SpikeRoster` order,
Sorcerer last), places = the five arena places.

```gbnf
root ::= command | question
command ::= "{\"intent\":" intent ",\"who\":" who ",\"where\":" where ",\"when\":" when "}"
question ::= "{\"ask\":" ask "}"
ask ::= "\"which_unit\"" | "\"which_place\"" | "\"how_many\"" | "\"which_intent\"" | "\"unsupported\""
intent ::= "\"send\"" | "\"guard\"" | "\"ambush\"" | "\"follow\"" | "\"charge\"" | "\"fallback\"" | "\"rally\""
kind ::= "\"footman\"" | "\"archer\"" | "\"knight\"" | "\"miner\"" | "\"militiamob\"" | "\"pikeman\"" | "\"sapper\"" | "\"cavalry\"" | "\"longbowman\"" | "\"cleric\"" | "\"ogre\"" | "\"wizard\"" | "\"sorcerer\""
where ::= "\"enemy_castle\"" | "\"own_castle\"" | "\"mid\"" | "\"ancient_ground_near\"" | "\"ancient_ground_far\"" | "\"none\""
count ::= "1" | "2" | ... | "30" | "\"all\""
at-least ::= "1" | "2" | ... | "30"
item ::= "{\"kind\":" kind ",\"n\":" count "}"
selection ::= "[" item "]" | "[" item "," item "]" | "[" item "," item "," item "]"
exceptlist ::= "[" kind "]" | "[" kind "," kind "]" | "[" kind "," kind "," kind "]"
except ::= "{\"all_except\":" exceptlist "}"
who ::= selection | except | "\"all\"" | "\"none\""
when ::= "\"now\"" | "{\"kind\":" kind ",\"at_least\":" at-least "}"
```

⚠️ **PROVENANCE, STATED BECAUSE THIS FILE'S OWN LAW DEMANDS IT: the block above was
produced by hand-TRANSCRIBING the emitter into a scratch script, NOT by running
`USiegeAssistantGrammar::Build`.** TASK-526 owns the only compile, so no build of this
code exists yet. Per this file's standing warning — *"string-comparing two generators
can never prove either one is valid; only the target parser can"* — **treat this as the
intended output, not as a measured one.** The real gates are TASK-523's automation
tests and `Siege.Llama.SpikeGrammar`. What the transcription *did* check mechanically:
every rule reference resolves to a defined rule; every rule name and reference matches
`[a-zA-Z0-9-]`; the grammar contains no `*`, `+` or `?`.

**Degenerate boards:**
- **0 kinds** — `kind`/`count`/`at-least`/`item`/`selection`/`exceptlist`/`except` are all
  omitted; `who ::= "\"all\"" | "\"none\""` (**2 alternatives, unchanged from today**);
  `when ::= "\"now\""`. An empty roster has nothing to exclude for exactly the reason it
  has nothing to select.
- **1 kind** — all rules emit. `exceptlist`'s 2- and 3-kind alternatives can only produce
  the same symbol twice, which the parser refuses with `DuplicateKind`. **That is
  degradation to REFUSABLE, not to UNREACHABLE, and it is deliberate:** `selection` has
  had the identical property since it shipped, and bounding `exceptlist` by the live kind
  *count* rather than by the cap constant would make two sibling rules disagree about
  their own construction for a case the parser already answers. Documented at the `who`
  rule in source.

## 3. EVERY PARSER REFUSAL PATH, AND WHAT TRIGGERS IT

New paths (all in the `who` = **Object** branch unless stated):

| # | reason code | payload | trigger |
|---|---|---|---|
| 1 | `unknown_key` | the key | a key other than `all_except` inside the `who` object, e.g. `{"all_except":[…],"n":2}` |
| 2 | `missing_key` | `all_except` | `"who":{}` — an object with no `all_except` |
| 3 | `bad_type` | `who` | `who` is an object that fails `AsObject()` |
| 4 | `bad_type` | `all_except` | `all_except` is present but not an array, e.g. `{"all_except":"miner"}` |
| 5 | `exclude_arity` | the length | `{"all_except":[]}` (0) or more than `SiegeAssistantMaxExclusionKinds` (4+). **Empty is REFUSED, not promoted to plain "all"** — "everyone except —" and stopping is how an exception gets dropped unnoticed |
| 6 | `bad_type` | `kind` | a non-string element in the array (via `ParseKindSymbol`) |
| 7 | `bad_kind` | the value | an empty element, or the reserved `"none"` (via `ParseKindSymbol`) |
| 8 | `duplicate_kind` | the kind | the same symbol twice in `ExcludeKinds`, e.g. `["miner","miner"]`. Caught at the final `SiegeAssistantValidateSelection` gate — **one uniqueness path serves both lists** |
| 9 | **`exclude_conflict`** | **the intent symbol** | ⭐ **`!SiegeAssistantIntentTakesSelection(Intent)` — i.e. `charge` / `fallback` / `rally`.** e.g. `exclude_conflict:fallback` |
| 10 | `exclude_conflict` | `none` | `who` = `"none"` **and** `ExcludeKinds` non-empty. **⚠️ Unreachable from JSON today** — see §6 |
| 11 | `exclude_conflict` | `<kinds>/<excludes>` | a positive selection **and** an exclusion both non-empty. **⚠️ Unreachable from JSON today** (the `who` shapes are disjoint); live on the **M8 P2 wire path**, which is exactly why the pin puts it in the validator |
| 12 | `exclude_arity` | the length | validator-side upper-bound re-check for a struct that did not come from this parser (wire path). **Upper bound only** — an empty `ExcludeKinds` is the normal state of every command that excludes nothing |
| 13 | `bad_kind` | the index | validator-side `NAME_None` element (wire path) |

**Refusal #9 is the ruling.** It is read off the executor, not chosen for safety:
`Charge`/`Fallback` execute via `ASiegePlayerController::ApplyArmyWideStance(...)` — the
same shipped API the `T`/`E` keys call — and `Rally` via `AHeroCharacter::Rally()`. None
walks the candidate list, so an `ExcludeKinds` handed to them has **no code path that
could subtract anything**. Accepting it would make *"fall back except the miners"*
execute as *"fall back INCLUDING the miners"* — a valid-shaped wrong command that looks
obeyed. The gate reuses `SiegeAssistantIntentTakesSelection()` itself so there is no
second list of army-wide verbs to drift.

**UNCHANGED, verified by reading:** `bad_who` still fires for a `who` string that is
neither `"all"` nor `"none"`; `who_arity` still fires for a `who` **array** outside
1..`SiegeAssistantMaxSelectionKinds`; `who_required` still fires for `"none"` + a
selection-bearing intent. The new branch is a fourth `else if` on `Who->Type`, so **no
existing input reaches new code and no existing failure shape moved.**

## 4. ⚖️ THE RULING YOU ASKED FOR — "AN EXCLUSION THAT LISTS EVERY LIVE KIND"

> **It is a LEGAL PARSE. The parser does not refuse it. The EXECUTOR refuses the order.**

Two independent reasons, and the second is decisive:

1. **`AS-§20.1` already ruled it** ("EMPTY-AFTER-EXCLUSION"): the executor refuses the
   whole order through the **existing unsupported-ask outcome**, carrying the arithmetic
   in the log, exactly as the shipped shortfall path does. ⛔ No new `ask` symbol. ⛔
   Never a silent no-op.
2. **`ParseSiegeAssistantCommand` structurally CANNOT answer it.** It is a pure function
   with no world, no roster and no snapshot, and it is never handed the live kind list —
   it does not know what "every live kind" is. Making this `ExcludeConflict` would
   require giving the parser the roster, which breaks `AS-§1`'s division of labour
   ("grammar guarantees existence, executor guarantees legality").

Made **explicit rather than emergent**, as the spec demanded: it is written into
`ExcludeKinds`' field comment in the header, and named in the parser's
"every other cross-field question is the executor's" list. **⇒ TASK-522 owns the
runtime refusal. If 522 does not implement it, an all-kinds exclusion is a silent
no-op, which is an automatic QA fail — please check that specifically at TASK-525.**

## 5. ⚠️⚠️ THE ONE COMPROMISE — `SiegeAssistantValidateSelection`'s SIGNATURE

**As written:**
```cpp
bool SiegeAssistantValidateSelection(const TArray<FName>& Kinds, const TArray<int32>& Counts,
    FString& OutError, const TArray<FName>& ExcludeKinds = TArray<FName>());
```

**Why the parameter is trailing and defaulted rather than sitting beside the two arrays
it belongs with:** the function has callers in **two files TASK-518 does not own** —

- `SiegeAssistantComponent.cpp:930` (TASK-520 / TASK-522's file), and
- `Tests/SiegeAssistantGrammarTest.cpp` × **6** (TASK-523's file).

A **required** fourth parameter would have broken the module compile in files this task
is explicitly forbidden to edit, and TASK-526 is the single compile. A trailing default
is the only shape that adds the pinned invariants without reaching into another task's
file. The default is declared **only** in the header (never re-declared in the `.cpp`).

⛔ **THE COST, STATED NOT HIDDEN: a caller that omits the argument validates NOTHING
about exclusion.** Two consequences that are now cross-task obligations:

- ⭐ **TASK-522 MUST change `SiegeAssistantComponent.cpp:930` to pass
  `Command.ExcludeKinds` as the fourth argument.** It compiles fine without it and
  silently checks nothing — that is precisely the shape this project treats as a
  guardrail reporting SAFE.
- **TASK-523** should cover the 4-argument form; the 6 existing 3-argument calls still
  compile and still assert exactly what they asserted before.

**If TASK-525 would rather have the honest 4-required-parameter signature, say so and
name who is authorised to touch the other two files** — I can write it either way, but I
cannot edit those files under this task's scope.

## 6. DELIBERATE NO-OPS AND DECLARED DEPARTURES (a silent one would be a gap)

1. **`SC-§15` DECLARED DEPARTURE — the rule is `exceptlist`, not `except_list`.**
   `AS-§20.1` spells `except_list` but anticipated this and named `exceptlist` as the
   substitute. The departure is forced: llama.cpp reads a rule name as `[a-zA-Z0-9-]`
   and stops at the underscore, so `except_list` would parse as the name `except` and
   **the whole grammar would be rejected, running generation UNCONSTRAINED** — the exact
   defect `at_least` shipped with, which cost TASK-413 two of its six bars. **I did not
   invent a third name** (`except-list` would have been house-consistent kebab-case, and
   the pin forbids picking it silently — flagged here for TASK-525 if it prefers it).
   The JSON key keeps its underscore (`all_except`) because that is wire format.
2. **`except`'s object opening is emitted as ONE terminal, not three.** The pin writes
   `"{" "\"all_except\"" ":"`; the code calls the shipped `JsonObjectOpen()` helper,
   producing `"{\"all_except\":"`. **The generated language is identical**, and this is
   how `command` / `question` / `item` / `when` have always emitted an object opening.
   Using the helper avoids a second construction path.
3. **`Grammar.Reserve(2048)` left unchanged.** The two new rules add ~110 chars; a
   13-kind grammar transcribes to ~1523 chars, still inside the hint. Reserve is an
   allocation hint only, so this is a deliberate no-op, not an oversight.
4. **Refusal #10 (`who:"none"` + exclusion) is UNREACHABLE from JSON today and is
   labelled as such in the source rather than presented as a live guard.** The `who`
   shapes are disjoint: `bWhoIsNone` is set only in the String branch, `ExcludeKinds`
   filled only in the Object branch. It is written because the law names "none" as one
   of the three `ExcludeConflict` cases and because a fifth `who` shape added later
   could make it reachable. Same for refusal #11 — unreachable from JSON, **live on the
   M8 P2 wire**, which is the pin's stated reason for the validator re-check.

## 7. ⛔ A SPEC CLAIM THAT IS FALSE AT THE ARTIFACT

> TASK-518 spec **(3)**: *"⚠️ The struct's own class comment says **"five fields"** — update it."*

**There is no such comment.** `grep -rni "five field|5 field|fields\b"` over
`SiegeAssistantCommand.{h,cpp}` and `SiegeAssistantComponent.h` returns **nothing**; the
only "five" in the header is `"The `ask` code is not one of the five"`, which is about
ask codes. Reported rather than quietly skipped, because a later reader hunting that
sentence will not find it either. **The instruction's intent was honoured:** the struct's
doc comment now states it has **six** fields and describes the sixth's role and its
2026-08-04 provenance.

## 8. ⛔ A SHIPPED TEST THIS CHANGE BREAKS — TASK-523 MUST RE-BASE IT

> **`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGrammarTest.cpp`, test
> `Siegebound.Assistant.Grammar.SelectionCap`, assertion
> `"`who` offers selection | all | none"` — expects `SplitAlternatives(WhoRule).Num() == 3`.**

**It is now 4.** ⛔ **I did not edit it — TASK-523 owns that file.** The exact numbers so
523 can re-base it honestly rather than re-copying from the code it is testing:

| assertion | before | after | why |
|---|---|---|---|
| `who` alternatives, non-empty roster | 3 | **4** | `except` added |
| `who` alternatives, **empty** roster (`:727`) | 2 | **2 — UNCHANGED** ✅ | `except` is gated on `bHasKinds` |
| grammar contains no `*` / `+` / `?` (`:671-673`) | pass | **pass — UNCHANGED** ✅ | `exceptlist` is a bounded alternation, no repetition operators |
| `who` references `selection` exactly once (`:678`) | pass | **pass — UNCHANGED** ✅ | |
| empty roster omits `kind`/`item`/`selection`/`count`/`at-least` (`:711-715`) | pass | **pass** ✅ | 523 may want to add `exceptlist` / `except` to that list |

The verdict is a *number moving because we deliberately changed the thing it counts* —
not a defect. ⚠️ **Do not "update the test to pass" without reading why it moved.**

## 9. WHAT QA SHOULD SCRUTINISE (TASK-525)

1. ⭐ **§5 — the defaulted validator parameter.** The one place the pin's shape was bent
   by file ownership. It is a real (bounded, documented) silent-skip hazard and it needs
   a ruling plus the cross-task obligation on TASK-522 confirmed.
2. ⭐ **§4 — that "exclude every kind" is a legal parse is a RULING, not a gap**, and that
   TASK-522 actually implements the runtime refusal through the existing unsupported-ask
   outcome. A silent no-op here is an automatic fail.
3. **Refusal #9 covers every route into `charge`/`fallback`/`rally`.** The gate is the
   shipped predicate, so verify against `SiegeAssistantIntentTakesSelection`'s own
   switch — `Send`/`Guard`/`Ambush`/`Follow` true, everything else (including `None`)
   false. `Intent` cannot be `None` at that point (the parser fails earlier with
   `unknown_intent`), so the check cannot mis-fire on a default-constructed command.
4. **Additivity.** Confirm by reading that no existing `who` shape, key set or failure
   code changed, and that the top-level key set is still exactly
   `{intent, who, where, when}`.
5. **§8's re-base numbers** reach TASK-523 intact.
6. **§1's alignment-whitespace footnote** on `ExcludeArity` — the only textual difference
   from the registry anywhere in this task.
