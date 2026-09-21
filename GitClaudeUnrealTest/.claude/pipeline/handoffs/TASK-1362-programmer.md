# TASK-1362 — [VAULT-NOTE-VERBATIM-RESYNC] — gameplay-programmer handoff

**Marker:** `TASK-1362-VAULT-NOTE-VERBATIM-RESYNC` · **Date:** 2026-09-20 · **Outcome:** ✅ **PROCEEDED — F7 + F8 both taken, plus spec (5)'s source-stamps.**

---

## 1. ⛔ FIRST SECTION — THE THINGS THAT MUST BE SAID BEFORE ANYTHING ELSE

### 1.1 ⛔ I DID NOT FLIP MY OWN `status:` LINE — **THE MANAGER FLIPS IT** (`SC-§134` cl. 7(b))

My `names:` does **not** include `TASKBOARD.md`. The row's `status:` is still whatever the manager last wrote (`backlog` at boarding). **I have not touched it and will not.** ⇒ **Manager: this row is complete and the line is yours to flip.** `TASK-1354` and `TASK-1360` both declared this and the relay held both times; this is the third.

### 1.2 🚨 THE RESIDUAL COUPLING — spec (1-bis), in the terms the spec required

> ⛔ **These edits are PAIRED to the `CLAUDE.md` `MEASURED` amendment; if that is reverted, revert these with it.**

The amendment is **still uncommitted** (`git status` shows ` M GitClaudeUnrealTest/CLAUDE.md` — it was already dirty at my session start, before I read anything) and it remains 🧑 **Jonathan's to revoke**. If he reverts it, the two passages I just repaired become **false-by-attribution in the opposite direction**, with *this row* as their author. That is not a reason to have refused the row — every document that quotes a live file carries this — but it is a reason the pairing is written here, where the next reader meets it. The dated source-stamp at §3 is the standing mitigation.

### 1.3 ⛔ REPO vs VAULT — WHAT A COMMIT HOST CAN AND CANNOT TAKE

