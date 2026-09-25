# Handoff — TASK-1400 — [MENU-REENTRY-FOCUS]

**Agent:** gameplay-programmer · **Date:** 2026-09-24 · **Marker:** `TASK-1400-MENU-REENTRY-FOCUS`
**Status →** `ready-for-qa` · **Gate:** `TASK-1451` (⛔ **not** `TASK-1401` — that one was consumed on `TASK-1394`)

---

## 0. 🚨 DECLARATIONS OWED AT THE TOP (the row's (6) and (4)-rider)

**(6) `.uasset` TWIN — ⛔ NONE. THE FIX IS 100 % C++.** No Blueprint was opened, edited, compiled or
saved. **This row carries NO 🧑 human `Compile` + `Ctrl+S` keystroke cost.** The measurement put the
fix inside `SiegeMenuInputSubsystem.{cpp,h}`, which is exactly this row's existing file grant.

**⚠️ HEADER CHANGE — DECLARED LOUDLY (the (5) fence).** `SiegeMenuInputSubsystem.h` **was** touched.
⛔ **No new method was added** — the poll calls the *existing* `ApplyInitialFocus()`. The header
carries only what a looping timer cannot do without: an `#include "Engine/TimerHandle.h"`, one
`static constexpr float` interval, one `FTimerHandle` member, and an updated doc comment on
`ApplyInitialFocus()` (it said *"fired once"*, which this row makes false — see §5).

**⚠️ SHIPPING LIMIT ON THE (4) INSTRUMENT — the reader's precondition, not a code defect.**
`UE_LOG` at `Log` verbosity is **compiled out entirely** under **Shipping** (`USE_LOGGING_IN_SHIPPING`
= 0 ⇒ `NO_LOGGING` = 1; this project sets **no** `bUseLoggingInShipping` override — documented in-tree
at `SiegeAssistantGrammar.cpp:226-270`). ⇒ **In a packaged build my line does not exist.** The
precedent Accept line shares the property, so this is not a reason to change it. **`TASK-1402` must
declare the session's build configuration before reading an absence.**

**⛔ SCOPE — STATE LIMB ONLY. I did not chase the ring.** `TASK-1399` §6 measured focus moving with no
visible outline; `TASK-1446` answered β; `TASK-1450` is measuring it in PIE. **A ringless verifier
capture is not a failure of this row.** I did not widen the file grant to go looking for it, and I did
not adopt the refuted H1.

**Fences honoured:** ⛔ no compile · ⛔ no PIE · ⛔ no MCP · ⛔ no asset · ⛔ no git · ⛔ no editor
lifecycle action · ⛔ `Tests/SiegeMenuInputTest.cpp` untouched · ⛔ `CONVENTIONS.md` untouched · ⛔ only
this row's `status:` line on the board.
**Capability note (`SC-§71b`):** my grant is `Read`/`Grep`/`Edit` **only**, so ⛔ **I did not run
`git diff`.** Every shape claim below is a **`Read`/`Grep` reconciliation** (line-count arithmetic +
text census), and §6 states exactly what that does **not** entitle me to say.

---

## 1. ⭐ THE (1) MEASUREMENT — TAKEN AND WRITTEN **BEFORE** THE DIFF

> The row: *"MEASURE BEFORE YOU FIX. THE FIX LOCATION IS NOT GIVEN TO YOU."* The boarded hypothesis:
> *"closing the deck builder makes `IsMenuUncovered()` true again, but nothing re-fires
> `ApplyInitialFocus`."*

### 🚨 VERDICT: **CONFIRMED** — on all four limbs, three of them measured, one honestly incomplete.

**Limb A — "nothing re-fires `ApplyInitialFocus`". ✅ CONFIRMED, exhaustively.**
Repo-wide grep for `ApplyInitialFocus` over `--include=*.cpp --include=*.h` returns **exactly 5 hits**:

