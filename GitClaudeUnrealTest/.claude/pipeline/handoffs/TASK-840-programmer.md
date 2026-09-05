# TASK-840 — [F-5] the `Fog` card row — HANDOFF (gameplay-programmer)

**Status:** `ready-for-qa` · **Gate:** `TASK-850` (rescoped to this row alone) · **Date:** 2026-09-04

This row **ADDS a card** (32 → 33 data rows). It does not edit one. Both data halves landed
(`FOG-§9.11a`), each verified by read-back rather than by a success return.

---

## 1. The two anchors this row exists to reconcile — VERIFIED AT SOURCE, not trusted from the prompt

| anchor | where | measured value | verdict |
|---|---|---|---|
| `AFogVolume::FogDurationSeconds` | `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h:150` | `float FogDurationSeconds = 300.f;` | ✅ exists, is `300.f` |
| `ESpellEffect::FogCover` | `Source/GitClaudeUnrealTest/Siegebound/CardRow.h:97` | enumerator `FogCover` (last in the enum) | ✅ exists, spelled `FogCover` |

Neither was invented or renamed. `EffectDuration 300` and `FogDurationSeconds = 300.f` agree.
⚠️ **They agree by my writing them to agree — NOT by any mechanism.** See §6 WARN-1.

---

## 2. The CSV half — `Docs/Data/cards.csv`

- **32 → 33 data rows**; header unchanged at **32 columns**.
- **sha256 `d53c6861…f270` → `474d7ea0…f677`.** Bytes `7823 → 8003`.
- One **appended** line (last line of the file). CRLF preserved; file stays **pure ASCII, no BOM**.

**Control that no other byte moved** (stronger than a diff, and uses no git): I stripped the last
line from the post-edit file and hashed the remainder — it reproduces the baseline
`d53c6861…f270` **exactly**. ⇒ the file is *provably* the baseline plus one appended line.
**Zero other bytes changed.**

**`FOG-§9.8e` instrument control.** The first column header is **empty**. I keyed on
`list(row.keys())[0]` and ran the positive control: a known `CardID` comes back **non-empty**
(`'Footman'`), while the trap (`row['CardID']`) returns `None`. The table I verified against is
real, not a well-formed table of blanks.

**The row, quoted back from the file, all 32 fields:**

| # | column | value | | # | column | value |
|---|---|---|---|---|---|---|
| 1 | *(CardID)* | `Fog` | | 17 | `bSuicide` | `false` |
| 2 | `DisplayName` | `Fog` | | 18 | `SwarmCount` | `0` |
| 3 | `CardType` | `Spell` | | 19 | `AoERadius` | `0` |
| 4 | `Cost` | `50` | | 20 | `MinRange` | `0` |
| 5 | `MaxCopies` | `2` | | 21 | `SpawnCardID` | `None` |
| 6 | `HP` | `0` | | 22 | `SpawnInterval` | `0` |
| 7 | `Damage` | `0` | | 23 | `Lifetime` | `0` |
| 8 | `Range` | `0` | | 24 | `CardArt` | `/Game/UI/CardArt/T_CardArt_Fog.T_CardArt_Fog` |
| 9 | `Cadence` | `0` | | 25 | `SpellEffect` | `FogCover` |
| 10 | `Speed` | `0` | | 26 | `EffectDuration` | `300` |
| 11 | `Profile` | `None` | | 27 | `MaxTargets` | `0` |
| 12 | `Notes` | *(see below)* | | 28 | `GoldSteal` | `0` |
| 13 | `DeckCount` | `0` | | 29 | `ChainTargets` | `0` |
| 14 | `bRanged` | `false` | | 30 | `ChainFalloff` | `0` |
| 15 | `bCharge` | `false` | | 31 | `SpellDelivery` | *(blank)* |
| 16 | `bSlayer` | `false` | | 32 | `NoticeRange` | *(blank)* |

`Notes` = `Spell: raises battlefield-wide fog for 300s; unit target acquisition is clamped to
609.6uu while it is up; a re-cast refreshes the timer and never stacks (GDD 4)`
— **contains no comma** (asserted in the writer; a comma would shift every later field) and is
**pure ASCII** (the file has no non-ASCII byte and I did not introduce the first one — so no `§`).

**Field-count invariant:** every one of the 33 data rows carries **all 32 fields**. A blank cell
is still a field — my row ends `…,0,0,,` (two trailing empties = `SpellDelivery` + `NoticeRange`).

---

