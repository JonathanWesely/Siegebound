# TASK-1307 — [FOCUS-READBACK-INSTRUMENT] — gameplay-programmer handoff

**Status:** `ready-for-qa` · **Gate:** `TASK-1308` · **Host:** `TASK-1309`
**Baseline:** `HEAD = 89752eb`; the `.cpp` and `.h` are **byte-identical to `1d4ae90`** at HEAD
(`git diff --stat 1d4ae90 HEAD -- ...DeckBuilderWidget.cpp` → empty) ⇒ **`DECK-§9`'s measurement
baseline is exactly the file I edited.**
**Written:** `DeckBuilderWidget.cpp`, `DeckBuilderWidget.h`, this file. **Nothing else.**
⛔ No compile, no Git, no `.uasset`, no test, no board row but my own, no `CONVENTIONS.md`.

---

## 0 · ⚠️ THE TWO FOREIGN FILES IN `git status` ARE THE OTHER TWO LANES, AND `SC-§102` DOES **NOT** FIRE

`git status --porcelain` at hand-off:

```
 M GitClaudeUnrealTest/Content/Blueprints/BP_MenuGameMode.uasset      <- ⛔ NOT MINE (TASK-1296's menu .uasset)
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp   <- mine
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h     <- mine
 M GitClaudeUnrealTest/Tools/run_suite_bounded.ps1                    <- ⛔ NOT MINE (TASK-1294's suite-lane file)
```

⛔ **My change landed on `DeckBuilderWidget.{h,cpp}` and on nothing else.** The other two entries are
the concurrent lanes writing their own declared files, exactly as the dispatch predicted, so the
`SC-§102` STOP condition (*"if your change lands on either of those other files"*) is **not met** and I
did not stop. ⭐ **I did not read, stage, revert or touch either file.**
⚠️ **For `TASK-1309`: commit BY PATHSPEC.** A bare `git commit -a` here would swallow two other rows'
work into a `TASK-1307`-subject commit.

⚠️ Also recorded because it bit me mid-row and is the cheapest lesson here: **`Tools/run_suite_bounded.ps1`
NOW EXISTS.** `MEMORY` still carries *"`Tools/run_suite_bounded.ps1` DOES NOT EXIST though `TL-§6` names
it the sanctioned executor"* — `TASK-1294` is creating it in the lane beside me. Not my row, not my
claim to close; flagged so nobody re-derives it.

---

## 1 · CURRENT `file:line` FOR EVERY SITE TOUCHED — **quoted BEFORE the diff**, `SC-§91`, re-grepped at my instant

⛔ The row's spec quoted line numbers read at `1d4ae90`. Here is what they read **now**, before my edit,
and what they read **after**. The pre-edit column is recoverable from `git show HEAD:<path>`.

| site | pre-edit (`89752eb` = `1d4ae90`) | post-edit | what I did |
|---|---|---|---|
| `AcquireBuilderFocus` definition | `cpp:1472` | `cpp:1472` | unmoved |
| its only call site, in `NativeConstruct` | `cpp:670` | `cpp:670` | **untouched** |
| the deferred-lane explanation comment | `cpp:1528-1537` | `cpp:1528-1537` | **untouched** |
| the `Log`-not-`Verbose` rationale comment | `cpp:1546-1550` | `cpp:1564-1568` | text unmoved, shifted by my insert above it |
| the read-back `GetUserFocusedWidget` | `cpp:1551` | `cpp:1597` | unmoved logically; now explicitly labelled PRE-FLUSH |
| the read-back `UE_LOG` | `cpp:1552-1557` | `cpp:1598-1611` | **rewritten** (Faults A + B + WARN-2) |
| ⭐ the return expression | `cpp:1559` | `cpp:1613` | ⛔ **BYTE-IDENTICAL, not touched** |
| the `IsFocusable()` assert | `cpp:1484-1487` | `cpp:1484-1487` | untouched |
| `CancelFocusRequest` / `GetSlateOperations().SetUserFocus` | `cpp:1524` / `cpp:1537` | same | ⛔ untouched — **no focus behaviour changed** |
| the rider citation | `h:227` | `h:227-233` | comment-only, see §6 |

**New sites (all additive):**

