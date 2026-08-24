// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeCloudSync.h"

#include "Dom/JsonValue.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Siegebound/DeckTypes.h"
#include "Siegebound/SiegeAccountSubsystem.h"
#include "Siegebound/SiegeDeckSaveGame.h"
#include "Siegebound/SiegeSettingsSaveGame.h"

// ⚠️ CONSTANT-ONLY INCLUDE (the ACC-§4 .cpp-include precedent, cloned from
// SiegeAccountSubsystem.cpp): this file reads EXACTLY ONE symbol from the
// settings subsystem header — USiegeSettingsSubsystem::SettingsUserIndex — to
// address the settings SAVE SLOT the seam names. ⛔ No settings-subsystem TYPE
// is used, no member is called, and the file itself is untouched (P2-R5).
#include "Siegebound/SiegeSettingsSubsystem.h"

namespace SiegeCloudSyncLocal
{

// ─── Row + payload keys ──────────────────────────────────────────────────────
// snake_case, mirroring the ACC-§12 SQL columns character-for-character;
// TASK-647 pins the row-level keys with TestEqualSensitive (SC-§13). The
// payload INTERIOR keys are this file's projection schema (ACC-§13: payloads
// are projections of the existing save classes — the cloud never learns UE
// property spellings, and a field added to a save class later is added here
// deliberately, mirroring tagged-property absence semantics on apply).
static const TCHAR* const KeyUserId           = TEXT("user_id");
static const TCHAR* const KeyDeckName         = TEXT("deck_name");
static const TCHAR* const KeyPayload          = TEXT("payload");
static const TCHAR* const KeyUpdatedAt        = TEXT("updated_at");
static const TCHAR* const KeyCards            = TEXT("cards");
static const TCHAR* const KeyCardId           = TEXT("card_id");
static const TCHAR* const KeyCount            = TEXT("count");
static const TCHAR* const KeyAssistantConfirm = TEXT("assistant_confirm_before_execute");

// ─── Per-cloud-user session state (in-flight guard ONLY since TASK-653) ──────
// ⭐ 2026-08-23, TASK-653 (rider R2 — ACC-§15 P2.1): the session-scoped
// LastSyncUtc MIRROR that lived here (the SC-§15 deviation D1 shape) is
// RETIRED. The registry now pins USiegeAccountSubsystem::GetLastSyncUtc(), so
// the PERSISTED A3 sync clock is readable and MakeContext() consumes IT as the
// one pull baseline — both of D1's no-baseline branches collapse to the
// ACC-§13 letter (never-synced => FDateTime() => ShouldPullRow pulls every
// real row; previously-synced => strictly-newer rows only, across sessions).
// SC-§15 NOTE, THE DECLARED MIRROR FATE: the same-session cache is REMOVED,
// not kept — two baseline authorities would need a merge rule the law does not
// have. Consequence of removal, stated: the one degraded corner (account
// subsystem unresolvable at FinishSync => stamp not persisted) now re-pulls
// already-applied rows on a later cycle instead of being masked in-session —
// idempotent (overwrite-on-collision with the same or newer rows), honest,
// never destructive. What remains below is the D5 hygiene guard only.
// Game thread only (FHttpModule completion delegates fire on the game thread).
struct FSessionState
{
	/** One sync at a time per cloud user (hygiene guard D5; cleared by RAII in ~FSyncContext). */
	bool bInFlight = false;
};

static TMap<FString, FSessionState> GSessionStateByCloudUser;

// ─── The trigger flavors (ACC-§13's three, exactly) ──────────────────────────
enum class ESyncTrigger : uint8
{
	PullAll,   // trigger 1 — cloud-login pull
	PushAll,   // trigger 2 — first-link upload (A4)
	SyncNow    // trigger 3 — pull-newer -> push-all -> stamp
};

static const TCHAR* TriggerName(ESyncTrigger Trigger)
{
	switch (Trigger)
	{
	case ESyncTrigger::PullAll: return TEXT("PullAll");
	case ESyncTrigger::PushAll: return TEXT("PushAll");
	default:                    return TEXT("SyncNow");
	}
}

// ─── The in-flight pipeline state ────────────────────────────────────────────
// One heap-shared context per running sync; the HTTP continuation lambdas hold
// the only references, so the FSiegeCloudSync INSTANCE can die freely while a
// round trip is on the wire (the class carries zero members by design). The
// destructor clears the in-flight guard on EVERY exit path — including a
// dropped chain (dead GameInstance) whose delegates are simply released.
struct FSyncContext
{
	TWeakObjectPtr<UGameInstance> GameInstance;
	ESyncTrigger                  Trigger = ESyncTrigger::SyncNow;
	FString                       CloudUserId;
	FSiegeCloudResult             OnDone;

	/** Pull filter baseline — the PERSISTED profile stamp via GetLastSyncUtc() at MakeContext() (⭐ TASK-653 R2; no session mirror). Zero ticks = never synced => ShouldPullRow pulls every real row. */
	FDateTime                     PullBaseline;

