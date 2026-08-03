# QA Report — TASK-433
**THE OWED DOC-VS-CODE AUDIT — §8's clauses read against the shipped snapshot builder**

Verdict: **FAIL** — 2 BLOCKER · 8 WARN · 3 NIT · 6 candidates killed

⚠️ **The verdict is about the SHIPPED BUILDER, not about §8's prose.** Both blockers are defects in
`SiegeAssistantSnapshot.cpp` that §8 does not describe. Nothing here reverses a §8 ruling.

---

## 0. SCOPE, TREE, AND WHAT I COULD NOT RUN (§9c's duty)

⛔ **I could not run git or read file timestamps — this session has no shell tool.** I am not asserting a HEAD.
The tree I read is whatever was on disk at the time of this pass, 2026-08-03. **Somebody with Git must record
the hash beside this report**; treat every line number below as valid against *that* commit and re-check them if
the files moved.

**READ IN FULL (raw `Read`, never `Grep` — see NOTE-1):**
- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.h` (485 lines)
- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.cpp` (963 lines)
- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantVocabulary.cpp` (207 lines)
- `Source/GitClaudeUnrealTest/Siegebound/TeamId.h` (41 lines)

**READ IN PART:** `SiegeAssistantGrammar.cpp` (:230-545 raw), `SummonedUnit.cpp` (:1930-1963 raw),
`Castle.h` (:140-215), `Plugins/SiegeLlama/.../SiegeLlamaSpike.cpp` (:155-215, :330-600, :4190-4250),
`Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/include/llama.h` (:200-213, :375-390, :548-559 raw).

**NOT OPENED, DELIBERATELY:** `Docs/Data/assistant_eval_holdout.csv` (spent) and
`assistant_eval_holdout2.csv` (sealed — TASK-432's one-shot). Neither was needed; no finding here depends on a
corpus row, and no corpus sentence, Id or expected field is quoted or paraphrased anywhere in this report.

**TASK-428 CONCURRENCY (spec item 3), stated explicitly.** TASK-428 is editing **Zone A's TEXT** in this same
file. My subject is the **budget and enforcement path**, which lives in `BuildZoneB` / `AppendRosterBlock` /
`BuildZoneC` / `SanitizeForPrompt` and in the four constants — **a Zone-A text edit does not move any of it.**
Two findings DO touch Zone A and are marked **⏳ RE-CHECK AFTER TASK-428** where they appear: **WARN-3**
(the dead `llama_kv_cache_seq_rm` comment sits inside `BuildZoneA`'s body at `cpp:543`) and the corrected
line anchor for §9c's few-shot symbols (`cpp:673-678`). Everything else is stale-proof against that task.

**📥 THE BATCHED LIST — I owe a verdict on every line in it. IT IS EMPTY.** `CONVENTIONS.md:795` still reads
*"EMPTY at ruling time"* and no line has been appended below it. ⚠️ **Per the ledger's own instruction, that is
NOT a clean bill and I am not reporting it as one** — it means the STOPPING RULE has not yet had to catch
anything, not that §8's text is exhausted. **Verdicts owed: 0. Verdicts delivered: 0.** The audit below is
therefore entirely the code-side dimension, which is what it was created to supply.

---

## 1. CLAUSE ENUMERATION — FOUR VERDICTS, WITH COUNTS

| # | §8 clause | Verdict | Settled by |
|---|---|---|---|
| 1 | Three zones, their order, which builder emits which | **AGREES** (with a caveat, §2.9) | `cpp:548` `[RULES]` · `cpp:692` `[MATCH]` · `cpp:812` `[FORCES]` + `cpp:857` `[ORDER]` |
| 2 | Zone A byte-identical for process life — **and by what mechanism** | **AGREES**, mechanism is real and two-part | `cpp:538-681` (reads no member state) + `SiegeAssistantVocabulary.cpp:37-43, 55-58` (string sort, not FName) |
| 3 | ≤400-tok cap governs B+C only, never Zone A | **AGREES in scope · DISAGREES in strength** | `cpp:864` — see **WARN-1** |
| 4 | `MaxSnapshotChars = 1085` and **every site that reads it** | **AGREES** — 3 sites, 1 enforcing, 0 pre-filters | `h:225` · `cpp:864` (only enforcing) · `cpp:911`, `cpp:921` (log text) · spike `:174`/`:4207` print-only |
| 5 | `ZoneBCharReserve = 192` against a 68-char Zone B | **AGREES** — and it is the binding constraint | `h:233` · `cpp:864` — see **WARN-2** |
| 6 | `MaxRosterKinds = 8`, collapse path, cause bit, escalating latch | **AGREES** — all four | `h:241` · `cpp:866` · `cpp:782-797` · `cpp:899-902` · `cpp:913-922` |
| 7 | `MaxUtteranceChars = 240` on BOTH `order:` and `pending:`, neither budget-truncated | **AGREES** — WARN-1 of `qa/TASK-416.md` is **CONFIRMED** | `h:244` · `cpp:853`, `cpp:856` · `cpp:928-962` — and see **BLOCKER-1/2** |
| 8 | Fixed key order · every key always emitted · `none` for empty · bands · no timestamps/coords | **AGREES** — verified key by key | `cpp:694-735` (Zone B, 4/4) · `cpp:814-858` (Zone C, 7/7) · `PlaceLocations` never printed |
| 9 | §4's **six** `TActorIterator` passes, once per sentence, never per tick | **DISAGREES** (count) · **NOT REACHED** (per-tick) | `cpp:201`, `cpp:295`, `cpp:373` + 5 traversals inside helpers — see **WARN-7** |
| 10 | §9a place vocabulary character-for-character; near/far derived not stored | **AGREES** (7/7 spellings, derivation) · **DISAGREES** (order) | `cpp:52-61` · `cpp:251-279` — see **WARN-6** |
| 11 | The Zone-A byte-identity **QA criterion** ("call it twice") | **UNTESTABLE BY READING** | Parser not run: **no automation test constructs a `USiegeAssistantSnapshot`.** Only `Tests/SiegeAssistantGrammarTest.cpp` exists and it never does |
| 12 | §8's `ZoneA_tokens` startup budget assertion | **NOT IMPLEMENTED** (expected — TASK-423's) | no `llama_n_ctx` / `llama_tokenize` in the game lane |

**COUNTS: `AGREES` 7 · `DISAGREES` 3 (two of them partial) · `NOT IMPLEMENTED` 1 · `UNTESTABLE BY READING` 1.**

---

## 2. FINDINGS — RANKED BY WHETHER THEY PRODUCE A WRONG NUMBER OR A SILENT FAILURE

### ⛔ BLOCKER-1 — `SanitizeForPrompt` truncates the utterance AND the pending line SILENTLY, and §8's re-anchored condition binds it by its own words

`SiegeAssistantSnapshot.cpp:939-942`, reached from `cpp:853` (`pending:`) and `cpp:856` (`order:`):

```cpp
if (Out.Len() >= MaxUtteranceChars)
{
    break;
}
```

**No log. No latch. No marker in the prompt.** The cut is at exactly 240 TCHARs, mid-word, and what reaches the
model is a *grammatically complete-looking* order line that ends wherever the counter ran out.

**§8 does not merely fail to cover this — it covers it and the code does not comply.** The re-anchoring ruled at
`CONVENTIONS.md:718` is deliberately written to survive exactly this kind of exemption:

> **THE OBSERVABLE-TRUNCATION CONDITION BINDS *ANY ENFORCEMENT OF THE SNAPSHOT BUDGET, BY ANY MECHANISM, UNDER
> ANY NAME*** … **IF THE SNAPSHOT REACHES THE MODEL SHORTER THAN IT WAS BUILT, SOMETHING SAYS SO.**

`h:304-306` exempts this path by calling it *"a separate, always-on sanitiser"* rather than a budget. **That
exemption is the defect.** The clause was anchored onto **the act of truncating**, not onto the constant that
performs it, precisely so that renaming the cutter cannot orphan the condition — and "sanitiser" is a name.

**Why this is strictly worse than the roster case the condition was written for.** The roster collapse leaves
`other_kinds: 5 kinds, 9 units` in the prompt, so the model is *told* something was hidden (`cpp:796`). A
truncated `order:` line tells it nothing at all: the model receives a half-sentence as if it were the whole
request and the grammar guarantees it produces a well-formed command from it. That is the
valid-shaped-wrong-command failure §1 exists to prevent, arriving through the one string the player actually
typed.

**And the `pending:` half is worse again.** §1 makes that line **the entire mechanism by which a clarification
turn carries context forward** without feeding the model its own output. Silently cutting it corrupts the FSM's
own carried state, and the FSM has no way to learn that it happened. §8's own example pending line
(`pending: guard footman×10 -> ancient_ground_near ; problem: only 8 available`) is ~72 chars, so today's
authored lines are safe — **which is exactly the profile of a latent defect: it will first fire on a pending
line a future FSM task makes longer, and it will fire silently.**

**Reachability: immediate.** Any typed or pasted utterance over 240 characters. `MaxUtteranceChars`'s own
rationale at `h:243` is *"a pasted paragraph must not become the whole context"* — **the anticipated case is the
silent one.**

**Direction of fix: CODE.** `SanitizeForPrompt` must report that it cut, and which of the two lines it cut,
under the same escalating-latch discipline `BuildZoneC` already uses at `cpp:913-922`. (⚠️ It is `static` and
takes no `this`, so the latch has to move or the function has to stop being static — that is the implementer's
call, not mine.)

---

### ⛔ BLOCKER-2 — the char caps count UTF-16 code units; the budget they proxy for is tokens over UTF-8, and the 2.71 calibration is ASCII-only. The one string that comes from outside the program is the one nothing constrains.

`SiegeAssistantSnapshot.cpp:944-957` filters exactly four characters — `\n`, `\r`, `\t`, space — and appends
**everything else verbatim**:

```cpp
const bool bIsSpace = (Character == TEXT('\n')) || (Character == TEXT('\r'))
    || (Character == TEXT('\t')) || (Character == TEXT(' '));
