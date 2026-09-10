---
description: Cook, verify, zip and document a Siegebound release. Every gate is a STOP.
argument-hint: "[Shipping|Development] (default Shipping)"
---

# `/ship` — one word, one repeatable, **refusable** release procedure

**Jonathan's standing instruction, verbatim:** *"anytime we make any changes, I can say 'ship' to you and you will update the zip file and any other documentation with all the current changes to the game."*

**Law:** `.claude/pipeline/CONVENTIONS.md` → **`SHIP-§0..§10`** and **`PKG-§1..§12`** (plus **`SC-§118`** — a `-game` session is Jonathan's, never the pipeline's). Read `SHIP-§` before running this if you have not this session. **The script `Tools/Packaging/ship.ps1` is the authority on the RECIPE; this file is the authority on the PROCEDURE AND ITS REFUSALS.** Neither may contradict `PKG-§`; where they seem to, **`PKG-§` wins and the divergence is a defect** you report rather than paper over.

⚠️ **If you read only one new thing before your first `/ship`, read §2b below.** A Shipping run does **not** complete in one invocation: it ends at **`ADJUDICATE C3`** and waits for you to *look at a screenshot* and record what you saw. That is `SHIP-§8`, and it is the step no script can do for you.

---

## ⛔⛔ THE POSTURE — READ THIS FIRST, IT IS THE WHOLE VALUE OF THE COMMAND

> ### **A FAILED GATE *STOPS* THE SHIP AND *SAYS SO*. THE COMMAND NEVER SHIPS A BUILD IT COULD NOT PROVE.**

A one-word release command that can emit a broken zip is **worse than no command** — it converts *"I'll go check the build"* into *"the build is fine, he shipped it"*, at exactly the moment nobody is watching.

- ⛔ **NO PARTIAL SHIPS.** Zip written but README not updated ⇒ the ship is **INCOMPLETE** and reported as such. ⛔ Never announced as done.
- ⛔ **NO GATE MAY BE SKIPPED TO SAVE TIME.** The script offers no flag to do it, and you may not work around one by hand. If a gate is wrong, **fix the gate** — under a task, with QA.
- ⛔ On a stop: report **which gate failed, the evidence, and what is required to clear it**, and confirm **the previous zip is untouched**.
- ⚠️ **`BUILD SUCCESSFUL` is not evidence of anything but the build.** The last cook printed it, booted to a perfect main menu, and shipped a game with an empty deck, no HUD and no hero (`PKG-§5a`).

---

## THE TRUTH SURFACE — how you read the script's result

**Build.bat returns exit 0 on a FAILED build; UAT lies the same way. That law extends to `ship.ps1` itself.**

⇒ **Read the LAST LINE of the script's stdout**, never just the exit code:

```
SHIP RESULT: PASS | STOP at <GATE-ID> - <reason> | ADJUDICATE C3 - <capture path> | DRYRUN-OK | DRYRUN-WOULD-STOP
```

### ⛔ THERE ARE **THREE** TERMINAL VERDICTS, NOT TWO (`SHIP-§1` as amended)

| Verdict | Exit | What it means | Is it a ship? |
|---|---|---|---|
| `PASS` | 0 | Every gate proved itself. The zip exists and was read back | ✅ **Yes** |
| `STOP at <gate> - <reason>` | 2 | A gate refused. **The previous zip is untouched** | ⛔ **No** |
| ⭐ `ADJUDICATE C3 - <capture>` | **4** | The build cooked and captured, and is now **SUSPENDED** awaiting the one judgment a script physically cannot make | ⛔ **No** |

> ### ⛔⛔ **`ADJUDICATE` IS NOT A PASS AND IS NOT A SHIP.**
> **An unresolved suspension is exactly as unshipped as a `STOP`.** ⛔ Never report an `ADJUDICATE` run as "the ship succeeded, just needs a quick look". It succeeded at *cooking*. It has not shipped, and it will not until you discharge §2b.
> ⚠️ **Exit 4 is not success.** Treat it exactly as seriously as exit 2.

✅ **What the phase order buys for free:** `C3` runs **before** PHASE D, so at an adjudication stop **there is no zip yet** — the previous artifact sits untouched and there is **nothing to clean up**.

The summary block above that line lists **every gate with PASS / STOP / CHECK / PLAN / ADJUD**, every measured fact (HEAD, sizes, window title, zip entries, the pixel verdict and its observations), and what the run does **not** prove. **Quote from it. Do not paraphrase it into something rosier.**

---

## THE PROCEDURE

### 0 — Orient (before touching anything)

