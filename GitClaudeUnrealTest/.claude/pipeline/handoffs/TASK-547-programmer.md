# TASK-547 — the snapshot's region data + the Zone A teaching lines — programmer handoff

- **task:** TASK-547 [AC-7] — region columns, `ResolvePlaceRegion`, the Zone A mirror + the `in`-vs-`where` rule line
- **agent:** gameplay-programmer
- **status:** `ready-for-qa`
- **QA gate:** ⛔ **TASK-550** (`qa/TASK-550.md` — the batch's only gate; this task names it)
- **law:** CONVENTIONS `AS-§21.4` · `AS-§21.7` · `AS-§21.9` · `AS-§9c` · `AS-§20.4` · `AS-§3` · `AS-§12g`
- **compile / Git:** ⛔ **none.** TASK-551 owns the only compile and the only commit.
- **files touched (exactly two, both exclusively owned by this task):**
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeAssistantSnapshot.h`
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeAssistantSnapshot.cpp`

📌 **M8, verbatim as required:** *"adds no replicated property, no new replicated class, no new relevancy tier."*

---

## 1. ⭐ THE ONE PLACE I DEPARTED FROM THE WRITTEN SPEC — READ THIS FIRST

**Board spec (4)(b) says the `ZONE = ` line is *"GENERATED FROM `GetRegionPlaceNames()`"*. ⛔ I did NOT do that, and doing it would have been a QA FAIL on this file's own contract.**

`GetRegionPlaceNames()` is **per-match member state** — it is filled by `Capture()` and is legitimately *shorter* on a map with one ancient ground or no capture zone. Zone A is the **frozen KV prefix**: it *"READS NO MEMBER STATE, BY CONSTRUCTION"* and *"making it state-dependent is a QA FAIL"* (`SiegeAssistantSnapshot.h`, `BuildZoneA`'s own declaration comment), because a Zone A that varies destroys the measured **77.1 % KV reuse** silently — as a latency regression, not a wrong answer.

✅ **What I did instead, and why it satisfies the ruling rather than dodging it:** the line is generated **from the `PlaceVocabulary` table's new `bHasRegion` column**, at build time, from a `static constexpr` table. That is:
- **generated, never hard-coded** — which is what `AS-§21.4` actually requires (*"THE LIST IS GENERATED, NEVER HARD-CODED. `PlaceVocabulary` stays the SINGLE OWNER and gains one `bHasRegion` column"*). A place that later gains a region primitive appears in Zone A by flipping **one bool**; one that loses it disappears the same way. No literal list of three exists anywhere in this file.
- **state-free** — the table is a compile-time constant, so Zone A stays byte-identical for the life of the process.
- **exactly the split the `places` block already ships.** `GetPlaceNames()`'s own comment states the rule for `where` in these words: *"Zone A still prints the FULL vocabulary (it must stay static); the grammar is what enforces existence."* The `ZONE =` line is that same rule applied to the region subset — Zone A prints all three symbols, and **TASK-546's grammar** generates its `zone` alternation from the **live** `GetRegionPlaceNames()`, so a region the map lacks is **unsamplable** even though the prompt names it.

⚖️ **Net effect on the batch:** none of the pinned symbols moved, `GetRegionPlaceNames()` still exists and is still what the grammar consumes (TASK-546 already reads it — verified in its shipped code). **Only the Zone A generator's INPUT changed, from live state to the table the live state is itself derived from.** ⛔ If QA disagrees, the fix is to change **this file**, not to make Zone A state-dependent.

---

## 2. THE PINNED SIGNATURES, AS WRITTEN (`AS-§21.9`, character-for-character)

```cpp
// ── SiegeAssistantSnapshot.h ──────────────────────────────────────────────
const TArray<FName>& GetRegionPlaceNames() const;
bool ResolvePlaceRegion(FName Place, FVector& OutCentre, FVector2D& OutHalfExtent) const;
```

As shipped:

```cpp
const TArray<FName>& GetRegionPlaceNames() const { return RegionPlaceNames; }

bool ResolvePlaceRegion(FName Place, FVector& OutCentre, FVector2D& OutHalfExtent) const;
```

⚠️ **`GetRegionPlaceNames()` is defined INLINE in the header** — the declaration is character-for-character the pin, and inline is the house idiom for its three siblings (`GetRoster` / `GetPlaceNames` / `GetUnitKinds`). ⛔ Nothing else in the pin belongs to this task; I added no other public symbol.

**New private members (declared in the `names:` block, not in the pin — additive, nothing renamed):**

```cpp
UPROPERTY(Transient) TArray<FVector2D> PlaceHalfExtents;   // parallel to PlaceNames / PlaceLocations
UPROPERTY(Transient) TArray<FName>     RegionPlaceNames;   // the region-bearing SUBSET of PlaceNames
```

**`FPlaceDefinition` gains one column:**

```cpp
struct FPlaceDefinition
{
    const TCHAR* Symbol;
    const TCHAR* Description;
    bool bHasRegion;   // TRUE iff a shipped IsPointInZone answers for this place
};
```

**Exactly three rows are `true`: `mid` · `ancient_ground_near` · `ancient_ground_far`.** The other four are `false`, and the reason is on the column's own doc comment (`Castle.h` has **zero** `Radius` properties, `AGoldNode`'s radius is a *caller's* parameter, the hero's `RallyRadius` belongs to the Rally ability ⇒ each would need an **invented number**, and two of the four **move**).

---

## 3. ⭐ ZONE A — THE THREE LINES VERBATIM, WITH MEASURED LENGTHS

⛔ **Every figure below was MEASURED, not estimated and not transcribed from a draft.** The instrument extracts the literals **programmatically out of the shipped `SiegeAssistantSnapshot.cpp`**, unescapes them, rebuilds the generated `ZONE` line **from the table's own rows**, and counts characters. No compiler and no model were used; nothing was re-typed.

### Component 1 — the `WHO =` schema line, **Δ = +16**

BEFORE (120 chars incl. newline):

```
WHO    = [{"kind":KIND,"n":COUNT}] with 1 to 3 entries, or {"all_except":[KIND]} with 1 to 3 kinds, or "all", or "none"
```

AFTER (136 chars incl. newline):

```
WHO    = [{"kind":KIND,"n":COUNT}] with 1 to 3 entries, or {"all_except":[KIND]} with 1 to 3 kinds, or {"in":ZONE}, or "all", or "none"
```

Inserted text is `, or {"in":ZONE}` — **16 chars**, placed **THIRD, after the `all_except` shape and before the two bare strings**.

⭐ **THE ORDER WAS TAKEN OFF TASK-546's SHIPPED CODE, NOT OFF ITS HANDOFF (which does not exist yet).** `SiegeAssistantGrammar.cpp`, the `--- who ---` block, builds:

```
who ::= selection | except | inplace | "all" | "none"
```

and its own comment says the Zone A line mirrors it and a test asserts they agree. **My line lists the five shapes in exactly that order.**

### Component 2 — the `ZONE = ` metavariable line, **76 chars** (incl. newline)

```
ZONE   = an area place symbol: mid, ancient_ground_near, ancient_ground_far
```

- Emitted as a fixed prefix (`ZONE   = an area place symbol: `) plus the symbols the `bHasRegion` column selects, joined `", "`, in **fixed vocabulary order**.
- Padded to the block's existing 7-wide metavariable column (`INTENT`/`WHO`/`KIND`/`COUNT`/`WHERE`/`WHEN`).
- **Position: after `COUNT`, before `WHERE`.** `ZONE` is a sub-metavariable of `WHO` (so it sits with `KIND` and `COUNT`), and putting it directly above `WHERE` makes the two place-valued lines adjacent — which is where the rule line then draws the distinction.
- 📌 The placeholder is `ZONE` to mirror the **GBNF rule `zone`**. It has nothing to do with prompt Zones A/B/C; the model never sees that vocabulary. This is written into the source so nobody "fixes" it.

### Component 3 — the `in`-vs-`where` teaching rule, **147 chars** (incl. newline)

```
- Units already in a place: who is {"in":ZONE}, where is still where they go, and one order may set both. Only with send, guard, ambush or follow.
```

- **House style, `- <antecedent>: <instruction>.`**, a rule with a **testable antecedent**, ⛔ not an exemplar sentence (`AS-§20.4` leg 2 — a decision-ordering rule beat an exemplar pass at this exact seam, and 🔒 a new few-shot would need an absence certificate against a **sealed** corpus).
- **It carries the trigger word.** *"Units already **in** a place"* matches Jonathan's *"units currently **in** an ancient ground"*, and the word it matches on **is the JSON key it must emit**.
- **It says BOTH-IN-ONE-ORDER explicitly** (*"and one order may set both"*), which is the clause the manager's finding is about. Jonathan's sentence populates both keys: `who:{"in":"ancient_ground_near"}` **and** `where:"enemy_castle"`.
- **It names no place symbol** — `ZONE` is the metavariable (the `AS-§9c` seam rule; a concrete symbol in a byte-frozen Zone A is one the sampler may forbid on a map that lacks it).
- **Position:** immediately **after** the TASK-521 minimal pair, extending the `who`-shape ladder (kinds named → selection · everyone → `"all"` · everyone minus kinds → the exclusion · **the units standing somewhere → the region**). ⛔ The byte-frozen economy line keeps **both** of its declared neighbours; the minimal pair stays adjacent and in order.

### ⚠️ ONE CLAUSE DELIBERATELY OMITTED, DECLARED RATHER THAN OVERLOOKED

The exclusion rule directly above carries **both** halves — *"only with send, guard, ambush or follow"* **and** *"On charge, fallback or rally: `{"ask":"unsupported"}`"*. **My line carries only the positive half.** The negative half measures **52 more chars**, which would take the batch to **291 against a 250 ceiling** ⇒ `AS-§21.7` says STOP.

**The residual is bounded and known:** a region handed to `charge`/`fallback`/`rally` is **REFUSED BY THE PARSER** (`region_conflict`, `AS-§21.6`, TASK-545), never silently dropped — and `AS-§21.11` item 1 already records that refusal as **designed behaviour on Jonathan's playtest sheet**. ⇒ The cost of the missing half is *a refusal the player sees*, never *an army that moves unasked*.

⚠️ **The other three conflicts need no prose at all, and their absence is deliberate:** `who` is ONE alternation, so the grammar makes a region-plus-selection, region-plus-`all_except` and region-plus-`"none"` **structurally unsayable**. Only the INTENT conflict survives into prose, and that is what the line spends its characters on.

---

## 4. THE NEW ZONE A TOTAL, AND WHICH BASELINE I USED

⚠️ **BASELINE USED: 5419 — the RE-BASED figure, post-TASK-541. ⛔ NOT 5424.** I read TASK-541's handoff (`handoffs/TASK-541-programmer.md` §3), which states it measured **5419** from source after its −5 to the vocabulary `[notes]` line (that line reaches Zone A through `BuildSynonymTable()`, so it is Zone A's byte even though it lives in another file).

