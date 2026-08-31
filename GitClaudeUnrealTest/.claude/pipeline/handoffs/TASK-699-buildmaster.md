# TASK-699 — build-master handoff — THE SHIPPING COOK

**Verdict: COOK PASSED · BOOT-VERIFY INCOMPLETE (`PKG-§6a` arena NOT reached in the Shipping binary).**
Law: `PKG-§2a` · `§5a` · `§6a` · `§7a` · `§8` · `§9`. Date 2026-08-30. ⛔ No zip, no README, no commit, no push (TASK-700 owns those).

---

## 0. PRE-COOK GATES

### 0a. TASK-698 diff read against `PKG-§8`/`§9a` — **IN SCOPE, nothing bounced**
`Config/DefaultGame.ini`: `ProjectName=Siegebound` (replacing `Third Person Game Template`), `ProjectDisplayedTitle` (NSLOCTEXT), `Description` (companion field, `PKG-§8` permits), **`ProjectID` untouched** ✅.
`Config/DefaultEngine.ini`: the inert `bUseLoggingInShipping=True` with its explanatory comment, and the `bAllowHighDPIInGameMode=True` provenance comment (comment-only; Jonathan's ruling honored, `PKG-§5`).
⇒ **`PKG-§9a` ROUTE 2 DECLARED BEFORE THE COOK**, per 698's measured finding. Route 1 is unavailable.

### 0b. `PKG-§7a` fence — **PROVEN, but §7a's stated mechanism is WRONG IN BOTH HALVES** ⚠️
| §7a claims | Measured this run |
|---|---|
| the folder is *"OUTSIDE the work tree… the parent of the git root (`…/GitClaudeUnrealTesting/GitClaudeUnrealTest/`)"* | **FALSE.** `git rev-parse --show-toplevel` = `C:/GitProjects/GitHub/GitClaudeUnrealTesting`; no nested `.git`. `packagedZIPofGame/` is **INSIDE** the work tree, a sibling of the project folder. |
| *"the root `.gitignore` CONTAINS NO `packagedZIPofGame` line (grepped, zero hits)"* | **FALSE.** `.gitignore:19` = `packagedZIPofGame/`, under a comment block describing the fence. TASK-694's own board entry records adding it. |

✅ **The fence HOLDS via route (ii) — a live ignore rule**: `git check-ignore -v` matched folder and inner paths; `git ls-files packagedZIPofGame` = **0**; re-checked post-cook, the TASK-683 auto-stage trap did **not** fire.
⚖️ **The law saved itself.** §7a ruled the fence as a *property proven per run, never a path* — which is exactly why stale prose did not become an unfenced multi-GB cook. **The prose still needs a manager rider.**

### 0c. `L_Arena` (ROT-§2) — `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58` ✅ ledger match, `git status` clean, mtime Aug 27 — **never opened, never saved.**

---

## 1. THE COOK — three passes, and the two failures are worth more than the success

⛔ **The exit-code-lie law fired LIVE, twice**: my wrapper returned **exit 0** on a run whose log said `BUILD FAILED`. Every verdict below is UAT's own line.

**Pass 1 — `Result: Failed (OtherCompilationError)` in 1.87s.**
```
Unable to build while Live Coding is active. Exit the editor and game, or press Ctrl+Alt+F11 …
```
⚠️ **Read WHERE it fired.** UAT builds **two** targets, and the block hit `GitClaudeUnrealTestEditor Win64 Development` — which UAT builds only so it can run the cook commandlet. **The Shipping monolithic target was never reached.**
⇒ Added **`-skipbuildeditor`**. Justified by measurement, not convenience: **zero `Source/` changes were pending** at cook time, so the existing editor binary was code-current; UBT wanted a rebuild only because *"config setting changed"* (698's ini edits), and an ini change needs no C++ relink. The Shipping game target still built **fresh, with the new inis**.
✅ **Manager ruling 12's hypothesis CONFIRMED: the Shipping monolithic target does NOT collide with a running editor.** The bounce was never needed. **No editor was closed, bounced, or touched. PID 17044 was alive at start and alive at finish.**

**Pass 2 — cook ran to `LogCook: Display: Done!` and STILL failed:** `ExitCode=25 (Error_UnknownCookFailure)`.
Sole cause, from the commandlet's own summary:
```
LogHttpListener: Error: HttpListener unable to bind to 127.0.0.1:8000
LogInit: Display: Failure - 1 error(s), 3 warning(s)
```
The cook commandlet is *itself* an editor-shaped node that auto-starts the MCP listener on **port 8000 — which the running editor owns**. One logged error ⇒ commandlet exit 1 ⇒ UAT calls it a cook failure. **The node-identity law in a new form: not just "don't send editor calls to a packaged client", but "two editor-shaped nodes cannot share port 8000."**
⇒ Fixed with a **per-process switch**, read at engine source (`ModelContextProtocolSettings.cpp:19` parses it; `ModelContextProtocolServer.cpp:445` documents it): **`-ModelContextProtocolPort=8077`**, passed through `-AdditionalCookerOptions`. ⛔ No repo file edited, ⛔ no plugin surgery, ⛔ no error suppression, ⛔ editor's own node on 8000 untouched.

**Pass 3 — ✅ `BUILD SUCCESSFUL`.** `LogInit: Display: Success - **0 error(s)**, 3 warning(s)` (errors 1 → 0 — the diagnosis was exact). All four phases: BUILD → COOK → STAGE → ARCHIVE COMPLETED. 1326 packages cooked.
The 3 warnings are the known-benign set (2× navmesh serialize → rebuilt at runtime, 1× UE-EULA LLM notice).

**⛔ No Shipping-specific compile failure occurred.** The monolithic Shipping target, the `!UE_BUILD_SHIPPING`-guarded code and the llama plugin all built clean. ⛔ Nothing was `#if`'d around.

### The recipe actually used (`PKG-§5a`/`§9e` — pinned, not re-derived)
Maps allowlist: `/Game/Maps/L_MainMenu` + `/Game/Maps/L_Arena` **only**.
`-COOKDIR` via `-AdditionalCookerOptions`: `Data · UI · Blueprints · Input · Characters · Meshes · Materials · Textures · VFX · Audio · LevelPrototyping`.
Only deltas vs the proven TASK-696 pass-2 line: `-clientconfig=Shipping`, `-skipbuildeditor`, `-ModelContextProtocolPort=8077`. **The content recipe is byte-for-byte the pinned one.**

---

## 2. `PKG-§6a` BOOT-VERIFY — ⚠️ **INCOMPLETE. THE ARENA WAS NOT REACHED IN THE SHIPPING BINARY.**

⛔ **I am not reporting a pass.** §6a says a menu is not a pass, and it says the requirement does not weaken because the instrument changed. It did not weaken here; **I could not meet it.**

### What the SHIPPING binary DID prove (route 2: process + title + pixels)
- Launched as a double-click (root shim, no args) **and** direct: process lives, stable **81 s**, ~1.4–1.6 GB working set, no crash, no crash dump.
- **WINDOW TITLE = `Siegebound`** ✅ — 698's `GameEngine.cpp:488` prediction **CONFIRMED**.
- **Rendered pixels**: menu renders correctly with all **7 entries** (Play (vs Bot) · Sandbox (No Bot) · Deck Builder · Multiplayer · Settings · Login · Quit).
- Captures: `shot-menu2-t18s.png`, `shot-arena-t25s.png`, `shot-arena-t45s.png`, `shot-arena-execcmds-t45s.png`.

### Why the arena was not reached — measured, with a self-correction
The Shipping binary **ignores the command-line map argument**. Tried and captured: `/Game/Maps/L_Arena`, short `L_Arena`, and `-ExecCmds="open /Game/Maps/L_Arena"`. All three booted to the menu.

⚠️ **I initially treated a bogus-map control as decisive and it was NOT** — "arg ignored" and "arg honored but map missing" both predict a menu fallback. Recorded because the wrong inference would have hidden a `PKG-§5a`-class defect. The real discriminator was the container listing:

✅ **`UnrealPak -List` on the shipped container proves the content is THERE** (3324 files, 1,074,909,575 B):
- `Content/Maps/L_Arena.umap` — 64,920 B ✅ · `L_MainMenu.umap` — 4,671 B ✅
- **The five `PKG-§5a` pass-1 casualties ALL PRESENT**: `DT_Cards` · `WBP_HUD` · `BP_HeroCharacter` · `BP_CommanderNpc` · `BP_Torch` ✅
- All 11 `-COOKDIR` dirs contributed (694 files): Data 2 · UI 43 · Blueprints 26 · Input 29 · Characters 244 · Meshes 86 · Materials 51 · Textures 153 · VFX 11 · Audio 6 · LevelPrototyping 43.

✅ **AND THE CONTENT WAS PROVEN TO RESOLVE AT RUNTIME — on this package's own cooked bytes.** The stale Development exe sitting in the stage still *has* logging, so I pointed it at **the very pak the Shipping artifact ships**:
```
LogLoad: LoadMap: /Game/Maps/L_Arena?Name=Player
LogLoad: Took 0.998029 seconds to LoadMap(/Game/Maps/L_Arena)
UDeckComponent on 'SiegePlayerController_…': built a 50-card draw pile from 15 card rows (GDD §3.4)
Bot … 'Bot Defensive Economy' (50 cards, avg cost 21.06)
MinesPass seed=1718209793 … minesSpawned=6
Traversability CONFIRMED (nav settled: 0 pending) — Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s))
```
`not found`/`unavailable` sweep = **3, all benign** (2× engine `Deviceprofile LinuxArm64… not found`; 1× the `PKG-§4` GGUF degrade — *"THE MATCH IS FULLY PLAYABLE"*). **Zero content-asset misses.** Log: `TASK-699/devexe-on-shipping-pak-arena.log`.

⚖️ **State the split precisely.** This proves **the CONTENT half** of §6a — a real deck from real card rows, mines, nav, arena — **on the shipped bytes**. It does **NOT** prove the **Shipping BINARY's** runtime, because the instrument was the Development binary. **The gap is: nobody has seen this Shipping exe render an arena.**

⛔ **Declared honestly: no input-injection lane exists, and I did not invent one.** Reaching "Play (vs Bot)" needs a click. ⛔ No gameplay-feel claim. Human acceptance remains Jonathan's extract-and-click.

### ⭐ NEW MEASURED `PKG-§9`-CLASS DELTA (proposed §9f)
**A Shipping build ignores the command-line map override and `-ExecCmds`; the Development build honors them.** Same machine, same content, minutes apart — Development logged `LoadMap: /Game/Maps/L_Arena`, Shipping did not. ⚠️ **Consequence: `PKG-§6a`'s launch-arg route DOES NOT EXIST in Shipping**, so every future Shipping cook hits this same wall.
⛔ **Mechanism NOT asserted** — I checked `UE_COMMAND_LINE_USES_ALLOW_LIST` (defaults **0**, so that is *not* it) and did not finish reading the startup-URL path. Per the relayed-diagnosis law this is **a measurement with an unread cause**, not an engine claim. **Needs a task.**

---

## 3. THE FIVE SHIPPING MEASUREMENTS

**(a) `PKG-§9a` route — ROUTE 2**, declared before the cook, and **the silence was measured, not assumed**:
A/B on the same stage — the **Shipping** runs wrote **zero** log files (`Saved/Logs` created at my boot times, **empty**; `%LOCALAPPDATA%` empty too); the **Development** exe on the same content wrote a **123 KB** log. **Logging is compiled out of Shipping — proven by controlled comparison.**
⛔ I did **not** read that silence as a clean run. It is why §6a is reported INCOMPLETE rather than passed.

**(b) Row S5 — the in-game AI assistant console in Shipping. MEASURED, ⛔ no surgery.**
- `SiegeAssistantConsoleWidget.cpp/.h`: **zero** `UE_BUILD_SHIPPING` guards ⇒ **the console class IS compiled into Shipping.**
- Its open key `IA_AssistantConsole.uasset` **IS in the shipped container** (318 B).
- The only `!UE_BUILD_SHIPPING` guard nearby is on `DebugCaptureAndComposePrompt`, a **dev-only diagnostic** — not the console.
- ⛔ **Not proven functional**: pressing the key needs input injection. And `-ExecCmds` was ignored (above), so console/exec surfaces are demonstrably restricted somehow.
- ✅ **Player impact is nil either way**: with no GGUF the assistant is unavailable regardless of config — measured live in this package (*"THE MATCH IS FULLY PLAYABLE"*). **No worse than the Development zip.**
⇒ **Finding, not a defect. ⛔ No `#if` was touched.**

**(c) `PKG-§9c` — the 143 suite was NOT run here, deliberately.** Automation tests do not exist in a Shipping build; the suite gates the CODE on the **Development editor target**. A Shipping cook can never substitute. (Also: `-skipbuildeditor` means this run did not even rebuild that target.)

**(d) Row S2 — the `.exe`'s Windows file properties, VERBATIM.**
⚠️ **The stage contains TWO game exes.** The Shipping one is `GitClaudeUnrealTest-Win64-Shipping.exe`; `GitClaudeUnrealTest.exe` in the same folder is a **stale Development orphan** (§4 below). Measuring the wrong one reports the old template string — worth stating, because it is an easy mistake.

| Field | Development baseline (Aug 29) | **SHIPPING (measured)** |
|---|---|---|
| ProductName | `Third Person Game Template` | ✅ **`Siegebound`** |
| FileDescription | `Third Person Game Template` | ✅ **`Siegebound`** |
| CompanyName | `Epic Games, Inc.` | ✅ **`Jonathan Wesely`** |
| LegalCopyright | `Fill out your copyright notice in the Description page of Project Settings.` | ✅ **`Copyright 2026 Jonathan Wesely. All Rights Reserved.`** |
| OriginalFilename | `GitClaudeUnrealTest.exe` | `GitClaudeUnrealTest-Win64-Shipping.exe` — expected **S1 residue**, ⛔ not a defect |
| InternalName | `GitClaudeUnrealTest` | `GitClaudeUnrealTest` — expected **S1 residue** |
| ProductVersion / FileVersion | `5.8.0` | `++UE5+Release-5.8-CL-55116800` |

✅ **698's prediction confirmed and Jonathan's mid-task identity edit LANDED IN THE ARTIFACT.** His `CompanyName`/`CopyrightNotice` arrived while pass 1 was dying; I verified at source and measured that **no game binary had been linked yet** (staged exe still Aug-29), so the restart picked them up. **No re-cook is owed for the identity fields.**
**Window title at boot: `Siegebound`** (Development shows `Siegebound (64-bit Development PCD3D_SM6)` — same base string, config suffix appended; in Shipping the suffix is empty).

🙋 **FOR-JONATHAN:** the file a player double-clicks — the root `GitClaudeUnrealTest.exe` — is the engine's **BootstrapPackagedGame** shim, and *its* properties read `ProductName=BootstrapPackagedGame`, `CompanyName=Epic Games, Inc.`. It is an **engine binary**, not the project's, so `ProjectName`/`CompanyName` do not reach it. Your name is on the real game exe. ⛔ Not fixed here (engine-file territory).

**(e) `PKG-§9d` sizes.**
| Item | Development baseline | Shipping (measured) | Δ |
|---|---|---|---|
| game `.exe` | 347,694,080 B (331.6 MB) | **177,716,736 B (169.5 MB)** | **−48.9%** ✅ |
| `.pdb` | 401,526,784 B (382.9 MB) | **249,204,736 B (237.7 MB)** — present, renamed `…-Win64-Shipping.pdb` | −37.9% |
| `.ucas` | 1,077,741,296 B | **1,077,741,392 B** | **+96 B** |
| `.utoc` | 664,195 B | 664,292 B | +97 B |
| `.pak` | 11,469,788 B | 11,464,556 B | −5,232 B |
| staged tree | 2,053,734,759 B (1.91 GiB) | 2,480,649,570 B (2.31 GiB) | **+0.40 GiB — see §4** |
| staged tree **minus orphans** | — | **1,731,428,706 B (1.61 GiB)** | **−15.7%** ✅ |

⭐ **The ucas landing within 96 bytes of the corrected Development pass-2 cook is the strongest single proof `-COOKDIR` took effect** — the same content set, re-cooked for a different config.
⚠️ **The staged tree grew, which §9d calls "the cheapest possible smoke alarm."** It rang, and the cause is fully explained: 714.5 MB of stale orphans, not over-cooking. Net of them the artifact shrank as Shipping should.

---

## 4. ⛔ BLOCKING FINDING FOR TASK-700 — 714.5 MB OF STALE **DEVELOPMENT** BINARIES ARE SITTING IN THE STAGE

UAT overwrites `Windows/` **in place and does not clean it**. Left behind from the Aug-29 Development cook:
| Orphan | Size | In any manifest? |
|---|---|---|
| `GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest.exe` | 347,694,080 B | **NO — 0 hits** |
| `GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest.pdb` | 401,526,784 B | **NO — 0 hits** |

Verified against all three manifests (`UFSFiles`, `NonUFSFiles`, `DebugFiles`) — the manifests list **only** `…-Win64-Shipping.exe`, its `.pdb`, the redists and the shim. **These two files are pure orphans.**

⛔ **TASK-700 MUST prune them before zipping.** Two reasons, and the second is the serious one:
1. **714.5 MB** of dead weight in a zip Jonathan hands to people.
2. ⚠️ **A curious player who opens `Binaries\Win64\` and runs `GitClaudeUnrealTest.exe` gets the OLD DEVELOPMENT BUILD** — different config, different behaviour, stale code — while believing they ran Siegebound. A wrong-build-shipped-silently hazard of exactly the `PKG-§5a` family.

📌 Also cleaned by me: my boot-verify created `Windows/GitClaudeUnrealTest/Saved/` (test logs). **Purged** after copying the log to the handoff (TASK-696's precedent). Stage file count now **72**.

---

## 5. `PKG-§4` MODEL POSTURE — ✅ re-checked in the Shipping stage
`Models/` directories: **0** · `.gguf` files: **0** · template `Variant_*` / `ThirdPerson*` / `__External*`: **0**.
Graceful degrade measured live in this package's own run (quoted in §2).

---

## 6. ⚠️ WHAT CODE THIS ARTIFACT ACTUALLY CONTAINS — measured, because the tree moved under me

The Shipping exe linked at **00:04**. Other lanes landed `Source/` files **after** that:
- `SiegeControlsHelpWidget.cpp` written **00:23** (+19 min) · `SiegePlayerController.cpp` **00:26** (+22 min)
- `strings` on the shipped exe for `SiegeControlsHelpWidget` → **0 hits**

⇒ **The artifact = HEAD `22728c8` + TASK-698's ini changes + Jonathan's Company/Copyright keys. NOTHING ELSE.**
⛔ **It does NOT contain the TAB controls menu (TASK-704/705) or any of TASK-701/712/713.** That is correct — those are unbuilt/ungated (TASK-709 is gate 2) — but **TASK-700's README must not imply the TAB menu ships in this zip**, and Jonathan asked for that feature in the same directive.

---

## 7. STATE AT HANDOFF
- Editor **PID 17044 UP and untouched** all run; port 8000 never taken from it; ⛔ no MCP/editor-shaped call was made at any point (route 2 needs none). All boot-verify game processes closed; **0 remaining**.
- `L_Arena` hash unchanged, **never saved**. Fence re-verified post-cook: **0** tracked files under `packagedZIPofGame/`.
- ⛔ Nothing committed, nothing staged, nothing pushed by this task. My only tree additions are `handoffs/TASK-699*`.
- Staging: `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Windows\`. Zip name RESERVED for 700: `Siegebound-Win64-Shipping-2026-08-30.zip`. The Development zip is **preserved** (`PKG-§7b`, row S4).

## 8. FINDINGS FOR THE MANAGER
1. ⛔ **`PKG-§7a`'s two factual claims are both stale/false** (git root, and "no gitignore line"). The **ruled property is sound and saved the run** — repair the prose, keep the property.
2. ⭐ **Proposed `PKG-§9f`**: Shipping ignores the command-line map override and `-ExecCmds`. **Kills §6a's launch-arg route in Shipping.** Mechanism unread ⇒ needs a task. **Without it, no future Shipping cook can self-verify the arena.**
3. ⭐ **Bake `-skipbuildeditor` + `-ModelContextProtocolPort=<free>` into `ship.ps1` (`SHIP-§`)** — together they let a Shipping cook run with the editor UP. **Manager ruling 12 is confirmed; the bounce request can be dropped from the procedure.**
4. ⛔ **UAT does not clean the stage** ⇒ stale wrong-config binaries accumulate and are individually runnable. Deserves a `PKG-§` clause + a `ship.ps1` prune step, not just a TASK-700 fix.
5. 🙋 **Row S2 follow-up**: the double-clicked shim is an engine binary and will always read `BootstrapPackagedGame` / `Epic Games, Inc.` in its properties.
6. 📌 The inert `bUseLoggingInShipping=True` key (698, documented) is **harmless and was not edited** — flagged for a law rider as instructed.
7. ⚠️ **`Tools/Packaging/` (TASK-703) is untracked in the tree.** Per my standing mandate `Tools/**/*.py` and pipeline tooling are CODE and need a PASS QA report before any commit — **not mine, and not committed here.**
