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

1. **M1 — Core loop, local, one card** — `feedback-in-progress` (exit criteria passed 2026-07-03; reopened same day for playtest round-1 combat-legibility fixes → TASK-016..020; stays open until Jonathan confirms round 2)
2. **M2 — Economy + deck/hand + core set + defenses** — `decomposed — file tasks executing, editor tasks gated on round-2 sign-off` (TASK-021..040; gates + M2a/M2b sequencing in "M2 manager decisions" below)
3. M3 — Bot opponent = real 1v1 match — `not-started`
4. M4 — Card Set II (16 cards, keywords, hero upgrades) — `not-started`
5. M5 — Spell system + Set III — `not-started`
6. M6 — Deck-builder meta — `not-started`
7. M7 — Premium art & feel pass — `not-started`
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

### TASK-016 — Hero attack feedback hooks (C++)
- assignee: gameplay-programmer
- status: done (commit f6fa7ba; qa/TASK-016-report.md PASS)
- blocked-by: none
- parallel-safe: yes
- spec: >
    Files only, no editor. Playtest R1 finding 1. In AHeroCharacter add UPROPERTYs (EditAnywhere,
    Category "Combat|Feedback"): TObjectPtr<UAnimMontage> AttackMontage; FName AttackMontageSection
    (default NAME_None); TObjectPtr<UNiagaraSystem> HitImpactEffect; TSubclassOf<UCameraShakeBase>
    HitCameraShake. All left unset in C++ (wired in TASK-017); EVERY use null-safe — code must compile
    and behave with nothing assigned (M1 house style). Behavior: (1) on every melee swing that passes the
    0.5 s cooldown — hit OR whiff — and only when melee is NOT suppressed (placement mode), call
    PlayAnimMontage(AttackMontage) and jump to AttackMontageSection if set; the montage is VISUAL ONLY —
    damage timing/numbers stay exactly as M1 (20 dmg, 150 units, 60-degree cone, 0.5 s cooldown), never
    gate damage on anim notifies. (2) For each enemy actually damaged, spawn HitImpactEffect at the closest
    point on that target's collision to the hero (fallback: target GetActorLocation) via
    UNiagaraFunctionLibrary::SpawnSystemAtLocation. (3) If >=1 enemy was damaged this swing,
    ClientStartCameraShake(HitCameraShake) on the local PlayerController. (4) Add "Niagara" to
    Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs dependency modules — this task OWNS that
    Build.cs edit (TASK-020 is serialized behind it for exactly this file). Acceptance: compiles and runs
    clean with nothing wired; swing/damage behavior byte-identical to M1 incl. suppression; with TASK-017's
    assets wired, every swing plays the montage and every damaging hit spawns the effect + shake.
- names: >
    AHeroCharacter (Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h/.cpp) — UPROPERTYs AttackMontage,
    AttackMontageSection, HitImpactEffect, HitCameraShake. Build.cs:
    Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs (add Niagara). Donor assets wired in TASK-017
    (exact, READ-ONLY): /Game/Variant_Combat/Anims/AM_ComboAttack (or AM_ChargedAttack),
    /Game/Variant_Combat/VFX/NS_Damage, /Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy.

### TASK-017 — Wire hero attack feedback + LMB real-input verification (editor)
- assignee: gameplay-programmer
- status: ready-for-integration (PIE-verified 2026-07-03; handoffs/TASK-017.md — all 4 props were already wired by the interrupted session, this run verified: montage AM_ComboAttack sec Melee01 on ABP_Unarmed, −20 HP/swing, NS_Damage + camera shake fire, 12/12 real-path LMB clicks land, no click-swallow found)
- blocked-by: TASK-016 (integrated + compiled)
- parallel-safe: no
- spec: >
    Editor/MCP work. (1) On /Game/Blueprints/BP_HeroCharacter assign: AttackMontage =
    /Game/Variant_Combat/Anims/AM_ComboAttack — use AM_ChargedAttack instead if it reads better as ONE
    swing inside the 0.5 s cooldown; set AttackMontageSection so exactly one swing section plays; record
    the choice in the handoff. HitImpactEffect = /Game/Variant_Combat/VFX/NS_Damage. HitCameraShake =
    /Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy. All Variant_Combat assets are READ-ONLY
    donors — reference, never edit (CONVENTIONS template-donor rule). (2) Montage playback path: first try
    keeping anim class ABP_Unarmed and verify the montage VISIBLY plays in PIE (its slot must exist in
    ABP_Unarmed's graph). If it does not play, set BP_HeroCharacter AnimClass =
    /Game/Variant_Combat/Anims/ABP_Manny_Combat and re-verify walk/sprint/jump locomotion AND montage AND
    no per-frame cast/error spam in the log. If BOTH fail, stop and write findings to the handoff for
    manager re-spec — do NOT edit any Variant_* asset. (3) LMB click-swallow investigation (user reports
    LMB "does nothing" on real hardware; MCP injection applied damage — verify, don't assume): a) WBP_HUD
    root and panels are Not Hit-Testable (Self Only), only the card button hit-testable; b) input mode is
    GameOnly with bShowMouseCursor false outside placement mode, including after PlayAgain; c) LMB maps
    only to IA_Attack in IMC_Hero (no competing consuming mapping). Fix what is broken; record findings —
    even "nothing found" — in handoffs/TASK-017.md. (4) PIE verify via MCP injection: LMB swings play the
    montage visibly, Red castle drops 20/swing, impact effect appears at the hit point, shake fires.
    Acceptance: all of (4) pass + handoff documents the montage path chosen and the click-swallow findings;
    real-hardware confirmation is explicitly deferred to user playtest round 2.
