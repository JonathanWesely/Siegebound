# TASK-1360 — VAULT-NOTE-RESIDUAL-SYNC — gameplay-programmer handoff

**Marker:** `TASK-1360-VAULT-NOTE-RESIDUAL-SYNC` · **Date:** 2026-09-20 · **Agent:** gameplay-programmer
**Law:** `SC-§50` · `SC-§100` · `SC-§91` · `SC-§97` · `SC-§134` cl. 7(b)

---

## 0. ⛔ STATUS RELAY — I DID NOT FLIP MY OWN ROW, AND THE FLIPPER IS NAMED

`SC-§134` cl. 7(b). My `names:` block does **NOT** include `TASKBOARD.md`. ⇒ **I did not touch the board.**
This row's `status:` still reads `backlog` while the work is on disk. **THE FLIPPER IS THE MANAGER**, exactly as this row's own `status:` line states and exactly as `TASK-1354` did it (where the relay held).

⇒ 🔁 **OWED: the manager flips `TASK-1360` → `ready-for-qa` / `done` per its ROUTING (gate waived).** Saying it here so it cannot be dropped silently.

---

## 1. ⛔ REPO vs VAULT — WHICH FILES A COMMIT HOST CAN TAKE

| File | Tree | In `git status`? | Host action |
|---|---|---|---|
| `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\UE5 Agent Team System.md` | ⛔ **VAULT — OUTSIDE THE REPO** | ❌ **NEVER** | ⛔ **NOT COMMITTABLE. Do not look for it; it is a different tree** (`C:\GitProjects\GitHub\MyObsidianVault`) |
| `.claude/pipeline/handoffs/TASK-1360-programmer.md` | ✅ **IN REPO** | ✅ yes, untracked | ✅ **THE ONLY ARTEFACT A HOST TAKES FROM THIS ROW** |

