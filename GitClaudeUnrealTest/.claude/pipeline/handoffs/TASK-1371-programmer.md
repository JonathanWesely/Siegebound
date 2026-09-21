# TASK-1371 — `Docs/Aura AI for Unreal — Integration Plan.md` made accurate

**Agent:** gameplay-programmer · **Date:** 2026-09-21 · **Marker:** `TASK-1371-AURA-PLAN-ACCURACY`
**Law:** `SC-§68` · `SC-§50` · `SC-§39` · `SC-§101` · `SC-§120` · `SC-§138` · `VER-§1`/`§3`/`§5`/`§6`/`§7`/`§8` · `PKG-§13` (R11) · `ACC-§11` (R15/R17) · R10
**Limb (`SC-§134` cl. 7):** this row carries **BOTH limbs** — it edits the two copies of the document **and** flips its own `status:` line; no relay is owed to anyone else.

---

## 0. The two copies (`SC-§68`)

| | |
|---|---|
| **Repo** | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Docs\Aura AI for Unreal — Integration Plan.md` |
| **Vault twin** (⛔ outside git, ⛔ NEVER staged) | `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\Aura AI for Unreal — Integration Plan.md` |
| **sha256 BEFORE** (repo) | `0901104c4497e62cf26cda89f65ec126d3ead89fc3a20f13e6c175a5e9a14bb5` |
| **sha256 AFTER — repo** | `AD76067269BF9D94E8E1F5A0D1B89880F45F1E196F81335FA348D506442DA9B9` |
| **sha256 AFTER — vault** | `AD76067269BF9D94E8E1F5A0D1B89880F45F1E196F81335FA348D506442DA9B9` |
| **EQUAL** | ✅ **yes** (`Get-FileHash … -Algorithm SHA256`, both read after the copy) |
| **Lines** | **343 → 375** (`wc -l`) |
| **Diff shape** | **+66 / −34 across 28 hunks**, one file (`git diff --numstat` / `-U0 | grep -c '^@@'`) |

Method, as prescribed: the **repo** file was edited, then `Copy-Item -Force` wrote the vault twin. ⛔ The vault copy was **never hand-edited** — it is byte-identical **by construction**, which the equal hashes above attest. `git status` shows the vault path not at all (it is outside the repo) and the repo file as the **only** file this row modified.

⚠️ Also dirty in the tree, ⛔ **not mine**: `CONVENTIONS.md` + `TASKBOARD.md` (dirty at session start) and `Docs/setupdirections.md` (**`TASK-1372`**, running in parallel).

---

## 1. Result in one line

**15 of 15 boarded correction items (a)–(o) APPLIED · 1 unboarded correction applied on measurement · 8 things left UNVERIFIED and therefore NOT written** (§4 below, with a count).

⛔ Every correction is **STRUCK, dated and sourced**, never deleted — the file's own established shape (its lines 21–23 and 307 already carried `~~struck~~` + *"superseded"*). No section was rewritten, no history removed, no new document created.

---

## 2. 🚨 ITEM (b) FIRST — AND IT IS WIDER THAN THE BOARD MEASURED

The board named **three** sites. A `grep` census of the file for `mcp__unreal_inspector__*` / `mcp__unreal_editor__*` found **five prescriptive grant sites**, all the same R10 defect:

| # | site | boarded? | before | after |
|---|---|---|---|---|
| 1 | §1 server table, the `(read-only)` bullet | ✅ yes | quotes Aura's doc calling the server read-only, unqualified | new `[!danger]` callout directly beneath: *"(read-only)" is the server's **NAME**, not its **CONTENTS**, refuted as a grant basis by R10*; names the 13 non-read tools incl. `recompile_unreal_project`; points at `VER-§7` cl. 3 + `setupdirections` §11.5; ⛔ does **not** restate the 49 names |
| 2 | §4 Phase 3 step 3, the `jsonc` block | ✅ yes | `"mcp__unreal_inspector__*",  // read-only: safe to allow entirely` **+** the lead-in *"Read-only tools can be allowed wholesale"* | both **struck in place**; block now prescribes `"mcp__unreal_inspector__<enumerated read name>"` (49) + `"mcp__unreal_editor__<pie/verify name>"` (32), with *"⛔ NEVER a wildcard on EITHER server"* and a pointer to `AURA-MCP-CENSUS.md` as the only legal name source |
| 3 | §4 Phase 3 step 4, the agent **frontmatter `tools:` line** | ⛔ **NO — found by census** | `tools: …, mcp__unreal_inspector__*, <the enumerated unreal_editor …>` | wildcard replaced by *"the 49 enumerated unreal_inspector read names"*; the struck token kept as a `#` comment with its ruling |
| 4 | §4 Phase 3 step 5, the **`qa-reviewer`** sentence | ⛔ **NO — found by census** | *"Give `qa-reviewer` **only** `mcp__unreal_inspector__*`"* | struck; corrected to the same enumerated 49, with the reason spelled out — the wildcard would hand QA a compile path **and break the very read-only posture that sentence is arguing for** |
| 5 | §6 item 8 (+ item 12) | ✅ item 8 | *"`mcp__unreal_inspector__*` wholesale"* / item 12 *"add `mcp__unreal_inspector__*` to its `tools:` line"* | both struck, both corrected to the enumerated 49 |

