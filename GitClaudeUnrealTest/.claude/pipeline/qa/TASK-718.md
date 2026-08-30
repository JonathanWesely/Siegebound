# QA Report — TASK-718 — the SCOPED re-QA of TASK-717's repairs (QA-LOOP 1 of 3, closing)

**Verdict: PASS — 0 BLOCKERS · 5 WARNS · 5 NITS**

Reviewed 2026-08-30 · reviewer: qa-reviewer · read-only.
Artifacts: `Tools/Packaging/ship.ps1` (2,263 -> **3,134** lines) · `.claude/commands/ship.md` (277 -> **313** lines).
Inputs: `qa/TASK-702.md` (the 3 blockers, §5's non-reversions, §6's 703 contract) · `handoffs/TASK-717-programmer.md` · `handoffs/TASK-669-buildmaster.md` §6/§9 · CONVENTIONS **`SHIP-§9`** (`§9a`/`§9b`/`§9c`/`§9d`), `SHIP-§7`/`§8`/`§8b`/`§8c`/`§8d`, `PKG-§6a`/`§9a-1`/`§9a-2`/`§9a-3`/`§9f`/`§10`.
⛔ Nothing executed — **`ship.ps1` was NOT run, not even `-DryRun`** (TASK-703 owns that). ⛔ No edits · no compile · no engine · no Git · **no TASKBOARD write** (this dispatch fenced it; the orchestrator carries the status flip).

⛔ **SCOPE HELD.** This is not a re-review of `qa/TASK-702.md` §3's twenty rulings — **those stand**. Reviewed here: the three blocker repairs, the three pinned non-reversions, 717's 8 deviations, and the two amendments 717 makes to 702's own §6 contract.

---

## 0. ⭐⭐ THE `A8-DESKTOP` RULING — DECIDED FIRST, BECAUSE IT SETS TONIGHT'S SCHEDULE

> ### ⚖️ **RULED: OPTION (i). A8 RUNNING FOR REAL IN A DRY RUN IS CORRECT. ON A LOCKED MACHINE, `DRYRUN-WOULD-STOP` NAMING `A8-DESKTOP` — AND NOTHING ELSE — IS A ⭐ PASS-SHAPED OUTCOME FOR TASK-703, AND ⭐ **703 CAN RUN TONIGHT.**

**Verified at source before ruling:** A8 lives at `ship.ps1:1800-1820`, inside PHASE A, with **no `-DryRun` guard**. `$deskChecked = ($A6Ok -and ($BootEvidence -eq 'Pixel'))` (`:1801`); on this machine (installed engine + Shipping) A6 auto-selects `Pixel` at `:1658` and assigns `$BootEvidence` at `:1726`, so `$deskChecked` is `$true` in a dry run. On a locked session `Test-DesktopLocked` names the lock owner and `Assert-Gate -Ok $false` fires; under `-DryRun` that adds to `$script:WouldStop` and **returns** (`:514-517`), so PHASE G emits `DRYRUN-WOULD-STOP` / exit 3 (`:3105`). **717's report of this behaviour is accurate.**

### The ruling, argued both ways on the merits

**(ii) "predict but do not enforce under `-DryRun`" — REJECTED, and its own premise is false.**
The case for (ii) is *"a dry run drives nothing, so gating it on desktop availability makes the recipe unvalidatable on a locked machine."* ⛔ **That premise does not hold against this implementation.** `Assert-Gate` under `-DryRun` **records and continues** — it never halts. I traced the whole path: after A8's would-stop the run still executes A7, the resume-seam fact, PHASE B's two `Note-Skipped`s, **the full PHASE C plan** (the exact UAT line `:2251`, all 11 `-COOKDIR`s expanded one per line `:2254`, the stage-hygiene plan with **both** resolved binary names and their authorities `:2268-2271`, the boot-verify drive plan `:2273-2305`, **the entire adjudication contract** `:2337`), the retention plan, the commit plan, the `DRY RUN SCOPE` block and the full gate/fact table. ⇒ **Every single acceptance item in `qa/TASK-702.md` §6.1 is still printed in full on a locked machine.** The only things that change are the last line and the exit code. **(ii) buys nothing and costs a divergence.**

**(i) A8-in-dry-run is CORRECT — three independent grounds:**
1. **`SHIP-§7`'s literal text:** *"executes every pre-flight and every check EXCEPT the cook, the zip and the commit."* A8 **is** a pre-flight. Excluding it would be the exception, and it is not the law's.
2. **`SHIP-§9c(1)`, the law this whole diff was written under:** *"VALIDATE AN INSTRUMENT AGAINST THE FAILURE IT IS MEANT TO DETECT... Demonstrate that it FAILS when the thing is broken, or do not call it a gate."* ⭐ **The machine is locked right now. A8 stopping tonight is `SHIP-§9`'s FIRST LIVE DEMONSTRATION that A8 is a gate and not a status line — a strictly stronger acceptance artifact than `DRYRUN-OK` would have been.** A `DRYRUN-OK` on a locked machine would mean A8 is the `SHIP-§9b` fake-gate family (a check wearing a gate's clothes) and would be a **BLOCKER against A8**.
3. **`SHIP-§0`:** a plan that behaves differently from the run it predicts is a divergence, and *"the divergence is a defect."* (ii) would manufacture exactly one.

⭐ **And the prediction is TRUE and USEFUL:** *"this ship would stop at A8"* is a correct statement about the real run, delivered **before** a 30-minute cook is queued rather than after — which is `PKG-§9f`'s *schedulable, not unattended* ruling arriving at the cheap end. ⇒ **Not a regression. The gate working.**

### ⭐ TASK-703's AMENDED TERMINAL-LINE CONTRACT ON A **LOCKED** MACHINE

