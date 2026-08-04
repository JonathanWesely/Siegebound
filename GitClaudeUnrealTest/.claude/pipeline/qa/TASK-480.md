# QA Report — TASK-480 (FT-A-GATE) — the Stage-A instrument, all four tasks in one report

**Verdict: PASS — 0 BLOCKER · 9 WARN · 5 NIT**

| Task | Verdict |
|---|---|
| **TASK-476** — `repeats=<N>` on `SpikeEval` | ✅ **PASS** |
| **TASK-477** — `out=` / `ids=` on `SpikePrompt`, `chat=` verified-not-re-added | ✅ **PASS** |
| **TASK-478** — per-fixture GBNF + subset parity | ✅ **PASS** |
| **TASK-479** — `DumpAssistantPrompt` + the dev-only accessor | ✅ **PASS** |

---

## ⛔ §0 — WHAT THIS PASS MEANS, STATED FIRST BECAUSE IT BOUNDS EVERY LINE BELOW

⛔ **NOT ONE LINE OF STAGE A HAS BEEN COMPILED OR EXECUTED.** ⇒ **A PASS HERE MEANS THE INSTRUMENTS ARE CORRECTLY *BUILT*. IT NEVER MEANS THE SHIPPED LANE HAS BEEN *MEASURED*, AND IT NEVER MEANS THEY WORK.**

- **§12g's standing WARN is NOT discharged.** No command has yet printed the shipped `BuildZoneA`. Every byte figure on record still comes from the spike's `AppendZoneA` — a different lane. TASK-479 built the instrument that will take that reading; **TASK-485 takes it.**
- **§16's INSTRUMENT-UNCHANGED proof is TASK-481's, not mine.** The dev-split number reproducing (**20/25**, same five failing rows `DEV-01 · DEV-04 · DEV-07 · DEV-16 · DEV-20`) is the *only* evidence these edits moved nothing, and it does not exist yet. ⚠️ **Until it is paid, "the instrument is unmoved" is an argument, not a measurement** — 478's own sentence, and it is correct.
- **§32 applies to nearly every new guard here.** The subset/duplicate/order/empty branches, the empty-roster grammar error, the eval-lane parity check, `ReportZoneAKindSeam`, the `wire=` discriminator, `bVerdictMovedWithoutBytes`, and all four of the accessor's refusal gates are **unreachable on `t0` and `t1` by construction.** That is exactly what makes the additive proof strong **and** what makes them untested assertions rather than tested safeguards. ✅ Criterion **(11)** confirmed: **all four handoffs say so themselves** — 476 §4, 477 §4, 478 §9, 479 §2.10 (verbatim: *"A PASS here means 'the instrument is correctly built', never 'the shipped lane has been measured.'"*).

---

## §1 — HOW I VERIFIED, AND ⛔ WHAT I COULD NOT (criterion 12, answered honestly)

⛔ **I HAVE NO SHELL AND NO GIT — BY ROLE DESIGN, AND THERE WAS NO BASH TOOL IN THIS SESSION.** Criterion (12) says *"re-run it yourself; do not take the count on relay."* **I could not run `git diff`, `git show HEAD:` or `<scratchpad>\verify_478.py`.** Stating that plainly rather than implying a re-run I did not perform is the whole point of the criterion.

**What I DID re-derive mechanically, each with its own positive control (§14):**

| Claim | Source | My re-derivation | Result |
|---|---|---|---|
| `UE_LOG` calls now = **120** | 478 §5(a) | `rg -c "UE_LOG\("` on the working tree | ✅ **120** — the ledger's endpoint is real |
| "**0 non-ASCII inside `TEXT()`**" in the spike | 478 §5(f) | `TEXT\("[^"]*[^\x00-\x7F]` | ✅ **0 hits.** ⭐ **Positive control: the identical pattern returns 34 hits in `SiegeCheatManager.cpp`** ⇒ the checker sees non-ASCII when it is there |
| the emitted grammar is **byte-unmoved for every fixture that exists** | 478 §5(d) | read `SpikeRoster` `:438-453` against `SpikeRosterT1` `:491-506`, and `SpikeFixtureT0` `:574-581` | ✅ **stronger than the script** — see §2 |
| `ComposeTurnPrompt` **still `private`** | 479 §2.4 | `rg` for access specifiers on the current header | ✅ **625 public < 1016 accessor < 1019 private < 1306 composer** |
| **zero `friend`** declarations | 479 §2.4 | same alternation | ✅ the only `friend` token is the word inside 479's own comment (`h:959`); positive control returned `public:`, `private:`, both symbols and the `#if` |

**⛔ WHAT REMAINS UNVERIFIED BY ME, WITH ITS OWNER NAMED — see WARN-1.** `87` (HEAD `UE_LOG` baseline) · the `+16 / +10 / +7` deltas · `ORPHANED = 0` · all sha256 ledgers · 479's `354 insertions / 0 deletions` · the help-string prefix claims. **All are git-dependent. TASK-481 has the shell.**

✅ **The toplevel trap 478 flagged did not reach me** — I never invoked git, so I could not have taken an empty `HEAD` for "the file is new". ⚠️ **It aims squarely at TASK-481, which will:** the toplevel is the **parent** of the project dir, so every path needs the `GitClaudeUnrealTest/` prefix and every checker needs a blob-size assertion before its zero is believed.

📌 **One phantom I chased and killed at the artifact, recorded because it is the criterion working:** a `rg -C` context render made `SiegeAssistantComponent.cpp:2414` appear to begin with `\` instead of `//` — a hard compile error if real. **Read directly: it is `// here on purpose:`.** No defect. ⚖️ **A tool artifact reported as a blocker would have cost a whole review cycle; §18c/§14's "verify at the artifact" is why it cost thirty seconds instead.**

---

## §2 — THE PRE-REGISTERED CRITERIA, ONE BY ONE

### ✅ (1) ADDITIVITY — the first and hardest, and it holds on all four counts

Every new flag's default is verified **at the declaration and at every read site**:

| Flag | Default | Verified |
|---|---|---|
| `repeats` | **1** | `SiegeLlamaSpike.cpp:2740` `int32 EvalRepeats = 1;` |
| `chat` | **1 (true)** | `:2688` `bool bUseChatTemplate = true;` |
| `out=` | **empty ⇒ nothing written** | `:2755` `FString PromptOutPath;` |
| `ids=` | **false** | `:2771` `bool bDumpTokenIds = false;` |

