# QA Report — TASK-1323
Verdict: PASS — 0 BLOCKERS, 1 WARN, 2 NOTE

subject: TASK-1310 (REV-2, fix loop 1 of 3)
gate: TASK-1323 (delta re-gate) · prior gate: TASK-1317 (FAIL, 2 BLOCKERS) · host: TASK-1318 · reviewer: qa-reviewer · 2026-09-19

---

## 0. SCOPE — DELTA ONLY, AND I HELD THE FENCE

`TASK-1317`'s passed checks **stand and were not re-litigated** (board `TASK-1323` (1); `SC-§59` cl. 5).
I did **not** re-open the kill evidence, the census reconciliation's correctness, or the three-way
split's correctness. I judged **the two blockers and the delta**, and I confirmed the delta did no
collateral damage.

⚖️ **Prompt-vs-board:** they agree throughout. The one place a naive reading of the dispatch prompt
would have diverged — *"the numbers to write are `:1647`/`:1644`"* — the board pre-empts at (2) and
the prompt itself retracts. **The board wins, and the row was right not to ship the prescribed pair.**

---

## 1. ⛔ BLOCKER-1 — **CLEARED. I RE-GREPPED BOTH SITES MYSELF; I ACCEPTED NOTHING ON THE HANDOFF'S WORD.**

**My own greps, at my instant (`SC-§91`), against the shipped 1,815-line file:**

| site | my measurement | what the comment ships (`:87-88`) | agree |
|---|---|---|---|
| launch — `Start-Process -FilePath $EditorCmd` | **`:1680`** | `:1680` | ✅ |
| receipt — `Set-Content -LiteralPath ($LogPath + '.cmdline')` | **`:1677`** | `:1677` | ✅ |

⇒ ✅ **The shipped pair is correct against the file that carries it.**

🚨 **And the trap the board warned of is real, so I state explicitly how I judged it.** I checked the
shipped numbers against **the current file**, **not** against `TASK-1317`'s prescribed `:1647`/`:1644`.
Had the row obeyed the prescription it would have shipped a number **true when the gate measured it
and false when the row committed it** — its own fix inserts 33 lines above the launch site. The
prescription was a **remedy that prescribed a defect** (⭐ `SC-§126` cl. 8, minted off this event).
**`:1647`/`:1644` were the pre-fix truth and are now wrong; `:1680`/`:1677` are right. Shipping the
gate's pair would have been the failure.**

**Independent corroboration that the fixed point is genuine, not two agreeing typos** — I re-derived
the displacement from a source neither the row nor the prior gate controls:

- pre-edit (HEAD) addresses, recorded by a third party: `qa/TASK-1295-report.md:98` and `:270` —
  launch `:1577`, receipt `:1574`.
- **`1577 + 103 = 1680` ✅ · `1574 + 103 = 1677` ✅.**
- the file is **1,815 lines** (my count) and HEAD's was **1,712** ⇒ net **+103**, which equals the
  declared `numstat` **114 − 11 = +103**. **Three independent routes to the same +103.**
- Against **REV-1** (1,782 lines, header `1`→`195` per `TASK-1317` §2): the delta is **+33**, and
  **every one of the twelve other block-comment delimiter pairs moved by exactly +33**
  (`270→303`, `279→312`, `329→362`, … `1014→1047`, `1022→1055`). Not one moved by anything else.

⇒ ✅ **The body below the header is displaced by *precisely* the header's growth — the signature of a
comment-only diff, measured twice against two different baselines.**

**Shape, not just digits (⭐ `SC-§126` cl. 7):** clause (a) at `:83-90` now **leads with the grep
anchors** and demotes the numbers to a dated aside that says why. Both anchors are unique in code:
`Start-Process -FilePath` → `:1680` only; `+ '.cmdline'` → `:1677` only (each also matches its own
citation line, which is the intended self-reference).

