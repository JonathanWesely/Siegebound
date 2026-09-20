# TASK-1337 — HEADER-GUARD-FAIL-CLOSED-HOST — build-master handoff

- **subject:** `TASK-1335` (ONLY) · **gate:** `TASK-1336` `Verdict:` = **PASS**, 0 BLOCKER / 3 WARN / 6 NIT
- **author:** build-master, 2026-09-20
- **commit:** **`b55f622`** · parent `ebc5bc4` · NOT amended · 6 files · **NEVER pushed**
- **`main`:** 13 → **14 ahead** · `origin/main` still `d818b5e` (re-verified AFTER the commit)
- **law named:** ⭐ **`SC-§129`** (see §2 — it applies to *me*, and the key is **touches**, never *changes code*)

---

## 0. The two legs that were declared, not omitted

- ⛔ **NO C++ COMPILE.** The diff is PowerShell inside `Tools/`. There is nothing for UBT to build; a
  compile here would have produced a `Result: Succeeded` about an empty build set — the exact shape of
  evidence `UE-§ exit-code-lies` warns is indistinguishable from success.
- ⛔ **NO 5b.** `TASK-1335` carries **no runtime acceptance criterion**, so `VER-§5` cl. 2 routes straight
  to 5c. This is **not** an `UNOBSERVABLE` and is never read as a pass.

---

## 1. The hazard moved a FOURTH time — DERIVED, never inherited

The final `^#>` has now been `:1061` → `:1065` (`TASK-1329`) → `:1145` (`TASK-1333`) → and at my instant:

```
grep -n '^#>' Tools/run_suite_bounded.ps1 | tail
  897:#>   964:#>   1035:#>   1065:#>   1168:#>
```

- **final `#>` = `:1168`** (derived by me; `:1065` and `:1145` are hearsay and were treated as such)
- **pair count = 14** — `^<#` = **14**, `^#>` = **14**. ⛔ A count of **13** would have meant the
  pre-`TASK-1331` file and an immediate STOP. **It did not occur.**
- raw `<#` anywhere = **15**; the 15th is at `:1183`, inside
  `$t.Text.StartsWith('<#')` — a **string literal**, not a delimiter.

### The control's FIRING behaviour (not merely a clean parse)

Three scratchpad copies, `[Parser]::ParseFile` on each. **`SHIP-§9`: a gate is validated against the
failure it must detect, never against success.**

| control | broke | errors | reading |
|---|---|---|---|
| BASELINE | nothing | **0** | the tracked file parses |
| **A — CORRECT** | the **derived** `:1168` | **1** | *"The terminator `'#>'` is missing from the multiline comment."* reported at the **opener `:1089`** ⇒ **THE CONTROL FIRES** |
| **B — HAZARD** | the inherited `:1145` | **0** | **SILENT** |
| **C — HAZARD** | the inherited `:1065` | **0** | **SILENT** |

⭐⭐ **And B is now worse than `TASK-1333` measured it.** `:1145` is **no longer a delimiter at all** — at
my instant that line reads

```
    failure this case can emit is capable of naming those two addresses UNDER ANY INPUT,
```

— **prose inside the doc comment.** Breaking it edits a comment's interior, so the file parses
**identically**. `TASK-1333`'s version of this trap at least re-paired against a later `<#`; mine does not
even reach the parser. **A control built on an inherited address would have reported "clean" while being
structurally incapable of failing.**

⛔ Scratchpad copies **only**, all three **deleted**. Tracked whole-file sha256
`E6EA8B7C87670AB52D57F6092A5BC5D93047A8E05F21CE790C4871C9C7F7A0C5` **BEFORE = AFTER**, and **again
after the suite ran through it** (the file *is* the runner).

---

## 2. ⭐ `SC-§129` — I NAME IT, because my diff TOUCHES this file

