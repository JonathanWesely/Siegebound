# TASK-1434 — [MENU-NAV-ASSISTANT-CONSOLE] — programmer handoff

Marker: `TASK-1434-MENU-NAV-ASSISTANT-CONSOLE`
Gate: `TASK-1435` ("THE REGRESSION IS THE POINT"). Status flipped to `ready-for-qa`.
Files written: `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.cpp` (**+42 / −1**) · `SiegeAssistantConsoleWidget.h` (**+137 / −0**).
⛔ No compile. ⛔ No PIE. ⛔ No MCP mutation. ⛔ No editor lifecycle action. ⛔ No git. ⛔ No `.uasset`. ⛔ No `CONVENTIONS.md`. Editor left as found (PID 18236, `TASK-1421`'s PIE untouched).

---

## §1 — THE NO-REGRESSION ARGUMENT, FIRST (spec (1))

This screen already does the two things the milestone is about, and it is the only screen in the project that does either. **Neither is touched by this diff, and both claims are measured rather than asserted.**

### 1.1 Focus on open — reached on every open path, all four call sites at **unchanged line numbers**

`FocusInputBox()` → `InputBox->SetKeyboardFocus()`. The four in-function call sites `qa/TASK-1435`'s criterion (1) names are at **`:513`, `:620`, `:697`, `:714` — the same numbers as before this diff**, because the cpp insertion lands at `:742`, below all four. The fifth (`AppendToInput`'s) moved `:1296` → **`:1337`**; its body is unchanged.

| site | function | now at | changed? |
|---|---|---|---|
| already-open re-assert | `OpenConsole()` | `:513` | ⛔ no |
| the open itself | `OpenConsole()` | `:620` | ⛔ no |
| empty-submit refusal | `SubmitPressed()` | `:697` | ⛔ no |
| post-submit re-assert | `SubmitPressed()` | `:714` | ⛔ no |
| war-map insert | `AppendToInput()` | `:1337` (was `:1296`) | ⛔ body unchanged, line moved |

`FocusInputBox()` itself is at `:1425-1442`, `InputBox->SetKeyboardFocus()` at **`:1441`**. Untouched.

⚠️ And the negative that must NOT be inherited here, as the row instructed: `TASK-1399`'s census sampled `InputBox` **≈160 s after open**, so it makes **no claim** about focus-on-open. Nothing in this row treats it as one.

### 1.2 Focus re-homed on close — both sites unchanged

`ReleaseKeyboardFocusToGame()` is called at `:481` (`NativeDestruct`, gated on `bConsoleOpen && !World->bIsTearingDown`) and `:645` (`CloseConsole`) — **both at unchanged line numbers**, both above the insertion point. The function body (`:1444-1458`) is untouched; `FSlateApplication::Get().SetAllUserFocusToGameViewport()` is at **`:1457`**.

### 1.3 ⛔ THE RULING: THIS SCREEN DOES **NOT** REGISTER A NAV TARGET — and that is (1)'s answer, measured

Spec (3) says *"REGISTER / UNREGISTER **only if (1) says it is safe**"*, and (1) says *"if registration would fight `SetKeyboardFocus`, say so and propose the smaller change."* **(1) says it is not safe, on four measurements.** The full argument is written into the class comment as **§5b** so the next author meets a ruling rather than an omission; here is the short form with its evidence.

**(a) The ring would have exactly ONE stop, and it is the text box.** `IsNavFocusStop` (`SiegeMenuInputSubsystem.cpp:773-872`) admits `UButton` / `UCheckBox` / `USlider` / `UEditableTextBox`, and a `UEditableTextBox` **unconditionally** (`:864-870`: *"a text box is keyboard-focusable by construction"*). `ConstructConsoleTree()` builds seven widgets and exactly one is an admitted class — see §2's stop table. Since `TASK-519` (Jonathan's ruling 3) it builds **no button at all**.

