# TASK-061 — DT_Cards reimport: 22-row Set II + M4 test deck (editor) — HANDOFF

**Agent:** gameplay-programmer
**Date:** 2026-07-04 (M4 editor-wave task 1/4; owned the editor BOOT)
**Status:** ready-for-qa
**Editor left:** UP with MCP reachable (PID 6172) for TASK-062/063/064/069. NOT closed.

## STEP 0 — Editor boot outcome (non-trivial; recovered a hung startup)

Boot was NOT the fast/clean ~30s the spec anticipated. Sequence:

1. Probed `list_toolsets` → down (expected). Launched `UnrealEditor.exe` DETACHED via PowerShell `Start-Process` (NOT Bash background — honored the hard-won lesson).
2. Port 8000 came up and the engine log reached `LogInit: Engine is initialized. Leaving FEngineLoop::Init()` + `LogModelContextProtocol: registered 3 meta-tools` — so the MCP server DID start and the process did NOT crash. But every MCP call timed out / "unable to connect", and the log froze at engine-init (frame counter stuck at `[0]`) for ~11 min.
3. Diagnosed via the process MainWindowTitle = **"Restore Packages"**: the first boot HUNG on the startup SPackageRestore modal (it spins its own blocking Slate loop, so startup never advanced and the game thread never reached the tick that services MCP). `SetForegroundWindow` was blocked by Windows; the Slate modal is not exposed to UIAutomation, so it could not be dismissed programmatically.
   - **NB on the crash heuristic:** a `CrashReportClientEditor.exe` process WAS present, but that is UE5's benign out-of-process crash *monitor* that always spawns with the editor — it was NOT a crash (editor process stayed alive, log clean). The real signal here was the frozen log + modal title, not the monitor's presence.
4. **Root cause:** `Saved/Autosaves/PackageRestoreData.json` (from a prior session at 13:56) held one stale recovery pointer: `/Game/UI/WBP_VictoryScreen` → auto-save `WBP_VictoryScreen_Auto4.uasset`. Unrelated to build-master's clean close on 65861ce.
5. **Recovery (declined the restore, the correct+safe choice):** killed the hung editor + crash monitor, MOVED `PackageRestoreData.json` aside to the session scratchpad (`.../scratchpad/restore-backup/PackageRestoreData.json`), and relaunched detached. This is equivalent to clicking "Don't Restore" — the committed on-disk WBP_VictoryScreen is untouched; restoring the stale unsaved edit would have created exactly the working-tree residue the pipeline forbids. `Saved/` is gitignored, so moving the file produces zero git residue. The inert `WBP_VictoryScreen_Auto4.uasset` remains in `Saved/Autosaves/Game/UI/` but has no pointer now.
6. Relaunch booted clean: window title `GitClaudeUnrealTest - Unreal Editor`, no modal, frame counter advancing (`[8]/[10]/[11]`), `list_toolsets` returned all toolsets. Total boot-to-ready incl. the recovery ≈ 13 min wall clock (would have been ~1 min without the modal).

## STEP 1 — Reimport approach (reference-safe in-place rebuild; TASK-031 pattern)

Epic's native MCP (UE5.8 EditorToolset) still has no reimport verb and `import_file` refuses to overwrite an existing asset, and `/Game/Data/DT_Cards` is referenced by `/Game/UI/WBP_HUD` + `/Game/UI/WBP_CardHand` (delete+recreate forbidden). So used the same reference-safe path:

- `DataTableTools.add_rows` for the 16 Set II row names, then `DataTableTools.set_rows` (4 batches) driving **every** value straight from `Docs/Data/cards.csv`. Row struct `/Script/GitClaudeUnrealTest.CardRow` (FCardRow) preserved; asset GUID + CSV import-source linkage intact.
- **Also overwrote the 6 core rows' `deckCount`** — the M4 CSV redistributes the deck (core sum dropped from the old M1 50 to 20 so Set II can add 30). Left as-is they'd have summed to 80.
- Exact camelCase keys the tool uses (non-obvious): `hP`, `aoERadius`, `spawnCardId` (lowercase d), `bRanged/bCharge/bSlayer/bSuicide`.

