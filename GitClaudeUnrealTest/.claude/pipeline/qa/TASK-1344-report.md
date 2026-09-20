# QA Report — TASK-1344 (the RE-GATE over TASK-1338 fix loop 1)

Verdict: **PASS** — **0 BLOCKER** · 0 WARN · 3 NIT

> ⚖️ **`TASK-1339`'s BLOCKER-1 is DISCHARGED.** The shipped clause is **TRUE**, measured behaviourally
> against all four `return` shapes **and** — for the first time in this chain — against the **actual print
> site**, not merely the string-composition site. The 0-above / +1-below landmark table **held at every
> landmark I could name**, which is the strongest corroboration of the reverse-substitution proof available
> to a reviewer without a shell. ⛔ **`TASK-1340` UNBLOCKS.**

- **subject:** `TASK-1338` — COMMENT-QUANTIFIER-DISCHARGE, **FIX LOOP 1 ONLY** (one prose clause, zero code bytes)
- **marker:** `TASK-1344-COMMENT-QUANTIFIER-REGATE`
- **reviewer:** qa-reviewer, 2026-09-20 · **a NEW ROW, not a re-run of `TASK-1339`** (`TASK-1310`→`TASK-1323` precedent)
- **host:** `TASK-1340` · **no 5a** · **no 5b**
- **reviewed:** `Tools/run_suite_bounded.ps1` — the doc comment `1089..1176`, the changed paragraph
  `1156..1168` line by line, the whole helper `1177..1240`, the emit block `1644..1660`, **the ledger emitter
  `1026..1087`** (new instrument this gate adds), the header landmarks, the whole `-SelfTest` registration
  census · `handoffs/TASK-1338-programmer.md` **whole** (`SC-§38a`) · `qa/TASK-1339-report.md` **whole** ·
  `TASKBOARD.md` `TASK-1338`/`1339`/`1344`/`1340`/`1345`

---

## 0. WHAT I EXECUTED vs DERIVED vs ACCEPTED AS DECLARED (`SC-§71b` · `SC-§91`)

**I EXECUTED NOTHING. I have no `Bash` tool in this session** — no PowerShell, no `-SelfTest`, no `git`, no
hashing, no parser. ⛔ **I did NOT route a shell or git through the read-only `unreal_inspector`.** Five
predecessors declined; `TASK-1339` called its own the **sixth refusal**. ⭐ **This is the SEVENTH, and I say so
explicitly as the dispatch required.** A read-only editor bridge used as a shell circumvents a designed fence
rather than working around a missing tool.

⇒ Everything marked **MEASURED** below I produced myself, at my own instant, with `Grep`/`Read` over today's
shipped bytes. ⛔ **No address, count or verdict is inherited from the handoff.** Where I compare against
`TASK-1339`'s published numbers I do so **after** taking my own, as the +1/0 differential — which is the
point of check (3).

### ⛔ ACCEPTED AS DECLARED, itemised (`SC-§71b` — these words used deliberately)

1. **The header `sha256` `072F55CE…8593A2E` / 14,741 bytes through the 238th LF.** I cannot hash.
   Corroborated six other ways in §3; the residual is declared there.
2. **The reverse-substitution `sha256` `4C668F97…94D0E8B`.** I cannot hash. ⭐ **Corroborated by the landmark
   table AND by a word-for-word reflow reconstruction (§3c) — but the hash itself is declared, not verified.**
3. **`git diff --numstat` = `32 16`** and the **`-U0` hunk list `@@ -1145, -1148, -1153, -1638, -1642`**
   (lowest `1145` ⇒ zero hunks at or above `:238`). I have no git.
4. **`git status --porcelain`** and the claim that the row's delta is exactly
   `Tools/run_suite_bounded.ps1` + its handoff.
5. **`[Parser]::ParseFile` = 0 errors**, the **positive control firing on the re-derived `:1176`**, and the
   **trap on `:238` returning a silent 0** (reproduced a second time). ⭐ I re-derived `:1176` myself; the
   *outcomes* are the row's.
6. **`-SelfTest` = 55 / 55 GREEN, `RUNNER_EXIT = 0`,** and the `SC-126` detail string being emitted green.
   ⭐ **I measured that the registration count IS 55 and that `SC-126` is LAST, statically (§4); I cannot
   measure *green*.**
7. **0 CR bytes / no line-ending change**, and the scratchpad deletion.
8. **The nine controls, the 55-case name-for-name ledger, and every fence** — `TASK-1339` measured them at
   its instant and the board rules them out of scope; I did **not** re-run them and do **not** re-score them.

---