This **amends `qa/TASK-702.md` §6.1 items 7 and 11** and supersedes item 1's `DRYRUN-OK` expectation for a locked run.

| Run | Acceptable terminal line on a LOCKED machine | Exit |
|---|---|---|
| **1. `-DryRun`** (default: Shipping, route auto-Pixel) | ✅ `SHIP RESULT: DRYRUN-WOULD-STOP`, with the summary line reading **`DRY RUN WOULD STOP AT: A8-DESKTOP`** — **A8-DESKTOP and no other gate ID** | **3** |
| **2. `-DryRun -BootEvidence Log`** | ✅ `SHIP RESULT: DRYRUN-WOULD-STOP`, **`DRY RUN WOULD STOP AT: A6-EVIDENCE-ROUTE`** — **A6 and no other gate ID.** A8 is **not** evaluated on this path (`$deskChecked` is false because `$A6Ok` is false) and records **`[PASS] A8-DESKTOP`** | **3** |

**Binding rules for 703, stated so there is no judgement call at 2 a.m.:**
- ⛔ **`DRYRUN-OK` on a LOCKED machine is a BLOCKER, not a pass.** It would mean A8 lied in exactly the case it exists to catch (`SHIP-§9b`). Route back.
- ⛔ **Any gate ID other than the one named in its row appearing in `DRY RUN WOULD STOP AT:` is a finding — quote it and route back.** In particular `A1-*`, `A4-FENCE`, `A5-DISK`, `A6` (run 1), `A7-*` stopping is a regression, judged on its own merits.
- ✅ **If the desktop happens to be UNLOCKED when 703 runs**, run 1's acceptable line reverts to `SHIP RESULT: DRYRUN-OK` (exit 0) **and A8 must appear as `[PASS] A8-DESKTOP  route Pixel: no lock-screen owner found at the primary-screen centre (...)`.** ⛔ A8 missing from the gate table entirely is a BLOCKER (it must never `Note-Skipped`).
- ⛔ **The exit code is never the verdict. Read the last stdout line** (the standing law, restated at `:179-190`).
- **Everything else in `qa/TASK-702.md` §6.1 items 2–10 and §6.2 is UNCHANGED and still mandatory**, with two amendments 717 makes and I confirm:
  - **item 7 amended:** the stage-hygiene plan now prints **TWO** resolved names plus **two** resolution authorities (`:2268-2271`) — expect `GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest-Win64-Shipping.exe` **[resolved by: the manifests (1 matching game exe)]** *and* `GitClaudeUnrealTest.exe` **[resolved by: the manifests (1 root-level exe)]**. ⭐ **If either prints `CONVENTION FALLBACK`, quote it — it is not automatically a defect (an uncooked stage legitimately falls back), but it is the fact that says whether to trust the name.** ⛔ If the game binary prints `.../GitClaudeUnrealTest.exe`, the 714 collateral repair regressed — BLOCKER, route back.
  - **item 11 amended:** WARN-1's confident stale line **is fixed** (`:2319-2328`) and **must NOT appear**; the run-2 output must instead read *"A6 found NO usable evidence route... this run resolved NO route at all."* ⚠️ **But expect a NEW cosmetic stale line one gate over on run 2 — A8's `[PASS]` evidence says "route Log ... no desktop is required" when the resolved route is NONE.** That is **WARN-2 of this report; note it, do not treat it as a new defect.**
- 📌 **703's handoff must state, in its own words:** a `-DryRun` on a locked machine proves PHASE A and the printed plan **and demonstrates A8 firing**; it proves nothing about the cook, the boot-verify, the click route, the adjudication, the zip or the commit (`SHIP-§7` amended, `SHIP-§9c(5)`).

---

## 1. THE THREE REPAIRS, VERIFIED AT SOURCE

### BLOCKER-1 — resolve the running game BY PATH — ✅ **REPAIRED, AND THE GUARD HOLDS**

`Get-StageGameProcess` (`:858-873`) enumerates `Get-Process -ErrorAction SilentlyContinue` and filters on `$_.Path.StartsWith($StageRoot.TrimEnd('\','/') + '\', OrdinalIgnoreCase)`. **Name-free.** Call sites: `:1514` (PHASE-A fact), `:2528` (pre-launch snapshot), `:2574` (window wait), `:2627` (title read), feeding the kill loop at `:2657`.

**⭐ THE `UNEXPECTED-ERROR` GUARD — I ATTACKED THIS FIRST, AS 717 INVITED, AND IT CANNOT FIRE.** Every read on the new enumeration path is covered, in both possible PowerShell behaviours:
- **the enumeration itself** — `Get-Process -ErrorAction SilentlyContinue` (`:865`); the parameter overrides `$ErrorActionPreference = 'Stop'` for this call. ✅
- **`.Path`** (`:868`) — `try { $path = $p.Path } catch { $path = $null }`, then `if (-not $path) { continue }`. ⭐ **This covers BOTH outcomes and that matters:** `Path` is a PS `ScriptProperty` over `MainModule.FileName`, which for a protected or cross-bitness process either (a) surfaces as an error that `ErrorActionPreference=Stop` promotes to terminating — **caught** — or (b) is swallowed and yields `$null` — **caught by the `-not $path` continue.** There is no third outcome.
- **`.MainWindowTitle`** — routed through `Get-ProcWindowTitle` (`:878-881`), which wraps `Refresh()` **and** the property read; `Refresh()` on an exited process throws `InvalidOperationException` and is caught. ✅
- **`.MainWindowHandle`** (`:2578`), **`.Kill()`** (`:2657`), **`$bp.HasExited`** (`:2658`) — all individually wrapped. ✅
- **`.Id` / `.ProcessName`** (`:1519`, `:2532`, `:2631`, `:2636`) — cached at object construction, cannot throw. ✅
⇒ ⛔ **An `UNEXPECTED-ERROR` (exit 1) from the enumeration is unreachable. The gate cannot fail in the wrong voice.** ✅

