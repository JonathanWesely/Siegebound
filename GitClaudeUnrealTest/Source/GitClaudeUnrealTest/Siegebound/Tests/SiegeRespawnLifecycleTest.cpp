// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "GameFramework/Pawn.h"
#include "Math/Rotator.h"            // TASK-1102: FRotator — the two death-camera claims are stated in rotations
#include "Misc/FileHelper.h"         // TASK-1102: FFileHelper::LoadFileToString — the CALL-SITE CENSUS half (SC-§36.1)
#include "Misc/Paths.h"              // TASK-1102: FPaths::ProjectDir — ditto
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeDeathCameraStatics.h" // TASK-1102: the two pure rules under test
#include "Siegebound/SiegeGameMode.h"
#include "Siegebound/SiegeGhostPawn.h"
#include "Siegebound/SiegePlayerController.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE DEATH → GHOST → 180 s → RESPAWN LIFECYCLE
 *      (TASK-750; law: CONVENTIONS `GHOST-§0`/`§2`/`§3`/`§4`/`§5`/`§6`, `HELP-§5`,
 *      `HIGH-§1`, `SHIP-§9c`) ═══
 *
 *  ⭐ WHAT THIS FILE IS ABOUT: the LIFECYCLE — the number, the transition rules,
 *  which hero comes back, and what the ghost may not do. It is deliberately NOT
 *  about the ghost PAWN, whose structural claims (⛔ not an `ITeamAgent`, ⛔ not an
 *  `IHealthBarProvider`, the `G-2` collision response, the net tier) are already
 *  asserted by `SiegeGhostPawnTest.cpp` (TASK-749). ⛔ Nothing here duplicates a
 *  claim from that file; the one place the two touch — "the ghost is not a hero" —
 *  is used here as a SELF-CHECK rather than re-asserted as a finding.
 *
 *  ⚠️⚠️ THE LESSON THIS FILE WAS WRITTEN AGAINST, AND IT HAS COST TWO QA LOOPS THIS
 *  WEEK: **AN ASSERTION WHOSE TWO SIDES ARE EQUAL BY CONSTRUCTION PROVES NOTHING.**
 *  Every claim below therefore ships with a SELF-CHECK that fails if the instrument
 *  has gone blind:
 *    • the property reader is validated against a DIFFERENT property with a
 *      DIFFERENT value on the SAME class, through the SAME call,
 *    • the property-flag reader is validated by finding a property that does NOT
 *      carry the flag being tested,
 *    • the class scan is validated by requiring it to FIND a token it must find
 *      before its failure to find another token is allowed to mean anything,
 *    • every truth-table test asserts at least one TRUE row and at least one FALSE
 *      row, so a function that hard-returned a constant could not pass,
 *    • the ghost-path resolve is validated by first proving that the PREVIOUS,
 *      shipped implementation would have returned a DIFFERENT answer for it.
 *  If any self-check ever fires, the test around it was about to pass vacuously.
 *
 *  MECHANISM — ⛔ zero world, ⛔ zero PIE, ⛔ zero asset loads, ⛔ zero writes. Class
 *  default objects, the reflection tables, and the two pure statics the lifecycle
 *  is built on. Identical in kind to `SiegeHighGroundTest.cpp` and
 *  `SiegeClimbableTowerTest.cpp`; ⛔ there is not one `UWorld::CreateWorld` and not
 *  one `SpawnActor` in this directory and this file does not introduce the first.
 *
 *  ⛔ WHAT THIS FILE DELIBERATELY DOES NOT COVER (`SC-§32` — green here is ⛔ NOT
 *  "the ghost lifecycle works"):
 *    • ⛔ THAT THE POSSESSION ACTUALLY HANDS THE PLAYER WORKING INPUT. Enhanced
 *      Input mapping contexts, the pawn input-component rebuild and the cursor
 *      posture are live-session facts. `GHOST-§4` names the instrument and it has
 *      ⛔ NO SUBSTITUTE: ONE PIE session with a MESSAGE-LOG READ, and that row may
 *      ⛔ never be waived on the grounds that the compile is clean.
 *    • ⛔ THAT THE HERO RESPAWNS AT THE CASTLE. `GetHeroStartTransform` traces live
 *      castle bounds; it is settled by integration and by Jonathan's playtest.
 *      What IS asserted here is the half a headless suite can prove: that the
 *      respawn restores the SAME hero actor, so the shipped own-castle resolve is
 *      handed the same team it was handed before the death.
 *    • ⛔ THAT 180 SECONDS IS THE RIGHT NUMBER. That is Jonathan's design call
 *      (`GHOST-§0`), TASK-752 measures the thing it trades against, and no test can
 *      or should have an opinion about it.
 */
namespace SiegeRespawnLifecycleTestFixture
{
	/** Jonathan's three minutes, as seconds. Written once, so no test re-types it. */
	constexpr float ThreeMinutesSeconds = 180.f;

	/** Exact-equality tolerance — the whole content of the claim is the word EXACTLY. */
	constexpr float Exact = 0.f;

	/** Reads a shipped float UPROPERTY off a class default object. Returns false (writing nothing) when the property is gone — the caller FAILS on that rather than substituting a guess. */
	static bool TryReadDefaultFloat(const UClass* Class, const UObject* Defaults, const TCHAR* PropertyName, float& OutValue)
	{
		if (!Class || !Defaults)
		{
			return false;
		}

		const FFloatProperty* const FloatProperty = CastField<FFloatProperty>(Class->FindPropertyByName(FName(PropertyName)));
		if (!FloatProperty)
		{
			return false;
		}

		OutValue = FloatProperty->GetPropertyValue_InContainer(Defaults);
		return true;
	}

	/** Every reflected member a class DECLARES ITSELF (ExcludeSuper) — properties then functions. */
	static void CollectDeclaredMemberNames(const UClass* Class, TArray<FString>& OutNames)
	{
		if (!Class)
		{
			return;
		}
		for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
		{
			OutNames.Add(PropertyIt->GetName());
		}
		for (TFieldIterator<UFunction> FunctionIt(Class, EFieldIteratorFlags::ExcludeSuper); FunctionIt; ++FunctionIt)
		{
			OutNames.Add(FunctionIt->GetName());
		}
	}

