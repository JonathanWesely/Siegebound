// Copyright Epic Games, Inc. All Rights Reserved.

#include "SiegeSessionSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

// The ONE definition of the M8 net log category (CONVENTIONS "Networked 1v1
// (M8)" - LogSiegeNet lives in SiegeSessionSubsystem.{h,cpp} by ruling).
DEFINE_LOG_CATEGORY(LogSiegeNet);

namespace SiegeSessionInternal
{
	// Map package paths (verified assets: Content/Maps/L_Arena.umap and
	// Content/Maps/L_MainMenu.umap - CONVENTIONS map naming).
	static const TCHAR* ArenaMapPath = TEXT("/Game/Maps/L_Arena");
	static const TCHAR* MenuMapPath = TEXT("/Game/Maps/L_MainMenu");

	// The listen-server URL option. UGameplayStatics::OpenLevel composes it as
	// "L_Arena?listen" - the exact travel the signed doc's D2 latch keys on at
	// InitGame (TASK-356's half of the contract).
	static const TCHAR* ListenOption = TEXT("listen");
}

void USiegeSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Bind the engine-level failure surfaces ONCE for the whole game instance
	// lifetime (doc section 5). GEngine is valid during GameInstance init on
	// every real path; the null check is a defensive belt.
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &USiegeSessionSubsystem::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &USiegeSessionSubsystem::HandleTravelFailure);
	}
	else
	{
		UE_LOG(LogSiegeNet, Warning, TEXT("[SiegeSession] Initialize: GEngine is null - network/travel failure handlers NOT bound."));
	}

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USiegeSessionSubsystem::HandlePostLoadMap);

	UE_LOG(LogSiegeNet, Log, TEXT("[SiegeSession] Session subsystem initialized (listen-server + LAN/direct-IP scope, M8 P1)."));
}

void USiegeSessionSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}
	NetworkFailureHandle.Reset();
	TravelFailureHandle.Reset();

	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	PostLoadMapHandle.Reset();

	UE_LOG(LogSiegeNet, Log, TEXT("[SiegeSession] Session subsystem deinitialized."));

	Super::Deinitialize();
}

void USiegeSessionSubsystem::HostListenMatch()
{
	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogSiegeNet, Error, TEXT("[SiegeSession] HostListenMatch: no world - cannot travel."));
		OnSessionError.Broadcast(TEXT("Unable to host: no active world."));
		return;
	}

	UE_LOG(LogSiegeNet, Log, TEXT("[SiegeSession] HostListenMatch: opening '%s' as a LISTEN server (host = server + Blue, doc D1)."),
		SiegeSessionInternal::ArenaMapPath);
	OnSessionStatus.Broadcast(TEXT("Hosting - waiting for opponent"));

	// bAbsolute travel; OpenLevel composes the option as "?listen" - the URL
	// half of TASK-356's bNetworkedMatch latch (doc D2).
	UGameplayStatics::OpenLevel(this, FName(SiegeSessionInternal::ArenaMapPath), true, SiegeSessionInternal::ListenOption);
}

void USiegeSessionSubsystem::JoinMatch(const FString& Address)
{
	FString TravelAddress;
	FString ParseError;
	if (!ParseJoinAddress(Address, TravelAddress, ParseError))
	{
		// Invalid input NEVER travels - it degrades to a user-facing error and
		// the menu stays interactive (doc section 5, gate (f)).
		UE_LOG(LogSiegeNet, Warning, TEXT("[SiegeSession] JoinMatch: rejected address '%s' - %s"), *Address, *ParseError);
		OnSessionError.Broadcast(ParseError);
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogSiegeNet, Error, TEXT("[SiegeSession] JoinMatch: no world - cannot travel."));
		OnSessionError.Broadcast(TEXT("Unable to join: no active world."));
		return;
	}

	// The menu world's single LOCAL player controller carries the client
	// travel. This is session plumbing, not gameplay identity - the signed doc
	// (section 5) notes this resolve explicitly against the
	// GetFirstPlayerController gameplay ban.
	APlayerController* LocalController = GameInstance->GetFirstLocalPlayerController(World);
	if (!LocalController)
	{
		UE_LOG(LogSiegeNet, Error, TEXT("[SiegeSession] JoinMatch: no local player controller - cannot travel."));
		OnSessionError.Broadcast(TEXT("Unable to join: no local player."));
		return;
	}

	UE_LOG(LogSiegeNet, Log, TEXT("[SiegeSession] JoinMatch: client travel to '%s' (direct IP, doc D11/ruling 2)."), *TravelAddress);
	OnSessionStatus.Broadcast(FString::Printf(TEXT("Joining %s ..."), *TravelAddress));

	bJoinInFlight = true;
	LocalController->ClientTravel(TravelAddress, ETravelType::TRAVEL_Absolute);
}

