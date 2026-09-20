# QA Report — TASK-1332 (gate over TASK-1331)

Verdict: **PASS** — 0 BLOCKER · 2 WARN · 6 NIT · 1 DECLARED RESIDUAL (routed to `TASK-1333`)

- **subject:** `TASK-1331` — HEADER-ADDRESS-GUARD
- **marker:** `TASK-1332-HEADER-ADDRESS-GUARD-GATE`
- **reviewer:** qa-reviewer, 2026-09-20
- **host:** `TASK-1333` · **no 5b** (no runtime acceptance criterion)
- **reviewed:** `Tools/run_suite_bounded.ps1` (header `1..238` whole; new helper `:1089-1200`; new case `:1604-1612`) ·
  `handoffs/TASK-1331-programmer.md` whole · `CONVENTIONS.md` `SC-§126` cl. 7/8/9/10 whole ·
  `handoffs/TASK-1324-programmer.md` · `handoffs/TASK-1329-buildmaster.md` · `qa/TASK-1325-report.md` · `qa/TASK-1328-report.md`

---

## 0. What I MEASURED vs what I ACCEPTED AS DECLARED — stated first, because the row's whole thesis is that provenance rots (`SC-§71b` · `SC-§91`)

**I have no `Bash` in this session.** I therefore ran **no** PowerShell, **no** `-SelfTest`, and **no** `git`
command. ⛔ **I did NOT route git or a shell through the read-only `unreal_inspector`** — that server is
read-only by name, but using it as a shell would circumvent a designed fence rather than work around a
missing tool. `TASK-1317` and `TASK-1328` hit this identical wall and both declined; I decline the same
way, and `TASK-1328`'s residual was accepted on exactly this basis.

**MEASURED BY ME, with `Grep`/`Read` over today's bytes:**

| # | claim | my instrument | result |
|---|---|---|---|
| M1 | the `:[0-9]+` census inside the header | `Grep ':[0-9]'` over `run_suite_bounded.ps1` | **6 lines, 4 species** — matches the board's and the row's **exactly** |
| M2 | the header boundary | `Grep '^#>'` → first close at `:238`; `^<#` at `:1` | **`1..238`**, and **13 delimiter pairs strictly interleave** below it |
| M3 | the boundary DID NOT MOVE under this diff | `TASK-1329`'s pre-diff ledger vs mine | **identical for all 13 pre-existing pairs** (`1:238 · 313:322 · 372:382 · 405:418 · 520:525 … 951:964 · 1032:1035 · 1057:1065`) |
| M4 | no address is an exclusion anchor in the **code** | `Grep '\b146\b\|\b238\b'` | **2 hits, both in doc-comment prose** (`:1132`, `:1137`); **zero in executable code** |
| M5 | all seven control **verdicts** re-derived from the shipped regexes against today's bytes | by hand, per shape | **all seven agree with the handoff** (§3 below) |
| M6 | `:5122` vs `:5093` | 4 independent artefacts | **the row is RIGHT** (§4) |
| M7 | the clause-(7) census | `Grep 'SelfTest'` over `.claude/pipeline` | **3 handoffs + 2 QA reports, exactly as the row reports** (§5) |
| M8 | the "no disk access" wording | `Grep` + `Read` in context | **the row misattributed it; nothing is false** (NIT-6) |

**ACCEPTED AS DECLARED, NAMED ONE BY ONE (`SC-§71b`: a tool I do not hold cannot have produced a measurement):**

1. **That the seven controls were EXECUTED** and produced the quoted `exit 5` / `exit 0` and `54 / 55` /
   `55 / 55` ledger lines. I verified every control's **verdict** analytically (M5) — a second, independent
   mechanism, per `SC-§126` cl. 9's *"re-measure with a DIFFERENT instrument"* — but **I did not run the harness.**
2. **The self-test ledger arithmetic `54 → 55`.** Not statically countable: `Add-SelfTestCase` is called
   inside loops and via `Add-ThrowCase`. `TASK-1333` spec (2) reconciles it **by name**, which is the correct home.
3. **`[Parser]::ParseFile` = 0 errors** on the tracked file, and the tracked `sha256`
   `E07515…31C6` identical before/after the control work.
