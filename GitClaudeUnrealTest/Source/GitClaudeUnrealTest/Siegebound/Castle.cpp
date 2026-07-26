// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/Castle.h"

#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GitClaudeUnrealTest.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "Siegebound/CastleHealthBarWidget.h"
#include "Siegebound/DamageTypes.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegeHitFlashComponent.h"

namespace
{
	//~ M7 §6 juice/feel soft-ref paths (TASK-157/158/179) — null-safe (art arrives in TASK-171-adjacent/174/180).
	const TCHAR* CastleHitSoundPath = TEXT("/Game/Audio/S_CastleHit");             // TASK-179
	const TCHAR* CastleDestroyedSoundPath = TEXT("/Game/Audio/S_CastleDestroyed"); // TASK-179
	const TCHAR* CastleDebrisVFXPath = TEXT("/Game/VFX/NS_CastleDebris");          // TASK-157 debris burst

	/** Height above the castle origin for its floating damage number (clears the ~900-tall mesh, HP-bar Z parity). */
	constexpr float CastleDamageNumberHeightZ = 1050.f;
}

ACastle::ACastle()
{
	// Pure event-driven objective — nothing to tick.
	PrimaryActorTick.bCanEverTick = false;

	CastleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CastleMesh"));
	SetRootComponent(CastleMesh);
	// Explicit blocking profile (QA TASK-002 WARN, pre-approved fix): TASK-004's unit
	// targeting/blocking contract must not rest on the engine's implicit default.
	CastleMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// Overhead HP bar (playtest R1 finding 2, TASK-018). Screen space so it reads at
	// any camera angle/distance; relative Z +1050 clears the 900-tall castle mesh
	// (TASK-013). The widget CLASS is soft-resolved at BeginPlay (WBP_CastleHealthBar
	// arrives in TASK-019); the bare component always exists and draws nothing.
	HPBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidget"));
	HPBarWidget->SetupAttachment(CastleMesh);
	HPBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidget->SetDrawSize(FVector2D(256.0f, 32.0f));
	HPBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 1050.0f));
	// UI-only component: never collides, never blocks traces (placement cursor
	// trace TASK-007, unit acquisition TASK-004).
	HPBarWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// §6 hit-flash (TASK-154): overlay-based white flash on every actual damage event,
	// driven from TakeDamage. Overlay (not slot-swap) so it composes cleanly with the
	// TASK-157 crumble MI swap on this same mesh. Null-safe.
	HitFlashComponent = CreateDefaultSubobject<USiegeHitFlashComponent>(TEXT("HitFlashComponent"));

	// §6 castle-hit screen shake donor (TASK-158): the READ-ONLY Variant_Combat
	// BP_CameraShake_Hit_Enemy (CONVENTIONS template-donor rule). Soft, null-safe; a BP
	// may retarget it to a dedicated BP_CameraShake_CastleHit.
	CastleHitCameraShake = TSoftClassPtr<UCameraShakeBase>(FSoftObjectPath(TEXT("/Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy.BP_CameraShake_Hit_Enemy_C")));

	// Content contract (TASKBOARD TASK-002 names block / CONVENTIONS.md). These assets are
	// produced in parallel (TASK-013 mesh, TASK-012 materials) and are resolved null-safe
	// in OnConstruction — a missing asset must never crash.
	CastleMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Meshes/SM_Castle.SM_Castle")));
	TeamMaterialBlue = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue")));
	TeamMaterialRed = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/Instances/MI_TeamColor_Red.MI_TeamColor_Red")));
	// TASK-018 names block: widget asset built in TASK-019 — resolved null-safe at BeginPlay.
	HPBarWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_CastleHealthBar.WBP_CastleHealthBar_C")));
}

void ACastle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyTeamVisuals();
}

void ACastle::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = MaxHP;
	bDestroyed = false;

	// Seed broadcast (TASK-018 spec): listeners that bound before BeginPlay
	// (level BP, game framework) start from the boot values without polling.
	OnCastleHPChanged.Broadcast(CurrentHP, MaxHP);

	InitHPBarWidget();
}

