# TASK-1335 — HEADER-GUARD-FAIL-CLOSED — programmer handoff

- **subject:** `TASK-1335`
- **marker:** `TASK-1335-HEADER-GUARD-FAIL-CLOSED`
- **author:** gameplay-programmer, 2026-09-20
- **gate:** `TASK-1336` · **host:** `TASK-1337` · **no 5b** (no runtime acceptance criterion)
- **files written:** `Tools/run_suite_bounded.ps1` (helper + its doc comment, ALL below the header block) · this note
- **scratchpad:** `…/scratchpad/t1335-lab/`, `t1335-harness.ps1`, `t1335-parse-control.ps1`, `head_blob.ps1` — **all DELETED** after the controls (verified `True`)

---

## 0. The one-line summary

Both of `TASK-1332`'s WARN are discharged, each **proven in two directions** rather than asserted:
the exhibit exclusion now **fails CLOSED** like the header boundary already did, and the doc comment
**stops carrying live counts and addresses altogether** instead of merely correcting them.

⚖️ **The irony, written down because it is the row's whole point:** this was a **rot-guard's own
documentation making an unmeasured claim about movement** — `SC-§126` cl. 1's exact shape, inside the
row built to stop it, in a comment whose subject is that claims about position rot. The remedy taken
is not "write a truer number"; it is **write no number**, because a count in a comment is an address
wearing different clothes.

---

## 1. (0) BASELINE FIRST — the guard was GREEN on the unmodified tracked file before I changed a byte

⛔ Run against the **tracked file at `8cbd5d1`**, `git diff --numstat` on it **empty** (clean at my instant).

```
SHA256-BEFORE: E07515B16B4B3241D0BA862CA691DB52B353FAA2087F21DF514544908DFA31C6
SC-126: header grows no rot-prone line address                 yes  header derived as lines 1..238; 6 exhibit line(s) excluded by substring, never by address
SELFTEST                55 / 55 cases produced their EXPECTED result.
RUNNER_EXIT = 0
```

⇒ **not already red.** Nothing had moved under it, so my diff is creditable/blameable for its own
state and nothing else (`SC-§95`).

**Process purity re-confirmed at MY instant (`SC-§118` · `SC-§91`), never inherited from the dispatch:**
`Invoke-SelfTest` spans lines 1202..1631 at HEAD and contains **0** occurrences of
`Start-Process` / `Stop-Process` / `Get-Process` / `Invoke-Expression` / `Start-Job` / `.exe` /
`Start-Sleep` (grep count, measured). The file's **only** `Start-Process` call site is in the launch
path, and `Invoke-Main`'s **first statement** is `if ($SelfTest) { return (Invoke-SelfTest) }` — it
returns before any launch code. **No editor was launched; none was running and none was started.**

### My own derived header boundary and my own pair count — `SC-§126` cl. 11, nothing inherited

| quantity | my instrument | my value |
|---|---|---|
| header boundary | `Parser::ParseFile` → first block-comment token's `Extent` | **lines 1..238** |
| block-comment **pairs** | parser token count | **14** |
| corroboration | raw `#>` occurrences | **14** (and raw `<#` = **15**, the 15th being the **string literal** `'<#'` inside the helper itself — not a delimiter) |
| exhibit marker | `ANCHOR TO QUOTED TEXT` substring | line **144**, addresses on **146**, exempt block **144..149 = 6 lines** |
| header `:<digit>` census | line-by-line scan of 1..238 | **6 lines, 4 species** (`:19`, `:21` engine · `:104`, `:121`, `:122` clock · `:146` the exhibit) |

⇒ **NIT-1 confirmed by my own measurement: the shipped comment said `13`; the true value is `14`.**
I did **not** take that from the gate or the manager. ⭐ **And I then removed the number rather than
correcting it** — see §4.

---

## 2. 🚨 (1) + (1b) WARN-2 — THE TWO-DIRECTION CONTROL. **This is the row's acceptance and it is shown in BOTH limbs.**

