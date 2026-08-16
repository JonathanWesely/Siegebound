# TASK-600 — `USiegeAccountSubsystem` + the profile seam — programmer handoff (2026-08-16)

**Status: ready-for-qa.** File-only, per the QUIET-MODULE law: no compile, no editor, no MCP, no git. Compiled-against-registry honesty: this code has NOT been built (TASK-606 owns the gate); every engine signature I lean on was verified by reading the installed UE 5.8 headers (listed below).

## Files touched (the complete set — single-writer law respected)

- `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSubsystem.h` — **NEW**
- `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSubsystem.cpp` — **NEW**

Nothing else. No TASKBOARD writes beyond the one status flip; no CONVENTIONS edits; no Build.cs change needed (only Core/CoreUObject/Engine surface — `UGameplayStatics`, `FSHA1`, `FGuid`).

## Registry signatures used (`ACC-§7` block 2 — character-for-character)

- `virtual void Initialize(FSubsystemCollectionBase& Collection) override;`
- `UFUNCTION(BlueprintCallable) bool CreateAccount(const FString& DisplayName, const FString& Password, FString& OutReason);`
- `UFUNCTION(BlueprintCallable) bool Login(const FString& DisplayName, const FString& Password, FString& OutReason);`
- `UFUNCTION(BlueprintCallable) void Logout();`
- `UFUNCTION(BlueprintPure) bool    IsLoggedIn() const;`
- `UFUNCTION(BlueprintPure) FString GetActiveDisplayName() const;`
- `FGuid GetActiveProfileId() const;` (non-UFUNCTION, per registry)
- `UFUNCTION(BlueprintPure) FString GetDeckSlotName() const;`
- `UFUNCTION(BlueprintPure) FString GetSettingsSlotName() const;`
- `UPROPERTY(BlueprintAssignable) FOnSiegeActiveProfileChanged OnActiveProfileChanged;` + `DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSiegeActiveProfileChanged);`
- `static FString MakeCredentialHashHex(const FString& Password, const FString& SaltHex);`
- `static FString MakeProfileSlotSuffix(const FGuid& ProfileId);`
- `void SetSlotNameForAutomationTests(const FString& InSlotName);`

UFUNCTION/UPROPERTY specifiers kept EXACTLY as the registry writes them (no added `Category=` metadata — the registry is character-for-character law and five siblings compile against it; the functions will show uncategorized in a BP palette until the law says otherwise).

Cross-task symbols consumed: `FSiegeProfileInfo`, `USiegeAccountSaveGame::SlotName/UserIndex/Profiles/ActiveProfileId` (TASK-599 — its files had already landed; I verified both statics are DEFINED in its .cpp, so linking is safe) · `USiegeDeckSaveGame::SlotName/UserIndex` · `USiegeSettingsSubsystem::SettingsSlotName/SettingsUserIndex`. `ACC-§6` rows 3–5 honored: `USiegeAccountSubsystem`, `LogSiegeAccount` (declared .h / defined .cpp), `FOnSiegeActiveProfileChanged` member `OnActiveProfileChanged`.

## What was built (spec points 1–3)