## 1. 🚨 CHECK ONE — IS THE CLAUSE TRUE AGAINST THE CODE? **YES.** All four returns driven by hand, *derived?* and *printed?* answered SEPARATELY.

### 1a. The clause, located by QUOTED TEXT (`SC-§126` cl. 10/11)

`Grep 'EVERY RUN THAT DERIVES'` ⇒ **one hit**, at the line that reads
`THEM, and the boundary itself is printed in the ledger line on EVERY RUN THAT DERIVES`, continuing
`ONE, pass or fail, which is the one place it cannot go stale.` ⛔ I took no address from the handoff; the
final `^#>` I **re-derived** (§3) and it is the **sixth** value.

### 1b. The four `return` shapes of `Get-HeaderRotProneAddress`, read out of the shipped bytes

| # | path | return shape (read, not quoted from the row) | **(i) DERIVED a boundary?** | **(ii) boundary clause PRINTED?** | (i) ⟺ (ii) |
|---|---|---|---|---|---|
| **1** | **parse error** | `Ok=$false; First=0; Last=0; Exempt=0; Why=@('FAIL-CLOSED: the parser reported {0} error(s)…')` — returns **before** `$first`/`$last` exist | ❌ **NO** — the assignment `$first = $header.Extent.StartLineNumber` is **downstream** of this return; `0` is a **literal**, not a measurement | ❌ **NO** — emit predicate `if ($hdr.Last -gt 0)` ⇒ `0 -gt 0` = false ⇒ `$hdrLead = @()` ⇒ detail is `$hdr.Why` alone | ✅ |
| **2** | **no leading block comment** | `Ok=$false; First=0; Last=0; Exempt=0; Why=@('FAIL-CLOSED: no leading block comment found…')` — `$header` is `$null`, so again returns before the assignment | ❌ **NO** — same structural reason | ❌ **NO** — `0 -gt 0` = false | ✅ |
| **3** | 🚨 **anchor missing** | `Ok=$false; First=$first; Last=$last; Exempt=0; Why=@('FAIL-CLOSED: exhibit anchor not found…')` — returns the **real** derived pair | ✅ **YES** — `$first`/`$last` are assigned from `$header.Extent.Start/EndLineNumber` **two statements earlier**; on the shipped file that is `1` / `238` | ✅ **YES** — `238 -gt 0` = TRUE ⇒ lead built ⇒ `header derived as lines 1..238; `**`0 exhibit line(s) excluded`**`…; FAIL-CLOSED: exhibit anchor not found…` | ✅ |
| **4** | **happy** | `Ok=($why.Count -eq 0); First=$first; Last=$last; Exempt=$exempt.Count; Why=$why` | ✅ **YES** | ✅ **YES** — `$last` is a comment token's `EndLineNumber`, **1-based** ⇒ can never be `0` ⇒ predicate always true here | ✅ |

⇒ **(i) ⟺ (ii) on ALL FOUR. CONJUNCT D IS TRUE.**

### 1c. ⭐ THE `"pass or fail"` WITNESS — NAMED, CONFIRMED, **and there are TWO of them**

🚨 **Witness 1 (the one the dispatch named): return 3, anchor missing.** It carries `Ok = $false` — **a FAIL**
— **derives** `1..238`, and **still prints** the boundary clause. Without it, *"pass or fail"* would be pure
decoration: every other printing path would be a pass. ⭐ **CONFIRMED from the return shape, not from the
row's oracle table.**

⭐ **Witness 2, which nobody in this chain has named: return 4 with `$why.Count -gt 0`.** The happy return's
`Ok` is **computed**, not constant — if the scan finds a rot-prone address, the case **REDs** while still
having derived `1..238`, and the boundary clause prints ahead of the `line N grew a …` findings. ⇒ **the fail
side of the quantifier has two independent witnesses, on two different returns.** The clause is not
over-worded in either direction.

### 1d. ⭐ THE INSTRUMENT THIS GATE ADDS — *composing* a string is not *printing* it

`TASK-1336` verified conjunct D by observing that `$hdrDetail` is **built**; `TASK-1339` verified it by
reading the same composition block. ⛔ **Neither followed `$hdrDetail` into the function that prints it**, and
a suppression there would have falsified the clause with the composition block looking perfect. **I followed
it.** `Add-SelfTestCase` has **two** output branches: a `-Row` branch that prints a caller-supplied row and
**drops `$Detail` entirely**, and an `else` branch that prints `'{0,-62} {1,-5}{2}' -f $Name, $okText, $Detail`.
⭐ **The `SC-126` registration passes NO `-Row`** (I read the whole call line; it ends at `-Detail $hdrDetail`
with no continuation backtick) ⇒ `$Row -eq ''` ⇒ the **`else`** branch runs ⇒ **`$Detail` is printed
unconditionally, on `yes`/Green and on `NO`/Red alike.** ✅ **The "printed" column above is measured at the
`Write-Host`, not at the `-f`.**
⭐ Additionally, on a fail the detail is appended to `$script:StFails` as well, so on the anchor-missing path
the boundary appears **twice** — ledger line and failure summary. **The clause understates the truth.**

