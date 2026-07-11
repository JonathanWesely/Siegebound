# TASK-133 handoff — Castle-spacing 4× bot fallback constants (C++ files)

**Status:** ready-for-qa · files only, NO compile / NO editor / NO Git (per spec).

## What changed (exact constant edits)

### `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h`
- `CastleRedFallbackLocation` : `FVector(2000.f, 0.f, 0.f)` → **`FVector(8000.f, 0.f, 0.f)`** (comment updated to "+8000,0").
- `GoldNodeRedFallbackLocation` : `FVector(1200.f, 0.f, 0.f)` → **`FVector(7200.f, 0.f, 0.f)`** (comment updated to "+7200,0"; note added that the node moved WITH the castle, staying 800 units in front, to preserve the miner economy).
- `BotCenterlineSpawnX` = **`350.f` UNCHANGED**. Added the ruling comment: centerline-relative, the centerline did NOT move (still X=0), so it stays 350; a playtest may revisit whether the bot over-commits units across the wider field.

### `Source/GitClaudeUnrealTest/Siegebound/GoldNode.h` (comments only — no code)
- Class-doc arena-contract comment: `(-1200,0,0)` / `(+1200,0,0)` → **`(-7200,0,0)` / `(+7200,0,0)`**, with the M6.5 rationale note.
- `Team` member comment: `at -1200 … at +1200` → **`at -7200 … at +7200`**.

## Confirmations for QA
- **NO Blue fallback constant exists or was added** — the player side uses live `TActorIterator<ACastle>` / `TActorIterator<AGoldNode>` lookups (confirmed by reading SiegeBotController.h; only `CastleRedFallbackLocation` + `GoldNodeRedFallbackLocation` exist).
- **SiegeGameMode NOT touched by this task** — TASK-134 owns that file's PlayerStart comment fix.
- **KillZ NOT touched** (it is a vertical fall-death value, not horizontal spacing).
- No logic changed — only two FVector literals + comments.

## QA scan notes
- Shadow scan: no new locals/params introduced (constant-value + comment edits only).
- Complete-type-include scan: no new type usage; `FVector` already in scope.

## Downstream
- **build-master (TASK-136):** these CODE fallbacks now match the level actors you move to ±8000 / ±7200. The fallbacks only fire when the live actor lookup fails, so the moved level actors are still authoritative.