	/** Max server updated_at observed across the cycle — the pull-side stamp basis (D3). */
	FDateTime                     MaxSeenUpdatedUtc; // ticks 0 = none seen

	int32 PulledDecks            = 0;
	bool  bPulledSettings        = false;
	int32 SkippedMalformedRows   = 0;

	/** Push queue: pre-serialized decks row bodies (settings pushes after, LAST — see D3). */
	TArray<FString> PendingDeckBodies;
	FString         SettingsBody;
	int32           PushedDecks = 0;

	~FSyncContext()
	{
		if (FSessionState* State = GSessionStateByCloudUser.Find(CloudUserId))
		{
			State->bInFlight = false;
		}
	}
};
using FSyncContextRef = TSharedRef<FSyncContext, ESPMode::ThreadSafe>;

// ─── Small helpers ───────────────────────────────────────────────────────────

static USiegeCloudClient* ResolveClient(const FSyncContextRef& Ctx)
{
	UGameInstance* GameInstance = Ctx->GameInstance.Get();
	return GameInstance ? GameInstance->GetSubsystem<USiegeCloudClient>() : nullptr;
}

static USiegeAccountSubsystem* ResolveAccounts(const FSyncContextRef& Ctx)
{
	UGameInstance* GameInstance = Ctx->GameInstance.Get();
	return GameInstance ? GameInstance->GetSubsystem<USiegeAccountSubsystem>() : nullptr;
}

/**
 *  Postgres timestamptz arrives as ISO 8601 with a microsecond fraction and a
 *  +00:00 offset (e.g. 2026-08-23T17:47:12.123456+00:00). FDateTime::
 *  ParseIso8601 handles offsets; the retry clamps an over-long fraction to
 *  milliseconds in case the engine parser rejects six digits.
 */
static bool ParseDbTimestampUtc(const FString& In, FDateTime& Out)
{
	if (FDateTime::ParseIso8601(*In, Out))
	{
		return true;
	}

	int32 DotIndex = INDEX_NONE;
	if (In.FindChar(TEXT('.'), DotIndex))
	{
		int32 FractionEnd = DotIndex + 1;
		while (FractionEnd < In.Len() && FChar::IsDigit(In[FractionEnd]))
		{
			++FractionEnd;
		}
		const int32 KeptDigits = FMath::Min(3, FractionEnd - (DotIndex + 1));
		const FString Clamped =
			In.Left(DotIndex + 1 + KeptDigits) + In.Mid(FractionEnd);
		return FDateTime::ParseIso8601(*Clamped, Out);
	}
	return false;
}

static FString JsonObjectToCondensedString(const TSharedRef<FJsonObject>& Object)
{
	FString Result;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Result);
	FJsonSerializer::Serialize(Object, Writer);
	return Result;
}

static bool ParseJsonArray(const FString& Body, TArray<TSharedPtr<FJsonValue>>& OutRows)
{
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Body);
	return FJsonSerializer::Deserialize(Reader, OutRows);
}

// ─── Payload projections (ACC-§3: projections, never save-class rewrites) ────

/** FDeckList -> {"cards":[{"card_id","count"},...]} (deck_name is the ROW column, not duplicated inside the payload). */
static TSharedRef<FJsonObject> MakeDeckPayload(const FDeckList& Deck)
{
	const TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> CardsJson;
	CardsJson.Reserve(Deck.Cards.Num());
	for (const FDeckCardEntry& Entry : Deck.Cards)
	{
		const TSharedRef<FJsonObject> CardJson = MakeShared<FJsonObject>();
		CardJson->SetStringField(KeyCardId, Entry.CardID.ToString());
		CardJson->SetNumberField(KeyCount, Entry.Count);
		CardsJson.Add(MakeShared<FJsonValueObject>(CardJson));
	}
	Payload->SetArrayField(KeyCards, CardsJson);
	return Payload;
}

