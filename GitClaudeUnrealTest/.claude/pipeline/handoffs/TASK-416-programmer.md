# TASK-416 — [LLM-A3] `USiegeAssistantSnapshot` — `Capture` + the three-zone serializer

**Agent:** gameplay-programmer · **Status:** ready-for-qa · **Date:** 2026-08-02

**M8 DECLARATION DUTY (verbatim, as required):**
> adds no replicated property, no new replicated class, no new relevancy tier.

---

## Files touched

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.h` | **NEW** — `FSiegeAssistantRosterEntry`, `USiegeAssistantSnapshot` |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.cpp` | **NEW** — `Capture` + the three zone builders |

**Nothing else was touched.** No `SiegePlayerController.*`, no `SummonedUnit.*`, no `MinerUnit.*`, no `GitClaudeUnrealTest.Build.cs`, no `Content/`, no compile, no Git, no editor, no MCP. `git status --porcelain` shows my two files as the only `??` entries under `Siegebound/` attributable to this task.

**Assets referenced:** `/Game/Data/DT_Cards` (soft path, resolved null-safe at use time, read-only — used only for the fixed roster sort order). No new asset is required by this task; `DA_AssistantVocabulary` is TASK-421's and arrives through `BuildZoneA`'s parameter.

---

## Signatures — §9 registry, character-for-character

All eight pinned members are implemented exactly as pinned: `Capture` / `BuildZoneA` / `BuildZoneB` / `BuildZoneC` / `GetRoster` / `GetPlaceNames` / `GetUnitKinds` / `ResolvePlace`, plus `FSiegeAssistantRosterEntry {Kind, Count, GroupId}` and `MaxSnapshotChars = 1440`.

**Two deliberate, declared supersets of the pin — please confirm both at TASK-419:**

1. **`GITCLAUDEUNREALTEST_API` on the UCLASS.** The registry writes `UCLASS() class USiegeAssistantSnapshot : public UObject`. Every other game-module class here carries the export macro (`UDeckLibrary`, `ASummonedUnit`, …), and it changes no pinned member signature. Flagged rather than assumed.
2. **`USTRUCT` boilerplate on the roster entry** (`GENERATED_BODY()` + `UPROPERTY()` per field). Required by UHT; the registry's shorthand omits it exactly as it does for every other struct in that list.

**The dispatch prompt described this as a `UBlueprintFunctionLibrary` static in the `USiegeCombatStatics` / `UDeckLibrary` idiom. I did NOT build it that way** — CONVENTIONS §5 and the §9 registry both pin it as a `UObject` with instance methods, and `Capture` writes member state that four const builders read. The pinned law won; noting it so the discrepancy is visible rather than silent.

---

## The measurement that matters: token budget

Character counts are measured from the actual emitted bytes (Zone A extracted programmatically from the compiled literals, not hand-copied).

| Zone | Chars | ≈ tokens @3.6 | Budget | Verdict |
|---|---|---|---|---|
| A (static) | **2158** | ~600 | ~350 | over — see below, and it is not a defect |
| B (slow) | **71** | ~20 | ~120 | well under; reserve is 192 |
| C (fast, typical) | **521** | ~145 | ~150 | on budget |
| **SNAPSHOT = B + C (typical mid-match)** | **592** | **~164** | **≤400 (`MaxSnapshotChars` 1440)** | ✅ **41 % of cap** |
| **SNAPSHOT = B + C (heavy: 8 kinds + collapse + pending + long utterance)** | **821** | **~228** | ≤400 | ✅ **57 % of cap** |

### Two budget interpretations I had to make — both are QA decision points

**(1) `MaxSnapshotChars` covers ZONE B + ZONE C, not Zone A.** The board says over-cap truncation "must never truncate Zone A or the utterance", and the three-zone budget is itself `A+B+C ≈ 620` tokens — so a 400-token cap covering A would leave B+C about 50 tokens, which is not a readable snapshot. The cap is therefore enforced on what `Capture()` produced. This is written into the `MaxSnapshotChars` doc comment with the reasoning.

