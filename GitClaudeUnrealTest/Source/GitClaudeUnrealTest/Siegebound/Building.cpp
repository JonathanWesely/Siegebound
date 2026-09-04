// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/Building.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DamageEvents.h"
#include "Engine/DataTable.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GitClaudeUnrealTest.h"
#include "Materials/MaterialInterface.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/DamageTypes.h"
#include "Siegebound/CombatantHealthBarComponent.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegeHitFlashComponent.h"
#include "Siegebound/SiegeMeshJuiceComponent.h"
#include "TimerManager.h"

namespace
{
	/** Height above a building's origin for its floating damage number. */
	constexpr float BuildingDamageNumberHeightZ = 160.f;
}

ABuilding::ABuilding()
{
	// Stationary — no tick logic in the base (TASK-027 spec). ATower's firing
	// runs on a timer, never on tick, so the whole family stays tickless.
	PrimaryActorTick.bCanEverTick = false;

	// VisualMesh is the root AND the collision (CONVENTIONS: the mesh component
	// on card actors is named exactly VisualMesh; the BP child assigns
	// SM_<CardID> and the team material — TASK-035 — so no mesh or material is
	// set in C++).
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	SetRootComponent(VisualMesh);

	// Explicit blocking profile (the ACastle precedent, qa/TASK-002 WARN —
	// gameplay contracts must not rest on the engine's implicit default): §3.7
	// buildings must physically stop pawns. BlockAll blocks ECC_Pawn (unit
	// capsules, the hero) and keeps the closest-point reach tests of hero
	// melee / unit attacks / projectiles working against buildings — they all
	// query ECC_Pawn (TASK-003/004/026; qa/TASK-026 ruling 2 counts on
	// "buildings block Pawns").
	VisualMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// §3.7 "dynamically updates the navmesh": the mesh is navigation-relevant,
	// so a runtime-placed wall dirties the nav octree and — with
	// RuntimeGeneration=Dynamic (Config/DefaultEngine.ini, this task) — carves
	// the navmesh; units reroute instead of walking through. True is the
	// component default, but the spec pins it EXPLICITLY so a BP child or
	// template change can never silently break §3.7.
	VisualMesh->SetCanEverAffectNavigation(true);

	// Overhead health bar (TASK-130 castle-parity REBUILD): one screen-space, team-tinted PUSH
	// bar per building. Added at the ABuilding base so every subclass (ATower, ABarracks,
	// ADeepMine) and the Wall get it for free; the component binds this building's OnHPChanged
	// delegate (seed-then-bind) and soft-resolves WBP_CombatantHealthBar (TASK-131) null-safe at
	// its own BeginPlay. Attached to VisualMesh (the root). No poll timer.
	HPBarWidget = CreateDefaultSubobject<UCombatantHealthBarComponent>(TEXT("HPBarWidget"));
	HPBarWidget->SetupAttachment(VisualMesh);

	// §6 juice components (TASK-154/155): shared hit-flash (driven from TakeDamage) +
	// transform juice (spawn squash for every building; ATower additionally drives
	// PlayRecoil on fire). Added at the base so ATower/ABarracks/ADeepMine/Wall inherit
	// them. Both null-safe and inert until triggered. NOTE: VisualMesh is the collision
	// ROOT, so the squash/recoil momentarily move/scale it — tiny + brief, and always
	// restored exactly; a BP can zero the durations/distance to disable.
	HitFlashComponent = CreateDefaultSubobject<USiegeHitFlashComponent>(TEXT("HitFlashComponent"));
	MeshJuiceComponent = CreateDefaultSubobject<USiegeMeshJuiceComponent>(TEXT("MeshJuiceComponent"));

	// Data contract (GDD §3.0): stats resolve from DT_Cards at BeginPlay, never
	// from code. Same soft path as ASummonedUnit (TASK-004).
	CardTableAsset = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Cards.DT_Cards")));
}

