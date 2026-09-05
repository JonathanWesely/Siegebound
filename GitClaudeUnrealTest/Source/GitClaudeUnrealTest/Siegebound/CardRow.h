// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/SoftObjectPtr.h"
#include "CardRow.generated.h"

class UTexture2D;

/**
 *  Card category (GDD section 4). M1 uses Unit only; the rest are defined
 *  up front so cards.csv can grow without struct changes.
 */
UENUM(BlueprintType)
enum class ECardType : uint8
{
	Unit,
	Building,
	Economy,
	Spell,
	HeroUpgrade,
	Utility
};

/**
 *  Unit AI/behavior profile (GDD section 3.8). None for non-unit cards.
 */
UENUM(BlueprintType)
enum class ECardProfile : uint8
{
	None,
	Standard,
	Siege,
	Support
};

/**
 *  Spell resolution dispatch token (GDD section 3.11 / section 4 Set III; M5,
 *  CONVENTIONS "Spells & Set III (M5)"). None for non-spell cards.
 *  USpellLibrary::ResolveSpell (TASK-098) dispatches on this column; CSV cells
 *  use these value names character-for-character.
 *
 *  ⛔⛔ APPEND-ONLY, AND IT IS A SERIALISATION CONTRACT RATHER THAN A STYLE RULE.
 *  This is a reflected `uint8` UENUM held as a UPROPERTY by FCardRow below, and
 *  FCardRow is the row type of the SAVED asset /Game/Data/DT_Cards. Inserting a
 *  value in the MIDDLE renumbers every value after it, so every already-saved
 *  cell (and every Blueprint pin holding one) silently re-reads as the NEXT
 *  effect along: a Pickpocket row would resolve as whatever took index 5. ⛔ That
 *  is a data corruption with no compile error, no log line and no red test —
 *  the DataTable would simply start resolving the wrong spells. ⇒ ⭐ NEW VALUES
 *  GO AT THE END, always, and a RETIRED one needs a CoreRedirect rather than a
 *  deletion.
 */
UENUM(BlueprintType)
enum class ESpellEffect : uint8
{
	None,
	AoEDamage,        // Fireball: Damage over AoERadius at the reticle
	Freeze,           // FrostNova: freezes enemy units/towers for EffectDuration
	TopTargetsDamage, // Lightning: Damage to the MaxTargets highest-current-HP enemies in AoERadius
	AllyBuff,         // BattleCry: friendly units in AoERadius buffed for EffectDuration
	GoldSteal,        // Pickpocket: steals GoldSteal gold, instant resolve (M5 ruling 7)

