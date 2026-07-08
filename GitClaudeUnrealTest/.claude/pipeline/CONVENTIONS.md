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
`T_<Name>_D` base color · `T_<Name>_N` normal · `T_<Name>_R` roughness · `T_<Name>_M` metallic · `T_<Name>_E` emissive · `T_<Name>_ORM` packed Occlusion/Roughness/Metallic (LINEAR — sRGB off; added 2026-07-07, TRELLIS pipeline)

## C++ (Source/GitClaudeUnrealTest/)
- Classes: `A` actors, `U` UObjects/components, `F` structs, `E` enums, `I` interfaces
- One class per header/cpp pair; file name = class name without prefix
- Exception: pure data types (enums, structs, UInterfaces) may share a header-only file when they form one concept (e.g., `TeamId.h`)
- Gameplay-relevant members exposed with UPROPERTY/UFUNCTION; use TObjectPtr for UObject members
- **No shadowing inherited reflected members (coding law):** no local variable, function parameter, or loop variable may shadow an inherited reflected UPROPERTY — e.g. `Owner`/`PlayerState`/`Instigator`/`Controller` on AActor/APawn/AController, `Slot` on UWidget. UHT + the UE toolchain compile this as a HARD ERROR (C4457 param / C4458 member / C4459 global), not a warning. Rename the local (e.g. `OwnerState`, `IterPlayerState`, `SlotIndex`). **QA MUST scan every code task for this class of shadow pre-compile** — it slipped past QA twice in the M2 batch (TASK-025, TASK-029) and cost 2 build loops.

## C++ layout — Siegebound gameplay
- All new Siegebound gameplay code lives in `Source/GitClaudeUnrealTest/Siegebound/`
- Framework classes carry the `Siege` name: `ASiegeGameMode`, `ASiegePlayerState`, `ASiegePlayerController`
- Gameplay actors use plain descriptive names: `ACastle`, `AHeroCharacter`, `ASummonedUnit`
- Never modify template code (`GitClaudeUnrealTest*` classes, `Variant_*` folders, `ThirdPerson/FloatingCastle.*`); subclassing template classes is allowed

## Blueprint subclasses of C++ classes
- Name = `BP_` + C++ class name without its prefix: `BP_HeroCharacter` (from `AHeroCharacter`) — in Content/Blueprints/
- Per-card unit blueprints: `BP_Unit_<CardID>` in Content/Blueprints/Units/ (e.g., `BP_Unit_Footman`)
- Per-card building blueprints: `BP_Building_<CardID>` in Content/Blueprints/Buildings/ (e.g., `BP_Building_ArrowTower`, `BP_Building_Wall`)
- Code spawns card actors by composed soft-class path from the CardID: `/Game/Blueprints/Units/BP_Unit_<CardID>.BP_Unit_<CardID>_C` (units/economy) and `/Game/Blueprints/Buildings/BP_Building_<CardID>.BP_Building_<CardID>_C` (buildings) — always null-safe (missing BP = refused play + log, never a crash)

## Per-card visual assets
- A card actor's visual mesh asset is `SM_<CardID>` in Content/Meshes/ (e.g., `SM_Footman`, `SM_ArrowTower`). This is a code contract: placement-ghost previews resolve `/Game/Meshes/SM_<CardID>` by string.
- The mesh component on card actors (ASummonedUnit, ABuilding) is named exactly `VisualMesh`.

