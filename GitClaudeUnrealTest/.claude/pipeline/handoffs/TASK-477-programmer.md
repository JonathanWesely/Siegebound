# TASK-477 — [FT-A2] `out=<path>` + `ids=1` on `SpikePrompt`; `chat=0|1` **verified already present**, not re-added

- **Agent:** gameplay-programmer
- **Date:** 2026-08-03
- **Files touched:** `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp` — **one file, nothing else.**
  ⛔ No header, no new file, no `.Build.cs`, no include added (`Misc/FileHelper.h` and `Misc/Paths.h` were already there; `HAL/FileManager.h` arrives transitively through `Misc/FileHelper.h:12`).
- **Compile:** ⛔ **NONE RUN**, per the spec. Two compile gates this batch (TASK-481, TASK-492). Everything a compiler would settle is in §6, labelled.
- **Git:** ⛔ **none.** No add, no commit, no branch.
- **Editor / MCP / PIE:** ⛔ **none.** No model was loaded. ⚠️ **No number in this file is a measurement** — §4 is a proof about *bytes of source*.
- **Gate:** TASK-480. **M8 declaration: this task adds no replicated property, no new replicated class, and no new relevancy tier.**

---

## 1. ⭐ THE HEADLINE — SPEC CLAUSE (3) WAS ALREADY SATISFIED, AND I DID NOT RE-ADD IT

**TASK-476 reported that `chat=0|1` already exists. I confirmed it at the artifact rather than taking the message
(the RELAYED-DIAGNOSIS LAW), and it is correct on every limb.** Four mechanisms, each read directly:

| Claim | Where, by symbol | Verified |
|---|---|---|
| The field exists and defaults to **true** | `FSpikeOptions::bUseChatTemplate` | ✅ `= true` |
| It is parsed in the **shared** parser | `ParseOptions` → `GetArgBool(Args, TEXT("chat"), Options.bUseChatTemplate)` | ✅ |
| ⇒ it is therefore live on **`SpikeEval`** | `CmdSpikeEval` → `ParseOptions(Args)` on its first line | ✅ |
| ⇒ and live on **`SpikePrompt`** | `CmdSpikePrompt` → `ParseOptions(Args)` on its first line | ✅ |
| `BuildPrompt` honours it | `Options.bUseChatTemplate && GRunner.Model ? llama_model_chat_template(...) : nullptr`, then `system`=Zone A / `user`=Zone B+C / `add_generation_prompt=true`, with a raw-concat fallback | ✅ |

⇒ ⛔ **RE-ADDING IT WOULD HAVE SHIPPED A DUPLICATE FLAG.** §15's call, made here and owned here: **the deliverable is
satisfied by an existing mechanism, and the correct response is to cite the mechanism and refuse the spec line.**

**✅ THE PAYOFF, WHICH IS WHY THIS MATTERS BEYOND THE FLAG: M1 IS EXACTLY TWO COMMANDS.**
`Siege.Llama.SpikeEval chat=1 repeats=5` then `Siege.Llama.SpikeEval chat=0 repeats=5`. **No code change stands between
this batch and STOP 1** — the measurement that can end the whole batch, and per §2 ending there is the *best available
result*, not a failure.

### ⛔ BUT THE FLAG WAS NOT **ENOUGH**, AND THAT IS THE REAL FINDING OF THIS TASK

A verification that stops at *"the flag works"* would have shipped a green describing something else. **`chat=` records
an INTENTION; only the produced bytes record the wire format**, and the two can differ **silently**:

> `BuildPrompt` takes the raw-concatenation branch whenever `llama_model_chat_template(...)` returns `nullptr` — i.e.
> **when the loaded GGUF carries no chat template — and that branch logs NOTHING.** (The *other* fallback, a failing
> `llama_chat_apply_template`, does warn. **The silent one is the dangerous one.**)