**The three-way truth table, re-derived rather than read:**

| Scenario | What the code does | ✓ |
|---|---|---|
| **shim + real exe** (Shipping, the standing case) | both match the path prefix; the wait loop at `:2575-2584` requires **a non-empty title AND a non-zero hwnd**, so the title-less shim is skipped and `…-Win64-Shipping` supplies both. The `:2629-2632` re-read takes the last non-empty title. **Both** are killed at `:2657`, plus `$bp`. | ✅ **the broken case is fixed** |
| **shim only / same process** (Development) | the one process supplies title + hwnd. The filter never assumed two. | ✅ |
| **stale process present** | matched by path, then **excluded by PID** — the snapshot at `:2528` is taken **before** `Start-Process` at `:2553`/`:2564`, and `$preExistingIds` is passed to every later query (`:2574`, `:2627`). It ⛔ **cannot** supply the title and is ⛔ **not** killed (`:2653-2657` iterates only the excluded list). If it is the *only* stage process, `$gameProcs.Count -lt 1` trips the prefilter at `:2735` — a correct STOP. | ✅ |

⭐ **PID (not `StartTime`) is the right exclusion key and the reason is right:** PIDs are unique among live processes; `.StartTime` throws for protected processes and a failed read would then **exclude the real game** — the unsafe direction. Concur.
✅ **WARN-5 closed for free** (`:2653-2658`). ✅ **`A3-QUIET-MODULE`'s `-Ok` is byte-identical** (`:1495`) with the non-inclusion reasoned in place at `:1499-1512` — **and the reasoning is correct**: a running game does not contend for the serialized compile/cook gate `PKG-§6` protects, and naming `<Project>-Win64-Shipping` in `$busyNames` would have repeated the very defect this task closed. The hazard is closed downstream **by measurement**, which is `SHIP-§9c(2)`'s preferred shape.

### BLOCKER-2 — the launch is a CLICK, not a refuted argument — ✅ **REPAIRED. ROUTE CHANGE ONLY.**

- **Route Pixel** (`:2561-2612`): `Start-Process -FilePath $StageExe -PassThru` — **no `-ArgumentList` at all** (`:2564`). Then wait up to `$BOOT_MENU_WAIT_SEC` for a titled window owned by a stage process, pre-click diagnostic capture, `Invoke-MenuClick`, `$BOOT_TRAVEL_WAIT_SEC` settle, capture. ✅ Exactly `PKG-§9f`'s ruled route.
- **⭐ The `GetAncestor(WindowFromPoint)` abort predicate is present and is the point** (`:992-1004`): the probe point is the post-`ClientToScreen` screen coordinate, `GA_ROOT` = 2, and on `$root -ne $Hwnd` it **restores non-topmost, names the offending root class, and returns `Ok=$false` with `NO INPUT WAS INJECTED`** — before any `mouse_event`. ✅ ⛔ There is no path that injects input without the game owning the pixel.
- **⭐ Route `Log` is unchanged where it matters.** `:2551-2552` reproduces the pre-repair line verbatim (`'{0} -windowed -ResX=1280 -ResY=720 -abslog="{1}"' -f $RECIPE_MAPS[1], $bootLog`), the same `while` loop on `$BOOT_TIMEOUT_SEC = 300` with the same two conditions, and `C3-BOOT-ARENA`'s `-Ok` at `:2685` is byte-identical (`$sawArena -and ($deckCards -gt 0) -and ($deckRows -gt 0) -and $heroOk -and ($bad.Count -eq 0)`). The only route-Log deltas are one `Say` line and the fact that its process resolution is now by path — **strictly safer, identical in the case route Log runs in.** ✅
- **⛔ `PKG-§6a` UNTOUCHED — swept, not taken on trust:** `$BOOT_MARK_ARENA/_DECK/_HERO` (`:366-368`), `$BOOT_RE_DECK` (`:373`), `$BOOT_FAIL_PATTERNS` (`:376`), `$BOOT_BENIGN_PATTERNS` (`:383`), `$ADJ_CRITERIA` (`:327-336`), `$SUITE_BASELINE = 143`, `$MIN_FREE_GB = 4`, `$RETENTION_KEEP = 2`, `$SIZE_ALARM_RATIO = 0.60`, `$BOOT_TIMEOUT_SEC = 300` — **all unmodified.** ✅ The arena bar's text is the same text the adjudicator will read.
- **The self-contradiction is gone:** the dry-run boot plan (`:2295-2305`) now *explains why there is no map argument* instead of declaring the live line refuted. `SHIP-§0`'s divergence is closed. ✅
- **The P/Invoke surface** (`:803-821`): I checked every signature by inspection. `SetWindowPos`, `ClientToScreen`, `SetCursorPos`, `SetForegroundWindow`, `mouse_event`, `GetClassName`, `GetWindowThreadProcessId`, `GetAncestor`, `WindowFromPoint` are canonical; **`GetClientRect(IntPtr, out RECT)` — the one new one — is also canonical**, and `RECT` is the correct 4×`int` layout. `SWP_FLAGS = 0x13` = `NOSIZE(1)|NOMOVE(2)|NOACTIVATE(0x10)` ✅; `HWND_TOPMOST = -1`, `HWND_NOTOPMOST = -2` ✅; `mouse_event` `0x0002`/`0x0004` = LEFTDOWN/LEFTUP ✅. ⚠️ **Reviewed, not run** — 717 declared that honestly (§8.6) and the fence forbids more.
- **⭐ The error direction is genuinely safe, and I checked the off-by-one case specifically:** entry 2 is *Sandbox (No Bot)* and entry 3 is *Deck Builder*. One pitch **low** lands on Sandbox — still a real arena, still satisfies all four `PKG-§6a` criteria, so **not a false pass of the bar**; two low lands on Deck Builder, and one **high** lands above the list — both produce a non-arena frame the adjudicator must FAIL. ⇒ ⛔ **No aim error produces a false PASS.** ✅