**(b) No nav verb has anything to do on a one-stop text-box ring.** `MoveFocus(±1)` reads the index from Slate, `WrapIndex(0, ±1, 1) = 0`, and it re-focuses the box it is already on (`:1265-1285`). `HandleMenuAccept` gives a `UEditableTextBox` **no Accept semantics on purpose** — `:1297-1302`, verbatim: *"⛔ A `USlider` and a `UEditableTextBox` deliberately get NO Accept semantics"* — and logs `IA_MenuAccept declined: focused stop '…' has no Accept semantics` (`:1335`). `StepFocusedStop` (Left/Right) declines for the same reason.

**(c) ⛔ The one verb that WOULD work is the one that must not.** `IA_MenuBack` is mapped to **`Backspace`** — the most-pressed key in a text box. It is inert today **only** because this class does not implement `ISiegeMenuNavCloseTarget` (`HandleMenuBack:1779-1785` logs *"implements no ISiegeMenuNavCloseTarget — Back is INERT for it"*), and **`TASK-1454` is boarded to wire that interface onto screens**. Registering here would put a **fifth, unenumerated close route one row away, keyed to Backspace**, against an `AS-§6 A-2` list of exactly **four** routes that the header says *"IS the contract"*. ⇒ the only reachable nav behaviour registration would buy on this screen is a **defect**.

**(d) ⛔ And `Up`/`Down` reach Enhanced Input EVEN WHILE THE BOX HAS FOCUS — measured at the installed UE 5.8 source, with the engine's own comment.**
`FSlateEditableTextLayout::HandleKeyDown` wraps the vertical arrows in `BoolToReply(MoveCursor(...))` (`Slate/Private/Widgets/Text/SlateEditableTextLayout.cpp:1040-1059`), and `MoveCursor`'s single-line branch is:

```
else
{
    // Vertical movement not supported on single-line editable text controls - return false so we fallback to generic widget navigation
    return false;
}
```
(`:2262-2265`)

⇒ `Up`/`Down` are **Unhandled by the box**, bubble past it to `SViewport` and reach Enhanced Input. **Left / Right / Backspace do not**: `MoveCursor`'s horizontal path falls through to `return true` and `HandleBackspace` returns `true` unless the box is read-only (`:1479-1534`). And with focus **outside** the box — a state this file already documents at `NativeOnPreviewKeyDown` (*"if a player clicks the world and focus leaves the box"*) — **all eleven** of the in-match-live `IMC_MainMenu` keys reach it, so `Up` would **yank keyboard focus back into the chat box** on a key that does nothing in this game today.

**⇒ Registration buys zero reachable navigation and costs a menu mapping context armed over live combat plus a latent `Backspace` close.** Declined. ✅ What would change the answer is stated in §5b: a `WBP_AssistantConsole` supplying a `ConfirmButton` (ruling A(b)) gives the ring a second stop that *does* have Accept semantics, and registering is then worth re-arguing.

### 1.4 ⛔ `WARN-2` (the register-on-open contract) is therefore SATISFIED VACUOUSLY, not violated

The dispatch's hard contract — *register on OPEN, unregister on CLOSE, never at construction* — exists because both in-match screens are built `CreateAndAddToViewport`-**closed** and the arming backstop is disarm-only with **no rising edge on show**. ⛔ **This row registers nowhere: not in `NativeConstruct`, not in `OpenConsole`, not at all.** There is no registration that could be stranded. The string `RegisterMenuNavTarget` does not appear in either file (grep: 0 hits in both).

---

## §2 — THE EXPECTED STOP-NAME LIST, DECLARED BEFORE 5b SO `TASK-1436` CAN FALSIFY IT

🚨 **ANCHORING: `UNANCHORED` by object path, and the reason is structural, not an omission.** This widget is **code-authored with no `.uasset`** (`CONVENTIONS AS-§6` RULING A; `/Game/UI/WBP_AssistantConsole` is *reserved and not authored*), and the instance is created at runtime by `ASiegePlayerController` at **`SiegePlayerController.cpp:6722`** (`USiegeAssistantConsoleWidget::CreateAndAddToViewport(this)` → `CreateWidget<>(OwningController, ResolvedClass)` → `AddToViewport(5)`, **closed**). There is **no design-time object path to give**. The substitute anchor is the construction route: every child below is constructed in **`SiegeAssistantConsoleWidget.cpp::ConstructConsoleTree` (`:174-377`)** by the literal `TEXT("…")` name shown, which is a stronger identifier than a runtime `GetName()` here.

