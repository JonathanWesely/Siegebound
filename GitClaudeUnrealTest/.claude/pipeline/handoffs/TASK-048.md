# TASK-048 — IA_Rally input asset + Rally wiring on BP_HeroCharacter (editor) — HANDOFF

**Agent:** gameplay-programmer
**Date:** 2026-07-04
**Status:** ready-for-qa
**Prior state:** REDISPATCH. Earlier attempt blocked on editor being DOWN; build-master booted the editor detached on the `c8a40b2` DLL (PID 30092) — which added the C++ `BindAction(RallyAction, ETriggerEvent::Started, this, &AHeroCharacter::Rally)` in `AHeroCharacter::SetupPlayerInputComponent`. So this task became pure editor/asset wiring: no BP event-graph work, no compile of new code.

## Editor state
- Editor was already UP + STABLE (build-master's boot on `c8a40b2`). I did NOT boot or close it. All work done live via Unreal MCP.
- Ran one PIE session (`StartPIE`, 5 s warmup, in-viewport) for acceptance, then `StopPIE`. `IsPIERunning` = **false** afterward. **Editor is LEFT UP** and MCP-responsive for TASK-049/050/052.

## What shipped (3 assets)

### 1. `/Game/Input/Actions/IA_Rally` (NEW)
- Created by **duplicating `/Game/Input/Actions/IA_Card1`** (AssetTools.duplicate) — MCP AssetTools has no create-asset verb; IA_Card1 is the exact config needed (plain digital bool press; the Q key lives in the IMC, not the action). Same dup pattern as TASK-032.
- Readback: `get_asset_class` = `InputAction`; `ValueType` = **Boolean**, `Triggers` = **[]**, `Modifiers` = **[]**. Saved (not dirty). On disk: `Content/Input/Actions/IA_Rally.uasset`.
- IA_Card1 donor was **not dirtied** by the duplication (`is_dirty` = false).

### 2. `/Game/Input/IMC_Hero` — appended Q → IA_Rally (EDITED)
- Read the live `DefaultKeyMappings.mappings` array (UE 5.8 live array; legacy top-level `Mappings` stays empty, per TASK-009/032), deep-copied the existing IA_Card1 (`One`) entry as a structural template (0 triggers / 0 modifiers / `InheritSettingsFromAction` / `playerMappableKeySettings=None`), retargeted its `action` → `IA_Rally.IA_Rally` and `key` → **`Q`**, appended, and wrote the full array back via ObjectTools.set_properties. Idempotency-guarded (skips if a Q→IA_Rally already exists).
- **17 → 18 mappings.** Post-write readback confirms the **17 M1/M2 mappings preserved byte-for-byte**, including WASD instanced-modifier subobjects (modifier counts unchanged): SpaceBar→IA_Jump(0), W→IA_Move(1 Swizzle), S→IA_Move(2 Swizzle+Negate), A→IA_Move(1 Negate), D→IA_Move(0), Mouse2D→IA_Look(1 Negate-Y), LeftShift→IA_Sprint(0), LMB→IA_Attack(0), One→IA_Card1(0), RMB→IA_CancelPlace(0), Escape→IA_CancelPlace(0), Two→IA_Card2(0), Three→IA_Card3(0), Four→IA_Card4(0), Five→IA_Card5(0), Six→IA_Card6(0), LeftAlt→IA_UICursor(0). New #18: **Q→IA_Rally (0 triggers / 0 mods)**.
- **IMC_Default was NOT touched** (`is_dirty` = false; only IMC_Hero was set). No donor re-dirtied. Saved (not dirty). On disk: `Content/Input/IMC_Hero.uasset`.

### 3. `/Game/Blueprints/BP_HeroCharacter` — assigned RallyAction (EDITED)
- **This is the wiring that makes the c8a40b2 `BindAction(RallyAction, …)` fire.** `AHeroCharacter::RallyAction` (`UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input") TObjectPtr<UInputAction>`, HeroCharacter.h:206) is a details-panel/class-defaults slot — the identical pattern SprintAction/AttackAction use (TASK-009). Assigned via ObjectTools.set_properties on the Blueprint CDO (ObjectTools auto-targets the CDO for a Blueprint path).
- **Before:** `RallyAction` = `None` (null — this is exactly what would trip the c8a40b2 null warning). **After set:** `/Game/Input/Actions/IA_Rally.IA_Rally`. Then **compiled** the Blueprint (compile_blueprint, no warnings-as-errors) and re-read: RallyAction **survived the compile** = `/Game/Input/Actions/IA_Rally.IA_Rally`. No re-apply needed.
- Co-slot readback intact after compile: SprintAction=IA_Sprint, AttackAction=IA_Attack, HeroMappingContext=IMC_Hero. Saved (not dirty). On disk: `Content/Blueprints/BP_HeroCharacter.uasset`.

## PIE-boot acceptance (the null-RallyAction warning does NOT appear)
The c8a40b2 code logs `UE_LOG(LogGitClaudeUnrealTest, Warning, "AHeroCharacter '%s': RallyAction not assigned …")` in the `else` branch of `SetupPlayerInputComponent` when RallyAction is null.
- **Baseline (pre-PIE):** `GetLogEntries(LogGitClaudeUnrealTest, "Rally")` = **[]** (no prior PIE hero spawn — clean baseline).
- **After StartPIE (5 s warmup):**
  - `GetLogEntries(LogGitClaudeUnrealTest, "Rally")` = **[]** → **the null-RallyAction warning did NOT fire.**
  - `GetLogEntries(LogGitClaudeUnrealTest, "not assigned|not resolved|null")` = **[]** → no input-resolution warnings.
  - **All-category** `GetLogEntries("", "RallyAction|not assigned")` = **[]** → warning absent under every log category.
  - Hero/match flow confirmed running: `SiegeGameMode_0` spawned the Red bot (`SiegeBotController_0`), **both** SiegePlayerController_0 and SiegeBotController_0 built 50-card draw piles (BeginPlay ran), units spawned. This is the standard playable L_Arena flow (SiegeGameMode.DefaultPawnClass = BP_HeroCharacter), so the hero pawn is spawned + possessed and `SetupPlayerInputComponent` runs — making the warning's absence meaningful.
  - Only warnings present are the benign pre-existing M1 Miner-profile lines (`row 'Miner' has profile 0 but only Standard is implemented in M1`) — unrelated to this task.

**Two independent proofs the assignment worked:** (1) positive — CDO readback shows `RallyAction = IA_Rally`, survived compile; (2) negative — the c8a40b2 null-RallyAction warning does not appear in a live PIE boot. Together conclusive.

### What PIE could and could not verify
This MCP surface has **no keypress-injection verb**, so I could not physically press Q inside PIE. The interactive Q → Rally behavior (friendly units within RallyRadius 600 get +25% move speed for 5 s; 20 s cooldown) is verified by **TASK-052 / Jonathan** at the keyboard. This task delivers + verifies the *wiring*: asset created, key-mapped, slot assigned, clean compile, and no null-RallyAction warning at PIE boot. `GetVisibleActors` enumerates the editor persistent level (not the PIE world), so it is not a route to the runtime hero pawn — I relied on the log flow + CDO readback instead, and did not fake keypress results.

## Constraints honored
- **No Git**, no compile of C++ (the binding is already in c8a40b2). The Blueprint compile is an in-editor asset compile via MCP, not a C++ build.
- Did **NOT** edit TASKBOARD.md (orchestrator owns the board). Requested status: `in-progress` → `ready-for-qa`.
- Targeted saves only (IA_Rally, IMC_Hero, BP_HeroCharacter). IMC_Default + IA_Card1 donor left clean (`is_dirty` = false on both).
- The UE Git provider auto-staged the new `IA_Rally.uasset` (git shows `A`); BP_HeroCharacter + IMC_Hero show `M`. Left exactly as-is — **no git run**. TASK-052 commits these.
- Editor left **UP**, PIE stopped.

## Files / assets touched
- NEW: `Content/Input/Actions/IA_Rally.uasset`
- EDITED: `Content/Input/IMC_Hero.uasset` (1 mapping appended; 17 M1/M2 mappings + WASD modifiers preserved byte-for-byte)
- EDITED: `Content/Blueprints/BP_HeroCharacter.uasset` (RallyAction slot assigned, recompiled)
- No source (.cpp/.h) changes — the `BindAction(RallyAction, …)` is already compiled in c8a40b2.

## For QA to scrutinize
- IMC_Hero readback: 18 mappings; 17 originals unchanged (WASD modifier counts W=1/S=2/A=1/Mouse2D=1 preserved through the array rewrite); new Q→IA_Rally is 0 triggers/0 modifiers. Legacy top-level `Mappings` still empty (runtime reads `DefaultKeyMappings.mappings`).
- IA_Rally: `InputAction`, Boolean/Digital, 0 triggers/0 modifiers, path matches the spec exactly (`/Game/Input/Actions/IA_Rally`) and the c8a40b2 code's expected path.
- BP_HeroCharacter RallyAction slot = IA_Rally, survived a clean Blueprint compile; SprintAction/AttackAction/HeroMappingContext unchanged.
- PIE boot: no null-RallyAction warning (all categories), no input-resolution warnings, match flow ran (decks built, bot spawned).
