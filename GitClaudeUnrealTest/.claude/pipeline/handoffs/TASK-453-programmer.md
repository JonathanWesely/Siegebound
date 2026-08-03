# TASK-453 — [W1-JOIN] Hand the live console widget to the component at open time

- **assignee:** gameplay-programmer · **date:** 2026-08-03 · **status on completion:** `ready-for-qa`
- **QA gate:** TASK-446 (item **(j)**) · **compile gate:** TASK-447 — the ONE gate
- **files touched (EXCLUSIVE owner, sequential re-ownership after TASK-449):**
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegePlayerController.cpp` — **one insertion, 42 lines**
  - `SiegePlayerController.h` — ⛔ **NOT TOUCHED.** No new member, no new method, no new include was needed. Declared here so QA does not go looking for a header change: **the deliverable needed none.**
- **files NOT touched:** everything else. ⛔ No component edit, no widget edit, no `Build.cs`, no `Content/`, no `.ini`, no Git, no editor, no MCP, no PIE, **no compile.**

> **M8 DECLARATION, verbatim: adds no replicated property, no new replicated class, no new relevancy tier.**
> *Why it is true rather than merely asserted:* the entire change is one client-local call on a `UActorComponent` the local controller already owns, inside a path that only runs on the machine that pressed the key. No RPC, no `GetLifetimeReplicatedProps` entry, no `UPROPERTY` added at all, no authoritative outcome reachable. Handing a local widget to a local component decides nothing about the match.

---

## 1. ⛔ THE HARD STOP — CHECKED, AND IT DID NOT FIRE. THE ORDER OF EVENTS MATTERS, SO HERE IT IS

The spec's item (3) and my dispatch both say: *if `AttachConsoleWidget` does not exist when you arrive, STOP AND REPORT; do not edit the component.* **I checked it four times, and the first two answers were WRONG — recording that honestly because it nearly produced a false blocker report.**

| # | probe | result |
|---|---|---|
| 1 | `Grep` over `Source/` | **no matches** |
| 2 | `git grep` + `grep -rn` repo-wide over `*.h`/`*.cpp` | **no matches** |
| 3 | `grep` directly on `SiegeAssistantComponent.h` | ✅ **FOUND — `:723`** |
| 4 | re-verified after a further edit by 443 (mtime `12:36:47`) | ✅ **still present, unchanged** |

⚠️ **Why probes 1–2 lied, and it is not a tooling bug:** `SiegeAssistantComponent.{h,cpp}` are **untracked** (`??` in `git status`), so `git grep` cannot see them at all — and **TASK-443 was writing the file between my calls** (both its files' mtimes moved during this session: `.cpp` `12:34:33`, `.h` `12:36:47`). ⇒ **A single negative grep against a file another agent is actively authoring is not evidence of absence.** I did not report the blocker on probe 2, and that was the right call.

**The pinned signature matches CHARACTER-FOR-CHARACTER:**

```cpp
// board  : void USiegeAssistantComponent::AttachConsoleWidget(USiegeAssistantConsoleWidget* InWidget);
// shipped: SiegeAssistantComponent.h:723
UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant")
void AttachConsoleWidget(USiegeAssistantConsoleWidget* InWidget);
```

`public:` governs it (last specifier at `:602`), so the call site is legal. ⛔ **I did not touch the component. Not one byte.** TASK-443's own header comment at `:697` says *"⛔ THE CALL SITE IS TASK-453's (`SiegePlayerController`), NEVER THIS FILE'S"* — **the two tasks agree exactly, with no coordination between them.**

---

## 2. ⛔ WHERE THE CALL SITS, AND WHY THE POSITION IS THE ENTIRE TASK

`ASiegePlayerController::OnAssistantConsolePressed()`, **`SiegePlayerController.cpp:4322-4331`** — after the creation null-check (`:4282`), **before `Console->OpenConsole()` (`:4333`)**.

```cpp
if (USiegeAssistantComponent* Assistant = GetAssistantComponent())
{
    Assistant->AttachConsoleWidget(Console);
}
else { /* Warning; the console still opens, inert */ }