### BLOCKER-3 — the spent verdict is retired — ✅ **REPAIRED, AND ⭐ D2 IS NOT REVERTED**

**The rule as implemented — verified by control-flow, not by the comment:** the retirement is reachable **only past** `C3-VERDICT-FORM` (`:2021`) and `C3-VERDICT-BINDING` (`:2075`), both of which `throw` in a live run. ⇒ *"retired when READ TO A VERDICT"* is enforced structurally.

| Reading | Code | Record afterwards | State | ✓ |
|---|---|---|---|---|
| **PASS that resumes** | `:2357-2455` | **ARCHIVED** `…<stamp>.consumed.json` (`Move-Item`, `:2437`) | `boot ADJUDICATE -> PASS` + `bootInstrument` (`:2417-2418`), `adjudication` added (`:2424`) | ✅ |
| **FAIL** | `:2105-2115`, **before** the `C3-VERDICT` STOP at `:2117` | **ARCHIVED** `…consumed-FAIL.json` | ⛔ **deliberately NOT advanced** — still `ADJUDICATE`, so the fixed build re-cooks | ✅ |
| **Malformed** | `C3-VERDICT-FORM` throws at `:2021` | ⛔ **left in place** | untouched | ✅ |
| **Non-binding / stale** | `C3-VERDICT-BINDING` throws at `:2075` | ⛔ **left in place** | untouched | ✅ **D2 INTACT** |

- **⭐ "Exactly two fields move" — VERIFIED FIELD BY FIELD** at `:2407-2425`: `schema`, `timestampUtc`, `head`, `treeHash`, `config`, `recipeHash`, `compile`, `suiteTotal`, `cook`, `bootCapture`, `bootCaptureSha`, `stageExeRel`, `stageExeBytes`, `stageExeUtc` are carried **verbatim**; `boot` and `bootInstrument` move; `adjudication` is added. **And carrying is provably identical to re-deriving**, because the reuse decision at `:2147-2170` had already required `schema`/`head`/`treeHash`/`config`/`recipeHash`/`compile`/`cook`/`suiteTotal`/`stageExeUtc` all equal to this run's measured values, and `C3-VERDICT-BINDING` had separately re-measured `stageExeBytes` (`:2071`). ⇒ ⛔ **Nothing can be laundered through the carry.**
- **⭐ Order (`:2426-2439`) — archive first, advance second — is correct and the argument is right.** I re-derived the failure modes: archive-then-crash leaves an armed record beside `boot=ADJUDICATE`, which the next run re-consumes **idempotently**; advance-then-crash would leave `boot=PASS` beside an armed record, which stops at `C3-VERDICT-BINDING` with *"not a pending adjudication"* — **the chore back again, produced by the repair meant to remove it.** The chosen order is the survivable one. The whole thing is in `try/catch` and a failure is reported as a fact (`:2453`), never hidden.
- **⭐ A CONSUMED RECORD CANNOT RESURRECT — verified mechanically, not from the comment:** copy the archive back and `:2046-2047` reads `boot='PASS'` and appends *"the state file records boot='PASS', not a pending adjudication"* to `$mis` ⇒ `C3-VERDICT-BINDING` STOPS. ✅
- **⭐ A GENUINELY STALE RECORD STILL STOPS:** `$mis` accumulates HEAD (`:2063`), config (`:2064`), pending-capture identity (`:2052`), capture sha (`:2053`, `:2061`), staged-exe size and mtime (`:2071-2072`). **`SHIP-§8b(1)`'s anti-rubber-stamp property is untouched.** ✅
- ⭐ **`boot=PASS` is not trusted afterwards** — it re-enters the same nine-measurement guard list at `:2149-2170` on every future run. Correct: this is a second *lock*, not a second *door*.
- ⭐ **717's D2 extension to the FAIL branch is right and I adopt it as doctrine:** leaving a FAIL armed reproduces the identical chore one branch over the moment HEAD moves, and **not** advancing the state on FAIL is what guarantees a failed boot is never resumed onto.

---

## 2. THE THREE PINNED NON-REVERSIONS — ✅ ALL THREE INTACT

| Pin | Verified at | Result |
|---|---|---|
| ⛔ **714's manifest-derivation repair, coupled to the prune** | `Get-StageGameExeRel` `:1144-1172`; consumers `$StageBinExe` `:1853`/`:2476`/`:2492`/`:2828`, `$pdb` `:2847` (`ChangeExtension` of the **derived** exe), `$realBinary` `:2942` | ✅ **NOT reverted — EXTENDED.** ⭐ **The coupling is now stated IN THE CODE at three sites** (`:1136-1140`, `:1844-1851`, `:2935-2940`), including the catastrophic direction verbatim: *"once stage hygiene deletes that orphan this gate would have failed every clean package."* `qa/TASK-702.md` §1.4's binding coupling ruling is honoured at the artifact, where the next reader meets it. |
| ⛔ **D2 — a stale record STOPs** | `:2026-2079` | ✅ Unchanged. Only records **read to a verdict** retire. |
| ⛔ **`$ADJ_HEDGES` unchanged, PASS-only, pre-PHASE-B** | `:352-354` (**18 entries, text identical**); PASS-only guard `:2010`; seam ends `:2131`, `Head 'PHASE B'` at `:2193` | ✅ All three properties hold. A rejected wording still costs **one text edit**, ⛔ never a cook. `$ADJ_BANNED_OBS` also unchanged (23 entries, `:344-346`). |

