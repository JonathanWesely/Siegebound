// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeGameMode.h"

#include "Engine/LocalPlayer.h" // ULocalPlayer::GetSubsystem — the MARK-§ M-4 clear on PlayAgain (TASK-744's cross-task line)
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "Siegebound/Barracks.h"
#include "Siegebound/BattlefieldScatter.h"
#include "Siegebound/Building.h"
#include "Siegebound/CaptureZone.h"
#include "Siegebound/Castle.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/Projectile.h"
#include "Siegebound/SiegeBotController.h"
#include "Siegebound/SiegeGameState.h"
#include "Siegebound/SiegeGhostPawn.h" // TASK-750 — produced in parallel by TASK-749 (one module, one compile at TASK-754)
#include "Siegebound/SiegeMapMarkSubsystem.h" // USiegeMapMarkSubsystem::ClearMarks — MARK-§ M-4's clear-on-reset (the class is TASK-744's; this file owns the ONE call site)
#include "Siegebound/SiegePlayerController.h"
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SiegeSessionSubsystem.h" // LogSiegeNet (CONVENTIONS M8)
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

	// AI opponent (GDD §4, TASK-045): the C++ bot brain by default. Unlike the
	// hero pawn (a content blueprint resolved lazily), this is a pure C++ class
	// with no asset dependency, so a direct StaticClass default is safe at
	// construction — no load, no missing-asset log.
	BotControllerClass = ASiegeBotController::StaticClass();

	// Death ghost (TASK-750, GHOST-§5), authored by TASK-763 and pointed at here by
	// TASK-764. ⛔ THIS LINE REPLACES A DELIBERATE UNSET, and the reason it was unset is
	// now spent rather than forgotten: TASK-750 left it blank because no task in the
	// GHOST batch produced a ghost blueprint, so a path to an asset nobody creates would
	// have warned on every match forever. BP_SiegeGhostPawn now exists (parent
	// ASiegeGhostPawn, all seven designer slots filled), so the resolver lands on its
	// authored-and-loaded branch and NEITHER log fires. Without this line nothing spawns
	// the blueprint: the property is EditDefaultsOnly and not config, DefaultEngine.ini
	// names the raw C++ ASiegeGameMode as GlobalDefaultGameMode, and there is no
	// BP_SiegeGameMode — so there is nowhere for an editor-side default to persist.
	// ⚠️ The `_C` suffix is load-bearing: without it the path resolves to the Blueprint
	// ASSET rather than its generated class, which fails at spawn while looking correct.
	GhostPawnClassAsset = TSoftClassPtr<ASiegeGhostPawn>(FSoftObjectPath(TEXT("/Game/Blueprints/BP_SiegeGhostPawn.BP_SiegeGhostPawn_C")));

	// Main-menu start-match target (GDD §7, TASK-047 → TASK-049). Soft world ref
	// (CONVENTIONS map /Game/Maps/L_Arena) — read from the CDO by the static
	// StartMatch; the menu never force-loads the arena until Play is pressed.
	ArenaLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Maps/L_Arena.L_Arena")));
}

void ASiegeGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Dev/test Sandbox latch (CONVENTIONS "Dev / test tooling", TASK-071): the
	// menu's "Sandbox (No Bot)" button opens L_Arena with ?Sandbox=1 via
	// StartSandboxMatch. InitGame runs once, at the very start of the world's life
	// and BEFORE BeginPlay/SpawnBot, so latching here makes the flag authoritative
	// for the whole match — and it survives Play Again (an in-place reset that
	// never re-runs InitGame), so a sandbox match stays sandbox. The token string
	// is exactly "Sandbox" (CONVENTIONS — code and any future consumer must match
	// it character-for-character).
	bSandboxMatch = UGameplayStatics::HasOption(Options, TEXT("Sandbox"));

	if (bSandboxMatch)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] Sandbox match (?Sandbox=1, TASK-071): no bot opponent will spawn; the Blue player will start with %d gold (dev test bench)."),
			*GetNameSafe(this), SandboxStartingGold);
	}

	// M8 networked-match latch, half 1 of the dual latch (TASK-356 doc §1.3/D2):
	// the real hosting path travels `L_Arena?listen` (USiegeSessionSubsystem::
	// HostListenMatch), and InitGame runs before anything else in the world's
	// life. Half 2 — the NetMode belt — lands in BeginPlay. Standalone/Play-vs-Bot
	// carries no `listen` option ⇒ false ⇒ byte-identical.
	bNetworkedMatch = UGameplayStatics::HasOption(Options, TEXT("listen"));

	// Sandbox refuses to network (audit §9 flag 5, accepted at the doc sign-off):
	// a networked sandbox would hand the joiner a bot-less dev bench — force the
	// sandbox latch OFF with one log line. Ordered here where both latches are
	// fresh (doc §1.3).
	if (bNetworkedMatch && bSandboxMatch)
	{
		bSandboxMatch = false;
		UE_LOG(LogSiegeNet, Warning,
			TEXT("[%s] ?Sandbox=1 ignored in a NETWORKED match (M8 doc §1.3) — running a normal 1v1 world."),
			*GetNameSafe(this));
	}

	if (bNetworkedMatch)
	{
		UE_LOG(LogSiegeNet, Log,
			TEXT("[%s] Networked match latched from the ?listen travel option (M8 doc D2)."), *GetNameSafe(this));
	}
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

	// M8 networked-match latch, half 2 — the NetMode BELT (TASK-356 doc §1.3/D2):
	// PIE "Play As Listen Server" does not reliably thread the ?listen option
	// through InitGame, but the net driver exists by BeginPlay on every listen
	// path — OR the NetMode in BEFORE the SpawnBot consumer below. Standalone:
	// NM_Standalone ⇒ no change ⇒ byte-identical.
	bNetworkedMatch |= (GetNetMode() != NM_Standalone);

	// Spawn the single Red bot opponent (GDD §4, TASK-045). GameState exists by
	// BeginPlay and the local player has already logged in (InitNewPlayer tagged
	// its PS Blue), so PlayerStateClass is set for the bot's auto-created PS.
	// In a Sandbox match SpawnBot early-returns — no bot, no Red PlayerState.
	// M8: a NETWORKED match early-returns too (the Red seat is human, doc §2.2).
	SpawnBot();

	// Sandbox test bench (TASK-071): grant the Blue player the generous starting
	// pile. Deferred one tick so the Blue ASiegePlayerState's own BeginPlay (which
	// seeds gold to StartingGold = 10, TASK-089, via ResetGold) has already run — granting
	// synchronously here could be clobbered by a later player-state seed. Fires
	// exactly once (BeginPlay runs once per world begin); Play Again re-grants on
	// its own path. No-op when not a sandbox match.
	if (bSandboxMatch)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ASiegeGameMode::GrantSandboxStartingGold);
	}
}

void ASiegeGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Own-timer hygiene only — this class never touches other objects' timers.
	// M8: the per-controller respawn map (doc §3.4.4) is the only timer set owned.
	for (TPair<TWeakObjectPtr<AController>, FTimerHandle>& RespawnPair : HeroRespawnTimers)
	{
		GetWorldTimerManager().ClearTimer(RespawnPair.Value);
	}
	HeroRespawnTimers.Empty();

	// TASK-750: drop the ghost bookkeeping with the handles it shadows. ⛔ NO Destroy()
	// and ⛔ no re-possession here — the world itself is ending, every actor in it is
	// about to be torn down, and re-possessing a hero during EndPlay would be
	// gameplay work on a world that no longer has a match. Weak pointers ⇒ emptying
	// the map is the whole of this class's obligation.
	ActiveGhosts.Empty();

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

// ─────────────────────────────────────────────────────────────────────────────
// THE DEATH LIFECYCLE'S TWO PURE SEAMS (TASK-750, GHOST-§2/§3)
//
// ⛔ NO WORLD, ⛔ NO MEMBER STATE, ⛔ NO SIDE EFFECTS — so the rules they encode
// can be exercised headlessly across their whole truth tables, which is the only
// way an assertion about them can actually FAIL (SHIP-§9c). Every call site in
// this file passes live state into them; there is no second copy of either rule.
// ─────────────────────────────────────────────────────────────────────────────

bool ASiegeGameMode::ShouldEnterGhostState(bool bInMatchEnded, bool bHasOwningController)
{
	// ⛔ THE MATCH-END CLAUSE IS INHERITED, NOT INVENTED (GHOST-§2, and the spec is
	// explicit that a second match-end rule would be the defect): the shipped header
	// contract is "After match end no respawn is scheduled" and "the hero stays down;
	// PlayAgain() revives it". A ghost is the visible half of a pending respawn, so
	// where there is no respawn there is no ghost — the end screen goes up over the
	// hero exactly as it did before this feature existed.
	if (bInMatchEnded)
	{
		return false;
	}

	// The shipped no-controller guard, unchanged in effect: nobody to possess a ghost,
	// nobody to respawn. (Death normally disables input WITHOUT unpossessing, so the
	// controller is still attached — this is the defensive edge, not the usual path.)
	return bHasOwningController;
}