| new site | `file:line` |
|---|---|
| the arm | `cpp:1550` (`bFocusReadbackPending = true;`) + `cpp:1551` (`FocusRequestFrameCounter = GFrameCounter;`) |
| `UDeckBuilderWidget::NativeTick` | `cpp:1820` |
| ⭐ the once-per-open guard | `cpp:1835` |
| ⭐ the flag clear (**before** the log) | `cpp:1841` |
| the dispatch to the read-back | `cpp:1842` |
| ⭐ `UDeckBuilderWidget::NativeDestruct` (teardown) | `cpp:1846`, disarm at `cpp:1854` |
| `UDeckBuilderWidget::LogPostFlushFocusReadback` | `cpp:1859` |
| the `UNREADABLE` third answer | `cpp:1876-1882` |
| ⭐ the identity verdict | `cpp:1894` (`bIsThisBuilder`) |
| ⭐ the POST-FLUSH line | `cpp:1900-1913` |
| `NativeTick` decl | `h:730` · `NativeDestruct` decl `h:744` |
| `bFocusReadbackPending` | `h:826` · `FocusRequestFrameCounter` `h:837` |
| `LogPostFlushFocusReadback` decl | `h:855` |
| `AcquireBuilderFocus` doc addendum | `h:969-978` (decl still `h:980`) |

---

## 2 · 🚨 FAULT B — **CONFIRMED**, at engine source, with the `file:line` I read

⭐ **THE MANAGER IS RIGHT. `GetTypeAsString()` cannot discriminate this builder from any other
`UUserWidget`, and I did not take it on trust (`SC-§101`).** The chain, read end to end in
`UE_5.8/Engine/Source`:

1. **Every `UUserWidget` is wrapped in an `SObjectWidget`** — `Runtime/UMG/Private/Components/Widget.cpp:975`
   and `:980`, both branches of `UWidget::TakeWidget()`:
   `return TakeWidget_Private([](UUserWidget* Widget, TSharedRef<SWidget> Content) -> TSharedPtr<SObjectWidget> { return SNew(SObjectWidget, Widget)[Content]; });`
   The result is assigned to `MyGCWidget` at `Widget.cpp:1023` and returned by `GetCachedWidget()` at
   `Widget.cpp:1104-1106` — **which is the very pointer `AcquireBuilderFocus` holds in `SelfSlate`.**
2. **`SNew` stringifies the literal type name** — `Runtime/SlateCore/Public/Widgets/DeclarativeSyntaxSupport.h:37-38`:
   `MakeTDecl<WidgetType>( #WidgetType, __FILE__, __LINE__, ... )` ⇒ the `ANSICHAR*` is the token
   `SObjectWidget`, not the owning `UUserWidget` class.
3. **It is handed straight to `SetDebugInfo`** — `DeclarativeSyntaxSupport.h:929`:
   `_Widget->SetDebugInfo(InType, InFile, OnLine, sizeof(WidgetType));`
4. **`SetDebugInfo` assigns `TypeOfWidget`** — `Runtime/SlateCore/Private/Widgets/SWidget.cpp:1398-1400`:
   `TypeOfWidget = InType;`
5. **`GetTypeAsString()` returns exactly that** — `SWidget.cpp:1116-1119`:
   `FString SWidget::GetTypeAsString() const { return this->TypeOfWidget.ToString(); }`

⇒ ⛔ **`GetTypeAsString()` returns the constant string `"SObjectWidget"` for the cached widget of
EVERY `UUserWidget` in the tree — `WBP_MainMenu`, `WBP_DeckBuilder`, every card tile that is a user
widget.** A matching pair proves only *"a UMG widget has focus"*, which is nearly always true on this
screen. **Even taken post-flush it would have been a near-tautology, so fixing Fault A alone would
have produced an instrument that reads PASS on a run where focus landed on the WRONG widget.**

⭐ **And the function's `return` has always been right:** `return bTookFocus && Focused == SelfSlate;`
(`cpp:1613`) compares `TSharedPtr<SWidget>` — **pointer identity**. ⇒ the log was strictly weaker than
the return. **The fix makes the log at least as strong: the POST-FLUSH verdict is the same pointer
comparison**, with the two type names demoted to context that the line itself labels as context.

### 2b · ⚠️ A THIRD FAULT ON THE SAME LINE — QA's `TASK-1305` WARN-2, RE-MEASURED AND CARRIED INTO THE TEXT

WARN-2 (`qa/TASK-1305-report.md:469-480`) said `TAKEN|DEFERRED` is not a pass signal because
`SetUserFocus` returns `false` when the widget is **already** focused. **I re-read it rather than
citing it** — `Runtime/Slate/Private/Framework/Application/SlateApplication.cpp:3028-3032`:

```cpp
// Is we aren't changing focus then simply return
if (WidgetToFocus.Widget == OldFocusedWidget)
{
    return false;
}
```