**Post-edit census: 0 prescriptive wildcard grants remain.** The eight surviving textual occurrences are: 2 prohibitions, 5 struck fragments, 1 namespace reference in a §7 question (`Exact tool names under mcp__unreal_editor__*`) which grants nothing.

⚠️ **QA should scrutinise sites 3 and 4** — they are the two the boarding census missed, and site 4 is the most quotable one in the file, because it *argues for* read-only-ness while prescribing the grant that destroys it.

🚨 **FINDING for the manager (`SC-§50`), not repaired here:** the defect class may exist outside this file. ⛔ I did **not** read `.claude/settings.local.json`, any agent `tools:` line, or `.mcp.json` — this row documents R10, it does not apply it, and **whether the live grant surfaces agree with the law is UNMEASURED by me.**

---

## 3. Every item (a)–(o), before → after, with its instrument

### (a) VERSION — ✅ APPLIED
- **Instrument:** `Read` of `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Marketplace\Aura\Aura.uplugin`, 2026-09-21.
- **Measured:** `"Version": 74`, `"VersionName": "1.0.6"` (`"EngineVersion": "5.8.0"`, `CreatedBy: RamenVR`) — the board's sighting reproduced exactly.
- **Before:** `**Aura 1.0.5** — "VersionName": "1.0.5", "Version": 73 … MEASURED … 2026-09-20`
- **After:** `~~1.0.5 / 73~~ → **CORRECTED 2026-09-21 (TASK-1371): Aura 1.0.6 — "VersionName": "1.0.6", "Version": 74**`, prior value kept struck, plus a line the plan lacked: ⚠️ *this row is a point-in-time reading of a **self-updating** plugin and will go stale again without telling anyone.* Callout header date moved `2026-09-20` → `2026-09-21 (TASK-1231, amended TASK-1371)`.

### (b) `unreal_inspector` IS NOT READ-ONLY — ✅ APPLIED AT 5 SITES
See §2 above. **Sources:** `CONVENTIONS.md` `VER-§7` cl. 3 (the R10 amendment, with its 13-name enumeration) + `CONVENTIONS.md:48` (the artefact-table R10 entry), both `Read` 2026-09-21.

### (c) `INDEX_IGNORE.txt` LIVES IN `Saved/.Aura/` — ✅ APPLIED (2 sites)
- **Instrument:** `Read`/grep of `Tools/aura_sync.ps1` — its pair table at `:68` names `Docs\AuraIndexIgnore.txt -> Saved/.Aura/INDEX_IGNORE.txt`; corroborated at `CONVENTIONS.md:43`.
- **Before (§4 Phase 2 step 3 · §6 item 3):** *"(project root — verify the exact location in Aura's Project Understanding doc on first run)"* / *"verify with a WebFetch … if the doc is ambiguous, project root"*.
- **After:** both struck; destination is `Saved/.Aura/INDEX_IGNORE.txt`, the file you **edit and commit** is the canonical `Docs/AuraIndexIgnore.txt`, `aura_sync.ps1` copies it, ⛔ the `Saved/` copy is never staged. Rider carried from (i): ⛔ the list shapes the **index** and nothing else — it is **not a read fence**.

