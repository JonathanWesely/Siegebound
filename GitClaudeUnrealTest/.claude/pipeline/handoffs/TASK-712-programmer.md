# TASK-712 — gameplay-programmer handoff — repair the `A6-EVIDENCE-ROUTE` gate

2026-08-30 · law **`PKG-§9a-1`** + **`PKG-§9a-2`** (authorizing ruling, cited not restated) · `SHIP-§1` preserved verbatim
**File-only: ⛔ no compile, no cook, ⛔ no dry-run execution of the script (parse-check only, as TASK-701 did), no editor, no MCP, no Git, no board edit beyond this task's status row.**
Inputs: `handoffs/TASK-701-programmer.md` · CONVENTIONS `PKG-§9a`..`§9a-3` · TASKBOARD TASK-712 spec.

---

## 0. WHAT CHANGED — two files, six hunks, nothing else

| File | Hunks | What |
|---|---|---|
| `Tools/Packaging/ship.ps1` | 3 (`@@ -54,10 +54,16 @@`, `@@ -454,7 +460,7 @@`, `@@ -611,24 +617,187 @@`) | the `A6-EVIDENCE-ROUTE` gate block · its `-BootEvidence` header doc · one fact **label** |
| `.claude/commands/ship.md` | 3 (`@@ -70,7 +70,7 @@`, `@@ -135,7 +135,7 @@`, `@@ -157,6 +157,6 @@`) | the A6 gate-table row · the commit-cargo route-1 example · the hazard-crib "logging may be compiled out" line |

`ship.ps1`: **1251 → 1310 lines**, 18 lines removed / 178 added — **all of it inside PHASE A's A6 block plus two documentation lines.**