| # | component | Δ chars |
|---|---|---|
| — | **RE-BASED BASELINE (post-TASK-541)** | **5419** |
| 1 | `WHO =` schema line — `, or {"in":ZONE}` | **+16** |
| 2 | the whole `ZONE   = ` line, incl. newline | **+76** |
| 3 | the `in`-vs-`where` rule line, incl. newline | **+147** |
| — | **TASK-547 TOTAL** | **+239** |
| — | ⭐ **NEW ZONE A** | **5658 chars / 5658 UTF-8 BYTES** |

- ✅ **ASCII-clean** — verified programmatically on all three components, so chars and bytes are the same number (this file's ASCII law for prompt literals).
- ✅ **+239 is inside the batch's +250 ceiling** (`AS-§21.7`), with **11 chars left** for any future Zone A edit. ⛔ Nothing was trimmed to fit and no shipped few-shot was touched.
- ✅ **`Out.Reserve(6144)` still covers it with 486 spare** ⇒ not re-tuned. It exists to stop the builder reallocating, not to bound the prompt.
- ⛔ **TOKENS ARE NOT DERIVED.** `zoneA_tok = 1139` and the 77.1 % KV-reuse figure stay **STALE — PENDING RE-MEASUREMENT ON THE MODEL** (`AS-§12g`, `AS-§21.7`). Only `Siege.Llama.SpikePrompt` prints the real figure, and Stage 5 is where it is printed. **No token figure appears anywhere in this handoff or in the code I wrote.**

**The arithmetic is also recorded in the source**, component by component, in `BuildZoneA`'s header comment — so the total is checkable at the artifact and a transcription error fails with the component named.

---

## 5. ZONE C IS BYTE-UNCHANGED — AND IT IS STRUCTURALLY VERIFIABLE, NOT ASSERTED

✅ **CONFIRMED: Zone C is byte-unchanged. Zone B is byte-unchanged too.**

**How to check it in one look:** every hunk in my `.cpp` diff falls inside exactly five regions —

1. `namespace SiegeAssistantSnapshotInternal` (the table + the count guard)
2. `ResetSnapshot()`
3. `Capture()`
4. `ResolvePlace()` (the new `ResolvePlaceRegion` is appended immediately after it)
5. `BuildZoneA()`

⇒ **`BuildZoneB`, `BuildZoneC`, `AppendRosterBlock`, `SanitizeForPrompt`, `ReportLineTruncation` and `ValidateCommandAgainstSnapshot` have ZERO changed lines.** Run `git diff -U0 | grep '^@@'` and read the hunk headers.

⭐ **And the reason it costs Zone C nothing is the design, not luck:** no per-unit position, no per-region datum and no occupancy count is ever printed. The model names a region **SYMBOL** — the identical operation it already performs for `where` — and the geometry never leaves this object except through `ResolvePlaceRegion` into the executor (`AS-§3`, the coordinate airlock).

---

## 6. PART 1 — THE REGION DATA, AND THE "NO NEW TRAVERSAL" PROOF

⛔ **ZERO new traversals. ⛔ No registry, no actor cache, no dirty flag, no subscription list (`§4` rejects all four on sight).**

| region | actor | already held by | accessor |
|---|---|---|---|
| `ancient_ground_near` | `NearGround` | **pass 3** (`FindNearestAncientGround`) | `AAncientGround::GetZoneHalfExtent()` |
| `ancient_ground_far` | `FarGround` | **pass 3** (same finder, enemy reference) | same |
| `mid` | `Zone` | **pass 5** (the existing `ACaptureZone` loop) | `ACaptureZone::GetZoneHalfExtent()` |

Each is **one extra read off a pointer `Capture()` already dereferences**, on the same line block that already reads `GetActorLocation()`. No `TActorIterator` was added; the `Capture()` doc comment now says so explicitly so a reviewer does not have to re-count the seven-to-eight traversals.

⚠️ **The centre is the ACTOR LOCATION and that is not a coincidence to be tidied.** Both shipped predicates test a 2D XY box **about the actor origin** (`AncientGround.cpp:143-151`, `CaptureZone.cpp:112-118` — byte-copies of each other). So the pair published here is exactly the pair the membership test uses. Reading a component bounds or the decal size instead would answer a **different question** than the shipped predicate.

**Publishing rule (in the existing fixed-vocabulary loop, no second loop):** a symbol reaches `RegionPlaceNames` only if it is **already in `PlaceNames`** (so the two can never disagree about what exists), **`bHasRegion` is true**, **and** the captured extent is `> 0` on both axes.

⚠️ **That last clause is a judgement I made and it is not in the spec — flagging it for QA.** `ZoneHalfExtent` is an instance-editable tunable (default `(840,840)`). At `(0,0)` the shipped `IsPointInZone` answers TRUE only for a point exactly on the actor origin, so the region would be **offerable, sampleable, and then contain nobody** — a refusal the player cannot act on. Dropping it makes the shape **unsayable** instead, which is the same fail-closed direction `AS-§21.5` takes everywhere else. ⛔ If QA rules this out of scope, deleting the two float compares is a two-line revert with no other consequence.

### ⚖️ THE DECLARED RESIDUAL — SNAPSHOT-TIME GEOMETRY, EXECUTION-TIME MEMBERSHIP

**I capture the BOXES; TASK-548 tests live unit positions against them. The units move, the grounds do not** — so a unit that walked out of the ground between the sentence and the order landing must not be selected, and it is not.

⚠️ **A ground DESTROYED between capture and execution leaves a stale centre.** ⭐ **That is the IDENTICAL staleness `ResolvePlace` already carries for the DESTINATION** — the snapshot holds no actor pointers by design, so the same window exists for every place symbol shipped today. **Same risk profile, not a new one**, and bounded the same way: `Capture()` runs **once per typed sentence**, so the window is one inference call. It is recorded on `ResolvePlaceRegion`'s declaration and beside the capture array.

### `ResolvePlaceRegion` — the contract

- **The REGION list is the authority, asked FIRST.** `own_castle` resolves perfectly well as a *destination* and has no box at all; answering it with the castle's location and a zero extent would be a silent wrong answer wearing a `true`.
- **Both out-params are left UNTOUCHED on failure**, mirroring `ResolvePlace` (⚠️ deliberately the *opposite* of `ValidateCommandAgainstSnapshot`, which always writes — the two differ for the reason `ResolvePlace`'s comment gives). ⛔ **There is no "whole map" fallback**: a region named and not resolved must become a **REFUSAL**, never an unfiltered order (`AS-§21.5`'s hard rule).
- `IsValidIndex` guards on both parallel arrays, the same defensive shape `GetOrderableCount` uses.

---

## 7. TWO SMALL THINGS I ADDED THAT THE SPEC DID NOT ASK FOR — BOTH DECLARED

1. **`PlaceVocabulary` is now `static constexpr` instead of `static const`, and there is a `static_assert` that at least one row is region-bearing.** The keyword is what lets a `constexpr` counter read the column **at compile time**; without it the "no region-bearing place exists" state would emit `ZONE   = an area place symbol: ` with **nothing after the colon** — a metavariable the model can never fill. Every initialiser was already a constant expression (string literals + bools), so **not one emitted byte moves** and the table's storage is unchanged. The count is **counted from the table, never transcribed** (the `UtteranceTruncationMarkerBytes` idiom).
2. **The end-of-`Capture` Verbose log gained one `%d`:** `%d place(s) (%d region-bearing)`. An **empty region list turns the whole feature off silently** — the grammar then emits no `inplace` alternative, the shape is unsampleable, and nothing anywhere says why. This is the cheapest artifact that answers *"did the snapshot publish any regions?"*. ⛔ It stays at **Verbose** with its neighbours (it fires on every typed sentence); TASK-542 owns the one line that had to be promoted to `Log`.

⛔ **Nothing else moved.** No new intent, no new place symbol, no vocabulary row, no few-shot, no CSV, no test, no `Docs/`, no `Content/`, no `Build.cs`.

---

## 8. TESTS I KNOWINGLY BREAK — ⛔ NOT MINE TO FIX

1. **`Siegebound.Assistant.ZoneA.MeasuredCharCount`** — asserts `ShippedZoneAChars == 5424`. **The value to re-base to is `5658`** (TASK-541's −5 plus my +239, both measured).
2. **`Siegebound.Assistant.ZoneA.TwoLaneByteEquality`** — the shipped lane moves again; ⛔ **the SPIKE lane stays at its measured 5116**, because **D4 still holds** (`Plugins/SiegeLlama/**` was not touched by this batch either).

⛔ **TASK-549 owns both re-bases, and it must NEVER re-copy this builder's output into the fixture** — a test transcribed from its subject is a guardrail that reports SAFE. ⛔ **I did not touch `Tests/`.**

**The per-component figures TASK-549 should assert** (the `ZoneATest.cpp:617-641` idiom, so a transcription error fails with the component named): **WHO delta = 16 · ZONE line = 76 · rule line = 147 · sum = 239.**

---

## 9. WHAT QA SHOULD SCRUTINISE (TASK-550)

1. ⭐⭐ **THE `ZONE = ` GENERATOR'S INPUT — §1 above.** I generate from the **table**, not from `GetRegionPlaceNames()` as the board spec's wording says. **Decide this explicitly rather than by silence.** The test that matters: *does `BuildZoneA` read one byte of member state?* It must not.
2. ⭐ **THE `WHO =` ORDER AGAINST THE GRAMMAR.** `selection | except | inplace | "all" | "none"`. I read it off `SiegeAssistantGrammar.cpp`'s shipped `--- who ---` block, not off a handoff. **There is no compile-time link — the comment and this gate are the whole tie.**
3. **THE THREE CHARACTER COUNTS.** Re-measure them; do not take 16/76/147 from this file. The literals are in the source and are ASCII.
4. **ZONE C / ZONE B BYTE-EQUALITY** — check the hunk headers (§5). If any hunk lands in `BuildZoneB` / `BuildZoneC` / `AppendRosterBlock`, that is a finding.
5. **THE DEGENERATE-EXTENT FILTER** (§6) — an unspecced judgement call, fail-closed, two-line revert if rejected.
6. **`static const` → `static constexpr` ON `PlaceVocabulary`** (§7.1) — confirm it is byte-neutral to the prompt and that the `static_assert` is the reason it exists.
7. **`ResetSnapshot` resets BOTH new arrays.** A missed reset here is exactly the *"stale row from the previous sentence"* class the function exists to prevent.
8. **THE PARALLEL-ARRAY INVARIANT.** `PlaceNames` / `PlaceLocations` / `PlaceHalfExtents` are appended at **one** site in **one** loop. If a future edit adds a second append site, they desynchronise.
9. ⛔ **DO NOT record the two red Zone-A tests as a defect of this task** (§8). They are TASK-549's re-base, and **5658** is the number to re-base to.
10. ⛔⛔ **NO ACCURACY FIGURE IS ATTACHED TO ANYTHING HERE** — no predicted score, no row list, no percentage. Nobody has ever seen what the model emits for either of Jonathan's sentences; that is what TASK-542's output log exists to fix. **If a later artifact adds one, deletion is the fix.**

---

## 10. DOWNSTREAM CONTRACT (TASK-548, the executor)

- `Snapshot->GetRegionPlaceNames()` → the grammar's `RegionPlaceNames` parameter (TASK-546's third defaulted arg). Already matches what TASK-546 shipped.
- `Snapshot->ResolvePlaceRegion(Command.RegionPlace, Centre, HalfExtent)` → **false ⇒ REFUSE.** ⛔ Never fall through to an unfiltered order.
- Then `FSiegeAssistantRegionStatics::IsPointInRegion(Unit->GetActorLocation(), Centre, HalfExtent)` (TASK-544) **inside the loop that already exists**, boundary **inclusive**, Z **ignored** — mirroring `AAncientGround::IsPointInZone`, which is the predicate the captured pair was read from.