Console->OpenConsole();
```

### ⛔ IT MUST PRECEDE `OpenConsole()`, AND THIS IS THE ONE FINDING QA SHOULD CHECK HARDEST

**I did not take this on faith from the spec — I read the widget.** `USiegeAssistantConsoleWidget::OpenConsole()` **ENDS** with the broadcast (`SiegeAssistantConsoleWidget.cpp`, final two statements):

```cpp
OnConsoleOpenStateChanged(true);
OnConsoleOpenChanged.Broadcast(true);
```

The component subscribes to `OnConsoleOpenChanged` **inside `AttachConsoleWidget`** (443's header `:690-694`: it *"binds all eight forwards"*). Therefore:

- **Attach BEFORE ⇒** the broadcast lands on a live listener; the component's `NotifyConsoleOpened` runs, **Idle → Composing**, on the very first press. ✅
- **Attach AFTER ⇒** the component is not yet bound when the **only** "opened" broadcast of that press goes out. **The FSM sits in `Idle` while the player types into a box it does not know is open**, and it self-corrects only on the *second* open. ⛔ **No error, no log, no crash — the exact silent-failure class this task exists to kill, one level deeper than the `BeginPlay` trap the spec warned about.**

⚠️ **The spec's wording is *"immediately after a successful create/open"*. I read that as after the successful CREATE and before the OPEN, because that is the only ordering in which the component actually receives the open event.** If QA reads "after the open" literally, it is choosing the broken half — and the widget source above settles it, not my opinion. **Flagged rather than silently resolved.**

### ⛔ 449's GUARD-FIRST ORDERING IS UNDISTURBED — THE REFUSAL PATH CANNOT REACH MY CALL

`SetAssistantConsoleOpen(true)` at `:4274` still runs **before anything is created**, and its refusal `return`s at `:4278`. My block is at `:4322`, **below the creation that is itself below the guard.** ⇒ On a refusal (placement / targeting / group pick / match ended) there is still **no widget, nothing added to the viewport, no focus move, no flash — and now also no attach.** The refusal path is byte-identical; I inserted below it, never inside it.

---

## 3. WHY IT SURVIVES CLOSE-AND-REOPEN — BY CONSTRUCTION, NOT BY LUCK

The call sits on the **per-press open path**, so it fires on **every** successful open, not once at creation.

- **Close** — `CloseConsole()` broadcasts `OnConsoleOpenChanged(false)`; the component's `NotifyConsoleClosed` runs. The widget persists on the viewport (collapsed) and the component's binding persists with it.
- **Reopen** — the close-first branch (`:4246`) is skipped, the guard re-runs, `GetOrCreateAssistantConsoleWidget()` returns the **same** widget, and **`AttachConsoleWidget` is called again.** Per its pinned contract it *"drops every binding before it makes it"* and **re-seeds** the widget from live FSM state ⇒ **exactly one of each binding, and a console reopened mid-conversation is correct on its first frame** (443's SEED-THEN-BIND, header `:717`).

⚖️ **Why the per-open call rather than a one-shot at the creation site** (`GetOrCreateAssistantConsoleWidget`, right after 449's `AddDynamic` — the other obvious landing spot): a creation-time attach fires **once, ever**, and has **no recovery** if the link is ever dropped — an FSM-side `DetachConsoleWidget()` (443 ships one, `:727`), a widget rebuilt mid-match, or a stale weak pointer (443 holds the widget **weakly** by design, `:699`). The per-open call re-establishes all eight forwards on the next press. **It costs one idempotent call per keypress and the spec explicitly blesses it:** *"you may call it on every open without double-binding."*

**"Exactly once per widget"** (spec item 1 / QA item (j)) is satisfied **in effect**, which is the claim that matters: idempotence guarantees one set of bindings per widget however many times I call. ⚠️ **If QA reads it as "call the function only once", that contradicts the same spec sentence two lines below it and the contract 443 shipped — I chose the reading that both documents share.**

---

## 4. NULL-SAFETY (spec item 4) — RESOLVED FROM SELF, AND A MISSING COMPONENT IS INERT, NOT FATAL

`GetAssistantComponent()` (`SiegePlayerController.h:626`, inline, returns the `AssistantComponent` default subobject created in the constructor at `.cpp:152`). Resolved into an `if`-scoped local, so the pointer is checked and used in one expression and cannot be used unchecked.

**On null** — a genuine construction defect, not a normal state — the console **still opens**: I log one `Warning` and fall through to `OpenConsole()`. ⛔ **I deliberately do NOT roll back the posture here.** Rolling it back would refuse the player the *console* over a defect in the *assistant*, which is the worse behaviour and is inconsistent with 449's rule that the console is never a requirement for any action. The player gets a console whose Enter goes nowhere — **precisely how this seam behaved before this task existed**, so the failure mode is a known one, not a new one.

⛔ **§2 still holds: no keyboard command changes behaviour and no key is re-routed through the assistant.** I added no binding, touched no `BindAction`, and changed no key.

---

## 5. ✅ THE ADDITIVE PROOF (spec item 5) — `git diff --stat`, QUOTED

**Pre-flight `git status --porcelain`, run and BRANCHED ON, not predicted:**
```
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h
```
⇒ both files carry TASK-449's **uncommitted** work, so I edited **on top of it**, additively.

| | insertions | deletions |
|---|---|---|
| after TASK-449 (measured by me at start, matching its handoff §0) | 715 | **94** |
| after TASK-453 (now) | **757** | **94** |

```
 .../Siegebound/SiegePlayerController.cpp           | 577 ++++++++++++++++++---
 .../Siegebound/SiegePlayerController.h             | 274 +++++++++-
 2 files changed, 757 insertions(+), 94 deletions(-)
