# TASK-100 handoff — Targeting mode: reticle-anywhere spell play on ASiegePlayerController (files)

- **Author:** gameplay-programmer
- **Date:** 2026-07-08
- **Status requested:** ready-for-qa (orchestrator flips the board)
- **Scope:** files only — no compile, no editor, no Git, exactly the assigned file set. Builds ON TOP of
  TASK-093's qa-passed state; the placement machine is untouched except for the additive interplay
  points listed below.

## Files touched

- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`

Nothing else. Consumes (read-only): `SpellLibrary.h` (TASK-098 pinned resolver), `CardRow.h`
(TASK-097 ESpellEffect/columns), `SiegePlayerState.h` (gold APIs) — all via existing includes; three
new cpp includes: `Components/DecalComponent.h`, `Engine/DecalActor.h`, `Siegebound/SpellLibrary.h`
(alphabetical order preserved).

## Mode state machine

TARGETING mode is a strict SIBLING of placement mode with a fully disjoint state block:

```
IDLE ──PlayHandSlot(Spell, SpellEffect==GoldSteal)──► INSTANT RESOLVE (no mode entered; ruling 7)
IDLE ──PlayHandSlot(Spell, else) → EnterTargetingMode──► TARGETING
TARGETING ──per frame (PlayerTick)──► RMB/Esc poll → EXIT (free)
                                      UpdateSpellReticle (TraceCursorToGround → surface point)
                                      LMB poll → TryConfirmSpellTarget
TryConfirmSpellTarget: trace-miss point → refuse free, STAY
                       SpendGold(Cost) → ResolveSpell(...)
                         true  → ConfirmPlayFromHand(TargetingHandSlot) → EXIT (success)
                         false → AddGold(Cost) FULL refund → refuse "Spell fizzled" → EXIT (card kept)
