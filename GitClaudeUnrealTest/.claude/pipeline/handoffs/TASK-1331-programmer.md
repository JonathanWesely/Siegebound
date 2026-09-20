# TASK-1331 — HEADER-ADDRESS-GUARD — programmer handoff

- **subject:** `TASK-1331`
- **marker:** `TASK-1331-HEADER-ADDRESS-GUARD`
- **author:** gameplay-programmer, 2026-09-20
- **gate:** `TASK-1332` · **host:** `TASK-1333` · **no 5b** (no runtime acceptance criterion)
- **files written:** `Tools/run_suite_bounded.ps1` (self-test lane only) · this note
- **scratchpad:** `…/scratchpad/t1331-controls.ps1` + `…/scratchpad/lab/` — **lab DELETED** after the controls

---

## 0. The diff, in one line

**123 insertions, 0 deletions, two hunks**, both inside the `-SelfTest` lane:

```
@@ -1088,0 +1089,113 @@ function Add-ThrowCase {      <- the helper Get-HeaderRotProneAddress
@@ -1490,0 +1604,10 @@ function Invoke-SelfTest {    <- the one new Add-SelfTestCase block
```

`git diff | grep '^-[^-]'` returns **zero deleted lines**. That is the structural proof for most of
spec (5): a pure-insertion diff below line 238 cannot have changed a parameter default, a bound,
the `-ExecCmds` composition, the comma-vs-semicolon separator, the `QUIT_EDITOR` terminator, the
header's prose, or any existing self-test case's text, name or order.

---

## 1. (0)(b) THE CENSUS, RE-TAKEN AT MY INSTANT — I did not inherit the manager's

`:[0-9]` over the derived header (lines 1..238) returns **six lines, four species**. This
reproduces the manager's boarding census **exactly**; I state that as an independent
re-measurement, not as a citation of his.

| # | line | text | species | in ban list? |
|---|------|------|---------|--------------|
| 1 | 19 | `(Engine/Source/Runtime/Engine/Private/ParseExecCommands.cpp:29)` | engine-source, **2 digits** | **no** — declared out of scope |
| 2 | 21 | `(EditorServer.cpp:5993); the process hangs until someone kills it.` | engine-source, 4 digits | **no** — declared out of scope |
| 3 | 104 | `+  3 (the 09-18 23:42..23:44 runs, …)` | **clock time** | **no** — not an address |
| 4 | 121 | `…-045537.log opens at 04:55:38,` | **clock time** | **no** — not an address |
| 5 | 122 | `its last line is 04:55:58 (20 s), …` | **clock time** | **no** — not an address |
| 6 | 146 | `CONVENTIONS.md at :5966-5969 and then at :6172-6175, and BOTH rotted…` | **THE EXHIBIT** (pipeline-markdown address, 2nd spelling) | **excluded — it is evidence** |

**⇒ the nomination `:[0-9]{3,4}` is refuted, and I confirm the manager's refutation from my own
measurement.** Row 1 is a **two-digit** real address, so `{3,4}` would have missed it — `SC-126`
cl. 9(b-2) firing inside the very file the check guards. Rows 3–5 are the `401`-digit trap in a
different hat. **"Zero outside the exhibit" does not exist today**: row 6 *is* a banned shape and
it is *supposed* to be there, so a sweep-and-subtract test reds on day one.

---

## 2. (1) THE BAN LIST — enumerated shapes, each grounded in an instance that ROTTED

I did not design these from the board's prose. I recovered the **exact deleted text** from the two
commits that removed them, so every ban names a real corpse. A **commit hash is the one reference
in this note that cannot rot**, so each shape is cited by hash, never by address.

**(a) a pipeline-markdown address (`CONVENTIONS.md` / `TASKBOARD.md`), EITHER spelling**
```
f5697f3 (TASK-1324) deleted:   tool runs" (CONVENTIONS.md:5093), and the correction that caught that then quietly
the header's own exhibit records the second spelling:   … at :5966-5969 and then at :6172-6175
```
> ⚠️ **A correction to the board, offered as measurement, not as a dispute.** The spec's clause (1)
> names this instance as `CONVENTIONS.md:5122`. The text actually deleted by `f5697f3` is
> **`CONVENTIONS.md:5093`**. Same defect, same row, same commit — I record the value I measured
> (`SC-91`) and use it in the code comment.

