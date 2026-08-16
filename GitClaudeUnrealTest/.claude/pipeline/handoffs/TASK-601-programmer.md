# TASK-601 — [ACC-3] Settings lane profile scoping — handoff (gameplay-programmer)

**Status:** ready-for-qa · **Date:** 2026-08-16 · **Gate:** TASK-605 (`qa/TASK-605.md`)

## What changed (2 files, the task's exact names block — nothing else touched)

- `Source/GitClaudeUnrealTest/Siegebound/SiegeSettingsSubsystem.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeSettingsSubsystem.cpp`

Manager's pre-flight finding **confirmed first-hand before editing**: the settings lane's slot-name reads all funnel through `ResolveSlotName()` (formerly `SiegeSettingsSubsystem.cpp:177`), called from exactly three sites in the same file (`Initialize` log line, `LoadSettingsFromSlot`, `SaveSettingsToSlot`). This was a bounded diff, not a persistence rewrite.

### 1. `Initialize` (spec item 1)
- Gains `Collection.InitializeDependency<USiegeAccountSubsystem>()` **before** the existing `LoadSettingsFromSlot()` call, so a profile persisted as active in the account registry is visible to the first load.
- Subscribes the returned subsystem's `OnActiveProfileChanged` via `AddUniqueDynamic` (the `SettingsMenuWidget.cpp:377` idiom). Null return ⇒ no subscription, guest behavior, no crash (ACC-§4 fail-safe).

### 2. `ResolveSlotName()` precedence (spec item 2)
Now: `SlotNameOverride` (tests — **semantics unchanged**, still outranks everything) > account-profile slot via `USiegeAccountSubsystem::GetSettingsSlotName()` (unresolvable GameInstance or subsystem ⇒ fall through) > pinned `SettingsSlotName`.
- `SettingsSlotName` constant is **byte-identical, untouched at its definition** (`.cpp:19` after the include diff; still `TEXT("SiegeSettings")`). `SiegeSettingsTest.cpp:120-121` stays green as written.
- No new bare `"SiegeSettings"` literal introduced anywhere (ACC-§4 QA grep criterion).

### 3. New `ReloadForActiveProfile()` (spec item 3, ACC-§7 block 3)
Signature character-for-character from the registry: `void ReloadForActiveProfile();` (public). Logs the profile switch (always, unlatched — the missing-slot fallback log inside `LoadSettingsFromSlot` keeps its existing once-per-instance latch), then reloads through the SAME single load path (missing slot ⇒ C++ defaults off the `USiegeSettingsSaveGame` CDO).

### 4. Private `UFUNCTION() HandleActiveProfileChanged()` — the one structural addition beyond the spec text
`FOnSiegeActiveProfileChanged` is a **dynamic** multicast delegate, which binds by UFUNCTION name — but the ACC-§7 registry pins `ReloadForActiveProfile()` as a plain member with no `UFUNCTION` macro. Binding the pinned member directly would fail at bind time. Resolution: a private forwarder `UFUNCTION() void HandleActiveProfileChanged();` that only calls `ReloadForActiveProfile()`. The registry line stays character-for-character; the forwarder is internal and invisible cross-task.

## Flagged decision for QA — the "then broadcast" wording in spec item 3
Spec (3) says "reload …, then broadcast the EXISTING `OnSettingsChanged`". Implemented as: the reload funnels through `ApplyBoolSetting`, which broadcasts the existing `OnSettingsChanged` **exactly when a value actually changes** — NOT unconditionally. Reasons, on the record:
- The delegate law + this header's own contract: "Broadcast on every ACTUAL value change, never on a no-op write" — an unconditional broadcast on a no-change reload is the exact anti-pattern the law names.
- `BroadcastSettingChanged` remains the file's ONLY `OnSettingsChanged.Broadcast` caller — a structural property the shipped tests observe via `SettingsChangeBroadcastCount`.
- A no-change reload needs no UI refresh: the widget already displays the loaded value. The "refreshes for free" outcome (verified: `USettingsMenuWidget` subscribes at `SettingsMenuWidget.cpp:377`) holds precisely when a refresh is needed.
If QA reads spec (3) as mandating an unconditional broadcast, that is a conflict between the spec sentence and the standing delegate law; I followed the law and flag it here rather than absorbing it silently.

## Standing-law audits
- **Trailing-defaulted parameters (SC-§33): NONE added.** Zero new parameters of any kind on existing functions; the two new functions take no parameters. Call-site audit: N/A, vacuously satisfied.
- **Additive only (spec item 4):** no existing member renamed, moved, or removed. Existing edits are: 3 added includes (`Engine/GameInstance.h`, `Siegebound/SiegeAccountSubsystem.h`, `Subsystems/SubsystemCollection.h`), the `Initialize` insertion, the `ResolveSlotName` body (guest return value byte-identical: tier-2 guest returns the bare constant per ACC-§7 `GetSettingsSlotName()` contract; tier-3 returns `FString(SettingsSlotName)` as before), and two doc comments updated to match new behavior (`ResolveSlotName`, plus the new declarations' docs).
- **Dependency direction (ACC-§4):** Settings now includes the Account header — the lawful direction. Nothing in `SiegeAccountSubsystem.*` was touched by this task.
- **Guest path byte-identical (QA criterion):** with no account subsystem ⇒ identical control flow result to before (pinned constant). With the subsystem present but guest ⇒ `GetSettingsSlotName()` returns the bare `USiegeSettingsSubsystem::SettingsSlotName` per the ACC-§7 contract — same string, same slot, same file on disk.
- **Test-harness safety:** shipped tests build the subsystem on a bare `NewObject<UGameInstance>` outer and set the scratch override before any load (`SiegeSettingsTest.cpp:86-94`) — tier 1 short-circuits; even with an empty override, `GetSubsystem` on an uninitialized collection returns nullptr and tier 2 falls through. Tests never call `Initialize`, so the new `InitializeDependency` path doesn't run under automation.

## What QA should scrutinize
1. The flagged broadcast decision above — rule on it explicitly.
2. `ResolveSlotName()` is `const`; it calls `GetGameInstance()` (const) and `UGameInstance::GetSubsystem<>()` (const) — confirm no const-correctness issue against the 5.8 headers at compile (TASK-606's gate, not mine).
3. Cross-task compile coupling: this file now includes `Siegebound/SiegeAccountSubsystem.h` and calls `GetSettingsSlotName()` / `OnActiveProfileChanged` — both pinned in ACC-§7 block 2; any TASK-600 drift from the registry surfaces here at compile.

## M8 DECLARATION (verbatim, batch header)
Adds no replicated property, no new replicated class, no new relevancy tier, no RPC. All account state is client-local (`UGameInstanceSubsystem` + local `USaveGame`); the display name touches no session/player name (A7). Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.
