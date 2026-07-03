# Naming & Folder Conventions — GitClaudeUnrealTest

Owned by the **manager** agent. All agents MUST follow these. If a needed pattern is missing, the manager adds it here BEFORE issuing the task.

## Asset prefixes (Content Browser)

| Prefix | Type | Folder |
|--------|------|--------|
| BP_    | Blueprint class | Content/Blueprints/ |
| WBP_   | UMG widget | Content/UI/ |
| SM_    | Static mesh | Content/Meshes/ |
| SK_    | Skeletal mesh | Content/Characters/ |
| M_     | Material | Content/Materials/ |
| MI_    | Material instance | Content/Materials/Instances/ |
| T_     | Texture | Content/Textures/ |
| NS_    | Niagara system | Content/VFX/ |
| S_     | Sound | Content/Audio/ |
| DT_    | Data table | Content/Data/ |
| L_     | Level | Content/Maps/ |
| IA_    | Input action | Content/Input/Actions/ |
| IMC_   | Input mapping context | Content/Input/ |

## Texture suffixes
`T_<Name>_D` base color · `T_<Name>_N` normal · `T_<Name>_R` roughness · `T_<Name>_M` metallic · `T_<Name>_E` emissive

## C++ (Source/GitClaudeUnrealTest/)
- Classes: `A` actors, `U` UObjects/components, `F` structs, `E` enums, `I` interfaces
- One class per header/cpp pair; file name = class name without prefix
- Exception: pure data types (enums, structs, UInterfaces) may share a header-only file when they form one concept (e.g., `TeamId.h`)
- Gameplay-relevant members exposed with UPROPERTY/UFUNCTION; use TObjectPtr for UObject members

## C++ layout — Siegebound gameplay
- All new Siegebound gameplay code lives in `Source/GitClaudeUnrealTest/Siegebound/`
- Framework classes carry the `Siege` name: `ASiegeGameMode`, `ASiegePlayerState`, `ASiegePlayerController`
- Gameplay actors use plain descriptive names: `ACastle`, `AHeroCharacter`, `ASummonedUnit`
- Never modify template code (`GitClaudeUnrealTest*` classes, `Variant_*` folders, `ThirdPerson/FloatingCastle.*`); subclassing template classes is allowed

## Blueprint subclasses of C++ classes
- Name = `BP_` + C++ class name without its prefix: `BP_HeroCharacter` (from `AHeroCharacter`) — in Content/Blueprints/
- Per-card unit blueprints: `BP_Unit_<CardID>` in Content/Blueprints/Units/ (e.g., `BP_Unit_Footman`)

## Data-driven card stats (GDD §3.0)
- Source of truth: `Docs/Data/cards.csv` (checked into Git), imported as `/Game/Data/DT_Cards` with row struct `FCardRow`
- Row name = CardID in PascalCase (e.g., `Footman`); code and blueprints reference cards by CardID FName
- Never hardcode a stat that exists in the table

## Team contract
- Enum `ETeamId { Blue, Red }`. The local player is always Blue; the enemy is Red
- Team master material `/Game/Materials/M_TeamColor` exposes a vector parameter named exactly `TeamColor`
- Instances: `/Game/Materials/Instances/MI_TeamColor_Blue` = linear (0.05, 0.30, 1.00); `/Game/Materials/Instances/MI_TeamColor_Red` = linear (1.00, 0.10, 0.05)

## World axes (arena contract)
- In `L_Arena`: Blue castle at (X=-2000, Y=0), Red castle at (X=+2000, Y=0); the centerline is the plane X=0
- Blue placement half: X <= 0; Red half: X >= 0
- Level marker actors are TargetPoints named `<Purpose>Anchor_<Team>`: `CastleAnchor_Blue`, `CastleAnchor_Red`

## Delegates (C++)
- Pattern: `FOn<Owner><Event>`, declared in the owner's header; the UPROPERTY(BlueprintAssignable) member is named `On<Owner><Event>`. Existing: `FOnCastleDestroyed`, `FOnGoldChanged`, `FOnCastleHPChanged(float CurrentHP, float MaxHP)`
- Broadcast on every ACTUAL value change and on reset paths; never on refused/ignored mutations (e.g., friendly-fire damage)
- UI consumers must seed from a getter first, THEN bind (qa/TASK-005-report.md major 2 — a bind-only widget created at a pinned value stays stale)

## Template-donor rule (Variant_* and other template Content)
- Template content is READ-ONLY. Reuse it exactly two ways: (a) direct soft-reference (montages, Niagara systems, camera shakes, anim BPs), or (b) duplicate into a /Game/ project folder and modify only the duplicate. Never edit a donor in place.
- UMG: MCP tooling cannot author widget trees from scratch — every new widget starts as a duplicate of a donor (e.g., `/Game/Variant_Combat/UI/UI_LifeBar`, `/Game/Input/Touch/UI_TouchSimple`) and is rewired incrementally.
- BP function params: enum-typed params are impossible via MCP — use byte (uint8) or float params instead.
- Approved donors so far: `AM_ComboAttack` / `AM_ChargedAttack` / `ABP_Manny_Combat` (hero attack anim), `NS_Damage` (hit impact VFX), `BP_CameraShake_Hit_Enemy` (hit shake), `UI_LifeBar` (health bars) — all under /Game/Variant_Combat/.

## Widgets with C++ bases
- Pattern: `U<Name>Widget` (UUserWidget subclass) in Source/GitClaudeUnrealTest/Siegebound/, files `<Name>Widget.h/.cpp`; the UMG asset `WBP_<Name>` in Content/UI/ is reparented to it. Widget-facing events are BlueprintImplementableEvents with float/byte params only. Example: `UCastleHealthBarWidget` ↔ `/Game/UI/WBP_CastleHealthBar`
- UWidgetComponents on actors are named `<Purpose>Widget` (e.g., `HPBarWidget` on `ACastle`)

## Numbering
Variants use two digits: `SM_Rock_01`, `SM_Rock_02`

## Cross-discipline rule
The task spec's `names:` block is the single source of truth. Programmer code references and Artist asset names must BOTH come from it, character-for-character.
