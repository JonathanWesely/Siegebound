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
4. **M4 — Card Set II (16 cards, keywords, hero upgrades)** — `done (playtested + signed off by Jonathan 2026-07-08; committed 65861ce + e586699, not pushed; m4-testable @ e586699)` (TASK-053..069; branches m2-testable @ f903cf0 + m3-testable @ 56247c9 + m4-testable @ e586699 preserve the milestone slices; see "## M4 tasks") — sign-off note: "we will have to make some balancing changes later, but it is fine" → see Standing backlog
4.5. **M4.5 — Gameplay terrain pass (grass, hills, trees + rocks)** — `parked — awaiting Jonathan's Fab asset drop (plan + folders ready; resume on his return)` — Jonathan directive 2026-07-08: the arena is "a boring white board"; wants "a nice large grass area with trees and hills". REAL GAMEPLAY TERRAIN — mirror-symmetric hills (physical high ground) + trees/rocks (navmesh/placement obstacles) INSIDE the playfield; amends GDD §5 (rulings in "M4.5 manager decisions" under Active tasks, incl. the same-day Fab amendment). **Fab pivot (Jonathan, same day):** he supplies premade Fab assets for FOUR slots — tree, rock, grass, hill (FAB-001..004 approved in .claude/pipeline/fab/FAB-REQUESTS.md; drop zone Content/Fab/README_DROP_ZONE.md). TASK-091/092 are now Fab conform+integration tasks blocked on his drop; TASK-093/094 (C++) remain valid and dispatchable. **ORDERING INVERSION (Jonathan's ruling): M5 proceeds AHEAD of this milestone — nobody blocks M5 work on M4.5.** M4.5 resumes the moment the Fab assets land.
5. **M5 — Spell system + Set III** — `current (decomposed 2026-07-08, TASK-097..109; runs AHEAD of parked M4.5 per Jonathan's directive)` — targeting mode, 5 spells + Crystal Tower, spell Niagara VFX at the §6 bar, bot M5 spell rules. Carry-in baked into the specs (not a follow-up): the targeting reticle ground-projects via TRACE so M4.5's hills need no rework when they land. Slice: spell VFX showcase reel.
6. M6 — Deck-builder meta — `not-started`
7. M7 — Premium art & feel pass — `not-started` · **Jonathan request (2026-07-04):** raise fidelity on SM_Castle + SM_Footman + SM_Archer (higher detail than the current blockouts); wants the game to look nicer. Decision: DEFERRED here (mesh swaps are non-breaking; roster still growing through M4-M6). Two integration paths to scope at M7: (a) art-director custom higher-detail Blender models, and/or (b) **Fab/UE-marketplace assets — Jonathan must download packs into the project via the Epic Launcher first (agents can't browse/buy/download Fab autonomously); art-director then swaps meshes/materials.** Could be pulled forward as a standalone art pass after M3/M4 if Jonathan wants it sooner.
8. M8 — Networked 1v1 multiplayer — `not-started`

### Standing backlog (manager notes — NOT tasks, no IDs yet)
- **Balance pass** — Jonathan flagged balancing changes wanted post-M4 (M4 playtest sign-off 2026-07-08: "we will have to make some balancing changes later, but it is fine"); awaiting his specific notes before task-izing. Feed-ins already on file for when the notes arrive: TASK-090 balance ledger (undefended-castle kill time ~56.5 s / ~71.3 s post-economy-change vs ~33 s prior; bot played ZERO early Miners in both rush matches — bot spend-mix), TASK-070 tuning note (bot opens with attack, not economy).
- **HUD overtime indicator never shows** (pre-existing bug found at TASK-090, routed to manager): WBP_HUD ShowOvertime calls UpdateOvertimeDisplay with a hardcoded-false pin (bound via SetupStatTexts CreateEvent; UpdateOvertimeDisplay itself is correct). One-pin UMG fix + shortened-threshold verify — fold into the next UMG-touching chain or the balance pass; do not lose it.

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

**M1/M2/M3/M4 all complete + committed on `main` (nothing pushed).** M4 wrapped 2026-07-04 — code **65861ce** (via TASK-068) + editor/art **e586699** (via TASK-069); milestone preserved on **m4-testable @ e586699** (the full-game superset M2+M3+M4). **M5 (Spell system + Set III) is NOT started** — Jonathan authorized only through M4; awaiting his go-ahead + M2/M3/M4 playtest feedback before decomposing M5.

**Current state (2026-07-05):** on `main` @ **5c1fcb7**, clean tree, NOT pushed. **TASK-070** (L_Arena stray-actor cleanup, a745799) and **TASK-041** (visual hand UI, 5c1fcb7) are BOTH `done` + committed — the one known M2 gap (visual hand) is CLOSED and the M3 transient-Blue-unit WATCH is CLOSED. Branches m2/m3/m4-testable preserved. Old carry-forwards resolved: (a) M2 TASK-041 visual hand UI — DONE (5c1fcb7); (b) M3 transient-Blue-unit WATCH — CLOSED by TASK-070; (c) TASK-046 WARN-2 bot discard hardening — CLOSED in TASK-060.

**2026-07-07 (BUG — read first):** Jonathan's M4 playtest is BLOCKED — joining a match from L_MainMenu via EITHER menu button gives ZERO input in L_Arena. Bugfix chain **TASK-074..076** below. This is a bugfix chain like TASK-071..073, NOT M5 content — **M5 remains NOT authorized.** → **RESOLVED same day:** TASK-074..076 done + committed **218b4c9**; Jonathan live-confirmed the fix ("that problem is resolved"); both menu-path WATCHes closed.

**2026-07-07 (FEATURES — read first):** TWO Jonathan-approved chains issued below — **TASK-077..081 (card artwork on the hand UI)** and **TASK-082..088 (TRELLIS.2 → Blender → UE5 automated art pipeline + Fab lane)**. Both are Jonathan-authorized UI/art/tooling work like TASK-071..073, NOT M5 content — **M5 remains NOT authorized.** State at issue: main @ **218b4c9** clean, NOT pushed; editor UP (PID 18480, MCP healthy); Blender MCP verified LIVE — both art gates OPEN. Dispatch frontier: **TASK-077 ∥ TASK-079 ∥ TASK-082 ∥ TASK-083** (all file-side, mutually parallel-safe). → **BOTH CHAINS COMPLETE:** card-art committed **61bd457** (TASK-077..081 done 2026-07-08); Trellis pilot committed **cb29882** (2026-07-08, TASK-082..088 all done — SM_Footman/SM_Archer/SM_Castle are now textured pipeline meshes at unchanged paths; same-path swap mechanism = one human Content-Browser Reimport click per mesh until an MCP console-exec/reimport tool exists). Jonathan's visual sign-off received 2026-07-08 ("the trellis pilot was successful", zero findings) — WATCH CLOSED; the pipeline IS the M7 template for the remaining 16 meshes. **M5 remains NOT authorized.**

**2026-07-08 (BALANCE — read first):** Jonathan URGENT balance directive, chain **TASK-089..090** below. This is a Jonathan-directed standalone balance chain like TASK-074..076, NOT M5 content — **M5 remains NOT authorized.** State at issue: main @ cb29882; known worktree residue (WBP_MainMenu + BP_Unit_Footman + BP_Unit_Archer .uassets) is adjudicated inside TASK-090's bounce window per the TASK-088 residue note.

**2026-07-08 LATER (FAB PIVOT + M5 AUTHORIZED — read first, supersedes the paragraph below):** Right after the M4.5 decomposition landed, Jonathan directed (verbatim): "create a folder that expects a tree asset, rock asset, grass asset, and hill asset. I am going to use some premade assets from Fab and insert them where you need them. After that, have the agents move on to M5." Consequences, all live below: (1) **M4.5 is PARKED** awaiting his Fab drop — FAB-001..004 pre-approved in .claude/pipeline/fab/FAB-REQUESTS.md, drop zone Content/Fab/README_DROP_ZONE.md; TASK-091/092 are now Fab CONFORM tasks blocked on the drop; **rock is added M4.5 scope** (same law as trees); the code contracts now read tag `Obstacle` (trees + rocks). (2) **M4.5's TASK-093/094 (C++) stay dispatchable NOW** — they read tags, not meshes; they ride M5's compile batch (TASK-103). (3) **M5 IS AUTHORIZED and decomposed — TASK-097..109 below; M5 runs AHEAD of parked M4.5 (ordering inversion is Jonathan's explicit ruling — nobody blocks M5 on M4.5).** Jonathan is away for a few hours and pre-authorized everything, editor work included; the editor-MCP-up hard gate still applies.

**2026-07-08 (M4 SIGN-OFF + M4.5 MILESTONE):** Jonathan playtested M4 and SIGNED OFF ("it is fine"; balance changes wanted later → Standing backlog, no notes yet). He then directed a NEW MILESTONE inserted before M5: **M4.5 — Gameplay terrain pass** (TASK-091..096 below, own rulings block) — real gameplay terrain (symmetric hills = physical high ground, trees = obstacles) replacing the flat white board; amends GDD §5. State at issue: main @ HEAD post-TASK-090, clean-ish tree (bounce-window residue adjudicated in TASK-090), NOT pushed. ~~M5 remains NOT authorized~~ → SUPERSEDED same day by the Fab-pivot paragraph above: M5 authorized + decomposed.

### M5 — Spell system + Set III (TASK-097..109) — decomposed 2026-07-08

**Authorization:** Jonathan's 2026-07-08 directive ("After that, have the agents move on to M5") — pre-authorized while he is away, editor/MCP work included. Hard gate stands: if the editor MCP (127.0.0.1:8000) is unreachable, park the task and tell the orchestrator — never fake results. Naming law added to CONVENTIONS.md "Spells & Set III (M5)" BEFORE task issue.

**M5 manager decisions (binding for all M5 tasks):**
1. **Resolver home:** `USpellLibrary` (UBlueprintFunctionLibrary, SpellLibrary.h/.cpp) — the ONE spell-resolution path, shared by player controller (targeting mode) and bot. Pinned entry: `static bool ResolveSpell(UWorld* World, FName CardID, const FCardRow& Row, ETeamId CasterTeam, const FVector& TargetPoint)`. Effects dispatch on the `ESpellEffect` column.
2. **Data columns** per CONVENTIONS "Spells & Set III (M5)": SpellEffect / EffectDuration / MaxTargets / GoldSteal / ChainTargets / ChainFalloff; spells REUSE Damage + AoERadius. ECardType gains `Spell` if absent (TASK-097 flags the current enum shape + every downstream switch it touches).
3. **Spell castle scaling:** `USiegeDamageType_Spell` = 50% in `ACastle::TakeDamage` ONLY; buildings take FULL spell damage — Lightning (200) must kill an Arrow Tower (150 HP), it is the §4 "tower-killer". Mirrors the M2 Projectile precedent, NOT the M4 Siege both-rule. QA watch item.
4. **Lightning selection:** the MaxTargets highest **CURRENT-HP** enemy actors (units, hero, buildings/towers — **castle EXCLUDED**, anti-sniping intent) within AoERadius of the reticle; ties broken by distance to the reticle (deterministic). Flagged: current HP, not MaxHP (Slayer uses MaxHP — different concept).
5. **Freeze (FrostNova):** pinned API `ApplyFreeze(float Seconds)` on ASummonedUnit and ABuilding (+ `IsFrozen()`); freeze pauses movement/AI/attack cadence; refresh-not-stack (max of remaining vs new); castle never freezable; **hero NOT freezable in M5** (GDD §4 says "enemy units and towers" — flagged); **match-end FreezeAI has PRECEDENCE** — a spell-freeze expiry must never resume a match-end-frozen actor.
6. **Battle Cry (AllyBuff):** friendly units in AoERadius get +50% attack speed +25% move speed for EffectDuration (8 s); magnitudes = mechanic-rule UPROPERTYs (Rally precedent, GDD §4 comments). Self-refresh non-stacking; DOES stack with Rally and War Banner (independent systems) — QA watch.
7. **Pickpocket (GoldSteal):** resolves INSTANTLY on play — no reticle for a global effect (recorded deviation from §3.5; CardType stays Spell). Steal = min(GoldSteal, victim's gold), moved ONLY via existing ASiegePlayerState gold APIs (SetGold choke-point law; TASK-098 documents the exact API composition).
8. **Targeting mode (§3.5/§7):** reticle ANYWHERE on the map — no half restriction, no navmesh requirement; reticle TRACE-projected onto the surface under the cursor (**M4.5 terrain carry-in baked as LAW** — never assume the Z=0 plane); LMB confirm = deduct-then-resolve (card-leaves-hand-at-CONFIRM law; resolver false ⇒ full refund + HUD reason); RMB/Esc cancel free; reticle visual = decal w/ soft-referenced M_SpellReticle, null-safe. Cursor posture mirrors placement mode.
9. **Chain (Crystal Tower):** INSTANT-hit, no projectile actor. Primary = nearest valid enemy in Range (800); bounces to ChainTargets−1 more enemies, each within `ChainBounceRadius` = 350 (mechanic UPROPERTY — GDD unspecified, manager-defined) of the PREVIOUS target; damage Dmg − n×ChainFalloff (15/10/5); no friendly fire; no double-hit per zap; tagged USiegeDamageType_Projectile (tower attack family). Visual NS_ChainZap, null-safe.
10. **Bot spell rules** insert as rule 3 in the 2 s ordered loop (defend=1, miners=2, **SPELLS=3**, big unit=4, discard=5): 3a Fireball at ≥3 clustered player units (300-radius cluster); 3b Lightning at a player tower with ≥2 player units within 400. Affordability + card-in-hand checks as always; resolve directly through USpellLibrary (targeting mode is a human affordance); one LogSiegeBot line per fired rule (law). FrostNova/BattleCry/Pickpocket get NO bot cast rule (GDD is silent) — they fall through to rule-5 discard economics; flagged.
11. **VFX contract:** every resolve spawns `/Game/VFX/NS_Spell_<CardID>` (soft path composed from CardID, null-safe, log-once). Donor-duplicate authoring from Variant_Combat Niagara is acceptable (template-donor law); the M5 bar is §6 one-frame readability + showcase-able — fully custom premium sims may roll to M7 (flag anything below bar).
12. **Card art** for the 6 new CardIDs follows the existing law (T_CardArt_<CardID>, 512², /Game/UI/CardArt/): paths are deterministic, so TASK-097 writes the CardArt CSV cells UP FRONT; missing textures fall back gracefully (text-only face) until TASK-106 lands.
13. **M5 test deck:** DeckCount rebalances so every Set III card is reachable — suggested Fireball 2 / FrostNova 1 / Lightning 2 / BattleCry 1 / Pickpocket 1 / CrystalTower 1 (= 8 slots carved from the M4 22-card spread); sum EXACTLY 50, each ≤ MaxCopies; programmer documents the cuts, QA re-sums.
14. **File-conflict serialization:** TASK-100 (targeting mode) shares SiegePlayerController.h/.cpp with M4.5's TASK-093 (placement v3, dispatchable now) — 093 goes FIRST, 100 is `blocked-by: TASK-093` (file serialization, not logic). Tower.cpp: the tower freeze-gate is implemented in TASK-101 against 099's pinned IsFrozen() API so 099 stays OUT of Tower.cpp — 099 ∥ 101 parallel-safe.

**M5 exit criteria (playable slice):** all 5 spells playable end-to-end with visible VFX; Fireball meets §3.11 acceptance (kills 3 clustered 80-HP Footmen, adjacent friendly untouched, 50 not 100 vs castle); FrostNova freezes enemy units + a tower 4 s (castle unaffected), they resume cleanly; Lightning kills an Arrow Tower picking the 3 highest-current-HP enemies in 400; BattleCry visibly speeds attack + movement 8 s; Pickpocket moves exactly min(10, victim gold); Crystal Tower chains 15/10/5 within 800; reticle projects onto the ground surface, LMB confirms (gold at confirm), RMB/Esc cancels free; bot casts Fireball + Lightning per its rules with LogSiegeBot traces; deck = 50 with Set III reachable; Play Again clears all spell state (freeze/buff timers). Recordable: spell VFX showcase reel.

Dispatch shape: **FILE WAVE NOW (parallel): TASK-097 ∥ 098 ∥ 099 ∥ 101 ∥ 102, alongside M4.5's TASK-093 ∥ 094** (seven file tasks, distinct file sets); TASK-100 after 093 (ruling 14). ART when Blender/editor free: TASK-105 ∥ 106 (serialize imports); TASK-108 when the editor is free. QA gates every code task (shadow-scan mandatory). Then TASK-103 (batch compile + commit, folds in 093/094) → TASK-104 + TASK-107 (editor wave) → TASK-109 (final assembly + PIE + commit + m5-testable).

#### TASK-097 — Card data Set III: ESpellEffect + M5 columns + cards.csv 28 rows + M5 test deck (files)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-08; qa/TASK-097-report.md — PASS, 0 blockers / 1 warn / 2 nits, all 7 flagged decisions ACCEPTED. WARN = CrystalTower dead-card window between TASK-104 reimport and TASK-107 BP — orchestrator ruling: covered, no PIE runs between 104 and 107 [109 is the only PIE task and sits behind both]. Awaits TASK-103 batch compile)
- blocked-by: none
- parallel-safe: yes (CardRow.h/.cpp + Docs/Data/cards.csv only)
- spec: >
    Files only. (1) CardRow.h: UENUM ESpellEffect { None, AoEDamage, Freeze, TopTargetsDamage, AllyBuff,
    GoldSteal }; NEW FCardRow UPROPERTY columns per the CONVENTIONS registry (CSV headers 1:1):
    SpellEffect (ESpellEffect, None), EffectDuration (float, 0), MaxTargets (int32, 0), GoldSteal
    (int32, 0), ChainTargets (int32, 0), ChainFalloff (int32, 0). Add Spell to ECardType if absent —
    flag the current enum shape and EVERY downstream switch touched (ruling 2). (2) cards.csv: 6 Set III
    rows per GDD §4 (row names character-for-character: Fireball, FrostNova, Lightning, BattleCry,
    Pickpocket, CrystalTower): costs 7/6/8/5/6/9, MaxCopies 3/3/2/3/2/3; Fireball Damage 100 AoERadius
    300 SpellEffect AoEDamage; FrostNova AoERadius 350 EffectDuration 4 SpellEffect Freeze; Lightning
    Damage 200 AoERadius 400 MaxTargets 3 SpellEffect TopTargetsDamage; BattleCry AoERadius 400
    EffectDuration 8 SpellEffect AllyBuff; Pickpocket GoldSteal 10 SpellEffect GoldSteal; CrystalTower =
    Building row: HP 150, Dmg 15, Range 800, Cadence 1.5, ChainTargets 3, ChainFalloff 5, bRanged FALSE
    (chain is instant-hit, ruling 9). (3) CardArt cells for all 6 with the deterministic
    /Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID> paths (ruling 12 — graceful fallback until
    TASK-106). (4) M5 test deck per ruling 13: DeckCount sum EXACTLY 50, each ≤ MaxCopies, Set III all
    ≥1; document the cuts from the M4 spread in the handoff. (5) Stats live in the table — nothing
    hardcoded; shadow law. Acceptance (by inspection): header/UPROPERTY 1:1; 28 rows; sums verified.
    handoffs/TASK-097.md. Post in ⚙️ Dev & QA.
- names: >
    FCardRow + ESpellEffect (Source/GitClaudeUnrealTest/Siegebound/CardRow.h/.cpp); Docs/Data/cards.csv;
    CardIDs Fireball, FrostNova, Lightning, BattleCry, Pickpocket, CrystalTower; columns SpellEffect,
    EffectDuration, MaxTargets, GoldSteal, ChainTargets, ChainFalloff (+ reused Damage, AoERadius,
    DeckCount, CardArt). Law: CONVENTIONS "Spells & Set III (M5)" + "Data-driven card stats".

#### TASK-098 — USpellLibrary resolver + USiegeDamageType_Spell + castle 50% spell scaling (files)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-08; qa/TASK-098-report.md — fix loop 1 verified PASS, 0 blockers remaining, 0 new findings. BattleCry seam closed [per-unit accessor composition char-identical to the SummonedUnit.h pin; editor tunables live]. TASK-100's in-tree ResolveSpell call sites noted — covered by TASK-100's own QA pass, already dispatched. Awaits TASK-103 batch compile)
- blocked-by: none
- parallel-safe: yes (NEW SpellLibrary.h/.cpp + DamageTypes.h/.cpp + Castle.cpp; TASK-099's APIs are pinned by spec — file-independent, compile happens at TASK-103 anyway)
- spec: >
    Files only. (1) NEW USpellLibrary (UBlueprintFunctionLibrary) in SpellLibrary.h/.cpp; pinned entry:
    static bool ResolveSpell(UWorld* World, FName CardID, const FCardRow& Row, ETeamId CasterTeam,
    const FVector& TargetPoint). Dispatch on Row.SpellEffect: AoEDamage = radial Damage in AoERadius at
    TargetPoint via the TASK-055 AoE radial helper (REUSE it; no friendly fire; castle hits flow through
    TakeDamage with the Spell type); Freeze = ApplyFreeze(EffectDuration) on enemy units + buildings in
    radius (TASK-099 pinned API; castle + hero excluded, ruling 5); TopTargetsDamage = ruling-4
    selection (MaxTargets highest CURRENT HP, castle excluded, tie = nearest reticle), Damage each;
    AllyBuff = TASK-099's combat-buff API on friendly units in radius (ruling 6); GoldSteal = ruling 7
    via ASiegePlayerState gold APIs + GetPlayerStateForTeam (TASK-043). (2) EVERY resolve spawns
    /Game/VFX/NS_Spell_<CardID> — soft path composed from CardID, null-safe, log-once (ruling 11).
    (3) NEW USiegeDamageType_Spell in DamageTypes.h/.cpp; ACastle::TakeDamage gains the Spell = 50%
    branch (ruling 3 — Castle ONLY; ABuilding.cpp NOT touched). (4) All spell damage tagged Spell;
    no friendly fire; null-safe everywhere (bad row/world/state = return false + log, never crash).
    (5) Shadow law. Acceptance (by inspection): five effects per rulings 3-7; VFX contract; bool
    refusal semantics documented (false ⇒ caller refunds). handoffs/TASK-098.md with flagged decisions
    (gold-steal API composition, AoE helper reuse shape). Post in ⚙️ Dev & QA.
- names: >
    USpellLibrary::ResolveSpell (Source/.../SpellLibrary.h/.cpp NEW); USiegeDamageType_Spell
    (DamageTypes.h/.cpp); ACastle::TakeDamage (Castle.cpp). Pinned externals (TASK-099):
    ASummonedUnit::ApplyFreeze/IsFrozen/ApplyCombatBuff, ABuilding::ApplyFreeze/IsFrozen. VFX soft path
    /Game/VFX/NS_Spell_<CardID>. Law: M5 rulings 1-7, 11.

#### TASK-099 — Freeze + combat-buff APIs on units & buildings (files)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-08; qa/TASK-099-report.md — PASS, 0 blockers / 2 warns / 3 nits, all 10 flagged decisions ACCEPTED. Match-end precedence triple guard verified, no hole. Seam ruling concurs with qa/TASK-098: 098 moves [fix loop 1 in flight implements the exact specified composition]. Carries: TASK-100 QA must verify targeting refuses casts at match end; stale comments SiegeGameMode.cpp:273 + Building.cpp:279 owed to next owners; Rally !bStatsLoaded gate on next touch. Awaits TASK-103 batch compile)
- blocked-by: none
- parallel-safe: yes (SummonedUnit.h/.cpp + Building.h/.cpp ONLY — stays OUT of Tower.cpp per ruling 14)
- spec: >
    Files only. (1) ASummonedUnit::ApplyFreeze(float Seconds) + IsFrozen(): pause movement/AI/attack
    cadence reusing the M2 FreezeAI infrastructure WITHOUT breaking match-end precedence (ruling 5 —
    a spell-freeze expiry must never resume a match-end-frozen actor; document the mechanism, e.g.
    separate spell-freeze state vs the match-end flag); refresh = max(remaining, new). Frozen visual =
    optional flagged decision (minimal tint acceptable; the NS burst is the primary read).
    (2) ABuilding::ApplyFreeze(float Seconds) + IsFrozen() — STATE ONLY here; the tower fire-gate lands
    in TASK-101 against this API (ruling 14). Castle gets NO freeze API. (3) ASummonedUnit combat buff:
    extend the TASK-042 Rally move-speed buff API (align names with handoffs/TASK-042.md) with an
    attack-cadence multiplier; pinned resolver entry: ApplyCombatBuff(float MoveSpeedMult, float
    AttackSpeedMult, float Seconds); BattleCry magnitudes (+50% attack / +25% move — mechanic
    UPROPERTYs, GDD §4 comments) live with the API owner per Rally precedent (flag exact placement);
    self-refresh non-stacking; stacks WITH Rally/War Banner (ruling 6). (4) All freeze/buff timers
    cleared on match-end + Play Again reset paths. (5) Shadow law (new timer members especially).
    Acceptance (by inspection): freeze pause/resume + precedence correct; buff applies and restores
    exactly; reset paths clean. handoffs/TASK-099.md. Post in ⚙️ Dev & QA.
- names: >
    ASummonedUnit::ApplyFreeze / IsFrozen / ApplyCombatBuff (SummonedUnit.h/.cpp);
    ABuilding::ApplyFreeze / IsFrozen (Building.h/.cpp). Mechanic UPROPERTYs: BattleCry +50% attack /
    +25% move (GDD §4). DO NOT touch Tower.cpp (TASK-101 owns it). Law: M5 rulings 5, 6, 14.

#### TASK-100 — Targeting mode: reticle-anywhere spell play on ASiegePlayerController (files)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-08; qa/TASK-100-report.md — fix loop 1 verified PASS, 0 blockers remaining; rotation write proven single [file-wide grep], UpdateSpellReticle location-only, no-drift anchor audit clean [+8 line shift confined to SpawnSpellReticle]. Cleared for TASK-103 batch compile. WARN carry → TASK-109 PIE: elevated-anchor ring readability)
- blocked-by: TASK-093 (same-file serialization on SiegePlayerController.h/.cpp — ruling 14; logic-independent)
- parallel-safe: yes once unblocked (file-only)
- spec: >
    Files only. (1) RequestPlaySlot routing: CardType Spell + SpellEffect != GoldSteal → enter TARGETING
    mode (sibling of placement mode); GoldSteal → instant resolve on play (ruling 7; deduct-then-resolve,
    refusal-safe). (2) Targeting mode: cursor visible (placement-mode posture, M2 TASK-023 + TASK-074
    normalization laws — no new input assets); reticle position = TRACE to the surface under the cursor
    (M4.5 terrain carry-in LAW — never assume Z=0; reuse/extend the placement projection trace); reticle
    visual = decal component with soft-referenced /Game/Materials/M_SpellReticle (null-safe — missing
    material ⇒ targeting still works, log once). NO half restriction, NO navmesh requirement (§3.5 —
    spells land anywhere incl. the enemy half). (3) LMB confirm: deduct cost THEN
    USpellLibrary::ResolveSpell (card-leaves-hand-at-CONFIRM law; resolver false ⇒ FULL refund + HUD
    reason per §3.0). RMB/Esc cancel: exit free. (4) No BIE signature changes (refusal text rides the
    existing FString path); no UMG edits this task. (5) Shadow law. Acceptance (by inspection): mode
    transitions clean (placement/targeting/Alt-cursor interplay flagged for QA); surface-based
    projection; gold flow per confirm law. handoffs/TASK-100.md. Post in ⚙️ Dev & QA.
- names: >
    ASiegePlayerController (Source/.../SiegePlayerController.h/.cpp); USpellLibrary::ResolveSpell;
    M_SpellReticle soft path /Game/Materials/M_SpellReticle; existing OnCardRefusedMessage path
    (signature UNCHANGED). Law: M5 rulings 7, 8 + CONVENTIONS "Spells & Set III (M5)".

#### TASK-101 — Crystal Tower Chain attack + tower freeze-gate (files)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-08; qa/TASK-101-report.md — PASS, 0 blockers / 1 warn / 3 nits, all 10 flagged decisions ACCEPTED. Chain = units+hero only [manager carry: chains-hit-buildings would be a rule change]; bounces may exceed Range [balance carry: worst-case 1500 uu]. 099's IsFrozen confirmed on disk. WARN carry: stale SiegeGameMode.cpp:268 timer-invariant comment — one-line doc touch when that file next legally opens. Awaits TASK-103 batch compile)
- blocked-by: none
- parallel-safe: yes (Tower.h/.cpp only; TASK-099's IsFrozen pinned by spec)
- spec: >
    Files only. (1) ATower fire path: when Row.ChainTargets > 0, fire an INSTANT chain zap instead of a
    projectile (ruling 9): primary = nearest valid enemy in Range; bounce to ChainTargets−1 additional
    enemies, each within ChainBounceRadius (NEW UPROPERTY float = 350, // GDD §4 Chain,
    manager-defined) of the PREVIOUS target; per-hit damage = Dmg − n×ChainFalloff, floored at 0
    (15/10/5 with the CrystalTower row); no friendly fire; no target hit twice per zap; damage tagged
    USiegeDamageType_Projectile. (2) Chain visual: spawn /Game/VFX/NS_ChainZap at each hit — soft path,
    null-safe, log-once. (3) Freeze gate (ruling 14): ALL tower firing (projectile AND chain) gates on
    !IsFrozen() (TASK-099's ABuilding API). (4) Existing towers (Arrow/Bomb/Ballista — AoERadius +
    MinRange paths) behaviorally unchanged. (5) Shadow law. Acceptance (by inspection): falloff math;
    bounce measured from the previous target, not the tower; cadence respected; freeze-gate on both
    paths. handoffs/TASK-101.md. Post in ⚙️ Dev & QA.
- names: >
    ATower (Source/.../Tower.h/.cpp) — NEW UPROPERTY ChainBounceRadius (float, 350); consumes FCardRow
    ChainTargets/ChainFalloff (TASK-097); IsFrozen() (TASK-099); VFX /Game/VFX/NS_ChainZap;
    USiegeDamageType_Projectile. Law: M5 rulings 9, 14.

#### TASK-102 — Bot v4: M5 spell rules — Fireball at clusters, Lightning at defended towers (files)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-08; qa/TASK-102-report.md — PASS, 0 blockers / 2 warns / 3 nits, all 9 flagged decisions ACCEPTED. Pinned ResolveSpell signature verified char-for-char at both call sites. WARN carries → TASK-109 playtest: spell hand-clog watch [future hardening: exempt only first copy], centroid-outlier coverage watch. Awaits TASK-103 batch compile)
- blocked-by: none
- parallel-safe: yes (SiegeBotController.h/.cpp only)
- spec: >
    Files only. (1) Insert spell evaluation as rule 3 in the 2 s ordered loop (ruling 10; defend=1,
    miners=2, SPELLS=3, big-unit attack=4, discard=5 — keep trace labels consistent after renumbering):
    3a if Fireball in hand + affordable + ≥3 player-team units within a 300-radius cluster (cluster =
    any unit having ≥2 other player units within 300 — document the algorithm) → ResolveSpell at the
    cluster centroid; 3b if Lightning in hand + affordable + a player tower with ≥2 player units within
    400 → ResolveSpell at that tower's location. (2) Plays go through the bot's normal card-play/deck
    path (gold + discard-pile accounting identical to unit plays; the bot resolves directly through
    USpellLibrary — targeting mode is a human affordance, ruling 10). (3) FrostNova/BattleCry/Pickpocket:
    NO cast rule (GDD silent) — they fall through to rule-5 discard economics; document. (4) LogSiegeBot:
    exactly one line per fired spell rule (law). (5) No scans outside the 2 s cadence; shadow law.
    Acceptance (by inspection): rule order per ruling 10; affordability/hand checks precede target
    searches; decision trace grep-able. handoffs/TASK-102.md. Post in ⚙️ Dev & QA.
- names: >
    ASiegeBotController (Source/.../SiegeBotController.h/.cpp); USpellLibrary::ResolveSpell;
    LogSiegeBot. Law: M5 ruling 10 + GDD §4 M5 extension.

#### TASK-103 — M5 code batch: compile + residue adjudication + commit (build)
- assignee: build-master
- status: done (2026-07-08; handoffs/TASK-103.md — compile SUCCESS 18.84 s ZERO warnings [zero C4458], TASK-094 confinement check PASS, reset-and-restage doctrine applied [index arrived stale with 15 auto-staged art .uassets — left for TASK-109]. Commit 2c65164 on main, 20 files, NOT pushed. Editor relaunched, MCP live. DEVIATION: desktop LOCKED — SendInput blocked; "Don't Import" + "re-open asset editors" prompts pending [answer Don't Import / No when unlocked]; TASK-109 SendInput checks → WATCH list per TASK-076 doctrine)
- blocked-by: TASK-097..102 qa-passed (100 after its 093 serialization) + M4.5's TASK-093/094 qa-passed (fold them into THIS batch — their code ships regardless of the parked art; batching law)
- parallel-safe: no (owns the editor bounce + compile + commit)
- spec: >
    Editor-down batch compile (Build.bat per CLAUDE.md) of ALL qa-passed file tasks: M5 TASK-097..102 +
    M4.5 TASK-093/094. Failures → append errors to the offending task's qa report and route back to
    gameplay-programmer (counts as a QA loop). Adjudicate boot-resave residue per standing doctrine.
    ONE code commit on main listing every task ID, NOT pushed. Post compile result + hash in
    🔧 Build & Git.
- names: >
    Build target GitClaudeUnrealTestEditor (Build.bat per CLAUDE.md); commit to main only, NOT pushed.

#### TASK-104 — DT_Cards reimport: 28-row Set III + M5 test deck (editor)
- assignee: gameplay-programmer
- status: done (2026-07-08; handoffs/TASK-104.md — set_rows in-place, 28 rows column-complete, full-cell readback 0 mismatches, deck sum 50, CardArt plain-string paths verified resolving, saved not-dirty. Dead-card window open until TASK-107 — no PIE until then)
- blocked-by: TASK-103 (FCardRow columns must be compiled first; qa/TASK-021 WARN-2 law — reimport IMMEDIATELY after the compile, before any PIE)
- parallel-safe: no (editor-mutating)
- spec: >
    Editor/MCP. In-place set_rows update of /Game/Data/DT_Cards from Docs/Data/cards.csv (import_file
    refuses DataTable overwrite — learnings): all 28 rows, the six M5 columns, CardArt soft-object cells
    as PLAIN STRING paths, then READBACK-VERIFY every cell (DataTableTools NULL-storage trap — learnings
    law). Readback-verify DeckCount sum = 50 and each ≤ MaxCopies. Save clean. handoffs/TASK-104.md.
    Post in ⚙️ Dev & QA.
- names: >
    /Game/Data/DT_Cards (row struct FCardRow); Docs/Data/cards.csv. Law: M5 rulings 2, 12, 13.

#### TASK-105 — SM_CrystalTower blockout mesh (art)
- assignee: art-director
- status: ready-for-integration (2026-07-08; handoffs/TASK-105.md — SM_CrystalTower 1212 tris + M_CrystalGlow imported, authored UCX readback-verified [1 convex, footprint-exact], render confirmed, zero import warnings. Slot contract for TASK-107: slot 0 MI_TeamColor_Blue recolor, slot 1 M_CrystalGlow do-NOT-override. Integration check + commit ride TASK-107/109)
- blocked-by: none
- parallel-safe: yes (Blender + own new asset; serialize the editor-import step)
- spec: >
    Blockout-tier crystal spire tower per §6 (M4 TASK-067 pattern): crystal cluster atop a stone base,
    silhouette reads at 15 m, beveled edges, emissive crystal accent, NO flat single color; ≤8k tris;
    origin ground-center; UV layer UVMap; authored footprint-exact UCX base hull(s); Nanite OFF. Export
    Content/RawAssets/CrystalTower.fbx (axis contract); import NEW /Game/Meshes/SM_CrystalTower; verify
    hulls via ObjectTools BodySetup readback (TASK-088 technique). Slot 0 = MI_TeamColor_Blue
    design-time placeholder (team recolor law). Acceptance: imported, budgets/collision verified,
    silhouette capture in the handoff. handoffs/TASK-105.md. Post in 🎨 Art.
- names: >
    SM_CrystalTower (/Game/Meshes/SM_CrystalTower), FBX Content/RawAssets/CrystalTower.fbx,
    UCX_SM_CrystalTower*; slot 0 placeholder MI_TeamColor_Blue. Law: per-card visual assets + team
    contract + §6.

#### TASK-106 — Set III card art: 6 illustrations + import (art)
- assignee: art-director
- status: ready-for-integration (2026-07-08; handoffs/TASK-106.md — 6/6 T_CardArt_* imported + readback-verified [512², UI group, sRGB], /Game/UI/CardArt/ = 28 assets char-exact vs cards.csv; CrystalTower card rendered from the live SM_CrystalTower geometry. DT_Cards row readback deferred to TASK-109 [104 not yet run]. Integration check + commit ride TASK-109)
- blocked-by: none (TASK-097 writes the deterministic CSV paths up front; graceful text-only fallback until these land)
- parallel-safe: yes (Blender renders; serialize the editor-import step)
- spec: >
    TASK-077 pipeline: six illustrations — Fireball, FrostNova, Lightning, BattleCry, Pickpocket,
    CrystalTower — per the card-art law: 512², NO baked text, one dominant subject, strong silhouette,
    distinct per-card color key, team-agnostic, reads at ~150 px. PNG sources
    Content/RawAssets/CardArt/<CardID>.png; import as T_CardArt_<CardID> to /Game/UI/CardArt/ (Texture
    Group UI, sRGB on). After TASK-104 lands, readback one DT_Cards row to confirm a CardArt path
    resolves (else record for TASK-109). handoffs/TASK-106.md. Post in 🎨 Art.
- names: >
    T_CardArt_Fireball / T_CardArt_FrostNova / T_CardArt_Lightning / T_CardArt_BattleCry /
    T_CardArt_Pickpocket / T_CardArt_CrystalTower (/Game/UI/CardArt/); PNGs Content/RawAssets/CardArt/.
    Law: CONVENTIONS "Card artwork (hand UI)".

#### TASK-107 — BP_Building_CrystalTower (editor)
- assignee: gameplay-programmer
- status: done (2026-07-08; handoffs/TASK-107.md — data-only BP at the composed soft-class path, parent ATower verified by readback, zero graph edits/zero stat literals, slot-1 do-not-override contract honored, compiled clean warnings-as-errors, disk-backed CDO re-read post-save. Dead-card window CLOSED. M2 editor-wave precedent: live verification rides TASK-109's PIE suite [bot plays CrystalTower])
- blocked-by: TASK-103 (standard post-compile editor wave), TASK-105 (mesh)
- parallel-safe: no (editor-mutating)
- spec: >
    Editor/MCP. BP_Building_CrystalTower in Content/Blueprints/Buildings/ per the composed
    soft-class-path law (M2 TASK-035 pattern; same parent lineage as BP_Building_ArrowTower):
    VisualMesh = SM_CrystalTower, slot 0 MI_TeamColor_Blue placeholder, stats arrive from the DT_Cards
    CrystalTower row (nothing typed into the BP). Verify the placement ghost resolves
    /Game/Meshes/SM_CrystalTower. Save clean. handoffs/TASK-107.md. Post in ⚙️ Dev & QA.
- names: >
    BP_Building_CrystalTower (/Game/Blueprints/Buildings/BP_Building_CrystalTower); VisualMesh;
    SM_CrystalTower; MI_TeamColor_Blue. Law: Blueprint subclasses + per-card visual assets.

#### TASK-108 — Spell Niagara VFX set: NS_Spell_* ×5 + NS_ChainZap + M_SpellReticle (art)
- assignee: art-director
- status: ready-for-integration (2026-07-08; handoffs/TASK-108.md — 7/7 assets at exact paths, readback sweep verified, donor NS_Damage untouched [sole Variant_Combat Niagara donor — plural-spec deviation recorded]. §6 one-frame eyeball rides TASK-109 PIE [MCP can't replay one-shot previews]. M7 flags: Lightning ribbon bolt, Pickpocket coin meshes, reticle pulse. NOTE for next editor owner: dismiss the "7 source content changes" popup with DON'T IMPORT. Integration check + commit ride TASK-109)
- blocked-by: none (donor duplication; the code soft-references are null-safe in either landing order)
- parallel-safe: no (editor-mutating)
- spec: >
    Editor/MCP. (1) Duplicate Variant_Combat Niagara donors into /Game/VFX/ (template-donor law — never
    edit donors) and restyle per spell: NS_Spell_Fireball (orange burst, ~300-radius read),
    NS_Spell_FrostNova (blue-white ground ring, ~350), NS_Spell_Lightning (white-violet strikes),
    NS_Spell_BattleCry (gold rally ring, ~400), NS_Spell_Pickpocket (small coin flourish), NS_ChainZap
    (cyan zap impact for Crystal Tower hits). §6 bar: readable in ONE frame; visual radius should
    roughly match the gameplay radius (§3.11 legibility). Flag anything below bar for M7 (ruling 11).
    (2) M_SpellReticle (Content/Materials/, DeferredDecal domain, emissive ring — M_CenterlineStripe
    recipe; spell-blue, visually distinct from the centerline gold). Names character-exact — code
    COMPOSES /Game/VFX/NS_Spell_<CardID> (null-safe). handoffs/TASK-108.md. Post in 🎨 Art.
- names: >
    NS_Spell_Fireball, NS_Spell_FrostNova, NS_Spell_Lightning, NS_Spell_BattleCry, NS_Spell_Pickpocket,
    NS_ChainZap (/Game/VFX/); M_SpellReticle (/Game/Materials/M_SpellReticle). Law: CONVENTIONS
    "Spells & Set III (M5)" + template-donor rule.

#### TASK-109 — M5 final assembly: exit-criteria PIE verification + commit + m5-testable branch (build)
- assignee: build-master
- status: in-progress (dispatched 2026-07-08; all blockers done [104, 106, 107, 108]. Constraint: desktop LOCKED — SendInput items → WATCH list per TASK-076 doctrine)
- blocked-by: TASK-104, TASK-106, TASK-107, TASK-108
- parallel-safe: no (owns the single editor + the Git commit)
- spec: >
    Full M5 exit-criteria PIE run against the "M5 exit criteria" block above, check by check (SendInput
    injection drives hotkey plays + LMB reticle confirm — TASK-088 laws; mind the Alt-tap trap). Verify
    bot spell rules via LogSiegeBot grep; Play Again spell-state reset; log sweep vs knowns (DeepMine
    CardType-2, victory-focus, RecastNavMesh boot, CrowdFollowing teardown). Anything requiring a human
    hand → WATCH list per TASK-076 doctrine. ONE commit on main (editor/art batch + docs, all task IDs
    in the message), NOT pushed; cut branch m5-testable at the commit (milestone-preservation workflow;
    branch not pushed). Post verification summary + hash + branch in 🔧 Build & Git. Acceptance: exit
    criteria PASS or findings routed; ONE commit; m5-testable cut; nothing pushed.
- names: >
    /Game/Maps/L_Arena PIE; DT_Cards 28 rows live; branch m5-testable; commit to main only, NOT pushed.

### M4.5 — Gameplay terrain pass (TASK-091..096) — decomposed 2026-07-08 — **PARKED (Fab pivot, see amendment)**

**Verbatim intent (Jonathan):** the arena is "a boring white board"; he wants "a nice large grass area with trees and hills." Scoping ruled by Jonathan: (1) NOW, as a standalone milestone before M5 (the pulled-forward art pass noted on the M7 line); (2) REAL GAMEPLAY TERRAIN — hills act as high ground and trees act as obstacles INSIDE the playable arena. Deliberately amends GDD §5's "mostly open battlefield". Naming law added to CONVENTIONS.md "Arena terrain & environment (M4.5)" BEFORE task issue.

**AMENDMENT 2026-07-08 (same day — Jonathan Fab directive; supersedes the art-production path below):**
- **Assets come from Fab, not Blender production.** Jonathan supplies premade packs for FOUR slots — tree, rock, grass, hill — via the Epic Launcher into Content/Fab/<Pack>/ (READ-ONLY donor quarantine). Ledger: FAB-001 (tree) / FAB-002 (rock) / FAB-003 (grass) / FAB-004 (hill), all `approved` in .claude/pipeline/fab/FAB-REQUESTS.md; drop instructions in Content/Fab/README_DROP_ZONE.md. Donors are CONFORMED to the CONVENTIONS target names (SM_Tree_01/02, SM_Rock_01+, M_ArenaGround/T_ArenaGrass_*, SM_ArenaTerrain or base+SM_Hill_##) — target names stay the law.
- **ROCK is added scope** under the SAME law as trees (recommendation accepted): mirror-symmetric pairs, base-footprint-only collision, blocks navmesh/movement/placement/projectiles. Rock instances Rock_<NN>_<Side>.
- **TAG CONTRACT CHANGE (binding over the original rulings below):** the placement-clearance and projectile code contracts now read actor tag **`Obstacle`** (carried by ALL trees AND rocks) instead of `Tree`; walkable ground AND hill instances carry tag **`Terrain`**. The UPROPERTY is renamed **`ObstaclePlacementClearance`** (=150) and the refusal string is **"Too close to obstacles"**. Wherever a ruling or task block below says tag "Tree" for a CODE contract, read `Obstacle` — instance NAMES (Tree_*/Rock_*) are unaffected. New obstacle types never require code changes.
- **Hills may be placed instances:** if the Fab hill pack suits mounds-on-flat-ground better than one sculpted floor, TASK-091 may take the composite path (flat base + Hill_<NN>_<Side> mirror-pair instances, tag Terrain) — all original laws (flat pads, slope ≤35°, placeable crowns, mirror symmetry, complex-as-simple collision) bind either way.
- **Status: PARKED.** TASK-091/092 (now Fab-conform tasks) + TASK-095/096 wait on Jonathan's drop. TASK-093/094 (C++) remain valid + dispatchable NOW and ride M5's TASK-103 compile batch. **M5 runs AHEAD of this milestone (Jonathan's explicit ordering inversion — never block M5 on M4.5).** Resume trigger: Jonathan says the assets are in (Slack or Claude Code).
- Amended dispatch shape: 093 ∥ 094 now (with the M5 file wave) → QA → ship via TASK-103. On Fab drop: 091 ∥ 092 (serialize Blender/editor) → 095 → 096 (096's compile step is likely a no-op if 093/094 already shipped via TASK-103 — then it is verify + level/art commit + m4.5-testable only).

**M4.5 manager decisions (binding for all M4.5 tasks; amends GDD §5; read WITH the amendment above):**
1. **Symmetry REMAINS law.** The arena stays fair by geometry: the terrain heightfield mirrors exactly across the X=0 centerline plane (H(x,y) = H(−x,y)) and every tree at (x, y) has an exact twin at (−x, y). The centerline/placement-halves rule and the gold-node positions (±1200, 0) stay functional and unchanged.
2. **High ground is PHYSICAL ONLY** (orchestrator-relayed recommendation ACCEPTED): elevation grants NO stat bonuses — no damage/range/armor/vision modifiers, nothing enters cards.csv. Its value is physical: climbing costs pathing time, hills and tree trunks BLOCK projectiles in flight, and hill crowns are legal building ground (a tower on a hill is defended by geometry, not stats). Target ACQUISITION stays range-only this milestone — a tower may waste shots into a hillside at a target behind it; **WATCH:** if Jonathan's playtest reads that as broken, a LOS-at-acquisition check becomes a follow-up task (do NOT build it now).
3. **Terrain is a Blender-sculpted static mesh, NOT a Landscape actor.** MCP has no Landscape create/sculpt route (no editor Python, no console exec — orchestration learnings), while Blender FBX → MCP import is the proven pipeline. `SM_ArenaTerrain` replaces the `ArenaGround` engine-cube slab at the identical 6400×3200 footprint with the base walk surface at Z=0, so EVERY existing actor transform stays valid. Use Complex Collision As Simple (hulls cannot carry hills); Nanite OFF; ≤60k tris. This is a NEW asset (import_file works; no reimport-click problem).
4. **Flat-pad law (Z=0, near-zero slope):** castle pads r700 at (±2000,0); gold-node pads r400 at (±1200,0); main lane |Y| ≤ 300 between the castle pads (the §3.3 ~10 s miner walk stays flat); centerline strip |X| ≤ 300 (decal + fair mid). Hills/trees never intrude into pads or flat lanes.
5. **Hills: 4, mirrored** — centers (+700, +900), (+700, −900), (−700, +900), (−700, −900); height 250; near-flat crown r200 (≤10° — placeable per ruling 7); base radius ~600; walkable faces ≤ 35° (navmesh default 44° with margin). Max terrain height 250 ≪ the 1800-unit ArenaBoundary walls — no escape ramps (verified at integration). X-mirror is LAW; the Y-mirror here is composition, not law.
6. **Trees: 16 (8 mirrored pairs)** at (±400, 600), (±400, −600), (±900, 1300), (±900, −1300), (±1500, 900), (±1500, −900), (±2600, 450), (±2600, −450); Z snapped to the terrain surface (pairs 1–4 deliberately sit on hill flanks). Art may nudge a pair ≤150 units for composition ONLY if all pad/lane/crown clearances hold and the twin moves identically. Collision is TRUNK-ONLY (authored UCX; canopy has NO collision): the trunk carves the navmesh, blocks movement, building placement, and projectiles. **Instancing (HISM/foliage) DEFERRED** — 16 individual StaticMeshActors are trivially within budget and keep per-instance naming/tagging simple; revisit at M7 if counts grow.
7. **Placement on terrain (mechanic rules → UPROPERTY defaults with GDD § comments, NOT CSV):** buildings refused on ground steeper than `MaxPlacementSlopeDegrees` = 20 (HUD "Too steep") and within `ObstaclePlacementClearance` = 150 (2D) of any `Obstacle`-tagged actor (trees AND rocks — Fab amendment) (HUD "Too close to obstacles"); both pre-checked BEFORE gold moves (net-zero refusal law). Units/miners need only a navmesh-valid point (unchanged). The ghost projects to the terrain SURFACE height and stays upright (no normal tilt). Castle-roof refusal (navmesh projection law), enemy-half refusal, and the 200-unit building-vs-building clearance are unchanged.
8. **Projectile terrain law:** homing projectiles are DESTROYED (zero damage, no AoE) on impact with walkable terrain or obstacle footprints (actor tags `Terrain` / `Obstacle` — Fab amendment) — and deliberately do NOT collide with walls/buildings/castles beyond shipped behavior (the archer-behind-own-wall comp and §3.0 castle scaling stay exactly as shipped).
9. **Grass = material tier this milestone:** stylized grass MATERIAL per §6 (no flat single color — macro variation; slope-darkened dirt on hill flanks allowed); grass-blade foliage is M7 polish. **Perf gate:** §6 60 fps @1440p (RTX 3060 class, 60+ units) — no machine console/stat route exists (learnings), so integration records best-effort timings and the formal gate is a human WATCH at Jonathan's M4.5 playtest.
10. **Bot: verify, don't rewrite.** Its placement is navmesh-based; M4.5 changes zero bot code. Preserved in place: KillZ −2000, the 4 ArenaBoundary walls, PlayerStart (−1400,0,100), CenterlineMarker decal, NavMeshBounds_Arena, castles, gold nodes, anchors, lighting stack.
11. **M5 forward-flag (inherit at M5 decomposition):** the spell-targeting reticle must project onto UNEVEN terrain — trace to the terrain surface, never assume the Z=0 plane.

Dispatch shape (SUPERSEDED by the amendment above — kept for the audit trail): ~~TASK-091 ∥ TASK-093 ∥ TASK-094 immediately; TASK-092 behind 091~~. Current shape: 093 ∥ 094 now (ride M5's TASK-103 compile); 091 ∥ 092 on Jonathan's Fab drop → 095 → 096.

#### TASK-091 — Fab conform: hill/grass donors → arena terrain + M_ArenaGround (art)
- assignee: art-director
- status: backlog
- blocked-by: FAB-003 + FAB-004 fulfilled (Jonathan's Fab drop — external gate; not an agent dependency)
- parallel-safe: yes vs code tasks (serialize Blender-socket work with TASK-092 and the editor-import step with any other editor-mutating work)
- spec: >
    Fab CONFORM + integration (amendment path; FAB-REQUESTS.md protocol step 4). Donors: Jonathan's hill
    pack (FAB-004) + grass pack (FAB-003) under Content/Fab/<Pack>/ — READ-ONLY; duplicate into /Game/
    or route through Blender (bridge <30 s ops; headless blender.exe --background for heavy work) to
    conform. (1) TERRAIN — pick and record the path: (a) UNIFIED: build/adapt the donor into ONE
    SM_ArenaTerrain per the original rulings 3-5 (6400×3200, walk surface at origin Z, ≥100 skirt, 4
    hills at (±700,±900) h250 crown r200 base ~600, flat pads/lanes per ruling 4, exact X-mirror, ≤60k
    tris); or (b) COMPOSITE: a flat base ground (SM_ArenaTerrain as the flat slab replacement, grass
    material applied) + donor hill meshes conformed to SM_Hill_01(+variants) for placement as mirror-pair
    instances by TASK-095 — hills must be WALKABLE (faces ≤35°, near-flat placeable crown, no pad/lane
    intrusion at the ruling-5 positions). Either path: Use Complex Collision As Simple on all walkable
    terrain (CollisionTraceFlag readback — TASK-088 technique), Nanite OFF, UV layer UVMap. Report
    measured face angles (walkable band + crowns) and the mirror-verification method. (2) GRASS —
    conform the FAB-003 donor material/textures into M_ArenaGround (/Game/Materials/): §6 stylized, NO
    flat single color, macro variation; donor textures duplicated into /Game/Textures/ as
    T_ArenaGrass_*; apply to the ground (and hill meshes if composite). Grass-blade foliage from the
    pack: only if trivially within the §6 perf budget — flag the call. (3) Do NOT touch L_Arena
    (TASK-095 places everything). (4) Record conform notes + chosen path in FAB-REQUESTS.md
    (integration lines FAB-003/004) and handoffs/TASK-091.md. Acceptance: conformed assets imported at
    the CONVENTIONS target names with collision/budgets/slope laws verified by readback; material §6-
    compliant; donor packs untouched. 🎨 Art post.
- names: >
    Targets: SM_ArenaTerrain (/Game/Meshes/), SM_Hill_01+ (/Game/Meshes/, composite path only),
    M_ArenaGround (/Game/Materials/), T_ArenaGrass_* (/Game/Textures/). Donors: Content/Fab/<Pack>/
    (FAB-003, FAB-004 — READ-ONLY). UV layer UVMap. Law: CONVENTIONS "Arena terrain & environment
    (M4.5)" incl. Fab amendment + rulings 3-5.

#### TASK-092 — Fab conform: tree + rock donors → SM_Tree_01/02 + SM_Rock_01 obstacles (art)
- assignee: art-director
- status: backlog
- blocked-by: FAB-001 + FAB-002 fulfilled (Jonathan's Fab drop — external gate; not an agent dependency)
- parallel-safe: yes vs code tasks (serialize Blender-socket work with TASK-091 and the editor-import step with any other editor-mutating work)
- spec: >
    Fab CONFORM + integration (amendment path). Donors: Jonathan's tree pack (FAB-001) + rock pack
    (FAB-002) under Content/Fab/<Pack>/ — READ-ONLY. Select two visually distinct trees and one-or-more
    rocks; conform each (duplicate into /Game/ or via Blender) to: SM_Tree_01, SM_Tree_02, SM_Rock_01
    (+ SM_Rock_02.. if variants are worth it). Per-asset conform law (CONVENTIONS obstacles bullet):
    ≤4k tris (decimate donors if over), Nanite OFF, origin at trunk-base/rock-base ground-center, UV
    layer UVMap, §6 silhouette/material bar (donor materials conformed under M_Tree / M_Rock; MI hue
    variants allowed). COLLISION: FOOTPRINT-ONLY — strip donor canopy/full-mesh collision; keep/author
    ≤2 simple hulls hugging the trunk (r~50-70) or rock base; verify hull count + type via ObjectTools
    BodySetup_0.AggGeom readback (nothing auto-generated, no canopy hulls). Do NOT place anything in
    L_Arena (TASK-095). Record conform notes in FAB-REQUESTS.md (integration lines FAB-001/002) +
    handoffs/TASK-092.md with viewport captures. Acceptance: all obstacle meshes imported at target
    names, footprint-only hulls verified by readback, budgets met, donors untouched. 🎨 Art post.
- names: >
    Targets: SM_Tree_01, SM_Tree_02, SM_Rock_01(+) (/Game/Meshes/); materials M_Tree, M_Rock
    (/Game/Materials/; MI variants in /Game/Materials/Instances/). Donors: Content/Fab/<Pack>/ (FAB-001,
    FAB-002 — READ-ONLY). UV layer UVMap. Law: CONVENTIONS "Arena terrain & environment (M4.5)" incl.
    Fab amendment + ruling 6.

#### TASK-093 — Placement v3: building slope limit + tree clearance + ghost on sloped ground (C++)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-08; qa/TASK-093-report.md — PASS, 0 blockers / 1 warn / 3 nits, all 8 flagged decisions ACCEPTED. WARN: transient-actor one-frame "Too steep" flash, fail-safe, PIE-check at 096/103. QA confirmed TraceCursorToGround safely reusable for TASK-100's reticle. Manager carry-forward: names blocks should pin OnCardPlayRefused/OnCardRefused explicitly. Single-writer hold lifted → TASK-100 dispatched. Awaits TASK-103 batch compile)
- blocked-by: none
- parallel-safe: yes (file-only: SiegePlayerController.h/.cpp; no overlap with TASK-094's Projectile files; serialize around compiles per standing law)
- spec: >
    Files only, no editor, no compile. Extend the TASK-030/059 placement path in ASiegePlayerController
    for the M4.5 terrain (rulings 4, 7). (1) NEW UPROPERTYs (EditDefaultsOnly, Category
    "Siegebound|Placement", // GDD §5 (M4.5) comments — mechanic rules, NOT CSV): float
    MaxPlacementSlopeDegrees = 20.f; float ObstaclePlacementClearance = 150.f. (2) Slope check, BUILDINGS
    only: at the navmesh-projected candidate point, line-trace straight down (candidate +Z500 →
    −Z500, WorldStatic/Visibility — programmer picks + documents); slope = angle between ImpactNormal
    and +Z; slope > MaxPlacementSlopeDegrees ⇒ refuse with HUD reason "Too steep" through the existing
    refusal path, pre-checked BEFORE gold moves (net-zero law). Trace miss ⇒ refuse (fail-closed, log
    verbose). (3) Obstacle clearance, BUILDINGS only (Fab amendment — covers trees AND rocks): any actor
    carrying tag "Obstacle" (exact FName) whose location is within ObstaclePlacementClearance 2D of the
    candidate ⇒ refuse "Too close to obstacles" (small N — TActorIterator acceptable; caching optional,
    document the choice). (4) Ghost projection:
    the ghost actor's Z must come from the projected/traced SURFACE height at the cursor point (works on
    the 250-high crowns and on flanks); rotation stays upright — NO normal alignment; the new refusals
    show the red ghost exactly like existing invalid placements. (5) UNCHANGED: unit/miner placement
    (navmesh-valid point suffices), castle-roof refusal, enemy-half refusal, 200 building clearance,
    miner cap, card leaves hand at CONFIRM. (6) No cards.csv/DT_Cards change; no BIE signature change
    (refusal text rides the existing FString path). (7) CONVENTIONS shadow law (C4457/58/59) — QA MUST
    scan pre-compile. Acceptance (by inspection, pre-compile): both refusals route pre-gold with the
    exact HUD strings; slope math correct at the 20° threshold; tag string "Obstacle"
    character-for-character; ghost Z from surface + upright; nothing unchanged-listed touched.
    handoffs/TASK-093.md with flagged decisions (trace channel, caching, where the slope check sits in
    the validation order). Post in ⚙️ Dev & QA.
- names: >
    ASiegePlayerController (Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h/.cpp) — NEW
    UPROPERTYs MaxPlacementSlopeDegrees (float, 20), ObstaclePlacementClearance (float, 150); actor tag
    "Obstacle" (exact — trees AND rocks, set by TASK-095); HUD refusal strings "Too steep" / "Too close
    to obstacles"; existing
    refusal path OnCardRefusedMessage (signature UNCHANGED). Law: CONVENTIONS "Arena terrain &
    environment (M4.5)" + rulings 4, 7.

#### TASK-094 — Projectile terrain/tree collision: arrows die on hills and trunks (C++)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-08; qa/TASK-094-report.md — PASS, 0 blockers / 2 warns / 2 nits, all 5 flagged decisions PASS/ACCEPTED. BINDING CARRIES: [1] TASK-096 must treat a missing TASK-091/092 collision readback as a projectile-law failure [bTraceComplex=false makes those readbacks load-bearing]; [2] TASK-103 runs a git-diff confinement check on Projectile.h/.cpp lineage. WATCHes: crown-lip downhill shots, final-approach sliver. Awaits TASK-103 batch compile)
- blocked-by: none
- parallel-safe: yes (file-only: Projectile.h/.cpp; no overlap with TASK-093; serialize around compiles per standing law)
- spec: >
    Files only, no editor, no compile. Implement ruling 8 on AProjectile (TASK-026 lineage). (1) In
    flight, detect impact with walkable terrain (tag "Terrain") or obstacle footprints (tag "Obstacle" —
    Fab amendment: trees AND rocks): mechanism
    is the programmer's choice (per-tick sweep from last position, or blocking-hit filtering by owner
    tag) but MUST be robust at 1500 u/s (no tunneling through a trunk) and O(hit-result) per tick — no
    world scans. On impact: Destroy(), ZERO damage, no AoE, no friendly-fire side effects; impact VFX
    optional only if a one-line reuse of the existing impact effect, else silent despawn with a
    VeryVerbose log. (2) MUST NOT change behavior vs walls, buildings, castles, or units — homing,
    target-overlap damage, §3.0 castle scaling (Projectile 50%), and despawn-on-target stay byte-level
    equivalent in behavior. (3) Homing note: when the target moves behind a hill the projectile may
    legitimately impact the hillside — that IS the high-ground value (ruling 2). NO acquisition/LOS
    changes anywhere (towers/units keep range-only targeting) — restate the ruling-2 WATCH in the
    handoff. (4) Null-safe on tag lookups; CONVENTIONS shadow law — QA scans pre-compile. Acceptance
    (by inspection, pre-compile): terrain/obstacle impact destroys without damage; tag strings "Terrain" /
    "Obstacle" character-for-character; no change to any shipped projectile interaction; flagged decisions
    (detection mechanism, VFX choice) in handoffs/TASK-094.md. Post in ⚙️ Dev & QA.
- names: >
    AProjectile (Source/GitClaudeUnrealTest/Siegebound/Projectile.h/.cpp); actor tags "Terrain" / "Obstacle"
    (exact, set by TASK-095). UNCHANGED: USiegeDamageType_* (DamageTypes.h/.cpp), castle 50% projectile
    scaling, homing + target overlap, MinRange/AoERadius consumers. Law: rulings 2, 8 + Fab amendment.

#### TASK-095 — L_Arena v3: terrain swap + mirrored trees & rocks + navmesh over hills (editor)
- assignee: art-director
- status: backlog
- blocked-by: TASK-091, TASK-092 (⇒ transitively on Jonathan's Fab drop)
- parallel-safe: no (editor-mutating — one editor instance; touches L_Arena.umap)
- spec: >
    Editor/MCP work in /Game/Maps/L_Arena. NO C++ dependency — runs before/parallel to the code compile.
    (1) DELETE the ArenaGround engine-cube slab (StaticMeshActor at (0,0,-50), scale (64,32,1) —
    confirm by class + transform before deleting; handoffs/TASK-015.md). PRESERVE everything else
    (ruling 10): Castle_Blue/Red, CastleAnchor_Blue/Red, GoldNode_Blue/Red, PlayerStart (−1400,0,100),
    CenterlineMarker decal, NavMeshBounds_Arena, ArenaBoundary_East/West/North/South (tag ArenaBoundary),
    WorldSettings KillZ −2000, template lighting stack. (2) Place StaticMeshActor "ArenaTerrain" =
    SM_ArenaTerrain at (0,0,0) rot (0,0,0) scale 1; add actor tag "Terrain" (exact). If TASK-091 took
    the COMPOSITE path (Fab amendment): additionally place the hill meshes as Hill_<NN>_<Side> mirror
    pairs at the ruling-5 positions (±700,±900), each with tag "Terrain". Trace-verify the walk surface:
    Z≈0 at (0,±800), (±1200,0), (±2000,0), (−1400,0); Z≈250 at the four crowns (±700,±900); spot slope
    checks on flanks ≤35°. (3) Place obstacles per ruling 6 + Fab amendment: 16 trees
    Tree_01_Blue..Tree_08_Blue (X<0) / Tree_01_Red..Tree_08_Red (X>0) at the ruling-6 coordinates
    (odd NN → SM_Tree_01, even NN → SM_Tree_02 by default — art may swap variants per pair, twins ALWAYS
    identical), PLUS rocks Rock_01_<Side>.. as mirror pairs (art places 2-4 rock pairs for composition —
    same clearance laws: never in pads/lanes/crowns, |X| ≥ 400, twins exact). Z snapped to the traced
    surface; EVERY obstacle instance (tree AND rock) gets actor tag "Obstacle" (exact); per-instance yaw
    free. Nudges ≤150 allowed (twin moves identically). READBACK-VERIFY: for every pair, Location(Red) ==
    Location(Blue) × (−1,+1,+1) within 1 unit; every obstacle carries the Obstacle tag. (4) Navmesh:
    verify RecastNavMesh regenerates over the hills (crowns covered — NavMeshBounds_Arena spans Z±500 so
    250 fits; raise the volume ONLY if coverage fails, record it) and carves around every obstacle
    footprint; the Y=0 lane is navmesh-continuous castle-to-castle. (5) CenterlineMarker still renders
    on the terrain at the flat centerline strip; boundary traces at all 4 edges still block (walls start
    at ±3200/±1600). (6) Save L_Arena clean (is_dirty=false). NO PIE gameplay suite here (TASK-096 owns
    it); a PIE boot smoke test is fine. Acceptance: slab gone; terrain (and hills, composite path)
    placed + tagged Terrain; all obstacles mirror-exact + tagged Obstacle; navmesh covers hills and
    carves footprints; every preserved actor untouched (readback); level saved. handoffs/TASK-095.md
    MUST include the final obstacle coordinate table (the reference for TASK-096 and future passes).
    🎨 Art post.
- names: >
    /Game/Maps/L_Arena. DELETE: ArenaGround. ADD: ArenaTerrain (SM_ArenaTerrain, tag "Terrain", at
    origin) [+ Hill_<NN>_<Side> (SM_Hill_##, tag "Terrain") if composite]; Tree_01..08_<Side>
    (SM_Tree_01/02) + Rock_01..NN_<Side> (SM_Rock_01+), ALL tagged "Obstacle", at ruling-6 coordinates
    + mirrored rock picks. PRESERVE: Castle_Blue/Red, CastleAnchor_*, GoldNode_Blue/Red, PlayerStart,
    CenterlineMarker, NavMeshBounds_Arena, ArenaBoundary_*, KillZ −2000, lighting. Law: rulings 1, 4-6,
    10 + Fab amendment + CONVENTIONS "Arena terrain & environment (M4.5)".

#### TASK-096 — M4.5 integration: compile, terrain-gameplay PIE suite, commit + m4.5-testable branch (build)
- assignee: build-master
- status: backlog
- blocked-by: TASK-093 (qa-passed), TASK-094 (qa-passed), TASK-095
- parallel-safe: no (owns the single editor + the compile + the Git commit)
- spec: >
    Integration for the M4.5 terrain milestone (TASK-090 pattern). (1) EDITOR DOWN → compile the
    TASK-093/094 batch via the standard Build.bat (CLAUDE.md) — NOTE (Fab amendment): if 093/094 already
    shipped via M5's TASK-103 batch, this step is a no-op; this task is then verify + level/art commit
    only. Failure → append errors to the offending task's qa report and route back to gameplay-programmer
    (counts as a QA loop). Adjudicate any editor-bounce boot-resave residue per standing doctrine (never
    blind-commit; record decisions).
    (2) Relaunch; PIE suite on direct-boot L_Arena:
    (a) PATHING: a Blue Footman placed on a south flank paths over/around hills to the Red castle; a
    Miner reaches GoldNode_Blue in ~10 s (flat-lane law); a Siege unit (Sapper or Ogre) crosses the
    field; a Wall placed mid-lane carves the navmesh and units reroute (dynamic obstruction unregressed).
    (b) PLACEMENT: ghost sits ON the slope surface and upright; a building on a hill FLANK refused
    "Too steep" with net-zero gold; a building on a hill CROWN ACCEPTED — place an ArrowTower on a crown
    (the high-ground payoff); a building within 150 of a trunk refused "Too close to obstacles"; a unit
    placed on a slope spawns fine; castle-roof refusal, enemy-half refusal, and 200 building clearance
    unregressed. (c) PROJECTILES: an archer/tower shot at a target behind a hill impacts the terrain and
    despawns with ZERO damage (HP readback); a tree trunk blocks a shot; arrows still fly from behind
    the player's own wall (comp preserved); castle 50% projectile scaling unchanged. (d) HERO: climbs a
    crown at sprint; perimeter escape check — hills must not ramp over the ArenaBoundary walls; forced
    KillZ fall → respawn at own castle in 5-6 s. (e) BOT SANITY (verify, don't rewrite): a ≥3-minute
    (or full) match vs the bot — bot placements land navmesh-valid on the terrain, its waves path, no
    t=0 Rule-1 false trigger (TASK-070 law), match end still fires if reached. (f) PERF (ruling 9):
    best-effort machine-readable timings only (no console/stat route — learnings); record terrain/tree
    tri + actor counts; the formal §6 60 fps gate is a human WATCH for Jonathan's M4.5 playtest — write
    the WATCH into the handoff. (g) LOG SWEEP vs knowns (DeepMine CardType-2, victory-focus,
    RecastNavMesh boot warning, CrowdFollowing teardown). (3) ONE commit on main, message
    "TASK-091..096: M4.5 gameplay terrain pass — ..." covering code + L_Arena + meshes/materials/
    textures + FBX/PNG raws + pipeline docs; **NOT pushed**. Then cut branch m4.5-testable at that
    commit (milestone-preservation workflow; branch not pushed either). (4) Post compile result +
    verification summary + commit hash + branch in 🔧 Build & Git. Acceptance: clean compile; PIE checks
    (a)-(g) PASS (or findings routed); ONE commit on main + m4.5-testable cut; nothing pushed.
- names: >
    Build target GitClaudeUnrealTestEditor (Build.bat per CLAUDE.md). Verify live: ASiegePlayerController
    MaxPlacementSlopeDegrees=20 / ObstaclePlacementClearance=150; map /Game/Maps/L_Arena; actors ArenaTerrain
    [+ Hill_* if composite] (tag Terrain) + Tree_*/Rock_* (tag Obstacle); walkable terrain
    complex-as-simple. Branch m4.5-testable; commit to main only, NOT pushed.

### Gold economy balance chain (TASK-089..090) — Jonathan directive 2026-07-08
**Verbatim intent (URGENT):** "I need to implement a balancing change right now. The default passive gold accumulation is way too high, lower it to about 1 gold every 2 seconds, and have the players start the game with only 10 gold."

**Manager rulings (binding for this chain):**
1. **Mechanic-rule territory CONFIRMED, not card data:** base income + starting gold are ASiegePlayerState UPROPERTY defaults with GDD § comments (M2 ruling) — no cards.csv/DT_Cards column is touched, so NO DT_Cards reimport this chain. Nothing here belongs in the table; nothing table-owned gets hardcoded.
2. **"1 gold per 2 s" implementation ruling — every-Nth-tick base grant; the 1.0 s tick stays:** gold stays int32 throughout (no float gold — HUD integer display assumptions hold). `GoldTickInterval` stays 1.0f (SiegePlayerState.h:267) so miner + DeepMine per-second income is LITERALLY untouched. `GoldPerTick` 2 → 1 (h:263), redefined as "base gold per base-income grant"; NEW UPROPERTY `int32 BaseIncomeTickPeriod = 2` (EditDefaultsOnly, Category "Siegebound|Gold", ClampMin "1"; 1 = legacy every-tick behavior) = number of income ticks between base grants. `HandleGoldTick` (SiegePlayerState.cpp:120-134) decomposes: miner + flat income granted EVERY tick; base (× overtime multiplier, read live) granted on every BaseIncomeTickPeriod-th tick via a transient non-reflected tick counter; still exactly ONE SetGold call per tick (one OnGoldChanged max, choke-point law intact). REJECTED alternative (recorded): GoldTickInterval → 2.0 s — it halves miner/DeepMine per-second income unless their per-tick values double (MinerGoldPerTick 1→2, DeepMineIncome 2→4), contradicts the GDD §3.3/§8 "+N/s" comments, and turns the HUD "+N/s" into a 2× lie once miners exist.
3. **Overtime knock-on:** the doubling IS a multiplier, not a hardcoded sum — verified at SiegePlayerState.cpp:152 (`GoldPerTick * OvertimeIncomeMultiplier`); `OvertimeIncomeMultiplier` stays 2 (h:279). Post-change overtime base = 2 per 2 ticks = **1 gold/s** — exactly double the new 0.5/s base. Multiply-per-grant, applied on the grant tick.
4. **HUD rate display law:** FOnGoldRateChanged stays `int32` — BIE contract byte-identical, NO UMG change in this chain. `GetGoldRate()` (cpp:146-158) is REDEFINED as the DISPLAY rate: miners + flat + `FMath::DivideAndRoundUp(EffectiveBase, BaseIncomeTickPeriod)` — the per-second average with the base rounded UP. Pre-overtime it shows "+1/s" while the true base is 0.5/s (max error 0.5, exact from overtime on); RULED acceptable — "+0/s" over a visibly rising counter reads as broken, and round-up is stable (no alternating values spamming RefreshGoldRate change detection). Consequence: HandleGoldTick NO LONGER calls GetGoldRate() for accrual (doc comments must say so). The 7:00 signal stays on the overtime HUD indicator (display base is 1 both sides of the flip — OnGoldRateChanged correctly stays silent for a miner-less economy).
5. **Starting gold:** `StartingGold` 50 → 10 (h:259) AND the private `Gold` field initializer 50 → 10 (h:313) — they were in sync at 50; keep them in sync (pre-BeginPlay seed value), with a keep-in-sync comment. Play Again is automatically correct: ResetGold() → SetGold(StartingGold) (cpp:78). The tick-parity counter resets on the reset path (ResetEconomy() preferred — Play Again runs ResetEconomy + ResetGold + ResumeIncome) so the post-reset base cadence is deterministic; programmer picks the exact spot and documents it in the handoff.
6. **Explicitly UNCHANGED (Jonathan didn't ask):** `MinerGoldPerTick` = 1 (h:283), `DeepMineIncome` = 2 (DeepMine.h:66 — file untouched), `GoldTickInterval` = 1.0f (h:267), `OvertimeIncomeMultiplier` = 2 (h:279), `MaxGold` = 999 (h:271), `SandboxStartingGold` = 9999 (SiegeGameMode.h:271 — dev bench, AddGold grant ON TOP of StartingGold, clamps at MaxGold; the sandbox stays generous by design, TASK-071). SpendGold/AddGold/AddIncome/RemoveIncome APIs and both gold delegates (FOnGoldChanged/FOnGoldRateChanged) byte-identical.
7. **Bot symmetry is automatic and intentional** — the bot economy is the same ASiegePlayerState, so its opening slows with the player's (softens the measured bot rush). TASK-090 MUST re-measure the undefended-Blue-castle kill time in PIE for the balance ledger (prior marks: ~48 s TASK-076, ~33 s TASK-088 post-Trellis).
8. **GDD divergence note:** GDD §3.2 (gold 50, base +2/s) now diverges from the shipped defaults — Jonathan's directive supersedes. The GDD § comments IN CODE are corrected by TASK-089; the GDD document itself is Jonathan's to revise (no doc task issued).
9. **Bounce-window chores FOLD IN to TASK-090** — this chain's compile is the first editor bounce since they were queued, and a running editor holds streaming locks, so ALL residue work happens while the editor is DOWN for the compile: (a) **WBP_MainMenu.uasset resave delta** (standing since TASK-081 phase 1; TASK-085 resume note = restore at next bounce window) → `git restore Content/UI/WBP_MainMenu.uasset` while the editor is down, then structural readback (both menu buttons bound) after relaunch; if the next editor session re-dirties it, STOP chasing — commit it knowingly at the next opportunity and close the loop; record which branch was taken. (b) **BP_Unit_Footman + BP_Unit_Archer resave deltas** (TASK-088 residue note: mesh-reimport component re-registration, verified benign, saved by Jonathan 2026-07-08) → COMMIT knowingly as a chore line in this chain's commit — they serialize the settled state of the shipped Trellis meshes; restoring would only re-dirty them.

Dispatch shape: **TASK-089 (C++, file-only — can start immediately) → QA (standard status-flow gate on TASK-089; shadow-scan mandatory) → TASK-090 (build-master: editor-down residue chores + compile + PIE economy verification + bot-rush re-measure + ONE commit, NOT pushed).**

#### TASK-089 — Economy balance: StartingGold 10 + base income 1 gold per 2 s (C++)
- assignee: gameplay-programmer
- status: qa-passed (QA PASS 2026-07-08 — 0 blocker/1 warn/3 nit; all 10 spec points verified incl. Play Again order-of-operations proof (ResetClock L583→ResetEconomy L605→ResetGold L606→ResumeIncome L607, first grant exactly tick 2 boot AND reset); F1–F5 all ACCEPT; shadow scan clean; WARN-1 = 3 now-false "back to 50" comments at SiegeGameMode.cpp:596/613/627 — fold into next programmer touch; NITs logged (stale comments in untouched files + optional Max(1,divisor) hardening). CF-1..8 carry-forwards to TASK-090 in qa/TASK-089-report.md. Orchestrator reconciled CF residue question: git status confirms BOTH BP_Unit deltas + WBP_MainMenu present — QA snapshot staleness again, ruling 9 stands as written.) (2026-07-08: StartingGold+Gold init 50→10 w/ keep-in-sync comments; GoldPerTick 2→1 + NEW BaseIncomeTickPeriod=2 (ClampMin 1) + non-reflected BaseIncomeTickCounter; HandleGoldTick decomposed — miner/flat every 1s tick, base grant every Nth tick, ONE SetGold/tick, no GetGoldRate in accrual; GetGoldRate = display rate (DivideAndRoundUp); counter reset in ResetEconomy (Play Again path deterministic); SiegeGameMode.cpp comment-only ×3 hunks verified; delegates + income APIs byte-identical; 5 flagged decisions (F1 >= threshold, F2 delegate doc prose, F3 extra stale comments, F4 shadow self-scan clean, F5 residue untouched). handoffs/TASK-089.md)
- blocked-by: none
- parallel-safe: yes (file-only: SiegePlayerState.h/.cpp + comment-only touch-ups in SiegeGameMode.cpp; no other chain currently open)
- spec: >
    Files only, no editor, no compile. Jonathan balance directive (URGENT, 2026-07-08): passive base gold
    accumulation → ~1 gold per 2 seconds; starting gold → 10. All in ASiegePlayerState
    (Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h/.cpp); current values verified by the manager
    2026-07-08 and cited below (line numbers pre-change, cited ~).
    (1) StartingGold 50 → 10 (h:259) AND the private Gold field initializer 50 → 10 (h:313); add/keep a
    keep-in-sync comment tying the two (ruling 5). Play Again needs no code change — ResetGold() already
    seeds from StartingGold (cpp:78).
    (2) GoldPerTick 2 → 1 (h:263); redefine its doc comment: base gold added per BASE-INCOME GRANT (one
    grant every BaseIncomeTickPeriod income ticks), doubled by OvertimeIncomeMultiplier while overtime is
    active. Keep the GDD §3.2 tag and note the 2026-07-08 balance directive.
    (3) NEW UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Gold", meta = (ClampMin = "1"))
    int32 BaseIncomeTickPeriod = 2; — number of GoldTickInterval income ticks between base-income grants
    (2 ⇒ base lands every 2 s; 1 = legacy every-tick). GoldTickInterval itself stays 1.0f (h:267) so miner
    and DeepMine per-second income is untouched (ruling 2).
    (4) HandleGoldTick (cpp:120-134) decomposition: keep the bIncomePaused gate; advance a transient
    non-reflected tick counter (plain int32 member, CachedGoldRate pattern — NOT a UPROPERTY); per tick
    grant = MinerIncomeCount*MinerGoldPerTick + FlatIncomePerTick, PLUS (GoldPerTick ×
    OvertimeIncomeMultiplier if overtime, read live as today) on every BaseIncomeTickPeriod-th tick; exactly
    ONE SetGold(Gold + Grant) per tick (SetGold stays the only Gold writer; a zero-grant tick is a harmless
    SetGold no-op). HandleGoldTick MUST NOT call GetGoldRate() for accrual anymore — say so in comments.
    (5) Counter reset for determinism: reset the tick counter on the reset path (ResetEconomy() preferred;
    Play Again = ResetEconomy + ResetGold + ResumeIncome) so the first post-reset base grant lands exactly
    on the BaseIncomeTickPeriod-th tick. Document the chosen spot + rationale in the handoff.
    (6) GetGoldRate() (cpp:146-158) redefined as the DISPLAY rate (ruling 4): MinerIncomeCount*
    MinerGoldPerTick + FlatIncomePerTick + FMath::DivideAndRoundUp(EffectiveBase, BaseIncomeTickPeriod)
    where EffectiveBase = GoldPerTick × (overtime ? OvertimeIncomeMultiplier : 1). Doc comment MUST state:
    per-second average with the base rounded UP for display; no longer the exact per-tick accrual. With
    defaults this shows +1/s pre-overtime (true 0.5/s) and +1/s in overtime (exact) — ruled acceptable;
    RefreshGoldRate change detection is unaffected (value is stable, never alternates).
    (7) BIE/DELEGATE CONTRACT BYTE-IDENTICAL: FOnGoldChanged + FOnGoldRateChanged signatures untouched
    (int32); no UMG/widget change in this chain. SpendGold/AddGold/AddIncome/RemoveIncome/PauseIncome/
    ResumeIncome behavior unchanged.
    (8) EXPLICITLY UNCHANGED (ruling 6 — verify you did not touch them): GoldTickInterval 1.0f,
    OvertimeIncomeMultiplier 2, MinerGoldPerTick 1, MaxGold 999, DeepMine.h DeepMineIncome 2 (file
    untouched), SiegeGameMode.h SandboxStartingGold 9999.
    (9) Comment hygiene: update the ASiegePlayerState class doc block (h:42-45 "Gold starts at 50 ...
    base GoldPerTick (2/s)") and the stale cpp comments (~148 "base 2/s, doubling to 4/s", ~308 "lands on
    the base 2/s"). COMMENT-ONLY corrections in SiegeGameMode.cpp for now-false lines (~106 "StartingGold =
    50", ~579 "base 2/s", ~867 "+2/s economy") — zero code changes in that file.
    (10) CONVENTIONS shadow law (C4457/58/59): no local/param may shadow an inherited reflected UPROPERTY
    (the new tick counter especially). QA MUST scan pre-compile.
    Acceptance (by inspection, pre-compile): base drip = exactly +1 per 2 ticks pre-overtime and +2 per
    2 ticks (1/s) in overtime; miner/DeepMine accrual still every 1 s tick at unchanged values; starting
    gold 10 at boot AND after Play Again; one SetGold per tick; delegates byte-identical; GetGoldRate
    display semantics documented; nothing DT_Cards-owned hardcoded (nothing here is table territory —
    ruling 1). handoffs/TASK-089.md MUST document the implementation choice (every-Nth-tick vs interval
    change, per ruling 2), the counter-reset placement, and the display-rounding rule for the HUD. Post in
    ⚙️ Dev & QA.
- names: >
    ASiegePlayerState (Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h/.cpp) — UPROPERTYs:
    StartingGold (50→10, h:259), Gold (50→10, h:313), GoldPerTick (2→1, h:263), NEW BaseIncomeTickPeriod
    (int32 = 2), unchanged GoldTickInterval (1.0f, h:267) / OvertimeIncomeMultiplier (2, h:279) /
    MinerGoldPerTick (1, h:283) / MaxGold (999, h:271). Functions: HandleGoldTick, GetGoldRate,
    RefreshGoldRate, ResetEconomy, ResetGold, SetGold. Delegates (BYTE-IDENTICAL): FOnGoldChanged,
    FOnGoldRateChanged. Comment-only: SiegeGameMode.cpp (~106/~579/~867). READ-ONLY context: DeepMine.h
    (DeepMineIncome 2), SiegeGameMode.h (SandboxStartingGold 9999). Law: M2 ruling "mechanic rules =
    UPROPERTY defaults with GDD § comments" + CONVENTIONS shadow law.

#### TASK-090 — Balance integration: bounce-window residue chores, compile, PIE economy verify + bot re-measure, commit (build-master)
- assignee: build-master
- status: done (2026-07-08: committed on main — hash in build-master report + 🔧 Build & Git — NOT pushed. Compile PASS clean 17.4s; CDO live-verified (10/1/2/1.0/2/999). Residue per ruling 9: WBP_MainMenu restored editor-down + both menu buttons readback-bound + restore HELD on disk through 3 PIE sessions (9a fallback NOT triggered; in-memory dirty flag only — do not save-all on this editor session); BP_Unit_Footman/Archer committed knowingly (9b chore). CF-7 proofs: SiegeGameMode.cpp diff comment-only ×3 hunks, delegate DECLAREs absent from diff. PIE economy: seed EXACTLY 10 (gold=10+floor((clock−0.5)/2) fits every sample, 3 sessions); base +1 per exactly 2.000s, zero drift over a full 758s match (end gold 557 = predicted 557 incl. OT segment); HUD +1/s pre-OT recorded as EXPECTED (ruling 4); OT flip 420.1s log fired once, first post-flip grant +2 at 420.5s (live latch on grant tick), 1 gold/s exact double; match-end income freeze proven twice; log sweep zero NEW lines (victory-focus ×3, nav known, lifted-castle MoveToActor burst all expected; DeepMine CardType-2 notably ABSENT this session). BALANCE LEDGER: undefended Blue castle dies ~56.5s / ~71.3s (2 runs; bot draw variance) vs ~33s TASK-088 — survival ≈ doubled; bot played ZERO early Miners both rush matches (played 2 late in the long match) — bot spend-mix note for next balance pass. FINDING (pre-existing, NOT this chain): HUD OVERTIME indicator never shows — WBP_HUD:ShowOvertime calls UpdateOvertimeDisplay(false) hardcoded-false pin (bound via SetupStatTexts CreateEvent; UpdateOvertimeDisplay itself correct) → manager: 1-pin UMG fix + shortened-threshold verify. WATCH (human, one PIE session, folded per TASK-076 doctrine — desktop was in active human use all session, injection suspended; Slack ask unanswered in time box): Play Again reset (gold→10, first grant ~2s), Miner rate text +2/s, + TASK-081 grey-tint leftover. handoffs/TASK-090.md)
- blocked-by: TASK-089 (qa-passed)
- parallel-safe: no (owns the single editor + the compile + the Git commit)
- spec: >
    Integration for the gold-balance chain, TASK-076 pattern, plus the standing bounce-window chores
    (ruling 9 — this is the first editor bounce since they were queued; do ALL residue work while the
    editor is DOWN, it holds streaming locks when up).
    (1) EDITOR DOWN — residue chores first: (a) `git restore Content/UI/WBP_MainMenu.uasset` (standing
    TASK-081/085 adjudication: benign phase-1 boot-resave, restore at bounce window); (b) leave the
    BP_Unit_Footman + BP_Unit_Archer .uasset deltas IN PLACE — they are committed knowingly in step 5
    (TASK-088 residue note: benign reimport re-registration, saved by Jonathan 2026-07-08).
    (2) Compile TASK-089's C++ via the standard Build.bat (CLAUDE.md). Failure → append errors to
    qa/TASK-089-report.md and route back to gameplay-programmer (counts as a QA loop).
    (3) Relaunch the editor; structural check: WBP_MainMenu still loads with BOTH buttons bound post-restore
    (readback: Play-vs-Bot → StartMatch, Btn_Sandbox → StartSandboxMatch). If the fresh session re-dirties
    WBP_MainMenu, record it and apply ruling 9a's fallback (commit knowingly next window — stop chasing).
    (4) PIE economy verification on direct-boot L_Arena (real match path — the SandboxStartingGold grant
    fires only via StartSandboxMatch, TASK-071): (a) gold seeds at 10 (HUD + readback); (b) base drip:
    +1 gold exactly every 2 s over a ≥10 s observation window, no drift, one OnGoldChanged per grant tick;
    (c) HUD rate text reads "+1/s" pre-overtime — EXPECTED per the ruling-4 display law (round-up average),
    record it, do NOT flag as a bug; (d) play a Miner: after arrival, accrual gains +1 per 1 s tick and the
    rate shows +2/s (miner income unchanged); (e) overtime at 7:00: base becomes +2 per 2 s (= 1/s) and the
    overtime indicator fires — use the TASK-081 lifted-castle harness to keep the match alive to 7:00 if
    the bot ends it sooner; (f) Play Again: gold resets to 10, base cadence restarts deterministically,
    match-end income freeze unregressed; (g) no new log errors/warnings (knowns: DeepMine CardType-2,
    victory-focus error, RecastNavMesh boot warning).
    (5) BALANCE LEDGER (ruling 7): re-measure the undefended-Blue-castle kill time vs the bot under the new
    economy (prior marks ~48 s TASK-076, ~33 s TASK-088); record the number in handoffs/TASK-090.md and in
    the 🔧 Build & Git post — Jonathan reads it for the next balance pass.
    (6) ONE commit to main with TASK-089/090 in the message: SiegePlayerState.h/.cpp, SiegeGameMode.cpp
    (comment-only), Content/Blueprints/Units BP_Unit_Footman + BP_Unit_Archer .uassets (chore line, ruling
    9b), pipeline docs (board/handoffs/qa). WBP_MainMenu.uasset stays OUT (restored in step 1) unless the
    ruling-9a fallback triggered. **NOT pushed** (no remote push without Jonathan's explicit instruction).
    Post compile result + ledger number + commit hash in 🔧 Build & Git.
    Acceptance: clean compile; residue adjudicated per ruling 9 (restore/commit branches recorded); PIE
    economy checks (a)-(g) PASS; bot-rush re-measure recorded; ONE commit on main, not pushed.
- names: >
    Build target GitClaudeUnrealTestEditor (Build.bat per CLAUDE.md). Verify: ASiegePlayerState
    StartingGold=10 / GoldPerTick=1 / BaseIncomeTickPeriod=2 live in PIE; map /Game/Maps/L_Arena. Residue:
    Content/UI/WBP_MainMenu.uasset (restore), Content/Blueprints/Units/BP_Unit_Footman.uasset +
    BP_Unit_Archer.uasset (commit knowingly). Ledger: undefended-castle kill time. Commit to main only,
    not pushed.

### Card artwork on the hand UI (TASK-077..081) — Jonathan feature request 2026-07-07
Verbatim intent: "have the art agent generate artwork for all the cards and have it get displayed instead of just having the text you have for it." Scope = all 22 roster CardIDs (cards.csv rows): Footman, Archer, Knight, Miner, ArrowTower, Wall, MilitiaMob, Pikeman, Sapper, Cavalry, Longbowman, Cleric, Ogre, BombTower, BallistaTower, Barracks, DeepMine, Masons, SharpenedBlade, PlateArmor, SwiftBoots, WarBanner. Naming law added to CONVENTIONS.md "Card artwork (hand UI)" 2026-07-07 BEFORE task issue.

**Manager rulings (binding for this chain):**
1. **Face composition:** art + overlaid text, NOT art-instead-of-text — the art is the background layer of each card face; DisplayName + cost stay overlaid and legible (contrast strip/shadow allowed). Artwork contains NO baked-in text. Art images HitTestInvisible (clicks belong to the play/discard buttons, M1 WARN-4 posture).
2. **Data law:** the art reference is DT_Cards data — new FCardRow column `CardArt` (TSoftObjectPtr<UTexture2D>) + cards.csv column carrying the full object path. Unset/unresolvable ⇒ graceful text-only fallback (today's face), log once, never a crash. No widget-side CardID→texture mapping.
3. **BIE contract stays byte-identical:** OnHandSlotUpdated / OnNextCardUpdated / OnCardRefusedMessage signatures unchanged (backward compatible). Art is delivered via new null-safe BlueprintCallable resolver(s) on UCardHandWidget — BIE params stay float/int/bool/byte/FString; UObject RETURNS on BlueprintCallable are fine. Programmer's audit picks the exact seam; a versioned BIE is allowed ONLY if the audit proves it strictly cleaner, documented, with every UMG call-site updated in TASK-080.
4. **Sync load ruling:** 512² UI textures, ≤7 visible (6 slots + preview), loaded on hand refresh — LoadSynchronous is acceptable; no async streaming machinery for this feature.
5. **Art tier:** blockout-tier stylized acceptable (premium art = M7) but must read at ~150 px — one dominant subject, strong silhouette, distinct per-card color key, team-agnostic palette.
6. **Reimport law:** the new CSV column requires a DT_Cards reimport in the editor IMMEDIATELY after the TASK-079 compile and before ANY PIE (TASK-031 WARN-2 precedent). Owned by TASK-081 phase 1.
7. **Batching:** all 22 renders in ONE art task (internal batching, one handoff); the Unreal import is its own editor task so render work stays parallel-safe with C++ file work.
8. **WBP_CardHand ownership:** the widget edit is gameplay-programmer (TASK-041/072 precedent — the hand widget is BIE-contract-heavy); art-director owns textures only.
9. Pre-existing follow-ups (victory-widget focus error, DeepMine CardType-2 warning, bot-rush balance) are NOT folded in — logged, left for later.

Dispatch shape: **[TASK-077 art renders ∥ TASK-079 C++/CSV] → TASK-079 QA → TASK-081 phase 1 (compile + DT_Cards reimport) → TASK-078 import (editor-free slot any time after 077; may run before or after the phase-1 bounce) → TASK-080 UMG → TASK-081 phase 2 (PIE + commit).**

#### TASK-077 — Card artwork: render all 22 card illustrations to PNG (Blender)
- assignee: art-director
- status: done (2026-07-07: 22/22 PNGs at Content/RawAssets/CardArt/<CardID>.png, all 512×512 readback-verified, casing char-for-char vs cards.csv; single shared EEVEE studio rig, style-family consistent, 8 cards reworked for ~150px readability; donor-less cards (Masons + 4 upgrades) as iconographic props. handoffs/TASK-077-artist.md. Import = TASK-078; commit rides TASK-081 phase 2.)
- blocked-by: none (Blender MCP verified LIVE 2026-07-07)
- parallel-safe: yes (file-side only — Blender scene work + PNG writes to Content/RawAssets/; NO Unreal editor, NO Content/ .uasset mutation)
- spec: >
    Blender MCP work (repo bridge Tools/blender_mcp_bridge.py — execute_blender_code / get_scene_info /
    get_object_info). Produce ONE square card illustration per CardID, rendered to PNG at exactly 512×512,
    saved as Content/RawAssets/CardArt/<CardID>.png (CardID casing character-for-character), for ALL 22
    roster CardIDs: Footman, Archer, Knight, Miner, ArrowTower, Wall, MilitiaMob, Pikeman, Sapper, Cavalry,
    Longbowman, Cleric, Ogre, BombTower, BallistaTower, Barracks, DeepMine, Masons, SharpenedBlade,
    PlateArmor, SwiftBoots, WarBanner.
    Style law (CONVENTIONS "Card artwork (hand UI)"): blockout-tier stylized is acceptable (premium art is
    M7) but every card MUST read at hand-slot size (~150 px) — one dominant subject filling the frame, strong
    silhouette, high subject/background contrast, a DISTINCT color key per card so all 22 are tellable apart
    at a glance; team-agnostic palette (cards are player-neutral — avoid reading as Blue/Red team colors).
    NO text baked into the artwork (name/cost are overlaid by the widget, TASK-080).
    Subject guide (from cards.csv DisplayName/Notes): units = the unit figure (the project blockout FBX
    donors in Content/RawAssets/*.fbx MAY be imported into Blender scenes as staging donors — READ-ONLY,
    never modify or re-export them); buildings = the structure; Miner/DeepMine = gold/economy motifs;
    Masons = repair motif (trowel/wall); Barracks = the spawner building; hero upgrades = the item itself
    (sword blade / plate chest / boots / war banner).
    Internal batching at your discretion (reuse one camera + light rig, stage per card); ONE handoff for all
    22. Acceptance: 22 PNGs on disk under Content/RawAssets/CardArt/, exactly 512×512 each, named exactly
    <CardID>.png, each readable at 150 px; handoffs/TASK-077.md lists all 22 with a one-line content
    description each. Post progress/completion in 🎨 Art.
- names: >
    PNGs: Content/RawAssets/CardArt/<CardID>.png — the 22 CardIDs character-for-character from
    Docs/Data/cards.csv row names (list above). Future import targets (TASK-078, not this task):
    /Game/UI/CardArt/T_CardArt_<CardID>. Law: CONVENTIONS.md "Card artwork (hand UI)".

#### TASK-078 — Card artwork: import the 22 PNGs as UTexture2D (editor)
- assignee: art-director
- status: done (2026-07-07: 22/22 imported to /Game/UI/CardArt/T_CardArt_<CardID> + saved; TEXTUREGROUP_UI, sRGB on, 512×512 double-verified (transient 32×32 readings = async-texture-compile placeholder, settled pre-save); DT_Cards CardArt cells cross-checked 22/22 match; zero import warnings — the 22 LogCSVImportFactory 'Expected String, got Object' lines are TASK-081 phase-1 TSoftObjectPtr noise pre-dating these imports. Editor auto-staged the 22 .uassets; the 22 source PNGs remain untracked → BOTH belong to TASK-081 phase 2's selective commit. handoffs/TASK-078-artist.md)
- blocked-by: TASK-077
- parallel-safe: no (editor-mutating — single editor instance)
- spec: >
    Unreal MCP import work. Import each Content/RawAssets/CardArt/<CardID>.png as a UTexture2D at
    /Game/UI/CardArt/T_CardArt_<CardID> — all 22. Settings per CONVENTIONS "Card artwork (hand UI)":
    Texture Group = UI, sRGB on, default compression. Verify by readback that all 22 assets exist and are
    512×512; save all. NO other Content/ mutation (do not touch WBP_CardHand — that is TASK-080).
    Editor sequencing note for the orchestrator: this task only needs the editor UP; it may run before or
    after TASK-081's phase-1 compile bounce (imported .uassets survive the bounce), but never concurrently
    with another editor-mutating task. Acceptance: 22 T_CardArt_* assets under /Game/UI/CardArt/, each
    512×512, all saved; handoffs/TASK-078.md lists the 22 asset paths. Post in 🎨 Art.
- names: >
    /Game/UI/CardArt/T_CardArt_<CardID> (Content/UI/CardArt/) for the 22 CardIDs. Sources:
    Content/RawAssets/CardArt/<CardID>.png (TASK-077). Law: CONVENTIONS.md "Card artwork (hand UI)".

#### TASK-079 — Card-art data path: FCardRow.CardArt + cards.csv column + UCardHandWidget resolvers (C++)
- assignee: gameplay-programmer
- status: qa-passed (QA PASS 2026-07-07 — 0 blocker/1 warn(doc-only: handoff column arithmetic, corrected in report)/1 nit; all 8 flagged decisions ACCEPTED; BIEs byte-identical vs TASK-029 contract; CSV 23 data columns verified char-for-char, DeckCount still 50. qa/TASK-079-report.md. Carry-forwards recorded for TASK-080 wiring + TASK-081 reimport.) (2026-07-07. FCardRow.CardArt TSoftObjectPtr + 22 CSV rows + GetCardArtTexture/GetNextCardArtTexture via shared ResolveCardArtTexture; BIE signatures byte-identical; 8 flagged decisions; shadow-scan clean. handoffs/TASK-079.md)
- blocked-by: none
- parallel-safe: yes (file-only: CardRow.h, CardHandWidget.h/.cpp, Docs/Data/cards.csv — no file overlap with TASK-077/082/083)
- spec: >
    Files only, no editor, no compile.
    (1) FCardRow (CardRow.h): add UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
    TSoftObjectPtr<UTexture2D> CardArt; doc comment: hand-UI card illustration (CONVENTIONS "Card artwork
    (hand UI)"); unset = text-only face. Property name MUST match the CSV header exactly (reimport law).
    (2) Docs/Data/cards.csv: append a CardArt column to the header and ALL 22 rows; each cell = the FULL
    object path /Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID> (e.g. Footman →
    /Game/UI/CardArt/T_CardArt_Footman.T_CardArt_Footman). No blank cells — the 22 textures are being
    produced in TASK-077/078.
    (3) UCardHandWidget (CardHandWidget.h/.cpp): expose card art to the runtime-built UMG tree WITHOUT
    breaking the BIE contract — the three BIEs (OnHandSlotUpdated / OnNextCardUpdated / OnCardRefusedMessage)
    keep byte-identical signatures (ruling 3). Preferred seam (audit may refine): a null-safe
    UFUNCTION(BlueprintCallable) UTexture2D* GetCardArtTexture(const FString& CardID) — CardID → DT_Cards
    row (soft table resolved null-safe at use time, as the existing pushes do) → CardArt.LoadSynchronous();
    returns nullptr on empty CardID / missing row / unset or unresolvable path (log once, never crash).
    ALSO provide preview art access (OnNextCardUpdated carries no CardID): preferred = cache the last pushed
    next CardID and expose UTexture2D* GetNextCardArtTexture(); audit picks the exact shape, document in the
    handoff. If the audit PROVES a versioned BIE is strictly cleaner, it is a deliberate version bump —
    documented, with TASK-080 updating every UMG call-site; default expectation is NO BIE change.
    (4) LoadSynchronous is ACCEPTED for this feature (ruling 4) — note it in a comment.
    (5) CONVENTIONS shadow law (C4457/58/59): no local/param may shadow an inherited reflected UPROPERTY.
    QA MUST scan pre-compile.
    Acceptance: CSV header ↔ UPROPERTY 1:1 (TASK-081's DT_Cards reimport must add zero NEW warnings — the
    known DeepMine CardType-2 warning is pre-existing/watched); exactly 22 CardArt cells with exact paths;
    resolvers null-safe by inspection; BIE signatures untouched (or deliberately versioned per audit);
    nothing hardcoded that belongs in DT_Cards. handoffs/TASK-079.md documents the chosen seam for TASK-080.
    Post in ⚙️ Dev & QA.
- names: >
    FCardRow::CardArt (TSoftObjectPtr<UTexture2D>) — Source/GitClaudeUnrealTest/Siegebound/CardRow.h.
    Docs/Data/cards.csv column CardArt; cells /Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>.
    UCardHandWidget::GetCardArtTexture / ::GetNextCardArtTexture —
    Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h/.cpp. Table: /Game/Data/DT_Cards. BIEs
    (unchanged): OnHandSlotUpdated, OnNextCardUpdated, OnCardRefusedMessage. Law: CONVENTIONS.md
    "Card artwork (hand UI)" + FCardRow column registry.

#### TASK-080 — WBP_CardHand: art on the 6 card faces + next-card preview (editor/UMG)
- assignee: gameplay-programmer
- status: done (2026-07-07: Img_CardArt per slot + Img_NextCardArt as face-background overlays (Collapsed when resolver returns null), affordability tint white/grey-0.35, text shadows for legibility; BuildHandTree extended, EventGraph spliced granularly (2 nodes), all 12 AssignOnClicked bindings readback-intact; widget compiles clean, saved. PIE: art on all 6 faces + preview, correct per CardID, zero new warnings; human-input items (Alt-click, grey-on-spend, refusal overlay) → TASK-081 ph2 WATCH. MCP law learned: class-ambiguous Appearance|SetBrushFromTexture DSL ids need granular create_node with declaring_class. handoffs/TASK-080.md)
- blocked-by: TASK-078 (textures in Content), TASK-079 (qa-passed AND compiled via TASK-081 phase 1 — the resolvers must be callable in the live module; the phase-1 DT_Cards reimport must also be done so rows carry CardArt before PIE verification)
- parallel-safe: no (editor-mutating — edits WBP_CardHand; single editor instance)
- spec: >
    Additive MCP UMG in /Game/UI/WBP_CardHand (parent UCardHandWidget). Extend the TASK-041 runtime
    BuildHandTree construction: per hand slot add a UImage named Img_CardArt (runtime-created per slot)
    layered UNDER the DisplayName + cost texts — art is the face background; text stays overlaid and
    legible (add a translucent dark strip/shadow behind the text if contrast needs it). Add Img_NextCardArt
    to the preview slot. All art images HitTestInvisible (clicks must still land on play/discard buttons —
    WARN-4 posture).
    Wiring: in the OnHandSlotUpdated handler path call GetCardArtTexture(CardID) — non-null →
    SetBrushFromTexture + show; null or empty CardID → hide the image (text-only fallback = today's face).
    Affordability: when bAffordable is false, tint the art grey (SetColorAndOpacity ≈ (0.35,0.35,0.35))
    alongside the existing SetIsEnabled greying; restore white when affordable. Preview: in the
    OnNextCardUpdated handler call GetNextCardArtTexture(); empty DisplayName → hide preview art too.
    (Use the exact resolver names/seam from handoffs/TASK-079.md.)
    PRESERVE (readback + PIE): never round-trip the protected M1 gold Construct (TASK-033/041 law); refusal
    message ~2 s show/hide; play/discard buttons + hotkeys; root SelfHitTestInvisible posture; Rally + M4
    upgrade rows; InitForController path. Verify in PIE (after the phase-1 DT_Cards reimport): all 6 faces
    show art matching their CardID, preview shows art, grey-tint tracks affordability, empty slots hide art,
    play/discard/refusal unregressed, no new log errors. Acceptance as above; handoffs/TASK-080.md records
    the widget names + wiring. Post in ⚙️ Dev & QA.
- names: >
    /Game/UI/WBP_CardHand (parent UCardHandWidget). New runtime-created widgets: Img_CardArt (one per hand
    slot), Img_NextCardArt (preview). Calls: UCardHandWidget::GetCardArtTexture /
    ::GetNextCardArtTexture (TASK-079 handoff is authoritative on exact names). Textures:
    /Game/UI/CardArt/T_CardArt_<CardID> (TASK-078). BIEs unchanged: OnHandSlotUpdated / OnNextCardUpdated /
    OnCardRefusedMessage. Law: CONVENTIONS.md "Card artwork (hand UI)".

#### TASK-081 — Card-art integration: compile, DT_Cards reimport, editor wave, PIE, commit (build-master)
- assignee: build-master
- status: done (2026-07-08 phase 2: import toast dismissed with Don't Import (OS-level click; zero accidental imports — Content/RawAssets/CardArt still 22 PNGs, no .uasset). PIE regression PASS on direct-boot L_Arena, 3 sessions: art per CardID on all 6 slots + preview LIVE-verified across 2 different shuffles — 14 distinct CardIDs matched to their art incl. a duplicate-Archer pair rendering identically; HOTKEY PLAY LIVE-verified via OS input injection (pressed '1': SharpenedBlade played, slot-0 art swapped to BallistaTower = the predicted preview card, preview advanced to Wall, discard pile +1, Blade 1/2 upgrade HUD row appeared) — user32 SendInput injection WORKS for game hotkeys, superseding the MCP-only "not machine-drivable" precedent (Alt-cursor UI clicks still did not register → stays human WATCH); gold/rate/miner/Rally HUD unregressed; match loop sane (bot plays logged, castle to 0, Defeat + Play Again, income freeze at defeat, Play Again re-deals 44/6/0 + gold 50); zero NEW errors/warnings — knowns fired as expected (DeepMine CardType-2 ×1, victory-focus error ×3 at defeats, RecastNavMesh boot warning; MoveToActor failures were artifacts of the test harness lifting Castle_0 to Z=4000 to keep the match alive). Grey-tint = structural only (agent gold too high; DT cost-bump route denied by permission system) → WATCH. Residue adjudicated: WBP_MainMenu.uasset resave delta (new oid +1.8KB, phase-1 bounce) EXCLUDED from commit, left in worktree for the next bounce window. Chain committed to main in ONE commit (hash in orchestrator report + 🔧 Build & Git), NOT pushed. Phase 1 record: compile PASS clean 20.4s, editor UP PID 34120, DT_Cards reference-safe set_rows, CardArt 22/22 char-for-char, zero NEW warnings; TOOLING LAW: DataTableTools set_rows silently nulls soft-object cells passed as {"refPath":...} objects — use plain string paths + readback-verify.)
- blocked-by: TASK-079 (qa-passed) for phase 1; TASK-077 + TASK-078 + TASK-080 for phase 2
- parallel-safe: no (owns the single editor + the compile + the Git commit)
- spec: >
    Two-phase integration (TASK-073/076 pattern).
    PHASE 1 — after TASK-079 qa-passed: editor bounce + Build.bat compile of the TASK-079 C++ (failure →
    append errors to qa/TASK-079-report.md, route back to gameplay-programmer; counts as a QA loop).
    IMMEDIATELY after the compiled editor is up: reimport /Game/Data/DT_Cards from Docs/Data/cards.csv
    BEFORE any PIE (TASK-031 WARN-2 law). Expect zero NEW warnings; the known DeepMine CardType-2 warning is
    pre-existing (TASK-035 watch) — record if it fires, do not treat as new. Then hand back to the
    orchestrator so TASK-078 (if not already done) and TASK-080 run against the live module.
    PHASE 2 — after TASK-080: full PIE regression on direct-boot L_Arena (menu path not machine-drivable,
    TASK-073 precedent): 6 hand faces show art matching their CardIDs + preview art (spot-check ≥4 distinct
    cards across plays/discards/redraws — the M4 test deck surfaces variety); grey-tint on unaffordable;
    empty slot hides art; text overlays legible; play/discard/refusal/Alt-cursor/hotkeys 1–6 and the M1
    gold counter unregressed; no new log errors (a failed soft-load would log).
    COMMIT — everything in ONE commit to main with TASK-077..081 in the message: CardRow.h,
    CardHandWidget.h/.cpp, Docs/Data/cards.csv, Content/RawAssets/CardArt/*.png (22),
    Content/UI/CardArt/*.uasset (22), WBP_CardHand.uasset, DT_Cards.uasset, pipeline docs. NOT pushed (no
    remote push without Jonathan's explicit instruction). Post compile result + commit hash in 🔧 Build & Git.
    Acceptance: clean compile; reimport clean (no new warnings); PIE regression PASS with art live on the
    hand; committed to main, not pushed.
- names: >
    Build target GitClaudeUnrealTestEditor (Build.bat per CLAUDE.md). Reimport: /Game/Data/DT_Cards ←
    Docs/Data/cards.csv. Verify: /Game/UI/CardArt/T_CardArt_<CardID> (22), /Game/UI/WBP_CardHand faces,
    UCardHandWidget resolvers. Map: /Game/Maps/L_Arena. Commit to main only, not pushed.

### TRELLIS.2 → Blender → UE5 art pipeline + Fab lane (TASK-082..088) — Jonathan-approved plan 2026-07-07
Approved plan: C:\Users\wesel\.claude\plans\swirling-plotting-globe.md (Jonathan's Obsidian pipeline note, operationalized + plan-mode approved). Goal: automated generate→refine→import pipeline producing game-ready textured meshes. Pilot scope = **SM_Footman first, then SM_Archer + SM_Castle** (Jonathan's 2026-07-04 fidelity request, pulled forward from M7); the remaining 16 blockout meshes reuse this pipeline as the M7 template. Tooling + art fidelity only — NOT GDD content; **M5 remains NOT authorized.** Naming law added to CONVENTIONS.md "Textured mesh law (TRELLIS.2 art pipeline)" 2026-07-07 BEFORE task issue. Fab protocol file: .claude/pipeline/fab/FAB-REQUESTS.md.

**Manager rulings (binding for this chain):**
1. **HF_TOKEN is ENV-ONLY** — read from the environment at runtime; NEVER written to any file, never passed on argv, never echoed/logged. trellis_generate.py exits code 2 with a clean message if unset. guard-secrets hook gains the `hf_` pattern (TASK-083). QA audits every leakage path.
2. **Tools/**/*.py is CODE** — the full pre-commit QA gate applies (agent definitions updated 2026-07-07: qa-reviewer checklist adds secret handling, network timeouts, write confinement, headless-bpy context pitfalls).
3. **Heavy Blender work runs HEADLESS** via Bash (`blender.exe --background --python refine_trellis_glb.py`) — the live Blender MCP bridge has a 30 s socket cap; MCP is for <30 s inspection/preview only.
4. **Lane isolation:** the card-art chain (TASK-077..081) owns Content/RawAssets/CardArt/ + /Game/UI/CardArt/ — this chain NEVER writes there. This chain owns Tools/ArtPipeline/**, Content/RawAssets/<AssetName>.fbx + RawAssets/Textures/ + RawAssets/Concepts/, /Game/Textures/T_<AssetName>_*, /Game/Materials/M_AssetPBR + MI_<AssetName>_PBR.
5. **Two-slot material contract requires ZERO C++ changes** (slot-0 TeamRegion recolor keeps working — SummonedUnit.cpp:146 / Building.cpp:80 / Castle.cpp:125; placement ghost tints all slots). Any discovered need for a C++ change = STOP + escalate, not improvise.
6. **Nanite OFF** on pipeline meshes.
7. **Same-path SM overwrite** is the swap mechanism (non-breaking; validated FIRST on SM_Footman). Contingencies in order: console `Obj Reimport` → Jonathan one-click import (escalate via 🚨 Blockers). NEVER delete+recreate the SM asset.
8. **ZeroGPU quota is an expected pause, not a blocker:** free tier ≈ 5 GPU-min/day ≈ 1–2 assets/day. If quota blocks a generate, record the reset time in the handoff and resume next window (recommend HF PRO to Jonathan before the M7 batch); escalate only if stuck >48 h.
9. **Editor-mutating imports serialize** as always (single editor). Stage 1 (generate) + Stage 2 (headless refine) are file-side and may overlap other file work.
10. **Fab lane is request-only:** art-director AUTHORS FAB-### entries in .claude/pipeline/fab/FAB-REQUESTS.md; Jonathan approves/fulfills via the Epic Launcher (human-only); Content/Fab/<Pack>/ is read-only donor quarantine. No Fab task is issued in this chain — the lane is standing infrastructure.

Dispatch shape: **[TASK-082 ∥ TASK-083] (files → QA) → TASK-084 (smoke + commit tooling) → TASK-085 (EXTERNAL GATE: Jonathan) → TASK-086 (Footman pilot end-to-end) → TASK-087 (Archer + Castle; Stage-1/2 may pre-run after 084+085) → TASK-088 (PIE + commit) → Jonathan visual sign-off (WATCH).**

#### TASK-082 — Trellis Stage-1 tooling: Tools/ArtPipeline scaffold + trellis_generate.py (files)
- assignee: gameplay-programmer
- status: qa-passed (QA-loop 2 PASS 2026-07-07 — 0 blocker/1 warn/2 nit; fix = Client(token=) at line 460 + offline --check signature assert (lines 356-374, pre-network, tokenless); exit-1-vs-4 ruling APPROVED with stderr disambiguation law: `gradio_client kwarg drift:` = route-back, `Space unreachable:` = transient. WARN-L2-A: QA's git snapshot looked stale — orchestrator re-verified git diff = exactly trellis_generate.py; build-master re-confirms scope at the next Tools/ commit. Loop-2 record in qa/TASK-082-report.md + handoffs/TASK-082.md. LIVE DRIFT HISTORY: gradio_client 2.5.0 renamed Client(hf_token=)→token=, caught at TASK-086 Stage 1, TypeError pre-network; original --check structurally couldn't catch it.) — prior: qa-passed (QA PASS 2026-07-07 — 0 blocker/2 warn/5 nit; security audit CLEAN (token redaction, write confinement); all 11 flags approved. qa/TASK-082-report.md. WARN-1 + WARN-2 hardening APPLIED + verified 2026-07-07 (usage-error path redacts — proven with fake-token argv, exit 64 shows [hf-token-redacted]; failures now write state_failed.json, state.json = last success only; py_compile/--help/exit-2 re-verified; "Post-QA hardening" section in handoffs/TASK-082.md) — 082 clear for the tooling commit; TASK-084 MUST gitignore Tools/ArtPipeline/.venv/ BEFORE the tooling commit — 700+ untracked files, orchestrator-flagged URGENT.) (2026-07-07. uv env on managed CPython 3.12.13, gradio-client 2.5.0; trellis_generate.py with token redactor, atomic Client session, view_api assert + schema snapshot, quota exit 3 / token-unset exit 2 / usage exit 64; --check deferred to TASK-084 network smoke per spec. handoffs/TASK-082.md)
- blocked-by: none
- parallel-safe: yes (new files only, under Tools/ArtPipeline/ — no overlap with TASK-077/079/083)
- spec: >
    Files only — author, do not run (network/GPU smoke tests are TASK-084's). This is dev tooling
    (CONVENTIONS "Textured mesh law", tooling law), not gameplay code.
    (1) uv project scaffold at Tools/ArtPipeline/: pyproject.toml pinned to Python 3.12 (gradio_client is
    NOT validated on 3.14), .python-version, uv.lock, README.md (the three stage commands, concept-image
    guidance for Jonathan, fallback procedures — incl. the manual fallback: Jonathan browser-runs the HF
    Space and drops the GLB at Cache/<AssetName>/trellis_raw.glb; the pipeline resumes at Stage 2).
    Deps: gradio_client, pillow.
    (2) trellis_generate.py CLI (Stage 1): HF_TOKEN from env ONLY (ruling 1 — exit code 2 + clean message
    if unset; the token must never appear in files, argv, logs, or exception text); gradio_client against
    the official HF Space microsoft/TRELLIS.2; view_api() discovery + assert of the three endpoints
    (/preprocess_image → /image_to_3d → /extract_glb), writing an api_schema.json snapshot beside the
    output; the preprocess→generate→extract sequence runs atomically on ONE Client (gr.State is
    per-Client-session — never split across runs); timeouts ≥20 min per GPU call; surface ZeroGPU
    quota-exceeded messages verbatim incl. reset time (ruling 8); writes Cache/<AssetName>/trellis_raw.glb
    + state.json; a --check flag = TOKENLESS smoke test (Space reachability + endpoint schema assert only,
    no GPU call, no token needed).
    (3) Create Tools/ArtPipeline/Inbox/ + Cache/ as working dirs (e.g. .gitkeep) — the .gitignore entries
    land in TASK-084.
    Acceptance: files as specified; by inspection the token cannot reach disk/argv/logs; --check runs
    tokenless; QA gate per ruling 2. handoffs/TASK-082.md. Post in ⚙️ Dev & QA.
- names: >
    Tools/ArtPipeline/pyproject.toml, .python-version, uv.lock, README.md, trellis_generate.py, Inbox/,
    Cache/. HF Space: microsoft/TRELLIS.2 (gradio_client). Env var: HF_TOKEN (env-only law). Outputs:
    Tools/ArtPipeline/Cache/<AssetName>/trellis_raw.glb + state.json + api_schema.json. Law: CONVENTIONS.md
    "Textured mesh law (TRELLIS.2 art pipeline)".

#### TASK-083 — Trellis Stage-2 tooling: refine_trellis_glb.py + pipeline_manifest.json + guard-secrets hf_ pattern (files)
- assignee: gameplay-programmer
- status: qa-passed (QA PASS 2026-07-07 — 0 blocker/4 warn/4 nit; all 14 flags ACCEPTED; axis contract verbatim, castle UCX geometry-checked wall-footprint-exact, write confinement real code, hf_ pattern correctly placed. qa/TASK-083-report.md. WARNs are runtime-verifiable → TASK-084 carry-forward checklist: Footman smoke MUST assert exactly-2-materials-in-order in the smoke FBX (WARN-3), Castle UCX smoke recommended, guard-secrets pipe-test with fake hf_ token.) (2026-07-07. Headless refine with write-confinement guard, native fallback, --smoke/--quick; manifest pilot rows Footman/Archer/Castle incl. 9-hull wall-footprint UCX. handoffs/TASK-083.md)
- blocked-by: none
- parallel-safe: yes (disjoint files from TASK-082: refine_trellis_glb.py, pipeline_manifest.json, .claude/hooks/guard-secrets.sh — no overlap with TASK-077/079)
- spec: >
    Files only — author, do not run (headless round-trip smoke is TASK-084's).
    (1) refine_trellis_glb.py — HEADLESS bpy script (ruling 3: runs via blender.exe --background --python;
    must never require UI context — QA checks for context-dependent bpy calls). Pipeline per asset manifest:
    import Cache/<AssetName>/trellis_raw.glb → cleanup (loose geo, doubles) → voxel-remesh + decimate to
    the manifest tri budget → Smart-UV project into a UV layer named exactly "UVMap" → Cycles CPU bake
    D/N/ORM (mind the metallic-via-EMIT-rewire gotcha for the metallic pass) → two-slot split per
    CONVENTIONS (slot 0 TeamRegion minority face-set from manifest selectors, slot 1 <AssetName>PBR) →
    UCX collision authoring for buildings (UCX_SM_Castle wall-footprint-exact — plinth dead-zone lesson;
    bounds within ±10% of the blockout) → FBX export to Content/RawAssets/<AssetName>.fbx with the axis
    contract (axis_forward='-Z', axis_up='Y', apply_unit_scale, FACE smoothing; units feet-center origin,
    buildings ground-center) → texture PNGs to Content/RawAssets/Textures/<AssetName>/T_<AssetName>_D|_N|
    _ORM.png → Workbench/Cycles preview renders + refine_report.json (tris/bounds/UV-layer/PNG inventory)
    to Cache/<AssetName>/. Modes: "bake" (standard) and "native" fallback (keep TRELLIS's own
    mesh/UVs/textures — the escape hatch for bake artifacts).
    (2) pipeline_manifest.json — per-asset entries for Footman, Archer, Castle: tri budgets (units ≤15k,
    castle ≤40k), target dims from the blockout handoffs, team-region selectors, bake sizes (1024² units,
    2048² buildings), mode.
    (3) guard-secrets: add the hf_[A-Za-z0-9]{20,} token pattern to .claude/hooks/guard-secrets.sh
    (micro-edit; do not disturb existing patterns).
    Acceptance: script headless-safe by inspection; ALL writes confined to Tools/ArtPipeline/Cache/ +
    Content/RawAssets/ (QA verifies write confinement — and NEVER Content/RawAssets/CardArt/, ruling 4);
    refine_report.json complete; manifest carries the three pilot assets; hook pattern added. QA gate per
    ruling 2. handoffs/TASK-083.md. Post in ⚙️ Dev & QA.
- names: >
    Tools/ArtPipeline/refine_trellis_glb.py, Tools/ArtPipeline/pipeline_manifest.json,
    .claude/hooks/guard-secrets.sh (pattern hf_[A-Za-z0-9]{20,}). Outputs: Content/RawAssets/<AssetName>.fbx
    (existing blockout paths: Footman.fbx, Archer.fbx, Castle.fbx),
    Content/RawAssets/Textures/<AssetName>/T_<AssetName>_D|_N|_ORM.png, Cache/<AssetName>/refine_report.json
    + previews. UV layer: "UVMap". Slot names: TeamRegion / <AssetName>PBR. Collision: UCX_SM_Castle.
    Law: CONVENTIONS.md "Textured mesh law (TRELLIS.2 art pipeline)".

#### TASK-084 — Trellis tooling integration: smoke tests, .gitignore, commit (build-master)
- assignee: build-master
- status: done (2026-07-07: ALL smokes PASS. (a) `--check` exit 0 — 3 endpoints schema-OK, snapshot to gitignored Cache/api_schema.json; NOTE required an environmental TLS workaround: Norton AV MITMs HTTPS (cert issuer "Norton Web/Mail Shield Root", in Windows store but NOT certifi) → ran with SSL_CERT_FILE=certifi+Norton bundle. CARRY-FORWARD TASK-085/086: Stage-1 live runs need the same SSL_CERT_FILE bundle OR a Norton exclusion for huggingface.co/*.hf.space — raw exit-1 "CERTIFICATE_VERIFY_FAILED" otherwise; not a code bug, not API drift. (b) Footman round-trip exit 0 in 2.8s — 15000 tris on budget, bounds/min-Z/UVMap OK, report complete, zero warnings; WARN-3 CLOSED: re-imported smoke FBX carries exactly 2 slots in order [TeamRegion, FootmanPBR] (donor exercised a 21.2% live selector match, not the zero-face branch); D/N/ORM are real PNGs. (c) Castle round-trip exit 0 — 40000 tris, 9 UCX hulls UCX_SM_Castle_00..08 in the FBX, side-wall hull exactly 100×820×300 UE (create_cube full-edge semantics confirmed), no slab; donor FBX checksums unchanged (write confinement proven). (d) guard-secrets pipe tests: fake hf_+27-alnum → deny JSON; benign → silence; line-12 hf_ pattern visually confirmed (file untracked pre-commit, no diff possible). .gitignore hardened BEFORE any git add: Tools/ArtPipeline/.venv/ + Inbox/* + Cache/* ignored, .gitkeeps kept via negations. Tooling committed to main (hash in orchestrator report + 🔧 Build & Git), NOT pushed; card-art-chain files excluded per lane isolation — they ride TASK-081 phase 2.)
- blocked-by: TASK-082 (qa-passed), TASK-083 (qa-passed)
- parallel-safe: no (owns Git; runs Bash smoke tests; serialize with any other build-master work — no UE compile needed, this chain has no C++)
- spec: >
    (1) `uv sync` in Tools/ArtPipeline (creates the pinned 3.12 env). (2) Run `uv run trellis_generate.py
    --check` — TOKENLESS smoke: HF Space reachable + the three endpoints match the schema assert. An assert
    failure = API drift: append to qa/TASK-082-report.md and route back (counts as a QA loop). Do NOT run a
    real generation (no token, no GPU quota spend). (3) Headless Blender round-trip smoke: run
    refine_trellis_glb.py via blender.exe --background against an EXISTING blockout FBX/GLB in a smoke mode
    that writes ONLY to Cache/ (must NOT overwrite any shipping Content/RawAssets FBX) — validates the
    headless bpy environment, write confinement, and refine_report.json generation. (4) Add
    Tools/ArtPipeline/Inbox/ + Tools/ArtPipeline/Cache/ to .gitignore. (5) Commit the tooling to main
    (Tools/ArtPipeline/**, guard-secrets.sh, .gitignore, pipeline docs incl. FAB-REQUESTS.md + CONVENTIONS +
    agent-def updates + this board update) with TASK-082..084 in the message. NOT pushed. Post smoke results
    + commit hash in 🔧 Build & Git.
    Acceptance: --check PASS; headless round-trip PASS with a complete report and zero writes outside
    Cache/; working dirs gitignored; committed to main, not pushed.
- names: >
    Tools/ArtPipeline/** (TASK-082/083 files), .gitignore (Inbox/ + Cache/ entries),
    .claude/pipeline/fab/FAB-REQUESTS.md. Smoke donor: any existing Content/RawAssets/*.fbx (read-only).
    Commit to main only, not pushed.

#### TASK-085 — EXTERNAL GATE: HF_TOKEN + pilot concept images (Jonathan — not an agent task)
- assignee: Jonathan (external gate — board-recorded; orchestrator posts the ask in 🚨 Blockers and flips this when satisfied)
- status: done (SATISFIED 2026-07-07, orchestrator-verified: (1) HF_TOKEN set at user scope — presence/length/hf_-prefix checked, value never read into output; NOTE it was set mid-session, so shells only inherit it after a terminal restart — Jonathan is restarting; verify `$env:HF_TOKEN` is visible before dispatching TASK-086. (2) Norton ruling: Jonathan added SSL-scanning exclusions for huggingface.co/*.hf.space — TASK-086 runs WITHOUT SSL_CERT_FILE first; the TASK-084 cert-bundle workaround is the documented fallback if TLS still fails. (3) Inbox verified: Castle.png/Footman.png/Archer.png present, exact casing. (4) Jonathan upgraded to HF PRO 2026-07-07 — 40 ZeroGPU-min/day + top queue priority on the existing token; quota is NOT a scheduling constraint anymore: TASK-086/087 generations + re-rolls can run same-day, and the M7 16-mesh batch is feasible in 1-2 days. RESUME NOTE for next session: dispatch TASK-086 (SM_Footman end-to-end pilot) on Jonathan's go; also queue manager follow-ups: A-pose-for-units line in the CONVENTIONS concept-image guidance (skeletal-mesh readiness for M7), WBP_MainMenu resave-residue restore at next bounce window, post-defeat castle-VFX linger, bot-rush balance.)
- blocked-by: none (may be satisfied any time; TASK-086 requires BOTH this and TASK-084)
- parallel-safe: yes (human action, no repo mutation by agents)
- spec: >
    Jonathan: (1) set the user environment variable HF_TOKEN to your Hugging Face token (env-only law —
    agents never read it aloud, never store it; new shells/sessions pick it up). Free ZeroGPU ≈ 5 GPU-min/day
    ≈ 1–2 assets/day; HF PRO ($9/mo, 40 min/day) recommended before the M7 16-mesh batch, optional for the
    3-asset pilot. (2) Drop three concept PNGs in Tools/ArtPipeline/Inbox/: Footman.png, Archer.png,
    Castle.png (guidance in Tools/ArtPipeline/README.md — single subject, neutral background, ¾ view works
    best). Gate is satisfied when the orchestrator confirms the env var EXISTS (existence check only — never
    echo the value) and the three PNGs are present. Record satisfaction here + in 🚨 Blockers.
- names: >
    Env var: HF_TOKEN (user-level). Files: Tools/ArtPipeline/Inbox/Footman.png, Archer.png, Castle.png.

#### TASK-086 — Pilot asset: SM_Footman end-to-end + M_AssetPBR master authoring (art)
- assignee: art-director
- status: done (2026-07-07 late, completed same evening. FINAL OVERWRITE VERDICT (ruling 7): validated mechanism = human Content-Browser Reimport click — no file prompt (Stage 2's same-path FBX overwrite makes reimport-in-place resolve the stored source path); references preserved by construction. MCP import_file refuses existing SMs (verbatim error recorded); console Obj Reimport unavailable (no console/exec surface, exhaustively checked). Post-reimport verification ALL PASS: 15,000 tris (blockout was 2,152), bounds match refine_report, UVMap present, slots [TeamRegion→MI_TeamColor_Blue, FootmanPBR→MI_Footman_PBR] readback-correct, Nanite false, convex collision generated ≤4 hulls (NO hull-count readback tool exists — TASK-088 structural pass eyeballs it), BP_Unit_Footman referencer intact, zero import/MikkTSpace warnings, blanket-import check CLEAN (zero strays; /Game/RawAssets browser folder = benign on-disk mirror, no assets), 7 assets saved; editor thumbnail archived Cache/Footman/sm_footman_editor_thumb.png. ✅ Art ts 1783489208.796149; Blockers closed ts 1783489211.235009. handoffs/TASK-086.md = the TASK-087/M7 playbook (7 must-knows incl. batch-the-clicks, no SSL_CERT_FILE, verify facing, selector tune loops expected, ORM manual sRGB→false, MI recipe, 16-clicks-at-M7 tooling gap for manager). Stage history: Stages 1+2+3(a–c) completed pre-click. Stage 1 seed-0 first-try, 87 s on PRO queue, trellis_raw.glb 20.4 MB; authenticated TLS with NO cert bundle — Norton exclusions fully proven. Stage 2 after one eyeball-gate tune (blockout-era shield_band selector striped the spear arm on the mirrored TRELLIS stance → tuned to helm_dome+shoulder_caps 5.7%, manifest _tuned note): 15,000 tris exact, minZ 0.028, UVMap, FBX verified exactly [TeamRegion, FootmanPBR] (QA WARN-3 closed live), concept copied to Concepts/. Accepted warn for the TASK-088 WATCH: X/Y slimmer than blockout (86×48 vs 147×80, Z exact — height-fit law). Stage 3: textures imported (ORM needed manual sRGB→false — TASK-087 note), MI_Footman_PBR verified. OVERWRITE VERDICT (partial): MCP import_file REFUSED same-path overwrite (TASK-031/085 precedent CONFIRMED); console Obj Reimport NOT AVAILABLE on this MCP server → ruling-7 contingency = Jonathan one-click reimport; Content Browser pre-navigated to /Game/Meshes with SM_Footman selected. Remaining after his click: slots [TeamRegion→MI_TeamColor_Blue, FootmanPBR→MI_Footman_PBR], Nanite off, ≤4-hull collision, readbacks + saves. TOOLING GAP for manager before TASK-087/M7: an MCP reimport/console route, or every mesh swap costs a human click. handoffs/TASK-086.md is the living playbook.) — blocker history: (2026-07-07: Stage 1 FAILED on TASK-082 tooling bug — trellis_generate.py:440 passes Client(hf_token=token) but locked gradio_client 2.5.0 renamed the kwarg to token= → TypeError before any network I/O. NOT quota, NOT TLS (Norton-exclusion question untested, still open). Routed back to gameplay-programmer as a TASK-082 QA loop (TASK-084 API-drift rule). SALVAGED while blocked: Stage 3(a) done — /Game/Materials/M_AssetPBR authored+compiled+saved, params BaseColor/Normal/ORM readback-verified, incl. documented helper default /Game/Textures/T_AssetPBR_NeutralORM (16×16 linear AO1/R0.8/M0; texture params can't compile with None); both .uassets uncommitted, ride TASK-088. Same-path-overwrite verdict NOT YET VALIDATED. Resume at Stage 1 after fix passes QA — art-director agent resumable, M_AssetPBR needs no rework. handoffs/TASK-086.md. Dispatch record: gates verified (HF_TOKEN inherited existence-only, Inbox 3/3, editor UP PID 34120, main @ 61bd457).)
- blocked-by: TASK-084 (tooling committed + smoke-tested), TASK-085 (token + concepts)
- parallel-safe: no for Stage 3 (editor-mutating); Stages 1–2 are file-side/Bash
- spec: >
    Full pipeline on the Footman, plus one-time material infrastructure.
    STAGE 1 (Bash): uv run trellis_generate.py for Footman (HF_TOKEN from env; if quota blocks, ruling 8 —
    record reset time, resume next window). Eyeball Cache/Footman/trellis_raw.glb (quick MCP inspection ok,
    <30 s calls). Bad generation → reroll seed (quota permitting) before refining.
    STAGE 2 (Bash, headless): refine_trellis_glb.py per manifest (≤15k tris, 1024² bakes, feet-center,
    UVMap, TeamRegion/FootmanPBR two-slot split). PRE-IMPORT GATE: read refine_report.json + eyeball the
    Cache previews — nothing enters the editor unseen. Copy the accepted concept
    Tools/ArtPipeline/Inbox/Footman.png → Content/RawAssets/Concepts/Footman.png (committed at TASK-088).
    STAGE 3 (editor, serialized): (a) one-time: author master material /Game/Materials/M_AssetPBR with
    texture params named exactly BaseColor, Normal, ORM (ORM wired as linear packed AO/Rough/Metal);
    (b) import textures → /Game/Textures/T_Footman_D (sRGB), T_Footman_N (normal), T_Footman_ORM (LINEAR,
    sRGB off); (c) create /Game/Materials/Instances/MI_Footman_PBR from M_AssetPBR; (d) import the FBX
    OVERWRITING /Game/Meshes/SM_Footman at the same path (ruling 7 — this task VALIDATES the same-path
    overwrite; contingencies in order: console `Obj Reimport`, then escalate to Jonathan one-click via 🚨
    Blockers; NEVER delete+recreate); (e) slots exactly [TeamRegion → MI_TeamColor_Blue (design-time
    placeholder), FootmanPBR → MI_Footman_PBR]; (f) Nanite OFF; simple collision ≤4 hulls; (g) verify zero
    import/MikkTSpace warnings, tris/bounds vs manifest, UVMap present.
    Acceptance: SM_Footman IS the textured mesh at the unchanged path; two slots named/ordered per law;
    M_AssetPBR + MI_Footman_PBR exist; report + overwrite-validation verdict in handoffs/TASK-086.md
    (TASK-087 depends on it). Post in 🎨 Art.
- names: >
    Inputs: Tools/ArtPipeline/Inbox/Footman.png, Cache/Footman/*. Assets: /Game/Meshes/SM_Footman
    (same-path overwrite), /Game/Textures/T_Footman_D | T_Footman_N | T_Footman_ORM,
    /Game/Materials/M_AssetPBR (params BaseColor/Normal/ORM), /Game/Materials/Instances/MI_Footman_PBR.
    Slots: [TeamRegion, FootmanPBR]. Concept: Content/RawAssets/Concepts/Footman.png. FBX:
    Content/RawAssets/Footman.fbx. Law: CONVENTIONS.md "Textured mesh law (TRELLIS.2 art pipeline)".

#### TASK-087 — Pilot batch 2: SM_Archer + SM_Castle via the validated pipeline (art)
- assignee: art-director
- status: done (2026-07-07 late/2026-07-08. Post-click verification ALL PASS both assets: Archer 15,000 tris exact / bounds 90.41×92.15×179.89 / slots [TeamRegion→MI_TeamColor_Blue, ArcherPBR→MI_Archer_PBR] no shuffle / Nanite off / ≤4-hull convex generated (no hull-count readback — TASK-088 eyeballs) / BP_Unit_Archer intact; Castle 40,000 tris exact / bounds 813×819×897 / [TeamRegion, CastlePBR] wired / Nanite off / UCX SURVIVAL CONFIRMED: AggGeom exactly 11 convexElems all bIsGenerated:false, nothing auto-generated, wall footprint intact / L_Arena referencer intact. Strays CLEAN. Deviations recorded: (1) MI_TeamColor_Blue actually lives at /Game/Materials/Instances/ (spec path corrected); (2) Castle front edge is +Y +329 in-editor (interim's −328 was FBX-space; standard Y-flip, magnitude holds, dead-zone shrink vs |410| confirmed); (3) Castle accepted WARN for TASK-088 watch: LogStaticMesh nearly-zero normals/tangents ×2 (voxel-remesh artifact, cosmetic risk, only warnings in the window — strict zero-warnings not met, recorded); (4) both FBXs reimported twice (both selected at click; idempotent — M7 note: select one asset at a time). Post-save is_dirty false. ✅ Art ts 1783492207.129619; Blockers closed ts 1783492217.060459. Commit manifest itemized in handoffs/TASK-087.md + relayed to TASK-088. NOTE: original agent lost at the click gate (transcript unrecoverable); a fresh agent completed the finish pass purely from the interim handoff — handoff-file discipline proven load-bearing. Pre-click history: Blockers ask ts 1783490849.018339. 2026-07-07 late: Stage 1 both seed-0 first-try (Archer 86 s / Castle 123 s, bare env, no re-rolls); facing −Y verified on both. Stage 2 both PASS after exactly one selector tune each (Archer: cap painted the bare head → pauldron caps + probe-measured quiver, 6.3%; Castle: Z-stretch steepened roofs past min_dot 0.3 → min_dot 0.10/z≥0.30, 6.8%; both `_tuned` in the manifest). Archer 15,000 tris exact, [TeamRegion, ArcherPBR], height-fit WARN 90.7×92.5 vs blockout 125×67.3 (TASK-088 WATCH). Castle 40,000 tris, bounds EXACT 814.5×820×900 (±10% by construction), UCX re-derived: 11 wall-footprint hulls (4 walls+4 towers+keep+chapel+gatehouse — 2 structures the blockout lacked); front collision y −328 vs blockout −410 ⇒ M1 dead-zone SHRINKS (improvement, no regress). Stage 3 pre-click done: 6 textures (ORM sRGB→false) + MI_Archer_PBR + MI_Castle_PBR readback-verified, 8 assets saved. Remaining post-click: readbacks, slot fixes if shuffled, MI assignments, Nanite off, Archer ≤4-hull collision, Castle UCX-survival check, referencers, zero-warning scan, saves, final handoff. Interim handoffs/TASK-087.md)
- blocked-by: TASK-086 (needs M_AssetPBR + the same-path-overwrite verdict). EXCEPTION per ruling 9: Stage-1 generation + Stage-2 refine for Archer/Castle are file-side and MAY pre-run any time after TASK-084 + TASK-085 (quota permitting); only the Stage-3 imports wait on TASK-086.
- parallel-safe: no for Stage 3 (editor-mutating; serialize imports); Stages 1–2 file-side
- spec: >
    Repeat the TASK-086 pipeline for the two remaining pilot assets, using handoffs/TASK-086.md as the
    import playbook.
    ARCHER (unit path): ≤15k tris, 1024² bakes, feet-center, slots [TeamRegion, ArcherPBR] →
    MI_Archer_PBR; simple collision ≤4 hulls; same-path overwrite /Game/Meshes/SM_Archer.
    CASTLE (building path): ≤40k tris, 2048² bakes, ground-center, slots [TeamRegion, CastlePBR] →
    MI_Castle_PBR; explicit UCX_SM_Castle collision, wall-footprint-exact with bounds ±10% of the blockout
    (the M1 plinth ~410-unit dead-zone must NOT regress — gold-node/miner clearance depends on it); same-path
    overwrite /Game/Meshes/SM_Castle. Castle bounds sanity: CastleAnchor placement, HP-bar clearance above
    the roof, and the L_Arena silhouette must stay sane (±10% rule).
    Both: textures T_<AssetName>_D/_N/_ORM per law; concepts copied to Content/RawAssets/Concepts/;
    pre-import gate (refine_report.json + preview eyeball) per asset; zero import warnings; Nanite OFF.
    Quota ruling 8 applies — one asset per day is an acceptable pace; record windows in the handoff.
    Acceptance: both SMs are textured meshes at unchanged paths with law-conformant slots/collision;
    handoffs/TASK-087.md complete. Post in 🎨 Art.
- names: >
    /Game/Meshes/SM_Archer + /Game/Meshes/SM_Castle (same-path overwrites). Textures:
    /Game/Textures/T_Archer_D|_N|_ORM, T_Castle_D|_N|_ORM. MIs: /Game/Materials/Instances/MI_Archer_PBR,
    MI_Castle_PBR (from M_AssetPBR). Slots: [TeamRegion, ArcherPBR] / [TeamRegion, CastlePBR]. Collision:
    UCX_SM_Castle. Concepts: Content/RawAssets/Concepts/Archer.png, Castle.png. FBX:
    Content/RawAssets/Archer.fbx, Castle.fbx. Law: CONVENTIONS.md "Textured mesh law".

#### TASK-088 — Trellis pilot integration: PIE verification + commit (build-master)
- assignee: build-master
- status: done (2026-07-08: committed **cb29882** on main, NOT pushed — 39 files (3 FBX, 12 PNGs, 3 SMs, 10 textures, M_AssetPBR+3 MIs, tuned manifest, trellis_generate.py loop-2 fix, pipeline docs); WBP_MainMenu.uasset excluded per TASK-081 ruling, still the only residue; staged LFS oids verified vs worktree. STRUCTURAL all PASS incl. hull-count readback — GAP CLOSED: ObjectTools.get_properties on BodySetup_0.AggGeom reads hull counts (Footman/Archer exactly 4 hulls; Castle exactly 11, all bIsGenerated:false, 1:1 to manifest UCX list). PIE PASS: SendInput hotkey→ghost→LMB-click-confirm played Archer/Footman/Miner through the REAL placement path (in-PIE clicks now PROVEN, extending TASK-081 doctrine; new law: Alt-tap on refocus arms the Windows menu accelerator and eats the next number key — follow refocus with a viewport click; Alt = IA_UICursor); two-slot contract live-proven BOTH directions (Blue player + Red bot recolor slot 0 only, PBR slot untouched); ghost/refusal correct net-zero; both castles textured w/ team roofs; HP bar Z+1050 vs roof 897.65; bot miner reached GoldNode (clearance no-regress); texture delta ~17 MB (<100 budget), zero streaming warnings; no new log entries (all knowns). stat overlays NOT machine-drivable (no console-exec surface) → 1-keystroke human WATCH. FINDINGS→manager: (1) bot rush measured — undefended Blue castle dies ~33 s, will dominate the next playtest (pre-existing balance, not art); (2) M7 tooling asks: console-exec MCP tool would close reimport-click + stat + test-harness gaps; (3) AggGeom readback = M7 standard collision check. Harness anomalies (lifted-castle-only, non-reproducible in human play) logged in the final report. WATCH posted 🔧 ts 1783495651.317409. POST-COMMIT RESIDUE NOTE (2026-07-08): after cb29882, Jonathan saved 2 editor-dirty packages at the orchestrator's confirmation — BP_Unit_Footman + BP_Unit_Archer .uassets, dirtied by the mesh reimports re-registering components (verified benign; nothing else was dirty in a 24-asset sweep). These two modified .uassets + the standing WBP_MainMenu.uasset delta are the known worktree residue — next build-master commits or restores them KNOWINGLY (reimport-re-registration chore, not feature work).)
- blocked-by: TASK-086, TASK-087
- parallel-safe: no (owns the single editor + the Git commit)
- spec: >
    (1) Structural checks on the three swapped meshes: slots == [TeamRegion, <AssetName>PBR] with the right
    MIs; Nanite false; collision present (≤4 hulls units, UCX castle); tris/bounds vs pipeline_manifest.json;
    zero pending import warnings.
    (2) PIE on direct-boot L_Arena (menu/sandbox not machine-drivable — TASK-073 precedent) vs the bot:
    play Footman + Archer via hotkeys — textured meshes render with blue TeamRegion accents; the bot's Red
    spawns recolor slot 0 (two-slot contract live-proof); placement mode still resolves
    /Game/Meshes/SM_<CardID> ghosts and tints them fully; both castles render textured, HP bars clear the
    roofs, castle plinth clearance unchanged (miners reach GoldNodes; placement near castles behaves as
    before); `stat unit` + `stat streaming` sanity — texture memory delta <100 MB; no new log
    warnings/errors.
    (3) Commit the art batch to main with TASK-086..088 (+085 gate note) in the message: Content/RawAssets/
    {Footman,Archer,Castle}.fbx, RawAssets/Textures/**, RawAssets/Concepts/**, /Game/Meshes SM uassets,
    /Game/Textures/**, M_AssetPBR + MIs. NOT pushed.
    (4) Record the WATCH: Jonathan's visual sign-off (style cohesion vs the remaining blockouts, silhouette
    at gameplay camera, team read at distance). On sign-off the remaining 16 meshes become the M7 template.
    Post results + hash in 🔧 Build & Git.
    Acceptance: structural + PIE checks PASS; committed to main, not pushed; WATCH posted.
- names: >
    Verify: SM_Footman/SM_Archer/SM_Castle slots + collision + Nanite; MI_Footman/Archer/Castle_PBR;
    M_AssetPBR; /Game/Maps/L_Arena PIE. Budget refs: Tools/ArtPipeline/pipeline_manifest.json. Commit to
    main only, not pushed.

#### WATCH — Trellis pilot visual sign-off (Jonathan, after TASK-088) — CLOSED ✅
- **SIGNED OFF 2026-07-08 by Jonathan, verbatim "the trellis pilot was successful" — ZERO findings.**
  The TRELLIS.2 → Blender → UE5 pipeline is now the validated **M7 template** for the remaining 16 blockout
  meshes. Chain TASK-082..088 fully closed (commits 1e923d4 tooling + cb29882 art batch, NOT pushed).
  Proven economics for M7 planning: ~1.5–2 min/generation on HF PRO, seed-0 first-try 3/3, one selector
  tune loop per asset, one human Content-Browser Reimport click per mesh (until an MCP console-exec tool
  lands). Original WATCH scope for reference: style cohesion vs remaining blockouts, silhouette at gameplay
  camera, Blue/Red team read at distance.

### Menu→arena input-loss bugfix (TASK-074..076) — Jonathan bug report 2026-07-07
**Blocks the M4 playtest.** Verbatim symptom: from L_MainMenu, clicking "Play vs Bot" OR "Sandbox (No Bot)" joins the match, but then **WASD does nothing and no cards can be used — no user input at all**. Hitting Play directly in L_Arena still works with full input.

**Manager triage rulings (binding for this chain):**
1. **BOTH buttons affected ⇒ the fault is the menu→arena travel path generally, NOT the TASK-071 sandbox gate** ("Play vs Bot" → static `ASiegeGameMode::StartMatch`, "Sandbox (No Bot)" → static `::StartSandboxMatch`; both travel via `UGameplayStatics::OpenLevelBySoftObjectPtr` → `/Game/Maps/L_Arena`). TASK-071's gate is NOT reopened. Direct-L_Arena PIE working means the M2 arena input plumbing (TASK-023) is healthy.
2. **Prime suspect — AUDIT-FIRST, not assumed:** `BP_MenuGameMode` puts the menu in UIOnly + visible cursor (handoffs/TASK-049.md, TASK-052.md) so buttons are clickable; input-routing state survives `OpenLevel` travel on the persistent `UGameViewportClient`, so L_Arena boots input-swallowed. `ASiegePlayerController::BeginPlay` currently sets NO input mode.
3. **Fix direction:** the ARENA side normalizes its OWN input posture at startup rather than trusting the traveler — new law in CONVENTIONS.md "Input-mode ownership (level-travel law)" (added 2026-07-07, before task issue). The C++ normalization ships regardless; a menu-side editor edit (TASK-075) happens ONLY if TASK-074's audit proves it is required.
4. **Must-not-break contract:** Alt-held UI cursor (M2), placement-mode cursor, menu buttons clickable on L_MainMenu, HandleMatchEnd UIOnly + PlayAgain restore, M1 WARN-4 clickability posture, direct-L_Arena feel byte-identical.
5. **Verification constraint (binding, TASK-073 precedent):** MCP cannot click menu buttons or pass level-open URL options in PIE — the menu→arena path is NOT machine-drivable. Pre-compile QA as usual; build-master machine-verifies only the direct-L_Arena input regression; the live bug-fix confirmation is Jonathan's ONE menu click (WATCH below).

Chain runs strictly in sequence: **TASK-074 (C++ audit+fix, file-only) → TASK-075 (CONDITIONAL editor fix) → TASK-076 (build-master compile + regression PIE + commit, NOT pushed).** Editor state: TASK-073 left it UP (PID 16916, 2026-07-05) but it may be closed — build-master bounces it regardless for the compile.

#### TASK-074 — Menu→arena travel input loss: audit + arena-side input normalization (C++)
- assignee: gameplay-programmer
- status: done (committed 218b4c9 via TASK-076, NOT pushed; git diff-audit closed rulings 4/5 — exactly SiegePlayerController.h/.cpp. QA PASS 2026-07-07 — 0 blocker/0 warn/1 nit; all 6 flagged decisions ruled, all 6 regression-contract items PASS, shadow-clean; root cause independently re-verified in BP_MenuGameMode.uasset; qa/TASK-074-report.md. WATCH open: Jonathan's one menu click confirms the fix live.) (2026-07-07. Root cause CONFIRMED = prime suspect: BP_MenuGameMode EventBeginPlay calls SetInputMode_UIOnlyEx → SetIgnoreInput(true) + NoCapture on the PERSISTENT UGameViewportClient, surviving OpenLevelBySoftObjectPtr travel; fresh arena PC set no input mode → viewport swallowed all input. Menu uses engine-default APlayerController (escalation clause not triggered; fix arena-scoped by construction). Fix: ASiegePlayerController::BeginPlay first statement = ApplyCursorInputState() → exact FInputModeGameOnly on fresh controller, clears the viewport ignore-input latch; no-op by value on direct PIE. Editor change needed: NO → TASK-075 cancel condition met. Files: SiegePlayerController.cpp (+ .h doc comment only). handoffs/TASK-074.md)
- blocked-by: none
- parallel-safe: yes (file-only C++; touches SiegePlayerController.h/.cpp only — within this chain strictly serial 074→[075]→076, but 074 shares no files with any other open work)
- spec: >
    Files only, no editor, no compile. Bug (Jonathan 2026-07-07, blocks the M4 playtest): entering a match from
    L_MainMenu via EITHER menu button ("Play vs Bot" → static ASiegeGameMode::StartMatch; "Sandbox (No Bot)" →
    static ASiegeGameMode::StartSandboxMatch, TASK-071) yields NO user input in L_Arena — WASD dead, hotkeys 1–6
    dead, no cards playable. Direct-PIE on L_Arena works with full input, so the M2 arena input plumbing
    (TASK-023: GameOnly free-look, hotkeys 1–6, Alt-held GameAndUI cursor) is healthy; BOTH buttons broken means
    the fault is the menu→arena travel path generally, NOT the sandbox gate.
    (0) AUDIT FIRST — confirm the root cause before editing; do NOT assume. Prime suspect (unproven):
    BP_MenuGameMode (handoffs/TASK-049.md; TASK-052.md line "cursor + CreateWidget WBP_MainMenu + UIOnly") puts
    the menu in FInputModeUIOnly + visible cursor; both Start* statics travel via
    UGameplayStatics::OpenLevelBySoftObjectPtr (SiegeGameMode.cpp ~669 / ~706); input-routing state set by
    SetInputMode lives partly on the persistent UGameViewportClient and can survive that travel, so the fresh
    ASiegePlayerController in L_Arena boots with UI-only routing swallowing game input. Evidence base already
    verified by the manager: ASiegePlayerController::BeginPlay (SiegePlayerController.cpp ~56) sets NO input
    mode — the only SetInputMode sites are HandleMatchEnd's UIOnly end screen (~729) and ApplyCursorInputState()
    (~1562). Engine-source / documented-behavior reasoning is acceptable audit evidence (no editor access).
    Also confirm which PlayerController class L_MainMenu uses — expected: the engine default via BP_MenuGameMode
    (parent GameModeBase), NOT ASiegePlayerController; record the answer in the handoff.
    (1) FIX — arena-side self-normalization (ships regardless of audit fine detail, per CONVENTIONS
    "Input-mode ownership (level-travel law)"): ASiegePlayerController establishes its own match posture at
    BeginPlay — GameOnly free-look + hidden cursor — instead of trusting inherited state. Preferred
    implementation: call the existing ApplyCursorInputState() from BeginPlay (on a fresh controller
    bInPlacementMode/bUICursorHeld/bMatchEnded are all false, so it applies exactly FInputModeGameOnly, hides
    the cursor, and clears bEnableClickEvents). If the audit shows that is insufficient (e.g. residual viewport
    state needing FlushPressedKeys or ignore-input clearing), extend minimally and document why in the handoff.
    (2) MUST NOT BREAK (regression contract, ruling 4): (a) Alt-held IA_UICursor GameAndUI cursor; (b)
    placement-mode cursor + click-confirm; (c) HandleMatchEnd's UIOnly victory-screen state and the
    PlayAgain/HandleMatchReset restore path; (d) L_MainMenu buttons staying clickable — the fix must be
    arena-scoped; if the audit finds the menu DOES use ASiegePlayerController, STOP and escalate in the handoff
    instead of shipping a normalization that would kill menu clicks; (e) the M1 WARN-4 clickability posture;
    (f) direct-PIE L_Arena feel byte-identical (normalization is a no-op when state is already GameOnly).
    (3) Do NOT change StartMatch / StartSandboxMatch signatures or behavior unless the audit PROVES the travel
    call itself must change — default expectation is SiegePlayerController.h/.cpp only.
    (4) Conditional editor follow-up: if the audit shows the root cause ALSO requires an editor-asset change
    (BP_MenuGameMode graph / WBP_MainMenu / L_MainMenu settings), the C++ normalization still ships as the
    robustness layer; write the EXACT prescribed editor change into handoffs/TASK-074.md so TASK-075 executes it
    verbatim. If no editor change is needed, say so explicitly — TASK-075 is then cancelled by the manager.
    (5) CONVENTIONS shadow law (C4457/58/59): no local/param may shadow an inherited reflected UPROPERTY. QA
    MUST scan this task for shadow vars pre-compile.
    Acceptance: root cause documented with evidence in handoffs/TASK-074.md; after the fix, a fresh
    ASiegePlayerController beginning play in L_Arena applies GameOnly + hidden cursor REGARDLESS of prior
    viewport/input state; all six regression-contract behaviors preserved by inspection; handoff notes that the
    live menu-click confirmation is Jonathan's (menu path not machine-drivable, TASK-073 precedent).
- names: >
    ASiegePlayerController (Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h/.cpp) — BeginPlay
    (~line 56), ApplyCursorInputState (~line 1562), HandleMatchEnd UIOnly block (~line 728), HandleMatchReset
    (~line 744), flags bInPlacementMode / bUICursorHeld / bMatchEnded. Travel entries (read-only, behavior
    unchanged): ASiegeGameMode::StartMatch / ::StartSandboxMatch (Siegebound/SiegeGameMode.cpp ~669 / ~706),
    UGameplayStatics::OpenLevelBySoftObjectPtr → /Game/Maps/L_Arena. Menu side (READ-ONLY this task):
    /Game/Blueprints/BP_MenuGameMode, /Game/UI/WBP_MainMenu (Btn_Sandbox per CONVENTIONS "Dev / test tooling"),
    /Game/Maps/L_MainMenu. Law: CONVENTIONS.md "Input-mode ownership (level-travel law)".

#### TASK-075 — CONDITIONAL menu-side editor fix (only if TASK-074's audit demands it)
- assignee: gameplay-programmer
- status: cancelled (2026-07-07, orchestrator applying the manager's pre-authorized condition: handoffs/TASK-074.md verdict "editor change needed: NO" — menu UIOnly posture is correct per the level-travel law; the arena-side C++ normalization is the complete fix. TASK-076 skips the wait per its blocked-by line)
- blocked-by: TASK-074 (needs its audit verdict + exact prescription; if the prescribed change binds new C++ symbols, ALSO wait for TASK-076's phase-1 compile per the TASK-072/073 editor-bounce pattern)
- parallel-safe: no (editor-mutating — single editor instance)
- spec: >
    Execute EXACTLY the editor-asset change prescribed in handoffs/TASK-074.md — candidates are the
    BP_MenuGameMode event graph (its UIOnly/cursor setup), WBP_MainMenu, or L_MainMenu settings. Nothing beyond
    the prescription; additive/minimal. MUST NOT break: menu buttons remaining mouse-clickable on L_MainMenu
    (the menu keeps its UIOnly-or-equivalent cursor posture per CONVENTIONS "Input-mode ownership"), the
    Play-vs-Bot binding (→ StartMatch, byte-identical, TASK-049), the Btn_Sandbox binding (→ StartSandboxMatch,
    TASK-072). Save and report the exact edit in the handoff.
    Acceptance: prescribed change applied verbatim; both menu buttons still present + bound; menu still fully
    mouse-operable in PIE.
- names: >
    Candidates (whichever handoffs/TASK-074.md prescribes): /Game/Blueprints/BP_MenuGameMode,
    /Game/UI/WBP_MainMenu (existing Play-vs-Bot button + Btn_Sandbox), /Game/Maps/L_MainMenu. Bindings:
    ASiegeGameMode::StartMatch / ::StartSandboxMatch. Laws: CONVENTIONS.md "Input-mode ownership
    (level-travel law)" + "Dev / test tooling".

#### TASK-076 — Menu-travel bugfix integration: compile, regression PIE, commit (build-master)
- assignee: build-master
- status: done (2026-07-07, committed on main, NOT pushed — hash in the build-master report/Slack 🔧 thread. Compile PASS clean ~25s (only SiegePlayerController.cpp rebuilt). DIFF AUDIT closes QA rulings 4/5: git diff showed ONLY SiegePlayerController.cpp (+18: comment block + one ApplyCursorInputState() call after Super::BeginPlay) and .h (+11/-1: doc-comment only, BeginPlay declaration byte-identical); SiegeGameMode.cpp absent from diff → StartMatch/StartSandboxMatch untouched. Regression PIE on direct-boot L_Arena (2 sessions): fresh-BeginPlay posture read LIVE = bShowMouseCursor:false + bEnableClickEvents:false (normalized GameOnly); full match loop ran to completion under the new BeginPlay (bot spawned + played, economy exactly +2/s, castle destroyed, match-end freeze fired); HandleMatchEnd UIOnly flip read LIVE post-match = cursor:true + clicks:true (contract item c live-verified); zero new log lines from the change as QA predicted. CONSTRAINT: live WASD/hotkey/Alt keystroke injection was IMPOSSIBLE this session — Jonathan's desktop was LOCKED (SendInput blocked by Winlogon; Slate drops unfocused PostMessage keys) — so the felt-input check folds into Jonathan's existing WATCH click, which exercises WASD+hotkeys+cards anyway. Structural: WBP_MainMenu readback = Play (vs Bot)→StartMatch and Btn_Sandbox (BuildSandboxButton)→StartSandboxMatch both bound. No boot-resave .uasset noise; .claude/settings.json + hooks/ left uncommitted per orchestrator. Editor left UP on L_Arena on the committed DLL. Pre-existing follow-ups (NOT from this change): (1) 'InputMode:UIOnly - Attempting to focus Non-Focusable widget' engine error at every match end (HandleMatchEnd's victory widget not focusable — cosmetic, untouched code); (2) DeepMine CardType-2 warning (TASK-035 watch) fired both matches; (3) balance: undefended bot rush kills Blue castle in ~48s.)
- blocked-by: TASK-074 (qa-passed), TASK-075 (only if dispatched — skip if cancelled by the audit)
- parallel-safe: no (owns the single editor + the compile + the Git commit)
- spec: >
    Integration for the menu→arena input-loss bugfix. (1) Editor: TASK-073 left it UP (PID 16916, 2026-07-05)
    but it may be closed by now — bounce/relaunch regardless and compile TASK-074's C++ via the standard
    editor-bounce/Build.bat. Compile failure → append errors to qa/TASK-074-report.md and route back to
    gameplay-programmer (counts as a QA loop). If TASK-075 was prescribed, sequence like TASK-073: compile
    first, hand back to the orchestrator so TASK-075 runs against the live module, then finish here.
    (2) Regression PIE — machine-drivable part ONLY (binding constraint, TASK-073 precedent: MCP cannot click
    menu buttons or pass level-open URL options in PIE, so the menu→arena path is NOT machine-verifiable):
    PIE directly on L_Arena and verify input is UNREGRESSED — WASD free-look moves the hero; hotkeys 1–6 reach
    PlayHandSlot; Alt-held cursor appears, clicks land, release restores free-look; placement mode shows its
    cursor; match-end → UIOnly victory screen → PlayAgain restores play. Structural checks: WBP_MainMenu still
    has BOTH buttons bound (readback: Play-vs-Bot → StartMatch, Btn_Sandbox → StartSandboxMatch).
    (3) HUMAN-VERIFY acceptance (record in commit message + handoff + Slack 🔧 Build & Git): the actual bug-fix
    confirmation needs Jonathan's ONE click — from L_MainMenu press either button and confirm WASD + hotkeys +
    cards all work in the match, cursor hidden, Alt-cursor still works. Post the ask and point at the WATCH
    below.
    (4) Commit to `main` with TASK-074/075/076 in the message. **NOT pushed** (no remote push without
    Jonathan's explicit instruction).
    Acceptance: clean compile; direct-L_Arena input regression PASS; menu buttons structurally intact;
    committed to main, not pushed; Jonathan's click recorded as the outstanding WATCH.
- names: >
    Build target GitClaudeUnrealTestEditor (Build.bat per CLAUDE.md). Maps: /Game/Maps/L_MainMenu,
    /Game/Maps/L_Arena. Verify: ASiegePlayerController posture at BeginPlay (GameOnly + hidden cursor),
    ApplyCursorInputState behaviors, WBP_MainMenu bindings (StartMatch / StartSandboxMatch). Commit to main
    only, not pushed.

#### WATCH — menu→arena input fix live confirm — **CLOSED 2026-07-07**
- Jonathan confirmed live in Claude Code ("that problem is resolved") after 218b4c9: joining from L_MainMenu
  now gives full input. The TASK-074..076 chain is fully closed; the Sandbox TASK-073 menu-click WATCH
  (expect "Sandbox match"/"SpawnBot skipped" logs) is implicitly satisfied by the same confirmation path
  being exercised — manager may treat both menu-path WATCHes as done.

### Sandbox / test-tooling feature (TASK-071..073) — Jonathan-approved 2026-07-05
**This is a developer/test affordance, NOT M5 content — building it does NOT break the "M5 not authorized" hold.** Jonathan wants a calm "Sandbox (No Bot)" test bench to exercise the full 22-card roster on `main` without the enemy AI's chaos. Not a milestone; it lives in Active tasks and integrates as a small chain. Mechanism = a level-open URL option `Sandbox=1` (NOT a GameInstance) — full naming law in CONVENTIONS.md "Dev / test tooling". Chain runs strictly in sequence: **TASK-071 (C++ gate) → TASK-072 (menu button, editor) → TASK-073 (build-master integrate + commit).** The editor is CLOSED now; build-master relaunches it (and compiles TASK-071) before the editor/UMG task. Note: `WBP_MainMenu` already EXISTS at `/Game/UI/WBP_MainMenu` (TASK-049) — the menu is a real widget, not a level-BP.

#### TASK-071 — Sandbox bot-spawn gate + generous economy (C++)
- assignee: gameplay-programmer
- status: done (committed e9cb7f7 via TASK-073). QA PASS (0 blocker/0 warn/2 nit; Red-PS null-safety complete, shadow-clean, StartMatch byte-identical, AddGold correct). Compile PASS clean. ORCH RULINGS: AddGold ACCEPTED, 999 gold ACCEPTED. handoffs/TASK-071.md, qa/TASK-071-report.md.
- blocked-by: none
- parallel-safe: yes (file-only C++; touches SiegeGameMode.h/.cpp only. Within this feature the chain is strictly serial 071→072→073, but 071 shares no files with any other open work)
- spec: >
    Files only, no editor, no compile. This is a dev/test affordance (Sandbox/test-tooling — see CONVENTIONS.md
    "Dev / test tooling"), NOT M5/GDD content. Goal: when a sandbox flag is set, L_Arena boots with NO enemy
    bot so the full 22-card roster is freely testable against a static Red-castle target dummy (M1-like, but with
    the full M2/M3/M4 hand + roster).
    (0) AUDIT FIRST: the bot spawn site is already located — ASiegeGameMode::SpawnBot() (SiegeGameMode.cpp
    ~line 677, called from BeginPlay) spawns the single ASiegeBotController into the member BotController; the
    Red bot ASiegePlayerState is created by the bot controller's bWantsPlayerState. Confirm this before editing;
    do NOT introduce a USiegeGameInstance (none exists — use the level-open option instead).
    (1) Latch a sandbox flag from a level-open URL option: override AGameModeBase::InitGame (or read the mode's
    OptionsString at the earliest safe point) and set a new `bool bSandboxMatch` when
    UGameplayStatics::HasOption(Options, TEXT("Sandbox")) is true. The token string is exactly "Sandbox" (=1).
    bSandboxMatch persists for the life of the L_Arena world so PlayAgain stays sandbox.
    (2) Gate the bot: SpawnBot() early-returns when bSandboxMatch is true — no ASiegeBotController spawned, no Red
    bot PlayerState seeded, no bot decisions ever fire. Everything else (Blue player, hero, castles, hand, deck,
    economy tick, win condition binding on both castles) stays exactly as today. Verify nothing dereferences the
    Red ASiegePlayerState unconditionally in a way that would crash when it is absent (win condition, overtime
    rate, GetPlayerStateForTeam(Red) callers) — Blue units target the Red ACastle actor directly, which still
    exists, so the roster stays testable; guard any Red-PS read null-safely.
    (3) Menu entry: add `static void StartSandboxMatch(const UObject* WorldContextObject)`
    (UFUNCTION BlueprintCallable, WorldContext) mirroring the existing StartMatch, but pass the Options string
    "Sandbox=1" through UGameplayStatics::OpenLevelBySoftObjectPtr(WorldContextObject, ArenaLevel, true,
    TEXT("Sandbox=1")). Do NOT change StartMatch's signature or behavior (Play vs Bot must be byte-identical).
    (4) Generous economy: add UPROPERTY(EditDefaultsOnly, Category="Siegebound|Sandbox") int32 SandboxStartingGold
    = 9999 (// dev sandbox — full roster freely playable). When bSandboxMatch is true, grant the Blue player this
    starting pile once at match start THROUGH the existing ASiegePlayerState gold API (do not bypass it / do not
    hardcode a raw gold field write). Keep the normal gold rate.
    (5) CONVENTIONS shadow law (C4457/58/59): no local/param/loop var may shadow an inherited reflected UPROPERTY
    (Owner/PlayerState/Instigator/Controller/etc.) — the InitGame override's `Options`/`ErrorMessage` params are
    engine-named, keep new locals distinct. QA MUST scan this task for shadow vars pre-compile.
    Acceptance: with the option set (open L_Arena?Sandbox=1, i.e. via StartSandboxMatch), at BeginPlay there is
    ZERO ASiegeBotController in the world and no bot ever plays a card; the Blue player starts with SandboxStartingGold
    and the full hand/roster is playable against Castle_Red; PlayAgain in a sandbox match does NOT spawn a bot.
    Without the option (StartMatch / Play vs Bot), the bot spawns and behaves EXACTLY as today. Nothing hardcoded
    that lives in DT_Cards; the StartMatch path is unchanged.
- names: >
    ASiegeGameMode (Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h/.cpp) — new member bool bSandboxMatch;
    UPROPERTY int32 SandboxStartingGold (default 9999); static void StartSandboxMatch(const UObject* WorldContextObject);
    InitGame override to parse the option. Option token: "Sandbox" (value 1), parsed via
    UGameplayStatics::HasOption / passed via OpenLevelBySoftObjectPtr Options="Sandbox=1". Bot gate:
    ASiegeGameMode::SpawnBot() early-return; class ASiegeBotController (Siegebound/SiegeBotController.h).
    Player gold API: existing ASiegePlayerState gold methods (Siegebound/SiegePlayerState.h). Arena target:
    ArenaLevel (/Game/Maps/L_Arena). Full naming law: CONVENTIONS.md "Dev / test tooling".

#### TASK-072 — "Sandbox (No Bot)" main-menu button (editor / UMG)
- assignee: gameplay-programmer
- status: done (committed e9cb7f7 via TASK-073; WBP_MainMenu.uasset). Additive Btn_Sandbox "Sandbox (No Bot)" at VBox index 1 (Play·Sandbox·Deck·Quit), OnClicked→static StartSandboxMatch (WorldContext=self). Play-vs-Bot binding byte-identical. Compiles clean + saved. handoffs/TASK-072.md.
- blocked-by: TASK-071 (needs the compiled StartSandboxMatch UFUNCTION resolvable in the editor to bind the button)
- parallel-safe: no (editor-mutating — single editor instance; edits WBP_MainMenu. Requires the editor running with TASK-071 compiled in — build-master performs the editor-bounce compile of TASK-071 before this task, per the M2/M4 "C++ compiles, then editor wave" pattern)
- spec: >
    Editor/MCP UMG work in /Game/UI/WBP_MainMenu (it EXISTS — TASK-049 authored it; the orchestrator's "no
    WBP_MainMenu" note is stale). ADDITIVE only. (1) Read the existing menu: find the current "Play vs Bot"
    button (bound OnClicked → ASiegeGameMode::StartMatch) and note its parent panel + naming so the new button
    matches its layout/style. (2) Add a sibling button `Btn_Sandbox` directly next to it, label text
    "Sandbox (No Bot)"; bind its OnClicked to call the static ASiegeGameMode::StartSandboxMatch (WorldContext =
    self). (3) Do NOT disturb the existing Play-vs-Bot button, its binding, or any other menu widget/nav — this is
    purely additive; the existing button must still open L_Arena with the bot exactly as today.
    Acceptance (verified at integration PIE by build-master): the menu shows both buttons; clicking "Sandbox
    (No Bot)" opens L_Arena with NO enemy bot; clicking "Play vs Bot" still opens L_Arena WITH the bot. MCP-authored
    UMG is reliable now (TASK-041/050/064). Post the WBP_MainMenu save + button name in the handoff.
- names: >
    /Game/UI/WBP_MainMenu — new button `Btn_Sandbox`, label "Sandbox (No Bot)", OnClicked →
    ASiegeGameMode::StartSandboxMatch (from TASK-071). Existing button (do not touch) calls
    ASiegeGameMode::StartMatch. Full naming law: CONVENTIONS.md "Dev / test tooling".

#### TASK-073 — Sandbox mode integration: compile, PIE-verify, commit (build-master)
- assignee: build-master
- status: done (commit e9cb7f7 on main, parent 5c1fcb7, NOT pushed; 9 files selective, +231/-0 code additive). Phase1 compile PASS (clean, ~23s). PIE: Play-vs-Bot REGRESSION verified LIVE (bot spawns + Rule2/3/4 decisions + Red PS present → gate didn't break shipping). Sandbox branch NOT drivable via MCP (bSandboxMatch is non-reflected; StartPIE ignores ?Sandbox=1 AdditionalServerGameOptions — proven via listen-server test; no console-open/UFUNCTION-invoke/menu-click injection) → QA-verified + compiled, needs Jonathan's 1 menu-click to confirm live (expect log: "Sandbox match", "SpawnBot skipped", "granted 9999 gold (now 999)"). Both menu buttons present (structural). Editor left UP PID 16916 on L_Arena. handoffs/TASK-073.md.
  - FOLLOW-UP: CONVENTIONS.md "Dev/test tooling" section left unstaged → committing separately.
- blocked-by: TASK-071, TASK-072
- parallel-safe: no (owns the single editor + the compile + the Git commit)
- spec: >
    Integration for the Sandbox/test-tooling feature. (1) Relaunch the UE editor (currently CLOSED) and compile the
    TASK-071 C++ via the standard editor-bounce/Build.bat — this compile must happen BEFORE TASK-072's UMG binding
    can resolve StartSandboxMatch, so sequence: compile TASK-071 → hand back to the orchestrator so TASK-072 authors
    the button against the live module → then this integration completes. If the compile fails, append errors to
    qa/TASK-071-report.md and route back to gameplay-programmer (counts as a QA loop). (2) After TASK-072 lands,
    PIE-verify the full slice: main menu (L_MainMenu) shows both buttons → click "Sandbox (No Bot)" → L_Arena boots
    with ZERO ASiegeBotController (check logs: no "Spawned bot opponent" line; no LogSiegeBot decisions), Blue starts
    with the generous SandboxStartingGold, and the full roster is playable via the visual hand / hotkeys 1–6 against
    the static Castle_Red with NO opposing AI. (3) Regression: from the menu click "Play vs Bot" → confirm the bot
    STILL spawns exactly as today (the "Spawned bot opponent … Red ASiegePlayerState" log line appears and the bot
    plays cards). (4) Commit to `main` with the task IDs (TASK-071/072/073) in the message. **NOT pushed** (no remote
    push without Jonathan's explicit instruction). Post compile result + commit hash in 🔧 Build & Git.
    Acceptance: clean compile; sandbox slice verified bot-free + roster playable; Play-vs-Bot regression confirmed
    bot-present; committed to main, not pushed.
- names: >
    Build target GitClaudeUnrealTestEditor (Build.bat per CLAUDE.md). Maps: /Game/Maps/L_MainMenu (menu),
    /Game/Maps/L_Arena (match). Verify absence of ASiegeBotController; StartSandboxMatch vs StartMatch paths.
    Commit to main only, not pushed.

### TASK-070 — L_Arena stray-actor cleanup (editor)
- assignee: gameplay-programmer
- status: done (commit a745799 on main, parent e586699, NOT pushed; selective — only L_Arena.umap + handoff). Removed 3 M2 TM040_ verification strays (Footman_C_1/Miner_C_2/ArrowTower_C_1); 27 intended actors intact; PIE clean-start VERIFIED (0 strays, bot opens Rule 3 Attack not t=0 defend) → M3 transient-unit WATCH CLOSED. main-only fix (m2/m3/m4-testable snapshots still carry the strays → playtest full game on MAIN for clean start). NOTE (tuning, not defect): bot opens with attack not economy (affords Ogre at start) — possible balance item for playtest. handoffs/TASK-070.md.
- blocked-by: none
- parallel-safe: no (editor-mutating — one editor instance; touches L_Arena.umap)
- spec: >
    Editor/MCP work in /Game/Maps/L_Arena. Root-caused in handoffs/TASK-069.md: three verification actors
    were accidentally saved into L_Arena.umap during the M2 editor pass and have been committed on EVERY
    branch since 5bb9507 / 40b69ef — they appear as stray units at match start and make the bot play a
    Rule-1 "defend" at t=0 (it reads them as an enemy push on its half). (1) Delete the three stray actor
    instances from L_Arena: BP_Unit_Footman_C_1, BP_Unit_Miner_C_2, BP_Building_ArrowTower_C_1 (confirm by
    class + transform before deleting; do NOT touch the legitimate GoldNode_Blue/Red, Castle_Blue/Red,
    arena boundary volumes, PlayerStart, KillZ, nav, or decal actors). (2) Re-save L_Arena (is_dirty=false).
    (3) PIE a COLD-BOOT match start and verify a CLEAN start: zero stray Blue/Red units on the field at
    t=0, and the bot does NOT play a Rule-1 defensive card at t=0 (LogSiegeBot shows no defend until the
    player actually pushes onto the bot half). Fixing on `main` ONLY — the -testable branches are frozen
    snapshots; the fix lands going forward. build-master commits the re-saved L_Arena at integration.
    Acceptance: L_Arena saved clean; PIE match starts with zero stray actors; the M3 transient-Blue-unit
    WATCH is closed; no legitimate arena actor disturbed.
- names: >
    /Game/Maps/L_Arena (L_Arena.umap). Stray actors to remove: BP_Unit_Footman_C_1, BP_Unit_Miner_C_2,
    BP_Building_ArrowTower_C_1. Preserve: GoldNode_Blue (-1200,0,0), GoldNode_Red (+1200,0,0), Castle_Blue,
    Castle_Red, arena boundary volumes, PlayerStart, WorldSettings KillZ. Root cause: handoffs/TASK-069.md.

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

### M4 COMPLETE — 2026-07-04 (done; committed, not pushed; read this first)
All M4 tasks integrated + committed on `main`, **NOT pushed**: C++ batch TASK-053..060 at **65861ce** (via TASK-068, parent 56247c9, clean compile+link first try); editor/art TASK-061..067 + assembly TASK-069 at **e586699** (parent 65861ce). Milestone preserved on branch **m4-testable @ e586699** = the FULL-GAME superset M2+M3+M4 (earlier slices on **m3-testable @ 56247c9** + **m2-testable @ f903cf0**). TASK-053..069 all `done`; blocks left in place (not relocated to ## Done) to preserve the M4 audit trail.
- **Slice verified (TASK-069):** §9-4 expanded-roster slice — ZERO M4-feature FAILs. LIVE PASS: the bot self-drives Set II (repeated Ogre push, discards upgrades/Instants, never unaffordable / never Blue-half, Siege army killed the Blue castle 2000→0 in ~41 s), DT_Cards 22-row reconfirmed warm+cold (DeckCount=50, save-clean), and a real-PIE Defeat screen fired (closes the M2 "no PlayerController end-screen" watch).
- **DEFERRED to Jonathan's playtest** (each backed by a code-QA PASS; MCP cannot inject card-play): keyword fires (Charge/Slayer/Swarm/Suicide/Siege), Support heal, new-tower behaviors (Bomb AoE, Ballista blind spot), Barracks spawn+expire, Deep Mine +2/s, and hero-upgrade apply/stack/cap/refund + persist-through-death + reset-on-PlayAgain.
- **FOLLOW-UP found → TASK-070 (backlog, in ## Active tasks):** 3 stray verification actors committed into L_Arena.umap since M2 (BP_Unit_Footman_C_1, BP_Unit_Miner_C_2, BP_Building_ArrowTower_C_1) — present on ALL branches; cause the "stray Blue unit at match start" + a bot t=0 defend. Root cause in handoffs/TASK-069.md.

### M4 — COMPLETE (Jonathan greenlit M4 2026-07-04; developed on `main`; branches `m2-testable` @ f903cf0 + `m3-testable` @ 56247c9 + `m4-testable` @ e586699 preserve the milestone slices)
M4 = GDD §9-4 + §4 Set II (16 cards) + §3.0 keywords (Siege/Charge/Slayer/Swarm) + §3.8 Siege & Support profiles + §3.10 hero upgrades + §4 Bot Set II extension. **Slice:** expanded-roster combat clip — Ogre push vs Bomb Tower defense. Statuses are all `done` (code TASK-053..060 @ 65861ce, editor/art TASK-061..067 @ e586699, not pushed). **Every code task (053–060) implied a QA review** (standard qa loop); the chain ended at build-master integration (TASK-068 compile, TASK-069 final assembly).

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
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-053-report.md — 0 blk/0 warn/0 nit). CONFIRMED: header↔FCardRow 1:1 name+order (TASK-061 reimport will warn zero); DeckCount = exactly 50 (each ≥1 ≤MaxCopies); all 16 Set II rows §4 character-exact; 6 core rows byte-unchanged; keyword-column sparsity clean; no enum adds/shadowing. Data foundation for TASK-054-060. handoffs/TASK-053.md.
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
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-054-report.md — 0 blk/0 warn/3 nit; both interp calls ACCEPTED [Siege nearest-building map-wide, Support fallback follows combat units]). NON-REGRESSION CONFIRMED: Standard/melee/ranged/miners byte-identical M3 (Profile dispatch returns before untouched Standard body); castle Siege×2 before Projectile×0.5-castle-only; building Siege×2 branch, ScaledDamage==ActualDamage for non-Siege. Cleric heals 8/s≤400 clamped, never attacks. FreezeAI stops healing. C4458 clean, TASK-055 seam clean. handoffs/TASK-054-programmer.md. Ready for TASK-068 batch.
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
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-055-report.md — 0 blk/0 warn/3 nit). CONFIRMED: non-keyword ComputeOutputDamage bit-for-bit AttackDamage; ONE-hit charge (bChargePrimed consumed once); SINGLE detonation (bDetonated+bDead guards); drift-free aura (multiplier separate, restore 1.0f, FreezeAI/EndPlay clear); NO friendly-fire radial (GetTeamId==Team sole authority, closest-point reaches fortifications, routes through TakeDamage so Siege fires). Downstream signatures verified: FSiegeCombatStatics::ApplyRadialDamage (→056), ASummonedUnit::SetAuraDamageBonus (→058). No Build.cs. C4458 clean. NIT: fold the 3 closest-point mirrors into SiegeCombatStatics later (qa/TASK-026 NIT-4). handoffs/TASK-055.md. Ready for TASK-068 batch.
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
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-056-report.md — 0 blk/0 warn/2 nit). CONFIRMED: single-target/ArrowTower byte-unchanged (trailing default InAoERadius=0, sole 4-arg Archer caller binds, AoE branch gated AoERadius>0, ArrowTower MinRangeSq==0 inert); Bomb AoE 25-in-250 via ApplyRadialDamage (7-arg sig matches, closest-point catches primary); Ballista MinRangeSq skip ring [300,1400] max-gate intact; NO friendly fire (Bomb carries tower Team). Row-driven ATower (both BPs parent ATower, TASK-063). No Build.cs. C4458 clean. handoffs/TASK-056.md. Ready for TASK-068 batch.
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
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-057-report.md — 0 blk/1 warn/3 nit). M2/M3 miner economy BYTE-PRESERVED (symbol-traced: FlatIncomePerTick isolated, GetGoldRate base+miner unchanged, flat not overtime-doubled, ResetEconomy zeroes it, miner cap never gates DeepMines). Freeze step-2b CLEAN (additive, no double-handle, synchronous). WARN: mis-teamed DeepMine idle retry timer (bounded, never in designed flows). C4458 clean. CARRY→TASK-059: route Economy-typed DeepMine down building spawn path. handoffs/TASK-057-programmer.md. Ready for TASK-068 batch.
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
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-058-report.md — 0 blk/0 warn/1 nit). Hero non-regression byte-preserved @ 0 stacks: sprint 500/750 (ApplyMovementSpeed single live-writer + bSprinting; SwiftBoots composes 625/937.5), melee 20 (getter not hardcoded, TASK-016/017 feedback intact), HP 200+regen (all clamps route through GetEffectiveMaxHP, PlateArmor heals 100 clean), Rally untouched. 4 upgrades work, DRIFT-FREE (only 4 int32 stacks mutate), refund contract clean, caps from MaxCopies. WarBanner aura signature matches TASK-055. PlayAgain-reset gap VERIFIED clean 059 carry (ResetHero is shared respawn+PlayAgain path). NIT: 2 direct MaxWalkSpeed writes (HandleDeath/ctor) provably consistent. handoffs/TASK-058-programmer.md. Ready for TASK-068 batch. ⚠️CARRY→TASK-059: add Hero->ResetUpgrades() in SiegeGameMode::PlayAgain.
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
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-059-report.md — 0 blk/2 warn/2 nit; both flagged items ACCEPTED). CONFIRMED: melee-suppression intact all exit paths; net-zero refund all branches (Swarm unwinds copies on fail); existing Unit/Building/Miner/placement-v2 byte-preserved; apply-then-spend SAFE (CanAfford pre-check, ApplyUpgrade first, SpendGold only on Applied); SpawnUnitSwarm clean static (TASK-060 consumes as-is); IsBuildingCard routes DeepMine→building path (not miner-capped), Miner still unit; Masons 300/10 clamp+cancel clean. WARNs benign (post-match Masons tick on winner castle out-of-scope; off-navmesh ring fallback). C4458 clean. handoffs/TASK-059.md. Ready for TASK-068 batch.
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
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-060-report.md — 0 blk/1 warn/1 nit). Invariants CONFIRMED (never unaffordable, never Blue-half center); M3-core byte-preserved; rule-4 fix GENUINE (closes TASK-046 WARN-2 — no double/0 charge); SwarmCount via SpawnUnitSwarm (4 Red copies, unwind on fail). Type-unplayable-only discard RULED ACCEPTABLE (no deadlock — 50g start, income accrues; excluding banked units preserves "growing Ogre waves"). WARN: swarm-fan never-Blue-half contingent on Center.X≥radius (holds in L_Arena centerline 350; same shared-helper property as player/TASK-059; hard-clamp optional). handoffs/TASK-060.md. Ready for TASK-068 batch.
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
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04 (editor UP PID 6172). DT_Cards rebuilt to 22 rows (reference-safe add/set_rows from cards.csv, GUID+linkage preserved). VERIFIED: 22 rows no legacy/dupes, DeckCount sum=50, keyword cols set only where §4 (swarmCount 4 MilitiaMob; aoERadius 250 Sapper+BombTower; minRange 300 Ballista; bSlayer Pikeman/bCharge Cavalry/bSuicide Sapper; Barracks spawn triple; profiles Siege Sapper/Ogre, Support Cleric). BOOT NOTE: hung on stale Saved/Autosaves PackageRestoreData modal — declined restore + relaunched clean (~13min, no git residue). WATCH: live CSV auto-reimport → TASK-069 re-confirm 22 rows + save-clean before commit. handoffs/TASK-061.md.
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
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04 (editor UP PID 6172). 7 BPs in /Game/Blueprints/Units/ (MilitiaMob/Pikeman/Sapper/Cavalry/Longbowman/Cleric/Ogre), all ASummonedUnit, Footman recipe (-90 yaw, slot0 MI_TeamColor_Blue, no stats on BP), compiled clean. SIE stat verify PASS vs cards.csv: Ogre 500/250 Siege, Cavalry 140/600 bCharge, Pikeman 100/350 bSlayer, Sapper 60/500 Siege bSuicide, Longbowman 70/300 bRanged, Cleric 90/350 Support, MilitiaMob 25/400. Custom SM_MilitiaMob + SM_Longbowman confirmed wired (not fallbacks). Deferred to TASK-069: Siege/heal/Charge/Slayer/Swarm combat behavior. handoffs/TASK-062.md.
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
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04 (editor UP PID 6172). 4 BPs in /Game/Blueprints/Buildings/: BombTower+BallistaTower (parent ATower, row-driven), Barracks (ABarracks), DeepMine (ADeepMine); Team-default, slot0 MI_TeamColor_Blue, BlockAll + bCanEverAffectNavigation=true pinned (§3.7). Compiled clean. SIE verify: stats bind from DT_Cards (BombTower 180/25/800/2.5/AoE250, Ballista 120/45/1400/3.0/MinRange300, Barracks 250+spawn-triple, DeepMine 200/income2); bonus — match-end freeze log "1 barracks frozen, 2 towers silenced" confirms 056/057 freeze hooks. Deferred to TASK-069: AoE splash/blind-spot/Barracks spawn+self-destruct/DeepMine rate. MCP stable. handoffs/TASK-063.md.
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
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04 (editor UP PID 6172). Additive to WBP_HUD: 4 TextBlocks (Blade/Plate/Boots/Banner) + UpdateUpgradeEntry/UpdateUpgradeRow/SetupUpgradeRow fns; renders "Label cur/cap", collapses at 0; caps from GetUpgradeStackCap (not guessed); seed-then-bind FOnHeroUpgradesChanged on Tick do-once. VERIFIED: M1 gold Construct byte-INTACT + Rally indicator untouched (read_graph_dsl, zero EventGraph write); compile clean; full-match PIE zero errors, row attached. Live upgrade-pip → TASK-069/Jonathan (no MCP play-inject). MCP survived ~60 calls (1 ICE cascade handled). handoffs/TASK-064.md.
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
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04. 4 meshes in /Game/Meshes/ (Cavalry 804tris/208u, Pikeman 532/190, Sapper 620/169, MilitiaMob 452/149), feet-center, slot0 MI_TeamColor_Blue, ≤8k tris, distinct silhouettes, ZERO import warnings. MilitiaMob = CUSTOM (not reused Footman) → TASK-062 BP_Unit_MilitiaMob VisualMesh = /Game/Meshes/SM_MilitiaMob. Editor was free (no PIE) — no playtest disruption. FBX in Content/RawAssets/. Left untracked for TASK-069 commit. handoffs/TASK-065.md.
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
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04. SM_Ogre (1572tris, 290 tall — ROSTER's LARGEST), SM_Cleric (920, 182), SM_Longbowman (1984, 184). Feet-center, slot0 MI_TeamColor_Blue, ≤8k tris, ZERO import warnings, editor free (no disruption). Longbowman = CUSTOM (not reused Archer) → TASK-062 BP_Unit_Longbowman VisualMesh = /Game/Meshes/SM_Longbowman. FBX in Content/RawAssets/. Left untracked for TASK-069. handoffs/TASK-066.md.
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
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04. SM_BombTower (580tris/250×250×451), SM_BallistaTower (512/250×270×500), SM_Barracks (312/400×419×349), SM_DeepMine (1064/300×305×300). Ground-center, slot0 MI_TeamColor_Blue, ≤15k tris, TIGHT authored UCX = base footprint (overhangs outside hull, plinth-lesson honored), distinct silhouettes (DeepMine headframe ≠ GoldNode crystal), ZERO import warnings, editor free. ALL M4 ART DONE (065/066/067 = 11 meshes). FBX in Content/RawAssets/. Left untracked for TASK-069. handoffs/TASK-067.md.
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
- status: done (commit 65861ce on main, parent 56247c9, NOT pushed; 47 files +4825/-257). Clean compile+LINK FIRST TRY (~16.6s, zero C4456/57/58/59 + zero C4244, 14 TUs incl. 3 new pairs SiegeCombatStatics/Barracks/DeepMine). No donor re-saves; 11 M4 art meshes+FBX unstaged→untracked for TASK-069. Editor left DOWN on 65861ce DLL (TASK-061 boots it — path tracing already disabled → fast boot). TASK-053..060 code committed here (→done; manager wraps at M4 finish). handoffs/TASK-068.md. FOLLOW-UPS (manager): optional swarm own-half hard-clamp; off-navmesh swarm fallback.
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
- status: done (commit e586699 on main, parent 65861ce, NOT pushed; 44 files editor/art). ZERO M4-feature FAILs. LIVE bot Set II verified (repeated Ogre push; discards upgrades/Instants; never unaffordable/never-Blue-half; Siege army killed Blue castle 2000→0 ~41s; deck 50/6). DT_Cards 22-row reconfirmed warm+cold (DeckCount=50, save-clean). Keywords/upgrades/new-buildings DEFERRED-to-playtest (MCP no card-play inject), each backed by code QA PASS. BONUS: real-PIE Defeat screen fired → CLOSES M2 "no PlayerController end-screen" watch. Editor UP PID 10932 (cold-booted, clean). handoffs/TASK-069.md.
  - **FOLLOW-UP FOUND → TASK-070:** 3 stray verification actors (BP_Unit_Footman_C_1, BP_Unit_Miner_C_2, BP_Building_ArrowTower_C_1) are COMMITTED in L_Arena.umap since M2 (5bb9507/40b69ef) — present on ALL branches. Cause the "stray Blue unit at match start" + bot t=0 defend. Cleanup = remove 3 actors, re-save L_Arena, PIE-verify clean start.
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