**The mutation, identical in both limbs:** the anchor line is **reworded in place** (not deleted), so
the paragraph, the line count, the exhibit's addresses and the derived boundary are all unchanged and
the ONLY variable is whether the substring still exists:

```
-    ANCHOR TO QUOTED TEXT, NEVER TO A LINE NUMBER OR TO "THE LAW STILL READS Y"
+    ANCHOR TO THE QUOTED WORDS, NEVER TO A LINE NUMBER OR TO "THE LAW STILL READS Y"
```

### ⛔ LIMB A — **TODAY'S CODE** (the `8cbd5d1` blob), anchor reworded → REDS **NAMING THE EXHIBIT'S ADDRESSES**

```
expect=NO  got=NO  CONTROL OK
exit=5   SELFTEST 54 / 55 passed. FAILURES (1 cases):
SC-126: header grows no rot-prone line address   NO   header derived as lines 1..238;
  0 exhibit line(s) excluded by substring, never by address;
  line 146 grew a pipeline-markdown address, either spelling (TASK-1324 shape): "CONVENTIONS.md at :5966" …
  line 146 grew a bare self-address into this .ps1 (TASK-1327 shape): ":5966" …
  line 146 grew a bare self-address into this .ps1 (TASK-1327 shape): ":6172" …
>>> DETAIL NAMES THE EXHIBIT ADDRESSES <<<
```

⇒ **WARN-2 reproduced live.** `Exempt` silently collapses to 0, the exhibit is scanned, and the case
emits **three findings prescribing the deletion of the two addresses that exist BECAUSE they rotted** —
the one finding its own doc comment called itself *"structurally incapable"* of emitting, and an
engraved invitation to the **seventh** hand-edit of the header.

### ✅ LIMB B — **FIXED CODE**, same mutation → REDS **FAIL-CLOSED, NAMING NOTHING**

```
expect=NO  got=NO  CONTROL OK
exit=5   SELFTEST 54 / 55 passed. FAILURES (1 cases):
SC-126: header grows no rot-prone line address   NO   header derived as lines 1..238;
  0 exhibit line(s) excluded by substring, never by address;
  FAIL-CLOSED: exhibit anchor not found inside the derived header, so the exhibit could not be excluded and NOTHING was scanned

lines in the ENTIRE run output naming an exhibit address (:5966 | :6172 | 5966-5969 | 6172-6175): 0
```

⇒ ⭐ **Both limbs RED — the mutation is still caught, which is the half a one-direction test would have
missed. What changed is the CONTENT of the red:** from *"delete these two addresses"* to *"I could not
find my own exclusion anchor, so I refused to look."* The `0 exhibit line(s) excluded` meter still
prints, so the tripwire `TASK-1332` ruled load-bearing is untouched.

**The fix, one line as specced, and it is the identical treatment the boundary already got:**

```powershell
    if ($exempt.Count -eq 0) {
        return @{ Ok = $false; First = $first; Last = $last; Exempt = 0
                  Why = @('FAIL-CLOSED: exhibit anchor not found inside the derived header, so the exhibit could not be excluded and NOTHING was scanned') }
    }
```

It returns **before** `$shapes` is even constructed, so the corpus is never matched.

---

## 3. ✅ (2) NIT-4 — the parse-error fail-close, also in BOTH directions

**Why this one needed a different instrument, declared rather than glossed:** the shipped case calls
`Get-HeaderRotProneAddress -Path $PSCommandPath` — it parses **itself** — so a copy carrying a syntax
error cannot be launched to drive it. I therefore ran the **shipped function bytes**, lifted out of
each file by `FunctionDefinitionAst.Extent.Text` (the same parser the function itself uses), so nothing
was retyped or paraphrased. **The probe asserts instrument fidelity first**, against a known state:

```
FIDELITY on the real file -> Ok=True  first=1 last=238 exempt=6      (matches the shipped ledger line exactly, BOTH directions)
```

The subject: a faithful copy of the tracked file with **one** deliberate syntax error appended at the
very end — header token, exhibit anchor and ban list all intact, so the ONLY variable is the parse.