void ABuilding::BeginPlay()
{
	Super::BeginPlay();

	// TASK-044 (CONVENTIONS Team contract): recolor VisualMesh to the ACTUAL Team.
	// The deferred spawn sets Team before BeginPlay (InitBuilding → FinishSpawning,
	// TASK-030/046), so the correct team material lands here. Cosmetic only — the
	// BlockAll collision, nav relevance, and stat binding are untouched (slot 0 only).
	ApplyTeamMaterial();

	// §6 spawn squash-and-stretch (TASK-155): pop the building once at spawn. The juice
	// captures the authored scale and restores it EXACTLY. Null-safe (no mesh = no-op).
	if (MeshJuiceComponent)
	{
		MeshJuiceComponent->SetTargetMesh(VisualMesh);
		MeshJuiceComponent->PlaySpawnSquash();
	}

	LoadStats();
}

void ABuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// TASK-099: no dangling spell-freeze expiry on a destroyed building
	// (belt-and-braces — the timer manager drops object-bound timers on destroy;
	// this keeps the family's explicit-clear discipline). ATower::EndPlay clears
	// its own fire timer first and chains here via Super::EndPlay.
	GetWorldTimerManager().ClearTimer(SpellFreezeTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ABuilding::ApplyFreeze(float Seconds)
{
	// STATE ONLY (TASK-099, M5 ruling 14): latch bSpellFrozen for the duration —
	// the tower fire-gate consuming it is TASK-101's, and the castle can never
	// arrive here (ACastle is not an ABuilding; ruling 5 gives it NO freeze API).
	// A dying building takes no state; non-positive Seconds is a defensive no-op
	// (FrostNova's EffectDuration is 4).
	if (bDestroyed || Seconds <= 0.f)
	{
		return;
	}

	// refresh-not-stack (ruling 5): the single expiry timer re-arms at
	// max(remaining, new) — a shorter re-freeze never trims a longer one, and
	// nothing ever adds. GetTimerRemaining is only read while the timer is live
	// (it returns -1 otherwise).
	float RemainingFreeze = 0.f;
	if (GetWorldTimerManager().IsTimerActive(SpellFreezeTimerHandle))
	{
		RemainingFreeze = GetWorldTimerManager().GetTimerRemaining(SpellFreezeTimerHandle);
	}

	bSpellFrozen = true;
	GetWorldTimerManager().SetTimer(SpellFreezeTimerHandle, this, &ABuilding::EndSpellFreeze,
		FMath::Max(RemainingFreeze, Seconds), /*bLoop=*/ false);
}

void ABuilding::EndSpellFreeze()
{
	// state-only inverse: drop the latch, nothing to resume (TASK-101's fire path
	// re-checks IsFrozen() every shot). MATCH-END PRECEDENCE (ruling 5) needs no
	// gate here: on towers the match-end ClearAllTimersForObject sweep already
	// cleared this expiry (a match-end-frozen tower stays IsFrozen() until Play
	// Again destroys it), and on non-tower buildings flipping the flag resumes
	// nothing by construction.
	GetWorldTimerManager().ClearTimer(SpellFreezeTimerHandle);
	bSpellFrozen = false;
}

void ABuilding::ApplyTeamMaterial()
{
	// TASK-044 — CONVENTIONS Team contract: the bot reuses the player's BP_Building_*
	// assets (authored with the Blue placeholder material); this overrides slot 0 by
	// the ACTUAL Team so a Red-spawned building reads red with no Red BP duplicate.
	// Blue re-applies the identical MI_TeamColor_Blue, so Blue-side visuals are unchanged.
	if (!VisualMesh)
	{
		return;
	}

	// cached static resolve (spec): the two MI instances resolve ONCE per process and
	// are shared by every building — never a hot-path load. LoadSynchronous re-resolves
	// through the soft path if GC ever unloaded them and returns nullptr for a missing
	// asset — in which case the slot is left as authored (null-safe; the AProjectile::
	// ApplyTeamVisuals pattern, mirrored — keep the MI paths in sync by hand).
	static const TSoftObjectPtr<UMaterialInterface> BlueTeamMaterial(FSoftObjectPath(TEXT("/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue")));
	static const TSoftObjectPtr<UMaterialInterface> RedTeamMaterial(FSoftObjectPath(TEXT("/Game/Materials/Instances/MI_TeamColor_Red.MI_TeamColor_Red")));

	const TSoftObjectPtr<UMaterialInterface>& TeamMat = (Team == ETeamId::Red) ? RedTeamMaterial : BlueTeamMaterial;
	if (UMaterialInterface* ResolvedTeamMat = TeamMat.LoadSynchronous())
	{
		VisualMesh->SetMaterial(0, ResolvedTeamMat);
	}
}

void ABuilding::InitBuilding(ETeamId InTeam, FName InCardID)
{
	Team = InTeam;

	// TASK-044: keep VisualMesh's team color matched to a late/updated Team. Deferred
	// spawns (InitBuilding before FinishSpawning — TASK-030/046) run this pre-BeginPlay
	// (HasActorBegunPlay() false) and BeginPlay does the single apply; a plain
	// SpawnActor + InitBuilding (or a post-bind Team update) re-applies for the now-
	// current Team. Idempotent and null-safe; runs even on the stats-already-bound path.
	if (HasActorBegunPlay())
	{
		ApplyTeamMaterial();
	}

	if (bStatsLoaded)
	{
		if (CardID != InCardID)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ABuilding '%s': InitBuilding card '%s' ignored — stats already bound to '%s' (bind happens once at spawn)."),
				*GetNameSafe(this), *InCardID.ToString(), *CardID.ToString());
		}
		return;
	}

	CardID = InCardID;

	// plain SpawnActor + InitBuilding: BeginPlay already ran (and logged without
	// a usable CardID) — bind now. SpawnActorDeferred callers hit this with
	// !HasActorBegunPlay() and BeginPlay does the bind after FinishSpawning
	// (the TASK-007 unit pattern).
	if (HasActorBegunPlay())
	{
		LoadStats();
	}
}

