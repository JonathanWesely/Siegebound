# TASK-1005 — [RANGE-REVERT-DT] — build-master handoff

**Status:** ✅ **DONE 2026-09-04.** The `FOG-§9.11a` bad intermediate state is **CLOSED**.
**Law:** ⭐⭐ `FOG-§9.11a` · ⭐ `SC-§68` · `SC-§39` · `SC-§40` · `§25b`.
**Input (the only value source):** `handoffs/TASK-1004-programmer.md` §6. ⛔ **No value was taken from the board.**
**Consumer:** ⭐ `TASK-1006` (qa-reviewer) — §4 below is the read-back it is told to check.

---

## 0. TOOLS, DECLARED ABOVE THE FINDINGS (`SC-§39`)

| Instrument | Used for | Controlled how |
|---|---|---|
| `DataTableTools.get_rows` **before AND after** the write | every value quoted below | the **same** call shape both times, over the **same 5 rows** — a before/after pair on one instrument, so a constant-returning or input-echoing reader is detectable |
| cell-level diff of the two read-back payloads (`scratchpad/dt_pre.json` / `dt_post.json`) | the "nothing else moved" claim | **155 cells** compared (5 rows × 31 props); a whole-row replacement or neighbour-cell hit **cannot** hide |
| `sha256sum` on `Content/Data/DT_Cards.uasset`, **outside the engine** | the save actually reached disk | ⛔ **never size** (`SC-§68`) — size is reported below only as an *observation*, never as evidence |
| raw **float-byte census** of the saved `.uasset` | independent confirmation the shipped bytes changed | `struct.pack('<f', v)` — reads the file as bytes, **no engine, no MCP**; immune to any editor-side lie |
| `find Content -newermt '-15 minutes'` | proof the one-asset save flushed nothing else | a filesystem sweep, **not** Git |

⛔ **Not executed, by fence:** any compile · **any Git command of any kind** · `L_Arena` (⛔ **not even dirty-checked**) · `Docs/Data/cards.csv` **written** (read-only, for §5) · any other `DT_Cards` cell · `Content/FogArea/**`.

---

## 1. ENVIRONMENT, RE-CONFIRMED BEFORE ACTING

```
UnrealEditor.exe   PID 3172   (Console, 4,461,452 K)
TCP 127.0.0.1:8000 LISTENING  PID 3172   <- same PID owns the MCP port
```

⭐ The MCP port is owned by **the same PID** as the editor — so this is that editor's server, not a stale listener from a dead process. The editor was left **OPEN**, as the row requires.

---

## 2. ⛔⛔ THE BAD STATE WAS CONFIRMED LIVE BEFORE THE WRITE — TWICE, TWO WAYS

**On disk, before touching anything:**

```
Content/Data/DT_Cards.uasset
  sha256 b8205df1c5b5dd2938097132bd842d48bd5ff16c13c6a651adb21bf5759274fa
  size   42,722    mtime 2026-09-04 02:55:51
```

⭐ **Bit-identical to the hash `TASK-1004` recorded** ⇒ nothing had re-saved the asset in the interval, and the defect was still live at the moment I started.

**In the engine, before the write** (`get_rows`, quoted verbatim):

| `CardID` | property | value read | |
|---|---|---|---|
| `Archer` | `range` | **`2000`** | ⛔ the live defect |
| `Wizard` | `range` | **`2000`** | ⛔ the live defect |

`is_dirty` = **`false`** ⇒ a clean baseline; the `2000` was the **saved** state, not an unsaved edit.

---

## 3. THE WRITE — EXACTLY TWO CELLS

```json
set_rows /Game/Data/DT_Cards  <=  {"Archer": {"range": 2100}, "Wizard": {"range": 2100}}
```

⛔ **Returned `null`.** As the row warned, that is the success return **and** what a silent accept-and-drop returns. **It is not evidence and is not treated as any.** Everything in §4 is what settles it.

---

## 4. ⭐⭐ THE READ-BACK — QUOTED, WITH BOTH-WAYS CONTROLS (`SC-§39`)

### (A) The two cells that had to move

| `CardID` | property | before | **after** | |
|---|---|---|---|---|
| **`Archer`** | **`range`** | `2000` | ✅ **`2100`** | MOVED, as specified |
| **`Wizard`** | **`range`** | `2000` | ✅ **`2100`** | MOVED, as specified |

### (B) The five controls — quoted from the SAME call as (A)

