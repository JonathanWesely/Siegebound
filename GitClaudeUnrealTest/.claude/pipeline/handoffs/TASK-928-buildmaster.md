# TASK-928 — [DOC-HOST] the pipeline's own gate records enter the repository — build-master handoff

**Assignee:** build-master · **Status → `done`** · **Commit `950d8c5`** · 2026-09-03 · ⛔ **NOT pushed** (`main` 16 ahead of `origin/main`)

**Law:** `TL-§5e` cl. 7 (this row *is* cl. 7) · `TL-§5e` cl. 5 · `§25b` cl. R + cl. S · `SC-§29` · `SC-§40` cl. 1.

---

## 1. THE COMMIT

```
950d8c5  TASK-928: the pipeline's own gate records enter the repository -
         66 QA verdicts and handoffs for a batch whose tracked backlog had
         stopped at TASK-805
```

**66 files changed, 15860 insertions(+), 0 deletions(−).** All 66 status `A` (new file).
**48 handoffs + 18 QA verdicts.** Zero `Source/`, zero `Content/`, zero `Tools/`, zero binaries.
No compile, no suite, no editor, no MCP — **expected suite delta `0` by construction, and nothing was run that could move it.**

✅ **The commit contents are byte-for-byte the resolved set**: `git show --name-only HEAD` sorted equals the printed 66-path list, verified by `diff` — nothing extra, nothing missing.

---

## 2. ⭐ THE PATHSPEC — DERIVED, AND HOW

`TL-§5e` cl. 7 grants this row the **one authorized exception** to `§25b` cl. R's hand-named pathspec. I used it, and here is the derivation, not just its result.

```
git status --porcelain -z -- .claude/pipeline/handoffs .claude/pipeline/qa
```

⚠️ **`-z` (NUL-delimited) deliberately, and read with `IFS= read -r`.** `TASK-925` recorded that a bash `read -r` strips the leading status space, so `${line:3}` eats a character of every ` M ` path and **manufactures phantom orphans**. I took its correction as a precondition rather than rediscovering it.

**Raw candidates: 66.** Exclusion filters removed **0** of them (see §5 — every exclusion class turned out to be *structurally* empty at my instant, which is a finding in itself). **Resolved: 66** — printed in full, with its count, **before** staging.

| status | count |
|---|---|
| `??` untracked | **66** |
| ` M` modified | **0** |

⛔ **I did not work from any briefed number.** The class was measured at **61 → 60 → 65 → 67** at four points tonight. It resolved at **66** at mine. ⭐ **All five numbers are correct — each for its own instant.** The design is vindicated a *fifth* time: my figure differs from the `67` measured by `TASK-925` less than an hour earlier, because `TASK-925`'s 67 counted `CONVENTIONS.md` and `TASKBOARD.md` (modified, and **not mine** — see §5(e)) while mine does not, and no new document landed in between.

**ID span:** `TASK-735` · `TASK-807`–`TASK-926` · plus one undated `LADDER-REGRESSION-diagnosis.md`.
**mtime span:** 38 files 2026-09-02, 28 files 2026-09-03 — **one continuous batch, no strays from older sittings.**

---

## 3. ⛔⛔ THE SECRETS SWEEP — CLEAN, AND WHY THE HITS ARE NOT HITS

This was the bulk first-time publication of **66 agent transcripts**, and handoffs quote transcripts verbatim. Swept **before** staging.

| pattern | raw | verdict |
|---|---|---|
| `hf_` | 1 | ⚠️ **`qa/TASK-865.md:89`** — quotes the tool's own redaction regex `hf_[A-Za-z0-9]{15,}`. **A pattern, not a token.** |
| `hf_[A-Za-z0-9]{8,}` (real shape) | **0** | ✅ |
| `msy_` · `sk-`* · `eyJ`* · `xoxb-` · `xoxp-` · `AKIA` · `ghp_` · `Bearer ` · `PRIVATE KEY` | 0 real | ✅ |
| `SUPABASE` · `anon key` · `service_role` · `password` | **0** | ✅ |
| `HF_TOKEN` · `MESHY_TOKEN` · `api_key` · `secret` | 40 | ✅ **all variable NAMES**, in prose *asserting* the env-only discipline |
| email addresses | **0** | ✅ |
| bare 32+ char alnum runs | 30 | ✅ sha256 asset digests, git hashes, one UE GUID, long CamelCase test names |