| hit | what it is |
|---|---|
| `SiegeMenuInputSubsystem.h:192` (pre-edit) | the declaration |
| `SiegeMenuInputSubsystem.cpp:382` (pre-edit) | the definition |
| `SiegeMenuInputSubsystem.cpp:116` (pre-edit) | ⭐ **the ONE and ONLY call** — `SetTimerForNextTick`, inside `OnWorldBeginPlay` |
| `DeckBuilderWidget.cpp:665` | a **comment** (*"is NOT used and NOT edited"*) |
| `DeckBuilderWidget.h:945` | a **comment** |

⇒ **One caller, and it is reachable only at level boot. No re-arm site exists anywhere in `Source/`.**

**Limb B — "`IsMenuUncovered()` becomes true again on close". ✅ CONFIRMED by source trace, with
runtime corroboration for the *covered* half.** `IsMenuUncovered()` (`.cpp:170-199`) returns false iff a
**visible top-level** widget whose class path ≠ `MainMenuWidgetClassPath` exists. `GetAllWidgetsOfClass(…,
TopLevelOnly = true)` filters on `IsInViewport()`, so a `RemoveFromParent`'d panel drops out of the set
the same frame ⇒ the gate re-opens with no help from anyone. **Runtime corroboration for the covered
state:** `qa/TASK-1274-verify.md:9`'s viewport census read `in_viewport=2` with `WBP_MainMenu_C_0 |
visible=True` **plus** `SettingsMenuWidget_0 | visible=True` while Settings was up; and `TASK-1399` §3
found **zero** `LogSiegeMenuInput` lines for any injection made while a sub-screen was open.

**Limb C — `OnWorldBeginPlay` does not re-run on a return. ✅ CONFIRMED (source).** A return to the menu
is a **viewport widget swap on the same world**, not a level load, so nothing re-enters the subsystem's
only arming site. ⚠️ **Honest limit:** `TASK-1399` §3's six `IMC_MainMenu applied` lines are one per PIE
*session*, and that run **closed no screen at all** (§4), so the log corroborates "once per level load"
but **cannot** corroborate "not re-fired on a return". That limb is a source fact only.

**Limb D — focus is genuinely *lost*, not merely un-refreshed. ✅ CONFIRMED, and this is the limb the
hypothesis did not state.** The builder **takes** Slate focus off the menu: `AcquireBuilderFocus()`
(`DeckBuilderWidget.cpp:1472`, called from `NativeConstruct` `:670`) and `FocusCardTile` `:1124`
(`EFocusCause::Navigation`); `TASK-1399` row 2 read `WBP_DeckBuilder_C_0` root **`focused:true`** at
t=83.290. Its `NativeDestruct` (`:1846-1857`) makes **no focus call**, and its BP `Exit` graph has **no
focus node** (`TASK-1399` §5.3). ⇒ On return, **no menu button holds focus** — which is precisely 🧑 the
state he described as *"the outline is no longer there"*.

### ✅ `TASK-1398`'s branch (i) RE-MEASURED against the current file — **IT STILL HOLDS**

Re-read at my own instant (`SC-§138`), not inherited:
- `ApplyInitialFocus()` is **`private:`** (`.h:192` pre-edit) and **not** a `UFUNCTION` — ⇒ the re-arm
  must live inside `SiegeMenuInputSubsystem.{cpp,h}` and nowhere else. ✅ still true.
- It **resolves every input live** and **caches nothing**. ✅ still true.
- It is **idempotent**: `.cpp:391` (pre-edit) guards `Buttons.Num() > 0 && !GetFocusedMenuButton()`. ✅
  still true, and ⛔ **I left that guard byte-identical** (see §4).
⇒ **A single shared re-arm is reachable and covers every sub-screen. Branch (i) confirmed.**

### 🚨 THE THING THE MEASUREMENT ADDED THAT THE BOARD DID NOT HAVE — and it is for the manager

**Two `WBP_MainMenu` instances can plausibly be in the viewport at once after a deck-builder Exit.**
`TASK-1398`'s census says the session menu is **"the ONE screen that REMOVES the main menu"** rather than
covering it (row 6) — ⇒ the deck builder **covers** it. `qa/TASK-1274-verify.md:9` measured exactly that
shape at runtime for Settings (`in_viewport=2`, the menu still `visible=True` underneath). But the deck
builder's Exit **creates a new** `WBP_MainMenu` rather than re-showing the one already there.
⇒ **old + new = two live instances.**

⛔ **I did NOT fix this and I am NOT claiming it happens.** `WBP_MainMenu`'s EventGraph is still
**BP-opaque** (`TASK-1398` §176: *"the worst one"* — the 7 buttons' `OnClicked` handlers are unreadable),
so **nobody has measured whether the Deck Builder button removes the menu on the way in.** This is
flagged, not adjudicated (`SC-§101`).

**Why it does not change my fix:** `IsMenuUncovered()` sets `bMenuPresent = true` for *either* instance
and returns **true**; `ApplyInitialFocus()` then focuses `Buttons[0]` of whichever instance
`FindMainMenuWidget()` resolves. **The state limb succeeds in both the one-instance and the
two-instance case.** ⚠️ **Where it could matter is the PIXEL limb, which is not mine:**
`FindMainMenuWidget()` returns the **first** `TObjectIterator` match and that order is not defined, so in
the two-instance case the ring could be painted on the instance *underneath*. ⛔ **I deliberately did not
"fix" `FindMainMenuWidget()`'s selection** — it is shared with `HandleMenuUp`/`Down`/`Accept`, so changing
it is a behaviour change well beyond this row (`SC-§50`). **Manager: this is worth a row.**

---

## 2. THE (3) BRANCH, NAMED BY LETTER

## ⭐ **BRANCH (i)** — a single shared re-arm trigger, implemented **once**, covering **every** sub-screen.

⛔ Not (ii). I did **not** do the deck builder only, and I did **not** silently do all four close paths —
**I touched no close path at all.** The re-arm is one timer in one file and it is blind to *which* screen
the player came back from, which is exactly why it covers screens nobody has enumerated yet.

---

## 3. 🚨 HOW THE FIX SURVIVES A FRESH WIDGET — the `TASK-1451` (3) question, answered explicitly

**The mechanism: a low-rate looping poll on the world timer manager, armed only on `L_MainMenu`, that
calls the existing `ApplyInitialFocus()`.**

**Why a poll and not an event — this is a measurement, not a preference.** ⛔ **Both return paths emit
nothing to subscribe to:**
- deck-builder `Exit` = pure Blueprint `CreateWidget(WBP_MainMenu_C) → Is Valid → AddToViewport(ZOrder 0)
  → RemoveFromParent(self)`, **no focus node, no dispatcher** (`TASK-1399` §5.3);
- `USessionMenuWidget::BackPressed` (`SessionMenuWidget.cpp:151-165`) — **the same shape** in C++, and in
  a file this row may not touch.

⭐ **And the obvious alternative is *provably* insufficient, not merely unattractive:** a Slate
`OnFocusChanging` hook only fires if focus was inside the removed subtree — but `TASK-1399`'s table read
**all nodes `focused:false`** on Settings (row 3, 9 nodes), Login (row 5, 24 nodes) and Session (row 6, 13
nodes). ⇒ **closing those three changes no focus and would fire no event.** A tickable subsystem is this
same poll at 60+ Hz. **A low-rate look is the only mechanism that covers a transition that announces
itself to nobody.**

**⭐ WHY IT SURVIVES WIDGET REPLACEMENT — it holds no widget, so there is nothing to go stale.**
`ApplyInitialFocus()` takes **no argument** and stores **nothing**. Each of its three inputs re-resolves
from the live viewport on **every** call:

| input | how it resolves | why a fresh widget is fine |
|---|---|---|
| `IsMenuUncovered()` | `GetAllWidgetsOfClass(…, TopLevelOnly)` → `IsInViewport()` filter | a removed panel leaves the set; a new menu enters it |
| `GetMenuButtons()` | → `FindMainMenuWidget()`, matched on the **class path string** `MainMenuWidgetClassPath` | matches **any** instance of `WBP_MainMenu_C`, including one created 3 ms ago |
| `GetFocusedMenuButton()` | walks those same live buttons, `HasUserFocus(PC)` | never reads a cached index |

⇒ **Pointer-free by construction.** The subsystem's only persistent state is the mapping context, the
three `UInputAction`s and `bArmed` — **none of them is widget-related.** A re-arm built on a remembered
pointer would pass every static read and work nowhere; this one cannot, because there is no pointer.

**⛔ AND IT CANNOT STEAL FOCUS (the `TASK-1451` (2) regression surface):** the `!GetFocusedMenuButton()`
guard is **unmodified**, so a repeat is a **no-op** whenever a menu button already holds focus; and
`IsMenuUncovered()` is **false for the whole time any sub-screen is up**. The only state the poll ever
acts in is *"menu visible, uncovered, nothing on it focused"*. ⚠️ Keeping that guard is also what keeps us
out of `TASK-1446`'s trap — `SetUserFocus` early-returns `false` when the target is **already** focused
(`SlateApplication.cpp:3028-3033`) — and ⛔ **I read `FocusButton`'s return value nowhere.**

**Rate:** `FocusReentryPollSeconds = 0.2f` (5 Hz). Armed **after** the map gate ⇒ the timer **does not
exist on `L_Arena`**. Cleared in `Deinitialize` — ⚠️ **not belt-and-braces**: `UWorld::GetTimerManager()`
resolves to the **owning game instance's** manager, which **outlives** the world, so an uncleared loop
would survive the travel off the menu map.

---

## 4. THE DIFF — BEFORE / AFTER, VERBATIM

### 4.1 `SiegeMenuInputSubsystem.cpp` — **3 insertions, ⛔ ZERO deletions, ZERO modifications**

**(a) `OnWorldBeginPlay` — the trigger.** Inserted **after** the untouched `SetTimerForNextTick` line and
**before** the untouched `IMC_MainMenu applied` log.

*Before:*
```cpp
	InWorld.GetTimerManager().SetTimerForNextTick(this, &USiegeMenuInputSubsystem::ApplyInitialFocus);

	UE_LOG(LogSiegeMenuInput, Log,
```
*After* (comment body elided; the code added is exactly these three lines):
```cpp
	InWorld.GetTimerManager().SetTimerForNextTick(this, &USiegeMenuInputSubsystem::ApplyInitialFocus);

	// ─── TASK-1400 (MENU-REENTRY-FOCUS): THE RE-ENTRY RE-ARM ──── [29 comment lines] ────
	InWorld.GetTimerManager().SetTimer(
		FocusReentryPollTimerHandle, this, &USiegeMenuInputSubsystem::ApplyInitialFocus,
		FocusReentryPollSeconds, /*bLoop=*/ true);

	UE_LOG(LogSiegeMenuInput, Log,
```

**(b) `Deinitialize` — the teardown.** Inserted **before** the untouched `bArmed = false;`:
```cpp
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FocusReentryPollTimerHandle);
	}