## Card artwork (hand UI)
Added 2026-07-07 (TASK-077..081, Jonathan feature request: real card art on the hand-UI card faces). Every CardID in the roster gets exactly ONE illustration texture.
- **Texture asset:** `T_CardArt_<CardID>` in Content/UI/CardArt/ (`/Game/UI/CardArt/T_CardArt_<CardID>`), one per CardID, exactly **512×512** (square power-of-two — plenty for a hand-slot face). Deliberate exception to the prefix table's `T_` → Content/Textures/ row: card art is UI-owned and lives with the UI. Import settings: Texture Group = UI, sRGB on, default compression.
- **Source render:** PNG at `Content/RawAssets/CardArt/<CardID>.png`, checked into Git alongside the imported .uasset (mirrors the FBX raw-asset rule). `<CardID>` casing character-for-character from cards.csv row names.
- **Data path (§3.0 law):** the art reference is DT_Cards data, never hardcoded — FCardRow column `CardArt` (`TSoftObjectPtr<UTexture2D>`); CSV cell = the FULL object path `/Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>`. Unset/unresolvable path ⇒ graceful text-only card face (today's presentation), log once, never a crash. Widgets never hardcode a CardID→texture mapping.
- **Resolution seam:** `UCardHandWidget` exposes null-safe `UFUNCTION(BlueprintCallable)` resolver(s) — `GetCardArtTexture(CardID)` + a preview-art equivalent (TASK-079). The three hand BIEs (`OnHandSlotUpdated` / `OnNextCardUpdated` / `OnCardRefusedMessage`) keep their signatures: BIE PARAMS stay float/int/bool/byte/FString only; UObject returns are allowed on BlueprintCallable functions, which is why the resolver pattern is the law here.
- **Face composition:** art is the BACKGROUND layer of the card face; DisplayName + cost text overlay on top and must stay legible (translucent contrast strip / shadow behind text is allowed). The artwork itself contains NO baked-in text. Art images are HitTestInvisible — clicks belong to the play/discard buttons (M1 WARN-4 posture). Style may be blockout-tier stylized (premium art = M7) but each card must read at ~150 px: one dominant subject, strong silhouette, distinct per-card color key, team-agnostic palette (cards are player-neutral).

## Data-driven card stats (GDD §3.0)
- Source of truth: `Docs/Data/cards.csv` (checked into Git), imported as `/Game/Data/DT_Cards` with row struct `FCardRow`
- Row name = CardID in PascalCase, no spaces (e.g., `Footman`, `ArrowTower`); code and blueprints reference cards by CardID FName; `DisplayName` carries the spaced human name ("Arrow Tower")
- Never hardcode a stat that exists in the table
- FCardRow columns beyond the GDD §4 stat columns (registry — CSV header must match UPROPERTY names 1:1):
  - `DeckCount` (int32) — copies of this card in the default 50-card deck (GDD §3.4); all DeckCount values must sum to exactly 50; 0 = not in the default deck. **M4 note:** the M4 test deck (TASK-053) repurposes DeckCount as an expanded 22-card 50-count deck so Set II is reachable until the M6 deck-builder — still sums to 50, each ≤ MaxCopies.
  - `bRanged` (bool) — true if the card's attack is delivered by a homing projectile (GDD §3.0) instead of melee contact
  - `CardArt` (TSoftObjectPtr<UTexture2D>) — hand-UI card illustration; CSV cell = full object path `/Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>`; unset = text-only face fallback (added 2026-07-07, TASK-079; law in "Card artwork (hand UI)")
  - **M4 keyword/behavior columns (Set II, TASK-053):** typed one-per-concept, sparse (defaults shown):
    - `bCharge` (bool, false) — Charge keyword: first attack after ≥2 s uninterrupted movement deals 2× (GDD §3.0). (Cavalry)
    - `bSlayer` (bool, false) — Slayer keyword: 2× damage vs targets with MaxHP ≥ 150 (GDD §3.0). (Pikeman)
    - `bSuicide` (bool, false) — unit explodes on contact/death then dies, dealing its Damage as AoE over `AoERadius` (GDD §4). (Sapper)
    - `SwarmCount` (int32, 0) — Swarm keyword: >0 ⇒ playing the card spawns this many copies in a 300-unit circle for one cost (GDD §3.0). (Militia Mob = 4)
    - `AoERadius` (float, 0) — splash radius for area attackers; 0 = single target. (Sapper 250, Bomb Tower 250)
    - `MinRange` (float, 0) — inner blind-spot radius; the actor cannot fire at targets closer than this (GDD §4). (Ballista Tower 300)
    - `SpawnCardID` (FName, None) / `SpawnInterval` (float, 0) / `Lifetime` (float, 0) — spawner building: spawns `SpawnCardID` every `SpawnInterval` s, self-destructs after `Lifetime` s (GDD §4). (Barracks = Footman / 8 / 60)
  - The keyword-set token approach is deferred; M5's Chain adds its own typed column when it arrives.
- Mechanic RULES (not per-card stats) — e.g., active miner cap 6, building clearance 200, overtime at 420 s, Swarm 300-unit spawn circle, Charge 2 s / 2× multipliers, Slayer 150-HP / 2× threshold, Deep Mine +2 gold/s, Masons 300 HP over 10 s, hero-upgrade magnitudes (§3.10), Rally 600/25%/5 s/20 s — are UPROPERTY defaults in the owning class with a `// GDD §x.x` comment; they do not get CSV columns

## Team contract
- Enum `ETeamId { Blue, Red }`. The local player is always Blue; the enemy is Red
- Team master material `/Game/Materials/M_TeamColor` exposes a vector parameter named exactly `TeamColor`
- Instances: `/Game/Materials/Instances/MI_TeamColor_Blue` = linear (0.05, 0.30, 1.00); `/Game/Materials/Instances/MI_TeamColor_Red` = linear (1.00, 0.10, 0.05)
- **Team-driven visuals (M3):** team-owned actors (units, buildings, miners) apply the matching `MI_TeamColor_<Team>` to their `VisualMesh` slot 0 in `BeginPlay` from their `Team` value; the Blue material authored on the BP is only the design-time placeholder, so the bot's Red units recolor at spawn. Gold nodes are the exception — they carry the authored `M_GoldGlow` emissive regardless of team.

## World axes (arena contract)
- In `L_Arena`: Blue castle at (X=-2000, Y=0), Red castle at (X=+2000, Y=0); the centerline is the plane X=0
- Blue placement half: X <= 0; Red half: X >= 0
- Level marker actors are TargetPoints named `<Purpose>Anchor_<Team>`: `CastleAnchor_Blue`, `CastleAnchor_Red`
- Gold nodes (GDD §5, 800 units in front of each castle): `AGoldNode` instances `GoldNode_Blue` at (-1200, 0), `GoldNode_Red` at (+1200, 0)
- Team-owned level instances are named `<Thing>_<Team>` (e.g., `Castle_Blue`, `GoldNode_Red`)

## Damage types (C++)
- UDamageType subclasses named `USiegeDamageType_<Kind>`, all declared in `Source/GitClaudeUnrealTest/Siegebound/DamageTypes.h/.cpp`
- Kinds: `Melee`, `Projectile`, `Siege` (**added M4, TASK-054**); `Spell` reserved M5
- Damage-vs-fortification scaling (GDD §3.0) reads `DamageEvent.DamageTypeClass`: Projectile = 50%, Melee/default = 100%, **`Siege` = 200%**. **M4 ruling:** the 200% applies to BOTH the castle (`ACastle::TakeDamage`) AND buildings (`ABuilding::TakeDamage`) — Siege units (Profile=Siege: Ogre, Sapper) tag their attacks with `USiegeDamageType_Siege`. Units and the hero always take the listed damage (no scaling). Attackers tag projectile/siege damage; melee needs no tag.

## Raw asset sources
- Blender FBX exports live in `Content/RawAssets/<AssetNameWithoutPrefix>.fbx` (e.g., `Castle.fbx` → `SM_Castle`); the FBX is checked into Git alongside the imported .uasset

## Textured mesh law (TRELLIS.2 art pipeline)
Added 2026-07-07 (TASK-082..088, Jonathan-approved plan: automated TRELLIS.2 → Blender → UE5 pipeline). Governs game-ready textured meshes produced by `Tools/ArtPipeline/` (pilot scope: Footman, Archer, Castle; the remaining 16 blockouts follow this template at M7). `<AssetName>` = CardID for card actors, `Castle` for the castle.
- **Concepts:** the ACCEPTED concept image for each produced asset is committed at `Content/RawAssets/Concepts/<AssetName>.png`. Working inputs live in `Tools/ArtPipeline/Inbox/<AssetName>.png` and intermediates in `Tools/ArtPipeline/Cache/<AssetName>/` — both gitignored.
- **Mesh swap:** the refined FBX exports to the EXISTING blockout path `Content/RawAssets/<AssetName>.fbx` and imports OVERWRITING `/Game/Meshes/SM_<AssetName>` in place — same-path swaps keep every code/BP soft reference (including the placement-ghost `/Game/Meshes/SM_<CardID>` string contract) intact. NEVER delete+recreate the SM asset.
- **Textures:** `/Game/Textures/T_<AssetName>_D` (sRGB) · `T_<AssetName>_N` (normal) · `T_<AssetName>_ORM` (packed Occlusion/Roughness/Metallic, LINEAR — sRGB off). PNG sources checked in at `Content/RawAssets/Textures/<AssetName>/`. Bake sizes: 1024² units / 2048² buildings.
- **Two-slot material contract (requires ZERO C++ changes):** every pipeline mesh has EXACTLY two material slots, in order — **slot 0 named `TeamRegion`** (a minority face-set: trim/banners/accents; the BeginPlay team recolor hardcodes `MI_TeamColor_<Team>` onto slot 0 — SummonedUnit.cpp:146 / Building.cpp:80 / Castle.cpp:125 — and keeps working unchanged; author `MI_TeamColor_Blue` as the design-time placeholder), **slot 1 named `<AssetName>PBR`** assigned `MI_<AssetName>_PBR` (Content/Materials/Instances/), an instance of the master **`/Game/Materials/M_AssetPBR`** whose texture parameters are named exactly `BaseColor`, `Normal`, `ORM`. The placement ghost tints ALL slots (already true today).
- **Mesh settings:** **Nanite OFF** on pipeline meshes (ruling). UV layer named exactly `UVMap`. Tri budgets: units ≤15k, castle ≤40k (per `pipeline_manifest.json`). Origins: units feet-center, buildings ground-center. Export axis contract: `axis_forward='-Z'`, `axis_up='Y'`, `apply_unit_scale`, FACE smoothing. Collision: units ≤4 simple hulls; buildings author explicit `UCX_SM_<AssetName>` geometry — the castle UCX must be wall-footprint-exact (M1 plinth ~410-unit dead-zone lesson) with bounds within ±10% of the blockout.
- **Tooling law:** `Tools/**/*.py` is CODE — the full QA gate applies before commit. **`HF_TOKEN` is ENV-ONLY**: read from the environment at runtime; never written to any file, never passed on argv, never echoed/logged (guard-secrets hook carries the `hf_` pattern). Heavy Blender refine runs HEADLESS via `blender.exe --background --python` through Bash — the live Blender MCP bridge has a 30 s socket cap and is for <30 s inspection/preview only.
- **Fab quarantine:** marketplace packs land in `Content/Fab/<Pack>/` and are READ-ONLY donors (the Variant_* rule applies: soft-reference or duplicate-into-/Game/, never edit in place). Acquisition is HUMAN-ONLY via `.claude/pipeline/fab/FAB-REQUESTS.md` (FAB-### entries: requested → approved → fulfilled → integrated; Jonathan fulfills via the Epic Launcher). Agents never browse/buy/download Fab.
- **Lane isolation:** this pipeline NEVER writes `Content/RawAssets/CardArt/` or `/Game/UI/CardArt/` — those belong to the card-art chain (TASK-077..081, "Card artwork (hand UI)" law).

## Delegates (C++)
- Pattern: `FOn<Owner><Event>`, declared in the owner's header; the UPROPERTY(BlueprintAssignable) member is named `On<Owner><Event>`. Existing: `FOnCastleDestroyed`, `FOnGoldChanged`, `FOnCastleHPChanged(float CurrentHP, float MaxHP)`
- Broadcast on every ACTUAL value change and on reset paths; never on refused/ignored mutations (e.g., friendly-fire damage)
- UI consumers must seed from a getter first, THEN bind (qa/TASK-005-report.md major 2 — a bind-only widget created at a pinned value stays stale)

## Logging (C++)
- Gameplay log categories are named `LogSiege<Domain>`, declared in the owning module. The M3 rule-based bot's decision trace uses **`LogSiegeBot`** — exactly one line per fired decision (which of the §4 ordered rules played, what card, and where), so the "logged decision trace" acceptance in GDD §4 is grep-able.

## Template-donor rule (Variant_* and other template Content)
- Template content is READ-ONLY. Reuse it exactly two ways: (a) direct soft-reference (montages, Niagara systems, camera shakes, anim BPs), or (b) duplicate into a /Game/ project folder and modify only the duplicate. Never edit a donor in place.
- UMG: MCP tooling cannot author widget trees from scratch — every new widget starts as a duplicate of a donor (e.g., `/Game/Variant_Combat/UI/UI_LifeBar`, `/Game/Input/Touch/UI_TouchSimple`) and is rewired incrementally.
- BP function params: enum-typed params are impossible via MCP — use byte (uint8) or float params instead.
- Approved donors so far: `AM_ComboAttack` / `AM_ChargedAttack` / `ABP_Manny_Combat` (hero attack anim), `NS_Damage` (hit impact VFX), `BP_CameraShake_Hit_Enemy` (hit shake), `UI_LifeBar` (health bars) — all under /Game/Variant_Combat/.

## Widgets with C++ bases
- Pattern: `U<Name>Widget` (UUserWidget subclass) in Source/GitClaudeUnrealTest/Siegebound/, files `<Name>Widget.h/.cpp`; the UMG asset `WBP_<Name>` in Content/UI/ is reparented to it. Widget-facing events are BlueprintImplementableEvents with float/int/bool/byte/FString params only (never enums). Examples: `UCastleHealthBarWidget` ↔ `/Game/UI/WBP_CastleHealthBar`, `UCardHandWidget` ↔ `/Game/UI/WBP_CardHand`
- UWidgetComponents on actors are named `<Purpose>Widget` (e.g., `HPBarWidget` on `ACastle`)

## Input-mode ownership (level-travel law)
Added 2026-07-07 (TASK-074 bugfix chain). Input-routing state set via `APlayerController::SetInputMode` lives partly on the persistent `UGameViewportClient` and can SURVIVE `UGameplayStatics::OpenLevel*` travel — **a level must never trust the input posture it inherits from whoever traveled it in.** Each level's controller establishes its own posture at startup:
- **L_Arena / `ASiegePlayerController`:** match posture is GameOnly free-look + hidden cursor (M2 TASK-023 ruling), composed by `ApplyCursorInputState()` — the only cursor owners are placement mode, the Alt-held `IA_UICursor`, and `HandleMatchEnd`'s UIOnly end screen. The controller normalizes to this posture at `BeginPlay` (TASK-074) instead of assuming engine defaults.
- **L_MainMenu / `BP_MenuGameMode`:** menu posture is UIOnly + visible cursor (TASK-049) so `WBP_MainMenu` buttons stay clickable.
- The static travel entries (`ASiegeGameMode::StartMatch` / `::StartSandboxMatch`) stay posture-agnostic — they only open the level and never set input modes.

## Dev / test tooling (non-gameplay affordances)
Names for developer/test-bench features that are NOT GDD content and NOT part of any milestone. They must not disturb the shipping flow; every one is additive.
- **Sandbox (No Bot) mode** (added 2026-07-05, TASK-071/072) — opens `L_Arena` with the full card roster but **no enemy AI**, as a calm test bench for the 22-card roster. Mechanism is a **level-open URL option**, deliberately NOT a GameInstance (no `USiegeGameInstance`, no `GameInstanceClass` config change):
  - **Option token:** `Sandbox=1`, parsed with `UGameplayStatics::HasOption(OptionsString, TEXT("Sandbox"))`. The token string is `Sandbox` — code and any future consumer must use it character-for-character.
  - **Game-mode latch:** `ASiegeGameMode` reads the option in `InitGame` into a `bool bSandboxMatch` (persists for the life of the `L_Arena` world, so `PlayAgain` stays sandbox).
  - **Bot gate:** `ASiegeGameMode::SpawnBot()` early-returns when `bSandboxMatch` is true — no `ASiegeBotController` is spawned and no Red bot `ASiegePlayerState` is created. The Red `ACastle` (`Castle_Red`) remains as a static target dummy; Blue units/buildings still march on it.
  - **Menu entry:** `static void ASiegeGameMode::StartSandboxMatch(const UObject* WorldContextObject)` — mirrors `StartMatch` but appends the `Sandbox=1` option to `OpenLevelBySoftObjectPtr`. `StartMatch` (Play vs Bot) keeps its exact signature and behavior — untouched.
  - **Generous economy:** `UPROPERTY(EditDefaultsOnly) int32 SandboxStartingGold` on `ASiegeGameMode`, default `9999` (`// dev sandbox — full roster freely playable`), granted to the Blue player once at match start when `bSandboxMatch` is true, via the existing player-state gold API (never hardcode a gold mutation that bypasses it). Normal gold rate otherwise.
  - **Menu button:** `Btn_Sandbox` on `/Game/UI/WBP_MainMenu`, label text `"Sandbox (No Bot)"`, `OnClicked → ASiegeGameMode::StartSandboxMatch`. Placed as an additive sibling next to the existing Play-vs-Bot button (which calls `StartMatch`); the existing button and its binding are not disturbed.

## Numbering
Variants use two digits: `SM_Rock_01`, `SM_Rock_02`

## Cross-discipline rule
The task spec's `names:` block is the single source of truth. Programmer code references and Artist asset names must BOTH come from it, character-for-character.
