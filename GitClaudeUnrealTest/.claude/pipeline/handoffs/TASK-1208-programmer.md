# TASK-1208 — README-SAVED-PATH-FIX — programmer handoff (2026-09-10)

marker `TASK-1208-README-SAVED-PATH-FIX` · gate `TASK-1209` · host `TASK-1210`
Files written: `Docs/Packaging/README-source.md` (one sentence + one citation comment) · this handoff. Nothing else. No editor, no compile, no Git, no `Source/**` / `Content/**` / `Config/**` / `Tools/**`.

## 1. Before / after — VERBATIM

**Before** (`README-source.md:170-172`, one sentence wrapped over three physical lines, uncited):

```
- The game writes its save data (accounts, decks, settings) into
  `Windows\GitClaudeUnrealTest\Saved\` next to the executable, so extract somewhere you have
  write permission (not `C:\Program Files`).
```

**After** (`README-source.md:170-175`, one sentence wrapped over six physical lines; the citation comment is on the last line so it stays a single-line HTML comment like every other `src:` marker in the file):

```
- The game writes its save data (accounts, decks, settings) into your Windows user profile, not
  next to the executable: `%LOCALAPPDATA%\GitClaudeUnrealTest\Saved\` — normally
  `C:\Users\<you>\AppData\Local\GitClaudeUnrealTest\Saved\` (typing `%LOCALAPPDATA%` into the
  File Explorer address bar opens that `Local` folder). Your decks are in
  `SaveGames\SiegeDecks.sav` and your settings in `Config\Windows\GameUserSettings.ini`; nothing
  is written beside the executable. <!-- src: … (see §2) -->
```

