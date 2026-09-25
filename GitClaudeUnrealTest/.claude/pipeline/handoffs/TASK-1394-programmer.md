# TASK-1394 — [MENU-DOWN-LOG-INSTRUMENT] — gameplay-programmer handoff

**Marker:** `TASK-1394-MENU-DOWN-LOG-INSTRUMENT`
**Date:** 2026-09-24
**Status on completion:** `ready-for-qa`
**Blocker cleared before start:** `TASK-1392` committed as `c5e8d97`.

---

## 0. One-line result

Four `UE_LOG` lines added to **one file** — one unconditional entry line as the **first statement**
of `HandleMenuDown()`, and one line at **each of `MoveFocus`'s three exits**, every one of them
carrying `Delta`. **Pure insertion: 23 lines added, 0 deleted, 0 modified.** Control flow is
unchanged.

---

## 1. 🚨 SPEC ARITHMETIC CONFLICT — DECLARED, NOT SILENTLY RESOLVED

The row's heading and (2)'s preamble both say **"EXACTLY THREE `UE_LOG` LINES, NO MORE"**. Its own
enumeration asks for more than three:

- **(2)(a)** = **1** line (entry line in `HandleMenuDown()`)
- **(2)(b)** = **"ONE LINE PER EXIT OF `MoveFocus(int32 Delta)`"**, and **(1)** states — as a
  measurement taken at boarding — that `MoveFocus` has **THREE EXITS**, naming all three reason
  strings (*covered* · *no menu buttons* · *focus moved `<from>` → `<to>` of `<N>``*) = **3** lines

**1 + 3 = 4.** Three is arithmetically unreachable without dropping one of the row's own explicit
sub-requirements.

**I resolved toward the enumeration, and delivered 4.** The reasons, in order of weight:

1. **The consumer row demands all four.** `TASK-1395` (3) pre-writes its three outcomes against
   both kinds of line at once: **(a)** needs the *entry line*; **(b)** needs the *entry line absent*;
   **(c)** needs the *entry line present* **AND** *"`TASK-1394` (b)'s REASON LINE says **covered** or
   **no menu buttons**"*. Dropping either the entry line or a reason line makes at least one of
   `TASK-1395`'s pre-written outcomes unreadable.
2. **Dropping the entry line is explicitly forbidden** by (2)(a) — *"the 'first statement' property
   is THE WHOLE POINT"*.
3. **Dropping an exit line reproduces the exact defect this row exists to remove** — a reader could
   not tell "did not run" from "ran and declined".

⚠️ **This is flagged for QA and the manager, not self-adjudicated into law.** Five downstream places
(`TASK-1400` (5) + (2), `TASK-1406` (6), `TASK-1447` (5), `TASK-1448` (2)) fence *"`TASK-1394`'s **three** new
lines"*. **There are four.** Nobody should read that phrase as a count they can check against — it
should be read as *"the `TASK-1394` instrument lines"*. I do not edit those rows (`SC-§101`).

---

## 2. THE FOUR NEW LINES, QUOTED IN FULL

All four are `UE_LOG(LogSiegeMenuInput, Log, …)`. File:
`Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp`.

### (a) THE ENTRY LINE — `HandleMenuDown()`, `.cpp:306-307`

```cpp
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1)."));
```

🚨 **THE "FIRST STATEMENT" PROPERTY, STATED EXPLICITLY:** this `UE_LOG` is the **first statement in
the function body**. Nothing precedes it — no guard, no branch, no local, no early return, no call.
`HandleMenuDown()` in full, after the edit:

```cpp
void USiegeMenuInputSubsystem::HandleMenuDown()
{
	// TASK-1394 instrument. FIRST STATEMENT, unconditional, before MoveFocus is called: this is
	// the line that makes a SILENCE readable. Placed after any branch it would print nothing in
	// exactly the cases a reader must tell apart -- "the handler never ran" (the action did not
	// route here at all) vs "it ran and declined" (MoveFocus took an early exit). Same category
	// and verbosity as the already-proven-live IA_MenuAccept line below.
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1)."));

	MoveFocus(+1);
}
```

⇒ **If this line is absent from a log in which the arming line is present, `HandleMenuDown` did not
run.** That is the single property the whole row exists to buy, and it holds only because the line
is unconditional and first.

### (b1) EXIT 1 — the `!IsMenuUncovered()` early return, `.cpp:319-320`

```cpp
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] MoveFocus(%+d) declined: menu covered."), Delta);
```

### (b2) EXIT 2 — the `Buttons.Num() == 0` early return, `.cpp:329-330`

```cpp
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] MoveFocus(%+d) declined: no menu buttons."), Delta);
```

### (b3) EXIT 3 — the `FocusButton(Buttons[Next])` success path, `.cpp:350-352`

```cpp
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] MoveFocus(%+d): focus moved %d -> %d of %d ('%s')."),
			Delta, Current, Next, Buttons.Num(), *Buttons[Next]->GetName());