**(2) Zone A is ~600 tokens against a "~350" guidance, and shrinking it would make the spike WORSE.** This is the one number I want QA to actually think about rather than reflex-fail:

- The hard cap is on the snapshot, and the snapshot is at 41 % of it. Nothing is over a *cap*.
- Zone A is prefilled **once** and cached, so its size costs turn-1 TTFT only. Bar #2 measures TTFT **with a warm prefix**, i.e. with Zone A already cached — so Zone A's size barely touches it.
- **Bar #3 (KV reuse, "~70 % drop in prefill tokens turn 1 → turn 2") gets BETTER as Zone A grows,** because the drop ratio is `(B+C) / (A+B+C)`. At the shipped sizes: `165 / 765` ⇒ a **78 % drop — passes**. Had I cut Zone A to the nominal 350, it would be `165 / 515` ⇒ a **68 % drop — marginally fails the bar.** Trimming Zone A toward its estimate would be optimising the wrong number and could turn a GO into a GO-WITH-RESCOPE.
- Context headroom is fine: 600 + 230 + 96 output ≈ 930 of `ContextTokens` 2048.
- The "~350" was written before the schema existed and before ruling 15 added the multi-kind `who` shape and the `ask` branch.

If it must come down anyway, the two levers are named in the code in priority order: the `intents:` glossary (~330 chars) and one few-shot (~140 chars). I recommend neither until TASK-413 has real numbers.

---

## The real captured snapshot (typical mid-match board)

Zones B and C exactly as `BuildZoneB()` + `BuildZoneC("send 10 footmen with a sorcerer to the nearest ancient ground", "")` emit them. **592 characters.**

```
[MATCH]
own_castle_hp: 80%
enemy_castle_hp: 60%
mid: neutral
gold: 120
[FORCES]
places: own_castle, enemy_castle, mid, ancient_ground_near, ancient_ground_far, nearest_mine, hero
roster:
- footman: 14 total, 14 orderable, 14 followable
- archer: 6 total, 6 orderable, 6 followable
- cleric: 2 total, 0 orderable, 2 followable
- sorcerer: 1 total, 1 orderable, 1 followable
- miner: 4 total, 4 orderable, 4 followable
other_kinds: none
stances: free 7, following 12, holding 8, ambushing 0
hero: alive
pending: none
[ORDER]
order: send 10 footmen with a sorcerer to the nearest ancient ground
```

Note the Cleric row: `2 total, 0 orderable, 2 followable`. That is the Cleric-follows-but-cannot-hold split arriving in the prompt **as data**, straight from the shipping predicates — not re-encoded here.

Zone A in full (2158 chars, with `synonyms: none`; `DA_AssistantVocabulary` adds its table there):

```
[RULES]
Turn ONE Siegebound order into ONE JSON command. Output the JSON object only: no prose, no explanation.

schema (a command):
{"intent":INTENT,"who":WHO,"where":WHERE,"when":WHEN}
INTENT = send | guard | ambush | follow | charge | fallback | rally
WHO    = [{"kind":KIND,"n":COUNT}] with 1 to 3 entries, or "all", or "none"
KIND   = a unit symbol from roster in [FORCES]
COUNT  = 1 to 30, or "all"
WHERE  = a place symbol from places in [FORCES], or "none"
WHEN   = "now", or {"kind":KIND,"at_least":1 to 30}

schema (a question, when the order cannot be translated):
{"ask":ASK}
ASK = which_unit | which_place | how_many | which_intent | unsupported

intents:
send = move the selected units to a place
guard = station them at a place and hold it
ambush = station them at a place and let them chase kills
follow = they follow the hero
charge = whole army attacks; who and where are "none"
fallback = whole army defends home; who and where are "none"
rally = hero rallies units near him; who and where are "none"

places (fixed vocabulary; only those listed in [FORCES] exist this match):
own_castle = the player's castle
enemy_castle = the enemy castle
mid = the capturable centre zone
ancient_ground_near = the ancient ground on the player's side
ancient_ground_far = the ancient ground on the enemy side
nearest_mine = the best gold mine for the player now
hero = where the player's hero stands

rules:
- Symbols only. Never a coordinate, distance, direction or actor name.
- Ask for the count the player said even if the roster holds fewer; the game reports the shortfall.
- One order in, one command out. You never see an earlier turn.
- If the order is not one of the seven intents, return a question instead of guessing.

synonyms:
none

examples:
order: send ten footmen with a sorcerer to the ancient ground on our side
{"intent":"send","who":[{"kind":"footman","n":10},{"kind":"sorcerer","n":1}],"where":"ancient_ground_near","when":"now"}
order: all archers guard the middle
{"intent":"guard","who":[{"kind":"archer","n":"all"}],"where":"mid","when":"now"}
order: everyone attack
{"intent":"charge","who":"none","where":"none","when":"now"}
```