Wording choices, against the spec: player-facing; no task ids or law citations in the visible prose; the measured location (`%LOCALAPPDATA%\GitClaudeUnrealTest\Saved\`) is stated without hedge; the only "normally" qualifies the `C:\Users\<you>\AppData\Local` expansion, which the dispatch asked me not to assert for every machine (this machine's `cmd /c echo %LOCALAPPDATA%` → `C:\Users\wesel\AppData\Local`); the two files named are the two the measurement recorded; the old "extract somewhere you have write permission" instruction is dropped because its premise was false, and I did **not** replace it with "the folder can be read-only" — that was never measured (`SC-§109`: the text says what to look for, not what to conclude).

## 2. The citation comment (full text, as written on line 175)

```
<!-- src: measured — .claude/pipeline/handoffs/TASK-1193-buildmaster.md:400-402 (PART 4 §5: two runs of the staged Shipping build, 2026-09-09 — SaveGames\SiegeDecks.sav 4,391 B 23:25:48 and Config\Windows\GameUserSettings.ini 1,378 B 23:27:44 under C:\Users\<user>\AppData\Local\GitClaudeUnrealTest\Saved\; Windows\GitClaudeUnrealTest\Saved\ absent). Engine rule — C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Core/Private/Misc/Paths.cpp:183-192 (ShouldSaveToUserDir() = FApp::IsInstalled() || -SaveToUserDir || FPlatformProcess::ShouldSaveToUserDir() || -UserDir=), :451-473 (ProjectUserDir() = FPlatformProcess::UserSettingsDir() / FApp::GetProjectName() / when ShouldSaveToUserDir(), :466), :485-494 + :112-115 + :87 (ProjectSavedDir() = ProjectUserDir() + "Saved" + "/"); C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Core/Private/Misc/App.cpp:212-242 (IsInstalled(): bIsInstalled = true under UE_BUILD_SHIPPING && PLATFORM_DESKTOP && !UE_SERVER, :218-219 — this package's reason; the other three disjuncts are false here: no such switch on the no-args launch, GenericPlatformProcess.cpp:98-102 returns false with no Windows override, Engine/Build/InstalledProjectBuild.txt absent from the stage); C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Core/Private/Windows/WindowsPlatformProcess.cpp:1459-1476 (UserSettingsDir() = SHGetKnownFolderPath(FOLDERID_LocalAppData), :1467 = %LOCALAPPDATA%). Note: the stage's Engine/Config/StagedBuild_GitClaudeUnrealTest.ini (present 2026-09-10, 3 B) feeds FPaths::IsStaged() (Paths.cpp:161-181, probe at :174), which is NOT in the ShouldSaveToUserDir() chain; Config/DefaultGame.ini and Config/DefaultEngine.ini carry no SaveToUserDir/UserDir/SavedDir key (grep 2026-09-10, 0 hits) -->
```

### 2a. Part (a) — the measurement

`.claude/pipeline/handoffs/TASK-1193-buildmaster.md:400-402` (PART 4 §5). Quoted: *"Measured after two runs of the staged Shipping build (his 23:2x sitting and the rig's 23:37 launch): `packagedZIPofGame\Windows\GitClaudeUnrealTest\Saved\` DOES NOT EXIST; the game wrote to `C:\Users\wesel\AppData\Local\GitClaudeUnrealTest\Saved\` — `SaveGames\SiegeDecks.sav` 4,391 B (23:25:48) · `Config\Windows\GameUserSettings.ini` 1,378 B (23:27:44) · `GitClaudeUnrealTest_PCD3D_SM6.upipelinecache` (23:27:44) · `Config\CrashReportClient\…\CrashReportClient.ini` (23:37:57, the rig's launch)."*

### 2b. Part (b) — the engine rule, READ from the installed 5.8 source (every line below was read on 2026-09-10 with `sed -n`; nothing is from memory)

**(i) A staged/installed build saves to the user directory.**

`C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Core/Private/Misc/Paths.cpp:183-192`:
```
183 bool FPaths::ShouldSaveToUserDir()
184 {
185 	static bool bShouldSaveToUserDir =
186 		FApp::IsInstalled()
187 		|| FParse::Param(FCommandLine::Get(), TEXT("SaveToUserDir"))
188 		|| FPlatformProcess::ShouldSaveToUserDir()
189 		|| !CustomUserDirArgument().IsEmpty();
190
191 	return bShouldSaveToUserDir;
192 }
```

`Paths.cpp:451-473` (the branch that puts the project's user dir under the platform user-settings dir):
```
451 FString FPaths::ProjectUserDir()
452 {
453 	const FString& UserDirArg = CustomUserDirArgument();
454
455 	if (!UserDirArg.IsEmpty())
456 	{
457 		return UserDirArg;
458 	}
459
460 	if (ShouldSaveToUserDir())
461 	{
462 		// if defined, this will override both saveddirsuffix and enginesaveddirsuffix
463 #ifdef UE_SAVED_DIR_OVERRIDE
464 		return FPaths::Combine(FPlatformProcess::UserSettingsDir(), TEXT(UE_STRINGIZE(UE_SAVED_DIR_OVERRIDE))) + TEXT("/");
465 #else
466 		return FPaths::Combine(FPlatformProcess::UserSettingsDir(), FApp::GetProjectName()) + TEXT("/");
467 #endif
468 	}
469 	else
470 	{
471 		return FPaths::ProjectDir();
472 	}
473 }
```

`Paths.cpp:485-494` + `:112-115` + `:85-87` (why the folder is named `Saved`):
```
485 const FString& FPaths::ProjectSavedDir()
486 {
487 	FStaticData& StaticData = TLazySingleton<FStaticData>::Get();
488 	if (!StaticData.bGameSavedDirInitialized)
489 	{
490 		StaticData.GameSavedDir = UE4Paths_Private::GameSavedDir();
...
112 	FString GameSavedDir()
113 	{
114 		return GetSavedDirSuffix(FPaths::ProjectUserDir(), TEXT("-saveddirsuffix="));
115 	}
...
 85 	FString GetSavedDirSuffix(const FString& BaseDir, const TCHAR* CommandLineArgument)
 86 	{
 87 		FString Result = BaseDir + TEXT("Saved");
```

**Which disjunct of `:186-189` is true for THIS package — `FApp::IsInstalled()`, by the Shipping default.** `C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Core/Private/Misc/App.cpp:212-242`:
```
212 bool FApp::IsInstalled()
213 {
214 	static int32 InstalledState = -1;
215
216 	if (InstalledState == -1)
217 	{
218 #if UE_BUILD_SHIPPING && PLATFORM_DESKTOP && !UE_SERVER
219 		bool bIsInstalled = true;
220 #else
221 		bool bIsInstalled = false;
222 #endif
223
224 #if PLATFORM_DESKTOP
225 		FString InstalledProjectBuildFile = FPaths::RootDir() / TEXT("Engine/Build/InstalledProjectBuild.txt");
226 		FPaths::NormalizeFilename(InstalledProjectBuildFile);
227 		bIsInstalled |= IFileManager::Get().FileExists(*InstalledProjectBuildFile);
228 #endif
229
230 		// Allow commandline options to disable/enable installed engine behavior
231 		if (bIsInstalled)
232 		{
233 			bIsInstalled = !FParse::Param(FCommandLine::Get(), TEXT("NotInstalled"));
234 		}
...
241 	return InstalledState == 1;
242 }
```
The package is `-clientconfig=Shipping` Win64 client (TASK-1193's recipe) ⇒ `UE_BUILD_SHIPPING && PLATFORM_DESKTOP && !UE_SERVER` ⇒ `bIsInstalled = true` at `:219`; no `-NotInstalled` on the no-args launch. The other three disjuncts of `Paths.cpp:186-189` are false here: no `-SaveToUserDir` / `-UserDir=` on a no-args launch; `C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Core/Private/GenericPlatform/GenericPlatformProcess.cpp:98-102` is
```
 98 bool FGenericPlatformProcess::ShouldSaveToUserDir()
 99 {
100 	// default to use the engine/game directories
101 	return false;
102 }
```
with no override in `Public/Windows/WindowsPlatformProcess.h` or `Private/Windows/WindowsPlatformProcess.cpp` (grep, 0 hits); and `packagedZIPofGame\Windows\Engine\Build\InstalledProjectBuild.txt` does not exist (`Engine\Build\` itself is absent from the stage).

**(ii) The user directory on Windows is `%LOCALAPPDATA%`.** `C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Core/Private/Windows/WindowsPlatformProcess.cpp:1459-1476`:
```
1459 const TCHAR* FWindowsPlatformProcess::UserSettingsDir()
1460 {
1461 	static FString WindowsUserSettingsDir;
1462 	if (!WindowsUserSettingsDir.Len())
1463 	{
1464 		TCHAR* UserPath;
1465
1466 		// get the local or locallow AppData directory depending on integrity configuration
1467 		HRESULT Ret = SHGetKnownFolderPath(ShouldExpectLowIntegrityLevel() ? FOLDERID_LocalAppDataLow : FOLDERID_LocalAppData, 0, NULL, &UserPath);
1468 		if (SUCCEEDED(Ret))
1469 		{
1470 			// make the base user dir path
1471 			WindowsUserSettingsDir = FString(UserPath).Replace(TEXT("\\"), TEXT("/")) + TEXT("/");
1472 			CoTaskMemFree(UserPath);
1473 		}
1474 	}
1475 	return *WindowsUserSettingsDir;
1476 }
```
`FOLDERID_LocalAppData` is the known folder `%LOCALAPPDATA%` resolves to; the measured `AppData\Local` (not `LocalLow`) shows the normal-integrity branch fired. Chain: `UserSettingsDir()` (`…/AppData/Local/`) + `GetProjectName()` (`GitClaudeUnrealTest`) + `/` (`Paths.cpp:466`) + `Saved` (`:87`) + `/` (`:107`) = `%LOCALAPPDATA%\GitClaudeUnrealTest\Saved\` — exactly the measured path.

### 2c. ⚠️ Correction to the lead (report, not act — `SC-§101`)

The board's lead and PART 4 §5's sentence *"The staged `Engine\Config\StagedBuild_GitClaudeUnrealTest.ini` marks the build as staged/installed, which routes `ProjectSavedDir` to the user directory"* is **not what the source says**. The `StagedBuild_%s.ini` probe lives in `FPaths::IsStaged()` (`Paths.cpp:161-181`, the `FileExists` at `:174`), which sets `bIsStaged` and nothing else; `IsStaged()` is not referenced by `ShouldSaveToUserDir()`, `ProjectUserDir()` or `FApp::IsInstalled()`. The marker **does exist** at my instant — `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Windows\Engine\Config\StagedBuild_GitClaudeUnrealTest.ini`, 3 bytes (`EF BB BF`, a bare UTF-8 BOM), mtime 2026-09-09 23:37 — but the fact that routes this package's saves to `%LOCALAPPDATA%` is the Shipping configuration (`App.cpp:218-219`). A Development package of the same stage would save beside the executable (`bIsInstalled = false` at `:221`, and neither `InstalledProjectBuild.txt` nor a switch is present). The citation comment states this so the gate does not inherit the routing claim as settled (`SC-§110`).

### 2d. The `Config/` grep, quoted

```
$ grep -n -i "SaveToUserDir\|UserDir\|SavedDir\|saveddirsuffix\|ProjectUserDir" Config/DefaultGame.ini    → (no output) exit=1
$ grep -n -i "SaveToUserDir\|UserDir\|SavedDir\|saveddirsuffix\|ProjectUserDir" Config/DefaultEngine.ini  → (no output) exit=1
```
None found, grep quoted; the engine default applies.

## 3. Placeholder re-count (after the edit; `grep -o "{{SHIP:[A-Za-z0-9_]*}}"`)

| | |
|---|---|
| occurrences | **15** (unchanged) |
| distinct | **9** (unchanged): CHANGED_SINCE 1 · CLOUD_SYNC 2 · CONFIG 3 · DATE 1 · DIFF_BASE 2 · HEAD 2 · SIZE 2 · VERIFIED 1 · ZIP_NAME 1 |
| any other `{{` | 0 |
| `NOT_MEASURED` | 0 |
| `{{SHIP:VERIFIED}}` | untouched (`README-source.md`, *What was verified* — the host's) |

## 4. `<!-- src:` marker count

Re-derived at my instant (`SC-§91`): **332 before → 333 after** (my citation is a new comment on the corrected sentence, which previously had none). The board's "308" is the TASK-1192 relay, not today's count.

## 5. The unified diff (`git diff -U1 -- Docs/Packaging/README-source.md`, run from the project dir; git root is one level up, `SC-§102`)

```
diff --git a/GitClaudeUnrealTest/Docs/Packaging/README-source.md b/GitClaudeUnrealTest/Docs/Packaging/README-source.md
index 496beef..a3ca28e 100644
--- a/GitClaudeUnrealTest/Docs/Packaging/README-source.md
+++ b/GitClaudeUnrealTest/Docs/Packaging/README-source.md
@@ -169,5 +169,8 @@ That route has not been exercised on a packaged Shipping build, so treat it as u
   is recorded in the size line at the top.
-- The game writes its save data (accounts, decks, settings) into
-  `Windows\GitClaudeUnrealTest\Saved\` next to the executable, so extract somewhere you have
-  write permission (not `C:\Program Files`).
+- The game writes its save data (accounts, decks, settings) into your Windows user profile, not
+  next to the executable: `%LOCALAPPDATA%\GitClaudeUnrealTest\Saved\` — normally
+  `C:\Users\<you>\AppData\Local\GitClaudeUnrealTest\Saved\` (typing `%LOCALAPPDATA%` into the
+  File Explorer address bar opens that `Local` folder). Your decks are in
+  `SaveGames\SiegeDecks.sav` and your settings in `Config\Windows\GameUserSettings.ini`; nothing
+  is written beside the executable. <!-- src: [the comment in §2, verbatim, on this one line] -->
 - Carried forward from the 2026-08-29 package: on that machine's first runs the audio device
```
`--stat`: 1 file, +6 / −3; one hunk. File: 81,034 B / 946 lines → 83,081 B / 949 lines; LF, no BOM, 0 CRs before and after (git printed its standing autocrlf warning about a future checkout — that is its config, not a change in the file). Neighbouring lines 169 and 173(→176) untouched.

## 6. Other `Saved` mentions in the file — read, deliberately NOT touched (scope = the one false sentence)

- `:122` — the repo-layout table row *"`Saved/`, `Intermediate/`, … Local build scratch"*: about the source tree, not the package; true as written.
- `:922` (now `:925`) — *"Accounts are stored locally in the `Saved\` folder."*: names no path, so it asserts nothing false; with the fix, "the `Saved\` folder" now resolves to the location *Known notes* gives. Lead for the manager, not a defect: it could say "the same `Saved\` folder as above" on a future doc pass.

## 7. State at my instant

- No `RunUAT` / `UnrealEditor` / `UnrealPak` / `GitClaudeUnrealTest` process alive (the row is parallel-unsafe against a `/ship`; none was running).
- `git status --short`: `M Docs/Packaging/README-source.md` (mine) · `M .claude/pipeline/TASKBOARD.md` (my status flip + the manager's prior dirt) · `M .claude/pipeline/CONVENTIONS.md` (not mine, untouched). Nothing staged, nothing committed.
- What QA should scrutinise: that every `file:line` in the comment resolves to the quoted text above (the engine paths are the installed 5.8 tree, read with `sed -n`); that the diff is the single hunk in §5; that §2c's correction is right — i.e. that `IsStaged()` really has no path into `ShouldSaveToUserDir()`.