```

⇒ **+42 insertions, and the deletion count is UNCHANGED at 94.** ✅ Nothing of TASK-440's or TASK-449's was reverted, reformatted or moved. **The header's `274` is also unchanged from 449's figure — independent confirmation that I made zero header edits.** All 94 deletions remain TASK-440's `SpawnGroupCircleDecal` private→public move.

---

## 6. ⛔ WHAT I COULD NOT VERIFY — AND ONE ITEM IS A REAL SEQUENCING CONSTRAINT ON TASK-447

**Nothing here has been compiled, linked or run.**

1. ⛔ **`AttachConsoleWidget` IS DECLARED BUT NOT YET DEFINED. THIS WILL NOT LINK UNTIL TASK-443 LANDS ITS `.cpp`.** Verified twice: `USiegeAssistantComponent::AttachConsoleWidget` **has no definition** in `SiegeAssistantComponent.cpp` as of `12:34:33`. My call therefore **compiles** (the declaration is in scope via the existing `#include` at `.cpp:32`) but will fail at **link** with an unresolved external. ⚠️ **This is the state the board predicted** (*"it will not LINK alone and is not expected to"* — the TASK-416/417 precedent) ⇒ ⛔ **do NOT open a QA loop over it.** ⚠️ **BUT IT IS A HARD ORDERING CONSTRAINT AND I AM STATING IT LOUDLY: TASK-447 MUST NOT RUN UNTIL TASK-443's DEFINITION EXISTS**, or the gate fails on my line for a reason that is not my defect. **Check for the definition before compiling, not after.**
2. **UHT has not run.** I added no `UPROPERTY`/`UFUNCTION`, so my exposure is nil — but `AttachConsoleWidget` is a `UFUNCTION(BlueprintCallable)` on 443's side and I call it from C++, which needs no reflection at all.
3. **Overload resolution unverified by a compiler.** `Console` is `USiegeAssistantConsoleWidget*` and the parameter is `USiegeAssistantConsoleWidget*` — an exact match by reading, not by build.
4. ⛔ **ZERO runtime verification. Nobody has pressed this key.** The before-`OpenConsole()` argument in §2 is a **read-through of the widget's source**, proven by statement order; **that the FSM actually reaches `Composing` is not observed.**
5. **The idempotence and seed-then-bind claims are 443's CONTRACT, read from its header comments — not executed and not verified by me.** If 443's implementation does not honour them, my per-open call double-binds. ⚠️ **QA item (j) already owns exactly this check** (*"the component's implementation is IDEMPOTENT and NULL-SAFE"*) — **it is the check my design depends on, so it is not optional.**
6. **The key is still INERT until TASK-445's `IA_AssistantConsole` asset is mapped in `IMC_Hero`.** ⚠️ **TASK-448 cannot test any of this without it.** *(An `IA_AssistantConsole.uasset` is now untracked in `Content/Input/Actions/` — 445 appears to have landed it; the `IMC_Hero` mapping is not mine to confirm.)*