\* `sk-` matched inside **`di**sk-**vs-index`**; `eyJ` matched inside **`WasInputK**eyJ**ustPressed`**. Both are substring artefacts of the pattern, and I checked every one rather than counting them.

⭐ **The best line in the sweep is `qa/TASK-865.md:93`**, which reviews the one token-shaped literal in the whole tool suite and records it as `"not-a-real-token"` — a stub return. ⇒ **the corpus I published is largely *about* not leaking secrets.**

⛔ **No redaction was performed and none was needed.** Had there been a hit I would have stopped: editing another agent's evidence file is **authoring** it, which destroys the property that makes it evidence.

---

## 4. ⛔ COMPLETENESS — PER FILE, BECAUSE A TRUNCATED VERDICT READS AS A VERDICT

`TASK-926`'s precedent (it decoded every PNG before committing it) applied to text.

- ✅ **Zero-byte files: 0 / 66.**
- ✅ **Unterminated last line: 0 / 66** — last byte is `0x0a` on all 66, measured with `tail -c 1 | od`.
- **Size range 791 B → 67 072 B.** The 791-byte minimum is **`qa/TASK-853.md`, and it is complete by design**: a deliberate pointer stub whose body says the real verdict lives in `qa/TASK-854.md`, closing with a dated `SC-§15` declared-departure note. **Short, not truncated** — I read its tail rather than trusting its size.
- ✅ **All 18 QA reports carry an explicit verdict line** (17 PASS · 1 with a preserved loop-1 FAIL, see §6).
- ✅ **`git check-attr filter`: `unspecified` on 66/66.** No path returned `lfs` ⇒ no non-text file smuggled into a text list. ⭐ This mattered: `*.png` is an **unanchored** repo-root rule and **does** reach `.claude/` — confirmed here, since `.claude/pipeline/handoffs/TASK-105-*.png` appear in the repo's LFS set.

---

## 5. THE EXCLUSIONS — EVERY ONE NAMED

| # | excluded | count | reason |
|---|---|---|---|
| **(a)** | `handoffs/TASK-928-buildmaster.md` | 1 | ⭐ **MY OWN — the standing tail.** See §7. |
| **(b)** | documents of live tasks | **0** | Nothing was running but me (dispatcher restated the live set). Highest IDs present are handoff `926` / qa `924`; the concurrently-boarded `TASK-931`–`939` have **produced no documents yet**, so there was nothing to hold back. |
| **(c)** | files in another pending pathspec | **0** | `TASK-919` and `TASK-925` **spent** their pathspecs (`239ca77`, `d101b1e`). The only pipeline `.md` any batch commit took was `handoffs/TASK-832-artist.md` (by `8e74596`) — already tracked, so it never entered my set. **No double claim.** |
| **(d)** | non-`.md` under the two dirs | **0** | ⚠️ **The class was empty** — no untracked `.csv`/`.json`/render sits under `handoffs/` or `qa/`. Nothing to name-and-leave. |
| **(e)** | `CONVENTIONS.md` + `TASKBOARD.md` | 2 | ⛔ **EXCLUDED, DELIBERATELY — and I was asked to decide, so here is the decision and its reason.** Both are ` M` modified, both live under `.claude/pipeline/`, and both were therefore *plausible* members. They are out for **three independent reasons, any one sufficient**: (1) `TL-§5e` cl. 7's SCOPE bullet limits the doc-host to `.md` **under `handoffs/` and `qa/` only** — these are one level up; (2) the row's item (3)(e) excludes them by name — the manager writes `TASKBOARD.md` concurrently and it has taken **five** concurrent-writer conflicts tonight, so staging it can capture a half-write; (3) Jonathan historically sweeps both into his own commits. ⭐ **Structurally they were never at risk**: my derivation pathspec names the two directories, which cannot reach a file above them. The exclusion is belt *and* braces. |
| — | `Content/FogArea/` | 27 | ⛔ **`TASK-927`'s, awaiting Jonathan.** Not mine, not touched. |

