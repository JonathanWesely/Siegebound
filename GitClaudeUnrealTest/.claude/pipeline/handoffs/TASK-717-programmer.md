# TASK-717 — gameplay-programmer handoff — THE QA-LOOP REPAIR (3 blockers, `qa/TASK-702.md`)

**Status: ready-for-qa.** Files touched: **exactly two.**
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\Packaging\ship.ps1` — 2,263 → **3,134 lines**
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\commands\ship.md` — 277 → **313 lines**

⛔ **Nothing executed.** No cook · no compile · no editor · no MCP · no Git · no `ship.ps1` run, not even `-DryRun`. Static validation only: **AST parse CLEAN (0 errors)** and **0 non-ASCII bytes / 0 section signs** in the `.ps1`. Law citations in the script stay ASCII (`SHIP-8b`, `PKG-9f`, `SHIP-9a`); the `.md` keeps `SHIP-§8b` form.

Read in full first: `qa/TASK-702.md` (§3's twenty rulings, §5's non-reversions, §6's 703 contract) · `handoffs/TASK-715-buildmaster.md` §6 · `handoffs/TASK-716-buildmaster.md` (landed mid-task) · CONVENTIONS `PKG-§6a/§9a-1/§9f/§10`, `SHIP-§8a..§8d`, and **`SHIP-§9`** — which was written while I worked and is the exact generalisation of what these three blockers are.

---

## 1. THE THREE BLOCKERS — BEFORE / AFTER

### BLOCKER-1 — the running game is resolved BY PATH, not by name

| | |
|---|---|
| **Before** | `ship.ps1:1801` — `Get-Process -Name $ProjectName \| Where-Object { $_.Path.StartsWith($StageWin) }`. `-Name` applies **no implicit wildcard**, so under Shipping it matched only the title-less root shim (715 §6: pid 28688 `Title=''`) and **never** pid 9104 `GitClaudeUnrealTest-Win64-Shipping` `Title='Siegebound'`. ⇒ `$windowTitle` stayed `'(not measured)'` ⇒ `C3-BOOT-TITLE` false on **every** Shipping run ⇒ `throw` at `:1863`, **upstream of the state write at `:1929`**. The kill loop at `:1828` inherited the same empty list, so the real game survived the ship holding `Saved/` open (WARN-5). |
| **After** | New function **`Get-StageGameProcess`** (`ship.ps1:830-877`, comment + body) enumerates all processes and filters on `$_.Path` under `$StageWin + '\'` — **name-free**, catches shim *and* game, works when they are the same process, and fixes the kill loop for free. Call sites: **`:1514`** (PHASE A fact), **`:2528`** (pre-launch snapshot), **`:2574`** (window wait), **`:2627`** (the title read). `C3-BOOT-TITLE`'s `-Ok` expression is **byte-identical**; only what feeds `$windowTitle` changed. |

**Two guards that are not decoration:**
1. **`Process.Path` throws** for protected/cross-bitness processes, and the script runs under `$ErrorActionPreference = 'Stop'` — an unguarded read inside a *full* enumeration would surface as `UNEXPECTED-ERROR` (exit 1), i.e. **a gate failing in the wrong voice**, the class 714 closed for the record and WARN-3 for the state file. Every `.Path` and every `MainWindowTitle` read is wrapped (`Get-ProcWindowTitle`, **`:878-881`**).
2. **Stale exclusion is by PID snapshot taken *before* the launch** (**`:2522-2534`**), not by clock arithmetic. The stage path is stable across runs, so path matching alone would happily adopt a leftover game as proof that *this* cook booted. PIDs are unique among live processes, so the snapshot is exact; `.StartTime` was deliberately **not** used as a filter because it can throw and a failed read would then *exclude the real game*.

**Leftover processes are reported, never killed** (`:2529-2534` and the PHASE-A fact at **`:1514`**). They may be Jonathan's.

**`A3-QUIET-MODULE`: `-Ok` left byte-identical, with the reason written in place (`:1499-1521`).** A running game does not contend for the serialized compile/cook gate `PKG-§6` protects, so stopping for it would be a gate firing on the wrong thing — *and* naming `<Project>-Win64-Shipping` in `$busyNames` would have repeated the very defect this task closes. What actually closes the hazard is the path resolution + pre-launch exclusion downstream. A new fact `Staged-game processes` names any leftovers with their PIDs.

### BLOCKER-2 — the launch route is now a CLICK, not a refuted argument

| | |
|---|---|
| **Before** | `ship.ps1:1786-1788` launched with `$RECIPE_MAPS[1]` — the argument `PKG-§9f` measured as **ignored in Shipping** — then spun the full `$BOOT_TIMEOUT_SEC = 300` waiting for log markers that can never appear, then screenshotted whatever was in front with the game on its menu. ⇒ the only capture obtainable was a **menu**, whose only honest adjudication is **FAIL**. And `:1709-1719` already declared that launch line refuted: **the file contradicted itself** (`SHIP-§0`: that is a defect). |
| **After** | The launch is **route-dependent** (**`:2544-2613`**). Route `Log` is **byte-for-byte unchanged** (map arg + the same `while` loop + `$BOOT_TIMEOUT_SEC`) — 699 measured the Development binary honouring it. Route `Pixel` executes `PKG-§9f`'s ruled route: **(a)** launch `$StageExe` with **no arguments at all**; **(b)** wait up to `$BOOT_MENU_WAIT_SEC = 120` for a titled window owned by a stage process (not for a log line — there is none); **(c)** take a **pre-click menu capture**, diagnostic only; **(d)** click `Play (vs Bot)` via `Invoke-MenuClick`; **(e)** wait `$BOOT_TRAVEL_WAIT_SEC = 90` for menu → level travel → HUD, then capture. |

⛔ **This is a route change ONLY. `PKG-§6a` is untouched** — `$BOOT_MARK_*`, `$BOOT_RE_DECK`, `$BOOT_FAIL_PATTERNS`, `$BOOT_BENIGN_PATTERNS`, `$ADJ_CRITERIA`, `$SUITE_BASELINE`, `$MIN_FREE_GB`, `$RETENTION_KEEP`, `$SIZE_ALARM_RATIO` all unmodified. What must be on screen is exactly what it was, and a caller that can see still judges it.

**The click rig is reproduced IN THE FILE (`Invoke-MenuClick`, `:938-1022`), not shelled out to — declared deviation D1.** Same mechanism as `t669_topclick.ps1`: `SetWindowPos(HWND_TOPMOST, SWP_NOSIZE|NOMOVE|NOACTIVATE)`, `ClientToScreen`, `SetCursorPos`, the **`GetAncestor(WindowFromPoint(pt), GA_ROOT) == hwnd` abort predicate**, `SetForegroundWindow`, a double press, restore non-topmost. **Why in-file:** the rig lives in a *session scratchpad that does not survive the session*. A release procedure depending on an ephemeral file is a forever-stop waiting to happen — and this lane has spent three tasks removing forever-stops. The mechanism is twenty lines; the dependency was the risk.

**⭐ THE CLICK POINT IS A MEASUREMENT WHOSE ERROR DIRECTION IS SAFE — please scrutinise this specifically.** Constants at **`:415-425`** (the `PKG-9f` block runs `:386-425`), derived from TASK-669's measured menu (handoff §6): a centred **7**-entry VBox, pitch **49.7 px at 720p**, 3rd entry ("Deck Builder") centred at client **(639, 310)**. With 7 entries the 4th sits on the window centre, so entry *n* is at `H/2 + (n-4)*pitch` — and that reproduces the measurement exactly (`360 - 49.7 = 310.3` vs measured 310). "Play (vs Bot)" is entry 1 ⇒ `yFrac = 0.5 + (1-4)*0.069028 = 0.292917`, `xFrac = 0.5`. Held as **fractions of the client rect** so it survives a resolution change. **If the point is wrong the game stays on its menu, the capture shows a menu, and the adjudicator judging the printed `PKG-§6a` bar records FAIL** ⇒ a mis-aimed click can only produce a **FAIL, never a false PASS**. The pre-click capture exists so that a miss is *diagnosable* rather than mysterious.

**The dry run and the live run print the same numbers from the same constants** (`$BOOT_MENU_PLAY_XFRAC`/`_YFRAC`, **`:424-425`**), because a plan that prints one number and executes another is exactly the `SHIP-§0` divergence this blocker was.

**The self-contradiction is gone:** the dry-run boot plan (**`:2273-2318`**) now *explains why the launch carries no map argument* instead of declaring the live line refuted.

### BLOCKER-3 — the spent verdict is retired and the state advances

| | |
|---|---|
| **Before** | The record sat at a stable path forever and state `boot` never left `ADJUDICATE` (the resume takes the `$reuse` branch; the state write lives in the cook branch). ⇒ Jonathan changes something and says "ship": HEAD is `H2`, the record still says `H1`, `C3-VERDICT-BINDING` STOPs — **the first invocation of every subsequent ship, until a human hand-deletes the file.** |
| **After** | QA option **(a)**, at **`:2361-2444`** (PASS) and **`:2089-2118`** (FAIL). Rule, stated once in the code: **a record is RETIRED when it has been READ TO A VERDICT; it is left in place when it could not be read.** |

- **PASS:** the record is **moved** to `<runlog>\ship-adjudication.<stamp>.consumed.json` and the state advances. **Exactly two fields change value** — `boot: ADJUDICATE → PASS` and `bootInstrument` gains the adjudication string — **plus one added field** `adjudication` (the archive path). **Every other field is carried forward verbatim, not recomputed**, because the reuse decision immediately above already proved `head`/`treeHash`/`config`/`recipeHash`/`schema`/`stageExeUtc` equal to this run's measured values ⇒ carrying is provably identical to re-deriving and cannot launder a difference.
- **FAIL:** the record is archived as `…consumed-FAIL.json` **before** the `C3-VERDICT` STOP, and **the state is deliberately NOT advanced** — it still reads `ADJUDICATE` with no record, so the reuse decision refuses and the fixed build is cooked and captured afresh. ⛔ A failed boot is never resumed onto. *(This branch is my extension of QA's fix; the defect generalises — leaving a FAIL armed reproduces the same chore one branch over as soon as HEAD moves. Declared as D2.)*
- **Malformed / non-binding:** ⛔ **left exactly where they are.** They could not be read to a verdict, so they must keep stopping until a human looks — **that is D2 and it is not reverted.**
- **⭐ Order is load-bearing and is commented as such (`:2426-2434`): ARCHIVE FIRST, ADVANCE THE STATE SECOND.** The two writes are not atomic. Archive-then-advance leaves, on failure, a record still armed beside a state still reading `ADJUDICATE` — exactly where we started, which the next run re-consumes idempotently. **The reverse order would leave `boot=PASS` beside an armed record, which STOPs at `C3-VERDICT-BINDING` with "not a pending adjudication" — the chore back again, produced by the repair meant to remove it.** The whole retirement is wrapped in try/catch; a failure is reported as a fact, never hidden, and the ship continues because the verdict was already proven.
- **Not a weakening:** `boot=PASS` is not trusted afterwards — it is re-guarded on every future run by the same nine measurements a log-route PASS is guarded by. The only way to reach the retirement is past `C3-VERDICT-FORM`, `C3-VERDICT-BINDING` and `C3-VERDICT`, all three of which throw on failure in a live run.

---

## 2. ⭐ THE FOURTH INSTANCE OF THE ROOT CAUSE, FOUND WHILE HUNTING — plus the four I cleared

I swept every use of `$ProjectName` and every hardcoded staged-binary name rather than fixing the three named instances.

| # | Site | Verdict |
|---|---|---|
| 1 | `:1801` `Get-Process -Name $ProjectName` | ⛔ **BROKEN — BLOCKER-1.** Fixed by path. |
| 2 | `:1325` `$StageExe = <stage>\<Project>.exe` (the **root shim** — launch target, `C2-STAGE-PRESENT`, and `D2-ZIP-READBACK`'s `$clickTarget`) | ⚠️ **FOURTH INSTANCE OF THE CLASS — GUESSED, THOUGH NOT PRESENTLY BROKEN.** 715 §4 measured the shim surviving a **Shipping** cook as `GitClaudeUnrealTest.exe` and named in `Manifest_NonUFSFiles_Win64.txt`, so it is *not* configuration-dependent today. But it was a **guess feeding three gates**, and `SHIP-§9a` says to prefer an identity that moves with the condition. ✅ **Now derived: `Get-StageShimExeRel` (`:1174-1210`)** selects the manifest's single **root-level** `.exe` — *by position (no `/` in the relative path), never by basename*, which is **TRAP 2 in reverse**: the 347 MB orphan shares that basename and lives one directory down. Convention fallback retained so it can never become a forever-stop of its own. Re-derived after the cook from the fresh manifests, exactly as the game exe is. |
| 3 | `:1790` `$defaultLog = <Project>\Saved\Logs\<Project>.log` | ✅ **CLEARED, and the clearance is commented in place (`:2536-2541`).** UE names the staged project folder and its log after the **project**, not the target — neither substitution is configuration-dependent. Under Shipping the file does not exist at all (`PKG-§9a-1`), which is why route Pixel never reads it. Route Log left byte-identical. |
| 4 | `:2049` `$savedDir = <Project>\Saved` | ✅ **CLEARED** — same reason (staged project folder, confirmed by 715's tree). |
| 5 | `:1367` `('{0}Editor' -f $ProjectName)` | ✅ **CLEARED** — the editor target genuinely *is* `<Project>Editor`; it is the CLAUDE.md build line and is configuration-independent. |

**⭐ And a fifth instance of the *class*, not of the name — relayed to me mid-task and now closed: `SHIP-§9b`.** `A6`'s pixel-instrument probe **succeeds on a locked desktop** (it captures the lock screen), so A6 cannot answer "can this desktop be driven?". That is the same shape as blocker 1 — an instrument correct in the ordinary case and false in exactly the case that matters. See §3.

---

## 3. NEW GATE `A8-DESKTOP` — the one addition to the gate inventory

`ship.ps1:1773-1823` (block header `:1773`, gate at **`:1812`**). **Built on TASK-716's measured finding and deliberately NOT on the two probes anyone would reach for.** 716 measured, on a provably locked session (12/12 samples over 4 min): `OpenInputDesktop()` returned **`"Default"`**, `SetCursorPos` **succeeded**, and screen capture **succeeded** — capturing the lock screen. The truthful signals were the **class/owner of the root window under the click point** and the pixels. `Test-DesktopLocked` (**`:884-951`**, comment + body) therefore uses the click rig's own predicate, `GetAncestor(WindowFromPoint(pt), GA_ROOT)`, matched against the constants at **`:427-441`** — class `LockScreenBackstopFrame`, owner `LockApp`|`LogonUI`.

**Three properties QA should check hardest:**
1. **It is in PHASE A, not at C3** — because at C3 the answer costs a 30-minute cook first (716 finding 3 asked for exactly this).
2. **It is a POSITIVE LOCK DETECTOR, not a proof-of-unlock.** It stops only when it can *name* a lock owner; *"could not measure"* is reported as a fact and **passes**. This is deliberate and commented: demanding proof of an unlocked desktop is how this lane would acquire **forever-stop number five** the first time Windows renamed a class. A missed lock still cannot manufacture a pass — the rig refuses to inject input it cannot land, and `C3-CAPTURE` records that the drive never happened.
3. **Under route Log it ASSERTS and says no desktop is needed** — ⛔ it never `Note-Skipped`s. That was A6's repaired shape and I did not reintroduce the silent-skip form.

---

## 4. GATE-ID INVENTORY, DIFFED BEFORE/AFTER (AST-extracted, as 712 did)

**Δ = exactly ONE addition. ZERO removals, ZERO renames, ZERO relaxations.**

| | Before (37) | After (38) |
|---|---|---|
| A | A1-UPROJECT · A1-REPO · A1-ENGINE · A1-STAGING · A2-NO-MID-OPERATION · A3-QUIET-MODULE · A4-FENCE · A5-DISK · A6-EVIDENCE-ROUTE · A7-COOKDIRS · A7-MAPS · A7-BOOTMAP | *(same)* **+ `A8-DESKTOP`** |
| B | B1-COMPILE · B2-SUITE | unchanged |
| C | C1-COOK · C2-UAT-LOG · C2-STAGE-PRESENT · C2-STAGE-MANIFESTS · C2-STAGE-PRUNE · C2-STAGE-ONE-EXE · C3-BOOT-ARENA · C3-BOOT-TITLE · C3-CAPTURE · C3-VERDICT-FORM · C3-VERDICT-BINDING · C3-VERDICT · C4-NO-MODELS | unchanged |
| D/F | D0-README · D1-ZIP · D2-ZIP-READBACK · D2-SIZE-SANITY · D3-PRUNE · F1-COMMIT · F1-COMMIT-PATHS · F2-COMMIT · F2-INDEX-CLEAN | unchanged |

**`-Ok` expressions: every one byte-identical except two.**
- `C2-STAGE-ONE-EXE`: `($runnable.Count -eq 1)` → `(($runnable.Count -eq 1) -and ($shim.Count -le 1))` — **QA NIT-3, declared**, strictly stronger. `-le 1` not `-eq 1`: a stage built without the bootstrap has zero shims and is not a hazard; *two* root exes is exactly the ambiguity the gate forbids.
- `A8-DESKTOP`: new.

**Param block: byte-identical, 9 parameters, none of them a skip.** Ban sweep (`SkipTests|SkipCook|SkipSuite|NoVerify|IgnoreGates|AcceptWarnings|FastShip|YesReally|PixelAdjudicated|AcceptPixel`) → **3 hits, all in the header's own prohibition list** (`:32`, `:33`, `:39`). `Note-Skipped 'A6…'` → **0**. `git add -A` / `git add .` / `git push` / `reset --hard` → **0**.

---

## 5. TRUTH TABLE — PROCESS RESOLUTION

`$P` = a process whose `.Path` is under `<stage>\Windows\`. Snapshot `$S` = PIDs matching before the launch.

| Scenario | Enumerated | Excluded | Title source | Kill loop | Result |
|---|---|---|---|---|---|
| **Shim + real exe** (Shipping, the standing case) | shim (`Title=''`) **and** `…-Win64-Shipping` (`Title='Siegebound'`) — **both**, because the filter is a path prefix | none | the real exe (last non-empty title wins; shim has none) | **both** killed, plus `$bp` | ✅ `C3-BOOT-TITLE` passes. **This is the case that was broken.** |
| **Shim only / same process** (bootstrap disabled, or shim *is* the game — Development) | the one process | none | that process | it, plus `$bp` | ✅ Works — the filter never assumed two processes |
| **Stale process from an earlier run** | matched by path… | …**then excluded by PID** (`$S` snapshot taken *before* the launch) | ⛔ cannot supply the title | ⛔ **not killed** — it may be Jonathan's | ✅ Reported as a fact; if it is the *only* stage process, `$gameProcs` is empty ⇒ `C3-CAPTURE` fails with "no live game process", which is correct |
| **Stale + fresh together** | both matched | stale excluded | fresh only | fresh only | ✅ Evidence is unambiguous |
| **Game crashed before the capture** | 0 | — | retains the menu-phase title | nothing to kill | ✅ `C3-BOOT-TITLE` may pass on the retained title, then `C3-CAPTURE` STOPs on "no live game process" — the mechanical stop names the real problem |
| **`.Path` unreadable** (protected/cross-bitness) | skipped by the per-process try/catch | — | — | — | ✅ ⛔ **Never `UNEXPECTED-ERROR`.** This is the guard I would attack first if I were reviewing |

## 6. TRUTH TABLE — RECORD LIFECYCLE

| State | `ship-adjudication.json` | state `boot` | What happens | Record afterwards |
|---|---|---|---|---|
| **Fresh** | absent | absent / not `ADJUDICATE` | cooks, captures, **SUSPENDS** at C3 → exit 4 | written by the caller |
| **Pending → PASS** | present, well-formed, binds, `PASS` | `ADJUDICATE`, pending capture matches | FORM ✓ BINDING ✓ VERDICT ✓ → reuse → **retire**: `boot → PASS`, `bootInstrument` = the adjudication | ⭐ **ARCHIVED** `…<stamp>.consumed.json` |
| **Pending → FAIL** | present, well-formed, binds, `FAIL` | `ADJUDICATE` | archived, then **STOP at `C3-VERDICT`** with the named reason. State **not** advanced ⇒ next run re-cooks | ⭐ **ARCHIVED** `…consumed-FAIL.json` |
| **Consumed** | absent (archived) | `PASS` | seam not entered; reuse proceeds on the state's `PASS`, re-guarded by all nine measurements | n/a |
| **Consumed, resurrected** (archive copied back) | present | `PASS` | ⛔ **STOP at `C3-VERDICT-BINDING`** — *"the state file records boot='PASS', not a pending adjudication"* | left in place |
| **Stale** (different HEAD/config/exe, or names an older capture) | present, binds to *something else* | `ADJUDICATE` | ⛔ **STOP at `C3-VERDICT-BINDING`**, remedy names the file — **D2, unchanged** | ⛔ **left in place** |
| **Malformed** | present, unreadable/hedged/bare | any | ⛔ **STOP at `C3-VERDICT-FORM`** | ⛔ **left in place** |
| **`-DryRun`** | any | any | reported as a fact, ⛔ **never consumed** (**`:1944`**, 714 D3 intact) | untouched |
| **Route Log** | any | any | block never entered (`$BootEvidence -eq 'Pixel'` gate) | untouched |

---

## 7. THE FIVE WARNS AND SEVEN NITS

| Finding | Disposition |
|---|---|
| **WARN-1** confident stale line on the dry-run no-route path | ✅ Fixed **`:2319-2332`** — an explicit `-not $A6Ok` branch that says **NONE**, matching the fact table |
| **WARN-2** resume contract omits the README precondition | ✅ Fixed in `Write-AdjudicationContract` (**`:654-666`**) + `ship.md` §2b.4 |
| **WARN-3** state file read with bare property access under StrictMode | ✅ Fixed — **every** state read now goes through `Get-JsonProp` (**`:2140-2172`**, `:2179`, `:2299`, and the reuse branch's `bootInstrument` read). A missing property yields `$null`, fails its check and re-runs B+C: the safe direction |
| **WARN-4** prune scope includes `Engine/` while the invariant excludes it | ✅ Comment added **`:1230-1239`** — the manifests are the authority *everywhere*; the invariant must exclude `Engine/` only because the `vc_redist` **installers** are manifest-listed non-game exes |
| **WARN-5** boot-verify cleanup can leave the game running | ✅ Fixed for free by BLOCKER-1's path resolution |
| **NIT-1** `$BUILD_RELEVANT_PREFIXES` is a contains-match | ✅ Renamed `…_FRAGMENTS` (**`:453`**, used `:1831`), with the reason ("a constant whose name lies…"). Behaviour unchanged |
| **NIT-2** silent convention fallback in the exe derivation | ✅ `$script:StageExeResolvedBy` (**`:1164/1167/1170`**) / `…ShimResolvedBy`, surfaced in the facts and the dry-run plan |
| **NIT-3** shim count only reported, not asserted | ✅ Asserted `-le 1` at **`:1293`** (see §4) |
| **NIT-4** `Move-Item -Force` silently overwrites a same-day zip | ✅ Detected and named as a fact + a line (**`:2977-2993`**) |
| **NIT-5** hedge list collisions | ⛔ **No change** — ruled ADOPTED UNCHANGED |
| **NIT-6** "PHASE D → E → F → G" misleads about what the script owns | ✅ Header RESUME block + the contract now read "PHASE D (zip) → F (commit) → G (report)" and say **PHASE E is the caller's** |
| **NIT-7** duplicated pixel probe | ⛔ **No refactor of A6's probe.** I *did* add `Save-ScreenCapture` because C3 now needs two captures; **A6's probe is untouched**, so the "only A6 moved" auditability 712 bought is intact |
| **QA §7.1** `PKG-§7a`'s two false factual claims | 📌 **Manager's, not mine.** `A4-FENCE` implements the *property* and measures its way to the right answer either way |

---

## 8. ⚠️⚠️ WHAT QA AND TASK-703 SHOULD SCRUTINISE — INCLUDING THINGS THAT WILL LOOK LIKE REGRESSIONS

1. **⛔⛔ THE CLICK ROUTE IS LAW-RULED AND RIG-SUPPORTED — IT IS ⛔ NOT YET OBSERVED END-TO-END ON A SHIPPING BINARY.** `PKG-§9f` rules the route; `t669_topclick`'s mechanism was proven against *this game's* menu at TASK-669. **But TASK-716 was dispatched to prove exactly this route by hand and returned `BLOCKED — desktop locked`; it never launched the exe.** ⇒ **The first `/ship` that reaches C3 under Pixel is also the first live exercise of this route.** ⛔ Do not read §1's *"After"* as measured. What **is** measured and load-bearing: 715's pruned stage is byte-exact and unaltered (70 files, 1,731,428,706 B, exe SHA-256 `A853A1E5…B8472`, one-exe invariant holding, both orphans absent — re-verified by 716 §5), so **the manifest derivation I must not revert is still valid against the real tree.**
2. **⚠️ `A8-DESKTOP` CHANGES TASK-703's EXPECTED DRY-RUN OUTPUT, and `qa/TASK-702.md` §6.1 predates it.** PHASE A runs for real in a dry run, so **on a locked machine a `-DryRun` now legitimately reports `DRYRUN-WOULD-STOP` naming `A8-DESKTOP`.** ⛔ **That is not a regression** — it is the gate telling you a Shipping ship is not schedulable right now, *before* a 30-minute cook is queued. Also amending §6.1: item 7's plan block now prints **two** resolved names (game binary **and** shim) plus their resolution authorities; item 11's expected WARN-1 stale line is **fixed and should NOT appear**.
3. **The click coordinate.** Re-derive it: `0.5 + (1 - 4) * (49.7/720) = 0.292917`. Check my reading of TASK-669 §6 — note the handoff's own prose formula (`H/2 − 2·49.7`) **disagrees with its own measured number** (639, 310); I took the **measurement** as authoritative (`360 − 49.7 = 310.3` ⇒ entry 3 is one pitch above centre ⇒ 7 entries put entry 4 on centre). If you read 669 differently, say so — this is the single softest number in the diff.
4. **Double press on a menu button.** `$BOOT_MENU_CLICKS = 2` matches the t669 rig's proven default (UE eats the first press often enough that 669 pinned two). The second press may land in-world after travel. I judged that harmless (no unit is selected, no card is held) and it preserves the proven lane — but it is a deliberate call, not an oversight.
5. **`Add-Type` compiles a P/Invoke shim.** It is **lazy** (`Initialize-ShipWin32`) so a route-Log run never pays for it, and it is wrapped: a compile failure yields a clean *"the click could not be attempted"* → `C3-CAPTURE` STOP, ⛔ never an `UNEXPECTED-ERROR` before PHASE A. **Under route Pixel a `-DryRun` will compile it** (via `A8`), writing a temp assembly to `%TEMP%` — same class as A6's existing `Add-Type -AssemblyName`, and ⛔ nothing is written to the project or the stage. Declared as D3.
6. **P/Invoke signatures.** Every one is copied verbatim from the two proven scratchpad scripts (`t716_probe3.ps1`, `t669_topclick.ps1`) **except `GetClientRect`**, which is new. ⛔ I did not execute them (the fence forbids it), so **they are reviewed, not run.**
7. **The pre-click menu capture cannot be adjudicated by accident** — and that is mechanical, not care: only `$pixelShot` is written into the state's `bootCapture`, and `SHIP-§8b` rule 9 rejects any record whose `capturePath` is not the state's pending capture. The filename also says so.
8. **`C3-CAPTURE` gained one pre-filter condition** (`:2744-2751`, "the click was not delivered"). It is fail-only and about whether the **drive happened**, ⛔ never about frame content — the same family as "the capture never happened". It STOPs *before* the state write, as `C3-BOOT-TITLE` and `C3-CAPTURE` already did; the systematic cause (a locked desktop) is caught in PHASE A by `A8` instead.
9. **Not fixed, and named rather than hidden:** a STOP at `C3-BOOT-TITLE` / `C3-CAPTURE` still discards a proven compile+suite+cook, because the state write remains at **`:2774`**. Making an instrument-failure re-*capture* without re-*cooking* is a **new capability**, not a blocker repair, and I judged it out of fence. Worth a follow-up task.

---

## 9. DECLARED DEVIATIONS (`SC-§15`)

| # | Deviation | Why |
|---|---|---|
| **D1** | The click rig is **reproduced in `ship.ps1`** rather than invoked as `t669_topclick.ps1` | The rig lives only in a session scratchpad, which does not survive the session. A release procedure that depends on an ephemeral file is a forever-stop waiting to happen — the exact class this task closes three times. Same mechanism, same abort predicate, same flags; twenty lines against an unbounded dependency. QA already accepted duplication over a shared dependency once, at 712 D3 |
| **D2** | Retirement extended to the **FAIL** branch (QA's fix names only "after a successful adjudicated resume") | The defect generalises: leaving a FAIL armed reproduces the same chore one branch over the moment HEAD moves. State deliberately **not** advanced on FAIL, so the fixed build is cooked and captured afresh — a failed boot is never resumed onto |
| **D3** | New gate **`A8-DESKTOP`** (the brief said "the three blockers, and nothing else") | Authorised by the mid-task relay of TASK-716, and required by BLOCKER-2's repair: a click route needs a drivable desktop, and `SHIP-§9b` forbids building that check on the two lying probes. Strictly stronger, positive-detector only, never `Note-Skipped` |
| **D4** | The **root shim** is now manifest-derived (`Get-StageShimExeRel`) | The fourth instance of the named root cause. Guessed, feeding three gates. Convention fallback retained. §2 row 2 |
| **D5** | `C2-STAGE-ONE-EXE`'s `-Ok` changed (`$shim.Count -le 1` added) | QA NIT-3, explicitly suggested, strictly stronger. It is the **only** pre-existing `-Ok` expression that moved |
| **D6** | New shared helper `Save-ScreenCapture` (C3 now takes two captures) | ⛔ **A6's probe is untouched** — NIT-7's refactor is not done. Only C3's own capture was factored, because it is now needed twice |
| **D7** | A **pre-click menu capture** is written that no spec asked for | A mis-aimed click and a broken build produce the *same* adjudicated frame; without this you cannot tell them apart. Diagnostic only, mechanically un-adjudicable, and named so in the filename |
| **D8** | Route `Log` deliberately keeps the map argument | `PKG-§9f` is a **Shipping** measurement; 699 measured the Development binary honouring the argument. Changing route 1 would break the byte-for-byte invariance QA verified at 714 |

⛔ **NOTHING IN QA §5 WAS TOUCHED:** 714's collateral manifest derivation **stands and was extended**, never reverted (the prune/derivation coupling is now stated in the code as binding) · **D2 stands** — a stale record still STOPs · **`$ADJ_HEDGES` is byte-identical** · no flag of any kind · no gate relaxed · `PKG-§6a` untouched · the 11 verdict rules, the 15-row resume table and both `PKG-§10` trap guards are unmodified · no `.Target.cs` or ini edit.