**Provenance relabelling (the board's *"relabelled without being re-derived is a BLOCKER"*):**
handoff §10b **retracts** the false provenance in plain words — *"`:1646`/`:1643` were NOT read — they
were `:1577 + 69` … a DERIVED number reported as MEASURED"* — and §10a shows **the commands and their
output** for the re-derivation. This is a re-derivation, not a relabelling. ✅

---

## 2. 🚨 BLOCKER-2 — **CLEARED. ALL FOUR ANCHORS RESOLVE BY SUBSTRING. `SC-§126` cl. 7 TEST APPLIED, ONE GREP EACH.**

The divergence paragraph is gone; `:133-167` now publishes the **resolution**, dated, and says the
sentence *was true when written and false a few hours later*. It asserts **no** *"the law still reads
Y"* construction and **no** load-bearing `CONVENTIONS.md:<line>` pointer.

**My four greps against `.claude/pipeline/CONVENTIONS.md`, at my instant — one each, exactly as cl. 7
exists to make possible:**

| anchor quoted in the comment | resolves | hits | my line |
|---|---|---|---|
| `STRUCK 2026-09-19 BY THE MANAGER` | ✅ | 1 | **`:6204`** |
| `FALSE SENTENCE 1` | ✅ | 1 | **`:6205`** |
| `FALSE SENTENCE 2` | ✅ | 1 | **`:6206`** |
| `WHAT REPLACES THEM` | ✅ | 1 | **`:6207`** |

⇒ ✅ **4 / 4, unique, and the quoted glosses match the law's content** (`FALSE SENTENCE 2` reads
*"every one of those 19 runs terminated normally … Measured 21 of 22; the 22nd IS the kill"*, which is
what the comment says it says).

🚨⭐ **AND THE LAW MOVED A FIFTH TIME, BETWEEN THE DISPATCH PROMPT AND ME.** Recorded because it is
the measurement that settles the design question:

| reader | instant | where the anchors sat |
|---|---|---|
| `TASK-1317` (prior gate) | 2026-09-19 | `:6185-6189` |
| `TASK-1310` (the programmer) | 2026-09-19, later | `:6192-6195` |
| the manager / this dispatch | 2026-09-19, later still | `:6197-6198` |
| **me, now** | 2026-09-19 | **`:6204-6207`** |

⇒ ⚖️ ***The text moved four times in one day and every substring anchor still resolved, unchanged.
A line number written into that file this morning would be wrong by lunchtime, and nothing —
no compile, no parse, no suite — would have gone red. The fix is correct in kind, not merely in
content.*** This is why I marked the board's own stale citations false rather than renumbering them.

⚖️ **Authorship:** the board records BLOCKER-2 as **the manager's**, not the author's. I judged the
artefact only, and it discharges cleanly. It does not colour anything else in this review.

---

## 3. THE THREE THINGS THAT WOULD HAVE RED A CORRECT FIX — CHECKED AS **STATE**, NEVER AS A TALLY (`SC-§104`)

**(a) The `NOT MEASURED — NULL WITH NO POSITIVE CONTROL` phrase — 3 occurrences, and 3 is correct.**

| line | attaches to | is that bound actually `NOT MEASURED`? |
|---|---|---|
| `:123` | *"The BOOT and STALL bounds are …"* — `(c2)`'s verdict | ✅ yes |
| `:161` | `-BootSeconds` in the quoted replacement law | ✅ yes |
| `:162` | `-StallSeconds` in the quoted replacement law | ✅ yes |

Every occurrence carries the **U+2014 em dash** verbatim. **I asserted the state of each bound, not
the count** — a gate counting `2` and failing the row would have been wrong, because the vanished
occurrence was the **quotation of the struck sentence** living inside the paragraph BLOCKER-2 ordered
deleted. Preserving it would have re-shipped the struck claim. The row **declared** this deviation
(§10e) rather than performing it silently (`SC-§121` cl. 5). ✅ **Correct call, correctly declared.**

**(b) The sidecar glob returning 24 is expected and is not a third number.** The comment's `22` is
**instant-stamped** (*"Census taken 2026-09-19 for TASK-1310"*) and the block itself states at
`:102-103` that *"This count only ever GROWS. A larger one is not a contradiction; a SMALLER one means
somebody deleted evidence."* `22 + 2` (`TASK-1313`'s 12:12 / 12:13 runs today) `= 24`, pre-reconciled
in §10g. ⛔ **I did not demand the comment say `24`** — that number is false tomorrow; the instant
stamp *is* the fix. The reconciliation at `:97-103` (`15 + 4 = 19`, `19 + 3 = 22`) is **unchanged**
from what `TASK-1317` verified. ✅

**(c) NIT-1 is homed, not dropped, and not actionable this loop.** §10g measures the `…211827.log`
gap as **four** lines, not two, and declines the fix on `TASK-1317`'s own stated condition
(*"fix only if `:120-125` is being edited anyway"*) because that range is `(c2)`, fenced. **Correct.
Not raised as new, not failed.** Owner: whoever next edits `(c2)`.