## 3. The `DT_Cards` half — `/Game/Data/DT_Cards`

`set_rows` only updates **existing** rows, so an added row needs `add_rows` first:

1. `add_rows(["Fog"])` → created with **struct defaults**.
2. `set_rows({"Fog": {…}})` → every cell above.
3. `get_rows` → **read back and quoted** (below). ⛔ Both writes returned `null`; I treated that as
   **no evidence** and proved the write by read-back.
4. `is_dirty` **`false` → `true`** (measured before *and* after the write).
5. `save_assets(["/Game/Data/DT_Cards"])` — **explicit one-asset list**, never the empty list
   (which would flush every dirty asset in a shared editor).
6. `is_dirty` **`true` → `false`**.
7. **sha256 measured OUTSIDE the engine — ⛔ never by size:**
   `1c8a2a5a8be3f0debee640a7bc52397ab73929a0d58a54d69c7cb26957923832`
   → `abe66ed250f05e432238fc2e50ced85117ddc5e1b7851f7e24f573d87566f2d0`
   *(size 43670 → 45035, recorded for completeness only — per `SC-§68` this asset has lied by size
   in both directions today, so size is not an instrument here.)*

**`Fog` as `get_rows` actually returns it:**

```json
"Fog": {"displayName": "Fog", "cardType": "Spell", "cost": 50, "maxCopies": 2, "hP": 0,
 "damage": 0, "range": 0, "cadence": 0, "speed": 0, "profile": "None",
 "notes": "Spell: raises battlefield-wide fog for 300s; unit target acquisition is clamped to 609.6uu while it is up; a re-cast refreshes the timer and never stacks (GDD 4)",
 "deckCount": 0, "bRanged": false, "bCharge": false, "bSlayer": false, "bSuicide": false,
 "swarmCount": 0, "aoERadius": 0, "minRange": 0, "noticeRange": 0, "spawnCardId": "None",
 "spawnInterval": 0, "lifetime": 0,
 "cardArt": "/Game/UI/CardArt/T_CardArt_Fog.T_CardArt_Fog",
 "spellEffect": "FogCover", "spellDelivery": "Auto", "effectDuration": 300,
 "maxTargets": 0, "goldSteal": 0, "chainTargets": 0, "chainFalloff": 0}
```

**Control both ways — two untouched rows, quoted back AFTER my write, unchanged:**

```json
"Longbowman": {… "cost": 18, "range": 3600, "noticeRange": 0, "deckCount": 2,
                "spellDelivery": "Auto", "cardArt": "…T_CardArt_Longbowman" …}
"Pickpocket": {… "cost": 18, "maxCopies": 2, "spellEffect": "GoldSteal", "goldSteal": 10,
                "noticeRange": 0, "effectDuration": 0, "spellDelivery": "Auto" …}
```

`Longbowman.range` is still **`3600`** and its `noticeRange` is still **`0`**. Untouched.

**Cross-half parity:** both halves hold **33 rows**, the **same 33 CardIDs**, in the **same order**
(`Fog` last in both). Every `Fog` cell matches across halves, including the two sparse ones:
CSV blank `SpellDelivery` → `Auto` (struct default, `CardRow.h:303`) and CSV blank `NoticeRange`
→ `0.0` (struct default, `CardRow.h:258`).

---

## 4. `sum(DeckCount)` — STATED, as required

**It stays `50`. It does not change.** `Fog` carries **`DeckCount = 0`**, so the asserted
exactly-50 invariant is untouched — verified by summing the column across all 33 rows *after* the
edit: **50**. This is deliberate: board spec item (3), *"keep the deck legal at exactly 50."*
`Fog` is a roster card, not a starter-deck card.

---

## 5. The `noticeRange` trap — NOT executed

`handoffs/TASK-993-programmer.md` §9.6's boldfaced *"REQUIRED … `set_rows`
`{"Longbowman": {"noticeRange": 3600}}`"* is **superseded and I did not run it.**

- I set `noticeRange` on **no row**, including my own — I never passed the key to `set_rows` at all,
  so the default supplies it rather than a literal.
- `CardRow.h:258` is `float NoticeRange = 0.0f;` ⇒ `add_rows`' default is already the correct end state.
- **Measured end state:** `Longbowman.noticeRange = 0` (read back post-write), and the CSV column is
  **fully sparse — all 33 cells blank.** The two halves agree at the default.

`Archer`/`Wizard` `Range` = `2100` and `Longbowman` `Range` = `3600` confirmed unchanged in the CSV.

