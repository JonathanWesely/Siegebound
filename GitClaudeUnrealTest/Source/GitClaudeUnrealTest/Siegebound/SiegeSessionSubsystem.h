// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/EngineBaseTypes.h"
#include "SiegeSessionSubsystem.generated.h"

class UNetDriver;
class UWorld;

/**
 *  The M8 net log category (CONVENTIONS "Networked 1v1 (M8)", the LogSiegeNet
 *  ruling): declared/defined in SiegeSessionSubsystem.{h,cpp} by law, usable by
 *  any M8 code - both P1 lanes compile together at TASK-357 integration.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeNet, Log, All);

/**
 *  One dynamic multicast delegate type for session text surfaces (FString-only,
 *  widget law). The subsystem exposes TWO instances of it - OnSessionStatus and
 *  OnSessionError - which USessionMenuWidget forwards into its two
 *  BlueprintImplementableEvents (TASK-353 architecture doc, section 5).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionStatusChanged, const FString&, Message);

/**
 *  M8 P1 session plumbing (TASK-354; the SIGNED TASK-353 architecture doc,
 *  section 5, decisions D1/D11): a UGameInstanceSubsystem that owns Host/Join/
 *  Leave for the networked 1v1. It survives level travel by design, which is
 *  exactly why the failure handlers live here and not on any actor.
 *
 *  Scope (manager rulings 1/2, D11): LISTEN SERVER + LAN/direct-IP ONLY.
 *  - Host = listen-server travel to L_Arena with the "listen" option (host =
 *    server + Blue, D1). The "?listen" URL option is one half of TASK-356's
 *    bNetworkedMatch latch (D2) - the latch itself lands in the GameMode at
 *    TASK-356; this subsystem only PRODUCES the travel URL.
 *  - Join = validated direct-IP client travel (the "open <ip>" equivalent).
 *  - NO OnlineSubsystem / EOS / Steam, no matchmaking, no lobby/ready flow,
 *    no reconnect, no seamless travel (all out of M8 P1 scope by ruling).
 *
 *  Practice mode ("Play vs Bot") is UNTOUCHED: that path enters through
 *  ASiegeGameMode::StartMatch and never routes through this subsystem. In a
 *  standalone session this object is inert unless a menu explicitly calls it
 *  (ruling 3 - single-player behavior stays byte-identical).
 *
 *  Error surfaces (doc section 5): GEngine OnNetworkFailure + OnTravelFailure
 *  are bound at Initialize, logged VERBATIM on LogSiegeNet, broadcast to the
 *  menu widget as user-facing text, and hard failures on the CLIENT role fall
 *  back to the main menu (gate (f): a bad IP degrades to an error message +
 *  menu, never a hang or crash). Every transition logs on LogSiegeNet.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/** Default UE game port, appended when the join address carries no ":port" (doc section 5). */
	static constexpr int32 DefaultJoinPort = 7777;

	//~ Begin USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface

	/**
	 *  HOST: open /Game/Maps/L_Arena as a LISTEN server (bAbsolute travel with
	 *  the "listen" option - UGameplayStatics::OpenLevel composes it into
	 *  "L_Arena?listen"). The host machine becomes server + Blue (D1). Null-safe:
	 *  no world resolves to a log + error broadcast and NO travel. Broadcasts the
	 *  "Hosting - waiting for opponent" status before traveling (doc section 5).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Session")
	void HostListenMatch();

	/**
	 *  JOIN: validate Address FIRST, then client-travel to it (TRAVEL_Absolute on
	 *  the menu world's single local player controller - session plumbing, not
	 *  gameplay identity; the doc notes this explicitly against the
	 *  GetFirstPlayerController ban, section 5). Accepted input: a trimmed IPv4
	 *  dotted quad with an optional ":port" (default port 7777). Anything else
	 *  is REJECTED with a user-facing error broadcast and NO travel is attempted
	 *  - an invalid address must never hang the game (gate (f)).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Session")
	void JoinMatch(const FString& Address);

	/**
	 *  LEAVE: absolute travel back to /Game/Maps/L_MainMenu on EITHER role.
	 *  On the host this tears down the listen server and drops the remote
	 *  client (whose own failure handler returns it to its menu); on a client
	 *  it disconnects. Idempotent: a return-to-menu already in flight is
	 *  ignored (the in-flight latch clears when the next map finishes loading).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Session")
	void LeaveMatch();

	/**
	 *  Strict IPv4[:port] validation (static, no side effects - QA/testable in
	 *  isolation). Success: OutTravelAddress = "a.b.c.d:port" (port defaulted to
	 *  DefaultJoinPort when absent) and true. Failure: OutError = user-facing
	 *  text and false.
	 *
	 *  NOTE (flagged in the TASK-354 handoff): the architecture doc names
	 *  FIPv4Address::Parse as the mechanism, but that type lives in the
	 *  "Networking" module which is NOT in this project's Build.cs - and
	 *  TASK-354 is NEW-FILES-ONLY (the M8 parallel law forbids editing the
	 *  unowned Build.cs when not truly needed). This self-contained parser is
	 *  deliberately EQUIVALENT-OR-STRICTER: exactly four dot-separated,
	 *  digits-only octets in 0..255, at most one ':', port digits-only in
	 *  1..65535.
	 */
	static bool ParseJoinAddress(const FString& RawAddress, FString& OutTravelAddress, FString& OutError);

	/**
	 *  Session STATUS text surface ("Hosting - waiting for opponent",
	 *  "Joining ...", "Opponent disconnected."). USessionMenuWidget forwards
	 *  this into its OnSessionStatusUpdated BlueprintImplementableEvent.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Session")
	FOnSessionStatusChanged OnSessionStatus;

	/**
	 *  Session ERROR text surface (invalid address, connection/travel failure).
	 *  USessionMenuWidget forwards this into its OnSessionErrorShown
	 *  BlueprintImplementableEvent.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Session")
	FOnSessionStatusChanged OnSessionError;

private:

	/**
	 *  GEngine->OnNetworkFailure() handler. Logs the failure VERBATIM, then:
	 *  - a failure while a join is in flight => error broadcast; if we already
	 *    left the menu world as a client, also fall back to the menu (a pending
	 *    connection that fails - the bad-IP timeout - dies while the menu world
	 *    is still current, so no travel is needed there);
	 *  - client role => error broadcast + LeaveMatch (never strand the joiner);
	 *  - net-DRIVER-level failures (listen/create/already-exists) => broken
	 *    session on ANY role => error broadcast + LeaveMatch;
	 *  - host role, connection-level failure (the joiner dropped) => status
	 *    broadcast + KEEP the host's world (the host must not be yanked to the
	 *    menu because the opponent disconnected).
	 */
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	/**
	 *  GEngine->OnTravelFailure() handler: log verbatim, error broadcast, then
	 *  fall back to the menu unless the failed travel WAS the return-to-menu
	 *  travel (bounded - never loops) or we are already standalone in a local
	 *  world (nowhere better to go than where we are).
	 */
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	/**
	 *  FCoreUObjectDelegates::PostLoadMapWithWorld handler: clears the
	 *  in-flight latches once a map load completes for OUR game instance and
	 *  logs the arrival (every transition logs on LogSiegeNet, doc section 5).
	 */
	void HandlePostLoadMap(UWorld* LoadedWorld);

	/** Handles for the engine delegates bound at Initialize, removed at Deinitialize. */
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
	FDelegateHandle PostLoadMapHandle;

	/** True while a LeaveMatch travel is in flight - makes LeaveMatch idempotent and failure handling loop-free. */
	bool bReturnToMenuInFlight = false;

	/** True between JoinMatch dispatching a client travel and the next map load (or failure) - disambiguates the bad-IP pending-connection failure, which fires while the MENU world is still current. */
	bool bJoinInFlight = false;
};