AHeroCharacter* ASiegeGameMode::ResolveHeroToRestore(APawn* PossessedPawn, AHeroCharacter* TrackedHero)
{
	// Row 1 — the possessed pawn IS the hero. Every non-ghost path lands here and the
	// result is byte-identical to the shipped Cast<AHeroCharacter>(Player->GetPawn()).
	if (AHeroCharacter* PossessedHero = Cast<AHeroCharacter>(PossessedPawn))
	{
		return IsValid(PossessedHero) ? PossessedHero : nullptr;
	}

	// Row 2 — the GHOST is possessed, so the hero is the one recorded at death time.
	// ⛔⛔ THIS ROW IS THE DOUBLE-APPLY GUARD. Without it the respawn sees a non-hero
	// pawn and falls into RestoreHeroAtStart's defensive RestartPlayer branch, which
	// spawns a SECOND hero: the original is orphaned in the world still carrying every
	// upgrade stack, and ResetHero()'s cumulative re-apply (HeroCharacter.cpp — full HP
	// at GetEffectiveMaxHP, the War Banner aura re-armed, the loadout re-broadcast)
	// lands on the corpse rather than on the pawn the player is driving. ONE hero actor
	// lives across the whole death ⇒ ResetHero() runs exactly once, on exactly one pawn.
	//
	// Row 3 — neither resolves ⇒ nullptr, which is precisely the signal
	// RestoreHeroAtStart already handles by restarting the player with a fresh pawn.
	// ⛔ The ghost can never be returned: the return type is AHeroCharacter*, and
	// ASiegeGhostPawn is not an AHeroCharacter (GHOST-§1).
	return IsValid(TrackedHero) ? TrackedHero : nullptr;
}

UClass* ASiegeGameMode::ResolveGhostPawnClass()
{
	if (ResolvedGhostPawnClass)
	{
		return ResolvedGhostPawnClass;
	}

	// ⭐ THE UNSET CASE IS THE EXPECTED, SHIPPED CASE AND IS NOT A WARNING (see the
	// header): no task in this batch authors a ghost blueprint, so the raw C++ class is
	// the ghost. Logged once at Log so a reader of a live log can still see which class
	// is being spawned — the honest half of the HeroPawnClassAsset pattern, with the
	// alarm removed from the case that is not alarming.
	if (GhostPawnClassAsset.IsNull())
	{
		if (!bWarnedGhostClassMissing)
		{
			bWarnedGhostClassMissing = true;
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("[%s] No ghost pawn blueprint configured (GhostPawnClassAsset has been cleared — the shipped default is BP_SiegeGhostPawn, TASK-764) — using the raw C++ ASiegeGhostPawn."),
				*GetNameSafe(this));
		}
		return ASiegeGhostPawn::StaticClass();
	}

	// TSoftClassPtr<ASiegeGhostPawn>::LoadSynchronous already returns nullptr for a
	// class that is not an ASiegeGhostPawn subclass, so a successful load is guaranteed
	// compatible. Only success is cached: a blueprint imported later in an editor
	// session is picked up by the next death.
	if (UClass* LoadedClass = GhostPawnClassAsset.LoadSynchronous())
	{
		ResolvedGhostPawnClass = LoadedClass;
		return ResolvedGhostPawnClass;
	}

	// AUTHORED BUT UNRESOLVABLE — a real mis-configuration, and the one that deserves a
	// Warning. ⛔ Still never fatal: the fallback below is a fully functional ghost.
	if (!bWarnedGhostClassMissing)
	{
		bWarnedGhostClassMissing = true;
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] Ghost pawn blueprint '%s' is configured but could not be loaded (missing or not an ASiegeGhostPawn) — falling back to the raw C++ ASiegeGhostPawn."),
			*GetNameSafe(this), *GhostPawnClassAsset.ToString());
	}

	return ASiegeGhostPawn::StaticClass();
}

FString ASiegeGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	// Super creates/finishes the player state assignment (the engine sets the
	// player name / unique id on NewPlayerController->PlayerState in here).
	const FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	// M8 SEAT LATCH (TASK-356 doc §2.1/D3 — retires the audit-§1b#2 unconditional
	// Blue tag, the single most load-bearing team bug for P1): first login takes
	// the Blue seat (the host — on a listen server the local player always logs
	// in first), second takes Red (the joiner), third+ is warned onto Red (P1
	// has no kick logic). Logins serialize on the server game thread, so the
	// order is deterministic; Play Again never re-logs-in, so seats persist.
	// Standalone: exactly one login ⇒ Blue — byte-identical to the old tag.
	if (NewPlayerController)
	{
		if (ASiegePlayerState* SiegePS = NewPlayerController->GetPlayerState<ASiegePlayerState>())
		{
			ETeamId SeatTeam = ETeamId::Red;
			if (!bBlueSeatTaken)
			{
				SeatTeam = ETeamId::Blue;
				bBlueSeatTaken = true;
			}
			else if (!bRedSeatTaken)
			{
				SeatTeam = ETeamId::Red;
				bRedSeatTaken = true;
			}
			else
			{
				UE_LOG(LogSiegeNet, Warning,
					TEXT("[%s] Third+ player login '%s' — both team seats are taken; assigning Red (unsupported in P1, doc §2.1)."),
					*GetNameSafe(this), *GetNameSafe(NewPlayerController));
			}

			SiegePS->SetTeam(SeatTeam);

			UE_LOG(LogSiegeNet, Log,
				TEXT("[%s] Player login '%s' seated as %s (M8 seat latch, doc §2.1)."),
				*GetNameSafe(this), *GetNameSafe(NewPlayerController),
				SeatTeam == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));
		}
	}

	// The bot's Red ASiegePlayerState is tagged Team=Red in SpawnBot() (TASK-045)
	// — NOT here: InitNewPlayer only runs for real player logins, so it is not the
	// bot's tagging site. (M8: SpawnBot itself is gated OFF in networked matches,
	// so exactly one PS per team resolves either way — doc §2.2.)

	return ErrorMessage;
}

void ASiegeGameMode::RestartPlayer(AController* NewPlayer)
{
	// M8 per-player spawn resolve (TASK-356 doc §3.4.4/D10): every (re)start goes
	// through the team-keyed transform resolve. Blue/standalone resolved the level
	// PlayerStart — the same spawn the engine path used (§10 byte-identity; the
	// one site where "identical route" is not literal: the engine used the start
	// actor's full rotation, this uses its yaw — L_Arena's PlayerStart has zero
	// pitch/roll, so the transform is identical; QA-scrutinize). The Red client
	// resolves the castle-relative fallback (no Red PlayerStart exists in L_Arena
	// — the fallback IS the design, doc §3.4.4).
	//
	// ⚠️ TASK-573 AMENDS THE FIRST HALF OF THAT PARAGRAPH: at the 9× castle
	// L_Arena's Blue PlayerStart lies INSIDE Castle_Blue, so BLUE now takes the
	// same castle-relative fallback Red does. The §10 byte-identity claim holds
	// only while the PlayerStart is outside its own keep — see
	// GetHeroStartTransform for the arithmetic and for why the level is not
	// edited (CONVENTIONS WR-§2b row A, WR-§3).
	if (!NewPlayer)
	{
		return;
	}

	const ASiegePlayerState* SiegePS = NewPlayer->GetPlayerState<ASiegePlayerState>();
	if (!SiegePS)
	{
		// Not one of ours (defensive) — the engine path is the null-safe fallback.
		Super::RestartPlayer(NewPlayer);
		return;
	}

	FVector StartLocation = FVector::ZeroVector;
	FRotator StartRotation = FRotator::ZeroRotator;
	GetHeroStartTransform(NewPlayer, SiegePS->GetTeam(), StartLocation, StartRotation);

	RestartPlayerAtTransform(NewPlayer, FTransform(StartRotation, StartLocation));
}

APawn* ASiegeGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	// Engine path FIRST — when it succeeds (every standalone spawn; the TASK-357
	// standalone regression proved zero spawn failures) this override is a pure
	// pass-through and behavior is byte-identical.
	if (APawn* ResultPawn = Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform))
	{
		return ResultPawn;
	}

	// ── TASK-356 loop-2 safety net: Super returned NULL, which means the pawn
	//    class refused a colliding spawn (its own SpawnCollisionHandlingMethod —
	//    the engine's implementation passes a bare FActorSpawnParameters). That is
	//    precisely how BLOCKER 5 left the joining player with `pawn=None`, and an
	//    unplayable seat is never an acceptable outcome. Retry the SAME transform
	//    with AdjustIfPossibleButAlwaysSpawn: the engine nudges the capsule to a
	//    free spot if it can, and spawns regardless if it cannot.
	//
	//    This is defense in depth, NOT the fix — the resolver above now derives
	//    its distance from the castle's live bounds, so this path should never
	//    run. If it ever does, the ERROR below is the signal that the geometry
	//    moved again and the clearance needs a look.
	UWorld* World = GetWorld();
	UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	if (!World || !PawnClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient; // never save a default player pawn into a map (mirrors the engine path)
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* AdjustedPawn = World->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo);

	UE_LOG(LogGitClaudeUnrealTest, Error,
		TEXT("[%s] Default pawn spawn at (%.0f, %.0f, %.0f) was refused for collision — retried with AdjustIfPossibleButAlwaysSpawn: %s. The castle-relative clearance (HeroSpawnCastleClearance) likely needs re-checking against current geometry."),
		*GetNameSafe(this),
		SpawnTransform.GetLocation().X, SpawnTransform.GetLocation().Y, SpawnTransform.GetLocation().Z,
		AdjustedPawn ? TEXT("succeeded") : TEXT("STILL FAILED"));

	return AdjustedPawn;
}

void ASiegeGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);

	// FinishRestartPlayer calls this for EVERY pawn the mode hands to a player —
	// the initial spawn and any restart fallback — so the death binding always
	// exists on the current hero, including a fresh pawn spawned after a destroy.
	// M8 (TASK-356 doc §3.4.4): the single TrackedHero assignment is RETIRED —
	// hero identity is resolved per-controller at death/restore time.
	if (AHeroCharacter* Hero = Cast<AHeroCharacter>(PlayerPawn))
	{
		Hero->OnHeroDied.AddUniqueDynamic(this, &ASiegeGameMode::HandleHeroDied);

		// TASK-750 (discharging TASK-748's named contract — see the header): the
		// destination owner binds here, at the SAME site and with the SAME idiom as the
		// death seam above, because it is the same rule being reused. AddUniqueDynamic
		// keeps repeated restarts idempotent exactly as it does for OnHeroDied.
		Hero->OnHeroRecallArrived.AddUniqueDynamic(this, &ASiegeGameMode::HandleHeroRecallArrived);
	}
}

void ASiegeGameMode::HandleHeroRecallArrived(AHeroCharacter* RecallingHero)
{
	if (!IsValid(RecallingHero))
	{
		return;
	}

	// ⛔ THE TELEPORT, AND ⛔ NOTHING ELSE (RECALL-§1: "Recall performs exactly two
	// effects: the teleport, and the heal" — the heal is the hero's own, applied by
	// EndRecall the moment this returns). ⛔ NO ResetHero() on this path: it is the
	// DEATH-path restore, and on a live hero it double-applies every upgrade stack and
	// re-arms a running War Banner aura. ⛔ No possession change, ⛔ no input change,
	// ⛔ no cooldown reset, ⛔ no HP write.
	// ⚠️ NAMED `RecallingController`, ⛔ NEVER `Owner` (TASK-761, C4458): AActor::Owner is an
	// inherited member of this very class, and UE builds with C4458 (declaration hides class
	// member) promoted to an ERROR. ⛔⛔ The rename is ALL-OR-NOTHING — a local left named
	// `Owner` at any ONE of the three uses below would resolve to the GAME MODE'S OWN owner
	// (null for a game mode), which COMPILES CLEAN and silently passes the wrong controller.
	AController* RecallingController = RecallingHero->GetController();

	FVector StartLocation = FVector::ZeroVector;
	FRotator StartRotation = FRotator::ZeroRotator;

	// ⭐ THE SAME RESOLVER THE RESPAWN USES — the whole reason this seam points at the
	// game mode. "Back at the castle" is decided in exactly one place, so a channel that
	// completes and a hero that respawns can never arrive at different homes (and the
	// TASK-569 row (n) "hero spawns OUTSIDE the keep" defect cannot be reintroduced by a
	// second, hand-typed destination).
	GetHeroStartTransform(RecallingController, RecallingHero->GetTeamId(), StartLocation, StartRotation);

	// Sweepless, like the respawn teleport: the resolved start is clear by design, and a
	// blocked sweep would silently leave the hero where it stood after a 10 s channel.
	RecallingHero->SetActorLocationAndRotation(StartLocation, StartRotation, /*bSweep*/ false, /*OutSweepHitResult*/ nullptr, ETeleportType::TeleportPhysics);

	if (APlayerController* PC = Cast<APlayerController>(RecallingController))
	{
		PC->SetControlRotation(StartRotation);
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] Recall completed for hero '%s' — teleported home to (%.0f, %.0f, %.0f) through the SHARED respawn resolver (RECALL-§1; the heal is the hero's own)."),
		*GetNameSafe(this), *GetNameSafe(RecallingHero), StartLocation.X, StartLocation.Y, StartLocation.Z);
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

	// Cancel EVERY pending hero respawn (M8: per-controller map, doc §3.4.4):
	// nothing revives under the end screen — PlayAgain() owns hero restoration
	// from here. (Own timer handles only — the timer policy stands.)
	for (TPair<TWeakObjectPtr<AController>, FTimerHandle>& RespawnPair : HeroRespawnTimers)
	{
		GetWorldTimerManager().ClearTimer(RespawnPair.Value);
	}
	HeroRespawnTimers.Empty();

	// ⭐ AND RETIRE EVERY LIVE GHOST, FOR EXACTLY THE SAME REASON AND UNDER EXACTLY THE
	// SAME ALREADY-EXISTING RULE (TASK-750, GHOST-§2 — ⛔ this is NOT a new match-end
	// rule): the shipped contract is "After match end the hero stays down; PlayAgain()
	// revives it", so the ghost inherits it verbatim — the player is handed his (still
	// dead, still hidden) hero back, the ghost leaves the field, and the end screen
	// goes up over the same state it went up over before this feature existed.
	// ⚠️ RUNS BEFORE the freeze and the end-screen push below, so no ghost is ever left
	// standing under the Victory screen and no controller reaches HandleMatchEnd
	// possessing a pawn that is about to be destroyed.
	// Collected first — RetireGhostFor mutates ActiveGhosts, never iterate it live.
	{
		TArray<TWeakObjectPtr<AController>> GhostedControllers;
		ActiveGhosts.GetKeys(GhostedControllers);
		for (const TWeakObjectPtr<AController>& WeakGhosted : GhostedControllers)
		{
			if (AController* Ghosted = WeakGhosted.Get())
			{
				RetireGhostFor(Ghosted, TEXT("match ended"));
			}
		}
		// Any entry whose controller has gone (disconnect) is dropped with it — the
		// actor dies with the world, and leaving a stale key would outlive the match.
		ActiveGhosts.Empty();
	}

	UE_LOG(LogGitClaudeUnrealTest, Log, TEXT("[%s] Castle '%s' (%s) destroyed — match over, winner: %s."),
		*GetNameSafe(this), *GetNameSafe(DestroyedCastle),
		CastleTeam == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"),
		Winner == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));

	// Freeze the world BEFORE the end screen goes up (§3.9 / M2 exit criteria,
	// TASK-024 — closes the qa/TASK-006-report.md finding-2 TODO(M2)): units,
	// towers, in-flight projectiles, income, and the match clock all stop here.
	FreezeWorldAtMatchEnd();

	// M8 (TASK-356 doc §3.4.1/D7): the direct HandleMatchEnd push loop is RETIRED
	// — the match result is now GameState STATE. SetMatchResult latches
	// bMatchEnded + WinningTeam (replicated; each CLIENT's OnRep shows its own
	// end screen) and notifies the LOCAL controller(s) server-side (the HOST's
	// screen — in standalone the same single local PC the old loop resolved,
	// same observable call via a compliant route, doc §10). Null-safe fallback:
	// with no ASiegeGameState (defensive mis-config) the old direct push runs so
	// a standalone match can never lose its end screen.
	if (ASiegeGameState* SiegeGameState = Cast<ASiegeGameState>(GameState))
	{
		SiegeGameState->SetMatchResult(Winner);
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] No ASiegeGameState — match result cannot replicate; falling back to the direct local end-screen push (GameStateClass should be ASiegeGameState)."),
			*GetNameSafe(this));

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

	// 2b) Barracks spawners (TASK-057): a live ABarracks would keep summoning
	//     fresh units into a frozen match under the Victory screen. Unlike a
	//     tower, the Barracks is a class I author with a public FreezeAI() hook
	//     (stops both its spawn + self-destruct timers), so freeze it the same
	//     way units are FreezeAI()d — a clean per-object stop, no reach into a
	//     private handle. FreezeAI never spawns/destroys, so it is safe inside
	//     the live iterator; PlayAgain destroys every building regardless.
	//     (Deep Mines need no per-mine hook here — step 4's PauseIncome stops
	//     their flat income with all other accrual.)
	int32 FrozenBarracks = 0;
	for (TActorIterator<ABarracks> It(World); It; ++It)
	{
		It->FreezeAI();
		++FrozenBarracks;
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

	// 6) Bot decision loop (GDD §4, TASK-047 — the TASK-045 forward-dependency):
	//    step 4 above already PAUSED the bot's income (its Red ASiegePlayerState
	//    is in PlayerArray) and step 1 FROZE its already-spawned units, but the
	//    bot's own 2 s decision timer is a separate handle that would keep ticking
	//    EvaluateDecisions under the Victory screen — playing fresh cards and
	//    spawning fresh Red units into a frozen match. StopDecisionTimer() halts
	//    that loop (the public TASK-045 hook). Idempotent; ResetBot() re-arms it on
	//    Play Again. (TASK-046 also gates EvaluateDecisions on match-active as
	//    belt-and-suspenders; this is the freeze-side stop TASK-047 owns.) Uses the
	//    single tracked bot member — exactly one bot per match (SpawnBot).
	bool bBotStopped = false;
	if (IsValid(BotController))
	{
		BotController->StopDecisionTimer();
		bBotStopped = true;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] Match-end freeze: %d unit(s) frozen, %d tower(s) silenced, %d barracks frozen, %d projectile(s) cleared, income paused, clock stopped, bot decision loop %s."),
		*GetNameSafe(this), FrozenUnits, SilencedTowers, FrozenBarracks, Projectiles.Num(), bBotStopped ? TEXT("stopped") : TEXT("absent"));
}

