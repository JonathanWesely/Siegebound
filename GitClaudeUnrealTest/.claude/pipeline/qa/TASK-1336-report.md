# QA Report — TASK-1336 (gate over TASK-1335)

Verdict: **PASS** — 0 BLOCKER · 3 WARN · 6 NIT · 1 DECLARED RESIDUAL (routed to `TASK-1337`)

- **subject:** `TASK-1335` — HEADER-GUARD-FAIL-CLOSED
- **marker:** `TASK-1336-HEADER-GUARD-FAIL-CLOSED-GATE`
- **reviewer:** qa-reviewer, 2026-09-20
- **host:** `TASK-1337` · **no 5b** (no runtime acceptance criterion)
- **reviewed:** `Tools/run_suite_bounded.ps1` — header `1..238`, helper doc comment `1089..1168`, helper body
  `1169..1232`, the self-test case and its neighbours `1620..1659`, the param block `241..298`, `Invoke-Main`'s
  entry `1736..1740` · `handoffs/TASK-1335-programmer.md` **whole** (`SC-§38a`) · `qa/TASK-1332-report.md` whole ·
  `CONVENTIONS.md` `SC-§129` whole and `SC-§126` cl. 11 whole · `TASKBOARD.md` `TASK-1335` / `TASK-1336` / `TASK-1337`

---

## 0. WHAT I EXECUTED vs WHAT I DERIVED vs WHAT I ACCEPTED AS DECLARED — first, because the chain's subject is provenance (`SC-§71b` · `SC-§91`)

**I EXECUTED NOTHING. I have no `Bash` in this session** — no PowerShell, no `-SelfTest`, no `git`, no hashing.
⛔ **I did NOT route a shell or git through the read-only `unreal_inspector`.** `TASK-1317`, `TASK-1328` and
`TASK-1332` hit this identical wall and all three declined; using a read-only editor bridge as a shell
circumvents a designed fence rather than working around a missing tool. I decline the same way.

⇒ **Every verdict below marked DERIVED was re-derived by me, by hand, from today's shipped bytes** — the
regexes as they now read, the control flow as it now runs — using `Grep`/`Read` only. That is a *different
instrument* from the row's harness (`SC-§126` cl. 9), not the same one re-read.

### MEASURED BY ME, at my own instant (`SC-§126` cl. 11 — nothing here is quoted from a ledger)