```

**`Delta` on every one of the three**, per (2)(b): `MoveFocus` is shared with `HandleMenuUp()`
(`.cpp:294-297`, unedited), so a line without `Delta` cannot tell Up from Down and is not an
instrument. `%+d` renders it `+1` / `-1`, signed, at a glance.

**Two deliberate extras in (b3), both justified rather than assumed:**

- **`('%s')` — the chosen button's `GetName()`.** The precedent line (`IA_MenuAccept`) carries
  `*Focused->GetName()` and `TASK-1399` read 12 of those against `ui_snapshot` and found them to
  agree. Naming the button is what makes that same cross-check possible for `Down`. `Buttons[Next]`
  cannot be null — `GetMenuButtons` only appends a non-null `UButton*` (`.cpp:215`).
- **`of %d` = `Buttons.Num()`**, exactly as (2)(b)'s reason string specifies (`of <N>`).

---

## 3. ⚠️ THE `SetUserFocus` TRAP — ENCODED *AGAINST*, NOT INTO, THE INSTRUMENT

`TASK-1446` measured that `FSlateApplication::SetUserFocus` **early-returns `false` when the target
is ALREADY focused** (`SlateApplication.cpp:3028-3033`) ⇒ **`FocusButton()` returning `false` is NOT
a failure.**

⇒ **(b3) does not consult `FocusButton`'s return value at all, and there is no fourth line reporting
a "focus failed" / "focus refused" case.** The line is emitted **before** the `FocusButton` call and
reports **the index `MoveFocus` chose**, which is a fact about `MoveFocus`'s own decision and cannot
be falsified by what Slate does with it. The reasoning is recorded in a comment at the call site
(`.cpp:346-349`) so it survives `ApplyInitialFocus`'s `!GetFocusedMenuButton()` guard being removed
later (that guard is `TASK-1400`'s territory, untouched here).

---

## 4. CATEGORY AND VERBOSITY — `LogSiegeMenuInput, Log`, WITH THE REASON

All four lines use **`UE_LOG(LogSiegeMenuInput, Log, …)`** — the **same category and the same
verbosity** as the already-proven-live Accept line at `.cpp:372-374`.

- **Category `LogSiegeMenuInput`** is declared `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeMenuInput, Log,
  All)` (`SiegeMenuInputSubsystem.h:18`, read, not edited) and defined at `.cpp:22`. Compile-time
  ceiling `All`, default runtime verbosity `Log` ⇒ **these lines print with no verbosity step.**
- **Never `Verbose`**, per (3): a `Verbose` line needs a `Log LogSiegeMenuInput Verbose` step that
  nobody will remember, and **an empty log reads as a zero** — the exact false pass this project has
  already paid for once. `Log` removes that failure mode from `TASK-1395` entirely.
- The category is **measured not suppressed**: the row states there is no `[Core.Log]` override for
  it in `Config/`, and the live-evidence corroboration is that `TASK-1393` and `TASK-1399` both read
  `LogSiegeMenuInput` `Log`-verbosity lines (the arming line and 12 Accept lines) straight out of
  `GitClaudeUnrealTest.log` with no verbosity step taken.

---

## 5. ⛔ ZERO BEHAVIOUR CHANGE — THE ARGUMENT, PER BRANCH

**Shape of the diff: 23 insertions, 0 deletions, 0 modifications.** `git diff --stat` reports
`1 file changed, 23 insertions(+)` with **no `-` lines at all**. Every pre-existing statement,
condition, brace and blank line in the file is byte-for-byte what it was at `c5e8d97`. Nothing was
reordered, renamed, re-signed or re-scoped.

Breakdown of the 23 inserted lines, by insertion site:

| Site | Comment lines | `UE_LOG` physical lines | Blank | Total |
|---|---|---|---|---|
| (a) `HandleMenuDown` entry | 5 | 2 | 1 | 8 |
| (b1) `MoveFocus` exit 1 | 3 | 2 | 0 | 5 |
| (b2) `MoveFocus` exit 2 | 1 | 2 | 0 | 3 |
| (b3) `MoveFocus` exit 3 | 4 | 3 | 0 | 7 |
| **Total** | **13** | **9** | **1** | **23** |

⇒ **9 lines of executable code, all of it `UE_LOG`.** The other 14 are comments and one blank.

**Per branch:**

| Branch | Before | After | Behaviour argument |
|---|---|---|---|
| `HandleMenuDown()` | `{ MoveFocus(+1); }` | `{ UE_LOG(...); MoveFocus(+1); }` | A `UE_LOG` with no side effect on any program value. `MoveFocus(+1)` is still reached on every call, with the same argument, and it is still the last statement. No return value existed to change (`void`). |
| `MoveFocus` exit 1 — `!IsMenuUncovered()` | `{ return; }` | `{ UE_LOG(...); return; }` | The **condition is untouched**; the `return` is untouched and still the last statement in the block. The branch is taken in exactly the same cases as before and does exactly the same nothing. |
| `MoveFocus` exit 2 — `Buttons.Num() == 0` | `{ return; }` | `{ UE_LOG(...); return; }` | Same argument. `Buttons` is read (`.Num()` already read it) but never mutated by the log. |
| `MoveFocus` exit 3 — `Buttons.IsValidIndex(Next)` | `{ FocusButton(Buttons[Next]); }` | `{ UE_LOG(...); FocusButton(Buttons[Next]); }` | The log reads `Delta`, `Current`, `Next`, `Buttons.Num()` and `Buttons[Next]->GetName()` — **all five are reads of values the function had already computed**; none is assigned, and `GetName()` is `const`. `FocusButton(Buttons[Next])` runs unconditionally after it, on the same argument, as before. |

**What was NOT done, each explicitly:**

- ⛔ **No branch added, removed or reordered.** The `if`/`return` skeleton of `MoveFocus` is
  identical; `git diff` shows zero changed lines to prove it.
- ⛔ **No early-return condition touched.** `!IsMenuUncovered()`, `Buttons.Num() == 0` and
  `Buttons.IsValidIndex(Next)` are byte-identical.
- ⛔ **No signature changed.** `HandleMenuDown()`, `HandleMenuUp()`, `MoveFocus(int32 Delta)`,
  `FocusButton`, `IsMenuUncovered`, `GetMenuButtons`, `WrapIndex` — all unchanged.
- ⛔ **No new `UCLASS` / `UPROPERTY` / `UFUNCTION`**, no new member, no new local variable, no new
  `#include`. `LogSiegeMenuInput`, `UE_LOG` and `TEXT` were all already used in this translation unit.