**Mechanical checks run (both file-only, neither executes the script):**
- `[System.Management.Automation.Language.Parser]::ParseFile` → **PARSE: CLEAN, 0 errors**
- byte audit → **0 non-ASCII bytes** (701's D3 discipline held; citations written `PKG-9a-1`, ⛔ never `PKG-§9a-1`, inside the script)

---

## 1. THE GATE — BEFORE / AFTER

### BEFORE (as TASK-701 built it, `ship.ps1:614-630`)

```powershell
$hasShipLogging = [bool](Select-String ... 'bUseLoggingInShipping\s*=\s*True' ...)
if ($Configuration -eq 'Shipping' -and $BootEvidence -eq 'Log') {
    Assert-Gate -Id 'A6-EVIDENCE-ROUTE' -Ok $hasShipLogging `
        -Evidence 'Shipping + -BootEvidence Log requires bUseLoggingInShipping=True in Config/DefaultEngine.ini; it is absent' ...
} else {
    Note-Skipped 'A6-EVIDENCE-ROUTE' ("not applicable: config={0} evidence={1}" ...)
}
```

**Two defects, both bought by measurement, ⛔ neither a coding error:**
1. **The condition could never be true.** `bUseLoggingInShipping` is not an ini setting (`PKG-§9a-1`), so `$hasShipLogging` is permanently `$false` ⇒ **every default `/ship` STOPPED at A6, forever.**
2. **The `else` branch `Note-Skipped`s the gate** — i.e. in every non-(Shipping+Log) combination A6 recorded `PLAN` and **named no route at all.** That is the silent-no-route pass `PKG-§9a-2` clause 3 forbids.

### AFTER (`ship.ps1:617-799`)

The gate now answers **its actual question — *"does this run have SOME valid way to prove the boot?"*** — in four steps:

1. **Read the inert ini key, for one reason only: to call it inert.** If someone re-adds `bUseLoggingInShipping=True`, the fact line now reads *"PRESENT and INERT — a UBT TargetRules property with no ini binding (PKG-9a-1). It does NOT enable Shipping logging. Strike it (PKG-9a-3)."* ⛔ **It is never an input to the route decision.** (This is the `PKG-§9a-3` stale-symbol trap, guarded at the place the next reader will actually look. It pairs with TASK-713, which strikes the key — but the guard holds whether or not 713 lands, and whether or not someone re-adds it later.)
2. **Measure the machine class** — `Test-Path "$EngineRoot\Engine\Build\InstalledBuild.txt"`. A **property, measured every run**, the same discipline `A4-FENCE` uses; it survives someone shipping from a source-built engine. **Measured on this machine today: the marker EXISTS ⇒ INSTALLED (Launcher) ⇒ Shipping is log-silent.**
3. **Resolve the route**, auto-selecting `Pixel` and **announcing it** (see §4).
4. **Probe the selected instrument before the cook**, then **always `Assert-Gate`** — ⛔ never `Note-Skipped`, in any configuration.

**`$LogRouteUsable = ($Configuration -ne 'Shipping') -or (-not $EngineIsInstalled)`** — the Log route exists when the build actually emits a log: any Development build, or Shipping on a source-built engine. **Shipping + installed engine is the one combination where it cannot exist.**

---

## 2. THE ROUTE-SELECTION TRUTH TABLE (machine class × requested route → outcome)

`-BootEvidence` is now a **request, not a demand** — except when it is passed explicitly, which is exactly what makes it a demand. The script distinguishes the two with `$PSBoundParameters.ContainsKey('BootEvidence')` (**verified in an isolated snippet: `False` when defaulted, `True` when `-BootEvidence Log` is typed**).

| # | Config | Engine class | `-BootEvidence` | Pixel instrument | Resolved route | A6 outcome |
|---|---|---|---|---|---|---|
| 1 | **Shipping** | **INSTALLED** | *(not passed → `Log` by default)* | available | **`Pixel`** | ✅ **PASS — AUTO-SELECTED + ANNOUNCED** ⭐ *this is the row this machine hits* |
| 2 | **Shipping** | **INSTALLED** | *(not passed)* | **unavailable** | — | ⛔ **STOP** — no usable route |
| 3 | **Shipping** | **INSTALLED** | **`Log` (explicit)** | *(not probed)* | — | ⛔ **STOP**, citing `PKG-9a-1`: you demanded an instrument that cannot exist |
| 4 | **Shipping** | **INSTALLED** | `Pixel` (explicit) | available | `Pixel` | ✅ PASS — as requested |
| 5 | **Shipping** | **INSTALLED** | `Pixel` (explicit) | **unavailable** | — | ⛔ **STOP** — no usable route |
| 6 | **Shipping** | source-built | `Log` (default **or** explicit) | *(not probed)* | `Log` | ✅ PASS — `PKG-§9a-1`'s selectable case |
| 7 | **Shipping** | source-built | `Pixel` | available / unavailable | `Pixel` / — | ✅ PASS / ⛔ STOP |
| 8 | **Development** | either | `Log` (default **or** explicit) | *(not probed)* | `Log` | ✅ PASS — Development compiles logging in |
| 9 | **Development** | either | `Pixel` | available / unavailable | `Pixel` / — | ✅ PASS / ⛔ STOP |

**Reading of the rows that matter:**
- **Row 1 is the repair.** Default `/ship` on this machine now proceeds — on the honest route — instead of stopping forever.
- **Row 3 is `PKG-§9a-2` clause 2 case 1.** An explicit `-BootEvidence Log` is a demand, and the answer is a STOP with the reason, ⛔ never a silent downgrade to Pixel.
- **Rows 2 / 5 / 7 / 9 are clause 2 case 2** — *a ship that cannot verify its own boot must not ship.*
- **⛔ There is no row that passes with no route.** Every PASS names its instrument in the gate evidence, in the fact table, and (row 1) in a banner.
- **Row 6 is stated honestly**, not overclaimed: on a source engine the Log route is *selectable*, which is not the same as *armed*. The gate says so and points at what actually proves the channel is alive — **C3's POSITIVE markers** (arena `LoadMap` + a non-zero deck + hero spawned), per 701's D5. ⛔ **I did not extend A6 into reading `.Target.cs`** — that is unmeasured scope, and C3 already catches a silent log by failing its positive markers.

---

## 3. ⛔ PROOF THAT NO OTHER GATE MOVED

**(a) Gate-ID inventory, mechanically diffed** — every `Assert-Gate -Id '…'` / `Note-Skipped '…'` string extracted from the before-copy and the after-file, sorted, diffed. **The entire difference is one line:**

```
30d29
< Note-Skipped 'A6-EVIDENCE-ROUTE'
```

⇒ **The only gate whose call sites changed is A6**, and the change is the *removal of its ability to be skipped*. Every other gate ID — `A1-*`, `A2`, `A3`, `A4`, `A5`, `A7-*`, `B1`, `B2`, `C1`, `C2-*`, `C3-*`, `C4`, `D0`, `D1`, `D2-*`, `D3`, `F1-*`, `F2-*` — is present in identical form and count.

**(b) Diff hunks confined** — `ship.ps1` has exactly 3 hunks, all inside PHASE A / the header: the A6 block, A6's parameter documentation, one fact label. **⛔ No hunk touches PHASE B, C, D, E, F or G.** `C3-BOOT-ARENA`, `C3-BOOT-TITLE` and the `PKG-§6a` arena logic are **byte-identical**.

**(c) The parameter block is byte-identical** (`diff` of the `[CmdletBinding()] param(…)` block → no output). ⛔ **No parameter added, removed, renamed or re-defaulted.** ⛔ **No skip/force flag:** `[switch] $SkipTests|SkipCook|SkipSuite|NoVerify|Force|IgnoreGates|AcceptWarnings|FastShip|YesReally` → **0 declarations.**

**(d) Git-posture sweep, before → after, all unchanged:**

| Pattern | before | after |
|---|---|---|
| `git add -A` / `git add .` | 0 | **0** |
| `git push` (as an invocation) | 0 | **0** |
| bare `push` (prohibition prose only) | 4 | **4** |
| `--amend` | 2 | **2** |
| `reset --hard` / `filter-branch` | 0 | **0** |
| `$LASTEXITCODE` | 3 | **3** |

> ⚠️ **Correction for QA criterion 9, so nobody trips on it:** 701's handoff says *"the 3 `push` hits are prohibition prose"*; the actual count is **4** (`ship.ps1:339, 449, 1383, 1388`), all four prose/evidence strings, **`git push` invocations = 0**. **Unchanged by me** — a transcription slip in 701's note, ⛔ not a code change.

**(e) `PKG-§6a` is untouched, by construction and by intent.** The word the ruling uses is *instrument*, and that is the only thing that moved. The boot still has to reach `L_Arena` with a real deck built from real card rows and a spawned hero. The A6 comment block says so in the file, twice.

---

## 4. ⭐ THE REPORT WORDING THAT ANNOUNCES THE AUTO-SELECTED ROUTE

**A silent switch is its own trap**, so the auto-selection is announced in **three** places that all survive into the final summary block.

**(1) The inline banner** (printed at the gate, verbatim):

```
  *** EVIDENCE ROUTE AUTO-SELECTED: PIXEL (route 2, PKG-9a-1) ***
      This engine is an INSTALLED (Launcher) build, so a Shipping cook here is
      LOG-SILENT PERMANENTLY.  The Log route does not exist on this machine
      class - it was not "left unset", it CANNOT be set (PKG-9a-1).
      Route 2 is therefore the STANDING route: not a fallback, not a degradation.
      PKG-6a IS UNCHANGED - the boot must still reach a REAL ARENA with a REAL
      DECK and a spawned hero.  ONLY THE INSTRUMENT CHANGED.
      PLAN FOR THIS: route 2 ends in CALLER ADJUDICATION at C3-BOOT-ARENA - a
      human or agent LOOKS at the capture.  This script never reads pixels and
      will never claim it did.
