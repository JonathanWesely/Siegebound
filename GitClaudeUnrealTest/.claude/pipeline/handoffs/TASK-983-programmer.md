# TASK-983 — [BS-2] the `BrightSun` card row — ⛔ **BLOCKED (half done, and the half that is done is the CSV)**

**Agent:** gameplay-programmer · **Date:** 2026-09-04 · **Gate:** `TASK-1012` (✅ confirmed to exist — TASKBOARD line 19489)

---

## 0. ⛔⛔ BOTTOM LINE FIRST

**The CSV half LANDED and is fully verified. The `DT_Cards` half CANNOT LAND TODAY and I did not fake it.**

The running editor **cannot store `ESpellEffect::FogClear`** — its loaded module binary predates `TASK-982`. `set_rows`
accepted the write, **returned success, and silently dropped the enum to `None`**. I caught it on read-back, reverted
the half-written row, and left `/Game/Data/DT_Cards` **byte-identical on disk**.

⇒ ⛔ **`FOG-§9.11a` IS NOT SATISFIED. This row must NOT be committed as-is.** See §5 for the exact unblock.

---

## 1. What I changed

| artefact | state | evidence |
|---|---|---|
| `Docs/Data/cards.csv` | ✅ **WRITTEN** — one appended line, 34th data row | `diff` vs pre-write backup = `34a35`, one added line, **nothing else moved** |
| `/Game/Data/DT_Cards` | ⛔ **NOT WRITTEN — deliberately reverted** | `sha256` **unchanged**: `abe66ed250f05e43…7566f2d0` before **and** after; on-disk `BrightSun` byte-probe = **ABSENT** |

**No C++ touched. No compile. No Git. No editor close.** The editor (PID 3172) is still up and MCP is still reachable.

### The row as it now stands in `cards.csv`

```
BrightSun,Bright Sun,Spell,60,2,0,0,0,0,0,None,Spell: clears battlefield-wide fog and prevents new fog for 120s plus 60s per 1524uu of hero height above the flat-grass datum; height is sampled once at cast and is uncapped; the shield expires to clear and never restores the fog; no reticle (GDD 4),0,false,false,false,false,0,0,0,None,0,0,/Game/UI/CardArt/T_CardArt_BrightSun.T_CardArt_BrightSun,FogClear,120,0,0,0,0,,
```

Field-by-field read-back (derived off **raw bytes**, not `nl` — `FOG-§9.8e`; field 1 header is empty ⇒ keyed on index 0):

| # | column | value | # | column | value |
|---|---|---|---|---|---|
| 1 | *(CardID)* | `BrightSun` | 17 | bSuicide | `false` |
| 2 | DisplayName | `Bright Sun` | 18 | SwarmCount | `0` |
| 3 | CardType | `Spell` | 19 | AoERadius | `0` |
| 4 | Cost | `60` | 20 | MinRange | `0` |
| 5 | MaxCopies | `2` | 21 | SpawnCardID | `None` |
| 6 | HP | `0` | 22 | SpawnInterval | `0` |
| 7 | Damage | `0` | 23 | Lifetime | `0` |
| 8 | Range | `0` | 24 | CardArt | `/Game/UI/CardArt/T_CardArt_BrightSun.T_CardArt_BrightSun` |
| 9 | Cadence | `0` | 25 | SpellEffect | `FogClear` |
| 10 | Speed | `0` | 26 | EffectDuration | `120` |
| 11 | Profile | `None` | 27 | MaxTargets | `0` |
| 12 | Notes | *(see row above)* | 28 | GoldSteal | `0` |
| 13 | DeckCount | `0` | 29 | ChainTargets | `0` |
| 14 | bRanged | `false` | 30 | ChainFalloff | `0` |
| 15 | bCharge | `false` | **31** | **SpellDelivery** | ⛔ **BLANK** |
| 16 | bSlayer | `false` | **32** | **NoticeRange** | ⛔ **BLANK** |

---

## 2. ⛔ THE BLOCKER — proven, not inferred

`set_rows` wrote 30 fields correctly and **dropped exactly one**:

```
mismatches: ["spellEffect: wrote 'FogClear' read 'None'"]
```

**Root cause, measured against the loaded binary:**