---

## ⚠️ THE FINDING QA SHOULD CHECK FIRST — Zone A had to be rewritten against TASK-417's real grammar

**Zone A teaches the schema; `USiegeAssistantGrammar::Build` constrains it. There is NO compile-time link between them.** If they disagree, constrained decoding fights the few-shots on every token and bar-#5 accuracy collapses for a reason no log line names.

I first wrote Zone A from the approved plan's sketch (`"select":[{"kind","count"}]`). When TASK-417's files landed in the tree mid-task I read them and **corrected Zone A to the grammar's actual emission**, verified rule by rule against `SiegeAssistantGrammar.cpp`:

| | Plan sketch (what I wrote first) | TASK-417's actual grammar (what ships) |
|---|---|---|
| selection key | `select` | **`who`** |
| per-pair count key | `count` | **`n`** (`count` is the GBNF *rule* name, not the JSON key) |
| army-wide selection | `[]` | **`"none"`** |
| question branch | absent | **`{"ask":ASK}`**, 5 codes |

**QA acceptance suggestion:** diff the Zone A `schema` block against `SiegeAssistantGrammar.cpp`'s `root` / `command` / `question` / `who` / `selection` / `item` / `count` / `at_least` rules. A one-line comment block above the schema in `BuildZoneA` records this tie and states that an edit to either file must edit both — that comment is currently the *entire* enforcement mechanism, and it is worth a WARN if QA thinks it needs more.

## ✅ The `nearest_mine` / `own_castle` pin — checked against the sealed corpus, no change needed

The mid-task instruction listed the vocabulary as `enemy_castle · my_castle · mid · …`. **I verified against the sealed corpus on disk rather than the message**, and `Docs/Data/assistant_eval_{dev,holdout}.csv` assert **`own_castle`** (DEV-16, HOLD-06) — not `my_castle`. That matches CONVENTIONS §8 and the board spec, and it is what I implemented. `my_castle` appears nowhere on disk; I read that as a transcription slip in the dispatch, and no code change was needed.

`nearest_mine` is character-for-character as required (DEV-23, HOLD-12 — the holdout row that scores the gate). All six corpus-asserted place symbols match my vocabulary exactly:

`own_castle` ✓ · `enemy_castle` ✓ · `mid` ✓ · `ancient_ground_near` ✓ · `ancient_ground_far` ✓ · `nearest_mine` ✓ (+ `hero`, which my vocabulary carries and the corpus never asserts — a harmless superset).

The corpus also validates two design choices: DEV-01 is my few-shot #1 verbatim in shape (`footman|sorcerer`, `10|1`, `ancient_ground_near`), and HOLD-06 ("guard our castle with 6 knights" ⇒ emit 6 even though 3 are alive) is exactly the behaviour Zone A rule 2 teaches.

## ✅ Eligibility — zero disagreement is structural, not lucky

The instruction asked me to flag any disagreement with the corpus's shipped-code reading. **There cannot be one:** `orderable` is literally `IsGroupCommandEligible()` and `followable` is literally `IsFollowCommandEligible()`, called per unit. I never reimplemented either. Confirmed by reading `SummonedUnit.cpp:1915-1960`: `CanTakeZoneOrders()` ⇒ `Profile == Standard`; `CanFollowHero()` ⇒ `Standard || Support`. So Cleric = follow-only, Ogre/Sapper = neither, Sorcerer = `Standard` ⇒ orderable (which is what makes the flagship sentence executable), Miner = both via its overrides.