| File | Location | In the project repo? | Host action |
|---|---|---|---|
| `UE5 Agent Team System.md` | `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\` | ⛔ **NO — a SEPARATE git repository** (`C:\GitProjects\GitHub\MyObsidianVault`) | ⛔ **UNTAKEABLE from this repo.** It will **never** appear in `GitClaudeUnrealTest`'s `git status`. Measured: `git ls-files` filtered for `Obsidian` / `Agent Team System` → **empty**. It shows as ` M "JonWesOBVault/UE5 Agent Team System.md"` in the **vault's own** repo only. |
| `handoffs/TASK-1362-programmer.md` | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\handoffs\` | ✅ **YES** | ✅ **This file is the ONLY committable artefact of this row.** Rides `TASK-1361` if I land before that host's dispatch, else the next host, derived at that host's own instant (`SC-§133`). |

⛔ Git root is **one level up** (`SC-§102`) — the pathspec is `GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1362-programmer.md`.

---

## 2. 🚨 SPEC (1) — THE PRECONDITION, MEASURED AT MY OWN INSTANT, STATED AS A DECISION

```
$ grep -c MEASURED CLAUDE.md
3
```

Re-measured at close of work: **still `3`.** The three matching lines (`grep -n`):

```
25:- `.claude/pipeline/qa/TASK-###-verify.md` — ... (`VERIFIED` / `VERIFY-FAILED` / `UNOBSERVABLE` / `MEASURED`; law: CONVENTIONS VER-§)
49:   - **5c** `verified` (or `UNOBSERVABLE`, or `MEASURED`, or no runtime criterion) → ...
66:- Nothing with a runtime acceptance criterion is committed without a VERIFIED report; UNOBSERVABLE and MEASURED are recorded on the row, not treated as a pass.
```

> ⚖️ **THE DECISION: the reading is `3`, which is `> 0`, therefore spec (1) says PROCEED, and I proceeded.**
> I state it as a decision and not as a formality because the branch was live: **`= 0` would have stopped this row dead** and I would have written the handoff saying so. ⭐ **I did not inherit the dispatch's "I measured 3" — the dispatch explicitly told me not to, and I ran the instrument myself.** The instrument decided.

**Spec (2) re-measured rather than trusted (`SC-§91`):** ⛔ **no `Docs/` twin exists.** A recursive grep for `UE5 Agent Team System` under `Docs/` returns exactly one file, `Docs/Aura AI for Unreal — Integration Plan.md`, and it is **not** a twin — it merely wikilinks `[[UE5 Agent Team System]]` at three places (an overview line, its own step 16 "Vault sync" action item, and a related-notes list). The note remains **vault-only**.

---

## 3. ⛔ THE DIFF — BEFORE AND AFTER FOR EVERY LINE CHANGED

**File:** `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\UE5 Agent Team System.md`
**sha256 BEFORE:** `e67b378ada845bce605f35262d3ce63fdc1f9fc96a2361fb83871b852190e3a1`
**sha256 AFTER:**  `d55a8254b8c0d143e31829b031b4481b5b9fb66435f91098c345e563c39b4274`
**Lines:** 337 → **341** (+4 = 2 source-stamp lines + their 2 separating blanks). **2 lines changed in place, 4 inserted, 0 deleted.**

⭐ **METHOD (`SC-§97`): I did not transcribe the source sentences by hand.** The edit script **read `CLAUDE.md` and extracted the spans programmatically** (the `- **5c** ` prefix stripped from rule 5c; the `- ` list marker stripped from the hard-gates line), then substituted those extracted strings into the note. **Byte-identity is therefore guaranteed by construction, not by my typing** — and then verified independently in §4 by a second script that re-read both files from disk.

### Edit 1 — **F7**, the 5c quotation

**BEFORE:**
```
- **`verified` → `integrating` → `done` (5c)** — *"`verified` (or `UNOBSERVABLE`, or no runtime criterion) → invoke `build-master` to assemble and commit as today."*
```
**AFTER:**
```
- **`verified` → `integrating` → `done` (5c)** — "`verified` (or `UNOBSERVABLE`, or `MEASURED`, or no runtime criterion) → invoke `build-master` to assemble and commit as today. `MEASURED` routes *like* `UNOBSERVABLE` but is **not** it: it never blocks and never bounces, and it is earned by a control that discriminated (a controlled negative), whereas `UNOBSERVABLE` means the lane could not see at all."
```
✅ `MEASURED` restored to the route list **and the second sentence taken whole**, per spec (3) — the sentence `TASK-1360` called *"the single most confusable point in the verify lane"*, which is exactly what a reader of this note needs.

### Edit 2 — **F8**, the second hard-gates bullet

**BEFORE:**
```
- *"Nothing with a runtime acceptance criterion is committed without a VERIFIED report; UNOBSERVABLE is recorded on the row, not treated as a pass."* ← **the second gate, and the one the older version of this note was missing.** `UNOBSERVABLE` = *the row has nothing PIE can see* (`CONVENTIONS.md` `VER-§5` cl. 1); it never blocks the commit and it is never read as a pass.
```
**AFTER:**
```
- *"Nothing with a runtime acceptance criterion is committed without a VERIFIED report; UNOBSERVABLE and MEASURED are recorded on the row, not treated as a pass."* ← **the second gate, and the one the older version of this note was missing.** `UNOBSERVABLE` = *the row has nothing PIE can see* (`CONVENTIONS.md` `VER-§5` cl. 1); it never blocks the commit and it is never read as a pass. `MEASURED` is its newer sibling — defined in the 5c bullet above; it is likewise recorded on the row and likewise never read as a pass.
```
✅ The sentence is now quoted **whole** (this is the **F8 judgement** the spec reserved), and `and MEASURED` is inside the marks because the source has it there.

### Edits 3 & 4 — spec (5), the dated source-stamps (one line under **each** *"verbatim"* heading)

**INSERTED** under the § Routing rules heading (*"The three stages after `qa-passed` are the compile/verify/commit chain, quoted verbatim from `CLAUDE.md` § Routing rules…"*):
```
*Source-stamp: quoted from `CLAUDE.md` § Routing rules, as of 2026-09-20. `CLAUDE.md` is a live file — if it moves, this block ages; re-check it against the source rather than trusting the label.*
```
**INSERTED** under the § Hard gates heading (*"**Hard gates**, verbatim from `CLAUDE.md` § Hard gates:"*):
```
*Source-stamp: quoted from `CLAUDE.md` § Hard gates, as of 2026-09-20. `CLAUDE.md` is a live file — if it moves, this block ages; re-check it against the source rather than trusting the label.*
```
⛔ **No line numbers**, per spec (5). Each is **one line and reverts alone.**

> ⚖️ **Spec (5) invited me to say if I think the stamp is wrong. I do not — I think it is right, and I want to record *why* I agree rather than just complying.** The § pointer alternative was declined with a reason and I am **not** re-proposing it. The stamp does not remove the liability and does not claim to; it converts **silent** staleness into **legible** staleness, which is genuinely the most a document can do about a source it does not control. The `TASK-1354` → `TASK-1360` → `TASK-1362` sequence is the proof: this note went stale **twice within days** and both times it took a human-noticed diff to catch it. A dated stamp is what lets the *next* reader catch it in one glance instead.

---

## 4. ✅ ACCEPTANCE — THE MEASUREMENTS, PRINTED

### 4.1 ⛔ STRING-COMPARE: each quoted passage vs its `CLAUDE.md` source line

Independent second script, both files re-read from disk, quoted span extracted by regex from the note and compared to the span extracted from `CLAUDE.md`:

```
========== STRING-COMPARE: each quoted passage vs its CLAUDE.md source ==========
[F7] note 5c quoted span == CLAUDE.md 5c source span : IDENTICAL
     len(note)=355 len(src)=355  spans_found=1