The trigger is **touches**, never *changes code*. My commit carries `run_suite_bounded.ps1`, so the
`-SelfTest` ledger is owed **whole and BY NAME** (`SC-§129` cl. 3(b) · `SC-§104`), never as a tally.

**55 / 55**, re-derived — 11 fixture + 5 bound + 12 lane + 5 echo + 4 cmdline + 2 Aura + 2 merge +
7 `-LogPath` + 7 input = **55**, in the order `TASK-1336` published.

**§1 fixture corpus (11):** `green-suite.log` · `red-suite.log` · `skipped-suite.log` ·
`zero-started.log` · `zero-started-filtered.log` · `result-absent.log` · `count-mismatch.log` ·
`w9-cmd-semicolon.log` · `green-commands.log` · `no-terminator-echo.log` · `__does_not_exist__.log`
**§2 bound arithmetic (5):** overall bound tripped · boot bound: command never echoed · stall bound: log
stopped growing · healthy run is NOT killed · slow boot inside the bound survives
**§3 lane construction (12):** suite value is `'Automation RunTests Siegebound;Quit'` · suite value
honours a sub-group filter · command value is `'A,B,QUIT_EDITOR'` · a caller-supplied `'Quit'` is DROPPED,
QUIT_EDITOR appended · caller separator `';'` REFUSED (exit 64) · caller separator `','` REFUSED (64) ·
empty command list REFUSED (64) · only-terminators list REFUSED (64) · W-4 `-Filter` with `';'` REFUSED ·
W-4 `-Filter` with `','` REFUSED · W-4 `-Filter` with a quote REFUSED · W-4 empty `-Filter` REFUSED
**§4 expected echoes (5):** suite expects exactly 1 echo without `";Quit"` · B-1 command lane requires 2
echoes and NOT the terminator · symmetry: `Get-ExpectedEchoes` refuses `';'` · symmetry: refuses `','` ·
symmetry: refuses a mangled `-Filter`
**§5 command line (4):** `New-EditorCommandLine` returns `[string]` · `-ExecCmds=…` survives verbatim ·
paths quoted / `-nullrhi`+`-unattended` present · command lane emits the COMMA form with QUIT_EDITOR
**§5b Aura exclusion (2):** `-DisablePlugins=Aura` verbatim in BOTH lanes · the flag guard REFUSES 4
degenerate lines, still accepts the real one
**§6 bound merge (2):** B-2 green log + tripped bound → exit 6, NOT 0 · no bound tripped → log verdict
untouched
**§7 `-LogPath` safety (7):** `Saved\Logs\run.log` allowed · absolute path inside `Saved` allowed ·
`.uasset` REFUSED · non-`.log` extension REFUSED · outside the project REFUSED · traversal out of `Saved`
REFUSED · empty path REFUSED
**§8 input tolerance (7):** `Get-CmdEchoes` survives BLANK LINES · survives an EMPTY array ·
`Test-SuiteCounts` survives BLANK LINES and still counts · zero echoes is NOT a pass · B-1 terminator echo
is corroboration only · W-10 incremental reader advances and never re-reads ·
**`SC-126: header grows no rot-prone line address` — LAST and GREEN**, detail **verbatim as predicted**:
`header derived as lines 1..238; 6 exhibit line(s) excluded by substring, never by address`

⇒ **no name missing, no detail differing** — the equal-line-count rewrite did not show itself here either.

---

## 3. 🚨 `TASK-1336`'s §5 DECLARED RESIDUAL — CLOSED, with the only instrument that could

The gate executed **nothing** and said so. Its residual is an **equal-line-count rewrite below the
header**, and it correctly warned that `git show --stat HEAD` **lists files, not hunks** and therefore
*cannot* close it. Run at my instant:

```
@@ -1110,0 +1111,13 @@ function Add-ThrowCase {
@@ -1130,4 +1143,7  @@ function Add-ThrowCase {
@@ -1137,8 +1153,15 @@ function Add-ThrowCase {
@@ -1152,0 +1176,5  @@ function Get-HeaderRotProneAddress {
@@ -1179,0 +1208,4  @@ function Get-HeaderRotProneAddress {
@@ -1183   +1215    @@ function Get-HeaderRotProneAddress {
```

**Lowest hunk `+1111` (old side `-1110`) ⇒ ZERO hunks at or above `:238`.** Required ≥ `1108`. ✅

**Check (5) — header BYTE-IDENTICAL, verified at my instant, three ways.** 14741 bytes through the 238th
LF, sha256 **`072F55CEB8BC4AD189FCACDBABF5E97C6E912028150C3891E19EF55F08593A2E`** on **`8cbd5d1`**, on
**`ebc5bc4`**, and on the **working copy**. ⭐ The seventh hand-edit of that header — the one that would
have ended this chain by proving it never worked — **did not happen.**

**WARN-3, free while the diff was open:** deletion side = **13** = **12 doc-comment prose lines**
(including one blank) + **1** shape-(a) line `Rx = '(?:CONVENTIONS|TASKBOARD)\.md(?:\s+at)?\s*:\s*\d+' },`.
**No deleted line is unaccounted for.** `--numstat` on the file: **45 / 13**.

⛔ **The 3 WARN / 6 NIT are NOT mine and I did not touch one of them.** Every one is prose ⇒ programmer
work on a future row if the manager boards it. A host "just fixing" them is the seventh hand-edit wearing
a build-master's hat.

---

## 4. The suite — 561/561, delta 0 BY NAME

PID 26316, boot observed at 12 s, wall clock 55 s, `RUNNER_EXIT` **0**, dispatch echo 1/1.
N 561 / completed 561 / M **0** / discovered 561. `Result={Success}` **561**, `Result={Fail}` **0**.

