// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeCloudClient.h"

#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Subsystems/SubsystemCollection.h"

// The ONE definition of the cloud log category (ACC-§14 row 4 — LogSiegeCloud
// lives in SiegeCloudClient.{h,cpp} by law).
DEFINE_LOG_CATEGORY(LogSiegeCloud);

bool FSiegeCloudConfig::IsValid() const
{
	// Registry-pinned semantics (ACC-§15 block 1): both non-empty. TASK-647
	// pins the truth table offline.
	return !ProjectUrl.IsEmpty() && !AnonKey.IsEmpty();
}

namespace
{
	/**
	 *  Builds a request with the two ACC-§11 headers every Supabase call
	 *  carries: apikey (always the anon key) and Authorization: Bearer
	 *  <BearerToken> (the anon key before a session exists, the user's access
	 *  JWT once one does — RLS keys on the JWT, ACC-§12).
	 */
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> MakeCloudRequest(
		const FSiegeCloudConfig& CloudConfig, const FString& BearerToken, const FString& Url, const FString& Verb)
	{
		const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
		Request->SetURL(Url);
		Request->SetVerb(Verb);
		Request->SetHeader(TEXT("apikey"), CloudConfig.AnonKey);
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *BearerToken));
		return Request;
	}

	/** One success definition for every request: transport OK + a response + a 2xx code. */
	bool IsHttpSuccess(const FHttpResponsePtr& HttpResponse, bool bConnectedOk)
	{
		return bConnectedOk && HttpResponse.IsValid() && EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode());
	}

	/** The request URL with any query string stripped — the only URL shape that may reach a log line (P2-R6 hygiene: filters carry row ids; queries stay out of logs). */
	FString UrlSansQuery(const FHttpRequestPtr& HttpRequest)
	{
		if (!HttpRequest.IsValid())
		{
			return FString(TEXT("<no request>"));
		}
		FString Url = HttpRequest->GetURL();
		int32 QueryIndex = INDEX_NONE;
		if (Url.FindChar(TEXT('?'), QueryIndex))
		{
			Url.LeftInline(QueryIndex);
		}
		return Url;
	}

	/**
	 *  The FSiegeCloudResult error payload for a failed request. ⛔ Never a
	 *  credential, never a token: a no-response failure is a fixed local
	 *  string; a non-2xx failure forwards the server's error body (GoTrue /
	 *  PostgREST error JSON — the server never echoes a password) so
	 *  CloudStatusText can say something true (ACC-§13).
	 */
	FString DescribeHttpFailure(const FHttpResponsePtr& HttpResponse, bool bConnectedOk)
	{
		if (!bConnectedOk || !HttpResponse.IsValid())
		{
			// The recorded environmental suspect FIRST (ACC-§10: Norton MITMs
			// HTTPS on this machine until the *.supabase.co exclusion, S3).
			return FString(TEXT("Cloud request got no response (network/TLS failure; on this machine check the Norton *.supabase.co exclusion, hand-step S3)."));
		}
		const FString Body = HttpResponse->GetContentAsString();
		if (!Body.IsEmpty())
		{
			return Body;
		}
		return FString::Printf(TEXT("Cloud request failed: HTTP %d."), HttpResponse->GetResponseCode());
	}

	/** ONE Warning per failed request: verb + query-stripped URL + status. ⛔ Never the body, never a header, never a token (P2-R6). */
	void LogHttpFailure(const FHttpRequestPtr& HttpRequest, const FHttpResponsePtr& HttpResponse)
	{
		UE_LOG(LogSiegeCloud, Warning, TEXT("[SiegeCloud] %s %s failed (HTTP %d)."),
			HttpRequest.IsValid() ? *HttpRequest->GetVerb() : TEXT("<none>"),
			*UrlSansQuery(HttpRequest),
			HttpResponse.IsValid() ? HttpResponse->GetResponseCode() : 0);
	}
}