---

## 6. ⚠️ FINDINGS — reported, NOT fixed. None is in this row's `names:` fence.

### WARN-1 (from `qa/TASK-1011.md`, routed **to this row**) — CONFIRMED AT SOURCE. **`EffectDuration` on this row is INERT.**

I did not take this on trust; I measured it:

- The `FogCover` arm (`SpellLibrary.cpp`) calls **`FogVolume->RaiseFog();`** — **no argument.**
- `RaiseFog` uses the actor's own property: `FogVolume.cpp:98` —
  `FogActiveUntilTimeSeconds = World->GetTimeSeconds() + static_cast<double>(FogDurationSeconds);`
- `Row.EffectDuration` **is** read by other arms (`SpellLibrary.cpp:175`, `:288`, `:313`, `:321`, `:463`)
  — so the absence in the `FogCover` arm is a real asymmetry, not a file-wide pattern.
- `grep EffectDuration` over `FogVolume.h` + `FogVolume.cpp`: **zero hits.**

⇒ **The `300` I just wrote does nothing at runtime.** The duration comes wholly from the CDO.
The two numbers agree **today because I wrote them to agree**; nothing enforces it, and if
`FogDurationSeconds` is ever retuned the CSV keeps saying `300` and **no test reddens.**

**I shipped the cell anyway, deliberately:** the board row names `EffectDuration 300` explicitly and
requires it to match the constant. Both are satisfied. But QA/manager should know the cell is
**documentation, not mechanism.** The suggested repair (the arm reads `Row.EffectDuration` with the
constant as fallback, **or** a test pins CSV↔CDO agreement) touches `SpellLibrary.cpp` or a test
file — **both outside my fence**, and `qa/TASK-1011.md` itself marks the fix *"not this row's."*
**It needs a row.** (NIT-4 — `CardRow.h:67`'s *"raises the WORLD-GLOBAL fog for `EffectDuration`"* —
pairs with this and is now actively misleading.)

### FINDING B — the deck-builder glossary will describe `Fog` **wrongly**, not merely blankly

`CONVENTIONS` line 14591 predicted a *blank effect line*. Measured, it is **worse than blank**:

- The glossary switch (`DeckBuilderWidget.cpp`) has arms for `AoEDamage`, `Freeze`,
  `TopTargetsDamage`, `AllyBuff`, `GoldSteal` — **no `FogCover` arm** ⇒ falls to `default: break;`
  ⇒ **no effect line.** (`grep -c FogCover DeckBuilderWidget.cpp` = **0**.)
- **Then the delivery line fires anyway.** `bLineCapableEffect` is `(AoEDamage || Freeze)` = false,
  so `bDeliversAsLine` = false, and the guard is `if (Row.SpellEffect != ESpellEffect::GoldSteal)` —
  **true for `FogCover`** ⇒ it prints `DeliveryGroundCircle`:
  > *"Aimed at a spot on the ground: it goes off where you place the reticle."*

That is **flatly false** for a battlefield-wide, no-reticle fog (`J-F1`, board item (2)).
`GoldSteal` is special-cased out of that exact line *because it has no aim*; **`FogCover` needs the
same exclusion** and does not have it. ⇒ the card will tell the player to aim a reticle it does not have.
`CONVENTIONS` 14591 already says this file **is in no row's fence and NEEDS A ROW.** Same gap will hit
`BrightSun` via `TASK-983`.

### FINDING C — stale prose (a comment, **not** an assertion; nothing reddens)

`Tests/SiegeCardArtRosterTest.cpp:228` reads *"32 rows in `Docs/Data/cards.csv`, 32
`T_CardArt_*.uasset` … ⇒ 32/32 PRESENT."* Now **33 rows**. It is documentation drift only — line 129
states `TestEqual(Rows, 32)` **is absent on purpose**, and I confirmed no test pins a row count.

---

## 7. Gate impact + SUITE DELTA

**Suite delta: `+0` tests, `+0` test files.** I authored no test — this row's `names:` fence contains
no test file, and the board row commands data only. ⛔ **Nothing in this batch has been executed;**
this is a *declared* delta, not an observed one. (For context, `qa/TASK-1011.md` declares
`446/34 ⇒ 453/35` for `TASK-998`; **my row adds nothing to either number.**)

Existing gates that now walk one more row — **no verdict change expected:**