---

## 4. THE TWO CARRIES

**The census-range trap — HELD.** `:96` reads `spanning 20260909-045216 to 20260918-234424`.
⛔ **It did NOT sync to `…045326`.** The command-lane sidecar `…045326` appears at `:106`, correctly
labelled as the **command-lane receipt**, which is what it is. ⇒ ✅ **The code was right, the law text
was the copy that drifted, and the row did not "fix" itself to match a wrong law.**

**No collateral damage — each item measured:**

| must survive | measured | verdict |
|---|---|---|
| three-way split intact | `:111` `(c1) … OVERALL … MEASURED` · `:123` `(c2) … NOT MEASURED — NULL …` | ✅ intact |
| census + reconciliation | `:95-96`, `:97-103` — matches `TASK-1317`'s verified text | ✅ unchanged |
| header block bounds | `<#` `:1` → `#>` **`:228`**; changed lines max **`172`** | ✅ all inside |
| first executable line | `[CmdletBinding(…)]` at **`:230`**, `param(` at **`:231`** | ✅ untouched |
| bounds unchanged | `$OverallSeconds = 1500` `:243` · `$BootSeconds = 420` `:244` · `$StallSeconds = 180` `:245` — `TL-§6`'s proven triple | ✅ untouched |
| `QUIT_EDITOR` / `-ExecCmds` composition (`SC-§116`) | `$script:Terminator = 'QUIT_EDITOR'` `:288` · `New-EditorCommandLine` `:426` | ✅ untouched |
| the two-lane separator explanation | `:23-38`, unchanged prose | ✅ untouched |
| `#19` orphan-census logic | lives at `:1766-1769`; nothing above `:228` mentions it | ✅ untouched |
| block-comment delimiters, both sides | **13 `<#` / 13 `#>`**, strictly alternating, no nesting: `1→228, 303→312, 362→372, 395→408, 510→515, 576→584, 639→643, 666→679, 787→798, 881→887, 941→954, 1022→1025, 1047→1055`. Against REV-1's census every pair moved by **exactly +33** | ✅ 13/13, unchanged |
| `Saved/**` in the diff | sidecars/logs read only; the count **grew** 22→24, never shrank | ✅ absent |
| `TASKBOARD.md` / `CONVENTIONS.md` in **this row's** diff | dirty in the tree, but they are **the manager's** and **`TASK-1311`'s** — `TASK-1317` already ruled this and I do not re-score it | ✅ not this row's |
| files from `TASK-1311` / `TASK-1314` | this diff is one `.ps1`; `SiegePlayerController.cpp` is `TASK-1311`'s | ✅ zero |

---

## 5. ACCEPTED AS DECLARED (`SC-§71b`) — NAMED, AND THE ROW THAT MEASURES THEM IS NAMED

**QA holds no `Bash`.** I accept the following as declared and **⭐ `TASK-1318` is the host that
measures them**:

- `git diff --numstat` = **1 file, 114+/11−** *(corroborated indirectly: `114 − 11 = +103` equals the
  measured `1712 → 1815` growth and the measured `+103` displacement of both launch/receipt sites —
  but the hunk set itself I did not read)*
- the exact changed-line set `6-9` / `62` / `64-172`
- `git status --porcelain`, and **nothing staged**
- `Parser::ParseFile` = **0 errors** — my delimiter census is a **count, not a parse**
- the scratchpad control runs of §10f, and that the tracked file was never written by them

**The limit of my structural corroboration, stated rather than left to be discovered:** the `+103`
invariant proves the body below the header suffered **zero net line change**. It could not detect an
equal-sized substitution below `:228`. **Only `git diff` can, and that is `TASK-1318`'s.**

**Inspector:** not used. This row touches no asset, graph, editor state or log; nothing in this
verdict rests on the editor being up, and I claim no inspection I did not perform.

---

## 6. Findings

