# TASK-543 — `ContextTokens` 2048 → 3072 + the tier-selector arithmetic re-check

**Agent:** gameplay-programmer · **Date:** 2026-08-05 · **Status:** ready-for-qa · **QA gate:** **TASK-550**
**Law:** `AS-§21.0` ruling 2 · `AS-§21.7` · `AS-§19` · `AS-§23` · `AS-§12g` · `FT-§6` · `SC-§32`

> 📌 **M8, verbatim: adds no replicated property, no new replicated class, no new relevancy tier.**

⛔ **NOT COMPILED.** TASK-551 owns the only compile. ⛔ No Git. ⛔ No editor, no PIE.

---

## 1. Files touched — exactly two, both in the plugin

| File | Change |
|---|---|
| `Plugins/SiegeLlama/Source/SiegeLlama/Public/SiegeLlamaSubsystem.h` | `ContextTokens` `2048` → **`3072`**; freeze comment **rewritten, not deleted** |
| `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp` | **NEW** `ContextGrowthVramReserveMiB = 151`; added to **both** `ChooseTier` comparisons; **all three** `OutReason` strings name it |

**Fences honoured — verified, not assumed:**
- ⛔ `SiegeLlamaSpike.cpp` (D4) — **untouched**. `git diff --stat` shows it absent.
- ⛔ `Source/GitClaudeUnrealTest/Siegebound/` — **untouched by me**. Six other tasks have that tree dirty in the same working copy; none of those edits are mine.
- ⛔ `FullOffloadCostMiB` (2703), `PartialOffloadCostMiB` (1264), `VramGameReserveMiB` (768), `PartialGpuLayers` (18), `MaxSnapshotTokens`, `MaxOutputTokens`, `SafetyMarginTokens`, `SnapshotPreFilterMaxChars` — **all byte-unchanged**.

**Style note for QA:** `SiegeLlamaSubsystem.cpp` contained **zero** non-ASCII bytes before this change. I drafted the new comment with `⛔`/`⚠️` and then **stripped them back to ASCII** to keep the file as I found it (the file has no BOM; `SiegeLlamaSpike.cpp` is the only plugin file carrying unicode). Re-verified: **0 non-ASCII occurrences** in both edited files.

---

## 2. The header diff — `ContextTokens`

`2048` → **`3072`**. The freeze text is **quoted inside the new comment** rather than removed, then the unfreeze is recorded on top of it:

- **Who:** Jonathan, **2026-08-05**, via `AskUserQuestion`, `AS-§21.0` ruling 2.
- **Why, verbatim:** *raise the ceiling rather than trim teaching content.*
- **The provenance point, written into the code:** the freeze reserved this constant **to him**; the person who could unfreeze it is the person who did. The comment states that **an agent proposing a further move needs his word again**, and that *"the last move was allowed"* is not authority for the next one.
- **Kept as required:** the **`IT IS A REQUEST`** clause (`llama.h:551-556`; everything that sizes work reads `llama_n_ctx(ctx)`).
- **Occupancy, labelled DERIVED in the comment itself:** `~1362 + 400 + 96 + 48 = ~1906`; **~142 spare at 2048 ⇒ ~1045 at 3072**.
- `zoneA_tok = 1139` stays labelled **STALE — PENDING RE-MEASUREMENT** and is **not** recomputed.
- The comment names the stale-by-consequence set: bars #1/#3/#4 and the tier table's `vram_delta` figures were all taken at 2048; **TASK-552** re-takes them.

### ⛔ The 27-vs-250 correction — stated exactly as the law requires

`CONVENTIONS.md:1859` derives a **325-char** growth budget from a **1139** baseline, but the ladder **measured** `1139 → 1356 → 1362` (`:1229-1230`) and `AS-§12f` records *"27 of the allowed tokens still unspent"* — so the room was **~27 tokens, not ~250**, and TASK-521 then spent **+308 chars** against it.

