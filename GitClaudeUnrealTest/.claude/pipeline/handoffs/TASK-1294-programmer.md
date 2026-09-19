# TASK-1294 — handoff (gameplay-programmer)

marker `TASK-1294-SUITE-AURA-401` · date 2026-09-18 · status → `ready-for-qa` · gate `TASK-1295` · host `TASK-1298`

**ROUTE TAKEN: EXCLUSION. This is the fix, NOT the fallback.** The fallback (a suite-wide
expected-error / log-suppression pin on the `401` string) was **NOT taken** and was **not
needed** — the exclusion lever was reachable from the suite's own launch lane without
touching `GitClaudeUnrealTest.uproject`.

**⛔ NO HARD STOP.** Cl. (1)'s hard stop did not trigger: the `.uproject` is **not** the only
lever. Its diff is **0** and it was never opened for writing.

**FILES CHANGED — ONE, and it is not in `Source/`:**

| file | insertions | deletions |
|---|---|---|
| `Tools/run_suite_bounded.ps1` | 96 | 0 |

⇒ **Acceptance (5): MY diff touches NO `Source/` file.** Nothing in this row requires a
compile. (The tree *does* show `Source/.../DeckBuilderWidget.{h,cpp}` and
`Content/Blueprints/BP_MenuGameMode.uasset` dirty — those are **`TASK-1307`'s and
`TASK-1296`'s**, not mine. See §6.)

---

## 0. THE ROUTE, MEASURED BEFORE IT WAS CHOSEN (`SC-§101`)

### (i) Where the suite command is composed

⚠️ **`TL-§6`'s named executor `Tools/run_suite_bounded.ps1` EXISTS.** The standing note that
it "DOES NOT EXIST on disk" is **stale and should be retired** — the file is 82 KB, dated
2026-09-09, and is the live executor. It is also no longer true that it has never launched
the editor: its own header block (`:62`) still says *"As of 2026-09-09 this script has NEVER
launched UnrealEditor-Cmd.exe"*, but `Saved/Logs/` holds **15 runs it launched** between
2026-09-14 and 2026-09-18. **I did not edit that stale comment** (out of this row's scope —
flagged here for the manager, `SC-§39.1` cl. 7).

- **`Tools/run_suite_bounded.ps1:323` `function New-EditorCommandLine`** — composes the whole
  command line as one verbatim string. The `$parts` array was, pre-change, `:337-347`.
- **`Tools/run_suite_bounded.ps1:1481`** (pre-change numbering) —
  `$proc = Start-Process -FilePath $EditorCmd -ArgumentList $commandLine -PassThru -WindowStyle Hidden`
- **`Tools/run_suite_bounded.ps1:1403`** — `$EditorCmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'`
- **Corroborated by the runner's own receipt**, which it writes beside every log.
  `Saved/Logs/run_suite_bounded_suite_20260918-161724.log.cmdline`, verbatim:

  ```
  C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "C:\GitProjects\...\GitClaudeUnrealTest.uproject" -ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog="...\run_suite_bounded_suite_20260918-161724.log"
  ```

  ⇒ the composed line and the launched line are the same line. This is the lane.

### (ii) How the Aura plugin is enabled

| source | what it says | verdict |
|---|---|---|
| **`GitClaudeUnrealTest.uproject:54-60`** | `{ "Name": "Aura", "Enabled": true, "SupportedTargetPlatforms": ["Win64","Mac"] }` | **THIS is what enables it.** 🧑 **Jonathan's file, ruling R11 — untouched.** |
| **`Config/DefaultEngine.ini`** | `grep -n "Aura"` → **0 hits**. No plugin-enable section of any kind. | not a lever |
| **`C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/Aura.uplugin`** | no `"EnabledByDefault"` key at all; `"Installed": true`; modules `Aura` + `AuraModelGenerator`, both `"Type": "EditorNoCommandlet"`, `LoadingPhase` `PreDefault`/`Default` | engine-install file, **outside the repo**; editing it would kill Aura for the **GUI editor** too ⇒ refused under cl. (2) / `VER-§7` |

⚠️ Note on `EditorNoCommandlet`: it does **not** save us. `UnrealEditor-Cmd.exe <project>
-ExecCmds=... -nullrhi` is **not** a commandlet (no `-run=`), so `IsRunningCommandlet()` is
false and both modules load. That is measured, not inferred — see the `InternalLoadLibrary`
line in §2.

### (iii) The levers the suite launch lane can actually reach

**`-DisablePlugins=<name>` is a real, command-line-only lever in UE 5.8, and it beats the
`.uproject`.** Read in the engine's own sources (`SC-§105` — never from memory), all in
`C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Projects/Private/`:

1. **`PluginManager.cpp:2043`** — `FPluginManager::ConfigureEnabledPlugins()` calls
   `FindCommandLinePlugins(Context)` **FIRST**, at `:2043`, **before** `FindTargetPlugins(Context)` at `:2049`.
2. **`PluginManager.cpp:1478-1479`** — `FParse::Value(FCommandLine::Get(), InListKey, PluginsListStr, false);`
   then `PluginsListStr.ParseIntoArray(PluginsList, TEXT(","))`. ⇒ the value is taken from
   immediately after `=` with **no space**, and split on **commas**.
3. **`PluginManager.cpp:1587-1598`** — the `DisablePlugins=` branch: builds
   `FPluginReferenceDescriptor(DisablePluginName, /*bEnabled=*/false)` (`:1592`) and records the
   name in `Context.ConfiguredPluginNames` (`:1597`).
4. **`PluginReferenceDescriptor.cpp:41-47`** — `IsEnabledForPlatform()` opens with
   `// If it's not enabled at all, return false` / `if(!bEnabled) { return false; }`. A disabled
   reference therefore hits the `continue` at `PluginManager.cpp:2425-2428` and **never enters
   `EnabledPlugins`**.
5. **Every later source is gated on `!Context.ConfiguredPluginNames.Contains(...)`:** the target
   receipt at **`:1636`**, the **`.uproject` `Plugins` array at `:1709`** (whose source string is
   `"Enabled plugins in .uproject for %s"`, `:1731-1733`), and `.uplugin` `EnabledByDefault` at
   **`:1748`**.

⇒ **the command line overrides `GitClaudeUnrealTest.uproject:54-60` for that one process, and
only that process.** The GUI editor is launched without the flag and is **untouched by
construction** — the flag is typed in exactly one place, inside the suite runner.

**Levers considered and rejected, with the reason:**

| lever | why not |
|---|---|
| edit `GitClaudeUnrealTest.uproject` | 🧑 his file, **ruling R11**; also project-wide ⇒ breaks the GUI editor's Aura (`VER-§7`) |
| edit `Aura.uplugin` `EnabledByDefault` | engine-install file outside the repo; project-wide; same `VER-§7` break |
| `Config/DefaultEngine.ini` | **no config lever exists** — UE 5.8's plugin enable/disable has no ini surface; the only sources are compile-time, command line, target receipt, `.uproject`, `.uplugin` (`PluginManager.cpp:2041-2058`) |
| `-NoEnginePlugins` (`:1607`) | nuclear — disables **all** engine plugins; would take `EnhancedInput`, `StateTree`, `GameplayStateTree`, `ModelingToolsEditorMode` with it |
| expected-error / log-suppression pin | **the declared FALLBACK.** Not taken. It pins exactly one string and leaves the next `LogAura` Error line free to red a random test; it re-buys the defect at the next plugin update. |

### The `Aura` grep over the test sources — and the homonym trap

Corpus: `Source/GitClaudeUnrealTest/Siegebound/Tests/`, **47 files, 561 tests**.

- `grep -rn "Aura"` over that corpus → **1 hit**, and it is a **COMMENT**:
  `SiegeMenuInputTest.cpp:35`: `* verifier uses (`UEnhancedInputLocalPlayerSubsystem::InjectInputForAction` — Aura's`
- ⇒ **zero code dependency on the Aura plugin anywhere in the automation suite.** The manager's
  steer is vindicated by measurement, not by assumption.
- ⚠️ **`grep -rn "Aura" Source/` returns 61** — do not quote that number. **All 60 others are
  the game's own `WarBannerAura` / `AuraRadius` / `SetAuraDamageBonus` gameplay concept**, an
  unrelated homonym (`HeroCharacter.cpp:915,1013,1081,1252,1268-1275`, `DeckBuilderWidget.cpp:130`).
  This is `SC-§121`'s exact shape: a real identifier on a real code line that every clause reads
  as a legitimate hit **because it is one**.
- **Needle positive control over that same corpus** (`SC-§39` — a needle that matches nothing
  is indistinguishable from a true zero): `IMPLEMENT_SIMPLE_AUTOMATION_TEST` → **561**,
  `Siegebound.` → **651**. The corpus is readable and the greps fire.

---

## 1. THE CHANGE

`Tools/run_suite_bounded.ps1`, two hunks, +96/-0:

1. **`New-EditorCommandLine`** — `'-DisablePlugins=Aura'` added to `$parts` (now **`:400`**),
   between `-NoLiveCoding` and `-log`, preceded by a ~55-line comment block carrying: the
   defect, the three-state model, the engine `file:line` chain above, why this lever and not
   the `.uproject`, and an explicit **"DO NOT simplify this to an expected-error pin"** rider
   naming that as the fallback that was *not* taken (cl. (1) / `SC-§70`).
   **Unconditional, both lanes** — the Command lane boots the same `-nullrhi` editor and
   inherits the same race, and neither lane needs Aura.

2. **A new self-test section `5b`** (`:1236-1276`; header `:1238`, `Write-Head` `:1248`, last case `:1275`) with two cases.

**Verified live, real file, `-DryRun` (returns before any launch, before any `Remove-Item`):**

```
UnrealEditor-Cmd.exe "...\GitClaudeUnrealTest.uproject" -ExecCmds="Automation RunTests Siegebound;Quit"
  -nullrhi -unattended -nopause -nosplash -NoLiveCoding -DisablePlugins=Aura -log -abslog="..."
```

One token, no space after `=`, no comma. ⛔ No log file was created by the dry run (confirmed: 0).

---

## 2. THE POSITIVE CONTROL — AND IT FIRED

### 2a. The guard over the flag (`SC-§39` / `SHIP-§9`)

The flag's absence is **silent**: the suite still boots, still runs 561 tests, still prints a
total — it just goes back to rolling dice. Nothing downstream would notice. So section `5b`
runs the **same predicate** that judges the real command line against **four synthetic
degenerate lines** and requires it to refuse every one (`flag deleted` · `space after =` ·
`value emptied` · `name mistyped`), **and** to still accept the real one — the controls go
**through** the predicate, never beside it.

`.\Tools\run_suite_bounded.ps1 -SelfTest` → **`54 / 54` cases produced their EXPECTED result**
(was 52 before this row; +2). No engine launched, no process started.

**⭐ AND THE GUARD WAS SEEN TO GO RED.** A throwaway copy (scratchpad, fixtures copied
alongside; the repo file was never mutated and the copy is deleted) was mutated three ways at
the **`$parts` site itself**:

| mutation | self-test result |
|---|---|
| **M1** — the `'-DisablePlugins=Aura',` line deleted outright | **52 / 54, exit 5** — both new cases RED |
| **M2** — `'-DisablePlugins= Aura',` (space after `=`) | **52 / 54, exit 5** — both new cases RED |
| **M3** — `'-DisablePlugins=',` (value emptied) | **52 / 54, exit 5** — both new cases RED |

Each failure printed the offending command line in full. Repo file re-verified intact after:
`Tools/run_suite_bounded.ps1:400` still `'-DisablePlugins=Aura',`.

### 2b. The log instrument (the one `TASK-1295` check (4) needs)

Every suite log on disk, `401` count · total `LogAura` count · reds **by name**:

| run | bare `401` | `LogAura` total | started/completed | reds by name |
|---|---|---|---|---|
| 20260909-045216 | 0 | **0** | 555/555 | — |
| 20260909-045412 | 0 | **0** | 555/555 | — |
| 20260909-045537 | 0 | **0** | 134/133 | — (truncated run) |
| 20260914-143038 | 2 | 22 | 555/555 | — |
| 20260914-143207 | 4 | 27 | 556/556 | `DownTwiceThenAcceptOpensDeckBuilder`, `TheThreeWheelMeaningsAreThreeDistinctRows` |
| 20260914-143418 | 4 | 27 | 556/556 | `DownTwiceThenAcceptOpensDeckBuilder`, `MigrationFreshSave` |
| 20260914-145342 | 2 | 22 | 556/556 | — |
| 20260914-145446 | 2 | 22 | 556/556 | — |
| 20260914-211849 | 2 | 22 | 558/558 | — |
| 20260914-211943 | 2 | 22 | 558/558 | — |
| 20260914-222703 | 2 | **31** | 559/559 | — |
| 20260914-222813 | 2 | 22 | 559/559 | — |
| 20260917-211827 | 4 | 27 | 559/559 | `IllegalDeckCannotBecomeActive` |
| 20260917-211945 | 4 | 27 | 559/559 | `MigrationOverflow` |
| 20260917-212139 | **0** | **16** | 561/561 | — |
| 20260917-212229 | **0** | **18** | 561/561 | — |
| 20260918-161600 | 2 | 22 | 561/561 | — |
| 20260918-161724 | **0** | **18** | 561/561 | — |

**THE FIRING POSITIVE CONTROL, and it fires 15 times:** `grep -c "LogAura"` returns
**16–31** on every run since the plugin was installed. The instrument can return non-zero.

**THE NEGATIVE CONTROL, and it is real history, not a synthetic:** the three 2026-09-09 runs
**pre-date the Aura install** and show `LogAura` = **0**. ⇒ a total of 0 is a *previously
observed* state in this exact log shape, produced by genuine plugin absence. **That is
precisely the state the fix must reproduce.**

**⭐⭐ AND IT CLOSES `CONSEQUENCE 2`'s HOLE — MEASURED, ON THE THREE RUNS THAT WOULD HAVE LIED.**
Runs `20260917-212139`, `20260917-212229` and `20260918-161724` are the "`401` = 0 with **no
fix applied**" runs. The bare count calls them clean. The **structural** count calls them
`LogAura` = **16 / 18 / 18 ≠ 0** — *the plugin loaded and talked, this run was merely quiet*.
⇒ **the discriminator the row demanded exists, and it discriminates correctly on exactly the
three runs that produced the false green.**

**Plugin-enumeration evidence to demand post-change** (present in every loaded run, e.g.
`20260918-161600`):

- `:147` `LogPluginManager: Mounting Engine plugin Aura`
- `:1452` `LogModuleManager: InternalLoadLibrary: 'Aura' ('C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/Binaries/Win64/UnrealEditor-Aura.dll')`
- `:1644` the same for `AuraModelGenerator`
- `:2491` `LogAura: StartIndexing called at: 16:16:24:402` → `:4669` the `401`, **21 s later**

---

## 3. ⭐⭐ NEW FINDING — THE ROW'S OWN ACCEPTANCE NEEDLE IS CONFLATED, AND THE SEPARATION `CONSEQUENCE 3` CALLED UN-ISOLATED IS VISIBLE IN THE LOG BY PREFIX

`CONSEQUENCE 3` says presence and ability-to-red are separable "on conditions we have **not**
isolated". **They are separable in the log, by prefix, and always were.** The `401` text
appears under **two different categories**:

```
[..]LogAura: Error: Response code: 401                                  <- THE FAULT (Aura)
[..]LogAutomationController: Error: LogAura: Response code: 401 [log]   <- THE CAPTURE (the controller claiming it)
```

Split across all 18 logs:

| | `LogAura:` side (**FAULT**) | `LogAutomationController:` side (**CAPTURE**) |
|---|---|---|
| plugin loaded, no red (11 runs) | **2**, every time | **0**, every time |
| plugin loaded, red (4 runs) | **2**, every time | **2**, every time |
| plugin absent / quiet (3+3 runs) | **0** | **0** |

**Invariant, checked on all 18 logs, zero exceptions: `bare 401 == FAULT + CAPTURE`.**

⇒ **The "2 vs 4" that has been read as *"Aura fired twice"* is not a second burst. It is the
automation controller copying the same two lines into a test's error list.** The row's
prescribed needle `grep -c 'Response code: 401'` is **unanchored across both categories**: it
reads **2** when the fault fires unclaimed and **4** when it is claimed. It has been read all
along as *"did the fault fire"*; it has actually been reading *"did it fire, **and** was it
claimed"*, summed. (`SC-§121`'s shape again — a real hit on a real line, in the wrong role.)

**The practical payoff, and it is free:** `grep -cE "\]LogAutomationController: .*Response code: 401"`
is a **direct, zero-inference instrument for "did the 401 red a test"** — 2 iff yes, 0 iff no,
4/4 and 11/11 with no exceptions. Nobody has to adjudicate a red by timestamp again.

⚠️ **Stated at its true strength (`SC-§119` cl. 8):** this is a **measured correlate over 18
runs**, and it tells us *whether* the controller claimed the line — it does **not** explain
*why* it claims it sometimes. The mechanism remains un-isolated. It is also **moot under the
fix**, which removes the fault entirely; it is offered so the *host* can adjudicate cheaply
and so the manager can correct the acceptance needle on the row.

*(Footnote: `20260914-143207` / `-143418` show 2 reds but CAPTURE = 2. One red in each —
`DownTwiceThenAcceptOpensDeckBuilder`, present in **both** runs — is a **genuine** failing test,
not the race. That the split still isolates the 401-attributed red from a real one is further
corroboration.)*

---

## 4. ⛔ WHAT I DID **NOT** DO — ACCEPTANCE (3)'s THREE SUITE RUNS

**I did not run the suite. Not once.** Stated plainly rather than worked around:

1. My dispatch fences it: *"⛔ Do NOT compile — `TASK-1298`/`1309` hosts own that."*
2. The row is `parallel-safe: ⛔ NO vs any compile/verify task`, and **two sibling lanes are
   editing `Source/` right now** (`TASK-1307`, `TASK-1296` — both dirty in the tree, §6). A suite
   run now would measure a half-edited tree against stale binaries.
3. **The GUI editor (PID 1812) is UP.** The runner warns that a second instance contends for
   `Saved/`, DDC and the asset registry, and correctly refuses to kill it. Closing it is not
   this row's grant.
4. **`TASK-1298` cl. (3) already owns the ×3**, and says so by name: *"THE SUITE — THREE RUNS,
   NOT TWO, AND THE REASON IS ON `TASK-1294`'s ROW."*

⇒ **`CONSEQUENCE 1` re-affirmed, in writing, so nobody trims it: the ×3 on `TASK-1298` is
CONFIRMED and is NOT optimisable back to ×2.** This race has hidden from a two-run sample on
**five** separate occasions.

### The exact greps the host must run — per log, all four, ⛔ never a single number

```bash
grep -c "LogAura"                                        <log>   # STRUCTURAL PROOF. MUST be 0.
grep -c "Mounting Engine plugin Aura"                    <log>   # MUST be 0.
grep -cE "\]LogAura: .*Response code: 401"               <log>   # the FAULT.    MUST be 0.
grep -cE "\]LogAutomationController: .*Response code: 401" <log>  # the CAPTURE.  MUST be 0.
grep -o "Result={Fail} Name={[^}]*}" <log> | sed 's/.*Name={//;s/}//' | sort -u   # reds BY NAME
grep -c "Test Started\." <log>; grep -c "Test Completed\. Result=" <log>          # total
```

**Pass shape per run:** `LogAura` **0** · `Mounting Engine plugin Aura` **0** · FAULT **0** ·
CAPTURE **0** · reds **none** · **561/561**.

⛔ **A bare `401` count of 0 is NOT a pass on its own** — it has been 0 three times with no fix
applied. **`grep -c "LogAura" == 0` is the claim; everything else corroborates it.**

**Baseline for acceptance (4), cross-checked two independent ways:** **561** discovered /
**561** passed — from the last three logs (`212229`, `161600`, `161724`, all 561/561) **and**
from `grep -rc IMPLEMENT_SIMPLE_AUTOMATION_TEST` over the 47 test files = **561**. This row
adds no test and removes none; **any total ≠ 561 is a STOP.**

---

## 5. THE FENCES (cl. (2) / `TASK-1295` check (3)) — each measured, not asserted

| fence | measured |
|---|---|
| `GitClaudeUnrealTest.uproject` diff | **0** lines |
| `.claude/settings.local.json` diff | **0** lines |
| `Saved/**` in porcelain | **0** |
| credential needle `grep -c 'eyJ'` in my diff | **0** |
| tests added / removed / renamed / reordered | **0** — `Source/` untouched by me |
| assertions changed | **0** |
| GUI editor's Aura | **untouched** — the flag exists only inside `New-EditorCommandLine`; the GUI editor is launched without it (`VER-§7` intact) |
| any auth/credential added anywhere | **none** |

---

## 6. `git status --porcelain` (from the git root, **ONE LEVEL UP** — `SC-§102`)

```
 M GitClaudeUnrealTest/Content/Blueprints/BP_MenuGameMode.uasset          <- TASK-1296, NOT mine
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp   <- TASK-1307, NOT mine
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h     <- TASK-1307, NOT mine
 M GitClaudeUnrealTest/Tools/run_suite_bounded.ps1                        <- MINE, the only one
```

**File sets are disjoint as designed — confirmed live, not assumed.** I read
`SiegeMenuInputTest.cpp:35` (one grep hit) but **never opened it for writing**; its diff is 0.

---

## 7. FOR QA TO SCRUTINISE

1. **The strongest claim in this handoff is a `file:line` chain in the engine, not in our repo.**
   Re-read §0(iii) items 1–5 against
   `C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Projects/Private/PluginManager.cpp`
   and `PluginReferenceDescriptor.cpp`. If the ordering claim (command line **before**
   `.uproject`) is wrong, the fix silently does nothing — and it would do nothing **quietly**,
   which is exactly this row's disease.
2. **`§3`'s split is new and I am the only one who has looked at it.** The invariant
   `bare == FAULT + CAPTURE` is reproducible in one loop over `Saved/Logs/run_suite_bounded_suite_*.log`.
   Try to falsify it.
3. **I did not run the suite (§4).** If check (4) requires three runs *in this handoff*, that is
   a genuine gap and I would rather it be named than papered over — but the three runs belong to
   `TASK-1298` cl. (3) by the board's own construction, and running them here would have measured
   a half-edited tree. Route the decision rather than failing it silently.
4. **`grep -rn "Aura" Source/` = 61 is a trap** (§0). Only **1** is in the test corpus and it is
   a comment. If a reviewer quotes 61, they have counted `WarBannerAura`.
5. **Two stale statements I deliberately did not edit:** `TL-§6`'s *"`run_suite_bounded.ps1` does
   not exist"* (it does), and the script's own `:62` *"has NEVER launched UnrealEditor-Cmd.exe"*
   (it has, 15 times). Both are out of this row's `names:` scope. Flagged for the manager.