...
Out.AppendChar(Character);
```

**The file's own law does not reach it.** `cpp:41-44` states:

> *"⚠️ **ASCII ONLY in every prompt literal in this file.** The prompt is a byte budget and a tokenizer input,
> not a doc comment: a stray multi-byte glyph costs tokens, and makes the character-count cap stop matching the
> byte count a QA reviewer measures."*

**That is the correct hazard, correctly reasoned — and it is scoped to LITERALS.** Literals are author-written
and were never the risk. The `order:` line is player-written. **The assumption crossed the authorship boundary
(author-controlled text → player-controlled text) with its name and its value unchanged, and its meaning
changed: it stopped being true.** That is the UNCHECKED CARRY, on the code side, in the class §8 named — and it
is the fifth kind the audit was commissioned to look for.

**What it costs, concretely.** `MaxSnapshotChars = 1085` is derived at `h:169` as `400 tokens × 2.71
chars/token`, where 2.71 was measured on **ASCII** Zone B+C content. `FString` is UTF-16 on Windows, so
`Out.Len()` counts **code units**:

- 240 TCHARs of CJK ≈ 720 UTF-8 bytes and tokenizes at roughly 1–2 chars/token on this tokenizer — **2–3× the
  tokens the 2.71 calibration admits for the same `Len()`.**
- Emoji are **surrogate pairs: 2 TCHARs each**, commonly 3–4 tokens each. 240 TCHARs of emoji is ~120 glyphs
  ≈ **360–480 tokens from the `order:` line alone**, against a **400-token B+C budget** — while
  `MaxSnapshotChars` reports 240 of 1085 consumed and the roster-trim loop at `cpp:870-880` sees nothing wrong.
- **Doubled**, because `MaxUtteranceChars` applies to the pending line too.

⇒ **The budget can be exceeded by ~3× with every char-based check reporting healthy.** No log, no crash, no
wrong-looking number — the exact signature §9c catalogues.

**Second, smaller half: the cut can split a surrogate pair.** `break` at exactly 240 can land between the high
and low surrogate of one emoji, emitting a lone surrogate into the prompt string. What the UTF-8 conversion
downstream does with that is not this file's contract to guarantee.

**Direction of fix: CODE, and it is TASK-423's to size.** §8 needs no correction here — it never made a claim
about non-ASCII input, which is precisely why nothing caught it. Two obvious closes (restrict the utterance to
a safe charset, or count the cap in UTF-8 bytes) are the implementer's choice; **the finding is that neither
exists.**

---

### ⚠️ WARN-1 — `MaxSnapshotChars` is not an enforced cap on Zone B + Zone C. Nothing in the repo sums the two and compares them to it.

§10 states the enforcement as *"Zone B+C ≤ `MaxSnapshotChars`"*. **The code enforces something narrower.**
`cpp:864`:

```cpp
const int32 RosterBudget = MaxSnapshotChars - ZoneBCharReserve - Head.Len() - Tail.Len();
```

Only the **roster block** is bounded, and only against a **reserve** standing in for Zone B. `BuildZoneB()` and
`BuildZoneC()` never see each other; the caller concatenates. **The only place in the entire repo that computes
`ZoneB.Len() + ZoneC.Len()` is a `UE_LOG` in the spike** (`SiegeLlamaSpike.cpp:4194`, printed at `:4207-4209`).

The gap is closed today only by arithmetic, not by code — and I checked it rather than assuming: Zone C's
`Head` maxes at **108** chars and `Tail` at **~587**, so `RosterBudget` is never below **~198** and the whole of
Zone C is held under `1085 − 192 = 893`. **The guarantee therefore rests entirely on `ZoneB.Len() ≤ 192`, and
that is the one thing the code does NOT enforce** — `cpp:740-746` logs a Warning once and proceeds:

```cpp
if (Out.Len() > ZoneBCharReserve && !bWarnedZoneBOverReserve)
```

⇒ **A future key added to Zone B puts B+C over 1085 with a Warning and no truncation.** This is the "§8 says
enforced, the code only logs" shape exactly. Today Zone B is four fixed keys at 68 chars, so it is latent.

**Direction: doc first (state that the authority is a roster budget with a Zone-B reserve, not a B+C cap), code
second (TASK-423 owns the real check and must not inherit the assumption).**

---

### ⚠️ WARN-2 — `ZoneBCharReserve = 192` over-charges a 68-char Zone B by 124, and that over-charge — not the 1085 cap — is what makes the 13-kind case marginal

Confirmed at `h:233` and consumed at `cpp:864`. Both §8 and the header already name the over-charge
(`h:220-223`: *"the honest lever is ZoneBCharReserve … re-measure it, do not eyeball it"*). **What neither
quantifies is what it buys, so here it is, computed from the builder's own format strings on the sealed
fixture's t0 board:**

| | reserve = 192 (shipped) | reserve = 68 (measured) |
|---|---|---|
| Roster budget, empty pending + 61-char order | 627 | 751 |
| 13-kind roster block | 621 → **6 chars spare** | 621 → **130 chars spare** |
| Worst case (240-char order **and** 240-char pending) | **3 of 13 kinds print** | **6 of 13 kinds print** |

⇒ **The reserve costs three printed kinds in the worst case, and it is ~95 % of the reason the header's own
warning (*"887 against 893"*, `h:216`) reads as marginal.** The header's arithmetic there is **correct** — I
re-derived it: Zone C at 13 kinds is exactly 887 against the 893 available. It is correct about a margin that a
badly-sized reserve created.

⚠️ **`192` has no recorded derivation anywhere** — not in the header, not in §8, not in §10. Per §8's own line
(`CONVENTIONS.md:758`) *"A BUDGET MAY BE A CONSTANT. A MEASUREMENT MAY NOT."* — 192 is legitimately a budget, so
it may be a constant. **It is simply the wrong constant, and it is the only one in this file with no stated
provenance.**

**Direction: CODE.** ⚠️ **And it is not free** — lowering the reserve raises the roster budget, which makes the
13-kind case fit, which makes the `MaxRosterKinds` collapse *less* visible. Whoever moves it re-reads
`cpp:892-922` in the same pass.

---

### ⚠️ WARN-3 — `llama_kv_cache_seq_rm` does not exist in the vendored build. It is named in §8 and in three shipped comments. This is the recorded `use_mmap` defect, recurring in the same section.

The vendored header carries **`llama_memory_seq_rm`** (`llama.h:735`). **There is no `llama_kv_cache_seq_rm`
anywhere in `llama.h`** — I grepped the whole header for `seq_rm`; the eight hits are all `llama_memory_seq_*`.

The dead symbol survives in:
- `CONVENTIONS.md:686` — *"This is what lets `llama_kv_cache_seq_rm` keep the prefix"*
- `SiegeAssistantSnapshot.h:280` ⏳ *(Zone-A adjacent — re-check after TASK-428)*
- `SiegeAssistantSnapshot.cpp:543` ⏳ *(inside `BuildZoneA`'s body — re-check after TASK-428)*
- `SiegeAssistantVocabulary.h:52`
- `TASKBOARD.md:4288`, `:4634`, `:5055`

**§8 names both spellings, four lines apart.** `CONVENTIONS.md:668` correctly says *"Bar #3 IS
`llama_memory_seq_rm` prefix reuse"*; `:686` says `llama_kv_cache_seq_rm`. **One of them is a symbol a search
cannot find.**

**The plugin lane is RIGHT and the game lane's comments are wrong** — `SiegeLlamaSpike.cpp:2819` and `:2830` call
`llama_memory_seq_rm`, and `:3163` correctly sets `ModelParams.load_mode = LLAMA_LOAD_MODE_MMAP`. So this is
purely a documentation/comment defect with no runtime consequence **today** — the game lane never links llama.

⚠️ **Report it anyway, because §8 already ruled on this exact class and it recurred inside the ruling's own
section.** `CONVENTIONS.md:805`: *"`use_mmap` is the name a search will fail to find, and **'the field is gone'
must never be read as 'the requirement is gone.'**"* TASK-423 is the first task that will grep for the KV-reuse
mechanism, and the mechanism it will find named in the file it is implementing does not exist.

**Direction: DOC + the three game-lane comments.**

---

### ⚠️ WARN-4 — every game-lane `file:line` anchor in §8/§9c is stale (7 of 7). Every vendored-header anchor is exact (3 of 3).

§8's STOPPING RULE instructs future authors to record *"the code symbol that would settle it"*, and §9c makes
the landed artifact the authority. **The coordinates currently on offer do not land on the code they name.**

| Cited in CONVENTIONS | What is actually there now | Correct anchor |
|---|---|---|
| `SiegeAssistantSnapshot.h:186` (`MaxRosterKinds`) | prose inside `MaxSnapshotChars`'s comment | **`h:241`** (decl) · **`cpp:866`** (use) |
| `SiegeAssistantSnapshot.cpp:775-789` (collapse) | inside `AppendRosterBlock`, off by ~4 | **`cpp:779-797`** |
| `SiegeAssistantSnapshot.cpp:873` (truncation warning) | `AppendRosterBlock(RosterBlock, KindsToPrint);` | **`cpp:892-922`** (predicate `cpp:893`) |
| `SiegeAssistantSnapshot.cpp:666-669` (few-shots name footman/sorcerer/archer) | a comment about few-shot *shapes* | **`cpp:673-678`** ⏳ *TASK-428* |
| `SiegeAssistantGrammar.cpp:250-262` (kind alts from `GetUnitKinds()`) | Shipping-build guard rationale | **`cpp:495-508`** |
| `SiegeAssistantGrammar.cpp:312` (`at_least` rule) | a `UE_LOG` format string | **`cpp:570`** (now `at-least`) |
| `SiegeAssistantGrammar.cpp:392` (the reference) | a charset-loop `break` | **`cpp:654`** |
| `SiegeLlamaSpike.cpp:318` (declared 13-kind deviation) | the few-shot transcription note | **`cpp:341-371`** |
| ✅ `llama.h:382-385` (abort_callback CPU-only) | **EXACT** — verified verbatim | unchanged |
| ✅ `llama.h:205-211` (`llama_load_mode`) | **EXACT** | unchanged |
| ✅ `llama.h:551-556` (requested vs actual) | **EXACT** | unchanged |
| ✅ `Castle.h:190` (`FindNearestCastleForTeam`) | **EXACT** | unchanged |

⚠️ **The pattern is the finding, not the offsets.** The anchors that rotted are precisely the files under active
edit; the ones that held are the frozen vendored header and a file this batch does not touch. **A stale anchor
into a heavily-commented file is worse than a missing one, because it lands on prose that reads plausibly** —
`h:186` lands inside a comment *about* the cap, and a hurried reader would accept it.

**Direction: DOC.** Recommend recording line numbers only alongside the **symbol name**, which does not rot.

---

### ⚠️ WARN-5 — `Capture(World, ETeamId::Red)` prints `0 orderable, 0 followable` for every kind and says nothing

§8's 🚩 flag at `CONVENTIONS.md:810` is **ACCURATE AND STILL LIVE** — verified by raw read, not by grep:

```cpp
// SummonedUnit.cpp:1940-1943
return CanFollowHero() && Team == ETeamId::Blue && !bDead && !bAIFrozen;
// SummonedUnit.cpp:1956-1959
return CanTakeZoneOrders() && Team == ETeamId::Blue && !bDead && !bAIFrozen;
```

**What §8 does not say is what the prompt then contains.** `cpp:771-775` prints all three numbers
unconditionally, so a Red capture emits `- footman: 8 total, 0 orderable, 0 followable` for **every** kind —
not an absent column, but a **positively false statement** that no unit can be commanded. Per `h:84-89` those
two columns exist specifically to stop an unnecessary clarification turn; zeroed, they *cause* one on every
selection-bearing order.

`ETeamId` is `{Blue, Red}` only (`TeamId.h:14-18`), the parameter is public and unvalidated, and **nothing logs
that the columns are structurally zero.** §8 correctly rules the fix belongs in the shipped predicates at M8 P2
and correctly keeps the parameter. The gap is that a one-line "Team != Blue ⇒ eligibility columns are
structurally zero" log would cost nothing and would remove this from the "no crash, no error, no log line"
list §9c keeps.

**Direction: CODE (a log), doc unchanged.**

---

### ⚠️ WARN-6 — §9a lists the place vocabulary in a different order than the code's contract, and the code makes order load-bearing. It has already been misread once, on disk.

`SiegeAssistantSnapshot.cpp:35-39` states the contract in as many words:

> *"⚠️ **ORDER IS PART OF THE CONTRACT.** Zone A prints in this order (and Zone A must be byte-identical for the
> life of the process), `GetPlaceNames()` returns in this order, and TASK-417's grammar generates its `where`
> alternation from that array. Appending is safe; **reordering rewrites Zone A and throws away every cached
> prefix.**"*

`PlaceVocabulary[]` (`cpp:52-61`) is **`own_castle, enemy_castle, mid, …`**. §9a (`CONVENTIONS.md:921`) and §8
(`:798`) both list **`enemy_castle · own_castle · mid · …`**.

**All seven spellings match character-for-character — the pin itself is intact.** §9a never claims to pin the
sequence, and a careful reader would notice. **A careful reader already did not:** `SiegeLlamaSpike.cpp:439`
reads *"The seven pinned place symbols, **in the pinned order** (CONVENTIONS section 9a)"* and then lists them
in the **code's** order, not §9a's. The spike happens to be right; it is right by having read the code.

⇒ **Anyone who "conforms" the code to §9a's listing destroys the KV prefix**, which §8 calls a QA FAIL, for a
reason no log line names.

**Direction: DOC** — one sentence in §9a stating it pins the **set and spelling**, and that
`SiegeAssistantSnapshot.cpp`'s `PlaceVocabulary[]` is the **order** of record.

---

### ⚠️ WARN-7 — §4's "six `TActorIterator` passes" is literally false. The header is honest; the document is not.

`Capture` runs **three** direct `TActorIterator` loops — hero fallback `cpp:201`, capture zone `cpp:295`,
summoned units `cpp:373` — plus **five** traversals inside shipped finders: `FindNearestCastleForTeam` ×2
(`cpp:234-235`), `FindNearestAncientGround` ×2 (`cpp:260-263`), `FindBestMineFor` ×1 (`cpp:285`). **Up to eight
traversals, seven when the hero is possessed.**

**The header already says so** (`h:247-258`): *"Two of them are called TWICE … which is why the honest traversal
count is higher than six."* §4 (`CONVENTIONS.md:609`) says *"does **six `TActorIterator` passes**"* and is
quoted as the rejection's justification.

Consequence is small but real: §4's whole argument is *"~0.2 ms we never pay per frame"*, and the number that
argument rests on is understated by ~33 %. **The conclusion survives comfortably** — the code is right, the
rejection stands, and I am not re-opening it.

**"Never per tick": NOT REACHED — see WARN-8.**

**Direction: DOC** — say "six logical passes / up to eight traversals", which is what the header says.

---

### ⚠️ WARN-8 — the entire class has ZERO callers and ZERO tests. §8 and §10 reason about an "interim state" that has never executed.

`USiegeAssistantSnapshot::Capture`, `BuildZoneA`, `BuildZoneB`, `BuildZoneC` and `ResolvePlace` are **called
from nowhere in `Source/`.** The only references outside the file are comments. The only test file in the lane
is `Tests/SiegeAssistantGrammarTest.cpp`, which **never constructs a snapshot** (its single match on
"Snapshot" is a comment at `:1166`).

This is expected — TASK-423 is the first caller — but it changes how two live §8/§10 sentences should be read:

- `CONVENTIONS.md:745`: *"the interim state IS better than the 1440 it replaced … **because the truncation
  became visible**"*
- `CONVENTIONS.md:973` (§10): the condition *"**is LANDED** … UNDER QA, NOT VERIFIED"*

**"Landed" is true — the code is present, correct, escalating, and names its cause bit.** "Became visible" is
not: **nothing calls it, so it has never emitted a line, and it cannot until Wave 1.** And `MaxSnapshotChars`
governs no executing code path at all today — **1085 and 1440 are equally inert right now.** The argument that
one interim state is better than the other is an argument about code that will run later.

⚠️ **This is §8's own `ZoneA_tokens` shape in a third costume:** *"A CHECK THAT SURVIVES IN **FORM** WHILE
LOSING THE **PROPERTY** THAT MADE IT A CHECK"* (`CONVENTIONS.md:758`). Here the lost property is not a movable
operand — it is **execution**.

**I verified from the code that it WOULD fire, which is the strongest statement available without a run.** On
the sealed fixture's own t0 board (13 kinds) the shipped builder prints 8, collapses 5, and takes the
`CollapsedKinds > 0` branch at `cpp:893` on the **first sentence**, with cause `"the MaxRosterKinds cap"`
(`cpp:902`) — Verbose every turn (`cpp:908`), Warning on first and every escalation (`cpp:913`).

**Direction: DOC** — say "landed, unexecuted (no caller yet)", not "became visible"; and **TASK-423 owes the
first observation**, which is the parser §9c says a reviewer must name when it was not run. **I am naming it:
the parser here is a PIE run with `LogSiegeAssistant Verbose` and a >8-kind board.**

---

### 🔎 SCOPE FINDING (no severity — it errs safe, but the noun is wrong): `955 chars / 352 tok` is the SPIKE FIXTURE's Zone B+C, not the shipped builder's

§8 (`:689`) and §10 (`:973`) both call **955 chars = 352 tok** the *"As-built B + C"*. **The shipped builder
cannot emit 955 on that board, because it honours `MaxRosterKinds = 8` and the fixture deliberately does not**
(`SiegeLlamaSpike.cpp:341-371`, a deviation §8 declares elsewhere at `CONVENTIONS.md:802`).

Computed from `SiegeAssistantSnapshot.cpp`'s own format strings, same t0 board:

| | Zone B | Zone C | B+C |
|---|---|---|---|
| Spike fixture, 13 kinds | 68 | **887** | **955** |
| **Shipped builder, cap honoured** | 68 | **670** | **738** |

⇒ The 2.71 chars/token ratio that **sizes the authority constant** was measured on a string the shipped path
does not produce. **Direction of error is safe** (the shipped string is shorter, the headroom figures are
pessimistic), and the composition is near-identical so the ratio itself should hold. **But it is instance 6's
tell firing again, one section later:** *"as-built"* consumed as a different noun than the one measured. §8's
own rule (`:709`) — *"The 2.71 is a property of today's content… whoever changes Zone C's shape re-measures its
ratio"* — has a standing exception nobody wrote down: the fixture's shape is not the builder's shape.

**Direction: DOC** — one qualifier: *"as-built **of the 13-kind spike fixture**; the shipped builder at
`MaxRosterKinds = 8` emits 738 on the same board."*

---

### NITs

- **NIT-1 — `QuantizeHealthBand` ROUNDS where `QuantizeGoldBand` FLOORS, and only the flooring is argued.**
  `cpp:117`: `RoundToInt(Percent / 10.f) * 10` ⇒ a castle at **95 % prints `100%`**. `cpp:122-131` argues
  explicitly that gold must floor because *"a band that rounds up tells the model the player can afford
  something they cannot"*. The identical argument applies to own-castle HP: rounding up says "your castle is
  fine" when it is not. §8 says only "quantized into bands" and does not choose. Small, but the two functions
  sit six lines apart and reason oppositely.
- **NIT-2 — the header's Zone-C key list omits a key the cpp emits.** `h:297-299` lists *"places, roster,
  stances, hero, pending, order"*; `cpp:802-803` lists *"places, roster, **other_kinds**, stances, hero,
  pending, order"*. The cpp is right (`other_kinds` is emitted unconditionally at `cpp:790-797`). Neither
  mentions the `[ORDER]` section marker at `cpp:857`. Under the fixed-key law an incomplete key list in the
  public docstring is the one that a caller will read.
- **NIT-3 — the degenerate-map identity guard can leave `ancient_ground_near` naming the ENEMY-side ground.**
  `cpp:259` falls back to `ReferenceLocation` (the hero) when there is no standing own castle; `cpp:275` then
  drops `ancient_ground_far` if it resolved to the same actor. With the own castle destroyed and the hero deep
  in enemy territory, `near` resolves to the far ground and `far` disappears. **Effectively unreachable in
  shipped 1v1** — the own castle being destroyed ends the match — and recorded only so the next reader does not
  rediscover it as new.

---

## 3. CANDIDATES I KILLED (the method's own evidence — §11's relayed-diagnosis law applied to myself)

Six hypotheses were derived and then settled against the artifact. **Five died. One survived and became a
positive result.**

- **KILLED — "3× castles mean `own_castle_hp` describes one of three, and `own_castle` moves as the hero
  moves."** Derived from the board's *"3×-castle"* language plus `FindNearestCastleForTeam(World, Team,
  ReferenceLocation)` being hero-anchored (`cpp:234`). It would have been a live Zone-B instability attacking
  the KV-prefix argument directly. **Killed by `Castle.h:184`: *"With exactly one own castle per match the two
  can never disagree about the winner."*** CASTLE-3X is 3× **scale** (hollow, walk-in), not three castles.
- **KILLED — "`RosterBudget` can go negative and the shrink loop emits an over-budget block."** Killed by
  arithmetic on the code's own maxima: `Head ≤ 108`, `Tail ≤ ~587` ⇒ `RosterBudget ≥ ~198`, against a 45-char
  minimum block at `KindsToPrint == 0`.
- **KILLED — "removing a kind can GROW the roster block (the collapse line gains digits), so the loop may not
  converge."** Killed by inspection: the first removal is **−46** chars of row against **+12** of collapse-line
  text (`other_kinds: none` → `other_kinds: N kinds, M units`); strictly decreasing, with a `KindsToPrint <= 0`
  floor at `cpp:875`.
- **KILLED — "the `EnemyTeam` ternary at `cpp:175` mishandles a third team value."** `ETeamId` is `{Blue, Red}`
  only (`TeamId.h:14-18`). Total.
- **KILLED — "the truncation warning may not actually fire."** It fires on the sealed fixture's own board — see
  WARN-8.
- **✅ SURVIVED AS A POSITIVE RESULT — "the spike fixture is a transcription, so the 68/887 anchors that size
  `MaxSnapshotChars` may be measurements of a string the builder does not produce."** This is §9c's
  same-authorship law pointed at §8's most-quoted operands, and it was the single highest-value thing this
  audit could have found. **I hand-counted both zones from `SiegeAssistantSnapshot.cpp`'s own format strings —
  not from the spike, not from the handoff — and got exactly 68 and exactly 887.** Zone B: `8 + 19 + 21 + 10 +
  10 = 68`. Zone C: `108 (head) + 621 (13-kind roster block) + 158 (tail) = 887`. ⇒ **The fixture is
  byte-faithful to the builder's format, and the operands under 1085 are sound.** Reported as loudly as a
  defect: §9c predicts a transcription *can* hide a shared error, and here, checked from the other side, it
  did not.

---

## 4. NOTE-1 — THE `Grep` TRAP, RE-CONFIRMED A THIRD TIME, IN A NEW FILE (spec item 5)

Recommending promotion into **CONVENTIONS §10's compile/tooling trap list**, from my own evidence in this pass.
⛔ **I am not making the edit** — manager's, per spec item 4.

`Grep` on `SummonedUnit.cpp` returned:

```
1936-	\ CONVENTIONS §3 \ the §7 pin: the class predicate plus the shipped
1949-	\ ORDERS by TASK-396: R (Hold) / F (Ambush) only. Name and signature are KEPT
```

Raw `Read` of the same two lines returns:

```
1936		// CONVENTIONS §3 / the §7 pin: the class predicate plus the shipped
1949		// ORDERS by TASK-396: R (Hold) / F (Ambush) only. Name and signature are KEPT
```

**`//` renders as `\`, and an inline `/` renders as `\` too.** A reviewer reading only the Grep output sees
what looks like a line-continuation or a malformed comment inside a shipped predicate and has a plausible
BLOCKER in hand. `qa/TASK-416.md` NOTE-1 recorded this twice on `SiegeAssistantGrammar.cpp`; **this is the
third occurrence and the first in a different file**, so it is a property of the tool on this machine, not of
one file's encoding.