**Tree order = `UWidgetTree::ForEachWidget`'s depth-first pre-order (root, then each `UPanelWidget`'s children in slot order):**

| # | name | class | construction route | admitted stop? |
|---|---|---|---|---|
| 1 | `RootPanel` | `UVerticalBox` | `ConstructWidget<UVerticalBox>(…, TEXT("RootPanel"))`, `.cpp:204` | ⛔ no |
| 2 | `ConsoleTopSpacer` | `USpacer` | `.cpp:229` | ⛔ no |
| 3 | `ConsoleBackdrop` | `UBorder` | `.cpp:240` | ⛔ no |
| 4 | `ConsoleColumn` | `UVerticalBox` | `.cpp:241`, set as the Border's content `.cpp:255` | ⛔ no |
| 5 | `TranscriptText` | `UTextBlock` | `.cpp:281` | ⛔ no |
| 6 | `StatusText` | `UTextBlock` | `.cpp:307` | ⛔ no |
| 7 | **`InputBox`** | **`UEditableTextBox`** | `.cpp:327` | ✅ **YES — the only one** |
| — | `ConfirmButton` | `UButton` | ⛔ **NOT CONSTRUCTED** (`TASK-519`); `BindWidgetOptional` only | n/a in v1 |

**⇒ EXPECTED STOP LIST: exactly one — `InputBox` (`UEditableTextBox`).**

**THE RULE THAT GENERATES THE COUNT, not a bare integer:** *one stop per admitted-class child of the tree, and `ConstructConsoleTree` constructs exactly one admitted-class child.* The tree's **structure** is static — the transcript is a single `UTextBlock` whose **text** changes, never a list of widgets — so the count is invariant across every runtime state of the v1 console. If a `WBP_AssistantConsole` is ever authored, the count becomes "however many admitted-class children that asset's tree holds", and `ConfirmButton` is the expected second.

🚨 **AND THE PROPERTY `TASK-1436` SHOULD ACTUALLY TEST IS THAT THIS WALK NEVER HAPPENS.** Because the console never registers, `GetActiveNavTarget()` never returns it and `GetMenuFocusStops()` is never called with it as the target. The table above is *what a walk would find if one ever ran*, supplied so the row can be falsified rather than believed.

---

## §3 — THE SECOND ENTRY POINT, SHOWN CALLING THE **SAME** CONFIRM (spec (2))

### 3.1 The diff, in full

**New: `USiegeAssistantConsoleWidget::MenuAcceptPressed()`** — `UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")`, declared `.h:493` (immediately after `ConfirmPressed()`), defined **`.cpp:742-769`**. Its entire body:

```cpp
	UE_LOG(LogSiegeAssistant, Log,
		TEXT("[AssistantConsole] Accept arrived through the MENU-NAV entry point (IA_MenuAccept -> ConfirmButton::OnClicked), not the positional `Z` key. The confirm below is the SAME one — this function adds no gate and no broadcast of its own."));

	ConfirmPressed();
```

**Changed: `HandleConfirmClicked()`** (`.cpp:861-873`) — `ConfirmPressed();` → `MenuAcceptPressed();`.

### 3.2 ⛔ ONE confirm implementation, TWO entry points — `qa/TASK-1435` (3)

```
  entry point 1 (live, byte-unchanged)   NativeOnPreviewKeyDown  -> ConfirmPressed()   [.cpp:943]
  entry point 2 (new)                    MenuAcceptPressed()     -> ConfirmPressed()   [.cpp:768]
                                                    ^
  IA_MenuAccept -> HandleMenuAccept -> focused UButton::OnClicked.Broadcast()
                -> HandleConfirmClicked() -> MenuAcceptPressed()                       [.cpp:873]
```

