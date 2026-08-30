# QA Report — TASK-702 — the SHIP lane reviewed as ONE artifact (TASK-701 + TASK-712 + TASK-714)

**Verdict: FAIL — 3 BLOCKERS · 5 WARNS · 7 NITS**

Reviewed 2026-08-30 · reviewer: qa-reviewer · read-only.
Artifacts: `Tools/Packaging/ship.ps1` (2,109 lines) · `.claude/commands/ship.md` (277 lines).
Inputs: `handoffs/TASK-701-programmer.md` · `TASK-712-programmer.md` · `TASK-714-programmer.md` · `handoffs/TASK-715-buildmaster.md` · CONVENTIONS `SHIP-§0..§8d`, `PKG-§5a/§6a/§7a/§7b/§9a-1/-2/-3/§9c/§9d/§9e/§9f/§10/§11` · TASKBOARD TASK-701/702/703/712/714.
⛔ Nothing executed — **`ship.ps1` was NOT run, not even `-DryRun`** (TASK-703 owns that). ⛔ No edits, no compile, no engine, no Git, no TASKBOARD edit beyond this task's status row.

> ### ⚖️ THE HEADLINE, STATED HONESTLY BEFORE THE FINDINGS
> **The three implementers did good work and the hard parts are RIGHT.** The three-verdict contract, the mechanically-validated verdict record, the pre-PHASE-B resume seam, the `PKG-§10` trap guards and — above all — **the collateral `$StageBinExe` repair are correct, and the collateral repair is the single highest-value thing in this diff.**
> **It fails anyway, on one measured fact:** the boot-verify still resolves the running game **by `$ProjectName`**, and under Shipping the game process is `<Project>-Win64-Shipping` — so `C3-BOOT-TITLE` stops **every** Shipping run, **before the state write**, after a 30-minute cook. ⚠️ **That is the exact shape of the defect TASK-714 just repaired, one gate to the left.** The forever-stop moved a fourth time; it did not disappear.

---

## 1. THE DOCKET, RULED ITEM BY ITEM

### 1. The three-verdict contract (`SHIP-§1` / `§8`) — ✅ CORRECT

| Surface | Verified at | Result |
|---|---|---|
| `PASS` → exit 0 | `ship.ps1:2240-2241` | ✅ |
| `STOP at <gate> - <reason>` → exit 2 | `:405`, `:2244-2246` | ✅ |
| ⭐ `ADJUDICATE C3 - <capture>` → **exit 4** | `:566`, `:2252-2254` | ✅ |
| `DRYRUN-OK` / `DRYRUN-WOULD-STOP` → 0 / 3 | `:2233-2236` | ✅ |

⛔ **An unresolved suspension is as unshipped as a STOP — verified on all four surfaces, not just the banner:**
- **truth surface** (`:922`, last stdout line): `SHIP RESULT: ADJUDICATE C3 - <path>` — cannot be pattern-matched as `PASS`;
- **banner** (`:890-899`): *"THIS RUN DID NOT SHIP. IT IS SUSPENDED… ADJUDICATE IS NOT A PASS AND IS NOT A SHIP"*, printed **immediately above** the truth surface (correct placement — a bar restated 900 lines up is a bar that gets remembered instead of read);
- **file header** (`:117-121`, `:139`): *"EXIT 4 IS NOT SUCCESS. Treat it exactly as seriously as exit 2."*;
- **gate table**: status renders `ADJUD`, distinct from `PASS`/`STOP`/`CHECK`/`PLAN`;
- **`ship.md:39-53`**: the three-verdict table + *"It succeeded at cooking. It has not shipped."*

⭐ **The one residual mis-read risk is named by the implementer itself (714 D8) and I confirm it:** a caller that treats *"not 2"* as success is wrong. Every caller in this pipeline reads the truth surface or `-eq 0`; both are correct. ✅ **RULED SOUND.**

### 2. The verdict-record schema (`SHIP-§8b`), 11 rules — ✅ ALL 11 PRESENT AND ENFORCED

Verified rule-by-rule at `:1442-1576` against the handoff's table:

| # | Rule | Site | ✓ |
|---|---|---|---|
| 1 | `schema`=1, `gate`=`C3-BOOT-ARENA` | `:1454-1455` | ✅ |
| 2 | `verdict` ∈ {PASS,FAIL}, **case-sensitive** `-cne` | `:1457-1459` | ✅ `"pass"` rejected |
| 3 | all four observations present, non-empty | `:1463-1472` | ✅ |
| 4 | 23-entry ban list, **whole-string** match | `:293-295`, `:1475` | ✅ counted 23 |
| 5 | ≥ 24 chars **and** ≥ 4 words | `:291-292`, `:1479-1484` | ✅ |
| 6 | 18-entry hedge list, **PASS-only**, substring | `:301-303`, `:1487-1494` | ✅ counted 18 |
| 7 | `adjudicatedBy` / `adjudicatedUtc` non-empty | `:1460-1461` | ✅ |
| 8 | capture exists **and re-hashes** | `:1532-1539` | ✅ |
| 9 | ⭐ record's capture **is the state's pending capture** | `:1515-1530` | ✅ |
| 10 | HEAD · config · staged-exe size **and** mtime | `:1540-1550` | ✅ |
| 11 | `verdict -ceq 'PASS'` | `:1565-1567` | ✅ |

⭐ **The ban list is whole-string (`-contains $lower`), the hedge list is substring (`.Contains($h)`) — and that asymmetry is CORRECT, not an oversight.** A legitimate observation *"…cost pips render correctly, bottom-left"* survives the ban list (`correct` is banned only as the entire observation) while *"the HUD is possibly present"* is caught. **If the two had been implemented the same way, one of them would be broken.** I checked this specifically because it is the kind of thing that reads fine and behaves wrong.

**⭐ RULING ON 714's Q (D6): "could the hedge list reject an honest *possibly*?" — YES, IT CAN, AND THAT IS CORRECT.**
`SHIP-§8b(4)` is unambiguous: *"'I think it's probably fine' is a **FAIL**, not a pass"* and *"the asymmetry is the point."* An observation containing *possibly* is, by the law's own definition, an ambiguous observation, and an ambiguous observation on a PASS record is a FAIL. **The design intent is preserved exactly: because the check is PASS-only (`:1487`), it can only ever make a PASS HARDER to obtain and can never make a FAIL easier.** I re-derived that from the code rather than taking the claim: on a `FAIL` record the loop at `:1488` is never entered, so a FAIL may hedge freely — which is right, since hedging is *why* you record FAIL.
**And the cost is smaller than the handoff feared, which I verified structurally:** `C3-VERDICT-FORM` is evaluated **before PHASE B** (`:1498` vs `Head 'PHASE B'` at `:1627`), and neither the stage nor the state file is disturbed by the STOP — so a rejected wording costs **one text edit and a re-invocation, ⛔ never a second cook.** The STOP text at `:1490` names both correct exits (say what you saw, or record FAIL). ✅ **ADOPTED AS WRITTEN — no trim to `$ADJ_HEDGES` required.**

