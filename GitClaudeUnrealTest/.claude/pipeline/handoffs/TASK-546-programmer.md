# TASK-546 — THE GRAMMAR: `inplace` / `zone`, and `Build`'s third defaulted parameter

**Agent:** gameplay-programmer · **Date:** 2026-08-05 · **Status:** ready-for-qa
**QA gate:** `.claude/pipeline/qa/TASK-550.md` (the batch's ONLY gate — names 541..549)
**Law:** CONVENTIONS `AS-§21.4` · `AS-§21.5` · `AS-§21.9` · `AS-§9c` · `AS-§1` · `SC-§15`

📌 **M8: adds no replicated property, no new replicated class, no new relevancy tier.**

⛔ **NOT COMPILED, NOT RUN, NO GIT.** TASK-551 owns the only compile. Every grammar line
pasted below is a **hand-TRANSCRIPTION of the emitter through a scratch script**, not the
output of a build — the same provenance TASK-518's handoff declared, and this file's own
law applies to it: *"string-comparing two generators can never prove either one is valid;
only the target parser can."*

---

## 1. FILES TOUCHED — TWO, BOTH EXCLUSIVELY OWNED BY THIS TASK

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.h` | `Build` gains a **third, trailing, defaulted** parameter; class doc + degenerate-input doc updated so they do not go stale (`SC-§22`) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.cpp` | canonicalises the region list, emits `inplace` + `zone` when it is non-empty, adds `inplace` to `who` |

⛔ **Nothing else was opened for writing.** No parser (545), no snapshot (547), no executor
(548), no tests (549), no `Tests/`, no `Docs/Data/*`, no `Build.cs`, no `Content/`.

**The pinned signature, character-for-character against `AS-§21.9`:**

```cpp
static FString Build(const TArray<FName>& UnitKinds, const TArray<FName>& PlaceNames,
    const TArray<FName>& RegionPlaceNames = TArray<FName>());
```

**Symbols referenced that this task does not own:** `SiegeAssistantJsonKeys::In`
(TASK-545). ✅ **It is already present on disk** — `SiegeAssistantCommand.h:344`,
`inline constexpr const TCHAR* In = TEXT("in");` — so there is no build-order hazard even
though the board still shows TASK-545 as `backlog` (**stale board status, flagged to the
orchestrator, not touched by me**).

**Call sites: unchanged, byte-identical, zero edits.** `SiegeAssistantComponent.cpp:3130`
and the 20-odd test call sites all pass two arguments and keep compiling; they emit **no
`in` alternative**, which is declared behaviour and is documented on the parameter (see §5).

---

## 2. ⭐ THE `who` ALTERNATION ORDER — FOR TASK-547 TO MIRROR CHARACTER-FOR-CHARACTER

> ### `selection | except | inplace | "all" | "none"`

**`inplace` is THIRD: after `except`, before the two bare strings.** The three array/object
shapes stay grouped ahead of the two scalars. This is the order `AS-§21.5` pins and the
order the emitted rule line carries.

**What TASK-547's Zone A `WHO =` line must do:** the shipped line is prose, not GBNF, and
it currently reads

```
WHO    = [{"kind":KIND,"n":COUNT}] with 1 to 3 entries, or {"all_except":[KIND]} with 1 to 3 kinds, or "all", or "none"
```

⇒ **the fifth shape goes AFTER the `{"all_except":…}` clause and BEFORE `"all"`**, i.e. the
prose clause order becomes **array → all_except object → in object → "all" → "none"**.
⚠️ **There is no compile-time link between the two files.** The tie is (a) the comment I
added above the `who` rule, (b) the mirror comment at `SiegeAssistantSnapshot.cpp:806-840`,
and (c) the TASK-550 gate. TASK-549 asserts they agree.

---

## 3. THE GENERATED GBNF — (a) A REPRESENTATIVE THREE-REGION BOARD

Board = the 13 commandable kinds in `DT_Cards` row order, the **seven** places of
`PlaceVocabulary` in table order, and the **three** region-bearing places `AS-§21.4` names,
also in table order (`mid`, `ancient_ground_near`, `ancient_ground_far` — ⚠️ **the ORDER is
the caller's and is preserved verbatim, never sorted**; TASK-547 supplies it).

```gbnf
root ::= command | question
command ::= "{\"intent\":" intent ",\"who\":" who ",\"where\":" where ",\"when\":" when "}"
question ::= "{\"ask\":" ask "}"
ask ::= "\"which_unit\"" | "\"which_place\"" | "\"how_many\"" | "\"which_intent\"" | "\"unsupported\""
intent ::= "\"send\"" | "\"guard\"" | "\"ambush\"" | "\"follow\"" | "\"charge\"" | "\"fallback\"" | "\"rally\""
kind ::= "\"footman\"" | "\"archer\"" | "\"knight\"" | "\"miner\"" | "\"militiamob\"" | "\"pikeman\"" | "\"sapper\"" | "\"cavalry\"" | "\"longbowman\"" | "\"cleric\"" | "\"ogre\"" | "\"wizard\"" | "\"sorcerer\""
where ::= "\"own_castle\"" | "\"enemy_castle\"" | "\"mid\"" | "\"ancient_ground_near\"" | "\"ancient_ground_far\"" | "\"nearest_mine\"" | "\"hero\"" | "\"none\""
count ::= "1" | "2" | ... | "30" | "\"all\""
at-least ::= "1" | "2" | ... | "30"
item ::= "{\"kind\":" kind ",\"n\":" count "}"
selection ::= "[" item "]" | "[" item "," item "]" | "[" item "," item "," item "]"
exceptlist ::= "[" kind "]" | "[" kind "," kind "]" | "[" kind "," kind "," kind "]"
except ::= "{\"all_except\":" exceptlist "}"
inplace ::= "{\"in\":" zone "}"
zone ::= "\"mid\"" | "\"ancient_ground_near\"" | "\"ancient_ground_far\""
who ::= selection | except | inplace | "\"all\"" | "\"none\""
when ::= "\"now\"" | "{\"kind\":" kind ",\"at_least\":" at-least "}"
```

**THE ENTIRE DIFF AGAINST TODAY'S GRAMMAR IS THREE LINES:** two added
(`inplace`, `zone`) and one changed (`who` gains ` inplace |`). `count` and `at-least` are
elided with `...` here exactly as TASK-518's handoff elided them; the emitter is unchanged
and still writes all 30 alternatives.

**Emission position:** the two new rules are written **after `except` and immediately before
`who`**, mirroring how `exceptlist`→`except`→`who` already sit together, so the fifth
shape's machinery reads as one contiguous block. `inplace` references `zone` one line before
`zone` is defined — legal, and already this file's practice: `root ::= command | question` is
the first line emitted and both of its references are defined below it.

**Jonathan's sentence, now expressible:** *"send all units currently in an ancient ground to
attack a castle"* ⇒
`{"intent":"send","who":{"in":"ancient_ground_near"},"where":"enemy_castle","when":"now"}`
— **both `who` and `where` populated, in one command**, which is the case `AS-§21.5` and
TASK-547's rule line exist to teach.

## 3b. THE GENERATED GBNF — (b) AN EMPTY REGION LIST ⇒ ⛔ BYTE-IDENTICAL TO TODAY

```gbnf
root ::= command | question
command ::= "{\"intent\":" intent ",\"who\":" who ",\"where\":" where ",\"when\":" when "}"
question ::= "{\"ask\":" ask "}"
ask ::= "\"which_unit\"" | "\"which_place\"" | "\"how_many\"" | "\"which_intent\"" | "\"unsupported\""
intent ::= "\"send\"" | "\"guard\"" | "\"ambush\"" | "\"follow\"" | "\"charge\"" | "\"fallback\"" | "\"rally\""
kind ::= "\"footman\"" | ... | "\"sorcerer\""
where ::= "\"own_castle\"" | ... | "\"none\""
count ::= "1" | "2" | ... | "30" | "\"all\""
at-least ::= "1" | "2" | ... | "30"
item ::= "{\"kind\":" kind ",\"n\":" count "}"
selection ::= "[" item "]" | "[" item "," item "]" | "[" item "," item "," item "]"
exceptlist ::= "[" kind "]" | "[" kind "," kind "]" | "[" kind "," kind "," kind "]"
except ::= "{\"all_except\":" exceptlist "}"
who ::= selection | except | "\"all\"" | "\"none\""
when ::= "\"now\"" | "{\"kind\":" kind ",\"at_least\":" at-least "}"
```

⛔ **No `inplace`, no `zone`, and `who` is the shipped four-alternative line.** The gating
is a single `if (bHasRegions)` around the emission block and a single `if (bHasRegions)`
around the `who` alternative, so **there is no code path on which an empty region list can
add or remove one byte.** This is what a 2-argument caller gets, and it is what every
existing test fixture gets.

## 3c. THE OTHER TWO DEGENERATE BOARDS (the spec asked for 0 and 1)

- **ONE region, ZERO kinds** — `kind` / `count` / `at-least` / `item` / `selection` /
  `exceptlist` / `except` are all omitted as they are today, and:
  ```gbnf
  inplace ::= "{\"in\":" zone "}"
  zone ::= "\"mid\""
  who ::= inplace | "\"all\"" | "\"none\""
  ```
  ⭐ **This is the case the `bHasKinds` gate would have destroyed.** *"Everyone in the mid"*
  names no unit kind, and a board with nothing spawned is exactly where a player points at
  ground instead of at units. Every rule referenced is still defined; the grammar is
  well-formed.
- **ZERO regions, ZERO kinds** — byte-identical to today's `who ::= "\"all\"" | "\"none\""`
  collapse. Nothing about this task can reach that path.

---

## 4. THE FIVE THINGS THE SPEC SAID DECIDE THIS — ANSWERED IN ORDER

1. **Rule names `[a-z]` only, no underscore.** `inplace` and `zone` are one word each,
   declared in a comment beside the existing `exceptlist` `SC-§15` declaration and citing
   the same `at_least` / TASK-413 evidence. Both names and both references pass
   `IsLegalGbnfRuleName` (the emitter checks them at `AppendRule`, definitions **and**
   references). ✅ The JSON key is `in` — no underscore, so no split was needed; the comment
   says so explicitly **and warns that the coincidence is not permission to unify
   `at_least`/`at-least` or `all_except`/`exceptlist`.**
2. **`zone` is GENERATED.** Same construction as `where` (`:510-525`), same helpers —
   `CanonicalizeSymbols` → `GbnfJsonString` per alternative → `JoinAlternatives` →
   `AppendRule`. ⛔ There is no literal place symbol anywhere in the new code. `Build` gained
   the third trailing default; the parameter doc names the cost of that shape (a new caller
   that forgets it loses a feature quietly instead of failing to compile) and states the
   duty: **any caller holding a snapshot passes `GetRegionPlaceNames()`.**
3. **Emitted only when the list is non-empty**, on the `bHasKinds`-gates-`except`
   reasoning, with the llama.cpp-generates-unconstrained consequence spelled out.
4. **Gated on zones only, NOT `bHasKinds`.** The justification sits directly above the
   `who` alternation as required, and names the asymmetry before a reviewer can read it as a
   bug: `except` is gated on kinds because `exceptlist` **literally references the `kind`
   rule**; `zone` references no kind at all.
5. **The order is stated in §2 above** and pinned in a comment on the rule itself.

---

## 5. ⚠️ DECLARED DEPARTURES AND THINGS QA SHOULD SCRUTINISE

**(D1) The `inplace` right-hand side is written in the emitter's fused-terminal idiom, not
in the four-terminal form `AS-§21.5` prints.**
- Law prints: `inplace ::= "{" "\"in\"" ":" zone "}"`
- I emit: `inplace ::= "{\"in\":" zone "}"`
- **Same accepted language, different terminal segmentation.** ⚖️ **The precedent is exact
  and it is one rule up:** `AS-§20.1` printed `except ::= "{" "\"all_except\"" ":" except_list "}"`
  and what shipped — and passed QA — is `except ::= "{\"all_except\":" exceptlist "}"`, via
  `JsonObjectOpen`. That same law block also printed `except_list`, which shipped as
  `exceptlist`. ⇒ **The law's rule blocks are SHAPE notation, not a byte specification**, and
  the spec's own instruction was *"same construction, same helpers"*. Using `JsonObjectOpen`
  is what makes that true. **If QA rules otherwise it is a one-line change**, but it would
  then also make `inplace` the only rule in the file that does not use the helper.

**(D2) The gate is the POST-canonicalisation count, not `RegionPlaceNames.Num() > 0`.**
The spec's literal wording is the raw parameter. I gate on `Regions.Num() > 0` **after**
`CanonicalizeSymbols`, because a caller passing only `NAME_None` / `""` / `"all"` / `"none"`
hands in a non-empty array that filters to nothing — and gating on the raw count would then
emit `zone ::= ` with an **empty right-hand side**, which is precisely the whole-grammar
rejection the requirement exists to prevent. **This is strictly stronger than the spec and
satisfies it on every input where the two differ.** Flagged rather than done quietly.

**(D3) `zone` has NO `"none"` alternative, unlike `where`.** Deliberate, commented:
`where` is a key that is always emitted and the army-wide verbs have no destination, whereas
`zone` is reachable only from inside `inplace`, which the model chooses to enter. Absence is
already said by the other four `who` shapes, and `{"in":"none"}` is `AS-§21.5`'s `BadRegion`.
⇒ **The three reserved symbols are unsamplable rather than merely refused** — `CanonicalizeSymbols`
drops `all`/`none` before they can become alternatives.

**(D4) ⛔ A COVERAGE HOLE THAT IS TASK-549's TO CLOSE, AND I CANNOT.**
`Siegebound.Assistant.Grammar.RuleNameCharset` (`Tests/SiegeAssistantGrammarTest.cpp:370-376`)
builds **four** grammars and its own comment claims *"every shape the builder can produce,
because a rule that is only emitted on one branch is exactly the rule that escapes review."*
⚠️ **All four use the 2-argument overload, so as of this task that claim is FALSE — `inplace`
and `zone` are invisible to the one test that guards the charset.** ⛔ **TASK-549 must add
region-bearing shapes to that array** (at minimum: kinds+regions, no-kinds+regions,
kinds+no-regions). I am forbidden to touch `Tests/`; this is the handoff carrying it.

**(D5) Doc comments were updated, not left to rot.** The header claimed *"Only `kind` and
`where` are generated"* and *"with no unit kinds `who` collapses to `"all" | "none"`"* — both
would have become false. Fixed in place (`SC-§22`). ⛔ No behaviour rides on those edits.

**(D6) `Grammar.Reserve(2048)` untouched.** It is a hint, not a bound; the two new lines are
~120 chars at three regions. Not my call to retune at a gate.

---

## 6. WHAT I DID NOT DO

⛔ No compile, no build, no editor, no MCP, no Git, no test authored or edited.
⛔ No change to `kind`, `where`, `count`, `at-least`, `item`, `selection`, `exceptlist`,
`except`, the `ask` alternatives, `GrammarCountMin` / `GrammarCountMax`, or the emission
order of any shipped rule.
⛔ No fourth top-level JSON key. ⛔ No new `ask` symbol. ⛔ No accuracy claim of any kind —
`AS-§12f`: nobody has seen what the model emits for this shape, and `AS-§21.11` #5 already
records the fifth-shape teaching risk honestly.
