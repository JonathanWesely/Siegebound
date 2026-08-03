# TASK-465 — handoff (gameplay-programmer)

**Status: `ready-for-qa`. Gate: TASK-469.**

**M8 DECLARATION, verbatim:** *adds no replicated property, no new replicated class, no new relevancy tier.*

⛔ **NOT COMPILED, NOT RUN, NO EDITOR, NO PIE, NO MCP, NO GIT, NO `Content/`.** Nothing in this file has been built since `cf8ef8e`; TASK-468 owns that. **No automation test has ever run in this repo.** Every claim below is about SOURCE.

---

## 1. THE FIX — one string, and the verb is now an argument

**Defect (qa/TASK-464.md WARN-4):** `Assistant_ShortfallCount` hard-coded **"Send"** on a row that also serves **Guard** and **Ambush**.

**Before → after** (`SiegeAssistantComponent.cpp`, `SiegeAssistantReasonTemplate`, case `ShortfallCount`):

```
"You asked for {Requested} {Kind} - {Available} can take that order. Send {Available}?"
"You asked for {Requested} {Kind} - {Available} can take that order. {Intent} {Available}?"
```

and one new named argument in the **single** formatting site, `PushMessage(Code, Args)`:

```cpp
FormatArgs.Add(TEXT("Intent"), SiegeAssistantComponentInternal::IntentDisplayText(Args.Command.Intent));
```

**That is the whole behavioural change: two lines.** No branch was forked, no row was duplicated, no call site moved.

### Why this satisfies §3 — the verb's provenance, traced
`{Intent}` comes from `IntentDisplayText(Args.Command.Intent)`. Its only input is `FSiegeAssistantMessageArgs::Command`, a struct pinned to `uint8`/`int32`/`FName` whose own header comment states there is **no slot in which a model-produced string could travel**. The verb is therefore **the parsed command's own intent**, resolved through a game-authored `NSLOCTEXT` switch — ⛔ **not derived from anything the model emitted.** It is the **same helper** that already produces `{Order}`, so `{Intent}` and `{Order}` cannot disagree about the word.

### Why not a per-intent row (the thing the spec barred, and I agree)
Three rows are three sentences to drift, and the drift would be invisible: each one reads correct in isolation. One parameterised row cannot go out of sync with itself.

### ⚠️ §20 — I TRACED QA's OTHER SUGGESTION AND REFUSED IT, AND THE TRACE CHANGED THE ANSWER
WARN-4 offered two hypotheses: *"a verb-neutral phrasing, or `{Order}`."* **`{Order}` is wrong, and not subtly.** At shortfall time `Command.Counts[PendingShortfallIndex]` **still holds the count that cannot be met** — it is only overwritten later, at `TryShortCircuitClarification`'s accept step. So `{Order}` would render *"Send 10 footman to mine_near"* — ⛔ **the sentence would offer back verbatim the very order it exists to say is impossible.** `{Intent}` takes the verb and nothing else. Recorded at the line as an anti-revert note, per §20's closing rule.

### §20 — the consequences of adding a shared format argument, traced before typing
1. **`{Intent}` is added for every reason code, not just this one.** Benign, and the mechanism is **already proven in this same function**: `PushMessage` has always handed `{Requested}`/`{Available}`/`{Kind}`/`{Order}` to rows that name none of them (*"Order cancelled."*). `FText::Format` ignores an argument the pattern does not reference. This is not an assumption — it is the shipped behaviour of the line directly above mine.
2. **No other reason row uses the token `{Intent}`** — swept the whole table. `DescribeCommandForPlayer` uses `{Intent}` in a **separate, function-local** `FFormatNamedArguments`; the two never meet.
3. **No double substitution.** `FText::Format` substitutes argument *values* into the *pattern*; it does not re-scan a substituted value for tokens. `{Order}` already contains a formatted intent and is unaffected either way.
4. ⛔ **The `Send` path is byte-identical to today.** `IntentDisplayText(Send)` is `"Send"`, in the same position. **The only sentences that change are Guard's and Ambush's — which are the two that were wrong.**
5. **No new player-facing literal was created.** The `NSLOCTEXT` census in the `.cpp` is **unchanged at 35 matches (34 sites + the comment token at the head of the file)**. WARN-5's count of ten out-of-table literals is **not increased** by this change — `IntentDisplayText`'s seven rows were already among the ten; they now feed one more template argument.
6. **Localization key unchanged** (`Assistant_ShortfallCount`), source string changed. No translations exist yet, so nothing is orphaned.

### The reachability proof for `{Intent}` on this path — ⚠️ verified BY SYMBOL, never by offset (§18c)
Every route to a `ShortfallCount` message carries a command whose intent is **guaranteed** to be one of Send / Guard / Ambush:

- `FindShortfall` **returns false** unless `Command.Intent` is `Send`, `Guard` or `Ambush` — its own intent gate, immediately after the null-`Snapshot` check.
- `EnterClarify(ShortfallCount, …)` is called from **exactly three sites**, all downstream of a `FindShortfall` that returned true:
  1. `HandleModelCompletion`'s shortfall branch — `ShortfallArgs.Command = Command` after `FindShortfall(Command, …)`.
  2. `TryShortCircuitClarification`'s **re-ask** branch — `NextArgs = PendingArgs`, a copy of the same command with only `Requested` changed.
  3. `TryShortCircuitClarification`'s **multi-kind** branch — after a second `FindShortfall(PendingArgs.Command, …)` returned true.