| `CardID` | property | before | **after** | | why this control |
|---|---|---|---|---|---|
| **`Longbowman`** | **`range`** | `3600` | ✅ **`3600`** | **UNCHANGED** | the primary control — longest firing range, the cell a careless "revert all ranges" clobbers. ⛔ **CONFIRMED, NOT WRITTEN** |
| **`Footman`** | **`range`** | `120` | ✅ **`120`** | **UNCHANGED** | a melee row — proves the reader is not echoing the write |
| **`Wizard`** | **`aoERadius`** | `250` | ✅ **`250`** | **UNCHANGED** | ⭐ **same row as a write, different property** — proves `set_rows` updated only the named property and did **not** replace the row |
| **`Archer`** | **`deckCount`** | `8` | ✅ **`8`** | **UNCHANGED** | the same test on the other written row |
| **`BallistaTower`** | **`range`** | `1400` | ✅ **`1400`** | **UNCHANGED** | a second ranged row nobody is writing |

⭐ **The read is therefore discriminating in both directions**: it reports change where change was made and no change where none was — so it is neither echoing my input nor returning a constant.

### (C) The machine diff — "nothing else moved" is measured, not eyeballed

```
rows compared      : 5
cells compared     : 155
CELLS MOVED        : 2
   MOVED  Archer   range  2000 -> 2100
   MOVED  Wizard   range  2000 -> 2100
```

⛔ **2 of 155.** No neighbour cell, no row replacement, no default-reset.

### (D) `noticeRange` — NOT WRITTEN, on any row

```
noticeRange after write: {Archer: 0, BallistaTower: 0, Footman: 0, Longbowman: 0, Wizard: 0}
across all 32 rows     : noticeRange_all_zero = True   (nonzero cells: {})
```

⛔ **No `noticeRange` write was issued.** Per `TASK-1004` §5 such a write would now be a **defect**, not a leftover.

⚠️⭐ **FINDING — the board's premise here is out of date, in the direction that makes the instruction MORE correct.** The row states `noticeRange` "is **absent from the schema today**" and becomes the sparse sentinel only "once `TASK-987` compiles". **Measured: it is already present in the schema now, reading `0` on all 32 rows.** ⇒ the desired end state is **already achieved, with zero writes**, and it already agrees with the CSV's 32 blank cells. The instruction not to write it was right; only its stated reason has moved. **Nothing is owed** — recorded so `TASK-987`/`TASK-1001` are not planned around a premise that has already resolved itself.

---

## 5. `FOG-§9.11a` — BOTH HALVES NOW AGREE, ON EVERY CELL

The post-write consistency check `TASK-1004` §6(C) says only this row can make:

```
CSV rows: 32   DT rows: 32   row-name sets identical: True
RANGE cells compared : 32
RANGE MISMATCHES     : 0
```

| `CardID` | `cards.csv` `Range` | `DT_Cards` `range` | |
|---|---|---|---|
| `Archer` | `2100` | **`2100`** | ✅ AGREE |
| `Wizard` | `2100` | **`2100`** | ✅ AGREE |
| `Longbowman` | `3600` | **`3600`** | ✅ AGREE |

Full asset `range` column, all 32 rows:
`Footman 120 · Archer 2100 · Knight 120 · Miner 0 · ArrowTower 900 · Wall 0 · MilitiaMob 120 · Pikeman 120 · Sapper 120 · Cavalry 120 · Longbowman 3600 · Cleric 400 · Ogre 120 · BombTower 800 · BallistaTower 1400 · Barracks 0 · DeepMine 0 · Masons 0 · SharpenedBlade 0 · PlateArmor 0 · SwiftBoots 0 · WarBanner 0 · Fireball 0 · FrostNova 0 · Lightning 0 · BattleCry 0 · Pickpocket 0 · CrystalTower 800 · Wizard 2100 · Sorcerer 0 · WatchTower 0 · Witch 400`

⇒ matches `TASK-1004` §6(C)'s expected list **exactly**.

**Also re-confirmed on the asset side:** `sum(deckCount) == 50` ✅ · `Wizard.aoERadius == 250` ✅ · `NoticeRange` sparse on **both** halves (CSV: **32/32 blank**, populated `{}` · asset: **32/32 zero**).

⇒ ⭐⭐ **The window `TASK-1004` opened is closed. Every human-readable inspection and the asset the running game reads now say the same thing.**

---

## 6. THE SAVE — EXPLICIT ONE-ASSET LIST, VERIFIED OUTSIDE THE ENGINE

```
save_assets(["/Game/Data/DT_Cards"])   ->  true      <- ⛔ explicit list, NEVER the empty list
```

| check | before | after | |
|---|---|---|---|
| `is_dirty` | `false` → (write) → **`true`** | **`false`** | ✅ the true→false transition, both legs observed |
| **`sha256`** | `b8205df1c5b5dd2938097132bd842d48bd5ff16c13c6a651adb21bf5759274fa` | ✅ **`1c8a2a5a8be3f0debee640a7bc52397ab73929a0d58a54d69c7cb26957923832`** | **CHANGED** — the save reached disk |
| mtime | `2026-09-04 02:55:51` | `2026-09-04 16:11:47` | |
| size *(observation only)* | `42,722` | `43,670` | ⚠️ see §7 |

