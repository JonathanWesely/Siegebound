# TASK-1377 — `Docs/setupdirections.md` self-contradiction repair (gameplay-programmer, 2026-09-21)

**Limb carried (`SC-§134` cl. 7): the EDIT limb — one tracked markdown file + its vault twin.
No compile, no suite, no commit, no push, no PIE, no MCP, no editor action.**

---

## 1. THE RE-DERIVATION — MY OWN COUNT, AND IT IS **5**, NOT 4

**Prediction handed to me: 4 six-vs-seven sites (lines 87 · 219 · 304 + §3.2's table).
I measured 5. The prediction was a floor, not a count — same species as the boarding
census that said 2 when the gate measured 4.**

### 1.1 The greps that produced it (run at my own instant, on the pre-edit file)

Word/digit census — this is the probe the spec named, and **on its own it finds only 3**:

```
grep -niE '\bsix\b'            → 87, 219, 304          (3 hits)
grep -niE '\bseven\b'          → 1268 (Appendix E, already CORRECT)
grep -niE '(\b6[- ]?agent|agent[s]?[^.]{0,20}\b6\b|\b6\b[^.]{0,20}agents?)'  → 0 roster hits
grep -niE 'sixth|seventh|dozen|\b6\b'  → no further count claims
```

⇒ **A word/digit census cannot find a roster that never states a number.** Sites 4 and 5
are both of that kind, so I ran two structural probes instead:

```
# (a) every line naming 3+ distinct agent roles  → found line 692
awk '... counts the 7 role names per line, prints lines with >=3 ...' Docs/setupdirections.md
    692 [3 roles]: the speaker: `📋 MANAGER:` · `🔍 QA:` · `⚙️ GAMEPLAY-PROGRAMMER:` …

# (b) every line STARTING with a role name (catches tables, which probe (a) cannot:
#     a table puts one agent per line, so no line ever reaches 3 roles)
grep -nE '^[-*|> ]*\**(manager|gameplay-programmer|art-director|qa-reviewer|build-master|footage-analyst|playtest-verifier)\**'
    239–244  → SIX table rows, playtest-verifier absent
```

### 1.2 The five sites

| # | line (pre-edit) | how it says "six" | predicted? |
|---|---|---|---|
| 1 | 87 | `← the six agent definitions` (file tree) | yes |
| 2 | 219 | "the **orchestrator**; six specialist **subagents**" | yes |
| 3 | 304 | "the six `.claude/agents/*.md` files" | yes |
| 4 | 239–244 | §3.2 roster **table**: 6 rows, no `playtest-verifier` | yes |
| 5 | **692–693** | **§9.2 identity-prefix roster: 6 speaker prefixes + ORCHESTRATOR, `🎮 VERIFIER:` absent** | **NO — MY FIFTH** |

**Site 5 is a six-vs-seven site by substance:** it is an enumeration of the agent team in
which the seventh agent does not appear. It states no number, so **every count-word probe
misses it** — including the one the spec prescribed. Its correct value is not inferred: it is
in `.claude/pipeline/SLACK.md` line 64, where `🎮 VERIFIER:` was minted 2026-09-13 by
`TASK-1226` (line 66 adds that `🎮` is an IDENTITY prefix, not a status emoji — reproduced).

### 1.3 Candidates I checked and REJECTED (declared, so the gate can re-check my rejections)

- **§3.7 "The five-role core (manager / builder / maker-of-content / reviewer / integrator)"**
  — NOT a site. It is a deliberate genre-agnostic abstraction over role *kinds* for other
  projects, not a count of this project's roster. Left untouched.
- **§9.2's standing-thread list** (Planning & Feedback · Dev & QA · Art · Build & Git ·
  Blockers · Footage Review = 6) — NOT a site, and I verified rather than assumed: SLACK.md's
  registry has exactly **6** thread rows (lines 30–35), and the verifier posts in the existing
  ⚙️ Dev & QA thread (SLACK.md line 31). The seventh agent did **not** add a seventh thread.
  **Correct as written — I nearly "fixed" a true sentence.**
