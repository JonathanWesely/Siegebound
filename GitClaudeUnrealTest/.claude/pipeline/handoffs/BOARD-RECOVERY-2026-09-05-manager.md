# 🚨 BOARD RECOVERY INPUT — `TASKBOARD.md` WAS FOUND **EMPTY (0 bytes)** 2026-09-05

**Author:** manager · **Marker:** `BOARD-TRUNCATION-2026-09-05` · **This file is an INPUT for the restore, not a report.**

## 0. THE INCIDENT, STATED WITHOUT GUESSING AT THE CULPRIT

- ⛔ **`\.claude/pipeline/TASKBOARD.md` is `0` bytes.** Confirmed three ways: `Read` returns *"file exists but the contents are empty"*; `Grep` for `TASK-1059`, `TASK-1067`, `TASK-1057-CL3A-CORRECTED` and `PIN-HARDEN-2]` all return **0 matches**; the marker `TASK-1055-BUILD-DONE`, which lived on the board, now resolves **only** in `handoffs/TASK-1055-buildmaster.md`.
- ⛔ **Timeline I can attest to:** three of my `Edit`s applied **successfully** (tool confirmed each). The **fourth** `Edit` on the same file failed with *"File has been modified since read"*, and every read after that point showed the file empty. ⇒ **the truncation happened between my third and fourth edit.**
- ⚠️ **I do NOT name a cause.** A build-master was running `TASK-1059` concurrently and holds that file legitimately; a linter is also named as a possible writer by the tool's own error text. **Attribute by evidence, not by adjacency** (`SC-§59`). I hold **no `Bash` and no git** (`SC-§78`), so I cannot inspect the reflog, the index, or the file's mtime.
- ⛔ **I did NOT attempt a from-memory rewrite.** I read roughly 800 lines of a ~30,000-line file. **Reconstructing it would be data destruction wearing the costume of a fix.**

## 1. 🚨 THE URGENT PART — READ THIS BEFORE ANY COMMIT

⛔⛔ **A live host is staging `.claude/pipeline/**` by named path. If it commits now it commits an EMPTY BOARD.**
⇒ ✅ **RESTORE FIRST, COMMIT SECOND.** The last known-good copy is in git: the board was staged into **`c4955cb`** (`TASK-1055`, which staged `.claude/pipeline/**`).

Suggested restore, **for the role that holds git** (I do not):
1. `git status --porcelain -- .claude/pipeline/TASKBOARD.md` — record what the index holds **before touching anything**.
2. ⛔ **If the file is NOT staged:** `git checkout -- .claude/pipeline/TASKBOARD.md` restores it to `c4955cb`'s content.
3. ⛔ **If an empty version is ALREADY staged:** unstage that ONE path by name (`git restore --staged --worktree -- <that one path>`) — ⛔ **never `git reset`, never `-a`** (`SC-§77a`).
4. Then re-apply §2 and §3 below, and re-check any status flips made after `c4955cb` (at minimum: `TASK-1058` → `qa-passed`, and whatever `TASK-1059` wrote).

## 2. THE DELTA I HAD ALREADY APPLIED AND THAT THE TRUNCATION DESTROYED — RE-APPLY VERBATIM

Three edits landed and are gone. **Locate by heading, never by line number.**

### 2.1 `TASK-1055` — one clause of build-master's own status is WITHDRAWN (append, do not rewrite)

Anchor: the end of `TASK-1055`'s `- status:` line, `` Handoff: `handoffs/TASK-1055-buildmaster.md`. `` — insert this as a new continuation line **immediately after it**, before the `🚦` line:

```
  ✅⚖️⛔ **⛔ MANAGER — ⛔ ONE CLAUSE OF THE ABOVE IS ⛔ WITHDRAWN, ⛔ APPENDED NOT REWRITTEN 2026-09-05 (`TASK-1057-CL3A-CORRECTED`): the ⛔ *"their handoff … is ⛔ SILENT on the 6 executable ADDED lines"* ⛔ IS ⛔ WRONG, and I ⛔ RELAYED IT ONWARD BEFORE CHECKING.** ⛔ **`qa/TASK-1058.md` §2.4 ⛔ measured the handoff: ⛔ §6.5 names the additions as ⛔ `TEXT(...)` message text ⛔ BY NAME and ⛔ §7 hands the host ⛔ *"the first execution of ⛔ BOTH changed literals"* ⇒ ⛔ ⛔ THE DISCLOSURE WAS ⛔ COMPLETE.** ✅ **⛔ EVERYTHING ELSE IN THIS STATUS ⛔ STANDS — ⛔ the ⛔ 8-line measurement is ⛔ CORRECT, ⛔ CORROBORATED, and it is ⛔ WHY the false label was caught. ⛔ The ⛔ FALSE LABEL WAS ⛔ THE MANAGER'S (⭐ `TASK-1057` cl. 3(a)), ⛔ NOT THE AUTHOR'S** (⭐ `SC-§93`).
```

