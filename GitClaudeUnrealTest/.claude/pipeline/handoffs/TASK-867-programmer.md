# TASK-867 — THE STALE-`names:` SWEEP — programmer handoff

**Status:** `ready-for-qa` · **Terminal / read-only** · **Law:** `SC-§38`, `SC-§39`, `SC-§40` cl. 9, `WITCH-§1` (instruction-not-description)
**Swept:** every **open** row in Lane W (`826`–`835`, `847`–`851`, `860`–`863`) and Lane F (`836`–`843`, `858`, `859`). `847` and `836` are `done` and were skipped per the row.
**⛔ Fence honoured:** ZERO writes to `Source/`, ZERO writes to `TASKBOARD.md`, ZERO writes to `CONVENTIONS.md`. No compile, no editor, no MCP, no Git writes. `git log`/`git status` read only.

---

## 0. THE HEADLINE

**Class 1 (a `names:` symbol that no longer exists): ⛔ ZERO found.** Every C++ symbol pinned in every open Lane W/F row resolves in the tree **at the name given**. Method in §5; this is a measured clean sweep, not an assumption.

**Class 2 (the symbol exists but a test now COUNT-PINS it, so a new caller breaks the suite *by existing*): ⛔ THREE found, two of them blocking.** All three sit on rows that have **not been dispatched yet**, which is the only reason they are still cheap.

| # | row | pinned thing | pinned at | class | severity |
|---|---|---|---|---|---|
| **F-1** | ⛔⛔ **`TASK-839`** | `return false;` inside `FSiegeCombatStatics::ReadFogState` | **2** | 2 | ⛔⛔ **BLOCKER** |
| **F-2** | ⛔⛔ **`TASK-860`** | `SetVisibility(` in **all** of `CombatantHealthBarComponent.cpp` | **1** | 2 | ⛔⛔ **BLOCKER** |
| **F-3** | ⚠️ `TASK-839` | `GatherHostileAgents(`/`GatherFriendlyAgents(` in `SpellLibrary.cpp` | **2** / **1** | 2 | ⚠️ conditional |
| **F-4** | ⚠️ `TASK-849` | dated `file:line` hints for the Sapper trio | drifted **~650 lines** | 1-adjacent | ⚠️ WARN |

---

## 1. ⛔⛔⛔ FINDING F-1 — `TASK-839` — **THE ROW IT MUST BREAK IS NOT IN ITS FENCE, AND THE FAILURE IS SILENT**

**This is the worst one, and it is worse than `TASK-851`'s, because `851`'s trap failed LOUDLY (a red suite) while this one has a second failure mode that ships a green suite and a dead card.**

### The pin
`Tests/SiegeFogClampTest.cpp`, test **`Siegebound.Fog.TheFogStateIsReadInExactlyOnePlaceAndIsNotLiveUntilTask839`**, clause (b):

```cpp
CountOccurrencesInCode(SeamBody, TEXT("return false;")), 2);
```
…where `SeamBody` is the extracted body of `bool FSiegeCombatStatics::ReadFogState(`.

**Measured live: exactly 2.** The seam's own source comment says so in as many words:

> `// ⛔⛔ THIS LINE IS THE SEAM, AND IT IS THE ONLY LINE TASK-839 REPLACES.`
> `// ⭐ TASK-839: iterate AFogVolume, take its active flag and its FSiegeFogTuning, return here.`

The test's comment anticipates it too: *"⭐ WHEN IT DOES, **INVERT THIS ROW, DO NOT DELETE IT**."*

### Why an obedient agent breaks
`TASK-839`'s whole job (item 1: *"the authoritative 'fog is active until T' state"*) **is** to replace that final `return false;`. Doing so takes the count 2 → 1 and turns that test **RED**.

⛔ **And the board row never mentions any of it.** `TASK-839`'s `parallel-safe:` line says *"writes `SpellLibrary.cpp` + `CardRow.h`"* and its `names:` line lists `AFogVolume` · `FogDurationSeconds` · `ESpellEffect` · `SpellLibrary.cpp` · `CardRow.h`. **`SiegeCombatStatics.cpp`, `ReadFogState` and `Tests/SiegeFogClampTest.cpp` appear in neither.**

