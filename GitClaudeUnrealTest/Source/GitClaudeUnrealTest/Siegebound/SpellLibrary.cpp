// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SpellLibrary.h"

#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h" // TActorIterator — hero/castle line-origin lookup (TASK-236)
#include "GameFramework/Actor.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Siegebound/Building.h"
#include "Siegebound/Castle.h"
#include "Siegebound/DamageTypes.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeCombatStatics.h"
#include "Siegebound/SiegeGameState.h"
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SpellLineSweep.h" // hero-origin line delivery actor (TASK-236)
#include "Siegebound/SummonedUnit.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"

namespace
{
	/** Team name for log lines (matches the CONVENTIONS Blue/Red contract). */
	const TCHAR* TeamToString(ETeamId Team)
	{
		return (Team == ETeamId::Red) ? TEXT("Red") : TEXT("Blue");
	}

	/**
	 *  Distance from a point to the CLOSEST POINT on the target's collision,
	 *  actor-origin fallback — the ApplyRadialDamage/GetDistanceToTarget
	 *  measurement convention (qa/TASK-026 ruling: large-footprint
	 *  fortifications are "in radius" when the point sits at their wall, not
	 *  only when their origin does). File-local because SiegeCombatStatics.h
	 *  is outside this task's file set; folds into the ONE shared closest-point
	 *  helper on the wave that owns all mirrors (qa/TASK-026 NIT-4 debt —
	 *  flagged in handoffs/TASK-098.md).
	 */
	float DistanceToTargetCollision(const FVector& From, const AActor* Target)
	{
		if (!Target)
		{
			return TNumericLimits<float>::Max();
		}

		FVector ClosestPoint = FVector::ZeroVector;
		const float Distance = Target->ActorGetDistanceToCollision(From, ECC_Pawn, ClosestPoint);
		if (Distance < 0.f)
		{
			// no ECC_Pawn-blocking collision — actor-origin fallback (shared convention)
			return static_cast<float>(FVector::Dist(From, Target->GetActorLocation()));
		}
		return Distance;
	}