[F8] note hard-gate quoted span == CLAUDE.md hard-gate source : IDENTICAL
     len(note)=157 len(src)=157  spans_found=1
```
✅ **Both byte-identical, equal length, exactly one quoted span per line** (the last clause matters — it proves the comparison covered the *whole* quotation and not a prefix of it).

### 4.2 ⛔ THE FOUR-VS-THREE SEAM — EXPLICITLY RE-CHECKED, **REPORTED CLOSED**

```
mermaid 5c edge  : V -->|verified / UNOBSERVABLE / MEASURED / no runtime criterion| C["build-master — 5c..."]
5c quotation     : `verified` (or `UNOBSERVABLE`, or `MEASURED`, or no runtime criterion) → invoke ...

diagram routes   : ['verified', 'UNOBSERVABLE', 'MEASURED', 'no runtime criterion'] -> count 4
quotation routes : ['verified', 'UNOBSERVABLE', 'MEASURED', 'no runtime criterion'] -> count 4

SEAM: CLOSED - 4 vs 4, identical route names in identical order
```
✅ **CLOSED.** The seam `TASK-1360` declared rather than hid — its Edit 5 relabelled the mermaid edge to four routes while the fenced quotation ~13 lines below still listed three — **is gone.** The diagram and the quotation now agree on **four routes, same names, same order**. ⛔ I closed it **from the quotation side only**: I did not touch the mermaid edge (it is `TASK-1360`'s Edit 5 and fenced by spec (7)).

> ⚠️ **AN HONESTY NOTE ON THIS MEASUREMENT, because my first run of it printed `STILL OPEN` and that was *my instrument*, not the note.** My first parser split the route list on `,` alone, so the source's repeated `or ` left a token reading `` or `MEASURED` `` and the comparison failed on a **string artefact**. I did **not** wave it through as "obviously fine" — I fixed the parser (strip a leading `or `) and re-ran, and *then* it agreed. Recording it because `SC-§104`'s spirit cuts both ways: a red reading deserves the same "is the instrument right?" scrutiny as a green one, and the fix had to be to the parser, in the open, rather than to the verdict.

### 4.3 ⛔ SPEC (4) CONSISTENCY CHECK — **ANSWER: YES, CONSISTENT. NOTHING CHANGED.**

Spec (4) required me to re-read `TASK-1360`'s Edit 3 and Edit 4 — the two F4 repairs that deliberately quote only the **invariant leading clause** of the hard-gates line — and check whether, *with F8 taken*, the note now states that one source sentence **consistently**.

```
Invariant leading clause from source:
  'Nothing with a runtime acceptance criterion is committed without a VERIFIED report'

  note line  74 (Edit 3) : leading clause present=True, inside quotation marks=True
     -> substring of current source sentence: True
  note line  80 (Edit 4) : leading clause present=True, inside quotation marks=True
     -> substring of current source sentence: True
  sites found: 2 (expect 2 = Edit 3 + Edit 4)