```

- **Mutual exclusion:** `EnterPlacementMode` silently ignores while `bInTargetingMode` and vice versa
  (the M1 already-placing early-out pattern, Verbose log). `PlayHandSlot` ignores mid-either-mode;
  `DiscardHandSlot` REFUSES (broadcast) mid-targeting exactly like mid-placement (hand-slot desync
  rule). At most one card mode is ever live.
- **State block (all reset by ExitTargetingMode):** `bInTargetingMode`, `bTargetingSurfaceValid`,
  `TargetingLocation`, `TargetingCardID`, `TargetingCost`, `TargetingRow` (full row snapshot),
  `TargetingHandSlot`, `TargetingHero` (Transient), `SpellReticleActor` (Transient). Plus the
  log-once latch `bWarnedNoReticleMaterial` (deliberately NOT reset — "log once" per ruling 8).
- **Entry gates (EnterTargetingMode), in order:** match-ended/mid-mode silent ignores → NAME_None →
  row resolve (DT_Cards, data-driven law) → CardType==Spell defensive gate → PlayerState → CanAfford
  → GoldSteal reroute to instant → hero-dead refusal ("Hero is down"). NO gold moves at entry.
- **Reticle:** transient `ADecalActor` spawned pitch −90° (projects straight down), decal material =
  soft `/Game/Materials/M_SpellReticle` (TASK-108), `DecalSize = (500, R, R)` where R = the row's own
  AoERadius (else `SpellReticleDefaultRadius` = 150). Hidden while the cursor is off every surface;
  positioned each frame at the trace ImpactPoint — surface Z, never Z=0 (M4.5 carry-in LAW).
- **NO half restriction, NO navmesh projection, NO slope/obstacle/clearance gates** anywhere in the
  targeting path — spells land anywhere a surface answers the Visibility trace (ruling 8).
- **Cursor posture:** `ApplyCursorInputState`'s owner composition gains targeting as the third owner
  (`bInPlacementMode || bInTargetingMode || bUICursorHeld`) — byte-identical GameAndUI posture, no
  new input assets (M2 TASK-023 + TASK-074 laws). Hero melee suppressed while the mode owns the LMB
  (TASK-003 API) on the separate `TargetingHero` record.

## Exit paths (the placement seven-exit-path law, targeting edition — NINE paths)

Every one funnels through `ExitTargetingMode()`, which releases the melee suppression BEFORE its
early-out (qa/TASK-003-report.md warning 2 law), resets every Targeting* member, destroys the
reticle, and restores the composed cursor posture:

1. **Confirm success** — TryConfirmSpellTarget after resolve + hand consume (cpp ~1558).
2. **Confirm resolver-false** — full refund + "Spell fizzled" + exit, card kept (cpp ~1533).
3. **Cancel, action path** — IA_CancelPlace binding → OnCancelPlacePressed (free).
4. **Cancel, polled path** — RMB/Esc polled in PlayerTick (asset-missing soft-lock cover, free).
5. **Match end** — HandleMatchEnd, before the UI-only switch (next to ExitPlacementMode).
6. **Hero death** — HandleHeroDied (suppression released on the recorded TargetingHero).
7. **Unpossession** — OnUnPossess defensive exit.
8. **Match reset** — HandleMatchReset FIRST, before the deck rebuild (the TASK-023 WARN rationale:
   never rebuild the hand under a live TargetingHandSlot).
9. **EndPlay** — teardown defensive exit.

Refusals that deliberately STAY in mode (not exits): trace-miss confirm click ("No target under
cursor"), defensive SpendGold false ("Not enough gold"), null PlayerState/World at confirm.

## Gold flow (confirm law, §3.0 net-zero)

- Targeted spells: NOTHING moves before LMB confirm; cancel is always free. At confirm:
  `SpendGold(Cost)` THEN `USpellLibrary::ResolveSpell(World, CardID, TargetingRow, CasterTeam,
  TargetingLocation)`. False ⇒ `AddGold(Cost)` full refund (choke-pointed API) + HUD reason + card
  kept. True ⇒ card leaves the hand NOW (`ConfirmPlayFromHand`, M2 law).
- GoldSteal (Pickpocket): `ResolveSpellInstant` at PLAY time — same deduct-then-resolve shape,
  refusal-safe: Sandbox mode (no Red economy) returns resolver-false ⇒ full refund, card kept. A
  0-gold victim RESOLVES (spent like a wasted Fireball — resolver contract). Hand consume only on
  success.

## Flagged decisions (QA: please rule on each)

1. **Hero-dead refusal at targeting entry ("Hero is down").** Spec silent; mirrored from
   EnterPlacementMode — targeting owns the LMB exactly like placement and the sibling modes should
   not diverge on the card-play-while-dead rule. Instant GoldSteal is NOT hero-gated (the
   Masons/instant-play precedent) — the GoldSteal reroute sits BEFORE the hero-dead gate so both
   GoldSteal entries (PlayHandSlot and direct) behave identically.
2. **Resolver-false confirm EXITS the mode** (refund + card kept). Justification: ResolveSpell's
   false set (null world, NAME_None, SpellEffect None/unknown, malformed row, unresolvable state) is
   position-INDEPENDENT by the SpellLibrary contract — nothing a different click could fix — so the
   missing-BP-class placement precedent (exit, don't strand the player) applies, not the
   invalid-point precedent (stay).
3. **Trace-miss confirm stays in mode** ("No target under cursor", `CardRefused_NoTarget`) — the
   placement invalid-click law analog; the ONLY positional refusal in the whole targeting path.
4. **Disjoint Targeting\* state block** (incl. a separate `TargetingHero` suppression record).
   Rationale: `ExitPlacementMode` releases `PlacementHero` UNCONDITIONALLY before its early-out, so
   sharing the record would let any defensive ExitPlacementMode call strand/steal a live targeting
   suppression. Double-release across the two exits is an idempotent flag write.
5. **`TargetingRow` = full FCardRow snapshot at entry** — the placement scalar-snapshot pattern
   (PendingCost/PendingCardType) generalized because the pinned resolver consumes the whole row; a
   plain non-UPROPERTY member (FCardRow carries no hard UObject refs — soft paths only).
6. **Reticle = transient `ADecalActor`** (the engine's decal-component wrapper), mirroring the
   ghost's transient-actor pattern (RF_Transient, AlwaysSpawn). Spawned at pitch −90° EXPLICITLY:
   the spawn rotation stomps the CDO root's relative rotation, so relying on ADecalActor's default
   would be fragile. Missing M_SpellReticle ⇒ NO actor spawned at all — targeting runs on the OS
   cursor alone (invisible-ghost degradation precedent), warned ONCE per controller via the latch.
7. **Reticle sized to the spell's own AoERadius** (Fireball 300 / FrostNova 350 / Lightning 400 /
   BattleCry 400 — data-driven, no hardcoded stats) with `SpellReticleDefaultRadius` = 150 UPROPERTY
   fallback for a radius-less spell; decal projection half-depth 500 brackets the M4.5 max terrain
   height (250), mirroring the ±500 slope-trace window. `DecalSize` is written directly +
   `MarkRenderStateDirty()` (UDecalComponent has no SetDecalSize API).
8. **Cursor-owner composition** (`bWantCursor = placement || targeting || Alt`): targeting joins the
   composition rather than inventing a posture (ruling 8 "mirrors placement"). Alt-cursor interplay
   is therefore identical to placement: pressing/releasing Alt mid-targeting never drops the cursor;
   releasing targeting with Alt still held keeps it up for the HUD.
9. **Mutual exclusion = silent ignore both ways** (Enter\* and PlayHandSlot; Verbose logs), matching
   the M1 already-placing pattern; `DiscardHandSlot` gets a player-facing REFUSAL
   (`DiscardRefused_Targeting`, "Cannot discard while targeting a spell") matching the placement
   discard rule.
10. **EnterTargetingMode is BlueprintCallable and defensively type-gated:** a non-Spell CardID
    refuses ("Card not available", caller-regression warning); a direct GoldSteal call reroutes to
    the instant resolve HAND-LESS (Slot = INDEX_NONE skips the draw step) — PlayHandSlot owns hand
    plays and routes GoldSteal before ever reaching the mode.
11. **GoldSteal VFX anchor = hero location** (else world origin) — the resolver treats TargetPoint
    as a VFX anchor only for a global effect (SpellLibrary contract).
12. **Refusal strings** (spec left them unpinned; all ride the EXISTING FText → RefuseCardPlay /
    BroadcastRefusal FString path — zero delegate/BIE signature changes, zero UMG edits):
    "No target under cursor", "Spell fizzled", "Cannot discard while targeting a spell".
13. **`> 0` guard on both AddGold refund sites:** AddGold refuses non-positive grants WITH a log, so
    the guard only silences a spurious log for a hypothetical 0-cost spell — net-zero holds either
    way (0 spent ⇒ 0 refunded).
14. **Stale doc flag (NOT my file, untouched):** `ASiegePlayerState::AddGold`'s comment still says
    "Sole caller: the dev/test Sandbox starting-gold grant" — TASK-098 (GoldSteal transfer) and this
    task (refunds) add callers. One-line doc fix candidate for the TASK-103 batch; flagged to the
    manager rather than edited out-of-scope.
15. **Reticle can sit on a unit's mesh:** TraceCursorToGround hits whatever blocks Visibility, so
    hovering a unit puts the reticle/target point on the unit's surface. For SPELLS this is benign
    and arguably desired (aiming at a clump); the TASK-093 WARN-1 transient-actor concern does not
    apply — there is no slope/validity gate to flicker.

## Placement-machine preservation (verified by inspection)

- `UpdatePlacementGhost` / `TryConfirmPlacement` / `EnterPlacementMode` internals / `ExitPlacementMode` /
  all TASK-093 gates (slope, obstacle, building clearance, castle roof/plinth, enemy half, miner cap,
  fail-closed trace): byte-identical except the single additive `bInTargetingMode` early-out at the
  top of `EnterPlacementMode`.
- `PlayerTick`'s placement chain is untouched below the new self-contained targeting block (which
  `return`s before reaching it).
- Card-leaves-hand-at-CONFIRM, net-zero refusal, seven-exit-path melee law: all intact; targeting
  adds its own parallel nine-path enumeration above.
- No BIE/delegate signature changes; `FOnCardPlayRefused`/`FOnCardRefused` byte-identical.

## Shadow-law scan (C4457/58/59) — self-audit

New members are unique repo-wide (grep verified: only SiegePlayerController.h/.cpp). New locals per
function, none colliding with any member: EnterTargetingMode (`RowError`, `Row`, `SiegeState`,
`Hero`), TryConfirmSpellTarget (`SiegeState`, `World`, `CasterTeam`), UpdateSpellReticle (`Hit`,
`bSurfaceHit`), SpawnSpellReticle (`World`, `ReticleMaterial`, `SpawnParams`, `ReticleDecal`,
`ReticleRadius`), ResolveSpellInstant (`World`, `Hero`, `CasterTeam`, `AnchorPoint`). `Hero`/`World`/
`SiegeState`/`Row` follow the existing sibling-function local conventions and shadow nothing.

## What QA should scrutinize

- ADecalActor/UDecalComponent API usage (UE 5.8): `GetDecal()`, `SetDecalMaterial`, direct
  `DecalSize` write + `MarkRenderStateDirty()`, spawn-rotation-vs-CDO note.
- Gold ordering in both confirm shapes (deduct → resolve → refund-on-false) and that NO branch can
  double-spend or double-refund.
- The GoldSteal routing triangle: PlayHandSlot (hand slot) vs EnterTargetingMode direct (INDEX_NONE)
  vs the ruling-7 no-reticle law.
- Mode-transition hygiene at every lifecycle site (EndPlay, OnUnPossess, HandleHeroDied,
  HandleMatchEnd, HandleMatchReset, OnCancelPlacePressed, PlayerTick) — each now handles BOTH modes.
- NSLOCTEXT keys/strings character-for-character (listed in flagged decision 12).

## Assets/contracts referenced (built by parallel tasks)

- `/Game/Materials/M_SpellReticle` — TASK-108 (art); soft, null-safe, warn-once until it lands.
- `USpellLibrary::ResolveSpell` — TASK-098 (on disk; compiles together in the TASK-103 batch).
- `FCardRow.SpellEffect` / `ESpellEffect` — TASK-097 (on disk).
- No new content paths, no input assets, no UMG edits; ships via TASK-103's batch compile.

## Fix loop 1 (2026-07-08)

**Scope:** qa/TASK-100-report.md BLOCKER-1 only — `SpawnSpellReticle` in
`Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`. No other function, file, or asset
touched. No compile, no editor, no git (build-master owns those).

**Root cause (per QA, engine-source-verified):** `ADecalActor`'s constructor already sets its root
decal component to relative pitch −90 (DecalActor.cpp:30), and UE 5.8's `PostSpawnInitialize`
COMPOSES root ∘ spawn transform under the default `ESpawnActorScaleMethod::MultiplyWithRoot`
(Actor.cpp:4310-4324) — it does not stomp it, as the original comment claimed. Spawning with pitch
−90 therefore composed to −180: projection axis horizontal, ring smeared/invisible on the ground,
permanently (UpdateSpellReticle only moves location).

**Fix applied (exactly as prescribed):** spawn at `FRotator::ZeroRotator` (identity composes to the
CDO's own −90), then — immediately after the existing spawn-failure null check — set the downward
projection explicitly and composition-immune with absolute world rotation:
`SpellReticleActor->SetActorRotation(FRotator(-90.f, 0.f, 0.f));`. The adjacent comment was rewritten
to describe the compose-not-stomp engine model and cite BLOCKER-1.

Before (cpp, old ~1622-1636):
```cpp
	// pitch -90° points the decal's local X (its projection axis) straight DOWN
	// — ADecalActor's own editor-default orientation, reproduced explicitly
	// because the spawn rotation stomps the CDO's root relative-rotation. The
	// decal drapes whatever surface lies under the reticle point (hill crowns
	// and flanks included): the M4.5 surface-projection law in visual form.
	// Decals carry NO collision, so the reticle can never block the cursor trace.
	SpellReticleActor = World->SpawnActor<ADecalActor>(FVector::ZeroVector, FRotator(-90.f, 0.f, 0.f), SpawnParams);
	if (!SpellReticleActor)
	{
		... warn + return ...
	}
	SpellReticleActor->SetActorHiddenInGame(true); // shown on the first surface hit
