# TASK-1345 — VERIFY-LANE-ARGUMENT-COUPLING — gameplay-programmer handoff

**Status flipped by me, on both legs** (`SC-§134` cl. 7(a) — my `names:` carves out this row's own `status:`
line, so there is no relay to drop): `backlog` → `in-progress` at start, `in-progress` → `ready-for-qa` at
finish. Confirmed present on the board at `TASKBOARD.md:4699`.

**Subject:** `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`, the two `SC-§135` comment
blocks in `HandleMatchEnd`. **Comment-only. Zero behavioural bytes.** Not compiled, not committed —
`TASK-1346` gates, `TASK-1347` owes the 5a.

---

## 1. (0) MEASURE FIRST — FOUND vs PREDICTED

Every subject located by **quoted text** (`grep -nF`), never by line number.

| Subject | Needle | Predicted | **Found (pre-edit)** | Verdict |
|---|---|---|---|---|
| Comment block A | `⛔⭐ THE LITERAL BELOW IS A PUBLISHED INTERFACE` (2 tabs) | `:2322-2335` | **`:2322-2335`** | exact |
| Comment block B | same needle, 1 tab | `:2413-2426` | **`:2413-2426`** | exact |
| Format string A | `match ended — winner %s.` | `:2337` | **`:2337`** | exact |
| Format string B | same | `:2428` | **`:2428`** | exact |
| Argument line A | `Winner == ETeamId::Blue ? TEXT("Blue") : TEXT("Red")` | `:2338` | **`:2338`** | exact |
| Argument line B | same | `:2429` | **`:2429`** | exact |

**Zero divergence** from the manager's snapshot — the file had not moved since `4e388d6`.

⚠️ **One thing the census surfaced that the board did not name:** the argument-line needle returns a **third**
hit at **`:5259`** — `*GetNameSafe(this), FriendlyTeam == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));`. It is a
**different variable** (`FriendlyTeam`, not `Winner`) in a different function and is **not** part of this
coupling; no verify report quotes it. I name it so a later census does not read my "two sites" as a miscount.
I did not touch it.

Post-edit the four subject lines sit at **`:2349`/`:2350`** and **`:2452`/`:2453`** — moved by my added comment
lines only, which the projection proof below covers.

---

## 2. (1) CORRECTION ONE — THE ARGUMENT LINE IS NOW INSIDE THE PROTECTED SURFACE

### BEFORE (both sites, identical prose; site A shown, 2 tabs)

```
// ⛔⭐ THE LITERAL BELOW IS A PUBLISHED INTERFACE, NOT AN INTERNAL LOG (SC-§135). The
// playtest-verifier lane greps this exact line as the runtime evidence behind a VERIFIED
// verdict — qa/TASK-1314-verify.md, qa/TASK-1311-verify.md and qa/TASK-1068-verify.md all
// quote it — and ⛔ NO census over Source/, Tools/ or the suite can see that reader, because
// it is an agent following VER-§, not a caller (SC-§50's orphan, inverted).
// ⛔ KEEP IT BYTE-IDENTICAL WITH THE SUCCESS PATH'S COPY at the end of this function: the
// return just below makes the two sites mutually exclusive, so a scraper gets exactly one hit
// per match end either way — a property preserved ONLY while the two strings match, which is
// the whole basis on which TASK-1320 TRADE 2 upheld the duplication. ⛔ Do not de-duplicate.
// ⛔ REWORD EITHER COPY AND THE VERIFIER'S GREP RETURNS ZERO ⇒ the lane reports UNOBSERVABLE,
// ⛔ NOT VERIFY-FAILED. It will not have observed a failure; it will have failed to observe —
// so the breakage announces itself as "could not observe", which reads at a glance like an
// ENVIRONMENT problem rather than a code change. That is a fail-silent in the one lane whose
// entire job is to be the runtime witness (SC-§132), and it is why this comment exists.
```