4. **The scratchpad lab was deleted.** Immaterial to the commit either way — the scratchpad lives outside
   the repo and cannot reach a pathspec.
5. **Byte-identity of the header's prose** — see the DECLARED RESIDUAL at §8.

---

## 1. CHECK (1) — THE THREE POSITIVE CONTROLS FIRED ✅ (check one, and it is the one that decides a green-that-cannot-fail)

All three are present, **named individually, each with its own distinct output block** — not a summary sentence.
Seven controls in total, which is more than the spec demanded:

| control | injected | reported | my analytic re-derivation |
|---|---|---|---|
| **(i)** | `CONVENTIONS.md:9999` | **FIRED**, exit 5, `54 / 55` | shape (a) matches (`(?:\s+at)?` optional) ✅ |
| **(ii)** | `CONVENTIONS.md at :9999` | **FIRED**, exit 5, caught **twice** | shape (a) via `\s+at`, **and independently** shape (b) — the colon is space-preceded ✅ |
| **(iii)** | bare `(:9999)` | **FIRED**, exit 5 | shape (b): `(` is outside the lookbehind class ✅ |
| **(iv)** bonus | `CONVENTIONS.md:29` — **two digits** | **FIRED** | shape (a) uses `\d+`, no width constraint ✅ |
| **(v)** bonus specificity | `EngineThing.cpp:1234` | **NOT FIRED** (must stay green) | shape (a): wrong filename; shape (b): `p` precedes the colon ✅ |
| **(vi)** bonus specificity | clock `07:15:09` | **NOT FIRED** | shape (b): digit-preceded, both colons ✅ |
| **0** baseline | nothing | **NOT FIRED** | the copy is faithful ⇒ a later red is attributable ✅ |

⭐ **Control (ii) being caught TWICE is the detail that most raises my confidence the harness really ran.**
It is a non-obvious consequence of the two shapes overlapping on the ` at :` spelling, it is reported as an
oddity rather than smoothed over, and it is exactly what the shipped regexes predict. A fabricated report
does not volunteer that.

⭐ **And the harness's own defect report (§5 of the handoff) is the strongest single piece of evidence in
the note:** the first detector matched `'\sNO\s'`, PowerShell's `-match` is case-**insensitive**, and it hit
the `no` in the case's own name `"grows no rot-prone line address"` ⇒ it reported **FIRED on every row,
including the green ones**. The row noticed that its control harness could not tell green from red, and
fixed it to a positional read. That is `SC-§39` applied to the *instrument* rather than to the subject,
caught by the row and not by me — and it is the exact species (`SC-§126` cl. 9) this chain exists to fight.

**The controls satisfy `SC-§126` cl. 9(b-2)'s combined rule precisely:** (i)–(iv) are the **positive**
controls that prove the pattern *covers the target's shapes* — the control the law says everybody skips —
and (v)–(vi) are the **specificity** controls that prove it discriminates. Both questions answered, separately.

## 2. CHECK (4) — THE NEGATIVE CONTROL PASSED ✅ (and I re-derived it independently, `SC-§39`)

The case must **pass** on unmodified tracked bytes with the exhibit, the engine addresses and the clock
times all present. Reported: `exit 0`, `55 / 55`, detail `header derived as lines 1..238; 6 exhibit line(s)
excluded by substring, never by address`.

**I re-derived that verdict from the shipped patterns against today's file, line by line:**

| header line | text | shape (a)? | shape (b)? | why |
|---|---|---|---|---|
| `:19` | `…ParseExecCommands.cpp:29` | no | **no** | `p` precedes the colon — lookbehind excludes |
| `:21` | `…EditorServer.cpp:5993` | no | **no** | same |
| `:104` | `…09-18 23:42..23:44…` | no | **no** | digit-preceded |
| `:121` | `…opens at 04:55:38,` | no | **no** | digit-preceded |
| `:122` | `…is 04:55:58 (20 s)…` | no | **no** | digit-preceded |
| `:146` | `CONVENTIONS.md at :5966-5969 … :6172-6175` | *would fire* | *would fire* | **EXEMPT — inside the exhibit block** |

