# QA Report — TASK-1325

subject: `TASK-1324` (marker `TASK-1324-SUITE-RUNNER-CITATION-ROT`)
gate: `TASK-1325` · host: `TASK-1326`
reviewer: qa-reviewer · 2026-09-19
Verdict: **PASS** — 0 BLOCKER · 1 WARN · 2 NIT

> Dogfooding `SC-§126` cl. 10(a): the `Verdict:` token is above, its position unconstrained, with a
> title and a front-matter block over it. No line-1 contortion.

---

## 0. THE HEADLINE — CHECK ONE, RE-MEASURED AT MY OWN INSTANT

✅ **The fix cites the SUBSTRING `THE FILE EXISTS`. It contains NO address. Not `:5122`, not
`:5140`, not any digit at all.** I read the replacement text directly rather than inferring it from
the diff, and I swept the whole file for addresses with a pattern deliberately broader than the
programmer's.

**My grep, my instant, with a negative control (`SC-§39`):**

```
grep -n "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md
  -> 1 hit:  5140:  ... ⇒ ⚖️ ***⛔ *"THE FILE EXISTS"* IS ⛔ NOT *"THE TOOL RUNS"*. ⛔ CORRECTING
              THE ⛔ FIRST SENTENCE DOES ⛔ NOT DISCHARGE THE ⛔ SECOND ...***

grep -c "THE FILE EXISTZ" .claude/pipeline/CONVENTIONS.md        # negative control
  -> 0
```

**1 hit, unique, and it resolves to the correct sentence** — not merely to *a* line. The hit is
verbatim the claim the comment attributes to it, and `CONVENTIONS.md:5141` immediately below carries
the *"quietly welded RUNS to KILLS"* finding that the comment's next clause paraphrases. The needle
is discriminating (control returns 0), so the `1` is not a facade (`SC-§96`).

### 0a. THE ROW'S OWN THESIS IS CONFIRMED — THE LAW MOVED AGAIN, THE SAME DAY

| reader | instant | `THE FILE EXISTS` sat at |
|---|---|---|
| the board / the manager | 2026-09-19 | `:5122-5123` |
| `TASK-1323` (the gate) | 2026-09-19, later | `:5122` |
| `TASK-1324` (the fix) | 2026-09-19, later still | `:5140` |
| **me, now (independent)** | 2026-09-19, later still | **`:5140`** |

⚖️ **The programmer's claim stands at my instant.** The address moved `+18` between the gate that
measured it this morning and the fix that removed it this afternoon. Had `TASK-1324` written the
prescribed-looking `:5122`, it would have shipped **wrong on the day it was written** — which is
`SC-§126` cl. 8's measured trap exactly, one chain later. The substring has not moved and cannot.

---

## 1. FINDINGS

- **[WARN]** `handoffs/TASK-1324-programmer.md` §1c — the heading **"no address survived anywhere in
  the file, not merely in my new text"** is an **overclaim of an under-powered instrument**. It is
  false as written, and the sweep behind it could not have detected the counter-examples.
  The four greps quoted (`CONVENTIONS\.md:[0-9]`, `5093`, `5122|5140`, `two lines earlier`) all
  return 0 — but `CONVENTIONS\.md:[0-9]` **cannot match the form actually present in the file**,
  which is `CONVENTIONS.md at :5966-5969` (note the ` at ` between). My own broad sweep
  `:[0-9]{3,4}` over the whole file returns **nine** surviving address sites: `:21`, `:87`, `:88`,
  `:143`, `:293`, `:475-478`, `:482-483`, `:647`, `:1361`.
  ⚠️ I made the same mistake first and caught it only by *reading* the header — recording that,
  because it is the point: the narrow pattern returned the right answer for the wrong reason, with
  no positive control (`SC-§39`, and `SC-§126` cl. 9(b) — *this project's own prose defeats the
  obvious search*).
  **Not a BLOCKER, and it does not move the verdict:** every one of the nine is **pre-existing**
  text outside both diff hunks, and I verified the artefact independently. Suggested fix: no code
  change. In the handoff, narrow the sentence to what was measured — *"no address in the new text;
  the pre-existing ones are at §5 and §1c-note."*