void ACastle::InitHPBarWidget()
{
	if (!HPBarWidget)
	{
		return;
	}

	// WBP_CastleHealthBar is built in TASK-019 and may not exist yet — a missing
	// class is a SILENT no-op per spec (LoadSynchronous returns nullptr for unset
	// paths and absent assets alike; the bare component simply draws nothing).
	UClass* LoadedWidgetClass = HPBarWidgetClass.LoadSynchronous();
	if (!LoadedWidgetClass)
	{
		return;
	}

	// Post-BeginPlay SetWidgetClass triggers the component's InitWidget, creating
	// the user widget instance (components have begun play — Super::BeginPlay ran
	// before this is called).
	HPBarWidget->SetWidgetClass(LoadedWidgetClass);

	// Seed-then-bind is the widget's own job (qa/TASK-005-report.md major 2):
	// InitForCastle pushes the current values FIRST, then binds OnCastleHPChanged.
	// A widget of some other class (mis-authored TASK-019 asset) is skipped, not a crash.
	if (UCastleHealthBarWidget* HealthBar = Cast<UCastleHealthBarWidget>(HPBarWidget->GetWidget()))
	{
		HealthBar->InitForCastle(this);
	}
}

void ACastle::ApplyTeamVisuals()
{
	if (!CastleMesh)
	{
		return;
	}

	// LoadSynchronous() returns nullptr for unset paths and not-yet-imported assets alike;
	// in either case we simply skip the assignment (never crash per spec).
	if (UStaticMesh* Mesh = CastleMeshAsset.LoadSynchronous())
	{
		if (CastleMesh->GetStaticMesh() != Mesh)
		{
			CastleMesh->SetStaticMesh(Mesh);
		}
	}

	const TSoftObjectPtr<UMaterialInterface>& TeamMaterial = (Team == ETeamId::Red) ? TeamMaterialRed : TeamMaterialBlue;
	if (UMaterialInterface* Material = TeamMaterial.LoadSynchronous())
	{
		// SM_Castle has a single material slot (TASK-013 spec); the same mesh serves both teams.
		CastleMesh->SetMaterial(0, Material);
	}
}