⇒ **A `chat=1` run can produce raw bytes.** An A/B that was secretly **raw-vs-raw** would measure a difference of zero
and read as *"D1 does not exist"* — ⛔ **the one conclusion this measurement must never reach by accident**, and exactly
what TASK-481's step (3) is told to treat as a major finding. So the residual work of clause (3) was **not to add a flag
but to make the flag checkable**, which is §14's *prove the positive* applied to a knob instead of a search:

- **`SPIKE_PROMPT_OUT` now prints `wire=raw|chat` MEASURED off the produced bytes**, beside `chat_requested=`,
  `model_has_template=` and `prompt_starts_with_zoneA=`. **The label is never copied from the flag.**
- **The discriminator is a positive structural fact, not an absence:** both of `BuildPrompt`'s raw branches emit
  `ZoneAText + ZoneB + ZoneC`, so **Zone A sits at offset 0**; any chat template opens with a role marker *before* the
  system content, so it cannot. ⇒ `FullPrompt.StartsWith(ZoneA)` **iff** the bytes are raw. ⚠️ **It also catches the
  apply-template failure path, which a `model_has_template` check alone would miss.**
- **`chat=1` that came out raw fires a WARNING naming both possible causes** and saying the run may not be quoted as the
  templated half of an A/B.

---

## 2. ⛔ THE ANSWER THE SPEC DEMANDED IN WRITING — IS `chat=0` BYTE-IDENTICAL TO `ComposeTurnPrompt`?

> ### ⚖️ **THE ASSEMBLY RULE IS BYTE-IDENTICAL. THE RESULTING PROMPT BYTES ARE NOT, AND STRUCTURALLY CANNOT BE — AND THE ENTIRE RESIDUAL IS D2/D3, WHICH IS M3's JOB, NOT THIS FLAG'S.**

**Lines I read, named as the spec required** (`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp`,
`USiegeAssistantComponent::ComposeTurnPrompt`, **read-only — I do not own that file, and TASK-479 is live in it**):

```
2739   const FString& ZoneA = GetCachedZoneA();
2740   const FString ZoneB = Snapshot->BuildZoneB();
2741   const FString ZoneC = Snapshot->BuildZoneC(Utterance, BuildPendingLine());
2743   ReportFirstCapture(ZoneA, ZoneB, ZoneC);
2745-2749  FString Prompt; Prompt.Reserve(ZoneA.Len() + ZoneB.Len() + ZoneC.Len());
           Prompt += ZoneA; Prompt += ZoneB; Prompt += ZoneC;
2751   return Prompt;
```

against the spike's `chat=0` path in `BuildPrompt`:

```
   const FString UserBlock = ZoneB + ZoneC;
   ...
   OutPrompt = ZoneAText + UserBlock;      // the TemplateText == nullptr branch
```

✅ **THE TWO ASSEMBLY RULES AGREE ON EVERY DEGREE OF FREEDOM THERE IS:** the same three parts, **the same A → B → C
order**, ⛔ **no separator, no delimiter, no role marker, no prefix, no suffix, no trailing newline added by either.**
There is nothing left for them to disagree about at the assembly seam. **This is equivalence established by reading two
sites, and it is exact rather than approximate, because the operation is a bare concatenation.**

⛔ **THE BYTES ARE A DIFFERENT QUESTION AND THE HONEST ANSWER IS NO — BY CONSTRUCTION, NOT BY DEFECT.** The two lanes
feed that identical rule from **different zone builders**:

| Zone | spike (`chat=0`) | shipped (`ComposeTurnPrompt`) | the gap |
|---|---|---|---|
| A | `AppendZoneA` — a transcribed constant | `Snapshot->BuildZoneA(GetVocabulary())` | the measured **3029-vs-5116** vocabulary-lane defect |
| B | `AppendZoneB(fixture)` | `Snapshot->BuildZoneB()` — live capture | fixture vs live board |
| C | `AppendZoneC(fixture, ...)`, **`other_kinds: none` HARDCODED at line 632**, all 13 kinds | `BuildZoneC`, capped at `MaxRosterKinds = 8`, collapse **computed** | **D2**, and per CONVENTIONS §3 the spike **cannot express collapse at all, on any fixture** |

