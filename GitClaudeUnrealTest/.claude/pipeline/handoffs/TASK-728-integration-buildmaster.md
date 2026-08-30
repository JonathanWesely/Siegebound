# TASK-728 / TASK-731 — ART INTEGRATION: the `DT_Cards` reimport, verified by read-back

**Agent:** build-master
**Date:** 2026-08-30
**Closes:** TASK-731 acceptance lines **(2)** and **(4)**, which the `20db517` pass explicitly fenced out and declared STILL OWED.
**Outcome:** ✅ **The Watch Tower is now summonable and the ×3 ranges are live in the DataTable.** The divergence `20db517` declared is hereby **closed**.

---

## 1. Git measured FIRST (before any change)

| Probe | Measured | Expected | Verdict |
|---|---|---|---|
| `git rev-parse HEAD` | `20db51768b078e028d7a1158efed8966cae64524` | `20db517` | ✅ match |
| `git rev-list --left-right --count origin/main...HEAD` | `0  3` | 3 ahead / 0 behind | ✅ match |
| Working tree | 2 staged `A`, 5 `M`, 7 `??` | — | as handed over |

⛔ **No self-commit of Jonathan's appeared between the batch and this pass** — nothing to duplicate, nothing to amend.

---

## 2. ⭐ The reimport — and why it was NOT done with `import_file` alone

**The blocker:** `DataTableTools.import_file` **refuses to overwrite an existing asset**:

```
error: import_asset: DT_Cards at /Game/Data already exists
```

**The trap I declined:** delete-then-import at the same path. `get_referencers(/Game/Data/DT_Cards)` returns
**`/Game/UI/WBP_CardHand`** and **`/Game/UI/WBP_HUD`** — deleting the asset with the editor live would null those
two bindings. ⛔ **Not done.**

**The route taken — factory import as ground truth, then reconcile:**

1. Imported `Docs/Data/cards.csv` through the **real DataTable factory** to a scratch path
   `/Game/Data/DT_Cards_ReimportTmp` with schema `/Script/GitClaudeUnrealTest.CardRow`.
   ⭐ This is a genuine, factory-produced import — **not my interpretation of the CSV**.
2. **Diffed the live `DT_Cards` against that factory import, every row × every column.**
3. Applied **only** the diff, with values **copied verbatim out of the factory import**.
4. **Re-diffed to zero**, saved, deleted the scratch asset.

⚖️ **This is not a hand-edit.** No value was typed by me; every value written came out of the factory's own
import of the CSV, and the end state was proven equal to that import. The asset's identity — and therefore both
widget references — survived intact.

### The diff, before it was applied

This is the load-bearing measurement of the whole pass. The **entire** delta between the shipped DataTable and a
factory import of `cards.csv` was:

```json
{"tmp_count": 31, "real_count": 30,
 "missing_in_real": ["WatchTower"], "extra_in_real": [],
 "diff_row_count": 3,
 "diffs": {"Archer":     {"range": {"real": 700,  "csv": 2100}},
           "Longbowman": {"range": {"real": 1200, "csv": 3600}},
           "Wizard":     {"range": {"real": 700,  "csv": 2100}}}}
```

⭐ **Four changes, and nothing else in the table moved.** That is itself the proof the ×3 did not spill: had it
leaked into `AoERadius`, the tower rows or `Cleric`, those columns would appear in this diff. They do not.
`700 × 3 = 2100` and `1200 × 3 = 3600` — the arithmetic is exact.

### After applying

```json
{"real_count": 31, "tmp_count": 31, "missing_in_real": [], "extra_in_real": [],
 "residual_diff_row_count": 0, "residual_diffs": {}, "deckcount_sum": 50}
```

⭐ **Zero residual difference across 31 rows × 30 columns.** The live asset is now semantically identical to a
factory import of its own source CSV.

---

## 3. ⛔ READ-BACK TABLE — measured from the live asset, ⛔ never from a call returning success

Every number below was read back out of `/Game/Data/DT_Cards` **after** the write and **after** the save.

| Row | Type | Cost | **Range** | AoERadius | DeckCount | Verdict |
|---|---|---|---|---|---|---|
| **Archer** | Unit | 12 | **2100** | 0 | 8 | ✅ ×3 landed (was 700) |
| **Wizard** | Unit | 24 | **2100** | **250** | 0 | ✅ ×3 landed (was 700); ⭐ **AoERadius UNCHANGED at 250 — the ×3 did not spill** |
| **Longbowman** | Unit | 18 | **3600** | 0 | 2 | ✅ ×3 landed (was 1200) |
| **Cleric** | Unit | 18 | **400** | 0 | 2 | ✅ **UNCHANGED** — heal radius, not an attack range |
| **ArrowTower** | Building | 15 | **900** | 0 | 3 | ✅ **UNCHANGED** (`R-2`: towers are Buildings) |
| **BombTower** | Building | 24 | **800** | 250 | 0 | ✅ **UNCHANGED** |
| **BallistaTower** | Building | 21 | **1400** | 0 | 0 | ✅ **UNCHANGED** |
| **CrystalTower** | Building | 27 | **800** | 0 | 0 | ✅ **UNCHANGED** (4th ranged building — not named in the spec, checked anyway) |
| **WatchTower** | **Building** | **30** | 0 | 0 | 0 | ✅ **ROW PRESENT** |