**⭐ RULING ON D5 / rule 9 (the added pending-capture binding): it genuinely closes the hole.** The claimed hole is *"an older capture that still exists and still hashes would validate."* Verified: the state file's `bootCapture` is rewritten on every cook to the **new run-stamped** path (`:1941`, `$RunLogDir` carries a per-run stamp from `:1004-1005`), so a record naming an older capture fails `$recCap -ne $pendCap` at `:1529` even though that older file still exists and still hashes correctly. **The hole is closed by a path identity that changes every run — a measurement, not a convention.** ✅ **Strictly stronger than the law's literal list; nothing is weakened.**

### 3. The resume path (`SHIP-§8d`), 15-row truth table — ✅ 15/15 CONFIRMED AGAINST THE CODE

I re-derived every row rather than reading the table. All 15 hold.
- **Rows 5–10 (every identity mismatch STOPs at `C3-VERDICT-BINDING`)** — ✅ `:1503-1556`; each mismatch appends to `$mis` and the gate asserts `$mis.Count -eq 0`. There is **no** warn-and-continue path and no `-Warning` branch anywhere in the block.
- **Row 11 (FAIL STOPs at `C3-VERDICT`)** — ✅ `:1565`; the named reason is echoed, falling back to the four observations when `reason` is absent (`:1561-1564`) — a good touch: a FAIL always says *something* concrete.
- **Row 4 (a valid PASS + identical identity resumes D→F→G with no second cook)** — ✅ and **it is genuinely a second lock, not a second door**, which is the claim I checked hardest: `$AdjPass = $true` at `:1569` is reachable **only** past `C3-VERDICT-FORM`, `C3-VERDICT-BINDING` and `C3-VERDICT`, all three of which `throw` on failure in a live run (`:405`); and the reuse block at `:1586-1604` then applies its own pre-existing guards (schema, HEAD, treeHash, recipeHash, compile, cook, suiteTotal, staged-exe mtime) **on top**. ✅
- **⭐ Evaluation before PHASE B — ✅ CONFIRMED AND LOAD-BEARING.** The seam block ends at `:1577`; `PHASE B` begins at `:1627`. A malformed or FAIL record costs seconds. **D4 is right and should be recorded as doctrine, not as a deviation.**
- **Row 14 (route Log never consults the record)** — ✅ `:1431` gates the whole block on `$BootEvidence -eq 'Pixel'`. Route 1 is byte-for-byte unchanged.
- **Row 15 (a `-DryRun` never consumes an adjudication)** — ✅ `:1425-1430`, and it still reports the record's presence as a fact, so it is never invisible.

**⭐ RULING ON D2 — "a stale record STOPs rather than silently re-cooking": THE TRADE IS RIGHT, THE IMPLEMENTATION IS NOT COMPLETE. See BLOCKER-3.**
- **The trade itself: ACCEPTED.** `SHIP-§8d` says *"any mismatch is a STOP, never a warning"*, and a STOP is strictly more informative than a silent re-cook — it tells the operator *"you adjudicated build X and you are standing on build Y"* instead of quietly burning 30 minutes. The remedy string at `:1556` names the exact file to delete. **An explicit one-command speed bump beats an implicit 30-minute re-cook. I read `SHIP-§8d` the same way the implementer did.**
- **⛔ But the law's word is *resume*, and the code cannot tell a resume from a fresh ship** — because D11 leaves the record on disk and the state's `boot` is never advanced past `ADJUDICATE` after a successful ship. The consequence is mechanical and I traced it end to end: **the first invocation of every subsequent `/ship` after the first successful one STOPs at `C3-VERDICT-BINDING` on `HEAD` mismatch and cannot proceed until a human deletes `ship-adjudication.json`.** That fires on precisely the scenario Jonathan's directive names — *"anytime we make any changes, I can say ship."* ⇒ **BLOCKER-3.** D2 survives; **D11 does not.**

### 4. ⭐ THE COLLATERAL REPAIR — ✅ **IN FENCE, REQUIRED, AND CORRECT AT SOURCE. THE HIGHEST-VALUE ITEM IN THE DIFF.**

**Verified against `handoffs/TASK-715-buildmaster.md` §1a/§1b/§4, i.e. against measured bytes, not against the handoff's narration.**

The pre-repair hardcode `<Project>/Binaries/Win64/<Project>.exe` resolves — **under Shipping** — to `GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest.exe`, **which is precisely the 347,694,080-byte orphan `PKG-§10` deletes** (715 §1b: 0 hits in all three manifests, `ProductName = Third Person Game Template`, mtime 2026-08-29 16:56). Three gates were bound to it:

| Site | Consumer | Without the repair, on a correctly-pruned Shipping stage |
|---|---|---|
| `$StageBinExe` | `C2-STAGE-PRESENT`, `stageExeUtc`, the reuse "staged exe changed" check, the `Staged exe` fact | **STOP** — `Test-Path` on a file the prune just deleted |
| `$pdb` | the `Staged pdb` fact | reports `absent` for a `.pdb` that is present |
| `$realBinary` | **`D2-ZIP-READBACK`** | **STOP** — every clean Shipping package fails read-back |

**The derivation (`Get-StageGameExeRel`, `:764-782`) checked against 715's measured stage:**
- convention fallback for Shipping = `GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest-Win64-Shipping.exe` — **byte-for-byte the survivor 715 names** (§4, `Manifest_NonUFSFiles_Win64.txt` line 4). ✅
- manifest resolution filters `startsWith('<project>/binaries/win64/')` **and** `endsWith('.exe')` **and** no further `/` — so it selects the Shipping exe, ⛔ excludes the **root shim** (`GitClaudeUnrealTest.exe`, no prefix — 715 §2b proved by PID→path that the shim is legitimate and must survive) and ⛔ excludes both `Engine/Extras/Redist/…/vc_redist*.exe`. ✅
- **the 3,446 phantom UFS entries cannot create a false hit** — they are `…/Content/…` paths and are not `.exe`. I checked this because the union set is 3,489 paths while only ~44 files exist on disk. ✅
- **re-resolved after the cook** (`:1766-1768`) — correct and necessary: the PHASE-A value came from the *previous* cook's manifests and may name a different configuration entirely. ✅
- **`Invoke-StageHygiene` returns the on-disk survivor** (`:858`), which after the prune is necessarily manifest-listed — so the invariant and the derivation cannot disagree. ✅

**⭐ THE COUPLING CLAIM (714 D1) — RULED, AND IT BINDS:**
> **The `PKG-§10` prune and the manifest derivation must land or revert TOGETHER.** I verified the asymmetry rather than accepting it: **prune-without-derivation is catastrophic** (`C2-STAGE-PRESENT` and `D2-ZIP-READBACK` both stop on every clean Shipping package — a gate that breaks the ship); derivation-without-prune is merely inert. ⇒ **A future revert of the hygiene gate MUST carry the derivation with it. Recording that here so the next reader meets it at the artifact, not by re-deriving it.**

✅ **RULED: the repair is INSIDE the fence** (scope +1 ordered the prune; the prune deletes the exact file those three sites pointed at — declaring it and landing it was the only non-defective option), **all three sites became stricter or truer, none was relaxed**, and the derivation is right against measured reality. ⭐ **This is the finding that justified holding 702 for one combined review.**

### 5. Stage hygiene (`PKG-§10`) + 715's two traps — ✅ BOTH GUARDS PRESENT **AND OBEYED**