### 1e. Why `Last -gt 0` is a faithful predicate for *"derives one"* — re-derived, not inherited

The two non-deriving returns set `Last = 0` as a **literal**. The two deriving returns set it from
`$header.Extent.EndLineNumber`, which is **1-based** and therefore ≥ 1 for any real token. ⇒ **the predicate
and the concept cannot diverge**; there is no input under which `Last = 0` means "derived" or `Last > 0` means
"not derived". ✅

---

## 2. 🚨 CHECK TWO — **MY OWN CONJUNCT SPLIT** under `SC-§136`. Instrument named **per conjunct, before looking**.

### 2a. The SHIPPED string, verbatim, reconstructed from the file by concatenating its own lines

> **THE DERIVED BOUNDARY IS NOWHERE WRITTEN DOWN HERE, AND NO ADDRESS INTO THIS FILE SURVIVES IN THIS COMMENT
> AS A LIVE CITATION: the only ones pointing into this file are quoted FROM THE DIFFS THAT DELETED THEM, and
> the boundary itself is printed in the ledger line on EVERY RUN THAT DERIVES ONE, pass or fail, which is the
> one place it cannot go stale.**

⛔ Split on **its own** seams (`, AND` · `:` · `, and` · `,`) — **not** on the row's list, **not** on
`TASK-1339`'s.

### 2b. 🚨 MY SPLIT YIELDS **FIVE** SEGMENTS, NOT FOUR — and the fifth is the rider