**The gates, read at the artifact:**
- **`repeats=1` prints nothing new.** `RunSplitRepeated` `:4795-4805` assigns `RepeatTag` **only inside `if (Repeats > 1)`**, and `:4812-4815` **returns before the entire distribution block** at `Repeats <= 1`. ✅ **And the empty-`FString` splice is safe, not merely intended:** `UE_LOG("split=%s%s", *SplitName, *RepeatTag)` with an empty `FString` — UE's `FString::operator*` yields `TEXT("")` for an empty string, never `nullptr` ⇒ **zero bytes contributed, no dereference hazard.**
- **`out=`/`ids=` are unreachable unless typed.** Parsed at `:5638-5651` / `:5653`; **`GetArgBool` `:5487-5494` and `GetArgValue` `:5445-5458` assign only when the key is present and non-empty** — I read them rather than assume. Every new emission sits inside `if (!Options.PromptOutPath.IsEmpty() || Options.bDumpTokenIds)` at `:5314` (StartJob), `:5830` (the refusal) and `:5885` (the tail block).
- **The tail block is appended AFTER every pre-existing line** in `CmdSpikePrompt` (`:5874-5885`) ⇒ **no pre-existing line changed POSITION in the stream** — the half of "identical output" a format-string comparison alone would miss. Verified by reading `:5742-5885` in order.
- **478's new lines cannot fire either.** `ReportZoneAKindSeam` `:888-955` **returns 0 without logging** when nothing is forbidden and `bReportWhenClosed == false` (`:937-946`); the eval and bench lanes pass **`false`** (`:4249`, `:4250`, `:5111`). The new eval-lane parity check `:5104-5109` logs **only on failure**.

### ✅ (2) `t0`'s BYTES — unchanged as far as any tool I hold can establish, and independently corroborated

⛔ **I cannot sha256 against `HEAD`.** What I *can* do is check `t0` against **three independent, older witnesses**, and all three agree:

1. **`SpikeFixtureT0` `:574-581` still reads `SpikeRoster, SpikeRosterNum`** — its roster IS `SpikeRoster` by identity, not by copy.
2. **The corpus premises still hold:** `SpikeRoster:440` `footman 8` (so DEV-03's *"you asked for 10 and 8 exist"* is a real shortfall) and `:442` `knight 3` (so DEV-11's deferred trigger is unsatisfied) — the two the fixture comment `:431-433` names as load-bearing.
3. **The in-file frozen assertion is intact:** `:5785-5790` still checks `ZoneB.Len() != 68 || ZoneC.Len() != 887` for the default order line.

⇒ **Nothing in the four tasks touches `SpikeFixtureT0`, `SpikeRoster` or the zone builders**, and 477 and 478 each independently ran a HEAD-vs-now hash on it. 🔒 **`holdout2`'s seal at `21f7e01` is not endangered by anything I found.** ⚠️ **But see WARN-2 — the three handoffs' hash ledgers are mutually incomparable, so "identical" here rests on two independent scripts agreeing, not on one reproducible fingerprint.**

### ✅ (3) `chat=0` vs `ComposeTurnPrompt` — ⛔ DERIVED BY ME AT BOTH ARTIFACTS, NOT TAKEN FROM THE HANDOFF

> ### ⚖️ **THE ASSEMBLY RULE IS BYTE-IDENTICAL. THE RESULTING BYTES ARE NOT, AND STRUCTURALLY CANNOT BE. 477's SPLIT ANSWER IS CORRECT AND I REACH IT INDEPENDENTLY.**

**Shipped** — `SiegeAssistantComponent.cpp:2739-2751`:
```cpp
const FString& ZoneA = GetCachedZoneA();
const FString  ZoneB = Snapshot->BuildZoneB();
const FString  ZoneC = Snapshot->BuildZoneC(Utterance, BuildPendingLine());
ReportFirstCapture(ZoneA, ZoneB, ZoneC);
FString Prompt; Prompt.Reserve(...); Prompt += ZoneA; Prompt += ZoneB; Prompt += ZoneC; return Prompt;
```
**Spike `chat=0`** — `SiegeLlamaSpike.cpp:3153` + `:3166`:
```cpp
const FString UserBlock = ZoneB + ZoneC;
OutPrompt = ZoneAText + UserBlock;     // the TemplateText == nullptr branch
```
✅ **Same three parts, same A→B→C order, bare concatenation on both sides.** ⛔ **No separator, no delimiter, no role marker, no prefix, no suffix, no trailing newline added by either.** There is nothing left at the assembly seam for them to disagree about. **This is exact, not approximate, because the operation is a bare concatenation** — and it is the reason **STOP 1 is measurable today.**

⛔ **THE BYTES DIFFER, BY CONSTRUCTION:** Zone A (spike's transcribed `AppendZoneA` vs shipped `BuildZoneA(GetVocabulary())` — the 3029-vs-5116 lane) · Zone B (fixture vs live capture) · Zone C (spike's hardcoded 13 kinds + `other_kinds: none` vs shipped `MaxRosterKinds = 8` + a computed collapse = **D2**).

⇒ ⚖️ **`chat=` controls the WIRE FORMAT and nothing else, which is exactly what STOP 1 is about.** The content gap is STOP 2's, is measured by M3's diff, and is already escalated (TASK-486). **477's "STOP AND SAY SO" judgement was right:** the gap is not a surprise, it is D2 + D3, already law and already owned. ⚠️ **But see WARN-3 — I found a FOURTH divergence in the same comparison that nobody has named.**

### ✅ (4) `VerifyFixtureKindParity` STILL REJECTS AN UNKNOWN KIND

`SiegeLlamaSpike.cpp:746-752` — a row whose kind is in no `SpikeRoster` slot returns **false** with a named message. ⛔ **The check was not removed; it was relaxed in exactly one direction and tightened in three** (`:719-725` non-empty, `:754-760` duplicate-free, `:762-769` relative order). ✅ Logic re-derived: `RosterIndex < PreviousRosterIndex` is safe because duplicates are caught first, so equal indices cannot reach it. **No off-by-one.**

### ✅ (5) §1 IS INTACT