*(Site B differs in exactly one sentence: `KEEP IT BYTE-IDENTICAL WITH THE DEGRADED PATH'S COPY inside the
!VictoryWidget block above: / that block's early return makes …`. Everything else was word-for-word the same.)*

### AFTER — site A (`:2322-2347`, 2 tabs), quoted whole

```
// ⛔⭐ THE TWO LINES BELOW ARE A PUBLISHED INTERFACE, NOT AN INTERNAL LOG (SC-§135). The
// playtest-verifier lane greps the emitted line as the runtime evidence behind a VERIFIED
// verdict — qa/TASK-1314-verify.md, qa/TASK-1311-verify.md and qa/TASK-1068-verify.md all
// quote it — and ⛔ NO census over Source/, Tools/ or the suite can see that reader, because
// it is an agent following VER-§, not a caller (SC-§50's orphan, inverted).
// ⛔ THE PROTECTED SURFACE IS THE WHOLE EMITTED LINE: FORMAT STRING *AND* ARGUMENTS. What the
// evidence quotes is the RENDERED text — `match ended — winner Red.` — and that "Red" is
// produced by the argument line immediately below, not by the literal above it. So
// TEXT("Blue") / TEXT("Red") are as published as the format string: renaming either token
// (to TEXT("RED"), to a localised or UEnum-derived name) breaks the verifier's grep ⛔
// IDENTICALLY to rewording the literal — and a reader who checks only the literal will find
// it untouched and wrongly conclude they are safe.
// ⛔ KEEP BOTH LINES BYTE-IDENTICAL WITH THE SUCCESS PATH'S COPY at the end of this function:
// the argument line is duplicated too and must stay in sync for the same scraper reason. The
// return just below makes the two sites mutually exclusive, so a scraper gets exactly one hit
// per match end either way — a property preserved ONLY while BOTH pairs match.
// ⛔ Do not de-duplicate WITHOUT A FRESH RULING: TASK-1320 TRADE 2 upheld the duplication as
// the CHEAPER OF TWO COSTS (both alternatives break the byte-identity TASK-1314 shipped on),
// and its WARN-2 sanctions a private LogMatchEnded(ETeamId) "at the point where re-indentation
// is no longer a cost" — a conditional remedy, ⛔ NOT a permanent prohibition.
// ⛔ REWORD EITHER COPY — LITERAL OR ARGUMENT — AND THE VERIFIER'S GREP RETURNS ZERO ⇒ the
// lane reports UNOBSERVABLE, ⛔ NOT VERIFY-FAILED. It will not have observed a failure; it will
// have failed to observe — so the breakage announces itself as "could not observe", which reads
// at a glance like an ENVIRONMENT problem rather than a code change. That is a fail-silent in
// the one lane whose entire job is to be the runtime witness (SC-§132), and it is why this
// comment exists.
```

### AFTER — site B (`:2425-2450`, 1 tab), quoted whole

