// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"                 // TASK-1600: GEngine — CreateNewWorldContext / DestroyWorldContext / ShutdownWorldNetDriver (the TASK-1173 fixture shape)
#include "Engine/EngineBaseTypes.h"        // TASK-1600: FURL — InitializeActorsForPlay's argument
#include "Engine/EngineTypes.h"            // TASK-1600: ESpawnActorCollisionHandlingMethod — AlwaysSpawn, exactly as ASiegeGameMode::SpawnBot spawns the bot
#include "Engine/World.h"                  // TASK-1600: UWorld::CreateWorld / SpawnActor / FActorSpawnParameters
#include "EngineUtils.h"                   // TASK-1600: FActorRange — the fixture's teardown RouteEndPlay loop (inert without a game mode; kept as the fixture's own shape)
#include "GameFramework/Actor.h"           // TASK-1600: AActor::RouteEndPlay (explicit IWYU — no compile verifies a transitive pull)
#include "HAL/IConsoleManager.h"           // TASK-1600: IConsoleManager::FindConsoleVariable + IConsoleVariable::Set / Unset / GetInt / GetFlags — siege.BotEnabled
#include "Math/Transform.h"                // TASK-1600: FTransform::Identity — a controller has no physical presence
#include "Siegebound/SiegeBotController.h" // TASK-1600: ASiegeBotController::SetBotEnabled / IsBotEnabled — the two symbols under test
#include "UObject/UnrealType.h"            // TASK-1600: FBoolProperty / FindFProperty — reading the PRIVATE bBotEnabled UPROPERTY back by reflection
#include "UObject/UObjectGlobals.h"        // TASK-1600: MakeUniqueObjectName / GetTransientPackage

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE DEV-ONLY BOT SWITCH (TASK-1600; gate TASK-1601; 5a TASK-1602;
 *  5b TASK-1603). Two tests, the two the row names:
 *
 *    Siegebound.Bot.SetBotEnabled.FlagRoundTrip — an ASiegeBotController spawned in a test
 *      world answers IsBotEnabled() true, false after SetBotEnabled(false), true after
 *      SetBotEnabled(true); the PRIVATE bBotEnabled UPROPERTY is read back by reflection at
 *      each step, so the FLAG is shown to move, not merely the composed answer.
 *    Siegebound.Bot.SetBotEnabled.CVarComposes — `siege.BotEnabled 0` makes IsBotEnabled()
 *      false WITH THE FLAG STILL TRUE (reflection read), the two halves compose with AND
 *      (flag off→on under cvar 0 never re-enables), and the console variable is RESTORED at
 *      test end by Unset(ECVF_SetByCode) — the house release idiom (AFogVolume::
 *      ReleaseFogRenderFloor) — with the value AND the SetBy layer read back to prove it, so
 *      no later test in this process runs under a variable this one stranded.
 *
 *  ⚠ WHAT THIS FILE DOES NOT MEASURE, ON PURPOSE: the "exactly once per effective transition"
 *  LogSiegeBot property is TASK-1603's to measure in PIE (the row says so); the Shipping
 *  no-op is TASK-1601's to read in the guards. Nothing here counts log lines.
 *
 *  ⚖️ THE WORLD. The row's spec says "spawn an ASiegeBotController in a test world", so this
 *  file uses a REAL UWorld — the TASK-1173 fixture (SiegeFogVisualTest.cpp,
 *  SiegeFogRealWorldFixture::FScopedPlayWorld, itself the engine's FActorTestSpawner shape:
 *  context ⇒ world ⇒ root ⇒ SetCurrentWorld ⇒ InitializeActorsForPlay ⇒ BeginPlay), copied
 *  step for step rather than re-invented. ⛔ The older house sentence "not one SpawnActor and
 *  not one UWorld::CreateWorld anywhere in Siegebound/Tests/" is STALE since TASK-1173 /
 *  TASK-1178 (that fixture's own comment says so); this is the second such file, by the
 *  manager's explicit spec, and it is declared here rather than hidden.
 *  ⭐ WHY THE WORLD IS SAFE FOR THIS ACTOR, MEASURED NOT ASSUMED (the TASK-1174/1178 finding):
 *  this world has NO game mode, so AWorldSettings::NotifyBeginPlay never runs and
 *  World->HasBegunPlay() is false for the world's whole life ⇒ SpawnActor runs the bot's
 *  constructor and PostInitializeComponents (InitPlayerState skips: no game mode, no game
 *  state) but ⛔ NEVER its BeginPlay — so no deck build, no DT_Cards load, no decision timer,
 *  no deck-select log line. The ONLY project code these tests execute is the switch itself:
 *  SetBotEnabled / IsBotEnabled / RefreshBotEnabledTransition. The transient package is the
 *  world package, so no map can be dirtied or saved. Teardown is the fixture's.
 */

namespace SiegeBotSwitchTestFixture
{
	/**
	 *  The TASK-1173 real-world fixture, copied from SiegeFogVisualTest.cpp (the
	 *  FActorTestSpawner shape). World first, context second, so a failed CreateWorld is a
	 *  plain early return with nothing leaked (DestroyWorldContext is keyed by world).
	 *  The teardown RouteEndPlay loop is INERT here — no game mode ⇒ bBegunPlay is never set
	 *  ⇒ RouteEndPlay returns without dispatching — and is kept as the fixture's own shape
	 *  (it becomes live the day the rig grows a game mode). Nothing this file spawns needs an
	 *  EndPlay: the bot's EndPlay only clears a timer that BeginPlay never started.
	 */
	struct FScopedPlayWorld
	{
		UWorld* World = nullptr;

		FScopedPlayWorld()
		{
			if (!GEngine)
			{
				return;
			}

			const FName WorldName = MakeUniqueObjectName(
				nullptr, UWorld::StaticClass(), NAME_None, EUniqueObjectNameOptions::GloballyUnique);

			World = UWorld::CreateWorld(
				EWorldType::Game, /*bInformEngineOfWorld=*/ false, WorldName, GetTransientPackage());

			if (!World)
			{
				return;
			}

			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);

			World->AddToRoot();
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
		}

		~FScopedPlayWorld()
		{
			if (!World || !GEngine)
			{
				return;
			}

			if (World->AreActorsInitialized())
			{
				for (AActor* const Actor : FActorRange(World))
				{
					if (Actor)
					{
						Actor->RouteEndPlay(EEndPlayReason::LevelTransition);
					}
				}
			}

			GEngine->ShutdownWorldNetDriver(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ true);
			World->SetPhysicsScene(nullptr);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
			World = nullptr;
		}

		FScopedPlayWorld(const FScopedPlayWorld&) = delete;
		FScopedPlayWorld& operator=(const FScopedPlayWorld&) = delete;
	};

	/**
	 *  Spawns the bot the way ASiegeGameMode::SpawnBot does (AlwaysSpawn — a controller has
	 *  no physical presence; RF_Transient — never saved), on the C++ class directly. Null when
	 *  the world is null or the spawn fails; the caller asserts.
	 */
	ASiegeBotController* SpawnBot(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags |= RF_Transient;
		return World->SpawnActor<ASiegeBotController>(ASiegeBotController::StaticClass(), FTransform::Identity, SpawnParams);
	}

	/**
	 *  Reads the PRIVATE bBotEnabled UPROPERTY off a live bot by reflection — the only honest
	 *  way to show the FLAG moved (or stayed), as distinct from the composed IsBotEnabled()
	 *  answer. False (with an AddError) when the property is missing or not a bool, which
	 *  would mean the row's pinned `bool bBotEnabled` UPROPERTY is not what shipped.
	 */
	bool ReadBotEnabledFlag(FAutomationTestBase& Test, const ASiegeBotController& Bot, bool& OutFlag)
	{
		const FBoolProperty* FlagProperty = FindFProperty<FBoolProperty>(ASiegeBotController::StaticClass(), TEXT("bBotEnabled"));
		if (!FlagProperty)
		{
			Test.AddError(TEXT("ASiegeBotController has no bool UPROPERTY named bBotEnabled — the row pins one (private, VisibleInstanceOnly, Transient); the flag cannot be read back"));
			return false;
		}

		OutFlag = FlagProperty->GetPropertyValue_InContainer(&Bot);
		return true;
	}

	/** The console variable's name, spelled once. The row pins `siege.BotEnabled`. */
	static const TCHAR* const BotEnabledCVarName = TEXT("siege.BotEnabled");

	/** True when the variable currently reports its value as set at ECVF_SetByCode — the house priority read (SiegeFogVisualTest.cpp step (6)), copied character for character. */
	bool IsPinnedBySetByCode(const IConsoleVariable& CVar)
	{
		return static_cast<EConsoleVariableFlags>(CVar.GetFlags() & ECVF_SetByMask) == ECVF_SetByCode;
	}

	/**
	 *  RAII: holds siege.BotEnabled at 0 (Set at ECVF_SetByCode — the Set itself is never
	 *  cheat-gated; only console INPUT is) for the scope, and RESTORES it on EVERY exit path by
	 *  Unset(ECVF_SetByCode), which drops this layer so the variable falls back to whatever
	 *  held it before (its constructor default 1 in a clean suite) and no longer reports
	 *  SetByCode. ⛔ Not a Set(prior, SetByCode) restore: that would leave the SetBy layer
	 *  pinned for the rest of the process (AFogVolume::ReleaseFogRenderFloor's own reasoning).
	 */
	struct FScopedBotCVarHeldAtZero
	{
		IConsoleVariable* CVar = nullptr;

		explicit FScopedBotCVarHeldAtZero(IConsoleVariable* InCVar)
			: CVar(InCVar)
		{
			if (CVar)
			{
				CVar->Set(0, ECVF_SetByCode);
			}
		}

		~FScopedBotCVarHeldAtZero()
		{
			if (CVar)
			{
				CVar->Unset(ECVF_SetByCode);
			}
		}

		FScopedBotCVarHeldAtZero(const FScopedBotCVarHeldAtZero&) = delete;
		FScopedBotCVarHeldAtZero& operator=(const FScopedBotCVarHeldAtZero&) = delete;
	};
}

