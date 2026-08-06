# TASK-542 — LOG THE RAW MODEL OUTPUT — programmer handoff

- **Task:** TASK-542 [AC-2] — the one failure class with no artifact (`AS-§21.8`)
- **Status:** `ready-for-qa`
- **QA gate:** ⛔ **TASK-550** (`.claude/pipeline/qa/TASK-550.md`) — this task has **no gate of its own**; it is reviewed as part of the batch gate naming 541..549.
- **Compile:** ⛔ none run. TASK-551 owns the batch's only compile. **Tests:** ⛔ none written — TASK-549 owns them.

---

## 1. THE CHANGE — ONE STATEMENT, ONE FILE

**File:** `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp`
**Function:** `USiegeAssistantComponent::HandleModelCompletion`
**The line, character-for-character (`:1016` post-edit):**

```cpp
UE_LOG(LogSiegeAssistant, Log, TEXT("Turn %d: RAW MODEL OUTPUT (%d chars): [%s]"), InTurnId, Output.Len(), *Output);
```

⛔ **`SiegeAssistantComponent.h` IS NOT TOUCHED.** The header was in the task's scope line but needed no edit: `@param Output`'s *"⛔ A LOCAL FOREVER — never stored (mechanism #1)"* and the class header's *"NO MEMBER OF THIS CLASS EVER HOLDS MODEL OUTPUT"* are both **still literally true after this change**, so amending either would have been a false correction. ⇒ **The whole diff is one `.cpp` file, one statement plus its comment block.**

## 2. PLACEMENT — AND WHY IT IS THE PLACEMENT THAT WAS ASKED FOR

Order of statements in `HandleModelCompletion` after the edit:

```
  :952   stale-turn guard        (InTurnId != TurnId)        -> return        ── before
  :959   state guard             (State != Thinking)         -> return        ── before
  :966   success guard           (!bSuccess)                 -> return        ── before
  :977   FSiegeAssistantCommand Command;   FString ParseError;
  :980   ── the comment block ──
▶ :1016  UE_LOG(... RAW MODEL OUTPUT ...)                    <<< THE ONLY NEW STATEMENT
  :1018  if (ParseSiegeAssistantCommand(Output, Command, ParseError))
```

- ✅ **AFTER all three guards** ⇒ it fires **once per ACCEPTED completion** and **never for a dropped, stale, wrong-state or subsystem-failed one**. A dropped turn already has its own artifact and would only add noise.
- ✅ **IMMEDIATELY BEFORE `ParseSiegeAssistantCommand`** — the last statement before the call, nothing between them ⇒ **it fires whether or not the parse succeeds.** ⭐ That is the load-bearing half: **the malformed emission is the case most in need of its own bytes**, and a line that printed only on success would be missing exactly when it is wanted.
- The two locals (`Command`, `ParseError`) sit above it, so the shipped *"`Output` IS A LOCAL AND STAYS ONE (mechanism #1)"* comment stays adjacent to the declarations it describes and reads straight into the new block.

## 3. VERBOSITY — `Log`, AND BOTH ALTERNATIVES REFUSED IN THE SOURCE

- ⛔ **NOT `Verbose`.** Jonathan's report arrives **after** the session. A `Verbose` line needs a config change he will not have made, on a turn he cannot reproduce ⇒ the artifact would not exist at the only moment it is read. **A diagnostic that requires foreknowledge of the bug is not a diagnostic.**
- ⛔ **NOT `Warning`.** This fires on the **HAPPY path**, and the automation runner reads `Warning` as failure (the project has recorded that reasoning before) ⇒ every successful assistant turn would fail the suite.
- ✅ **`Log`, category `LogSiegeAssistant`** — as everything else in the file. The category is `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeAssistant, Log, All)` (`SiegeAssistantCommand.h:37`), so **`Log` is the category's default runtime verbosity and this line prints with no config anywhere.** ⚠️ That is the whole point and it is worth QA confirming: `Verbose` would NOT have printed by default under this same declaration.
- ⛔ **Both refusals are written INTO the comment block**, so a later editor "tidying" the verbosity has to delete a stated reason rather than merely overlook one.

