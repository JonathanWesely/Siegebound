# TASK-698 — [NAME-1] THE DISPLAYED IDENTITY — handoff (gameplay-programmer)

**Date:** 2026-08-30 · **Status:** ready-for-integration (TASK-699's pre-cook diff read is the review gate — manager ruling on 698)
**Law:** `PKG-§8` · `PKG-§9a` · `PKG-§5`
**Fences honoured:** two ini files only · zero C++ · zero `.uasset` · no compile · no editor · no MCP · no Git · no test changes.

## Files touched (complete list)

| File | Change |
|---|---|
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Config\DefaultGame.ini` | `ProjectName` replaced; `ProjectDisplayedTitle` + `Description` added; provenance comments |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Config\DefaultEngine.ini` | `bUseLoggingInShipping=True` added (⚠️ see FINDING 1); provenance comment above `bAllowHighDPIInGameMode` |

Nothing else. `git diff --name-only` shows only these two Config files from this task (`CONVENTIONS.md`/`TASKBOARD.md` were already dirty from the manager before dispatch).

## 1. Per-key BEFORE / AFTER

### `Config/DefaultGame.ini` — `[/Script/EngineSettings.GeneralProjectSettings]`

| Key | BEFORE | AFTER |
|---|---|---|
| `ProjectName` | `Third Person Game Template` | `Siegebound` |
| `ProjectDisplayedTitle` | *(absent)* | `NSLOCTEXT("[/Script/EngineSettings]", "ProjectDisplayedTitle", "Siegebound")` |
| `Description` | *(absent)* | `Two heroes, two castles, one arena: spend gold from a six-card hand to summon units, raise towers, cast spells and fight as your hero until the enemy castle falls.` |
| `ProjectID` | `A498658547E539A761E904989C4C2DD8` | **UNCHANGED** (explicitly forbidden) |

### `Config/DefaultEngine.ini`

| Key | Section | BEFORE | AFTER |
|---|---|---|---|
| `bUseLoggingInShipping` | `[/Script/Engine.Engine]` | *(absent — zero entries in the file, as `PKG-§9a` measured)* | `True` — ⚠️ **inert, see FINDING 1** |
| `bAllowHighDPIInGameMode` | `[/Script/Engine.UserInterfaceSettings]` | `True` | `True` — **byte-identical, NOT moved, NOT changed**; a 7-line provenance comment was added directly above it |

### ⛔ What was NOT touched, deliberately
`GitClaudeUnrealTest.uproject` · `*.Build.cs` · `*.Target.cs` · any folder name · the `GITCLAUDEUNREALTEST_API` macro. Per manager ruling 1 + row **S1**, this is the **displayed identity only**. Residue, stated rather than left to be discovered: **the shipped exe stays `GitClaudeUnrealTest.exe` and the staged folder stays `Windows\GitClaudeUnrealTest\`.**

## 2. ⚠️⚠️ FINDING 1 — `bUseLoggingInShipping` IS NOT AN INI SETTING. ROUTE 1 IS NOT AVAILABLE ON THIS MACHINE.

**`PKG-§9a` and this task's spec both assert the ini mechanism. Measured against UE 5.8 source, that assertion is false, and the key I was told to add cannot do anything.** Reported rather than quietly implemented, because a dead config line that everyone believes is live is precisely the "silent log read as a clean run" inversion `PKG-§9a` exists to prevent — one step upstream of the cook.

**Evidence (engine source read, not recalled):**

| Claim | Evidence |
|---|---|
| It is a UBT **C# target property**, not a config key | `TargetRules.cs:1610-1611` — `[RequiresUniqueBuildEnvironment] public bool bUseLoggingInShipping { get; set; }` |
| It has **no ini binding** | No `[ConfigFile]` / `[XmlConfigFile]` attribute on the property (contrast `UEBuildWindows.cs:539` which *does* carry `[ConfigFile(ConfigHierarchyType.Game, …)]`) |
| **No C++ reads it** | `bUseLoggingInShipping` across `Engine/Source`: hits are UBT `.cs` + generated `.xml` docs **only** |
| **No engine ini precedent** | zero hits across `Engine/Config` |
| What it actually drives | `UEBuildTarget.cs:6452-6458` → `USE_LOGGING_IN_SHIPPING=1` → `Build.h:351` `NO_LOGGING = !USE_LOGGING_IN_SHIPPING` (compile-time) |

**And the correct fix would ALSO fail here** — this matters more than the first half:

- The real lever is one line in `Source/GitClaudeUnrealTest.Target.cs` (which currently sets only `Type`, `DefaultBuildSettings=V7`, `IncludeOrderVersion`, `ExtraModuleNames`).
- But the property is `[RequiresUniqueBuildEnvironment]`, and this target resolves to **`TargetBuildEnvironment.Shared`**: `TargetRules.cs:2859` returns `Shared` when `Unreal.IsEngineInstalled()`, and **`C:\Program Files\Epic Games\UE_5.8\Engine\Build\InstalledBuild.txt` exists** (measured) — this is a Launcher install.
- Modifying a `RequiresUniqueBuildEnvironment` property under a shared environment throws at `UEBuildTarget.cs:1512-1516`: *"…modifies the values of properties: […]. This is not allowed, as … has build products in common with …"* ⇒ **the cook fails outright.**
- The engine's own escape hatches (`BuildEnvironment = TargetBuildEnvironment.Unique` or `bOverrideBuildEnvironment = true`) are **not free**: the first requires building engine modules an installed engine only supplies precompiled; the second forces a define that Epic's precompiled Shipping engine libs were built *without*, i.e. a deliberately inconsistent build. **Neither is a decision an implementer should take on a relay** — and both are outside this task's fence anyway. The fence and the engineering agree here; I am not citing the fence to dodge the work.

### ⇒ RECOMMENDATION TO TASK-699 (declare BEFORE the cook, per `PKG-§9a`)
**Declare ROUTE 2** — process liveness + **window title** + **rendered-pixel** proof of the arena/deck/HUD. `PKG-§9a` already calls route 2 "the honest fallback, not a downgrade", and `PKG-§6a`'s arena requirement **does not weaken because the instrument changed**. ⛔ Do not declare route 1 on the strength of the ini line below it.

### The `PKG-§9a` route-1 cost statement (stated as the spec requires, for the record)
Route 1, *had it worked*, ships in the player's build: logging stays compiled in, the artifact is slightly larger and slightly slower, and **the game writes log files onto the player's disk**. That was the accepted cost of the proceeding default, and **row S3 remains Jonathan's strike route.** As landed the cost is **zero**, because the line is inert — which is the same thing as saying **the benefit is zero too.**

### Why I wrote the line at all
Spec item (2) mandates it and I have no authority to silently drop a specced item. It is written **with the measurement in a comment block directly above it**, so it cannot be mistaken for a working instrument. Removing the line is harmless; **removing that comment is not.** If the orchestrator prefers the line deleted outright, that is a one-line follow-up.

## 3. Jonathan's claimed line — the provenance text as landed (`PKG-§5`)

Above `Config/DefaultEngine.ini` → `[/Script/Engine.UserInterfaceSettings]` → `bAllowHighDPIInGameMode=True`, verbatim as written:

```
; JONATHAN'S SETTING - CLAIMED, DATED, AND IT STAYS (2026-08-30, CONVENTIONS
; PKG-5). His verbatim reason: "I set that because it fixed a bug with screen
; recording, so leave it as it is."
; Recorded by TASK-698 because three commits in a row excluded this line as
; unexplained foreign dirt: an unexplained config line is indistinguishable from
; an accident, so the provenance IS the fix. NO future audit may strip it.
; TASK-698 added this comment ONLY - the setting itself was not moved or changed.
```

The setting's own line is byte-identical to before. Per manager ruling, it **commits with TASK-700 with its reason named in the commit message**.

## 4. Predictions for TASK-699 to TEST (row **S2** is settled by measurement, ⛔ not by this table)

| # | Prediction | Confidence | Basis |
|---|---|---|---|
| P1 | The standalone window title reads **`Siegebound`** | **High** | `GameEngine.cpp:488` reads `ProjectDisplayedTitle` from `GGameIni` via `GConfig->GetText`; non-empty ⇒ it replaces the `{GameName}` fallback (`:489`) |
| P2 | The exe's **"Product name"** and **"File description"** both read **`Siegebound`** | **Medium-high** | Chain read end to end: `DefaultGame.ini ProjectName` → `WindowsTargetRules.ProductName` (`UEBuildWindows.cs:539-540`) → `PROJECT_PRODUCT_NAME` (`VCToolChain.cs:2989-2991`) → `BUILD_PROJECT_PRODUCT_NAME` (`Default.rc2:26-30`) → the `ProductName` **and** `FileDescription` version-resource fields (`Default.rc2:76,78`). The `VCToolChain.cs:2977` gate is `!bUseSharedBuildEnvironment`, and `UEBuildBinary.cs:743-745` **forces that false** for a binary whose intermediate dir sits under the project — which is the monolithic game exe. That same chain is the best explanation for why it read `Third Person Game Template` before. |
| P3 | **"Original filename"** and **"Internal name"** still read **`GitClaudeUnrealTest`** | **High** | `PROJECT_PRODUCT_IDENTIFIER` = the `.uproject` filename (`VCToolChain.cs:2994-2997`), consumed at `Default.rc2:38-42,79`. **This is the row-S1 residue, expected and correct — ⛔ not a defect to "fix".** |
| P4 | **"Company name"** and **"Legal copyright"** still read the **Epic defaults** | **High** | `Default.rc2` falls back to `EPIC_COPYRIGHT_STRING` / Epic company when `PROJECT_COMPANY_NAME` / `PROJECT_COPYRIGHT_STRING` are undefined, and both come from `DefaultGame.ini` keys `CompanyName` / `CopyrightNotice` (`UEBuildWindows.cs:527-534`) which **are absent** — see the deliberate deviation below. |
| P5 | The Shipping log is **silent** | **High** | FINDING 1. ⛔ `PKG-§9a`'s ban applies: a silent log is not a clean run. |

If P2 fails, that is **a finding for Jonathan (row S2)** — ⛔ never a silent fix and ⛔ never a hack into engine files.

## 5. Deviations from spec (SC-§15)

1. **`bUseLoggingInShipping` written but INERT** — FINDING 1 above. Complied with the letter of the spec; the mechanism it assumes does not exist, and the correct lever fails the cook on an installed engine. Route 2 recommended to 699.
2. **"Companion description fields": I added `Description` only, and deliberately did NOT add `CompanyName`, `CopyrightNotice`, `ProjectVersion`, `Homepage`, `SupportContact` or `LicensingTerms`.** `Description` is descriptive and I derived it from `Docs/GDD.md` §1 rather than inventing it. The others are **identity and legal claims** — a copyright holder and a company name are Jonathan's to assert, not an agent's to infer, and the same ruling-1 logic ("a legitimate thing to want and an illegitimate thing to INFER") applies. ⭐ **This is a live, cheap opportunity, now measured:** `CompanyName` and `CopyrightNotice` feed the exe's **"Company name"** and **"Legal copyright"** properties directly (`UEBuildWindows.cs:527-534` → `Default.rc2:74-75`), which today read **Epic Games**. Two ini lines on his word — suggest a FOR-JONATHAN row.
3. **`ProjectDisplayedTitle` uses a literal, not the `{GameName}` token.** The token expands to `FApp::GetProjectName()` = `GitClaudeUnrealTest` (`GameEngine.cpp:504`) — the exact string this task removes.
4. **Comments added beyond the one mandated provenance comment.** The ini-comment style is established house practice in `DefaultEngine.ini` (TASK-027/217/349/530 blocks). All additions are pure ASCII (verified by grep; the only non-ASCII lines in the file are pre-existing).

## 6. What QA / TASK-699 should scrutinise

- **The route-1 declaration.** Read FINDING 1 before declaring an evidence route. This is the one thing in this handoff that changes what 699 does.
- **`ProjectDisplayedTitle`'s `NSLOCTEXT` form.** It is what UE itself writes for config `FText` and `GConfig->GetText` parses it — but it is the only value here with non-trivial parsing. If the title comes up empty at boot, this line is the suspect (a bare `ProjectDisplayedTitle=Siegebound` is the fallback, via `FTextStringHelper`'s literal path).
- **`bAllowHighDPIInGameMode` is byte-identical** — confirm the diff shows only added comment lines around it.
- **Scope:** confirm the diff touches nothing but these two Config files, and that `ProjectID` is unchanged.
- **A future editor re-save of these ini files can strip comments.** If Project Settings is ever edited in-editor, the provenance blocks are at risk — worth a glance after any editor session that touches project settings.