---

## 3. THE FOURTH AND FIFTH INSTANCES — `SHIP-§9`'s FIRST LIVE TEST — ✅ **THE SWEEP HOLDS**

### The fourth instance (`SHIP-§9a`): the root shim — ✅ derived, and the derivation is right

`Get-StageShimExeRel` (`:1188-1207`) selects the manifests' single **root-level** `.exe` — `$l.EndsWith('.exe') -and ($l.IndexOf('/') -lt 0)` — ⭐ **by POSITION, never by basename, which is TRAP 2 in reverse and is exactly right**: the 347 MB orphan shares that basename and lives one directory down, so a basename test would have selected the wrong file. Convention fallback retained with the authority surfaced (`$script:StageShimResolvedBy`, NIT-2's shape). Re-derived after the cook from fresh manifests (`:2477`), symmetrically with the game exe.
**No false hit is possible on the measured stage:** the only other manifest `.exe` entries are `GitClaudeUnrealTest/Binaries/Win64/…` and `Engine/Extras/Redist/…/vc_redist*.exe`, both of which contain `/`. ⇒ exactly one hit. ✅
**Consumers all agree with the zip writer:** `$clickTarget = 'Windows/' + $StageShimRel` (`:2941`) against entries built as `'Windows/' + <stage-relative, forward-slashed>` (`:2920`); `-contains` is case-insensitive, so manifest casing cannot break the readback. ✅

### The three cleared sites — ⭐ **I re-derived each clearance rather than accepting it. ALL THREE ARE CORRECT.**

| Site | 717's clearance | My verification |
|---|---|---|
| `:2536` `<Project>\Saved\Logs\<Project>.log` | staged project folder + log are named after the **project**, not the target; configuration-independent | ✅ **CORRECT.** UE stages under the `.uproject`'s own name and names the log from the project, not the build target. It is also **doubly harmless**: route Log passes `-abslog=` (`:2551`) which overrides it, and route Pixel never reads it (`PKG-§9a-1`, log-silent). |
| `:2903` `<Project>\Saved` | same reason | ✅ **CORRECT.** Same staged-project-folder identity; matches 715's measured tree. Failure direction is also benign — `Remove-Item … -ErrorAction SilentlyContinue`, so a wrong name would leave logs in the zip, not break the ship. |
| `:1885` `('{0}Editor' -f $ProjectName)` | the editor target genuinely **is** `<Project>Editor` | ✅ **CORRECT**, and independently corroborated: `CLAUDE.md`'s own build line reads `Build.bat GitClaudeUnrealTestEditor …`. This is a **target name**, and the editor target is project-named in every configuration — it is not the `SHIP-§9a` class. |