⛔ **Confirmed.** A re-entrant or already-focused open prints `DEFERRED` **while focus is exactly
where it belongs**. ⇒ the word survives in the line but is now **labelled in the line itself** —
`SetUserFocus returned DEFERRED to the next frame (diagnostic colour only, never a pass signal)` — so
the next reader cannot misread it a third time. ⛔ **This also means `bTookFocus` is NOT part of the
POST-FLUSH verdict:** that line's `IDENTITY=` is derived from the pointer comparison alone.

---

## 3 · 🚨 THE ROUTE — **I REJECTED THE ROW'S FIRST SUGGESTION, AND THE ENGINE LOOP IS WHY** (`SC-§101`)

The row offered *"a next-tick timer (`SetTimerForNextTick`) **or** the first `NativeTick`"*, route
mine. ⛔ **`SetTimerForNextTick` is still pre-flush in the case a human actually produces.** Measured,
not assumed — one `FEngineLoop::Tick` iteration, `Runtime/Launch/Private/LaunchEngineLoop.cpp`:

| phase | line | what lands here |
|---|---|---|
| (a) `GEngine->Tick()` — the world tick | `:5859` | ⛔ `FTimerManager`, i.e. **`SetTimerForNextTick` fires HERE** |
| (b) `ProcessLocalPlayerSlateOperations()` | `:5918` | ⭐ **THE FOCUS FLUSH** (*"Process slate operations accumulated in the world ticks"*) |
| (c) `FSlateApplication::Tick(PlatformAndInput)` | `:5921` | a **mouse click** on the menu button runs here ⇒ `AddToViewport` → `NativeConstruct` |
| (d) `FSlateApplication::Tick(TimeAndWidgets)` | `:5991` | ⭐ `SObjectWidget::Tick` → **`UUserWidget::NativeTick`** |
| (e) `GFrameCounter++` | `:6131` | ⇒ **(a)…(d) of ONE iteration all read the SAME counter** |

- **Opened from a Slate click** (his actual route, and the one that matters): the focus op is queued at
  **(c) of frame N**, so it is flushed at **(b) of frame N+1**. ⛔ **A next-tick timer fires at (a) of
  frame N+1 — BEFORE that flush.** ⇒ the timer route ships **the same defect, one frame along**.
- **First `NativeTick` alone is also insufficient**: in that same case, **(d) of frame N** runs before
  the frame-N+1 flush too.

✅ **WHAT IS ACTUALLY GUARANTEED, and it is the only thing I relied on:** because **(b) precedes both
(c) and (d) within its own iteration**, an op queued anywhere in frame N is flushed no later than
**(b) of frame N+1**, and therefore **any `NativeTick` in a frame STRICTLY LATER than the construct
frame is post-flush — in every case, click / Enhanced Input / another widget's tick alike.**

⇒ **Route chosen: `NativeTick`, gated on `GFrameCounter > FocusRequestFrameCounter`** — strictly
greater, never `>=`, precisely because (e) makes all four phases of one iteration share a counter
value.

### 3a · ⭐ `NativeTick` WILL ACTUALLY FIRE — **MEASURED ON THE LIVE ASSET, NOT ASSUMED** (`SC-§123`)

The obvious way for this instrument to fail silently is for the widget never to tick, which would put
us straight back to *an empty log reading as a pass*. So I closed it both ways:

- **At source:** `UUserWidget::UpdateCanTick` sets `bCanTick |= !WidgetBPClass || WidgetBPClass->ClassRequiresNativeTick();`
  (`Runtime/UMG/Private/UserWidget.cpp:2360`), and that flag is
  `!NativeParent->HasMetaData("DisableNativeTick")` (`Editor/UMGEditor/Private/WidgetBlueprint.cpp:1563`).
  **`UDeckBuilderWidget` is a bare `UCLASS()` (`h:45`) with no such meta** ⇒ the flag is **true**. The
  `!WidgetBPClass` disjunct additionally covers the C++-only widgets built in `SiegeDeckSlotsTest.cpp`.
- **On the live asset** (read-only, `execute_unreal_python_readonly` against the running editor
  **PID 1812** — ⛔ nothing written, nothing saved, no `.uasset` touched):
  `WBP_DeckBuilder_C` CDO → **`TickFrequency = WidgetTickFrequency.AUTO`**, and (incidentally)
  **`bIsFocusable = True`**, confirming `TASK-1304`'s shipped asset state is intact.

⇒ ⛔ **This widget was ALREADY ticking before my diff.** The override adds **one guarded branch while
the builder is open and nothing at all once it has fired** — it is not a new per-frame cost.