void ABuilding::LoadStats()
{
	if (bDestroyed || bStatsLoaded)
	{
		return;
	}

	// GDD §3.0: stats live in the data table, NEVER in code. Missing anything =
	// log and stand statless (CurrentHP 0 — dies to the first enemy hit rather
	// than standing invincible; see the class comment).
	const UDataTable* CardTable = CardTableAsset.LoadSynchronous();
	if (!CardTable)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ABuilding '%s': card table '%s' not found (imported from Docs/Data/cards.csv — TASK-008/031) — building has no stats."),
			*GetNameSafe(this), *CardTableAsset.ToString());
		return;
	}

	if (CardID.IsNone())
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ABuilding '%s': CardID is not set (BP children preset it — TASK-035; spawners call InitBuilding — TASK-030) — building has no stats."),
			*GetNameSafe(this));
		return;
	}

	const FCardRow* Row = CardTable->FindRow<FCardRow>(CardID, TEXT("ABuilding::LoadStats"), /*bWarnIfRowMissing=*/ false);
	if (!Row)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ABuilding '%s': row '%s' not found in '%s' — building has no stats."),
			*GetNameSafe(this), *CardID.ToString(), *CardTable->GetName());
		return;
	}

	// bind the card stats (TASK-027 spec: HP from the row — ArrowTower 150,
	// Wall 300). Values are applied as authored (§3.0).
	MaxHP = Row->HP;
	CurrentHP = MaxHP;
	// Push spawn-init HP to the overhead bar (TASK-130 push model); pairs with the component's
	// InitForCombatant seed so the bar is correct regardless of stats-load vs bind order.
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());

	if (Row->HP <= 0.f)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ABuilding '%s': row '%s' has HP %.1f — is this card really a building? (check Docs/Data/cards.csv)"),
			*GetNameSafe(this), *CardID.ToString(), Row->HP);
	}

	if (Row->CardType != ECardType::Building)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ABuilding '%s': row '%s' has CardType %d, expected Building — check the BP child's CardID (TASK-035)."),
			*GetNameSafe(this), *CardID.ToString(), static_cast<int32>(Row->CardType));
	}

	bStatsLoaded = true;

	// subclass stat hook (ATower binds Damage/Range/Cadence and arms its fire
	// loop in here). The BASE deliberately starts no timer of any kind —
	// qa/TASK-021-report.md WARN-1: Wall's Cadence is 0, and SetTimer with a
	// rate <= 0 CLEARS a timer instead of scheduling it.
	OnStatsLoaded(*Row);
}