`ConfirmPressed()` (`.cpp:721-740`) is **byte-unchanged**. It remains the single owner of the gate (`if (!bConfirmPromptVisible)`), the single `Accept ignored` refusal, and the single `OnConsoleConfirmed.Broadcast()`. ⛔ **`MenuAcceptPressed()` deliberately carries no gate of its own** — a second gate would be `CONVENTIONS §19`'s duplicated-authority defect on the exact question the confirm step exists to answer. **Nothing was copied.**

### 3.3 Why the route is `ConfirmButton::OnClicked` and not a new input binding

`USiegeMenuInputSubsystem::HandleMenuAccept` delivers Accept by firing **the focused `UButton`'s `OnClicked.Broadcast()`** (`SiegeMenuInputSubsystem.cpp:1345`) — *"the SAME delegate a mouse click fires"*. `ConfirmButton` is already pinned, already `BindWidgetOptional`, and already bound to `HandleConfirmClicked` in `WireChildWidgets` (`.cpp:396`). ⇒ routing that thunk through the named door makes `IA_MenuAccept` reach this console's confirm **with zero new input binding, zero new key, zero subsystem change and zero new asset**. ⛔ This widget still binds nothing to Enhanced Input (class comment §5), and `KBD-§1`/`§2` are untouched.

### 3.4 ⛔ REACHABILITY DECLARED AT AUTHORING TIME, NOT SPRUNG AT 5b

**Entry point 2 has no live caller in this binary, and that is stated here rather than discovered at the sitting.** The v1 code-authored tree constructs **no** `ConfirmButton` (`TASK-519`), so `ConfirmButton` is null, so `WireChildWidgets` never makes the binding, so `HandleConfirmClicked` is **never called**. ⇒ the `HandleConfirmClicked` edit is **observably inert in the shipped build by construction**, not by promise — which is also why it cannot regress anything. It goes live the moment a `WBP_AssistantConsole` supplies a child named `ConfirmButton` (ruling A(b)'s escape hatch), with **zero further C++ change**.

⛔ The alternative — reaching `IA_MenuAccept` some other way — is **structurally impossible today**, by three independent mechanisms, and I state all three so nobody re-tries them:
1. `IMC_MainMenu` is applied in a match **only** on the subsystem's registration rising edge (`TASK-1429`), and this screen does not register.
2. Even applied, `HandleMenuAccept` **declines on a focused `UEditableTextBox`** by design (`TASK-1409` (4b)) — the console's only stop.
3. Even reaching it, `IA_MenuAccept`'s `Enter` is **shadowed in a match** by `IA_AssistantConsole` at the higher priority (`qa/TASK-1430.md`'s census), so the only key that could carry it is `Gamepad_FaceButton_Bottom`.
   ⇒ a `UButton` in this tree is the only door, and **`TASK-519` / Jonathan's ruling 3 forbids constructing one.**

---

## §4 — THE POSITIONAL-`Z` PATH, RE-QUOTED UNCHANGED (spec (2), `qa/TASK-1435` (4))

⭐ **PROVEN BY DIGEST, not by reading.** The whole accept-key region — `NativeOnPreviewKeyDown` + its `Escape` fall-through + `ResolveKeyboardLayoutSubsystem` + `GetAcceptKey` — old `:845-970` vs new `:886-1011`:

```
sha256(HEAD  :845-970 ) = fbb5d1a977797e64325bed58410a799de4d5afb62f991df852570a57d8165511
sha256(work  :886-1011) = fbb5d1a977797e64325bed58410a799de4d5afb62f991df852570a57d8165511
```

**Identical.** The two load-bearing lines, re-quoted verbatim from the working tree:

```cpp
	if (bAcceptIsLive && bIsUnmodified && InKeyEvent.GetKey() == GetAcceptKey())   // .cpp:924
		ConfirmPressed();                                                          // .cpp:943
```

```cpp
		return LayoutSubsystem->GetPositionalKey(EKeys::Z);                        // .cpp:1008
	}

	return EKeys::Z;                                                               // .cpp:1011
```