⇒ ⚖️ **THE DISTINCTION THAT MATTERS FOR THE DECISION RULES: `chat=` CONTROLS THE WIRE FORMAT AND NOTHING ELSE, AND THE
WIRE FORMAT IS EXACTLY WHAT STOP 1 IS ABOUT. STOP 1 IS THEREFORE MEASURABLE TODAY.** The zone-content gap is **STOP 2's
subject**, it is already escalated (D2 → TASK-486, Jonathan's), and it is measured by **M3's artifact diff**, not by this
flag. ⛔ **Closing it here was not an option and is not a hidden shortfall:** the spike would have to call the shipped
builders across a UBT module boundary, or re-implement them — **the forbidden reimplementation, which would guess the
vocabulary lane and produce a green M3 that measures nothing (T1 and T8 in one move).**

📌 **Why this is a FINDING and not the spec's "STOP AND SAY SO".** The spec's stop is for *"you cannot make `chat=0`
byte-identical"* being a **surprise**. It is not a surprise: it is D2 + D3, already law, already boarded, already owned by
another task, and **the part `chat=0` was actually asked to reproduce — the shipped SHAPE — it reproduces exactly.**
⚠️ **Recorded here at full strength anyway, because "equivalent-by-reading on the assembly rule, NOT byte-identical on
the content" is precisely the sentence TASK-480's criterion (3) exists to extract, and QA is told to derive it itself.**

---

## 3. WHAT WAS ACTUALLY ADDED — the two genuinely-new flags

| Spec | Delivered | Where (by **symbol** — §18c: every line number predating today is stale) |
|---|---|---|
| **(1)** `out=<path>` on `SpikePrompt` | writes the assembled prompt's exact bytes | parsed in `ParseOptions`; written in the new trailing block of `CmdSpikePrompt` |
| **(2)** `ids=1` on `SpikePrompt` | the engine's `llama_tokenize` id sequence, indexed chunks | same block; `FormatTokenIdSpan()` + `SpikePromptIdsPerLine` |
| **(3)** `chat=0|1` on both | ⛔ **ALREADY PRESENT — VERIFIED, NOT RE-ADDED.** Delivered instead: the measured `wire=` readout, the silent-fallback WARNING, and the documentation that was missing from both help strings | §1 above |

**⛔ `out=` writes UTF-8 WITHOUT A BOM, and that is not a style choice.** `TokenizePrompt` hands `llama_tokenize` the
`FTCHARToUTF8` of this exact `FString`, so **these bytes are byte-for-byte what the tokenizer saw.** ⚠️ **`FFileHelper`'s
default encoding is `AutoDetect`, which writes UTF-16-with-BOM the moment the prompt contains one non-ASCII character** —
a file that still opens correctly in an editor, still has a plausible length, and **hashes to something the consumer can
never reproduce.** Hence an explicit `TArray64<uint8>` + `SaveArrayToFile`, never `SaveStringToFile`.

**⚠️ THE `sha256` IS DELIBERATELY NOT COMPUTED ENGINE-SIDE.** The consumer asserts it **over the file it actually reads**,
which is the only place the assertion means anything — **a hash printed by the writer certifies the writer.** The printed
`utf8_bytes=` is the cheap disagreement detector, and it is the **byte** count, printed beside the **char** count so the
two can never be confused. *(Also: no platform SHA-256 call is introduced into a file with one compile gate.)*

**⚠️ `ids=` REUSES the token array that produced `assembled_total_tok`** rather than re-tokenizing. The ids are then
**provably the same sequence** that produced the figure on the `SPIKE_TOKENS` line above them, instead of a second
reading that merely ought to agree. **`add_special=1 parse_special=1` is printed beside them** — ⛔ **a Python count taken
under different flags is a different measurement, and comparing them would manufacture a divergence or hide one.**
Chunked at **32 ids/line**: a templated Zone A alone is ~1362 tokens, and a truncated single line would fabricate a
divergence at exactly the index the log gave up.

---

## 4. 🔒 THE ADDITIVE PROOF — MECHANICAL, AND STRICTLY STRONGER THAN TASK-476's

**The mechanism is simpler than 476's, so the claim is stronger: I inserted ZERO characters into any pre-existing format
string.** Every new line is on a **new tag stream** (`SPIKE_PROMPT_OUT` / `SPIKE_PROMPT_IDS`) or on `SPIKE_WARN`, and
every one of them is inside a block gated on `out=`/`ids=` being **typed**.

**(a) Every HEAD format string still exists, character-identically.** Script compares `git show HEAD:` against the
working tree, so it re-derives **476's** proof and mine in one pass (⚖️ the RELAYED-DIAGNOSIS LAW applied to 476's
handoff rather than trusting it):

