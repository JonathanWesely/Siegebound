<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
## M3 tasks (decomposed 2026-07-04 — HELD)

### M3 COMPLETE — 2026-07-04 (done; committed, not pushed; read this first)
All M3 tasks integrated + committed on `main`, **NOT pushed**: C++ batch TASK-042..047 at **2f6a8fc** (via TASK-051, parent f903cf0, clean compile first try); boot-crash fix + Rally binding + unity-ODR dedupe at **c8a40b2** (r.PathTracing=False); editor/art TASK-048..050 + assembly TASK-052 at **56247c9** (parent c8a40b2). Milestone preserved on branch **m3-testable @ 56247c9** (also **m2-testable @ f903cf0**). TASK-042..052 all `done`; blocks left in place (not relocated to ## Done) to preserve the M3 audit trail.
- **Slice verified (TASK-052):** §9-3 slice — 16 criteria, 0 genuine FAILs. LIVE PASS: bot self-drives (gold accrual, miner cap 3, growing waves, Rule-1 defend within 2 s, never-unaffordable, never-Blue-half, LogSiegeBot decision trace, match-end freeze both sides, live Blue-castle→Defeat).
- **DEFERRED to Jonathan's playtest** (verified structurally; MCP can't inject input): Q-Rally greying, menu Play / Play-Again clicks, live Victory drive. Editor was left UP (PID 30092) on the c8a40b2 DLL + M3 editor assets — M3 is playable now.

