// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/UnrealString.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Siegebound/HealthBarProvider.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeGhostPawn.h"
#include "Siegebound/SiegeNavAreas.h"
#include "Siegebound/SiegeNetLimits.h"
#include "Siegebound/SummonedUnit.h"
#include "Siegebound/TeamId.h"
#include "Siegebound/Tower.h"
#include "UObject/Class.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE SIEGEBOUND GHOST PAWN (TASK-749; law `GHOST-§1`
 *      / `§3` / `§4` / `§6`, `TOWER-§9.2` inverted, `SHIP-§9c`) ═══
 *
 *  ⭐⭐ THE MOST IMPORTANT TEST IN THIS FILE ASSERTS SOMETHING THAT IS ⛔ NOT THERE.
 *  `ASiegeGhostPawn`'s entire "cannot be attacked" guarantee is the ABSENCE of
 *  `ITeamAgent`. There is no mechanism to exercise, no flag to read and no guard
 *  to drive — so the guarantee is asserted **at the type**, which is also the only
 *  place it can be defended. ⭐ The day someone adds the interface "for
 *  consistency", `FSiegeGhostPawnNotATeamAgentTest` goes red and NAMES THE REASON
 *  in its failure text. That is the whole point of writing it as a test rather
 *  than as a comment (`GHOST-§1`: *"a structural property cannot be forgotten at a
 *  guard point; a flag can"*).
 *
 *  ⚠️⚠️ THE LESSON THIS FILE IS WRITTEN AGAINST, AND IT HAS COST THIS PIPELINE QA
 *  LOOPS BEFORE: **AN ASSERTION THAT CANNOT FAIL PROVES NOTHING.** A test that
 *  says "this class does not implement X" passes just as happily when the probe
 *  is broken, when the interface UClass is wrong, or when the reflection table is
 *  empty. ⇒ **EVERY negative claim below ships with a SELF-CHECK that proves the
 *  instrument can still see a POSITIVE** — the same interface is looked up on
 *  classes that DO implement it, the reflection walk is validated against members
 *  it MUST find, and the banned-token scan is validated by proving it finds those
 *  same tokens on `AHeroCharacter`. If a self-check ever fires, the test beside it
 *  was about to pass vacuously.
 *
 *  MECHANISM — ⛔ zero world, ⛔ zero PIE, ⛔ zero asset loads, ⛔ zero writes.
 *  Class-default objects and the reflection tables only, the house pattern from
 *  `SiegeClimbableTowerTest.cpp`.
 *  ⚠️ **AMENDED BY TASK-758, STATED SO THE CLAIM ABOVE STAYS TRUE:** tests 9 and 10
 *  additionally construct **transient `NewObject` instances** of the pawn, because
 *  an ORDERING cannot be observed on a CDO — the CDO never runs an initialisation
 *  sequence. ⭐ This is still ⛔ zero world, ⛔ zero `SpawnActor`, ⛔ zero PIE and
 *  ⛔ zero asset loads (`GhostMaterial`/`GhostMesh` ship unset in C++, so no soft
 *  pointer is ever resolved), and the instances are transient and GC-rooted for the
 *  duration only — ⛔ the CDO is never mutated, so ⛔ no state leaks to another test.
 *
 *  ⚠️ WHAT THESE TESTS DELIBERATELY DO ⛔ NOT COVER, STATED SO THE GAP IS HONEST:
 *  the possession/cursor ordering, the 180 s timer, and the actual re-add of the
 *  mapping context are **runtime** behaviours owned by TASK-750, and `GHOST-§4`
 *  rules that their instrument is **PIE with a message-log read**, which may
 *  ⛔ never be waived on the grounds that the compile is clean. ⛔ No assertion
 *  here should be read as covering them.
 */

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 ⭐⭐ — THE HEADLINE. THE GHOST IS NOT AN ITeamAgent, AND THEREFORE IS
//  NOT RETURNED BY ANY ACQUISITION ENUMERATION IN THE PROJECT.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostPawnNotATeamAgentTest,
	"Siegebound.Ghost.DoesNotImplementITeamAgentAndIsThereforeUnacquirable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostPawnNotATeamAgentTest::RunTest(const FString& Parameters)
{
	UClass* const GhostClass = ASiegeGhostPawn::StaticClass();
	UClass* const TeamAgentInterface = UTeamAgent::StaticClass();
	UClass* const HeroClass = AHeroCharacter::StaticClass();
	UClass* const UnitClass = ASummonedUnit::StaticClass();
	UClass* const TowerClass = ATower::StaticClass();

	if (!GhostClass || !TeamAgentInterface || !HeroClass || !UnitClass || !TowerClass)
	{
		AddError(TEXT("SELF-CHECK FAILED: a StaticClass() lookup returned null — the reflection instrument is dead and nothing below would mean anything."));
		return false;
	}

	// ── SELF-CHECKS: prove ImplementsInterface can still see a POSITIVE. ──
	// ⛔ Without these three, the negative assertion below would pass even if
	// ImplementsInterface were broken or TeamAgentInterface were the wrong UClass.
	// All three are actors the acquisition enumerations DO return today.
	TestTrue(TEXT("SELF-CHECK: AHeroCharacter DOES implement ITeamAgent — the probe can see a positive"),
		HeroClass->ImplementsInterface(TeamAgentInterface));
	TestTrue(TEXT("SELF-CHECK: ASummonedUnit DOES implement ITeamAgent — the probe is live on the unit fleet"),
		UnitClass->ImplementsInterface(TeamAgentInterface));
	TestTrue(TEXT("SELF-CHECK: ATower DOES implement ITeamAgent — the probe is live on the tower cards"),
		TowerClass->ImplementsInterface(TeamAgentInterface));

	// ── THE CLAIM, LANE 1: the REFLECTION lane. ──
	// This is the exact predicate UGameplayStatics::GetAllActorsWithInterface
	// filters on, so this assertion IS the acquisition result for every one of the
	// eight enumerations listed in SiegeGhostPawn.h — ASummonedUnit::AcquireTarget
	// (SummonedUnit.cpp:1717), AcquireEnemyNearPoint (:2262), ATower::AcquireTarget
	// (Tower.cpp:220), the chain zap (:378), FSiegeCombatStatics::ApplyRadialDamage
	// (SiegeCombatStatics.cpp:36 — EVERY AoE), USpellLibrary (:64),
	// USpellLineSweep (:133) and AHeroCharacter's melee (HeroCharacter.cpp:404).
	TestFalse(
		TEXT("⭐⭐ ASiegeGhostPawn does NOT implement ITeamAgent. ")
		TEXT("THIS IS THE ENTIRE \"cannot be attacked\" MECHANISM (GHOST-§1) — every hostile-actor ")
		TEXT("selection in this project gathers its candidates with ")
		TEXT("GetAllActorsWithInterface(UTeamAgent::StaticClass()), so an actor outside the interface ")
		TEXT("is never returned to ANY of them. If this assertion is red, someone added ITeamAgent to ")
		TEXT("the ghost and the ghost is now ATTACKABLE — units, towers, chain zaps, EVERY AoE blast and ")
		TEXT("spells can all reach it. ⛔ Do NOT fix this by adding a bInvulnerable flag or a damage ")
		TEXT("guard; REMOVE THE INTERFACE."),
		GhostClass->ImplementsInterface(TeamAgentInterface));

	// ── THE CLAIM, LANE 2: the CAST lane — a genuinely different code path. ──
	// After gathering, every acquisition narrows with Cast<ITeamAgent>(Candidate)
	// (SummonedUnit.cpp:1688 is the canonical one) to read GetTeamId(). Asserting
	// the cast independently means the guarantee holds even if the reflection
	// table and the cast machinery ever disagreed.
	const ASiegeGhostPawn* const GhostDefaults = GetDefault<ASiegeGhostPawn>();
	const AHeroCharacter* const HeroDefaults = GetDefault<AHeroCharacter>();
	if (!GhostDefaults || !HeroDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<>() returned null for the ghost or the hero."));
		return false;
	}

	TestNotNull(TEXT("SELF-CHECK: Cast<ITeamAgent> DOES succeed on AHeroCharacter — the cast lane is live"),
		Cast<ITeamAgent>(HeroDefaults));

	TestNull(
		TEXT("⭐ Cast<ITeamAgent>(ASiegeGhostPawn) is NULL — the second, independent lane of the same ")
		TEXT("guarantee: the narrowing cast every acquisition performs after gathering can never ")
		TEXT("resolve the ghost to a team agent."),
		Cast<ITeamAgent>(GhostDefaults));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 — NO HEALTH BAR, BECAUSE THERE IS NO HEALTH (`GHOST-§5`).
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostPawnNotAHealthBarProviderTest,
	"Siegebound.Ghost.DoesNotImplementIHealthBarProvider",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostPawnNotAHealthBarProviderTest::RunTest(const FString& Parameters)
{
	UClass* const GhostClass = ASiegeGhostPawn::StaticClass();
	UClass* const HealthBarInterface = UHealthBarProvider::StaticClass();
	UClass* const HeroClass = AHeroCharacter::StaticClass();

	if (!GhostClass || !HealthBarInterface || !HeroClass)
	{
		AddError(TEXT("SELF-CHECK FAILED: a StaticClass() lookup returned null."));
		return false;
	}

	// SELF-CHECK: the probe can see a positive on the class that DOES provide one.
	TestTrue(TEXT("SELF-CHECK: AHeroCharacter DOES implement IHealthBarProvider — the probe is live"),
		HeroClass->ImplementsInterface(HealthBarInterface));

	TestFalse(
		TEXT("⛔ ASiegeGhostPawn does NOT implement IHealthBarProvider (GHOST-§5) — it has no health, ")
		TEXT("so it has no bar to show. A health bar over an untargetable pawn would advertise a ")
		TEXT("damage model that does not exist."),
		GhostClass->ImplementsInterface(HealthBarInterface));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 — `G-2` COLLISION: BLOCKS WorldStatic, IGNORES Pawn, AND STAYS ECC_Pawn.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostPawnCollisionTest,
	"Siegebound.Ghost.BlocksWorldStaticAndNeverBlocksPawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostPawnCollisionTest::RunTest(const FString& Parameters)
{
	const ASiegeGhostPawn* const GhostDefaults = GetDefault<ASiegeGhostPawn>();
	if (!GhostDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<ASiegeGhostPawn>() returned null."));
		return false;
	}

	const UCapsuleComponent* const GhostCapsule = GhostDefaults->GetCapsuleComponent();
	if (!GhostCapsule)
	{
		AddError(TEXT("SELF-CHECK FAILED: the ghost's capsule component is null on the CDO — every claim below reads from it."));
		return false;
	}

	const ECollisionResponse WorldStaticResponse = GhostCapsule->GetCollisionResponseToChannel(ECC_WorldStatic);
	const ECollisionResponse PawnResponse = GhostCapsule->GetCollisionResponseToChannel(ECC_Pawn);

	// ── SELF-CHECK: prove we are reading a real PER-CHANNEL container. ──
	// ⛔ If every channel answered the same value, both assertions below could pass
	// or fail together for a reason that has nothing to do with G-2. Requiring the
	// two responses to DIFFER proves the container is populated and discriminating.
	TestTrue(
		TEXT("SELF-CHECK: the WorldStatic and Pawn responses DIFFER — the response container is real ")
		TEXT("and per-channel, not a uniform default answering every query the same way"),
		WorldStaticResponse != PawnResponse);

	// ⛔ TestTrue on an explicit comparison, ⛔ NOT TestEqual/TestNotEqual, for every
	// enum claim in this test: the automation API ships TestNotEqual for strings,
	// FText and FName ONLY — there is no generic overload (AutomationTest.h:2004-2012),
	// and the generic TestEqual template needs a reportable formatter these
	// collision enums do not have. This is the house workaround already recorded in
	// SiegeClimbableTowerTest.cpp.

	// (a) It must have a floor.
	TestTrue(
		TEXT("(a) ✅ G-2: the ghost BLOCKS ECC_WorldStatic — this is what makes it WALK THE GROUND ")
		TEXT("instead of falling through the world, and it is also what makes G-1's \"no wall-pass\" ")
		TEXT("true, since the castle's real walls are WorldStatic geometry."),
		WorldStaticResponse == ECR_Block);

	// (b) ⭐ The one that matters most: no body-blocking.
	TestTrue(
		TEXT("(b) ⛔⛔ G-2: the ghost must NEVER BLOCK ECC_Pawn. A ghost that blocked pawns would be a ")
		TEXT("FREE BODY-BLOCK WALL handed to a dead player — a combat effect on a pawn that is ")
		TEXT("specified to have none. If this is red, the ghost can physically stop the enemy army."),
		PawnResponse != ECR_Block);

	// (c) ⭐ THE PROJECTILE-SHIELD GUARD — the subtle one, and the reason it exists
	//     is a measurement, not a hunch. AProjectile::FindTerrainHit
	//     (Projectile.cpp:434) line-traces ECC_WorldStatic/ECC_WorldDynamic OBJECT
	//     TYPES. A ghost re-typed to WorldStatic — a naive way to read "blocks
	//     WorldStatic" — would enter that hit list and shield its team from fire.
	//     G-2 is a RESPONSE ruling, ⛔ not an object-type ruling.
	const ECollisionChannel GhostObjectType = GhostCapsule->GetCollisionObjectType();

	TestTrue(
		*FString::Printf(
			TEXT("(c) ⭐ the ghost's capsule OBJECT TYPE is ECC_Pawn (read: %d, expected %d). ")
			TEXT("⛔ Re-typing it to ECC_WorldStatic would insert the ghost into ")
			TEXT("AProjectile::FindTerrainHit's object trace (Projectile.cpp:434) and hand a dead ")
			TEXT("player a PROJECTILE SHIELD."),
			static_cast<int32>(GhostObjectType), static_cast<int32>(ECC_Pawn)),
		GhostObjectType == ECC_Pawn);

	// (d) And it must not wear a COMBATANT BODY channel.
	//     AHeroCharacter re-stamps its capsule to SiegeTeamObjectChannel(GetTeamId())
	//     (HeroCharacter.cpp:113); those channels mean "combatant body" in this
	//     project, and a ghost is not one.
	TestTrue(TEXT("SELF-CHECK: ECC_SiegeTeamBlue and ECC_SiegeTeamRed are DISTINCT channels — the constants are real"),
		ECC_SiegeTeamBlue != ECC_SiegeTeamRed);

	TestTrue(
		TEXT("(d) ⛔ the ghost carries NEITHER team combatant channel. Those channels are the ")
		TEXT("COMBATANT BODY channels (HeroCharacter.cpp:113); a ghost is not a combatant body and ")
		TEXT("must never wear one."),
		GhostObjectType != ECC_SiegeTeamBlue && GhostObjectType != ECC_SiegeTeamRed);

	// (e) Collision must actually be ON — QueryOnly would let it sink through the
	//     floor, because CharacterMovement stops at geometry via query sweeps.
	TestTrue(
		TEXT("(e) the capsule is QueryAndPhysics — collision disabled or QueryOnly would drop the ghost ")
		TEXT("through the world, which is the other half of what G-2 is protecting against."),
		GhostCapsule->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 — THE TYPE CLAIM `GHOST-§1`/`§4` ACTUALLY RELY ON: IT IS AN APawn.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostPawnIsAPawnTest,
	"Siegebound.Ghost.IsAPawnAndIsNeverAHeroOrAUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostPawnIsAPawnTest::RunTest(const FString& Parameters)
{
	UClass* const GhostClass = ASiegeGhostPawn::StaticClass();
	UClass* const PawnClass = APawn::StaticClass();
	UClass* const HeroClass = AHeroCharacter::StaticClass();
	UClass* const UnitClass = ASummonedUnit::StaticClass();

	if (!GhostClass || !PawnClass || !HeroClass || !UnitClass)
	{
		AddError(TEXT("SELF-CHECK FAILED: a StaticClass() lookup returned null."));
		return false;
	}

	// SELF-CHECK: IsChildOf can see a positive.
	TestTrue(TEXT("SELF-CHECK: AHeroCharacter IS-A APawn — the IsChildOf probe is live"),
		HeroClass->IsChildOf(PawnClass));

	// ⭐ THE LAW'S OWN CLAIM, MACHINE-CHECKED. GHOST-§1 writes the shape as
	// "ASiegeGhostPawn : public APawn" and GHOST-§4 reasons from it
	// ("...is an APawn, so TryGetPawnOwner() resolves"). This class derives from
	// ACharacter — which IS-A APawn — for the G-1/G-2 movement reasons stated at
	// length in SiegeGhostPawn.h. ⇒ the law's claim is asserted HERE so the
	// deviation can never quietly become a violation.
	TestTrue(
		TEXT("⭐ ASiegeGhostPawn IS-A APawn (GHOST-§1 / GHOST-§4). It derives from ACharacter — which ")
		TEXT("is an APawn — so UCharacterMovementComponent can give it G-1's walk and G-2's ground ")
		TEXT("contact; a bare APawn cannot host that component. Any ABP assigned to its mesh may ")
		TEXT("therefore assume a pawn owner and TryGetPawnOwner() WILL resolve."),
		GhostClass->IsChildOf(PawnClass));

	// ⛔ And it must never inherit the hero's or a unit's combat surface.
	TestFalse(
		TEXT("⛔ ASiegeGhostPawn is NOT an AHeroCharacter — it must never inherit the hero's melee, ")
		TEXT("health, rally or ITeamAgent membership."),
		GhostClass->IsChildOf(HeroClass));

	TestFalse(TEXT("⛔ ASiegeGhostPawn is NOT an ASummonedUnit — it is not a combatant of any kind"),
		GhostClass->IsChildOf(UnitClass));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 5 ⭐ — THE CLASS DECLARES NO COMBAT, DAMAGE OR SUPPRESSION SURFACE.
//  This is the assertion that catches the WRONG FIX: someone "restoring" an
//  attack, or adding the bInvulnerable flag GHOST-§1 explicitly refused.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostPawnNoCombatSurfaceTest,
	"Siegebound.Ghost.DeclaresNoAttackDamageOrInvulnerabilityMember",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostPawnNoCombatSurfaceTest::RunTest(const FString& Parameters)
{
	UClass* const GhostClass = ASiegeGhostPawn::StaticClass();
	UClass* const HeroClass = AHeroCharacter::StaticClass();
	if (!GhostClass || !HeroClass)
	{
		AddError(TEXT("SELF-CHECK FAILED: a StaticClass() lookup returned null."));
		return false;
	}

	// Walk ONLY what each class DECLARES ITSELF (ExcludeSuper). ⛔ Including
	// inherited members would trip on AActor::TakeDamage and ACharacter's own
	// surface, which this class neither adds nor can remove.
	//
	// ⚠️ LIMITATION, STATED RATHER THAN GLOSSED (an overclaimed test is worse than
	// a modest one): this walk sees only REFLECTED members — UPROPERTY and
	// UFUNCTION. A plain, un-reflected C++ member such as `bool bInvulnerable;`
	// would evade it entirely. That is accepted deliberately, because the shapes
	// this test defends against are all reflected by nature: a designer-tickable
	// flag must be a UPROPERTY to be tickable, and a Blueprint-callable attack must
	// be a UFUNCTION to be callable. The un-reflected case is QA's diff review, and
	// it is named here so nobody reads a green bar as covering it.
	auto CollectDeclaredNames = [](UClass* Class) -> TArray<FString>
	{
		TArray<FString> Names;
		for (TFieldIterator<FProperty> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			Names.Add(It->GetName());
		}
		for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			Names.Add(It->GetName());
		}
		return Names;
	};

	const TArray<FString> GhostNames = CollectDeclaredNames(GhostClass);
	const TArray<FString> HeroNames = CollectDeclaredNames(HeroClass);

	// ── SELF-CHECK A: the walk FOUND the members this class is known to declare. ──
	// ⛔ An empty walk would make every banned-token check below pass vacuously.
	TestTrue(TEXT("SELF-CHECK: the reflection walk found members declared by ASiegeGhostPawn"),
		GhostNames.Num() > 0);
	TestTrue(TEXT("SELF-CHECK: …including GhostTeam by name — the walk is reading THIS class"),
		GhostNames.Contains(TEXT("GhostTeam")));
	TestTrue(TEXT("SELF-CHECK: …and GhostMappingContext, the property GHOST-§4 depends on"),
		GhostNames.Contains(TEXT("GhostMappingContext")));

	// The banned tokens. "Invulnerab" and "Immune" are here because GHOST-§1
	// explicitly refused that shape: the ghost is safe because it is not in the
	// candidate list, ⛔ never because a flag says so.
	static const TCHAR* const BannedTokens[] = {
		TEXT("Attack"), TEXT("Damage"), TEXT("Invulnerab"), TEXT("Immune"), TEXT("Health")
	};

	// ── SELF-CHECK B ⭐: prove the token scan can find a POSITIVE. ──
	// AHeroCharacter demonstrably declares attack/damage/health members, so if the
	// scan finds NONE of the banned tokens on the hero, the scan itself is blind
	// and the ghost's clean result below means nothing.
	int32 HeroHits = 0;
	for (const TCHAR* const Token : BannedTokens)
	{
		for (const FString& Name : HeroNames)
		{
			if (Name.Contains(Token))
			{
				++HeroHits;
				break;
			}
		}
	}
	TestTrue(
		TEXT("SELF-CHECK: the banned-token scan FINDS these tokens on AHeroCharacter — the scan is live. ")
		TEXT("If this is red, the clean result on the ghost below is vacuous, not a pass."),
		HeroHits > 0);

	// ── THE CLAIM. ──
	for (const TCHAR* const Token : BannedTokens)
	{
		TArray<FString> Offenders;
		for (const FString& Name : GhostNames)
		{
			if (Name.Contains(Token))
			{
				Offenders.Add(Name);
			}
		}

		TestTrue(
			*FString::Printf(
				TEXT("⛔ ASiegeGhostPawn declares NO member containing '%s' — the ghost cannot attack ")
				TEXT("and cannot be attacked, and BOTH are structural (no binding exists / not an ")
				TEXT("ITeamAgent). ⛔ Do NOT add a suppression flag; that shape was refused by GHOST-§1 ")
				TEXT("because it would also force an edit to ASummonedUnit::IsTargetAlive. Offenders: %s"),
				Token, Offenders.Num() > 0 ? *FString::Join(Offenders, TEXT(", ")) : TEXT("(none)")),
			Offenders.Num() == 0);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 6 — `G-4`: THE ENEMY CAN SEE IT. His explicit words, not a default.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostPawnEnemyVisibleTest,
	"Siegebound.Ghost.IsVisibleToEveryoneNotJustItsOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostPawnEnemyVisibleTest::RunTest(const FString& Parameters)
{
	const ASiegeGhostPawn* const GhostDefaults = GetDefault<ASiegeGhostPawn>();
	if (!GhostDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<ASiegeGhostPawn>() returned null."));
		return false;
	}

	const USkeletalMeshComponent* const GhostMeshComponent = GhostDefaults->GetMesh();
	if (!GhostMeshComponent)
	{
		AddError(TEXT("SELF-CHECK FAILED: the ghost's skeletal mesh component is null on the CDO — G-4 cannot be satisfied without a body to render."));
		return false;
	}

	// ⚠️ Deliberately the OPPOSITE of MARK-§ M-3, where map marks are NOT
	// enemy-visible. ⭐ The contrast is a RULING, not an inconsistency — asserted
	// here so nobody ever "makes the two features consistent".
	TestFalse(
		TEXT("(a) ✅ G-4: bOnlyOwnerSee is FALSE — Jonathan's explicit words are \"The enemy should be ")
		TEXT("able to see this ghost as well.\" An owner-only ghost would break a stated requirement."),
		GhostMeshComponent->bOnlyOwnerSee);

	TestFalse(
		TEXT("(b) ✅ G-4: bOwnerNoSee is FALSE — the ghost's own player must see the body they are ")
		TEXT("driving, which is also how they know they are a ghost."),
		GhostMeshComponent->bOwnerNoSee);

	TestTrue(
		TEXT("(c) the mesh is set to be visible at all — a hidden component satisfies neither half of G-4"),
		GhostMeshComponent->IsVisible() || GhostMeshComponent->GetVisibleFlag());

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 7 — `GHOST-§6` M8 TIER B, DECLARED **AND** ASSERTED.
//  ⭐ A declaration in a comment cannot fail; this can.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostPawnNetRelevancyTierTest,
	"Siegebound.Ghost.NetCullDistanceIsArenaScaledTierB",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostPawnNetRelevancyTierTest::RunTest(const FString& Parameters)
{
	const ASiegeGhostPawn* const GhostDefaults = GetDefault<ASiegeGhostPawn>();
	const AHeroCharacter* const HeroDefaults = GetDefault<AHeroCharacter>();
	if (!GhostDefaults || !HeroDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<>() returned null."));
		return false;
	}

	// SELF-CHECK: the arena constant is real and non-trivial, so equality below is
	// not an accident of both sides being zero.
	TestTrue(TEXT("SELF-CHECK: SiegeNet::ArenaRelevancyDistanceSquared is > 0 — the constant is real"),
		SiegeNet::ArenaRelevancyDistanceSquared > 0.f);

	TestEqual(
		TEXT("(a) ⚖️ GHOST-§6 Tier B: the ghost's net cull distance is the ONE arena constant, ")
		TEXT("SiegeNet::ArenaRelevancyDistanceSquared — ⛔ never a per-class literal. G-4 requires the ")
		TEXT("ENEMY to see this across the full arena; under the engine's default cull it would pop in ")
		TEXT("and out at range."),
		GhostDefaults->GetNetCullDistanceSquared(), SiegeNet::ArenaRelevancyDistanceSquared);

	// ⭐ And it must match the hero's, because the ghost STANDS IN for the hero:
	// an enemy who could see the hero at range must see the ghost at that range.
	TestEqual(
		TEXT("(b) ⭐ the ghost is on the SAME relevancy tier as AHeroCharacter — it stands in for the ")
		TEXT("hero, so it must be visible wherever the hero would have been. This assertion goes red if ")
		TEXT("either class is retuned without the other."),
		GhostDefaults->GetNetCullDistanceSquared(), HeroDefaults->GetNetCullDistanceSquared());

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 8 — `G-1`: NO FLIGHT, AND THE DERIVED-SPEED SOURCE IS LIVE.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostPawnMovementParityTest,
	"Siegebound.Ghost.WalksAndNeverFliesAndDerivesTheHeroSpeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostPawnMovementParityTest::RunTest(const FString& Parameters)
{
	const ASiegeGhostPawn* const GhostDefaults = GetDefault<ASiegeGhostPawn>();
	const AHeroCharacter* const HeroDefaults = GetDefault<AHeroCharacter>();
	if (!GhostDefaults || !HeroDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<>() returned null."));
		return false;
	}

	const UCharacterMovementComponent* const GhostMovement = GhostDefaults->GetCharacterMovement();
	if (!GhostMovement)
	{
		AddError(TEXT("SELF-CHECK FAILED: the ghost has no UCharacterMovementComponent on the CDO — G-1's walk and G-2's ground contact both come from it."));
		return false;
	}

	// (a) ⛔ G-1 "no flight". His words were "similar to their original body";
	//     a flying ghost is a scouting buff granted as a reward for dying.
	TestFalse(
		TEXT("(a) ⛔ G-1: the ghost CANNOT FLY. His words are \"similar to their original body\", and ")
		TEXT("GHOST-§3 G-1 forbids flight, wall-pass and extended vision explicitly."),
		GhostMovement->NavAgentProps.bCanFly);

	TestTrue(
		TEXT("(b) the ghost's DEFAULT LAND MOVEMENT MODE is WALKING — on the ground, not flying or ")
		TEXT("falling by default."),
		GhostMovement->DefaultLandMovementMode == MOVE_Walking);

	TestEqual(
		TEXT("(c) the ghost is under NORMAL gravity — a reduced GravityScale would be flight by ")
		TEXT("another name, which G-1 forbids."),
		GhostMovement->GravityScale, 1.0f);

	// (d) ⭐ THE DERIVATION SOURCE, ASSERTED RATHER THAN ASSUMED.
	//     BeginPlay sets MaxWalkSpeed from GetDefault<AHeroCharacter>()->
	//     GetWalkSpeed() so that G-1's "THE HERO'S OWN" speed cannot
	//     drift into a duplicated literal. That derivation is only as good as its
	//     source — and this assertion is what goes red if the hero's speed API is
	//     ever removed, renamed or zeroed by another task.
	//     ⭐ This reads the SAME public accessor BeginPlay reads (TASK-761): the
	//     protected GetEffectiveWalkSpeed() is unreachable from a non-derived class,
	//     so asserting on the wrapper is asserting on the exact call the ghost makes.
	const float HeroWalkSpeed = HeroDefaults->GetWalkSpeed();
	TestTrue(
		*FString::Printf(
			TEXT("(d) ⭐ SELF-CHECK ON THE DERIVATION SOURCE: AHeroCharacter's default effective walk ")
			TEXT("speed is %.2f and must be > 0. ASiegeGhostPawn::BeginPlay DERIVES MaxWalkSpeed from ")
			TEXT("this value (G-1 \"the hero's OWN\" speed, never a duplicated literal). If this is red, ")
			TEXT("the ghost falls back to the movement default and G-1 parity is silently lost."),
			HeroWalkSpeed),
		HeroWalkSpeed > 0.f);

	// (e) ⛔ The ghost must NOT shove the living fleet — the physics half of the
	//     same body-block concern G-2's ECC_Pawn=Ignore handles on the query side.
	TestFalse(
		TEXT("(e) ⛔ physics interaction is OFF — a ghost that could shove units would have a combat ")
		TEXT("effect through impulses even with its query response set to Ignore."),
		GhostMovement->bEnablePhysicsInteraction);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TESTS 9 AND 10 (TASK-758) — THE INITIALISATION ORDERING.
//
//  ⛔⛔ THE DEFECT THEY DEFEND AGAINST, MEASURED AT THE CALL SITE
//  (`SiegeGameMode.cpp:823-848`): the lifecycle owner runs
//  `SpawnActor` → `InitializeGhost(Team)` → `Possess`, and **`BeginPlay` fires
//  INSIDE `SpawnActor`.** ⇒ during `BeginPlay` the ghost's team is STILL ITS
//  DECLARATION DEFAULT (`Blue`), so any team-dependent work placed there is
//  **silently wrong for Red on every single death** — compiling, reviewing as
//  correct, and invisible.
//
//  ⚠️⚠️ WHY THESE TESTS ARE SHAPED THE WAY THEY ARE, AND IT IS THE LESSON THAT COST
//  THIS PIPELINE TWO QA LOOPS THIS WEEK: **AN ASSERTION THAT PASSES UNDER BOTH THE
//  OLD AND THE NEW ORDERING PROVES NOTHING.** Asserting `GetGhostTeam() == Red`
//  after `InitializeGhost(Red)` would have passed against the DEFECTIVE code — the
//  field was always assigned correctly; what was wrong was **WHEN the work that
//  reads it happened.** ⇒ these tests measure the team the team-dependent step
//  **ACTUALLY OBSERVED** (`HasAppliedTeamAppearance`), which under the old ordering
//  was `Blue` on a Red ghost, and there is a self-check below proving that value
//  can differ from the argument at all.
//
//  ⛔ NO WORLD, ⛔ NO SpawnActor, ⛔ NO PIE, ⛔ NO ASSET LOAD: `GhostMaterial` and
//  `GhostMesh` ship unset in C++, so the appearance step resolves no soft pointer
//  and emits no log line. The instances are transient and GC-rooted for the test.
//
//  ⚠️⚠️ WHAT THEY DELIBERATELY DO ⛔ NOT COVER, STATED SO A GREEN BAR IS ⛔ NOT
//  OVER-READ (an overclaimed test is worse than a modest one): **they cannot RUN
//  `BeginPlay`.** An actor outside a world never begins play — `DispatchBeginPlay`
//  no-ops without one — and this directory ships ⛔ no world by house rule. ⇒ they
//  prove the team-dependent step is **DRIVEN BY, and OBSERVES, the team hand-off**;
//  they do ⛔ **not** prove that `BeginPlay` itself stays free of team-dependent
//  lines. ⭐ That second claim is carried by the DESIGN (one gated seat, documented
//  at both call sites) and by **QA's diff review** — ⛔ not by these assertions.
// ═══════════════════════════════════════════════════════════════════════════════

namespace
{
	/** A transient, world-free ghost instance. ⛔ Never the CDO — mutating that would leak into every other test in this file. */
	TStrongObjectPtr<ASiegeGhostPawn> MakeScratchGhost()
	{
		return TStrongObjectPtr<ASiegeGhostPawn>(
			NewObject<ASiegeGhostPawn>(GetTransientPackageAsObject(), ASiegeGhostPawn::StaticClass(), NAME_None, RF_Transient));
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 9 ⭐⭐ — THE TEAM-DEPENDENT STEP OBSERVES THE **REAL** TEAM, NOT THE DEFAULT.
//  ⛔ THIS TEST FAILS AGAINST THE PRE-TASK-758 ORDERING.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostPawnTeamKnownBeforeTeamWorkTest,
	"Siegebound.Ghost.TeamDependentInitialisationObservesTheRealTeamNotTheDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostPawnTeamKnownBeforeTeamWorkTest::RunTest(const FString& Parameters)
{
	const ASiegeGhostPawn* const GhostDefaults = GetDefault<ASiegeGhostPawn>();
	if (!GhostDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<ASiegeGhostPawn>() returned null."));
		return false;
	}

	// ── SELF-CHECK A ⭐: THE DEFAULT IS BLUE, WHICH IS WHAT MAKES A RED CLAIM ABLE
	//    TO FAIL AT ALL. If the declaration default were ever changed to Red, row (d)
	//    below would pass vacuously against the broken ordering too — so the premise
	//    of the whole test is asserted rather than assumed.
	const ETeamId DeclarationDefaultTeam = GhostDefaults->GetGhostTeam();
	TestTrue(
		TEXT("SELF-CHECK: ASiegeGhostPawn's declaration default team is Blue. ⭐ This is the PREMISE of ")
		TEXT("this test: the pre-TASK-758 defect was that team-dependent work in BeginPlay read THIS ")
		TEXT("value instead of the real team, so a Red assertion below is only discriminating while the ")
		TEXT("default is NOT Red."),
		DeclarationDefaultTeam == ETeamId::Blue);

	TStrongObjectPtr<ASiegeGhostPawn> RedGhost = MakeScratchGhost();
	if (!RedGhost.IsValid())
	{
		AddError(TEXT("SELF-CHECK FAILED: NewObject<ASiegeGhostPawn>() returned null — nothing below would mean anything."));
		return false;
	}

	// ── (a) ⭐ BEFORE THE TEAM IS KNOWN, THE TEAM-DEPENDENT STEP MUST NOT HAVE RUN.
	//     This is the half of the fix that makes the trap unable to fire: the step is
	//     gated on the team being KNOWN, so it cannot execute at a moment that merely
	//     LOOKS like startup and quietly act on the Blue default.
	ETeamId ObservedTeam = ETeamId::Blue;
	const bool bAppliedBeforeInit = RedGhost->HasAppliedTeamAppearance(ObservedTeam);
	TestFalse(
		TEXT("(a) ⛔ a freshly constructed ghost has NOT run its team-dependent initialisation. It is ")
		TEXT("gated on the team being KNOWN (bGhostTeamAssigned), so it can never fire against the Blue ")
		TEXT("declaration default. If this is red, something is doing team-dependent work at a moment ")
		TEXT("when the team has not been set — which is the exact defect TASK-756 found."),
		bAppliedBeforeInit);

	// ── THE SHIPPED SEQUENCE'S DECISIVE STEP: the team becomes known. ──
	RedGhost->InitializeGhost(ETeamId::Red);

	const bool bAppliedAfterInit = RedGhost->HasAppliedTeamAppearance(ObservedTeam);

	// ── (b) The step RAN as a consequence of the team becoming known. ──
	TestTrue(
		TEXT("(b) ⭐ InitializeGhost() DRIVES the team-dependent initialisation — the work happens at the ")
		TEXT("moment the team is actually known, not at a moment that looks like startup. ⛔ Against the ")
		TEXT("pre-TASK-758 code this is RED: InitializeGhost only assigned a field and drove nothing."),
		bAppliedAfterInit);

	// ── (c) ⭐⭐ THE ASSERTION THAT FAILS AGAINST THE OLD ORDERING. ──
	TestTrue(
		*FString::Printf(
			TEXT("(c) ⭐⭐ THE TEAM-DEPENDENT STEP OBSERVED **RED** (read: %d, expected %d). ")
			TEXT("⛔ THIS IS THE ROW THAT GOES RED AGAINST THE OLD ORDERING: BeginPlay fires inside ")
			TEXT("SpawnActor and InitializeGhost is called AFTER it returns (SiegeGameMode.cpp:823 → :842), ")
			TEXT("so team-dependent work in BeginPlay observed the Blue DEFAULT on every Red ghost, every ")
			TEXT("time, silently. ⛔ Do NOT fix a failure here by editing the expected value — move the ")
			TEXT("work back into ApplyGhostTeamAppearance(), which runs when the team is known."),
			static_cast<int32>(ObservedTeam), static_cast<int32>(ETeamId::Red)),
		bAppliedAfterInit && ObservedTeam == ETeamId::Red);

	// ── (d) …and it is explicitly NOT the declaration default. Same claim, stated
	//     against the value the defect would have produced, so the failure message
	//     names the bug rather than a number.
	TestTrue(
		TEXT("(d) ⛔ the observed team is NOT the Blue declaration default — i.e. the work did not happen ")
		TEXT("before anybody had said whose ghost this is."),
		bAppliedAfterInit && ObservedTeam != DeclarationDefaultTeam);

	// ── (e) The record is of the SAME field the published API reports. ──
	TestTrue(
		TEXT("(e) the observed team agrees with GetGhostTeam() — HasAppliedTeamAppearance reports the ")
		TEXT("SAME team TASK-750's published accessor does, so it is a record of the real thing and not ")
		TEXT("a parallel value that could drift."),
		ObservedTeam == RedGhost->GetGhostTeam());

	// ── SELF-CHECK B ⭐: PROVE THE RECORD TRACKS THE ARGUMENT AND IS NOT A CONSTANT.
	//    ⛔ Without this, a `AppliedAppearanceTeam = ETeamId::Red;` hard-coding would
	//    sail through every row above.
	TStrongObjectPtr<ASiegeGhostPawn> BlueGhost = MakeScratchGhost();
	if (!BlueGhost.IsValid())
	{
		AddError(TEXT("SELF-CHECK FAILED: the second NewObject<ASiegeGhostPawn>() returned null."));
		return false;
	}

	BlueGhost->InitializeGhost(ETeamId::Blue);

	ETeamId BlueObservedTeam = ETeamId::Red;
	const bool bBlueApplied = BlueGhost->HasAppliedTeamAppearance(BlueObservedTeam);

	TestTrue(
		TEXT("SELF-CHECK: a ghost initialised BLUE records Blue, and the two instances' recorded teams ")
		TEXT("DIFFER. ⭐ This is what stops row (c) passing for the wrong reason: if the recorded team ")
		TEXT("were hard-coded or were simply echoing the declaration default, these two ghosts could not ")
		TEXT("disagree."),
		bBlueApplied && BlueObservedTeam == ETeamId::Blue && BlueObservedTeam != ObservedTeam);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 10 — THE TEAM-DEPENDENT STEP IS **RE-RUNNABLE**, WHICH IS WHAT MAKES THE
//  OTHER ORDERING CORRECT TOO.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGhostPawnTeamStepIsRerunnableTest,
	"Siegebound.Ghost.TeamDependentInitialisationRerunsAndHasNoAlreadyAppliedEarlyOut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGhostPawnTeamStepIsRerunnableTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<ASiegeGhostPawn> Ghost = MakeScratchGhost();
	if (!Ghost.IsValid())
	{
		AddError(TEXT("SELF-CHECK FAILED: NewObject<ASiegeGhostPawn>() returned null."));
		return false;
	}

	// ⚠️ WHY THIS PROPERTY IS LOAD-BEARING AND NOT A CURIOSITY: the fix calls the
	// team-dependent step from BOTH BeginPlay AND InitializeGhost so that whichever
	// runs LAST is what drives it. Under a deferred spawn (InitializeGhost BEFORE
	// FinishSpawning) the first run happens while the skeletal mesh whose material
	// slots it stamps DOES NOT YET EXIST — BeginPlay's later run is the one that
	// lands. ⇒ an "if (already applied) return;" early-out, which reads like a
	// perfectly sensible idempotence guard, would FREEZE that first empty run and
	// ship an untinted ghost. ⛔ This test is what makes that unfixable-in-silence.
	Ghost->InitializeGhost(ETeamId::Blue);

	ETeamId FirstObservedTeam = ETeamId::Red;
	const bool bFirstApplied = Ghost->HasAppliedTeamAppearance(FirstObservedTeam);
	TestTrue(
		TEXT("(a) the first run happened and observed Blue — the baseline the re-run must be able to ")
		TEXT("overwrite."),
		bFirstApplied && FirstObservedTeam == ETeamId::Blue);

	// The API's own published promise — "safe to call more than once" — now has an
	// assertion behind it rather than only a comment.
	Ghost->InitializeGhost(ETeamId::Red);

	ETeamId SecondObservedTeam = ETeamId::Blue;
	const bool bSecondApplied = Ghost->HasAppliedTeamAppearance(SecondObservedTeam);

	TestTrue(
		*FString::Printf(
			TEXT("(b) ⭐ THE STEP RE-RAN and now observes Red (read: %d, expected %d). ⛔ If this is red, ")
			TEXT("someone added an \"already applied\" early-out to ApplyGhostTeamAppearance(). That guard ")
			TEXT("looks like harmless idempotence and is NOT: it breaks the InitializeGhost-before-BeginPlay ")
			TEXT("ordering, where the first run has no mesh to stamp and only the re-run can land."),
			static_cast<int32>(SecondObservedTeam), static_cast<int32>(ETeamId::Red)),
		bSecondApplied && SecondObservedTeam == ETeamId::Red);

	// ⭐ And the published accessor agrees — a re-initialised ghost is genuinely the
	//   new team everywhere, not only in the record.
	TestTrue(
		TEXT("(c) GetGhostTeam() also reports Red after the second call — the re-initialisation is real ")
		TEXT("and consistent across both reads, so no consumer can see two different answers."),
		Ghost->GetGhostTeam() == ETeamId::Red);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