---

## Design decisions QA should scrutinise

1. **⚠️ `IsFollowCommandEligible()` / `IsGroupCommandEligible()` hardcode `Team == ETeamId::Blue`.** So `Capture(World, ETeamId::Red)` returns a roster with correct totals but **all-zero** `orderable`/`followable`. Correct for v1 (host/standalone, local player is always Blue) but the `ETeamId` parameter implies a generality the availability columns do not have. Recorded rather than worked around — "fixing" it here would mean reimplementing the predicates, which is banned.
2. **The honest traversal count is not six.** Six *logical* passes, but: 5 always-on `TActorIterator`s are not what runs — I run 2 here (`ACaptureZone`, `ASummonedUnit`) plus up to 1 conditional hero fallback, and the shipped finders traverse 5 more internally (`FindNearestCastleForTeam` ×2, `FindBestMineFor` ×1, `FindNearestAncientGround` ×2). ~8 traversals worst case. I chose that over re-deriving their loops: reuse is explicitly required, and the alternative is a second copy of a QA'd selection rule that drifts. Still unmeasurable next to one inference call, and still once per sentence.
3. **No unit registry, no actor cache, no dirty flag, no subscription list** (CONVENTIONS §4). The object holds no actor pointers or weak pointers at all — every field is re-derived per `Capture`, so a unit dying between sentences cannot leave a stale row.
4. **The char-budget truncation path is close to unreachable in practice, by design.** `MaxRosterKinds = 8` bounds the roster block at ~412 chars, and head+tail are bounded by `MaxUtteranceChars = 240` on both the utterance and the pending line — so the budget maths never binds on a realistic board. It is a genuine backstop, not routine behaviour. **To exercise it, temporarily lower `MaxRosterKinds` or `MaxSnapshotChars`** — QA should not expect to trigger it by stacking units.
5. **Roster order = `UDataTable::GetRowNames()` order**, which is the table's internal order and is fixed for a loaded table (a reimport is a content-lock-time change, like a vocabulary change). Never count, never distance. Fallback on an unresolvable `DT_Cards` is lexical `FName::Compare` — still fixed, still deterministic — logged once. **`FName` comparison is case-insensitive, so the lower-cased canonical symbols find the `"Footman"`-cased row keys without a second map.**
6. **Sort predicates are made TOTAL on purpose** (`TArray::Sort` is introsort, not stable): kinds break ties on `Kind.Compare`, roster rows on `Kind.Compare` then `GroupId`. Two rows can never swap between sentences.
7. **`GetUnitKinds()` is never truncated**, even when the printed roster collapses into `other_kinds:`. The grammar must still admit every kind the player owns, or a legitimate order becomes unsayable — the §1 failure this design exists to prevent.
8. **Quantisation:** HP to 10 % steps with a **standing castle floored at 10 %** (0 % would read as destroyed, and destroyed castles are dropped from the vocabulary entirely by the finder); gold **floored** to 10s, never rounded — a band must not overstate what the player can afford.
9. **`mid` is reported as `ours` / `theirs` / `neutral` / `none`, never Blue/Red.** A colour is one more thing for a small model to get backwards; ownership is resolved relative to the ordering team at capture time.
10. **Places resolve to slots, and only resolved slots are published.** A destroyed castle, a map with no capture zone, or a dead hero simply drops out of `GetPlaceNames()` and therefore out of the grammar — it can never be named. There is also an identity guard so a degenerate single-ancient-ground test map cannot publish `near` and `far` pointing at the same actor.
11. **`SanitizeForPrompt`** flattens newlines/tabs/whitespace runs and caps at 240 chars on **both** the utterance and the pending line. The layout is line-oriented, so unsanitised player text could forge a key (`order: hi\nplaces: enemy_castle`). Worth a look as the one security-shaped thing in this file.
12. **`ResolvePlace` leaves `OutLocation` untouched on failure** — a caller ignoring the return value gets its own initialised value, never a plausible-looking origin it might march an army to.
13. **Prompt literals are pure ASCII** (comments use the house `⚠️`/`§` freely). Keeps the character cap equal to the byte count a reviewer measures, and avoids tokeniser noise. Files carry no BOM, matching every shipped file in the module.

