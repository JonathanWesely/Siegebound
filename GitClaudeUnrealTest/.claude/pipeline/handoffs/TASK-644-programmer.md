# TASK-644 — [ACC-P2-6] THE CLOUD-LINK MODEL — `FSiegeProfileInfo` fields + `USiegeAccountSubsystem` link API (gameplay-programmer handoff)

Date: 2026-08-23 · Status requested: **ready-for-qa** (board flip left to the orchestrator per dispatch — I did not edit TASKBOARD.md) · **ZERO compile (TASK-649 owns the lane's one — QUIET-MODULE; TASK-636's commit `1025160` verified landed and `Source/` clean before my first write)** · no editor/MCP · no git writes.

Law of record: `ACC-§15` blocks 3–4 (character-for-character) · `ACC-§11` token law · `ACC-§13` · P2-R6 · the delegate law (the shipped P1 header) · the trailing-defaulted-parameter law · the P1 versioning contract.

## 1. Files touched (complete list — nothing else)

| file | nature |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSaveGame.h` | +4 `FSiegeProfileInfo` fields (ACC-§15 block 3) + doc-comment amendments |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSubsystem.h` | +5 public methods (ACC-§15 block 4) + 1 private helper decl + doc-comment amendments |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSubsystem.cpp` | +162 lines, **0 deletions** — the five implementations + `FindActiveProfileMutable` |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSaveGame.cpp` | **ZERO edits** — the spec's `names:` block lists it, but the statics already defined there need no change and the new fields need no .cpp presence |

Diff shape, measured (`git diff --stat` + removed-line listing with a positive control): 271 insertions, **3 deletions total**, and all three deleted lines are doc-comment sentences I deliberately amended (the two registry-citation sentences + the `OnActiveProfileChanged` one-liner — each replaced by an extended version of itself). **No P1 executable line changed anywhere.** Other working-tree dirt (`Castle.*`, `Torch.h`, `.gitignore`, board) is parallel-lane work, not mine.

## 2. Registry conformance (ACC-§15, character-for-character)

**Block 3 — `FSiegeProfileInfo` additions, in registry order, appended after `LastLoginUtc`:**

- `UPROPERTY(SaveGame, BlueprintReadOnly) FString LinkedEmail;`
- `UPROPERTY(SaveGame) FString CloudUserId;`
- `UPROPERTY(SaveGame) FString CloudRefreshToken;`
- `UPROPERTY(SaveGame, BlueprintReadOnly) FDateTime LastSyncUtc;`

Formatting note (the P1 precedent, not a deviation): the registry writes specifier+type+name on one line; the shipped P1 file (TASK-599, qa-passed) renders every registry field as doc-comment + `UPROPERTY(...)` on its own line + declaration. I applied the identical transformation — specifiers, types, names, order are character-for-character.