- **[WARN-1]** `Tools/run_suite_bounded.ps1:171` — **a `CONVENTIONS.md` line citation inside the
  changed range does not resolve at my instant.** The text reads *"…'the file exists' was read as 'the
  tool runs' (`CONVENTIONS.md:5093`)"*. **Measured:** `:5093` is the **`SC-§96` facade-counter blind
  spot** law, unrelated. The sentence it means lives at **`:5122`** — *"⚖️ **THE FILE EXISTS** is NOT
  **THE TOOL RUNS**…"* — with its 2026-09-18 annotation at `:5123` (which is also the source of this
  block's *"quietly welded 'runs' to 'kills'"* phrasing).
  ⛔ **Why this is a WARN and NOT a third BLOCKER, stated so the reasoning is auditable:**
  (i) it is **not load-bearing** — the claim it supports ("this is how the block got wrong in the
  first place") is true independent of the address, and every anchor a reader is *told to use* for the
  law's current state is a verified substring;
  (ii) the row **already names this citation class as rotted evidence, not as a pointer to follow**
  (`:143` lists `:5966-5969` and `:6172-6175` as the two that rotted);
  (iii) `TASK-1317` read this same block and did not raise it — **re-opening a discharged check is the
  expensive error** (`SC-§59` cl. 5, board (1));
  (iv) the number is **inherited from the board's own spec**, which carried `:5072` for the same
  pointer — the same manager-side species already adjudicated as not the author's fault.
  **Suggested fix, for whoever next edits that paragraph (not a condition of this PASS):** replace the
  digits with the substring anchor the block already prefers — `grep -n "THE FILE EXISTS"`, which I
  verified returns **exactly 1 hit**. 🙋 **Homed here so it is not an `SC-§50` orphan.**

- **[NOTE-1]** The two em dashes in a BOM-less `.ps1` (`TASK-1317` NIT-2) are now **three** (`:123`,
  `:161`, `:162`). The reasoning is unchanged and still correct: they sit inside `<# … #>`, no CP1252
  mis-decode of `E2 80 94` can yield `#` or `>`, and the acceptance demands the phrase *in those
  words*. **Keep them.** Recorded only so the next reader does not "fix" it.

- **[NOTE-2]** `run_suite_bounded.ps1:78` states *"TL-6's matching law bullet was struck by the manager
  on **2026-09-18**"* while `:136-137` and the law itself say the strike of **both false sentences**
  was **2026-09-19**. Both are true of different strikes (the 09-18 launch-path annotation at
  `CONVENTIONS.md:5123` vs the 09-19 strike at `:6204`), so this is **not** a contradiction — recorded
  only because the two dates sit 58 lines apart and a future reader may mistake one for a typo.

---

## 7. Notes for build-master (`TASK-1318`)

1. ⛔ **YOUR POSITIVE CONTROL AS WRITTEN DOES NOT FIRE — USE CONTROL B.** `TASK-1317` §2(2)/§4 handed
   you *"parse the file with a deliberately broken `#>`"*. The row **executed** that control:
   breaking the **header's own** `#>` (`:228`) yields **0 errors**, because the file holds **13** block
   comments and the opened comment simply re-pairs against the next `#>`. Only breaking the **final**
   `#>` (`:1055`) yields **1 error** — *"The terminator '#>' is missing from the multiline comment."*
   ⇒ **A run that breaks `:228`, sees 0 errors and reads that as "my instrument is broken" gets the
   wrong answer from a correctly-written gate (`SHIP-§9`).** The manager has amended `TASK-1318` (2).
   I did **not** re-measure this — I hold no `Bash`; it is declared by `TASK-1310` §10f and it is
   yours to execute.
2. **Measure what I accepted:** `git diff --numstat` (1 file, 114+/11−), the hunk set `6-9`/`62`/
   `64-172`, `git status --porcelain`, nothing staged, `Parser::ParseFile` = 0 errors, **and one real
   suite run through the edited runner.** My delimiter census (13/13) is a count, not a parse; neither
   substitutes for the other.
3. **Stage by pathspec and sweep nothing.** The tree is legitimately dirty with
   `Source/…/SiegePlayerController.cpp` (`TASK-1311`), `CONVENTIONS.md` and `TASKBOARD.md` (the
   manager), plus several handoffs and QA reports. **None of them belong to `TASK-1310`.**
   `git show --stat HEAD` verifies the **commit**, never the index.
4. **No runtime acceptance criterion** on `TASK-1310` — it is documentation debt in a tracked tool.
   **No 5b leg**, declared rather than forgotten.

**Loop accounting:** this was the gate over **fix loop 1**. It **PASSES**, so the loop count stops at
1 of 3. No escalation.