⇒ **Both branches are bad, and the second is the dangerous one:**
- **(a)** The agent edits the seam (correct behaviour) → **suite red**, in a test naming neither the task nor `AFogVolume`, on a file it was never told it owned.
- **(b) ⭐⭐ The agent respects its declared fence, does *not* touch `SiegeCombatStatics.cpp`, and ships `AFogVolume` + the timer + the card.** **The suite stays GREEN. The fog card renders, costs 50 gold, and clamps nobody** — `ReadFogState` still answers *"no fog"* on every call, so `EffectiveVisionRadius` is never given an active tuning. ⚖️ ***A green suite and a working-looking card, with the headline mechanic inert.***

### Two more tree-wide pins on the same row
- `ReadFogState(` is pinned at **3** tree-wide (`CountAcrossShippingSource`) — declaration + definition + the one funnel call. **Measured: 3.** A convenience second read inside `AFogVolume` makes it **4** ⇒ red.
- `FSiegeFogStatics::EffectiveVisionRadius(` is pinned at **2** tree-wide. **Measured: 2** (`SiegeFogStatics.cpp` definition ×1, `SiegeCombatStatics.cpp` funnel call ×1). ⭐ **This is the exact structural twin of the `FSiegeInvisibilityStatics::IsVisibleTo(` == 2 pin that nearly took `TASK-851` down** — same idiom, same failure, different lane. An `AFogVolume` that helpfully exposes *"how far can you see right now"* makes it **3** ⇒ red, and that test's message explicitly says a third hit is *"an automatic QA FAIL"*.

### ✅ Recommended correction (manager's to board)
Add to `TASK-839`: **`SiegeCombatStatics.cpp` (`ReadFogState`'s final `return false;` ⛔ ONLY)** and **`Tests/SiegeFogClampTest.cpp` (⛔ INVERT the `return false; == 2` row, ⛔ do not delete it)** to the fence and the `names:` line, and state the two tree-wide pins (`ReadFogState(` == 3, `EffectiveVisionRadius(` == 2) as **⛔ do-not-perturb**.

---

## 2. ⛔⛔ FINDING F-2 — `TASK-860` — **THE COLLAPSE ITEM (5) ASKS FOR IS THE ONE CALL THAT IS PINNED**

### The pin
`Tests/SiegeHealthBarOcclusionTest.cpp`, test 11 **`Siegebound.HealthBarOcclusion.TheOwnerIntentLatchIsWrittenOnlyByShowBarIfEnabledAndHideBar`**, clause (c):

```cpp
TestEqual(TEXT("(c) ⭐ 'SetVisibility(' appears exactly once in the whole component — "
               "ApplyBarVisibility() is THE single writer, and it exists"),
    CountOccurrencesInCode(ComponentSource, TEXT("SetVisibility(")), 1);
```
`ComponentSource` is the **whole** of `CombatantHealthBarComponent.cpp`. **Measured live: exactly 1** (raw 3 — two hits live in comments and are correctly skipped, which is *also* why a naive grep reading "3" would mislead here).

### Why an obedient agent breaks
`TASK-860`'s fence is **`CombatantHealthBar*.{h,cpp}` SOLE** — precisely this file. Its item (5) reads:

> **`CastBarRoot` is `Collapsed` while `bCasting == false`**, never merely transparent (a hidden-but-laid-out element still moves the HP bar).

The natural C++ implementation of "collapse it" inside the only file you are allowed to touch is a **second `SetVisibility(`** ⇒ count 2 ⇒ **test 11(c) RED**. The failing test is named for the *owner-intent latch* and mentions neither `TASK-860`, nor the cast bar, nor `CastBarRoot`. **The symbol resolves, the code compiles, the failure is filed under an unrelated name** — the `SC-§40` cl. 4 discoverability trap on top of the count trap.

⚠️ **And the obvious "fix" is the wrong one.** A reader who hits this red will be tempted to loosen `== 1` to `<= 2`, which deletes the single-writer guarantee that makes *"the cull may only subtract"* true — the property tests 2 and 10 in that file rest on.

✅ **The correct resolution exists and costs nothing:** collapse is **`TASK-861`'s** (the UMG half binds `OnCastProgressChanged` and sets `CastBarRoot`'s visibility in the widget). `TASK-860` should fire the event and **write no visibility call at all** — which is also what item (2)/(3) actually describe. The row just never says *"and that is why you must not call `SetVisibility` here."*