```
(2) BEFORE -- TODAY'S code against a file that does not parse
    broken file really does not parse: 1 parser error(s) -- first: Missing closing ')' in subexpression.
    RESULT  Ok=True    why: <none -- it reported CLEAN>

(2) AFTER  -- FIXED code against the same file
    broken file really does not parse: 1 parser error(s) -- first: Missing closing ')' in subexpression.
    RESULT  Ok=False   why: FAIL-CLOSED: the parser reported 1 error(s) on this file, so its token stream cannot be trusted to bound the header
```

⇒ **NIT-4 was live:** the case reported **"clean"** about a file that does not parse. Now it reds.

**On the guard's own shape:** `if ($errors -and $errors.Count -gt 0)`. An empty `ParseError[]` is
falsy, so the happy path is untouched (proved by the green tracked run); `$null` is falsy too, so no
StrictMode dereference. ⛔ I deliberately did **not** write `@($errors).Count`, which returns **1** for
`$null` and would have fail-closed the whole guard spuriously on every run.

---

## 4. ✅ (4) WARN-1 + NIT-1 — the doc comment stops claiming movement it never measured

### The claims, MEASURED BY ME across the actual commits (not inherited from the gate — `SC-§126` cl. 11)

| commit | exhibit marker line | header first `#>` |
|---|---|---|
| `9d80505^` (before TASK-1327) | **141** | — |
| `9d80505` (TASK-1327) | **144** | — |
| `8cbd5d1^` (before TASK-1331) | **144** | **238** |
| `8cbd5d1` (TASK-1331's own diff) | **144** | **238** |

⇒ **WARN-1 is right, and I re-derived it rather than accepting it.** The exhibit moved `141 → 144`
(**`+3`**) under `TASK-1327` — and it did **NOT** move under `TASK-1331`'s "very diff", nor did the
boundary. Both claims were false the day they were written.

### OLD text (quoted verbatim, both offending sentences)

```
    the corpus BEFORE any matching runs, so no failure this case can emit is even capable
    of naming them. Hard-coding that block's line number instead would carry the exact
    defect this case guards -- it measured :146 today, it moved +3 under TASK-1327, and it
    moves again under this very diff.
...
    language's problem and not this function's guess. It measured lines 1..238 at this
    writing, out of 13 block-comment delimiter pairs -- and it moves with every edit to
    this file, including this one.
...
    IT FAILS CLOSED. If the header cannot be located, the case REDS. A guard that reports
    "clean" when it could not find its subject is worse than no guard (SC-39).
```

### NEW text (quoted verbatim)

```
    the corpus BEFORE any matching runs -- and if that substring is ever deleted or
    reworded the function REFUSES TO SCAN AT ALL rather than scanning without it, so no
    failure this case can emit is capable of naming those two addresses UNDER ANY INPUT,
    not merely under the happy one. Hard-coding that block's line number instead would
    carry the exact defect this case guards: the block moved +3 under TASK-1327 (measured
    across that commit, not quoted from a ledger), and the next edit that adds or removes
    a line ABOVE it moves it again.
...
    language's problem and not this function's guess. NO BOUNDARY, COUNT OR OFFSET IS
    WRITTEN DOWN IN THIS COMMENT: the derived value is printed in the ledger line on EVERY
    run, pass or fail, which is the one place it cannot go stale. It moves with any edit
    that adds or removes a line ABOVE it -- not with every edit to this file, and a number
    recorded here would rot exactly as the hand-deleted citations above it did. A count in
    a comment is an address wearing different clothes.
...
    IT FAILS CLOSED AT ALL THREE OF ITS ENTRY CONDITIONS -- the parse, the header boundary
    and the exhibit anchor. A parse error, a missing leading block comment, or a missing
    exhibit anchor each RED the case with a FAIL-CLOSED sentence instead of scanning a
    corpus this function could not bound. A guard that reports "clean" when it could not
    find its subject is worse than no guard (SC-39) -- and a guard that fails closed at one
    anchor while failing open at the next has only moved the hole.
```

### 🚨 Three deliberate departures from the gate's literal suggested wording, declared for the gate to rule on (`SC-§101`)

1. ⭐ **I DELETED `:146` and `lines 1..238` and `13 … pairs` outright, rather than refreshing them.**
   The spec's (4) says the count-free phrasing is **PREFERRED IF YOU CAN PHRASE IT**; the gate's
   two-word fix would have **kept** `it measured :146 today` and `It measured lines 1..238 at this
   writing`. Keeping either leaves a **live address and a live count inside the comment of the guard
   whose subject is that live addresses rot** — and they sit below the derived header, so the guard
   **cannot** catch them itself. I removed all three. The information is not lost: the boundary and the
   exempt count are **printed in the ledger line on every run, pass or fail**, which is strictly more
   current than a comment can ever be. **If the gate prefers the literal WARN-1 wording, this is a
   one-line revert and I will take it.**
2. **I tightened *"the next edit ABOVE it moves it again"* to *"the next edit that ADDS OR REMOVES A
   LINE above it."*** The gate's phrasing is overbroad in the same direction it was correcting: an
   equal-line-count rewrite above the block moves nothing. Same for the boundary sentence.
3. **`+3 under TASK-1327` is retained — because I measured it** (`141 → 144`, table above), and it is a
   **delta about a closed commit**, which cannot rot the way a position can. I added the parenthetical
   `(measured across that commit, not quoted from a ledger)` so the next reader knows its provenance.

---

## 5. ✅ (3) NIT-2 + NIT-3 — precision, each proven in BOTH directions, and ALL SEVEN controls RE-RUN

**The pattern change (shape (a) only; shape (b) untouched, as specced):**

```
-  Rx = '(?:CONVENTIONS|TASKBOARD)\.md(?:\s+at)?\s*:\s*\d+'
+  Rx = '(?i)(?:CONVENTIONS|TASKBOARD)\.md(?:\s+at\s*)?:\d+'
```

### 🚨 ALL SEVEN of `TASK-1331`'s controls RE-RUN AT MY INSTANT — reported individually, none inherited

⛔ Method: a scratchpad copy of the **edited** tracked file plus the fixture corpus; **one** shape
injected at a time as a new header line at line 24 (inside `.DESCRIPTION`, deliberately **outside** the
exhibit block); `-SelfTest` run against the copy in a child process. The tracked file was never mutated.
⛔ The harness reads the ledger verdict **positionally** (`Add-SelfTestCase` formats `'{0,-62} {1,-5}{2}'`,
so column 63 begins `yes`/`NO`) — **never** by regex, because PowerShell's `-match` is case-insensitive
and hits the `no` inside the case's own name, which is precisely the instrument failure `TASK-1331`
recorded. Every run asserts `expect` vs `got` explicitly.

| # | control | expect | got | verdict |
|---|---|---|---|---|
| (i) | `CONVENTIONS.md:9999` — first spelling | NO | **NO** | ✅ fired, shape (a) |
| (ii) | `CONVENTIONS.md at :9999` — the sweep-defeating second spelling | NO | **NO** | ✅ fired **TWICE**, shape (a) *and* shape (b) independently |
| (iii) | bare `(:9999)` | NO | **NO** | ✅ fired, shape (b) |
| (iv) | `CONVENTIONS.md:29` — **TWO digits** | NO | **NO** | ✅ fired — `{3,4}` refuted again, executed not argued |
| (v) | `EngineThing.cpp:1234` — engine address | yes | **yes** | ✅ did **not** fire |
| (vi) | clock time `07:15:09` | yes | **yes** | ✅ did **not** fire |
| (vii) | **NEGATIVE CONTROL — unmodified `HEAD` prose, AFTER the regex change** | yes | **yes** | ✅ **exit 0, `55 / 55`**, `6 exhibit line(s) excluded` |

⇒ ⛔ **A check that cannot pass is as broken as one that cannot fail (`SC-§39`). (vii) is the half that
proves it is shippable, and it was re-run AFTER the pattern moved, not before.**

### The two NEW controls the pattern change bought, each in BOTH directions

| # | control | on **TODAY'S** code | on **FIXED** code |
|---|---|---|---|
| (viii) | **NIT-3** lowercase `conventions.md:9999` | **yes — SLIPPED SILENTLY** (`55/55`, exit 0) ⛔ the defect, live | **NO — fires** ✅ |
| (ix) | **NIT-2** English *"see CONVENTIONS.md: 3 rows apply"* | **NO — FALSE RED**, quoting `"CONVENTIONS.md: 3"` ⛔ the defect, live | **yes — stays green** ✅ |

⇒ **Both NITs were real and live, not theoretical.** (ix) is the expensive direction by the row's own
argument: a false red on this header invites the next hand-edit of it.

⛔ **And I verified `(?i)` adds no new hit on `HEAD`:** the only `CONVENTIONS`/`TASKBOARD` mentions in
lines 1..238 are `:141`, `:146`, `:151`, `:181`, and only `:146` (the **exempt** exhibit) is followed by
a colon-digit. That is why (vii) still passes.

---

## 6. ✅ (6) `SC-§129` — I am the rule's first live subject. The FULL ledger, reconciled BY NAME

⛔ `SC-§129` cl. 3(b): the whole ledger, **by name, never by total** (`SC-§104`). Run against the
**edited tracked file**, `RUNNER_EXIT = 0`:

**§1 FIXTURE CORPUS (11):** `green-suite.log` yes · `red-suite.log` yes · `skipped-suite.log` yes ·
`zero-started.log` yes · `zero-started-filtered.log` yes · `result-absent.log` yes ·
`count-mismatch.log` yes · `w9-cmd-semicolon.log` yes · `green-commands.log` yes ·
`no-terminator-echo.log` yes · `__does_not_exist__.log` yes

**§2 BOUND ARITHMETIC (5):** overall bound tripped yes · boot bound yes · stall bound yes ·
healthy run NOT killed yes · slow boot inside the bound survives yes

**§3 LANE CONSTRUCTION (12):** suite value string yes · sub-group filter yes · command value COMMAS+terminator yes ·
caller `Quit` dropped yes · `;` refused yes · `,` refused yes · empty list refused yes · only-terminators refused yes ·
W-4 `;` filter yes · W-4 `,` filter yes · W-4 quote filter yes · W-4 empty filter yes

**§4 EXPECTED ECHOES (5):** suite 1 echo without `;Quit` yes · B-1 command lane 2 echoes yes ·
symmetry `;` yes · symmetry `,` yes · symmetry mangled `-Filter` yes

**§5 COMMAND LINE (4):** returns `[string]` yes · `-ExecCmds` verbatim yes · paths quoted + `-nullrhi`/`-unattended` yes ·
command lane COMMA form yes

**§5b AURA EXCLUSION (2):** `-DisablePlugins=Aura` both lanes yes · flag guard refuses 4 degenerate lines yes

**§6 BOUND MERGE (2):** B-2 exit 6 not 0 yes · no bound tripped → log verdict untouched yes

**§7 `-LogPath` SAFETY (7):** default shape yes · absolute inside `Saved` yes · `.uasset` refused yes ·
non-`.log` refused yes · outside project refused yes · traversal refused yes · empty refused yes

**§8 INPUT TOLERANCE (7):** `Get-CmdEchoes` blank lines yes · `Get-CmdEchoes` empty array yes ·
`Test-SuiteCounts` blank lines yes · zero echoes not a pass yes · B-1 terminator corroboration yes ·
W-10 incremental reader yes · **`SC-126: header grows no rot-prone line address` — `yes`,
`header derived as lines 1..238; 6 exhibit line(s) excluded by substring, never by address`**

```
SELFTEST                55 / 55 cases produced their EXPECTED result.
RUNNER_EXIT = 0
```

⇒ **11+5+12+5+4+2+2+7+7 = 55 named cases, every one `yes`. ZERO ledger-count delta** (55 before,
55 after): I added **no** new `-SelfTest` case and edited, renamed or reordered **none** of the
existing ones. ⛔ **ZERO UE-suite delta** — no automation test was added, and I ran no UE suite.

⚖️ **On why the two NEW controls are controls and not shipped cases, declared so a skipped choice does
not read as a forgotten one:** the spec's acceptance lists them among *the controls*, my `names` block
authorises *"its self-test case"* (singular), and shipping them would move the ledger off 55 and give
the host a count to reconcile against a row whose fences say the delta is zero. They are documented in
the doc comment instead (§5's two new paragraphs), which is where the next reader will look.

---

## 7. ✅ (7) FENCES — every one held, each with its instrument

| fence | verdict | how I know |
|---|---|---|
| 🚨 **the header block comes out BYTE-IDENTICAL** | ✅ | **sha256 of bytes through the 238th LF: `072F55CEB8BC4AD189FCACDBABF5E97C6E912028150C3891E19EF55F08593A2E` on `8cbd5d1` AND on my working copy — equal.** Lowest diff hunk starts at **`@@ -1108`** |
| the exhibit untouched | ✅ | it is inside the header block, which is byte-identical; its addresses are still `:146` |
| no parameter default / bound / `-ExecCmds` composition / comma-vs-semicolon / `QUIT_EDITOR` (`SC-§116`) | ✅ | all 12 deleted lines are accounted for: 11 doc-comment prose lines + the one shape-(a) `Rx` line. Nothing else was removed |
| no existing self-test case edited, renamed or reordered | ✅ | zero hunks inside `Invoke-SelfTest` (1202..1631 at HEAD); the full by-name ledger above is unchanged at 55 |
| no new UE automation test / zero suite-count delta | ✅ | no `.cpp`, no automation macro touched; I ran no suite |
| `#19` orphan-census logic untouched | ✅ | not in any hunk |
| `TASKBOARD.md` / `CONVENTIONS.md` / `Saved/**` / `.uproject` / any `.cpp` / any `.uasset` absent from MY diff | ✅ | §8 below — and see the ⚠️ |
| **ZERO files from `TASK-1314`'s lane** | ✅ | I never opened `SiegePlayerController.cpp` — and see the ⚠️, which is **not mine** |
| no compile, no UE suite, no git write, no commit | ✅ | `TASK-1337` is the host. Git used **read-only** (`status`, `diff`, `log`, `show`) |
| no editor launched | ✅ | none was running at my start, none was started; `-SelfTest` purity measured in §1 |

**`Parser::ParseFile` on the edited tracked file = 0 errors**, 14 block-comment tokens, header still
`1..238`. (Positive control that the probe can fire: the deliberately-broken copy in §3 returned
**1** error from the same call — so the `0` is a measurement, not a silent instrument.)

**`sha256` of the tracked file, identical before and after the whole control battery** —
`E6EA8B7C87670AB52D57F6092A5BC5D93047A8E05F21CE790C4871C9C7F7A0C5` measured immediately after my
edits and again after every control had run. **The controls mutated only scratchpad copies.**

---

## 8. ⚠️ `git status` IS NOT CLEAN — AND ONE ENTRY APPEARED DURING MY SESSION AND IS **NOT MINE**

⛔ Stated loudly because the acceptance asks for *"only that file + your handoff"* and it is **not**
satisfiable by me. `git diff --numstat` (⛔ `--numstat`, never `--stat` — `SC-§128`), at git root
`C:\GitProjects\GitHub\GitClaudeUnrealTesting`:

```
3   3   GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
66  0   GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
45  13  GitClaudeUnrealTest/Tools/run_suite_bounded.ps1
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1333-buildmaster.md
```

**MINE: exactly one file — `Tools/run_suite_bounded.ps1`, `+45 / −13`.** Plus this handoff.

- `TASKBOARD.md` (`3/3`) and `handoffs/TASK-1333-buildmaster.md` were **already dirty in the
  session-start snapshot**, before I read anything. Not mine.
- 🚨 **`SiegePlayerController.cpp` (`+66 / −0`) was CLEAN in my OWN `git status` taken mid-session, and
  appeared afterwards.** Timestamps: my file last written **02:10:55**, the `.cpp` written **02:12:29**,
  my status taken between them. Its added text names **`TASK-1319`** and sits in `HandleMatchEnd` —
  i.e. **`TASK-1314`'s held lane, landed concurrently by another agent.** ⛔ **I never opened that file**
  (read-never, written-never). I record it rather than reporting a clean tree I did not have (`SC-§95`).
- ⇒ 🚨 **FOR THE HOST (`TASK-1337`): commit BY PATHSPEC and never `-a`.** `Tools/run_suite_bounded.ps1`
  + `handoffs/TASK-1335-programmer.md` only. A `-a` here would sweep a held lane and the board into a
  gated diff (`SC-§102` · `SC-§106`). ⚠️ The `LF will be replaced by CRLF` warning on this file is
  **pre-existing**, as `TASK-1324`/`1327`/`1331` all recorded.

---

## 9. ⛔ (5) THE THREE DECLINES — honoured, not re-proposed

- **(a) The block→marker-line narrowing: NOT taken, and I did not re-offer it.** I re-derived why in
  my own numbers: the marker is at **144**, the addresses at **146** ⇒ narrowing leaves `:146` in the
  corpus ⇒ shape (a) fires ⇒ **the negative control fails on `HEAD`.** Any `+2` variant is an address
  in disguise. ⭐ And my fail-closed fix makes the wide block **strictly safer**, because the failure
  mode that made its width worrying — the anchor going missing — now refuses to scan instead of
  scanning everything.
- **(b) NIT-6: CLOSED, nothing touched.** The *"no disk access"* wording is in two other functions'
  doc comments far below the header and describes those functions.
- **(c) NIT-5: DECLINED, and its trigger is unchanged** — it gets a row the first time it aborts a
  real ledger. I added **no** `try/catch`. ⚠️ One honest note: my two new `return` guards make an
  abort **less** likely, not more — both fire on conditions that previously ran on into `ReadAllLines`.

---

## 10. What `TASK-1336` should scrutinise hardest

1. ⭐ **The two-direction control is check one — read BOTH limbs (§2).** A fix proven only in the
   "after" direction has not been proven. LIMB A must show the old code naming `:5966`/`:6172`; LIMB B
   must show `0` lines in the whole run output naming any exhibit address.
2. **My three departures from the gate's literal suggested wording (§4).** The big one: I **deleted**
   `:146`, `1..238` and the pair count instead of refreshing them. That is the spec's stated
   preference, but it is more than WARN-1's two-word fix — **rule on it.** One-line revert if you
   disagree.
3. **The `(?i)`-on-shape-(a)-only asymmetry.** Shape (b) has no letter outside its lookbehind, so
   `(?i)` there would be decoration. Controls (i)–(vi) all re-run to prove the change broke nothing.
4. **The §3 instrument.** I could not drive the parse-error control through the shipped case (it
   parses itself), so I lifted the shipped function via the AST and asserted fidelity first. If you
   judge that too indirect, say so — the alternative is a scratchpad-only edit of the case's `-Path`
   argument, which I avoided precisely because it mutates the case.
5. ⚠️ **§8 — `SiegePlayerController.cpp` is dirty and is NOT mine.** Verify against the commit
   pathspec, not the tree.

---

## 11. ⚠️ STATUS FLIP — I CANNOT DO IT

My `names` block explicitly fences me from `TASKBOARD.md` (*"NEVER … `TASKBOARD.md`"*). **I cannot flip
`TASK-1335` myself.** Routing it to the orchestrator, exactly as `TASK-1331` had to.

**Requested status:** `TASK-1335` → **`ready-for-qa`** (gate `TASK-1336`, host `TASK-1337`).