### 2.2 `TASK-1057` — the status rider (append under `- status:`, author's words untouched)

```
  ✅ **GATED: ⭐ `TASK-1058` returned ⛔ PASS 2026-09-05 (⛔ 0 BLOCKERS, 3 WARN, 2 NIT) — `.claude/pipeline/qa/TASK-1058.md`. ⛔ HOST ⭐ `TASK-1059` ⛔ IN FLIGHT.**
  🚨⛔⛔⭐⭐ **⛔ MANAGER CORRECTION 2026-09-05 — ⛔ APPENDED, ⛔ NOT REWRITTEN (⛔ the author's own words above stay; marker `TASK-1057-CL3A-CORRECTED`): ⛔ THE ⛔ *"ZERO executable lines"* ABOVE IS ⛔ FALSE, AND IT IS ⛔ MY VOCABULARY THEY WERE HANDED — ⛔ cl. 3(a)'s parenthetical, ⛔ NOW CORRECTED ON THIS ROW.**
  ⛔ **⛔ MEASURED: ⛔ 8 EXECUTABLE LINES (⛔ 2 removed, ⛔ 6 added), whole-file ⛔ `545 → 549` — ⛔ `TEXT("…")` fragments in a ⛔ LIVE `TestTrue(FString::Printf(…), …)` argument** (`qa/TASK-1058.md` §2.2 + ⛔ F-1). ⇒ ⛔ **⛔ THE BRANCH DECLARATION ⛔ (a) ⛔ STANDS AND IS ⛔ CORRECT (⛔ opener predicate ⛔ UNTOUCHED at `:315-324` ⇒ ⭐ `SC-§83` ⛔ NEVER BOUND, ⛔ NO DECOY WAS OWED — ⛔ ⛔ DO NOT BOARD ONE). ⛔ What is wrong is the ⛔ LABEL; the diff ⛔ OWES A ⛔ COMPILE, ⛔ which is ⭐ `TASK-1059`'s.**
  ✅ **⛔ AND IT IS ⛔ NOT A FINDING AGAINST THE AUTHOR: their ⛔ DISCLOSURE WAS ⛔ COMPLETE (⛔ handoff §4 · §6.5 · §7) — ⛔ they named the removals as ⛔ message-literal lines, the additions as ⛔ `TEXT(...)` message text, and handed the host ⛔ *"the first execution of both changed literals"*** (⭐ `SC-§93`).
```

### 2.3 `TASK-1057` cl. 3(a) — THE CORRECTED CLAUSE (this is WARN F-1's second leg)

⛔ **Replace the single line**

```
      ✅ **(a) ⛔ DEFAULT — ⛔ SOFTEN THE MESSAGE to *"no ⛔ SPACED `// return;`"*.** ⛔ **⛔ Comment text. ⛔ Keeps this row at ⛔ zero executable lines.**