/** Payload -> FDeckList cards (DeckName is applied by the caller from the row column). Unknown keys are ignored; a missing "cards" clears nothing (returns false). */
static bool ParseDeckPayloadInto(const TSharedPtr<FJsonObject>& Payload, FDeckList& OutDeck)
{
	if (!Payload.IsValid())
	{
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* CardsJson = nullptr;
	if (!Payload->TryGetArrayField(KeyCards, CardsJson) || CardsJson == nullptr)
	{
		return false;
	}
	OutDeck.Cards.Reset();
	for (const TSharedPtr<FJsonValue>& CardValue : *CardsJson)
	{
		const TSharedPtr<FJsonObject>* CardJson = nullptr;
		if (!CardValue.IsValid() || !CardValue->TryGetObject(CardJson) || CardJson == nullptr)
		{
			continue;
		}
		FDeckCardEntry Entry;
		FString CardIdString;
		if ((*CardJson)->TryGetStringField(KeyCardId, CardIdString))
		{
			Entry.CardID = FName(*CardIdString);
		}
		double CountNumber = 0.0;
		if ((*CardJson)->TryGetNumberField(KeyCount, CountNumber))
		{
			Entry.Count = static_cast<int32>(CountNumber);
		}
		OutDeck.Cards.Add(Entry);
	}
	return true;
}

/** USiegeSettingsSaveGame -> {"assistant_confirm_before_execute":bool}. */
static TSharedRef<FJsonObject> MakeSettingsPayload(const USiegeSettingsSaveGame& Settings)
{
	const TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetBoolField(KeyAssistantConfirm, Settings.bAssistantConfirmBeforeExecute);
	return Payload;
}

/** Applies only the keys this projection knows; an absent key leaves the local value untouched (the tagged-property absence idiom, mirrored). */
static void ApplySettingsPayload(const TSharedPtr<FJsonObject>& Payload, USiegeSettingsSaveGame& Settings)
{
	bool bConfirmValue = false;
	if (Payload.IsValid() && Payload->TryGetBoolField(KeyAssistantConfirm, bConfirmValue))
	{
		Settings.bAssistantConfirmBeforeExecute = bConfirmValue;
	}
}

// ─── Local slot IO (through the ACC-§4 seam slots, existing save classes) ────

static USiegeDeckSaveGame* LoadOrCreateDeckSave(const FString& SlotName)
{
	USiegeDeckSaveGame* DeckSave =
		Cast<USiegeDeckSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, USiegeDeckSaveGame::UserIndex));
	if (DeckSave == nullptr)
	{
		DeckSave = Cast<USiegeDeckSaveGame>(
			UGameplayStatics::CreateSaveGameObject(USiegeDeckSaveGame::StaticClass()));
	}
	return DeckSave;
}

static USiegeSettingsSaveGame* LoadOrCreateSettingsSave(const FString& SlotName)
{
	USiegeSettingsSaveGame* SettingsSave = Cast<USiegeSettingsSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SlotName, USiegeSettingsSubsystem::SettingsUserIndex));
	if (SettingsSave == nullptr)
	{
		SettingsSave = Cast<USiegeSettingsSaveGame>(
			UGameplayStatics::CreateSaveGameObject(USiegeSettingsSaveGame::StaticClass()));
	}
	return SettingsSave;
}

// ─── Terminal steps ──────────────────────────────────────────────────────────

/** The ONE failure funnel: one LogSiegeCloud line + OnDone(false, reason). Local state is left exactly as it was; no stamp is written (the next cycle re-pulls/re-pushes — upserts are idempotent). ⛔ Reason strings carry no token, password, or credential material (P2-R6) — they are this file's own text plus the client's error text, which is bound by the same law. */
static void FailSync(const FSyncContextRef& Ctx, const FString& Reason)
{
	const FString Clamped = Reason.Left(200);
	UE_LOG(LogSiegeCloud, Warning, TEXT("[SiegeCloudSync] %s failed: %s"),
		TriggerName(Ctx->Trigger), *Clamped);
	Ctx->OnDone.ExecuteIfBound(false, Clamped);
}

/**
 *  The ONE success funnel. Order is load-bearing: the SLOT WRITES all happened
 *  earlier in the pipeline, so SetLastSyncUtc runs LAST — its
 *  OnActiveProfileChanged broadcast (the TASK-644 save-on-change contract)
 *  rides the EXISTING P1 wiring (USiegeSettingsSubsystem::
 *  HandleActiveProfileChanged -> ReloadForActiveProfile) and the live settings
 *  subsystem re-reads the just-written slot — pulled settings go live with
 *  ZERO edits to the P2-R5 protected files.
 */
static void FinishSync(const FSyncContextRef& Ctx, const FDateTime& StampUtc, const FString& Summary)
{
	if (USiegeAccountSubsystem* Accounts = ResolveAccounts(Ctx))
	{
		Accounts->SetLastSyncUtc(StampUtc);
	}
	else
	{
		// The data moved; only the stamp is lost (⭐ TASK-653 R2: the session
		// mirror that also recorded it here is retired). The next cycle's
		// baseline is the older persisted stamp, so already-applied rows
		// re-pull idempotently (overwrite-on-collision with the same or newer
		// rows) — honest, never destructive.
		UE_LOG(LogSiegeCloud, Warning,
			TEXT("[SiegeCloudSync] %s: account subsystem unresolvable at finish — LastSyncUtc not persisted this cycle."),
			TriggerName(Ctx->Trigger));
	}

	UE_LOG(LogSiegeCloud, Log, TEXT("[SiegeCloudSync] %s ok: %s"),
		TriggerName(Ctx->Trigger), *Summary);
	Ctx->OnDone.ExecuteIfBound(true, Summary);
}

static FString BuildSuccessSummary(const FSyncContextRef& Ctx)
{
	FString Summary;
	if (Ctx->Trigger != ESyncTrigger::PushAll)
	{
		// ⭐ TASK-653 R2: the "pull skipped (no session baseline)" branch is gone
		// with D1 — the pull phase always runs now (persisted baseline).
		Summary += FString::Printf(TEXT("pulled %d deck(s)%s"),
			Ctx->PulledDecks, Ctx->bPulledSettings ? TEXT(" + settings") : TEXT(""));
	}
	if (Ctx->Trigger != ESyncTrigger::PullAll)
	{
		if (!Summary.IsEmpty())
		{
			Summary += TEXT("; ");
		}
		Summary += FString::Printf(TEXT("pushed %d deck(s) + settings"), Ctx->PushedDecks);
	}
	if (Ctx->SkippedMalformedRows > 0)
	{
		Summary += FString::Printf(TEXT(" (%d malformed cloud row(s) skipped)"), Ctx->SkippedMalformedRows);
	}
	return Summary;
}

