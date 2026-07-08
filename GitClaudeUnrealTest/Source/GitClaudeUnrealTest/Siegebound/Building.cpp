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
#include "TimerManager.h"

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
