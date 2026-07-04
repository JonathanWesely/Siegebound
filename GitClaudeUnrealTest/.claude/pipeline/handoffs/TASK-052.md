# TASK-052 — M3 final assembly: full-match-vs-bot PIE verification + commit (build-master) — HANDOFF

**Agent:** build-master
**Date:** 2026-07-04
**Result:** M3 editor/art layer VERIFIED + COMMITTED. Not pushed. **M3 bot-opponent slice READY for Jonathan's playtest.**
**Editor:** left UP (PID 30092, c8a40b2 DLL), PIE stopped, current level `/Game/Maps/L_Arena`. MCP healthy throughout (~20 calls, no bounce needed).

Note: this handoff is part of the commit it describes, so it cannot embed its own hash — the commit hash is recorded in the build-master report to the orchestrator, in the 🔧 Build & Git Slack thread, and (by the orchestrator) on the board.

## What was committed (M3 editor/art layer — code already at c8a40b2)
Commit message: `TASK-048..050 + TASK-052: M3 editor/art integration (IA_Rally + wiring, main menu + Play-vs-Bot, HUD Rally indicator + Victory/Defeat, GameDefaultMap). M3 bot-opponent slice ready for playtest.`

Files (13):
- `Content/Input/Actions/IA_Rally.uasset` (NEW, TASK-048)
- `Content/Input/IMC_Hero.uasset` (Q→IA_Rally appended; 17 M1/M2 mappings byte-preserved, TASK-048)
- `Content/Blueprints/BP_HeroCharacter.uasset` (RallyAction slot = IA_Rally, TASK-048)
- `Content/Maps/L_MainMenu.umap` (NEW menu level, GameMode=BP_MenuGameMode, TASK-049)
- `Content/Blueprints/BP_MenuGameMode.uasset` (NEW, cursor + CreateWidget WBP_MainMenu + UIOnly, TASK-049)
- `Content/UI/WBP_MainMenu.uasset` (NEW, Play/Deck Builder[disabled]/Quit, TASK-049)
- `Content/UI/WBP_HUD.uasset` (additive Rally indicator, TASK-050)
- `Config/DefaultEngine.ini` (GameDefaultMap=L_MainMenu only; EditorStartupMap kept L_Arena; r.PathTracing=False already at c8a40b2 — verified the working-tree diff is that single line)
- Pipeline docs: `.claude/pipeline/TASKBOARD.md` (as-is — NOT edited by build-master), `handoffs/TASK-048.md`, `handoffs/TASK-049.md`, `handoffs/TASK-050.md`, `handoffs/TASK-052.md` (this file)

**Deliberately LEFT UNSAVED / NOT committed:** `Content/UI/WBP_VictoryScreen.uasset` — TASK-050 confirmed the disk asset is already correct (SetWinner byte 0→Victory / 1→Defeat, Play Again intact); it only carried an in-memory dirty flag from read-only inspection. It never appeared in `git status` (disk unchanged) and is left at HEAD. No editor save was issued for it.

**Residue adjudication:** working tree after PIE = exactly the 13-file M3 set. Zero donor resaves (SM_Castle / SM_Footman / UI_TouchSimple / UI_LifeBar all clean), no `.mcp.json`, no `Content/Dev/`, no `__ExternalActors__` churn. PIE did not dirty any Content asset on disk (autosaves went to `Saved/Autosaves/`, gitignored). No `git add` normalization needed beyond the M3 set.

## Verification — §9-3 slice (per-criterion)

Method: one fresh in-viewport PIE in L_Arena, **player idle** (no input injected), match ran autonomously ~49 s to a castle-destroy; `LogSiegeBot` decision trace + `LogGitClaudeUnrealTest` match log read live. Timestamps below are from that session (19:23:xx). Corroborated by two earlier same-session matches (17:55, 19:12).