```

⚖️ **VERDICT: YES.** The note now states that sentence at **three** sites, and all three are byte-identical substrings of the **one current** source sentence:

| Site | Quotes | Byte-identical to current source? |
|---|---|---|
| line 74 (`TASK-1360` Edit 3) | the leading clause, stopping before the `;` | ✅ yes — an exact prefix |
| line 80 (`TASK-1360` Edit 4) | the leading clause, stopping before the `;` | ✅ yes — an exact prefix |
| line 149 (**my F8**) | the sentence **whole** | ✅ yes — the whole line |

⭐ **This is the point the spec was protecting, and it held:** `TASK-1360` chose the **invariant substring** precisely so that a later row taking F8 would *not* create a contradiction — and it did not. The clause it quoted sits on the **pre-amendment side of the semicolon**, which the amendment never touched, so it was correct before F8 and is still correct after. ⇒ ✅ **I CHANGED NOTHING at lines 74 and 80**, as spec (4) directed on a `yes`. **The manager's ruling that "choosing the invariant substring is the general technique and worth copying" is vindicated by measurement here, not just by argument** — it is *why* this row cost one line instead of three.

---

## 5. 🚩 SPEC (8) — POSSIBLE **F9**, NAMED AND ⛔ **NOT TAKEN**

I ran a read-only audit of **every** quoted span in the note (26 spans ≥ 25 chars), testing each as a substring of `CLAUDE.md`: **13 match verbatim, 13 do not.** ⛔ **All 13 misses are legitimate and are NOT defects** — mermaid node labels (which are syntax, not quotations), example user prompts, Jonathan's own words, and a filesystem path. None of them is attributed to `CLAUDE.md`. **No F9 in that set.**

### 🚩 **F9 (candidate) — two `CONVENTIONS.md` quotations carry no source-stamp and were NOT re-measured by this row**

Two spans are presented as quotations of **`CONVENTIONS.md`**, not `CLAUDE.md`:

- note line 130 — `CONVENTIONS.md` `VER-§1` cl. 1: *"nothing precedes it … so `head -1` of any report IS its verdict"*
- note line 144 — `CONVENTIONS.md` `VER-§6` cl. 5, dated amendment: *"a `VERIFY-FAILED` BLOCKS the commit host and bounces the row to `gameplay-programmer` with the `qa/TASK-###-verify.md` path, as a QA loop: max 3, then escalate to 🧑 him"*

⛔ **I did NOT verify these against `CONVENTIONS.md`, and I am saying so rather than letting silence imply I did.** `CONVENTIONS.md` is on my `names:` fence and is **not** in my `READS:` list; my two source-stamps cover only the two `CLAUDE.md` headings. `CONVENTIONS.md` is **also currently dirty** in `git status`, so it is a moving source with exactly the same liability this whole row is about.

⇒ ⛔ **NAMED, NOT TAKEN** (`SC-§50`, spec (8)). Neither is under a *"verbatim"* heading; line 130 marks its elision honestly with `…`. **This is a lower-grade concern than F7/F8 — the exposure is real but the attribution is not currently known to be false.** A row that wants it would need `CONVENTIONS.md` on its `READS:`. ⭐ The pattern the spec cited has now worked **four** times running.

---

## 6. ⚖️ JUDGEMENT CALLS — DECLARED, NOT BURIED