⇒ **zero non-exempt matches. The case passes on `HEAD`, derived rather than inherited.** The ban list is
right, and — the point of (3b) — **the check is shippable.** A check that cannot pass is as broken as one
that cannot fail.

**The exhibit block arithmetic, walked by hand and confirmed at exactly 6 lines:** `:143` blank ·
`:144` marker · `:145–:149` body · `:150` blank. Upward walk stops immediately (`$a=144`); downward walk
stops at `:149` (`$lines[149]`=`:150`=blank). Exempt = `144..149` = **6** — matching the published count.
The `-and` short-circuit correctly prevents the `$lines[$a-2]` → `$lines[-1]` negative-index trap at `$a=$first`.

**Did the row "fix" the prose instead?** No — and the temptation it names was never in the header at all
(NIT-6). Position-wise this is settled: the block-comment ledger is **byte-for-byte identical to
`TASK-1329`'s pre-diff reading for all 13 pairs**, so **zero lines were added or removed anywhere at or
above `:1065`**, which partitions `1..1065` into 13+ intervals each with zero net delta. Residual at §8.

## 3. CHECK (2) + (3) — NO ADDRESS IS AN ANCHOR ✅ · THE BOUNDARY IS DERIVED ✅

**`Grep '\b146\b|\b238\b'` over the whole file returns exactly two hits, `:1132` and `:1137`, both inside
the new doc comment's prose. Zero in executable code.** The exclusion key is the substring
`ANCHOR TO QUOTED TEXT`; the boundary is the parser's answer. Neither is a number.

The derivation, quoted:

```powershell
$null = [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref] $tokens, [ref] $errors)
foreach ($t in $tokens) {
    if ($t.Kind -eq [...TokenKind]::Comment -and $t.Text.StartsWith('<#')) { $header = $t; break }
}
$first = $header.Extent.StartLineNumber
$last  = $header.Extent.EndLineNumber
```

⭐ **Asking the language's own tokenizer, instead of scanning for `<#`, is the right call and is better
than the spec asked for** — nesting, same-line delimiters and a leading `#` line comment are the parser's
problem, not a guess. It skips non-block comments correctly (`StartsWith('<#')` filters, the loop only
`break`s on a block comment). **And it fails CLOSED:** no leading block comment ⇒ the case REDS with
`FAIL-CLOSED: … the header could not be bounded`, never "clean" (`SC-§39`).

**The substring choice is sound.** `ANCHOR TO QUOTED TEXT` occurs **once in the file** (I confirmed: one
hit at `:144`, plus one *reference* to it at `:1127` inside the new comment, which is outside the scanned
range). It contains **no digit and no colon**, so the exclusion key can never itself become an address —
a small, genuinely elegant property. And it is load-bearing prose: it is the verbatim restatement of
`SC-§126` cl. 7 that the exhibit exists to illustrate.

**§3's "structurally incapable" sentence is correct as implemented:** exempt lines are dropped from the
corpus **before** `[regex]::Matches` runs, so no finding the case can emit is able to name the exhibit's
two dead addresses. That discharges the deeper fence (`TASK-1328` NIT-4 · `SC-§126` cl. 9's sibling
hazard: *an address being wrong may be the point*). One boundary condition qualifies it — **WARN-2**.

## 4. ⚖️ THE `:5122` vs `:5093` RESOLUTION — **THE ROW IS RIGHT AND THE BOARD'S CLAUSE (1) IS WRONG**

The row corrected the board's example from `CONVENTIONS.md:5122` to **`CONVENTIONS.md:5093`** and used its
own measurement. **Confirmed, four ways, none of them requiring `git`:**

1. `handoffs/TASK-1324-programmer.md:52` quotes the deleted text verbatim:
   `tool runs" (CONVENTIONS.md:5093), and the correction that caught that then quietly`
2. `TASKBOARD.md:4259` — `TASK-1324`'s **own spec**: *"cites `CONVENTIONS.md:5093` for the sentence 'the
   file exists' was read as 'the tool runs'. `:5093` is the `SC-§96` FACADE-COUNTER LAW — a different
   section entirely. The sentence lives at `:5122-5123`."*