- **TRAP 1 guard** — block header `:698-709` **and** an inline `# TRAP 1 GUARD - DO NOT REMOVE THIS FILTER` at `:807-811`, immediately above the filter. **The implementation obeys it:** `:812-813` `if (($ext -ne '.exe') -and ($ext -ne '.pdb')) { continue }` — the prune can never reach the `.ucas`/`.utoc`. ✅ The comment carries 715's numbers (28 of 70 non-manifest, 2 orphans, 1.078 GB) and the *why it looks safe* (the `.pak` IS listed). ✅
- **TRAP 2 guard** — `:711-717` **and** inline `# TRAP 2 GUARD - FULL STAGE-RELATIVE PATH, NEVER A BASENAME` at `:814-816`. **Obeyed:** `:817-818` builds `$rel` via `Get-StageRelPath` and tests `$m.Set.ContainsKey($rel.ToLowerInvariant())` — ⛔ **no basename appears anywhere in the membership test.** ✅
- **`C2-STAGE-MANIFESTS`** — no manifests ⇒ **STOP**, ⛔ never "prune what looks stale" (`:801-803`). ✅ Correct: the law makes the manifests the *only* authority.
- **⭐ "exactly one runnable exe" scoped to GAME exes** — ✅ `:847-856`: excludes `engine/` (the two `vc_redist` installers), splits root-level (the shim) from nested (game binaries), asserts `nested -eq 1`, and **names every offending path on failure**. On 715's measured post-prune stage this evaluates to `1 runnable + 1 shim` ⇒ PASS. ⛔ Without the `Engine/` exclusion it would have failed a perfectly clean stage — the comment at `:838-846` says exactly that, citing 715.
- **ORDER** — ✅ `PKG-§10`'s *prune → measure → boot-verify → zip* is honoured: hygiene at `:1778` (after `C2-STAGE-PRESENT`, **before** the boot launch at `:1788`), sizes measured at `:1989-1999`, second idempotent hygiene call at `:1971-1975` **before PHASE D**, zip at `:2045`. ✅ The second call is the right call — a *reused* stage is asserted by the run that writes the archive.

### 6. `SHIP-§7`'s dry-run amendment — ✅ PRESENT AND ACCURATE

- **The contract a dry run cannot exercise is PRINTED** — `Write-AdjudicationContract -Prospective` at `:1732`, headed *"PRINTED BECAUSE A DRY RUN CAN NEVER REACH IT"* (`:460`). It carries the four `PKG-§6a` criteria **verbatim** from `$ADJ_CRITERIA`, the six verdict rules, the record path, the exact JSON schema, the two PowerShell one-liners that produce the measured fields, and **the resume line composed from what this run actually resolved** (`:1393-1399`, `$PSBoundParameters.ContainsKey` — ⛔ never typed from memory). ✅
- **It prints under route `Log` too** (`:1727-1731`), with an honest lead line. ✅ Correct: *a dry run is not evidence about C–F either way.*
- **The `DRY RUN SCOPE` block** (`:907-919`) states *"A -DryRun pass is NOT EVIDENCE ABOUT PHASES C-F"* and — ⭐ **names this lane's own history as the reason**: *"an unconditional stop at C3 once sat in this script behind a green dry run."* ✅ Accurate: that is exactly what TASK-712 §6 measured.
- **`ship.md:74-76`** carries the same amendment in the procedure. ✅
✅ **The hole `SHIP-§7` was amended to close IS closed.** ⚠️ **And it is worth saying plainly: that amendment is what makes this review's three blockers findable at all — none of them is reachable by TASK-703's dry run.**

### 7. A6 route selection (`PKG-§9a-2`) — ✅ CORRECT, ALL 9 TRUTH-TABLE ROWS HOLD

- installed engine + Shipping + route Log **by default** ⇒ auto-select **Pixel** and **ANNOUNCE it** — ✅ `:1192-1197` + the banner at `:1242-1255` + the gate evidence at `:1231-1233` + the fact row at `:1264-1268`. **Three surfaces, all reaching the final summary.** ⛔ A silent switch is impossible.
- STOP only when no route exists — ✅ explicit `-BootEvidence Log` on an installed engine (`:1185-1191`, cites `PKG-9a-1`), **or** the pixel instrument unavailable (`:1221-1226`, *"a ship that cannot verify its own boot must not ship"*). ✅
- ⛔ **never passes silently with no route** — ✅ **verified mechanically: `Note-Skipped 'A6…'` = 0 hits in the file.** A6 always asserts, in every configuration. That was 712's whole repair and it holds. ✅
- ⛔ **`PKG-§6a` untouched** — ✅ `$BOOT_MARK_*`, `$BOOT_RE_DECK`, `$BOOT_FAIL_PATTERNS`, `$BOOT_BENIGN_PATTERNS`, `$SUITE_BASELINE=143`, `$MIN_FREE_GB=4`, `$RETENTION_KEEP=2`, `$SIZE_ALARM_RATIO=0.60` all unmodified. **Only the instrument moved.** ✅

### 8. Cross-cutting fences — ✅ ALL CLEAN (swept mechanically)

| Fence | Result |
|---|---|
| ⛔ Zero skip flags | ✅ `SkipTests\|SkipCook\|SkipSuite\|NoVerify\|IgnoreGates\|AcceptWarnings\|FastShip\|YesReally\|PixelAdjudicated\|AcceptPixel` → **hits only in the header's ban list** (`:32-33`, `:39`) + one benign fact label `"Adjudicated: "` (`:1574`). **0 parameter declarations.** Param block = 9 parameters, none of them a skip. `-Force` appears only on `Remove-Item`/`Move-Item`/`New-Item`. ✅ |
| ⛔ Nothing self-certifies on log-silence | ✅ `C3` under route Pixel **never** reads a log; the pre-filter records `CHECK` (`:428`) with the evidence string opening *"THIS IS NOT A PASS OF C3-BOOT-ARENA AND CAN NEVER BECOME ONE"*; `Assert-PreFilter` **cannot** emit a PASS row by construction, and its failure path delegates to `Assert-Gate` so there is one stop mechanism. The blank-frame test is a **mechanical** property (32×18 grid, distinct-ARGB ≥ 2, `:1890-1903`) and the count is printed. ⛔ No "looks like an arena" heuristic anywhere. ✅ |
| ⛔ `PKG-§6a` bar unmoved | ✅ (see item 7) — and it is now **printed verbatim at the stop**, which makes drift harder, not easier |
| ⛔ No `git push` / `add -A` / `--amend` | ✅ `git add -A` **0** · `git add .` **0** · `git push` **0** · `reset --hard` **0** · `filter-branch` **0**. The 4 bare `push` hits and the 1 `--amend` hit are **prohibition prose** (`:579`, `:903`, `:2218`, `:2223`). Only two writing git calls exist, both PHASE F: `git add -- <one explicit path>` in a loop (`:2190`) and `git commit -F` (`:2222`). ⛔ No directory sweep. `$LASTEXITCODE` = 3 hits, **one** of them code (`:585`, inside `Invoke-Git`) and annotated in place; ⛔ no build/cook/suite/boot verdict comes from an exit code. ✅ |
| ASCII discipline | ✅ **`ship.ps1`: 0 non-ASCII bytes, 0 section signs** (swept). 104 ASCII law citations (`SHIP-8b`, `PKG-9a`, `PKG-10`…). **`ship.md`: 14 `SHIP-§8`/`PKG-§10`/`PKG-§9f` citations** — correct, it is read by Claude, not by PowerShell 5.1. ✅ |
| 712's `[ValidateSet]`-on-reassignment landmine | ✅ **Avoided twice.** 712 assigns `$BootEvidence` **only** on the `$A6Ok` path and only to `$RouteSelected ∈ {Log,Pixel}` (`:1263`), with the reason commented in place (`:1257-1262`); the no-route case is carried by a `NONE` **fact**, ⛔ never a sentinel. 714 assigns **no parameter at all** — every new state lives in fresh unvalidated variables and the third verdict rides a distinct exception message (`SHIP-ADJUDICATE`). ✅ |
| 714's `Get-JsonProp` under `Set-StrictMode -Version Latest` | ✅ **A malformed record yields a clean STOP, not exit 1.** Every read of the hand-authored record goes through `Get-JsonProp` (`:438-444`), which tests `PSObject.Properties[$Name]` before dereferencing; invalid JSON is caught at `:1437` and produces `$AdjRec = $null` → `'the file is not valid JSON'` → `C3-VERDICT-FORM` STOP (exit 2). ⛔ No bare `$rec.foo` on the record path anywhere. ✅ **The state-file path is NOT equally hardened — see WARN-3.** |