### Sibling pins in the same file, measured (all currently satisfied, all ⛔ do-not-perturb)
`bBarShownByOwner =` **2** · `LineTraceSingleByChannel` **1** · `LineTraceTestByChannel` **0** · `bFindInitialOverlaps` **0** · `UpdateHealthBarOcclusion` **2**.
✅ **`DrawSize` is pinned by NO test** (measured: zero `DrawSize` references anywhere in `Tests/`) ⇒ **item (4)'s `DrawSize` bump is safe.**

### ✅ Recommended correction
Add to `TASK-860` item (5)/(7): **⛔ `SetVisibility(` is pinned at EXACTLY 1 in `CombatantHealthBarComponent.cpp` (`SiegeHealthBarOcclusionTest` test 11(c)). The collapse is `TASK-861`'s widget-side job. ⛔ Do not add a second call, and ⛔ do not loosen the assertion.**

---

## 3. ⚠️ FINDING F-3 — `TASK-839` × `SpellLibrary.cpp` — conditional, but it is the file the row *does* own

`SpellLibrary.cpp` is count-pinned by **two** tests at once:

| test | needle | pinned | measured |
|---|---|---|---|
| `SiegeAcquisitionFunnelTest` site 6 | `FSiegeCombatStatics::GatherHostileAgents(` | **2** | ✅ 2 |
| `SiegeAcquisitionFunnelTest` site 6 | `FSiegeCombatStatics::GatherFriendlyAgents(` | **1** | ✅ 1 |
| `SiegeFogClampTest` lane 4 | vision queries | **0** | ✅ 0 |
| `SiegeAcquisitionFunnelTest` | `GatherTeamAgents(` | **0** | ✅ 0 |
| `SiegeAcquisitionFunnelTest` | `Cast<ASummonedUnit>(Candidate)` | **0** | ✅ 0 |

`TASK-839` adds a fog resolver here. **Assessed probability: LOW** — fog is a world-global scalar and its resolver has no reason to enumerate agents. But `WITCH-§1`'s standing law (*"one gatherer, never a second enumeration"*) actively **pushes** an author toward `GatherHostileAgents` if they ever need a unit list, and site 6's failure message reads *"an INCREASE means a new acquisition appeared"* — which would send the reader hunting an acquisition bug that does not exist. Worth one sentence in the row.

---

## 4. ⚠️ FINDING F-4 — `TASK-849`'s dated hints have drifted **~650 lines**, not the ~3 the row records

`TASK-849`'s `names:` line carries *"📌 dated hints: `SummonedUnit.cpp:4212`/`:2419`/`:4230`"*. **Measured today, by symbol:**

| symbol | row's hint | **actual** | drift |
|---|---|---|---|
| `ASummonedUnit::Detonate` | `:2419` | **`:4852`** | +2,433 |
| `ASummonedUnit::ApplyDetonation` | `:4212` | **`:4865`** | +653 |
| `ASummonedUnit::HandleDeath` | `:4230` | **`:4916`** | +686 |

`TASK-830` has been writing several hundred lines of witch code into that file. ⛔ **The row is defended** — it says "locate by SYMBOL, the numbers are dated hints" — so this is a WARN, not a blocker. **But the *magnitude* is the finding:** the board records this drift as `4209 → 4212`, i.e. **three lines**, which reads as *"close enough to eyeball."* At 650 lines a reader who spot-checks near the hint lands in unrelated code and may conclude the trap is gone. Same for `SiegeInvisibilityStatics.h`'s in-source citation block (`:102`–`:117`), which still names `SummonedUnit.cpp:4209` for the Sapper blast.