## 4. TRUNCATION — **DECIDED: NONE. THE STRING IS PRINTED IN FULL.**

Asked for explicitly, so the reasoning is recorded rather than implied:

- ⛔ **No elision, no `Left(N)`, no `…`.** **The tail is precisely where a malformed or invented field would sit**, so a truncating log would drop the evidence this line exists to capture — it would re-create the no-artifact class it was written to close, only quieter.
- **The length is bounded in practice by two shipped mechanisms, not by hope:** the output is **grammar-constrained** (`SiegeAssistantGrammar`) to a single small JSON object, and the sampler's own token budget bounds it. There is no path here to an unbounded dump.
- **`UE_LOG` imposes no length limit** — the engine's formatter grows its buffer — so no engine-side clipping silently reintroduces truncation.
- ✅ **INSTEAD OF TRUNCATING, THE LINE IS MADE SELF-DIAGNOSING:** `Output.Len()` is printed **beside** the text and the text is **bracketed**. ⇒ `(0 chars): []` is an **empty** completion, unmistakably; a short `%d` against a JSON that stops mid-field is a **truncated** one; a full-length body that parses badly is a **malformed** one. ⚠️ **Without the length figure those three read identically in a log**, which is why the board asked for it.
- **`InTurnId` leads the line** — a log line that cannot be tied to a turn is not evidence, and every other line in this function is keyed the same way (`Turn %d: …`), so the whole turn greps as one block.
- **ASCII-only inside `TEXT(...)`** deliberately: `RAW MODEL OUTPUT` is the grep token Jonathan or QA will paste, and this file has **no BOM**, so a new non-ASCII byte inside a string literal is a risk taken for nothing. (The comment block uses the house emoji as the rest of the file does — comments are immune.)

## 5. COMPILE-TRAP CHECKLIST (restated, per the standing rule)

- ✅ **Format string is a bare `TEXT("...")` literal** ⇒ satisfies UE 5.8 `TCheckedFormatString` (the TASK-268 C7595 lesson). No variable, no concatenation, no `FString` fed as a format.
- ✅ **Arg types match the specifiers:** `%d` ← `InTurnId` (`int32`), `%d` ← `Output.Len()` (`int32`), `%s` ← `*Output` (`const TCHAR*` via `FString::operator*`). ⛔ No `FString` passed to `%s` un-dereferenced.
- ✅ **No shadowing** — no new identifier of any kind is introduced, so no inherited reflected member can be shadowed.
- ✅ **No most-vexing-parse** — the statement is a call, not a declaration.
- ✅ **No new include needed** — `FString`, `Output` and `LogSiegeAssistant` are all already complete types in this TU; nothing new is named.
- ✅ **No literal `*/` introduced** in the comment block.

## 6. ⛔ BEHAVIOUR-FREEDOM — HOW A REVIEWER SEES IT **IN ONE LOOK**

⭐ **The whole argument is visible in the diff's shape: the diff adds exactly one statement, and that statement is a `UE_LOG`.**

Mechanically, the four things that would make it more than an observation are each **absent from the diff**, and their absence is checkable without reading the function:

1. **No control flow** — the added statement contains **no `if`, no `return`, no `continue`, no loop, no early exit**. Every path into and out of `HandleModelCompletion` is byte-identical to before.
2. **No writes** — it appears **on the right-hand side of nothing**. It assigns to no member, no local, no out-parameter; it calls **no non-`const` method** (`FString::Len()` is `const`, `operator*` is `const`).
3. **No new state** — ⛔ **`Output` STAYS A LOCAL. No new member, no new field, no retention, no second copy.** The diff introduces **zero declarations**, so there is nothing new for QA to trace the lifetime of.
4. **No player-facing surface and zero prompt bytes** — it touches no template, no transcript, no widget, no delegate, and **no Zone A/B/C builder** ⇒ `AS-§3` (*the LLM emits symbols only; a log is not a template*) is untouched, and **Zone A/B/C are byte-unchanged, so this task's contribution to the batch's +250-char ceiling is exactly 0.**