void ABuilding::OnStatsLoaded(const FCardRow& Row)
{
	// intentionally empty: plain buildings (Wall) bind nothing beyond HP and
	// run no timers (qa/TASK-021 WARN-1). ATower overrides this to arm its
	// cadence loop behind the Cadence > 0 guard (TASK-027 spec item 2).
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  STACK-§ — TOWER STACKING (TASK-812). Deliberately parked directly beneath the MaxHP bind
//  above: the health half of an upgrade is a mutation of the very two lines LoadStats writes,
//  and STACK-§5 J-10 asked for it HERE rather than in the placement path — MaxHP/CurrentHP are
//  private, and a caller reaching around that is how two rules for one number get shipped.
// ═══════════════════════════════════════════════════════════════════════════════════════════

float ABuilding::StackHeightMultiplier(int32 UpgradeCount, int32 MaxMultiplier)
{
	// ⛔⛔ THE CAP IS **HANDED IN**, ⛔ never read here and ⛔ never written as a literal here
	// (STACK-§7's amended row, STACK-§10 cl. 2, ruling J-13). MaxStackHeightMultiplier is
	// EditDefaultsOnly precisely so Jonathan can retune it, and a second `5` in this function
	// would be the HIGH-§1 booby trap the day he does.
	//
	// ⚠️⚠️ THIS FUNCTION READ `GetDefault<ABuilding>()->MaxStackHeightMultiplier` UNTIL
	// 2026-09-03, AND THE COMMENT SITTING HERE CLAIMED IT READ "THIS CLASS'S CDO". ⛔ IT DID
	// NOT — `GetDefault<ABuilding>()` is the BASE class's CDO unconditionally, whatever
	// instance is calling. ⇒ a subclass ceiling set in a constructor was read STRAIGHT PAST
	// and the series kept climbing to ABuilding's `5`, while every readback of the subclass's
	// own tunable reported the number the designer typed. ⭐ For AClimbableTower that is the
	// difference between a stackable tower and one whose deck cannot be reached at all
	// (STACK-§10 cl. 1), so the parameter is LOAD-BEARING, ⛔ not a tidy-up.
	//
	// ⭐ The cap being a parameter also makes this seam completely free of the reflection
	// system: no CDO, no world, no actor instance — and a test can pass a ceiling the project
	// does not ship and assert the SHAPE of the series rather than its literal (SC-§37).

	// The tunable, defended against a hand-edited 0 or negative in a .uasset (ClampMin only
	// guards the editor field) and against a caller that read it off a null pointer. A cap
	// below 1 would mean "shrink every building", which is a state no ruling contemplates.
	// ⛔ The degrade is the IDENTITY series, ⛔ never a restated `5`.
	const int32 Cap = FMath::Max(1, MaxMultiplier);

	// ⛔ Negative is not a state the game can enter (StackUpgradeCount only ever increments
	// from 0), but this seam is public and pure, so it answers for the whole int32 domain
	// rather than trusting its callers: no upgrades applied ⇒ the AUTHORED height, exactly 1.0.
	// ⚠️ The UPPER clamp is not decoration either — it is what makes the `1 +` below unable to
	// OVERFLOW on a pathological count (INT32_MAX + 1 wraps NEGATIVE, i.e. an inverted mesh).
	// Clamping to the cap first is free, because everything at or above it saturates anyway.
	const int32 Upgrades = FMath::Clamp(UpgradeCount, 0, Cap);

	// ⭐⭐ ADDITIVE — `+1 ×` the ORIGINAL per upgrade (STACK-§1, ruling J-0), ⛔ NOT `2ⁿ`.
	// Under the doubling reading Jonathan's own "5 times taller" ceiling is UNREACHABLE
	// (2, 4, 8, 16 …), so his cap sentence would describe a state the game can never enter;
	// under this one ×5 lands EXACTLY, on the 4th upgrade. ⛔ Do not "fix" this toward his
	// summary sentence — it is flagged to him as J-0 instead.
	//
	// ⭐ INTEGER MIN, then ONE widening: "the cap is reached exactly" is a promise only integer
	// arithmetic can keep, and it is the single property that falsifies the wrong series. The
	// saturated value is therefore bit-identical to the tunable rather than a float that merely
	// rounds to it.
	return static_cast<float>(FMath::Min(1 + Upgrades, Cap));
}

float ABuilding::StackHealthMultiplier(int32 UpgradeCount)
{
	// ⛔ A CDO read, and — since 2026-09-03 — ⛔ NOT the same shape as StackHeightMultiplier
	// above. StackHealthStep is EditDefaultsOnly and this function must not be a second copy
	// of its value, but the step stays GAME-WIDE: the height ceiling went per class because a
	// MEASUREMENT forced it (a climbable tower's deck stops being reachable past its own
	// ceiling — STACK-§10), and ⛔ nothing analogous exists for health, which is uncapped by
	// Jonathan's explicit word and touches no geometry. ⇒ ⛔ do not "finish the refactor" by
	// parameterising this one; that would invent a rule nobody ruled (see StackHealthStep).
	const ABuilding* const Defaults = GetDefault<ABuilding>();
	if (!Defaults)
	{
		return 1.f;
	}

	// ⛔ THE GUARD, NaN-SAFE AND APPLIED BEFORE THE LOOP (the HeightAdvantageMultiplier /
	// HeightToBrightness doctrine): StackHealthStep is an EditDefaultsOnly float, and a
	// hand-edited 0, negative or NaN would put a zeroed — or NaN — MaxHP into a live building,
	// which is a silent one-hit-kill rather than a visible bug. Written as !(Step >= 1.f) so
	// NaN, which fails EVERY comparison, lands here too. Below 1 would mean "shrink", which no
	// ruling contemplates (J-3's no-shrinking principle, applied to the other series). A
	// disabled step means NO gain — exactly 1.0 — ⛔ never an explosion.
	const float Step = Defaults->StackHealthStep;
	if (!(Step >= 1.f))
	{
		return 1.f;
	}

	const int32 Upgrades = FMath::Max(0, UpgradeCount);

	// ⭐ REPEATED MULTIPLICATION, ⛔ NOT FMath::Pow. 1.5 and its low powers are EXACTLY
	// representable in binary32 (3ⁿ / 2ⁿ, exact through n = 15), so this returns Jonathan's own
	// numbers bit-for-bit — 1.5² = 2.25 and 1.5³ = 3.375, both of which he wrote out — where
	// powf would return something that merely prints as them.
	//
	// ⛔ UNCAPPED, and that is his explicit word ("there is no maximum on the health"): the
	// loop has no ceiling term. The !IsFinite break is a runaway guard, ⛔ not a cap — it can
	// only trigger once the value has already overflowed float (n ≈ 1,750 at a step of 1.5),
	// a state no match can reach, and it stops a pathological UpgradeCount from spinning.
	float Multiplier = 1.f;
	for (int32 Index = 0; Index < Upgrades; ++Index)
	{
		Multiplier *= Step;
		if (!FMath::IsFinite(Multiplier))
		{
			break;
		}
	}

	return Multiplier;
}

void ABuilding::OnStackUpgradeApplied()
{
	// intentionally empty: a plain building's whole upgrade is the transform and the HP push
	// ApplyStackUpgrade already did, exactly as OnStatsLoaded is empty because a plain
	// building binds nothing beyond HP. AClimbableTower overrides this to re-arm the
	// navigation element its rescaled root does not reach.
	//
	// ⛔ Defined ABOVE ApplyStackUpgrade on purpose, ⛔ not below it: SiegePlacementTest's J-4
	// probe extracts ApplyStackUpgrade's body as "the text from its signature to the next
	// `\nfloat ABuilding::`", so a definition inserted between it and TakeDamage would silently
	// widen what that probe reads (SC-§41 — a gate's needle is only as honest as its window).
}

bool ABuilding::ApplyStackUpgrade()
{
	// ⛔ M8 (STACK-§7): StackUpgradeCount is AUTHORITATIVE GAME STATE — it drives MaxHP — so it
	// is SERVER-SET at confirm and the client may NEVER author it. ⛔ No new RPC and no new
	// relevancy tier are introduced: the resulting HP rides the already-shipped OnHPChanged
	// push, and the ghost / blue state / wheel are client-local PRE-gate.
	if (!HasAuthority())
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ABuilding '%s': ApplyStackUpgrade refused — the client may never author StackUpgradeCount (STACK-§7 M8). The upgrade is applied on the SERVER at confirm."),
			*GetNameSafe(this));
		return false;
	}

	// A dying building takes no upgrade (the same-frame window before Destroy lands — the
	// ApplyFreeze / TakeDamage guard, unchanged).
	if (bDestroyed)
	{
		return false;
	}

	// ⛔⛔ THE STACK (Z) PREDICATE, RE-ASKED AT THE BUILDING ITSELF — the THIRD of the three
	// gates STACK-§8 cl. 3 repointed. The placement path gates on the same virtual at hover
	// and again at the click, and that is the point: a future caller that forgets the
	// paragraph must still be UNABLE to grow a building that refuses it. ⛔ Structural — there
	// is deliberately no CardID string compare anywhere on this path.
	//
	// ⛔⛔ IT IS CanStackHeight(), ⛔ NOT CanScaleFootprint(). Those were ONE virtual until
	// 2026-09-03, which is the defect Jonathan filmed: the height question and the footprint
	// question had one answer between them, so the one building that refuses the placement
	// wheel also refused to be stacked. ⛔ Asking the wheel's predicate here again would
	// restore the bug behind a passing test suite (STACK-§8 cl. 1).
	if (!CanStackHeight())
	{
		return false;
	}

	// ⭐ THE BASELINE, CAPTURED ONCE, ⛔ BEFORE the first increment. The wheel scales X/Y ONLY
	// (STACK-§4), so the Z standing here is the AUTHORED height the additive series is defined
	// against. Capturing lazily rather than at BeginPlay makes this independent of whatever
	// spawn/scale ordering the placement path uses.
	if (StackUpgradeCount == 0 && VisualMesh)
	{
		// FVector is double-precision in UE5 and the series are float — the narrowing is spelled
		// out rather than left implicit.
		AuthoredHeightScaleZ = static_cast<float>(VisualMesh->GetRelativeScale3D().Z);
	}

	++StackUpgradeCount;

	// ── HEIGHT: Z ONLY, RECOMPUTED FROM THE BASELINE ─────────────────────────────────────────
	// ⛔ X and Y are NOT touched: STACK-§5 J-4 is his own sentence, "keeping the same width and
	// length" — they belong to the placement wheel (TASK-815) and an upgrade inherits them
	// VERBATIM. ⭐ And the Z is RECOMPUTED rather than multiplied in place, so the series is a
	// pure function of StackUpgradeCount: no float accumulates, and the cap lands on the ruled
	// multiple exactly however many times this runs.
	// ⭐ Collision and navmesh follow for free — VisualMesh is the root with BlockAll +
	// bCanEverAffectNavigation(true), so a scaled component carves a scaled hole (STACK-§3).
	// ⚠️⚠️ "FOR FREE" IS TRUE OF THIS **SCENE COMPONENT** AND OF NOTHING ELSE: the rescale
	// refreshes VisualMesh's own navigation octree entry (USceneComponent::PropagateTransform-
	// Update -> UpdateNavigationData) and no other component's. A subclass carrying a
	// UActorComponent-derived navigation element re-arms it in OnStackUpgradeApplied below.
	//
	// ⛔⛔ THE CAP HANDED IN IS **THIS INSTANCE'S OWN** (STACK-§10 cl. 2). ⛔ Not
	// GetDefault<ABuilding>()'s, which is what the one-parameter signature used to force and
	// which discarded every subclass ceiling silently.
	if (VisualMesh)
	{
		FVector Scale = VisualMesh->GetRelativeScale3D();
		Scale.Z = AuthoredHeightScaleZ * StackHeightMultiplier(StackUpgradeCount, MaxStackHeightMultiplier);
		VisualMesh->SetRelativeScale3D(Scale);
	}

	// ── HEALTH: GRANT THE NEW HIT POINTS, ⛔ DO NOT REPAIR THE OLD DAMAGE (STACK-§5 J-10) ─────
	// The step is taken through the SERIES rather than off StackHealthStep directly, so the
	// number a HUD preview shows and the number the building actually gains can ⛔ never
	// disagree — one rule, one expression of it.
	// ⚖️ CurrentHP moves by the DELTA, never to the new maximum: a full heal would make the
	// upgrade a repair tool, which is the Masons card's job, and would make upgrading strictly
	// better than defending. A damaged tower stays damaged, and is simply damaged out of a
	// bigger pool. (A statless building — MaxHP 0, the missing-row failure mode — stays at 0
	// through this, which is correct: 0 × anything is still 0.)
	const float OldMaxHP = MaxHP;
	MaxHP = OldMaxHP * StackHealthMultiplier(1);
	CurrentHP += (MaxHP - OldMaxHP);

	// The EXISTING push (TASK-130), ⛔ not a second one: the overhead bar and every
	// IHealthBarProvider consumer already listen here, so the new pool reaches the UI with no
	// new lane and no direct widget call.
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());

	// ⭐⭐ THE SUBCLASS HOOK, ⛔ LAST AND ⛔ ONLY ON SUCCESS. Everything a subclass might need
	// to re-derive from the new height — a navigation element, a marker, a bar — is valid only
	// once the transform above has actually been written, and must ⛔ never run on a path that
	// refused. ⛔ This class knows nothing about what the override does; see the declaration.
	OnStackUpgradeApplied();

	// ⚠️⚠️⚠️ A TRAP PARKED HERE DELIBERATELY, BECAUSE THIS IS THE FUNCTION SOMEBODY WOULD EDIT
	// TO SET IT OFF — ⛔ REPORTED, ⛔ NOT REPAIRED, AND THERE IS ⛔ NOTHING TO FIX TODAY
	// (STACK-§10 cl. 6a):
	//
	//   USiegeMeshJuiceComponent::SetTargetMesh SNAPSHOTS `BaseScale = InMesh->
	//   GetRelativeScale3D()`, and the squash channel's terminal branch writes
	//   `TargetMesh->SetRelativeScale3D(BaseScale)` VERBATIM.
	//
	// ⇒ ⛔ A "juicy little squash on upgrade" added HERE would, at the end of its animation,
	// restore the mesh to the scale that component snapshotted — SILENTLY UN-STACKING the
	// building: its height AND, on a climbable one, its whole climb line, with ⛔ every
	// readback still reporting the correct StackUpgradeCount and the correct MaxHP.
	//
	// ✅ MEASURED SAFE AS SHIPPED, which is why this is a comment and not a change:
	// SetTargetMesh/PlaySpawnSquash are called exactly ONCE, from ABuilding::BeginPlay, the
	// scale channel runs only while that one squash is active, and AClimbableTower derives
	// ABuilding rather than ATower so ATower::PlayRecoil (a LOCATION channel anyway) cannot
	// reach it. ⛔ Do NOT "harden" the juice component for this — the hazard is the CALL that
	// does not exist, and adding one is what would create it.
	return true;
}