void USiegeCloudClient::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// LOAD ONCE, at Initialize — the config is never re-read on a gameplay
	// path (the settings-lane contract, same clothes). The ini is GITIGNORED
	// (ACC-§11): on a machine without it the cloud is simply OFF.
	const FString IniFilePath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("SiegeCloudDev.ini"));
	if (LoadCloudConfigFile(IniFilePath, CloudConfig))
	{
		// ⛔ The URL is loggable (it is in every request); the anon key is
		// NOT logged — not because it is secret (it is public-by-design,
		// ACC-§11) but because tokens-in-logs is the habit P2-R6 exists to
		// prevent.
		UE_LOG(LogSiegeCloud, Log, TEXT("[SiegeCloud] Cloud configured: %s."), *CloudConfig.ProjectUrl);
	}
	else
	{
		// THE ONE cloud-off line (ACC-§11). Log, not Warning: a machine
		// without the dev ini is a legitimate state, and the game behaves
		// byte-identically to Phase 1 from here on.
		UE_LOG(LogSiegeCloud, Log, TEXT("[SiegeCloud] Cloud is OFF: '%s' missing or incomplete — local Phase-1 behavior only."), *IniFilePath);
	}
}

bool USiegeCloudClient::IsCloudConfigured() const
{
	// ONE source of truth — the loaded config itself; no shadow bool to drift.
	return CloudConfig.IsValid();
}

bool USiegeCloudClient::IsCloudAuthenticated() const
{
	// Registry semantics: access token held (memory only — ACC-§11).
	return !AccessToken.IsEmpty();
}

FString USiegeCloudClient::GetCloudUserId() const
{
	// Registry semantics: empty when signed out.
	return CloudUserId;
}

void USiegeCloudClient::SignUp(const FString& Email, const FString& Password, FSiegeCloudResult OnDone)
{
	// The fresh-typed-password lane (ACC-§11: the P1 local hash NEVER
	// uploads). The password's only destination is the HTTPS body below.
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("email"), Email);
	Body->SetStringField(TEXT("password"), Password);
	SendAuthRequest(TEXT("/auth/v1/signup"), Body, OnDone);
}

void USiegeCloudClient::SignIn(const FString& Email, const FString& Password, FSiegeCloudResult OnDone)
{
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("email"), Email);
	Body->SetStringField(TEXT("password"), Password);
	SendAuthRequest(TEXT("/auth/v1/token?grant_type=password"), Body, OnDone);
}

void USiegeCloudClient::RefreshSession(const FString& RefreshToken, FSiegeCloudResult OnDone)
{
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("refresh_token"), RefreshToken);
	SendAuthRequest(TEXT("/auth/v1/token?grant_type=refresh_token"), Body, OnDone);
}

void USiegeCloudClient::SignOut()
{
	if (!IsCloudAuthenticated())
	{
		// Complete no-op while signed out: no HTTP, no clear, NO BROADCAST
		// (the delegate law — the Logout-while-guest precedent).
		return;
	}

	// Fire-and-forget /logout per the registry: the server invalidates the
	// refresh-token family; we do not wait, bind no completion, and treat any
	// failure as the server's problem — the LOCAL sign-out below is
	// unconditional (ACC-§11: cloud failure degrades, never blocks).
	if (IsCloudConfigured())
	{
		const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
			MakeCloudRequest(CloudConfig, AccessToken, CloudConfig.ProjectUrl + TEXT("/auth/v1/logout"), TEXT("POST"));
		Request->ProcessRequest();
	}

	const bool bWasAuthenticated = true; // guarded above — this IS a transition
	AccessToken.Reset();
	InMemoryRefreshToken.Reset();
	CloudUserId.Reset();
	UE_LOG(LogSiegeCloud, Log, TEXT("[SiegeCloud] Signed out; in-memory tokens cleared."));
	BroadcastIfAuthStateChanged(bWasAuthenticated);
}