	/**
	 *  ⭐⭐ Fog: raises the WORLD-GLOBAL fog, which then lifts on its own.
	 *  ⛔ CORRECTED 2026-09-04 (TASK-982 item (0)): this line used to read "for
	 *  EffectDuration", which is FALSE AS A MECHANISM — the duration is
	 *  AFogVolume::FogDurationSeconds, read off the CDO by RaiseFog(), which
	 *  takes NO argument; the row's own `EffectDuration` cell is NOT read by the
	 *  fog arm at all. The old sentence was true only by the COINCIDENCE that
	 *  both values are `300`. ⭐ TASK-1016 is the row that makes the cell
	 *  AUTHORITATIVE; until it lands the cell is inert and this comment is the
	 *  only place that inertness is visible.
	 *  TASK-839 item (4) — the ONE new value the Fog card dispatches on, and the
	 *  value TASK-840's `Fog` row writes into its SpellEffect cell
	 *  character-for-character. Law FOG-§6 (the card), FOG-§9.4 (the duration is
	 *  `300` — "fog is up for exactly 5 minutes"), FOG-§10.3 (the state machine).
	 *
	 *  ⛔⛔ NOT "fog near the caster". Jonathan's ruling (J-F1, FOG-§9.1) is that
	 *  fog is "completely universal … it will cover the ENTIRE battlefield", so
	 *  there is no radius on this effect and no AoERadius cell to read. A future
	 *  reader looking for the missing radius should stop looking: its absence IS
	 *  the design.
	 *
	 *  ⭐⭐ THE EFFECT IS LIVE (TASK-998, 2026-09-04). The paragraph that stood
	 *  here declared the opposite — that the actor half was held, that
	 *  `AFogVolume` had zero declarations in Source/, and that ResolveSpell
	 *  refused this effect out loud — and every sentence of it died the instant
	 *  the arm inverted. ⛔ It is STRUCK rather than left standing, because a
	 *  shipped mechanic carrying a comment saying it does not work is exactly the
	 *  drift defect SC-§65 exists to stop.
	 *  ⇒ WHAT IS TRUE NOW: `AFogVolume` (Siegebound/FogVolume.h) holds the ONE
	 *  "fog is active until T" scalar; USpellLibrary::ResolveSpell stamps a
	 *  `FogDurationSeconds` expiry on it and REFRESHES rather than stacks on a
	 *  re-cast (J-F16); FSiegeCombatStatics::ReadFogState consults it, so the
	 *  vision ceiling really fires. The refusal path survives for the one case
	 *  that still warrants it — a state actor that cannot be found or spawned.
	 *
	 *  ⭐ SIBLING, ⛔ KEPT RATHER THAN TIDIED (qa/TASK-1015 NIT-2 asked for exactly
	 *  that): `BrightSun` has its OWN value — `FogClear`, appended below on
	 *  2026-09-04 (FOG-§10.1, TASK-982 item (9)). It clears fog and opens a
	 *  prevention window, which is a DIFFERENT effect, not this one inverted.
	 *  ⛔ Only the TENSE moved when it landed; the sentence's job — stopping the
	 *  two effects being overloaded onto one value — is why it is still here.
	 */
	FogCover,

	/**
	 *  ⭐⭐ BrightSun: CLEARS the world-global fog and then PREVENTS new fog for
	 *  a height-scaled window. TASK-982 item (9) — the ONE new value the
	 *  `BrightSun` card dispatches on, and the value TASK-983's `BrightSun` row
	 *  writes into its SpellEffect cell character-for-character. Law FOG-§10.1
	 *  (the card and its names), FOG-§10.3 (the three-state machine), FOG-§10.7
	 *  (the 2026-09-04 rulings).
	 *
	 *  ⛔⛔ APPENDED AT THE END, ⛔ NOT placed beside `FogCover` for tidiness —
	 *  and the append-only rule at the top of this enum is aimed squarely at
	 *  this value. Landing it ABOVE `FogCover` would renumber `FogCover` from 6
	 *  to 7 and silently re-read every saved `Fog` cell in /Game/Data/DT_Cards as
	 *  this effect instead. ⭐ THE TIDIEST-LOOKING EDIT IS THE CORRUPTING ONE,
	 *  and nothing in the toolchain reports it: no compile error, no log line, no
	 *  red test — except Tests/SiegeFogVolumeTest.cpp test 1, which now pins THIS
	 *  value as the append point.
	 *
	 *  ⛔ THE NAME IS NARROWER THAN THE EFFECT, SAID PLAINLY SO NOBODY "FIXES" IT:
	 *  `FogClear` names the visible half; the PREVENTION WINDOW is the half that
	 *  actually decides matches. Both halves resolve through
	 *  AFogVolume::ApplyBrightSun, and the window is
	 *  `BrightSunBaseDurationSeconds + BrightSunBonusSecondsPerStep ×
	 *  floor(height / BrightSunHeightStepUU)` — 120 s + 1 min per 50 ft above the
	 *  flat-grass datum, sampled ONCE at the cast (J-F15) and UNCAPPED (J-F14).
	 *  ⛔ Renaming it now needs a CoreRedirect, exactly like a retirement.
	 *
	 *  ⛔⛔ AND IT IS THE ONE-WAY DOOR, WHICH IS THE EASIEST THING HERE TO GET
	 *  WRONG (his sentence, verbatim): "Even when the 'bright sun' fog prevention
	 *  timer ends, the fog that was cleared STILL REMAINS CLEAR." ⇒ the shield
	 *  expires to CLEAR, ⛔ NEVER back to FOGGED. There is no suspended fog and no
	 *  remembered remainder anywhere in the implementation.
	 */
	FogClear
};