**Also NIT-grade, both symbol-correct:**
- `TASK-849` `names:` writes **`AMinerUnit::UpdateMining` → `TryRegisterArrivedMiner`**. `AMinerUnit::UpdateMining` exists (`MinerUnit.cpp:291`), but `TryRegisterArrivedMiner` is **`AGoldNode::`** (`GoldNode.cpp:124`), called from `MinerUnit.cpp:515`. The arrow is a correct call-graph; a reviewer grepping `AMinerUnit::TryRegisterArrivedMiner` gets nothing.
- `TASK-830` cites `UnitCommand.h:124-128` for `PositionCenter`/`PositionRadius`. Actual: **`:126`** and **`:130`** — `PositionRadius` sits *outside* the cited range.

---

## 5. ⛔ METHOD — HOW I MEASURED, AND HOW I PROVED THE INSTRUMENT (`SC-§39`, `SC-§40`)

The house helper `CountOccurrencesInCode` has **two** failure directions, so a single control proves nothing. I wrote a faithful replica (scratchpad `count.py`) reproducing its exact skip rule — skip a line whose `TrimStart()`'d form starts with `//`, `* `, `*/`, `/*`, or equals `*` — and reported both the skip-aware count and the raw count, so every number below is **what the suite itself would see**, not what a grep sees.

**⭐ Control 1 — the OVER-COUNT direction (a trailing `//` on a code line manufactures a false hit).**
On `SiegeBotController.cpp` my replica reads bare `IsAgentVisibleTo` = **4** and call-shaped `FSiegeCombatStatics::IsAgentVisibleTo(` = **3** — **reproducing `TASK-851`'s measured 4-vs-3 exactly.** It also independently reproduced three *shipped* assertions in that file: `TActorIterator<ASummonedUnit>` = **4**, `ApplyRadialDamage` = **0** on code lines (raw 1 — the hit is in a comment), `FSiegeCombatStatics::IsAgentVisibleTo(` = **3**.

**⭐ Control 2 — the UNDER-COUNT direction (a leading `/*` hides a real hit).**
On `SiegeInvisibilityStatics.h`, needle `/*`: raw = **16**, skip-aware = **0**. The skip rule provably eats 16 real occurrences, so the blind spot is reproduced rather than assumed.

⇒ **Both edges verified against known-good values before any finding below was trusted.** Every needle I report is **call-shaped** (`Symbol(`) except where I state otherwise.

**Absence claims — each with a named instrument and a positive control (`SC-§40` cl. 1):**

| claimed absent | instrument | result | ⭐ positive control |
|---|---|---|---|
| `GetCastProgressPercent` / `IsCastInProgress` | `grep -rn` over `Siegebound/**` | **0 tree-wide** | sibling `GetDamageBoostPercent` on the *same interface* returns **10** ⇒ the probe can see interface methods |
| `AFogVolume` (as a declaration) | `grep -rn` | **0 declarations**, 11 prose mentions | the 11 prose mentions prove the scanner read those files |
| `MI_Unit_Invisible` in code | `grep -rn` | **0 code refs**, 3 comment refs | `MI_TeamColor_Blue` **is** hard-referenced in `Building.cpp` ⇒ the probe can see MI names in code |

---

## 6. ✅ COUNTS RE-MEASURED BY SYMBOL AND REPORTED **EVEN THOUGH THEY MATCH** (`SC-§40` cl. 9)

*A silent match is indistinguishable from a skipped check.*

| count in law / row | asserted | **measured** | verdict |
|---|---|---|---|
| `ESiegeVeilBreakReason` "CLOSED AT SIX" (`TASK-830` item 9) | 6 | **6** — `{Attack, Heal, Mine, Empower, Cast, Death}` | ✅ and `Cast` is present as the row claims |
| `ESummonedUnitState` `{Idle, Advance, Attack}` (`830`, `849`) | 3 | **3**, exactly those names | ✅ |
| `ESpellEffect` family (`TASK-839` item 4) | 6 | **6** — `{None, AoEDamage, Freeze, TopTargetsDamage, AllyBuff, GoldSteal}` in `CardRow.h` | ✅ shape matches character-for-character |
| `FOG-§7` row 1 "the FIVE gatherer calls" | 5 | **5** — `FSiegeVisionQuery::Seeing` = Hero 1 + SummonedUnit 2 + Tower 2 | ✅ **this count is correct** |
| `EffectiveVisionRadius(` tree-wide | 2 | **2** | ✅ |
| `ReadFogState(` tree-wide | 3 | **3** | ✅ |
| `SpellLibrary.cpp` gathers | 2 hostile / 1 friendly | **2 / 1** | ✅ |
| `SetVisibility(` in the bar component | 1 | **1** | ✅ (raw 3) |
| `cards.csv` "31 columns" (`TASK-831` item 2) | 31 | **31** | ✅ |
| `ASpellLineSweep::LineRange = 900.f` @ `SpellLineSweep.h:103` | 900 @ :103 | **900.f @ :103** | ✅ **symbol AND line both exact** |