3. `handoffs/TASK-1318-buildmaster.md:239` and `handoffs/TASK-1326-buildmaster.md:173` both name the
   removed string as `CONVENTIONS.md:5093`.
4. **The header's surviving text is the clincher.** `:173-174` today reads *"`the file exists` was read as
   the `tool runs`, and the correction that caught that then quietly welded…"* — **the deleted parenthetical's
   exact context, minus the parenthetical.** The wound matches the corpse.

🚨 **And the correction matters more than a digit.** `CONVENTIONS.md:5122` **was never in this file.** It is
the number `TASK-1324` was explicitly forbidden to write — the would-be **fourth** wrong address in
`CONVENTIONS.md`'s own ledger `:5072 → :5093 → :5122 → :5140` (`SC-§126` cl. 7, fourth instance). Had the
row obeyed the board, it would have recorded as ban-list provenance **a citation that never existed**,
inside the comment whose entire subject is false provenance. ⇒ **This is `SC-§101` working exactly as
designed: a prescribed remedy is a CLAIM, the row measured it, and the board was wrong.** Endorsed without
reservation; routed to the manager as a board correction (his edit, not mine — `SC-§100`).

## 5. 🚨 CHECK (7) — THE TRIGGER. THE CENSUS IS CONFIRMED BY MY OWN MEASUREMENT, AND I RULE ON THE COMMIT

**My independent `Grep 'SelfTest'` over `.claude/pipeline/` returns, excluding `TASKBOARD.md` and the row's
own handoff, exactly five files:**

> `handoffs/TASK-1294-programmer.md` · `handoffs/TASK-1183-buildmaster.md` · `handoffs/TASK-1181-programmer.md`
> `qa/TASK-1295-report.md` · `qa/TASK-1182-report.md`

⇒ **3 handoffs + 2 QA reports — the row's numbers exactly.** And **zero** occurrences in `TASK-1310`,
`1317`, `1318`, `1324`, `1325`, `1326`, `1327`, `1328`, `1329` — confirmed by their absence from the file
list, which is a stronger form of the claim than a per-file count.

⇒ **CONFIRMED, at my own instant: `-SelfTest` was run by 1 of the 4 rows that ever touched this file, and by
NONE of the three header-prose rows this guard exists to catch — nor by their gates, nor by their hosts.
As practice stands, the guard would have been run by ZERO of its intended subjects.**

The inversion the row found is the load-bearing part: **rows that changed the runner's CODE ran `-SelfTest`;
all three rows that changed only its HEADER PROSE did not.** The rot lives in the prose.

### ⚖️ MY RULING — and I am explicit that the trigger **rule** is the manager's (`SC-§100`), not mine

⛔ **I do NOT fail the row for this.** The programmer was asked to measure and report, and it did exactly
that — it refuted the manager's provisional rule with evidence, declined to invent a replacement, and said
so in plain words. That is `SC-§100` honoured, not breached.

**ON THE QUESTION ASKED — may `TASK-1333` commit before the trigger rule exists? ✅ YES. Commit it.**
Four reasons, and the third is the one I would defend hardest:

1. **This is NOT the `TASK-1286` shape, and the difference is structural, not a matter of degree.** In
   `TASK-1286` the entry path was **unreachable**: the inertness was a property of the *code*, and no
   convention could have cured it. Here the lane exists, is invoked today by code-change rows, and the
   mechanism is proven to fire on demand. **The inertness is a property of PRACTICE, not of the artefact.**
   `SC-§123` is correctly cited — *reachability is not correctness* — but it bites an artefact that cannot
   be invoked. This one can be, and is.
2. **`TASK-1333`'s own spec (2) already runs it** — *"RUN `-SelfTest` AND QUOTE THE WHOLE LEDGER … THIS IS
   THE ROW'S REAL ACCEPTANCE."* ⇒ the guard's **first invocation is already boarded**, on the very row that
   commits it. "Run by zero subjects" is therefore not true even on day one.