- **`SiegeCardRosterTest`** — walks every `DT_Cards` row. `CardType::Spell` classifies **`NotSpawnable`**,
  so `Fog` is **EXCLUDED** from the actor-Blueprint probe and **obliges no `BP_*` asset.**
  Its info line moves `32 read; 22 SPAWNABLE; 10 EXCLUDED` → **`33 read; 22 SPAWNABLE; 11 EXCLUDED`**
  (`AddInfo`, not an assertion).
- **`SiegeCardArtRosterTest`** — walks **every** row and `AddError`s on an empty `CardArt` cell or a
  path that will not load as `UTexture2D`. **Confirmed live, not assumed:** `exists` → true,
  `load_asset("/Game/UI/CardArt/T_CardArt_Fog.T_CardArt_Fog")` returns the object, and
  `get_asset_class` = **`Texture2D`**. On-disk sha256 `513322e7daa7d9…` matches `TASK-842`'s declared
  `513322e7…`. ⇒ **passes.**
- **Deck legality (`UDeckLibrary::IsDeckLegal`)** — `sum(DeckCount)` = **50**, unchanged.

**Card-art orphan count, for the record:** 34 `T_CardArt_*.uasset` vs 33 rows. The one orphan is
`T_CardArt_BrightSun`, awaiting `TASK-983`. It was **34 vs 32 before my change** — my row *consumed*
one of the two orphans. The gate walks rows→art only (no orphan-direction assertion), so a spare
texture reddens nothing.

---

## 8. Things QA should scrutinise

1. **§6 WARN-1 is the important one.** I shipped an inert `EffectDuration`. Confirm you agree the row
   spec compelled it and that the repair belongs on a new row, not here.
2. **Finding B is a wrong string, not a missing one** — please confirm the severity read; it is a
   player-visible falsehood on a card that is otherwise correct.
3. **`MaxCopies = 2` is my choice; the board spec does not name it.** Justification: `UNCAP-§2` made
   `MaxCopies` the **hero-upgrade stack cap only** (`CardRow.h:161`); its per-deck copy-cap meaning is
   abolished, and only `HeroUpgrade` rows read it. On a `Spell` row it is **inert**. `2` matches the
   other two non-starter spells (`Lightning`, `Pickpocket`) and the two most recent cards
   (`Sorcerer`, `Witch`). It is also safe under the *old* reading, since `DeckCount = 0` means no cap
   can bind. Overrule freely — it is a one-cell change.
4. **`SpellDelivery` blank → `Auto` is the literal `Pickpocket` precedent** (board item (2)). Note
   `GetEffectiveDelivery` resolves `Auto` + `FogCover` → `GroundCircle`, exactly as it does for
   `Pickpocket`'s `GoldSteal`; the resolver ignores `TargetPoint` for both. An explicit
   `GroundCircle` cell would read as a *deliberate* reticle and would be wrong — blank is correct.
5. **`Cost 50` is off the ×3 grid, and that is intended** (`J-F8`). Board item (1a) asked me to state
   it: **`Fog`'s `50` is the game's only off-grid cost.** `BrightSun`'s `60 = 3 × 20` is **on** the
   grid — confirmed at source in `FOG-§10.1`. He broke his own grid **once, not twice.**

---

## 9. Files touched

| file | change |
|---|---|
| `Docs/Data/cards.csv` | **+1 appended line** (the `Fog` row). Zero other bytes — proven by hash reconstruction. |
| `/Game/Data/DT_Cards` (`Content/Data/DT_Cards.uasset`) | **+1 row** `Fog`, saved. sha256 `1c8a2a5a…` → `abe66ed2…`. |
| `.claude/pipeline/TASKBOARD.md` | **only** TASK-840's own `status:` line. |
| `.claude/pipeline/handoffs/TASK-840-programmer.md` | this file. |

**Not touched:** `CardRow.h` · `SpellLibrary.cpp` · `FogVolume.*` · `SummonedUnit.*` ·
`DeckBuilderWidget.cpp` · any test · any other card row's cells · `Archer`/`Wizard`/`Longbowman`
`Range` · any `NoticeRange` cell. **No compile. No Git command of any kind.**

**Editor:** left **UP** (PID 3172), MCP reachable, `DT_Cards` **clean** (`is_dirty` false). Not closed.

---

## 10. Serialisation note for the orchestrator

`TASK-983` (the `BrightSun` row) writes the **same two artefacts** and must not run concurrently with
this row. **This row is now finished writing** — both halves are saved and clean — so the file
collision is **over** and `TASK-983` is free to run.