**⚠️ One instrument note for whoever owns `SiegeFogClampTest.cpp`:** the needle `FSiegeVisionQuery::Seeing` is a **prefix**, not a call shape — it matches both `SeeingFrom(` and `SeeingFromUnbounded(`. That is correct today and is presumably deliberate, but a future third factory beginning `Seeing…` would be counted silently. Flagged, not a defect.

---

## 7. ⚠️ BOARD-VS-TREE DRIFT — `TASK-830` AND `TASK-838` READ `backlog` BUT ARE SUBSTANTIALLY IN THE TREE

Not a `names:` defect, but it materially changes who can be dispatched, so it is reported rather than left for someone to trip over:

- **`TASK-838`** (`backlog`): `Tests/SiegeFogClampTest.cpp` **exists**; `FSiegeVisionQuery` and `ReadFogState` are in `SiegeCombatStatics.h`; the funnel makes its one `EffectiveVisionRadius` call. ⭐ **`FOG-§7b` has shipped BOTH halves** — `SiegeFogStatics.h` reads `meta = (ClampMin = "304.8")` and the sanitiser comparison is `<= 0.f` with a comment naming `FOG-§7b` half (b). `TASK-850`'s item (e) can be verified as satisfied today.
- **`TASK-830`** (`backlog`): `SummonedUnit.h` carries `WitchCastSeconds = 3.f` (:1127), `WitchVeilRadiusFallbackUU = 400.f` (:1146), `UpdateStateWitch`, `IsVeilCaster`; `SiegeInvisibilityTest.cpp` already asserts the cast channel.
  ⛔ **BUT its item (8) has NOT landed:** `GetCastProgressPercent` / `IsCastInProgress` are **0 tree-wide** (measured, §5). ⇒ **`TASK-860` is still genuinely blocked**, and its item (0a) STOP guard is correctly placed and will fire.

⭐ **A free gift for `TASK-831` (dispatchable now):** its spec says *"`Range` = the ungrouped veil fallback"* without giving the number. It is now measurable: **`WitchVeilRadiusFallbackUU = 400.f`** ⇒ the `Witch` row's `Range` cell should be **400**.

---

## 8. ⛔ WHAT QA SHOULD SCRUTINISE

1. ⭐⭐ **F-1's branch (b)** — that a fence-respecting `TASK-839` ships a **green suite and an inert fog card**. This is the one claim in this report whose consequence is invisible; please re-derive it from `ReadFogState`'s body rather than taking my word (`SC-§40` cl. 3).
2. **My probability call on F-3 ("LOW")** is a judgement, not a measurement. The counts are measured; the likelihood a fog resolver gathers agents is my opinion and you should overrule it if you disagree.
3. **F-2's resolution** — I assert collapse belongs to `TASK-861`. Confirm against `WITCH-§9.3`; if the law actually puts it C++-side, F-2 becomes a genuine conflict between item (5) and the pin, and the manager must rule which gives way.
4. **Rows I swept and cleared with nothing to report:** `826`, `827`, `828`, `829`, `832`, `833`, `834`, `835`, `840`, `841`, `842`, `843`, `847`, `848`, `851`, `858`, `859`, `862`, `863`. `840`/`831` are data-only (`cards.csv` path convention verified: `/Game/UI/CardArt/T_CardArt_<X>.T_CardArt_<X>` — both rows match). `862`/`850`/`849` are read-only gates and add no callers.
5. ⛔ **I changed nothing.** Every correction above is described, not applied. `git status` at my finish is byte-identical to my start apart from this file.
