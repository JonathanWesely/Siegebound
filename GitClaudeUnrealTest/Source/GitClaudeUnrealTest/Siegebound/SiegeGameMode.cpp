// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeGameMode.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "GitClaudeUnrealTest.h"
#include "Siegebound/Building.h"
#include "Siegebound/Castle.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/Projectile.h"
#include "Siegebound/SiegeGameState.h"
#include "Siegebound/SiegePlayerController.h"
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SummonedUnit.h"
#include "Siegebound/Tower.h"
#include "TimerManager.h"

ASiegeGameMode::ASiegeGameMode()
{
	// Framework classes per the TASK-006 spec (TASK-005 / TASK-007 contracts);
	// GameStateClass per TASK-024 (match clock + overtime latch, GDD §3.2).
	GameStateClass = ASiegeGameState::StaticClass();
	PlayerStateClass = ASiegePlayerState::StaticClass();
	PlayerControllerClass = ASiegePlayerController::StaticClass();

	// Hard fallback so anything reading the property directly always gets a
	// spawnable pawn. The real default — BP_HeroCharacter — is resolved lazily
	// and null-safe in GetDefaultPawnClassForController (see header: the
	// blueprint is produced in parallel by TASK-009 and may not exist yet, so
	// a constructor-time ConstructorHelpers::FClassFinder would log a load
	// error on every CDO construction until it does).
	DefaultPawnClass = AHeroCharacter::StaticClass();

	// Content contract (TASKBOARD TASK-006 names block / CONVENTIONS.md).
	HeroPawnClassAsset = TSoftClassPtr<AHeroCharacter>(FSoftObjectPath(TEXT("/Game/Blueprints/BP_HeroCharacter.BP_HeroCharacter_C")));
}

void ASiegeGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Both castles are level-placed (integration puts Castle_Blue / Castle_Red in
	// L_Arena — M1 manager decisions), so every ACastle exists by BeginPlay.
	// AddUniqueDynamic keeps a hypothetical re-entry idempotent.
	int32 BoundCastles = 0;
	for (TActorIterator<ACastle> It(GetWorld()); It; ++It)
	{
		It->OnCastleDestroyed.AddUniqueDynamic(this, &ASiegeGameMode::OnCastleDestroyedHandler);
		++BoundCastles;
	}

	if (BoundCastles == 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] No ACastle actors found at BeginPlay — the win condition is unreachable. Integration places Castle_Blue/Castle_Red in L_Arena."),
			*GetNameSafe(this));
	}
}

void ASiegeGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Own-timer hygiene only — this class never touches other objects' timers.
	GetWorldTimerManager().ClearTimer(HeroRespawnTimerHandle);

	Super::EndPlay(EndPlayReason);
}

UClass* ASiegeGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	return ResolveHeroPawnClass();
}

UClass* ASiegeGameMode::ResolveHeroPawnClass()
{
	if (ResolvedHeroPawnClass)
	{
		return ResolvedHeroPawnClass;
	}

	// TSoftClassPtr<AHeroCharacter>::LoadSynchronous already returns nullptr for
	// a class that is not an AHeroCharacter subclass, so a successful load is
	// guaranteed compatible. Only success is cached: if the blueprint appears
	// later in the session (editor import), the next spawn picks it up.
	if (UClass* LoadedClass = HeroPawnClassAsset.LoadSynchronous())
	{
		ResolvedHeroPawnClass = LoadedClass;
		return ResolvedHeroPawnClass;
	}

	if (!bWarnedHeroClassMissing)
	{
		bWarnedHeroClassMissing = true;
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] Hero pawn blueprint '%s' unavailable (created in TASK-009) — falling back to the raw AHeroCharacter (meshless but playable)."),
			*GetNameSafe(this), *HeroPawnClassAsset.ToString());
	}

	return AHeroCharacter::StaticClass();
}

void ASiegeGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);

	// FinishRestartPlayer calls this for EVERY pawn the mode hands to a player —
	// the initial spawn and any RestartPlayerAtPlayerStart fallback — so the
	// death binding always exists on the current hero, including a fresh pawn
	// spawned after the tracked one was destroyed.
	if (AHeroCharacter* Hero = Cast<AHeroCharacter>(PlayerPawn))
	{
		TrackedHero = Hero;
		Hero->OnHeroDied.AddUniqueDynamic(this, &ASiegeGameMode::HandleHeroDied);
	}
}