```

**(2) The gate row** — appears in the `SHIP SUMMARY` GATES table:

```
PASS   A6-EVIDENCE-ROUTE   route PIXEL (AUTO-SELECTED by PKG-9a-1: installed engine, so the
                           Log route does not exist here; requested was Log-by-default);
                           instrument: screen capture OK, primary screen <W>x<H>; PKG-6a arena
                           bar UNCHANGED - only the instrument changed; ends in CALLER
                           ADJUDICATION at C3-BOOT-ARENA, it never self-passes
```

**(3) Three fact rows** — appear in the `SHIP SUMMARY` FACTS table:

```
  Boot evidence (requested)          Log
  bUseLoggingInShipping              absent - correct: it is not an ini key at all (PKG-9a-1)
  Engine class                       INSTALLED (Launcher): C:\Program Files\Epic Games\UE_5.8\
                                     Engine\Build\InstalledBuild.txt exists ==> Shipping is
                                     LOG-SILENT (PKG-9a-1)
  Evidence route (resolved)          Pixel - AUTO-SELECTED (PKG-9a-1)
```

And on the no-route STOP path: `Evidence route (resolved)  NONE - no usable route; see gate A6-EVIDENCE-ROUTE`.

---

## 5. DECLARED DEVIATIONS AND INTERPRETATIONS (`SC-§15`)

**D1 — I edited two `ship.md` lines beyond the gate table, both asserting the refuted fact.** The spec says *"update the `ship.md` procedure text + the gate table."* Beyond the A6 row I changed exactly two lines, because each **stated route 1 as live** and is therefore the named stale-symbol trap in prose:
- **line 138 (commit cargo):** *"any config the ship itself changed (e.g. a `PKG-§9a` route-1 ini line)"* → now *"(⛔ never an evidence-route config — `PKG-§9a-1`: route 1 does not exist on this machine class and no ini key can create it)"*. **This one is load-bearing:** as written it invited a future ship to commit an inert ini key into the player's package — the exact thing `PKG-§9a-3` strikes.
- **line 160 (hazard crib):** *"logging **may** be compiled out"* → *"logging **is** compiled out and cannot be turned back on here (`PKG-§9a-1` …) ⇒ rendered pixels are the standing evidence route, and `A6` selects them for you."* A hedge that is now known to be a certainty reads as an open option to the next author.
⛔ **Neither is a gate row.** If QA reads the fence more narrowly, both revert cleanly and independently.

**D2 — I relabelled one fact: `'Boot evidence'` → `'Boot evidence (requested)'`** (`ship.ps1:460`, one string, zero logic). `Add-Fact` **appends**, so without this the summary would print `Boot evidence: Log` beside `Evidence route (resolved): Pixel` — **a confident stale line**, which this project's own law (`KBD-§2a` cond. 3, `PKG-§9a-3`) rates as *more* dangerous than a missing one. One word buys the disambiguation.

**D3 — the pixel probe is DUPLICATED from C3 rather than refactored into a shared helper.** A shared `Test-PixelInstrument` function would have been tidier, but it would have put a hunk in the helper region **and** rewritten C3's capture block — i.e. it would have destroyed the clean *"only A6 moved"* proof in §3, on a task whose whole point is that only A6 may move. **I chose the auditable diff over the tidier code, deliberately**, and said so in the file at the duplication site. ⚠️ **If QA prefers the refactor, it is a contained follow-up** — but it should be its own task, after 702.

**D4 — the probe writes NO file, so 701's `-DryRun` "writes nothing at all" contract is preserved.** It allocates an 8×8 **in-memory** bitmap, does one `CopyFromScreen`, and disposes. It does **not** touch `$RunLogDir` (which does not exist yet in PHASE A, and which a dry run never creates). This was the specific constraint that decided the probe's shape.

**D5 — `$BootEvidence` is REASSIGNED to carry the resolved route, and is never assigned outside its `ValidateSet`.** PHASE C reads `$BootEvidence` at lines 1013 / 1070 / 1091, so assigning the decision there is what makes the auto-selection actually take effect **without touching a single PHASE C line**. ⚠️ **A real trap was found and avoided here, verified in an isolated snippet:** PowerShell **re-enforces a parameter's `[ValidateSet]` on every later assignment` —
```
assign in-set OK -> Pixel
assign out-of-set: BLOCKED -> The variable cannot be validated because the value NONE is not a valid value for the R variable.
```
⇒ assigning a sentinel like `'NONE'` would have thrown **inside the gate**, surfacing as the outer handler's *unexpected error* (exit 1) instead of a clean `STOP` — a gate that fails in the wrong voice. **Therefore `$BootEvidence` is only ever assigned on the `$A6Ok` path**, and the no-route case is reported through the `Evidence route (resolved) = NONE` fact instead. This is commented in place so the next author does not "simplify" it back.