```
// ⛔⭐ THE TWO LINES BELOW ARE A PUBLISHED INTERFACE, NOT AN INTERNAL LOG (SC-§135). The
// playtest-verifier lane greps the emitted line as the runtime evidence behind a VERIFIED
// verdict — qa/TASK-1314-verify.md, qa/TASK-1311-verify.md and qa/TASK-1068-verify.md all
// quote it — and ⛔ NO census over Source/, Tools/ or the suite can see that reader, because
// it is an agent following VER-§, not a caller (SC-§50's orphan, inverted).
// ⛔ THE PROTECTED SURFACE IS THE WHOLE EMITTED LINE: FORMAT STRING *AND* ARGUMENTS. What the
// evidence quotes is the RENDERED text — `match ended — winner Red.` — and that "Red" is
// produced by the argument line immediately below, not by the literal above it. So
// TEXT("Blue") / TEXT("Red") are as published as the format string: renaming either token
// (to TEXT("RED"), to a localised or UEnum-derived name) breaks the verifier's grep ⛔
// IDENTICALLY to rewording the literal — and a reader who checks only the literal will find
// it untouched and wrongly conclude they are safe.
// ⛔ KEEP BOTH LINES BYTE-IDENTICAL WITH THE DEGRADED PATH'S COPY inside the !VictoryWidget block
// above: the argument line is duplicated too and must stay in sync for the same scraper reason.
// That block's early return makes the two sites mutually exclusive, so a scraper gets exactly one
// hit per match end either way — a property preserved ONLY while BOTH pairs match.
// ⛔ Do not de-duplicate WITHOUT A FRESH RULING: TASK-1320 TRADE 2 upheld the duplication as
// the CHEAPER OF TWO COSTS (both alternatives break the byte-identity TASK-1314 shipped on),
// and its WARN-2 sanctions a private LogMatchEnded(ETeamId) "at the point where re-indentation
// is no longer a cost" — a conditional remedy, ⛔ NOT a permanent prohibition.
// ⛔ REWORD EITHER COPY — LITERAL OR ARGUMENT — AND THE VERIFIER'S GREP RETURNS ZERO ⇒ the
// lane reports UNOBSERVABLE, ⛔ NOT VERIFY-FAILED. It will not have observed a failure; it will
// have failed to observe — so the breakage announces itself as "could not observe", which reads
// at a glance like an ENVIRONMENT problem rather than a code change. That is a fail-silent in
// the one lane whose entire job is to be the runtime witness (SC-§132), and it is why this
// comment exists.
```

### The three mandated contents, each located in the shipped text

| | Mandated | Where it now lives (both sites) |
|---|---|---|
| **(a)** | rendered line is what the evidence quotes; team tokens are as published as the literal; renaming either breaks the grep **identically** | para 2, sentences 1-3: *"What the evidence quotes is the RENDERED text — `match ended — winner Red.` — and that "Red" is produced by the argument line immediately below"* … *"breaks the verifier's grep ⛔ IDENTICALLY to rewording the literal"* |
| **(b)** | the **argument line is also duplicated** and must also stay byte-identical, same scraper reason | para 3, sentence 1: *"KEEP **BOTH LINES** BYTE-IDENTICAL … the argument line is duplicated too and must stay in sync for the same scraper reason"*, closing on *"a property preserved ONLY while **BOTH pairs** match"* |
| **(c)** | failure mode unchanged: **`UNOBSERVABLE`, NOT `VERIFY-FAILED`** | para 5, widened trigger *"REWORD EITHER COPY — **LITERAL OR ARGUMENT** —"*, unchanged consequence |

**Headline widened too:** `THE LITERAL BELOW IS A PUBLISHED INTERFACE` → **`THE TWO LINES BELOW ARE A PUBLISHED
INTERFACE`**, and `greps this exact line` → `greps the emitted line`. Those two edits matter as much as the new
paragraph: a reader who stops at the first sentence previously learned the wrong scope.

**Report paths retained:** all three (`qa/TASK-1314-verify.md`, `qa/TASK-1311-verify.md`,
`qa/TASK-1068-verify.md`) survive verbatim at both sites — the board asked for *at least one*.

### 🚨 (c) AUDITED THE RIGHT WAY ROUND AT BOTH SITES

This is the WARN the gate is posted to catch, so I assert it as state, not as a tally:

```
$ grep -c "lane reports UNOBSERVABLE" SiegePlayerController.cpp   → 2
$ grep -c "NOT VERIFY-FAILED"                                     → 2
$ grep -c "will not have observed a failure; it will"             → 2
$ grep -c "have failed to observe"                                → 2
```

Order at **both** sites, verbatim: *"the lane reports **UNOBSERVABLE**, ⛔ **NOT VERIFY-FAILED**. It will not
have observed a failure; it will have failed to observe — so the breakage announces itself as "could not
observe", which reads at a glance like an **ENVIRONMENT** problem rather than a code change."*
⇒ `UNOBSERVABLE` is the **outcome**, `VERIFY-FAILED` is the **negated** one, and the *failed-to-observe* gloss
follows the outcome. **Not inverted at either site.**