---

## 2. FINDINGS

### ⛔ BLOCKERS

**[BLOCKER-1] `ship.ps1:1801` (blast radius `:1803`, `:1828-1830`, `:1863`, `:1906-1907`) — THE RUNNING GAME IS RESOLVED BY `$ProjectName`, WHICH UNDER SHIPPING MATCHES ONLY THE TITLE-LESS ROOT SHIM ⇒ `C3-BOOT-TITLE` STOPS EVERY SHIPPING RUN, *BEFORE THE STATE WRITE*, AFTER A 30-MINUTE COOK.**

```powershell
$gameProcs = @(Get-Process -Name $ProjectName -ErrorAction SilentlyContinue |
               Where-Object { $_.Path -and $_.Path.StartsWith($StageWin, [System.StringComparison]::OrdinalIgnoreCase) })
foreach ($gp in $gameProcs) { if ($gp.MainWindowTitle) { $windowTitle = $gp.MainWindowTitle } }
```

**This is not inferred — it is read straight off TASK-715 §6, which launched this exact stage through the shim and listed both processes:**

```
t+ 10s : PID  28688  GitClaudeUnrealTest                 WS     7 MB  Title=''
t+ 10s : PID   9104  GitClaudeUnrealTest-Win64-Shipping  WS 1,640 MB  Title='Siegebound'
```

`Get-Process -Name` does **not** apply an implicit wildcard, so `'GitClaudeUnrealTest'` matches PID 28688 (the shim) and **never** PID 9104 (the game). ⇒ `$windowTitle` stays `'(not measured)'` ⇒ `C3-BOOT-TITLE` (`:1863`, `$windowTitle -like '*Siegebound*'`) is **false on every Shipping run** ⇒ `throw 'SHIP-STOP'`.

**Why this is a BLOCKER and not a WARN — three compounding facts:**
1. ⚠️⚠️ **The throw happens at `:1863`, and the state write is at `:1929`.** ⇒ **nothing arms the resume**, and the next `/ship` re-cooks for ~30 minutes and stops in the same place. **This is byte-for-byte the failure `SHIP-§8`/TASK-714 was written to eliminate — a throw upstream of the state write — reintroduced one gate to the left.** The forever-stop has now moved four times: `A6` → `C3-BOOT-ARENA` → **`C3-BOOT-TITLE`**.
2. **It is invisible to every check anyone has run.** Under **Development** the staged binary *is* `<Project>.exe`, so shim and game share a name, both match, and the title resolves — the route that works is the only route anybody has exercised. **The standing, default route (Shipping + Pixel) is the broken one.**
3. **The kill loop at `:1828` inherits the same list**, so under Shipping **the real game process is never killed**. It survives the ship holding `Saved/` open; `A3-QUIET-MODULE`'s busy list (`:1047`) does not name `<Project>-Win64-Shipping` either, so the next run does not see it. Downstream that risks a silently-failed `Remove-Item $savedDir` (`:2051`, `SilentlyContinue`) and a `CreateEntryFromFile` throw on an open log during the zip.

⭐ **Note the symmetry, because it is the lesson:** TASK-714 correctly refused to *guess* the staged exe's **path** and derived it from the manifests. **The same file still guesses the running process's NAME from the same wrong assumption.** The repair is the identical move.

**Suggested fix (one statement, no new gate, no relaxation):** resolve by **path, not name** — it is name-free, catches shim *and* game, and fixes the kill loop for free:
```powershell
$gameProcs = @(Get-Process -ErrorAction SilentlyContinue |
               Where-Object { $_.Path -and $_.Path.StartsWith($StageWin, [System.StringComparison]::OrdinalIgnoreCase) })
```
(If a full enumeration is unwanted, derive the name from the *resolved* binary — `[IO.Path]::GetFileNameWithoutExtension($StageBinExe)` — and query it **plus** `$ProjectName`.) ⚠️ **Please also add `<Project>-Win64-*` to `A3-QUIET-MODULE`'s consideration or leave a comment saying why not** — a leftover Shipping game process is now a real, reachable state.

---

**[BLOCKER-2] `ship.ps1:1786-1788` vs `ship.md:113-124` — THE LIVE BOOT-VERIFY STILL LAUNCHES WITH THE MAP ARGUMENT `PKG-§9f` MEASURED AS NON-FUNCTIONAL IN SHIPPING, AND NEVER DRIVES THE CLICK RIG ⇒ THE ONLY CAPTURE IT CAN PRODUCE IS A MENU (OR THE DESKTOP), WHOSE ONLY CORRECT ADJUDICATION IS `FAIL`.**

```powershell
$bootArgs = ('{0} -windowed -ResX=1280 -ResY=720 -abslog="{1}"' -f $RECIPE_MAPS[1], $bootLog)
$bp = Start-Process -FilePath $StageExe -ArgumentList $bootArgs -PassThru
```

- `PKG-§9f`, measured as a controlled A/B: **a Shipping binary ignores the map argument and `-ExecCmds`** and boots to the menu.
- `ship.md:119` — **the authority on the procedure** — states the ruled route: *"launch the shipped exe with no args (as a double-click), then drive the game's OWN menu with the click rig (`t669_topclick.ps1`) to reach a match. Capture. Then adjudicate."*
- **`ship.ps1` does neither.** It launches *with* the refuted argument, spins the `while` loop the full `$BOOT_TIMEOUT_SEC = 300` (the positive markers can never appear: Shipping is log-silent per `PKG-§9a-1` **and** the map arg is ignored), then screenshots **the whole primary screen** at `:1819-1823` — whatever happens to be in front, with the game sitting on its main menu.
- ⇒ The adjudicator is handed a menu and, judging the printed bar honestly (`arena: … A MENU IS NOT A PASS`), **must record `FAIL`**. The ship ends. **Every time.**

