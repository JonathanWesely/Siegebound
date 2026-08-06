# TASK-541 — the `defend` note repair — programmer handoff

- **task:** TASK-541 [AC-1] — the `defend` note repair, −5 chars
- **assignee:** gameplay-programmer
- **status on completion:** `ready-for-qa`
- **QA gate:** ⛔ **TASK-550** (`qa/TASK-550.md` — the batch's only gate; this task names it)
- **date:** 2026-08-05
- **law:** CONVENTIONS `AS-§21.1` · `AS-§21.2` · `AS-§21.3` · `AS-§12f` · `AS-§20.4` · `SC-§18c` · `SC-§22`
- **compile / Git:** ⛔ none. TASK-551 owns the only compile and the only commit.

---

## 1. FILES TOUCHED — one, and the diff is two hunks

`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantVocabulary.cpp` — **the only file edited.**

| hunk | what | prompt-byte cost |
|---|---|---|
| **A** | the `[notes]` line in `BuildSynonymTable()` — the spec'd −5 | **−5 chars of Zone A** |
| **B** | the `// --- INTENTS ---` comment block in `USiegeAssistantVocabulary::USiegeAssistantVocabulary()` — board spec item (2), the `SC-§22` stale-divergence fix | **0** (a C++ comment; it is not in any prompt string) |

⛔ **No test file touched. No `Docs/Data/*.csv` opened. No vocabulary row, alias or second rule line added.** `Plugins/SiegeLlama/**` untouched.

⚠️ **SCOPE NOTE FOR QA — the dispatch and the board disagreed, and I followed the board.** The dispatch prompt said *"one line"*; the board spec's item **(2)** additionally requires rewriting the comment block that is the second home of the false claim. The board is the spec of record, the comment is inside this task's exclusively-owned file, and it costs **zero** prompt bytes — so both were done. If QA reads the narrower scope as binding, hunk B is the finding, not hunk A.

## 2. THE EDIT — located by symbol (`SC-§18c`), never by line offset

Located by searching for the substring `defend = ambiguous between guard and fallback`; it occurred **exactly once** in `Source/GitClaudeUnrealTest/Siegebound/` outside `Tests/`.

**BEFORE** (the full `Table += TEXT(...)` payload, `\n` shown as the literal two-character escape it is in source, counted as **one** character):

```
send, guard, ambush, follow take a unit list. charge, fallback, rally move the whole army or the hero and take who = none. defend = ambiguous between guard and fallback -> ask which_intent.\n
```

**AFTER:**

```
send, guard, ambush, follow take a unit list. charge, fallback, rally move the whole army or the hero and take who = none. defend with units -> guard. defend alone -> ask which_intent.\n
```

| | incl. trailing newline | excl. trailing newline |
|---|---|---|
| **before** | **190** | 189 |
| **after** | **185** | 184 |
| **Δ** | **−5** | −5 |

✅ The frozen clause ahead of it — `send, guard, ambush, follow take a unit list. charge, fallback, rally move the whole army or the hero and take who = none.` — is **byte-identical**, verified by string comparison, not by eye.
✅ **ASCII-only:** the reconstructed post-edit Zone A is `all(ord(c) < 128)` = **True**. No curly apostrophe, no em dash, no `->` replaced by an arrow glyph.

## 3. ZONE A — 5424 → **5419**, AND I MEASURED IT RATHER THAN SUBTRACTING FROM A HANDOFF

⭐ **The new Zone A total is 5419 characters (= 5419 UTF-8 bytes, ASCII-clean).**

⛔ **How it was measured, because "5424 − 5" would have been arithmetic on a number I took on trust.** No compiler and no model were used; the whole figure is a count of characters produced from **literals extracted programmatically out of the shipped source**, never re-typed:

1. Extracted all **98** `Out += TEXT("…");` literals from `Tests/SiegeAssistantZoneATest.cpp`'s frozen `BuildSpikeLaneZoneA()` fixture and C++-unescaped them → **5116**, which **reproduces the fixture's own pinned `SpikeLaneZoneAChars`.** That agreement is what validates the instrument.
2. Applied the three declared `D4` edits, each read from its own named literal in that file: `WHO =` line **+44**, `D4_RuleAllUnits` **+111**, `D4_RuleExclusion` **+153** → **+308** → **5424**, reproducing `ShippedZoneAChars`.
3. Substituted the pre-edit `[notes]` line with the post-edit line **read back out of the live `SiegeAssistantVocabulary.cpp`** → **5419**.

⇒ Both pinned baselines reproduce from source before my edit is applied, and the same instrument reports **5419** after it. **Δ = −5, as specified; nothing to escalate.**

📌 The throwaway script lives in this session's scratchpad (`zonea_count.py`); it is **not** committed and must not become project infrastructure — it is a one-shot measurement, and `Siege.Llama.SpikePrompt` remains the only thing that prints real figures with a model resident.

⛔ **TOKENS ARE NOT DERIVED.** `zoneA_tok` stays **STALE — PENDING RE-MEASUREMENT** (`AS-§12g`, `AS-§20.4`). Characters are countable offline; converting −5 chars into a token delta would be a derivation wearing a measurement's authority. **No token figure appears in this handoff.**

## 4. ⛔ THE TWO SHIPPED TESTS THIS BREAKS — EXPECTED, AND ⛔ NOT MINE TO FIX

Both will go **RED** at TASK-551's compile-and-suite, and that is the designed outcome, not a regression:

1. **`Siegebound.Assistant.ZoneA.MeasuredCharCount`** — asserts `ShippedZoneAChars == 5424`. **The value it must be re-based to is `5419`, which I measured (section 3).**
2. **`Siegebound.Assistant.ZoneA.TwoLaneByteEquality`** — asserts the shipped lane is the frozen spike fixture plus exactly the three `D4` edits. My edit is a **fourth** divergence, so it fails **exactly as that block's own comment says a fourth undeclared edit should** ("it still fails, loudly and with a character offset, on any fourth edit to Zone A"). The instrument is working.

⛔ **TASK-549 owns the re-base.** ⛔ **I did not touch `Tests/`.**

⚠️ **AND THE STANDING PROHIBITION, RESTATED BECAUSE IT IS THE EASY WRONG FIX:**
- The **spike lane stays 5116**. `Plugins/SiegeLlama/**` was deliberately **not** touched — it is another batch's in-flight instrument (`FT-§16`, TASK-481). The divergence is **`D4`, declared and now widened**, not a bug to reconcile.
- ⛔ **NEVER re-copy `BuildSynonymTable()`'s / `BuildZoneA()`'s output into the frozen fixture.** A test transcribed from its own subject is a guardrail that reports SAFE. The correct re-base declares my line as a **new, named component** of the diff, exactly as the three `D4` literals are declared today.

## 5. ⛔ `AS-§12f` SURVIVAL STATEMENT — the whole justification, and the only one permitted

> `AS-§12f`, **quoted from the artifact** (`CONVENTIONS.md:1208` and `:1209`, both verified first-hand 2026-08-05):
> *"⛔ **THE LAW — IT BINDS EVERY FUTURE PROMPT-LEVEL TASK ON THIS MODEL: NO SUCH TASK MAY BE JUSTIFIED BY NAMING THE ROWS IT WILL FIX.**"* … *"⇒ ⚖️ **RATE IS UNDER PROMPT CONTROL; SELECTION IS NOT.** A task whose case is 'this fixes row X and row Y' is a lottery ticket with a name on it, and **it is failed at the gate.**"*

⚠️ **A CITATION NOTE FOR QA, RAISED BECAUSE THIS BATCH IS STRICT ABOUT QUOTE FIDELITY AND ITS CITATIONS HAVE DRIFTED.** `AS-§21.2` (`CONVENTIONS.md:2864`) renders the first fragment as *"NO PROMPT-LEVEL TASK MAY BE JUSTIFIED BY NAMING THE ROWS IT WILL FIX"* inside quote marks. **The line range 1208-1209 is CORRECT and the substance is identical**, but that fragment is a **compression, not a verbatim string** — the artifact says *"NO SUCH TASK…"* after naming prompt-level tasks in the preceding clause. ⛔ **Nothing turns on it** and I did not edit `CONVENTIONS.md`; it is reported, not patched.

⭐ **This edit survives that law because its case names no row:**

> **Two shipped prompt lines assert contradictory things about the same input class. One is unconditional; the other is conditioned on a harm the first makes impossible. The deliverable is the REMOVAL OF A CONTRADICTION — verifiable as a diff, at NEGATIVE character cost.**

Concretely, and verified first-hand at both artifacts today: `USiegeAssistantSnapshot::BuildZoneA` teaches, unconditionally, *"If the player names units, the intent is send, guard, ambush or follow, never charge, fallback or rally."* The `[notes]` line's stated harm — *"picking one would move an army the player never mentioned"* — is therefore **structurally impossible whenever a selection is present**, because `fallback` is already unreachable there. **The note was true when written; a later edit elsewhere in the prompt falsified its antecedent; nobody re-read it.**

⛔⛔ **NO ACCURACY FIGURE IS ATTACHED TO THIS TASK, ANYWHERE.** No predicted score, no expected improvement, no percentage, no row list — not in the code, not in the comment, not in this handoff, not in the Slack post. **The honest statement is that nobody has ever seen what the model emits for either wording** (which is what TASK-542's output log exists to fix). ⛔ **If a later artifact adds one, deletion is the fix.**

## 6. WHY THIS EXACT WORDING — recorded so it is not "improved" later

- **Char-negative**, so it spends none of the batch's Zone-A budget.
- **House arrow notation**, matching the two `[notes]` lines above it verbatim in style.
- **A testable antecedent, then one instruction** — the decision-ordering shape `AS-§20.4` records as the instrument this seam has already responded to.
- It **removes a competitor** rather than adding emphasis, which is the lever with recorded evidence behind it at this seam.
- ⛔ **`defend` was NOT made an eighth intent and NOT added as an alias** — `AS-§21.3` refuses both, and hunk B now records that refusal at the artifact so it is not re-proposed by the next reader of this file.

## 7. 📌 M8 DECLARATION — verbatim

> **adds no replicated property, no new replicated class, no new relevancy tier.**

This edit changes the contents of one string literal and one comment. There is no new field, no new class, no `UPROPERTY`, no `Replicated` specifier, and no change to what crosses the wire.

## 8. WHAT QA SHOULD SCRUTINISE (TASK-550)

1. ⛔ **Hunt for an accuracy claim and fail this task if one exists** — in the diff, this handoff, or the Slack post. Deletion is the fix (`AS-§21.2`).
2. **Re-count the two strings yourself.** 190 → 185 including the newline. A mismatch means the line is not what the spec thought it was, and that is a STOP, not a patch.
3. **Confirm the frozen clause ahead of `defend` is byte-identical** and that no character outside `defend = ambiguous between guard and fallback -> ask which_intent.` moved.
4. **ASCII.** One non-ASCII character inside the `TEXT("…")` literal breaks every char-based figure in the feature. The comment block (hunk B) intentionally contains `⚠️ ⛔ ⇒ —` in the house style — that is a **comment**, not prompt content, and `ZoneA.AsciiCleanliness` tests the built string, not the file.
5. **Hunk B's citation.** It cites `SiegeAssistantSnapshot.cpp:1050` **with an explicit `as of 2026-08-05` date stamp**, and I verified that line first-hand today; the rule text is also quoted inline so the claim survives the line number drifting (`SC-§18c` — citations across this batch have drifted).
6. ⛔ **Do NOT record the two red Zone-A tests as a defect of this task.** They are TASK-549's re-base, and 5419 is the number to re-base to.