## Known non-defects (do not open a loop on these)

- **This file cannot compile alone, by design** (manager ruling 8). It includes TASK-417's `SiegeAssistantCommand.h` (for `LogSiegeAssistant` — no second category is declared anywhere) and `SiegeAssistantVocabulary.h`, and calls TASK-418's `AAncientGround::FindNearestAncientGround`. Both are now on disk; the batch compiles as one UBT unit at TASK-420.
- **`ACastle::FindNearestCastleForTeam` is the Follow batch's parked, UNCOMMITTED work** sitting in the working tree (`Castle.h:190`). I built against it as instructed and it is genuinely required here; it is not shipped, and this file will not build until the Follow batch lands.
- **`ASiegePlayerController::FindUnitGroup` is read, not edited.** Used only to label the stance tally, inside the unit loop, never cached — the shipped aliasing rule ("read within the call stack, never cache"). `FindControllerForTeam` is the resolve (the M8 TEAM LAW; `GetFirstPlayerController` is banned and is not used).
- **Built against manager ruling 15 (multi-kind) and the castle-helper clause**, as requested for the TASK-419 check. No fresh castle `TActorIterator` exists in this file.

## Flagged for a later decision (not blocking)

- **The `ask` branch has no few-shot,** because the three-example count is pinned by CONVENTIONS §8. If TASK-413's bar-#5 run shows the model never declining (DEV-02 / HOLD-06-style rows), a fourth shot is the cheapest first fix — a Zone A change costs one cache warm-up and no code anywhere else.
- **Castle HP names only the NEAREST castle per side** under the 3× castle law. Lossy but cheap; adding standing-counts would have meant a fresh castle iteration, which is now a QA FAIL.
- **`MaxSnapshotChars = 1440` is a 3.6 chars/token guess and must be corrected from TASK-413's real tokeniser count**, never guessed a second time. At the measured 592 chars for a typical snapshot there is a large margin either way.

---

## POST-SUBMIT FIX — two compile errors surfaced by TASK-401's gate (fixed pre-gate)