3. ⭐ **An UNCOMMITTED guard is strictly MORE inert than a committed one, and there is no state in which
   withholding the commit increases the number of rows that run it.** Holding it leaves a reviewed,
   control-proven artefact loose in a working tree that **already carries one held lane** (`TASK-1314`) —
   which is a live loss risk, not a hypothetical one. Withholding buys nothing and spends something.
4. **`SC-§70`'s half a fact cuts both ways, and the remedy for half a fact is to supply the other half —
   not to discard the half you hold.** The manager's boarding text says *"six hand-corrections and no
   enforcer is `SC-§70`'s half a fact."* The row returns the mirror: *an enforcer nobody runs is half a fact
   too.* Both are right. The resolution is the rule, and the rule is cheap.

**⇒ COMMIT, BUT DO NOT RECORD THE CHAIN AS CLOSED AT `TASK-1333`.** The commit ships a **proven mechanism**;
it does **not** ship an **enforcement**. My recommendation to the manager, offered as recommendation only:

- **Board the trigger rule now, in parallel — not "after".** The precise failure mode of this chain is a
  correct artefact whose accompanying obligation is forgotten, and `SC-§50` is unambiguous that **the fence
  is a ROW, not a memory.** A rule that waits on someone remembering is `TL-§5e` cl. 7a's *habit, not a rule*.
- ⭐ **And the row's measurement tells you the SHAPE the rule must take, which is the dividend of clause (7)
  being asked at all:** key it on **"any row whose diff touches this file"**, never on *"any row that changes
  this file's code."* The code-keyed rule is precisely inverted against the evidence — all three rot events
  were **prose-only** edits, and a code-keyed trigger would have caught **none** of them while feeling correct.
- The gate half of the provisional rule (*"its gate checks the ledger line was quoted"*) is sound and costs
  a gate one grep.

## 6. CHECK (5) — THE CENSUS, RE-TAKEN AT MY OWN INSTANT, WITH SPECIES LABELS

`Grep ':[0-9]'` over `run_suite_bounded.ps1`, restricted to the derived header `1..238`:

| # | line | species | in ban list? |
|---|---|---|---|
| 1 | `:19` | engine-source, **2 digits** (`ParseExecCommands.cpp:29`) | no — declared out of scope |
| 2 | `:21` | engine-source, 4 digits (`EditorServer.cpp:5993`) | no — declared out of scope |
| 3 | `:104` | **clock time** (`23:42`, `23:44`) | no — not an address |
| 4 | `:121` | **clock time** (`04:55:38`) | no — not an address |
| 5 | `:122` | **clock time** (`04:55:58`) | no — not an address |
| 6 | `:146` | **THE EXHIBIT** — pipeline-markdown, 2nd spelling | **excluded: it is EVIDENCE** |

⇒ **six lines, four species — it matches the board's six and the row's six exactly.** Three independent
readers at three instants, one reading. (Note `:103`'s `16:xx` is correctly *not* a hit — the colon is not
followed by a digit. The 401-digit trap in yet another hat.)

⭐ **And the census corroborates the row's central thesis for free, which is what `SC-§126` cl. 7's fourth
instance asks a gate to do:** the nominated `:[0-9]{3,4}` **misses row 1** (two digits) **and fires on row 6**
(the exhibit) — wrong in both directions on this one file. The row's design — `\d+` with the discrimination
moved onto **what precedes the colon** — excludes the out-of-scope species **by construction**, which is the
correct answer to cl. 9(b-2) and is strictly better than sweep-and-subtract.

## 7. CHECK (6) — FENCES

