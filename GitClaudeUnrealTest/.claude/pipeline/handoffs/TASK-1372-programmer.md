# TASK-1372 — `Docs/setupdirections.md` Appendix E (+ the Appendix B / C / Chapter 11 repairs)

**Agent:** gameplay-programmer **Date:** 2026-09-21 **Status on exit:** `ready-for-qa`
**Gate:** `TASK-1373` **Host:** `TASK-1370` clause (vii)
**Limb carried (`SC-§134` cl. 7):** the DOCUMENT limb — one file, prose only. No code, no asset,
no compile, no suite, no editor, no MCP, no PIE, no commit, no push, no grant surface touched.

---

## 1. The two hashes (`SC-§68`) — EQUAL

| | sha256 | lines | bytes |
|---|---|---|---|
| **before** (both twins) | `2999348bc199c1d4e246f7f34bcc9da72c82c11ea384c08cef8ac222204d8c6b` | 1210 | 76023 |
| **after** `Docs/setupdirections.md` | `89639aa32f02ca7860244925ccab81d5fe68b2a6602038756083411afe19a89e` | 1343 | 86052 |
| **after** vault twin `GitClaudeUnrealsetupdirections.md` | `89639aa32f02ca7860244925ccab81d5fe68b2a6602038756083411afe19a89e` | 1343 | 86052 |

Line count **1210 → 1343 (+133)**. The vault copy was produced by a single copy of the finished
repo file — never hand-edited, and it is outside the repository (git's own answer, quoted:
`fatal: ... is outside repository at 'C:/GitProjects/GitHub/GitClaudeUnrealTesting'`), so it
cannot be staged. `git status --porcelain` names exactly one path:
` M GitClaudeUnrealTest/Docs/setupdirections.md` — note the `GitClaudeUnrealTest/` prefix,
which is `SC-§102` visible in the output.

### ⚠️ INCIDENT, DECLARED (it belongs in the record, not buried)

My first edit script opened the target with `io.open(P, "w")` **before** the encode could fail.
It failed (lone surrogates in my own escape literals, not in the file), and `"w"` had **already
truncated `Docs/setupdirections.md` to 0 bytes**. Recovery, measured:

1. `wc -c` = **0**, sha256 = `e3b0c442...` (the empty-file hash) — the damage was confirmed, not assumed.
2. Restored by copying the vault twin back, which I had hashed **before** any edit and which
   therefore was provably the pre-edit bytes.
3. Post-restore sha256 = `2999348b...d8c6b` — **identical to the pre-edit hash** — 1210 lines,
   76023 bytes, and `git status --porcelain <path>` printed **nothing** (clean vs HEAD).

Every subsequent write used write-to-temp → size assert → `os.replace`, so the original is never
truncated before the replacement exists. ✅ Net effect on the file: none — but the lesson is
real, and it is the same family as `SC-§102`: **a destructive step that runs BEFORE the step
that can fail leaves no error message on the thing it destroyed.**

---

## 2. ⭐ THE SELF-CHECK (acceptance's headline) — METHOD AND RESULT

**Method, stated as the acceptance requires.** Two passes:

- **(i) Programmatic.** I split the file at `## Appendix E` into `pre` and `appendixE`, then
  counted 17 distinctive tokens in each half (the fact-bearing nouns and code spans of every
  line I had drafted: `no input injection`, `seventh agent`, `` `verified` gate ``,
  `Appendix A rows 12`, `editor-bounce`, the MCP-unreachable sentence, `46510`, `` oid` ``,
  `UCLASS`, `toolset_name`, `Provider=Git`, `SC-§102`, `SC-§125`, …). Any token with
  `pre >= 1` is a duplication candidate.
- **(ii) By reading.** The programmatic pass only catches token collisions; it cannot see a
  fact restated in different words. So I read Chapter 11's opening paragraph, §4.4, §2.2 and
  §11.6 against my draft line by line.

**RESULT: 4 duplication findings, all 4 DELETED.** "None found" was not available — pass (ii)
found the two that pass (i) could not.

| # | The duplicate line I drafted | Where the fact already lives | What I did |
|---|---|---|---|
| 1 | E.1's *"...the one lane Chapter 4's MCP cannot provide; it joins as a seventh agent, `playtest-verifier`, behind a `verified` gate"* | Chapter 11's **opening paragraph** says this almost verbatim | **Deleted the whole sentence.** Kept only the half Chapter 11 does NOT have — the PROCESS half (`qa-passed` is a verdict over text; the reviewer never watches the game) — and replaced the rest with a pointer. |
| 2 | E.1's `verify §11.7 (+ Appendix A rows 12–16)` | §11.7's own first line is *"Appendix A rows 12–16."* | **Deleted the parenthetical.** Token probe: `Appendix A rows 12` → pre=1, Appendix E=0. |
| 3 | E.1's quoted `*"no input injection"*` | Chapter 11's opening **already quotes that exact phrase** and cites §4.4 for it (probe: pre=1) | **Deleted the quotation.** The line now reads *"Chapter 11's opening already names the ENGINE-side half of the hole and cites §4.4 for it."* — a pointer that does not re-say what it points at. |
| 4 | Four E.4 table rows that carried the **mechanism**, not just a label: *"Smart App Control **kills every UE C++ build**"*, *"`Build.bat` **returns exit 0 on a FAILED build**"*, *"Aura's index is not a secret fence **(the disk still is)**"*, *"Permissions: **permanent vs session-only, and what to refuse**"* | §2.2 Gotcha 2 · §2.2 Gotcha 1 · §11.3 · Appendix D | **Deleted every mechanism clause**, leaving label + pointer: `Smart App Control / 0x800711C7`, `The Build.bat exit-code lie`, `Aura's index is not a secret fence`, `The permissions law`. This is the manager's own rule applied to my own draft: *a pointer that re-states the fact it points at IS a duplicate.* |

**Lines affected:** 3 full prose lines deleted outright (findings 1–3) and 6 lines trimmed to
pointer-only (findings 3–4). Nothing survived that a reader could have gotten by reading an
earlier section.

**What I deliberately KEPT, with the reason, so QA can overrule rather than guess:**

- E.1 says *"the seventh agent and the gate §11.6"* — `seventh agent` is pre=2. Kept because
  that is the literal **title of §11.6** (`### 11.6 The seventh agent — ...`); citing a section
  by its own heading is navigation, not restatement.
- E.3's editor-pairing bullet names *"the editor-bounce, §2.2"* — `editor-bounce` is pre=1.
  Kept for the same reason: it is §2.2's own coined term, used to locate it.
- E.2 line 2 names the three ceilings in three words (Slate/UMG un-actuable · the confirm click
  a raw key poll · credit invisible) and then points at `VER-§8`. Those words are **not in this
  file** (pre=0), so they are not an in-file duplicate; and a pointer with no noun in it is
  unnavigable. The **measurements, the control pair and the dates stay in `VER-§8`** — nothing
  from the law's body is reproduced.

---

## 3. ⭐ THE E.3-vs-E.4 SPLIT — the rule I applied

**E.3 = the fact is NOWHERE in this file, so the file must now carry it (+ law + cost).**
**E.4 = the fact IS in this file, so the only thing owed is its address.**

That single test decided every line. Nothing was moved from E.3 to E.4 or back on style — each
candidate was grepped, and the grep decided.

### 3.1 The E.3 re-greps, run at my own instant (`SC-§91` / `SC-§138`)

The manager grep-confirmed these absent; I re-ran every one against the pre-edit file. **All six
confirmed ABSENT — none was dropped.**

| E.3 candidate | tokens I grepped | hits in the pre-edit file |
|---|---|---|
| (a) Blueprint compile does not dirty the package | `compile_blueprint`, `save_assets`, `SC-§125`, `sha256`, `dirty` | **0 / 0 / 0 / 0** · `dirty` hit once at §4.3 in an unrelated sense ("which dirty assets to save") |
| (b) Git plugin auto-stages the index | `Provider=Git`, `auto-stage`, `autostage`, `git add`, `git show` | **0 / 0 / 0** · `git add` hits once at §1.3 about `testvideo/`, not staging · `git show` hits once inside D.1's allow-list array |
| (c) LFS oid-vs-sha256 | `oid`, `sha256` | **0 / 0** |
| (d) git root is one level up | `git root`, `one level up`, `pathspec`, `SC-§102` | `git root` hits twice as a **location label** (§1.2 heading, §1.3) · `one level up` **0** · `pathspec` **0** · `SC-§102` **0** ⇒ the layout is drawn, the **trap** is absent |
| (e) editor-state pairing + the new-`UCLASS` caveat | `UCLASS`, `Ctrl+Alt+F11`, `editor-bounce`, `Live Coding` | `UCLASS` **0** · `Ctrl+Alt+F11` **0** · each HALF present (§2.2 editor-bounce, §4.5 hard gate) ⇒ exactly as the manager measured: **the pairing and the caveat are absent** |
| (f) `call_tool` two-field shape | `toolset_name`, `tool_name`, `Tool not found`, `call_tool` | **0 / 0 / 0** · `call_tool` appears once, but only as the string `"mcp__unreal-mcp__call_tool"` inside D.1's allow-list array — a grant entry, not the argument shape |

Also grepped and **0**: `Appendix E`, `Integration Plan`, `vault`, `Obsidian`, `46510`,
`runtime-verification`.

### 3.2 ⚖️ (f) — my judgement call, and I INCLUDED it

The manager left this optional: one line, or omit-with-a-reason if I judge it Chapter 4's.
**I judge that it belongs in Chapter 4 — and I included it in E.3 anyway.** The reasoning:

- *"It belongs in Chapter 4"* is an argument about **placement**, not about whether the file
  should carry the fact. Omitting it would mean the row whose entire job is *"anything else we
  have done that is missing from those files"* (🧑 his words) deliberately dropped a measured,
  cost-bearing, absent fact — and the fact would then exist in no document at all.
- The cost of the wrong placement is that a reader looking for MCP call syntax may not look in
  Appendix E. The cost of omission is that nobody can find it anywhere. The first is cheaper.
- So the bullet carries an **explicit, self-deleting placement debt**: it names §4.3 as its
  proper home and instructs the next editor of Chapter 4 to move it and delete the bullet in
  the same edit. ⛔ **I did not touch Chapter 4** — forbidden on this row, and the debt note is
  how the fact survives without me touching it.

QA may reasonably rule the other way; if so the remedy is one deleted bullet, and the fact is
already boarded for §4.3 by the bullet's own text.

### 3.3 Every E.4 pointer re-checked, resolving at my instant

| Pointer | Resolves? |
|---|---|
| Smart App Control → §2.2 Gotcha 2 | ✅ `**⛔ Gotcha 2 — Windows Smart App Control.**` |
| `Build.bat` exit-code lie → §2.2 Gotcha 1 · Appendix A row 3 · Appendix C | ✅ all three — Gotcha 1 present; A row 3 reads *"compile works (never trust the exit code)"*; C reads *"Parse build logs for `Result:` — exit codes lie."* |
| build command verbatim → §2.2 | ✅ the fenced canonical line |
| Blender → §5.1 | ✅ heading is literally *"The one correct route (two known-wrong routes explicitly warned)"* |
| AV → THE LIST → *The antivirus reality* | ✅ `### ⚠️ The antivirus reality` — carries Norton TLS, `UV_SYSTEM_CERTS`, and the HKCU-wipe warning |
| LFS / `testvideo/` → §1.2–§1.3 | ✅ §1.2 patterns incl. `*.mp4`; §1.3 *"Raw videos never enter git"* |
| Fab → THE LIST row 4 | ✅ row 4 (Epic Games Launcher) carries *"Fab/marketplace asset imports are a HUMAN step"* |
| Aura index → §11.3 | ✅ *"`INDEX_IGNORE.txt` shapes the semantic index — it is NOT a secret fence"* |
| permissions → Appendix D | ✅ |

⚠️ **I did NOT renumber or re-title any section**, so no pointer anywhere else in the file moved.

---

## 4. Before / after — every edit outside Appendix E

Five sites. Each was applied by an exact single-occurrence replacement with an asserted
`count == 1`; every assertion printed `OK` before the write.

### 4.1 Chapter 11 staleness site (c) — §11.1 step 1

**BEFORE**
```
1. **[You]** Create the Aura account (THE LIST row 17). The trial is free and card-less;
   the paid tier is decided AFTER the pilot measures credit-per-verification (Appendix B).
```
**AFTER**
```
1. **[You]** Create the Aura account (THE LIST row 17). The trial is free and card-less.
   ~~The paid tier is decided AFTER the pilot measures credit-per-verification (Appendix B).~~
   ⚠️ **Corrected 2026-09-21 — that is not what happened.** The tier was
   decided **WITHOUT** that measurement (🧑 his call, recorded on `TASK-1215`
   stage B), and credit-per-verification **remains unmade** — Appendix E.2 line 3. Budget
   for a tier you pick on headroom, not on a $-per-run figure you do not have.
```
Read as quoted before editing ✅. Struck, not deleted — it is a correction, so the original
sentence stays visible.

### 4.2 Chapter 11 staleness sites (a) + (b) — §11.6, one replacement

**BEFORE**
```
quoted actor/widget value; the verdict is `VERIFIED`, `VERIFY-FAILED` (routes back to the
programmer and counts as a QA loop), or `UNOBSERVABLE` (the honest answer for pure-data or
editor-only tasks — recorded on the row, never treated as a pass). The hard gate: nothing
with a runtime acceptance criterion is committed without a `VERIFIED` report. Aura output
still passes `qa-reviewer`; the verifier is advisory until three of its verdicts match the
human's own playtest, and the human decides when it becomes binding.
```
**AFTER**
```
quoted actor/widget value; the verdict is `VERIFIED`, `VERIFY-FAILED` (routes back to the
programmer and counts as a QA loop), `UNOBSERVABLE` (the honest answer for pure-data or
editor-only tasks — recorded on the row, never treated as a pass), or — added
2026-09-20 — **`MEASURED`**, a fourth token whose point is its CELL RULE rather than its
headline (`VER-§1` cl. 1 + cl. 3a — read cl. 3a before writing one). The hard gate:
nothing with a runtime acceptance criterion is committed without a `VERIFIED` report. Aura
output still passes `qa-reviewer`. ~~The verifier is advisory until three of its verdicts
match the human's own playtest, and the human decides when it becomes binding.~~
⚠️ **Discharged 2026-09-14 — 🧑 he ruled, and `verified` is
BINDING** (`VER-§6` cl. 5): the pilot ran its five legs, he ruled in one word, and from
that date a report's line-1 verdict governs with no advisory suffix.
```
Both sites read **exactly as the manager quoted them** before I touched them — neither edit was
forced. `MEASURED` is added as the fourth token and **cl. 3a is POINTED AT, not restated**: the
text says *"read cl. 3a before writing one"* and reproduces none of the cell rule.

### 4.3 Appendix C — the three promotions (COUNT: **3**)

Appended, in Appendix C's existing voice (imperative, one line, no explanation — C is the index
and E.3 is the explanation, which is the non-duplicating relationship the manager ruled):