void ASiegeGameMode::OnCastleDestroyedHandler(ACastle* DestroyedCastle, ETeamId CastleTeam)
{
	// Double match-end guard: two castles falling in the same frame, or any
	// duplicate broadcast, can never end the match twice. Only PlayAgain()
	// re-arms this latch — a match cannot end any other way (GDD §3.9).
	if (bMatchEnded)
	{
		return;
	}
	bMatchEnded = true;

	// The team whose castle FELL loses; the other team wins (TASK-002 handoff):
	// Red castle destroyed -> Victory (Winner = Blue); Blue destroyed -> Defeat
	// variant (Winner = Red) — wired even though nothing damages Blue in M1.
	const ETeamId Winner = (CastleTeam == ETeamId::Red) ? ETeamId::Blue : ETeamId::Red;

	// Cancel any pending hero respawn: nothing revives under the end screen —
	// PlayAgain() owns hero restoration from here. (Own timer handle only.)
	GetWorldTimerManager().ClearTimer(HeroRespawnTimerHandle);

	UE_LOG(LogGitClaudeUnrealTest, Log, TEXT("[%s] Castle '%s' (%s) destroyed — match over, winner: %s."),
		*GetNameSafe(this), *GetNameSafe(DestroyedCastle),
		CastleTeam == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"),
		Winner == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));

	// Freeze the world BEFORE the end screen goes up (§3.9 / M2 exit criteria,
	// TASK-024 — closes the qa/TASK-006-report.md finding-2 TODO(M2)): units,
	// towers, in-flight projectiles, income, and the match clock all stop here.
	FreezeWorldAtMatchEnd();

	// Push the result to the (M1: single local) player controller(s) — shows the
	// end screen and switches to UI-only input (TASK-007 contract).
	bool bNotifiedAnyController = false;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASiegePlayerController* SiegePC = Cast<ASiegePlayerController>(It->Get()))
		{
			SiegePC->HandleMatchEnd(Winner);
			bNotifiedAnyController = true;
		}
	}

	if (!bNotifiedAnyController)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] Match ended but no ASiegePlayerController was found to show the end screen."), *GetNameSafe(this));
	}
}

void ASiegeGameMode::FreezeWorldAtMatchEnd()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1) Units: permanent, idempotent AI stop (ASummonedUnit::FreezeAI,
	//    TASK-028 contract) — movement, attack/acquire timers, and any
	//    in-flight lunge all end with zero residual offset; frozen units idle
	//    until PlayAgain destroys them. FreezeAI never spawns or destroys
	//    actors, so calling it inside a live iterator is safe.
	int32 FrozenUnits = 0;
	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		It->FreezeAI();
		++FrozenUnits;
	}

	// 2) Towers (qa/TASK-027-report.md WARN-1): a live ATower would keep
	//    acquiring and firing at the frozen units under the Victory screen
	//    (bAIFrozen units are alive to its acquisition gate). Tower.h/.cpp are
	//    frozen qa-passed contracts — private fire-timer handle, no public
	//    stop hook — so the loop is silenced through FTimerManager's public
	//    per-object surface instead. Safe and permanent: the fire loop is the
	//    ONLY timer a tower ever arms (qa/TASK-027 verified), OnStatsLoaded is
	//    single-fire so nothing can re-arm it, and PlayAgain destroys every
	//    building regardless. This is the ONE sanctioned exception to the
	//    own-timers-only policy — see the class comment.
	int32 SilencedTowers = 0;
	for (TActorIterator<ATower> It(World); It; ++It)
	{
		World->GetTimerManager().ClearAllTimersForObject(*It);
		++SilencedTowers;
	}

	// 3) In-flight projectiles (qa/TASK-026-report.md WARN-1): one landing
	//    after this frame would deal post-match damage under the Victory
	//    screen. Destroyed rather than frozen — a projectile hanging in midair
	//    under the end screen would just be debris. Collected first: never
	//    Destroy() out of a live TActorIterator. With units frozen and towers
	//    silenced above, nothing can spawn a new one until PlayAgain.
	TArray<AProjectile*> Projectiles;
	for (TActorIterator<AProjectile> It(World); It; ++It)
	{
		Projectiles.Add(*It);
	}
	for (AProjectile* Projectile : Projectiles)
	{
		if (IsValid(Projectile))
		{
			Projectile->Destroy();
		}
	}

	// 4) Income (§3.9 freeze): every player state — §3.2's overtime symmetry
	//    means every ASiegePlayerState shares the frozen state (M3 bot ready).
	//    PauseIncome is the player state's OWN API (timer policy intact).
	if (GameState)
	{
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (ASiegePlayerState* SiegePS = Cast<ASiegePlayerState>(PS))
			{
				SiegePS->PauseIncome();
			}
		}
	}

	// 5) Match clock: frozen at the final time until PlayAgain resets it.
	if (ASiegeGameState* SiegeGameState = Cast<ASiegeGameState>(GameState))
	{
		SiegeGameState->StopClock();
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] Match-end freeze: %d unit(s) frozen, %d tower(s) silenced, %d projectile(s) cleared, income paused, clock stopped."),
		*GetNameSafe(this), FrozenUnits, SilencedTowers, Projectiles.Num());
}

