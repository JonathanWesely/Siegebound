# TASK-714 — gameplay-programmer handoff — the `C3-BOOT-ARENA` adjudication seam (+ `PKG-§10` stage hygiene, + `PKG-§9f` in the procedure)

2026-08-30 · law **`SHIP-§8a..§8d`** + amended **`SHIP-§1`**, **`SHIP-§2` step 9**, **`SHIP-§7`** · scope additions **`PKG-§10`** and **`PKG-§9f`** (authorizing rulings, cited ⛔ not restated)
**File-only: ⛔ no compile, no cook, ⛔ no execution of `ship.ps1` (parse-check only, as 701/712 did), no editor, no MCP, no Git, no board edit beyond this task's status row.**
Inputs: `handoffs/TASK-701-programmer.md` (the script's design) · `handoffs/TASK-712-programmer.md` **§6 option (a) — the ADOPTED design, and its §5 D5 landmine** · `handoffs/TASK-699-buildmaster.md` · TASKBOARD TASK-714 + the two orchestrator scope messages.

---

## 0. WHAT CHANGED — two files

| File | Before | After | What |
|---|---|---|---|
| `Tools/Packaging/ship.ps1` | 1420 | **2109** | the C3 seam · the state write · the resume path · `PKG-§10` hygiene · the dry-run seam text · header docs |
| `.claude/commands/ship.md` | 162 | **199** | three-verdict truth surface · **new §2b** (the adjudication procedure) · gate-table rows · README/report duties · `PKG-§9f` + `PKG-§10` hazards |

**Mechanical checks run (all file-only; ⛔ neither file was executed):**
- `[System.Management.Automation.Language.Parser]::ParseFile` → **PARSE: CLEAN, 0 errors**
- byte audit → **0 non-ASCII bytes** (701's D3 discipline held; `SHIP-8b` inside the script, `SHIP-§8b` only in the markdown — grep for the section sign in the script returns **0**)
- the verdict-record validator was extracted into a **scratchpad snippet** and exercised over **15 cases** (§7). ⛔ **That is a copy of the predicate, not a run of `ship.ps1`.**

⚠️ **No git baseline exists for a textual diff — both files are still UNTRACKED** (`git ls-files` → empty; 701 created them and nothing has been committed yet). The no-other-gate-moved proof in §5 is therefore an **AST inventory**, which is stronger than a line diff for this claim and is reproducible in one command.

---

## 1. THE THREE TERMINAL VERDICTS (`SHIP-§1` as amended)

| Verdict | Exit | Emitted by | Is it a ship? |
|---|---|---|---|
| `SHIP RESULT: PASS` | 0 | PHASE G | ✅ yes |
| `SHIP RESULT: STOP at <gate> - <reason>` | 2 | `Assert-Gate` → `throw 'SHIP-STOP'` | ⛔ no |
| ⭐ `SHIP RESULT: ADJUDICATE C3 - <capture path>` | **4** | `Note-Adjudicate` → `throw 'SHIP-ADJUDICATE'` | ⛔ **no** |
| `DRYRUN-OK` / `DRYRUN-WOULD-STOP` | 0 / 3 | PHASE G | ⛔ no |

- ⛔ **`ADJUDICATE` is not a pass and is not a ship.** The summary prints, above the truth surface: *"THIS RUN DID NOT SHIP. IT IS SUSPENDED AT C3-BOOT-ARENA… an unresolved suspension is exactly as unshipped as a STOP."* The header documents **exit 4 is not success — treat it exactly as seriously as exit 2.**
- ✅ **The free property is stated in the output, not just believed:** C3 precedes PHASE D, so at the stop **no zip exists yet**; the previous artifact is untouched and there is nothing to clean up.
- The gate row renders as status **`ADJUD`**, distinct from `PASS`/`STOP`/`PLAN`/`CHECK`.

---

## 2. THE VERDICT-RECORD SCHEMA (`SHIP-§8b`) — a measured artifact, ⛔ never a flag

**Path:** `<staging>\.ship\ship-adjudication.json` — deliberately **beside** `ship-state.json` at a **stable** path, ⛔ **not** in the per-run stamped dir: a resume is a new run with a new stamp, and a record the next invocation cannot find is a record that does not exist. **The script only ever READS it.**

```json
{
  "schema": 1,
  "gate": "C3-BOOT-ARENA",
  "verdict": "PASS",                      // PASS | FAIL only, case-sensitive
  "adjudicatedBy": "<who looked>",
  "adjudicatedUtc": "<ISO-8601 UTC>",
  "capturePath":   "<...>\\bootverify-arena.png",
  "captureSha256": "<lowercase hex>",
  "head":          "<40-hex HEAD>",
  "config":        "Shipping",
  "stageExeBytes": 177716736,
  "stageExeUtc":   "<round-trip 'o'>",
  "observations": { "arena": "...", "deck": "...", "hud": "...", "hero": "..." },
  "reason": "<optional, echoed on FAIL>"
}
```

**Every rule is mechanically enforced, which is exactly what keeps it from degenerating into a flag:**

| # | Rule | Enforced at | Failure mode |
|---|---|---|---|
| 1 | `schema` = 1, `gate` = `C3-BOOT-ARENA` | `C3-VERDICT-FORM` | STOP |
| 2 | `verdict` ∈ {`PASS`,`FAIL`}, **case-sensitive** (`-cne`) | `C3-VERDICT-FORM` | STOP — no third state, no `"pass"` |
| 3 | all four observations **present, non-empty** | `C3-VERDICT-FORM` | STOP |
| 4 | no observation is a **bare verdict word** (23-entry ban list: `PASS`, `criteria met`, `ok`, `verified`, …) | `C3-VERDICT-FORM` | STOP |
| 5 | each observation **≥ 24 chars AND ≥ 4 words** | `C3-VERDICT-FORM` | STOP |
| 6 | **on a `PASS` only:** no hedge (18-entry list: `probably`, `i think`, `cannot tell`, `unreadable`, …) | `C3-VERDICT-FORM` | STOP — `SHIP-§8b(4)`, ambiguity is a FAIL |
| 7 | `adjudicatedBy` / `adjudicatedUtc` non-empty | `C3-VERDICT-FORM` | STOP |
| 8 | capture **still exists** and **re-hashes** to `captureSha256` | `C3-VERDICT-BINDING` | STOP |
| 9 | the record's capture **is the one the state file marks pending** (path + hash) | `C3-VERDICT-BINDING` | STOP |
| 10 | `head`, `config`, staged exe **size** and **timestamp** re-measured | `C3-VERDICT-BINDING` | STOP |
| 11 | `verdict` is `PASS` | `C3-VERDICT` | STOP with the named reason |

⭐ **The doctrine, implemented literally:** the script **cannot** judge whether the observations are TRUE — it has no eyes. It **can** require that they were **MADE**, and rules 3–6 are that requirement. Same as `KBD-§2a` condition 3: *prove survivors by naming the objects, never by a summary.*

⚠️ **Rule 6 is deliberately asymmetric and PASS-only.** It can only ever make a PASS **harder** to obtain; a `FAIL` may hedge freely (a FAIL is what you record when you are unsure). QA should confirm it agrees with that direction.

⚠️ **Rule 9 is the one I added beyond the law's literal list, and it closes a real hole:** without the cross-check to the state file's `bootCapture`, an **older** capture that still exists and still hashes would validate — a stale PASS replayed onto a fresh cook.

---

## 3. THE RESUME TRUTH TABLE (`SHIP-§8d`) — state × re-measured identity → outcome

**The repair in one sentence:** the state write (formerly `ship.ps1:1129-1143`) is now **reachable** — it executes **before** the adjudication stop, so the resume is armed before the run suspends. `boot` now carries a third honest value.

| # | State `boot` | Verdict record | Re-measured identity | Outcome |
|---|---|---|---|---|
| 1 | *(no state)* | — | — | full A+B+C; route Pixel ⇒ capture ⇒ **`ADJUDICATE C3`** |
| 2 | `ADJUDICATE` | **absent** | — | re-cook (reuse declined on `boot verdict`) ⇒ capture ⇒ **`ADJUDICATE C3`** again |
| 3 | `ADJUDICATE` | **malformed** | not reached | ⛔ **STOP `C3-VERDICT-FORM`** — before PHASE B, so **no re-cook is spent** |
| 4 | `ADJUDICATE` | `PASS` | **all identical** | ✅ **REUSE B+C ⇒ PHASE D→E→F→G on the SAME staged build, ⛔ NO second cook** |
| 5 | `ADJUDICATE` | `PASS` | capture **hash** differs | ⛔ **STOP `C3-VERDICT-BINDING`** |
| 6 | `ADJUDICATE` | `PASS` | capture ≠ the **pending** capture | ⛔ **STOP `C3-VERDICT-BINDING`** |
| 7 | `ADJUDICATE` | `PASS` | **HEAD** differs | ⛔ **STOP `C3-VERDICT-BINDING`** |
| 8 | `ADJUDICATE` | `PASS` | **config** differs | ⛔ **STOP `C3-VERDICT-BINDING`** |
| 9 | `ADJUDICATE` | `PASS` | staged exe **size or mtime** differs | ⛔ **STOP `C3-VERDICT-BINDING`** — *"this is a DIFFERENT BUILD"* |
| 10 | `ADJUDICATE` | `PASS` | capture **deleted** | ⛔ **STOP `C3-VERDICT-BINDING`** |
| 11 | `ADJUDICATE` | **`FAIL`** | identical | ⛔ **STOP `C3-VERDICT`** with the named reason. **The ship ends.** Nothing written; previous zip untouched |
| 12 | `PASS` (route Log) | irrelevant | tree/recipe/exe identical | ✅ reuse B+C (pre-existing 701 D2 behaviour, **unchanged**) |
| 13 | `ADJUDICATE` | `PASS`, binds | but **treeHash/recipe/suite** differ | re-cook in full (the pre-existing reuse guards still apply) ⇒ new capture ⇒ **`ADJUDICATE`** again |
| 14 | any | present, route = **`Log`** | — | ⛔ **record NOT consulted at all** — route 1 behaviour is byte-for-byte what it was |
| 15 | any | present, **`-DryRun`** | — | ⛔ **not evaluated** — a dry run must not consume an adjudication; a fact row says so |

**Rows 5–10 are the point of the whole seam.** ⚠️ *Resuming onto a different build than the one adjudicated is the single worst failure this seam could produce*, and it is closed **by measurement, not by care**. ⛔ Every one is a **STOP**, never a warning.

⚠️ **Row 4 does not skip a gate.** `$AdjPass` cannot be true unless `C3-VERDICT-BINDING` and `C3-VERDICT` both passed — they **throw** otherwise — and the reuse block's own pre-existing guards (schema, HEAD, treeHash, recipeHash, compile, cook, suiteTotal, staged-exe) still all apply on top. It is a **second lock on the same door, ⛔ not a second door.**

**Ordering ruling I made and am declaring:** the seam is evaluated **before PHASE B**, so a `FAIL` or a malformed record costs **seconds**, not another compile + suite + 30-minute cook (rows 3, 11).

---

## 4. `SHIP-§8c` — THE PRE-FILTER MAY ONLY FAIL

`C3-CAPTURE` runs five cheap checks: **capture exists · ≥ 1024 B · decodes as an image · not uniformly blank · a live game process was found · the window title was read.**

- ⛔ **It records `CHECK`, never `PASS`** — a dedicated `Assert-PreFilter` primitive exists precisely so it cannot emit a PASS row that a sweeper might misread. Its evidence string opens with **"MECHANICAL PRE-FILTER ONLY (SHIP-8c) - THIS IS NOT A PASS OF C3-BOOT-ARENA AND CAN NEVER BECOME ONE."**
- ⛔ **There is no "looks like an arena" heuristic**, and the comment says why in the file: *it would pass a black screen with a loading spinner while wearing a gate's clothes, and a fake gate is worse than no gate.*
- ✅ **"Not uniformly blank" is a mechanical property, not a content judgement**: a 32×18 sample grid, distinct-ARGB count ≥ 2. The count is printed so the adjudicator sees it.
- **Blast radius:** a pre-filter failure STOPs **before** the state write, so nothing is armed and the next run re-cooks. Correct — there is nothing to adjudicate.

---

## 5. ⛔ PROOF THAT NO OTHER GATE MOVED

**(a) AST gate inventory with each gate's exact `-Ok` expression.** Reproduce with:

```powershell
$ast=[System.Management.Automation.Language.Parser]::ParseFile($f,[ref]$null,[ref]$null)
$ast.FindAll({param($n) $n -is [System.Management.Automation.Language.CommandAst]},$true) |
  Where-Object { $_.GetCommandName() -in 'Assert-Gate','Assert-PreFilter','Note-Skipped','Add-GateRecord','Note-Adjudicate' }
```

**Every pre-existing gate keeps its condition expression character-for-character:**

`A1-UPROJECT ($found.Count -eq 1)` · `A1-REPO $topRes.Ok` · `A1-ENGINE ((Test-Path $RunUAT) -and …)` · `A1-STAGING ([bool]$StagingDir)` · `A2-NO-MID-OPERATION ($midMerge.Count -eq 0)` · `A3-QUIET-MODULE ($busy.Count -eq 0)` · `A4-FENCE $fenceOk` · `A5-DISK ($freeGb -ge $MIN_FREE_GB)` · `A6-EVIDENCE-ROUTE $A6Ok` · `A7-COOKDIRS ($missingDirs.Count -eq 0)` · `A7-MAPS ($missingMaps.Count -eq 0)` · `A7-BOOTMAP $bootMapOk` · `B1-COMPILE $ok` (+ the SAC `$false` arm) · `B2-SUITE $ok2` · `C1-COOK (-not $r3.TimedOut)` · `C2-UAT-LOG ($uatOk -and -not $uatBad)` · `C2-STAGE-PRESENT ((Test-Path …$StageExe) -and (Test-Path …$StageBinExe))` · **`C3-BOOT-ARENA $bootOk` (the Log route — UNCHANGED)** · `C3-BOOT-TITLE ($windowTitle -like '*Siegebound*')` · `C4-NO-MODELS ((…-eq 0) -and (…-eq 0))` · `D0-README $readmeFresh` · `D2-ZIP-READBACK $verifyOk` · `D2-SIZE-SANITY ($ratio -ge $SIZE_ALARM_RATIO)` · `F1-COMMIT-PATHS ($rejected.Count -eq 0)` · `F2-INDEX-CLEAN ((…-eq 0) -and (…-eq 0))` · `F2-COMMIT $ci2.Ok`.

**Δ inventory — exactly one removal and seven additions, all inside my fence:**

| Change | Gate | Why it is in fence |
|---|---|---|
| ⛔ **REMOVED** | `Assert-Gate 'C3-BOOT-ARENA' -Ok $false` (the unconditional pixel-route stop) | **this is the task** |
| ➕ | `C3-CAPTURE` (`Assert-PreFilter`) | `SHIP-§8c` |
| ➕ | `C3-VERDICT-FORM`, `C3-VERDICT-BINDING`, `C3-VERDICT` | `SHIP-§8b`/`§8d` |
| ➕ | `C3-BOOT-ARENA` via `Note-Adjudicate` (the suspension) | `SHIP-§8a` |
| ➕ | `C2-STAGE-MANIFESTS`, `C2-STAGE-PRUNE`, `C2-STAGE-ONE-EXE` | scope **+1**, `PKG-§10` |

**(b) The parameter block is byte-identical** — 9 parameters, ⛔ **none added, removed, renamed or re-defaulted.**

**(c) No skip flag, by name:** `-SkipTests|SkipCook|SkipSuite|NoVerify|Force|IgnoreGates|AcceptWarnings|FastShip|YesReally|PixelAdjudicated|Adjudicated|AcceptPixel` → **0 switch declarations and 0 variable references each.** `-PixelAdjudicated` is additionally **named in the file header as permanently banned**, with the reason (*"a flag is an assertion; a record is a measurement"*).

**(d) Git posture, unchanged:** `git add -A` **0** · `git add .` **0** · `git push` **0** · `reset --hard` **0** · `filter-branch` **0** · bare `push` **4** (prohibition prose, matching 712's corrected count) · `$LASTEXITCODE` **3**.
> ⚠️ **Transcription note for QA, so nobody trips:** 712's table lists `--amend` = 2. The literal two-dash string `--amend` occurs **once** (the `Invoke-Git` prohibition comment); the *word* "amend" occurs 5×, of which 2 are pre-existing prose (`ship.ps1` comment + the `F2-COMMIT` evidence string *"no amend, no push"*) and **3 are mine** — all in new comments reading *"SHIP-1 as amended"*. ⛔ **`git --amend` invocations remain 0 and I changed no git call.**

**(e) `PKG-§6a` is untouched, by construction.** `$BOOT_MARK_ARENA` / `$BOOT_MARK_DECK` / `$BOOT_MARK_HERO` / `$BOOT_RE_DECK` / `$BOOT_FAIL_PATTERNS` / `$BOOT_BENIGN_PATTERNS` are **unmodified**, and so are `$SUITE_BASELINE = 143`, `$MIN_FREE_GB = 4`, `$RETENTION_KEEP = 2`, `$SIZE_ALARM_RATIO = 0.60`. **Only the instrument and the judge moved; the bar did not.** The bar is now *printed verbatim* at the stop, which makes it harder to drift, not easier.

---

## 6. SCOPE ADDITION +1 — `PKG-§10` STAGE HYGIENE (prune-and-assert logic)

**Three gates, one idempotent function `Invoke-StageHygiene`, called TWICE.**

1. **`C2-STAGE-MANIFESTS`** — ≥ 1 `Manifest_*.txt` at the stage root. ⛔ **No manifests ⇒ no authority ⇒ STOP.** We do not guess.
2. **`C2-STAGE-PRUNE`** — for every staged file whose extension is **`.exe` or `.pdb`**, compute its **full stage-relative path** (forward slashes, lowercased) and delete it if that path is in **no** manifest. Sizes and names are printed and recorded.
3. **`C2-STAGE-ONE-EXE`** — after the prune, enumerate `.exe` under the stage **excluding `Engine/`**, split into *root-level* (the shim) and *nested* (runnable game binaries), and assert **exactly one runnable**. On failure, **every offending path is named**.

**Call sites — the order is the ruling:**
- **inside the cook branch, after `C2-STAGE-PRESENT` and BEFORE the boot-verify launch** — *verify the artifact you actually ship*;
- **outside the branch, before PHASE D** — so a **reused** stage is also asserted by the run that writes the archive.

### ⚠️ BOTH OF TASK-715's LIVE TRAPS ARE WRITTEN INTO THE FILE AS COMMENTS

- **TRAP 1 — "delete every non-manifest file" deletes the entire game.** The `.ucas` (1.078 GB) and `.utoc` are in **no** manifest **by design** (the UFS manifest lists the 3,446 assets packed *into* the container, not the container), and the `.pak`'s presence makes the naive rule **look safe on a spot-check**. 715 measured **28 of 70** staged files non-manifest, of which only **2** were orphans. ⇒ The `.exe`/`.pdb` filter carries an inline **"TRAP 1 GUARD - DO NOT REMOVE THIS FILTER"** comment plus the full block-header explanation.
- **TRAP 2 — a basename grep exonerates the orphan.** `GitClaudeUnrealTest.exe` matches the **legitimate root shim** (the player's click target; 715 proved by PID→path that it resolves to the Shipping exe). ⇒ Membership is tested on the **full relative path**, with an inline **"TRAP 2 GUARD"** comment.
- The invariant is **scoped to GAME exes** for the reason 715 measured: the two `Engine/Extras/Redist/…/vc_redist*.exe` installers and the root shim are all legitimate and would otherwise fail a clean stage.

### ⚠️⚠️ THE COLLATERAL REPAIR THIS GATE FORCED — QA MUST RULE ON IT (declared, not smuggled)

**Landing the prune was impossible without it.** `ship.ps1` hardcoded the staged game binary in **three** places as `<Project>/Binaries/Win64/<Project>.exe`:

| Site | Used by | Under **Shipping** that path is… |
|---|---|---|
| `$StageBinExe` | `C2-STAGE-PRESENT`, the state's `stageExeUtc`, the reuse "staged exe changed" check, the `Staged exe` size fact | **the 347 MB stale Development ORPHAN** |
| `$pdb` | the `Staged pdb` size fact | the orphan's `.pdb` |
| `$realBinary` | **`D2-ZIP-READBACK`** | **the orphan** |

⇒ **As written, the script bound its build identity to a file `PKG-§10` requires deleting, and `D2-ZIP-READBACK` would have failed every clean Shipping package.** Verified at the live stage: `Manifest_NonUFSFiles_Win64.txt` names `…/GitClaudeUnrealTest-Win64-Shipping.exe`, while `…/GitClaudeUnrealTest.exe` sits beside it in **zero** manifests.

**Repair:** a `Get-StageGameExeRel` helper resolves the binary **from the manifests** (exactly one `.exe` directly under `<Project>/Binaries/Win64/`), falling back to the **UE naming convention for the requested configuration** only when no manifests exist (an uncooked stage, where nothing resolves anyway). It is **re-resolved after the cook**, because the PHASE-A value came from the *previous* cook's manifests and may have been a different configuration entirely. ⛔ **All three gates got stricter/truer; none was relaxed.**

---

## 7. SCOPE ADDITION +2 — `PKG-§9f` IN THE PROCEDURE

`ship.md` gains **§2b.1 — Reach the arena the way a player does**, stating: the measured A/B (map arg, bare map name, **and** `-ExecCmds` all boot to the menu on Shipping; Development logs `LoadMap`); the ruled route (**launch with no args, drive the game's own menu with `t669_topclick.ps1`, capture, adjudicate under `SHIP-§8`**); why it is **strictly better evidence** (it exercises the real menu → level-travel → HUD path); the honest cost (**unlocked desktop ⇒ a Shipping `/ship` is SCHEDULABLE, ⛔ not unattended**); the separately-provable **content half** (UnrealPak listing + the Development exe against this package's own cooked bytes) with ⛔ *neither substitutes for the other*; and the forbidden conclusion, stated as such: ⛔ **this does not lower `PKG-§6a` — "hard to reach" is an argument about the INSTRUMENT, never about the BAR**, with the perfect-menu/unplayable-game precedent attached.

`ship.ps1`'s dry-run boot plan prints the same `PKG-9f` warning beside the launch line it would otherwise imply works, so the stale instruction cannot be followed from the script alone.

---

## 8. THE DRY-RUN SEAM TEXT (`SHIP-§7` amended) — exact output

A `-DryRun` now prints, in PHASE C, **the whole adjudication contract it cannot exercise**, led by a line that names this run's resolved route:

```
  This run resolved evidence route PIXEL, so a LIVE run of this exact
  invocation WOULD end at: SHIP RESULT: ADJUDICATE C3 - <capture>.
------------------------------------------------------------------------------
 ADJUDICATION CONTRACT (SHIP-8) - PRINTED BECAUSE A DRY RUN CAN NEVER REACH IT
------------------------------------------------------------------------------
  WHY THIS EXISTS: this script cannot read a PNG and will never claim it
  did.  You can.  Judgment moves to where sight exists; THE BAR DOES NOT
  MOVE (PKG-6a is untouched).
  ... the four PKG-6a criteria VERBATIM ...
  ... the six verdict rules ...
  WRITE THE RECORD TO:  <staging>\.ship\ship-adjudication.json
  EXACT SCHEMA (UTF-8 JSON; replace every <...> with what you SAW): { ... }
  THEN RE-INVOKE.  A PASS resumes at PHASE D (zip) -> E -> F -> G against
  THE SAME STAGED BUILD, with NO SECOND COOK. ...
    & "<...>\ship.ps1" -Configuration Shipping -BootEvidence Pixel
------------------------------------------------------------------------------
```

⛔ **It prints under route `Log` too** (with an honest lead line saying route 1 does not reach it), because *a dry run is not evidence about phases C–F either way*. The dry-run summary additionally carries a **DRY RUN SCOPE** block naming this exact history: *"an unconditional stop at C3 once sat in this script behind a green dry run, and it would have surfaced only on the first real ship, after a 30-minute cook."*

**The resume line is composed from what the run actually resolved** — `$PSCommandPath` plus `-Configuration`, `-BootEvidence Pixel`, and any of `-ProjectPath|-EngineRoot|-StagingDir|-ShipLogDir` that were **explicitly bound** (`$PSBoundParameters.ContainsKey`). ⛔ Never typed from memory.

---

## 9. ⚠️ 712's LANDMINE — HEEDED, AND HOW

712 D5: **PowerShell re-enforces a parameter's `[ValidateSet]` on every later assignment**, so a sentinel assigned to `$BootEvidence` throws *inside* the gate and surfaces as a generic exit-1 crash instead of a clean STOP.

⇒ **I assign NO parameter anywhere.** `$BootEvidence`, `$Configuration` and `$DryRun` are **read-only** throughout my changes. Every new state lives in fresh, unvalidated variables (`$AdjPass`, `$AdjInstrument`, `$AdjRec`, `$AdjObs`, `$bootVerdict`, `$StageGameExeRel`, `$pixelSha`, …). The third verdict is carried by a **distinct exception message** (`SHIP-ADJUDICATE`) and script-scope carriers, ⛔ never by a sentinel in a validated variable.

**A second StrictMode hazard I closed deliberately:** the verdict record is **authored by hand**, so a missing property is an *expected input*, not a bug — and under `Set-StrictMode -Version Latest` a bare `$rec.foo` on an absent property **throws**, which would surface a malformed record as `UNEXPECTED-ERROR` (exit 1) instead of a clean `STOP`. Every record/state read therefore goes through **`Get-JsonProp`**, which tests `PSObject.Properties[...]` first. ⭐ **This is the same class of defect as the landmine: a gate failing in the wrong voice.**

---

## 10. VALIDATOR TEST RESULTS (isolated snippet, ⛔ not a run of `ship.ps1`)

15/15 as intended:

| Case | Result |
|---|---|
| a good `PASS` record | ✅ accepted |
| observation is bare `"PASS"` | ⛔ rejected |
| observation is `"criteria met"` | ⛔ rejected |
| observation empty | ⛔ rejected |
| observation too short / too few words | ⛔ rejected (both reasons named) |
| hedged `PASS` (*"probably … hard to be sure"*) | ⛔ rejected |
| lowercase `"pass"` | ⛔ rejected |
| third value `"PROBABLY"` | ⛔ rejected |
| a well-formed `FAIL` | ✅ **accepted by FORM**, then ⛔ **STOPs at `C3-VERDICT`** — the intended two-stage design |
| a `FAIL` that hedges | ✅ accepted (hedge check is PASS-only, by design) |
| `observations` block missing | ⛔ rejected |
| wrong `gate` / wrong `schema` / invalid JSON / empty `adjudicatedBy` | ⛔ rejected |

⚠️ **QA: do not misread row 9's "accepted".** `C3-VERDICT-FORM` answers *"is this a real record?"*; `C3-VERDICT` answers *"does it say PASS?"*. A well-formed FAIL must pass the first to reach the second, where it stops the ship **with its named reason** — which is exactly what a FAIL is for.

---

## 11. DECLARED DEVIATIONS AND INTERPRETATIONS (`SC-§15`)

**D1 — The three-site `$StageBinExe` / `$pdb` / `$realBinary` repair (§6).** The largest thing I did that the original brief did not name. **It is forced:** scope +1 orders the prune, and the prune deletes the exact file those three sites pointed at under Shipping. ⛔ Not landing it would have shipped a gate that breaks the ship. **All three become stricter; none is relaxed.** If QA reads the fence more narrowly, note that the prune and the derivation must land or revert **together** — ⛔ separating them is the one combination that is worse than either.

**D2 — A binding mismatch is a STOP, ⛔ not a silent re-cook.** A silent re-cook would also be *safe*, but `SHIP-§8d` says *"any mismatch is a STOP, never a warning"*, and a STOP is more informative: it tells the operator *"you adjudicated build X and you are now at build Y."* **Cost, stated honestly:** a stale record **blocks** the next ship until it is deleted, and the remedy string names the exact file path to delete. I judged an explicit one-command speed bump better than an implicit 30-minute re-cook.

**D3 — The record is consulted ONLY under route `Pixel`, and never in a `-DryRun`.** Keeps route 1 byte-for-byte unchanged (proof §5) and prevents a dry run from consuming an adjudication. A fact row reports the record's presence either way, so it is never invisible.

**D4 — The adjudication is evaluated BEFORE PHASE B** (rows 3 and 11). ⛔ Not stated in the law; it follows from *"prove, then cook"* and it is what makes a `FAIL` cost seconds instead of a second cook.

**D5 — Rule 9 (the pending-capture cross-check) exceeds the law's literal binding list.** `SHIP-§8b(1)` names capture path+hash and D2's identity set; I added *"and it must be the capture the state file marks pending"* because without it a still-hashing older capture validates. **Strictly stronger; nothing is weakened.**

**D6 — The hedge check (rule 6) is my mechanisation of `SHIP-§8b(4)`.** The law makes ambiguity the *adjudicator's* duty; I made a narrow, PASS-only word list enforce it mechanically. ⚠️ **The honest cost: a legitimate PASS observation containing e.g. "possibly" is rejected** and must be reworded. The STOP text says exactly that and offers the two correct exits (say what you saw, or record FAIL). **QA should rule on the word list** — it is one constant, `$ADJ_HEDGES`, and trims cleanly.

**D7 — The pre-filter records `CHECK`, a new status string, rather than `PASS`.** `SHIP-§8c` says the pre-filter *"can never produce a PASS"*; emitting a `PASS` row — even for a mechanical sub-check — invites exactly the misreading the clause exists to prevent. New primitive `Assert-PreFilter`; the STOP path delegates to `Assert-Gate` so there is **one** stop mechanism.

**D8 — Exit code 4 is new.** 0/1/2/3 were taken. The header states **"EXIT 4 IS NOT SUCCESS. Treat it exactly as seriously as exit 2."** ⚠️ **Any caller that branches on `-eq 0` vs `-ne 0` is already correct**; only a caller that treats "not 2" as success would be wrong, and the truth surface (the last stdout line) remains authoritative per the exit-code-lie law.

**D9 — `$STATE_SCHEMA` bumped 3 → 4.** The state gained `bootCapture`, `bootCaptureSha`, `stageExeRel`, `stageExeBytes`, and `boot` gained the value `ADJUDICATE`. A schema-3 file is simply not reused (full re-run) — ⛔ and it is **not** a StrictMode hazard, because schema 4 only **adds** fields, so every property the reuse block reads still exists on an old file.

**D10 — `ship.md` §2b is a substantial new section, not a table row.** The brief says *"update ship.md's procedure + gate table"*; the procedure for a step that did not previously exist could not be expressed as a row. Everything outside §2b is targeted edits to existing rows/lists.

**D11 — I did not delete the verdict record after a successful ship.** So a subsequent ship of a byte-identical tree reuses B+C without re-adjudicating — which is exactly 701 D2's existing resume semantics for the Log route, and the bindings are re-measured every time. ⛔ Silently deleting an operator's artifact is its own trap. **Flagging it as a design choice for QA to confirm.**

---

## 12. WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐⭐ **The resume truth table (§3) row by row against the code**, especially **rows 5–10** (every identity mismatch is a STOP) and **row 4** (confirm the reuse path genuinely cannot be entered without both `C3-VERDICT-BINDING` and `C3-VERDICT` having passed).
2. ⭐⭐ **D1 (§6, the three-site derivation repair)** — the one change QA could reasonably read as out of fence. Please rule explicitly; note the prune and the derivation must land or revert **together**.
3. ⭐ **§5's AST inventory** — reproduce it; the whole "only C3 + hygiene moved" claim rests on it, and there is **no git baseline** to diff against.
4. **D6 — the `$ADJ_HEDGES` list.** Is mechanising `SHIP-§8b(4)` right, and is the word list too broad?
5. **D2** — stale record STOPs rather than silently re-cooking. Confirm you read `SHIP-§8d` the same way.
6. **`SHIP-§8c` compliance** — confirm `C3-CAPTURE` cannot produce a PASS of the arena claim, and that the blank-frame sample is a *mechanical* property and not a content heuristic.
7. ⭐ **PKG-§10 TRAP 1** — confirm the `.exe`/`.pdb` filter is present, commented, and cannot be widened; and **TRAP 2** — confirm membership is tested on the full relative path (`Get-StageRelPath`), never a basename.
8. **ASCII discipline** — grep the script for `SHIP-8b`, ⛔ **not** `SHIP-§8b`. Verified **0 non-ASCII bytes**, section-sign hits **0**.
9. **The no-skip-flag sweep** (§5c) — reproduce it, including `-PixelAdjudicated`.

## 13. STATE

⛔ **Nothing executed**: no compile, no cook, ⛔ **no run of `ship.ps1` (not even `-DryRun` — 703 owns that)**, no editor, no MCP, no `git add`, no commit, no push. ⛔ **No engine or content file touched.** The only code I ran was a **scratchpad copy** of the record-validation predicate (§10).
Files touched: **`Tools/Packaging/ship.ps1`** · **`.claude/commands/ship.md`** · this handoff · the TASKBOARD status row for TASK-714 only.
Status → **ready-for-qa**. ⭐ **TASK-702 gates 701 + 712 + 714 as ONE artifact**; this handoff is written to be read after those two.