⇒ ⛔ **No "cleared" comment is wrong.** ✅ (📌 But see NIT-1: two of the three clearances are **not commented in place**, contrary to the handoff's §2 claim.)
**Sweep completeness:** every remaining `$ProjectName` reference in the file (`:1392`, `:1395`, `:1842`, `:1852`, `:2475`, `:2477`, `:2490`, `:2826`) is either the definition, a fact row, or an argument **into** a manifest-derivation function. ⇒ **Zero un-swept guessed staged-binary or process names remain.** ✅

### The fifth instance (`SHIP-§9b`): A8 closes the class — ✅ **CONFIRMED**

- **A6's probe cannot answer the question, and I verified why at source:** `:1671-1689` does `CopyFromScreen` into an 8×8 bitmap and reports success. TASK-716 measured screen capture **succeeding on a locked session** (capturing the lock screen). ⇒ A6 passes, route Pixel is selected, and — **without A8** — the run would cook for 30 minutes and only then discover at C3 that no click can land. ⭐ **Same shape as B1: an instrument correct in the ordinary case and false in exactly the case that matters.**
- **A8 uses the predicate the CONSUMER uses, literally the same call:** `Test-DesktopLocked:918` → `[ShipWin32]::GetAncestor([ShipWin32]::WindowFromPoint($pt), 2)`; `Invoke-MenuClick:997` → the identical expression. ⇒ **`SHIP-§9c(2)` satisfied by identity, not by analogy.** ✅
- **⛔ It never `Note-Skipped`s** — swept: `Note-Skipped 'A8` = **0 hits**; A8 asserts in every configuration (`:1812`). ✅ A6's repaired shape was not un-repaired.
- **Positive-lock-detector direction is correct and cannot manufacture a pass:** `-Ok (-not $deskRes.Locked)` can only ever ADD a stop; *"could not measure"* passes and is reported (`:1809-1811`); a **missed** lock is still caught downstream because the rig refuses to inject (`:998-1004`) and `C3-CAPTURE` records that the drive never happened (`:2745-2749`). ⭐ **And the small asymmetry is already handled in place:** A8 probes the primary-screen centre while the rig probes the game's client point, and the gate's own evidence says so — *"this is not a proof that the click will land; the rig re-tests the exact point before it injects anything"* (`:1818`). Honest and correctly scoped.

---

## 4. TASK-717's 8 DECLARED DEVIATIONS — RULED

| # | Deviation | Ruling |
|---|---|---|
| **D1** | The click rig **reproduced in-file** rather than shelled out to `t669_topclick.ps1` | ✅⭐ **ACCEPT — 717 IS RIGHT, AND I VERIFIED THE PREMISE RATHER THAN TAKING IT.** `t669_topclick.ps1` **does not exist anywhere in the repo** (globbed: 0 hits; same for `t716_probe3.ps1`), and `handoffs/TASK-669-buildmaster.md` §9 states in its own words that it lives in the *"session scratchpad"*. ⇒ **A release procedure calling it would be a `Test-Path` failure on every future run — forever-stop number five, arriving by dependency instead of by name.** The mechanism is ~20 lines against an unbounded, session-scoped dependency, the abort predicate came across intact, and QA already accepted duplication over a shared dependency once (712 D3). ⛔ **The alternative was not "call the proven script" — it was "call a file that is not there."** |
| **D2** | Retirement extended to the **FAIL** branch | ✅ **ACCEPT — the generalisation is correct** (see §1 BLOCKER-3). State deliberately not advanced ⇒ a failed boot is never resumed onto. ⭐ Promote to doctrine alongside 714's D4. |
| **D3** | New gate **`A8-DESKTOP`** | ✅ **ACCEPT — IN FENCE.** Authorised by the mid-task TASK-716 relay and **required** by B2's repair: a click route needs a drivable desktop, and `SHIP-§9b` forbids building that check on the two lying probes. Strictly stronger, never `Note-Skipped`, positive-detector only. See §0 for the dry-run ruling. |
| **D4** | The root shim is manifest-derived (`Get-StageShimExeRel`) | ✅ **ACCEPT — IN FENCE and hunting the class rather than the instance**, which is precisely what `SHIP-§9a` asks for. Not presently broken; still a guess feeding three gates. Convention fallback keeps it from becoming its own forever-stop. |
| **D5** | `C2-STAGE-ONE-EXE`'s `-Ok` gains `$shim.Count -le 1` | ✅ **VERIFIED AS EXACTLY NIT-3 AND NOTHING MORE.** `:1293` reads `(($runnable.Count -eq 1) -and ($shim.Count -le 1))`. ⭐ **`$allExe`/`$shim`/`$runnable` are defined at `:1281-1284` identically to the pre-repair form** (exclude `engine/`, split root vs nested by `IndexOf('/')`). ⇒ the delta is **one added conjunct**, no scope change, no set redefinition. `-le 1` not `-eq 1` is correct: a bootstrap-less stage has zero shims and is not a hazard, while **two** root exes is exactly the wrong-exe-to-click ambiguity `PKG-§10` forbids. **Strictly stronger; passes 715's measured stage (1 shim).** ⭐ **It is the ONLY pre-existing `-Ok` expression that moved — I diffed the full inventory: 38 gate IDs, Δ = +1 (`A8-DESKTOP`), 0 removals, 0 renames, 0 relaxations, param block 9 parameters unchanged.** |
| **D6** | Shared `Save-ScreenCapture` for C3's two captures | ✅ **ACCEPT.** ⛔ **A6's probe at `:1671-1689` is UNTOUCHED** — I compared it against `qa/TASK-702.md` §3's description of 712 D3/D4: still an 8×8 **in-memory** bitmap, still writes no file. ⇒ **NIT-7's refactor is correctly NOT done** and 712's *"only A6 moved"* auditability is intact. Factoring only the path that genuinely became two calls is the right, minimal move. |
| **D7** | A pre-click menu capture no spec asked for | ✅⭐ **ACCEPT — and it is the best small idea in the diff.** A mis-aimed click and a broken build produce the *same* adjudicated frame; without this you cannot tell a **rig** finding from a **build** finding. ⛔ **And it cannot be adjudicated by accident — that is mechanical, not care:** only `$pixelShot` is written into the state's `bootCapture` (`:2786`), and `SHIP-§8b` rule 9 (`:2052`) rejects any record whose `capturePath` is not the state's pending capture. The filename carries `NOT-ADJUDICABLE` for the reader who meets the file before the comment. |
| **D8** | Route `Log` deliberately keeps the map argument | ✅ **ACCEPT — correct, and the reasoning is exact.** `PKG-§9f` is a **Shipping** measurement; TASK-699 measured the **Development** binary honouring the argument. Removing it would break route 1's byte-for-byte invariance that `qa/TASK-702.md` §1.3 row 14 verified, in exchange for nothing. |

---

## 5. FINDINGS

### ⛔ BLOCKERS — **NONE**

### ⚠️ WARNS

- **[WARN-1] `ship.ps1:1341` (and `ship.md:297`) — the summary's *"There is no input-injection lane"* is now FALSE, and it is printed in the run's own final report.** This diff **added** an input-injection lane: `Invoke-MenuClick` calls `mouse_event(LEFTDOWN/LEFTUP)` at `:1008-1010`. The **conclusion** (*"nothing here is a gameplay-feel claim"*) survives and is still the honest one — a single menu click is not gameplay input — but the **premise** is a confident stale literal, in the `PKG-§9a-3` class, sitting in `SHIP-§6`'s honesty block where a reader is most entitled to trust it. It is contradicted three screens up by the run's own `HOW THIS CAPTURE WAS REACHED` block (`:2799-2806`), which is what keeps this a WARN rather than a blocker. **Fix (one clause):** *"There is no GAMEPLAY input lane. The only input this script injects is a single menu click to reach the arena (PKG-9f), and it is reported above. Nothing here is a gameplay-feel claim."*
- **[WARN-2] `ship.ps1:1803-1804` + `:1813-1814` — A8's not-applicable text asserts *"route Log"* on the path where the resolved route is NONE.** When `$A6Ok` is `$false`, `$deskChecked` is `$false` and A8 records `[PASS] … route Log: the boot-verify reads the packaged build own log and injects no input`, while the fact table one screen up correctly says `Evidence route (resolved): NONE`. ⭐ **This is exactly the WARN-1 shape 717 just repaired at `:2319-2328`, one gate over.** Live-unreachable (A6's `Assert-Gate` throws first), but **dry-run-reachable — it will print on TASK-703's `-BootEvidence Log` variant.** **Fix:** mirror the `-not $A6Ok` branch already written at `:2319`. ⚠️ **703: note it, do not treat it as a new defect** (same handling `qa/TASK-702.md` §6.1 item 11 gave its predecessor).
- **[WARN-3] `ship.ps1:1800-1820` evaluated before the reuse decision at `:2133-2179` — A8 stops a RESUME that drives no input at all.** The second invocation of an adjudicated ship takes the `$reuse` branch: no cook, no launch, no click — it zips, commits and reports. It needs no desktop. But A8 sits in PHASE A and fires on route Pixel regardless, so a resume attempted while the desktop is locked STOPS at `A8-DESKTOP`. **Bounded, which is why it is not a blocker:** nothing is consumed (the seam at `:1919+` is downstream of A8), no cook is lost (the state stays armed), and unlocking clears it. But it is a gate firing on a condition its run does not depend on — the shape `A3-QUIET-MODULE`'s comment at `:1502-1504` correctly refuses one paragraph earlier. **Fix:** move A8 below the reuse decision, or add `-and (-not $reuse)` to `$deskChecked` once `$reuse` is known.
- **[WARN-4] `ship.ps1:415-425` + `:981-982` — the click point is a client-rect FRACTION derived at 1280×720, and route Pixel launches with NO arguments, so the client size is whatever the player's saved settings produce.** UMG DPI scaling is a **curve**, not a linear factor, so a fraction measured at 720p is not guaranteed to hold at 1080p/4K, and the game may come up fullscreen. ⭐ **I verified 717's reading of TASK-669 and it is right:** the handoff records *"centered VBox, pitch 49.7 px … Deck Builder button center = client (639, 310)"* and separately confirms **7 entries with `Play (vs Bot)` first and `Deck Builder` third**. `360 − 49.7 = 310.3` ⇒ entry 3 is one pitch above centre ⇒ entry 4 is on centre ⇒ `(7+1)/2 = 4`, and `0.5 + (1−4)·0.069028 = 0.292917` reproduces it. ⛔ **669's own prose formula (`H/2 − 2·49.7` = 260.6) contradicts its own measured 310 and is off by one entry; 717 correctly took the MEASUREMENT as authoritative.** The error direction is safe (§1 B2), and the drive report prints `client (x,y) of {cw}x{ch}` (`:1015-1016`) which makes a mismatch diagnosable. **Not a blocker, but a real first-live-ship cost.** ⇒ 📌 **Instruction for the first live `/ship`: QUOTE the `menu click:` detail line. If `{cw}x{ch}` is not 1280×720 and the adjudicated frame is a menu, that is a RIG finding first — check the pre-click frame before touching the cook.**
- **[WARN-5] `ship.ps1:2692` and `:2754` still throw upstream of the state write at `:2774`.** A `C3-BOOT-TITLE` or `C3-CAPTURE` stop still discards a proven compile + suite + 30-minute cook. 717 named this honestly (§8.9) and judged it out of fence; **I agree it is out of fence for a blocker repair** — making an instrument failure re-*capture* without re-*cooking* is a new capability. But with A8 landing, this is now the **last** expensive-discard path in the file. ⇒ **Recommend the manager board it as its own task** (a re-capture seam, or moving the state write above the pixel gates with `boot='CAPTURE-FAILED'`).