void ASiegeGameMode::HandleHeroDied(AHeroCharacter* DeadHero)
{
	if (!IsValid(DeadHero))
	{
		return;
	}

	TrackedHero = DeadHero;

	// After match end the hero stays down until PlayAgain() (GDD §3.9); the
	// match-end handler also cancels any respawn already pending.
	if (bMatchEnded)
	{
		return;
	}

	// Exactly HeroRespawnDelay (5 s, §3.1: back within 5-6 s) later the hero is
	// back at its own-castle side. SetTimer on the same handle replaces any
	// pending timer, so respawns can never stack (FOnHeroDied fires once per
	// death anyway — TASK-003 contract).
	GetWorldTimerManager().SetTimer(HeroRespawnTimerHandle, this, &ASiegeGameMode::RespawnHero, HeroRespawnDelay, false);
}

void ASiegeGameMode::RespawnHero()
{
	RestoreHeroAtStart();
}

void ASiegeGameMode::RestoreHeroAtStart()
{
	ASiegePlayerController* SiegePC = FindLocalSiegeController();

	// The tracked hero is authoritative; fall back to whatever the controller
	// possesses (covers a tracked pointer lost to GC after a destroy).
	AHeroCharacter* Hero = IsValid(TrackedHero) ? TrackedHero.Get() : nullptr;
	if (!Hero && SiegePC)
	{
		Hero = Cast<AHeroCharacter>(SiegePC->GetPawn());
	}

	// Prefer the hero's own controller — death disables input WITHOUT
	// unpossessing (TASK-003), so this is normally still the player.
	APlayerController* PC = Hero ? Cast<APlayerController>(Hero->GetController()) : nullptr;
	if (!PC)
	{
		PC = SiegePC;
	}

	if (!IsValid(Hero))
	{
		// Pawn gone entirely (no M1 flow destroys it — defensive): hand the
		// player a fresh default pawn at the start. FinishRestartPlayer calls
		// SetPlayerDefaults, which re-binds OnHeroDied on the new pawn.
		if (PC)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("[%s] Hero pawn missing at respawn — restarting the player with a fresh default pawn."), *GetNameSafe(this));
			RestartPlayerAtPlayerStart(PC, FindPlayerStart(PC));
		}
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Error,
				TEXT("[%s] Cannot respawn the hero: no pawn and no player controller."), *GetNameSafe(this));
		}
		return;
	}

	FVector StartLocation = FVector::ZeroVector;
	FRotator StartRotation = FRotator::ZeroRotator;
	GetHeroStartTransform(PC, Hero->GetTeamId(), StartLocation, StartRotation);

	// Teleport first — a dead hero is still hidden here, so the death spot
	// never flashes on screen. No sweep: the start point is clear by design.
	Hero->SetActorLocationAndRotation(StartLocation, StartRotation, /*bSweep*/ false, /*OutSweepHitResult*/ nullptr, ETeleportType::TeleportPhysics);

	// Repossess BEFORE ResetHero: its EnableInput mirrors death's DisableInput
	// against the possessing controller (TASK-003 handoff). Normally the
	// controller never lost the pawn and this is skipped.
	if (PC && PC->GetPawn() != Hero)
	{
		PC->Possess(Hero);
	}

	// Full HP, visible, collision + movement + input restored (TASK-003 contract).
	Hero->ResetHero();

	// Face the hero's spawn direction (PlayerStart yaw 0 looks across the arena).
	if (PC)
	{
		PC->SetControlRotation(StartRotation);
	}
}

