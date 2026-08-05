# QA Report — TASK-525 (batch ASSISTANT-EXCLUDE gate)

**Verdict: PASS** — 0 BLOCKER · 7 WARN · 11 NIT
**Covers: TASK-516 · 517 · 518 · 519 · 520 · 521 · 522 · 523 · 524.** The union IS the roster (`SC-§29`). No gap.
**Date:** 2026-08-04 · **Reviewer:** qa-reviewer · **Report is the authoritative verdict; Slack is the mirror.**

---

## ⛔ THE LIMIT OF THIS PASS, STATED PLAINLY

**Nothing in this batch has been compiled. No test has been run. Nothing has been seen on screen.**
A PASS here means **CORRECT AS SOURCE** and nothing more. Three things this gate structurally cannot close and does not claim to:

1. **The compile.** TASK-526 owns the only build. Baseline to beat: TASK-514, `Result: Succeeded`, 0/0, suite 40/40.
2. **The `Z` key actually firing.** No automation test can drive Slate focus headlessly (TASK-523 §6). I verified the *mechanism* against the installed UE 5.8 source below; the *behaviour* is TASK-527's pixel check.
3. **Model accuracy.** `AS-§20.4`'s named #1 risk — *a rule line is a weaker teaching signal than an exemplar for a shape the model has never seen* — is **undischarged** and goes to Jonathan.

I also have **no shell and no Git tool**, so criterion 10 is discharged from the diff and the handoffs only — see W-8.

---

## ⚖️ THE FIVE RULINGS THE DISPATCH ASKED FOR

### RULING 1 — ⭐ `SetIsFocusable(true)` was correctly NOT called. **TASK-519's reasoning is VERIFIED, not merely plausible.**

Read from the installed UE 5.8 tree on this machine. All four claims hold, and I checked the two the handoff did not quote.

**(a) The tunnel runs first and the bubble only if the tunnel did not handle** — `Engine/Source/Runtime/Slate/Private/Framework/Application/SlateApplication.cpp:5024-5046`:

```cpp
// Tunnel the keyboard event
Reply = FEventRouter::RouteAlongFocusPath(this, FEventRouter::FTunnelPolicy(EventPath), InKeyEvent, [] (const FArrangedWidget& CurrentWidget, const FKeyEvent& Event)
{
    if (CurrentWidget.Widget->IsEnabled())
    {
        const FReply TempReply = CurrentWidget.Widget->OnPreviewKeyDown(CurrentWidget.Geometry, Event);
        ...
        return TempReply;
    }
    ...
    return FReply::Unhandled();
}, ESlateDebuggingInputEvent::PreviewKeyDown);

// Send out key down events.
if ( !Reply.IsEventHandled() )
{
    Reply = FEventRouter::RouteAlongFocusPath(this, FEventRouter::FBubblePolicy(EventPath), ... OnKeyDown ... );
}
```

**(b) `FTunnelPolicy` walks ROOT → LEAF** — `SlateApplication.cpp:347-366`: constructed `WidgetIndex(0)`, `ShouldKeepGoing()` is `WidgetIndex < RoutingPath.Widgets.Num()`, `Next()` is `++WidgetIndex`, `GetWidget()` returns `RoutingPath.Widgets[WidgetIndex]`. (Contrast `FDirectPolicy` at `:326-330`, which takes `Widgets.Num()-1` — the leaf.) ⇒ an **ancestor of the focused `SEditableText` is offered the key before the leaf can turn it into the character `z`.**

**(c) `SObjectWidget` forwards it into the `UUserWidget`** — `Engine/Source/Runtime/UMG/Private/Slate/SObjectWidget.cpp:221-229`:

```cpp
FReply SObjectWidget::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    if ( CanRouteEvent() ) { return WidgetObject->NativeOnPreviewKeyDown( MyGeometry, InKeyEvent ); }
    return FReply::Unhandled();
}
```
with `CanRouteEvent()` = `WidgetObject && WidgetObject->CanSafelyRouteEvent()` (`SObjectWidget.h:112-115`) — **no focusability term.**

**(d) The `Super` fall-through really is `Unhandled` in v1** — `UMG/Private/UserWidget.cpp:2500-2503` is `return OnPreviewKeyDown( InGeometry, InKeyEvent ).NativeReply;`, and an unimplemented BIE yields a default `FEventReply` whose `NativeReply` is `FReply::Unhandled()`.

**(e) The focusability claim itself** — `UserWidget.h:1030` carries exactly the quoted deprecation: *"Direct access to bIsFocusable is deprecated… this property is only set at construction and is not modifiable at runtime."*

> ⇒ ⭐ **The routing walks the focus PATH — a `FWidgetPath` from the window root down to the focused leaf. Every ancestor is on it by construction. `SupportsKeyboardFocus()` is consulted NOWHERE in the tunnel.** `SetIsFocusable(true)` is **not required**, and adding it would have put a focusable console in competition with its own `InputBox` for zero gain. **The headline feature is not dead. TASK-519 was right to refuse the dispatch's instruction and right to say so.**

⚠️ **BUT I FOUND THE ONE TERM NOBODY QUOTED — see W-1.** The lambda at `:5027` is gated on `CurrentWidget.Widget->IsEnabled()`. Today that is satisfied (nothing calls `SetIsEnabled(false)` on the console `UUserWidget` itself — `ApplyConsoleVisualState` disables only `InputBox` and `ConfirmButton`, `SiegeAssistantConsoleWidget.cpp:1145-1169`), so the key is live. It is a real, undocumented precondition and a live trap for the next editor.

