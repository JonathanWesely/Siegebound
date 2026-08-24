// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SiegeCloudClient.generated.h"

class FConfigFile;
class FJsonObject;

/**
 *  The cloud log category (CONVENTIONS ACC-§14 row 4: LogSiegeCloud is declared
 *  and defined in SiegeCloudClient.{h,cpp} by law — the LogSiegeSettings /
 *  LogSiegeAccount shape, cloned).
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeCloud, Log, All);

/**
 *  THE CLOUD CONFIG (ACC-§15 block 1, registry-pinned). Loaded from the
 *  gitignored Config/SiegeCloudDev.ini (ACC-§11 — the anon key's ONE ruled
 *  home; a hardcoded key in a .cpp is a QA FAIL, so no default value here ever
 *  carries a real URL or key).
 */
USTRUCT()
struct FSiegeCloudConfig
{
	GENERATED_BODY()

	UPROPERTY() FString ProjectUrl;   // https://<ref>.supabase.co  (ACC-§11 config home)
	UPROPERTY() FString AnonKey;      // publishable key — RLS is the boundary, key is public-by-design

	bool IsValid() const;             // both non-empty
};

/** Registry-pinned result delegate (ACC-§14 row 5): non-dynamic, C++-only. On success PayloadOrError is the raw response body (JSON); on failure it is a human-readable error that NEVER echoes a credential or token (P2-R6). */
DECLARE_DELEGATE_TwoParams(FSiegeCloudResult, bool /*bOk*/, const FString& /*PayloadOrError*/);

