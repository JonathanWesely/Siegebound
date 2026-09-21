# TASK-1369 — AURA-PHASE0 §Tier fill — build-master handoff

**Written by:** build-master, 2026-09-21, under `TASK-1369`.
**Limb carried:** this row writes **documentation only** — `qa/AURA-PHASE0.md` §Tier, this handoff, and its own `status:` line. ⛔ **No commit, no push, no compile, no suite, no PIE, no MCP, no editor lifecycle action.**
**Law:** `SC-§97` cl. 3 · `SC-§39` · `SC-§50` · `SC-§100` · `SC-§120` · `SC-§136` cl. 6 · `SC-§138` · `SC-§139` cl. 2/4(b) · `VER-§7` cl. 2 · `TL-§5e` cl. 1.

🚨 **THE ONE THING TO READ IF YOU READ NOTHING ELSE:** the file is **written**; it is **not committed**, and there is **no boarded host for it**. That is this row's single open debt and it is named in §7.

---

## 1. The four re-measurements — taken at MY instant, not inherited (`SC-§138`)

`TASK-1370` measured this file four ways minutes earlier and **held it rather than filling it itself**. Its dispatch handed me those four numbers. I re-ran all four rather than quoting them. **All four reproduced.**

| # | probe | command | my result | matches `TASK-1370`? |
|---|---|---|---|---|
| 1 | row status | `sed -n '5327p' TASKBOARD.md` | `- status: \`backlog\` — ⛔ **BOARDED, ⛔ NOT DISPATCHED.** …` | ✅ still `backlog` |
| 2 | file byte-clean vs `HEAD` | `git diff --numstat -- …/qa/AURA-PHASE0.md` | **empty** · `git status --porcelain` on it **empty** · last commit `bbee7d9` (2026-09-14) · mtime `2026-09-14 12:44:23` · 38,558 bytes | ✅ byte-clean, same commit, same mtime |
| 3 | tier vocabulary absent | `grep -ncE "Ultimate\|Indie\|overage"` → **0** · `grep -ncE "\bPro\b"` → **0** | **ZERO across the whole 576-line file** | ✅ zero |
| 4 | handoff absent | `ls handoffs/TASK-1369-buildmaster.md` | **No such file or directory** | ✅ absent |

**Pre-edit whole-file sha256:** `9c46420b0237fcb9c74eb3c0e0560c037f2815a24a29d9d7a70c987589518e2b`

⚠️ The git root is **one level up** (`SC-§102`) at `C:/GitProjects/GitHub/GitClaudeUnrealTesting`; every pathspec above is anchored there. A pathspec anchored at the project folder answers with **silence**, not an error.

---

## 2. §Tier — BEFORE

Four lines, in full. The heading, one placeholder sentence, then straight into `## Pilot`:

```
546  ## Tier
547
548  🧑 Jonathan's decision — recorded on TASK-1215 stage B, copied here verbatim by the orchestrator
549
550  ## Pilot (TASK-1230) — 2026-09-14
```

That placeholder sentence **promises a decision the file does not contain**. Two documents committed in `a9880b9` already point a reader here.

---

## 3. §Tier — AFTER

**70 insertions · 0 deletions · ONE `-U0` hunk `@@ -549,0 +550,70 @@` ⇒ a PURE INSERTION.**

The placeholder sentence at L548 was **kept, not replaced** — it is now the section's lede. Everything else is new text inserted between it and `## Pilot`. Structure, in order:

| heading | what it carries |
|---|---|
| *(lede + "Filled by")* | source = `TASK-1215`'s `status:` line, **copied not paraphrased**; explicit *"no web page, no pricing page, no estimate"*; **"This section RECORDS; it DECIDES NOTHING."** |
| `### 🚨 Read this before the table…` | **the provenance split** — see §4 below |
| `### The decision` | **TIER = `Pro`. DECIDED 2026-09-21, by Jonathan.** |
| `### The three tiers — 🌐 THIRD-PARTY ACCOUNT …` | the table, label **in the heading itself** |
| `### Why \`Pro\` and not \`Ultimate\` …` | near-linear ⇒ headroom-not-efficiency; overage-is-purchasable ⇒ the top-up is the measurement |
| `### The Fab ~$150 lifetime SKU — ⛔ UNCONFIRMED …` | the non-finding, fenced by `SC-§39` |
| `### Cost characterisation — ⛔ STILL UNMEASURED` | **the subsection two committed docs name** — see §5 |
| `### ⛔ What this row did and did not do` | the explicit decides-nothing closer |

### The table as written (label is on its own heading, not in a footnote)

> ### The three tiers — 🌐 THIRD-PARTY ACCOUNT (Aura's about page as read 2026-09-21) · ⛔ NOT a measurement · ⛔ NOT his words · ⛔ will go stale silently

| tier | monthly | annual | premium credit | overage | other |
|---|---|---|---|---|---|
| Indie | $20/mo | $10/mo billed annually (yearly total: **OWED** — not on the source line) | $15 | ⛔ **NONE** | — |
| **`Pro` ← CHOSEN** | **$40/mo** | **$360/yr** | **$60** | ✅ **PURCHASABLE** | higher rate limits · Super mode |
| Ultimate | $200/mo | $1,800/yr | $335 | **OWED** — not on the source line (⛔ not inferred from the rows above) | highest limits |

⚠️ **Two cells read `OWED` rather than an estimate**, per spec (5): Indie's yearly total and Ultimate's overage. `TASK-1215`'s line does not carry them. ⛔ Ultimate's overage was **not** inferred from Indie's "NONE" or Pro's "PURCHASABLE" — inferring it would have manufactured exactly the kind of number this file's header forbids.

---

## 4. The header rule — broken **explicitly**, not silently

This file's header states: *"No agent-derived number appears below — every measurement is copied from Jonathan's own words."*

🚨 **The tier prices are not his words.** They are an account of a third-party page read on 2026-09-21. Slipping them in under that header would have made the header false about its own file — a defect that **errors nowhere** and is only ever caught by reading. So the section states the distinction on its face (`SC-§97` cl. 3 · `SC-§139` cl. 4(b)):

- 🧑 **HIS — the only thing sourced to him, verbatim, quoted as a blockquote:**

  > "I just upgraded to the Aura Pro subscription."

  Labelled as an **ACCOUNT**, not a measurement: it reached `TASK-1215` through the **orchestrator lane**, and ⛔ no agent here can measure who typed it (`SC-§139` cl. 2). The section says so rather than implying first-hand knowledge. It also records that the **permission-class content is nil** — a purchase is recorded; nothing is authorised.
- 🌐 **NOT HIS — the price table:** third-party page, point in time, **not re-measurable on this machine**, and the section says in as many words that it **will go stale without telling anyone**, so a later reader re-reads the page rather than trusting the table's age.

⛔ **Sourcing honoured exactly as fenced:** every figure came from `TASK-1215`'s `status:` line. **No web tool was used, no pricing page opened, no second derivation performed.** The two ratios (5× price / ~5.6× credit) are `TASK-1215`'s **own words, copied**; the section marks them as inheriting the third-party label rather than presenting them as fresh arithmetic.

---

## 5. `### Cost characterisation — ⛔ STILL UNMEASURED` — quoted whole

This is the subsection `a9880b9` already points at. Reproduced verbatim as written:

> ### Cost characterisation — ⛔ STILL UNMEASURED
>
> 🚨 **This subsection exists because a reader who sees a filled §Tier will assume the cost question is closed. It is not.**
>
> ⚖️ **THE SPLIT, SAID PLAINLY AND ONCE: the TIER is KNOWN · the $-PER-VERIFICATION is UNMEASURED AND MAY STAY SO. ⛔ A DECIDED TIER IS NOT A CHARACTERISED COST. Two different things.**
>
> **Why it is unmeasured — the citation is this file's own §Pilot, below:**
>
> - §Pilot's table has **5 rows** (N1 · N3 · N2 · Probe 5 · N4) and its **`credit` column reads `not visible` in every one of them — 5/5.**
> - §Pilot's lane facts close with: *"**Credit:** not visible in any tool reply across all five legs."*
> - ⛔ **That is the absence of the FIELD in every tool reply. It is NOT a zero and must never be read as one.** No Aura tool reply carries a credit field at all, so there was nothing to record.
> - ⇒ This is an **instrument ceiling, not an omission.** Nobody failed to write a number down; the number was never emitted. The ceiling itself is enumerated in `CONVENTIONS.md` `VER-§8`, which is where it stays — restating a ceiling is how a ceiling quietly gets relaxed.
>
> **The only figure anyone has ever supplied, and what it is NOT:** §Totals he supplied records *"18% used context (90k of 500k tokens)"* across his three chat probes — 🧑 MEASURED BY JONATHAN (Aura chat, 2026-09-13). ⚠️ **That is Aura's CONTEXT-WINDOW gauge. It is not dollars and it is not credit**, and no arithmetic anywhere in this file converts it into either.
>
> **What would close this — and only these two:**
>
> 1. 🧑 **Jonathan reading Aura's own account/usage page** and supplying the $-credit figure. ⛔ No agent can reach that page; it is a human dashboard read.
> 2. 🧑 **An overage top-up purchase** — ⭐ **the top-up is itself the measurement.** A month in which he buys overage produces the first real number this project has ever had for Aura burn, and it arrives as a **receipt** rather than an estimate. (This is also exactly why `Pro` was the right tier to sit on: `Indie` has no overage to buy, so it could not have produced the measurement at all.)
>
> **Until one of those lands, the plan's `credit consumed` column stays `OWED`** — as §The three cases and §Totals he supplied already record it — and ⛔ is never filled with an estimate.
>
> **Consumers pointing at this subsection by name** (both shipped in commit `a9880b9` while it did not yet exist, both correctly hedged *"being written under `TASK-1369`"*): `Docs/Aura AI for Unreal — Integration Plan.md` (its *Credit per verification* row and its Phase 0 checklist item) · `Docs/setupdirections.md` §E.2 item 3. ⇒ **This row is what makes those two pointers resolve. It does not change what either document claims.**

### The consumers, measured rather than assumed

`git grep -n -i "Cost characterisation"` over tracked files, plus a targeted grep of `setupdirections.md`:

| file | site | how it hedged |
|---|---|---|
| `Docs/Aura AI for Unreal — Integration Plan.md` | L34, the *Credit per verification* row | *"the record for it is `qa/AURA-PHASE0.md` §Tier's `Cost characterisation` subsection — ⚠️ at this amendment's instant that subsection did **not** exist … it is **being written under `TASK-1369`**, so ⛔ nothing here asserts its contents"* |
| `Docs/Aura AI for Unreal — Integration Plan.md` | L344, the Phase 0 checklist item | *"the record is `qa/AURA-PHASE0.md` §Tier's `Cost characterisation` subsection, **being written under `TASK-1369`** (⚠️ absent at this amendment's instant, so its contents are not asserted here)"* |
| `Docs/setupdirections.md` | L1278, §E.2 item 3 | *"Both halves land in `.claude/pipeline/qa/AURA-PHASE0.md` §Tier (being written under `TASK-1369` at the time of writing). ⛔ **A tier chosen is never a cost characterised.**"* |

⚠️ One precision worth recording: `setupdirections.md` points at **§Tier**, not at the subsection by its literal name — `git grep "Cost characterisation"` does **not** hit that file. `TASK-1370`'s handoff §159 describes both as pointing at the subsection; the substance is right (both are left dangling without this row) but the grep evidence differs by file. **Named, not repaired** (`SC-§50`).