## STEP 2 — Verification (read back from the SAVED state via get_rows)

`list_rows` = exactly the 22 expected rows in order, no legacy/extra/dupe rows:
Footman, Archer, Knight, Miner, ArrowTower, Wall, MilitiaMob, Pikeman, Sapper, Cavalry, Longbowman, Cleric, Ogre, BombTower, BallistaTower, Barracks, DeepMine, Masons, SharpenedBlade, PlateArmor, SwiftBoots, WarBanner.

**DeckCount sum = 50** (the M4 test deck):
- Core (20): Footman 6, Archer 4, Knight 2, Miner 3, ArrowTower 2, Wall 3
- Set II units (14): MilitiaMob/Pikeman/Sapper/Cavalry/Longbowman/Cleric/Ogre = 2 each
- Set II buildings/econ/util (10): BombTower/BallistaTower/Barracks/DeepMine/Masons = 2 each
- Hero upgrades (6): SharpenedBlade 2, PlateArmor 2, SwiftBoots 1, WarBanner 1

**New keyword columns — set ONLY where §4 specifies:**
| Column | Set on (value) | Everywhere else |
|--------|----------------|-----------------|
| bRanged | Archer, ArrowTower, Longbowman, BombTower, BallistaTower = true | false |
| bCharge | Cavalry = true | false |
| bSlayer | Pikeman = true | false |
| bSuicide | Sapper = true | false |
| swarmCount | MilitiaMob = 4 | 0 |
| aoERadius | Sapper 250, BombTower 250 | 0 |
| minRange | BallistaTower 300 | 0 |
| spawnCardId | Barracks = Footman | None |
| spawnInterval | Barracks = 8 | 0 |
| lifetime | Barracks = 60 | 0 |

Profiles: Siege = Sapper, Ogre; Support = Cleric; Standard = the other combat units; None = economy/buildings/upgrades. Every core-stat (cost/maxCopies/HP/damage/range/cadence/speed) matches §4/cards.csv exactly (spot-checked all 22 rows twice — once pre-save, once post-save).

## STEP 3 — Save
Targeted `save_assets(["/Game/Data/DT_Cards"])` → true (NOT save-all; no donor re-dirty). `is_dirty` = false before AND after a re-read. Save log shows clean `SavePackage` + `AssetCheck: Validating asset`. Left the `.uasset` for TASK-069 to commit (auto-stage OK; do NOT commit here).

## Warnings / items for QA to scrutinize
1. **CSV auto-reimport activity in the log (benign):** `LogCSVImportFactory: Imported DataTable 'DT_Cards'` fired several times during the edit window — the editor auto-reimporting from the newer `cards.csv` source (source linkage is alive, correct per GDD §3.0), interleaved with Jonathan at the keyboard. The FIRST such reimport logged "**352 Problems**" (a burst of `Row 'X' is missing an entry for column 'Y'` on the tail rows — a transient parse pass); every subsequent reimport logged "**0 Problems**". All imports draw from the same 22-row cards.csv I set from. The FINAL saved state was re-verified clean via `get_rows` AFTER the last reimport and AFTER the save. Net: no data impact.
2. **No `Missing RowStruct while saving` error this time** (the benign TASK-031 log did NOT recur). Save was clean.
3. **Boot recovery moved a `Saved/` file** (`PackageRestoreData.json`) — gitignored, backed up in scratchpad, zero working-tree impact.

## Constraints honored
- No Git, no compile. Did NOT edit TASKBOARD.md (orchestrator flips the board).
- Targeted save only; no donor assets re-dirtied. Editor left UP with MCP reachable.

## Files/assets touched
- `/Game/Data/DT_Cards` (DataTable rows rebuilt to the 22-row Set II + M4 test deck; saved). Source of truth remains `Docs/Data/cards.csv` (unchanged on disk).
- Moved aside (Saved/, gitignored): `Saved/Autosaves/PackageRestoreData.json` → scratchpad backup.