| # | segment (verbatim) | seam that ends it | kind | **INSTRUMENT, NAMED BEFORE LOOKING** | result |
|---|---|---|---|---|---|
| **A** | *"THE DERIVED BOUNDARY IS NOWHERE WRITTEN DOWN HERE"* | `, AND` | **TEXT** (absence of a live value) | census: `Grep '238\|1089\|1176\|[0-9]\.\.[0-9]'` **file-wide**, then intersect with `1089..1176` | ✅ **TRUE** — **3 hits file-wide, ZERO in the comment** (two are date arithmetic in the header's run census; one is the quoted `"lines 0..0"` in the code comment, outside the doc comment) |
| **B** | *"NO ADDRESS INTO THIS FILE SURVIVES IN THIS COMMENT AS A LIVE CITATION"* | `:` | **TEXT** (absence of a live address) | **two** censuses: complete colon-digit `Grep ':[0-9]'` file-wide, **plus** the non-colon escape `Grep -i '\b(lines?\|at\|near\|around)\s+[0-9]+'` file-wide | ✅ **TRUE** — colon-digit: **7 lines in the comment, 13 tokens, exactly 2 into this file**; non-colon: **2 hits file-wide, NEITHER in the comment** |
| **C** | *"the only ones pointing into this file are quoted FROM THE DIFFS THAT DELETED THEM"* | `, and` | **TEXT** (provenance of those 2) | read the two carrier lines whole | ✅ **TRUE** by exhaustion — both carry the literal prefix `9d80505 (TASK-1327) deleted` |
| **D** | *"the boundary itself is printed in the ledger line on **EVERY RUN THAT DERIVES ONE**, pass or fail"* | `,` | 🚨 **BEHAVIOURAL** — a text census is **structurally incapable** of deciding it | **THE CODE PATH**: all four `return` shapes read, **plus the emit predicate, plus the print site** | ✅ **TRUE** — §1b/§1c/§1d. Sub-clause **D-ii** *"pass or fail"* got its **own** instrument (find a FAIL that derives) and **two** witnesses |
| **E** | ⭐ *"which is the one place it cannot go stale"* | `.` | **MIXED** — E1 behavioural (immunity), E2 a **universal over places** | E1: trace the `-f` arguments back to their assignment. E2: file-wide `238` census + conjunct A | ✅ **TRUE** — E1: the printed value is `-f $hdr.First, $hdr.Last`, which trace to `$header.Extent.Start/EndLineNumber`, **computed at run time, never recorded** ⇒ a computed value cannot be stale. E2: the boundary appears **nowhere else in this file** (A), and every *other* place it could be written is by construction rot-prone — which is this paragraph's whole thesis |

⇒ **My split finds FIVE segments where the row's table published four plus a separately-labelled "Rider".**
⛔ **This is NOT a new finding and I say so plainly:** the rider **was** named — for the first time — in fix
loop 1 (`handoff §0b.4`, under the heading *"Rider, enumerated rather than filed as residue"*). The dispatch
required me to **re-decide it rather than inherit it**, and I did: **I re-derived E1 and E2 with my own
instruments and reach the same verdict, TRUE.** ⛔ **There is NO sixth conjunct.** I checked the remaining
candidates and record them as presuppositions, not claims: *"the ledger line"* (definite singular — satisfied,
there is exactly **one** registration for this case) and *"the only ones"* in C (presupposes non-empty —
satisfied, there are exactly **two**).

⚠️ **One honest reservation on E, graded NIT-1, not a blocker:** *"the ONE place"* is a universal whose domain
is **places**, not a file — which is the same unbounded shape that cost this chain three loops. It is **TRUE
as written** and cheaply refutable **within this file** (one `238` grep), so demanding a fourth loop over a
true clause's quantifier *style* is exactly the "spend a loop for zero information" the board forbade.
**Recorded, not actioned.**

---

## 3. 🚨 CHECK THREE — THE 0-ABOVE / +1-BELOW LANDMARK TABLE. **IT HELD AT EVERY LANDMARK.**

⛔ I cannot hash, so I corroborate the reverse-substitution proof with the instrument I **do** hold. ⭐ I took
**more** landmarks than the dispatch listed, including **four inside the doc comment above the edit**, which
localises the `+1` far more tightly than a table that only straddles the whole comment.

### 3a. ABOVE the doc comment — must be **UNMOVED** (shift 0)

| landmark | `TASK-1339`'s address | **mine, MEASURED** | shift | verdict |
|---|---|---|---|---|
| `[int] $OverallSeconds = 1500,` | `:253` | **`:253`** | **0** | ✅ same address **and** same value |
| `$script:Terminator = 'QUIT_EDITOR'` | `:298` | **`:298`** | **0** | ✅ same address **and** same value |
| the header's matching `^#>` | `:238` | **`:238`** | **0** | ✅ |
| header `:[0-9]` census (hits ≤ 238) | 6 lines / 4 species | **6 lines** — `:19`,`:21` engine · `:104`,`:121`,`:122` clock · `:146` exhibit | **0** | ✅ **the identical six lines, a fifth reader at a fifth instant** |
| exhibit anchor `ANCHOR TO QUOTED TEXT` | `:144` | **`:144`** | **0** | ✅ |
| exhibit address line | `:146` | **`:146`**, content `CONVENTIONS.md at :5966-5969 and then at :6172-6175, and BOTH rotted with nothing` | **0** | ✅ address **and** content |
| the **13 pre-existing `^<#`/`^#>` intervals** | `1:238 · 313:322 · 372:382 · 405:418 · 520:525 · 586:594 · 649:653 · 676:689 · 797:808 · 891:897 · 951:964 · 1032:1035 · 1057:1065` | **byte-for-byte the same 13 intervals** | **0** | ✅ ⇒ **zero NET line delta anywhere in `1..1089`** |
| doc comment **opener** `^<#` | `:1089` | **`:1089`** | **0** | ✅ |

### 3b. ⭐ INSIDE the doc comment, ABOVE the edited clause — must also be **UNMOVED** (my addition to the table)

| landmark | `TASK-1339` | **mine** | shift |
|---|---|---|---|
| `f5697f3 (TASK-1324) deleted … (CONVENTIONS.md:5093)` | `:1105` | **`:1105`** | **0** ✅ |
| the exhibit's record `:5966-5969` / `:6172-6175` | `:1106` | **`:1106`** | **0** ✅ |
| `9d80505 (TASK-1327) deleted … :1680 (launch)` | `:1108` | **`:1108`** | **0** ✅ |
| `9d80505 (TASK-1327) deleted   :1677 (receipt)` | `:1109` | **`:1109`** | **0** ✅ |
| `':[0-9]{3,4}' ban would MISS A REAL ADDRESS` | `:1129` | **`:1129`** | **0** ✅ |
| clocks `'23:42'`, `'04:55:38'`, `'04:55:58'` | `:1131` | **`:1131`** | **0** ✅ |
| *"two dead addresses ON PURPOSE"* | `:1140` | **`:1140`** | **0** ✅ |

⇒ ⭐ **the `+1` is confined to strictly BELOW `:1140`** — a much tighter box than "below the doc comment".

### 3c. THE EDITED PARAGRAPH ITSELF — **7 → 8 lines, and the reflow added or dropped NO wording**

⭐ **This is the one place the reverse-substitution proof could actually be false, so I tested it directly.**
The paragraph's first three lines are **unchanged and unmoved**; the tail reflowed. I **concatenated** the
shipped tail and the superseded tail (the latter's bytes independently confirmed by `TASK-1339`'s M12 against
the then-shipped file, so it is not a bare declaration) and compared them as running prose:

- shipped tail = **8** lines; superseded tail = **7** lines ⇒ **+1** ✅ exactly as declared
- **the ONLY difference in the running text is `EVERY run` → `EVERY RUN THAT DERIVES ONE`** — every other
  word, hyphen and parenthesis is identical, in the same order ✅
- **+17 characters**, matching the row's declared length delta ✅
- ⇒ **no wording was added or dropped in the reflow. MEASURED, not accepted.**

### 3d. BELOW the doc comment — must be **EXACTLY +1**

| landmark | `TASK-1339`'s address | **mine, MEASURED** | shift | verdict |
|---|---|---|---|---|
| final `^#>` (doc-comment closer) | `:1175` | **`:1176`** | **+1** | ✅ ⭐ the **sixth** value (`1061`→`1065`→`1145`→`1168`→`1175`→**`1176`**), **re-derived by me** |
| parse-error guard `if` | `:1183` | **`:1184`** | **+1** | ✅ |
| its `FAIL-CLOSED` string | `:1185` | **`:1186`** | **+1** | ✅ content identical |
| no-block-comment guard `if` | `:1195` | **`:1196`** | **+1** | ✅ |
| its `FAIL-CLOSED` string | `:1197` | **`:1198`** | **+1** | ✅ content identical |
| the string literal `'<#'` (the 15th raw `<#`) | `:1190` | **`:1191`** | **+1** | ✅ |
| exempt walk (outward, blank-bounded) | `:1204-1214` | **`:1205-1215`** | **+1** | ✅ |
| `-notlike '*ANCHOR TO QUOTED TEXT*'` | `:1208` | **`:1209`** | **+1** | ✅ |
| anchor fail-closed return block | `:1215` | **`:1216`** | **+1** | ✅ |
| its `FAIL-CLOSED` string | `:1217` | **`:1218`** | **+1** | ✅ content identical |
| `$shapes = @(` | `:1220` | **`:1221`** | **+1** | ✅ |
| shape (a) `Rx` with `(?i)` | `:1222` | **`:1223`** | **+1** | ✅ regex byte-identical |
| shape (b) `Rx` | `:1224` | **`:1225`** | **+1** | ✅ regex byte-identical |
| the happy `return @{ Ok = ($why.Count -eq 0)…` | `:1238` | **`:1239`** | **+1** | ✅ |
| the `"lines 0..0"` code comment | `:1648` | **`:1649`** | **+1** | ✅ |
| `$hdr = Get-HeaderRotProneAddress…` block | `:1652-1658` | **`:1653-1659`** | **+1** | ✅ **content byte-identical to `TASK-1339`'s quoted block** |
| `if ($hdr.Last -gt 0)` | `:1654` | **`:1655`** | **+1** | ✅ ⭐ **NIT-3's conditional is UNTOUCHED, and `$hdr.Exempt` is still absent from the predicate** |
| `W-10` self-test case | `:1641` | **`:1642`** | **+1** | ✅ |
| `SC-126` self-test case | `:1659` | **`:1660`** | **+1** | ✅ |
| **file length** | 1995 | **1996** | **+1** | ✅ |
| `^<#` / `^#>` pairs | 14 / 14 | **14 / 14** | **0** | ✅ and lines containing either = **29** ⇒ exactly **1** non-line-start occurrence = the string literal ⇒ **raw `<#` = 15** |

🚨 **RESULT: every ABOVE landmark shifted by 0; every BELOW landmark shifted by exactly +1. NOT ONE LANDMARK
SHIFTED BY ANYTHING ELSE.** ⇒ **the reverse-substitution proof is corroborated to the limit of a shell-less
instrument**, and is further corroborated by §3c's wording-preservation check and by the **content** match on
every landmark I could compare (the three `FAIL-CLOSED` strings, both regexes, the whole emit block, the
exhibit line, `1500`, `QUIT_EDITOR`).

⚠️ **DECLARED RESIDUAL, unchanged and honestly restated (the one four gates have declared and none could
close):** an **equal-line-count byte substitution** that also preserves the `:[0-9]` census and every landmark
above is invisible to every instrument I hold. ⛔ **The `sha256` and `git diff -U0` close it and I hold
neither** — both are `ACCEPTED AS DECLARED` (§0 items 1–3). ⇒ routed to `TASK-1340` in the Notes. ⭐ **But the
box is smaller this time than at any prior gate:** the +1 is localised strictly below `:1140`, and §3c shows
the changed paragraph's prose is wording-identical bar the 17 characters under review.

---

## 4. CHECK FOUR — THE LEDGER. **55, and `SC-126` is PRESENT and LAST.** One command, not a re-run.

`Grep 'Add-SelfTestCase -Name|Add-ThrowCase -Name'` ⇒ **36 matching lines.** Reconciled:

- **−1** the internal call inside `Add-ThrowCase`'s own body (it forwards to `Add-SelfTestCase`; it is a
  helper implementation, not a registration) ⇒ **35 registration sites**