---

## 6. ⛔ The fences — proven byte-untouched by **sha256**, not by eye

The file **pins a sha256 of the baseline census** (`23d376fad53330b6359762f8632d6c530f4e4202e7605ef03a2983c76208910f`) on L99 and copies the census verbatim between two HTML markers. Disturbing that pairing **would break a pinned citation and error nowhere** — which is exactly why the 1.0.6 re-census lands under a new filename (`VER-§7` cl. 2). So I hashed each fenced region **before** the edit and again **after**, rather than asserting they look the same.

| fenced region | lines (before → after) | sha256 BEFORE | sha256 AFTER | verdict |
|---|---|---|---|---|
| **Pinned-sha256 line** (`### Verbatim copy of …`, carrying `23d376fa…910f`) | L99 → L99 | `1ebf721ebe890dc68e9a641e74f96396c0ce7080286852c277f46cdde7e747ea` | `1ebf721ebe890dc68e9a641e74f96396c0ce7080286852c277f46cdde7e747ea` | ✅ **IDENTICAL** |
| **Verbatim-copy region**, `<!-- BEGIN … -->` through `<!-- END … -->`, 441 lines | L103–543 → L103–543 | `5aac070ba517675b1b5f4a1b31c16144df8dc8319b3d0375c0dd14037ea92cc8` | `5aac070ba517675b1b5f4a1b31c16144df8dc8319b3d0375c0dd14037ea92cc8` | ✅ **IDENTICAL** |
| **§Pilot** (heading + preamble + 5-row table + lane facts), 27 lines | L550–576 → L620–646 | `2fa9166c697f8d38df35e139773f50f16c5f88faa119685f8600ee39de716294` | `2fa9166c697f8d38df35e139773f50f16c5f88faa119685f8600ee39de716294` | ✅ **IDENTICAL** (shifted only) |

✅ **EXPLICIT CONFIRMATION: the pinned sha256 `23d376fa…910f` and the copied region it cites are BYTE-UNTOUCHED. The pairing is intact.** §Pilot's five `credit: not visible` legs — 🧑 his evidence that the field was invisible — are byte-identical; the subsection I added **cites** them and changes none.

### The diff shape, which proves the same thing a second way

```
git diff --numstat  →  70   0   …/qa/AURA-PHASE0.md
git diff -U0        →  ONE hunk:  @@ -549,0 +550,70 @@
deleted lines       →  grep -c "^-[^-]" over the diff  =  0
```

**70 insertions, 0 deletions, one hunk.** A whole-file rewrite or a clobbered region is **one giant hunk with deletions**; a pure insertion cannot have touched a byte outside it. Whole-file sha256 `9c46420b…8e2b` → `1495ba48…21dd`; 38,558 → 46,470 bytes.

### Acceptance read-back — the five probes `TASK-1370` spec (3) defined, run against my own output

| probe | result |
|---|---|
| **(a)** his sentence appears verbatim | ✅ `grep -cF 'I just upgraded to the Aura Pro subscription.'` = **1** |
| **(b)** third-party-account label on the table's face | ✅ L567, in the table's **own heading** |
| **(c)** `Cost characterisation` subsection exists and says UNMEASURED | ✅ L592 `### Cost characterisation — ⛔ STILL UNMEASURED` |
| **(d)** §Pilot's table unchanged | ✅ sha256 identical (§6) |
| **(e)** pinned-sha256 line + verbatim-copy region unchanged | ✅ both sha256 identical (§6) |

Post-edit, the probe-3 greps that returned **0** now return **8** (`Ultimate\|Indie\|overage`) and **6** (`\bPro\b`).

---

## 7. 🚨 THE DEBT — named, not invented

⛔ **This row produced two files and one board line, and NONE of them is committed:**