```

After (cpp 1622-1645):
```cpp
	// Spawn at IDENTITY rotation: ADecalActor's constructor already gives its
	// root decal component relative pitch -90 (DecalActor.cpp:30), and UE 5.8's
	// PostSpawnInitialize COMPOSES root ∘ spawn transform (MultiplyWithRoot
	// default, Actor.cpp:4310-4324) — it does NOT stomp it. Spawning with -90
	// here would compose to -180 and lay the projection axis horizontal
	// (qa/TASK-100 BLOCKER-1). Identity composes to the CDO's own -90.
	SpellReticleActor = World->SpawnActor<ADecalActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!SpellReticleActor)
	{
		... warn + return ... (unchanged)
	}

	// pitch -90° points the decal's local X (its projection axis) straight DOWN.
	// SetActorRotation is ABSOLUTE world rotation — immune to CDO/spawn transform
	// composition — so the downward projection is explicit and composition-proof.
	// The decal drapes whatever surface lies under the reticle point (hill crowns
	// and flanks included): the M4.5 surface-projection law in visual form.
	// Decals carry NO collision, so the reticle can never block the cursor trace.
	SpellReticleActor->SetActorRotation(FRotator(-90.f, 0.f, 0.f));
	SpellReticleActor->SetActorHiddenInGame(true); // shown on the first surface hit
```

**Placement note:** QA's prescription said "immediately after the spawn"; the `SetActorRotation` call
sits immediately after the existing spawn-failure early-out (cannot rotate a null actor) and before
`SetActorHiddenInGame` — same frame, same call stack, before the actor is ever shown, so nothing can
observe the pre-rotation state.

**Adjacent comment left alone (scope rule):** `UpdateSpellReticle`'s comment (~cpp:1586) says the
decal "projects straight down (spawn-time rotation)". The rotation is still established inside
SpawnSpellReticle at spawn time, so the comment remains substantively true; editing it would exceed
the one-function scope QA set for re-review. Flagging here rather than touching it.

**Shadow scan of the changed lines:** zero new identifiers introduced — the diff only uses the
existing member `SpellReticleActor` and existing locals `World`/`SpawnParams`. C4457/58/59 clean by
construction.

**QA re-review scope:** SpawnSpellReticle only, per the report's build-master note.
