# TASK-1327 — SUITE-RUNNER-ASIDE-DIGIT-DELETE — programmer handoff

- task: `TASK-1327` (marker `TASK-1327-SUITE-RUNNER-ASIDE-DIGIT-DELETE`)
- agent: gameplay-programmer
- date: 2026-09-19
- base: `f5697f3` (`main`, `TASK-1326`'s commit — the file fence is spent)
- gate: `TASK-1328` · host: `TASK-1329`
- status requested: `ready-for-qa` — **I cannot flip it myself** (see §8)

---

## 1. What changed

One file, comment-only, two edits, both inside the header comment block
(`<#` at `:1` … `#>` at `:238` post-edit).

### Edit (1) — spec clause (1): the clause-(a) dated aside. DELETED the digits.

**OLD (4 lines):**

```
        As this paragraph was last written (2026-09-19) they stood at :1680 (launch) and
        :1677 (receipt), both RE-GREPPED against the finished file, never offset from a
        diff: the revision before this one derived them by arithmetic (+69 where the
        header's growth was +70) and both were wrong by one.
```

**NEW (7 lines):**

```
        The two addresses this paragraph used to carry were DELETED, not refreshed
        (TASK-1327): they had been RE-GREPPED against the finished file, never offset
        from a diff -- an earlier revision derived them by arithmetic (+69 where the
        header's growth was +70) and both were wrong by one -- and they went stale
        ANYWAY, displaced by this block's own next edit. A citation that every later
        edit above it moves has no correct value to iterate toward; the two greps are
        the entire remedy (SC-126 cl. 7).
```

**I wrote no address.** `:1686` / `:1683` (the values true at the gate's instant) appear
nowhere. Neither does any other `:NNNN`. The digits I did write are `+69` / `+70`, which
spec (2)(c) blesses by name: they are *history about a past arithmetic error*, carry no
live address, and cannot rot.

**What survived, per spec (2), each checked by eye against the OLD text:**

| must survive | status |
|---|---|
| (a) `grep -n "Start-Process -FilePath"   -- the launch` | untouched, `:85` |
| (a) `grep -n "+ '.cmdline'"              -- the receipt` | untouched, `:86` |
| (b) *"FIND BOTH SITES BY GREP, NOT BY LINE NUMBER -- EDITING THIS BLOCK MOVES THEM, which is how the previous revision went stale"* | untouched, `:83-84` |
| (c) provenance: *"RE-GREPPED against the finished file, never offset from a diff"* | kept verbatim, re-tensed to `had been` because its subject (*"they stood at …"*) is gone |
| (c) the `+69/+70` cautionary tale | kept verbatim |

**(d) THE DATE STAMP — I DROPPED IT, and here is the one line of why:** the stamp
*"As this paragraph was last written (2026-09-19)"* existed to bound the validity of the
two numbers it introduced; with the numbers gone it bounds nothing, and a date stamp left
hovering over a number-free paragraph is an open invitation for the next editor to hang a
fresh number under it. I replaced its provenance role with `(TASK-1327)` — a task ID is a
stable handle that does not drift, where `2026-09-19` had, in the gate's own finding, zero
discriminating power (the aside and the edit that staled it carry the same date).

### Edit (1-bis) — spec clause (1-bis): the recipe's hit count. DELETED `(one hit)`.

**OLD (1 line):**

```
        grep -n "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md    (one hit)
```

**NEW (2 lines):**

```
        grep -n "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md
            -- every hit is that sentence, or a later entry quoting it
```

Survived verbatim, all three blockers: the `grep -n` invocation · the quoted substring
`"THE FILE EXISTS"` · the path `.claude/pipeline/CONVENTIONS.md`. The invocation line is
byte-identical to the old one minus its trailing `    (one hit)` — a **thirteen-character
subtraction** (nine for the parenthetical, four for the spacing that only existed to hold
it). No trailing whitespace was left behind.

**WHICH SHAPE I CHOSE, AND WHY (spec 1-bis requires this line):** I chose **subtraction
plus a non-numeric replacement clause**, not bare subtraction. Reason: `(one hit)` was
doing real work — it told a reader arriving at a multi-hit grep that they had not
mis-grepped. Deleting it silently leaves that reader to adjudicate between hits with no
guidance, which is the failure mode the whole block exists to prevent. The clause carries
**no digit, no address, and — deliberately — no ordinal** (see §3).

---

## 2. Acceptance evidence

**(A) Absence sweep, WITH THE POSITIVE CONTROL (`SC-§126` cl. 9(b-2)).**
Pattern `:[0-9]{3,4}` over the header block (`:1`–`:238`), *before* any absence claim:

```
21:         (EditorServer.cpp:5993); the process hangs until someone kills it.
146:    CONVENTIONS.md at :5966-5969 and then at :6172-6175, and BOTH rotted with nothing
```

⇒ the pattern **MATCHES KNOWN-PRESENT ADDRESSES** of exactly the form I am claiming
absent (`:5993`, `:5966`, `:5969`, `:6172`, `:6175`). The zero below is therefore a fact
about the file, not about my pattern.

```
grep -c ':1680'    <header block>  ->  0
grep -c ':1677'    <header block>  ->  0
grep -c '(one hit)' <header block> ->  0
grep -nEi '\((one|two|2|1)[^)]*hit' <header block>  ->  (no output)
```

**(B) My own run of the recipe, verbatim, at my instant** — reported as whatever it
actually is (`SC-§91`):

```
$ grep -n "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md
4488:  ... THE LEDGER FOR ONE SENTENCE - `THE FILE EXISTS` - IN ONE WORKING DAY ...
5181:  ... AND THE HALF THAT IS STILL TRUE ...
$ grep -c "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md
2
```

**2 hits.** Note both *addresses* differ from the ones the boarding finding recorded a few
hours ago (`:4466` / `:5152` → `:4488` / `:5181`) while the *substring* resolved
identically — which is the row's own thesis measured once more, for free.

**(C) `git diff --stat`** (git root one level up, `SC-§102`):

```
 GitClaudeUnrealTest/Tools/run_suite_bounded.ps1 | 14 +++++++++-----
 1 file changed, 9 insertions(+), 5 deletions(-)
```

One file. Hunks: `@@ -84,10 +84,13 @@` (changed lines land at **`:87`–`:93`**) and
`@@ -175,7 +178,8 @@` (changed lines land at **`:181`–`:182`**). Header block is
`:1`–`:238` ⇒ **every changed line is inside it. ZERO executable lines**, re-derived here
and not inherited: the file's first `<#` is `:1`, its matching `#>` is `:238`, and
`87 ≤ 93 < 181 ≤ 182 < 238`.

**(D) Delimiter census — 13/13 on both sides.** `grep -o` occurrence counts (not line
counts) over the whole file: `<#` = **13**, `#>` = **13**. Unchanged by this edit; neither
edit adds, removes or nests a delimiter.

**(E) `Parser::ParseFile` = 0 errors, with the instrument proven live.**

```
LIVE FILE  ParseFile errors = 0
CONTROL (header-closing #> removed) ParseFile errors = 11
  -> instrument FIRES: True
```

The control ran on a scratchpad copy (outside the repo, deleted afterwards); the live file
was never broken. A green parse on this file means nothing unless the parser can go red on
it, and it can.

**(F) `git status`** — the tree carries **pre-existing dirt from other lanes that was
already present at my start**, so the literal *"only that file"* cannot be true and I am
declaring the delta instead. At the git root:

```
 M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md              <- manager, pre-existing
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md                <- manager, pre-existing
 M GitClaudeUnrealTest/Source/.../SiegePlayerController.cpp         <- TASK-1314, pre-existing
 M GitClaudeUnrealTest/Tools/run_suite_bounded.ps1                  <- MINE
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1314-programmer.md   <- TASK-1314, pre-existing
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1326-buildmaster.md  <- TASK-1326, pre-existing
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1314-verify.md             <- TASK-1314, pre-existing
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1315-report.md             <- TASK-1314 lane, pre-existing
```

**Delta introduced by me = exactly `Tools/run_suite_bounded.ps1` + this handoff.** Every
other entry is byte-for-byte the session-start snapshot. `TL-§5e` cl. 7a-v: the
`SiegePlayerController.cpp` set is **SCHEDULED to `TASK-1314`, not orphaned** — host
`TASK-1329` must leave it held, not sweep it.

---

## 3. THE FINDING QA SHOULD SCRUTINISE FIRST — the spec's own suggested wording is already false

Spec (1-bis) offers, as its example of a permitted non-numeric clause:

> *"the first hit is the sentence; later hits are ledger entries quoting it"*

**I did not use it, because at my instant it is FALSE.** The two hits are `:4488` (the
manager's ledger entry *about* the sentence) and `:5181` (the sentence itself) — the
**ledger sorts FIRST**, because `grep -n` orders by position in the file and the ledger
section lives ~700 lines above the law section it describes. A reader following *"the
first hit is the sentence"* would land on the commentary and read it as the primary text.

⇒ **An ordinal is a number wearing a word.** `first` is as falsifiable as `one`, rots on
exactly the same event (somebody writes about the sentence), and rots *silently*, because
unlike a count it still points at a real line. My clause — *"every hit is that sentence,
or a later entry quoting it"* — is a **universal over the hit set**: it has no count, no
order, no position, and no address, so no future citation of that sentence can falsify it.
The only edit that could is one that puts the string somewhere unrelated, and that edit
would be the thing worth reporting anyway.

**This is the row's own lesson recurring inside the row's own remedy**, which is the
framing worth carrying forward: *documenting the rot created the second hit — the act of
recording a citation is itself an edit to what the citation matches.* I hit the same wall
one level deeper: **drafting the fix for a rotted count nearly wrote a rotted ordinal.**

---

## 4. `TASK-1324` was RIGHT, and the spec asks me to say so

The substring citation **did not rot into the wrong place.** Both hits are the right
sentence; a reader following the recipe lands correctly either way. `TASK-1324`'s remedy
(address → substring) worked exactly as designed. The only falsifiable component of that
line was the parenthetical count, and the count is the only component that broke. This row
is the completion of `TASK-1324`'s idea, not a correction of it.

---

## 5. Fences — each one declared, each one held

- **The `:143` exhibit: UNTOUCHED.** `CONVENTIONS.md at :5966-5969` and `:6172-6175` are
  **evidence, not citations** — the sentence carrying them says *"and BOTH rotted"*.
  Deleting or refreshing them would destroy what the sentence is about. They are
  byte-identical, and they appear in §2(A) only as my **positive control**, read and never
  written. ⚠️ **They now live at `:146`, not `:143`** — my own +3 lines displaced them.
  The spec's own reference to their address went stale by my edit, which is the third
  independent demonstration of this row's thesis inside this row.
- **Out of scope, mentioned and changed in no way (spec (4)):** the engine-source
  citations at `:21` (`EditorServer.cpp:5993`), `:293`, `:475-483`, `:1361`
  (`PluginManager.cpp`). They cite engine source this pipeline does not edit and they
  already name their functions (`FindCommandLinePlugins`, `FindTargetPlugins`), which is
  `DECK-§9` cl. 9's prescribed form. **Untouched.**
- **No executable line, no parameter default, no bound, no `-ExecCmds` composition, no
  comma-vs-semicolon separator, no `QUIT_EDITOR`, no `#19` orphan-census logic**
  (`SC-§116`) — `git diff` shows only comment text.
- **Not touched:** `Saved/**` · `TASKBOARD.md` · `CONVENTIONS.md` · `settings.local.json` ·
  the `.uproject` · any test file · any `.uasset` · `SiegePlayerController.cpp` ·
  `WBP_VictoryScreen.uasset` · every file of the `TASK-1314` / `TASK-1319` diffs.
- **Did not compile, did not run the suite, did not run the parser against anything but a
  read-only parse + a scratchpad control, did not stage, did not commit, did not push.**
  `TASK-1329` is the host.

---

## 6. Cosmetic facts for the gate

- No trailing whitespace introduced (checked: `grep -nE ' +$'` over the changed lines = no
  output).
- Changed-line widths: 82, 85, 84, 82, 84, 85, 41, 65, 70 — all under the header block's
  existing maximum of 100.
- File grew **1821 → 1825 lines** (net **+4**, measured both ways and they agree: +3 in
  the aside (4 lines → 7) and +1 at the recipe (1 line → 2); diff counts 9 in / 5 out).

---

## 7. What I would flag to the manager (nominating, never assigning — `SC-§50`)

Nothing new is owed by this row. One observation, offered without a suggested home:
**the aside at `:87` and the recipe at `:181` are now the only two self-describing
citations in the block, and both describe themselves in prose that a future editor may
"tidy."** If the chain wants insurance, the shape that would actually hold is a test that
greps this header for `:[0-9]{3,4}` outside the `:146` exhibit — a machine check rather
than another sentence asking to be trusted. That is a row, not a remark, and homing it is
the manager's.

---

## 8. Status flip — I could not do it

`TASK-1327`'s `names` block fences me from `TASKBOARD.md` (*"NEVER … `TASKBOARD.md`"*), so
**I cannot flip my own row to `ready-for-qa`.** The row is `ready-for-qa` **in fact** as of
this handoff. `TASK-1328`'s spec already assigns both flips to the gate
(*"then flip TASK-1327 and this row"*) — same shape as `TASK-1326`/`TASK-1325`. Routing is
the orchestrator's.

---

## 9. Files

- **Written:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\run_suite_bounded.ps1`
  (header comment block only) · this handoff.
- **Read:** `.claude/pipeline/TASKBOARD.md` (`TASK-1327` spec incl. clause (1-bis)) ·
  `.claude/pipeline/CONVENTIONS.md` (the `THE FILE EXISTS` recipe target, read only) ·
  `Tools/run_suite_bounded.ps1` header block.
