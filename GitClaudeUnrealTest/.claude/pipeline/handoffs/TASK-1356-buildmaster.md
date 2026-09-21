# TASK-1356 — AIM-LATCH-SWEEP-HOST — build-master handoff

**Commit `0c26c61`** · parent `36d85d3` (`%P`-confirmed ⇒ built **ON** `TASK-1355`, **not amended**) · 9 files, 1059 insertions / 10 deletions · `main` **5 → 6 ahead**, **UNPUSHED** (`origin/main` still `c316929`).

Law: `TL-§5e` cl. 1/7a/7b/7c/7d · `SC-§133` · `SC-§128` · `SC-§137` · `SC-§138` · `SC-§118` cl. 1/8 · `SC-§120` cl. 2 · `SC-§127` · `SC-§102` · `SC-§104` · `VER-§4` cl. 2/4 · `VER-§5` cl. 2.

---

## 0. The early flip — WHEN, measured

`SC-§134` cl. 7(a)/3/8. The `status:` line was flipped to **`in-progress — 5c COMMIT RUNNING`** and **read back as state** (grep, line 5002) **before** `git commit` was invoked. No hash was asserted on that line, because none existed. The `backlog` wording was **struck, not deleted** — deleting it deletes the proof the flip was early. The `done` + hash state was **appended** afterwards; the early wording survives verbatim beneath it.

Sequence, by clock: list-print/window open **20:55:23** → stage → probe → commit returns **20:57:42** → `done` + hash appended.

## 1. `git status --porcelain -uall` BEFORE (quoted)

```
 M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/CLAUDE.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1353-manager.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1354-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1355-buildmaster.md
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1352-a1-ghost-after-setmouselocation_t50.31s_f1883165.png
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1352-a1-ghost-visible-at-set-cursor-point_t82.59s_f1885085.png
?? GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1352-a2-aim-point-latch-check_t14.34s_f1890177.png
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1352-verify.md
```

`main` ahead-count **BEFORE: 5** · **AFTER: 6**. Both measured, neither inherited.

## 2. Anchor proof — `SC-§102`, with a negative control that returned silence

The git root is **one level up** at `C:\GitProjects\GitHub\GitClaudeUnrealTesting`. Proven, not assumed:

- **Mis-anchored control** (`git status … -- .claude/pipeline/` from the git root) ⇒ `could not open directory '.claude/pipeline/': No such file or directory` — **no matching lines**. The trap is real and it answers with silence, not an error against the data.
- **Correct anchor** (`-- GitClaudeUnrealTest/.claude/pipeline/`) ⇒ 9 lines.
- `git ls-files --error-unmatch` ⇒ **TRACKED OK** for `TASKBOARD.md` and `CONVENTIONS.md`.
- `.gitattributes` is likewise **at the git root, not in the project dir** — the first `grep` for the LFS pattern inside `GitClaudeUnrealTest/` failed with *No such file*. Same trap, second instrument.

## 3. The FULL cl. 7a census — every line classified, with the test that produced the verdict

Standing command, run verbatim at my own instant with `--ignored`:
`git status --short --untracked-files=all --ignored -- GitClaudeUnrealTest/.claude/pipeline/`

| line | test run | verdict |
|---|---|---|
| `M CONVENTIONS.md` | `TL-§5e` cl. 7b — adopted, **no grant needed** | ✅ **STAGED.** Content **not reviewed by me** — that is the clause's whole point. Carries the ruling's six law items. |
| `M TASKBOARD.md` | named in my own row's `names:` ⇒ 7a-iii override of the table default | ✅ **STAGED** |
| `?? handoffs/TASK-1353-manager.md` | board read: `TASK-1353` = **`done`** (TERMINAL) | ✅ **ORPHAN ⇒ TAKEN** |
| `?? handoffs/TASK-1354-programmer.md` | board read: `TASK-1354` = **`done`**; mtime **19:20:18** vs `36d85d3` at **19:17:36** ⇒ arrived **2m42s after** that commit and was correctly held by `TASK-1355` | ✅ **ORPHAN ⇒ TAKEN** (it was mine) |
| `?? handoffs/TASK-1355-buildmaster.md` | `TASK-1355` = **`done`**; its `TAIL-TAKER:` reads value (ii) *"STANDING — next commit host under TL-§5e cl. 7a"* ⇒ that is me | ✅ **BOUNDED TAIL ⇒ TAKEN** |
| `?? qa/TASK-1352-verify.md` | `TASK-1352` = `verified`, report on disk | ✅ **TAKEN** — and **not** a forbidden shape on this row (deliberate omission from the probe list; I am boarded to take it) |
| `?? VER-TASK-1352-a1-ghost-after-setmouselocation_…png` | classifier's non-`.md` evidence test: *is the document that argues from it in this commit?* ⇒ **yes** | ✅ **TAKEN** |
| `?? VER-TASK-1352-a1-ghost-visible-at-set-cursor-point_…png` | same | ✅ **TAKEN** |
| `?? VER-TASK-1352-a2-aim-point-latch-check_…png` | same | ✅ **TAKEN** |

