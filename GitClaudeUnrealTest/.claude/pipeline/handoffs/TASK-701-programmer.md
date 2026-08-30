# TASK-701 — gameplay-programmer handoff — the `/ship` command infrastructure

2026-08-30 · law `SHIP-§0..§7` + `PKG-§1`..`§9e` · **file-only: no compile, no cook, no editor, no Git, no board edit beyond this task's status row** · inputs for `qa/TASK-702.md` and the TASK-703 dry run

---

## 0. WHAT LANDED

| # | File | Role (`SHIP-§0`) | Size |
|---|---|---|---|
| 1 | `Tools/Packaging/ship.ps1` | **the RECIPE** — `PKG-§5a`'s `-COOKDIR` lesson baked in so no future ship can silently regress to a menu-only package | 1150 lines, **created** `Tools/Packaging/` |
| 2 | `.claude/commands/ship.md` | **the PROCEDURE AND ITS REFUSALS** — makes `/ship` a real slash command; **created** `.claude/commands/` | ~190 lines |
| 3 | the `CLAUDE.md` routing line | **AUTHORED, ⛔ NOT APPLIED** — verbatim in §7 below for the orchestrator to place (ruling S8) | 1 line |

⛔ **`CLAUDE.md` is NOT in this diff** (QA criterion 12 — verify with `git diff --name-only`). ⛔ **Nothing was run**: no compile, no cook, no dry run (TASK-703 owns the dry run), no `git add`, no board decomposition.

**Two mechanical checks I DID run, both file-only and both non-executing:**
- `[System.Management.Automation.Language.Parser]::ParseFile` over `ship.ps1` → **PARSE: CLEAN, 0 errors.** (Parses the AST; executes nothing.)
- Byte audit → **0 non-ASCII bytes** (see deviation D3).

---

## 1. ⭐ THE BAKED-IN COOK RECIPE — the reason this file exists

`ship.ps1` lines ~148–210 carry the `-COOKDIR` block under a comment header that tells the whole story, because **a recipe without its reason gets "cleaned up" by the next reader.** The comment states, in the file:

- **the measured failure** (TASK-696 pass 1, 2026-08-29): UAT printed its own `BUILD SUCCESSFUL`, the exe launched, the main menu came up perfect, and the game was **UNPLAYABLE** — `DT_Cards` not found (deck EMPTY, 6 blank slots), `WBP_HUD` not found (no HUD), `BP_HeroCharacter`/`BP_CommanderNpc`/`BP_Torch` unavailable, mines and torches invisible. **Nothing errored.**
- **the architectural citation, verified at source this task:** `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp:194-215` — the constructor's *"content contract … everything soft, resolved null-safe at runtime"* block. Soft refs are the **house style**, so **the cooker cannot see the content graph by construction** and every future feature re-arms the trap for free.
- **the measured effect of the fix:** ucas 901.1 → 1027.8 MB, archive 1.789 → 1.91 GB, all five missing assets resolve, `not found`/`unavailable` = 0, errors 12 → 1.
- **what is deliberately NOT in the list** (the ~8 GB of marketplace packs) and **what must never be done to achieve it** (no `DirectoriesToAlwaysCook` ini surgery, no plugin surgery).
- **`PKG-§9e`:** not optional in Shipping either — and in Shipping, with logging possibly off, the empty artifact is **harder** to detect.
- the closing line: **"A hand-run cook that omits `-COOKDIR` is a defect even if UAT says BUILD SUCCESSFUL."**

**The list, complete (11 entries, each commented with what it carries):**
`Data` · `UI` · `Blueprints` · `Input` · `Characters` · `Meshes` · `Materials` · `Textures` · `VFX` · `Audio` · `LevelPrototyping`

**The exact UAT line the script composes** (`-DryRun` prints it verbatim; `$ContentDir`/`$ProjectPath`/`$StagingDir` are all **derived**, never hardcoded):

