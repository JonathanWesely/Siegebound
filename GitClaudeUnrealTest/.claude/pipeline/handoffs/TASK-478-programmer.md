# TASK-478 — [FT-A3] per-fixture GBNF + subset parity — the last of Stage A's serial chain

- **Agent:** gameplay-programmer
- **Date:** 2026-08-03
- **Files touched:** `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp` — **one file, nothing else.**
  ⛔ No header, no new file, no `.Build.cs`, **no include added** (`ESearchDir` was already in use in this TU at `BuildPrompt`'s `OutPrompt.Find(...)` — a positive control from the file itself, not an assumption).
- **Compile:** ⛔ **NONE RUN**, per the spec. Two compile gates this batch (TASK-481, TASK-492), each needing a quiet module across both live batches. Everything a compiler would settle is in §7, labelled.
- **Git:** ⛔ **none.** No add, no commit, no branch. `main` is 25 ahead, unpushed.
- **Editor / MCP / PIE:** ⛔ **none.** No model was loaded. ⚠️ **No number in this file is a measurement** — §5 is a proof about *bytes of source*.
- **Gate:** TASK-480. **M8 declaration: this task adds no replicated property, no new replicated class, and no new relevancy tier.**

---

## 1. ⭐ THE HEADLINE — THE DEFECT WAS STRUCTURAL AND IT IS GONE; THE CHECK THAT LOOKED REDUNDANT AFTERWARDS IS THE ONE THAT GOT MORE LOAD-BEARING

**Both spec clauses landed, and the second one inverted while I was tracing it (§20 — trace before typing).**

| Spec | Delivered | Where, **by symbol** (§18c — every offset predating today is stale) |
|---|---|---|
| **(1)** `BuildSpikeGrammar` derives `kind` from the FIXTURE | `BuildSpikeGrammar(const FSpikeWorldFixture&)`; `RunGeneration` gained the fixture and threads it in | `BuildSpikeGrammar` · `RunGeneration` · 4 call sites |
| **(2)** `VerifyFixtureKindParity` permits a SUBSET, still rejects an unknown kind | rewritten: duplicate-free, in-order, non-empty subset of `SpikeRoster` | `VerifyFixtureKindParity` (signature unchanged) |
| **(3)** `t0` unaffected, its bytes do not move | `SpikeFixtureT0` **sha256 IDENTICAL** vs committed HEAD; and the emitted grammar is unmoved for **both** existing fixtures | §5(c), §5(d) |
| **(4)** no `tA`–`tF` | none added. The registry is the extension point; adding a row is one line | `SpikeFixtureRegistry` |
| ⛔ identity/quantity split untouched | `count` and `at-least` rule blocks **sha256-identical**; both loops still bounded by the CONSTANTS | §5(i) |

### ⚖️ THE PART THAT IS NOT IN THE SPEC AND MATTERS MOST: **CLAUSE (1) MAKES CLAUSE (2) MORE IMPORTANT, NOT LESS**

The obvious reading is that once the grammar comes from the fixture, a fixture can no longer disagree with its
grammar, so the parity check is vestigial. **That is backwards, and shipping it on that reading would have been the
worst outcome of this task.**

> **BEFORE:** `SpikeRoster` generated the grammar ⇒ a typo'd fixture kind was a symbol the sampler **FORBADE**. Loud —
> every row needing it fails.
> **AFTER:** the fixture generates the grammar ⇒ a typo'd fixture kind is **ADDED to the GBNF as a legal alternative.**
> The sampler can emit a unit **the game does not have**, and the only thing left between that and a valid-shaped wrong
> command is the shipped executor.

⇒ ⚖️ **`SpikeRoster` stops being the GRAMMAR authority and becomes purely the VOCABULARY authority — and clause (2)(a)
is the whole of that guarantee.** This is why the relaxation is deliberately narrow (§2) rather than "is it in the list".

---

## 2. ⚠️ THE JUDGEMENT CALLS, DECLARED — flag each to QA rather than let them be found

1. **⛔ I KEPT AN ORDER REQUIREMENT THE SPEC DID NOT ASK FOR, RELAXED FROM INDEX-IDENTITY TO RELATIVE ORDER.** A fixture
   may **OMIT** kinds; it may not **REORDER** them. ⚠️ **`SpikeRoster`'s order IS `Docs/Data/cards.csv` row order, which
   is what the shipped `USiegeAssistantSnapshot::Capture()` sorts to** (the roster comment says so in as many words). A
   reordered fixture prints a `roster:` block **no live board can produce**, so bar #3's byte-level prefix claims would
   be measuring a layout that does not ship. ⚖️ **"Permit a subset" and "permit a subset in arbitrary order" are
   different relaxations and I took the smaller one.** ⚠️ **`qa/TASK-500.md` D3 already noticed the old check was
   stricter on order than the law states and flagged it as "relevant to TASK-478's design" — this is my answer to that,
   and if QA rules the other way, deleting the third branch is a two-line revert.**
2. **⛔ I ADDED TWO CHECKS THE OLD CODE GOT FOR FREE, BECAUSE A NAIVE SUBSET TEST WOULD HAVE SILENTLY DROPPED THEM.**
   *(b)* **no duplicates** — index-identity forbade them implicitly; a duplicate emits a duplicate GBNF alternative and
   a duplicate `roster:` line. *(d)* **non-empty** — count-equality-with-13 made an empty roster impossible; an empty one
   generates `kind ::= ` with no alternatives, **llama.cpp refuses the WHOLE grammar, `llama_sampler_init_grammar`
   returns NULL, and every generation runs UNCONSTRAINED** — the precise failure §9c records as having reached a
   measurement run. ⚖️ **"Relax the direction that is over-strict; do not delete the check" is satisfied by naming what
   the old form was enforcing accidentally and re-enforcing it on purpose.**
3. **⛔ THE EMPTY-ROSTER CASE IS REPORTED AND THEN EMITTED ANYWAY — I DID NOT SUBSTITUTE `SpikeRoster`'s FULL LIST.**
   This is the file's own idiom, cited: `AppendRule`'s illegal-rule-name path already says *"the offending rule is still
   EMITTED, not dropped: llama.cpp refuses the grammar either way, and a faithful dump is what makes the failure
   diagnosable."* ⚠️ **A silent fallback would be far worse than the failure it prevents: the run proceeds under a
   grammar nobody asked for and reports a clean number for a board it never used.**
4. **⛔ NO `fixture=` ON `Siege.Llama.SpikeEval`, AND THIS IS A REFUSAL, NOT AN OVERSIGHT.** It is the obvious next step
   and it is barred: `RunOneSplit`'s own comment says *"THE EVAL ALWAYS USES t0, AND THAT IS NOT AN ARBITRARY CHOICE …
   running a split against t1 would silently rewrite the corpus's premises and score the model on questions nobody
   wrote."* The registry's doc comment now says the same thing at the point a future task will look.
5. **⚖️ THE ALWAYS-ON SEAM READOUT IS ON `Siege.Llama.SpikeGrammar`, NOT ON `SpikeEval` — TASK-477's DISPOSITION,
   COPIED.** §14 wants the positive proved, and a check that only ever prints on failure is indistinguishable from a
   check that never ran. But a "seam CLOSED" line on `SpikeEval`'s **default** path moves the instrument every number on
   record was taken with (§16 freeze #2, unconditional). ⇒ **The measurement lanes get the WARNING only; the inspection
   command gets the verdict both ways, and `SpikeEval`'s help text was NOT touched by me.**
6. **⚠️ I TOUCHED NEITHER FROZEN COMMAND'S HELP STRING.** 476 and 477 each appended to one; I appended to
   `Siege.Llama.SpikeGrammar`'s only — a command **not in §16's frozen set**, from which no measurement has ever been
   taken. **Strictly less surface than either predecessor**, inheriting their QA item all the same.
7. **⛔ I REWROTE TWO PRE-EXISTING `UE_LOG` STRINGS. DECLARED, COUNTED, AND ACCOUNTED FOR IN THE PROOF — see §3.**

---

## 3. ⛔ THE §22 SWEEP — A DEAD MECHANISM WAS BEING ASSERTED IN FOUR PLACES, TWO OF THEM RUNTIME LOG STRINGS

§22 names runtime log strings as **the priority surface**, because *"a log line is the artifact an investigator TRUSTS
MOST, so a false one is believed over the code"* and *"a grep for PROSE does not surface them."* Clause (1) killed the
sentence **"the GBNF's `kind` alternatives are generated from `SpikeRoster`"**, so I searched for the **claim**, not the
comment. **Extent found: 4 artifacts — 2 comments, 2 `UE_LOG` strings.**

| # | surface | kind | disposition |
|---|---|---|---|
| 1 | `SpikeRosterT1`'s doc comment — *"the GBNF's `kind` alternatives are generated from SpikeRoster… VerifyFixtureKindParity checks this"* | comment | rewritten. ⚠️ **The old reason is quoted and then retired in place**, and the reason that SURVIVES is stated: t0/t1 must differ in NUMBERS only or bar #3 compares two turns under two grammars |
| 2 | the `static_assert` message — *"must carry exactly the kinds the GBNF was generated from"* | compile-time string | **assert KEPT, message rewritten.** ⚠️ It is now load-bearing for a second thing: it is what makes the seam readout provably silent on t1 **at compile time** |
| 3 | `RunBenchJob`'s parity warning — *"A BENCH FIXTURE DISAGREES WITH THE GRAMMAR'S KIND LIST… the sampler cannot emit"* | **`UE_LOG(Warning)`** | rewritten — the danger **inverted** (see §1) and the old text described the opposite failure |
| 4 | `CmdSpikePrompt`'s parity warning — *"fixture %s disagrees with the grammar's kind list"* | **`UE_LOG(Warning)`** | rewritten. ⚠️ **Now self-contradictory**: the grammar's kind list *is* this fixture's roster |

- ⚠️ **#4 IS INSIDE TASK-477's FUNCTION AND I WENT IN ANYWAY — SAY SO OUT LOUD.** 477's collision note reads *"478 does
  not own this function."* I read that as a bound on stepping on **in-flight work** (477 is complete and gated), not a
  permanent easement over a stale claim §22 calls the priority surface. **What I changed is one format string in a
  branch that is unreachable for `t0` and `t1`** — no logic, no identifier, no line-position change for any reachable
  line. ⛔ **If QA rules the boundary absolute, reverting the string is a one-line revert and nothing depends on it.**
- ✅ **AND THE SWEEP'S NEGATIVE RESULTS ARE RESULTS (§22), so they are reported:**
  - **`AppendZoneC`'s `other_kinds: none` is HARDCODED and I did NOT touch it.** ⚖️ Its comment (*"Nothing collapses in
    EITHER fixture by construction: both print all thirteen kinds"*) is **still true today** — and for a *subset*
    fixture `other_kinds: none` remains **correct**, because a sparse board has not collapsed anything, it simply has
    fewer kinds. ⚠️ **This is D2, escalated and owned elsewhere (TASK-486).** The surface a Stage D fixture author will
    need to revisit is that comment, and it is named here so they do not have to find it.
  - **`CmdSpikeGrammar`'s pre-existing header line — *"diff this against `USiegeAssistantGrammar::Build(fixture kinds,
    the 7 pinned places)`"* — needed no change and is EVIDENCE FOR THIS TASK.** The **shipped** generator has always
    taken *fixture kinds*. ⇒ ⚖️ **Clause (1) is a correction TOWARD §9c's named authority, not a divergence from it: the
    spike was the lane generating from a fixed thirteen-symbol list.**
  - **Three Zone-A-consuming commands were swept for the `prompt=` bypass shape (§7 finding 1): 2 clean, 1 defective.**

---

## 4. ⭐ WHAT MAKES D3 *VISIBLE* RATHER THAN MERELY *POSSIBLE* — `ZONE_A_KIND_SEAM`

Clause (1) makes a sparse board **expressible**. It does not make a score taken on one **attributable**, and §9c is
explicit that the attribution is the whole problem:

> *"What makes it dangerous is an ABSENCE, not an event: **THERE IS NO LOG LINE.** Nothing throws, nothing refuses,
> nothing warns, and the JSON that does come back is well-formed… the cost arrives as **diffuse accuracy loss with no
> cause attached** — spread across rows that have nothing to do with the missing kind, and read as 'the model is bad'."*

**That paragraph is a specification for `ReportZoneAKindSeam`.** On a fixture that omits a kind Zone A still teaches,
the run now **says so**, naming the symbols — so a Stage D score drop is a reading rather than a mystery.

- **Two sets, because they are different strengths of evidence.** `NAMED` = the symbol appears anywhere in Zone A (the
  synonym table names all thirteen, so this is the wide set). `DEMONSTRATED` = the symbol appears inside a few-shot
  **answer** as `"kind":"X"` — §9c's actual claim, and the sharper signal: the model was shown that exact token *being
  emitted*.
- **⛔ THE DEMONSTRATED SET IS SCANNED OUT OF THE ACTUAL ZONE A TEXT, NEVER TRANSCRIBED.** A hardcoded
  `{footman, sorcerer, archer}` would be a **third copy** of Zone A's content, free to drift — and **flatly wrong under
  `prompt=`**, which is the case that matters most. ⚖️ **§9c's own promoted law: "a method that compares an artifact to
  another artifact of the same authorship cannot detect an error they share."** The scan verifies, mechanically, that
  today's Zone A demonstrates exactly `archer, footman, sorcerer` (§5e) — i.e. **§9c's three names, confirmed at the
  artifact rather than quoted from the law.**
- **⚠️ THE `NAMED` TEST IS A SUBSTRING TEST AND SAYS SO IN ITS OWN COMMENT.** No `SpikeRoster` kind is a substring of
  another, so the sets cannot cross-contaminate; a Zone A that merely mentions a kind in prose counts as naming it,
  which is the intended reading (*"is this symbol in front of the model"*). **It is a POSITIVE structural fact about the
  prompt bytes, not a search for an absence (§14).**

---

## 5. 🔒 THE ADDITIVE PROOF — MECHANICAL, AND IT INCLUDES A NUMBER 477's DID NOT

⚠️ **FIRST, THE §14 TRAP THIS PROOF ALMOST FELL INTO, RECORDED BECAUSE IT WOULD HAVE PRODUCED A VACUOUS PASS.**
The git **toplevel is the PARENT of the project directory** (`.../GitClaudeUnrealTesting`, prefix
`GitClaudeUnrealTest/`), so my first `git show HEAD:Plugins/…` returned **empty** — and an empty HEAD reads as
*"the file is new"*, which turns **every** survival check into a silent pass: `HEAD UE_LOG calls: 0 … ORPHANED: 0`.
✅ **Caught by a positive control** (`assert len(blob) > 100000`), and the real HEAD is **4764 lines**, which matches
TASK-476's independently reported starting size. **§14 instance 3 wearing a different hat: the tool answered about the
path, not about the file.**

**(a) FORMAT-STRING SURVIVAL, `git show HEAD:` vs the working tree — the ledger closes at zero unaccounted:**

```
HEAD UE_LOG calls                    : 87
CURRENT UE_LOG calls                 : 120
  survive CHARACTER-IDENTICAL        : 77
  survive via 476's tag insert       : 8    (split=%s -> split=%s%s)
  survive via 478's DECLARED rewrite : 2    (§3 rows #3 and #4, both named above)
  ORPHANED / UNACCOUNTED (must be 0) : 0
```

⛔ **I DO NOT CLAIM 477's `ORPHANED = 0` WITH NO REWRITES — I CLAIM TWO REWRITES, BOTH DECLARED BEFORE THE COUNT WAS
TAKEN, AND ZERO UNACCOUNTED.** ⚖️ **A stale runtime log string is not a thing to leave alone in order to keep a metric
clean; §22 makes it the surface to fix first.** Reporting it as `ORPHANED = 0` without the declaration would be exactly
the *confident green describing something else* this batch exists to prevent.

**✅ AND THE ARITHMETIC CLOSES ACROSS ALL THREE HANDOFFS INDEPENDENTLY — a corroboration none of us could fake alone:**

```
 87 (HEAD) + 16 (476) = 103   <- TASK-476 reported 103
103        + 10 (477) = 113   <- TASK-477 reported 113
113        +  7 (478) = 120   <- counted in the working tree now
```

**(b) ARG-COUNT CHECK OVER EVERY CALL, with the untouched majority as the positive control on the checker (§14):**

```
calls checked            : 120
MISMATCHES (must be 0)   : 0
of which UNTOUCHED       : 77      <-- these balance, so the checker works
```

**(c) LEDGER A — sha256 of every symbol NO task in the 476 → 477 → 478 chain touched. HEAD is a real committed
baseline for these, so `IDENTICAL` here is unfakeable:**

```
SpikeFixtureT0        IDENTICAL  cb1f3b283d0ed1be   <-- 🔒 t0's BYTES DO NOT MOVE (§16 freeze #1)
SpikeFixtureT1        IDENTICAL  74e4ce6f97f3c8b2
SpikeRoster           IDENTICAL  3993614450c2af9b   <-- the vocabulary authority, unmoved
SpikeRosterT1         IDENTICAL  26986420d4608f07
SpikePlaces           IDENTICAL  ea5e0e5def7d0115
FSpikeWorldFixture    IDENTICAL  e5c9f9d472735998   <-- no field added; no fixture literal had to move
FSpikeRosterRow       IDENTICAL  5ac883064f701985
AppendZoneA           IDENTICAL  824f92834ce74a93   <-- the seam scan READS it, never edits it
AppendZoneB           IDENTICAL  fcee10f6f1011037
AppendZoneC           IDENTICAL  400f530ecdd2168f   <-- the hardcoded other_kinds is untouched (D2)
BuildPrompt           IDENTICAL  90d6098a731c1176
TokenizePrompt        IDENTICAL  68575e2340799930
SanitizeForPrompt     IDENTICAL  1b84c43a0caf671d
ScoreRow              IDENTICAL  d09786fd99c36359
LoadCorpus            IDENTICAL  20630213773e88c4
AppendRule            IDENTICAL  2867510987d0e5ca
IsLegalGbnfRuleName   IDENTICAL  688c3ddcb5e897f5   <-- the at-least / at_least charset guard, unmoved
ResolveZoneAText      IDENTICAL  12acbd1f3b61987c
SpikeBenchUtterances  IDENTICAL  a2d4e279f5db7da7
ALL IDENTICAL: True
```

**(c2) LEDGER B — the symbols 476/477 changed, AND ITS LIMIT IS STATED RATHER THAN GLOSSED.** ⛔ **There is no committed
baseline for "as 477 left it" — both tasks are uncommitted — so a sha256 here would prove nothing about 478 and I did
not manufacture one by reverse-applying my own edits.** What is checkable is containment: **none of 478's identifiers
appears inside any of them** (`CmdSpikePrompt` · `ParseOptions` · `RunOneSplit` · `RunSplitRepeated` ·
`FormatTokenIdSpan` · `StartJob` · `FSpikeOptions` · `SummariseIntSeries` — all `NONE`).
⚠️ **This proves nothing was ADDED, not that nothing was REMOVED** — (a)'s zero-unaccounted ledger is what covers
removal, since a dropped pre-existing line would orphan a HEAD format string.
✅ **The probe carries its own positive control (§14):** run against the three functions 478 **did** edit, it returns
`CmdSpikeGrammar → 7 hits`, `RunEvalJob → 3`, `RunBenchJob → 2`. **It detects what it is looking for.**
📌 **`CmdSpikePrompt` is listed as *"1 string swept"*, not as clean** — a check whose label hid §3 #4 would be its own
defect.

**(d) 🔒 THE ACCEPTANCE CRITERION, MECHANICALLY: THE EMITTED GRAMMAR IS UNMOVED FOR EVERY FIXTURE THAT EXISTS.**

```
HEAD SpikeRoster   : 13 kinds [footman archer knight miner militiamob pikeman sapper cavalry longbowman cleric ogre wizard sorcerer]
CUR  SpikeRoster   : identical to HEAD = True
CUR  SpikeRosterT1 : identical to HEAD = True
t0 kind list == SpikeRoster's : True   (t0.Roster IS SpikeRoster, by construction -- identity, not merely subset)
t1 kind list == SpikeRoster's : True   <-- so BuildSpikeGrammar(t1) is byte-unmoved too
```

⇒ **The `kind` rule's emitted text is character-identical for t0 AND for t1, so every generation on record would
produce a byte-identical grammar.** ⚠️ **The board's criterion named only `t0`; `qa/TASK-500.md` NIT-6 flagged that `t1`
is invisible in the fine-tune law and that "a subset-parity change that overlooks it is a real seam." It was not
overlooked — t1 is covered on all four limbs** (sha256, kind-list identity, the static_assert, the seam gate).

**(e) THE SEAM READOUT CANNOT EMIT A LINE ON t0 OR t1 — the gate is enforced by a `static_assert`, not by my reading:**

```
Zone A DEMONSTRATES ("kind":"X")       : [archer, footman, sorcerer]   <-- §9c's three names, read off the artifact
Zone A NAMES (SpikeRoster kinds)       : 13 of 13
named kinds absent from t0 / t1        : 0 / 0
demonstrated kinds absent from t0 / t1 : 0 / 0
```

⇒ ⛔ **On every fixture that exists, `ReportZoneAKindSeam` returns before logging. Zero new lines on `SpikeEval`'s and
`SpikeBench`'s default paths.** It first speaks on a Stage D subset fixture — which is precisely when D3 becomes
measurable.

**(f) STRUCTURE + ENCODING:** brace depth `0`, min depth `0`, paren balance `0` (strings and comments excluded);
**611 `TEXT()` literals scanned, 0 non-ASCII** (one `⛔` I had introduced into the help string was found by this check
and replaced — emoji stay in comments, matching the file's convention); **no BOM**; `4764 → 6185` lines.

**(g) COMMAND INVENTORY + HELP STRINGS, isolated mechanically rather than asserted:**

```
commands in HEAD=6 CURRENT=6            (none added, none removed; command NAMES identical)
Siege.Llama.SpikeBench    identical=True
Siege.Llama.SpikeLoad     identical=True
Siege.Llama.SpikeUnload   identical=True
Siege.Llama.SpikeEval     identical=False  old_is_PREFIX_of_new=True    575 -> 2314   (477's; NOT touched by 478)
Siege.Llama.SpikePrompt   identical=False  old_is_PREFIX_of_new=True    369 -> 1874   (477's; NOT touched by 478)
Siege.Llama.SpikeGrammar  identical=False  old_is_PREFIX_of_new=True    107 -> 1610   <-- 478's ONLY help change
```

**(i) ⛔ THE BOARD'S EXPLICIT PROHIBITION, DISCHARGED PER RULE.** Every GBNF rule block hashed HEAD-vs-now:

```
root command question ask intent where count at-least item selection who when : IDENTICAL
kind                                                                          : CHANGED   <-- the one rule this task may change
BOTH quantity loops still `SpikeGrammarCountMin .. SpikeGrammarCountMax`      : True
every `Fixture.` reference in BuildSpikeGrammar (8)                           : ALL inside the kind block; 0 outside
```

⇒ ⚖️ **`count` stays 1–30 and is never the live max. §1's "constrain identity hard, leave quantity soft" is intact —
`kind` is the identity side, which is exactly the side §1 says to constrain hard.**

**⚠️ WHAT (a)–(i) DO AND DO NOT ESTABLISH.** They prove things about **source bytes**. They are **not a run.**
⇒ **§16's INSTRUMENT-UNCHANGED proof is still owed at TASK-481: re-run `SpikeEval dev=` and show the same score with the
same failing-row identities. Nothing here substitutes for it.**

---

## 6. ⛔ THE DEFECT TASK-477 HANDED ME — I DID **NOT** FIX IT, AND ITS STATED REASON DOES NOT SURVIVE CHECKING

> **`CmdSpikePrompt` calls `AppendZoneA(ZoneA)` DIRECTLY instead of `ResolveZoneAText(Options)`, so `prompt=<path>` is
> SILENTLY IGNORED by `Siege.Llama.SpikePrompt`.**

**Confirmed at the artifact, not taken from the message (the RELAYED-DIAGNOSIS LAW).** `CmdSpikePrompt` reads
`FString ZoneA; AppendZoneA(ZoneA);` and then hands that same `ZoneA` to `BuildPrompt` and to `out=`.

**⚠️ §22 SWEEP OF THE SHAPE — a citation is a lower bound, so I swept the three Zone-A-consuming lanes:**

| lane | resolves Zone A via | honours `prompt=` |
|---|---|---|
| `Siege.Llama.SpikeBench` → `RunBenchJob` | `ResolveZoneAText(Options)` | ✅ |
| `Siege.Llama.SpikeEval` → `RunEvalJob` | `ResolveZoneAText(Options)` | ✅ |
| **`Siege.Llama.SpikePrompt` → `CmdSpikePrompt`** | **`AppendZoneA` directly** | ⛔ **NO** |

⇒ **Sweep result: 1 of 3. The other two are clean, and there is no fourth site.**

### ⚖️ 477's REASON FOR REFUSING IT IS NOT ACCURATE, AND WHOEVER BOARDS THE FIX SHOULD KNOW THAT

477 recorded *"I did not change it (it would move the default path)"*. **Checked at `ResolveZoneAText`: it does not.**
With `ZoneAOverridePath` empty the function skips its override branch entirely and returns
`FString ZoneA; AppendZoneA(ZoneA); return ZoneA;` — **the same bytes, and zero log lines** (both its `Display` and its
fallback `Warning` sit *inside* the override branch). ⇒ ⛔ **The one-token fix is provably default-identical; its ONLY
observable effect is on a command line that types `prompt=`, where today the flag does nothing at all.**

**⚠️ I STILL DID NOT MAKE IT, AND HERE IS THE HONEST REASON:** `#### TASK-478`'s spec is per-fixture GBNF and subset
parity; `CmdSpikePrompt` is not in its `names:` list; and my dispatch was explicit that an uncovered defect is to be
**named**, not quietly absorbed. **So it is named — with the mechanism, the sweep, and the cost estimate — and it is not
mine to close.**

**⛔ WHY IT DESERVES A TASK OF ITS OWN RATHER THAN A FOOTNOTE:** Stage D renders every gold training prompt through
`SpikePrompt out=`. A silently-ignored Zone A override there builds the training set **against the wrong prefix, with no
warning** — and per 477's own `out=` doctrine the consumer's length/sha256 assertions would **PASS**, because they
certify the file, not the prefix it was built from. ⚖️ **That is this batch's founding defect shape aimed at the data,
where it is invisible forever once it reaches the weights.**

**📌 A SECOND, SMALLER INSTANCE OF THE SAME FAMILY, FOUND WHILE SWEEPING:** `CmdSpikePrompt` resolves `fixture=` as
`FixtureName.Equals(TEXT("t1")) ? t1 : t0`, so **`fixture=t2` silently yields t0.** My new `fixture=` on
`Siege.Llama.SpikeGrammar` deliberately does **not** copy that: `ResolveSpikeFixture` reports an unknown name at
`Warning` and says which fixture it fell back to. ⚠️ **Two `fixture=` parsers with different strictness now exist in one
file** — the registry is the intended single home, and adopting it in `CmdSpikePrompt` is a one-line change I left with
the finding above rather than take unilaterally.

---

## 7. ⛔ UNVERIFIED WITHOUT A COMPILE — stated plainly

Nothing below is a known defect; each is a claim only a compiler settles, and I was ordered not to run one.

1. **`bool bSeen[SpikeRosterNum] = {};`** — `SpikeRosterNum` is a `static constexpr int32`, so the bound is a constant
   expression. **Read, not built.**
2. **`FString::Find(const FString&, ESearchCase::Type, ESearchDir::Type, int32)`** — the 4-argument form with a start
   position. ✅ **Not a guess: `BuildPrompt` in this same TU already calls `OutPrompt.Find(UserBlock,
   ESearchCase::CaseSensitive, ESearchDir::FromStart)`**, so the enum and the overload family are both live here. The
   4th-parameter form is the part not independently confirmed.
3. **`FString::Contains(const TCHAR*, ESearchCase::Type)`** — standard surface; not compiler-confirmed in this file.
4. **`FString::Equals(const TCHAR*, ESearchCase::Type)`** in `ResolveSpikeFixture` — the pre-existing
   `FixtureName.Equals(TEXT("t1"), ESearchCase::IgnoreCase)` in `CmdSpikePrompt` binds the same overload with the same
   argument type after array-to-pointer decay, which is the strongest evidence available short of a build.
5. **Static-init order for `SpikeFixtureRegistry`** — it takes the addresses of `SpikeFixtureT0` / `SpikeFixtureT1` and
   is **defined after both**, in the same TU. Address-taking of namespace-scope objects is constant initialisation, so
   there is no order hazard; **verified by reading, not by a build.**
6. **`UE_LOG` / `FString::Printf` argument TYPES.** Arg *counts* are machine-checked (§5b); the *types* are not.
   UE 5.8's `TCheckedFormatString` will check `%s`-vs-`TCHAR*` and `%d`-vs-`int32` at compile time — **all my format
   strings are literals**, which is what that mechanism requires.
7. **Unused-return warnings** — `ReportZoneAKindSeam` returns `int32` and two of its four call sites ignore it. It is
   not `[[nodiscard]]`, so this should not warn. **Unproven.**
8. **Declaration order** — `FixtureHasKind`, `CollectZoneAFewShotKinds`, `ReportZoneAKindSeam`, `ResolveSpikeFixture`
   and `ListSpikeFixtureNames` all sit **above** every caller (`BuildSpikeGrammar`, `RunBenchJob`, `RunEvalJob`,
   `CmdSpikeGrammar`). Verified by reading.
9. **⚠️ §32, SAID PLAINLY: NOT ONE NEW GUARD HERE HAS EVER BEEN OBSERVED TO FUNCTION.** The subset/duplicate/order/empty
   branches, the empty-roster grammar error, the eval-lane parity check and the seam readout are **all unreachable on
   t0 and t1 by construction** — which is exactly what makes the additive proof strong **and** what makes them untested
   assertions rather than tested safeguards. ✅ **The cheapest available discharge needs no model and no compile-gate
   slot: run `Siege.Llama.SpikeGrammar` and `Siege.Llama.SpikeGrammar fixture=t1` after TASK-481 builds, and confirm
   both dumps are identical and both report `ZONE_A_KIND_SEAM … CLOSED`. The first fixture that trips them is Stage D's.**

---

## 8. WHAT QA SHOULD SCRUTINISE (ranked)

1. **⛔ §2.1 — THE ORDER REQUIREMENT I KEPT.** The spec said *"permit a subset"*; I permitted *"a subset in
   `SpikeRoster`'s order"*. ⚖️ **Rule on it directly** — `qa/TASK-500.md` D3 already flagged the order strictness as
   TASK-478's design question. **My mechanism is cards.csv row order == what the shipped `Capture()` sorts to; if that
   does not hold, the third branch should go.**
2. **§1 — THE INVERSION.** Confirm at the artifact that deriving the grammar from the fixture makes clause (2)(a)
   *more* load-bearing. **If that reasoning is wrong, the whole shape of the new check is wrong.**
3. **§6 — THE `prompt=` DEFECT I DECLINED, AND MY CONTRADICTION OF 477's STATED REASON.** ⚠️ **Read
   `ResolveZoneAText` yourself and rule on whether the fix is default-identical.** If it is, this wants a task before
   Stage D renders a single gold prompt — **and please rule on whether I was right to leave it or should have taken it.**
4. **§3 #4 — I EDITED ONE STRING INSIDE TASK-477's `CmdSpikePrompt`.** A scope call. **Rule it in or out; reverting is
   one line.**
5. **§2.5 / §5e — THE SEAM READOUT'S GATE.** The whole additive claim for `SpikeEval` and `SpikeBench` rests on
   *"it cannot fire on t0 or t1."* **Re-derive it** (`verify_478.py` in the scratchpad is re-runnable against
   `git show HEAD:GitClaudeUnrealTesting-prefixed path` — ⚠️ **note the toplevel trap in §5**).
6. **§5(c2) — LEDGER B IS WEAKER THAN LEDGER A AND SAYS SO.** Decide whether containment plus the zero-unaccounted
   format ledger is enough to believe 476's and 477's work is unmoved, given no committed baseline exists for it.
7. **§2.3 — emitting a knowingly-unparseable grammar rather than substituting a working one.** I copied `AppendRule`'s
   idiom; confirm that is the intended reading of it.
8. **§7.9 — nothing new here has ever been executed.** Per §32 that is a real limitation of this delivery, not a
   formality.

---

## 9. STATUS — ⭐ STAGE A's SERIAL CHAIN IS CLOSED

Board set to **`ready-for-qa`** (gate **TASK-480**). ⛔ Not marked passing by me.

**✅ `SiegeLlamaSpike.cpp` IS FREE. 476 → 477 → 478 is complete; no task in this chain left anything half-done, and
there is no fourth writer queued behind me.**

**⚠️ WHAT "STAGE A's INSTRUMENT SET IS COMPLETE" DOES AND DOES NOT MEAN — stated plainly because I am the last of the
three and somebody will read this as a green light:**

- ✅ **COMPLETE AS SPECIFIED.** All three board entries' deliverables exist in source: `repeats=` (476), `out=` / `ids=`
  / the measured `wire=` (477), per-fixture GBNF + subset parity (478). **M1 is two commands and needs no further code.**
- ⛔ **NOT COMPLETE AS MEASURED. NOT ONE LINE OF STAGE A HAS BEEN COMPILED OR EXECUTED.** Three handoffs of mechanical
  proofs about *source bytes* are not a run, and **§16's instrument-unchanged obligation — re-run the dev split, show
  the same score with the same failing rows — is owed at TASK-481 and is unpaid.** ⚠️ **Until it is paid, "the
  instrument is unmoved" is an argument, not a measurement.**
- ⛔ **THREE NAMED GAPS SURVIVE STAGE A, ALL BOARD-LEVEL, NONE MINE TO CLOSE:**
  1. **`prompt=` is silently ignored by `Siege.Llama.SpikePrompt`** (§6) — ⚠️ **the one that touches Stage D's data.**
  2. **No `wire=` on `SpikeEval`** (477 §5.5) — the A/B must be validated on `SpikePrompt` first.
  3. **D2 is unexpressible by the spike at all** — `other_kinds: none` is hardcoded; escalated, TASK-486.
- 📌 **AND ONE THING STAGE A NOW MAKES POSSIBLE THAT IT COULD NOT BEFORE:** a Stage D fixture author adds a row to
  `SpikeFixtureRegistry`, and `Siege.Llama.SpikeGrammar fixture=<name>` shows the resulting grammar and the Zone-A seam
  **with no model loaded and no measurement spent.**