1. `git status --porcelain` + `git rev-parse HEAD` + `git log --oneline -5`. **A dirty tree does NOT stop the ship** — Jonathan ships mid-work — but **every dirty path is listed in your report**, and the commit at the end stages **explicit paths only**.
2. **STOP** on an unresolved merge/rebase/cherry-pick (the script gates this too — `A2-NO-MID-OPERATION`).
3. **QUIET-MODULE:** confirm no other compile or cook gate is live. The cook is a **serialized** gate (`PKG-§6`) — ⛔ never concurrent with a compile. A running **UBT/UAT/AutomationTool** is a stop.
   ⛔⛔ **AND NO `UnrealEditor.exe` OF ANY KIND MAY BE ALIVE (`SHIP-§10`).** ~~A running editor is fine and is not a stop~~ — **FALSE for the cook, measured 2026-09-09 (TASK-1193 invocation 2, the first UAT run in this project's history, refused in 9 s):** UAT's `-build` invokes UBT, whose first act is the Live Coding mutex check, keyed to the *engine binary path*. The GUI editor **and a `-game` instance** both hold it, and UBT refuses the whole cook at *"Creating makefile"* before compiling one file. The old "different binaries" argument was true and irrelevant — UBT never reaches linking.
   ⛔ **Identify instances by COMMAND LINE** — `Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'"` → `CommandLine`, `ProcessId`, `CreationDate`, `ParentProcessId` — **never by process name or window title: a `-game` session carries the editor's exact title** (`Siegebound (64-bit Development PCD3D_SM6)`). An **editor** instance may be closed under the standing grant (by PID, after classification; never-save law; hash `L_Arena` before and after). A **`-game`** instance is Jonathan's play session (`SC-§118`): **name it in 🚨 Blockers, ask, and wait — never kill, never `Ctrl+Alt+F11`.** ⚠️ Until the boarded `ship.ps1` row lands, `A3-QUIET-MODULE` does **not** perform this check — you do, by hand, before invoking the script.
4. Read `packagedZIPofGame/README.md` and note the **last shipped commit** recorded in it. **That is this ship's diff base.** If it is absent, say so and use the previous zip's date to bracket the git log.

### 1 — Dry run first if anything about the machine changed

```powershell
& "<repo>\GitClaudeUnrealTest\Tools\Packaging\ship.ps1" -DryRun
```

Runs **PHASE A for real** and prints the whole plan — every resolved path, the exact Build.bat line, the exact suite line, the **exact UAT line including every `-COOKDIR`**, the stage-hygiene plan (with **which authority** resolved each staged binary name), the **boot-verify drive plan** (the no-argument launch and the exact client-relative click point, printed from the same constants the live run clicks — ⛔ a plan that prints one number and executes another is the divergence `SHIP-§0` calls a defect), the zip name, the retention plan, the commit plan. It **compiles nothing, launches no editor, cooks nothing, writes no zip, deletes nothing, commits nothing.**

⚠️ **PHASE A running for real means `A8-DESKTOP` runs for real too**, so a dry run on a locked machine reports `DRYRUN-WOULD-STOP` naming it. **That is the gate telling you a Shipping ship is not schedulable right now** — which is worth knowing *before* you queue a 30-minute cook, not after.

> ### ⛔⛔ **A `-DryRun` PASS IS *NOT EVIDENCE* ABOUT PHASES C–F (`SHIP-§7`, amended).**
> A dry run **never reaches** the cook, the boot-verify, the zip or the commit. **`DRYRUN-OK` means PHASE A passed and the plan printed — nothing more.**
> ⚠️ **This is not hypothetical.** An **unconditional forever-stop** once sat at `C3` behind a perfectly green dry run; it would have surfaced only on the first real ship, *after* a 30-minute cook. ⇒ The dry run now **PRINTS the adjudication contract it cannot exercise** — the bar, the record schema and the resume line — so the seam is visible in the test's own output. ⚖️ **A gate a test cannot reach must at minimum ANNOUNCE itself in that test's output.** Read that block; do not skim past it because the run said OK.

### 2 — The real run (first invocation: proves and cooks)

```powershell
& "<repo>\GitClaudeUnrealTest\Tools\Packaging\ship.ps1"        # -Configuration Shipping is the default (PKG-§2a)
```

What it does, in order — **each one a STOP**:

| Phase | Gate | What it proves |
|---|---|---|
| **A** | `A1-*` | Project, engine and staging dir resolve (all **derived**, none assumed) |
| A | `A2-NO-MID-OPERATION` | No unresolved merge/rebase |
| A | `A3-QUIET-MODULE` | No compile/cook gate is live. ⚠️ **GAP, NAMED (`SHIP-§10` cl. 2–3):** today the gate looks only for UBT/UAT/UnrealPak/`UnrealEditor-Cmd` and records `Editor processes N` as a *fact it does not gate on* — so a live GUI editor or `-game` instance passes A3 and the cook then fails at `C2-UAT-LOG` on the Live Coding mutex. Until the boarded `ship.ps1` row lands, **you** enumerate `UnrealEditor.exe` by command line before invoking the script (§0.3), classify each instance (`-game` ⇒ his play session; otherwise editor), and STOP yourself if any is alive. ⛔ Never close a `-game` instance (`SC-§118`). When the row lands, A3 STOPs with the class, PID, start time, parent and command line named — and it reports, it never kills |
| A | ⭐ `A4-FENCE` | **Measured THIS RUN** (`PKG-§7a`): the staging dir is either **outside the work tree** or **matched by a live `.gitignore` rule**. ⛔ Neither ⇒ stop before anything is written. It is a **property, never a hardcoded path** |
| A | `A5-DISK` | ≥ 4 GB headroom beside the retained zips |
| A | ⭐ `A6-EVIDENCE-ROUTE` | **`PKG-§9a-1`/`§9a-2`, checked BEFORE the 30-minute cook.** The gate's question is *"does this run have **some** valid way to prove the boot?"* — ⛔ **never** *"does an ini contain a key."* It **measures the machine class**: `<engine>\Engine\Build\InstalledBuild.txt` present ⇒ an **installed (Launcher) engine** ⇒ **Shipping is LOG-SILENT permanently** (`bUseLoggingInShipping` is a UBT `TargetRules` property with **no ini binding**, and `[RequiresUniqueBuildEnvironment]` makes it unsettable on a `Shared` environment) ⇒ the **`Pixel` route is AUTO-SELECTED and ANNOUNCED in the report** — ⛔ **a silent switch is its own trap.** **It STOPS only when there is NO usable route:** `-BootEvidence Log` demanded **explicitly** on an installed engine (you asked for an instrument that cannot exist), **or** the **pixel instrument is unavailable** (⛔ ***a ship that cannot verify its own boot must not ship***). ⛔ **It never passes with no route** (`SHIP-§1` intact), and ⛔ **`PKG-§6a`'s real-arena bar is UNTOUCHED — only the instrument changed.** ⚠️ Claiming a boot PASS from a log that *cannot emit* stays **banned**: in Shipping here a silent log is the **expected** state, so silence is worth nothing as evidence |
| A | ⭐ `A7-COOKDIRS` / `A7-MAPS` / `A7-BOOTMAP` | The `PKG-§5a` recipe still matches the tree: every `-COOKDIR` directory exists, both maps exist, `GameDefaultMap` still points at the boot map |
| A | ⭐⭐ `A8-DESKTOP` | **CAN THIS DESKTOP BE DRIVEN? (`PKG-§9f`'s stated cost, checked BEFORE the 30-minute cook.)** Under route `Pixel` the arena is reached by **clicking `Play`** in the shipped menu, and simulated input dies on a locked session (standing `TASK-076`). TASK-716 hit exactly that and the whole boot-verify was **BLOCKED**. ⛔⛔ **And it is deliberately NOT built on the two probes anyone would reach for, because TASK-716 measured BOTH of them lying on a locked machine:** `OpenInputDesktop()` returned **`"Default"`** and `SetCursorPos` **succeeded** — and screen capture succeeded too, capturing the **lock screen**, which is why `A6`'s instrument probe cannot answer this question either. (Windows 11 renders the lock screen on the *Default* desktop via `LockApp` + explorer's `LockScreenBackstopFrame`; only the *credential* stage switches to `Winlogon`.) ⚠️ **A gate on either API passes on a locked machine and then boot-verifies into a backstop — the `SHIP-§8c` fake-gate family exactly.** The honest test is the **click rig's own abort predicate**: the class and owner of `GetAncestor(WindowFromPoint(pt), GA_ROOT)`. ⭐ **It is a POSITIVE LOCK DETECTOR, not a proof-of-unlock:** it stops only when it can *name* a lock-screen owner; *"could not measure"* is reported as a fact and passes — because demanding proof of an unlocked desktop is how this lane would acquire **forever-stop number five**. Under route `Log` it asserts and says no desktop is needed (⛔ it never silently skips) |
| **B** | `B1-COMPILE` | Development editor target compiles — **verdict parsed from the log** (`Result: Succeeded`), ⛔ never the exit code. Detects Smart App Control (`0x800711C7`) and names it as a **machine state, not a code error** |
| B | `B2-SUITE` | The automation suite against the **Development editor target** (`PKG-§9c`), baseline **143**, **0 failures**. ⚖️ *Prove, then cook* — discovering after a 30-minute cook that the code never compiled is the most expensive ordering available |
| **C** | `C1-COOK` / `C2-UAT-LOG` | UAT `BuildCookRun`, Win64, Shipping, the maps allowlist **+ the `-COOKDIR` list** — verdict from **UAT's own lines**, ⛔ never `%ERRORLEVEL%` |
| C | `C2-STAGE-PRESENT` | The cook produced a staged exe. ⭐ **Both staged binary names are RESOLVED FROM THE MANIFESTS, never guessed** — the game binary's name is configuration-dependent (`<Project>.exe` for Development, `<Project>-Win64-Shipping.exe` for Shipping), and the **root shim** (the click target) is derived the same way even though it is *not* configuration-dependent today. ⚠️ **Guessing `<Project>.exe` under Shipping resolves to the 347 MB stale Development ORPHAN that stage hygiene deletes.** The report names **which authority answered** for each — manifest or convention fallback — so an ambiguous manifest is visible rather than papered over |
| C | ⭐ `C2-STAGE-MANIFESTS` / `C2-STAGE-PRUNE` / `C2-STAGE-ONE-EXE` | **STAGE HYGIENE (`PKG-§10`) — run BEFORE the boot-verify and again before the zip.** UAT does **not** clean the stage: TASK-699 measured **714.5 MB of stale *Development* binaries** surviving a Shipping cook, in **zero** manifests, **individually runnable**. ⛔ **The hazard is not the dead weight — it is that the WRONG EXE RUNS:** a player who opens `Binaries\Win64\` and double-clicks `GitClaudeUnrealTest.exe` silently launches the **old Development build** while believing they ran Siegebound. The gate deletes every **game `.exe`/`.pdb` in no manifest** and then asserts the invariant: **exactly ONE runnable game exe, plus the root shim.** ⚖️ *Prune → measure → boot-verify → zip*: a boot-verify against a tree that is then modified proves something about a tree that no longer exists |
| C | ⭐⭐ `C3-BOOT-ARENA` | **BOOT-VERIFY TO A REAL ARENA (`PKG-§6a`) — A MENU IS NEVER A PASS.** Requires `L_Arena` loaded · a **real deck built from real card rows** (⛔ not 6 blank slots) · the **hero spawned** · **HUD present** · `not found`/`unavailable` sweep = **0** (the one benign exception is the SiegeLlama no-model line, `PKG-§4`). ⭐ **The LAUNCH is route-dependent (`PKG-§9f`).** Route `Log` launches **with** the map argument — 699 measured the Development binary honouring it — and proves everything from the packaged build's own log. **Route `Pixel` — the STANDING route here — launches with NO ARGUMENTS (a player's double-click) and CLICKS `Play` in the game's own menu**, then captures and ends in `ADJUDICATE C3`: see §2b. ⛔ It never self-certifies |
| C | `C3-BOOT-TITLE` | (route `Pixel`) the window title reads *Siegebound* (`PKG-§8`). ⭐ **The running game is resolved BY PATH, never by name.** `Get-Process -Name` applies **no wildcard**, and under Shipping the game process is `<Project>-Win64-Shipping` while `<Project>` matches only the **title-less root shim** — TASK-715 measured both (pid 28688 `Title=''` vs pid 9104 `Title='Siegebound'`). ⚠️ Under Development the two share a name and it worked, which is why **the only route anybody exercised was the one that was not broken.** Every process already running from the stage is snapshotted **before** the launch and **excluded**, so a leftover game can never supply this run's title — and the run kills only what it started |
| C | ⭐ `C3-CAPTURE` | (route `Pixel`) **the `SHIP-§8c` mechanical pre-filter, and it may only ever FAIL.** Capture exists · non-zero · decodes · **not uniformly blank** · process was alive · title was read · ⭐ **the menu click was actually DELIVERED**. It records **`CHECK`, never `PASS`** — ⛔ **it is not a pass of `C3-BOOT-ARENA` and can never become one.** ⚠️ There is deliberately **no "does it look like an arena" heuristic**: in PowerShell that would pass a black screen with a loading spinner **while wearing a gate's clothes**, and a fake gate is worse than no gate. ⭐ *"The click was not delivered"* belongs here for the same reason *"the capture never happened"* does — it is a mechanical fact about whether **the drive occurred**, ⛔ never a judgement about what is *in* the frame, and it keeps a guaranteed-`FAIL` menu from being handed to you after a 30-minute cook |
| C | `C4-NO-MODELS` | No `Models/`, no `.gguf` in the stage (`PKG-§4`) |

Then, **under route `Pixel`, it ends at `ADJUDICATE C3` — go to §2b.** Under route `Log` it runs on and **STOPS at `D0-README`**, handing you the evidence. **Both are expected and correct.**

⚠️ **A Shipping first invocation takes over the screen for a couple of minutes** — the game is launched fullscreen, raised topmost and clicked. **That is the rig working, not a bug**, and it is why `PKG-§9f` rules a Shipping `/ship` *schedulable, not unattended*.

### 2b — ⭐⭐ THE PIXEL ADJUDICATION (`SHIP-§8`) — the step no script can do for you

**Why this exists:** a PowerShell script genuinely **cannot read a PNG**, and `ship.ps1` will never pretend it did. **You can.** So judgment moves to where sight exists — ⛔ **and the bar does not move with it.** `PKG-§6a` is untouched.

⚠️ **This is not a formality.** Since `PKG-§9a-1`, rendered pixels carry the **whole** boot-verify on this machine: Shipping is log-silent, so *"no errors in the log"* proves **nothing** here. Your eyes are the instrument.

#### 2b.1 — Reach the arena **the way a player does** (`PKG-§9f`) — ⭐ **the script now does this for you**

> ### ⛔ **A SHIPPING BINARY IGNORES THE MAP ARGUMENT *AND* `-ExecCmds`. `PKG-§6a`'s LAUNCH-ARG ROUTE INTO AN ARENA DOES NOT EXIST IN SHIPPING.**

**Measured by TASK-699 as a controlled A/B, ⛔ not inferred from one failure:** `/Game/Maps/L_Arena`, bare `L_Arena`, **and** `-ExecCmds="open /Game/Maps/L_Arena"` **all three booted to the menu** on the Shipping exe — while the **Development** exe, same machine, same content, minutes apart, logged `LoadMap: /Game/Maps/L_Arena`.

⇒ **The ruled route: launch the shipped exe with no args (as a double-click), then drive the game's OWN menu to reach a match. Capture. Then adjudicate.**

**⭐ `ship.ps1` IMPLEMENTS THAT ROUTE UNDER `Pixel`. You do not drive it by hand.** In order, and it prints every one of these as it goes:

1. **Launch `<stage>\Windows\<shim>.exe` with NO ARGUMENTS.** ⛔ No map argument, ⛔ no `-ExecCmds` — passing the refuted argument would guarantee a **menu** capture, and the only honest adjudication of a menu is **`FAIL`**, on every build, forever. *(That is not hypothetical: it was BLOCKER-2 of `qa/TASK-702.md`, found in this very script.)*
2. **Wait for the game's own window** (not for a log line — there is none, `PKG-§9a-1`).
3. **Take a PRE-CLICK menu capture.** ⛔ **Diagnostic only — it is NOT adjudicable and is NOT the boot evidence.** It exists so a mis-aimed click is *diagnosable* instead of mysterious, and it cannot be adjudicated by accident because only the arena capture is bound as the run's **pending capture** (`SHIP-§8b` rule 9 rejects any other).
4. **Click `Play (vs Bot)`** — the `t669_topclick` mechanism (topmost-without-activation, real cursor, the `GetAncestor(WindowFromPoint)` **abort predicate**, double press). ⛔ **No input is injected unless the game itself owns the pixel under the cursor.** The click point is a **client-relative fraction** derived from TASK-669's measured menu geometry (centred 7-entry VBox, pitch 49.7 px at 720p).
5. **Wait for menu → level travel → HUD to settle**, then take the adjudicated capture.

⭐ **THE CLICK POINT IS A MEASUREMENT, AND ITS ERROR DIRECTION IS SAFE — this is why a coordinate in a release script is not the liability it looks like.** If the point is wrong the game simply stays on its menu, the capture shows a menu, and **you**, judging the printed `PKG-§6a` bar which says in as many words that *a menu is not a pass*, record **`FAIL`**. ⇒ **A mis-aimed click can only ever produce a FAIL, never a false PASS.**

> ### ⚠️ **STATUS, STATED PLAINLY SO IT IS NOT MISTAKEN FOR MEASURED: this route is LAW-RULED (`PKG-§9f`) and RIG-SUPPORTED (`t669_topclick`, proven against this game's menu at TASK-669) — but it has ⛔ NOT YET been observed end-to-end on a Shipping binary.**
> **TASK-716 was dispatched to do exactly that and returned `BLOCKED — desktop locked`; it never launched the exe.** ⇒ The first `/ship` that reaches `C3` under `Pixel` is also the first live exercise of this route. **Read its printed drive report, and if the capture is a menu, check the PRE-CLICK frame before assuming the build is broken** — a menu after a *delivered* click is a build finding; a menu after a *mis-aimed* click is a recipe finding, and the two captures are how you tell them apart.

- ⭐ **This is strictly BETTER evidence than the launch-arg route it replaces:** it exercises the real **menu → level-travel → HUD** path a player actually takes, on the shipped binary, instead of a developer shortcut that bypasses it.
- ⚠️ **ITS COST IS PART OF THE RULING AND IS STATED, ⛔ NOT ENGINEERED AROUND: the click needs an UNLOCKED DESKTOP.** ⇒ **A Shipping `/ship` is SCHEDULABLE, ⛔ not unattended.** Say so when you plan one. ⭐ **`A8-DESKTOP` now refuses in PHASE A rather than discovering it at `C3` after the cook is spent** — and see that row for why it is ⛔ **not** built on `OpenInputDesktop`/`SetCursorPos`, both of which TASK-716 measured **lying** on a locked machine. ⛔ **Never let the boot-verify quietly degrade to *"the menu came up"*** — that is `PKG-§5a` all over again with our own gate as the false witness.
- ⛔⛔ **AND THE TEMPTING CONCLUSION IS THE FORBIDDEN ONE: this does NOT lower `PKG-§6a`.** *"The arena is hard to reach in Shipping"* is an argument about the **INSTRUMENT**, ⛔ **never about the BAR.** ⚠️ The last Development cook booted a **perfect menu** on a game with no deck, no HUD and no hero.
- 🧑 **Jonathan can discharge the whole thing faster than any rig (`SHIP-§8b(7)`, and his eye wins):** double-click the staged shim, click **Play**, and report the four criteria.
- ✅ **The CONTENT half is separately provable and unattended** (TASK-699's ratified method): a **UnrealPak listing** proves the `PKG-§5a` casualties are present and the `-COOKDIR` set landed, and the **Development exe pointed at this package's own cooked bytes** proves the content graph resolves. ⇒ **CONTENT is machine-provable; RENDERING is what the click adjudication buys.** ⛔ **Neither substitutes for the other.**

#### 2b.2 — **LOOK** at the capture, and judge it against **the printed bar**

The script prints the `PKG-§6a` criteria **verbatim** beside the capture path. ⛔ **Judge the written bar, not your own idea of "looks right" — an adjudication performed without the criteria in front of it is not one.** Read the image (the `Read` tool renders PNGs).

| Criterion | What must be **visible** |
|---|---|
| `arena` | Terrain, castle, a match in progress. ⛔ **A MENU IS NOT A PASS** |
| `deck` | A **real card deck built from `DT_Cards` rows** — cards with art and costs. ⛔ **NOT 6 blank slots** |
| `hud` | The in-match readouts render (gold / stance / the card-bar chrome) |
| `hero` | The hero pawn is spawned and visible in the world |

#### 2b.3 — Write the verdict record. ⛔ It is a **MEASURED ARTIFACT**, never a flag.

⚖️ **A flag says *"trust me, it passed"*. A record is an artifact on disk that the next invocation READS AND RE-VALIDATES.** ⛔ **There is no `-PixelAdjudicated` switch, there never will be, and adding one is an automatic QA FAIL.**

Write JSON to the path the script names (`<staging>\.ship\ship-adjudication.json`) — the script prints the exact schema **and the six binding values to copy**:

```json
{
  "schema": 1,
  "gate": "C3-BOOT-ARENA",
  "verdict": "PASS",
  "adjudicatedBy": "build-master (Claude agent)",
  "adjudicatedUtc": "2026-08-30T21:14:02.0000000Z",
  "capturePath": "...\\.ship\\20260830-210900\\bootverify-arena.png",
  "captureSha256": "9f2c...",
  "head": "22728c8...",
  "config": "Shipping",
  "stageExeBytes": 177716736,
  "stageExeUtc": "2026-08-30T07:04:48.5100000Z",
  "observations": {
    "arena": "castle walls and lit terrain fill the frame, siege units mid-field - this is a match, not a menu",
    "deck": "six card slots along the bottom, each with distinct art and a cost pip in the corner",
    "hud": "gold counter top-left reads 120; stance readout and the card bar chrome both render",
    "hero": "the hero character stands on the terrain in third person, left of centre"
  }
}
```

**The rules — every one of them is mechanically enforced on the next invocation, so a record that breaks one is REJECTED, not warned:**

1. ⛔ **`PASS` or `FAIL` ONLY.** No "probably", no "looks fine", no third state.
2. ⛔⛔ **AMBIGUITY IS A FAIL, AND THE ASYMMETRY IS THE POINT.** Unreadable, black, truncated or uncertain ⇒ **FAIL**. ⚠️ *"I think it's probably fine"* is a **FAIL**, not a pass. The script rejects hedging words in a `PASS` record's observations for exactly this reason.
3. ⛔ **NAME WHAT YOU SAW, PER CRITERION, IN THE AFFIRMATIVE.** ⛔ A record that says `"PASS"`, `"criteria met"`, or leaves an observation empty is **MALFORMED and rejected** (minimums: 24 characters and 4 words each). ⭐ **The script cannot judge whether your observations are TRUE — but it can absolutely require that they were MADE, and that alone defeats the reflexive rubber-stamp.** ⚖️ Same doctrine as `KBD-§2a` condition 3: **prove survivors by naming the objects, never by a summary.**
4. ⛔ **It binds to THIS capture and THIS build** — capture path + sha256, plus HEAD, configuration and the staged exe's size and timestamp. **All of it is re-measured on resume and any mismatch is a STOP.**
5. ✅ **The capture is KEPT** alongside the handoff record, and the verdict + its observations ride the ship report **and** the README's *what was verified* section. ⚖️ **A pixel verdict must be exactly as auditable as the log line it replaced** — otherwise we traded a checkable instrument for an unlogged opinion.
6. 🧑 **JONATHAN MAY DISCHARGE THIS HIMSELF AT ANY TIME, AND HIS EYE WINS.** His extract-and-click acceptance is unchanged and is still the final word (`PKG-§6`).

#### 2b.4 — Resume, or fail the ship

⚠️ **Write the README FIRST (step 3 below — that is PHASE E, and it is yours).** `D0-README` is the **first** gate of the resume and it STOPS unless the README already names this ship's zip. A resume that skips it does not fail late; it fails immediately.

Then re-invoke with the exact line the script printed:

| The record says | What the next invocation does |
|---|---|
| **`PASS`**, bindings re-measure identical | ✅ **Resumes at PHASE D (zip) → F (commit) → G (report) on the SAME staged build, ⛔ with NO second cook.** ⭐ **And the record is RETIRED** — see below |
| **`FAIL`** | ⛔ **STOP at `C3-VERDICT`** with your named reason. **The ship ends.** Nothing was written; the previous zip is untouched. ⭐ The record is **archived** and the state is left at `ADJUDICATE`, so the *next* run cooks and captures afresh — ⛔ **a FAILED boot is never resumed onto** |
| **Malformed** (bare `"PASS"`, empty or hedged observation, bad verdict value) | ⛔ **STOP at `C3-VERDICT-FORM`.** Re-read the capture and write a real record. ⛔ The record is **left exactly where it is** — it could not be read to a verdict, so it must keep stopping until a human looks at it |
| **Bindings differ** (different capture, different HEAD/config, exe changed) | ⛔ **STOP at `C3-VERDICT-BINDING`.** ⚠️ **Resuming onto a different build than the one adjudicated is the single worst failure this seam could produce**, so it is closed by **measurement**, not by care. ⛔ Also left in place. Delete it and re-run to cook, capture and adjudicate the build you actually have |

##### ⭐ THE RECORD IS RETIRED WHEN IT HAS BEEN READ TO A VERDICT (`SHIP-§8d`)

**The rule in one line: a record that produced a verdict is spent and is archived; a record that could *not* be read stays put and keeps stopping.**

- On a `PASS` that actually resumes: the record is **moved** into that run's log dir as `ship-adjudication.<stamp>.consumed.json` and the ship state advances **`boot: ADJUDICATE → PASS`**, carrying the adjudication (who, when, which capture) in `bootInstrument`.
- ⛔ **ARCHIVED, NEVER DELETED.** `SHIP-§8b(6)` keeps the verdict and its four observations auditable, and an operator's artifact is never silently destroyed.
- ⛔ **A consumed record cannot resurrect.** Copy the archive back to the stable path and the next run STOPS at `C3-VERDICT-BINDING` — the state now reads `boot=PASS`, so nothing is pending adjudication.
- ⛔ **A genuinely stale record still STOPS.** Retirement changes nothing about that; it only stops the *chore*.
- ⚖️ **Why this matters more than it looks:** without it, the record sat armed at a stable path forever, so **the first invocation of every subsequent `/ship` stopped on a HEAD mismatch until someone hand-deleted a file** — a chore standing exactly where Jonathan's *"anytime we make any changes, I can say ship"* is supposed to be. **`boot=PASS` is not trusted afterwards:** it is re-guarded on every future run by the same nine measurements a log-route PASS is guarded by.

⛔ **On a `FAIL`, report it as a real outcome.** A failed adjudication is the gate working. ⛔ Do **not** re-adjudicate the same capture hoping for a different answer — fix the build (the `-COOKDIR` recipe is the first suspect, `PKG-§5a`) and ship again.

---

### 3 — Write the README from the evidence you just got (`SHIP-§3`) — ⭐ **PHASE E, and it is BY HAND until `ship.ps1` gains a renderer**

⛔⛔ **MEASURED 2026-09-09 (`TASK-1192`, the README gate): `ship.ps1` has NO placeholder renderer, and this file never named the source.** So the step existed in law (`SHIP-§3a`) and nowhere in the procedure. **This is the procedure:**

1. **The README's durable body is the TRACKED source `Docs/Packaging/README-source.md`** (`SHIP-§3a`). ⛔ You do not author the README from scratch and you do not edit the rendered file's prose — you **render the source**.
2. **Copy the source to `packagedZIPofGame/README.md`** (git root, one level up — `SC-§102`) and **resolve every `{{SHIP:*}}` placeholder from THIS RUN's own measurements** — the nine: `ZIP_NAME` · `DATE` · `CONFIG` · `SIZE` · `HEAD` · `DIFF_BASE` · `VERIFIED` (the pixel verdict **and its four observations, who, when** — `SHIP-§8b(6)`) · `CHANGED_SINCE` (`git log <diff base>..HEAD`, in **player-facing language**, ⛔ not commit subjects) · `CLOUD_SYNC` (the standing text in `PKG-§12`, ⛔ re-measured if any void condition there holds). ⛔ A value you did not measure this run is not a value; write what you measured or STOP.
3. ⛔⛔ **Before the resume that zips: `Select-String -Path packagedZIPofGame\README.md -Pattern '\{\{SHIP:' | Measure-Object` must read `0`.** A surviving placeholder is a **STOP** (`SHIP-§3a`: *same family as `D0-README`*) — and in the interim, **you** are that gate. Paste the count into the handoff.
4. The source, its census and its gate report ride the ship's **own Phase F commit** (`SHIP-§3a`) — ⛔ never committed between invocation 1 and the resume (a `HEAD` change re-runs B+C in full, by design).

🙋 **Boarded as a programmer row (reported by the manager 2026-09-09, alongside the `A3` row): `ship.ps1` gains a Phase E renderer** — resolves the measurable placeholders itself, takes the prose ones (`CHANGED_SINCE`) from a host-written file, and STOPs at a new `E1-README-RENDER` gate on any surviving `{{SHIP:` or unknown placeholder. Until it lands, steps 1–3 above are yours.

`packagedZIPofGame/README.md` is **MANDATORY every ship** and is **rewritten**, not appended. Required content (all of it already lives in the source; this list is what you verify survived the render):

- zip **name · date · configuration · sizes** (exe, pdb *or its absence*, pak/ucas/utoc, archive — `PKG-§9d`)
- ⭐ **WHAT CHANGED since the previous zip** — from `git log <last-shipped-commit>..HEAD`, **in player-facing language**. ⛔ **Not raw commit subjects.** *"The war map now draws the battlefield the right way round"*, not *"TASK-694: flip WorldToMapUV"*
- ⭐ **WHAT WAS VERIFIED this ship** — the `PKG-§6a` arena evidence **as actually measured**, naming the instrument (log lines vs pixels). ⭐ **Under the pixel route this means the verdict AND its per-criterion observations, quoted** (`SHIP-§8b(6)`), plus **who adjudicated it and when**. ⛔ *"Boot verified"* is not a record of a pixel adjudication; **what you saw** is
- ⭐ **THE CLICK TARGET, NAMED EXPLICITLY** (`PKG-§10`) — the exact path a player double-clicks, stated once and unambiguously. ⚠️ With stage hygiene in force there is now **exactly one runnable game exe**, so this statement is finally unambiguous; before the prune, the README's explanation of *why the exe is not called `Siegebound.exe`* was actively misleading
- ⭐ **WHAT IS *NOT* IN THIS BUILD** (`PKG-§11`) — the commit the artifact was built from, and a plain-language list of work that exists in the repo but landed **after** it. ⛔ **The *what changed* section may never imply a feature is present when it is not** — ⚖️ the whole trust value of the README is that it describes **this zip**, not the project's ambitions
- the **`PKG-§1` correction** (a project folder is not a game — this is a cooked package, and it is stated plainly, ⛔ never silently substituted)
- **`PKG-§4`** (ships without the GGUF; the assistant degrades gracefully and *"THE MATCH IS FULLY PLAYABLE"* by the code's own words)
- **`PKG-§8`** the display-identity statement, and the honest residue: the exe stays `GitClaudeUnrealTest.exe` and the staged folder stays `Windows\GitClaudeUnrealTest\`. ⛔ If the template-name apology is no longer true, **delete it**
- the **`PKG-§9b`** assistant-in-Shipping finding, **stated honestly whichever way it measured**
- ⭐ **the last shipped commit, recorded IN the README**, so the *next* ship knows its own diff base
- ⚠️ what was **not** verified: no input-injection lane ⇒ no gameplay claim

⛔ **The README lives in an ignored/out-of-tree folder and CANNOT be committed.** ~~Reproduce its full text in the handoff/report~~ — **superseded by `SHIP-§3a` (2026-09-09): the body is the tracked source, so the handoff records the NINE RESOLVED PLACEHOLDER VALUES (a `placeholder → value` table) plus the `{{SHIP:` grep count, ⛔ not the full text.** The repo keeps the record as source + values; the rendered file is reproducible from the two (`PKG-§3`).

### 4 — Second invocation (packages, verifies, prunes, commits)

```powershell
& "...\ship.ps1" -CommitPaths "GitClaudeUnrealTest/Docs/setupdirections.md", "GitClaudeUnrealTest/.claude/pipeline/..." 
```

PHASE A re-runs (cheap). **PHASE B and C are reused only if a state file proves they ran for a byte-identical build input** — same HEAD, same build-relevant working tree, same configuration, same recipe, same staged exe. **Any difference re-runs the compile, the suite and the cook in full.** The summary says which happened, loudly. ⚠️ **If you changed `Source/`, `Content/`, `Config/` or `Plugins/` between the two invocations, expect — and accept — a full re-cook.** That is the gate working.

⭐ **Under the pixel route the boot is proven by the verdict record, and `SHIP-§8d`'s rule is RESUME RE-VERIFIES, IT DOES NOT TRUST:** the record's capture hash and the build identity are **re-measured every time** at `C3-VERDICT-BINDING`. ⛔ **A stale `PASS` cannot be replayed onto a new cook** — and a mismatch is a **STOP**, never a warning.

⭐ **Once that resume ships, the record is RETIRED (§2b.4)** — archived, and the state's `boot` advances to `PASS`. ⇒ **The NEXT `/ship` starts clean.** ⚠️ If a run stops at `C3-VERDICT-BINDING` complaining that the state records `boot='PASS'` rather than a pending adjudication, you are looking at a **consumed** record that came back to the stable path; that stop is the seam working, not a bug.

| Phase | Gate | What it proves |
|---|---|---|
| **D** | `D0-README` | The README **names this ship's zip**. ⛔ A stale README sealed into the archive is a partial ship |
| D | `D1-ZIP` | Written with a **ZIP64-capable** writer (.NET `ZipArchive`). ⛔ **`Compress-Archive` is BANNED** — PS 5.1 truncates near 2 GB and a silently-truncated archive is the worst possible failure mode. Boot-verify `Saved/` output is purged first. Written to a `.partial` name |
| D | ⭐ `D2-ZIP-READBACK` | **The zip is VERIFIED BY READING IT BACK** (`SHIP-§4`) — entry count · the click-target `.exe` · the real binary · pak/ucas/utoc · `README.md` **at the archive root** · **`Models/`/`.gguf` = 0**. *The tool's success report is about the tool, not about the artifact* |
| D | `D2-SIZE-SANITY` | Plausible size against the newest zip of the **same** configuration |
| D | `D3-PRUNE` | **Only after read-back passes** (`PKG-§7b`): keep **N=2 per configuration**. ⛔⛔ **A zip of a DIFFERENT configuration is NEVER pruned** — `Siegebound-Win64-Development-2026-08-29.zip`, the one Jonathan has actually been handed, is preserved indefinitely |
| **F** | `F1-COMMIT-PATHS` | Every path exists, is in the work tree, is **not ignored**, and is **not under the staging dir** |
| F | ⭐ `F2-INDEX-CLEAN` | The staging dir appears **neither staged nor untracked** before the commit (the TASK-683 editor auto-stage trap fired live — the fence is a property, not a hope) |
| F | `F2-COMMIT` | `git add -- <one explicit path>` per path, then `git commit -F`. ⛔ **`git add -A` / `git add .` are BANNED in this lane** |

---

## `SHIP-§3` — WHICH DOCUMENTATION A SHIP UPDATES. Ruled, ⛔ not left to judgement.

| Doc | In scope? | Rule |
|---|---|---|
| **`packagedZIPofGame/README.md`** | ⛔ **MANDATORY** | Rewritten every ship (step 3 above). Records the last shipped commit |
| **`CLAUDE.md`** | ✅ **once, at setup only** | The routing line for the bare word *"ship"*. ⛔ **Not rewritten per ship** |
| **`Docs/setupdirections.md`** | 🙋 **NARROW YES** | Updated **only** when this ship changed something that doc *asserts* (e.g. the packaging chapter, a config it documents). ⛔ **Never a blanket rewrite** — it is a hand-authored guide and a release procedure has no business restructuring it. If nothing it asserts changed, **touch nothing and say so** |
| **`Docs/GDD.md` change log** | ⛔ **NO (default)** | ⚖️ **A release procedure must not edit the DESIGN document.** The GDD is Jonathan's design intent; writing build results into it conflates *what we meant to build* with *what we packaged* |
| **`TASKBOARD.md` / `CONVENTIONS.md`** | ⛔ **NO** | Pipeline law is the manager's, ⛔ never a build script's |

---

## ⛔ GIT POSTURE — THE THREE PROHIBITIONS, ABSOLUTE

1. ⛔⛔ **NEVER PUSH.** No exception for `/ship`. **Distribution is Jonathan's alone.**
2. ⛔⛔ **NEVER STAGE THE BUILD OR THE ZIP.** `git add -A` / `git add .` are **banned in this lane** — explicit paths only, proven clean before committing.
3. ⛔ **NEVER AMEND OR REWRITE HISTORY.** Append-only. ⚠️ **Jonathan self-commits milestones** — read `HEAD`/`origin`, **report** what you find, ⛔ never "tidy" his commit.

**Commit cargo:** the doc updates + any config the ship itself changed (⛔ **never an evidence-route config — `PKG-§9a-1`: route 1 does not exist on this machine class and no ini key can create it**) + pipeline files. Message names the ship date, the configuration, the zip name and the verification verdict.

---

## `SHIP-§6` — THE REPORT BACK. Honest, and it names what it did not prove.

Every `/ship` ends with:

- the zip's **absolute path** + size, and the configuration
- **HEAD** and the **diff base**
- **what changed**, in player-facing language
- ⭐ **what was verified AND BY WHICH INSTRUMENT** — log evidence or pixels (`PKG-§9a` makes this material; say which)
- ⭐ **the pixel verdict, in full** (`SHIP-§8b(6)`): `PASS`/`FAIL`, **who adjudicated it**, when, the **capture's retained path**, and the **four per-criterion observations quoted**. ⚖️ **A pixel verdict must be exactly as auditable as the log line it replaced**
- ⭐ **HOW THE CAPTURE WAS REACHED** (`PKG-§9f`): that the exe was launched with **no arguments**, whether the menu click was **delivered or aborted**, and where the **pre-click diagnostic frame** was retained. ⚖️ *"I judged a screenshot"* is not a record of a boot-verify; **which path produced the screenshot** is — and it is the difference between a build finding and a rig finding
- ⭐ **whether an adjudication was RETIRED this run** (`SHIP-§8d`): the archived record's path, and that the ship state advanced `boot: ADJUDICATE → PASS`
- ⭐ **the stage-hygiene result** (`PKG-§10`): what was pruned (or that nothing was), and the **named click target** — the one runnable game exe. ⚠️ **A size delta that does not reconcile is INVESTIGATED, ⛔ never noted and passed** — that is how the 714.5 MB of orphans was found
- ⭐ **what was NOT verified** — ⛔ **always includes:** there is **no input-injection lane**, therefore **no gameplay-feel claim**; **human acceptance is Jonathan's extract-and-click**
- docs touched (and docs deliberately **not** touched, per the table)
- the commit hash · ⛔ **not pushed**
- any **`PKG-§9b`**-class finding (the assistant console in Shipping) — **stated plainly whichever way it measured**, ⛔ never "fixed" by lifting a shipped-code `#if` guard

---

## ⚠️ Known-hazard crib (do not rediscover these at 2 a.m.)

- ⭐ **The `-COOKDIR` list is not optional, in any configuration** (`PKG-§5a`, `§9e`). Soft references are this project's **house style** (`SiegePlayerController.cpp:194-215`) — the cooker cannot see the content graph, and a `-map`-only cook produces a beautiful, unplayable menu. It is baked into `ship.ps1`. ⛔ **Never hand-run a cook that omits it, even if UAT says `BUILD SUCCESSFUL`.**
- **Shipping is not "Development with a flag"** (`PKG-§9`): logging **is** compiled out and ⛔ **cannot be turned back on here** (`PKG-§9a-1` — it is a UBT `TargetRules` property, ⛔ **not an ini key**, and `[RequiresUniqueBuildEnvironment]` makes it unsettable on this installed engine; ⇒ **rendered pixels are the standing evidence route**, and `A6` selects them for you), the assistant console may be stripped, automation tests **do not exist** in a Shipping build, and the `.pdb` changes size or disappears.
- ⭐⭐ **A Shipping build cannot self-drive into an arena** (`PKG-§9f`) — the map argument **and** `-ExecCmds` are both ignored; only Development honours them. **`ship.ps1` reaches the arena by clicking `Play` in the shipped menu** (§2b.1). ⚠️ **That needs an unlocked desktop ⇒ a Shipping `/ship` is schedulable, not unattended** (`A8-DESKTOP` refuses in PHASE A). ⛔ It does **not** lower `PKG-§6a`. ⚠️ **And ⛔ never "fix" a menu capture by putting the map argument back** — that argument is *why* the capture was a menu.
- ⭐⭐ **THE TWO OBVIOUS DESKTOP PROBES LIE ON A LOCKED SESSION** (TASK-716, measured three ways). `OpenInputDesktop()` returns **`"Default"`**, `SetCursorPos` **succeeds**, and screen capture **succeeds** — capturing the lock screen. The only truthful mechanical test is the **class/owner of the root window under the click point** (`GetAncestor(WindowFromPoint(pt), GA_ROOT)` → explorer's **`LockScreenBackstopFrame`**), plus the pixels. ⛔ **Never build a desktop gate on the first two** — it passes on a locked machine and then boot-verifies into a backstop, which is the `SHIP-§8c` fake-gate family. ⚖️ **Note the shape, because it is this lane's recurring one:** an API that answers correctly in the normal case and lies in exactly the case that matters — the same shape as `Get-Process -Name` matching under Development and missing the Shipping game.
- ⭐⭐ **NEVER NAME A STAGED BINARY OR A GAME PROCESS FROM THE PROJECT NAME.** UE names both after the **configuration**: `<Project>.exe` under Development, `<Project>-Win64-Shipping.exe` under Shipping. Every gate written against the Development names *works in Development and stops forever in Shipping* — this lane has now moved a forever-stop **four** times (`A6` → `C3-BOOT-ARENA` → `C3-BOOT-TITLE` → the record-never-retired binding stop), and **three of those four share exactly this root cause.** ⇒ Staged binaries come from the **manifests**; running processes come from the **path**.
- ⭐⭐ **UAT does not clean the stage** (`PKG-§10`) — it leaves the previous configuration's **runnable** binaries behind, and a player can double-click the **wrong one**. `ship.ps1` now prunes them by **manifest membership** before every boot-verify and every zip. ⛔ **Two traps live here, both hit for real:** *(1)* "delete every non-manifest file" **deletes the game** — the `.ucas` (1.078 GB) and `.utoc` are in no manifest **by design**, and the `.pak`'s presence makes the mistake look safe on a spot-check; the rule is **game `.exe`/`.pdb` only**. *(2)* A **basename** grep exonerates the orphan, because the legitimate **root shim** shares its name — **match on the full relative path, never the basename.**
- **Smart App Control** enforced ⇒ every UE compile dies in ~2 s with `0x800711C7`. **Machine state, not a code error.** Report it; ⛔ do not loop QA.
- ⭐⭐ **Every `UnrealEditor.exe` must be down for the cook** (`SHIP-§10`) — the Live Coding mutex is global per engine binary, and a **`-game` instance holds it exactly as the GUI editor does.** ⛔⛔ **`UnrealEditor.exe` is ONE binary in three roles with ONE window title: classify by command line before touching anything (`SC-§118`).** A GUI **editor** instance falls under the standing close/reopen grant (kill by PID after classification, never by name; never-save law; `L_Arena` hashed before and after) — unless Jonathan says he is mid-edit, in which case closing it is **his** choice. A **`-game`** instance is Jonathan's play session and is **never ours to close**: name it in 🚨 Blockers and **wait for his word**. If a gate is blocked on either, **stop and say so.** ⚠️ **2026-09-09:** two of his play sessions were force-stopped by a kill-by-name that believed them to be the reopened editor — and the third, correctly left alone, still blocked the cook. Both halves of that sentence are the law. ⛔ The next `/ship` after that incident runs only on his word that nothing is running, and closes nothing (`SC-§118` cl. 6).
