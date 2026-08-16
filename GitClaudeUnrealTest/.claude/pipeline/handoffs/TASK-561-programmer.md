# TASK-561 — [WR-7] the console INPUT-INSERT seam — the map's only channel to the AI

**Agent:** gameplay-programmer · **Date:** 2026-08-15 · **Status → `ready-for-qa`**
**Law:** `WR-§5` · `WR-§6` · `SC-§20` · `SC-§33` · `SC-§15` · `AS-§6` RULING A-2 (Escape, CLOSED)
**Compile / editor / MCP / Git / PIE:** ⛔ NONE RUN. File-only, as dispatched. Batch compile = TASK-566, gate = TASK-565, commit = TASK-570.
**Token figures:** ⛔ NONE QUOTED ANYWHERE IN THIS DOCUMENT (`AS-§12g` / `WR-§6`). Char/byte assertions only.

---

## 1. FILES TOUCHED — exactly the two I own

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.h` | +127 lines, **0 deletions, 0 modifications** |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.cpp` | +128 lines, **0 deletions, 0 modifications** |

⛔ **NOT touched, confirmed by `git diff --stat`:** `SiegeAssistantComponent.{h,cpp}` · `SiegeAssistantSnapshot.{h,cpp}` · `SiegeAssistantGrammar.{h,cpp}` · `SiegeAssistantCommand.{h,cpp}` · `SiegeAssistantVocabulary.{h,cpp}` · `SiegePlayerController.{h,cpp}` · any `TEXT()` prompt payload · any Zone builder · any `.csv` · any `Content/` asset · any test file.

⚠️ `AncientGround.h`, `SiegeGameMode.cpp`, `SummonedUnit.h` are also dirty in the working tree. **They are NOT mine** — they belong to the parallel TASK-554/557/558/559/560 agents. Ownership verified disjoint: nothing else in the tree touches `SiegeAssistantConsoleWidget.*`.

---

## 2. ⛔ THE BEFORE/AFTER SIGNATURE BLOCK (required by the dispatch)

### BEFORE — `SiegeAssistantConsoleWidget.h` @ `HEAD`

There was **no** append/insert entry point of any kind. The public inbound surface ended at:

```cpp
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	void SetConsoleEnabled(bool bEnabled, const FString& DisabledReason);
```

### AFTER — the ONE new public method, added immediately below it

```cpp
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
	bool AppendToInput(const FString& TextToInsert);
```

**One parameter. NO default argument. NO overload. NO existing signature altered, in either file.**

### The one new private member

```cpp
	bool bWarnedNoInputBoxForAppend = false;
```

⚠️ **It is separate from the shipped `bWarnedNoInputBox` deliberately.** Sharing that latch would let a war-map insert *consume* it, so `FocusInputBox()`'s own shipped warning (since TASK-444) would silently stop printing. **Suppressing an existing diagnostic is a behaviour change on an existing path**, which this task is forbidden to make. One bool buys byte-identical shipped behaviour.

---

## 3. ⛔ `SC-§33` — THE TRAILING-DEFAULT LAW, DISCHARGED WITH PASTED COMMANDS AND RAW COUNTS

`SC-§33`'s own SCOPE clause: *"this binds a DEFAULTED parameter added to a function that ALREADY HAS CALL SITES… it does not bind a brand-new function."* **Both legs are proven mechanically below, not asserted.**

### (a) NO DEFAULT ARGUMENT WAS ADDED ANYWHERE — the file's default-argument set is byte-identical before and after

```
CMD (HEAD):
  git show HEAD:GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.h \
    | grep -nE "^\s+(TSubclassOf|int32|float|bool|const |FString|FText|APlayerController)[^;]*=\s*[^;]*[,)]"

RAW HITS: 2
  463:		TSubclassOf<USiegeAssistantConsoleWidget> ConsoleClass = nullptr,
  464:		int32 ZOrder = 5);

CMD (WORKING TREE): same expression against the edited file
RAW HITS: 2
  578:		TSubclassOf<USiegeAssistantConsoleWidget> ConsoleClass = nullptr,
  579:		int32 ZOrder = 5);
```