TASK-401 (the Follow lane's compile gate) swept this file mid-write — `Source/GitClaudeUnrealTest/` is one UBT module, so file-disjointness is not build-disjointness — and surfaced:

```
SiegeAssistantSnapshot.cpp(338,58) : error C2228 — left of '.LoadSynchronous' must have class/struct/union
SiegeAssistantSnapshot.cpp(338,31) : error C2737 — 'CardTable': const object must be initialized
```

**Both are real, both were mine, both are fixed.** One line changed, at what is now `SiegeAssistantSnapshot.cpp:351`.

### ⚠️ The root cause is NOT the include law — please do not record it as one

The relayed diagnosis was "a complete-type include-law violation (criterion 10): a `TSoftObjectPtr<UDataTable>` whose type is only forward-declared; add `#include "Engine/DataTable.h"`."

**`Engine/DataTable.h` was already included, at line 6, from the first version of the file.** Adding it would have changed nothing and both errors would have survived into TASK-420. A genuinely missing complete type also produces a *different* diagnostic — C2027 "use of undefined type" — not C2228 on a name that resolves fine.

The actual cause is **C++'s most vexing parse**:

```cpp
const TSoftObjectPtr<UDataTable> CardTableAsset(FSoftObjectPath(CardTablePath));   // line 337, BEFORE
```

`FSoftObjectPath` is a **type** and `CardTablePath` is an **identifier**, so `FSoftObjectPath(CardTablePath)` parses as a *parameter declaration* — a parameter named `CardTablePath` of type `FSoftObjectPath`, the redundant parens around a parameter name being legal. The statement therefore declares a **function** `CardTableAsset` returning `const TSoftObjectPtr<UDataTable>`. The next line applies `.` to a function name ⇒ **C2228 at col 58**, and the failed initialiser leaves the `const` local with none ⇒ **C2737 at col 31**. Both diagnostics on line 338, both explained by line 337, exactly as reported.

**Fix — braces, which cannot be parsed as a parameter list:**

```cpp
const TSoftObjectPtr<UDataTable> CardTableAsset{ FSoftObjectPath(CardTablePath) };   // AFTER
```

Verified against the engine on disk rather than assumed: `TSoftObjectPtr` has `explicit TSoftObjectPtr(SoftObjectPathType ObjectPath)` constrained to exactly `FSoftObjectPath` (`SoftObjectPtr.h:302-307`) — direct-list-initialisation calls it correctly — `LoadSynchronous() const` (`:547`) is const so the `const` local is fine, and `explicit FSoftObjectPath(const WIDECHAR*)` (`SoftObjectPath.h:94`) accepts the `const TCHAR* const` constant. A 14-line comment now sits above the line explaining the trap so nobody "tidies" the braces back to parens.

**Why this file hit a trap no other file in the repo hits:** every shipped call site passes a string **literal** — `TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Cards.DT_Cards")))` — and a literal can never be a parameter name, so the ambiguity never arises. Hoisting the path to a named constant (which I did so the log message and the resolve share one string) is what re-opened it.

### Sweep result: one instance, file-wide

Swept both files with a regex for the vexing shape `Decl Name(TypeName(identifier));`. **Exactly one hit — line 337, now fixed.** The three `TActorIterator<T> It(World)` loop-inits are safe because `World` is a variable, not a type, so they cannot parse as parameter declarations. `FName(*CardID.ToString().ToLower())` is safe because the argument begins with `*`, which cannot start a declaration.

Since the compiler stops at the first error in a translation unit, I also re-verified the API calls I could not compile-check, against the UE 5.8 headers on disk: `FName::Compare` (`NameTypes.h:943`), `FString::AppendChar` / `TrimEndInline` (`UnrealString.h.inl:434` / `:1685`), and `FString`'s range-for `end()` stepping back over the null terminator (`:286-287`) — which is what makes `SanitizeForPrompt`'s `for (const TCHAR Character : In)` correct. `static constexpr` members inside a `UCLASS` and plain nested `enum class` inside a `UCLASS` both have shipped precedent in this module (`AGoldNode::DrainTickIntervalSeconds`; `ASiegePlayerController::EPlacementInvalidReason` / `EGroupPickStage`).

**Not compiled** — TASK-420 owns that.

---

## TASK-433 — the two BLOCKERs from the doc-vs-code audit (`qa/TASK-433.md`)

**Files touched: `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.{h,cpp}` and nothing else.**
Not compiled (no gate scheduled), not committed, board untouched. Neither holdout CSV opened.

### BLOCKER-1 — the silent cut is now observable on TWO channels

`SanitizeForPrompt` cut `order:` and `pending:` at 240 units with no log, no latch and no marker.

**Header exemption DELETED.** `BuildZoneC`'s docstring called `MaxUtteranceChars` *"a separate, always-on
sanitiser"* and thereby exempted it from the observable-truncation condition. That exemption was the defect: the
condition is anchored to **the act of truncating**, "by any mechanism, under any name" — and "sanitiser" is a
name. The replacement text says so explicitly, so the exemption cannot be re-derived from the file.

**Channel 1 — the log**, in the same shape as the roster collapse: unlatched Verbose every turn, plus a Warning
on the first cut and on every *deeper* one. Two latches (`WarnedUtteranceBytes`, `WarnedPendingBytes`),
**separate per line on purpose** — they mean different things and are fixed in different places, so a deep cut on
one must not hide behind a deeper cut on the other. `SanitizeForPrompt` stays `static` (a pure text transform);
it reports magnitude through an out-param, and the new const member `ReportLineTruncation` owns the latching.

**Channel 2 — a marker in the prompt. This is the design call the spec asked me to make, and I made it: YES.**
Reasoning, recorded in full on `SanitizeForPrompt`'s declaration so it can be overturned deliberately:

- A collapsed roster still prints `other_kinds:`, so the model is *told*. A truncated `order:` line told it
  nothing, and the sampler is grammar-constrained — it produces a confident, well-formed command from half a
  sentence. **The log informs the developer; only the marker informs the model, and the model is the one acting.**
- **A VALUE, never a new key.** A `truncated:` key would obey the fixed-key law only by being emitted every
  turn, adding its bytes to every Zone C on every board including all-ASCII ones — which would move the
  hand-verified Zone C figure. The marker rides on the value of the existing `order:` / `pending:` key, exactly
  as `mid: ours` vs `mid: none` does. **The key set is untouched.**
- **The marker's bytes come OUT OF the cap, never on top of it** (` ...[truncated]`, 15 ASCII bytes, derived via
  `UE_ARRAY_COUNT` rather than transcribed, with a `static_assert` that it stays ASCII). A capped line is still
  at most 240 bytes, so Zone C's tail and the roster budget derived from it do not move by one byte.