### (d) THE ONE-CLICK PATH — ✅ APPLIED (3 body sites)
- **Instrument:** `ls` on disk, 2026-09-21 — `~/.claude/mcp.json` → **absent**; `~/.claude.json` → **present** (57,372 bytes).
- **Before:** §3 caution 2, §4 Phase 3 step 2, §6 item 7 all say `~/.claude/mcp.json`.
- **After:** all three struck and **pointed at the *Installed* callout, which already carries the measurement** — ⛔ the argument is **not** re-made in three places.

### (e) THE SKILLS MIRROR IS DROPPED — ✅ APPLIED (2 sites)
- **Instrument:** `ls`/`find` 2026-09-21 — `.claude/skills/` **does not exist** in this repo (0 files); `Tools/aura_sync.ps1:36` says in its own header that it *"mirrors nothing else - the integration plan's skills-mirror step was dropped"*.
- **Before:** §4 Phase 2 step 5 + §6 item 5 prescribe mirroring `.claude/skills/*` → `Saved/.Aura/Skills/*`.
- **After:** both **struck whole**, with the measurement and the `setupdirections` §11.3 record, and an explicit *"do not re-add it on the strength of this plan's original text."*

### (f) THE VERIFIER'S THREAD — ✅ APPLIED
- **Instrument:** `Read` of `.claude/pipeline/SLACK.md:31` — the registry row reads **⚙️ Dev & QA**, `thread_ts` **`1783116269.740549`**, and lists `playtest-verifier` on it by name.
- **Before:** *"posts once in the 🧪 Dev & QA thread"* — **After:** `~~🧪~~ **⚙️ Dev & QA**` + the `thread_ts` + channel `C0BF0QZP3CN` + ⛔ never top-level.

### (g) RULING R11 → OPTION B — ✅ **FOUND AND APPLIED**
🚨 The manager could not verify this. **I found it.** It is **not labelled "R11" as a section name** — R11 *is* **`PKG-§13`**:
- `CONVENTIONS.md:7614` — *"## ⚖️ PKG-§13 … (added 2026-09-13 by the manager, **ruling R11**, from `handoffs/TASK-1248-buildmaster.md`)"*.
- 🧑 **His answer, verbatim, on `TASKBOARD.md` → `TASK-1257`'s `status:` line:** *"We will also go with option B for 1257"* — status reads `done — 🧑 DISCHARGED 2026-09-13, OPTION B: the .uproject Aura entry stays WITHOUT a TargetAllowList; the next /ship names the Shipping module delta plugin-by-plugin under PKG-§13 and STOPS on anything unexplained. No agent edits the .uproject.*
- **Applied to:** §7's `.uproject` question box — the 23 dependency plugins / 19 runtime modules consequence, his Option B ruling, the `/ship` obligation it creates, ⛔ no agent edits the `.uproject`, and `PKG-§13`'s **void condition** tied back to (a)'s finding that this plugin updates itself.

### (h) RULING R15 — ✅ APPLIED (one new caution)
- **Instrument:** `CONVENTIONS.md` `ACC-§11` (grep, 2026-09-21) — the 2026-09-13 contents-rule amendment and the 2026-09-17 R17 amendment, incl. `SC-§119` cl. 5's *"the index is clean; the disk is not."*
- **Added as §3 caution 9** (ordering repaired so the list reads 1…9): the on-disk exposure is **accepted with conditions, not absent**; Aura's file tools read the disk on demand; under training-ON, what is in `Config/SiegeCloudDev.ini` is what may enter a chat turn. ⛔ **The law is not restated** — it points at `ACC-§11` + `setupdirections` §11.3 and stops.