void ASiegeGameMode::HandleHeroDied(AHeroCharacter* DeadHero)
{
	if (!IsValid(DeadHero))
	{
		return;
	}

	// M8 (TASK-356 doc §3.4.4/D10): resolve the OWNING controller at death time —
	// death disables input WITHOUT unpossessing (TASK-003), so the controller is
	// normally still attached. In a P1 session the HOST's units kill the CLIENT's
	// hero, so per-controller tracking is load-bearing, not theoretical.
	AController* OwningController = DeadHero->GetController();
	if (!OwningController)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] Hero '%s' died with no owning controller — no respawn scheduled (PlayAgain still restores every player)."),
			*GetNameSafe(this), *GetNameSafe(DeadHero));
	}

	// ⭐ THE ONE PREDICATE (TASK-750, GHOST-§2/§3) — it carries BOTH shipped guards
	// with their behaviour unchanged: no owning controller ⇒ nothing to schedule (the
	// warning above is the shipped message, kept verbatim), and after match end the
	// hero stays down until PlayAgain() (GDD §3.9; OnCastleDestroyedHandler also
	// cancels any respawn already pending). ⛔ Routing them through the predicate is
	// what makes the ghost's existence and the timer's existence literally the same
	// condition rather than two conditions that have to be kept in step by hand.
	if (!ShouldEnterGhostState(bMatchEnded, OwningController != nullptr))
	{
		return;
	}

	// Exactly HeroRespawnDelay later THIS player's hero is back at its own-castle
	// side (the number is Jonathan's 180 s — GHOST-§0; see the property's comment).
	// One handle per controller (FindOrAdd + SetTimer-replaces), so respawns can never
	// stack per player and two players' deaths never clobber each other's timers. The
	// weak controller rides the delegate payload — a controller gone by fire time is a
	// logged no-op (HandleHeroRespawnTimer). ⛔ POLICY AND SHAPE UNCHANGED (QA-binding).
	FTimerHandle& RespawnHandle = HeroRespawnTimers.FindOrAdd(OwningController);
	GetWorldTimerManager().SetTimer(RespawnHandle,
		FTimerDelegate::CreateUObject(this, &ASiegeGameMode::HandleHeroRespawnTimer, TWeakObjectPtr<AController>(OwningController)),
		HeroRespawnDelay, false);

	// ⭐ AND THE PLAYER GETS SOMETHING TO DRIVE FOR THOSE THREE MINUTES (GHOST-§1).
	// Armed AFTER the timer on purpose: the timer is the contract the player is owed,
	// and a ghost that fails to spawn must never be able to cost anyone a respawn.
	SpawnAndPossessGhost(OwningController, DeadHero);
}

void ASiegeGameMode::SpawnAndPossessGhost(AController* Player, AHeroCharacter* DeadHero)
{
	if (!Player || !IsValid(DeadHero))
	{
		return;
	}

	// ⛔ ONLY A PLAYER CONTROLLER GETS A GHOST. The bot (an AAIController) possesses no
	// pawn at all and has nothing to look through; handing it a ghost would put an
	// enemy-visible actor on the field that nobody is driving (G-4 makes that a lie
	// told to the human observer). Logged, not silent — an unexpected controller class
	// here is worth seeing.
	APlayerController* PC = Cast<APlayerController>(Player);
	if (!PC)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] Hero death on non-player controller '%s' — no ghost spawned (the respawn timer still runs)."),
			*GetNameSafe(this), *GetNameSafe(Player));
		return;
	}

	// ⛔ IDEMPOTENT PER CONTROLLER: a second ghost would strand the first one in the
	// world and overwrite the hero reference the respawn needs. AHeroCharacter's own
	// bDead latch already makes a double OnHeroDied broadcast impossible, so this is
	// the belt for a future second death path rather than a live case.
	const TWeakObjectPtr<AController> Key(Player);
	if (ActiveGhosts.Contains(Key))
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] Ghost requested for '%s' which already has one — keeping the existing ghost."),
			*GetNameSafe(this), *GetNameSafe(Player));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// At the death location, facing the way the hero was facing — his words are
	// "instead get a ghost creature", so the swap is meant to read as continuous.
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = PC;
	SpawnParams.Instigator = nullptr;

	// ⛔ ALWAYS SPAWN. The hero just died where it was standing, which may be inside a
	// unit blob or against a wall — and the collision-handling method that refuses a
	// colliding spawn is exactly how a player once ended up with no pawn at all
	// (the M8 BLOCKER-5 lesson, recorded at SpawnDefaultPawnAtTransform_Implementation).
	// ⚠️ A ghost that fails to spawn is 180 seconds of nothing, which is the single
	// worst outcome this feature can produce.
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector GhostLocation = DeadHero->GetActorLocation();
	const FRotator GhostRotation = DeadHero->GetActorRotation();

	ASiegeGhostPawn* Ghost = World->SpawnActor<ASiegeGhostPawn>(ResolveGhostPawnClass(), GhostLocation, GhostRotation, SpawnParams);
	if (!IsValid(Ghost))
	{
		// ⛔ NEVER FATAL AND NEVER A CRASH: the player keeps the (hidden, input-disabled)
		// hero possessed for the wait — exactly the pre-TASK-750 experience — and the
		// respawn timer armed above still fires and still restores him.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] Failed to spawn the death ghost for '%s' at (%.0f, %.0f, %.0f) — the player waits out the respawn on the dead hero (no ghost, no crash)."),
			*GetNameSafe(this), *GetNameSafe(Player), GhostLocation.X, GhostLocation.Y, GhostLocation.Z);
		return;
	}

	// ① THE GHOST'S OWN API, IN ITS SPECIFIED ORDER (TASK-749): InitializeGhost is
	// called ONCE after SpawnActor and BEFORE Possess.
	// ⛔⛔ THE TEAM PASSED HERE IS IDENTITY, NOT AFFILIATION, and it must never become
	// affiliation: G-4 makes the ghost enemy-visible, so an observer needs to know
	// WHOSE ghost it is — it feeds a material tint and nothing else. ⛔ There is no
	// targeting, no friend/foe test and no collision channel behind it, and exposing it
	// through ITeamAgent would destroy the entire untargetability design (GHOST-§1).
	Ghost->InitializeGhost(DeadHero->GetTeamId());

	// ⛔ POSSESS, AND LET THE ENGINE DO THE HAND-OFF. Possess() unpossesses the hero
	// (which stays in the world, hidden and dead — RestoreHeroAtStart teleports and
	// heals THAT SAME ACTOR later), rebuilds the pawn input plumbing on the ghost and
	// moves the view target to it. ⛔ No camera code and no input-mode code here.
	PC->Possess(Ghost);
	PC->SetControlRotation(GhostRotation);

	ActiveGhosts.Add(Key, FSiegeGhostState{ Ghost, DeadHero });

	// ⛔⛔ THE POSTURE HAND-OFF GOES THROUGH THE CONTROLLER, WHICH ROUTES IT THROUGH
	// ApplyCursorInputState() — THE ONE OWNER (HELP-§5 / GHOST-§4). ⛔ There is no
	// SetInputMode and no bShowMouseCursor write in this class, and the ghost adds no
	// term to the cursor-owner ladder (it is a free-look pawn exactly like the hero).
	if (ASiegePlayerController* SiegePC = Cast<ASiegePlayerController>(PC))
	{
		SiegePC->HandleGhostPossessionChanged();
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] '%s' died — ghost '%s' spawned at (%.0f, %.0f, %.0f) and possessed; respawn in %.0f s (GHOST-§, Jonathan's ruling)."),
		*GetNameSafe(this), *GetNameSafe(DeadHero), *GetNameSafe(Ghost),
		GhostLocation.X, GhostLocation.Y, GhostLocation.Z, HeroRespawnDelay);
}

void ASiegeGameMode::RetireGhostFor(AController* Player, const TCHAR* Reason)
{
	if (!Player)
	{
		return;
	}

	// A controller with no ghost is a clean no-op, so every caller may call this
	// unconditionally — which is what lets the four call sites sit beside the four
	// places that already clear this class's own respawn handles.
	FSiegeGhostState State;
	if (!ActiveGhosts.RemoveAndCopyValue(TWeakObjectPtr<AController>(Player), State))
	{
		return;
	}

	ASiegeGhostPawn* Ghost = State.Ghost.Get();
	AHeroCharacter* Hero = ResolveHeroToRestore(Player->GetPawn(), State.Hero.Get());

	// ⛔ POSSESS FIRST, DESTROY SECOND — never the reverse. Destroying the possessed
	// pawn would drive the controller through PawnPendingDestroy into the Inactive
	// state and park the view target at the death spot; possessing the hero makes the
	// engine unpossess the ghost cleanly and hands the camera straight back to the
	// SAME hero actor the player died in. ⛔ No new pawn is spawned on this path.
	// ⭐⭐ AND THE RESPAWN-SIDE INPUT-CONTEXT HAND-OFF IS SAFE **STRUCTURALLY** — checked
	// at source, ⛔ not assumed, because TASK-749 found the outbound half of exactly this
	// trap and it is the most dangerous thing in the batch (GHOST-§4):
	//   • The Enhanced Input mapping context is added by the PAWN, never by
	//     ASiegePlayerController (HeroCharacter.cpp:228-280 says so in its own comment).
	//   • `APawn::NotifyControllerChanged()` fires on POSSESSION as well as on unpossess
	//     ⇒ `AHeroCharacter::NotifyControllerChanged` re-adds IMC_Hero (with its KBD-§5
	//     positional-layout resolve) the instant this Possess() lands. The reverse trap
	//     therefore does NOT exist: the hero re-arms its own context, by the same
	//     mechanism the ghost mirrors in the other direction.
	//   • ⭐ Stronger still, there is no WINDOW to be caught in: ASiegeGhostPawn adds the
	//     SAME context (IMC_Hero) at the SAME priority (GhostMappingContextPriority == 1
	//     == HeroMappingContextPriority), and there is not one RemoveMappingContext call
	//     in this entire module ⇒ the input composition is INVARIANT across the whole
	//     death → ghost → respawn cycle, and AddMappingContext collapses the duplicate.
	//   • ⛔ THEREFORE THIS FILE ADDS NO CONTEXT AND REMOVES NONE. Doing so would be a
	//     second owner of a composition that already has exactly one per pawn.
	// ⚠️ This is a compile-time/structural argument. It is NOT a substitute for GHOST-§4's
	// PIE obligation, which may never be waived on a clean compile.
	APlayerController* PC = Cast<APlayerController>(Player);
	if (PC && Hero && PC->GetPawn() != Hero)
	{
		PC->Possess(Hero);
	}

	// ③ The ghost's own teardown (TASK-749) — ⛔ NOT a raw Destroy(). RetireGhost() is
	// idempotent by contract, so the two legitimate callers (the respawn boundary and
	// match end) can both fire for one ghost when a match ends near that boundary
	// without this class needing a "did I already retire it?" flag. It logs loudly if
	// it is ever reached while STILL POSSESSED — which is the possess-first ordering
	// above being checked from the other side.
	if (IsValid(Ghost))
	{
		Ghost->RetireGhost();
	}

	// Posture again through the ONE owner, for the same reason as the outbound
	// hand-off. At match end this is a deliberate no-op — ApplyCursorInputState early-
	// outs while the controller's bMatchEnded is latched, because HandleMatchEnd owns
	// the end-screen posture and must not be overridden (HELP-§5).
	if (ASiegePlayerController* SiegePC = Cast<ASiegePlayerController>(PC))
	{
		SiegePC->HandleGhostPossessionChanged();
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] Ghost retired for '%s' (%s) — hero '%s' %s."),
		*GetNameSafe(this), *GetNameSafe(Player), Reason ? Reason : TEXT("unspecified"),
		*GetNameSafe(Hero), Hero ? TEXT("re-possessed") : TEXT("MISSING — the restore path will hand out a fresh pawn"));
}