---

## 6. ⛔ RECORDS COMMITTED AS THEY STAND — I AUTHORED NOTHING

Item (9): *you commit records, you do not author them.* **Zero files were edited, renamed, redacted or tidied.** Three carry appended corrections with the original text intact, and I want the next reader to know these are **deliberate**, not damage:

- **`qa/TASK-889.md`** — a complete **two-loop** record: `Verdict: FAIL` at line 3 (loop 1), `Verdict: ✅ PASS` at line 269 (loop 2), and a manager annotation at line 491 that states in its own heading that **no verdict text above it was edited**.
  ⚠️ **A TRAP FOR A FUTURE GREP, FLAGGED DELIBERATELY:** line 260 reads **"⛔ DO NOT COMMIT THIS FILE YET — the verdict is FAIL."** That is **loop-1 text, superseded** by line 458's *"`TASK-919` may proceed on this verdict"* and the board's `DONE … LOOP 2/3 verdict PASS`. ⇒ **committing it was correct**; anyone grepping for that sentence must read on to line 269.
- **`handoffs/TASK-874-programmer.md`** §11.3/§11.7 — appended corrections, original intact.
- **`TASK-918`'s board row** — annotated a third time rather than rewritten (not my file; noted for continuity).

⭐ **The error is worth more than the correction.** A wrong report stays wrong and earns an annotation row.

---

## 7. ⛔⛔ MY OWN ORPHAN, NAMED EXPLICITLY

> **`.claude/pipeline/handoffs/TASK-928-buildmaster.md` — THIS FILE — is NOT in commit `950d8c5`, and that is correct.**

It records `950d8c5`. A file cannot contain the hash of the commit that contains it, and **amending only moves the hash again** (`TL-§5e` cl. 7). It was written **after** the commit, exactly as the row directs.

⛔ **DO NOT rediscover this as a defect.** ⭐ **The constructed tail is bounded at ONE**: this commit closed the other 66, and the next doc-host takes this one file. A row that tries to include itself is **wrong, not thorough**.

### ⚠️⭐⭐ AND A **SECOND** ORPHAN ARRIVED **WHILE I WAS RUNNING** — A DIFFERENT CLASS, NAMED SO IT IS NOT CONFUSED WITH MINE

> **`.claude/pipeline/qa/TASK-934.md` — 28 806 bytes, mtime `2026-09-03 17:04:11` — did NOT exist at my derivation instant and DOES exist now. It is NOT in `950d8c5`, and excluding it was correct.**

⛔ **Why it is out:** item (3)(b) forbids taking the document of a task **live at my instant**, and this one was **being written while I staged** — the exact half-file hazard the clause exists for. It arrived after the resolved list was printed, and the row forbids amending.

⛔⛔ **THREE THINGS ABOUT IT THAT ARE THE MANAGER'S, NOT MINE:**

1. ⛔ **The board says this file will never exist.** `TASKBOARD.md`'s `TASK-934` `names:` line reads *"~~report `qa/TASK-934.md`~~ ⛔ **NEITHER WILL EXIST**"*, and `TASK-937` was explicitly told **not to look for it**. ⇒ **a row that was struck ran anyway.** `TASK-937` is now the row most likely to be wrong about its own inputs.
2. ⛔⛔ **Its verdict is `PASS`, and its closing sentence reports a fresh instance of THIS ROW'S DEFECT:** *"the hard gate is satisfied for this hunk **as of this report — not as of the commit**. `d101b1e` **shipped it ungated**."* ⇒ **`d101b1e` is a ninth commit in the same condition the other eight were in** — and unlike them, its gate is **late**, not merely unreadable.
3. ✅ **The document itself is complete** — non-zero, terminated, explicit `Verdict: PASS — 0 BLOCKERS · 3 WARN · 4 NIT`. **Nothing is wrong with the file.** It simply belongs to the next doc-host.

⭐ **THE STANDING TAIL IS THEREFORE TWO FILES, OF TWO DIFFERENT KINDS:** this handoff (**constructed**, `TL-§5e` cl. 7, unavoidable) and `qa/TASK-934.md` (**arrived**, ordinary lateness). ⚖️ ***Cl. 7 bounds the orphans a doc-host MINTS at one. It does not bound the ones that LAND while it runs*** — and tonight, for the first time, both happened in one row. **Offered as a cl. 7 refinement.**