- **Appendix E line 1268 "Seven agents"** — already correct; it is the sentence the other
  five contradicted. Untouched.
- Chapter 11 (lines 40, 791, 939, 941, 985) already carries the verifier correctly.

### 1.4 Counts for findings (2), (3), (4) — stated even where they agree

| finding | my count | agrees with prediction? |
|---|---|---|
| (2) §3.4 routing law lacking a `verified` gate | **1** site (step 5, line 278) **+ 1** hard-gates paragraph (283–285) = 2 edits in one section | yes |
| (3) §3.3 lifecycle lacking `built`/`verified` | **1** site (the `**Task lifecycle:**` line) | yes |
| (4) THE LIST row 17 vs §11.1 | **1** site (line 40) | yes |

---

## 2. WHERE I DERIVED THE PIPELINE SHAPE FROM

**Not from the file I was repairing, and not from the dispatch's summary.**

- **`CLAUDE.md`** (READ ONLY — see §4): its agent table gives the seven roles; its routing
  rule 5 gives the 5a/5b/5c shape and the `built` status; its hard-gates paragraph gives the
  VERIFIED gate and the "UNOBSERVABLE is recorded on the row, not treated as a pass" rule.
- **`CONVENTIONS.md` `VER-§`** (READ ONLY): `VER-§1` cl. 1 (the byte-literal verdict line),
  `VER-§2` cl. 3 (`built` = compiled, not yet committed), `VER-§5` (an UNOBSERVABLE is never
  promoted to a pass), `VER-§6` cl. 5 (`verified` binding since 2026-09-14).
- **`SLACK.md`** lines 31/64/66 for site 5's prefix.

**Anti-duplication honoured (spec (2) and (4)):** §3.4's new 5b **points at `VER-§1`/`VER-§5`
and deliberately does not copy them**, and §3.3 says in as many words that the verdict
vocabulary "lives in `VER-§1` cl. 1 and is **not** restated here — a second copy drifts."
Row 17 now points at §11.1 instead of restating the tier claim.

**I state NO verdict-token COUNT anywhere in the document.** Reason, flagged for the manager
rather than resolved by me: the spec's (3) describes the four tokens as
`VERIFIED · VERIFY-FAILED · UNOBSERVABLE · and the absence of a runtime criterion`, whereas
**`VER-§1` cl. 1 reads `VERIFIED · VERIFY-FAILED · UNOBSERVABLE · MEASURED`** (MEASURED minted
2026-09-20 by the manager on `TASK-1350`; its cell rule is cl. 3a). Both say "four"; **they do
not name the same four.** Writing either count into the document would have minted a fresh
contradiction against one of the two, so I wrote a pointer instead. **NAMED, NOT REPAIRED —
the spec text is the manager's (`SC-§50`).**

---

## 3. BEFORE / AFTER — EVERY EDIT (8 edits, 5 sections)

1. **Line 87 (site 1, file tree).**
   before: `.claude/agents/              ← the six agent definitions (Chapter 3)`
   after: `… ← the seven agent definitions (Chapter 3)`, plus a dated prose note beneath the
   fence carrying the struck original. **Why prose:** the line is inside a fenced code block,
   where `~~…~~` renders literally — so the strike is preserved where it renders, per
   `SC-§136` cl. 6's intent rather than its letter.
2. **Line 219 (site 2).** `; six` → `; ~~six~~ **seven**` + dated correction naming
   `CLAUDE.md`'s table as source and `playtest-verifier` as the seventh.