**Proposed §10 line:** *"⛔ **`Grep` mangles `//` and `/` in comment text on this machine (renders both as
`\`).** Never read comment or syntax-level content from `Grep` output — confirmed three times, across two
files, twice nearly producing a false BLOCKER. Use raw `Read` for anything structural; `Grep` is for locating,
never for reading."*

---

## 5. IS §8 NOW AUDITED IN BOTH DIMENSIONS?

**Partly — and I will say exactly where the road ended rather than round up.**

✅ **AUDITED, and I consider these closed against the code:** all four constants and every site that reads them ·
the roster collapse path, its cause bit and its escalating latch · the fixed-key law in both zones (key by key)
· the no-coordinate / no-timestamp law · the band quantizers · the place vocabulary's spellings and the
near/far derivation · the eligibility-predicate reuse · the 68/887 operands under `MaxSnapshotChars`, now
independently re-derived from the builder's own format strings.

⚠️ **NOT AUDITED — I ran out of road, and the reason is the same in every case: THERE IS NOTHING TO RUN.**
- **Zone A byte-identity across two calls** — argued from construction, never executed. No test exists.
- **The truncation log's actual output** — traced to the branch, never observed.
- **The KV-prefix reuse claim** — belongs to the plugin lane and to TASK-423; unreachable from the game lane.
- **Anything about tokens** — the game lane has no tokenizer, so every token figure in §8 remains the spike's.
- **The whole `Capture` → assemble → infer path** — **it has no caller.**

⚠️ **AND THE HONEST FRAMING OF WHAT THAT MEANS.** §8's standing gap said the code *"has never been read against
these clauses"*. **It has now been read.** It has still never been **run** against them, and §9c is explicit
that reading is the floor and not the ceiling. **A second audit is owed at TASK-423** — not of the prose, and
not a re-read of this file, but of the first execution: the first `BuildZoneC` that logs a collapse, the first
`BuildZoneA` compared byte-for-byte against its own second call, the first assembled prompt whose B+C is
tokenized rather than counted. **Recording that here so its absence stays visible, in exactly the shape §8
recorded this audit's absence.**

**Nothing in §9c, §10, §11 or §12 produced a finding beyond WARN-3/4 and NOTE-1** — §9c's law was applied
rather than audited, §11's corpus is out of scope by seal, and §12 governs a wave whose code does not exist
yet. **Saying so rather than padding.**

---

## Notes for build-master

⛔ **Nothing here is committable code.** This report is read-only output; **I edited no file, including the
board.** Board text is returned to the orchestrator separately.

- **TASK-434 may commit this report as a docs-only artifact.** It contains **no holdout content** — no
  sentence, Id or expected field from either generation.
- **BLOCKER-1 and BLOCKER-2 are `SiegeAssistantSnapshot.cpp` fixes and need a programmer task + a compile.**
  Both live in `SanitizeForPrompt` / its call sites and are **outside** TASK-428's Zone-A text, so they do not
  serialize behind it.
- **WARN-3's three game-lane comment fixes DO touch `BuildZoneA`'s body — sequence them AFTER TASK-428** or
  they will conflict.
- The doc-side items (WARN-1, WARN-2 framing, WARN-4, WARN-6, WARN-7, WARN-8 status wording, the scope
  qualifier, NOTE-1's §10 promotion) are **manager edits to CONVENTIONS**, not mine and not build-master's.
