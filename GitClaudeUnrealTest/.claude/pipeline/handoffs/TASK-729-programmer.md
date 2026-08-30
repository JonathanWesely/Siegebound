# TASK-729 — the `Ctrl+Z` sentence at `SiegeControlsHelpWidget.cpp` — VERIFIED ✅, sentence KEPT, citation + warning added

**Board step (2), branch ✅ CONFIRMED** — the spec offered exactly two: ✅ confirmed ⇒ the sentence stays and gains its citation; ⛔ refuted/undeterminable ⇒ restate. **It is CONFIRMED at engine source.** The player-facing string is therefore **unchanged, byte-for-byte**, and the whole delivery is **one 17-line comment**.

## (1) VERIFY AT SOURCE — the claim is TRUE, and here is the `file:line`

The sentence asserts *"Ctrl+Z and Ctrl+Shift+Z are the text box's own undo and redo"*. Both halves verified in **UE 5.8 engine source**:

| Claim | Engine source | Exact code |
|---|---|---|
| `Ctrl+Z` = undo | `Engine/Source/Runtime/Slate/Private/Framework/Commands/GenericCommands.cpp:19` | `UI_COMMAND(Undo, "Undo", ..., FInputChord(EModifierKey::Control, EKeys::Z))` |
| …and that command is wired to the shipped text box | `Engine/Source/Runtime/Slate/Private/Widgets/Text/SlateEditableTextLayout.cpp:154-157` | `UICommandList->MapAction(FGenericCommands::Get().Undo, FExecuteAction::CreateRaw(this, &FSlateEditableTextLayout::Undo), FCanExecuteAction::CreateRaw(this, &FSlateEditableTextLayout::CanExecuteUndo))` |
| `Ctrl+Shift+Z` = redo | `Engine/Source/Runtime/Slate/Private/Widgets/Text/SlateEditableTextLayout.cpp:1170-1172` | `else if (CanExecuteRedo() && ((Key == EKeys::Y && InKeyEvent.IsControlDown()) \|\| (Key == EKeys::Z && InKeyEvent.IsControlDown() && InKeyEvent.IsShiftDown()) \|\| ...))` |

⇒ The help text tells Jonathan the truth. **⛔ No restatement was warranted**, and restating it would have *removed* a true, now-cited statement.

⚠️ Note for the record: the citation was **not invented here** — `SiegeAssistantConsoleWidget.cpp:868-878` already carried `FSlateEditableTextLayout::HandleKeyDown:1168-1176` beside the shipped guard. I re-verified it independently against the installed 5.8 engine rather than trusting it; the redo line is **1170-1172** in this install (the neighbouring `1168-1176` window is the same `else if` block, so the existing citation is not wrong, just wider). The **undo** half was previously uncited anywhere — that is the genuinely new citation.

## (2) THE ⭐ REAL FINDING — `qa/TASK-708.md` W-1 is CORRECT, and 707 §3.1 is FALSE for this sentence

`USiegeKeyboardLayoutSubsystem` **never sees these two chords.** Slate dispatches both itself against a **hardcoded `EKeys::Z`** — there is no `GetPositionalKey` in either path (checked: the subsystem has zero references to `SlateEditableTextLayout`, and its only `EKeys::Z` mentions are the accept-key doc comments at `SiegeKeyboardLayoutSubsystem.h:232`/`:234`/`:269`).

⇒ **This `Z` does NOT follow the accept key's pinned position.** TASK-707 handoff §3.1 — and the in-file note at `:927-929` — claim an F-1 reversal moves *"this prose"* to `{Interface.AssistantAccept}` tokens. ⛔ **That is false for this one sentence, and acting on it would print a key the player does not press** — the exact defect the tokenisation work exists to prevent. The shipped code is correct; what was missing was a warning where the next author will look.

⚖️ **The carve-out is the two `Ctrl` mentions ONLY.** The same sentence's trailing *"Shift+Z is accepted"* clause **IS** the accept key and **DOES** follow the pin — so a future tokenisation pass must split this sentence, not exempt it wholesale. The comment says so explicitly, because the opposite error (exempting the whole sentence) is just as available to the next reader.

## (3) WHAT CHANGED — one comment, `+17 / -0`

**File:** `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp`, inserted between the old `:938` and `:939`, at the site, inside the `Row.Detail` literal-concatenation (legal C++: comments die in translation phase 3, literal concatenation happens in phase 6).