**Nothing else was flushed** — the whole `Content/` tree, swept by mtime:

```
Content/ files modified in the last 15 minutes:
  2026-09-04 16:11:47   43670   Content/Data/DT_Cards.uasset
  (nothing else)
```

⇒ the explicit one-asset list did **not** flush other dirty assets in this shared editor. ⛔ `L_Arena` was **not touched and not dirty-checked**.

### The independent byte-level confirmation (no engine, no MCP)

Float-byte census of the **saved file**:

```
float 2100.0  bytes 00 40 03 45  ->  2 occurrences   <- Archer + Wizard
float 2000.0  bytes 00 00 fa 44  ->  0 occurrences   <- ⭐ the retired value is GONE from the shipped asset
float 3600.0  bytes 00 00 61 45  ->  1 occurrence    <- Longbowman, intact
float 1400.0  bytes 00 00 af 44  ->  1 occurrence    <- BallistaTower, intact
float  120.0  bytes 00 00 f0 42  ->  8 occurrences   <- melee rows, intact
```

⭐⭐ **This is the strongest evidence in the report and it does not trust the editor at all**: the bytes the packaged game will load contain `2100` twice and `2000` **zero** times.

---

## 7. ⚠️ THE ONE THING IN THE DIFF THAT IS NOT THE TWO CELLS — DECLARED, NOT BURIED

The asset grew **+948 bytes** for what was two in-place float edits. That is *unexpected* and is stated plainly rather than left for QA to trip over.

- ⛔ **It is not evidence of anything and was not used as any** — the verdict rests on `sha256` + the byte census, exactly as `SC-§68` requires. (Note the irony worth recording: `TASK-993`'s edit was byte-identical at `42,722`, so **size lied by not moving**; here size moved for a reason unrelated to the cells. **Size is uninformative in both directions** — the law is right.)
- **Most probable cause, arithmetic exact:** the `NoticeRange` property is now persisted for every row. `32 rows × 29 B` (tagged-property record) `+ 20 B` (one `FName` table entry: `4` len `+ 12` string `+ 4` hash) **= 948 B**, matching the delta **exactly**. `NoticeRange` is confirmed present in the file's name table.
- ⚠️ **Flagged as INFERENCE, not measurement.** I could not diff against the previous bytes: recovering them needs Git, and **this row forbids every Git command**. The arithmetic matching to the byte is suggestive, not proof.
- ✅ **Either way it is benign and is the state the board wants**: every `noticeRange` is `0.0` (the struct default = the correct sparse sentinel), agreeing with all 32 blank CSV cells. **No data changed** — a default-valued property being serialized is a save-path artifact, not a value edit, and I issued no write to that column.

⇒ **For `TASK-1006`:** if the size delta must be *proved* rather than inferred, that check requires a Git-side byte diff and belongs to a row that is permitted Git.

---

## 8. SCOPE — WHAT WAS AND WAS NOT DONE

| | |
|---|---|
| ✅ written | `/Game/Data/DT_Cards` — **`Archer.range` + `Wizard.range` ⇒ `2100`**, and nothing else |
| ✅ saved | `/Game/Data/DT_Cards`, explicit one-asset list |
| ⛔ not run | **any Git command of any kind** · any compile · any `Source/` edit |
| ⛔ not touched | `L_Arena` (not even dirty-checked) · `Docs/Data/cards.csv` (**read only**, for §5) · `Content/FogArea/**` · every other `DT_Cards` cell · `noticeRange` on every row |
| ⛔ editor | left **OPEN** (PID 3172), as the row requires. `TASK-987`'s compile will need it closed — **that is not this row** |

---

## 9. FOLLOW-UPS FOR THE MANAGER (reported, not acted on)

1. ⚠️ **The board's `noticeRange` premise is stale** (§4 D): it is already in the schema at `0.0` on all 32 rows, *before* `TASK-987` compiles. `TASK-987`/`TASK-1001` should not be planned around "the column arrives at compile time" — it is already there and already correct. **No work owed.**
2. ⚠️ **The `+948 B` size delta** (§7) is explained by exact arithmetic but **not proven**, because proving it needs Git and this row forbids it. If the manager wants it closed by measurement, it needs a Git-permitted row.
3. ✅ **`Content/Data/DT_Cards.uasset` is now dirty in the working tree with the correct values** and is a **binary/LFS** file. It still has to reach a commit — this row is forbidden Git, so **the asset half is correct on disk but uncommitted**, and whichever row owns this batch's commit must stage it or the revert never ships.

---

**Gate:** ⭐ `TASK-1006` (qa-reviewer).
**Slack:** posted to 🔧 Build & Git (`C0BF0QZP3CN`, thread `1783116286.945249`) with the read-back values and the sha256 pair.