`count` `:1380-1385` and `at-least` `:1396-1402` both still loop `SpikeGrammarCountMin..SpikeGrammarCountMax` = **`:160-161` 1 and 30** — never the live max. `count` keeps `"all"`; `at-least` still omits it. **Kinds not pruned; no policy production added.** ✅ `kind` is the identity side, which is the side §1 says to constrain hard.

### ✅ (6) TASK-479 — ALL FOUR LIMBS

- **(6a) SHIPPED PATH, READ-ONLY.** `SiegeCheatManager.cpp:618` is the **only** line in that file producing bytes; the file contains **no `BuildZone*` call and no write to `MaxRosterKinds`** (it appears only in a log string `:670` and a comment `h:146`). The accessor runs `SubmitUtterance` steps 7 and 8 and stops (`cpp:2819` `CaptureTurnSnapshot()`, `:2833` `return ComposeTurnPrompt(Utterance)`) — no `BeginTurn`, no `TurnId`, no `SetState`, no `PushMessage`, no dispatch.
- **(6b) ⛔ `ComposeTurnPrompt` IS STILL `private` — VERIFIED BY ME IN THE HEADER.** Sole `public:` **625** < accessor **1016** < sole `private:` **1019** < composer **1306**. Exactly one of each specifier, no `protected:`, **zero `friend` declarations.** ✅ **Not an automatic FAIL.**
- **(6c) ONE COMPOSER, AND THE ACCESSOR DELEGATES.** `cpp:2727` is the sole definition; callers are `:796` (`SubmitUtterance`) and `:2833` (the accessor, a bare `return` of the delegate). **It re-derives nothing, re-assembles nothing, re-orders nothing.**
- **(6d) NO OUT-PARAM, NO NEW MEMBER, NO SECOND `ReportFirstCapture`.** The signature is `FString DebugCaptureAndComposePrompt(const FString&)` (`h:1016`) — one param, one return. `ReportFirstCapture` still has **exactly one call site**, `cpp:2743`, inside the composer and not 479's.

### ⚠️ (7) M8 DECLARATION — 3 of 4. **TASK-476's handoff does not carry it** → WARN-4.

### ✅ (8) ⛔ THE INVARIANT CRITERION — SATISFIED, AND EXEMPLARILY

`SiegeAssistantComponent.h:929-963` names mechanism #2, **quotes it verbatim**, and argues the distinction that is the ruling: *"ITS PROPERTY IS 'NO ROUTE TO A ZONE BUILDER' — ⛔ NOT 'no public function ever returns a prompt'."* It then states the three consequences (a finished immutable `FString` hands out no builder · the composer stays private · the TASK-443 seam is untouched), records that widening the composer **and** `friend class USiegeCheatManager` were both **refused** and why (*friendship is the WIDER grant*), and cites `SpawnGroupCircleDecal` **by symbol, not by line** (§18c). ⇒ ⚖️ **Mechanism #2 has not been quietly converted into a comment. The next reader meets the argument where the grant is made.**

### ✅ (9) RE-CHECKED BEFORE INVOKING — **TASK-479's deliverable EXISTS.** Verified at `SiegeAssistantComponent.h:1016`, `.cpp:2754-2835`, `SiegeCheatManager.h:170-171`, `.cpp:579-678`. `qa/TASK-500.md` WARN-9 was a schedule fact and is now spent. **All four gated.**

---

## §3 — ⚖️ THE THREE DECLARED DECISIONS (criterion 10) — RULED EXPLICITLY

### ✅ (10a) THE AT-REST FSM WHITELIST — **RATIFIED.** Mechanism verified at the artifact, and it is a whitelist.

1. **`Capture()` re-surveys the ONE snapshot object IN PLACE.** `SiegeAssistantComponent.cpp:2718-2722`: `EnsureSnapshot();` then `Snapshot->Capture(World, OrderingTeam);` — the same object, re-surveyed. **Not a fresh allocation.**
2. **`GetTurnSnapshot()`'s own contract forbids the second survey.** `h:861-868`, verbatim: *"⛔ Do NOT re-Capture() from the executor: **a second survey mid-turn would silently answer a different question from the one the model was asked.**"* ⇒ ⛔ **Without the guard the dump would silently replace the survey a live turn is still reading — a change to shipped behaviour, which is the line this task was told not to cross.** The addition is **not** defensive padding; it closes a real hazard the spec did not see.
3. **⭐ IT IS A WHITELIST.** `cpp:2807-2809`: `if (State != Idle && State != Composing && State != Failed) → refuse`. **Only those three are admitted; every other value, including one added later, is refused by default.** ✅ The safe direction.
4. **And the three admitted states are the right three**, checked against the enum's own documentation (`h:311-333`): `Idle` = *"Console shut, nothing pending, no request in flight"* · `Composing` = *"Console open, waiting for a sentence"* (⚠️ **the PLAYER is composing, not the component** — no prompt exists yet) · `Failed` = *"THIS TURN failed"*, over. The four excluded — `Thinking`, `AwaitConfirm`, `Clarify`, `Deferred` — each still have a downstream reader of the current survey.

### ✅ (10b) `ForceUTF8WithoutBOM` — **RATIFIED. Present and unconditional in both lanes.**

- **Shipped:** `SiegeCheatManager.cpp:651-652` — `FFileHelper::SaveStringToFile(Prompt, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)`. **No branch, no option, no fallback.**
- **Spike:** `SiegeLlamaSpike.cpp:5962-5966` goes one better and **removes the question**: `FTCHARToUTF8` → `TArray64<uint8>` → `SaveArrayToFile`, so `FFileHelper`'s encoding logic is never consulted.
- ✅ **The diagnosis is correct and I confirm it independently:** `SaveStringToFile`'s default is `AutoDetect`, which makes the file's **encoding depend on its content** — a file that opens fine, has a plausible length, and hashes to something no consumer can reproduce. ⚖️ **A byte-diff instrument whose encoding varies with its input is not an instrument.** Two independent sightings, now §10(C); the pin is right in both places. See NIT-1 on the two mechanisms being different code paths.

### ✅ (10c) THE FAULT-LATCH NON-MIRROR — **RATIFIED, AND ON STRONGER GROUND THAN 479 CLAIMED**

⚖️ **Argued in the correct direction:** the latch exists to stop the **console** offering a broken feature to the **player**; a dev dump is not that.