3. **§3.2 table (site 4).** Added the missing 7th row: `| **playtest-verifier** | Drives
   Play-In-Editor verification and writes the runtime evidence report `qa/TASK-###-verify.md`
   | Editing code/art, compiling, Git, editor lifecycle | Read/Write + Aura PIE MCP
   (ENUMERATED, never wholesale) + Slack |`, plus a dated note. **Not struck** — a missing row
   is a statement being *completed*, not replaced (`SC-§136` cl. 6's own distinction). The
   tools cell says ENUMERATED because `VER-§7`/R10 forbid a wholesale `mcp__unreal_editor__*`
   grant; I did **not** open or assert anything about the live grant files (see §4).
4. **§3.3 lifecycle (finding 3).** Old chain struck in full, new chain:
   `backlog → in-progress → ready-for-qa → qa-passed/qa-failed → built → verified/verify-failed
   → integrating → done`, + one sentence mapping the two new tokens to 5a/5b and pointing at
   `VER-§2` cl. 3 and `VER-§1` cl. 1.
5. **§3.4 step 5 (finding 2).** Old one-liner struck, replaced by 5a / 5b / 5c sub-bullets in
   `CLAUDE.md`'s shape: 5a compile → `built`, no commit · 5b runtime criterion →
   `playtest-verifier` writes `qa/TASK-###-verify.md`, `verify-failed` bounces and burns a QA
   loop (max-3-then-escalate), Blueprint/asset-only rows skip 5a · 5c passing 5b or no runtime
   criterion → assemble and commit.
6. **§3.4 hard gates (finding 2, second half).** Added the second gate: nothing with a runtime
   acceptance criterion commits without a passing 5b, and a report that could not observe is
   recorded on the row, never read as a pass (`VER-§5`). Existing gates untouched.
7. **Line 304 (site 3).** `the six` → `the ~~six~~ **seven**` + dated source.
8. **Line 692 (site 5) + row 17 (finding 4).** `🎮 VERIFIER:` inserted into the prefix list
   with a dated note citing SLACK.md/`TASK-1226` and the identity-vs-status caveat. Row 17's
   parenthetical `(trial first → paid tier after the pilot measures credit-per-verification)`
   struck and replaced with a pointer to §11.1 step 1's dated correction.

### What I deliberately did NOT write into the hard-gates paragraph
`VER-§3`'s PIE duty. The **wait** is discharged standing (2026-09-20, his sentence); the
**announce-and-report survives** — two different sentences, and only one is true. I left it
out because `CLAUDE.md`'s own hard-gates line still says PIE "waits for a go", and that
disagreement is **F5 — 🧑 his ask, not a row**. Propagating the corrected sentence into this
document would have taken a side in an open human-owned question. **NAMED, LEFT.**

---

## 4. FENCES — DISCHARGED