// ─── Pipeline phases (forward declarations — the chain reads top-down) ───────

static void StartPullPhase(const FSyncContextRef& Ctx);
static void HandleDecksFetched(const FSyncContextRef& Ctx, bool bFetchOk, const FString& Body);
static void HandleSettingsFetched(const FSyncContextRef& Ctx, bool bFetchOk, const FString& Body);
static void FinishPullOnly(const FSyncContextRef& Ctx);
static void StartPushPhase(const FSyncContextRef& Ctx);
static void PushNextDeck(const FSyncContextRef& Ctx);
static void PushSettingsRow(const FSyncContextRef& Ctx);
static void ReadBackStamp(const FSyncContextRef& Ctx);

// ─── Pull phase ──────────────────────────────────────────────────────────────

static void StartPullPhase(const FSyncContextRef& Ctx)
{
	// ⭐ TASK-653 (rider R2 — ACC-§15 P2.1): 645-D1's skip-pull compromise is
	// REMOVED. The baseline is now the PERSISTED LastSyncUtc (read at
	// MakeContext), so EVERY SyncNow pull phase runs: never-synced => zero
	// ticks => pull ALL (trigger 1's fresh-link semantics, now the lawful
	// trigger-3 shape too — the ACC-§13 letter); previously-synced =>
	// strictly-newer rows only (ShouldPullRow), including the first SyncNow of
	// a NEW session, which used to skip its pull entirely.
	USiegeCloudClient* Client = ResolveClient(Ctx);
	if (Client == nullptr)
	{
		FailSync(Ctx, TEXT("cloud client unavailable"));
		return;
	}

	// RLS already scopes the response to auth.uid()'s rows (ACC-§12); the
	// user_id filter is belt and braces, and the newness decision is made
	// CLIENT-SIDE by the ONE pinned predicate, ShouldPullRow.
	const FString Query = FString::Printf(
		TEXT("user_id=eq.%s&select=deck_name,payload,updated_at&order=deck_name.asc"),
		*Ctx->CloudUserId);
	Client->FetchRows(TEXT("decks"), Query,
		FSiegeCloudResult::CreateLambda([Ctx](bool bFetchOk, const FString& Body)
		{
			HandleDecksFetched(Ctx, bFetchOk, Body);
		}));
}

static void HandleDecksFetched(const FSyncContextRef& Ctx, bool bFetchOk, const FString& Body)
{
	if (!Ctx->GameInstance.IsValid())
	{
		return; // world gone — drop the chain (the guard clears via ~FSyncContext)
	}
	if (!bFetchOk)
	{
		FailSync(Ctx, FString::Printf(TEXT("deck fetch failed: %s"), *Body.Left(120)));
		return;
	}

	TArray<TSharedPtr<FJsonValue>> Rows;
	if (!ParseJsonArray(Body, Rows))
	{
		FailSync(Ctx, TEXT("deck fetch returned unparsable JSON"));
		return;
	}

	const FDateTime Baseline = Ctx->PullBaseline;
	TArray<FDeckList> DecksToApply;
	for (const TSharedPtr<FJsonValue>& RowValue : Rows)
	{
		const TSharedPtr<FJsonObject>* Row = nullptr;
		FString DeckNameValue;
		FString UpdatedAtValue;
		FDateTime UpdatedAtUtc;
		if (!RowValue.IsValid() || !RowValue->TryGetObject(Row) || Row == nullptr ||
			!(*Row)->TryGetStringField(KeyDeckName, DeckNameValue) ||
			!(*Row)->TryGetStringField(KeyUpdatedAt, UpdatedAtValue) ||
			!ParseDbTimestampUtc(UpdatedAtValue, UpdatedAtUtc))
		{
			++Ctx->SkippedMalformedRows;
			continue;
		}

		// Every row SEEN advances the stamp basis (D3): a row at or before the
		// stamp is either applied below or already local — never lost.
		Ctx->MaxSeenUpdatedUtc = FMath::Max(Ctx->MaxSeenUpdatedUtc, UpdatedAtUtc);

		if (!FSiegeCloudSync::ShouldPullRow(UpdatedAtUtc, Baseline))
		{
			continue;
		}

		const TSharedPtr<FJsonObject>* PayloadJson = nullptr;
		FDeckList Deck;
		Deck.DeckName = DeckNameValue;
		if (!(*Row)->TryGetObjectField(KeyPayload, PayloadJson) || PayloadJson == nullptr ||
			!ParseDeckPayloadInto(*PayloadJson, Deck))
		{
			++Ctx->SkippedMalformedRows;
			continue;
		}
		DecksToApply.Add(MoveTemp(Deck));
	}

	if (DecksToApply.Num() > 0)
	{
		USiegeAccountSubsystem* Accounts = ResolveAccounts(Ctx);
		if (Accounts == nullptr)
		{
			FailSync(Ctx, TEXT("account subsystem unresolvable during deck apply"));
			return;
		}
		const FString DeckSlot = Accounts->GetDeckSlotName();
		USiegeDeckSaveGame* DeckSave = LoadOrCreateDeckSave(DeckSlot);
		if (DeckSave == nullptr)
		{
			FailSync(Ctx, TEXT("local deck save object could not be created"));
			return;
		}
		for (FDeckList& Pulled : DecksToApply)
		{
			// Merge by name, overwrite-on-collision (the M6 ruling-2 idiom).
			// CASE-SENSITIVE match, mirroring the server's unique
			// (user_id, deck_name) text semantics. Pull never REMOVES a local
			// deck (no tombstones — ACC-§13 recorded limitation) and never
			// touches ActiveDeckName (per-device, qa/TASK-640's P3 candidate).
			FDeckList* Existing = DeckSave->SavedDecks.FindByPredicate(
				[&Pulled](const FDeckList& Candidate)
				{
					return Candidate.DeckName.Equals(Pulled.DeckName, ESearchCase::CaseSensitive);
				});
			if (Existing != nullptr)
			{
				*Existing = MoveTemp(Pulled);
			}
			else
			{
				DeckSave->SavedDecks.Add(MoveTemp(Pulled));
			}
			++Ctx->PulledDecks;
		}
		if (!UGameplayStatics::SaveGameToSlot(DeckSave, DeckSlot, USiegeDeckSaveGame::UserIndex))
		{
			FailSync(Ctx, TEXT("local deck save write failed"));
			return;
		}
	}

	USiegeCloudClient* Client = ResolveClient(Ctx);
	if (Client == nullptr)
	{
		FailSync(Ctx, TEXT("cloud client unavailable"));
		return;
	}
	const FString Query = FString::Printf(
		TEXT("user_id=eq.%s&select=payload,updated_at"), *Ctx->CloudUserId);
	Client->FetchRows(TEXT("settings"), Query,
		FSiegeCloudResult::CreateLambda([Ctx](bool bFetchOk2, const FString& Body2)
		{
			HandleSettingsFetched(Ctx, bFetchOk2, Body2);
		}));
}