> ⛔ **This is a DERIVATION FROM TWO MEASURED NUMBERS, NOT A READING. `AS-§12g` forbids treating a derivation as a reading. It is cited here ONLY as *the reason the ceiling is being raised* — NEVER as a finding, NEVER as an overflow report, NEVER as evidence that any shipped prompt is broken.**

**And the silence is not evidence.** Nothing has ever printed `PROMPT BUDGET OVERFLOW` — but `RunBudgetAssertion` **early-returns** unless `ActualCtx > 0 && Context != nullptr`, and its Zone-A half is inert unless `SetStaticPrefix` has succeeded. **The assertion may never have executed.** `SC-§32`: a mechanism never observed to function is not known to function. **I do not cite its silence as evidence anywhere**, and the header comment says so in as many words.

---

## 3. ⭐ The tier arithmetic — the half that is easy to get wrong

### What I did NOT do
⛔ I did **not** edit `2703` → `2854`, or `1264` → anything. Both are **measurements taken at `ContextTokens = 2048`**, and moving a measured constant by arithmetic is *a derivation wearing a measurement's authority* (`AS-§23`, `AS-§12g`). Their provenance blocks are intact.

### The new constant
```cpp
static constexpr uint64 ContextGrowthVramReserveMiB = 151;
```
Placed in `namespace SiegeLlamaPrivate` **immediately after** `VramGameReserveMiB`, so the derived value sits visibly outside the measured tier-table block. Typed **`uint64`** to match `FreeMiB` and its three neighbours — the comparisons stay unsigned-clean and every `%llu` matches.

**Its comment states, as ruled:** the operands · that it is **DERIVED, NOT MEASURED** · that the **128 head-dim is an ASSUMPTION** · that it **errs HIGH on purpose** (`AS-§19`: a gate that promotes must assume the LARGER cost) · that **TASK-552's live `vram_delta` reading replaces it**.

**The derivation, recorded in the source:**
`36 layers × 8 KV heads × 128 head-dim × 2 (K+V) × 2 bytes (f16) = 147,456 B/token` × `1024` added tokens (`3072 − 2048`) = `150,994,944 B` = **144 MiB exactly**.

### ⚠️ One precision point QA should check deliberately: **151 is not the MiB figure**
`144 MiB` is the honest binary conversion; **`151` is the same quantity in decimal MB** (150.99 MB) — the figure `AS-§21.7` records. The constant is named `...MiB`, so **151 MiB is ~7 MiB larger than the derivation strictly yields.** I kept the larger number **deliberately**, and the comment says so with the reason: `AS-§19`'s safe direction on a promoting gate. Erring high costs a boundary machine one tier of speed; erring low costs it a tier it cannot run. **I am flagging this rather than silently reconciling it — if QA rules the constant must equal its own derivation, `144` is the alternative and it is the *less* safe one.**

### ⚠️ The partial gate is knowingly over-reserved, and the comment says so
At `PartialGpuLayers = 18 of 36`, only the **offloaded half** of the KV cache is GPU-resident, so the layer-scaled figure would be roughly **half** (~72–76 MiB). The ruling says **add it to BOTH comparisons**, so I did. The comment records that this is conservative-by-choice, **not** an oversight, and that it must **not** be "corrected" by scaling — a second derived number stacked on the first is not more knowledge, it is more assumption.

---

## 4. ⛔ REQUIRED TABLE — thresholds before and after, and who moves

| Gate | Threshold BEFORE | Threshold AFTER | Δ |
|---|---|---|---|
| `FullOffload` | `2703 + 768` = **3471 MiB** | `2703 + 768 + 151` = **3622 MiB** | **+151** |
| `Partial` | `1264 + 768` = **2032 MiB** | `1264 + 768 + 151` = **2183 MiB** | **+151** |
| `CpuOnly` | below 2032 | below 2183 | fall-through, **unchanged behaviour** |

**Which machines change tier:**