---

## 3. (2) CORRECTION TWO — THE MODALITY, RE-MEASURED AT SOURCE

I read `qa/TASK-1320-report.md:248-267` **whole**, both TRADE sections, and did not grep a line out of either.
The manager's retraction is **confirmed correct at source**:

- **TRADE 1 — the now-always-true `if (VictoryWidget)` guard.** Ends:
  > *"⛔ **NOT a BLOCKER, NOT a follow-up row, and I do not want it "cleaned up" on a later touch either.** The rationale is already in-code at `:2261-2301`."*
  ⇒ the *"later touch"* sentence is **TRADE 1's**, and its subject is **the guard**.

- **TRADE 2 — the duplicated `match ended — winner %s.` log.** Ends:
  > *"⇒ ⛔ **UPHELD as the cheaper cost, and no row.** See WARN-2 for the one correction I am putting on the record about how the "no consumer" measurement was characterised."*
  ⇒ **conditional, cost-based, and it explicitly forwards to WARN-2** — no permanent prohibition anywhere in it.

- **WARN-2**, quoted exactly as I cite it in code (`qa/TASK-1320-report.md:334-335`):
  > *"Named remedy **for a future touch of this function only, explicitly NOT now and explicitly NOT a row**: extract the winner record into one private `LogMatchEnded(ETeamId)` helper **at the point where re-indentation is no longer a cost**."*

**Shipped rewrite:** *"⛔ Do not de-duplicate **WITHOUT A FRESH RULING**: TASK-1320 TRADE 2 upheld the
duplication as the CHEAPER OF TWO COSTS (both alternatives break the byte-identity TASK-1314 shipped on), and
its WARN-2 sanctions a private LogMatchEnded(ETeamId) "at the point where re-indentation is no longer a cost" —
a conditional remedy, ⛔ NOT a permanent prohibition."*

