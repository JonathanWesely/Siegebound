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

1. **M1 — Core loop, local, one card** — `in-progress`
2. M2 — Economy + deck/hand + core set + defenses — `not-started`
3. M3 — Bot opponent = real 1v1 match — `not-started`
4. M4 — Card Set II (16 cards, keywords, hero upgrades) — `not-started`
5. M5 — Spell system + Set III — `not-started`
6. M6 — Deck-builder meta — `not-started`
7. M7 — Premium art & feel pass — `not-started`
8. M8 — Networked 1v1 multiplayer — `not-started`

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
- status: qa-passed
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
- status: in-progress
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
- status: in-progress
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
- status: backlog
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
- status: qa-passed
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
- status: backlog
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

### TASK-007 — PlayerController: Footman card play + placement mode (C++)
- assignee: gameplay-programmer
- status: backlog
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

### TASK-008 — Import DT_Cards data table (editor)
- assignee: gameplay-programmer
- status: backlog
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
- status: backlog
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

### TASK-010 — BP_Unit_Footman (editor)
- assignee: gameplay-programmer
- status: backlog
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
- status: backlog
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

### TASK-012 — Team-color + ghost materials (art)
- assignee: art-director
- status: integrating
- blocked-by: none
- parallel-safe: yes
- spec: >
    Blockout-tier materials that later premium passes (M7) can upgrade in place. (1) Master material
    /Game/Materials/M_TeamColor with a vector parameter named EXACTLY "TeamColor" (CONVENTIONS Team
    contract); a simple vertical gradient (darker base -> lighter top, §6 spirit) is welcome but optional at
    this tier. (2) Instances: /Game/Materials/Instances/MI_TeamColor_Blue with TeamColor = linear
    (0.05, 0.30, 1.00); /Game/Materials/Instances/MI_TeamColor_Red with TeamColor = linear (1.00, 0.10,
    0.05). (3) /Game/Materials/M_Ghost: translucent, unlit, two-sided, vector parameter named EXACTLY
    "GhostColor" defaulting to green (0,1,0) at ~0.35 opacity — the programmer swaps it to red at runtime
    for invalid placement (§3.5). Acceptance: applying MI_TeamColor_Blue vs _Red to the same mesh reads
    unmistakably blue vs red at 15 m; M_Ghost renders see-through and recolors via GhostColor.
- names: >
    /Game/Materials/M_TeamColor (param TeamColor), /Game/Materials/M_Ghost (param GhostColor),
    /Game/Materials/Instances/MI_TeamColor_Blue, /Game/Materials/Instances/MI_TeamColor_Red.

### TASK-013 — Castle blockout mesh (art)
- assignee: art-director
- status: in-progress (Blender modeling + FBX export; editor import deferred until editor free)
- blocked-by: TASK-012
- parallel-safe: yes
- spec: >
    One castle static mesh in Blender, blockout tier but with a readable keep-and-towers silhouette
    (identifiable at 15 m, §6): footprint ~800x800 units, height ~900 units, origin at ground-center,
    <= 15k tris, ONE material slot with /Game/Materials/Instances/MI_TeamColor_Blue assigned as default
    (code swaps the instance per team — same mesh serves both castles). Simple collision (box/convex) so
    units and the hero collide. Import to /Game/Meshes/SM_Castle. Later premium passes replace this mesh at
    the same path without breaking references. Acceptance: imports clean at correct scale next to the
    ~180-unit mannequin; one material slot; blocks movement.
- names: >
    /Game/Meshes/SM_Castle (single material slot, default MI_TeamColor_Blue).

### TASK-014 — Footman blockout mesh (art)
- assignee: art-director
- status: in-progress (Blender modeling + FBX export; editor import deferred until editor free)
- blocked-by: TASK-012
- parallel-safe: yes
- spec: >
    Placeholder Footman static mesh in Blender: chunky stylized proportions (~2.5-3 heads tall, §6),
    ~180 units tall, sword-and-shield silhouette readable at 15 m, <= 8k tris, ONE material slot with
    /Game/Materials/Instances/MI_TeamColor_Blue as default. Static mesh only — no rig/anim in M1 (units are
    capsule-driven; skeletal swap comes with the M7 art pass at the same visual-slot contract). Import to
    /Game/Meshes/SM_Footman. This exact path is also used by the placement ghost (TASK-007). Acceptance:
    imports clean at ~180 units, one material slot, origin at feet-center.
- names: >
    /Game/Meshes/SM_Footman (single material slot, default MI_TeamColor_Blue).

### TASK-015 — L_Arena blockout level (art)
- assignee: art-director
- status: in-progress
- blocked-by: none
- parallel-safe: yes
- spec: >
    New level per GDD §5 at /Game/Maps/L_Arena (do NOT modify Lvl_ThirdPerson). Symmetric layout on the
    CONVENTIONS world-axes contract: flat walkable ground ~6400 (X) x 3200 (Y) centered on origin at Z=0;
    Blue side -X, Red side +X. Place TargetPoints named EXACTLY CastleAnchor_Blue at (-2000, 0, 0) and
    CastleAnchor_Red at (+2000, 0, 0) — castles ~4000 units apart; integration places the ACastle actors at
    these anchors, art does NOT place castles. Visible centerline stripe along X=0 (thin emissive plane or
    decal, no gameplay collision). PlayerStart at (-1700, 0, 100) facing +X. NavMeshBoundsVolume covering
    the entire ground so units path castle-to-castle; a NavMesh appears (P key) across both halves. Lighting:
    DirectionalLight + SkyLight + SkyAtmosphere + ExponentialHeightFog, defaults fine (premium lighting is
    M7). Leave open silhouette room for M7 set dressing and for M2 gold nodes near (±1200, 0). Acceptance
    (§5): PIE loads L_Arena with walkable ground; navmesh covers both halves; anchors and PlayerStart at the
    exact coordinates; centerline visibly divides the halves.
- names: >
    /Game/Maps/L_Arena; actors CastleAnchor_Blue, CastleAnchor_Red (TargetPoints), PlayerStart;
    centerline marker actor named CenterlineMarker.

---

## Done

(move completed tasks here)