float ACastle::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// A destroyed castle absorbs nothing further; the destroyed event can never re-fire.
	if (bDestroyed || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	// No friendly fire (GDD §3.0): ignore damage whose instigator is on our own team.
	ETeamId InstigatorTeam = ETeamId::Blue;
	if (TryGetInstigatorTeam(EventInstigator, DamageCauser, InstigatorTeam) && InstigatorTeam == Team)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage <= 0.0f)
	{
		return 0.0f;
	}

	// Damage-vs-fortification scaling (GDD §3.0), read from the damage TYPE.
	// USiegeDamageType_Siege — and any subclass — applies at 200% (Siege units,
	// Ogre/Sapper, batter the castle, TASK-054); USiegeDamageType_Projectile at
	// 50% (the anti-sniping rule); USiegeDamageType_Spell at 50% (GDD §3.11 /
	// M5 ruling 3, TASK-098 — CASTLE ONLY: it mirrors the M2 Projectile
	// precedent, NOT the M4 Siege both-rule, so ABuilding takes FULL spell
	// damage and Lightning at 200 kills an Arrow Tower at 150);
	// melee/default/untyped at 100% (melee needs no tag, CONVENTIONS
	// damage-type registry — M1 attackers pass base UDamageType and stay
	// byte-identical). Siege, Projectile, and Spell are mutually disjoint
	// types, so the branch order is irrelevant. Units and the hero take listed
	// damage from everything (scaling is castle/building-only).
	float ScaledDamage = ActualDamage;
	const UClass* IncomingDamageType = DamageEvent.DamageTypeClass.Get();
	if (IncomingDamageType && IncomingDamageType->IsChildOf(USiegeDamageType_Siege::StaticClass()))
	{
		ScaledDamage *= 2.0f;
	}
	else if (IncomingDamageType && IncomingDamageType->IsChildOf(USiegeDamageType_Projectile::StaticClass()))
	{
		ScaledDamage *= 0.5f;
	}
	else if (IncomingDamageType && IncomingDamageType->IsChildOf(USiegeDamageType_Spell::StaticClass()))
	{
		ScaledDamage *= 0.5f;
	}

	CurrentHP = FMath::Max(CurrentHP - ScaledDamage, 0.0f);

	// Actual HP change -> broadcast (TASK-018). Ignored friendly fire and hits on a
	// destroyed castle returned above WITHOUT touching CurrentHP, so a broadcast here
	// always reports a real change (CurrentHP > 0 and ScaledDamage > 0 guarantee the
	// clamp lowered the value — half of a positive float is still positive). Fired
	// BEFORE HandleDestroyed so listeners see the 0-HP value before the destroyed
	// event hides the bar.
	OnCastleHPChanged.Broadcast(CurrentHP, MaxHP);

	// §3.9 crumble (TASK-157): advance the 75/50/25% stages on the way down (each once,
	// in order). Runs before the §6 feedback below; the overlay hit-flash sits on TOP of
	// whatever crumble material is now on the mesh, so the two never fight.
	UpdateCrumbleStages();

	// §6 castle damage feedback (TASK-154/156/158/179) — ACTUAL damage only (friendly
	// fire + destroyed-castle hits returned above). All null-safe until the art/audio land.
	if (HitFlashComponent)
	{
		HitFlashComponent->TriggerFlash();
	}
	USiegeFeedbackLibrary::ShowDamageNumber(this, ScaledDamage,
		GetActorLocation() + FVector(0.0f, 0.0f, CastleDamageNumberHeightZ), USiegeFeedbackLibrary::TeamTint(Team));
	USiegeFeedbackLibrary::PlayWorldSound(this, CastleHitSoundPath, GetActorLocation());
	// screen shake <= 0.2 s on the LOCAL player controller (TASK-158): resolve the soft
	// shake class null-safe, then the library kicks the index-0 controller.
	if (UClass* ShakeClass = CastleHitCameraShake.LoadSynchronous())
	{
		USiegeFeedbackLibrary::PlayLocalCameraShake(this, ShakeClass);
	}

	if (CurrentHP <= 0.0f)
	{
		HandleDestroyed();
	}

	// AActor contract: return the damage actually applied — the SCALED amount the
	// castle really took. Melee returns exactly the M1 value; attacker hit feedback
	// (TASK-016/020 puff-on-damage checks) keys off > 0 and is unaffected either way.
	return ScaledDamage;
}

void ACastle::HandleDestroyed()
{
	// Single-fire guard: cumulative overkill, duplicate calls, or re-entrancy
	// during the broadcast can never fire the event twice.
	if (bDestroyed)
	{
		return;
	}
	bDestroyed = true;

	// A fallen castle stops any in-progress Masons repair (TASK-059) — nothing
	// heals a destroyed objective, and the timer must not tick on a hidden actor.
	StopHealOverTime();

	// Hide the mesh and stop colliding (GDD §3.9) BEFORE broadcasting, so any
	// listener querying this castle during the event already sees it destroyed.
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	// Screen-space widget components do NOT follow actor hidden-in-game state
	// (the viewport layer checks component visibility only) — hide explicitly
	// (TASK-018: bar disappears with the castle).
	if (HPBarWidget)
	{
		HPBarWidget->SetVisibility(false, /*bPropagateToChildren=*/true);
	}

	// §6 castle-destroyed stinger (TASK-179): a 2D one-shot (the win/loss moment).
	// Null-safe until S_CastleDestroyed lands (TASK-180).
	USiegeFeedbackLibrary::PlaySound2D(this, CastleDestroyedSoundPath);

	OnCastleDestroyed.Broadcast(this, Team);
}