⇒ **The same two defaults, on the same one function (`CreateAndAddToViewport`), before and after.** Line numbers moved only because I inserted above them (`SC-§18c` — I located by symbol; the numbers are reported as evidence, not used as anchors).

**Enumeration of `CreateAndAddToViewport`'s call sites** (the only default-bearing function in either file), so the gate can re-run it:

```
CMD:  grep -rn "CreateAndAddToViewport" Source/
RAW HITS: 10
```

| hit | classification |
|---|---|
| `SiegeAssistantConsoleWidget.h:576` | the declaration — **unchanged** |
| `SiegeAssistantConsoleWidget.cpp:107` | the definition — **unchanged** |
| `SiegeAssistantConsoleWidget.cpp:115`, `:141` | log strings naming it — **unchanged** |
| `SiegePlayerController.cpp:4421` — `AssistantConsoleWidget = USiegeAssistantConsoleWidget::CreateAndAddToViewport(this);` | **THE ONE REAL CALL SITE.** (ii) **DELIBERATELY LEFT AT BOTH DEFAULTS** — that is the shipped v1 state (`AS-§6` ruling A(c): `WBP_AssistantConsole` is reserved and not authored, so `ConsoleClass = nullptr` is correct; `ZOrder = 5` is the shipped value). ⛔ **Not mine to edit anyway** — `SiegePlayerController.cpp` is TASK-563's file. |
| `SiegePlayerController.cpp:33`, `:4257`, `:4308`, `:4419`; `SiegePlayerController.h:1398` | comments/includes naming it — (iii) **outside this task's ownership**, owner **TASK-563**. No action needed: nothing about the signature moved. |

### (b) THE NEW FUNCTION HAS ZERO EXISTING CALL SITES — the `SC-§33` exemption, proven

```
CMD:  grep -rn "AppendToInput" Source/
RAW HITS: 5
  SiegeAssistantConsoleWidget.cpp:1151   the DEFINITION
  SiegeAssistantConsoleWidget.h:542      the DECLARATION
  SiegeAssistantConsoleWidget.h:156      doc comment (class comment §2)
  SiegeAssistantConsoleWidget.h:216      doc comment (class comment §4, the seam list)
  SiegeAssistantConsoleWidget.h:860      doc comment (the log-latch member)
```

⇒ **Zero callers, by design** — exactly the `FSiegeAssistantRegionStatics::IsPointInRegion` precedent `SC-§33` names as owing nothing here. **The two callers that WILL exist are named to their owners:** `UWarMapWidget` (**TASK-560**, the marker click) and `ASiegePlayerController` (**TASK-563**, which owns the war-map lifecycle). ⚠️ **TASK-563 must open the console through the controller's existing posture-owning path BEFORE calling this** — see §5(iii).

---

## 4. ⛔⛔ THE DECLARED DEPARTURE (`SC-§15`) — "AT THE CARET" REFUSED, AND WHY IT IS THE SAME BEHAVIOUR

**Spec line (1) said:** *"appends a string to the console's input box **at the caret** and focuses it"*, and explicitly labelled the shape a **HYPOTHESIS under `SC-§20`** to be traced before typing. **I traced it. The "at the caret" half is not implementable through any supported API, and it would not change what the player sees if it were.** Three independent mechanisms, all read at the installed UE 5.8 source on this machine:

**(a) ⛔ THERE IS NO CARET TO READ.**
`UEditableTextBox` exposes no caret getter and no caret setter — the full public surface is `GetText/SetText/SetHintText/…` and its `TSharedPtr<SEditableTextBox> MyEditableTextBlock` is **`protected`** (`EditableTextBox.h:300`, under the `protected:` at `:299`). The Slate widget beneath it, **`SEditableTextBox`, has `GoTo()` but NO `GetCursorLocation()`** — that getter exists **only on the MULTI-LINE box** (`SMultiLineEditableTextBox.h:482`). The console's `InputBox` is single-line. **Nothing public can read its caret**, so no implementation could honour the line as written.

**(b) THE ENGINE MOVES THE CARET TO END-OF-DOCUMENT ON A PROGRAMMATIC `SetText` WHILE FOCUSED.**
`FSlateEditableTextLayout::SetText` raises `bForceBoundTextReview` and calls `OnBoundTextChanged()`, which runs:

```cpp
// Make sure we move the cursor to the end of the new text if we had keyboard focus,
// but only if we set it via SetText or LoadText
if (bForceBoundTextReview && Widget->HasAnyUserFocus().IsSet() && !bWasFocusedByLastMouseDown)
{
    JumpTo(ETextLocation::EndOfDocument, ECursorAction::MoveCursor);
}
```
(`SlateEditableTextLayout.cpp`, symbol `OnBoundTextChanged`, `:4280-4284`.)

**(c) AND IT MOVES IT TO END-OF-DOCUMENT AGAIN ON THE FOCUS THIS FUNCTION TAKES.**
```cpp
// Jump to the end of the document?
if (InFocusEvent.GetCause() != EFocusCause::Mouse
    && InFocusEvent.GetCause() != EFocusCause::OtherWidgetLostFocus
    && OwnerWidget->ShouldJumpCursorToEndWhenFocused())
{
    GoTo(ETextLocation::EndOfDocument);
}
```
(same file, symbol `HandleFocusReceived`, `:848-851`.) `ShouldJumpCursorToEndWhenFocused()` returns `bIsCaretMovedWhenGainFocus` (`SEditableText.cpp`, symbol `ShouldJumpCursorToEndWhenFocused`), which **defaults TRUE** (`EditableTextBox.cpp:31`) and is **deliberately never changed by the shipped `ApplyInputBoxContract`** (it sets ReadOnly / RevertTextOnEscape / ClearKeyboardFocusOnCommit / SelectAllTextWhenFocused, and nothing else). And `UWidget::SetKeyboardFocus()` — what `FocusInputBox()` calls — focuses with **`EFocusCause::SetDirectly`** (`Widget.cpp`, symbol `UWidget::SetKeyboardFocus`, via `FSlateApplication::SetKeyboardFocus`'s default argument in `SlateApplication.h:724`), which is neither `Mouse` nor `OtherWidgetLostFocus`.

> ### ⇒ **BOTH ORDERINGS CONVERGE ON END-OF-DOCUMENT.** An already-focused box takes (b); an unfocused box takes (c). A hypothetical "insert at the caret" implementation would place the text mid-string **and still leave the caret at the end** — strictly worse for the player, bought with new retained state and an `OnCursorMovedWithSelectionEvent` subscription.

✅ **THEREFORE: `AppendToInput` appends at the end, and the name says exactly what it does.** The suggested name from the spec (`AppendToInput`) and the traced behaviour agree — which is the tell that the hypothesis was right about the shape and wrong only about the word "caret".

⚠️ **What I did NOT run:** I did not execute this. The three citations are engine-source reads, not a PIE observation. The behaviour is asserted from the source and is checkable by anyone who opens those four files.

---

## 5. THE DESIGN DECISIONS, EACH MADE ON A MECHANISM

### (i) ⚠️ THE WHITESPACE RULE — stated, because the spec called it a real decision

Given `Existing` (box contents) and `Symbol` (the trimmed insert):

1. **ONE space before the symbol iff `Existing` is non-empty AND does not already end in whitespace.** (Last-character test via `FChar::IsWhitespace`, so a tab counts too.)
2. **Exactly ONE trailing space always follows the symbol.**
3. ⛔ **Nothing else in the player's text is touched** — no global whitespace normalisation, no re-casing, no head trim, no reordering (`SC-§31`: a presentation layer may not repair its input).

**The cases TASK-564 should assert (they are the ones I designed to):**

| existing box | + symbol | result |
|---|---|---|
| `""` (empty) | `mid` | `"mid "` |
| `"send 10 footmen to "` | `ancient_ground_near` | `"send 10 footmen to ancient_ground_near "` — ⭐ the spec's own hazard case; **no `nearand` collision** |
| `"send 10 footmen to"` (no trailing space) | `ancient_ground_near` | `"send 10 footmen to ancient_ground_near "` |
| `"mid "` (i.e. after one click) | `hero` | `"mid hero "` — **never `"mid  hero"`**; rule 2 is what makes rule 1 idempotent |
| anything | `""` / `"   "` | **refused**, `false`, box unchanged, **no stray separator appended** |

### (ii) ⛔ ZERO BEHAVIOUR CHANGE ON EVERY EXISTING PATH — verified at the artifact, not claimed

- **The diff is `+255 / −0`.** Not one existing line in either file was modified or deleted. (`git diff -U0 | grep "^-"` on the two files returns nothing.)
- **`InputBox->SetText()` fires no existing handler.** `WireChildWidgets` binds **only** `InputBox->OnTextCommitted`; `SetText` commits nothing, and `OnTextChanged` has **no subscriber in this class**. Read at `WireChildWidgets`, not assumed.
- **Open/close routes, the confirm step, `CancelPressed()`, `SubmitPressed()`, `HandleTextCommitted()`, `NativeOnPreviewKeyDown()`, `ApplyInputBoxContract()`, `FocusInputBox()`, `RefreshStatusLine()` — all byte-unchanged.** None is called from the new code except `FocusInputBox()`, which is called exactly as `SubmitPressed` and `OpenConsole` already call it.
- ⛔ **`Escape` — `AS-§6` RULING A-2 IS UNTOUCHED.** `git diff | grep -c "^+.*FReply\|^+.*Handled\|^+.*EKeys::"` returns **0**. No preview handler was edited, no Enhanced Input action was added, no `FReply::Handled` was introduced, and the token `Escape` appears **zero** times in anything I added. The four shipped close routes are unchanged and un-extended — **`AppendToInput` is not a fifth close route and is not a route of any kind.**
- ✅ **THE CONSOLE OPEN PATH IS BYTE-UNCHANGED, AND I VERIFIED IT RATHER THAN ASSERTING IT** (dispatch item 3): `OpenConsole()` in this file is untouched, and `ASiegePlayerController::OnAssistantConsolePressed` / `SetAssistantConsoleOpen` live in `SiegePlayerController.cpp`, which **does not appear in `git diff --stat`**.

### (iii) ⛔ IT NEVER OPENS THE CONSOLE — a mechanism, not a taste. **THIS IS A CONTRACT FOR TASK-563.**

A closed console **refuses, loudly, and returns `false`.** Two reasons, both checkable:

1. **The input posture belongs to `ASiegePlayerController`.** This widget never calls `SetInputMode` (its own class comment §2, the level-travel law). A widget-initiated `OpenConsole()` would put a visible console on the wrong posture and could strand the cursor.
2. **A write into a closed console would be destroyed anyway** — `OpenConsole()` calls `InputBox->SetText(FText::GetEmpty())` on **every** open. That would be a silent loss of the player's click, which is the one outcome this seam must not have.

⇒ ⚠️ **TASK-563 MUST open the console through the shipped controller path first, then call `AppendToInput`, and should check the `bool`.** Flagged here because a caller that ignores it gets a refusal it never sees.

### (iv) ⛔ NO GATE, NO VOCABULARY, NO LENGTH CAP

- ⛔ **No proximity check, NPC reference or range condition entered this file** (`WR-§5` RULING 5, *"the console still works anywhere"*). I additionally wrote the **converse of §2 into the class comment** — *nothing is a requirement for the console either* — placed exactly where a future author would go to add the thing it forbids.
- The **only** state guard beyond the null check is `!bConsoleEnabled`, and it is **the shipped fault latch, not a new gate**: in that state `ApplyConsoleVisualState` has already called `InputBox->SetIsEnabled(false)` and `SubmitPressed` already refuses, so writing there would strand text the player can neither send nor clear. It mirrors `SubmitPressed`'s first guard exactly.
- ⛔ **The function knows nothing about `PlaceVocabulary` and must not learn.** It moves an **opaque string**. Which symbols exist is `USiegeAssistantVocabulary`'s; which are clickable is `UWarMapWidget`'s (TASK-560). A validation branch here would be a second, drifting copy of a vocabulary this file does not own.
- ⛔ **No length cap here.** `USiegeAssistantSnapshot::MaxUtteranceBytes` is the **shipped authority** on how much player text reaches a prompt (and is already `static_assert`ed against the Zone C budget at `SiegeAssistantComponent.cpp:72-74`). A second copy inside a widget is `SC-§19`'s duplicated-authority defect — a guardrail that reports safe while the real one moves away from it.

---

## 6. ⭐ THE AIRLOCK PROPERTY — what this seam does and does not spend

**This task edited two UI files and nothing else.** It therefore:

- ✅ **spends ZERO prompt characters** — no Zone builder, no `TEXT()` prompt payload, no few-shot, no rule line was opened;
- ✅ **leaves Zone A byte-frozen at its named/dated 2026-08-05 baseline of 5658 chars / 5658 UTF-8 bytes** — asserted by the **pre-existing, untouched** `Tests/SiegeAssistantZoneATest.cpp` (`ShippedZoneAChars = 5658`). ⛔ **I did not touch that file and added no second assertion** (TASK-564 owns the verification);
- ✅ **adds no `who` shape, no grammar rule, no schema key, no place symbol, no coordinate field** — nothing here can reach any of them;
- ✅ **carries NO coordinate, dot, count or marker geometry.** The only thing that crosses this line is a **literal place symbol the model already transcribes for `where` out of the same `[FORCES]` vocabulary** — and the **player sends it himself.**

⛔ **No token figure is quoted anywhere above.** Chars/bytes only, per `AS-§12g` and `WR-§6`.

---

## 7. 📌 M8 DECLARATION (`WR-§8` — ⛔ NOT the last three batches' boilerplate)

**This task adds NO replicated property, NO new replicated class, NO new relevancy tier, and NO RPC.** `USiegeAssistantConsoleWidget` remains **client-local by construction**: a local input surface whose only outputs are delegate broadcasts to a local component (the class comment already declares this; nothing about it moved). `AppendToInput` is client-local, has no authority concern, and mutates no gameplay state — it writes one `FText` into one local `UEditableTextBox`. ⚠️ **The batch's two RPCs (`ServerRequestEnemyReveal` / `ClientReceiveEnemyReveal`) are TASK-563's, on the controller, and are not reachable from this file.**

---

## 8. ⚠️ WHAT QA SHOULD SCRUTINISE

1. ⭐ **The `SC-§15` departure in §4.** Re-open `SlateEditableTextLayout.cpp` (`OnBoundTextChanged`, `HandleFocusReceived`), `EditableTextBox.h:299-300`, and `SMultiLineEditableTextBox.h:482`. **If any one of the three legs is wrong, the departure is wrong and this should come back to me.** The claim is: no readable caret, and end-of-document by two independent routes.
2. **The `SC-§33` discharge in §3.** Re-run both greps. Expected: **2 default-argument hits before and after, identical**; **5 `AppendToInput` hits, all in my two files, zero callers.**
3. ⛔ **`Escape`.** Re-run `git diff | grep "^+.*Escape\|^+.*FReply\|^+.*Handled\|^+.*EKeys::"` on my two files. Expected **zero**. `AS-§6` A-2 is closed and this is the file most able to break it.
4. **The whitespace rule's idempotence** — that rule 2's always-a-trailing-space is what makes rule 1 never double up. The table in §5(i) is the spec TASK-564 should encode.
5. **The refusal-when-closed contract** in §5(iii) — this is the one place a caller can get it wrong, and the caller is TASK-563.
6. **The separate log latch** (`bWarnedNoInputBoxForAppend`) — confirm you agree that reusing the shipped `bWarnedNoInputBox` would have been a behaviour change on an existing path. If you think that is over-cautious, say so; it is one bool either way.
7. ⚠️ **What I could NOT verify (`SC-§32`, honestly):** nothing here was compiled, run, or seen in PIE. The whitespace rule is pure string arithmetic and is fully testable without a world (TASK-564 can call it against a widget with a code-authored `InputBox`, or extract the composition into a testable helper if it prefers — I left the composition inline and obvious rather than pre-emptively splitting it, and **that is a call TASK-564 is free to reverse; say so if you want the split**).

---

## 9. ⚠️ NOT MINE, FLAGGED NOT FIXED

- **`OnConsoleCancelled` still has zero live broadcasters** (TASK-527's open product question). Untouched, as dispatched.
- **`ToggleConsole()` and `CancelPressed()` remain deliberately uncalled** shipped API. Untouched.
- **`/Game/UI/WBP_AssistantConsole` remains reserved and unauthored.** `AppendToInput` works identically on a code-authored or an asset-authored tree — it only ever touches the `BindWidgetOptional` `InputBox`, null-guarded.