⚖️ **AND THE ONE OBJECTION WORTH ANSWERING UP FRONT — "isn't logging it *keeping* it?" (`AS-§1`: *never keep a word of it*).** **No, and the reason is structural rather than a promise:** `AS-§1` bans a word of model output **surviving into another turn**, i.e. re-entering a prompt. A log line is **write-only and one-directional** — `ComposeTurnPrompt()` assembles Zone A + Zone B + Zone C from its own builders and **has no route to a log**, exactly as mechanism #2 records. ⇒ **There is no path, and no small edit that creates one, by which a printed byte reaches the model.** `Output` remains a function local that dies with the frame; **mechanism #1's *"NO MEMBER OF THIS CLASS EVER HOLDS MODEL OUTPUT"* is still literally true**, which is why the header needed no amendment. This argument is written into the comment block at the log itself, not left to this note.

## 7. WHAT THIS UNBLOCKS / WHAT COMES NEXT ON THIS FILE

- ⚠️ **TASK-548 (executor + `DescribeCommandForPlayer`) IS `blocked-by` THIS TASK AND OWNS `SiegeAssistantComponent.{h,cpp}` NEXT.** I was the **first and only** writer of this file for the batch; **548 takes the file from here.** ⛔ No other task in 541..547 touches it, so there is no merge surface.
- ⭐ **TASK-552 (Stage 5) IS THE CONSUMER.** With this landed, the log shows **exactly what the model emitted** for Jonathan's two sentences. ⚖️ **That either CONFIRMS the `AS-§21.1` diagnosis or FALSIFIES it, and both outcomes are results and are reported as results.** ⛔ **Nobody has ever seen this JSON** — this line is the first time it exists as an artifact.
- ⛔ **NO ACCURACY CLAIM IS ATTACHED TO THIS TASK**, and none may be derived from it before Stage 5 prints one (`AS-§12f`). **This task delivers an instrument, not a number.**

## 8. WHAT QA SHOULD SCRUTINISE

1. **The placement, both halves** — that it is **after** all three guards (never fires for a dropped/stale/failed completion) **and** **before** the parse call (fires on the malformed case too). Breaking either half silently removes the case the task exists for.
2. **The verbosity is `Log`** — ⛔ a "tidy-up" to `Verbose` destroys the feature outright (nothing prints by default) and to `Warning` fails the automation runner on the happy path. **Both refusals are already in the comment; a reviewer should confirm they are still there.**
3. **That `Output` is still only ever read** — grep the diff for an assignment, a member, or a second copy. **There should be none.**
4. **The `%d`/`%s` ↔ arg pairing and the literal format string** (§5 above) — the only compile-shaped risk in the change.
5. ⛔ **That nothing else in the file moved.** The intended diff is **one hunk** inside `HandleModelCompletion`. **Any second hunk in this file is not mine and should be questioned.**

## 9. 📌 M8 DECLARATION (verbatim, per `AS-§21.12`)

📌 **M8: adds no replicated property, no new replicated class, no new relevancy tier.**

⛔ **This feature adds no replicated property, no new replicated class, and no new relevancy tier.** ✅ **And the reason is structural: the change introduces no declaration of any kind — it reads an existing function parameter and writes it to a log, so there is no type, no field and no actor for replication to reach.** *"There is nothing to declare"* only counts when it is stated.

## 10. ANYTHING I THINK IS WRONG WITH THE SPEC

**Nothing blocking; the spec was implementable exactly as written.** Two notes recorded rather than acted on:

- ⚠️ **The scope line names `SiegeAssistantComponent.{h,cpp}` but the `.h` correctly needs no edit** (§1). I left it untouched deliberately — the header's two claims about `Output` survive this change verbatim, and amending them would have written a falsehood into the file that most needs to be true. **Flagged so QA does not read the untouched header as an omission.**
- ⚠️ **The dispatch asked me to "think about whether any truncation is needed" while the board's §4 says ⛔ do NOT truncate.** **No conflict in the end — I reached the board's answer independently and recorded the reasoning (§4).** But had they disagreed, **the board is the spec and the dispatch is the summary.**