void ASiegeGameMode::HandleHeroRespawnTimer(TWeakObjectPtr<AController> WeakController)
{
	AController* Player = WeakController.Get();

	// Fired ⇒ this controller's pending entry is spent either way.
	HeroRespawnTimers.Remove(WeakController);

	if (!Player)
	{
		// ⛔ The map entry for a gone controller must go too, or a ghost actor outlives
		// the player it belonged to (weak pointers stop it leaking memory, they do not
		// stop it standing on the battlefield). RemoveAndCopyValue inside RetireGhostFor
		// needs a live AController*, so the entry is dropped directly here.
		ActiveGhosts.Remove(WeakController);

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] Hero respawn timer fired for a controller that no longer exists (disconnect) — skipped."), *GetNameSafe(this));
		return;
	}

	// ⭐ THE THREE MINUTES ARE UP (TASK-750): give the hero back FIRST, then let the
	// SHIPPED restore do the teleport + ResetHero() completely unchanged. After this
	// call Player->GetPawn() is the hero again, so RestoreHeroAtStart resolves its
	// team-keyed own-castle start exactly as it always has — "respawn back at the
	// castle" is the shipped GetHeroStartTransform, ⛔ not re-implemented here.
	RetireGhostFor(Player, TEXT("respawn timer expired"));

	RestoreHeroAtStart(Player);
}

void ASiegeGameMode::RestoreHeroAtStart(AController* Player)
{
	// M8 (TASK-356 doc §3.4.4): parameterized per controller — the retired
	// FindLocalSiegeController/TrackedHero pair assumed "first controller = THE
	// player" (audit §1a#4/#5). In standalone the one caller passes the one
	// controller, whose pawn is the same hero the old resolve found (§10).
	//
	// ⛔⛔ PRECONDITION (TASK-750): THIS FUNCTION IS UNCHANGED AND EXPECTS THE HERO TO
	// BE THE POSSESSED PAWN. Both callers therefore run RetireGhostFor first —
	// HandleHeroRespawnTimer immediately above, PlayAgain in its step 1b. ⛔ A future
	// caller that reaches here with a ghost possessed will take the defensive
	// RestartPlayer branch below and hand out a SECOND hero; retire the ghost first.
	// (The teleport and the heal are deliberately NOT re-implemented anywhere in the
	// ghost path — GHOST-§2: they already ship, and this is them.)
	if (!Player)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("[%s] RestoreHeroAtStart called with no controller — nothing to restore."), *GetNameSafe(this));
		return;
	}

	AHeroCharacter* Hero = Cast<AHeroCharacter>(Player->GetPawn());
	APlayerController* PC = Cast<APlayerController>(Player);

	if (!IsValid(Hero))
	{
		// Pawn gone entirely (no shipped flow destroys it — defensive): hand the
		// player a fresh default pawn. The RestartPlayer OVERRIDE resolves the
		// team-keyed start transform, and FinishRestartPlayer's SetPlayerDefaults
		// re-binds OnHeroDied on the new pawn.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] Hero pawn missing at respawn for '%s' — restarting the player with a fresh default pawn."),
			*GetNameSafe(this), *GetNameSafe(Player));
		RestartPlayer(Player);
		return;
	}

	FVector StartLocation = FVector::ZeroVector;
	FRotator StartRotation = FRotator::ZeroRotator;
	GetHeroStartTransform(Player, Hero->GetTeamId(), StartLocation, StartRotation);

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

	// Face the hero's spawn direction, whatever the resolver derived: a branch-2
	// PlayerStart's authored yaw, or — since TASK-665 (ROT ACTIVATION RULING
	// item 5) — the branch-3 castle-relative facing TOWARD the own castle's
	// rotated gate. (This line used to say "PlayerStart yaw 0 looks across the
	// arena" — a world-frame claim the CASTLE-ROTATION wave retired, CONVENTIONS
	// ROT-§4: L_Arena's only PlayerStart is refused at the 9× castle and the
	// live respawn facing is branch 3's.)
	if (PC)
	{
		PC->SetControlRotation(StartRotation);
	}
}