void ACastle::UpdateCrumbleStages()
{
	// §3.9 crumble (TASK-157): advance while the current HP fraction has crossed the NEXT
	// stage's threshold, firing each stage exactly once IN ORDER. A single big hit that
	// crosses 75% and 50% in one blow fires stage 1 then stage 2 this call. CrumbleStage is
	// monotonic (never retreats), so a Masons heal-back-up never un-crumbles or re-arms a
	// passed stage — only ResetCastle re-arms (CrumbleStage = 0). No-op with a zero MaxHP.
	if (MaxHP <= 0.0f || CrumbleStage >= 3)
	{
		return;
	}

	const float Fraction = CurrentHP / MaxHP;
	while (CrumbleStage < 3)
	{
		const int32 NextStage = CrumbleStage + 1;
		const float NextThreshold = (NextStage == 1) ? CrumbleFraction1 : (NextStage == 2) ? CrumbleFraction2 : CrumbleFraction3;
		if (Fraction <= NextThreshold)
		{
			CrumbleStage = NextStage;
			ApplyCrumbleStage(NextStage);
		}
		else
		{
			break;
		}
	}
}

void ACastle::ApplyCrumbleStage(int32 Stage)
{
	if (Stage < 1 || Stage > 3 || !CastleMesh)
	{
		return;
	}

	// Soft, null-safe (composed per stage, resolved via the feedback library's cached
	// log-once resolvers). A missing mesh/material keeps the current look — the debris
	// still bursts so the stage always READS even before the swap art lands (TASK-171/174).
	const FString CrumbleMeshPath = FString::Printf(TEXT("/Game/Meshes/SM_Castle_Crumble0%d"), Stage);
	const FString CrumbleMaterialPath = FString::Printf(TEXT("/Game/Materials/MI_Castle_Crumble0%d"), Stage);

	if (UStaticMesh* CrumbleMesh = USiegeFeedbackLibrary::ResolveStaticMesh(CrumbleMeshPath))
	{
		// VISUAL swap only — the crumble mesh variants must preserve the castle's UCX
		// footprint (art contract, handoff), so collision/placement/pathing are untouched.
		CastleMesh->SetStaticMesh(CrumbleMesh);
	}
	if (UMaterialInterface* CrumbleMaterial = USiegeFeedbackLibrary::ResolveMaterial(CrumbleMaterialPath))
	{
		CastleMesh->SetMaterial(0, CrumbleMaterial);
	}

	// debris burst at the castle (soft, null-safe until NS_CastleDebris lands).
	USiegeFeedbackLibrary::SpawnNiagara(this, CastleDebrisVFXPath, GetActorLocation());

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ACastle '%s': crumble stage %d (%.0f%% HP threshold, GDD §3.9) — mesh/material swap + debris (visual only; footprint unchanged)."),
		*GetNameSafe(this), Stage, ((Stage == 1) ? CrumbleFraction1 : (Stage == 2) ? CrumbleFraction2 : CrumbleFraction3) * 100.0f);
}

void ACastle::ResetCastle()
{
	// Play Again (GDD §3.9): cancel any in-progress Masons repair (TASK-059) first,
	// then back to full HP, visible, solid, and armed to fire again.
	StopHealOverTime();
	bDestroyed = false;
	CurrentHP = MaxHP;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	// §3.9 crumble reset (TASK-157): back to stage 0 and restore the pristine SM_Castle +
	// team material (undo any crumble mesh/material swap), re-arming all thresholds.
	CrumbleStage = 0;
	ApplyTeamVisuals();

	// Counterpart of the HandleDestroyed hide — the bar returns with the castle (TASK-018).
	if (HPBarWidget)
	{
		HPBarWidget->SetVisibility(true, /*bPropagateToChildren=*/true);
	}

	// Reset-path broadcast (TASK-018 + CONVENTIONS delegate rules): the bar refills
	// to MaxHP/MaxHP. Unconditional by spec — resetting an undamaged castle still
	// notifies (reset is an explicit reset event, not a suppressed no-op mutation).
	OnCastleHPChanged.Broadcast(CurrentHP, MaxHP);
}

bool ACastle::IsPointInSpawnBox(const FVector& Point) const
{
	// Castle-centered 2D square test (Z ignored). Additive third reader of the (840,840)
	// paired-tunable — does NOT touch the bot's IsPointInBotSpawnBox (TASK-262). W1 TASK-275.
	const FVector Origin = GetActorLocation();
	return FMath::Abs(Point.X - Origin.X) <= SpawnBoxHalfExtent.X
		&& FMath::Abs(Point.Y - Origin.Y) <= SpawnBoxHalfExtent.Y;
}