### (i) RULING R17 — THE INDEX IS A CURATED DOCUMENT — ✅ APPLIED (+1 unboarded site)
- **Instrument:** `CONVENTIONS.md` `ACC-§11`'s 2026-09-17 amendment (R17, `TASK-1283`) + `Tools/aura_sync.ps1` (only writes = `New-Item`, `Copy-Item`; pair table `:68–69`).
- **§3 caution 4 amended in place, dated:** the *"<30,000 files / we are almost certainly over that"* framing is **materially misleading** — `Saved/.Aura/project_memory.txt` is a **byte-identical copy** of the hand-authored, git-tracked `Docs/AuraProjectMemory.md` (61 lines each, sha256 `4bb799687cbc5c8c…f514297`) ⇒ the index is bounded by **one small file a person maintains line by line** and cannot acquire a token nobody typed.
- 🚨 **And the other half is carried, not relaxed:** *"THE INDEX IS CLEAN; THE DISK IS NOT"* — the ignore list shapes the index and **nothing else**; ⛔ no line may call a listed file "excluded" or "never indexed".
- **+1 unboarded correction, applied because I measured it:** §4 Phase 2 step 4 told a reader to write *"regenerate from `CLAUDE.md`"* into Chapter 11. **Measured false** (same instrument): the canonical is **hand-authored** and **copied verbatim**; `CLAUDE.md` and `CONVENTIONS.md` are **not sources**. Struck and corrected — it is the fact that makes caution 4's claim self-evident rather than asserted.

### (j) TIER = `Pro` — ✅ APPLIED, WITH THE SPLIT SAID IN THOSE WORDS
- **Source (single, as ordered):** `TASKBOARD.md` → `TASK-1215`'s `status:` line, **copied, not paraphrased**. ⛔ **No web, no pricing page, no re-derived number.** 🧑 Verbatim: *"I just upgraded to the Aura Pro subscription."*
- **Before:** callout row *"⏳ OWED — no tier decision exists"*; §5 *"Recommendation: Trial → Pro"* presented as live.
- **After:** callout row = ✅ `Pro`, **decided 2026-09-21**, labelled an **ACCOUNT of a purchase**, ⛔ not a measurement re-takeable on this machine; the *why* kept (Ultimate ≈5× price for ≈5.6× credit ⇒ near-linear ⇒ **headroom, not efficiency**; Pro's decisive property is that **overage is purchasable — the top-up is itself the measurement**); prior text struck. §5's Recommendation callout retitled **SPENT — the decision was taken and it is `Pro`**, its original argument left intact as the record.
- 🚨 **AND THE HALF A READER WOULD ELIDE, written in those words at 3 sites (credit row, §5 banner, §7 box):** ⛔ **A DECIDED TIER IS NOT A CHARACTERISED COST.** The `$`-per-verification stays **OWED** (`credit: not visible` on all five pilot legs — **absence of a field, never a zero**).

### (k) THE FAB ~$150 SKU — ✅ APPLIED (3 sites), AS A NON-FINDING WITH PROVENANCE
- **Before:** §3 caution 8, §5 table row, §7 box all say *"verify on Fab"* / *"unverified"*.
- **After:** recorded **UNCONFIRMED / possibly never existed as recorded**, with the **provenance of the non-finding stated so it can be re-checked rather than re-believed**: *the **orchestrator** read Aura's own **Pricing Explained** and **About** pages on **2026-09-21** and found no mention* (via `TASK-1215`'s status line; ⛔ **I read no page** — the row forbids it).
- 🚨 Written at every site: **an absence of evidence is not evidence of absence and is not a refutation** (`SC-§39`) ⇒ ⛔ **never** a missed option or a forgone deal. **The §7 box is left UNTICKED** — a non-finding does not close a question.

### (l) `verified` IS BINDING — ✅ APPLIED
- **Instrument:** `CONVENTIONS.md` `VER-§6` cl. 5 (the 2026-09-14 `TASK-1273` amendment, commit `bbee7d9`).
- **Before (§4 Phase 4):** *"Only after three matches does the `verified` gate become binding … until then it is advisory."*
- **After:** struck; 🧑 he ruled *"binding"* on the pilot **as it actually stood** (two distinct behaviours, not three) and the "three" is **superseded by his ruling, not left standing as a pending condition**; no advisory suffix, `VERIFY-FAILED` blocks, `UNOBSERVABLE` never blocks.

