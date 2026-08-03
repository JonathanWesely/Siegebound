# QA Report — Post-TASK-413 fix pass (spans TASK-410 · TASK-416 · TASK-417)

**Date:** 2026-08-03 · **Reviewer:** qa-reviewer · **Gate in:** `handoffs/TASK-413-buildmaster.md` PART 2 (why)
+ `handoffs/TASK-410-programmer.md` §"Post-TASK-413 fix pass" (what).

> **⏩ STATUS SUPERSEDED 2026-08-03 — SEE `## Re-gate — BLOCKER-1` AT THE END OF THIS FILE.**
> **The current verdict is PASS (0 BLOCKERS).** The `FAIL` immediately below is the preserved, verbatim
> record of **gate 1** and is left untouched on purpose. Do not read it as the live status.

> **FILE CHOICE, DECLARED:** the dispatch offered appending to `qa/TASK-412.md` as `## Post-TASK-413 fix pass`
> **or** a separate file. **I took the separate file.** I have no Edit tool, and the only way to "append" with
> Write is to re-emit all ~960 lines of `qa/TASK-412.md` from memory — which risks corrupting the authoritative
> record of the spike gate to save one file. `qa/TASK-412.md` is **untouched and remains the record for
> TASK-410's original gate + fix loop 1**; this file is the record for the post-TASK-413 pass. All three board
> entries should point here.

---

# Verdict: **FAIL** — 1 BLOCKER · 4 WARN · 4 NIT

### ⚠️ FIRST, FOR THE BUILD-MASTER RUNNING BARS #2 AND #5 RIGHT NOW

> # **NOTHING IN THIS REPORT INVALIDATES YOUR NUMBERS. DO NOT STOP THE RE-RUN.**
>
> The one blocker is a **false claim in three comments and one handoff bullet** about UE's Shipping build
> configuration. It changes **no emitted byte, no grammar, no prompt, no constant and no measurement**. The
> code as it stands is correct and is the code I want compiled.
>
> **Bars #2 and #5 measured against this tree are valid and I will accept them.** So is bar #3's 77.1 %
> reproduction. See §6 for the mechanical checks that prove nothing moved, and §7a for why the re-run
> **cannot** truncate.
>
> **The FAIL is on the fix pass, not on the run.** The fix is ~4 lines of comment.

---

## 0. What I actually did

Raw `Read` on every changed file and every changed region — **never `Grep` for anything structural**, per the
standing law and for a reason this pass proved twice (§8, NOTE-1). Files reviewed:

- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.h` — full
- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.cpp` — full
- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.h` — 78-330, 440-485
- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.cpp` — 525-615, 745-950
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGrammarTest.cpp` — 1-410, 550-730
- `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp` — 155-185, 350-380, 685-1006, 4190-4250
- **Engine source, read from disk, not recalled:** `Misc/Build.h:195-353`, `Logging/LogMacros.h:184-214`
- **Project config, read from disk:** `Source/GitClaudeUnrealTest.Target.cs`
- `Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/VERSION.md`

---

## 1. ⛔ BLOCKER-1 — "not compiled out of Shipping" is FALSE for this project, and it is asserted three times

**Where:**
- `SiegeAssistantGrammar.cpp:216-219` — *"it is **DELIBERATELY NOT COMPILED OUT OF SHIPPING**. A charset guard
  that only fires in a developer build is the same silent failure wearing a different hat."*
- `SiegeAssistantGrammar.cpp:229-231` — *"the Error log carries it everywhere, **including Shipping**, where
  ensures compile out."*
- `handoffs/TASK-410-programmer.md` §4 — *"**Not compiled out of Shipping.** `UE_LOG(..., Error, ...)`
  always"*

**Why it is false — read out of the engine and the project's own target, not from memory:**

| fact | source, on disk |
|---|---|
| `USE_LOGGING_IN_SHIPPING` defaults to **0** | `Engine/.../Misc/Build.h:200-203` — *"If not specified, disable logging in shipping"* |
| Shipping ⇒ `NO_LOGGING = !USE_LOGGING_IN_SHIPPING` ⇒ **1** | `Build.h:350-352` |
| `NO_LOGGING` ⇒ `UE_LOG` **"will only log Fatal errors"** — `Error` expands to an empty `if constexpr(false)` block | `Logging/LogMacros.h:184-194` |
| `USE_CHECKS_IN_SHIPPING` **0**, `USE_ENSURES_IN_SHIPPING = USE_CHECKS_IN_SHIPPING` ⇒ **0** | `Build.h:205-212` |
| Shipping ⇒ `DO_ENSURE = USE_ENSURES_IN_SHIPPING` ⇒ **0** ⇒ `ensureAlwaysMsgf` degrades to `(!!(expr))` | `Build.h:332-334` |
| **`GitClaudeUnrealTest.Target.cs` sets NO override** — no `bUseLoggingInShipping`, no `bUseChecksInShipping` | `Source/GitClaudeUnrealTest.Target.cs` (15 lines, read in full) |

⇒ **In a Shipping build of this project, BOTH halves of the validator vanish.** The same is true of `Test`
(`Build.h:322-324` gives `Test` the identical `NO_LOGGING = !USE_LOGGING_IN_SHIPPING`). The guard is live in
**Editor, Development and DebugGame only** — which is *precisely* the "only fires in a developer build" the
comment declares it has avoided. The stated rationale is exactly inverted by the artifact.

**Why this is a BLOCKER and not a WARN.** I considered WARN, and there is a real precedent for it —
`qa/TASK-412.md` graded WARN-R1 (a stale comment asserting an overturned expectation) as a WARN. This is
different on two counts, and both matter:

1. **It was an explicit acceptance criterion of this gate** ("the validator … is **not compiled out of
   Shipping**"). Grading it WARN and passing would enter a criterion into the record as verified-true when it
   is verified-false. That is the false pass this role exists to prevent.
2. **It is the same failure class the entire pass exists to eliminate.** The charset was verified against the
   consuming system (llama.cpp, empirically — §2). The Shipping behaviour was asserted **from memory about a
   different consuming system (UBT/Core) and never checked**, and it is wrong. A pass whose thesis is *"an
   assertion about a target system must be checked against the target system"* may not itself ship an
   unchecked assertion about a target system.

**⛔ THE FIX IS TO THE COMMENT, NOT THE CODE. DO NOT CHANGE THE CODE.** `UE_LOG(Error)` +
`ensureAlwaysMsgf` is the correct, idiomatic pair here and I endorse it. Required:

- Correct all three sites to state what is true: *"live in Editor / Development / DebugGame; **compiled out in
  Shipping and Test** (`NO_LOGGING`, `DO_ENSURE`), because this project's target sets no
  `bUseLoggingInShipping` / `bUseChecksInShipping` override."*
- **Do NOT flip those target flags to make the comment true.** Turning on Shipping logging project-wide is a
  project-level decision with performance and log-volume consequences far outside this batch, it is not
  `SiegeAssistantGrammar.cpp`'s to take, and no Shipping build of this project exists. Record it as an option
  for TASK-423 and move on.
- Same correction in `handoffs/TASK-410-programmer.md` §4.

**⚠️ Knock-on, recorded but NOT a second blocker:** `BuildZoneC`'s truncation `UE_LOG(..., Warning, ...)` —
the log the §8 cap ruling is *conditional on* — disappears in Shipping and Test for the identical reason. I am
**not** blocking on that: it is the project-wide norm for every `UE_LOG` in this codebase, holding this one
pass to a standard nothing else meets would be a false fail, and the ruling was made in a
measurement/Development context. The condition is satisfied for **every build configuration this project
actually produces** (§5). But the fixed comment must not repeat the Shipping claim about the truncation log
either.

---

## 2. ✅ The rename — and the charset, verified against the parser rather than the description

### 2a. `at_least` → `at-least` as a RULE NAME, both generators. CORRECT.

| lane | rule definition | rule reference | JSON key emitted |
|---|---|---|---|
| **`SiegeAssistantGrammar.cpp`** (the shipped authority) | `:512` `AppendRule(Grammar, TEXT("at-least"), …)` | `:596` `Parts.Add(TEXT("at-least"))` | `:595` `JsonNextKey(SiegeAssistantJsonKeys::AtLeast)` |
| **`SiegeLlamaSpike.cpp`** (the mirror) | `:942` `AppendRule(Grammar, TEXT("at-least"), …)` | `:998` `Parts.Add(TEXT("at-least"))` | `:997` `JsonNextKey(TEXT("at_least"))` |

Emitted `when` in both lanes:

```gbnf
when ::= "\"now\"" | "{\"kind\":" kind ",\"at_least\":" at-least "}"
                                        ^ JSON key      ^ rule name
