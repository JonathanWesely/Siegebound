# TASK-516 — [AX-1] `USiegeKeyboardLayoutSubsystem::GetPositionalKey` — programmer handoff

- **Task:** TASK-516 (batch ASSISTANT-EXCLUDE) · **assignee:** gameplay-programmer
- **Status set to:** `ready-for-qa`
- **QA gate:** **TASK-525** (`.claude/pipeline/qa/TASK-525.md`) — `SC-§29`. ⛔ No other gate file exists for this batch.
- **Law implemented:** CONVENTIONS `KBD-§8` (the *"PIN AMENDED 2026-08-04 — `GetPositionalKey`"* block) · `KBD-§4` · `KBD-§5` · `KBD-§6` · `KBD-§7` · `AS-§20.5`
- **Date:** 2026-08-04

## M8 DECLARATION (verbatim, as required)

**adds no replicated property, no new replicated class, no new relevancy tier.**

Reason is structural and unchanged from `KBD-§10`: this is a `const` read of one client-local
`TMap<FKey, FKey>` on a `UGameInstanceSubsystem` — one per client process, never touched by the
network. The same sentence is written into the header above the declaration.

## Files touched — TWO, and only two

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeKeyboardLayoutSubsystem.h` | +53 lines: the doc comment + the declaration, inserted **between `RefreshKeyboardLayout()` and `OnKeyboardLayoutChanged`** — the exact position the `KBD-§8` pin lists it in. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeKeyboardLayoutSubsystem.cpp` | +21 lines: the definition, placed **after `RefreshKeyboardLayout()` and before `SetTranslationMapForAutomationTests()`**, so .cpp order still matches .h order. |

⛔ **Nothing else moved.** No `Build.cs` change · no new member · no change to `TranslationMap`'s
privacy · no Win32 · no include added (`InputCoreTypes.h` was already included for the member) ·
`SiegeKeyboardLayoutStatics.{h,cpp}` untouched · `Tests/SiegeKeyboardLayoutTest.cpp` untouched ·
nothing in the assistant lane touched. **No compile, no Git, no editor/MCP/PIE.**

## The signature — as written, next to the pin

Pinned in `KBD-§8` (registry one-line form):

```cpp
UFUNCTION(BlueprintPure,     Category = "Siegebound|Input") FKey GetPositionalKey(const FKey& QwertyKey) const;
```

As written in `SiegeKeyboardLayoutSubsystem.h:274-275`:

```cpp
UFUNCTION(BlueprintPure, Category = "Siegebound|Input")
FKey GetPositionalKey(const FKey& QwertyKey) const;
```

⚠️ **The ONLY difference is whitespace/line-break, and it is the file's existing rendering of the
same pin:** the registry writes all four reflected members on one line for compactness, while the
shipped header already puts every `UFUNCTION(...)` on its own line above its declaration
(`:182 IsPositionalRemapActive`, `:200 DescribeActiveTranslation`, `:221 RefreshKeyboardLayout`).
**Every token that the linker or UHT can see is character-for-character identical** — specifier set
`BlueprintPure`, `Category = "Siegebound|Input"`, return `FKey`, parameter `const FKey& QwertyKey`,
trailing `const`. Matching the pin's literal single-line layout instead would have made this the only
member in the file formatted differently. **Flagging it explicitly so QA rules on it rather than
discovering it.**

## The body — three lines, and the comment is longer than the code

```cpp
const FKey* TranslatedKey = TranslationMap.Find(QwertyKey);
return TranslatedKey ? *TranslatedKey : QwertyKey;
```

The `Find`-then-fallback idiom is the one already in the file at
`SiegeKeyboardLayoutSubsystem.cpp:511` (`DescribeActiveTranslation`).

## What QA should scrutinise

1. ⛔ **`EKeys::Invalid` is unreachable — check the `FindRef` trap is closed.** `TMap::FindRef`
   would have been the "obvious simplification" and it is **WRONG**: it returns a
   default-constructed `FKey` on a miss, whose `KeyName` is left `NAME_None`
   (`InputCoreTypes.h:53-55`); `EKeys::Invalid` **is** `FKey(NAME_None)` (`InputCoreTypes.cpp:414`)
   and `operator==` compares `KeyName` alone (`InputCoreTypes.h:109`). So `FindRef` returns exactly
   the forbidden value **on the most common path of all** — a QWERTY host, where the map is empty and
   every lookup misses. **This is written into the .cpp comment so the next reader cannot re-introduce
   it.** Verify no future edit swaps in `FindRef`.