`git diff --stat` = **1 file changed, 17 insertions(+), 0 deletions(-)**. ⛔ **Zero deletions and zero modified lines — nothing pre-existing was edited.**

## ⛔ FENCES — all held

- ⛔ **The `Ctrl+Z` player-facing string is UNTOUCHED**, byte-for-byte (0 deletions proves it).
- ⛔ **No behaviour.** No key handler added/removed/altered; the modified-press branch (`SiegeAssistantConsoleWidget.cpp:876-877`) untouched; **`Escape` not touched** (`HELP-§5`); **`Shift+Z` still accepted**.
- ⛔ **No other file.** Only `SiegeControlsHelpWidget.cpp` is mine. (`Docs/Data/cards.csv` + `Docs/setupdirections.md` are dirty in the tree from **other tasks in this batch — not mine**.)
- ⛔ **No test changes ⇒ suite total stays 156.** **No delta declared** for TASK-731's gate.
- ⛔ No compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ `Tools/Packaging/` and TASK-716/700 untouched.
- ✅ **Encoding checked:** file is BOM-less UTF-8 both at `HEAD` and after the edit (first 3 bytes `2f 2f 20`). The added emoji/em-dashes match the block's existing style, which already ships in this file and already compiles.
- ✅ **`:927-929` citation is stable:** the insert is *after* :929, so the cited lines did not shift (re-read post-edit to confirm).

## 🔍 WHAT QA SHOULD SCRUTINISE

1. ⭐ **The carve-out's accuracy** — is *"Shift+Z is accepted"* really the accept key (⇒ tokenisable) while the two `Ctrl` mentions are not? If that split is wrong, the comment is wrong in the more dangerous direction.
2. **The three engine citations** against your own 5.8 install — `GenericCommands.cpp:19`, `SlateEditableTextLayout.cpp:154-157`, `:1170-1172`. A citation that drifts is the M7.7 lesson repeating.
3. **That `:927-929` is still the F-1 reversal note** the comment points at (verified post-edit, but it is a line-number reference and those rot).
4. ⛔ **That the diff really is 0-deletion** — i.e. the help string a player reads is identical to what shipped.
5. ⚠️ **Left deliberately un-edited:** the stale claim at `:927-929` itself still says "this prose would then move to tokens". Per the dispatch fence (*that one comment site only*) I did **not** amend it; the new comment carves out the exception directly beneath it and cites it by line. **If QA prefers the correction at the source of the false claim, that is a one-line follow-up and I'd take it** — I read the fence as forbidding it here.

---

# ADDENDUM — the fence widened by one site: the stale claim CORRECTED AT SOURCE

⭐ **Coordinator ruling (post-delivery): take the follow-up.** Rationale accepted as the governing one — *a false comment a reader meets FIRST is this project's named stale-symbol trap* (same class as the inert `bUseLoggingInShipping` ini key, and the phantom symbol that replicated to six sites because it sat in law). **A reader who stops at `:927-929` never reaches the carve-out below.** The correction has been moved to where the claim lives.

## THE CORRECTED TEXT AT `:927-930` (was `:927-929`)

**Before (the false claim, verbatim):**
```
// "Press Z" would be worse than either choice alone. ⭐ The reversal is still exactly one
// flag: bLiteralKeyLabel = false derives the chip, and this prose would then move to
// {Interface.AssistantAccept} tokens — a `KBD-§8` amendment, ⛔ not a help edit.
```

**After — ⭐ BOTH TRUTHS KEPT.** The true half (an F-1 reversal DOES move the accept-key mentions to tokens) survives verbatim in substance; the one word doing the damage was the unqualified **"this prose"**, now narrowed to **"the ACCEPT-KEY mentions in this prose"**:
```
// "Press Z" would be worse than either choice alone. ⭐ The reversal is still exactly one
// flag: bLiteralKeyLabel = false derives the chip, and the ACCEPT-KEY mentions in this
// prose would then move to {Interface.AssistantAccept} tokens — a `KBD-§8` amendment,
// ⛔ not a help edit.
// ⛔⛔ CORRECTION, AND READ IT BEFORE ACTING ON THE PARAGRAPH ABOVE (708 W-1): ⛔ NOT EVERY
// `Z` IN THIS PROSE IS AN ACCEPT KEY, SO THE REVERSAL DOES ⛔ NOT REACH ALL OF THEM. The
// Ctrl+Z / Ctrl+Shift+Z mentions are SLATE'S OWN undo/redo, which no layout remap touches —
// ⛔ tokenising THOSE would print a key the player does not press. ⭐ The carve-out is those
// two Ctrl mentions ONLY: the trailing "Shift+Z is accepted" clause IS the accept key and
// DOES follow the pin, so a reversal must SPLIT that sentence, ⛔ not exempt it wholesale.
// ⇒ The engine citations and the full reasoning are in the ⛔ DO-NOT-TOKENISE block at
// :948-965, immediately above the sentence it guards.
```

