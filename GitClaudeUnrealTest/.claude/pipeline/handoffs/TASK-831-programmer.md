# TASK-831 — THE `Witch` CARD ROW — programmer handoff

**Status:** `ready-for-qa` · **Gate:** `TASK-849` · **Law:** `WITCH-§4`/`§6`, "Data-driven card stats (GDD §3.0)", `UNCAP-§2`/`§3`, `SC-§39.1`, `SC-§40` cl. 9, `TL-§5c`
**Blocked-by:** none (row was `parallel-safe: yes`; `STACK-§7` says `cards.csv`/`DT_Cards` are untouched by the live wave — confirmed, nothing else moved either file during my task)
**Compile / Git:** ⛔ neither touched. **Editor:** used, MCP verified live by a real request before any write.
**Fence honoured:** ⛔ no mesh, material or card art · ⛔ no `SummonedUnit` / `SiegeCombatStatics` / `SiegeFogStatics` / `SiegeBotController` edit (all **read-only** for measurement) · ⛔ `WBP_CombatantHealthBar` untouched (`TASK-861`'s) · ⛔ `L_Arena` and `BP_FogArea` never saved — **I saved exactly one asset, by explicit path, never an empty list.**

---

## 1. WHAT SHIPPED

Exactly **one** row — `CardID` **`Witch`** — written to **both** sources of record and then **read back from each**: `Docs/Data/cards.csv` (byte-verified on disk) and `/Game/Data/DT_Cards` (round-tripped through MCP `get_rows`, then proven saved by a **sha256 change on the `.uasset`**, because the tool's own success signals turned out to be worthless — see §5). All **31 columns filled deliberately**; the two records agree **cell-for-cell** on all 29 comparable columns, so a future CSV re-import is a no-op for this row.

**Zero code changed. Zero tests added.** The spec has no test item, and the two things worth pinning are already pinned by `TASK-830`'s suite.

---

## 2. FILES / ASSETS TOUCHED

| thing | what |
|---|---|
| `Docs/Data/cards.csv` | **+1 line, appended after `WatchTower`.** No existing byte altered. |
| `/Game/Data/DT_Cards` | **+1 row** via MCP `add_rows` → `set_rows` (⛔ **not** a CSV re-import, per spec (3)). Saved. `41378 → 42722` bytes, sha256 `eedc07e4…` → `edffce06…`. |

⛔ **Not touched:** any `.h`/`.cpp`, any test, any mesh/material/texture, any Blueprint, `L_Arena`, `BP_FogArea`.

---

## 3. ⭐ THE ROW, AND WHY EACH NUMBER IS THAT NUMBER

| column | value | why — ⛔ measured, not chosen by feel |
|---|---|---|
| RowName / `DisplayName` | `Witch` / `Witch` | ⛔ **character-for-character.** `TASK-830` hard-couples to it: `WitchCardID` is a file-scope constant and `IsVeilCaster()` is `CardID == WitchCardID`. Any other spelling spawns a unit that **never casts, with nothing in any log** (830 §6.8). |
| `CardType` | `Unit` | spec (1) |
| `Cost` | **50** | 🧑 his number. ⚠️ Off the ×3 grid (`J-F8`) — **shipped as-is, deliberately not "corrected" to 51.** |
| `Profile` | **`Support`** | spec (1)+(6). Makes her follow-eligible for free (`CanFollowHero()` covers `Support`) ⇒ Follow is her spawn default, which is the case `TASK-830`'s circle resolver is built around. ⛔ No fourth profile invented. |
| `Damage` | **0** | spec (1) — she does not attack. ⚠️ **This is trap 1 and it is only safe because of 830's guard** — see §4. |
| `Range` | **400** | ⭐ **load-bearing twice** — see §4. Matches `WitchVeilRadiusFallbackUU = 400.f` and `TASK-830` §10's explicit request. |
| `HP` | **70** | The **Sorcerer's** body. She and the Sorcerer are the project's only two `CanEverAttack() == false` units (the Cleric's `CanEverAttack()` is **true**; its seal is per-body) ⇒ the Sorcerer is the structural analogue, and 60g↔50g is the adjacent cost tier. ⚠️ Declared: **my call**, §6.1. |
| `Speed` | **350** | The fleet caster speed (Archer/Pikeman/Cleric/Wizard/Sorcerer/Miner). ⭐ Not cosmetic — see §4.3. |
| `Cadence` | **0** | The shipped convention for a unit that never attacks (Sorcerer 0, Miner 0). ⚠️ Safe **only** because of 830's three-part seal — §4.2. |
| `MaxCopies` | **2** | ⛔ **Inert for a Unit.** `UNCAP-§2`/`§3`/`§4` abolished the copy cap; the column now means *hero-upgrade stack cap only* (`AHeroCharacter::ApplyUpgrade` reads it; `UDeckLibrary::IsDeckLegal` no longer does). Matched to the Sorcerer's 2. ⛔ Not `0` — no shipped row carries 0, and `0` is the value that **refuses the play** for upgrades, a bad signal to plant. |
| `DeckCount` | **0** | ⛔ **REQUIRED — see §4.1.** |
| `CardArt` | `/Game/UI/CardArt/T_CardArt_Witch.T_CardArt_Witch` | ⭐ **The full object path, written now, though the texture does not exist** — see §4.4. |
| `Notes` | see below | ⛔ **No comma** — see §4.5. |
| everything else | `false` / `0` / `None` / blank | The sparse defaults, cross-checked cell-by-cell against the `Cleric` row. `bRanged` **false** (a `true` + `Damage 0` would fire zero-damage projectiles). `SpellDelivery` **blank ⇒ `Auto`**, matching all 31 shipped rows. |

**Notes cell (verbatim):**
> `Support: cannot attack; veils nearest enemy-visible friendly in her position circle over a 3s interruptible cast one at a time; Range 400 is the ungrouped veil radius not an attack range (WITCH-4)`

Law cited as `WITCH-4` without the `§` — the shipped precedent is `WatchTower`'s `(TOWER-3)`. ⛔ Deliberate: `§` is non-ASCII and `cards.csv` is currently **pure ASCII** (measured); one `§` would be the only non-ASCII byte in the file and would have to survive both an MCP round trip and a future re-import for zero benefit.

---

## 4. ⛔ THE INTERACTIONS — the half of this task that is not typing

### 4.1 The deck arithmetic (spec (4))
Measured, not cited: the `DeckCount` column across the **31 prior rows sums to exactly 50** (Footman 9 + Archer 8 + Knight 3 + Miner 3 + ArrowTower 3 + Wall 4 + MilitiaMob 3 + Pikeman 3 + Cavalry 3 + Longbowman 2 + Cleric 2 + Ogre 2 + Fireball 2 + FrostNova 1 + Sorcerer 2 = 50; all others 0).
⇒ **`Witch` `DeckCount` MUST be 0**, and it is. **Re-measured after the write: still exactly 50.** Any non-zero value would have made the shipped default deck illegal (`UDeckLibrary::IsDeckLegal` binds on the exactly-50 total). This also matches the shipped pattern for recently-added cards — `Wizard`, `WatchTower` and every building/spell added since ship carry 0.
⛔ `MaxCopies` does **not** enter this arithmetic at all under `UNCAP-§2`; the spec pairs the two columns but only `DeckCount` binds today. Said explicitly so nobody re-derives it.

### 4.2 ⭐⭐ Trap 1 — `Damage 0` + `Support` is the exact pair that would have un-veiled her, and `Cadence 0` is a second edge on the same knife
The dispatch flagged that a `Damage`-0 Support witch would arm the Cleric heal timer and call `BreakInvisibility(Heal)` every 0.1 s for zero HP. **I verified the fix is really in the shipped source before writing the values** — `ASummonedUnit::UpdateSupportHealTargeting()` opens with `if (IsVeilCaster()) { StopHealing(); SupportHealTarget = nullptr; return nullptr; }`, above the Cleric's four extracted statements. ⇒ `Damage 0` is safe.

⚠️ **And a second one I found while pricing `Cadence`, which the dispatch did not name.** `SummonedUnit.h:641` records: `AttackCadence = FMath::Max(Row->Cadence, MinAttackCadence)` and **`MinAttackCadence` is 0.05 s** — so a `Cadence`-0 row that ever reached `Attack` would fire **20×/s**. That is survivable here only because `TASK-830` extended the Sorcerer's **three-part** seal to the witch (`EnterAttack()` stands her down · `UpdateStateGrouped()` acquires nothing · `PerformAttack()` refuses), and because her `Damage` is 0 so even a breach delivers nothing. ⇒ **`Cadence 0` and `Damage 0` are each safe only in the presence of 830's code.** If 830 is ever reverted or its seal narrowed, **this row becomes a 20-attacks-per-second unit** — recorded here because the coupling is invisible from the data side.

### 4.3 ⭐⭐ `Range = 400` is load-bearing in **two** places, and one of them is not the circle
1. **The veil radius.** `ResolveWitchPositionCircle` does `OutRadius = (AttackRange > 0.f) ? AttackRange : WitchVeilRadiusFallbackUU;` and `AttackRange = Row->Range` (`SummonedUnit.cpp:1267`). With Follow being the spawn default and a Follow group carrying `PositionRadius == 0`, **the common witch uses this cell**, not a circle.
   ⚠️ **Honest measurement, because it cuts against the dispatch's framing:** since `WitchVeilRadiusFallbackUU` also defaults to `400.f`, `Range = 0` would yield the *same* 400 radius via the backstop. So 400 is not load-bearing *for the radius value* — it is load-bearing for **where the tuning knob lives**: at 400 the **data** drives it (GDD §3.0's whole point) and the constant is a genuine backstop; at 0 the column would be a lie and the number would be unreachable from `cards.csv`.
2. ⭐ **The move acceptance radius — this one has no fallback.** Her cast subject is a Pawn, so `UpdateStateWitch` → `EnterAdvance(Goal)` reaches `AI->MoveToActor(Goal, FMath::Max(AttackRange * 0.8f, 40.f), true)` (`SummonedUnit.cpp:3434`). **`0.8 × 400 = 320 uu`**, and `320 < 400` ⇒ **she halts comfortably inside her own veil circle, so the cast she walked over to start is still valid when she arrives.** With `Range = 0` this would be `max(0, 40) = 40 uu` — she would shove her face into the subject's capsule. This is the same 320 the shipped Cleric uses, i.e. behaviour Jonathan has already played.

### 4.4 The missing `T_CardArt_Witch` — handled the way the shipped rows already handle it, ⛔ no new convention
`T_CardArt_Witch` does not exist (`TASK-834` deliberately not dispatched). I wrote **the full object path anyway**, which is exactly the shipped mechanism, not a workaround: `FCardRow::CardArt` is a `TSoftObjectPtr<UTexture2D>` (stores a path without resolving), and `UCardHandWidget::ResolveCardArtTexture` handles the unresolvable case in a branch whose own comment says *"normal until TASK-078 lands"* — `LoadSynchronous()` returns null → **logged once per CardID** → **text-only card face** → ⛔ never a crash. ⛔ No placeholder texture, no empty cell, no sentinel invented.
⭐ **And I did not assume the path would survive:** a soft pointer to a nonexistent asset is exactly the sort of thing a setter nulls out. **`get_rows` read it back intact**, and the string `T_CardArt_Witch` is present in the saved `.uasset` bytes. When `TASK-834` lands the texture at that path, the card face lights up with **zero data edits**.

### 4.5 The comma ban in `Notes` is not style — it is what keeps a shipped test alive
`Tests/SiegeFogClampTest.cpp` **parses `Docs/Data/cards.csv` at runtime** and **skips any row whose field count disagrees with the header**, precisely because the free-text `Notes` column can carry an embedded comma. ⇒ **a comma in my Notes cell would have made the probe silently skip the Witch row.** Mine has none (asserted in the writer script before the append), so the row stays header-aligned and readable. Its assertions are all unaffected by my row regardless: `RowsRead >= 10` (31→32), largest AoE stays `Lightning` 700, and my `AoERadius 0` adds nothing to the above-ceiling count.

---

## 5. ⛔⛔ AN INSTRUMENT DEFECT I MEASURED — `is_dirty` IS BLIND (`SC-§39.1`, new member of the family)

The dispatch warned that `save_asset` can return `true` while writing nothing. I went looking for that and **found a different blind instrument next to it.**

**`AssetTools.is_dirty` returns `true` for every asset that exists, regardless of actual state.**

| probe | result |
|---|---|
| `DT_Cards` **before** save | `true` |
| `DT_Cards` **after** a successful save | `true` ← should be `false` |
| `T_CardArt_Cleric` (untouched; disk mtime **2026-07-07**) | `true` ← control, cannot be dirty |
| `SM_Witch` (untouched this task) | `true` ← control |
| a **nonexistent** path | **errors correctly** ⇒ it does resolve the asset; it is not merely echoing |

⇒ ⛔ **`is_dirty` can be used neither to prove an edit registered nor to prove a save landed.** My own earlier reading — *"the package is dirty, so the `MapKey` trap did not fire"* — was **worthless as evidence**, and I am retracting it here rather than letting it stand in the record.

⭐ **The only evidence that survives is the disk, and it is unambiguous:** `Content/Data/DT_Cards.uasset` went **41378 → 42722 bytes**, sha256 `eedc07e4ed9356ef…` → `edffce064bff1a51…`, mtime moved to today, and the byte strings `Witch` **and** `T_CardArt_Witch` are present in the saved package (neither was before). Combined with the `get_rows` round trip and `list_rows` returning **32** rows with all 31 originals intact and in order, the write is proven three independent ways.

---

## 6. ⚠️ DECLARED — my calls, and one consequence QA/manager must rule on

### 6.1 `HP 70` and `MaxCopies 2` are mine
Neither is in his sentence or the spec. `HP 70` = the Sorcerer's, reasoned in §3. If the manager prefers the Cleric's `90` it is a one-cell change with no code impact. `MaxCopies` is provably inert for a `Unit` and can be any value.

### 6.2 ⭐⭐⭐ **THE ONE REAL CONSEQUENCE — MY ROW MOVES THE TAIL OF THE ASSISTANT ROSTER, AND THAT TAIL IS THE SUBJECT OF ONE OF JONATHAN'S OWN DEFECTS. ⛔ I DID NOT "FIX" IT.**

`Tests/SiegeAssistantSelectionTest.cpp` carries a hand-transcribed table, **`ThirteenKindsInCardRowOrder()`**, whose comment states in writing:

> *"THE 13 COMMANDABLE KINDS, IN DT_Cards ROW ORDER — read off `Docs/Data/cards.csv`… ⛔ **THE ORDER IS THE TEST, NOT DECORATION.** `Sorcerer` is the LAST commandable row in the table, which is the entire mechanism of Jonathan's defect: the shrink loop always drops from the TAIL, so the Sorcerer is the first kind hidden on every board… `Capture()` only ever tallies live `ASummonedUnit`s, so a roster can hold exactly these 13."*

**Measured against the shipped source, not inferred:** `USiegeAssistantSnapshot::Capture` iterates `TActorIterator<ASummonedUnit>` and filters on **only** validity, team and death — ⛔ **no `Profile` filter, no `CardType` filter** — then takes `CanonicalKind(Unit->GetCardID())`, which is a **total** lower-casing of any non-`None` CardID. The Witch is an `ASummonedUnit` with a bound CardID.

⇒ **Three statements in that comment are now false in the shipped data:**
1. There are **14** commandable kinds, not 13.
2. **`witch` — not `sorcerer` — is the last commandable row** in DT_Cards row order (I appended, which is the shipped convention: `Wizard`, `Sorcerer`, `WatchTower` were all appended).
3. ⇒ **the shrink loop now hides the *Witch* first on every board, not the Sorcerer.**

**✅ No test goes red.** The array is a static transcription, not a runtime read; its 13 entries and their relative order are unchanged, so every assertion still holds. **The defect is that the test now guards a case that is no longer the live tail** — the exact `SC-§38` citation-rot / `SC-§40` cl. 9 shape, arriving through *data* rather than through a `file:line`.

⛔ **I did not touch that file, and I believe that is the right call:** it is outside my names list, it belongs to a different shipped defect of Jonathan's, and its own comment warns that reshaping the array *"would quietly convert the collapse tests into tests of a case that cannot happen"* — precisely the damage a well-meaning edit would do. **Boarding this is a manager decision.** ⚠️ Note the ordering hazard for whoever takes it: inserting `Witch` *before* `Sorcerer` in DT_Cards would preserve the old tail but reorders shipped data and is worse; the honest fix is to update the transcription to 14 and re-point the tail.

### 6.3 A cosmetically stale label, ⛔ not a failure
`Tests/SiegeLadderClimbTest.cpp`'s `ShippedUnitSpeedNames[]` reads `"Archer / Pikeman / Cleric / Wizard / Sorcerer (350)"` — now an incomplete enumeration. ⛔ **Nothing breaks:** the asserted array is the **deduplicated set** of distinct speeds `{250,300,350,400,500,600}`, and **350 is already in it**. Only a failure-message string is stale.
⭐ **This is the payoff for `Speed 350` rather than an invented number:** a novel speed (say 320) would have added a shipped unit speed the roster sweep does **not** cover, silently narrowing that test's span over the fleet.

### 6.4 Residuals inherited, not introduced
The row cannot be played end-to-end yet: **`BP_Unit_Witch` does not exist** (`TASK-835`'s integration — parent `ASummonedUnit`, there is no witch C++ class, deliberately, per 830 §5a), and `T_CardArt_Witch` awaits `TASK-834`. ⛔ **A green row is not "the witch works."**

---

## 7. SUITE DELTA (`TL-§5c`)

- **My delta: `0`.** ⛔ No test file touched, no test added — the spec has no test item, and this is a data row.
- **Tree census, re-measured by symbol** (`^IMPLEMENT_SIMPLE_AUTOMATION_TEST` scoped to `Siegebound/Tests/*.cpp`): **408 declared across 30 files.**
- ⭐ `SC-§40` cl. 9 honoured: that number **matches** `TASK-830`'s reported end-state, and I am reporting it **because I re-measured it**, not because it agreed.
- ⛔ **`declared`, ⛔ not executed** — no compile, no suite run in my lane.

---

## 8. ⛔ WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐⭐⭐ **§6.2 — the assistant-roster tail.** The highest-consequence thing in this task and it is not in the row. Confirm you agree the Witch really does enter `Capture()`'s roster (no profile filter) and that this belongs to the **manager** to board, not to me to patch.
2. ⭐ **§4.2 — `Cadence 0` + `MinAttackCadence 0.05`.** Confirm the three-part seal genuinely covers her, since the data alone would license 20 attacks/s.
3. ⭐ **§4.3 — `Range 400`.** I argued it is load-bearing for the **move acceptance radius (320 uu)** and for *where the knob lives*, and I **corrected the dispatch's framing** on the radius value itself (0 would give the same 400 via the backstop). Please rule on that correction rather than let it pass silently.
4. **§3 — `HP 70` and `MaxCopies 2`,** the two cells nobody specified.
5. **§5 — `is_dirty` is blind.** If you concur, this deserves a law row beside `SC-§39.1`; other lanes are using that tool as a gate.
6. **`Witch` spelling**, character-for-character, against `WitchCardID` — the whole feature hangs off it.
7. **The CSV↔DT_Cards agreement.** I diffed all 29 comparable columns programmatically (0 mismatches); re-run it if you want independent confirmation.

## 9. FOR `TASK-835` (integration)
Create `/Game/Blueprints/Units/BP_Unit_Witch` parented to **`ASummonedUnit`** (⛔ no witch C++ class), mesh `/Game/Meshes/SM_Witch`, materials `[MI_TeamColor_Blue, MI_Witch_PBR]` per `TASK-833`. The row is live in `DT_Cards` **and saved**, so the card is playable the moment the Blueprint exists.
