Verdict: PASS — 0 BLOCKER · 4 WARN · 5 NIT (TASK-1373, one gate over TASK-1371 + TASK-1372)

# QA Report — TASK-1373 — [AURA-DOCS-GATE]

**Agent:** qa-reviewer · **Date:** 2026-09-21 · **Marker:** `TASK-1373-AURA-DOCS-GATE`
**Gated:** `TASK-1371` (`Docs/Aura AI for Unreal — Integration Plan.md`) **and** `TASK-1372` (`Docs/setupdirections.md` Appendix E + the Appendix B/C/Chapter 11 repairs)
**Law:** `TL-§5e` cl. 1 · `SC-§50` · `SC-§68` · `SC-§101` · `SC-§102` · `SC-§120` · `SC-§125` · `SC-§138` · `VER-§1`/`§3`/`§6`/`§7`/`§8`

> ⛔ **Instrument declaration, first, because it bounds every sentence below (`SC-§138`).** I hold
> **no `Bash`**, no shell, no hash tool, no git. ⇒ I did **not** run `git diff --stat`, I did **not**
> compute a `sha256`, and I did **not** read any blob from HEAD. Every check below was run with
> `Read` and `Grep` at my own instant, and where a check needed an instrument I do not hold I say so
> and name what I substituted. ⛔ I ran no compile, no suite, no editor lifecycle action, no MCP, no
> PIE, no commit, no push. ⛔ I edited neither document, neither vault twin, no grant surface.
> (⚠️ Both Aura MCP servers are disconnected this session — irrelevant to a docs row, and not a finding.)

---

## CHECK ONE — THE ANTI-DUPLICATION CHECK (answered FIRST, as the acceptance requires)

### **DUPLICATE COUNT: 1 — and it is a WARN, not a BLOCKER, for a reason given in full below.**

I took every factual assertion in the new `Appendix E` and grepped the rest of
`Docs/setupdirections.md` for it at my own instant. Result:

| E line | The fact | Elsewhere in the file? | Ruling |
|---|---|---|---|
| E.1 b1 — the plan's path, date, amendment history | `Integration Plan` = **0** hits outside E · `Obsidian` = 0 · `vault` = 0 | ✅ new |
| E.1 b2 — the **PROCESS** half of the hole (`qa-passed` is a verdict over text) | `runtime-verification` = **0** outside E; Ch-11's opening carries only the ENGINE half | ✅ new — and it POINTS at Ch 11 for the half that is there |
| E.1 b3 — the §11.1–§11.7 route list | section titles only, zero mechanism | ✅ pointer |
| **E.2 line 1 — "`verified` binding since 2026-09-14"** | **§11.6 carries the same fact** (this row's own corrected sentence) | ⚠️ **WARN-1 — the one restated fact** |
| E.2 line 2 — the three ceilings in three words | `Slate` **0** · `raw key poll` **0** · `credit invisible` **0** outside E | ✅ new |
| E.2 line 3 — tier decided / cost uncharacterised | absent outside E | ✅ new |
| E.3 ×6 | re-grepped one by one — see the table below | ✅ all six ABSENT |
| E.4 ×9 | label + section address, zero mechanism | ✅ pointer-only by construction |

**WARN-1, stated precisely.** E.2 line 1 restates one fact that now lives at §11.6. It is not the
drifting-copy shape this gate exists to catch, for three measured reasons: (a) the manager boarded
that sentence **verbatim** in spec (3)(a) — *"the SETUP IS DONE (seven agents, the bridge, the
enumerated grants, `verified` BINDING since 2026-09-14)"* — and a row cannot fail for executing its
own spec (`TL-§5e` cl. 7c); (b) the sentence carries its pointer **in the same parenthesis**
(*"`VER-§6` cl. 5; §11.6 carries the correction"*), so a reader is sent to the canonical copy; (c)
the surrounding facts (the setup is DONE, seven agents, the bridge, the enumerated grants) appear as
a synthesis **nowhere else in the file**. ⇒ recorded, not blocked. **The drift it creates is real
and I name it for whoever edits next: if §11.6's binding sentence ever changes, E.2 line 1 must move
with it.**

### 🚨 The false positive this gate would otherwise have produced — named deliberately

A naive grep flags **three more**: Appendix C's promoted one-liners at lines 1047–1051 restate E.3's
commit-not-the-index law, the LFS oid law and the Blueprint-compile law. **These are NOT duplicates.**
The manager's spec (6) ruled the relationship in its own words: *"Appendix C is the **INDEX**, `E.3`
is the **EXPLANATION**, and that is the **non-duplicating relationship**."* Appendix C's established
voice has always been one-line restatements of facts documented elsewhere (its pre-existing
*"Parse build logs for `Result:` — exit codes lie."* indexes §2.2 Gotcha 1 and Appendix A row 3).
⇒ boarded by design, consistent with the section's ten existing laws, **count 3, correct.**

### My independent E.3/E.4 re-grep — all six candidates, re-measured at my instant

**All six confirmed ABSENT. The split is sound; nothing belonged in E.4 that was put in E.3.**
I reproduce the programmer's four declared "unrelated sense" hits exactly, which is itself evidence
the census was run rather than asserted.