void USiegeCloudClient::FetchRows(const FString& Table, const FString& QuerySuffix, FSiegeCloudResult OnDone)
{
	if (!IsCloudConfigured())
	{
		OnDone.ExecuteIfBound(false, TEXT("Cloud is not configured (Config/SiegeCloudDev.ini missing or incomplete)."));
		return;
	}
	if (!IsCloudAuthenticated())
	{
		// Fail FAST and LOCALLY: the live policies are `to authenticated`
		// (ACC-§12) — the server would deny this round trip anyway.
		OnDone.ExecuteIfBound(false, TEXT("Not signed in to the cloud."));
		return;
	}

	// GET /rest/v1/<Table>?<QuerySuffix> (ACC-§15). QuerySuffix verbatim;
	// empty suffix appends no '?'.
	FString Url = FString::Printf(TEXT("%s/rest/v1/%s"), *CloudConfig.ProjectUrl, *Table);
	if (!QuerySuffix.IsEmpty())
	{
		Url += TEXT("?");
		Url += QuerySuffix;
	}

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = MakeCloudRequest(CloudConfig, AccessToken, Url, TEXT("GET"));
	// Weak-bound: a callback into a torn-down subsystem is silently skipped
	// (shutdown-safe); row ops touch no auth state, so only OnDone is needed.
	Request->OnProcessRequestComplete().BindWeakLambda(this,
		[OnDone](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bConnectedOk)
		{
			const bool bHttpOk = IsHttpSuccess(HttpResponse, bConnectedOk);
			if (!bHttpOk)
			{
				LogHttpFailure(HttpRequest, HttpResponse);
			}
			OnDone.ExecuteIfBound(bHttpOk,
				bHttpOk ? HttpResponse->GetContentAsString() : DescribeHttpFailure(HttpResponse, bConnectedOk));
		});
	Request->ProcessRequest();
}

void USiegeCloudClient::UpsertRow(const FString& Table, const FString& JsonBody, FSiegeCloudResult OnDone)
{
	if (!IsCloudConfigured())
	{
		OnDone.ExecuteIfBound(false, TEXT("Cloud is not configured (Config/SiegeCloudDev.ini missing or incomplete)."));
		return;
	}
	if (!IsCloudAuthenticated())
	{
		OnDone.ExecuteIfBound(false, TEXT("Not signed in to the cloud."));
		return;
	}

	// POST /rest/v1/<Table>, Prefer: resolution=merge-duplicates (ACC-§15,
	// character-for-character). Table is appended VERBATIM — the ACC-§13
	// per-deck conflict target rides inline as
	// "decks?on_conflict=user_id,deck_name" (see the header note).
	const FString Url = FString::Printf(TEXT("%s/rest/v1/%s"), *CloudConfig.ProjectUrl, *Table);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = MakeCloudRequest(CloudConfig, AccessToken, Url, TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Prefer"), TEXT("resolution=merge-duplicates"));
	Request->SetContentAsString(JsonBody);
	Request->OnProcessRequestComplete().BindWeakLambda(this,
		[OnDone](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bConnectedOk)
		{
			const bool bHttpOk = IsHttpSuccess(HttpResponse, bConnectedOk);
			if (!bHttpOk)
			{
				LogHttpFailure(HttpRequest, HttpResponse);
			}
			OnDone.ExecuteIfBound(bHttpOk,
				bHttpOk ? HttpResponse->GetContentAsString() : DescribeHttpFailure(HttpResponse, bConnectedOk));
		});
	Request->ProcessRequest();
}

bool USiegeCloudClient::LoadCloudConfigFile(const FString& IniFilePath, FSiegeCloudConfig& OutConfig)
{
	// Reset FIRST — a failed load leaves clean emptiness, never a partial
	// config (the cloud-off law needs an unambiguous IsValid() == false).
	OutConfig = FSiegeCloudConfig();

	if (!FPaths::FileExists(IniFilePath))
	{
		return false;
	}

	// FConfigCacheIni-family per the spec: a LOCAL FConfigFile, deliberately
	// NOT GConfig — the dev ini must never enter the global config cache
	// where unrelated systems could see it.
	FConfigFile ConfigFile;
	ConfigFile.Read(IniFilePath);
	return ExtractCloudConfig(ConfigFile, OutConfig);
}

bool USiegeCloudClient::ExtractCloudConfig(const FConfigFile& ConfigFile, FSiegeCloudConfig& OutConfig)
{
	OutConfig = FSiegeCloudConfig();

	// [SiegeCloud] ProjectUrl= / AnonKey= — the ACC-§11 config home, exactly.
	// A missing key just stays empty and IsValid() reports the config
	// incomplete. The `; DbPassword=` custody line is an ini COMMENT —
	// FConfigFile never even parses it, so the game structurally cannot read
	// it (ACC-§11).
	ConfigFile.GetString(TEXT("SiegeCloud"), TEXT("ProjectUrl"), OutConfig.ProjectUrl);
	ConfigFile.GetString(TEXT("SiegeCloud"), TEXT("AnonKey"), OutConfig.AnonKey);

	OutConfig.ProjectUrl.TrimStartAndEndInline();
	OutConfig.AnonKey.TrimStartAndEndInline();

	// Normalize away trailing slashes so "<ProjectUrl>/auth/v1/..." can never
	// produce "//" (Supabase URLs are exact; a doubled slash 404s).
	while (OutConfig.ProjectUrl.EndsWith(TEXT("/")))
	{
		OutConfig.ProjectUrl.LeftChopInline(1);
	}

	return OutConfig.IsValid();
}

void USiegeCloudClient::SendAuthRequest(const FString& AuthPathAndQuery, const TSharedRef<FJsonObject>& Body, FSiegeCloudResult OnDone)
{
	if (!IsCloudConfigured())
	{
		// The ACC-§11 cloud-off law: fail fast, locally, through the
		// delegate — the caller's CloudStatusText says why; nothing blocks.
		OnDone.ExecuteIfBound(false, TEXT("Cloud is not configured (Config/SiegeCloudDev.ini missing or incomplete)."));
		return;
	}

	FString BodyText;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyText);
	FJsonSerializer::Serialize(Body, Writer);

	// Bearer is the access token when a session is held (e.g. switching
	// accounts), otherwise the anon key — GoTrue accepts either alongside
	// apikey.
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = MakeCloudRequest(
		CloudConfig,
		AccessToken.IsEmpty() ? CloudConfig.AnonKey : AccessToken,
		CloudConfig.ProjectUrl + AuthPathAndQuery,
		TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyText);

	// Weak-bound (shutdown-safe). Completion runs on the game thread — the
	// FHttpModule tick — so member access below needs no synchronization.
	Request->OnProcessRequestComplete().BindWeakLambda(this,
		[this, OnDone](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bConnectedOk)
		{
			const bool bWasAuthenticated = IsCloudAuthenticated();
			const bool bHttpOk = IsHttpSuccess(HttpResponse, bConnectedOk);

			FString Payload;
			if (bHttpOk)
			{
				// The raw success body is the payload: TASK-644/646 extract
				// refresh_token + user fields from it for SetCloudLink.
				Payload = HttpResponse->GetContentAsString();
				AdoptSessionFromAuthResponse(Payload);
			}
			else
			{
				LogHttpFailure(HttpRequest, HttpResponse);
				Payload = DescribeHttpFailure(HttpResponse, bConnectedOk);
			}

			// Broadcast BEFORE OnDone so state listeners are already fresh
			// when the caller's result handler reads the getters.
			BroadcastIfAuthStateChanged(bWasAuthenticated);
			OnDone.ExecuteIfBound(bHttpOk, Payload);
		});
	Request->ProcessRequest();
}