```

### 2b. ⚠️ THE CHARSET — I could NOT read it from the vendored source, and I am not pretending otherwise

The dispatch asked me to read llama.cpp's rule-name grammar out of the vendored source. **That source is not
in this repo.** `Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/VERSION.md` §"The C API only" and the directory
listing confirm the vendored drop is **7 headers + 3 import libs + 19 DLLs** — official upstream binaries,
tag `b10235`, commit `221f0f63…`. There is no `llama-grammar.cpp`, no `grammar-parser.cpp`, and `llama.h`
does not document the charset. **A source read was not available to me and I did not fake one.**

**What I have instead is stronger than a source read, and it is already in the record.** Build-master ran the
**exact vendored build** (`llama-completion.exe`, same b10235 release) against both grammars
(`handoffs/TASK-413-buildmaster.md` §13a / §13b):

| input | the real parser's answer |
|---|---|
| as-built (`at_least` as rule name) | `parse: error parsing grammar: expecting ::= at _least ::= "1" \| "2" \| ...` |
| renamed (`at-least`) | **parses clean, no diagnostics** |

That pair establishes **both edges that the validator turns on**, empirically, against the binary that ships:
`_` is **not** a rule-name character (the parser stopped at it and named the remainder), and `-` **is** (the
same grammar parsed once the underscore became a hyphen). The remaining members of `[a-zA-Z0-9-]` — lowercase
letters — are exercised by the other twelve rule names in the same grammar, which parsed in the same run.

⚠️ **The one edge NOT verified is digits/uppercase**, which `IsLegalGbnfRuleName` admits and the test asserts
(`item2`). **This is harmless and I am not making it a finding**: all 13 shipped rule names are lowercase
letters plus one hyphen, so even if llama.cpp's charset were *narrower* than `[a-zA-Z0-9-]`, no shipped rule
name would be affected. The over-permissive direction can only fail to reject a hypothetical future name.

⇒ **The charset matches the target parser on every edge that this grammar can reach. RULED CORRECT.**

### 2c. ✅ THE JSON KEY IS BYTE-FOR-BYTE UNTOUCHED — no corpus breach, and I did not open the holdout

The "consistency fix dressed as a tidy-up" the dispatch warned about **did not happen**. Verified in four
places:

| what | evidence |
|---|---|
| the pinned constant | `SiegeAssistantCommand.h:204` — `inline constexpr const TCHAR* AtLeast = TEXT("at_least");` **unchanged** |
| Zone A, game lane | `SiegeAssistantSnapshot.cpp:587` — `Out += TEXT("WHEN   = \"now\", or {\"kind\":KIND,\"at_least\":1 to 30}\n");` — **raw read**, snake_case |
| Zone A, spike lane | `SiegeLlamaSpike.cpp:239` — the identical line, snake_case |
| **both corpus CSVs** | see below |

**🔒 THE SEALED HOLDOUT WAS NOT OPENED.** I used a **negative-existence check only**: a repo-wide search for
the kebab string `at-least` returns **9 files, and not one of them is a `.csv`**
(`Docs/Data/assistant_eval_dev.csv`, `assistant_eval_holdout.csv` and `cards.csv` all absent). A separate
scoped check over `Docs/Data/` for `at.least` in either spelling returns **0 occurrences across 0 files**.
**I learned only that a string is absent; I read no row, no field and no content from either corpus file, and
the holdout's one-shot remains UNSPENT.** That is the exact breach test, at zero information cost.

⇒ **No corpus breach. No Zone A byte moved. The wire format is intact.**

The two `at-least` occurrences inside `SiegeAssistantSnapshot.cpp` (`:557`, `:565`) are **comment text only** —
confirmed by raw read of 525-615. See NOTE-1 for why that needed a raw read.

---

## 3. ✅ THE SWEEP — RE-DERIVED, NOT ACCEPTED. Both halves hold.

### 3a. `at_least` was the only violation — confirmed by enumeration

I enumerated every `AppendRule` call site in both generators independently of the handoff.

**`SiegeAssistantGrammar.cpp` — 13 sites:** `:373` root · `:391` command · `:401` question · `:416` ask ·
`:434` intent · `:449` kind · `:466` where · `:483` count · `:512` at-least · `:527` item · `:559` selection ·
`:575` who · `:602` when.

**`SiegeLlamaSpike.cpp` — 13 sites:** `:839` `:852` `:860` `:876` `:893` `:904` `:915` `:925` `:942` `:952`
`:976` `:984` `:1002` — same names, same order.

**Every one of the 26 names is `[a-zA-Z0-9-]`.** References, hand-collected from the emitted right-hand sides:
`command`, `question`, `intent`, `who`, `where`, `when`, `ask`, `kind`, `count`, `item`×6, `selection`,
`kind`, `at-least`. **All legal, all defined.** The two generators remain true mirrors.

### 3b. ⚠️ THE LOAD-BEARING HALF — no rule name is generated or derived. CONFIRMED.

This is the claim that makes a hard-failing validator safe on a live board, and the dispatch was right to
demand it be re-derived. **It holds, and I checked it by reading `Build` end-to-end rather than by trusting
the sweep:**

- **All 26 rule names are compile-time `TEXT("…")` literals.** Not one `AppendRule` call takes a variable, a
  concatenation, or an `FString`.
- **Every path by which data reaches the grammar ends inside a quoted terminal:**
  - unit kinds → `CanonicalizeSymbols` → `GbnfJsonString(Kind)` (`:446`)
  - place symbols → `CanonicalizeSymbols` → `GbnfJsonString(Place)` (`:462`)
  - intents → `SiegeAssistantIntentSymbols` → `GbnfJsonString` (`:431`)
  - ask codes → `SiegeAssistantAskCodes` → `GbnfJsonString` (`:413`)
  - counts → `GbnfTerminal(FString::FromInt(Quantity))` (`:479`, `:509`)
  - the sentinels `all` / `none` / `now` → `GbnfJsonString` (`:464`, `:481`, `:572-573`, `:584`)
  - JSON keys → `JsonObjectOpen` / `JsonNextKey`, both of which wrap in `GbnfTerminal`
- **`CanonicalizeSymbols` cannot escape into identifier position.** Its output is only ever consumed by
  `GbnfJsonString`. Even its documented odd-character path (`:336-345`, a symbol outside `[a-z0-9_]`) emits
  **escaped, inside the terminal**.

⇒ **A live unit called `militia_mob`, or one with a space or an accent in its name, can never produce an
illegal rule name.** The validator is a source-only check on a fixed set of 13 literals, and **cannot fire on
live roster data.** ✅ **The crash-risk-on-a-live-board concern is closed.**

### 3c. ✅ And the reference scanner does not fire on the JSON key — hand-traced

This was the real hazard: `CollectRuleReferences` runs over `when`'s RHS, and that RHS **contains the literal
text `at_least` inside a terminal**. A scanner that mis-tracked quote state would report the *wire-format JSON
key* as an illegal reference and log an `Error` on **every single `Build` call in shipped play**. Hand-trace of
`SiegeAssistantGrammar.cpp:152-207` against `when`'s emitted RHS:

```
"\"now\""   → enter terminal at ", \ escapes the ", exits at the final "   → no identifier
 |          → separator
"{\"kind\":" → terminal, skipped
kind        → identifier, pushed at the following space
",\"at_least\":"  → TERMINAL. The underscore is INSIDE it and is skipped.   ← the hazard, and it is clean
at-least    → identifier, pushed
"}"         → terminal
⇒ References = { "kind", "at-least" }.  Both legal.
```

The backslash escape is honoured **only inside a terminal** and consumes exactly one following character, so
`\"` cannot close a terminal early and `"foo bar"` is skipped as one unit rather than torn on the space —
which is what the programmer claimed and what I verified. The spike's copy (`:742-793`) is
character-identical in behaviour. **No false positive is reachable from any input this generator can produce.**

---

## 4. ✅ `MaxSnapshotChars` 1440 → 1085 — all four recording conditions met

`SiegeAssistantSnapshot.h:225` — `static constexpr int32 MaxSnapshotChars = 1085;`