---

## 4 · THE THREE REQUIREMENTS THE ROW NAMED, EACH AT A `file:line`

**(i) EXACTLY ONCE PER BUILDER OPEN — `cpp:1835` + `cpp:1841`.**
`if (bFocusReadbackPending && GFrameCounter > FocusRequestFrameCounter)` then **`bFocusReadbackPending = false;`
on the line BEFORE the call**, not after. ⛔ Cleared first deliberately: `LogPostFlushFocusReadback()`
has its own early returns, and clearing afterwards would let a widget that momentarily lost its local
player re-enter the branch on the next frame. **One line per open, whatever the read-back does.**
⛔ **Per construct, not per class:** the flag is a **plain per-instance member** (`h:826`) armed inside
`AcquireBuilderFocus` (`cpp:1550`), whose only caller is `NativeConstruct` (`cpp:670`) ⇒ **it re-arms
on every open of the same instance.** Nothing is `static`.
⭐ **Armed at `cpp:1550`, i.e. AFTER the three early returns, not in `NativeConstruct`** — so a session
with no Slate / no cached widget / no local player leaves it **disarmed**, and the absence of a
post-flush line there is *correct* rather than a silent failure.

**(ii) SURVIVES A WIDGET THAT DIES FIRST — `cpp:1846`, disarm at `cpp:1854`.**
⭐ **The crash class the row warned about is removed STRUCTURALLY, not cleaned up after:** choosing
`NativeTick` over a timer means **there is no handle and no lambda** — a tick cannot be delivered to a
destroyed widget, so there is nothing that can dangle into a dead `UUserWidget`. `NativeDestruct` is
still added, for a different reason I want QA to check rather than assume: **a `UUserWidget` is REUSED
across `RemoveFromParent`/`AddToViewport`**, so a builder torn down before its first post-flush tick
would otherwise carry a **stale armed flag into its NEXT open** and print a read-back for a request
that open never made. `Super::NativeDestruct()` is called (`cpp:1856`).

**(iii) THE TWO MOMENTS ARE DISTINGUISHABLE FROM THE TEXT ALONE.** I took the row's ✅ PREFERRED
steer: **the construct-time line is KEPT and relabelled**, the post-flush line is added as the one
that answers. Two reasons for keeping it, argued rather than assumed: its **absence** still means
*"the call site was never reached"* (the trap QA named at `TASK-1305` (4), and the trap that proved
useful at `TASK-1306` — `total_matches: 0` → `1` is how we know the call site ran at all); and the
`bTookFocus == true` branch genuinely **is** answerable at that moment. The PRE-FLUSH line now says,
in its own text, that **a `NO-MATCH` there is EXPECTED** — ⛔ the row's core complaint, that a healthy
run reads as a failure, is removed **from the old line as well as absent from the new one**.

**(iv) VERBOSITY.** ⛔ **All three new/changed lines are `UE_LOG(LogGitClaudeUnrealTest, Log, ...)`.**
Nothing was "tidied" to `Verbose`. Grep proof: `grep -c "LogGitClaudeUnrealTest, Verbose" ` over my
added lines = **0**.

**(v) THE VERDICT IS IDENTITY-BASED — `cpp:1894`.**
`const bool bIsThisBuilder = Focused.IsValid() && Focused == SelfSlate;` — the **same** comparison the
`return` at `cpp:1613` has always made, now made where it can be true. Type names and both **raw
`SWidget` addresses** ride along as context, and the line **says** they are context.

---

## 5 · ⭐ THE GREPPABLE SUBSTRINGS — **VERBATIM, FOR `TASK-1309`'s VERIFY LEG TO PIN**

⛔ **THE ONE TO PIN (the answering moment):**

```
AcquireBuilderFocus POST-FLUSH read-back
```

⛔ **THE VERDICT TOKENS — plain substrings, no regex metacharacters, mutually exclusive:**