static void HandleSettingsFetched(const FSyncContextRef& Ctx, bool bFetchOk, const FString& Body)
{
	if (!Ctx->GameInstance.IsValid())
	{
		return;
	}
	if (!bFetchOk)
	{
		FailSync(Ctx, FString::Printf(TEXT("settings fetch failed: %s"), *Body.Left(120)));
		return;
	}

	TArray<TSharedPtr<FJsonValue>> Rows;
	if (!ParseJsonArray(Body, Rows))
	{
		FailSync(Ctx, TEXT("settings fetch returned unparsable JSON"));
		return;
	}

	// ONE settings row per user (user_id IS the PK — ACC-§12); 0 rows = never
	// pushed from any device yet.
	if (Rows.Num() > 0)
	{
		const TSharedPtr<FJsonObject>* Row = nullptr;
		FString UpdatedAtValue;
		FDateTime UpdatedAtUtc;
		if (Rows[0].IsValid() && Rows[0]->TryGetObject(Row) && Row != nullptr &&
			(*Row)->TryGetStringField(KeyUpdatedAt, UpdatedAtValue) &&
			ParseDbTimestampUtc(UpdatedAtValue, UpdatedAtUtc))
		{
			Ctx->MaxSeenUpdatedUtc = FMath::Max(Ctx->MaxSeenUpdatedUtc, UpdatedAtUtc);
			const FDateTime Baseline = Ctx->PullBaseline;
			if (FSiegeCloudSync::ShouldPullRow(UpdatedAtUtc, Baseline))
			{
				USiegeAccountSubsystem* Accounts = ResolveAccounts(Ctx);
				if (Accounts == nullptr)
				{
					FailSync(Ctx, TEXT("account subsystem unresolvable during settings apply"));
					return;
				}
				const TSharedPtr<FJsonObject>* PayloadJson = nullptr;
				if ((*Row)->TryGetObjectField(KeyPayload, PayloadJson) && PayloadJson != nullptr)
				{
					const FString SettingsSlot = Accounts->GetSettingsSlotName();
					USiegeSettingsSaveGame* SettingsSave = LoadOrCreateSettingsSave(SettingsSlot);
					if (SettingsSave == nullptr)
					{
						FailSync(Ctx, TEXT("local settings save object could not be created"));
						return;
					}
					ApplySettingsPayload(*PayloadJson, *SettingsSave);
					if (!UGameplayStatics::SaveGameToSlot(
						SettingsSave, SettingsSlot, USiegeSettingsSubsystem::SettingsUserIndex))
					{
						FailSync(Ctx, TEXT("local settings save write failed"));
						return;
					}
					// The LIVE settings subsystem re-reads this slot when
					// FinishSync's SetLastSyncUtc broadcast fires (P1 wiring).
					Ctx->bPulledSettings = true;
				}
				else
				{
					++Ctx->SkippedMalformedRows;
				}
			}
		}
		else
		{
			++Ctx->SkippedMalformedRows;
		}
	}

	if (Ctx->Trigger == ESyncTrigger::PullAll)
	{
		FinishPullOnly(Ctx);
	}
	else
	{
		StartPushPhase(Ctx);
	}
}