| condition | met? | where |
|---|---|---|
| the derivation recorded beside it | ✅ | `:169` — `1085 = 400 tokens × 2.71 chars/token`, with **both operands named as measurements** (`:171-174`) |
| the date recorded | ✅ | `:164` — "CORRECTED FROM MEASUREMENT ON 2026-08-03" |
| TASK-413 named | ✅ | `:164-166` — "BY TASK-413's SPIKE RUN … §8's RESOLUTION trigger armed; the trigger has now FIRED" |
| the 2.13 stricter bound noted | ✅ | `:185-192` — named, quantified (`400 × 2.13 ≈ 850`), **and the reason it is not used today is given** (Zone B is 68 chars of dense key/value symbols; applying its ratio to the whole region over-tightens by ~22 %), with the trigger for adopting it |
| TASK-423's supersession noted **but not implemented** | ✅ | `:194-197` — "TASK-423 SUPERSEDES THIS CONSTANT AND THE WHOLE QUESTION … **Do not implement that here.**" I confirmed **no tokenizer call exists anywhere in `SiegeAssistantSnapshot.{h,cpp}`** |

Also correct and worth recording: `:176-183` states plainly that 1440 was wrong **in the unsafe direction**
(3.6 was optimistic on all four measured readings), that 1440 admitted ~531 tok against a 400-tok budget
(33 % over), and that **the breach was latent, not active** (955 chars / 352 tok on the live board). The zone
table at `:93-96` now carries TASK-413's measured 1139 / 32 / 320 tok. No stale `1440` literal survives
anywhere in code — the four remaining occurrences in the header and the two in the spike are all inside the
comment that explains the correction.

---

## 5. ✅ WARN-5 CLOSED — the truncation genuinely fires at the cap, and it ships with the constant

**This is the binding condition on the §8 ruling and it is the one I gated hardest after BLOCKER-1.**

**The old bug, confirmed from the record:** the condition was
`KindsToPrint < FMath::Min(UnitKinds.Num(), MaxRosterKinds)` — **false at exactly the cap**, so a 13-kind board
collapsing five kinds logged nothing, on every sentence, forever.

**The new condition, `SiegeAssistantSnapshot.cpp:892-893`:**

```cpp
const int32 CollapsedKinds = UnitKinds.Num() - KindsToPrint;
if (CollapsedKinds > 0)
```

**Traced at the exact case that used to be silent** — 13 kinds alive, `MaxRosterKinds = 8`, roster inside
budget:

- `CapKinds = FMath::Min(13, 8) = 8` → loop exits immediately → `KindsToPrint = 8`
- `CollapsedKinds = 13 − 8 = 5 > 0` ⇒ **FIRES.** ✅ The exactly-at-the-cap case now logs.

**The cause is named, and correctly** (`:899-902`): `bBudgetBite = KindsToPrint < CapKinds` distinguishes
*"the CHARACTER BUDGET, below the MaxRosterKinds cap"* from *"the MaxRosterKinds cap"*. In the trace above
`8 < 8` is false ⇒ it reports the **cap**, which is the truth. When the budget loop has bitten, `KindsToPrint`
is strictly below `CapKinds` ⇒ it reports the **budget**. Correct in both directions.

**The escalating latch is correct** (`:913-916`), traced:

| state | `KindsToPrint < Printed` | `Collapsed > Collapsed` | logs? |
|---|---|---|---|
| first fire (init `MAX_int32` / `0`) | 8 < MAX_int32 ✅ | 5 > 0 ✅ | **yes** → latches 8 / 5 |
| steady state, same board | 8 < 8 ✗ | 5 > 5 ✗ | no — the spam the latch exists to prevent |
| deeper collapse (budget bites to 7) | 7 < 8 ✅ | — | **yes** |
| wider board (15 kinds ⇒ 7 collapsed) | — | 7 > 5 ✅ | **yes** |

⇒ **A new, deeper collapse can never hide behind an earlier, milder one** — which a plain bool made
impossible. The `Verbose` per-turn line (`:908-911`) makes a specific degraded answer reconstructable, which
the escalating `Warning` structurally cannot. Both messages carry kinds printed / total / collapsed, roster
chars, budget, `MaxSnapshotChars` and `ZoneBCharReserve`, and the `Warning` says *"do NOT raise it to hide
this."*

**Do the cap and the log ship together?** ✅ Yes — both are in this one pass, in the same file pair, in the
same handoff. The ruling's condition is **not** breached. (`ResetSnapshot()` correctly does not clear the
latches: they are log-spam state, not snapshot state. `mutable` is required and correct — the builders are
`const` by the §9 signature pin.)

**Caveat, already stated in BLOCKER-1 and not double-counted:** in Shipping/Test the `Warning` compiles out
with everything else. The condition holds for every configuration this project builds.

---

## 6. ✅ NOTHING ELSE MOVED — and here is the mechanical proof, for build-master to confirm at runtime

I cannot run `git diff`. So instead of asserting "unchanged", here are the **printed invariants** that make it
checkable from the re-run's own log. **All of these must reproduce; any one that does not means something moved.**

```
SPIKE_TOKENS fixture=t0 zoneA_chars=4314 zoneB_chars=68 zoneC_chars=887 zoneBC_chars=955
             MaxSnapshotChars=1085 headroom=130            <-- ONLY these last two may differ from part 2
SPIKE_TOKENS fixture=t0 zoneB_start_tok~=1147 zoneC_start_tok~=1179
SPIKE_PREFILL ... reused=1156 landed_in=ZONE_B ... DROP=77.1%
             ⇒ reused − zoneB_start = 1156 − 1147 = 9      <-- QA's 8-10 band, unchanged
```

Reasoning that supports it, verified in the artifacts:

- **The spike's Zone A is untouched.** `SiegeLlamaSpike.cpp:239` still emits the snake_case
  `"at_least"` schema line. Zone A is where the zone boundaries come from, so **4314 → 1147/1179 → bar #3
  is undisturbed.** The only spike edits are the constant (`:174`), the validator (`:691-832`), the two rename
  sites (`:942`, `:998`) and comments — **none of which is inside Zone A, Zone B or Zone C.**
- **The game lane cannot reach the spike at all.** The plugin is architecturally forbidden to include the game
  header; `SiegeAssistantSnapshot.{h,cpp}` is not compiled into, referenced by, or readable from
  `SiegeLlamaSpike.cpp`. **`MaxSnapshotChars` moving cannot touch a single measured number.**
- **The t0 tripwire survives** (`:4213-4218`): the spike still hard-checks Zone B = 68 and Zone C = 887 for the
  default order line and warns loudly if the fixture drifted. `VerifyFixtureKindParity` (`:4221`) still runs.
- **Corpus, holdout, few-shots, JSON schema, scoring rule, fixtures, zone boundaries: unchanged** — see §2c
  for the corpus/holdout evidence and §4 for the schema.

⇒ **Bar #3's 77.1 % must reproduce, and I expect it to.**

---

## 7. THE TWO FLAGGED ITEMS — ruled

### 7a. ⚖️ THE NEW COUPLING — ruled, and the programmer's arithmetic re-derived independently

**First, the arithmetic. I re-derived it from the emitted format strings rather than accepting it**
(`AppendRosterBlock` `:751-798`; roster line = `"- %s: %d total, %d orderable, %d followable\n"` = `36 + len(kind)
+ digits`; collapsed line = `"other_kinds: %d kinds, %d units\n"`):

| case | Zone C | budget (`1085 − 192`) | margin |
|---|---|---|---|
| spike fixture, **13 kinds printed** | **887** (my derivation: 9 + 99 + 8 + **595** roster + 18 + 55 + 12 + 14 + 8 + 69 order = 887 ✅ reproduces the measured figure exactly) | 893 | **6 chars** |
| shipped, **`MaxRosterKinds = 8`** | ≈ **670** (8 roster lines = 366; `other_kinds: 5 kinds, 9 units` = 30) | 893 | **≈ 223** |

⇒ **Both of the programmer's figures are CONFIRMED by independent derivation** (their "~220" vs my 223;
their "six characters" exactly). **The coupling is real and correctly described.**

**(a) Can the re-run truncate? — NO. Definitively.**

I traced every use of the spike's mirror constant. `SpikeMaxSnapshotChars` appears **three times in the whole
file**: the declaration (`:174`) and twice on **one `UE_LOG` display line** (`:4207-4209`). It feeds **no
comparison, no loop, no branch, no abort and no truncation path.** The spike has no roster-collapse code at
all — it prints all 13 fixture kinds unconditionally by design (the declared §5e deviation).

