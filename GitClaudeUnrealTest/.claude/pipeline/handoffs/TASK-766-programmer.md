# TASK-766 — one stale clause in the unset-ghost log string

**Agent:** gameplay-programmer
**Status:** ready-for-qa
**Date:** 2026-09-01
**Source:** TASK-765 (raised it, correctly left it — `SiegeGameMode.cpp` was outside its fence)
**Law:** `SC-§36` (prose that misdescribes shipped state is a debt that fails nothing and is therefore silent) · `HIGH-§1` · `GHOST-§5`

---

## The change — one file, one line

**File:** `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp`, line **293**, inside `ResolveGhostPawnClass()`.

**BEFORE:**

```cpp
TEXT("[%s] No ghost pawn blueprint configured (GhostPawnClassAsset is unset — the shipped default, TASK-750) — using the raw C++ ASiegeGhostPawn."),
```

**AFTER:**

```cpp
TEXT("[%s] No ghost pawn blueprint configured (GhostPawnClassAsset has been cleared — the shipped default is BP_SiegeGhostPawn, TASK-764) — using the raw C++ ASiegeGhostPawn."),
```

Nothing else in the file was edited.

## Why the old clause was false

TASK-750 left `GhostPawnClassAsset` deliberately blank, so at that time "unset" genuinely *was* the shipped default and the clause was accurate. **TASK-764 set the property in the constructor at `:67`** (`/Game/Blueprints/BP_SiegeGhostPawn.BP_SiegeGhostPawn_C`), which retired that fact. From that commit onward the log told a reader of a live log that the state they were looking at was normal and shipped, when in fact it means somebody **cleared** a field that ships populated — i.e. the message was actively steering diagnosis away from the thing worth noticing.

The new wording states both halves that are now true: the field was cleared, and what it ships as.

## ⛔ The branch is unchanged and still reachable

Only the string literal moved. Byte-identical around it:

- `if (GhostPawnClassAsset.IsNull())` — the condition itself, untouched
- `bWarnedGhostClassMissing` — the one-shot latch and its set, untouched
- verbosity stays **`Log`**, ⛔ not `Warning` — this case is still not an alarm
- the argument list stays `*GetNameSafe(this)` (one `%s`, one argument — matched)
- `return ASiegeGhostPawn::StaticClass();` — untouched

**Clearing the field remains a legitimate designer route back to the raw C++ ghost**, exactly as `SiegeGameMode.h:487-490` says (TASK-765's sweep). The new string is now *more* consistent with that docblock than the old one was: the header says "the designer clearing this field is a legitimate way back to the raw C++ ghost", and the log now uses the same verb.

## ⭐ Sweep of the rest of that message — asked for, and scoped to it

The other three clauses in the same `TEXT(...)` were each checked against the code and are **all still true**:

| Clause | Verdict | Evidence |
|---|---|---|
| `[%s]` | ✅ true | fed by `*GetNameSafe(this)` on `:294`; one specifier, one argument |
| `No ghost pawn blueprint configured` | ✅ true | this branch is guarded by `GhostPawnClassAsset.IsNull()` — the field genuinely is empty here |
| `using the raw C++ ASiegeGhostPawn.` | ✅ true | `return ASiegeGhostPawn::StaticClass();` two lines below, `:296` |

⇒ **exactly one stale clause existed in this message, and it is gone.** I did not go hunting the file.

## ⚠️ Raised, ⛔ NOT edited — for the manager

The **comment block at `:282-286`**, directly above the branch, is stale for the same `SC-§36` reason:

> `⭐ THE UNSET CASE IS THE EXPECTED, SHIPPED CASE AND IS NOT A WARNING (see the header): no task in this batch authors a ghost blueprint, so the raw C++ class is the ghost.`

Both halves are now retired — TASK-763 authored `BP_SiegeGhostPawn` and TASK-764 pointed at it, so the unset case is no longer expected-and-shipped, and a task in the batch *did* author a ghost blueprint. The **code under the comment is correct** and the comment's final sentence (the *reason* the case is logged at `Log` rather than `Warning`) is still sound reasoning — it is only the shipped-state framing that has gone false.

I left it verbatim because the fence for this task was explicitly "repair the clause that misdescribes shipped state **in the log string**, nothing else". It wants the same treatment TASK-764 gave the `:55-66` comment (quote the retired claim, then say what replaced it) — one line of manager judgement, not a programmer's unilateral sweep.

## ⛔ Fences honoured

- ⛔ **`:67` (TASK-764's line) NOT touched**
- ⛔ death → ghost → 180 s lifecycle, `ResolveHeroToRestore`, `ClearMarks()` at `PlayAgain()` step 6b, the `OnHeroRecallArrived` bind — **all four untouched** (all passed QA, all load-bearing)
- ⛔ `SiegeGameMode.h` **not opened for edit** (TASK-765's swept docblock; read only, to confirm the new wording agrees with it)
- ⛔ `Tests/SiegeRespawnLifecycleTest.cpp` **not opened for edit** (TASK-765's repaired assertion)
- ⛔ no compile · ⛔ no editor (it is UP and stays up) · ⛔ no MCP · ⛔ no Git

## ⛔ Suite count: stays 248, delta ZERO

This is a string-literal edit inside a `UE_LOG` in a **non-test** translation unit. No test file was edited, no `IMPLEMENT_SIMPLE_AUTOMATION_TEST` was added, removed, or renamed.

**It should not move at all, and it does not.** I additionally confirmed **no test asserts on this message**: a grep over all of `Source/` for `No ghost pawn blueprint` returns exactly one hit — the emitting line itself — so no automation test can break on the new wording either.

## What QA should scrutinise

1. **The diff is one line.** If anything other than `SiegeGameMode.cpp:293` shows a change, that is not mine.
2. **The `%s` count still matches the argument list** — one specifier, one `*GetNameSafe(this)`. (The neighbouring `Warning` at `:314-316` has *two* of each; don't cross-read them.)
3. **Verbosity is still `Log`.** The whole point of the branch is that this case is not alarming; a drift to `Warning` would undo TASK-750's deliberate design.
4. **The new clause is itself checkable prose** — it names `BP_SiegeGhostPawn` and TASK-764. If a future task ever clears `:67` again, *this* clause becomes the stale one, by the same `SC-§36` rule.
5. The `:282-286` comment above the branch is **knowingly left stale** — flagged above, not an oversight.