```
RunUAT.bat BuildCookRun -project="<uproject>" -nop4 -utf8output -platform=Win64
  -clientconfig=Shipping -build -cook
  -map=/Game/Maps/L_MainMenu+/Game/Maps/L_Arena
  -pak -stage -prereqs -archive -archivedirectory="<staging>"
  -AdditionalCookerOptions="-COOKDIR=<Content>\Data -COOKDIR=<Content>\UI -COOKDIR=<Content>\Blueprints
   -COOKDIR=<Content>\Input -COOKDIR=<Content>\Characters -COOKDIR=<Content>\Meshes
   -COOKDIR=<Content>\Materials -COOKDIR=<Content>\Textures -COOKDIR=<Content>\VFX
   -COOKDIR=<Content>\Audio -COOKDIR=<Content>\LevelPrototyping"
```

⚠️ **Two flags I added beyond TASK-696's measured line, declared:** `-nop4` (skips Perforce probing) and `-utf8output`. Both are standard UAT switches with no effect on what is cooked. Everything else is byte-for-byte the shape that was measured working, with `-clientconfig` moved to **Shipping** per `PKG-§2a`.

**The maps allowlist is re-verified against the tree every run** (gate `A7-*`), not trusted:
| Map | Source of truth (re-read this task) |
|---|---|
| `/Game/Maps/L_MainMenu` | `Config/DefaultEngine.ini:10` `GameDefaultMap` |
| `/Game/Maps/L_Arena` | `SiegeSessionSubsystem.cpp:20` `ArenaMapPath` |

---

## 2. THE PROCEDURE TABLE — every gate, its stop condition, its law

**One-for-one for QA.** Every row is a **STOP** (`SHIP-§1`). Gate IDs are the literal strings the script prints.