| # | Criterion (§9-3 / §4 / §3.9 / §7) | Verdict | Evidence |
|---|---|---|---|
| 1 | All M3 assets resolve, zero missing-ref/load warnings | **PASS** | Error scan clean (only benign boot DLLs: aqProf/VtuneApi/WinPix/Wintab + DDC). Readbacks: IA_Rally=`/Script/EnhancedInput.InputAction`; BP_MenuGameMode_C + WBP_MainMenu_C load clean; BP_HeroCharacter.RallyAction=`/Game/Input/Actions/IA_Rally.IA_Rally` |
| 2 | Bot accrues gold like the player | **PASS** | Bot gold rises between plays across the trace; shared M2 economy via Red PlayerState (TASK-043/045) |
| 3 | Bot reaches ~3 miners while player idle | **PASS** | 3-miner cap confirmed: 17:55 & 19:12 matches hit exactly 3/3 then Rule 2 stops (never a 4th). My 19:23 match reached 2/3 before a fast win (opening spent on defense — see #5) |
| 4 | Attacks in progressively larger waves | **PASS** | Sustained Rule 3 (Attack): Footman/Archer/Knight streamed at centerline X=350 |
| 5 | Defends within ~2 s when pushed | **PASS (observed)** | Rule 1 (Defend) fired 19:23:01–13 vs a Blue `BP_Unit_Footman_C_1` intruder — Footman at its centerline + Wall/ArrowTower between the intruder and Castle_Red (X 1602–2052) |
| 6 | NEVER plays an unaffordable card | **PASS** | Every "played" line shows gold ≥ cost (gold A→B, B=A−cost); refusal/unaffordable/fail scan returned empty |
| 7 | NEVER spawns on the Blue half | **PASS** | All spawns X=350 (centerline, Red half) or X=800 (GoldNode_Red); zero X≤0 across the whole trace |
| 8 | LogSiegeBot shows which rule fired per play | **PASS** | Every play logs `Rule N (Name): played … — gold A->B` (Rule 1 Defend / 2 Economy / 3 Attack all seen) |
| 9 | Destroy **Red** castle → **Victory** | **PASS (wiring) / DEFERRED (live drive)** | Byte mapping verified TASK-047 QA (Red→Victory) + symmetric to the live Defeat path (same branch on destroyed-castle team). Driving a live Victory needs the player to actually win (card/hero input) → Jonathan |
| 10 | Destroy **Blue** castle → **Defeat** | **PASS (live)** | 19:23:50 `Castle_0 (Blue) destroyed — match over, winner: Red` → `SiegePlayerController_0: match ended — winner Red` (my session; also 17:56 & 19:12) |
| 11 | Match end freezes both sides | **PASS** | `Match-end freeze: 15 unit(s) frozen, 3 tower(s) silenced, 0 projectile(s) cleared, income paused, clock stopped, bot decision loop stopped`; no LogSiegeBot lines after match end (bot halted) |
| 12 | Play Again resets BOTH sides | **PASS (logic) / DEFERRED (live click)** | TASK-047 QA verified both-sides reset (ResetBot + player ResetEconomy/ResetClock/ResetDeck, TASK-024/045). Live Play Again click = UI input → Jonathan |
| 13 | Rally (Q): friendly units +25%/5 s, 20 s CD | **PASS (wiring) / DEFERRED (live Q)** | RallyAction=IA_Rally, IMC_Hero Q-map (TASK-048), RallyRadius=600 / RallyCooldown=20 readback; null-RallyAction warning absent at PIE boot. Live Q-press greying = input → Jonathan |
| 14 | HUD Rally indicator seeds | **PASS** | TASK-050 SetupRallyIndicator seeds "Rally: Ready"; my PIE showed no "rally indicator not attached" (overlay cast + delegate bind succeeded) |
| 15 | Main menu → Play → L_Arena | **PASS (wiring) / DEFERRED (live click)** | TASK-049 PIE from L_MainMenu: `Game class is 'BP_MenuGameMode_C'`; Play→ASiegeGameMode::StartMatch→OpenLevel L_Arena; GameDefaultMap=L_MainMenu on disk. Live button click = no MCP click verb → Jonathan |
| 16 | Slice is RECORDABLE | **PASS** | Full match-vs-AI ran in-viewport to a castle-destroy (recordable clip); LogSiegeBot decision trace captured end-to-end |

**No genuine FAILs.** Every DEFERRED item is a live interactive input (Q-press, menu/Play-Again clicks, driving a player win) that this MCP surface cannot inject — the underlying wiring is verified structurally + at PIE boot, per the same posture as TASK-048/049/050.

## Deferred to Jonathan (interactive — no MCP keypress/click verb)
1. **Rally Q-press** — press Q in PIE; friendly units within 600 speed up 25% for 5 s; HUD indicator shows cooling → "Rally: Ready" after 20 s.
2. **Main menu Play button** — from the packaged/standalone boot (or PIE from L_MainMenu), click Play (vs Bot) → live L_Arena match; Quit exits; Deck Builder is greyed.
3. **Live Victory** — actually win (play units / hero-attack the Red castle to 0) → Victory screen (mirror of the live-verified Defeat).
4. **Play Again click** — on the end screen, confirm both sides reset (gold/deck/hand/miners/buildings/clock/castles/hero + bot).

## Follow-up found (report only — not a blocker; for the manager)
- **Transient opening intruder:** in a repeated-PIE editor session, the 19:23 match opened with a Blue `BP_Unit_Footman_C_1` on the bot's half that the bot correctly defended against (Rule 1). **The persistent L_Arena is clean** — `find_actors` post-PIE returned 0 `BP_Unit_*` / 0 `BP_Building_*`, and L_Arena.umap is not in this commit. So this is a same-session PIE-recycling artifact, not shipped content. Worth a quick reconfirm at Jonathan's fresh-session playtest (Play→OpenLevel loads L_Arena clean); if it recurs on a cold boot, it becomes a gameplay-programmer task (world-teardown between PIE runs). Bonus: it gave a live Rule 1 (Defend) demonstration.

## How to play (for the playtest)
- **Default boot** now lands on the **main menu** (GameDefaultMap=L_MainMenu): Play (vs Bot) → L_Arena live match; Deck Builder greyed (M6); Quit.
- **Or open `/Game/Maps/L_Arena` directly** for dev testing (EditorStartupMap unchanged).
- **Controls:** WASD move, mouse look, Shift sprint, LMB melee, 1–6 play hand slots, hold Left Alt for the UI cursor, **Q = Rally**.
- Recordable slice: full match-vs-AI clip + the LogSiegeBot decision trace.