### (m) FOUR VERDICT TOKENS, NOT THREE — ✅ APPLIED (2 sites, not 1)
- **Instrument:** `VER-§1` cl. 1 (the `MEASURED` mint, 2026-09-20, `TASK-1350`) + cl. 3a (the cell rule).
- The board named the draft body. A grep found **two** sites: the draft body's `Verdict:` line **and** the Phase 3 step 4 prose that describes it. Both now read `VERIFIED | VERIFY-FAILED | UNOBSERVABLE | MEASURED`, each with *"it is worthless without its **CELL RULE**, `VER-§1` cl. 3a"* and ⛔ **the cell rule is not restated**.

### (n) THE PIE WAIT — ✅ APPLIED AT THAT PRECISION (3 sites)
- **Instrument:** `CONVENTIONS.md` `VER-§3` — its heading and cl. 2's **struck-in-place** wait, discharged standing by cl. 6 (2026-09-20, `TASK-1363`, on 🧑 his own sentence).
- Written in exactly these terms at §3 caution 5, §6 item 10 and the draft verifier body: **the BLOCKING WAIT is discharged standing; the ANNOUNCE-AND-REPORT SURVIVES IN FULL.** *"The wait is gone"* and *"the announcement is gone"* are different sentences and only one is true. Serialization and the *"if he is already in PIE, that is his session"* rule are untouched.
- 🚨 **FINDING (`SC-§50`), recorded in the file and NOT repaired:** `CLAUDE.md`'s Hard-gates section **still carries the original *"waits for a go"* sentence** — measured by reading the copy of `CLAUDE.md` given to this session as project instructions, ⛔ **not by opening the file**, which this row may never do. Routed to the manager.

### (o) §6 IS SPENT — ✅ APPLIED, AND DELIBERATELY CHEAPLY
- **One `[!done]` line at the head of §6**: the 16-item list ran as `TASK-1213`..`TASK-1273`, the outcome is the *Installed* callout, ⛔ do not execute it as a checklist, the live procedure is `setupdirections` Chapter 11 — and, said explicitly, *only the items that would be **actively harmful** if pasted are struck individually; ⛔ annotating sixteen spent items is bloat, not accuracy.* Four were struck on that test (3, 5, 7, 8/10/12 — the grant and path ones).
- **§7 boxes: ⛔ ZERO new ticks.** The `.uproject` box gained (g)'s ruling but keeps its existing partial mark; **the credit box and the Fab box are left OPEN on purpose**, each now saying *why* it is still open.

### (3) THE HONEST FRAME — ✅ APPLIED, AS ITS OWN BLOCK IN THE *Installed* CALLOUT
> ⚖️ ***THE SETUP IS DONE; THE CEILINGS ARE PERMANENT — AND THOSE ARE TWO DIFFERENT THINGS.***

Six short bullets, each sourced, pointing at `CONVENTIONS.md` **`VER-§8`** and ⛔ **not restating the clause**:

- **(i)** input injects **beneath Slate**, **permanently** — measured **twice**, two PIE sessions, a discriminating control pair each time (`VER-§8` cl. 1: `Tab` moved focus not at all, `Down`→`IA_MenuDown` did) ⇒ UMG-click / Slate-keypress flows are `UNOBSERVABLE` **by construction**, and `VER-§8` cl. 2 puts that **on the row at boarding**. *The callout already had a version of this; what it lacked were the word **PERMANENT** and the law pointer — that is exactly what was added.*
- **(ii)** the **confirm click** is a **raw key poll on an unreflected function** ⇒ ⚖️ *a reproducible aim point is **not** a played card* (`VER-§8` cl. 7 — wall (a) **stands unrelaxed**, wall (b) fell, and ⛔ nobody may cite the fallen half against the standing one).
- **(iii)** the aim point **does not latch across MCP round trips** — ≈**648 uu** scatter vs **≤1.2026 uu** batched inside one `run_verification_sequence` ⇒ the recipe is to batch the set and every dependent read. ⛔ **No mechanism is written** — why batching removes the scatter is **not measured** (`SC-§101`).
- **(iv)** **credit appears in no tool reply** ⇒ a 🧑 human dashboard read; *"not visible"* is the **absence of a field**, never a zero.
- **(v)** **GAMEPAD: granted, but it does not reach our bindings** — ⛔ *"ungranted"* would be the wrong word (see §4.2 below).
- **(vi)** **menu navigation is out of the rig's scope**, with an explicit ⚠️ marking what I could **not** verify (see §4.1 below).

