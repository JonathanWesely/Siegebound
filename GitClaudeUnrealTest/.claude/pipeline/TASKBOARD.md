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
4.5. **M4.5 — Gameplay terrain pass (grass, hills, trees + rocks)** — `SUPERSEDED 2026-07-10 by M6.5 (approach); climbable-terrain INTENT UNPARKED 2026-07-14 as M6.6` (Jonathan's 2026-07-10 battlefield directive changed the approach to RUNTIME PROCEDURAL RANDOMIZED scatter + 4× castle spacing; the M4.5 hand-placed mirror-symmetric plan is retired — its Terrain/Obstacle tag + placement-clearance [TASK-093] + projectile code contracts carry into M6.5, inert under the non-blocking default; see CONVENTIONS "Arena terrain & environment (M4.5)" supersede note). Original entry kept for history: — Jonathan directive 2026-07-08: the arena is "a boring white board"; wants "a nice large grass area with trees and hills". REAL GAMEPLAY TERRAIN — mirror-symmetric hills (physical high ground) + trees/rocks (navmesh/placement obstacles) INSIDE the playfield; amends GDD §5 (rulings in "M4.5 manager decisions" under Active tasks, incl. the same-day Fab amendment). **Fab pivot (Jonathan, same day):** he supplies premade Fab assets for FOUR slots — tree, rock, grass, hill (FAB-001..004 approved in .claude/pipeline/fab/FAB-REQUESTS.md; drop zone Content/Fab/README_DROP_ZONE.md). TASK-091/092 are now Fab conform+integration tasks blocked on his drop; TASK-093/094 (C++) remain valid and dispatchable. **ORDERING INVERSION (Jonathan's ruling): M5 proceeds AHEAD of this milestone — nobody blocks M5 work on M4.5.** M4.5 resumes the moment the Fab assets land. **UPDATE 2026-07-14 — INTENT UNPARKED as M6.6 "Climbable terrain" (TASK-138..145):** M4.5's core gameplay intent — hills as CLIMBABLE physical high ground (walkable low-angle faces + flat crowns, TASK-091..096's design) — is now delivered on top of the M6.5 procedural scatter via purpose-built convex hill meshes (`SM_Hill_01/02/03`) that replace the unclimbable `stone_hill` dome. The M4.5 hand-placed mirror-symmetric APPROACH stays retired (M6.5's asymmetric runtime scatter is the live placement model); only the walkable-terrain INTENT carries into M6.6.
5. **M5 — Spell system + Set III** — `done-pending-playtest (functionally complete 2026-07-08, TASK-097..109 all done; code commit 2c65164 + editor/art/docs commit 979f552 on main, NOT pushed; m5-testable @ 979f552; machine-verified, live spell/reticle/bot-cast items on the WATCH list below — need one unlocked-desktop human playtest to close the slice)` — targeting mode, 5 spells + Crystal Tower, spell Niagara VFX at the §6 bar, bot M5 spell rules. Carry-in baked into the specs (not a follow-up): the targeting reticle ground-projects via TRACE so M4.5's hills need no rework when they land. Slice: spell VFX showcase reel. **QA: 2 fail→fix→pass loops, both one-shot (TASK-098 BattleCry-magnitude seam ruled to the unit side; TASK-100 reticle decal double-rotation).**

### M5 CHECKPOINT — 2026-07-08 (playtest WATCH list — read before the human playtest)
M5 shipped machine-verified with the desktop LOCKED (SendInput blocked → no simulated input, TASK-076 doctrine). A single **unlocked-desktop human playtest** closes every item below. How to play: hotkeys 1–6 play hand slots, hold Left-Alt for the cursor; spells now enter TARGETING mode (reticle anywhere on the map, LMB confirm / RMB-Esc cancel); Play vs Bot from the menu, or open /Game/Maps/L_Arena. On boot, dismiss the two passive prompts: "source content changes — import?" → **Don't Import**; "re-open asset editors?" → **No**.
- ☐ Cast each of the 5 spells (Fireball AoE, FrostNova freeze, Lightning top-3-HP, BattleCry ally buff, Pickpocket instant gold-steal): reticle projects onto the ground, cost deducts at confirm, resolver-false fully refunds, 50% vs castle, no friendly fire, NS_Spell_* VFX reads in one frame at the §6 bar.
- ☐ Crystal Tower: place it, confirm the chain zap bounces (15/10/5 falloff, NS_ChainZap cyan) and the tower freeze-gate.
- ☐ Reticle readability on elevated anchors (TASK-100 WARN carry) + targeting/placement/Alt-cursor interplay feels clean.
- ☐ Bot casts spells: push 3+ units into a cluster (Fireball rule 3a) and put 2+ units by a player tower (Lightning rule 3b) — grep LogSiegeBot for "Rule 3a/3b". Also watch the spell hand-clog + centroid-outlier carries (TASK-102 WARNs).
- ☐ Play Again resets all spell state (freezes, buffs, reticle) alongside the existing reset.
- Benign (no action, informational): stale LogCSVImportFactory "missing CardType" warnings on load — cards.csv + live DT_Cards verified correct; it's the import-factory enum quirk that set_rows sidesteps.
5.5. **M5.5 — Overhead health bars** — `done-pending-playtest (2026-07-09, TASK-110..112 all done; commit 9a8a75f on main, NOT pushed [no branch — batch, not a milestone; m5-testable already preserves M5]. Functionally verified via PIE property readback; live on-screen bar appearance owed to Jonathan's playtest — see WATCH in TASK-112)` — Jonathan directive 2026-07-09 (direct in Claude Code): "add a health bar to every tower and character." A small standalone batch inserted between M5 and M6 (M4.5 shape), NOT a GDD milestone. Broadens GDD §7's enemy-only-when-damaged line to a floating overhead HP bar on EVERY combat actor, both teams — units (incl. miners), all buildings (towers, Wall, Barracks, Deep Mine), and the hero. Castles keep their existing M1 bar. Rulings + tasks in "M5.5 manager decisions" under Active tasks; naming law in CONVENTIONS "Overhead unit health bars (M5.5)". Jonathan's M6 go-ahead is given but M6 is decomposed AFTER this batch ships (separate step). **REOPENED 2026-07-09 by post-M6 playtest feedback (Item 1):** the hide-at-full behavior law is REVERSED to ALWAYS-VISIBLE (bar visible the entire time, fill drops as HP drops) + a fill bug fixed → TASK-122..124 in "M6 playtest feedback" under Active tasks; CONVENTIONS law updated. **UPDATE 2026-07-10: the overhead-bar feature was RE-REBUILT from scratch (TASK-130..132, castle push/delegate parity, RED enemy / BLUE friendly) and SHIPPED @ `61a1e72` (not pushed) — this retired the poll system AND the failed first-attempt fix (TASK-122/123/124/127/128, now SUPERSEDED). The overhead bars are `done` as of the rebuild.**
6. **M6 — Deck-builder meta** — `done-pending-playtest (2026-07-09, TASK-113..121 all done; ONE M6 commit 975ee90 on main, NOT pushed; m6-testable @ 975ee90 [4 ahead of origin]). Machine + live-PIE verified [deck-feed + fallback + bot-deck pick]; SaveGame save→relaunch persistence + live 28-tile grid click-through + cheat execs owed to Jonathan's playtest (locked desktop — see WATCH in TASK-120). 5 code tasks, 1 build-fix loop total [TASK-110-class was M5.5]; M6 had ZERO QA fail loops. OPEN CHECKPOINT ITEM: bot decks spell-free — QA recommends Lightning ×2 in Defensive Economy so the M5 bot-spell feature is exercised; Jonathan's call.` — Deck-builder screen (§7): browse the 28-card collection, add/remove copies with per-card MaxCopies enforced, live x/50 counter + average-cost guide (§8), save/load named decks (USaveGame, cross-session), a deck playable only at exactly 50; the active saved deck feeds the player's match, the bot gets 2 distinct curated decks; the `DeckCount` column is re-authored into a legal curated default that supersedes the M4/M5 test spread. **State preserved (undisturbed by M6):** M5 + M5.5 stay `done-pending-playtest` (m5-testable @ 979f552; M5.5 commit 9a8a75f; human WATCH lists still owed); M4.5 stays `parked` on Jonathan's Fab drop. Slice: UI/UX + save-load systems clip. **POST-M6 PLAYTEST FEEDBACK (2026-07-09) → TASK-122..126 in the current milestone** ("M6 playtest feedback" block under Active tasks): Item 1 = overhead health bars now always-visible + fill-drops fix (TASK-122..124); Item 2 = deck-builder tiles render as physical cards (TASK-125..126, blocked-by Item 1 per Jonathan's ordering). **UPDATE 2026-07-10: both post-M6 feedback chains SHIPPED — deck-builder physical-card tiles (TASK-125/126/129) committed @ `274c160`; the health-bar effort was ultimately delivered by the TASK-130..132 REBUILD @ `61a1e72` (which retired the failed first-attempt TASK-122/123/124/127/128). Current HEAD `61a1e72`, not pushed. All M6 + feedback tasks `done`.**
6.5. **M6.5 — Battlefield & procedural terrain** — `done (2026-07-14 — Jonathan committed the assembled battlefield HIMSELF as `6a4c17d "battlefield created"` and PUSHED it; this SATISFIES the TASK-136/137 held-commit gate — no separate build-master M6.5 commit. GATE 0 for M6.6 is thereby satisfied. NOTE: an `m6.5-testable` branch was never cut at the self-commit — TASK-145 cut it retroactively at 6a4c17d alongside m6.6-testable @ 057ca9f (DONE 2026-07-14).)` (decomposed 2026-07-10, TASK-133..137; Jonathan directive verbatim: *"start creating the terrain/battlefield the characters fight on … space out the castles … 4 times larger … a grassy terrain filled with rocks, trees, and hills … randomly generated at the start of each match … choose what you think will look best … use as many assets as possible … for variety"*). Standalone milestone after M6 (M4.5/M5.5 sub-milestone shape); SUPERSEDES the parked M4.5. **PART 1** = 4× castle spacing (±2000 → ±8000, gold nodes ∓1200 → ±7200, PlayerStart + navmesh + ground + boundary walls widened — CONVENTIONS "World axes" updated). **PART 2** = a RUNTIME procedural scatter (`ASiegeBattlefieldScatter` + `USiegeScatterConfig`/`DA_BattlefieldScatter` + `M_BattlefieldGround`) of trees/rocks/hills/grass soft-referenced from Jonathan's imported Fab packs, re-seeded each match. **Decisions ANSWERED by Jonathan 2026-07-10:** (1) obstacles **BLOCK** unit movement + carve the navmesh (Dynamic RecastNavMesh + a NON-NEGOTIABLE castle-to-castle traversability guarantee); (2) grass **material** (not a Landscape); (3) keep-clear zones **YES**; (4) 4× pacing **PROCEED**. Placement is **ASYMMETRIC organic random** (Jonathan ruling, flagged — can switch to mirror-symmetric at playtest if unfair). Details in the "M6.5 tasks" block under Active tasks. Naming law in CONVENTIONS "Battlefield & procedural terrain (M6.5)" + "World axes (arena contract)".
6.6. **M6.6 — Climbable terrain** — `done (2026-07-14 — playtested + signed off by Jonathan: hero climbs the hill flanks + anti-exploit gate passes [enemy melee reaches a crowned hero] + camera/tower/escape/perf all good. Committed by Jonathan HIMSELF as `057ca9f "walkable terrain"` and PUSHED [self-commit, same pattern as M6.5]; TASK-138..145 all done. m6.6-testable @ 057ca9f + m6.5-testable @ 6a4c17d cut. Committed L_Arena carries the STALE serialized nav bake [umap byte-identical to pre-widen 6a4c17d] but non-breaking — runtime-Dynamic RecastNavMesh regenerates at PIE. Non-blocking follow-ups for manager: (i) manual Build>Navigation is required after any arena-bounds change [MCP has no nav-build tool]; (ii) scatter density reads thin on the wider ±4000 field — Trees ~15/55, Grass ~1450/2500 — optional tuning pass.)` (decomposed 2026-07-14, TASK-138..145; **UNPARKS the M4.5 "Gameplay terrain pass" intent**). Jonathan wants the battlefield hills/rocks CLIMBABLE by the hero — root-cause investigation established this is currently BY-DESIGN (M6.5's scatter built every rock/hill/tree as a route-around blocker) and that the parked M4.5 TASK-091..096 already specified exactly this feature, so M6.6 delivers the parked M4.5 intent on top of the M6.5 procedural scatter. **ROOT CAUSE (corrects the earlier jump-height hypothesis):** the scatter applies UNIFORM scale (`BattlefieldScatter.cpp:247`, `FVector(Scale)`) → face angles are SCALE-INVARIANT; the squashed `stone_hill` dome goes near-vertical at the rim → unclimbable regardless of jump. FIX = purpose-built CONVEX hill meshes (`SM_Hill_01/02/03`) with ≤30° faces + flat crowns, under BOTH the character's 44.76° WalkableFloorAngle AND Recast's 44° AgentMaxSlope, so hero AND units climb with essentially no movement retune. **FOUR DECISIONS LOCKED (Jonathan, 2026-07-14):** (1) M6.5 already committed by Jonathan @ `6a4c17d` (pushed) — GATE 0 satisfied, NO build-master M6.5 commit; (2) widen arena Y ±2400 → ±4000; (3) terrain BLOCKS projectiles (arrows die on rocks/hills/tree-trunks — accepted balance change); (4) units climb too (navmesh generates over hills — closes the melee-can't-reach-a-crowned-hero exploit). Authoritative plan on disk: `C:\Users\wesel\.claude\plans\we-last-left-off-partitioned-puppy.md`. Naming law in CONVENTIONS "Climbable terrain (M6.6)". Details in "M6.6 tasks" under Active tasks.
7. M7 — Premium art & feel pass — `in-progress (decomposed 2026-07-15 — TASK-153..188 in "## M7 tasks" under Active tasks; 1 asset [Ogre] already SHIPPED via the pull-forward below; CONVENTIONS "Skeletal rig & animation workstream (M7)" + the M7 batch/GoldNode-variant clauses added FIRST)` · **OGRE PULLED FORWARD 2026-07-14 (TASK-147..152):** Jonathan dropped an ogre concept (`Tools/ArtPipeline/Inbox/ogre.png`) and directed the validated TRELLIS.2 pipeline be run NOW to replace the `SM_Ogre` blockout with a game-ready textured mesh — one of the 16 M7 blockouts pulled ahead on his directive (chain in "M7 pull-forward — Ogre textured mesh" under Active tasks). The rest of the M7 batch (15 blockouts + the premium/feel pass) stays deferred. · **Jonathan request (2026-07-04):** raise fidelity on SM_Castle + SM_Footman + SM_Archer (higher detail than the current blockouts); wants the game to look nicer. Decision: DEFERRED here (mesh swaps are non-breaking; roster still growing through M4-M6). Two integration paths to scope at M7: (a) art-director custom higher-detail Blender models, and/or (b) **Fab/UE-marketplace assets — Jonathan must download packs into the project via the Epic Launcher first (agents can't browse/buy/download Fab autonomously); art-director then swaps meshes/materials.** Could be pulled forward as a standalone art pass after M3/M4 if Jonathan wants it sooner.
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

**2026-07-15 M7 KICKOFF (READ FIRST — CURRENT milestone) — Premium art & feel pass (TASK-153..188):** Jonathan authorized M7 in full. This is the whole §6 premium bar: (A) §6 juice C++ checklist; (B) a NEW skeletal rig + animation workstream stood up from scratch (spike-proven before batch); (C) the 16 remaining blockouts upgraded to textured meshes via the proven TRELLIS.2 pipeline; (D) Niagara VFX on every ability/impact/spawn/death/spell; (E) Lumen lighting + post stack + gradient skybox + arena set dressing (no collision change, §5); (F) full audio set; (G) the Sequencer cinematic flythrough + gameplay b-roll portfolio slice. CONVENTIONS updated FIRST (TASK-153, done): the "Skeletal rig & animation workstream (M7)" section + the M7-batch and GoldNode-emissive-variant clauses under "Textured mesh law". Full decomposition + rulings in **"## M7 tasks"** below. **Dispatch frontier (parallel-safe, headless, NOW): TASK-154, 155, 156, 157, 158 (juice C++) ∥ TASK-159 (skeletal swap path C++) ∥ TASK-179 (audio hooks C++) ∥ TASK-160 (rig spike, Blender) ∥ TASK-166 (mesh-batch prep, file-only) ∥ TASK-174/175 (VFX art, editor-queued).** **2026-07-16 DIRECTIVE AMENDMENTS (TASK-184..188 — see "M7 DIRECTIVES ADDENDUM" below) opened more NOW lanes and closed two gates:** additionally dispatchable NOW: **TASK-184** (concept-gen tool, C++/tooling) ∥ **TASK-180** (4 covered audio cues, art) ∥ **TASK-186** (best-effort audio from imported packs, art) ∥ **TASK-187** (fire/ice VFX re-skin, art/editor). The concept gate is RESOLVED by the concept-gen step (TASK-184 build → TASK-185 run → 16 Inbox PNGs; **TASK-167 is now OPTIONAL/non-blocking** review). The audio gate is PARTIALLY resolved (MedievalWeaponsSFX pack) — the ONLY remaining external Jonathan gate is **TASK-188** (7 genuinely-missing cues: mining loop, 2 UI clicks, castle-destroy, 2 music, overtime sting — NON-blocking, M7 ships silent+logged if ungated). Repo base `057ca9f` on `main` (pushed); the Ogre pull-forward already committed to `main` bundled with the M6.6 scatter tune (see TASK-152, not pushed).

---

## M7 tasks (decomposed 2026-07-15) — Premium art & feel pass

**Authorization:** Jonathan's 2026-07-15 M7 go-ahead (the "most ambitious path"). Milestone GDD §9-7 + the full §6 "premium stylized" bar + §3.9 castle crumble + §3.8 skeletal-animation replacement of the procedural lunge + §5 non-collision set dressing. Hard gates stand: code rides the QA gate (qa-reviewer, shadow-law + complete-type-include scans) before compile; nothing commits without a PASS QA report (code) / completed integration (art); editor/MCP work needs 127.0.0.1:8000 up and Blender+Lab addon on 9876 (park + tell the orchestrator if unreachable — never fake); `HF_TOKEN` is ENV-ONLY; heavy Blender runs HEADLESS. Nothing pushed unless Jonathan says so.

### M7 manager decisions (binding rulings for all M7 tasks)
1. **SM_GoldNode is IN-SCOPE** (all 16 blockouts upgraded — the most ambitious path) BUT under the EMISSIVE ECONOMY-PROP VARIANT, NOT the standard two-slot TeamRegion conform: it is the §"Team contract" team-color EXCEPTION (emissive regardless of team; `AGoldNode` is a level prop that never runs the slot-0 recolor), so it gets a SINGLE-slot PBR + preserved warm-yellow emissive (`MI_GoldNode_PBR`, keep `M_GoldGlow` OR bake `T_GoldNode_E`), `team_region:null`, building path. Recorded in CONVENTIONS "Textured mesh law" → "GoldNode emissive economy-prop VARIANT". Rationale: forcing GoldNode through the TeamRegion contract would fake a team accent on a team-agnostic prop and lose §6's "gold glows warm yellow (emissive)".
2. **Concept-image source = AUTOMATED concept-gen step (Jonathan AUTHORIZED 2026-07-16 — directive 3; SUPERSEDES the hand-drop gate).** The 16 concepts are now GENERATED by `Tools/ArtPipeline/concept_generate.py` (BUILT in TASK-184, RUN in TASK-185) into `Tools/ArtPipeline/Inbox/<CardID>.png` — a text→image tool mirroring `trellis_generate.py` (HF PRO, ENV-only token). The Stage-1 blocker on TASK-168..171 is now **TASK-185 (concepts generated)**, NOT a Jonathan drop. TASK-167 becomes Jonathan's OPTIONAL, NON-blocking review (he may replace any PNG before its wave runs; silence = accept). Still does NOT block the juice C++ (154..158), the skeletal swap path (159), the rig spike (160), the audio hooks (179), or the file-only mesh prep (166). Casing is a non-issue — the tool writes PascalCase `<CardID>.png` directly. Law: CONVENTIONS "Textured mesh law" → "Stage 0 — concept generation".
3. **Rig-spike-BEFORE-batch is MANDATORY (mirrors the Ogre static proof).** TASK-160 stands up rig/anim tooling and rigs+animates ONE character (Footman — already textured) end-to-end; TASK-161 is a JONATHAN EYEBALL GATE on the animated result; NO batch rigging (163/164) starts until the spike is APPROVED. The SkeletalMeshComponent swap path (159) is separate headless C++ and ships independently so the spike has a code path to integrate into (162).
4. **Audio-source = PARTIALLY RESOLVED by the imported "MedievalWeaponsSFX" pack (2026-07-16 — directive 1).** The pack (`Content/MedievalWeaponsSFX/`) is a WEAPON-IMPACT/WHOOSH pack ONLY. It covers 4 of the 14 §6 cues → **TASK-180 (rewritten) authors those 4 SoundCues at `/Game/Audio/S_<Event>` NOW** (no gate). 3 more are sourceable from OTHER imported packs / a weak in-pack placeholder → **TASK-186 (S_SpellCast, S_UnitSpawn, weak S_CastleHit)**, dispatchable now. The remaining 7 (mining LOOP, 2 UI clicks, castle-destroy, 2 music, overtime sting) exist in NO current pack → **TASK-188 = external gate on Jonathan** (supply/approve a source). The audio TRIGGER HOOKS (TASK-179, C++) are dispatchable NOW and null-safe, so covered cues light up as they land and the gaps stay silent+logged. Full coverage map: CONVENTIONS "Audio event cues (M7)". Gap list recorded in the M7 directives decisions note below.
5. **QA is implied per code task (no separate IDs — board precedent).** Every gameplay-programmer task (154..159, 179) routes `ready-for-qa` → qa-reviewer → `qa/TASK-###-report.md`; the shadow-law (C4457/58/59) + complete-type-include-law scans are mandatory. The M7 code compiles in ONE build-master batch (TASK-182) after those pass; the milestone ends in the build-master final-assembly integration (TASK-183). All juice/crumble/gold-burst/audio C++ references its art (materials/VFX/sounds) by SOFT path, null-safe, so it compiles and ships before the art lands.

### M7 DIRECTIVES ADDENDUM — Jonathan's 4 directives, 2026-07-16 (folded into the board; new IDs TASK-184..188)

Jonathan gave four M7 directives on 2026-07-16, resolving two of the milestone's external gates (concepts + audio) and adding two content re-skins. A content inventory established ground truth (paths trusted, spot-verified: FrostNova DeckCount 0 / Fireball DeckCount 2 in cards.csv; `trellis_generate.py` present as the tool model; Content/ is gitignored so pack contents are taken from the inventory).

- **Directive 1 — Audio (MedievalWeaponsSFX pack) → resolves PART of the TASK-180 gate.** The pack covers 4 of 14 cues (authored now, TASK-180); 3 sourceable from other imported packs / a weak in-pack placeholder (TASK-186); 7 genuinely missing (TASK-188, Jonathan gate). Coverage map: CONVENTIONS "Audio event cues (M7)". Decision #4 rewritten above.
- **Directive 2 — Fire/Ice VFX RE-SKIN (NOT new cards).** Fireball (cards.csv row 24) + Frost Nova (row 25) ALREADY exist from M5. `USpellLibrary::ResolveSpell` (SpellLibrary.cpp:77-78) spawns the CardID-COMPOSED path `/Game/VFX/NS_Spell_<CardID>`, so the re-skin MUST land IN PLACE at `/Game/VFX/NS_Spell_Fireball` (← Fire_Magic explosion/AoE) + `/Game/VFX/NS_Spell_FrostNova` (← Ice_Magic shockwave/frozen/snowstorm) — **TASK-187**. TASK-175 (spell VFX polish) hands Fireball+FrostNova to TASK-187 to avoid double-work. Law: CONVENTIONS "Spells & Set III (M5)" → "Spell VFX element re-skin (M7)".
- **Directive 3 — Concept-gen step (Jonathan AUTHORIZED) → resolves the TASK-167 gate.** BUILD `concept_generate.py` (TASK-184) + RUN it → 16 Inbox PNGs (TASK-185); TASK-167 becomes Jonathan's OPTIONAL review. Decision #2 rewritten above. Law: CONVENTIONS "Textured mesh law" → "Stage 0 — concept generation".
- **Directive 4 — Imported-content REUSE ledger (Jonathan: "use anything useful").** Annotated onto the relevant tasks; consolidated here:

  | Imported pack | Reuse | Task(s) annotated |
  |---|---|---|
  | `Content/MedievalCastleEnvironmentAndSiegeWeaponProps/` | castle meshes → `SM_Castle_Crumble01/02/03` crumble stages; siege props → arena set dressing | TASK-157, TASK-178 |
  | `Content/Prickly_Knight/` | RIGGED knight (Anim/Mesh/skins) → potential skeletal DONOR (Knight/Footman) — art-director evaluates vs. `rig_character.py` | TASK-163 |
  | `Content/sA_ArcheryVfxPack/` | arrow trails/impact → projectile juice VFX | TASK-174 |
  | `Content/Realistic_Rocks/` | rock chunks → castle-debris meshes paired with `NS_CastleDebris` | TASK-157, TASK-174, TASK-178 |
  | `Content/sA_StylizedWizardSet/` | wizard cast/muzzle VFX → spell-cast VFX + possibly `S_SpellCast` source | TASK-175, TASK-186 |
  | `Content/Fab/Stone_Hills_FREE/` + Megascans | scenic backdrop for §6 lighting | TASK-176, TASK-177 |
  | **GAP — no dedicated skybox pack** | TASK-177 authors the stylized gradient sky FROM SCRATCH (`M_Skybox` / Sky Atmosphere) — flagged | TASK-177 |

**DECISIONS NEEDING JONATHAN (consolidated — orchestrator surfaces in 🚨 Blockers):**
- **D-AUDIO-GAP (→ TASK-188):** 7 §6 cues exist in NO imported pack — `S_MinerClink` (mining LOOP), `S_CardPlay` + `S_CardDiscard` (UI clicks), `S_CastleDestroyed` (destruction stinger), `S_VictoryMusic` + `S_DefeatMusic` (music), `S_OvertimeSting` (7:00 stinger). Jonathan supplies/approves a royalty-free source, OR we ship these silent+logged (null-safe — no crash). Separately, 3 cues (`S_SpellCast`, `S_UnitSpawn`, weak `S_CastleHit`) are art-director best-effort from imported packs (TASK-186) — flag if none prove usable.
- **D-FROSTNOVA-DECK:** Frost Nova is `DeckCount 0` (VERIFIED — cards.csv row 25) → NOT in the default curated deck, so the new ice VFX is NEVER SEEN in normal play. Recommend bumping FrostNova DeckCount (e.g. +1, re-balanced to keep sum==50, each ≤ MaxCopies) and/or adding it to a bot deck so the effect shows. Small balance/deck call — Jonathan's. (Fireball is `DeckCount 2` — its fire VFX WILL show; no action.)
- **D-FIREICE-SCOPE:** Default M7 scope = VFX RE-SKIN of the two EXISTING spells (TASK-187). If Jonathan actually wants NEW dedicated hero fire/ice ABILITIES (beyond the existing Fireball/FrostNova cards), that is a separate feature (new C++ + cards + data) — flagged as an OPEN decision, NOT assumed. Default proceeds as re-skin only.

Dispatch shape: **TASK-153 (manager CONVENTIONS) lands FIRST (done).** Then, in parallel: the HEADLESS CODE WAVE (154..159, 179 — juice + skeletal path + audio hooks, all null-safe, QA'd, compiled together at 182); the RIG SPIKE (160→161 gate→162 integ); the MESH PREP (166) + concept gate (167) → PRODUCTION waves (168..171, headless, quota-paced) → per-wave eyeball → IMPORT waves (172/173, editor-serial); the VFX art (174/175) and LIGHTING/POST/SKYBOX/DRESSING (176..178, editor-serial). BATCH RIGGING (163/164) gates on the spike-approve + the relevant textured meshes; its integration is 165. AUDIO assets (180) gate on the audio source. The SEQUENCER slice (181) is the capstone — blocked on the full visual pass. Build-master compiles the code batch (182) and does the final assembly + §6-checklist PIE + 60 fps@1440p perf watch + commit + `m7-testable` branch (183).

#### TASK-153 — CONVENTIONS: skeletal/animation law + M7 batch + GoldNode variant (manager)
- assignee: manager
- status: **done** (2026-07-15 — CONVENTIONS "Skeletal rig & animation workstream (M7)" section written + live, plus the "M7 batch scope" and "GoldNode emissive economy-prop VARIANT" clauses under "Textured mesh law". Must land before 159/160/166 — it does. This decomposition is the deliverable.)
- blocked-by: none
- parallel-safe: yes
- spec: >
    Add the naming law for the NEW skeletal/animation workstream BEFORE any task issues it: `SK_<CardID>` → Content/Characters/;
    `A_<CardID>_<Action>` anim sequences (Idle/Walk/Attack/Death); `AM_<CardID>_Attack` montage; `ABP_<CardID>` AnimBlueprint;
    shared `SKEL_SiegeBiped`; and the `SkeletalVisualMesh` swap contract (soft-ref parity with the static `SM_<CardID>` ghost path).
    Add the M7 16-blockout batch scope + the GoldNode emissive-prop variant to the "Textured mesh law" section.
- names: >
    CONVENTIONS.md "Skeletal rig & animation workstream (M7)" + "Textured mesh law" additions.

#### TASK-154 — Juice: hit-flash on damage (0.1 s white material swap) (C++)
- assignee: gameplay-programmer
- status: done/integrated (TASK-182 build PASS 2026-07-16, local commit f313253, no push) ← was: qa-passed (loop-3 fix 2026-07-16 gameplay-programmer: removed embedded */ in SiegeFeedbackLibrary.h:39 comment — reworded "raw UWorld*/context" → "raw UWorld pointer / context" so the /** */ class doc comment (opened L20) now closes only at L41; UCLASS(L42)/GENERATED_BODY(L45) are back at file scope, no longer "in a skipped block". PLUS full UHT-hazard sweep of all 10 batch headers + their .cpp — all 4 hazard classes CLEAN: (1) no other embedded */ / stray /*; (2) UPROPERTY/UFUNCTION specifiers legal incl. MinerUnit.h:155 AllowPrivateAccess; (3) all UCLASS/UENUM/GENERATED_BODY at correct scope, no macro-in-comment/#if; (4) no deprecated APIs, includes complete (Engine/World.h present). Ready for TASK-182 recompile, NOT committed.) [prior BUILD loop-3/TASK-182: the L39 */ closed the doc comment early → UHT fatal under -WarningsAsErrors; MinerUnit.h:155 loop-2 fix CONFIRMED. loop-1: added #include "Engine/World.h" to SiegeHitFlashComponent.cpp, batch include self-audit CLEAN]
- blocked-by: none
- parallel-safe: yes
- spec: >
    §6 juice — every damage event flashes the hit actor white for 0.1 s then restores. Add a shared hit-flash on the combatant
    base classes (`ASummonedUnit`, `ABuilding`, `AHeroCharacter`, `ACastle`) driven from their EXISTING TakeDamage / HP-mutation
    paths (reuse the M5.5-rebuild `OnHPChanged` broadcast points — do NOT re-plumb damage). On a damage event, swap every material
    slot to a soft-referenced flash material `/Game/Materials/M_HitFlash` (null-safe — missing ⇒ no flash, log once, never a crash)
    for 0.1 s, then restore the prior materials (cache slot MIDs at BeginPlay; restore includes the team-recolored slot 0). Skip
    heals/regen and refused/friendly-fire mutations (flash on ACTUAL damage only). Timer-driven, no per-tick cost. Works on both the
    static `VisualMesh` and (M7) the `SkeletalVisualMesh` when active. Duration is a `UPROPERTY(EditDefaultsOnly) HitFlashSeconds`
    = 0.10 (`// GDD §6`). ACCEPTANCE: an actor taking damage flashes white ~0.1 s and returns to its exact prior look incl. team
    tint; heal does not flash; no crash with M_HitFlash absent. QA implied (shadow + include scans). Post in ⚙️ Dev & QA.
- names: >
    Soft ref `/Game/Materials/M_HitFlash` (art TASK-174 provides; null-safe). Hook the existing `OnHPChanged` broadcast points on
    `ASummonedUnit`/`ABuilding`/`AHeroCharacter`/`ACastle`. Law: CONVENTIONS "Overhead combatant health bars — REBUILT" (delegate
    points), "Textured mesh law" (slot 0 team recolor).

#### TASK-155 — Juice: spawn squash-and-stretch (0.15 s) + tower recoil on fire (C++)
- assignee: gameplay-programmer
- status: done/integrated (TASK-182 build PASS 2026-07-16, local commit f313253, no push) ← was: ready-for-qa (QA loop 1 2026-07-16: qa-failed → fixed missing #include "Engine/World.h" in SiegeHitFlashComponent.cpp; batch include self-audit CLEAN → back to ready-for-qa) → qa-passed (loop-1 re-review PASS 2026-07-16)
- blocked-by: none
- parallel-safe: yes
- spec: >
    Two procedural mesh-transform juice items, NO new assets (pure C++). (1) SPAWN SQUASH-AND-STRETCH: on spawn, units and buildings
    play a 0.15 s squash→overshoot→settle scale animation on their visual mesh (`UPROPERTY SpawnSquashSeconds` = 0.15 `// GDD §6`);
    driven by a timeline/curve or timer-lerp on RelativeScale3D, restoring to the authored scale exactly. (2) TOWER RECOIL: `ATower`
    kicks its `VisualMesh` back a short distance opposite its fire direction on each shot and eases back before the next shot
    (`UPROPERTY TowerRecoilDistance` + `TowerRecoilSeconds`, sane defaults, `// GDD §6`), hooked into the EXISTING fire cadence — do
    NOT alter targeting/damage. Both null-safe and frame-rate-independent; both operate on whichever visual mesh is active. ACCEPTANCE:
    a spawned Footman/tower visibly squash-stretches and settles at correct scale; a firing Arrow Tower recoils and returns each shot
    with unchanged fire timing/damage. QA implied. Post in ⚙️ Dev & QA.
- names: >
    `ASummonedUnit`/`ABuilding` spawn hook; `ATower` fire hook. No new assets. Law: CONVENTIONS "Per-card visual assets" (`VisualMesh`).

#### TASK-156 — Juice: floating damage numbers (C++ spawner) (C++)
- assignee: gameplay-programmer
- status: done/integrated (TASK-182 build PASS 2026-07-16, local commit f313253, no push) ← was: qa-passed (loop-4 fix: brace-init soft-ptr decls (vexing-parse) + full .cpp-body sweep — DamageNumberActor.cpp:78 now `WidgetClass{ FSoftObjectPath(...) }`, breaking the C2228 most-vexing-parse. Full 14-file .cpp compile-stage sweep done, no other hazards. See qa/TASK-154-159-179-qa.md loop-4 + handoffs/TASK-154-159-179-programmer.md.)
- blocked-by: none
- parallel-safe: yes
- spec: >
    §6 floating damage numbers. On every ACTUAL damage event, spawn a short-lived world-space number that rises + fades over the hit
    actor showing the damage dealt. C++ owns the spawn/lifetime/animation and drives a soft-referenced widget `/Game/UI/WBP_DamageNumber`
    (created in the editor wiring task, TASK-176-adjacent — soft, null-safe: missing ⇒ no number, log once). Reuse the existing damage
    magnitude at the TakeDamage seam; a `UFUNCTION(BlueprintCallable) ShowDamageNumber(float Amount, FVector WorldLocation)`-style seam
    or a pooled `UWidgetComponent`/`UDamageNumberComponent` — programmer's call, keep it cheap (pool or cap concurrent numbers for the
    60-units §6 perf budget). BIE params to the widget are float/int only (MCP BP-param rule). Optional team/crit tint via float RGB.
    ACCEPTANCE: damaging an actor spawns a rising, fading "-N" over it matching the dealt amount; no leak/uncapped growth under 60 units;
    no crash with the widget absent. QA implied. Post in ⚙️ Dev & QA.
- names: >
    Soft ref `/Game/UI/WBP_DamageNumber` (editor-wired later; null-safe). Optional `UDamageNumberComponent`
    (`Source/GitClaudeUnrealTest/Siegebound/`). Law: CONVENTIONS "Widgets with C++ bases" (float-only BIE params).

#### TASK-157 — Juice: castle crumble stages 75/50/25 % (C++ threshold + swap + debris trigger) (C++)
- assignee: gameplay-programmer
- status: done/integrated (TASK-182 build PASS 2026-07-16, local commit f313253, no push) ← was: ready-for-qa (QA loop 1 2026-07-16: qa-failed → fixed missing #include "Engine/World.h" in SiegeHitFlashComponent.cpp; batch include self-audit CLEAN → back to ready-for-qa) → qa-passed (loop-1 re-review PASS 2026-07-16)
- blocked-by: none
- parallel-safe: yes
- spec: >
    GDD §3.9 castle crumble. In `ACastle`, detect the 75 % / 50 % / 25 % max-HP thresholds ON THE WAY DOWN (fire each stage once, in
    order, off the existing `FOnCastleHPChanged` path — never on heal-back-up or reset; Play Again restores stage 0 and re-arms all
    thresholds). At each stage: swap the castle mesh AND/OR material to the damaged variant (soft refs `/Game/Meshes/SM_Castle_Crumble0N`
    and/or `/Game/Materials/MI_Castle_Crumble0N`, N=1..3; null-safe — missing ⇒ keep current look, log once) and trigger a debris burst
    Niagara `/Game/VFX/NS_CastleDebris` at the castle (soft, null-safe). Thresholds are `UPROPERTY` defaults (`// GDD §3.9`). The
    collision/UCX footprint is UNCHANGED by a crumble swap (visual only — do not alter placement/pathing). ACCEPTANCE: driving a castle
    through 75/50/25 % fires each stage exactly once in order with the mesh/material change + debris FX; Play Again resets to full and
    re-arms; no double-fire on chip damage across a threshold; no crash with crumble assets absent. QA implied. Post in ⚙️ Dev & QA.
- names: >
    `ACastle` thresholds off `FOnCastleHPChanged`. Soft refs `/Game/Meshes/SM_Castle_Crumble01..03`,
    `/Game/Materials/MI_Castle_Crumble01..03`, `/Game/VFX/NS_CastleDebris` (art TASK-171-adjacent + TASK-174; null-safe). REUSE (directive 4):
    crumble-stage castle meshes/materials source from `Content/MedievalCastleEnvironmentAndSiegeWeaponProps/` (damaged castle variants); the
    `NS_CastleDebris` burst pairs `Content/Realistic_Rocks/` rock chunks as debris meshes. Law:
    CONVENTIONS "Delegates (C++)".

#### TASK-158 — Juice: gold-coin burst on unit kills + screen shake on castle hits (C++)
- assignee: gameplay-programmer
- status: done/integrated (TASK-182 build PASS 2026-07-16, local commit f313253, no push) ← was: ready-for-qa (QA loop 1 2026-07-16: qa-failed → fixed missing #include "Engine/World.h" in SiegeHitFlashComponent.cpp; batch include self-audit CLEAN → back to ready-for-qa) → qa-passed (loop-1 re-review PASS 2026-07-16)
- blocked-by: none
- parallel-safe: yes
- spec: >
    Two event-driven cosmetics. (1) GOLD-COIN BURST: when a summoned unit dies, spawn a coin-burst Niagara `/Game/VFX/NS_GoldBurst` at
    its location (soft, null-safe), hooked into the EXISTING unit-death path — cosmetic only, no gold mutation. (2) SCREEN SHAKE ≤0.2 s
    ON CASTLE HITS: when a castle takes ACTUAL damage, play a brief client camera shake (reuse the `/Game/Variant_Combat/...
    BP_CameraShake_Hit_Enemy` donor OR a new `BP_CameraShake_CastleHit`, ≤0.2 s, `// GDD §6`) via the local `APlayerController`
    (`ClientStartCameraShake`), null-safe. Skip heals/reset/friendly-fire. ACCEPTANCE: a dying unit emits a coin burst (no gold change);
    a castle-damage event kicks a short camera shake ≤0.2 s and none on heal/reset; no crash with the VFX/shake absent. QA implied.
    Post in ⚙️ Dev & QA.
- names: >
    Soft refs `/Game/VFX/NS_GoldBurst` (art TASK-174). Camera shake donor `/Game/Variant_Combat/.../BP_CameraShake_Hit_Enemy` or new
    `BP_CameraShake_CastleHit`. Hook `ASummonedUnit` death + `ACastle` damage. Law: CONVENTIONS "Template-donor rule" (shake donor).

#### TASK-159 — Skeletal swap path: SkeletalVisualMesh on ASummonedUnit (C++)
- assignee: gameplay-programmer
- status: done/integrated (TASK-182 build PASS 2026-07-16, local commit f313253, no push) ← was: qa-passed (loop-4 fix: brace-init soft-ptr decls (vexing-parse) + full .cpp-body sweep — SummonedUnit.cpp:236 now `SkSoft{ FSoftObjectPath(...) }` AND the previously-hidden identical hazard at SummonedUnit.cpp:252 `AbpSoft{ FSoftObjectPath(...) }` (would have been the next C2228 once :236 compiled). Full 14-file .cpp compile-stage sweep done, no other hazards. See qa/TASK-154-159-179-qa.md loop-4 + handoffs/TASK-154-159-179-programmer.md.)
- blocked-by: none
- parallel-safe: yes
- spec: >
    The code path that lets a `SK_<CardID>` skeletal mesh replace the static `SM_<CardID>` `VisualMesh` at runtime WITHOUT breaking the
    placement-ghost soft-ref contract (units resolve `/Game/Meshes/SM_<CardID>` by string today). Add an OPTIONAL `USkeletalMeshComponent`
    named `SkeletalVisualMesh` to `ASummonedUnit` alongside `VisualMesh`. At BeginPlay compose the soft path `/Game/Characters/SK_<CardID>`
    from the CardID; if it RESOLVES: set it on `SkeletalVisualMesh`, set `AnimClass` = `/Game/Characters/ABP_<CardID>` (soft, null-safe),
    HIDE the static `VisualMesh`, and route the BeginPlay team recolor (`MI_TeamColor_<Team>` on slot 0) to `SkeletalVisualMesh`; if it
    does NOT resolve, keep the static `VisualMesh` exactly as today (purely additive, null-safe). The PLACEMENT GHOST is UNCHANGED — it
    still resolves the static `/Game/Meshes/SM_<CardID>` (ghosts don't animate). No CSV column (path composed from CardID). Include the
    complete `SkeletalMeshComponent.h` / `AnimInstance` headers (complete-type-include law). ACCEPTANCE: a unit with a valid `SK_<CardID>`
    +`ABP_<CardID>` shows the skeletal mesh (team-recolored slot 0) and the ghost still previews the static mesh; a unit with no SK asset
    is byte-for-byte today's behavior; enemy Red unit recolors slot 0 only. QA implied (shadow + include scans — this is exactly the
    class of task that tripped TASK-110). Post in ⚙️ Dev & QA.
- names: >
    `ASummonedUnit::SkeletalVisualMesh` (`USkeletalMeshComponent`). Soft refs `/Game/Characters/SK_<CardID>`,
    `/Game/Characters/ABP_<CardID>`. Slot-0 recolor `MI_TeamColor_<Team>`. Ghost path `/Game/Meshes/SM_<CardID>` UNCHANGED. Law:
    CONVENTIONS "Skeletal rig & animation workstream (M7)".

#### TASK-160 — Rig SPIKE: stand up rig/anim tooling + rig+animate ONE character (Footman) end-to-end (art)
- assignee: art-director
- status: ready-for-integration (2026-07-15 — SPIKE COMPLETE, GATED at TASK-161. New tooling Tools/ArtPipeline/rig_character.py + rig_manifest.json (shared SiegeBiped 21-bone skeleton, bone-heat skin w/ envelope fallback, Idle/Walk/Attack/Death). Footman: SK_Footman rigged FBX + 4 anim FBXs → Content/RawAssets/Characters/[Anims/]; two-slot [TeamRegion,FootmanPBR]+UVMap preserved; skin 0.0% unweighted; all round-trip-verified. Eyeball-gate packet (bind stills + turntable strip + 4 contact strips + seq frames + report) in Cache/Footman/rig/previews/. NO editor import (TASK-162, after the gate + TASK-159 compiled). Handoff: handoffs/TASK-160-artist.md.)
- blocked-by: none
- parallel-safe: yes (Blender headless — Footman is already textured; disjoint from all code)
- spec: >
    Prove the skeletal pipeline on ONE character before any batch, exactly as the Ogre proved static. Stand up rig/anim tooling from
    scratch (there is none today) and rig+animate the FOOTMAN (already textured `SM_Footman`, simplest humanoid). Deliver: a skeleton
    (prefer shared `SKEL_SiegeBiped`), the rigged `SK_Footman` (from the textured `SM_Footman`, two-slot `[TeamRegion, FootmanPBR]`
    material contract preserved, ≤15k tris, feet-center, `UVMap`), the four anim sequences `A_Footman_Idle/Walk/Attack/Death`, and an
    attack montage `AM_Footman_Attack`. Raw rigged FBX → `Content/RawAssets/Characters/Footman.fbx`. Heavy Blender runs HEADLESS
    (`blender.exe --background --python`; the live MCP bridge is <30 s inspection only). Produce Blender-rendered PREVIEW clips of each
    anim for the TASK-161 eyeball gate. Record the tooling/approach (auto-rig vs manual, retarget strategy) + chosen skeleton in the
    handoff so it generalizes to the batch. DO NOT import to the editor here (that + the ABP is TASK-162, after the gate). ACCEPTANCE:
    `SK_Footman` rigged FBX + the 4 anims + montage exist; preview clips render; two-slot material contract intact; approach documented.
    Post in 🎨 Art.
- names: >
    `Content/RawAssets/Characters/Footman.fbx` (rigged). Skeleton `SKEL_SiegeBiped` (or `SK_Footman_Skeleton`). Anims
    `A_Footman_Idle/Walk/Attack/Death`, montage `AM_Footman_Attack`. Preview clips for the gate. Law: CONVENTIONS "Skeletal rig &
    animation workstream (M7)".

#### TASK-161 — EYEBALL GATE: rigged Footman animation sign-off (Jonathan — external gate)
- assignee: Jonathan (external gate — orchestrator posts the preview clips + rig report in 🚨 Blockers and flips this on his verbatim go)
- status: approved (Jonathan approved the Footman rig — unblocks TASK-162 integration + TASK-163/164 batch rigging)
- blocked-by: TASK-160
- parallel-safe: yes (human review — no repo mutation by agents)
- spec: >
    Jonathan reviews the Footman rig/anim PREVIEW clips (idle/walk/attack/death + the attack montage) BEFORE any editor import or batch
    rigging — the spike-before-batch gate (decision 3). Confirms the animation quality + silhouette read at the §6 bar and that the
    approach is worth generalizing. OUTCOME: APPROVE → unblocks the spike integration (TASK-162) AND the batch rigging (TASK-163/164);
    TUNE → art-director iterates the rig/anims (TASK-160 loops) and re-review. NEVER batch-rig on an un-approved spike.
- names: >
    Review TASK-160 preview clips + handoff. Approve/Tune. Law: CONVENTIONS "Skeletal rig & animation workstream (M7)".

#### TASK-162 — Rig spike integration: import SK_Footman + ABP_Footman, wire BP_Unit_Footman, PIE-prove real attack anim (art, editor)
- assignee: art-director
- status: integrated / done (build-master, commit b33dbc9 — LOCAL only, no push, 2026-07-16). Committed: SK_Footman + SK_Footman_Skeleton + SK_Footman_PhysicsAsset + A_Footman_{Idle,Walk,Attack,Death} + ABP_Footman + BP_Unit_Footman (SkeletalVisualMesh transform). Idle/Walk DONE + PIE-proven skeletal render. Attack/Death anims imported but NOT wired → follow-up W3 (gameplay-programmer montage hook: Montage_Play(AM_Footman_Attack) on attack tick + death-anim window before Destroy; art: AM montage + ABP Slot node). See handoffs/TASK-162-artist.md)
- blocked-by: TASK-160, TASK-161 (approved), TASK-159 (compiled — the SkeletalVisualMesh path must exist; via TASK-182)
- parallel-safe: no (editor-mutating — single editor, serialize)
- spec: >
    Editor import + wiring of the approved Footman rig, on the TASK-159 swap path. Import `SK_Footman` → `/Game/Characters/SK_Footman`
    (two slots `[TeamRegion → MI_TeamColor_Blue, FootmanPBR → MI_Footman_PBR]`, Nanite off), the four `A_Footman_*` sequences +
    `AM_Footman_Attack` → `/Game/Characters/Anims/`, and author `ABP_Footman` → `/Game/Characters/ABP_Footman` (Idle/Walk by velocity →
    Attack montage slot → Death). Confirm `BP_Unit_Footman` picks up the skeletal runtime via the TASK-159 CardID-composed path (no per-BP
    hardcoding needed) and the static `/Game/Meshes/SM_Footman` STILL backs the placement ghost. PIE-prove: a spawned Footman plays the
    real `AM_Footman_Attack` on its attack tick (replacing the TASK-020 procedural lunge), walks with locomotion, recolors slot 0 by team
    (Red enemy), and the ghost preview is the static mesh. ACCEPTANCE: Footman animates from real skeletal anims in-match; ghost unchanged;
    team recolor correct; procedural-lunge fallback still fires if the montage is absent. Post in 🎨 Art; hand to build-master.
- names: >
    `/Game/Characters/SK_Footman` (slots [TeamRegion, FootmanPBR]), `/Game/Characters/Anims/A_Footman_*`, `AM_Footman_Attack`,
    `/Game/Characters/ABP_Footman`. Reuse `BP_Unit_Footman` + `MI_Footman_PBR` + `MI_TeamColor_Blue`. Ghost `/Game/Meshes/SM_Footman`
    UNCHANGED. Law: CONVENTIONS "Skeletal rig & animation workstream (M7)".

#### TASK-163 — Batch rig wave 1: rig+animate 5 units (art)
- assignee: art-director
- status: backlog
- blocked-by: TASK-161 (spike approved), TASK-172 (wave-1 textured meshes imported — rig the final mesh)
- parallel-safe: yes (Blender headless, per-asset disjoint; quota-free — rigging is local Blender)
- spec: >
    Rig+animate 5 summoned units on the TASK-160 approach, each FROM its textured `SM_<CardID>` (two-slot material contract preserved):
    `Knight`, `Archer`, `Cavalry`, `Pikeman`, `MilitiaMob`. Per unit deliver `SK_<CardID>` (rigged FBX → `Content/RawAssets/Characters/<CardID>.fbx`),
    `A_<CardID>_Idle/Walk/Attack/Death` (retarget shared `SKEL_SiegeBiped` locomotion where humanoid; per-card attack/death silhouettes),
    `AM_<CardID>_Attack`, and preview clips. Headless Blender. DO NOT import (editor wiring is TASK-165). Record skeleton choice per asset.
    (Archer is already textured from the pilot; the other 4 come from TASK-172.) ACCEPTANCE: 5 rigged FBX + anims + montages exist,
    two-slot contracts intact, previews render. Post in 🎨 Art.
- names: >
    `SK_Knight/Archer/Cavalry/Pikeman/MilitiaMob` + `A_<CardID>_*` + `AM_<CardID>_Attack` + `Content/RawAssets/Characters/<CardID>.fbx`.
    Skeleton `SKEL_SiegeBiped`. REUSE (directive 4): art-director EVALUATES `Content/Prickly_Knight/` (a RIGGED knight — Anim/Mesh/skins) as a
    skeletal DONOR for `Knight` (and possibly the shared biped) vs. building from `rig_character.py`; use whichever hits the §6 bar faster, record
    the choice in the handoff. Law: CONVENTIONS "Skeletal rig & animation workstream (M7)".

#### TASK-164 — Batch rig wave 2: rig+animate remaining units (art)
- assignee: art-director
- status: backlog
- blocked-by: TASK-161 (spike approved), TASK-172 (unit textured meshes imported)
- parallel-safe: yes (Blender headless, per-asset disjoint)
- spec: >
    Rig+animate the remaining summoned units: `Sapper`, `Cleric`, `Longbowman`, `Miner`, `Ogre`. Same deliverables as TASK-163
    (`SK_<CardID>` + `A_<CardID>_Idle/Walk/Attack/Death` + `AM_<CardID>_Attack` + rigged FBX + previews; retarget where humanoid). The
    OGRE is non-humanoid/large — it may carry its own `SK_Ogre_Skeleton` and bespoke anims (record it). Miner's "attack" is the mining
    animation (§3.3 clink loop pairs with TASK-179 audio). Ogre is already textured; the others come from TASK-172. Headless Blender; no
    import (TASK-165 wires). ACCEPTANCE: 5 rigged FBX + anims + montages exist, contracts intact, previews render. Post in 🎨 Art.
- names: >
    `SK_Sapper/Cleric/Longbowman/Miner/Ogre` + `A_<CardID>_*` + `AM_<CardID>_Attack` + `Content/RawAssets/Characters/<CardID>.fbx`.
    Skeleton `SKEL_SiegeBiped` (Ogre may be bespoke). Law: CONVENTIONS "Skeletal rig & animation workstream (M7)".

#### TASK-165 — Batch rig integration: import + ABP + BP wiring for all rigged units, PIE-verify (art, editor)
- assignee: art-director
- status: in-progress (PARKED 2026-07-16 — editor modal-blocked; needs Jonathan). Scope this pass = 8 units (Knight, Cavalry, Pikeman, MilitiaMob, Sapper, Cleric, Longbowman, Miner); Archer/Ogre separate. Pre-import gate PASSED for all 8 (rig_reports clean, deform bones == Footman, shared skeleton valid). DONE: SK_Knight imported+materialed on shared SK_Footman_Skeleton + A_Knight_{Idle,Walk,Attack,Death} clean AnimSequences. BLOCKER: `BlueprintTools.create` for an AnimBlueprint opened a modal skeleton-picker that froze the editor game thread (all MCP calls time out) — Jonathan must dismiss the "Create Anim Blueprint" dialog. TOOLING SNAG: `AssetTools.duplicate` STRIPS an AnimBlueprint's TargetSkeleton (verified) + no MCP tool can set it back (ObjectTools redirects to CDO) → no unattended path to per-unit ABPs. Junk to delete on resume: ABP_ZTest, ABP_ZTest2(?), ABP_Knight (skeleton-less). Full detail + resume plan: handoffs/TASK-165-artist.md.
- blocked-by: TASK-163, TASK-164, TASK-159 (compiled), TASK-162 (spike integ pattern proven)
- parallel-safe: no (editor-mutating — single editor, serialize)
- spec: >
    Import every rigged unit from waves 1+2 → `/Game/Characters/SK_<CardID>` (two slots `[TeamRegion → MI_TeamColor_Blue, <CardID>PBR →
    MI_<CardID>_PBR]`, Nanite off), the `A_<CardID>_*` sequences + `AM_<CardID>_Attack` → `/Game/Characters/Anims/`, and author
    `ABP_<CardID>` (or reuse a shared `ABP_SiegeBiped` retargeted). Each `BP_Unit_<CardID>` picks up its skeletal runtime via the TASK-159
    CardID path; the static `SM_<CardID>` still backs each ghost. PIE-verify a representative spread (a humanoid, the Ogre, the Miner):
    real attack montage on the attack tick, locomotion on walk, team recolor slot 0, ghost = static mesh, procedural-lunge fallback intact
    where a montage is missing. ACCEPTANCE: all rigged units animate in-match from real skeletal anims; ghosts unchanged; team recolor
    correct; no per-BP hardcoding. Post in 🎨 Art; hand to build-master (TASK-183).
- names: >
    `/Game/Characters/SK_<CardID>` (all rigged units) + `/Game/Characters/Anims/A_<CardID>_*` + `AM_<CardID>_Attack` + `ABP_<CardID>`
    (or `ABP_SiegeBiped`). Reuse each `BP_Unit_<CardID>` + `MI_<CardID>_PBR`. Law: CONVENTIONS "Skeletal rig & animation workstream (M7)".

#### TASK-166 — Mesh batch prep: 16 manifest entries + blockout measure + concept casing (art)
- assignee: art-director
- status: ready-for-integration (2026-07-15 — 16 manifest entries authored with HEADLESS-MEASURED blockout dims; GoldNode team_region:null; JSON parses; existing Footman/Archer/Ogre/Castle + defaults untouched. Building tri budget set to 20000 (GoldNode 12000). Concept casing reconcile is a no-op until TASK-167 drops land. Handoff: handoffs/TASK-166-artist.md. Rides the TASK-183 commit.)
- blocked-by: none
- parallel-safe: yes (file-only — edits pipeline_manifest.json, reads Content/RawAssets/*.fbx; disjoint)
- spec: >
    File-side prep for the 16-mesh batch (mirrors the Ogre TASK-147, batched). NO editor/MCP, NO HF quota. For each of the 16 CardIDs
    (`Knight, Miner, Cavalry, Cleric, Longbowman, MilitiaMob, Pikeman, Sapper` UNIT; `ArrowTower, Wall, BombTower, BallistaTower, Barracks,
    DeepMine, CrystalTower` BUILDING; `GoldNode` PROP): (1) MEASURE the existing blockout `Content/RawAssets/<CardID>.fbx` bounds (Blender
    headless / MCP <30 s) BEFORE any Stage-2 overwrites it → the `target_dims_ue` source. (2) AUTHOR the `pipeline_manifest.json` entry —
    UNIT path (feet-center, tri 15000, bake 1024, two-slot team_region ~0.35 upper-body guess) / BUILDING path (ground-center, tri per
    building budget, bake 2048, `ucx` authored at import, two-slot team_region) / GoldNode PROP (`team_region:null`, single PBR+emissive
    variant per CONVENTIONS, building path, bake 2048) — each with a measured `target_dims_ue`, `pre_rotate_z_deg:0.0` STARTING GUESS, and
    a `_dims_source` note. (3) Concept casing is now a NO-OP: `concept_generate.py` (TASK-184/185) writes PascalCase `Inbox/<CardID>.png` directly; only reconcile if Jonathan HAND-replaces a concept with a lowercase filename (TASK-167 optional review).
    Keep VALID JSON (manifest is CODE — rides the TASK-183 commit). Do NOT touch Footman/Archer/Castle/Ogre entries or `defaults`.
    ACCEPTANCE: 16 complete manifest entries with MEASURED dims; GoldNode entry has `team_region:null`; JSON parses; no other entries changed.
    Post in 🎨 Art.
- names: >
    Edit `Tools/ArtPipeline/pipeline_manifest.json` (+16 entries). Read `Content/RawAssets/<CardID>.fbx` (16). AssetName = CardID
    (PascalCase). Law: CONVENTIONS "Textured mesh law" ("M7 batch scope" + "GoldNode emissive economy-prop VARIANT").

#### TASK-167 — CONCEPT REVIEW (OPTIONAL, non-blocking): Jonathan reviews/replaces the generated concepts (Jonathan — external, optional)
- assignee: Jonathan (OPTIONAL external review — orchestrator posts the generated concept thumbnails in 🚨 Blockers; NON-blocking — silence = accept)
- status: backlog
- blocked-by: TASK-185 (concepts generated into Inbox/)
- parallel-safe: yes (external — no agent repo mutation)
- spec: >
    REWRITTEN 2026-07-16 (Jonathan AUTHORIZED the automated concept-gen step — directive 3). The 16 concepts are NO LONGER hand-dropped:
    they are GENERATED by `concept_generate.py` (built in TASK-184, run in TASK-185) into `Tools/ArtPipeline/Inbox/<CardID>.png`. This task
    is now Jonathan's OPTIONAL review — he may inspect the generated PNGs and REPLACE any before its TRELLIS production wave consumes it, but
    the mesh batch does NOT wait on his review (silence = accept). The Stage-1 blocker on TASK-168..171 is now TASK-185 (concepts EXIST), not
    this review. Per-wave: replacing a concept before its wave runs is honored; after a wave has run, dropping a replacement re-triggers that
    asset only. Casing is a non-issue — the tool writes PascalCase `<CardID>.png` directly. Naming character-for-character = the CardID.
- names: >
    Review/replace `Tools/ArtPipeline/Inbox/<CardID>.png` × 16 (Knight, Miner, Cavalry, Cleric, Longbowman, MilitiaMob, Pikeman, Sapper,
    ArrowTower, Wall, BombTower, BallistaTower, Barracks, DeepMine, CrystalTower, GoldNode). Law: CONVENTIONS "Textured mesh law" (Concepts,
    "Stage 0 — concept generation").

#### TASK-168 — Production wave U1 (Stage 1+2): Knight, Cavalry, Pikeman, MilitiaMob (art)
- assignee: art-director
- status: ready-for-integration (2026-07-16 — Stage 1+2 COMPLETE, all 4 clean. Knight/Cavalry/Pikeman/MilitiaMob: 15000 tris (budget 15000), slots [TeamRegion, <CardID>PBR], UVMap ✓, feet-center (min_z≈0), team-region 2.8-9.1% (cap 35%), D/N/ORM 1024² + refine_report + 5 previews each; concepts copied → Concepts/. Warn-only X/Y dim deviation (height fit keeps Z exact; Pikeman pike widens X, Cavalry horse deepens Y — flag at eyeball gate). FBX at Content/RawAssets/<CardID>.fbx. STAGE 3 IMPORT = TASK-172 (editor, separate). NOT imported, NOT committed.)
- blocked-by: TASK-166 (manifest), TASK-185 (generated concepts for these 4; Jonathan may pre-replace via TASK-167)
- parallel-safe: yes (Bash + headless Blender; HF quota serializes Stage-1 in practice — orchestrator paces, exit 3 = quota pause/resume)
- spec: >
    Run the TRELLIS.2 pipeline Stage 1 (`uv run trellis_generate.py --check` then `trellis_generate.py <CardID>`) → Stage 2
    (`blender.exe --background --python refine_trellis_glb.py -- --asset <CardID>`) for the 4 units, per the Ogre playbook
    (handoffs/TASK-086.md / TASK-148/149). Outputs per asset OVERWRITE the blockout: `Content/RawAssets/<CardID>.fbx` (two slots
    `[TeamRegion, <CardID>PBR]`, ≤15k tris, feet-center, `UVMap`) + `Content/RawAssets/Textures/<CardID>/*.png` (D/N/ORM 1024²) +
    `Cache/<CardID>/refine_report.json` + previews. Copy each accepted concept → `Content/RawAssets/Concepts/<CardID>.png`. HF_TOKEN
    ENV-ONLY; surface non-zero exit codes VERBATIM in 🚨 Blockers (2 token · 3 quota-pause · 4 API-drift/manual-fallback · 5 image-missing),
    never fake. `pre_rotate_z_deg`/`team_region` are STARTING GUESSES — produce previews for the per-wave eyeball checkpoint (decision 3
    pattern); if obviously wrong, record the finding for the checkpoint, don't loop blindly. ACCEPTANCE: 4 FBX (two slots, budgets, UVMap,
    feet-center) + D/N/ORM + refine_reports + previews; report tris/bounds vs manifest. Post in 🎨 Art. DO NOT import (TASK-172).
- names: >
    Stage 1/2 for `Knight, Cavalry, Pikeman, MilitiaMob`. Outputs `Content/RawAssets/<CardID>.fbx` + `.../Textures/<CardID>/*` +
    `Cache/<CardID>/*` + `Content/RawAssets/Concepts/<CardID>.png`. Law: CONVENTIONS "Textured mesh law".

#### TASK-169 — Production wave U2 (Stage 1+2): Sapper, Cleric, Longbowman, Miner (art)
- assignee: art-director
- status: in-progress (2026-07-16 — TRELLIS mesh batch running headless (combined driver, after U1 completed). Stage 1+2 for Sapper, Cleric, Longbowman, Miner. Quota-paced, halts on exit 3.)
- blocked-by: TASK-166, TASK-185 (generated concepts for these 4; Jonathan may pre-replace via TASK-167)
- parallel-safe: yes (Bash + headless Blender; quota-paced)
- spec: >
    As TASK-168 for the 4 units `Sapper, Cleric, Longbowman, Miner` (UNIT path). Same Stage-1+2 flow, outputs, exit-code discipline, and
    per-wave eyeball previews. ACCEPTANCE: 4 FBX (two slots, ≤15k tris, UVMap, feet-center) + D/N/ORM + refine_reports + previews + concepts
    copied. Post in 🎨 Art. DO NOT import (TASK-172).
- names: >
    Stage 1/2 for `Sapper, Cleric, Longbowman, Miner`. Outputs as TASK-168. Law: CONVENTIONS "Textured mesh law".

#### TASK-170 — Production wave B1 (Stage 1+2): ArrowTower, Wall, BombTower, BallistaTower (art)
- assignee: art-director
- status: in-progress (2026-07-16 — queued in the combined mesh-batch driver after U2. BUILDING path (ground-center, 2048² bakes, tri_budget 20000). Quota-paced.)
- blocked-by: TASK-166, TASK-185 (generated concepts for these 4; Jonathan may pre-replace via TASK-167)
- parallel-safe: yes (Bash + headless Blender; quota-paced)
- spec: >
    As TASK-168 for the 4 BUILDINGS `ArrowTower, Wall, BombTower, BallistaTower` (BUILDING path: ground-center, 2048² bakes, two-slot
    `[TeamRegion, <CardID>PBR]`; the authored `UCX_SM_<CardID>` wall-footprint-exact hulls are done at import, TASK-172-analog TASK-173,
    NOT here). Same Stage-1+2 flow + exit-code discipline + per-wave eyeball previews. ACCEPTANCE: 4 FBX (two slots, building budget, UVMap,
    ground-center) + D/N/ORM 2048² + refine_reports + previews + concepts copied. Post in 🎨 Art. DO NOT import (TASK-173).
- names: >
    Stage 1/2 for `ArrowTower, Wall, BombTower, BallistaTower`. Outputs as TASK-168 (2048² textures, ground-center). Law: CONVENTIONS
    "Textured mesh law".

#### TASK-171 — Production wave B2 (Stage 1+2): Barracks, DeepMine, CrystalTower, GoldNode (emissive variant) (art)
- assignee: art-director
- status: in-progress (2026-07-16 — queued in the combined mesh-batch driver after B1. BUILDING path. GoldNode single-slot flag WILL be checked at its refine (script may still emit 2 slots on team_region:null per TASK-171 note — will flag if so). Quota-paced.)
- blocked-by: TASK-166, TASK-185 (generated concepts for these 4; Jonathan may pre-replace via TASK-167)
- parallel-safe: yes (Bash + headless Blender; quota-paced)
- spec: >
    As TASK-170 for `Barracks, DeepMine, CrystalTower` (BUILDING path, two-slot) PLUS `GoldNode` under the EMISSIVE ECONOMY-PROP VARIANT
    (decision 1 / CONVENTIONS): GoldNode gets a SINGLE PBR slot `GoldNodePBR` (NO TeamRegion) with the warm-yellow emissive preserved
    (keep `M_GoldGlow` OR produce a `T_GoldNode_E` emissive PNG for `MI_GoldNode_PBR`), `team_region:null`. Same Stage-1+2 flow + exit-code
    discipline + per-wave eyeball previews. ACCEPTANCE: 3 building FBX (two slots) + GoldNode FBX (single PBR slot + emissive) + textures
    + refine_reports + previews + concepts copied; GoldNode has NO TeamRegion slot. Post in 🎨 Art. DO NOT import (TASK-173).
- names: >
    Stage 1/2 for `Barracks, DeepMine, CrystalTower` (two-slot) + `GoldNode` (single `GoldNodePBR` slot + emissive, `team_region:null`).
    Emissive `T_GoldNode_E` or keep `M_GoldGlow`. Law: CONVENTIONS "GoldNode emissive economy-prop VARIANT".

#### TASK-172 — Import wave: 8 UNIT meshes (Stage 3, overwrite SM_<CardID> in place) (art, editor)
- assignee: art-director
- status: done (INTEGRATED 2026-07-16 @ commit 2dc8031, build-master — 8 unit SM_ meshes + T_/MI_PBR committed as LFS pointers, LOCAL-ONLY per Jonathan, NO push; reimport Tools/ scripts are CODE and remain pending separate QA before any push. Prior import note: ALL 8 UNIT MESHES NOW TEXTURED + AUTOMATED (gameplay-programmer, TASK-172/173 combined reimport wave). U1 (Knight/Cavalry/Pikeman/MilitiaMob) verified UNCHANGED via MCP readback (15000 tris, [TeamRegion→MI_TeamColor_Blue, <CardID>PBR→MI_<CardID>_PBR], Nanite off, ← BP_Unit_<CardID>). U2 (Sapper/Cleric/Longbowman/Miner) NEWLY reimported via the EXTENDED Tools/reimport_meshes.py: per-CardID textures T_<CardID>_{D,N,ORM} imported (D sRGB / N normal-map / ORM linear-Masks) + MI_<CardID>_PBR created from M_AssetPBR (BaseColor/Normal/ORM wired) + same-path SM overwrite + Nanite OFF + ≤4 convex hulls; slots [TeamRegion→MI_TeamColor_Blue, <CardID>PBR→MI_<CardID>_PBR]. MCP readback: all 8 units 15000 tris (U2 up from 452–1984 blockout), correct slots/MIs, Nanite off, HARD REF survived (each SM ← BP_Unit_<CardID>, object path unchanged so ghost/cards.csv string refs resolve). Editor-bounce (authorized): MCP save-all → graceful CloseMainWindow → headless commandlet on unlocked project → relaunch → MCP material finalize + verify. Reimport SCRIPTS extended = CODE → QA (Tools/reimport_meshes.py + Tools/reimport_finalize_materials_mcp.py). handoffs/TASK-173-programmer.md.)
- blocked-by: TASK-168, TASK-169 (unit FBX + textures + per-wave eyeball APPROVED)
- parallel-safe: no (editor-mutating — single editor, serialize; per-wave Jonathan eyeball precedes import)
- spec: >
    Unreal MCP editor import (serialized) for the 8 units (Knight, Cavalry, Pikeman, MilitiaMob, Sapper, Cleric, Longbowman, Miner), per
    the Ogre import playbook (handoffs/TASK-151.md). `M_AssetPBR` already exists — do NOT re-author. Per asset: import textures →
    `/Game/Textures/T_<CardID>_D` (sRGB), `_N` (normal), `_ORM` (LINEAR — sRGB OFF, the manual flip); create
    `/Game/Materials/Instances/MI_<CardID>_PBR` from `M_AssetPBR` (BaseColor/Normal/ORM); import `Content/RawAssets/<CardID>.fbx`
    OVERWRITING `/Game/Meshes/SM_<CardID>` at the SAME PATH (never delete+recreate — the BP_Unit_<CardID> + cards.csv + placement-ghost
    soft refs MUST survive; the same-path overwrite is the human Content-Browser Reimport click per the TASK-086/151 mechanism unless an
    MCP reimport route exists — flag the click in 🚨 Blockers if needed); slots EXACTLY `[0] TeamRegion → MI_TeamColor_Blue, [1]
    <CardID>PBR → MI_<CardID>_PBR`; Nanite OFF; ≤4-hull collision; zero import/MikkTSpace warnings; UVMap present. ACCEPTANCE: each
    `SM_<CardID>` IS the textured mesh at its UNCHANGED path with correct slots/MIs/collision; readbacks reported. Post in 🎨 Art;
    hand to build-master (TASK-183).
- names: >
    `/Game/Meshes/SM_<CardID>` (8 units, same-path overwrite) + `/Game/Textures/T_<CardID>_{D,N,ORM}` + `/Game/Materials/Instances/MI_<CardID>_PBR`
    (from `/Game/Materials/M_AssetPBR`). Slots [TeamRegion → MI_TeamColor_Blue, <CardID>PBR → MI_<CardID>_PBR]. Reuse each BP_Unit_<CardID>
    + cards.csv row. Law: CONVENTIONS "Textured mesh law".

#### TASK-173 — Import wave: 7 BUILDINGS + GoldNode (Stage 3, overwrite SM_<CardID>; UCX + emissive) (art, editor)
- assignee: art-director
- status: in-progress (6/8 INTEGRATED 2026-07-16 @ commit 2dc8031, build-master — 5 buildings + GoldNode SM_ meshes + T_/MI_PBR committed as LFS pointers, LOCAL-ONLY NO push; Wall + DeepMine PENDING (not refine-ready — await TRELLIS quota for their textured FBX); reimport Tools/ scripts are CODE pending separate QA before any push. Prior import note: 5 BUILDINGS + GoldNode REIMPORTED + AUTOMATED (6/8, gameplay-programmer). ArrowTower/BallistaTower/Barracks/BombTower/CrystalTower + GoldNode swapped to their refined textured meshes via the EXTENDED Tools/reimport_meshes.py (per-CardID collision mode, editor-bounce authorized). Buildings = 2-slot [TeamRegion→MI_TeamColor_Blue, <CardID>PBR→MI_<CardID>_PBR] + textures T_<CardID>_{D,N,ORM} + explicit UCX-analog BOX hull authored from pipeline_manifest.json ucx.boxes (wall-footprint, 1 box each — NOT unit auto-hulls). GoldNode = the emissive VARIANT: single slot GoldNodePBR→M_GoldGlow (warm-yellow emissive PRESERVED, NO TeamRegion/TeamColor) + box hull. Nanite OFF all. MCP readback: buildings 20000 tris / GoldNode 12000 tris (all UP from 222–1212 blockout), correct slots/mats, refs intact (BP_Building_<CardID> / GoldNode←L_Arena). Thumbnails confirm textured stone tower + blue team roof + warm-yellow glowing GoldNode (Saved/Screenshots/M7_ReimportWave/). FLAG (non-blocking, textured mesh shipped): CrystalTower crystal-glow (M_CrystalGlow) NOT preserved — M_AssetPBR has NO emissive param + the refined FBX authored only 2 slots + no T_CrystalTower_E baked; crystal reads blue via PBR albedo but does not emit. Needs an art-director emissive pass (T_CrystalTower_E + emissive-capable master, OR a dedicated glow slot authored into the FBX) — matches manifest _emissive_note. PENDING (2/8): Wall + DeepMine are NOT refine-ready (no baked D/N/ORM textures) — reimport via the same automation once their textured FBX land. Reimport SCRIPTS = CODE → QA. handoffs/TASK-173-programmer.md.)
- blocked-by: TASK-170, TASK-171 (building/prop FBX + textures + per-wave eyeball APPROVED)
- parallel-safe: no (editor-mutating — single editor, serialize)
- spec: >
    As TASK-172 for the 7 buildings (ArrowTower, Wall, BombTower, BallistaTower, Barracks, DeepMine, CrystalTower) — BUILDING path: 2048²
    textures, slots `[TeamRegion → MI_TeamColor_Blue, <CardID>PBR → MI_<CardID>_PBR]`, AUTHOR explicit `UCX_SM_<CardID>` wall-footprint-exact
    collision (bounds within ±10 % of the blockout — the M1 castle-plinth dead-zone lesson), Nanite OFF — PLUS `GoldNode` under the EMISSIVE
    VARIANT: single slot `GoldNodePBR → MI_GoldNode_PBR` (from `M_AssetPBR`, warm-yellow emissive via kept `M_GoldGlow` or `T_GoldNode_E`),
    NO TeamRegion slot, `SM_GoldNode` same-path overwrite. Each same-path overwrite preserves BP/csv/ghost soft refs (Reimport-click
    mechanism, flag if needed). ACCEPTANCE: each building `SM_<CardID>` textured at its path with two slots + authored UCX; `SM_GoldNode`
    textured with a single PBR+emissive slot (no TeamRegion) glowing warm-yellow; Nanite off; readbacks reported. Post in 🎨 Art; hand to
    build-master (TASK-183).
- names: >
    `/Game/Meshes/SM_<CardID>` (7 buildings + GoldNode, same-path overwrite) + `/Game/Textures/T_<CardID>_{D,N,ORM}` (+ `T_GoldNode_E`)
    + `MI_<CardID>_PBR` / `MI_GoldNode_PBR` (from `M_AssetPBR`) + authored `UCX_SM_<CardID>` (buildings). GoldNode `team_region:null`
    single slot. Law: CONVENTIONS "Textured mesh law" (+ GoldNode variant).

#### TASK-174 — Core combat VFX: impact / spawn / death + gold-burst + castle-debris Niagara (art, editor)
- assignee: art-director
- status: backlog
- blocked-by: none
- parallel-safe: no (editor-mutating Niagara authoring — single editor; but no logical blocker, editor-queued)
- spec: >
    §6 "every ability, impact, spawn, death, and spell gets a Niagara effect." Author the core combat VFX set at the §6 one-frame-readable
    bar (template-donor Niagara acceptable — Variant_Combat `NS_Damage` etc.): `NS_Impact` (per damage/projectile-impact), `NS_Spawn`
    (unit/building spawn), `NS_Death` (unit death) → Content/VFX/, PLUS the juice-referenced `NS_GoldBurst` (coin burst, TASK-158) and
    `NS_CastleDebris` (crumble debris, TASK-157). These are the systems the juice C++ soft-references null-safe. Palette per §6 (cool-blue
    friendly / warm-red enemy where team-relevant; gold warm-yellow). ACCEPTANCE: the 5 NS_ systems exist at `/Game/VFX/`, read in one
    frame, respect the perf budget. Post in 🎨 Art.
- names: >
    `/Game/VFX/NS_Impact`, `NS_Spawn`, `NS_Death`, `NS_GoldBurst`, `NS_CastleDebris`. Donors under `/Game/Variant_Combat/`. REUSE (directive 4):
    `NS_Impact` (projectile/arrow impacts + trails) draws on `Content/sA_ArcheryVfxPack/`; `NS_CastleDebris` pairs `Content/Realistic_Rocks/` rock
    chunks. Law: CONVENTIONS prefix table (`NS_` → Content/VFX/), "Template-donor rule".

#### TASK-175 — Ability/spell VFX polish to the §6 bar (art, editor)
- assignee: art-director
- status: backlog
- blocked-by: none
- parallel-safe: no (editor-mutating Niagara — single editor, editor-queued)
- spec: >
    Raise the EXISTING ability/spell VFX to the §6 premium bar: the M5 spell systems `/Game/VFX/NS_Spell_<CardID>` (Lightning/BattleCry/Pickpocket
    — Fireball + FrostNova are RE-SKINNED separately from the Fire_Magic/Ice_Magic packs in TASK-187, do NOT double-work them here) + `NS_ChainZap`,
    the hero Rally, and the hero swing/hit `NS_Damage` reuse. Upgrade readability/palette/intensity (keep the code contracts — same asset paths, same
    soft-ref names; this is a look pass, NOT a rename). REUSE (directive 4): the spell-cast/muzzle look may draw on `Content/sA_StylizedWizardSet/`
    wizard VFX. ACCEPTANCE: each spell/ability effect (excluding the two TASK-187 owns) reads in one frame at the §6 bar with the correct
    team/element palette; no path/name changes that would break the M5 resolver soft refs. Post in 🎨 Art.
- names: >
    Polish in place `/Game/VFX/NS_Spell_Lightning|BattleCry|Pickpocket`, `/Game/VFX/NS_ChainZap`, Rally/`NS_Damage` reuse (Fireball + FrostNova →
    TASK-187). REUSE: `Content/sA_StylizedWizardSet/` wizard cast/muzzle VFX. NO renames. Law: CONVENTIONS "Spells & Set III (M5)" (VFX soft-ref
    contract), "Template-donor rule".

#### TASK-176 — Lumen key+GI lighting + post-process stack (bloom/vignette/color-grade LUT) in L_Arena (art, editor)
- assignee: art-director
- status: done (integrated 2026-07-16 build-master — commit 86324b5, local only no push. 2026-07-16 art-director — DONE. L_Arena: key DirectionalLight re-angled+warmed (11 lux, 5400K temp, pitch -38/yaw 145, softer LightSourceAngle 1.5); Lumen GI+reflections confirmed active (r.DynamicGlobalIlluminationMethod=1, r.ReflectionMethod=1) and pinned in the PPV. SkyLight cooled ((0.80,0.87,1.0) @1.0, real-time capture). Warm-vs-cool team framing = 2 shadow-OFF RectLights: TeamFill_Cool_Blue @(-4000,0,1600) cool over Blue/-X half, TeamFill_Warm_Red @(+4000,0,1600) warm over Red/+X half, attenuation 6500 so they blend neutral at centerline. Unbound global PostProcessVolume PP_Arena_Global: bloom 0.6/thr 0.85, vignette 0.4, ColorSaturation 0.9 (mild env desat = Ori-rule approximation), ColorContrast 1.05, manual-locked exposure (bias 0.4) for deterministic stylized grade. NO LUT ASSET authored — MCP toolset has no LUT texture-import/assign path; grade approximated via PPV color-grading controls (FLAGGED for a later in-editor LUT pass). Gameplay collision/navmesh/actor transforms UNCHANGED (Castle ±8000, GoldNode ±7200 verified). New actors in outliner folder M7_SceneLighting. Perf note for TASK-183: +2 dynamic RectLights (shadow-off) + Lumen GI on white-walled arena. Handoff: handoffs/TASK-176-177-artist.md. Before/after PNGs: Saved/Screenshots/M7_SceneLighting/. → build-master.)
- blocked-by: none
- parallel-safe: no (editor-mutating L_Arena — single editor, serialize)
- spec: >
    §6 scene standards in `/Game/Maps/L_Arena`: one strong KEY light + Lumen GI (warm-vs-cool team framing — cool over the Blue half, warm
    over the Red), and a Post-Process Volume with bloom + subtle vignette + a color-grade LUT (saturated characters over a slightly
    desaturated environment — the Ori rule). Do NOT alter gameplay collision, navmesh, or actor transforms — lighting/PP only. Keep the §6
    60 fps@1440p budget in mind (Lumen cost is a human WATCH at Jonathan's playtest). ACCEPTANCE: L_Arena renders with Lumen key+GI, warm/cool
    team framing, and the bloom/vignette/LUT post stack; no gameplay geometry/transform change. Post in 🎨 Art; hand to build-master.
- names: >
    `/Game/Maps/L_Arena` DirectionalLight (key) + SkyLight + PostProcessVolume (bloom/vignette + color-grade LUT `/Game/Textures/T_ColorGrade_LUT`
    if authored). REUSE (directive 4): `Content/Fab/Stone_Hills_FREE/` + Megascans as scenic distant backdrop that the key/GI reads against.
    Law: CONVENTIONS "World axes (arena contract)" (do NOT move actors), GDD §6 scene standards.

#### TASK-177 — Stylized gradient skybox + skylight (art, editor)
- assignee: art-director
- status: done (integrated 2026-07-16 build-master — commit 86324b5, local only no push. 2026-07-16 art-director — DONE. Authored `/Game/Materials/M_Skybox` FROM SCRATCH: Unlit + TwoSided gradient (WorldPosition.Z → mask/scale/saturate → Lerp), warm horizon (0.70,0.38,0.17) → cool zenith (0.09,0.15,0.32) matching §6 palette. On a sky-dome sphere actor SkyDome_Gradient (/Engine/BasicShapes/Sphere, scale 900 = ~45000 radius, centered origin) — CastShadow OFF, bAffectDynamicIndirectLighting OFF (does NOT flood Lumen), NoCollision profile + bCanEverAffectNavigation=false (shell far outside ±8000 play area — cannot touch gameplay collision/nav). Default VolumetricCloud_0 HIDDEN (bVisible=false) so the sky reads stylized not default-HDRI; SkyAtmosphere_0 kept (feeds cool SkyLight real-time capture behind the dome). SkyLight cooled + feeds Lumen GI (see TASK-176). Character legibility preserved (units still read cool-blue/warm-red vs desaturated field). No geometry/transform change. Handoff: handoffs/TASK-176-177-artist.md. → build-master.)
- blocked-by: none
- parallel-safe: no (editor-mutating L_Arena — single editor, serialize)
- spec: >
    §6 "stylized gradient skybox." Author a stylized gradient sky (sky material `M_Skybox` on a sky sphere/dome, or a Sky Atmosphere tuned to
    a stylized gradient) + a matching SkyLight feeding Lumen GI, in `/Game/Maps/L_Arena`. Neutral-to-cool horizon per the §6 palette; must not
    wash out the saturated characters (coordinate with TASK-176's LUT). No gameplay collision/transform change. ACCEPTANCE: a stylized gradient
    sky is visible over the arena and feeds the skylight; character legibility preserved; no geometry change. Post in 🎨 Art; hand to build-master.
- names: >
    `/Game/Materials/M_Skybox` (or Sky Atmosphere) + sky mesh + SkyLight in `/Game/Maps/L_Arena`. GAP (directive 4): NO dedicated skybox pack was
    imported — author the gradient sky FROM SCRATCH; `Content/Fab/Stone_Hills_FREE/` + Megascans may sit as a distant scenic silhouette under it.
    Law: CONVENTIONS prefix table (`M_` → Content/Materials/), GDD §6 scene standards.

#### TASK-178 — Arena set dressing: banners / braziers / siege debris (NO collision change) (art, editor)
- assignee: art-director
- status: backlog
- blocked-by: none
- parallel-safe: no (editor-mutating L_Arena — single editor, serialize)
- spec: >
    §6 + §5 set dressing: place decorative banners (team-colored, cool-blue Blue side / warm-red Red side), braziers (warm emissive point
    lights — pairs with the §6 juice), and siege debris in the arena silhouette room (§5 "leave silhouette room for set dressing"). SOURCE:
    authored simple meshes and/or the approved Fab siege props (`Content/MedievalCastleEnvironmentAndSiegeWeaponProps/` castle + siege meshes,
    template-donor rule — flag if used) and `Content/Realistic_Rocks/` chunks as scattered siege-debris dressing (directive 4). HARD CONSTRAINT
    (§5): set dressing MUST NOT alter gameplay collision, navmesh, placement halves, or the traversability guarantee —
    decorative actors are NoCollision / `bCanEverAffectNavigation=false`, kept OUT of the castle pads, gold-node pads, spawn, and the central
    corridor (the keep-clear zones). ACCEPTANCE: the arena reads as a dressed battlefield with team banners + braziers + debris; a match's
    navmesh/pathing/placement is UNCHANGED vs before dressing (units still reach both castles; no new "Too close"/path failures). Post in 🎨
    Art; hand to build-master (must PIE-confirm no collision regression).
- names: >
    Decorative actors in `/Game/Maps/L_Arena` (banners/braziers/debris), NoCollision + no-nav. Fab donors under `Content/Fab/` (soft-ref/
    duplicate-into-/Game/ only). Law: CONVENTIONS "Fab quarantine", "Battlefield & procedural terrain (M6.5)" keep-clear/traversability, GDD §5.

#### TASK-179 — Audio trigger hooks: play S_<event> on all §6 events (C++)
- assignee: gameplay-programmer
- status: done/integrated (TASK-182 build PASS 2026-07-16, local commit f313253, no push) ← was: ready-for-qa (QA loop 2 2026-07-16: fixed BlueprintReadOnly-on-private (MinerUnit.h:155) + swept batch. Prior loop-2: TASK-182 compile UHT error MinerUnit.h:155 — `BlueprintReadOnly should not be used on private members` on `ClinkAudio`; batch build blocked → back to gameplay-programmer. Other 6 tasks stay qa-passed but held: single-module batch cannot integrate until this compiles.)
- blocked-by: none
- parallel-safe: yes
- spec: >
    Wire the §6 audio list to gameplay events in C++, each playing a SOFT-referenced sound null-safe (missing ⇒ silent, log once): hero
    swing + hit, unit spawn, projectile fire + impact, miner "clink" loop (§3.3), card play + discard clicks, spell cast, castle-hit +
    castle-destroyed stingers, victory/defeat music, overtime sting (§3.2, 7:00). Reuse EXISTING event seams (the same TakeDamage / spawn /
    fire / play-card / match-clock / match-end points the juice + M2..M5 code already own — do NOT re-plumb). Sounds referenced by composed
    soft path `/Game/Audio/S_<Event>` (null-safe). 2D UI/stinger sounds via `PlaySound2D`; world SFX via `SpawnSoundAtLocation`; the miner
    clink is a looping component started on mining/arrival and stopped on death. ACCEPTANCE: each listed event triggers its `S_<Event>` when
    present and is silent+logged when absent (no crash); the miner clink loops only while mining and stops on death; overtime sting fires once
    at 7:00. QA implied (shadow + include scans). Post in ⚙️ Dev & QA.
- names: >
    Soft refs `/Game/Audio/S_HeroSwing, S_HeroHit, S_UnitSpawn, S_ProjectileFire, S_ProjectileImpact, S_MinerClink, S_CardPlay, S_CardDiscard,
    S_SpellCast, S_CastleHit, S_CastleDestroyed, S_VictoryMusic, S_DefeatMusic, S_OvertimeSting` (art TASK-180; null-safe). Law: CONVENTIONS
    prefix table (`S_` → Content/Audio/), GDD §6 audio list.

#### TASK-180 — Audio: author the 4 COVERED SoundCues from MedievalWeaponsSFX (art)
- assignee: art-director
- status: done (integrated 2026-07-16 build-master — commit 86324b5, local only no push. 2026-07-16 — all 4 covered cues authored + verified at /Game/Audio/ as valid SoundCues [class=SoundCue, valid FirstNode, non-zero duration, non-looping, mono/3D-spatializable]: S_HeroSwing←S_Sword_Whoosh_1, S_HeroHit←S_Hit_Body_Mono_1, S_ProjectileFire←S_Bow_Mono_1, S_ProjectileImpact←S_Arrow_Hit_Body_Mono_1. Each is a duplicate-into-place of the donor cue [law-sanctioned; donor left untouched]. NOTE: Random-node/variety enhancement was NOT achievable via the MCP toolset [no unreal-py / console-exec route; SoundNode subobject construction is rejected by set_properties] — single-variant per cue, flagged for a future in-editor pass. Handoff: handoffs/TASK-180-186-artist.md. Build-master: PIE audio confirm at TASK-183.)
- blocked-by: none (REWRITTEN 2026-07-16 — the MedievalWeaponsSFX pack IS imported; the old audio-source gate no longer blocks these 4)
- parallel-safe: yes (editor import/cue-authoring; disjoint `/Game/Audio/` folder)
- spec: >
    REWRITTEN 2026-07-16 (directive 1). Author the 4 §6 event cues COVERED by the imported weapon-impact/whoosh pack `Content/MedievalWeaponsSFX/`
    (donor, READ-ONLY — duplicate-into-place or wrap the donor SoundWaves in a new cue; NEVER edit the donor). Each cue lands at the EXACT
    CardID-composed path TASK-179 references (character-for-character — the cross-discipline contract): `S_HeroSwing` ← `WeaponsSFXCue/WhooshCue/
    S_Sword_Whoosh_*`; `S_HeroHit` ← `SwordCue/S_Sword_Mono_*` OR `HitCue/S_Hit_Body_*`; `S_ProjectileFire` ← `BowCue/S_Bow_Mono_*`;
    `S_ProjectileImpact` ← `BowCue/S_Arrow_Hit_Body_*`. Prefer a random-selector over the `*_1/_2/…` variants for variety. All 4 are WORLD SFX
    (TASK-179 plays them via `SpawnSoundAtLocation`) — sensible 3D attenuation, NOT looping. DO NOT block these on the audio gaps (the other 10
    cues are TASK-186/188). ACCEPTANCE: the 4 `S_<Event>` cues exist at `/Game/Audio/`, import clean, and play on their event in PIE. Post in
    🎨 Art; hand to build-master.
- names: >
    `/Game/Audio/S_HeroSwing, S_HeroHit, S_ProjectileFire, S_ProjectileImpact` (match TASK-179 exactly). Donor `Content/MedievalWeaponsSFX/`
    (READ-ONLY). Law: CONVENTIONS "Audio event cues (M7)" (coverage map), prefix table (`S_` → Content/Audio/), "Cross-discipline rule",
    "Template-donor rule".

#### TASK-181 — Sequencer slice: cinematic flythrough + gameplay b-roll (art, editor)
- assignee: art-director
- status: backlog
- blocked-by: TASK-172, TASK-173 (textured meshes), TASK-165 (rigged units), TASK-174, TASK-175, TASK-187 (VFX + fire/ice re-skin), TASK-176, TASK-177, TASK-178 (lighting/skybox/dressing)
- parallel-safe: no (editor-mutating — single editor; capstone, after the full visual pass)
- spec: >
    The M7 portfolio slice (§9-7). Author a Level Sequence `LS_M7_Flythrough` (`/Game/Cinematics/`) — an archviz-style cinematic flythrough of
    the dressed, lit arena showcasing the premium art + skybox + Lumen + VFX — plus a gameplay b-roll capture plan (a real match with the full
    juice/crumble/animation/VFX running, for the "game-feel before/after" reel). Camera cuts that read the environment silhouette + the
    saturated-character / desaturated-environment framing. ACCEPTANCE: `LS_M7_Flythrough` plays a clean flythrough of the finished arena;
    a gameplay b-roll shot list is recorded for build-master's capture at TASK-183. Post in 🎨 Art; hand to build-master.
- names: >
    `/Game/Cinematics/LS_M7_Flythrough` (Level Sequence). Law: CONVENTIONS prefix table (add `LS_` cinematics if needed — manager to confirm),
    GDD §9-7 slice.

#### TASK-182 — M7 code batch: compile + residue adjudication + commit (build-master)
- assignee: build-master
- status: build-fail-escalated (loop-3 2026-07-16: batch does NOT compile. Loop-2 MinerUnit.h:155 fix CONFIRMED working/gone. New fatal: SiegeFeedbackLibrary.h:39 stray "*/" in comment prose "raw UWorld*/context" closes the /** */ doc comment early → UHT "block being skipped" on UCLASS/GENERATED_BODY, fatal under -WarningsAsErrors (was mislabeled a "cascade" at loop-2). NOT committed; git HEAD 71a985e unchanged. Fix = 1-line comment reword in SiegeFeedbackLibrary.h:39. Escalated to orchestrator/Jonathan; build-master did NOT auto-loop. See qa/TASK-154-159-179-qa.md loop-3.)
- blocked-by: TASK-154, 155, 156, 157, 158, 159, 179 (all qa-passed)
- parallel-safe: no (single editor + Git)
- spec: >
    Compile the M7 headless C++ batch (juice 154..158 + skeletal swap path 159 + audio hooks 179) after ALL are qa-passed. Editor-bounce
    compile per the learnings (close editor if it blocks the build, rebuild, reopen); resolve any residue; confirm the SkeletalVisualMesh path,
    hit-flash, squash/recoil, damage numbers, crumble thresholds, gold-burst/shake, and audio hooks are present and null-safe with their soft
    art absent (nothing hard-depends on unshipped art/VFX/sounds). Build command per CLAUDE.md. COMMIT the code batch to `main` (NOT pushed)
    with TASK-154..159 + 179 in the message; VERIFY GIT STATE FIRST (Jonathan self-commits) and reconcile rather than duplicate. A build
    failure appends to the QA report and routes back to gameplay-programmer (counts as a QA loop). ACCEPTANCE: clean compile; code present +
    null-safe; committed to main (not pushed) or reconciled. Post results + hash in 🔧 Build & Git.
- names: >
    Compile `GitClaudeUnrealTestEditor` (CLAUDE.md build cmd). Commit `Source/GitClaudeUnrealTest/Siegebound/**` (juice + SkeletalVisualMesh +
    audio hooks). Law: CLAUDE.md hard gates, CONVENTIONS "C++" laws.

#### TASK-183 — M7 final assembly: §6-checklist PIE + 60 fps perf watch + commit + m7-testable branch (build-master)
- assignee: build-master
- status: backlog
- blocked-by: TASK-162, 165 (rig integ), 172, 173 (mesh imports), 174, 175, 187 (VFX + fire/ice re-skin), 176, 177, 178 (lighting/skybox/dressing), 180, 186 (audio — covered + best-effort; TASK-188 gap is NON-blocking, ships silent+logged if ungated), 181 (sequencer), 182 (code committed)
- parallel-safe: no (single editor + Git)
- spec: >
    Final M7 integration + verification + commit. (1) STRUCTURAL: all 16 SM_<CardID> textured at unchanged paths (slots/MIs/Nanite/collision
    per law; GoldNode single-slot emissive); all rigged units carry SK_<CardID>+ABP_<CardID> and animate; the 5 core NS_ + polished spell VFX
    resolve; L_Arena has Lumen key+GI + post stack + gradient skybox + set dressing with UNCHANGED collision/navmesh. (2) §6 JUICE-CHECKLIST
    PIE (use `SummonTestUnit`/`ApplyTestDamage`/`AddTestGold` cheats to drive headless): hit-flash on damage, spawn squash-stretch, floating
    damage numbers, tower recoil on fire, castle crumble at 75/50/25 % with debris, gold-coin burst on kills, screen shake ≤0.2 s on castle
    hits, real skeletal attack animations replacing the procedural lunge, audio on each event. (3) TRAVERSABILITY regression: a match runs
    end-to-end, units reach both castles, no new "Failed to find path"/"Too close" spam (set dressing didn't break nav). (4) PERF WATCH: record
    best-effort FPS at 1440p with 60+ units (the §6 60 fps target — human WATCH at Jonathan's playtest per learnings; flag if Lumen/VFX/anim
    counts must drop). (5) Capture the TASK-181 flythrough + gameplay b-roll for the slice. (6) COMMIT M7 art+editor to `main` (NOT pushed;
    verify git state, reconcile Jonathan self-commits) and cut the `m7-testable` branch (milestone branch-preservation workflow). ACCEPTANCE:
    structural + §6-checklist + traversability PASS; perf recorded + flagged; committed (not pushed) + `m7-testable` cut; WATCH list posted for
    Jonathan's premium-art playtest. Post results + hash in 🔧 Build & Git.
- names: >
    Verify all `/Game/Meshes/SM_<CardID>` + `/Game/Characters/SK_<CardID>`/`ABP_<CardID>` + `/Game/VFX/NS_*` + `/Game/Maps/L_Arena` lighting/
    skybox/dressing + `/Game/Audio/S_*` + `/Game/Cinematics/LS_M7_Flythrough`. PIE via `SummonTestUnit`/`ApplyTestDamage`/`AddTestGold`. Commit
    to main only, not pushed; cut `m7-testable`. Law: CLAUDE.md hard gates + GDD mode checkpoint, CONVENTIONS all M7 laws.

---

### M7 directive tasks (added 2026-07-16 — Jonathan's 4 directives, TASK-184..188)

#### TASK-184 — Concept-gen tool: build concept_generate.py (text→image) (tooling/C++)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-16 — QA PASS, 0 blockers, 2 non-blocking WARNs + 2 NITs; report qa/TASK-184-qa.md. Flipped by art-director at TASK-185 start per QA verdict.)
- blocked-by: none
- parallel-safe: yes (headless Python tooling; disjoint from all gameplay code + editor)
- spec: >
    Jonathan AUTHORIZED an automated concept-art step (directive 3). BUILD `Tools/ArtPipeline/concept_generate.py` — a text→image generator that
    turns a per-CardID art-direction PROMPT into `Tools/ArtPipeline/Inbox/<CardID>.png`, driving an HF text-to-image Space/model via the EXISTING
    HF PRO token. MIRROR `trellis_generate.py` in structure + secret-handling + CLI/exit-code discipline: `HF_TOKEN` is ENV-ONLY (read at runtime,
    never argv/file/log; same redactor + `hf_` guard), the Space call runs atomically on one Client, exit codes reuse the trellis map (0 ok/`--check`;
    2 token unset; 3 quota; 4 API drift; 5 input missing; 64 usage). CLI: `concept_generate.py --check` (probe token + endpoints) and
    `concept_generate.py <CardID>` (reads the prompt for that CardID from `Tools/ArtPipeline/concept_prompts.json` — prompts authored in TASK-185 —
    and writes `Inbox/<CardID>.png`, PascalCase). The tool is PROMPT-AGNOSTIC (does not hardcode any card design). `Tools/**/*.py` is CODE → the
    tooling QA gate applies. ACCEPTANCE: `--check` passes with the token set and fails cleanly (exit 2) without it; a single `<CardID>` run writes a
    valid PNG to Inbox/; no secret leak; exit codes correct. QA implied (tooling QA — secret-handling + exit-code review). Post in ⚙️ Dev & QA.
- names: >
    `Tools/ArtPipeline/concept_generate.py` (+ reads `Tools/ArtPipeline/concept_prompts.json`). Output `Tools/ArtPipeline/Inbox/<CardID>.png`.
    Mirrors `Tools/ArtPipeline/trellis_generate.py`. Law: CONVENTIONS "Textured mesh law" ("Stage 0 — concept generation", tooling law).

#### TASK-185 — Concept-gen run: author prompts + produce 16 concept PNGs (art)
- assignee: art-director
- status: done (2026-07-16 — ALL 16 concept PNGs present in Inbox/ (produced by a detached `concept_generate.py --all` run, 03:33-03:35; the earlier HF-router 504 outage cleared). Verified via `ls Inbox/`: ArrowTower, BallistaTower, Barracks, BombTower, Cavalry, Cleric, CrystalTower, DeepMine, GoldNode, Knight, Longbowman, MilitiaMob, Miner, Pikeman, Sapper, Wall — all 16 PascalCase, each 500KB-1.1MB. Prompts committed in concept_prompts.json. This UNBLOCKS the TRELLIS mesh batch (TASK-168..171), now running. Prior handoff: handoffs/TASK-185-artist.md.)
- blocked-by: TASK-184 (tool built + QA-passed), HF-INFERENCE-OUTAGE (router.huggingface.co 504 — transient upstream; resume when it clears)
- parallel-safe: yes (Bash + HF; HF quota serializes in practice — orchestrator paces, exit 3 = quota pause/resume)
- spec: >
    Author the per-CardID art-direction PROMPTS in `Tools/ArtPipeline/concept_prompts.json` (one dominant subject, strong silhouette, team-agnostic,
    §6 stylized bar — the read-at-150px discipline) and RUN `concept_generate.py <CardID>` for the 16 M7 CardIDs → `Tools/ArtPipeline/Inbox/<CardID>.png`
    (PascalCase). Surface non-zero exit codes VERBATIM in 🚨 Blockers (2 token · 3 quota-pause · 4 API-drift · 5 input). This RESOLVES the concept
    source for the mesh batch (unblocks TASK-168..171); Jonathan may OPTIONALLY replace any PNG before its wave runs (TASK-167). Partial completion
    unblocks the matching production wave (e.g. the 4 unit-wave-1 concepts → TASK-168). ACCEPTANCE: 16 PascalCase concept PNGs exist in `Inbox/`;
    each reads as a usable TRELLIS input at the §6 bar; prompts committed. Post in 🎨 Art.
- names: >
    `Tools/ArtPipeline/concept_prompts.json` (16 prompts) + `Tools/ArtPipeline/Inbox/<CardID>.png` × 16 (Knight, Miner, Cavalry, Cleric,
    Longbowman, MilitiaMob, Pikeman, Sapper, ArrowTower, Wall, BombTower, BallistaTower, Barracks, DeepMine, CrystalTower, GoldNode). Law:
    CONVENTIONS "Textured mesh law" ("Stage 0 — concept generation").

#### TASK-186 — Audio: best-effort cues from OTHER imported packs (S_SpellCast, S_UnitSpawn, weak S_CastleHit) (art)
- assignee: art-director
- status: done (integrated 2026-07-16 build-master — commit 86324b5, local only no push. 2026-07-16 — 2 of 3 filled, 1 rolled to TASK-188 per spec. FILLED (valid SoundCues at /Game/Audio/, mono/3D, non-looping): S_UnitSpawn←MedievalWeaponsSFX WhooshCue/S_Whoosh_Mono_1 (generic deploy whoosh, distinct from HeroSwing's sword whoosh); S_CastleHit (WEAK placeholder)←MedievalWeaponsSFX HitCue/S_Hitting_Wall_Mono_1 — DEVIATION FROM the convention-named 'S_Hit_Wood_*/S_Axe_Wood_Hit_*': used 'Hitting Wall' instead as a strictly-better masonry/structural stand-in (the pack DOES have a wall-impact; still weak, upgrade at TASK-188; orchestrator/manager may veto). LEFT SILENT: S_SpellCast — sA_StylizedWizardSet ships NO audio (only Blueprints/Fx/Materials/Models); no fitting cast SFX in any imported pack, so per 'do not force it' it rolls into the TASK-188 Jonathan gate. Handoff: handoffs/TASK-180-186-artist.md. Build-master: PIE audio confirm at TASK-183.)
- blocked-by: none (the candidate source packs are all imported)
- parallel-safe: yes (editor cue-authoring; disjoint `/Game/Audio/` folder)
- spec: >
    Author up to 3 more §6 cues from ALREADY-imported content (directive 1, the "sourceable-elsewhere" tier): `S_SpellCast` ← evaluate
    `Content/sA_StylizedWizardSet/` cast/muzzle SFX (if the pack ships audio); `S_UnitSpawn` ← evaluate a usable whoosh/muzzle from an imported pack
    (e.g. MedievalWeaponsSFX `WhooshCue`, or the wizard set); `S_CastleHit` ← WEAK placeholder from MedievalWeaponsSFX `S_Hit_Wood_*` /
    `S_Axe_Wood_Hit_*` (the pack has NO stone/masonry impact — mark it a placeholder to upgrade later). Author each at its exact `/Game/Audio/
    S_<Event>` path (donor READ-ONLY, duplicate/wrap). All 3 are WORLD SFX (`SpawnSoundAtLocation`), not looping. Any of the 3 with NO usable imported
    source ROLLS INTO the TASK-188 Jonathan gate (do not fake). ACCEPTANCE: each cue with a usable imported source exists at `/Game/Audio/` + plays
    in PIE; S_CastleHit flagged as a weak placeholder; unusable ones escalated to TASK-188. Post in 🎨 Art; hand to build-master.
- names: >
    `/Game/Audio/S_SpellCast, S_UnitSpawn, S_CastleHit`. Donors `Content/sA_StylizedWizardSet/`, `Content/MedievalWeaponsSFX/` (READ-ONLY). Law:
    CONVENTIONS "Audio event cues (M7)" (coverage map), "Template-donor rule".

#### TASK-187 — Fire/Ice VFX re-skin: Fire_Magic → NS_Spell_Fireball, Ice_Magic → NS_Spell_FrostNova (art, editor)
- assignee: art-director
- status: done/integrated (2026-07-16 — build-master LOCAL commit `5b6bc82`, NO push. NS_Spell_Fireball + NS_Spell_FrostNova re-skin committed IN PLACE with their hard-ref Fire_Magic/Ice_Magic pack deps [Materials/Mesh/Textures/VFX_Niagara]; Demo/BluePrints/Maps excluded. 192 files / ~351 MB LFS. Dep chain verified from the .uasset import tables: NS_Spell_* resolve from Materials+Mesh+Textures only (never touch the pack VFX_Niagara/). RESIDUAL FLAG for Jonathan: the included Ice_Magic/VFX_Niagara/NS_Ice_Magic_Frozen.uasset (a non-shipping pack DEMO effect, NOT on the FrostNova chain) hard-refs SKM_Quinn_Simple in the excluded Demo/ → cosmetic missing-ref warning on that demo asset only; the shipping spells are clean. NOTE: this board line flipped in working tree only — NOT committed (avoids dragging the uncommitted M7 decomposition into a TASK-187 asset commit).)
- blocked-by: none (the Fire_Magic + Ice_Magic packs are imported)
- parallel-safe: no (editor-mutating Niagara — single editor, serialize with the other VFX tasks)
- spec: >
    Directive 2 — a VFX RE-SKIN of the two EXISTING M5 spells (NOT new cards; Fireball cards.csv row 24 + FrostNova row 25 already exist). Retarget
    the fire/ice look to the imported `Content/Fire_Magic/` + `Content/Ice_Magic/` Niagara packs, authored IN PLACE at the CardID-composed code-
    contract paths so `USpellLibrary::ResolveSpell` (SpellLibrary.cpp:77-78) still resolves them: `/Game/VFX/NS_Spell_Fireball` ←
    `Fire_Magic/VFX_Niagara/NS_Fire_Magic_Explosion` or `_AOE` (Fireball = 100 dmg / 300-radius AoE burst); `/Game/VFX/NS_Spell_FrostNova` ←
    `Ice_Magic/VFX_Niagara/NS_Ice_Magic_Shockwave` / `_Frozen` / `_Snowstorm` (FrostNova = freeze, 350 radius). NO rename, NO new data column, NO
    C++ change — the path is COMPOSED from the CardID (a data column cannot point at a Fire_Magic asset). Scale/time the effect to the spell radius;
    keep it §6 one-frame-readable. SUPERSEDES TASK-175's Fireball+FrostNova polish (avoid double-work). FLAG: FrostNova is DeckCount 0 (not in the
    default deck) → the ice effect won't be seen in normal play until Jonathan bumps its DeckCount (D-FROSTNOVA-DECK). ACCEPTANCE: casting Fireball
    shows the Fire_Magic burst + casting FrostNova shows the Ice_Magic effect, both at `/Game/VFX/NS_Spell_<CardID>`, resolver soft-refs intact,
    one-frame readable. Post in 🎨 Art; hand to build-master.
- names: >
    Re-skin IN PLACE `/Game/VFX/NS_Spell_Fireball` (← `Content/Fire_Magic/VFX_Niagara/`) + `/Game/VFX/NS_Spell_FrostNova` (← `Content/Ice_Magic/
    VFX_Niagara/`). NO renames. Law: CONVENTIONS "Spells & Set III (M5)" → "Spell VFX element re-skin (M7)", "Template-donor rule".

#### TASK-188 — Audio GATE: source the 7 genuinely-missing cues (Jonathan — external gate)
- assignee: Jonathan (external gate — orchestrator posts the missing-cue list in 🚨 Blockers; art-director imports once a source lands)
- status: backlog
- blocked-by: audio-source gate (Jonathan supplies/approves a source — these 7 exist in NO imported pack; decision D-AUDIO-GAP). NON-blocking for the M7 commit: TASK-179 keeps them silent+logged.
- parallel-safe: yes (external — no agent repo mutation until a source lands)
- spec: >
    The 7 §6 cues with NO source in any imported pack (directive 1 gap list): `S_MinerClink` (mining LOOP), `S_CardPlay` (UI click), `S_CardDiscard`
    (UI click), `S_CastleDestroyed` (destruction stinger), `S_VictoryMusic` (music), `S_DefeatMusic` (music), `S_OvertimeSting` (7:00 stinger).
    Jonathan supplies loose files or approves a royalty-free/Fab SFX+music pack; the art-director then imports each at its exact `/Game/Audio/
    S_<Event>` path (raw under `Content/RawAssets/Audio/`), loop/2D flags per the TASK-179 contract (S_MinerClink LOOPS; the UI/music/sting cues are
    2D). Until then TASK-179's hooks keep these silent+logged (null-safe — no crash), so M7 SHIPS without them. ACCEPTANCE: once a source lands, all
    7 exist at `/Game/Audio/`, import clean, correct loop/2D flags, play on their event in PIE. Post the ask in 🚨 Blockers; import work in 🎨 Art.
- names: >
    `/Game/Audio/S_MinerClink (LOOP), S_CardPlay, S_CardDiscard, S_CastleDestroyed, S_VictoryMusic, S_DefeatMusic, S_OvertimeSting` (match TASK-179).
    Raw under `Content/RawAssets/Audio/`. Law: CONVENTIONS "Audio event cues (M7)", "Cross-discipline rule".

---

**2026-07-14 OGRE PULL-FORWARD (COMPLETE — history; the first M7 asset, superseded by the M7 KICKOFF above):** M6.6 is DONE + signed off (Jonathan self-committed + pushed `057ca9f "walkable terrain"`). Jonathan then dropped an ogre concept (`Tools/ArtPipeline/Inbox/ogre.png`) and directed the **TRELLIS.2 art pipeline** be run to REPLACE the existing `/Game/Meshes/SM_Ogre` blockout with a game-ready textured mesh — one of the 16 M7 blockouts pulled forward on his directive. Same validated pipeline as the Footman/Archer/Castle pilots (TASK-082..088). Chain **TASK-147..152** below ("### M7 pull-forward — Ogre textured mesh"). CONVENTIONS "Textured mesh law" updated (Ogre = active pipeline asset; no new pattern). NOT in scope: the 2D card art `T_CardArt_Ogre` (separate lane, unchanged). **Two Jonathan touchpoints:** (1) **HF generation** — if Stage-1 `--check`/generate surfaces a token/quota/API-drift/image issue, surface the exit code VERBATIM + escalate 🚨 Blockers, never fake (2=token unset · 3=quota, expected pause · 4=API drift · 5=image missing); HF_TOKEN + HF PRO already live (TASK-085). (2) **EYEBALL GATE (TASK-150)** between Stage 2 and Stage 3 — a NEW asset's TRELLIS orientation + team-region are unknown until first generation, so `pre_rotate_z_deg`/`team_region` are starting guesses that need Jonathan's eye before import (exactly the TASK-086/087 gate). **Dispatch frontier = TASK-147 ONLY** (serial single-asset chain — each stage blocks the next; art-director does 147/148/149/151, Jonathan gates 150, build-master integrates 152). Repo base = `057ca9f` on `main`, pushed.

**2026-07-14 M6.6 KICKOFF (done milestone — history):** **M6.5 is DONE** — Jonathan committed the assembled battlefield HIMSELF as `6a4c17d "battlefield created"` and PUSHED it (this satisfies the held TASK-136/137 commit gate; no separate build-master M6.5 commit — GATE 0 for M6.6 is satisfied). **M6.6 "Climbable terrain" (TASK-138..145) is now the CURRENT milestone** — it UNPARKS the M4.5 gameplay-terrain intent (make hills/rocks climbable). Root cause of the un-climbable hills = the scatter's UNIFORM scale makes face angles scale-invariant, so the squashed `stone_hill` dome is un-climbable at any scale; fix = purpose-built CONVEX hill meshes `SM_Hill_01/02/03` (≤30° faces, flat crowns) + a 4000-wide arena + terrain-blocks-projectiles + units-climb. Four decisions locked (see the "M6.6 tasks" block). Naming law: CONVENTIONS "Climbable terrain (M6.6)". Authoritative plan: `C:\Users\wesel\.claude\plans\we-last-left-off-partitioned-puppy.md`. Repo base = `6a4c17d` on `main`, pushed. TASK-138 (this CONVENTIONS block) is DONE; dispatch frontier = **TASK-139 ∥ TASK-140 ∥ TASK-141** (all parallel-safe, blocked only by TASK-138).

**2026-07-10 BOARD RECONCILIATION (repo state for M6.5 integration — READ FIRST):** current HEAD on `main` = **61a1e72** (NOT pushed). Two post-M6 chains shipped after the M6 commit (975ee90): (1) **deck-builder physical-card tiles** — TASK-125/129 authored, integrated + committed by TASK-126 @ **274c160**; (2) **overhead health-bar REBUILD** (castle push/delegate parity, RED enemy / BLUE friendly) — TASK-130/131/132 @ **61a1e72**, which RETIRED the failed first-attempt chain TASK-122/123/124/127/128 (their poll-system code + `WBP_UnitHealthBar` were DELETED by the rebuild — those five are SUPERSEDED, not shipped-as-authored). All `done`, nothing pushed. **M6.5 (Battlefield & procedural terrain) is now the CURRENT milestone** (TASK-133..137). The individual TASK-122..124 status lines retain their forensic QA-loop history below their new done/superseded marker. (Reconciliation note: the coordinator's hand-typed buckets had TASK-126/129 under 61a1e72 and omitted 124/128 — corrected here by task type: deck tiles → 274c160, health-bar rebuild → 61a1e72, first-attempt health-bar → superseded.)

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

---

### M6 — Deck-builder meta (TASK-113..121) — decomposed 2026-07-09

**Authorization:** Jonathan's 2026-07-09 direct Claude Code M6 go-ahead ("move on to M6") after the M5.5 health-bar batch shipped. GDD mode: only M6 is decomposed. Naming law added to CONVENTIONS "Deck-builder & saved decks (M6)" BEFORE task issue; the two M5.5 process follow-ups are folded in (see "Follow-ups" below). Hard gates stand: editor/MCP work needs the editor MCP (127.0.0.1:8000) up — park + tell the orchestrator if unreachable (never fake results); nothing pushed. This batch does NOT touch M5's, M5.5's, or M4.5's state (all preserved on the board); M6 runs INDEPENDENT of parked M4.5.

**THE CRUX — build on the existing deck system, do not reinvent it (infra audited 2026-07-09):** `UDeckComponent::BuildAndShuffle()` (DeckComponent.h/.cpp) builds the draw pile from the `DeckCount` column of `/Game/Data/DT_Cards` and deals a 6-card hand; draw/discard/reshuffle/preview all exist and are frozen contracts. BOTH `ASiegePlayerController` (BeginPlay) and `ASiegeBotController` (OnPossess) own a `DeckComponent` subobject named `DeckComponent` and call `BuildAndShuffle()` at match start / `ResetDeck()` on Play Again — so TODAY every controller gets the SAME deck (the DeckCount column). `FCardRow` already carries `MaxCopies` (per-card cap) and `Cost` (average-cost math). M6 ADDS a per-controller override (a settable pending `FDeckList`) that `BuildAndShuffle` prefers when set-and-legal, else falls back to DeckCount — a purely additive seam, zero behavior change to the empty/unset path.

**M6 manager decisions (binding for all M6 tasks; each judgment call FLAGGED for QA + Jonathan):**
1. **Where saved decks live — FLAG 1 (RECOMMEND: SaveGame).** Player-authored named decks persist in a `USiegeDeckSaveGame` (`USaveGame` subclass) written to the fixed slot `"SiegeDecks"` (user index 0) → `Saved/SaveGames/SiegeDecks.sav`. Chosen over a DataAsset (NOT runtime-writable in a packaged build — can't save the player's edits) and a hand-rolled `.json` in `Saved/` (SaveGame is the idiomatic runtime-writable, cross-session, package-safe store). The SaveGame doubles as the menu→match handoff — the active deck is NOT passed through the level-open URL (50 CardIDs would bloat it; and the URL wouldn't persist across sessions anyway). Recommend SaveGame — ACCEPT unless Jonathan wants human-readable deck files.
2. **"Named decks" keyed by name — FLAG 2.** A deck is keyed by its `DeckName` (FString, player-entered). `SavedDecks` is a `TArray<FDeckList>`; `SaveDeckAs(Name)` overwrites a same-named entry (no silent duplicates); `ActiveDeckName` (FString) records which deck the next match uses. Reserved/empty name handling and a max-deck-count cap are left to the widget (flag any limit at TASK-118). Recommend name-keyed with overwrite-on-collision — ACCEPT.
3. **"Replace default deck with a legal curated one" — FLAG 3 (concrete meaning + M5-test-deck reconciliation).** The current `DeckCount` column is the M4/M5 TEST spread (22 cards spread to 50 so Set II/III were reachable pre-deck-builder — CONVENTIONS DeckCount registry note). TASK-115 RE-AUTHORS that column into a legal, intentional 50-card STARTER deck (a good curated deck, still `sum==50`, each `DeckCount<=MaxCopies`), superseding the test spread. This single cards.csv edit IS the "replace default deck" deliverable. The re-authored DeckCount is BOTH the deck-builder's "reset to default" template AND the match fallback when no legal saved active deck exists. cards.csv stays the single source of truth (§3.0). Recommend re-authoring DeckCount (not a parallel new asset) — ACCEPT; Jonathan may tweak the exact 50 at playtest (it's a CSV edit).
4. **Bot's "2 distinct decks" — definition + selection — FLAG 4.** `TArray<FDeckList> BotDecks` (EditDefaultsOnly) on `ASiegeBotController`, exactly TWO distinct legal curated decks as C++ constructor defaults (tunable per-BP; a deck composition is content, not a per-card stat → NOT a CSV column, mirroring the mechanic-rule-as-UPROPERTY precedent). At spawn the bot RANDOMLY picks one (`FMath::RandRange(0,1)`), logs it on `LogSiegeBot` (grep-able), and pushes it via `SetPendingDeckList`; missing/illegal entry ⇒ DeckCount fallback. Recommend random-of-2-at-spawn (variety across matches) over fixed/alternating — ACCEPT; Jonathan may prefer a fixed pairing at playtest.
5. **Copy-cap + exactly-50 enforcement — FLAG 5 (data-driven).** Both enforced from cards.csv: per-card `MaxCopies` caps `AddCopy` (the widget greys the "+" at the cap and refuses over-cap adds); the deck is legal (and "Play with this deck" enabled) ONLY at `TotalCount()==50`. The ONE legality function `UDeckLibrary::IsDeckLegal(CardTable, Deck, OutReason)` is shared by the widget, the DeckComponent build path, and the bot — no duplicated rules. Recommend the single shared data-driven validator — ACCEPT.
6. **Average-cost display (§8) — FLAG 6.** `UDeckLibrary::GetDeckAverageCost` = sum(Cost×Count)/TotalCount, shown as a soft guide (§8: <4 spams, >7 bricks; NO hard rule — it never blocks saving/playing). Data-driven from cards.csv Cost. Recommend display-only guide — ACCEPT.
7. **Tech reality — heavy UMG, editor-MCP-dependent — FLAG 7 (WATCH).** `WBP_DeckBuilder` is a heavy screen (28-card browser grid + per-card counters + x/50 + avg-cost + save/load list + play/back buttons). MCP CAN author full widget trees (PROVEN: TASK-041 built the complete interactive 6-slot hand; TASK-111 the health-bar widget) by duplicating a donor and rewiring incrementally — the working technique. It is EDITOR-MCP-dependent: if the MCP is down, TASK-118 parks and tells the orchestrator. The C++ base (`UDeckBuilderWidget`) owns ALL model/logic so the WBP is layout + BlueprintCallable calls + float/int/bool/FString BIEs (never enum/struct BP params — widget rule). The main-menu already has a greyed Deck Builder button (TASK-049) — the screen wires to it (`Btn_DeckBuilder`, enable + open as a viewport overlay on L_MainMenu; no new level/game mode).
8. **Verification reality — FLAG 8 (WATCH + the follow-up that fixes it).** Save/load and the exactly-50 gate are largely MACHINE-verifiable (SaveGame readback + DeckComponent deck-content readback in PIE — no human input needed). The LIVE click-through of the 28-card grid (add/remove feel, greyed caps, the counters updating) is a human WATCH on Jonathan's unlocked desktop (locked-desktop = no SendInput, TASK-076/112 doctrine). TASK-121 (the debug-exec cheat, follow-up 2) is issued IN this milestone so build-master can drive combat-side verification (health bars, castle HP, spawns) headlessly starting with M6's own TASK-120 — it does not help the grid click-through, which stays a human WATCH.

**Follow-ups from the M5.5 build (how handled):**
- **(1) CONVENTIONS include-rule** — handled as a CONVENTIONS coding-law note ("Complete-type include law", C++ section), NOT a task: a `.cpp` upcasting/dereferencing a forward-declared component (e.g. `GetCapsuleComponent()`) must `#include` that type's header; QA now scans every code task for it. This is what broke TASK-110's first compile.
- **(2) Debug-exec cheat** — task-ized as **TASK-121** (parallel-safe, folds into the M6 compile so build-master gains `SummonTestUnit`/`ApplyTestDamage`/`AddTestGold` for TASK-120 and every future headless verification). Placed in M6 because the follow-up notes it "would pay off starting with M6's own verification."

**M6 exit criteria (playable slice):** from the main menu, the (now-enabled) Deck Builder button opens `WBP_DeckBuilder`; the screen browses all 28 cards, adds/removes copies with each card's `MaxCopies` enforced (the "+" refuses/greys at the cap), shows a live "x/50" counter and an average-cost readout (§8); "Play with this deck" is enabled only at exactly 50; the player saves and loads NAMED decks that survive quitting and relaunching the game (SaveGame); the active saved deck FEEDS the next match (the player's in-match hand is dealt from it, verified by DeckComponent readback) and, with no legal saved deck, the match falls back to the curated 50-card DeckCount default; the bot plays with one of its 2 distinct curated decks (random pick logged on LogSiegeBot). Recordable: UI/UX + save-load systems clip.

Dispatch shape: **WAVE 1 — FILE tasks, parallel-safe NOW: TASK-113 ∥ TASK-115 ∥ TASK-121** (distinct file sets: deck-model/save/lib vs cards.csv vs cheat-manager). **WAVE 2 (after TASK-113 qa-passed): TASK-114 ∥ TASK-116** — 116 is parallel-safe (new DeckBuilderWidget files); 114 is `parallel-safe: no` because it shares `SiegePlayerController.cpp` with TASK-121, so 114 waits on 121 too. QA gates every code task (shadow-scan + the new complete-type-include scan mandatory). Then **TASK-117** (build: compile the C++ batch 113/114/116/121 + reimport DT_Cards for the TASK-115 curated DeckCount; editor-bounce so `UDeckBuilderWidget` exists) → **TASK-118** (art: WBP_DeckBuilder screen) → **TASK-119** (programmer editor: enable + wire the WBP_MainMenu Deck Builder button — single editor at a time, after 118) → **TASK-120** (build: final assembly + PIE verification + commit + cut m6-testable). Nothing after WAVE 2 is parallel-safe (single editor / serial integration).

#### TASK-113 — Deck data model + SaveGame + legality/avg-cost library (C++ files)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-09; qa/TASK-113-report.md — PASS, 0 blockers / 0 warns / 2 nits, all flagged decisions ACCEPTED [aggregate cap is a valid strengthening]. Both scans CLEAN; cap boundary correct [==MaxCopies passes]; avg-cost div0 guarded; SaveGame persistence verified vs UE 5.8 engine source [SlotName/UserIndex shared consts, no silent data loss]. Carry-forwards for TASK-114/116: use USiegeDeckSaveGame::SlotName/::UserIndex not re-literals, null-check LoadGameFromSlot cast, .cpp calling UGameplayStatics must include Kismet/GameplayStatics.h. Rides TASK-117 compile)
- blocked-by: none
- parallel-safe: yes (all-new files: DeckTypes.h, DeckLibrary.h/.cpp, SiegeDeckSaveGame.h/.cpp — no overlap with TASK-115/121)
- spec: >
    Files only — NO editor/MCP. Deliver the deck data/persistence/rules core per CONVENTIONS "Deck-builder
    & saved decks (M6)". (1) NEW header-only `DeckTypes.h`: `FDeckCardEntry` (USTRUCT BlueprintType — FName
    CardID, int32 Count), `FDeckList` (USTRUCT BlueprintType — FString DeckName, TArray<FDeckCardEntry>
    Cards, with `int32 TotalCount() const`), and `static constexpr int32 SiegeLegalDeckSize = 50` (GDD §3.4).
    (2) NEW `UDeckLibrary` (UBlueprintFunctionLibrary, DeckLibrary.h/.cpp): `static bool IsDeckLegal(const
    UDataTable* CardTable, const FDeckList& Deck, FString& OutReason)` — data-driven from DT_Cards: every
    entry CardID must exist as a row, every Count in [0..that row's MaxCopies], and TotalCount()==
    SiegeLegalDeckSize; OutReason = first violation (HUD/log); null table ⇒ false+reason. `static float
    GetDeckAverageCost(const UDataTable* CardTable, const FDeckList& Deck)` — sum(Cost×Count)/TotalCount
    (§8 guide), 0 for empty. Never hardcode a stat that lives in DT_Cards (§3.0). (3) NEW
    `USiegeDeckSaveGame` (USaveGame subclass, SiegeDeckSaveGame.h/.cpp): `TArray<FDeckList> SavedDecks`,
    `FString ActiveDeckName`; expose the fixed slot name `"SiegeDecks"` as a const the readers share.
    ACCEPTANCE: compiles warnings-as-errors; IsDeckLegal returns true only for a 50-card cap-respecting
    deck and false with a reason otherwise; GetDeckAverageCost matches sum(Cost×Count)/50 on the curated
    default; no hardcoded card stats. → qa-reviewer (MANDATORY: inherited-reflected-member shadow scan AND
    the new complete-type-include scan — CONVENTIONS coding laws). Post in ⚙️ Dev & QA
    (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-113`).
- names: >
    `DeckTypes.h` (FDeckCardEntry{FName CardID; int32 Count}, FDeckList{FString DeckName; TArray<FDeckCardEntry> Cards; int32 TotalCount() const}, constexpr int32 SiegeLegalDeckSize=50) in Source/GitClaudeUnrealTest/Siegebound/.
    `UDeckLibrary` (UBlueprintFunctionLibrary), DeckLibrary.h/.cpp — IsDeckLegal(const UDataTable*, const FDeckList&, FString& OutReason), GetDeckAverageCost(const UDataTable*, const FDeckList&).
    `USiegeDeckSaveGame` (USaveGame), SiegeDeckSaveGame.h/.cpp — TArray<FDeckList> SavedDecks, FString ActiveDeckName, slot const "SiegeDecks", user index 0.
    Reuse only (do NOT redefine): FCardRow.MaxCopies / .Cost / .DisplayName from CardRow.h; /Game/Data/DT_Cards.

#### TASK-114 — Wire decks into matches: DeckComponent override path + player loads active deck + bot 2 decks (C++ files)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-09; qa/TASK-114-report.md — PASS, 0 blockers / 1 warn / 2 nits, all 5 flagged decisions ACCEPTED. Backward-compat traced BYTE-IDENTICAL [&&-short-circuit; unset/illegal → exact pre-M6 DeckCount build]; TASK-121 lines CONFIRMED intact [SiegePlayerController.cpp:31 include + :50 CheatClass]; both bot decks re-summed 50 + caps ok + distinct; case-insensitive lookup safe [FString== is IgnoreCase]; both scans clean. Rides TASK-117 compile.
  **JONATHAN CHECKPOINT ITEM (design, non-blocking): bot decks are spell-free → shipped M5 bot rule-3 Fireball/Lightning never fires in M6, and with a spell-free bot deck TASK-120 CANNOT verify the bot-spell feature at all. QA recommends adding Lightning ×2 to Defensive Economy [drop Longbowman ×1 + Wall ×1 → still legal 50]. Reversible constructor-default edit. Orchestrator ruling: NOT applied unilaterally [bot design = Jonathan's domain]; surfaced at the M6 checkpoint.**)
- blocked-by: TASK-113 (uses FDeckList / UDeckLibrary / USiegeDeckSaveGame); TASK-121 (shares SiegePlayerController.cpp)
- parallel-safe: no (edits SiegePlayerController.cpp — shared with TASK-121; and DeckComponent + SiegeBotController)
- spec: >
    Files only — NO editor/MCP. Make the existing deck system consume chosen decks, additively (backward-
    compatible), per CONVENTIONS "Deck-builder & saved decks (M6)". (1) `UDeckComponent`: add
    `void SetPendingDeckList(const FDeckList& Deck)` storing a guarded pending list; in `BuildAndShuffle()`,
    when the pending list is SET AND `UDeckLibrary::IsDeckLegal` passes, build the draw pile from it
    (Count copies of each CardID) INSTEAD OF the DeckCount column — else fall back to the EXISTING
    DeckCount build UNCHANGED (empty/unset/illegal = today's exact path, nothing breaks). The pending list
    PERSISTS across ResetDeck()/Play Again (same match keeps the same deck). Log which source built the
    deck. (2) `ASiegePlayerController` BeginPlay (the existing DeckComponent->BuildAndShuffle call site):
    BEFORE building, load `USiegeDeckSaveGame` from slot "SiegeDecks"; if it has an ActiveDeckName whose
    FDeckList IsDeckLegal, `SetPendingDeckList` it (else leave unset → curated DeckCount fallback).
    Null-safe: no save file ⇒ fallback. (3) `ASiegeBotController`: add `UPROPERTY(EditDefaultsOnly,
    Category="Siegebound|Bot") TArray<FDeckList> BotDecks` with TWO distinct legal curated decks as
    constructor defaults; at spawn (before its BuildAndShuffle) pick one via FMath::RandRange(0,1),
    `SetPendingDeckList` it, and LOG the choice on `LogSiegeBot` (one grep-able line); missing/illegal ⇒
    fallback. Do NOT alter draw/discard/reshuffle/preview. ACCEPTANCE: compiles warnings-as-errors; a
    legal pending list drives the built deck (verified by GetDrawPileCount + card composition); no pending
    / illegal falls back to DeckCount exactly as today; bot logs its chosen deck index. → qa-reviewer
    (MANDATORY shadow scan + complete-type-include scan). Post in ⚙️ Dev & QA (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-114`).
- names: >
    `UDeckComponent::SetPendingDeckList(const FDeckList&)` + guarded pending member, in DeckComponent.h/.cpp (BuildAndShuffle prefers pending-if-legal, else DeckCount; persists across ResetDeck).
    `ASiegePlayerController` BeginPlay: load USiegeDeckSaveGame slot "SiegeDecks" → active FDeckList → SetPendingDeckList if IsDeckLegal.
    `ASiegeBotController`: TArray<FDeckList> BotDecks (EditDefaultsOnly, 2 curated defaults), random pick at spawn → SetPendingDeckList, LogSiegeBot line.
    Reuse only: UDeckLibrary::IsDeckLegal (TASK-113), existing DeckComponent subobject name `DeckComponent`, existing BuildAndShuffle/ResetDeck.

#### TASK-115 — Curated legal 50-card default deck: re-author the cards.csv DeckCount column (data)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-09; qa/TASK-115-report.md — PASS, 0 blockers, sum 50 + caps + avg 5.08 independently recomputed. WARN carry → TASK-117 (already in its spec, do NOT skip): QA had no git/shell, so build-master MUST run the git-diff byte-check [DeckCount-only, frozen columns intact] + live DT_Cards re-verify before the M6 commit. NITs (playtest visibility only): Footman 24% of deck, Miner sole econ. Rides TASK-117 compile+reimport)
- blocked-by: none
- parallel-safe: yes (edits Docs/Data/cards.csv only)
- spec: >
    Files only — NO editor/MCP (the DT_Cards reimport rides TASK-117). Re-author the `DeckCount` column of
    `Docs/Data/cards.csv` from the M4/M5 test spread into a legal, intentional 50-card STARTER deck (GDD
    §9-6 "replace default deck with a legal curated one"), per CONVENTIONS "Deck-builder & saved decks (M6)"
    ruling 3. Constraints (HARD, data-driven from the same file): the DeckCount values must sum to EXACTLY
    50, and each row's DeckCount must be <= that row's MaxCopies; only real CardIDs. Design it as a sane
    playable curated deck (a frontline + ranged + a tower/wall + an economy + a finisher spread), average
    cost in the §8 healthy band (~4–6). Change ONLY the DeckCount column — do NOT touch any other column
    (Cost/MaxCopies/stats/CardArt/spell columns are frozen). Leave a one-line Notes/comment trail of the
    intended deck if the CSV supports it (else record it in the handoff). ACCEPTANCE: sum(DeckCount)==50;
    every DeckCount<=MaxCopies; no other column changed; the deck is a coherent starter. → qa-reviewer
    (verify the sum + per-card caps + no collateral column edits — data legality). Post in ⚙️ Dev & QA
    (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-115`).
- names: >
    `Docs/Data/cards.csv` — DeckCount column only. Legality: sum==50, each DeckCount<=MaxCopies (same row).
    Reimports as /Game/Data/DT_Cards at TASK-117. Single source of truth (§3.0) — no hardcoded deck elsewhere.

#### TASK-116 — Deck-builder widget C++ base UDeckBuilderWidget (C++ files)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-09; qa/TASK-116-report.md — PASS, 0 blockers / 0 warns / 2 nits, all 9 flagged decisions ACCEPTED. Both scans CLEAN; 3 TASK-113 carry-forwards verified IN CODE; AddCopy cap data-driven, RemoveCopy floors 0, library-delegated legality/avg-cost, strict SetActiveDeck. Carry → TASK-118: the 4 additive card resolvers [GetCardDisplayName/Cost/MaxCopies/ArtTexture] are the ONLY card-data path [keep WBP out of DT_Cards]; grey "+" at count≥MaxCopies; Play = SaveDeckAs→SetActiveDeck→StartMatch. Carry → TASK-114 QA: confirm its ActiveDeckName→FDeckList lookup uses ESearchCase::IgnoreCase [safe either way — 116 stores canonical name]. Rides TASK-117 compile)
- blocked-by: TASK-113 (uses FDeckList / UDeckLibrary / USiegeDeckSaveGame)
- parallel-safe: yes (all-new files DeckBuilderWidget.h/.cpp — no overlap with TASK-114's file set)
- spec: >
    Files only — NO editor/MCP (WBP_DeckBuilder authoring is TASK-118). Deliver the deck-builder MODEL as a
    `UDeckBuilderWidget` (UUserWidget subclass) per CONVENTIONS "Deck-builder & saved decks (M6)". The C++
    base owns ALL logic; the WBP will be layout + calls. BlueprintCallable/Pure API (UObject/struct returns
    allowed here): `AddCopy(FName CardID)` (refuse over that card's MaxCopies — data-driven), `RemoveCopy(
    FName CardID)`, `GetCountOf(FName CardID) const`, `GetTotalCount() const`, `GetAverageCost() const`
    (via UDeckLibrary), `IsCurrentDeckLegal() const` (via UDeckLibrary::IsDeckLegal), `LoadDefaultDeck()`
    (seed the working FDeckList from the DeckCount curated default in DT_Cards), `SaveDeckAs(const FString&
    Name)` + `LoadDeck(const FString& Name)` + `GetSavedDeckNames() const` + `SetActiveDeck(const FString&
    Name)` (all through USiegeDeckSaveGame slot "SiegeDecks", null-safe). Also a Pure getter over the 28-card
    collection (row names of DT_Cards) so the WBP can build the browser grid. BIEs are float/int/bool/FString
    params ONLY (widget rule): `OnDeckModelChanged()` (widget re-reads getters — seed-then-bind law) and
    `OnDeckSlotCountChanged(FString CardID, int32 Count)`. Everywhere null-safe (missing DT_Cards / missing
    SaveGame ⇒ graceful, log once, never crash). NO gameplay/combat coupling — this only edits/persists
    FDeckLists. ACCEPTANCE: compiles warnings-as-errors; AddCopy respects MaxCopies; GetTotalCount/
    GetAverageCost/IsCurrentDeckLegal match UDeckLibrary; save/load round-trips a named deck through the
    SaveGame; BIE params are float/int/bool/FString only. → qa-reviewer (MANDATORY shadow scan — watch
    `Slot` on UWidget — + complete-type-include scan). Post in ⚙️ Dev & QA (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-116`).
- names: >
    `UDeckBuilderWidget` (UUserWidget subclass), DeckBuilderWidget.h/.cpp in Source/GitClaudeUnrealTest/Siegebound/.
    BlueprintCallable/Pure: AddCopy(FName)/RemoveCopy(FName)/GetCountOf(FName)const/GetTotalCount()const/GetAverageCost()const/IsCurrentDeckLegal()const/LoadDefaultDeck()/SaveDeckAs(FString)/LoadDeck(FString)/GetSavedDeckNames()const/SetActiveDeck(FString)/ + a collection-CardIDs getter.
    BIEs (float/int/bool/FString ONLY): OnDeckModelChanged(), OnDeckSlotCountChanged(FString CardID, int32 Count).
    UMG asset (TASK-118): `WBP_DeckBuilder` at /Game/UI/WBP_DeckBuilder. Reuse: UDeckLibrary, USiegeDeckSaveGame, FDeckList (TASK-113), /Game/Data/DT_Cards.

#### TASK-121 — Debug exec cheats for headless verification: USiegeCheatManager (C++ files)
- assignee: gameplay-programmer
- status: qa-passed (2026-07-09; qa/TASK-121-report.md — PASS, 0 blockers / 1 warn / 3 nits, all 5 flagged decisions ACCEPTED. Shipping-safety confirmed byte-for-byte [footprint = 1 ctor line + its include; grep found no other instantiation]; all 3 cheats verified vs real reused paths. WARN → TASK-114 (shares SiegePlayerController.cpp): programmer added 3 comment lines + inline comment beyond the strict 2 — non-functional, no landmine, but TASK-114 must READ THE FILE FRESH [ctor line numbers shifted] and PRESERVE the SiegeCheatManager.h include + CheatClass assignment. Rides TASK-117 compile)
- blocked-by: none
- parallel-safe: yes vs TASK-113/115 (new SiegeCheatManager.h/.cpp); note it also touches SiegePlayerController (CheatClass) — TASK-114 is sequenced AFTER it to avoid a shared-file race
- spec: >
    Files only — NO editor/MCP. Add a non-shipping debug-exec affordance so build-master can drive PIE
    verification on a locked desktop (no SendInput — TASK-076/112 doctrine), per CONVENTIONS "Dev / test
    tooling" (TASK-121 entry). (1) NEW `USiegeCheatManager` (UCheatManager subclass, SiegeCheatManager.h/.cpp)
    with `UFUNCTION(exec)` commands, each null-safe and routed through EXISTING shipping code paths (never a
    raw field write, never a bespoke spawn): `SummonTestUnit(FString CardID, bool bRed)` — spawn a card actor
    for the given team via the same spawn path the controller/bot uses; `ApplyTestDamage(float Amount)` —
    apply damage to the actor under the crosshair / nearest enemy through the normal TakeDamage path (induces
    health-bar + castle-HP changes); `AddTestGold(int32 Amount)` — top up the Blue player through the
    ASiegePlayerState gold API. (2) Set `CheatClass = USiegeCheatManager` in the `ASiegePlayerController`
    constructor. The engine only instantiates a CheatManager in non-shipping builds with cheats enabled, so
    this cannot leak into Shipping — additive by construction. ACCEPTANCE: compiles warnings-as-errors; the
    three exec commands exist, are null-safe, and reuse shipping paths; zero behavior change to normal play;
    Shipping unaffected. → qa-reviewer (shadow scan + complete-type-include scan; confirm no shipping-path
    behavior change and no raw field writes). Post in ⚙️ Dev & QA (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-121`).
- names: >
    `USiegeCheatManager` (UCheatManager subclass), SiegeCheatManager.h/.cpp in Source/GitClaudeUnrealTest/Siegebound/.
    exec: SummonTestUnit(FString CardID, bool bRed), ApplyTestDamage(float Amount), AddTestGold(int32 Amount).
    Wire: ASiegePlayerController::CheatClass = USiegeCheatManager (constructor). Reuse existing spawn path, TakeDamage, ASiegePlayerState gold API.

#### TASK-117 — M6 C++ batch compile + DT_Cards reimport (build)
- assignee: build-master
- status: done (2026-07-09; handoffs/TASK-117.md — 4 gates PASS: cards.csv byte-diff DeckCount-ONLY confirmed [closes TASK-115 QA WARN], compile SUCCESS 0 warn/0 C4458, DT_Cards reimport via set_rows [live readback sum 50 + 0 cap violations], all 4 classes live [DeckBuilderWidget/DeckLibrary/SiegeDeckSaveGame/SiegeCheatManager]. NO commit [rides TASK-120]. TASK-120 commit must include Docs/Data/cards.csv + Content/Data/DT_Cards.uasset [now dirty from in-editor save]. TASK-118 unblocked, editor up PID 4948)
- blocked-by: TASK-113, TASK-114, TASK-115, TASK-116, TASK-121 (all qa-passed)
- parallel-safe: no
- spec: >
    Build-master. PHASE-A compile for M6 (mirrors the M5.5 TASK-112 phase-A pattern — compile early so the
    art task has the class). (1) Compile the accumulated M6 C++ batch (TASK-113/114/116/121) with the
    standard Build.bat command; editor-bounce protocol (editor releases the DLL). Pre-compile: scan the
    batch for inherited-reflected-member shadows AND the new complete-type-include law (CONVENTIONS coding
    laws). Any error → append to the failing task's QA report, set qa-failed, stop (counts as a QA loop;
    build-master never edits code). (2) Reimport `/Game/Data/DT_Cards` from the TASK-115 re-authored
    `Docs/Data/cards.csv` (the curated DeckCount) via the editor, and verify `sum(DeckCount)==50` and each
    `DeckCount<=MaxCopies` on the live table. (3) Confirm `UDeckBuilderWidget`, `UDeckLibrary`,
    `USiegeDeckSaveGame`, `USiegeCheatManager` exist live in the editor via reflection (so TASK-118 can
    reparent). Do NOT commit here — the commit rides TASK-120 (single M6 commit). If the editor MCP is
    down, compile via Build.bat and report the reimport/reflection checks as owed (never fake). Post the
    compile result in 🔧 Build & Git (`🔧 BUILD-MASTER: … TASK-117`).
- names: >
    Build command per CLAUDE.md. Reimport /Game/Data/DT_Cards from Docs/Data/cards.csv. Verify classes:
    UDeckBuilderWidget, UDeckLibrary, USiegeDeckSaveGame, USiegeCheatManager. No commit (rides TASK-120).

#### TASK-118 — WBP_DeckBuilder screen: 28-card browser + counters + save/load list + play/back (art/editor)
- assignee: art-director
- status: ready-for-integration (2026-07-09; handoffs/TASK-118.md — WBP_DeckBuilder [donor WBP_MainMenu] + NEW WBP_DeckCardTile sub-widget [TASK-120 MUST commit BOTH], reparent to UDeckBuilderWidget readback-confirmed, 28-card WrapBox grid + counters + avg + save/load/reset + legal-gated Play + Back all wired to the C++ API, both BIEs bIsImplemented=true, compile clean. TASK-120 must PIE-verify [not done, no PIE per constraint]: grid renders 28, +/− change counts, "+" greys at cap, counters/avg track, Play gates at 50, Save/Load/Reset/Back work. M7-polish flags: saved-deck is text readout not clickable rows; no ScrollBox [check 28-tile overflow])
- blocked-by: TASK-117 (reparents to UDeckBuilderWidget — needs the class compiled + live in the editor)
- parallel-safe: no (editor-mutating — single editor instance)
- spec: >
    Editor/MCP only — needs the editor MCP up (else park + tell the orchestrator). Author the heavy deck-
    builder screen per CONVENTIONS "Deck-builder & saved decks (M6)". (1) DUPLICATE a multi-element donor
    (`/Game/UI/WBP_CardHand` or `/Game/UI/WBP_MainMenu`) to `/Game/UI/WBP_DeckBuilder`; never author the
    tree from scratch (template-donor rule). (2) REPARENT it to `UDeckBuilderWidget` (compiled in TASK-113/
    117). (3) Build the screen, driving EVERYTHING through the C++ base's BlueprintCallable/Pure API + BIEs
    (no logic in the WBP graph beyond calling them): a browser GRID of all 28 collection cards (use the
    collection-CardIDs getter; reuse the card-art resolver pattern / `T_CardArt_<CardID>` if convenient,
    else text tiles) each with a "+"/"−" (AddCopy/RemoveCopy) and its current count (GetCountOf); the "+"
    greys/refuses at the card's MaxCopies; a live "x/50" counter (GetTotalCount); an average-cost readout
    (GetAverageCost, §8 guide); a saved-deck LIST with load (GetSavedDeckNames/LoadDeck), a name field +
    Save (SaveDeckAs), and a "reset to default" (LoadDefaultDeck); `Btn_PlayWithDeck` enabled ONLY when
    IsCurrentDeckLegal() (sets the active deck via SetActiveDeck then ASiegeGameMode::StartMatch); `Btn_Back`
    returns to WBP_MainMenu. Seed the widget from the getters, THEN bind the BIEs (seed-then-bind law).
    Art skips QA → build-master integration check (TASK-120). ACCEPTANCE: /Game/UI/WBP_DeckBuilder exists,
    parent=UDeckBuilderWidget, the grid/counter/avg-cost/save-load/play/back elements are wired to the C++
    API, the "+" respects MaxCopies, Play is legal-gated. Post the handoff in 🎨 Art (`🎨 ART-DIRECTOR: …
    TASK-118`) with the asset path.
- names: >
    `WBP_DeckBuilder` at /Game/UI/WBP_DeckBuilder, parent class `UDeckBuilderWidget`. Donor: /Game/UI/WBP_CardHand or /Game/UI/WBP_MainMenu.
    Buttons: per-card +/− (AddCopy/RemoveCopy), Btn_PlayWithDeck (legal-gated → SetActiveDeck + StartMatch), Btn_Back (→ WBP_MainMenu), Save/Load/reset-to-default.
    Drive via UDeckBuilderWidget API: AddCopy/RemoveCopy/GetCountOf/GetTotalCount/GetAverageCost/IsCurrentDeckLegal/LoadDefaultDeck/SaveDeckAs/LoadDeck/GetSavedDeckNames/SetActiveDeck + collection getter; BIEs OnDeckModelChanged()/OnDeckSlotCountChanged(FString,int32).

#### TASK-119 — WBP_MainMenu: enable + wire the Deck Builder button to open WBP_DeckBuilder (editor)
- assignee: gameplay-programmer
- status: ready-for-integration (2026-07-09; handoffs/TASK-119.md — additive granular MCP edit, compiled clean. Button is runtime-constructed [no designer Btn_DeckBuilder — it's the "Deck Builder" construct node from TASK-049]; enabled + relabeled + OnClicked → RemoveFromParent + CreateWidget WBP_DeckBuilder_C + AddToViewport. Play/Sandbox/Quit bindings byte-unchanged [full-graph readback]. TASK-120 flag: CreateWidget.OwningPlayer left null [mirrors Back] — wire GetOwningPlayer if PIE shows focus issue. Integration + commit ride TASK-120)
- blocked-by: TASK-118 (opens WBP_DeckBuilder — it must exist)
- parallel-safe: no (editor-mutating — single editor instance; edits WBP_MainMenu)
- spec: >
    Editor/MCP only — needs the editor MCP up (else park + tell the orchestrator). ADDITIVE edit to the
    EXISTING `/Game/UI/WBP_MainMenu` (TASK-049/072). (1) Find the existing greyed "Deck Builder" button
    (authored disabled in TASK-049; name it `Btn_DeckBuilder` — reconcile with its actual authored name and
    record it). (2) ENABLE it and wire `OnClicked` to remove WBP_MainMenu from the viewport and add a fresh
    `WBP_DeckBuilder` (overlay navigation on L_MainMenu — no new level/game mode; WBP_DeckBuilder's Btn_Back
    reverses it). Do NOT disturb the Play-vs-Bot / Btn_Sandbox bindings (TASK-072 additive-only caution).
    ACCEPTANCE (PIE from L_MainMenu): the Deck Builder button is enabled and clicking it opens the deck-
    builder screen; Back returns to the menu; Play-vs-Bot and Sandbox still work. Editor task → build-master
    integration check (TASK-120). Post the handoff in ⚙️ Dev & QA (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-119`)
    with the button name.
- names: >
    /Game/UI/WBP_MainMenu — button `Btn_DeckBuilder` (reconcile with the TASK-049 authored name), enable + OnClicked → RemoveFromParent + CreateWidget/AddToViewport `WBP_DeckBuilder`. Do NOT touch the Play-vs-Bot or Btn_Sandbox bindings.

#### TASK-120 — M6 final assembly: deck-builder PIE verification + commit + m6-testable (build)
- assignee: build-master
- status: done (2026-07-09; handoffs/TASK-120.md — commit 975ee90 on main [41 files], branch m6-testable cut at it, NOTHING pushed [main ahead 4]. Desktop LOCKED → machine-only. VERIFIED: compile clean, DT_Cards sum 50/0 cap violations, WBP_DeckBuilder parent=UDeckBuilderWidget, 2 bot decks via CDO, menu button opens deck-builder + Play/Sandbox/Quit intact, CheatClass wired; LIVE PIE: no-saved-deck → curated DeckCount fallback [byte-identical to TASK-114], bot builds 50 from OVERRIDE deck + logs 1-of-2 pick [override path = same code player active deck uses]. WATCH [owed to Jonathan, locked desktop — MCP has no UFUNCTION-invoke/console/Python to author a .sav headless]: SaveGame save→relaunch persistence, player active-deck feed, live 28-tile grid click-through, TASK-121 cheat execs. Residue: TASK-120.md handoff [post-commit hash insert] + WBP_CastleHealthBar boot-resave [not staged, reverts on editor close] — both safe)
- blocked-by: TASK-118 (ready-for-integration) + TASK-119 (ready-for-integration)
- parallel-safe: no
- spec: >
    Build-master final assembly for M6 (TASK-109/112 pattern). (1) Re-verify the build compiles clean
    warnings-as-errors and DT_Cards carries the curated 50-card DeckCount (TASK-117 reimport). (2) Run the
    M6 exit-criteria PIE suite: from L_MainMenu open the deck-builder (TASK-119 button); browse the 28-card
    grid; add copies to a card past its MaxCopies and confirm the "+" refuses/greys; watch the "x/50"
    counter and average-cost readout update; confirm "Play with this deck" is enabled only at exactly 50;
    SaveDeckAs a named deck, then verify the SaveGame persists across an editor/PIE restart (LoadGameFromSlot
    "SiegeDecks" readback); set it active, Play, and confirm the player's in-match hand is dealt from that
    deck (DeckComponent deck-composition readback) and that with no legal saved deck the match falls back to
    the curated DeckCount default; confirm the bot logs one of its 2 curated decks on LogSiegeBot. Use the
    TASK-121 cheats (SummonTestUnit/ApplyTestDamage/AddTestGold) to drive any combat-side checks headlessly.
    Machine-verify save/load + the 50-gate + deck-feed; the live 28-card grid CLICK-THROUGH (add/remove feel,
    greyed caps) is a human WATCH (locked desktop = no SendInput) — record it as owed to Jonathan. If the
    editor MCP is down, compile via Build.bat and report PIE items as owed (never fake). (3) On PASS: commit
    the whole M6 batch (code + WBP_DeckBuilder + WBP_MainMenu + cards.csv/DT_Cards + pipeline docs) with a
    task-ID message; cut the `m6-testable` branch at that commit (milestone-branch-preservation workflow).
    Do NOT push. Build failure → append errors to the offending task's QA report and route back to
    gameplay-programmer (counts as a QA loop). Post compile result + commit hash + branch in 🔧 Build & Git
    (`🔧 BUILD-MASTER: … TASK-120`).
- names: >
    Assets/classes exactly as TASK-113..119 names blocks. Commit on `main`, message pattern "TASK-113..121:
    M6 deck-builder meta — WBP_DeckBuilder, USaveGame named decks, curated default + 2 bot decks, debug cheats".
    Cut branch `m6-testable` at the commit. No push.

---

### M6 playtest feedback (2026-07-09) — TASK-122..126 — decomposed 2026-07-09

**Source:** Jonathan's post-M6 playtest, TWO items, direct to the orchestrator (current-milestone change requests — GDD-mode feedback flow: playtest notes become tasks in the CURRENT milestone, M6). **Item 1** = overhead health bars don't drop when hit + a DESIGN REVERSAL (bars visible the entire time). **Item 2** = the deck builder needs physical card displays. Jonathan's ordering: *"after that gets fixed"* → **Item 1 (TASK-122..124) lands BEFORE Item 2 (TASK-125..126).** This batch does NOT touch M5/M4.5 parked state; M6 stays `done-pending-playtest` with these fixes folded into its slice.

**Ordering / parallel-safety ruling (for the orchestrator):** Item 1 and Item 2 touch DISJOINT files/assets — Item 1: `HealthBarComponent.cpp/.h` + `WBP_UnitHealthBar`; Item 2: `WBP_DeckCardTile` — so they are file-conflict-free. **BUT NOT dispatched concurrently:** (a) Jonathan explicitly sequenced Item 1 first, and (b) BOTH chains' editor/MCP tasks (TASK-123, TASK-125) and both build tasks need the SINGLE editor instance (CONVENTIONS: one editor-mutating task at a time). Ruling: **Item 2's TASK-125 is `blocked-by: TASK-124`.** The one thing that starts IMMEDIATELY (no blockers) is **TASK-122** (head of Item 1). Within Item 1 the chain is SERIAL — TASK-122 (code+diagnosis) → TASK-124 phase-A compile → TASK-123 (WBP repair) → TASK-124 phase-B (PIE+commit) — mirroring the M5.5 TASK-110→111→112 shape.

**Decision block (each judgment call FLAGGED for QA + Jonathan):**
1. **Item 1 is TWO changes, specced separately (do not conflate) — FLAG 1.** *1a* = the DESIGN REVERSAL: kill hide-at-full, bar visible the entire time the actor is alive (full HP included) — a deliberate override of the shipped M5.5 law (commit `9a8a75f`). CONVENTIONS "Overhead unit health bars (M5.5)" behavior law is UPDATED to always-visible, attributed to Jonathan + dated 2026-07-09. *1b* = the BUG: the fill never reflects HP. The programmer must NOT assume 1a fixes 1b — they may be independent (the fill is only driven while shown, and the WBP/reparent may be broken). Both live in TASK-122's scope; the WBP-side repair is TASK-123.
2. **1b root-cause is UNVERIFIED — diagnose read-only FIRST — FLAG 2.** Highest-value first action (TASK-122 STEP 1, read-only MCP): (a) does `WBP_UnitHealthBar`'s `OnHPChanged` actually drive `SetPercent` in its GRAPH (an FName in the name table does NOT prove the graph is wired); (b) is `WBP_UnitHealthBar` actually reparented to `UUnitHealthBarWidget` — **a null `BarWidget` cast at `HealthBarComponent.cpp:80` is the PRIME SUSPECT: the component shows/hides the bar via its own `SetVisibility` regardless of `BarWidget`, so a null cast produces EXACTLY the "bar appears but never drops" symptom** (`OnHPChanged` at `:152-154` is guard-skipped when `BarWidget` is null); (c) is `HPBarWidget` constructed on all three families with the soft class resolving to `/Game/UI/WBP_UnitHealthBar`. Why the castle is unaffected: it is a PUSH-model delegate bar (`FOnCastleHPChanged`) — a wholly separate system from this POLL-model component. Do NOT touch the castle.
3. **`bShowHealthBar` opt-out — OPEN QUESTION, flagged for Jonathan, NOT silently decided — FLAG 3.** Manager recommendation: **KEEP** the per-BP EditDefaultsOnly opt-out (default `true`). It is orthogonal to hide-at-full (it gates whether a TYPE carries a bar at all — e.g. suppress miner clutter — not WHEN a bar shows), and always-visible bars make it MORE useful, not less. TASK-122 keeps it. If Jonathan wants EVERY actor's bar always-on with zero exceptions, the opt-out can be dropped — his call (non-blocking; the recommended path ships either way).
4. **Item 2 needs ZERO new C++ — art re-skin only — FLAG 4.** `UDeckBuilderWidget` ALREADY exposes every card-data resolver a card face needs (`GetCardArtTexture` / `GetCardDisplayName` / `GetCardCost` / `GetCardMaxCopies` / `GetCountOf`, DeckBuilderWidget.h:83-128). The hand's card face is INLINED in `WBP_CardHand` — NO shared card-face sub-widget exists to extract. Per Jonathan's explicit "for now" stopgap, TASK-125 REPRODUCES the hand's card-face composition in the EXISTING `WBP_DeckCardTile` (additive re-skin), rather than factoring out a shared widget (extraction deferred — the shape that reuse-over-authoring + additive-only-to-working-widgets both favor). No programmer task, no QA (art skips QA).
5. **No new naming pattern needed — FLAG 5.** `WBP_DeckCardTile` and the card-face composition law already exist (M6 + "Card artwork (hand UI)"). CONVENTIONS got two RECORDING edits (not new asset types): the M5.5 hide-at-full REVERSAL, and an M6 note that `WBP_DeckCardTile` adopts the hand's card-face composition via the existing resolvers.

**Exit criteria (feedback batch):** *(Item 1)* every unit / tower / wall / Barracks / Deep Mine / miner / hero overhead bar is VISIBLE at full HP and DROPS live as the actor takes damage, team-tinted, hidden only on death; castle unchanged (own bar, no duplicate); gold nodes none. *(Item 2)* the deck-builder's 28-card grid renders each tile as a physical card (art + name + cost, matching the hand), with +/−, counter, cap-grey, and legal-gated Play all still working. Recordable: overhead-bar drop clip + deck-builder physical-cards clip.

**Dispatch shape:** **TASK-122 starts immediately** (head of Item 1, no blockers). Then SERIAL: TASK-124 phase-A compile → TASK-123 (WBP repair) → TASK-124 phase-B (PIE + commit). Then Item 2 SERIAL after Item 1: TASK-125 (tile re-skin) → TASK-126 (integration + commit). QA gates TASK-122 (shadow-scan + complete-type-include scan MANDATORY). Nothing here is dispatched concurrently across the two items (single editor + Jonathan's ordering).

**LOOP-1 PLAYTEST FEEDBACK (2026-07-10) — ITEMs A/B/C added to the Item-1 chain (TASK-127/128 + a TASK-122 QA mandate):** Jonathan PIE'd the loop-1 DLL and confirmed *"the health bars are working, however … make the part of the bar that shows the health red if they are enemies and blue if they are friendly, and the background of the bar grey."* The `[TASK122DIAG]` runtime log (40 polls) confirms the fill now DRAINS on 23 actors. Three items: **A** — a QA RE-REVIEW MANDATE on TASK-122 (the fix WORKS but its stated root cause is REFUTED by that log; qa-reviewer must adjudicate LOAD-BEARING vs HARMLESS HARDENING and NOT close as "works, ship it" — full mandate in TASK-122's status; **NO new task ID, it rides TASK-122's already-pending QA loop-1**). **B** — the bar color scheme → **TASK-127** (art-director, WBP_UnitHealthBar): team-tinted fill + grey track + contrast; CONVENTIONS "Bar colors (ITEM B)" added, and the grey-as-authored-WBP-brush-vs-C++-data call is the manager's (ruling: authored WBP brush, canonical value recorded in CONVENTIONS — a static style, not per-actor data). **C** — strip the temporary `[TASK122DIAG]` logs → **TASK-128** (gameplay-programmer), gated between TASK-127 and the TASK-124 phase-B commit. **REVISED Item-1 chain (serial):** TASK-122 QA loop-1 (ITEM A verdict) → TASK-127 (ITEM B color) → TASK-128 (ITEM C strip, code→QA) → TASK-124 phase-B (recompile stripped + final PIE incl. the color/contrast hard gate + SINGLE Item-1 commit). Item-2 (TASK-125/126 deck-builder physical cards) stays queued behind the whole Item-1 chain (Jonathan's original "after that gets fixed"). **CLOSURE INSIGHT (from ITEM A's leading hypothesis):** if the loop-1 C++ is only harmless hardening, ITEM B (TASK-127) IS the real user-visible fix — 1b (the bar not dropping *visibly*) is NOT truly closed until TASK-127 lands. **What starts NOW:** qa-reviewer on TASK-122 with the ITEM A mandate (it is `ready-for-qa`).

#### TASK-122 — Overhead health bars: reverse hide-at-full (always-visible) + fix the fill never dropping (C++ + read-only MCP diagnosis)
- assignee: gameplay-programmer
- status: **SUPERSEDED by the TASK-130..132 health-bar REBUILD** (2026-07-10; this poll-system attempt's `HealthBarComponent` code was DELETED by the rebuild — the always-visible + fill-drop intent was ultimately delivered by TASK-130..132 @ 61a1e72, NOT by this chain; forensic history retained below). PRIOR: **ready-for-qa (QA LOOP 2 — gameplay-programmer, 2026-07-10)** — Delivered: `LogDiag()` instrumentation of the ONLY unproven hop (`Percent` UPROPERTY → pixels) — captures widget Slate cached, the `Bar` ProgressBar's Slate cached + live `GetPercent`, `GetWidget()==BarWidget` identity, `IsInViewport`/`IsVisible`/`ownerHidden`/`tickEnabled`, `Space`/`DrawSize`; PLUS a labeled low-risk castle-aligned candidate fix (removed constructor `SetVisibility(false)`; BeginPlay seeds visibility from `bShowHealthBar`, killing the hide→show toggle the castle never does). **Castle diff MEASURED:** the InitWidget-order hypothesis is REFUTED (castle uses the identical `Super::BeginPlay`→`InitWidget(null)`→`SetWidgetClass` order); CDO read shows our `TickMode=Enabled` (component ticks continuously → weakens the visibility-toggle theory). **Root cause HONESTLY UNDETERMINED from static analysis** — leading candidates the log discriminates: `BarSlate=0` (the `Bar` SMyProgressBar not live when `SetPercent` runs — `GetPercent` reads the UPROPERTY so it can't tell) or `identity=0` (on-screen widget ≠ the `BarWidget` I push). **NEEDS TASK-124 phase-A compile + ONE PIE pass to read `[TASK122DIAG] LogDiag`;** final "fill visibly drains" is a human WATCH (screen-space Slate uncapturable, OVERNIGHT-AUTH §3). If loop 3 doesn't close it, escalate (§5). Details: `qa/TASK-122-report.md` "QA LOOP 2" + handoff "QA LOOP 2". Prior ITEM-A / loop-1 record retained below. — PRIOR: **qa-passed (ITEM A)** — orchestrator-proxied 2026-07-10 (qa-reviewer has no partial-edit tool). **RULING: the loop-1 C++ is HARMLESS HARDENING, NOT load-bearing.** Hypothesis H1 confirmed; H2 and H3 refuted. Evidence: (i) all three loop-1 deltas are non-visual (never-taken `CreateWidget` fallback, `ApplyTeamTint()` refactor with identical call/values/guard, `[TASK122DIAG]` logs) — the always-visible gate, the re-resolve, and the every-poll `OnHPChanged` push are byte-identical to round-1; (ii) `[TASK122DIAG]` shows `side-effect-created=YES` + cast `VALID` on 23/23 actors, so the programmer's "SetWidgetClass side-effect never fired" root cause is refuted by his own instrumentation and the fallback is dead code; (iii) UE 5.8 engine source (`WidgetComponent.cpp:2359` `SetWidgetClass`, `:1746` `InitWidget`) — our ctor sets only the SOFT `HealthBarWidgetClass`, so base `WidgetClass` starts null and BeginPlay's `SetWidgetClass` deterministically constructs the widget once `HasBegunPlay()`; no run-to-run null path exists in a PIE client world; (iv) TASK-123 was a no-op (WBP byte-identical across both PIE runs), so the fill rendered IDENTICALLY in both — round-1 was already draining and already pushing the tint. ~~The real defect was a WHITE fill on a white/near-white track~~ → **CORRECTION (orchestrator, 2026-07-10, post-TASK-127): THIS PREMISE IS FALSE; H1'S MECHANISM IS UNSUPPORTED AND THE ROOT CAUSE IS UNEXPLAINED.** TASK-127's readback found the track's `backgroundImage.tintColor` was **black `{0,0,0,0.5}`**, not white — only the FILL was white. A white/blue/red fill draining across a translucent black track would have been plainly visible AND plainly tinted, so "invisible due to low contrast" cannot explain round-1's report. The white-on-white story was an ORCHESTRATOR INFERENCE from a partial reading of the TASK-123 audit (which reported only `FillColorAndOpacity` and `fillImage.tintColor` as white, and never characterized the track); manager concurred and QA built ITEM A's ruling on top of it. It does not survive contact with the authored asset. **Standing state of knowledge:** round-1 = visible + frozen + untinted; loop-1 = draining; `WBP_UnitHealthBar` byte-identical across both PIE runs; the only loop-1 C++ delta the DIAG log proves executed is a functionally-identical `ApplyTeamTint()` refactor (fallback never ran — `side-effect-created=YES` 23/23). **RESOLVED 2026-07-10 — THE ANOMALY WAS A PHANTOM. Jonathan retracted the "bars are working" report:** *"I may have given you some bad information, the health bar does not seem to drop at all when the unit takes damage, for characters, hero, and towers, so that was never fixed."* **There was never a round-1 → loop-1 behavior flip.** The bar has not dropped in ANY build. Everything is now consistent and no mechanism needs inventing: the C++ demonstrably pushes correct falling values to a VALID widget every poll (`[TASK122DIAG]`: `BarWidget=VALID -> OnHPChanged PUSHED`, Knight 200→6, wall 300→152, hero 200→46.8), and the widget ignores them — in every build. **QA's ITEM A CONCLUSION STANDS AND IS CORRECT (loop-1 C++ = harmless hardening, KEEP it); only its supporting mechanism (H1 contrast) was wrong, and both it and the "unexplained flip" were artifacts of a mistaken playtest report.** No experiment needed. **The defect is inside `WBP_UnitHealthBar` — TASK-123 REOPENED.** Prime suspect: `OnHPChanged` / `SetTeamColor` authored as `K2Node_CustomEvent`s rather than true `bOverrideFunction=true` overrides of the C++ BlueprintImplementableEvents — indistinguishable in a DSL dump, wired identically, never called by C++, no "Accessed None". One structural cause for BOTH the frozen fill AND the missing tint. Standing lesson: a graph can be perfectly wired and never execute — prove execution, not structure. **VERDICT: KEEP the C++ (no churn revert) — the fallback is legitimate null-safe hardening and `ApplyTeamTint()` closes round-1 WARN-1. 1b IS NOT CLOSED BY TASK-122; the real user-visible fix is TASK-127.** Ship gates: TASK-128 must strip every `[TASK122DIAG]` line AND the orphaned `DiagPollCount` member before TASK-124 phase-B recompiles/commits (they log at Warning, per-actor per-poll). Dormant NIT: `bTeamTintApplied` keys on "tinted once ever", so a destroyed/recreated widget would re-resolve but early-out untinted — cannot occur today (Screen-space widget persists for the actor's life); fix by resetting the latch when `BarWidget` is reassigned, if ever needed. Report: `qa/TASK-122-report.md` "QA LOOP 1 — ITEM A adjudication" (0 BLOCKER / 2 WARN / 2 NIT). Prior loop context retained below for the record. Loop context: Jonathan PIE'd the new DLL (editor started 23:45:25 > DLL 23:42:34, and bars ARE always-visible, so he IS on the new code): *"the health bars are now always visible, but they still do not drop when the characters and tower take damage."* **1a shipped; 1b NOT fixed.** Two prior verdicts are now EMPIRICALLY REFUTED: (i) the programmer's "1b was merely a symptom of 1a", and (ii) art-director's TASK-123 elimination of `BarWidget == nullptr` (it proved only that `WidgetClass` was set, not that `GetWidget()` returned a castable widget). **Decisive new evidence from Jonathan: overhead bars are NOT team-tinted on EITHER team, and the hero DOES have a bar** (so art-director's "hero bar hidden" was an editor-time artifact, not PIE behavior). `SetTeamColor` (`HealthBarComponent.cpp:86`) and `OnHPChanged` (`:162`) sit behind the SAME `if (BarWidget)` guard, while `SetVisibility` (`:144`) runs AHEAD of it — a persistently-null `BarWidget` explains visible + frozen + untinted in one stroke. The self-healing re-resolve (`:153-156`) is NOT rescuing it: had it ever succeeded, the fill would work and only the tint would be missing (WARN-1's exact signature) — we observe neither. Suspect site: `SetWidgetClass(LoadedWidgetClass)` → `Cast<UUnitHealthBarWidget>(GetWidget())` at `:75-76`. WARN-1's `ApplyTeamTint()` fix is PRE-AUTHORIZED into this loop. Prior verdict retained for the record: (qa-passed, orchestrator-proxied 2026-07-09 — qa-reviewer has no partial-edit tool. Report: `qa/TASK-122-report.md` — 0 BLOCKER / 1 WARN / 3 NIT. WARN-1: the self-healing BarWidget re-resolve heals the fill but NOT the team tint (SetTeamColor runs only in BeginPlay), so a late-resolved widget renders untinted; dead code in the common path, becomes ship-blocking only if TASK-124 phase-B PIE shows the deferred-widget path is live. QA ruling on the orchestrator's doubt: an edge-gated OnHPChanged push (a hidden second bug) is REFUTED — the old code pushed every poll, guarded only by `if (BarWidget)`. Truth is most likely imprecise observation compounded by hide-at-full, BUT a WBP-render defect (graph wires SetPercent; the ProgressBar fill *brush* may not reflect percent) and a runtime-null BarWidget remain OPEN — neither is statically provable. The always-visible C++ fix is necessary but NOT proven sufficient. TASK-123/124 PIE must therefore be a hard gate: "fill visibly moves AND is team-tinted, both teams" — no rubber-stamp.) **MANAGER AMENDMENT 2026-07-10 (ITEM A — QA RE-REVIEW MANDATE, binding on this loop; supersedes the "still do not drop" note above with the latest PIE result):** the loop-1 DLL was PIE'd and the `[TASK122DIAG]` log is now IN HAND — 40 polls show the fill DRAINS (Current<Max: hero 200→160→60→128 w/regen, archer 45→25, cavalry 140→80; each `BarWidget=VALID -> OnHPChanged PUSHED` across 23 actors). Jonathan's loop-1 verdict: *"the health bars are working, however … make the fill red for enemies / blue for friendly and the background grey"* → 1b (drain) now WORKS; the remaining ask is ITEM B color (TASK-127). **THE ANOMALY QA MUST ADJUDICATE (do NOT close as "works, ship it"):** the programmer's stated root cause — "SetWidgetClass construction side-effect never fired → GetWidget() null" — is REFUTED by the very log that proves the fix: all 23 actors logged `side-effect-created=YES` + cast `VALID`, and the explicit CreateWidget fallback NEVER executed. `git diff` shows the loop-1 change is functionally NEAR-IDENTICAL to the round-1 code that FAILED (same SetWidgetClass→GetWidget→Cast→`if (BarWidget)` shape); the only real deltas are the never-taken fallback, the `ApplyTeamTint()`/`bTeamTintApplied` refactor, and the `[TASK122DIAG]` logs — none of which plausibly explains why round-1 read visible+frozen+untinted and loop-1 reads draining. **qa-reviewer MUST state PLAINLY whether the loop-1 C++ change is LOAD-BEARING or merely HARMLESS HARDENING**, adjudicating against the diff + log. Leading hypothesis (manager + orchestrator concur): the C++ was never the bug — round-1 was draining too, but a WHITE fill on a white/near-white track (TASK-123 audit) made it imperceptible and "untinted" was the SAME contrast miss → the real user-visible fix is ITEM B (TASK-127), so the bug is NOT truly closed until TASK-127 lands; QA states whether closure depends on it. A PASS may KEEP the C++ as harmless defensive hardening — the deliverable is the recorded KNOWLEDGE (qa/TASK-122-report.md), not a further code change. The `[TASK122DIAG]` logs are stripped by TASK-128 before the TASK-124 phase-B commit.
- blocked-by: none
- parallel-safe: yes file-wise vs Item 2 (touches HealthBarComponent.cpp/.h only — disjoint from WBP_DeckCardTile); but it is the HEAD of a serial Item-1 chain and its read-only diagnosis uses the single editor
- spec: >
    Files + a READ-ONLY MCP diagnosis pass (NO editor mutation) — the fix for Jonathan's playtest report:
    "the overhead bars on the characters and towers do not drop when they get hit; the castle health bar is
    still working fine, all others are not." TWO changes, per the REVERSED CONVENTIONS "Overhead unit health
    bars (M5.5)" behavior law. Do NOT assume 1a fixes 1b — spec treats them as possibly independent.
    STEP 1 — DIAGNOSE FIRST (read-only, via Unreal MCP BlueprintTools; the editor is up). Verify and RECORD
    in the handoff BEFORE touching code: (a) does `/Game/UI/WBP_UnitHealthBar`'s `OnHPChanged` BIE actually
    drive the ProgressBar `SetPercent(Current/Max)` in its GRAPH (an FName in the name table does NOT prove
    the graph is wired); (b) is `WBP_UnitHealthBar` actually REPARENTED to `UUnitHealthBarWidget` so the
    `Cast<UUnitHealthBarWidget>(GetWidget())` at HealthBarComponent.cpp:80 returns NON-NULL — a null BarWidget
    is the PRIME SUSPECT: the component's own SetVisibility show/hide runs regardless of BarWidget, so the bar
    would APPEAR on damage yet the fill would never update (OnHPChanged at :152-154 is guard-skipped), matching
    the symptom exactly; (c) is a `UHealthBarComponent` named `HPBarWidget` actually constructed on all three
    families (ASummonedUnit, ABuilding, AHeroCharacter) and does `HealthBarWidgetClass` soft-resolve to
    `/Game/UI/WBP_UnitHealthBar`. Context (do NOT re-derive — hand to the fix): the castle works because it is
    a PUSH-model delegate bar (FOnCastleHPChanged, Castle.h), everything else is this POLL-model component
    (~0.15 s timer) — TWO independent systems. Do NOT touch ACastle.
    STEP 2 — 1a DESIGN REVERSAL (C++, HealthBarComponent.cpp PollHealth ~:130): make the bar VISIBLE the whole
    time the actor is alive + opted-in, at FULL HP included. Change the show-gate from
    `bShowHealthBar && bAlive && (Current < Max - HealthBarFullEpsilon)` to `bShowHealthBar && bAlive` (retire
    the hide-at-full epsilon term from the show/hide decision). Hide ONLY on !alive/destruction and when
    `bShowHealthBar` is false. KEEP the per-BP `bShowHealthBar` opt-out (default true) — see the CONVENTIONS
    OPEN QUESTION (FLAG 3); do NOT remove it (Jonathan's call).
    STEP 3 — 1b FILL BUG: ensure `OnHPChanged(Current, Max)` is pushed to a NON-NULL BarWidget every poll while
    shown, so the fill reflects HP live INCLUDING at full (SetPercent 1.0). After 1a "shown" == "alive", so the
    push is driven every poll — BUT if STEP 1 finds BarWidget null / the WBP graph unwired, the C++ change alone
    will NOT move the fill; that repair belongs to TASK-123. Land the C++ side here and HAND the WBP finding to
    TASK-123 in the handoff. NO new HP fields (bind the existing getters); everywhere null-safe; ZERO combat/
    stat behavior change. ACCEPTANCE: compiles warnings-as-errors; the bar shows whenever alive+opted-in (full
    HP included); OnHPChanged is driven every poll while shown; the (a)(b)(c) diagnosis is recorded in the
    handoff. → qa-reviewer (MANDATORY inherited-reflected-member shadow scan AND complete-type-include scan —
    CONVENTIONS coding laws). Post progress/handoff in ⚙️ Dev & QA (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-122`).
- names: >
    `HealthBarComponent.cpp/.h` (Source/GitClaudeUnrealTest/Siegebound/) — PollHealth show-gate + fill drive.
    Keep EditDefaultsOnly `bShowHealthBar` (default true). Reuse only (do NOT redefine): IHealthBarTarget
    GetHealthCurrent()/GetHealthMax()/IsHealthBarActorAlive(); UUnitHealthBarWidget::OnHPChanged(float,float)/
    SetTeamColor(float,float,float); /Game/UI/WBP_UnitHealthBar. Diagnose (read-only): WBP_UnitHealthBar graph
    wiring + reparent to UUnitHealthBarWidget + HPBarWidget construction on ASummonedUnit/ABuilding/AHeroCharacter.
    Law: CONVENTIONS "Overhead unit health bars (M5.5)" (reversed 2026-07-09). Do NOT touch ACastle / its HPBarWidget.

#### TASK-123 — WBP_UnitHealthBar: repair/verify OnHPChanged→SetPercent + reparent + full-bar render (editor)
- assignee: art-director
- status: **SUPERSEDED by the TASK-130..132 health-bar REBUILD** (2026-07-10; target `WBP_UnitHealthBar` was DELETED by TASK-132 @ 61a1e72; forensic history retained below). PRIOR: REOPENED round-3 CLOSED — widget EXONERATED by execution-level proof → needs-orchestrator-routing (art-director 2026-07-10; handoff `handoffs/TASK-123.md` REOPENED section). Prime hypothesis (BIEs authored as Custom Events) REFUTED: both OnHPChanged & SetTeamColor are genuine `K2Node_Event` overrides (same class + type_id as the working WBP_CastleHealthBar control), NOT K2Node_CustomEvent. Temporary PrintString diagnostics PROVED both events FIRE at runtime (OHC every poll, STC once/widget). DECISIVE `GetPercent` live readback PROVED the ProgressBar's actual Percent DROPS and HOLDS: hero bar 0.85→0.775→0.7 as it took combat damage; full units read 1.0 → SetPercent sticks, no reset/binding, fill fraction genuinely drops. All fallbacks refuted: divide=Current/Max (castle-identical); BarFillStyle=Scale (castle-identical); single ProgressBar (CDO exposes one `bar`); guard MaxHP>0 taken. The whole chain C++(falling values)→OnHPChanged(fires)→SetPercent(Bar.Percent drops & holds) is proven working; only Percent→pixels is unobservable headless (works for castle w/ identical config). ⇒ NO widget-asset defect; nothing to fix in WBP_UnitHealthBar. Diagnostics fully removed, graph restored to clean TASK-127 state (11 nodes, DSL matches), compiled clean, SAVED (is_dirty=false), TASK-127 grey track kept. ROUTING: either (a) fill drops now & report is stale (Jonathan retracted a prior report; TASK-127 fixed the black-on-black contrast) — Jonathan visual re-verify; or (b) screen-space UWidgetComponent (UHealthBarComponent, C++) presentation/refresh — route to gameplay-programmer, diff its render/redraw settings vs ACastle::HPBarWidget. NO Source/ edit, NO Git, castle/menu untouched, [TASK122DIAG] C++ logs left for TASK-128.
- blocked-by: TASK-122 (needs its recorded diagnosis + the phase-A compile from TASK-124 to PIE-verify the always-visible behavior)
- parallel-safe: no (editor-mutating — single editor instance; the fix depends on TASK-122's diagnosis)
- spec: >
    Editor/MCP only — needs the editor MCP up (else park + tell the orchestrator). Close the 1b fill bug on
    the WIDGET side and confirm the 1a always-visible rendering, per the REVERSED CONVENTIONS "Overhead unit
    health bars (M5.5)" law and TASK-122's recorded diagnosis. On `/Game/UI/WBP_UnitHealthBar`: (1) CONFIRM it
    is REPARENTED to `UUnitHealthBarWidget` (readback the parent class — if not, reparent it; a wrong/missing
    parent makes HealthBarComponent.cpp:80 Cast<UUnitHealthBarWidget> null → OnHPChanged never fires → the fill
    never moves). (2) VERIFY/REPAIR the graph so the `OnHPChanged(float CurrentHP, float MaxHP)` BIE actually
    drives the ProgressBar `SetPercent(CurrentHP / MaxHP)` guarding MaxHP > 0 (the WBP_CastleHealthBar
    contract); check the ProgressBar has NO competing Percent binding/override that ignores SetPercent. (3)
    Confirm `SetTeamColor(float R,float G,float B)` still tints the FILL brush. (4) Confirm a FULL bar renders
    VISIBLY FULL (SetPercent 1.0) now that the bar is shown at full HP — hide-at-full meant a full bar was
    NEVER displayed before, so this render path is newly exercised; the track + fill must read cleanly at full.
    Keep it HitTestInvisible, compact (90×12 DrawSize), no baked text. ADDITIVE to WBP_UnitHealthBar ONLY — do
    NOT touch WBP_CastleHealthBar (castle keeps its own working delegate bar) or any other widget. If TASK-122's
    diagnosis proved the graph + reparent already correct, this is a verify-and-confirm pass (record it) — do
    NOT re-author a working graph. Art skips QA → build-master integration check (TASK-124). Post the handoff in
    🎨 Art (`🎨 ART-DIRECTOR: … TASK-123`) with the parent-class + OnHPChanged→SetPercent wiring readback.
- names: >
    `WBP_UnitHealthBar` at /Game/UI/WBP_UnitHealthBar, parent class `UUnitHealthBarWidget`. BIEs:
    OnHPChanged(float CurrentHP, float MaxHP) → ProgressBar SetPercent(guard Max>0); SetTeamColor(float R,
    float G, float B) → fill brush tint. Donor (if a rebuild is ever needed): /Game/UI/WBP_CastleHealthBar.
    Do NOT touch WBP_CastleHealthBar.

#### TASK-124 — Health-bar fix integration: compile + always-visible/fill-drop PIE verify + commit (build)
- assignee: build-master
- status: **SUPERSEDED by the TASK-130..132 health-bar REBUILD** (2026-07-10; this first-attempt integration's phase-B never committed — the rebuild's own integration TASK-132 @ 61a1e72 shipped the overhead-bar fix; forensic history retained below). PRIOR: phase-A DONE (2026-07-10; loop-1 DLL compiled clean 00:51:28 — unblocked the WBP/color work). phase-B PENDING the ITEM A qa verdict + TASK-127 (ITEM B color) + TASK-128 (diagnostics stripped). **MANAGER AMENDMENT 2026-07-10:** phase-B PIE now ALSO hard-gates Jonathan's ITEM B color scheme — fill BLUE friendly / RED enemy, unfilled track GREY, unmistakable contrast, both teams (the "fill visibly moves AND is team-tinted" gate from TASK-122) — and REQUIRES the `[TASK122DIAG]` logs removed (TASK-128) before it recompiles + commits. Keep the diagnostics ON through the color PIE check; TASK-128 strips them LAST, immediately before this commit. One Item-1 commit covers TASK-122/124 + 127 + 128 (TASK-123 was a no-op diagnosis).
- blocked-by: TASK-127 (ready-for-integration, ITEM B color) + TASK-128 (qa-passed, diagnostics stripped); TASK-122 ITEM-A qa verdict recorded; TASK-123 closed not-asset-side
- parallel-safe: no
- spec: >
    Build-master integration for the health-bar fix (TASK-112 two-phase pattern). SINGLE owner, TWO PHASES:
    (PHASE A) compile the TASK-122 C++ change via the standard Build.bat command (editor-bounce protocol — the
    editor releases the DLL) so the reversed always-visible component behavior is live and
    `UUnitHealthBarWidget`/`UHealthBarComponent` are present — this UNBLOCKS TASK-123 (the WBP repair needs the
    compiled behavior to PIE-verify against). Pre-compile: scan the change for inherited-reflected-member
    shadows + the complete-type-include law (CONVENTIONS). Any error → append to TASK-122's QA report, set
    qa-failed, stop (counts as a QA loop; build-master never edits code).
    (PHASE B — after TASK-123's WBP lands) re-verify the build compiles clean warnings-as-errors, then run the
    PIE suite in a Play-vs-Bot session in L_Arena: every friendly unit, tower, wall, Barracks, Deep Mine, miner,
    and the hero shows an overhead bar that (a) is VISIBLE at FULL HP (the reversal — no longer hidden at full),
    (b) DROPS live as the actor takes damage (the fill reflects Current/Max — the bug), (c) is team-tinted (blue
    friendly; red via the bot's units/towers), (d) hides only on death/destruction, and (e) the castle still
    shows ONLY its own delegate bar (no duplicate) and gold nodes show none. Drive damage headlessly with the
    TASK-121 cheats (`ApplyTestDamage`, `SummonTestUnit`) to sidestep the locked-desktop no-input debt; record
    any residual on-screen-pixel confirmation as owed to Jonathan's playtest (Slate widgets are uncapturable
    headless — the TASK-112 WATCH). If the editor MCP is down, compile via Build.bat and report the PIE items as
    owed (never fake). On PASS: commit code + WBP_UnitHealthBar with the task-ID message. Do NOT push, NO new
    branch (a fix batch, not a milestone slice — m6-testable already preserves M6). Build failure → append
    errors to the offending task's QA report and route back to gameplay-programmer (counts as a QA loop). Post
    compile result + commit hash in 🔧 Build & Git (`🔧 BUILD-MASTER: … TASK-124`).
- names: >
    Assets/classes exactly as the TASK-122/123/127/128 names blocks. Commit on `main`, message pattern
    "TASK-122..124/127/128: overhead health bars — always-visible, fill tracks HP, blue/red team fill + grey
    track (hide-at-full reversed per Jonathan; diagnostics stripped)". No push, no branch.

#### TASK-125 — WBP_DeckCardTile: render as a physical card (same in-game card art) + above-card in-deck copy count (editor)
- assignee: art-director
- status: done (commit 274c160 via TASK-126, 2026-07-10; not pushed). PRIOR: ready-for-integration (art-director 2026-07-10; handoff `handoffs/TASK-125.md`). WBP_DeckCardTile re-skinned, compiled clean, SAVED (is_dirty=false). NO C++, NO Git; WBP_CardHand/WBP_DeckBuilder/WBP_UnitHealthBar/WBP_CastleHealthBar/WBP_MainMenu untouched. (1) CARD ART: new CardArtBorder (fill, SelfHitTestInvisible) brush = GetCardArtTexture(CardID) — the SAME T_CardArt_<CardID> the hand uses — applied once in SetupCell. (2) ABOVE-CARD COUNT: new CopyCountText (font22) in a top-right dark plate (black@0.65), plain GetCountOf (in-progress WorkingDeck); /max DROPPED (cap-grey still signals cap). (3) Name/Cost in a bottom dark caption plate (black@0.55) = legibility guard. +/− + cap-grey KEPT. NODE-CLASS CHECK (anti-custom-event): WBP_DeckBuilder OnDeckModelChanged + OnDeckSlotCountChanged = genuine K2Node_Event overrides (AddEvent|Siegebound|Deck|...) → RefreshAll → RefreshCell WILL fire. [TASK125DIAG] PrintString LEFT LIVE in RefreshCell (fires on Jonathan's +/−); TASK-126 must STRIP it (mirror TASK-128). Could NOT machine-run the +/− click (deck builder opens only via a menu click; locked desktop, no SendInput, no MCP UFUNCTION/exec to trigger AddCopy) → count-updates-on-click + legibility-over-art + art-renders = HUMAN WATCH owed to Jonathan (TASK-126 carries). AssignOnClicked MCP quirk recurred (buttons auto-bound to empty OnClicked_Event_13/14) → FIXED by authoring RemoveCopy/AddCopy bodies into those bound events (verified wired, single instances); OnAddPressed/OnRemovePressed now orphaned dupes + donor cruft = M7 sweep. Card is content-sized (fixed card size lives in WBP_DeckBuilder WrapBox slot, not touched) = sizing polish follow-up.
- blocked-by: TASK-124 (Jonathan's ordering — Item 1 lands first; and the single-editor rule serializes it after the Item-1 editor/build work)
- parallel-safe: no (editor-mutating — single editor instance; sequenced after Item 1)
- spec: >
    Editor/MCP only — needs the editor MCP up (else park + tell the orchestrator). Jonathan playtest request:
    the deck builder "needs to have physical cards displayed so I can test properly — just use the same displays
    you did for the card displays … for the actual match … for now." Re-skin the EXISTING per-card tile
    `/Game/UI/WBP_DeckCardTile` (created in TASK-118) so each browser-grid cell RENDERS AS AN ACTUAL CARD using
    the SAME card-face treatment as the in-match hand (`WBP_CardHand`), per CONVENTIONS "Deck-builder & saved
    decks (M6)" (the WBP_DeckCardTile card-face note) + "Card artwork (hand UI)" Face composition law. NO new
    C++ and NO new asset: reproduce the card-face composition — the `T_CardArt_<CardID>` texture as the
    BACKGROUND layer, DisplayName + Cost overlaid legibly on top (translucent contrast strip / shadow behind
    text allowed), art HitTestInvisible. Drive EVERYTHING from the EXISTING `UDeckBuilderWidget` resolvers the
    tile already reaches (the same reference its +/− buttons use to call AddCopy/RemoveCopy): `GetCardArtTexture
    (CardID)` (background art; null → text-only face fallback = today's look), `GetCardDisplayName(CardID)`,
    `GetCardCost(CardID)`, plus the existing `GetCountOf(CardID)` / `GetCardMaxCopies(CardID)` for the copy
    counter + cap-grey. KEEP the tile's existing +/− buttons (AddCopy/RemoveCopy) and count wiring from TASK-118
    fully INTACT — this is a VISUAL re-skin, additive only. Do NOT touch `WBP_CardHand` (the hand's face is
    inlined there — no shared card-face sub-widget exists to extract; extraction is explicitly deferred per
    Jonathan's "for now") and do NOT touch `WBP_DeckBuilder`'s grid / counter / legality logic. ACCEPTANCE: each
    grid tile shows the card art + name + cost like a hand card; the +/−, count, and cap-grey still work;
    WBP_CardHand and WBP_DeckBuilder grid logic unchanged. Art skips QA → build-master integration check
    (TASK-126). Post the handoff in 🎨 Art (`🎨 ART-DIRECTOR: … TASK-125`) with the asset path.
- names: >
    `WBP_DeckCardTile` at /Game/UI/WBP_DeckCardTile (existing, TASK-118). Card-face = T_CardArt_<CardID> art
    background + DisplayName + Cost overlay (CONVENTIONS Face composition). Drive via existing UDeckBuilderWidget
    resolvers: GetCardArtTexture(FName)/GetCardDisplayName(FName)/GetCardCost(FName)/GetCountOf(FName)/
    GetCardMaxCopies(FName). Do NOT touch WBP_CardHand or WBP_DeckBuilder grid logic; keep the tile's AddCopy/
    RemoveCopy +/− wiring.
- **MANAGER AMENDMENT 2026-07-10 (Jonathan sharpened requirements — two parts):** verbatim: *"add the card visuals … just use the same images that were generated for the in-game cards … Also, add a number above each card that shows how many you have of it in the current deck build."*
    (1) **Card visuals = the SAME in-game images (CONFIRMED against the header, ZERO new C++).** `UDeckBuilderWidget` (DeckBuilderWidget.h:83-128) already exposes `GetCardArtTexture`/`GetCardDisplayName`/`GetCardCost`/`GetCardMaxCopies`/`GetCountOf`; `GetCardArtTexture` resolves the SAME `T_CardArt_<CardID>` textures the hand uses (CONVENTIONS "Card artwork (hand UI)"). No resolver is missing → NO programmer task. If art-director finds a resolver actually absent at author time, STOP and tell the orchestrator — do NOT add C++ yourself.
    (2) **Above-card copy count — RULING: (b) reposition/restyle an EXISTING element, NOT new C++, WITH a (c) legibility guard.** Evidence (handoffs/TASK-118.md:17-22): the tile ALREADY has a `CountText` TextBlock driven by `GetCountOf`, today rendered as "count/max" in the BOTTOM `HBox[− CountText +]`. `GetCountOf` is CONFIRMED to return the IN-PROGRESS working deck's copies (DeckBuilderWidget.cpp:134-137 reads `WorkingDeck.Cards[Index].Count`), NOT the saved deck — correct data source. Deliverable: PRESENT that count as a PROMINENT NUMBER ABOVE the card face (the copies-in-current-build count — a "×2"/"2" badge at the top of the tile) — repositioning + restyling the existing count, NO new data path. Drop the "/max" ratio above the card (the "+"-greys-at-cap already signals the cap); show the plain in-deck count. **(c) GUARD — the health-bar failure mode in miniature:** once the card ART becomes the tile background, the count number AND the name/cost overlay can go invisible/illegible against the art (exactly the white-fill-on-white-track bug this session burned three root causes on). The count + text MUST be verified LEGIBLE on top of the art (contrast strip / outline / drop shadow), not merely present in the widget tree.
    (3) **RUNTIME-EXECUTION CONSTRAINT (hard-won this session — bake it in, do NOT skip):** the tile's on-screen count updates through a C++→BP path — `AddCopy`/`RemoveCopy` fire `OnDeckModelChanged()` / `OnDeckSlotCountChanged()` (BIE overrides on WBP_DeckBuilder) → `RefreshAll()` → each tile's `RefreshCell()`. Graph structure ("the nodes are wired", `bIsImplemented:true`) does NOT prove execution: a BIE authored as a `K2Node_CustomEvent` instead of a true `bOverrideFunction=true` override is DSL-INDISTINGUISHABLE and is NEVER called from C++ (the exact defect class that hid behind TASK-122's graph readbacks). VERIFY the refresh path EXECUTES at runtime — a `Print String` / `[TASK125DIAG]` log inside `RefreshCell` (or the BIE) proving the above-card number ACTUALLY changes on a +/− click — not merely that the graph exists. Strip that diag before handing to TASK-126, or flag it for TASK-126 to strip (mirror TASK-128).
    (4) **On-screen truth = human WATCH.** If art-director cannot see rendered pixels (screen-space Slate is uncapturable headless; locked desktop = no input), NAME the "count visible + legible over art + updates on +/−" check as a human WATCH owed to Jonathan — never infer it from tree structure. TASK-126 carries it. names addition: above-card count element (reposition the existing `CountText`, or a new `CopyCountText`) driven by `GetCountOf`, styled legibly over the art, ABOVE the card face.
    Sequencing UNCHANGED: TASK-125 stays blocked-by the whole Item-1 health-bar chain (reopened TASK-123 → TASK-128 → TASK-124 phase-B); Jonathan reconfirmed "after that finishes"; TASK-125 is editor-mutating so it serializes against TASK-123 on the single editor. Do NOT dispatch early.

#### TASK-126 — Deck-builder physical-cards integration: PIE verify + commit (build)
- assignee: build-master
- status: done (commit 274c160, 2026-07-10 — WBP_DeckCardTile.uasset only, LFS pointer; not pushed)
- blocked-by: TASK-129 (strip [TASK125DIAG] first) — TASK-125 APPROVED by Jonathan 2026-07-10 (*"the deck builder fixes are fine"*; human WATCH satisfied)
- parallel-safe: no
- spec: >
    Build-master integration for the deck-builder physical-cards re-skin (art-only chain — no new C++). Re-verify
    the build compiles clean warnings-as-errors (the editor bounce may have touched the DLL). PIE from L_MainMenu
    → Deck Builder: confirm the 28-card grid now renders each tile as a PHYSICAL CARD (art background + name +
    cost, matching the in-match hand look); the +/− still add/remove copies; the x/50 counter, average-cost
    readout, cap-grey, and the legal-gated "Play with this deck" all still work (TASK-120 behavior preserved);
    WBP_CardHand and the rest of WBP_DeckBuilder are visually/functionally unchanged. Live click-through of the
    grid is a human WATCH on the locked desktop (no SendInput — TASK-076/112 doctrine): machine-verify the tile
    renders the art + text and record the click-feel as owed to Jonathan. If the editor MCP is down, report the
    PIE items as owed (never fake). On PASS: commit `WBP_DeckCardTile` (+ any WBP_DeckBuilder tile-instance
    deltas) with the task-ID message. Do NOT push, no new branch. Post compile/verify result + commit hash in
    🔧 Build & Git (`🔧 BUILD-MASTER: … TASK-126`).
- names: >
    `WBP_DeckCardTile` per the TASK-125 names block. Commit on `main`, message pattern "TASK-125..126: deck-
    builder tiles render as physical cards (same in-game card art) + above-card in-deck copy count". No push, no branch.
- **MANAGER AMENDMENT 2026-07-10 (verify Jonathan's sharpened requirements):** the PIE suite ALSO hard-gates — (a) each tile renders the SAME in-game card art (`T_CardArt_<CardID>` via `GetCardArtTexture`) as the hand; (b) a PROMINENT copy-count number ABOVE each card face shows the IN-PROGRESS deck count (`GetCountOf`) AND it ACTUALLY UPDATES on a +/− click at RUNTIME (execution-proven via the TASK-125 `[TASK125DIAG]` / Print String — NOT graph readback; watch for a BIE authored as a CustomEvent that never fires); (c) the count + name + cost are LEGIBLE over the art (the white-on-white failure mode). If any `[TASK125DIAG]` diag survives, STRIP it before commit (mirror TASK-128). Live click-through on the locked desktop is a human WATCH owed to Jonathan — record it explicitly; never infer on-screen legibility / count-update from the widget tree.
- **MANAGER AMENDMENT 2026-07-10 #2 (Directive 1 — APPROVED, SCOPED COMMIT):** Jonathan approved the deck-builder (*"the deck builder fixes are fine"* — the human WATCH is satisfied; card art + above-card count both accepted). Commit it NOW, but **SCOPE THE COMMIT TO `Content/UI/WBP_DeckCardTile.uasset` ONLY.** It must NOT sweep in any health-bar work (`HealthBarComponent.cpp/.h`, `WBP_UnitHealthBar.uasset`, or any Source/ rebuild files) — those are being TORN DOWN + rebuilt (TASK-130..132) and must not ride this commit. NO compile needed (TASK-129 removed a Blueprint node, not C++; verify the build is still clean but expect no code delta). Do NOT revert the WBP_CastleHealthBar / WBP_MainMenu churn here — that folds into the rebuild commit (TASK-132). Commit message: "TASK-125/126/129: deck-builder tiles render as physical cards + above-card in-deck copy count". No push, no branch.

#### TASK-127 — WBP_UnitHealthBar bar colors: team-tinted fill (blue friendly / red enemy) + grey track + contrast (editor)
- assignee: art-director
- status: **SUPERSEDED 2026-07-10 by the health-bar REBUILD (TASK-130..132)** — Jonathan: the bars STILL do not visibly drop, directed a rebuild from scratch; this task's target `WBP_UnitHealthBar` is DELETED by TASK-132. NOTE FOR THE RECORD: this task's machine-level "fill drive PROVEN via [TASK122DIAG]" is EXACTLY the trap the rebuild's screenshot gate exists to catch — `OnHPChanged` PUSHED + HP decreasing does NOT prove the rendered pixels moved; the on-screen result stayed a frozen bar. Prior status retained below. ~~ready-for-integration (art-director 2026-07-10; handoff `handoffs/TASK-127.md`). WBP_UnitHealthBar `Bar` track authored to canonical grey `backgroundImage.tintColor=(0.03,0.03,0.03)@0.7` (was black (0,0,0)@0.5); fill `fillImage.tintColor` verified white (identity) so the C++-pushed `SetTeamColor` blue/red shows undimmed; fill color stays C++ data (NOT hardcoded); grid fill material KEPT (parity with the known-good WBP_CastleHealthBar, which uses the identical brush legibly). Compiled clean + saved (is_dirty=false) → also supersedes the churn-only WBP_UnitHealthBar residue. PIE(L_Arena, real combat) `[TASK122DIAG]` PROVES fill-drive + tint path end-to-end: BarWidget=VALID + OnHPChanged PUSHED with live DECREASING HP on units (Knight 200→6, Cavalry 140→20, Longbowman 70→25, Ogre 500→386), TOWERS/walls (Wall 300→152; ArrowTower×3), and the hero (200→46.8→200; TASK-123 hero bVisible anomaly did NOT reproduce) — both teams (hero=Blue, bot units+towers=Red). Zero LogBlueprint errors. RESIDUAL HUMAN WATCH (does not block commit): on-screen pixel colors/contrast — screen-space Slate uncapturable + live widget FillColorAndOpacity unserializable (TASK-112 WATCH). No Git, no Source/ edit, WBP_CastleHealthBar/WBP_MainMenu untouched, [TASK122DIAG] logs left for TASK-128.
- blocked-by: TASK-122 (ITEM A qa verdict — B may BE the original bug's fix, so it lands after QA adjudicates; TASK-123's WBP audit is the input evidence)
- parallel-safe: no (editor-mutating — single editor instance; WBP_UnitHealthBar)
- spec: >
    Editor/MCP only — needs the editor MCP up (else park + tell the orchestrator). Jonathan loop-1 playtest
    request: *"make the part of the bar that shows the health RED if they are enemies and BLUE if they are
    friendly, and the background of the bar GREY."* Per CONVENTIONS "Overhead unit health bars (M5.5)" → the
    "Bar colors (ITEM B)" note. On `/Game/UI/WBP_UnitHealthBar` (additive, NO C++): (1) VERIFY the team tint
    actually RENDERS on the FILL — trace `SetTeamColor(float R,float G,float B)` → `SetFillColorAndOpacity(Bar,…)`
    and confirm the pushed color LANDS and is NOT swamped: set the fill `fillImage.tintColor` to neutral white
    (1,1,1 = identity multiply) so the pushed blue (0.05,0.30,1.00) / red (1.00,0.10,0.05) shows undimmed;
    confirm nothing (authored `FillColorAndOpacity`, a Percent binding, or `SetPercent`) overwrites the tint each
    frame. The fill COLOR stays DATA-driven from C++ (`BlueBarColor`/`RedBarColor` via `SetTeamColor`) — do NOT
    hardcode blue/red in the WBP. (2) Set the BACKGROUND / unfilled TRACK brush to a fixed neutral GREY
    (canonical ~ linear (0.03,0.03,0.03) @ ~0.7 alpha — a static widget style authored HERE, NOT a runtime
    param) that CONTRASTS clearly with BOTH the blue and red fill — this closes the round-1 white-fill-on-white-
    track contrast miss (TASK-123 audit found `Bar` `FillColorAndOpacity` + `fillImage.tintColor` both authored
    WHITE). (3) Confirm a full bar (SetPercent 1.0) reads as a clearly-full team-colored fill on the grey track,
    and the drain is unmistakable. Keep it HitTestInvisible, compact (90×12 DrawSize), no baked text. ADDITIVE to
    WBP_UnitHealthBar ONLY — do NOT touch WBP_CastleHealthBar or any other widget (this also SUPERSEDES the
    churn-only WBP_UnitHealthBar dirty-uasset residue). Art skips QA → build-master integration check (TASK-124
    phase-B). Post the handoff in 🎨 Art (`🎨 ART-DIRECTOR: … TASK-127`) with the fill-tint + track-brush readback.
- names: >
    `WBP_UnitHealthBar` at /Game/UI/WBP_UnitHealthBar. Fill: `SetTeamColor(float R,float G,float B)` →
    `SetFillColorAndOpacity(Bar,…)`; `fillImage.tintColor` = white (1,1,1). Track/background brush: fixed grey
    ~(0.03,0.03,0.03)@~0.7α (CONVENTIONS "Bar colors (ITEM B)"). Fill color stays C++ data
    (`UHealthBarComponent::BlueBarColor`/`RedBarColor`). Do NOT touch WBP_CastleHealthBar.

#### TASK-128 — Strip the temporary [TASK122DIAG] diagnostics from HealthBarComponent.cpp (C++)
- assignee: gameplay-programmer
- status: **SUPERSEDED / MOOT 2026-07-10 by the rebuild** — `HealthBarComponent.cpp` (this task's target) is DELETED by TASK-130 (the poll system is retired), so there is nothing to strip. Any leftover `[TASK122DIAG]` dies with the file. (Prior: backlog.)
- blocked-by: TASK-127 (keep the diagnostics ON through the ITEM B color PIE check; strip them LAST, right before the TASK-124 phase-B commit)
- parallel-safe: no (edits HealthBarComponent.cpp — same file as TASK-122; sequenced after the color verification)
- spec: >
    Files only — NO editor/MCP. The `[TASK122DIAG]` logs the programmer added to `HealthBarComponent.cpp`
    (currently at WARNING level, firing per-actor per-poll) are TEMPORARY diagnostics and MUST NOT ship.
    REMOVE them entirely (default), or — if a single line is worth keeping as a permanent trace — demote it to
    Verbose. Change NOTHING else: the always-visible show-gate (TASK-122), the fill push, `ApplyTeamTint()` /
    `bTeamTintApplied`, and every null-safe guard stay byte-for-byte. This is a PURE log removal — zero behavior
    change. Do it AFTER TASK-127's color scheme is PIE-verified (the diagnostics are useful signal through that
    check) and BEFORE TASK-124 phase-B recompiles + commits. ACCEPTANCE: compiles warnings-as-errors; no
    `[TASK122DIAG]` (nor any Warning-level per-poll log) remains; the diff is log-lines-ONLY — no collateral
    change to the show-gate / fill / tint. → qa-reviewer (confirm the diff is diagnostics-removal ONLY; the
    shadow + complete-type-include scans are trivially clean on a log removal but run them). Post in ⚙️ Dev & QA
    (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-128`).
- names: >
    `HealthBarComponent.cpp` (Source/GitClaudeUnrealTest/Siegebound/) — remove the `[TASK122DIAG]` UE_LOG lines
    (or demote to Verbose). No other change. Rides the TASK-124 phase-B compile + commit.

---

### Health-bar REBUILD from castle parity + deck-builder finalize (2026-07-10) — TASK-129..132

**Source:** Jonathan back at the machine (desktop UNLOCKED — real GDI screenshots now possible, the capability missing for all 5 failed attempts). TWO directives. **Directive 1 — deck-builder APPROVED** (*"the deck builder fixes are fine"*): finalize via TASK-129 (strip the [TASK125DIAG] BP node) + TASK-126 (scoped commit, WBP_DeckCardTile.uasset ONLY). **Directive 2 — health bars FULL REBUILD FROM SCRATCH** (*"there is still the same issue with all the unit health bars … rebuild the entire feature from scratch, make sure to make enemy health bars red and friendly health bars blue"*). The castle bar is left UNTOUCHED (it works).

**SUPERSEDES the old Item-1 health-bar chain:** TASK-127 (color the old WBP_UnitHealthBar) and TASK-128 (strip [TASK122DIAG]) are RETIRED — their target files are DELETED by the rebuild; TASK-124's phase-B is replaced by TASK-132. TASK-122/123 stay as the orchestrator's correction-chain record (untouched here).

**ARCHITECTURE RULING (manager, castle-parity ADOPTED) — PUSH / delegate model, NOT poll.** The castle bar is the ONLY screen-space widget-component HP bar in this project that demonstrably renders + updates; it uses a PUSH model (`ACastle` owns `FOnCastleHPChanged`, broadcasts on damage, `UCastleHealthBarWidget::InitForCastle` seed-then-binds). The failed unit bars POLL `IHealthBarTarget` on a timer. On the broken build EVERY machine check passes (widget valid, Cast valid, `OnHPChanged` PUSHED, `BarPercent==Cur/Max`) yet the pixels never move — so the defect is in the render link no machine check saw. **Copy the one thing that works** (widget + data model); if a faithful castle-mirror STILL fails, the cause is environmental, not architectural. Weighed the counter-argument (the poll data path is machine-proven to deliver floats) and rejected it: 5 failures + "from scratch" + the mandate to start from the working castle ⇒ full parity (removes the poll timer as a variable) is the lower-risk bet.

**REPLACE-IN-PLACE vs FRESH — RULING: FRESH assets + FRESH classes, RETIRE the old.** "From scratch" per Jonathan; the old `WBP_UnitHealthBar` is tainted across 5 failures. RETIRE `IHealthBarTarget`/`UHealthBarComponent`/`UUnitHealthBarWidget`/`WBP_UnitHealthBar`. The old component was added in the 3 base-class CONSTRUCTORS (not per-BP), so swapping it for the new component needs NO per-BP rewiring — the reason FRESH is cheap here.

**PRIME SUSPECT to hand the programmer (do NOT re-derive):** the old WBP_UnitHealthBar's fill brush is the `DefaultWhiteGrid_Low` MATERIAL; a material fill brush may not visually respond to `SetPercent`/`FillColorAndOpacity` the way a plain image does — and TASK-127 KEPT that material brush claiming castle-parity. The rebuild's widget is a FRESH DUPLICATE of the WORKING `WBP_CastleHealthBar`, preserving the castle's exact fill-brush setup (diff against the castle; do NOT diverge). IF TASK-132's screenshot STILL shows a frozen fill, switching the fill to a PLAIN IMAGE brush is the first change to try — but do not pre-emptively diverge from the working castle.

**BINDING LAWS from tonight (on the rebuild):** (1) prove EXECUTION not structure — a BIE authored as `K2Node_CustomEvent` vs a true `bOverrideFunction=true` override is DSL-indistinguishable and NEVER fires from C++ (verify the node class). (2) prove RENDERED PIXELS not UPROPERTY values — `GetPercent()` reads the UPROPERTY, not what Slate paints; this gap hid the bug for 5 attempts. (3) report the OBSERVATION, not the conclusion. (4) RED = ENEMY, BLUE = FRIENDLY (recorded law); ALWAYS-VISIBLE while alive; HIDDEN on death; BOTH teams; castle bar untouched; gold nodes none.

**HARD EXIT CRITERION (the unlocked-desktop capability):** the feature is NOT fixed until a REAL GDI SCREENSHOT from a REAL-COMBAT PIE (units actually taking damage — NOT economy-only) shows, and an agent VISUALLY CONFIRMS in the pixels: the fill VISIBLY LOWER after damage AND correctly team-tinted (blue friendly / red enemy), on a unit + the hero + a tower, BOTH teams. NO "machine checks pass, ship it."

**Dispatch shape — WHAT STARTS NOW (parallel):** **TASK-129 (art, strip diag — editor) ∥ TASK-130 (programmer, rebuild C++ — files)** — file/resource-disjoint. Then Directive 1 finalizes: **TASK-126** (build, commit WBP_DeckCardTile.uasset ONLY). Directive 2 serial: TASK-130 → qa-reviewer (shadow + complete-type-include scans; verify EVERY HP-mutation path broadcasts) → **TASK-132 phase-A** compile (retire old assets) → **TASK-131** (art, WBP_CombatantHealthBar from the WORKING castle widget) → **TASK-132 phase-B** (real-combat PIE + the SCREENSHOT visual gate + churn revert + commit). Single editor + git serialize the two build tasks (TASK-126 first — small + approved).

#### TASK-129 — Strip [TASK125DIAG] from WBP_DeckCardTile RefreshCell (editor)
- assignee: art-director
- status: done (commit 274c160 via TASK-126, 2026-07-10; not pushed). PRIOR: ready-for-integration (art-director 2026-07-10; handoff `handoffs/TASK-129.md`). [TASK125DIAG] PrintString REMOVED from WBP_DeckCardTile RefreshCell; compiled clean, SAVED (is_dirty=false). Readback confirms production path INTACT: CopyCountText←GetCountOf (plain count), NameText←GetCardDisplayName, CostText←GetCardCost, SetIsEnabled(AddBtn, GetCountOf<GetCardMaxCopies) (cap-grey) — no PrintString remains. Only WBP_DeckCardTile.uasset touched; NO C++, NO Git, no health-bar assets touched, editor not terminated. build-master (TASK-126) can now commit the clean tile.
- blocked-by: none (TASK-125 APPROVED by Jonathan 2026-07-10 — human WATCH satisfied)
- parallel-safe: yes (editor-only on WBP_DeckCardTile; file/resource-disjoint from TASK-130's C++ files — the two run alongside each other)
- spec: >
    Editor/MCP only — needs the editor MCP up (else park + tell the orchestrator). The `[TASK125DIAG]` node the
    tile carried to PROVE the count updates at runtime (TASK-125) is a BLUEPRINT node (Print String / log) inside
    `WBP_DeckCardTile`'s `RefreshCell` — REMOVE it. It is a BP node, NOT C++, so NO compile is needed. Change
    NOTHING else: the card-face art, the above-card copy count, the +/− wiring, and the
    `GetCountOf`/`GetCardArtTexture`/`GetCardDisplayName`/`GetCardCost` calls all stay intact. ACCEPTANCE: no
    `[TASK125DIAG]` node remains in WBP_DeckCardTile; the tile still renders art + count + name + cost. Art skips
    QA → build-master commit (TASK-126). Post the handoff in 🎨 Art (`🎨 ART-DIRECTOR: … TASK-129`) with a
    readback confirming the diag node is gone.
- names: >
    `/Game/UI/WBP_DeckCardTile` — remove the `[TASK125DIAG]` Print String / log node from `RefreshCell`. No other
    change. No compile (BP node). Commit rides TASK-126 (scoped to WBP_DeckCardTile.uasset only).

#### TASK-130 — Health-bar REBUILD (castle-parity push/delegate): retire poll, new delegate + component + widget base on units/hero/buildings (C++ files)
- assignee: gameplay-programmer
- status: done (commit 61a1e72 via TASK-132 rebuild integration, 2026-07-10 — the SHIPPED overhead-bar fix; not pushed). PRIOR: **ready-for-qa (RENDER-SIDE follow-up)** — added 2026-07-10, `CombatantHealthBarComponent.h/.cpp` ONLY. The C++/data layer already qa-passed and runtime logs prove `OnHPChanged`/`SetTeamColor` receive correct DROPPING values on the real `Bar`, yet pixels stay frozen → the failure is the LAST hop (the widget-component doesn't repaint). **Hosting diff CONFIRMED:** castle & unit bars use the IDENTICAL Screen-space `UWidgetComponent` + delegate + LIVE Slate (`TakeWidget`, `WidgetComponent.cpp:87`); there is NO castle HUD (the top-center castle bar IS the overhead component on the distant enemy castle). So `RequestRedraw` is World-space-only (engine-verified) and by static analysis mine SHOULD repaint like the castle — I could NOT find the config difference; it's runtime-only. **Fix covers the component-side modes + instruments the rest:** the COMPONENT now also binds the delegate → drives the CURRENT `GetWidget()` (identity-proof) + `RequestRedraw()` (World-space) + `SetTickMode(Enabled)` (screen-layer) + `[TASK130DIAG]` probes logging `space`/`same`(identity)/`tick`. If the log shows `space=Screen`+`same=1`+`tick=1` and pixels STILL frozen → widget-asset Slate invalidation → bounces to art-director (TASK-131), not the component. **NEEDS RECOMPILE + PIE RE-VERIFY** (NOT machine-closable — Jonathan must SEE the drop; OVERNIGHT-AUTH §3). Handoff: `handoffs/TASK-130.md` "render-side fix". — PRIOR: **qa-passed (C++/data layer)** — orchestrator-proxied 2026-07-10 (qa-reviewer has no partial-edit tool). Broadcast completeness VERIFIED 14/14 (QA grepped every `CurrentHP =` across all of Siegebound, matched each to `OnHPChanged.Broadcast`) + denominator/Max paths covered (hero effective-max via PlateArmor both broadcast); damage broadcasts fire BEFORE death handling on all 3 (castle parity). Zero dangling refs to the 5 deleted types (files gone from disk; SiegeCheatManager was the only external consumer, swapped). Team map correct — RED=enemy / BLUE=friendly, not inverted; null TeamAgent → Blue. Seed-then-bind + AddUniqueDynamic + UFUNCTION HandleHPChanged sound; hero hide/show correct on death/respawn/KillZ; scans CLEAN; castle/GoldNode untouched. **DATA-LAYER PASS ONLY — does NOT prove rendered pixels.** The five prior attempts all passed data-layer review while pixels stayed frozen; the TRUE-OVERRIDE WBP (TASK-131) + real-combat GDI screenshot gate (TASK-132) remain the actual proof of fix. 0 BLOCKER / 0 WARN / 3 NIT. **FLAG for manager (non-blocking): miners now SHOW a bar** — the new component doesn't carry the old per-BP `bShowHealthBar=false` miner opt-out (spec-accepted "no per-BP rewiring" trade-off; arguably more consistent with "all unit health bars"). Report: `qa/TASK-130-report.md`. (Prior: ready-for-qa —) REBUILT on the castle PUSH/delegate model 2026-07-10. Retired the 5 poll-system files; added `FOnCombatantHPChanged`/`IHealthBarProvider` (HealthBarProvider.h), `UCombatantHealthBarWidget` (seed-then-bind, float BIEs), `UCombatantHealthBarComponent` (in-ctor HPBarWidget, NO poll, SetTeamColor RED enemy/BLUE friendly). `OnHPChanged` broadcasts on ALL 14 HP-mutation sites (SummonedUnit ×4, Building ×3, Hero ×7 — grep-verified 1:1); hero hides/shows the bar on death/respawn (castle parity). Swapped SiegeCheatManager's `IHealthBarTarget`→`IHealthBarProvider` (only external consumer; no dangling refs). Shadow + complete-type-include scans CLEAN. Handoff: `handoffs/TASK-130.md` (delegate sig, every broadcast site, TASK-131 art spec). Do NOT compile/Git (build-master TASK-132). (Prior: backlog.)
- blocked-by: none (files only — starts NOW)
- parallel-safe: yes (C++ files; disjoint from TASK-129's editor work on WBP_DeckCardTile — the two run in parallel)
- spec: >
    Files only — NO editor/MCP. REBUILD the overhead unit/hero/building health bar FROM SCRATCH on the WORKING
    CASTLE's push/delegate model (`ACastle` + `UCastleHealthBarWidget` + `FOnCastleHPChanged` — read them as the
    template), per CONVENTIONS "Overhead combatant health bars — REBUILT (2026-07-10)". Jonathan: the bars still
    do not visibly drop after 5 attempts — rebuild it.
    (1) RETIRE the failed POLL system: DELETE `HealthBarTarget.h` (IHealthBarTarget), `HealthBarComponent.h/.cpp`
    (UHealthBarComponent), `UnitHealthBarWidget.h/.cpp` (UUnitHealthBarWidget), and remove the old in-constructor
    `HPBarWidget` add from ASummonedUnit / ABuilding / AHeroCharacter. (Deleting the `WBP_UnitHealthBar` ASSET is
    TASK-132.)
    (2) NEW delegate `FOnCombatantHPChanged(float CurrentHP, float MaxHP)` (mirror FOnCastleHPChanged) — a
    `UPROPERTY(BlueprintAssignable)` member named `OnHPChanged` on ASummonedUnit, ABuilding, and AHeroCharacter,
    BROADCAST on EVERY HP mutation (TakeDamage, ApplyHealing / regen, reset / respawn) — MISS NONE, or the bar goes
    stale (qa/TASK-005 major-2 seed-then-bind trap). Broadcast on reset too (like ACastle).
    (3) NEW provider interface (replaces IHealthBarTarget) so ONE widget/component binds across the 3 unrelated
    classes: `IHealthBarProvider` (`UHealthBarProvider`, HealthBarProvider.h) — `FOnCombatantHPChanged&
    GetHPChangedDelegate()`, `float GetHealthCurrent() const`, `float GetHealthMax() const`, `bool
    IsHealthBarActorAlive() const`; team via the EXISTING ITeamAgent (do NOT duplicate team).
    (4) NEW widget base `UCombatantHealthBarWidget` (CombatantHealthBarWidget.h/.cpp) — MIRROR
    UCastleHealthBarWidget EXACTLY: `InitForCombatant(TScriptInterface<IHealthBarProvider> Provider)` that SEEDS
    `OnHPChanged` immediately from the current HP THEN binds the delegate (seed-then-bind); float-only BIEs
    `OnHPChanged(float,float)` and `SetTeamColor(float,float,float)`; an internal `UFUNCTION` handler bound to the
    delegate (the `HandleCastleHPChanged` shape).
    (5) NEW widget component `UCombatantHealthBarComponent` (CombatantHealthBarComponent.h/.cpp, UWidgetComponent
    subclass), added in-constructor as `HPBarWidget` on the 3 base classes (subclasses inherit): BeginPlay sets
    WidgetClass to `/Game/UI/WBP_CombatantHealthBar` (soft, null-safe — missing = silent no bar, log once), Screen
    space, DrawSize ~90×12, relative Z = BarHeightZ (default 120), reads its owner as IHealthBarProvider +
    ITeamAgent, calls `InitForCombatant` (seed-then-bind), pushes `SetTeamColor` ONCE from
    BlueBarColor(0.05,0.30,1.00)/RedBarColor(1.00,0.10,0.05); ALWAYS VISIBLE while alive (NO hide-at-full — the
    reversed law), HIDDEN on death/destruction; per-BP `bShowHealthBar` opt-out kept (EditDefaultsOnly, default
    true). NO poll timer anywhere.
    Everywhere null-safe; ZERO combat/stat behavior change; castle/GoldNode untouched. LAW: the widget BIEs must be
    TRUE overrides (a K2Node_CustomEvent never fires from C++) — that is TASK-131's concern but write the C++ so a
    correctly-overridden WBP works. ACCEPTANCE: compiles warnings-as-errors; the 4 poll-system source files are
    gone; the delegate broadcasts on ALL HP paths on all 3 classes; the 3 bases own one HPBarWidget (new
    component); no dangling refs to the deleted classes; ACastle/AGoldNode not touched. → qa-reviewer (MANDATORY
    shadow scan + complete-type-include scan; VERIFY every HP-mutation path on all 3 classes broadcasts OnHPChanged
    — the stale-bar trap; VERIFY no lingering include/reference to the deleted classes). Post in ⚙️ Dev & QA
    (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-130`).
- names: >
    RETIRE (delete): IHealthBarTarget/HealthBarTarget.h, UHealthBarComponent/HealthBarComponent.h+.cpp,
    UUnitHealthBarWidget/UnitHealthBarWidget.h+.cpp; asset /Game/UI/WBP_UnitHealthBar (asset delete = TASK-132).
    NEW (Source/GitClaudeUnrealTest/Siegebound/): delegate `FOnCombatantHPChanged(float CurrentHP, float MaxHP)`;
    `UPROPERTY(BlueprintAssignable) OnHPChanged` on ASummonedUnit/ABuilding/AHeroCharacter (broadcast every HP
    change + reset); interface `IHealthBarProvider`/`UHealthBarProvider` (HealthBarProvider.h) —
    GetHPChangedDelegate()/GetHealthCurrent()/GetHealthMax()/IsHealthBarActorAlive(); widget base
    `UCombatantHealthBarWidget` (CombatantHealthBarWidget.h/.cpp) — InitForCombatant(TScriptInterface<IHealthBarProvider>)
    + BIEs OnHPChanged(float,float)/SetTeamColor(float,float,float); component `UCombatantHealthBarComponent`
    (CombatantHealthBarComponent.h/.cpp), instance `HPBarWidget`, props HealthBarWidgetClass
    (=/Game/UI/WBP_CombatantHealthBar), BarHeightZ(120), bShowHealthBar(true), BlueBarColor(0.05,0.30,1.00)/
    RedBarColor(1.00,0.10,0.05). Mirror ACastle / UCastleHealthBarWidget / FOnCastleHPChanged. Reuse:
    ITeamAgent::GetTeamId. Do NOT touch ACastle / AGoldNode. UMG asset (TASK-131): /Game/UI/WBP_CombatantHealthBar.

#### TASK-131 — WBP_CombatantHealthBar from the WORKING castle widget: duplicate, reparent, preserve castle fill brush, team tint (editor)
- assignee: art-director
- status: done (commit 61a1e72 via TASK-132, 2026-07-10; not pushed). PRIOR: ready-for-integration (art-director 2026-07-10; handoff `handoffs/TASK-131.md`). `/Game/UI/WBP_CombatantHealthBar` DUPLICATED from the working WBP_CastleHealthBar, REPARENTED to UCombatantHealthBarWidget (get_parent readback=/Script/GitClaudeUnrealTest.CombatantHealthBarWidget). Compiled clean, SAVED (is_dirty=false). BOTH BIEs verified TRUE OVERRIDES via get_node_infos (class K2Node_Event, NOT K2Node_CustomEvent): OnHPChanged (AddEvent|Siegebound|UI|EventOnHPChanged, survived reparent) → SetPercent(GetBar, Cur/Max) guard Max>0; SetTeamColor (AddEvent|Siegebound|UI|EventSetTeamColor, added via add_event) → SetFillColorAndOpacity(GetBar, MakeLinearColor(R,G,B,1)). FILL BRUSH: preserved the castle's EXACT setup = /Engine/EngineMaterials/DefaultWhiteGrid_Low MATERIAL (per board 'do not pre-emptively diverge'; BarFillStyle=Scale so fill scales w/ Percent; tint WHITE so pushed team color shows undimmed). FLAG: coordinator #2 assumed castle=plain-image but castle=this material — it's a RED HERRING (castle drains w/ identical material; real 5x defect was the C++ screen-space registration, fixed by TASK-130). Plain-image swap = 1-edit, available on request / phase-B fallback per board. TRACK = grey (0.03,0.03,0.03)@0.7. Bar HitTestInvisible, no self-hide, no baked text. DESIGNER SHRINK TEST at Percent=0.35 NOT visually runnable headless (CaptureAssetImage unsupported for WBPs; locked desktop=black GDI) — set 0.35 (readback 0.35) then reset 1.0; structural proof = BarFillStyle=Scale + byte-identical to the draining castle fill; visual shrink + red/blue combat proof OWED to TASK-132 phase-B GDI screenshot. NO Git, NO C++, WBP_CastleHealthBar/WBP_MainMenu untouched, editor not terminated.
- blocked-by: TASK-130 (reparents to UCombatantHealthBarWidget — needs it compiled) + TASK-132 phase-A compile
- parallel-safe: no (editor-mutating — single editor instance)
- spec: >
    Editor/MCP only — needs the editor MCP up (else park + tell the orchestrator). Author
    `/Game/UI/WBP_CombatantHealthBar` per CONVENTIONS "Overhead combatant health bars — REBUILT (2026-07-10)".
    (1) DUPLICATE the WORKING `/Game/UI/WBP_CastleHealthBar` (the ONE bar that demonstrably renders + updates) —
    NOT the retired WBP_UnitHealthBar (tainted across 5 failures). (2) REPARENT the duplicate to
    `UCombatantHealthBarWidget` (TASK-130); READBACK-confirm the parent took (a silent reparent failure = a dead
    bar — the TASK-111 crux). (3) Implement the float BIEs as TRUE OVERRIDES — after authoring, READBACK-verify
    each is a real override (`bOverrideFunction=true`), NOT a `K2Node_CustomEvent` (a custom event is
    DSL-indistinguishable and NEVER fires from C++ — the defect class that hid the bug across 5 attempts):
    `OnHPChanged(float CurrentHP, float MaxHP)` → ProgressBar `SetPercent(CurrentHP/MaxHP)` guard Max>0;
    `SetTeamColor(float R,float G,float B)` → fill tint. (4) PRESERVE the castle bar's EXACT fill-brush setup —
    diff against WBP_CastleHealthBar and do NOT diverge (the rebuild premise: the old WBP_UnitHealthBar diverged
    from the castle somewhere). The unfilled TRACK = neutral GREY (~0.03,0.03,0.03 @ ~0.7α), contrasting both blue
    and red fills; fill `tintColor` neutral so the C++-pushed team color shows undimmed. (5) Compact (90×12),
    HitTestInvisible, no baked text, no self-hide logic (the component owns show/hide). Do NOT touch
    WBP_CastleHealthBar. **If TASK-132's screenshot still shows a frozen fill, the FALLBACK is to switch the fill
    to a PLAIN IMAGE brush (the DefaultWhiteGrid_Low material is the prime suspect) — but do not pre-emptively
    diverge from the working castle.** Art skips QA → build-master integration + the SCREENSHOT gate (TASK-132
    phase-B). Post the handoff in 🎨 Art (`🎨 ART-DIRECTOR: … TASK-131`) with the parent-class readback + an
    explicit statement that each BIE is a TRUE override (node class read back) and the fill-brush setup matches
    the castle.
- names: >
    `/Game/UI/WBP_CombatantHealthBar`, parent `UCombatantHealthBarWidget`. Donor: WORKING /Game/UI/WBP_CastleHealthBar.
    BIEs (TRUE overrides, node class verified): OnHPChanged(float,float)→SetPercent(guard Max>0);
    SetTeamColor(float,float,float)→fill tint. Fill brush = the castle's exact setup (fallback: plain image, NOT
    the DefaultWhiteGrid_Low material); track = grey ~(0.03,0.03,0.03)@~0.7α. Do NOT touch WBP_CastleHealthBar.

#### TASK-132 — Health-bar rebuild integration: compile, delete old assets, REAL-COMBAT PIE + GDI-screenshot visual gate, churn revert, commit (build)
- assignee: build-master
- status: done (commit 61a1e72, 2026-07-10 — compiled the rebuild, deleted the retired poll-system assets, ran the GDI-screenshot visual gate, committed on `main`; NOT pushed). PRIOR: backlog.
- blocked-by: TASK-130 (qa-passed) for phase-A; TASK-131 (ready-for-integration) for phase-B
- parallel-safe: no
- spec: >
    Build-master integration for the health-bar REBUILD. TWO PHASES, single owner.
    (PHASE A — after TASK-130 qa-passed) compile TASK-130 via the standard Build.bat (editor-bounce) so
    `UCombatantHealthBarWidget` / `UCombatantHealthBarComponent` exist and the retired classes are gone — this
    UNBLOCKS TASK-131. Pre-compile: shadow + complete-type-include scans. DELETE the retired asset
    `/Game/UI/WBP_UnitHealthBar` (its C++ base is gone → it would orphan). Any compile error → append to TASK-130's
    QA report, qa-failed, stop (counts as a QA loop; build-master never edits code).
    (PHASE B — after TASK-131) re-verify clean compile, then the MANDATORY VISUAL GATE on the now-UNLOCKED desktop
    (the capability missing for all 5 failed attempts): run a REAL-COMBAT PIE in L_Arena where units ACTUALLY take
    damage — drive it with the TASK-121 cheats (`SummonTestUnit` + `ApplyTestDamage`) and/or a real Play-vs-Bot
    with combat; NOT an economy-only run that never damages anything. Take a REAL GDI SCREENSHOT at full HP and
    again AFTER damage, and VISUALLY INSPECT THE PIXELS (Read the PNG). HARD EXIT CRITERION — declare the feature
    fixed ONLY when the screenshots show, and you CONFIRM in the pixels: the fill VISIBLY LOWER after damage AND
    correctly team-tinted — BLUE friendly, RED enemy — on a unit + the hero + a tower, BOTH teams; bar hidden on
    death; castle still shows only its own bar; gold nodes none. Report the OBSERVATION (what the pixels show),
    not the conclusion. NO "machine checks pass, ship it" — a green machine check without a confirming screenshot
    is NOT a pass. If the screenshot still shows a frozen/untinted bar → append to TASK-130's QA report + route
    back to gameplay-programmer (counts as a QA loop; note the plain-image-brush fallback for TASK-131). Also
    REVERT the two churn .uassets (WBP_CastleHealthBar, WBP_MainMenu — close-resave residue) in this bounce. On
    PASS: commit the rebuild (new Source/ + WBP_CombatantHealthBar + the deletions) with the task-ID message and
    reference the confirming screenshot in the handoff. Do NOT push, no new branch. Post compile + the SCREENSHOT
    OBSERVATION + commit hash in 🔧 Build & Git (`🔧 BUILD-MASTER: … TASK-132`).
- names: >
    Build per CLAUDE.md. Delete /Game/UI/WBP_UnitHealthBar. Revert WBP_CastleHealthBar + WBP_MainMenu churn.
    Commit on `main`, message "TASK-130..132: health-bar rebuild (castle push/delegate parity) —
    WBP_CombatantHealthBar, per-actor FOnCombatantHPChanged, red-enemy/blue-friendly, screenshot-verified; retired
    the poll system". No push, no branch.

---

### M7 pull-forward — Ogre textured mesh (TASK-147..152) — issued 2026-07-14 — ✅ COMPLETE 2026-07-15 (integrated + bundled-committed to main with the M6.6 scatter tune; NOT pushed — see handoffs/TASK-152.md)

**Authorization:** Jonathan's 2026-07-14 directive — he dropped an ogre concept image and wants the TRELLIS.2 art pipeline run to replace the `SM_Ogre` blockout with a game-ready textured mesh, pulling ONE of the 16 M7 blockouts forward. NOT a new milestone (M7 stays deferred) — a single-asset pull-forward run on the validated pipeline. Naming law: the Ogre follows CONVENTIONS "Textured mesh law (TRELLIS.2 art pipeline)" verbatim (updated 2026-07-14 to list Ogre as an active pipeline asset — NO new pattern needed). Hard gates stand: HF_TOKEN is ENV-ONLY (never echoed/logged/argv); heavy Blender runs HEADLESS; editor/MCP work needs 127.0.0.1:8000 up (park + tell the orchestrator if unreachable — never fake results). Repo base `057ca9f` on `main` (pushed). Does NOT touch any prior milestone's state.

**Scope pins (recon, do not re-investigate):**
- **Replace target** `/Game/Meshes/SM_Ogre` — Stage 3 OVERWRITES at this SAME path (never delete+recreate) so soft refs survive: `Content/Blueprints/Units/BP_Unit_Ogre` and the Ogre row in `Docs/Data/cards.csv:14` (Ogre, Unit/Siege, cost 12) both resolve to it.
- **Concept** `Tools/ArtPipeline/Inbox/ogre.png` arrived LOWERCASE — AssetName is PascalCase `Ogre` (matches `SM_Ogre`); TASK-147 renames it so Stage-1 `trellis_generate.py Ogre` and all downstream names are consistent.
- **Blockout** `Content/RawAssets/Ogre.fbx` — its bounds are the `target_dims_ue` source; MEASURE it in TASK-147 BEFORE Stage 2 (TASK-149) overwrites it.
- **Manifest gap** — `pipeline_manifest.json` has NO "Ogre" entry (only Footman/Archer/Castle); TASK-147 authors the UNIT-path entry.
- **NOT in scope:** the 2D card art `T_CardArt_Ogre` / `Content/RawAssets/CardArt/Ogre.png` (separate card-art lane — this pipeline never writes CardArt). Only the 3D mesh is replaced.

**Two Jonathan touchpoints:** (1) HF generation quota/token at Stage 1 (TASK-148 — surface exit codes verbatim, escalate 🚨 Blockers, never fake); (2) the EYEBALL GATE (TASK-150) between Stage 2 and Stage 3 — orientation + team-region are unknown for a new asset and need his eye before import.

Dispatch shape: **strictly SERIAL single-asset chain** (one art-director, one asset, one editor). **TASK-147** (prep: casing + manifest) → **TASK-148** (Stage 1 generate) → **TASK-149** (Stage 2 refine) → **TASK-150** (Jonathan eyeball gate; TUNE loops back to 149, no HF cost) → **TASK-151** (Stage 3 import, editor) → **TASK-152** (build-master integration + commit). Only TASK-147 can start immediately.

#### TASK-147 — Ogre pipeline prep: concept casing reconcile + manifest entry (measure blockout) (art)
- assignee: art-director
- status: done
- blocked-by: none
- parallel-safe: yes (file-only — renames Inbox/ogre.png, edits pipeline_manifest.json, reads Content/RawAssets/Ogre.fbx; DISJOINT from all other work)
- spec: >
    File-side prep — NO editor/MCP, NO HF quota. Get the Ogre pipeline inputs consistent BEFORE Stage 1/2.
    (1) CASING RECONCILE: rename `Tools/ArtPipeline/Inbox/ogre.png` → `Tools/ArtPipeline/Inbox/Ogre.png`
    (PascalCase AssetName = Ogre, matching SM_Ogre + the cards.csv row). `trellis_generate.py Ogre` reads the
    exact-cased file and every downstream name derives from `Ogre`, so this MUST happen before Stage 1. Do NOT
    alter the image content.
    (2) MEASURE THE BLOCKOUT (BEFORE Stage 2 overwrites it): read the bounds of the EXISTING
    `Content/RawAssets/Ogre.fbx` blockout (Blender headless or MCP <30 s inspection) — X/Y/Z extents in UE
    units, feet-center convention. These are the `target_dims_ue` source.
    (3) AUTHOR THE MANIFEST ENTRY: add an `"Ogre"` object under `assets` in
    `Tools/ArtPipeline/pipeline_manifest.json`, modeled on the existing "Footman"/"Archer" UNIT entries:
    `category:"unit"`, `mode:"bake"`, `tri_budget:15000`, `bake_resolution:1024`, `origin:"feet-center"`,
    `fit_mode:"height"`, `target_dims_ue:[X,Y,Z]` from the measured blockout + a `_dims_source` note
    ("TASK-147 blockout: <X> x <Y> x <Z>, feet-center, front -Y"), `pre_rotate_z_deg:0.0` (STARTING GUESS —
    tuned at the TASK-150 eyeball gate), `voxel_size_ue:1.5`, `team_region` with `max_fraction:0.35` and a
    STARTING-GUESS upward-facing shoulder / upper-body selector modeled on the Footman recipe (the minority
    slot-0 `TeamRegion` face-set; tuned at the eyeball gate), and `ucx:null` (units generate ≤4 simple hulls
    at Stage-3 import — NOT authored here). Keep VALID JSON (the manifest is CODE — rides the QA/commit gate at
    TASK-152). Do NOT touch the Footman/Archer/Castle entries or `defaults`.
    ACCEPTANCE: `Inbox/Ogre.png` exists (lowercase gone); `pipeline_manifest.json` parses and has a complete
    `Ogre` UNIT entry with a MEASURED `target_dims_ue`; no other asset entries changed. Post in 🎨 Art
    (`🎨 ART-DIRECTOR: 🟦/✅ TASK-147 …`).
- names: >
    Rename `Tools/ArtPipeline/Inbox/ogre.png` → `Tools/ArtPipeline/Inbox/Ogre.png`. AssetName = `Ogre`.
    Edit `Tools/ArtPipeline/pipeline_manifest.json` → add `assets.Ogre` (unit path per above). Read-only
    measure `Content/RawAssets/Ogre.fbx`. Law: CONVENTIONS "Textured mesh law (TRELLIS.2 art pipeline)".

#### TASK-148 — Ogre Stage 1: generate (trellis_generate.py Ogre) (art)
- assignee: art-director
- status: done
- blocked-by: TASK-147 (needs Inbox/Ogre.png renamed)
- parallel-safe: yes (Bash only — writes Cache/Ogre/*; SERIAL in the pipeline)
- spec: >
    Bash — NO editor/MCP. Stage 1 of the TRELLIS.2 pipeline on the Ogre (README + the TASK-086 playbook
    handoffs/TASK-086.md). From `Tools/ArtPipeline`:
    (1) HEALTH PROBE FIRST: `uv run trellis_generate.py --check` (tokenless — surfaces HF_TOKEN/env/TLS/Space
    issues with NO GPU/quota cost). HF_TOKEN is already set + HF PRO active (TASK-085); Norton HF exclusions
    proven → run bare (no SSL_CERT_FILE). `--check` exit 4 (API drift) → file `api_schema.json`, escalate
    🚨 Blockers, use the README manual-browser fallback (resume Stage 2 from a hand-delivered GLB); never fake.
    (2) GENERATE: `uv run trellis_generate.py Ogre` → writes `Cache/Ogre/trellis_raw.glb` + `state.json` +
    `api_schema.json`. The whole preprocess→generate→extract runs atomically in ONE session — never split it.
    Exit codes surfaced VERBATIM, never faked: 0 ok · 2 HF_TOKEN unset (→ Jonathan, 🚨 Blockers) · 3 quota
    exhausted (EXPECTED pause — record the reset time, resume next window; HF PRO ≈ 40 GPU-min/day) · 4 API
    drift (file schema, manual fallback) · 5 concept image missing (check the TASK-147 rename).
    (3) EYEBALL the raw GLB (quick MCP inspection, <30 s calls): if the mesh is mangled/wrong, reroll with
    `--seed <n>` (quota permitting) BEFORE Stage 2. Record the seed/params in the handoff.
    ACCEPTANCE: `Cache/Ogre/trellis_raw.glb` + `state.json` exist and the raw mesh reads as a plausible ogre
    (not mangled). Report seed + generation time + any reroll in the handoff. Post in 🎨 Art (flag any
    non-zero exit + the reset time in 🚨 Blockers).
- names: >
    `uv run trellis_generate.py --check` then `uv run trellis_generate.py Ogre` (from Tools/ArtPipeline).
    Outputs: `Cache/Ogre/{trellis_raw.glb, state.json, api_schema.json}`. HF_TOKEN ENV-ONLY (never
    echoed/logged/argv). Law: CONVENTIONS "Textured mesh law", README Stage 1.

#### TASK-149 — Ogre Stage 2: refine (refine_trellis_glb.py --asset Ogre) (art)
- assignee: art-director
- status: done
- blocked-by: TASK-148 (needs Cache/Ogre/trellis_raw.glb), TASK-147 (needs the Ogre manifest entry)
- parallel-safe: yes (headless Blender via Bash — writes Content/RawAssets/Ogre.fbx + Textures/Ogre/*; SERIAL in the pipeline)
- spec: >
    Bash headless Blender — NO editor/MCP. Stage 2 refine on the Ogre per its manifest entry (UNIT path).
    Run `& "C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --python
    refine_trellis_glb.py -- --asset Ogre` from `Tools/ArtPipeline` (heavy Blender ALWAYS headless — the live
    MCP bridge has a 30 s socket cap). The script does cleanup → remesh/decimate to ≤15k tris → Smart-UV
    (`UVMap`) → Cycles CPU bake D/N/ORM (1024²) → two-slot split (slot 0 `TeamRegion` / slot 1 `OgrePBR`) →
    FBX + texture PNGs + previews + `refine_report.json`.
    OUTPUTS (OVERWRITE the blockout in place — why TASK-147 measured it first):
    `Content/RawAssets/Ogre.fbx` + `Content/RawAssets/Textures/Ogre/*.png`.
    PRE-GATE READ: read `Cache/Ogre/refine_report.json` + eyeball the Cache previews — nothing proceeds unseen.
    Copy the accepted concept `Tools/ArtPipeline/Inbox/Ogre.png` → `Content/RawAssets/Concepts/Ogre.png`
    (committed at TASK-152).
    NOTE — NEW asset: `pre_rotate_z_deg` + the `team_region` selectors are STARTING GUESSES. Do NOT
    tune-and-loop blindly here — produce the refine + previews and hand to the TASK-150 EYEBALL GATE. If the
    previews are OBVIOUSLY wrong (facing backwards, team-region striping the wrong faces), record the finding
    FOR the gate — the manifest tune + Stage-2 re-run happens as the gate's OUTCOME, not silently.
    ACCEPTANCE: `Ogre.fbx` exists with EXACTLY two slots ordered [TeamRegion, OgrePBR], ≤15k tris, UVMap,
    feet-center (minZ≈0); `refine_report.json` written; D/N/ORM PNGs present. Report tris/bounds vs manifest +
    the selector-area % in the handoff. Post in 🎨 Art.
- names: >
    `refine_trellis_glb.py --asset Ogre` (headless Blender). Outputs `Content/RawAssets/Ogre.fbx` (slots
    [TeamRegion, OgrePBR]), `Content/RawAssets/Textures/Ogre/*.png` (D/N/ORM), `Cache/Ogre/refine_report.json`
    + previews. Concept → `Content/RawAssets/Concepts/Ogre.png`. Law: CONVENTIONS "Textured mesh law", README
    Stage 2.

#### TASK-150 — EYEBALL GATE: Ogre orientation + team-region sign-off (Jonathan — external gate)
- assignee: Jonathan (external gate — board-recorded; the orchestrator posts the ask in 🚨 Blockers with the Cache/Ogre previews + refine_report, and flips this when satisfied)
- status: done (Jonathan accepted the eyeball gate as-is 2026-07-14 — dark texture approved, no reroll/re-tune; unblocked Stage-3 import TASK-151 — see handoffs/TASK-151.md)
- blocked-by: TASK-149
- parallel-safe: yes (human review — no repo mutation by agents)
- spec: >
    Jonathan checkpoint BETWEEN Stage 2 and Stage 3 (exactly the TASK-086/087 eyeball-gate pattern, made
    EXPLICIT because the Ogre is a NEW asset whose TRELLIS output orientation + team-region face-set are
    UNKNOWN until first generation — `pre_rotate_z_deg` and the `team_region` selectors in the manifest are
    STARTING GUESSES). Jonathan reviews the Stage-2 previews (`Cache/Ogre/*preview*` + `refine_report.json`)
    and confirms: (a) FACING — the ogre's front faces the blockout contract (Blender -Y); (b) TEAM REGION —
    the slot-0 `TeamRegion` face-set is a sensible minority accent (shoulders / upper-body trim), NOT striping
    the whole body or a bare face. OUTCOME:
      - APPROVE → unblocks Stage 3 import (TASK-151).
      - TUNE → adjust the manifest (`pre_rotate_z_deg` and/or `team_region.selectors`, add a `_tuned` note like
        the Footman/Archer entries) and RE-RUN Stage 2 (TASK-149 loops), then re-review. Loop until APPROVE —
        each Stage-2 re-run is HEADLESS and costs NO HF quota (only Stage 1 costs quota).
    Gate is satisfied when Jonathan (or the orchestrator on his verbatim go) records APPROVE here + in
    🚨 Blockers. NEVER import an un-eyeballed NEW-asset generation.
- names: >
    Review `Cache/Ogre/` previews + `refine_report.json`. Tune targets (if needed):
    `pipeline_manifest.json` → `assets.Ogre.pre_rotate_z_deg` / `.team_region.selectors` → re-run TASK-149.
    Law: CONVENTIONS "Textured mesh law" (pre_rotate_z_deg / team_region = per-asset eyeball-tuned guesses).

#### TASK-151 — Ogre Stage 3: import — overwrite SM_Ogre + T_Ogre_* + MI_Ogre_PBR (art, Unreal MCP)
- assignee: art-director
- status: done
- blocked-by: TASK-149 (needs Ogre.fbx + textures), TASK-150 (eyeball gate APPROVED — never import an un-eyeballed new asset)
- parallel-safe: no (editor-mutating — single editor, serialize)
- spec: >
    Unreal MCP editor import (serialized) — editor UP with MCP at 127.0.0.1:8000 (if unreachable, park + tell
    the orchestrator, never fake). Use handoffs/TASK-086.md as the import playbook. `M_AssetPBR` ALREADY EXISTS
    (TASK-086, committed cb29882) — do NOT re-author the master.
    (a) Import textures → `/Game/Textures/T_Ogre_D` (sRGB ON), `T_Ogre_N` (normal), `T_Ogre_ORM` (LINEAR —
    sRGB OFF; the ORM needs the manual sRGB→false flip, per the TASK-086/087 note).
    (b) Create `/Game/Materials/Instances/MI_Ogre_PBR` from `/Game/Materials/M_AssetPBR`; wire params
    BaseColor→T_Ogre_D, Normal→T_Ogre_N, ORM→T_Ogre_ORM.
    (c) Import `Content/RawAssets/Ogre.fbx` OVERWRITING `/Game/Meshes/SM_Ogre` at the SAME PATH (NEVER
    delete+recreate — the soft refs from BP_Unit_Ogre + the cards.csv Ogre row + the placement-ghost
    `/Game/Meshes/SM_Ogre` string contract MUST survive). KNOWN MECHANISM (TASK-086/088): MCP import_file
    REFUSES a same-path overwrite and no console `Obj Reimport` surfaced — the validated route is a human
    Content-Browser Reimport click (Stage-2's same-path FBX overwrite makes reimport-in-place resolve). Do all
    pre-click setup (textures, MI, pre-navigate the Content Browser to /Game/Meshes with SM_Ogre selected) and,
    if no MCP reimport/console-exec route exists, flag the ONE reimport click to Jonathan (🚨 Blockers) — the
    TASK-086 contingency. If an MCP reimport tool has since landed, use it.
    (d) Slots EXACTLY ordered [0] `TeamRegion` → `MI_TeamColor_Blue` (design-time placeholder; the BeginPlay
    team recolor drives slot 0), [1] `OgrePBR` → `MI_Ogre_PBR`. (e) Nanite OFF. (f) Simple collision ≤4 hulls
    (units generate hulls at import — ucx:null). (g) Verify zero import/MikkTSpace warnings; tris/bounds vs the
    manifest; UVMap present.
    ACCEPTANCE: `SM_Ogre` IS the textured mesh at the UNCHANGED path; slots named/ordered per law with the
    right MIs; T_Ogre_D/_N/_ORM + MI_Ogre_PBR exist; Nanite off; ≤4-hull collision. Report readbacks + the
    overwrite mechanism used in handoffs/TASK-151.md (TASK-152 depends on it). Post in 🎨 Art.
- names: >
    `/Game/Meshes/SM_Ogre` (SAME-PATH overwrite). Textures `/Game/Textures/T_Ogre_D | T_Ogre_N | T_Ogre_ORM`.
    `/Game/Materials/Instances/MI_Ogre_PBR` (from `/Game/Materials/M_AssetPBR`, params BaseColor/Normal/ORM).
    Slots [TeamRegion → MI_TeamColor_Blue, OgrePBR → MI_Ogre_PBR]. FBX `Content/RawAssets/Ogre.fbx`. Reuse
    EXISTING `/Game/Blueprints/Units/BP_Unit_Ogre` + the cards.csv Ogre row (do NOT touch). Law: CONVENTIONS
    "Textured mesh law".

#### TASK-152 — Ogre integration: verify BP_Unit_Ogre resolves + PIE-spawn + commit (build-master)
- assignee: build-master
- status: done
- blocked-by: TASK-151
- parallel-safe: no (single editor + the Git commit)
- spec: >
    Integration + commit for the Ogre mesh swap. Editor UP with MCP (park + tell the orchestrator if down).
    (1) STRUCTURAL on the swapped mesh: SM_Ogre slots == [TeamRegion, OgrePBR] with MI_TeamColor_Blue +
    MI_Ogre_PBR; Nanite false; collision present (≤4 hulls — AggGeom readback per the TASK-088 standard);
    tris/bounds vs the `pipeline_manifest.json` Ogre entry; zero pending import warnings.
    (2) SOFT-REF SURVIVAL: confirm `/Game/Blueprints/Units/BP_Unit_Ogre` still resolves SM_Ogre (VisualMesh)
    and the placement-ghost `/Game/Meshes/SM_Ogre` string still resolves — the same-path overwrite must have
    preserved every reference (the whole point of never delete+recreate).
    (3) PIE on direct-boot L_Arena: spawn the Ogre card (hotkey/placement, or the cheat
    `SummonTestUnit("Ogre", false)` Blue / `("Ogre", true)` Red) and confirm the NEW textured mesh RENDERS
    in-match with blue TeamRegion accents, and the bot's Red Ogre recolors slot 0 ONLY (two-slot contract
    live-proof); the Siege damage profile is unchanged (mesh swap doesn't touch combat — still tags
    USiegeDamageType_Siege). No new log warnings/errors; texture-memory delta sane.
    (4) COMMIT the Ogre art to main (NOT pushed) with TASK-147..152 in the message:
    `Content/RawAssets/Ogre.fbx` (refined, overwriting the blockout), `Content/RawAssets/Textures/Ogre/**`,
    `Content/RawAssets/Concepts/Ogre.png`, `/Game/Meshes/SM_Ogre`, `/Game/Textures/T_Ogre_*`, `MI_Ogre_PBR`,
    and the `pipeline_manifest.json` Ogre entry. `Cache/Ogre/*` is gitignored — do NOT commit it. VERIFY GIT
    STATE FIRST — Jonathan often self-commits; if he has already committed some of these, RECONCILE (commit
    only the residue) rather than duplicating.
    (5) Record the WATCH: Jonathan's visual sign-off (Ogre silhouette at the gameplay camera, style cohesion vs
    the pilot meshes + remaining blockouts, blue/red team read at distance).
    ACCEPTANCE: structural + soft-ref + PIE checks PASS; committed to main (not pushed) OR reconciled with
    Jonathan's self-commit; WATCH posted. Post results + hash in 🔧 Build & Git.
- names: >
    Verify `/Game/Meshes/SM_Ogre` slots + collision + Nanite; `MI_Ogre_PBR`; `BP_Unit_Ogre` +
    `/Game/Meshes/SM_Ogre` ghost resolve; `/Game/Maps/L_Arena` PIE (`SummonTestUnit "Ogre"`). Budget ref:
    `Tools/ArtPipeline/pipeline_manifest.json` (Ogre). Commit to main only, not pushed. Law: CONVENTIONS
    "Textured mesh law".

---

### M6.6 — Climbable terrain (TASK-138..145) — decomposed 2026-07-14 — UNPARKS M4.5

**Authorization:** Jonathan's 2026-07-14 decision to make the battlefield hills/rocks CLIMBABLE by the hero. Root-cause investigation established this is currently BY-DESIGN — M6.5's scatter built every rock/hill/tree as a route-around blocker — and that the parked M4.5 "Gameplay terrain pass" (TASK-091..096) already specified exactly this feature, so M6.6 UNPARKS the M4.5 intent as a fresh milestone (delivered on top of the M6.5 procedural scatter, NOT the retired M4.5 hand-placed approach). A full design pass is approved; the authoritative spec of record is on disk at `C:\Users\wesel\.claude\plans\we-last-left-off-partitioned-puppy.md`. Naming law added to CONVENTIONS "Climbable terrain (M6.6)" BEFORE task issue (TASK-138, done). Hard gate stands: editor/MCP + Blender work needs the tools up (127.0.0.1:8000) — park + tell the orchestrator if unreachable (never fake). Nothing pushed beyond Jonathan's own `6a4c17d`. Does NOT touch prior milestones' state.

**ROOT CAUSE (corrects the earlier jump-height hypothesis):** the scatter applies UNIFORM scale (`BattlefieldScatter.cpp:247`, `FVector(Scale)`), so face angles are SCALE-INVARIANT — scaling the squashed `stone_hill` dome 15× makes it taller AND wider, its flank still goes near-vertical at the rim (a ~4 m unclimbable skirt). No jump buff fixes a dome. FIX = purpose-built CONVEX hill meshes (`SM_Hill_01/02/03`) with ≤30° faces + flat crowns, under BOTH the character's 44.76° `WalkableFloorAngle` AND Recast's 44° `AgentMaxSlope` → hero AND units climb with essentially no movement retune (the hero tuning is comfort/margin only).

**FOUR DECISIONS — LOCKED by Jonathan 2026-07-14 (binding rulings for all M6.6 tasks):**
1. **M6.5 committed FIRST — already done.** Jonathan committed the assembled battlefield HIMSELF as `6a4c17d "battlefield created"` and PUSHED it. This satisfies the held TASK-136/137 commit gate → GATE 0 is satisfied; there is NO build-master M6.5 commit task in M6.6. M6.6 starts on this clean, pushed base. (An `m6.5-testable` branch was never cut at the self-commit — TASK-145 cuts it retroactively at `6a4c17d`.)
2. **Widen arena Y ±2400 → ±4000.** The M6.5 ±2400 field read narrow (14–69 blocking instances culled/seed; center sparse); the wider field + the radius-aware keep-clear (TASK-140) drive culls toward ~0. Build-master widens ground Y, NavMeshBounds XY + **Z (mandatory)**, and the boundary walls (TASK-143).
3. **Terrain BLOCKS projectiles** — arrows die on rocks/hills/tree-trunks. Accepted balance change. Delivered by the single `Tags.Add("Terrain")` on `ASiegeBattlefieldScatter` (TASK-140) — `AProjectile::FindEnvironmentImpact` already object-traces the HISMs and filters by owner tag, so one tag closes the gap. Regression to prove: a crown tower still shoots units below without its arrows dying on its own hill (PIE #7; fallback = drop the tag).
4. **Units climb too** — the navmesh generates OVER hills (`bFillCollisionUnderneathForNavmesh` on real-geometry blockers + the mandatory NavMeshBounds Z raise, 5→12 / ±1200). Closes the "melee can't reach a hero standing on a crown" exploit.

Plus a recorded manager ruling: **the loop-reorder that makes scatter placement radius-aware CHANGES existing seeds' layouts** (mesh+scale rolled BEFORE the keep-clear test → a different `FRandomStream` draw order). This is DOCUMENTED, NOT a regression — a fixed `OverrideSeed` produces a new-but-deterministic layout.

**M6.6 exit criteria (playable slice):** the hero WALKS (not jumps) up a ≤30° hill flank to a flat crown, on all 3 meshes at min+max scale (repeat at sprint) and descends without launching/sliding; the camera does NOT clip inside the mound on a crown; a melee unit PATHS OVER a hill to its target and a unit on a crown paths down; an enemy Footman at a hill base REACHES and DAMAGES a hero on the crown (anti-exploit); the placement ghost projects onto the hill SURFACE — a tower on a crown is ACCEPTED, on a flank REFUSED "Too steep" (net-zero gold); a crown tower shoots units below without its arrows dying on its own hill; the hero stops ~70 cm from a tree trunk (not 8 m) and units don't detour around empty air; across 3 fresh seeds `LogSiegeTerrain` reports "Traversability CONFIRMED" with 0 culls on ≥2 of 3; a full match runs end-to-end with no "Failed to find path" spam; a sprint+jump off the highest crown at the field edge does NOT clear a boundary wall (KillZ → respawn). Recordable: hero + unit climbing a hill, the anti-exploit reach, and the high-ground tower payoff.

Dispatch shape: **TASK-138 (manager — CONVENTIONS law) lands FIRST (done).** Then the FILE/ART WAVE (parallel): **TASK-139 (art hills) ∥ TASK-140 (scatter C++) ∥ TASK-141 (hero-movement C++)** — 140 and 141 edit DISJOINT files (BattlefieldScatter/ScatterConfig vs HeroCharacter), the art is independent. QA gates the two code tasks together: **TASK-142** (pre-compile review of 140+141). Then the BUILD chain (serial, single editor + Git): **TASK-143** (compile 140+141 editor-bounce + widen L_Arena + navmesh XY/Z) → **TASK-144** (repopulate DA_BattlefieldScatter — runs AFTER the 143 compile, since the CollisionProxy/FootprintRadius fields only exist then; also needs the 3 imported hills from 139) → **TASK-145** (final compile-verify, the 13-point PIE suite, ONE commit not pushed, cut `m6.6-testable` + the skipped `m6.5-testable` @ 6a4c17d).

#### TASK-138 — CONVENTIONS "Climbable terrain (M6.6)" law block (manager)
- assignee: manager
- status: **done** (2026-07-14 — the CONVENTIONS "Climbable terrain (M6.6)" section is written + live; this decomposition is the deliverable. Must land before 139/140/141 — it does.)
- blocked-by: none
- parallel-safe: no (the naming law MUST exist before the code/art tasks reference it)
- spec: >
    Write the CONVENTIONS.md "Climbable terrain (M6.6)" section (the naming/geometry law the assignees follow
    character-for-character). Pin: (1) the three hill mesh names `SM_Hill_01/02/03` at `/Game/Meshes/` + raw
    FBX paths `Content/RawAssets/Hill_0N.fbx`; (2) the climbable-geometry law — ≤30° faces, ≤8° crowns, ≥120 cm
    toe fillet, CONVEX geometry (no undercuts), ≤1200 tris, Nanite OFF, origin base-center, UV `UVMap`,
    `generate_convex_collisions(hull_count=1)`, readback acceptance (convexElems==1 / hull ZMax==mesh ZMax /
    measured max face angle), MI_BattlefieldGround on slot 0; (3) the hero UPROPERTY names
    `HeroMaxStepHeight`/`HeroWalkableFloorAngle`/`HeroJumpZVelocity` with the C4457/58/59 shadow-avoidance
    rationale (the `Hero` prefix disambiguates from the identically-named UCharacterMovementComponent fields);
    (4) the `"Terrain"` actor tag on `ASiegeBattlefieldScatter`; (5) the scatter collision-channel law (real
    geometry blocks Pawn+Visibility+Camera, WorldStatic stays Ignore) and the tree collision-proxy contract
    (visual HISM = NoCollision + no-nav; paired proxy HISM = Pawn-block-only + nav + fill-underneath, VisualToProxy
    cull-in-parallel); (6) the radius-aware `FootprintRadius` seed-reorder note. Respect existing CONVENTIONS
    formatting; do NOT disturb prior sections (M6.5 stays live, M4.5 stays superseded-history). Post the milestone
    kickoff (top-level, manager-allowed) + the full breakdown in 📢 Planning & Feedback.
- names: >
    New CONVENTIONS.md section "## Climbable terrain (M6.6)". Pins: `SM_Hill_01/02/03` (/Game/Meshes/,
    raws Content/RawAssets/Hill_0N.fbx); `HeroMaxStepHeight`/`HeroWalkableFloorAngle`/`HeroJumpZVelocity`
    (AHeroCharacter); `"Terrain"` tag on ASiegeBattlefieldScatter; FScatterLayer CollisionProxyMesh/
    CollisionProxyScale/CollisionProxyZOffset + FootprintRadius; VisualToProxy proxy contract.

#### TASK-139 — Author 3 convex climbable hill meshes SM_Hill_01/02/03 (art)
- assignee: art-director
- status: done (integrated by build-master at TASK-144, 2026-07-14 — SM_Hill_01/02/03 swapped into the DA_BattlefieldScatter Hill layer, stone_hill dropped; read-back confirmed)
- blocked-by: TASK-138
- parallel-safe: yes (Blender authoring + import; no code dependency — the naming law is fixed at TASK-138. The editor-import step serializes with any other single-editor-mutating task, but the file work is independent)
- spec: >
    Art content — Blender authoring → FBX → editor import; needs Blender + the editor MCP up (else park + tell
    the orchestrator). Author THREE purpose-built CONVEX climbable hill meshes per CONVENTIONS "Climbable
    terrain (M6.6)". These REPLACE the unclimbable `stone_hill` dome in the Hill scatter layer (build-master
    swaps the layer at TASK-144). Dimensions (base / crown / height / target max face angle):
    `SM_Hill_01` knoll = r700 / r220 / 250 / ~27.5°; `SM_Hill_02` hill = r1100 / r320 / 400 / ~27°;
    `SM_Hill_03` ridge = 2200×1700 / 1400×350 / 350 / ~27.5°.
    LAW (the gate): every face angle ≤ 30° (under both the 44.76° WalkableFloorAngle AND Recast's 44°
    AgentMaxSlope); crown near-flat ≤ 8°; a toe fillet ≥ 120 cm where the flank meets ground (NO near-vertical
    skirt — the stone_hill defect); CONVEX, NO undercuts (one hull can't represent concavity — the ridge is one
    stretched dome, never a saddle); ≤ 1200 tris; Nanite OFF; origin at base-center (z_min=0); UV layer `UVMap`.
    Export FBX to `Content/RawAssets/Hill_01/02/03.fbx` (checked into Git); import to `/Game/Meshes/SM_Hill_01/
    02/03`. Collision via `generate_convex_collisions(hull_count=1)` — exactly ONE convex hull matching the
    render mesh (a multi-hull / box hull re-introduces an unclimbable step). Material: assign the EXISTING
    `/Game/Materials/Instances/MI_BattlefieldGround` to slot 0 — NO new material.
    ACCEPTANCE = READBACK, not vibe (TASK-088/135 technique): for each mesh report `convexElems == 1`,
    `hull ZMax == mesh ZMax` (proves the crown is not bulged, not domed), and the MEASURED max face-normal-vs-+Z
    angle (that number is the gate — must read ≤ 30°). Deliver the three readbacks in handoffs/TASK-139.md.
    Do NOT touch code, the DataAsset, or L_Arena. Art skips QA → build-master integration (TASK-144). Post the
    handoff in 🎨 Art (`🎨 ART-DIRECTOR: … TASK-139`) with the three readback triplets.
- names: >
    `SM_Hill_01` (knoll) / `SM_Hill_02` (hill) / `SM_Hill_03` (ridge) at `/Game/Meshes/SM_Hill_0N`;
    raws `Content/RawAssets/Hill_01/02/03.fbx`. Collision `generate_convex_collisions(hull_count=1)`
    (convexElems==1, hull ZMax==mesh ZMax). Material slot 0 = `/Game/Materials/Instances/MI_BattlefieldGround`
    (no new material). ≤1200 tris, Nanite OFF, origin base-center, UV `UVMap`. Law: CONVENTIONS "Climbable
    terrain (M6.6)".

#### TASK-140 — Scatter C++: Terrain tag, blocking-channel + nav-fill, radius-aware keep-clear/spacing/edge-clamp, FootprintRadius, tree collision-proxy (C++ files)
- assignee: gameplay-programmer
- status: qa-passed
- blocked-by: TASK-138
- parallel-safe: yes (edits ScatterConfig.h + BattlefieldScatter.h/.cpp ONLY — DISJOINT from TASK-141's HeroCharacter.h/.cpp; no compile in this task)
- spec: >
    Files only — NO editor/MCP, NO compile (build-master compiles at TASK-143). Per CONVENTIONS "Climbable
    terrain (M6.6)". Edit `Siegebound/ScatterConfig.h` + `Siegebound/BattlefieldScatter.h/.cpp`:
    (1) Constructor: `Tags.Add(FName(TEXT("Terrain")))` — exact string `Terrain` (NOT `Obstacle`); closes
    projectile pass-through (decision #3).
    (2) Real-geometry blocking branch (rocks/slabs/hills) in the mesh-resolve path: `SetCollisionResponseToChannel
    (ECC_Visibility, ECR_Block)` + `(ECC_Camera, ECR_Block)` + `bFillCollisionUnderneathForNavmesh = true` (units
    climb OVER, camera doesn't clip a crowned hero, arrows die on the mound). Leave `ECC_WorldStatic` on Ignore
    (`GroundZAt` `:459` traces it for the placement ghost).
    (3) RADIUS-AWARE keep-clear (the headline fix): `IsInKeepClear` (`:427`) today tests instance CENTER only, so
    a wide hill off-lane still sprawls across the corridor (the 14–69-culls/seed cause). Add an `InstanceRadius`
    param; inflate the corridor test to `|Y| <= CorridorHalfWidth + R` and each zone to `DistSq <= (sqrt(RadiusSq)+R)^2`.
    Apply on the mirror path (`:258`) too.
    (4) FIELD-EDGE clamp: reject candidates where `|X|+R > HalfX` or `|Y|+R > HalfY` (big instances stop sprawling
    into the boundary walls).
    (5) Radius derivation: new `FScatterLayer::FootprintRadius` (0 = auto = `FVector2D(Bounds.BoxExtent.X,.Y).Size()*Scale`).
    ⚠️ FORCES A LOOP REORDER — mesh+scale rolled at `:236/:243` must move BEFORE the keep-clear test at `:214`; this
    changes the `FRandomStream` draw order so EXISTING seeds produce NEW layouts (NOT a regression — DOCUMENT loudly
    in a comment + the handoff).
    (6) Radius-aware `MinSpacing`: store `TArray<TPair<FVector2D,float>>`, test `DistSq < (MinSpacing+Ri+Rj)^2`
    (hills stop interpenetrating).
    (7) TREE COLLISION-PROXY: new `FScatterLayer` fields `TSoftObjectPtr<UStaticMesh> CollisionProxyMesh`,
    `FVector CollisionProxyScale=(1,1,1)`, `float CollisionProxyZOffset=0`. When set: the VISUAL HISM → NoCollision +
    `bCanEverAffectNavigation=false`; ONE paired PROXY HISM per visual mesh (`SetVisibility(false)`,
    `SetCastShadow(false)`) carrying QueryOnly + **Pawn block ONLY** + `bCanEverAffectNavigation=true` +
    `bFillCollisionUnderneathForNavmesh=true`. ⚠️ `CullCorridorBlockers` (`:534`) removes from nav-relevant comps
    (= the proxy) and will ORPHAN the visible tree — keep `TMap<UHISM*,UHISM*> VisualToProxy`, cull BOTH in
    parallel, and register both proxies in `ScatterComponents` so `ClearScatter` reaches them.
    (8) Channel rule: real-geometry blockers block Pawn+Visibility+Camera; tree proxies block Pawn ONLY (else the
    placement ghost snaps to the invisible cylinder). Everywhere null-safe; ZERO combat/stat behavior change; do
    NOT touch HeroCharacter (TASK-141), cards.csv, or DT_Cards. ACCEPTANCE: compiles warnings-as-errors (verified
    at TASK-143); Terrain tag present; blocking channels + nav-fill on real geometry; radius-aware keep-clear/edge/
    spacing; FootprintRadius + the reorder documented; tree proxy system + VisualToProxy parallel cull. → qa-reviewer
    (MANDATORY inherited-reflected-member shadow scan + complete-type-include scan — the HISM/UStaticMesh/collision-
    channel/NavigationSystem includes are the trap; ALSO trace the visual/proxy cull-desync item 7 explicitly). Post
    in ⚙️ Dev & QA (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-140`).
- names: >
    `Siegebound/ScatterConfig.h` — `FScatterLayer` gains `FootprintRadius` (float, 0=auto), `CollisionProxyMesh`
    (TSoftObjectPtr<UStaticMesh>), `CollisionProxyScale` (FVector=(1,1,1)), `CollisionProxyZOffset` (float=0).
    `Siegebound/BattlefieldScatter.h/.cpp` — ctor `Tags.Add(FName(TEXT("Terrain")))`; real-geometry blockers
    block ECC_Pawn+ECC_Visibility+ECC_Camera + bFillCollisionUnderneathForNavmesh; WorldStatic stays Ignore;
    `IsInKeepClear` gains InstanceRadius (corridor + zones radius-inflated, mirror path too); field-edge clamp;
    radius-aware MinSpacing; `TMap<UHISM*,UHISM*> VisualToProxy` (cull visual+proxy in parallel; both registered
    in ScatterComponents). Tree proxy = Pawn-block-only + nav. Law: CONVENTIONS "Climbable terrain (M6.6)".

#### TASK-141 — Hero movement tuning: HeroMaxStepHeight/HeroWalkableFloorAngle/HeroJumpZVelocity via ApplyTerrainMovementTuning() (C++ files)
- assignee: gameplay-programmer
- status: qa-passed
- blocked-by: TASK-138
- parallel-safe: yes (edits `HeroCharacter.h/.cpp` ONLY — DISJOINT from TASK-140's scatter files; no compile in this task)
- spec: >
    Files only — NO editor/MCP, NO compile (build-master compiles at TASK-143). Per CONVENTIONS "Climbable
    terrain (M6.6)". Edit `Siegebound/HeroCharacter.h/.cpp` ONLY — NEVER touch the template base
    `GitClaudeUnrealTestCharacter.*` (CONVENTIONS template law; it is the base for the Variant_* maps).
    Expose THREE `UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Siegebound|Movement")` tunables and apply
    them via a helper `ApplyTerrainMovementTuning()` called from BOTH the constructor AND `BeginPlay` (mirror the
    existing `ApplyMovementSpeed()` at `HeroCharacter.cpp:45,82`):
    `HeroMaxStepHeight` = 50 (was 45) → `GetCharacterMovement()->MaxStepHeight`;
    `HeroWalkableFloorAngle` = 50 (was 44.76) → `SetWalkableFloorAngle(HeroWalkableFloorAngle)`;
    `HeroJumpZVelocity` = 600 (was 500) → `GetCharacterMovement()->JumpZVelocity`.
    The `Hero` prefix is MANDATORY — it avoids SHADOWING the identically-named UCharacterMovementComponent fields
    (MaxStepHeight/WalkableFloorAngle/JumpZVelocity), which compiles as the C4457/58/59 shadow HARD ERROR (the
    CONVENTIONS shadow law). The retune is comfort/margin only (the ≤30° hills are already climbable without it);
    `GravityScale`/`AirControl` UNCHANGED. `CharacterMovementComponent.h` is already included at `HeroCharacter.cpp:13`
    (complete-type law satisfied — but QA re-verifies). ACCEPTANCE: compiles warnings-as-errors (verified at
    TASK-143); the three UPROPERTYs read 50/50/600; `ApplyTerrainMovementTuning()` called from ctor + BeginPlay;
    no template-base file touched; no shadow. → qa-reviewer (MANDATORY shadow scan — the three names are the whole
    point + complete-type-include scan). Post in ⚙️ Dev & QA (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-141`).
- names: >
    `Siegebound/HeroCharacter.h/.cpp` ONLY. UPROPERTYs (EditAnywhere, BlueprintReadOnly, Category=
    "Siegebound|Movement"): `HeroMaxStepHeight`=50, `HeroWalkableFloorAngle`=50, `HeroJumpZVelocity`=600, applied
    by `ApplyTerrainMovementTuning()` from ctor + BeginPlay. NEVER edit `GitClaudeUnrealTestCharacter.*`. Law:
    CONVENTIONS "Climbable terrain (M6.6)".

#### TASK-142 — QA pre-compile review of TASK-140 + TASK-141 (qa)
- assignee: qa-reviewer
- status: backlog
- blocked-by: TASK-140, TASK-141
- parallel-safe: no (reviews both code tasks together before the compile)
- spec: >
    Pre-compile review of TASK-140 (scatter C++) + TASK-141 (hero movement) against CONVENTIONS + the M6.6 law.
    Mandated scans (CONVENTIONS coding laws): (a) inherited-reflected-member SHADOW law — the three hero tunables
    MUST be `Hero`-prefixed (an un-prefixed `MaxStepHeight`/`WalkableFloorAngle`/`JumpZVelocity` is the C4457/58/59
    hard error); watch scatter locals too (`Owner`/`Instigator`/`Slot`). (b) complete-type-include law — every
    member/`Cast<>`/method on a forward-declared type needs the full header `#include`d in that .cpp (HISM,
    UStaticMesh, NavigationSystem, collision channels for TASK-140; CharacterMovementComponent for TASK-141 — the
    latter already included at HeroCharacter.cpp:13, confirm it survived). (c) every `IsInKeepClear` caller updated
    for the new `InstanceRadius` param. (d) THE ONE THAT WILL BITE — the visual/proxy cull-desync (TASK-140 item 7):
    trace that `CullCorridorBlockers` culls BOTH the visual and proxy HISMs in parallel and both are registered in
    `ScatterComponents`, else the visible tree orphans. Write a PASS/FAIL report to qa/TASK-142-report.md and post
    the verdict + report path in ⚙️ Dev & QA (`🔍 QA: … TASK-142`). PASS → TASK-143; FAIL → back to
    gameplay-programmer (max 3 loops then escalate).
- names: >
    Report qa/TASK-142-report.md. Scans: shadow (CONVENTIONS §C++), complete-type-include (§coding law),
    IsInKeepClear caller sweep, visual/proxy cull-desync (TASK-140 item 7). Law: CONVENTIONS "Climbable terrain
    (M6.6)".

#### TASK-143 — PART A integration: compile 140+141, widen L_Arena Y→±4000 + navmesh XY/Z, rebuild nav, save (build)
- assignee: build-master
- status: done (build-master 2026-07-14, RESUME 2). GATE A compile PASSED (Build.bat editor target, Result: Succeeded, 0 warnings-as-errors, UHT+link clean — ScatterConfig/BattlefieldScatter/HeroCharacter). Editor relaunched detached, MCP healthy. L_Arena widened via set_actor_transform + read-back verified: ArenaGround scale.Y 48→80; NavMeshBounds_Arena scale.Y 24→40 + scale.Z 5→12 (±1200 mandatory raise); ArenaBoundary_North/South loc.Y ±2400→±4000; East/West scale.Y 48→80 (corner-leak catch). KillZ −2000 unchanged. save_assets(['/Game/Maps/L_Arena'])=true. Nav is runtime-Dynamic → regenerates at PIE (validated at TASK-145 traversability); no MCP Build>Navigation tool exists (would only matter for a static bake, which this level is not).
- blocked-by: TASK-142 (qa-passed); AND env-blocked on Smart App Control enforcement (see status)
- parallel-safe: no (single editor + the compile that unblocks TASK-144's DataAsset edit)
- spec: >
    Build-master integration, PART A. Needs the editor MCP up (else park + tell the orchestrator). Per CONVENTIONS
    "Climbable terrain (M6.6)" + "World axes (arena contract)".
    (0) COMPILE TASK-140 + TASK-141 C++ via the standard Build.bat editor-bounce (both scans first); any error →
    append to the offending task's QA report, qa-failed, stop (counts as a QA loop; NEVER edit code). This compile
    makes the new `FScatterLayer` CollisionProxy/FootprintRadius fields exist in the editor — REQUIRED for TASK-144's
    DataAsset edit (the M6.5 TASK-136 precedent: compile-in-the-first-build-task).
    (1) WIDEN L_Arena Y ±2400 → ±4000 (decision #2), via MCP `set_actor_transform` (proven TASK-136): the ArenaGround
    slab Y scale 48 → 80 (±2400 → ±4000); `ArenaBoundary_North/South` → ±4000 (hero still can't leave).
    (2) NAVMESH: `NavMeshBounds_Arena` XY to match the widened field AND **Z scale 5 → 12 (±500 → ±1200) — NOT
    OPTIONAL**: a ~520 cm crown + 144 headroom = 664 > 500, so without the Z raise the nav never generates on crowns
    and the whole feature silently fails (units can't climb, decision #4 breaks). REBUILD the navmesh.
    (3) SAVE via `save_assets(['/Game/Maps/L_Arena'])` (`save_actor` errors on this non-WP level). Do NOT change
    KillZ (−2000, vertical). If the nav volume needs a brush REBUILD (not just a scale) or a manual `Build >
    Navigation` click, that is a possible Jonathan-only step — flag it (do not fake).
    Do NOT commit here (TASK-145 owns the single commit). Post the compile result + widen + nav-rebuild + save in
    🔧 Build & Git (`🔧 BUILD-MASTER: … TASK-143`).
- names: >
    Compile TASK-140+141 (Build.bat editor-bounce). ArenaGround Y scale 48→80 (±4000); ArenaBoundary_North/South
    →±4000; NavMeshBounds_Arena XY match + **Z scale 5→12 (±1200)** + rebuild nav; `save_assets(['/Game/Maps/
    L_Arena'])`. KillZ −2000 UNCHANGED. NO commit (TASK-145). Law: CONVENTIONS "Climbable terrain (M6.6)" + "World
    axes (arena contract)".

#### TASK-144 — PART B integration: repopulate DA_BattlefieldScatter (3 new hills, tree collision-proxies, ±4000 extent) (build)
- assignee: build-master
- status: done (build-master 2026-07-14). DA_BattlefieldScatter repopulated + read-back verified: Hill=[SM_Hill_01/02/03] (stone_hill dropped), count 6, scale 0.9–1.3, spacing 2000, WholeField; Slabs scale 3–5×; Trees CollisionProxyMesh=/Engine/BasicShapes/Cylinder, CollisionProxyScale (1.4,1.4,17), CollisionProxyZOffset +850 (derived from live 100³ centered-pivot bounds); ArenaHalfExtent.Y=4000, CorridorHalfWidth=800, bMirrorSymmetric=false. Cylinder-not-a-visual-mesh invariant holds. save_assets(DA)=true.
- blocked-by: TASK-143 (compiled config fields + widened arena), TASK-139 (SM_Hill_01/02/03 imported)
- parallel-safe: no (single editor + Git; the CollisionProxy fields only exist after the TASK-143 compile)
- spec: >
    Build-master integration, PART B. Needs the editor MCP up (else park + tell the orchestrator). Edit
    `/Game/Data/DA_BattlefieldScatter` (a USiegeScatterConfig instance — editing FScatterLayer via MCP proven
    TASK-137). Ground truth is the `LogSiegeTerrain` scatter log, NOT `get_properties` (shallow-reads nested struct
    fields as null — TASK-137 caveat). Per CONVENTIONS "Climbable terrain (M6.6)".
    - **Hill layer:** Meshes → `[SM_Hill_01, SM_Hill_02, SM_Hill_03]`, DROP `stone_hill`; scale range 0.9–1.3 (the
    new meshes are authored at real size — no more 10–15×); bias → WholeField (safe now the radius test guards the
    lane).
    - **Slabs/Rocks:** scale 3–5× (stays a blocking obstacle, not a hill).
    - **Trees:** set `CollisionProxyMesh = /Engine/BasicShapes/Cylinder`; READ BACK the engine Cylinder's actual
    bounds (100³, centered pivot) to derive `CollisionProxyScale` ≈ (1.4, 1.4, 17) + `CollisionProxyZOffset` ≈ +850
    (do the math from the real readback — don't hardcode blind). Result: hero stops ~70 cm from the trunk, not 8 m.
    - **Config:** `ArenaHalfExtent.Y` 2400 → 4000; `CorridorHalfWidth` stays 800 (now genuinely honored by the
    radius test). `bMirrorSymmetric=false` (asymmetric — unchanged M6.5 ruling).
    Apply any per-mesh origin offsets. Do NOT commit here (TASK-145 owns the commit). Post the layer changes + the
    Cylinder-proxy readback math + the scatter-log confirmation in 🔧 Build & Git (`🔧 BUILD-MASTER: … TASK-144`).
- names: >
    `/Game/Data/DA_BattlefieldScatter`: Hill layer Meshes=[SM_Hill_01,02,03] (drop stone_hill), scale 0.9–1.3,
    WholeField; Slabs scale 3–5×; Trees CollisionProxyMesh=`/Engine/BasicShapes/Cylinder`, CollisionProxyScale
    ≈(1.4,1.4,17), CollisionProxyZOffset≈+850 (from readback); ArenaHalfExtent.Y=4000; CorridorHalfWidth=800;
    bMirrorSymmetric=false. NO commit (TASK-145). Law: CONVENTIONS "Climbable terrain (M6.6)".

#### TASK-145 — Final integration: compile-verify, 13-point PIE suite, ONE commit (not pushed), cut m6.6-testable + m6.5-testable (build)
- assignee: build-master
- status: done (build-master 2026-07-14 — FINALIZED). Jonathan PLAYTESTED and confirmed every human-gated item PASSES (hero climbs the hill flanks up/down; anti-exploit gate #5 — an enemy unit reaches and damages a hero standing on a crown; camera/tower/escape/perf all good) → combined with the already-passed machine gates (#9 traversability on all 3 seeds, #10 no path failures, #13 clean log sweep) all commit gates are satisfied. **M6.6 was committed by Jonathan HIMSELF as `057ca9f "walkable terrain"` and PUSHED** (author=committer=Jonathan; the same self-commit pattern he used for M6.5 @ 6a4c17d). Its file set matches the intended M6.6 scope EXACTLY — ScatterConfig.h, BattlefieldScatter.cpp/.h, HeroCharacter.cpp/.h, SM_Hill_01/02/03.uasset, Hill_01/02/03.fbx, DA_BattlefieldScatter.uasset, L_Arena.umap + pipeline docs (TASKBOARD/CONVENTIONS/handoffs 139/140/141/qa-142); NO marketplace packs, NO .uproject. No separate build-master M6.6 commit made (an extra commit would be empty/duplicate; amending pushed history is forbidden). Branches cut: `m6.6-testable` @ 057ca9f, `m6.5-testable` @ 6a4c17d (retroactive). Committed L_Arena carries the STALE serialized nav bake — the umap LFS size is byte-identical (146618) to the pre-widen 6a4c17d, so no fresh 513-tile `Build>Navigation` save landed on disk — NON-BREAKING: the RecastNavMesh is runtime-Dynamic and regenerates the correct ±4000 mesh at every PIE start (proven by gates #9/#10 + Jonathan's live climb + anti-exploit playtest). GATE A compile PASS; TASK-143 widen + TASK-144 DA both done/verified/saved. Machine gates run headlessly: #10 CLEAN (no unit "Failed to find path" in a live full match, both teams spawning+marching); #13 benign-only; #9 traversability CONFIRMED on 3 fresh seeds (781344065/1316789505/364718977 — the Blue→Red guarantee holds) BUT not clean 0-cull (instances culled 3/6/1). Root cause: L_Arena's BAKED navmesh is stale after the ±4000 widen — serialized 285 tiles/9-bit (old ±2400) vs 513/10-bit required → RecastNavMesh recreated at every PIE start → nav-settle latency → defensive corridor culls (seed-3 attempt-2 culled 0 yet path still pending = proves latency, NOT a corridor breach). HELD on two Jonathan-only items: (a) manual `Build > Navigation` + save L_Arena (MCP exposes no nav-build tool; also clears the #9 culls); (b) machine gates #5 (anti-exploit, SummonTestUnit) + #4 (unit-climb) are not runnable via MCP (no console-exec / UFunction-call; ProgrammaticToolset sandbox excludes `unreal`) — they overlap Jonathan's manual PIE list #1/2/3/7/11/12. RESOLVED 2026-07-14: Jonathan playtested (all human/console gates pass — climb, anti-exploit #5, unit-climb #4) and self-committed `057ca9f` (pushed); the manual nav bake was NOT re-saved but is moot (runtime-Dynamic regen); both `m6.6-testable` @ 057ca9f and `m6.5-testable` @ 6a4c17d are now cut. Feature otherwise healthy: terrain tag live (projectile block #3), hills place (4/6/6 across seeds, all 3 SM_Hill variants resolve). Density notes (Jonathan visual #12): Trees 14–18/55, Grass ~1450/2500 on the wider field (radius-aware spacing).
- blocked-by: TASK-144
- parallel-safe: no (single editor + Git; the closing task)
- spec: >
    Build-master final integration. Needs the editor MCP up (else park + tell the orchestrator). Build.bat per
    CLAUDE.md. (1) RE-VERIFY a clean compile of TASK-140+141 (shadow + complete-type-include scans). (2) Run the
    13-POINT PIE CHECKLIST from the authoritative plan (`C:\Users\wesel\.claude\plans\we-last-left-off-partitioned-
    puppy.md`), GDI screenshots + LogSiegeTerrain: #1 hero WALKS (no jump) up a flank to a crown, all 3 meshes,
    min+max scale, repeat at sprint; #2 descends without launching/sliding; #3 camera doesn't clip inside the mound
    on a crown; #4 a unit climbs a hill + a unit on a crown paths down (proves nav on crown → NavBounds Z landed);
    #5 ANTI-EXPLOIT — hero on crown, enemy Footman at base REACHES + damages him; #6 ghost projects onto the hill
    surface, tower on crown ACCEPTED / on flank REFUSED "Too steep" (net-zero), unit placement on flank still works;
    #7 REGRESSION — crown tower shoots a unit below, arrows NOT destroyed by its own hill; #8 hero stops ~70 cm from
    a trunk (not 8 m), units don't detour around empty air; #9 TRAVERSABILITY — 3 fresh seeds, "Traversability
    CONFIRMED" with 0 culls on ≥2 of 3 (>5 = radius keep-clear didn't land → route back to gameplay-programmer,
    counts as a QA loop); #10 full match end-to-end, no "Failed to find path" spam; #11 sprint+jump off the highest
    crown at the field edge does NOT clear a boundary wall (KillZ → respawn); #12 perf/visual = JONATHAN's eyeball
    (foreground PIE) — record best-effort FPS + instance counts; #13 log sweep vs the known-benign set (DeepMine
    CardType-2, victory-focus, RecastNavMesh boot, CrowdFollowing teardown). Checks 1/2/3/12 are Jonathan's to
    eyeball (HISM scatter is PIE-runtime-only — un-capturable in the editor viewport); confirm the rest in the
    pixels/logs. Any HARD-gate failure → route back per the routing rules (never fake a pass). (3) COMMIT ONE
    commit on `main` (task-ID message, TASK-138..145), NOT pushed; cut `m6.6-testable` at the commit AND
    `m6.5-testable` at `6a4c17d` (the skipped M6.5 branch — milestone-preservation workflow; neither pushed). Post
    the PIE results per check + FPS + the commit hash + both branches in 🔧 Build & Git (`🔧 BUILD-MASTER: …
    TASK-145`).
- names: >
    Compile-verify (Build.bat). 13-point PIE suite (plan file). ONE commit on `main` "TASK-138..145: climbable
    terrain — convex SM_Hill_01/02/03 + ±4000 arena + terrain-blocks-projectiles + units-climb + tree
    collision-proxies + hero tuning", NOT pushed. Branches: `m6.6-testable` @ the commit + `m6.5-testable` @
    `6a4c17d`. Law: CONVENTIONS "Climbable terrain (M6.6)".

---

### M6.6 follow-ups (playtest feedback — post-`057ca9f`)

**Source:** Jonathan's M6.6 playtest (2026-07-14). The milestone shipped and is signed off (`057ca9f`, pushed); this is a
data-only refinement noted at TASK-145 (density observation) and in the M6.6 milestone entry (follow-up ii). NOT a reopen —
M6.6 stays `done`; these tune the shipped base. No CONVENTIONS change (no new asset names; the DataAsset + FScatterLayer
fields already exist per "Climbable terrain (M6.6)" / "Battlefield & procedural terrain (M6.5)").

#### TASK-146 — Scatter density tuning for the widened ±4000 field (build — data-only, no QA gate)
- assignee: build-master
- status: done (build-master 2026-07-14 — data-only tune of DA_BattlefieldScatter via MCP set_properties; SAVED + UNCOMMITTED per spec, awaiting Jonathan's live-Play eyeball). Editor was on L_MainMenu (stale PIE running); loaded L_Arena to run the scatter. Values (before→after): Trees minSpacing 600→300, footprintRadius 0→150 (explicit trunk-scale override — decouples placement from the auto-derived ~canopy bounds, the starvation cause; collision proxy unchanged so unit routing/"70cm-from-trunk" is unaffected), InstanceCount 55→70. Grass minSpacing 120→50 (InstanceCount 2500 kept). Blocking layers (Rocks/Boulders/Hill/Slabs) + all config (ArenaHalfExtent, CorridorHalfWidth 800, keep-clear) UNTOUCHED; read-back verified no clobber. 3 fresh seeds 944795841/513883457/400437185 → Trees 70/70·70/70·70/70 (100% ×3), Grass 2306/2313/2282 of 2500 (91–93% ×3) — up from ~12–18/55 trees & ~1435/2500 grass. Traversability CONFIRMED Blue→Red on all 3 with 0 culls (M6.6 baseline 0–3). Instance total ~2920/match ≈ original M6.5 budget (no new perf ceiling). DA saved (is_dirty=false); NO commit/branch/push.
- blocked-by: none (tunes the committed `057ca9f` base; single-editor serialize only)
- parallel-safe: no (single editor + the `DA_BattlefieldScatter` DataAsset edit; NO C++, NO new art, NO qa-reviewer gate — pure data tune via Unreal MCP)
- spec: >
    Data-only tune of `DA_BattlefieldScatter` (`/Game/Data/DA_BattlefieldScatter`) via Unreal MCP — NO C++, NO new art,
    NO qa-reviewer gate. PROBLEM (Jonathan M6.6 playtest): the ±2400 → ±4000 arena widen (TASK-143, ~60% more area) spread
    the decorative scatter out without a matching density bump, so the field reads THIN — only ~15 of 55 Trees and ~1450 of
    2500 Grass instances actually place on the wider field (`LogSiegeTerrain` "placed N (target M)"). GOAL: refill the field
    so it reads FULL. LEVERS (build-master iterates on the DECORATIVE Trees + Grass layers only): (a) lower the Trees/Grass
    `MinSpacing`; and/or (b) set/reduce an explicit small Trees `FootprintRadius`; and/or (c) raise the Trees/Grass
    `InstanceCount`. GROUND TRUTH is the `LogSiegeTerrain` "placed N (target M)" line read across a few FRESH seeds — tune
    until Trees and Grass place near their targets and the field reads full. HARD CONSTRAINTS: (1) do NOT break the
    traversability guarantee — the Blue→Red path must still confirm ("Traversability CONFIRMED"); (2) do NOT re-introduce
    corridor culls (the decorative fill must not wall the lane); (3) keep the BLOCKING layers (Hill / Slab / Rock / Boulder)
    placement essentially AS-IS — this task is about the decorative fill (Trees + Grass), not the blockers. Save
    `DA_BattlefieldScatter` when done. Report the BEFORE/AFTER placement counts (placed/target for Trees + Grass) in the
    handoff. Editor/MCP must be up (127.0.0.1:8000) — if unreachable, park + tell the orchestrator (never fake counts).
- acceptance: >
    Trees and Grass each place NEAR target on ≥2 of 3 fresh seeds (placed/target from `LogSiegeTerrain`); traversability
    still CONFIRMED (Blue→Red path holds, no new corridor culls); blocking-layer placement unchanged; `DA_BattlefieldScatter`
    saved. Final density is Jonathan's eyeball (HISM scatter is PIE-runtime-only). No commit unless Jonathan directs — report
    the tuned values + before/after counts to the orchestrator.
- names: >
    Edit `/Game/Data/DA_BattlefieldScatter` (`Content/Data/DA_BattlefieldScatter.uasset`) ONLY. FScatterLayer fields on the
    Trees + Grass layers: `MinSpacing`, `FootprintRadius`, `InstanceCount` (per CONVENTIONS "Battlefield & procedural terrain
    (M6.5)" / "Climbable terrain (M6.6)"). Leave Hill/Slab/Rock/Boulder layers as-is. Verify via `LogSiegeTerrain` "placed N
    (target M)". Law: CONVENTIONS "Climbable terrain (M6.6)" + "Battlefield & procedural terrain (M6.5)".

---

### M6.5 — Battlefield & procedural terrain (TASK-133..137) — decomposed 2026-07-10

**Authorization:** Jonathan's 2026-07-10 direct directive (verbatim): "I want to start creating the terrain/battlefield that the characters fight on. First, I want to space out the castles more. Make the distance that they are apart 4 times larger. After that … create a grassy terrain filled with rocks, trees, and hills. The location of all of these rocks, trees, and hills should be randomly generated at the start of each match. … choose what you think will look best. Try to use as many assets as possible (that still make sense …) for variety." Standalone milestone inserted after M6 (M4.5/M5.5 sub-milestone shape). SUPERSEDES the parked M4.5 (its hand-placed mirror-symmetric plan is retired — CONVENTIONS "Arena terrain & environment (M4.5)" supersede note; the Terrain/Obstacle tag + placement-clearance + projectile code contracts carry forward, INERT under the non-blocking default). Naming law added to CONVENTIONS "Battlefield & procedural terrain (M6.5)" + "World axes (arena contract)" (updated) BEFORE task issue. Hard gate stands: editor/MCP work needs 127.0.0.1:8000 up — park + tell the orchestrator if unreachable (never fake). Nothing pushed. Does NOT touch prior milestones' state.

**THE TWO PARTS:** PART 1 = 4× castle spacing (a LEVEL move + a small CODE fallback sync — no new assets); PART 2 = a RUNTIME procedural scatter of trees/rocks/hills/grass, re-seeded each match, soft-referencing Jonathan's imported Fab meshes. Per Jonathan's decision #1 the obstacles (trees/rocks/hills) BLOCK units + carve the navmesh (Dynamic RecastNavMesh + a hard castle-to-castle traversability guarantee); grass is non-blocking decoration; placement is ASYMMETRIC organic random (config toggle for mirror). PART 1's enlarged arena is where PART 2 scatters — but the asset browse (art) runs in parallel from the start.

**FOUR DECISIONS — ANSWERED by Jonathan 2026-07-10 (specs below reflect these; no longer pending):**
1. **Do scattered rocks/trees/hills BLOCK unit movement? → YES, BLOCKING (the bigger path).** Obstacles (trees/rocks/hills) physically block units AND carve the navmesh; grass stays non-blocking decoration. This MANDATES: (a) `L_Arena`'s RecastNavMesh set to `RuntimeGeneration = Dynamic` (TASK-136 — else runtime obstacles don't affect pathing and units walk through); (b) obstacle instances block the Pawn channel + `bCanEverAffectNavigation=true` (TASK-134); (c) a NON-NEGOTIABLE **traversability guarantee** — the scatter must NEVER wall off a side / trap units / block a spawn/castle; a match where units can't reach the enemy castle is a HARD FAILURE (TASK-134 reserves a clear corridor + keep-clear radii AND/OR validates castle-to-castle reachability post-placement). Perf is heightened (blocking + highpoly + dynamic-nav rebuild) → mobile/low-poly/LODs + capped counts + TASK-137 FPS check.
   - **Placement symmetry — ASYMMETRIC organic random (Jonathan ruling, FLAGGED, default asymmetric):** PvE (Blue player vs Red bot), so fairness matters less than PvP and Jonathan wants organic variety → FULLY ASYMMETRIC random placement (NOT the retired M4.5 mirror-symmetry). Exposed as a config toggle so Jonathan can switch to mirror-symmetric at playtest if matches feel unfair — do NOT silently impose mirror.
2. **Grass base → grass MATERIAL (`M_BattlefieldGround`) + scattered grass-blade meshes (HISM). No Landscape** (no MCP create/sculpt route; a real Landscape would need Jonathan's own editor time). No spec change — TASK-135/137 proceed as written.
3. **Keep-clear zones → YES** (exclusion radii around castles/nodes/PlayerStart + a clear central lane; grass excepted). Now doubly important — it is part of the traversability guarantee.
4. **4× castle distance ≈ 4× combat march time → PROCEED** (Jonathan confirmed). Miner→own-node economy UNCHANGED (nodes moved WITH the castles, ∓1200 → ±7200); only cross-map combat lengthens ~4×. Dial-able at playtest.

Plus a recorded manager ruling: **Play Again re-randomizes** (each new match = a fresh battlefield layout; `bReRandomizeOnMatchReset=true`) — matches Jonathan's "start of each match" wording; flip the flag to keep a layout if he prefers.

**M6.5 exit criteria (playable slice):** the castles sit 16000 apart (±8000), symmetric about X=0, with gold nodes at ±7200 and the hero spawning near the Blue castle; the navmesh (Dynamic RecastNavMesh), floor, and boundary walls cover the widened field; at match start the arena is a GRASSY battlefield randomly scattered (ASYMMETRIC organic random) with trees, rocks, and hills (variety across many Fab meshes), keep-clear zones honored; the obstacles physically BLOCK units and carve the navmesh so units visibly ROUTE AROUND them (not through); the **traversability guarantee holds** — across several matches units ALWAYS reach the enemy castle (never walled off / trapped / spawn-blocked); a SECOND match produces a DIFFERENT layout (seed logged on `LogSiegeTerrain`), Play Again re-scatters; FPS stays within the §6 budget (blocking + dynamic-nav is costly — flagged if counts must drop). Recordable: a fly-through of the procedurally-generated grassy battlefield + units pathing around obstacles + a two-match layout-diff clip.

Dispatch shape: **FILE WAVE NOW (parallel): TASK-133 ∥ TASK-134** (disjoint file sets) **∥ TASK-135** (art browse/curate/material — independent). QA gates 133 + 134 (shadow + complete-type-include scans mandatory). Then **TASK-136** (build: compile 133+134 + PART 1 scene move + widen + PIE + commit) → **TASK-137** (build: populate DA_BattlefieldScatter from TASK-135, place scatter actor + ground material, verify randomized scatter + perf, commit + m6.5-testable). 136/137 are serial (single editor); 137 needs BOTH the compiled config class (from 136) AND the curated list (from 135).

#### TASK-133 — Castle-spacing 4×: bot fallback constants + centerline-relative ruling (C++ files)
- assignee: gameplay-programmer
- status: qa-passed (orchestrator-proxied 2026-07-10 — 0 findings; constants CastleRed=(8000,0,0), GoldNodeRed=(7200,0,0), BotCenterlineSpawnX unchanged, no Blue fallback. Report: qa/TASK-134-report.md)
- blocked-by: none
- parallel-safe: yes (edits SiegeBotController.h constants/comments + GoldNode.h comments ONLY — disjoint from TASK-134's file set)
- spec: >
    Files only — NO editor/MCP, NO compile. PART 1 of the 4× battlefield-widening (Jonathan: "make the
    distance that they are apart 4 times larger"), CODE half. The castles/nodes/PlayerStart THEMSELVES are
    level actors moved by build-master (TASK-136); this task keeps the CODE fallbacks + docs in sync, per
    CONVENTIONS "World axes (arena contract)" (updated) and "Battlefield & procedural terrain (M6.5)".
    (1) `ASiegeBotController` (SiegeBotController.h): `CastleRedFallbackLocation` FVector(2000,0,0) →
    **FVector(8000,0,0)**; `GoldNodeRedFallbackLocation` FVector(1200,0,0) → **FVector(7200,0,0)** (gold
    nodes stay 800 units in front of each castle → move WITH the castle, preserving the miner economy — NOT
    left at 1200). Update the two doc comments (~lines 314/318) to the new world-axes coordinates.
    (2) `BotCenterlineSpawnX` (=350) is CENTERLINE-relative and the centerline did NOT move (still X=0) —
    LEAVE IT AT 350; add a one-line comment recording the ruling (arena widened 4× but this value is
    centerline-relative so it stays valid; a playtest may revisit whether the bot over-commits units across
    the wider field). (3) Confirm there is NO Blue fallback constant in code (player side uses live
    TActorIterator lookups) — add none. Update the GoldNode.h header comments (~lines 21/78) that cite ∓1200
    to ±7200. Do NOT touch SiegeGameMode (TASK-134 owns that file's PlayerStart comment fix). KillZ (−2000,
    VERTICAL) is NOT a horizontal-spacing value — do NOT touch it. ACCEPTANCE: the two fallback FVectors read
    (8000/7200); BotCenterlineSpawnX unchanged with the ruling comment; comments cite the new coordinates; no
    other logic changed. → qa-reviewer (MANDATORY inherited-reflected-member shadow scan + complete-type-
    include scan — CONVENTIONS coding laws). Post in ⚙️ Dev & QA (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-133`).
- names: >
    `ASiegeBotController::CastleRedFallbackLocation = FVector(8000.f,0.f,0.f)`, `GoldNodeRedFallbackLocation
    = FVector(7200.f,0.f,0.f)` (SiegeBotController.h). `BotCenterlineSpawnX = 350` UNCHANGED (ruling comment
    only). Comment-only: GoldNode.h ∓1200 → ±7200. NO Blue fallback constant (player uses TActorIterator).
    Do NOT edit SiegeGameMode or KillZ. Law: CONVENTIONS "World axes (arena contract)" (M6.5-updated).

#### TASK-134 — Procedural battlefield scatter: USiegeScatterConfig + ASiegeBattlefieldScatter (HISM, per-match seed, keep-clear, match-reset hook) (C++ files)
- assignee: gameplay-programmer
- status: qa-passed (orchestrator-proxied 2026-07-10 — 0 BLOCKER/2 WARN/3 NIT. Traversability guarantee VERIFIED holds (origin-based |Y|<=400 corridor + centerline actors + confirmatory nav cull-recheck converges). WARN resolved at TASK-137 DATA: keep obstacle footprint < CorridorHalfWidth(400); no cross-layer mesh reuse blocking+grass. Report: qa/TASK-134-report.md)
- blocked-by: none
- parallel-safe: yes (all-new files BattlefieldScatter.h/.cpp + ScatterConfig.h/.cpp; only shared edit is SiegeGameMode.h/.cpp for the reset hook + PlayerStart comment — disjoint from TASK-133's SiegeBotController.h/GoldNode.h)
- spec: >
    Files only — NO editor/MCP, NO compile. PART 2 code: the RUNTIME procedural scatter that fills the
    (widened) arena with trees/rocks/hills/grass, re-seeded each match (Jonathan: "randomly generated at the
    start of each match … use as many assets as possible for variety"), per CONVENTIONS "Battlefield &
    procedural terrain (M6.5)". Content (which meshes/density) is DATA — build-master populates the DataAsset
    (TASK-137); this task delivers the CLASS + LOGIC only.
    (1) NEW `USiegeScatterConfig` (UDataAsset subclass, ScatterConfig.h/.cpp): a per-layer array of
    `FScatterLayer { TArray<TSoftObjectPtr<UStaticMesh>> Meshes; int32 InstanceCount/Density; FVector2D
    ScaleRange; bool bRandomYaw; bool bBlocking; region-bias (whole-field / edge-bias); float MinSpacing; }`
    for TREES / ROCKS / HILLS / GRASS (programmer picks the exact struct shape + sane defaults). `bBlocking`
    defaults TRUE for the obstacle layers (TREES/ROCKS/HILLS — Jonathan's decision #1: they physically block
    units + carve the navmesh) and FALSE for GRASS (pure decoration). Also expose a config-level
    `bMirrorSymmetric` toggle (default FALSE = ASYMMETRIC organic random per Jonathan's ruling; TRUE mirrors
    placement across X=0 — the fallback if playtest reads unfair). BlueprintType, UPROPERTY(EditAnywhere) so
    the DataAsset is editor-populated.
    (2) NEW `ASiegeBattlefieldScatter` (AActor, BattlefieldScatter.h/.cpp): `UPROPERTY(EditDefaultsOnly)
    TObjectPtr<USiegeScatterConfig> ScatterConfig` (unset ⇒ graceful no-op, log once); `int32 OverrideSeed`
    (0 = random each match) + `bool bReRandomizeOnMatchReset` (default true). Owns ONE
    UHierarchicalInstancedStaticMeshComponent PER unique mesh (perf law — NEVER individual actors).
    `GenerateScatter()`: pick a seed (OverrideSeed>0 else FMath::Rand), LOG it on `LogSiegeTerrain`
    (grep-able, so a layout is reproducible), then for each layer scatter InstanceCount instances via an
    `FRandomStream` (ASYMMETRIC organic random unless bMirrorSymmetric) across the arena bounds (read from the
    config or the level extents), honoring: MinSpacing, ScaleRange, random yaw, region bias, and the KEEP-CLEAR
    zones — exclude trees/rocks/hills from castle pads (r~900 @ ±8000), gold-node pads (r~500 @ ±7200), the
    PlayerStart/spawn area, and the central combat corridor (|Y| ≤ ~400); GRASS ignores keep-clear.
    **COLLISION (Jonathan's decision #1 = BLOCKING):** obstacle-layer (bBlocking) instances block the Pawn
    channel and set `bCanEverAffectNavigation = true` so they carve the navmesh and units route AROUND them
    (this requires L_Arena's RecastNavMesh = RuntimeGeneration Dynamic — TASK-136 sets it; without Dynamic,
    runtime obstacles do nothing). GRASS instances = NoCollision, no nav effect.
    **TRAVERSABILITY GUARANTEE (NON-NEGOTIABLE — a match where units can't reach the enemy castle is a HARD
    FAILURE):** GenerateScatter MUST guarantee a navigable Blue-castle → Red-castle path every match. Achieve it
    via the reserved central corridor + keep-clear radii above AND a post-placement REACHABILITY VALIDATION:
    after scattering (once the nav has updated — Dynamic nav is async, so wait/rebuild as needed), test a path
    between the castle anchors; if none exists, cull the offending blocking instances (or re-roll) until a path
    is confirmed. Never leave a match unwinnable. `ClearScatter()`: clear all HISM instances.
    (3) Match lifecycle: scatter at match start (BeginPlay) AND re-scatter on Play Again — SiegeGameMode's
    reset path calls `ClearScatter()` + `GenerateScatter()` (new seed if bReRandomizeOnMatchReset) on the
    `ASiegeBattlefieldScatter` found via TActorIterator (null-safe — no scatter actor = no-op; nothing
    breaks). While in SiegeGameMode, ALSO fix the stale PlayerStart doc comment (~.cpp:457 / ~.h:317) to the
    new ≈(−6800,0,100).
    Everywhere null-safe; ZERO combat/stat behavior change; do NOT touch cards.csv / DT_Cards / any card
    actor. ACCEPTANCE: compiles warnings-as-errors (verified at TASK-136); one HISM per unique mesh; seed
    logged; keep-clear excludes the listed zones; obstacle layers block Pawn + affect navigation (grass does
    not); the reachability validation guarantees a castle-to-castle path every match (offending blockers
    culled/re-rolled); asymmetric placement by default (bMirrorSymmetric toggle present); Play Again
    re-scatters; unset config = clean no-op. → qa-reviewer (MANDATORY shadow scan + complete-type-include scan
    — the HISM / UDataAsset / NavigationSystem / GameMode includes are the trap; ALSO review the traversability
    validation logic — a wall-off is a hard failure). Post in ⚙️ Dev & QA (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-134`).
- names: >
    `USiegeScatterConfig` (UDataAsset), ScatterConfig.h/.cpp + `FScatterLayer` struct
    (Meshes[TSoftObjectPtr<UStaticMesh>], count/density, ScaleRange, bRandomYaw, bBlocking [TRUE obstacle
    layers / FALSE grass], region, MinSpacing) + config `bMirrorSymmetric` (default false) for
    TREES/ROCKS/HILLS/GRASS. `ASiegeBattlefieldScatter` (AActor), BattlefieldScatter.h/.cpp —
    HISM-per-mesh, ScatterConfig ref, OverrideSeed(0), bReRandomizeOnMatchReset(true), GenerateScatter()/
    ClearScatter(), LogSiegeTerrain. SiegeGameMode reset path Clear+Generate via TActorIterator (null-safe) +
    PlayerStart comment fix. DataAsset instance (TASK-137): /Game/Data/DA_BattlefieldScatter. Level instance
    (TASK-137): `BattlefieldScatter` in L_Arena. Law: CONVENTIONS "Battlefield & procedural terrain (M6.5)".

#### TASK-135 — Curate + prep the Fab battlefield meshes + author M_BattlefieldGround grass material (art)
- assignee: art-director
- status: **done** (2026-07-10/11) — curated set (12 trees / 12 rocks / 6 hills / 15 grass, collision-verified) + `M_BattlefieldGround` authored, SAVED, and applied to ArenaGround (was lost to an editor bounce once, re-authored + disk-verified 2nd time). MI_BattlefieldGround saved. Handoff: handoffs/TASK-135-artist.md.
- blocked-by: none
- parallel-safe: yes (browse/curate/material — no code dependency; the editor-import/material-author step serializes with any other single-editor-mutating task)
- spec: >
    Art content — needs the editor MCP up for the material author + mesh inspection (else park + tell the
    orchestrator). Per Jonathan: "look through the newly imported assets from Fab … choose what you think will
    look best … use as many assets as possible (that still make sense) for variety." Per CONVENTIONS
    "Battlefield & procedural terrain (M6.5)".
    (1) BROWSE ALL the imported Fab packs (roots in the CONVENTIONS M6.5 section: Tree_Pack_1 Highpoly/Mobile,
    Realistic_Grass_and_plant, Megaplant_Library, Realistic_Rocks, Fab/Rocks/highpoly…, Fab/Stone_Hills_FREE/
    stone_hill). CURATE the best-looking set for FOUR layers — TREES, ROCKS, HILLS, GRASS/plants —
    maximizing variety within reason. Donors are READ-ONLY: SOFT-REFERENCE the meshes IN PLACE (do NOT
    conform/duplicate/rename — the point is cheap variety). PREFER the MOBILE / low-poly tree variants and the
    lowest-poly grass for the scattered layers (perf law). Do NOT pick character-pack, VFX, or castle-wall/
    siege meshes (a few siege props MAY be dressing at your judgment — flag if used).
    (2) For each chosen mesh RECORD: its FULL /Game/ object path, tri count, whether it has usable LODs, and
    whether its pivot/origin sits on the ground (note any off-ground pivot so build-master can offset it).
    **COLLISION MATTERS NOW (Jonathan's decision #1 = BLOCKING obstacles):** for the OBSTACLE layers
    (trees/rocks/hills) verify each mesh has usable SIMPLE collision (a footprint-ish hull/primitive) so the
    HISM instances can block the Pawn channel and carve the navmesh — FLAG any obstacle mesh that ships with
    NO simple collision (or only complex/per-poly) so build-master/programmer can add a simple hull or pick a
    different mesh; prefer trunk/base-footprint hulls, not full-canopy. GRASS needs no collision. Deliver this
    as a per-layer PATH LIST (with the collision note per obstacle mesh) + recommended per-layer density/scale
    ranges in handoffs/TASK-135.md — build-master transcribes it into DA_BattlefieldScatter (TASK-137); you do
    NOT touch the DataAsset (its class may not be compiled yet).
    (3) AUTHOR the grass GROUND material `M_BattlefieldGround` (Content/Materials/): §6 stylized grass, NO flat
    single color (macro variation required; slope/dirt breakup allowed); MI hue variants in
    Content/Materials/Instances/ if useful. Build-master applies it to the scaled floor at TASK-137. Do NOT
    place anything in L_Arena and do NOT edit donors in place. ACCEPTANCE: a curated, documented per-layer mesh
    path list (variety, low-poly-preferred, origins noted); `M_BattlefieldGround` imported + §6-compliant;
    donors untouched. Art skips QA → build. Post the handoff in 🎨 Art (`🎨 ART-DIRECTOR: … TASK-135`).
- names: >
    Curated donor mesh SOFT-REFERENCES (in place, READ-ONLY) for layers TREES/ROCKS/HILLS/GRASS — paths
    documented in handoffs/TASK-135.md (fed into DA_BattlefieldScatter at TASK-137). Prefer
    /Game/Tree_Pack_1/…/SM-Mobile_Tree_* for trees; grass from Realistic_Grass_and_plant / Megaplant_Library;
    rocks from Realistic_Rocks / Fab/Rocks/highpoly_rocks_free_download; hills from
    Fab/Stone_Hills_FREE/stone_hill. Material: `M_BattlefieldGround` (/Game/Materials/; MI variants in
    /Game/Materials/Instances/). Law: CONVENTIONS "Battlefield & procedural terrain (M6.5)".

#### TASK-136 — PART 1 integration: compile + move castles/nodes/anchors/PlayerStart to ±8000/±7200, widen ground+navmesh+boundary, PIE-verify + commit (build)
- assignee: build-master
- status: **integration-done, HELD FROM COMMIT** (2026-07-10) — compiled clean (DLL 23:31); castles→±8000, gold nodes→±7200, PlayerStart→-6800 (symmetric); RecastNavMesh RuntimeGeneration=Dynamic (verified: Ogre rerouted around a test cube); ground/navmesh/walls widened to ±9000 X/±2400 Y; full PIE match completed end-to-end (~85s, ~4x). NOT committed — held with TASK-137 for Jonathan visual approval. Handoff: handoffs/TASK-136.md.
- blocked-by: TASK-133 (qa-passed), TASK-134 (qa-passed)
- parallel-safe: no (single editor + Git)
- spec: >
    Build-master integration, PART 1 (4× castle spacing) + the M6.5 code compile. Needs the editor MCP up
    (else park + tell the orchestrator). Per CONVENTIONS "World axes (arena contract)" (M6.5-updated).
    (1) COMPILE TASK-133 + TASK-134 C++ via the standard Build.bat editor-bounce (both scans first); any error
    → append to the offending task's QA report, qa-failed, stop (counts as a QA loop; NEVER edit code). This
    compile makes `USiegeScatterConfig` / `ASiegeBattlefieldScatter` exist in the editor (unblocks TASK-137).
    (2) SCENE MOVE in L_Arena (keep BOTH castles symmetric about X=0): Castle_Blue/CastleAnchor_Blue →
    (−8000,0), Castle_Red/CastleAnchor_Red → (+8000,0); GoldNode_Blue → (−7200,0), GoldNode_Red → (+7200,0)
    (800 in front of each castle); PlayerStart → ≈(−6800,0,100) on solid ground near the Blue castle (verify
    the hero spawns cleanly, NOT inside the plinth keep-out). Do NOT change KillZ (−2000, vertical).
    (3) WIDEN the arena to contain the 16000-wide field: SCALE/resize the arena floor slab so it covers the
    full play area with margin; resize the NavMeshBoundsVolume to match and REBUILD the navmesh; move the 4
    ArenaBoundary walls outward (past ±8000) so the hero still cannot leave. (Grass material + scatter come in
    TASK-137 — PART 1 just makes the enlarged, navigable, bounded field.)
    (3b) **DYNAMIC NAVMESH (MANDATORY for the M6.5 blocking obstacles — Jonathan's decision #1):** set
    `L_Arena`'s `RecastNavMesh` `RuntimeGeneration = Dynamic` (project Navigation setting / the nav-mesh actor)
    so TASK-134's runtime-scattered obstacles actually carve the navmesh — WITHOUT this, units walk straight
    through them. VERIFY the dynamic path: drop ONE temporary blocking test cube in the lane during PIE,
    confirm the navmesh carves around it AND a unit reroutes (GDI screenshot / nav readback), then REMOVE the
    test cube. (The real scatter + full obstacle-routing verify is TASK-137; this proves the nav CONFIG works
    before scatter integration.)
    (4) PIE-VERIFY (GDI screenshots + logs): arena boots; hero spawns near the Blue castle and can walk; the
    bot spawns + plays (units path from the Red side toward the Blue castle across the widened field — the ~4×
    longer march is EXPECTED, Jonathan's explicit ask); miners still reach their (moved) gold node and earn;
    navmesh valid end-to-end (no unreachable gaps); nothing falls through the floor; Play Again still resets.
    Record best-effort timings. COMMIT PART 1 + the M6.5 code on `main` (task-ID message), NOT pushed. Post
    compile + PIE observations + the dynamic-nav verify + hash in 🔧 Build & Git (`🔧 BUILD-MASTER: … TASK-136`).
- names: >
    Move (symmetric about X=0): Castle_Blue/CastleAnchor_Blue(−8000,0), Castle_Red/CastleAnchor_Red(+8000,0),
    GoldNode_Blue(−7200,0), GoldNode_Red(+7200,0), PlayerStart≈(−6800,0,100). Scale arena floor +
    NavMeshBoundsVolume (rebuild navmesh) + move 4 ArenaBoundary walls past ±8000. **Set RecastNavMesh
    RuntimeGeneration = Dynamic on L_Arena + verify a test blocker reroutes a unit.** KillZ −2000 UNCHANGED.
    Compile TASK-133+134 (Build.bat editor-bounce). Commit on `main`, NOT pushed. Law: CONVENTIONS "World axes
    (arena contract)" + "Battlefield & procedural terrain (M6.5)" (dynamic-nav rule).

#### TASK-137 — PART 2 integration: populate DA_BattlefieldScatter, place BattlefieldScatter, apply M_BattlefieldGround, verify randomized scatter + perf + commit (build)
- assignee: build-master
- status: **core-done, PENDING JONATHAN VISUAL + COMMIT** (2026-07-10/11) — DA_BattlefieldScatter populated (7 layers ~3400 instances), BP_BattlefieldScatter placed in L_Arena, M_BattlefieldGround applied. Traversability guarantee CONFIRMED across 3 fresh seeds; re-scatter proven. OPEN: (a) live populated-battlefield visual + real FPS need Jonathan foreground (blocked while he games — HISM scatter is PIE-runtime-only, un-capturable headlessly); (b) TUNING: ±2400 field is narrow → 14-69 blocking instances culled/seed (holds, but center reads sparse — widen field or fewer/smaller hills); (c) held PART1+PART2 scoped commit pending his approval. Handoff: handoffs/TASK-137.md.
- blocked-by: TASK-135 (ready-for-integration), TASK-136 (compiled config class + enlarged arena)
- parallel-safe: no (single editor + Git)
- spec: >
    Build-master integration, PART 2 (procedural grassy battlefield). Needs the editor MCP up (else park +
    tell the orchestrator). Per CONVENTIONS "Battlefield & procedural terrain (M6.5)".
    (1) CREATE + POPULATE `/Game/Data/DA_BattlefieldScatter` (a USiegeScatterConfig instance) from TASK-135's
    handoff: fill the TREES/ROCKS/HILLS/GRASS layers with the curated donor mesh soft-refs + the recommended
    per-layer density/scale/spacing/region params. Keep the obstacle layers (trees/rocks/hills) `bBlocking=true`
    and GRASS `bBlocking=false`; `bMirrorSymmetric=false` (ASYMMETRIC organic random — Jonathan's ruling).
    Apply any per-mesh origin offsets TASK-135 flagged; for any obstacle mesh TASK-135 flagged as lacking
    simple collision, resolve it (simple hull / substitute) so blocking + nav-carve actually works.
    (2) PLACE ONE `ASiegeBattlefieldScatter` actor in L_Arena named `BattlefieldScatter`; assign
    ScatterConfig = DA_BattlefieldScatter. APPLY `M_BattlefieldGround` to the arena floor slab (the one scaled
    in TASK-136). (L_Arena's RecastNavMesh is already RuntimeGeneration Dynamic from TASK-136.)
    (3) PIE-VERIFY (GDI screenshots + LogSiegeTerrain): at match start the field fills with trees/rocks/hills/
    grass across the widened arena; KEEP-CLEAR holds (nothing on castle pads / gold-node pads / PlayerStart /
    the central lane). **BLOCKING obstacles (Jonathan's decision #1):** the obstacles carve the navmesh and
    units visibly ROUTE AROUND them (not through) — confirm in the pixels + nav readback. **TRAVERSABILITY
    GUARANTEE (HARD GATE):** across SEVERAL matches (re-scatter via Play Again a few times — each a new seed),
    units ALWAYS reach the enemy castle — never walled off, trapped, or spawn-blocked; if ANY match strands
    units, that is a HARD FAILURE → route back to gameplay-programmer (the TASK-134 reachability validation is
    incomplete; counts as a QA loop). A SECOND match yields a DIFFERENT layout (LogSiegeTerrain seed changed +
    screenshots differ). Take a GDI screenshot and CONFIRM the pixels read as a grassy field with rocks/trees/
    hills (NOT a bare board). **FPS CHECK (heightened — blocking + highpoly + dynamic-nav rebuild is costly):**
    record best-effort FPS at full scatter; if it drops below the §6 budget, FLAG that per-layer instance
    counts must come down (config) — the §6 60 fps gate is a human WATCH at Jonathan's playtest. If a mesh
    renders wrong (floating/sunk/huge) → adjust the config/offset (DATA), not code.
    (4) COMMIT PART 2 (DA_BattlefieldScatter, the L_Arena scatter actor + ground-material application,
    M_BattlefieldGround) on `main` (task-ID message), NOT pushed; cut a `m6.5-testable` branch at the commit
    (milestone-preservation workflow; not pushed). Post the SCREENSHOT OBSERVATION + obstacle-routing +
    traversability result + randomization proof + FPS + hash + branch in 🔧 Build & Git (`🔧 BUILD-MASTER: … TASK-137`).
- names: >
    `/Game/Data/DA_BattlefieldScatter` (USiegeScatterConfig instance) populated from handoffs/TASK-135.md:
    obstacle layers bBlocking=true / grass bBlocking=false / bMirrorSymmetric=false. Level actor
    `BattlefieldScatter` (ASiegeBattlefieldScatter) in L_Arena, ScatterConfig=DA_BattlefieldScatter. Apply
    `M_BattlefieldGround` to the arena floor. HARD GATE: castle-to-castle traversability holds every match +
    units route around obstacles. Branch `m6.5-testable`, commit on `main`, NOT pushed. Law: CONVENTIONS
    "Battlefield & procedural terrain (M6.5)".

---

### M5.5 — Overhead health bars (TASK-110..112) — decomposed 2026-07-09

**Authorization:** Jonathan's 2026-07-09 direct Claude Code directive — "add a health bar to every tower and character." A small standalone batch (M4.5 shape), NOT a GDD milestone. Naming law added to CONVENTIONS "Overhead unit health bars (M5.5)" BEFORE task issue. Hard gate stands: engine/MCP work needs the editor MCP (127.0.0.1:8000) up — if unreachable, park the editor task and tell the orchestrator (never fake results). This batch does NOT touch M5's or M4.5's parked work; M6 decomposition is a SEPARATE step after this ships.

**DESIGN RECONCILIATION — GDD §7 amendment (manager ruling, binding for this batch; the actual GDD-file edit is Jonathan's call, mirroring how M4.5 "amends GDD §5" via a board ruling):**
GDD §7 today reads "…both castles' HP bars (top). Enemy hero/units show floating health bars when damaged." Jonathan's directive broadens this. Recommended §7 wording: *"Every combat actor — friendly and enemy alike — shows a floating overhead health bar: the hero(es), all summoned units (including miners), and all buildings (towers, walls, Barracks, Deep Mine). The bar is hidden at full HP and appears once the actor has taken damage (damage-triggered, hide-at-full), tinted by team (blue friendly / red enemy). Castles keep their existing top-of-mesh bars (§3.9)."*

**M5.5 manager decisions (binding for all M5.5 tasks; each call FLAGGED for QA + Jonathan):**
1. **Scope — FLAG 1.** In: ALL `ASummonedUnit` (incl. `AMinerUnit`), ALL `ABuilding` (Arrow/Bomb/Ballista/Crystal Tower + Wall + Barracks + Deep Mine), and `AHeroCharacter` — BOTH teams. Non-combat buildings (Wall/Barracks/Deep Mine) ARE included: they all have HP and are destroyable, the system attaches at the `ABuilding` base so they come for free, and hide-at-full keeps an untouched building clutter-free; excluding them would need a per-subclass opt-out for less consistency. A per-actor `bShowHealthBar` opt-out (EditDefaultsOnly, default true) is provided so a playtest can suppress a specific type (e.g. miners) with zero code. OUT: `ACastle` (keeps its M1 delegate bar — do NOT duplicate) and `AGoldNode` (not damageable, no HP). **Enemy hero:** the code is team-agnostic and covers ALL `AHeroCharacter` instances; TODAY exactly one hero exists (the Blue player's — the Red bot is card-only, no hero pawn: `ASiegeGameMode::SpawnBot` spawns only an `ASiegeBotController` + Red PlayerState), so §7's "enemy hero" is auto-covered if/when one is ever spawned (M8). Recommendation: widen §7 to friendly+enemy, towers+buildings included — ACCEPT.
2. **Persistent vs damage-triggered — FLAG 2.** Rule: **hide-at-full** — the bar is visible only while the actor is alive AND `CurrentHP < MaxHP − epsilon`; hidden at full HP and on death/destruction. Rationale: §7 says "when damaged," and always-on bars on 60+ actors (§6 perf budget) clutter the screen and add overdraw; hide-at-full shows a bar exactly when it carries information. Recommend over always-on — ACCEPT.
3. **Reuse — do NOT reinvent (existing infra audited).** Castle bar EXISTS: `ACastle::HPBarWidget` (`UWidgetComponent`) + `UCastleHealthBarWidget`/`WBP_CastleHealthBar`, driven by the `FOnCastleHPChanged` delegate (seed-then-bind, M1 TASK-018/019) — LEFT UNTOUCHED. Hero's OWN HP already shows on `WBP_HUD` (M1) — that stays; the new overhead bar is additive. **Key reuse fact:** units/buildings/hero already expose BlueprintPure `GetCurrentHP()`/`GetMaxHP()` and `ITeamAgent::GetTeamId()` — the new code binds to those getters, adds NO new HP fields. **Key non-reuse fact:** units/buildings/hero carry NO HP-changed delegate (only ACastle does — ASummonedUnit::ApplyHealing comment), so the new bar POLLS the getters (~0.15 s timer) rather than the castle's bind pattern. Donor for the new widget: duplicate `WBP_CastleHealthBar` (already a `UI_LifeBar` duplicate with the ProgressBar+OnHPChanged wiring) — never author a tree from scratch (template-donor rule).
4. **Team color — FLAG 4.** Rule: tint the bar FILL by team (blue friendly / red enemy) from the §6 palette / `MI_TeamColor` linear values, pushed as data through `SetTeamColor(float R,float G,float B)` — never hardcoded in logic, tunable via the component's `BlueBarColor`/`RedBarColor`. Fixed regardless of HP (bar communicates team by color, damage by length; no green→red recolor, which would fight the team signal). Recommend team-colored fill over a fixed fill + colored border (fill reads at a glance on 60 units) — ACCEPT.
5. **Tech approach — FLAG 5.** WidgetComponent-per-actor (the proven `ACastle` pattern), Screen-space. ONE reusable `UHealthBarComponent` (`UWidgetComponent` subclass) added in-constructor to the three base classes (named `HPBarWidget`), polling HP through the shared `IHealthBarTarget` interface and driving `WBP_UnitHealthBar` (`UUnitHealthBarWidget` base) via float-only BIEs. Chosen over a HUD-drawn pooled approach: reuses shipped infra (castle WidgetComponent), attaches once per base class (miners/towers inherit free), and MCP can author the UMG tree (proven TASK-041 CardHand; TASK-019 WBP_CastleHealthBar from UI_LifeBar). Recommend WidgetComponent path — ACCEPT. Full names in CONVENTIONS "Overhead unit health bars (M5.5)".
6. **Optional rider (NOT forced):** the Standing-backlog HUD overtime-indicator one-pin fix lives in `WBP_HUD`; this batch touches `WBP_UnitHealthBar` (a DIFFERENT widget), so it does NOT naturally fold in — leave it in the backlog for the balance/HUD pass. Do not add it to this chain.
7. **Perf (§6) — WATCH.** No machine fps route exists (learnings); hide-at-full + the `bShowHealthBar` opt-out cap the live-bar count. TASK-112 records best-effort observations; the formal 60 fps @1440p / 60+ units gate is a human WATCH at Jonathan's playtest.

**M5.5 exit criteria (playable slice):** damage any friendly unit, tower, wall, Barracks, Deep Mine, miner, and the hero, and each shows a floating overhead bar that (a) is HIDDEN at full HP, appears on first damage, and tracks HP down live; (b) is tinted by team (blue for friendly, and — via the Red bot's units/towers — red for enemy); (c) hides again on death/destruction; (d) never appears on castles as a duplicate (castle keeps its own bar) or on gold nodes. Both teams verified in one Play-vs-Bot session. Recordable: overhead-bar clip across unit + tower + hero on both teams.

Dispatch shape: **SERIAL chain** (the WBP reparents to the new C++ base, so it can't precede the compile) — only TASK-110 starts immediately. TASK-110 (C++) → qa-reviewer (shadow-scan mandatory) → build-master compiles TASK-110 so `UUnitHealthBarWidget` exists in the editor → TASK-111 (art authors WBP_UnitHealthBar against the compiled base) → TASK-112 (final assemble + PIE-verify the exit criteria + commit). Nothing here is parallel-safe within the batch.

#### TASK-110 — Overhead health-bar system: interface + UHealthBarComponent + UUnitHealthBarWidget base, wired onto units/buildings/hero (C++)
- assignee: gameplay-programmer
- status: qa-passed + compiled-clean (2026-07-09; logic PASS + build-fix loop 1 [CapsuleComponent.h include] compiled clean at TASK-112 phase-A re-run — 0 err/0 warn/0 C4458, UHealthBarComponent + UUnitHealthBarWidget verified live in editor via reflection. Code commit rides TASK-112 phase B)
- blocked-by: none
- parallel-safe: no (edits shared base headers ASummonedUnit/ABuilding/HeroCharacter; nothing else in this batch runs until it compiles)
- spec: >
    Files only — NO editor/MCP, NO new HP fields (bind the existing getters). Deliver the overhead
    health-bar system for units, buildings, and the hero per CONVENTIONS "Overhead unit health bars
    (M5.5)" and the M5.5 rulings. (1) NEW header-only `HealthBarTarget.h`: `IHealthBarTarget`
    (`UHealthBarTarget` UINTERFACE, NotBlueprintable) — three pure-virtual const methods (the
    ITeamAgent C++-interface shape, NOT BlueprintNativeEvent): `float GetHealthCurrent() const`,
    `float GetHealthMax() const`, `bool IsHealthBarActorAlive() const`. (2) Implement `IHealthBarTarget`
    on `ASummonedUnit` (→ GetCurrentHP / GetMaxHP / !IsUnitDead), `ABuilding` (→ GetCurrentHP /
    GetMaxHP / !IsBuildingDestroyed), `AHeroCharacter` (→ GetCurrentHP / GetMaxHP / !IsDead) — add
    NOTHING to ACastle or AGoldNode. (3) NEW `UHealthBarComponent` (`UWidgetComponent` subclass,
    HealthBarComponent.h/.cpp): a poll-driven overhead bar. On BeginPlay/register: soft-resolve
    `HealthBarWidgetClass` (default /Game/UI/WBP_UnitHealthBar, null-safe — missing = silent no bar,
    log once, never crash), set widget space = Screen + DrawSize ~90×12 + relative Z = BarHeightZ,
    read the owner as `IHealthBarTarget` (+ `ITeamAgent` for team), push `SetTeamColor` ONCE from
    BlueBarColor/RedBarColor, and start a repeating `PollInterval` timer. Each poll: if `!bShowHealthBar`
    or actor not alive or Current >= Max−epsilon → HIDE the widget; else SHOW it and call
    `OnHPChanged(Current, Max)`. EditDefaultsOnly tunables exactly per CONVENTIONS (HealthBarWidgetClass,
    PollInterval=0.15, bShowHealthBar=true, BarHeightZ=120, BlueBarColor/RedBarColor = palette linear
    values). (4) NEW `UUnitHealthBarWidget` (`UUserWidget` subclass, UnitHealthBarWidget.h/.cpp): two
    BlueprintImplementableEvents, FLOAT PARAMS ONLY — `OnHPChanged(float CurrentHP, float MaxHP)` and
    `SetTeamColor(float R, float G, float B)`. (5) Add exactly one `UHealthBarComponent` named
    `HPBarWidget` in the CONSTRUCTOR of ASummonedUnit, ABuilding, and AHeroCharacter (subclasses inherit
    it). Do NOT touch ACastle's existing HPBarWidget. Everywhere null-safe; zero behavior change to
    combat/stats. ACCEPTANCE: compiles warnings-as-errors; the three base classes each own one
    HPBarWidget; poll show/hide + team-tint logic present; no new HP field introduced. → qa-reviewer
    (MANDATORY shadow-scan: no inherited-reflected-member shadow — CONVENTIONS coding law; watch
    `Owner`/`Instigator`/`Slot`). Post progress/handoff in ⚙️ Dev & QA (`⚙️ GAMEPLAY-PROGRAMMER: … TASK-110`).
- names: >
    Interface `IHealthBarTarget` / `UHealthBarTarget` in Source/GitClaudeUnrealTest/Siegebound/HealthBarTarget.h;
    methods GetHealthCurrent() / GetHealthMax() / IsHealthBarActorAlive().
    Component `UHealthBarComponent` (UWidgetComponent subclass), HealthBarComponent.h/.cpp; instance name `HPBarWidget`.
    Component props: HealthBarWidgetClass (TSoftClassPtr<UUserWidget>, default /Game/UI/WBP_UnitHealthBar),
    PollInterval (0.15), bShowHealthBar (true), BarHeightZ (120), BlueBarColor (0.05,0.30,1.00), RedBarColor (1.00,0.10,0.05).
    Widget base `UUnitHealthBarWidget` (UUserWidget subclass), UnitHealthBarWidget.h/.cpp; BIEs
    OnHPChanged(float CurrentHP, float MaxHP) and SetTeamColor(float R, float G, float B).
    UMG asset (TASK-111): `WBP_UnitHealthBar` at /Game/UI/WBP_UnitHealthBar.
    Reuse only (do NOT redefine): ASummonedUnit/ABuilding/AHeroCharacter GetCurrentHP()/GetMaxHP(),
    IsUnitDead()/IsBuildingDestroyed()/IsDead(), ITeamAgent::GetTeamId().

#### TASK-111 — WBP_UnitHealthBar UMG asset: duplicate donor, reparent to UUnitHealthBarWidget, wire fill + tint (art)
- assignee: art-director
- status: ready-for-integration (2026-07-09; handoffs/TASK-111.md — /Game/UI/WBP_UnitHealthBar duplicated from WBP_CastleHealthBar, reparent to UUnitHealthBarWidget READBACK-CONFIRMED [the crux WARN — closed], both float BIEs bIsImplemented=true [OnHPChanged→SetPercent guard Max>0; SetTeamColor→fill tint], compact HitTestInvisible overhead bar sized to the 90×12 DrawSize, no baked text. Integration check + commit ride TASK-112 phase B)
- blocked-by: TASK-110 (reparents to UUnitHealthBarWidget — needs the class compiled; the orchestrator has build-master compile TASK-110 before dispatching this, the M1 TASK-018→019 order)
- parallel-safe: no
- spec: >
    Editor/MCP only — needs the editor MCP up (else park + tell the orchestrator). Author the
    `WBP_UnitHealthBar` UMG widget per CONVENTIONS "Overhead unit health bars (M5.5)". (1) DUPLICATE
    `/Game/UI/WBP_CastleHealthBar` (the approved health-bar donor — itself a UI_LifeBar duplicate,
    already carrying a ProgressBar + an OnHPChanged handler) to `/Game/UI/WBP_UnitHealthBar`; never
    author the tree from scratch (template-donor rule). (2) REPARENT it to `UUnitHealthBarWidget`
    (compiled in TASK-110). (3) Implement the `OnHPChanged(float CurrentHP, float MaxHP)` BIE as
    ProgressBar `SetPercent(CurrentHP / MaxHP)` guarding MaxHP > 0 (same as the castle donor — keep it).
    (4) Implement `SetTeamColor(float R, float G, float B)` to tint the ProgressBar FILL brush from the
    pushed RGB (make a small compact bar — thin, dark/translucent background track, so it reads at ~150
    px; no baked text). Keep it HitTestInvisible. ACCEPTANCE: /Game/UI/WBP_UnitHealthBar exists,
    parent = UUnitHealthBarWidget, both BIEs implemented (fill percent + team tint), compact overhead
    size. Art skips QA → build-master integration check (TASK-112). Post the handoff in 🎨 Art
    (`🎨 ART-DIRECTOR: … TASK-111`) with the asset path.
- names: >
    `WBP_UnitHealthBar` at /Game/UI/WBP_UnitHealthBar, parent class `UUnitHealthBarWidget`.
    Donor: /Game/UI/WBP_CastleHealthBar (fallback /Game/Variant_Combat/UI/UI_LifeBar).
    BIEs to implement: OnHPChanged(float CurrentHP, float MaxHP) → ProgressBar SetPercent(guard Max>0);
    SetTeamColor(float R, float G, float B) → fill brush tint.

#### TASK-112 — M5.5 integration: compile, overhead-bar PIE verification, commit (build)
- assignee: build-master
- status: done (2026-07-09; handoffs/TASK-112.md — phase A compile clean [0 err/warn/C4458]; phase B PIE property-verification in live Play-vs-Bot: hide-at-full ✅, appears-on-damage+fill ✅ [hero 44%], blue tint ✅, red wiring ✅, castle own-bar-only ✅, gold node none ✅; QA WARN closed functionally [live WidgetClass=WBP_UnitHealthBar_C resolved, reparent cast non-null → fill fills + tint applies]. Commit 9a8a75f on main [12 files, code + WBP], NOT pushed, no branch. Desktop LOCKED → WATCH [Jonathan playtest]: literal on-screen bar pixels [Slate widgets uncapturable headless], live show-on-damage for Blue units + all buildings [no card input on locked box], §6 60fps. Follow-up to manager: a debug-exec cheat [ApplyTestDamage/summon-Blue] would make headless verification complete)
- blocked-by: TASK-110 (qa-passed) + TASK-111 (ready-for-integration)
- parallel-safe: no
- spec: >
    Integration for the health-bar batch (TASK-090/109 pattern). TWO-PHASE, single owner: (phase A —
    already done to unblock TASK-111) compile TASK-110 (editor-bounce) so `UUnitHealthBarWidget`
    /`UHealthBarComponent` exist; (phase B — this task) after TASK-111's WBP lands: re-verify the build
    compiles clean warnings-as-errors, then run the M5.5 exit-criteria PIE suite in L_Arena via a
    Play-vs-Bot session — damage a friendly unit, a tower, a Wall, Barracks, Deep Mine, a miner, and the
    hero and confirm each shows a floating overhead bar that is HIDDEN at full, appears on first damage,
    tracks HP down, is team-tinted (BLUE friendly; and via the Red bot's units/towers RED enemy), and
    hides on death/destruction; confirm castles still show ONLY their own bar (no duplicate) and gold
    nodes show none. Record best-effort perf observations (§6 WATCH — no machine fps route). If the
    editor MCP is down, compile via Build.bat and report the PIE items as owed-to-Jonathan (never fake).
    On PASS: commit code + WBP_UnitHealthBar with message "TASK-110..112: overhead health bars on units/
    towers/hero — hide-at-full, team-tinted; §7 widened". Do NOT push. Build failure → append errors to
    the offending task's qa report and route back to gameplay-programmer (counts as a QA loop). Post
    compile result + commit hash in 🔧 Build & Git (`🔧 BUILD-MASTER: … TASK-112`).
- names: >
    Assets/classes exactly as TASK-110/111 names blocks. Commit on `main`, no push. No new branch
    (this is a batch, not a milestone slice — m5-testable already preserves M5).

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
- status: done (2026-07-08; handoffs/TASK-109.md — desktop LOCKED so machine-only verification. Commit 979f552 on main [53 files, TASK-104..109], branch m5-testable cut at it, NOTHING pushed [main ahead 2: 2c65164 + 979f552]. VERIFIED machine: DT_Cards 28 rows + deferred CardArt check closed, 8/8 Set III assets at exact paths, CrystalTower BP loads as ATower subclass, PIE booted L_Arena, bot rules 4/5 fired with renumbered labels + economy/waves unregressed, StopPIE clean, log sweep = knowns only, staged==worktree (L_Arena correctly excluded). WATCH [locked desktop, needs human playtest]: all 5 live spell casts + acceptance, Crystal Tower chain, reticle project/confirm/cancel, "bot casts spells" [3a/3b need player-side targets an idle player never made; MCP can't inject into live GWorld w/o polluting L_Arena], Play Again spell reset. Match self-ended ~71s [bot razed undefended Blue castle]. Benign finding: stale LogCSVImportFactory CardType warnings [exactly why 104 uses set_rows] — CONVENTIONS one-liner to manager)
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