**Not wider, and not weakened into an invitation.** The prohibition stays **operative today** (`Do not
de-duplicate`); only its *modality* is corrected from absolute to conditional, and the condition is the source's
own (`without a fresh ruling` mirrors WARN-2's *"explicitly NOT now and explicitly NOT a row"*).

⛔ **The ruling itself is untouched: the duplicate STAYS and TRADE 1's `if (VictoryWidget)` guard STAYS.** I
de-duplicated nothing and did not re-open the guard. **The defect corrected here is the manager's own
mis-attribution in the `TASK-1341` spec — `TASK-1341` carried its instruction faithfully** and nothing in this
row is a mark against it.

---

## 4. (3)/(5) THE COMMENT-ONLY PROOF — DIALECT NAMED, COMPLEMENT CONTROL RUN

### 4.1 ⭐ The strongest form: the **whole file's non-comment projection**

This catches a behavioural byte **anywhere in the file**, not merely inside my diff:

```
$ grep -v -E "^[[:space:]]*//" <HEAD version>  > proj_head.txt
$ grep -v -E "^[[:space:]]*//" <worktree>      > proj_work.txt

HEAD projection sha256 : ba752038be496ca539ed6c5195ae4a5d9e8f119e77c1391197e419cbea3f95d2
WORK projection sha256 : ba752038be496ca539ed6c5195ae4a5d9e8f119e77c1391197e419cbea3f95d2
HEAD projection lines  : 4667
WORK projection lines  : 4667
diff proj_head proj_work : (empty) → CLEAN
```

**Dialect note (`SC-§137` cl. 4):** the class is `[[:space:]]`, a **POSIX bracket class that `-E` does expand** —
deliberately *not* `[ \t]`, which under `-E` is the set *{space, backslash, `t`}* and would have silently failed
to strip this file's **tab**-indented comments.

Whole-file line count 7588 → 7612 (**+24**), every one a comment.

### 4.2 `git diff --numstat` (`SC-§128` — not `--stat`)

```
45      21      GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
```

### 4.3 `SC-§137` cl. 3 COMPLEMENT CONTROL — **dialect = `grep -P`**

```
TOTAL ADDED (grep -P '^\+(?!\+\+)')          = 45
added COMMENT     (grep -cP  '^\+[ \t]*//')  = 45
added NON-COMMENT (grep -cvP '^\+[ \t]*//')  =  0
SUM = 45  vs  TOTAL = 45                     → SUMS ✅
```

Same control on the **removed** side, because a deleted executable line is just as behavioural as an added one:

```
TOTAL REMOVED (grep -P '^-(?!--)')           = 21
removed COMMENT     (grep -cP  '^-[ \t]*//') = 21
removed NON-COMMENT (grep -cvP '^-[ \t]*//') =  0
SUM = 21  vs  TOTAL = 21                     → SUMS ✅
```

**The broken POSIX dialect reproduced live, for the record (`SC-§137` cl. 1):**
`grep -cvE '^\+[ \t]*//'` over the same 45 added lines returns **45** — i.e. *"100% code"*, a BLOCKER-shaped
false answer against a diff with **zero** code lines. With `-P` it is **0**. `SC-§137`'s first measured event
reproduces exactly on this diff.

### 4.4 `SC-§137` cl. 3(a)/(b) double control on the `-F` needles

The log-line needle crosses an **em dash** (3 UTF-8 bytes), so every probe used `-F`, never `.`:

- **(a)** needle `match ended — winner %s.` returns **2** where it is known present ⇒ not a dead pattern.
- **(b)** a **different** needle, `HandleMatchEnd`, returns **13** in the **same** corpus ⇒ the corpus is readable.

### 4.5 🚨 A dialect defect I hit **in my own instrument**, caught and corrected

While auditing the board edit I ran `grep -P '^[+-](?![+-])' | grep -cP '^[+-]- status:'` and got **0** — which
would have read as *"I touched no status line"*. **It was false.** For the diff line `+- status: …` the second
character **is** `-`, so my own `(?![+-])` header guard ate every added board line. Split per sign
(`^\+(?!\+\+)` and `^-(?!--)`) the true answer is **7 added / 3 removed** status lines — all of them the
manager's own uncommitted boarding and other lanes' pre-existing dirt, **none mine** (see §5). Publishing this
because `SC-§137` cl. 2's point is that *the failure mode of a wrong dialect is a confident number*, and mine
was a confident zero on the very row that exists to teach that.

### 4.6 The four subject lines — byte-identity **shown**, not asserted

Leading tabs differ by nesting depth (site A is 3 tabs, site B 2) exactly as at `HEAD`, and exactly as
`TASK-1342`'s `^\t+TEXT\(…\)$` pattern allows; identity is over the content:

```
$ grep -F 'match ended — winner %s.'                            f | sed 's/^\t*//' | sort -u | wc -l  → 1
$ grep -F 'Winner == ETeamId::Blue ? TEXT("Blue") : TEXT("Red")' f | sed 's/^\t*//' | sort -u | wc -l  → 1
      (same two commands against the HEAD blob)                                                        → 1, 1
$ diff <fmt canonical, worktree> <fmt canonical, HEAD>  → identical ✅
$ diff <arg canonical, worktree> <arg canonical, HEAD>  → identical ✅
```

The two canonical lines, shown character for character:

```
TEXT("ASiegePlayerController '%s': match ended — winner %s."),
*GetNameSafe(this), Winner == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));
```

⇒ **both format strings byte-identical to each other AND to `HEAD`; both argument lines byte-identical to each
other AND to `HEAD`.** I documented them; I did not normalise them.

Cross-check with indentation included, all four lines together: worktree md5 = `HEAD` md5 = `05af085f…`.

### 4.7 The four `FInputMode*` posture lines

| Line | `HEAD` | worktree | text |
|---|---|---|---|
| degraded arm | `:2307` | `:2307` | `FInputModeGameAndUI DegradedInputMode;` |
| success arm | `:2356` | `:2368` | `FInputModeUIOnly InputMode;` |
| (3rd) | `:6440` | `:6464` | `FInputModeGameAndUI InputMode;` |

Text byte-identical; only line numbers shifted, which the **identical projection hash in §4.1 already proves is
comment displacement**. No posture changed on either arm. No `IA_` / `IMC_` / `BindAction` / `UEnhancedInput*`
token added or removed; `EKeys::Escape` untouched (0 occurrences added).

---

## 5. (4) WHAT THIS ROW DID NOT DO — SKIPPED, NOT FORGOTTEN

All five refused under the standing manager ruling in the section header above this row, not by my choice:

1. ⛔ did **not** pin the literal in a test — 2. ⛔ did **not** build a registry of verify-lane strings —
3. ⛔ did **not** change the verifier — 4. ⛔ did **not** touch any of the three `*-verify.md` (`VER-§1`) —
5. ⛔ did **not** re-open TRADE 1's `if (VictoryWidget)` guard.

`SC-§135` cl. 5 has **not** fired and I did not fire it: this is the **same** literal with a **wider** surface,
one emitted line and one consumer lane — not a second distinct literal.

**Also not done, and declared:** ⛔ no compile, ⛔ no suite run, ⛔ no commit, ⛔ no editor action.
⛔ **No suite-count change is claimed** — a comment-only C++ diff changes no test.
⛔ I did not touch `Tools/run_suite_bounded.ps1` (`TASK-1338`/`1344`'s live lane), `CONVENTIONS.md`, `Saved/**`,
the `.uproject`, or any `.uasset`.

### `git status` at hand-off (git root **one level up**, `SC-§102`)

```
 M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md                          ← pre-existing (manager)
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md                            ← MINE: 1 line only (+ pre-existing)
 M GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1343-buildmaster.md       ← pre-existing
 M GitClaudeUnrealTest/Source/.../SiegePlayerController.cpp                     ← MINE
 M GitClaudeUnrealTest/Tools/run_suite_bounded.ps1                              ← pre-existing (TASK-1338)
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1338-programmer.md        ← pre-existing
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1339-report.md                  ← pre-existing
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1344-report.md                  ← ⚠️ APPEARED DURING my run
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1345-programmer.md        ← MINE (this file)
```

**Exactly three paths are mine.** Five others were already dirty at my dispatch snapshot; a **sixth**,
`qa/TASK-1344-report.md`, **appeared while I worked** — that is `TASK-1344`, the parallel re-gate over
`Tools/run_suite_bounded.ps1`, landing its report, and the board already names it for `TASK-1340`'s pathspec.
I record the delta rather than the dispatch-time snapshot because a stale census is the species this wave
exists to punish; **re-measure at your own instant (`SC-§104`), this list is `EXPECTED, NOT EXHAUSTIVE`.**

⛔ `TASK-1347`: `SiegePlayerController.cpp` + this handoff are **yours**; everything else above is
`TASK-1338`/`1344`'s lane, behind its own gate and its own host — name it, stage none of it.

### The board edit, proven one line

Anchor collision counts **measured at my instant** (the board quotes 51/157/37 from 2026-09-20 — I re-measured):

```
bare '- status: backlog'                              = 157 collisions
bare '- status:'                                      = 1356 collisions
'BOARDED AND ⛔ READY TO DISPATCH (⭐ `TASK-1345`)'     =   1  ← the discriminator, load-bearing
```

`Edit` only, never `replace_all` (`SC-§120` / `SC-§127`). Reverting my exact string into a temp copy and
diffing yields **`4699c4699`** — **one changed line, nothing else in the file.** The `TASK-1345` row does not
exist at `HEAD` at all (the manager's boarding is itself uncommitted), so my flip appears in
`git diff` as part of an added block rather than a modify; the revert-diff above is the instrument that isolates
**my** byte from the manager's.

---

## 6. ⚠️ PROPORTIONALITY — THE TEST `TASK-1342`'s GATE LEFT ME, AND I MET IT

> *a rationale block earns its length only when each paragraph is a content the board named.*

Each block is **five paragraphs, five board-named contents, no sixth**:

| ¶ | Lines | Content | Named by |
|---|---|---|---|
| 1 | 5 | published interface + the lane + three report paths + no-census-can-see-it | `SC-§135` cl. 4(b); spec (1) *"name at least one report by path"* |
| 2 | 7 | **(a)** rendered line ⇒ argument tokens are published; renaming breaks the grep identically | spec (1)(a) — **the row's reason for existing** |
| 3 | 4 | **(b)** the argument line is duplicated too; both pairs must stay in sync | spec (1)(b); `SC-§135` cl. 4(c) |
| 4 | 4 | **(2)** the narrowed, conditional modality | spec (2) |
| 5 | 6 | **(c)** `UNOBSERVABLE`, not `VERIFY-FAILED` | spec (1)(c); `SC-§135` cl. 3; `SC-§132` |

14 → 26 lines per block (**+12**), and the growth sits almost entirely in ¶2 + ¶4, which are precisely the two
contents the board boarded this row to add. **Nothing was cut** — ¶1, ¶3 and ¶5 keep every correct element of
the shipped text.

**One licensed thing I deliberately did NOT add.** Spec (2) offers: *"This is the same species as the false
universal that cost three gates … (`SC-§136`). Say so in the comment **if it helps the next reader**."*
Permissive, not mandatory. A sixth paragraph naming a law about a **different file's** history would be the
first paragraph in the block that is **not** a content the board named — the exact thing the proportionality
test is guarding. The correction itself already carries its own reason (*"a conditional remedy, NOT a permanent
prohibition"*). ⇒ **recorded here instead of in code.** If `TASK-1346` would rather see it in-line, it is a
one-line add and I will not argue.

---

## 7. WHAT QA SHOULD SCRUTINISE

1. **⭐ The widened surface is the whole point — check ¶2 and ¶3 at BOTH sites**, and that the argument line is
   named *as an interface*, not merely mentioned. A site where only ¶2 landed still has the `(b)` hole.
2. **⭐ (c) not inverted.** `UNOBSERVABLE` must be the outcome and `VERIFY-FAILED` the negated one, at both
   sites. §2 above audits it four ways; re-measure, do not take my count (`SC-§104`).
3. **⭐ Your byte-identity instrument must be WIDENED to the argument line** — `TASK-1342`'s `$`-anchored
   `^\t+TEXT\("…match ended — winner %s\."\),$` **passes an argument-line divergence silently**. §4.6 gives a
   pattern that covers both; publish whichever you use.
4. **Dialect.** Any pattern you write over this file crosses an **em dash** (use `-F`) and **tab** indentation
   (use `-P` or `[[:space:]]`, never `-E` with `[ \t]`). §4.5 is a live example of that trap firing on me.
5. **The third `ETeamId::Blue ? TEXT("Blue") : TEXT("Red")` at `:5259`** (`FriendlyTeam`, different function) —
   expect **3** hits on that needle and **2** on the coupled pair. Not a miscount, not in scope, not touched.
6. **The modality must not read as an invitation.** Confirm `Do not de-duplicate` is still the operative verb
   and that the condition attaches to it rather than replacing it.
7. **The duplicate and the `if (VictoryWidget)` guard are both still present and unchanged** — both upheld.
8. The projection hash in §4.1 is re-runnable in two commands and is the single check that would catch a
   behavioural byte I introduced anywhere, including outside my diff. Prefer it over reading the diff.

**Downstream:** `TASK-1346` gates → `TASK-1347` owes the **5a compile** (`.cpp`, so mandatory even comment-only;
`Result: Succeeded` parsed from the **log**, never the exit code) → relaunch → **5c**. **No 5b** — this row
carries **no runtime acceptance criterion**, declared, not forgotten, and that is **not** an `UNOBSERVABLE`.