/**
 *  Spell delivery selector (CONVENTIONS "Spell delivery overhaul (2026-07-21)",
 *  TASK-236). Chooses HOW a spell's effect reaches its targets; the effect
 *  itself (SpellEffect) and every magnitude are unchanged by delivery.
 *
 *  Auto is the sparse default and resolves PER-EFFECT in
 *  USpellLibrary::GetEffectiveDelivery: AoEDamage and Freeze — today exactly
 *  Fireball and FrostNova, the two cards Jonathan's directive names — deliver
 *  as a HERO-ORIGIN LINE; every other effect keeps the reticle-placed ground
 *  circle. The explicit values exist as the per-card DATA override lever: a
 *  future card can pin GroundCircle or HeroLine in its cards.csv cell
 *  regardless of its effect.
 *
 *  CSV note — ⛔ CORRECTED 2026-09-04 (TASK-1000). It used to say the column
 *  header was *"NOT yet appended to the CSV"* because cards.csv was frozen for
 *  the TASK-236/TASK-240 wave. ⛔ That wave is long over and the sentence went
 *  false with it: the `SpellDelivery` header IS in Docs/Data/cards.csv today,
 *  appended at the END (the importer maps by header NAME, not order). ⛔ And TWO
 *  CELLS ARE POPULATED — `Fireball` and `FrostNova` each pin `HeroLine`
 *  explicitly, which is REDUNDANT with what `Auto` already resolves for their
 *  AoEDamage/Freeze effects and is therefore a LIVE TRAP: GetEffectiveDelivery
 *  branches on the CELL FIRST, so those two never reach the per-effect
 *  resolution below and retuning it will NOT move them — every OTHER row is
 *  blank and takes the struct default `Auto`.
 *  ⚠️ This is the SAME claim shape as the one repaired on `NoticeRange` below,
 *  and it rotted the same way: *"the header is not in the CSV yet"* is a
 *  sentence about a FILE THAT MOVES, written in a file that does not.
 */
UENUM(BlueprintType)
enum class ESpellDelivery : uint8
{
	Auto,         // per-effect default: AoEDamage/Freeze -> HeroLine (2026-07-21 directive), everything else -> GroundCircle
	GroundCircle, // reticle-placed ground-circle AoE at the confirm point (the M5 delivery)
	HeroLine      // hero-origin line in the air toward the aim point (bot: castle-origin — flagged design default)
};

/**
 *  One row of /Game/Data/DT_Cards, imported from Docs/Data/cards.csv (GDD section 3.0).
 *  Row name = CardID in PascalCase (e.g. Footman).
 *  Property names MUST match the CSV header columns 1:1 - do not rename
 *  without updating cards.csv and re-importing DT_Cards.
 */