**Block 4 — `USiegeAccountSubsystem` additions, verbatim (including the registry's alignment spacing):**

- `UFUNCTION(BlueprintPure) bool    IsCloudLinked() const;`
- `UFUNCTION(BlueprintPure) FString GetLinkedEmail() const;`
- `void SetCloudLink(const FString& Email, const FString& UserId, const FString& RefreshToken);`
- `void ClearCloudLink();`
- `void SetLastSyncUtc(const FDateTime& WhenUtc);`

The three mutators are non-UFUNCTION, exactly as pinned. `IsCloudLinked` implements the registry predicate verbatim: active profile AND `LinkedEmail` AND `CloudUserId` both non-empty (refresh token deliberately NOT in the predicate — an expired/absent token is still a link).

## 3. Back-compat statement (the SaveGame versioning law)

The P1 class comment's contract does the whole job: **tagged-property serialization** (the `USiegeSettingsSaveGame` precedent — no version int32 carried, none needed).

- **Old .sav → new binary:** a Phase-1 `SiegeAccounts.sav` simply lacks the four new tagged properties; each loads at its C++ default (empty `FString` ×3, unset `FDateTime`). A Phase-1 profile is therefore a valid, **UNLINKED** Phase-2 profile — `IsCloudLinked()` false, `GetLinkedEmail()` empty, sync clock "never synced" — with zero migration code.
- **New .sav → old binary (downgrade):** tagged serialization skips unknown properties safely; the P1 fields round-trip untouched.
- No field was renamed, removed, retyped, or reordered ahead of existing fields; the four additions sit at the END of the struct. `USiegeAccountSaveGame` itself (slot/user-index statics, `Profiles`, `ActiveProfileId`) is untouched.

## 4. Semantics implemented (and the declared decisions QA should rule on)

All three mutators follow the task spec's contract: **mutate ONLY the active profile → `SaveAccountsToSlot()` → `BroadcastActiveProfileChanged()`**, and the delegate law: a refused or valueless call saves nothing and broadcasts NOTHING. Guest ⇒ no-op + one log line for all five entry points (getters return the safe empties).

**Declared decisions (SC-§15 posture — implemented, flagged here for the 648 ruling):**

1. **`SetLastSyncUtc` BROADCASTS.** The TASK-644 spec (the authority) says all three mutators broadcast; ACC-§15's inline comment says only "saves registry" and the §15 footer names only `SetCloudLink`/`ClearCloudLink` as broadcasting. I followed the task spec. It is also load-bearing for the sync lane: after a `SyncNow` pull writes the settings SLOT, the broadcast is what makes `USiegeSettingsSubsystem::ReloadForActiveProfile` load the pulled data into memory. If QA rules the §15 comment binds instead, the fix is deleting one call.
2. **`ClearCloudLink` resets `LastSyncUtc` too** (the registry names no field set). Rationale in the code comment: a stale sync clock surviving into a future re-link would suppress the ACC-§13 trigger-1 pull for every cloud row with `updated_at` older than it — silently losing data on re-link. "Never synced" is the only honest state for an unlinked profile. The LOCAL profile (identity, display name, credential material, deck/settings slots) survives untouched — the ACC-§11 sign-out law and the 647 test pin.
3. **`SetCloudLink` input hardening:** trims Email/UserId; refuses (Warning, no mutation) if either is empty after trimming — a half-link would break the `IsCloudLinked` predicate while leaving a token on disk. An empty `RefreshToken` is ACCEPTED (not part of the predicate; a later sign-in refills it).
4. **Identical-values no-op guards** on `SetCloudLink` and `SetLastSyncUtc` (and already-unlinked on `ClearCloudLink`) — the delegate law ("never broadcast on a no-op") applied to the new mutators. Refresh-token ROTATION (same email/id, new token) is a real mutation and does save+broadcast.
5. **`FindActiveProfileMutable()` — new PRIVATE helper** (const-twin via `const_cast` on the const funnel), so exactly ONE active-profile predicate exists in the file and the P2 mutators cannot disagree with the P1 getters. Private ⇒ not part of any pinned surface.
6. **Doc-comment amendments to P1 text (the 3 deleted lines):** the `FOnSiegeActiveProfileChanged` comment + the two class/struct registry citations were EXTENDED with dated ⭐ sentences (the delegate's firing set now includes real cloud-link mutations; the pinned-registry citations now name ACC-§15 beside ACC-§7). Original sentences preserved inside the amended text; zero behavior.

## 5. Fences held

- **P2-R6:** `CredentialHashHex`/`CredentialSaltHex` untouched and NOT reachable through any new API (the getters return email/bool only; no new method reads either field). No password parameter exists in this task's surface. **The refresh token is NEVER logged** — the `SetCloudLink` log line prints `refresh token: held|none` only; no other line touches it. Self-grep of my diff for `CredentialHash|CredentialSalt|Password` in ADDED lines: the only hits are the P2-R6 fence comments themselves.
- **Guest path byte-identical:** `CreateAccount`/`Login`/`Logout`/seam/load/save bodies have a 0-line diff (§1's measured deletion list). New struct fields default-empty, changing no P1 serialization outcome for existing data. Guest calls into the new API are early-return no-ops.
- **No HTTP** (nothing from `HttpModule`/`Json` — 643's), **no sync logic** (645's), **no widget code** (646's). Zero new includes in all three files; the ACC-§4 dependency direction is unchanged (this class still includes no Cloud/Deck/Settings header from its .h; my additions required none — the QA-648 dependency-direction criterion: "Account includes no Cloud header beyond what 644's API requires" = it requires ZERO).
- **QUIET-MODULE:** no compile, no build artifacts, no editor, no git writes; wrote only the files in §1 + this handoff.

## 6. The trailing-defaulted-parameter law — discharged

**Zero new defaulted parameters introduced** (all five signatures are registry-pinned with none). Call-site audit is therefore vacuous, and additionally: pre-edit grep of `Source/` for all nine new names (`LinkedEmail|CloudUserId|CloudRefreshToken|LastSyncUtc|IsCloudLinked|GetLinkedEmail|SetCloudLink|ClearCloudLink|SetLastSyncUtc`) = **ZERO matches** — pure additions, no existing call site can change meaning. (Sibling tasks 645/646/647 author their call sites in parallel against the same registry.)

## 7. M8 declaration (P2 batch, verbatim)

**Adds no replicated property, no new replicated class, no new relevancy tier, no RPC.** All cloud traffic is client-local HTTPS from `USiegeCloudClient` (a `UGameInstanceSubsystem`); nothing crosses the UE networking layer. Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.

(Also placed as a comment block heading the P2 section in `SiegeAccountSubsystem.h`; the P1 M8 blocks are untouched.)

## 8. QA scrutiny list (where I would look first)

1. The §4 declared decisions, especially #1 (broadcast on `SetLastSyncUtc` — spec-vs-§15-comment) and #2 (`LastSyncUtc` reset in `ClearCloudLink`).
2. Character-for-character check of the five signatures + four fields against ACC-§15 (§2 above states the one formatting transformation, with the P1 precedent).
3. The delegate-law interaction: `BroadcastActiveProfileChanged()` (unchanged P1 code) logs "Active profile changed — now X" and bumps the shared counter for cloud-link mutations too — the ACTIVE PROFILE'S identity does not change on a link mutation, only its state; the delegate doc now says so. Confirm no P1 test pins the broadcast count across flows this could touch (my pre-edit grep says the new names appear nowhere in `Tests/`, and no P1 flow calls the new API — P1 broadcast counts are unchanged).
4. `const_cast` twin in `FindActiveProfileMutable` (§4.5) — sound because the storage is non-const, but worth an eyeball.
5. Serialization: four `SaveGame` fields inside a USTRUCT inside a `TArray` — tagged per-member, defaults on absence (§3). The settings-lane precedent is the citation.
6. Log lines: verify none can emit token material (the only line touching `CloudRefreshToken` prints a held/none literal).