**D6 — one cosmetic residue on the DRY-RUN no-route path, declared rather than hidden.** Because of D5, when A6 would stop, `$BootEvidence` keeps its requested value, so the dry run's C3 plan line still reads `instrument would be: Log`. **A dry run does not stop at the first failure by design** (701's stated behaviour), so that line is printed after A6 already reported `[STOP]`, the resolved-route fact already says `NONE`, and the summary ends `DRYRUN-WOULD-STOP … A6-EVIDENCE-ROUTE`. ⛔ **Not a silent pass in any run**, and ⛔ not reachable at all in a live run (A6 throws first). Fixing the cosmetic would require editing a PHASE C line; **I judged the clean diff worth more.**

**D7 — A6 now also evaluates in configurations where it previously self-skipped, and this can stop a run earlier than before. That is strictly stronger, never a relaxation.** Concretely: `Development + -BootEvidence Pixel` with no usable screen previously ran the compile, the suite and a **30-minute cook** and only then failed at `C3-BOOT-ARENA` with `CAPTURE FAILED`. It now stops in PHASE A for **the same real reason**. ⛔ **No combination that previously stopped now passes** — the only direction of movement is row 1 of the truth table, where the old gate stopped on a **phantom** condition.

---

## 6. ⚠️⚠️ FINDING FOR THE MANAGER — ROUTE 2 BECOMING THE STANDING ROUTE MAKES `C3` A SECOND FOREVER-STOP. ⛔ I DID NOT TOUCH IT.