- names: >
    /Game/Blueprints/BP_HeroCharacter. Donors (exact, read-only): /Game/Variant_Combat/Anims/AM_ComboAttack,
    AM_ChargedAttack, ABP_Manny_Combat; /Game/Variant_Combat/VFX/NS_Damage;
    /Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy. Inspected: /Game/UI/WBP_HUD,
    /Game/Input/IMC_Hero, /Game/Input/Actions/IA_Attack.

### TASK-018 — Castle HP delegate + health-bar widget component (C++)
- assignee: gameplay-programmer
- status: done (commit f6fa7ba; qa/TASK-018-report.md PASS)
- blocked-by: none
- parallel-safe: yes
- spec: >
    Files only, no editor. Playtest R1 finding 2. (1) ACastle: declare dynamic multicast delegate
    FOnCastleHPChanged(float CurrentHP, float MaxHP); UPROPERTY(BlueprintAssignable) OnCastleHPChanged.
    Broadcast on every ACTUAL CurrentHP change (after applying damage in TakeDamage — NOT on ignored
    friendly-fire damage), in ResetCastle, and once at BeginPlay (seed). Add BlueprintPure float
    GetCurrentHP() / GetMaxHP(). (2) ACastle: UWidgetComponent "HPBarWidget" attached to root — Space =
    Screen, DrawSize 256x32, relative location (0,0,1050) (castle mesh is 900 tall); widget class resolved
    null-safe at BeginPlay from a TSoftClassPtr<UUserWidget> defaulting to
    /Game/UI/WBP_CastleHealthBar.WBP_CastleHealthBar_C (asset arrives in TASK-019 — a missing asset is a
    silent no-op, never a crash). Hide the component when the castle is destroyed; show it again in
    ResetCastle. (3) New class UCastleHealthBarWidget : UUserWidget in CastleHealthBarWidget.h/.cpp:
    UFUNCTION BlueprintCallable InitForCastle(ACastle*) — seeds by calling OnHPChanged(GetCurrentHP(),
    GetMaxHP()) immediately, THEN binds OnCastleHPChanged (seed-then-bind, per qa/TASK-005-report.md
    major 2); UFUNCTION BlueprintImplementableEvent OnHPChanged(float CurrentHP, float MaxHP) — float
    params only, MCP cannot author enum BP params. ACastle BeginPlay: if HPBarWidget's user widget is a
    UCastleHealthBarWidget, call InitForCastle(this). Verify "UMG" is already in Build.cs (it is, from M1
    widgets) — do not touch Build.cs otherwise (TASK-016 owns the Niagara edit). Acceptance: compiles and
    runs with no widget asset present; 3 enemy hits = exactly 3 broadcasts with correct values; friendly
    damage = 0 broadcasts; destroyed -> bar hidden; ResetCastle -> broadcast(2000,2000) + bar visible.
- names: >
    ACastle (Source/GitClaudeUnrealTest/Siegebound/Castle.h/.cpp) — delegate FOnCastleHPChanged, property
    OnCastleHPChanged, component HPBarWidget, getters GetCurrentHP/GetMaxHP. UCastleHealthBarWidget in
    Source/GitClaudeUnrealTest/Siegebound/CastleHealthBarWidget.h/.cpp — functions InitForCastle,
    OnHPChanged. Widget asset (exact, built in TASK-019): /Game/UI/WBP_CastleHealthBar.

### TASK-019 — WBP_CastleHealthBar from UI_LifeBar donor (editor)
- assignee: gameplay-programmer
- status: ready-for-integration (audit-and-finish complete 2026-07-03: orphan session had finished the widget correctly at 14:41 — audited state is SHA256-identical; all 6 PIE acceptance items PASS incl. zero Accessed None; donor UI_LifeBar verdict = merely resaved, NOT functionally altered [structural diff vs pristine template]; handoffs/TASK-019.md. CAUTION for build-master: donor likely dirty in editor MEMORY — never save-all; staged widget blob == worktree so unstage is conflict-free)
- blocked-by: TASK-018 (integrated + compiled)
- parallel-safe: no
- spec: >
    Editor/MCP work. DUPLICATE donor /Game/Variant_Combat/UI/UI_LifeBar -> /Game/UI/WBP_CastleHealthBar
    (MCP cannot author widget trees from scratch; never edit the donor). Reparent the duplicate to
    UCastleHealthBarWidget. Strip all template logic/bindings referencing Variant_Combat classes; keep the
    bar visuals. Implement event OnHPChanged(CurrentHP, MaxHP): ProgressBar SetPercent(CurrentHP / MaxHP),
    guard MaxHP > 0. If the donor has a numeric text block, bind it to "Current / Max" as ints; otherwise
    bar-only is fine. Placeholder styling acceptable (premium UI pass is M7). Acceptance (PIE in L_Arena):
    both castles show a full overhead bar at boot; hero swings on the Red castle lower its bar live in
    20-HP steps; footman attacks lower it in 12-HP steps; at 0 HP the bar disappears with the castle;
    Play Again -> both bars full and visible again.