⛔ `GetAcceptKey()` is untouched. ⛔ The positional resolver (`USiegeKeyboardLayoutSubsystem::GetPositionalKey`) is untouched and not in this row's fence. ⛔ The token `Escape` still does not appear in `NativeOnPreviewKeyDown`'s implementation (`AS-§6 A-2` is closed and this row does not reopen it). ⛔ `RevertTextOnEscape` stays `false` (`ApplyInputBoxContract:427`, untouched). ⛔ **The engine's `SEditableTextBox`-absorbs-Escape behaviour is inherited and unchanged; his `Escape` grant was widened by zero keys.**

---

## §5 — ⚖️ 🧑 WHAT CHANGES IF JONATHAN FLIPS THE `Enter` PRIORITY (`SiegeMenuInputSubsystem.h:472`, `0` → `2`)

⛔ I did not touch that line, and this row ships correct under **either** outcome. Precisely, for **this widget**:

| | priority `0` (shipping) | priority `2` (the flip) |
|---|---|---|
| **Console is the only in-match screen open** | `Enter` opens/closes the console (`IA_AssistantConsole`) | ⛔ **IDENTICAL — NOTHING CHANGES.** This screen never registers ⇒ it never arms `IMC_MainMenu` ⇒ a priority it never applies cannot take a key from it. |
| **`Enter` in the focused box (submit, and CLOSE ROUTE 4)** | submits / closes on empty | ⛔ **IDENTICAL.** A focused editable box consumes `Enter` in **Slate** before the viewport is offered it (`SlateEditableTextLayout.cpp:1092`, the `qa/TASK-411` ruling). Priority orders Enhanced Input's contexts only. |
| **Some OTHER in-match screen is registered (context armed) and the console is CLOSED** | `Enter` still opens the console | 🚨 **THE ONE REAL CHANGE: `Enter` builds as `IA_MenuAccept` instead, and the console's OPEN KEY is dead until that screen closes.** |
| **The WORLD-B re-open suppression** (`OpenConsole:565-583`) | can fire | can only *stop* firing while armed (`Enter` no longer reaches `IA_AssistantConsole`). The guard is one-shot and time-bounded either way ⇒ no new failure. |

⇒ **The flip's only assistant-console consequence is cross-screen and only while another screen holds the ring.** Nothing in this diff has to change under either value, and this table is the deliverable the ruling asked for, not a footnote.

---

## §6 — DELETIONS (the house shape demands they be declared)

⛔ **EXACTLY ONE line deleted, and it is superseded in place:**

```
-	ConfirmPressed();     (inside HandleConfirmClicked, old .cpp:832)
+	MenuAcceptPressed();  (new .cpp:873, whose body calls ConfirmPressed())
```

No symbol, log string, comment, `UPROPERTY`, `UFUNCTION`, delegate or behaviour was removed. `git diff -U0 | grep '^-'` over both files returns that one line and nothing else. Header: **zero deletions**.

---

## §7 — THE COMPILE SWEEP (the trap that cost a build tonight)