## ⛔ NO WHOLESALE DUPLICATION — the refinement is stated ONCE and pointed at twice

Per the "two copies that can drift apart" instruction, the carve-out boundary is now authored in **exactly one place — `:934-936`, the reversal action-site**, because that is where someone flipping F-1 actually reads. The lower block's former duplicate (2 lines) was collapsed to a pointer:

```
// ⚠️ THE CARVE-OUT IS THE TWO Ctrl MENTIONS ONLY — the trailing "Shift+Z is accepted"
// clause IS the accept key and DOES follow the pin. That boundary is stated ONCE, at
// :934-936; ⛔ do not restate it here, because two copies drift apart.
```
⇒ The gist stays readable at the sentence (so an editor of the sentence is not left with a bare prohibition), but the authoritative statement has **one** home. ⛔ The engine citations likewise remain in exactly one place (the lower block) and are pointed at from the upper.

## ⚠️ LINE-CITE RE-VERIFICATION — the coordinator's catch was REAL, and it caught two

The insert at `:927` pushed the lower block down by 9 lines, and the pointer rewrite added 1 more. **Two cites went stale and both are fixed.** All re-verified against the file *after* the final edit:

| Cite | In block | Resolves to | Status |
|---|---|---|---|
| `:927-930` | lower | the reversal claim (4 lines) | ✅ was `:927-929`, **CORRECTED** |
| `:948-965` | upper | the DO-NOT-TOKENISE block, first→last line | ✅ was `:948-964`, **CORRECTED** (the pointer rewrite grew it) |
| `:934-936` | lower | the carve-out boundary sentence | ✅ was `:931-936`, **TIGHTENED** to the exact sentence |
| `:966` | (implied by "immediately above the sentence it guards") | `TEXT("Modified presses are not the accept key — …")` | ✅ block ends 965, sentence starts 966 |
| `GenericCommands.cpp:19` | lower | `UI_COMMAND(Undo, …, FInputChord(EModifierKey::Control, EKeys::Z))` | ✅ re-verified |
| `SlateEditableTextLayout.cpp:154-157` | lower | `MapAction(FGenericCommands::Get().Undo, …)` | ✅ re-verified |
| `SlateEditableTextLayout.cpp:1170-1172` | lower | the `Ctrl+Shift+Z` redo branch | ✅ re-verified |
| `SiegeKeyboardLayoutSubsystem.h:256-260` | upper (**pre-existing**, not mine) | the `KBD-§8`/`KBD-§0` ruling-1 pin | ✅ re-verified, still resolves |

⛔ **Stale-cite sweep: `grep` for `:927-929`, `:948-964`, `:931-936` returns ZERO hits.**

## FINAL STATE — `+29 / -2`

`git diff --stat` = **1 file changed, 29 insertions(+), 2 deletions(-)**.

⭐ **Machine-checked comment-only proof** (not an assertion — the greps were run):
- Every **added** line matches `^+\s*//` ⇒ **all 29 additions are comments.**
- Every **removed** line matches `^-\s*//` ⇒ **both deletions are comments** (they are the two rewrapped/collapsed comment lines, nothing else).
- ⛔ **`git diff | grep 'TEXT('` returns ZERO** ⇒ **not one player-facing string line appears in the diff, in either direction.**

⛔ Fences all still held: no behaviour · `Escape` untouched · `Shift+Z` still accepted · no compile/editor/MCP/Git · `Tools/Packaging/` untouched. **Suite UNCHANGED at 156, no delta for TASK-731.**

⚠️ **Tree note for QA:** `SummonedUnit.{h,cpp}` went dirty *during* this follow-up — that is TASK-726/`HIGH-§` landing in parallel, ⛔ **not mine**. My diff is `SiegeControlsHelpWidget.cpp` and nothing else.

## 🔍 ADDED FOR QA (TASK-730 Q8)

9. ⭐ **The `:934-936` / `:948-965` / `:927-930` cross-references are line numbers in a file this batch just moved.** Re-run them — they are correct as of this edit, and they are exactly the kind of reference that rots on the next insert above them.
