# TASK-109 handoff — M5 final assembly: exit-criteria PIE verification + commit + m5-testable branch (build-master)

- **Author:** build-master
- **Date:** 2026-07-08 (PIE run wall-clock 2026-07-09 ~01.40 local)
- **Commit:** `979f5522fe6a4e38b7ed91d3c78b88edef6be06f` on `main` (NOT pushed)
- **Branch:** `m5-testable` cut at `979f552` (NOT pushed). Still on `main`.
- **Baseline:** HEAD was `2c65164` (TASK-103 code batch); no partial TASK-109 commit pre-existed.

## Desktop-lock mode: LOCKED → machine-only verification

Probed three independent ways, all agree the workstation is **LOCKED**:
1. `LogonUI.exe` running (PID 8464, Console session) — present only while the lock/logon screen is up.
2. Win32 `GetForegroundWindow()` returned `0` — foreground is on the secure desktop, invisible to a normal-session process.
3. Consistent with the prior TASK-103 session's finding + standing TASK-076 doctrine.

**Consequence:** every input-driven check (SendInput hotkey spell plays, LMB reticle confirm, RMB/Esc cancel, Play Again click, and dismissing the two passive editor prompts) goes to the **WATCH list per TASK-076 doctrine** — recorded, not faked. All MCP calls, PIE start/stop, readbacks, and log reads work regardless and were performed in full. The two passive prompts ("7 source content changes — import?" and the re-open-asset-editors state) could NOT be dismissed (locked); they are passive/non-blocking and were never actioned, so the TASK-106 CardArt was NOT re-imported/churned.

## Exit-criteria table (check by check)

| # | Exit criterion | Result | Evidence / reason |
|---|---|---|---|
| A | Pre-PIE: DT_Cards 28 rows live, exact cards.csv order | **VERIFIED** | `list_rows`=28 exact order; TASK-106 deferred CardArt check CLOSED — Footman (M4), Fireball + CrystalTower (Set III) all carry `/Game/UI/CardArt/T_CardArt_<CardID>` soft paths that resolve to real assets |
| B | Pre-PIE: all Set III assets at exact paths | **VERIFIED** | `exists`=true for SM_CrystalTower, NS_Spell_{Fireball,FrostNova,Lightning,BattleCry,Pickpocket}, NS_ChainZap, M_SpellReticle (8/8) |
| C | Pre-PIE: BP_Building_CrystalTower loads as ATower subclass | **VERIFIED** | `search_subclasses(ATower,"CrystalTower")` → exactly `BP_Building_CrystalTower_C` |
| D | Pre-PIE: /Game/UI/CardArt = 28 textures | **VERIFIED** | `find_assets` → 28 (22 M4 + 6 Set III), all names char-exact |
| E | PIE boots on L_Arena | **VERIFIED** | `get_current_level`=/Game/Maps/L_Arena; PIE log "Created PIE world by copying editor world from /Game/Maps/L_Arena" |
| F | Bot self-drive; renumbered rules 1-5 fire with new labels | **PARTIAL — VERIFIED (4,5) / SOURCE-VERIFIED (1,2,3)** | Live: `Rule 4 (Attack)` (played Longbowman/Cavalry/MilitiaMob/Footman ×8) and `Rule 5 (Cycle)` (discarded unplayables incl. Pickpocket) fired with the new labels. Rules 1 (Defend), 2 (Economy), 3 (SPELLS) did NOT arise vs an idle player (no threat / economy satisfied / no player targets); their renumbering + labels are source-verified in SiegeBotController.cpp (Defend:369, Economy:412/448, SPELLS:464, Attack:579, Cycle:622) |
| G | Bot economy/waves unregressed (M3 acceptance) | **VERIFIED** | Gold cycled with visible income regen between plays (spent to 6, next play began at 12/13); 8 unit waves in ~70 s; deck built 50-from-28; clean instrumented match-end freeze |
| H | Deck = 50 with Set III reachable | **VERIFIED** | Log: both player and bot `UDeckComponent` "built a 50-card draw pile from 28 card rows"; TASK-104 readback: DeckCount sum=50, Set III carve (Fireball 2/FrostNova 1/Lightning 2/BattleCry 1/Pickpocket 1/CrystalTower 1) |
| I | All 5 spells playable end-to-end w/ visible VFX | **WATCH** | Player casting is input-driven (locked desktop). Assets present (B) + code paths QA-passed (TASK-098/100/102). Live cast + §6 VFX read requires an unlocked desktop |
| J | Fireball §3.11 (3× 80-HP Footmen, adj. friendly safe, 50 vs castle) | **WATCH** | Needs a player Fireball cast |
| K | FrostNova freezes units + tower 4 s, castle unaffected, clean resume | **WATCH** | Needs a player cast |
| L | Lightning kills Arrow Tower via top-3 current-HP in 400 | **WATCH** | Needs a player cast |
| M | BattleCry speeds attack + move 8 s | **WATCH** | Needs a player cast |
| N | Pickpocket moves exactly min(10, victim gold) | **WATCH** | Needs a player cast (bot has no cast rule — it discarded Pickpocket via Rule 5, documented) |
| O | Crystal Tower chains 15/10/5 within 800 | **WATCH** | BP + SM + chain code verified present; needs a placed Crystal Tower with enemy units in range 800 — none arose (idle player) |
| P | Reticle projects to ground; LMB confirm (gold at confirm); RMB/Esc cancel free | **WATCH** | Input-driven; M_SpellReticle asset + material graph verified present (TASK-108) |
| Q | Bot casts Fireball + Lightning per rules w/ LogSiegeBot traces | **WATCH — "bot casts spells"** | Rules 3a/3b require PLAYER-side targets (≥3 clustered player units / a player tower w/ 2+ units within 400). An idle player produced none, and MCP SceneTools operate on the EDITOR world (not the live PIE `GWorld`), so player-team targets could not be injected into the running sim without polluting L_Arena (TASK-070). Rule 3 SPELLS block is source-verified + TASK-102 QA-passed; only live firing is WATCH-listed |
| R | Play Again clears all spell state (freeze/buff timers) | **WATCH** | Play Again is an input-driven UI click; also no spell state was ever created this run (I,Q WATCH-listed), so nothing to observe resetting |
| S | StopPIE cleanly | **VERIFIED** | StopPIE → IsPIERunning=false; clean `BeginTearingDown` + PIE online-subsystem shutdown, no teardown errors |