```
- Verify the COMMIT (`git show --stat HEAD`), never the index — the editor stages files
  nobody's pathspec named.
- Verify an LFS asset by oid-vs-`sha256`, never by size — two different blobs can weigh the same.
- A Blueprint compile does not dirty the package; a tool-side save returns true and writes
  nothing — the hash must change (E.3).
```
Nothing above the insertion point was altered; the ten existing laws are byte-identical.

### 4.4 Appendix B — the Aura bullets, each answered / struck / left-open EXPLICITLY

**Parent bullet — BEFORE:** `- **Aura (Chapter 11) — the ⚠️ items no one has measured yet:**`
**AFTER:** re-titled to say the list was re-measured 2026-09-21 and that an answered item is
struck in place and moved to its answer, never deleted.

| Bullet | Ruling | What the file now says |
|---|---|---|
| Fab `$150` SKU | **UNCONFIRMED, left open** | *"Still **UNCONFIRMED** — nobody has looked since, so it is recorded as unconfirmed and ⛔ never as an option that was missed: an unverified SKU is not a cheaper path, it is an unverified SKU."* — this is the manager's fence, written into the file's own voice |
| Enhanced Input | **PARTLY ANSWERED** | ✅ our input ACTIONS drive (`inject_input_action` on `IA_Card1` / `IA_Move` through `IMC_Hero`, 2026-09-14 pilot); ⚠️ **still OWED**: the positional `KBD-§` KEY layout — no physical-position keypress has ever been driven, and `VER-§8` is why |
| Sandbox second `.uproject` entry | **still OPEN** | unchanged in substance, now marked `⚠️ still OPEN` |
| Credit per verification | **SPLIT** (it was carrying two facts) | original struck; replaced by *"the tier decision is MADE, and credit per verification is STILL UNMEASURED"*, with both halves' sources deferred to **E.2 line 3** so the explanation lives in exactly one place |
| `mcp__unreal_editor__*` tool names | **ANSWERED, struck, moved to its answer** | struck; answer names `handoffs/AURA-MCP-CENSUS.md` (verified present on disk, 21501 bytes) + D.1's enumerated allow-list, then the version-bound caveat: `VER-§7` cl. 2's re-census trigger fired on `1.0.5 → 1.0.6`, re-census boarded as `TASK-1366` (verified boarded on the board) |

