# QA Report — TASK-500 (FT-LAW-AUDIT)

**Verdict: FAIL — 3 BLOCKER · 10 WARN · 6 NIT**

⚖️ **WHAT "FAIL" MEANS HERE, STATED FIRST SO NOBODY MISREADS IT.** This is not a verdict on the batch, the plan, or anybody's work. It means: **the law as committed is not internally consistent, and three of the inconsistencies would cause a task to act wrongly if the law is quoted as written.** ⛔ **Every one is reported, none is resolved — rewriting law is the manager's lane** (TASK-500 spec (4), the boundary build-master honoured an hour earlier).

⚠️ **N is not 0.** Full null-result statement, per spec (5), is in §D: I state exactly what I read and exactly what came back clean.

---

## §A — WHAT I READ, AND HOW I VERIFIED IT

| Artifact | Read |
|---|---|
| `CONVENTIONS.md` "⚖️ THE FINE-TUNE RUNG" §1–§13 | end to end, raw `Read`, lines 1695–1881 |
| The four in-place edits | §6 CUDA amendment (`:668-674`) · §7 teacher-model clause (`:681-686`) · §7 `check-ignore`/`dir/*` clause (`:702-710`) · §16 edit-is-not-deletion (`:1434-1441`) |
| Supporting law the batch leans on | §5 · §7 · §8 (incl. 🔢 TOKEN PROVENANCE) · §11 · §12a–§12h · §13 · §14 · §15 · §16 · §17 · §18a/b/c · §19 · §22 · §29 · §32 · THE RELAYED-DIAGNOSIS LAW |
| `TASKBOARD.md` "## FINE-TUNE" | header, provenance, outcome law, D1/D2/D3, the stops, manager rulings 1–15, findings (i)–(v), TASK-476..500, the gated one-liners, the dependency frontier |
| The approved plan | `C:\Users\wesel\.claude\plans\inherited-bubbling-acorn.md`, all six stages + verification table + trap ledger |

⛔ **CODE CLAIMS WERE VERIFIED AT THE ARTIFACT, NOT BY SEARCH** (§14). Files read directly: `SiegeLlamaSpike.cpp` · `SiegeAssistantSnapshot.{h,cpp}` · `SiegeAssistantComponent.{h,cpp}` · `SiegeLlamaSettings.cpp` · `Tools/fetch_llm_model.py` · `Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/VERSION.md` · `.gitignore` · `Docs/Data/` (directory listing).

✅ **POSITIVE CONTROL BEFORE TRUSTING ANY ZERO.** `Docs/Data/` was enumerated by glob and returned the four files finding (i) names — so *"`assistant_eval_dev2.csv` does not exist"* rests on a listing that demonstrably finds files, not on a search that found nothing. Likewise every "absent" claim below (`audit clause`, `MaxSnapshotChars` in §12a) is paired with a search that **did** return hits elsewhere in the same file.

⚠️ **`SiegeLlamaSpike.cpp` MOVED UNDER ME MID-AUDIT.** `GetArgBool(Args, TEXT("chat"), …)` read at line **5034** early in this session and at line **5115** later, with `out=`/`ids=` parsing appearing in between — **TASK-477 is live in that file right now.** ⇒ Every claim I make about it is **by symbol** (§18c), and any line number I quote for it is already stale.

---

## §B — FINDINGS

Ranked by *would a task act wrongly on this*, and separated by routing class as the spec requires.

### 🔴 BLOCKER-1 — FALSE ASSERTION ABOUT CODE — the founding defect, still live in the law
**CONVENTIONS.md:1774** (FINE-TUNE §5, naming table) vs **the code** vs **the board** vs **the plan**.

Law, verbatim:
> | New spike flags | `repeats=<N>` (`SpikeEval`) · `out=<path>` · `ids=1` · `chat=0\|1` (`SpikePrompt`/`SpikeEval`) | `Siege.Llama.*` namespace, unchanged |

Board (`#### TASK-477`), verbatim:
> ⚖️ **AND THE PROVENANCE IS RECORDED AGAINST ME, NOT AGAINST THE SPEC: `chat=0|1` IS NOT IN THE APPROVED PLAN's STAGE A. I ADDED IT**, reasoning that M1's A/B needed it — **without reading the harness.**