- **[NIT]** `Tools/run_suite_bounded.ps1:143` — the surviving `CONVENTIONS.md at :5966-5969` and
  `:6172-6175` are **correct as-is and should not be touched**: they are framed as *"and BOTH
  rotted"*, i.e. they are **evidence of rot, not citations**. Recorded only so a future sweep does
  not flag them as live citations and "fix" them into silence.

- **[NIT]** `Tools/run_suite_bounded.ps1:21, :293, :475-483, :1361` — engine C++ citations
  (`EditorServer.cpp:5993`, `PluginManager.cpp:2043/1587/…`). These cite **engine source**, not a
  project file this pipeline edits, and they already name their functions
  (`FindCommandLinePlugins`, `FindTargetPlugins`), which is `DECK-§9` cl. 9's form. Out of this
  row's scope; no action.

- **No BLOCKER found, and I did not hunt for one (`SC-§59` cl. 5).** B1 and B2 were fixed **by
  construction** — a substring cannot rot and a number-free phrase cannot be off by two — so PASS
  was the expected verdict and it is the measured one.

---

## 2. THE SPEC'S CHECKS, ONE BY ONE

### (1) SUBSTRING, NOT A FOURTH NUMBER — ✅ PASS

**The NEW text, read directly at `:169-178`:**

```
169     DO NOT "TIDY" (c2) AWAY WHILE FIXING (a). Deleting the true sentence next to a false
170     one is how this block got wrong in the first place: "the file exists" was read as "the
171     tool runs", and the correction that caught that then quietly welded "runs" to "kills".
172     That law sentence is cited here the way everything else in this block is cited -- by
173     SUBSTRING, never by address. It earned that twice over: the address this line used to
174     carry was wrong on arrival (it pointed at a different section entirely), and the true
175     address then moved AGAIN, the same day, between the gate that measured it and the fix
176     that deleted it. A fourth number would have rotted too.
177
178         grep -n "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md    (one hit)
```