static void FinishPullOnly(const FSyncContextRef& Ctx)
{
	// D3: a pure pull's stamp is the max server updated_at it observed — a
	// true server-side time at or after every row it landed. An EMPTY cloud
	// observed nothing; FDateTime::UtcNow() is the declared client-clock
	// fallback (ahead => under-pull, recoverable at the next cloud login;
	// behind => over-pull, idempotent re-apply).
	const FDateTime Stamp = (Ctx->MaxSeenUpdatedUtc.GetTicks() > 0)
		? Ctx->MaxSeenUpdatedUtc
		: FDateTime::UtcNow();
	FinishSync(Ctx, Stamp, BuildSuccessSummary(Ctx));
}

// ─── Push phase ──────────────────────────────────────────────────────────────

static void StartPushPhase(const FSyncContextRef& Ctx)
{
	if (!Ctx->GameInstance.IsValid())
	{
		return;
	}
	USiegeAccountSubsystem* Accounts = ResolveAccounts(Ctx);
	if (Accounts == nullptr)
	{
		FailSync(Ctx, TEXT("account subsystem unresolvable at push"));
		return;
	}

	// Snapshot the ACTIVE profile's local slots through the seam (ACC-§4).
	// Reading the slots directly is this batch's ruled posture (P2-R5): no
	// widget, controller, or settings-subsystem live state is consulted.
	const FString DeckSlot = Accounts->GetDeckSlotName();
	const USiegeDeckSaveGame* DeckSave =
		Cast<USiegeDeckSaveGame>(UGameplayStatics::LoadGameFromSlot(DeckSlot, USiegeDeckSaveGame::UserIndex));
	Ctx->PendingDeckBodies.Reset();
	if (DeckSave != nullptr)
	{
		for (const FDeckList& Deck : DeckSave->SavedDecks)
		{
			if (Deck.DeckName.IsEmpty())
			{
				// text not null accepts '' but an unnamed deck is not a
				// meaningful sync key; skipped, counted, never fatal.
				++Ctx->SkippedMalformedRows;
				continue;
			}
			Ctx->PendingDeckBodies.Add(FSiegeCloudSync::MakeDeckRowJson(
				Ctx->CloudUserId, Deck.DeckName, MakeDeckPayload(Deck)));
		}
	}

	// Settings ALWAYS pushes one row: a missing slot projects the class
	// defaults off the CDO (the effective local state — the subsystem's own
	// missing-slot fallback, mirrored).
	const FString SettingsSlot = Accounts->GetSettingsSlotName();
	const USiegeSettingsSaveGame* SettingsSave = Cast<USiegeSettingsSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SettingsSlot, USiegeSettingsSubsystem::SettingsUserIndex));
	if (SettingsSave == nullptr)
	{
		SettingsSave = GetDefault<USiegeSettingsSaveGame>();
	}
	Ctx->SettingsBody = FSiegeCloudSync::MakeSettingsRowJson(
		Ctx->CloudUserId, MakeSettingsPayload(*SettingsSave));

	PushNextDeck(Ctx);
}

static void PushNextDeck(const FSyncContextRef& Ctx)
{
	if (!Ctx->GameInstance.IsValid())
	{
		return;
	}
	if (Ctx->PendingDeckBodies.Num() == 0)
	{
		PushSettingsRow(Ctx);
		return;
	}

	USiegeCloudClient* Client = ResolveClient(Ctx);
	if (Client == nullptr)
	{
		FailSync(Ctx, TEXT("cloud client unavailable"));
		return;
	}

	const FString Body = Ctx->PendingDeckBodies[0];
	Ctx->PendingDeckBodies.RemoveAt(0);

	// D4: the on_conflict target rides the Table parameter so PostgREST's
	// merge-duplicates resolves on the ACC-§12 unique (user_id, deck_name)
	// constraint — the uuid PK never collides, so a bare "decks" POST would
	// take the INSERT branch and 409 on every re-push of an existing deck.
	Client->UpsertRow(TEXT("decks?on_conflict=user_id,deck_name"), Body,
		FSiegeCloudResult::CreateLambda([Ctx](bool bUpsertOk, const FString& Response)
		{
			if (!Ctx->GameInstance.IsValid())
			{
				return;
			}
			if (!bUpsertOk)
			{
				FailSync(Ctx, FString::Printf(TEXT("deck upload failed: %s"), *Response.Left(120)));
				return;
			}
			++Ctx->PushedDecks;
			PushNextDeck(Ctx);
		}));
}