/** Registry-pinned cloud-state delegate (ACC-§14 row 6): fired on REAL auth-state transitions only (signed out -> signed in, signed in -> signed out) — never on a no-op (the delegate law). No payload: consumers re-read IsCloudAuthenticated / GetCloudUserId. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSiegeCloudStateChanged);

/**
 *  THE CLOUD CLIENT (batch ACCOUNTS PHASE 2, TASK-643; CONVENTIONS ACC-§10,
 *  ACC-§11, ACC-§14 rows 1–6, and the ACC-§15 block-1 pinned registry — the
 *  public surface below is registry-pinned character-for-character).
 *
 *  Owns the Supabase transport and NOTHING else: GoTrue auth
 *  (/auth/v1/signup · /auth/v1/token?grant_type=password ·
 *  /auth/v1/token?grant_type=refresh_token · /auth/v1/logout) and PostgREST
 *  row ops (GET + upsert-POST under /rest/v1/), via FHttpModule + Json ONLY —
 *  ⛔ no third-party SDK (ACC-§11). ⛔ No sync logic (FSiegeCloudSync,
 *  TASK-645), no UI (TASK-646), no account-registry write (TASK-644).
 *
 *  ⛔ THE CLOUD-OFF LAW (ACC-§11): a missing or incomplete
 *  Config/SiegeCloudDev.ini means cloud OFF — Initialize logs ONE
 *  LogSiegeCloud line, IsCloudConfigured() returns false, and every entry
 *  point fails fast through its delegate without touching the network. Cloud
 *  gates NOTHING; the game behaves byte-identically to Phase 1.
 *
 *  ⛔ THE TOKEN LAW (ACC-§11): the access token (JWT) lives in a PRIVATE
 *  MEMBER, memory only — NEVER logged, NEVER persisted by this class. The
 *  per-profile persisted refresh token is TASK-644's field on
 *  FSiegeProfileInfo; this class only passes refresh tokens through
 *  (RefreshSession parameter in, response payload out). ⛔ No password and no
 *  token ever appears in any log line or error string (P2-R6). Real
 *  credential hashing is server-side (GoTrue bcrypt); the P1 local hash/salt
 *  never touch this class.
 *
 *  ⛔ NOTHING BLOCKS ON HTTP (ACC-§11): every network entry point is
 *  delegate-async (FSiegeCloudResult fires on the game thread when the
 *  request completes or fails). There is no synchronous wait anywhere.
 *
 *  It is a UGameInstanceSubsystem because a cloud session must survive
 *  OpenLevel — the USiegeSettingsSubsystem / USiegeAccountSubsystem lesson,
 *  same clothes.
 *
 *  HOW CONSUMERS RESOLVE IT (the shipped subsystem snippet, cloned):
 *
 *      UGameInstance* GI = GetGameInstance();               // or Actor->GetGameInstance()
 *      USiegeCloudClient* Cloud =
 *          GI ? GI->GetSubsystem<USiegeCloudClient>() : nullptr;
 *      if (!Cloud || !Cloud->IsCloudConfigured()) { /... degrade to Phase-1 local behavior .../ }
 *
 *  ⛔ AND THE FALLBACK WHEN THE SUBSYSTEM DOES NOT RESOLVE OR IS UNCONFIGURED
 *  IS THE LOCAL PHASE-1 BEHAVIOR — one LogSiegeCloud line, a CloudStatusText
 *  message where UI is involved, never a crash, never a block (ACC-§11).
 *
 *  RECORDED LIMITATIONS (⛔ not bugs): the access token expires server-side
 *  (~1 h); this class does NOT auto-refresh — callers drive RefreshSession
 *  with the persisted refresh token (the 644/645/646 lanes). Until Jonathan's
 *  S2 dashboard hand-step flips "Confirm email" OFF, a SignUp succeeds
 *  WITHOUT a session (GoTrue returns no access_token until the email is
 *  confirmed) — expected live-test behavior, never a code bug
 *  (handoffs/TASK-641-buildmaster.md §5.2).
 *
 *  M8 DECLARATION (batch header, verbatim): Adds no replicated property, no
 *  new replicated class, no new relevancy tier, no RPC. All cloud traffic is
 *  client-local HTTPS from USiegeCloudClient (a UGameInstanceSubsystem);
 *  nothing crosses the UE networking layer. Does NOT consume the M8 Phase-1
 *  checkpoint gate; does NOT substitute for Jonathan's owed feedback items.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeCloudClient : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	//~ Begin USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override; // loads Config/SiegeCloudDev.ini; missing => disabled, logged once
	//~ End USubsystem interface

	UFUNCTION(BlueprintPure) bool    IsCloudConfigured() const;
	UFUNCTION(BlueprintPure) bool    IsCloudAuthenticated() const;          // access token held (memory only — ACC-§11)
	UFUNCTION(BlueprintPure) FString GetCloudUserId() const;                // empty when signed out

	/**
	 *  GoTrue POST /auth/v1/signup with {"email","password"}. The typed
	 *  password goes ONLY into the HTTPS request body (ACC-§11: the P1 local
	 *  hash never uploads; this is the fresh-typed-password lane) and is never
	 *  stored or logged here. If the response carries a session (email
	 *  confirmation OFF — the A5/S2 config) it is adopted and
	 *  OnCloudStateChanged fires; without one, OnDone still reports success
	 *  and the caller surfaces "confirm your email" via CloudStatusText.
	 */
	void SignUp        (const FString& Email, const FString& Password, FSiegeCloudResult OnDone);

	/** GoTrue POST /auth/v1/token?grant_type=password with {"email","password"}. On success adopts the session (access token + user id in memory, refresh token in the success payload for TASK-644's SetCloudLink) and broadcasts the state change. */
	void SignIn        (const FString& Email, const FString& Password, FSiegeCloudResult OnDone);

	/** GoTrue POST /auth/v1/token?grant_type=refresh_token with {"refresh_token"}. The caller supplies the persisted token (FSiegeProfileInfo::CloudRefreshToken via TASK-644's API); on success the fresh session is adopted exactly like SignIn. */
	void RefreshSession(const FString& RefreshToken,                   FSiegeCloudResult OnDone);

	void SignOut();                                                         // clears in-memory tokens; fire-and-forget /logout

	/**
	 *  PostgREST GET. QuerySuffix is appended verbatim after '?' (empty =>
	 *  no '?'), e.g. "select=deck_name,payload,updated_at". Requires a held
	 *  session: the live policies are scoped `to authenticated` (ACC-§12), so
	 *  an unauthenticated call fails FAST and LOCALLY through OnDone instead
	 *  of wasting a round trip the server would deny anyway.
	 */
	void FetchRows (const FString& Table, const FString& QuerySuffix, FSiegeCloudResult OnDone); // GET  /rest/v1/<Table>?<QuerySuffix>

	/**
	 *  PostgREST upsert. Table is appended VERBATIM after /rest/v1/ — a caller
	 *  needing an alternate conflict target passes it inline (e.g.
	 *  "decks?on_conflict=user_id,deck_name" — the ACC-§13 per-deck-row lane;
	 *  merge-duplicates alone resolves on the PRIMARY KEY). JsonBody is sent
	 *  as-is (TASK-645's Make*RowJson builds it; ⛔ it never carries
	 *  updated_at — server-owned, ACC-§12). Requires a held session, as above.
	 */
	void UpsertRow (const FString& Table, const FString& JsonBody,    FSiegeCloudResult OnDone); // POST /rest/v1/<Table>, Prefer: resolution=merge-duplicates

	UPROPERTY(BlueprintAssignable) FOnSiegeCloudStateChanged OnCloudStateChanged;

	/**
	 *  THE CONFIG-LOAD SEAM, file half (ACC-§11; static + side-effect-free on
	 *  the class so TASK-647's offline tests can drive it without a subsystem
	 *  instance). Resets OutConfig, reads IniFilePath via the
	 *  FConfigCacheIni-family FConfigFile, and extracts the [SiegeCloud]
	 *  section. Returns OutConfig.IsValid(). A missing file returns false
	 *  with OutConfig left empty — ⛔ no partial values ever linger.
	 */
	static bool LoadCloudConfigFile(const FString& IniFilePath, FSiegeCloudConfig& OutConfig);

	/**
	 *  THE CONFIG-LOAD SEAM, parse half — PURE: [SiegeCloud] ProjectUrl= +
	 *  AnonKey= out of an already-populated FConfigFile (TASK-647 builds one
	 *  from a scratch config string; no disk needed). Values are trimmed and
	 *  ProjectUrl loses any trailing '/' so endpoint concatenation cannot
	 *  produce '//'. The `; DbPassword=` custody comment line is invisible to
	 *  FConfigFile and is NEVER read (ACC-§11: the game never reads it).
	 *  Returns OutConfig.IsValid().
	 */
	static bool ExtractCloudConfig(const FConfigFile& ConfigFile, FSiegeCloudConfig& OutConfig);

	/**
	 *  DIAGNOSTICS + THE AUTOMATION TESTS' OBSERVATION POINT (the
	 *  SettingsChangeBroadcastCount precedent, cloned): how many times
	 *  OnCloudStateChanged has been broadcast this session. Bumped inside
	 *  BroadcastIfAuthStateChanged(), the ONLY place in the .cpp that calls
	 *  OnCloudStateChanged.Broadcast — so the count cannot diverge from the
	 *  broadcasts. Read-only for consumers; no logic anywhere reads it.
	 */
	int32 CloudStateChangedBroadcastCount = 0;