### 📌 NITS

- **[NIT-1] `:1885` and `:2903` carry no clearance comment.** Handoff §2 claims the three swept sites were *"cleared with the clearance commented in place"*; **only `$defaultLog` is** (`:2537-2541`). ⭐ **All three clearances are factually CORRECT (verified in §3)** — but the whole value of a clearance is that the *next* reader meets it at the line instead of re-deriving it, which is `SHIP-§9`'s own stated method. Two one-line comments.
- **[NIT-2] `Initialize-ShipWin32` (`:799-827`) now runs in a `-DryRun` via A8, compiling a P/Invoke assembly into `%TEMP%`.** ⛔ **The fence holds** — nothing is written into the project, the stage, `.ship/`, the zip or git — but the header's *"it produces no artifact at all"* (`:52`) and `ship.md:72`'s *"compiles nothing"* are now marginally overstated against 712 D4's *"a dry run writes nothing at all"*. ⇒ **703's handoff should state the temp assembly plainly rather than repeat the absolute.**
- **[NIT-3] `Get-StageGameProcess` enumerates every process and reads `.Path` (one caught exception per protected process) inside a 5-second poll loop for up to 120 s.** Correct, necessary and comfortably inside the poll budget, but measurably slower than the name query it replaced. ⛔ **If it ever needs tuning, cache the enumeration or widen the poll — never go back to a name.**
- **[NIT-4] A record read to a PASS verdict on a run where `$reuse` is FALSE is not retired.** Reachable when the bindings hold (HEAD, config, staged exe identical) but a reuse guard differs — realistically only `treeHash` (uncommitted build-relevant dirt) or `recipeHash`. The run falls to the cook branch, re-cooks, and suspends afresh; the caller then **overwrites the same stable path** with the new record, so it is self-healing in practice, and a genuine leftover still STOPs at `C3-VERDICT-BINDING` with a remedy that names the file. Recorded so it is not rediscovered as a defect.
- **[NIT-5] `$BOOT_MENU_CLICKS = 2` (`:418`) may land the second press in-world after travel.** 717 judged it harmless (no unit selected, no card held) and it preserves the proven t669 default — **concur** — but note it in the first live ship's report so an unexplained in-world artifact in the adjudicated frame is not misdiagnosed as a build defect.