Artifact (`SiegeLlamaSpike.cpp`, by symbol): `FSpikeOptions::bUseChatTemplate` is initialised **`true`**; `GetArgBool(Args, TEXT("chat"), Options.bUseChatTemplate)` sits in the **shared** `ParseOptions` beside `tier`/`grammar`/`ctx`; `BuildPrompt` already calls `llama_model_chat_template` → `llama_chat_apply_template(…, /*add_ass*/ true)` with a documented raw-concat fallback; and the pre-existing help string already advertises `chat=0|1`. **The flag is not new.** The approved plan's Stage A lists five items and `chat=` is not among them.

⛔ **WHY THIS IS THE TOP FINDING AND NOT A NIT: the board self-reported this defect and the LAW WAS NOT AMENDED.** The correction lives only in a task spec that will be `done` in hours; the naming table is the durable authority and it still calls a shipped, wired flag "new". ⚠️ **The board's own remedy for the identical class was applied one clause away** (ruling 15 struck `819 insertions` in place) — this one was left standing. **This is the exact shape TASK-500 was convened over, sitting inside the document convened to fix it.**

⚖️ **What it would cost:** a later reader of §5 — the table is explicitly *"the cross-task contract, every name pinned before its task issues"* — ships a duplicate `chat` on a shared parser. **Two spellings of one switch, disagreeing at some future edit**, which TASK-477 itself calls *"worse than the gap it was meant to fill."*
📌 **Not urgent for TASK-480:** its criterion (1) reads *"Check … `chat` defaults to 1"*, which is a verification, not an addition. The exposure is downstream.

---

### 🔴 BLOCKER-2 — CONTRADICTION — one prohibition is absolute in §5 and conditional in FINE-TUNE §8, and the batch's own doctrine says that is the wrong repair
**CONVENTIONS.md:640** (assistant §5) vs **CONVENTIONS.md:1802-1805** (FINE-TUNE §8).

Side A, verbatim (`:640`):
> **DEV/SPIKE CONSOLE COMMANDS REGISTER VIA `FAutoConsoleCommand` / `FAutoConsoleCommandWithWorld` IN A NEW FILE — NEVER as a `UFUNCTION(exec)` on a shipped class.** `USiegeCheatManager` and `ASiegePlayerController` stay untouched by this batch.

Side B, verbatim (`:1804`):
> ⇒ ✅ **A SHIPPED-LANE dev dump is the thing §5 was protecting, not the thing it was banning** … It sits beside the existing `SummonTestUnit` / `ApplyTestDamage`, which are that class's established idiom.

**The decision they disagree about:** whether `DumpAssistantPrompt` — a `UFUNCTION(exec)` on `USiegeCheatManager`, TASK-479's entire deliverable — is permitted. §5 says never, without qualification. §8 says yes, by reading §5's reason.

⛔ **AND THE LAW ITSELF RULES THAT THIS IS THE WRONG MECHANISM.** Twice, in this same file, in this same batch:
> **CONVENTIONS.md:668** — ⛔ **This is an AMENDMENT, not an exception and not a waiver** … ⚖️ **A letter that outlived its reason is REPAIRED, never granted an exception, because the next reader obeys the letter.**
> **CONVENTIONS.md:1071** — ⛔ **The clause is amended rather than waived — a letter that outlived its reason is repaired, never granted an exception**, because the next reader obeys the letter.

⚖️ **CUDA got the in-place repair (§6, and manager ruling 8 forbids anyone calling it an exception). §16 got the in-place repair (`:1434`, "UNTOUCHABLE FOR DELETION ≠ UNTOUCHABLE"). §5 did not — it got a reinterpreting clause 1160 lines away.** The next reader who opens §5 alone reads an absolute ban, and §5 is exactly the clause a QA gate quotes when reviewing a cheat-manager edit.
⇒ **This touches a Stage-A gate criterion** (`#### TASK-480` gates TASK-479), which is why it is cross-posted to 🚨 Blockers. **Manager's call, not mine:** amend §5 in place, or record why §8's separate-clause treatment is right here and wrong for CUDA.

---