// ═════════════════════════════════════════════════════════════════════════════════════════════
//  TEST 1 — Siegebound.Bot.SetBotEnabled.FlagRoundTrip
// ═════════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBotSetBotEnabledFlagRoundTripTest,
	"Siegebound.Bot.SetBotEnabled.FlagRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBotSetBotEnabledFlagRoundTripTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBotSwitchTestFixture;

	// ── (0) PRECONDITION ON THE CONSOLE HALF — so the flag is the only moving part below ──
	IConsoleVariable* const CVar = IConsoleManager::Get().FindConsoleVariable(BotEnabledCVarName);
	if (!TestNotNull(TEXT("(0) siege.BotEnabled exists in this (non-shipping) build"), CVar))
	{
		return false;
	}
	if (!TestEqual(TEXT("(0) siege.BotEnabled reads its default 1 before the test — any other value means something stranded it"), CVar->GetInt(), 1))
	{
		return false;
	}

	// ── (1) A BOT IN A TEST WORLD ─────────────────────────────────────────────────────────
	FScopedPlayWorld Scoped;
	if (!TestNotNull(TEXT("(1) a test world was created (GEngine present, CreateWorld succeeded)"), Scoped.World))
	{
		return false;
	}

	ASiegeBotController* const Bot = SpawnBot(Scoped.World);
	if (!TestNotNull(TEXT("(1) an ASiegeBotController spawned in the test world"), Bot))
	{
		return false;
	}

	// ── (2) FRESH: true / true ────────────────────────────────────────────────────────────
	bool bFlag = false;
	TestTrue(TEXT("(2) a fresh bot answers IsBotEnabled() true"), Bot->IsBotEnabled());
	if (ReadBotEnabledFlag(*this, *Bot, bFlag))
	{
		TestTrue(TEXT("(2) a fresh bot's bBotEnabled flag is true (reflection read)"), bFlag);
	}

	// ── (3) OFF: false / false ────────────────────────────────────────────────────────────
	Bot->SetBotEnabled(false);
	TestFalse(TEXT("(3) after SetBotEnabled(false) IsBotEnabled() is false"), Bot->IsBotEnabled());
	if (ReadBotEnabledFlag(*this, *Bot, bFlag))
	{
		TestFalse(TEXT("(3) after SetBotEnabled(false) the bBotEnabled flag is false (the FLAG moved, not just the composed answer)"), bFlag);
	}

	// ── (4) SAME-VALUE REPEAT: still false (idempotent; the log side of this is TASK-1603's) ──
	Bot->SetBotEnabled(false);
	TestFalse(TEXT("(4) a same-value SetBotEnabled(false) repeat leaves IsBotEnabled() false"), Bot->IsBotEnabled());

	// ── (5) ON AGAIN: true / true ─────────────────────────────────────────────────────────
	Bot->SetBotEnabled(true);
	TestTrue(TEXT("(5) after SetBotEnabled(true) IsBotEnabled() is true"), Bot->IsBotEnabled());
	if (ReadBotEnabledFlag(*this, *Bot, bFlag))
	{
		TestTrue(TEXT("(5) after SetBotEnabled(true) the bBotEnabled flag is true"), bFlag);
	}

	// ── (6) THE CONSOLE HALF WAS NEVER TOUCHED ────────────────────────────────────────────
	TestEqual(TEXT("(6) this test never wrote siege.BotEnabled — it still reads 1"), CVar->GetInt(), 1);
	TestFalse(TEXT("(6) and it is not held at SetByCode"), IsPinnedBySetByCode(*CVar));

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════════════════════
//  TEST 2 — Siegebound.Bot.SetBotEnabled.CVarComposes
// ═════════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBotSetBotEnabledCVarComposesTest,
	"Siegebound.Bot.SetBotEnabled.CVarComposes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBotSetBotEnabledCVarComposesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBotSwitchTestFixture;

	// ── (0) THE VARIABLE, AND ITS STATE BEFORE WE TOUCH IT (what the restore must return to) ──
	IConsoleVariable* const CVar = IConsoleManager::Get().FindConsoleVariable(BotEnabledCVarName);
	if (!TestNotNull(TEXT("(0) siege.BotEnabled exists in this (non-shipping) build"), CVar))
	{
		return false;
	}

	const int32 PriorValue = CVar->GetInt();
	if (!TestEqual(TEXT("(0) siege.BotEnabled reads its default 1 before the test — any other value means something stranded it"), PriorValue, 1))
	{
		return false;
	}
	if (!TestFalse(TEXT("(0) siege.BotEnabled is NOT held at SetByCode before the test — the Unset at the end must return it to exactly this"), IsPinnedBySetByCode(*CVar)))
	{
		return false;
	}

	// ── (1) A BOT IN A TEST WORLD, EFFECTIVELY ENABLED ────────────────────────────────────
	FScopedPlayWorld Scoped;
	if (!TestNotNull(TEXT("(1) a test world was created"), Scoped.World))
	{
		return false;
	}

	ASiegeBotController* const Bot = SpawnBot(Scoped.World);
	if (!TestNotNull(TEXT("(1) an ASiegeBotController spawned in the test world"), Bot))
	{
		return false;
	}

	TestTrue(TEXT("(1) before the console half moves, IsBotEnabled() is true"), Bot->IsBotEnabled());

	// ── (2)-(3) UNDER `siege.BotEnabled 0` — the scope guard restores on every exit path ──
	{
		FScopedBotCVarHeldAtZero HeldAtZero(CVar);

		TestEqual(TEXT("(2) siege.BotEnabled now reads 0"), CVar->GetInt(), 0);
		TestFalse(TEXT("(2) IsBotEnabled() is false with ONLY the console half off"), Bot->IsBotEnabled());

		bool bFlag = false;
		if (ReadBotEnabledFlag(*this, *Bot, bFlag))
		{
			TestTrue(TEXT("(2) the bBotEnabled flag is STILL true — the console variable held the bot off, the flag did not move"), bFlag);
		}

		// AND, not OR: the function half cannot override the console half.
		Bot->SetBotEnabled(false);
		TestFalse(TEXT("(3) flag off + cvar 0: IsBotEnabled() false"), Bot->IsBotEnabled());
		Bot->SetBotEnabled(true);
		TestFalse(TEXT("(3) flag on + cvar 0: IsBotEnabled() STILL false — the two halves compose with AND"), Bot->IsBotEnabled());
		if (ReadBotEnabledFlag(*this, *Bot, bFlag))
		{
			TestTrue(TEXT("(3) and the flag is back to true under the still-zero variable"), bFlag);
		}
	}

	// ── (4) RESTORED — value AND priority layer, read back off the machine ────────────────
	TestEqual(TEXT("(4) after the scope the variable reads its prior value again (Unset dropped the SetByCode layer)"), CVar->GetInt(), PriorValue);
	TestFalse(TEXT("(4) and it is no longer held at SetByCode — nothing stranded for the rest of the suite"), IsPinnedBySetByCode(*CVar));

	// ── (5) THE CONTROL: no SetBotEnabled call since (3) left the flag true, and the bot is enabled again ──
	TestTrue(TEXT("(5) IsBotEnabled() is true again with NO SetBotEnabled call after the restore — only the console half had moved"), Bot->IsBotEnabled());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
