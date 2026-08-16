# TASK-604 — AUTOMATION TESTS `Tests/SiegeAccountTest.cpp` — programmer handoff (2026-08-16)

## What changed

ONE new file, nothing else touched:

- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAccountTest.cpp` (new) — 7 automation tests under `Siegebound.Account.*`, pure-function/unit lane (no world, no PIE, no widget, no model), cloned from the `SiegeSettingsTest.cpp` scratch-slot idiom.

Written **against the ACC-§7 pinned signature registry, character-for-character**, NOT against TASK-599/600/601/602/603's in-flight files (single-writer law; the compile gate TASK-606 reconciles). Observed in passing: TASK-601's `SiegeSettingsSubsystem.h` additions are already present in the working tree — noted only, not relied upon.

## The tests and what each pins

| Test (`Siegebound.Account.*`) | Registry/law pins |
|---|---|
| `SlotContract` | `USiegeAccountSaveGame::SlotName == "SiegeAccounts"` + `UserIndex == 0` (ACC-§7 block 1) · guest constants `USiegeDeckSaveGame::SlotName == "SiegeDecks"`, `USiegeSettingsSubsystem::SettingsSlotName == "SiegeSettings"` (ACC-§3 table, `TestEqualSensitive` per SC-§13) · scratch slot ≠ shipped registry slot (mechanical hermeticity). |
| `CredentialHashContract` | `MakeCredentialHashHex` (ACC-§7 block 2): determinism (`TestEqualSensitive`) · salt sensitivity · password sensitivity · 40-hex shape · **the ACC-§2 formula computed independently in the test** (`FSHA1::HashBuffer(UTF8(Salt + ":" + Password))` → `BytesToHex`). |
| `ProfileSlotSuffix` | `MakeProfileSlotSuffix == ProfileId.ToString(EGuidFormats::Digits)` sensitive · 32 chars, all hex · the concrete Digits rendering of a known GUID · per-GUID determinism (ACC-§3). |
| `GuestFallbackIsBareConstants` | Fresh store: `IsLoggedIn()==false`, empty `GetActiveDisplayName()`, invalid `GetActiveProfileId()`, `GetDeckSlotName()`/`GetSettingsSlotName()` == the bare shipped constants AND the literals, sensitive (ACC-§1/§4) · CI-clean cold boot: `Login` on an empty registry rejects, populates `OutReason`, writes no file. |
| `CreateLoginLogoutRoundTrip` | `CreateAccount` → active + valid `FGuid` + display-name casing preserved (sensitive) · composed slots exactly `"SiegeDecks_<Digits>"` / `"SiegeSettings_<Digits>"` (ACC-§3 table, sensitive) · save-on-change wrote the scratch registry · `Logout` → guest constants again · `Login` → same `ProfileId` · **case-insensitive lookup** with created casing returned (ACC-§3). |
| `TwoProfilesDisjointSlots` | Two creates ⇒ distinct `ProfileId`s, disjoint deck/settings slots, all ≠ guest · login-switch back re-resolves profile A's slots byte-identically. |
| `RejectionsPopulateOutReason` | Every rejection class returns `false` + non-empty `OutReason` (the `IsDeckLegal` idiom): empty/whitespace/2-char-after-trim/25-char names · empty/3-char passwords · duplicate name exact + **case-variant** + padded (ACC-§3 uniqueness) · unknown login name · **wrong password** · state undisturbed after each rejection · boundary creates (3-char name, 24-char name, 4-char password) succeed · correct-login anchor at the end (anti-vacuous, the `SwizzleAxis` lesson). |

## CI-clean + leave-nothing-behind (spec item 2)

- Every store is redirected to `"SiegeAccounts_AutomationScratch"` via `SetSlotNameForAutomationTests` BEFORE any operation; `FAccountScratchGuard` deletes that slot on the way in AND out (the settings-guard clone). The shipped `SiegeAccounts.sav` is unreachable — asserted mechanically in `SlotContract`, not commented.
- **The seed-copy janitor:** `CreateAccount` performs the RULING-8 seed-copy, which on a dev machine READS the real guest slots (reads only — ACC-§1) and WRITES `SiegeDecks_<Digits>`/`SiegeSettings_<Digits>`. The guard tracks every created profile's suffix and deletes both copies in its destructor. Suffixes are fresh GUIDs, so the janitor can only ever delete files this run created. On a CI-clean machine the guest slots don't exist ⇒ no copy is made ⇒ nothing to clean; the tests hold in both worlds.
- Tests never call `Initialize` (a `FSubsystemCollectionBase` cannot be fabricated outside the engine's creation path — the `SiegeSettingsTest.cpp` header precedent), so every store starts with an empty in-memory registry = the no-save-files first-boot state.

## Deliberate decisions QA should scrutinize

1. **One plain `TestEqual` on an `FString`, on purpose, flagged so it is not misread as an SC-§13 violation:** the ACC-§2 formula assertion in `CredentialHashContract` compares `MakeCredentialHashHex` against an independently computed digest **case-insensitively**, because ACC-§2 pins the digest BYTES, not the hex CASE (hex of different bytes can never differ by case alone, so the assert still fails on any byte deviation). Every casing/byte-identity claim elsewhere in the file uses `TestEqualSensitive`. The comment at the call site says all of this.
2. **"A rejected login writes no registry file"** (`GuestFallbackIsBareConstants`): derived from the ACC-§7 tail ("load once, save on change") — a rejection changes nothing so it may not save. If TASK-600 legitimately saves on failed login, this is the assertion to argue about; it is one line.
3. **The concrete `"00112233445566778899AABBCCDDEEFF"` literal** in `ProfileSlotSuffix` pins the ENGINE's `EGuidFormats::Digits` rendering (uppercase `%08X`×4). If it ever fails while the `ToString(Digits)` sensitive-equality above it passes, the engine changed — the comment marks that as a finding, not a test bug.
4. Composed-slot assertions build expected values from the **literal** prefixes `"SiegeDecks_"`/`"SiegeSettings_"` (the ACC-§3 table), while `SlotContract` pins the guest constants to the same literals — so constant drift and composition drift are both caught, from two directions.
5. Bare `"SiegeDecks"`/`"SiegeSettings"` literals appear in this file **by design** — the ACC-§4 grep law explicitly exempts the two guest definition sites *and the tests*.

## Named coverage gaps — stated, not papered over (each with the checkable mechanism)

- **Cross-instance registry reload** (create in store 1, read in store 2): NOT assertable. Mechanism: `Initialize` is the ONLY registry load path in ACC-§7, and a `FSubsystemCollectionBase` cannot be constructed from a test (`SiegeSettingsTest.cpp:34-43` precedent; unlike the settings lane there is no pinned `Load...FromSlot()` public seam). Closes at Jonathan's TASK-609 (d) across a session boundary. If TASK-600 happens to ship a public load helper beyond the registry, a follow-up test can cover it — not assumed here.
- **`OnActiveProfileChanged` broadcast counts**: NOT assertable. Mechanism: binding a dynamic multicast needs a `UFUNCTION` on a `UCLASS`, which a test `.cpp` cannot declare (no UHT header), and ACC-§7 pins no broadcast counter on the account subsystem (the settings lane's `SettingsChangeBroadcastCount` has no account twin). Closes at TASK-605 code review (call-order inspection) + TASK-609 (f) behaviorally.
- **RULING-8 seed-copy correctness** (payload actually copied): NOT assertable CI-clean. Mechanism: proving it would require WRITING the real guest slots from a test — forbidden (never write the real slots). Closes at TASK-609 (g).
- **Consumer-side no-subsystem fallback** (deck/settings lanes falling back to guest constants when the account subsystem is unresolvable): TASK-601/602's code, their files — out of my names block. The subsystem-side guest behavior IS covered here. Closes at TASK-605's per-task review.
- **`LastLoginUtc`/`CreatedUtc` updates**: not observable through any pinned accessor (the registry save game object is not exposed by the subsystem). Closes at TASK-605 code review.

## ACC-§2 compliance statement

No password value is persisted, logged, or embedded in any assertion message in this file; password literals appear ONLY as call parameters to `CreateAccount`/`Login`/`MakeCredentialHashHex` (which is what tests are for). Nothing in the file calls the Phase-1 credential "secure" — the header comment carries the honest-credential phrasing verbatim.

## M8 declaration (verbatim, per the batch header)

Adds no replicated property, no new replicated class, no new relevancy tier, no RPC. All account state is client-local (`UGameInstanceSubsystem` + local `USaveGame`); the display name touches no session/player name (A7). Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.

## Refused / not done, with mechanisms

Nothing in the task was mis-assigned; nothing was refused. Not done by law: no compile (QUIET-MODULE — TASK-606's), no editor/MCP touch, no git, no board rewrite beyond the single status flip, no file outside my names block.