---

## 4. ⚠️ WHAT I COULD NOT VERIFY — AND THEREFORE DID NOT WRITE (count: **8**)

### 4.1 🧑 His own confirming sentence on the **menu-navigation** exclusion — **NOT FOUND**
**What I found, and it is the exclusion, not his word:**
- `TASKBOARD.md:4790` and `:4882` — *"OUT — (C5) MENU NAVIGATION … **DECLARED OUT, NOT FORGOTTEN, WITH ITS REASON** … ⇒ it is an **ASK FOR 🧑 HIM**, carried to him in the 📢 post, not a silent omission."*
- `qa/TASK-1348-verify.md:83` — *"OUT OF SCOPE — 🧑 **his ruling (C5)** + ceiling 2/3"*, corroborated in passing (`ui_snapshot` could not resolve a widget root).
- `handoffs/TASK-1353-manager.md:140` — *"remains **out**, **still an ask for him**."*

🚨 **Those are two different claims.** One source calls it **his ruling**; two call it **an ask carried to him**. I searched the board, `handoffs/` and `qa/` for *"confirmed out"*, *"keep it excluded"*, *"stays excluded"*, *"leave it excluded"* and found **no verbatim sentence of his**. ⇒ **In the document I wrote only the narrow, sourced claim** (declared out, with its reason, carried to him as an ask) **and marked his confirming sentence ⚠️ UNVERIFIED in the text itself.** 🚨 **FINDING for the manager (`SC-§50`): the report and the board disagree about whether (C5) is his ruling or an open ask — one of them is wrong, and it is not mine to settle.**

### 4.2 Gamepad — **FOUND AND WRITTEN**, but only to the width of the measurement
`qa/TASK-1348-verify.md` probe **P3** (`Read`, 2026-09-21): `simulate_right_stick` → `control_rotation` `{pitch:317.5,yaw:265}` → **no change**; the tool's own reply `"binding_found": false`, `"applied_mapping_contexts": ["IMC_Hero_Positional_0"]`, *"delivered to the player controller, but **NOTHING BINDS IT**"*; a neutral-stick control also produced no change, and **the same observable in the same posture was moved three times by P1/P2** ⇒ a **real negative**, not a dead observable.
⇒ Written as **"granted but does not reach our bindings"**, ⛔ **never "ungranted"**. **NOT written:** gamepad **face-button** reach — P8b declares it *"**inferred, not measured**"* and returns `unobs`. I did not upgrade an inference into a fact.

### 4.3 The Fab SKU's existence — **unconfirmed, and the box stays open**
I read no page (forbidden). Recorded with the orchestrator's 2026-09-21 provenance as a **non-finding**, never a refutation (`SC-§39`).

### 4.4 The **$-per-verification** — still unmeasured, and may stay so (instrument ceiling, not an omission).

### 4.5 `qa/AURA-PHASE0.md` §Tier's `Cost characterisation` subsection — **ABSENT at my instant**
`Read` 2026-09-21: `## Tier` (line 546) is still the empty placeholder heading reading *"🧑 Jonathan's decision — recorded on TASK-1215 stage B, copied here verbatim by the orchestrator"*. ⇒ the plan says it is **"being written under `TASK-1369`"** and ⛔ **asserts nothing about its contents**.

### 4.6 The **Sandbox half** of §7's `.uproject` question — left exactly as it was (`⏳ UNMEASURED`). I hold no editor and no MCP; I did not re-measure it and did not touch the sentence.

### 4.7 **Why batching removes the aim-point scatter** — deliberately not written (`SC-§101`). The recipe is recorded; the mechanism is not.