- **Wording is plain editorial English because the model cannot be taught it** — Zone A is byte-identical for
  process life and is TASK-428's text. An ellipsis plus a bracketed word is what a cut quotation looks like
  throughout pretraining. **Safe untaught:** the GBNF constrains every emitted symbol to the live kind/place
  alternations, so the model physically cannot echo the marker into a command, and nothing parses Zone C. Worst
  case it is ignored — which is today's behaviour.

### BLOCKER-2 — the cap is now UTF-8 bytes, and the constant was RENAMED

`MaxUtteranceChars` → **`MaxUtteranceBytes`** (value unchanged, 240). **The rename is part of the fix**: a
constant named "Chars" that measures bytes would be a fresh instance of the same name-vs-meaning divergence the
audit caught. Verified before renaming: **CONVENTIONS.md names neither `MaxUtteranceChars` nor
`SanitizeForPrompt` anywhere (0 hits)** — §9 pins neither, so the rename is law-neutral.

⚠️ **Applying the recorded `use_mmap` lesson:** the dead name is spelled out **once**, in the new constant's
comment as "RENAMED FROM `MaxUtteranceChars`", so a reader arriving from an older doc and grepping the old name
lands on the correction instead of on nothing.

**Residual, stated honestly — bytes narrow the gap, they do not close it.** Bytes are correct-by-construction
for ASCII (1 byte = 1 char, so nothing measured moves) and strictly conservative otherwise. Worst case per line
falls from 720 bytes (240 units × 3 for CJK-class BMP text) to 240 — **exactly 3× tighter**, matching the ~3×
over-admission the audit measured. But at the byte-fallback tokenizer floor (~1 token/byte) a 240-byte line
still costs up to 240 tokens where the 2.71 ratio prices it at ~89: **the residual over-admission factor is
2.71× per line, down from 8.13×.** Both lines pathological = up to 480 tokens against a 400-token B+C budget, so
**two adversarial lines can still alone exceed the budget.** TASK-423 closes it with the real tokenizer; I did
not build a second one.

**Second half also fixed:** surrogate pairs move as a unit through pass 1 and the cut only ever lands on a
code-point boundary, so a lone surrogate can no longer be emitted into the prompt. Unpaired surrogates are
dropped (not text, no UTF-8 encoding, unreachable from ASCII input).

### PROOF that ASCII output is byte-identical — run, not asserted

Both algorithms were transcribed to a differential harness and run over **200,013 ASCII inputs** (0–600 chars,
whitespace runs, boundary lengths 239/240/241, plus the two fixture lines):

```
non-truncating (must match): 80572      MISMATCHES: 0
truncating (intended diff) : 119441     max output len old / new: 240 / 240  (cap 240)
order  (~61 chars): flattened_bytes=66  identical=True
pending(~72 chars): flattened_bytes=68  identical=True
```

**Max output length is 240 under both old and new**, so no length-derived figure can move. Consequences:

- **Zone B = 68 and Zone C = 887 are UNCHANGED.** `BuildZoneB` was not touched at all. Zone C's Head and roster
  block are untouched; the Tail changes only if a line exceeds 240 **bytes**, and the fixture's two lines are 66
  and 68 ASCII bytes — nowhere near it.