```

⛔ **with:**

```
      ✅ **(a) ⛔ DEFAULT — ⛔ SOFTEN THE MESSAGE to *"no ⛔ SPACED `// return;`"*.**
      🚨⛔⛔⭐⭐ **⛔ CORRECTED 2026-09-05 BY THE ⛔ MANAGER, ⛔ AFTER DELIVERY, ⛔ AGAINST ⛔ MYSELF (`qa/TASK-1058.md` ⛔ F-1 second leg, marker `TASK-1057-CL3A-CORRECTED`). ⛔ THE PARENTHETICAL THAT STOOD HERE WAS ⛔ FALSE, AND IT ⛔ ALREADY MISLED THE ONE AUTHOR WHO READ IT.**
      ⛔ ~~*"⛔ Comment text. ⛔ Keeps this row at ⛔ zero executable lines."*~~ ⇒ ⛔⛔ **⛔ WRONG. The `(c)` message is a `TEXT("…")` fragment in the ⛔ FIRST ARGUMENT of a ⛔ LIVE `TestTrue(FString::Printf(…), …)` CALL ⇒ ⛔ ⛔ EDITING IT IS ⛔ EDITING CODE.** ⛔ **⛔ The delivered diff is ⛔ 8 EXECUTABLE LINES (⛔ 2 removed, ⛔ 6 added; whole-file ⛔ `545 → 549`) and it ⛔ OWES A COMPILE** (⭐ `TASK-1059` ⛔ runs it).
      ✅ **⛔ WHAT (a) ⛔ ACTUALLY BUYS, ⛔ STATED CORRECTLY: it keeps this row ⛔ OUT OF ⭐ `SC-§83` — ⛔ NOT out of the ⛔ COMPILER.** ⛔ **(b) is defined by ⛔ CONTENT (⛔ folding `;` into the ⛔ OPENER PREDICATE); (a) leaves that predicate ⛔ BYTE-FOR-BYTE ⇒ ⛔ the pin's ⛔ DECISION PROCEDURE is ⛔ UNCHANGED ⇒ ⛔ ⛔ NO DECOY IS OWED AND ⛔ NONE MAY BE MANUFACTURED** (⛔ QA ⛔ read the predicate itself and ruled the ⛔ branch declaration ⛔ TRUE).
      ⭐⭐ **⛔ THE RULE, ⛔ CARRIED HERE SO THE NEXT AUTHOR MEETS IT AT THE ⛔ POINT OF USE (⛔ promoted to ⭐ `SC-§93` in the ⛔ SAME ACTION as this correction): ⛔ *Text inside `TEXT("…")` is ⛔ CODE, ⛔ never a comment — so editing it ⛔ ALWAYS owes a ⛔ COMPILE; whether it ⛔ ALSO owes an ⭐ `SC-§83` decoy depends on whether the ⛔ PIN'S DECISION PROCEDURE changed, ⛔ never on whether the edited text ⛔ reads like prose.*** ⇒ ⛔ **⛔ *"COMMENT-ONLY"* AND ⛔ *"BEHAVIOUR-NEUTRAL"* ARE ⛔ DIFFERENT PROPERTIES.**
      ⚖️⛔ **⛔ AND THE FAULT IS ⛔ THE SPEC'S, ⛔ NOT THE AUTHOR'S — ⛔ recorded on the ⛔ ROW and not only in the gate, because the ⛔ row is what the ⛔ next author reads: they ⛔ INHERITED THIS SENTENCE'S VOCABULARY while ⛔ DISCLOSING THE SUBSTANCE ⛔ ACCURATELY (⛔ handoff §4 names the removals as ⛔ message-literal lines · §6.5 names the additions as ⛔ `TEXT(...)` message text · §7 hands the host ⛔ *"the first execution of both changed literals"*).** ⛔ **⛔ A RELAY OF MINE THAT SAID THEY WERE ⛔ *"silent on the six additions"* IS ⛔ WITHDRAWN — ⛔ IT WAS ⛔ WRONG, and the ⛔ correction runs ⛔ IN THE AUTHOR'S FAVOUR.**
```

## 3. THE NEW ROW THAT NEVER LANDED — `TASK-1067` (WARN F-3)

Insert immediately **before** the `#### TASK-1045 — [PIN-HARDEN-2]` heading.