Reconciled **BY NAME**, never by tally (`SC-§104`): `Path={…}` extracted from the baseline log
(`run_suite_bounded_suite_20260920-015622.log`, `TASK-1333`'s own pass) and from mine, sorted, `comm`-diffed
⇒ **added set EMPTY, removed set EMPTY**. Red list **EMPTY** (zero non-`Success` completions).

**The triple by SIGNATURE:**

| probe | mine | baseline |
|---|---|---|
| `HTTP 401` | **0** | 0 |
| `401 Unauthorized` | **0** | 0 |
| `Unauthorized` (-i) | **0** | 0 |
| `LogAura` | **0** | 0 |
| **bare digit `401`** | **8** ⛔ | 7 ⛔ |

🚨 **The bare-digit grep lied again: 8 where the signature truth is 0. That is TEN FOR TEN.**

---

## 5. Pathspec — derived at my own instant (`TL-§5e` cl. 7a/7a-v/7b)

Git root is **one level up** (`SC-§102`); every anchor proven with `git ls-files --error-unmatch` (tracked)
or `-o --exclude-standard` (untracked) **before** staging.

- `Tools/run_suite_bounded.ps1` — tracked, M
- `TASKBOARD.md` — tracked, M (carrying the prior hosts' `TASK-1319`/`1320`/`1321` flips, swept knowingly)
- `handoffs/TASK-1335-programmer.md` · `qa/TASK-1336-report.md` — named in my row's pathspec
- ⛔ **`CONVENTIONS.md` RE-MEASURED at my instant ⇒ CLEAN ⇒ cl. 7b DID NOT APPLY.** I did not inherit the
  last three hosts' reading.
- ✅ **cl. 7a ORPHAN SWEEP EXECUTED — both measured at `git log --all` = 0 commits, not assumed:**
  - **`handoffs/TASK-1333-buildmaster.md`** — `TASK-1333`'s own commit `8cbd5d1` landed without it (the
    bounded-at-one tail). `TASK-1321` **held it for me** precisely because my row carries a sweep clause
    and its own pathspec was closed-enumerated. **Swept. The tail is closed.**
  - **`handoffs/TASK-1321-buildmaster.md`** — fresh orphan from `ebc5bc4`, which carries its hash and
    cannot contain it. **Swept.**

**Fences held, probed TWICE — before staging and again inside the commit:** `SiegePlayerController.cpp`
**CLEAN** (the gate's "dirty right now" warning was a stale instant; `ebc5bc4` landed it) · `.uasset`
**0** · `qa/TASK-1314-verify.md` **CLEAN**. The commit contains **0 `.cpp`, 0 `.uasset`, 0 `Source/`**.
**Index was CLEAN before I staged** ⇒ the UE Git plugin did not autostage (editor was down). Verified the
**COMMIT**, never the index. Committed with `-F <file> -- <paths>`; untracked paths `git add`-ed
individually; ⛔ never `-a`, never `-A`, never `.`, never a bare directory; ⛔ **never pushed**.

`git show --numstat HEAD`:

```
3    3    .claude/pipeline/TASKBOARD.md
274  0    .claude/pipeline/handoffs/TASK-1321-buildmaster.md
280  0    .claude/pipeline/handoffs/TASK-1333-buildmaster.md
428  0    .claude/pipeline/handoffs/TASK-1335-programmer.md
497  0    .claude/pipeline/qa/TASK-1336-report.md
45   13   Tools/run_suite_bounded.ps1
```

The `LF will be replaced by CRLF` warning on the `.ps1` is **pre-existing** (`TASK-1324`/`1327`/`1331` all
recorded it), not from this diff.

---

## 6. Flips — three, `Edit` only, collisions RE-MEASURED

Committed **before** editing the board. Single `Edit`s on **task-ID-bearing anchors**, ⛔ never
`replace_all` (`SC-§127`), each read back as **STATE** (`SC-§104`), with the old state probed to **0**.

⛔ **Collision counts measured at MY instant, not inherited** (tonight's recorded values span 1, 2, 4, 5, 6):

| anchor | collisions |
|---|---|
| `status: qa-passed — **⭐ \`TASK-1335\` GATED` | **1** |
| `status: qa-passed — **⭐ \`TASK-1336\` COMPLETE` | **1** |
| ``BOARDED, ⛔ NOT DISPATCHED (⭐ `TASK-1337`)`` | **1** |
| ⛔ **bare** `status: backlog — **BOARDED, ⛔ NOT DISPATCHED` | **2** |
| ⛔ **bare** `status: qa-passed` | **37** |

⇒ the `TASK-1337:` discriminator the row told me not to remove was **load-bearing, measured**.

- `TASK-1335` → **`done`** · `TASK-1336` → **`done`** · `TASK-1337` → **`done`**

---

## 7. Environment

Editor censused **by command line** at my instant (`SC-§118` cl. 8): **PID 20140**, GUI
`UnrealEditor.exe` on the `.uproject`, **`-game` ABSENT** ⇒ **not Jonathan's**. Closed under the standing
grant for the suite run, post-close census **zero** `UnrealEditor` processes. **Left DOWN** — no C++
changed, so no relaunch on new binaries is owed. 🧑 Jonathan is asleep; nothing of his was touched.

One pre-existing scratchpad file (`t1331-controls.ps1`, from `TASK-1331`) is untracked and not mine; left
in place.

---

## 8. LEFT DIRTY BY DESIGN — and nothing is boarded to sweep it

These **3 flips** + **this handoff** carry `b55f622` and cannot live inside it.

🚨 **`TASK-1337` is the LAST BOARDED HOST.** Unlike every previous link, there is **no successor row** with
a cl. 7a sweep clause to pick them up. ⇒ **the manager must board one, or fold them into the next
commit** — otherwise `handoffs/TASK-1337-buildmaster.md` becomes a permanent orphan. Naming it here
because a silent leave is indistinguishable from an oversight.

**Nothing else started. Handing back.**