| # | quantity | my instrument | my value | agrees with the row? |
|---|---|---|---|---|
| M1 | header boundary | `Grep '^<#'` first hit = `1`; `Grep '^#>'` first hit = `238` | **lines 1..238** | ✅ |
| M2 | block-comment **pairs** | `^<#` = **14** · `^#>` = **14**, strictly interleaved (`1:238 · 313:322 · 372:382 · 405:418 · 520:525 · 586:594 · 649:653 · 676:689 · 797:808 · 891:897 · 951:964 · 1032:1035 · 1057:1065 · 1089:1168`) | **14 pairs** | ✅ — and it **refutes the shipped `13`** exactly as the row's own count did |
| M2b | corroboration of M2 | raw `<#` = **15**, raw `#>` = **14**; the 15th `<#` is the **string literal** at `:1183` | 14 delimiter pairs | ✅ identical to the row's reasoning, reached independently |
| M3 | exhibit geometry | `Grep 'ANCHOR TO QUOTED TEXT'` → `:144`; `Read 138..155` | marker **144**, addresses **146**, blank `143`/`150` ⇒ exempt **144..149 = 6 lines** | ✅ |
| M4 | header `:[0-9]` census | `Grep ':[0-9]'` restricted to `1..238` | **6 lines, 4 species** — `:19`,`:21` engine · `:104`,`:121`,`:122` clock · `:146` the exhibit | ✅ (and identical to `TASK-1332`'s census — three readers, three instants, one reading) |
| M5 | all **nine** controls' verdicts | re-derived by hand from the **new** shipped regexes against today's bytes | **all nine agree with the handoff** (§3) | ✅ |
| M6 | the `-SelfTest` ledger | `Grep 'Add-SelfTestCase -Name\|Add-ThrowCase'` + the three literal case arrays | **55 cases, reconciled BY NAME *and BY ORDER*** (§7) | ✅ |
| M7 | `(?i)` adds no new hit on `HEAD` | case-**insensitive** `Grep 'conventions\|taskboard'` | header hits are `:141`, `:146`, `:151`, `:181`; **only `:146` is followed by colon-digit, and it is exempt** | ✅ |
| M8 | `-SelfTest` process purity | `Read 1736..1740` | `Invoke-Main`'s **first statement** is `if ($SelfTest) { return (Invoke-SelfTest) }` — it returns before any launch code | ✅ measured, not inherited |
| M9 | the declines | `Grep 'no disk access\|no process'` → `:524`, `:963`, header `:236`; `Read 1197..1211`; `Read 1636..1644` | block **not** narrowed · NIT-6 text untouched · **no** `try/catch` on the new case | ✅ |
| M10 | param defaults / terminator | `Grep` | `1500` / `420` / `180` at `:253-255`, `$script:Terminator = 'QUIT_EDITOR'` at `:298` — unchanged | ✅ |

### ACCEPTED AS DECLARED, named one by one

1. **That the two limbs, the parse control and the nine controls were EXECUTED** and produced the quoted
   `exit 5` / `exit 0` / `54 / 55` / `55 / 55` output. I re-derived **every one of their verdicts** and, for
   LIMB A, **the exact finding strings and their multiplicity** (§2) — but I did not run the harness.
2. **`sha256` of the first 238 lines** = `072F55CE…93A2E` on both `8cbd5d1` and the working copy. I cannot
   hash. I corroborated byte-identity **four other ways** and declare the residual at §6.
3. **`sha256` of the whole tracked file identical before/after the control battery**, and the scratchpad lab deleted.
4. **The commit-table measurement `141 → 144` (`+3`) under `9d80505`.** Needs `git`. See NIT-1.
5. **`git diff --numstat` = `45 13` on one file.** Provided as the row's own reading; I also use it
   arithmetically in WARN-3.

---

## 1. 🚨 CHECK (1) — THE FAIL-CLOSED CONTROL, BOTH DIRECTIONS. **PASS, and I re-derived both limbs.**

The mutation is held constant across limbs — the anchor line is **reworded in place**, not deleted — so line
count, paragraph shape, exhibit addresses and derived boundary are all invariant and **the only variable is
whether the substring exists.** ⭐ That is what makes the pair a *control* rather than two observations, and
it is the single best methodological decision in this row.

### LIMB A — today's `8cbd5d1` code, anchor reworded ⇒ reds NAMING the addresses. **DERIVED, including the multiplicity.**

With `Exempt = 0` and no early return, the corpus is the whole header. Against `8cbd5d1`'s shape (a)
`(?:CONVENTIONS|TASKBOARD)\.md(?:\s+at)?\s*:\s*\d+` and shape (b) `(?<![0-9A-Za-z._/\\-]):\d+`, line `:146`
reads `CONVENTIONS.md at :5966-5969 and then at :6172-6175`:

| shape | hits on `:146` | why |
|---|---|---|
| (a) | **1** — `CONVENTIONS.md at :5966` | `\.md` + `\s+at` + `\s*` + `:` + `5966`; there is only one filename token on the line |
| (b) | **2** — `:5966` and `:6172` | both colons are **space**-preceded, so the lookbehind admits both |

⇒ **exactly three findings, all on line 146** — and no other header line fires (`:19`/`:21` are `p`-preceded,
`:104`/`:121`/`:122` are digit-preceded, `:141`/`:151`/`:181` carry no colon-digit). **That is precisely the
three-line block the handoff quotes, in the right order, with the right match values.**

⭐ **This is the strongest single piece of evidence in the submission, and it is evidence I produced, not
accepted.** The asymmetric multiplicity — shape (a) once, shape (b) twice, from one line — is a non-obvious
consequence of two independent patterns overlapping on one spelling. A fabricated transcript reports two
findings or four; it does not volunteer one-plus-two.

⇒ **WARN-2 is REPRODUCED LIVE, not asserted.** The old code emits the one finding its own doc comment called
itself *"structurally incapable"* of emitting, and prescribes the deletion of two addresses that exist
**because they rotted** — i.e. it invites the seventh hand-edit of the header.

### LIMB B — fixed code, same mutation ⇒ reds FAIL-CLOSED, naming nothing. **DERIVED from the shipped code.**

`:1208-1211` returns on `$exempt.Count -eq 0` **before `$shapes` is constructed** (`:1213`), so `[regex]::Matches`
is never reached and no `$why` entry can exist that quotes a line. The returned hashtable carries
`First=$first; Last=$last; Exempt=0`, and `:1642` formats those into the detail ⇒ the ledger line reads
`header derived as lines 1..238; 0 exhibit line(s) excluded …; FAIL-CLOSED: exhibit anchor not found …`,
**verbatim as reported**. The published `0 exhibit line(s) excluded` meter — the tripwire `TASK-1332` ruled
load-bearing — still prints. ✅

⭐ **And the absence claim is stated the right way:** *"lines in the ENTIRE run output naming an exhibit
address (`:5966` | `:6172` | `5966-5969` | `6172-6175`): **0**"*. An absence that **names the strings it
searched for** is refutable in seconds; *"it names nothing"* is not.

### ⚖️ IS "BOTH LIMBS RED" THE RIGHT BAR? **YES — and it is right BECAUSE both are red, not in spite of it.**

The dispatch asks whether the row can hide a regression behind *"still red."* It cannot, here, and the reason
is structural rather than charitable:

1. **In LIMB B, red is the REQUIRED outcome, and green would have been the regression.** The mutation destroys
   the guard's exclusion anchor. The correct response to *"I cannot find my own subject"* is to refuse to scan
   **and say so** — which is a red (`SC-§39`, quoted in the file's own comment). A green LIMB B would mean the
   guard went **silent** on a mutated header: the exact failure the law names as worse than no guard.
2. ⇒ **The discriminator here is not red-vs-green; it is WHAT THE RED SAYS** — and that discriminator was
   measured with a **zero over the whole run output**, which is a stronger assertion than *"the detail differs."*
3. **The one-direction version would have been an assertion.** *"It fails closed"* without LIMB A does not
   establish that anything changed; LIMB A is what proves the old code did something **different and worse**.
4. ⚠️ **WHERE THE BAR IS INCOMPLETE, AND I SAY SO PLAINLY:** the pair controls **one** input class — *the
   anchor is lost*. It does **not** control *the anchor is present but the exhibit paragraph is reflowed*, and
   that input **still fails open and still names the addresses** (WARN-2 below). The bar is the right bar for
   the **boarded defect**; it does **not** support the doc comment's new universal *"UNDER ANY INPUT."* That
   gap — between what was controlled and what is claimed — is the whole of my WARN-2, and it is a **prose**
   over-claim sitting on a **code improvement**.

## 2. CHECK (2) — THE PARSE-ERROR CONTROL (NIT-4). **PASS. And the old behaviour is WORSE than the WARN it was boarded under — confirmed by construction.**

`:1176-1179`: `if ($errors -and $errors.Count -gt 0) { return @{ … FAIL-CLOSED: the parser reported N error(s) … } }`.

**DERIVED, old code:** `ParseFile` is best-effort — with one syntax error appended at EOF it still returns a
token stream, the leading block comment still resolves, `ReadAllLines` still succeeds, the exhibit is still
found, and no banned address survives ⇒ `$why` is empty ⇒ `Ok = $true`. ⇒ **the case reported `yes` — a GREEN
ledger line — about a file that does not parse.** The handoff's `RESULT Ok=True … it reported CLEAN` is exactly
what the old code must produce.

🚨 **Recorded explicitly because the dispatch asked, and because it revises my predecessor's grading:** WARN-2
was a **loud wrong answer** (a red, naming the wrong thing); **NIT-4 was a SILENT wrong answer** (a green about
an unparseable file). A fail-**silent** outranks a fail-**loud**, so `TASK-1332` under-graded NIT-4 by a rank.
The row fixed both anyway and shipped the harder one with its own control — no consequence, but the ordering
should be on the record before someone triages by label next time.

**The guard's own shape is right, and the trap it avoided is real:** an empty `ParseError[]` is falsy and `$null`
is falsy, so the happy path is untouched (corroborated by the green tracked run and by the case passing today);
⛔ `@($errors).Count` would have returned **1** for `$null` and fail-closed the guard **on every run**. The row
names that trap and declines it. ✅

**Safety, checked by me:** all three `return` shapes (`:1177`, `:1189`, `:1209`) carry **`First`, `Last`, `Exempt`
and `Why`**, so `:1642-1644`'s `$hdr.First/.Last/.Exempt/.Why` dereference cannot miss a key on any path. No
null-key crash exists. ✅

## 3. 🚨 CHECK (3) — THE SEVEN WERE RE-RUN, NOT INHERITED. **PASS — no inheritance is visible, and all nine verdicts re-derive.**

⛔ The blocker is *inheritance* — citing `TASK-1331`'s results across a change to the instrument. **Three
independent reasons say this handoff did not do that:**

1. **Two of its nine controls measure the OLD code's behaviour on inputs `TASK-1331` never tested**
   (lowercase slipping; the English sentence false-redding). Those numbers **cannot** have been inherited —
   they did not exist to inherit.
2. **The method is different and is declared**: injection at line 24 inside `.DESCRIPTION` (deliberately
   outside the exhibit block), one shape at a time, verdict read **positionally** at column 63 rather than by
   regex — the fix for the `-match`-is-case-insensitive instrument failure `TASK-1331` itself recorded.
   ⭐ I checked the injection site is sound: a line added at 24 grows the header to `1..239` and shifts the
   exhibit to `145/147` with `Exempt` still 6, so the injected line is inside the scanned corpus and the
   exclusion is undisturbed. The harness tests what it claims to test.
3. **Every verdict re-derives against today's regexes.** Shape (a) is now
   `(?i)(?:CONVENTIONS|TASKBOARD)\.md(?:\s+at\s*)?:\d+`; shape (b) is untouched, as specced.

| # | control | expected | **my derivation against today's bytes** | agrees |
|---|---|---|---|---|
| (i) | `CONVENTIONS.md:9999` | RED | shape (a): optional group empty, `:` touches `9999` ⇒ **fires once** (shape (b) is blocked by the `d`) | ✅ |
| (ii) | `CONVENTIONS.md at :9999` | RED | shape (a): `\s+at\s*` = `" at "` ⇒ fires; shape (b): colon is space-preceded ⇒ fires ⇒ **TWICE, independently** | ✅ |
| (iii) | bare `(:9999)` | RED | shape (b): `(` is outside the lookbehind class ⇒ fires | ✅ |
| (iv) | `CONVENTIONS.md:29` — **two digits** | RED | `\d+` carries no width constraint ⇒ fires. `{3,4}` refuted again | ✅ |
| (v) | `EngineThing.cpp:1234` | GREEN | (a) filename mismatch; (b) `p` precedes the colon | ✅ |
| (vi) | clock `07:15:09` | GREEN | (b) both colons digit-preceded; (a) no filename | ✅ |
| (vii) | **negative control — unmodified `HEAD` prose, AFTER the pattern moved** | GREEN, exit 0 | all six census lines walked in §4 below ⇒ **zero non-exempt matches** | ✅ |
| (viii) | **NEW** lowercase `conventions.md:9999` | RED on fixed, **GREEN (slips) on old** | `(?i)` present on (a) only ⇒ fires; without it `[regex]::Matches` is case-sensitive in .NET ⇒ slips | ✅ both directions |
| (ix) | **NEW** English *"see `CONVENTIONS.md`: 3 rows apply"* | GREEN on fixed, **RED (false) on old** | new (a) needs the colon to touch the digit ⇒ no match; (b) blocked by `d`. Old `\s*:\s*\d+` matched `CONVENTIONS.md: 3` | ✅ both directions |

⇒ **all nine agree.** Control (ii) firing **twice** and control (iv) firing on **two digits** are both
non-obvious consequences of the shipped patterns, and both are reported rather than smoothed over.

⚠️ **The honest limit:** I can prove the **verdicts** are right and that **no result was copied from
`TASK-1331`**; I cannot prove a process ran. That is `SC-§71b`, and `TASK-1337` runs the lane for real.

## 4. CHECK (4) — THE NEGATIVE CONTROL STILL PASSES ON UNMODIFIED `HEAD`. **PASS, DERIVED line by line, with the case-insensitive instrument.**

| header line | text | new shape (a)? | shape (b)? | why |
|---|---|---|---|---|
| `:19` | `…ParseExecCommands.cpp:29` | no | **no** | `p` precedes the colon |
| `:21` | `…EditorServer.cpp:5993` | no | **no** | same |
| `:104` | `…09-18 23:42..23:44…` | no | **no** | digit-preceded |
| `:121` | `…opens at 04:55:38,` | no | **no** | digit-preceded |
| `:122` | `…is 04:55:58 (20 s)…` | no | **no** | digit-preceded |
| `:141` | `…did not touch, CONVENTIONS.md). Nothing…` | **no** | no | `.md` is followed by `)`, not by a colon |
| `:146` | `CONVENTIONS.md at :5966-5969 … :6172-6175` | *would fire* | *would fire ×2* | **EXEMPT — inside the exhibit block** |
| `:151`, `:181` | `grep -n "…" .claude/pipeline/CONVENTIONS.md` | no | no | line ends at `.md` |

⇒ **zero non-exempt matches ⇒ exit 0, `6 exhibit line(s) excluded`.** ✅ **The ban list is right and the check
is shippable** — a check that cannot pass is as broken as one that cannot fail.

🚨 **And I closed the specific new risk the pattern change created (M7):** the only `CONVENTIONS`/`TASKBOARD`
tokens in `1..238` are `:141`, `:146`, `:151`, `:181`, **all uppercase**, so `(?i)` adds **no** new hit. The
row reports the same four lines; I took the census with a case-insensitive grep, which is the instrument that
could have refuted it.

## 5. 🚨 CHECK (5) — THE HEADER BLOCK. **PASS: I find ZERO evidence of a changed line at or above `:238`, from four independent instruments. One residual, declared and routed.**

I **cannot** hash, so the `072F55CE…93A2E` equality is accepted as declared. What I **measured**:

1. **The block-comment delimiter ledger is identical to `TASK-1332`'s post-`TASK-1331` reading for all 13
   pre-existing pairs** (`1:238 · 313:322 · 372:382 · 405:418 · 520:525 · … · 951:964 · 1032:1035 · 1057:1065`)
   **and the 14th still opens at `:1089`.** ⇒ every interval from line 1 to line 1089 has **zero net line
   delta** ⇒ no insertion and no deletion anywhere at or above the header.
2. **The exhibit is byte-consistent**: marker at `:144`, addresses at `:146`, blanks at `:143`/`:150`, block
   `144..149` = **6** — identical to `TASK-1332`'s hand-walked arithmetic, and `Read 138..155` reproduces the
   prose `TASK-1332` quoted, word for word.
3. **The header census is unchanged** — the same six lines, the same four species (M4).
4. ⭐ **The declared lowest hunk `@@ -1108` is internally consistent with the new file, which is a check the
   row could not have faked without arithmetic:** with git's default 3 lines of context, `@@ -1108` means the
   first **changed** old line is **1111** — and in the working copy, line **1111** is exactly where the new
   paragraph `SHAPE (a) IS CASE-INSENSITIVE, SHAPE (b) IS NOT…` begins, with `1089..1110` being unchanged
   pre-existing text. The hunk address and the file agree.

⇒ **No seventh hand-edit of the header is visible to any instrument I hold.** The exhibit's addresses are
still `:146`, untouched, still evidence.

⚠️ **DECLARED RESIDUAL — an equal-line-count substitution inside `1..238` is invisible to every check above,
and it is invisible to `TASK-1337`'s stated legs too.** This is the identical residual `TASK-1328` declared and
`TASK-1329` discharged, and `TASK-1332` declared and `TASK-1333` discharged — **but note the difference:**
`TASK-1333`'s spec ordered a `git diff` over the whole file, and **`TASK-1337`'s spec does not.** `git show
--stat HEAD` lists files, not hunks. ⇒ **routed explicitly in the Notes below, as one command.**

## 6. ⭐ CHECK (6) — PAIR COUNT AND BOUNDARY RE-MEASURED. **They match the row's. And the corrected comment is TRUE at my instant in every clause but two.**

**Boundary `1..238` ✅ · pairs `14` ✅ · exhibit `144/146`, exempt `6` ✅ — all four re-measured (M1–M3), all
four agree with the row's own values.** This is `SC-§126` cl. 7's fourth instance honoured: I re-measured
precisely because a gate that re-measured once got a different number.

**True at my instant, verified against the code:**

- *"SHAPE (a) IS CASE-INSENSITIVE, SHAPE (b) IS NOT, AND THE ASYMMETRY IS DELIBERATE"* ✅ — `(?i)` is on
  `:1215` and absent from `:1217`, and shape (b) genuinely matches no letter outside its lookbehind.
- *"IT FAILS CLOSED AT ALL THREE OF ITS ENTRY CONDITIONS — the parse, the header boundary and the exhibit
  anchor"* ✅ — `:1176`, `:1188`, `:1208`, all three present, all three returning before any scan.
- *"the derived value is printed in the ledger line on EVERY run, pass or fail"* ✅ **— I verified this rather
  than accepting it:** `:1642-1643` builds `$hdrDetail` **unconditionally** and `:1644` passes it as `-Detail`
  irrespective of `$hdr.Ok`. The claim that replaces the deleted numbers is itself true.
- *"A guard that fails closed at one anchor while failing open at the next has only moved the hole"* ✅ — true,
  well put, **and ironically the sentence my WARN-2 invokes.**

**Not true at my instant:** the two universals at WARN-1 and WARN-2 below.

## 7. ✅ CHECK (7) — `SC-§129` cl. 3(c), ITS FIRST LIVE APPLICATION. **PASS on the handoff, and I went further than the clause requires.**

The ledger is present in handoff §6, **by name**, `RUNNER_EXIT = 0`, every case `yes`. ⇒ **not a blocker, and
nothing is owed by the code.** The duty **was** present in the row's spec (clause (6)), so there is **no board
defect to record against `TASK-1334`.**

⭐ **`TASK-1332` recorded the ledger as "not statically countable." It is countable, and I counted it.** Every
`Add-SelfTestCase`/`Add-ThrowCase` site is either a literal call or a loop over a **statically visible literal
array**:

| ledger § | shipped site(s) | my count | handoff's names | order |
|---|---|---|---|---|
| §1 fixture corpus | loop `:1335` over `$cases` `:1251-1284` | **11** | 11, same files | ✅ identical |
| §2 bound arithmetic | loop `:1374` over `$boundCases` `:1347-1351` | **5** | 5, same `What` strings | ✅ identical |
| §3 lane construction | `:1387,1392,1397,1402` + `Add-ThrowCase :1404,1407,1409,1411,1413,1415,1417,1419` | **12** | 12 | ✅ identical |
| §4 expected echoes | `:1429,1437,1442,1445,1448` | **5** | 5 | ✅ identical |
| §5 command line | `:1464,1473,1482,1493` | **4** | 4 | ✅ identical |
| §5b Aura exclusion | `:1517,1533` | **2** | 2 | ✅ identical |
| §6 bound merge | `:1548,1556` | **2** | 2 | ✅ identical |
| §7 `-LogPath` safety | loop `:1577` over `$pathCases` `:1564-1570` | **7** | 7 | ✅ identical |
| §8 input tolerance | `:1590,1594,1603,1611,1620,1634,1644` | **7** | 7 | ✅ identical |

⇒ **11+5+12+5+4+2+2+7+7 = 55, matching the handoff name-for-name AND order-for-order against the shipped
bytes.** ⇒ **no case was added, renamed, reordered or dropped**, and the `SC-126` case is **last**, appended
after `W-10` (`:1634`) and before `Write-Head 'SELF TEST RESULT'` (`:1647`) — exactly where `TASK-1332`
measured it. **ZERO ledger delta (55 → 55) confirmed statically, not accepted.**

⚖️ **The row's choice not to ship the two new controls as cases is correct and correctly declared**: its
`names` block authorises *"its self-test case"* (singular), the fences demand a zero delta, and a shipped case
would hand the host a count to reconcile against a row whose acceptance says the delta is zero. Documented in
the doc comment instead, which is where the next reader looks. ✅

## 8. ⛔ CHECK (8) — THE THREE DECLINES: HONOURED. **And I do NOT re-propose the narrowing — it is RULED WRONG.**

- **(a) The block→marker-line narrowing was NOT taken** — `:1197-1207` still walks outward to the paragraph's
  blank-line boundaries. ⭐ **My own WARN-2 independently RE-CONFIRMS the decline from the opposite side:** the
  residual fail-open I found depends on the exempt block being **split**; narrowing it to the marker line would
  make that fail-open **unconditional** — `:146` would sit in the corpus on `HEAD` and the negative control
  would red today. **Any `+2` variant is an address in disguise.** Recorded so no future row "tightens" this.
- **(b) NIT-6: untouched and correctly closed.** `no disk access` is still at `:524` (`Resolve-SafeLogPath`) and
  `no process` at `:963` (`Merge-BoundIntoVerdict`), both far below `:238` and both describing their own
  functions; the header's own `:236` still reads *"Launches no engine, no process"* — accurate. Nothing false.
- **(c) NIT-5: declined with its reason**, and there is still no `try/catch` around `:1641-1644` (verified).
  ⭐ The row adds an honest note I endorse: its two new `return` guards make an abort **less** likely, not more,
  because both fire on conditions that previously ran on into `ReadAllLines`. **The trigger is unchanged: NIT-5
  gets a row the first time it aborts a real ledger.**

## 9. CHECK (9) — FENCES

| fence | verdict | basis |
|---|---|---|
| no parameter default / bound changed | ✅ | M10: `1500`/`420`/`180` at `:253-255`; and `:241-298` is above `:1089`, which §5's ledger proves has zero net delta |
| no `-ExecCmds` composition / comma-vs-semicolon separator / `QUIT_EDITOR` (`SC-§116`) | ✅ | `$script:Terminator = 'QUIT_EDITOR'` at `:298`; the lane-construction and command-line cases are present **by name and in order** (§7) |
| no existing self-test case edited, renamed or reordered | ✅ | §7's by-name **and by-order** reconciliation — stronger than a count, which would hide exactly this |
| the exhibit untouched | ✅ | M3 + `Read 138..155` reproduces `TASK-1332`'s quoted text verbatim |
| no new UE automation test / zero suite-count delta | ✅ | no `.cpp` in the row's delta; no automation macro in the file |
| diff = ONE file + the handoff | ✅ | my session-start `git status` snapshot: `M Tools/run_suite_bounded.ps1` (the row's) · `?? handoffs/TASK-1335-programmer.md` (the row's) |
| `TASKBOARD.md` / `CONVENTIONS.md` / `Saved/**` / `.uasset` / `.cpp` absent from the ROW's delta | ✅ | see the ⚠️ immediately below — the dirt is other rows' |
| **ZERO files from `TASK-1314`'s / `TASK-1319`'s lane** | ✅ | `SiegePlayerController.cpp` is **`TASK-1319`'s live in-flight work**, declared by the row and confirmed by my dispatch. **Not the row's, not damage, not mine to touch.** |
| no compile, no suite, no git write, no commit by the row | ✅ | `TASK-1337` is the host |

⚠️ **The tree is dirty and none of it is `TASK-1335`'s**: `TASKBOARD.md` (board edits), `SiegePlayerController.cpp`
(`TASK-1319`, running in parallel right now), `handoffs/TASK-1319-programmer.md`, `handoffs/TASK-1333-buildmaster.md`.
The row reported the tree it had rather than the clean tree the acceptance wanted (`SC-§95`), which is the
correct behaviour, and it told the host to commit **by pathspec**. ✅

---

## ⚖️ RULING ON THE THREE DECLARED DEPARTURES (`SC-§101` — a prescribed remedy is a CLAIM)

### Departure 1 — **DELETING** `:146`, `lines 1..238` and the pair count instead of refreshing them. **UPHELD. ⛔ DO NOT TAKE THE OFFERED REVERT.**

Six reasons, and the fourth is the one I would defend hardest:

1. **It is not a departure from the board at all — it is the board's stated first choice.** Spec (4) reads
   *"BETTER, AND PREFERRED IF YOU CAN PHRASE IT: say the same thing WITHOUT a live count — a count in a comment
   is an ADDRESS WEARING DIFFERENT CLOTHES."* WARN-1's two-word fix was my predecessor's **minimum**, offered
   before the manager boarded the preference. The row took the preference. That is obedience, not deviation.
2. ⭐ **`TASK-1327` settled this exact question on this exact file, in this exact direction, and shipped**
   (`9d80505`: the clause-(a) aside's `:1680`/`:1677` were **DELETED, not refreshed**, gated PASS by
   `TASK-1328`). Ruling the other way here would split the file's own precedent down the middle.
3. **The three deleted values sat BELOW the scanned header, where the guard cannot catch their rot.** They were
   unenforced numbers inside the comment of the guard whose subject is that unenforced numbers rot — the
   generative shape, not an instance of it. Refreshing them re-arms the same trap for the next row.
4. ⭐ **The information is not lost, and I verified the replacement rather than accepting it:** `:1642-1644`
   builds and emits the detail line **unconditionally**, on pass and on fail, so the boundary and the exempt
   count print on **every** run. **A value re-derived every run is strictly more current than a comment, and it
   is the only form of this information that cannot go stale.** That is the whole thesis of the chain, applied
   to the chain's own documentation.
5. **Reverting would be a net regression**, not a neutral restoration: it re-introduces a live address (`:146`)
   and a live boundary (`1..238`) into the file that has now paid for **four** rotted citations.
6. ⚠️ **The one thing the departure did NOT buy, and the row should not be credited with it:** the comment is
   **not** number-free (WARN-1). The deletion is right; the sentence announcing it over-reaches.

### Departure 2 — tightening *"the next edit ABOVE it"* to *"the next edit that ADDS OR REMOVES A LINE above it."* **UPHELD.**

Strictly more accurate than my predecessor's suggested wording, and it corrects an over-claim in the same
direction the WARN was correcting — which is the right instinct. ⇒ NIT-2 below records the remaining sliver
(an edit that both adds **and** removes equal lines above moves nothing); non-blocking, **no re-edit required**.

### Departure 3 — retaining `+3 under TASK-1327` with the provenance parenthetical. **UPHELD.**

A **delta about a closed commit** cannot rot the way a position can, the board's own suggested wording
contained it, and the parenthetical *"(measured across that commit, not quoted from a ledger)"* is precisely
what `SC-§126` cl. 11 asks a citation to carry. ⚠️ Two caveats, both recorded rather than held against it:
it is the one load-bearing number in the comment I could **not** re-measure (NIT-1), and it is the `OFFSET`
that falsifies the sentence six lines below it (WARN-1).

---

## Findings

- **[WARN-1]** `Tools/run_suite_bounded.ps1:1153-1154` — **the sentence that announces the fix is itself a false
  universal, and it is the same species as the WARN it discharges.** *"NO BOUNDARY, COUNT OR OFFSET IS WRITTEN
  DOWN IN THIS COMMENT"* — **measured against the comment it is in:** `:1147` writes the **offset `+3`** six
  lines above it (this diff's own text); `:1092` writes *"**Six** consecutive rows"*; `:1140` writes *"**two**
  dead addresses"*; `:1128` writes *"NOTE THE **TWO** DIGITS"*; `:1162` writes *"ALL **THREE** OF ITS ENTRY
  CONDITIONS."* The **intent** — no live value describing *this file's present geometry* — is true, valuable and
  the right design. The **sentence** is not.
  ⇒ *Suggested fix, one clause, and it keeps every word of the point:* *"NO POSITION, BOUNDARY OR LIVE COUNT OF
  THIS FILE'S PRESENT GEOMETRY IS WRITTEN DOWN IN THIS COMMENT — the only numbers here are deltas about closed
  commits: the derived value is printed in the ledger line on EVERY run…"*
  ⇒ **Not a blocker:** no behavioural consequence, nothing is prescribed, and the guard is correct. Recorded
  prominently because `SC-§126` cl. 9 is explicit that *a false claim accompanying a correct artefact is the one
  nobody has an incentive to catch* — and because **this is the second consecutive row in which this comment's
  universal quantifier outran its measurement.** `qa/TASK-1325-report.md` and `qa/TASK-1332-report.md` both set
  the precedent of recording exactly this as a WARN with the verdict unmoved.
- **[WARN-2]** `Tools/run_suite_bounded.ps1:1143-1146` (claim) / `:1197-1211` (code) — **a SECOND, DIFFERENT
  fail-open survives, and the new sentence claims it does not. I found it by testing the claim's quantifier
  rather than its subject.** The comment now asserts *"no failure this case can emit is capable of naming those
  two addresses **UNDER ANY INPUT**, not merely under the happy one."* **Derived counter-example, walked through
  the shipped loop:** insert **one blank line** between the anchor (`:144`) and the address line (`:146`) — a
  plain reflow of a 6-line prose paragraph, the exact kind of edit six rows have already made to this header.
  The downward walk `while ($b -lt $last -and $lines[$b].Trim() -ne '')` stops **immediately** at the new blank,
  the upward walk is already at `$first`'s boundary, so `$exempt = {144}` — **non-empty, so the new fail-closed
  guard does NOT fire** — the address line falls **outside** the exemption, is scanned, and the case REDS
  **naming `:5966` and `:6172`**: the precise outcome WARN-2 was boarded to end.
  ⇒ **Mitigated and metered, which is why it is a WARN and not a blocker:** the published count drops from
  `6` to `1` in the same ledger line, so the exemption's own tripwire fires visibly — the property `TASK-1332`
  ruled load-bearing does its job here.
  ⇒ *Suggested fix, and it is PROSE, not code:* narrow the claim to the input class actually controlled —
  *"…is capable of naming those two addresses under any input that deletes or rewords the anchor."*
  ⛔ **I am explicitly NOT proposing a code change, and explicitly NOT re-proposing the narrowing** (spec (8));
  ⭐ **this finding CONFIRMS that decline from the other side** — narrowing to the marker line would make this
  fail-open unconditional and red the file on `HEAD` today. Whether the exclusion's robustness deserves a row at
  all is the **manager's** call (`SC-§100`), and my recommendation is **no row for now**: the hole is smaller
  than it was this morning, it is metered, and the cheap half of the remedy is one clause of prose.
- **[WARN-3]** `handoffs/TASK-1335-programmer.md` §7 vs §8 — **two numbers in the same handoff do not
  reconcile, and this one I can settle arithmetically.** §7's fence row says *"all **12** deleted lines are
  accounted for: **11** doc-comment prose lines + the one shape-(a) `Rx` line"*; §8's own `git diff --numstat`
  says **`45  13`**. **Derived:** the doc comment ran `1089..1145` (57 lines) and now runs `1089..1168` (80
  lines) ⇒ net **+23**; the helper body gained the 4-line parse guard, the 4-line exempt guard and one blank
  ⇒ **+9**; the `Rx` line is **+1/−1**. Solving `A−D = 32` with `A = 45, D = 13` closes **exactly** on
  **12 prose deletions + 1 `Rx`**, with **35** prose additions. ⇒ the code is fine and the fence holds; the
  handoff's accounting is **one line short of its own numstat**, so one deleted line is formally unaccounted.
  ⇒ *Fix: none in the code.* `TASK-1337` can close it for free while it has `git` open — see the Notes.
  Recorded because an accounting that does not close is exactly what this chain exists to notice, **even when
  the artefact is right.**
- **[NIT-1]** `:1147-1148` — *"the block moved `+3` under `TASK-1327`"*: **the one load-bearing number in the
  comment I could not re-measure.** It needs `git show 9d80505^:…`, and I have no shell. The row measured it
  itself across both commits (`141 → 144`) and labelled its provenance in-line, which is the correct form.
  **Accepted as declared, flagged so the provenance of my PASS is legible** (`SC-§71b`).
- **[NIT-2]** `:1148-1149` and `:1155-1156` — *"the next edit that **adds or removes a line** ABOVE it moves it
  again"* is still a sliver overbroad: an edit that adds **and** removes equal lines above moves nothing. Exact
  form: *"any edit that **changes the line count** above it."* ⛔ **Explicitly not worth a revision cycle** —
  recorded only because the row asked for strictness and because the same sliver is what WARN-1 was born from.
- **[NIT-3]** `:1177` + `:1642` — on the parse-error path the detail line prints *"header derived as lines
  **0..0**"* before the `FAIL-CLOSED` sentence. Nothing was derived in that state, so the leading clause is
  false-ish in the one state where the reader most needs precision. The `FAIL-CLOSED` text follows in the same
  joined string, so it is loud rather than misleading. ⇒ *One line: emit the boundary clause only when
  `$hdr.Last -gt 0`.* Future row; not this one.
- **[NIT-4]** `:1158` — *"This paragraph may NOT quote **those two delimiters** literally"* lost its antecedent
  when the pair count was deleted: the nearest referent is now *"same-line delimiters"* three sentences up. A
  reader gets there, but the collateral is worth naming. ⇒ *"…may not quote a block comment's own opener and
  closer literally."*
- **[NIT-5]** `:1162` — *"ALL **THREE** OF ITS ENTRY CONDITIONS"* is a live count of the **present code's**
  structure, sitting nine lines below the sentence that says no count is written down. Low rot risk because the
  same sentence **enumerates** them (*"the parse, the header boundary and the exhibit anchor"*), which makes it
  self-checking — a count that ships with its own denominator is not an address. Recorded for completeness with
  WARN-1, not as a second instance of it.
- **[NIT-6]** `:1215` — **a real, board-ordered coverage trade, recorded so it is a decision and not a
  surprise.** NIT-2's fix (colon must touch the digit) means a genuine rotted address spelled
  `CONVENTIONS.md: 5093` — filename, colon, **space**, digits — now slips **both** shapes: shape (a) requires
  the colon to touch, and shape (b)'s lookbehind is blocked by the `d`. There is **no clean remedy** (a
  minimum-width rule would re-break `:29`, which the comment rules out for good reason), the surviving gap is
  narrower than the false-red it removes, and `:1118-1122` documents the trade explicitly and correctly. ⇒ **No
  action. Correct call under `SHIP-§9`** — a false red on this header invites the hand-edit the chain exists to end.

## Notes for build-master (`TASK-1337`)

- ✅ `Verdict:` = **PASS** over `TASK-1335` — **0 BLOCKER** · 3 WARN · 6 NIT. Grep the **token**, never a line
  address (`SC-§126` cl. 10). Nothing here fences your commit.
- ⛔ **None of the 3 WARN / 6 NIT is an edit request, and none is yours to make.** Every one is prose inside
  `Tools/run_suite_bounded.ps1` or a line in a handoff ⇒ **programmer work on a future row, if the manager
  boards one.** 🚨 **A host "just fixing" this comment is the seventh hand-edit wearing a build-master's hat**
  — and WARN-1/WARN-2 are both **below** `:238`, so it would not even be caught by the guard you are shipping.
- 🚨 **YOU CLOSE MY §5 RESIDUAL, AND — UNLIKE `TASK-1333` — YOUR SPEC DOES NOT ALREADY ASK FOR IT.**
  `git show --stat HEAD` lists **files, not hunks**. One extra read-only command closes the chain's own
  headline fence:
  `git diff -U0 -- GitClaudeUnrealTest/Tools/run_suite_bounded.ps1 | Select-String '^@@'`
  ⇒ **assert the LOWEST hunk header is at or below line `1108`, i.e. ZERO hunks at or above `:238`.** That is
  the only instrument in this chain that can catch an equal-line-count rewrite of the header. ⛔ Git root is
  **one level up** (`SC-§102`) — a mis-anchored pathspec answers with **silence**, not an error.
- ✅ While that diff is open, **WARN-3 costs you nothing**: confirm the deletion side is **13**, of which **12**
  are doc-comment prose and **1** is the shape-(a) `Rx` line. If any deleted line is **not** one of those,
  **stop and report** — that would be a changed line nobody has accounted for.
- ⭐ **Your parse control: break the FINAL `#>`, never the header's own — and RE-GREP it, because IT MOVED
  AGAIN.** `TASK-1332` measured the final `^#>` at `:1145`; **I measure it at `:1168` today**, because this
  row's doc comment grew by 23 lines. `:1065` and `:1145` are both **no longer** the final close. **My pair
  count is `14`** (`^<#` = 14, `^#>` = 14, raw `<#` = 15 — the 15th is the **string literal** at `:1183`, not a
  delimiter). ⛔ Derive both yourself at your instant and quote them — mine are hearsay to you
  (`SC-§126` cl. 11).
- ✅ **Your `-SelfTest` ledger should reconcile to 55, BY NAME**, in this order: fixture corpus **11** · bound
  arithmetic **5** · lane construction **12** · expected echoes **5** · command line **4** · Aura exclusion
  **2** · bound merge **2** · `-LogPath` **7** · input tolerance **7**, with
  `SC-126: header grows no rot-prone line address` **last** and green, its detail reading
  `header derived as lines 1..238; 6 exhibit line(s) excluded by substring, never by address`. ⛔ If any name is
  missing or any detail differs, that is the equal-line-count rewrite showing itself — **stop and report.**
- Expect **no** UE suite-count delta: the row adds **no** automation test. The `-SelfTest` ledger (55) and the
  suite (561) are different instruments — ⛔ **do not reconcile one against the other.**
- The `LF will be replaced by CRLF` warning on this file is **pre-existing** (`TASK-1324`/`1327`/`1331` all
  recorded it), not from this diff.
- 🚨 **HARD FENCE, and it is live right now:** `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`
  is dirty with **`TASK-1319`'s in-flight work**, landed by another agent mid-session. ⛔ **Commit BY PATHSPEC,
  never `-a`.** A `-a` here sweeps a held lane and the board into a gated diff (`SC-§102` · `SC-§106`).

## ⚖️ For the manager — three items, none blocking the commit

1. **WARN-2 is a newly found residual fail-open in the exhibit exclusion** (a reflow that splits the paragraph
   defeats it). **My recommendation is NO row for now**: it is metered by the published count, it is strictly
   smaller than the hole closed today, and the cheap half of the remedy is one clause of prose that can ride the
   next row to touch this file. ⛔ It must **not** be cured by narrowing the block — that is ruled wrong and my
   finding reconfirms it from the opposite direction.
2. **WARN-1's sentence would be worth folding into the same future row**, together with NIT-2/3/4. None is worth
   a row of its own; all four are one paragraph of prose.
3. ⭐ **A rank correction worth carrying forward:** `TASK-1332` graded the discarded `$errors` as **NIT-4** and
   the exhibit fail-open as **WARN-2**. Measured, the NIT was the **fail-silent** (a green ledger line about a
   file that does not parse) and the WARN was the **fail-loud**. **A guard that answers green when it could not
   look outranks one that answers red about the wrong thing** — worth remembering the next time this file's
   findings are triaged by label.

## Flips performed by this gate

`TASK-1335` was `ready-for-qa` **in fact** — its `names` block fences it from `TASKBOARD.md`, so it could not
flip itself (handoff §11), exactly as `TASK-1331` could not. **Both flips are mine**, made with `Edit` on
task-ID-bearing anchors, ⛔ never `replace_all` (`SC-§127`), each read back as **STATE** afterwards (`SC-§104`):

- `TASK-1335` → **`qa-passed`**
- `TASK-1336` → **`qa-passed`**