- names: >
    /Game/UI/WBP_CastleHealthBar (parent UCastleHealthBarWidget; donor /Game/Variant_Combat/UI/UI_LifeBar,
    READ-ONLY).

### TASK-020 — Footman procedural attack lunge + impact VFX (C++)
- assignee: gameplay-programmer
- status: done (commit f6fa7ba; qa/TASK-020-report.md PASS)
- blocked-by: TASK-016 (shared Build.cs edit only — Niagara module lands there; no logic dependency)
- parallel-safe: yes
- spec: >
    Files only, no editor. Playtest R1 finding 3. Blockout-tier "attack animation" for the static-mesh
    footman — NO skeletal rig (M7). In ASummonedUnit: (1) each time the Attack state deals its cadence hit,
    run one lunge cycle on the VisualMesh component: offset its RELATIVE location along local +X (the
    capsule's local X is actor forward — do NOT use the mesh's own rotation; VisualMesh carries a -90 yaw
    import fix per handoffs/TASK-014.md) out AttackLungeDistance (default 40.0) and back, sine-eased, over
    AttackLungeDuration (default 0.3 s), clamped to 0.8 x Cadence. Cache the BP-authored base relative
    location once at BeginPlay (post-construction); drive offset as Base + f(elapsed); ALWAYS restore
    exactly Base at cycle end, on leaving the Attack state, and on death — zero drift after any number of
    cycles. UPROPERTYs (EditAnywhere, Category "Combat|Feedback"): float AttackLungeDistance = 40.f, float
    AttackLungeDuration = 0.3f. (2) On each damage application spawn AttackImpactEffect —
    TSoftObjectPtr<UNiagaraSystem> with C++ default /Game/Variant_Combat/VFX/NS_Damage.NS_Damage (READ-ONLY
    donor, referenced not edited) — at the closest point on the target's collision to the unit (fallback:
    target location); null-safe; resolve/cache once, no per-attack sync-load hitch. Do NOT touch Build.cs —
    TASK-016 owns the Niagara module edit. Acceptance: in PIE a footman attacking the Red castle visibly
    lunges toward it once per 1.0 s with a damage puff at the contact point; mesh sits at exact rest pose
    between hits and after 50+ attacks; correct for either team/facing; damage numbers/timing unchanged;
    stats still read from DT_Cards (nothing hardcoded).
- names: >
    ASummonedUnit (Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h/.cpp) — UPROPERTYs
    AttackLungeDistance, AttackLungeDuration, AttackImpactEffect; existing component VisualMesh.
    Donor (exact, read-only): /Game/Variant_Combat/VFX/NS_Damage.

---

## M2 tasks (decomposed 2026-07-03)

File tasks (TASK-021..030) dispatch NOW per the gates above; wave order from blocked-by: [021, 026] → [022, 027, 028] → [023, 024, 029] → [025, 030]. Editor/build tasks (031..036, 039, 040) wait for round-2 sign-off; Blender tasks (037, 038) also need Blender MCP.

### TASK-021 — Core-set card data: FCardRow columns + cards.csv rows (files)
- assignee: gameplay-programmer
- status: ready-for-qa (handoffs/TASK-021.md; 5 flagged decisions for QA ruling; DeckCount sums to 50, M1 Footman bytes preserved)
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
- status: backlog
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
- status: backlog
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
- status: backlog
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
- status: backlog
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
- status: in-progress (dispatched 2026-07-03 ~15:45, M2 wave 1)
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
- status: backlog
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
- status: backlog
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
- status: backlog
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
- status: backlog
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
- status: backlog
- blocked-by: round-2 sign-off; TASK-021; TASK-039 (FCardRow columns compiled)
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
- status: backlog
- blocked-by: round-2 sign-off; TASK-023; TASK-039 (controller slots compiled)
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
- status: backlog
- blocked-by: round-2 sign-off; TASK-029; TASK-031; TASK-039 (widget base + delegates compiled)
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
- status: backlog
- blocked-by: round-2 sign-off; TASK-031; TASK-037 (meshes); TASK-039 (AMinerUnit/ranged code compiled)
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
- status: backlog
- blocked-by: round-2 sign-off; TASK-031; TASK-038 (meshes); TASK-039 (ABuilding/ATower compiled)
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
- status: backlog
- blocked-by: round-2 sign-off; TASK-038 (SM_GoldNode imported); TASK-039 (AGoldNode compiled)
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
- status: backlog
- blocked-by: round-2 sign-off (import step); Blender MCP available
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
- status: backlog
- blocked-by: round-2 sign-off (import step); Blender MCP available
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
- status: backlog
- blocked-by: round-2 sign-off; TASK-021..030 all qa-passed
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
- status: backlog
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