```

**(c) `ApplyInitialFocus` — the (4) log line.** ⛔ **The guard line is byte-identical and in place.**

*Before:*
```cpp
	TArray<UButton*> Buttons;
	GetMenuButtons(Buttons);
	if (Buttons.Num() > 0 && !GetFocusedMenuButton())
	{
		FocusButton(Buttons[0]);
	}
```
*After* (comment body elided):
```cpp
	TArray<UButton*> Buttons;
	GetMenuButtons(Buttons);
	if (Buttons.Num() > 0 && !GetFocusedMenuButton())
	{
		// ─── TASK-1400 deliverable (4) ──── [25 comment lines] ────
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] ApplyInitialFocus: focus placed on the TOP option '%s' (\"%s\"), index 0 of %d, in menu instance '%s'."),
			*Buttons[0]->GetName(), *GetButtonLabel(Buttons[0]), Buttons.Num(), *GetNameSafe(FindMainMenuWidget()));

		FocusButton(Buttons[0]);
	}
```

### 4.2 `SiegeMenuInputSubsystem.h` — 3 insertions + **1 modification** (declared)

1. `#include "Engine/TimerHandle.h"` + 3 comment lines. Precedent: `SiegeKeyboardLayoutSubsystem.h:13-14`
   carries the identical note (`FTimerHandle` is **not** in `CoreMinimal.h` and **not** in `EngineTypes.h`).