---

## 8. `§25b` cl. R — FULL RECONCILIATION

**Pre-commit — 95 dirty paths (`-uall`), every one classified, `0` in neither:**

| class | count |
|---|---|
| mine (`TASK-928`) | 66 |
| `TASK-927` `Content/FogArea/` | 27 |
| `CONVENTIONS.md` + `TASKBOARD.md` | 2 |
| ⛔ **in neither** | **0** |

**Post-commit — 29 dirty: 27 FogArea + 2 docs. `0` pipeline documents left. `0` in neither.**
✅ **Index clean on arrival (`0` staged) and clean after commit.** ⛔ No `git reset` at any point; staging was an explicit 66-path `git add`, never a directory glob — a glob would have swept anything created between derivation and staging.

⚠️ **One expectation the row set did NOT materialise:** item (6) warned I would hit *"the live lane's dirty `Source/` files."* **There were none** — the lane had closed and `239ca77`/`d101b1e` had taken them. Recorded so the next host does not treat their absence as a missed check.

---

## 9. `§25b` cl. S — SWEPT ANYWAY, AND THE DETECTOR WAS PROVEN

⛔ **This commit contains ZERO binaries** (measured: `git check-attr filter` returns `lfs` for **0** of my 66 paths) ⇒ the "changed binary" set is empty. **I swept the entire tracked set anyway**, to `TASK-919`/`925`/`926`'s standard, because **cl. S's fires are intermittent, not continuous — a clean index is a result, not a repeal.**

- **4993 LFS-tracked binaries** (of 6127 tracked files) — the identical count `TASK-925` measured; `TASK-919` had 4971.
- **Compared by DIGEST, never by size**: the sha256 recorded **inside the index's LFS pointer blob** vs the **sha256 of the worktree file**. All 4993 index blobs parsed as pointers.
- **Pre-commit: 4993 joined · 0 drop-outs · 0 MISMATCHES.**
- **Post-commit: 4993 joined · 0 drop-outs · 0 MISMATCHES.**

### ⭐⭐ AND THE PART WORTH KEEPING — THE POSITIVE CONTROL CAUGHT **MY OWN** BROKEN DETECTOR

My first join returned **`0 mismatches` — from `0` joined rows.** The comparison had silently matched nothing: **`sha256sum` writes binary-mode output as `<hash> *<path>`**, and the `*` made every key differ from the git-side path. ⇒ **a total join failure that printed a perfect result.**

⛔ **The `0 mismatches` was a FALSE PASS, and only the control exposed it** — the spiked-digest run reported `mismatches: 0 (expect 1) -> detector IS BROKEN`. After the fix, both controls fire:

| control | expected | observed |
|---|---|---|
| spike one worktree digest | 1 mismatch | ✅ **1 — FIRES** |
| drop one worktree row | 1 drop-out | ✅ **1 — FIRES** |

⚖️ ***A sweep that joins nothing reports the same "0" as a sweep that matches everything. The count of MISMATCHES is meaningless without the count of JOINED ROWS beside it.*** ⭐ This is exactly `TASK-925`'s `IFS= read -r` lesson in a second instrument, and exactly the failure mode the whole `44a8710` chain exists to fight: **a tool reporting SUCCESS over an empty artefact.** I report both numbers above for that reason.

---

## 10. `SC-§29` COVERAGE — NO GATE OWED, AND WHAT THAT DOES NOT MEAN

⛔ **No QA report gates this row and none is owed**: zero `Source/`, zero `Content/`, zero executable, text records only.

⛔⛔ **AND SAYING IT PLAINLY, AS THE ROW REQUIRES: THIS COMMIT IS NOT A SUBSTITUTE FOR THOSE GATES. IT IS WHAT MAKES THEM READABLE.** It re-gates nothing, re-verifies no verdict, and changes no behaviour.

⭐ **What it fixes, measured rather than briefed.** I verified the sharpest instance myself instead of inheriting it — and it is **worse than briefed, in the row's favour**: commit `44a8710` shipped **five** files resting on **two** PASS verdicts, and **both were outside the repository**:

| verdict | was | gated, in `44a8710` |
|---|---|---|
| `qa/TASK-865.md` — PASS, 0 BLOCKER | ⛔ untracked | `concept_generate.py` · `test_concept_guard.py` · `README.md` |
| `qa/TASK-878.md` — PASS, 0 BLOCKER | ⛔ untracked | `trellis_generate.py` · `meshy_generate.py` |

Both are now readable at `HEAD` — verified by `git show HEAD:…/qa/TASK-865.md`, which returns `Verdict: **PASS** — 0 BLOCKER · 5 WARN · 6 NIT`.

⚖️ ***An unreadable gate is indistinguishable from an ungated commit to everyone who was not in the room.*** Every commit of this batch had that property. It is closed for all eight.

**The eight commits whose evidence trail this is** — verified by `git rev-list bf0cd9e..HEAD`, **count 8**, not the seven the dispatch prose said:

`d287102` · `e9df584` · `1aa0fee` · `44a8710` · `a8b97b3` · `8e74596` · `239ca77` · `d101b1e`

**Before this commit** the tracked backlog stopped at **`TASK-805`** (handoffs) and **`TASK-804`** (qa) — measured, not quoted. The single mid-batch exception was `handoffs/TASK-832-artist.md`, taken by `8e74596` as the evidence for the renders it committed.

---

## 11. NOT TOUCHED (FENCE)

⛔ No `Source/**` · ⛔ no `Content/**` · ⛔ no `Content/FogArea/` (`TASK-927`, Jonathan's) · ⛔ no `Tools/**` · ⛔ no `CONVENTIONS.md` / `TASKBOARD.md` content edit beyond this row's own status flip · ⛔ **no compile, no suite, no editor, no MCP** (the editor was **down** and I did not open it) · ⛔ **no push** (`main` **16 ahead** of `origin/main`, counted myself) · ⛔ **no edit to any committed record.**

---

## 12. FOLLOW-UPS FOR THE MANAGER (reported, not boarded — I do not board)

1. ⚠️ **`TASK-810`'s board row still reads `- status: backlog`**, but `qa/TASK-810.md` is a **complete PASS** ("Gate status: TASK-807 → `qa-passed`") and `TASK-807`'s own row cites it as delivered. **The row is stale, the report is sound.** I committed the report and flag the row.
2. ⚠️ **`qa/TASK-889.md:260` now says "DO NOT COMMIT THIS FILE YET" inside a committed file.** Superseded loop-1 text (§6). Worth an annotation so it is never read as live instruction.
3. ⭐ **`TASK-882` sits at `ready-for-qa`, deliberately stopped per its own spec item (0)**, and its §7 records a **compile hazard `TASK-868` self-caught that nobody who can compile has examined.** Its handoff is now readable at `HEAD`; the debt is unchanged and still owed.
4. ⭐ **Law candidate from §9**, offered for `§25b` cl. S: ***report JOINED ROWS beside MISMATCHES.*** A digest sweep that joins zero rows reports the same `0` as a perfect one. My own first run did exactly that and only the positive control caught it.
5. ⛔ **The standing tail is live, and it is TWO files** (§7): `handoffs/TASK-928-buildmaster.md` (**constructed**, unavoidable) **and** `qa/TASK-934.md` (**arrived mid-row**). The next doc-host takes both.
6. ⛔⛔⭐⭐ **URGENT AND NOT MINE — `qa/TASK-934.md` landed at `17:04:11`, after my list was printed.** Three things follow, and all three are the manager's: **(a)** the board says that file **"WILL NEVER EXIST"** and told `TASK-937` not to look for it — **a struck row ran anyway**, so `TASK-937`'s inputs are now wrong; **(b)** its own closing sentence states that **`d101b1e` shipped its hunk UNGATED** and that the gate is satisfied *as of the report, not as of the commit* ⇒ **a ninth commit in this row's defect class, and a worse variety — a LATE gate, not merely an unreadable one**; **(c)** the file is complete and sound (`Verdict: PASS — 0 BLOCKERS · 3 WARN · 4 NIT`) — it needs a host, not a fix.

— build-master, 2026-09-03