void ASiegeGameMode::GetHeroStartTransform(AController* Player, ETeamId HeroTeam, FVector& OutLocation, FRotator& OutRotation)
{
	// ── M8 loop-1 BLOCKER-3 FIX (TASK-357: both heroes stacked at the BLUE
	//    PlayerStart, -23800). The old order ran FindPlayerStart FIRST and
	//    returned on any APlayerStart, so `HeroTeam` was only ever read by
	//    unreachable code and the Red client spawned on the Blue side. The team
	//    now GOVERNS: a PlayerStart is accepted only when it lies on the HERO'S
	//    OWN side of the centerline (X=0, CONVENTIONS world axes), measured
	//    against that team's castle — data-driven, NOT a Blue/Red hardcode (the
	//    M8 team law retires "Blue = local"), so a future Red-side PlayerStart is
	//    picked up automatically.
	//
	// ── TASK-573 (CONVENTIONS WR-§2b row A): THE SIDE TEST ALONE IS NOT SUFFICIENT
	//    AT THE 9× CASTLE, AND IT WAS THE ONLY TEST. Branch 2 accepted a same-side
	//    PlayerStart and returned BEFORE the hardened branch 3 could run. The
	//    castle then grew 3× a second time (7,313.7 x 7,384.5 x 8,082.6 uu,
	//    CONVENTIONS WR-§0) around a PlayerStart that did not move: Castle_Blue
	//    sits at X=-25000 with a colliding half-extent of 3,656.85 on X, so the
	//    footprint spans -28,656.85…-21,343.15 and L_Arena's Blue PlayerStart
	//    (≈-23800, 0, 98) is 2,456.85 uu INSIDE THE KEEP — and 76 uu BELOW the 9×
	//    interior floor (z≈174), i.e. inside the floor slab, not standing on it.
	//    The hero would spawn inside the castle at match start AND at every
	//    respawn, or SpawnActor would refuse on collision and leave the player
	//    with NO PAWN — the TASK-357 BLOCKER-5 failure, reproduced on the branch
	//    nobody hardened.
	//
	//    THE REPAIR IS HERE AND NOT IN THE LEVEL, DELIBERATELY: the PlayerStart
	//    transform is LEVEL DATA and the one-time L_Arena save exception is SPENT
	//    (CONVENTIONS WR-§3). A PlayerStart lying inside the own castle's COLLIDING
	//    bounds is REFUSED and falls through to branch 3, which already derives a
	//    clear spawn from those very same bounds. The .umap is never touched.
	//
	//    STANDALONE BYTE-IDENTITY (load-bearing, TASK-357 gate g passed and must
	//    keep passing): the two tests are ANDed, and the new one can only ever
	//    REJECT — and only a start that is inside the castle, which is exactly the
	//    geometry in which the old branch was ALREADY BROKEN. For every geometry
	//    where the PlayerStart lies OUTSIDE the castle's colliding box this
	//    function still returns the identical location and the identical yaw-only
	//    rotation it always did. With no castle in the level at all — or with
	//    unresolvable/degenerate bounds — the rejection CANNOT fire (see
	//    bCastleBoundsUsable) and ANY same-side PlayerStart is accepted, the
	//    pre-M8 behavior, preserved for defensive/test maps.
	//    ⚠️ That guard direction is deliberate and load-bearing: branch 3 reads the
	//    SAME bounds, so a mis-signed or over-eager test would break BOTH branches
	//    at once and leave only the arena-origin last resort.

	// 1) Resolve the hero's OWN-team castle first — it defines "this team's side".
	const ACastle* OwnCastle = nullptr;
	for (TActorIterator<ACastle> It(GetWorld()); It; ++It)
	{
		if (It->GetTeamId() == HeroTeam)
		{
			OwnCastle = *It;
			break;
		}
	}

	// 1b) ── TASK-573: ONE castle-bounds query, read by BOTH branch 2 and branch 3.
	//     Branch 3 has derived its spawn distance from these bounds since TASK-356
	//     loop-2; branch 2 now derives its REJECTION from them. Two independent
	//     derivations of the same geometry inside one function is the pairing-law
	//     hazard in miniature — they drift, and a drifted pair is a spawn that one
	//     branch calls clear and the other calls occupied. There is exactly one
	//     query and exactly one result.
	//
	//     bOnlyCollidingComponents = true: what matters is what BLOCKS a pawn
	//     spawn, not the render/widget bounds — ACastle::HPBarWidget sits at
	//     relative Z +9450 (Castle.cpp, re-derived by TASK-557) and would inflate
	//     an all-components query catastrophically.
	//
	//     bCastleBoundsUsable DECIDES THE FAILURE DIRECTION, and it is written to
	//     fail toward ACCEPTING (CONVENTIONS WR-§2b row A): a mesh that has not
	//     streamed in, an actor with no colliding component, or any other path
	//     that yields a ~zero extent must NEVER be able to reject a PlayerStart,
	//     because branch 3 depends on these same bounds and would have nothing
	//     left but the arena origin. Branch 3 is deliberately NOT gated on this
	//     flag — it keeps its own FMath::Max against the authored floor, which is
	//     precisely how it already absorbs a degenerate bound.
	FVector CastleBoundsOrigin = FVector::ZeroVector;
	FVector CastleBoxExtent = FVector::ZeroVector;
	bool bCastleBoundsUsable = false;
	if (OwnCastle)
	{
		OwnCastle->GetActorBounds(/*bOnlyCollidingComponents=*/ true, CastleBoundsOrigin, CastleBoxExtent);
		bCastleBoundsUsable = CastleBoxExtent.GetMin() > UE_KINDA_SMALL_NUMBER;
	}

	// 2) The level's PlayerStart (L_Arena: ≈(-23800, 0, 98) yaw 0 on the Blue
	//    side — moved outward with the ±25000 castle in the M7.6 10× widening,
	//    and SWALLOWED by the castle at the 9× pass, which is what TASK-573's
	//    second test exists to survive). FindPlayerStart falls back to
	//    WorldSettings when the level has no PlayerStart; that is not a spawn
	//    point, so only a real APlayerStart is accepted here — and only when it
	//    is (a) on this hero's own half AND (b) not inside this team's keep.
	if (AActor* Start = FindPlayerStart(Player))
	{
		if (Start->IsA<APlayerStart>())
		{
			const FVector StartLocation = Start->GetActorLocation();

			const bool bStartOnOwnSide = !OwnCastle
				|| ((StartLocation.X <= 0.0) == (OwnCastle->GetActorLocation().X <= 0.0));

			// TASK-573: the castle-footprint rejection. The colliding AABB read
			// once at (1b), tested RAW — no added margin, no clearance padding.
			// That is the minimum test that catches the defect, and the minimum is
			// what keeps the divergence from the shipped behavior as small as the
			// defect itself. Padding it with HeroSpawnCastleClearance would reject
			// starts that are demonstrably fine, and would re-introduce exactly the
			// kind of hand-tuned derived margin CONVENTIONS SC-§34 exists to ban.
			const bool bStartInsideOwnCastle = bCastleBoundsUsable
				&& FBox(CastleBoundsOrigin - CastleBoxExtent, CastleBoundsOrigin + CastleBoxExtent).IsInsideOrOn(StartLocation);

			if (bStartOnOwnSide && !bStartInsideOwnCastle)
			{
				OutLocation = StartLocation;
				OutRotation = FRotator(0.0f, Start->GetActorRotation().Yaw, 0.0f);
				return;
			}

			if (bStartInsideOwnCastle)
			{
				// Warning, not Log, and not once-only: this fires at match start and
				// at every respawn, and each occurrence is a real level/geometry
				// mismatch a human should be able to read straight out of the PIE
				// log. The branch-3 Log line below prints where the hero went
				// instead, so the pair reads as one story.
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("[%s] PlayerStart '%s' at (%.0f, %.0f, %.0f) lies INSIDE the %s castle's colliding bounds (centre X %.0f, half-extent %.0f x %.0f x %.0f) — REFUSED, falling through to the castle-relative resolver. The PlayerStart did not move; the castle grew around it (CONVENTIONS WR-§2b row A). The level is NOT edited to fix this."),
					*GetNameSafe(this), *GetNameSafe(Start),
					StartLocation.X, StartLocation.Y, StartLocation.Z,
					HeroTeam == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"),
					CastleBoundsOrigin.X, CastleBoxExtent.X, CastleBoxExtent.Y, CastleBoxExtent.Z);
			}
		}
	}

	// 3) The hero's own-castle side (§3.1): the own-team castle's position,
	//    offset toward the centerline (X=0, CONVENTIONS world axes) so the spawn
	//    clears the castle footprint, facing the OWN castle (TASK-665 — pre-ROT
	//    this branch faced the enemy half; see the facing derivation below,
	//    CONVENTIONS ROT-§4). THIS is the branch the Red client takes (no
	//    Red-side PlayerStart exists in L_Arena — the fallback IS the design,
	//    doc §3.4.4), and it is now reachable.
	if (OwnCastle)
	{
		// ── TASK-356 loop-2 BLOCKER-5 FIX: DERIVE the distance from the castle's
		//    LIVE colliding bounds instead of trusting an authored constant. The
		//    old flat 600 was sized for the M1 castle's ~810-uu footprint and
		//    rotted at the 3× remaster: TASK-357 measured the real colliding
		//    half-extent at 1,219 uu, so 600 spawned the Red hero 819 uu INSIDE
		//    its own castle and SpawnActor refused ⇒ pawnless player. Querying the
		//    geometry means the next castle resize carries the spawn with it (the
		//    lesson the 3× remaster taught: hardcoded extents rot).
		//
		//    The bounds themselves are read ONCE at (1b) above — see that block for
		//    why bOnlyCollidingComponents = true (the HP-bar widget now sits 9,450
		//    uu up, re-derived by TASK-557, and would inflate an all-components
		//    query catastrophically) and for the degenerate-bounds reasoning.
		//    Degenerate/unresolvable bounds (mesh not yet loaded, extent ~0) simply
		//    fall through to the authored floor here, unchanged by TASK-573.

		// Floor at the authored X (1,500 — the empirically validated ≥1,200 band
		// with margin), so a zero/degenerate bound can never produce an inside-the-
		// castle spawn again. With the live 9× castle (half-extent 3,656.85) this
		// resolves 3,656.85 + 300 = 3,956.85 (derived wins, and the authored floor
		// is now inert by a wide margin — it was 1,219 + 300 = 1,519 at the 3×
		// castle); with no usable bounds it resolves 1,500 (floor wins). ⚠️ The
		// floor is deliberately LEFT at 1,500: it is a body-scale last resort for
		// the no-bounds case, not a castle-derived number, so scaling it with the
		// castle would be the defect (CONVENTIONS WR-§1, SC-§34 human-scale
		// exemption; TASK-557 row S9 verified it).
		// Both operands cast to float explicitly: FVector components are DOUBLE in
		// UE5, and FMath::Max is a single-type template — mixing double and float
		// would fail template deduction (CONVENTIONS compile traps).
		const float AuthoredFloorX = static_cast<float>(HeroSpawnCastleOffset.X);
		const float DerivedSpawnDistance = static_cast<float>(CastleBoxExtent.X) + HeroSpawnCastleClearance;
		const float SpawnDistance = FMath::Max(AuthoredFloorX, DerivedSpawnDistance);

		const FVector CastleLocation = OwnCastle->GetActorLocation();
		const float TowardCenterline = (CastleLocation.X <= 0.0f) ? 1.0f : -1.0f;
		OutLocation = CastleLocation + FVector(SpawnDistance * TowardCenterline, HeroSpawnCastleOffset.Y, HeroSpawnCastleOffset.Z);

		// ── TASK-665 SPAWN-FACING FIX (ROT ACTIVATION RULING item 5; measured facing
		//    delta 180°, handoffs/TASK-663-buildmaster.md §1): with the CASTLE-ROTATION
		//    wave the gate mouth sits dead ahead ON this spawn axis (292 uu out, 663
		//    §1), so the hero now spawns FACING HIS OWN CASTLE instead of the enemy
		//    half. The yaw is DERIVED AT RUNTIME from the resolved castle transform —
		//    atan2 toward the castle centre, ⛔ never a hardcoded yaw — so a moved
		//    castle carries the facing with it exactly as it already carries the
		//    location. Under the current layout this lands at yaw 180 (Blue) / 0 (Red)
		//    — the old TowardCenterline facing negated, as the ruling derives.
		//    Yaw-only on purpose (pitch/roll 0 — the same yaw-only contract branch 2
		//    keeps). Degenerate-input proof: SpawnDistance >= the authored floor
		//    (1,500) > 0, so ToOwnCastle.X is never ~0 and Atan2 is well-defined.
		//    Whole derivation in double (FVector components are double in UE5) —
		//    CONVENTIONS compile traps.
		const FVector ToOwnCastle = CastleLocation - OutLocation;
		const double FacingYawDeg = FMath::RadiansToDegrees(FMath::Atan2(ToOwnCastle.Y, ToOwnCastle.X));
		OutRotation = FRotator(0.0, FacingYawDeg, 0.0);

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] Castle-relative hero start for %s: castle X %.0f, measured colliding half-extent %.0f + clearance %.0f => spawn distance %.0f (authored floor %.0f) -> (%.0f, %.0f, %.0f), facing yaw %.0f toward the own castle (TASK-665)."),
			*GetNameSafe(this), HeroTeam == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"),
			CastleLocation.X, CastleBoxExtent.X, HeroSpawnCastleClearance, SpawnDistance, AuthoredFloorX,
			OutLocation.X, OutLocation.Y, OutLocation.Z, OutRotation.Yaw);
		return;
	}

	// 4) Last resort — arena origin, above the Z=0 ground plane (TASK-015).
	UE_LOG(LogGitClaudeUnrealTest, Warning,
		TEXT("[%s] No own-side PlayerStart and no own-team castle found — respawning the hero at the arena origin."), *GetNameSafe(this));
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
	//    M8: the owned set is now the per-controller respawn map (doc §3.4.4).
	for (TPair<TWeakObjectPtr<AController>, FTimerHandle>& RespawnPair : HeroRespawnTimers)
	{
		GetWorldTimerManager().ClearTimer(RespawnPair.Value);
	}
	HeroRespawnTimers.Empty();

	// 1b) Retire every live death ghost (TASK-750), beside the handles it belongs to.
	//     ⛔⛔ THIS MUST PRECEDE STEP 5 AND IT IS LOAD-BEARING, NOT TIDINESS: step 5
	//     reaches the hero through IterPC->GetPawn(), so a Play Again pressed while a
	//     player is ghosted would find a non-hero pawn, SKIP ResetUpgrades() entirely,
	//     and then RestoreHeroAtStart's ResetHero() would re-apply the OLD upgrade
	//     stacks onto the "reset" hero — upgrade stacks surviving a full match reset,
	//     which is exactly the §3.9 clearing that TASK-058 exists to guarantee. It also
	//     keeps step 6's controller walk operating on the pawn it was written for.
	//     Same collect-then-mutate discipline as the loops below.
	{
		TArray<TWeakObjectPtr<AController>> GhostedControllers;
		ActiveGhosts.GetKeys(GhostedControllers);
		for (const TWeakObjectPtr<AController>& WeakGhosted : GhostedControllers)
		{
			if (AController* Ghosted = WeakGhosted.Get())
			{
				RetireGhostFor(Ghosted, TEXT("Play Again"));
			}
		}
		ActiveGhosts.Empty();
	}

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

	// 3a2) Capture zone back to Neutral (W1-PREP additions 3, TASK-260 — §3.9
	//      reset path). Play Again destroys every unit (step 2), so the next
	//      capture eval would see an empty zone and LATCH the pre-reset owner
	//      (empty = unchanged) — a Blue/Red mid zone would carry over. Force it
	//      Neutral here, the same collect-free TActorIterator pattern as the
	//      ResetCastle loop above. Null-safe: no CaptureZone in the level = a
	//      clean no-op (pre-capture-feature behavior).
	for (TActorIterator<ACaptureZone> It(GetWorld()); It; ++It)
	{
		It->ResetCaptureZone();
	}

	// 3b) Match clock back to 0:00, overtime latch cleared, clock running again
	//     (§3.9 "match clock"; the §3.2 doubling re-arms for the new match).
	//     MUST precede step 4: ResetEconomy() re-derives each player's gold
	//     rate by reading this latch live — clearing it first lands the rate
	//     on the pre-overtime base (display +1/s, and with the 2026-07-24
	//     defaults it is EXACT, not a round-up — true accrual is 1 gold per
	//     1 s, TASK-278 reverting TASK-089's 1-per-2-s rate).
	if (ASiegeGameState* SiegeGameState = Cast<ASiegeGameState>(GameState))
	{
		SiegeGameState->ResetClock();

		// M8 (TASK-356 doc §3.4.2): clear the replicated match result right after
		// the clock reset — the false-edge OnRep drives each CLIENT's local reset
		// (PerformLocalMatchReset); the server-side controllers are walked in
		// step 6 below exactly as before. Standalone: a state write no OnRep ever
		// reads — the local reset is step 6's direct call, unchanged (doc §10).
		SiegeGameState->ClearMatchResult();
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

	// 4b) Reset the bot (GDD §4 / §3.9, TASK-045): fresh deck + hand, decision
	//     timer restarted (and an idempotent ResetEconomy). The bot's gold-to-50
	//     and income-timer restart already happened in step 4's generic loop (its
	//     ASiegePlayerState is in PlayerArray); ResetBot owns only what that loop
	//     cannot reach — the bot's DECK (the player-controller reset loop in step 6
	//     skips the AAIController) and the DECISION timer. Runs after step 3b's
	//     ResetClock, so ResetBot's ResetEconomy re-derives against a cleared latch.
	if (IsValid(BotController))
	{
		BotController->ResetBot();
	}

	// 4c) Sandbox re-grant (TASK-071): the match stays sandbox across Play Again
	//     (bSandboxMatch persists — InitGame never re-runs), so restore the
	//     generous starting pile the same way match start did. Step 4 just
	//     ResetGold'd every player state back to 50; top the Blue player back up
	//     through the gold API. Synchronous here (unlike match start's deferred
	//     grant) because the Blue player state already exists and was reset
	//     synchronously in step 4 — no seeding race. No-op when not a sandbox match.
	if (bSandboxMatch)
	{
		GrantSandboxStartingGold();
	}

	// 5) Re-arm the win condition, then EVERY player's hero back at its start:
	//    full HP, repossessed, input restored (works for a dead OR alive hero).
	//    M8 (TASK-356 doc §3.4.4): iterate the player controllers instead of the
	//    retired single TrackedHero — in standalone the loop visits exactly the
	//    one controller/hero the old code restored (§10). Upgrade clearing (GDD
	//    §3.9, TASK-058) runs per hero BEFORE its restore, so the subsequent
	//    ResetHero re-applies zero stacks — a clean base hero, per player.
	bMatchEnded = false;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* IterPC = It->Get();
		if (!IterPC)
		{
			continue;
		}
		if (AHeroCharacter* IterHero = Cast<AHeroCharacter>(IterPC->GetPawn()))
		{
			IterHero->ResetUpgrades();
		}
		RestoreHeroAtStart(IterPC);
	}

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

	// 6b) THE WAR MAP'S MARKS (TASK-744, CONVENTIONS MARK-§ **M-4**: "Marks clear on
	//     match reset / PlayAgain()"). ⭐⭐ THIS IS THE ONE CALL SITE `ClearMarks()` HAS,
	//     AND IT HAS TO LIVE HERE: USiegeMapMarkSubsystem is a ULocalPlayerSubsystem, so
	//     it OUTLIVES an in-place PlayAgain — the world resets around it and nothing in
	//     it is touched. ⛔ Without this line last match's circles are still painted on
	//     next match's map, still numbered, and still referenceable by name to the AI
	//     commander ("hold 2" pointing at ground from a match that is over).
	//     ⚠️ TASK-744 could not add it: this file is TASK-750's sole-owned surface, and
	//     that fence is what let the two run in parallel at all. Reaching the subsystem
	//     is the ordinary local-player route named in its handoff — ⛔ no custom getter.
	//     Null-safe at every step: a remote client's server-side PC has no ULocalPlayer
	//     and is skipped, which is correct — marks are per-player (M-2) and each machine
	//     clears its own.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* const IterPC = It->Get();
		if (!IterPC)
		{
			continue;
		}
		if (ULocalPlayer* LocalPlayer = IterPC->GetLocalPlayer())
		{
			if (USiegeMapMarkSubsystem* MarkSubsystem = LocalPlayer->GetSubsystem<USiegeMapMarkSubsystem>())
			{
				MarkSubsystem->ClearMarks();
			}
		}
	}

	// 7) Re-scatter the procedural battlefield (M6.5, TASK-134): Play Again gets a
	//    FRESH random layout when bReRandomizeOnMatchReset is true (the actor owns
	//    that decision + the new seed). Found via TActorIterator; null-safe — no
	//    ASiegeBattlefieldScatter in the level = a clean no-op, nothing breaks.
	//    (The INITIAL scatter is the actor's own BeginPlay; this is reset-only.)
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ASiegeBattlefieldScatter> It(World); It; ++It)
		{
			if (ASiegeBattlefieldScatter* Scatter = *It)
			{
				Scatter->ClearScatter();
				Scatter->GenerateScatter();
			}
		}
	}
}

