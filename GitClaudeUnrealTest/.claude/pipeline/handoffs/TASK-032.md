# TASK-032 — Input assets v2: IA_Card2..6 + IA_UICursor (editor) — HANDOFF

**Agent:** gameplay-programmer
**Date:** 2026-07-04 (autonomous overnight run, serial editor wave — exclusive editor access)
**Status:** ready-for-qa

## Editor state
- Editor was already UP from TASK-031 (commit aafd968, MCP reachable). I did NOT boot or close it.
- Ran a PIE session for acceptance, then **StopPIE**. `IsPIERunning` = false. **Editor is LEFT UP** for TASK-033..036 / TASK-040.
- All work done in the live editor via the Unreal MCP (`editor_toolset` toolsets through `execute_tool_script`).

## What shipped

### 1. Six Input Action assets created (`/Game/Input/Actions/`)
Created by **duplicating `IA_Card1`** (the existing M1 bool/Digital action with no triggers/modifiers) — the MCP AssetTools surface has no "create asset" verb, and IA_Card1 is exactly the config all six need (a plain digital press; the IA_UICursor hold behavior lives in code via Started/Completed/Canceled, not in the asset). Verified each is `valueType = Boolean`, 0 triggers, 0 modifiers:

| Asset | Mapped key (IMC_Hero) | Purpose |
|-------|------------------------|---------|
| `/Game/Input/Actions/IA_Card2` | `Two` | key 2 → hand slot 1 |
| `/Game/Input/Actions/IA_Card3` | `Three` | key 3 → hand slot 2 |
| `/Game/Input/Actions/IA_Card4` | `Four` | key 4 → hand slot 3 |
| `/Game/Input/Actions/IA_Card5` | `Five` | key 5 → hand slot 4 |
| `/Game/Input/Actions/IA_Card6` | `Six` | key 6 → hand slot 5 |
| `/Game/Input/Actions/IA_UICursor` | `LeftAlt` | hold = cursor for HUD clicks |

Paths match the constructor-seeded soft paths in `SiegePlayerController.cpp` exactly (`/Game/Input/Actions/IA_Card2.IA_Card2` .. `IA_Card6.IA_Card6`, `IA_UICursor.IA_UICursor`).

### 2. IMC_Hero mappings (`/Game/Input/IMC_Hero`)
Appended **6** entries to `defaultKeyMappings.mappings` (the live array in UE 5.8 — legacy `mappings` stays empty, per TASK-009). The 11 pre-existing M1 mappings were preserved **byte-for-byte**, including their instanced WASD modifier subobjects. Post-write readback (17 total):

- Original 11 intact with modifier counts unchanged: Space→IA_Jump (0 mods), W→IA_Move (1: Swizzle), S→IA_Move (2: Swizzle+Negate), A→IA_Move (1: Negate), D→IA_Move (0), Mouse2D→IA_Look (1: Negate-Y), LeftShift→IA_Sprint (0), LMB→IA_Attack (0), One→IA_Card1 (0), RMB→IA_CancelPlace (0), Escape→IA_CancelPlace (0).
- New 6: Two→IA_Card2, Three→IA_Card3, Four→IA_Card4, Five→IA_Card5, Six→IA_Card6, LeftAlt→IA_UICursor (all 0 triggers/0 mods).

**IMC_Default and all template/donor assets were NOT touched.** IA_Card1 (duplicate source) was read-only; duplication does not dirty the source.

### 3. Wiring — the binding site
The IA_Card1 site is the **controller**, not BP_HeroCharacter, and it is already compiled in aafd968:
- Constructor (`SiegePlayerController.cpp:54-60`) seeds the soft paths `Card2ActionAsset`..`Card6ActionAsset` + `UICursorActionAsset`.
- `SetupInputComponent` (`:126-164`) calls `ResolveInputAction` (hard slot → soft path), then binds: `Card1Action` → `OnCard1Pressed` (slot 0, with the M1 empty-hand Footman fallback); `Card2..Card6Action` → `OnCardSlotKeyPressed` with payload `ActionIndex+1` (slots 1..5); `UICursorAction` → `OnUICursorPressed` (Started) + `OnUICursorReleased` (Completed AND Canceled).