- **−3** loop-driven sites (the fixture loop, the bound loop, the `-LogPath` loop) ⇒ **32 literal one-shot
  registrations**, which I counted out by section: **4 + 8 + 2 + 3 + 4 + 2 + 2 + 7 = 32**
- **+ the three literal arrays**, counted by their own entries: `$cases` = **11** `File =` entries ·
  `$boundCases` = **5** · `$pathCases` = **7** ⇒ **23**

⇒ **32 + 23 = 55.** ✅ **ZERO ledger delta.** MEASURED statically at my own instant; **GREEN is
`ACCEPTED AS DECLARED`.**

🚨 **`SC-126: header grows no rot-prone line address` is PRESENT and LAST** — it is the **final**
`Add-SelfTestCase`/`Add-ThrowCase` in the file (the next occurrence of anything in that census is 120 lines
later and is a `$ProjectPath` assignment), registered after `W-10` and immediately before
`Write-Head 'SELF TEST RESULT'`. ✅

⛔ **Neither the handoff nor the code is wrong here** — the dispatch asked me to attribute a mismatch; **there
is no mismatch to attribute.**

---

## 5. CHECK FIVE — THE RETRACTION. **PRESENT, STRUCK-NOT-DELETED, AND LEGIBLE.** ✅

Three separate pieces of the loop-1 record survive in place rather than being cleaned up:

