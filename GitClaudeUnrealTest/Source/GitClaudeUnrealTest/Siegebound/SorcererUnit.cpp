// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SorcererUnit.h"

ASorcererUnit::ASorcererUnit()
{
	// Per-card class (the AMinerUnit precedent): the row IDENTITY is the class's nature, so
	// it defaults here. Every STAT on that row still binds from DT_Cards at BeginPlay, never
	// from code (GDD §3.0) — this only names the row to read. TASK-375's BP_Unit_Sorcerer
	// sets the same value (a no-op) and the deferred-spawn InitUnit(Team, "Sorcerer") agrees
	// with it, so no spawn path changes.
	CardID = FName(TEXT("Sorcerer"));

	// ── Behavioral quieting (CONVENTIONS §3). These are NOT the seal — CanEverAttack() is
	// (see the header). They are what stops a unit that cannot attack from still ACTING like
	// it wants to, in the two legacy bodies that predate group orders:
	//
	// AggroRadius 0 — AcquireTarget() rejects every candidate farther than this, so at 0 it
	// can never return one and the legacy Standard body's CurrentTarget stays null forever
	// (it only ever enters Attack for a non-null target). The same zero also makes the ONE
	// synchronous UpdateState() that LoadStatsAndStart runs inside Super::BeginPlay
	// acquisition-dead.
	AggroRadius = 0.f;

	// DefendRadius 0 — the Shield Wall DEFEND stance targets via
	// AcquireEnemyNearPoint(own castle, DefendRadius); at 0 the disc is empty, so a sorcerer
	// under DEFEND falls back to marching home instead of picking a fight it cannot finish.
	DefendRadius = 0.f;

	// ⚠️ StateCheckInterval is DELIBERATELY LEFT AT THE BASE 0.25 s — do not "seal" it the way
	// AMinerUnit does (StateCheckInterval = 0, which makes FTimerManager::SetTimer clear the
	// handle instead of scheduling). The Miner is not commandable; THIS unit is, and it needs
	// its state timer running to resolve its group each tick and walk to its station. The
	// attack machine is sealed by CanEverAttack() at three guard points instead, which is
	// exactly why that virtual exists.
}