---

## 6. STANDING FENCES — ✅ ALL CLEAN (swept mechanically, not asserted)

| Fence | Result |
|---|---|
| ⛔ Zero skip flags | ✅ `SkipTests\|SkipCook\|SkipSuite\|NoVerify\|IgnoreGates\|AcceptWarnings\|FastShip\|YesReally\|PixelAdjudicated\|AcceptPixel` → **4 hits, ALL in the header's own prohibition prose** (`:32`, `:33`, `:39`). **0 parameter declarations.** Param block = **9 parameters, byte-identical**. |
| ⛔ No gate relaxed | ✅ 38 gate IDs, **Δ = +1 (`A8-DESKTOP`), 0 removals, 0 renames**. Every pre-existing `-Ok` expression byte-identical **except** `C2-STAGE-ONE-EXE` (D5, strictly stronger, verified exactly NIT-3). `Note-Skipped 'A6…'` = **0**; `Note-Skipped 'A8…'` = **0**. |
| ⛔ `PKG-§6a` untouched | ✅ All ten boot/adjudication constants unmodified (listed in §1 B2). |
| ⛔ No `git push` / `add -A` / `--amend` / `reset --hard` / `filter-branch` | ✅ `git push` **0** · `git add -A` **0** · `git add .` **0** · `reset --hard` **0** · `filter-branch` **0**. The single `--amend` hit is prohibition prose (`:705`). Only two writing git calls exist, both PHASE F: `git add -- <one explicit path>` in a loop (`:3061`) and `git commit -F` (`:3093`). ⛔ No directory sweep. |
| 0 non-ASCII bytes in the `.ps1` | ✅ **Swept: 0 matches for `[^\x00-\x7F]`.** 0 section signs. Law citations stay ASCII (`SHIP-8b`, `PKG-9f`, `SHIP-9a`). |
| `ship.md` in lockstep | ✅ Carries A8-DESKTOP as a gate row (`:97`) **and** the dry-run consequence (`:74`), the click route (§2b.1 steps 1–5), the pre-click diagnostic (`:130`), the click-delivered pre-filter (`:105`), the manifest-derived shim (`:101`), the retirement table + rules (§2b.4, `:201-214`), and `SHIP-§9a`/`§9b` in the hazard crib (`:309-311`). ⭐ **And it states the honest status: `:136-137` says the click route is law-ruled and rig-supported but ⛔ NOT YET observed end-to-end.** ⛔ No divergence from the script found. |
| Parse clean | ✅ **Reviewed structurally, not executed** (the fence forbids running the file). I hand-checked the new code for the two PS 5.1 traps that a reader can catch statically: **(a) StrictMode member access** — every `$res`/`$r`/`$clickRes` hashtable is fully initialised at every return path, and every state/record read goes through `Get-JsonProp`; **(b) accidental pipeline output from the six new functions** — every non-void call is `\| Out-Null` or assigned, so each returns exactly its one intended object. 717's AST-parse claim (0 errors) is consistent with everything I read. |

---

## 7. NOTES FOR BUILD-MASTER / TASK-703

1. ⭐ **Read §0 before you run anything.** The acceptable terminal line on tonight's locked machine is **`DRYRUN-WOULD-STOP` naming `A8-DESKTOP` and nothing else** (exit 3). ⛔ **`DRYRUN-OK` on a locked machine is a BLOCKER**, not a pass.
2. **Quote, do not summarise:** the exact UAT line, the **11** `-COOKDIR` entries one per line (read them character by character against `PKG-§5a`), the **two** resolved binary names **with their resolution authorities**, the full adjudication contract, the `DRY RUN SCOPE` block, and the whole gate table.
3. **Expect and quote WARN-2's stale *"route Log"* line on the `-BootEvidence Log` variant.** It is a known, recorded cosmetic residue — ⛔ not a new defect.
4. **Expect a temp P/Invoke assembly in `%TEMP%`** from A8's `Add-Type` (NIT-2). ⛔ Nothing is written into the project, the stage, `.ship/`, the zip or git — verify that and say so in those terms rather than repeating *"writes nothing at all"*.
5. ⛔ **`SHIP-§7`'s blind spot is unchanged and must be restated in the handoff:** this dry run proves PHASE A and the printed plan, **plus** that A8 fires. It proves **nothing** about the cook, the boot-verify, the click route, the adjudication, the zip or the commit. ⭐ **And the click route has never been observed end-to-end on a Shipping binary** (`ship.md:136-137`) — the first `/ship` that reaches C3 under Pixel is its first live exercise.
6. **For that first live ship:** quote the `menu click:` detail line including `client (x,y) of {cw}x{ch}` (WARN-4), and if the adjudicated frame is a menu, **read the pre-click frame before blaming the cook** — a menu after a *delivered* click is a build finding; a menu after a *mis-aimed* click is a recipe finding, and the two captures are how you tell them apart.

## 8. NOTES FOR THE MANAGER (not blockers, not for the programmer)

1. **WARN-5 wants its own task:** `C3-BOOT-TITLE`/`C3-CAPTURE` still discard a proven compile+suite+cook. With A8 landed, it is the last expensive-discard path in the file, and closing it is a **new capability**, correctly out of this loop's fence.
2. **`PKG-§7a`'s two false factual claims are still open** (`qa/TASK-702.md` §7.1, re-confirmed). Unchanged, still the manager's; `A4-FENCE` measures its way to the right answer either way.
3. **`SHIP-§9` earned its keep on its first outing.** Two of this loop's most valuable items — the shim derivation and A8 — exist because 717 hunted the *class* instead of the three named instances. ⭐ Worth recording that the law changed the diff's shape, not just its commentary.