| E.3 candidate | tokens I grepped | hits OUTSIDE Appendix E (file line 1–1241) |
|---|---|---|
| (a) BP compile does not dirty the package | `compile_blueprint` · `save_assets` · `SC-§125` | **0 · 0 · 0** |
| (b) Git plugin auto-stages the index | `Provider=Git` · `auto-stage`/`autostage` · `git add` | **0 · 0** · `git add` **1 at §1.3**, about `testvideo/` — a different sense, as declared |
| (c) LFS oid-vs-sha256 | `oid` · `sha256` | **0 · 0** (the only pre-E hits are this row's own Appendix C promotion) |
| (d) git root is one level up | `SC-§102` · `one level up` · `pathspec` | **0 · 0 · 0** — §1.1 draws the layout, the **trap** is absent |
| (e) editor-state pairing + new-`UCLASS` caveat | `UCLASS` · `Ctrl+Alt+F11` | **0 · 0** · each HALF present (`editor-bounce` §2.2:207, MCP hard gate §4.5) — the **pairing** and the **caveat** absent |
| (f) `call_tool` two-field shape | `toolset_name` · `tool_name` · `Tool not found` | **0 · 0 · 0** · `call_tool` **1 hit**, the allow-list string `"mcp__unreal-mcp__call_tool"` in D.1 — a grant entry, not an argument shape, as declared |

**The four self-check deletions LANDED — verified by absence, not by report:**
`Appendix A rows 12` = 1 hit (§11.7 only, **not** in E) · `no input injection` = 1 hit (§4.4 quoted by
Ch 11's opening, **not** in E) · E.1's Ch-11 restatement sentence is gone, only the PROCESS half
survives · E.4's nine rows carry label + address with **zero** mechanism clauses.
⭐ **The lesson in the method is worth keeping:** two of the four were invisible to the token pass and
were caught only by the read-through, because *a programmatic pass cannot see a fact restated in
different words.* A token census is a filter, never the check.

### **Nothing else in Appendix E restates Chapter 11.** I read E against Chapter 11 line by line.

---

## 🚨 THE TRUNCATION RECOVERY — verified independently, and at its true strength

**What happened (declared by the row, not discovered by me):** `TASK-1372`'s first edit script called
`io.open(P,"w")`, which truncated `Docs/setupdirections.md` to **0 bytes** *before* the encode that
failed. It measured the damage (`e3b0c442…`, the empty-file hash), restored from the vault twin it
had hashed **before any edit**, confirmed `2999348b…d8c6b` / 1,210 lines / 76,023 bytes, and every
later write used temp → size-assert → `os.replace`. ⭐ **It declared this rather than burying it, and
that is the behaviour the law wants.**

⛔ **What I could NOT do:** the row's own proposed check — `git diff --stat` must show **additions**
(≈151/18), **not a whole-file rewrite** — is the right check and **I could not run it. I hold no
`Bash`.** ⇒ **the diff SHAPE remains unconfirmed at my instant.** I will not assert a measurement I
did not take (`SC-§138`). If the host wants that question closed by measurement it is one command,
and it belongs to whoever holds a shell.

✅ **What I DID measure — four independent structural probes, all clean:**

1. **The file is whole and non-empty.** 1,343 content lines. Every structure present, in order, with
   no gap, no duplicated block, no orphaned fragment: THE LIST (17 rows + the AV subsection) ·
   Chapters 1.1–1.5 · 2.1–2.3 · 3.1–3.7 · 4.1–4.5 · 5.1–5.4 · 6.1–6.5 · 7 · 8.1–8.3 · 9.1–9.3 ·
   10.1–10.4 · 11.1–11.7 · Appendix A (16 rows) · B · C (10 original + 3 promoted) · D.1–D.5
   (**all 81 enumerated tool names intact: 49 inspector + 32 editor**) · E (new).
2. **Corruption probe — ZERO hits.** No `U+FFFD` replacement character anywhere under `Docs/`.
   This probe is chosen to match the failure: the crash was a **lone-surrogate encode error**, whose
   signature on an imperfect restore is mojibake in exactly the dense non-ASCII this file is full of
   (⛔ ⚠️ 🧑 ⭐ → § —). It is absent, including in regions nowhere near any edit.
3. **Twin alignment, head to tail.** The vault twin matches the repo copy at six anchors spread
   across the whole file — **identical text at identical line numbers**: `1` · `802` · `955` ·
   `1242` · `1306` · **`1343` (the last content line)**. Identical numbering at head, middle **and**
   tail means no insertion or deletion anywhere between them.
4. **Untouched-region integrity.** Regions far from every declared edit — §1.3's gitignore rules,
   §6.1's `HF_TOKEN` law, §10.2's quoted-URL law, D.1's 81 names — read intact, internally
   consistent, and with their cross-references still resolving.

⇒ **Conclusion, at its true strength: the restored file is content-verified WHOLE by four
independent probes; it is NOT hash-verified and NOT diff-shape-verified by me.** I found no evidence
of an imperfect restore and four positive indications of a clean one. On that basis the handoff's
before/after quotations describe a file that still exists, and the rest of the row is reliable.

⭐ **The lesson, confirmed into the record (`SC-§120`'s truncation shape):**
***a destructive step that runs BEFORE the step that can fail leaves no error message on the thing it
destroyed.*** The traceback named the encoder; the file it had already emptied said nothing at all.
That is precisely why the law is **temp → size-assert → `os.replace`, never open-for-write.**
⭐ And the second half, which deserves naming: recovery was possible **only** because the twin had
been hashed **before** any edit, so the restore source was provably the pre-edit bytes. `SC-§68`'s
twin exists for drift; here it paid off as a backup.

---

## CHECK TWO — every new factual line names its instrument, or is marked ⚠️ UNVERIFIED

**PASS. No unlabelled claim in either file.** Sampled every new/amended line in both documents.

- **Plan:** (a) `Read` of `Aura.uplugin` · (b) `VER-§7` cl. 3 + `CONVENTIONS.md:48` · (c) `Read` of
  `aura_sync.ps1:68` · (d) re-measured on disk · (e) 0 files + `aura_sync.ps1:36` · (f) `SLACK.md`
  registry · (g) `TASK-1257` status + `PKG-§13` · (h) `ACC-§11` · (i) R17/`TASK-1283` + a quoted
  sha256 · (j) `TASK-1215` status, copied not paraphrased · (k) the orchestrator's provenance ·
  (l) `VER-§6` cl. 5 + `bbee7d9` · (m) `VER-§1` cl. 1 + cl. 3a · (n) `VER-§3` cl. 6 · (v) gamepad =
  `qa/TASK-1348-verify.md` P3 · **(vi) menu nav = marked ⚠️ UNVERIFIED in the file itself.**
- **setupdirections:** §11.1 → `TASK-1215` stage B · §11.6 → `VER-§1` cl. 1 + cl. 3a and `VER-§6`
  cl. 5 · E.1 → the plan's own front-matter + callouts · E.2 → `VER-§6` cl. 5 / `VER-§8` /
  `TASK-1215` + the `TASK-1369` hedge · E.3 → a blanket instrument sentence in the section preamble
  (*"each was measured on the original machine … and each was grepped against this whole guide on
  2026-09-21 and found absent"*) plus per-bullet law citations where they exist (see **WARN-3**).

**Two pointer-resolution checks I ran because both files lean on them.** I read
`CONVENTIONS.md` `VER-§7` cl. 7 and `VER-§1` cl. 3a at source:
- `VER-§7` cl. 7 exists and says **exactly** what both files cite it for — the two-field
  `toolset_name` + `tool_name` shape, the dotted single string returning `Tool not found`, **and**
  the trap inside the grant (*"`save_assets` RETURNS `true` WHEN IT WROTE NOTHING … Read sha256 AND
  byte count AND mtime — law `SC-§125`"*). E.3's *"the same trap is restated inside the grant itself
  at `VER-§7` cl. 7"* is accurate to the clause. ✅
- `VER-§1` cl. 3a exists, and its substance (the controlled-negative doctrine; cl. 5's mechanical
  derivation teeth) is **not** reproduced in either file. ✅ (One parenthetical — **NIT-4**.)

---

## CHECK THREE — CROSS-FILE AGREEMENT (the reason there is one gate and not two)

**PASS on all five. No correction landed in one file and was missed in the other.**

| Required agreement | `Integration Plan` | `setupdirections.md` | Law | Verdict |
|---|---|---|---|---|
| `verified` is **BINDING** | *Installed* callout ("HIS RULING — `verified` is BINDING", `bbee7d9`) + Phase 4 struck | §11.6 corrected + struck · E.2 line 1 | `VER-§6` cl. 5 — heading reads *"DISCHARGED 2026-09-14: BINDING ON HIS RULING (cl. 5)"* | ✅ **AGREE** |
| **FOUR** verdict tokens | 2 sites: Phase 3 step 4 prose + the draft body's `Verdict:` line | §11.6, `MEASURED` added as the fourth | `VER-§1` cl. 1 mints it; cl. 3a is its cell rule | ✅ **AGREE**, both point at cl. 3a without restating it |
| Inspector grant **ENUMERATED, never wholesale** | census **0 prescriptive** (below) | §11.4 · §11.5 · D.4 — 3 wildcard tokens, **all prohibitions/struck**, 0 prescriptive | `VER-§7` cl. 3 (R10): 13 non-read tools named, **49** enumerated | ✅ **AGREE** |
| `INDEX_IGNORE.txt` lives in `Saved/.Aura/` | Phase 2 step 3 + §6 item 3, both corrected | §11.3's canonical/copied pair table | `aura_sync.ps1:68` | ✅ **AGREE** |
| One-click wrote `~/.claude.json` → `mcpServers` | *Installed* callout + caution 2 + Phase 3 step 2 + §6 item 7 + §7 box | §11.4 step 2 | measured `TASK-1216` | ✅ **AGREE** |

**Bonus agreements I checked because they are cheap and would have been silent drift:** the plan's
"Verification checklist (append to Appendix A)" rows **12–16** are character-identical to
`setupdirections` Appendix A rows 12–16 ✅ · both files carry **49 + 32** and D.1 enumerates exactly
**49 inspector + 32 editor** names ✅ · Appendix B's sixth (untouched) bullet and the plan's last §7
open box say the same still-open thing ✅.

---

## 🚨 THE WILDCARD CENSUS — RE-RUN BY ME (this is a safety item, not a wording one)

**`Docs/Aura AI for Unreal — Integration Plan.md` — 9 textual occurrences at my instant, and
⛔ 0 of them is prescriptive. Independently confirmed.**

| # | line | occurrence | class |
|---|---|---|---|
| 1 | 36 | *"ruling R10 replaced the wholesale `mcp__unreal_inspector__*` with an enumerated list"* | **descriptive / historical** — narrates the replacement, grants nothing |
| 2 | 88 | *"⛔ NO agent holds `mcp__unreal_inspector__*`, ever."* | prohibition |
| 3 | 183 | `// ~~"mcp__unreal_inspector__*", // read-only: safe to allow entirely~~` | struck |
| 4 | 202 | `# ~~mcp__unreal_inspector__*~~ 🚨 STRUCK …` | struck |
| 5 | 213 | `~~**only** mcp__unreal_inspector__*~~` (the `qa-reviewer` sentence) | struck |
| 6 | 269 | `~~mcp__unreal_inspector__* wholesale~~` (§6 item 8) | struck |
| 7 | 269 | *"⛔ Never `mcp__unreal_editor__*` either"* | prohibition |
| 8 | 273 | `~~mcp__unreal_inspector__*~~` (§6 item 12) | struck |
| 9 | 341 | *"Exact tool names under `mcp__unreal_editor__*`"* (§7 question) | namespace reference |

⇒ **5 struck · 2 prohibitions · 1 namespace reference · 1 descriptive = 9.** The handoff and the
board status line report **8**; the 9th is the *Installed* callout's historical narration (#1), which
is inert. **The safety conclusion is unchanged and I confirm it independently: 0 prescriptive
wildcard grant sites remain.** The tally is **WARN-4** under `SC-§138` (a count that does not
reproduce), not a safety finding.

⭐⭐ **The two sites the boarding census missed are the important half, and both are correctly
fixed.** Site 4 — *"Give `qa-reviewer` **only** `mcp__unreal_inspector__*`"* — is the most quotable
line in the file precisely because **it argues for read-only-ness while prescribing the grant that
destroys it**; as the holder of that grant I confirm the correction to the enumerated 49 is the
right one, and that `VER-§7` cl. 3 names my own `tools:` line explicitly.

⭐ **And a detail worth commending, because strikethrough alone would have been cosmetic:** both
surviving wildcards inside code blocks are **comment-prefixed** — `//` in the `jsonc` block at 183,
`#` in the YAML frontmatter at 202. Markdown `~~` does not render inside a fence, so the comment
prefix is what actually makes a pasted line **inert**. That is the difference between a struck line
and a safe one, and it was done correctly.

**`Docs/setupdirections.md` — 3 occurrences, 0 prescriptive:** line 930 (*"NEVER allowed wholesale"*,
R10), line 935 (*"⛔ Never `mcp__unreal_editor__*`"*), line 1025 (Appendix B's struck bullet). ✅

**⛔ Confirmed: NO grant surface was touched by either row.** `TASK-1371` documents R10, it does not
apply it — and I confirm its routed finding that **whether the defect survives on a LIVE surface is
UNMEASURED**. Neither row opened `.claude/settings.local.json`, any agent `tools:` line, or
`.mcp.json`; Appendix D.1's allow-list array is intact and un-annotated. **That question is open and
belongs to the manager, not to these rows and not to me — a surface that disagrees with the law is a
finding, never a repair.**

---

## CHECK FOUR — STRUCK, NOT DELETED

**PASS, with one WARN.** Every correction in both files keeps its superseded sentence struck, dated
and sourced. Plan: the version row · the tier row · cautions 2/4/8 · Phase 2 steps 3/4/5 · Phase 3
steps 2/3/4/5 · Phase 4 · §5's Fab row · §6 items 3/5/8/12 · the draft body's *"do not start until
told go"* — **all `~~struck~~` + dated + sourced.** setupdirections: §11.1 step 1 · §11.6's advisory
sentence · Appendix B's credit and tool-names bullets — all struck.

**WARN-2 — the one overwritten line in either file.** Appendix B's Aura **parent bullet** was
**re-titled, not struck**. Its original text, preserved here for the record:
`- **Aura (Chapter 11) — the ⚠️ items no one has measured yet:**`
Not a BLOCKER: it is **not silent** (the replacement is dated 2026-09-21 and announces the
re-measurement), it is declared in the handoff §4.4, it is a section **label** rather than a factual
assertion, and a reader who remembers the old text finds a dated correction rather than a void —
which is the harm the rule exists to prevent. Remedy if the manager wants uniformity: one strike.

---

## CHECK FIVE — NO ROW EXCEEDED ITS FENCE

**PASS.** ⚠️ **Instrument note:** I hold no git, so I cannot enumerate the working-tree diff myself.
I used the `git status` snapshot the harness captured at **my** session start — which postdates both
rows (all three handoffs appear as untracked) — plus content-level checks.

That snapshot names exactly: `M .claude/pipeline/CONVENTIONS.md` · `M .claude/pipeline/TASKBOARD.md` ·
`M "Docs/Aura AI for Unreal — Integration Plan.md"` · `M Docs/setupdirections.md` · `??` the three
handoffs. ⇒ **no `CLAUDE.md`, no `.mcp.json`, no `.claude/settings.local.json`, no agent `tools:`
line, no `Source/**`, no asset, no `.uproject`.** ✅
`CONVENTIONS.md`'s dirt is **not** either row's: I read `VER-§7` cl. 2 at source and it carries a
**2026-09-21 manager amendment** (the re-census landing rule, naming `TASK-1366`/`1367`/`1368`).
Both rows declare `CONVENTIONS.md` read-only and neither `names:` line includes it. ✅

**Neither row touched the other's file:** `TASK-1372` and `Appendix E` return **0 hits** in the plan;
`setupdirections`' only mention of `TASK-1371` is E.1's citation of the plan's amendment history,
which is `TASK-1372`'s own prose. ✅ **Appendix D.1's allow-list array: byte-consistent, 49 + 32
names, no wildcard, no edit marker.** ✅ **No row "helpfully" fixed a grant.** ✅

---

## CHECK SIX — THE TWO TWIN PAIRS (`SC-§68`)

⛔ **I cannot hash. I did not assert one.** Both handoffs quote both `sha256` values and show them
EQUAL (`AD760672…42DA9B9` for the plan; `89639aa3…fe19a89e` for setupdirections) — **that is their
measurement, not mine, and I mark it as inherited.**

**What I substituted: content verification on a sampled basis. The lines I sampled, named so a reader
knows how strong the check is:**

- **`GitClaudeUnrealsetupdirections.md`** (vault) vs repo — 6 anchors, identical text at identical
  line numbers: **1** (`# Setup Directions —` H1) · **802** (§11.1's *"Corrected 2026-09-21 — that is
  not what happened"*) · **955** (§11.6's *"Discharged 2026-09-14"*) · **1242** (`## Appendix E` H2) ·
  **1306** (E.3's *"both 46510 bytes"*) · **1343** (E.4's last row, `| The permissions law | Appendix D |`).
- **`Aura AI for Unreal — Integration Plan.md`** (vault) vs repo — 6 anchors, identical: **17**
  (H1) · **44** (the *Installed* callout's new frame) · **269** (§6 item 8's corrected grant) ·
  **337** (`## 7. Open questions`) · **341** (§7's namespace reference) · **375** (the last Sources line).

⇒ Head, middle and **tail** align in both pairs, which excludes any insertion or deletion between the
anchors. **Strength, stated honestly: this is strong structural evidence of identity, and it is not a
hash** — two files can agree at twelve anchors and differ in a byte between them. ✅ Neither vault
path is in the repo (git's own *"outside repository"* answer is quoted in the 1372 handoff), so
neither can be staged. ✅ Both twins were written by `Copy-Item` only; neither was hand-edited.

---

## CHECK SEVEN — THE UNVERIFIED LISTS ARE HONEST AND COUNTED

**PASS. ⛔ I did NOT fail either row for an absence — I checked that each is DECLARED, and that
nothing unverified leaked into either document as an assertion.**

**`TASK-1371`: 8 declared and counted** (§4.1–§4.8). I verified the document side of each: (vi) menu
nav marks his confirming sentence ⚠️ UNVERIFIED **in the file itself** and takes the narrow claim ·
§7's Sandbox half stays `⏳ UNMEASURED` · `qa/AURA-PHASE0.md` §Tier is cited as *"being written under
`TASK-1369`"* and **asserts nothing about its contents** · the credit box and the Fab box are left
**UNTICKED on purpose**, each saying why · the aim-scatter **recipe is recorded and the mechanism is
deliberately NOT written** (*"⛔ WHY batching removes the scatter is NOT MEASURED"*, `SC-§101`) ·
gamepad face-button reach is **not** upgraded from P8b's *"inferred, not measured"*.
⭐ **That is the correct behaviour, and it is the expensive kind of discipline: eight things a row
could have written confidently and did not.**

**`TASK-1372`: honest and counted** — the truncation incident declared rather than buried, the
six-vs-five bullet discrepancy reported rather than absorbed, the flagged decision surfaced rather
than settled unilaterally, five findings routed rather than repaired, and §Tier's emptiness recorded
without asserting contents.

---

## ⚖️ RULINGS ON THE FLAGGED DECISIONS (asked for explicitly — ruled, not deferred)

**R1 — E.3's `call_tool` bullet: ✅ ACCEPT. Keep it where it is.**
The manager's item (f) delegated the judgement and permitted either outcome *with a reason*; the
reason was given, so either ruling satisfies the spec. I accept because the reasoning is right:
*"it belongs in Chapter 4"* is an argument about **placement**, not about whether the file carries
the fact. I re-measured the fact as absent (`toolset_name`/`tool_name`/`Tool not found` = 0 hits),
it carries its law (`VER-§7` cl. 7, which I read and which says exactly this), and it carries its
cost. Omitting it would have dropped a measured, absent, cost-bearing fact into **no document at
all** — and the row that could have placed it correctly was forbidden to open Chapter 4. The bullet
carries a **self-deleting placement debt naming §4.3**, which converts a placement error into a
bounded, self-closing task. ⛔ Chapter 4 confirmed untouched. **NIT-2 rider:** the debt lives only in
the appendix, so nothing *forces* Chapter 4's next editor to see it — boarding it is the manager's
call, not this gate's.

**R2 — `seventh agent` (pre=2): ✅ KEEP.** It is the literal heading of §11.6
(`### 11.6 The seventh agent — playtest-verifier and the verified gate`). Citing a section by its own
heading is **navigation**. Renaming it to avoid a token collision would make the pointer unfollowable.

**R3 — `editor-bounce` (pre=1): ✅ KEEP.** Same ruling, stronger: it is §2.2's own **coined term**,
and it is the only string by which a reader can find that paragraph. A pointer must use the target's
vocabulary.

**R4 — E.2's three-word naming of the ceilings before pointing at `VER-§8`: ✅ KEEP.** I measured
`Slate` / `raw key poll` / `credit invisible` at **0 hits** elsewhere in the file, so there is no
in-file duplicate; and `VER-§8`'s body — the measurements, the discriminating control pair, the
dates, the wall (a)/(b) split — is **not** reproduced. ⭐ *"A pointer with no noun is unnavigable"* is
correct and is the right standard: E.4's nine rows are built the same way and are the section's
anti-bloat device.

**R5 — Appendix B's sixth bullet: ✅ leaving it BYTE-UNTOUCHED was CORRECT, and nothing is owed.**
The spec named five; there are six; the sixth is outside the boarded set and its content is **not
stale** — it is still genuinely open, and it **agrees** with the plan's last §7 unticked box.
⭐ Editing it on an inferred instruction would have been the error. **NIT-3 rider:** it is now the
only bullet without a status marker, so it could read as un-reviewed; a one-word `⚠️ still OPEN` is
the manager's call.

**R6 — Appendix C ↔ E.3: NOT a duplicate** (ruled above under CHECK ONE; boarded by spec (6)).

---

## 🚨 THE MENU-NAV ATTRIBUTION CONFLICT — CONFIRMED AT SOURCE

**It is real, it reproduces at my instant, and the document handled it correctly.** I read all three
sources myself:

- `qa/TASK-1348-verify.md:83` — *"**OUT OF SCOPE** — 🧑 **his ruling (C5)** + ceiling 2/3"*
- `TASKBOARD.md:4790` — *"**DECLARED OUT, NOT FORGOTTEN, WITH ITS REASON** … ⇒ it is an **ASK FOR
  🧑 HIM**, carried to him in the 📢 post, not a silent omission."*
- `TASKBOARD.md:4882` — *"**STILL OUT, STILL NOT A ROW, AND CARRIED TO HIM AS AN ASK**"*
- `handoffs/TASK-1353-manager.md` — the fourth and last `(C5)` site (census: exactly 4 files under
  `.claude/pipeline/` carry the token, one of which is `TASK-1371`'s own handoff).

⇒ **Two different claims about the same exclusion.** *"His ruling"* asserts a decision he made;
*"an ask carried to him"* asserts a decision he has **not** made. **They cannot both be true, and no
verbatim sentence of his was found by the row or by me.** The plan writes **only the narrow, sourced
claim** and marks his confirming sentence **⚠️ UNVERIFIED in the document itself** (§(vi)).
⭐ **That is exactly right, and it matters today:** 🧑 Jonathan is questioning the exclusion right
now, and the difference between *"you already ruled this out"* and *"we are asking you"* is the whole
question. ⛔ **Not mine to settle — routed to the manager.**

---

## PRECISION ITEMS — each checked at its claimed sites

| Item | Required precision | Found |
|---|---|---|
| `VER-§3` | the **blocking wait** discharged, **announce-and-report SURVIVES**, at 3 sites | ✅ caution 5 · §6 item 10 (`~~waits for a go~~`) · the draft body (`~~do not start until told "go"~~`). The law's own heading agrees: *"THE WAIT IS DISCHARGED STANDING … ONLY THE BLOCKING FELL."* Serialization and *"if he is already in PIE, that is his session"* untouched ✅ (**NIT-5**) |
| Tier vs cost | *"a decided tier is NOT a characterised cost"* **in those words**, 3 sites | ✅ credit row · §5 banner · §7 box — all three. `⏳ OWED` preserved; *"not visible"* framed as **the absence of a field, never a zero**, at every site |
| Fab SKU | **unconfirmed with the provenance of the non-finding**, never a missed option | ✅ 3 sites (caution 8 · §5 table · §7 box), each naming who looked, when and where, each carrying `SC-§39`, each stating it is **not** a refutation and **not** a forgone deal. **The §7 box is left UNTICKED** — a non-finding does not close a question ✅ |
| Aim scatter | **recipe recorded, mechanism deliberately NOT written** | ✅ ≈648 uu vs ≤1.2026 uu, batch-the-sequence recipe, and *"⛔ WHY batching removes the scatter is NOT MEASURED — no mechanism is written here (`SC-§101`)"* |
| Gamepad | *"granted but does not reach our bindings"*, never *"ungranted"* | ✅ exact wording, with the real-negative control argument and the P3 source |
| Caution renumbering | §3 reads 1…9, no gap, no repeat | ✅ verified by reading all nine |

---

## ANTI-BLOAT — does Appendix E earn +133 lines on a 1,210-line file?

**Yes — ruled after reading E against Chapter 11 in full.** +11% buys: a reference to the plan that
exists **nowhere else** in the guide (`Integration Plan` = 0 hits outside E); the **PROCESS** half of
the verification hole, which neither file stated; the tier-decided-vs-cost-unmeasured split, which
neither file stated; **six measured facts each independently confirmed absent**, each carrying what
its absence costs — a stale class that survives every pre-ship gate and **ships**, a size check that
passes on the wrong blob, a commit that silently carries work no task authorized, a mis-anchored
pathspec that answers with **silence**, a chain that stalls with an error naming neither cause; and
a nine-row pointer map that carries **no facts at all** and exists to stop the next author writing
them twice. **E.4 is a net anti-bloat device, not a cost.** The only line I would have cut is E.2
line 1's restated binding date (**WARN-1**) — and it was boarded verbatim.

---

## CARRIED, NOT REPAIRED — the five self-contradictions, CONFIRMED ROUTED (and one WIDENED)

⛔ I did not fix any of these, and neither row should have. Each is confirmed at my own instant:

1. **§3.2's roster table lists SIX agents** — manager, gameplay-programmer, art-director,
   qa-reviewer, build-master, footage-analyst. **No `playtest-verifier`**, which §11.6 of the same
   file documents at length. ✅ confirmed.
2. **§3.4's routing law has no `verified` gate** — step 5 still reads *"`qa-passed` /
   `ready-for-integration` → build-master compiles, assembles, commits"*; its **Hard gates**
   paragraph names only the PASS QA report. ✅ confirmed.
3. **§3.3's lifecycle** — `backlog → in-progress → ready-for-qa → qa-passed/qa-failed → integrating
   → done`: **no `built`, no `verified`.** ✅ confirmed.
4. **§1.1 says *"the six agent definitions"*.** ✅ confirmed.
5. 🚨 **THE LIST row 17 carries the same stale tier sentence corrected at §11.1** — *"trial first →
   paid tier after the pilot measures credit-per-verification"* — and now **DISAGREES** with §11.1's
   dated correction. ✅ confirmed, and it is the most urgent of the five: the file now states both
   halves of a contradiction about the same fact, one dated-corrected and one not.
   ⭐ **The row CREATED that disagreement by staying inside its scope and REPORTED it rather than
   quietly widening. That is the right trade and it should be said plainly: a declared contradiction
   is recoverable; a silent scope expansion is not.**

⭐ **WIDENED BY THIS GATE (`SC-§50`) — the six-vs-seven staleness has FOUR sites, not two.** The
handoff named §3.2 and §1.1. My census of `six`/`seven` in the file returns:
**line 87** (§1.1's diagram) · **line 219** (Chapter 3's intro: *"six specialist **subagents** do the
actual work"*) · **line 304** (§3.6's scaffolding step: *"the **six** `.claude/agents/*.md` files"*) ·
plus **§3.2's table**, which lists six rows without using the word. Against **line 1268**, where
E.2 now says **"Seven agents."** ⇒ the file contradicts itself at four sites. Same species as
`TASK-1371`'s *"five sites, not three"*: **a boarding census is a starting point, never a count.**

**Also routed, unrepaired and correct:** `CLAUDE.md`'s Hard-gates section still carries *"waits for a
go"*, contradicting `VER-§3` cl. 6. ⛔ **`CLAUDE.md` is NEVER AUTHORED here** — I did not open it,
and I confirm neither row did. Manager's call.

---

## Findings

- **[WARN-1]** `Docs/setupdirections.md` E.2 line 1 — the *"`verified` binding since 2026-09-14"*
  fact also lives at §11.6 ⇒ the file's **one** two-copy fact. Boarded verbatim by spec (3)(a) and
  carrying an explicit `§11.6` pointer, so not the drifting-copy shape. **Fix if ever touched:**
  whoever edits §11.6's binding sentence must move E.2 line 1 with it.
- **[WARN-2]** `Docs/setupdirections.md` Appendix B, Aura parent bullet — **re-titled rather than
  struck**; the only overwritten line in either file. Original preserved in this report.
  **Suggested fix:** strike the original title in place, as everything else in both files does.
- **[WARN-3]** `Docs/setupdirections.md` E.3 — 3 of 6 bullets carry a house-law citation
  (`SC-§125`/`VER-§7` cl. 7 · `SC-§102` · `VER-§7` cl. 7); the **Git-plugin**, **LFS-oid** and
  **editor-pairing** bullets cite no `CONVENTIONS.md` section, while the acceptance line demanded
  *"its LAW CITATION"* on every gap. Mitigating and load-bearing: the boarded item text for (b), (c)
  and (e) named no law either, so the gap is **inherited from the board, not invented**; all six
  carry their COST; the section preamble names a blanket instrument. **Suggested fix (manager):** if
  those three facts have law homes, add the citations; if they do not, that is itself worth knowing.
- **[WARN-4]** `handoffs/TASK-1371-programmer.md` §2 + `TASKBOARD.md` `TASK-1371` status — the
  post-edit census reports **8** textual wildcard hits; I measure **9** (the 9th is the *Installed*
  callout's descriptive narration, inert). `SC-§138`: a count that does not reproduce.
  ⛔ **The safety conclusion is unaffected — 0 prescriptive sites, independently confirmed.**
- **[NIT-1]** Line-count reconciliation: my reader reports 1,344 lines for `setupdirections.md`, the
  handoff and board say 1,343 (`wc -l`). The difference is the trailing newline after the last
  content line. **Not a discrepancy** — recorded so the two numbers reconcile.
- **[NIT-2]** E.3's `call_tool` placement debt lives only in the appendix; nothing forces Chapter 4's
  next editor to see it. **Suggested fix:** the manager boards the §4.3 move (`SC-§50`).
- **[NIT-3]** Appendix B's sixth bullet is now the only one without a status marker and could read as
  un-reviewed rather than still-open. Leaving it byte-untouched was **correct**; the marker is the
  manager's call.
- **[NIT-4]** Plan, draft body — says *"it is not restated here"* while carrying the parenthetical
  *"(the table's last column then reads `measured`)"*. I read `VER-§1` cl. 3a: its substance is
  **not** reproduced, and the parenthetical merely names the column the rule governs. **Ruled
  compliant**; recorded only because the sentence claims a stricter standard than it keeps.
- **[NIT-5]** Plan §3 caution 5 — the `VER-§3` amendment is **appended with no strike**, because that
  caution never carried a blocking-wait sentence to strike (it read *"unless you are told first"*).
  Correct behaviour — the edit was not forced onto a sentence that did not exist — but the handoff
  reports *"written at 3 sites"* without noting that only 2 of the 3 required a strike.

**BLOCKERS: 0.**

---

## Notes for build-master / `TASK-1370` (the host)

- ⭐ **Line 1 of this report reads `PASS` at its own instant.** `TASK-1370` clause (vii)'s condition
  is met: **both files ride**, and their evidence rides with them —
  `handoffs/TASK-1371-programmer.md` · `handoffs/TASK-1372-programmer.md` ·
  `qa/TASK-1373-report.md` (`TL-§5e` cl. 7a — a committed doc whose gate report stays on one disk is
  the orphan shape).
- ⛔ **Two files only:** `Docs/Aura AI for Unreal — Integration Plan.md` and `Docs/setupdirections.md`.
  ⛔ **Neither vault twin is staged — they are outside the repository.** ⛔ Commit by pathspec and
  verify **the commit**, never the index (the file you are committing now documents exactly this).
- ⛔ `CONVENTIONS.md`'s dirt is the **manager's** 2026-09-21 `VER-§7` cl. 2 amendment, **not** either
  doc row's — do not sweep it in under these rows' names.
- ⛔ **NO 5a** (no code) · ⛔ **NO 5b** — a docs row has **no runtime acceptance criterion**, so 5b is
  never owed and **no verdict is missing**; this is **not** an `UNOBSERVABLE` (`VER-§5` cl. 3).
- ⚠️ **One thing I could not close and you may be able to:** `git diff --stat` on
  `Docs/setupdirections.md` — additions vs a whole-file rewrite. I hold no `Bash`. Everything I
  could measure says the restore is clean; the diff **shape** is unmeasured at my instant.

## Findings routed to the MANAGER (`SC-§50`) — named and left, never repaired

1. The **six-vs-seven** contradiction in `setupdirections.md` at **four** sites (87 · 219 · 304 ·
   §3.2's table) against E.2's *"Seven agents"* — **widened from the two the row named.**
2. **§3.4's routing law** has no `verified` gate; **§3.3's lifecycle** has no `built`/`verified`.
3. 🚨 **THE LIST row 17** now disagrees with §11.1's dated correction about the tier.
4. **`CLAUDE.md`** Hard gates still says *"waits for a go"* vs `VER-§3` cl. 6.
5. 🚨 **The (C5) menu-nav attribution conflict** — *"his ruling"* vs *"an ask for him"*; **no
   verbatim sentence of his exists in any of the four `(C5)` sites.** 🧑 Live question today.
6. **Whether the LIVE grant surfaces agree with R10 is UNMEASURED** by either row or by me.
7. `qa/AURA-PHASE0.md` §Tier was an empty placeholder at both rows' instants (`TASK-1369` pending).