void ASiegeGameMode::StartMatch(const UObject* WorldContextObject)
{
	// Static main-menu entry (GDD §7, TASK-047 → TASK-049): open the arena into a
	// brand-new match. No ASiegeGameMode instance is required — L_MainMenu runs its
	// own menu game mode — so the arena path comes from the CDO, and the world
	// context is threaded from the calling widget (Blueprint auto-fills it).
	if (!WorldContextObject)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[ASiegeGameMode::StartMatch] No world context object — cannot open the arena (GDD §7 main-menu flow)."));
		return;
	}

	const ASiegeGameMode* Defaults = GetDefault<ASiegeGameMode>();
	const TSoftObjectPtr<UWorld> Arena = Defaults ? Defaults->ArenaLevel : TSoftObjectPtr<UWorld>();
	if (Arena.IsNull())
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[ASiegeGameMode::StartMatch] ArenaLevel is unset on the CDO — cannot start a match (expected /Game/Maps/L_Arena)."));
		return;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[ASiegeGameMode::StartMatch] Main menu -> opening arena '%s' into a fresh match vs the bot (GDD §7)."),
		*Arena.ToString());

	// OpenLevelBySoftObjectPtr resolves the FULL asset path (robust vs a bare
	// short package name in a packaged build) and performs an absolute travel to a
	// clean world: the new ASiegeGameMode's BeginPlay spawns the bot and deals both
	// decks, so this route needs no in-place reset.
	UGameplayStatics::OpenLevelBySoftObjectPtr(WorldContextObject, Arena);
}