```
Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll   mtime 2026-09-04 03:32:54
Source/.../Siegebound/CardRow.h                        mtime 2026-09-04 18:49:46   (TASK-982)
Source/.../Siegebound/FogVolume.h                      mtime 2026-09-04 18:47:46   (TASK-982)

DLL contains [FogCover]                    : YES
DLL contains [ESpellEffect]                : YES
DLL contains [FogClear]                    : NO      ⛔
DLL contains [ApplyBrightSun]              : NO      ⛔
DLL contains [BrightSunBaseDurationSeconds]: NO      ⛔
```

The editor is running a build **~15 hours older** than `TASK-982`'s source. Its live reflection data holds
`ESpellEffect` values `0..6` only. The name `FogClear` does not resolve, so the byte-enum property fell back to
`None` (0) — **with no error, no log line, and a success return.**

⚠️ **This is `SC-§68` / the dispatch's own warning arriving through a side door**: had I trusted the `null` return
I would have shipped a `BrightSun` row that costs 60 gold and resolves as *no spell at all*, while `cards.csv`
and every human-readable inspection read `FogClear`. **That is precisely the defect `FOG-§9.11a` exists to stop.**

⛔ **I did NOT work around it.** Writing the raw byte `7` into an enum the running build does not have would be
inventing architecture and would plant an out-of-range value in a saved asset — exactly the corruption
`CardRow.h`'s append-only comment warns about. Declined per *"report and stop rather than inventing."*

---

## 3. Verification actually performed (the three trusted instruments only)

**`sha256` — never size, never count:**
- `DT_Cards.uasset` **BEFORE**: `abe66ed250f05e432238fc2e50ced85117ddc5e1b7851f7e24f573d87566f2d0`
- `DT_Cards.uasset` **AFTER** : `abe66ed250f05e432238fc2e50ced85117ddc5e1b7851f7e24f573d87566f2d0` ✅ **identical**
- `T_CardArt_BrightSun.uasset`: `15570b043fe0ab7fd1205b54d582a1fdbdf796187fce1ee85072edcd8d70a387` — ✅ **matches
  `TASK-984`'s declared `15570b04…` exactly.** `exists`→`true`, `load_asset` on the full object path resolves,
  `get_asset_class`→`Texture2D`. **The reference resolves; not assumed.**

**`get_rows` — quoted back, including ≥2 untouched controls (the both-ways control):**

| row | field | value | status |
|---|---|---|---|
| `Archer` | Range | `2100` | ✅ unchanged |
| `Wizard` | Range | `2100` | ✅ unchanged |
| `Longbowman` | Range / NoticeRange | `3600` / `0` | ✅ unchanged, notice cell still blank |
| `Fog` | Cost/SpellEffect/EffectDuration/CardArt | `50` / `FogCover` / `300` / `T_CardArt_Fog` | ✅ unchanged |
| `Pickpocket` | Cost/SpellEffect/GoldSteal | `18` / `GoldSteal` / `10` | ✅ unchanged |
| `Witch` | Cost/Range | `50` / `400` | ✅ unchanged |

**`is_dirty` transitions:** `DT_Cards` was **already `true` BEFORE I touched it** (someone else's unsaved in-memory
edit) and is `true` now. ⛔ **I never called `save_assets` — not with an explicit list, and certainly not with the
empty list.** Since nothing of mine survives in memory, there was nothing of mine to save.

⭐ **Pre-existing dirt audited before I wrote anything** (this mattered — an unsaved retired `noticeRange 3600`
would have been flushed by any save I performed): I dumped all 33 in-memory rows and diffed them against the CSV
across `cost / maxCopies / deckCount / range / noticeRange / spellEffect / spellDelivery / effectDuration` —
**IDENTICAL on all 33 rows.** The dirt is benign; `noticeRange` is `0` on every row in memory **and** blank on
every row in the CSV.

---

## 4. Declarations the spec demanded

### `sum(DeckCount)` — **stays `50`, deliberately**
`BrightSun.DeckCount = 0`, so the sum is **unchanged at `50`**, still equal to `SiegeLegalDeckSize`. Measured
after the append: `sum=50`. A new card is not auto-dealt into the starting deck; it is a deck-builder option.
*(Note for the record: no test actually asserts this sum — `grep DeckCount Tests/` returns zero hits — so it is
a runtime invariant consumed by `UDeckBuilderWidget` seed and `UDeckComponent::BuildAndShuffle`, not a gated one.)*