⚠️ **The substance of this row is invisible to git.** The one in-repo trace is this handoff. A host that stages only `handoffs/TASK-1360-programmer.md` has taken 100% of what is takeable.
⚠️ Vault path re-measured: the vault **MOVED** — `C:\JonWesOBVault` is stale/empty; live path is `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\`.

---

## 2. ⛔ MEASUREMENTS AT MY OWN INSTANT (`SC-§91` — nothing inherited from the dispatch)

### 2a. The agent count — RE-MEASURED FROM THE DIRECTORY, NOT RELAYED

```
$ ls -1 .claude/agents/*.md | wc -l
7
```

The seven, by name, measured not assumed:

```
art-director.md   build-master.md      footage-analyst.md   gameplay-programmer.md
manager.md        playtest-verifier.md qa-reviewer.md
```

⇒ **7.** The dispatch told me `TASK-1354` measured 7; I did not inherit that — the count above is my own `ls`. It agrees.

### 2b. `grep -c MEASURED CLAUDE.md` — THE (5) INTERLOCK

```
$ grep -c MEASURED CLAUDE.md
3
```

**⇒ 3, which is `> 0` ⇒ PER SPEC (5) I ⛔ TOOK the four-word edit, AND I SAY SO HERE.** The three sites, quoted from the file at my instant:

```
25:- `.claude/pipeline/qa/TASK-###-verify.md` — … (`VERIFIED` / `VERIFY-FAILED` / `UNOBSERVABLE` / `MEASURED`; law: CONVENTIONS VER-§)
49:   - **5c** `verified` (or `UNOBSERVABLE`, or `MEASURED`, or no runtime criterion) → invoke `build-master` …
66:- Nothing with a runtime acceptance criterion is committed without a VERIFIED report; UNOBSERVABLE and MEASURED are recorded on the row, not treated as a pass.
```

🚨 **THE PREMISE GENUINELY CHANGED AND I RE-MEASURED IT RATHER THAN INHERITING EITHER STATE.** `TASK-1354` §6/F1 recorded `grep -c MEASURED CLAUDE.md` = **0** and correctly refused the edit on that basis. That reading is now **stale**: the file carries the token at **exactly the three sites F1 named (25, 49, 66)**. ⇒ The restraint was right then; the edit is right now. Same rule, different measurement.

### 2c. `Docs/` twin — RE-MEASURED (spec (1) told me to re-measure rather than trust it)

```
$ find . -iname "*Agent Team System*" -not -path "./.git/*"
(no output)
$ ls Docs/
Aura AI for Unreal — Integration Plan.md   AuraIndexIgnore.txt   AuraProjectMemory.md   Data
GDD-Submission-v3.pdf   GDD-TEMPLATE.md   GDD-UPDATE-2026-07-21.md   GDD.md
Packaging   ThirdPartyNotices.md   setupdirections.md
```
⇒ ✅ **CONFIRMED: no `Docs/` twin exists. Vault-only. One copy, no divergence risk.**

### 2d. F3's three files — EXISTENCE VERIFIED ON DISK, not trusted from the count

```
$ ls .claude/pipeline/
CONVENTIONS.md  SLACK.md  TASKBOARD.md  fab/  footage/  handoffs/  playtest-evidence/  qa/  …
$ ls .claude/pipeline/qa/ | grep -i verify      → TASK-1068-verify.md, TASK-1230-verify.md, … (13+)
$ ls .claude/pipeline/footage/                  → VID-001 … VID-007 (7 reports)
```
⇒ ✅ All three referents are real: `qa/TASK-###-verify.md` (a live filename pattern), `footage/`, `SLACK.md`. **The bullet list was short by three against a directory that actually holds them.**

### 2e. File hashes

| | sha256 |
|---|---|
| **BEFORE** | `5dbd489402e0c93239b9daad4d4be900d07620c0f166836ab39909bcc6ec8eac` |
| **AFTER** | `e67b378ada845bce605f35262d3ce63fdc1f9fc96a2361fb83871b852190e3a1` |

⇒ **The hash CHANGED.** Size 20,521 B → grown by the F3 insertion and the F4 expansions.

---

## 3. ⛔ THE DIFF — BEFORE AND AFTER, EVERY LINE CHANGED

### ✅ EDIT 1 — **F2**: `(the five agents)` → the measured number (§ *Launching the Team*, step 2)

**BEFORE**
```
Starting in the project root is what loads the whole system: `CLAUDE.md` (orchestrator rules), `.claude/agents/` (the five agents), `.mcp.json` (Unreal + Blender).
```
**AFTER**
```
Starting in the project root is what loads the whole system: `CLAUDE.md` (orchestrator rules), `.claude/agents/` (the seven agents), `.mcp.json` (Unreal + Blender).
```
**Token traceability:** `seven` ⟵ **my own `ls -1 .claude/agents/*.md | wc -l` = 7** (§2a). Nothing else in the line touched.
**Residual check:** `grep -c "five agents"` ⇒ **0**. ⭐ And a sweep for any *other* stale roster count (`five|six|seven|four agents|the N agents`) returned **this line only** — so after this edit the note has **no** surviving miscount.

> ⭐ **THE LESSON THE SPEC NAMED, CONFIRMED BY MEASUREMENT:** the 5→7 defect was fixed in the **roster table** and the **Status callout** (`TASK-1231`) and still survived here, in running prose under a *setup-instructions* heading that neither row's spec thought to name. **A count fixed in the places you think of is not a count fixed.** The sweep above — not the spec's list — is what proves this one is now the last.

---

### ✅ EDIT 2 — **F3**: the Communication Architecture list, short by three · **THE SEAM `TASK-1354` DECLARED IS NOW CLOSED**

**BEFORE** (§ *Communication Architecture*, the list ended at `qa/`)
```
- `.claude/pipeline/qa/` — QA reports flowing back to the programmer and forward to the build-master

```mermaid
```
**AFTER**
```
- `.claude/pipeline/qa/` — QA reports flowing back to the programmer and forward to the build-master
- `.claude/pipeline/qa/TASK-###-verify.md` — runtime verification reports from `playtest-verifier` (`VERIFIED` / `VERIFY-FAILED` / `UNOBSERVABLE` / `MEASURED`; law: CONVENTIONS VER-§)
- `.claude/pipeline/footage/` — footage diagnosis reports (VID-###) consumed by manager to board fixes (law: CONVENTIONS FR-§)
- `.claude/pipeline/SLACK.md` — Slack mirror protocol: channel, threading law, posting matrix

```mermaid
```

**⛔ QUOTED FROM `CLAUDE.md`, NOT RE-WORDED (`SC-§97`) — AND PROVEN, NOT ASSERTED.** Each new bullet was string-compared against its `CLAUDE.md` source line:

```
$ for each of the 3 bullets: compare CLAUDE.md line vs vault line
IDENTICAL (verify bullet)      ← CLAUDE.md:25
IDENTICAL: pipeline/footage/   ← CLAUDE.md:26
IDENTICAL: pipeline/SLACK.md   ← CLAUDE.md:27
```
⇒ **All three are byte-identical to their source lines**, in `CLAUDE.md`'s own order. Zero tokens of mine.

⭐ **THE SEAM THAT ROW CREATED IS THE ONE THAT CLOSED.** `TASK-1354` declared it honestly: its new prose at line 127 cites `.claude/pipeline/qa/TASK-###-verify.md` as the path a `verify-failed` bounce carries, while the bullet list **four lines above** did not mention the file existed. A reader met the path in a sentence before the inventory admitted it. **That is now repaired at the inventory, which is the right end** — the prose was never wrong.

> ⚠️ **ONE STYLE NOTE, DECLARED NOT HIDDEN:** the four pre-existing bullets are *lightly re-worded* from `CLAUDE.md` (e.g. `"the hub: task specs…"` vs the source's `"task specs… (the hub)"`). My three are **verbatim**. The list is therefore mixed-register by one notch. I chose verbatim because spec (3) ordered it explicitly (*"Quote them FROM `CLAUDE.md`, do not re-word them from here"*) — **fidelity beats uniformity**, and re-wording to match would have destroyed the only property that makes these three auditable.

---

### ✅ EDIT 3 — **F4 (a)**: the Workflow step that knows only one gate

**BEFORE** (§ *Workflow*, step 5)
```
5. A **review/QA step** verifies results before anything is committed to Git
```
**AFTER**
```
5. **Two gates** stand before anything is committed to Git: the **QA review** of the code (*"Nothing is committed to Git without a PASS QA report (code) or completed integration check (art)"*), and then — for any task whose spec carries a runtime acceptance criterion — the **Aura PIE verification** (*"Nothing with a runtime acceptance criterion is committed without a VERIFIED report"*)
```

**Token traceability — BOTH quotations verbatim from `CLAUDE.md` § Hard gates at my instant:**
- `"Nothing is committed to Git without a PASS QA report (code) or completed integration check (art)."` ⟵ **`CLAUDE.md`:65**
- `"Nothing with a runtime acceptance criterion is committed without a VERIFIED report"` ⟵ **`CLAUDE.md`:66**, leading clause

---

### ✅ EDIT 4 — **F4 (b)**: the design principle that knows only one gate

**BEFORE** (§ *Design principles* callout)
```
> - **Verify before commit.** No agent pushes to Git without a QA pass.
```
**AFTER**
```
> - **Verify before commit — two gates, not one.** *"Nothing is committed to Git without a PASS QA report (code) or completed integration check (art)"* — and, when the task has a runtime acceptance criterion, *"Nothing with a runtime acceptance criterion is committed without a VERIFIED report"* on top of it.
```

**Token traceability:** same two `CLAUDE.md` § Hard gates lines (65, 66) as Edit 3.

> 🚨⭐ **A DELIBERATE QUOTATION CHOICE, MADE TO AVOID MANUFACTURING A SECOND SEAM — THE SINGLE MOST LOAD-BEARING DECISION IN THIS ROW.**
> `CLAUDE.md`:66 now reads in full: *"…without a VERIFIED report; **UNOBSERVABLE and MEASURED** are recorded on the row, not treated as a pass."* The note's **own** hard-gates bullet (line 144, **fenced by spec (6) as `TASK-1354`'s**) quotes the **pre-amendment** form of that same sentence: *"…UNOBSERVABLE is recorded on the row…"*.
> ⇒ Had I quoted line 66 **in full** here, the note would hold **the same source sentence quoted two contradictory ways, four screens apart** — and I would have *authored* the contradiction.
> ⇒ **So I quoted only the leading clause** — *"Nothing with a runtime acceptance criterion is committed without a VERIFIED report"* — which is **byte-identical in the old and the new `CLAUDE.md`**, i.e. **invariant across the amendment**. It repairs F4 completely (F4's defect is *one gate vs two*, and the VERIFIED gate is now named) while **colliding with nothing**.
> ⛔ **The seam at line 144 therefore stays exactly one item — F8 below — instead of becoming two, and I did not create either.**

---

### ✅ EDIT 5 — **SPEC (5)**: the conditional four-word edge label — ⛔ **TAKEN**, on `grep` = **3**

**BEFORE** (mermaid, 5c edge)
```
    V -->|verified / UNOBSERVABLE / no runtime criterion| C["build-master — 5c<br/>assemble + git commit"]
```
**AFTER**
```
    V -->|verified / UNOBSERVABLE / MEASURED / no runtime criterion| C["build-master — 5c<br/>assemble + git commit"]
```

**⛔ DECISION, STATED AS SPEC (5) DEMANDS:** `grep -c MEASURED CLAUDE.md` = **3** at my instant ⇒ **`> 0` ⇒ I TOOK IT.** The label now cites a file that **does** say it (`CLAUDE.md`:49, rule 5c). One label, four words, nothing else in the mermaid touched — spec (6)'s sole carve-out, honoured exactly.

🔁 **RELAY TO `TASK-1358` (its spec (5) asked which happened):** ⇒ **IT RODE `TASK-1360`.** `TASK-1358` does **not** need to board it fresh, and no later row owes it.

---

## 4. ⛔ NOT MINE — FLAGGED, NOT FIXED (spec (6): *"FLAG IT, do not fix it"*)

I touched **nothing** in `TASK-1231`'s roster/tool-layer tables or `TASK-1354`'s lifecycle line, hard-gates block or mermaid, beyond Edit 5's one authorised label.

**Two of those fenced elements are now STALE — through no fault of either row.** Both quote `CLAUDE.md` sentences that the `MEASURED` amendment rewrote **after** `TASK-1354` shipped. Both are presented to the reader as **verbatim**, which is what makes them worth a row:

### 🚩 **F7 — the 5c quotation no longer matches its source** (note line ~138)

Under a heading that reads *"quoted **verbatim** from `CLAUDE.md` § Routing rules"*:
```
- **`verified` → `integrating` → `done` (5c)** — *"`verified` (or `UNOBSERVABLE`, or no runtime criterion) → invoke `build-master` to assemble and commit as today."*
```
`CLAUDE.md`:49 **now** reads:
```
- **5c** `verified` (or `UNOBSERVABLE`, or `MEASURED`, or no runtime criterion) → invoke `build-master` to assemble and commit as today. `MEASURED` routes *like* `UNOBSERVABLE` but is **not** it: it never blocks and never bounces, and it is earned by a control that discriminated (a controlled negative), whereas `UNOBSERVABLE` means the lane could not see at all.
```
⇒ The quotation is **missing `MEASURED`** *and* the whole second sentence — the one that draws the `MEASURED` ≠ `UNOBSERVABLE` distinction, which is the single most confusable point in the verify lane.

### 🚩 **F8 — the second hard-gates bullet no longer matches its source** (note line ~147)

Under *"**Hard gates**, **verbatim** from `CLAUDE.md` § Hard gates"*:
```
- *"Nothing with a runtime acceptance criterion is committed without a VERIFIED report; UNOBSERVABLE is recorded on the row, not treated as a pass."*
```
`CLAUDE.md`:66 **now** reads `…; UNOBSERVABLE **and MEASURED** are recorded on the row, not treated as a pass.`

> 🚨 **WHY THESE TWO ARE WORTH A ROW AND NOT A SHRUG — AND WHY I AM CERTAIN THEY ARE THE SAME DEFECT CLASS AS F2/F3/F4:**
> Neither says anything *false about how the pipeline works*. **What is false is the attribution.** Both are labelled **verbatim**, and neither is. That is **precisely** the failure `TASK-1354` §6 refused to commit when it declined to write `MEASURED` into a verbatim-labelled bullet — and the manager **ruled that restraint correct**. The label is a promise that the text was copied; a reader who trusts it and greps `CLAUDE.md` finds a different sentence and cannot tell which is current.
> ⭐ **AND NOTE THE SYMMETRY, WHICH IS THE WHOLE POINT:** `TASK-1354` was right to withhold `MEASURED` when the source lacked it. The source now has it — so the *same* rule that justified withholding then **requires adding now**. F7/F8 are not a reversal of that ruling; they are its continuation.
> ⛔ **A verbatim label is a maintenance liability by construction:** it is correct only for as long as its source is frozen. `CLAUDE.md` is not frozen — it moved **within days**. Any row taking F7/F8 may want to consider whether these blocks should cite `CLAUDE.md` by **§ and line** rather than by copy, so the next amendment ages them into *stale pointers* (harmless) instead of *false quotations* (not).

### ⚠️ ONE SEAM I **DID** CREATE, DECLARED LOUDLY RATHER THAN LEFT TO BE FOUND

Edit 5 was **ordered** by spec (5) conditional on a measurement I made and reported. Taking it means the note now shows, within one screen:

| Site | Says | State |
|---|---|---|
| mermaid 5c edge (my Edit 5) | `verified / UNOBSERVABLE / **MEASURED** / no runtime criterion` | ✅ current |
| 5c quotation ~13 lines below (**F7**, fenced) | `(or UNOBSERVABLE, or no runtime criterion)` | ⛔ stale |

⇒ **The diagram now lists four routes and the quotation beneath it lists three.** This is a **real**, reader-visible inconsistency and I am not softening it. It exists because spec (5) authorised the label and spec (6) fenced the quotation — **I obeyed both**, and the correct response to that collision is `SC-§50`: **name it, don't quietly widen my scope to resolve it.** ⛔ **F7 is therefore not optional cleanup — it is the closing half of an edit this row was ordered to make.**

---

## 5. ⛔ FENCES — WHAT I DID NOT DO

⛔ No compile · ⛔ no suite · ⛔ no commit · ⛔ no push · ⛔ no git of any kind · ⛔ no engine / MCP / editor action · ⛔ no `Source/**`, `Saved/**`, `.uproject` · ⛔ no asset.
⛔ **`TASKBOARD.md` — NOT TOUCHED** (§0; a commit host, `TASK-1356`, was live) · ⛔ **`CONVENTIONS.md` — NOT TOUCHED** · ⛔ **`CLAUDE.md` — READ AND QUOTED, NEVER WRITTEN** (`SC-§97`; that file is `TASK-1359`'s and gated on 🧑 him — I read it 4×, wrote it 0×).

---

## 6. ⛔ WHAT QA / THE MANAGER SHOULD SCRUTINISE

1. **The count.** Re-run `ls -1 .claude/agents/*.md | wc -l`. If it is not 7, Edit 1 is wrong — it is the one token in this row with no textual source, only a directory.
2. **The three F3 bullets.** Re-run the byte-comparison against `CLAUDE.md`:25–27. They must be **identical**, not merely equivalent.
3. **Edit 3/4's quotation choice** (§3, Edit 4 callout) — I deliberately quoted only the **invariant leading clause** of `CLAUDE.md`:66 to avoid authoring a contradiction with the fenced line 147. **If you would rather the note quote line 66 in full, then F8 must be taken in the same row**, or the note will contradict itself.
4. **F7 / F8** — both need a row, and **F7 is the closing half of Edit 5**, not independent cleanup.
5. **The seam table in §4** — confirm you accept the four-vs-three inconsistency as the correct consequence of obeying specs (5) and (6) together, rather than a defect of mine.

🧑 **STILL OPEN AND ⛔ NOT CLOSED BY ME — `TASK-1354`'s pixel check: *does the mermaid flowchart actually render in Obsidian?*** I edited **one edge label inside that block** and therefore had one more reason than `TASK-1354` did to want the answer. ⛔ **I still cannot measure it — I have no Obsidian renderer — and spec (5) forbids closing it by assertion.** The label edit is a pure text substitution inside an existing, unchanged edge, so it cannot *newly* break parsing; but *"it cannot newly break"* ⛔ **is not** *"it renders"*, and the original question is untouched. **Still owed to 🧑 him.**

---

## 7. ⛔ SUMMARY

| # | Finding | Action | Evidence |
|---|---|---|---|
| **F2** | `(the five agents)` | ✅ **FIXED** → `seven` | my own `ls` = 7 |
| **F3** | comms list short by 3 | ✅ **FIXED** — 3 bullets, byte-identical | `CLAUDE.md`:25–27 |
| **F4** | 2 single-gate restatements | ✅ **FIXED** — both now name both gates | `CLAUDE.md`:65–66 |
| **(5)** | 5c edge label | ✅ **TAKEN** (`grep` = **3** > 0) | `CLAUDE.md`:49 |
| **F7** | 5c quotation stale under a *verbatim* label | 🚩 **FLAGGED, NOT TAKEN** | fenced by spec (6) |
| **F8** | hard-gates bullet 2 stale under a *verbatim* label | 🚩 **FLAGGED, NOT TAKEN** | fenced by spec (6) |

**Vault file:** `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\UE5 Agent Team System.md` — `5dbd4894…8eac` → `e67b378a…e3a1` ⛔ **NOT IN GIT.**
**Repo file:** `.claude/pipeline/handoffs/TASK-1360-programmer.md` ✅ **the only committable artefact.**
**Status:** work complete; **`status:` flip owed by the MANAGER** (§0).