### RULING 2 — the `CancelButton` conflict: **TASK-519 IS RIGHT. The member stays DELETED. Do not revert.**

The dispatch paraphrase is the outlier against **two written authorities that agree**:
- CONVENTIONS `AS-§6` RULING A, amended (`CONVENTIONS.md:701`): *"⇒ **The `CancelButton` member, its `HandleCancelClicked` thunk and its `CancelLabelText` all go.** ⛔ A future `WBP_AssistantConsole` MUST NOT re-introduce a widget named `CancelButton` expecting it to bind — the C++ no longer declares it, so a WBP child of that name would bind to nothing and look wired."*
- TASKBOARD TASK-519 spec clause (1) (`TASKBOARD.md:6616`): *"Delete the `CancelButton` member, its `HandleCancelClicked` thunk, its `CancelLabelText`, its construction, its binding and its visibility handling."*

And the law's own reasoning settles it against the dispatch: a surviving `BindWidgetOptional` with **no thunk, no construction and no visibility handling** is *precisely* the "binds to nothing and looks wired" state the clause forbids. The retirement the law asks for is the **strikethrough in the pin**, and TASK-519 additionally put a `//~ RETIRED 2026-08-04` block at the code site (`SiegeAssistantConsoleWidget.h:595-604`) naming the WBP prohibition. That is the better outcome on both counts. **Files beat paraphrase; no revert; no re-amendment of `AS-§6` needed.**

✅ **`CancelPressed()` survived intact and is documented as deliberately uncalled** — `SiegeAssistantConsoleWidget.h:373-374` still reads `UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant") void CancelPressed();`, and `:352-371` says *"DELIBERATELY UNCALLED FROM 2026-08-04… exactly like ToggleConsole()"* and names the `OnConsoleCancelled` consequence in place. Body byte-unchanged.
✅ `ConfirmButton` stays declared `BindWidgetOptional` (`:592-593`), stays bound (`.cpp:394-397`) and stays shown/hidden (`.cpp:1164-1169`) — ruling A(b)'s escape hatch is intact **and now load-bearing**.

### RULING 3 — the third broken test, and **is the sweep complete? YES — I re-ran it, and it is.**

TASK-523's find is real and correctly repaired: `Siegebound.Assistant.ZoneA.NullVocabularyIsNotTheMeasuredLane` **derives** its expectation from the constant, and is re-based to the **shipped** figure at `Tests/SiegeAssistantZoneATest.cpp:849-851` (`ShippedZoneAChars`, not the spike's). Using `SpikeLaneZoneAChars` there would have compiled, run, and been wrong by exactly 308.

**I ran the right question myself — *"what reads the constant, or `MaxRosterKinds`?"* — over `**/*.{h,cpp,cs}` in `Source/` and `Plugins/`. There is no fourth.**
- `MeasuredZoneAChars` (the old name) returns **zero hits repo-wide** — the rename is complete.
- Every surviving `5116` literal outside `Tests/SiegeAssistantZoneATest.cpp` is a **historical attribution to the SPIKE lane**, which is untouched and still 5116: `SiegeAssistantComponent.h:965` / `:1398`, `.cpp:3072` / `:3159` / `:3174`, `SiegeAssistantSnapshot.cpp:780/785` (relabelled by TASK-521 with 5424 beside them at `:787/:792`), `SiegeCheatManager.cpp:586`. **None is an assertion. None can fail. All remain true of the lane they name.** One invites a misread — N-5.
- Every `MaxRosterKinds` consumer is either the constant's own file, a log format arg (`SiegeAssistantComponent.cpp:3247/3255`, `SiegeCheatManager.cpp:670`, `SiegeCheatManager.h:146`), or the new tests. `Plugins/SiegeLlama/…:403` is a comment in the untouched spike lane.
⇒ ✅ **The `SC-§14` hole is closed with a positive enumeration, not with a negative search.**

### RULING 4 — the two accepted instrument weaknesses. **BOTH SHIP — with their limits written into the batch's report, not buried.**

**(a) `Clarify` cannot fail a silent drop. VERIFIED AT THE ARTIFACT, and TASK-524's F-2 is exactly right** — `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp:2266-2281`:
```cpp
if (bWantsExecute)      { Result.bOutcomeOk = !Parsed.bIsQuestion; }
else if (bWantsRefuse)  { Result.bOutcomeOk =  Parsed.bIsQuestion; }
else if (bWantsClarify) { Result.bOutcomeOk =  true; }
```
⇒ on DEV-28/29/30 a model that emits **a command with the exception dropped** passes if intent and place match. `Refuse` is the only verdict that demands a question. **Three of the four new refusal rows are unfalsifiable against the one failure mode `AS-§20.1` exists to prevent.**
**RULING: ship.** The verdicts are manager-pinned, DEV-08 consistency is worth more than a unilateral flip, and DEV-32 is deliberately `Refuse` so at least one new row can fail. ⛔ **But the batch may NOT be reported as "exclusion refusal is measured on the corpus." It is not.** The fix belongs at the boarded runner follow-up (teach `SpikeEval` the `except=` prefix and a "must ask" notion), and TASK-524's F-3 already names the cheap win. → W-4.