bool USiegeCloudClient::AdoptSessionFromAuthResponse(const FString& ResponseBody)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	// No access_token => no session to adopt. This is the NORMAL signup shape
	// while "Confirm email" is still ON (the S2 hand-step,
	// handoffs/TASK-641-buildmaster.md §5.2) — success without a session,
	// state untouched.
	FString NewAccessToken;
	if (!Root->TryGetStringField(TEXT("access_token"), NewAccessToken) || NewAccessToken.IsEmpty())
	{
		return false;
	}

	AccessToken = NewAccessToken;
	Root->TryGetStringField(TEXT("refresh_token"), InMemoryRefreshToken);

	const TSharedPtr<FJsonObject>* UserObject = nullptr;
	if (Root->TryGetObjectField(TEXT("user"), UserObject) && UserObject != nullptr && UserObject->IsValid())
	{
		(*UserObject)->TryGetStringField(TEXT("id"), CloudUserId);
	}

	// ⛔ P2-R6: the user id is loggable (it keys public RLS rows); NO token
	// ever is.
	UE_LOG(LogSiegeCloud, Log, TEXT("[SiegeCloud] Session adopted (user id %s)."), *CloudUserId);
	return true;
}

void USiegeCloudClient::BroadcastIfAuthStateChanged(bool bWasAuthenticated)
{
	if (IsCloudAuthenticated() == bWasAuthenticated)
	{
		return; // no-op — no broadcast (the delegate law)
	}
	++CloudStateChangedBroadcastCount;
	UE_LOG(LogSiegeCloud, Log, TEXT("[SiegeCloud] Auth state changed: %s."),
		IsCloudAuthenticated() ? TEXT("signed in") : TEXT("signed out"));
	OnCloudStateChanged.Broadcast();
}