```
#### TASK-1067 — [EXPOSURE-TIGHT] ⚙️📌⛔⭐⭐ **THE ⛔ STRONG ARGUMENT LIVES IN A ⛔ DOCUMENT NOBODY READS AT THE ⛔ POINT OF USE, AND THE ⛔ WEAK ONE IS IN THE ⛔ CODE — ⛔ residual (iii)'s exposure proof sits in a ⛔ HANDOFF while the ⛔ FILE still offers the ⛔ `TEXT(`-ONLY REASON.** (gameplay-programmer) — ⭐ **NEW 2026-09-05, from `qa/TASK-1058.md` ⛔ WARN ⛔ F-3, marker `TASK-1067-EXPOSURE-TIGHT`**
- assignee: gameplay-programmer
- status: **boarded 2026-09-05** (`TASK-1067-EXPOSURE-TIGHT`) — ⛔ **NOT URGENT · ⛔ NOT DISPATCHED TONIGHT (⛔ dispatch stops after ⭐ `TASK-1059` lands) · ⛔ NOTHING DEGRADES IF IT SITS.**
  ✅⛔⛔⭐⭐ **⛔ COMMENT TEXT ONLY — ⛔ AND THAT CLASSIFICATION IS ⛔ MEASURED, ⛔ NOT ASSUMED, ⛔ WHICH IS THE ⛔ WHOLE POINT OF THE ROW ABOVE IT** (⭐ `SC-§93` cl. 3-4). ⛔ **The manager ⛔ OPENED THE FILE before writing these words: the target sits ⛔ INSIDE the doc block above `CodeWithoutTrailingComments` — ⛔ opener and closer ⛔ BOTH verified, ⛔ nothing between (corroborates `qa/TASK-1058.md` §6) — and the `TEXT(` that appears in the target sentence is ⛔ BACKTICKED PROSE, ⛔ NOT a literal.** ⇒ ⛔⛔ **⛔ A ⛔ `TEXT(` GREP OVER THIS FILE ⛔ HITS A ⛔ COMMENT. ⛔ RE-CONFIRM THE ENCLOSING BLOCK ⛔ YOURSELF BEFORE YOU TYPE ⛔ *"comment-only"* — ⛔ the row directly upstream of you got this ⛔ EXACTLY WRONG.**
  ⛔ **⛔ ONE FILE · ⛔ ONE SITE · ⛔ ZERO executable lines · ⛔ ZERO behaviour change.**
- blocked-by: 🚨⭐ **`TASK-1059` — ⛔ must have ⛔ COMMITTED, ⛔ not merely started** (⭐ `SC-§89` ⛔ rule 3 — ⛔ **ORDERING; ⛔ the edge is written on ⛔ BOTH rows, ⛔ never left in a coordinator's memory**).
  ⛔ **⛔ THE ARROW RUNS THE WAY IT LOOKS: ⛔ you edit ⛔ THE SAME FILE ⭐ `TASK-1059` IS ⛔ STAGING RIGHT NOW.** ⇒ ⛔ **⛔ writing first would ⛔ SILENTLY WIDEN A ⛔ GATED DIFF and ⛔ SPEND `qa/TASK-1058.md`'s verdict on content it ⛔ NEVER SAW — ⛔ the ⛔ *"a gate over a diff that changed after the verdict"* shape, ⛔ which is the ⛔ exact defect this lineage exists to clean up after. ⛔ It is also the ⛔ SAME edge ⭐ `TASK-1057` carried against ⭐ `TASK-1056`, ⛔ one lane back.**
  ⛔ **⛔ WHEN YOU RUN, that file must be ⛔ CLEAN and ⛔ COMMITTED, and its residual list must read ⛔ THREE. ⛔ If it is ⛔ DIRTY, or reads ⛔ TWO, the fence ⛔ FAILED ⇒ ⛔ ⛔ STOP AND REPORT; ⛔ do ⛔ NOT absorb anyone's work into your diff.**
- parallel-safe: ⛔ **no vs ⛔ ANY writer ⛔ OR STAGER of `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRetentionWiringTest.cpp`** · ⛔ **no vs ⭐ `TASK-1052`** (⛔ **it rewrites `CountOccurrencesInCode` ⛔ INSIDE this file; ⛔ you are ⛔ AHEAD of it — ⛔ edge written on ⛔ ITS row too**) · ✅ **yes vs ⭐ `TASK-1063` / ⭐ `TASK-1064`** (⛔ **⛔ DIFFERENT FILES — `SiegeFogVolumeTest.cpp` / `SiegeBrightSunTest.cpp` ⇒ ⛔ ZERO overlap**) · ✅ **yes vs art and vs anything outside `Source/`**
  ⚠️⛔ **⛔ ONE ATTRIBUTION NOTE IF YOU ⛔ DO RUN ALONGSIDE ⭐ `TASK-1063`/⭐ `TASK-1064`: ⭐ `TASK-1066` cl. 2 lists ⛔ YOUR file in its ⛔ named expected set as ⛔ NOT-ITS, ⛔ attributing it to ⭐ `TASK-1059`.** ⛔ **⛔ The ⛔ ACTION that row takes is ⛔ CORRECT either way (⛔ leave it unstaged), but the ⛔ ATTRIBUTION would be ⛔ STALE once ⭐ `TASK-1059` has committed** ⇒ ⛔ **⛔ YOUR OWN HOST MUST ⛔ NAME YOU. ⛔ Do ⛔ NOT let another lane's host stage your diff.**
  🚨⛔⛔ **⛔ STAGE BY ⛔ NAMED PATH, ⛔ NEVER BY DIRECTORY** (⭐ `SC-§77a`) — ⛔ **the ⛔ SILENT SWEEP is the hazard here, ⛔ not the STOP. ⛔ It ⛔ REDS NOTHING.**
- spec: >
    Law: ⭐⭐⭐ **`SC-§93`** (⛔ **NEW, added in the ⛔ SAME ACTION as this row — ⛔ THE governing law of this row's ⛔ classification**) · ⭐⭐⭐ `SC-§91` · ⭐⭐⭐ `SC-§89` · ⭐⭐ `SC-§83` · ⭐ `SC-§79` · ⭐ `SC-§67` · `TL-§5c` — ⛔ cited, ⛔ NOT restated.
    **(0) 🚨⛔⛔⭐⭐ THE DEFECT, ⛔ IN ONE SENTENCE, ⛔ AND IT IS ⛔ NOT *"the comment is wrong"*: ⛔ THE ⛔ CLAIM IN THE FILE IS ⛔ ALREADY TIGHT AND THE ⛔ REASON OFFERED FOR IT IS ⛔ NOT.**
      ⛔ **The residual (iii) entry asserts `UpdateState()`'s body holds ⛔ *"NOT ONE ⛔ STRING LITERAL"* — ⛔ a claim about ⛔ LITERALS OF ⛔ ANY SPELLING — and then justifies it with ⛔ *"the `TEXT(` census of `SummonedUnit.cpp` steps straight ⛔ OVER the body"*, ⛔ which is a claim about ⛔ ONE SPELLING.**
      ⇒ ⛔⛔ **⛔ THE REACHABLE MISREADING IS ⛔ SPECIFIC, and it is ⛔ QA's, ⛔ not mine: someone adding a ⛔ PLAIN `"…"` literal (⛔ not `TEXT(`) to `UpdateState()` reads that sentence, sees a `TEXT(` argument, and concludes ⛔ their literal is unaffected. ⛔ It is ⛔ not.**
      ⚖️ ***⛔ THE STRONG ARGUMENT IS IN A DOCUMENT NOBODY READS AT THE POINT OF USE; THE ⛔ WEAK ONE IS IN THE CODE. ⛔ THAT IS THE ⛔ WHOLE ROW.*** ⛔ **(⛔ The strong one is in `handoffs/TASK-1057-programmer.md` and `qa/TASK-1058.md` §4.2 — ⛔ ⭐ `SC-§67`: ⛔ this row does ⛔ NOT create a third source, it ⛔ MOVES the argument to the ⛔ point of use.)**
    **(1) ⛔ THE SITE — ⛔ LOCATE BY ⛔ TEXT, ⛔ NEVER BY LINE** (⛔ `qa/TASK-1058.md`'s `:279-281` is ⛔ DATED, and ⭐ `TASK-1059` ⛔ commits the file before you run):
      > ⛔ **the residual ⛔ (iii) entry inside the doc block above the ⛔ SYMBOL `CodeWithoutTrailingComments`, at the sentence beginning ⛔ *"Live exposure MEASURED ZERO: `UpdateState()`'s body holds NOT ONE string literal"*.**
      ⚠️⛔ **⛔ THERE ARE ⛔ TWO *"Live exposure MEASURED ZERO"* SENTENCES IN THAT BLOCK — ⛔ residual ⛔ (i)'s (⛔ the `[^ \t/]//` one) and residual ⛔ (iii)'s. ⛔ YOURS IS ⛔ (iii). ⛔ Disambiguate by the ⛔ FOLLOWING words, ⛔ not by the shared opening.**
    **(2) 🚨⛔⛔⭐⭐ THE DELIVERABLE — ⛔ AND THE ⛔ TRAP IS ⛔ NOT THE ARGUMENT, IT IS ⛔ HOW YOU SPELL IT: ⛔ ⛔ DO ⛔ NOT BAKE A ⛔ NUMBER INTO THE COMMENT.**
      ⛔ **The tight argument is an ⛔ ENUMERATE-AND-ELIMINATE over a ⛔ STRICT SUPERSET predicate: ⛔ every `"`-bearing line in the body span is a ⛔ WHOLE `//` COMMENT LINE, so ⛔ `CodeLinesOnly` drops ⛔ ALL of them at ⛔ STAGE 1, ⛔ before this stage ever runs** ⇒ ⛔ **⛔ NO literal of ⛔ ANY spelling — `TEXT("…")`, plain `"…"`, `R"(…)"` alike — ⛔ reaches this stage from that body.** ⇒ ⛔ **the hole is ⛔ ENUMERATED, ⛔ not merely ⛔ UNOBSERVED, and the zero is ⛔ TIGHT rather than a ⛔ FLOOR.**
      🚨⛔⛔⛔ **⛔ THE COUNT ⛔ `15` GOES IN YOUR ⛔ HANDOFF. ⛔ IT DOES ⛔ NOT GO IN THE ⛔ COMMENT.** ⛔ **⛔ A bare count in a comment ⛔ goes stale ⛔ SILENTLY on the next edit — that is ⛔ `qa/TASK-1058.md`'s ⛔ OWN WARN ⛔ F-2, filed against the entry ⛔ TWO ENTRIES ABOVE YOURS in the ⛔ SAME BLOCK.** ⇒ ⛔⛔ **⛔ WRITING `15` INTO THE FILE WOULD ⛔ RE-CREATE ⛔ F-2 ⛔ ONE ENTRY DOWN, ⛔ IN THE ACT OF FIXING ⛔ F-3. ⛔ WRITE THE ⛔ PREDICATE (`"` across the body span) AND THE ⛔ STRUCTURAL REASON; ⛔ let the reader ⛔ RE-MEASURE.**
      ✅ **⛔ QA's suggested clause, ⛔ OFFERED AS A ⛔ SHAPE AND ⛔ NOT AS DICTATION** (⭐ `SC-§92`'s ⛔ report-vs-file discipline — ⛔ **if you adopt a prescribed phrase, ⛔ VERIFY IT IS TRUE OF THE SHIPPED FILE ⛔ FIRST, and ⛔ say you did**): ⛔ *"…and ⛔ no literal of ⛔ ANY spelling: the body's ⛔ only quote-bearing lines are ⛔ whole `//` comment lines, ⛔ dropped by `CodeLinesOnly` ⛔ before this stage runs."*
    **(3) 🚨⛔⛔⭐ RE-DERIVE THE CENSUS AT YOUR ⛔ OWN INSTANT** (⭐ `SC-§91` — ⛔ **⛔ EVERY count in this incident came back ⛔ HIGHER; ⛔ a relayed zero is a ⛔ LOWER BOUND**).
      ⛔ **⛔ (a) ⛔ RE-DERIVE THE ⛔ SPAN yourself: `void ASummonedUnit::UpdateState()` ⛔ to the ⛔ FIRST COLUMN-0 `}` — ⛔ `ExtractFunctionBody`'s ⛔ ACTUAL `Find("\n}")` needle, ⛔ not an eyeballed brace.** ⛔ **(⛔ QA read `[:1459, :1793]` at ⛔ ITS instant — ⛔ DATED, ⛔ a ⛔ LOWER BOUND, ⛔ NOT your answer.)**
      ⛔ **⛔ (b) ⛔ CENSUS `"` ACROSS THAT SPAN and ⛔ CLASSIFY ⛔ EVERY HIT: ⛔ whole `//` comment line ⛔ or ⛔ not.** ⛔ **⛔ Report ⛔ YOUR OWN number and ⛔ YOUR OWN classification ⛔ in the handoff.**
      ⇒ 🚨⛔⛔ **⛔ IF ⛔ EVEN ONE HIT IS ⛔ NOT A WHOLE COMMENT LINE, ⛔ THE TIGHT ARGUMENT IS ⛔ FALSE AND YOU MAY ⛔ NOT WRITE IT. ⛔ THAT IS A ⛔ FINDING: ⛔ report it, ⛔ name the line, ⛔ STOP — ⛔ do ⛔ NOT reconcile it, ⛔ do ⛔ NOT weaken the sentence to fit, and ⛔ do ⛔ NOT quietly fall back to the `TEXT(`-only reason.**
    **(4) ⛔ WHAT MUST ⛔ NOT CHANGE — ⛔ THE BLOCK AROUND YOU IS ⛔ FRESHLY GATED AND ⛔ FRESHLY COMMITTED.**
      ⛔ **⛔ The residual list ⛔ STAYS ⛔ THREE. ⛔ The ⛔ SILENT markings stay. ⛔ The ⛔ *"MONOTONICALLY STRONGER / STRICT SUBSET"* paragraph stays ⛔ WORD FOR WORD — ⛔ it is a ⛔ cl. 5 deliverable of ⭐ `TASK-1057`, ⛔ graded ✅ by ⭐ `TASK-1058` §3, and ⛔ a residual list that reads as a ⛔ RETREAT is how the ⛔ next author talks themself into ⛔ WEAKENING THE PIN.**
      ⛔ **⛔ ZERO edits to residual ⛔ (i) or ⛔ (ii), to the ⛔ opener predicate, to `CodeLinesOnly`, to `CountCharacter`, to `ExtractFunctionBody`, or to ⛔ ANY `TEXT("…")` ⛔ ANYWHERE in this file.**
      ⛔⛔ **⛔ F-2 IS ⛔ NOT YOURS: `qa/TASK-1058.md` ⛔ WARN ⛔ F-2 (⛔ the ⛔ bare `5,763` and ⛔ `64` in residual ⛔ (i)'s entry) is ⛔ DECLARED-AND-FENCED, ⛔ deliberately ⛔ NOT boarded — ⛔ QA's own words: ⛔ *"does not hold the commit; ⛔ do not open a loop for it."*** ⇒ ⛔ **⛔ DO ⛔ NOT FIX IT ⛔ WHILE YOU ARE IN THERE.** 📌 **⛔ MANAGER NOTE, ⛔ SO A ⛔ SECOND VISIT TO ONE DOC BLOCK NEVER HAPPENS: ⛔ if F-2 is ⛔ ever boarded it ⛔ RIDES ⛔ THIS ROW'S ⛔ DISPATCH as a ⛔ second named clause, ⛔ never as a later row.**
    **(5) ⛔ ⭐ `SC-§83` DOES ⛔ NOT BIND — ⛔ AND ⛔ SAY SO ⛔ WITH THE REASON, ⛔ NOT AS A BARE ASSERTION** (⭐ `SC-§79`: ⛔ **⛔ *"§83 does not bind"* with no reason is ⛔ indistinguishable from ⛔ *"I skipped it"*).**
      ⛔ **⛔ THE REASON: you are editing a ⛔ DOC COMMENT. ⛔ A doc comment has ⛔ NO DECISION PROCEDURE ⇒ ⛔ there is ⛔ no pin behaviour to witness and ⛔ NO DECOY MAY BE MANUFACTURED.** ⛔ **⛔ Declare the suite delta ⛔ `0/0` and mark it ⛔ DECLARED-NOT-EXECUTED** (`TL-§5c`).
      🚨⛔ **⛔ AND THE ⛔ ONE THING THAT WOULD ⛔ FLIP THIS ROW INTO ⛔ CODE: ⛔ if you find yourself editing ⛔ INSIDE A `TEXT("…")`, ⛔ ⛔ STOP. ⛔ That is a ⛔ DIFFERENT ROW, it ⛔ owes a ⛔ COMPILE, and ⛔ it is ⛔ NOT BOARDED** (⭐ `SC-§93`).
    **(6) ⛔ SCOPE FENCE: ⛔ ONE FILE, ⛔ ONE SITE.** ⛔ **⛔ ZERO other `Source/` paths · ⛔ ZERO `.claude/pipeline/CONVENTIONS.md` (⛔ MANAGER's, ⭐ `SC-§82` WHO — ⭐ `SC-§93` is ⛔ ALREADY WRITTEN BY ME) · ⛔ ⭐ `TASK-1052`'s `From >= …Len()` ⛔ BREAK-GUARD must ⛔ NOT appear (⛔ if you ⛔ SEE it, the ⛔ ordering fence ⛔ FAILED ⇒ ⛔ STOP AND REPORT; ⛔ do ⛔ NOT adopt, remove or add it).**
    **(7) ⛔ NO COMMITS. ⛔ NEVER PUSH.**
    **Slack: ⚙️ Dev & QA (`C0BF0QZP3CN`, thread `1783116269.740549`), prefix `⚙️ GAMEPLAY-PROGRAMMER:`, ⛔ ONE line: ⛔ your ⛔ OWN body-span bounds + ⛔ your ⛔ OWN `"` census and its ⛔ classification + ⛔ confirmation you wrote ⛔ NO NUMBER into the comment + ⛔ confirmation the edit is ⛔ inside the block comment (⛔ how you checked), emoji + TASK-1067.**
- names: > `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRetentionWiringTest.cpp` (⛔ **SOLE WRITE — ⛔ residual (iii)'s exposure sentence ⛔ ONLY; ⛔ LOCATE BY SYMBOL + QUOTED TEXT, ⛔ NEVER BY LINE**) · `handoffs/TASK-1067-programmer.md` · ⛔⛔ **INPUTS, ⛔ AUTHORITATIVE: `.claude/pipeline/qa/TASK-1058.md` ⛔ F-3 + §4.2 (⛔ the ⛔ TIGHT ARGUMENT and its ⛔ independent re-derivation) · `handoffs/TASK-1057-programmer.md` (⛔ where the argument ⛔ currently lives)** · ⛔ **DO ⛔ NOT TOUCH: `.claude/pipeline/CONVENTIONS.md` · `Tests/SiegeUnitNoticeRangeTest.cpp` · `Tests/SiegeFogVolumeTest.cpp` · `Tests/SiegeBrightSunTest.cpp` · `FogVolume.{h,cpp}` · ⛔ ANY other `Source/` path** · ⚠️⛔⛔ **GATE + HOST: ⛔ OWED-AND-NAMED, ⛔ deliberately ⛔ NOT boarded tonight (⛔ the row is ⛔ not dispatched and ⛔ nothing waits on it).** ⛔⛔ **BINDING (⛔ the ⭐ `TASK-1052` form): a ⛔ QA row ⛔ AND a ⛔ build host must be ⛔ BOARDED IN THE ⛔ SAME ACTION AS THIS ROW'S ⛔ DISPATCH. ⛔ This row may ⛔ NEVER reach a commit ⛔ without both.** 📌 **⛔ CHEAPEST DISCHARGE, ⛔ RECORDED SO IT IS ⛔ NOT RE-DERIVED: if it is dispatched ⛔ ALONGSIDE ⭐ `TASK-1063`/⭐ `TASK-1064`, ⛔ widen ⭐ `TASK-1065` to a ⛔ THIRD verdict section and ⭐ `TASK-1066` to a ⛔ THIRD named staged path — ⛔ ONE gate, ⛔ ONE commit. ⛔ Left ⛔ UNCOUPLED tonight ⛔ on purpose: ⛔ coupling it now would put a ⛔ NOT-URGENT row inside a gate that is ⛔ already blocked on two others.** · Law: ⭐⭐⭐ `SC-§93` · ⭐⭐⭐ `SC-§91` · ⭐⭐⭐ `SC-§89` · ⭐⭐ `SC-§83` · ⭐ `SC-§79` · ⭐ `SC-§67`.
```

## 4. THE TWO EDGE COUNTERPARTS I NEVER GOT TO APPLY (`SC-§89` rule 3 — **the edge lives on BOTH rows**)

### 4.1 On `TASK-1052` — append to its `blocked-by:` block

```
  🚦⛔⭐ **AMENDED 2026-09-05 — ⛔ THE QUEUE AHEAD OF YOU GREW BY ⛔ ONE MORE, ⛔ AND THE EDGE IS WRITTEN ⛔ HERE AND ON ⛔ ITS ROW** (⭐ `SC-§89` rule 3): ⭐ **`TASK-1067`** — ⛔ **⛔ COMMENT TEXT in `Tests/SiegeFogRetentionWiringTest.cpp`, ⛔ the ⛔ SAME FILE whose `CountOccurrencesInCode` you rewrite, ⛔ and it is ⛔ ALREADY IN YOUR 18-LOOP CENSUS.** ⛔ **⛔ It must have ⛔ CLOSED, ⛔ THROUGH ITS OWN HOST, before you run** ⇒ ⛔ **⛔ when you run, that file should be ⛔ CLEAN and ⛔ COMMITTED. ⛔ If it is ⛔ DIRTY, the fence ⛔ FAILED ⇒ ⛔ STOP AND REPORT; ⛔ do ⛔ NOT absorb its work into your diff.** ⛔ **⛔ And it ⛔ STILL costs you nothing — ⛔ clause 1 stands ⛔ unchanged: ⛔ ZERO live callers trigger this defect today.**
```

### 4.2 On `TASK-1059` — append to its `parallel-safe:` line ⛔ **(do NOT touch that row's `status:` — its owner holds it)**

```
  🚦⛔⭐ **⛔ COUNTERPART ⛔ ADDED 2026-09-05 (⭐ `SC-§89` rule 3 — ⛔ the edge is on ⛔ BOTH rows): ⭐ `TASK-1067` is ⛔ SEQUENCED ⛔ BEHIND YOU — ⛔ a ⛔ COMMENT-ONLY correction to the ⛔ SAME FILE YOU ARE STAGING (`qa/TASK-1058.md` ⛔ F-3).** ⛔ **⛔ IT CHANGES ⛔ NOTHING YOU DO: it is ⛔ NOT dispatched, so its bytes ⛔ CANNOT be in your tree.** ⇒ ⛔ **⛔ IF YOU ⛔ NEVERTHELESS SEE THE ⛔ `TEXT(`-ONLY EXPOSURE SENTENCE ⛔ ALREADY REWRITTEN, the ⛔ ORDERING FENCE ⛔ FAILED ⇒ ⛔ ⛔ STOP AND REPORT — ⛔ your gated diff would have ⛔ SILENTLY WIDENED.**
```

## 5. WHAT SURVIVED — DO NOT RE-DO THESE

- ✅ **`CONVENTIONS.md` is INTACT.** ⭐ **`SC-§93`** landed at its heading (marker `SC-93-COMPILER-SEES`), `SC-§92` still directly beneath it. **Verified by `Grep` after the truncation.** ⇒ ⛔ **do NOT re-add it; a duplicate section is worse than the gap.**
- ✅ `qa/TASK-1058.md`, all `handoffs/`, and the source tree are untouched by this incident (only `TASKBOARD.md` was lost).
