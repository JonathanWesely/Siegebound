# TASK-1248 — [AURA-RUNTIME-MODULE-CHECK] build-master handoff (2026-09-13)

**Status: done (gate WAIVED per the board — read-only measurement; host `TASK-1253` reads it back). ⛔ Nothing edited: no `.uproject`, no `.uplugin`, no engine file, no compile, no cook, no commit.** Law: R8 (`qa/TASK-1239-report.md` § "The ruling" cl. 5) · `PKG-§5a` · `SC-§101` · `SC-§97`.

## Path actually read
`C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/Aura.uplugin` (under `TASK-1216`'s measured root). Plugin header: `"Version": 73`, `"VersionName": "1.0.5"`, `"EngineVersion": "5.8.0"`, `"CreatedBy": "RamenVR"`, `"CanContainContent": true`, `"Installed": true`, `"SupportedTargetPlatforms": ["Win64","Mac"]`.

## Quoted fragment 1 — `Aura.uplugin` → `Modules` (verbatim)
```json
	"Modules": [
		{
			"Name": "Aura",
			"Type": "EditorNoCommandlet",
			"LoadingPhase": "PreDefault",
			"PlatformAllowList": [
				"Win64",
				"Mac"
			]
		},
		{
			"Name": "AuraModelGenerator",
			"Type": "EditorNoCommandlet",
			"LoadingPhase": "Default",
			"PlatformAllowList": [
				"Win64",
				"Mac"
			]
		}
	],
```
- `TargetAllowList`: ABSENT on both modules (and on the plugin header).
- `EnabledByDefault`: KEY ABSENT from `Aura.uplugin` (measured: `'EnabledByDefault' in json` → False). The plugin is enabled for this project by the `.uproject` entry below, not by default.
- `Content/` directory under the plugin root: ABSENT (`os.path.isdir` → False) — `CanContainContent: true` is inert; there is no plugin content for a cook to pick up. `Binaries/` holds `Win64` only.

## Quoted fragment 2 — `GitClaudeUnrealTest.uproject` → the `Aura` entry (Jonathan's own `0399d1c`, verbatim)
```json
		{
			"Name": "Aura",
			"Enabled": true,
			"SupportedTargetPlatforms": [
				"Win64",
				"Mac"
			]
		}
```
- No `TargetAllowList` on the entry ⇒ the plugin is enabled for EVERY target (Editor, Game, Client, Server) of this project.

## THE LINE
**Does any Aura module have `Type` ∈ {`Runtime`, `RuntimeNoCommandlet`, `ClientOnly`, `ServerOnly`}? — NO.** Both modules are `EditorNoCommandlet`.

**Why the cook of Aura's own modules is unaffected:** UnrealBuildTool compiles an `Editor*`-type module only into Editor targets (`ModuleHostType` Editor / EditorNoCommandlet are excluded from Game, Client and Server targets), so `Aura` and `AuraModelGenerator` cannot enter a `GitClaudeUnrealTest` Shipping binary, and with no plugin `Content/` there is nothing of Aura's for the pak. The previous Shipping package (`c6fce42`, 18 modules) is not changed by these two modules.

## ⚠️ LEAD, `SC-§101` — the dependency block is the part that CAN reach a Shipping package (NOT MEASURED against a cook; recorded, not ruled)
`Aura.uplugin` carries a `"Plugins"` block of 23 dependency plugins, each `"Enabled": true`. UBT enables a plugin's dependencies transitively for every target the PARENT is enabled for — and with no `TargetAllowList` on the `.uproject` entry the parent is enabled for Game/Shipping too. So the dependency plugins' RUNTIME modules are eligible to compile and link into the next monolithic Shipping executable even though Aura's own modules are not. Read-only measurement of each dependency's `.uplugin` under `Engine/Plugins` (module `Type` counted, nothing else):

| dependency | `EnabledByDefault` | already in `.uproject` before `0399d1c`? | runtime-class modules |
|---|---|---|---|
| RemoteControl | (absent) | no | 5 — `RemoteControl`, `RemoteControlLogic`, `WebRemoteControl`, `RemoteControlCommon`, `RemoteControlProtocol` |
| WebBrowserWidget | False | no | 1 — `WebBrowserWidget` |
| PythonScriptPlugin | False | no | 1 — `PythonScriptPluginPreload` |
| GameplayAbilities | False | no | 1 — `GameplayAbilities` |
| PropertyBindingUtils (Optional) | False | no | 1 — `PropertyBindingUtils` |
| GeometryScripting | (absent) | no | 1 — `GeometryScriptingCore` |
| MeshModelingToolset | (absent) | no | 3 — `MeshModelingTools`, `ModelingComponents`, `ModelingOperators` |
| AVCodecsCore (Win64/Linux/Mac) | False | no | 2 — `AVCodecsCore`, `AVCodecsCoreRHI` |
| NVCodecs (Win64/Linux) | False | no | 4 — `NVCodecs`, `NVCodecsRHI`, `NVDEC`, `NVENC` |
| StateTree · ModelContextProtocol | False | **yes** (already enabled) | 1 · 2 — no change from Aura |
| ProceduralMeshComponent · EnhancedInput · PCG · IKRig · Interchange | **True** | n/a (engine default-on) | already in every prior cook — no change from Aura |
| EnvironmentQueryEditor · EditorScriptingUtilities · AssetSearch · CascadeToNiagaraConverter · FileSandbox · SandboxedEditing · AllToolsets | mixed | no | 0 runtime modules — editor-only, no Shipping effect |

Net: **9 dependency plugins carrying 19 runtime-class modules become newly eligible for the Shipping target** because of the `.uproject` Aura entry. Whether each actually links (monolithic Shipping links every enabled runtime module; `Optional` deps only if present) and whether any ships plugin Content into the pak is **NOT MEASURED HERE — only a cook measures it.** Hypothesis, labelled: the next Shipping compile will report more than the 18 modules of `c6fce42`, and `Binaries/Win64/` size will move.

### What the next `/ship`'s `C4`-class sweep must look for in the stage (from this lead)
1. The Shipping compile's module count vs `c6fce42`'s **18** — any delta must be named plugin-by-plugin against the table above.
2. The stage's `Binaries/Win64/` — the executable size vs the 2026-09-09 ship; no stray `UnrealEditor-Aura*.dll` (there should be none — Editor modules) and no `PortablePython`/`MCP` payload.
3. The pak listing — any `/RemoteControl/`, `/WebBrowserWidget/`, `/GameplayAbilities/`, `/MeshModelingToolset/`, `/GeometryScripting/` plugin-content mount paths (each of those plugins has a `Content/` folder in the engine tree; the cooker includes plugin content only when referenced or when `bCookAll`/`DirectoriesToAlwaysCook` says so — the listing is the measurement).
4. The Shipping executable's imported modules (or the `.target` receipt's `BuildProducts`) for `NVENC`/`NVDEC`/`AVCodecs` — video-codec modules that were never in a Siegebound package.

### OWED
**A `PKG-§` ruling is OWED by the manager before the next `/ship`** — the module set of the Shipping target changed at `0399d1c` without a ship gate having said so; `PKG-§5a` is the law that says why it matters.

### Recommendation — ⛔ NOT APPLIED (🧑 his `.uproject`, `0399d1c`)
Give the `.uproject` Aura entry an editor-only shape so the dependency block stops propagating to Game/Shipping:
```json
		{
			"Name": "Aura",
			"Enabled": true,
			"TargetAllowList": [ "Editor" ],
			"SupportedTargetPlatforms": [ "Win64", "Mac" ]
		}
```
(`TargetAllowList` is the `.uproject`-side field UBT honours for plugin references; the Editor target then gets Aura and all 23 dependencies, the Game/Shipping target gets none of them beyond what the project already enables.) Whether he wants this is his call; it changes nothing about how Aura runs in the editor.

## Acceptance (row): two quoted JSON fragments ✅ · the YES/NO line ✅ (NO) · sweep item + OWED line ✅ (given, because the LEAD, not the two Aura modules, is what can reach the stage)

## Fences honoured
`.uproject`, `.uplugin`, engine tree: read only · no compile · no cook · no git add / commit / push · only the `TASK-1248` `status:` line edited on the board.