| meaning | exact substring |
|---|---|
| ✅ the builder holds focus | `POST-FLUSH read-back: IDENTITY=MATCH` |
| ❌ something else holds focus | `POST-FLUSH read-back: IDENTITY=NO-MATCH` |
| ⚠️ could not look (widget/player gone) — ⛔ **NOT a failure** | `POST-FLUSH read-back: IDENTITY=UNREADABLE` |
| the construct-time line (⛔ never the verdict) | `AcquireBuilderFocus PRE-FLUSH read-back` |
| both moments at once (back-compatible with `TASK-1306`'s grep) | `AcquireBuilderFocus` |

⭐ **`MATCH` IS HYPHENATED AS `NO-MATCH` ON PURPOSE, AND THIS IS THE ONE DELIBERATE DEVIATION FROM THE
ROW'S WORDING** (the row wrote *"`MATCH`/`NO MATCH`"*): **`"IDENTITY=NO-MATCH"` does not contain
`"IDENTITY=MATCH"`**, so a plain substring search for the positive token **cannot be satisfied by the
negative one**. With a space, every grep for the pass would also hit every fail.

⚠️ **AND I CAUGHT THE SAME CLASS OF BUG IN MY OWN FIRST DRAFT, which is worth one line because it is
exactly this row's disease:** my PRE-FLUSH line originally ended *"The POST-FLUSH read-back line
answers this"* — which **contained the pin substring**, so a grep for the answering line would have
matched the pre-flush line too and the instrument would have lied again, one level up. The shipped
wording is *"A second read-back line follows and carries the verdict"* — ⛔ **the string `POST-FLUSH`
appears in the PRE-FLUSH line ZERO times.** Verified: `sed -n '1598,1612p' … | grep POST-FLUSH` → no
output.

**Expected shape of a healthy run (two lines, in this order, once each per builder open):**

```
LogGitClaudeUnrealTest: UDeckBuilderWidget::AcquireBuilderFocus PRE-FLUSH read-back: IDENTITY=NO-MATCH (NOT the verdict - a NO-MATCH here is EXPECTED whenever the request was deferred); SetUserFocus returned DEFERRED to the next frame (diagnostic colour only, never a pass signal); focused SWidget 0x0000000000000000 ('<none>') vs this builder's SWidget 0x00000XXXXXXXXXXX ('SObjectWidget'); IsFocusable()=true. A second read-back line follows and carries the verdict; if none follows, this builder was torn down or stopped ticking before the flush.
LogGitClaudeUnrealTest: UDeckBuilderWidget::AcquireBuilderFocus POST-FLUSH read-back: IDENTITY=MATCH (THIS is the verdict - taken 1 frame(s) after the request, past FEngineLoop::ProcessLocalPlayerSlateOperations); focused SWidget 0x00000XXXXXXXXXXX ('SObjectWidget') vs this builder's SWidget 0x00000XXXXXXXXXXX ('SObjectWidget'); IsFocusable()=true. The two type names are context, never the test - 'SObjectWidget' is every UUserWidget's Slate type.
```

⛔ **THE FAILURE MODES, STATED SO NONE OF THEM READS AS A PASS:**
- **Zero lines** ⇒ `AcquireBuilderFocus` was never reached (`NativeConstruct` did not run, or the
  binaries are stale). ⛔ **Not a pass.**
- **PRE-FLUSH only, no POST-FLUSH** ⇒ the builder was destroyed, or stopped ticking, before a
  later-frame tick. ⛔ **Not a pass** — and the PRE-FLUSH line says so in its own text.
- **`IDENTITY=UNREADABLE`** ⇒ third answer, deliberately distinguishable from `NO-MATCH`.
- ⛔ **`PRE-FLUSH … IDENTITY=NO-MATCH` is EXPECTED and is evidence of NOTHING.**

⚠️ **All three lines are pure ASCII inside the `TEXT()` literals** (no emoji, no em dash) so the pin
substring cannot be broken by a log-encoding round trip. Verified: the diff's added `TEXT(` lines
contain **0** non-ASCII bytes. The emoji live in the comments only.

---

## 6 · THE RIDER (4) — ⛔ **COMMENT-ONLY, 0 EXECUTABLE LINES**, and here is the whole hunk

Single hunk, `DeckBuilderWidget.h` — **`@@ -227 +227,7 @@`**, **one comment line replaced by seven
comment lines, every one of them beginning `//`:**

```diff
-	//  digits/punctuation/Enter/Escape BY DESIGN, CONVENTIONS:2519-2522); ⛔ Tab
+	//  digits/punctuation/Enter/Escape BY DESIGN — CONVENTIONS `KBD-§4`, SECOND
+	//  BULLET. ⛔ The SECTION is the citation; the line number is corroboration
+	//  only. This claim read CONVENTIONS:2519-2522 when the comment was written
+	//  and reads CONVENTIONS.md:2528 — ONE line, not a four-line range — on
+	//  2026-09-18, three inserts later in that same file on that same day. ⇒ a
+	//  `§` does not drift when the file grows; a line number drifts every
+	//  time); ⛔ Tab
```

✅ **The fix cites the SECTION (`KBD-§4`), with the line as corroboration** — ⛔ **not a bare new line
number**, which is what `TASK-1308` WARNs on.
✅ **Re-measured at my instant, not copied from the row:** `CONVENTIONS.md:2525` is
`### KBD-§4. SCOPE LAW — ⛔ LETTERS ONLY…`; the claim itself is the **second bullet at `:2528`** —
*"⛔ DELIBERATELY NOT REMAPPED: digits (the `1`–`6` hotkeys), punctuation, modifiers …, `Space`,
`Enter`, `Escape`."* ⇒ **one line, not the 4-line range the old citation named.** The claim is TRUE;
only the address had drifted.
✅ Parenthesis balance preserved (`(it tables … time)` still closes exactly once; whole-file paren
delta is **-2 before and -2 after**, i.e. my delta is **0** against a pre-existing comment imbalance).

---

## 7 · ⭐ `DECK-§9` — **THE CENSUS RE-RUN ON MY OWN DIFF: 20 BOUND EXPRESSIONS ON 7 EXECUTABLE LINES. UNCHANGED.**

⛔ **THE NUMBER IS `20`, ON `7` LINES.** Re-derived from the working tree, not quoted from the law:

| line (post-edit) | expression count | keys |
|---|---|---|
| `cpp:1225` | **3** | `Left` · `Gamepad_DPad_Left` · `Gamepad_LeftStick_Left` |
| `cpp:1229` | **3** | `Right` · `Gamepad_DPad_Right` · `Gamepad_LeftStick_Right` |
| `cpp:1233` | **3** | `Up` · `Gamepad_DPad_Up` · `Gamepad_LeftStick_Up` |
| `cpp:1237` | **3** | `Down` · `Gamepad_DPad_Down` · `Gamepad_LeftStick_Down` |
| `cpp:1639` | **3** | `Enter` · `Virtual_Gamepad_Accept.GetVirtualKey()` · `Gamepad_FaceButton_Bottom` |
| `cpp:1663` | **2** | `Delete` · `Gamepad_FaceButton_Left` |
| `cpp:1727` | **3** | `Escape` · `Gamepad_FaceButton_Right` · `Virtual_Gamepad_Back.GetVirtualKey()` |
| **TOTAL** | ⭐ **20 on 7 lines** | ⛔ **identical to `DECK-§9` cl. 2** |

⭐ **AND THE STRONGER PROOF — THE TABLE DIFFED `0` AT THE BYTE LEVEL, NOT MERELY AT THE TALLY
(`SC-§104`: assert STATE, not tallies):**

```
$ git diff -- Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp | grep -E "^[-+].*(EKeys::|GetVirtualKey)"
(no output)
```

⇒ ⛔ **My diff adds and removes ZERO lines containing `EKeys::` or `GetVirtualKey`.** `EKeys::Tab`
remains at **0** occurrences; no letter, no digit was added anywhere; `KBD-§4`'s tables are untouched
(I changed a *citation of* `KBD-§4`, never `KBD-§`'s own code, which lives in
`USiegeKeyboardLayoutSubsystem` and is not in my file set). `NativeOnPreviewKeyDown` (`cpp:1769`) and
`NativeOnKeyDown` (`cpp:1791`) are **unchanged** — the two-pass fence is byte-identical.

### 7a · ⚠️ `DECK-§9`'s LINE NUMBERS DRIFT BY `+54` — **THE MANAGER'S TO AMEND, AND HERE IS THE MAP**

⛔ **The CENSUS is unchanged; the ADDRESSES moved**, because my insert sits inside
`AcquireBuilderFocus` (`cpp:1472-1613`), above the `HandleCardGridKey` block. ⛔ **I may not edit
`CONVENTIONS.md`**, so this is handed over rather than fixed:

| `DECK-§9` cites | now reads | drift |
|---|---|---|
| `:1224` (the `Tab`-absent words) | `:1224` | **0** |
| `:1225` `:1229` `:1233` `:1237` (the movement table) | unchanged | **0** |
| `:1562` `HandleCardGridKey` | `:1616` | +54 |
| `:1585` Accept | `:1639` | +54 |
| `:1609` Remove | `:1663` | +54 |
| `:1673` Exit | `:1727` | +54 |
| `:1683` dispatch | `:1737` (`NavigationFromKey` dispatch) | +54 |
| `:1697` enter-the-grid | `:1751` | +54 |
| `:1706` (`Tab` in words) | `:1760` | +54 |
| `:1715` `NativeOnPreviewKeyDown` | `:1769` | +54 |
| `:1737` `NativeOnKeyDown` | `:1791` | +54 |

⇒ ⭐ **This is the rider's own lesson arriving one hour later and in the opposite direction: I moved
eleven of `DECK-§9`'s line citations without touching a single thing they describe.** The table's
**Site** column would read cleanly if its cells cited the *function* plus the line, the way rider (4)
now cites the `§` plus the line. Offered to the manager as an observation; ⛔ not my file, ⛔ not
boarded, ⛔ not done.

---

## 8 · ⛔ NO NEW TEST — **AND THE REASON IS A MEASUREMENT** (`SC-§105` cl. 4b · `SC-§94` · `SC-§123`)

⛔ **Suite total must come back UNCHANGED at `561`. I added, removed and edited ZERO test files.**

**The reason, measured at the line:** `AcquireBuilderFocus` returns early at

```cpp
	if (!FSlateApplication::IsInitialized())
	{
		return false; // offline automation lane: there is no Slate focus to take
	}
```

— **`DeckBuilderWidget.cpp:1489`** (unchanged by me), and **`LogPostFlushFocusReadback` repeats the
same guard at `cpp:1864`**. ⇒ in the offline `-nullrhi` automation lane:

1. **`FSlateApplication::IsInitialized()` is false**, so the function returns **before** the arm at
   `cpp:1550` ⇒ `bFocusReadbackPending` is **never set**;
2. there is **no Slate focus to read** and **no `ProcessLocalPlayerSlateOperations` flush to be after**,
   so the very ordering this row is about **does not exist** there;
3. an offline test would therefore assert **the early-return path** — it would be **GREEN ON THE BUG**,
   green before my diff and green after it, and would have been green on `1d4ae90` too.

⚠️ **AND I DID NOT ADD A DIRECT-INVOCATION TEST TO LOOK THOROUGH.** Calling
`LogPostFlushFocusReadback()` by hand would prove *mechanism-when-invoked* — which is precisely what
**561 green tests "proved" about a feature that could not execute one line of its own entry path**
(`SC-§123`). ⛔ The claim this row makes is about **WHICH FRAME the read happens in**, and a test that
calls the function itself chooses the frame, so it **cannot fail in the way the bug failed**.

⛔ **My honest answer to the row's "if you believe a test IS possible":** a *partial* one is —
`bFocusReadbackPending` and the `GFrameCounter >` guard could be exercised by driving `NativeTick`
directly. ⛔ **I did not write it and I do not recommend it as-is**, because it would assert my guard's
arithmetic while saying nothing about the engine-loop ordering the guard exists to exploit — the
weaker half, dressed as the claim. **If anyone wants it, it is its own row and it does not ride this
one.** ⇒ **the instrument for this row is the runtime log, read at `TASK-1309`'s verify leg.**

---

## 9 · WHAT THIS ROW DID **NOT** DO — the (5) enumeration, each with its check

- ⛔ **Key table:** `20`/`7`, **byte-identical**, §7. `EKeys::Tab` = **0**. No letter, no digit.
- ⛔ **Focus behaviour:** `SetUserFocus` (`cpp:1520`), the deferred `GetSlateOperations().SetUserFocus`
  (`cpp:1537`), `EFocusCause::SetDirectly`, `CancelFocusRequest` (`cpp:1524`) and the return
  expression (`cpp:1613`) are **untouched**. ⭐ **Nothing I added calls any focus-setting API** —
  `LogPostFlushFocusReadback` only ever calls `GetUserFocusedWidget`, which is a **read**.
- ⛔ **`.uasset`:** none written. The one editor interaction was a **read-only** Python CDO query.
- ⛔ **`TASKBOARD.md`:** only my own row's `status:` line, by `Edit`, per the dispatch.
- ⛔ **`CONVENTIONS.md` / `settings.local.json` / `Saved/**` / the `.uproject`:** untouched.
- ⛔ **`DeckSlotEntryWidget.*` / `SiegeDeckSlotsTest.cpp`:** untouched.

---

## 10 · ⚠️ WHAT QA SHOULD SCRUTINISE — including the one place I read the contract against itself

1. 🚨 **THE ROW'S `names:` LINE SAYS THE `.h` IS "⛔ comment-only, rider (4)", BUT SPEC (3) MANDATES A
   MEMBER FLAG AND A `NativeDestruct`. THOSE CANNOT BOTH BE SATISFIED, AND I CHOSE THE SPEC.**
   My reading: the `names:` parenthetical **annotates rider (4)** (*"the `.h` edit that rider (4) makes
   is comment-only"*), it does not forbid the declarations spec (3)(i)/(ii) **requires** — a member
   flag and a `NativeDestruct` override are **undeclarable** without touching the header, and
   **Acceptance (6) scopes the phrase explicitly**: *"`(4)`'s rider declared as comment-only"*.
   ⛔ **I am flagging it rather than burying it.** The header's code-bearing additions are **exactly
   five lines**, all of them spec-(3) machinery, and they are listed individually in §1 so the gate can
   check the claim in one look:
   `virtual void NativeTick(...) override;` · `virtual void NativeDestruct() override;` ·
   `bool bFocusReadbackPending = false;` · `uint64 FocusRequestFrameCounter = 0;` ·
   `void LogPostFlushFocusReadback();`
   **Rider (4) itself contributes 0 of those 5.** If the gate reads the `names:` line strictly instead,
   this is a FAIL I will take and re-cut — but then spec (3)(i)/(ii) needs rewording, because no route
   satisfies both clauses.
2. **The frame-ordering argument in §3 is the load-bearing claim of the whole row** — it is the reason
   `NativeTick` is right and the row's own first suggestion is wrong. ⛔ Please re-read
   `LaunchEngineLoop.cpp:5859 / :5918 / :5921 / :5991 / :6131` yourself rather than accepting my table.
   **If (b) does NOT precede (c) and (d) in 5.8, my guard is wrong and the row fails.**
3. **`GFrameCounter` monotonicity.** `static_cast<int32>(GFrameCounter - FocusRequestFrameCounter)` is
   a printed diagnostic only, never a branch — the branch is the `>` at `cpp:1835`. A pathological
   wrap would misprint the delta and could not produce a false `MATCH`.
4. **`UPTRINT` pointer printing** — `static_cast<uint64>(reinterpret_cast<UPTRINT>(Ptr))` with
   `0x%016llX`. Four sites (`cpp:1607`, `:1609`, `:1909`, `:1911`). If the house style forbids raw
   addresses in a shipping log, say so and I will drop them; they are **context only** and the verdict
   does not depend on them.
5. **Format-specifier/argument parity** — I counted them by hand: PRE-FLUSH **7 specifiers / 7 args**,
   POST-FLUSH **7 / 7**, UNREADABLE **2 / 2**. ⛔ Worth a second pair of eyes; a mismatch is a runtime
   garbage read, not a compile error.
6. **`Super::` calls** — `NativeTick` calls `Super::NativeTick` **first** (`cpp:1822`), `NativeDestruct`
   calls `Super::NativeDestruct` **last** (`cpp:1856`). Deliberate: the disarm must run before the base
   class tears state down.
7. **The `UNREADABLE` third answer** (`cpp:1876-1882`) is new surface the row did not ask for. I judged
   a silent return there to be the same disease this row exists to cure. ⛔ Push back if you disagree.
8. ⛔ **Two files in `git status` are not mine** — §0. Please confirm the gate reads them as the other
   lanes' and not as scope creep.

---

## 11 · FOR `TASK-1309` (the host), in one block

- **Compile:** C++ only. ⛔ **The editor must be CLOSED for the compile; it is currently UP as PID
  1812** — and the diff **adds new `UFUNCTION`-free but new *virtual overrides* and new members**, so
  ⛔ **Live Coding cannot carry it.** Graceful-quit lane, then relaunch on the new binaries.
- **Suite:** ⛔ expected delta **0**; baseline **561**; reconcile **by name** (`SC-§104`).
- **Verify leg:** pin **`AcquireBuilderFocus POST-FLUSH read-back`**; the pass token is
  **`POST-FLUSH read-back: IDENTITY=MATCH`**. ⛔ Read §5's failure-mode list before verdicting — in
  particular, **a PRE-FLUSH-only log is NOT a pass**, and **`PRE-FLUSH … IDENTITY=NO-MATCH` is
  EXPECTED**.
- **Commit:** ⛔ **BY PATHSPEC** — `DeckBuilderWidget.cpp`, `DeckBuilderWidget.h`, this handoff,
  `qa/TASK-1308-report.md`, `TASKBOARD.md`. ⛔ **Expect ZERO `.uasset`**, and note that
  `BP_MenuGameMode.uasset` + `Tools/run_suite_bounded.ps1` are **other rows' work sitting in the same
  tree** (§0) — ⛔ do not sweep them in.
- 🙋 **For the manager:** `DECK-§9`'s Site column needs a **+54** amendment on eleven cells (§7a), and
  the census it asserts is **unchanged at 20/7**.