| Gate | Phase | Stops the ship when… | Law |
|---|---|---|---|
| `A1-UPROJECT` / `A1-REPO` / `A1-ENGINE` / `A1-STAGING` | A | project, work tree, engine or staging dir cannot be **derived** (staging: first *existing* probed candidate; ⛔ never created silently) | `SHIP-§2` A |
| `A2-NO-MID-OPERATION` | A | `MERGE_HEAD`/`CHERRY_PICK_HEAD`/`REVERT_HEAD`/`BISECT_LOG`/`rebase-*` present. ⚠️ **A dirty tree does NOT stop** — every dirty path is listed instead | `SHIP-§2` A1 |
| `A3-QUIET-MODULE` | A | UBT / AutomationTool / UnrealPak / UnrealEditor-Cmd / UHT / cl / link is live. ⚠️ A running **editor** is reported, **not** a stop (manager ruling 12: the Shipping monolithic target links different binaries) | `PKG-§6` |
| ⭐ `A4-FENCE` | A | the staging dir is **neither** outside this work tree **nor** matched by a live `.gitignore` rule — **measured this run** via `git rev-parse --show-toplevel` + `git check-ignore -v` against whatever `-StagingDir` resolved to. ⛔ **No path is hardcoded** | `PKG-§7a` |
| `A5-DISK` | A | < 4 GB free on the staging drive | `SHIP-§2` A4 |
| ⭐ `A6-EVIDENCE-ROUTE` | A | config is Shipping, evidence route is Log, and `bUseLoggingInShipping=True` is **absent** from `Config/DefaultEngine.ini`. **Checked BEFORE the 30-minute cook** | `PKG-§9a` |
| `A7-COOKDIRS` | A | any `-COOKDIR` directory no longer exists on disk (a renamed content dir would silently under-cook exactly the `PKG-§5a` way) | `PKG-§5a` |
| `A7-MAPS` / `A7-BOOTMAP` | A | a listed `.umap` is missing, or `GameDefaultMap` has drifted off the recipe's boot map | `PKG-§5` |
| `B1-COMPILE` | B | the **log** does not read `Result: Succeeded`, or reads `Result: Failed`, or times out. ⛔ exit code deliberately ignored. Detects `0x800711C7` and names it **Smart App Control = machine state, not a code error, do not loop QA** | exit-code-lie law |
| `B2-SUITE` | B | any `Result={Fail}`, or `tests performed` < **143**, or `Result={Success}` count < 143, or timeout. Development **editor** target | `PKG-§9c` |
| `C1-COOK` / `C2-UAT-LOG` | C | UAT times out, or its **own lines** do not carry `BUILD SUCCESSFUL`, or carry `BUILD FAILED`. ⛔ `%ERRORLEVEL%` ignored | `PKG-§6` |
| `C2-STAGE-PRESENT` | C | the cook claimed success but produced no staged exe | `SHIP-§4` family |
| ⭐⭐ `C3-BOOT-ARENA` | C | **route Log:** the boot log does not show `LoadMap` **and** `/Game/Maps/L_Arena` **on the same line**; or the deck line reports 0 cards / 0 rows; or the hero-start line is absent; or the `not found` / `unavailable` / `continuing without a HUD` sweep returns any non-benign hit. **route Pixel:** always stops for **caller adjudication** (see §5) | `PKG-§6a` |
| `C3-BOOT-TITLE` | C | (route Pixel only) the window title does not read *Siegebound* | `PKG-§8`, `§9a` r2 |
| `C4-NO-MODELS` | C | any `.gguf` or `Models/` in the stage. **Runs on a reused stage too** — what matters is what is about to be zipped | `PKG-§4` |
| `D0-README` | D | `packagedZIPofGame/README.md` does not **name this ship's zip filename** | `SHIP-§1`, `§3` |
| `D2-ZIP-READBACK` | D | reading the archive back does not find: the click-target `.exe`, the real binary, `README.md` **at the root**, ≥1 `.pak` + `.ucas` + `.utoc`, **and 0 `Models/`/`.gguf` entries** | `SHIP-§4` |
| `D2-SIZE-SANITY` | D | the new zip is < 60% of the newest zip **of the same configuration** | `PKG-§9d` |
| `F1-COMMIT-PATHS` | F | any `-CommitPaths` entry does not exist / is outside the work tree / **is git-ignored** / **is under the staging dir** | `SHIP-§5` |
| ⭐ `F2-INDEX-CLEAN` | F | after staging, the index or porcelain shows any `packagedZIPofGame` or `*.zip` row (the TASK-683 auto-stage trap) | `SHIP-§5` 2 |
| `F2-COMMIT` | F | `git commit -F` fails | `SHIP-§5` |

**Non-assertion action records** (printed in the gate table as `PASS` with evidence, not as assertions): `D1-ZIP` (the archive path) and `D3-PRUNE` (what was kept, what was pruned, and every **PROTECTED other-configuration zip that was not touched**).

### The truth surface (per the exit-code-lie law)
The script's **last stdout line** is authoritative:
`SHIP RESULT: PASS | STOP at <GATE-ID> - <reason> | DRYRUN-OK | DRYRUN-WOULD-STOP`
Exit codes are consistent with it (0 / 2 / 3, 1 = unexpected error) but the summary line is the surface the caller reads. Above it the script prints the full gate table, the fact table (HEAD, dirty count, sizes, window title, zip entries, resume decision) and a fixed **"NOT PROVEN BY THIS SCRIPT, EVER"** block naming the missing input-injection lane and the never-push law.

---

## 3. THE `-DryRun` CONTRACT (TASK-703's acceptance instrument, `SHIP-§7`)

**What it DOES:** executes **PHASE A for real** — every path resolution, the git state, QUIET-MODULE, **the fence measurement**, disk, the `PKG-§9a` route precondition, and the recipe-vs-tree checks. Then prints, verbatim:
- the exact **Build.bat** command line
- the exact **suite** command line
- ⭐ the exact **UAT** command line **with every `-COOKDIR` expanded on its own line**
- the boot-verify plan (launch line, required markers, the sweep patterns)
- the target zip path, the **retention plan** (marking every other-configuration zip as `PROTECTED … never pruned`), and the commit plan (`would stage: …` per path)

