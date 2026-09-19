# QA Report — TASK-1317
Verdict: FAIL — 2 BLOCKERS, 3 WARN, 3 NIT

subject: TASK-1310
gate: TASK-1317 (delta-gate) · host: TASK-1318 · reviewer: qa-reviewer · 2026-09-19

---

## 0. READ THIS FIRST — THE ROW'S CENTRAL JUDGEMENT IS UPHELD, AND IT MUST NOT BE "FIXED"

**`TASK-1310` was RIGHT to refuse its own acceptance (3), and this gate affirms that in full.**
The claim it was ordered to preserve — *"NO BOUND HAS EVER KILLED A REAL PROCESS"* — is **FALSE**,
and I re-measured it myself rather than inheriting it from the row, the board, the manager or the
orchestrator (`SC-§119`). The three-way split it wrote instead (`OVERALL` = MEASURED · `BOOT` +
`STALL` = `NOT MEASURED — NULL WITH NO POSITIVE CONTROL`) is **the correct disposition** and is
present in the file in the required words.

⇒ **Neither BLOCKER below touches the split.** Both are factual defects in the *supporting* text of
the same block, and **both are repaired by one edit to one file**. A fix that changes the split, the
verdicts, or the `NOT MEASURED` wording would be a regression, not a repair.

---

## 1. (4-bis) THE KILL EVIDENCE — RE-MEASURED, NOT INHERITED. **IT REPRODUCES, ALL FOUR READS.**

Taken first, exactly as the row asked (handoff §9 item 1) and as check (4-bis) orders, because every
other verdict rests on it.

| reading | source | measured by me | handoff's claim | agree |
|---|---|---|---|---|
| bound-trip line | `handoffs/TASK-1183-buildmaster.md:178` | `**Result: `BOUND TRIPPED: OVERALL bound 20s exceeded (elapsed 21s)`**` | same | ✅ |
| `RUNNER_EXIT` | `:183` | **6** (and `$LASTEXITCODE` **6** at `:182`, agreeing) | same | ✅ |
| PID gone three ways | `:196-199` | **PID 13816** — script probe `HasExited`; `Win32_Process` returns **nothing**; fresh name census **absent** | same | ✅ |
| **positive control fired** | `:198` | *"the same query on my own PID returned my process, so the reader worked"* — **the control is real and it fired** | same | ✅ |
| corpse opens | `…suite_20260909-045537.log:1` | `Log file open, 09/09/26 04:55:38` | same | ✅ |
| corpse last line | `:3977` (file is **3977** lines) | `[2026.09.09-11.55.58:394][661]LogAutomationWorker: Received RunTests TheGrownHeightIsWholePixels…` ⇒ **04:55:58 = exactly 20 s** | same | ✅ |
| stops mid-test | `:3976-3977` | `Test Started` for `TheGrownHeightIsWholePixels…` at `:3976`, **no matching `Test Completed`** | same | ✅ |
| started / completed | grep | **134 / 133** | 134 / 133 | ✅ |
| crash markers | grep `Fatal error\|Critical error\|LogWindows: Error` | **0** | 0 | ✅ |
| clean neighbour | `…suite_20260909-045412.log` | **555 started / 555 completed** (both counted) | 555/555 | ✅ |

**Two corroborations the handoff did not claim, added because they close the chain end-to-end:**

1. **The sidecar ties the kill to *this script* and to *the real editor*.** `…045537.log.cmdline`
   contains, in one line:
   `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "…GitClaudeUnrealTest.uproject" -ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi … -abslog="…\run_suite_bounded_suite_20260909-045537.log"`
   ⇒ the receipt's `-abslog` names **the exact truncated log**. Script → real `UnrealEditor-Cmd.exe`
   → that corpse. Not inferred from a filename.
2. **The (c2) self-termination tally reproduces exactly.** 21 suite logs; **19** contain
   `LogCore: Engine exit requested (reason: Win RequestExit)` (measured by file census). The two that
   do not are precisely `…045537` (the kill) and `…211827`. `19 + 1 (211827) + 1 (command log) = 21`
   self-terminated, **22nd is the kill** — the comment's arithmetic at `:120-124` is sound.

⇒ ✅ **The kill evidence is correct. The comment's (c1) does not ship a new false sentence.**
⇒ ✅ **`OVERALL` = MEASURED is earned, not asserted.**

---

