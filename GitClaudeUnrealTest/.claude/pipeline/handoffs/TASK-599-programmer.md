# TASK-599 — [ACC-1] THE ACCOUNT MODEL — programmer handoff (2026-08-16)

## What shipped

The Phase-1 account/profile data model, exactly per the `ACC-§7` pinned registry, block 1. Model only — no subsystem logic, no widget, no hashing implementation (those are TASK-600/603's; this file carries data and doc-law).

## Files touched (both NEW — no existing file modified)

- `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSaveGame.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSaveGame.cpp`

## Registry conformance (`ACC-§7` block 1, character-for-character)

- `FSiegeProfileInfo` (USTRUCT, BlueprintType, no API macro — as pinned): `ProfileId` (FGuid, `SaveGame, BlueprintReadOnly`) · `DisplayName` (FString, `SaveGame, BlueprintReadOnly`) · `CredentialSaltHex` (FString, `SaveGame`) · `CredentialHashHex` (FString, `SaveGame`) · `CreatedUtc` / `LastLoginUtc` (FDateTime, `SaveGame, BlueprintReadOnly`). The registry's inline comments (`ACC-§2: convenience, NOT security` · `hex(SHA1(Salt + ":" + Password))`) are preserved in the field docs.
- `USiegeAccountSaveGame : USaveGame` (UCLASS, `GITCLAUDEUNREALTEST_API`): `static const FString SlotName` = `TEXT("SiegeAccounts")` · `static const int32 UserIndex` = `0` (both defined once in the .cpp — the `SiegeDeckSaveGame.cpp:7` one-definition idiom) · `UPROPERTY(SaveGame) TArray<FSiegeProfileInfo> Profiles` · `UPROPERTY(SaveGame) FGuid ActiveProfileId` (invalid GUID = guest, per `ACC-§1`/registry comment).

## Deliberate choices QA should scrutinize

1. **`UserIndex` is `static const int32` (declared in-class, defined in the .cpp), NOT the `static constexpr int32 UserIndex = 0;` shape `USiegeDeckSaveGame` ships.** The registry pins `static const int32 UserIndex;  // 0` character-for-character and the registry outranks the loose "SiegeDeckSaveGame idiom" phrase in the spec — five sibling tasks compile against the registry text. Either shape links fine at every `USiegeAccountSaveGame::UserIndex` consumer; if QA rules the constexpr shape was intended, it is a two-line change.
2. **`ACC-§2` honesty statement:** both the struct and class doc comments state the credential fields are a **local convenience credential, NOT security — real auth is the Phase-2 backend's job**, and that anyone with disk access can edit `SiegeAccounts.sav`. Grep check: the words "secure"/"encrypted"/"protected" appear NOWHERE in either file describing P1 auth.
3. **`ACC-§2` plaintext-password grep = trivially ZERO:** no function in this task takes a password parameter; there is no plaintext field in the model, by design (stated in the doc comment).
4. **`ACC-§4` bare-slot-literal grep stays clean:** the only quoted slot literal in either file is `TEXT("SiegeAccounts")` — the new registry constant's ONE definition site (`ACC-§3` table row 3). Doc-comment prose references the guest slots via their constant names (`USiegeDeckSaveGame::SlotName` / `USiegeSettingsSubsystem::SettingsSlotName`) and the composed forms unquoted (`SiegeDecks_<Digits>`), specifically so the gate's mechanical grep for `"SiegeDecks"` / `"SiegeSettings"` matches nothing here.
5. **No headers included beyond need:** `CoreMinimal.h` + `GameFramework/SaveGame.h` + generated. No Settings/Deck/Account-subsystem include (the `ACC-§4` dependency direction is moot here — the model includes nothing of theirs; the constant names above appear only in comments).

## Trailing-defaulted-parameter audit (standing `SC-§33` law)

**None.** This task adds zero functions, zero parameters, zero defaults — nothing to audit, and therefore no call-site grep is owed.

## Refused / not done (with the checkable mechanism)

- Nothing refused. Scope fences honored as written: no subsystem logic (checkable: the .cpp contains only the two static definitions — 10 lines), no hashing implementation (checkable: no `FSHA1` reference anywhere in the diff), no widget, no `.uasset`, no compile run, no git, no editor/MCP touch, no board rewrite (one targeted status Edit only).

## M8 DECLARATION (batch header, verbatim)

Adds no replicated property, no new replicated class, no new relevancy tier, no RPC. All account state is client-local (`UGameInstanceSubsystem` + local `USaveGame`); the display name touches no session/player name (A7). Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.

## Downstream

- TASK-600 (subsystem) compiles against these statics + `FSiegeProfileInfo` exactly as pinned — nothing here deviates from the registry it was written against.
- TASK-604's assertions on the composed slot names test TASK-600's `MakeProfileSlotSuffix`, not this file; this file intentionally hosts no derivation logic.
- Gate: `qa/TASK-605.md` (TASK-605 names 599 among the six).