1. **§0b.1, at the top of the handoff:** *"§3 residue 1 below is WRONG and I retract it"*, naming the
   misclassification as **the cause, not a bookkeeping slip** — *"The misclassification is not a bookkeeping
   slip — it is the whole mechanism of the miss."* ✅
2. **§3 residue 1 itself:** the old text survives inside `~~…~~` strike-through — *"~~pre-existing, retained
   verbatim … not in my diff.~~"* — under a 🚨 heading declaring it retracted, followed by the correction
   *"A clause I re-worded is my clause. Provenance is not a defence."* ⭐ **Struck, not deleted. Fully
   legible.** ✅
3. **§3's *"exactly three checkable claims"* line** carries an inline correction block naming itself **THE
   DEFECT**, and **§3's "NEW verbatim" code block is kept unedited** under a `SUPERSEDED BY FIX LOOP 1`
   banner — *"Kept unedited as the record of what the gate actually blocked."* ✅

⇒ ⭐ **`SC-§136` cl. 6 honoured exactly.** A silent cleanup would have destroyed the only evidence the defect
had a **mechanism**; the row kept the mechanism visible at the cost of prose that reads worse. **That is the
correct trade and I credit it.**

---

## 6. FENCES AND SCOPE — what I did NOT do, and why (`SC-§100`)

| item | status | reason |
|---|---|---|
| the nine controls | ⛔ **NOT re-run** | out of scope on the row; `TASK-1339` re-derived them against today's regexes, and §3d shows **both regexes are byte-identical and merely +1** |
| the four `NIT-3` paths **as a battery** | ⛔ **NOT re-graded** | I read the four **returns** for check (1) — a different question. The conditional is ruled **CORRECT twice** and is **untouched** (§3d) |
| the 55 case names **one-by-one** | ⛔ **NOT re-taken** | check (4)'s count + last-case is the proportionate instrument, per the row |
| the header's prose | ⛔ **NOT re-read** | zero hunks reach it; §3a **is** the test, and it passed on eight independent landmarks |
| WARN-2's declared addition | ⛔ **NOT re-litigated** | `TASK-1339` ruling 4 UPHELD it clause by clause |
| the three out-of-scope NITs (NIT-1 · NIT-5 · NIT-6) | ⛔ **NOT re-checked individually**, but **incidentally corroborated untouched** | NIT-5's *"ALL THREE OF ITS ENTRY CONDITIONS"* is present in the shipped comment and unmoved relative to the +1 box; NIT-1's `+3` sits above `:1140` in the 0-shift zone |
| ⛔ no compile · no suite · no `-SelfTest` · no git · no commit · **no engine-lifecycle call** | ✅ | `TASK-1340` is the host. I hold no shell and refused to fabricate one |
| `TASK-1345` (`Source/**`, parallel) | ✅ **not my subject** | different file, lane, gate and host |