	/** True when any DECLARED member name of the class contains Token (case-insensitive). */
	static bool DeclaresMemberContaining(const UClass* Class, const TCHAR* Token)
	{
		TArray<FString> Names;
		CollectDeclaredMemberNames(Class, Names);
		for (const FString& Name : Names)
		{
			if (Name.Contains(Token, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 *  True when any DECLARED **PROPERTY** name contains Token — ⛔ functions excluded,
	 *  deliberately. "This class holds no recall STATE" is a claim about data; a handler
	 *  function named after the thing it listens to is not state and must not answer it.
	 */
	static bool DeclaresPropertyContaining(const UClass* Class, const TCHAR* Token)
	{
		if (!Class)
		{
			return false;
		}
		for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
		{
			if (PropertyIt->GetName().Contains(Token, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 *  Every DECLARED floating-point UPROPERTY on a class, as {name, value read off the
	 *  CDO}. Scalars only — an `FVector` is an `FStructProperty` and is not visited, so
	 *  the game mode's spawn-offset components can never be mistaken for a tunable.
	 */
	static void CollectDeclaredFloatDefaults(const UClass* Class, const UObject* Defaults, TArray<TPair<FString, double>>& OutValues)
	{
		if (!Class || !Defaults)
		{
			return;
		}
		for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
		{
			const FNumericProperty* const Numeric = CastField<FNumericProperty>(*PropertyIt);
			if (!Numeric || !Numeric->IsFloatingPoint())
			{
				continue;
			}
			const void* const ValuePtr = Numeric->ContainerPtrToValuePtr<void>(Defaults);
			OutValues.Emplace(Numeric->GetName(), Numeric->GetFloatingPointPropertyValue(ValuePtr));
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  1. ⭐⭐ HIS THREE MINUTES, AT THE SHIPPED VALUE — `HeroRespawnDelay` IS EXACTLY
//     180 AND IT IS AN `EditDefaultsOnly` TUNABLE (GHOST-§0, GHOST-§5)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRespawnDelayIsJonathansThreeMinutesTest,
	"Siegebound.RespawnLifecycle.HeroRespawnDelayIsExactlyOneHundredEightySeconds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRespawnDelayIsJonathansThreeMinutesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRespawnLifecycleTestFixture;

	const UClass* const ModeClass = ASiegeGameMode::StaticClass();
	const ASiegeGameMode* const ModeDefaults = GetDefault<ASiegeGameMode>();

	if (!ModeClass || !ModeDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<ASiegeGameMode>() or its UClass is null — nothing below could mean anything."));
		return false;
	}

	// ── SELF-CHECK: the reader reads REAL values, not a constant ────────────────────────
	// A DIFFERENT property, on the SAME class, through the SAME call, with a DIFFERENT
	// value. If TryReadDefaultFloat ever started returning a fixed number, this fires
	// before the claim below can pass by accident.
	float ClearanceProbe = -1.f;
	if (!TryReadDefaultFloat(ModeClass, ModeDefaults, TEXT("HeroSpawnCastleClearance"), ClearanceProbe))
	{
		AddError(TEXT("SELF-CHECK FAILED: HeroSpawnCastleClearance did not resolve as a float UPROPERTY — the probe is blind, so the 180 claim below cannot be trusted."));
		return false;
	}
	TestEqual(TEXT("SELF-CHECK: the probe reads a DIFFERENT value off a DIFFERENT property on the same CDO (300)"),
		ClearanceProbe, 300.f, Exact);

	// ── (a) THE NUMBER ITSELF ────────────────────────────────────────────────────────────
	float RespawnDelay = -1.f;
	if (!TryReadDefaultFloat(ModeClass, ModeDefaults, TEXT("HeroRespawnDelay"), RespawnDelay))
	{
		AddError(TEXT("HeroRespawnDelay is not a declared float UPROPERTY on ASiegeGameMode — GHOST-§5 names it, and the whole feature is that one number."));
		return false;
	}

	TestEqual(TEXT("(a) ⭐ HeroRespawnDelay is EXACTLY 180 s — Jonathan's \"they are dead for 3 minutes\" (GHOST-§0)"),
		RespawnDelay, ThreeMinutesSeconds, Exact);

	// ── (b) AND IT IS NOT THE RETIRED VALUE ─────────────────────────────────────────────
	// Stated separately and on purpose: the shipped value was 5 s and the GDD sentence
	// "back within 5-6 s" is now FALSE by his ruling. This assertion is what goes red the
	// day someone "restores" the GDD's number without his say-so.
	TestTrue(TEXT("(b) ⛔ it is NOT the retired 5 s — the 36× change is a Jonathan ruling and no agent may soften it"),
		!FMath::IsNearlyEqual(RespawnDelay, 5.f));

	// ── (c) IT IS A TUNABLE HE CAN TURN, NOT A CONSTANT SOMEONE MUST RECOMPILE ──────────
	// EditDefaultsOnly == Edit | DisableEditOnInstance. TASK-752 is re-measuring the
	// thing this number trades against, so "he can retune it in the editor" is part of
	// the deliverable, not a nicety.
	const FProperty* const DelayProperty = ModeClass->FindPropertyByName(TEXT("HeroRespawnDelay"));
	if (!DelayProperty)
	{
		AddError(TEXT("HeroRespawnDelay vanished between two lookups — impossible; treat as an instrument failure."));
		return false;
	}

	TestTrue(TEXT("(c) HeroRespawnDelay is editable on the class defaults (CPF_Edit)"),
		DelayProperty->HasAnyPropertyFlags(CPF_Edit));
	TestTrue(TEXT("(c) …and EditDefaultsOnly rather than EditAnywhere (CPF_DisableEditOnInstance)"),
		DelayProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance));

	// ── SELF-CHECK: the FLAG reader distinguishes, it does not just say yes ─────────────
	// A property that must NOT carry CPF_Edit. If this ever passes as "editable", the two
	// assertions above were meaningless.
	const FProperty* const TransientProperty = ModeClass->FindPropertyByName(TEXT("ResolvedHeroPawnClass"));
	if (!TransientProperty)
	{
		AddError(TEXT("SELF-CHECK FAILED: ResolvedHeroPawnClass did not resolve — the flag probe cannot be validated."));
		return false;
	}
	TestFalse(TEXT("SELF-CHECK: a Transient UPROPERTY on the same class is NOT CPF_Edit — the flag reader discriminates"),
		TransientProperty->HasAnyPropertyFlags(CPF_Edit));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. ⭐ ONE NUMBER, ONE PLACE — no second 180 exists on either class this task
//     owns, so retuning the ghost's lifetime is EXACTLY one edit (GHOST-§5)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRespawnDelayIsTheSingleTunableTest,
	"Siegebound.RespawnLifecycle.TheThreeMinutesIsASingleTunableWithNoSecondCopy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRespawnDelayIsTheSingleTunableTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRespawnLifecycleTestFixture;

	// The ghost lives exactly as long as the respawn timer, because the timer's own
	// callback is what retires it. That claim is only worth anything if there is no
	// SECOND number anywhere that a retune could leave behind — a ghost lifetime, a
	// "death screen" duration, a UI countdown constant. This walks both classes this
	// task owns and insists there is exactly one.
	TArray<TPair<FString, double>> Floats;
	CollectDeclaredFloatDefaults(ASiegeGameMode::StaticClass(), GetDefault<ASiegeGameMode>(), Floats);
	CollectDeclaredFloatDefaults(ASiegePlayerController::StaticClass(), GetDefault<ASiegePlayerController>(), Floats);

	// ── SELF-CHECK: the walk actually found things ──────────────────────────────────────
	// A scan that visited nothing would report "exactly zero second copies" and pass.
	if (Floats.Num() < 4)
	{
		AddError(FString::Printf(
			TEXT("SELF-CHECK FAILED: the float-property walk found only %d properties across ASiegeGameMode + ASiegePlayerController — the scanner is blind, so its failure to find a second 180 means nothing."),
			Floats.Num()));
		return false;
	}

	// ── SELF-CHECK: the VALUE comparison matches something other than the thing under
	//    test — otherwise a broken comparison would also report "no second copy". 300 is
	//    HeroSpawnCastleClearance, read through this same walk.
	int32 ThreeHundredCount = 0;
	int32 OneEightyCount = 0;
	FString OneEightyOwner;

	for (const TPair<FString, double>& Entry : Floats)
	{
		if (FMath::IsNearlyEqual(Entry.Value, 300.0))
		{
			++ThreeHundredCount;
		}
		if (FMath::IsNearlyEqual(Entry.Value, static_cast<double>(ThreeMinutesSeconds)))
		{
			++OneEightyCount;
			OneEightyOwner = Entry.Key;
		}
	}

	if (ThreeHundredCount < 1)
	{
		AddError(TEXT("SELF-CHECK FAILED: the walk matched NO property at 300 (HeroSpawnCastleClearance) — the value comparison is dead, so the 180 count below proves nothing."));
		return false;
	}

	TestEqual(TEXT("⭐ EXACTLY ONE declared float across ASiegeGameMode + ASiegePlayerController holds 180 — the ghost's lifetime is one tunable, not two numbers to keep in step"),
		OneEightyCount, 1);

	// Byte-identity claim about a NAME ⇒ TestEqualSensitive (SC-§13), never TestEqual —
	// FString's own comparison is case-insensitive and would accept "heroRespawnDelay".
	TestEqualSensitive(TEXT("…and it is HeroRespawnDelay — ⛔ not a duplicate hiding under another name"),
		*OneEightyOwner, TEXT("HeroRespawnDelay"));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. ⛔ THE TRANSITION RULE — a ghost exists IF AND ONLY IF a respawn is pending,
//     and match-end is INHERITED, never re-invented (GHOST-§2, GHOST-§3 (5))
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostStateTransitionRuleTest,
	"Siegebound.RespawnLifecycle.GhostStateIsEnteredExactlyWhenARespawnIsScheduled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostStateTransitionRuleTest::RunTest(const FString& Parameters)
{
	// The whole truth table. ⭐ ONE function governs BOTH the timer and the ghost, so
	// these four rows are simultaneously "is a respawn scheduled?" and "does the player
	// get a ghost?" — which is exactly what makes "a permanent ghost" and "180 seconds
	// with no pawn" unrepresentable rather than merely unlikely.

	// ── (a) THE LIVE MATCH, THE ONLY TRUE ROW ───────────────────────────────────────────
	const bool bLiveMatchWithController = ASiegeGameMode::ShouldEnterGhostState(/*bInMatchEnded*/ false, /*bHasOwningController*/ true);
	TestTrue(TEXT("(a) ⭐ mid-match death with an owning controller ⇒ ghost AND respawn"),
		bLiveMatchWithController);

	// ── (b) ⛔ MATCH ENDED ⇒ NEITHER. THE ALREADY-RULED RULE, INHERITED VERBATIM ─────────
	// SiegeGameMode.h ships "After match end no respawn is scheduled" and "the hero
	// stays down; PlayAgain() revives it". GHOST-§2 is explicit that inventing a SECOND
	// match-end rule for the ghost would be the defect. This row is that inheritance.
	const bool bEndedWithController = ASiegeGameMode::ShouldEnterGhostState(/*bInMatchEnded*/ true, /*bHasOwningController*/ true);
	TestFalse(TEXT("(b) ⛔ a death AFTER match end schedules nothing and spawns no ghost — the shipped rule, inherited, not a second one"),
		bEndedWithController);

	// ── (c) NO OWNING CONTROLLER ⇒ NEITHER ──────────────────────────────────────────────
	TestFalse(TEXT("(c) no owning controller ⇒ nobody to possess a ghost and nobody to respawn"),
		ASiegeGameMode::ShouldEnterGhostState(/*bInMatchEnded*/ false, /*bHasOwningController*/ false));

	TestFalse(TEXT("(c) …and both conditions failing is still false"),
		ASiegeGameMode::ShouldEnterGhostState(/*bInMatchEnded*/ true, /*bHasOwningController*/ false));

	// ── SELF-CHECK: THE PREDICATE IS NOT A CONSTANT ─────────────────────────────────────
	// Asserted explicitly rather than left implicit in the rows above: an implementation
	// that hard-returned true, or hard-returned false, must not be able to reach here.
	TestTrue(TEXT("SELF-CHECK: the predicate produced at least one TRUE and at least one FALSE — it is not a constant"),
		bLiveMatchWithController && !bEndedWithController);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  4. ⭐⭐ THE RESPAWN RESTORES THE **SAME HERO ACTOR** — the no-double-apply
//     guarantee, and the one row that used to resolve to nothing (GHOST-§2)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeRespawnRestoresTheSameHeroTest,
	"Siegebound.RespawnLifecycle.RespawnRestoresTheSameHeroActorAndNeverASecondOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeRespawnRestoresTheSameHeroTest::RunTest(const FString& Parameters)
{
	AHeroCharacter* const HeroDefaults = GetMutableDefault<AHeroCharacter>();
	ASiegeGhostPawn* const GhostDefaults = GetMutableDefault<ASiegeGhostPawn>();

	if (!HeroDefaults || !GhostDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: a class default object is null — no row below could mean anything."));
		return false;
	}

	// ── SELF-CHECK: THE TWO STAND-INS ARE GENUINELY DIFFERENT OBJECTS ───────────────────
	TestTrue(TEXT("SELF-CHECK: the hero CDO and the ghost CDO are different objects"),
		static_cast<APawn*>(HeroDefaults) != static_cast<APawn*>(GhostDefaults));

	// ── SELF-CHECK, AND IT IS THE ONE THAT MAKES ROW (b) NON-VACUOUS ────────────────────
	// The PREVIOUS implementation of this resolve was `Cast<AHeroCharacter>(GetPawn())`.
	// Prove that it would have returned NULL for a possessed ghost — otherwise row (b)
	// could pass against the old code and would be asserting nothing at all.
	if (Cast<AHeroCharacter>(static_cast<APawn*>(GhostDefaults)) != nullptr)
	{
		AddError(TEXT("SELF-CHECK FAILED: ASiegeGhostPawn casts to AHeroCharacter — the ghost has been made a hero subclass, which collapses the entire respawn resolve. Row (b) below would pass vacuously."));
		return false;
	}

	// ── (a) THE NON-GHOST PATH IS BYTE-IDENTICAL TO THE SHIPPED RESOLVE ─────────────────
	TestTrue(TEXT("(a) a possessed HERO resolves to itself — every pre-TASK-750 path unchanged"),
		ASiegeGameMode::ResolveHeroToRestore(HeroDefaults, nullptr) == HeroDefaults);

	// ── (b) ⭐⭐ THE GHOST PATH RETURNS THE TRACKED HERO — the double-apply guard ────────
	// The naive resolve returns null here (proven above), which sends the respawn into
	// RestoreHeroAtStart's RestartPlayer branch: a SECOND hero is spawned, the first is
	// orphaned still carrying every upgrade stack, and ResetHero()'s cumulative re-apply
	// lands on the corpse. This row is what keeps ONE hero actor alive across the whole
	// death, so ResetHero() runs exactly once on exactly one pawn.
	TestTrue(TEXT("(b) ⭐⭐ with the GHOST possessed the resolve returns the SAME tracked hero — one actor across the death, so ResetHero runs exactly once"),
		ASiegeGameMode::ResolveHeroToRestore(GhostDefaults, HeroDefaults) == HeroDefaults);

	// ── (c) ⛔ IT CAN NEVER HAND BACK THE GHOST ──────────────────────────────────────────
	// A ghost with no tracked hero is a genuine failure, and the honest answer is null —
	// which is exactly the signal the shipped restore already handles. Returning the
	// ghost would hand the player a pawn ResetHero() cannot be called on.
	TestNull(TEXT("(c) ⛔ a possessed ghost with NO tracked hero resolves to null — never the ghost"),
		ASiegeGameMode::ResolveHeroToRestore(GhostDefaults, nullptr));

	// ── (d) THE FULLY DEFENSIVE ROW ─────────────────────────────────────────────────────
	TestNull(TEXT("(d) no pawn and no tracked hero ⇒ null (the shipped fresh-pawn fallback)"),
		ASiegeGameMode::ResolveHeroToRestore(nullptr, nullptr));

	// ── SELF-CHECK: THE RESOLVE IS NOT A CONSTANT ───────────────────────────────────────
	TestTrue(TEXT("SELF-CHECK: the resolve produced both a non-null and a null answer — it is not a constant"),
		ASiegeGameMode::ResolveHeroToRestore(HeroDefaults, nullptr) != nullptr
		&& ASiegeGameMode::ResolveHeroToRestore(nullptr, nullptr) == nullptr);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  5. ⛔ THE GHOST MAY NOT PLAY CARDS — and the gate exists because the ghost
//     silently UNDID a shipped refusal (GHOST-§3 G-3/G-5)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostCannotPlayCardsTest,
	"Siegebound.RespawnLifecycle.TheGhostCannotPlayCardsAndEveryOtherPawnIsUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostCannotPlayCardsTest::RunTest(const FString& Parameters)
{
	const AHeroCharacter* const HeroDefaults = GetDefault<AHeroCharacter>();
	const ASiegeGhostPawn* const GhostDefaults = GetDefault<ASiegeGhostPawn>();

	if (!HeroDefaults || !GhostDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: a class default object is null."));
		return false;
	}

	// ── (a) ⛔ THE BAN (G-5's proceeding default) ────────────────────────────────────────
	const bool bGhostMayPlay = ASiegePlayerController::CanPlayCardsWhilePossessing(GhostDefaults);
	TestFalse(TEXT("(a) ⛔ the ghost may NOT play cards — G-5's default; his enumeration is closed and card play is not in it"),
		bGhostMayPlay);

	// ── (b) EVERY OTHER PAWN IS UNTOUCHED ───────────────────────────────────────────────
	// ⚠️ This is the half that keeps the gate from being a behaviour change: a live hero
	// still plays cards, and a controller with no pawn behaves exactly as it shipped.
	const bool bHeroMayPlay = ASiegePlayerController::CanPlayCardsWhilePossessing(HeroDefaults);
	TestTrue(TEXT("(b) a HERO pawn may play cards — the gate changes nothing for a living player"),
		bHeroMayPlay);

	TestTrue(TEXT("(b) …and NO pawn at all is byte-identical to the shipped behaviour (the gate never invents a refusal)"),
		ASiegePlayerController::CanPlayCardsWhilePossessing(nullptr));

	// ── SELF-CHECK: THE GATE IS NOT A CONSTANT ──────────────────────────────────────────
	TestTrue(TEXT("SELF-CHECK: the gate answered differently for the ghost and for the hero — it is not a constant"),
		bHeroMayPlay && !bGhostMayPlay);

	// ── (c) ⛔ THE GATE BANS **THE GHOST**, NOT "ANYTHING THAT IS NOT A HERO" ────────────
	// ⚠️ THIS IS THE ROW THAT KILLS THE MOST LIKELY WRONG IMPLEMENTATION. Written as
	// `return PossessedPawn->IsA<AHeroCharacter>()` the gate would agree with (a) and
	// with the null half of (b) and would still be WRONG: it silently bans every future
	// non-hero possession, and it states the rule backwards — the ban is on the death
	// state, not on "not being the hero". A plain APawn must be allowed.
	const APawn* const PlainPawnDefaults = GetDefault<APawn>();
	if (!PlainPawnDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<APawn>() is null — row (c) cannot be evaluated."));
		return false;
	}
	TestTrue(TEXT("(c) ⛔ a plain APawn is NOT banned — the gate discriminates on the GHOST, not on \"not a hero\" (this row fails an IsA<AHeroCharacter> implementation)"),
		ASiegePlayerController::CanPlayCardsWhilePossessing(PlainPawnDefaults));

	// ── (d) ⚖️ AND IT IS **ONE LINE** FOR JONATHAN TO OVERRULE ───────────────────────────
	// G-5 is the flagged row. That the verdict is reachable as a STATIC — called here
	// with no controller in existence — is the proof that it consults nothing but the
	// pawn: there is no latched death state anywhere for a G-5 flip to leave stale, so
	// allowing card play is a single `return true` with no other edit.
	// (The four rows above are that static being exercised with no instance at all.)

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  6. ⭐ THE GHOST KEEPS `G-3`'s POWERS BECAUSE THEY ARE BOUND ON THE **CONTROLLER**
//     — the possession swap cannot take them away (GHOST-§3 G-3)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostKeepsItsPowersTest,
	"Siegebound.RespawnLifecycle.OrdersCommanderAndMapAreBoundOnTheControllerNotThePawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostKeepsItsPowersTest::RunTest(const FString& Parameters)
{
	const UClass* const ControllerClass = ASiegePlayerController::StaticClass();
	const UClass* const HeroClass = AHeroCharacter::StaticClass();

	if (!ControllerClass || !HeroClass)
	{
		AddError(TEXT("SELF-CHECK FAILED: a UClass is null."));
		return false;
	}

	// ⭐ WHY THIS IS THE RIGHT ASSERTION FOR "the ghost can still command units, reach the
	// AI commander and open the map": those powers are input ACTIONS, and an action
	// survives a possession swap if and only if it is bound on the object that does not
	// change — the PLAYER CONTROLLER. If any of these ever moved onto AHeroCharacter,
	// possessing the ghost would silently strip that power, the compile would stay clean,
	// and Jonathan's enumerated ghost would quietly stop working.
	const TCHAR* const ControllerBoundPowers[] =
	{
		TEXT("CmdAttackAction"),   // unit orders — "it can command units"
		TEXT("CmdHoldAction"),
		TEXT("CmdDefendAction"),
		TEXT("CmdAmbushAction"),
		TEXT("CmdFollowAction"),
		TEXT("AssistantConsoleAction"), // "access the AI commander"
		TEXT("WarMapAction"),           // "look at the map"
	};

	for (const TCHAR* const PowerName : ControllerBoundPowers)
	{
		TestNotNull(*FString::Printf(TEXT("⭐ '%s' is declared on ASiegePlayerController — it survives the possession swap"), PowerName),
			ControllerClass->FindPropertyByName(FName(PowerName)));
	}

	// ── SELF-CHECK: the probe can also MISS, and it misses on the right class ────────────
	// SprintAction is a HERO ability bound on the hero pawn. It must NOT be on the
	// controller — and it must be findable on the hero, which proves the probe is live on
	// both classes rather than answering "yes" to everything or "no" to everything.
	TestNull(TEXT("SELF-CHECK: SprintAction is NOT on the controller (the probe can miss)"),
		ControllerClass->FindPropertyByName(TEXT("SprintAction")));
	TestNotNull(TEXT("SELF-CHECK: SprintAction IS on AHeroCharacter (the probe can hit on the other class)"),
		HeroClass->FindPropertyByName(TEXT("SprintAction")));

	// ⭐ AND THE CONVERSE, WHICH IS "it just cannot attack": the hero's offensive verbs are
	// bound on the HERO PAWN, so while the ghost is possessed there is no attack binding
	// in existence to press. ⛔ No suppression code implements this and none may.
	TestNotNull(TEXT("⭐ AttackAction is declared on AHeroCharacter — \"cannot attack\" is the ABSENCE of a binding on the ghost, not a suppression flag"),
		HeroClass->FindPropertyByName(TEXT("AttackAction")));
	TestNull(TEXT("…and it is NOT on the controller, so it cannot follow the player onto the ghost"),
		ControllerClass->FindPropertyByName(TEXT("AttackAction")));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  7. ⛔ DEATH MID-RECALL LEAVES NOTHING DANGLING **HERE** — the channel lives
//     entirely on the hero, and the ghost cannot carry one (GHOST-§3 G-6)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeathMidRecallLeavesNothingDanglingTest,
	"Siegebound.RespawnLifecycle.NoRecallStateLivesOnTheGameModeControllerOrGhost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeathMidRecallLeavesNothingDanglingTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRespawnLifecycleTestFixture;

	// ⚠️ WHAT THIS PROVES AND WHAT IT DOES NOT. The ABORT itself is TASK-748's and it
	// ships on the hero: HandleDeath() calls EndRecall(InterruptedByDeath) — RECALL-§4's
	// exit 5, G-6 — before it broadcasts OnHeroDied, so the channel is already cleared by
	// the time this task's death path runs at all. ⛔ That is asserted in the recall
	// suite, not here, and re-asserting it here would be a second source of truth.
	//
	// ⭐ WHAT THIS FILE OWES IS THE OTHER HALF: that the death → ghost → respawn lifecycle
	// contributes NOTHING to the channel that could outlive it. A dangling recall would
	// have to be a piece of recall state held by one of these three classes.

	const UClass* const ModeClass = ASiegeGameMode::StaticClass();
	const UClass* const ControllerClass = ASiegePlayerController::StaticClass();
	const UClass* const GhostClass = ASiegeGhostPawn::StaticClass();

	if (!ModeClass || !ControllerClass || !GhostClass)
	{
		AddError(TEXT("SELF-CHECK FAILED: a UClass is null."));
		return false;
	}

	// ── SELF-CHECK FIRST: THE PROPERTY SCANNER FINDS WHAT IT SHOULD ─────────────────────
	// "Respawn" is certainly a declared PROPERTY on the game mode (HeroRespawnDelay). If
	// this misses, every "no recall state here" below is worthless.
	if (!DeclaresPropertyContaining(ModeClass, TEXT("Respawn")))
	{
		AddError(TEXT("SELF-CHECK FAILED: the property scan did not find 'Respawn' on ASiegeGameMode — the scanner is blind and its misses below mean nothing."));
		return false;
	}

	// ── (a) ⛔ NO RECALL STATE ON THE LIFECYCLE OWNER ────────────────────────────────────
	// The game mode owns exactly one timer set (the per-controller respawn map) and one
	// ghost map. It holds no channel, no channel handle and no channel clock — so a hero
	// that dies mid-channel cannot leave anything behind in the class that owns the 180 s.
	// ⚠️ PROPERTIES ONLY, AND THE DISTINCTION IS REAL RATHER THAN CONVENIENT: this class
	// deliberately DOES declare a recall HANDLER (see (d)). It owns the destination, not
	// the channel — and a handler holds nothing that can dangle.
	TestFalse(TEXT("(a) ⛔ ASiegeGameMode declares NO recall STATE — the channel is not this class's to leak"),
		DeclaresPropertyContaining(ModeClass, TEXT("Recall")));

	TestFalse(TEXT("(a) ⛔ ASiegePlayerController declares no recall state and no recall handler at all"),
		DeclaresMemberContaining(ControllerClass, TEXT("Recall")));

	// ── (b) ⭐ AND THE GHOST STRUCTURALLY CANNOT CONTINUE A CHANNEL ──────────────────────
	// It is not an AHeroCharacter, so it has no RecallState to tick, no recall binding to
	// press and no EndRecall to reach. The channel cannot follow the player across the
	// possession swap because the thing that holds it did not come along.
	TestFalse(TEXT("(b) ⛔ ASiegeGhostPawn declares nothing recall-related"),
		DeclaresMemberContaining(GhostClass, TEXT("Recall")));

	TestFalse(TEXT("(b) ⭐ …and it is not an AHeroCharacter, so there is no channel for it to inherit"),
		GhostClass->IsChildOf(AHeroCharacter::StaticClass()));

	// ── (c) THE CHANNEL REALLY DOES LIVE ON THE HERO ────────────────────────────────────
	// The claim in (a)/(b) is "not here", and "not here" only means something if there is
	// a "there". This pins it: the recall state is AHeroCharacter's, which is why death
	// clears it in HandleDeath (RECALL-§4 exit 5 / G-6) and why nothing on this task's
	// path has to — or may.
	TestTrue(TEXT("(c) ⭐ AHeroCharacter DOES declare recall state — 'not here' is a real statement about a real thing that lives somewhere else"),
		DeclaresPropertyContaining(AHeroCharacter::StaticClass(), TEXT("Recall")));

	// ── (d) ⭐ THE ONE RECALL THING THAT **IS** THIS CLASS'S: THE DESTINATION ────────────
	// TASK-748's unbound-destination warning names TASK-750 as the binder, and without a
	// binder a COMPLETED 10 s channel moves and heals nothing. The handler is here — and
	// being a handler and not a property is exactly why (a) can be true at the same time.
	TestNotNull(TEXT("(d) ⭐ ASiegeGameMode declares HandleHeroRecallArrived — it owns the DESTINATION (the same resolver the respawn uses), never the channel"),
		ModeClass->FindFunctionByName(TEXT("HandleHeroRecallArrived")));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  8. ⛔ THE GHOST CLASS REFERENCE IS TYPED, AND IT POINTS AT THE AUTHORED BLUEPRINT
//     (GHOST-§5 — the TYPE is the safety; the `_C` suffix is what makes it SPAWN)
//
//     ⚠️ THIS SECTION USED TO ASSERT "IT SHIPS UNSET ON PURPOSE" AND THAT IS NOW
//     FALSE BY CONSTRUCTION: TASK-763 authored BP_SiegeGhostPawn and TASK-764
//     pointed the CDO at it (SiegeGameMode.cpp:67), so the deliberate blank is
//     SPENT, not forgotten. Rewritten rather than left lying — the TASK-517 /
//     HIGH-§1 idiom.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostPawnClassAssetTest,
	"Siegebound.RespawnLifecycle.GhostPawnClassAssetIsTypedToTheGhostAndPointsAtTheGeneratedBlueprintClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostPawnClassAssetTest::RunTest(const FString& Parameters)
{
	const UClass* const ModeClass = ASiegeGameMode::StaticClass();
	const ASiegeGameMode* const ModeDefaults = GetDefault<ASiegeGameMode>();

	if (!ModeClass || !ModeDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<ASiegeGameMode>() or its UClass is null."));
		return false;
	}

	const FSoftClassProperty* const GhostClassProperty =
		CastField<FSoftClassProperty>(ModeClass->FindPropertyByName(TEXT("GhostPawnClassAsset")));

	if (!GhostClassProperty)
	{
		AddError(TEXT("GhostPawnClassAsset is not a declared TSoftClassPtr UPROPERTY on ASiegeGameMode — GHOST-§5 names it and its type is the safety."));
		return false;
	}

	// ── (a) ⭐ THE TYPE IS THE SAFETY ────────────────────────────────────────────────────
	// TSoftClassPtr<ASiegeGhostPawn> makes LoadSynchronous return null for anything that
	// is not a ghost, so a mis-pointed designer reference degrades to the C++ fallback
	// instead of spawning an arbitrary pawn — or a HERO — as the ghost.
	TestTrue(TEXT("(a) ⭐ GhostPawnClassAsset is typed to ASiegeGhostPawn — a wrong asset can only fail to load, never spawn the wrong pawn"),
		GhostClassProperty->MetaClass == ASiegeGhostPawn::StaticClass());

	// ── (b) ⭐ IT POINTS AT THE BLUEPRINT'S **GENERATED CLASS**, PATH-EXACT ──────────────
	// ⛔ THIS IS DELIBERATELY AN EQUALITY ON THE WHOLE PATH AND ⛔ NOT AN INVERTED
	// `!IsNull()`, because "empty" is not the failure mode this property actually has.
	// Its failure mode is NON-EMPTY AND SUBTLY WRONG: drop the `_C` suffix and the path
	// resolves to the Blueprint ASSET instead of its generated class — non-null, looks
	// authored in the editor and in any log line that prints it, and LoadSynchronous
	// returns NULL for it through a TSoftClassPtr. The resolver then takes its
	// authored-but-unresolvable branch, warns once, and falls back — so the player spends
	// the full 180 s never seeing the blueprint ghost, with nothing red anywhere.
	// ⇒ an inverted null check PASSES on exactly that defect. This assertion is the one
	// that catches it, so it is pinned to the literal path and not to non-emptiness.
	//
	// (Case-insensitive TestEqual is the correct comparator, not a looseness: package and
	// asset names are FNames, so the engine itself resolves them case-insensitively. The
	// `_C` difference is a token, not a case, and is caught either way.)
	const FSoftObjectPtr* const GhostValue = GhostClassProperty->ContainerPtrToValuePtr<FSoftObjectPtr>(ModeDefaults);
	if (!GhostValue)
	{
		AddError(TEXT("SELF-CHECK FAILED: could not read GhostPawnClassAsset off the CDO."));
		return false;
	}

	const FString ExpectedGhostPath(TEXT("/Game/Blueprints/BP_SiegeGhostPawn.BP_SiegeGhostPawn_C"));

	TestEqual(TEXT("(b) ⭐ GhostPawnClassAsset points at BP_SiegeGhostPawn's GENERATED CLASS, `_C` exact — a path that loses `_C` is non-null, reads as authored, and silently never spawns"),
		GhostValue->ToString(), ExpectedGhostPath);

	// ── SELF-CHECK: THE READER RESOLVES **TWO DISTINCT PROPERTIES**, BY PATH AND BY TYPE ─
	// ⚠️⚠️ THE OLD DISCRIMINATOR HERE WAS "Hero is SET, Ghost is UNSET", AND TASK-764
	// KILLED IT: the ghost is authored now, so BOTH are set and that contrast is dead.
	// A dead contrast does not go red — it PASSES while testing nothing (the SHIP-§9 /
	// SC-§37 shape this project has now hit four times), so it is RE-ARMED rather than
	// deleted, on the two axes that genuinely still differ: the PATH and the TYPE.
	//
	// ⭐ AND THE RE-ARM IS NOT CEREMONIAL, BECAUSE 764 MADE THE TWO PROPERTIES NEARLY
	// IDENTICAL IN SHAPE: both are TSoftClassPtr on the same CDO, both authored to
	// /Game/Blueprints/BP_*_C. That is precisely the condition under which a reader that
	// silently resolved BOTH names onto ONE storage slot would go unnoticed — and such a
	// reader would make (b) above pass by reading the wrong field. Pinning the hero to
	// its OWN exact path, and asserting the two reads DIFFER, is what rules that out.
	const FSoftClassProperty* const HeroClassProperty =
		CastField<FSoftClassProperty>(ModeClass->FindPropertyByName(TEXT("HeroPawnClassAsset")));
	if (!HeroClassProperty)
	{
		AddError(TEXT("SELF-CHECK FAILED: HeroPawnClassAsset did not resolve — the soft-class reader cannot be validated."));
		return false;
	}

	const FSoftObjectPtr* const HeroValue = HeroClassProperty->ContainerPtrToValuePtr<FSoftObjectPtr>(ModeDefaults);
	if (!HeroValue)
	{
		AddError(TEXT("SELF-CHECK FAILED: could not read HeroPawnClassAsset off the CDO — the soft-class reader cannot be validated."));
		return false;
	}

	TestEqual(TEXT("SELF-CHECK: the SAME reader returns a DIFFERENT, equally exact path for HeroPawnClassAsset — (b) read the ghost's OWN storage, not one canned value"),
		HeroValue->ToString(), FString(TEXT("/Game/Blueprints/BP_HeroCharacter.BP_HeroCharacter_C")));

	TestNotEqual(TEXT("SELF-CHECK: …and the two paths are genuinely different strings — these are two properties, not one property read twice"),
		GhostValue->ToString(), HeroValue->ToString());

	TestTrue(TEXT("SELF-CHECK: …and the reader still discriminates on TYPE — Hero's MetaClass is AHeroCharacter, and it is NOT the ghost's MetaClass"),
		HeroClassProperty->MetaClass == AHeroCharacter::StaticClass()
		&& HeroClassProperty->MetaClass != GhostClassProperty->MetaClass);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  9. ⚠️⚠️ THE INPUT-CONTEXT HAND-OFF, IN BOTH DIRECTIONS — the mapping context is
//     owned by the PAWN, so each pawn must carry its own (GHOST-§4)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostInputContextOwnershipTest,
	"Siegebound.RespawnLifecycle.EachPawnOwnsItsOwnMappingContextAndTheControllerOwnsNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostInputContextOwnershipTest::RunTest(const FString& Parameters)
{
	using namespace SiegeRespawnLifecycleTestFixture;

	// ⚠️⚠️ THE TRAP THIS TEST EXISTS FOR, AND IT IS THE MOST DANGEROUS ONE IN THE BATCH:
	// the Enhanced Input mapping context is added by the PAWN, ⛔ NOT by
	// ASiegePlayerController (whose own comment says so). ⇒ every possession swap in this
	// lifecycle depends on the INCOMING pawn re-adding it. TASK-749 found the outbound
	// half (ghost → the player would control nothing for 180 s); this asserts the
	// PRECONDITION that makes BOTH directions work, so the day someone "centralises" the
	// context onto the controller — which looks like an obvious tidy-up — the suite goes
	// red and names the reason instead of shipping an input-dead arena.

	const UClass* const ControllerClass = ASiegePlayerController::StaticClass();
	const UClass* const HeroClass = AHeroCharacter::StaticClass();
	const UClass* const GhostClass = ASiegeGhostPawn::StaticClass();

	if (!ControllerClass || !HeroClass || !GhostClass)
	{
		AddError(TEXT("SELF-CHECK FAILED: a UClass is null."));
		return false;
	}

	// ── (a) EACH PAWN CARRIES ITS OWN CONTEXT ───────────────────────────────────────────
	// The hero's re-add is what makes the RESPAWN hand-off safe (APawn::
	// NotifyControllerChanged fires on possession, not only on unpossess); the ghost's is
	// what makes the DEATH hand-off safe. Both must exist for the cycle to close.
	TestNotNull(TEXT("(a) AHeroCharacter declares its own mapping context — this is what re-arms input on the RESPAWN hand-off"),
		HeroClass->FindPropertyByName(TEXT("HeroMappingContext")));

	TestNotNull(TEXT("(a) ASiegeGhostPawn declares its own mapping context — this is what re-arms input on the DEATH hand-off (TASK-749)"),
		GhostClass->FindPropertyByName(TEXT("GhostMappingContext")));

	// ── (b) ⛔ AND THE CONTROLLER OWNS NONE ──────────────────────────────────────────────
	// If a context were ever added here instead, it would be a SECOND owner of a
	// composition that already has exactly one per pawn — and the two would fight across
	// every possession swap.
	TestFalse(TEXT("(b) ⛔ ASiegePlayerController declares NO mapping context — pawn-side ownership is the whole mechanism, not an accident"),
		DeclaresMemberContaining(ControllerClass, TEXT("MappingContext")));

	// ── SELF-CHECK: the "declares no X" scan is alive on THIS class ──────────────────────
	// Without it, (b) would pass against a scanner that always answered false.
	TestTrue(TEXT("SELF-CHECK: the same scan FINDS a genuinely declared member on the controller (WarMapAction) — its miss in (b) is meaningful"),
		DeclaresMemberContaining(ControllerClass, TEXT("WarMapAction")));

	// ── (c) ⛔ THE LIFECYCLE OWNER ADDS AND REMOVES NOTHING ──────────────────────────────
	// ASiegeGameMode must stay entirely out of the input composition: it possesses, and
	// the pawns arm themselves.
	TestFalse(TEXT("(c) ⛔ ASiegeGameMode declares no mapping context — it possesses; the pawns arm themselves"),
		DeclaresMemberContaining(ASiegeGameMode::StaticClass(), TEXT("MappingContext")));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TASK-1102 (DEATH-CAM-ROLL) — THE TWO CAMERA-ROLL CLAIMS
//
//  ⛔ THE MEASURED DEFECT: handoffs/TASK-1094-buildmaster.md §5.6 + capture
//  playtest-evidence/2026-09-06/TASK-1094-E-OBSERVATION-death-camera-roll-90deg.png — the host
//  walked the hero into the RED army at 500 uu/s, died TWICE, and both times the player camera
//  ended up at `control rotation roll 89.9` with the whole frame on its side, and it ⛔ STAYED
//  rolled through a forced `AHeroCharacter::ResetHero()`.
//
//  ⛔ THE WRITE SITE: `SiegeGameMode.cpp:862` `PC->SetControlRotation(GhostRotation)`, fed at
//  `:833` by `DeadHero->GetActorRotation()` — the only site in the module that can transport a
//  non-zero roll into the control rotation on the death path.
//
//  ⚠️⚠️ WHY THESE TWO TESTS ARE SHAPED THE WAY THEY ARE (`SC-§79` + `SC-§36.1`): each has TWO
//  halves that fail for DIFFERENT reasons.
//    • the BEHAVIOUR half CALLS the shipped rule with the measured `roll 89.9` — so a rule that
//      stops removing roll goes red, and (the other direction) a rule that lazily returns
//      `FRotator::ZeroRotator` ALSO goes red, because pitch/yaw are asserted to survive EXACTLY;
//    • the CALL-SITE CENSUS half reads the SHIPPED source and requires the rule to actually be
//      CALLED — because this project has already shipped a built-and-tested trigger with ⛔ ZERO
//      callers and nothing failed. A green behaviour half over a dead function is ⛔ not evidence.
// ═══════════════════════════════════════════════════════════════════════════════════════════════

namespace SiegeDeathCameraRollTestFixture
{
	/** The roll the host actually measured on the rolled frame. ⛔ Not a round 90: the test uses the OBSERVED number so nobody can claim the case was invented. */
	constexpr double MeasuredDeathRollDegrees = 89.9;

	/** A pitch and a yaw that are BOTH non-zero and BOTH unlike each other — so a function that swapped or zeroed either one cannot pass. */
	constexpr double ProbePitchDegrees = -17.5;
	constexpr double ProbeYawDegrees = 133.25;

	/** Exact-equality comparison. Tolerance 0 is the claim: "preserved EXACTLY" means bit-for-bit, not "close enough". */
	static bool ExactlyEquals(double A, double B)
	{
		return FMath::IsNearlyEqual(A, B, 0.0);
	}

	/** Asserts all three components at once with a failure message that PRINTS the numbers — a red here must say what the rule returned, not merely that it was wrong. */
	static void CheckRotator(FAutomationTestBase& Test, const TCHAR* What, const FRotator& Actual,
		double ExpectedPitch, double ExpectedYaw, double ExpectedRoll)
	{
		Test.TestTrue(
			FString::Printf(TEXT("%s — got (pitch %.4f, yaw %.4f, roll %.4f), expected (pitch %.4f, yaw %.4f, roll %.4f)"),
				What, Actual.Pitch, Actual.Yaw, Actual.Roll, ExpectedPitch, ExpectedYaw, ExpectedRoll),
			ExactlyEquals(Actual.Pitch, ExpectedPitch)
			&& ExactlyEquals(Actual.Yaw, ExpectedYaw)
			&& ExactlyEquals(Actual.Roll, ExpectedRoll));
	}

	/** Loads a shipped source file. ⛔ A probe that cannot read its subject FAILS — it never reports SAFE. (Same shape as SiegeHeroLadderClimbTest.cpp:107.) */
	static bool LoadProjectSource(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutText)
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / FString(RelativePath));
		if (!FPaths::FileExists(FullPath))
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not find '%s'. The CALL-SITE CENSUS cannot be run, so it FAILS rather than passing quietly."), *FullPath));
			return false;
		}
		if (!FFileHelper::LoadFileToString(OutText, *FullPath))
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not read '%s' — a stale probe fails."), *FullPath));
			return false;
		}
		return true;
	}

	/**
	 *  Extracts ONE function body: signature → the first line closing at column 0 (`\n}`), which is
	 *  how every function in this codebase ends. ⛔ Deliberately not a brace-matcher (it would walk
	 *  into comments and string literals and fail for reasons unrelated to the claim).
	 *  ⭐ Carries its own positive control: a renamed/deleted function is an ERROR naming it, and a
	 *  body too small to contain anything is an ERROR — so a census can never pass by finding nothing.
	 *  (Same shape as SiegeHeroLadderClimbTest.cpp:143.)
	 */
	static bool ExtractFunctionBody(FAutomationTestBase& Test, const FString& Source, const TCHAR* Signature, FString& OutBody)
	{
		const int32 SignatureIndex = Source.Find(Signature, ESearchCase::CaseSensitive);
		if (SignatureIndex == INDEX_NONE)
		{
			Test.AddError(FString::Printf(TEXT("⛔ '%s' is not in the shipped source — the site this census names either moved or was deleted, and ⛔ neither is a pass."), Signature));
			return false;
		}
		const int32 BodyEnd = Source.Find(TEXT("\n}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SignatureIndex);
		if (BodyEnd == INDEX_NONE)
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not find the end of '%s' — the probe is stale, so it fails."), Signature));
			return false;
		}
		OutBody = Source.Mid(SignatureIndex, BodyEnd - SignatureIndex);
		if (OutBody.Len() < 40)
		{
			Test.AddError(FString::Printf(TEXT("⛔ '%s' extracted only %d characters — the probe is broken, ⛔ not the subject."), Signature, OutBody.Len()));
			return false;
		}
		return true;
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  10. ⭐⭐ HALF (a) — THE DEATH PATH HANDS THE CONTROLLER A YAW-ONLY ROTATION,
//      SO A ROLLED CORPSE CAN NEVER ROLL THE CAMERA (TASK-1102, SiegeGameMode.cpp:833/862)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeDeathPathNeverTransportsRollTest,
	"Siegebound.RespawnLifecycle.TheDeathPathHandsTheControllerAYawOnlyRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeDeathPathNeverTransportsRollTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeathCameraRollTestFixture;

	// ── (a1) BEHAVIOUR: the measured case ───────────────────────────────────────────────
	// The exact rotation the defect produces — a corpse carrying the observed roll — must come
	// back as facing-only. ⛔ Two sides that are NOT equal by construction: the input roll is
	// 89.9 and the expected output roll is 0.
	CheckRotator(*this,
		TEXT("(a1) a corpse rolled by the MEASURED 89.9° yields a yaw-only view rotation"),
		FSiegeDeathCameraStatics::MakeDeathViewRotation(FRotator(ProbePitchDegrees, ProbeYawDegrees, MeasuredDeathRollDegrees)),
		0.0, ProbeYawDegrees, 0.0);

	// ── (a2) BEHAVIOUR: the YAW IS THE PAYLOAD, and it survives ─────────────────────────
	// ⛔ This is the assertion that kills a lazy `return FRotator::ZeroRotator;` — the shipped
	// intent at SiegeGameMode.cpp:828 is "facing the way the hero was facing", so losing the yaw
	// would be a DIFFERENT defect with the same green suite.
	TestTrue(TEXT("(a2) ⛔ the yaw is CARRIED THROUGH, not zeroed — 'facing the way the hero was facing' is a claim about yaw"),
		ExactlyEquals(FSiegeDeathCameraStatics::MakeDeathViewRotation(FRotator(0.0, ProbeYawDegrees, 0.0)).Yaw, ProbeYawDegrees));

	// ── (a3) BEHAVIOUR: the negative roll, and the upright no-op ────────────────────────
	// Roll is removed whichever way it leans, and an already-upright corpse is returned unchanged
	// — i.e. this is ⛔ NOT a behaviour change for the case that was already correct.
	CheckRotator(*this, TEXT("(a3) a NEGATIVE roll is removed too"),
		FSiegeDeathCameraStatics::MakeDeathViewRotation(FRotator(0.0, ProbeYawDegrees, -MeasuredDeathRollDegrees)),
		0.0, ProbeYawDegrees, 0.0);

	CheckRotator(*this, TEXT("(a3) an UPRIGHT corpse is returned unchanged — the fix is a no-op on the healthy case"),
		FSiegeDeathCameraStatics::MakeDeathViewRotation(FRotator(0.0, ProbeYawDegrees, 0.0)),
		0.0, ProbeYawDegrees, 0.0);

	// ── (a4) ⛔ THE CALL-SITE CENSUS (SC-§36.1) ──────────────────────────────────────────
	// A rule nobody calls is a landed FILE, not a landed FIX. This half fails if the write site
	// stops routing through it, even while every assertion above stays green.
	FString GameModeSource;
	if (!LoadProjectSource(*this, TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp"), GameModeSource))
	{
		return false;
	}

	FString GhostBody;
	if (!ExtractFunctionBody(*this, GameModeSource,
		TEXT("void ASiegeGameMode::SpawnAndPossessGhost(AController* Player, AHeroCharacter* DeadHero)"), GhostBody))
	{
		return false;
	}

	// POSITIVE CONTROL FIRST: the extractor really has the death-ghost body in hand. Without this,
	// every "contains" below could be vacuously false against an empty or wrong extraction.
	// ⚠️ Every token this census matches is written WITH ITS TRAILING SEMICOLON or with an argument
	// list no comment repeats, ⛔ deliberately: a census that a PROSE MENTION could satisfy is a
	// census that passes over deleted code. That is the same class of blindness `SC-§36.1` records.
	TestTrue(TEXT("SELF-CHECK: the extracted body really is SpawnAndPossessGhost — it contains the possession statement itself"),
		GhostBody.Contains(TEXT("PC->Possess(Ghost);"), ESearchCase::CaseSensitive));

	TestTrue(TEXT("(a4) ⛔ GhostRotation is PRODUCED BY the rule — the dead pawn's rotation reaches nothing until it has been through MakeDeathViewRotation"),
		GhostBody.Contains(TEXT("const FRotator GhostRotation = FSiegeDeathCameraStatics::MakeDeathViewRotation(DeadHero->GetActorRotation());"), ESearchCase::CaseSensitive));

	// ⛔ AND THE SANITISED VALUE IS WHAT IS ACTUALLY HANDED OVER — both to the ghost's SPAWN
	// (which `AController::OnPossess` then copies into the control rotation on its own) and to the
	// explicit control-rotation write at :862. Naming both is the point: clamping only the second
	// would leave the first still carrying the roll.
	TestTrue(TEXT("(a4) the sanitised rotation is the GHOST'S SPAWN rotation — AController::OnPossess copies the pawn's rotation into the control rotation by itself, ahead of any explicit write"),
		GhostBody.Contains(TEXT("GhostLocation, GhostRotation, SpawnParams);"), ESearchCase::CaseSensitive));

	TestTrue(TEXT("(a4) the sanitised rotation is what reaches the WRITE SITE — the explicit control-rotation write named by TASK-1102"),
		GhostBody.Contains(TEXT("PC->SetControlRotation(GhostRotation);"), ESearchCase::CaseSensitive));

	// ── (a5) ⛔ NEGATIVE CONTROL ON THE OTHER TWO CONTROL-ROTATION WRITES ────────────────
	// There are exactly three SetControlRotation sites in the module; the other two are fed by
	// GetHeroStartTransform, which is yaw-only at every return. If a fourth ever appears, the count
	// changes and this fires — a new death-adjacent write is exactly how this defect got in.
	int32 WriteSiteCount = 0;
	int32 SearchFrom = 0;
	for (;;)
	{
		const int32 Found = GameModeSource.Find(TEXT("SetControlRotation("), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
		if (Found == INDEX_NONE)
		{
			break;
		}
		++WriteSiteCount;
		SearchFrom = Found + 1;
	}

	TestEqual(TEXT("(a5) ⛔ ASiegeGameMode still has EXACTLY THREE control-rotation writes (recall :514, death-ghost :862, respawn :1051) — a fourth is a new roll transport and must be reviewed, ⛔ not absorbed"),
		WriteSiteCount, 3);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  11. 🚨⭐⭐ HALF (b) — `ResetHero()` RETURNS CONTROL-ROTATION ROLL TO 0.
//      ⛔ THE PLAYER-VISIBLE COMPLAINT IS "A RESET THAT DOES NOT RESET" (TASK-1102)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeResetHeroLevelsTheCameraRollTest,
	"Siegebound.RespawnLifecycle.ResetHeroReturnsControlRotationRollToZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeResetHeroLevelsTheCameraRollTest::RunTest(const FString& Parameters)
{
	using namespace SiegeDeathCameraRollTestFixture;

	// ── (b1) BEHAVIOUR: the measured case, both sides asserted ──────────────────────────
	// ⛔ ROLL GOES TO 0 **and** PITCH AND YAW SURVIVE EXACTLY. Those are two independent failures:
	// a rule that stops zeroing roll fails on the third component; a rule that returns
	// FRotator::ZeroRotator — the obvious lazy "fix" — fails on the first two. Neither can hide.
	CheckRotator(*this,
		TEXT("(b1) the MEASURED rolled camera (roll 89.9) is levelled, and the player's look direction is untouched"),
		FSiegeDeathCameraStatics::LevelViewRoll(FRotator(ProbePitchDegrees, ProbeYawDegrees, MeasuredDeathRollDegrees)),
		ProbePitchDegrees, ProbeYawDegrees, 0.0);

	// ── (b2) BEHAVIOUR: the other lean, and idempotence ─────────────────────────────────
	CheckRotator(*this, TEXT("(b2) a NEGATIVE roll is levelled too"),
		FSiegeDeathCameraStatics::LevelViewRoll(FRotator(ProbePitchDegrees, ProbeYawDegrees, -MeasuredDeathRollDegrees)),
		ProbePitchDegrees, ProbeYawDegrees, 0.0);

	CheckRotator(*this, TEXT("(b2) an ALREADY-LEVEL camera is returned unchanged — a respawn must not snap a player who was looking somewhere valid"),
		FSiegeDeathCameraStatics::LevelViewRoll(FRotator(ProbePitchDegrees, ProbeYawDegrees, 0.0)),
		ProbePitchDegrees, ProbeYawDegrees, 0.0);

	// ── (b3) ⛔ THE CALL-SITE CENSUS — THE HALF THE ROW'S GATE ASKS FOR BY NAME ──────────
	// TASK-1103 cl. 2: "is (b) — ResetHero() zeroes roll — ACTUALLY IN THE DIFF?" This is that
	// question, asked by the suite, of the shipped source, every run.
	FString HeroSource;
	if (!LoadProjectSource(*this, TEXT("Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp"), HeroSource))
	{
		return false;
	}

	FString ResetBody;
	if (!ExtractFunctionBody(*this, HeroSource, TEXT("void AHeroCharacter::ResetHero()"), ResetBody))
	{
		return false;
	}

	// POSITIVE CONTROL FIRST: the body really is ResetHero's. Two shipped tokens that must be there.
	// ⚠️ As in (a4), the tokens below are whole STATEMENTS. `ResetHero`'s own comment block names
	// both `LevelViewRoll` and `SetControlRotation` in prose, so a loose `Contains` here would stay
	// green with the code deleted and only the comment left — the exact failure this census exists
	// to catch. ⛔ Do not relax these to bare identifiers.
	TestTrue(TEXT("SELF-CHECK: the extracted body really is ResetHero — it contains the shipped input restore statement"),
		ResetBody.Contains(TEXT("EnableInput(Cast<APlayerController>(GetController()));"), ESearchCase::CaseSensitive));

	TestTrue(TEXT("(b3) 🚨 `ResetHero()` LEVELS THE CAMERA — it resolves a controller and writes back FSiegeDeathCameraStatics::LevelViewRoll of its current control rotation. ⛔ Without this the reset restores HP, visibility, collision, movement and input, and leaves the player looking sideways"),
		ResetBody.Contains(TEXT("FSiegeDeathCameraStatics::LevelViewRoll(OwningControllerForCamera->GetControlRotation())"), ESearchCase::CaseSensitive));

	TestTrue(TEXT("(b3) …and it WRITES the levelled value back through that controller — computing it and dropping it would be the SC-§36.1 zero-caller failure in miniature"),
		ResetBody.Contains(TEXT("OwningControllerForCamera->SetControlRotation("), ESearchCase::CaseSensitive));

	// ── (b4) ⛔ (b) IS NOT MERELY (a) IN DISGUISE ────────────────────────────────────────
	// The gate is told to reject "a row that fixes only (a) and describes (b) as no longer
	// reachable". So the two halves are asserted to be in DIFFERENT FILES: the reset's levelling
	// must not quietly migrate into the game mode's death path and leave the primitive bare.
	TestFalse(TEXT("(b4) ⛔ the reset's levelling lives in ResetHero itself, ⛔ not only in the game mode's respawn — the row forbids shipping (a) alone"),
		ResetBody.Contains(TEXT("MakeDeathViewRotation"), ESearchCase::CaseSensitive));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