float ABuilding::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// a dying building absorbs nothing further (same-frame window before the
	// Destroy lands)
	if (bDestroyed || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	// no friendly fire (GDD §3.0): same-team damage is ignored entirely. Super
	// is skipped so damage delegates never observe friendly hits — the receiver
	// pattern QA approved on ACastle (TASK-002) and ASummonedUnit (TASK-004).
	ETeamId AttackerTeam = ETeamId::Blue;
	if (TryGetDamageTeam(EventInstigator, DamageCauser, AttackerTeam) && AttackerTeam == Team)
	{
		return 0.f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage <= 0.f)
	{
		return 0.f;
	}

	// Damage-vs-fortification scaling (GDD §3.0, M4 ruling — TASK-054): Siege
	// units (Ogre, Sapper) batter buildings for 200% via USiegeDamageType_Siege,
	// exactly like the castle. Everything else takes LISTED damage — the §3.0
	// projectile-50% rule stays castle-ONLY (an Archer bolt still hurts a wall for
	// its listed 10; M2 ruling / TASK-026 handoff). Siege is the only building-side
	// scaler.
	// TODO(Spell 50% — M5): spell damage types vs buildings (GDD §3.0).
	float ScaledDamage = ActualDamage;
	const UClass* IncomingDamageType = DamageEvent.DamageTypeClass.Get();
	if (IncomingDamageType && IncomingDamageType->IsChildOf(USiegeDamageType_Siege::StaticClass()))
	{
		ScaledDamage *= 2.0f;
	}

	CurrentHP = FMath::Max(CurrentHP - ScaledDamage, 0.f);

	// Push the damage to the overhead bar BEFORE any destruction handling (ACastle::TakeDamage
	// parity — listeners see the 0-HP value before HandleDestroyed tears the actor down).
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());

	// §6 damage feedback (TASK-154/156) — ACTUAL damage only (friendly fire returned above).
	// Flash white ~0.1 s; float the SCALED amount actually applied (a Siege 2× hit shows 2×),
	// tinted by team. Both null-safe; run before HandleDestroyed so the killing blow flashes.
	if (HitFlashComponent)
	{
		HitFlashComponent->TriggerFlash();
	}
	USiegeFeedbackLibrary::ShowDamageNumber(this, ScaledDamage,
		GetActorLocation() + FVector(0.f, 0.f, BuildingDamageNumberHeightZ), USiegeFeedbackLibrary::TeamTint(Team));

	if (CurrentHP <= 0.f)
	{
		HandleDestroyed();
	}

	// return the SCALED amount actually applied (the ACastle contract) — a Siege
	// hit reports 2×, everything else its listed value.
	return ScaledDamage;
}