⚠️ **MEASURED DISCREPANCY, reported not silently absorbed (`SC-§138`):** the manager's spec
says *"Appendix B's **five** Aura bullets."* **There are SIX** under the Aura parent bullet — the
sixth is *"Whether Aura's verification can observe the game's C++ assistant-snapshot /
cheat-manager state directly."* It is not in the manager's list, so **I left it byte-untouched.**
It reads as still-open and is not wrong; it is flagged here rather than edited on an inferred
instruction.

### 4.5 Appendix E — new, at the bottom, after D.5

Quoted whole in §5 below.

---

## 5. Appendix E, quoted whole

```markdown
## Appendix E — Aura: the plan behind it, what the setup is FOR, and what neither file carries

⛔ **Read this AFTER Chapter 11, never instead of it.** Chapter 11 is the HOW, end to end, and
none of it is repeated below — two copies of one fact in one file drift apart. The gaps in E.3
are the only reason this appendix exists.

### E.1 The plan, referenced — and the hole the Aura setup was bought to close

- **The plan:** `Docs/Aura AI for Unreal — Integration Plan.md` (and its Obsidian-vault twin of
  the same name, kept byte-identical by copy; the repo copy is the source and the only one that
  is ever staged). It is the **research + decision record** — written **2026-09-14, BEFORE any
  of Chapter 11 was executed** — and amended since wherever execution measured something the
  research had only guessed (`TASK-1231`, `TASK-1371`). Read the PLAN for *why this was chosen
  and what was rejected*; read CHAPTER 11 for *how it is built*.
- **What the setup is FOR.** Chapter 11's opening already names the ENGINE-side half of the hole
  and cites §4.4 for it. The half neither one names is the PROCESS half: `qa-passed` is a verdict
  over **text** — the reviewer reads a diff, it never watches the game — so before Aura a feature
  could be written, reviewed, compiled and committed with nobody, human or agent, having ever
  SEEN it happen. That is the runtime-verification hole, and Aura's PIE lane is the instrument
  bought to close it.
- ⇒ **Then stop: the HOW is Chapter 11** — install and tier §11.1 · project config §11.2 · the
  `Saved/.Aura` pair and its sync §11.3 · the two stdio servers §11.4 · the allow-list law
  §11.5 · the seventh agent and the gate §11.6 · verify §11.7.

### E.2 The honest status — three lines, and the last two are what neither file says

1. **The setup is DONE.** Seven agents, the bridge, the enumerated grants, and `verified`
   **binding** since 2026-09-14 (`VER-§6` cl. 5; §11.6 carries the correction).
2. **The ceilings are PERMANENT, and that is a different thing.** Slate/UMG un-actuable, the
   confirm click a raw key poll, credit invisible: each one measured, dated and enumerated in
   `.claude/pipeline/CONVENTIONS.md` `VER-§8`, which is where they stay — restating a ceiling is
   how a ceiling quietly gets relaxed. ⇒ *A setup being finished and an instrument being complete
   are two different claims, and only the first one is true here.*
3. **The cost is UNCHARACTERISED.** The **tier is decided** (🧑 his call, `TASK-1215` stage B).
   The **$-per-verification behind it is unmeasured**: no Aura tool reply carries a credit field,
   so every pilot row records credit as *not visible* — an absent field, never a zero. Both halves
   land in `.claude/pipeline/qa/AURA-PHASE0.md` §Tier (being written under `TASK-1369` at the time
   of writing). ⛔ **A tier chosen is never a cost characterised.**

### E.3 The gaps — facts in NEITHER file, with the law and what not knowing costs

Each was measured on the original machine and then written into house law, and each was grepped
against this whole guide on 2026-09-21 and found absent. What earns a line its place in a SETUP
guide is the last clause: what it costs you to stand a machine up without it.

- ⛔ **A UE5 Blueprint compile does NOT dirty the package.** `compile_blueprint` followed by a
  named `save_assets` returns **true, and writes nothing** — sha256, byte count and mtime all
  unchanged; only 🧑 the Blueprint editor's own Compile → `Ctrl+S` writes the package
  unconditionally, so a row whose deliverable is a compiled Blueprint carries a human keystroke.
  **The gate is that `sha256` must CHANGE**, never the return value (`SC-§125`; the same trap is
  restated inside the grant itself at `VER-§7` cl. 7). **Cost, and this is the sharp half:** a
  green test suite — *even a headless `-nullrhi` one* — **cannot** discriminate a stale on-disk
  class, because PIE recompiles the Blueprint in memory before running it. **A cooked build does
  not recompile on load.** The stale class therefore survives every pre-ship gate that called it
  green, and ships as a real defect.
- ⛔ **The UE editor's Git plugin auto-stages the index.** Under `Provider=Git` the editor runs
  `git add` on every asset it saves or imports, so files nobody's pathspec named are already
  staged. ⇒ **Commit by pathspec and verify THE COMMIT (`git show --stat HEAD`), never the
  index.** The durable fix is `Provider=None`, which is 🧑 the human's call. **Cost:** a `git
  status` that looks staged-and-correct about work no task authorized — and a commit that
  silently carries it.
- ⛔ **Verify an LFS asset by oid-vs-`sha256`, never by size.** §1.2's patterns get an asset INTO
  LFS; what is absent is how to check that the right bytes went in — compare the pointer's `oid`
  with the working file's `sha256`. **Cost, measured live:** a deliberately mutated `DT_Cards.uasset`
  and the committed one were **both 46510 bytes**, so a size check passes on the wrong blob; only
  the oid comparison proved the break existed, and then that it had been reverted.
- ⛔ **The git root is ONE LEVEL UP** (`SC-§102`). §1.1 draws the two-level layout; the trap it
  does not draw is what a command anchored at the wrong level does — **it answers with SILENCE.**
  A mis-anchored pathspec matches nothing and prints nothing. **Cost:** empty output reads
  *exactly* like "nothing to commit", so the work looks committed and is not. Anchor every
  pathspec at the git root, and never read an empty result as a negative answer.
- ⛔ **The editor-state PAIRING — CLOSED to compile, OPEN (+ MCP on `:8000`) to import.** Each
  half is already in this guide (the editor-bounce, §2.2; the MCP hard gate, §4.5), but they are
  opposite requirements, and the pairing is the thing a session actually has to sequence.
  ⚠️ The usual escape hatch does not apply: **Live Coding cannot help when the diff adds a new
  `UCLASS`** — a new type needs a full build, so the editor comes down regardless. **Cost:** a
  chain that imports before it compiles, or that reaches for Live Coding on a new class, stalls
  with an error that names neither cause.
- ⚠️ **`mcp__unreal-mcp__call_tool` takes `toolset_name` AND `tool_name` as two separate
  arguments**; the fully-qualified dotted single string returns `Tool not found` (`VER-§7` cl. 7).
  **Cost:** one failed call per wrong guess, with an error that reads like a missing tool rather
  than a wrong argument shape. ⛔ **Placement debt, declared rather than hidden:** this is a
  Chapter 4 fact about the `:8000` bridge, not an Aura one. It sits here only because the row
  that wrote this appendix was forbidden to edit Chapter 4 — move it to §4.3 when Chapter 4 is
  next opened, and delete this bullet in the same edit.

### E.4 The map — where the traps this guide ALREADY documents live

Pointers only, deliberately. If you ever find yourself explaining one of these *here*, it
belongs in the section named beside it instead.

| Trap | Where it lives |
|---|---|
| Smart App Control / `0x800711C7` | §2.2, Gotcha 2 |
| The `Build.bat` exit-code lie | §2.2 Gotcha 1 · Appendix A row 3 · Appendix C |
| The canonical build command, verbatim | §2.2 |
| Blender MCP: the one correct route and the two known-wrong ones | §5.1 |
| Norton-class TLS interception · `UV_SYSTEM_CERTS` / `HF_TOKEN` · the HKCU wipe | THE LIST → *The antivirus reality* |
| The LFS patterns · `testvideo/` · raw videos never enter git | §1.2–§1.3 |
| Fab / marketplace imports are a 🧑 human step | THE LIST, row 4 |
| Aura's index is not a secret fence | §11.3 |
| The permissions law | Appendix D |
```