- ⛔ **The header was NOT edited.** ✅ **No declaration was required** — the row asked me to say so
  if one were; none was. `SiegeMenuInputSubsystem.h` is untouched and shows clean in `git status`.
- ⛔ **`HandleMenuUp()` was NOT touched** (`.cpp:294-297`, still `{ MoveFocus(-1); }`). The row's
  no-scope-creep fence holds; `Delta` on (b1)/(b2)/(b3) covers Up without an edit there.
- ⛔ **`Tests/SiegeMenuInputTest.cpp` was NOT edited** and cannot be affected: grepped for
  `MoveFocus`, `HandleMenuDown` and `UE_LOG` — **zero hits**. No test asserts on log text or on any
  line number in this file.
- ⛔ No compile, no PIE, no MCP, no asset, no git, no editor lifecycle action, no `CONVENTIONS.md`,
  no other row's line.

---

## 6. THE ACCEPT LINE — RE-QUOTED, BYTE-IDENTICAL

Byte-compared against `HEAD` (`c5e8d97`) with `diff`: **IDENTICAL**, zero differences.

```cpp
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IA_MenuAccept -> OnClicked.Broadcast() on '%s' (\"%s\")."),
		*Focused->GetName(), *GetButtonLabel(Focused));
```

Not reworded, not renumbered within itself, not relocated within the function, not re-indented. It
still sits after both guards and immediately before `Focused->OnClicked.Broadcast()` — the property
`TASK-1393`'s fourth-explanation refutation rests on.

### 🚨 BUT ITS **LINE NUMBER** MOVED: `.cpp:349-351` → **`.cpp:372-374`**

**Unavoidable, and I am flagging it rather than hiding it.** Both functions the row required me to
instrument (`HandleMenuDown`, `MoveFocus`) sit **above** `HandleMenuAccept` in the file, so *any*
insertion this row mandates shifts the Accept line down. 23 inserted lines ⇒ **+23**.

- **The bytes are unchanged** (proven above), which is what `TASK-1400` (5) + (2), `TASK-1406` (6),
  `TASK-1447` (5) and `TASK-1448` (2) actually protect — *"changing it breaks a citation"*.