---

## 7. NOT DONE — DELIBERATELY

- ⛔ **No FSM logic, no delegate bodies, no executor, no confirm-step code, no posture change.** Spec: *"Nothing else."*
- ⛔ **No `BeginPlay` binder** — the trap the task exists to avoid.
- ⛔ **No attach in `HandleAssistantConsoleOpenChanged(true)`** (449's FSM-driven open path, `:4334`). If a future FSM opens the console, **the component is the thing doing the opening and is already attached** — an attach there would be circular. Flagged, not silently skipped.
- ⛔ **No `DetachConsoleWidget()` call in `EndPlay`.** 443's header says its own `EndPlay` calls it (`:725`), and the component outlives nothing the controller does not. ⚠️ **If QA finds 443 does NOT detach on its own teardown, a one-line detach in 449's `EndPlay` block is the fix and it belongs to whoever owns this file next.**
- ⛔ **No `ToggleConsole()` reintroduced** — manager-ratified exclusion, untouched.
- ⛔ **No cleanup, reformat or "tidy" anywhere.**

---

## 8. WHAT QA SHOULD SCRUTINISE HARDEST

1. ⛔ **§2 — that the call precedes `OpenConsole()`**, and that the reason is the terminal `OnConsoleOpenChanged.Broadcast(true)` in the widget. **Read `SiegeAssistantConsoleWidget::OpenConsole()`'s last two lines and rule on it.** If this is wrong, the first open is silently dead — and that is item (j)'s BLOCKER shape.
2. **§3 — the per-open (not per-creation) call**, and whether "exactly once per widget" is satisfied by idempotence. I argue it is; the spec's own next sentence agrees.
3. **§1 — that I touched only the controller and 443 only the component.** ⚠️ **And that my negative-grep story is understood: probes 1–2 returned nothing because the component files are untracked AND being written concurrently.** A reviewer repeating probe 2 may reproduce the false negative.
4. **§5 — deletions still 94, header insertions still 274.**
5. **§6.1 — the link-order constraint on TASK-447.** This is the one thing that can waste a gate.
6. ⚠️ **`Grep` mangles comment syntax on this machine** (CONVENTIONS §10) — my addition is 30 lines of `//` comments + a 10-line `if/else`. **Confirm structural findings from raw `Read` output only.**

### TASK-448 (Jonathan, playtest) — what this task adds
1. **Open the console, type a sentence, press Enter ⇒ the assistant actually responds.** *(This task is what makes Enter go anywhere at all. Requires 443 + 445.)*
2. **Close it and reopen it, then submit again ⇒ still works, and no duplicate response** (the double-bind symptom would be two model turns per sentence, surfacing as a spurious "busy" refusal).
3. **Open it mid-conversation ⇒ the state/transcript/confirm prompt are correct on the first frame**, not the next event (seed-then-bind).

---

## Compliance

No Git · no compile · no editor / MCP / PIE · no `Content/` · no `.ini` · sealed holdout untouched · `L_Arena` untouched · **stayed inside `SiegePlayerController.cpp` alone** · component and widget **untouched** · TASK-449's work edited **additively**, with the diff-stat proof in §5.