- **C4458 — swept against the BASE CHAIN, not the repository.** Verified at the engine headers: `USiegeAssistantConsoleWidget` → `UUserWidget` **+ `INamedSlotInterface`** (`UMG/Public/Blueprint/UserWidget.h:280`) → `UWidget` **+ `INotifyFieldValueChanged`** (`UMG/Public/Components/Widget.h:216`) → `UVisual` (`UMG/Public/Components/Visual.h:12`) → `UObject` → `UObjectBaseUtility` → `UObjectBase` — **eight bases**, matching `TASK-1424`'s count (`UUserWidget` and `UWidget` each also inherit an interface).
  ⛔ **The exposure is ZERO BY CONSTRUCTION: both new/changed bodies declare no local variables at all.** `MenuAcceptPressed()` is one `UE_LOG` plus one call; `HandleConfirmClicked()` is one call. There is nothing that could shadow `Slot`, `Visibility`, `Cursor`, `Clipping` or anything else in the chain. (The file's existing `Slot` warning at `.cpp:293-302` is pre-existing doctrine and still honoured — no new locals were named at all.)
- **Name collision sweep:** `MenuAcceptPressed` / `MenuAccept` — grepped across `UserWidget.h`, `Widget.h`, `Visual.h`, `Object.h`, `UObjectBaseUtility.h`, `UObjectBase.h` ⇒ **0 hits**. Grepped across `Source/` ⇒ 0 hits before this diff.
- **🚨 REFLECTION SURFACE CHANGES — PREDICT A `.generated.h` DIFF, AND IT IS NOT A FINDING HERE.** `MenuAcceptPressed` is a new `UFUNCTION(BlueprintCallable)`, so **`SiegeAssistantConsoleWidget.generated.h` MUST change** (one new `DECLARE_FUNCTION`/thunk + exec entry). ⛔ This is the *opposite* of `TASK-1429`'s prediction and is deliberate: an **unchanged** `.generated.h` here would mean UHT did not see the new `UFUNCTION`, which **is** a finding. No new `UPROPERTY`, `UCLASS`, `USTRUCT`, `UENUM` or delegate; no new member variable; no GC surface change.
- **Includes:** ⛔ **zero added, zero removed.** The diff introduces no new type — deliberately, and it is why registration would have been the larger change (it would have pulled `SiegeMenuInputSubsystem.h` into this widget).
- **Encoding:** both files remain UTF-8 **no BOM, pure LF** (measured: 0 CRLF, 1458 LF in the `.cpp`, 1110 in the `.h`), matching their siblings. The only two non-ASCII codepoints new to these files (`U+00B1`, `U+1F9D1`) already appear in **35** and **45** other `Source/` files that compile in this module today, and neither has `0x5C` in its UTF-8 bytes.
- **Deprecated API:** none used; the diff calls only `UE_LOG` and two of this class's own methods.
- ⛔ **The automation suite is NOT cited in either direction** (it and the walker have diverged; `TASK-1475` is closing that).

---

## §8 — ACCEPTANCE AS A PROPERTY, NEVER AN INDEX — and the unhappy path with its discriminator

**P-A · THE NO-REGRESSION LIMB (the one that matters, and it is fully falsifiable today).**
*Property:* opening the console in a match puts keyboard focus in `InputBox`; closing it returns focus to the game viewport.
✅ pass signature: `[AssistantConsole] Opened.` then `[AssistantConsole] Closed.`, with the cursor in the box between them.
⛔ **DEFECT discriminator — the STRING, not a count:** `[AssistantConsole] No InputBox to focus — the console cannot take typing. (Logged once.)`
🧑 **cl. 3(b), verbatim:** *"Open the assistant console in a match — does the text box still get the cursor immediately, and when you close it does typing go back to the game?"*

**P-B · THE REFUSAL (this row's ruling), and it has TWO CORRECT SIGNATURES.**
*Property:* with the assistant console open and **nothing else registered**, the in-match menu vocabulary stays **disarmed** and no focus ring exists.
✅ signature (a) — fresh match, nothing ever registered: **no `[USiegeMenuInputSubsystem]` line at all.**
✅ signature (b) — some other screen opened and closed earlier in the same match: the entry line `IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1).` **plus** `MoveFocus(+1) declined: menu covered.`
⛔ **A gate demanding "no lines" FAILS A CORRECT BUILD** — `qa/TASK-1430.md`'s finding, inherited.
⛔ **FAIL discriminator:** any `IN-MATCH MENU VOCABULARY ARMED` line while the assistant console is the only thing open ⇒ the refusal was not honoured. Also FAIL: any `MoveFocus(…): focus moved …` naming `InputBox`.

**P-C · THE SECOND ENTRY POINT — `UNOBSERVABLE` in this binary, DECLARED NOW.**
*Property:* `MenuAcceptPressed()` has no live caller in v1 (no `ConfirmButton` is constructed), so its log line must **never** appear.
⛔ **FAIL discriminator:** `[AssistantConsole] Accept arrived through the MENU-NAV entry point` appearing at all in a v1 log ⇒ something constructed a `ConfirmButton`; a real finding.
⭐ **AND IT SHIPS WITH A CONTROLLED NEGATIVE, so the silence is not a blind instrument:** with a confirm prompt up, pressing `Z` **must** produce `[AssistantConsole] Accept key pressed (physical QWERTY-Z position; this layout yields '…')` followed by `[AssistantConsole] Accept pressed.` ⇒ if **only** the `Z` line appears, the row is correct **and** the lane could see. If **neither** appears, the lane is blind and P-C is `UNOBSERVABLE`, not `VERIFY-FAILED`.

---

## §9 — Notes for `TASK-1435` (QA), in its own four terms

1. **`SetKeyboardFocus` on every open path** — four sites at **unchanged** line numbers `:513 :620 :697 :714` (+ `:1337`, moved not changed); the call itself at `:1441`. Prefer cite-by-text; the numbers are given because they happen to be stable.
2. **`ReleaseKeyboardFocusToGame()` on close** — `:481` and `:645`, both unchanged; body `:1444-1458`.
3. **ONE confirm, two entry points** — §3.2's diagram. `ConfirmPressed()` (`:721-740`) byte-unchanged; **nothing copied**; `MenuAcceptPressed()` carries no gate of its own on purpose.
4. **Positional-`Z` byte-unchanged** — §4's sha256 pair, `fbb5d1a9…165511` on both sides.
5. The **registration refusal** is the row's largest claim; please attack §1.3 (a)–(d) rather than accept it. The single most falsifiable limb is (d)'s engine quote at `SlateEditableTextLayout.cpp:2262-2265`.

---

## Not examined / limitations

- ⛔ **Nothing here was compiled, run, or observed at runtime.** Every claim is text-level or engine-source-level. `TASK-1436` owns runtime; build-master owns `Result: Succeeded` and the `.generated.h` delta.
- ⛔ **I did not open the editor, drive PIE, or read any `.uasset`.** `TASK-1421`'s PIE was running for this row's whole duration; the `IMC_Hero` / `IA_AssistantConsole` / `IMC_MainMenu` facts are **cited from `qa/TASK-1430.md`'s live re-measurement, not re-taken by me**, and should be read as inherited.
- ⚠️ **`ForEachWidget` descending into a `UBorder`'s content is reasoned from `UBorder : UContentWidget : UPanelWidget` + `ForWidgetAndChildren`'s use of `GetChildAt`, not observed.** If it did *not* descend, §2's table shrinks to non-stops only and the stop count would be **0, not 1** — which would *strengthen* §1.3, never weaken it. Named because the direction of the error matters.
- ⚠️ **The dispatch's cited walker facts were re-measured for the classes I depend on (`UEditableTextBox` admitted unconditionally; `HandleMenuAccept` declining on it) but I did NOT re-measure `ForEachWidget`'s non-descent into nested `UUserWidget`s** — this tree contains none, so it cannot bear on the answer.
- ⛔ **I did not measure whether `Gamepad_FaceButton_Bottom` routes through Slate to `NativeOnPreviewKeyDown`.** It is not used by this diff; it is named in §3.4 only as the key that *would* carry `IA_MenuAccept` in a match, which is a statement about the mapping, not about Slate routing.
- ⛔ **I did not verify that `UWidget::IsVisible()` returns true for `ESlateVisibility::SelfHitTestInvisible`.** It would matter only if this screen registered, and it does not.
- ⛔ **The `git diff` deletion count in §6 was taken with `git diff -U0` over my two files only.** The authoritative whole-tree count is the orchestrator's; this working tree carries other agents' uncommitted rows.
- ⛔ **No `CONVENTIONS.md` edit was made.** §5b's ruling lives in the class comment. If the manager wants it in `AS-§`, that is a board edit and a different row.
- ⛔ **`TASK-1454`'s eventual close-target wiring is named as a hazard in §1.3(c) but I did not read that row's spec** — the hazard stands on the interface's existence, not on that row's contents.