⇒ **The re-run measures the full, untruncated 13-kind prompt, byte-identical to part 2's.** The only
observable change is cosmetic: `MaxSnapshotChars=1085 headroom=130` in place of `1440 / 485`.
⇒ **BAR #5 IS NOT MEASURED ON A TRUNCATED PROMPT. Bar #2 is unaffected. Neither number is at risk.**
⇒ ⚠️ **Build-master: if any parsing script pins `1440` or `485`, that is the one line that changed.**

**(b) The `MaxRosterKinds` route to closing WARN-5 is CLOSED. Confirmed.**

At `MaxRosterKinds = 13` a full board yields Zone C = 887 against 893 — **six characters, measured on the
spike's own 61-character order line.** Any busier sentence truncates immediately. So raising the cap would not
close the seam; it would convert a *deterministic policy* collapse into a *budget-driven* one that fires on
almost every real sentence. **The programmer's read is correct and I endorse it.**

**⚠️ AND THE PROGRAMMER'S ANALYSIS MISSES THE DIMENSION THAT ACTUALLY BITES FIRST — see WARN-1.**

**Is `ZoneBCharReserve` the honest lever? — YES, I agree. Is it urgent? — NO, and it must NOT be touched now.**

*Agreed it is the lever*, and for a reason worth stating precisely: it is the **only** adjustment that buys
Zone C room **without raising the token admission ceiling**. `MaxSnapshotChars` caps B+C at 1085; the reserve
merely partitions that. Dropping the reserve to a measured value moves room from an unused reservation to the
roster while B+C stays inside 1085 — so it cannot restore the 33 % over-admission. Raising `MaxSnapshotChars`
would; raising `MaxRosterKinds` doesn't help (it is the thing being cut). The over-charge is real: Zone B
measures 68 (t0) / 71 (t1), and a bounded worst case for four fixed keys is ~85 chars, against a 192 reserve.

*Not urgent, and do not change it in this pass*, for three reasons:

1. **It cannot move any bar.** The spike does not use it (§7a-a).
2. **Changing a §10 tunable, unmeasured, mid-gate is precisely the "guess twice" the cap ruling forbids.** The
   right shape is to set it *from* Zone B's printed worst case, which `Siege.Llama.SpikePrompt` already
   measures — not to eyeball 96 or 128 today. The header itself says so at `:222-223`
   ("re-measure it, do not eyeball it").
3. **TASK-423 retires the whole question** by enforcing the budget with the tokenizer.

⇒ **Ruling: correct lever, correctly left alone. It belongs in TASK-423's spec, with WARN-1's dimension
attached to it.** The programmer's decision not to touch it was right and I am endorsing it, not tolerating it.

### 7b. ✅ THE HANDOFF DOES NOT OVERSTATE THE PARSE. Endorsed without reservation.

I went looking for an overstatement and there is none. Every place it could have crept in, it is explicitly
disclaimed instead:

- `handoffs/TASK-410-programmer.md` §7.1 — *"THE THING I CANNOT PROVE AND HAVE NOT CLAIMED: I did not parse the
  fixed grammar with llama.cpp … Do not let my validator, my test, or a rule-for-rule diff against the spike
  be read as proof the grammar loads — that is precisely the error that produced this task. The only proof is
  a non-NULL `llama_sampler_init_grammar` on the re-run."*
- §4 "What it does NOT guarantee" — *"it does not parse the grammar. It checks the one property that this
  failure turned on."*
- `SiegeAssistantGrammar.cpp:94-98` and `SiegeAssistantGrammarTest.cpp:44-51` both carry the same limit in
  the code itself, where the next reader will actually hit it: *"STRING-COMPARING TWO GENERATORS CAN NEVER
  PROVE EITHER ONE IS VALID. ONLY THE TARGET PARSER CAN. A dump is not a parse … the real proof stays a live
  `llama_sampler_init_grammar` call."*
- §2a takes the correct line on the prior reviews — that they were **correct and blind**, a method failure and
  not a diligence failure. **I agree, including about my own.** `qa/TASK-412.md`'s rule-for-rule diff was
  right about what it asserted and could not have caught this.

⇒ **Conduct under the new §9c law is exactly right. The parse is build-master's, at run time, and a non-NULL
sampler is the proof. Nothing in the handoff claims otherwise.**

---

## 8. ✅ THE TEST SUITE — the third blind spot, gated hardest

### 8a. The scanner now breaks identifiers where llama.cpp breaks them

`SiegeAssistantGrammarTest.cpp:238` — `if (FChar::IsAlnum(Char) || Char == TEXT('-'))`. The `_` is gone.

### 8b. ⚠️ WOULD IT FAIL ON THE PRE-FIX INPUT? — YES, on both halves, and I traced both

**Half 1, the definition.** Pre-fix line `at_least ::= "1" | "2" | …`:
`IsGrammarWellFormed` splits at `" ::= "` (`:181`), takes `RuleName = "at_least"` (`:188`), and calls
`IsLegalGbnfRuleName` (`:191`) → the `_` fails the explicit ASCII range at `:138-142` → returns false with
`"illegal rule name (llama.cpp accepts [a-zA-Z0-9-] only): at_least"`. **FAILS.** ✅
The new `RuleNameCharset` test also asserts this per-line and independently (`:357-360`).

**Half 2, the reference.** Pre-fix `when` RHS containing the bare `at_least` outside a terminal:
the scanner accumulates `at`, hits `_` (not alnum, not `-`), pushes `at` and resets; accumulates `least`,
pushes it. Neither is in `Defined` → `:276-277` fires. **FAILS.** ✅ And the error string is genuinely
well-designed — `undefined rule referenced: at (if this looks like half an identifier, the reference contains
a character outside [a-zA-Z0-9-] — most likely an underscore — and llama.cpp split it exactly here)`. That is
the difference between a 2 a.m. diagnosis and a wasted night.

**The guard is exercised, so the test is a gate and not decoration** (`:391-396`): `at-least`, `root`, `item2`
accepted; `at_least`, `at least`, empty rejected. `TestFalse(TEXT("snake_case is REJECTED — this is the
shipped defect"), …)` is the row that would have caught this in CI.

**It runs over all four builder shapes** (`:334-340`: populated · empty roster · empty places · both empty),
which matters because `at-least` is only emitted on the `bHasKinds` branch — a rule emitted on one branch is
exactly the rule that escapes review. ✅

**And the wrong-direction rename is blocked too** (`:382-388`): `at_least`, `ancient_ground_near` and
`own_castle` must all still be present in the emitted grammar. A "consistency" sweep that kebab-cased the JSON
key fails this test. That is the corpus-breach tripwire, in CI. ✅

**Existing tests correctly updated, none removed or weakened:** `GetRuleRhs(…, "at-least")` (`:583`),
`HasRule(EmptyRoster, "at-least")` (`:679`), plus two **new** rows asserting the split explicitly (`:591-595`:
`when` still contains the JSON key `at_least` **and** references the rule `at-least` exactly once). Count is
now 12 tests. ✅

⇒ **The test that certified the broken artifact now rejects it. Verified by trace, not by description.**

---

## ⚠️ WARNINGS

**[WARN-1] `SiegeAssistantSnapshot.h:212-223` + handoff §5a + the board — the coupling analysis omits the
dimension that bites first in shipped play.** §5a reasons only about roster *width* (kinds), and concludes
~223 chars of slack at `MaxRosterKinds = 8`. But `MaxUtteranceChars = 240` applies to **both** the `order:`
line **and** the `pending:` line (`SanitizeForPrompt` at `:853` and `:856`), and neither is ever truncated by
the budget — only the roster is. Bounding Zone C's fixed head and tail:

```
Head  = "[FORCES]\n" 9 + places ≤ 99 + "roster:\n" 8            ≈ 116
Tail  = stances ≈ 59 + hero 12 + pending ≤ 250 + "[ORDER]\n" 8 + order ≤ 248   ≈ 577
RosterBudget = 893 − 116 − 577 = 200   ⇒ ~4 of 8 kinds print
```