private:

	/** POSTs a JSON body to a GoTrue endpoint (path + query, e.g. "/auth/v1/token?grant_type=password"), adopts any returned session, broadcasts on a real auth-state transition, then forwards the result. The ONE auth-request path — SignUp / SignIn / RefreshSession cannot drift. */
	void SendAuthRequest(const FString& AuthPathAndQuery, const TSharedRef<FJsonObject>& Body, FSiegeCloudResult OnDone);

	/** Parses a GoTrue response body; if it carries access_token, adopts it (+ refresh_token + user.id) into the private members and returns true. A body without a session (signup pending email confirmation, error shapes) returns false and touches nothing. ⛔ Never logs any token (P2-R6). */
	bool AdoptSessionFromAuthResponse(const FString& ResponseBody);

	/** THE ONLY caller of OnCloudStateChanged.Broadcast — compares the current IsCloudAuthenticated() against the captured before-state, broadcasts + bumps the diagnostics counter on a REAL transition, stays silent on a no-op (the delegate law). */
	void BroadcastIfAuthStateChanged(bool bWasAuthenticated);

	/** The loaded ACC-§11 config. Empty (IsValid() == false) when Config/SiegeCloudDev.ini is missing or incomplete — the cloud-off state. */
	FSiegeCloudConfig CloudConfig;

	/** ⛔ MEMORY ONLY (ACC-§11): the GoTrue access JWT. Never logged, never persisted, cleared by SignOut. Empty == signed out. */
	FString AccessToken;

	/** ⛔ MEMORY ONLY: the most recent refresh token from GoTrue, held so SignOut can clear "in-memory tokens" plural per the registry. Persistence is EXCLUSIVELY TASK-644's FSiegeProfileInfo::CloudRefreshToken — this class never writes any save. */
	FString InMemoryRefreshToken;

	/** The authenticated auth.users id (uuid string); empty when signed out. Mirrors GetCloudUserId(). */
	FString CloudUserId;
};