### M3 carry-forwards (do NOT lose these — folded into M4 where noted)
- **(a) WATCH — transient Blue unit at match start.** A repeated-PIE session (TASK-052) showed a transient Blue unit appear at match start; the persistent L_Arena is clean. Needs a **cold-boot reconfirm at Jonathan's playtest**; becomes a world-teardown/PlayAgain-residue task **only if it recurs**. (Also re-echoed in the ## M4 tasks WATCH block.)
- **(b) TASK-046 WARN-2 — bot rule-4 discard path unhardened.** The bot's "discard most-expensive UNPLAYABLE card" path has no `DiscardFromHand` return-check / fee guard; it was dormant in M3 because the core deck is all-playable. **M4 introduces non-unit / keyword / hero-upgrade / Instant cards → this path goes LIVE → hardening is folded into TASK-060 (Bot v2).**
- **(c) M2 TASK-041 — visual hand UI manual UMG pass — STILL OPEN.** The functional data path shipped (aafd968); the 6-slot visual hand tree + HUD stat texts remain a manual/Jonathan-assisted UMG session (MCP can't author widget trees). Carried into M4; **it shares WBP_HUD with TASK-064 (HUD upgrade row) — do TASK-041 first or fold, so the two UMG passes don't collide** (same lesson as M3 TASK-050).

### M3 — ACTIVE (Jonathan authorized M3 on `main` 2026-07-04; safety branch `m2-testable` @ f903cf0 preserves the M2-testable state — the HELD/interference text below is SUPERSEDED, M3 may disrupt main's M2 testability by design)
**Interference gate (overnight constraint):** adding ANY new `.cpp`/`.h` to the Siegebound module makes the editor rebuild-on-boot, which could break Jonathan's in-progress M2 playtest. So **every M3 task — including the file-only C++ ones — is HELD tonight**: decomposed-and-ready, NOT dispatched. On M2 sign-off, dispatch order: file-only C++ wave first (TASK-042 + TASK-043 in parallel — different files), then TASK-044 (after 042) / TASK-045 (after 043), then TASK-046 (after 044+045) and TASK-047 (after 045; parallel with 046 — different files); build-master compiles the batch (TASK-051); editor tasks 048/049/050 after the batch compiles; TASK-052 verifies last. **Every code task (042–047) implies a QA review** (standard qa loop). M3 = GDD §9-3 + §4 Bot Opponent + §4 hero Rally + §7 main menu.

**M3 design rulings (binding for all M3 tasks):**
- **Bot = `ASiegeBotController : AAIController`, possesses no pawn** (§4 "controls no hero"). Spawned by `ASiegeGameMode` at match start on the **Red** team; `bWantsPlayerState = true` so it auto-creates a Red `ASiegePlayerState` → the M2 economy (accrual, overtime rate, miner income, Pause/Resume/ResetEconomy) is reused verbatim for the bot. It owns a `UDeckComponent` (TASK-022, unchanged — the component is controller-agnostic).
- **Multi-team economy:** `ASiegePlayerState` gains a `Team` (ETeamId) tag; `ASiegeGameState::GetPlayerStateForTeam(ETeamId)` iterates `PlayerArray`. Miners and any team-economy consumer resolve their economy through that accessor instead of assuming the single player (M2 assumed one). Player PS = Blue, bot PS = Red (GameMode sets both).
- **Bot units are Red at spawn** via the team-material rule (TASK-044): the bot reuses the SAME `BP_Unit_*` / `BP_Building_*` assets the player uses; the spawn path sets `Team=Red` and BeginPlay applies `MI_TeamColor_Red`. No Red-specific BP duplicates.
- **Bot spawn geometry:** units at the bot's centerline (just inside X≥0), miners to `GoldNode_Red`, defensive towers between the nearest intruder and `Castle_Red`. Reuses the player's placement validity (own half = X≥0 for Red, navmesh projection, building clearance 200).
- **Rally (§4 hero active):** units-only buff (NOT the hero); +25% move & sprint for 5 s to friendly units within 600; 20 s cooldown; key **Q**. Values are UPROPERTY defaults with `// GDD §4` comments (mechanic rule, not CSV).
- **Win/lose (§3.9):** Blue castle destroyed → **Defeat**; Red castle destroyed → **Victory**; winner passed to `WBP_VictoryScreen::SetWinner` (byte param, existing M1 deviation). Bot economy/deck/units reset on Play Again alongside the player.
- Unchanged laws still in force: stats in DT_Cards/cards.csv, `Variant_*` donors READ-ONLY, `L_Arena` is the arena, local player always Blue, only ONE editor-mutating task at a time.

### TASK-042 — Hero Rally ability + unit move-speed buff API (C++)
- assignee: gameplay-programmer
- status: done (committed 2f6a8fc via TASK-051, not pushed; qa-passed — qa/TASK-042-report.md — 0 blockers, 0 warns, 3 nits; all 3 flagged items ACCEPTED; full MaxWalkSpeed-writer census confirms zero drift risk; FreezeAI zero-residual verified; C4458 clean; M1/M2 byte-preserved). handoffs/TASK-042.md. Ready for TASK-051 M3 batch compile.
- blocked-by: none
- parallel-safe: yes (parallel with TASK-043; TASK-044 serializes AFTER it — shared SummonedUnit files)
- spec: >
    Files only. GDD §4 (Player Hero active ability) + §3.8. (1) ASummonedUnit: add
    ApplyMoveSpeedBuff(float Multiplier, float Duration) (BlueprintCallable) — applies a temporary max-walk-
    speed multiplier and restores the base after Duration via timer; re-applying REFRESHES the duration (no
    stacking) and never permanently drifts the base (store the base once, restore exactly — the TASK-020
    drift-free lunge lesson). FreezeAI must clear the buff timer and restore base speed. (2) AHeroCharacter:
    Rally() (BlueprintCallable, bound to IA_Rally in TASK-048): if off cooldown, iterate every friendly
    (same-team ITeamAgent) ASummonedUnit within RallyRadius and call ApplyMoveSpeedBuff(1.0 + RallySpeedBonus,
    RallyDuration); start RallyCooldown; broadcast FOnRallyStateChanged(bool bReady, float CooldownRemaining)
    on use and when it comes back ready. UPROPERTY defaults (// GDD §4): RallyRadius=600.f, RallySpeedBonus=
    0.25f, RallyDuration=5.f, RallyCooldown=20.f. Null-safe; a press on cooldown is a no-op (optional refusal
    broadcast). Acceptance: pressing Rally speeds every friendly unit within 600 by 25% for 5 s then restores
    EXACTLY, does nothing to the hero, is unusable again for 20 s, and the delegate reports cooldown state;
    enemy units unaffected; FreezeAI cancels an active buff cleanly with no residual speed.
- names: >
    AHeroCharacter (Siegebound/HeroCharacter.h/.cpp) — Rally(); UPROPERTYs RallyRadius, RallySpeedBonus,
    RallyDuration, RallyCooldown, UInputAction RallyAction; delegate FOnRallyStateChanged (member
    OnRallyStateChanged). ASummonedUnit (Siegebound/SummonedUnit.h/.cpp) — ApplyMoveSpeedBuff. Input asset
    (TASK-048): /Game/Input/Actions/IA_Rally.

### TASK-043 — Multi-team economy: PlayerState Team tag + GetPlayerStateForTeam + miner team-resolution (C++)
- assignee: gameplay-programmer
- status: done (committed 2f6a8fc via TASK-051, not pushed; qa-passed — qa/TASK-043-report.md — 0 blockers, 1 warn, 2 nits; all 3 flagged decisions ACCEPTED; M2 Blue-side economy byte-for-byte CONFIRMED; InitNewPlayer signature verified UE 5.8; C4458 clean). WARN (non-blocking, carry to TASK-044 MinerUnit pass): mis-teamed miner's 0.25s retry poll re-logs ~4/s — the null-PS branch lacks the one-shot guard; never fires in designed flows. handoffs/TASK-043.md. Ready for TASK-051 batch compile.
- blocked-by: none
- parallel-safe: yes (parallel with TASK-042 — different files)
- spec: >
    Files only. Enables two coexisting economies (player Blue + bot Red). (1) ASiegePlayerState: add UPROPERTY
    Team (ETeamId, default Blue); ASiegeGameMode sets the player PS Team=Blue and (once TASK-045 lands) the
    bot PS Team=Red at creation. (2) ASiegeGameState: add ASiegePlayerState* GetPlayerStateForTeam(ETeamId
    Team) — iterate the GameState PlayerArray, return the ASiegePlayerState whose Team matches (null + log if
    none). (3) AMinerUnit and any other team-economy consumer resolve their economy via
    GetPlayerStateForTeam(OwnTeam) instead of assuming the single/first player state. Preserve ALL M2 income
    behavior byte-for-byte for the Blue player. Acceptance: with a Blue player PS and a Red bot PS present,
    GetPlayerStateForTeam returns the correct one per team; a Red miner arriving at GoldNode_Red raises only
    the BOT's rate (player's unchanged) and vice-versa; killing a Red miner drops only the bot's rate.
- names: >
    ASiegePlayerState (Siegebound/SiegePlayerState.h/.cpp) — UPROPERTY Team. ASiegeGameState
    (Siegebound/SiegeGameState.h/.cpp) — GetPlayerStateForTeam. ASiegeGameMode
    (Siegebound/SiegeGameMode.h/.cpp) — sets Team on each PS. AMinerUnit (Siegebound/MinerUnit.h/.cpp) —
    economy resolution via GetPlayerStateForTeam. Enum ETeamId (TeamId.h).

### TASK-044 — Team-driven visuals: MI_TeamColor by Team at BeginPlay (C++)
- assignee: gameplay-programmer
- status: done (committed 2f6a8fc via TASK-051, not pushed; qa-passed — qa/TASK-044-report.md — 0 blockers, 1 warn, 2 nits). All 3 actor types color correctly (units/miners via base BeginPlay, buildings own); NO spawn-timing hole (Team set before FinishSpawning + idempotent InitUnit/InitBuilding re-apply — bot Red unit can't render blue); Blue-side byte no-op; TASK-043 WARN closure verified sound (never suppresses a legit resolution in designed flows). C4458 clean. handoffs/TASK-044.md. Ready for TASK-051 batch compile.
- blocked-by: TASK-042 (serialize — shares SummonedUnit files)
- parallel-safe: no
- spec: >
    Files only. CONVENTIONS Team contract (bot units must read Red). In ASummonedUnit, ABuilding, and
    AMinerUnit, at BeginPlay apply the MI_TeamColor matching Team to VisualMesh slot 0: Blue →
    /Game/Materials/Instances/MI_TeamColor_Blue, Red → MI_TeamColor_Red (soft refs, cached, null-safe). The
    BP-authored Blue material stays the design-time default; this overrides by actual Team. Do NOT disturb the
    -90 yaw VisualMesh convention or the M1 melee/lunge / M2 ranged paths. AGoldNode is exempt (keeps
    M_GoldGlow). Acceptance: an actor spawned Team=Red shows the red material; Team=Blue shows blue; zero
    change to any M1/M2 Blue-side visual.
- names: >
    ASummonedUnit, ABuilding, AMinerUnit (Siegebound/) — team-material apply in BeginPlay;
    /Game/Materials/Instances/MI_TeamColor_Blue, /Game/Materials/Instances/MI_TeamColor_Red.

### TASK-045 — ASiegeBotController: AIController brain, economy + deck ownership, spawn + Play Again reset (C++)
- assignee: gameplay-programmer
- status: done (committed 2f6a8fc via TASK-051, not pushed; qa-passed — qa/TASK-045-report.md — 0 blockers, 0 warns, 2 nits; all 4 flagged decisions ACCEPTED; M2 player Blue economy + match flow NON-REGRESSED confirmed — independent PS, consistent exclude[PlayerController-iter]/include[PlayerArray] asymmetry, no first-PS assumption). FORWARD-DEP: once TASK-046 fills EvaluateDecisions, bot must stop playing under Victory screen — 046 gates on match-active AND 047 wires StopDecisionTimer into the freeze. handoffs/TASK-045.md. Ready for TASK-051 batch compile.
- blocked-by: TASK-043
- parallel-safe: yes (new class pair; TASK-046 serializes AFTER it — same bot file)
- spec: >
    Files only (new class pair) — the bot SHELL, no decision rules yet (those are TASK-046). GDD §4/§9-3.
    ASiegeBotController : AAIController, bWantsPlayerState=true, possesses no pawn, Team=Red. Match-start path
    (spawned by ASiegeGameMode): its auto-created ASiegePlayerState is tagged Red (TASK-043) and its economy
    starts identically to the player (accrual, overtime via GameState, miner income); create a UDeckComponent
    default subobject named DeckComponent and BuildAndShuffle at match start. Provide the reset entry point the
    GameMode calls on Play Again (ResetDeck + ResetEconomy + clear the decision timer). Stub EvaluateDecisions()
    (empty — filled in TASK-046) on a repeating timer (UPROPERTY DecisionIntervalSeconds=2.f // GDD §4).
    ASiegeGameMode spawns exactly ONE bot at match start and resets it on Play Again; the bot is a no-op until
    TASK-046. Acceptance: on BeginPlay the bot exists with a Red ASiegePlayerState whose gold accrues +2/s
    (+4/s in overtime), a 50-card deck + hand of 6, and a 2 s timer ticking EvaluateDecisions (currently
    no-op); Play Again resets the bot's gold/deck/timer; no pawn possessed; the player's economy untouched.
- names: >
    ASiegeBotController in Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h/.cpp — component
    DeckComponent, function EvaluateDecisions, UPROPERTY DecisionIntervalSeconds, reset entry ResetBot.
    Spawned/owned by ASiegeGameMode (Siegebound/SiegeGameMode.h/.cpp). Uses UDeckComponent [TASK-022], Red
    ASiegePlayerState economy [TASK-024/043].

### TASK-046 — Bot decision loop: 2 s ordered rules + placement + LogSiegeBot decision trace (C++)
- assignee: gameplay-programmer
- status: done (committed 2f6a8fc via TASK-051, not pushed; qa-passed — qa/TASK-046-report.md — 0 blockers, 2 warns, 3 nits; all 5 flagged decisions ACCEPTED). Both hard invariants CONFIRMED: never-unaffordable (Cost<=Gold before every SpendGold, destroy-on-fail) + never-Blue-half (X>=0 on navmesh-snapped point, all paths). Placement fidelity vs TASK-030 identical (plinth 420, clearance 200, castle-not-building); SiegePlayerController NOT edited. WARN-1 NavProjectionExtent (200,200,1000) vs player (50,50,50) — justified (no cursor trace), re-validated. WARN-2 rule-4 discard-fee pre-check — unreachable in M3, harden before M4/M5. C4458 clean, C4244-safe. handoffs/TASK-046.md.
- blocked-by: TASK-044, TASK-045
- parallel-safe: yes (bot-internal; parallel with TASK-047 — different files)
- spec: >
    Files only. Implement GDD §4 Bot Opponent inside ASiegeBotController::EvaluateDecisions — every 2 s, play
    the FIRST rule that fires: (1) if enemy (Blue) units are on the bot's half (X≥0) AND gold ≥ the cheapest
    affordable defensive play → play a unit at the bot centerline OR a tower between the nearest intruder and
    Castle_Red; (2) else if alive miners < 3 AND no enemy units on the bot half AND gold ≥ 8 → play Miner (to
    GoldNode_Red); (3) else if gold ≥ 12 → play the most-expensive affordable UNIT card in hand at the bot
    centerline; (4) else if the hand holds an unplayable card AND gold ≥ 1 → discard the most-expensive card.
    Plays route through the deck (ConfirmPlayFromHand / DiscardFromHand) + SpendGold on the bot PS, spawn the
    composed BP by CardID (Team=Red, team-material via TASK-044) on a navmesh-projected, own-half, clearance-
    valid point (reuse the placement validity rules; a refused point retries next tick). NEVER plays a card it
    can't afford; NEVER spawns on the Blue half. Decision trace: exactly one LogSiegeBot line per fired rule
    (rule # + card + location). Bot has no hero → Hero-Upgrade/Spell/Instant cards are treated as discards
    (forward-compat for M4/M5). Acceptance (§4): player idle → bot reaches 3 miners then attacks in growing
    waves; player pushes onto the bot half → a defensive play within 2 s; the bot never plays an unaffordable
    card and never spawns on the Blue half; the log shows which rule fired for every play/discard.
- names: >
    ASiegeBotController (Siegebound/SiegeBotController.h/.cpp) — EvaluateDecisions body; log category
    LogSiegeBot (CONVENTIONS Logging). Spawns /Game/Blueprints/Units/BP_Unit_<CardID>,
    /Game/Blueprints/Buildings/BP_Building_<CardID> (Team=Red). Targets GoldNode_Red, Castle_Red. Uses
    UDeckComponent API [TASK-022], CanAddMiner/economy [TASK-024/043], placement validity rules [TASK-030].

### TASK-047 — Match resolution v3: win/lose by team + main-menu level-flow hook (C++)
- assignee: gameplay-programmer
- status: done (committed 2f6a8fc via TASK-051, not pushed; qa-passed — qa/TASK-047-report.md — 0 blockers, 0 warns, 1 nit [header doc-comment omits new step 6, cosmetic]). Win/lose byte mapping CONFIRMED correct (Red castle→Victory, Blue→Defeat; SetWinner byte Blue=0/Red=1, ETeamId enum:uint8; M1 win-display path verified non-regressed). Bot-freeze WORKS (StopDecisionTimer in FreezeWorldAtMatchEnd step6, IsValid-guarded, idempotent). PlayAgain both-sides reset, no Blue-side regression. StartMatch static+WorldContext valid UE5.8. C4458 clean. handoffs/TASK-047.md. Ready for TASK-051 batch compile.
- blocked-by: TASK-045 (serialize GameMode edits after the bot-spawn edits)
- parallel-safe: yes (parallel with TASK-046 — different files)
- spec: >
    Files only. GDD §3.9/§9-3 full win/lose. (1) ASiegeGameMode: on castle-destroyed, resolve the winner by
    the destroyed castle's team — Blue castle destroyed → local player LOSES (Defeat); Red castle destroyed →
    local player WINS (Victory). Pass the winning ETeamId to the end screen via the existing WBP_VictoryScreen
    SetWinner byte (M1 deviation); the controller shows Victory vs Defeat accordingly. (2) Freeze the bot with
    the player at match end (bot decision timer stopped, its units frozen by the existing FreezeAI sweep) and
    reset the bot on Play Again (call TASK-045's ResetBot). (3) Main-menu hook: a BlueprintCallable entry to
    start a match (OpenLevel L_Arena) for WBP_MainMenu (TASK-049). Acceptance: destroying the Red castle shows
    Victory + Play Again; the Blue castle shows Defeat + Play Again; Play Again fully resets BOTH sides (player
    + bot: gold/deck/hand/miners/buildings/clock/castles/hero); a match cannot end any other way; the menu
    start-match entry opens L_Arena.
- names: >
    ASiegeGameMode (Siegebound/SiegeGameMode.h/.cpp), ASiegeGameState (Siegebound/SiegeGameState.h/.cpp),
    ASiegePlayerController (Siegebound/SiegePlayerController.h/.cpp — end-screen winner display). Uses
    WBP_VictoryScreen SetWinner (byte); ResetBot [TASK-045]. Start-match entry consumed by WBP_MainMenu
    [TASK-049]. Opens /Game/Maps/L_Arena.

### TASK-048 — IA_Rally input asset + Rally wiring on BP_HeroCharacter (editor)
- assignee: gameplay-programmer
- status: done (committed 56247c9 via TASK-052, not pushed; editor PID 30092, c8a40b2). IA_Rally asset created (dup IA_Card1, bool/Digital); Q→IA_Rally appended to IMC_Hero (17 M1/M2 mappings preserved byte-for-byte, IMC_Default untouched); IA_Rally assigned to BP_HeroCharacter.RallyAction (CDO readback confirmed, survived BP compile). PIE-boot verify: c8a40b2 null-RallyAction Warning does NOT fire (2 proofs: +CDO readback, -boot log) → wiring complete. Interactive Q-press deferred to TASK-052/Jonathan (no MCP keypress verb). Assets auto-staged, left for TASK-052 commit. handoffs/TASK-048.md.
- blocked-by: TASK-042; TASK-051
- parallel-safe: no
- spec: >
    Editor/MCP work. Create /Game/Input/Actions/IA_Rally (bool/Digital, key Q; duplicate an existing IA_Card*
    for the pattern). Add it to /Game/Input/IMC_Hero (do NOT touch IMC_Default). Wire the AHeroCharacter
    RallyAction UPROPERTY at the same binding site IA_Sprint/IA_Card1 use (record the pattern in the handoff).
    All existing M1/M2 bindings unchanged. Acceptance (PIE): pressing Q triggers Rally (friendly units within
    600 speed up 25% for 5 s; 20 s cooldown); WASD / mouse / Shift / LMB / 1–6 / Alt all still work.
- names: >
    /Game/Input/Actions/IA_Rally; /Game/Input/IMC_Hero; wiring on /Game/Blueprints/BP_HeroCharacter
    (RallyAction slot per handoffs/TASK-009.md pattern).

### TASK-049 — Main menu: WBP_MainMenu + L_MainMenu + Play-vs-Bot flow (editor)
- assignee: gameplay-programmer
- status: done (committed 56247c9 via TASK-052, not pushed; editor PID 30092). L_MainMenu (dup L_Arena, gameplay actors stripped, WorldSettings GameMode=BP_MenuGameMode), BP_MenuGameMode (cursor + CreateWidget WBP_MainMenu + UIOnly), WBP_MainMenu (dup UI_TouchSimple; Play→ASiegeGameMode::StartMatch, Deck Builder greyed/M6, Quit→QuitGame). DefaultEngine.ini GameDefaultMap=L_MainMenu (EditorStartupMap kept L_Arena). PIE: Game class=BP_MenuGameMode_C confirmed, menu Construct clean. MCP SURVIVED ~70 calls (path-tracing fix held). Click-through deferred to TASK-052/Jonathan (no MCP click verb). Auto-staged for TASK-052 commit. handoffs/TASK-049.md.
- blocked-by: TASK-047; TASK-051
- parallel-safe: no
- spec: >
    Editor/MCP work. GDD §7 main menu. (1) Create /Game/Maps/L_MainMenu (minimal: camera + skybox; a menu
    GameMode with a mouse cursor). (2) Create /Game/UI/WBP_MainMenu (duplicate a donor per the UMG donor rule
    — pure BP nodes, no C++ base needed) with: Play (vs Bot) → start a match (TASK-047 entry, or OpenLevel
    L_Arena); Deck Builder button present but disabled/greyed (M6); Quit → Quit Game. (3) Set L_MainMenu as
    the game's default map (Config/DefaultEngine.ini GameDefaultMap; leave EditorStartupMap so devs still open
    L_Arena directly). Acceptance (PIE from L_MainMenu): Play opens L_Arena into a live match vs the bot; Quit
    exits; Deck Builder is visibly disabled; L_Arena still opens directly for dev testing.
- names: >
    /Game/Maps/L_MainMenu; /Game/UI/WBP_MainMenu; Config/DefaultEngine.ini (GameDefaultMap). Start-match
    entry from ASiegeGameMode [TASK-047]. Opens /Game/Maps/L_Arena.

### TASK-050 — HUD/Victory v3: Rally cooldown indicator + Victory/Defeat display (editor)
- assignee: gameplay-programmer
- status: done (committed 56247c9 via TASK-052, not pushed; editor PID 30092). WBP_HUD: additive Rally indicator (RallyText TextBlock + SetupRallyIndicator/UpdateRallyDisplay fns, seed "Rally: Ready" then bind FOnRallyStateChanged via Tick do-once; M1 gold Construct byte-INTACT). WBP_VictoryScreen: ALREADY correct (SetWinner 0→Victory/1→Defeat, Play Again intact) — NO edit made; **in-memory dirty from read-only inspection only — build-master must NOT save it (disk correct)**. Live PIE: full match ran to Castle_0(Blue) destroyed→winner Red→Defeat path fired clean. MCP survived ~50 calls. Seed caveat: no BP getter for live cooldown, seed=Ready is correct at HUD-create. handoffs/TASK-050.md.
- blocked-by: TASK-042; TASK-047; TASK-051
- parallel-safe: no
- spec: >
    Editor/MCP work, ADDITIVE to WBP_HUD / WBP_VictoryScreen (guard the M1 gold Construct + M2 additions per
    TASK-033's lesson — additive only; do NOT round-trip the protected Construct). (1) WBP_HUD: add a Rally
    cooldown indicator seeded from the hero and bound to FOnRallyStateChanged (ready vs cooling-down +
    remaining seconds). (2) WBP_VictoryScreen: confirm the byte winner drives Victory vs Defeat text (Blue win
    = Victory for the local player, Red win = Defeat); wire the Defeat state if missing. Acceptance (PIE):
    using Rally greys/animates the indicator for 20 s then restores; destroying the Red castle shows Victory,
    the Blue castle shows Defeat; Play Again clears both. NOTE: shares WBP_HUD with the deferred TASK-041
    visual-hand pass — do TASK-041 first, or fold the Rally indicator into it, so the two UMG passes don't
    collide.
- names: >
    /Game/UI/WBP_HUD (additive), /Game/UI/WBP_VictoryScreen. Delegate FOnRallyStateChanged [TASK-042];
    SetWinner byte [TASK-047].

### TASK-051 — M3 code batch: compile + residue adjudication + commit (build)
- assignee: build-master
- status: done (commit 2f6a8fc on main, parent f903cf0, NOT pushed; 29 files +2685/-46). Clean compile+link FIRST TRY (~17s, no shadow errors, ZERO build-fix loops — C4458 discipline held). No Content residue; donors untouched; m2-testable still @ f903cf0. Editor left DOWN on the 2f6a8fc DLL (TASK-048 boots it). TASK-042..047 code committed here (→ done; manager wraps to ## Done at M3 finish). handoffs/TASK-051.md.
- blocked-by: TASK-042, TASK-043, TASK-044, TASK-045, TASK-046, TASK-047 (all qa-passed)
- parallel-safe: no
- spec: >
    Build-master. Compile the accumulated M3 C++ batch (TASK-042..047) with the standard Build.bat command;
    editor-bounce protocol (editor must release the DLL). Any compile error → append to the failing task's QA
    report, set qa-failed, stop (counts as a QA loop; build-master never edits code). **Pre-compile: scan the
    batch for inherited-reflected-member shadows (CONVENTIONS coding law — cost 2 loops in M2).** Adjudicate
    any working-tree residue. Commit the M3 code batch + pipeline docs (TASKBOARD/CONVENTIONS/SLACK deltas)
    with task IDs. Acceptance: clean build; git status clean of unexplained residue; commit hash on the board
    + posted in 🔧 Build & Git. Do NOT push.
- names: >
    Build command per CLAUDE.md. Commit pattern: "TASK-042..047: M3 bot opponent + hero Rally + multi-team
    economy + win/lose (C++ batch)".

### TASK-052 — M3 final assembly: full-match-vs-bot PIE verification + commit (build)
- assignee: build-master
- status: done (commit 56247c9 on main, parent c8a40b2, NOT pushed; 13 files). M3 §9-3 slice VERIFIED: 16 criteria, 0 genuine FAILs. LIVE PASSes — bot self-drives (gold accrual, miner cap 3, growing waves, Rule-1 defend within 2s, never-unaffordable, never-Blue-half, LogSiegeBot trace, match-end freeze both sides, live Blue-castle→Defeat). DEFERRED (interactive, no MCP inject → Jonathan): Q-Rally greying, menu Play/PlayAgain clicks, live Victory drive. WBP_VictoryScreen left unsaved (disk correct). WATCH: transient Blue unit at match start in repeated-PIE session (persistent level clean; cold-boot reconfirm at playtest). Editor UP PID 30092. handoffs/TASK-052.md.
- blocked-by: TASK-048, TASK-049, TASK-050 (done); TASK-051 (committed)
- parallel-safe: no
- spec: >
    Build-master. Final M3 integration in editor + Git. (1) Verify all M3 assets resolve with zero load
    warnings (IA_Rally, WBP_MainMenu, L_MainMenu, HUD/Victory edits, bot spawn). (2) PIE the §9-3 slice: main
    menu → Play vs Bot → L_Arena; the bot accrues gold, reaches 3 miners while the player is idle, attacks in
    growing waves, defends within 2 s when pushed, never plays unaffordable cards; the LogSiegeBot trace shows
    the fired rule per play; Rally (Q) buffs friendly units 25%/5 s on a 20 s cooldown; destroying the Red
    castle = Victory, the Blue castle = Defeat; Play Again resets BOTH sides. Record pass/fail per line; fails
    route back per the QA loop. (3) Confirm the slice is RECORDABLE (full match-vs-AI clip + AI decision-trace
    log). (4) Commit all M3 editor assets with task IDs; hash on the board; post in 🔧 Build & Git. Do NOT
    push. Flag the M3 checkpoint ready for Jonathan's playtest.
- names: >
    /Game/Maps/L_MainMenu, /Game/Maps/L_Arena; /Game/UI/WBP_MainMenu, WBP_HUD, WBP_VictoryScreen;
    /Game/Input/Actions/IA_Rally. Handoff: .claude/pipeline/handoffs/TASK-052.md.

---