**What it does NOT do:** ⛔ no compile · ⛔ no editor launch · ⛔ no suite · ⛔ no cook · ⛔ no boot · ⛔ no zip · ⛔ no delete · ⛔ no `git add` · ⛔ no commit. It creates no run-log directory. **It writes nothing at all.**

**Behavioural rule, stated because it looks unusual:** in a **real** run the *first* failing gate stops the ship immediately. In `-DryRun` every gate is still **evaluated and recorded**, and the run continues so the operator sees the **whole** plan; the summary then reports `DRYRUN-WOULD-STOP` naming every gate that would have stopped it, and exits 3. **This cannot be a bypass — a dry run produces no artifact of any kind.**

### ⚠️ EXPECTED DRY-RUN OUTCOME TODAY, so TASK-703 does not mis-read it as a defect
`Config/DefaultEngine.ini` **has no `bUseLoggingInShipping` entry** (measured this task; matches `PKG-§9a`'s own measurement). ⇒ A default `-DryRun` (Shipping + route Log) will report:

> `[STOP] A6-EVIDENCE-ROUTE  Shipping + -BootEvidence Log requires bUseLoggingInShipping=True …`
> `SHIP RESULT: DRYRUN-WOULD-STOP`

**That is the gate working, not the script failing.** It clears the moment TASK-699/700 lands `PKG-§9a` route 1. To see a clean plan before then, TASK-703 can additionally run `-DryRun -Configuration Development` (route check not applicable) and/or `-DryRun -BootEvidence Pixel`, and compare the two `WOULD-STOP` lists. **Neither variant skips a gate.**

---

## 4. THE PARAMETERS — and why none of them can weaken a gate

The script's header carries this table too, so a future reader meets it before the code.

| Parameter | Can it weaken a gate? |
|---|---|
| `-DryRun` | **No** — produces no artifact at all |
| `-Configuration Shipping\|Development` | **No** — every gate still runs; the config is **in the zip name**, so the two can never overwrite each other, and prune is config-anchored |
| `-ProjectPath` / `-EngineRoot` / `-StagingDir` / `-ShipLogDir` | **No** — path resolution only. The `A4-FENCE` gate is measured against **whatever `-StagingDir` resolves to**, so overriding it cannot escape the fence |
| `-BootEvidence Log\|Pixel` | **No** — `Pixel` can only make the boot gate **harder** (it adds the window-title gate and then stops for adjudication). It never auto-passes |
| `-CommitPaths` | **No** — every entry is validated and rejected if missing / outside the tree / ignored / under the staging dir. There is **no directory sweep** |
| `-CommitTrailer` | **No** — literal commit-message lines only |

**⛔ THE FLAGS THAT DO NOT EXIST, listed by name in the file header so nobody adds one "just this once":** `-SkipTests`, `-SkipCook`, `-SkipSuite`, `-NoVerify`, `-Force`, `-IgnoreGates`, `-AcceptWarnings`, `-FastShip`, `-YesReally`.

**Git sweep of `ship.ps1`, run this task (QA criterion 9):**
- `git add -A` / `git add .` / `--all` / directory sweep → **0**
- `git push` → **0** (the 3 `push` hits are prohibition prose: two report lines and one gate-evidence string)
- `--amend` / `reset --hard` / `filter-branch` → **0** (the 5 `amend|rebase` hits are the `A2` mid-operation marker list and the SHIP-§5 prohibition comment)
- **Every git invocation is read-only except two**, both in PHASE F: `git add -- <one explicit path>` (per path, in a loop) and `git commit -F <file>`.
- `$LASTEXITCODE` appears **once**, in `Invoke-Git`, and is annotated in place: it reads exit codes **only for git queries** (`rev-parse`, `status`, `check-ignore`, `diff --cached`) where the exit code *is* the documented answer. ⛔ **No build / cook / suite / boot verdict anywhere in the file comes from an exit code.** `Invoke-Tool` captures the exit code and never uses it as a verdict.
- `-Force` appears only on `Remove-Item` / `Move-Item` / `New-Item` (filesystem semantics), **never as a gate parameter.**

---

## 5. DECLARED DEVIATIONS AND INTERPRETATIONS (`SC-§15`)

**D1 — `-DryRun` does NOT run the compile or the suite.** `SHIP-§7`'s prose says *"every pre-flight and every check EXCEPT the cook, the zip and the commit"*, which could be read as including PHASE B. **I took the narrower reading**, because the TASK-701 spec clause 4 says *"does pre-flight + prints the exact UAT line … does not compile, cook, zip or commit"* and manager ruling 13 calls the dry run **"gate-free by construction"** — a suite run spawns a second editor and **is** a gate. ⇒ DryRun executes PHASE A only. **If QA reads `SHIP-§7` the other way, this is a one-line change** (`if ($DryRun)` → run B, then plan C onward).

**D2 — RESUME-BY-MEASUREMENT, to resolve a real ordering defect in `SHIP-§2`.** The law orders **PHASE D (package) before PHASE E (document)**, but the README must be **inside** the zip and its *"WHAT WAS VERIFIED"* section can only be written **after** PHASE C's evidence exists. Taken literally, the law seals the **previous** ship's README into this ship's zip. Resolution, implemented:
- `D0-README` refuses to zip a README that does not name **this run's** zip ⇒ the first invocation runs A+B+C, proves the build, and stops handing the caller the evidence;
- the caller writes the README and invokes the script **again**; PHASE A re-runs (cheap), and **B+C are reused only when a state file proves they ran for a byte-identical build input** — same HEAD, same build-relevant working tree, same configuration, same recipe hash, same staged-exe timestamp, all verdicts PASS. **Any difference re-runs compile, suite and cook in full**, and the summary says which happened.
- ⛔ **This is not a phase/skip flag** — there is no parameter for it; it is a *measurement*. It cannot skip a gate, only decline to re-prove an input already proven identical.
- The reuse hash deliberately **ignores `.claude/**`, `Docs/**` and other non-build paths** (prefixes counted: `Source/`, `Content/`, `Config/`, `Plugins/`, plus `*.uproject`) — **a markdown file cannot change a cooked binary**, and without this filter writing the README would force a 30-minute re-cook. **QA should confirm it agrees with that filter**; it is the one judgement call in the reuse rule.

**D3 — `ship.ps1` is ASCII-only, and law citations in it are written `PKG-5a` / `SHIP-2`, not `PKG-§5a` / `SHIP-§2`.** Windows PowerShell 5.1 decodes a BOM-less UTF-8 script as ANSI; a corrupted character **inside a match pattern** is a gate that stops working without saying so. I judged a BOM too fragile for a load-bearing release script (any re-save without it breaks the file). The header explains the mapping. **`.claude/commands/ship.md` keeps full Unicode** — it is read by Claude, not by PowerShell. ⚠️ **QA: grep the script for `PKG-5a`, not `PKG-§5a`.** Verified: **0 non-ASCII bytes.**

**D4 — route 2 (`-BootEvidence Pixel`) makes `/ship` a caller-adjudicated procedure by construction.** `PKG-§9a` route 2's evidence is *process liveness + window title + a rendered-pixel capture*. The script **takes** the capture (`System.Drawing` screen grab into the run-log dir) and gates the window title, but **it cannot read pixels**, so it STOPS with `C3-BOOT-ARENA … PIXEL ADJUDICATION REQUIRED` naming the file. ⛔ **It never converts "I could not measure" into a pass** — that inversion is the specific thing `PKG-§9a` bans. **This means a Shipping ship under route 2 cannot complete without a human/agent looking at the capture.** That is honest, and it is why route 1 is the proceeding default. **Flagging it for QA as a design ruling to confirm.**

**D5 — the HUD is proven by absence-of-failure, and that is deliberate, not lazy.** There is no positive "HUD created" log line (`SiegePlayerController.cpp:360-376` logs only the two failure branches). The script therefore requires **positive** markers first — `LoadMap`+`L_Arena` on one line, the deck line with a **non-zero card and row count**, and the hero-start line — which is what **proves the log channel is alive**; only then is the absence of `continuing without a HUD` meaningful. This is written into the file as a comment, precisely because `PKG-§9a` bans reading absence in a log that cannot emit.

**D6 — the `unavailable` sweep carries exactly ONE benign exception, matched on a full phrase.** `PKG-§4`'s graceful-degrade line (`SiegeLlamaSubsystem.cpp:1994`, *"the in-match assistant is unavailable this session … THE MATCH IS FULLY PLAYABLE"*) is expected in every ship-without-GGUF and would otherwise fail the gate forever. The allowlist is a single full-phrase match so it **cannot widen into a blanket excuse**. Every other `not found` / `unavailable` / `continuing without a HUD` hit is a STOP, and up to 12 offending lines are printed.

**D7 — the `D2-SIZE-SANITY` alarm is honestly scoped.** It compares only against the newest zip of the **same** configuration, and the comment says plainly that **it does not catch the `PKG-§5a` defect** (the broken pass-1 package was only ~12% smaller than the fixed one) — **the arena boot-verify is what catches that.** It is a smoke alarm for a grossly mis-configured cook, nothing more.

---

## 6. ⚠️ FINDING FOR THE MANAGER — `PKG-§7a` IS FACTUALLY WRONG, MEASURED TODAY. THE PROPERTY SURVIVES; THE PARENTHETICAL DOES NOT.

`PKG-§7a` states, as measurements: *(a)* the staging folder is *"the parent of the git root … OUTSIDE the work tree entirely"* and *(b)* *"the root `.gitignore` CONTAINS NO `packagedZIPofGame` LINE (grepped 2026-08-30, zero hits)"*.

**Both are false as measured 2026-08-30 by me:**

```
$ git rev-parse --show-toplevel      ->  C:/GitProjects/GitHub/GitClaudeUnrealTesting
$ git check-ignore -v -- .../packagedZIPofGame/README.md
  .gitignore:19:packagedZIPofGame/    .../packagedZIPofGame/README.md
```

⇒ **The git root IS `…\GitClaudeUnrealTesting\` (the project folder is a subdirectory of the work tree), so `packagedZIPofGame/` is INSIDE the work tree — and it is fenced by a live `.gitignore` rule at line 19**, which TASK-696 added before the first cook and TASK-697 proved three ways. **The real situation is route (ii), not route (i).** TASK-696's own §1 manager rider already flagged the off-by-one-directory error; `PKG-§7a` appears to have recorded the correction backwards.

**Why this cost nothing here, and why it would have cost something to anyone else:** `PKG-§7a`'s *ruling* — the fence is a **PROPERTY, measured every run, satisfied by (i) outside-the-tree OR (ii) a live ignore rule** — is exactly right, and `A4-FENCE` implements the property, so the script measures its way to the correct answer regardless. **A hardcoded path taken from the law's parenthetical would have resolved to a folder that does not exist.** That is the law's own argument for itself, demonstrated.

⛔ **I did not edit `CONVENTIONS.md`** (pipeline law is the manager's). **Recommended repair:** strike the two factual claims in `PKG-§7a`, keep the ruling verbatim, and record that the live fence is `.gitignore:19` under route (ii).

---

## 7. ⭐ THE PROPOSED `CLAUDE.md` LINE — AUTHORED, ⛔ NOT APPLIED (ruling S8)

**Placement:** append as **item 8** at the end of the existing `## Routing rules` numbered list in `CLAUDE.md` (which currently ends at item 7, *"Report the outcome to the user with task IDs and commit hashes."*).

**The exact line, verbatim — copy as-is:**

```markdown
8. **The bare word "ship" = `/ship`.** Follow `.claude/commands/ship.md` end to end (recipe: `Tools/Packaging/ship.ps1`; law: `SHIP-§0..§7` + `PKG-§1..§9e`). The compile, cook and Git steps are `build-master`'s lane — dispatch it with the command file as its brief. ⛔ **Every gate is a STOP:** a failed gate ends the ship with a named reason and leaves the previous zip untouched — no partial ships, no skip flag, ⛔ never push.
```

**Two notes for whoever places it, because I will not decide the orchestration contract on a relay:**
1. **Why `build-master` and not the orchestrator directly:** `CLAUDE.md`'s own team table gives compiling and Git commits exclusively to `build-master`, and says the orchestrator *"never does specialist work"*. Routing "ship" to build-master keeps that contract intact. **If you would rather the orchestrator run `/ship` itself**, the line becomes a documented exception to the team table and should say so explicitly — that is a Jonathan-level call, not mine.
2. ⚠️ **`/ship` already works without this line.** It only covers Jonathan typing the bare word *"ship"* instead of `/ship`, which is exactly what his directive says he will do.

---

## 8. WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐ **The `-COOKDIR` block** (`ship.ps1` ~148–210) — list complete (11), commented with the *why*, and reachable from `$RECIPE_COOKDIRS` into `-AdditionalCookerOptions`. **Its absence or truncation is the blocker this whole task exists to prevent.**
2. **The `A4-FENCE` measurement** — confirm it is genuinely per-run and path-free, and that §6's finding above does not change your reading.
3. **D2 (resume-by-measurement)** — the one place a reviewer could reasonably see a skip. Read the guard list and the build-relevant path filter and rule on it.
4. **D1 (`-DryRun` scope)** and **D4 (pixel route stops rather than passes)** — both are readings of the law; both are cheap to reverse if you read them differently.
5. **`SHIP-§3`'s doc table in `ship.md`** — reproduced including the **NOs** (GDD change log OUT; `setupdirections.md` NARROW; `TASKBOARD`/`CONVENTIONS` NO). Confirm the wording did not soften.
6. **The `SHIP-§6` report shape** — confirm the "what it does NOT prove" block names the missing input-injection lane and Jonathan's extract-and-click acceptance.
7. **`CLAUDE.md` absent from the diff.**

## 9. OPEN QUESTIONS FOR QA / THE ORCHESTRATOR

- **Q1 (D2):** is the build-relevant path filter for the reuse hash (`Source/`, `Content/`, `Config/`, `Plugins/`, `*.uproject`) the right cut? Anything else that can change a cooked artifact?
- **Q2 (D4):** confirm that route 2 stopping for caller adjudication — rather than the script self-certifying pixels — is the intended reading of `PKG-§9a`.
- **Q3 (D1):** should `-DryRun` also run the compile and the suite (`SHIP-§7`'s "every check")? I read manager ruling 13 as no.
- **Q4:** `packagedZIPofGame/README.md` is **git-ignored** and therefore **cannot** be a commit path — `F1-COMMIT-PATHS` rejects it by design. Confirm the record-keeping route stays *"reproduce the full text in the handoff"* (`PKG-§3`).
- **Q5 (for the manager, §6):** the `PKG-§7a` factual repair.

## 10. STATE

⛔ Nothing executed. ⛔ No editor, no MCP, no cook, no compile, no `git add`, no commit, no push. Files touched: **`Tools/Packaging/ship.ps1`** (new) · **`.claude/commands/ship.md`** (new) · this handoff · TASKBOARD status row for TASK-701 only. Status → **ready-for-qa** (TASK-702).