| `FreeMiB` band | Before | After | Verdict |
|---|---|---|---|
| ≥ 3622 | Full | **Full** | no change |
| **3471 – 3621** | **Full** | **Partial** | ⚠️ **DEMOTED one tier.** 151-MiB-wide band. Partial measured **0 aborts of 5** — degraded speed, still correct. |
| 2183 – 3470 | Partial | **Partial** | no change |
| **2032 – 2182** | **Partial** | **CpuOnly** | ⛔ **DEMOTED INTO THE TIER THAT MEASURABLY FAILS.** 151-MiB-wide band. |
| < 2032 | CpuOnly | CpuOnly | no change |

**Against the two recorded machines — neither changes tier:**

| Recorded reading | Before | After | Margin after |
|---|---|---|---|
| **6893 MiB free** (`tier=full`) | Full (≥3471) | ✅ **Full** (≥3622) | **+3271 MiB** — comfortable |
| **3038 MiB free** (`tier=partial`) | Partial (≥2032, <3471) | ✅ **Partial** (≥2183, <3622) | **+855 MiB** over the partial floor, down from **+1006**. Tighter, exactly as `AS-§21.7` predicted. |

### ⛔ SAY IT PLAINLY, AS ITEM (4) REQUIRES — the real cost of the ruling
**A machine with 2032–2182 MiB free was on `Partial` (0 aborts of 5) and is now on `CpuOnly`, a tier that MEASURABLY FAILS — 2 of 5 generations hit the 10 s ceiling and returned truncated JSON.** That is a genuine cost of raising the context, it is **151 MiB wide**, and Jonathan is entitled to know it before he plays.

Three things that bound it honestly, in both directions:
1. **No recorded machine is in either band.** The pinned RTX 5070 Laptop is not affected at either reading.
2. **The band is probably narrower than 151 MiB in reality** — see §3: on the partial tier only ~half the KV growth is GPU-resident, so the true partial-gate demotion band is likely **~75 MiB**. ⛔ **That is a derivation too, and it does not license shrinking the constant.**
3. **The demotion is the safe failure.** The alternative is promoting a machine into a tier it cannot run — `FT-§6`'s named defect, and the whole reason this re-check exists.

⛔ **The CPU-tier fall-through defect is NOT fixed here.** No refusal added, no condition altered, no tier changed. Out of scope by ruling.

---

## 5. ⚠️ The one judgment call inside the scope fence — QA please rule on it

The third `OutReason` — the CpuOnly fall-through — **printed the partial threshold**: `PartialOffloadCostMiB + VramGameReserveMiB`. That gate moved. I **updated the number it prints** (and named the new operand), because leaving it would print **2032** while the code had actually tested against **2183** — a threshold the code never used.

**What changed on that path:** the format string and its arguments. **What did NOT change:** the condition (there is none — it is the fall-through), the returned tier, the control flow, the log verbosity. **The behaviour is byte-for-byte the same tier decision.**

I read this as *required* by spec item (3) — *"The `OutReason` **strings** must name the new term so the log line explains the decision it made… a selector whose reason string omits an operand it used is a log that lies quietly"* — and as **not** touching the fall-through defect, which is about the tier being *selected*, not about the number being *reported*. **Flagged explicitly so QA can overrule it rather than discover it.**

---

## 6. The other three constants — what I found

⛔ **None of their values moved.** This is the required check, reported as a finding.

| Constant | Assumption tied to 2048? | Verdict |
|---|---|---|
| `MaxOutputTokens = 96` | **No.** Its comment bounds *how many tokens a run may emit* — an axis independent of `n_ctx`, explicitly contrasted with `HardTimeoutSeconds`. | ✅ Clean. Nothing to change. Its consumers (`:1436`, `:1702`) compare against **`ActualCtx`**, read live. |
| `MaxSnapshotTokens = 400` | **No.** It is the Zone B+C authority, enforced by `llama_tokenize`. Its comment derives it from the snapshot's own content, never from the context size. | ✅ Clean. Raising `n_ctx` only makes it easier to satisfy. |
| `SafetyMarginTokens = 48` | **The VALUE, no. The COMMENT, yes — twice.** Its derivation (`13 + 20 = 33`, cleared ~1.45×) is context-independent and stands. But its closing *"Sanity"* paragraph reads `…= 1683 **of 2048**`, and `…it is 1933 **of 2048**`. **Both denominators are now stale.** | ⚠️ **Reported, DELIBERATELY NOT EDITED — see below.** |