1. `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\qa\AURA-PHASE0.md` — **modified, uncommitted**
2. `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\handoffs\TASK-1369-buildmaster.md` — **new, untracked**
3. `…\.claude\pipeline\TASKBOARD.md` — this row's `status:` line only, **modified, uncommitted**

**Why there is no commit:** my boarded host was `TASK-1370`, and **it already ran** — `a9880b9`, minutes before this row was dispatched. I therefore have **no host**. I did **not** invent one:

- `TL-§5e` cl. 1 — *a commit host is a **ROW**, not a hope.*
- `SC-§100` — boarding a row is a **board edit**, which is the **manager's** act, not a build-master's.
- My `names:` line forbids *"a commit on this row"* and *"a push"* outright.

⇒ 📋 **FOR THE MANAGER: a successor docs-only commit host is owed, carrying the three paths above.** Until it lands, the finding that made this row urgent is only half-discharged: `a9880b9` ships two documents pointing at a subsection which now **exists on one disk** but **not in git**.

---

## 8. Findings routed, none repaired (`SC-§50`)

1. 🚨 **No commit host exists for this row's output** — §7. The manager boards it; I do not.
2. ⚠️ **`TASK-1370`'s handoff (L159) attributes to `Docs/setupdirections.md` a pointer at the subsection *by name*** — a `git grep "Cost characterisation"` does not hit that file; it points at **§Tier**. The conclusion (both docs dangle without this row) is unaffected. **Named only.**
3. ⚠️ **The tier table is dated and will go stale silently.** It carries its own label, but nothing in the repo re-checks it. If a later row wants a live price it must re-read the vendor page — and that is a **new** third-party account, not a refresh of this one.
4. ⚠️ **`TASK-1215`'s `status:` line does not record Ultimate's overage policy**, so that cell reads `OWED`. Anyone who later "completes" the table by inference will have manufactured a number this file's header forbids.

---

## 9. Scope held — what this row did **not** do

⛔ No commit · ⛔ no push · ⛔ no compile · ⛔ no suite · ⛔ no PIE · ⛔ no MCP call (both Aura servers report **disconnected** pending 🧑 his restart — **not needed by this row and not a blocker on it**) · ⛔ no `replace_all` · ⛔ no web tool, no pricing page, no second derivation.

**Editor:** censused **by command line**, not by window title — one `UnrealEditor.exe`, **PID 26992**, no `-game` token ⇒ GUI ⇒ 🧑 **his**. **Described; changed nothing** (`SC-§118`).

**Never-touched surfaces:** `CLAUDE.md` (measured **clean** — `git status` does not list it — so its stage-withheld stands vacuously; ⛔ nothing authored, ⛔ nothing reverted) · `CONVENTIONS.md` · `Source/**` · `Saved/**` · the `.uproject` · every grant surface (`settings.local.json`, any agent `tools:` line, `.mcp.json`) · `handoffs/AURA-MCP-CENSUS.md` · any other row's board line.

**Board integrity (`SC-§138`):** the full-line anchor for this row's `status:` counted **1** at my instant. Its short substring *"BOARDED, ⛔ NOT DISPATCHED"* counted **43**, and the bare `- assignee: build-master` counted **292** — the law paid off live. `replace_all` never used. **Board 38,275 lines before and 38,275 after — it did not shrink.**

⚠️ `git diff` on `TASKBOARD.md` shows **two** hunks (2 insertions / 2 deletions): **L5327 is mine**; **L5343 is `TASK-1370`'s own `status:` line**, which was **already dirty at my session start** (the session-open `git status` snapshot lists `TASKBOARD.md` as `M`) and which `TASK-1370` itself named as **TAIL item (1)**. ⛔ **I did not write it and do not claim it.**

---

## 10. One-line summary

**The tier is `Pro` and it is 🧑 his decision; the $-per-verification is still unmeasured and may stay so — and the file now says both, with the price table labelled as the dated third-party reading it is. ⛔ Written, not committed: no host exists.**