```
HEAD UE_LOG calls              : 87
CURRENT UE_LOG calls           : 113
  survive CHARACTER-IDENTICAL  : 79
  survive via 476's tag insert : 8      (split=%s -> split=%s%s, TASK-476's documented rewrite)
  ORPHANED (must be 0)         : 0
```

⇒ ⛔ **BETWEEN HEAD AND NOW, THE ONLY MODIFICATIONS TO ANY PRE-EXISTING `UE_LOG` FORMAT STRING ARE TASK-476's EIGHT.
TASK-477 MODIFIED NONE.** ✅ **And `113 − 10 (mine) = 103`, which independently corroborates 476's own reported total.**

**(b) Arg-count check over EVERY call, with the untouched majority as the positive control on the checker (§14):**

```
calls checked            : 113
MISMATCHES (must be 0)   : 0
of which UNTOUCHED       : 79      <-- these balance, so the checker works
```

**(c) The frozen things are frozen — `sha256` of each symbol's full text, HEAD vs now:**

```
SpikeFixtureT0        IDENTICAL  d8b28b27cd5cab1b     <-- 🔒 t0's BYTES DO NOT MOVE. holdout2 is authored against it.
SpikeFixtureT1        IDENTICAL  e0a498cbb190efb5
AppendZoneA           IDENTICAL  11c0ae812370d510  (9087 bytes)
AppendZoneB           IDENTICAL  7959f232c120b4b5
AppendZoneC           IDENTICAL  03380939c9a133e4
BuildSpikeGrammar     IDENTICAL  1d0975f46bda965c     <-- TASK-478's, untouched
VerifyFixtureKindParity IDENTICAL b02ce0eb06895f5e    <-- TASK-478's, untouched
BuildPrompt           IDENTICAL  e127c4a03801d964     <-- ⛔ THE chat= IMPLEMENTATION WAS READ, NEVER EDITED
TokenizePrompt        IDENTICAL  289396cb40ffe1cb
RunGeneration         IDENTICAL  d0259159d42a0ab6
SanitizeForPrompt     IDENTICAL  a32ab7e97af9794d
CmdSpikeGrammar       IDENTICAL  65d201ee0504a896
ALL IDENTICAL: True
```

**(d) The two help strings I changed, isolated mechanically rather than asserted:**

```
commands in HEAD=6  CURRENT=6            (none added, none removed; command NAMES identical)
Siege.Llama.SpikeBench    identical=True
Siege.Llama.SpikeGrammar  identical=True
Siege.Llama.SpikeLoad     identical=True
Siege.Llama.SpikeUnload   identical=True
Siege.Llama.SpikeEval     identical=False  old_is_PREFIX_of_new=True    575 -> 2314
Siege.Llama.SpikePrompt   identical=False  old_is_PREFIX_of_new=True    369 -> 1874
```

⇒ **exactly two changed, both PURE APPENDS** (the old text is a literal prefix of the new), **nothing edited mid-string.**

**(e) The gate itself, by enumeration — all 10 new `UE_LOG` calls and what makes each unreachable by default:**