**JC-1 — I dropped the *outer italics* on the 5c bullet (F7). This is prose formatting OUTSIDE the quotation marks, which spec (6) makes mine; the bytes INSIDE the marks are untouched and verified identical.**
The note's convention is `- **label** — *"quote"*`. The newly-quoted second sentence **contains its own emphasis**: `` `MEASURED` routes *like* `UNOBSERVABLE` ``. Wrapping a span containing `*like*` inside a `*"…"*` italic run **breaks the render**: CommonMark pairs the opening `*` with the `*` before `like`, so Obsidian would show the first fragment italic, `like` plain, then re-open — garbled emphasis, on a note whose 🧑 Obsidian render check is **already open**. Dropping the outer `*` renders the source's own `*like*` and `**not**` exactly as the source intends. ⛔ **Cost: bullet 5c is no longer italic while its 5a/5b siblings are.** I judged a visible style difference strictly better than a visible render defect, and spec (6) explicitly permits quoting the sentence whole and putting summary prose outside the marks. ⚖️ **If the manager prefers uniform italics, the fix is to re-wrap — but do NOT escape the inner asterisks, because that changes bytes inside the marks and re-breaks the verbatim claim.**

**JC-2 — I extended the F8 bullet's trailing gloss by one sentence to define `MEASURED`.**
The gloss is **outside** the marks (after the `←`), so spec (6) makes it mine. My edit put `MEASURED` **inside** the quotation, and the existing gloss defined only `UNOBSERVABLE`; leaving it would have introduced a token at the exact spot where the note explains its sibling and then not explained it — a reader defect **I** would have authored. The addition is minimal, additive, contradicts nothing, and **points at the 5c bullet above rather than restating the definition**, so there is only ever one authoritative copy. ⛔ I did **not** otherwise touch `TASK-1354`'s hard-gates block.

---

## 7. ⛔ FENCES — WHAT I DID NOT DO

- ⛔ **`CLAUDE.md`: READ ONLY. I did not author it and I did not stage it** (both verbs, per `SC-§139` cl. 6). It was **already dirty at my session start** — that dirt is `TASK-1359`'s and its git act is `TASK-1361`'s. I quoted from it by extracting spans programmatically; I opened no write handle to it.
- ⛔ **`TASKBOARD.md` · `CONVENTIONS.md` · `Source/**` · `Saved/**` · the `.uproject` — untouched.** I located the spec **by quoted text**, never by line number.
- ⛔ **No compile, no suite, no commit, no push, no git write of any kind.** No `git add`. My only git calls were **read-only** `status` / `ls-files`.
- ⛔ **I did not touch the editor.** `TASK-1357` (playtest-verifier, PIE) was running in parallel in a different lane and I stayed out of it entirely.
- ⛔ **Spec (7) fenced elements untouched:** the roster and tool-layer tables (`TASK-1231`'s), the lifecycle line and the **mermaid** (`TASK-1354`'s + `TASK-1360`'s Edit 5), the F2 count, the F3 bullets and the F4 repairs (`TASK-1360`'s). The only contact with (7) was the **consistency CHECK at (4)**, which is a **read**, and it returned `yes` ⇒ I rewrote nothing.

### 🧑 On Jonathan's open Obsidian render check

⛔ **I do not close it and I do not assert it.** It remains open (`TASK-1354`, re-owed by `TASK-1360`). **I touched no mermaid block on this row, so I add no new reason to doubt it.** Nothing more.

---

## 8. 🔍 WHAT THE MANAGER SHOULD SCRUTINISE

1. **JC-1 (the dropped italics)** — the one place I chose render-correctness over style-consistency. Reversible in one edit; the fence on *how* to reverse it is stated above.
2. **JC-2 (the added gloss sentence)** — prose I authored adjacent to a `TASK-1354` block. If the manager reads that block as wholly fenced, this sentence is the thing to strike.
3. **The F9 candidate (§5)** — two unverified `CONVENTIONS.md` quotations. Worth a row or worth a shrug; that call is the manager's, not mine.
4. **§1.2's coupling** — if 🧑 Jonathan revokes the `MEASURED` amendment, **this row reverts with it.** Four edits, all in one vault file, all listed in §3.

**Status:** ⛔ **left as the manager wrote it — I do not flip my own row (`SC-§134` cl. 7(b)). Ready for the manager's flip.**