---

## Findings

- **[NIT-1]** `Tools/run_suite_bounded.ps1` — the trailing rider *"which is the one place it cannot go stale"*
  is **TRUE** (§2b conjunct E, re-derived, not inherited), but *"the ONE place"* quantifies over **places**,
  a domain no census bounds. It is refutable cheaply **within this file** (one `238` grep ⇒ 3 hits, none a
  boundary), and the claim's substance — *a value computed at run time cannot go stale, every recorded copy
  can* — is the paragraph's own thesis and is correct. ⛔ **Recorded, NOT actionable.** Opening a fourth loop
  over a **true** clause's quantifier style would spend a QA cycle for zero information, which the row's spec
  forbids. **No action.**

- **[NIT-2]** `Tools/run_suite_bounded.ps1` — **conjunct D's truth value is now owned by THREE code sites, in
  two directions, none of them adjacent to the sentence:** the emit predicate `if ($hdr.Last -gt 0)` (~490
  lines below), the four `return` shapes (~50 lines below), **and** — newly identified by this gate —
  `Add-SelfTestCase`'s `-Row` branch (~130 lines **above** the comment), which **drops `$Detail` entirely**.
  The `SC-126` registration passes no `-Row`, so the clause is true today (§1d); but a future edit adding
  `-Row` to that one call would falsify the doc comment **with the comment, the predicate and the returns all
  untouched**. ⛔ **Not a defect in this diff and not chargeable to this row** — the `-Row` branch predates
  every row in this chain. **Recorded for the manager (§7 item 1).** No action.

- **[NIT-3]** `Tools/run_suite_bounded.ps1` — a **pre-existing, out-of-scope** edge I name because I drove the
  paths and a reviewer who did not name it would look as if they had not: `[System.IO.File]::ReadAllLines`
  runs **after** `$first`/`$last` are assigned, so a TOCTOU I/O failure there would throw **with a boundary
  derived into locals and never returned**. ⛔ **It does NOT falsify conjunct D:** (a) the derivation is an
  unreturned local, not the function's boundary result, and (b) in that scenario `Add-SelfTestCase` never runs
  at all, so there **is** no ledger line and the universal is vacuous over it — the whole `-SelfTest` dies
  instead. It also requires a file that `ParseFile` read successfully microseconds earlier to become
  unreadable. **Pre-existing (untouched by both loops), out of this row's scope, no action.**

⇒ **ZERO BLOCKERS. ZERO WARNS.**

---

## ⚖️ RULINGS THE DISPATCH ASKED FOR

1. **Conjunct D — TRUE.** Measured on all four returns with *derived?* and *printed?* answered separately, and
   confirmed at the **print site**, which no predecessor reached. **`TASK-1339` BLOCKER-1 is DISCHARGED.**
2. **The `"pass or fail"` witness — CONFIRMED, and doubled.** The anchor-missing return is a fail that derives
   and prints; the happy return with `$why.Count -gt 0` is a **second**. The words are earned, not decoration,
   and the clause is **not** over-worded in the opposite direction.
3. **My own five-segment split — no sixth conjunct.** The fifth (the rider) exists, was named for the first
   time in fix loop 1, and I **re-decided** it with my own instruments: TRUE.
4. **The landmark table — HELD EVERYWHERE**, 0-above / +1-below, with four extra in-comment landmarks that
   localise the +1 to below `:1140`, plus a wording-preservation check on the reflow itself.
5. **The retraction — struck, not deleted, in three places.** Correct under `SC-§136` cl. 6, and credited.
6. 🚨 **RE-RANK (`SC-§132` cl. 4) — WAS `TASK-1339`'s FINDING RIGHTLY A BLOCKER? YES, and I say so in words.**
   Two precedents graded the same paragraph WARN and I would **not** overturn either of them, because what
   they saw was different from what `TASK-1339` saw. `TASK-1339`'s four reasons all hold, and I add a fifth
   from the outcome: **the FAIL cost exactly one prose clause and one re-read, and it bought a sentence that
   is now true under a behavioural instrument.** ⛔ A WARN would have shipped a **false universal** inside the
   one paragraph whose entire purpose is to stop false universals — in the very row boarded to end them —
   and the board would have opened a fourth row anyway. ⭐ **A gate that cannot fail the one thing it gates is
   not a gate.** The grade was right.