- **The citation `.cpp:349-351` is now stale in six places**: `TASKBOARD.md:6209` (`TASK-1400`),
  `:6223` (`TASK-1400`), `:6311` (`TASK-1406`), `:6942` (`TASK-1447`), `:6955` (`TASK-1448`), plus
  `qa/TASK-1393-verify.md:29`. The correct coordinates are now **`.cpp:372-374`** (and the broadcast,
  formerly `.cpp:356`, is now **`.cpp:379`**).
- ⛔ **I did not amend any of them** — they are other rows' lines and another agent's report
  (`SC-§101`). **Manager call.** Anyone verifying the Accept line should grep the text, not the line
  number.

---

## 7. THE FOURTH, STRUCTURALLY UNREACHABLE PATH — DECLARED, DELIBERATELY SILENT

`MoveFocus` has one more syntactic exit than the three the row names: **falling out of the bottom
when `Buttons.IsValidIndex(Next)` is false.** I left it silent, on purpose, and here is the proof it
is dead code rather than an un-instrumented exit:

- Control only reaches that `if` when `Buttons.Num() >= 1` (exit 2 returned otherwise).
- `WrapIndex(Current, Delta, Count)` with `Count > 0` returns `((Current + Delta) % Count + Count) % Count`,
  which is in `[0, Count)` **for every integer `Current` and `Delta`** — including the
  `Current == INDEX_NONE` case `IndexOfByKey` could in principle produce.
- ⇒ `Next` is always a valid index ⇒ the `if` is always taken ⇒ the fall-through never executes.

Adding a fifth log line for a branch that cannot run would add noise and widen a diff the row
requires to stay trivially readable. **Every *reachable* exit of `MoveFocus` is distinguishable in
the log**, which is the property the dispatch asked for.

---

## 8. FILES TOUCHED

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp` | **+23 / −0.** Four `UE_LOG` lines + their comments. The only source file touched. |
| `.claude/pipeline/handoffs/TASK-1394-programmer.md` | this note (new) |
| `.claude/pipeline/TASKBOARD.md` | **TASK-1394's own `status:` line only** (`Edit`, never `replace_all`) |

**Assets referenced:** none. No `/Game/` path, no `.uasset`, no new `IA_` action. The three existing
actions (`IA_MenuUp` / `IA_MenuDown` / `IA_MenuAccept`) are named only inside log strings and existing
binding code.

---

## 9. WHAT QA SHOULD SCRUTINISE

1. **The 3-vs-4 count (§1).** The row's headline says three; its own enumeration compels four. Please
   adjudicate rather than bounce — and note that reading *"`TASK-1394`'s three new lines"* in five
   downstream rows as a checkable count will produce a false finding.
2. **The Accept line's +23 line-number shift (§6).** Bytes identical, coordinates moved. Five board
   citations and one QA report now point at the wrong line. I deliberately did not fix other rows.
3. **`%+d` with `int32 Delta`** — `+1` / `-1` in the output. Confirm the format/arg pairing on all
   four lines: (a) has **no** variadic args and **no** `%` token in its string; (b1)/(b2) have one
   `%+d` / one arg; (b3) has `%+d %d %d %d %s` / five args in that order.
4. **`Buttons[Next]->GetName()` null-safety** — `GetMenuButtons` (`.cpp:212-219`) only appends a
   non-null `UButton*`, and `IsValidIndex(Next)` gates the access. Please re-derive rather than take
   my word.
5. **That the diff really is insertion-only** — `git diff` on the file should show **zero** `-` lines.
   That is the cheapest possible check that behaviour is unchanged, and it is the one `TASK-1400`
   (same file, blocked on this row at `qa-passed`) depends on.
6. **The `SetUserFocus` trap (§3)** — confirm no added line calls an already-focused target a failure
   or a refusal, and that none of them reads `FocusButton`'s return value.
7. **`HandleMenuUp` and the header are untouched** — both should show clean.

---

## 10. ROUTING

`ready-for-qa` → `qa-reviewer` (`TASK-1401`) → **5a build-master compile** (C++ ⇒ editor **CLOSED**,
then **relaunched on the new binaries**, graceful-quit lane, **never** Live Coding) → **5b =
`TASK-1395`** → commit host **`TASK-1396`**.

⚠️ **`TASK-1395` cannot read this instrument until 5a has run and the editor has been relaunched** —
a running process holds the old code, and the new line would be silent for the old reason. That is a
guaranteed false negative and `TASK-1395`'s own (1) control arm is written to catch it.