- **Nothing on the short-circuit path writes `Command.Intent`.** The only mutation is `PendingArgs.Command.Counts[PendingShortfallIndex] = Quantity`.

⇒ `IntentDisplayText` can never hit its empty default here, so **the sentence can never render with a missing verb.** And the default fails **safe** in any case: an unknown intent yields an *empty* word, never a *wrong* one.

### Blast radius — it is a sentence, and only a sentence
`Send` and `Guard` **both execute as `ESiegeGroupCommandType::Hold`** (`ExecuteCommand`'s switch → `ExecuteZoneOrder`). Nothing downstream keys off the display verb — which is precisely why nothing but a human reading the line could have caught this. The fix changes what the player is told and **nothing about what is executed.**

---

## 2. THE §22 SWEEP — the deliverable, stated as a result

**Shape swept for:** *a template (or any player-facing string) hard-coding a **verb**, a **place**, a **count** or a **unit kind** that its path does not guarantee.*
**Domain:** all 19 reason rows · all 5 state labels · `IntentDisplayText`'s 7 rows · `DescribeCommandForPlayer`'s 3 sentence frames · `SiegeAssistantConsoleWidget`'s 4 chrome literals · and — per §22's *"runtime log strings are the priority surface"* — the assistant lane's `UE_LOG` string literals and `SiegeAssistantSnapshot` / `SiegeAssistantVocabulary` prompt text, grepped for every shipped verb inside a string literal.

### ✅ RESULT: the reason-code template table contains **NO second instance**. ShortfallCount was the only row of the 19 that hard-coded a word its path does not guarantee.

Rows that *look* like candidates and are **not**, each checked at the path rather than at the string:

| row | why it holds |
|---|---|
| `DeferredExpired` — *"Stopped waiting for {Kind}."* | The deferred fork is gated on `Command.TriggerKind != NAME_None`, so `{Kind}` is guaranteed non-`None` on every route to this row. |
| `DeferredArmed` — *"Waiting for {Requested} {Kind}, then: {Order}"* | Count, kind and verb all parameterised. |
| `ConfirmPrompt` / `Executed` | The verb arrives inside `{Order}`, from the same `IntentDisplayText`. |
| `AskUnsupported` | Deliberately says what the **assistant** cannot do rather than diagnosing the board, because three different paths land on it. Its comment already states this. |
| `RefusedNoAuthority` | Character-for-character the keys' lockout; a shared localization key by design. |

### ⚠️ THREE THINGS THE SWEEP FOUND THAT ARE **NOT** THE TABLE — reported, and two deliberately left

**(a) ⚠️ A REAL INSTANCE OF THE SAME SHAPE, AND IT IS ALSO PLAYER-VISIBLE — `DescribeCommandForPlayer`'s preposition. LEFT UNFIXED ON PURPOSE; I want a ruling, not a silent redesign.**

`Assistant_Order_SelectionPlace` is **`"{Intent} {Selection} to {Place}"`** — one frame for all four selection-bearing verbs. **`to` is correct for Send and Follow and wrong for Guard and Ambush:**

> `Guard 8 footman to mine_near - accept?`
> `Ordered: Ambush 8 footman to ancient_ground_near`

This is **the same shape as WARN-4** (a word true of only some of the frame's callers) and **the same visibility class** — it renders inside `{Order}`, so Jonathan meets it on the confirm line and the executed line of **every** guard/ambush order, with the toggle either way. ⛔ **I did not change it, for three reasons, and I would rather be told to than assume:**
1. **It is one of WARN-5's ten out-of-table literals**, and my dispatch says in terms that WARN-5 *"is a different finding and not yours to fix."* Editing that literal is a lane collision, not a sweep.
2. **Both repairs have consequences that outrank a wording call.** Forking into per-intent frames is **exactly what this task forbids for the shortfall row** — I will not do to one string what I was told not to do to another. Substituting a connector word (`{Intent} {Selection} {Preposition} {Place}`) fixes the English and **breaks the localization**: a preposition handed to a translator as a standalone fragment has no case, no gender and no word order, and §3's whole premise is that a **sentence** is the localizable unit.
3. **It is cosmetically dominated by a decision already ratified:** `{Place}` prints the raw symbol (`mine_near`), by the documented no-display-name-table rule. Polishing the preposition on a line whose noun is `mine_near` is not obviously the right first move.

⇒ **Recommend a manager ruling / a boarded task.** If it should ship before TASK-448, the cheapest correct move I can see is a **verb-neutral frame** (e.g. `"{Intent} {Selection} - {Place}"`), which stays one string, stays one localizable sentence, and is true for all four verbs. **Say the word and it is a two-line change.**

**(b) ✅ CHECKED AND TRUE TODAY, anchored in the code rather than only here — `AskWhichIntent`'s seven-verb list.**
*"What should they do - send, guard, ambush, follow, charge, fall back or rally?"* enumerates `ESiegeAssistantIntent` **by hand**. **Counted at the enum: exactly seven non-`None` members, and the sentence names all seven.** ✅ Correct — a **result**, not a finding. But nothing enforces it, and the same is true of `IntentDisplayText`'s switch. Per §20's *"when the obvious repair is wrong, the code must say so AT THE LINE"*, I added **comments only** at both sites recording the hand-maintained coupling and why it is not joined from the enum at runtime (an assembled comma-and-*"or"* list is not translatable). ⛔ **Zero behaviour change; declared here rather than buried.**

**(c) ✅ CHECKED, CORRECT, LEFT ALONE — `ExecuteZoneOrder`'s log ternary.**
`GroupType == Ambush ? TEXT("AMBUSH") : TEXT("HOLD")` looks like a defaulting hazard, because `ESiegeGroupCommandType` has **three** members (`Hold`, `Ambush`, `Follow`) and a `Follow` group would print as `HOLD`. **Verified at the call sites, not assumed: `ExecuteZoneOrder` is called from exactly two places, with `Hold` and with `Ambush`.** `Follow` routes to `ExecuteFollowOrder` and never reaches this line. ⇒ **The log is true on every reachable path.** Also out of my lane (`no executor changes`).

**Also seen, not mine:** `SiegeAssistantConsoleWidget`'s `IdleStatusText = "Ready"` duplicates `SiegeAssistantStateLabel(Composing)`'s *"Ready"* in a second file — a §3/WARN-5-family duplication, **not** an instance of my shape. The prompt-side verb enumerations in `SiegeAssistantSnapshot.cpp` / `SiegeAssistantVocabulary.cpp` are **model-facing Zone A text** that must literally list the grammar's symbols; all seven verbs are present and correct.

---

## 3. Files touched

- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp`
  - `SiegeAssistantReasonTemplate` → case `ShortfallCount`: `Send` → `{Intent}`, plus the anti-revert comment.
  - `SiegeAssistantReasonTemplate` → case `AskWhichIntent`: **comment only** (anti-drift anchor).
  - `PushMessage(Code, Args)`: one `FormatArgs.Add` for `{Intent}`.
  - `SiegeAssistantComponentInternal::IntentDisplayText`: **doc comment only** (it is now the `{Intent}` source; the default fails safe).
- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.h`
  - `ESiegeAssistantReasonCode::ShortfallCount` doc — **§22 applied to my own change**: it said *"Filled from Requested / Available / Kind"*, which my edit made stale. Now names the intent too.
  - `SiegeAssistantReasonTemplate` doc — the named-argument list now includes `{Intent}`, plus the general rule: **a word true of only some of a row's callers is an argument, never a literal.**

**No other file. No asset. Nothing under `Content/`.**

### ⚠️ TASK-463's work is UNDISTURBED — verified by symbol
`GetVocabulary`'s `NewObject` fallback, `BeginPlay`, and both `EnsureStaticPrefixRegistered` call sites are **byte-unmodified**; all four of my `.cpp` edits are inside the template table, `PushMessage`, and one doc comment, none of which 463 touched. ⚠️ **The board itself moved ~79 lines under me mid-task** (TASK-465's header went 6252 → 6331 between two reads), which is the standing write-race — **every citation in this note is by symbol, and no line number here should be trusted after the next edit.**

---

## 4. FOR QA — what to scrutinise, and what I could not check

1. ⛔ **Not compiled.** `FFormatNamedArguments::Add(FString, FFormatArgumentValue)` taking a `const FText&` is the **identical construction** on the four lines above mine and on `DescribeCommandForPlayer`'s three, so I expect no conversion issue — but that is a source argument, not a build.
2. **Confirm the reachability proof independently** (the `FindShortfall` intent gate + the three `EnterClarify(ShortfallCount, …)` sites). If a fourth route to `ShortfallCount` exists that I missed, `{Intent}` could render empty — a missing word, never a wrong one, but worth the check. **Use a positive control on the search, per §14.**
3. **Rule on sweep item (a)** — `"{Intent} {Selection} to {Place}"`. It is a real instance of this task's shape, it is player-visible before the playtest, and I left it deliberately with reasons. ⚖️ **Disagreeing with my restraint is a legitimate outcome; I would rather it be argued than defaulted.**
4. **Latent, unreachable, and NOT mine — flagged so it is on the record:** `MarkAssistantFaulted` broadcasts `SiegeAssistantReasonTemplate(ShownReason).ToString()` **without** `FText::Format`, so any templated row used as a fault reason would show the player literal `{…}` braces. **Unreachable today** — the latch has zero callers (WARN-1) and its only reasons are placeholder-free rows. ⛔ **My change does not widen this**: `ShortfallCount` already carried three placeholders. **It belongs to TASK-466's lane**, which owns the latch next in the same file.
5. **Not accuracy progress.** This changes what the game says, not what the model emits. **Bar #5 stands uncleared at 20/25 vs 22.** **WARN-5 remains instrumented, not discharged.**