⛔ **Zero digits in `:169-178`.** Not an address, not a bare number — `(one hit)` is a word. The
strictest reading of the spec (*"an address anywhere in the new text is a BLOCKER even if currently
correct"*) is satisfied with room to spare.

**Whole-file address sweep, my own, broader than the handoff's:**

| site | text | in the diff? | verdict |
|---|---|---|---|
| `:21`, `:293`, `:475-483`, `:1361` | `EditorServer.cpp:5993`, `PluginManager.cpp:…` | no | pre-existing, engine source |
| `:87-88` | `:1680` (launch) / `:1677` (receipt) | no | **the declared residual — §4 below** |
| `:143` | `CONVENTIONS.md at :5966-5969` / `:6172-6175` | no | pre-existing, historical ("BOTH rotted") |
| `:647` | `05.48.32:329` | no | a log timestamp, not an address |
| **`:169-178` (new)** | — | **yes** | ✅ **no address** |
| **`:128` (new)** | `…-20260917-211827.log` | **yes** | ✅ a **filename**, pre-existing, not an address |

Also confirmed absent file-wide: `5093` → 0 · `5122` → 0 · `5140` → 0 · `5072` → 0.

### (2) NIT-1's WORDING — ✅ PASS

```
128              ...-20260917-211827.log ends a few lines earlier inside the same orderly
```

Number-free, as the row prescribes. `grep -c "two lines earlier"` = **0** file-wide. No corrected
number and therefore no measurement date needed.

⭐ I accept the programmer's §4 argument and record it as the stronger ground: the count is not
merely rot-prone, it is **not well-defined** — the sentence generalises over a population of logs
whose gap is 5 against most peers and 6 against three, so *any* single integer is false against part
of its own reference class. `"a few"` is **exactly as precise as the truth is**. (The population
measurement itself is accepted-as-declared — see §3.)

### (3) ZERO EXECUTABLE LINES — ✅ PASS on structural evidence, with one residual named and homed

🚨 **I re-derived this myself and did NOT take the handoff's claim — but I must be exact about what
my re-derivation can and cannot reach.** The `Bash` tool is **disabled for this session**, and my
role holds **no Git access by design**. I therefore could not run `git diff`. ⛔ **I declined to
route git through the read-only Unreal inspector's Python: that would circumvent a designed fence,
not work around a missing tool.**

**What I measured myself, from the file and from the prior gate's report:**

| derivation | my measurement | verdict |
|---|---|---|
| header block bounds | `<#` `:1` → `#>` **`:234`** | — |
| first executable token | `[CmdletBinding(DefaultParameterSetName = 'Run')]` **`:236`**, `param(` **`:237`** | below `:234` ✅ |
| new citation text | **`:169-178`** ∈ [1,234] | ✅ inside |
| NIT-1 change | **`:128`** ∈ [1,234] | ✅ inside |
| `#>` before (from `qa/TASK-1323-report.md`) | `:228` → now `:234` | **+6** |
| `#19` orphan census before (same source) | `:1766-1769` → now **`:1772-1775`** | **+6**, logic byte-identical in shape |
| launch site | `Start-Process -FilePath` → **`:1686`** | **+6** from `:1680` |
| receipt site | `Set-Content … ($LogPath + '.cmdline')` → **`:1683`** | **+6** from `:1677` |

**The +6 signature, against an independent source.** I took the BEFORE census from
`qa/TASK-1323-report.md` (the prior gate's own report), **not** from the handoff:

```
BEFORE (TASK-1323):  1→228  303→312  362→372  395→408  510→515  576→584  639→643
                     666→679  787→798  881→887  941→954  1022→1025  1047→1055
AFTER  (mine):       1→234  309→318  368→378  401→414  516→521  582→590  645→649
                     672→685  793→804  887→893  947→960  1028→1031  1053→1061
```

**All 25 movable delimiters moved by exactly +6; the immovable `<#` at `:1` did not move.** The body
below the header is displaced by precisely the header's growth — the signature of a comment-only
diff.

⚠️ **THE RESIDUAL THIS CANNOT CLOSE, STATED PLAINLY: an equal-line-count substitution BELOW `:234`
would preserve every displacement above and be invisible to all eight rows of that table.** This is
not a new worry — it is **verbatim the residual `TASK-1323` declared for itself** (its report: *"…
equal-sized substitution below `:228`. **Only `git diff` can, and that is `TASK-1318`'s.**"*).
⇒ **Homed, not inherited: it belongs to `TASK-1326`**, which runs both the parse and **one real
suite run through the edited script**. A substituted executable line would have to survive both.
Structural proof outlives its evidence (`SC-§116`'s own bullet); a tally does not.

### (4) DELIMITER CENSUS 13/13 BOTH SIDES — ✅ PASS

- **AFTER (my own grep):** 13 `<#`, 13 `#>`, strictly alternating, **no nesting** — I verified each
  close precedes the next open pairwise (`234<309`, `318<368`, `378<401`, `414<516`, `521<582`,
  `590<645`, `649<672`, `685<793`, `804<887`, `893<947`, `960<1028`, `1031<1053`).
- **BEFORE:** 13/13 per `qa/TASK-1323-report.md` — an independent source, not the handoff.

⇒ **13/13 both sides.** No unbalanced delimiter turned comment text into code, which is the failure
this check exists to catch and the one a "zero executable lines in the diff" reading would miss.

### (5) FENCES — ✅ PASS, all measured

| fence | my measurement | verdict |
|---|---|---|
| bounds / parameter defaults | `$OverallSeconds = 1500` `:249` · `$BootSeconds = 420` `:250` · `$StallSeconds = 180` `:251` | ✅ `TL-§6`'s triple intact |
| `-ExecCmds` composition | `('-ExecCmds="{0}"' -f $ExecValue)` `:503` · `New-EditorCommandLine` `:432` | ✅ untouched |
| comma-vs-semicolon separator | SUITE `;Quit` `:26` · COMMAND `,QUIT_EDITOR` `:32` · `ParseExecCommands.cpp:29` note `:1637` | ✅ untouched |
| `QUIT_EDITOR` | `$script:Terminator = 'QUIT_EDITOR'` `:294` | ✅ untouched |
| `#19` orphan-census logic | `:1772-1775`, nothing above `:234` mentions it | ✅ untouched |
| `Saved/**` · `TASKBOARD.md` · `CONVENTIONS.md` | absent from the dirty set | ✅ |
| **ZERO files of `TASK-1313`'s** | `SiegePlayerController.cpp` · `handoffs/TASK-1311-programmer.md` · `qa/TASK-1312-report.md` | ✅ — see the note below |

⚠️ **`SiegePlayerController.cpp` IS dirty in the working tree — and that is correct and not
`TASK-1324`'s.** It is **`TASK-1314`'s live work**, running in parallel by the orchestrator's own
declaration. It is `TL-§5e` cl. 7a-v **HELD-FOR** a named host, not an orphan. Flagged to
build-master in §5 because it is a live pathspec hazard.

### (6) ACCEPTED-AS-DECLARED — NAMED, NOT CLAIMED (`SC-§71b`)

I did not measure the following. I am not asserting them; I am recording that the verdict rests on
them as declared, and naming who earns each.

1. **`Parser::ParseFile` = 0 errors / `tokens = 9598`.** My spec forbids me the parse control and I
   have no shell at all. ⇒ **`TASK-1326` (1).** ⛔ And the control must break the **FINAL `#>` at
   `:1061`, NEVER the header's own at `:234`** — breaking the header's re-pairs against the next of
   13 block comments and returns a silent **0**, i.e. a control that cannot fail. Corroborated
   twice: `qa/TASK-1323-report.md` and `CONVENTIONS.md` `SC-§126` cl. 8's kinship bullet, which
   records it as one of two of the manager's own gates that failed `SHIP-§9` in one day.
   ⇒ **The programmer's `0` is a real reading of an unvalidated instrument.** He said so himself
   rather than letting it be found, which is the correct disclosure.
2. **`git diff --stat` = 1 file / 9+ / 3− · `--numstat` · the hunk headers `@@ -128 +128 @@` and
   `@@ -171,2 +171,8 @@` · the 1,815 → 1,821 line count · `git status` cleanliness.** No Git access
   by role design; `Bash` disabled. My structural re-derivation in (3) corroborates every one of
   these **consistently** (+6 everywhere, both hunks inside the header) but does not *replace* them.
   ⇒ **`TASK-1326` (4)** verifies the commit.
3. **The suite run.** ⇒ **`TASK-1326` (2)**.
4. **The §4 log-population measurements** (delta 5 on 18 runs, 6 on 3, 2 with no marker; the
   7,401-line tail of `…-211827.log`). Not re-measured. **Non-load-bearing** — the shipped wording
   is number-free, so no number in the artefact depends on them.

---

## 3. THE TWO THINGS THE ROW HANDS ME THAT ARE **NOT** FAILURES

### 3a. The previously fenced range — DECLARED, AUTHORISED, AND CORRECTLY LIMITED

✅ **Confirmed: NIT-1's site is inside a previously fenced range, and that is not a failure.** At my
instant `(c2)` spans **`:123-131`** and the NIT-1 sentence is at **`:128`** — inside it.
`TASK-1317` fenced it as `(c2)`; `TASK-1310` correctly declined; `TASK-1323` upheld the decline as
*"not actionable this loop"*. `TASK-1324`'s spec (3) instructs the row to **say so and fix it anyway
under this row's own authority**, and it did exactly that in §3 of its handoff. ⛔ **I do not fail
the row for editing it.**

**I verified nothing else moved inside `(c2)`**, asserting **STATE, not a tally** (`SC-§104`):

- `NOT MEASURED — NULL WITH NO POSITIVE CONTROL` occurs at **`:123`**, **`:161`**, **`:162`**, with
  the U+2014 em dash verbatim. Each attaches to a bound that genuinely **is** not measured:
  `:123` = the `(c2)` BOOT+STALL verdict · `:161` = `-BootSeconds` · `:162` = `-StallSeconds`.
  ⇒ the state is right at all three sites. (`TASK-1323` (7)(a) already established that **3** is the
  correct count and that a gate counting **2** would red a correct fix; I am not re-litigating it,
  and I did not count my way to a verdict.)
- The three-way split is intact: `(c1)` at `:111`, `(c2)` at `:123`, and `(c1)`'s corpse citation
  (`…-045537.log`, 134 started / 133 completed) is untouched.
- The two verbatim-quoted law verdict lines at `:161-162` are unchanged.

### 3b. THE DECLARED RESIDUAL — **MY RULING, IN WRITING**

**The facts, re-measured by me, not inherited.** The clause (a) aside at `:87-88` reads *"they stood
at `:1680` (launch) and `:1677` (receipt)"*. I ran the two greps the paragraph itself prescribes:

```
grep -n "Start-Process -FilePath"  ->  1686    (aside says 1680)
grep -n "+ '.cmdline'"             ->  1683    (aside says 1677)
```

⇒ **Both are stale by exactly +6 — the displacement `TASK-1324`'s own insertion caused.** The
programmer's §5 is accurate at my instant.

#### RULING 1 — leaving them is **CORRECT**. No BLOCKER, no WARN against `TASK-1324`.

Three grounds, the first of which is the law speaking about *this very paragraph*:

1. **`SC-§126` cl. 8 blesses this exact shape by name.** Its "better still" bullet reads: *"LEAD
   WITH A GREP ANCHOR AND DEMOTE THE NUMBER TO A DATED ASIDE ⇒ **a number that is DECORATION cannot
   rot LOAD-BEARINGLY**. That is what the shipped clause (a) does, and it is why the same fix
   survived the law moving FOUR TIMES in one day."* The paragraph leads with its two greps and
   **orders the reader not to use the digits**: *"FIND BOTH SITES BY GREP, NOT BY LINE NUMBER --
   EDITING THIS BLOCK MOVES THEM, which is how the previous revision went stale."* I obeyed that
   sentence and it took me to the right lines. The mechanism worked.
2. **Renumbering would have written an address — the species this row exists to delete**, and my own
   check (1) makes that a BLOCKER *even when currently correct*. `SC-§126` cl. 8's measured event is
   precisely a **gate that prescribed a defect** by prescribing an address against a pre-fix file.
   Had the programmer written `:1686`/`:1683`, I would have had to blocker it. He was right, and he
   was right for the reason the law gives.
3. **`SC-§59` cl. 5** — rewriting the aside re-opens text `TASK-1323` already passed, on a row scoped
   to two named defects.

#### RULING 2 — **YES, the aside owes a follow-up row.** But to **DELETE the digits, never to renumber them.**

The programmer's stated weakness is real, and I am upgrading it from *"a manager/QA call"* to a
named recommendation:

- The aside is stamped *"As this paragraph was last written (2026-09-19)"* — and `TASK-1324`'s edit
  is **also 2026-09-19**. ⇒ **The dated-aside mechanism cl. 8 relies on has, in this one instance,
  degraded to zero discriminating power.** The stamp resolves to the day; two edits landed on the
  same day; a reader today cannot tell from the stamp that the numbers moved after it was written.
  The decoration is still decoration — but its "dated" half has stopped carrying information.
- **cl. 8(b) says the row iterates *"until the numbers stop moving."* For a number displaced by
  every future growth of the block it lives in, the only fixed point is no number.** cl. 8's own
  "better still" and cl. 7's remedy (*"QUOTE THE TEXT YOU ARE CLAIMING ABOUT"*) both point there,
  and `TASK-1324` has just demonstrated the terminal form one paragraph below.

✅ **RECOMMENDED FOLLOW-UP (the manager's to board — ⛔ not mine to make, and ⛔ not `TASK-1326`'s to
sweep in):** a comment-only row deleting the two digits `:1680` / `:1677` from the clause (a) aside,
keeping both grep anchors and the provenance sentence (*"both RE-GREPPED against the finished file,
never offset from a diff"*, and the `+69/+70` cautionary tale, which is **history and carries no live
address**). One-line deletion. It removes the **last rotting address in the header** and makes the
paragraph permanently immune to its own growth.

⛔ **NOT urgent, and explicitly NOT a blocker on `TASK-1326`.** The digits are decoration by the
law's own definition, the paragraph self-defends, and a reader who follows its instruction is never
misled. Boarding it is `SC-§50` hygiene — a declared gap with no row is an orphan, and this chain
exists because two of those shipped.

---

## 4. WHAT I INSPECTED, AND WHAT I DID NOT

Per my standing duty to separate looking from accepting:

- **Inspected by direct read/grep at my instant:** `Tools/run_suite_bounded.ps1` (the whole header
  block `:1-234`, plus every fence anchor below it by grep) · `.claude/pipeline/CONVENTIONS.md`
  (`SC-§126` cl. 7/8/9/10 in full, and the `THE FILE EXISTS` site) · `qa/TASK-1323-report.md` ·
  `handoffs/TASK-1324-programmer.md` · the `TASK-1324` / `TASK-1325` / `TASK-1326` board rows.
- **The Unreal inspector was connected (`editor_connected`) and I did not need it.** This row's
  subject is a PowerShell comment block; there is no asset, graph or editor state in scope. I
  called only `get_headless_status`. ⛔ No lifecycle tool, no mutation.
- **Not inspected:** git (no access by role design, `Bash` disabled), the parser, the suite — all
  enumerated in §2(6) with their owners.

---

## 5. NOTES FOR BUILD-MASTER (`TASK-1326`)

1. 🚨 **The positive control breaks the FINAL `#>`, which at my instant is `:1061` — RE-GREP IT
   YOURSELF, it is the last of 13.** ⛔ Breaking the header's own `#>` (`:234`) re-pairs against the
   next block comment and returns a silent `0`: a control that cannot fail. Expect **1** error and
   quote its text; a `0` from the control means your instrument is wrong, not that the file is fine.
2. 🚨 **PATHSPEC HAZARD — `SiegePlayerController.cpp` is dirty and is `TASK-1314`'s, not this
   chain's.** Commit **by pathspec, never `-a`**. And under `TL-§5e` cl. 7a's derived orphan sweep:
   if `handoffs/TASK-1314-programmer.md` (or its gate's report) lands in your staging window, it is
   **HELD-FOR a named host** under cl. 7a-v — leave it. Verify the **commit** (`git show --stat
   HEAD`), never the index (the editor's Git provider auto-stages).
3. **You own the one residual this gate could not close** (§2(3)): an equal-line-count substitution
   below `:234` is invisible to every structural check available to me. Your `git diff` and your
   real suite run are what close it. `TASK-1323` declared the identical residual and homed it to
   `TASK-1318`; this is the same hand-off, one chain later.
4. **Baseline for the suite triple:** `TASK-1318`'s own pass (`SC-§95`). Count the `401`
   **signature** (`HTTP 401` / `Unauthorized`), **never the digits** — a bare `grep -c '401'`
   returned 8 on `TASK-1318`'s log, all timestamps and frame counters.
5. **No C++ compile and no 5b** — say both in one line rather than leaving them unmentioned; the
   diff is a PowerShell comment and the row has no runtime acceptance criterion.
6. **Three flips are yours** (`SC-§103`): `TASK-1324`, `TASK-1325`, `TASK-1326` — I have already
   flipped the first two to their post-gate values; yours are the post-commit ones. **Commit before
   you edit the board.**
7. **`TASKBOARD.md` and `CONVENTIONS.md` are concurrently written by up to ten agents and have no
   lock.** Re-read immediately before each dependent edit.

---

## 6. VERDICT

**Verdict: PASS.** 0 BLOCKER · 1 WARN · 2 NIT.

Check one — the reason this gate exists — is **clean**: the fix used the substring `THE FILE
EXISTS`, verified by me at my own instant as **1 unique hit with a discriminating negative control**,
and there is **no address and no digit** anywhere in the new text. The row's central thesis is
corroborated rather than merely asserted: **the law moved again the same day**, so the
natural-looking remedy `:5122` would have shipped false within hours of being written.

The single WARN is a handoff sentence that claims more than its instrument could measure — the
artefact is correct, the prose about it was over-broad, and I hit the same blind spot myself before
catching it by reading. The declared residual is **correctly left alone by this row** and **owes a
follow-up row to delete its digits, not to refresh them**.