	/** All ITeamAgent actors in the world — the same candidate universe as ApplyRadialDamage (AGoldNode deliberately opts out by not implementing it). */
	TArray<AActor*> GatherTeamAgents(UWorld* World)
	{
		TArray<AActor*> TeamAgents;
		UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);
		return TeamAgents;
	}

	/**
	 *  VFX contract (M5 ruling 11): spawn /Game/VFX/NS_Spell_<CardID> at
	 *  SpawnPoint. The soft path is COMPOSED from the CardID (the BP
	 *  soft-class-path law); a missing/mis-named system logs ONCE per CardID
	 *  and the resolve proceeds — VFX can never fail a spell. The log-once
	 *  guard is a function-local static (process lifetime) because a static
	 *  library has no instance to own it — the ApplyTeamMaterial cached-static
	 *  precedent; worst case one extra line per editor session, never spam.
	 *
	 *  TASK-236: SpawnRotation orients the system for the LINE delivery
	 *  (spawned at the muzzle facing the aim — the TASK-238 re-skin reads as a
	 *  hero-fired bolt); the defaulted ZeroRotator keeps every GroundCircle
	 *  call site byte-identical.
	 */
	void SpawnSpellVFX(UWorld* World, FName CardID, const FVector& SpawnPoint, const FRotator& SpawnRotation = FRotator::ZeroRotator)
	{
		const FString AssetName = FString::Printf(TEXT("NS_Spell_%s"), *CardID.ToString());
		const FString ObjectPath = FString::Printf(TEXT("/Game/VFX/%s.%s"), *AssetName, *AssetName);

		UNiagaraSystem* SpellSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(ObjectPath)).LoadSynchronous();
		if (!SpellSystem)
		{
			static TSet<FName> WarnedMissingVFXCardIDs;
			if (!WarnedMissingVFXCardIDs.Contains(CardID))
			{
				WarnedMissingVFXCardIDs.Add(CardID);
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("USpellLibrary: spell VFX '%s' not found — '%s' resolves without VFX (logged once per CardID; TASK-105 authors the systems)."),
					*ObjectPath, *CardID.ToString());
			}
			return;
		}

		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, SpellSystem, SpawnPoint, SpawnRotation);
	}

	/**
	 *  Line-origin resolution (TASK-236, CONVENTIONS "Spell delivery overhaul
	 *  (2026-07-21)"): the caster team's LIVING hero — its capsule center IS
	 *  chest height, no magic offset — else (the bot has no hero: the law's
	 *  flagged design default) the team's standing castle lifted by
	 *  ASpellLineSweep::CastleMuzzleHeight so the line stays at
	 *  unit-capsule-reachable height. Live TActorIterator lookups both sides
	 *  (the player-side precedent from TASK-133 — no cached fallbacks here).
	 *  False = no origin exists (dead hero AND fallen/absent castle) — the
	 *  caller refuses so the confirm path refunds.
	 */
	bool ResolveLineOrigin(UWorld* World, ETeamId CasterTeam, FVector& OutOrigin)
	{
		for (TActorIterator<AHeroCharacter> It(World); It; ++It)
		{
			AHeroCharacter* Hero = *It;
			if (IsValid(Hero) && Hero->GetTeamId() == CasterTeam && !Hero->IsDead())
			{
				OutOrigin = Hero->GetActorLocation();
				return true;
			}
		}

		for (TActorIterator<ACastle> It(World); It; ++It)
		{
			ACastle* Castle = *It;
			if (IsValid(Castle) && Castle->GetTeamId() == CasterTeam && !Castle->IsCastleDestroyed())
			{
				OutOrigin = Castle->GetActorLocation() + FVector(0.f, 0.f, GetDefault<ASpellLineSweep>()->CastleMuzzleHeight);
				return true;
			}
		}

		return false;
	}

	/**
	 *  HeroLine delivery (TASK-236): validates the row for its effect (the
	 *  same malformed-row bars as the GroundCircle resolvers — magnitudes are
	 *  unchanged by delivery; AoERadius deliberately NOT required, the line
	 *  replaces it), resolves the origin and the FLATTENED horizontal aim
	 *  direction (origin → AimPoint), then deferred-spawns an ASpellLineSweep
	 *  (InitLineSweep between spawn and FinishSpawning — the InitUnit
	 *  contract) and spawns the SAME NS_Spell_<CardID> at the MUZZLE with the
	 *  aim rotation (composed-path law — TASK-238 re-skins the content, the
	 *  path never changes). Spawning the sweep IS the successful resolve: a
	 *  line through empty air is a spent spell (the whiffed-Fireball rule —
	 *  no refund for a no-hit line, recorded).
	 */
	bool ResolveHeroLine(UWorld* World, FName CardID, const FCardRow& Row, ETeamId CasterTeam, const FVector& AimPoint)
	{
		// per-effect magnitude validation — mirrors the GroundCircle resolvers
		if (Row.SpellEffect == ESpellEffect::AoEDamage && Row.Damage <= 0.f)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: HeroLine AoEDamage row '%s' malformed (Damage %.1f must be positive) — refused."),
				*CardID.ToString(), Row.Damage);
			return false;
		}
		if (Row.SpellEffect == ESpellEffect::Freeze && Row.EffectDuration <= 0.f)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: HeroLine Freeze row '%s' malformed (EffectDuration %.1f must be positive) — refused."),
				*CardID.ToString(), Row.EffectDuration);
			return false;
		}

		// line geometry comes from the sweep actor's tunables (the ONE home for
		// every line-delivery number) — a non-positive config is a malformed
		// setup, refused like a malformed row
		const ASpellLineSweep* SweepDefaults = GetDefault<ASpellLineSweep>();
		if (!SweepDefaults || SweepDefaults->LineRange <= 0.f || SweepDefaults->LineHalfWidth <= 0.f)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: HeroLine '%s' refused — ASpellLineSweep defaults malformed (LineRange/LineHalfWidth must be positive)."),
				*CardID.ToString());
			return false;
		}

		FVector Origin = FVector::ZeroVector;
		if (!ResolveLineOrigin(World, CasterTeam, Origin))
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: HeroLine '%s' refused for %s — no living hero and no standing castle to fire from."),
				*CardID.ToString(), TeamToString(CasterTeam));
			return false;
		}

		// aim = origin → aim-point, FLATTENED horizontal (the overhaul law: the
		// line lives at the origin's height). Degenerate 2D aim (point directly
		// above/below the origin) cannot make a direction — refused; the PLAYER
		// path pre-checks this free-of-charge in TryConfirmSpellTarget, so a
		// refusal here is caller-independent in practice (bot centroids are
		// never at its castle origin).
		const FVector AimDirection = (AimPoint - Origin).GetSafeNormal2D();
		if (AimDirection.IsNearlyZero())
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: HeroLine '%s' refused — degenerate aim (aim-point (%s) has no horizontal offset from origin (%s))."),
				*CardID.ToString(), *AimPoint.ToCompactString(), *Origin.ToCompactString());
			return false;
		}

		const FTransform SpawnTransform(AimDirection.Rotation(), Origin);
		ASpellLineSweep* Sweep = World->SpawnActorDeferred<ASpellLineSweep>(
			ASpellLineSweep::StaticClass(), SpawnTransform,
			/*Owner=*/ nullptr, /*Instigator=*/ nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn); // no collision on the sweep — always spawns
		if (!Sweep)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: HeroLine '%s' refused — ASpellLineSweep spawn failed."),
				*CardID.ToString());
			return false;
		}
		Sweep->InitLineSweep(CardID, Row, CasterTeam, Origin, AimDirection);
		Sweep->FinishSpawning(SpawnTransform);

		// VFX contract (ruling 11, path UNCHANGED): the same composed
		// NS_Spell_<CardID>, spawned at the MUZZLE with the aim rotation
		// instead of the old impact center — the muzzle/travel/impact seams
		// for TASK-238 (travel seam = the sweep actor's transform).
		SpawnSpellVFX(World, CardID, Origin, AimDirection.Rotation());

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("USpellLibrary: '%s' fired as a hero-origin line for %s — origin (%s), aim (%.2f, %.2f), range %.0f, half-width %.0f (TASK-236)."),
			*CardID.ToString(), TeamToString(CasterTeam), *Origin.ToCompactString(),
			AimDirection.X, AimDirection.Y, SweepDefaults->LineRange, SweepDefaults->LineHalfWidth);
		return true;
	}

	/**
	 *  AoEDamage — Fireball (GDD §4: 100 damage in a 300 radius at the
	 *  reticle). REUSES the TASK-055 radial helper: enemy-only (the Team
	 *  filter is the friendly-fire authority — never the instigator chain),
	 *  closest-point radius, every hit routed through the receiver's own
	 *  TakeDamage tagged USiegeDamageType_Spell — so a blast overlapping the
	 *  castle lands there at 50% (§3.11 acceptance) while units/hero/buildings
	 *  take the listed amount. InstigatorController is null: the resolver has
	 *  no controller, exactly the tower-fired-projectile case the helper
	 *  documents.
	 */
	bool ResolveAoEDamage(UWorld* World, FName CardID, const FCardRow& Row, ETeamId CasterTeam, const FVector& TargetPoint)
	{
		if (Row.Damage <= 0.f || Row.AoERadius <= 0.f)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: AoEDamage row '%s' malformed (Damage %.1f, AoERadius %.1f must be positive) — refused."),
				*CardID.ToString(), Row.Damage, Row.AoERadius);
			return false;
		}

		FSiegeCombatStatics::ApplyRadialDamage(
			World, /*InstigatorController=*/ nullptr, CasterTeam,
			TargetPoint, Row.AoERadius, Row.Damage,
			USiegeDamageType_Spell::StaticClass());
		return true;
	}

	/**
	 *  Freeze — Frost Nova (GDD §4: freezes enemy units and towers in a 350
	 *  radius for 4 s; castle unaffected). Calls TASK-099's pinned
	 *  ApplyFreeze(Seconds) on every enemy ASummonedUnit (miners included —
	 *  they are units) and ABuilding (all subclasses: towers, walls, Barracks,
	 *  Deep Mine — ABuilding::ApplyFreeze is state-only; consumers gate on
	 *  IsFrozen, TASK-101) within row AoERadius. Ruling 5 exclusions fall out
	 *  of the type filter: ACastle is not an ABuilding and AHeroCharacter is
	 *  not an ASummonedUnit, so neither is ever touched. Refresh-not-stack and
	 *  match-end-freeze precedence live inside ApplyFreeze (TASK-099).
	 */
	bool ResolveFreeze(UWorld* World, FName CardID, const FCardRow& Row, ETeamId CasterTeam, const FVector& TargetPoint)
	{
		if (Row.EffectDuration <= 0.f || Row.AoERadius <= 0.f)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: Freeze row '%s' malformed (EffectDuration %.1f, AoERadius %.1f must be positive) — refused."),
				*CardID.ToString(), Row.EffectDuration, Row.AoERadius);
			return false;
		}

		int32 FrozenCount = 0;
		for (AActor* Candidate : GatherTeamAgents(World))
		{
			if (!IsValid(Candidate))
			{
				continue;
			}

			// enemies only (§3.0 no friendly fire; native cast valid — UTeamAgent is NotBlueprintable)
			const ITeamAgent* Agent = Cast<ITeamAgent>(Candidate);
			if (!Agent || Agent->GetTeamId() == CasterTeam)
			{
				continue;
			}

			if (DistanceToTargetCollision(TargetPoint, Candidate) > Row.AoERadius)
			{
				continue;
			}

			if (ASummonedUnit* Unit = Cast<ASummonedUnit>(Candidate))
			{
				if (!Unit->IsUnitDead())
				{
					Unit->ApplyFreeze(Row.EffectDuration);
					++FrozenCount;
				}
			}
			else if (ABuilding* Building = Cast<ABuilding>(Candidate))
			{
				if (!Building->IsBuildingDestroyed())
				{
					Building->ApplyFreeze(Row.EffectDuration);
					++FrozenCount;
				}
			}
			// every other ITeamAgent (castle, hero) is excluded by ruling 5 — no branch on purpose
		}

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("USpellLibrary: '%s' froze %d enemy actor(s) for %.1fs within %.0f of (%s)."),
			*CardID.ToString(), FrozenCount, Row.EffectDuration, Row.AoERadius, *TargetPoint.ToCompactString());
		return true;
	}

	/**
	 *  TopTargetsDamage — Lightning (GDD §4: 200 damage to the 3 highest-HP
	 *  enemies in a 700 radius; "the tower-killer"). Ruling 4 selection: the
	 *  row MaxTargets highest CURRENT-HP enemy actors — units, hero,
	 *  buildings/towers; castle EXCLUDED (anti-sniping) — within row AoERadius
	 *  of the reticle, ties broken by distance to the reticle (deterministic:
	 *  stable sort on HP desc, then distance asc). Each takes row Damage
	 *  tagged Spell; buildings take it FULL (the 50% branch is castle-only),
	 *  so Lightning 200 kills an Arrow Tower at 150.
	 */
	bool ResolveTopTargetsDamage(UWorld* World, FName CardID, const FCardRow& Row, ETeamId CasterTeam, const FVector& TargetPoint)
	{
		if (Row.Damage <= 0.f || Row.MaxTargets <= 0 || Row.AoERadius <= 0.f)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: TopTargetsDamage row '%s' malformed (Damage %.1f, MaxTargets %d, AoERadius %.1f must be positive) — refused."),
				*CardID.ToString(), Row.Damage, Row.MaxTargets, Row.AoERadius);
			return false;
		}

		// gather live enemy candidates with their CURRENT HP (ruling 4: current, not MaxHP —
		// Slayer's MaxHP gate is a different concept) and reticle distance
		struct FStrikeCandidate
		{
			AActor* TargetActor = nullptr;
			float TargetCurrentHP = 0.f;
			float ReticleDistance = 0.f;
		};
		TArray<FStrikeCandidate> Candidates;

		for (AActor* Candidate : GatherTeamAgents(World))
		{
			if (!IsValid(Candidate))
			{
				continue;
			}

			const ITeamAgent* Agent = Cast<ITeamAgent>(Candidate);
			if (!Agent || Agent->GetTeamId() == CasterTeam)
			{
				continue; // friendlies never selected (§3.0)
			}

			// castle EXCLUDED from selection entirely (ruling 4 anti-sniping intent)
			if (Candidate->IsA<ACastle>())
			{
				continue;
			}

			// live combatants only, with per-type current HP (the ruling's selection metric)
			float CandidateCurrentHP = 0.f;
			if (const ASummonedUnit* Unit = Cast<ASummonedUnit>(Candidate))
			{
				if (Unit->IsUnitDead())
				{
					continue;
				}
				CandidateCurrentHP = Unit->GetCurrentHP();
			}
			else if (const ABuilding* Building = Cast<ABuilding>(Candidate))
			{
				if (Building->IsBuildingDestroyed())
				{
					continue;
				}
				CandidateCurrentHP = Building->GetCurrentHP();
			}
			else if (const AHeroCharacter* Hero = Cast<AHeroCharacter>(Candidate))
			{
				if (Hero->IsDead())
				{
					continue;
				}
				CandidateCurrentHP = Hero->GetCurrentHP();
			}
			else
			{
				continue; // unknown ITeamAgent type — never a strike target
			}

			const float ReticleDist = DistanceToTargetCollision(TargetPoint, Candidate);
			if (ReticleDist > Row.AoERadius)
			{
				continue;
			}

			Candidates.Add({ Candidate, CandidateCurrentHP, ReticleDist });
		}

		// highest CURRENT HP first; ties by distance to the reticle (ruling 4 — deterministic).
		// StableSort so equal-key candidates keep a consistent relative order.
		Candidates.StableSort([](const FStrikeCandidate& A, const FStrikeCandidate& B)
		{
			if (A.TargetCurrentHP != B.TargetCurrentHP)
			{
				return A.TargetCurrentHP > B.TargetCurrentHP;
			}
			return A.ReticleDistance < B.ReticleDistance;
		});

		const int32 StrikeCount = FMath::Min(Row.MaxTargets, Candidates.Num());
		for (int32 Index = 0; Index < StrikeCount; ++Index)
		{
			// null instigator/causer on purpose: the enemy-only filter above is the
			// friendly-fire authority (the ApplyRadialDamage design); receivers that
			// resolve no team apply the damage (TASK-002 chain).
			UGameplayStatics::ApplyDamage(
				Candidates[Index].TargetActor, Row.Damage,
				/*EventInstigator=*/ nullptr, /*DamageCauser=*/ nullptr,
				USiegeDamageType_Spell::StaticClass());
		}

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("USpellLibrary: '%s' struck %d of %d candidate(s) for %.0f each within %.0f of (%s)."),
			*CardID.ToString(), StrikeCount, Candidates.Num(), Row.Damage, Row.AoERadius, *TargetPoint.ToCompactString());
		return true;
	}

	/**
	 *  AllyBuff — Battle Cry (GDD §4: friendly units in a 400 radius get +50%
	 *  attack speed, +25% move speed for 8 s). Calls TASK-099's pinned
	 *  ApplyCombatBuff(MoveSpeedMult, AttackSpeedMult, Seconds) on every
	 *  FRIENDLY live ASummonedUnit within row AoERadius (miners included —
	 *  they are units; the attack-speed term is inert on a non-attacker).
	 *  The magnitudes are the UNIT'S OWN BattleCry* mechanic UPROPERTYs
	 *  (M5 ruling 6 — the API owner is the UPROPERTY-capable home), read
	 *  PER-UNIT via GetBattleCryMoveSpeedMultiplier() /
	 *  GetBattleCryAttackSpeedMultiplier() exactly as the SummonedUnit.h doc
	 *  pins the composition — the resolver hardcodes nothing.
	 *  The hero is NOT a unit and is never buffed. Self-refresh non-stacking
	 *  and the Rally/War Banner independence (ruling 6) live inside
	 *  ApplyCombatBuff (TASK-099).
	 */
	bool ResolveAllyBuff(UWorld* World, FName CardID, const FCardRow& Row, ETeamId CasterTeam, const FVector& TargetPoint)
	{
		if (Row.EffectDuration <= 0.f || Row.AoERadius <= 0.f)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: AllyBuff row '%s' malformed (EffectDuration %.1f, AoERadius %.1f must be positive) — refused."),
				*CardID.ToString(), Row.EffectDuration, Row.AoERadius);
			return false;
		}

		int32 BuffedCount = 0;
		for (AActor* Candidate : GatherTeamAgents(World))
		{
			if (!IsValid(Candidate))
			{
				continue;
			}

			ASummonedUnit* Unit = Cast<ASummonedUnit>(Candidate);
			if (!Unit || Unit->GetTeamId() != CasterTeam || Unit->IsUnitDead())
			{
				continue; // friendly live UNITS only (GDD §4 "friendly units")
			}

			if (DistanceToTargetCollision(TargetPoint, Unit) > Row.AoERadius)
			{
				continue;
			}

			// per-unit magnitudes (M5 ruling 6): the unit's own BattleCry* mechanic
			// UPROPERTYs, read through its accessors — the SummonedUnit.h pinned composition.
			Unit->ApplyCombatBuff(Unit->GetBattleCryMoveSpeedMultiplier(), Unit->GetBattleCryAttackSpeedMultiplier(), Row.EffectDuration);
			++BuffedCount;
		}

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("USpellLibrary: '%s' buffed %d friendly unit(s) (each unit's own BattleCry multipliers) for %.1fs within %.0f of (%s)."),
			*CardID.ToString(), BuffedCount,
			Row.EffectDuration, Row.AoERadius, *TargetPoint.ToCompactString());
		return true;
	}

	/**
	 *  GoldSteal — Pickpocket (GDD §4: steal 10 gold from the opponent, up to
	 *  what they have; ruling 7: instant global effect). Steal = min(row
	 *  GoldSteal, victim's gold), moved EXCLUSIVELY through the existing
	 *  ASiegePlayerState choke-pointed gold APIs: SpendGold(Steal) on the
	 *  victim + AddGold(Steal) on the caster (SetGold choke-point law — the
	 *  clamp and OnGoldChanged broadcast are honored on both sides; the exact
	 *  composition is documented in handoffs/TASK-098.md). Player states
	 *  resolve via ASiegeGameState::GetPlayerStateForTeam (TASK-043); an
	 *  unresolvable side (Sandbox mode has no Red economy) REFUSES so the
	 *  caller refunds. A 0-gold victim still resolves — the spell stole
	 *  everything they had (nothing), the whiffed-Fireball rule.
	 */
	bool ResolveGoldSteal(UWorld* World, FName CardID, const FCardRow& Row, ETeamId CasterTeam)
	{
		if (Row.GoldSteal <= 0)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: GoldSteal row '%s' malformed (GoldSteal %d must be positive) — refused."),
				*CardID.ToString(), Row.GoldSteal);
			return false;
		}

		ASiegeGameState* GameState = World->GetGameState<ASiegeGameState>();
		if (!GameState)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: GoldSteal '%s' refused — no ASiegeGameState in this world."),
				*CardID.ToString());
			return false;
		}

		const ETeamId VictimTeam = (CasterTeam == ETeamId::Blue) ? ETeamId::Red : ETeamId::Blue;
		ASiegePlayerState* CasterState = GameState->GetPlayerStateForTeam(CasterTeam);
		ASiegePlayerState* VictimState = GameState->GetPlayerStateForTeam(VictimTeam);
		if (!CasterState || !VictimState)
		{
			// GetPlayerStateForTeam already logged which side is missing (normal in
			// Sandbox mode — no Red economy exists). Refuse -> caller refunds.
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("USpellLibrary: GoldSteal '%s' refused — no ASiegePlayerState for %s (caster) and/or %s (victim)."),
				*CardID.ToString(), TeamToString(CasterTeam), TeamToString(VictimTeam));
			return false;
		}

		const int32 StolenGold = FMath::Min(Row.GoldSteal, VictimState->GetGold());
		if (StolenGold > 0)
		{
			// choke-point composition: SpendGold clamps/refuses per its own contract —
			// it cannot fail here (StolenGold <= victim gold by the min above), but the
			// guard keeps the caster grant strictly conditional on the victim deduction.
			if (!VictimState->SpendGold(StolenGold))
			{
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("USpellLibrary: GoldSteal '%s' refused — victim SpendGold(%d) unexpectedly failed."),
					*CardID.ToString(), StolenGold);
				return false;
			}
			CasterState->AddGold(StolenGold);
		}

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("USpellLibrary: '%s' stole %d gold (requested %d) from %s for %s."),
			*CardID.ToString(), StolenGold, Row.GoldSteal, TeamToString(VictimTeam), TeamToString(CasterTeam));
		return true;
	}
} // namespace