static void PushSettingsRow(const FSyncContextRef& Ctx)
{
	USiegeCloudClient* Client = ResolveClient(Ctx);
	if (Client == nullptr)
	{
		FailSync(Ctx, TEXT("cloud client unavailable"));
		return;
	}
	// settings' PK IS user_id (in the body), so plain merge-duplicates
	// resolves on it — no on_conflict needed. Pushed LAST deliberately: its
	// server-stamped updated_at is the cycle's max, which the read-back below
	// adopts as "server now" (D3).
	Client->UpsertRow(TEXT("settings"), Ctx->SettingsBody,
		FSiegeCloudResult::CreateLambda([Ctx](bool bUpsertOk, const FString& Response)
		{
			if (!Ctx->GameInstance.IsValid())
			{
				return;
			}
			if (!bUpsertOk)
			{
				FailSync(Ctx, FString::Printf(TEXT("settings upload failed: %s"), *Response.Left(120)));
				return;
			}
			ReadBackStamp(Ctx);
		}));
}

static void ReadBackStamp(const FSyncContextRef& Ctx)
{
	USiegeCloudClient* Client = ResolveClient(Ctx);
	if (Client == nullptr)
	{
		FailSync(Ctx, TEXT("cloud client unavailable"));
		return;
	}
	// D3: one GET of the settings row just written — its trigger-stamped
	// updated_at IS server time at the end of this cycle (the ACC-§12
	// before-insert-or-update trigger guarantees the server wrote it, never
	// this client).
	const FString Query = FString::Printf(
		TEXT("user_id=eq.%s&select=updated_at"), *Ctx->CloudUserId);
	Client->FetchRows(TEXT("settings"), Query,
		FSiegeCloudResult::CreateLambda([Ctx](bool bFetchOk, const FString& Body)
		{
			if (!Ctx->GameInstance.IsValid())
			{
				return;
			}
			FDateTime Stamp;
			bool bStampParsed = false;
			TArray<TSharedPtr<FJsonValue>> Rows;
			if (bFetchOk && ParseJsonArray(Body, Rows) && Rows.Num() > 0)
			{
				const TSharedPtr<FJsonObject>* Row = nullptr;
				FString UpdatedAtValue;
				if (Rows[0].IsValid() && Rows[0]->TryGetObject(Row) && Row != nullptr &&
					(*Row)->TryGetStringField(KeyUpdatedAt, UpdatedAtValue))
				{
					bStampParsed = ParseDbTimestampUtc(UpdatedAtValue, Stamp);
				}
			}
			if (!bStampParsed)
			{
				// The push itself SUCCEEDED; only the stamp read-back came up
				// short. Fall back per D3 rather than failing a completed sync.
				Stamp = (Ctx->MaxSeenUpdatedUtc.GetTicks() > 0)
					? Ctx->MaxSeenUpdatedUtc
					: FDateTime::UtcNow();
				UE_LOG(LogSiegeCloud, Log,
					TEXT("[SiegeCloudSync] %s: stamp read-back unavailable — using fallback baseline."),
					TriggerName(Ctx->Trigger));
			}
			FinishSync(Ctx, Stamp, BuildSuccessSummary(Ctx));
		}));
}

// ─── Preflight (the shared gate — ACC-§13: guest NEVER syncs) ────────────────

/**
 *  Returns an unset optional when the trigger may proceed (and marks the user
 *  in-flight), otherwise the refusal reason. ⛔ Every refusal is a NO-OP on
 *  local state: one log line + OnDone(false, reason) at the caller — cloud
 *  gates nothing (ACC-§11).
 */
static TOptional<FString> PreflightRefusal(UGameInstance& GameInstance, FString& OutCloudUserId)
{
	USiegeCloudClient* Client = GameInstance.GetSubsystem<USiegeCloudClient>();
	if (Client == nullptr)
	{
		return FString(TEXT("cloud client unavailable"));
	}
	if (!Client->IsCloudConfigured())
	{
		return FString(TEXT("cloud not configured")); // ACC-§11 cloud-off law: Phase-1 behavior
	}
	USiegeAccountSubsystem* Accounts = GameInstance.GetSubsystem<USiegeAccountSubsystem>();
	if (Accounts == nullptr)
	{
		return FString(TEXT("account subsystem unavailable"));
	}
	if (!Accounts->IsLoggedIn() || !Accounts->IsCloudLinked())
	{
		return FString(TEXT("no cloud-linked profile active")); // guest never syncs (ACC-§13)
	}
	if (!Client->IsCloudAuthenticated())
	{
		return FString(TEXT("not signed in to cloud"));
	}
	OutCloudUserId = Client->GetCloudUserId();
	if (OutCloudUserId.IsEmpty())
	{
		return FString(TEXT("cloud user id unavailable"));
	}
	FSessionState& State = GSessionStateByCloudUser.FindOrAdd(OutCloudUserId);
	if (State.bInFlight)
	{
		return FString(TEXT("a sync is already in progress"));
	}
	State.bInFlight = true;
	return TOptional<FString>();
}