| fence | verdict | basis |
|---|---|---|
| no parameter default / bound changed | ✅ | pure-insertion below `:1065`; declared residual §8 |
| no `-ExecCmds` composition, no comma-vs-semicolon separator, no `QUIT_EDITOR` (`SC-§116`) | ✅ | same |
| no existing self-test case edited, renamed or reordered | ✅ | the new block sits **after** W-10 (`:1602`) and **before** `Write-Head 'SELF TEST RESULT'` (`:1615`) — appended last |
| diff = ONE file + the handoff | ✅ | session-start `git status` snapshot: `M Tools/run_suite_bounded.ps1` + `?? handoffs/TASK-1331-programmer.md` — **provided to me in context, not a command I ran** |
| `TASKBOARD.md` / `CONVENTIONS.md` / `Saved/**` / `.uasset` / `.cpp` absent | ✅ | same snapshot; the other dirt is `TASK-1314`'s held lane + the manager's board/law edits |
| ZERO files from `TASK-1314`'s or `TASK-1319`'s diff | ✅ | `SiegePlayerController.cpp`, `qa/TASK-1314-verify.md`, `qa/TASK-1315-report.md`, `handoffs/TASK-1329-buildmaster.md` untouched — **held, not orphaned** (`TL-§5e` cl. 7a-v) |
| the header's prose NOT "fixed" (spec (5)'s easy trip) | ✅ | positionally proven; §8 residual |
| `-SelfTest` **process** purity confirmed at the row's own instant (`SC-§118`) | ✅ | `Invoke-SelfTest` holds zero `Start-Process`/`Stop-Process`/`.exe` constructs; `TASK-1316`'s held verify leg undisturbed |
| no compile, no suite, no git, no commit | ✅ | all `TASK-1333`'s |

## 8. ⚠️ DECLARED RESIDUAL — routed to `TASK-1333`, which holds the tool I do not

**An equal-line-count substitution below `:238` is invisible to every check in this report.** I have no
`Bash` ⇒ no `git diff`, and I did not route git through the read-only inspector. My strongest available
corroboration is M3: the 13-pair block-comment ledger is **identical to `TASK-1329`'s pre-diff reading**, so
every interval between consecutive delimiters has **zero net line delta** — which excludes insertion and
deletion, but not a same-line-count rewrite inside an interval.

✅ **`TASK-1333` closes it, and its spec already does so by design, without needing to be told:**
(1) `git diff` over the whole file · (2) the **full `-SelfTest` ledger reconciled BY NAME, never by count**
(`SC-§104`) — which is exactly the instrument that catches a silently rewritten or dropped self-test case ·
(3) one real suite run through the edited script. **This is the identical residual `TASK-1328` declared and
`TASK-1329` discharged, in the identical way.** It is a limit on *my* provenance, not a defect in the row.

---

## Findings

- **[WARN-1]** `Tools/run_suite_bounded.ps1:1132-1133` and `:1137-1139` — **two claims in the new doc comment
  are FALSE AS WRITTEN, and I measured it rather than inferring it.** The comment says the exhibit
  *"moves again under this very diff"* and that the header boundary *"moves with every edit to this file,
  **including this one**."* **Measured:** this diff inserts only at `:1089` and `:1604`, both far **below**
  the header; `TASK-1329`'s pre-diff block-comment ledger and mine agree on **every one of the 13 pairs**
  ⇒ the header ended at `:238` before and ends at `:238` now, and the exhibit sat at `:146` before and sits
  at `:146` now. **Neither moved.** The general form is also overbroad: the boundary moves with edits
  **above** it, not with every edit.
  ⇒ *Suggested fix, two words:* *"…it moved +3 under `TASK-1327`, and the next edit above it moves it again"*
  and *"…it moves with every edit **above it** in this file."*
  ⇒ **Not a blocker:** no behavioural consequence, the guard is correct, the over-claim errs toward *more*
  caution, and it prescribes nothing. But it is recorded prominently because it is **precisely the species
  this chain fights** — `SC-§126` cl. 9's *"a false claim that happens to accompany a correct artefact is the
  one nobody has an incentive to catch"* — and because `qa/TASK-1325-report.md` set the precedent of recording
  exactly this as a **WARN with the verdict unmoved**. It is a sentence about *this* file written in the diff
  that carries it, which is cl. 8's deterministic shape, not cl. 7's concurrent one.