⚠️ **The script's own dry-run text already says the live path is wrong** (`:1709-1719`: *"AND IN SHIPPING THE LAUNCH ARGUMENT ABOVE DOES NOT WORK"*). **A file that documents its own live instruction as refuted, and then executes it anyway, is this project's named stale-symbol trap with our own script as the false witness** (`PKG-§9a-3`'s class). And `SHIP-§0` is explicit: where the command file and the script diverge, **the divergence is a defect.**

**⚖️ Fence note, stated fairly:** TASK-714's scope addition (10) asked only that `PKG-§9f` be *recorded in `ship.md`'s procedure*, and **714 did exactly that, well** (`ship.md` §2b.1 is complete and correct). **This blocker is against the combined artifact, ⛔ not against 714's diligence** — but the artifact cannot pass, because a `/ship` that can only ever produce an adjudicable `FAIL` is not a release procedure, and the failure surfaces **only after a 30-minute cook on the first real use** — the precise cost `SHIP-§7`'s amendment exists to prevent.

**Suggested fix (either is acceptable; neither relaxes a gate):**
- **(a) Preferred —** under Shipping, launch `$StageExe` **with no args**, then invoke the existing click rig (`t669_topclick.ps1`) to reach a match, then capture. This is the route `PKG-§9f` already ruled and the pipeline already owns both halves.
- **(b) Minimum —** drop the refuted map argument under Shipping, and make the boot window an **explicit, announced, attended pause**: print *"drive the shipped menu to a match now; capturing in N seconds"* with the rig's command line, then capture. `PKG-§9f` already rules Shipping boot-verify **schedulable, not unattended**, so an announced pause is honest rather than a degradation.
⛔ **What is NOT acceptable:** leaving the launch line as-is, or lowering `PKG-§6a` so a menu passes.

---

**[BLOCKER-3] `ship.ps1:1431-1577` + `:1929-1947` (714 D2 × D11) — THE VERDICT RECORD IS NEVER RETIRED AND THE STATE'S `boot` IS NEVER ADVANCED PAST `ADJUDICATE`, SO THE FIRST INVOCATION OF *EVERY SUBSEQUENT SHIP* STOPS AT `C3-VERDICT-BINDING` UNTIL A HUMAN DELETES A FILE.**

Traced end to end:
1. Ship #1 completes. State (`ship-state.json`) still reads `boot = 'ADJUDICATE'`, `bootCapture = cap1`, `head = H1` — **it is never rewritten on the resume run, because the resume takes the `$reuse` branch (`:1733-1746`) and the state write lives in the cook branch.** The record (`ship-adjudication.json`, stable path `:1011`) still reads `PASS`, `head = H1`, `capturePath = cap1` — **D11 deliberately leaves it.**
2. Jonathan makes changes and says *"ship"*. HEAD is now `H2`. Route resolves to Pixel. The record exists ⇒ the seam block is entered (`:1431-1434`).
3. `C3-VERDICT-FORM` passes (the record is still well-formed). `C3-VERDICT-BINDING` then hits `:1540` — `$recHead (H1) -ne $HeadSha (H2)` ⇒ `$mis` non-empty ⇒ **STOP, exit 2, before PHASE B.**
4. The remedy names the file to delete, so the operator deletes it and re-invokes. **Every time, forever.**

⚠️ **This is not the stale-PASS hazard the law is aimed at — it is the *normal* path.** `/ship` exists because Jonathan said *"anytime we make any changes, I can say 'ship'"*; the scenario above **is** that sentence, and the command answers it with a STOP and a chore. `SHIP-§8d`'s *"any mismatch is a STOP"* governs a **resume**; step 2 above is not a resume, it is a fresh ship with no adjudication pending for it.

**Suggested fix (either closes it; both keep the anti-rubber-stamp property intact):**
- **(a) Retire on success —** after a successful adjudicated resume, **advance the state** (`boot = 'PASS'`, carrying `bootInstrument = $AdjInstrument`) and **archive** the record by moving it into that run's log dir as `ship-adjudication.<stamp>.consumed.json`. ⭐ **This keeps `SHIP-§8b(6)`'s retention duty** (the verdict and its observations survive, and D11's real point — *never silently destroy an operator's artifact* — is honoured, because moving-and-naming is not deleting).
- **(b) Scope the consultation —** treat the record as a resume candidate **only** when the state is pending on *this* run's identity (`$st0.head -eq $HeadSha -and $st0.config -eq $Configuration`); otherwise report it as `stale - not consulted` in the fact table and proceed to cook. ⛔ **Not a weakening:** the fresh cook writes a fresh pending capture, so the stale record can never bind to it and would still STOP on the next invocation.

⚖️ **D2 is not the defect and should not be reverted. D11 is.** With either repair, D2's speed bump fires **only** on a genuine stale-resume — which is exactly where the law wants it.

### ⚠️ WARNS

- **[WARN-1] `ship.ps1:1727-1731` — a confident stale line on the dry-run no-route path.** When A6 stops for *no usable route*, `$BootEvidence` is deliberately left at its requested value (712 D5/D6, correctly), so this new 714 text prints **"This run resolved evidence route Log."** while the fact table one screen up says **`Evidence route (resolved): NONE`**. 712 declared the older, milder version of this residue at `:1694`; 714's addition upgrades it from a hedged plan line to an **assertion about a resolution that did not happen** — the class `KBD-§2a` cond. 3 and `PKG-§9a-3` rate as *more* dangerous than a missing line. **Fix:** print `$(if ($A6Ok) { $BootEvidence } else { 'NONE - A6 found no usable route' })`. One expression, no PHASE-C edit.
- **[WARN-2] `ship.ps1:538-540` and `ship.md:180` — "A PASS resumes at PHASE D → E → F → G" omits the README precondition.** On the resume, `D0-README` (`:2022`) stops unless `packagedZIPofGame/README.md` already names *this* ship's zip. The contract printed at the adjudication stop is the operator's last instruction before they act, and it does not mention step 3. **Fix:** one line — *"Write the README first (`ship.md` §3) — `D0-README` will STOP without it."*
- **[WARN-3] `ship.ps1:1587-1604`, `:1643`, `:1739-1742` — the state file is read with bare property access under `Set-StrictMode -Version Latest`.** `$st.schema`, `$st.head`, `$st.boot`, `$st.suiteTotal`, `$st.bootInstrument` etc. **throw** on a missing property, surfacing a truncated/hand-edited/interrupted state file as `UNEXPECTED-ERROR` (exit 1) instead of a clean STOP — *a gate failing in the wrong voice*, the very class 714 §9 identified and closed for the **record**. D9's *"not a StrictMode hazard"* is true only for a well-formed schema-3 file. **Fix:** route state reads through the existing `Get-JsonProp`, or short-circuit the reuse checks on a schema mismatch. Low likelihood, cheap fix, and it removes the last asymmetry between the two JSON readers.
- **[WARN-4] `ship.ps1:806-821` — the prune's scope includes `Engine/` while the invariant excludes it.** `C2-STAGE-ONE-EXE` deliberately excludes `engine/` (correct — the `vc_redist` installers), but `C2-STAGE-PRUNE` walks the whole stage and would delete any non-manifest `.exe`/`.pdb` under `Engine/`. On 715's measured stage this is **inert** (all 19 non-manifest `Engine/` files are `.dll`/`.json`/`.html`/`.bat`/`.sh`/`.ini`), so it is a WARN, not a blocker — but a future engine-side non-manifest binary would be silently removed from the prereq set. **Fix:** either exclude `Engine/` from the prune for symmetry, or add one comment line stating that engine binaries are deliberately in scope because the manifests are the authority everywhere.
- **[WARN-5] `ship.ps1:1828-1830` — the boot-verify cleanup can leave the shipped game running** (BLOCKER-1's blast radius, called out separately because it survives even a partial fix of B1): the kill loop only kills what `$gameProcs` found, plus `$bp` (the shim). Fixing B1 by path resolution fixes this too; fixing it by name alone would not.

### 📌 NITS

- **[NIT-1] `:1312-1321`** — `$BUILD_RELEVANT_PREFIXES` is matched with `-like "*$pre*"` (contains), not a prefix test. Over-inclusive ⇒ **safe direction** (more re-cooks, never fewer), but the name and the behaviour disagree. Rename to `..._FRAGMENTS` or use `StartsWith`.
- **[NIT-2] `:764-782`** — `Get-StageGameExeRel` falls back to the convention name when the manifest yields **0 or >1** hits, silently. Add a fact row (`game binary resolved by: manifest | convention-fallback`) so an ambiguous manifest is visible rather than papered over. (Harmless today: `C2-STAGE-PRESENT` catches a wrong resolution.)
- **[NIT-3] `:853-856`** — `C2-STAGE-ONE-EXE` asserts exactly one **nested** game exe but only *reports* the shim count. `PKG-§10`'s invariant says *"plus the root shim"*, singular. Asserting `$shim.Count -le 1` would close it at zero cost.
- **[NIT-4] `:2121`** — `Move-Item -Force` silently overwrites a same-day zip of the same configuration. The new archive is read-back-verified first, so nothing unproven ships; but the destroyed predecessor is invisible to `D3-PRUNE`'s N=2 accounting. Worth one line in the summary.
- **[NIT-5] `$ADJ_HEDGES`** — `possibly` / `unclear` / `appears to be` are the likely honest-prose collisions. ⛔ **No change requested** (see the ruling in §1.2); the STOP text already names both correct exits and the cost is one edit, not a cook.
- **[NIT-6]** — the script owns phases A, B, C, D, F, G; **PHASE E is the caller's** (`ship.md` §3). Several strings print *"PHASE D → E → F → G"*. Accurate to the law, mildly misleading about the script. One clarifying clause in the header's RESUME block.
- **[NIT-7] 712 D3** — the duplicated pixel probe (A6 `:1208-1226` vs C3 `:1817-1825`) was the right call for an auditable diff, and I am **not** asking for the refactor now. Record it as a contained follow-up **after** this lane, so the two capture paths cannot drift apart unnoticed.

---

## 3. EVERY DECLARED DEVIATION, RULED (7 + 2 + 11 = 20)

**TASK-701 (7):**

| # | Deviation | Ruling |
|---|---|---|
| D1 | `-DryRun` runs PHASE A only (no compile/suite) | ✅ **ACCEPT — and the board settles it.** TASK-703's spec makes compiling in a dry run *a BLOCKER*. The narrower reading is the mandated one. |
| D2 | Resume-by-measurement | ✅ **ACCEPT.** Not a flag: no parameter exists, the guard list is 9 measurements deep, and any difference re-runs B+C in full. The `Source/Content/Config/Plugins/*.uproject` cut is **right** — the recipe itself is covered separately by `$RecipeHash` (701 Q1 answered). |
| D3 | ASCII-only, `PKG-5a` not `PKG-§5a` | ✅ **ACCEPT — verified 0 non-ASCII bytes, 0 section signs.** Correct for PS 5.1; a BOM would be more fragile. |
| D4 | Route 2 stops for caller adjudication | ✅ **ACCEPT — superseded and formalised by `SHIP-§8`/714.** 701 was right to refuse to claim it had read a PNG (701 Q2 answered: yes, that is the intended reading of `PKG-§9a`). |
| D5 | HUD proven by absence-of-failure **after** positive markers | ✅ **ACCEPT.** The positives prove the channel is alive first — the ordering is what makes the absence meaningful, and it is commented in place. Route-Log only. |
| D6 | Exactly ONE benign `unavailable` exception, full-phrase | ✅ **ACCEPT.** Cannot widen into a blanket excuse. |
| D7 | `D2-SIZE-SANITY` honestly scoped | ✅ **ACCEPT.** The comment states plainly that it does **not** catch `PKG-§5a`. A smoke alarm that names its own blind spot is the correct shape. |
| Q4 | README is git-ignored ⇒ not committable | ✅ **CONFIRMED.** `F1-COMMIT-PATHS` rejects ignored paths (`:2176-2177`) by design; `ship.md:205` keeps the record by reproducing the full text in the handoff (`PKG-§3`). |
| §6 | `PKG-§7a` is factually wrong (git root is the parent; `.gitignore:19` is live) | 📌 **NOT A QA FINDING AGAINST THE CODE — ROUTE TO THE MANAGER.** `A4-FENCE` implements the **property** and measures its way to the right answer either way (`:1063-1085`). ⭐ **The law's own argument for itself, demonstrated: a hardcoded path from the parenthetical would have resolved to a folder that does not exist.** Recommended repair unchanged: strike the two factual claims, keep the ruling verbatim, record route (ii). |

**TASK-712 (2 + 5 further declared):**

| # | Deviation | Ruling |
|---|---|---|
| D1 | Two extra `ship.md` lines beyond the gate table | ✅ **IN FENCE.** Both asserted the refuted route 1; the commit-cargo one was load-bearing (it invited a future ship to commit an inert ini key into the player's package — exactly what `PKG-§9a-3` strikes). |
| D2 | `'Boot evidence'` → `'Boot evidence (requested)'` | ✅ **ACCEPT** (`:933`). One word buys the disambiguation against a confident stale line. |
| D3 | Pixel probe duplicated, not refactored | ✅ **ACCEPT** — auditable diff over tidier code was the right trade on a task whose whole claim was *"only A6 moved"*. See NIT-7. |
| D4 | Probe writes no file (8×8 in-memory, disposed) | ✅ **ACCEPT** — preserves the *"a dry run writes nothing at all"* contract. Verified `:1216-1219`. |
| D5 | `$BootEvidence` reassigned, never a sentinel | ✅ **ACCEPT, and the reasoning is right.** Assigning `'NONE'` would throw *inside* the gate and surface as exit 1 — a gate failing in the wrong voice. Only ever assigned on the `$A6Ok` path (`:1263`). |
| D6 | Cosmetic residue on the dry-run no-route path | ⚠️ **ACCEPT AS DECLARED, but 714 made it worse — see WARN-1.** |
| D7 | A6 now evaluates where it previously self-skipped | ✅ **ACCEPT — strictly stronger.** No combination that previously stopped now passes; the only movement is off a phantom condition. |
| §6 | Route 2 makes `C3` a second forever-stop | ✅ **CORRECTLY REFUSED TO FIX OUT OF FENCE, AND CORRECTLY ESCALATED.** Resolved by `SHIP-§8` + TASK-714. ⭐ 712's option (a) was the right design and was adopted. |

**TASK-714 (11):**

| # | Deviation | Ruling |
|---|---|---|
| **D1** | The three-site `$StageBinExe`/`$pdb`/`$realBinary` derivation repair | ✅⭐ **IN FENCE, REQUIRED, CORRECT — the highest-value item in the diff.** Verified against 715's measured bytes. **The coupling claim is UPHELD and now binding: prune and derivation land or revert TOGETHER** (prune-without-derivation stops `C2-STAGE-PRESENT` **and** `D2-ZIP-READBACK` on every clean Shipping package). |
| **D2** | A binding mismatch is a STOP, not a silent re-cook | ✅ **THE TRADE IS RIGHT — I read `SHIP-§8d` the same way.** ⛔ **But its interaction with D11 makes it fire on the normal path — BLOCKER-3.** D2 stays; D11 must change. |
| D3 | Record consulted only under `Pixel`, never in `-DryRun` | ✅ **ACCEPT.** Keeps route 1 byte-identical and stops a dry run consuming an adjudication; still reported as a fact, so never invisible. |
| **D4** | Adjudication evaluated **before PHASE B** | ✅⭐ **ACCEPT — and promote it from deviation to doctrine.** It is what makes a FAIL cost seconds. Verified `:1418-1577` precedes `:1627`. |
| D5 | Rule 9 exceeds the law's literal binding list | ✅ **ACCEPT — strictly stronger, and it genuinely closes the named hole** (the per-run stamped capture path is what closes it). |
| D6 | The hedge list mechanises `SHIP-§8b(4)` | ✅ **ACCEPT, list unchanged.** PASS-only ⇒ can only make a PASS harder. Yes, it can reject an honest *"possibly"* — **correctly**, per `SHIP-§8b(4)`; cost is one edit, ⛔ never a cook. See NIT-5. |
| D7 | Pre-filter records `CHECK`, not `PASS` | ✅ **ACCEPT — exactly right.** A `PASS` row for a mechanical sub-check invites the misreading `SHIP-§8c` exists to prevent. The single stop mechanism (delegating to `Assert-Gate`) is the correct shape. |
| D8 | Exit code 4 | ✅ **ACCEPT.** Header states *"EXIT 4 IS NOT SUCCESS"*; truth surface remains authoritative. |
| D9 | `$STATE_SCHEMA` 3 → 4 | ✅ **ACCEPT** (additive; a schema-3 file simply re-runs). ⚠️ **The "not a StrictMode hazard" claim is narrower than stated — WARN-3.** |
| D10 | `ship.md` §2b is a new section, not a row | ✅ **ACCEPT** — a procedure for a step that did not exist cannot be a table row. §2b is the best-written part of the command file. |
| **D11** | The record is **not** deleted after a successful ship | ⛔ **REJECTED AS IMPLEMENTED — BLOCKER-3.** The instinct is right (⛔ never silently destroy an operator's artifact) but *retain* ≠ *leave armed*. **Archive it and advance the state**; that satisfies `SHIP-§8b(6)` retention *and* removes the mandatory manual deletion before every future ship. |

**Also confirmed for the record:** 714 §5's AST inventory reproduces — every pre-existing gate keeps its `-Ok` expression, the param block is 9 parameters unchanged, the Δ is exactly **one removal** (`Assert-Gate 'C3-BOOT-ARENA' -Ok $false`) **and seven additions** (`C3-CAPTURE`, `C3-VERDICT-FORM`, `C3-VERDICT-BINDING`, `C3-VERDICT`, `C3-BOOT-ARENA` via `Note-Adjudicate`, `C2-STAGE-MANIFESTS`, `C2-STAGE-PRUNE`, `C2-STAGE-ONE-EXE`). ✅ With no git baseline to diff, the AST inventory is the right instrument and it holds.

---

## 4. THE ORIGINAL TASK-702 CRITERIA (1–13)

| # | Criterion | Result |
|---|---|---|
| 1 | ⭐ `-COOKDIR` list present, complete, commented with WHY | ✅ **11/11** (`:230-242`), 60 lines of *why* (`:182-229`) naming the measured failure, the `SiegePlayerController.cpp:194-215` architectural citation, the measured effect, what is deliberately excluded, and *"A hand-run cook that omits -COOKDIR is a defect even if UAT says BUILD SUCCESSFUL."* Reachable into `-AdditionalCookerOptions` (`:1342-1361`). Re-verified against the tree every run by `A7-COOKDIRS`. |
| 2 | Every gate a genuine STOP; previous zip intact | ✅ `Assert-Gate` throws; write-new-then-prune (`.partial` → readback → `Move-Item` → prune) at `:2045`/`:2100`/`:2121`/`:2129`. ⛔ No warn-and-continue path. |
| 3 | ⛔ No skip/force flag in either file | ✅ 0 |
| 4 | `PKG-§7a` fence measured per run | ✅ `:1063-1085`, `git rev-parse` + `git check-ignore -v` against whatever `-StagingDir` resolved to. ⛔ No hardcoded path. |
| 5 | Compile + suite precede the cook | ✅ PHASE B `:1627` before PHASE C `:1681` |
| 6 | Verdicts log-parsed, never exit codes | ✅ `Result: Succeeded` / `BUILD SUCCESSFUL`; `$LASTEXITCODE` confined to git queries |
| 7 | Boot-verify reaches a real arena | ⛔ **FAILS — BLOCKER-1 + BLOCKER-2.** The *bar* is intact and correctly stated; the **instrument cannot reach it** under the standing route. |
| 8 | ZIP64 writer · read-back · prune only after verify · never another config | ✅ `System.IO.Compression.ZipArchive`; `Compress-Archive` banned in comment; D3 anchored to `^Siegebound-Win64-<Config>-\d{4}-\d{2}-\d{2}\.zip$`; other configs printed as `PROTECTED` |
| 9 | Git sweep | ✅ all zero (see §1.8) |
| 10 | `-DryRun` genuinely skips cook/zip/commit | ✅ by construction — every write is `if (-not $DryRun)`; the only directory creation (`:1012`) is guarded; the hygiene prune is unreachable in a dry run (`:1691-1698`, `:1971`) |
| 11 | `SHIP-§3` doc table reproduced **including the NOs** | ✅ `ship.md:232-238` — GDD **NO**, `setupdirections.md` **NARROW YES**, TASKBOARD/CONVENTIONS **NO**, `CLAUDE.md` once. ⛔ Wording not softened |
| 12 | `CLAUDE.md` NOT modified; the line lives in the handoff | ✅ `CLAUDE.md` contains no ship routing line and is not in the diff; the prepared line is 701 §7 |
| 13 | `SHIP-§6` report names what it does NOT prove | ✅ `:900-906` — no input-injection lane ⇒ no gameplay-feel claim; not pushed; *"this script cannot read pixels"* |

---

## 5. ⛔ NOT FOR THE PROGRAMMER — WHAT THIS FAIL DOES **NOT** ASK FOR

Stated explicitly so the repair loop does not over-correct:
- ⛔ **Do not revert the collateral repair (714 D1).** It is required and correct. Reverting it while keeping the prune is the one combination worse than either.
- ⛔ **Do not revert D2** (mismatch ⇒ STOP). Fix **D11**.
- ⛔ **Do not trim `$ADJ_HEDGES`.**
- ⛔ **Do not add a flag, do not relax a gate, do not touch `PKG-§6a`, and do not lower the arena bar to make BLOCKER-2 go away.** *"The arena is hard to reach in Shipping"* is an argument about the instrument, never the bar.
- ⛔ **Do not edit `.Target.cs` or the ini to chase a log line** (`PKG-§9a-1`).

---

## 6. ⭐ THE CONTRACT FOR TASK-703 (the `-DryRun` acceptance) — WHAT IT MUST SHOW, AND WHAT IT **CANNOT** REACH

⚠️ **TASK-703 stays blocked until a re-QA passes** (BLOCKER-1/2/3 land first). When it runs, this is its contract.

### 6.1 What the dry run MUST show (all of it quoted verbatim into the handoff)
1. **`SHIP RESULT: DRYRUN-OK`** as the **last stdout line** — or `DRYRUN-WOULD-STOP` with the named gate list. ⛔ Read the line, never the exit code.
2. **The exact UAT command line**, and the **11 `-COOKDIR` entries expanded one per line** (`:1687-1688`). ⭐ **Read them character by character against `PKG-§5a`:** `Data · UI · Blueprints · Input · Characters · Meshes · Materials · Textures · VFX · Audio · LevelPrototyping` — **11, none missing, none truncated.** ⛔ A short list is a BLOCKER, not a note.
3. **`A6-EVIDENCE-ROUTE` = PASS with route `PIXEL`, AUTO-SELECTED**, plus the `*** EVIDENCE ROUTE AUTO-SELECTED ***` banner and the three fact rows (`Boot evidence (requested)`, `Engine class`, `Evidence route (resolved)`). ⚠️ **701's handoff §3 predicts an `A6` `WOULD-STOP`. That prediction is STALE — TASK-712 repaired A6. If 703 sees an A6 stop, that is a regression, not the documented outcome.**
4. **`A4-FENCE` = PASS via route (ii)**, naming `.gitignore:19` — proving the fence is *measured*, not hardcoded (and confirming 701 §6's finding).
5. **The `ADJUDICATION CONTRACT (SHIP-8) - PRINTED BECAUSE A DRY RUN CAN NEVER REACH IT`** block, in full: the four `PKG-§6a` criteria verbatim, the six verdict rules, the record path, the JSON schema, and **the resume invocation composed from this run's own resolved parameters**. ⛔ **Quote it. Do not summarise it** — its presence is the acceptance of `SHIP-§7`'s amendment.
6. **The `DRY RUN SCOPE` block** (`:909-918`) — including *"an unconditional stop at C3 once sat in this script behind a green dry run."*
7. **The stage-hygiene plan** (`:1699-1702`) with the manifest-resolved game binary printed — expect `GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest-Win64-Shipping.exe`. ⭐ **If it prints `…/GitClaudeUnrealTest.exe`, the collateral repair regressed — BLOCKER, route back.**
8. **The `PKG-§9f` warning block** beside the boot-verify plan (`:1709-1719`).
9. **Proof it wrote NOTHING:** no new `.ship/<stamp>` directory, no zip, no `.partial`, nothing deleted, no `git add`. ⛔ **If it compiled, cooked, zipped or committed, that is a BLOCKER — route back to the programmer (counts as a QA loop).**
10. **At least one STOP exercised for real** (TASK-703 spec (3)) — the `A4-FENCE` failure path is the cheapest: point `-StagingDir` at a path inside the work tree that no `.gitignore` rule covers and confirm it **halts** rather than warns. ⚠️ Remember `-DryRun` records every gate and continues by design; the proof is the `[STOP] A4-FENCE` row **and** `SHIP RESULT: DRYRUN-WOULD-STOP` naming it.
11. **Useful second variant:** `-DryRun -BootEvidence Log` on this machine ⇒ **`A6` STOP** citing `PKG-9a-1` (an explicit demand for an instrument that cannot exist). ⭐ That single run demonstrates the auto-select **and** the explicit-demand STOP are different code paths. ⚠️ **Expect WARN-1's stale line on that run if it has not been fixed — note it, do not treat it as a new defect.**

### 6.2 ⛔ WHAT THE DRY RUN CANNOT REACH — **STATE THIS IN THE HANDOFF, IN THESE TERMS**
> **A `-DryRun` pass is NOT evidence about phases C–F.** It never compiles, never runs the suite, never cooks, never boots, never captures, never adjudicates, never zips, never prunes and never commits.

**Specifically unreachable, and therefore ⛔ NOT accepted by TASK-703:**
- `B1-COMPILE`, `B2-SUITE` — **never executed** (701 D1, ratified: compiling in a dry run would itself be a blocker).
- `C1-COOK`, `C2-UAT-LOG`, `C2-STAGE-PRESENT` — the recipe is **printed, never run**. ⭐ **The `-COOKDIR` read IS the acceptance** (`SHIP-§7`): a second 30-minute cook to validate the wrapper of the first buys nothing and burns a serialized gate.
- `C2-STAGE-MANIFESTS` / `-PRUNE` / `-ONE-EXE` — **not evaluated; nothing is deleted.** The `PKG-§10` gate remains **unexercised by machine** until the first live cook. *(Its logic has been reviewed line-by-line here and matches 715's hand-run measurements — that is a code review, ⛔ not a run.)*
- `C3-BOOT-TITLE`, `C3-CAPTURE`, `C3-BOOT-ARENA`, `C3-VERDICT-FORM/-BINDING/-VERDICT` — **the entire adjudication seam and the resume path are unexercised.** ⛔ **The 15-row truth table is reviewed, not run.**
- `D0`–`D3`, `F1`, `F2` — **no zip, no read-back, no retention, no commit.**
- ⛔⛔ **AND, MOST IMPORTANTLY: all three of this report's BLOCKERS live in phases the dry run cannot reach.** ⚠️ **A green `DRYRUN-OK` would have been reported over every one of them.** That is not a criticism of the dry run — **it is `SHIP-§7`'s amendment being right**, restated with three fresh examples. ⇒ **TASK-703's handoff must say, in its own words: *"this dry run proves PHASE A and the printed plan. It proves nothing about the cook, the boot-verify, the adjudication, the zip or the commit, and the first real `/ship` is where those are first exercised."***

### 6.3 Commit cargo (unchanged from the board)
`Tools/Packaging/ship.ps1` + `.claude/commands/ship.md` + pipeline files (board / CONVENTIONS / handoffs / qa) — **explicit paths only.** ⛔ `git add -A` banned · ⛔ `CLAUDE.md` **not** in the cargo (row S8 — Jonathan's to land; the exact line is in 701 §7) · ⛔ **never push.**

---

## 7. NOTES FOR THE MANAGER (not blockers, not for the programmer)

1. **`PKG-§7a` carries two false factual claims** (701 §6, re-confirmed here): the git root is `…\GitClaudeUnrealTesting`, so `packagedZIPofGame/` is **inside** the work tree and is fenced by a **live `.gitignore:19` rule** — route **(ii)**, not (i). ⛔ The **ruling** is correct and untouched; only the parenthetical measurements are wrong.
2. **BLOCKER-2 likely wants its own task** (`SHIP-`/`PKG-§9f` lane): *"drive the click rig from the boot-verify, or make the boot window an announced attended pause"*. It is a contained change to ~10 lines of PHASE C, but it is a route decision, and route decisions in this lane have been the manager's.
3. **The forever-stop has now moved four times** (`A6` → `C3-BOOT-ARENA` → `C3-BOOT-TITLE` → the D2×D11 binding stop). ⭐ **The pattern is worth a law line:** *every gate on the standing route must be exercised against the STANDING configuration's measured process/file names, ⛔ never against the Development names that happen to match the project name.* Three of this lane's four forever-stops share exactly that root cause.
4. **TASK-703 stays blocked** on a re-QA. None of the three blockers is reachable by its dry run, so 703 cannot substitute for the fix.