### `SpellDelivery` — **BLANK ⇒ `Auto`, decided deliberately**
`FOG-§10.1` and spec item (2) both pin **no reticle**, following the `Pickpocket` precedent. Measured facts:
`Auto` resolves **per-effect** in `USpellLibrary::GetEffectiveDelivery`, and only `AoEDamage`/`Freeze` (i.e.
exactly `Fireball`/`FrostNova`) become `HeroLine`; every other effect keeps the non-aimed path. `FogClear` is
neither ⇒ blank/`Auto` **is** the no-reticle answer. Confirmed by precedent: `Pickpocket` ("instant resolve no
reticle") and `Fog` both carry a **blank** cell and read back `Auto`. Writing `GroundCircle` or `HeroLine` would
have *introduced* a reticle for a map-wide card. **Column state after my row: still only `Fireball`/`FrostNova`
populated, both `HeroLine`; 32 of 34 blank.**

### `NoticeRange` — **BLANK on all 34, the trap NOT executed**
⛔ `handoffs/TASK-993-programmer.md` §9.6's boldfaced *"REQUIRED post-compile `set_rows noticeRange 3600`"* is
**SUPERSEDED and I did not execute it.** Verified end state: **column blank on all 34 CSV rows** (`awk` census:
zero non-blank) and **`0.0` on all 33 DataTable rows** (in-memory census: `nonzero_noticeRange: []`). The struct
default and the sparse column agree.

### The ×3 cost grid — ⛔ **spec item (3) IS REFUTED AT SOURCE. REPORTING, NOT FIXING.**
`BrightSun` `60 = 3 × 20` ⇒ ✅ **on the grid**, exactly as specified. **But the other half of the claim is false.**
`FOG-§10.1`, `TASK-983` item (3) and `TASK-840` item (1a) all assert *"`Fog`'s `50` remains the game's **ONLY**
off-grid cost — he broke his own grid **once, not twice**."* Item (3) told me to *confirm this at source*. I did.
Full census of all 34 costs, `cost % 3 != 0`:

```
Witch = 50   (off-grid)
Fog   = 50   (off-grid)
⇒ 32 on-grid of 34 total
```

⛔ **There are TWO off-grid costs, not one. `Witch` also costs `50`.** He broke the grid **twice**, both at the
same value. ⛔ I changed nothing — `Witch` is frozen and out of scope. **`FOG-§10.1`'s "the cost note" row and
`TASK-840` item (1a) are both factually wrong and should be corrected by the manager.**

---

## 5. ⛔ THE UNBLOCK — what has to happen, and by whom

1. **build-master compiles** (`TASK-987`'s ship host already owns a compile). The editor must be **CLOSED** for it.
2. **Editor restarts** ⇒ its reflection data then contains `ESpellEffect::FogClear` (== 7).
3. **Then** the `DT_Cards` half can be written — either by re-running this row's asset half via `set_rows`, or by
   a **reimport** of `cards.csv` (the CSV is already correct and is the reimport source of truth).
4. ⛔ **Re-verify by `get_rows` that `spellEffect` reads back `FogClear` and not `None`.** ⛔ A success return is
   not evidence — that is exactly how this row failed the first time.

⚠️ **UNTIL STEP 3 LANDS, `Docs/Data/cards.csv` CARRIES A ONE-SIDED CHANGE.** Per `FOG-§9.11a` that state must not
be committed: the CSV-only half is *invisible to the DataTable gates and absent from the shipped game* — the card
would be un-playable in-game while every gate reports green. **I left the CSV in the working tree (uncommitted,
alongside `TASK-840`'s existing uncommitted `Fog` row) rather than discarding verified work, but it is
build-master's call whether to hold it or revert it.**

---

## 6. Test-suite delta

**Zero.** No test flips PASS/FAIL and the total test count does not change (all 15 CSV/DT-reading files use
`IMPLEMENT_SIMPLE_AUTOMATION_TEST`; there are no complex/data-driven tests).

| gate | effect of the new CSV row |
|---|---|
| `SiegeUnitNoticeRangeTest` | ⛔ **THE STRICT ONE** — `TestEqual(RowsMisaligned, 0)` on `Fields.Num() != Header.Num()`. **PASSES: my row is exactly 32 fields.** ⚠️ This is why the Notes cell contains **zero commas** — a single comma would yield 33 fields and turn this gate **RED**. `PopulatedNoticeCells` stays `0`. |
| `SiegeAssistantSelectionTest` | `BuildDerivedRoster` hard-fails on a **short** row (`Fields.Num() < Header.Num()`). **PASSES** (32 == 32). `CardType=Spell` ⇒ not commandable ⇒ roster/kinds unchanged. `RowsRead` 33→34 (informational only). |
| `SiegeFogClampTest` | `AoERadius=0` ⇒ not above the 609.6 ceiling, does not beat `Lightning`'s 700. **No delta.** |
| `SiegeCardArtRosterTest` | Walks **every** row with **no Spell exemption**: CardArt must be set **and load as a `UTexture2D`**. **PASSES — `T_CardArt_BrightSun` exists and its path is unique** (+2 assertions, but **only after `DT_Cards` gets the row**). |
| `SiegeCardRosterTest` | ⭐ **Explicitly exempts `ECardType::Spell`** (`NotSpawnable` ⇒ `++ExcludedRows; continue`). **No `BP_Unit_BrightSun` or mesh is required.** +0 assertions. |
| `SiegeFogVolumeTest` | Independently pins **`ESpellEffect::FogClear == 7` and highest-declared** — corroborating my source check. Reads no CSV. **No delta.** |
| `SiegeBrightSunTest` | Source-grep + CDO only; **reads neither `cards.csv` nor `DT_Cards`** ⇒ my row is currently **ungated by it**. |

⚠️ **The gap QA should note:** the three CSV-reading tests see the row **immediately**; the two DataTable gates see
**nothing** until `DT_Cards` is written. **The suite is green in both states** — so a green suite is *not* evidence
that this task is done. That is the same blind spot `FOG-§9.11a` describes.

---

## 7. ⛔ What QA (`TASK-1012`) should scrutinize hardest

1. ⛔ **That the asset half is genuinely absent, not half-written.** `sha256` of `DT_Cards.uasset` must still be
   `abe66ed2…f2d0` and the file must contain **zero** `BrightSun` bytes. If either has changed, someone saved after me.
2. ⛔ **That `noticeRange` stayed blank on all 34 CSV rows** — `TASK-993` §9.6's retired `3600` instruction is still
   sitting in the tree and reads as authoritative.
3. ⛔ **The Notes cell must contain no comma** — it is load-bearing for `SiegeUnitNoticeRangeTest`'s hard equality.
4. ⚠️ **The `Witch = 50` finding (§4)** — a convention row (`FOG-§10.1`) currently states something false.
5. ⚠️ **`EffectDuration = 120` is INERT for this card**, exactly as `Fog`'s `300` is inert (`CardRow.h`'s own
   comment says so and `TASK-1016` is the row that makes the cell authoritative). Verified at source:
   `AFogVolume::ApplyBrightSun(ETeamId CasterTeam)` takes **no duration argument** and the window is computed from
   the CDO's `BrightSunBaseDurationSeconds = 120.f` + `BrightSunBonusSecondsPerStep = 60.f` ×
   `floor(H / BrightSunHeightStepUU = 1524.f)`. The cell **matches** the CDO (`120` == `120.f`), so it is correct
   and consistent — but it is **not what drives behaviour today.** Out of scope to fix here.
6. ⛔ **`DT_Cards` was already dirty before this task started** and still is. That unsaved edit is **not mine** and
   remains unaudited by any gate; I verified it is benign against all 33 rows but I could not attribute it.

---

## 8. Files touched / referenced

**Written (1):** `Docs/Data/cards.csv` — one appended line.
**Written (this note):** `.claude/pipeline/handoffs/TASK-983-programmer.md`
**Board:** `TASK-983` `status:` line only.
**Reverted to pre-task state (0 net change):** `/Game/Data/DT_Cards` (in-memory row added then removed; disk never written).
**Referenced, unmodified:** `/Game/UI/CardArt/T_CardArt_BrightSun.T_CardArt_BrightSun` · `Source/.../CardRow.h` ·
`Source/.../FogVolume.h` · `Source/.../SpellLibrary.cpp` (read-only verification of the `FogClear` dispatch at line 708).

---

# ⭐ §9 — APPENDED 2026-09-04 (second pass): **THE ASSET HALF LANDED. `FOG-§9.11a` IS SATISFIED.**

> ⛔ **§0–§8 above are PRESERVED VERBATIM.** They are the blocked-state record and they are evidence — the enum
> drop they document is exactly what `TASK-1020` was raised to discharge. Nothing above has been edited.

## 9.0 Bottom line

**Both halves now exist and they agree cell-for-cell.** `spellEffect` reads back **`FogClear`**, not `None`.
`/Game/Data/DT_Cards` holds **34 rows**, saved to disk, hash moved. The commit is unblocked.

## 9.1 ⛔ The blocker was RE-MEASURED, not assumed

I did not take the dispatch's word that the enum was fixed. `get_schema` on `/Game/Data/DT_Cards`, run by me:

```
"spellEffect": { "title": "ESpellEffect",
  "enum": ["None","AoEDamage","Freeze","TopTargetsDamage","AllyBuff","GoldSteal","FogCover","FogClear"] }
```

**EIGHT values, `0..7`, ending `FogClear`.** It held `0..6` on the first pass — that is precisely what swallowed
the write. `noticeRange` is likewise present in the row schema.

## 9.2 ⛔ The row was ABSENT — `add_rows` before `set_rows`

`list_rows` returned **33** names with **no `BrightSun`**, confirming the re-dispatch's correction at source. The
row I created on the first pass lived only in the killed editor's unsaved memory and never reached disk — which is
exactly what my byte-identical hash proved, and why the revert-and-never-save was correct. So:
`add_rows(["BrightSun"])` → 34 rows → `set_rows` for the values. A bare `set_rows` would have failed on the
**missing row**, and reading that failure as "the enum still isn't there" would have misdiagnosed a solved problem.

## 9.3 ⛔⛔ A SECOND SILENT DROP — caught by the same read-back that caught the first

The first `set_rows` returned success (`null`) and **dropped one field**:

```
MISMATCHES: ["cardArt: wrote '/Game/UI/CardArt/T_CardArt_BrightSun.T_CardArt_BrightSun' read 'None'"]
```

**Root cause, measured — not guessed.** I read the `cardArt` cell of four known-good rows:

| row | `cardArt` read-back | python type |
|---|---|---|
| `Archer` | `/Game/UI/CardArt/T_CardArt_Archer.T_CardArt_Archer` | **`str`** |
| `Fireball` | `/Game/UI/CardArt/T_CardArt_Fireball.T_CardArt_Fireball` | **`str`** |
| `Pickpocket` | `/Game/UI/CardArt/T_CardArt_Pickpocket.T_CardArt_Pickpocket` | **`str`** |
| `Fog` | `/Game/UI/CardArt/T_CardArt_Fog.T_CardArt_Fog` | **`str`** |

The column is a **plain string** path in this tool's marshalling. I had passed the schema's own documented object
form `{"refPath": "..."}` — which `set_rows` accepted and silently coerced to `None`. Passing the bare string fixed
it; the re-read confirms the full object path. The texture itself was verified independently: `exists` → `true` on
both the short and the full path, `get_asset_class` → `Texture2D`.

⚠️⛔ **THIS IS THE SAME DEFECT SHAPE AS THE ENUM DROP, FROM A DIFFERENT CAUSE — and it is the one QA should weigh
most.** A `null` success return is **not evidence**, twice over now. Had it shipped, the row would carry
`cardArt = None`, and **`SiegeCardArtRosterTest` walks every row with NO Spell exemption** and requires the cell to
resolve as a `UTexture2D` ⇒ **that gate would have gone RED at the next run**, on a task everyone had already
called done.

## 9.4 ⛔ The read-back — every field quoted

```json
{"displayName":"Bright Sun","cardType":"Spell","cost":60,"maxCopies":2,"hP":0,"damage":0,"range":0,
 "cadence":0,"speed":0,"profile":"None",
 "notes":"Spell: clears battlefield-wide fog and prevents new fog for 120s plus 60s per 1524uu of hero height
          above the flat-grass datum; height is sampled once at cast and is uncapped; the shield expires to
          clear and never restores the fog; no reticle (GDD 4)",
 "deckCount":0,"bRanged":false,"bCharge":false,"bSlayer":false,"bSuicide":false,"swarmCount":0,
 "aoERadius":0,"minRange":0,"noticeRange":0,"spawnCardId":"None","spawnInterval":0,"lifetime":0,
 "cardArt":"/Game/UI/CardArt/T_CardArt_BrightSun.T_CardArt_BrightSun",
 "spellEffect":"FogClear","spellDelivery":"Auto","effectDuration":120,
 "maxTargets":0,"goldSteal":0,"chainTargets":0,"chainFalloff":0}
```

⭐⭐ **`"spellEffect": "FogClear"` — the entire reason this row exists.**

## 9.5 ⛔ `FOG-§9.11a` — CSV vs asset, machine-diffed on all 31 columns

I did **not** eyeball this. I dumped the asset read-back to disk and ran a `csv`-module comparison against
`Docs/Data/cards.csv`, normalising only the two documented sparse conventions (blank cell ⇒ `0`; blank
`SpellDelivery` ⇒ `Auto`). The `Notes` comparison ran on the **full** strings, so my transcription is verified
byte-exact rather than trusted:

```
columns compared : 31
MISMATCHES       : 0
RESULT           : CSV and DT_Cards AGREE cell-for-cell
```

⇒ ⭐ **`FOG-§9.11a` satisfied. The two-halves law is met and the commit is unblocked.**

## 9.6 ⛔ Verification by the three trusted instruments ONLY

**`sha256`, measured OUTSIDE the engine — never size, never count:**

| when | sha256 |
|---|---|
| **BEFORE** | `abe66ed250f05e432238fc2e50ced85117ddc5e1b7851f7e24f573d87566f2d0` |
| **AFTER** | `aa2c5bc18322343f1caaaa4ad453aacc0bde6bd8a3b0f41f1f4365d2d9dedf5b` |

The BEFORE hash is **identical to the one §3 recorded on the first pass**, independently confirming that nobody
wrote the asset between the two passes and that my earlier revert really did leave zero trace.

**`is_dirty` transitions:** `false` (pre-write) → `true` (post-write) → **`false`** (post-save).
⭐ **The asset was CLEAN before I touched it** — unlike the first pass, where it was already dirty with someone
else's unsaved edit. So this save carried **only my change**; there was no foreign dirt to flush.
Saved with an **explicit one-asset list** `["/Game/Data/DT_Cards"]` — never the empty list.

**Byte probe of the saved package** (true byte-occurrence counts, *not* `rg --count`, which counts matching
**lines** on a binary and has already lied about this file):

```
BrightSun            utf8=3     T_CardArt_BrightSun  utf8=2
Bright Sun           utf8=1     FogClear             utf8=1
```

`3 = 1 row name + 2 inside the two art-path occurrences` — internally consistent.

## 9.7 ⛔ The control, run BOTH ways

Every one of the **33 pre-existing rows** was snapshotted before the write and byte-compared (`json.dumps`,
`sort_keys=True`) after it:

```
control_rows_compared    : 33
DRIFT_on_untouched_rows  : []      <- zero
```

Quoted back explicitly, post-save:

| row | cost | spellEffect | effectDuration | range | noticeRange | verdict |
|---|---|---|---|---|---|---|
| `Fog` | `50` | `FogCover` | `300` | `0` | `0` | ✅ unchanged |
| `Pickpocket` | `18` | `GoldSteal` | `0` | `0` | `0` | ✅ unchanged |
| `Witch` | `50` | `None` | `0` | `400` | `0` | ✅ unchanged |
| `Archer` | `12` | `None` | `0` | `2100` | `0` | ✅ unchanged |

## 9.8 Declarations re-confirmed on the ASSET (the first pass measured the CSV only)

- ⛔ **`noticeRange` = `0` on ALL 34 rows** (`nonzero_noticeRange_rows: []`). `TASK-993` §9.6's `3600` instruction
  is **superseded and was NOT executed** — executing it would now be a defect.
- ⛔ **`sum(deckCount)` = `50`**, unchanged; the deck stays legal. `BrightSun.deckCount = 0`.
- ⛔ **`spellDelivery` = `Auto`** (the `add_rows` default; I never wrote the cell). The column is still populated on
  exactly `Fireball`/`FrostNova`, both `HeroLine` ⇒ no reticle introduced, per the `Pickpocket` precedent.
- ⚠️ **The `Witch = 50` finding now REPRODUCES ON THE ASSET, not just the CSV:** `offgrid_costs (cost % 3 != 0)`
  over all 34 rows = **`{"Witch": 50, "Fog": 50}`**. ⛔ **TWO off-grid costs, not one.** `FOG-§10.1`, this task's
  item (3) and `TASK-840` item (1a) all state *"`Fog`'s 50 is the game's ONLY off-grid cost"* — **that is false in
  both artefacts.** ⛔ Reported, **not fixed**; `Witch` is frozen and out of scope. Manager correction owed.

## 9.9 Scope discipline

- ⛔ `Docs/Data/cards.csv` — **NOT touched this pass** (read-only for me).
  `sha256 830a85417f773023a2bc0b6119f7c7a0cd042d862d8af1024bde18ab40b2e3e4`, 35 lines, unchanged.
- ⛔ **No `Source/` file, no compile, no Git command of any kind.**
- ⛔ Board: **only** `TASK-983`'s own `status:` line. I re-located it by heading first — it had shifted from
  18395 → **18397** since the earlier pass, so that warning was live.
- ⛔ **Editor left UP**, PID `24284`, MCP reachable.

## 9.10 The editor hazard — diagnosed, not assumed

The `Restore Packages` modal did **not** appear. I enumerated every top-level window of PID `24284` via
`EnumWindows` rather than inferring anything from the port:

```
visible=True  enabled=True  title='GitClaudeUnrealTest - Unreal Editor'
visible=False enabled=False title='MSCTFIME UI'
visible=False enabled=False title='Default IME'
```

⭐ The main window is **`enabled=True`** — a blocking modal would have left it `enabled=False`. No prompt was
accepted, so no discarded unsaved state was restored and the never-save law holds.

## 9.11 ⛔ What QA (`TASK-1012`) should scrutinize hardest

1. ⛔⛔ **The `cardArt` object-vs-string marshalling defect (§9.3).** This is the live lesson: the DataTable
   `get_schema` **documents `cardArt` as an object** with a `refPath` property, and passing exactly that
   documented shape **silently produces `None`**. Any future agent writing a `UObject`-typed DataTable cell
   through MCP will hit this. It arguably belongs in CONVENTIONS.
2. ⛔ **Re-verify `spellEffect` independently** — `get_rows` on `BrightSun` must read `FogClear`. If it reads
   `None`, the asset was written by a stale build again.
3. ⛔ **`sha256` must be `aa2c5bc1…df5b`.** If it differs, someone saved after me.
4. ⛔ **`noticeRange` blank/0 on all 34** — `TASK-993` §9.6's retired `3600` instruction is still sitting in the
   tree and still reads as authoritative.
5. ⛔ **The Notes cell must contain no comma** — still load-bearing for `SiegeUnitNoticeRangeTest`'s hard
   `Fields.Num() == Header.Num()` equality (32 fields).
6. ⚠️ **`FOG-§10.1`'s off-grid claim is false (§9.8)** — now measured in both artefacts.
7. ⚠️ **`EffectDuration = 120` remains INERT for this card**, exactly as §7(5) recorded: `AFogVolume::ApplyBrightSun`
   takes no duration argument and reads the CDO. The cell *matches* the CDO (`120` == `120.f`) so it is correct and
   consistent, but it is still not what drives behaviour today. `TASK-1016` is the row that makes it authoritative.

## 9.12 Test-suite delta

**Zero flips, zero count change** — same as §6. The two DataTable-reading gates that previously saw *nothing* now
see the row: `SiegeCardArtRosterTest` gains its `+2` assertions and **passes** (the art resolves as a `Texture2D`,
path unique), and `SiegeCardRosterTest` still exempts `ECardType::Spell`, so no `BP_Unit_BrightSun` or mesh is
required. ⭐ **The §6 blind spot is now closed rather than merely documented:** the suite was green in both the
one-sided and the two-sided state, so greenness never evidenced this task — the `sha256` + `get_rows` +
cell-for-cell diff do.

## 9.13 Files touched this pass

**Written (1 asset):** `/Game/Data/DT_Cards` — one added row (`BrightSun`), 34 total, saved.
**Appended (this note):** `.claude/pipeline/handoffs/TASK-983-programmer.md` §9 only.
**Board:** `TASK-983` `status:` line only.
**Read-only, unmodified:** `Docs/Data/cards.csv` · `/Game/UI/CardArt/T_CardArt_BrightSun` · all 33 other rows.