### 4.8 Whether the **live grant surfaces** agree with R10 — **UNMEASURED BY ME.** ⛔ I did not open `.claude/settings.local.json`, any agent `tools:` line, or `.mcp.json`. This row **documents** R10; it does not apply it. A disagreement there is a **FINDING for the manager**, never a repair taken in passing.

---

## 5. Findings routed to the manager (`SC-§50`) — ⛔ none repaired here

1. **Item (b) is wider than boarded** — **5** prescriptive wildcard sites in this file, not 3 (the agent frontmatter `tools:` line and the `qa-reviewer` sentence were missed by the boarding census). Both are fixed **in this document**; whether the same instruction survives in other documents or on a live surface is **unmeasured**.
2. **`CLAUDE.md` Hard gates still says *"waits for a go"*** — contradicts `VER-§3` cl. 6. ⛔ `CLAUDE.md` is NEVER touched by this row (`TASK-1365`'s carve-out died with that row).
3. **The (C5) menu-nav attribution conflict** — §4.1 above.
4. **`qa/AURA-PHASE0.md` §Tier is still an empty placeholder** at 2026-09-21 — `TASK-1369`'s consumer is not yet landed; anything citing *"§Tier's Cost characterisation"* today resolves to nothing.

---

## 6. What QA should scrutinise

1. **The five (b) sites** — especially that **no** surviving line, struck or live, can be pasted into a grant. Post-edit census: `grep 'mcp__unreal_inspector__\*\|mcp__unreal_editor__\*'` = 8 hits, **0 prescriptive** (2 prohibitions · 5 struck · 1 §7 namespace reference).
2. **That nothing was deleted** — every correction is `~~struck~~` + dated + sourced. Check the version row, §4 Phase 4, §4 Phase 2 step 5, §6 items 3/5/8/12.
3. **The tier/cost split** — that no reader can come away thinking the cost is characterised. It is said in those words at **3** sites.
4. **The Fab sentences** — that none of the three reads as a refutation or a missed option.
5. **(3)'s frame** — six bullets, each with a source, `VER-§8` pointed at and ⛔ never restated; and that **(vi) does not claim a ruling I could not find**.
6. **The twin** — both sha256 quoted and **equal**; the vault path appears in **no** `git status`.
7. **Caution numbering** — the new R15 caution was inserted and then **reordered** so §3 reads 1…9 with no gap or repeat.

---

## 7. Files touched

- ✅ `Docs/Aura AI for Unreal — Integration Plan.md` (repo, tracked) — **the only repo file this row wrote**
- ✅ `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\Aura AI for Unreal — Integration Plan.md` (vault twin, ⛔ outside git, ⛔ never staged) — written by `Copy-Item` only
- ✅ `.claude/pipeline/handoffs/TASK-1371-programmer.md` (this file)
- ✅ `.claude/pipeline/TASKBOARD.md` — **`TASK-1371`'s `status:` line only**, by `Edit`, ⛔ never `replace_all`

⛔ **Not touched:** `Docs/setupdirections.md` (`TASK-1372`'s) · `CLAUDE.md` · `CONVENTIONS.md` · `.claude/settings.local.json` · any agent `tools:` line · `.mcp.json` · `qa/AURA-PHASE0.md` · `handoffs/AURA-MCP-CENSUS.md` · `Source/**` · `Saved/**` · the `.uproject` · any asset.
⛔ **Not run:** no compile · no suite · no editor lifecycle action · no MCP call · no PIE · no commit · no push · no web fetch. (⚠️ Both Aura MCP servers are disconnected this session — ⛔ irrelevant to this row, which reads files and writes two copies of one.)

**Routing:** gate is **`TASK-1373`** (qa-reviewer, one gate over `TASK-1371` + `TASK-1372`); host is **`TASK-1370`** clause (vii). ⛔ No 5a (no code) and ⛔ no 5b — a docs row has **no runtime acceptance criterion**, so no verdict is missing and this is ⛔ **not** an `UNOBSERVABLE` (`VER-§5` cl. 3).
