# TASK-1004 — [RANGE-REVERT-CSV] the `2100` revert + the Longbowman's notice cell goes blank

**Agent:** gameplay-programmer · **Date:** 2026-09-04 · **Status:** `ready-for-qa` · **Gate:** `TASK-1006`
**Law:** `FOG-§9.11` · `FOG-§9.11a` · `FOG-§9.8e` · `SC-§45` · `SC-§39`
**Sole file touched:** `Docs/Data/cards.csv`
**Consumer:** ⭐ **`TASK-1005` (build-master) — §6 below is its ONLY input. It is a boarded deliverable, not a courtesy.**

---

## 0. TOOLS, DECLARED ABOVE THE FINDINGS (`SC-§39`)

| Instrument | Used for | Controlled how |
|---|---|---|
| byte snapshot of the **pre-edit working tree** → `scratchpad/cards_PREEDIT.csv`, `md5 7c80ffa0930d897c4693a36510b1c351` | the baseline every claim below is measured against | ⛔ **NOT `HEAD`** — `cards.csv` was dirty from `TASK-993`, so `HEAD` is the wrong baseline and would have shown 33 changed lines instead of 3 cells |
| cell-addressed writer with an **assert on the OLD value** before each write | the three edits | a wrong-cell or wrong-baseline write **refuses** rather than lands; it cannot silently hit a neighbour |
| `csv.DictReader` keyed on `list(row.keys())[0]` | every value quoted below | `FOG-§9.8e` positive control **both ways** — `Footman.Range → 120` non-empty; `rows[0].get('CardID') → None` demonstrated; `ZzNoSuchCardZz` absent |
| `sha256sum` on `Content/Data/DT_Cards.uasset` | §5's claim that the asset is still wrong | a plain file hash — ⛔ **no engine, no MCP, no editor** |

⛔ **Nothing was executed.** No compile, no engine, no MCP, no Git, no `Source/` file, no `DT_Cards` write.

---

## 1. THE THREE CELLS, MEASURED CELL-FOR-CELL AGAINST THE PRE-EDIT WORKING TREE

```
MOVED  Archer       Range        '2000' -> '2100'
MOVED  Longbowman   NoticeRange  '3600' -> ''
MOVED  Wizard       Range        '2000' -> '2100'
TOTAL CELLS MOVED : 3        rows added: []      rows removed: []
```

⭐ **Three cells. Not three lines — three cells, derived by comparing all 32 rows × 32 columns.** A line diff cannot make this claim: against `HEAD` every one of the 33 lines is changed (`TASK-993` appended a column), so `git diff` is structurally incapable of answering "which cells moved". That is why the baseline is the byte snapshot and not `HEAD`.

### The fourth cell — the one that must NOT move

| cell | pre-edit | post-edit | |
|---|---|---|---|
| `Longbowman` `Range` | `3600` | `3600` | ✅ **ZERO DIFF — confirmed, not edited.** It never entered the edit list; the writer was never handed it |

The Longbowman's row *does* appear in a line diff, because its trailing `NoticeRange` field changed (`,,3600` → `,,`). ⚠️ **A reviewer seeing that line move must not read it as a `Range` edit** — column 8 is byte-identical either side. That is the confusion this row was most likely to be failed on, so it is stated before QA has to ask.

---

## 2. WHY THE LONGBOWMAN'S CELL GOES BLANK RATHER THAN TO `5000`

His universal `5000` notice ruling (`J-F28`, `FOG-§9.11`) retired the `J-F20` exception the same day it was written. At a `5000` class default, a literal `3600` in that cell would have made the Longbowman **the one unit in the game that notices LESS than everyone else** — inverting the very ruling that excepted the card **upward**.