void ASiegeGameMode::StartSandboxMatch(const UObject* WorldContextObject)
{
	// Dev/test Sandbox entry (CONVENTIONS "Dev / test tooling", TASK-071):
	// deliberately mirrors StartMatch verbatim, then appends the ?Sandbox=1 option
	// so the fresh L_Arena world's InitGame latches bSandboxMatch — SpawnBot then
	// early-returns (no AI opponent) and the Blue player starts with the generous
	// SandboxStartingGold. StartMatch (Play vs Bot) is left byte-identical; this is
	// a SEPARATE static entry point (WBP_MainMenu's "Sandbox (No Bot)" button,
	// TASK-072). Same STATIC + WorldContext contract as StartMatch — L_MainMenu
	// runs its own menu game mode, so no ASiegeGameMode instance is required.
	if (!WorldContextObject)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[ASiegeGameMode::StartSandboxMatch] No world context object — cannot open the arena (Sandbox test bench, TASK-071)."));
		return;
	}

	const ASiegeGameMode* Defaults = GetDefault<ASiegeGameMode>();
	const TSoftObjectPtr<UWorld> Arena = Defaults ? Defaults->ArenaLevel : TSoftObjectPtr<UWorld>();
	if (Arena.IsNull())
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[ASiegeGameMode::StartSandboxMatch] ArenaLevel is unset on the CDO — cannot start a sandbox match (expected /Game/Maps/L_Arena)."));
		return;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[ASiegeGameMode::StartSandboxMatch] Main menu -> opening arena '%s' into a fresh SANDBOX match (no bot, ?Sandbox=1, TASK-071)."),
		*Arena.ToString());

	// Same OpenLevelBySoftObjectPtr travel as StartMatch, plus the "Sandbox=1"
	// option string that InitGame parses. bAbsolute = true is passed explicitly to
	// match StartMatch's implicit default while carrying the option token.
	UGameplayStatics::OpenLevelBySoftObjectPtr(WorldContextObject, Arena, /*bAbsolute*/ true, TEXT("Sandbox=1"));
}

//~ FindLocalSiegeController DELETED by TASK-356 (M8 doc §3.4.4/§3.7): the
//~ "first ASiegePlayerController is THE player" resolve was the audit-§1a#4
//~ ban-shaped helper. Per-player restore is parameterized; team-keyed resolves
//~ use ASiegePlayerController::FindControllerForTeam.

void ASiegeGameMode::SpawnBot()
{
	// M8 networked-match gate (TASK-356 doc §2.2/D2 — retires audit §1b#3's
	// unconditional bot): in a networked 1v1 the RED seat is HUMAN (the joiner,
	// seat latch §2.1) — no bot controller, no bot ASiegePlayerState, so exactly
	// one PS per team resolves and GetPlayerStateForTeam is never ambiguous.
	// Every downstream bot reader is IsValid(BotController)-guarded (the same
	// null-safe world the sandbox path below already proves out). Standalone:
	// the latch is false ⇒ byte-identical ("Play vs Bot" untouched, ruling 3).
	if (bNetworkedMatch)
	{
		UE_LOG(LogSiegeNet, Log,
			TEXT("[%s] SpawnBot skipped — networked 1v1 (the Red seat is human; M8 doc §2.2)."),
			*GetNameSafe(this));
		return;
	}
	// Sandbox test bench (CONVENTIONS "Dev / test tooling", TASK-071): NO AI
	// opponent. Early-return before any ASiegeBotController is spawned and before
	// any Red bot ASiegePlayerState is created (bWantsPlayerState), so at BeginPlay
	// ZERO bot exists and no bot decision ever fires. The Red ACastle (Castle_Red)
	// is level-placed and still present, so Blue units/buildings march on it as a
	// static target dummy and the win condition still fires — OnCastleDestroyedHandler
	// reads the destroyed castle's TEAM, never a Red player state. Every downstream
	// reader of the (now-absent) Red ASiegePlayerState is already null-safe: this
	// reproduces the M2 no-bot world, where GetPlayerStateForTeam(Red) returning
	// nullptr is the documented normal case (see SiegeGameState.cpp) — the freeze /
	// Play Again bot hooks are IsValid(BotController)-guarded, and the only Red-PS
	// readers (AMinerUnit / ADeepMine) resolve their OWN team and null-check the result.
	if (bSandboxMatch)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] SpawnBot skipped — Sandbox match (no AI opponent, no Red PlayerState, TASK-071)."),
			*GetNameSafe(this));
		return;
	}

	// Exactly one bot per match (GDD §4). BeginPlay runs once per world begin, and
	// PlayAgain is an in-place reset that never re-runs BeginPlay, so this is
	// normally the only call — the guard covers a defensive re-entry only.
	if (IsValid(BotController))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Fall back to the C++ bot if a designer cleared the class in a subclass CDO.
	TSubclassOf<ASiegeBotController> ClassToSpawn = BotControllerClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ASiegeBotController::StaticClass();
	}

	// A controller has no physical presence, so AlwaysSpawn (no collision test).
	// It possesses nothing (§4 "controls no hero"); bWantsPlayerState creates its
	// ASiegePlayerState (PlayerStateClass = ASiegePlayerState) in the bot's
	// PostInitializeComponents — valid before SpawnActor returns. RF_Transient so
	// the runtime-spawned controller never tries to save into the map.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;

	BotController = World->SpawnActor<ASiegeBotController>(ClassToSpawn, FTransform::Identity, SpawnParams);
	if (!BotController)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("[%s] Failed to spawn the ASiegeBotController (GDD §4, TASK-045) — the match has no AI opponent."),
			*GetNameSafe(this));
		return;
	}

	// Tag the bot's auto-created ASiegePlayerState Team=Red — completing the
	// TASK-043 forward-ref (the mode sets both teams: Blue in InitNewPlayer, Red
	// here). The value comes from the bot's own BotTeam so the PS team, the §4
	// spawn geometry, and GetPlayerStateForTeam can never diverge. Team is pure
	// identity: no economy delegate fires and GetGoldRate never reads it, so
	// setting it just after the PS's BeginPlay does not disturb the bot's accrual.
	if (ASiegePlayerState* BotPS = BotController->GetBotPlayerState())
	{
		BotPS->SetTeam(BotController->GetBotTeam());
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] Spawned bot opponent '%s' with a Red ASiegePlayerState '%s' (GDD §4, TASK-045)."),
			*GetNameSafe(this), *GetNameSafe(BotController), *GetNameSafe(BotPS));
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] Bot '%s' has no ASiegePlayerState to tag Team=Red (bWantsPlayerState should have created one of PlayerStateClass=ASiegePlayerState) — its Red economy will not resolve via GetPlayerStateForTeam."),
			*GetNameSafe(this), *GetNameSafe(BotController));
	}
}

void ASiegeGameMode::GrantSandboxStartingGold()
{
	// Guard: only a sandbox match ever grants (BeginPlay/PlayAgain gate on the
	// same flag, but this stays self-guarding for the deferred-timer entry).
	if (!bSandboxMatch)
	{
		return;
	}

	// Resolve the Blue economy through the public ASiegeGameState accessor
	// (GetPlayerStateForTeam, TASK-043) rather than assuming the first player
	// state — and it is the ONLY player state in a sandbox match (no Red bot PS).
	// A local named SiegeGameState (never GameState) avoids shadowing the
	// inherited AGameModeBase::GameState reflected UPROPERTY (CONVENTIONS C4458).
	ASiegeGameState* SiegeGameState = Cast<ASiegeGameState>(GameState);
	ASiegePlayerState* BlueState = SiegeGameState ? SiegeGameState->GetPlayerStateForTeam(ETeamId::Blue) : nullptr;
	if (!BlueState)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("[%s] Sandbox gold grant skipped — no Blue ASiegePlayerState resolved yet (TASK-071)."),
			*GetNameSafe(this));
		return;
	}

	// Grant through the gold API: AddGold routes through the player state's single
	// SetGold() choke point, so the [0, MaxGold] clamp and OnGoldChanged broadcast
	// both apply — NEVER a raw Gold field write. The BASE gold rate is untouched
	// (no AddIncome), so the normal base economy stands (1 gold per 1 s, TASK-278
	// (2026-07-24) reverting TASK-089's 1-per-2-s rate — ASiegePlayerState's
	// GoldPerTick=1 / BaseIncomeTickPeriod=1 are the record of truth, GDD §3.2;
	// spec: keep the normal rate). NOTE: MaxGold (999) clamps SandboxStartingGold
	// (9999) to 999 — still a full generous pile for the 22-card roster (flagged
	// for QA).
	BlueState->AddGold(SandboxStartingGold);

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] Sandbox: granted the Blue player %d starting gold (now %d after the MaxGold clamp, TASK-071)."),
		*GetNameSafe(this), SandboxStartingGold, BlueState->GetGold());
}
