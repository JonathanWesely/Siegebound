# Task Board — GitClaudeUnrealTest

The shared communication hub for the agent team. The **manager** creates tasks here; assignees update their status; the orchestrator routes based on status.

## Status flow

`backlog` → `in-progress` → `ready-for-qa` (code) → `qa-passed` / `qa-failed` → `integrating` → `done`
Art tasks skip QA: `backlog` → `in-progress` → `ready-for-integration` → `integrating` → `done`

## Task template

```
### TASK-000 — <short title>
- assignee: gameplay-programmer | art-director | build-master
- status: backlog
- blocked-by: (task IDs or none)
- parallel-safe: yes | no
- spec: >
    What to build, acceptance criteria.
- names: >
    Exact class/asset names + /Game/ paths to use (from CONVENTIONS.md).
```

---

## Milestones

Source: `Docs/GDD.md` §9. Only the current milestone is decomposed into tasks; later milestones stay one-liners until reached.

1. **M1 — Core loop, local, one card** — `done (playtested + signed off by Jonathan 2026-07-03 evening; round-1 combat-legibility findings all fixed and confirmed)`
2. **M2 — Economy + deck/hand + core set + defenses** — `done-pending-playtest (functionally complete, committed aafd968+5bb9507 not pushed; 1 known gap = visual hand UI manual pass [TASK-041]; awaiting Jonathan's round-1 M2 playtest)` (TASK-021..040; M2a/M2b exit criteria in "M2 manager decisions" below)
3. **M3 — Bot opponent = real 1v1 match** — `done (committed 2f6a8fc + c8a40b2 + 56247c9, not pushed; m3-testable @ 56247c9; slice verified, interactive items pending Jonathan's playtest)`
4. **M4 — Card Set II (16 cards, keywords, hero upgrades)** — `current (decomposed 2026-07-04 → TASK-053..069; Jonathan greenlit M4, developed on main; branches m2-testable @ f903cf0 + m3-testable @ 56247c9 preserve prior slices; see "## M4 tasks")`
5. M5 — Spell system + Set III — `not-started`
6. M6 — Deck-builder meta — `not-started`
7. M7 — Premium art & feel pass — `not-started` · **Jonathan request (2026-07-04):** raise fidelity on SM_Castle + SM_Footman + SM_Archer (higher detail than the current blockouts); wants the game to look nicer. Decision: DEFERRED here (mesh swaps are non-breaking; roster still growing through M4-M6). Two integration paths to scope at M7: (a) art-director custom higher-detail Blender models, and/or (b) **Fab/UE-marketplace assets — Jonathan must download packs into the project via the Epic Launcher first (agents can't browse/buy/download Fab autonomously); art-director then swaps meshes/materials.** Could be pulled forward as a standalone art pass after M3/M4 if Jonathan wants it sooner.
8. M8 — Networked 1v1 multiplayer — `not-started`

### M1 CHECKPOINT — 2026-07-03 (read this first on resume)
**M1 exit criteria are PIE-verified** (all 8 checks passed; commits df4bcd9 → 4d30efb → 8a87400 → 5029403 → 7011b7b → 4255c1d, none pushed). **Current state: playtest round-1 feedback received 2026-07-03 → M1 reopened as `feedback-in-progress`.** Next actions in order:
1. Run TASK-016..020 (see "M1 playtest feedback — round 1" below). Dispatch: TASK-016 + TASK-018 in parallel now; TASK-020 after TASK-016 (shared Build.cs edit); TASK-017 and TASK-019 are editor tasks — one at a time.
2. User re-playtests with real hardware input (round 2) — agents can only inject simulated input, so the LMB finding needs a human hand on the mouse to close.
3. On user go-ahead after round 2: manager decomposes M2 (economy + deck/hand + core set + defenses). Manager should also move the done TASK-001..011 blocks below into ## Done while decomposing.

**2026-07-03 ~15:15 addendum (manager) — Jonathan away for a few hours; standing instructions while he is out:**
- M1 round-2 playtest: when TASK-017 + TASK-019 integration makes round 2 ready, notify Jonathan via Slack (separate dispatch; not part of the M2 breakdown).
- **M2 FILE work is pre-authorized by Jonathan** and starts now: TASK-021..030 (C++ in Source/, cards.csv, config/ini edits) run immediately with QA as usual. **NO compile happens while he is away** — code accumulates QA-passed and uncommitted, exactly like the M1 batch pattern.
- **ALL M2 editor-mutating work and all M2 compile/integration** (TASK-031..036, TASK-039, TASK-040) is `blocked-by: round-2 sign-off` — the playtest environment must not change until Jonathan confirms round 2.
- Blender modeling tasks (TASK-037, TASK-038) additionally need `Blender MCP available` (roll call 2026-07-03: blender-mcp configured but not connected; needs Blender running with the addon).
- M1's TASK-016..020 blocks and statuses were deliberately left untouched by the M2 decomposition (TASK-019 in flight at time of writing).
- **Round-2 READY announced (2026-07-03, manager):** Jonathan notified via channel top-level (ts 1783119963.113549) + DM at commit 4f95730 (TASK-016..020 all done, PIE-verified; editor running the committed state) — awaiting his round-2 playtest; M2 editor/compile gates stay closed until sign-off.

**2026-07-03 evening — M1 ROUND-2 SIGN-OFF (final, manager):** Jonathan playtested and confirmed verbatim "I just did a playtest of milestone 1, everything looks good" — zero new findings. All round-1 combat-legibility fixes confirmed on real hardware, including the LMB real-input finding (TASK-017 carry-item) — now closed. M1 is `done`; TASK-016..020 moved to ## Done; the `round-2 sign-off` blocker is cleared from TASK-031..040. Remaining external gate: `Blender MCP available` on TASK-037/038.

### M1 playtest feedback — round 1 (2026-07-03)
User verdict: core loop works, but combat is illegible. Three findings → five tasks (no new art; all visuals reuse template Variant_Combat donors):
1. **Hero LMB shows no response** — no swing animation, no hit feedback; player cannot tell the castle is being damaged. MCP-injected PIE at M1 exit DID apply damage mechanically, so this is primarily a feedback gap — but TASK-017 must ALSO verify real-input plumbing (input mode / HUD hit-testing swallowing clicks), not assume. → TASK-016 (C++ hooks) + TASK-017 (editor wiring + verification).
2. **No visible castle health bar** — damage progress from footmen/hero unreadable. → TASK-018 (C++ delegate + widget component + widget base class) + TASK-019 (WBP_CastleHealthBar duplicated from Variant_Combat's UI_LifeBar).
3. **Footman has no attacking animation.** Footman is a static mesh — blockout answer is a procedural lunge, NOT a skeletal re-rig (that is M7). → TASK-020 (C++ only).
M1 is NOT complete until TASK-016..020 are `done` and the user confirms in a round-2 playtest. Code tasks route through ready-for-qa as usual; compile/assembly/commit happen at integration (no build-master tasks on the board, per M1 decisions).

**Carry-overs recorded from M1 final assembly & QA (feed into M2+ decomposition):**
- Arena has NO boundary — hero can sprint off the slab and fall forever (blocking volumes or KillZ+respawn; M2). → TASK-036 (volumes + KillZ) + TASK-024 (FellOutOfWorld → respawn path)
- SM_Castle plinth collision spans the full 814×820 base ~90 units high — nothing can stand/be placed within ~410 units of a castle anchor. Affects M2 gold-node (±1200,0) and building placement near castles. → TASK-030 (navmesh-projected placement respects it) + TASK-036 (verify miner pathing/node reachability); mesh-collision rework deliberately deferred (see M2 decisions)
- PlayerStart was MOVED at integration: (-1700,0,100) → (-1400,0,100) (castle collision enclosed the old spot). handoffs/TASK-015.md's transform table is stale on that row.
- Card button unclickable under GameOnly input (no cursor) — keyboard "1" is the trigger until the M2 hand UI (qa/TASK-007-report.md WARN-4). → TASK-023 (hotkeys 1–6 + Alt-held UI cursor) + TASK-033 (clickability verification)
- WBP_VictoryScreen::SetWinner uses a byte param, not ETeamId (MCP tooling can't author enum BP params) — ABI-identical, orchestrator-approved deviation (handoffs/TASK-011.md).
- Match end does not freeze units/income under the Victory screen — TODO(M2) (qa/TASK-006-report.md); → TASK-024 + TASK-028 (FreezeAI). Team-aware PlayerStart selection — TODO(M3); placement lacks navmesh projection (castle roof is placeable) — TODO(M2) → TASK-030.
- Uncommitted working-tree residue (deliberate): editor boot-resave deltas on SM_Castle/SM_Footman .uassets, .mcp.json, 2 pre-existing __ExternalActors__ files, untracked Docs/GDD-TEMPLATE.md. Next build-master decides their fate. → TASK-039 adjudicates.
- Cosmetic: Shift triggers an engine debug-binding log line each press (BaseInput.ini default; overridable in Config/DefaultInput.ini). → optional line item in TASK-032.

### M1 manager decisions (binding for all M1 tasks)
- M1 is built in a **new map `/Game/Maps/L_Arena`** (Content/Maps/L_Arena.umap). `Content/ThirdPerson/Lvl_ThirdPerson` stays untouched.
- `Docs/Data/cards.csv` holds **card rows only** (Footman for M1). Hero (§3.1) and castle (2000 HP) stats stay as C++ UPROPERTY defaults in M1 — they are not cards. Footman stats MUST be read from `DT_Cards` at runtime, never hardcoded (§3.0).
- **Gold nodes are deferred to M2** (miners are M2); L_Arena leaves space for them at ±1200 on the X axis.
- The local player is always **Blue**; the target-practice enemy castle is **Red**. No opponent, no deck/hand, no discard, no miners, no towers/walls, no spells, no upgrades in M1.
- No build-master tasks are listed: compile + scene assembly + commit happen automatically at integration after QA. Integration assembly for M1 = place two `ACastle` actors (`Castle_Blue` Team=Blue at `CastleAnchor_Blue`, `Castle_Red` Team=Red at `CastleAnchor_Red`) in L_Arena and verify the config-default game mode boots the loop.
- Orchestrator note: only ONE editor-mutating task should run at a time (TASK-008/009/010/011/015 and the import step of art tasks share the single editor instance). Pure C++/CSV file tasks and Blender modeling can run in parallel freely.

### M1 exit criteria (playable slice)
Walk the arena as the hero; gold ticks +2/s from 50 on the HUD; play the Footman card (cost 3) onto the Blue half; the Footman paths to the Red castle and attacks it; hero melee also damages it; at 0 castle HP a Victory screen appears; Play Again fully resets gold, units, castles, hero. Recordable core-loop + unit-pathing clip.

---

### M2 manager decisions (binding for all M2 tasks)

**Execution gates (binding policy while Jonathan is away, set 2026-07-03):**
1. **Pure file tasks** (C++ in Source/, cards.csv, Config/*.ini, docs) — dispatchable immediately; `parallel-safe: yes` where they touch different files; QA reviews as usual. **No compile until Jonathan's round-2 sign-off** — code accumulates QA-passed + uncommitted (M1 batch pattern).
2. **ALL editor-mutating tasks** (imports, Blueprints, level/UMG, DT_Cards reimport) and all M2 compile/integration — `blocked-by: round-2 sign-off`. The playtest environment must not change until Jonathan confirms round 2.
3. **Blender modeling tasks** — additionally `blocked-by: Blender MCP available`. If Blender MCP connects before sign-off, the orchestrator MAY run the modeling/FBX-export portion early (file-only), but the editor import step waits for sign-off regardless.
4. M1's TASK-016..020 and their statuses are outside M2's scope.

**Sequencing — M2a / M2b (integration + verification order, NOT extra gates):** M2 is the biggest milestone, so it integrates and PIE-verifies in two slices. **M2a = economy + hand** (deck/hand/discard/preview, miner + gold nodes, overtime, hand UI, input v2 — with Footman-only combat). **M2b = defenses + full core set** (projectiles + damage scaling, Archer/Knight, Arrow Tower/Wall, placement v2). File-task dispatch may interleave freely; build-master verifies the M2a criteria before the M2b criteria at TASK-040; art tasks prioritize M2a meshes (SM_Miner, SM_GoldNode) first.

**Design rulings:**
- **Input model:** default GameOnly free-look is preserved (M1 feel). Hand slots play via hotkeys **1–6** (IA_Card1 exists; IA_Card2..6 new). Holding **Left Alt (IA_UICursor)** switches to GameAndUI + visible cursor (camera look suspended) so cards/discard buttons are mouse-clickable; placement mode already shows the cursor and also allows clicks. This closes M1 WARN-4.
- **Card leaves the hand at CONFIRM, not at placement-entry.** Entering placement mode reserves nothing; cancel costs nothing. Refusals (miner cap, invalid placement) are pre-checked before gold moves — net gold unchanged + HUD reason message satisfies §3.0's refund acceptance.
- **Active miner cap = 6 counts ALIVE miners** (en-route included), enforced at play time via ASiegePlayerState. Cap/clearance/overtime-timing are mechanic rules → UPROPERTY defaults with GDD § comments, not CSV columns (CONVENTIONS).
- **Damage typing:** `USiegeDamageType_Melee/_Projectile`; scaling applied only by ACastle::TakeDamage (projectile 50%, melee/default 100%; Siege 200% is M4, spells M5). Units/hero/buildings take full listed damage from everything.
- **Deferred:** top-of-HUD castle bars (§7) → M3 HUD pass (floating bars from TASK-018/019 remain the baseline); floating unit/building HP bars → M3+ unless round-3 playtest pulls them forward; SM_Castle plinth-collision rework → only if playtest shows the ~410-unit placement dead zone hurts (watch item in TASK-036/040).
- Unchanged M1 laws still in force: stats live in DT_Cards/cards.csv and are never hardcoded; Variant_* donors are READ-ONLY; L_Arena is the map; local player is always Blue; only ONE editor-mutating task at a time.

### M2 exit criteria (playable slice)
**M2a:** hand of exactly 6 cards + next-card preview on the HUD; playing/discarding immediately draws; deck = the §3.4 default 50 (DeckCount column), reshuffles when the draw pile empties; discard costs 1 gold and is refused at 0; Miner (8g) walks to the gold node, +1 gold/s only on arrival, killing it removes the income, 7th refused with "Miner limit reached" and net-zero gold; at 7:00 base income doubles + overtime indicator; HUD shows gold rate and miner count x/6. **M2b:** Archer fires homing projectiles (700 range, 50% vs castle), Knight tanks per its row, Arrow Tower auto-fires at nearest enemy in 900 every 1.5 s, Wall blocks pathing and units reroute; buildings refuse placement within 200 of another building; placement is navmesh-projected (castle roof refused); hero cannot leave the arena; match end freezes units + income; Play Again resets deck/hand/gold/rate/miners/buildings/clock/castles/hero. Recordable: card-hand UI clip + unit targeting/combat clip.

---

## Active tasks

(empty — M1/M2/M3 all complete + committed [none pushed]. **M4 is decomposed-and-READY in ## M4 tasks (TASK-053..069, statuses `backlog`)** — Jonathan greenlit M4; first parallel wave = TASK-053 [card data, files] + TASK-065/066/067 [art meshes, Blender]. Open carry-forwards tracked in the "## M4 tasks" carry-forward block: (a) M3 WATCH transient-Blue-unit cold-boot reconfirm, (b) TASK-046 WARN-2 discard hardening [folded into TASK-060], (c) M2 TASK-041 visual hand UI manual pass [still open].)

---

## M2 tasks (decomposed 2026-07-03)

### M2 COMPLETE — 2026-07-04 (done-pending-playtest; read this first)
All M2 tasks integrated + committed on `main`, **NOT pushed**: C++ batch TASK-021..030 at **aafd968** (via TASK-039); editor/art TASK-031/032/034/035/036/037/038 + assembly TASK-040 at **5bb9507** (via TASK-040). TASK-033 = **done PARTIAL** — the functional data path is wired, but the VISUAL hand UI is deferred to **TASK-041**, a manual UMG pass (MCP cannot author widget trees). That deferral is the ONE known M2 gap.
- **Playable now:** hotkeys **1–6** play hand slots; hold **Left Alt** for the UI cursor (click cards / discard / placement). WASD + mouse + Shift-sprint + LMB melee as in M1.
- **Open M2 items (still part of M2 completion):** TASK-041 (visual hand UI + HUD stat texts) — see "### M2 open items" immediately below; plus a WATCH line to confirm the Victory screen shows on castle-destroy in a REAL playtest (a Simulate-mode session logged `no ASiegePlayerController to show end screen` — M1 shipped it working, so likely a Simulate artifact).
- TASK-021..040 statuses below are flipped to `done`; the full task blocks are left in place (not relocated to ## Done) to preserve the M2 audit trail.

### M2 open items (cleanup group — close these before M2 is fully signed off)

#### TASK-041 — WBP_CardHand visual hand UI + HUD stat texts (manual UMG pass)
- assignee: gameplay-programmer
- status: backlog (PRIORITY — the one known M2 gap; expected to be a manual / Jonathan-assisted session because MCP cannot author widget trees)
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

## M4 tasks (decomposed 2026-07-04)

### M4 — CURRENT (Jonathan greenlit M4 2026-07-04; developed on `main`; branches `m2-testable` @ f903cf0 + `m3-testable` @ 56247c9 preserve prior slices)
M4 = GDD §9-4 + §4 Set II (16 cards) + §3.0 keywords (Siege/Charge/Slayer/Swarm) + §3.8 Siege & Support profiles + §3.10 hero upgrades + §4 Bot Set II extension. **Slice:** expanded-roster combat clip — Ogre push vs Bomb Tower defense. Statuses are `backlog` (Jonathan authorized M4 — NOT held). **Every code task (053–060) implies a QA review** (standard qa loop); the chain ends at build-master integration (TASK-068 compile, TASK-069 final assembly).

**Dispatch order (from blocked-by):**
- **Wave 1 (parallel, start immediately):** TASK-053 (card data, files) + the three art mesh tasks TASK-065 / TASK-066 / TASK-067 (Blender modeling runs parallel with code; the editor-IMPORT step of each art task serializes with all editor-mutating tasks — one editor instance).
- **Wave 2 (after 053 qa-passed):** TASK-054 (new profiles: Siege+Support) and TASK-057 (new buildings) in parallel (different files). Then TASK-055 (keywords) serializes AFTER 054 (shared SummonedUnit files).
- **Wave 3:** TASK-056 (new towers, after 055's radial-AoE helper) + TASK-058 (hero upgrades, after 055's unit damage-buff hook) in parallel; then TASK-059 (play v3, after 055 + 058).
- **Wave 4:** TASK-060 (bot v2) after 054/055/056/057/059 (needs every card behavior + the play/spawn paths).
- **Build:** TASK-068 (M4 code batch compile + commit) after 053–060 all qa-passed.
- **Editor wave (after 068 compiles, one editor task at a time):** TASK-061 (DT_Cards reimport) → TASK-062 (BP units, after unit meshes) / TASK-063 (BP buildings, after building meshes) → TASK-064 (HUD upgrade row).
- **Final:** TASK-069 (M4 final assembly PIE verify + commit) last.
- **First parallel wave to launch NOW:** TASK-053 + TASK-065 + TASK-066 + TASK-067.

### M4 design rulings (binding for all M4 tasks)
- **M4 test deck (supersedes §3.4 core-only default until M6):** the DeckCount column is redistributed across all 22 rows into an expanded 50-card deck so the full roster is reachable in the hand for BOTH the player and the bot before the M6 deck-builder. Must sum to exactly 50, each ≤ MaxCopies, ≥1 of every card. First-pass (tunable): Footman 6, Archer 4, Wall 3, ArrowTower 2, Knight 2, Miner 3 (core = 20); MilitiaMob 2, Pikeman 2, Sapper 2, Cavalry 2, Longbowman 2, Cleric 2, Ogre 2 (= 14); BombTower 2, BallistaTower 2, Barracks 2, DeepMine 2 (= 8); Masons 2, SharpenedBlade 2, PlateArmor 2, SwiftBoots 1, WarBanner 1 (= 8). This changes M3's exact core-copy-count acceptance — a completed-milestone artifact M4 supersedes by design.
- **FCardRow columns** grow per the CONVENTIONS registry (added there 2026-07-04): `bCharge`, `bSlayer`, `bSuicide`, `SwarmCount`, `AoERadius`, `MinRange`, `SpawnCardID`, `SpawnInterval`, `Lifetime` — CSV header must map 1:1 for reimport. Siege/Support are the existing `Profile` column (targeting); Instant is derived from `CardType` (HeroUpgrade / Utility) — no play-placement step.
- **Siege (keyword + profile):** new `USiegeDamageType_Siege`; Profile=Siege units ignore units/hero and target buildings then the castle (§3.8); their attacks tag Siege damage → 200% vs castle AND buildings (§3.0). Sapper is a Siege unit with `bSuicide` (explode-on-contact/death AoE); Ogre is a Siege tank.
- **Keyword magnitudes are mechanic RULES → UPROPERTY // GDD, not CSV:** Charge (2 s move / 2×), Slayer (150-HP / 2×), Swarm (300-unit circle), Deep Mine (+2 gold/s), Masons (300 HP over 10 s), hero-upgrade effects (§3.10). Per-card numbers that DO vary live in CSV columns (SwarmCount, AoERadius, MinRange, spawner triple).
- **Support/Cleric:** Profile=Support unit never attacks; follows the nearest damaged friendly within Range (400) and heals `Damage`/sec (8); if none damaged, follows the nearest friendly combat unit (§3.8). Heal = negative damage / AddHP, no friendly-fire issue.
- **Hero upgrades (§3.10):** applied via a hero-side upgrade component/state; persist through hero death; reset on match end / Play Again; stack cap = MaxCopies; over-cap play refused + FULL refund (§3.0 refund rule); a `FOnHeroUpgradesChanged` delegate drives the §7 HUD icon row. War Banner is an aura (600, +20% friendly-unit damage) applied through the unit damage-buff hook (TASK-055). The bot has no hero → it treats HeroUpgrade/Utility/Instant/Spell cards as discards (§4 M4 extension).
- **Bot v2:** plays Set II by the same §4 ordered rules (rule 3 now reaches Ogre/Cavalry/etc.); spawns honor SwarmCount + Team=Red + team material; **folds the TASK-046 WARN-2 hardening** — the rule-4 "discard most-expensive unplayable card" path now goes live (Set II adds hero-upgrade/Instant cards the bot can't play) and must guard the discard fee / DiscardFromHand return.
- **Art stays blockout tier** (premium art DEFERRED to M7 per Jonathan 2026-07-04). New units/buildings get `SM_<CardID>` blockout meshes; Militia Mob MAY reuse SM_Footman and Longbowman MAY reuse SM_Archer to save art (set the BP VisualMesh accordingly; the ghost-preview falls back to a sphere for a missing SM_<CardID>). Masons + the four hero upgrades are Instant — no actor, no mesh.
- Unchanged laws still in force: stats in DT_Cards/cards.csv (never hardcode a table stat); `Variant_*` donors READ-ONLY (UMG duplicate-and-rewire); `L_Arena` is the arena; local player always Blue / bot always Red; spawn by composed BP path `/Game/Blueprints/Units|Buildings/BP_*_<CardID>` (null-safe); seed-then-bind for UI; delegate `FOn<Owner><Event>` naming; **no shadowing inherited reflected UPROPERTYs** (C4457/58/59 hard errors — QA must scan every code task); only ONE editor-mutating task at a time.

### M4 WATCH items (confirm at playtest / carry as needed)
- **Transient Blue unit at match start** (M3 carry-forward a): cold-boot reconfirm at Jonathan's M4 playtest; becomes a PlayAgain/world-teardown task only if it recurs.
- **M2 TASK-041 visual hand UI** still open — shares WBP_HUD with TASK-064; do 041 first or fold to avoid a colliding UMG pass.

### TASK-053 — Card data: FCardRow keyword columns + cards.csv Set II rows + M4 test deck (files)
- assignee: gameplay-programmer
- status: qa-passed (qa/TASK-053-report.md — 0 blk/0 warn/0 nit). CONFIRMED: header↔FCardRow 1:1 name+order (TASK-061 reimport will warn zero); DeckCount = exactly 50 (each ≥1 ≤MaxCopies); all 16 Set II rows §4 character-exact; 6 core rows byte-unchanged; keyword-column sparsity clean; no enum adds/shadowing. Data foundation for TASK-054-060. handoffs/TASK-053.md.
- blocked-by: none
- parallel-safe: yes
- spec: >
    Files only, no editor/no compile. (1) FCardRow (CardRow.h) — add nine UPROPERTYs (names must match CSV
    headers 1:1 for reimport; CONVENTIONS registry): bool bCharge=false; bool bSlayer=false; bool
    bSuicide=false; int32 SwarmCount=0; float AoERadius=0.f; float MinRange=0.f; FName SpawnCardID=NAME_None;
    float SpawnInterval=0.f; float Lifetime=0.f. The ECardType (Unit/Building/Economy/Spell/HeroUpgrade/
    Utility) and ECardProfile (None/Standard/Siege/Support) enums ALREADY exist — do not add enum values.
    (2) Docs/Data/cards.csv — append the nine columns to the header AND all six existing rows (defaults
    false/0/None; no behavior change to core cards). Then add the sixteen Set II rows EXACTLY per GDD §4:
    MilitiaMob (DisplayName "Militia Mob"): Unit, Cost 5, Max 6, HP 25, Dmg 6, Range 120, Cadence 1.0, Speed
    400, Standard, SwarmCount 4 (spawns 4 × 25 HP for one cost).
    Pikeman: Unit, 5, 6, HP 100, Dmg 30, Range 120, Cadence 1.5, Speed 350, Standard, bSlayer true.
    Sapper: Unit, 5, 4, HP 60, Dmg 80, Range 120, Cadence 1.0, Speed 500, Siege, AoERadius 250, bSuicide true.
    Cavalry: Unit, 7, 4, HP 140, Dmg 20, Range 120, Cadence 1.0, Speed 600, Standard, bCharge true.
    Longbowman: Unit, 6, 4, HP 70, Dmg 18, Range 1200, Cadence 1.5, Speed 300, Standard, bRanged true.
    Cleric: Unit, 6, 3, HP 90, Dmg 8, Range 400, Cadence 1.0, Speed 350, Support (Dmg 8 = heal HP/sec, §3.8;
    no attack).
    Ogre: Unit, 12, 2, HP 500, Dmg 35, Range 120, Cadence 1.5, Speed 250, Siege.
    BombTower (DisplayName "Bomb Tower"): Building, 8, 4, HP 180, Dmg 25, Range 800, Cadence 2.5, Speed 0,
    None profile, bRanged true, AoERadius 250.
    BallistaTower (DisplayName "Ballista Tower"): Building, 7, 4, HP 120, Dmg 45, Range 1400, Cadence 3.0,
    Speed 0, None, bRanged true, MinRange 300.
    Barracks: Building, 10, 3, HP 250, Dmg 0, Range 0, Cadence 0, Speed 0, None, SpawnCardID Footman,
    SpawnInterval 8, Lifetime 60.
    DeepMine (DisplayName "Deep Mine"): Economy, 15, 2, HP 200, Dmg 0, Range 0, Cadence 0, Speed 0, None
    (+2 gold/s income is a UPROPERTY on ADeepMine, not a column).
    Masons: Utility, 8, 3, all stats 0, None (Instant; 300 HP / 10 s is a UPROPERTY on the effect).
    SharpenedBlade (DisplayName "Sharpened Blade"): HeroUpgrade, 6, 2, all stats 0.
    PlateArmor (DisplayName "Plate Armor"): HeroUpgrade, 6, 2, all stats 0.
    SwiftBoots (DisplayName "Swift Boots"): HeroUpgrade, 5, 1, all stats 0.
    WarBanner (DisplayName "War Banner"): HeroUpgrade, 8, 1, all stats 0.
    (3) Redistribute the DeckCount column across all 22 rows into the M4 test deck (design ruling above) —
    must sum to exactly 50, each ≤ MaxCopies, ≥1 per card. Acceptance: CSV header maps 1:1 onto FCardRow
    (reimport at TASK-061 must warn zero); exactly 22 rows; DeckCount sums to exactly 50; every §4 value
    character-exact; keyword columns set ONLY where §4 specifies; no stat invented outside the GDD.
- names: >
    Source/GitClaudeUnrealTest/Siegebound/CardRow.h (FCardRow — new UPROPERTYs bCharge, bSlayer, bSuicide,
    SwarmCount, AoERadius, MinRange, SpawnCardID, SpawnInterval, Lifetime); Docs/Data/cards.csv. Row CardIDs
    (PascalCase, no spaces): MilitiaMob, Pikeman, Sapper, Cavalry, Longbowman, Cleric, Ogre, BombTower,
    BallistaTower, Barracks, DeepMine, Masons, SharpenedBlade, PlateArmor, SwiftBoots, WarBanner. Import
    target (TASK-061): /Game/Data/DT_Cards.

### TASK-054 — New targeting profiles: Siege + Support + Siege damage type & castle/building 200% (files)
- assignee: gameplay-programmer
- status: qa-passed (qa/TASK-054-report.md — 0 blk/0 warn/3 nit; both interp calls ACCEPTED [Siege nearest-building map-wide, Support fallback follows combat units]). NON-REGRESSION CONFIRMED: Standard/melee/ranged/miners byte-identical M3 (Profile dispatch returns before untouched Standard body); castle Siege×2 before Projectile×0.5-castle-only; building Siege×2 branch, ScaledDamage==ActualDamage for non-Siege. Cleric heals 8/s≤400 clamped, never attacks. FreezeAI stops healing. C4458 clean, TASK-055 seam clean. handoffs/TASK-054-programmer.md. Ready for TASK-068 batch.
- blocked-by: TASK-053
- parallel-safe: yes (parallel with TASK-057 — different files; TASK-055 serializes AFTER it on SummonedUnit)
- spec: >
    Files only. GDD §3.8 (Siege + Support profiles) + §3.0 (Siege 200%). Large but coherent — if the session
    runs long, hand off Siege first, then Support. (1) DamageTypes.h/.cpp: add USiegeDamageType_Siege
    (UDamageType subclass, no logic — CONVENTIONS damage-type registry). (2) ACastle::TakeDamage AND
    ABuilding::TakeDamage: read DamageEvent.DamageTypeClass — USiegeDamageType_Siege = 200%; keep
    Projectile 50% / Melee/default 100% (castle only had this from M2 — ADD the Siege branch to both; buildings
    otherwise still take listed damage). Leave the TODO(Spell 50%) marker for M5. (3) ASummonedUnit — implement
    the two profiles the M2 state machine stubbed (only Standard shipped): SIEGE profile (Ogre, Sapper): ignore
    units and the hero entirely in Acquire; target the nearest enemy ABuilding in path, else the enemy castle
    (§3.8); tag its dealt damage with USiegeDamageType_Siege. SUPPORT profile (Cleric): never attacks; in
    Acquire, follow the nearest DAMAGED friendly ASummonedUnit within Range (row 400) and heal row Damage
    (8) HP/sec continuously while in range (heal = clamp to MaxHP, no friendly-fire); if no friendly is
    damaged, follow the nearest friendly combat unit (§3.8). Standard profile byte-unchanged. FreezeAI must
    also stop Siege pathing and Support following/healing. Do NOT disturb the -90 yaw VisualMesh, the M1
    melee/lunge, or the M2 ranged path. Acceptance: an Ogre walks past enemy units without engaging and hits
    the first wall/tower then the castle, dealing 200% to castle/buildings; a Cleric follows and heals the
    nearest damaged friendly at 8 HP/s within 400, heals no enemies, never attacks; Standard units unchanged.
- names: >
    USiegeDamageType_Siege in Source/GitClaudeUnrealTest/Siegebound/DamageTypes.h/.cpp. ACastle
    (Siegebound/Castle.h/.cpp), ABuilding (Siegebound/Building.h/.cpp) — Siege 200% scaling in TakeDamage.
    ASummonedUnit (Siegebound/SummonedUnit.h/.cpp) — Siege + Support profile branches; reads ECardProfile
    (CardRow.h). Data rows: Ogre, Sapper (Siege), Cleric (Support).

### TASK-055 — Standard keywords: Charge / Slayer / Swarm + damage-multiplier infra + AoE radial helper + Sapper suicide (files)
- assignee: gameplay-programmer
- status: qa-passed (qa/TASK-055-report.md — 0 blk/0 warn/3 nit). CONFIRMED: non-keyword ComputeOutputDamage bit-for-bit AttackDamage; ONE-hit charge (bChargePrimed consumed once); SINGLE detonation (bDetonated+bDead guards); drift-free aura (multiplier separate, restore 1.0f, FreezeAI/EndPlay clear); NO friendly-fire radial (GetTeamId==Team sole authority, closest-point reaches fortifications, routes through TakeDamage so Siege fires). Downstream signatures verified: FSiegeCombatStatics::ApplyRadialDamage (→056), ASummonedUnit::SetAuraDamageBonus (→058). No Build.cs. C4458 clean. NIT: fold the 3 closest-point mirrors into SiegeCombatStatics later (qa/TASK-026 NIT-4). handoffs/TASK-055.md. Ready for TASK-068 batch.
- blocked-by: TASK-053, TASK-054 (serialize — shares SummonedUnit files; needs USiegeDamageType_Siege)
- parallel-safe: no (SummonedUnit serial hub; TASK-056 and TASK-058 depend on symbols added here)
- spec: >
    Files only. GDD §3.0 keywords + §4. Centralize ASummonedUnit damage OUTPUT so every modifier composes in
    one place: dealt = base row Damage × Charge × Slayer × AuraBonus (Siege 200% is applied fortification-side
    by TASK-054's damage type, NOT here). (1) CHARGE (bCharge, Cavalry): track uninterrupted-movement seconds;
    the first attack after ≥ ChargeMoveSeconds (2.f // GDD §3.0) of continuous movement deals ×ChargeMultiplier
    (2.f); reset the timer when the unit is blocked/attacking. (2) SLAYER (bSlayer, Pikeman): ×SlayerMultiplier
    (2.f) vs any target whose MaxHP ≥ SlayerHPThreshold (150.f // GDD §3.0). (3) AURA HOOK (for War Banner,
    TASK-058): SetAuraDamageBonus(float Bonus, float Duration) (BlueprintCallable) — a temporary additive
    output multiplier applied by a friendly hero's War Banner; refresh-not-stack; restore exactly on expiry;
    FreezeAI clears it (same drift-free discipline as ApplyMoveSpeedBuff). (4) AoE radial helper: a shared
    static ApplyRadialDamage(World, InstigatorController, Team, Center, Radius, Damage, DamageTypeClass) that
    damages all ENEMY combat actors within Radius (no friendly fire) — authored once here, reused by Bomb Tower
    (TASK-056). (5) SUICIDE (bSuicide, Sapper): a Siege+bSuicide unit, on reaching attack range OR on death,
    calls ApplyRadialDamage(AoERadius 250, Damage 80, Siege type) at its location then destroys itself — a
    single detonation, no repeat attacks. Acceptance: a Cavalry's first post-charge hit deals 40 (2×20) then
    reverts to 20; a Pikeman deals 60 to a 200-HP Knight but 30 to an 80-HP Footman; a Sapper explodes once on
    contact for 80 AoE in 250 (2× vs castle/buildings via Siege type) and dies; the aura hook adds then cleanly
    removes its bonus; nothing stat-like hardcoded that lives in DT_Cards.
- names: >
    ASummonedUnit (Siegebound/SummonedUnit.h/.cpp) — Charge/Slayer output multipliers, SetAuraDamageBonus,
    bSuicide detonation; UPROPERTYs ChargeMoveSeconds, ChargeMultiplier, SlayerMultiplier, SlayerHPThreshold.
    Shared ApplyRadialDamage static (Siegebound/ — e.g. SiegeCombatStatics or on DamageTypes.cpp). Reads
    bCharge/bSlayer/bSuicide/AoERadius [TASK-053]; uses USiegeDamageType_Siege [TASK-054]. Data rows: Cavalry,
    Pikeman, Sapper, Militia Mob (SwarmCount consumed spawn-side in TASK-059/060).

### TASK-056 — New towers: Bomb Tower (AoE) + Ballista Tower (min-range blind spot) (files)
- assignee: gameplay-programmer
- status: qa-passed (qa/TASK-056-report.md — 0 blk/0 warn/2 nit). CONFIRMED: single-target/ArrowTower byte-unchanged (trailing default InAoERadius=0, sole 4-arg Archer caller binds, AoE branch gated AoERadius>0, ArrowTower MinRangeSq==0 inert); Bomb AoE 25-in-250 via ApplyRadialDamage (7-arg sig matches, closest-point catches primary); Ballista MinRangeSq skip ring [300,1400] max-gate intact; NO friendly fire (Bomb carries tower Team). Row-driven ATower (both BPs parent ATower, TASK-063). No Build.cs. C4458 clean. handoffs/TASK-056.md. Ready for TASK-068 batch.
- blocked-by: TASK-053, TASK-055 (reuses ApplyRadialDamage; AProjectile AoE)
- parallel-safe: yes (parallel with TASK-057/058 — different files, once 055 lands)
- spec: >
    Files only. GDD §3.7/§4. Both are ATower-family buildings driven by DT_Cards; nothing stat-like on the BP.
    (1) AProjectile: add AoERadius support — an InitProjectile overload (or param) carrying AoERadius; on impact,
    if AoERadius > 0 call ApplyRadialDamage (TASK-055) at the impact point instead of single-target, then
    destroy + spawn the NS_Damage impact (existing donor). (2) Bomb Tower behavior: ATower firing every Cadence
    (row 2.5 s) at the nearest enemy unit/hero within Range (800); fires an AoE projectile (row Damage 25,
    AoERadius 250, Projectile type) — anti-swarm splash (§3.7 "damages all units within its 250-unit impact
    radius"). (3) Ballista Tower behavior: long-range single-target — nearest valid enemy within Range (1400)
    but OUTSIDE MinRange (row 300 — the blind spot, §4); fires every Cadence (3.0 s) for row Damage 45
    (Projectile type). A target inside 300 is ignored (re-scan next cadence). Both: no friendly fire; reuse the
    M2 ATower acquire/idle loop; stats all from DT_Cards. Decide in-code whether Bomb/Ballista are ATower with
    row-driven branching (AoERadius>0 / MinRange>0) or thin ATower subclasses — prefer row-driven ATower so no
    new class is needed unless behavior demands it; record the choice in the handoff. Acceptance: a Bomb Tower
    hits a 4-unit cluster with one 25-damage blast in 250 every 2.5 s; a Ballista hits a target at 1200 for 45
    but ignores one at 250 and fires every 3.0 s; both ignore friendlies and read all stats from the table.
- names: >
    AProjectile (Siegebound/Projectile.h/.cpp) — AoERadius on InitProjectile, reuses ApplyRadialDamage
    [TASK-055]. ATower (Siegebound/Tower.h/.cpp) — Bomb/Ballista behavior (row-driven AoERadius/MinRange), or
    new subclasses if required (record in handoff). Data rows: BombTower, BallistaTower. BP children (TASK-063):
    /Game/Blueprints/Buildings/BP_Building_BombTower, BP_Building_BallistaTower.

### TASK-057 — New buildings: Barracks spawner + Deep Mine economy (files)
- assignee: gameplay-programmer
- status: qa-passed (qa/TASK-057-report.md — 0 blk/1 warn/3 nit). M2/M3 miner economy BYTE-PRESERVED (symbol-traced: FlatIncomePerTick isolated, GetGoldRate base+miner unchanged, flat not overtime-doubled, ResetEconomy zeroes it, miner cap never gates DeepMines). Freeze step-2b CLEAN (additive, no double-handle, synchronous). WARN: mis-teamed DeepMine idle retry timer (bounded, never in designed flows). C4458 clean. CARRY→TASK-059: route Economy-typed DeepMine down building spawn path. handoffs/TASK-057-programmer.md. Ready for TASK-068 batch.
- blocked-by: TASK-053
- parallel-safe: yes (parallel with TASK-054/055 — different files; new class pairs + a PlayerState income addition)
- spec: >
    Files only. GDD §4/§8 + §3.3 economy pattern. Two ABuilding subclasses, DT_Cards-driven, team-resolved
    economy via GetPlayerStateForTeam (TASK-043). (1) ABarracks : ABuilding — on BeginPlay start a repeating
    timer of row SpawnInterval (8 s); each tick spawn the composed BP for row SpawnCardID (Footman) —
    /Game/Blueprints/Units/BP_Unit_Footman — Team = own team (team material via TASK-044), at a small offset in
    front of the Barracks on the navmesh; after row Lifetime (60 s) destroy self (its spawned units persist).
    FreezeAI/match-end must stop the spawn timer (hook the existing freeze sweep). (2) ADeepMine : ABuilding —
    raidable economy building (§8): on BeginPlay register +DeepMineIncome (UPROPERTY 2 // GDD §8, +2 gold/s) on
    the owning team's ASiegePlayerState IMMEDIATELY (no walk, unlike Miner); on death unregister it. Reuse/extend
    the PlayerState income API (AddMinerIncome pattern → a generic AddIncome(int32)/RemoveIncome(int32) or a
    DeepMine-specific pair) so GetGoldRate composes base + miners + deep mines; the miner cap does NOT apply to
    Deep Mines. 200 HP, destructible, blocks pathing like a building. Acceptance: a Barracks spawns one Footman
    every 8 s of its own team, stops + is destroyed at 60 s, and freezes at match end; a Deep Mine raises the
    owner's rate by +2/s the instant it is placed and drops it by 2 when destroyed; the miner cap is unaffected;
    all values from DT_Cards / the documented UPROPERTY.
- names: >
    ABarracks in Source/GitClaudeUnrealTest/Siegebound/Barracks.h/.cpp; ADeepMine in
    Source/GitClaudeUnrealTest/Siegebound/DeepMine.h/.cpp — UPROPERTY DeepMineIncome. ASiegePlayerState
    (Siegebound/SiegePlayerState.h/.cpp) — income API extension for non-miner income. Uses GetPlayerStateForTeam
    [TASK-043]. Spawns /Game/Blueprints/Units/BP_Unit_Footman. BP children (TASK-063):
    /Game/Blueprints/Buildings/BP_Building_Barracks (ABarracks), BP_Building_DeepMine (ADeepMine).

### TASK-058 — Hero upgrade system + War Banner aura (files)
- assignee: gameplay-programmer
- status: qa-passed (qa/TASK-058-report.md — 0 blk/0 warn/1 nit). Hero non-regression byte-preserved @ 0 stacks: sprint 500/750 (ApplyMovementSpeed single live-writer + bSprinting; SwiftBoots composes 625/937.5), melee 20 (getter not hardcoded, TASK-016/017 feedback intact), HP 200+regen (all clamps route through GetEffectiveMaxHP, PlateArmor heals 100 clean), Rally untouched. 4 upgrades work, DRIFT-FREE (only 4 int32 stacks mutate), refund contract clean, caps from MaxCopies. WarBanner aura signature matches TASK-055. PlayAgain-reset gap VERIFIED clean 059 carry (ResetHero is shared respawn+PlayAgain path). NIT: 2 direct MaxWalkSpeed writes (HandleDeath/ctor) provably consistent. handoffs/TASK-058-programmer.md. Ready for TASK-068 batch. ⚠️CARRY→TASK-059: add Hero->ResetUpgrades() in SiegeGameMode::PlayAgain.
- blocked-by: TASK-053, TASK-055 (uses the SetAuraDamageBonus unit hook)
- parallel-safe: yes (edits HeroCharacter + a new component — no other M4 task edits HeroCharacter)
- spec: >
    Files only. GDD §3.10 (hero upgrades) + §4 (War Banner aura). Implement the four Instant hero upgrades as a
    hero-side upgrade state (a UHeroUpgradeComponent on AHeroCharacter, or upgrade state on the hero — choose,
    record in handoff). ApplyUpgrade(FName UpgradeCardID) (BlueprintCallable, called by TASK-059's Instant path):
    (1) SharpenedBlade → +MeleeDamageBonus (10 // GDD §3.10) per stack, cap 2; (2) PlateArmor → +MaxHPBonus
    (100) per stack AND heal 100 immediately, cap 2; (3) SwiftBoots → +MoveSpeedBonus (25% // to base move AND
    sprint) per stack, cap 1; (4) WarBanner → enable an aura (600 radius, +20% friendly-unit damage // GDD §4),
    cap 1 — periodically (or on a timer) call SetAuraDamageBonus(0.20, ...) on friendly ASummonedUnit within
    RallyRadius-style 600 (like Rally's speed buff, TASK-042). Stack caps = MaxCopies (read from DT_Cards).
    Return a bool/enum so the caller can refund an over-cap play (§3.0 refund). Upgrades PERSIST through hero
    death (re-apply cumulative mods on respawn) and RESET on match end / Play Again. Broadcast
    FOnHeroUpgradesChanged (active upgrades + stack counts) for the §7 HUD icon row (TASK-064). Values are
    UPROPERTY // GDD §3.10 defaults, never CSV. Do NOT regress M1/M2 hero movement, the 20-dmg cone melee, the
    200 HP base, regen, or M3 Rally. Acceptance: one Sharpened Blade makes the cone melee deal 30, a second 40,
    a third refused + refunded; Plate Armor takes maxHP 200→300 and heals 100 now; Swift Boots raises move/
    sprint 25%; War Banner gives friendly units within 600 +20% damage; all survive respawn and reset on Play
    Again; the delegate reports the upgrade row.
- names: >
    AHeroCharacter (Siegebound/HeroCharacter.h/.cpp) + UHeroUpgradeComponent (Siegebound/
    HeroUpgradeComponent.h/.cpp, if used) — ApplyUpgrade, ResetUpgrades; UPROPERTYs MeleeDamageBonus,
    MaxHPBonus, MoveSpeedBonus, WarBannerAuraRadius, WarBannerDamageBonus; delegate FOnHeroUpgradesChanged
    (member OnHeroUpgradesChanged). Uses ASummonedUnit::SetAuraDamageBonus [TASK-055]; reads MaxCopies (stack
    cap) from DT_Cards. Upgrade CardIDs: SharpenedBlade, PlateArmor, SwiftBoots, WarBanner. HUD consumer:
    TASK-064.

### TASK-059 — Play v3: Instant/upgrade routing + Masons castle-heal + Swarm multi-spawn + stack-cap refund (files)
- assignee: gameplay-programmer
- status: qa-passed (qa/TASK-059-report.md — 0 blk/2 warn/2 nit; both flagged items ACCEPTED). CONFIRMED: melee-suppression intact all exit paths; net-zero refund all branches (Swarm unwinds copies on fail); existing Unit/Building/Miner/placement-v2 byte-preserved; apply-then-spend SAFE (CanAfford pre-check, ApplyUpgrade first, SpendGold only on Applied); SpawnUnitSwarm clean static (TASK-060 consumes as-is); IsBuildingCard routes DeepMine→building path (not miner-capped), Miner still unit; Masons 300/10 clamp+cancel clean. WARNs benign (post-match Masons tick on winner castle out-of-scope; off-navmesh ring fallback). C4458 clean. handoffs/TASK-059.md. Ready for TASK-068 batch.
- blocked-by: TASK-053, TASK-055 (SwarmCount), TASK-058 (ApplyUpgrade)
- parallel-safe: yes (edits SiegePlayerController + a Castle heal-over-time; serialize after TASK-058 only for the symbol)
- spec: >
    Files only. GDD §3.5/§3.6/§3.10 + §3.0 refund. Extend ASiegePlayerController's play path (built in
    TASK-023/030). (1) INSTANT routing: CardType HeroUpgrade or Utility resolves immediately with NO placement
    step (§3.5) — spend row Cost, apply the effect, then ConfirmPlayFromHand(Slot) to draw a replacement. Hero
    upgrades → ApplyUpgrade (TASK-058); if over stack cap, refuse with NO spend and OnCardRefused("… at max
    stacks") (full-refund rule, §3.10). (2) MASONS (Utility Instant): restore 300 castle HP over 10 s to the
    friendly castle — add ACastle::HealOverTime(float Total, float Duration) (or a heal component) clamped to
    MaxHP, broadcasting the existing FOnCastleHPChanged; Masons values are UPROPERTY // GDD §4. (3) SWARM
    multi-spawn: when confirming a Unit card with SwarmCount > 0, spawn SwarmCount copies in a 300-unit circle
    (UPROPERTY SwarmSpawnRadius // GDD §3.0) around the placement point (navmesh-projected each), for ONE Cost —
    reuse a shared spawn path so the bot (TASK-060) matches. (4) Preserve the M2 melee-suppression law on every
    exit path, and the net-zero refund discipline. Acceptance: playing Sharpened Blade deducts 6 and buffs the
    hero with no placement; a 3rd Sharpened Blade is refused with a message and unchanged gold; Masons deducts 8
    and heals the friendly castle 300 over 10 s (never over MaxHP); Militia Mob spawns 4 units in a 300 circle
    for one cost of 5; unaffordable/over-cap plays never move gold.
- names: >
    ASiegePlayerController (Siegebound/SiegePlayerController.h/.cpp) — Instant routing, Swarm multi-spawn,
    stack-cap refund via OnCardRefused; UPROPERTY SwarmSpawnRadius. ACastle (Siegebound/Castle.h/.cpp) —
    HealOverTime. Uses ApplyUpgrade [TASK-058], SwarmCount/AoERadius rows [TASK-053], ConfirmPlayFromHand
    [TASK-022]. Instant CardIDs: SharpenedBlade, PlateArmor, SwiftBoots, WarBanner, Masons.

### TASK-060 — Bot v2: play Set II + treat upgrades/instants as discard + rule-4 discard hardening (files)
- assignee: gameplay-programmer
- status: qa-passed (qa/TASK-060-report.md — 0 blk/1 warn/1 nit). Invariants CONFIRMED (never unaffordable, never Blue-half center); M3-core byte-preserved; rule-4 fix GENUINE (closes TASK-046 WARN-2 — no double/0 charge); SwarmCount via SpawnUnitSwarm (4 Red copies, unwind on fail). Type-unplayable-only discard RULED ACCEPTABLE (no deadlock — 50g start, income accrues; excluding banked units preserves "growing Ogre waves"). WARN: swarm-fan never-Blue-half contingent on Center.X≥radius (holds in L_Arena centerline 350; same shared-helper property as player/TASK-059; hard-clamp optional). handoffs/TASK-060.md. Ready for TASK-068 batch.
- blocked-by: TASK-054, TASK-055, TASK-056, TASK-057, TASK-059 (needs every card behavior + the play/spawn paths)
- parallel-safe: yes (bot-internal — only edits SiegeBotController)
- spec: >
    Files only. Extend ASiegeBotController::EvaluateDecisions (TASK-046) for Set II by the SAME §4 ordered rules
    — no new fuzzy logic. (1) Rule 3 "most expensive affordable UNIT" now naturally reaches Ogre/Cavalry/
    Pikeman/etc.; rule 1 defensive plays may now use the new towers; rule 2 economy may use Deep Mine as well as
    Miner where the rule fits (keep it simple — Deep Mine counts as an economy play, no miner-cap interaction).
    Bot spawns honor SwarmCount (Militia Mob → 4 copies) via the shared spawn path (TASK-059), Team=Red, team
    material (TASK-044), on a navmesh-projected own-half (X≥0) clearance-valid point. (2) The bot has NO hero →
    HeroUpgrade / Utility(Masons) / Spell / Instant cards are treated as DISCARDS (§4 M4 extension) — never
    "played". (3) HARDEN rule 4 (folds M3 TASK-046 WARN-2, now live because Set II adds cards the bot cannot
    play): guard the 1-gold discard fee (gold ≥ 1) and check the DiscardFromHand result before charging;
    "most-expensive unplayable card" selection must correctly classify hero-upgrade/Instant/unaffordable cards
    as unplayable. Keep the invariants: NEVER play a card it can't afford; NEVER spawn on the Blue half; exactly
    one LogSiegeBot line per fired rule. Acceptance (§4): player idle → bot builds economy then attacks with
    growing Set II waves (incl. Ogres); player pushes → a defensive play within 2 s; the bot discards
    hero-upgrade/Instant cards it draws (logged) and never plays them; it never plays an unaffordable card,
    never spawns on the Blue half, and the discard fee is never double-charged or charged at 0 gold.
- names: >
    ASiegeBotController (Siegebound/SiegeBotController.h/.cpp) — EvaluateDecisions Set II extension + rule-4
    hardening; log category LogSiegeBot. Uses UDeckComponent [TASK-022], economy [TASK-024/043], placement
    validity + shared Swarm spawn path [TASK-030/059], team material [TASK-044]. Spawns Team=Red BP_Unit_*/
    BP_Building_* by CardID.

### TASK-061 — DT_Cards reimport: 22-row Set II + M4 test deck (editor)
- assignee: gameplay-programmer
- status: backlog
- blocked-by: TASK-053; TASK-068 (FCardRow columns compiled)
- parallel-safe: no (editor)
- spec: >
    Editor/MCP work. Reimport Docs/Data/cards.csv into the existing /Game/Data/DT_Cards (FCardRow now carries
    the nine M4 columns). Zero import warnings. Verify all 22 rows (6 core + 16 Set II) match GDD §4 exactly,
    keyword columns present and set only where specified, and DeckCount sums to exactly 50 (the M4 test deck).
    Keep the CSV as the import source (§3.0). If MCP lacks a reimport verb (M3/TASK-031 lesson), use the same
    reference-safe in-place row rebuild that preserves the asset GUID + CSV linkage. Acceptance: GetRow on each
    of the 22 CardIDs returns exact §4 values incl. the new columns; no legacy/extra rows; DeckCount sum = 50;
    no warnings.
- names: >
    /Game/Data/DT_Cards (source Docs/Data/cards.csv, row struct FCardRow). 22 rows: Footman, Archer, Knight,
    Miner, ArrowTower, Wall + MilitiaMob, Pikeman, Sapper, Cavalry, Longbowman, Cleric, Ogre, BombTower,
    BallistaTower, Barracks, DeepMine, Masons, SharpenedBlade, PlateArmor, SwiftBoots, WarBanner.

### TASK-062 — BP_Unit_* for the 7 Set II units (editor)
- assignee: gameplay-programmer
- status: backlog
- blocked-by: TASK-061; TASK-065, TASK-066 (unit meshes); TASK-068 (unit code compiled)
- parallel-safe: no (editor)
- spec: >
    Editor/MCP work. Seven BPs in /Game/Blueprints/Units/, cloning the BP_Unit_Footman recipe (Team default
    Blue, capsule sized to mesh, VisualMesh with the -90 yaw import fix per handoffs/TASK-014.md, slot 0 =
    MI_TeamColor_Blue, NOTHING stat-like on the BP — all from DT_Cards): BP_Unit_MilitiaMob (ASummonedUnit,
    CardID MilitiaMob, SM_MilitiaMob or SM_Footman per art), BP_Unit_Pikeman (ASummonedUnit, Pikeman,
    SM_Pikeman), BP_Unit_Sapper (ASummonedUnit, Sapper, SM_Sapper), BP_Unit_Cavalry (ASummonedUnit, Cavalry,
    SM_Cavalry), BP_Unit_Longbowman (ASummonedUnit, Longbowman, SM_Longbowman or SM_Archer per art),
    BP_Unit_Cleric (ASummonedUnit, Cleric, SM_Cleric), BP_Unit_Ogre (ASummonedUnit, Ogre, SM_Ogre). Acceptance
    (SIE/PIE in L_Arena): each reports its §4 HP/speed/profile; Ogre/Sapper ignore units and path to buildings/
    castle (Siege); Cleric heals a damaged friendly and never attacks; Pikeman/Cavalry show Slayer/Charge
    2× against the right targets; Militia Mob's card yields 4 units; none hit friendlies.
- names: >
    /Game/Blueprints/Units/BP_Unit_MilitiaMob, BP_Unit_Pikeman, BP_Unit_Sapper, BP_Unit_Cavalry,
    BP_Unit_Longbowman, BP_Unit_Cleric, BP_Unit_Ogre (all ASummonedUnit, CardID = suffix). Meshes
    /Game/Meshes/SM_MilitiaMob, SM_Pikeman, SM_Sapper, SM_Cavalry, SM_Longbowman, SM_Cleric, SM_Ogre (per
    TASK-065/066; MilitiaMob/Longbowman may reuse SM_Footman/SM_Archer). Material
    /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-063 — BP_Building_* for the 4 Set II buildings (editor)
- assignee: gameplay-programmer
- status: backlog
- blocked-by: TASK-061; TASK-067 (building meshes); TASK-068 (building code compiled)
- parallel-safe: no (editor)
- spec: >
    Editor/MCP work. Four BPs in /Game/Blueprints/Buildings/, cloning the BP_Building_ArrowTower/Wall recipe
    (Team default, slot 0 MI_TeamColor_Blue, VisualMesh collision blocks Pawns + affects navigation, nothing
    stat-like on the BP): BP_Building_BombTower (ATower or its subclass per TASK-056, CardID BombTower,
    SM_BombTower), BP_Building_BallistaTower (ATower/subclass, BallistaTower, SM_BallistaTower),
    BP_Building_Barracks (ABarracks, Barracks, SM_Barracks), BP_Building_DeepMine (ADeepMine, DeepMine,
    SM_DeepMine). Match the parent class each task chose (record any subclass from TASK-056 in the handoff).
    Acceptance (PIE): Bomb Tower AoE-splashes a cluster every 2.5 s; Ballista single-targets at long range with
    a 300 blind spot; Barracks spawns a Footman every 8 s and self-destructs at 60 s; Deep Mine raises the
    owner rate +2/s on placement; all from DT_Cards.
- names: >
    /Game/Blueprints/Buildings/BP_Building_BombTower, BP_Building_BallistaTower, BP_Building_Barracks,
    BP_Building_DeepMine (CardID = suffix). Meshes /Game/Meshes/SM_BombTower, SM_BallistaTower, SM_Barracks,
    SM_DeepMine (TASK-067). Material /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-064 — HUD v4: hero-upgrade icon row + stack pips (editor)
- assignee: gameplay-programmer
- status: backlog
- blocked-by: TASK-058 (FOnHeroUpgradesChanged); TASK-068 (delegate compiled)
- parallel-safe: no (editor; shares WBP_HUD with the open M2 TASK-041 — do TASK-041 first or fold, per the M3 TASK-050 lesson)
- spec: >
    Editor/MCP work, ADDITIVE to WBP_HUD (guard the protected M1 gold Construct + M2/M3 additions — additive
    only, do NOT round-trip the Construct, per TASK-033/050). Add the §7 hero-upgrade icon row: one entry per
    active upgrade (SharpenedBlade / PlateArmor / SwiftBoots / WarBanner) with stack pips showing current/cap;
    seed from the hero's current upgrade state, THEN bind FOnHeroUpgradesChanged (seed-then-bind law).
    Placeholder styling is fine (premium icons are M7 — text/labels acceptable at blockout tier). Acceptance
    (PIE): playing an upgrade adds/updates its icon + pip within a frame; a 2-stack upgrade shows 2 pips; the
    row clears on Play Again; the M1 gold counter and M2/M3 HUD elements are unchanged.
- names: >
    /Game/UI/WBP_HUD (additive). Delegate FOnHeroUpgradesChanged [TASK-058]. Upgrade CardIDs: SharpenedBlade,
    PlateArmor, SwiftBoots, WarBanner.

### TASK-065 — Unit blockout meshes wave 1: SM_Cavalry, SM_Pikeman, SM_Sapper, SM_MilitiaMob (art)
- assignee: art-director
- status: ready-for-integration — done 2026-07-04. 4 meshes in /Game/Meshes/ (Cavalry 804tris/208u, Pikeman 532/190, Sapper 620/169, MilitiaMob 452/149), feet-center, slot0 MI_TeamColor_Blue, ≤8k tris, distinct silhouettes, ZERO import warnings. MilitiaMob = CUSTOM (not reused Footman) → TASK-062 BP_Unit_MilitiaMob VisualMesh = /Game/Meshes/SM_MilitiaMob. Editor was free (no PIE) — no playtest disruption. FBX in Content/RawAssets/. Left untracked for TASK-069 commit. handoffs/TASK-065.md.
- blocked-by: none (Blender MCP required — confirm UP before dispatch, per the M2 protocol)
- parallel-safe: yes (Blender modeling parallel with code; the editor-IMPORT step serializes with other editor-mutating tasks — one editor instance)
- spec: >
    Blender MCP + editor import. Blockout tier only (premium art DEFERRED to M7): SM_Footman-family style
    (GDD §6 — chunky 2.5–3 heads tall, oversized weapons/hands, silhouette-readable at 15 m, beveled, ≤ 8k
    tris, no textures, team material does the color). SM_Cavalry (~200 units, mounted/charging silhouette —
    reads "fast heavy"); SM_Pikeman (~185, long pike/spear — reads "anti-tank reach"); SM_Sapper (~170,
    carrying a bomb/keg — reads "expendable demolisher"); SM_MilitiaMob (~150, small ragged peasant — reads
    "weak swarm"; MAY instead reuse SM_Footman scaled if time-constrained — note the choice in the handoff so
    TASK-062 sets the BP VisualMesh accordingly). ALL: feet-center origin, SM_Footman export orientation
    (BPs apply the -90 yaw fix), capsule-friendly, slot 0 = MI_TeamColor_Blue on import, FBX to
    Content/RawAssets/<Name>.fbx, import to /Game/Meshes/. Acceptance: meshes at the named paths, ≤ 8k tris,
    distinct silhouettes at 15 m, zero import warnings, handoff records dims + tri counts.
- names: >
    /Game/Meshes/SM_Cavalry, SM_Pikeman, SM_Sapper, SM_MilitiaMob; FBX Content/RawAssets/Cavalry.fbx,
    Pikeman.fbx, Sapper.fbx, MilitiaMob.fbx; material slot 0 /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-066 — Unit blockout meshes wave 2: SM_Ogre, SM_Cleric, SM_Longbowman (art)
- assignee: art-director
- status: ready-for-integration — done 2026-07-04. SM_Ogre (1572tris, 290 tall — ROSTER's LARGEST), SM_Cleric (920, 182), SM_Longbowman (1984, 184). Feet-center, slot0 MI_TeamColor_Blue, ≤8k tris, ZERO import warnings, editor free (no disruption). Longbowman = CUSTOM (not reused Archer) → TASK-062 BP_Unit_Longbowman VisualMesh = /Game/Meshes/SM_Longbowman. FBX in Content/RawAssets/. Left untracked for TASK-069. handoffs/TASK-066.md.
- blocked-by: none (Blender MCP required)
- parallel-safe: yes (see TASK-065 note)
- spec: >
    Blender MCP + editor import. Blockout tier (premium art DEFERRED to M7), SM_Footman-family style, ≤ 8k
    tris, no textures: SM_Ogre (~280 units, massive hulking siege brute + club — the win-condition card, must
    read HUGE next to a Footman); SM_Cleric (~180, robed with a staff/holy symbol — reads "support/healer",
    NOT a fighter); SM_Longbowman (~185, tall longbow — reads "long-range archer"; MAY reuse SM_Archer if
    time-constrained — note the choice for TASK-062's VisualMesh). ALL: feet-center origin, SM_Footman export
    orientation, capsule-friendly, slot 0 = MI_TeamColor_Blue, FBX to Content/RawAssets/, import to
    /Game/Meshes/. Work priority: Ogre first (it anchors the slice). Acceptance: meshes at the named paths,
    ≤ 8k tris, distinct silhouettes at 15 m (Ogre clearly the largest unit in the roster), zero import
    warnings, handoff records dims + tri counts.
- names: >
    /Game/Meshes/SM_Ogre, SM_Cleric, SM_Longbowman; FBX Content/RawAssets/Ogre.fbx, Cleric.fbx,
    Longbowman.fbx; material slot 0 /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-067 — Building blockout meshes: SM_BombTower, SM_BallistaTower, SM_Barracks, SM_DeepMine (art)
- assignee: art-director
- status: ready-for-integration — done 2026-07-04. SM_BombTower (580tris/250×250×451), SM_BallistaTower (512/250×270×500), SM_Barracks (312/400×419×349), SM_DeepMine (1064/300×305×300). Ground-center, slot0 MI_TeamColor_Blue, ≤15k tris, TIGHT authored UCX = base footprint (overhangs outside hull, plinth-lesson honored), distinct silhouettes (DeepMine headframe ≠ GoldNode crystal), ZERO import warnings, editor free. ALL M4 ART DONE (065/066/067 = 11 meshes). FBX in Content/RawAssets/. Left untracked for TASK-069. handoffs/TASK-067.md.
- blocked-by: none (Blender MCP required)
- parallel-safe: yes (see TASK-065 note)
- spec: >
    Blender MCP + editor import. Four structure blockouts (GDD §6, ≤ 15k tris each, beveled, blockout tier,
    no textures; UCX hulls kept tight to the footprint — the M1 castle-plinth lesson: oversized collision =
    placement dead zones), ground-center origin, slot 0 = MI_TeamColor_Blue: SM_BombTower (~250×250 footprint,
    ~450 tall, mortar/cauldron top — reads "lobs bombs"); SM_BallistaTower (~250×250, ~500 tall, big
    horizontal bolt-thrower — reads "long-range sniper"); SM_Barracks (~400×400 footprint, ~350 tall, tent/
    hall with a doorway — reads "spawns troops"); SM_DeepMine (~300×300, ~300 tall, mine-shaft/ore-cart — reads
    "economy building", visually distinct from the SM_GoldNode ore cluster). FBX to Content/RawAssets/, import
    to /Game/Meshes/. Acceptance: four meshes at the named paths with stated dims (±10%), tight UCX collision
    verified, distinct silhouettes, zero import warnings, handoff records dims/tris/collision per mesh.
- names: >
    /Game/Meshes/SM_BombTower, SM_BallistaTower, SM_Barracks, SM_DeepMine; FBX Content/RawAssets/BombTower.fbx,
    BallistaTower.fbx, Barracks.fbx, DeepMine.fbx; material slot 0 /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-068 — M4 code batch: compile + residue adjudication + commit (build)
- assignee: build-master
- status: in-progress (2026-07-04; all TASK-053..060 qa-passed; M4 batch compile on main)
- blocked-by: TASK-053, TASK-054, TASK-055, TASK-056, TASK-057, TASK-058, TASK-059, TASK-060 (all qa-passed)
- parallel-safe: no
- spec: >
    Build-master. Compile the accumulated M4 C++ batch (TASK-053..060) with the standard Build.bat command;
    editor-bounce protocol (editor must release the DLL). **Pre-compile: scan the batch for inherited-reflected-
    member shadows (CONVENTIONS coding law — cost 2 loops in M2, held clean in M3).** Any compile error →
    append to the failing task's QA report, set qa-failed, stop (counts as a QA loop; build-master never edits
    code). Adjudicate any working-tree residue. Commit the M4 code batch + Docs/Data/cards.csv + pipeline docs
    (TASKBOARD/CONVENTIONS/SLACK deltas) with task IDs. Do NOT push. Acceptance: clean build; git status clean
    of unexplained residue; commit hash on the board + posted in 🔧 Build & Git.
- names: >
    Build command per CLAUDE.md. Commit pattern: "TASK-053..060: M4 Set II C++ batch (keywords, Siege/Support
    profiles, new towers/buildings, hero upgrades, play v3, bot v2)".

### TASK-069 — M4 final assembly: expanded-roster PIE verification + commit (build)
- assignee: build-master
- status: backlog
- blocked-by: TASK-061, TASK-062, TASK-063, TASK-064 (done); TASK-068 (committed)
- parallel-safe: no
- spec: >
    Build-master. Final M4 integration in editor + Git. (1) Verify all M4 assets resolve with zero load
    warnings (DT_Cards 22 rows, 7 unit BPs, 4 building BPs, HUD upgrade row). (2) PIE-verify the §9-4 slice +
    §4 acceptance: keywords fire (Cavalry Charge 2×, Pikeman Slayer 2×, Militia Mob spawns 4, Sapper suicide
    AoE, Ogre/Sapper Siege 200% vs castle/buildings); Support Cleric heals; new towers behave (Bomb AoE,
    Ballista blind spot); Barracks spawns + expires; Deep Mine +2/s; hero upgrades apply/stack/cap/refund +
    persist through death + reset on Play Again; the bot plays Set II (incl. an Ogre push) and discards
    upgrades/Instants; the M4 test deck deals 50/6 with Set II reachable. Record pass/fail per line; fails route
    back per the QA loop. (3) Confirm the slice is RECORDABLE (Ogre push vs Bomb Tower defense clip). (4)
    Cold-boot reconfirm the M3 transient-Blue-unit WATCH. (5) Commit all M4 editor/art assets with task IDs;
    hash on the board; post in 🔧 Build & Git. Do NOT push. Flag the M4 checkpoint ready for Jonathan's
    playtest.
- names: >
    /Game/Maps/L_Arena, /Game/Data/DT_Cards, /Game/Blueprints/Units/BP_Unit_* (7), /Game/Blueprints/Buildings/
    BP_Building_* (4), /Game/UI/WBP_HUD. Handoff: .claude/pipeline/handoffs/TASK-069.md.

---

## Done

### TASK-001 — Card data types, team types & cards.csv — done (commit 5029403)
- gameplay-programmer. TeamId.h (ETeamId, ITeamAgent), CardRow.h (FCardRow + enums), Docs/Data/cards.csv
  (Footman row). Handoff: handoffs/TASK-001.md; QA: qa/TASK-001-report.md.

### TASK-002 — Castle actor (C++) — done (commit 5029403)
- gameplay-programmer. ACastle (Siegebound/Castle.h/.cpp): 2000 HP, team, no friendly fire,
  FOnCastleDestroyed, ResetCastle. Handoff: handoffs/TASK-002.md; QA: qa/TASK-002-report.md.

### TASK-003 — Hero character (C++) — done (commit 5029403)
- gameplay-programmer. AHeroCharacter (Siegebound/HeroCharacter.h/.cpp): 500/750 movement, 20-dmg cone
  melee 0.5 s, 200 HP, regen, death notify. Handoff: handoffs/TASK-003.md; QA: qa/TASK-003-report.md
  (warning 2 = melee-suppression exit-path law, still binding).

### TASK-004 — Summoned unit AI, Standard profile (C++) — done (commit 5029403)
- gameplay-programmer. ASummonedUnit (Siegebound/SummonedUnit.h/.cpp): DT_Cards-driven stats,
  Advance→Acquire→Attack→Reacquire, aggro 600 / leash 900. Handoff: handoffs/TASK-004.md;
  QA: qa/TASK-004-report.md.

### TASK-005 — Gold economy on PlayerState (C++) — done (commit 5029403)
- gameplay-programmer. ASiegePlayerState: gold 50 start, +2/s, cap 999, CanAfford/SpendGold/ResetGold,
  FOnGoldChanged. Handoff: handoffs/TASK-005.md; QA: qa/TASK-005-report.md (major 1 timer-order + major 2
  seed-then-bind — both still binding laws).

### TASK-006 — GameMode: win condition, hero respawn, Play Again reset (C++) — done (commit 7011b7b)
- gameplay-programmer. ASiegeGameMode: castle-destroyed → match end, 5 s hero respawn, PlayAgain full
  reset, DefaultEngine.ini map/gamemode config. Handoff: handoffs/TASK-006.md; QA: qa/TASK-006-report.md
  (re-review loop 1 passed; match-end unit/income freeze recorded as TODO(M2) → TASK-024/028).

### TASK-007 — PlayerController: Footman card play + placement mode (C++) — done (commit 7011b7b)
- gameplay-programmer. ASiegePlayerController: EnterPlacementMode/ExitPlacementMode/HandleMatchEnd, ghost
  preview, Blue-half validity, DT-driven cost. Handoff: handoffs/TASK-007.md; QA: qa/TASK-007-report.md
  (0 blockers, 4 warnings incl. WARN-4 card-button clickability → TASK-023/033; navmesh projection
  TODO → TASK-030).

### TASK-008 — Import DT_Cards data table (editor) — done (commit 7011b7b)
- gameplay-programmer. /Game/Data/DT_Cards from Docs/Data/cards.csv (FCardRow, row Footman), zero
  warnings. Handoff: handoffs/TASK-008.md.

### TASK-009 — Input assets + BP_HeroCharacter (editor) — done (commit 7011b7b)
- gameplay-programmer. /Game/Input/Actions/IA_Sprint, IA_Attack, IA_Card1, IA_CancelPlace;
  /Game/Input/IMC_Hero; /Game/Blueprints/BP_HeroCharacter (SKM_Quinn_Simple + ABP_Unarmed; template
  slots wired per handoffs/TASK-003.md). Handoff: handoffs/TASK-009.md.

### TASK-010 — BP_Unit_Footman (editor) — done (commit 7011b7b)
- gameplay-programmer. /Game/Blueprints/Units/BP_Unit_Footman (ASummonedUnit, CardID Footman,
  SM_Footman + MI_TeamColor_Blue, -90 yaw VisualMesh fix). Handoff: handoffs/TASK-010.md.

### TASK-011 — HUD + Victory widgets (editor) — done (M1 final commit 4255c1d)
- gameplay-programmer. /Game/UI/WBP_HUD (live gold counter, Footman card button), /Game/UI/
  WBP_VictoryScreen (Play Again; SetWinner byte-param deviation, orchestrator-approved). Handoff:
  handoffs/TASK-011.md.

### TASK-012 — Team-color + ghost materials (art)
- assignee: art-director
- status: done (commit 4d30efb; scaffolding commit df4bcd9)
- summary: /Game/Materials/M_TeamColor (param TeamColor) + MI_TeamColor_Blue/_Red + M_Ghost (param
    GhostColor, bonus scalar GhostOpacity). Handoff: handoffs/TASK-012.md.

### TASK-013 — Castle blockout mesh (art)
- assignee: art-director
- status: done (commit 8a87400)
- summary: /Game/Meshes/SM_Castle — 2414 tris, 814x820x900 units, 9 UCX hulls, slot 0 = MI_TeamColor_Blue.
    FBX source Content/RawAssets/Castle.fbx. Handoff: handoffs/TASK-013.md.
    NOTE for final assembly: give Castle_Red yaw 180 so the gates face each other.

### TASK-014 — Footman blockout mesh (art)
- assignee: art-director
- status: done (commit 8a87400)
- summary: /Game/Meshes/SM_Footman — 2152 tris, 180 units tall, feet-center origin, capsule collision,
    slot 0 = MI_TeamColor_Blue. FBX source Content/RawAssets/Footman.fbx. Handoff: handoffs/TASK-014.md
    (VisualMesh needs -90° yaw on the BP per the handoff's facing note).

### TASK-015 — L_Arena blockout level (art)
- assignee: art-director
- status: done (commit 8a87400)
- summary: /Game/Maps/L_Arena — ground 6400x3200 top at Z=0, CastleAnchor_Blue (-2000,0,0),
    CastleAnchor_Red (+2000,0,0), PlayerStart (-1700,0,100) yaw 0, DecalActor centerline, navmesh both
    halves, PIE smoke-tested. Support asset /Game/Materials/M_CenterlineStripe. Handoff: handoffs/TASK-015.md.
    (PlayerStart later moved to (-1400,0,100) at M1 final assembly — see M1 checkpoint carry-overs.)

### TASK-016 — Hero attack feedback hooks (C++) — done (commit f6fa7ba)
- gameplay-programmer. AHeroCharacter "Combat|Feedback" UPROPERTYs (AttackMontage/AttackMontageSection,
  HitImpactEffect, HitCameraShake); null-safe montage + impact VFX + camera shake per swing, damage
  timing byte-identical to M1; Niagara module added to Build.cs. Handoff: handoffs/TASK-016.md;
  QA: qa/TASK-016-report.md.

### TASK-017 — Wire hero attack feedback + LMB real-input verification (editor) — done (commit 4f95730)
- gameplay-programmer. BP_HeroCharacter wired: AM_ComboAttack section Melee01 on ABP_Unarmed, NS_Damage
  impact, BP_CameraShake_Hit_Enemy; PIE-verified −20 HP/swing; no click-swallow found — real-hardware
  LMB confirmed closed at round-2 sign-off (2026-07-03). Handoff: handoffs/TASK-017.md.

### TASK-018 — Castle HP delegate + health-bar widget component (C++) — done (commit f6fa7ba)
- gameplay-programmer. ACastle FOnCastleHPChanged (broadcast on real HP change/reset/BeginPlay seed) +
  screen-space HPBarWidget component + GetCurrentHP/GetMaxHP; UCastleHealthBarWidget base
  (InitForCastle seed-then-bind, OnHPChanged BIE). Handoff: handoffs/TASK-018.md;
  QA: qa/TASK-018-report.md.

### TASK-019 — WBP_CastleHealthBar from UI_LifeBar donor (editor) — done (commit 4f95730)
- gameplay-programmer. /Game/UI/WBP_CastleHealthBar (UI_LifeBar duplicate reparented to
  UCastleHealthBarWidget); all 6 PIE acceptance items passed; donor restored to HEAD; build-master
  reset-and-restaged a stale staged widget blob — the committed blob is the audited SHA. Handoff:
  handoffs/TASK-019.md.

### TASK-020 — Footman procedural attack lunge + impact VFX (C++) — done (commit f6fa7ba)
- gameplay-programmer. ASummonedUnit sine-eased VisualMesh lunge per cadence hit
  (AttackLungeDistance/Duration, drift-free base restore) + NS_Damage impact puff per damage
  application; Build.cs untouched (TASK-016 owns Niagara). Handoff: handoffs/TASK-020.md;
  QA: qa/TASK-020-report.md.
