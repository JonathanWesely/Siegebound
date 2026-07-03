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

1. **M1 — Core loop, local, one card** — `feedback-in-progress` (exit criteria passed 2026-07-03; reopened same day for playtest round-1 combat-legibility fixes → TASK-016..020)
2. M2 — Economy + deck/hand + core set + defenses — `not-started`
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

### M1 playtest feedback — round 1 (2026-07-03)
User verdict: core loop works, but combat is illegible. Three findings → five tasks (no new art; all visuals reuse template Variant_Combat donors):
1. **Hero LMB shows no response** — no swing animation, no hit feedback; player cannot tell the castle is being damaged. MCP-injected PIE at M1 exit DID apply damage mechanically, so this is primarily a feedback gap — but TASK-017 must ALSO verify real-input plumbing (input mode / HUD hit-testing swallowing clicks), not assume. → TASK-016 (C++ hooks) + TASK-017 (editor wiring + verification).
2. **No visible castle health bar** — damage progress from footmen/hero unreadable. → TASK-018 (C++ delegate + widget component + widget base class) + TASK-019 (WBP_CastleHealthBar duplicated from Variant_Combat's UI_LifeBar).
3. **Footman has no attacking animation.** Footman is a static mesh — blockout answer is a procedural lunge, NOT a skeletal re-rig (that is M7). → TASK-020 (C++ only).
M1 is NOT complete until TASK-016..020 are `done` and the user confirms in a round-2 playtest. Code tasks route through ready-for-qa as usual; compile/assembly/commit happen at integration (no build-master tasks on the board, per M1 decisions).

**Carry-overs recorded from M1 final assembly & QA (feed into M2+ decomposition):**
- Arena has NO boundary — hero can sprint off the slab and fall forever (blocking volumes or KillZ+respawn; M2).
- SM_Castle plinth collision spans the full 814×820 base ~90 units high — nothing can stand/be placed within ~410 units of a castle anchor. Affects M2 gold-node (±1200,0) and building placement near castles.
- PlayerStart was MOVED at integration: (-1700,0,100) → (-1400,0,100) (castle collision enclosed the old spot). handoffs/TASK-015.md's transform table is stale on that row.
- Card button unclickable under GameOnly input (no cursor) — keyboard "1" is the trigger until the M2 hand UI (qa/TASK-007-report.md WARN-4).
- WBP_VictoryScreen::SetWinner uses a byte param, not ETeamId (MCP tooling can't author enum BP params) — ABI-identical, orchestrator-approved deviation (handoffs/TASK-011.md).
- Match end does not freeze units/income under the Victory screen — TODO(M2) (qa/TASK-006-report.md); team-aware PlayerStart selection — TODO(M3); placement lacks navmesh projection (castle roof is placeable) — TODO(M2).
- Uncommitted working-tree residue (deliberate): editor boot-resave deltas on SM_Castle/SM_Footman .uassets, .mcp.json, 2 pre-existing __ExternalActors__ files, untracked Docs/GDD-TEMPLATE.md. Next build-master decides their fate.
- Cosmetic: Shift triggers an engine debug-binding log line each press (BaseInput.ini default; overridable in Config/DefaultInput.ini).

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

## Active tasks

### TASK-001 — Card data types, team types & cards.csv
- assignee: gameplay-programmer
- status: done (commit 5029403)
- blocked-by: none
- parallel-safe: yes
- spec: >
    Files only, no editor. (1) `TeamId.h`: enum `ETeamId { Blue, Red }` plus UInterface `UTeamAgent`/`ITeamAgent`
    with `ETeamId GetTeamId() const` — the shared friendly-fire/team check used by hero, units, castles.
    (2) `CardRow.h`: enums `ECardType { Unit, Building, Economy, Spell, HeroUpgrade, Utility }` and
    `ECardProfile { None, Standard, Siege, Support }`; struct `FCardRow : FTableRowBase` with UPROPERTYs:
    DisplayName (FString), CardType (ECardType), Cost (int32), MaxCopies (int32), HP (float), Damage (float),
    Range (float), Cadence (float), Speed (float), Profile (ECardProfile), Notes (FString).
    (3) Author `Docs/Data/cards.csv` with a header row matching those property names (first column = row name)
    and exactly one data row per GDD §4: Footman = Unit, Cost 3, MaxCopies 12, HP 80, Damage 12, Range 120,
    Cadence 1.0, Speed 400, Profile Standard. Acceptance: code compiles; CSV columns map 1:1 onto FCardRow so
    the editor import (TASK-008) succeeds with zero warnings; Footman values match §4 exactly.
- names: >
    Source/GitClaudeUnrealTest/Siegebound/TeamId.h (ETeamId, UTeamAgent, ITeamAgent);
    Source/GitClaudeUnrealTest/Siegebound/CardRow.h (ECardType, ECardProfile, FCardRow);
    Docs/Data/cards.csv; row name (CardID): Footman. Future import target: /Game/Data/DT_Cards.

### TASK-002 — Castle actor (C++)
- assignee: gameplay-programmer
- status: done (commit 5029403)
- blocked-by: TASK-001
- parallel-safe: yes
- spec: >
    `ACastle` (AActor): UStaticMeshComponent root; EditAnywhere `Team` (ETeamId); implements ITeamAgent;
    MaxHP = 2000, CurrentHP (GDD §3.9), castle does not attack. Mesh/material are soft references resolved
    null-safe at OnConstruction: mesh /Game/Meshes/SM_Castle; material by team — /Game/Materials/Instances/
    MI_TeamColor_Blue or MI_TeamColor_Red (assets may not exist yet; never crash if missing). TakeDamage
    override: ignore damage whose instigator is same-team (no friendly fire, §3.0); melee applies at 100%
    (leave a marked TODO for M2's projectile 50% / Siege 200% scaling — only melee exists in M1). At 0 HP:
    broadcast `FOnCastleDestroyed` (params: ACastle*, ETeamId) exactly once, hide mesh, disable collision.
    `ResetCastle()` restores 2000 HP, visibility, collision (Play Again, §3.9). Acceptance: 2000 cumulative
    enemy damage fires the destroyed event exactly once; friendly damage is fully ignored; ResetCastle
    restores a destroyed castle to 2000/2000 and visible.
- names: >
    ACastle in Source/GitClaudeUnrealTest/Siegebound/Castle.h/.cpp; delegate FOnCastleDestroyed.
    Referenced content (exact): /Game/Meshes/SM_Castle, /Game/Materials/Instances/MI_TeamColor_Blue,
    /Game/Materials/Instances/MI_TeamColor_Red. Level instances (placed at integration): Castle_Blue, Castle_Red.

### TASK-003 — Hero character (C++)
- assignee: gameplay-programmer
- status: done (commit 5029403)
- blocked-by: TASK-001
- parallel-safe: yes
- spec: >
    `AHeroCharacter` per GDD §3.1. May subclass the abstract template `AGitClaudeUnrealTestCharacter` to
    inherit camera boom + Move/Look plumbing (do not modify template files). Team = Blue; implements
    ITeamAgent. Movement: walk 500 u/s, sprint 750 u/s while IA_Sprint (Shift) held. Melee: IA_Attack (LMB)
    deals 20 damage to ALL enemy ITeamAgent actors within 150 units AND inside a 60-degree forward cone
    (±30° of facing), 0.5 s cooldown, no friendly fire. HP: 200 max; out-of-combat regen 5 HP/s starting
    8 s after last taking OR dealing damage, stops at max. Death at 0 HP: hide, disable input/collision,
    notify game mode (respawn timing is TASK-006's job). Expose UPROPERTY TObjectPtr<UInputAction> slots
    (SprintAction, AttackAction) and a UInputMappingContext slot for IMC_Hero — assets wired in TASK-009;
    all input refs null-safe. Acceptance (§3.1): moves at 500 (750 sprinting); one LMB swing damages every
    enemy within 150 in the cone for 20; a target at 200 units is unaffected; swings are rate-limited to
    one per 0.5 s; a hero at 150/200 HP untouched for 8 s regenerates 5 HP/s.
- names: >
    AHeroCharacter in Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h/.cpp.
    Input assets it will receive (exact, created in TASK-009): /Game/Input/IMC_Hero,
    /Game/Input/Actions/IA_Sprint, /Game/Input/Actions/IA_Attack.

### TASK-004 — Summoned unit AI, Standard profile (C++)
- assignee: gameplay-programmer
- status: done (commit 5029403)
- blocked-by: TASK-001, TASK-002
- parallel-safe: yes
- spec: >
    `ASummonedUnit` (ACharacter, AIController-driven, navmesh walking) per GDD §3.8 + §4. EditAnywhere FName
    `CardID`; EditAnywhere `Team` (ETeamId); implements ITeamAgent. On BeginPlay load FCardRow `CardID` from
    /Game/Data/DT_Cards (soft object path, runtime load, log-and-idle if missing — NEVER hardcode stats,
    §3.0): HP -> max/current HP, Speed -> MaxWalkSpeed, Damage/Range/Cadence -> attack. State machine
    Advance -> Acquire -> Attack -> Reacquire, Standard profile: Advance = MoveToActor toward the enemy
    ACastle (nearest ACastle with Team != own, found via actor iteration); Acquire = nearest enemy ITeamAgent
    within 600 aggro radius; if two candidates are within 100 units of each other prefer units/hero over
    buildings/castle; Attack = when target within Range (Footman 120) deal Damage (12) every Cadence (1.0 s),
    melee = 100% vs castle; Reacquire/leash = target dead or beyond 900 units -> resume Advance. No friendly
    fire. Unit is destructible (dies at 0 HP, destroy actor). Visual mesh component left to the BP child
    (TASK-010): add a UStaticMeshComponent "VisualMesh" attached to the capsule, mesh unset in C++.
    Acceptance (§3.8/§4): a spawned Footman (80 HP, 400 speed) pathfinds castle-to-castle across L_Arena,
    attacks the Red castle for 12 every 1.0 s from within 120 units (2000 HP falls in ~167 hits); an enemy
    entering 600 is engaged; on its death the unit resumes Advance; a target beyond 900 is dropped; it never
    damages friendlies.
- names: >
    ASummonedUnit in Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h/.cpp; component name VisualMesh.
    Data (exact): /Game/Data/DT_Cards, row Footman.

### TASK-005 — Gold economy on PlayerState (C++)
- assignee: gameplay-programmer
- status: done (commit 5029403)
- blocked-by: none
- parallel-safe: yes
- spec: >
    `ASiegePlayerState` per GDD §3.2 (M1 subset — overtime/7:00 doubling is M2, note it as TODO). int32 Gold,
    start 50; +2 gold added every 1.0 s on a timer; hard cap 999 (never exceed). API: `bool CanAfford(int32)`,
    `bool SpendGold(int32)` (refuses and returns false if insufficient, never goes negative),
    `void ResetGold()` -> 50 (Play Again, §3.9). Broadcast `FOnGoldChanged(int32 NewGold)` on EVERY change so
    the HUD always matches the underlying value (§3.2 acceptance). Acceptance: starts at 50; gains ~10 over
    5 s; pinned at 999 once reached; SpendGold(3) at 2 gold fails and changes nothing; every mutation fires
    OnGoldChanged.
- names: >
    ASiegePlayerState in Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h/.cpp;
    delegate FOnGoldChanged.

### TASK-006 — GameMode: win condition, hero respawn, Play Again reset (C++)
- assignee: gameplay-programmer
- status: done (commit 7011b7b) (qa/TASK-006-report.md re-review loop 1 — 0 blockers; HasMatchEnded fix verified exact, no other code changed; ready for integration)
- blocked-by: TASK-002, TASK-003, TASK-005, TASK-007
- parallel-safe: yes
- spec: >
    `ASiegeGameMode` per GDD §3.9/§3.1. Class defaults: PlayerStateClass = ASiegePlayerState;
    PlayerControllerClass = ASiegePlayerController; DefaultPawnClass = soft class
    /Game/Blueprints/BP_HeroCharacter.BP_HeroCharacter_C (null-safe fallback to AHeroCharacter). On BeginPlay
    find all ACastle actors and subscribe to FOnCastleDestroyed. Red castle destroyed -> match end, Winner =
    Blue -> tell ASiegePlayerController to show the Victory screen; Blue castle destroyed -> Defeat variant
    (wire it even though nothing damages Blue in M1); a match cannot end any other way. Hero death: respawn
    after exactly 5 s at the hero's own castle (spawn near Castle_Blue / the PlayerStart), full HP, input
    restored (§3.1: respawn within 5-6 s). `PlayAgain()` (UFUNCTION BlueprintCallable, called by
    WBP_VictoryScreen): destroy all ASummonedUnit, ResetCastle() on both castles, ResetGold(), destroy+respawn
    the hero at start, clear all timers, remove end screen — full reset per §3.9 M1 scope (gold, units,
    castle HP, hero). Also edit Config/DefaultEngine.ini [/Script/EngineSettings.GameMapsSettings]:
    GameDefaultMap and EditorStartupMap = /Game/Maps/L_Arena.L_Arena, GlobalDefaultGameMode =
    /Script/GitClaudeUnrealTest.SiegeGameMode. Acceptance (§3.9): 2000 damage to the Red castle triggers the
    Victory screen; Play Again restores gold 50, castles 2000/2000, zero summoned units, hero alive at spawn;
    hero killed at 0 HP is back at its castle in 5-6 s.
- names: >
    ASiegeGameMode in Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h/.cpp; function PlayAgain.
    Referenced (exact): /Game/Blueprints/BP_HeroCharacter, /Game/Maps/L_Arena.
- qa-note: >
    From TASK-005 QA (qa/TASK-005-report.md, major 1): ASiegePlayerState::ResetGold() restarts the income
    timer — PlayAgain must clear timers BEFORE calling ResetGold (or clear selectively), never after,
    or the income timer dies.

### TASK-007 — PlayerController: Footman card play + placement mode (C++)
- assignee: gameplay-programmer
- status: done (commit 7011b7b) (qa/TASK-007-report.md — 0 blockers, 4 warnings, 4 nits)
- blocked-by: TASK-001, TASK-004, TASK-005
- parallel-safe: yes
- spec: >
    `ASiegePlayerController` per GDD §3.5, M1 subset: ONE always-available Footman card, no hand/deck.
    On BeginPlay create and add /Game/UI/WBP_HUD.WBP_HUD_C (soft class, null-safe — widget built in TASK-011).
    `EnterPlacementMode(FName CardID)` (UFUNCTION BlueprintCallable, called by the HUD card button or the
    IA_Card1 key "1"): read Cost from /Game/Data/DT_Cards row (Footman = 3, never hardcode); refuse if
    !CanAfford. In placement mode: show mouse cursor; every frame trace cursor to ground; show a ghost preview
    actor (StaticMesh /Game/Meshes/SM_Footman with dynamic material instance of /Game/Materials/M_Ghost,
    "GhostColor" green when valid / red when invalid, all soft refs null-safe); valid = ground hit AND
    X <= 0 (Blue half, centerline X=0 per CONVENTIONS). Confirm (IA_Attack / LMB while in mode — hero melee
    suppressed): SpendGold(Cost) then spawn /Game/Blueprints/Units/BP_Unit_Footman.BP_Unit_Footman_C
    (soft class) at the point with Team=Blue, exit mode. Invalid click: refuse, spend nothing, stay in mode.
    IA_CancelPlace (RMB/Esc) exits with no cost. Also `HandleMatchEnd(ETeamId Winner)`: show
    /Game/UI/WBP_VictoryScreen.WBP_VictoryScreen_C, enable UI-only input. Acceptance (§3.5): the cost-3 card
    is refused at 2 gold; at 3+ gold a click on the Blue half deducts exactly 3 and spawns the Footman at the
    clicked point; a click on the enemy half (X > 0) is refused with no gold spent and shows the red ghost;
    cancel spends nothing; hero cannot melee while placing.
- names: >
    ASiegePlayerController in Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h/.cpp;
    functions EnterPlacementMode, ExitPlacementMode, HandleMatchEnd. Referenced (exact):
    /Game/Data/DT_Cards (row Footman), /Game/Blueprints/Units/BP_Unit_Footman, /Game/Meshes/SM_Footman,
    /Game/Materials/M_Ghost (param GhostColor), /Game/UI/WBP_HUD, /Game/UI/WBP_VictoryScreen,
    /Game/Input/Actions/IA_Card1, /Game/Input/Actions/IA_CancelPlace.
- qa-note: >
    From TASK-003 QA (qa/TASK-003-report.md, warning 2): AHeroCharacter::ResetHero deliberately preserves
    bMeleeSuppressed — the controller MUST call SetMeleeSuppressed(false) on EVERY placement-mode exit path
    (confirm, cancel, match end, hero death), or the hero can be left unable to melee.

### TASK-008 — Import DT_Cards data table (editor)
- assignee: gameplay-programmer
- status: done (commit 7011b7b)
- blocked-by: TASK-001
- parallel-safe: no
- spec: >
    Editor/MCP work (requires TASK-001 integrated+compiled). Import Docs/Data/cards.csv as a DataTable with
    row struct FCardRow at /Game/Data/DT_Cards. Zero import warnings. Verify row "Footman": CardType Unit,
    Cost 3, MaxCopies 12, HP 80, Damage 12, Range 120, Cadence 1.0, Speed 400, Profile Standard. Keep the
    CSV as the import source so future balance edits are CSV re-imports (§3.0). Acceptance: /Game/Data/
    DT_Cards exists and GetRow("Footman") returns exactly the §4 values.
- names: >
    /Game/Data/DT_Cards (from Docs/Data/cards.csv, row struct FCardRow, row name Footman).

### TASK-009 — Input assets + BP_HeroCharacter (editor)
- assignee: gameplay-programmer
- status: done (commit 7011b7b; PIE speed check at final assembly)
- blocked-by: TASK-003
- parallel-safe: no
- spec: >
    Editor/MCP work (requires TASK-003 integrated+compiled). Create input actions (all bool/Digital) in
    /Game/Input/Actions/: IA_Sprint (Left Shift), IA_Attack (Left Mouse Button), IA_Card1 (keyboard 1),
    IA_CancelPlace (Right Mouse Button AND Escape). Create /Game/Input/IMC_Hero mapping those four PLUS the
    existing template actions /Game/Input/Actions/IA_Move (WASD), IA_Look (mouse), IA_Jump (Space) — do NOT
    modify IMC_Default or any existing asset. Create /Game/Blueprints/BP_HeroCharacter (parent
    AHeroCharacter): skeletal mesh /Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple with anim class
    /Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed (template assets, reuse as-is); assign IMC_Hero
    and the SprintAction/AttackAction UPROPERTY slots from TASK-003. Acceptance (§3.1): in PIE the possessed
    hero walks WASD at 500, sprints at 750 while Shift held, jumps on Space, LMB triggers the melee with
    0.5 s cooldown.
- names: >
    /Game/Input/Actions/IA_Sprint, IA_Attack, IA_Card1, IA_CancelPlace; /Game/Input/IMC_Hero;
    /Game/Blueprints/BP_HeroCharacter (parent AHeroCharacter).
- handoff-note: >
    From handoffs/TASK-003.md: BP_HeroCharacter must ALSO assign the inherited template slots
    JumpAction/MoveAction/LookAction/MouseLookAction (template's SetupPlayerInputComponent binds them);
    AHeroCharacter adds IMC_Hero itself in NotifyControllerChanged at priority 1 — only assign the
    HeroMappingContext/SprintAction/AttackAction UPROPERTYs, do not add the context elsewhere.

### TASK-010 — BP_Unit_Footman (editor)
- assignee: gameplay-programmer
- status: done (commit 7011b7b)
- blocked-by: TASK-004, TASK-008, TASK-014
- parallel-safe: no
- spec: >
    Editor/MCP work. Create /Game/Blueprints/Units/BP_Unit_Footman (parent ASummonedUnit): CardID = Footman;
    VisualMesh = /Game/Meshes/SM_Footman with material /Game/Materials/Instances/MI_TeamColor_Blue; capsule
    sized to the mesh (~90 half-height for a ~180-unit figure); default Team = Blue. Confirm stats resolve
    from DT_Cards at spawn — nothing stat-like set on the BP. Acceptance: a BP_Unit_Footman dropped into
    L_Arena PIE reports 80 HP / 400 speed from the table, paths to the Red castle, and hits it for 12 every
    1.0 s.
- names: >
    /Game/Blueprints/Units/BP_Unit_Footman (parent ASummonedUnit, CardID Footman);
    uses /Game/Meshes/SM_Footman + /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-011 — HUD + Victory widgets (editor)
- assignee: gameplay-programmer
- status: done (M1 final commit)
- blocked-by: TASK-005, TASK-006, TASK-007
- parallel-safe: no
- spec: >
    Editor/MCP work. Placeholder styling is fine (premium UI pass is M7); bindings must be exact.
    (1) /Game/UI/WBP_HUD: gold counter text bound to ASiegePlayerState.FOnGoldChanged (HUD value matches the
    underlying gold at all times, §3.2); one Footman card button showing "Footman" + cost "3" (cost read from
    DT_Cards row, not typed in), greyed/disabled while Gold < 3 (§3.5), OnClicked ->
    ASiegePlayerController.EnterPlacementMode("Footman").
    (2) /Game/UI/WBP_VictoryScreen: full-screen "Victory!" (or "Defeat" via an exposed Winner param) + a
    "Play Again" button -> ASiegeGameMode.PlayAgain() then RemoveFromParent (§3.9 full reset).
    Acceptance: counter ticks live from 50; card greys at <3 gold and re-enables at 3; destroying the Red
    castle brings up Victory; Play Again returns to a fully reset playable match.
- names: >
    /Game/UI/WBP_HUD, /Game/UI/WBP_VictoryScreen (widgets); calls EnterPlacementMode, PlayAgain.
- qa-note: >
    From TASK-005 QA (qa/TASK-005-report.md, major 2): FOnGoldChanged only fires on actual value changes —
    WBP_HUD MUST seed its gold text from ASiegePlayerState::GetGold() on construct, then bind the delegate,
    or a widget created while gold is pinned (e.g. 999) stays stale.

### TASK-016 — Hero attack feedback hooks (C++)
- assignee: gameplay-programmer
- status: integrating (qa-passed qa/TASK-016-report.md; build-master batch with 018+020)
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
- status: backlog
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
- status: integrating (qa-passed qa/TASK-018-report.md; build-master batch with 016+020)
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
- status: backlog
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
- status: integrating (qa-passed qa/TASK-020-report.md; build-master batch with 016+018)
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

## Done

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