Pattern: `(?:CONVENTIONS|TASKBOARD)\.md(?:\s+at)?\s*:\s*\d+`

**(b) a BARE self-address into this same `.ps1`**
```
9d80505 (TASK-1327) deleted:   they stood at :1680 (launch) and
9d80505 (TASK-1327) deleted:   :1677 (receipt), both RE-GREPPED against the finished file, never offset from a
```
Pattern: `(?<![0-9A-Za-z._/\\-]):\d+`

### Why the pattern covers the two-digit form — the load-bearing design decision

**Neither shape constrains how many digits the number has.** Both use `\d+`. The discrimination is
moved entirely onto **what precedes the colon**, which is the property that actually separates the
species:

- a **clock time** always has a **digit** before the colon (`04:55:38`, `23:42`) → excluded by the
  lookbehind, **by construction**, not as a subtracted exception;
- an **engine-source address** always has a **filename character** before the colon (`…cpp:29`) →
  excluded by the same lookbehind, and shape (a) never fires on it because it is not a pipeline
  markdown file;
- a **bare self-address** has whitespace or `(` before the colon (` at :1680`, `(:9999)`) → fires.

That is why `ParseExecCommands.cpp:29` is safe from *both* shapes while `CONVENTIONS.md:29` is
caught by shape (a) — **proved by control (iv) and control (v) below**, which are the same two
digits landing on opposite sides of the line.

### Third shapes considered and REJECTED (speculative bans are out, per spec (1))

`CONVENTIONS.md line 5966` and `CONVENTIONS.md#L5966` are *conceivable* rot-prone spellings. **I can
name no instance of either that rotted**, so neither is banned. If one ever rots, the ban list takes
one more entry and one more control.

### Out of scope, declared with its reason (a skipped class that is not declared reads as a forgotten one)

- **ENGINE-SOURCE addresses** (`<File>.cpp:NNN`) — they move on an **engine upgrade**, not on an
  agent's edit, so no agent diff can rot them; and banning them would gut the `-ExecCmds`
  explanation this header exists to carry. Implemented, not merely asserted: **control (v)**.
- **CLOCK TIMES** — not addresses at all. Implemented, not merely asserted: **control (vi)**.

---

## 3. (2) THE EXHIBIT IS EXCLUDED BY SUBSTRING — and the check cannot prescribe its removal

**The substring I chose: `ANCHOR TO QUOTED TEXT`.**

- It occurs **exactly once in the header and exactly once in the whole file** (measured).
- It is the **opening sentence of the exhibit's own paragraph** and is a verbatim restatement of
  `SC-126` cl. 7 — the law the exhibit exists to illustrate. It is load-bearing prose, not
  decoration: if it were ever deleted, the block would have stopped being an exhibit.
- **It contains no digits and no colon**, so the exclusion key can never itself be an address.

**The exclusion is a block, derived outward from the marker to its blank-line boundaries** (it
measured **6 lines**, printed on every run). I chose the block over a single-line match deliberately:
a single-line anchor breaks the moment the paragraph is reflowed and the addresses land on a
different physical line from the marker — and a **false red** is exactly the failure that would tempt
a seventh hand-edit of this header. The cost is that a genuinely new citation added *inside that one
paragraph* would be exempt; I accept it because that paragraph's entire subject is "never cite by
address", and I flag it for the gate in §8 below.

> **⛔ NO ADDRESS ANYWHERE IN THE EXCLUSION.** The exhibit measured `:146` today. It moved `+3`
> under `TASK-1327` and it moves again under this very diff. A test hard-coding `:146` would carry
> the exact defect it guards.

### The sentence spec (2) demands — the exhibit is EVIDENCE, not a citation

**The check is structurally incapable of prescribing the removal of the exhibit's two addresses,
because the exhibit's lines are removed from the corpus *before* any pattern is matched — so no
finding this case can emit is even able to name them.** Those two dead addresses are in the header
*because* they rotted; a sweep that "fixed" them would destroy the thing they demonstrate. The
negative control is the proof: the tracked file **contains `CONVENTIONS.md at :5966-5969`, a
textbook shape-(a) hit, and the case still passes green.**

---