2. ⛔ **Direction.** `GetPositionalKey(EKeys::Z) == EKeys::Semicolon` on US-Dvorak is stated as that
   exact sentence in the header, together with the call-site form
   `InKeyEvent.GetKey() == GetPositionalKey(EKeys::Z)` and an explicit ban on the reverse lookup.
   Getting this backwards compiles and silently binds the wrong key, so it is worth reading twice.
3. ⛔ **`const`, no re-probe, and no attempt to hide the staleness.** No `mutable`, no `const_cast`,
   no timer, no call to `RefreshKeyboardLayout()` from inside. The **caller-refreshes /
   accessor-reads** ruling is documented in the header, including the instruction TASK-519 needs:
   `OpenConsole()` calls `RefreshKeyboardLayout()` **once per open**, and ⛔ **does not** bind
   `OnKeyboardLayoutChanged` from a widget.
4. ⛔ **Absent == identity, with no redundant branch.** The header says identity is never *stored*, so
   "no entry" and "maps to itself" are the same answer; an extra `if (Translated == QwertyKey)` would
   be dead code and is called out as such.
5. ⛔ **Lookup, not validator.** An invalid input returns that same invalid input. There is no
   `IsValid()` guard and there must not be one — the function must not start refusing keys the caller
   already holds.
6. ⭐ **`KBD-§8`'s label rule is respected because this task emits NO user-visible text at all.** There
   is no `FText`, no `FString`, no log line, no format string in the diff. The header states the rule
   for the next reader — *the player-facing prompt says `Z` on every layout; the lookup is for the
   comparison, the literal is for the human* — which is TASK-519's constraint to obey, not mine to
   discharge. **Nothing here can leak `"Semicolon"` into the UI.**
7. **Placement/order.** Declaration sits exactly where the pin lists it (after `RefreshKeyboardLayout`,
   before `OnKeyboardLayoutChanged`); definition mirrors that order in the .cpp.
8. **No other pinned signature was altered.** `HeroCharacter.cpp` compiles against
   `GetPositionalContext`, `Tests/SiegeKeyboardLayoutTest.cpp` against the four statics — none of
   those declarations were touched. `GetPositionalContext` remains deliberately **not** a `UFUNCTION`
   (UHT rejects a `const UObject*` return); I did not "fix" it.
9. ⚠️ **A gate-number trap I pre-empted in the header:** the class comment ends with
   `QA gate: TASK-513`, which belongs to the KEYBOARD-LAYOUT batch. This member's gate is **TASK-525**,
   and the doc comment says so in place so nobody greps for a TASK-513 entry that will never cover it.
   The class-level line was **left as-is** — it is still correct for everything above it.

## Notes for downstream (TASK-519 in particular)

- ⛔ **The accessor does not refresh.** `USiegeAssistantConsoleWidget::OpenConsole()` must call
  `RefreshKeyboardLayout()` once per open. This is the ruled contract, not a suggestion — without it
  the accept key is correct only by luck when `siege.Input.LayoutPollEnabled` has been turned off.
- ✅ **`BlueprintPure` means it is callable from BP** if the confirm UX ever moves; the C++ path needs
  no BP node.
- **Testability (TASK-523 owns the tests, I wrote none):** the seam already exists —
  `SetTranslationMapForAutomationTests({{EKeys::Z, EKeys::Semicolon}})` then assert
  `GetPositionalKey(EKeys::Z) == EKeys::Semicolon`, and on the empty map assert
  `GetPositionalKey(EKeys::Z) == EKeys::Z` **and** `!= EKeys::Invalid`. ⚠️ **Assert the
  `!= EKeys::Invalid` leg explicitly** — it is the leg a `FindRef` regression would break, and an
  identity-only assertion would pass on a QWERTY host for the wrong reason.

## Spec problems found: NONE that block

The spec and the `KBD-§8` amendment are internally consistent and the contract was implementable
exactly as written. Two observations, neither a defect:

- The prompt's *"the one-line idiom already exists internally at `SiegeKeyboardLayoutSubsystem.cpp:511`"*
  is accurate — `:511` is `const FKey* TranslatedKey = TranslationMap.Find(Probe.QwertyKey);` — but it
  is the **`Find` half only**; the fallback (`? * : QwertyKey`) has no precedent in the file because
  `DescribeActiveTranslation` `continue`s on a miss instead. Reusing `:511` literally without adding
  the fallback would be the bug. Recorded so QA does not read "idiom already exists" as "copy one line".
- ⚠️ **Staleness is a real, accepted property of this design, not a defect:** between two
  `RefreshKeyboardLayout()` calls a Win+Space makes the returned key stale. The 1 Hz poll
  (`siege.Input.LayoutPollEnabled`, default ON) closes it within ~1 s on the shipped path, and the
  per-open refresh closes it even with the lever off. **Called out because "the accessor returned a
  stale key" will look like a bug in a playtest and it is the ruled behaviour.**
