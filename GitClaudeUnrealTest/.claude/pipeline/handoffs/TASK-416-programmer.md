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