void ASiegeGameMode::GetHeroStartTransform(AController* Player, ETeamId HeroTeam, FVector& OutLocation, FRotator& OutRotation)
{
	// 1) The level's PlayerStart (L_Arena: (-1700, 0, 100) yaw 0 on the Blue
	//    side — TASK-015). FindPlayerStart falls back to WorldSettings when the
	//    level has no PlayerStart; that is not a spawn point, so only a real
	//    APlayerStart is accepted here.
	if (AActor* Start = FindPlayerStart(Player))
	{
		if (Start->IsA<APlayerStart>())
		{
			OutLocation = Start->GetActorLocation();
			OutRotation = FRotator(0.0f, Start->GetActorRotation().Yaw, 0.0f);
			return;
		}
	}

	// 2) The hero's own-castle side (§3.1): the own-team castle's position,
	//    offset toward the centerline (X=0, CONVENTIONS world axes) so the spawn
	//    clears the castle footprint, facing the enemy half.
	for (TActorIterator<ACastle> It(GetWorld()); It; ++It)
	{
		if (It->GetTeamId() == HeroTeam)
		{
			const float TowardCenterline = (It->GetActorLocation().X <= 0.0f) ? 1.0f : -1.0f;
			OutLocation = It->GetActorLocation() + FVector(HeroSpawnCastleOffset.X * TowardCenterline, HeroSpawnCastleOffset.Y, HeroSpawnCastleOffset.Z);
			OutRotation = FRotator(0.0f, (TowardCenterline > 0.0f) ? 0.0f : 180.0f, 0.0f);
			return;
		}
	}

	// 3) Last resort — arena origin, above the Z=0 ground plane (TASK-015).
	UE_LOG(LogGitClaudeUnrealTest, Warning,
		TEXT("[%s] No PlayerStart and no own-team castle found — respawning the hero at the arena origin."), *GetNameSafe(this));
	OutLocation = FVector(0.0f, 0.0f, 100.0f);
	OutRotation = FRotator::ZeroRotator;
}