- **[WARN-2]** `Tools/run_suite_bounded.ps1:1169-1179` — **the exhibit exclusion fails OPEN (then loud) if the
  anchor substring is ever deleted or reworded.** If no line matches `ANCHOR TO QUOTED TEXT`, `$exempt` is
  silently empty, `:146`'s two addresses are scanned, and the case REDS **naming them** — i.e. it emits the one
  finding §3 promises it is *"structurally incapable"* of emitting, and invites the seventh hand-edit of the
  header that this row exists to prevent. Mitigated but not removed by publishing `0 exhibit line(s) excluded`
  in the ledger line.
  ⇒ *Suggested fix, one line, and it is the treatment the header boundary already gets:* if the marker is not
  found inside the header, return the **FAIL-CLOSED** shape with *"exhibit anchor not found"* rather than
  scanning. That makes §3's claim true unconditionally and is `SC-§39` applied consistently across both of the
  function's two anchors instead of only one.
- **[NIT-1]** `:1138` — *"out of 13 block-comment delimiter pairs."* **Measured today: 14** (`^<#` → 14, `^#>` → 14,
  the 14th being this comment's own `1089`/`1145`). The handoff §4 has it right (*"14 … 13 before my diff, +1"*);
  the shipped comment is the transcription slip. Since the sentence says *"at this writing"* and it ships **inside**
  the file it counts, `14` is the true value.
- **[NIT-2]** `:1183` — shape (a)'s `\s*:\s*\d+` admits ordinary English: *"see `CONVENTIONS.md`: 3 rows apply"*
  would RED. By the row's own argument a false red is the expensive direction here. `(?:\s+at\s*)?:\d+` keeps
  **both** banned spellings and rejects the prose form.
- **[NIT-3]** `:1183` — `[regex]::Matches` is **case-sensitive** in .NET (unlike PowerShell's `-match`, the very
  asymmetry that bit the row's own control harness). `conventions.md:99` / `taskboard.md:99` would slip. One
  `(?i)` on shape (a) — **not** on shape (b), which needs none.
- **[NIT-4]** `:1151` — `$errors` is captured and discarded. `ParseFile` can return tokens **and** errors, so the
  case can report "clean" about a file that does not parse. `TASK-1333`'s parse control covers this commit; one
  line (`if ($errors.Count) { <fail-closed> }`) would make the case self-sufficient.
- **[NIT-5]** `:1609-1612` — the new case is the **only** one in `Invoke-SelfTest` with no `try/catch`. House style
  (e.g. W-10 at `:1593-1601`) catches and reds the single case; here a throw from `ReadAllLines`/`ParseFile` (a
  locked or concurrently-written file) aborts the **whole ledger** as exit 70 `INTERNAL_ERROR`. Loud and
  documented, so not a hazard — but it is a departure from the file's own convention.
- **[NIT-6] — an attribution correction that CLOSES a would-be row rather than opening one.** Handoff §9 reports,
  honestly, that the lane *"is **not** 'no disk access' as the header says."* **Measured:** that phrase is at
  **`:524`**, inside **`Resolve-SafeLogPath`'s** doc comment — *"Pure string math -- no disk access -- so `-SelfTest`
  covers it"* — and it is a claim about **that function**, not about the lane. Its sibling, *"Pure. `-SelfTest`
  drives it with no process,"* is at **`:963`** on **`Merge-BoundIntoVerdict`**. **Neither is in the header**
  (both sit far below `:238`), and **neither asserts the lane performs no I/O.** The header's own `-SelfTest`
  example (`:236-237`) says *"Launches no engine, no process"* — **accurate as written.** ⇒ **Nothing is false;
  no corrective row is needed.** The row's restraint in not editing it was correct regardless of the reason, and
  the honesty that surfaced it is what let the question be closed for free.

## ⚖️ The §8 design trade — RULED: **ACCEPT THE BLOCK. DO NOT NARROW IT.** And the offered narrowing must NOT be taken.

The row flagged, rather than buried, that the exclusion is a **block** (6 lines) not a line, so *"a genuinely
new citation added inside that one paragraph would be exempt,"* and offered to narrow it to the marker line
as a one-line change. **My ruling: keep the block, and `TASK-1333` should not accept the offer.**

1. 🚨 **The offered narrowing is not merely riskier — it is WRONG AS DESCRIBED, and would red on today's `HEAD`.**
   The exhibit's two addresses are on **`:146`**; the marker is on **`:144`**. Exempting *"the marker line only"*
   leaves `:146` in the corpus ⇒ shape (a) fires on `CONVENTIONS.md at :5966-5969` ⇒ **the negative control (3b)
   FAILS**, which the board makes a blocker on the ban list. Any marker-line-plus-offset variant (`+2`) is an
   **address in disguise** wearing the exact defect the row guards. ⇒ **the block is not a convenience, it is the
   only anchor available that is both stable and address-free.** Recorded explicitly so no later row "tightens"
   this and reds the file.
2. **The residual exposure is the smallest available and is self-limiting:** the six exempt lines are the one
   paragraph in the project whose entire subject is *"never cite by address."* A fresh citation written **there**
   is written into the sentence forbidding it.
3. ⭐ **The trade is METERED, which is what makes a wide exclusion acceptable rather than merely convenient.**
   `6 exhibit line(s) excluded by substring` is printed **on every run, pass or fail**. If the paragraph ever
   grows and widens the exemption, **the number visibly changes in the ledger.** A published count is a cheap
   tripwire on the exemption itself.
4. **`SHIP-§9` — validate against the failure that has teeth.** A false red on this header invites a seventh
   hand-edit of it, which is the precise outcome this row exists to end. The row sized the trade against the
   failure mode with history behind it. Correct call.

## Notes for build-master (`TASK-1333`)

- ✅ `Verdict:` = **PASS** over `TASK-1331` — **0 BLOCKER** · 2 WARN · 6 NIT. Grep the **token**, never a line
  address (`SC-§126` cl. 10). Nothing here fences your commit.
- ⛔ **None of the 2 WARN / 6 NIT is an edit request, and none is yours to make.** Every one is prose or a
  one-line tightening inside `Tools/run_suite_bounded.ps1`, i.e. **programmer work on a future row**. ⛔ Do not
  hand-edit this file to discharge them — a host "just fixing" this header is the seventh hand-edit wearing a
  build-master's hat.
- 🚨 **YOU CLOSE MY RESIDUAL (§8), and your spec already asks for it:** the `git diff` over the whole file, and
  the **full `-SelfTest` ledger reconciled BY NAME** (`SC-§104`). **That name-reconciliation is the only
  instrument in this chain that can catch an equal-line-count rewrite of an existing self-test case — a total
  would hide exactly that.**
- ⭐ **Your parse control: break the FINAL `#>`, never the header's own** — and **RE-GREP it, because it MOVED
  under this diff.** It was `:1061` at `TASK-1325` and `:1065` at `TASK-1329`; **I measure the final `#>` at
  `:1145` today**, because the new helper's doc comment (`:1089`–`:1145`) is now the last block comment in the
  file. `:1065` is **no longer the final close.** Breaking the header's own `#>` re-pairs against the next of
  **14** (no longer 13) block comments and returns a **silent 0** — a control that cannot fire, measured twice.
- Expect **no** suite-count delta: the row adds **no** UE automation test. The `54 → 55` movement is the
  **`-SelfTest` ledger**, not the 555-test suite. Do not reconcile one against the other.
- The `LF will be replaced by CRLF` warning on this file is **pre-existing**, not from this diff.
- ⚖️ **Two items for the manager, neither blocking your commit:** (a) the board's clause (1) example
  `CONVENTIONS.md:5122` is **wrong** — the text `f5697f3` deleted was `CONVENTIONS.md:5093`, confirmed four ways
  (§4); (b) **the trigger rule (§5) is unset, and my ruling is that you may commit anyway** — but the chain is
  **not closed** at your commit, and the rule should be keyed on *"touches this file"*, never on *"changes this
  file's code."*

## Flips performed by this gate

`TASK-1331` was `ready-for-qa` **in fact** — its `names` block fences it from `TASKBOARD.md`, so it could not
flip itself (handoff §10). Both flips are mine, made with `Edit` on task-ID-bearing anchors, ⛔ never
`replace_all` (`SC-§127` — the bare `- status: backlog …` line collides across rows), and each read back as
**STATE** afterwards (`SC-§104`):

- `TASK-1331` → **`qa-passed`**
- `TASK-1332` → **`qa-passed`**