**Match outcome note:** with an idle player (locked desktop = no defense), the bot's unit stream destroyed the undefended Blue castle in ~71 s — `[SiegeGameMode_0] Castle 'Castle_0' (Blue) destroyed — match over, winner: Red` — so the run self-terminated well before a 3-4 min drive. This is expected M3 behavior vs a do-nothing player, not a regression; it also means rules 1/2/3 and any spell activity never had the game state to arise.

## Log-sweep triage (vs knowns + new)

| Item | Class | Note |
|---|---|---|
| victory-focus: `LogPlayerController: Error: InputMode:UIOnly - Attempting to focus Non-Focusable widget` @ match-end | **KNOWN — benign** | Fires once at 01.41.32 as the game-over UI takes UIOnly focus; documented known |
| CrowdFollowing teardown: `LogCrowdFollowing: Warning: Unable to find RecastNavMesh instance while trying to create UCrowdManager instance` @ StopPIE | **KNOWN — benign** | Standard PIE teardown; also covers the RecastNavMesh-boot known |
| DeepMine CardType-2 | **KNOWN — did not arise** | Bot never played DeepMine (Rule 2 didn't fire this match); nothing to trigger it |
| `LogCSVImportFactory: Row 'Fireball'/'FrostNova'/... missing an entry for 'CardType'` @ 22:26 (pre-PIE) | **FINDING — benign** | Stale CSVImportFactory enum-parse quirk from an earlier direct-import attempt. cards.csv HAS CardType populated (col 3 = Spell/Building) and live DT_Cards readback confirms all 28 CardTypes correct. This quirk is exactly why TASK-104 used `set_rows` not `import_file`. No action; DataTable is correct |
| `LogAudioMixerWasapi/LogAudioMixer: Error: AUDCLNT_E_DEVICE_INVALIDATED / device swap failed` (recurring whole session) | **FINDING — environmental** | Machine display-audio device (S2-L) unavailable; Windows falls back. Independent of PIE, zero gameplay impact |
| `LogModelContextProtocol: Error: Unknown session id` @ 22:21 (pre-PIE) | **benign** | MCP transport reconnect from an earlier session |

**No new game-code errors or warnings** appeared during the PIE window (01.40.21–01.41.40) beyond the victory-focus known.

## Commit / residue adjudication

- **Residue:** at start the UE Git provider had auto-staged BP_Building_CrystalTower (`A `, no worktree drift). No asset was saved/mutated this session and the import toast was never actioned, so no resave race occurred. Per TASK-103 doctrine I ran `git reset` and re-staged the whole set FRESH from the worktree, then verified **staged == worktree: 53 files, zero split (AM/MM) states, nothing left behind**.
- **L_Arena.umap correctly EXCLUDED** — not dirty (no editor edits, no actors placed; PIE does not resave the editor level).
- **DT_Cards.uasset** included as `M` (legitimate TASK-104 in-place work, not a boot resave).
- **Commit `979f552` — 53 files:** BP_Building_CrystalTower, DT_Cards, M_CrystalGlow, M_SpellReticle, SM_CrystalTower, 6× T_CardArt_ (Set III), 6× VFX (NS_Spell_×5 + NS_ChainZap), 7× RawAssets (6 PNG + CrystalTower.fbx), Content/Fab/README_DROP_ZONE.md, and pipeline docs (TASKBOARD/CONVENTIONS/FAB-REQUESTS + handoffs TASK-093..108 + 3 TASK-105 PNGs + qa reports TASK-093..102). Message lists TASK-104..109. LFS handled the binaries.
- **Branch `m5-testable`** cut at `979f552`. **NOTHING pushed** — `main` is `[origin/main: ahead 2]` (2c65164 + 979f552, both local); 979f552 is on no remote branch.
- This TASK-109.md handoff is written post-commit (carries the real hash) so it is untracked — it rides the orchestrator's follow-up commit alongside the TASKBOARD status flips (standing pattern).

## Follow-ups for the manager (turn into tasks as fit)

1. **Unlocked-desktop playtest is required to close the M5 slice** — all live spell behavior (I–P), the "bot casts spells" trace (Q), and Play-Again spell-state reset (R) are WATCH-listed solely because the desktop was locked and MCP cannot inject player-team targets into the live PIE world. A single human playtest with an unlocked desktop closes I–R plus the carry-forward WATCHes below.
2. **Carry-forward WATCHes (need live spells / human eyes):** elevated-anchor ring readability (TASK-100), spell hand-clog + centroid-outlier coverage (TASK-102), §6 one-frame VFX readability of the six restyled Niagara systems (TASK-108). All await the same playtest.
3. **CSVImportFactory enum-parse quirk** (log finding above) — informational; cards.csv/DT_Cards are correct. Worth a one-line CONVENTIONS note that direct cards.csv → DataTable import mis-parses the CardType enum (hence `set_rows`).
4. **Audio-device errors** are environmental (machine audio hardware), not a code issue.
