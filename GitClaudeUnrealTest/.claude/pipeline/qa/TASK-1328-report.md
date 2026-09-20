# QA Report — TASK-1328

- subject: `TASK-1327` (marker `TASK-1327-SUITE-RUNNER-ASIDE-DIGIT-DELETE`)
- gate: `TASK-1328` (marker `TASK-1328-SUITE-RUNNER-ASIDE-GATE`)
- host: `TASK-1329`
- agent: qa-reviewer
- date: 2026-09-19
- file under review: `Tools/run_suite_bounded.ps1` (header comment block only)
- handoff read WHOLE (`SC-§38a`): `.claude/pipeline/handoffs/TASK-1327-programmer.md`

**Verdict: PASS** — 0 BLOCKER · 0 WARN · 4 NIT · 1 declared residual (`TASK-1329`'s to close)

---

## 0. Instrument note — what I hold and what I do not

I have **no `Bash`** in this session, therefore **no `git diff`**. Every structural claim below
was re-derived from the file itself with `Grep`/`Read`, never inherited from the handoff
(`SC-§71b`, `SC-§91`). The one check that genuinely requires a diff — *"the two hunks are the
**only** changes"* — is **declared as a residual in §7**, not asserted. I did **not** route git
through the read-only Unreal inspector: that would circumvent a designed fence rather than work
around a missing tool. I ran no compile, no suite, no parser, no commit.

Every absence sweep below carries a **positive control** (`SC-§126` cl. 9(b-2)): before any zero
is reported, the pattern is shown firing on a known-present instance of **the form actually
present in this file**. A negative control proves only that a needle discriminates; it cannot
tell you the needle can reach the haystack's actual shape. That is the exact hole `TASK-1325`
recorded, and it is the hole this gate exists not to repeat.

---

## 1. CHECK (1) — DELETED, NOT RENUMBERED ✅ PASS

**Sweep A — the four forbidden values.**

```
pattern  1680|1677|1686|1683   over the WHOLE file  ->  0 matches
POSITIVE CONTROL, same alternation form:
pattern  5993|1478             ->  21:5993 · 297:5993 · 1365:1478   (FIRES)
```

The needle matches bare 4-digit runs that are present; its zero on the four forbidden values is
therefore a fact about the file.

**Sweep B — the broad address form, which is the sweep that matters.**

```
pattern  :[0-9]{3,4}   over the WHOLE file, occurrence-wise:
  21::5993      <- engine source (EditorServer.cpp), spec (4) out-of-scope, blessed
 146::5966      <- THE EXHIBIT
 146::6172      <- THE EXHIBIT
 297::5993 · 479::2043 · 479::2049 · 480::1587 · 481::1592 · 482::1597
 486::1636 · 486::1709 · 487::1748 · 651::329 · 1365::1478   <- all BELOW the header
```

Inside the header block (`:1`–`:238`) the **only** address hits are `:21` (engine source) and
`:146` (the exhibit). **Nothing at `:87`–`:93`** (the rewritten aside) and **nothing at
`:181`–`:182`** (the recipe). This pattern is its own positive control: it fires on the
` at :5966` form — the very form that `CONVENTIONS\.md:[0-9]` could not match and that fooled
both the programmer and the gate one chain ago.

**Digits that DO appear in the new aside, each adjudicated:** `TASK-1327` (a task ID — a stable
identifier, not a file position) · `+69` / `+70` and *"wrong by one"* (history about a closed
past arithmetic error, blessed by name in spec (2)(c)) · `SC-126 cl. 7` (a law clause ID, the
project's canonical rot-proof handle). **No address, no live count.**

### ⭐ 1a. The measurement that settles the ruling — and it is mine, not the row's

I resolved the two grep anchors against the current file:

```
grep "Start-Process -FilePath"  ->  85:  (the doc anchor)  ·  1690:  $proc = Start-Process -FilePath $EditorCmd ...
grep "+ '.cmdline'"             ->  86:  (the doc anchor)  ·  1687:  Set-Content -LiteralPath ($LogPath + '.cmdline') ...
```

**The true sites are `:1690` and `:1687` at my instant.** `TASK-1325` measured them at `:1686`
and `:1683`. This row's own `+4` net lines displaced them by exactly `+4`.

⇒ ***Had the row written `:1686`/`:1683` — the values that were CORRECT when the gate measured
them — they would have been WRONG before the row that wrote them was even committed.*** The
staling edit and the citation would have been **the same edit**. That is not an argument for
deleting the digits; it is a measurement of the fixed point's non-existence, taken inside the
window the fix occupies. `SC-§126` cl. 8(b)'s *"iterate until the numbers stop moving"* has no
solution here, and this is the receipt.

**Both anchors resolve, and they are the entire remedy.** A citation-free paragraph is only safe
if its replacement actually finds the thing; I checked that rather than assuming it.

---

## 2. CHECK (2) — THE `:143` EXHIBIT ✅ PASS (untouched; now at `:146`)

Current `:146`–`:147`:

```
    CONVENTIONS.md at :5966-5969 and then at :6172-6175, and BOTH rotted with nothing
    going red -- a clean diff, a green parse and a false sentence.
```

Compared against the **independent pre-edit quote** in `qa/TASK-1325-report.md` (NIT-1, written
before this edit existed): *"the surviving `CONVENTIONS.md at :5966-5969` and `:6172-6175` … they
are framed as 'and BOTH rotted'"*. Every protected component — both address pairs, the ` at `
form, the *"and BOTH rotted"* framing — is **present and identical to a source that is not the
programmer**. The exhibit moved `:143` → `:146` by this row's `+3` in the aside; a move is not a
touch.

⚠️ **Limit, declared:** this is **component-identity against an independent pre-edit quote**, not
**byte-identity via diff**. I cannot produce the latter without `git`. `TASK-1329` closes it.

---

## 3. CHECK (3) — THE SURVIVORS (2)(a)–(c) ✅ PASS

| spec item | site | status |
|---|---|---|
| (a) `grep -n "Start-Process -FilePath"   -- the launch` | `:85` | present, verbatim, **and resolves** (→ `:1690`) |
| (a) `grep -n "+ '.cmdline'"              -- the receipt` | `:86` | present, verbatim, **and resolves** (→ `:1687`) |
| (b) *"FIND BOTH SITES BY GREP, NOT BY LINE NUMBER -- EDITING THIS BLOCK MOVES THEM, which is how the previous revision went stale"* | `:83`–`:84` | present, verbatim vs the **spec's own quote** (line-wrapped; terminal `:` introduces the two greps) |
| (c) provenance *"RE-GREPPED against the finished file, never offset from a diff"* | `:88`–`:89` | present, verbatim clause, re-tensed to *"they had been"* |
| (c) the `+69` / `+70` cautionary tale | `:89`–`:90` | present, verbatim |

**On the re-tensing.** The spec quotes (c) as *"**both** RE-GREPPED against…"*. The word *both*
referred to the two deleted addresses; with them gone it has no antecedent, so the sentence could
not be preserved word-for-word without becoming ungrammatical. The programmer supplied an explicit
referent (*"The two addresses this paragraph used to carry"*) and kept the clause itself intact.
**The item was not lost; it was re-tensed as the subtraction forced.** Not a finding.

---

## 4. ⚖️ THE DATE-STAMP RULING (spec (2)(d)) — STATED, AND I ENDORSE IT

Spec (3) requires only that the call be **stated**; a silent drop would be the finding. It is
stated, at handoff §1(d). Beyond that bare compliance, the dispatch asks me to **rule on the
judgement itself**, so I do:

**The drop is not merely permitted — it is affirmatively correct, on four independent grounds.**

1. **Nothing is left for it to bound.** I read the whole replacement paragraph (`:87`–`:93`). It
   contains exactly two kinds of statement: **closed history** (the addresses were deleted; an
   earlier revision miscalculated `+69` against `+70`; they went stale anyway) and a **general
   principle** (a citation every later edit moves has no value to iterate toward). Neither class
   has a truth value that expires. A date stamp bounds the validity of time-sensitive claims;
   there are none left to bound. The stamp would be decoration wearing the costume of provenance.
2. **The stamp was measured at zero discriminating power — in this exact instance.** `TASK-1325`'s
   finding: the aside is stamped `2026-09-19` and the edit that staled it is **also** `2026-09-19`.
   I can now add a third and fourth same-day edit to that same block (`TASK-1310`, `TASK-1324`)
   plus this one. A stamp whose granularity is coarser than the edit rate of the thing it stamps
   cannot answer the only question a reader asks of it — *"did this go stale after it was
   written?"*
3. **The grammar forbade keeping it verbatim anyway.** The stamp read *"As this paragraph was last
   written (2026-09-19) **they stood at** …"*. It is not a standalone stamp; it is the
   **protasis of the sentence whose predicate is the two digits**. Delete the digits and the
   "As of X…" construction dangles with no Y. Keeping it would itself have required a rewrite —
   so "keep" was never the conservative option it looks like.
4. **`(TASK-1327)` is strictly more information, and it cannot rot.** A task ID resolves to a board
   row, a spec, a handoff, a gate report and a commit; `2026-09-19` resolves, in this block, to at
   least four different edits. The substitution trades an ambiguous handle for a unique one, and
   a task ID has no "correct current value" to drift away from — it is not a measurement.

The programmer's own stated reason — *"a date stamp left hovering over a number-free paragraph is
an open invitation for the next editor to hang a fresh number under it"* — is the sharpest of the
five and I adopt it. This block's entire failure history is editors helpfully supplying numbers
where a slot seemed to want one. Removing the slot is the remedy; removing the digit alone is not.

---

## 5. CHECK (2-bis) — THE SECOND EDIT ✅ PASS (deleted, not renumbered)

```
pattern  \(one hit\)|\(two hits\)|\(2 hits\)|one hit|two hits   (case-insensitive)  ->  0
POSITIVE CONTROL, same parenthetical-literal form:
pattern  \(one hit\)|\(c1\)   ->  114:(c1) · 132:(c1) · 155:(c1)   (FIRES)
```

The needle matches short parentheticals that exist here; its zero on the count is a fact about the
file. `grep -i "hit"` over the whole file returns exactly one header-block line — `:182`, the new
clause — and five body lines (`$hits` in the echo-guard logic, untouched).

**Survivors, all three, verbatim** (`:181`):

```
        grep -n "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md
```

the `grep -n` invocation · the quoted substring `"THE FILE EXISTS"` · the path
`.claude/pipeline/CONVENTIONS.md`. Corroborated against the **board spec's own quote** of the old
line (an independent source), which differs only by the trailing `    (one hit)`.

**The replacement clause** (`:182`) carries **no digit and no address** — read directly, and
confirmed by the `:[0-9]{3,4}` sweep returning nothing in that range.

**On "nine characters" vs "thirteen".** The spec called it a nine-character subtraction; the
handoff reports thirteen (nine for the parenthetical, four for the spacing that existed only to
hold it). Both are right about different spans, the discrepancy is declared, and no trailing
whitespace was left behind — verified:

```
pattern  [ \t]+$  over the whole file  ->  0
POSITIVE CONTROL that the end-anchor binds:  remedy \(SC-126 cl\. 7\)\.$  ->  93:  (FIRES)
```

### 5a. 🔁 MY OWN RUN OF THE RECIPE (`SC-§91`) — reported as mine, not reconciled to anyone's

```
grep -n "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md
  4488: ... THE LEDGER FOR ONE SENTENCE - `THE FILE EXISTS` - IN ONE WORKING DAY: ...
  5181: ... "THE FILE EXISTS" IS NOT "THE TOOL RUNS". CORRECTING THE FIRST SENTENCE ...
```

**My count: 2.** It happens to agree with the programmer's and to differ from the boarding
finding's addresses (`:4466`/`:5152` → `:4488`/`:5181` — the substring resolved identically while
both addresses moved, which is the row's thesis for free, a third time). Per check (2-bis) a
differing count is **not a defect and is expected to drift**; I record mine and reconcile nothing.

---

## 6. ⭐ THE ORDINAL FINDING — I REPLICATED IT INDEPENDENTLY, AND IT HOLDS

The dispatch asks me to verify the sort order myself rather than accept the fix author's word.
I did, by extracting a bounded window around each hit:

| `grep -n` position | line | what it actually is |
|---|---|---|
| **1st** | `:4488` | *"**THE LEDGER** FOR ONE SENTENCE — `THE FILE EXISTS` — IN ONE WORKING DAY: `:5122-5123` → `:5122` (TASK-1323, the gate) → `:5140` (TASK-1324, the fix) → …"* ⇒ **the commentary ABOUT the sentence** |
| **2nd** | `:5181` | *"«THE FILE EXISTS» IS NOT «THE TOOL RUNS». CORRECTING THE FIRST SENTENCE DOES NOT DISCHARGE THE SECOND…"* ⇒ **the law sentence itself** |

**CONFIRMED: the ledger sorts FIRST**, ~693 lines above the law section it describes. The spec's
own suggested wording — *"the first hit is the sentence; later hits are ledger entries quoting
it"* — **was already false when it was written**, and is still false at my instant. A reader
obeying it lands on `:4488`, the commentary, and reads meta-text as primary law.

**The generalisation stands, and it earns its place in the law:**

> ***An ordinal is a number wearing a word.***

`first` is exactly as falsifiable as `one`; it rots on the identical event (somebody writes about
the sentence); and it rots **worse**, because a stale count lands you on nothing and announces
itself, while a stale ordinal **lands you on a real line and says nothing**. The failure is
silent and the reader has no cue to doubt it. That asymmetry is the part worth boarding: the
remedy for a rotting count must not be a word that encodes the same measurement.

Note also that the fix's own drafting process reproduced the disease one level down — *drafting
the cure for a rotted count nearly wrote a rotted ordinal, in the spec that prescribed the cure*.
The programmer caught it by measuring instead of transcribing. That is `SC-§91` doing exactly what
it is for, and it is the reason this row is a PASS rather than a second loop.

---

## 7. ⚖️ IS THE NEW CLAUSE ITSELF ROT-PROOF? — YES, operationally. Here is the audit.

Clause under test (`:182`): `-- every hit is that sentence, or a later entry quoting it`

**(a) No count, no ordinal, no position, no address.** *"every"* is a universal quantifier over
the hit set: it asserts a property of **all** members and commits to **no cardinality and no
order**. Verified by reading and by the digit sweep.

**(b) The one word that looks like an ordinal is not one, and the distinction is the whole fix.**
*"a **later** entry"* is **temporal-by-authorship**, not **positional-in-output**. It is
**analytically safe**: an entry that quotes a sentence necessarily post-dates that sentence, so
no arrangement of the file can falsify it. The spec's rejected wording used *first*/*later* in
the **positional** sense — a claim about `grep -n`'s output order — which the file's own layout
already refutes. Same word family, **opposite rot profile**. The clause is not merely
number-free; it was chosen against the specific trap the spec fell into.

**(c) What could still falsify it:** a hit that is neither the sentence nor an entry quoting it —
i.e. the string `THE FILE EXISTS` appearing somewhere unrelated. **That is the correct residual
to keep.** It does **not** fire on the routine event that killed `(one hit)` and would have killed
the ordinal (somebody documents the sentence); it fires only on an event a reader would genuinely
want reported. A claim whose remaining falsifier is *interesting* is a well-built claim.

**(d) Two honest residuals, both NIT, neither worth a loop** — see §8 NIT-1 and NIT-2.

**Ruling: the clause is rot-proof against the event class that has broken every prior revision of
this line, and it contains no hidden ordinal or count.** This was the one thing that would have
justified a second loop; it does not.

---

## Findings

- **[NIT] `Tools/run_suite_bounded.ps1:182` — the universal is vacuously true if the target
  disappears.** If the sentence is ever deleted from `CONVENTIONS.md`, `every hit is …` is
  satisfied by an empty hit set while the recipe resolves to nothing; `(one hit)` would
  technically have flagged that (0 where 1 was promised). **Recorded for completeness, not for
  action:** a zero-hit grep is self-announcing — a reader cannot run the recipe, get nothing, and
  fail to notice — so the lost detection power is nil in practice, and it is not worth
  reintroducing a falsifiable number to buy it.
- **[NIT] `Tools/run_suite_bounded.ps1:182` — the pronoun *"that sentence"* depends on an
  antecedent ~7 lines above** (`:174`–`:176`, *"That law sentence is cited here the way everything
  else in this block is cited -- by SUBSTRING, never by address"*). This is **prose coupling, not
  numeric coupling**: it degrades gracefully (a reader can still run the grep and read both hits)
  and it cannot go silently false. Flagged only so a future "tidy" of the paragraph above does not
  orphan the reference — which is the same class of accident spec (3)'s exhibit fence guards.
- **[NIT] `Tools/run_suite_bounded.ps1:87-93` — the aside grew 4 lines → 7 where the spec framed
  the work as *"a SUBTRACTION, NOT A REWRITE."*** No survivor was lost and no address was written,
  so the framing sentence's actual purpose is met; the growth is additive justification prose that
  serves spec (2)(b)'s goal (explain why the paragraph has no numbers, so the next editor does not
  helpfully restore them). Side effect, measured and already declared by the programmer: the `+3`
  displaced the `:143` exhibit to `:146` and the launch/receipt sites to `:1690`/`:1687`, staling
  the spec's own `:143` reference. **Harmless precisely because no citation remains to be staled**
  — the row's thesis demonstrating itself on the row.
- **[NIT] `CONVENTIONS.md:4488` — not this row's business, flagged so a sweep does not destroy it
  (manager's file; nominating nothing, `SC-§50`).** The ledger entry I surfaced in §6 is itself
  built from line addresses (`:5122-5123` → `:5122` → `:5140` → `:5140`), **every one of which is
  now stale** — the sentence they track is at `:5181` today. That is **not rot and not a defect**:
  those addresses are a *record of where the citation stood at each named instant*, i.e. the
  **identical exhibit shape** the script protects at its own `:146` (*"and BOTH rotted"*). They are
  **evidence, not citations, and refreshing them would destroy what the ledger is about** —
  `TASK-1325` NIT-1 recurring one file over, where the script's own fence does not reach.

**BLOCKERS: 0.**

---

## 8. Re-derived structure — zero executable lines (not inherited)

**Delimiter census, occurrence-wise, over the whole file:**

```
<#  ->  1, 313, 372, 405, 520, 586, 649, 676, 797, 891, 951, 1032, 1057     = 13
#>  ->  238, 322, 382, 418, 525, 594, 653, 689, 808, 897, 964, 1035, 1065   = 13
```

**13 / 13.** They **strictly interleave** (`1<238<313<322<…<1057<1065`), so no block comment
nests or straddles another, and — this is the load-bearing part — **there is no `#>` anywhere
between `:2` and `:237`**. PowerShell block comments do not nest, so the `<#` at `:1` is closed by
the `#>` at `:238` and by no earlier token. **Header block = `:1`–`:238`, derived, not assumed.**

**First executable token after the header:** `:239` is blank; `:240` is
`[CmdletBinding(DefaultParameterSetName = 'Run')]`, followed by `param(` at `:241`. Nothing at or
below `:239` is touched by either hunk.

**Both edits are inside:** the aside at `:87`–`:93` and the recipe at `:181`–`:182`, and
`87 ≤ 93 < 181 ≤ 182 < 238`. ⇒ **ZERO EXECUTABLE LINES.**

**Census invariance across the edit** (rather than a bare post-edit count): the OLD text of both
hunks — quoted in the handoff **and** independently in the board spec — contains no `<#` and no
`#>`, and the NEW text at `:87`–`:93` / `:181`–`:182`, which I read, contains neither. A pair of
edits that adds and removes zero delimiters cannot move a delimiter census. **13/13 on both
sides** therefore follows structurally, not by trusting the pre-edit number.

**`SC-§116` guarded constructs — spot-checked live, and all intact:**

| construct | site | state |
|---|---|---|
| terminator | `:298` | `$script:Terminator = 'QUIT_EDITOR'` |
| suite lane separator | `:26`, `:376` | `;Quit` — semicolon, unchanged |
| command lane separator | `:32`, `:377` | `A,B,C,QUIT_EDITOR` — comma, unchanged |
| filter separator guard | `:325`–`:333` | throws on `,` `;` or a quote |
| `-ExecCmds` composition | `:507` | `('-ExecCmds="{0}"' -f $ExecValue)` — one quoted unit |
| self-test assertions | `:1251`, `:1256` | `'A,B,QUIT_EDITOR'` / caller `Quit` dropped |
| bounds (defaults) | `:253`–`:255` | `1500` / `420` / `180` |

The bounds are corroborated **against an independent source**: `CONVENTIONS.md:6275` states
*"defaults = the proven `1500` / `420` / `180`"*. Script and law agree. This is a **partial**
close on the residual below — the highest-value silent-substitution targets are demonstrably
unmutated — but it is a spot check over named constructs, not a proof over all 1,587 lines below
the header.

---

## 9. ACCEPTED AS DECLARED — named, per `SC-§71b` / spec (6)

I did not measure and do not vouch for:

1. **`Parser::ParseFile` = 0 errors**, and the handoff's control (header-closing `#>` removed →
   11 errors, instrument FIRES). **`TASK-1329`'s (1)** re-runs it — and note the corrected control
   shape: break the **final** `#>` at `:1065`, **never the header's own at `:238`**, because
   breaking `:238` re-pairs against the `<#` at `:313` and returns a **silent 0** (measured,
   `TASK-1318` CONTROL A). My census above gives `TASK-1329` both current addresses.
2. **The suite run** (401-by-signature / `LogAura` total / red count by name). **`TASK-1329`'s (2)**.
3. **The commit.** **`TASK-1329`'s (5)** — by pathspec, from the git root **one level up**
   (`SC-§102`), never `-a`, never a push.

### ⚠️ 9a. THE DECLARED RESIDUAL — mine to declare, `TASK-1329`'s to close

**An equal-line-count substitution BELOW the header would be invisible to every check in this
report.** I verified that the two *claimed* hunks are comment-only and inside the header; I
**cannot** verify they are the **only** hunks, because I have no `git diff`. The file grew `+4`
net (1821 → 1825 per the handoff), so a compensating edit elsewhere is not excluded by line count
either. §8's spot check narrows this materially but does not close it.

⇒ **`TASK-1329` closes it with two instruments and must say so explicitly (its spec (3) already
requires this):** the **`git diff`** (proves the hunk set is exactly two) and the **real suite
run** (proves the executable half still behaves). This is the identical hand-off `TASK-1323` and
`TASK-1325` each made one chain earlier — recorded as continuity, not as novelty.

---

## Notes for build-master (`TASK-1329`)

- **Verdict token is present** (top of file) for your `blocked-by` grep — **grep the token
  `Verdict:`, never a line address** (`SC-§126` cl. 10). It reads `PASS`.
- **Re-grep the final `#>` at your own instant before the parse control.** At mine it is
  **`:1065`** and the header's own close is **`:238`**. Break `:1065`. Breaking `:238` gives a
  silent green.
- **No C++ compile and no 5b** — stated in one line, not omitted: the diff is PowerShell comment
  text (zero executable lines, no module, no header, no `.uasset`) so there is nothing for UBT,
  and the row carries no runtime acceptance criterion, so `VER-§5` cl. 2 routes straight to 5c.
- **Leave the other lane held, not swept** (`TL-§5e` cl. 7a-v). `Source/…/SiegePlayerController.cpp`
  and `handoffs/TASK-1314-programmer.md` · `qa/TASK-1315-report.md` · `qa/TASK-1314-verify.md`
  belong to **`TASK-1314`** (host `TASK-1316`); `handoffs/TASK-1326-buildmaster.md` belongs to
  **`TASK-1326`**. They are **scheduled, not orphaned**. A `playtest-verifier` may amend
  `qa/TASK-1314-verify.md` inside your window — expected, not damage.
- **`CONVENTIONS.md` if dirty** rides along per `TL-§5e` cl. 7b; its content is not your business
  and none of it is mine — my §8 NIT-4 about `:4488` is an observation for the manager, **not** an
  edit request and **not** a fence on your commit.
- **Three flips are yours** (`SC-§103`): `TASK-1327` · `TASK-1328` · `TASK-1329` — wait: **two of
  those are already done by me** (see below). Yours is **your own row only**, plus whatever the
  board's (6) still names at your instant — **read the board back as STATE before flipping**
  (`SC-§104`), and ⛔ **`replace_all` is banned on `TASKBOARD.md`** (`SC-§127`).

---

## 10. Flips performed by this gate (spec: *"then flip TASK-1327 and this row"*)

`TASK-1327` could not flip itself — its `names` block fences it from `TASKBOARD.md`, so it was
`ready-for-qa` **in fact** and not on the board. Both flips are recorded in §11 of the board
edit log below, performed with `Edit` on task-ID-bearing anchors (never `replace_all`), each read
back out as state.

- `TASK-1327` → **`qa-passed`**
- `TASK-1328` → **`qa-passed`**