## 4. (0)(a) THE HEADER BOUNDARY IS DERIVED — quoted, with the derivation

**I never scan for `<#` myself.** I ask PowerShell's own parser for the first block-comment token,
so nesting and same-line delimiters are the language's problem, not a guess of mine:

```powershell
[System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$tokens, [ref]$errors)
# first token with .Kind -eq Comment whose .Text starts with the block-comment opener
$first = $header.Extent.StartLineNumber
$last  = $header.Extent.EndLineNumber
```

**Derived value at my instant: `lines 1..238`**, out of **14** block-comment tokens (13 before my
diff, +1 for my helper's doc comment — I confirmed the count moved by exactly one). The value is
**printed in the ledger line on every run, pass or fail**, so a green row is still evidence about
where the header was measured to end. It **fails closed**: if the parser cannot find a leading block
comment, the case REDS rather than reporting "clean" (`SC-39`).

---

## 5. (3) + (3b) THE CONTROLS — seven, each named, each with its output

Method: a scratchpad copy of the tracked file plus the fixture corpus; **one** banned shape injected
at a time as a new header line at line 24 (inside `.DESCRIPTION`, deliberately **outside** the
exhibit block); `-SelfTest` run against the copy. **The tracked file was never mutated.**

The ledger row is read **positionally** (`Substring(63,5) -ceq 'NO'`), for a reason worth recording:
my first harness matched `'\sNO\s'`, PowerShell's `-match` is **case-insensitive**, and it hit the
`no` in the case's own name `"grows no rot-prone line address"` — so the detector reported **FIRED on
every row, including the green ones**. A control harness that cannot tell green from red is not a
control harness; the fix is in the scratchpad script's comment.

### NEGATIVE CONTROL (3b) — the one that proves the check is shippable

**Unmodified tracked file — exhibit present, `ParseExecCommands.cpp:29` present, all three clock
times present:**
```
SC-126: header grows no rot-prone line address                 yes  header derived as lines 1..238; 6 exhibit line(s) excluded by substring, never by address
SELFTEST                55 / 55 cases produced their EXPECTED result.
RUNNER_EXIT = 0
```
**PASSED.** The ban list is right: it does not red on today's `HEAD`. **No header prose was edited**
(spec (5)'s easy trip — and the joke I was explicitly warned not to write).

### CONTROL 0 — baseline scratchpad copy, nothing injected → **NOT FIRED** (as expected)
```
exit 0 | SC-126: … yes  header derived as lines 1..238; 6 exhibit line(s) excluded by substring, never by address
SELFTEST 55 / 55 cases produced their EXPECTED result.
```
Proves the copy itself is faithful, so a later red is attributable to the injection and nothing else.

### CONTROL (i) — `CONVENTIONS.md:9999`, first spelling → **FIRED** ✅
```
exit 5 | SC-126: … NO   … line 24 grew a pipeline-markdown address, either spelling (TASK-1324 shape):
         "CONVENTIONS.md:9999" in >>The rule for this is written down at CONVENTIONS.md:9999 -- go read it.<<
         -- delete it or replace it with a grep anchor (SC-126 cl. 7)
SELFTEST 54 / 55 passed. FAILURES (1 cases):
```

### CONTROL (ii) — `CONVENTIONS.md at :9999`, **the second spelling that defeated a sweep once already** → **FIRED** ✅
```
exit 5 | SC-126: … NO   … line 24 grew a pipeline-markdown address, either spelling (TASK-1324 shape):
         "CONVENTIONS.md at :9999" …; line 24 grew a bare self-address into this .ps1 (TASK-1327 shape): ":9999" …
SELFTEST 54 / 55 passed. FAILURES (1 cases):
```
Caught **twice over** — by shape (a) on the ` at ` spelling *and* independently by shape (b), since
the colon is whitespace-preceded. The spelling that beat the last sweep is covered redundantly.

### CONTROL (iii) — bare `(:9999)` self-address → **FIRED** ✅
```
exit 5 | SC-126: … NO   … line 24 grew a bare self-address into this .ps1 (TASK-1327 shape):
         ":9999" in >>The launch site stands at (:9999) as this paragraph was last written.<< …
SELFTEST 54 / 55 passed. FAILURES (1 cases):
```

### CONTROL (iv) BONUS — `CONVENTIONS.md:29`, **TWO digits** → **FIRED** ✅
```
exit 5 | SC-126: … NO   … "CONVENTIONS.md:29" in >>See CONVENTIONS.md:29 for the short version.<< …
SELFTEST 54 / 55 passed. FAILURES (1 cases):
```
The direct refutation of `:[0-9]{3,4}`, executed rather than argued.

### CONTROL (v) BONUS specificity — `EngineThing.cpp:1234` → **NOT FIRED** ✅ (must stay green)
```
exit 0 | SC-126: … yes … | SELFTEST 55 / 55 cases produced their EXPECTED result.
```
The out-of-scope declaration is **implemented**, not merely claimed. Read against (iv): the same
digits, opposite verdicts, decided purely by what precedes the colon.

### CONTROL (vi) BONUS specificity — clock time `07:15:09` → **NOT FIRED** ✅ (must stay green)
```
exit 0 | SC-126: … yes … | SELFTEST 55 / 55 cases produced their EXPECTED result.
```

---

## 6. (6) ACCEPTANCE EVIDENCE

| criterion | result |
|---|---|
| `-SelfTest` run quoted with the new case's ledger line | ✅ §5, negative control |
| three positive controls FIRED, named individually, with output | ✅ (i) (ii) (iii) — plus bonus (iv) |
| negative control PASSED on the unmodified file | ✅ 55/55, exit 0 |
| header boundary DERIVED and quoted | ✅ `lines 1..238`, via `Parser::ParseFile`, §4 |
| `[Parser]::ParseFile` = 0 errors on the tracked file | ✅ **0 errors**, 14 block comments |
| tracked sha256 identical before/after control work | ✅ `E07515B16B4B3241D0BA862CA691DB52B353FAA2087F21DF514544908DFA31C6` **both** |
| NO suite-count change claimed | ✅ **zero** UE-suite delta; I added no automation test |
| scratchpad lab deleted | ✅ verified `True` |

**On the ledger count, stated precisely so it is not mistaken for a suite-count claim:** the
`-SelfTest` ledger goes **54 → 55**. I did not infer that from arithmetic — I extracted
`HEAD:…/run_suite_bounded.ps1`, ran it in the same lab, and **measured 54/54** with no `SC-126` row.
**That is the self-test ledger, not the 555-test UE suite**, which this row does not touch at all.

---

## 7. (7) WHO RUNS `-SelfTest`, AND WHEN — MEASURED. **The provisional rule is REFUTED as current practice.**

⛔ I did not invent a rule (`SC-100`). What follows is what the artifacts say.

**Every commit that has touched `Tools/run_suite_bounded.ps1`, against whether its row ran `-SelfTest`:**

| commit | row | what it changed | `-SelfTest` run? |
|---|---|---|---|
| `f41fd67` | TASK-1294 (+gate 1295) | **code** (+2 self-test cases) | ✅ **YES** — handoff quotes `54 / 54`; gate `TASK-1295` **checked the count** |
| `b98b78a` | TASK-1310 (gate 1317/1323, host 1318) | **header prose** | ❌ **no** |
| `f5697f3` | TASK-1324 (gate 1325, host 1326) | **header prose** | ❌ **no** |
| `9d80505` | TASK-1327 (gate 1328, host 1329) | **header prose** | ❌ **no** |

Measurement: `SelfTest` appears in exactly **3 handoffs** (`TASK-1181`, `TASK-1183`, `TASK-1294`) and
**2 QA reports** (`TASK-1182`, `TASK-1295`) across the whole pipeline. It appears **zero times** in
`TASK-1310`, `1317`, `1318`, `1324`, `1325`, `1326`, `1327`, `1328` or `1329`.

**Finding:** the manager's provisional rule — *"any row touching this file runs `-SelfTest`, and its
gate checks the ledger line was quoted"* — **was honoured exactly once** (`TASK-1294`/`1295`) and
**has been honoured by none of the last three rows that touched this file**.

**And the shape that practice actually follows is the one that matters here, and it is inverted:**

> Rows that changed the runner's **CODE** ran `-SelfTest`. Rows that changed only its **HEADER PROSE**
> did not — all three of them.

**⇒ `SC-70`'s half a fact, stated plainly: as practice stands today, this new check would have been
run by ZERO of the three rows it is designed to catch.** The rot happens in the header prose, and
header-prose rows are precisely the ones that have never run this lane. The guard is correct and
proven; **its trigger is the open question, and the trigger is the manager's to set.** I recommend
nothing beyond reporting it — boarding that rule is `SC-100` work.

---

## 8. What QA (`TASK-1332`) should scrutinise hardest

1. **Did the controls actually fire?** §5 — check (i), (ii), (iii) each show **exit 5** and
   **`54 / 55`**, and the negative control shows **exit 0** and **`55 / 55`**. A check that cannot
   fail is what this row exists to prevent (`SC-39` · `SHIP-§9`).
2. **The block exclusion is wider than a line.** I chose it over a single-line anchor to survive a
   reflow; the accepted cost is that a new citation added *inside the exhibit's own paragraph* would
   be exempt. **If the gate judges that too wide, say so — it is a one-line change** (drop the
   outward growth, match the marker line only) **and I will take the false-red risk instead.** This
   is the one genuine design trade in the diff and I am flagging it rather than burying it.
3. **The helper's own doc comment quotes `CONVENTIONS.md:5093` and `:1680` as ban-list provenance.**
   These are **deleted text quoted by commit hash** — the same species as the header's exhibit:
   evidence, not citations. They also sit at **line ~1100, far below the derived header (1..238)**,
   so the check does not scan them, by scope and not by exemption.
4. **A defect I hit and fixed, worth a look in case it recurs:** my helper's doc comment originally
   spelled the block-comment **closer** literally while explaining the delimiters. That **closed the
   doc comment early**; the remaining prose was parsed as code, an unterminated string ran on until
   the next quote, and `ParseFile` reported `MissingTerminatorMultiLineComment` **113 lines away
   from the real cause**. Fixed, and the comment now says why it may not quote those two characters.
   **Current parse: 0 errors.**
5. **Line endings:** `git diff` emits the pre-existing `LF will be replaced by CRLF` warning for this
   file. It is **not from my diff** — the file is LF on disk with `eol: unspecified`, exactly as
   `TASK-1324` and `TASK-1327` left it. Mentioned so the host does not read it as new.

---

## 9. Fences — all held

- ✅ `-SelfTest` purity **confirmed at my own instant** before I ran it: the **only** `Start-Process`
  call site in the file is at line 1690, inside the launch path; `Invoke-SelfTest` contains **zero**
  `Start-Process` / `Stop-Process` / `Get-Process` / `Invoke-Expression` / `Start-Job` / `.exe`
  constructs, and `Invoke-Main` returns `Invoke-SelfTest` **before** reaching any launch code.
  **No editor process was spawned. `TASK-1316`'s held verify leg was not disturbed** (`SC-118`).
  ⚠️ One precision: the lane is **not** "no disk access" as the header says — it reads the
  `SuiteRunnerFixtures` corpus, and my case reads the script file via `$PSCommandPath`. **Process
  purity, which is the fence that matters, holds absolutely.** I did **not** edit the header to
  correct that wording (spec (5)).
- ✅ No compile, no UE suite run, no git operation — `TASK-1333` is the host.
- ✅ `TASKBOARD.md`, `CONVENTIONS.md`, `Saved/**`, `.uproject`, all `.cpp` / `.uasset`: untouched.
- ✅ **Zero** files from `TASK-1314`'s or `TASK-1319`'s diff. `SiegePlayerController.cpp`,
  `handoffs/TASK-1314-programmer.md`, `qa/TASK-1314-verify.md`, `qa/TASK-1315-report.md` and
  `handoffs/TASK-1329-buildmaster.md` were **read-never, written-never** — held, not orphaned.
- ✅ `Tools/run_suite_bounded.ps1` was **clean at my instant** (the dispatch anticipated it might be
  dirty from `TASK-1316`'s fence 8; it was not). My delta is the only dirt on it now.

---

## 10. ⚠️ STATUS FLIP — I CANNOT DO IT

My `names` block fences me from `TASKBOARD.md`. **I cannot flip `TASK-1331` to `ready-for-qa`
myself.** Routing it to the orchestrator, per the dispatch.

**Requested status:** `TASK-1331` → **`ready-for-qa`** (gate `TASK-1332`).