bool USpellLibrary::ResolveSpell(UWorld* World, FName CardID, const FCardRow& Row, ETeamId CasterTeam, const FVector& TargetPoint)
{
	// null-safe everywhere (spec 4): bad input logs and refuses — never a crash.
	if (!World)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("USpellLibrary::ResolveSpell: null World for CardID '%s' — refused."), *CardID.ToString());
		return false;
	}
	if (CardID.IsNone())
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("USpellLibrary::ResolveSpell: CardID is None (caller bug — the CardID names the VFX and the row) — refused."));
		return false;
	}

	// TASK-236 delivery dispatch: HeroLine spells (AoEDamage/Freeze under the
	// Auto default — Fireball + FrostNova) treat TargetPoint as the AIM-POINT
	// and fire a line from the caster; the line path spawns its OWN VFX at the
	// muzzle, so the shared spawn below is skipped for it. Every other effect
	// (and any explicit GroundCircle cell) runs the M5 paths byte-untouched.
	bool bResolved = false;
	bool bLineDelivery = false;
	switch (Row.SpellEffect)
	{
	case ESpellEffect::AoEDamage:
		if (USpellLibrary::GetEffectiveDelivery(Row) == ESpellDelivery::HeroLine)
		{
			bLineDelivery = true;
			bResolved = ResolveHeroLine(World, CardID, Row, CasterTeam, TargetPoint);
		}
		else
		{
			bResolved = ResolveAoEDamage(World, CardID, Row, CasterTeam, TargetPoint);
		}
		break;
	case ESpellEffect::Freeze:
		if (USpellLibrary::GetEffectiveDelivery(Row) == ESpellDelivery::HeroLine)
		{
			bLineDelivery = true;
			bResolved = ResolveHeroLine(World, CardID, Row, CasterTeam, TargetPoint);
		}
		else
		{
			bResolved = ResolveFreeze(World, CardID, Row, CasterTeam, TargetPoint);
		}
		break;
	case ESpellEffect::TopTargetsDamage:
		bResolved = ResolveTopTargetsDamage(World, CardID, Row, CasterTeam, TargetPoint);
		break;
	case ESpellEffect::AllyBuff:
		bResolved = ResolveAllyBuff(World, CardID, Row, CasterTeam, TargetPoint);
		break;
	case ESpellEffect::GoldSteal:
		// instant global effect (ruling 7): TargetPoint plays no gameplay role —
		// it is only the VFX anchor the caller chose.
		bResolved = ResolveGoldSteal(World, CardID, Row, CasterTeam);
		break;
	case ESpellEffect::None:
	default:
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("USpellLibrary::ResolveSpell: row '%s' carries no resolvable SpellEffect — refused (caller refunds)."),
			*CardID.ToString());
		return false;
	}

	if (!bResolved)
	{
		return false; // per-effect resolver already logged the reason; caller refunds
	}

	// ruling 11: EVERY successful resolve spawns /Game/VFX/NS_Spell_<CardID> —
	// null-safe, log-once; a missing system never fails the spell. HeroLine
	// resolves already spawned theirs at the MUZZLE with the aim rotation
	// (TASK-236) — exactly one spawn per resolve either way.
	if (!bLineDelivery)
	{
		SpawnSpellVFX(World, CardID, TargetPoint);
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("USpellLibrary: resolved '%s' for %s at (%s)."),
		*CardID.ToString(), TeamToString(CasterTeam), *TargetPoint.ToCompactString());
	return true;
}

ESpellDelivery USpellLibrary::GetEffectiveDelivery(const FCardRow& Row)
{
	switch (Row.SpellDelivery)
	{
	case ESpellDelivery::GroundCircle:
		return ESpellDelivery::GroundCircle;
	case ESpellDelivery::HeroLine:
		return ESpellDelivery::HeroLine;
	case ESpellDelivery::Auto:
	default:
		// the 2026-07-21 directive's per-effect default: AoEDamage + Freeze —
		// today exactly Fireball and FrostNova — deliver as hero-origin lines;
		// every other effect keeps the M5 ground circle. An explicit cell (the
		// cases above) is the per-card data override lever.
		return (Row.SpellEffect == ESpellEffect::AoEDamage || Row.SpellEffect == ESpellEffect::Freeze)
			? ESpellDelivery::HeroLine
			: ESpellDelivery::GroundCircle;
	}
}

bool USpellLibrary::IsLineDeliverySpell(const FCardRow& Row)
{
	return GetEffectiveDelivery(Row) == ESpellDelivery::HeroLine;
}
