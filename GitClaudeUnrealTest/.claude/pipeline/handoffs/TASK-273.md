# TASK-273 handoff — Command input assets + IMC_Hero T/R/E mappings

**Status:** ready-for-integration (art tasks skip QA)
**Branch:** m7.6-arena10x
**Agent:** art-director
**Date:** 2026-07-23

## What was created (three new Input Action assets)

All three DUPLICATED from the existing `IA_Card1` asset, so their type/config is a byte-faithful
mirror (Value Type **Boolean/Digital**, no Triggers, no Modifiers, `bConsumeInput = true`). Verified
each after creation.

| Asset (exact /Game/ path) | Value Type | Key |
|---|---|---|
| `/Game/Input/Actions/IA_CmdAttack` | Boolean (Digital/bool) | T |
| `/Game/Input/Actions/IA_CmdHold`   | Boolean (Digital/bool) | R |
| `/Game/Input/Actions/IA_CmdDefend` | Boolean (Digital/bool) | E |

Source .uasset files live in-project under `Content/Input/Actions/`. These are the exact paths
TASK-274's C++ soft-resolves (`CmdAttackActionAsset` / `CmdHoldActionAsset` / `CmdDefendActionAsset`).
Names match the CONVENTIONS "Unit commands (Shield Wall stances)" law character-for-character.

## IMC edited + mappings added

**Context edited: `/Game/Input/IMC_Hero`** — confirmed as the player's context (assigned as the
`HeroMappingContext` slot on `BP_HeroCharacter`, added at priority 1 in `HeroCharacter.cpp:146`;
its dependency list references IA_Card1..6 + IA_Rally etc.). No new IMC created.

Three key mappings ADDED (each: no triggers, no modifiers, `InheritSettingsFromAction`):
- `IA_CmdAttack` → **T**
- `IA_CmdHold`   → **R**
- `IA_CmdDefend` → **E**

## E/R/T conflict-check finding (mandatory readback)

Opened IMC_Hero and read back its live `defaultKeyMappings.mappings` array (18 pre-existing rows).
Note: the legacy top-level `Mappings` property is EMPTY in UE 5.8 — the real bindings live in
`defaultKeyMappings.mappings`; reading the wrong property returns a misleading `[]`. Full pre-edit
key list read back:

`IA_Jump→SpaceBar`, `IA_Move→W/S/A/D`, `IA_Look→Mouse2D`, `IA_Sprint→LeftShift`,
`IA_Attack→LeftMouseButton`, `IA_Card1→One`, `IA_CancelPlace→RightMouseButton`,
`IA_CancelPlace→Escape`, `IA_Card2→Two`, `IA_Card3→Three`, `IA_Card4→Four`, `IA_Card5→Five`,
`IA_Card6→Six`, `IA_UICursor→LeftAlt`, `IA_Rally→Q`.

**RESULT: NO CONFLICT.** None of E, R, or T was mapped to any existing action. The only letter-key
binding present is Rally=Q — exactly matching the manager's text-check. Nothing was stomped.

## Integrity verification (no regression)

The mapping edit rewrote the full array (append semantics). Re-read after write confirmed all 18
original rows survived intact, INCLUDING the `IA_Move` diagonal/negate instanced modifier sub-objects
(`InputModifierSwizzleAxis_0/1`, `InputModifierNegate_0/1/2`) — WASD movement behavior is unchanged.
Array now holds 21 rows (18 original + 3 new). All four assets saved to disk.

## PIE / disruption note

Checked PIE at task start via `IsPIERunning` → **false**. Jonathan was NOT mid-playtest, so the
IMC_Hero mapping edit was done in full (no deferred remainder). No editor session was interrupted.

## Notes for integration (build-master / TASK-274)

- No C++, HUD, Blueprint wiring, compiling, or Git was touched (out of scope).
- The three IA assets + the IMC_Hero mapping edit are the full deliverable; TASK-274 binds
  `ETriggerEvent::Started` on these to its `OnCmd*Pressed` handlers.
- These assets are part of the branch-frozen set for this feature (per the manager lane ruling).