**This is the most important thing in this handoff, and it is outside my fence.**

`PKG-§9a-2` fixes A6 so `/ship` no longer stops forever *at A6*. But route 2 is now the **standing** route, and 701's **D4** built the pixel path as *always* `Assert-Gate -Ok $false` — `C3-BOOT-ARENA … PIXEL ADJUDICATION REQUIRED` — because the script cannot read pixels and correctly refuses to claim it did. Measured consequences, read out of the code:

1. **A live run under route 2 always throws at `C3`** (`ship.ps1:1125`), so it never reaches the state write at `ship.ps1:1129-1143`.
2. That state file is the **only** thing that arms resume-by-measurement (`$st.boot -ne 'PASS'` ⇒ re-run). ⇒ **route 2 can never reuse PHASE B+C.**
3. The caller adjudicates the capture and re-runs — and there is **no mechanism for the verdict to re-enter the run** (correctly, since a `-PixelAdjudicated` flag would be a skip-flag-class parameter, ⛔ banned by `SHIP-§1`).

⇒ **As the lane currently stands, a Shipping `/ship` on this machine re-cooks for ~30 minutes and stops at `C3` again, indefinitely.** The forever-stop moved from A6 to C3; it did not disappear.

⛔ **I did not fix this**, because every available fix either relaxes a gate, adds a banned flag, or edits `C3` — all three explicitly outside TASK-712's fence, and the second is a Jonathan-level call. **It needs a manager ruling and its own task.** For that ruling, the honest option space as I read it:
- **(a)** the adjudication verdict becomes an **input the script can measure** rather than a flag it is told (e.g. the caller records the verdict into the run-log dir and the *next* invocation reads it, gated on the same byte-identical-input proof `D2` already uses — a *measurement*, not a skip);
- **(b)** `C3` under route 2 gates on what the script **can** measure (process liveness + window title + capture-file existence + non-blank frame) and the arena claim is carried by the **operator's** adjudication recorded in the README/handoff;
- **(c)** the ship is ruled a **two-agent procedure** by design and the re-cook cost is accepted.
⚖️ **(a) preserves `SHIP-§1` best on my reading, and it reuses D2's existing machinery** — but ⛔ **it is not my call**, and I have implemented none of it. **TASK-703's dry run will not surface this** (a dry run never reaches C3), so it would otherwise be discovered live, mid-ship.

---

## 7. WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐ **The truth table in §2, row by row against `ship.ps1:698-799`** — especially **row 3** (explicit `Log` ⇒ STOP, ⛔ not a silent downgrade) and **rows 2/5** (instrument unavailable ⇒ STOP). Confirm **no row passes with no route**.
2. ⭐ **The gate-ID inventory diff in §3(a)** — reproduce it; the whole *"A6 only"* claim rests on it.
3. **D5's `ValidateSet` reasoning** — confirm you agree `$BootEvidence` must never be assigned a sentinel, and that reassigning it (rather than adding a new variable read by PHASE C) is the right way to keep PHASE C untouched.
4. **D1** — rule on whether the two non-gate `ship.md` lines were inside the fence. **Both revert independently** if not.
5. **D3** — the deliberate probe duplication: auditable diff vs tidier code.
6. **§6** — ⚠️ **not a QA finding against this task; route it to the manager.** Please confirm it lands in front of a ruling before TASK-703/704 run a live ship.
7. **ASCII discipline** — grep the script for `PKG-9a-1`, ⛔ **not** `PKG-§9a-1`. Verified **0 non-ASCII bytes**.

## 8. STATE

⛔ **Nothing executed**: no compile, no cook, ⛔ **no dry run of `ship.ps1`** (parse-check only, per the fence), no editor, no MCP, no `git add`, no commit, no push. ⛔ **`Config/DefaultEngine.ini` NOT touched** — TASK-713 owns it, and it ran in parallel.
Files touched: **`Tools/Packaging/ship.ps1`** · **`.claude/commands/ship.md`** · this handoff · the TASKBOARD status row for TASK-712 only.
Status → **ready-for-qa** (lands before TASK-702's review, which had not started — `qa/TASK-702.md` does not exist).
