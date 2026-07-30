// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 *  Siegebound network-relevancy limits — the ONE home for arena-scaled net
 *  distances (CONVENTIONS "Networked 1v1 (M8)" → NET RELEVANCY LAW, ruled
 *  2026-07-29 after the P1 two-client gate failure). One-concept header, the
 *  `TeamId.h` precedent: no UCLASS, no module dependency, just the constant
 *  every Tier-B class derives its cull distance from.
 *
 *  ⚖️ WHY THIS EXISTS (the measured defect it closes): UE's DEFAULT distance
 *  relevancy is 150 m (`NetCullDistanceSquared` 225,000,000 ≈ 15,000 uu) — but
 *  the M7.6 arena is ~500 m across (castles at ±25,000 uu). At TASK-357's
 *  two-client gate the client therefore never received the battlefield scatter
 *  (0 gold nodes vs the host's 6 — the actor is a point at the origin, players
 *  250 m away) and never received the FAR castle's HP/crumble/destroyed state
 *  (488 m: host 500 HP / client 2000), while the NEAR castle (12 m) replicated
 *  perfectly. The OnRep code was correct; relevancy was the defect.
 *
 *  THE THREE TIERS (every replicated class DECLARES its tier in its header —
 *  an undeclared class, or a Tier-B class carrying a hand-typed distance, is a
 *  QA FAIL):
 *    • TIER A — `bAlwaysRelevant = true`: match-critical singletons/near-
 *      singletons whose truth must not depend on where a camera is. Fixed, tiny
 *      population (≤ ~10 actors), low-frequency event-driven state ⇒ negligible
 *      bandwidth. A singleton that drives world generation or a win condition is
 *      ALWAYS Tier A.
 *    • TIER B — arena-scaled `SetNetCullDistanceSquared(...)` derived from
 *      `SiegeNet::ArenaRelevancyDistanceSquared` below — NEVER a per-class
 *      literal (that is the drift trap this header exists to prevent). For
 *      actors that are gameplay-relevant anywhere in the arena but numerous.
 *    • TIER C — engine default: purely local/cosmetic actors carrying no
 *      gameplay truth.
 *
 *  ⚠️ P2 WAVE DUTY (recorded here because this is the file P2 will open): the
 *  unit fleet is TIER B by default and must be BANDWIDTH-MEASURED at full
 *  fleet. A blanket `bAlwaysRelevant` across dozens of units is explicitly
 *  WRONG; if the measurement hurts, the levers are update-frequency throttling
 *  (`SetNetUpdateFrequency`) and dormancy — NEVER shrinking this band (units
 *  must stay visible marching across the arena).
 */
namespace SiegeNet
{
	/**
	 *  Arena-wide relevancy radius in Unreal units — the full arena diagonal
	 *  × 1.1 (ruled value: 60,000 uu). Derivation: the M7.6 10× arena spans
	 *  ±25,000 uu on X (castles at the extremes) with a comparable Y band, so
	 *  the corner-to-corner diagonal is ~54,000 uu; the ×1.1 margin covers the
	 *  Y span growing and any viewpoint sitting slightly outside the play area.
	 *
	 *  WHEN THE ARENA GROWS AGAIN, THIS ONE CONSTANT CHANGES — every Tier-B
	 *  class follows automatically. Do NOT hand-type a distance anywhere else.
	 */
	inline constexpr float ArenaRelevancyDistance = 60000.f;

	/**
	 *  The value `AActor::SetNetCullDistanceSquared` actually wants (3.6e9 at
	 *  the current 60,000 uu). DERIVED by multiplication — never typed as a
	 *  literal — so the square can never drift out of sync with the distance.
	 *  (float holds 3.6e9 comfortably; the engine stores this property as float.)
	 */
	inline constexpr float ArenaRelevancyDistanceSquared = ArenaRelevancyDistance * ArenaRelevancyDistance;
}