**(b) A green DEV-31 is NOT evidence the collapse leg is fixed. CONFIRMED.** Both spike fixtures carry all 13 kinds (`SpikeRoster` / `SpikeRosterT1`, bound by `static_assert` at `:517`) and the spike's `other_kinds:` is a **hardcoded constant**, so the collapse has **never happened on the eval lane, on any fixture, even in principle.**
✅ **And TASK-523's two collapse instruments DO assert against the SHIPPED builder, not a fixture** — verified: `Tests/SiegeAssistantSelectionTest.cpp:599` (`Scratch.Snapshot->BuildZoneC(LongOrder, LongPending)`) and `:815` (the 122-rung sweep) both call the real `USiegeAssistantSnapshot::BuildZoneC` on a snapshot whose private `Capture()`-owned state was filled through reflection. **They are the only instruments for the collapse leg and they are pointed at the right thing.** → W-5.

### RULING 5 — `AS-§20.3`'s headroom figure. **TASK-523's D1 is CORRECT AND MEASURED. The law needs a correcting amendment.**

Verified at the artifact — `SiegeAssistantSnapshot.cpp:1415-1419` prints **three** counts per row:
```cpp
Out.Appendf(TEXT("- %s: %d total, %d orderable, %d followable\n"), ...KindTotals..., ...KindOrderable..., ...KindFollowable...);
```
⇒ a two-digit board is **+3 chars per row = +39 across 13 kinds**. Reproducing the shipped operating point (head 108, tail 158, `RosterBudget` 627): a 13-kind block at single digits is **621** (fits, ~6 spare — `AS-§20.3`'s figure); at two digits it is **660**, which does **not** fit 627 ⇒ **one kind collapses at the shipped 61-char `order:` line on an ordinary mid-match board, and it is the Sorcerer.**
⇒ ⛔ **`AS-§20.3`'s risk statement is optimistic by a whole board-state dimension and must gain the count-magnitude qualifier. TASK-528 is materially more urgent than it reads.** This is a manager edit, not a code change, and Jonathan's ruling 1 is untouched by it.
✅ **TASK-523 was RIGHT not to assert it** — pinning "an ordinary board collapses" would make TASK-528's `ZoneBCharReserve` repair fail this file *for succeeding*. Reporting it via `AddInfo` (`:505-507`) and sweeping **both** magnitudes in Test 4 is the correct instrument design.

> ⭐ **AND THE ANSWER TO THE QUESTION THE SORCERER REPAIR RESTS ON: YES — TASK-517's `other_kinds:` NAMES genuinely make the collapse survivable, and the artifact proves it rather than the handoff claiming it.** The names are accumulated in the **same walk** that tallies the units, over `UnitKinds[PrintCount .. Num)` in the roster's own fixed order (`SiegeAssistantSnapshot.cpp:1448-1462`), so the line cannot disagree with the rows above it about *what* was hidden or *in what order*; the key is still always emitted (`:1464-1474`); and the line is printed **inside** the `[FORCES]` block, which is what makes Zone A's refusal rule true again. On BOTH board magnitudes the Sorcerer's **name** reaches the model. ⇒ **The failure mode drops from "the model refuses a unit that is alive" to "the model does not know how many there are." That is the whole repair, and it is the durable half — not the cap.**

---

## Findings

### BLOCKER — none.

### WARN

- **[WARN] W-1 — `SlateApplication.cpp:5027` / `SiegeAssistantConsoleWidget.h:518-524` — the preview tunnel has a SECOND precondition nobody documented: `CurrentWidget.Widget->IsEnabled()`.** The header records only the focus precondition. Today the console `UUserWidget` is never Slate-disabled (`ApplyConsoleVisualState` at `.cpp:1145-1169` disables only `InputBox` and `ConfirmButton`), so the accept key is live and there is no defect. **Fix:** one sentence in `NativeOnPreviewKeyDown`'s header comment — *"the tunnel also skips a Slate-DISABLED widget, so never `SetIsEnabled(false)` on this widget itself: it would kill typing AND the accept key, silently."* Doc-only; can ride TASK-526's fix pass or a follow-up.
- **[WARN] W-2 — `CONVENTIONS.md` `AS-§20.3` — the "~6 chars of headroom" row is the SINGLE-DIGIT reading and needs correcting.** See RULING 5. **Fix:** manager amends the Risk row to name the count magnitude (`+39 chars at two-digit counts ⇒ one kind already collapses at the default sentence length`), and re-rates TASK-528. ⛔ Not a code change and ⛔ not a re-escalation of Jonathan's ruling 1.
- **[WARN] W-3 — `SiegeAssistantConsoleWidget.cpp` / `SiegeAssistantComponent.cpp:677-682` — `OnConsoleCancelled` has ZERO live broadcasters, so a player cannot cancel a latched `Deferred` order from the console at all.** Verified: the only broadcaster was the widget's `CancelPressed()`, which nothing now calls; `NotifyConsoleClosed()` returns early on `Deferred` and deliberately **preserves** the latch. A deferred order can fire up to its TTL later, on a board the player has not looked at since, with no console-reachable drop. **Ruling: BOARDED FOLLOW-UP, not a blocker.** No code here is wrong — this is a downstream consequence of Jonathan's own ruling 3, the FSM branches were correctly *preserved* rather than deleted (`AS-§6` A-2 demands exactly that), and nothing crashes or lies. It is a **product gap** flagged by both TASK-519 §6(2) and TASK-520 §4 and owned by neither. **Fix:** manager boards it; **and it goes on TASK-527 as a named question for Jonathan** — *"do you want a way to drop a latched deferred order from the console?"* He is the only one who can price it.
- **[WARN] W-4 — `Docs/Data/assistant_eval_dev.csv` DEV-28/29/30 — three of four new refusal rows cannot fail a silent drop.** See RULING 4(a). Ships. ⛔ **Reporting duty: TASK-526's summary and the checkpoint must not describe the corpus as measuring exclusion refusal.** Runner follow-up is the fix.
- **[WARN] W-5 — a green `DEV-31` is not evidence about the collapse leg.** See RULING 4(b). Ships. Same reporting duty.
- **[WARN] W-6 — `SiegeAssistantCommand.h:478` — `SiegeAssistantValidateSelection`'s trailing-defaulted 4th parameter STAYS, but the QA grep proves less than it looks.** TASK-523's D3 is correct and I verified it: `ParseSiegeAssistantCommand`'s **final gate** at `SiegeAssistantCommand.cpp:881` is the identical 4-argument call on the same values, so the call at `SiegeAssistantComponent.cpp:1001` **cannot fire on the model path today** — dropping the argument would raise no compiler diagnostic, no test failure and no behavioural change. **RULING: keep the defaulted form for this batch.** Forcing the honest required-parameter form means hand-editing `SiegeAssistantComponent.cpp` and six call sites in `Tests/SiegeAssistantGrammarTest.cpp` *after* the gate, with no compile behind it, to harden a call that is already correct at both live sites. The compensating controls are adequate and real: the block comment at `SiegeAssistantComponent.cpp:987-999`, TASK-523's Test 11 (`ValidatorExclusionArgumentIsLoadBearing`) which asserts the 3-arg and 4-arg forms **disagree**, and the selector backstop at `:1919-1925`. ⛔ **Boarded condition: the day M8 P2 opens the wire path, the parameter becomes REQUIRED — that is when the edit is free because those files are being touched anyway.**
- **[WARN] W-7 — three file-ownership departures against RULING 1's table. All declared, all correct, all minimal — accept, and amend the `names:` blocks.** (a) `SiegeAssistantGrammar.h` (TASK-518, doc-only). (b) `Tests/SiegeAssistantGrammarTest.cpp` (TASK-523) — **the board contradicts itself**: TASK-518's entry item (b) assigns it to 523, while RULING 1's table and 523's own `names:` list it for nobody. 523 honoured the assignment and kept it to one re-based count (3 → 4) plus one `except` reference check (`:701-705`). ⭐ **And it re-based the COUNT, not the STRICTNESS** — refusing the `>= 3` "repair" that would have silently admitted a fifth `who` shape. That is the right call. (c) `SiegeCheatManager.cpp:670` (TASK-522) — one log line; the board's status calls it spec-authorized while the spec body says "ONE FILE PAIR". The old text was **outright false** after TASK-517 and no other task owned it; the new text is true, preserves the standing *"do NOT 'align' anything"*, and names the condition under which the divergence returns. **Fix:** manager amends the three `names:` blocks and RULING 1's table so the next batch's parallelism rests on a true document.
- **[WARN] W-8 — criterion 10 (the seal) is DISCHARGED ON EVIDENCE, NOT ON A GIT READING, because I have no shell or Git tool.** What I *can* attest: no changed file in this batch references either holdout; `Docs/Data/assistant_eval_dev.csv` is the only corpus file with batch content (header byte-identical, 33 lines each parsing to exactly 9 fields, zero `"` or `'`); TASK-524 §0 and TASK-521 §6 both state first-hand that `git status --porcelain Docs/Data/` showed one modified file. ⛔ **TASK-526 MUST re-run `git status --porcelain Docs/Data/` itself and confirm both holdouts are clean before committing** (`SC-§9`: a spec may say "run the check", never "it will be clean"). I am naming this as an owed check rather than claiming it.

### NIT

- **[NIT] N-1 — `SiegeKeyboardLayoutSubsystem.h:274-275` — `UFUNCTION` on its own line vs the registry's inline form. ACCEPT, no action.** Every token UHT or the linker can see is character-for-character identical (specifier set, `Category`, return `FKey`, `const FKey& QwertyKey`, trailing `const`). The file's other three reflected members already use this rendering (`:182`, `:200`, `:221`); matching the pin's literal layout would make this the only member formatted differently. Flagging it rather than burying it was correct.
- **[NIT] N-2 — `SiegeAssistantCommand.h:358` — `ExcludeArity` loses the registry's column-alignment padding.** Same class. Accept.
- **[NIT] N-3 — `exceptlist` vs `AS-§20.1`'s `except_list`. ACCEPT.** A properly declared `SC-§15` departure, pre-authorised by `AS-§20.1` itself, with the forcing reason in-source at `SiegeAssistantGrammar.cpp:636-645` (llama.cpp reads a rule name as `[a-zA-Z0-9-]` and stops at the underscore ⇒ the whole grammar is rejected and generation runs **unconstrained** — the `at_least` defect that cost TASK-413 two bars). Test 12 asserts `except_list ::=` is absent, so the underscore cannot come back.
- **[NIT] N-4 — `other_kinds: sorcerer (1 units)`. ACCEPT as pinned.** Declared, not an oversight (`SiegeAssistantSnapshot.cpp:1469-1473`). A pluralisation branch would spend characters out of a headroom W-2 says is already negative on an ordinary board. If Jonathan wants it, it is a ruling, not a fix.
- **[NIT] N-5 — `SiegeAssistantComponent.cpp:3159` — *"the ASSET lane is NOT the lane `zoneA_chars=5116` was measured on"*.** Still true (5116 is the spike lane's, unchanged), but after this batch a reader can conflate 5116 with the shipped Zone A, which is now 5424. One clause would close it. Non-blocking.
- **[NIT] N-6 — `SiegeAssistantComponent.cpp:1921` — the selection-plus-exclusion backstop's `Warning` level. RULING: `Warning` is CORRECT. Keep it.** The file's standing `Log`-for-model-refusals rule exists because the automation runner reads a `Warning` as a failure. This line is unreachable from the model (the parser's cross-field check 2 at `.cpp:855-873` and `SiegeAssistantValidateSelection`'s `Kinds.Num() > 0` clause both refuse the shape first), and no TASK-523 test has a world, so it **cannot redden a bar**. Its trigger genuinely is a code or wire defect — exactly the class `Warning` exists for. TASK-522 was right to add it and right to ask.
- **[NIT] N-7 — TASK-524's SEVENTH row (`DEV-32`) against a `names:` block that said six. ACCEPT; amend the block 6 → 7.** It is the **only** new row today's runner can score as *"the output must be a question"* (RULING 4a), it adds no new symbol, it continues the pinned numbering, and it deliberately reuses `DEV-04`'s noun so the only variable is the structure. Deleting it would remove the batch's single falsifiable refusal row.
- **[NIT] N-8 — TASK-521's `WHO =` schema line, +44 chars beyond its two rules. ACCEPT — and it was MANDATORY, not discretionary.** The block's own shipped comment makes Zone A the grammar's mirror in capitals. TASK-518 correctly moved the grammar's `who` rule and correctly stayed out of Zone A; without this edit the prompt would have **enumerated three `who` shapes while the sampler allowed four** — the "the rule was outvoted" failure loop 2 measured. Counted independently at the artifact: **44 + 111 + 153 = 308 of 325.**
  ⭐ **AND I ACCEPT THE STANDING LAW IT PROPOSES, verbatim in substance:** *a change to `SiegeAssistantGrammar.cpp`'s `root` / `command` / `who` / `selection` / `item` / `count` / `at-least` rules obliges the same-batch Zone-A schema-block edit, and the QA gate checks the pair.* **This batch is the proof it is needed: between two tasks that each correctly stayed in their own file, the mirror had NO OWNER.** → manager writes it into `AS-§9c` / `AS-§20`.
- **[NIT] N-9 — TASK-521's `:995` `[FORCES]` refusal-rule NO-OP. ACCEPT.** Checked against `AppendRosterBlock`'s actual output rather than the handoff's summary: `other_kinds:` is appended to the same buffer inside the `[FORCES]` block, so a collapsed symbol **is** a symbol in `[FORCES]` and the rule's antecedent is true again. The spec required any edit there to be char-neutral or negative and no addition can be. A deliberate no-op, stated with three reasons — which is the difference between a finding and a gap. If the model still refuses a collapsed kind at TASK-527, this is the first line to revisit; ~17 chars of slack exist for it.
- **[NIT] N-10 — TASK-522 declined a "(N units)" count and reports the ghost circles structurally cannot show an exclusion. ACCEPT — and the summary-text fix DOES close the confirm step's intent.** I checked the premise: the preview draws two **place** decals at the resolved destination and nothing per-unit, so no preview geometry depends on which units were selected — there is no ghost that can show a unit the order will not move. The real defect was the **sentence**: *"send everyone except the miners to mid"* rendered as **"Send (mid) — accept?"**, a prompt describing a different order from the one that would execute. `DescribeCommandForPlayer` (`SiegeAssistantComponent.cpp:1236-1262`) closes exactly that, inside `{Selection}`, with no new frame and no new template row. **The count is correctly refused:** it would need the selector run at *prompt* time on a board that can change before the player accepts — a second, staler survey that can disagree with execution. That is a new defect, not a fix.
- **[NIT] N-11 — TASK-520's `Thinking` asymmetry and its `AwaitConfirm`-alone test. BOTH CORRECT, and here is the answer it asked for.**
  > ⭐ **NO — there is NO path that reaches `AwaitConfirm` with an empty `PendingArgs`.** `EnterAwaitConfirm()` has **exactly one call site** in the whole component (`SiegeAssistantComponent.cpp:1549`), and it is reached only after `PendingArgs.Command = Command;` at `:1521`, on a command that has already passed the parser (so `Intent != None` by construction). The deferred fire re-enters through that same function with `bForceConfirmReview`. ⇒ **"State == AwaitConfirm" and "there is a parsed order the player was shown and has not accepted" are the same statement**, so `bDiscardedPendingOrder` is the state's *definition*, not a proxy for one, and a second `PendingArgs` clause would be dead code that invited a future reader to distrust the state. TASK-520's reasoning holds.
  The `Thinking` asymmetry matches `AS-§6` A-2's *"pushed on the AwaitConfirm→Idle close path ONLY"* verbatim, and the stated reason is right: in `Thinking` nothing was previewed and no order exists to lose, so a "cancelled" line would be a claim about something that never was.

---

## ✅ THE FIFTEEN CRITERIA — ROLL CALL

| # | criterion | verdict | evidence |
|---|---|---|---|
| 1 | `Escape` untouched, everywhere | ✅ **PASS** | `\bEscape\b` over the whole diff: hits **only in comments**, plus the shipped-unchanged `InputBox->SetRevertTextOnEscape(false)` (`.cpp:427`) — which is the line that *keeps* Escape unabsorbed. **Zero hits in `SiegeAssistantComponent.cpp`, `SiegeAssistantCommand.*`, `SiegeAssistantGrammar.cpp`, `SiegeAssistantSnapshot.*`, `SiegeKeyboardLayoutSubsystem.*`.** No handler returns `Handled` for it, harmlessly or otherwise. TASK-519's claim that the token does not appear in the implementation is **true**. |
| 2 | the `Z` intercept is narrow | ✅ **PASS** | `.cpp:866-905`. Gate = `bConsoleOpen && bConsoleEnabled && bConfirmPromptVisible`, **and** unmodified, **and** `GetKey() == GetAcceptKey()`. Exactly **one** `FReply::Handled()`, and it is after `ConfirmPressed()`. Every other path reaches `return Super::…` at `:931`. Traced by hand: console closed → Unhandled; open, no prompt → **types a normal `z`**; disabled → Unhandled; every other key → Unhandled. |
| 2b | ⭐ the modifier guard (beyond spec) | ✅ **ACCEPT — a real defect closed** | `Ctrl`/`Alt`/`Cmd` excluded. `Ctrl+Z` / `Ctrl+Shift+Z` are the box's own undo/redo; consuming them would kill undo **and** execute an order the player never asked for — the single worst outcome the confirm step exists to prevent. It makes the grab strictly **narrower**, which is what `AS-§20.5` demands. `Shift` correctly allowed. **Keep.** |
| 2c | `Handled` not re-checked after `ConfirmPressed()` | ✅ **AGREE with the programmer** | Acceptance is proven from the PRE-condition; a post-check would be **wrong**, not merely redundant — a consumer of `OnConsoleConfirmed` can synchronously re-enter `AwaitConfirm`, leaving the flag true and letting one physical press both execute an order and type a `z`. |
| 3 | positional resolution; prompt says `Z` | ✅ **PASS** | `.cpp:966` `LayoutSubsystem->GetPositionalKey(EKeys::Z)` — the pinned direction, never a reverse lookup. Fail-safe `.cpp:969` returns plain `EKeys::Z`. `GetPositionalKey` cannot return `EKeys::Invalid` (`SiegeKeyboardLayoutSubsystem.cpp:668-681`, `Find`-then-fallback, with the `FindRef` trap documented). **`"Semicolon"` appears in the repo only inside comments** — the player string is the literal `TEXT("Press Z to accept, or close this box to discard")` at `.cpp:71`, rendered at `:1201-1204`, ⛔ never built from `GetAcceptKey()`. `KBD-§8` satisfied. |
| 4 | ⭐ `ExcludeKinds` never silently ignored | ✅ **PASS — five independent gates, traced end to end** | Parser refuses on army-wide intents via the **shipped predicate itself** (`SiegeAssistantCommand.cpp:855-860`, gate = `!SiegeAssistantIntentTakesSelection(Parsed.Intent)` — no second verb list to drift). Validator re-checks (`:450-495`). `SiegeAssistantComponent.cpp:1001` passes the 4th argument — **grepped: exactly one call site in that file, and it carries four arguments.** Selector subtracts (`:2011-2020`). Empty-after-exclusion returns `false` → `ExecuteZoneOrder`/`ExecuteFollowOrder` → `ExecutePendingCommand` → **`ExecuteAndReport`'s UNCONDITIONAL `PushMessage(bExecuted ? Executed : AskUnsupported, …)` at `:1575`** ⇒ the player is told. ⛔ No new `ask` symbol, no new reason code at a call site, **no silent no-op anywhere on the path.** |
| 4b | exclusion is selector-only, ONE enforcement | ✅ **PASS** | The set of intents reaching the selector is **exactly** the set for which `SiegeAssistantIntentTakesSelection()` is true (`Send`/`Guard`/`Ambush`/`Follow` at `SiegeAssistantCommand.cpp:388-392`); `Charge`/`Fallback` are inline `ApplyArmyWideStance` statements and `ExecuteRallyOrder` takes no `Command` at all. **There is no second, divergent enforcement.** The executor's one extra refusal (`:1919`) is a *backstop* on a **different** shape (selection + exclusion), in the **same** direction, with the **same** verdict as two gates that already refuse it — it cannot produce an outcome they would not. |
| 5 | the count-controlled variant is structurally absent | ✅ **PASS** | `SiegeAssistantGrammar.cpp:650-665` — `exceptlist` alternatives reference `TEXT("kind")`, **never `item`**. There is **no `n` key anywhere in the exclusion rules.** Jonathan's declined feature is inexpressible by shape, not merely unimplemented. Test 12 asserts zero `item` references and no `\"n\"`. |
| 6 | the top-level JSON key set did not change | ✅ **PASS** | The exclusion is a **fourth `else if` on `Who->Type`** (`SiegeAssistantCommand.cpp:670`). The top-level `ValidateExactKeySet` is untouched; `bad_who` / `who_arity` / `who_required` / `duplicate_kind` all still fire with their own codes. **Every JSON that parsed before still parses, byte-for-byte.** Test 10 additionally asserts `except` as a fourth top-level key is refused `unknown_key`. |
| 7 | `other_kinds:` names, always emits, `none` intact | ✅ **PASS** | `SiegeAssistantSnapshot.cpp:1448-1474`. `MaxRosterKinds = 13` (`.h:403`); **`SnapshotTrimBudgetChars = 1085` and `ZoneBCharReserve = 192` UNCHANGED** (`.h:319`, `:355`). ⭐ **The "names are cheaper" claim CONFIRMED at the artifact:** a roster row costs `36 + len(symbol) + digits` = **43–49** chars (`:1415`); the same symbol on the collapse line costs `len(symbol) + 2`. The elastic trimmer, `RosterBudget` and the shrink loop are byte-identical; the loop still terminates unconditionally at `KindsToPrint <= 0` (`:1597`). |
| 8 | ⛔ Zone-A tests RE-BASED, not silenced | ✅ **PASS — and the replacement is STRICTLY STRONGER** | `TwoLaneByteEquality` now builds the **frozen fixture**, applies **exactly the three declared `D4` edits** (each its own compiler-measured literal at `:328-357`, each a byte-exact copy of the shipped/spike line — I diffed them against `SiegeAssistantSnapshot.cpp:850`, `:1050`, `:1095`, `:1115`), and asserts byte-equality with the shipped builder. ⛔ **`BuildZoneA`'s output is copied into the fixture NOWHERE.** It guards its own derivation (`:514-518` errors out if fewer than three edits matched — a `Replace` that matched nothing would silently degrade back into the ruled-false equality) and asserts the divergence is **still real** (`TestNotEqualSensitive`, `:558`). `D4` named in the message, `Plugins/SiegeLlama/**` untouched, chars **re-counted** (`:623-631` asserts 44/111/153 componentwise), `zoneA_tok = 1139` and `77.1 %` labelled **STALE — PENDING RE-MEASUREMENT** and ⛔ not recomputed. ⚠️ I also checked the one hazard nobody named: `FString::ReplaceInline` with a replacement **containing** the search string does **not** loop — `String.cpp.inl:1588-1610` moves the source into `Copy` and scans `Copy` while appending to `*this`, so the inserted text is never rescanned. |
| 9 | Zone A growth ≤ 325; frozen line untouched; no few-shot added | ✅ **PASS — counted, not accepted** | I counted the three emitted strings myself: `WHO =` delta **44**, rule 1 **111**, rule 2 **153** ⇒ **308 of 325, 17 spare.** The byte-frozen economy line (`:996+`) and the `[FORCES]` rule (`:995`) are byte-identical, and their **positions** are preserved (the two new rules sit *after* the selection rule at `:1050`, never above the frozen line). ⛔ No few-shot sentence added or altered; no kind symbol written into either rule (`KIND` is the metavariable). `ContextTokens` untouched. |
| 10 | 🔒 neither holdout opened | ⚠️ **DISCHARGED ON EVIDENCE — see W-8.** I have no shell/Git tool; TASK-526 must re-run `git status --porcelain Docs/Data/`. Nothing in the diff references either file. |
| 11 | `CancelPressed()` / `OnConsoleCancelled` / the existing template | ✅ **PASS** | See RULING 2. The `Cancelled` line is the **existing** `PushMessage(ESiegeAssistantReasonCode::Cancelled)` no-args overload (`SiegeAssistantComponent.cpp:757`) — the identical call `CancelPressed()` makes; the reason-code table is byte-unchanged; ⛔ **no new string authored at a call site, ⛔ no new broadcast added to the widget** (`CloseConsole()` verified byte-unchanged). Sampled **before** the discard (`:708`), pushed **last** (`:734-758`) — both orderings are correct and both are load-bearing. `Deferred` returns early so the latch survives (`:677-682`). |
| 12 | the five standing fences | ✅ **PASS** | Tree built **before** `Super::RebuildWidget()` · children wired in `NativeConstruct` · `ToggleConsole()` still uncalled (`.cpp:661`) · **no `SetInputMode` anywhere in the diff** · CLOSE ROUTE 4 + `ReopenSuppressionSeconds` + `LastRoute4CloseRealTimeSeconds` byte-unchanged (`.cpp:556-585`), with the new layout probe correctly placed **after** the suppression guard (`:609-612`). |
| 13 | standing C++ sweep | ✅ **PASS** | ⛔ **No `Build.cs` change** — `GitClaudeUnrealTest.Build.cs` carries no batch-era edit; `InputCore`, `UMG`, `Slate`, `SlateCore`, `Json` were all already there. Complete-type includes present and *justified in place* (`SiegeAssistantConsoleWidget.h:7-12` adds `InputCoreTypes.h` rather than leaning on `UserWidget.h`'s transitive pull — exactly right). No new member shadows an inherited reflected one (TASK-522 adds **no member at all**; TASK-519 adds one private `FString LastStateLabel`). No most-vexing-parse. No delegate broadcast on a no-op. Format specifiers match arity at every new `UE_LOG` I checked, including the 13-`%d`/2-`%s` `FIRST LIVE CAPTURE` line (`:3244-3257`). ⛔ **`TestEqualSensitive` on every `FString` claim** — verified mechanically across both test files: every surviving `TestEqual` compares `int32`s. `TestNotEqualSensitive` confirmed to exist in UE 5.8 (`AutomationTest.h:2021-2027`). Test-fixture APIs confirmed present: `AddExpectedMessagePlain` (`:1796`), `GetTransientPackageAsObject()`, `FindFProperty`. Reflection fixture checks **presence AND inner property type** (`SiegeAssistantSelectionTest.cpp:170-194`) and aborts by name — the vacuous-pass hole is closed; the six fields it reaches are all real `UPROPERTY(Transient)` members. |
| 14 | file ownership | ⚠️ **W-7** — three declared departures, all accepted, `names:` blocks to be amended. |
| 15 | the programmers' flags | ✅ **ALL RULED ABOVE.** Every one of the nine handoffs flagged rather than buried. **That is where every real finding in this report came from.** |

---

## Notes for build-master (TASK-526) — ⛔ WHAT TO WATCH FOR

**Compile — the six most likely failure sites, in order:**
1. **`Tests/SiegeAssistantSelectionTest.cpp` (NEW, ~1800 lines) is by far the largest new surface** and uses reflection (`FArrayProperty` / `FIntProperty` / `FNameProperty` / `ContainerPtrToValuePtr`), `TStrongObjectPtr`, `NewObject<UGameInstance>(GetTransientPackageAsObject())` and `NewObject<USiegeKeyboardLayoutSubsystem>(GameInstance.Get())`. Includes look complete and every API I checked exists in 5.8. **If anything in this batch fails to compile, look here first.**
2. **`USiegeAssistantConsoleWidget::NativeOnPreviewKeyDown` is this repo's FIRST `FReply` override anywhere** — zero precedent in `Source/`. Watch for a signature mismatch against `UUserWidget`'s `virtual FReply NativeOnPreviewKeyDown(const FGeometry&, const FKeyEvent&)`.
3. **`ResolveKeyboardLayoutSubsystem() const`** calls `GetGameInstance()` (which has a non-template and a template overload on `UWidget`) then `GetSubsystem<>()` on a `const UGameInstance*`. Both are `const` in 5.8 and the non-template wins overload resolution — but this is the one new call chain with an overload-resolution question in it.
4. **`SiegeAssistantValidateSelection`'s default argument is declared ONLY in the header** (`SiegeAssistantCommand.h:478`) and must NOT be repeated in the `.cpp` definition (`:401`). Verified correct as written — a redeclaration is a hard error.
5. **Warnings-as-errors:** TASK-519 deleted `Components/HorizontalBox.h` / `HorizontalBoxSlot.h` includes and `ButtonLabelSize`. Confirm nothing else still references them.
6. ⛔ **A failure whose diagnostic names a file in `Plugins/SiegeLlama/` is FOREIGN to this batch** — route it to the FINE-TUNE board, never record it as this lane's finding. (`Build.bat …Editor` builds the target, so a half-written spike CAN fail this gate.)

**Test run — what a green suite does and does not prove:**
- Expect **40 + 13 new = 53** tests, with **4 re-based** (3 in `SiegeAssistantZoneATest.cpp`, 1 in `SiegeAssistantGrammarTest.cpp`). ⛔ **A count below that means a test failed to register, not that one passed.**
- ⭐ **`Selection.CollapseNamesTheHiddenKinds` and `Selection.ShrinkLoopNeverHidesASymbol` are the batch's headline instruments** — they are the ONLY things that measure Jonathan's Sorcerer defect. If either goes red, **stop and route back**; do not commit.
- ⛔ **`ZoneA.TwoLaneByteEquality` failing means one of exactly two things**: a fourth, undeclared Zone-A edit exists, or the frozen spike fixture moved. ⛔ **In NEITHER case is the fix to re-copy `BuildZoneA`'s output into the fixture.** The test says so itself at `:517`.
- `ShrinkLoopNeverHidesASymbol` runs 122 `BuildZoneC` calls. If it is slow, that is a note, not a failure.
- ⛔ **A green suite proves NOTHING about:** the `Z` key firing (no headless Slate focus), the executor's exclusion filter (no world, no actors), the `Cancelled` transcript line (no headless path into `AwaitConfirm`), or the on-screen prompt. **All four are TASK-527's.**

**Before the commit:**
- ⛔ **Re-run `git status --porcelain` yourself and branch on the answer** (`SC-§9`). Jonathan self-commits and pushes without telling the pipeline. ⛔ **NEVER push.**
- ⛔ **W-8: re-run `git status --porcelain Docs/Data/` and confirm both holdouts are clean.** That is the one gate criterion I could not discharge first-hand.
- Confirm TASK-489 · 490 · 473 · 501 · 528 are still un-dispatched.
- ⛔ **If you author a compile fix, that diff is CODE and owes an `SC-§27` diff-scoped verdict before the commit** — changed lines plus the fences the fix must not have disturbed. Board it; do not assume it away.

**Reporting duties this gate imposes on TASK-526's summary and the checkpoint:**
- ⛔ **Do NOT report exclusion refusal as "measured on the corpus."** W-4: three of four new refusal rows cannot fail a silent drop.
- ⛔ **Do NOT report a green `DEV-31` as "the Sorcerer defect is fixed."** W-5: the collapse has never happened on the eval lane; only TASK-523's tests see it.
- ⛔ **Any dev score after 2026-08-04 is NOT comparable to one before it** (25 → 32 rows). A ladder graph spanning this date without a break is a false trend.

**Owed to the manager (not to build-master), from this gate:**
`W-2` (amend `AS-§20.3`; re-rate TASK-528) · `W-3` (board the deferred-cancel gap **and put it on TASK-527's question list**) · `W-6` (board "the parameter becomes required when M8 P2 opens") · `W-7` (amend three `names:` blocks + RULING 1's table) · `N-8` (write the grammar↔Zone-A mirror law).