**No file outside the fence was touched.** Working-tree census at my finish
(`git status --porcelain`, anchored ONE LEVEL UP at `C:/GitProjects/GitHub/GitClaudeUnrealTesting`,
`SC-§102`) shows exactly: `Docs/setupdirections.md` (mine), plus `TASKBOARD.md` and
`qa/TASK-1348-verify.md` and the two untracked build-master handoffs — **all four of which
were already dirty at my start and are other rows' work; I did not open or write any of them**
(beyond this row's own `status:` line on the board, which the `names:` block grants).

- ⛔ **`CLAUDE.md` — READ ONLY, NOT EDITED.** I read its agent table and routing rule 5 as the
  source of truth. The *"waits for a go"* sentence is **untouched**; it is F5 and it needs his
  sentence, not mine. The `TASK-1365` carve-out died with that row.
- ⛔ `CONVENTIONS.md` — read `VER-§` only, **not written**.
- ⛔ No grant surface: I did **not** open `settings.local.json`, `.mcp.json`, or any agent
  `tools:` line. §3.2's new tools cell is written from `VER-§7`'s published rule, not from a
  live grant file — that census is `TASK-1379`'s, not mine.
- ⛔ Untouched: `Source/**`, `Saved/**`, the `.uproject`, any `qa/*-verify.md`, Chapter 4,
  Appendix D.1's allow-list array, the Aura Integration Plan.
- ⛔ No compile · no suite · no commit · no push · no PIE · no MCP call · no editor action.
  **Editor PID 26992 observed as 🧑 his and NOT touched** (`SC-§118`).
- ⛔ `Edit` tool only — **no `replace_all`**, no whole-file rewrite, no Python `open(...,'w')`.
  Every `old_string` was unique at its edit (`SC-§138`: no collision count inherited).

---

## 5. THE MEASUREMENTS

**Vault twin (`SC-§68` + `SC-§120`) — hashed BEFORE any edit, as the law requires:**

| | repo `Docs/setupdirections.md` | vault `…\JonWesOBVault\GitClaudeUnrealsetupdirections.md` |
|---|---|---|
| **BEFORE** | `89639aa32f02ca7860244925ccab81d5fe68b2a6602038756083411afe19a89e` | `89639aa32f02ca7860244925ccab81d5fe68b2a6602038756083411afe19a89e` |
| **AFTER** | `433A7EC2F6A67A771553F01CC0E34A38AD4D3AF9DA538A97B9E83E9373B99DAD` | `433A7EC2F6A67A771553F01CC0E34A38AD4D3AF9DA538A97B9E83E9373B99DAD` |

**EQUAL before (86,052 B each) and EQUAL after (89,339 B each).** The repo file was edited;
the vault copy was produced by a single `Copy-Item -Force` over it — **edited once, copied
once, never edited twice.** The vault file is outside the git root and is **NEVER STAGED**.

**Diff shape (tracked file only, pathspec anchored one level up):**

- `git diff --numstat` → **51 insertions / 14 deletions**
- `git diff -U0` hunk count → **11 hunks** — i.e. N small hunks, **not one giant hunk with
  deletions**, which is what a failed restore looks like.
- **All 14 deleted lines accounted for:** they are the 8 edit sites' original lines, each
  re-emitted struck or corrected. Listed in §3. Nothing else was removed.
- Line count **1,343 → 1,380** (+37, consistent with 51−14). The 1,343 account from
  `TASK-1373` was **re-measured, not inherited**, and it held.
- Incidental corroboration that no bulk rewrite happened: git warns the working copy is LF; a
  line-ending conversion would have shown ~1,343 deletions, not 14.

**Post-edit verification greps:** §3.2 roster row count = **7** · every surviving `six` is
inside a `~~struck~~` form or a dated correction note (lines 94, 224, 254, 336, 730) · no bare
six-agent claim remains.

---

## 6. FOR THE GATE (`TASK-1378`) TO SCRUTINISE

1. **CHECK ONE (did it re-derive?):** §1 above is the answer — **5, not the predicted 4**, with
   the two structural probes that found the extra site and the reason the prescribed
   word/digit probe could not. Probe (b) exists because probe (a) is blind to tables.
2. **My rejections in §1.3 are the other half of the count** — especially §9.2's thread list,
   which I verified against SLACK.md's registry rather than assuming. Please re-check that
   `five-role core` (§3.7) really should stay six-agent-neutral.
3. **The verdict-token count discrepancy in §2** is a **spec-vs-`VER-§1` disagreement**, raised
   not repaired. If the manager rules that the document should carry a count, that is a new
   row, not a silent edit here.
4. **§3.3's pipeline-file bullet list** still omits `qa/TASK-###-verify.md`, `footage/` and
   `playtest-evidence/`. **NAMED, LEFT** — it is a file inventory, not an agent roster or a
   lifecycle token, its lead-in never claims to be exhaustive, and repairing it would be
   widening past findings F1–F4. Manager's call whether it becomes a row.
5. **Routing from here:** `ready-for-qa` → `TASK-1378`. **No 5a, no 5b** (a markdown file has
   no runtime criterion — this is *not* an UNOBSERVABLE). **No commit on this row: it rides
   `TASK-1380`.**