⛔ **`5000` was NOT hand-typed into any cell**, and the file contains the substring `5000` **zero** times (machine-checked). The default lives in `ASummonedUnit::UnitEngagementRadiusUU` (`TASK-1003`'s lane); a blank cell means *"use the class default"* and inherits `5000` with no data duplication. Typing `5000` into 32 cells would have created 32 sites to update the next time he moves the number.

### The column STAYS, entirely sparse — and that is the deliverable

```
NoticeRange populated cells : []          (was: [('Longbowman', '3600')])
NoticeRange blank cells     : 32          (was: 31)
header last field           : NoticeRange  <- PRESENT
```

⛔ **The column was NOT deleted.** Per `FOG-§9.9` the **channel** is the deliverable, not any value in it: a future card opts into a per-card notice radius from its own row with **zero code**. A fully-sparse column is the correct resting state of a working channel, not evidence of a channel that was never needed.

⚠️ **This is exactly the shape a well-meaning reviewer deletes.** A column where every cell is blank looks like dead weight. It is not. `FCardRow::NoticeRange` exists, `ASummonedUnit::LoadStatsAndStart` binds it through `ResolveNoticeRadiusUU`, and `TASK-1003`'s ordering pin resolves **every** row through that resolver precisely so a future populated cell is *caught* rather than assumed absent. Deleting the column would silently break that pin's whole reason for scanning.

---

## 3. THE 33-LINE SHAPE LAW HELD — A BLANK CELL IS STILL A FIELD

`Tests/SiegeAssistantSelectionTest.cpp`'s `BuildDerivedRoster` hard-fails on `Fields.Num() < Header.Num()`, so blanking a cell must **empty** it, never **remove** it.

| shape property | post-edit | |
|---|---|---|
| line count | **33** | 1 header + 32 data |
| distinct field widths across all 33 lines | **[32]** | one uniform width — no short row exists to find |
| header identical pre/post | **True** | 32 fields, last is `NoticeRange` |
| first header cell | **`''`** | ⛔ `FOG-§9.8e` — left EMPTY, deliberately not "fixed" |
| CRLF | **×33**, bare LF **0** | preserved |
| trailing CRLF at EOF | **True** | preserved |
| quote characters introduced | **0** | preserved |

The Longbowman's row ends `...,0,0,0,0,0,,` — **two** trailing empty fields (`SpellDelivery`, `NoticeRange`), byte-identical in shape to every other unit row. The write path asserted the field count *after* each edit, so a dropped field could not have shipped.

---

## 4. `TASK-993`'s INVARIANTS RE-CONFIRMED (board item 6)

| invariant | measured | |
|---|---|---|
| `sum(DeckCount)` | **50** | ✅ |
| `Wizard` `AoERadius` | **250** | ✅ |
| no other cell moved | **3 cells total, enumerated in §1** | ✅ |
| rows added / removed | **[] / []** | ✅ |

### The ordering, DERIVED from this file (`FOG-§9.11`, `SC-§45`)

Machine-scanned every `bRanged` row of the post-edit file — ⛔ **not read from the `Notes` prose, not typed:**

```
bRanged rows : Archer 2100 · ArrowTower 900 · Longbowman 3600 · BombTower 800 · BallistaTower 1400 · Wizard 2100
LONGEST FIRING RANGE : Longbowman 3600
NoticeRange column fully sparse => every card resolves to the class default 5000
ORDERING  LEASH 8000 > NOTICE 5000 > FIRING 3600  =>  True
```

⚠️ **`8000` and `5000` are `TASK-1003`'s constants, not mine — I quote them, I do not own them.** What this row establishes is the **third** term: after the revert, the longest firing range in the roster is `3600` (Longbowman), and the notice column contributes no per-card override that could push a card above `5000`. If `TASK-1003` lands its constants as specced, the ordering holds. ⛔ **The authoritative pin is `TASK-1003`'s derived test — this is a data-side corroboration, not a substitute for it.**

---

## 5. ⛔⛔ `DT_Cards` IS ALREADY WRONG ON DISK — REPORTED, DELIBERATELY NOT FIXED

`TASK-993` did not merely edit the CSV: it **wrote and saved** `/Game/Data/DT_Cards` (its §9.2), putting `2000` into `Archer.range` and `Wizard.range` in the asset **the running game actually reads**.

**Measured now, from outside the engine:**

```
Content/Data/DT_Cards.uasset
  sha256 b8205df1c5b5dd2938097132bd842d48bd5ff16c13c6a651adb21bf5759274fa
  size   42,722    mtime 2026-09-04 02:55
```

That hash is **bit-identical to the one `TASK-993` recorded after its save** (`edffce06… → b8205df1…`). ⇒ ⛔ **Nothing has re-saved the asset since. The `2000` is still in it, right now.**

⛔ **I did not fix it — that is `TASK-1005`'s row, and this row's fence forbids the engine.** Under `FOG-§9.11a` the two halves land together or neither does, so **as of this moment the project is in the bad intermediate state the law names**: the CSV reads correct and the shipped game is still nerfed. ⚠️ **`TASK-1005` is not optional cleanup — until it runs, this row has made the discrepancy *harder* to notice, because every human-readable inspection of the data now says `2100`.**

⚠️ **Size would have lied.** Two in-place float edits left the asset at a byte-identical `42,722` (`SC-§68`). `TASK-1005` must verify by `sha256`, never by size.

### One thing my change RETIRES for the asset side

`TASK-993` §9.6 flagged a **required** post-compile `set_rows {"Longbowman": {"noticeRange": 3600}}`, warning that skipping it would leave the CSV and the asset disagreeing on the one cell the feature existed for. ⭐ **Blanking that cell dissolves that requirement.** After `TASK-987` compiles, every `DT_Cards` row gains `noticeRange` at its struct default `0.0` — and `0.0` now **agrees** with the blank CSV cell: both mean *"class default"*. The two halves converge with no write at all.

✅ **Corroborated, not newly escalated:** the manager already re-scoped `TASK-1001` from a write to a verification on exactly this reasoning (board `:18723-18727`). My data change is what makes that re-scope correct rather than merely convenient. ⛔ **`TASK-1005` must therefore write `range` and NOTHING else — a `noticeRange` write would now be a defect, not a leftover.**

---

## 6. ⭐⭐ `TASK-1005`'s TARGET TABLE — ITS ONLY INPUT

⛔ **Property names are the compiled-schema lowerCamel form (`range`), not the CSV header form (`Range`).** `range` is confirmed present in the editor's schema — `TASK-993` wrote it successfully — so **no compile is needed** for this write.

### (A) ROWS THAT MUST CHANGE — exactly two cells, nothing else

| `CardID` | property | current value in `DT_Cards` | ✅ **TARGET VALUE** |
|---|---|---|---|
| **`Archer`** | **`range`** | `2000` | ✅ **`2100`** |
| **`Wizard`** | **`range`** | `2000` | ✅ **`2100`** |

```json
{"Archer": {"range": 2100}, "Wizard": {"range": 2100}}
```

⛔ **No other row. No other property. No `noticeRange` on any row (§5).**

### (B) THE `SC-§39` CONTROL — rows that must read back UNCHANGED

Quote these back in the **same** read-back call as (A). If they move, the write was not surgical; if they are absent or constant, the instrument is not discriminating.

| `CardID` | property | ⛔ **MUST STILL READ** | why this control |
|---|---|---|---|
| **`Longbowman`** | **`range`** | **`3600`** | ⭐ the primary control — it is the **longest** firing range and the one a careless "revert all ranges" would clobber. Its CSV cell is a **zero diff**; the asset must match |
| **`Footman`** | **`range`** | **`120`** | a melee row, untouched by any range work this wave — proves the read is not echoing the write |
| `Wizard` | `aoERadius` | `250` | ⭐ same **row** as a write, different **property** — proves `set_rows` updated only the named property and did not replace the row |
| `Archer` | `deckCount` | `8` | same, on the other written row (`sum(deckCount)` must stay **50**) |
| `BallistaTower` | `range` | `1400` | a second ranged row nobody is writing |

⛔ **A read-back that returns the two target values but cannot show an unchanged control proves nothing** — `set_rows` returns `null` on success, so a silent accept-and-drop is indistinguishable from working. **A success return is not evidence.**

### (C) The post-write consistency check `TASK-1005` can make and I cannot

After the write, `DT_Cards` and `Docs/Data/cards.csv` must agree on **every** `range` cell. The CSV side is now authoritative and readable without the engine:

```
Archer 2100 · Wizard 2100 · Longbowman 3600 · Footman 120 · Knight 120 · Cleric 400
ArrowTower 900 · BombTower 800 · BallistaTower 1400 · CrystalTower 800 · WatchTower 0 · Witch 400
```

---

## 7. WHAT QA SHOULD SCRUTINISE

1. ⛔ **§5 first — `DT_Cards` is wrong on disk *right now*, and this row made it less visible, not more.** Grade whether `TASK-1005` is correctly blocking; the `FOG-§9.11a` pairing is the actual risk in this batch, not the three cells.
2. **§1's fourth cell.** `Longbowman`'s **line** moves (trailing `NoticeRange` field) while its **`Range` cell** does not. Re-derive cell-for-cell before reading that line as a `Range` edit.
3. **§2 — the fully-sparse column.** Confirm you agree it stays. If any reviewer wants `0` written into the 32 cells instead of blank, say so explicitly: `TASK-993` §6(B) already flagged that UE's numeric `ImportText` consumes nothing from an empty buffer and a **CSV re-import** would likely log *"Problem assigning string '' to property 'NoticeRange'"* ×32. ⛔ Inert under `set_rows`, which is the only write path boarded — but it is a real property of this file and it is now **32** rows, not 31.
4. **§6's table is a boarded deliverable.** Check it is complete enough to execute `TASK-1005` **without** reading the board — that is the point of `FOG-§9.11a`'s pairing.
5. **The baseline.** Every claim here is measured against the pre-edit **working tree**, not `HEAD`. If you re-derive against `HEAD` you will see 33 changed lines and conclude something much larger happened.
6. **§4's ordering is a corroboration, not the pin.** The authoritative derived test belongs to `TASK-1003`. Do not let this section be mistaken for satisfying that requirement.

---

## 8. SUITE COUNT (`TL-§5c` — declared, nothing executed)

This row touches **zero** `Source/` files and adds **zero** tests. **Δ = 0 / 0.** ⛔ No absolute is quoted: this row executed nothing and a remembered pass count is not a measurement.

---

**Files touched: `Docs/Data/cards.csv` — and nothing else.**
`md5 7c80ffa0930d897c4693a36510b1c351 → 0a2f45931bb95430ede3815b3b8f7465` · 7,827 → 7,823 bytes · 33 lines · 32 fields · CRLF preserved.