static FSyncContextRef MakeContext(
	UGameInstance& GameInstance, ESyncTrigger Trigger,
	const FString& CloudUserId, FSiegeCloudResult&& OnDone)
{
	const FSyncContextRef Ctx = MakeShared<FSyncContext, ESPMode::ThreadSafe>();
	Ctx->GameInstance = &GameInstance;
	Ctx->Trigger      = Trigger;
	Ctx->CloudUserId  = CloudUserId;
	Ctx->OnDone       = MoveTemp(OnDone);

	// ⭐ TASK-653 (rider R2 — ACC-§15 P2.1): THE NAMED SWAP SITE
	// (handoffs/TASK-645-programmer.md §5 D1). The pull baseline is the
	// PERSISTED profile stamp via the pinned GetLastSyncUtc() — the session
	// mirror is retired (SC-§15 note at FSessionState). PreflightRefusal
	// resolved this same subsystem in this same synchronous frame, so the null
	// branch is structurally unreachable; its FDateTime() fallback (= pull-all,
	// trigger 1's fresh-link semantics) is null-safety, not a code path.
	USiegeAccountSubsystem* Accounts = GameInstance.GetSubsystem<USiegeAccountSubsystem>();
	Ctx->PullBaseline = Accounts ? Accounts->GetLastSyncUtc() : FDateTime();
	return Ctx;
}

static void RefuseTrigger(ESyncTrigger Trigger, const FString& Reason, const FSiegeCloudResult& OnDone)
{
	UE_LOG(LogSiegeCloud, Log, TEXT("[SiegeCloudSync] %s refused: %s"),
		TriggerName(Trigger), *Reason);
	OnDone.ExecuteIfBound(false, Reason);
}

} // namespace SiegeCloudSyncLocal

// ─── The pinned pure seams (ACC-§15 block 2; TASK-647's test surface) ────────

bool FSiegeCloudSync::ShouldPullRow(const FDateTime& CloudUpdatedUtc, const FDateTime& ProfileLastSyncUtc)
{
	// STRICTLY newer pulls; equal does not (that row landed in the cycle that
	// stamped the baseline); an unset baseline (ticks 0) pulls every real
	// cloud timestamp — ACC-§13 trigger 1's fresh-link semantics.
	return CloudUpdatedUtc > ProfileLastSyncUtc;
}

FString FSiegeCloudSync::MakeDeckRowJson(const FString& CloudUserId, const FString& DeckName, const TSharedRef<FJsonObject>& Payload)
{
	using namespace SiegeCloudSyncLocal;
	const TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
	Row->SetStringField(KeyUserId, CloudUserId);
	Row->SetStringField(KeyDeckName, DeckName);
	Row->SetObjectField(KeyPayload, Payload);
	// ⛔ NO "updated_at" — server-owned (ACC-§12; the A3 clock), NO "id" —
	// gen_random_uuid + the on_conflict merge own row identity.
	return JsonObjectToCondensedString(Row);
}

FString FSiegeCloudSync::MakeSettingsRowJson(const FString& CloudUserId, const TSharedRef<FJsonObject>& Payload)
{
	using namespace SiegeCloudSyncLocal;
	const TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
	Row->SetStringField(KeyUserId, CloudUserId);
	Row->SetObjectField(KeyPayload, Payload);
	// ⛔ NO "updated_at" — server-owned (ACC-§12; the A3 clock).
	return JsonObjectToCondensedString(Row);
}

// ─── The three triggers (ACC-§13 — this IS the entire sync surface) ──────────

void FSiegeCloudSync::PullAll(UGameInstance& GameInstance, FSiegeCloudResult OnDone)
{
	using namespace SiegeCloudSyncLocal;
	FString CloudUserId;
	if (const TOptional<FString> Refusal = PreflightRefusal(GameInstance, CloudUserId))
	{
		RefuseTrigger(ESyncTrigger::PullAll, Refusal.GetValue(), OnDone);
		return;
	}
	StartPullPhase(MakeContext(GameInstance, ESyncTrigger::PullAll, CloudUserId, MoveTemp(OnDone)));
}

void FSiegeCloudSync::PushAll(UGameInstance& GameInstance, FSiegeCloudResult OnDone)
{
	using namespace SiegeCloudSyncLocal;
	FString CloudUserId;
	if (const TOptional<FString> Refusal = PreflightRefusal(GameInstance, CloudUserId))
	{
		RefuseTrigger(ESyncTrigger::PushAll, Refusal.GetValue(), OnDone);
		return;
	}
	StartPushPhase(MakeContext(GameInstance, ESyncTrigger::PushAll, CloudUserId, MoveTemp(OnDone)));
}

void FSiegeCloudSync::SyncNow(UGameInstance& GameInstance, FSiegeCloudResult OnDone)
{
	using namespace SiegeCloudSyncLocal;
	FString CloudUserId;
	if (const TOptional<FString> Refusal = PreflightRefusal(GameInstance, CloudUserId))
	{
		RefuseTrigger(ESyncTrigger::SyncNow, Refusal.GetValue(), OnDone);
		return;
	}
	// pull-newer -> push-all -> LastSyncUtc = server now (the pinned order).
	StartPullPhase(MakeContext(GameInstance, ESyncTrigger::SyncNow, CloudUserId, MoveTemp(OnDone)));
}
