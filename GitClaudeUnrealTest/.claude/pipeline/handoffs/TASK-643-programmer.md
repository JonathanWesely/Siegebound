# TASK-643 — [ACC-P2-5] `USiegeCloudClient` — config load, GoTrue auth, PostgREST rows — programmer handoff

- **Date:** 2026-08-23 · **Status:** ready-for-qa (stated here; the orchestrator flips the board — no TASKBOARD edit by this task)
- **Authored against:** CONVENTIONS `ACC-§10..§15` (block 1 of the `ACC-§15` registry, character-for-character) · `handoffs/TASK-641-buildmaster.md` (config home + key custody) · `handoffs/TASK-642-buildmaster.md` (live schema/policy shapes) · the `SiegeSettingsSubsystem`/`SiegeAccountSubsystem` idiom.
- ⛔ Fences held: ZERO compile (TASK-649 owns the lane's one — this diff sits unbuilt) · no editor/MCP · no git writes · no network calls at authoring time · no secrets/keys/project-refs in C++ (grep proof §5).

## 1. Files

| file | kind |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCloudClient.h` | **NEW** — registry block 1 + doc law |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCloudClient.cpp` | **NEW** — implementation |
| `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs` | **EDIT — DECLARED DEVIATION D1 (§3)**: `"HTTP"` added to PublicDependencyModuleNames (one token + comment block) |

## 2. Registry-conformance table (`ACC-§15` block 1 — character-for-character)

| pinned item | where | conformance |
|---|---|---|
| `USTRUCT() struct FSiegeCloudConfig` + `UPROPERTY() FString ProjectUrl` + `UPROPERTY() FString AnonKey` + `bool IsValid() const` (both non-empty) | .h; IsValid defined in .cpp | EXACT (registry's inline comments carried verbatim) |
| `DECLARE_DELEGATE_TwoParams(FSiegeCloudResult, bool /*bOk*/, const FString& /*PayloadOrError*/);` | .h | EXACT |
| `DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSiegeCloudStateChanged);` | .h | EXACT |
| `UCLASS() class GITCLAUDEUNREALTEST_API USiegeCloudClient : public UGameInstanceSubsystem` | .h | EXACT |
| `virtual void Initialize(FSubsystemCollectionBase& Collection) override;` — loads `Config/SiegeCloudDev.ini`; missing => disabled, logged once | .h/.cpp | EXACT — `FPaths::ProjectConfigDir() + SiegeCloudDev.ini`, local `FConfigFile` (FConfigCacheIni-family), ONE `LogSiegeCloud` line on cloud-off |
| `UFUNCTION(BlueprintPure) bool IsCloudConfigured() const;` | .h/.cpp | EXACT — returns `CloudConfig.IsValid()` (one source of truth, no shadow bool) |
| `UFUNCTION(BlueprintPure) bool IsCloudAuthenticated() const;` | .h/.cpp | EXACT — `!AccessToken.IsEmpty()`, memory only |
| `UFUNCTION(BlueprintPure) FString GetCloudUserId() const;` | .h/.cpp | EXACT — empty when signed out; cleared by SignOut |
| `void SignUp(const FString& Email, const FString& Password, FSiegeCloudResult OnDone);` | .h/.cpp | EXACT — POST `/auth/v1/signup` |
| `void SignIn(const FString& Email, const FString& Password, FSiegeCloudResult OnDone);` | .h/.cpp | EXACT — POST `/auth/v1/token?grant_type=password` |
| `void RefreshSession(const FString& RefreshToken, FSiegeCloudResult OnDone);` | .h/.cpp | EXACT — POST `/auth/v1/token?grant_type=refresh_token` |
| `void SignOut();` — clears in-memory tokens; fire-and-forget `/logout` | .h/.cpp | EXACT — no-op (no HTTP, no broadcast) while signed out; else unbound-completion POST `/auth/v1/logout`, unconditional local clear, broadcast |
| `void FetchRows(const FString& Table, const FString& QuerySuffix, FSiegeCloudResult OnDone);` — GET `/rest/v1/<Table>?<QuerySuffix>` | .h/.cpp | EXACT — suffix verbatim after `?`; empty suffix appends no `?` |
| `void UpsertRow(const FString& Table, const FString& JsonBody, FSiegeCloudResult OnDone);` — POST `/rest/v1/<Table>`, `Prefer: resolution=merge-duplicates` | .h/.cpp | EXACT — header literal exactly as pinned; Table appended VERBATIM (§4 note 1) |
| `UPROPERTY(BlueprintAssignable) FOnSiegeCloudStateChanged OnCloudStateChanged;` | .h | EXACT — broadcast ONLY from `BroadcastIfAuthStateChanged()` on real transitions (delegate law) |
| Headers `apikey` + `Authorization: Bearer <token>` (spec (1)) | .cpp `MakeCloudRequest` | apikey = anon key always; Bearer = access token when held, else anon key (§4 note 2) |
| `FHttpModule` + `Json` only, no third-party SDK | .cpp includes | HELD — engine modules only |
| `ACC-§14` rows 1–6 names (`USiegeCloudClient`, `FSiegeCloudConfig`, `LogSiegeCloud` declared/defined in this pair, `FSiegeCloudResult`, `FOnSiegeCloudStateChanged`/`OnCloudStateChanged`) | .h/.cpp | EXACT |

## 3. Declared deviations + additions (`SC-§15` — declared, never silent)

- **D1 — `GitClaudeUnrealTest.Build.cs` gains `"HTTP"` (the ONE out-of-assignment file).** My `names:` block assigns only `SiegeCloudClient.h/.cpp`, but the module has NO `HTTP` dependency and `ACC-§11` pins `FHttpModule` as the transport — without this token the lane's one compile (TASK-649) fails at `#include "HttpModule.h"`, forcing exactly the compile-fix/`SC-§27` cycle the batch is built to avoid. Pre-flight `git status --porcelain` on the file: CLEAN (no parked work merged). Ownership model = the TASK-417 precedent (`Json`/`JsonUtilities` landed with their first user). ⚠️ The file's standing "NOT HTTP, NOT Sockets" comment is the **LLM-assistant** closed decision (a rejected LISTENING sidecar); the new comment block reconciles in place: that ruling stands byte-for-byte (llama stays in-process), `FHttpModule` is outbound-only client HTTPS under the later `ACC-§10/§11` law, and **`"Sockets"` remains absent**. QA-648: please rule this deviation explicitly.
- **A1 — config seam statics** `LoadCloudConfigFile(IniFilePath, OutConfig)` + `ExtractCloudConfig(const FConfigFile&, OutConfig)` (public static). Not registry-pinned; added because TASK-647's spec requires "ini-parse from a scratch config string" offline and the registry pins no seam for it. `ExtractCloudConfig` is PURE over an `FConfigFile` (647 builds one from a string; no disk). ⚠️ 647 is authored in parallel and may have guessed different names — if so, reconcile at 648 in MY files (these are additive; nothing else references them).
- **A2 — `CloudStateChangedBroadcastCount`** public diagnostics member — the `SettingsChangeBroadcastCount`/`ActiveProfileChangedBroadcastCount` idiom, cloned; bumped only inside the single broadcast site.
- **A3 — fast-fail guards:** every entry point checks `IsCloudConfigured()` (the `ACC-§11` cloud-off law) and the row ops additionally check `IsCloudAuthenticated()` — the live policies are `to authenticated` (`ACC-§12`, `qa/TASK-642` §3.2: anon holds zero policies), so an unauthenticated round trip would be denied server-side anyway; failing locally through `OnDone` is honest and never blocks.
- **A4 — value normalization in `ExtractCloudConfig`:** trim + strip trailing `/` from `ProjectUrl` (prevents `//` in endpoint concatenation). `IsValid()` itself stays registry-pure (both non-empty, nothing else) — 647's truth table is unaffected.
- **A5 — `InMemoryRefreshToken` private member:** the registry's `SignOut` comment says "clears in-memory tokens" (plural), so the most recent refresh token is held in memory alongside the access token and cleared with it. ⛔ Never persisted here, never logged — persistence is exclusively TASK-644's `FSiegeProfileInfo::CloudRefreshToken`.
- **A6 — internals:** `LogSiegeCloud` (mandated, `ACC-§14` row 4) · private `SendAuthRequest`/`AdoptSessionFromAuthResponse`/`BroadcastIfAuthStateChanged` · anonymous-namespace helpers (`MakeCloudRequest`, `IsHttpSuccess`, `UrlSansQuery`, `DescribeHttpFailure`, `LogHttpFailure`).

## 4. Cross-task notes (for TASK-645/646/647 and the 648 gate)

1. **The upsert conflict target rides inside `Table`.** `Prefer: resolution=merge-duplicates` alone resolves on the PRIMARY KEY; the `decks` conflict target is `unique (user_id, deck_name)` (`ACC-§12`; `handoffs/TASK-642` §5 pins it as the smoke's contract). `UpsertRow` appends `Table` VERBATIM after `/rest/v1/`, so TASK-645 passes `decks?on_conflict=user_id,deck_name` for deck rows (plain `settings` upserts on its PK `user_id`, no suffix needed). Documented on the method's doc comment.
2. **Bearer on auth endpoints:** anon key before a session exists, held access token afterwards (GoTrue accepts either alongside `apikey`); `/logout` runs before the clear so it carries the access token. `MakeCloudRequest` takes the bearer explicitly.
3. **Success payload = raw response body.** SignIn/SignUp/RefreshSession callers (644-link flow driven by 646) extract `refresh_token` and `user.id` from the payload; `GetCloudUserId()` also serves the id from memory. ⛔ The payload is a delegate argument, never a log line.
4. **SignUp without a session** (no `access_token` in a 200 body) is the NORMAL shape until Jonathan's S2 dashboard hand-step flips "Confirm email" OFF (`handoffs/TASK-641` §5.2): `OnDone` reports success, state stays signed-out, no broadcast — 646 should surface "confirm your email" from the payload when `IsCloudAuthenticated()` is still false.
5. **No auto-refresh** of the ~1 h access-token expiry — recorded limitation on the class doc; callers drive `RefreshSession` with the persisted refresh token.
6. **Ordering:** on auth completion the state broadcast fires BEFORE `OnDone`, so state listeners are fresh when the result handler reads the getters.

## 5. Self-check greps (P2-R2 / P2-R6, with the `SC-§14` positive control)

Scope = the three touched files (`SiegeCloudClient.h`, `SiegeCloudClient.cpp`, `GitClaudeUnrealTest.Build.cs`), ripgrep:

| pattern | hits | verdict |
|---|---|---|
| `service_role\|sb_secret\|eyJ\|CredentialHashHex\|CredentialSaltHex` | **0** | P2-R2 + P2-R6 ZERO — no key material, no JWT, no P1 hash/salt reference anywhere in the diff |
| `cjgqqeogsynrphowdcdp` | **0** | no project ref in code — config-loaded only (`ACC-§11`) |
| `supabase.co` | 3 | all benign: the registry's own pinned placeholder comment (`https://<ref>.supabase.co`) + the Norton `*.supabase.co` hand-step note (comment + error literal) — no live URL |
| **positive control:** `AnonKey` | **8** (h:2, cpp:6) | the grep instrument demonstrably fires; all hits are the config FIELD identifier, zero literal key material |

**Log-hygiene review (P2-R6, by hand):** the complete `UE_LOG` set is — configured-URL line · cloud-off line · signed-out line · auth-state-transition line · session-adopted line (auth.users id only) · `LogHttpFailure` (verb + query-stripped URL + status code). ⛔ No password, no token, no email, no request/response body ever reaches a log. Error payloads forward server error bodies (GoTrue/PostgREST never echo credentials) or fixed local strings.

## 6. Trailing-default law (`SC-§33`)

**ZERO defaulted parameters added** — every signature is registry-verbatim and default-free; all functions are NEW (no existing call sites exist). No call-site audit is owed; stated per the law.

## 7. M8 declaration (verbatim)

**Adds no replicated property, no new replicated class, no new relevancy tier, no RPC.** All cloud traffic is client-local HTTPS from `USiegeCloudClient` (a `UGameInstanceSubsystem`); nothing crosses the UE networking layer. Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.

## 8. What TASK-648's reviewer should scrutinize

1. **D1 (the Build.cs token)** — the one out-of-assignment edit; rule it (§3). Verify the diff is exactly one dependency token + comments, `"Sockets"` still absent.
2. **Registry conformance** §2 against `ACC-§15` block 1 directly (not this table).
3. **Cloud-off byte-identity argument (`ACC-§11`):** the subsystem auto-instantiates, but with no ini it loads nothing, logs ONE line, and no shipped file references `USiegeCloudClient` yet (644/645/646 add consumers) — zero behavioral delta to any Phase-1 path. Dependency direction: this class includes NO Deck/Settings/Account header.
4. **UE 5.8 API surface I could not compile against:** `FConfigFile::Read`/`GetString(const TCHAR*, const TCHAR*, FString&) const` · `TDelegate::BindWeakLambda` · `EHttpResponseCodes::IsOk` · `FJsonObject::TryGetStringField/TryGetObjectField` — all standard 5.x; if 5.8 moved any, TASK-649 sees it first.
5. **Lifetime/threading:** all completions `BindWeakLambda(this, …)` (shutdown-safe skip); FHttp completions arrive on the game thread — no synchronization added, deliberately.
6. **The Norton hand-step sentence inside the no-response error literal** — dev-honest per `ACC-§10`'s environmental-first rule; if ruled too machine-specific for a player-facing string, trimming it is a one-line polish (Phase-3 candidate), not a blocker I'd contest.
7. **`SignOut` fire-and-forget** deliberately binds NO completion delegate; local clear is unconditional — check that reading matches the registry comment.