void ASiegeGameMode::PlayAgain()
{
	// Robust to double invocation: re-entrant calls (e.g. a double-clicked Play
	// Again button dispatching twice in one frame) are dropped here, and a
	// second SEQUENTIAL call simply re-runs steps that are all idempotent.
	if (bPlayAgainInProgress)
	{
		return;
	}
	TGuardValue<bool> ReentrancyGuard(bPlayAgainInProgress, true);

	UE_LOG(LogGitClaudeUnrealTest, Log, TEXT("[%s] PlayAgain: full match reset (GDD §3.9, M2 scope)."), *GetNameSafe(this));

	// 1) Timers — QA-BINDING (TASKBOARD TASK-006 qa-note, from qa/TASK-005-report.md
	//    major 1): ASiegePlayerState::ResetGold() RESTARTS the income timer, so any
	//    timer clearing must happen BEFORE step 4, never after. We clear ONLY the
	//    specific handle this class owns — never ClearAllTimersForObject on foreign
	//    objects and never a world-wide clear, which could silently kill the income
	//    timer or other systems' timers. (The match-END freeze's tower silencing is
	//    the one sanctioned exception — see the class comment; it never runs in this
	//    function.) The units' and buildings' own timers die with their actors in
	//    steps 2/2b (their EndPlay clears them); the PlayerState manages its own.
	GetWorldTimerManager().ClearTimer(HeroRespawnTimerHandle);

	// 2) Zero summoned units (§3.9). Collected first — never Destroy() out of a
	//    live TActorIterator.
	TArray<ASummonedUnit*> Units;
	for (TActorIterator<ASummonedUnit> It(GetWorld()); It; ++It)
	{
		Units.Add(*It);
	}
	for (ASummonedUnit* Unit : Units)
	{
		if (IsValid(Unit))
		{
			Unit->Destroy();
		}
	}

	// 2b) Zero buildings (§3.9 "Play Again (full state reset: ... buildings)",
	//     TASK-024 — ABuilding is the TASK-027 contract; ATower included by
	//     inheritance). Same collect-then-destroy pattern. Safe: ATower::EndPlay
	//     clears its own fire timer synchronously inside Destroy(), and a dead
	//     wall's mesh unregisters from the nav octree so the navmesh heals.
	TArray<ABuilding*> Buildings;
	for (TActorIterator<ABuilding> It(GetWorld()); It; ++It)
	{
		Buildings.Add(*It);
	}
	for (ABuilding* Building : Buildings)
	{
		if (IsValid(Building))
		{
			Building->Destroy();
		}
	}

	// 2c) Clear in-flight projectiles (qa/TASK-026-report.md WARN-1): one still
	//     flying across the reset could hit a freshly reset castle for stale
	//     damage. Load-bearing for a MID-MATCH PlayAgain — units/towers may
	//     have fired this very frame; after a normal match end the freeze
	//     already swept them and this is a no-op.
	TArray<AProjectile*> Projectiles;
	for (TActorIterator<AProjectile> It(GetWorld()); It; ++It)
	{
		Projectiles.Add(*It);
	}
	for (AProjectile* Projectile : Projectiles)
	{
		if (IsValid(Projectile))
		{
			Projectile->Destroy();
		}
	}

	// 3) Castles back to 2000/2000, visible, colliding. ResetCastle() also
	//    re-arms OnCastleDestroyed; our BeginPlay binding persists on the actor,
	//    so the win condition works again without rebinding.
	for (TActorIterator<ACastle> It(GetWorld()); It; ++It)
	{
		It->ResetCastle();
	}

	// 3b) Match clock back to 0:00, overtime latch cleared, clock running again
	//     (§3.9 "match clock"; the §3.2 doubling re-arms for the new match).
	//     MUST precede step 4: ResetEconomy() re-derives each player's gold
	//     rate by reading this latch live — clearing it first lands the rate
	//     on the base 2/s.
	if (ASiegeGameState* SiegeGameState = Cast<ASiegeGameState>(GameState))
	{
		SiegeGameState->ResetClock();
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] PlayAgain: no ASiegeGameState — match clock/overtime not reset (GameStateClass should be ASiegeGameState, TASK-024)."),
			*GetNameSafe(this));
	}

	// 4) Economy + gold (§3.2/§3.3/§3.9). Safe AFTER step 1: ResetGold()
	//    restarts its own income timer and nothing later in this function
	//    clears any timer. Per player state, in order: ResetEconomy (miner
	//    counts to 0, rate re-derived against the just-cleared overtime latch),
	//    ResetGold (back to 50; restarts income — M1 law), ResumeIncome (lifts
	//    the match-end pause; idempotent when never paused — PlayAgain is a
	//    legal mid-match reset).
	if (GameState)
	{
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (ASiegePlayerState* SiegePS = Cast<ASiegePlayerState>(PS))
			{
				SiegePS->ResetEconomy();
				SiegePS->ResetGold();
				SiegePS->ResumeIncome();
			}
		}
	}

	// 5) Re-arm the win condition, then the hero back at its start: full HP,
	//    repossessed, input restored (works for a dead OR alive hero).
	bMatchEnded = false;
	RestoreHeroAtStart();

	// 6) Controllers last: drop the end screen (idempotent with the widget's own
	//    RemoveFromParent) and restore game-only input (TASK-007 contract),
	//    then a fresh deck + hand (§3.9 "deck, hand" — TASK-022 contract).
	//    HandleMatchReset is now the SINGLE §3.9 deck-reset entry point:
	//    TASK-023 wired DeckComponent->ResetDeck() into it (null-safe there),
	//    so the game mode no longer resets the deck a second time. This closes
	//    qa/TASK-024-report.md WARN-1 (PlayAgain double ResetDeck) — the
	//    game-mode-side drop was authorized on TASK-030 (handoffs/TASK-030.md).
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASiegePlayerController* SiegePC = Cast<ASiegePlayerController>(It->Get()))
		{
			SiegePC->HandleMatchReset();
		}
	}
}

ASiegePlayerController* ASiegeGameMode::FindLocalSiegeController() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// M1 is strictly single local player (CONVENTIONS: the local player is
	// always Blue) — the first ASiegePlayerController is THE player.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASiegePlayerController* SiegePC = Cast<ASiegePlayerController>(It->Get()))
		{
			return SiegePC;
		}
	}

	return nullptr;
}
