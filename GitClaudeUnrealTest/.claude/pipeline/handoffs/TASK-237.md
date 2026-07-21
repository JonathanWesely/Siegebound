# TASK-237 — Lightning AoERadius 400 → 700 + honest-scale consumption audit (gameplay-programmer)

**Change:** `Docs/Data/cards.csv` line 26 (Lightning row), `AoERadius` cell only: `400` → `700`. Verified single-cell diff via `git diff -U0` (Notes cell's literal "in 400" text left frozen per the cells-frozen / single-cell-diff law — see audit row 7). **No code changes** — the honest-scaling verify came back clean, so this lands as pure data.

## Consumption-chain audit (honest-scaling law: visible circle == damage circle)

| # | Consumer | Uses TRUE AoERadius? | Fixed / action |
|---|----------|----------------------|----------------|
| 1 | **DT_Cards runtime table** | Only after reimport — the CSV edit is inert in-game until then | **TASK-240 dependency**: DT_Cards reimport + git-diff confinement check ride the integration task |
| 2 | **Damage circle** — `SpellLibrary.cpp` `ResolveTopTargetsDamage` (line 272) | YES — `Row.AoERadius` gates candidates; distance metric = closest point on target collision (qa/TASK-026 convention, slightly generous vs center — never smaller than shown) | None needed |
| 3 | **Reticle decal (visual circle)** — `SiegePlayerController.cpp:1736-1737` | YES — `ReticleRadius = (TargetingRow.AoERadius > 0) ? TargetingRow.AoERadius : SpellReticleDefaultRadius(150)`; `DecalSize = (500, R, R)`. Lightning's radius is > 0, so the 150 fallback is NEVER hit for it; decal is spawned fresh per targeting session (`TargetingRow = *Row` at :1477 → `SpawnSpellReticle()` at :1497), so 700 flows automatically post-reimport | None needed — the displayed circle honestly becomes 700 |
| 4 | **NS_Spell_Lightning VFX spawn** — `SpellLibrary.cpp` `SpawnSpellVFX` (:75-95) | **NO** — `SpawnSystemAtLocation(World, System, TargetPoint)` at UNIT scale, ground reticle point, no radius user-parameter passed | **Explicit note for TASK-239** (below). Not a code defect — the M5 VFX contract is null-safe fire-and-forget; the honest read must be authored in-system |
| 5 | **Bot rule 3b** — `SiegeBotController.cpp:629` `FindLightningTowerTarget(Chosen.Row->AoERadius, …)` | YES — radius passed from the row; 2D dist-squared in `double` (:941, no precision/overflow issue at 700); log line prints the row radius | None needed. Behavior at 700: rule 3b qualifies towers MORE often (defenders within 700 instead of 400 count), and every counted defender is genuinely inside the resolver's damage circle (bot's center-to-center 2D >= resolver's closest-collision metric ⇒ conservative). Consistent with the buff's intent |
| 6 | **Stale code comments** (doc text only, zero behavior): `SpellLibrary.cpp:193` "in a 400 radius", `SiegeBotController.cpp:620` "(400 — GDD §4…)", `SiegeBotController.h:247` "Lightning 400", `SiegePlayerController.cpp:1731` "Lightning 400" | n/a (comments) | Left untouched — the verify was mandated read-only/no-code-edits; flag for a future doc pass or TASK-236's editing window (it already owns SpellLibrary/SiegePlayerController files) |
| 7 | **cards.csv Notes cell** ("…enemies in 400 castle excluded…") | Not consumed by ANY code (grep: no readers of `.Notes` / `Row->Notes`) — inert, never player-visible via code | Frozen per the single-cell-diff law. If the manager wants the Notes text corrected to 700, that is a separate one-cell follow-up |

**Hardcode sweep:** no numeric `400` Lightning-radius literal exists anywhere in logic — every occurrence in `SpellLibrary` / `SiegePlayerController` / `SiegeBotController` is comment text (the only code `400.f` is the unrelated `MinerNodeApproachOffset`).

## TASK-239 notes (VFX re-skin spec inputs)

- **The current VFX does NOT scale with AoERadius.** `SpawnSpellVFX` spawns `/Game/VFX/NS_Spell_Lightning` at the confirm point with scale (1,1,1) and passes NO user parameters. The re-authored system must intrinsically read as a **700-radius strike**: author the ground-circle/impact footprint at 700 uu radius (1400 uu diameter) inside the system.
- **Strike height:** the system's spawn origin is the GROUND reticle point. "Originates visibly higher in the sky" must be authored in-system (beam/bolt emitter offset high above the system origin, striking down to origin). There is currently NO spawn-height or scale seam in code; if TASK-239 needs one (e.g. a `User.Radius` or spawn-Z parameter), that is a code change owned by the TASK-236 lane and must go through QA — do not assume it exists.
- Post-reimport, the targeting reticle will already show the honest 700 circle — the new VFX footprint should visually match that reticle circle.

## QA notes

- Acceptance was "single-cell diff; scaling verdict recorded" — please verify the diff confinement (`git diff -U0 -- Docs/Data/cards.csv` shows exactly one line, one cell) and audit rows 2/3/5 against the cited lines.
- No compile here by design — the mixed-tree compile + DT_Cards reimport + PIE spell suite are TASK-240.
- MaxTargets stays 3 (unchanged by design): a bigger circle widens the candidate pool, it does not strike more targets.

**Files touched:** `Docs/Data/cards.csv` (only).
**Refs read:** `Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.cpp`, `SiegePlayerController.{h,cpp}`, `SiegeBotController.{h,cpp}`, `CardRow.h`, CONVENTIONS "Spell delivery overhaul (2026-07-21)".