### Why I did not fix the stale `of 2048` text, and I want this challenged rather than assumed
Two reasons, and either alone would have stopped me:
1. ⛔ Spec item **(5)** names it: *"no budget-assertion margin."* The block is inside the fence.
2. ⛔ Its sanity line is built on **`1139`**, which `AS-§21.7` labels **STALE — PENDING RE-MEASUREMENT** and says explicitly **"do not recompute it."** Rewriting `1683 of 2048` into a 3072-denominated figure would mean **re-deriving a line from a stale operand and re-blessing it** — the exact move `AS-§12g` bans, done in the name of tidiness.

⇒ **The correct repair is a comment-only pass AFTER TASK-552 prints the real `zoneA_tok`**, when the sanity line can be rebuilt from a reading instead of from a stale constant. **Recommend it be boarded as a follow-up.** I would rather leave a comment visibly stale than make it look freshly derived when it is not.

### The budget assertion's arithmetic — it still holds
`RunBudgetAssertion` (`SiegeLlamaSubsystem.cpp:1116+`) is **context-size-agnostic by construction**: it reads `ActualCtx` from `llama_n_ctx(ctx)` live and compares `PrefixTokens + 400 + 96 + 48` against it. **No literal 2048 appears anywhere in it.** Raising the request only raises `ActualCtx` (or gets clamped — and the clamp is already logged with a `<== CLAMPED` marker at `:1051-1052`). The `(requested %d)` fields print `ContextTokens`, so they will now correctly print **3072** with no edit. Same for the per-request guard at `:1436`, which also tests against `ActualCtx`.

⚠️ **Restating the trap:** the assertion **holding** is a property of its code, which I read. It is **not** a claim that it has ever run. See §2.

---

## 7. ⚠️ Out-of-scope stale reference, for the board (I did not touch it)

`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantZoneATest.cpp:636` carries a comment reading **"`ContextTokens` frozen at 2048"**. It is now false. That file is in the fenced tree **and** tests are **TASK-549's** exclusive scope. **Routing it as early information, not fixing it, and not counting it as this lane's finding.**

---

## 8. What QA should scrutinise hardest

1. **`151` vs the derivation's `144 MiB`** (§3) — a deliberate, documented upward round on `AS-§19` grounds. Rule on it.
2. **The CpuOnly `OutReason` threshold update** (§5) — the one edit that sits near the scope fence.
3. **The `%llu` argument counts** — Full: 5 specifiers / 5 args. Partial: 6 / 6. CpuOnly: 5 / 5. All operands `uint64`, `FreeMiB` `uint64`, all sums `uint64`. No signed/unsigned mixing, no truncation.
4. **That the freeze comment is REWRITTEN, not deleted** — the original freeze text is quoted verbatim inside the new block, and Jonathan is named as the author of the unfreeze with date and reason.
5. **That the 27-vs-250 correction appears ONLY as the reason for the raise** (§2) — never as a finding, never as an overflow report.
6. **That no measured constant moved** — `2703`, `1264`, `768`, `18`, `36` are all byte-identical.

---

## 9. Downstream

- **TASK-550** — QA gate on this task.
- **TASK-551** — the only compile. The plugin builds **inside** the `GitClaudeUnrealTestEditor` target, so these two files compile in that gate by design.
- **TASK-552 (Stage 5, Jonathan)** — prints the live `vram_delta` at `ctx=3072`; **that reading replaces `ContextGrowthVramReserveMiB`** and settles the 128-head-dim assumption. It is also where `zoneA_tok` stops being stale.