2. `static constexpr float FocusReentryPollSeconds = 0.2f;` + its rationale block, placed beside the
   existing `MenuMappingContextPriority` (same public-tunable idiom).
3. `FTimerHandle FocusReentryPollTimerHandle;` + its declaration block, above `bool bArmed`.
4. 🚨 **THE ONE NON-INSERTION IN THE WHOLE CHANGE, DECLARED:** `ApplyInitialFocus()`'s doc comment.
   *Before:* `/** Next-tick after BeginPlay: place the initial (visible) focus on the first button. */`
   *After:* an 11-line block naming **both** callers. **Why I changed it rather than leaving it:** it said
   *"next-tick after BeginPlay"* and `TASK-1398` §75 quoted it as *"fired **once**"*. This row makes that
   **false**, and a comment that asks to be re-read while stating a falsehood is the exact fail-silent
   `TASK-1398` §155 flagged in another file. ⛔ **No code line was reworded, renumbered or moved.**

### 4.3 Shape, reconciled without `git`

| file | before | after | net | deletions |
|---|---|---|---|---|
| `SiegeMenuInputSubsystem.cpp` | 428 | **503** | **+75** | ⛔ **0** |
| `SiegeMenuInputSubsystem.h` | 220 | **270** | **+50** | **1** (the doc-comment line in 4.2 #4) |

`.cpp` arithmetic: 33 (a) + 13 (b) + 29 (c) = **75** ✓. `.h` arithmetic: 4 + 22 + 11 + 14 = 51 added, 1
removed = **+50** ✓. Both reconcile exactly, so **an unnoticed deletion anywhere would break the sum.**

---

## 5. THE FENCE CHECKS, RE-READ AT MY OWN INSTANT

### ✅ ALL FOUR `TASK-1394` INSTRUMENT LINES SURVIVE — and the entry line is **STILL THE FIRST STATEMENT**

`grep -n "UE_LOG(LogSiegeMenuInput"` ⇒ **10 sites** = 5 pre-`TASK-1394` + **4** `TASK-1394` + **1** mine.
(`TASK-1401` §B censused **9** before my diff; 9 + 1 = 10 ✓.)

| # | line | text (grepped verbatim) | still first-in-its-block? |
|---|---|---|---|
| 1 | `.cpp:353-354` | `IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1).` | ⭐ **YES — FIRST STATEMENT of `HandleMenuDown()`** |
| 2 | `.cpp:366-367` | `MoveFocus(%+d) declined: menu covered.` | ✅ yes |
| 3 | `.cpp:376-377` | `MoveFocus(%+d) declined: no menu buttons.` | ✅ yes |
| 4 | `.cpp:397-398` | `MoveFocus(%+d): focus moved %d -> %d of %d ('%s').` | ✅ yes |

🚨 **THE PROPERTY THAT MATTERS, RE-READ LINE BY LINE:** `HandleMenuDown()` opens at `.cpp:346`; `{` at
`:347`; `:348-352` is the instrument's own comment; **`:353` is the `UE_LOG`**; `:356` is `MoveFocus(+1)`.
⛔ **I inserted NOTHING above it** — `HandleMenuDown` and `MoveFocus` are **completely untouched** by this
row. Their +47-line shift is entirely from insertions (a) and (b), which sit in *earlier functions*.
⇒ **`TASK-1395` and `TASK-1402` read an instrument this diff did not disturb.**

### ✅ THE ACCEPT LINE — BYTE-IDENTICAL, AND ITS **POSITION** IS INTACT

Re-quoted from `.cpp:419-421` (cited by text, never by number — `CITE-BY-TEXT-RULED-2026-09-24`):
```cpp
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IA_MenuAccept -> OnClicked.Broadcast() on '%s' (\"%s\")."),
		*Focused->GetName(), *GetButtonLabel(Focused));
```
⭐ **Position re-checked, not just the bytes** (the `TASK-1451` (5) requirement): it sits **after both
guards** — `if (!IsMenuUncovered())` at `:406` and `if (!Focused)` at `:412` — and **before**
`Focused->OnClicked.Broadcast()` at `:426`. That is the positional property `TASK-1393`'s
fourth-explanation refutation rests on, and it is unchanged.

### ✅ `Tests/SiegeMenuInputTest.cpp` — UNEDITED, and its **premise survives**

Zero edits (it is outside my grant). I traced its two focus assertions against the new behaviour:
- `:142` *"cold precondition: no menu button focused, or index 0"* — the poll **only ever** focuses index
  0, so it can only ever *satisfy* this, never break it.
- `:166-168` after `IA_MenuDown` ×2 it expects the focused button to read **"Deck Builder"** (index 2).
  Once index 2 holds focus, `!GetFocusedMenuButton()` is **false** ⇒ the poll is a **no-op** and cannot
  drag focus back to 0. ⛔ **The poll cannot fight the test.**

---

## 6. `## Not examined / limitations`

1. ⛔ **NOT COMPILED, NOT RUN, NOT PIE-TESTED.** The row forbids it and `TASK-1450` held the editor. Every
   claim here is **static**. `Result: Succeeded` is 5a's; runtime behaviour is `TASK-1402`'s.
2. ⛔ **I could not run `git diff`** (grant = `Read`/`Grep`/`Edit`). §4.3 is a **line-count + text
   reconciliation**, and it is weaker than a byte diff: it proves the **sums and the landmarks** agree,
   ⛔ **it does not entitle me to say "no byte inside an unexamined line changed."** A reviewer with
   `git` should take the real diff.
3. ⛔ **The post-close state has never been observed at runtime by anyone.** `TASK-1399` §4: **no
   sub-screen could be closed** by any instrument available to that lane. Limbs B and D of §1 are source
   traces with *adjacent* runtime corroboration — ⛔ **not a direct observation of the moment of return.**
   **That is precisely what `TASK-1402` is for**, and the (4) log line exists so that 🧑 his own sitting
   can supply it too.
4. ⛔ **The duplicate-instance question in §1 is UNRESOLVED, not answered.** `WBP_MainMenu`'s EventGraph
   remains BP-opaque; whether the Deck Builder button removes the menu on the way in is **unmeasured**.
   ⛔ **Manager's, not mine.**
5. ⚠️ **0.2 s is a chosen number, not a measured one.** No latency or cost measurement was taken (no
   engine access). Worst-case ring latency after a return is one poll period.
6. ⚠️ **The log line can repeat** at the poll rate **if and only if** the focus request never takes. I
   argue in-code that this is **signal, not spam** (a successful placement silences the next poll), but
   ⛔ **that argument is untested** — if `TASK-1402` sees a flood, the flood **is** the finding.
7. ⛔ **THE PIXEL LIMB IS NOT MINE AND I MAKE NO CLAIM ABOUT IT.** This fix can succeed completely and a
   verifier capture can still show **no ring** (`TASK-1399` §6 + 🧑 his photograph: the two lanes diverge).
   ⛔ **That is not a failed re-arm and must not bounce this row.**
8. ⚠️ **This row is ONE row inside the epic** — 🧑 he approved **41 rows for all ten screens**. ⛔ **It is
   not the answer to his ask and nobody should report it as one.**