void ABuilding::HandleDestroyed()
{
	// destruction side effects run exactly once (overkill / duplicate calls)
	if (bDestroyed)
	{
		return;
	}
	bDestroyed = true;
	CurrentHP = 0.f;

	// Push the final 0-HP to the overhead bar (TASK-130 push model — covers destruction paths
	// that do not route through TakeDamage). The actor is destroyed below, taking the bar with it.
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());

	// §3.7 destructible: the actor simply goes away (crumble FX is M7).
	// Destroy() tears down ATower's fire timer synchronously via EndPlay and
	// unregisters VisualMesh from the navigation octree — under
	// RuntimeGeneration=Dynamic the navmesh heals and units path through the
	// gap a dead wall leaves.
	Destroy();
}

bool ABuilding::TryGetDamageTeam(AController* EventInstigator, AActor* DamageCauser, ETeamId& OutTeam)
{
	// mirror of ACastle::TryGetInstigatorTeam / ASummonedUnit::TryGetDamageTeam
	// (TASK-002/004) — both live as PRIVATE statics in frozen upstream files,
	// so the chain is mirrored here per the QA-accepted precedent (qa/TASK-004,
	// qa/TASK-026 ruling 10). Keep all copies in sync by hand.

	// 1) the instigating controller's pawn (hero melee and unit attacks report their controller)
	if (EventInstigator)
	{
		if (const ITeamAgent* Agent = Cast<ITeamAgent>(EventInstigator->GetPawn()))
		{
			OutTeam = Agent->GetTeamId();
			return true;
		}
	}

	// 2) the damage causer itself (hero and units pass themselves; projectiles
	//    pass the projectile — deliberately NOT an ITeamAgent, falls through)
	if (const ITeamAgent* Agent = Cast<ITeamAgent>(DamageCauser))
	{
		OutTeam = Agent->GetTeamId();
		return true;
	}

	// 3) the causer's instigator pawn (pawn-fired projectiles: TASK-028 archers)
	if (DamageCauser)
	{
		if (const ITeamAgent* Agent = Cast<ITeamAgent>(DamageCauser->GetInstigator()))
		{
			OutTeam = Agent->GetTeamId();
			return true;
		}
	}

	// no team resolvable — caller APPLIES the damage. This is the documented
	// path for TOWER-fired projectiles (non-pawn shooter, TASK-026 handoff):
	// safe, because such a projectile only ever damages the single enemy it was
	// fired at, gated by its own same-team impact check (qa/TASK-026 ruling 3).
	return false;
}
