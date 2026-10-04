<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
## M2 tasks (decomposed 2026-07-03)

### M2 COMPLETE — 2026-07-04 (done-pending-playtest; read this first)
All M2 tasks integrated + committed on `main`, **NOT pushed**: C++ batch TASK-021..030 at **aafd968** (via TASK-039); editor/art TASK-031/032/034/035/036/037/038 + assembly TASK-040 at **5bb9507** (via TASK-040). TASK-033 = **done PARTIAL** — the functional data path is wired, but the VISUAL hand UI is deferred to **TASK-041**, a manual UMG pass (MCP cannot author widget trees). That deferral is the ONE known M2 gap.
- **Playable now:** hotkeys **1–6** play hand slots; hold **Left Alt** for the UI cursor (click cards / discard / placement). WASD + mouse + Shift-sprint + LMB melee as in M1.
- **Open M2 items (still part of M2 completion):** TASK-041 (visual hand UI + HUD stat texts) — see "### M2 open items" immediately below; plus a WATCH line to confirm the Victory screen shows on castle-destroy in a REAL playtest (a Simulate-mode session logged `no ASiegePlayerController to show end screen` — M1 shipped it working, so likely a Simulate artifact).
- TASK-021..040 statuses below are flipped to `done`; the full task blocks are left in place (not relocated to ## Done) to preserve the M2 audit trail.

### M2 open items (cleanup group — close these before M2 is fully signed off)

#### TASK-041 — WBP_CardHand visual hand UI + HUD stat texts (manual UMG pass)
- assignee: gameplay-programmer
- status: done (commit 5c1fcb7 on main, parent a745799, NOT pushed; 3 files +164/-4). DONE FULLY via MCP 2026-07-04 (editor UP PID 19464). M2 KNOWN GAP CLOSED — no human designer pass needed (path-tracing instability that killed TASK-033 is gone). WBP_HUD: GoldRateText/MinerCountText/OvertimeText, seed-then-bind (GetGoldRate→OnGoldRateChanged, GetAliveMinerCount→OnMinerCountChanged, IsOvertimeActive→OnOvertimeStarted; "/6"=MaxActiveMiners const). WBP_CardHand: full 6-slot hand via runtime BuildHandTree — per slot DisplayName+cost, play btn→RequestPlaySlot(i), discard "1"→RequestDiscardSlot(i), SetIsEnabled(bAffordable) grey, + preview + ~2s refusal; all 3 BIEs rendered (OnHandSlotUpdated/OnNextCardUpdated/OnCardRefusedMessage), empty CardID hides face, nothing typed in UMG. WARN-4: root SelfHitTestInvisible + interactive children Visible. Footman btn retired (collapsed — Btn_Jump anchors gold Construct nav, can't hard-delete). PRESERVED (readback+PIE): M1 gold Construct byte-intact, M3 Rally + M4 upgrade row intact, InitForController path. MCP survived ~140 calls; clean PIE boot 0 errors. Interactive click/grey/discard → Jonathan's playtest. build-master commits WBP_HUD+WBP_CardHand. handoffs/TASK-041.md.
- blocked-by: none (all C++ symbols shipped at aafd968; the data path is live — WBP_CardHand is reparented to UCardHandWidget and InitForController fires at runtime)
- parallel-safe: no (edits WBP_CardHand + WBP_HUD; coordinate with M3 TASK-050 which also edits WBP_HUD — do this one first, or fold the Rally indicator into it)
- spec: >
    Manual UMG designer pass. MCP cannot author widget trees from scratch, so this is a hands-on session
    (possibly Jonathan's own). Complete the deferred TASK-033 visual work; the C++ BIE contract + exact
    recipes + node IDs are all in handoffs/TASK-033.md. (1) WBP_CardHand: author the 6-slot hand tree
    (DisplayName + cost per slot; greyed/disabled when bAffordable false, §3.5); each slot = a play button
    (OnClicked → RequestPlaySlot(index)) + a small discard button labeled "1" (OnClicked →
    RequestDiscardSlot(index), §3.6/§7); a next-card preview slot; a refusal-message text shown ~2 s (§3.0).
    Render the three C++ BIEs (OnHandSlotUpdated / OnNextCardUpdated / OnCardRefusedMessage) — empty CardID/
    DisplayName ⇒ hide that face; never type costs/names into UMG (all arrive from DT_Cards via the BIEs).
    Flip the WBP_CardHand root to SelfHitTestInvisible with only the interactive children Visible (final
    WARN-4 posture: cards clickable under Alt-cursor + in placement mode, gameplay LMB not swallowed).
    (2) WBP_HUD: add gold-rate "+N/s" (seed GetGoldRate, bind OnGoldRateChanged), miner "x/6" (seed
    GetAliveMinerCount, bind OnMinerCountChanged; the "6" = MaxActiveMiners), overtime indicator (hidden
    until OnOvertimeStarted, §3.2) — each SEEDED from a getter first, THEN bound (seed-then-bind law).
    (3) Remove the M1 single-Footman card button in the designer, at the same time the visual hand ships
    (no UI gap). Do NOT round-trip the protected M1 gold Construct (TASK-033 note — it is lossy for
    GetDataTableRow / delegate-bind nodes and can silently regress the shipped gold counter). Acceptance
    (PIE): 6 slots + preview live-update on play/discard/reshuffle; a card greys the instant gold drops
    below its cost; discard deducts 1 and redraws; refusal messages appear and fade; gold-rate/miner/
    overtime texts track their delegates; the M1 gold counter is unchanged.
- names: >
    /Game/UI/WBP_CardHand (parent UCardHandWidget), /Game/UI/WBP_HUD (additive). Calls: RequestPlaySlot,
    RequestDiscardSlot. BIEs: OnHandSlotUpdated, OnNextCardUpdated, OnCardRefusedMessage. Delegates:
    FOnGoldRateChanged, FOnMinerCountChanged, FOnOvertimeStarted, FOnGoldChanged. Recipes + node IDs:
    handoffs/TASK-033.md.

#### WATCH — Victory-screen playtest confirm (not a task; close at Jonathan's M2 playtest)
- Confirm the Victory/Defeat screen appears on castle-destroy during a REAL (non-Simulate) playtest. TASK-040
  logged `no ASiegePlayerController to show end screen` in a Simulate-mode session; M1 shipped this working,
  so it is most likely a Simulate artifact. If it fails in real PIE, it becomes a gameplay-programmer task in
  the M2 cleanup group. (Also benign: a `LogCrowdFollowing … UCrowdManager` line at PIE teardown — ignore.)

### M2 RESUME — 2026-07-03: Blender MCP + UE5 MCP both confirmed UP by Jonathan (`/mcp`); Blender verified live. M2 fully unblocked. Dispatch frontier: TASK-025 + TASK-030 (code, fresh redispatch — no handoffs on disk) and TASK-037 (art, Blender). TASK-039 waits on 025+030 qa-passed.

### PLANNED SESSION SHUTDOWN — 2026-07-03 night (read this first on resume)
Jonathan deliberately closed all Claude clients to fix a blender-mcp bridge-process swarm (Claude Desktop's blender extension + stale /mcp bridges + an orphaned pre-warm tree were racing for the addon's single-client socket on 9876). **RESOLVED 2026-07-03 (next session):** the swarm was a symptom, not the root cause. The real fault was the wrong MCP client — `.mcp.json` pointed `blender` at the third-party `uvx blender-mcp`, whose wire protocol is incompatible with the **official Blender 5.1 Lab MCP addon** that actually owns port 9876. Every request came back as unparseable bytes and hung ~40s, past Claude Code's 30s startup timeout. A second, subtler fault also had to be fixed: the addon's OWN `mcp_bridge.py` speaks only Content-Length (LSP) framing, but MCP stdio (and Claude Code) use newline-delimited JSON, so it *also* hangs Code (worked in Desktop, which uses Content-Length). Final fix = a repo dual-framing bridge `Tools/blender_mcp_bridge.py` (auto-detects framing) that `.mcp.json` now launches; verified with live newline + Content-Length round-trips. Full writeup: Obsidian note "Blender MCP — official addon vs uvx client". State at shutdown:
- **qa-passed (8/10 file tasks): 021, 022, 023, 024, 026, 027, 028, 029.** All QA reports in qa/, all handoffs in handoffs/.
- **TASK-025 + TASK-030 agents were IN-FLIGHT at shutdown** and died with the session. On resume, check for handoffs/TASK-025.md and handoffs/TASK-030.md + their 🧪 posts in the Dev & QA Slack thread: if a handoff exists, that task is done → route to QA; if absent/partial, redispatch fresh with audit-first instructions (TASK-027 incident pattern — expect partial files: MinerUnit/GoldNode .h/.cpp for 025; SiegePlayerController edits for 030).
- After 025 + 030 pass QA → **TASK-039** (build-master: editor bounce, batch compile, THEN TASK-031 DT_Cards reimport IMMEDIATELY before any PIE per qa/TASK-021 WARN-2, residue cleanup incl. the three lock-blocked files, commit) → editor wave 032..036 → TASK-040.
- Uncommitted-but-QA-passed code on disk: all M2 file-task changes since commit 4f95730 (Source/Siegebound: CardRow, DeckComponent, SiegePlayerController, SiegeGameState [new], SiegePlayerState, SiegeGameMode, HeroCharacter, Building/Tower [new], Projectile/DamageTypes [new], SummonedUnit, CardHandWidget [new], + Docs/Data/cards.csv, Config/DefaultEngine.ini navmesh line). NOTHING commits until TASK-039's compile passes.
- Unreal editor was left RUNNING with MCP up (frozen at 4f95730 content state); it may be closed/rebooted freely — TASK-039 bounces it anyway.
- Blender restart order for art (037/038): start **Blender 5.1** (the official Lab MCP addon requires 5.1+; 5.0 cannot run it) + addon Connect FIRST, then verify only ONE Claude client has the `blender` server connected — Desktop's blender extension and Code both hitting 9876 contend on the addon's single-threaded socket. Then **fully restart Claude Code** (it reads `.mcp.json` at startup; `/mcp` reconnect may not hot-reload) — `/mcp` should then show `blender` connected. Protocol is settled: `.mcp.json` launches the repo dual-framing bridge `Tools/blender_mcp_bridge.py` (tools: `execute_blender_code`, `get_scene_info`, `get_object_info`) — no "probe protocol" step. art-director already updated for `execute_blender_code`.

File tasks (TASK-021..030) dispatch NOW per the gates above; wave order from blocked-by: [021, 026] → [022, 027, 028] → [023, 024, 029] → [025, 030]. Editor/build tasks (031..036, 039, 040): round-2 sign-off received 2026-07-03 — unblocked per their remaining blocked-by lines (TASK-039 compile leads); Blender tasks (037, 038) still need Blender MCP.

### TASK-021 — Core-set card data: FCardRow columns + cards.csv rows (files)
- assignee: gameplay-programmer
- status: done (code committed aafd968 via TASK-039). qa-passed (qa/TASK-021-report.md — 0 blockers, 2 warns, 1 nit; all 5 flagged decisions ruled PASS)
- blocked-by: none
- parallel-safe: yes
- spec: >
    Files only, no editor, no compile. (1) In FCardRow (CardRow.h) add two UPROPERTYs (CONVENTIONS
    column registry): int32 DeckCount = 0 (copies in the default 50-card deck, GDD §3.4) and bool
    bRanged = false (attack delivered by homing projectile, GDD §3.0). Property names must match the
    CSV headers exactly for reimport. (2) Extend Docs/Data/cards.csv: add columns DeckCount + bRanged
    to the header and ALL rows; update Footman (DeckCount 12, bRanged false); add five rows per GDD §4
    Core Set exactly —
    Archer: Unit, Cost 4, MaxCopies 10, HP 45, Damage 10, Range 700, Cadence 1.2, Speed 350, Standard,
    DeckCount 10, bRanged true.
    Knight: Unit, Cost 6, MaxCopies 6, HP 200, Damage 15, Range 120, Cadence 1.2, Speed 300, Standard,
    DeckCount 6, bRanged false.
    Miner: Economy, Cost 8, MaxCopies 4, HP 30, Damage 0, Range 0, Cadence 0, Speed 350, Profile None,
    DeckCount 4, bRanged false.
    ArrowTower (DisplayName "Arrow Tower"): Building, Cost 5, MaxCopies 8, HP 150, Damage 15, Range 900,
    Cadence 1.5, Speed 0, Profile None, DeckCount 8, bRanged true.
    Wall: Building, Cost 4, MaxCopies 10, HP 300, Damage 0, Range 0, Cadence 0, Speed 0, Profile None,
    DeckCount 10, bRanged false.
    Acceptance: CSV header maps 1:1 onto FCardRow (TASK-031 reimport must produce zero warnings); exactly
    6 rows; DeckCount column sums to exactly 50 matching the §3.4 default deck; every value matches §4
    character-for-character; no stat invented outside the GDD.
- names: >
    Source/GitClaudeUnrealTest/Siegebound/CardRow.h (FCardRow — new UPROPERTYs DeckCount, bRanged);
    Docs/Data/cards.csv. Row names (CardIDs, PascalCase no spaces): Footman, Archer, Knight, Miner,
    ArrowTower, Wall. Import target (TASK-031): /Game/Data/DT_Cards.

### TASK-022 — Deck & hand model: UDeckComponent (C++)
- assignee: gameplay-programmer
- status: done (code committed aafd968 via TASK-039). qa-passed (qa/TASK-022-report.md — 0 blockers, 1 warn, 2 nits; all 14 flagged decisions ruled PASS; WARN-1 = TASK-023 discard-order gold-leak guard, carry-forward)
- blocked-by: TASK-021
- parallel-safe: yes
- spec: >
    Files only (new class pair). GDD §3.4. UDeckComponent (UActorComponent, will live on
    ASiegePlayerController via TASK-023). Owns three piles + hand, NO gold logic (controller owns
    spend/refund): BuildAndShuffle() reads every row of /Game/Data/DT_Cards (soft path, runtime load,
    log-and-empty if missing) and builds a draw pile with DeckCount copies of each CardID (log an error
    if the total != 50 but still proceed with what the table gives); shuffle; deal exactly 6 into the
    hand. API: FName GetHandCardID(int32 Slot 0..5); FName PeekNextCardID() (top of draw pile — the
    §3.4 preview, must always be the ACTUAL next draw); ConfirmPlayFromHand(int32 Slot) — moves the card
    to the discard pile and immediately draws its replacement into the same slot; DiscardFromHand(int32
    Slot) — same pile movement (the 1-gold charge is the controller's job); int32 GetDrawPileCount() /
    GetDiscardPileCount(); ResetDeck() → full rebuild+reshuffle+redeal (Play Again). When the draw pile
    empties, shuffle the discard pile into a new draw pile automatically. Delegates (CONVENTIONS
    seed-then-bind law applies to consumers): FOnDeckHandChanged() broadcast whenever any hand slot
    changes, FOnDeckNextCardChanged(FName NextCardID) whenever the preview changes. Acceptance (§3.4):
    hand always has exactly 6; ConfirmPlayFromHand draws a replacement immediately; the preview always
    equals the actual next draw; after 50 plays/discards the deck has reshuffled and keeps dealing
    indefinitely; the built deck contains exactly the §3.4 copy counts (12/10/10/8/6/4).
- names: >
    UDeckComponent in Source/GitClaudeUnrealTest/Siegebound/DeckComponent.h/.cpp; delegates
    FOnDeckHandChanged (member OnDeckHandChanged), FOnDeckNextCardChanged (member OnDeckNextCardChanged);
    functions BuildAndShuffle, GetHandCardID, PeekNextCardID, ConfirmPlayFromHand, DiscardFromHand,
    GetDrawPileCount, GetDiscardPileCount, ResetDeck. Data (exact): /Game/Data/DT_Cards.

### TASK-023 — PlayerController v2: hand play, discard, refusal messages, input plumbing (C++)
- assignee: gameplay-programmer
- status: done (code committed aafd968 via TASK-039). qa-passed (qa/TASK-023-report.md — 0 blockers, 1 warn, 1 nit; all 18 flagged decisions ruled PASS; all three scrutiny walks clean; qa/TASK-022 WARN-1 gold-leak closure verified; M1 seven-exit-path law intact; FOnCardRefused seam to TASK-029 character-exact; WARN = defensive ExitPlacementMode in HandleMatchReset, carry to TASK-030)
- blocked-by: TASK-022
- parallel-safe: yes
- spec: >
    Files only. GDD §3.5/§3.6 + M2 input ruling. On ASiegePlayerController: (1) create DeckComponent as a
    default subobject named "DeckComponent"; call BuildAndShuffle at match start and ResetDeck from the
    PlayAgain flow. (2) PlayHandSlot(int32 Slot) (BlueprintCallable): look up the slot's CardID row; refuse
    if gold < Cost (grey-out is the widget's job, §3.5) with a reason message; for Unit/Economy/Building
    cards enter placement mode for that CardID/slot (card leaves the hand only at CONFIRM — M2 ruling;
    placement internals + confirm/spend/spawn are TASK-030's; until TASK-030 lands, route into the existing
    M1 EnterPlacementMode(CardID) path and pass the slot through so confirm calls
    DeckComponent->ConfirmPlayFromHand(Slot)); Instant/Spell types: log + refuse (M4/M5). (3)
    DiscardHandSlot(int32 Slot) (BlueprintCallable): SpendGold(1) — refused at 0 gold (§3.6) — then
    DeckComponent->DiscardFromHand(Slot). (4) Refusal surface: dynamic delegate FOnCardRefused(const
    FString& Reason), UPROPERTY(BlueprintAssignable) OnCardRefused, broadcast on EVERY refused
    play/discard with the §3.0-style reason ("Not enough gold", "Miner limit reached" [used by TASK-030],
    etc.). (5) Input plumbing (assets wired in TASK-032, all refs null-safe): UPROPERTY UInputAction slots
    Card2Action..Card6Action + UICursorAction following the exact binding-site pattern IA_Card1 uses today
    (record its property name in the handoff if it differs); keys 1..6 → PlayHandSlot(0..5). IA_UICursor
    HELD = GameAndUI input mode + bShowMouseCursor true + camera look suspended; released = restore
    GameOnly free-look (M1 feel). Placement mode's own cursor behavior unchanged. Preserve the melee
    suppression contract: SetMeleeSuppressed(false) on EVERY mode-exit path (qa/TASK-003-report.md
    warning 2). Acceptance: with 6-card hand, keys 1–6 attempt the right slots; discard at >=1 gold
    removes the card, deducts exactly 1, draws a replacement; discard at 0 gold refused + OnCardRefused
    fires; unaffordable play refused with no state change; holding Alt shows the cursor and suspends
    look, releasing restores both; nothing hardcoded that lives in DT_Cards.
- names: >
    ASiegePlayerController (Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h/.cpp) —
    component DeckComponent; functions PlayHandSlot, DiscardHandSlot; delegate FOnCardRefused (member
    OnCardRefused); UPROPERTYs Card2Action, Card3Action, Card4Action, Card5Action, Card6Action,
    UICursorAction. Input assets (exact, created in TASK-032): /Game/Input/Actions/IA_Card2..IA_Card6,
    /Game/Input/Actions/IA_UICursor.

### TASK-024 — Match clock, overtime, economy v2, match-end freeze (C++)
- assignee: gameplay-programmer
- status: done (code committed aafd968 via TASK-039). qa-passed (qa/TASK-024-report.md — 0 blockers, 1 warn, 2 nits; 13/13 flagged decisions ruled PASS/ACCEPTED; all 3 carry-forward closures VERIFIED CLOSED; qa/TASK-005 major-1 income-timer law intact; WARN-1 = PlayAgain double ResetDeck [TASK-023 seam], verified benign — drop one call in a later pass)
- blocked-by: TASK-027, TASK-028
- parallel-safe: yes
- spec: >
    Files only. GDD §3.2 + M1 carry-overs (match-end freeze, KillZ respawn). (1) NEW ASiegeGameState:
    float MatchClockSeconds ticking from 0 at match start; UPROPERTY OvertimeStartSeconds = 420.f
    (// GDD §3.2, 7:00); at threshold set bOvertimeActive and broadcast FOnOvertimeStarted() exactly
    once; FOnMatchClockChanged(int32 WholeSeconds) each elapsed second; StopClock() / ResetClock().
    ASiegeGameMode sets GameStateClass = ASiegeGameState. (2) ASiegePlayerState v2: income becomes
    rate-composed — BaseRate (2, or 4 when GameState overtime is active, §3.2) + 1 per active miner
    income (§3.3). API: AddMinerIncome()/RemoveMinerIncome(); RegisterMinerAlive()/UnregisterMinerAlive();
    int32 GetAliveMinerCount(); bool CanAddMiner() (cap UPROPERTY MaxActiveMiners = 6, // GDD §3.3 —
    ALIVE miners count, M2 ruling); int32 GetGoldRate(); delegates FOnGoldRateChanged(int32 NewRate) and
    FOnMinerCountChanged(int32 AliveCount) on every actual change; PauseIncome()/ResumeIncome();
    ResetEconomy() → rate/miner counts to base (Play Again). Respect qa/TASK-005-report.md major 1:
    clear/restart timers in a safe order — PlayAgain clears timers BEFORE ResetGold. (3) Match-end freeze
    (M1 carry-over, qa/TASK-006-report.md): when ASiegeGameMode ends the match it must — iterate all
    ASummonedUnit and call FreezeAI() (contract: implemented by TASK-028), PauseIncome() on the player
    state, StopClock() on the game state. (4) PlayAgain v2: additionally destroy all ABuilding actors
    (contract: class from TASK-027), ResetEconomy, ResetClock (+ overtime flag cleared), ResumeIncome,
    and trigger the controller's deck reset (call a controller-side reset entry point; TASK-023 provides
    ResetDeck via DeckComponent). (5) KillZ carry-over: override AHeroCharacter::FellOutOfWorld to route
    through the EXISTING death→5 s respawn path instead of a bare Destroy (the hero must survive falling
    off the world; TASK-036 sets the KillZ + boundary volumes). Acceptance (§3.2): at 7:00 rate goes
    ~2/s → ~4/s (+1 per arrived miner unchanged) and FOnOvertimeStarted fires once; GetGoldRate always
    matches observed accrual; match end freezes income + units + clock; Play Again restores clock 0,
    base rate 2, zero miners, no buildings, fresh deck; a hero falling past KillZ respawns at its castle
    within 5–6 s.
- names: >
    ASiegeGameState in Source/GitClaudeUnrealTest/Siegebound/SiegeGameState.h/.cpp — UPROPERTY
    OvertimeStartSeconds, delegates FOnOvertimeStarted (member OnOvertimeStarted), FOnMatchClockChanged
    (member OnMatchClockChanged), functions StopClock, ResetClock. ASiegePlayerState
    (Siegebound/SiegePlayerState.h/.cpp) — functions AddMinerIncome, RemoveMinerIncome,
    RegisterMinerAlive, UnregisterMinerAlive, GetAliveMinerCount, CanAddMiner, GetGoldRate, PauseIncome,
    ResumeIncome, ResetEconomy; UPROPERTY MaxActiveMiners; delegates FOnGoldRateChanged (member
    OnGoldRateChanged), FOnMinerCountChanged (member OnMinerCountChanged). ASiegeGameMode
    (Siegebound/SiegeGameMode.h/.cpp). AHeroCharacter (Siegebound/HeroCharacter.h/.cpp) —
    FellOutOfWorld override. Cross-task contracts (exact symbols): ASummonedUnit::FreezeAI() [TASK-028],
    class ABuilding [TASK-027].

### TASK-025 — Miner unit + gold node (C++)
- assignee: gameplay-programmer
- status: done (code committed aafd968 via TASK-039). qa-passed (build-fix loop 1 folded 2026-07-04: MinerUnit.cpp C4458 shadows cleared — local Owner→OwnerState ×3, loop PlayerState→IterPlayerState; pure local renames, no seams/stats. Batch-wide compiler-level shadow sweep confirms module shadow-clean. handoffs/TASK-025.md). Prior logic QA (0 blk/1 warn/1 nit, no-attack seal verified load-bearing) stands.
- blocked-by: TASK-021, TASK-024
- parallel-safe: yes
- spec: >
    Files only (two new class pairs). GDD §3.3 + §5. (1) AGoldNode (AActor): EditAnywhere Team (ETeamId);
    UStaticMeshComponent root with soft mesh /Game/Meshes/SM_GoldNode (null-safe — asset arrives in
    TASK-038); not damageable, no ITeamAgent combat participation; blocks nothing (no pawn-blocking
    collision). (2) AMinerUnit : ASummonedUnit — CardID Miner, stats (30 HP, 350 speed) from DT_Cards as
    usual. Overrides the combat state machine entirely: never acquires or attacks anything (§3.3
    non-combat); on BeginPlay RegisterMinerAlive() on the owning team's ASiegePlayerState, then MoveToActor
    toward the SAME-team AGoldNode (actor iteration; if none, log and idle); on arriving within
    UPROPERTY ArrivalRadius = 150.f (// GDD §3.3 ~10 s walk) call AddMinerIncome() exactly once and stand
    at the node (idle mining; "clink" audio is M7). On death: UnregisterMinerAlive() always,
    RemoveMinerIncome() only if it had arrived. Miner remains attackable by enemies (ITeamAgent via base;
    §3.3 raidable-investment rule). FreezeAI (from TASK-028 base) must also stop its walk. Acceptance
    (§3.3): a Blue miner spawned mid-half walks to GoldNode_Blue and the owner's rate rises +2/s → +3/s
    only on arrival (~10 s); killing it drops the rate back and decrements the miner count; a miner killed
    en route changes the rate not at all; miner count delegate tracks alive miners.
- names: >
    AGoldNode in Source/GitClaudeUnrealTest/Siegebound/GoldNode.h/.cpp; AMinerUnit in
    Source/GitClaudeUnrealTest/Siegebound/MinerUnit.h/.cpp — UPROPERTY ArrivalRadius. Mesh (exact, from
    TASK-038): /Game/Meshes/SM_GoldNode. Level instances (placed in TASK-036): GoldNode_Blue (-1200,0),
    GoldNode_Red (+1200,0). PlayerState API (exact, from TASK-024): RegisterMinerAlive,
    UnregisterMinerAlive, AddMinerIncome, RemoveMinerIncome. BP child (TASK-034):
    /Game/Blueprints/Units/BP_Unit_Miner with row Miner.

### TASK-026 — Projectile actor + damage types + castle damage scaling (C++)
- assignee: gameplay-programmer
- status: done (code committed aafd968 via TASK-039). qa-passed (qa/TASK-026-report.md — 0 blockers, 1 warn, 4 nits; all 10 flagged decisions ruled PASS/ACCEPTED; M1 castle melee path verified identical incl. both TakeDamage return-value consumers; WARN carry-forward: in-flight projectiles vs match-end freeze/PlayAgain → TASK-024/040)
- blocked-by: none
- parallel-safe: yes
- spec: >
    Files only. GDD §3.0. (1) DamageTypes.h/.cpp: USiegeDamageType_Melee and USiegeDamageType_Projectile
    (UDamageType subclasses, no logic — CONVENTIONS damage-type registry; Siege/Spell reserved M4/M5).
    (2) AProjectile (AActor): InitProjectile(ETeamId Team, AActor* Target, float Damage,
    TSubclassOf<UDamageType> DamageTypeClass); homing — every tick move toward the target actor's current
    location at UPROPERTY Speed = 1500.f (// GDD §3.0); on reaching/overlapping the target apply Damage
    with the given damage type (instigator plumbed so no-friendly-fire checks keep working) and destroy
    self (§3.0 destroyed on impact); if the target dies mid-flight, continue to its last known location
    and expire there harmlessly; lifetime cap ~5 s safety. Impact feedback: spawn NS_Damage
    (TSoftObjectPtr<UNiagaraSystem> default /Game/Variant_Combat/VFX/NS_Damage.NS_Damage, READ-ONLY donor,
    cached resolve, null-safe) at the impact point — keeps the §3.8 telegraph baseline for ranged hits.
    Visual: sphere StaticMeshComponent using engine /Engine/BasicShapes/Sphere scaled ~0.15 with
    /Game/Materials/Instances/MI_TeamColor_Blue or _Red by team (all soft refs null-safe). No collision
    with friendlies. (3) ACastle::TakeDamage — replace the M1 TODO with §3.0 scaling read from
    DamageEvent.DamageTypeClass: USiegeDamageType_Projectile = 50%, Melee/default/unknown = 100%; leave
    marked TODOs for Siege 200% (M4) and Spell 50% (M5). Scaling applies ONLY in ACastle (units/hero/
    buildings take listed damage, M2 ruling). Acceptance: a projectile carrying 10 damage deals 10 to a
    unit but 5 to a castle; melee still deals 100% to the castle; friendly projectiles never damage
    friendlies; projectile flies at 1500 u/s, homes onto a moving target, and always despawns on impact
    or expiry.
- names: >
    USiegeDamageType_Melee, USiegeDamageType_Projectile in
    Source/GitClaudeUnrealTest/Siegebound/DamageTypes.h/.cpp. AProjectile in
    Source/GitClaudeUnrealTest/Siegebound/Projectile.h/.cpp — function InitProjectile, UPROPERTY Speed.
    ACastle (Siegebound/Castle.h/.cpp — TakeDamage scaling only). Donor (read-only):
    /Game/Variant_Combat/VFX/NS_Damage. Engine mesh (read-only): /Engine/BasicShapes/Sphere.

### TASK-027 — Building base + tower (C++) + dynamic navmesh config
- assignee: gameplay-programmer
- status: done (code committed aafd968 via TASK-039). qa-passed (qa/TASK-027-report.md — 0 blockers, 2 warns, 4 nits; all 13 flagged decisions ruled PASS/ACCEPTED; resume-seam audit clean; ini verified single-section/next-boot-only; WARN carry-forwards: match-end tower firing → TASK-024, InitBuilding-deferred-must-pass-real-CardID constraint → TASK-030)
- blocked-by: TASK-026
- parallel-safe: yes
- spec: >
    Files only. GDD §3.7. (1) ABuilding (AActor): EditAnywhere FName CardID + Team (ETeamId); implements
    ITeamAgent; UStaticMeshComponent named VisualMesh (mesh set by BP child, CONVENTIONS); on BeginPlay
    load its FCardRow from /Game/Data/DT_Cards (soft path, log-and-idle if missing, NEVER hardcode) →
    HP from row. Destructible: TakeDamage ignores same-team damage (§3.0), dies at 0 HP → destroy actor
    (crumble FX is M7). Stationary, no tick logic in the base. Collision: VisualMesh blocks Pawns AND
    affects navigation (bCanEverAffectNavigation true) so walls reroute unit pathing (§3.7). (2) ATower :
    ABuilding — every Cadence seconds (row: ArrowTower 1.5) acquire the NEAREST enemy ITeamAgent that is
    a unit or the hero (exclude ACastle/ABuilding — §3.7 "targets units/hero") within Range (900); fire
    an AProjectile (InitProjectile with row Damage 15, USiegeDamageType_Projectile, own team); no target
    in range = idle, re-scan next cadence; no friendly fire. (3) Config/DefaultEngine.ini: under
    [/Script/NavigationSystem.RecastNavMesh] set RuntimeGeneration=Dynamic so runtime-placed walls carve
    the navmesh (§3.7 "dynamically updates the navmesh"; file edit only — takes effect at the gated
    compile/editor boot). Acceptance (§3.7): an ArrowTower-row tower fires at the nearest enemy inside
    900 every 1.5 s for 15 (projectile-typed: 7.5 if it somehow hits a castle) and ignores anything
    beyond 900; a Wall-row ABuilding with 300 HP blocks units, forces rerouting, and dies to 300 damage;
    both refuse friendly damage; all stats from DT_Cards.
- names: >
    ABuilding in Source/GitClaudeUnrealTest/Siegebound/Building.h/.cpp — component VisualMesh; ATower in
    Source/GitClaudeUnrealTest/Siegebound/Tower.h/.cpp. Uses AProjectile + USiegeDamageType_Projectile
    [TASK-026]. Config/DefaultEngine.ini ([/Script/NavigationSystem.RecastNavMesh]
    RuntimeGeneration=Dynamic). Data rows (exact): ArrowTower, Wall. BP children (TASK-035):
    /Game/Blueprints/Buildings/BP_Building_ArrowTower (ATower), BP_Building_Wall (ABuilding).

### TASK-028 — Summoned unit v2: ranged attacks + FreezeAI (C++)
- assignee: gameplay-programmer
- status: done (code committed aafd968 via TASK-039). qa-passed (qa/TASK-028-report.md PASS — 0 blockers / 0 warnings / 2 nits; 11/11 flagged decisions ruled; heightened-scrutiny out-of-scope sweep CLEAN; closes qa/TASK-020 WARN-1; compile gated on TASK-039 batch)
- blocked-by: TASK-021, TASK-026
- parallel-safe: yes
- spec: >
    Files only. GDD §3.8/§4 Archer + M1 freeze carry-over. In ASummonedUnit: (1) read bRanged from the
    card row (TASK-021). Melee path unchanged (incl. TASK-020 lunge). Ranged path (bRanged true): in the
    Attack state, at each Cadence tick spawn an AProjectile at the unit (InitProjectile: own team, current
    target, row Damage, USiegeDamageType_Projectile — 50% vs castle happens castle-side); NO lunge for
    ranged attacks (the projectile + its impact effect are the telegraph); same state machine, aggro 600,
    leash 900, Range gate from the row (Archer 700). (2) FreezeAI(): public; stops movement, clears
    attack/lunge/acquire timers, cancels any in-flight lunge offset (restore exact base relative location),
    unit idles until destroyed — used by the match-end freeze (TASK-024 contract) and inherited by
    AMinerUnit (must also stop its walk). Acceptance: an Archer-row unit (45 HP, 350 speed) advances,
    engages the nearest enemy entering 600 from up to 700 range, fires a homing projectile every 1.2 s
    for 10 (5 vs castle), disengages beyond 900 and resumes Advance; melee units byte-identical to M1;
    FreezeAI stops any unit mid-anything with zero residual offset; nothing stat-like hardcoded.
- names: >
    ASummonedUnit (Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h/.cpp) — function FreezeAI; uses
    AProjectile + USiegeDamageType_Projectile [TASK-026]; reads FCardRow.bRanged [TASK-021]. Data rows:
    Archer (ranged), Knight/Footman (melee).

### TASK-029 — Card hand widget C++ base: UCardHandWidget (C++)
- assignee: gameplay-programmer
- status: done (code committed aafd968 via TASK-039). qa-passed (build-fixes loop 1+2 folded 2026-07-04: loop-1 cleared UHT param shadow, loop-2 cleared CardHandWidget.cpp/.h C4458 shadows — loop var Slot→SlotIndex, PushHandSlot param Slot→SlotIndex; pure local renames, RequestPlay/DiscardSlot + BIE signatures untouched. Batch shadow sweep clean. handoffs/TASK-029.md). Prior logic QA (0 blk/1 warn/3 nit, TASK-023 seam clean, WARN-1 TASK-033 idempotent-BIE) stands.
- blocked-by: TASK-022
- parallel-safe: yes
- spec: >
    Files only (new class pair). GDD §3.4/§3.5/§3.6/§7 — C++ half of the hand UI (UMG asset is TASK-033;
    MCP cannot author widget trees, so C++ pushes display-ready data through BlueprintImplementableEvents
    with float/int/bool/FString params ONLY — never enums, CONVENTIONS). UCardHandWidget : UUserWidget.
    BlueprintCallable InitForController(ASiegePlayerController*): SEED all 6 slots + preview + affordability
    immediately from current state, THEN bind (seed-then-bind law) to: OnDeckHandChanged +
    OnDeckNextCardChanged (deck), FOnGoldChanged (affordability re-grey, §3.5 "greyed when unaffordable"),
    OnCardRefused (message pass-through). Per update push BIEs: OnHandSlotUpdated(int32 SlotIndex,
    const FString& CardID, const FString& DisplayName, int32 Cost, bool bAffordable);
    OnNextCardUpdated(const FString& DisplayName, int32 Cost); OnCardRefusedMessage(const FString& Reason)
    (HUD shows it ~2 s — §3.0 refund rule surface). Costs/DisplayNames read from DT_Cards rows — never
    typed into UMG. BlueprintCallable pass-throughs for the UMG buttons: RequestPlaySlot(int32 Slot) →
    controller PlayHandSlot; RequestDiscardSlot(int32 Slot) → controller DiscardHandSlot. All null-safe.
    Acceptance: with a mocked/PIE controller the widget seeds 6 correct slots + preview before any
    delegate fires; gold dropping below a card's cost flips its bAffordable on the next gold change;
    every refused action produces exactly one OnCardRefusedMessage.
- names: >
    UCardHandWidget in Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h/.cpp — functions
    InitForController, RequestPlaySlot, RequestDiscardSlot; BIEs OnHandSlotUpdated, OnNextCardUpdated,
    OnCardRefusedMessage. UMG asset (exact, TASK-033): /Game/UI/WBP_CardHand.

### TASK-030 — Placement v2: navmesh projection, building clearance, generalized spawn, miner cap (C++)
- assignee: gameplay-programmer
- status: done (code committed aafd968 via TASK-039). qa-passed (qa/TASK-030-report.md — 0 blockers, 2 warns, 3 nits; all 3 flagged items APPROVED [SiegeGameMode ResetDeck drop clean, Build.cs +NavigationSystem correct, CastlePlinthClearance acceptable]; TASK-023 baseline intact; all 3 carry-forward WARNs closed; melee-suppression on all 10 exit paths; NavigationSystem APIs verified vs UE 5.8). handoffs/TASK-030-programmer.md. BUILD NOTE for TASK-039: Build.cs delta forces a FULL editor rebuild, not hot-reload. Ready for TASK-039 batch.
- blocked-by: TASK-023, TASK-024, TASK-027
- parallel-safe: yes
- spec: >
    Files only. GDD §3.5 full placement rules + M1 carry-overs (navmesh projection; plinth-collision
    interplay). Rework ASiegePlayerController placement (serialized after TASK-023 — same files): (1)
    Validity for ALL placements = cursor ground hit AND own half (X <= 0, Blue) AND the point projects
    onto the navmesh (ProjectPointToNavigation within a small extent) — this refuses castle-roof and
    plinth-top placements (M1 carry-over). (2) Buildings additionally require >= BuildingClearance
    (UPROPERTY, 200.f // GDD §3.5) from the nearest other ABuilding (iterate; castles are NOT buildings
    for this rule). (3) Miner cap: entering placement AND confirming for CardID Miner checks
    PlayerState CanAddMiner(); refusal broadcasts OnCardRefused("Miner limit reached") with NO gold
    movement (net-zero refund ruling, §3.3). (4) Confirm path generalized: SpendGold(row Cost) then spawn
    by CardType — Unit/Economy: /Game/Blueprints/Units/BP_Unit_<CardID>.BP_Unit_<CardID>_C; Building:
    /Game/Blueprints/Buildings/BP_Building_<CardID>.BP_Building_<CardID>_C (composed soft-class paths per
    CONVENTIONS, null-safe: missing BP = refuse + reason message, no gold spent); Team=Blue; then
    ConfirmPlayFromHand(Slot). (5) Ghost preview generalized: ghost mesh = /Game/Meshes/SM_<CardID>
    (soft path, fallback /Engine/BasicShapes/Sphere), M_Ghost GhostColor green/red validity incl.
    clearance violations shown red (§3.5/§7). (6) Melee-suppression law re-verified on every exit path
    (qa/TASK-003-report.md warning 2). Acceptance (§3.5): a building placed 100 units from another is
    refused red with no gold spent, at 250 units accepted; clicks on the enemy half or on the castle
    roof are refused; a 7th alive miner is refused with the exact message and unchanged gold; each core
    card spawns its correct BP at the clicked point and only then leaves the hand; cancel exits free;
    hero melee restored after every exit.
- names: >
    ASiegePlayerController (Siegebound/SiegePlayerController.h/.cpp) — UPROPERTY BuildingClearance;
    spawn-path patterns /Game/Blueprints/Units/BP_Unit_<CardID>, /Game/Blueprints/Buildings/
    BP_Building_<CardID>; ghost pattern /Game/Meshes/SM_<CardID> + /Game/Materials/M_Ghost (param
    GhostColor). Uses CanAddMiner [TASK-024], class ABuilding [TASK-027], ConfirmPlayFromHand [TASK-022].

### TASK-031 — DT_Cards reimport: 6-row core set (editor)
- assignee: gameplay-programmer
- status: done (integrated + committed 5bb9507 via TASK-040; 2026-07-04, editor booted on aafd968 DLL, left UP w/ MCP reachable for the rest of the wave). MCP has no reimport verb → used reference-safe in-place row rebuild from cards.csv (asset GUID + CSV linkage preserved). 6 rows verified: DeckCount sum=50, bRanged true only Archer+ArrowTower, all §4 values exact. WATCH: benign `LogDataTable: Missing RowStruct while saving` log — row data confirmed on disk; build-master re-verify 6 rows on fresh load at TASK-040. handoffs/TASK-031.md. (No commit — TASK-040 commits.)
- blocked-by: TASK-021; TASK-039 (FCardRow columns compiled)
- parallel-safe: no
- spec: >
    Editor/MCP work. Reimport Docs/Data/cards.csv into the existing /Game/Data/DT_Cards (row struct
    FCardRow now carries DeckCount + bRanged). Zero import warnings. Verify all 6 rows (Footman, Archer,
    Knight, Miner, ArrowTower, Wall) match GDD §4 exactly, DeckCount sums to 50, bRanged true only on
    Archer + ArrowTower. Keep the CSV as the import source (§3.0 — balance edits stay CSV reimports).
    Acceptance: GetRow on each of the 6 CardIDs returns exact §4 values; no legacy/extra rows; no
    warnings.
- names: >
    /Game/Data/DT_Cards (source Docs/Data/cards.csv, row struct FCardRow). Rows: Footman, Archer,
    Knight, Miner, ArrowTower, Wall.

### TASK-032 — Input assets v2: IA_Card2..6 + IA_UICursor (editor)
- assignee: gameplay-programmer
- status: done (integrated + committed 5bb9507 via TASK-040). IA_Card2..6 (keys 2-6) + IA_UICursor (LeftAlt) created in /Game/Input/Actions/ (dup'd from IA_Card1); 6 mappings appended to IMC_Hero (11 M1 mappings preserved byte-for-byte, IMC_Default untouched). Wiring = assets at the soft-path locations the aafd968 controller SetupInputComponent already resolves (no BP subclass). PIE: clean boot, deck dealt 50-card pile from DT_Cards, ZERO input-resolution warnings. Interactive key/Alt behavior verified structurally (no MCP keypress-inject verb) — TASK-040 full PIE + Jonathan close it. Shift-log ini tweak SKIPPED (deliberate). handoffs/TASK-032.md. (TASK-040 commits.)
- blocked-by: TASK-023; TASK-039 (controller slots compiled)
- parallel-safe: no
- spec: >
    Editor/MCP work. Create bool/Digital input actions in /Game/Input/Actions/: IA_Card2 (key 2), IA_Card3
    (3), IA_Card4 (4), IA_Card5 (5), IA_Card6 (6), IA_UICursor (Left Alt). Add all six to /Game/Input/
    IMC_Hero (do NOT modify IMC_Default or any template asset). Wire the TASK-023 UPROPERTY slots
    (Card2Action..Card6Action, UICursorAction) at the same binding site IA_Card1 uses today
    (BP_HeroCharacter or controller — follow the existing pattern, record in handoff). Optional cosmetic
    (M1 carry-over): add a Config/DefaultInput.ini override to silence the engine's Shift debug-binding
    log line — skip if it costs more than minutes. Acceptance (PIE, MCP-injected): keys 1–6 each attempt
    the matching hand slot; holding Left Alt shows the cursor and suspends camera look; releasing
    restores free-look; existing M1 bindings (WASD/mouse/Space/Shift/LMB/RMB/Esc) unchanged.
- names: >
    /Game/Input/Actions/IA_Card2, IA_Card3, IA_Card4, IA_Card5, IA_Card6, IA_UICursor;
    /Game/Input/IMC_Hero; wiring on /Game/Blueprints/BP_HeroCharacter (or the IA_Card1 site per
    handoffs/TASK-009.md).

### TASK-033 — WBP_CardHand + HUD v2 wiring (editor)
- assignee: gameplay-programmer
- status: done (PARTIAL — visual hand UI deferred to a manual UMG task [TASK-041]; data path wired + committed 5bb9507 via TASK-040). DONE: WBP_CardHand created (dup WBP_HUD donor, reparented to UCardHandWidget), Construct→InitForController; WBP_HUD spawns it at runtime (do-once tick, Collapsed) so the C++ seed-then-bind DATA PATH runs end-to-end; M1 gold Construct left byte-intact (zero regression). DEFERRED to a MANUAL UMG designer pass (MCP cannot author widget trees + round-trip would risk regressing the shipped M1 gold counter): the 6-slot visual tree + 3 BIE renderers, preview/refusal text, HUD stat texts (gold-rate/miner-count/overtime), footman-button removal. NOT a code failure — tooling limit; recipes+symbols in handoffs/TASK-033.md. Play/discard works via hotkeys 1-6. THE one known M2 gap for Jonathan's morning.
- blocked-by: TASK-029; TASK-031; TASK-039 (widget base + delegates compiled)
- parallel-safe: no
- spec: >
    Editor/MCP work. Placeholder styling fine (premium UI is M7); bindings exact. (1) Create
    /Game/UI/WBP_CardHand by DUPLICATING a donor (UI_TouchSimple or the existing WBP_HUD — donors
    READ-ONLY, duplicate-and-rewire per CONVENTIONS; MCP cannot author widget trees from scratch);
    reparent to UCardHandWidget. Build: 6 card slots (DisplayName + cost, greyed/disabled when
    bAffordable false — §3.5), each slot a play button (OnClicked → RequestPlaySlot(index)) + a small
    per-slot discard button labeled with the 1-gold cost (OnClicked → RequestDiscardSlot(index), §3.6/§7);
    a next-card preview slot (OnNextCardUpdated); a refusal-message text that shows OnCardRefusedMessage
    for ~2 s (§3.0). Drive everything from the TASK-029 BIEs — no costs/names typed into UMG. (2) WBP_HUD
    v2: add WBP_CardHand to the layout; call InitForController at construct (seed-then-bind); REMOVE the
    M1 single-Footman card button; add gold-rate text ("+N/s", FOnGoldRateChanged), overtime indicator
    (hidden until FOnOvertimeStarted, §3.2), miner count "x/6" (FOnMinerCountChanged) — each seeded from
    getters first (CONVENTIONS law). (3) Verify the M2 input ruling end-to-end and close M1 WARN-4:
    cards clickable under Alt-held cursor AND in placement mode; nothing swallows gameplay LMB when the
    cursor is hidden (WBP roots Not Hit-Testable except interactive children). Acceptance (PIE): 6 slots
    + preview live-update on play/discard/reshuffle; card greys the moment gold drops below its cost;
    discard click deducts 1 and redraws; refusal messages appear and fade; gold rate/miner count/overtime
    indicator all track their delegates; M1 HUD gold counter still correct.
- names: >
    /Game/UI/WBP_CardHand (parent UCardHandWidget; donor UI_TouchSimple or WBP_HUD, duplicated);
    /Game/UI/WBP_HUD (edited). Calls: InitForController, RequestPlaySlot, RequestDiscardSlot. Delegates:
    FOnGoldRateChanged, FOnMinerCountChanged, FOnOvertimeStarted, FOnGoldChanged.

### TASK-034 — BP_Unit_Archer / BP_Unit_Knight / BP_Unit_Miner (editor)
- assignee: gameplay-programmer
- status: done (integrated + committed 5bb9507 via TASK-040). 3 BPs in /Game/Blueprints/Units/ cloning BP_Unit_Footman recipe: Archer(ASummonedUnit/SM_Archer), Knight(ASummonedUnit/SM_Knight), Miner(AMinerUnit/SM_Miner); Team=Blue, -90 yaw, slot0 MI_TeamColor_Blue, no stats on BP, compiled clean. SIE verify PASS: Archer 45/350 ranged, Knight 200/300 melee, Miner 30/350 (no-attack seal holds); Archer/Knight correctly acquired Red Castle (no friendly fire). Deferred to TASK-040: full combat/economy PIE (needs 036 gold nodes + waves). MCP stable. handoffs/TASK-034.md. (TASK-040 commits.)
- blocked-by: TASK-031; TASK-037 (meshes); TASK-039 (AMinerUnit/ranged code compiled)
- parallel-safe: no
- spec: >
    Editor/MCP work. Three BPs in /Game/Blueprints/Units/, all Team default Blue, capsule sized to mesh,
    VisualMesh with the -90 yaw import fix (same convention as BP_Unit_Footman, handoffs/TASK-014.md),
    slot 0 = MI_TeamColor_Blue, NOTHING stat-like set on the BP (all from DT_Cards): (1) BP_Unit_Archer —
    parent ASummonedUnit, CardID Archer, mesh SM_Archer. (2) BP_Unit_Knight — parent ASummonedUnit,
    CardID Knight, mesh SM_Knight. (3) BP_Unit_Miner — parent AMinerUnit, CardID Miner, mesh SM_Miner.
    Acceptance (PIE in L_Arena): Archer reports 45 HP/350 speed, engages from 700 with visible
    projectiles at 1.2 s cadence; Knight reports 200 HP/300 speed and melees for 15 at 1.2 s; Miner
    reports 30 HP/350 speed, walks to GoldNode_Blue and raises the gold rate on arrival; all three die
    correctly and never hit friendlies.
- names: >
    /Game/Blueprints/Units/BP_Unit_Archer (ASummonedUnit, CardID Archer, /Game/Meshes/SM_Archer);
    /Game/Blueprints/Units/BP_Unit_Knight (ASummonedUnit, CardID Knight, /Game/Meshes/SM_Knight);
    /Game/Blueprints/Units/BP_Unit_Miner (AMinerUnit, CardID Miner, /Game/Meshes/SM_Miner);
    material /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-035 — BP_Building_ArrowTower / BP_Building_Wall (editor)
- assignee: gameplay-programmer
- status: done (integrated + committed 5bb9507 via TASK-040). /Game/Blueprints/Buildings/ BP_Building_ArrowTower(ATower/SM_ArrowTower) + BP_Building_Wall(ABuilding/SM_Wall); Team-default, slot0 MI_TeamColor_Blue, BlockAll, bCanEverAffectNavigation=true (Wall pinned per §3.7/TASK-038), no stats on BP, compiled clean. Verify PASS: ArrowTower 150HP/900/1.5/15, Wall 300HP from DT_Cards; LIVE tower-fire check — tower acquired+killed a Red Footman via projectiles, no friendly-fire on Wall (validates ATower+AProjectile+damagetype stack). Deferred to TASK-040: wall reroute via navmesh carve + 300-dmg death + tower 1000-range boundary. MCP stable. handoffs/TASK-035.md. (TASK-040 commits.)
- blocked-by: TASK-031; TASK-038 (meshes); TASK-039 (ABuilding/ATower compiled)
- parallel-safe: no
- spec: >
    Editor/MCP work. New folder /Game/Blueprints/Buildings/ (CONVENTIONS). (1) BP_Building_ArrowTower —
    parent ATower, CardID ArrowTower, VisualMesh SM_ArrowTower, slot 0 MI_TeamColor_Blue. (2)
    BP_Building_Wall — parent ABuilding, CardID Wall, VisualMesh SM_Wall, slot 0 MI_TeamColor_Blue.
    Both: VisualMesh collision blocks Pawns and affects navigation (walls must carve the dynamic navmesh,
    §3.7); nothing stat-like on the BP. Acceptance (PIE): a placed wall visibly reroutes a Footman
    marching castle-to-castle and dies after 300 damage; a placed tower fires at the nearest enemy unit
    inside 900 every 1.5 s for 15 and ignores one at 1000; both refuse friendly damage; stats all from
    DT_Cards rows.
- names: >
    /Game/Blueprints/Buildings/BP_Building_ArrowTower (ATower, CardID ArrowTower,
    /Game/Meshes/SM_ArrowTower); /Game/Blueprints/Buildings/BP_Building_Wall (ABuilding, CardID Wall,
    /Game/Meshes/SM_Wall); material /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-036 — L_Arena v2: gold nodes, arena boundary, KillZ (editor)
- assignee: gameplay-programmer
- status: done (integrated + committed 5bb9507 via TASK-040). L_Arena SAVED (is_dirty=false). GoldNode_Blue(-1200,0,0)/GoldNode_Red(+1200,0,0) AGoldNode w/ SM_GoldNode+M_GoldGlow glowing. Arena boundary = 4 invisible collision walls (engine-cube StaticMeshActors, BlockAll, bHiddenInGame, bCanEverAffectNavigation=FALSE verified) enclosing 6400×3200, 1800 headroom — DEVIATION (orchestrator-accepted): MCP-spawned ABlockingVolume brushes come degenerate, so used property-verified collision boxes instead (functionally identical, flagged for QA). KillZ=-2000. Verify: edge-trace blocks at 100u all 4 sides; 10s PIE miner found GoldNode_Blue, zero nav-fails, deck 50/6. Deferred to TASK-040: exact miner walk-secs, KillZ respawn timing, full escape sweep. handoffs/TASK-036.md. (TASK-040 commits.)
- blocked-by: TASK-038 (SM_GoldNode imported); TASK-039 (AGoldNode compiled)
- parallel-safe: no
- spec: >
    Editor/MCP work in /Game/Maps/L_Arena. (1) Place two AGoldNode actors per the arena contract:
    GoldNode_Blue Team=Blue at (-1200, 0, 0), GoldNode_Red Team=Red at (+1200, 0, 0) (GDD §5 — 800 units
    in front of each castle; M1 reserved these spots). (2) Arena boundary (M1 carry-over): invisible
    blocking volumes fully enclosing the 6400x3200 ground so the hero cannot sprint/jump off the slab;
    leave vertical headroom (>= 1000) so gameplay is untouched; volumes must NOT affect the navmesh
    interior. (3) WorldSettings KillZ = -2000 as the safety net (TASK-024's FellOutOfWorld override
    routes a fallen hero through the 5 s respawn). (4) Verifications: navmesh still covers both halves;
    a Blue miner spawned at (-600, 400) reaches GoldNode_Blue around the castle plinth (M1 carry-over:
    plinth blocks ~410 around each castle anchor — nodes at 800 clear it; record actual walk time,
    expect ~10 s per §3.3); note in the handoff any placement dead-zone feel issues near castles (M2
    watch item — mesh rework only if playtest demands). Acceptance: hero cannot leave the arena from
    any edge at sprint+jump; a forced KillZ fall respawns the hero at its castle in 5–6 s; both gold
    nodes visible + glowing (M_GoldGlow via SM_GoldNode slot); miner pathing verified; PIE clean.
- names: >
    /Game/Maps/L_Arena; actors GoldNode_Blue (-1200,0,0), GoldNode_Red (+1200,0,0) (class AGoldNode);
    WorldSettings KillZ. Mesh (from TASK-038): /Game/Meshes/SM_GoldNode.

### TASK-037 — Unit blockout meshes: SM_Archer, SM_Knight, SM_Miner (art)
- assignee: art-director
- status: done (integrated + committed 5bb9507 via TASK-040). All 3 imported to /Game/Meshes/ (SM_Miner 1124 tris/173u, SM_Archer 1558 tris/180u, SM_Knight 1244 tris/190u; feet-center origin, slot0 MI_TeamColor_Blue, zero warnings, distinct silhouettes Knight>Archer>Miner). FBX in Content/RawAssets/. handoffs/TASK-037.md. .uassets stay UNTRACKED until TASK-040 art commit (NOT TASK-039). Note: collision hulls include weapon overhang — swap to body capsule at BP integration if desired.
- blocked-by: none (Blender MCP available ✓ 2026-07-03 — confirmed UP by Jonathan, verified live)
- parallel-safe: no
- spec: >
    Blender MCP + editor import. Three humanoid blockouts in the SM_Footman family style (GDD §6:
    chunky 2.5–3 heads tall, oversized weapons/hands, silhouette-readable at 15 m, beveled edges,
    <= 8k tris each; blockout tier — no textures, team material does the color): SM_Archer (~180 units
    tall, bow in hand — silhouette must read "ranged" next to SM_Footman); SM_Knight (~190 units, bulky
    armored tank + shield — reads "heavy"); SM_Miner (~170 units, pickaxe over shoulder — reads
    "civilian worker"). ALL: feet-center origin, same export orientation as SM_Footman (BPs apply the
    standard -90 yaw fix, handoffs/TASK-014.md), capsule-friendly proportions, slot 0 =
    MI_TeamColor_Blue on import, FBX saved to Content/RawAssets/<Name>.fbx (CONVENTIONS), import to
    /Game/Meshes/. Work priority: Miner first (M2a), then Archer, then Knight — if the session runs
    long, hand off completed meshes and note the remainder. Gate detail: modeling/FBX export MAY run
    before round-2 sign-off if Blender MCP connects; the editor IMPORT step never does. Acceptance:
    three meshes in /Game/Meshes/ at the named paths, tri counts <= 8k, distinct silhouettes at 15 m in
    the editor viewport, no import warnings, handoff records dimensions + tri counts per mesh.
- names: >
    /Game/Meshes/SM_Archer, /Game/Meshes/SM_Knight, /Game/Meshes/SM_Miner; FBX sources
    Content/RawAssets/Archer.fbx, Knight.fbx, Miner.fbx; material slot 0
    /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-038 — Structure blockout meshes: SM_ArrowTower, SM_Wall, SM_GoldNode + M_GoldGlow (art)
- assignee: art-director
- status: done (integrated + committed 5bb9507 via TASK-040). SM_GoldNode (222 tris, 190×200×249, slot0 M_GoldGlow emissive warm-yellow HDR), SM_Wall (264 tris, 400×100×250 EXACT, UCX box full visual, slot0 MI_TeamColor_Blue), SM_ArrowTower (512 tris, 250×250×497, UCX box = base footprint, slot0 MI_TeamColor_Blue). Ground-center origin, zero import warnings (fixed missing-UV tangent issue). Editor left UP. handoffs/TASK-038.md. FLAGS for TASK-040/build-master: UE Git provider AUTO-STAGED the 4 new .uasset (TASK-037's are untracked — normalize at TASK-040 art commit); confirm bCanEverAffectNavigation=true on BP_Building_Wall (TASK-035, §3.7 navmesh carve).
- blocked-by: none (Blender MCP available ✓ 2026-07-03 — confirmed UP by Jonathan, verified live)
- parallel-safe: no
- spec: >
    Blender MCP + editor import. Three structure blockouts (GDD §6: <= 15k tris each, beveled, blockout
    tier) + one material: (1) SM_GoldNode — glowing ore/crystal cluster ~200x200 footprint, ~250 tall,
    ground-center origin; emissive-ready (single material slot). (2) SM_Wall — straight wall segment
    exactly 400 long x 100 thick x 250 tall (M2 ruling — placement clearance is 200, §3.5), UCX box
    collision matching the visual, ground-center origin. (3) SM_ArrowTower — tower ~250x250 footprint,
    ~500 tall with a readable top platform/arrow-slit silhouette, UCX hulls (keep the footprint tight —
    M1 castle-plinth lesson: oversized collision creates placement dead zones), ground-center origin.
    (4) In-editor at import: create /Game/Materials/M_GoldGlow — emissive warm-yellow (§6 palette "gold
    glows warm yellow"), assign to SM_GoldNode slot 0. SM_Wall + SM_ArrowTower slot 0 =
    MI_TeamColor_Blue. FBX sources to Content/RawAssets/ (CONVENTIONS). Work priority: GoldNode first
    (M2a), then Wall, then ArrowTower. Gate detail: modeling/FBX export MAY run before round-2 sign-off
    if Blender MCP connects; the editor IMPORT step never does. Acceptance: three meshes at the named
    paths with stated dimensions (+/- 10%), collision verified (walls block pawns wall-to-wall, tower
    collision no wider than its base), M_GoldGlow visibly emissive in the viewport, no import warnings,
    handoff records dims/tris/collision per mesh.
- names: >
    /Game/Meshes/SM_ArrowTower, /Game/Meshes/SM_Wall, /Game/Meshes/SM_GoldNode;
    /Game/Materials/M_GoldGlow; FBX sources Content/RawAssets/ArrowTower.fbx, Wall.fbx, GoldNode.fbx;
    material slot 0 (tower/wall) /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-039 — M2 code batch: compile, residue adjudication, commit (build)
- assignee: build-master
- status: done (commit aafd968 on main, NOT pushed; 60 files +7166/-370). Clean compile+LINK on attempt #3 after 2 build-fix loops (UHT param-shadow, then 6× C4458 var-shadows — all mechanical local renames, batch swept shadow-clean). Committed: M2 C++ 021-030 + Build.cs + cards.csv + DefaultEngine.ini + pipeline docs + Blender-bridge infra + new .gitignore (/Content/Dev/). Art .uasset/.fbx left UNTRACKED for TASK-040. Editor left DOWN on the clean aafd968 DLL — TASK-031 boots it. handoffs/TASK-039.md. FOLLOW-UP (manager): add QA-checklist rule for inherited-reflected-member shadows — DONE 2026-07-04, codified in CONVENTIONS.md ("No shadowing inherited reflected members" coding law).
- blocked-by: TASK-021..030 all qa-passed
- parallel-safe: no
- spec: >
    Build-master. Runs ONLY after Jonathan's round-2 sign-off, and after M1's own R1 integration
    (TASK-017/019) has been committed by its separate dispatch — do NOT fold M1 work into this commit.
    (1) Compile the accumulated M2 C++ batch (TASK-021..030) with the standard Build.bat command;
    editor-bounce protocol per the orchestration learnings (editor must release the DLL). Any compile
    error: append to the failing task's qa report, set the task qa-failed, stop — the programmer fixes
    it (counts as a QA loop); build-master never edits code. (2) Adjudicate the M1 working-tree residue
    (M1 checkpoint list): editor boot-resave .uasset deltas, .mcp.json, 2 __ExternalActors__ files,
    untracked Docs/GDD-TEMPLATE.md — commit, restore, or ignore each deliberately and record the ruling
    in the handoff. (3) Commit the M2 code batch + Docs/Data/cards.csv + pipeline docs
    (TASKBOARD/CONVENTIONS/SLACK deltas) with task IDs in the message. Acceptance: clean build; git
    status shows no unexplained residue; commit hash recorded on the board and posted in 🔧 Build & Git.
- names: >
    Build command per CLAUDE.md. Commit message pattern: "TASK-021..030: M2 core-set C++ batch
    (deck/hand, economy v2, projectiles, buildings, miner, placement v2)".

### TASK-040 — M2 final assembly: PIE exit-criteria verification + commit (build)
- assignee: build-master
- status: done (commit 5bb9507 on main, NOT pushed). All asset wiring PASS (zero missing-ref warnings); DT_Cards 6 rows exact (DeckCount sum=50, bRanged only Archer+ArrowTower); live PASS on deck-builds-50 / deals-6, miner pathing, tower auto-fire, and match-end freeze. Interactive criteria (card play via keys, discard, placement clicks, Play Again, 7:00 overtime) DEFERRED to Jonathan's hands-on playtest — MCP has no keypress-injection; the underlying code is all qa-passed. Known gap: TASK-033 visual hand UI → TASK-041 manual pass. WATCH: (1) confirm the Victory screen shows on castle-destroy in a REAL playtest (a Simulate session logged `no ASiegePlayerController to show end screen`); (2) benign `LogCrowdFollowing … UCrowdManager` log at PIE teardown (ignore). handoffs/TASK-040.md.
- blocked-by: TASK-031..038 all done/ready-for-integration
- parallel-safe: no
- spec: >
    Build-master. Final M2 integration in the editor + Git. (1) Verify L_Arena scene state (gold nodes,
    boundary volumes, KillZ) and that all M2 BPs/widgets/DT rows resolve with zero load warnings. (2)
    PIE-run the FULL M2 exit-criteria list (see "M2 exit criteria" above) — verify the M2a slice first,
    then M2b, recording pass/fail per line in the handoff; any fail routes back per the standard QA
    loop. (3) Confirm both §9-2 slices are RECORDABLE (card-hand UI clip + unit targeting/combat clip) —
    actual capture is Jonathan's. (4) Commit all M2 editor/art assets with task IDs; record the hash on
    the board; post the result in 🔧 Build & Git. Do NOT push. (5) Flag on the board that the M2
    checkpoint is ready for Jonathan's playtest. Acceptance: every M2 exit criterion pass/fail recorded;
    commit hash on the board; checkpoint announced.
- names: >
    /Game/Maps/L_Arena; full M2 asset set per TASK-031..038 names blocks. Handoff:
    .claude/pipeline/handoffs/TASK-040.md.

---