- **Bar #3's 77.1 % cannot move, by construction.** It was measured on the spike fixture's assembled prompt, and
  the spike has its **own private `SanitizeForPrompt` with its own local `MaxUtteranceChars = 240`**
  (`SiegeLlamaSpike.cpp:600-602`) — the measured artifact does not execute my code at all.
- WARN-2's reserve table is likewise unmoved: it is driven by maximum line length, which is identical.

### Also taken

- **`llama_kv_cache_seq_rm` → `llama_memory_seq_rm`** in my two comments (the header, and the `BuildZoneA` body
  comment). **Verified against the vendored header myself: `llama.h:735` is `llama_memory_seq_rm`, and the dead
  spelling appears nowhere in it.** Dead name retained once, deliberately, as a correction note.
- **WARN-E1, one word:** *"Always-on per-turn record"* → *"Unlatched per-turn record"*. It is a `Verbose`
  `UE_LOG`, so it dies under `NO_LOGGING` **and** is off at the default runtime verbosity — the category is
  `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeAssistant, Log, All)`. "Always-on" was false twice over; "unlatched" is
  the property actually being claimed (it contrasts with the latched Warning below it).
- **Traversal count corrected in `Capture`'s docstring, counted from the code rather than copied.** Six run
  unconditionally (castle ×2, near ground, best mine, capture zone, summoned units); +far ground whenever an
  enemy castle stands; +hero fallback when the hero is unpossessed ⇒ **7 normal, 8 worst.** I opened all three
  finders and confirmed **each is exactly one `TActorIterator`** (`Castle.cpp:669`, `AncientGround.cpp:167`,
  `GoldNode.cpp:199`); `GoldNode.cpp:264` is `FindBestMineInDisc`, which `Capture` never calls. The old text's
  *"Four of those passes are SHIPPED STATIC FINDERS"* was also wrong — it is **three functions, five calls**.

### For QA to scrutinise

1. **The marker is a behaviour change visible to the model.** It is the design call, argued above; overturn it
   knowingly if at all.
2. **`SanitizeForPrompt` now walks the whole input** instead of stopping at the cap, so a pathological
   multi-megabyte paste is O(n). Deliberate — the true flattened size is what makes the report and the
   escalation latch meaningful, and it is nothing beside one inference call.
3. **Pass 1 changed from range-for to an index loop** (needed to look ahead one unit for surrogate pairing). The
   earlier handoff section reasoned about the range-for's `end()` stepping back over the null terminator;
   `In.Len()` excludes it, so the index form is equivalent and no longer depends on that subtlety.
4. **Unreachable-by-arithmetic branch:** `CutIndex == INDEX_NONE` in the truncating path cannot occur (content
   budget is 225 bytes and one code point is at most 4), but it is guarded defensively anyway.

### ⚠️ Reported, NOT touched — other owners' files

- **`SiegeLlamaSpike.cpp:600-613` carries BOTH blockers verbatim** — its own `SanitizeForPrompt`, its own
  `static constexpr int32 MaxUtteranceChars = 240`, the same silent `break`, the same TCHAR counting. **Plugin
  lane, not mine.** No compile coupling (it never includes `SiegeAssistantSnapshot.h`), so my rename cannot
  break it — but it is the same defect in a second place, and it is the code that produced the measured figures.
- **`SiegeAssistantVocabulary.h:52`** still names the dead `llama_kv_cache_seq_rm` (game lane, different file).
- Dead symbol also in `CONVENTIONS.md:686` and `TASKBOARD.md:4288` / `:4634` / `:5055` — doc owners'.
- **`MaxUtteranceChars` appears 6× in `TASKBOARD.md`** and in `qa/TASK-416.md`, `qa/TASK-419.md` and this
  handoff's earlier sections. Board lines need the manager's rename; **QA reports and prior handoff sections are
  immutable records and should NOT be rewritten.**
- **NIT-2 left alone deliberately** (the header's Zone-C key list omits `other_kinds` and the `[ORDER]` marker).
  It sits two lines from text I edited, but it is not in this task's scope and I kept the change tight so
  TASK-428 inherits a minimal diff.