**`sum(DeckCount)` across all 31 rows = `50`** ✅ **exact.**

### The `WatchTower` row in full, as read back

```
displayName "Watch Tower" · cardType Building · cost 30 · maxCopies 3 · hP 250 · damage 0
range 0 · cadence 0 · speed 0 · bRanged false · deckCount 0 · aoERadius 0
cardArt "/Game/UI/CardArt/T_CardArt_WatchTower.T_CardArt_WatchTower"
```

`Cost 30` · `HP 250` · `bRanged false` — the three values TASK-731 spec §(2) named, all confirmed.

### CardArt resolves to a real asset

- `cardArt` string: `/Game/UI/CardArt/T_CardArt_WatchTower.T_CardArt_WatchTower`
- `exists("/Game/UI/CardArt/T_CardArt_WatchTower")` → **`true`**
- `get_asset_class(...)` → **`Texture2D`**

⛔ **Not a dangling path.** The string resolves to a loaded texture of the right class.

### ⚠️ Texture properties re-verified — the third check of a twice-defective lane

This task has already produced **two** silent texture defects where the import returned success and the property
was wrong (the ORM landed `SRGB=true`; the card art landed `TEXTUREGROUP_World`). I did **not** take the
correction on trust. Read back, and compared against the established family reference `T_CardArt_Archer`:

| Property | `T_CardArt_WatchTower` | `T_CardArt_Archer` (reference) | Verdict |
|---|---|---|---|
| `LODGroup` | `TEXTUREGROUP_UI` | `TEXTUREGROUP_UI` | ✅ match — the `World` defect is genuinely fixed |
| `SRGB` | `true` | `true` | ✅ match |
| `CompressionSettings` | `TC_Default` | `TC_Default` | ✅ match |
| `Filter` | `TF_Default` | `TF_Default` | ✅ match |
| `NeverStream` | `false` | `false` | ✅ match |

---

## 4. Blueprint integration check (TASK-731 line (4)) — re-verified independently

| Probe | Measured | Verdict |
|---|---|---|
| `search_subclasses(ClimbableTower, "WatchTower")` | `/Game/Blueprints/Buildings/BP_Building_WatchTower.BP_Building_WatchTower_C` | ✅ ⭐ resolving from the **`ClimbableTower`** root proves the parent binding is real, not a nominal label |
| `PlatformHeightUU` (CDO) | **`1200`** | ✅ |
| `CardID` (CDO) | **`"WatchTower"`** | ✅ byte-identical to the `DT_Cards` row name |
| `get_dependencies(BP_Building_WatchTower)` | `/Script/GitClaudeUnrealTest` · `/Game/Materials/Instances/MI_TeamColor_Blue` · **`/Game/Meshes/SM_WatchTower`** | ✅ mesh bound |

⭐ **The summon chain is now closed end to end:** `cards.csv` → `DT_Cards.WatchTower` (Cost 30, Building) →
`CardID "WatchTower"` → `BP_Building_WatchTower_C` → parent `AClimbableTower` → mesh `SM_WatchTower`.
**That is what makes the card summonable**, and it was inert before this pass.

---

## 5. Asset state on disk

| Probe | Value |
|---|---|
| `DT_Cards.uasset` sha256 **before** | `938b59befe0bd0022526129727ff0f6c6b91924ec5e8db3086391a68f561ec97` |
| `DT_Cards.uasset` sha256 **after** | `eedc07e4ed9356ef857ea6ad4804e7a31d3533502d9dc3aa48c0339030455807` |
| Size | 40,089 → **41,378** bytes |
| `is_dirty` before save / after save | `true` → **`false`** |
| Scratch asset `DT_Cards_ReimportTmp` | **deleted**; `exists` → `false`; ⛔ **absent from disk and from the commit** |
| `get_referencers(DT_Cards)` after | `/Game/UI/WBP_CardHand`, `/Game/UI/WBP_HUD` — ⭐ **both survived** |

⚠️ **Only `/Game/Data/DT_Cards` was saved** — an explicit single-asset save, ⛔ never a save-all.
⛔ **No `.umap` was touched; `L_Arena` was not saved.** ⛔ Editor left **UP, PID 5584**, untouched.

⚠️ **No missing-column notice appeared on import** (TASK-731 §(2) asked me to watch for one). The `SpellDelivery`
column is empty for 3 spell rows in the CSV and resolves to the enum default `Auto`; `Fireball` and `FrostNova`
carry `HeroLine` and **read back as `HeroLine`** — checked precisely because an empty enum cell is a silent-default
risk.

---

## 6. LFS verification — oid vs worktree sha256, ⛔ never by size