| # | line | stream | gate |
|---|---|---|---|
| 1 | `ParseOptions` | `SPIKE_WARN` | inside `if (GetArgValue(Args, TEXT("out"), …))` — `out=` literally typed |
| 2 | `StartJob` | `SPIKE_WARN` | `if (!PromptOutPath.IsEmpty() \|\| bDumpTokenIds)` |
| 3 | `CmdSpikePrompt` guard | `SPIKE_PROMPT_OUT` | same condition |
| 4–10 | `CmdSpikePrompt` tail | `SPIKE_PROMPT_OUT` / `SPIKE_PROMPT_IDS` / `SPIKE_WARN` | all inside the one trailing `if (!PromptOutPath.IsEmpty() \|\| bDumpTokenIds)` block |

**And the fields can only become non-default if the flag was typed** — every reference enumerated by grep: the two
declarations, **two assignments** (`Options.PromptOutPath = PromptOutText;` inside the presence-and-non-empty branch, and
`GetArgBool(Args, TEXT("ids"), …)` which assigns only when the arg is present and non-empty), and six reads, all of them
the gate condition or inside it. ⇒ ⛔ **A command line passing neither `out=` nor `ids=` cannot emit one new byte.**

**(f) Structure + encoding:** brace depth `0`, min depth `0`, paren depth `0` (strings and comments excluded);
**578 `TEXT()` literals scanned, 0 non-ASCII** (emoji stay in comments, matching the file's convention); **no BOM.**

**(g) ⚠️ THE PLACEMENT IS PART OF THE PROOF, NOT A TIDINESS CHOICE.** The new block is appended **after every
pre-existing line** in `CmdSpikePrompt`, so **no existing line changed POSITION in the stream** — the half of "identical
output" that a format-string comparison alone would not catch.

**⚠️ WHAT (a)–(g) DO AND DO NOT ESTABLISH.** They prove things about **source bytes**. They are **not a run.**
⇒ **TASK-481 still owes §16's INSTRUMENT-UNCHANGED proof by re-running the dev split and showing 20/25 with the same five
failing rows. Nothing here substitutes for it.**

---

## 5. ⚠️ THE JUDGEMENT CALLS, DECLARED — flag each to QA rather than let them be found

1. **⛔ `out=`/`ids=` REFUSE without a resident model, at `Error` level, instead of returning quietly.** ⚖️ **The dump is
   not merely *unavailable* there — writing it would be actively WRONG.** With no model, `BuildPrompt` cannot reach
   `llama_model_chat_template` and **silently emits raw concatenation**, so `out=` would produce a file that is the raw
   wire format while the command line said `chat=1`. **A consumer asserting length and sha256 against that file would
   PASS** — T1 and T8 in one move. The refusal names the reason and the fix.
2. **⛔ I deliberately did NOT special-case `chat=0`, which genuinely needs no model at all.** It would work, and it
   would create **a SECOND site that writes the artifact.** *"Exactly one composer"* is the rule this whole batch exists
   to defend; **one write site, always, even at the cost of a redundant model load.**
3. **The two help strings.** Following **476's precedent and inheriting its QA item #3**: the console *registration
   blurb* is outside §16's frozen *measurement output* contract, and an undocumented flag is the worse defect. ⛔ **If QA
   rules the other way, both revert on their own** — nothing else depends on them.
4. **⛔ NO sidecar `.ids` FILE, though M4 would find one convenient.** `out=`'s contract is *the prompt bytes and nothing
   else*, and inventing a second artifact **filename** is a naming decision — **§5's naming law pins every name before
   its task issues, and it pins `out=<path>` / `ids=1` / `chat=0|1` and no third artifact.** The ids go to the log,
   indexed, so they are recoverable without a new pinned name.
5. **⛔ NO `wire=` INDICATOR WAS ADDED TO `SpikeEval`, AND THIS IS THE GAP I MOST WANT READ.** It would be genuinely
   valuable — but a new line on `SpikeEval`'s **default** path moves the instrument every number on record was taken
   with (§16 freeze #2, unconditional). ⇒ **Refused, and the refusal is written into `SpikeEval`'s own help text**,
   which directs the operator to take the wire reading with `SpikePrompt` first. ⚠️ **TASK-481 and whoever runs M1 must
   do exactly that: confirm `wire=chat` and `wire=raw` on `SpikePrompt` BEFORE trusting an A/B taken on `SpikeEval`.**
6. **An empty `out=` is refused out loud** rather than treated as absent — a typo whose whole effect is that no file
   appears while the command completes normally. Same *"reject loudly, never proceed quietly"* treatment as `deadline=`
   and `repeats=`.
7. **A §20 catch on my own first draft, recorded because the wrong version would have looked correct.** I initially used
   `FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Path)`. **`FPaths::ProjectDir()` is itself relative in an
   editor build** (`../../../<Project>/`), and the two-argument overload only **joins and collapses** — it does not force
   absoluteness when the base is relative (verified in `Engine/Source/Runtime/Core/Private/Misc/Paths.cpp`,
   `ConvertRelativePathToFullInternal`). ⚠️ **It would have opened the right file**, because `IFileManager` anchors
   relative paths at `BaseDir` — **and printed a `../../../` path, destroying the one job that line has: being the
   evidence of exactly what was written.** Now: join to `ProjectDir()`, then the **one-argument** overload.

---

## 6. ⛔ UNVERIFIED WITHOUT A COMPILE — stated plainly

Nothing below is a known defect; each is a claim only a compiler settles, and I was ordered not to run one.

1. **`FFileHelper::SaveArrayToFile(const TArray64<uint8>&, const TCHAR*)`** — the exact overload is at
   `Engine/.../Misc/FileHelper.h:190`; `TArray64<uint8>` binds it by identity rather than through the `TArrayView64`
   overload at `:185`, so overload resolution should be unambiguous. **Read, not built.**
2. **`IFileManager::Get().MakeDirectory(Path, /*Tree=*/true)`** — `IFileManager` reaches this TU transitively via
   `Misc/FileHelper.h:12`. ⚠️ **A transitive include is a real dependency but a fragile one**; if the build disagrees,
   the fix is one `#include "HAL/FileManager.h"`.
3. **`TArray64<uint8>::Append(const uint8*, int64)`** fed an `int32` length, and `FTCHARToUTF8::Length()`'s exact return
   type. Both standard UE surface; the same `Utf8.Get()/Utf8.Length()` pair is already used by `TokenizePrompt` **in
   this file**, which is the strongest available evidence short of a build.
4. **`FString::StartsWith(const FString&, ESearchCase::Type)`** — passed explicitly per §13.
5. **`llama_model_chat_template(model, nullptr)`** called a second time in this TU. Declared at
   `ThirdParty/LlamaCpp/include/llama.h:631` and already called by `BuildPrompt`; it is a metadata getter, so the second
   call is expected to be free and side-effect-free. **Not proven.**
6. **`FormatTokenIdSpan`'s declaration order** — it sits immediately above `CmdSpikePrompt`, its only caller. Verified by
   reading.
7. **`%d` fed `Utf8.Length()`** — arg *counts* are machine-checked (§4b); the *types* are not.

---

## 7. ⚠️ COLLISION BOUND FOR TASK-478 — the file is yours next

**Same file, strictly serial (ruling 11): 476 → 477 → 478. Nothing below was left half-done.**
⛔ **Verify by SYMBOL, never by offset — the file went 4764 → 5643 lines today and every pre-476 line number is stale.**

**NEW symbols (all confirmed free against a positive control, §14 — `grep` on the working tree, with `TagEvalRepeat`
returning 13 hits to prove the disk read sees 476's still-uncommitted work):**
`TagPromptOut` (= `"SPIKE_PROMPT_OUT"`) · `TagPromptIds` (= `"SPIKE_PROMPT_IDS"`) · `SpikePromptIdsPerLine` ·
`FormatTokenIdSpan()` · `FSpikeOptions::PromptOutPath` · `FSpikeOptions::bDumpTokenIds`

**MODIFIED symbols:**

| Symbol | What changed | Collision note for TASK-478 |
|---|---|---|
| `CmdSpikePrompt()` | ⚠️ **the largest change** — a gated refusal added *inside* the existing model guard, and a new block **appended after the last pre-existing line**. Signature unchanged. | 478 does not own this function |
| `ParseOptions()` | one contiguous insertion **immediately after 476's `repeats=` block**, before the `Clamp` lines | ⚠️ **478 appends AFTER mine if it needs a flag** |
| `FSpikeOptions` | two fields appended at the **end**, after 476's `EvalRepeats`; a **comment-only** block added above the pre-existing `bUseChatTemplate` | ⚠️ **append after mine** |
| `StartJob()` | one warning block after 476's `repeats=` warning | low |
| `GSiegeLlamaSpikePromptCommand` / `GSiegeLlamaSpikeEvalCommand` | help strings **appended to** | low — see §5.3 |
| tag block | two constants between `TagEvalRepeat` and `TagWarn` | low |

**⛔ UNTOUCHED AND VERIFIED IDENTICAL BY sha256 (§4c) — the whole of TASK-478's surface is clean:**
`SpikeFixtureT0` · `SpikeFixtureT1` · `BuildSpikeGrammar` · `VerifyFixtureKindParity` · `SpikeRoster` ·
`AppendZoneA/B/C` · `BuildPrompt` · `TokenizePrompt` · `RunGeneration` · `SanitizeForPrompt` · `CmdSpikeGrammar` ·
`ScoreRow` · `LoadCorpus` · `RunOneSplit` / `RunSplitRepeated` (476's, untouched by me).

📌 **A NOTE 478 WILL WANT: `CmdSpikePrompt` calls `AppendZoneA(ZoneA)` DIRECTLY, not `ResolveZoneAText(Options)`** — so
`prompt=<zoneA path>` is honoured by the eval and bench lanes but **NOT** by `SpikePrompt`. ⛔ **I did not change it**
(it would move the default path). ⚠️ **Consequence for anyone rendering training prompts: `SpikePrompt out=` always dumps
the transcribed Zone A constant, and silently ignores `prompt=`.** Recorded as a **finding, not a fix**.

---

## 8. WHAT QA SHOULD SCRUTINISE (ranked)

1. **⛔ §2 — the `chat=0` verdict, which TASK-480 criterion (3) says you may NOT take from this handoff.** Read
   `ComposeTurnPrompt` yourself and rule on *"byte-identical assembly rule, NOT byte-identical bytes"*. ⚖️ **The
   distinction is the whole of STOP 1's validity, and I have deliberately split it in two rather than answering yes or
   no.**
2. **§1 — the §15 refusal to re-add `chat=`.** Confirm the four mechanisms at the artifact. **If any limb is wrong, the
   deliverable is genuinely missing and this task under-delivered.**
3. **§5.5 — the missing `wire=` on `SpikeEval`.** The most consequential gap in this task, refused on §16 grounds.
   ⚠️ **If QA thinks M1 cannot be trusted without it, that is a finding for the board, not a code fix I may make.**
4. **§5.1/§5.2 — refusing the dump without a model, and refusing to special-case `chat=0`.** Both are *"do less"*
   decisions and both cost the operator a model load.
5. **§4 — re-derive the proof rather than accept it.** Scripts are in the scratchpad and re-runnable against
   `git show HEAD:`; the interesting number is **ORPHANED = 0**.
6. **The `wire=` discriminator itself** (`FullPrompt.StartsWith(ZoneA)`). ⚠️ **Per §32 this mechanism has never been
   observed to function and cannot be without a run** — it is an assertion, not a tested safeguard.
7. **§7's `prompt=` finding** — is silently ignoring `prompt=` on `SpikePrompt` acceptable, given Stage D renders gold
   prompts through this command?

---

## 9. STATUS

Board set to **`ready-for-qa`** (gate **TASK-480**). ⛔ Not marked passing by me.
⚠️ **TASK-478 must not start until this is gated** — same file, strict order 476 → 477 → 478.