- **Initialize** — `Super::Initialize` then ONE load of the registry slot via `LoadAccountsFromSlot()`. Missing/unreadable/foreign-class slot ⇒ empty registry + guest, logged ONCE on `LogSiegeAccount` (latched, Log not Warning — a first run is not a fault), never a crash. A persisted `ActiveProfileId` matching no profile is sanitized to guest (Warning), in memory only — the load path never writes disk.
- **CreateAccount** — first-violation `OutReason` in deterministic order (the `IsDeckLegal` idiom, `OutReason.Reset()` on success): trimmed name 3–24 chars → password ≥ 4 chars → case-insensitive uniqueness. Then: fresh `FGuid` ProfileId + fresh Digits-hex salt, hash per `ACC-§2`, register, set active, save registry, **SEED-COPY (RULING 8)**, broadcast — the spec's order character-for-character.
- **Login** — trimmed, case-insensitive lookup; salted-hash compare through the ONE hash implementation; sets active + `LastLoginUtc`, saves, broadcasts.
- **Logout** — clear active, save, broadcast; a Logout while already guest is a full no-op (no disk write, NO broadcast — the delegate law).
- **The seam** — `GetDeckSlotName()`/`GetSettingsSlotName()`: guest ⇒ the bare shipped constants byte-identical; active ⇒ `<base>_<MakeProfileSlotSuffix>` BUILT FROM the constants. Pure in-memory, safe at any call site.
- **Seed-copy mechanics** — loads the guest slot as a plain `USaveGame*` and re-saves the same object under the profile slot (`SaveGameToSlot` serializes the concrete class), so no Deck/Settings TYPE is named and the `ACC-§4` dependency direction holds. Missing guest slot = silent (fresh install); failed copy = Warning, never a failed CreateAccount; guest originals are read, never written.
- **Persistence model** — load-once / save-on-change (settings-lane contract cloned): failed writes log a Warning and are deliberately NOT reverted in memory.
- **Cloned automation surface** (additive beyond the registry, the `USiegeSettingsSubsystem` precedent): public `LoadAccountsFromSlot()` (reload after redirect), `ActiveProfileChangedBroadcastCount` (bumped ONLY inside `BroadcastActiveProfileChanged`, the single `.Broadcast` call site — TASK-604's observation point).

## ⛔ ACC-§2 honesty statement (RULING 3, stated plainly)

**Phase-1 credential handling is a local stub by design — a convenience, NOT security.** What ships: `hex(FSHA1::HashBuffer(UTF8(SaltHex + ":" + Password)))`, salt = per-profile `FGuid` Digits hex. It stops shoulder-surfing and accidental plaintext on disk and nothing more; anyone with disk access can edit `SiegeAccounts.sav` (replace a hash, blank a salt) and walk into any local profile. There is no rate limit, no lockout, no recovery. Real auth is the Phase-2 backend's job (server-side bcrypt via Supabase GoTrue). Nothing in the code or this handoff calls it "secure"/"encrypted"/"protected".

## QA self-audit (run at the artifact, results verbatim)

1. **`ACC-§2` password grep** — `Password` in `SiegeAccountSubsystem.cpp` appears ONLY in: the two registry signatures, `Password.Len() < 4`, a rule-stating message that echoes no value, the two `MakeCredentialHashHex` calls, and the transient `Combined` hash buffer. **ZERO writes, ZERO logs, ZERO OutReason echoes of the value.** No member stores it.
2. **`ACC-§4` bare-slot-literal grep** — `"Siege(Decks|Settings|Accounts)` over both my files: **ZERO matches** (code AND comments). Every slot name is built from `USiegeDeckSaveGame::SlotName`, `USiegeSettingsSubsystem::SettingsSlotName`, or `USiegeAccountSaveGame::SlotName`.
3. **`SC-§33` trailing-default audit** — **NO new trailing-defaulted parameter exists in these files** (every function's parameter list is default-free), so the enumerated call-site audit is owed and satisfied vacuously: zero defaulted signatures ⇒ zero call sites to enumerate.
4. **Engine signatures read from the installed 5.8 headers** (no compile allowed): `FSHA1::HashBuffer(const void*, uint64, uint8*)` (`SecureHash.h:366`) · `enum {DigestSize=20}` (`:317`) · `FString BytesToHex(const uint8*, int32)` (`Containers/UnrealString.h:103` — note: uppercase hex; consistent because both create and login go through the one implementation).

## Decisions QA should scrutinize (judgment calls, with reasoning)

- **Login failure reasons are DISTINCT** ("No profile named 'X'…" vs "Wrong password for 'X'."). Anti-enumeration vagueness would fake a security property `ACC-§2` explicitly disclaims (the profile list is world-readable on disk anyway); distinct reasons are the honest, friendlier local UX. Flag if the law is read otherwise — a one-line merge fixes it.
- **Login input is trimmed** before lookup (spec says "case-insensitive lookup"; trimming mirrors CreateAccount so a stray space cannot hide a profile). An empty trimmed name gets its own reason ("Enter a display name.").
- **`ActiveProfileId` persists in the registry slot** ⇒ a login survives a game restart (auto-resume). This is the model TASK-599 shipped (`ActiveProfileId` is `SaveGame`) and reads as intended design; guest remains the true default (absent/invalid ⇒ guest).
- **Re-login while already logged in** (same or different profile) always stamps `LastLoginUtc`, saves, broadcasts — a successful Login is a real state change; consumers (settings reload) tolerate a same-slot reload as a value-level no-op.
- **Seed-copy under `SlotNameOverride`**: the override redirects only the REGISTRY slot (the cloned settings seam contract). A CreateAccount during a test still reads the real guest slots and writes `SiegeDecks_<freshGuid>`/`SiegeSettings_<freshGuid>` — it can never OVERWRITE player data (fresh GUID = fresh slot) but does leave scratch profile-slot files. TASK-604 should prefer asserting on `MakeCredentialHashHex`/`MakeProfileSlotSuffix`/seam getters, or tolerate/clean the seeded files. Reported here rather than silently absorbed; if QA wants seed-copy suppressed while the override is active, it is a two-line guard.
- **`GetActiveProfileId()`/`IsLoggedIn()`/the seam all funnel through `FindActiveProfile()`** — a valid-but-unknown id structurally reads as guest everywhere at once.

## M8 declaration (batch header, verbatim)

Adds no replicated property, no new replicated class, no new relevancy tier, no RPC. All account state is client-local (`UGameInstanceSubsystem` + local `USaveGame`); the display name touches no session/player name (A7). Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.

## Notes for downstream

- **TASK-601** (settings lane): `ReloadForActiveProfile` should subscribe via `OnActiveProfileChanged` (`AddDynamic`); `GetSettingsSlotName()` is ready; `InitializeDependency<USiegeAccountSubsystem>` will find `SiegeAccountSubsystem.h` at `Siegebound/SiegeAccountSubsystem.h`.
- **TASK-604** (tests): the hermetic recipe is `SetSlotNameForAutomationTests(scratch)` → `LoadAccountsFromSlot()` (public for exactly this) → drive the flows; observe broadcasts via `ActiveProfileChangedBroadcastCount`; restore with an empty string. Statics `MakeCredentialHashHex`/`MakeProfileSlotSuffix` are instance-free.
- **TASK-606** (compile gate): new reflected type ⇒ full editor bounce per RULING 6/`SC-§26`; serialize against the floor-repair lane's gate per RULING 7.
- Handoff filename: board law says `handoffs/TASK-600-programmer.md` (this file); the dispatch prompt said `TASK-600.md` — board law wins, noted so nobody hunts for a missing file.
