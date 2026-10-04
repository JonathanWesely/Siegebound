# Aura project memory — Siegebound (GitClaudeUnrealTest, UE 5.8 C++)

CANONICAL COPY. `Tools/aura_sync.ps1` regenerates `Saved/.Aura/project_memory.txt` from this file; edit here, never under `Saved/`.

## The team (.claude/agents/) — orchestrator routes, specialists do the work

| Agent | Does | Never does |
|-------|------|-----------|
| manager | Splits requests/GDD into tasks on the board, owns naming conventions | Code, art, engine access |
| gameplay-programmer | C++ / Blueprint / Python logic via files + Unreal MCP | Art prompts, compiling, Git |
| art-director | Models/textures in Blender MCP, imports to Content/, UI layout | Gameplay code, integration, Git |
| qa-reviewer | Reviews code pre-compile, writes pass/fail reports | Editing code, engine, Git |
| build-master | Compiles, assembles assets+code in scene via Unreal MCP, Git commits | Writing new code/art |
| footage-analyst | Reviews gameplay videos in `testvideo/` via frame extraction, writes evidence-backed diagnosis reports (VID-###) | Editing code/art, engine access, Git |
| playtest-verifier | Runs Aura PIE verification, writes runtime evidence | Editing code/art, compiling, Git |

Pipeline files: `.claude/pipeline/TASKBOARD.md` (live tasks, statuses) · `.claude/pipeline/archive/` (finished rows) · `.claude/pipeline/CONVENTIONS.md` (naming law + `## Law index`) · `.claude/pipeline/law/<NAMESPACE>.md` (feature and namespace law) · `handoffs/` · `qa/`.

## Asset prefixes (Content Browser) — from CONVENTIONS.md "Asset prefixes (Content Browser)"

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
| DA_    | Data asset (UDataAsset instance) | Content/Data/ |
| L_     | Level | Content/Maps/ |
| LS_    | Level Sequence (Sequencer) | Content/Cinematics/ |
| ABP_   | Anim Blueprint | Content/Characters/ |
| A_     | Anim Sequence | Content/Characters/Anims/ |
| AM_    | Anim Montage | Content/Characters/Anims/ |
| SKEL_  | Skeleton (shared rig) | Content/Characters/ |
| IK_    | IK Rig (retargeting) | Content/Characters/ |
| RTG_   | IK Retargeter | Content/Characters/ |
| IA_    | Input action | Content/Input/Actions/ |
| IMC_   | Input mapping context | Content/Input/ |

## Texture suffixes — from CONVENTIONS.md "Texture suffixes"

`T_<Name>_D` base color · `T_<Name>_N` normal · `T_<Name>_R` roughness · `T_<Name>_M` metallic · `T_<Name>_E` emissive · `T_<Name>_ORM` packed Occlusion/Roughness/Metallic (LINEAR — sRGB off; added 2026-07-07, TRELLIS pipeline)

C++ (Source/GitClaudeUnrealTest/): `A` actors, `U` UObjects/components, `F` structs, `E` enums, `I` interfaces; one class per header/cpp pair; exception: pure data types (enums, structs, UInterfaces) may share a header-only file when they form one concept (e.g., `TeamId.h`).

## Build command (build-master only; the editor must be CLOSED)

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GitClaudeUnrealTestEditor Win64 Development -project="C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject" -waitmutex
```

## Laws Aura must never break

Never compile via Live Coding — compilation belongs to build-master via Build.bat.
Never write generated meshes into Content/ — they enter through Tools/ArtPipeline Stage 2.
Never run Git.
Never save a level under test (L_Arena is under a never-save law); decline every Save prompt.
Never close, launch, or restart an editor instance; a `-game` instance of UnrealEditor.exe is Jonathan's own play session.

## The verification lane (how Aura is used by the team, as of 2026-10-03)

The `playtest-verifier` agent drives Aura's PIE tools over the `unreal_editor` / `unreal_inspector` MCP servers to verify a task's acceptance lines after the build-master has compiled and relaunched the editor. It writes `.claude/pipeline/qa/TASK-###-verify.md`; line 1 is `Verdict: VERIFIED | VERIFY-FAILED | UNOBSERVABLE | MEASURED`. Jonathan's standing grant (2026-09-20) lets PIE be driven whenever needed; runs are announced and reported, never silent.

Input reaches the game through our own Enhanced Input actions (`inject_input_action`): `IMC_Hero` in a match (`IA_Move`, `IA_Card1`..`IA_Card6`, `IA_DiscardAll`, `IA_Attack`, `IA_ControlsHelp`, `IA_AssistantConsole`) and `IMC_MainMenu` on L_MainMenu (`IA_MenuUp/Down/Left/Right/Accept/Back/Secondary`). Raw key simulation lands below Slate; `ui_perform` `double_click` fires a UButton's OnClicked; `ui_perform` clears Slate keyboard focus. Observables are reflected UPROPERTYs, UFUNCTION getters, `LogSiege*` log lines, and frames; recordings are raw `.h264` under `Saved/AuraVerify/`. Maps: `L_MainMenu` (GameDefaultMap, UI-only input mode) and `L_Arena` (EditorStartupMap; `load_level` into a vs-bot match is the direct route). Full procedure and lessons: `Docs/GameDevSetup.md` Parts 12-13.