At the old 1440 the same case gave a 555-char roster budget and **did not truncate at all**. So the tightening
does change shipped behaviour in a reachable case — a long typed sentence *plus* a long pending line. It takes
**both** to bite (a 240-char utterance with a short `pending: none` still clears comfortably: budget ≈ 436 vs
366 needed), which is why this is a WARN and not a blocker. **It is logged, loudly, with both causes named —
which is exactly what makes it acceptable and is the ruling's condition doing its job.** Record the dimension
in the header comment and in TASK-423's spec beside the `ZoneBCharReserve` lever; do not act on it now.

**[WARN-2] `SiegeLlamaSpike.cpp:174` — a hand-mirrored constant with a documented duty and no mechanical
tie.** `SpikeMaxSnapshotChars = 1085` mirrors `USiegeAssistantSnapshot::MaxSnapshotChars` across a module
boundary the plugin may not cross. The mirror duty is written down (`:172`), which is the right mitigation
available — but this is **structurally the same shape as the defect this pass exists to fix**: two copies of
one truth, kept in step by a comment. **The consequence here is strictly bounded and I am not asking for a
change:** the constant is print-only (§7a-a), so a divergence can only misprint a headroom figure — it can
never move a measurement. Record it on TASK-423, which deletes the spike anyway.

**[WARN-3] Three copies of `IsLegalGbnfRuleName` now exist** — `SiegeAssistantGrammar.cpp:110`,
`SiegeLlamaSpike.cpp:710`, `SiegeAssistantGrammarTest.cpp:129` — and the test file additionally carries a
**second, differently-implemented** identifier scanner inside `IsGrammarWellFormed` (`:238`) that uses
`FChar::IsAlnum` (Unicode/locale-aware) where its own sibling three functions up uses an explicit ASCII range.
**The divergence is in the safe direction** — a non-ASCII alphanumeric in identifier position would be kept in
the token and then fail as an undefined rule rather than being blessed — and it is unreachable from this
generator, since all identifier-position text is a source literal (§3b). But two scanners in one file
disagreeing about the charset is how this defect class starts. Make `IsGrammarWellFormed`'s inner predicate
call `IsLegalGbnfRuleName`'s charset (or a shared `IsGbnfNameChar`).

**[WARN-4] The `IsLegalGbnfRuleName` charset is verified only on the edges this grammar reaches.** `_`
excluded and `-` included are both established empirically against vendored b10235 (§2b). **Digits and
uppercase are asserted by the test (`item2`) but not verified against the parser**, and could not be — the
llama.cpp source is not vendored. Harmless today (all 13 rule names are lowercase + one hyphen), but if a
future rule name uses a digit or a capital, **that name's legality is an assumption, not a measurement.** The
cheap closer: have build-master paste one grammar containing such a name into `llama-completion.exe` on the
next run that has the binary out anyway.

---

## NITs

**[NIT-1]** `AppendRule` takes `const TCHAR* RuleName` and passes it to `IsLegalGbnfRuleName(const FString&)`,
constructing a temporary `FString` per call — 13 allocations per `Build`, once per typed sentence. Genuinely
unmeasurable next to inference (the code says so and is right), but a `const TCHAR*` overload costs nothing.
Both lanes.

**[NIT-2]** `SiegeAssistantGrammar.cpp:242` / `:257` — `ensureAlwaysMsgf` is evaluated unconditionally after
the `if (!bLegal)` log block, so the legal path pays a branch it has already taken. Cosmetic; merging them
would also make the Shipping-compile-out behaviour visible at the site (see BLOCKER-1).