**Zero `!!` (ignored) lines** under the pathspec ⇒ nothing to route to the manager under 7a-ii.

**Floor items ABSENT from the derivation** (7a-i's classifier row — named with the commit that took them, so an already-swept file is never confused with an unwritten one). Three other PNGs sit in `playtest-evidence/2026-09-20/` and did **not** appear in the census; `git cat-file -e HEAD:<path>` proves why:

- `VER-TASK-1314-t01m24s-focus-holder-at-match-end.png` ⇒ **in `HEAD`, taken by `1d433ca`**
- `VER-TASK-1348-a2-t00m20s-placement-ghost-up.png` ⇒ **in `HEAD`, taken by `36d85d3`**
- `VER-TASK-1348-a3-t00m06s-wall-ghost-wheel-resize.png` ⇒ **in `HEAD`, taken by `36d85d3`**

⇒ **ALREADY SWEPT ⇒ not candidates.** Duty discharged by another commit, which discharges it as completely as mine would.

## 4. 🚨 NAMED AND HELD — `CLAUDE.md`. I did not stage it, and this is the one judgement call on the row.

**The dispatch instructed me to stage it**, recording that Jonathan authorised the amendment directly in Claude Code (*"Amend rule 5"*) and that the orchestrator applied it. **I measured the file and declined to take it.** Reasons, in order of weight:

1. **My own row's `names:` line reads `NEVER CLAUDE.md`** — and it says so *one clause after* granting exactly the staging carve-out it withholds here: *"NEVER `CONVENTIONS.md`'s CONTENT (you **STAGE** it under `TL-§5e` cl. 7b and say that you did; that is all)"*. The manager drew the write/stage distinction explicitly for one file and withheld it for the other. That asymmetry is drafting, not oversight.
2. **cl. 7a's standing sweep does not reach it.** The standing command's pathspec is `.claude/pipeline/`. `CLAUDE.md` is outside it — measured: the full-repo status returns **10** lines, the sweep returns **9**, and the difference is exactly this file.
3. **cl. 7b adopts `CONVENTIONS.md` alone.** It names one file and gives one reason (the manager edits it constantly and holds no git). `CLAUDE.md` is not in that clause.
4. **A named, dispatchable host already exists.** `TASK-1361` spec (2) boards `CLAUDE.md` *"IF AND ONLY IF `TASK-1359` HAS RUN"*, and its `blocked-by` states the **operative** test in disk terms: *"`TASK-1359` is a CANDIDATE you ADOPT IF AND ONLY IF **its diff is on disk at your own dispatch instant**"*. The diff **is** on disk ⇒ that test will pass. `TASK-1361`'s own blocker is a report on disk, never an undated human decision, so 7a-vi's staleness bound does **not** fire and 7a-v's default holds: **SCHEDULED, not orphaned.**
5. **`SC-§97` / cl. 7c.** Three agents have already correctly refused to touch this file on a relay. cl. 7c's own recorded precedent is a host refusing a dispatch that told it to take scheduled files: *"an orchestrator's dispatch prose is not authorisation to breach a board fence."* The dispatch itself concedes the order of precedence — *"Board and law outrank this dispatch."*

**I checked cl. 7c's bidirectional amendment before deciding**, because a dispatch *stricter* than the law is as much a defect as a looser one and it fails silently. It does not apply: no law **requires** this take. The sweep does not reach the file, 7b does not adopt it, and a live host is boarded for it. Holding costs nothing — the file stays on disk, fully intact — whereas taking it would breach an explicit `names:` fence on the strength of a relay.

**What I *can* measure, and did** (`SC-§91`) — the diff is **3 insertions / 3 deletions**, matching `TASK-1359`'s three specced sites exactly and corroborating the dispatch's description of a minimal edit:

- line 25's token enumeration gains `/ MEASURED`
- routing rule 5c's parenthetical gains `or MEASURED`, plus the load-bearing semantic clause (`MEASURED` routes *like* `UNOBSERVABLE` but never blocks, never bounces, and is earned by a control that discriminated)
- the hard-gates line becomes *"UNOBSERVABLE and MEASURED are recorded on the row"*

This is a **measurement of shape, not of authorisation.** I cannot measure who typed it.

⇒ **`HELD-FOR: TASK-1361`.**

**Routed to the orchestrator/manager, by name:** `TASK-1359`'s row still reads `backlog` while its substance sits applied on disk, and `handoffs/TASK-1359-programmer.md` does not exist. Either dispatch that row so its handoff is minted, or have the manager rule the orchestrator-applied edit onto it — otherwise `TASK-1361` inherits an ambiguity about whether the row "has run". **Not mine to flip** (`SC-§100`).

## 5. 🚨 ARRIVED INSIDE MY STAGING WINDOW — also named and held

`handoffs/TASK-1360-programmer.md` was **not present** at my census and appeared at mtime **20:57:48** — **6 seconds after my commit returned (20:57:42)** and 2m25s after my window opened (20:55:23).

**Two independent grounds to leave it**, either alone sufficient:
- cl. 7a's **mtime rule**: *ARRIVED ⇒ LEAVE IT*, named by name and by kind — it is a **`handoffs/` note from the parallel `gameplay-programmer` lane (`TASK-1360`)**.
- the classifier's **LIVE-row** test: `TASK-1360`'s `status:` still reads `backlog` (its flip is the manager's) ⇒ **LIVE ⇒ LEAVE IT.**

⇒ **`HELD-FOR: TASK-1361`** (or the next host that derives at its own instant). Its author may still have been writing; sweeping a half-written file is the failure this rule exists to prevent.

## 6. Forbidden-shape probe over the STAGED SET, before committing — with a control that FIRES

Run against `git diff --cached --name-only` **before** the commit. Every pattern **0**:

```
\.cpp$ => 0    \.h$ => 0    \.uasset$ => 0    \.umap$ => 0
^.*/Source/ => 0    ^.*/Tools/ => 0    run_suite_bounded\.ps1 => 0
/Saved/ => 0    testvideo/ => 0
```

**`SC-§137` complement control — TWO patterns, BOTH FIRING**, because this pathspec carries both kinds and an `.md`-only control would not prove the probe sees the PNGs:

```
\.md$  => 6   FIRES
\.png$ => 3   FIRES
```

⇒ the nine zeros are a **measurement**, not the silence of a broken probe.

Deliberate non-forbidden on this row: `\-verify\.md$` ⇒ **1**, and that is correct — I am boarded to take `qa/TASK-1352-verify.md`.

## 7. The three PNGs — oid-vs-sha256, NEVER size

`*.png filter=lfs diff=lfs merge=lfs -text` confirmed at the **git root** `.gitattributes:4`, and `git check-attr` confirms `filter: lfs` on a live target. Disk `sha256sum` vs the LFS pointer's `oid sha256:` — checked at the index **and re-read from `HEAD` after the commit**:

| frame | sha256 = oid | verdict |
|---|---|---|
| `…a1-ghost-after-setmouselocation_t50.31s_f1883165.png` | `ab6b4cc5e5d68f9a106f47e7ffd3fea1a9cd98f218ba281815b3332973b6efe4` | ✅ MATCH |
| `…a1-ghost-visible-at-set-cursor-point_t82.59s_f1885085.png` | `4e20eccaa38c844a959ef14e0685086266d18cec9050c32155d36d2af295361e` | ✅ MATCH |
| `…a2-aim-point-latch-check_t14.34s_f1890177.png` | `c64d888e406e4428eeebfd480584e125f6e874ca8171dd379bc44bf277870c72` | ✅ MATCH |

All three are **3/0 lines** in `--numstat` ⇒ pointer files, stored in LFS, not inlined.

**All three taken, including the two that show NO ghost** (`VER-§1` cl. 6 — a host that promotes only the pretty frame has edited the evidence). **No rename owed** (`VER-§4` cl. 2 as amended today: `capture_pie_frame`'s own `_t<S.SS>s_f<frame>` stamp satisfies the `-t…` element) — committed under the names on disk.

## 8. `git show --numstat HEAD` (`SC-§128`, never `--stat`)

```
32	4	GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md
143	6	GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
140	0	GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1353-manager.md
299	0	GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1354-programmer.md
194	0	GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1355-buildmaster.md
3	0	GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1352-a1-ghost-after-setmouselocation_t50.31s_f1883165.png
3	0	GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1352-a1-ghost-visible-at-set-cursor-point_t82.59s_f1885085.png
3	0	GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1352-a2-aim-point-latch-check_t14.34s_f1890177.png
242	0	GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1352-verify.md
```

**Index checked BEFORE any `add`** ⇒ **empty** (the UE Git plugin had auto-staged nothing). Every path staged with its **own explicit `git add -- <path>`** — never `-A`, never `.`, never a bare directory. Committed with `-F <msgfile> -- <paths>` (never `-m` after `--`, which is eaten as a pathspec). **The COMMIT was verified, never the index.**

## 9. Board-write discipline

`replace_all` **banned and not used**. Anchored on **unique quoted text**; `Edit`'s own non-unique failure mode is the discriminator, so uniqueness is proven by the edit succeeding. Per **`SC-§138` — minted today — I state the METHOD and inherit no count**: the unique `#### TASK-####` header was measured at my own instant (`^#### TASK-1352 ` ⇒ 1, `^#### TASK-1356 ` ⇒ 1). Every flip **read back as state**.

**Board did not shrink** (`SC-§120` cl. 2): 37784 → 37921 lines at the commit; after the two tail flips, 37921 = 37921 with `--numstat` reading **2/2 on `TASKBOARD.md` alone** ⇒ two status lines rewritten in place, **no other row touched**.

Rows flipped: **this one** (`names:` carve-out) and **`TASK-1352` → `done` + hash**, authorised at spec (3) **by name, on the manager's initiative** — absent that naming I would have been right to refuse. `TASK-1352`'s verdict prose kept **verbatim** beneath. **No other row.** `TASK-1353`'s and `TASK-1354`'s lines were **committed, not edited**.

## 10. Legs NOT run — declared, not omitted

- **NO 5a.** Docs-only; `Source/**` fenced ⇒ nothing to build, and a `Result:` line would be evidence of nothing. **`Build.bat` not invoked.**
- **NO 5b.** No runtime acceptance criterion (`VER-§5` cl. 2) — **not an `UNOBSERVABLE`**, never owed.
- **NO SUITE.** I claim **no fresh number**; baseline stands at `TASK-1340`'s **561/561 at `d9a98d1`**.
- **NO PUSH.** `main` is **6 ahead**, local.
- **NO `--allow-empty`** (banned in this duty; the derivation was non-empty anyway).

## 11. 🚨 Editor census — by command line, acted on NOTHING

Censused at my own instant (`SC-§118` cl. 1/8), **re-censused not inherited**, **before and after**:

| PID | image | classification |
|---|---|---|
| **27484** | `UnrealEditor.exe` | **GUI on `GitClaudeUnrealTest.uproject`, no `-game`** — the same process `TASK-1347` relaunched on the `ce4947d` binaries. CreationDate **11:42:09 AM**. |
| 15612 | `UnrealTraceServer.exe` | `daemon -d --sponsor 27484` — the editor's own trace daemon, **not an editor instance** |

**ZERO `-game` instances** ⇒ nothing of Jonathan's in the census, nothing to ask-and-wait on.

**CreationDate 11:42:09 is IDENTICAL before and after my work ⇒ UNRESTARTED, UNTOUCHED.** No lifecycle action of any kind: no close, no relaunch, no compile, no Live Coding. **Censused and reported; acted on nothing.** The fence has now held across **six** consecutive rows. **`TASK-1357` drives this editor.**

## 12. MY TAIL — `TAIL-TAKER:` honoured

**Value (i): `TASK-1361`** — a boarded row, assignee `build-master`, dispatchable (its `blocked-by` is a report on disk, never an undated human decision).

**Bounded at TWO FILES, named by path:**
1. `.claude/pipeline/TASKBOARD.md` — this row's `done` + hash line **and** `TASK-1352`'s. **`TL-§5e` cl. 7d(iii) as ruled today: the bound is at FILE granularity, not item granularity**, because the taker acts by pathspec and flips authorised elsewhere on the row are covered by the file that carries them. **Two files carrying four items is not a breach**, and I do not read my own compliance as one.
2. `.claude/pipeline/handoffs/TASK-1356-buildmaster.md` — this file (structurally excluded: it carries my hash).

**NO second commit was invented to swallow this ledger** — that regress does not terminate (`TASK-1347` refused it and is now law).

**Also riding to `TASK-1361`:** `CLAUDE.md` (§4) and `handoffs/TASK-1360-programmer.md` (§5), both **named and held**, neither swept.