## 2. THE CHECKS, IN ORDER

### (1) Zero executable lines — re-derived, with the provenance of each half stated

| fact | how I know it |
|---|---|
| header block spans **`1` (`<#`) → `195` (`#>`)** | **MEASURED** — my own delimiter grep |
| `param()` opens at **`:198`** | **MEASURED** — read |
| file is **1782** lines | **MEASURED** — line count |
| changed lines = **6-9, 62, 64-139**; `git diff --numstat` = **1 file, 81+/11−** | **ACCEPTED AS DECLARED** (`SC-§71b` — QA holds no `Bash`) → **TASK-1318** re-measures at `git show --stat HEAD` |

**An independent structural corroboration that the body was not touched** (this is stronger than
re-reading the handoff's own number, and it is what exposed BLOCKER-1):
`qa/TASK-1295-report.md:269-270` records the **pre-edit** addresses `Set-Content …'.cmdline'` = `:1574`
and `Start-Process` = `:1577`. I measure them **post-edit** at **`:1644`** and **`:1647`**.
**Displacement = exactly +70 for both**, and `81 − 11 = +70`, and `1712 + 70 = 1782` (measured).
⇒ the entire body below the header is displaced by *precisely* the header's net growth — the
signature of a comment-only diff. ✅

### (2) 🚨 BLOCK-COMMENT INTEGRITY — delimiters counted on BOTH sides

| side | `<#` | `#>` | basis |
|---|---|---|---|
| **POST (current file)** | **13** | **13** | **MEASURED** — grep, all 26 lines listed below |
| **PRE (pre-edit file)** | **13** | **13** | **DERIVED** — see reasoning |

POST, measured, strictly alternating open→close with **no nesting and no unbalanced delimiter**:
`1→195`, `270→279`, `329→339`, `362→375`, `477→482`, `543→551`, `606→610`, `633→646`, `754→765`,
`848→854`, `908→921`, `989→992`, `1014→1022`.

**Why PRE is also 13/13, derived rather than assumed:** the decisive fact is that **no delimiter line
falls inside the changed range on either side.** POST changed range is `6-139`; my grep shows the
only delimiters in `1…195` are at `:1` and `:195`, both **outside** it. PRE changed range is
`6-7, 60, 62-69`; the handoff quotes **all 11 deleted lines verbatim** (§2a), the quote's length
(2 + 9) **matches the numstat deletion count of 11**, and **none of the 11 contains `<#` or `#>`**.
⇒ the diff neither adds nor removes a delimiter ⇒ the census is identical on both sides, and the
header's own `<#`/`#>` pair is untouched. ✅ **No "comment-only" escape hatch: nothing became code,
no code was swallowed.**

⚠️ This is a **delimiter count**, not a parse. **`TASK-1318` parses the file with a deliberately
broken `#>` as its positive control.** Neither substitutes for the other, as the spec says.

### (3) + (4-ter) THE CENSUS — reconciled, and my number agrees

**My own glob, at my instant:** **22** `.cmdline` sidecars — **21 suite + 1 command**
(`run_suite_bounded_command_20260909-045326.log.cmdline`) — spanning **`20260909-045216` →
`20260918-234424`**. **Four dated 2026-09-09** (`suite_045216`, `command_045326`, `suite_045412`,
`suite_045537`). I also counted the `.log` files: **22**, one per sidecar.

The row's reconciliation checks out arithmetically against my list:
`15` (09-14 window: **9** on 09-14 + **4** on 09-17 + **2** on 09-18 16:xx — I counted each) `+ 4`
(the 09-09 runs) `= 19` `+ 3` (`…234245`, `…234334`, `…234424`, all post-dating the manager's
09-18 ~16:xx glob) `= 22`. ✅ **Nobody was wrong; each count was right at its instant (`SC-§91`).**
Per (4-ter), a different number **with** a reconciliation is correct and is **not** failed.

### (4) THE SPLIT — all three parts required, all three examined

| part | required | measured | verdict |
|---|---|---|---|
| **(a)** `OVERALL` = **MEASURED**, `TASK-1183` evidence cited in the comment | yes | `:105-115` — names `TASK-1183` §6, `-OverallSeconds 20` vs a ~44 s suite, the printed trip line, `RUNNER_EXIT = 6`, PID 13816 gone three ways, the corpse log with 134/133, the clean neighbour | ✅ **PASS** |
| **(b)** `BOOT` + `STALL` carry `NOT MEASURED — NULL WITH NO POSITIVE CONTROL` **verbatim** at `:117` and `:129-130` | yes | **re-grepped (`SC-§91`), both present, both with U+2014 em dash exactly as `SC-§107` cl. 4 and the law write it.** `:117` = `(c2) The BOOT and STALL bounds are NOT MEASURED — NULL WITH NO POSITIVE CONTROL.`; `:129-130` = the law quoted inside the divergence notice | ✅ **PASS** |
| **(c)** code-vs-law divergence **declared inside** the comment at `:127-134` | yes | **present at `:127-134`** — but **no longer accurate**, see **BLOCKER-2** | ⛔ **present, inaccurate** |
| **not** a wholesale `NOT MEASURED` over all three bounds | required absent | `(c1)`/`(c2)` split is explicit; the false sentence is **not** re-asserted | ✅ **PASS** |

### (5) FENCES — each measured against the current file

| fence | verdict | basis |
|---|---|---|
| parameter default changed | ✅ none | `param()` at `:198`, outside the changed range |
| any bound changed | ✅ none | `-OverallSeconds 1500` / `-BootSeconds 420` / `-StallSeconds 180` at `:210-212` — **identical to `TL-§6`'s proven `1500 / 420 / 180`** (`CONVENTIONS.md:6177`), and outside the range |
| `-ExecCmds` composition | ✅ untouched | `New-EditorCommandLine` at `:393` |
| comma-vs-semicolon separator | ✅ untouched | the two-lane explanation sits at `:23-38`, which falls **between** the two changed sub-ranges (`6-9` and `62`/`64-139`) and is unchanged; the executable composition lives at `:393` |
| `QUIT_EDITOR` terminator (`SC-§116`) | ✅ untouched | `:393`+ |
| `#19` orphan-census logic | ✅ untouched | lives below `:1700`; nothing in `1-195` mentions it |
| `Saved/**` in the diff | ✅ absent | sidecars/logs **read only** — I opened 6 of them and deleted nothing; count is 22, i.e. it **grew**, never shrank |
| `TASKBOARD.md` / `CONVENTIONS.md` in **this row's** diff | ✅ absent | both are dirty in the tree, both belong to **the manager** (the `SC-§82` strike) and **`TASK-1311`** — **expected, and the row NAMED them rather than implying ownership (§7)**. ⛔ Not a finding against `TASK-1310`. |
| files from `TASK-1314` / `TASK-1319`'s diff | ✅ zero | this diff is one `.ps1`; `SiegePlayerController.cpp` and `WBP_VictoryScreen.uasset` are not in it |
| files staged | ✅ none declared | **ACCEPTED AS DECLARED** → `TASK-1318` stages by pathspec |

### (6) ACCEPTED AS DECLARED (`SC-§71b`) — named, one by one

QA holds **no `Bash`**. I accept, and **`TASK-1318` is the host that measures them**:
`git diff --numstat` (1 file / 81+/11−) · the hunk headers and therefore the exact changed-line set ·
`git status --porcelain` · *nothing staged* · `Parser::ParseFile` = 0 errors · the file's UTF-8
encoding and absence of a BOM. **`TASK-1318` parses with a deliberately-broken positive control and
then runs one real suite through the edited runner** — that pairing, not my delimiter count, is what
turns "touched no code" into a measurement.

**And the thing this diff cannot be checked for at all:** whether the new comment text is *true* is a
claim about **22 sidecars, four handoffs and a law file** — not about the code. I verified the
**reconciliation and the citations**; no review of a PowerShell diff can verify that the evidence
base itself is complete.

**Inspector:** not used — this row has no asset, graph or editor state. Nothing here rests on the
editor being up.

---

## 3. Findings

- **[BLOCKER-1]** `Tools/run_suite_bounded.ps1:81-82` — **both line citations in clause (a) are wrong
  by one, and they were produced by arithmetic, not by a read.** The text asserts
  *"`Start-Process` at `:1646`"* and *"the `.cmdline` sidecar written at `:1643`"*.
  **Measured, two tools agreeing:** `:1644` is `Set-Content -LiteralPath ($LogPath + '.cmdline') …`
  and **`:1647`** is `$proc = Start-Process -FilePath $EditorCmd …`. `:1646` is `Write-Head 'LAUNCH'`
  and `:1643` is a comment line. **Root cause is visible in the arithmetic:** the pre-edit addresses
  (`:1574` / `:1577`, `qa/TASK-1295-report.md:269-270`) are right, the net insertion is **+70**
  (`81 − 11`, and `1712 + 70 = 1782` measured), but the handoff applied **+69** — §7(a) and §9 item 4
  both report these as *"measured at my instant"* / *"re-derived today"*, and for these two they were
  **derived**. ⛔ **Why this is a BLOCKER and not a NIT:** this is the block whose thesis is *"a
  sentence asserted true because nobody opened the evidence it names"*, and the sentence names an
  address nobody opened. It is also `SC-§104`/`SC-§95` exactly — a number from arithmetic where an
  execution was claimed, which is the combination the law calls most dangerous *because the delta
  checks out*. **Fix:** `:1646 → :1647`, `:1643 → :1644`. Two characters. The `grep -n
  "Start-Process -FilePath"` escape hatch already beside them is good and should stay.

- **[BLOCKER-2]** `Tools/run_suite_bounded.ps1:127-134` — **the divergence paragraph is now false: the
  law it says is unstruck WAS STRUCK, hours after the row wrote it.** The text reads *"TL-6's NOT
  MEASURED bullet … **still reads, unstruck**, 'NO BOUND HAS EVER KILLED A REAL PROCESS …'"*.
  **Measured now:** `CONVENTIONS.md:6185` carries both sentences inside `~~…~~`; `:6186` declares
  *"BOTH OF THOSE SENTENCES ARE STRUCK 2026-09-19 BY THE MANAGER (`SC-§82`) … AS FALSE, NOT AS
  SUPERSEDED"*; `:6187` is *"FALSE SENTENCE 1 … FALSE SINCE 2026-09-09"*; `:6188` corrects *"every one
  of those 19 runs terminated normally"* to **21 of 22**; `:6189` publishes the **same three-way
  split this comment writes** and cites `run_suite_bounded.ps1:117`, `:129-130`, `:127-134` by name.
  ⇒ **the divergence is RESOLVED; the law and the code now agree**, and the only thing left disagreeing
  is the paragraph that says they disagree.
  ⚖️ **This was NOT the row's error at its instant — the row did the right thing** (`SC-§82` reserves
  the strike; declaring the divergence in code was correct, and `:6189` shows the manager adopting the
  row's own structure). But **the artefact cannot be committed asserting it**: `TASK-1318` would put a
  false sentence about a struck law into git, in the block that exists to stop exactly that, on the
  same day — the pattern `CONVENTIONS.md:6191` records and `SC-§126` was minted to end. **Fix
  (non-binding, `SC-§101`):** replace the paragraph with the *resolution* — both sentences struck
  2026-09-19 by the manager under `SC-§82`, the law now published split at `TL-§6`, this block is its
  code-side half. **Address it by a stable anchor, not a line number** (e.g. *grep `FALSE SENTENCE 1`
  under `TL-§6`'s `NOT MEASURED` bullet) — see WARN-1 for why.

- **[WARN-1]** `Tools/run_suite_bounded.ps1:128` — the citation *"`CONVENTIONS.md:6172-6175`; surviving
  half at `:6174`"` **no longer resolves**: `:6174` is now a **blank line** and the bullet sits at
  `:6183-6193`. ⛔ **This is not the row's fault and must not be scored as one** — the manager minted
  **`SC-§126` at `CONVENTIONS.md:4449`** in the same sitting, which pushed `TL-§6` down by ~11 lines.
  The row's number was right at its instant (`SC-§91`), and the row *predicted this rot itself* (§9
  item 4). It is a WARN only because BLOCKER-2's rewrite is already opening these lines, so **fix it
  in the same edit and anchor it to text rather than to a line**, or the same rot recurs on the next
  law edit — the old block's dead `:5966-5969` is the precedent.

- **[WARN-2]** *(finding for the manager, homed by name so it is not an `SC-§50` orphan — not
  `TASK-1310`'s to fix, and `CONVENTIONS.md` is fenced from it)* — **`CONVENTIONS.md:6190`'s census
  date-range start is wrong.** It reads `20260909-045326 → 20260918-234424`; `…045326` is the
  **command-lane** sidecar, and the **earliest sidecar is `20260909-045216`**. The code comment at
  `:90` has it **right** (`20260909-045216 to 20260918-234424`), as does the board's `TASK-1310`
  status line. ⇒ the law is the copy that is off, by one file, in the same direction as everything
  else this row has been unpicking.

- **[WARN-3]** *(observation, no owner yet — named so it is not later mistaken for a second kill)* —
  `Saved/Logs/run_suite_bounded_suite_20260917-211827.log` holds **559 started / 558 completed**, a
  one-test gap with the same surface shape as a truncation. **It is NOT a kill, and I measured that
  rather than assuming it:** the log's last line is `Sending StopTestSession` — emitted *after* the
  final test's `Test Completed` + `BeginEvents` + `EndEvents` (`:7398-7401`), i.e. the suite reached
  its end; **0 crash markers**; the run lasted ~52 s against an `-OverallSeconds` default of **1500**.
  The comment's characterisation (*"inside the same orderly shutdown"*) is therefore **correct**. The
  missing `Test Completed` is somewhere mid-run and predates this row entirely.

- **[NIT-1]** `:122` — *"ends two lines earlier inside the same orderly shutdown"*. Measured against
  the clean neighbour, `Sending StopTestSession` is followed by **four** more lines
  (`Received StopTestSession` → `Shutting down` → `TEST COMPLETE. EXIT CODE: 0` →
  `RequestExitWithStatus`) before the exit-requested line. *"Two"* understates it. The load-bearing
  half — **orderly, not a kill** — is correct and measured (WARN-3). Fix only if `:120-125` is being
  edited anyway; *"a few lines earlier"* removes the number.

- **[NIT-2]** `:117`, `:129` — two **U+2014** em dashes in a `.ps1` the handoff declares UTF-8
  **without BOM**. Windows PowerShell 5.1 decodes a BOM-less script with the ANSI codepage, so those
  two lines can render as mojibake for a reader on that host. ⛔ **It cannot affect parsing** — both
  are inside `<# … #>`, and no CP1252 mis-decode of `E2 80 94` can produce `#` or `>`. The file
  already carried one non-ASCII comment character before this row (`:426`, `TASK-1294`'s `·`), so the
  situation is not new. **Keep the em dash**: acceptance demanded the phrase *in those words* and the
  law writes it with an em dash at `:6189`. Recorded only so the next reader does not "fix" it.

- **[NIT-3]** §9 item 2 asked whether `:6-9` (the `.SYNOPSIS` pointer) is in scope. **Ruled: yes.**
  `names:` says *"the header comment block ONLY"*, `<#` opens at `:1`, so `:6-9` is inside it by the
  letter; and leaving it would have left `.SYNOPSIS` pointing at *"the NOT EXECUTED block"*, a block
  that no longer exists under that name (`SC-§99`). **No revert wanted.**

---

## 4. Notes for the next pass (and then for build-master)

**To `gameplay-programmer` — one edit, one file, and DO NOT touch the split.**
1. `:81-82` — `:1646 → :1647`, `:1643 → :1644`. Re-grep both rather than re-deriving them; the
   escape-hatch sentence stays.
2. `:127-134` — rewrite as the **resolution**, not the divergence: struck 2026-09-19 by the manager
   under `SC-§82`, law now published split at `TL-§6` (`FALSE SENTENCE 1` / `FALSE SENTENCE 2` /
   the three-verdict replacement), this block is the code-side half. **Anchor by quoted text, not by
   a line number** (WARN-1).
3. Optional in the same edit: NIT-1's *"two lines"*.
4. ⛔ **Do not** touch `(c1)`/`(c2)`, the `NOT MEASURED — NULL WITH NO POSITIVE CONTROL` wording, the
   census, the reconciliation, or any executable line. ⛔ **Do not** touch `CONVENTIONS.md` — WARN-2
   is the manager's.

**To `build-master` (`TASK-1318`), when this returns PASS:** the parse + its broken-`#>` positive
control and the one real suite run are **yours** — this gate measured delimiters, not a parse.
`git status` is legitimately dirty with `SiegePlayerController.cpp`, `CONVENTIONS.md`, `TASKBOARD.md`
and two handoffs, all belonging to **`TASK-1311`** and **the manager**; stage **by pathspec** and
sweep none of it.

**Runtime:** `TASK-1310` carries no runtime acceptance criterion — it is documentation debt in a
tracked tool. No 5b leg, and that is declared, not forgotten.