Because the slots soft-resolve from the constructor-seeded paths, **creating the assets at those exact paths IS the wiring** — no editor UPROPERTY assignment or BP subclass needed (there is no BP controller subclass; TASK-023 contract §"Contract for TASK-032"). Net key→behaviour: keys 1–6 → `PlayHandSlot(0..5)`; hold Left Alt → cursor + GameAndUI + `SetIgnoreLookInput(true)`; release → GameOnly free-look restored (unless placement mode still owns the cursor).

## PIE acceptance (MCP)
Started PIE in-viewport (4 s warmup), read `LogGitClaudeUnrealTest`:
- **Zero** `"input action ... not resolved"` warnings — i.e. all eight actions (Card1..6, UICursor, CancelPlace) resolved and bound. Before this task IA_Card2..6/IA_UICursor did not exist and would each have logged a "not resolved" warning; those are now gone.
- **Zero** Errors/Warnings from the game category. The only entry: `UDeckComponent on 'SiegePlayerController_0': built a 50-card draw pile from 6 card rows (GDD §3.4).` — confirms the controller ran BeginPlay, the deck dealt a real hand from TASK-031's 6-row DT_Cards (so keys 1–6 target live hand slots), and the row struct is intact at runtime.
- No `Missing RowStruct` runtime error surfaced (that TASK-031 note was a save-time LogDataTable line, not a PIE runtime issue).

### What PIE could and could not verify
This MCP surface exposes **no input-injection / keypress-simulation verb**, so I could not physically press keys 1–6 or hold Left Alt inside PIE. The interactive key→slot and Alt-cursor *logic* is already compiled and QA-passed in aafd968 (TASK-023/TASK-039). What TASK-032 delivers — the assets + their key mappings + their resolution at the binding site — is verified structurally (mapping readback) and at runtime (clean binding resolution, no "not resolved" warnings, real hand dealt). I did not fake keypress results.

## Optional cosmetic (Config/DefaultInput.ini) — SKIPPED
Intentionally skipped per the spec's "skip if it costs more than minutes." I did not have the exact engine Shift debug-binding log string captured, and speculatively editing the shared input config to silence one cosmetic line risks adding more noise than it removes. No functional impact. Can be revisited if the manager wants it as its own tiny task.

## Constraints honored
- No Git, no compile (this was asset/wiring only; code already compiled in aafd968).
- Did NOT edit TASKBOARD.md (orchestrator flips the board). Requested status: `in-progress` → `ready-for-qa`.
- Targeted saves only (the 7 target assets); no donor/template re-dirtied. IMC_Default untouched.
- New `.uasset` files left for TASK-040's commit; if the UE Git provider auto-staged them on save, left as-is (no git run).
- Editor left UP, PIE stopped.

## Files / assets touched
- NEW: `Content/Input/Actions/IA_Card2.uasset`, `IA_Card3.uasset`, `IA_Card4.uasset`, `IA_Card5.uasset`, `IA_Card6.uasset`, `IA_UICursor.uasset`
- EDITED: `Content/Input/IMC_Hero.uasset` (6 mappings appended; 11 M1 mappings + modifiers preserved)
- No source (.cpp/.h) changes — the wiring was already compiled in aafd968.

## For QA to scrutinize
- IMC_Hero readback: 17 mappings, 11 originals unchanged (WASD modifier counts W=1/S=2/A=1/Mouse2D=1 preserved through the array rewrite), 6 new correct. Legacy `mappings` array still empty (runtime reads `defaultKeyMappings.mappings`).
- Key names: `Two`/`Three`/`Four`/`Five`/`Six` (number-row, matching the existing `One` for IA_Card1) and `LeftAlt`.
- Asset paths match the constructor soft-paths exactly; all six are `Boolean` digital, 0 triggers/0 modifiers.
- PIE: clean binding resolution (no "not resolved"), no errors, 50-card deck from 6 rows.