**[NIT-3]** `IsGrammarWellFormed`'s backslash handling (`:218-222`) applies **outside** terminals as well as
inside, unlike the production `CollectRuleReferences`, which honours it only inside. Unreachable in this
generator (no `\` is ever emitted outside a terminal) and harmless, but the two should agree if only so a
reader comparing them does not have to work out which one is right.

**[NIT-4]** `handoffs/TASK-410-programmer.md` §3 reports "15 references" in the sweep; my enumeration of the
emitted right-hand sides counts **16** (`item` appears 6 times across `selection`'s three alternatives, not 5).
The conclusion is unaffected — all of them are legal and defined — but the figure is quoted on the board.

---

## NOTES

**[NOTE-1] ⚠️ TOOLING HAZARD, CONFIRMED TWICE THIS PASS — the `Grep` tool mangles C++ comment markers on this
machine.** `Grep` rendered `SiegeAssistantSnapshot.cpp:558` as `\ the SiegeAssistantJsonKeys…` and `:567` as
`\               HARDER REASON:…`, and rendered `SiegeAssistantCommand.h:153`'s `/**` as `\**`. Read literally,
those would be **compile errors** and I very nearly opened a blocker on the first one. **Raw `Read` shows both
lines are correct `//` comments.** `//` is being collapsed to `\` and `/*` to `\*` in Grep's content output.
⇒ **The standing law — comment-trap audits on raw file reads, never Grep — is not a stylistic preference on
this machine; Grep output is actively misleading about comment structure.** Recommend this be promoted into
CONVENTIONS §10 beside the `*/` trap.

**[NOTE-2] Comment-trap audit: PASS (0).** Re-run on raw reads over every changed region in all six files. No
`*/` inside a glob pair, no block comment terminated early, no unmatched terminator. The heavy new comment
blocks (`SiegeAssistantGrammar.cpp:72-109`, `:134-151`, `:209-232`; `SiegeAssistantSnapshot.h:161-224`,
`:449-473`; `SiegeLlamaSpike.cpp:691-708`) are all clean, including the ones containing `[a-zA-Z0-9-]` and
`llama_sampler_init_grammar`.

**[NOTE-3] UE 5.8 API + safety sweep: clean.** No deprecated or removed API. `ensureAlwaysMsgf`, `UE_LOG`,
`FMath::Min/Max/Clamp`, `MAX_int32`, `FString::Appendf`, `TSet::Add(Elem, &bAlreadyExists)`,
`ParseIntoArrayLines`, `RightChop`, `FChar::IsAlnum` and
`EAutomationTestFlags::EditorContext | EngineFilter` are all current 5.8 forms (the modern scoped
`EAutomationTestFlags`, not the removed `ATF_*`). Format specifiers audited on all six new `UE_LOG` /
`ensureAlwaysMsgf` lines: `%s` against `const TCHAR*` for `RuleName`/`TagWarn` (no deref — correct) and
against `*Reference` for `FString` (deref — correct); the two `BuildZoneC` messages' 8 and 9 arguments match
their specifiers. No `IsValid`/null gap — `Build` is pure over two arrays with no pointers, and
`AppendRosterBlock` guards every parallel-array read with `IsValidIndex` (`:773-775`, `:787`). **No new
replicated state, no new `UPROPERTY` needed** (`WarnedRosterKinds*` are plain `mutable int32` — no GC
surface). **No per-tick work added**; `Build` and `BuildZoneC` remain once-per-sentence.

**[NOTE-4] CONVENTIONS is already current and does NOT contradict the code.** §8 (`CONVENTIONS.md:689`)
records the 1085 ruling, the measured ratio table, the binding logging condition, and explicitly states the
condition "**is UNDER QA and is NOT called verified here**". This report is that verification: **the condition
is met** (§5), subject to BLOCKER-1's Shipping caveat. The surviving `1440` mentions in that file are
historical narrative, not live pins. No follow-up task needed.

---

## Notes for build-master

1. **Your in-flight bars #2 and #5 are NOT affected by this FAIL. Report them.** Nothing here changes an
   emitted byte, a prompt, a grammar or a constant the spike reads. See §6 and §7a-a.
2. **Confirm these five invariants from your own log** and the "nothing moved" claim is proven mechanically
   rather than asserted: `zoneA_chars=4314` · `zoneB_chars=68` · `zoneC_chars=887` · `zoneB_start_tok~=1147`
   / `zoneC_start_tok~=1179` · `reused − zoneB_start = 9`. Bar #3 must reproduce **77.1 %**.
3. **The `SPIKE_TOKENS` line now reads `MaxSnapshotChars=1085 headroom=130`, not `1440 / 485`.** Expected, and
   the only intended change to that line. Repoint any parsing script.
4. **The proof of the fix is a non-NULL `llama_sampler_init_grammar`, and it is yours alone.** Nothing in this
   report, in the validator, or in the new test is evidence the grammar loads. Please state the sampler result
   explicitly — non-NULL, and constrained output that is JSON rather than `<think>` — as its own line item.
   If you have the b10235 binary out, WARN-4's one-line extra (a rule name containing a digit) closes the last
   unverified charset edge for free.
5. **Bar #5's two standing conditions still travel with the number** (unchanged): read against the holdout's
   own printed leniency floor, never dev's 48 % (WARN-9); and valid only while TASK-416's `MaxRosterKinds = 8`
   seam stays open as measured (WARN-5). **The holdout one-shot is still UNSPENT and I did not spend it** —
   §2c's corpus check was a negative-existence test that read no content.

## Notes for gameplay-programmer (the fix loop — this is small)

1. **BLOCKER-1 only.** Correct the three Shipping claims (`SiegeAssistantGrammar.cpp:216-219`, `:229-231`,
   handoff §4) to state what `Build.h` and `LogMacros.h` actually do. **Do not change the code, and do not
   flip the target flags.** The mirrored comment in `SiegeLlamaSpike.cpp:795-800` does not make the claim —
   leave it.
2. Optional in the same pass, all cheap: WARN-3 (one shared charset predicate in the test file), WARN-1 (add
   the utterance+pending dimension to `SiegeAssistantSnapshot.h:212-223`), NIT-4 (16 references, not 15).
3. WARN-2 / NIT-1 / NIT-2 / NIT-3 / WARN-4 are carry-forward. **Do not touch `ZoneBCharReserve`** — §7a.

---
---

# Re-gate — BLOCKER-1

**Date:** 2026-08-03 · **Reviewer:** qa-reviewer · **Scope:** targeted re-gate of BLOCKER-1 + WARN-3 only.
**Inputs:** `SiegeAssistantGrammar.cpp` (new comment block `:209-290`) · `Tests/SiegeAssistantGrammarTest.cpp`
(the WARN-3 convergence) · `handoffs/TASK-410-programmer.md` §4 / §4a.

# Verdict: **PASS** — **0 BLOCKERS** · 3 WARN (all new, all documentation) · 2 NIT

> ## ✅ **BLOCKER-1 IS CLOSED. THE LLM LANE IS CODE-CLEAR TO COMMIT.**
> **TASK-410, TASK-416 and TASK-417 are ALL COMMITTABLE.** No blocker survives on any of the three.
> Nothing here is a code defect; the three new WARNs are surviving *echoes* of the closed blocker in
> documentation, and one of them I close myself in this report's board text.

---

## R1. ⚠️ THE CITATIONS — SPOT-CHECKED BY RAW READ. **ALL TEN ARE EXACT. NONE HAS DRIFTED.**

This was the whole reason BLOCKER-1 existed: a comment that fixes a false claim with drifted line numbers is
the same defect wearing the fix's clothes. **I opened every cited range in the engine on disk and read the
lines.** Not one is off by even one line.

| citation | what is ACTUALLY on those lines | ✅ |
|---|---|---|
| `Build.h:200-203` | `/** If not specified, disable logging in shipping */` `#ifndef USE_LOGGING_IN_SHIPPING` `#define USE_LOGGING_IN_SHIPPING 0` `#endif` | exact |
| `Build.h:350-352` | inside `#elif UE_BUILD_SHIPPING`: `#ifndef NO_LOGGING` / `#define NO_LOGGING !USE_LOGGING_IN_SHIPPING` / `#endif` | exact |
| `Build.h:322-324` | inside `#elif UE_BUILD_TEST`: the **identical** three lines | exact |
| `Build.h:205-212` | `USE_CHECKS_IN_SHIPPING 0` (205-207) **and** `USE_ENSURES_IN_SHIPPING USE_CHECKS_IN_SHIPPING` (209-212), with the engine's own comment *"If not defined follow the CHECK behavior…"* | exact |
| `Build.h:332-334` | Shipping: `#ifndef DO_ENSURE` / `#define DO_ENSURE USE_ENSURES_IN_SHIPPING` / `#endif` | exact |
| `Build.h:304-306` | Test: the **identical** three lines | exact |
| `Build.h:241-296` | opens at `#if UE_BUILD_DEBUG` (241) and closes at the `#endif` of `UE_BUILD_DEVELOPMENT`'s `NO_LOGGING 0` (296). `DO_ENSURE 1` at 248-250 (Debug) and 276-278 (Development); `NO_LOGGING 0` at 266-268 and 294-296. **The range is chosen precisely** — it is exactly the two configurations the comment claims the guard is live in | exact |
| `LogMacros.h:184-194` | `#if NO_LOGGING` (184); `// This will only log Fatal errors` (186); the `UE_LOG` macro 187-194 whose body is `if constexpr ((ELogVerbosity::Verbosity & VerbosityMask) == ELogVerbosity::Fatal)`. For `Error` that is a compile-time false ⇒ **an empty block**, exactly as the comment says | exact |
| **`AssertionMacros.h:467-472`** ⚠️ **THE FLAGGED ONE** | 467 is `#else // DO_ENSURE && !USING_CODE_ANALYSIS`; 469-472 are the degraded macros, **ending at 472 with `#define ensureAlwaysMsgf( InExpression, InFormat, ... ) (LIKELY(!!(InExpression)))`** | exact |
| `GitClaudeUnrealTest.Target.cs` | read in full — **15 lines**, sets only `Type`, `DefaultBuildSettings`, `IncludeOrderVersion`, `ExtraModuleNames`. **No `bUseLoggingInShipping`, no `bUseChecksInShipping`** | exact |

**⚠️ ON `AssertionMacros.h:467-472` SPECIFICALLY — the range the programmer flagged as originally taken from a
`Grep` hit rather than a raw read. I gated it hardest, and it is right, including the part a line-range check
alone would not catch.** The cited range is meaningless unless the `#else` at 467 belongs to a `#if` on
`DO_ENSURE`, so I went and found the opener: **`AssertionMacros.h:368` — `#if DO_ENSURE && !USING_CODE_ANALYSIS`.**
So `DO_ENSURE 0` ⇒ the `#else` branch ⇒ line 472 ⇒ `ensureAlwaysMsgf` really is `(LIKELY(!!(InExpression)))`.
The comment's two sub-claims — *"degrades to `(LIKELY(!!(expr)))`"* and *"the condition is still evaluated, but
nothing is reported"* — are **both literally true of line 472**. The re-check the programmer said it performed
was performed, and it landed.

⇒ **The fix does not repeat the defect. The comment's every factual assertion is now sourced to a line that
says what it is claimed to say.**

---

## R2. ✅ THE REPLACEMENT CLAIM — RE-DERIVED FROM THE ARTIFACT, NOT ACCEPTED FROM THE COMMENT

The comment replaces the false "not compiled out of Shipping" with a positive claim: **dev-time scope is
*sufficient*, because the identifier set is settled before the program runs.** That claim has two premises and
a redirect. I checked all three.

**Premise 1 — no rule name is ever built from data.** ✅ **CONFIRMED BY ENUMERATION.** All 13 `AppendRule`
sites take a `TEXT("…")` literal: `:431` root · `:449` command · `:459` question · `:474` ask · `:492` intent ·
`:507` kind · `:524` where · `:541` count · `:570` **at-least** · `:585` item · `:617` selection · `:633` who ·
`:660` when. Not one takes a variable, a concatenation or an `FString`. Every data-derived string still reaches
the grammar through `GbnfJsonString` / `GbnfTerminal`, inside a quoted terminal (re-verified at the +58 offsets
of §3b's original enumeration — see R4).

**Premise 2 — "no preprocessor branch varies the set."** ✅ **CONFIRMED MECHANICALLY, and this is the premise I
was asked to confirm rather than assume.** A scan of `SiegeAssistantGrammar.cpp` for `#if` / `#ifdef` /
`#ifndef` / `#else` / `#elif` / `#endif` / `#define` returns **ZERO matches in the entire file**. There is no
conditional compilation anywhere in it. The only branch in `Build` is the **runtime** `bHasKinds`, which
selects between exactly two rule sets — **13 rules** (roster non-empty) and **8** (`root`, `command`,
`question`, `ask`, `intent`, `where`, `who`, `when`). Both are subsets of the same fixed literal set.

⇒ **A Shipping build therefore does contain exactly the identifiers a Development build contained.** The
conclusion follows from the artifact, not from assertion.

**The redirect — "`Siegebound.Assistant.Grammar.RuleNameCharset` is what actually protects a shipped build."**
⚠️ **A comment that redirects the guarantee to a test is only as true as that test, so I read the test rather
than its name.** It genuinely covers what the comment credits it with, and the coverage is exactly matched to
premise 2:

| the comment credits the test with… | in the test | ✅ |
|---|---|---|
| running `Build` over **all four builder shapes** | `:370-376` — populated · empty roster · empty places · both empty. **This is what makes the `bHasKinds` branch non-load-bearing: both rule sets are covered** | ✅ |
| asserting **this same charset** on every rule name | `:393-396` — `IsLegalGbnfRuleName(RuleName)` per line, per shape | ✅ |
| catching the **reference** half too | `:404-407` — `IsGrammarWellFormed`, whose scanner now breaks where llama.cpp breaks | ✅ |
| being a **gate, not decoration** | `:427-432` — `at-least`/`root`/`item2` accepted; `at_least`/`at least`/empty rejected | ✅ |
| protecting the **wire format** from the wrong-direction sweep | `:418-423` — `at_least`, `ancient_ground_near`, `own_castle` must survive | ✅ |

⇒ **The redirect is sound.** The test asserts the charset over every rule set the generator can emit, so it
does close the property in advance of packaging, which is precisely what the runtime guard's dev-only scope
requires of it. **The claim is calibrated, not inflated.**

---

## R3. ✅ WARN-3 — CONVERGED IN THE RIGHT DIRECTION, AND THE RESIDUE IS EXPLAINED RATHER THAN SILENT

**One predicate, both consumers.** `IsGbnfNameChar(const TCHAR)` at `:145-151` — the explicit ASCII range
`[a-zA-Z0-9-]`. `IsLegalGbnfRuleName` calls it at `:175`; `IsGrammarWellFormed`'s inner scanner calls it at
`:274`. `FChar::IsAlnum` no longer appears in the file. Defined before both consumers, same namespace, both
uses live ⇒ no linkage or ordering hazard.

**✅ THE DIRECTION IS CORRECT AND IT IS THE LOAD-BEARING PART OF THIS FIX.** Converging on `FChar::IsAlnum`
would have been the *wrong* fix even though it removes the same duplication: `IsAlnum` is Unicode/locale-aware,
so it accepts characters — an accented letter, a full-width digit — that llama.cpp's plain-ASCII rule-name scan
**breaks an identifier on**. That widens the test's notion of legality **past the parser's**, which is the
direction that lets a defect through rather than catching it. The code argues exactly this, at `:138-143`, and
the reasoning is correct as written. **Converging on the narrower predicate is the only convergence that makes
the file agree with the parser instead of with itself.**

**Behavioural check, because a shared predicate is still a code change in a test file:** on every character
this generator emits outside a terminal (`a-z`, `-`, space, `|`) the old and new predicates agree exactly, so
all four builder shapes still pass and no existing assertion is weakened. The change is strictly narrowing, and
only on input the generator cannot produce. ✅ Safe direction, no coverage lost.

**The residual three-file duplication is stated in the code, not left silent** — `:156-164` names all three
copies and gives the three reasons it is not fixable here: the production copy has internal linkage in an
anonymous namespace and is not declared in the header; asserting the charset *with the function under test*
would be circular; the spike's copy is across a module boundary the plugin may not cross. **Within a file there
is now exactly one definition, which was the fixable part.** That is the right scope and the right disclosure.
Carry-forward on TASK-423, which deletes the spike lane anyway.

---

## R4. ✅ NO CODE CHANGED, NO FLAG FLIPPED, NO EMITTED BYTE MOVED — **PROVEN, NOT ASSERTED**

I still cannot run `git diff`. So here is a mechanical proof from the artifact itself.

**⇒ EVERY ONE OF THE 13 `AppendRule` SITES SHIFTED BY EXACTLY +58 LINES.** Against §3a's enumeration from
gate 1:

```
root  373→431   command 391→449   question 401→459   ask   416→474   intent 434→492
kind  449→507   where   466→524   count    483→541   at-least 512→570
item  527→585   selection 559→617 who      575→633   when  602→660
                                                     ⇒ +58, +58, +58 … all thirteen
```

Every data-path site from §3b lands on the same offset (`:413→471`, `:431→489`, `:446→504`, `:462→520`,
`:464→522`, `:479→537`, `:481→539`, `:509→567`, `:572-573→630-631`, `:584→642`), as do NIT-2's two
`ensureAlwaysMsgf` lines (`:242→300`, `:257→315`). **Everything ABOVE the edited comment is at delta 0**
(`:94-98`, `:110`, `:134-151`, `:152-207`). And the comment block itself: it ended at `:232` before and ends at
`:290` now — **58 lines**.

⇒ **A uniform +58 across every site, with delta 0 above the block, is only possible if the entire diff in this
file is the comment block. Not one line of code was added, removed, reordered or edited.** The emitted grammar
is byte-identical.

**The other files, checked at the exact line numbers gate 1 cited — all unmoved:**

| artifact | gate 1 cited | now | |
|---|---|---|---|
| `MaxSnapshotChars = 1085` | `SiegeAssistantSnapshot.h:225` | `:225` | unmoved ⇒ file untouched |
| `ZoneBCharReserve = 192` | `:233` (§7a) | `:233` | unmoved |
| `MaxRosterKinds = 8` | — | `:241` | unchanged value |
| `MaxUtteranceChars = 240` | WARN-1 | `:244` | unchanged value |
| `SpikeMaxSnapshotChars = 1085` | `SiegeLlamaSpike.cpp:174` | `:174` | unmoved ⇒ spike untouched |
| spike `at-least` definition | `:942` | `:942` | unmoved |
| spike JSON key `at_least` | `:997` | `:997` | unmoved |
| spike rule reference `at-least` | `:998` | `:998` | unmoved |

**The target flags were NOT flipped.** A repo-wide search for `bUseLoggingInShipping` / `bUseChecksInShipping` /
`bUseEnsuresInShipping` / `GlobalDefinitions` / `USE_LOGGING_IN_SHIPPING` returns **no code hit anywhere** — the
only occurrences are the new explanatory comment and this report's own prose. `GitClaudeUnrealTest.Target.cs` is
still the same 15 lines. ✅

⇒ **Grammar output, corpus, holdout, Zone A, few-shots, JSON schema, scoring rule, `MaxSnapshotChars`,
`ZoneBCharReserve`, fixtures and zone boundaries are all untouched. Bar #3's 77.1 % and bar #4's footprint
remain reproducible, and §6's five printed invariants stand exactly as written.**

**⚠️ ONE HONEST QUALIFICATION ON "COMMENTS-ONLY":** it is comments-only in **production runtime code**. The
**test** file did change behaviourally — the shared `IsGbnfNameChar` (R3) — which is the WARN-3 fix this report
asked for. It is confined to `#if WITH_DEV_AUTOMATION_TESTS` and cannot reach a shipped byte. Recording it so
nobody reads "comments-only" as "nothing executable changed anywhere".

---

## R5. ✅ COMMENT TRAP — RAW READS ONLY. **PASS (0).** The relayed 9/9 · 15/15 RE-DERIVED AND EXPLAINED.

Per the relayed-diagnosis law I treated the orchestrator's terminator-balance scan as a **lead** and re-derived
it from raw reads of both files in full. **It reproduces — and because the relayed scanner did not exclude
string literals, the number alone was not the answer. Here is the composition, which is:**

**`SiegeAssistantGrammar.cpp` — 9 openers / 9 terminators.** Six multi-line blocks (`:9-14`, `:31-37`,
`:72-109`, `:134-151`, **`:209-290`**, `:326-342`) + three single-line `/** … */` (`:54`, `:60`, `:66`). **No
`/*ParamName*/` idiom in this file**, and **no `/*` or `*/` inside any string literal** — so the relayed 9 is
9 real comment pairs with **zero** literal-derived false pairs, and it pairs sequentially with no overlap.

**`SiegeAssistantGrammarTest.cpp` — 15 openers / 15 terminators, and the 15 decomposes as 11 + 4.** Four
multi-line blocks (`:13-52`, `:118-144`, `:153-165`, `:184-199`) + seven single-line `/** … */` (`:56`, `:62`,
`:68`, `:86`, `:92`, `:100`, `:321`) = **11 doc comments**, **plus 4 inline `/*ParamName*/` arguments**
(`:72` `/*bCullEmpty*/`, `:96` `/*InCullEmpty*/`, `:208` `/*bCullEmpty*/`, `:381` `/*bCullEmpty*/`).
4 + 7 + 4 = 15. ✅ **That reconciles the relayed count exactly and identifies the four a literal-blind scanner
could have mis-attributed — all four are the benign, self-balancing single-line form.**

**The new 82-line block at `:209-290` is clean** — no `*` adjacent to `/` anywhere in it, including the
high-risk content (`[a-zA-Z0-9-]`, `(LIKELY(!!(expr)))`, `if constexpr(false)`, `Build.h:200-203`, the escaped
`when ::=` line, `TEXT("…")`). Same for the test file's `:118-144` and `:153-165`. **No premature terminator,
no unmatched terminator, nothing swallowed.** ✅

---

## R6. 🔍 THE ECHO SWEEP — **THE LESSON GENERALISES, AND IT FOUND THREE MORE**

The programmer self-reported the echo two bullets below the one I flagged (handoff §4's `ensureAlwaysMsgf`-vs-
`checkf` bullet, which ended *"the Error log is the always-on channel"*) and corrected it at `:783-787`.
**Neither the orchestrator nor I caught that one.** So I stopped looking for the instance and swept for the
*class* — every phrasing that could re-assert build-configuration ubiquity: `Shipping`, `always-on`,
`compiled out`, `never compiled`, `everywhere`, across `Source/`, the plugin, and the whole pipeline. Every hit
raw-read.

**✅ CLEAN where it counts:** `SiegeLlamaSpike.cpp` contains **zero** occurrences of `Shipping` / `always-on` /
`compiled out` — gate 1's instruction to leave the mirrored comment alone was correct and no echo was
introduced there. In all of `Source/`, `Shipping` appears in only three files: the new (correct) block, and two
pre-existing, unrelated cheat-manager gates. `SiegeAssistantSnapshot.h:305`'s "always-on sanitiser" is about
`MaxUtteranceChars` — plain code that genuinely does run in every configuration. **Not an echo.**

**But three echoes DID survive the blocker:**

**[WARN-E1] `SiegeAssistantSnapshot.cpp:904` — *"Always-on per-turn record"*, describing a
`UE_LOG(…, Verbose, …)`.** Under `NO_LOGGING` that `Verbose` call compiles out exactly like the `Error` one.
**It is true as scoped** — the very next clause contrasts it with the latched `Warning` below (*"the Warning
below deliberately fires once per escalation"*), so "always-on" plainly means *every degraded turn* rather than
*every build configuration* — which is why this is a WARN and not a blocker. But it is the exact phrase that
just cost a blocker, attached to a `UE_LOG`, in the file whose truncation log the §8 cap ruling is conditional
on. One word ("per-turn, not latched") removes the last place a reader can re-derive the killed claim.
**Carry-forward; do not spend a cycle on it now.**

**[WARN-E2] `handoffs/TASK-416-programmer.md:185` — *"so the budget maths never binds on a realistic board"*.**
**True when written at 1440; falsified by 1085.** This is precisely WARN-1's case (a long utterance *plus* a
long pending line leaves ~200 chars of roster budget against 366 needed). The header was updated for the
coupling at `:212-223`; this older handoff bullet was not revisited. Documentation only — the code logs loudly
when it bites — but it is the same shape as the §4 echo: **a claim the 1085 correction falsified, surviving in
a file nobody re-read.** Fold into TASK-423's spec beside WARN-1.

**[WARN-E3] ⚠️ THE BLOCKER'S EXACT WORDING IS STILL LIVE ON THE TASKBOARD — IN TWO PLACES.**
`TASKBOARD.md:4269` (TASK-410's fix-pass bullet) says the validator *"is not compiled out of Shipping"*, and
`:4667` (TASK-417's) says *"is not compiled out of Shipping, and a new 12th automation test … gates it in CI."*
**The board is the hub and the record of the acceptance criterion** — the very ground on which gate 1 refused
to grade this a WARN. Leaving the code right and the record wrong would half-close the blocker.
**I am NOT grading this a blocker, because it costs zero cycles to fix and I am fixing it here:** corrected
replacement text for both lines is supplied to the orchestrator with this report. ⇒ **Closed on application.**

> **📌 FOR THE NEXT REVIEWER, AND WORTH PROMOTING INTO CONVENTIONS:** **a blocker's echo survives the blocker.**
> This one was asserted in **six** places across three artifact classes (three code comments, one handoff
> bullet, two board bullets), and the gate-1 report — mine — enumerated only three. The programmer found the
> fourth itself. I found the fifth and sixth only because this dispatch told me to look for echoes by default.
> **Grep the wording of every closed blocker across code, handoffs AND the board before calling it closed.**
> A claim that lives in the record is still a claim the project believes.

---

## R7. NITs

**[NIT-R1] "in CI" is this project's shorthand, and it is worth one clause of precision.** The comment says the
test *"asserts this same charset in CI, before anything is packaged."* There is **no hosted CI in this repo** —
no `.github/workflows`, no pipeline runner. What exists is real and exercised: build-master's headless
automation run, with the exact command already on record at `handoffs/TASK-420-buildmaster.md:56`
(`UnrealEditor-Cmd.exe … -ExecCmds="Automation RunTests Siegebound.Assistant" -TestExit="Automation Test Queue Empty" -unattended -nullrhi`),
and TASKBOARD's standing instruction to run the suite and paste the pass/fail list. So the claim is
true-if-run, and the mechanism is a person, not a machine. **The cheap closer is in build-master's hands this
pass — see note 3 below.**

**[NIT-R2] Gate 1's optional items 2 and 3 were not taken, and that was the right call.** WARN-1's header
addition and NIT-4's reference count were offered as optional. Not doing them is what keeps
`SiegeAssistantSnapshot.h` **byte-untouched** (R4), which is the safest possible posture for bar reproduction.
Both remain carry-forward. **Not a finding — recorded so nobody reads their absence as an oversight.**

**Carry-forward, unchanged and still open:** WARN-2 · WARN-4 · NIT-1 · NIT-2 · NIT-3, plus the new WARN-E1 /
WARN-E2. **Do not touch `ZoneBCharReserve`** (§7a).

---

## R8. ⛔ COMMITTABILITY — STATED PLAINLY

| task | verdict | committable? |
|---|---|---|
| **TASK-410** (spike lane) | **qa-passed** — file verified untouched since gate 1 (R4) | ✅ **YES** |
| **TASK-416** (snapshot lane) | **qa-passed** — gate 1 already passed this lane on the merits; file byte-untouched since | ✅ **YES** |
| **TASK-417** (grammar + tests) | **qa-passed** — BLOCKER-1 closed (R1/R2), WARN-3 closed (R3) | ✅ **YES** |

**All three are clear to commit.** No blocker survives anywhere in the batch.

**⚠️ WHAT A PASS HERE DOES AND DOES NOT MEAN — unchanged from gate 1 and it still travels with the commit:**
this is a **code review**, not a compile and not a parse. **Nothing in this batch has been compiled.** The
proof that the grammar loads remains a **non-NULL `llama_sampler_init_grammar` at run time**, and it is
build-master's alone. A green QA gate is the precondition for the commit, not evidence the feature works.

---

## Notes for build-master

1. **Everything in gate 1's "Notes for build-master" (items 1-5) still stands verbatim** — the five printed
   invariants, the `MaxSnapshotChars=1085 headroom=130` line change, the sampler-result line item, and bar #5's
   two standing conditions. **Nothing in this re-gate alters any of them.**
2. **Bars #2/#3/#5 are unaffected by the fix pass.** R4 proves the diff is a comment block (+58 uniform) plus a
   test-only predicate. **77.1 % must still reproduce.**
3. **⚠️ ONE CHEAP ADDITION, AND IT CLOSES NIT-R1 FOR FREE.** The corrected comment now names
   `Siegebound.Assistant.Grammar.RuleNameCharset` as *the* mechanism protecting a shipped build. **Please run
   the suite once and record the result** — you already have the editor up, and the command is on record at
   `handoffs/TASK-420-buildmaster.md:56` (`Automation RunTests Siegebound.Assistant`). Report the 12 tests'
   pass/fail, and call out `RuleNameCharset` by name. **That converts the comment's redirect from a documented
   intention into an evidenced gate, in one command.** If it fails, append here and route back — do not commit.
4. **WARN-4's one-line freebie still stands** if the b10235 binary is out: paste one grammar containing a rule
   name with a digit into `llama-completion.exe` and close the last unverified charset edge.
5. **The holdout one-shot is still UNSPENT. I did not spend it in this re-gate** — I read no corpus file at all.