void USiegeSessionSubsystem::LeaveMatch()
{
	if (bReturnToMenuInFlight)
	{
		UE_LOG(LogSiegeNet, Log, TEXT("[SiegeSession] LeaveMatch: a return-to-menu is already in flight - ignored (idempotent)."));
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogSiegeNet, Error, TEXT("[SiegeSession] LeaveMatch: no world - cannot travel."));
		OnSessionError.Broadcast(TEXT("Unable to return to the menu: no active world."));
		return;
	}

	bReturnToMenuInFlight = true;
	UE_LOG(LogSiegeNet, Log, TEXT("[SiegeSession] LeaveMatch: absolute travel to '%s' (NetMode=%d). Host role drops the remote client; client role disconnects."),
		SiegeSessionInternal::MenuMapPath, static_cast<int32>(World->GetNetMode()));
	OnSessionStatus.Broadcast(TEXT("Returning to the main menu..."));

	UGameplayStatics::OpenLevel(this, FName(SiegeSessionInternal::MenuMapPath), true);
}

bool USiegeSessionSubsystem::ParseJoinAddress(const FString& RawAddress, FString& OutTravelAddress, FString& OutError)
{
	OutTravelAddress.Reset();
	OutError.Reset();

	const FString Trimmed = RawAddress.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		OutError = TEXT("Enter an IP address to join (e.g. 192.168.1.50 or 192.168.1.50:7777).");
		return false;
	}

	FString HostPart = Trimmed;
	int32 Port = DefaultJoinPort;

	// Optional ":port" suffix. Exactly one ':' is allowed - the join scope is
	// IPv4-only (ruling 2), and an IPv6 literal would carry several.
	int32 FirstColonIndex = INDEX_NONE;
	if (Trimmed.FindChar(TEXT(':'), FirstColonIndex))
	{
		int32 LastColonIndex = INDEX_NONE;
		Trimmed.FindLastChar(TEXT(':'), LastColonIndex);
		if (LastColonIndex != FirstColonIndex)
		{
			OutError = TEXT("IPv6-style addresses are not supported - use an IPv4 address like 192.168.1.50:7777.");
			return false;
		}

		HostPart = Trimmed.Left(FirstColonIndex);
		const FString PortPart = Trimmed.Mid(FirstColonIndex + 1);

		bool bPortDigitsOnly = !PortPart.IsEmpty() && PortPart.Len() <= 5;
		if (bPortDigitsOnly)
		{
			for (const TCHAR PortChar : PortPart)
			{
				if (!FChar::IsDigit(PortChar))
				{
					bPortDigitsOnly = false;
					break;
				}
			}
		}

		const int32 ParsedPort = bPortDigitsOnly ? FCString::Atoi(*PortPart) : 0;
		if (!bPortDigitsOnly || ParsedPort < 1 || ParsedPort > 65535)
		{
			OutError = FString::Printf(TEXT("Invalid port '%s' - use a number from 1 to 65535 (default %d)."), *PortPart, DefaultJoinPort);
			return false;
		}
		Port = ParsedPort;
	}

	// Strict IPv4 dotted quad: exactly four parts, each non-empty, digits-only,
	// at most three characters, value 0..255. Empty parts are kept by the
	// non-culling split so inputs like "1..2.3" fail the count/empty checks.
	TArray<FString> Octets;
	HostPart.ParseIntoArray(Octets, TEXT("."), /*InCullEmpty*/ false);
	bool bValidHost = (Octets.Num() == 4);
	if (bValidHost)
	{
		for (const FString& Octet : Octets)
		{
			if (Octet.IsEmpty() || Octet.Len() > 3)
			{
				bValidHost = false;
				break;
			}
			for (const TCHAR OctetChar : Octet)
			{
				if (!FChar::IsDigit(OctetChar))
				{
					bValidHost = false;
					break;
				}
			}
			if (!bValidHost)
			{
				break;
			}
			// Digits-only and at most 3 characters: Atoi cannot overflow here.
			if (FCString::Atoi(*Octet) > 255)
			{
				bValidHost = false;
				break;
			}
		}
	}

	if (!bValidHost)
	{
		OutError = FString::Printf(TEXT("'%s' is not a valid IPv4 address (expected e.g. 192.168.1.50 or 192.168.1.50:7777)."), *Trimmed);
		return false;
	}

	OutTravelAddress = FString::Printf(TEXT("%s:%d"), *HostPart, Port);
	return true;
}

void USiegeSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// Only react to OUR game instance's world - PIE runs several game
	// instances in one process (the doc's Lane A verification path).
	if (World && World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	// Log VERBATIM first (doc section 5 / gate (h): net warnings recorded).
	UE_LOG(LogSiegeNet, Error, TEXT("[SiegeSession] NetworkFailure %s (world '%s'): %s"),
		ENetworkFailure::ToString(FailureType), *GetNameSafe(World), *ErrorString);

	const ENetMode NetMode = World ? World->GetNetMode() : NM_Standalone;

	// A join in flight: the pending connection died (the bad-IP timeout path
	// fires HERE, while the menu world is still current and standalone).
	if (bJoinInFlight)
	{
		bJoinInFlight = false;
		OnSessionError.Broadcast(FString::Printf(TEXT("Could not join: %s"), *ErrorString));
		if (NetMode == NM_Client)
		{
			// Rare: the failure hit AFTER we left the menu world - fall back.
			LeaveMatch();
		}
		// Otherwise we never left the menu - nothing to travel; the menu stays
		// interactive with the error shown (gate (f)).
		return;
	}

	// Net-driver-level failures break the session on ANY role.
	const bool bDriverLevelFailure =
		FailureType == ENetworkFailure::NetDriverAlreadyExists ||
		FailureType == ENetworkFailure::NetDriverCreateFailure ||
		FailureType == ENetworkFailure::NetDriverListenFailure;

	if (NetMode == NM_Client || bDriverLevelFailure)
	{
		OnSessionError.Broadcast(FString::Printf(TEXT("Connection failed: %s"), *ErrorString));
		LeaveMatch();
		return;
	}

	// Host role, connection-level failure: the JOINER dropped. Keep the host's
	// world - the host must not be yanked to the menu because the opponent
	// disconnected (flagged reading of "auto-LeaveMatch on hard failures" in
	// the TASK-354 handoff).
	OnSessionStatus.Broadcast(TEXT("Opponent disconnected."));
}

void USiegeSessionSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (World && World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	UE_LOG(LogSiegeNet, Error, TEXT("[SiegeSession] TravelFailure %s (world '%s'): %s"),
		ETravelFailure::ToString(FailureType), *GetNameSafe(World), *ErrorString);

	bJoinInFlight = false;
	OnSessionError.Broadcast(FString::Printf(TEXT("Travel failed: %s"), *ErrorString));

	if (bReturnToMenuInFlight)
	{
		// The return-to-menu travel ITSELF failed - do not retry in a loop;
		// clear the latch so a manual LeaveMatch/Back can try again.
		bReturnToMenuInFlight = false;
		UE_LOG(LogSiegeNet, Error, TEXT("[SiegeSession] TravelFailure hit during the return-to-menu travel - not auto-retrying."));
		return;
	}

	const ENetMode NetMode = World ? World->GetNetMode() : NM_Standalone;
	if (NetMode != NM_Standalone)
	{
		// Networked world in a failed travel: never strand the player - menu.
		LeaveMatch();
	}
	// Standalone (e.g. the failure fired while the menu world is current):
	// stay where we are with the error shown.
}

void USiegeSessionSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	bReturnToMenuInFlight = false;
	bJoinInFlight = false;

	UE_LOG(LogSiegeNet, Log, TEXT("[SiegeSession] Arrived in map '%s' (NetMode=%d)."),
		*LoadedWorld->GetMapName(), static_cast<int32>(LoadedWorld->GetNetMode()));
}