⛔ **And the condition — "confirm the dump cannot itself fault, mislead, or dump garbage when `bAssistantFaulted` is set" — I verified by enumeration rather than by argument.** `bAssistantFaulted` is read at **exactly four sites**: `cpp:641` (`NotifyConsoleOpened`, a message), `:723` (`SubmitUtterance` gate 2, refuses the turn), `:1020` (`MarkAssistantFaulted`'s own idempotence), `:2016` (the deferred tick, clears the intent). ⇒ ⭐ **NOT ONE OF THEM IS IN THE PROMPT-BUILDER PATH.** `GetCachedZoneA`, `BuildZoneB`, `BuildZoneC`, `BuildPendingLine` and `Capture` are all latch-independent. **A dump taken while faulted produces exactly the bytes the builder produces — it cannot lie and it cannot dump garbage.**

⭐ **AND THE INTERLOCK 479 DID NOT PROVE, WHICH IS THE HALF THAT MAKES THE DECISION ACTUALLY WORK:** `MarkAssistantFaulted` ends at `cpp:1044` with `SetState(ESiegeAssistantState::Failed)`, and **`Failed` is on the at-rest whitelist.** ⇒ **The two unasked-for decisions interlock exactly as intended: the dump remains usable in precisely the faulted session 479 argued for.** Had the latch left the FSM in `Thinking`, the whitelist would have silently defeated the non-mirror and the instrument would have been useless in the one session that most needs it. ✅ **It does not. Both stand.**

---

## §4 — ⚖️ THE FOUR RULINGS THE TASKS ASKED FOR

### ⚖️ RULING 1 — 478's REWRITE OF TWO PRE-EXISTING `UE_LOG` STRINGS, ONE INSIDE 477's `CmdSpikePrompt`: **RULED IN.**

**Grounds, in order of weight:**
1. **§22 makes a runtime log string the PRIORITY surface**, because *"a log line is the artifact an investigator TRUSTS MOST, so a false one is believed over the code."* Both strings asserted the exact mechanism clause (1) deleted. `:4238-4243` described **the opposite failure** (the sampler *forbidding* a symbol) and `:5802-5808` had become **self-contradictory** (*"disagrees with the grammar's kind list"* — the grammar's kind list **is** this fixture's roster). ⛔ **Leaving them to keep a metric clean would have shipped a false statement into the surface an investigator trusts most.**
2. **The blast radius is provably zero on every command line that exists today.** Both branches are gated on `VerifyFixtureKindParity` **failing**, and t0/t1 are valid subsets by construction (§5 below). **One format string, no logic, no identifier, no line-position change for any reachable line.**
3. **It was declared BEFORE the count was taken** and is carried in the ledger as its own row (*"survive via 478's DECLARED rewrite: 2"*), so `ORPHANED = 0` is not being borrowed from 477. ⇒ ⚖️ **478 explicitly does NOT claim 477's zero-rewrites result, and it was right not to.**
4. **477's collision note bounded CONCURRENT work, not sequential hand-off.** 477 was complete and handed off. ⚖️ **This is the same reading ruling (R1) applied to TASK-479/489: single-owner-per-file forbids concurrent edits, never sequential ones.**

⛔ **CONDITION — THIS IS NOT A GENERAL EASEMENT.** It is ruled in on *these* grounds: a **stale runtime claim**, in a **branch unreachable today**, **declared in advance**, **string-only**. **A logic change, an identifier change, or an undeclared one inside another task's function would not have passed**, and the next agent must not read this as licence.

### ⚖️ RULING 2 — 477's STATED REASON FOR LEAVING THE `prompt=` DEFECT: **NOT SOUND. 478 IS CORRECT, AND THE RECORD IS HEREBY CORRECTED.**

477 recorded *"I did not change it (it would move the default path)."* **Read `ResolveZoneAText` at `SiegeLlamaSpike.cpp:3991-4012` — it does not:**

```cpp
static FString ResolveZoneAText(const FSpikeOptions& Options)
{
    if (!Options.ZoneAOverridePath.IsEmpty()) { ...Display... ...Warning... }   // BOTH log lines live in here
    FString ZoneA; AppendZoneA(ZoneA); return ZoneA;                            // the default path
}
```

⇒ ⛔ **With `ZoneAOverridePath` empty the function skips the override branch entirely and returns the identical bytes `CmdSpikePrompt:5757-5758` builds today, emitting ZERO log lines** — both its `Display` (`:3998`) and its fallback `Warning` (`:4004`) sit **inside** the override branch. **The one-token fix is provably default-identical; its only observable effect is on a command line that types `prompt=`, where the flag does nothing at all today.**

⚖️ **The disposition, stated so nobody mis-reads the ruling:**
- **477 reached the RIGHT OUTCOME for the WRONG REASON.** Leaving it was correct — it is outside 477's `names:` and its spec. **The reason it gave is false, and it is now on the record where TASK-501 will read it.**
- ⛔ **TASK-501 MUST NOT INHERIT "this moves the default path" AS A CONSTRAINT.** It does not. The fix is one token.
- ✅ **478 was RIGHT to name it and RIGHT to leave it.** `CmdSpikePrompt` is not in its `names:`, and *"an uncovered defect is named, not quietly absorbed"* is the declared behaviour. **Both agents behaved correctly; exactly one sentence was wrong, and it is corrected here.**
- ⚠️ **Its consequence is the sharp part and it stands:** Stage D renders **every gold training prompt** through `SpikePrompt out=`, and per 477's own `out=` doctrine the consumer's length/sha256 assertions **certify the file, not the prefix it was built from.** ⇒ **A silently-ignored Zone A override there builds the training set against the wrong prefix, with no warning, and it is invisible forever once it reaches the weights.**

### ⚖️ RULING 3 — 477's REFUSAL TO PUT `wire=` ON `SpikeEval`'s DEFAULT PATH: **RATIFIED, AND THE DISCOVERABILITY HALF IS CONFIRMED PRESENT.**

§16 freeze #2 is unconditional; a new line on `SpikeEval`'s default output moves the instrument every number on record was taken with. ✅ **And the half that makes the refusal safe rather than merely correct is there** — `SiegeLlamaSpike.cpp:6159-6161`, in `SpikeEval`'s own help text:

> *"WARNING: this command prints NO wire= indicator, because adding a line to its default output would move the instrument every number on record was taken with. BuildPrompt falls back to raw concatenation SILENTLY when the loaded GGUF exposes no chat template, so a chat=1 eval can be secretly raw. **TAKE THE WIRE-FORMAT READING WITH Siege.Llama.SpikePrompt FIRST** … and only then trust an A/B taken here"*

⇒ ⚖️ **A refusal written only into a handoff would have been a gap; written into the help text, it is a routing instruction the operator meets at the moment it matters.** ⛔ **TASK-481 and whoever runs M1 are bound by it: confirm `wire=chat` and `wire=raw` on `SpikePrompt` BEFORE quoting any `SpikeEval` A/B.** The gap itself survives Stage A — recorded, not closed (§6).

✅ **The `wire=` discriminator is also sound in its own right.** `:5904-5905`: `FullPrompt.StartsWith(ZoneA, ESearchCase::CaseSensitive)` ⇒ `raw`. **I verified both of `BuildPrompt`'s raw branches put Zone A at offset 0** (`:3166` the no-template branch, `:3199` the apply-failed branch) and that the templated path (`:3210`) cannot, since a chat template opens with a role marker before the system content. ⭐ **It is a POSITIVE structural fact, not a search for an absence** (§14), and it catches the apply-failure path that a `model_has_template` check alone would miss. ⚠️ §32: never observed to function.

### ⚖️ RULING 4 — 478's CORE INSIGHT: **VERIFIED AT THE ARTIFACT. IT IS CORRECT, AND IT CHANGES WHAT THE CHECK MEANS.**

- **BEFORE:** `BuildSpikeGrammar` derived `kind` from `SpikeRoster` ⇒ a typo'd fixture kind was a symbol the sampler **FORBADE**. **Loud** — every row needing it fails.
- **AFTER** (`:1360-1364`, `Alternatives.Add(GbnfJsonString(Fixture.Roster[Index].Kind))`) ⇒ a typo'd kind is **ADDED to the GBNF as a legal alternative.** The sampler can emit a unit **the game does not have**, and **the shipped executor is the only thing left** between that and a valid-shaped wrong command. **Silent.**

⇒ ⚖️ **`SpikeRoster` HAS STOPPED BEING THE GRAMMAR AUTHORITY AND IS NOW ONLY THE VOCABULARY AUTHORITY — and `VerifyFixtureKindParity` clause (a) (`:746-752`) IS THE WHOLE OF THAT GUARANTEE.** ⛔ **Shipping this on the "the check is now vestigial" reading would have been the worst outcome available in this task**, and 478 caught the inversion while tracing rather than after. **Confirmed.**

✅ **The three tightenings are ratified with it** — without them a naive "is it in the list" subset test would have silently dropped protections index-identity gave for free: duplicates (`:754-760`, a duplicate emits a duplicate GBNF alternative and a duplicate `roster:` line) and empty (`:719-725`, `kind ::=` with no alternatives ⇒ llama.cpp refuses the **whole** grammar ⇒ **every generation runs UNCONSTRAINED**, the precise §9c failure that has already reached a measurement run).

✅ **AND THE ORDER REQUIREMENT (478 §2.1) IS UPHELD.** `SpikeRoster`'s order **is** `cards.csv` row order — the roster comment says so at `:430` — *"which is what the shipped `Capture()` sorts to."* A reordered fixture would print a `roster:` block **no live board can produce**, so bar #3's byte-level prefix claims would measure a layout that does not ship. ⚖️ **"Permit a subset" and "permit a subset in arbitrary order" are different relaxations, and 478 took the smaller one.** That answers `qa/TASK-500.md` D3's flag. **Do not delete the third branch.**

---

## §5 — 🔒 THE FROZEN-BYTES CONFIRMATIONS THE GATE DEMANDED

**⭐ "The emitted grammar is byte-unmoved for every fixture that currently exists" — VERIFIED INDEPENDENTLY, AND BY A STRONGER METHOD THAN 478's SCRIPT.** I compared the two literal tables directly:

| | `SpikeRoster` `:438-453` | `SpikeRosterT1` `:491-506` |
|---|---|---|
| kinds, in order | footman · archer · knight · miner · militiamob · pikeman · sapper · cavalry · longbowman · cleric · ogre · wizard · sorcerer | **identical, same order, 13 rows** |

- **`t0`: `SpikeFixtureT0:578` passes `SpikeRoster, SpikeRosterNum` — the fixture's roster IS the roster, by identity.** ⇒ `BuildSpikeGrammar(t0)`'s `kind` rule is **character-identical** to the pre-change `SpikeRoster`-derived rule. **Not "a subset of itself" as an argument — the same array.**
- **`t1`: the kind strings are identical to `SpikeRoster`'s in the same order** ⇒ its `kind` rule is byte-identical too. ✅ `qa/TASK-500.md` **NIT-6's warning that `t1` is invisible in the fine-tune law is answered: `t1` was not overlooked**, and I confirm it on the tables rather than on the ledger.
- **`static_assert` `:517-518` binds the row counts at compile time**; `VerifyFixtureKindParity` binds the symbols at runtime. ⚠️ **Note the honest limit: the compile-time binding is on COUNT only.** Symbol identity for `t1` is enforced only by a runtime check that **logs and does not stop the run** — see NIT-2.

**⭐ THE `count` / `at-least` BLOCKS.** I cannot sha256 them, but the substance is verified: **both loops read `SpikeGrammarCountMin..SpikeGrammarCountMax`** (`:1380`, `:1398`) and **those constants still read `1` and `30`** (`:160-161`). `count` keeps `"all"`; `at-least` still omits it, with the reason intact (`:1388-1390`). **1–30 stayed constant.**

**⭐ THE DEFAULT OUTPUT CONTRACT OF `SpikeEval` / `SpikePrompt`.** A caller passing no new arguments gets byte-identical output **on the source-level evidence I can reach**: every new emission is behind a typed-flag gate or an unreachable-today failure branch, no pre-existing line moved position, and the one relocated line (`:4784`, `"split '%s' NOT SCORED"`) is the first statement of `RunSplitRepeated` exactly as it was the first statement of `RunOneSplit`, at the same level with the same tag. ⛔ **This is a claim about source bytes. TASK-481 owes the run.**

**⭐ THE SIGNATURE CHANGE IS FULLY PROPAGATED.** `RunOneSplit` (`:4550`) has **exactly one caller** (`:4807`), and `RunSplitRepeated` **exactly two** (`:5115` dev, `:5135` holdout). **No stale call site survives** — the highest-risk item in 476's collision surface, and it is clean.

**⭐ QUEUE DEPTH 1 IS INTACT.** No `StartJob` call was added; the loop is inside the job (`:4795-4808`). §12h's recorded refusal is not weakened.

---

## §6 — ⚠️ THE GAPS THAT SURVIVE STAGE A — RECORDED, ⛔ NOT CLOSED

⚖️ **A gate report that let these disappear would be the confident green this batch exists to prevent. They are gaps in the INSTRUMENT SET, not defects in the four tasks, and none is a blocker to Stage A.**

1. ⛔ **`prompt=<path>` IS SILENTLY IGNORED BY `Siege.Llama.SpikePrompt`.** `CmdSpikePrompt:5757-5758` calls `AppendZoneA(ZoneA)` directly. **Confirmed by me at the artifact.** The sweep is complete and correct: `SpikeBench`→`RunBenchJob:4228` ✅ · `SpikeEval`→`RunEvalJob:5090` ✅ · **`SpikePrompt` ⛔ NO** — 1 of 3, no fourth site. ⚠️ **This is the one that touches Stage D's data.** Owner: **TASK-501**, and per Ruling 2 the fix is one token and is default-identical.
2. ⚠️ **NO `wire=` ON `SpikeEval`.** Refused on §16 grounds (**correctly** — Ruling 3). ⇒ **M1's A/B must be validated on `SpikePrompt` first, every time.**
3. ⛔ **D2 IS INEXPRESSIBLE BY THE SPIKE AT ALL.** `AppendZoneC`'s `other_kinds: none` is a **hardcoded constant**, not a computed branch — the harness does not *disagree* with the shipped collapse, **it has no mechanism to disagree.** Escalated: **TASK-486**, Jonathan's.
4. 🆕 **WARN-3 below — a FOURTH divergence, in the utterance line itself, named by nobody.**

---

## §7 — FINDINGS

### 🟡 WARN-1 — CRITERION (12)'s GIT-DEPENDENT EVIDENCE IS UNVERIFIED BY ME, AND ITS OWNER IS NAMED
⛔ **No shell, no Git — by role design.** Unverified: HEAD's **87** `UE_LOG` baseline · the `+16 / +10 / +7` deltas · **`ORPHANED = 0`** · every sha256 ledger · 479's **354 insertions / 0 deletions** · the help-string `old_is_PREFIX_of_new` claims.
⚖️ **Why this is a WARN and not a BLOCKER, argued rather than assumed:** the risk it covers is *"a pre-existing line was silently changed"*, and four independent things bear on it — **(a)** 477 and 478 each ran the HEAD-vs-now survival ledger **separately**, with 478 explicitly accounting for 476's 8 rewrites and its own 2 and closing at **zero unaccounted**; **(b)** the arithmetic's endpoint (**120**) I re-derived myself; **(c)** I read every changed region and found each new emission gated; **(d)** ⭐ **TASK-481 owes the definitive proof anyway, and no source-level ledger substitutes for it.**
⇒ **TASK-481 must run, and paste, at minimum:** `git diff --numstat` and `git diff -U0 -- <paths> | grep -E "^-[^-]"` for **all five changed files** (⛔ **prefix every path with `GitClaudeUnrealTest/` — the toplevel is the parent**), **each with a blob-size positive control before any zero is believed**, plus the §16 dev-split re-run.

### 🟡 WARN-2 — THE THREE HANDOFFS' sha256 LEDGERS ARE **MUTUALLY INCOMPARABLE**, AND A SEAL NOBODY CAN RE-DERIVE IS NOT A SEAL
Every symbol common to 477 §4(c) and 478 §5(c) carries a **different hash in each**, while both report `IDENTICAL`:

| symbol | 477 | 478 |
|---|---|---|
| `SpikeFixtureT0` | `d8b28b27cd5cab1b` | `cb1f3b283d0ed1be` |
| `AppendZoneA` | `11c0ae812370d510` | `824f92834ce74a93` |
| `BuildPrompt` | `e127c4a03801d964` | `90d6098a731c1176` |

⚖️ **This is not a contradiction — each ledger is internally valid** (HEAD-vs-now under one extractor), and the only consistent explanation is **different extraction boundaries per script**. ⛔ **But it means the three handoffs are NOT a chain of one running fingerprint, and no future task can re-derive `t0`'s seal without the exact script that produced the hash.** ⇒ 🔒 **§16's `t0` freeze deserves a reproducible fingerprint with the extraction rule written down** — recommend the manager pin one (e.g. sha256 over the exact line range, rule stated in §16). **Two independent scripts agreeing is good evidence; it is not a seal.**

### 🟡 WARN-3 — ⭐ **A FOURTH LANE DIVERGENCE, IN THE UTTERANCE LINE, NAMED BY NO HANDOFF, NO QA REPORT AND NO D-NUMBER**
The two lanes sanitise the utterance with **different functions**, and **the caps are in different units**:

| | spike | shipped |
|---|---|---|
| function | `SiegeLlamaSpike.cpp:1010-1040` `SanitizeForPrompt(const FString&)` | `SiegeAssistantSnapshot.cpp:1476+` `SanitizeForPrompt(const FString&, int32&)` |
| cap | `MaxUtteranceChars = 240` **CHARACTERS** (`:1012`) | `MaxUtteranceBytes = 240` **BYTES** (`SiegeAssistantSnapshot.h:381`) |
| on truncation | hard cut, ⛔ **no marker**, no log | appends `UtteranceTruncationMarker`, logs the cut (`:1620`) |
| surrogates | none — can cut mid-pair | pairs move as one; unpaired dropped (`:1513-1534`) |

✅ **The FLATTEN rule is character-identical** — same four whitespace chars, same collapse, same leading-drop (`:1017-1038` vs `:1496-1511`). ⇒ **For any pure-ASCII utterance ≤ 240 the two agree exactly.**
⇒ ⚖️ **BOUNDED AND NOT ALARMING: no measurement on record is affected** (every corpus row and the default order line are short ASCII), **and STOP 1 is unaffected** — the sanitiser sits inside Zone C, not at the assembly seam. ⛔ **But Stage D renders gold training prompts through `SpikePrompt out=` with arbitrary `order=` strings, and 478's own §6 argument applies verbatim: the consumer's length/sha256 assertions certify the file, not the lane that built it.** A gold utterance over 240 would be truncated **differently, with no marker, and with no warning** on the lane the trainer consumes.
📌 **Manager: board it beside the `prompt=` bypass. It is the same shape, in the one place nobody looked.** And whoever runs M3 must not read a Zone C utterance-line difference as D2.

### 🟡 WARN-4 — CRITERION (7): **TASK-476's HANDOFF CARRIES NO M8 DECLARATION**
477 (§ header), 478 (§ header) and 479 (§1 and §2.7) each state *"adds no replicated property, no new replicated class, no new relevancy tier."* **476's handoff contains the word "replicated" zero times** (searched; the same search returns hits in the other three ⇒ positive control).
⇒ ⚖️ **WARN, not BLOCKER, because the property itself HOLDS and I checked it rather than waiving it:** 476's new symbols (`SpikeMaxEvalRepeats`, `TagEvalRepeat`, `FSpikeOptions::EvalRepeats`, `FSplitRowObservation`, `FSplitRunTotals`, `SummariseIntSeries`, `FormatIntSeries`, `RunSplitRepeated`) are **`static constexpr`s, plain non-`USTRUCT` structs and static free functions in a plugin TU with no `UCLASS`, no `UPROPERTY` and no replication.** **Only the recital is missing.** ⛔ **The recital exists so the next reader need not re-derive it — 476 should append the sentence; nothing else changes.**

### 🟡 WARN-5 — `SpikePrompt`'s HELP TEXT DOES NOT WARN THAT `prompt=` IS IGNORED, AND `SpikeEval`'s ADVERTISES IT
`SpikeEval`'s help lists `prompt=<zoneA path>` (`:6148`) and honours it; **`SpikePrompt`'s help (`:6164-6181`) does not mention `prompt=` at all** — yet the **shared** `ParseOptions:5567` accepts it on every command, so `SpikePrompt prompt=X` parses cleanly and does nothing. ⚖️ **§22's own logic: the absence is as misleading as a stale string**, because the operator's mental model comes from the sibling command. ✅ **478 set the right precedent** — `CmdSpikeGrammar` states *"this command has NO prompt= override"* in its readout (`:6106`) and its help. ⇒ **TASK-501 should close the code path; until it lands, this is the discoverability half of gap #1.**

### 🟡 WARN-6 — §12g's WARN AND §16's INSTRUMENT PROOF ARE BOTH STILL OPEN, AND THE BOARD MUST NOT READ THIS PASS AS CLOSING THEM
Restated as a finding so it cannot be lost between reports. ⇒ **TASK-481 (2)** owes the dev-split re-run; **TASK-485** owes the first shipped-lane reading. ⛔ **Neither is discharged here.**

### 🟡 WARN-7 — 476's CHAINED KV CACHE ACROSS REPEATS — **DECISION RATIFIED, CONFOUND RECORDED**
The KV cache is chained, so repeat 1's first row has a different re-prefill boundary than repeats 2..N's, **and it can propagate down the split**. ⚖️ **Ratified:** clearing per repeat would move the **default path** at `repeats=1` (§16 forbids it outright), clearing only *between* repeats **relocates the asymmetry rather than removing it**, and choosing the replicate design is **M0's** decision — §12h's leading hypothesis for the DEV-11 flip *is* KV nondeterminism, and it is explicitly **not diagnosed**. ⛔ **A task may not spec a fix as though it were a finding.** The harness prints the caveat at WARNING level on every repeat run.
⚠️ **STANDING CONSTRAINT FOR STAGE B: the sound comparison is among repeats 2..N.** ⛔ **A `min` taken over all N includes repeat 1's different conditions, and §10 clause #1 scores the MINIMUM** — whoever takes the gate reading must say which runs it covers.

### 🟡 WARN-8 — THE HELP-STRING CHANGES ON THE TWO FROZEN COMMANDS — **RATIFIED, ONCE, WITH THE BOUNDARY DRAWN**
476, 477 (both to `SpikeEval` / `SpikePrompt`) and 478 (to `SpikeGrammar` only) each appended. ⚖️ **Ruled: the console REGISTRATION BLURB is outside §16's frozen MEASUREMENT-OUTPUT contract** — it is never emitted by a measurement run, so it cannot move a number — **and an undocumented flag is the worse defect.** ⭐ **Ruling 3 depends on this**: the `wire=` refusal is only safe *because* the help text carries it. ⛔ **BOUNDARY: this covers text APPENDED to a registration blurb. It does not extend to any line a measurement run emits.**

### 🟡 WARN-9 — 478's LEDGER B IS WEAKER THAN LEDGER A, AND THE GAP IS REAL
⛔ **There is no committed baseline for "as 477 left it"** — both 476 and 477 are uncommitted — so 478 could not hash-verify their work, and **correctly refused to manufacture one by reverse-applying its own edits.** What it substituted (containment: none of 478's identifiers appears inside 476's/477's symbols, with a positive control returning 7/3/2 hits on the three functions it *did* edit) **proves nothing was ADDED, not that nothing was REMOVED.** ⚖️ **Accepted, because (a)'s zero-unaccounted format ledger covers removal — a dropped pre-existing line would orphan a HEAD format string — and because TASK-481's re-run is the real closure.** 📌 **Recorded so the residual is visible rather than implied.**

### ⚪ NITs

- **NIT-1 — the two artifact writers are DIFFERENT CODE PATHS for outputs M3 will byte-diff.** Shipped uses `SaveStringToFile(..., ForceUTF8WithoutBOM)` (`SiegeCheatManager.cpp:651`); spike uses `FTCHARToUTF8` → `TArray64<uint8>` → `SaveArrayToFile` (`SiegeLlamaSpike.cpp:5962-5966`). ✅ **They agree** — both emit raw UTF-8, no BOM, no line-ending translation. **Named so M3 does not have to re-derive it under time pressure.**
- **NIT-2 — `t1`'s SYMBOL identity is enforced only at RUNTIME, and the check logs without stopping.** `static_assert:517` binds the **count**; `VerifyFixtureKindParity` binds the symbols but its failure path only `UE_LOG`s (`:4240`, `:5106`, `:5805`) — the run continues under the resulting grammar. ⚖️ **Consistent with the file's declared idiom** (`AppendRule`'s illegal-name path, quoted at `:1337-1346`: emit faithfully so the failure is diagnosable, never substitute) **and ratified on that ground.** ⛔ But it means "t1's grammar is unmoved" is compile-time-guaranteed on *count* and runtime-checked on *symbols*.
- **NIT-3 — `⚠️` (U+26A0 + U+FE0F) inside a `TEXT()` literal**, `SiegeCheatManager.cpp:668` — the only emoji in a `TEXT()` literal in either module. ⚖️ **Not a defect and not new-in-class:** the file already carries **24 pre-existing non-ASCII `TEXT()` literals** (em dash `—`, arrow `→`, lines 234–574), so 479 matched the established convention and the file demonstrably compiles with them. ⛔ **But the plugin module bans non-ASCII in `TEXT()` outright** (476/477/478 each scanned and removed one), so the two modules disagree. **A variation selector can render as a box in the UE log.** Suggest `WARNING:`; **revert cost is one word.**
- **NIT-4 — `FormatTokenIdSpan(Tokens, Max(0, Num()-8), 8)`** (`:6009`) overlaps the `first=` span when a prompt tokenizes to fewer than 16 ids. **Harmless, cannot occur on a real prompt.**
- **NIT-5 — `FSplitRunTotals::QuestionPasses`** (`:4476`) is written and never read. 476 declared it. **A struct field, so no unused warning; dead weight, not a defect.**

---

## §8 — ⛔ WHAT I DID NOT DO (§20)

1. **No compile, no editor, no MCP, no PIE, no model load, no `SpikeEval` run.** Every claim is source-level.
2. **No Git, no shell** — WARN-1.
3. ⛔ **No sealed corpus was opened.** `holdout.csv` / `holdout2.csv` untouched; nothing in this gate touches one, as the spec states.
4. **I did not re-audit the law** — that was TASK-500, whose 3 blockers are ruled and are the manager's. ⚠️ **BLOCKER-2 there (assistant §5's absolute exec ban vs FINE-TUNE §8's reinterpretation) touches TASK-479 directly.** I gate against §8's ruling and the board's `#### TASK-479` (R1/R2/R3), which are the operative authority; **the §5 letter still reads as an absolute ban and the next reviewer will hit it again.**
5. **I did not verify the help strings are pure appends** — git-dependent; substance read and correct.
6. **I did not review `SiegeAssistantComponent.cpp` / `SiegeAssistantSnapshot.cpp` as changed files** — reference-only, save for 479's one addition and the reads Rulings 2, (10a) and (10c) and WARN-3 required.

---

## §9 — NOTES FOR BUILD-MASTER (TASK-481)

1. ⭐ **STEP (2) IS THE ACCEPTANCE CRITERION AND IT OUTRANKS THIS REPORT.** `SpikeEval dev=Docs/Data/assistant_eval_dev.csv tier=full gpu=0` ⇒ **20/25**, failing rows **`DEV-01 · DEV-04 · DEV-07 · DEV-16 · DEV-20`**. ⛔ **Anything else means the instrument moved — STOP and report; do not proceed and do not adjust.**
2. ⛔ **CARRY WARN-1's git work.** All five files: `SiegeLlamaSpike.cpp` · `SiegeAssistantComponent.{h,cpp}` · `SiegeCheatManager.{h,cpp}`. **Prefix paths with `GitClaudeUnrealTest/`** and **assert a non-empty blob before believing any zero** (478's positive control).
3. ⛔ **TAKE THE WIRE READING FIRST** (Ruling 3): `SpikePrompt out=… chat=1` then `chat=0`, confirm `wire=chat` / `wire=raw` on the `SPIKE_PROMPT_OUT` line, **before** any `SpikeEval` A/B. **If `chat=0` and `chat=1` produce identical bytes, that is a major finding — report it, do not assume a bug.**
4. **`out=`/`ids=` REFUSE at Error level without a resident model** (`:5830-5835`) — **by design.** Run `Siege.Llama.SpikeLoad` first; a refusal there is the guard working, not a defect.
5. ✅ **A FREE §32 DISCHARGE THAT COSTS NO MODEL AND NO GATE SLOT:** run `Siege.Llama.SpikeGrammar` and `Siege.Llama.SpikeGrammar fixture=t1`, confirm **both dumps are identical** and both report `ZONE_A_KIND_SEAM … CLOSED`. **That converts 478's whole additive claim from an argument into an observation.** Also try `fixture=t2` — it must **warn and name the fallback** (`:6062-6064`), unlike `SpikePrompt`'s silent `t0` (478 §6's second finding).
6. **`repeats=5`:** confirm per-row `stable=`/`flips=` **and** split `min/median/max` **each labelled with the clause that reads it**, and that the **leniency floor travels beside the number**. ⚠️ **Per WARN-7, say which runs the reported `min` covers.**
7. 🔒 **`L_Arena` is NEVER saved — SHA256 before AND after, hash never mtime.** Verify measured FPS ≥ 58 before trusting any PIE reading (§12e).
8. **Run `DumpAssistantPrompt` with the FSM at rest** (console shut, or open with no sentence in flight) — `Thinking`/`AwaitConfirm`/`Clarify`/`Deferred` will **correctly refuse**, and that refusal is the guard working. ⭐ **Quote the `FIRST LIVE CAPTURE` line in the SAME session** — §13(c), and TASK-485 owns the artifact.

## §10 — BOARD FLIPS FOR THE ORCHESTRATOR (⛔ I APPLY NONE)

- `#### TASK-476` → **`qa-passed`** · `#### TASK-477` → **`qa-passed`** · `#### TASK-478` → **`qa-passed`** · `#### TASK-479` → **`qa-passed`**
- `#### TASK-480` → **`qa-passed`** (report `qa/TASK-480.md`)
- `#### TASK-481` → **unblocked, `ready-for-integration`** — subject to the QUIET-MODULE LAW (⛔ TASK-475 must not be in flight)
- **Manager's, arising from this gate:** Ruling 2's correction to `#### TASK-477`'s record (the `prompt=` reason is false — **TASK-501 must not inherit it**) · **WARN-3's fourth divergence wants a board line** · WARN-2's reproducible `t0` fingerprint · WARN-4's one-sentence M8 recital in 476's handoff · `bZoneAIdentityChecked` as §13(c)'s second consumable (479 raised it; still unboarded).