bool ACastle::TryGetInstigatorTeam(AController* EventInstigator, AActor* DamageCauser, ETeamId& OutTeam)
{
	// 1) The instigating controller's pawn (hero melee reports its controller).
	if (EventInstigator)
	{
		if (const ITeamAgent* Agent = Cast<ITeamAgent>(EventInstigator->GetPawn()))
		{
			OutTeam = Agent->GetTeamId();
			return true;
		}
	}

	// 2) The damage causer itself (summoned units apply damage directly).
	if (const ITeamAgent* Agent = Cast<ITeamAgent>(DamageCauser))
	{
		OutTeam = Agent->GetTeamId();
		return true;
	}

	// 3) The causer's instigator pawn (covers projectiles once M2 adds them).
	if (DamageCauser)
	{
		if (const ITeamAgent* Agent = Cast<ITeamAgent>(DamageCauser->GetInstigator()))
		{
			OutTeam = Agent->GetTeamId();
			return true;
		}
	}

	// No team could be resolved (e.g. world/kill-Z damage) — caller applies the damage.
	return false;
}

void ACastle::HealOverTime(float Total, float Duration)
{
	// A destroyed castle absorbs no repair; a non-positive amount/duration is a
	// caller error (never scheduled — matches the §3.0 "no free effect" discipline).
	if (bDestroyed)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ACastle '%s': HealOverTime ignored — castle is destroyed."), *GetNameSafe(this));
		return;
	}
	if (Total <= 0.0f || Duration <= 0.0f)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ACastle '%s': HealOverTime(%.1f over %.1fs) ignored — Total and Duration must be positive."),
			*GetNameSafe(this), Total, Duration);
		return;
	}

	// Add to the running pool and (re)derive the per-tick delivery so the WHOLE
	// remaining pool lands over Duration — a second Masons reinforces the stream
	// (restack) rather than replacing it (TASK-059).
	HealRemaining += Total;
	const float TicksOverDuration = FMath::Max(Duration / HealTickInterval, 1.0f);
	HealPerTick = HealRemaining / TicksOverDuration;

	// Already at full? Nothing to deliver (never over MaxHP) — drop the pool.
	if (CurrentHP >= MaxHP)
	{
		StopHealOverTime();
		return;
	}

	// (Re)arm the repeating tick; SetTimer replaces on the same handle (no stacking).
	GetWorldTimerManager().SetTimer(HealTimerHandle, this, &ACastle::HandleHealTick, HealTickInterval, /*bLoop=*/ true);

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ACastle '%s': repairing %.0f HP over %.1fs (%.1f/tick every %.2fs; %.0f/%.0f HP now)."),
		*GetNameSafe(this), Total, Duration, HealPerTick, HealTickInterval, CurrentHP, MaxHP);
}

void ACastle::HandleHealTick()
{
	// Stop the stream if the castle fell, the pool emptied, or HP is already full.
	if (bDestroyed || HealRemaining <= 0.0f || CurrentHP >= MaxHP)
	{
		StopHealOverTime();
		return;
	}

	const float Delta = FMath::Min(HealPerTick, HealRemaining);
	const float NewHP = FMath::Min(CurrentHP + Delta, MaxHP); // clamp — never over MaxHP
	const float Applied = NewHP - CurrentHP;
	CurrentHP = NewHP;
	HealRemaining = FMath::Max(HealRemaining - Delta, 0.0f);

	// Broadcast only on an ACTUAL change (mirrors TakeDamage's real-change contract).
	if (Applied > 0.0f)
	{
		OnCastleHPChanged.Broadcast(CurrentHP, MaxHP);
	}

	if (HealRemaining <= 0.0f || CurrentHP >= MaxHP)
	{
		StopHealOverTime();
	}
}

void ACastle::StopHealOverTime()
{
	GetWorldTimerManager().ClearTimer(HealTimerHandle);
	HealRemaining = 0.0f;
	HealPerTick = 0.0f;
}