7. ⭐ **And a credit that belongs on the record:** the row's response to the FAIL was **exemplary** — it
   adopted the prescribed wording **after enumerating and measuring it TRUE**, having **refused** a prescribed
   wording one loop earlier **after enumerating and measuring it FALSE**. ⛔ **Those two opposite actions come
   from one consistent rule — *the instrument decides, not the authorship* — and the row said so explicitly.**
   That is the behaviour this whole chain was trying to produce.

---

## Notes for build-master (`TASK-1340`)

- ✅ **UNBLOCKED.** This report's `Verdict:` token reads **PASS**. ⛔ Grep the **token**, never a line address
  (`SC-§126` cl. 10).
- ⭐ **The final `^#>` is `:1176` AT MY INSTANT. It is the SIXTH value** (`1061`→`1065`→`1145`→`1168`→`1175`→
  **`1176`**) **and it is HEARSAY TO YOU** (`SC-§126` cl. 11). **Re-derive it as the last `^#>` hit.**
  ⛔ **NEVER anchor your parse control on the header's own `#>` at `:238`** — that trap has now been
  **executed twice** and returns a **silent 0**; the block re-pairs against the next `<#`.
- ✅ Expect `-SelfTest` **55 / 55**, exit 0, `SC-126: header grows no rot-prone line address` **last and
  green**, detail `header derived as lines 1..238; 6 exhibit line(s) excluded by substring, never by address`.
  ⛔ If any name is missing or any detail differs, that is an equal-line-count rewrite showing itself — **stop
  and report.**
- 🚨 **CLOSE MY §3 RESIDUAL — it is the only fence this chain has never closed, and you are the first host who
  can.** One read-only command:
  `git diff -U0 -- GitClaudeUnrealTest/Tools/run_suite_bounded.ps1 | Select-String '^@@'`
  ⇒ assert the **lowest** hunk is at or below `1145`, i.e. **ZERO hunks at or above `:238`**. ⛔ **Git root is
  ONE LEVEL UP (`SC-§102`) — a mis-anchored pathspec answers with SILENCE, not an error.**
  ⭐ Optionally also re-assert the header `sha256` `072F55CE…8593A2E` / 14,741 bytes **through the 238th LF**,
  deriving the boundary rather than trusting the 238.
- ⛔ **COMMIT BY PATHSPEC, NEVER `-a`.** At the row's instant the tree also carried `TASKBOARD.md`,
  `handoffs/TASK-1343-buildmaster.md` and `Tools/run_suite_bounded.ps1`, and **`TASK-1345` is live in
  `Source/**` right now**. The gated delta is **`Tools/run_suite_bounded.ps1` + `handoffs/TASK-1338-programmer.md`
  + `qa/TASK-1339-report.md` + `qa/TASK-1344-report.md`** and the two board flips. ⛔ **Stage nothing else.**
- ⛔ **Do not reconcile the `-SelfTest` ledger (55) against the UE suite (561)** — different instruments. No
  suite-count delta is claimed and none should be asserted.
- ⛔ **No 5a (PowerShell in a tool) and no 5b (no runtime acceptance criterion).**
- The `LF will be replaced by CRLF` warning on this file is **pre-existing**, not from this diff.

## ⚖️ For the manager — one item

⭐ **`TASK-1339` recorded instance ONE of the species *"a prose claim whose truth value is owned by code
hundreds of lines away"* and recommended recording it without designing against it yet. I have now measured
that the same clause is owned by THREE such sites, in TWO directions** (NIT-2): the emit predicate below, the
four returns below, and `Add-SelfTestCase`'s `-Row` branch **above**. ⛔ **No census of the comment, and no
reading of the composition block, can see the third one** — both prior gates verified conjunct D at the `-f`
and stopped there; it happens to be true, but they could not have known that. ⇒ **This is still instance one
of the species, not a new defect** — but the *fan-out* is the datum: **a behavioural claim in a comment should
name the site that makes it true**, and the row's own code comment already does this well for the predicate.
**Recommendation: record; a second instance earns a mechanism.** ⛔ Not scored against any row.

## Flips performed by this gate

⛔ **Collisions measured at MY OWN instant (`SC-§127`), never assumed:** the bare `^- status:` shape collides
**1344** times on `TASKBOARD.md`. With the `TASK-####` discriminator each anchor is **1** occurrence.
⛔ `Edit` only, **NEVER `replace_all`**, each flip read back as **STATE** (`SC-§104`).

- `TASK-1338` → **`qa-passed`**
- `TASK-1344` → **`qa-passed`** (this gate COMPLETED; the word mirrors the verdict over its subject)