### 🔴 BLOCKER-3 — STALENESS → a task will read a wrong number as a criterion
**CONVENTIONS.md:727-733** (§8 🔢 TOKEN PROVENANCE) vs **CONVENTIONS.md:1845** (FINE-TUNE §11) and **:1825** (gate clause 7), consumed by **`#### TASK-498`**.

Side A — §8's measured table, verbatim rows:
> | Zone A (static prefix) | 4314 | **1139** | 3.79 |
> | assembled prompt | 5349 | **1504** | 3.56 |
> … **the assembled prompt STRING is 1504 tokens** — ⚠️ **but what the CONTEXT holds is `turn1_prompt` = 1517** … **Quote 1504 for the string and 1517 for the context**

Side B — FINE-TUNE §11, verbatim:
> `max_seq_len = 2048` — ⚠️ **Zone A is 1362 tokens (5116 chars), NOT the pre-ladder 1139**; a sample is ~1767 tokens.

⛔ **BOTH ARE QUOTABLE AND THEY CANNOT BOTH DESCRIBE TODAY'S PROMPT.** §8's assembled figure was measured with Zone A at **4314 chars / 1139 tok**; the ladder wave then added **+802 chars / +223 tok** (§12g's own table: 1139 → 1356 → 1362; §12g line 1162 confirms the char landmarks *"5108, 5116"*). ⇒ The assembled prompt is now ~**6151 chars**, and **1504 / 1517 are pre-ladder readings that §8 presents with no strike and no "pre-ladder" label** — while §8's own single-source rule (`:737`) says *"any further change to these figures requires a NEW measurement."* The ladder took new Zone-A measurements and §8's table was never updated.

**Where a task acts wrongly** — `#### TASK-498` spec, verbatim:
> **Compare the Python token COUNT against the engine's printed `turn1_prompt`** (⚠️ **1504 is the STRING; 1517 is what the CONTEXT holds** …)

⇒ Build-master will print ~1727 against a pinned 1517 and must decide whether M4 failed. ⚠️ **Either outcome is wrong for the same reason: the criterion is a stale constant, and §12g explicitly forbids baking an unmeasured token constant into a check** (`:1164`, *"a test asserting a number nobody measured is a guardrail that reports safe"*).
✅ **The char half is clean:** `zoneA_chars=5116` is consistent everywhere (gate clause 7, TASK-481, TASK-489). It is only the token totals that rotted.

---

### 🟡 WARN-1 — CONTRADICTION (board-internal) — two commit hashes for one completion
**`#### TASK-499`** status, verbatim:
> status: **done** — commit **`f4f88e6`** (2026-08-03, build-master; +`7d82394` hash fill-in …) … ⚠️ **THIS BOARD EDIT IS NOT IN THE TASK-499 COMMIT** … **the commit is `.gitignore` + the handoff only.**

Dependency-order block at the foot of the FINE-TUNE section, verbatim:
> · ~~TASK-499~~ ✅ **DONE at `5e85389`** ·

⛔ **`5e85389` is the decomposition commit** (ruling 15: *"`TASKBOARD.md` + `CONVENTIONS.md` ONLY … It explicitly carries TASK-499's board status"*). The frontier line names **the commit that recorded the status** as the commit that **did the work**, and TASK-499's own entry says in bold that the two are different. **Whoever later audits "what shipped in the `.gitignore` fix" will open the wrong diff.**

### 🟡 WARN-2 — STALENESS — `main is 21 ahead` is frozen in two places, and the batch's own remedy is one clause away
Manager **ruling 12**, verbatim: *"⛔ **NO push** (`main` is **21 ahead of origin**)"* · **`#### TASK-481`** (5), verbatim: *"⛔ **NO push** (`main` is **21 ahead**)."*
Against **`#### TASK-499`**, verbatim: *"⛔ **unpushed** — `main` was **21 ahead before this task**; ⚠️ **don't quote a total, count it:** `git rev-list --count origin/main..main`"*

⇒ TASK-499's own commits **and** `5e85389` landed after that reading, so **21 is arithmetically stale in both places that state it as present tense.** ⚖️ **This is TASK-434's self-describing-count defect and ruling 15's own lesson** (*"STATE THE COMMAND, NEVER THE READING"*) unapplied to the two sentences beside it. TASK-481 is a *commit* task — the number is load-bearing there.

### 🟡 WARN-3 — STALENESS (self-describing count) — "12 sub-sections", but there are 13
Board FINE-TUNE header, verbatim:
> **Law was written FIRST (house rule):** CONVENTIONS **"⚖️ THE FINE-TUNE RUNG — QLoRA on `Qwen3-4B` (2026-08-03)"** (12 sub-sections)

Against **`#### TASK-500`** itself, verbatim: *"READ `CONVENTIONS.md` "⚖️ THE FINE-TUNE RUNG" **§1–§13** END TO END"* — and §13 exists (`CONVENTIONS.md:1862`), added on TASK-479's refusal (ruling 14). ⇒ **The header's count outlived its referent by one section, in the same file, on the same day.** Second instance of the class ruling 15 named.

### 🟡 WARN-4 — CONTRADICTION/STALENESS inside the law — §9 orders work that §7 records as finished
**CONVENTIONS.md:1812** (FINE-TUNE §9), verbatim:
> ⚠️ **VERIFIED LIVE, AND IT IS A REAL DEFECT IN `.gitignore` TODAY: `git check-ignore -v Models/README.md` → `.gitignore:132:/Models/`.** … ⇒ **Strike the dead negations or fix the rule, with `git check-ignore -v` output pasted into the handoff**

**CONVENTIONS.md:710** (assistant §7), verbatim:
> ✅ **RATIFIED 2026-08-03 — STRIKING THE NEGATIONS BEAT REPAIRING THEM** … ⇒ **Model documentation lives in `Docs/`, never `Models/`, and that sentence is now in `.gitignore` itself where the next author will meet it.**

Artifact: `.gitignore` now carries `/Models/` at **line 41** (not 132) under a written block beginning *"⛔ NOTHING UNDER /Models/ CAN BE RE-INCLUDED BY A `!` NEGATION"*, and the dead `!Models/…` negations are gone. ⇒ **§9 states an open instruction and a line number that are both false today; §7 states it is done.** ⚠️ `.gitignore` is **TASK-499-ONLY** by ruling 11, and TASK-499 is closed — so a reader obeying §9 edits an unowned file to redo finished work, on a file that is *"one keystroke from un-ignoring 2.5 GB of weights."*

### 🟡 WARN-5 — CROSS-REFERENCE TO A CLAUSE THAT DOES NOT EXIST
**CONVENTIONS.md:318**, verbatim:
> ✅ **It is NOT the committer's job to audit a body of law for internal consistency, and build-master correctly REFUSED to imply it had** — **see the audit clause below.**

**There is no audit clause below.** The RELAYED-DIAGNOSIS LAW ends at `:319`; `:321` opens an unrelated 2026-08-01 section. Searched the whole file for `audit clause` / `internal consistency` / `coherence`: **one hit, this one.** (Positive control: the same search style returns many hits for other phrases in this file.) ⇒ The clause that creates the duty **points at the clause that would define it, and that clause was never written** — the duty exists only as `#### TASK-500` on the board, which is not law.

### 🟡 WARN-6 — CROSS-REFERENCE COLLISION, SYSTEMATIC — "§11" means two different sections under one prefix
Two sections both number §1–§1x, and every board `names:` block prefixes citations with `CONVENTIONS "THE FINE-TUNE RUNG"`:

| Board task | cites | resolves to | intended |
|---|---|---|---|
| `#### TASK-487` | *"THE FINE-TUNE RUNG" §4, §10 · **§11** · §12a · §12h"* | FINE-TUNE §11 = **QLoRA frozen switches** | assistant §11 = corpus sealed/split — the spec text says *"(§11/§12a)"* about `dev2` being open |
| `#### TASK-482` | *"§1 (ruling 4), §7 · **§11** · §12a/§12b"* | same collision | assistant §11 |
| `#### TASK-484` | *"§5, §7 · **§11** · §12a …"* | same collision | assistant §11 |
| `#### TASK-495` | *"§6, **§11** (the logits term)"* | FINE-TUNE §11 ✅ | FINE-TUNE §11 |

⇒ **The same token, under the same prefix, means the assistant section three times and the fine-tune section once.** ⚠️ A reader chasing TASK-487's `§11` lands on `use_dora=False` and LoRA rank. §12a/§12b/§12f/§12h have no fine-tune counterpart so they self-disambiguate; **§1–§13 all collide.** ⚖️ §18c's remedy (*cite by symbol*) is the obvious fix and is the manager's to apply.

### 🟡 WARN-7 — CROSS-REFERENCE POINTS AT THE WRONG CLAUSE — inside TASK-500's own spec
**`#### TASK-500`** spec, shape 4, verbatim:
> ⛔ **A rule stated as absolute in one place and conditional in another** — **§12a's `MaxSnapshotChars` history is the worked example of how expensive that gets**

`MaxSnapshotChars` occurs **21 times** in `CONVENTIONS.md` — at `:719-814` (assistant §8/§10), `:1015` (§9c) and `:1523-1527` (§19). **It occurs zero times in §12a** (`:1052-1074`, the spent-seal law, which is about holdout generations). The absolute-vs-conditional worked example the spec means is **§8's seventh and eighth seams** (`:743-760`: *"THE 1085 RULING WAS CONDITIONAL. THE CONDITION WAS WRITTEN AS A RIDER ON THE CAP RATHER THAN ON THE ACT OF TRUNCATING"*) and **§19** (`:1519`: three roles, safe direction inverts). ⇒ Harmless to me — I found the right clause — **but it is the fifth shape occurring inside the sentence that asks me to hunt the fifth shape**, and it belongs on the record for that reason.

### 🟡 WARN-8 — DELIVERABLE NAMED IN LAW THAT NO BOARD LINE OWNS
**CONVENTIONS.md:1833-1834** (FINE-TUNE §10), verbatim:
> **RE-RUN AFTER ANY WEIGHT CHANGE:** bars #1–#5 (⛔ **bar #1 in PIE on `L_Arena` WITH UNITS ON THE FIELD**), the **33 automation tests**, and the **`bThrottleCPUWhenNotForeground=False` check**
> ⛔ **TIER CONSTANTS ARE RE-MEASURED, NEVER CARRIED** (`SPIKE_MEM stage=model_load`, all three tiers, twice, **worse reading wins**) — T9

The board's gated one-liner **E-4** covers *"the nine conjunctive clauses"* only; **E-1/E-2/E-3** cover the trainer, the VRAM ladder and the merge. **Neither the bars-and-suite re-run nor the tier re-measure appears anywhere on the board, not even as a one-liner.** ⚖️ The plan carries both in its verification table (row E), so this is board-side attrition, not a plan gap. ⚠️ **Boarding is exactly what D/E/F's one-liners exist to guarantee** — *"they get IDs only after TASK-486"* is a statement about IDs, not about existence. Not urgent (nothing dispatches until the stops clear); **it will be invisible at the moment it matters** unless it is written down now.

### 🟡 WARN-9 — BOARD vs ARTIFACT — TASK-479's accessor has landed; the board still reads it as pending
**`#### TASK-479`** status, verbatim: *"⚖️ **RULED 2026-08-03 — RE-DISPATCHABLE.**"* · **`#### TASK-480`** (9), verbatim: *"📌 **IF TASK-479 IS STILL UNIMPLEMENTED WHEN YOU RUN, GATE THE OTHER THREE AND SAY SO.**"*

Artifact: `SiegeAssistantComponent.h` now declares
> `FString DebugCaptureAndComposePrompt(const FString& RawUtterance);`

under `#if !UE_BUILD_SHIPPING`, above a ~90-line rationale block that names mechanism #2, quotes it, argues *"THIS IS AN ADDITION, NOT A RE-EXPOSURE"*, and cites **CONVENTIONS "THE FINE-TUNE RUNG" §13(b)** and **§13(c)** by symbol.

⚖️ **Reported as a SCHEDULE FACT, not a defect** (§14: *"a file owned by a running task is a moving target"* — and `SiegeLlamaSpike.cpp` moved 80 lines under me during this audit, so at least one Stage-A task is live). ⇒ **TASK-480 must re-read before invoking its criterion (9);** on today's tree the answer is that the deliverable exists.

### 🟡 WARN-10 — SUPERSEDED-BUT-UNSTRUCK inside one task
**`#### TASK-479`** spec (1), verbatim:
> It captures a live snapshot, composes the turn prompt **through the shipped path**, and writes the **exact bytes** plus `zoneA_chars` / `zoneB_chars` / `zoneC_chars` to a file and the log.

**`#### TASK-479`** ruling (R3), 20 lines above it, verbatim:
> ✅ **M3 needs the prompt BYTES, not `zoneA/B/C_chars`** … ⛔ **Do NOT add an out-param, a member, or a second `ReportFirstCapture` call to get the counts.**

⇒ **Spec (1) demands an output the ruling above it makes unobtainable.** The R3 block declares it overrides the spec, so the ordering resolves it — **but every comparable supersession in this batch used `~~strikethrough~~`** (ruling 15's `819`, TASK-480's criterion 6, §6(b)'s `after Super::RebuildWidget()`). ⚠️ **The one route left to satisfy the unstruck sentence is recomputing the zone counts in the cheat manager — the forbidden reassembly, T1+T8 in one move.**

---

### ⚪ NITs

- **NIT-1 — line citations already rotted (§18c's own subject).** `CONVENTIONS.md:1877` cites *"`ReportFirstCapture` (`:2902-2909`)"*; it is now at `SiegeAssistantComponent.cpp:2985`. `#### TASK-479`'s status cites *"`SiegeAssistantComponent.h:1200`, sole `private:` at 913, sole `public:` at 621"*; today the header reads **1306 / 1019 / 625**. ✅ `ComposeTurnPrompt` at `cpp:2727` and the four-mechanism block at `h:49-74` still hold. **All were true when written; the accessor's ~90 lines moved everything below it.**
- **NIT-2 — two ranges for one block.** `h:49-74` (§13, TASK-480) vs `h:49-65` (TASK-479 status). Both defensible (all four mechanisms vs through #2); **the block is 49–74 and mechanism #2 ends at 65.**
- **NIT-3 — stale ranges in headings.** *"## FINE-TUNE … TASK-476..499"* and *"Manager rulings (binding for TASK-476..499)"* — the section now contains TASK-500, and TASK-500's own `names:` says *"rulings 1–15 + TASK-476..499"*. Is TASK-500 bound by the rulings? Unstated.
- **NIT-4 — ordering.** Manager rulings run 1…11, **14**, 12, 13, **15**; the five findings run (i), (ii), (iii), **(v)**, (iv). Content is complete; **the numbering is the only thing out of order**, and a reader scanning for "ruling 12" passes it.
- **NIT-5 — possible double-count of the chat wrapper.** §12c derives its ceiling as *"1389 (Zone A) + 400 + 96 + ~13 (chat-template wrapper) = 1898 of 2048"* — **the wrapper is already inside the budget.** `#### TASK-489` (3) says *"the wrapper adds ~13 tokens — state the new total and check it against the budget."* Adding 13 to `zoneA_tok` before comparing to 1389 charges the wrapper twice. Harmless at today's margin; **name which side of the ceiling the wrapper sits on.**
- **NIT-6 — `t1` is invisible in the fine-tune law.** §5's table pins the training fixtures *"beside `t0`"*; §16's frozen list names `t0` only; D2/D3 discuss `t0` only. The file also carries **`SpikeRosterT1` / `SpikeFixtureT1`**, `VerifyFixtureKindParity` is called on **both**, and a `static_assert` binds their kind counts. `#### TASK-478`'s acceptance criterion names only `t0`. **`t1` exists for bar #3's worst-case bound** (its own comment says so) — a subset-parity change that overlooks it is a real seam.

---

## §C — CLASSIFICATION SUMMARY (they route differently)

| Class | Findings | Routes to |
|---|---|---|
| **FALSE ASSERTION ABOUT CODE** | BLOCKER-1 | manager — amend §5's table; nothing to fix in code |
| **CONTRADICTION** (two clauses disagree) | BLOCKER-2 · WARN-1 · WARN-4 · WARN-10 | manager — rule which side stands; ⛔ do not let both keep standing |
| **STALENESS** (a clause outlived its referent) | BLOCKER-3 · WARN-2 · WARN-3 · WARN-9 · NIT-1 | manager for the law; the owning task for WARN-9 |
| **BROKEN / MIS-AIMED CROSS-REFERENCE** | WARN-5 · WARN-6 · WARN-7 · NIT-2 | manager — §18c, cite by symbol |
| **UNOWNED DELIVERABLE** | WARN-8 | manager at D/E/F decomposition |

---

## §D — THE NULL RESULT, STATED AS A RESULT

⚖️ **Everything below I checked and it came back CLEAN. The recipient holds the same files and can verify every line** (§12b's certificate standard).

**Law vs code — verified at the artifact, N = 0 defects:**
- **D1** — `USiegeAssistantComponent::ComposeTurnPrompt` concatenates raw; `BuildPrompt` in the spike applies `llama_chat_apply_template`. ✅ The divergence is real and correctly described.
- **D2** — `MaxRosterKinds = 8` at `SiegeAssistantSnapshot.h:342`; `SpikeRoster` carries **13** rows; the spike hardcodes `Out += TEXT("other_kinds: none\n");` at `SiegeLlamaSpike.cpp:632` under the comment *"Nothing collapses in EITHER fixture by construction"*; the shipped builder computes both branches at `SiegeAssistantSnapshot.cpp:1315/1319`. ✅ **The manager's first-hand D2 correction is right on every limb** — key set identical, the diff is five missing rows plus the `other_kinds:` value, and the spike genuinely **cannot express collapse on any fixture**.
- **D3** — `VerifyFixtureKindParity` requires equal row count **and** identical kind at each index against `SpikeRoster`; `BuildSpikeGrammar` derives from `SpikeRoster`. ✅ Correctly described (the *order* requirement is stricter than the law states — relevant to TASK-478's design, not a defect in the law).
- **§6 toolchain** — `VERSION.md` carries commit **`221f0f6356efe2260023208365705ec5d5a7c8f5`**, artifact sha256 **`f3ab5195…`**, zip **`llama-b10235-bin-win-vulkan-x64.zip`**, and *"Deliberately excluded from `bin/Win64/`: … all `.exe`"*. ✅ All four claims exact.
- **§6 donor convention** — `Tools/fetch_llm_model.py` prints `CHECK_VERDICT: {'PASS' if ok else 'FAIL'}`, reports `HF_TOKEN` as `SET`/`NOT SET` only, and its module header carries *"HF_TOKEN IS ENV-ONLY AND MUST NEVER BE PRINTED"*. ✅ The convention §6 tells `finetune_env.py` to mirror is really there.
- **§9 packaging** — `ModelParams.load_mode = LLAMA_LOAD_MODE_MMAP` in both lanes; `USiegeLlamaSettings::ResolveModelPath()` resolves cmdline → settings → `ProjectDir/Models` + default filename. ✅ *"`ResolveModelPath()` needs no change"* and *"swapping models never needs a rebuild"* both hold.
- **finding (i)** — `Docs/Data/` holds exactly `cards.csv`, `assistant_eval_dev.csv`, `assistant_eval_holdout.csv`, `assistant_eval_holdout2.csv`. ✅ `dev2` absent; the dependency inversion is real.

**Law vs plan — N = 0 unrecorded drifts, 1 recorded:**
- §10's **nine gate clauses** match the plan's nine **clause for clause**, including *minimum of 5*, `PassLenient − DegenerateQuestionFloor ≥ 7`, the refuse-class veto and `zoneA_chars=5116`. ✅
- §4's two stops and three decision rules match the plan's pre-registered block. ✅
- §11's recipe (`r=16/alpha=32/dropout=0.05`, `use_dora=False`, `use_rslora=False`, empty `modules_to_save`, completion-only loss, the logits term, the lever order) matches. ✅
- §7's composition percentages, four authoring stages and the disposition ceiling match. ✅
- **Deliberate corrections, recorded and legitimate:** the B1/B2 split of Stage B (finding (i)) · M4's engine/Python split (finding (ii)) · TASK-499 pulled ahead of everything (finding (iii)) · the `LoadCorpus`-fatal decision boarded rather than inherited (finding (iv)) · the `check-ignore` reading laws (finding (v)). **The one un-recorded drift is BLOCKER-1** — and even that is self-reported on the board, just not in the law.

**Internal consistency — N = 0 further contradictions found in:** §1 provenance (both sentences genuinely co-exist) · §2 outcome law (and its symmetric anti-trap) · §3's D2 escalation logic incl. the *"D2 bites only above the cap ⇒ `tA`–`tF` at ≤ 8 kinds are lane-identical"* consequence · §7's four data rules · §8's exec reasoning (its *content* is sound; only its *placement* is BLOCKER-2) · §12's two open contracts · §13's observer ruling — **which is the model the other conflicts should have followed:** it amends ruling 11 in place, names what it overrides, and its conditions reappear verbatim as TASK-480 criteria and again in the shipped header comment. ✅
**And ruling 15's `819 → 847` correction is a completed instance of the defect class, struck in place with the command stated.** That one is fixed; **WARN-2 and WARN-3 are the two it missed.**

---

## §E — WHAT I DID NOT AUDIT (stated plainly, per §20's *"say what you did NOT run"*)

1. ⛔ **I HAVE NO GIT ACCESS — BY ROLE DESIGN, AND I HAD NO SHELL IN THIS SESSION.** I could **not** run `git show --stat 5e85389`. ⇒ **The 847 figure is UNVERIFIED by me**, as are the hashes `5e85389`, `f4f88e6`, `7d82394`, `21f7e01` and the ahead-count. **WARN-1 and WARN-2 are internal-consistency findings only** — they say the board disagrees with itself, not that any hash is wrong. **Someone with a shell must close them.**
2. **Nothing was run.** No compile, no editor, no `SpikeEval`, no automation suite. Every code claim is source-level.
3. **No sealed corpus was opened** — `holdout.csv`, `holdout2.csv` untouched, `holdout3.csv` does not exist. None was involved.
4. **Line-anchored claims about `SiegeLlamaSpike.cpp` are stale on arrival** — TASK-477 was writing it during this audit (a symbol moved 81 lines between two reads). I cite it by symbol only.
5. **Out of scope, deliberately:** CONVENTIONS sections outside the LLM/assistant/fine-tune lanes (art, terrain, M8 networking, deck-builder) and board batches other than FINE-TUNE. I did **not** sweep them for §22's shape.
6. **I did not gate TASK-476/477/478/479's code.** That is `#### TASK-480`. WARN-9 is an observation about the board's currency, not a verdict on the accessor.
7. **I did not verify the ~800-row composition, the VRAM arithmetic (~5.6–6.9 GB vs ~7.3–7.7 GB) or the latency figures** — they are derivations the plan owns and labels as derived.

---

## §F — NOTES FOR THE MANAGER (⛔ REPORTED, NOT RESOLVED)

⚖️ **Five decisions are yours. I state the question, never the answer:**

1. **BLOCKER-1** — does §5's naming table get the `~~struck~~` treatment ruling 15 used, or a footnote? **Either way the table currently pins a flag that shipped before this batch existed.**
2. **BLOCKER-2** — is assistant §5 amended in place (the CUDA/§16 precedent, and §6's own *"repaired, never granted an exception"*), or does FINE-TUNE §8's separate-clause treatment stand? **If it stands, §6's doctrine sentence needs a scope, because it currently forbids what §8 did.**
3. **BLOCKER-3** — §8's token table needs either a re-measurement or a `PRE-LADDER` label, and TASK-498's `1504` / `1517` parenthetical needs to point at whichever survives. ⚠️ **§12g forbids substituting a derived correction** — 1727 is *my* arithmetic, not a measurement, and must not be written into the law on my say-so.
4. **WARN-5** — the audit duty is referenced in law and defined nowhere. If TASK-500's shape should recur per batch, it belongs in the RELAYED-DIAGNOSIS LAW where `:318` already points.
5. **WARN-6** — two sections numbered §1–§1x under one citation prefix. §18c already prescribes the fix.

📌 **For build-master, whenever it next commits law:** WARN-1 and WARN-2 are cheap to close with a shell and impossible to close without one.

⚖️ **And the finding that is not a finding:** build-master's refusal to imply it had audited coherence was correct, and this report is the evidence — **the six changes were each present and each code-accurate, and the file still contained three blocking incoherences.** ⛔ **Presence is not coherence** was the right sentence to say out loud.