---

## 6. ⛔ Fences — what I did NOT touch

- ⛔ **No grant surface.** `.claude/settings.local.json`, every agent `tools:` line, `.mcp.json`
  and **Appendix D.1's allow-list array** are byte-untouched. E.3's `call_tool` bullet
  **documents** an argument shape; it grants nothing and changes no array.
- ⛔ `CLAUDE.md` · `CONVENTIONS.md` · `Source/**` · `Saved/**` · the `.uproject` — untouched.
- ⛔ **Chapter 4 — untouched**, including §4.3 and §4.4, despite §3.2 above.
- ⛔ `Docs/Aura AI for Unreal — Integration Plan.md` (`TASK-1371`'s file) — **read-only**. I
  read its front-matter and its two status callouts to date the plan and name its amendments;
  I wrote nothing to it. No collision with the parallel row.
- ⛔ `qa/AURA-PHASE0.md` — read-only. §Tier was **empty at my instant** (one placeholder
  sentence, no tier named, no cost subsection), so E.2 line 3 says *"being written under
  `TASK-1369`"* and **asserts nothing about its contents**, exactly as the spec directs.
- ⛔ No compile, no suite, no commit, no push, no editor lifecycle action, no MCP, no PIE, no
  web fetch. The two Aura MCP servers are disconnected this session; nothing on this row needed
  them.
- `TASKBOARD.md`: **this row's `status:` line only.**

---

## 7. 🔍 FOR QA — scrutinise these first

1. **The truncation incident (§1).** Verify independently that `Docs/setupdirections.md` is
   whole: the recovery is claimed against a hash I measured myself. `git diff --stat` on the
   path should show **additions and a handful of modified lines, and no mass deletion**; if it
   shows the file rewritten, my recovery claim is wrong and this row fails.
2. **The E.3/E.4 boundary.** For each E.3 bullet, re-grep the pre-edit file yourself. If any
   one of the six was already in the file, it belongs in E.4 and my split is defective.
3. **⚖️ The `call_tool` bullet (§3.2).** A flagged decision, not a settled one. I included what
   the manager said I could omit. Rule it either way.
4. **The three Chapter 11 sites.** Confirm each ORIGINAL sentence is struck rather than deleted,
   and that cl. 3a is pointed at rather than reproduced.
5. **Appendix B's sixth bullet (§4.4).** I left it alone on a count discrepancy. Confirm that
   was right.
6. **Anti-bloat.** The spec's hardest constraint. +133 lines on a 1210-line file. Read Appendix E
   against Chapter 11 and tell me if any line still restates. My self-check found and deleted 4;
   a second reader is the point of the gate.

---

## 8. 📋 FINDINGS FOR THE MANAGER (`SC-§50`) — out of my WRITES scope, NOT repaired in passing

Each of these is stale in the same way the three Chapter 11 sites were. **None is inside my
`names:` line, so none was touched.** Boarding is the manager's call.

1. **§3.2 The roster table lists SIX agents.** `playtest-verifier` — the seventh, which Chapter 11
   §11.6 documents in the same file — is **absent from the table.** The file contradicts itself.
2. **§3.4 The routing law has no `verified` gate.** Step 5 reads *"`qa-passed` / `ready-for-integration`
   → build-master compiles, assembles, commits"* — no 5a/5b/5c, no verify leg. Its **Hard gates**
   paragraph names only the PASS QA report, while `CLAUDE.md` and `VER-§` require a `VERIFIED`
   report for any row with a runtime criterion.
3. **§3.3 The task lifecycle line** (`backlog → in-progress → ready-for-qa → qa-passed/qa-failed →
   integrating → done`) has **no `built` and no `verified` state.**
4. **§1.1's layout diagram** annotates `.claude/agents/` as *"the six agent definitions"*.
5. **THE LIST row 17 carries the SAME stale sentence** I corrected at §11.1 step 1 —
   *"paid tier after the pilot measures credit-per-verification."* My scope was §11.1; the twin
   sentence in THE LIST is still stale and now **disagrees with §11.1**, which is worse than
   before I started. ⚠️ **This one is the most urgent of the five** — I created the
   disagreement by fixing only the half I was scoped to, and said so rather than quietly
   widening scope.

⛔ I did not repair any of these. A surface that disagrees with the law is a finding, never a
repair in passing.

---

## 9. Files touched

- ✏️ `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Docs\setupdirections.md`
- ✏️ `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\GitClaudeUnrealsetupdirections.md` (copy only — outside git, never staged)
- ✏️ `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\handoffs\TASK-1372-programmer.md` (this file)
- ✏️ `TASKBOARD.md` — `TASK-1372`'s `status:` line only
- 👁️ read-only: the Integration Plan · `CONVENTIONS.md` (`SC-§102`, `SC-§125`, `VER-§1`/`§6`/`§7`/`§8`) · `qa/AURA-PHASE0.md` · `handoffs/AURA-MCP-CENSUS.md` · `TASKBOARD.md`