Both binaries in the cargo verified **three ways**: worktree sha256 vs the **staged index pointer** (the check that
catches a *stale* pointer, not merely a missing one), and `filter: lfs` confirmed on each.

| File | staged pointer oid == worktree sha256 | size | `filter` | Verdict |
|---|---|---|---|---|
| `Content/Data/DT_Cards.uasset` | `eedc07e4ed9356ef857ea6ad4804e7a31d3533502d9dc3aa48c0339030455807` | 41,378 | `lfs` | ✅ current, ⛔ not stale |
| `Content/Blueprints/Buildings/BP_Building_WatchTower.uasset` | `4262b2a96a8ecbc23777f72476da599af922fcbe1a08623a6d550f7929d9d571` | 37,876 | `lfs` | ✅ current, ⛔ not stale |
| `Content/UI/CardArt/T_CardArt_WatchTower.uasset` | `fdbd503a8980c0d30da9808c5b2a6e2ef99808288d9b878aa90011712ef29fc8` | 320,159 | `lfs` | ✅ current, ⛔ not stale |
| `Content/RawAssets/CardArt/WatchTower.png` | `695e18c513d1b8fa4cd196c10cdf2d485586734cfebeca43d16b636fd06093f3` | 307,689 | `lfs` | ✅ current, ⛔ not stale |

⭐ For each file the **staged index pointer's oid was compared against a fresh `sha256sum` of the worktree bytes** —
the check that catches a *stale* pointer (a pointer that exists and looks valid but describes an older revision of
the file). All four match exactly. ⛔ **Size was never used as the test**; it is recorded only as corroboration.

---

## 7. The commit

**ONE commit, explicit pathspecs only.** ⛔ `git add -A` was never used. ⛔ **No push.**

**Cargo:** `Content/Data/DT_Cards.uasset` · `Content/Blueprints/Buildings/BP_Building_WatchTower.uasset` ·
`Content/UI/CardArt/T_CardArt_WatchTower.uasset` · `Content/RawAssets/CardArt/WatchTower.png` ·
`.claude/pipeline/TASKBOARD.md` · `.claude/pipeline/handoffs/TASK-728-artist.md` · this handoff.

**Held back deliberately (standing exclusions, all still dirty and still ownerless):**

- `Docs/setupdirections.md` — unrelated lane, ownerless
- `Config/DefaultEngine.ini`, `Config/DefaultGame.ini` — TASK-698/713's lane
- `handoffs/TASK-698-programmer.md`, `TASK-699-buildmaster.md`, `TASK-699/`, `TASK-713-programmer.md`,
  `TASK-715-buildmaster.md`, `TASK-716-buildmaster.md` — ⛔ other lanes' records (TASK-716/700 explicitly fenced).
  ⚠️ **These six are uncommitted pipeline records with no owner in this batch** — they need a lane, or they will
  keep riding along as permanent dirt.

⛔ **No compile** (`20db517` already built this code; nothing here changes C++). ⛔ No cook, ⛔ no zip.
⛔ `Tools/Packaging/` untouched. ⛔ **No bulk reimport sweep** — `reimport_meshes.py::_category_of()` defaults an
unknown `CardID` to `unit` and would strip `SM_WatchTower`'s 14 hand-authored hulls while reporting DONE.

---

## 8. ⚠️ OPEN TUNING ITEM for Jonathan — ⛔ NOT a blocker, ⛔ not fixed here

**The ascent gate is centred on the pivot, but the mesh is not.**

`ClimbableTower.cpp:106-107` centres the gate box on the actor pivot at **x ±1500**, while `SM_WatchTower` spans
**x −600 → +2458.46**. The ramp's outer **~958 uu** therefore sits **outside** the gate: an enemy first meets the
blocker where the deck is already at **≈553 uu**, not the **300 uu** the header predicts — roughly **×1.36** height
bonus gained before being turned back.

✅ **`T-3` still holds.** The platform stays unreachable and the gate **fails open** — ⛔ **no stuck unit.**

⚖️ **Already known and boarded.** QA logged this as **W-3** and ruled it a follow-on; the artist re-found it
independently. Both `AscentGateHalfExtentXY` and the gate offset are `EditAnywhere` ⇒ **this is tuning, and it is
Jonathan's call at the sitting** — ⛔ deliberately not touched here.

**Also still standing from TASK-728:** the card art is a true render at family spec, but the camera sits on the
ramp's blind side so the walkable deck is never visible. **Re-render recommended, ⛔ not a blocker.**

---

## 9. Deviations declared

1. ⭐ **The reimport was performed as factory-import-to-scratch + verified reconcile, not as an in-place
   `Reimport`.** Cause: MCP exposes no reimport verb, `import_file` refuses to overwrite, the script sandbox
   allows no `unreal` module, and delete-then-import would have broken two live widget references. The end state
   was **proven equal to a factory import** (zero residual diff over 31×30) and asset identity was preserved.
2. `CrystalTower` was verified as an unchanged ranged building though the spec named only three — there are in
   fact **four** ranged Buildings, and leaving one unchecked would have left a gap in the no-spill claim.