USTRUCT(BlueprintType)
struct GITCLAUDEUNREALTEST_API FCardRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Player-facing card name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	FString DisplayName;

	/** Card category */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	ECardType CardType = ECardType::Unit;

	/** Gold cost to play the card */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	int32 Cost = 0;

	/**
	 *  Hero-upgrade STACK CAP only (AHeroCharacter::GetStackCapForUpgrade /
	 *  ApplyUpgrade read it; 0 refuses the play). CARD-UNCAP 2026-08-28
	 *  (UNCAP-§2): its per-deck copy-cap meaning is ABOLISHED — a deck may hold
	 *  any number of copies of any card; only the exactly-50 total binds
	 *  (UDeckLibrary::IsDeckLegal). Column kept, data untouched (U6).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	int32 MaxCopies = 0;

	/** Max hit points of the spawned unit/building (0 for stat-less cards) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float HP = 0.0f;

	/** Damage dealt per attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float Damage = 0.0f;

	/** Attack range in world units */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float Range = 0.0f;

	/** Seconds between attacks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float Cadence = 1.0f;

	/** Movement speed in units/second */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float Speed = 0.0f;

	/** AI/behavior profile for spawned units */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	ECardProfile Profile = ECardProfile::None;

	/** Designer notes; not used by gameplay code */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	FString Notes;

	/** Copies of this card in the default 50-card deck; 0 = not in the default deck (GDD section 3.4) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	int32 DeckCount = 0;

	/** True if the card's attack is delivered by a homing projectile instead of melee contact (GDD section 3.0) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	bool bRanged = false;

	// --- M4 Set II keyword / behavior columns (GDD section 3.0/section 4). Sparse: defaults leave core cards unchanged. ---

	/** Charge keyword: the first attack after >=2s of uninterrupted movement deals 2x (GDD 3.0). Cavalry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Keywords")
	bool bCharge = false;

	/** Slayer keyword: 2x damage vs targets whose MaxHP >= 150 (GDD 3.0). Pikeman. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Keywords")
	bool bSlayer = false;

	/** Suicide keyword: the unit explodes on contact/death, dealing Damage as AoE over AoERadius, then dies (GDD 4). Sapper. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Keywords")
	bool bSuicide = false;

	/** Swarm keyword: if >0, playing the card spawns this many copies in a 300-unit circle for one cost (GDD 3.0). Militia Mob = 4. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Keywords")
	int32 SwarmCount = 0;

	/** Splash radius for area attackers; 0 = single target (GDD 4). Sapper 250, Bomb Tower 250. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float AoERadius = 0.0f;

	/** Inner blind-spot radius; the actor cannot fire at targets closer than this (GDD 4). Ballista Tower 300. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float MinRange = 0.0f;

	/**
	 *  ⭐⭐ PER-UNIT NOTICE (ENGAGEMENT) RADIUS in world units — how far this card's unit
	 *  notices an enemy, bound over ASummonedUnit::AggroRadius in LoadStatsAndStart beside
	 *  `AttackRange = Row->Range` and applied through ASummonedUnit::ResolveNoticeRadiusUU
	 *  (TASK-979; FOG-§9.6b, FOG-§9.9, ⭐⭐⭐ FOG-§9.11).
	 *
	 *  ⛔⛔ RE-DERIVED WHOLE 2026-09-04 (TASK-1000), ⛔ not patched. Every premise the previous
	 *  text stood on moved on the same day: 🧑 J-F28 raised the class default, TASK-1004 blanked
	 *  the one populated cell, and FOG-§9.11 retired the notice==firing identity. The retired
	 *  numbers appear below ⛔ only where a reader who remembers them must find their
	 *  replacement, and ⛔ nowhere as live values.
	 *
	 *  ⭐ SPARSE BY DESIGN: 0 (or a blank cell) means *"use the class default"*, which is
	 *  ASummonedUnit::UnitEngagementRadiusUU = 5000 — 🧑 J-F28, *"changing the notice radius for
	 *  all units to 5000 with no fog and still 609 under fog"*, which SUPERSEDES his own earlier
	 *  *"2000"* of the same day. ⛔ Do NOT hand-type the default into row after row; that is one
	 *  number copied thirty-odd times and exactly the drift surface this column exists to avoid.
	 *
	 *  ⛔⛔ AND THE COLUMN SHIPS ENTIRELY SPARSE — ⛔ NOT ONE ROW CARRIES A CELL TODAY. TASK-993's
	 *  Longbowman 3600 was the only populated one and the same 5000 ruling retired it (TASK-1004
	 *  blanks it: under a 5000 default a 3600 cell would make that card notice LESS than every
	 *  other unit, inverting the exception it existed for). ⭐ An entirely sparse column is the
	 *  CORRECT shape, ⛔ not an unfinished one — what ships is the CHANNEL, and a future card
	 *  opts into its own reach from its own row with zero code (FOG-§9.9).
	 *
	 *  ⛔⛔ IT IS NEITHER CAPPED NOR WIDENED — AND ⛔ WHICH MISREADING IS DANGEROUS HAS INVERTED,
	 *  WHICH IS WHY RE-SIGNING THE OLD WARNING WOULD HAVE BEEN THE WHOLE DEFECT. 5000 is a
	 *  DEFAULT: ⛔ never a ceiling, and ⛔ never a floor either.
	 *    • ⛔ AT THE RETIRED 2000 the danger was a CEILING — `FMath::Min(NoticeRange, 2000)` clamped
	 *      the Longbowman's 3600 back down, silently. ⚠️ At 5000 ⛔ NO SHIPPED CARD SITS ABOVE THE
	 *      DEFAULT AT ALL, so a `min` is now INERT against today's roster and would go GREEN
	 *      against every data-derived assertion. ⇒ warning a data author about it FIRST spends
	 *      their vigilance on the safe operation.
	 *    • ⛔⛔ THE LIVE DANGER IS THE OPPOSITE SPELLING: an `FMath::Max(NoticeRange, 5000)`, or any
	 *      *"normalise the blank cell up to the default"* pass, would WIDEN a card that asked for
	 *      LESS — and the column would stop being an opt-in channel and become a floor nobody
	 *      voted for. This is the spelling his 5000 made reachable.
	 *  ⛔ Nothing errors and nothing logs for either, so the refusal is EXECUTED, not described:
	 *  ResolveNoticeRadiusUU's middle branch is a PASS-THROUGH IN BOTH DIRECTIONS, and both
	 *  spellings are asserted ABSENT by Tests/SiegeUnitNoticeRangeTest.cpp — the anti-cap half by
	 *  a SYNTHETIC value above the default and by a structural probe, ⛔ never by a card, because
	 *  no card can demonstrate it any more.
	 *  ⇒ ⭐ THE ONE UNIVERSAL CEILING IN THIS SYSTEM IS FOG'S VISION CEILING
	 *  (FSiegeFogTuning::FogVisionCeilingUU, 609.6 uu by default), applied as a `min` at the ONE
	 *  chokepoint inside the acquisition funnel (FOG-§7) and ⛔ nowhere else. ⛔ Never a second
	 *  ceiling, and ⛔ never one here.
	 *
	 *  ⛔ IT IS NOT `Range`. `Range` is the FIRING reach (the Longbowman keeps 3600 there,
	 *  untouched); this is the reach at which a target may be ACQUIRED at all. ⛔⛔ THE IDENTITY
	 *  RULE IS RETIRED FOR EVERY CLASS (⭐⭐⭐ FOG-§9.11): notice and firing are now DIFFERENT
	 *  numbers on every ranged card in the game — Longbowman 5000/3600, Archer 5000/2100, Wizard
	 *  5000/2100 — so they must NOT be collapsed into one column, and melee fires at 120 and
	 *  would never chase anything if it only noticed at 120.
	 *
	 *  ⚠️ A CELL HAS A LEASH CONSEQUENCE, said here because the author of the cell is who needs
	 *  it: the leash a drop site applies is ASummonedUnit::ResolveEffectiveLeashRangeUU =
	 *  max(LeashRange, notice × LeashMarginMultiplier). Below the crossover (`LeashRange /
	 *  LeashMarginMultiplier` — ⛔ DERIVED there, never typed here, and pinned by test 3(f) in
	 *  Tests/SiegeUnitNoticeRangeTest.cpp) the floor wins and a cell moves nothing; ⛔ above it a
	 *  cell lengthens the leash too, automatically and with no edit at any drop site.
	 *
	 *  ⛔ A NON-COMBAT CLASS IGNORES THIS CELL ENTIRELY. AMinerUnit/ASorcererUnit seal
	 *  themselves with AggroRadius 0 in their constructors, and ResolveNoticeRadiusUU refuses to
	 *  write over a non-positive class default — a card cell can never un-seal a sealed unit.
	 *
	 *  CSV note: the `NoticeRange` header IS in Docs/Data/cards.csv (appended at the END, like
	 *  CardArt and SpellDelivery before it — the DataTable importer maps by header NAME, not
	 *  order), and the property is present in /Game/Data/DT_Cards' row schema, measured there at
	 *  this struct default on every row (TASK-993 authored the column; TASK-1005 measured the
	 *  asset half). ⛔ What is empty is every CELL: every row deserializes to 0.0 ⇒ every unit
	 *  resolves to its class default ⇒ ⛔ zero behaviour change from the column's existence,
	 *  which is the point rather than a caveat.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float NoticeRange = 0.0f;

	/** Spawner building: CardID spawned every SpawnInterval seconds (GDD 4). Barracks = Footman. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner")
	FName SpawnCardID = NAME_None;

	/** Spawner building: seconds between spawns (GDD 4). Barracks = 8. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner")
	float SpawnInterval = 0.0f;

	/** Spawner building: self-destruct after this many seconds (GDD 4). Barracks = 60. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner")
	float Lifetime = 0.0f;

	// --- Card artwork (TASK-079). Column appended at the END of cards.csv; the CSV importer maps by header name, not order. ---

	/**
	 *  Hand-UI card illustration (CONVENTIONS "Card artwork (hand UI)"):
	 *  full object path /Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>.
	 *  Unset or unresolvable = graceful text-only card face (today's
	 *  presentation), logged once, never a crash. Resolved null-safe by
	 *  UCardHandWidget's art resolvers; LoadSynchronous is accepted for these
	 *  512x512 UI textures (TASK-079 ruling 4).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	TSoftObjectPtr<UTexture2D> CardArt;

	// --- M5 Set III spell columns (GDD section 3.11 / section 4; CONVENTIONS "Spells & Set III (M5)", TASK-097).
	//     Sparse: defaults leave every non-spell card unchanged. Spells REUSE Damage
	//     (Fireball 100, Lightning 200) and AoERadius (Fireball 300, FrostNova 350,
	//     Lightning 400, BattleCry 400) — no duplicate damage/radius columns. ---

	/** Spell dispatch token consumed by USpellLibrary::ResolveSpell (M5); None = not a spell effect. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell")
	ESpellEffect SpellEffect = ESpellEffect::None;

	/**
	 *  HOW the spell effect is delivered (TASK-236, CONVENTIONS "Spell delivery
	 *  overhaul (2026-07-21)"). Auto (the sparse default — see the enum doc)
	 *  resolves per-effect: AoEDamage/Freeze -> HeroLine, others ->
	 *  GroundCircle; the explicit values are the per-card data override. The
	 *  column header IS in cards.csv, where Fireball and FrostNova each pin
	 *  HeroLine in their own cell — a populated cell OUTRANKS the per-effect
	 *  map, so retuning that map will NOT move those two — while every OTHER
	 *  row is blank and takes Auto
	 *  (⛔ corrected 2026-09-04, TASK-1000 — this tooltip is REFLECTED into
	 *  the DataTable schema a designer reads in the editor, and it was still
	 *  saying *"header not yet in cards.csv (frozen this wave)"*).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell")
	ESpellDelivery SpellDelivery = ESpellDelivery::Auto;

	/** Timed-effect duration in seconds (GDD 4: FrostNova 4, BattleCry 8). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell")
	float EffectDuration = 0.0f;

	/** Top-N target count for TopTargetsDamage (GDD 4: Lightning 3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell")
	int32 MaxTargets = 0;

	/** Gold stolen from the opponent, capped at what they have (GDD 4: Pickpocket 10). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell")
	int32 GoldSteal = 0;

	/** Chain keyword: total targets hit per attack (GDD 4: CrystalTower 3). Typed column per the M4 keyword-deferral note. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Keywords")
	int32 ChainTargets = 0;

	/** Chain keyword: flat damage reduction per bounce (GDD 4: CrystalTower 5 => 15/10/5 with Damage 15). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Keywords")
	int32 ChainFalloff = 0;
};
