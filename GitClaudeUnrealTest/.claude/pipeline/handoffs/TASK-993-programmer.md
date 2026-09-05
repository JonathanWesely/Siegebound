# TASK-993 — [LONGBOW-DATA] the Longbowman's notice cell + `J-F29`'s two `Range` cells

**Agent:** gameplay-programmer · **Date:** 2026-09-04 · **Status:** `ready-for-qa` · **Gate:** `TASK-995` · **Ship host:** `TASK-987`
**Law:** `FOG-§9.9` · `FOG-§9.10` · GDD §3.0 · `SC-§45` · `FOG-§9.8e` · `SC-§38` · `SC-§38a` · `SC-§39` · `SC-§60` · `SC-§62` · `SC-§65` · `TL-§5c`

---

## 0. TOOLS, DECLARED ABOVE THE FINDINGS (`SC-§39`)

| Instrument | Used for | Controlled how |
|---|---|---|
| `csv.DictReader` keyed on `list(row.keys())[0]` | every cell value quoted below | positive control both ways — `Footman`/`Range` resolves to `120`; `ZzNoSuchCardZz` absent |
| cell-for-cell CSV differ vs `git show HEAD:./Docs/Data/cards.csv` | "which CELLS moved" | a line diff **cannot** answer this — every row grew a field, so `git diff` shows 33/33 changed |
| raw byte-substring scan of `.uasset` | BP CDO overrides, `DT_Cards` column presence | discriminates between assets — see §3 link 6 |
| `Grep` → `Read` | every character-exact claim | `SC-§38a`: `Grep` located, `Read` confirmed |

⛔ **Nothing was executed.** No compile, no editor, no engine, no Git (`QUIET-MODULE`; `TASK-987` owns the wave's one compile).

---

## 1. THE CELL I SET, AND WHERE

**File:** `Docs/Data/cards.csv` — the **only** file this row touched.

Column **`NoticeRange`** appended as the **last** header (index 31), following this file's own shipped precedent: `CardArt` and `SpellDelivery` were both appended at the END, and the DataTable CSV importer maps by header **name**, not order. The name was taken from `FCardRow::NoticeRange` (`CardRow.h`) — `TASK-979`'s field — and **not invented here**.

### Item (1) — the shipped value CONFIRMED AT THE ROW before changing anything

The board warned that `3600` was known only from two doc/test sites, never from the data. Machine-read from the column itself, pre-edit:

```
Longbowman   Range=3600   MinRange=0   AoERadius=0   bRanged=true   DeckCount=2
Archer       Range=2100   MinRange=0
Wizard       Range=2100   MinRange=0   AoERadius=250
```

⚠️ `FOG-§9.8e` honoured: `cards.csv`'s first column header is **empty**, so a `DictReader` keyed on `'CardID'` returns `None` for every row while looking perfectly well-formed. Demonstrated rather than asserted — the census prints `rows[0].get('CardID') -> None` beside the correct read.

### The complete diff, machine-derived cell-for-cell

```
header added  : ['NoticeRange']      header removed: []
rows added    : []                   rows removed  : []

  MOVED  Archer    Range  '2100' -> '2000'
  MOVED  Wizard    Range  '2100' -> '2000'

pre-existing cells moved: 2   (every other cell bit-identical)
NoticeRange AUTHORED : [('Longbowman', '3600')]
NoticeRange BLANK    : 31 rows
hand-typed 2000 in any NoticeRange cell? : False
```

- ✅ **`Longbowman` `NoticeRange` ⇒ `3600`** — his ruling, the one authored cell.
- ✅ **`Longbowman` `Range` UNTOUCHED at `3600`** — verified as part of "all pre-existing cells unchanged: True", not by eyeball. The firing side is a **zero diff**.
- ✅ **`Archer` + `Wizard` `Range` `2100 → 2000`** (item 3a / `J-F29`) — the cells were **NOT held**. Their `NoticeRange` cells are **BLANK** (`TASK-995` 4a-ii).
- ✅ Every other row carries the **sparse sentinel** (blank). **Zero** hand-typed `2000` anywhere in the column.
- ✅ Non-regression spot-checks all `True`: `Longbowman`, `ArrowTower`, `BombTower`, `BallistaTower`, `CrystalTower`, `WatchTower`, `Cleric`, and every melee row.
- ✅ `Wizard` `AoERadius` still `250` · `sum(DeckCount)` still **50** · CRLF ×33 preserved · trailing newline preserved · no quote character introduced · all 33 lines uniformly **32** fields.

### Item (5) — REPORT, don't fix

**`Longbowman` `MinRange` is `0`** (machine-read, pre- and post-edit). No inner blind spot, so the 3600 reach is a full disc. The board's concern — "a 3600 reach with a blind spot would be a different card" — does not apply.

---

## 2. ⚠️ THE MINIMUM CORRECT DIFF IS 33 LINES, NOT ONE — AND THE LITERAL READING GOES RED

Both the board ("EXACTLY ONE CELL, ONE ROW") and the dispatch ("This row sets that one cell") describe this as a one-cell change. **It cannot be.**

`Tests/SiegeAssistantSelectionTest.cpp`'s `BuildDerivedRoster` treats a short row as a hard failure, by design:

> `if (Fields.Num() < Header.Num())` → `"line %d has %d field(s) against a %d-field header — the row is truncated and a commandable kind could be lost silently."`

Adding `,NoticeRange` to the header and `,3600` to the Longbowman **only** — the literal reading — leaves 31 rows at 31 fields against a 32-field header and turns that test **RED for all 31 of them**. So every one of the 32 data rows had to receive its trailing sentinel field. This is also the only way to satisfy `TASK-995` item (4)'s "EVERY OTHER ROW's new column at the DEFAULT SENTINEL".

⇒ **Not a fence breach:** all 33 lines are inside `Docs/Data/cards.csv`, the file this row owns. Recorded because a reviewer diffing 33 changed lines against a spec that says "one cell" should know why before raising it.

---

## 3. PROOF THE LONGBOWMAN **RESOLVES** TO 3600 — NOT MERELY STORES IT

The dispatch is right that this is the property that matters: *a cell that is read and then clamped looks identical in the data.* Six links, each independently checkable.

1. **The cell reads 3600** — §1, machine-read, positive-controlled both ways.
2. **Header name → struct field.** The importer maps by header **name** (`CardRow.h`'s CardArt note; the `SpellDelivery` precedent). `NoticeRange` ⇒ `FCardRow::NoticeRange`, `UPROPERTY … float NoticeRange = 0.0f`.
3. **The field reaches the member ONLY through the resolver.** `Tests/SiegeUnitNoticeRangeTest.cpp` test 4(a), over the extracted body of `ASummonedUnit::LoadStatsAndStart`: `Row->NoticeRange` ×1, `ResolveNoticeRadiusUU(` ×1, `GetClassDefaultEngagementRadiusUU()` ×1. A direct `AggroRadius = Row->NoticeRange` would fail all three.
4. **The resolver returns 3600 — by EXECUTION, not inspection.** Test 2(a) calls the real static seam: `ResolveNoticeRadiusUU(DefaultUU, 3600.f) == 3600.f` exactly, **and** the row that actually matters — `ResolveNoticeRadiusUU(DefaultUU, 3600.f) > DefaultUU`, stated as a *property* so it cannot be satisfied by special-casing 3600. ⭐ **`DefaultUU` is `ASummonedUnit::UnitEngagementRadiusUU`, read from the class.** I hand-type `2000` nowhere in this row — not in code, not as a comparison basis, not in this document. Test 2(b) repeats the property against a synthetic `DefaultUU * 2.f`, so a clamp cannot hide behind an allow-list of known card numbers.
5. **The refusal is structural too.** Test 2(f), over the extracted resolver body: **zero** `FMath::Min`, **zero** `Clamp`. Values alone would pass a `min` whose threshold happened to sit above everything tested.
6. ⭐⭐ **THE LINK NOBODY HAD CHECKED — and the one this row adds.** Links 3–5 all resolve against *this instance's class default*, so the Longbowman's answer depends on the Longbowman's own class, which nobody had looked at:
   - `ASiegePlayerController::ResolveCardActorClass` composes `/Game/Blueprints/Units/BP_Unit_<CardID>.BP_Unit_<CardID>_C` with `RequiredBase = ASummonedUnit::StaticClass()` for `CardType == Unit` ⇒ the Longbowman spawns as `BP_Unit_Longbowman_C`, an `ASummonedUnit`.
   - `Content/Blueprints/Units/BP_Unit_Longbowman.uasset` contains **ZERO** occurrences of `AggroRadius` ⇒ **no Blueprint CDO override** on the notice radius — the un-greppable hazard test 2(e) documents is, for this card, measurably absent.
   - It is **not** one of the two sealed classes (`AMinerUnit`/`ASorcererUnit`, `AggroRadius = 0.f` at `MinerUnit.cpp:72` / `SorcererUnit.cpp:23`): the asset holds `SummonedUnit` ×8 and `MinerUnit` ×0. So the seal branch (test 2(d), `Resolve(0, 3600) == 0`) **cannot fire**.
   - ⇒ `ResolveNoticeRadiusUU(positive class default, 3600) = 3600`.

   **Instrument controlled both ways (`SC-§39`):** the same byte probe across all 14 unit BPs returns `AggroRadius = 0` everywhere — so **no** unit Blueprint overrides the notice radius — while `CardID` returns `1` for the 12 combat BPs and **`0` for exactly `BP_Unit_Miner` and `BP_Unit_Sorcerer`**. The probe therefore discriminates between assets and reads real per-asset property names; an absent `AggroRadius` is evidence, not a dead instrument.

⚠️ **DECLARED GAP.** None of this is a match. There is no PIE pass, no world, no `SpawnActor`, and a raw byte scan of a serialized `.uasset` name table is strong but not conclusive. What is proven is: *the data reaches the member as 3600 through a resolver that provably cannot clamp, on a class that provably is not sealed.* What is **not** proven is that a Longbowman noticed something at 3600 uu in a running game.

**Fog is unchanged and stays a `min`.** Under fog the Longbowman goes to `min(3600, FogVisionCeilingUU)` — the same shared ceiling every unit gets, applied at the one chokepoint inside the acquisition funnel. **`−83.1%` is still the correct figure for 3600, so the shipped documentation of it is still true and I edited none of it.**

---

## 4. THE ≈3656.85 uu CEILING — HOW IT IS RECORDED, AND BY WHICH SYMBOL

**The invariant, cited by SYMBOL:** `ASummonedUnit::GetDistanceToTarget` — **the 3-arg overload** — in its closest-point-on-collision comment, which states the castle's origin sits at the centre of a `7313.7 × 7384.5` uu footprint and *"would never come within `Range`/`AggroRadius` of a unit standing at its walls."*

⛔ **No line number is cited, deliberately.** That region moved **three times today** (`1101` → `1111` → `1129`). `SC-§38`: a line number is a dated annotation; **the symbol is normative.**

**The ceiling is DERIVED, not quoted.** I did not take `3656.85` on faith from the board — I re-derived it from sources outside the fenced files:

| Source (all unfenced) | States |
|---|---|
| `Castle.h:369` | 9× bounds **`7313.7 × 7384.5 × 8082.6`** uu |
| `SiegeBotController.h:486`, `SiegePlayerController.h:2040` | the same triple, independently |
| `CONVENTIONS.md:3398` | **`5000 − 3656.85 = 1343.15`** — 3656.85 used as the castle's colliding half-width |

⇒ `7313.7 / 2 = 3656.85` exactly. **`3600` is legal. Anything above ≈3657 is not.**

### ⭐ A REFINEMENT NOBODY HAD BOARDED: *why the SMALLER half-extent*

`CONVENTIONS.md:5198` records that the castle's live bounds are rotation-robust and that **the X/Y extents SWAP: ≈3656.85 ↔ ≈3692.25** (verified at TASK-663, never assumed). The castles were rotated 90° gates-to-centre, so **which half-extent faces a given unit depends on the castle's rotation and the unit's bearing.**

⇒ The binding ceiling is the **MIN** of the two, `3656.85`. The board picked the right number; what is now on the record is *why the smaller one* rather than merely *which one*. Margin at 3600: **56.85 uu** against the near axis, **92.25 uu** against the far one. A future value chosen against 3692.25 would be legal on one bearing and false on the other.

### Grading it correctly — the naive reading is wrong

Falsifying this invariant does **not** immediately change behaviour: the code already uses closest-point-on-collision, which is the **more permissive** call. What breaks is **the stated reason that call exists**. The next person to "tidy" that comment — reading a premise that is no longer true — could swap back to an origin-distance metric, and the castle would begin self-acquiring at its own walls. ⚖️ **A justification invariant fails silently, later, through somebody else's hand.**

⇒ **56.85 uu is not headroom to spend.** If a future value would exceed it: **STOP, re-derive the invariant, never adjust the number to fit** (`SC-§60`). 🧑 If he ever asks for `3800`, the answer is *"here is exactly what that breaks"* — not a silent failure.

---

## 5. `SC-§65` — STALE CLAIMS **MY** CHANGE CREATES. Searched by claim SHAPE, not by number.

Shapes censused: *"not yet in the CSV"* · *"until then"* · *"deserializes to"* · *"sparse today"* · *"zero behaviour change"* · any Archer/Wizard **reach** claim with or without a number.

| # | Site (by symbol) | The claim my diff falsifies | Owner | Action |
|---|---|---|---|---|
| S-1 | `FCardRow::NoticeRange` doc block, `CardRow.h` — the "CSV note" paragraph | *"the `NoticeRange` header is **NOT yet** in Docs/Data/cards.csv"*; *"Until then every row deserializes to 0.0"*; ⛔ *"**zero behaviour change from the column's existence**"* | ⛔ **`CardRow.h` is LIVE under `TASK-985`** | **NOT edited** — declared |
| S-2 | the `NoticeRange` binding note in `ASummonedUnit::LoadStatsAndStart` | *"Sparse today: cards.csv has **no** NoticeRange column **yet**"* | ⛔ `TASK-979`'s **live diff** | **NOT edited** — declared |
| S-3 | the `ASummonedUnit` class-header ranged-delivery/history comment | asserts the Archer's `Range` *"had been **2100** in cards.csv"* | ⛔ `TASK-979`'s **live diff** — see §6 (E) | **NOT edited** — declared |
| S-4 | `SiegeFogStatics.h:177`/`:178`; `Tests/SiegeFogTest.cpp:233`/`:383`/`:695` | Archer/Wizard `2100`, `−71.0%`, `0.710f` | ✅ **`TASK-997`**, blocked on this handoff | correctly owned |
| S-5 | `CONVENTIONS.md:4637`, `:6001`, `:8568-8569`, `:8771`, `:8835-8836` | Archer/Wizard `700`/`2100`/`−71.0%` | manager | declared |

⭐ **S-1 is the dangerous one, and it is exactly the shape `SC-§65` warns about: *"zero behaviour change from the column's existence"* contains no number at all.** As of this diff the column changes the Longbowman's notice radius from the class default to 3600. A reader could take that sentence as a licence that the column is inert.

⛔ **Why I did not fix S-1/S-2/S-3 — `SC-§62` tested clause by clause.** (i) obedience makes no spec item unsatisfiable and breaks no build — the claims are stale prose, not code; **(ii) FAILS outright: every one of those files is owned by a live row** (`CardRow.h` → `TASK-985`; `SummonedUnit.{h,cpp}` → `TASK-979`, in QA loop 2 right now). Clause (ii) is not satisfiable, so **the breach is not available to me**, and editing them would also corrupt a reviewer mid-read. Declared instead, per the dispatch.

✅ **And the reassuring measurement:** the **only** tests that pin an Archer/Wizard range are `Tests/SiegeFogTest.cpp:233`/`:383`/`:695` — all inside `TASK-997`'s fence, which is blocked on this handoff. **My data change orphans no red test.**

---

## 6. WHAT THE BOARD AND THE DISPATCH **BOTH** MISSED

**(A) ⭐ The one-cell framing is arithmetically impossible** — §2. Minimum correct diff is 33 lines; the literal reading turns `SiegeAssistantSelectionTest` red 31 times, and it would have been blamed on the compile, not on the data.

**(B) ⚠️ `NoticeRange` is the FIRST *numeric* column in this CSV to ship blank cells.** Every other numeric column (`AoERadius`, `MinRange`, `EffectDuration`, `MaxTargets`, `GoldSteal`, `ChainTargets`, `ChainFalloff`) carries an explicit `0`. The only blank column today is `SpellDelivery` — an **enum**, and precisely the precedent `CardRow.h` invokes. UE's numeric `ImportText` consumes no characters from an empty buffer and fails, so a **CSV re-import** would likely log *"Problem assigning string '' to property 'NoticeRange'"* **×31**. Inert under the board's own item (4) (`set_rows`, **not** a re-import), and I obeyed `TASK-995` 4(a)(ii)'s *"must STILL BE BLANK"* literally — writing `0` would have risked its *"any OTHER value ⇒ BLOCKER"*. **Flagged, not acted on. If QA prefers `0`, say so and it is a one-line change.**

**(C) ✅ RESOLVED — THE ENGINE WAS LICENSED AND THE `DT_Cards` STEP IS NOW DONE AS FAR AS IT *CAN* BE. ⛔ See §9: half of it is compile-blocked and that is not a fence problem.**
*(Original finding, kept for the record: my dispatch carried a blanket "no editor, no engine" that contradicted board item (4). The coordinator ruled the board wins and licensed the engine narrowly. The conflict was real and reporting it was correct.)*

**(D) ⭐⭐ `TASK-997` IS BOARDED TO FIX A CLAIM THAT LIVES IN A FILE ITS FENCE DOES NOT NAME.**
Item (5b) tells `TASK-997` to locate the shipped *"the Archer's range is 700"* claim **by claim shape** and correct it to the post-`TASK-993` value `2000`. Censused by shape (`SC-§65` + `SC-§38a`): **the only shipping-source hit is `SummonedUnit.h:157-158`** — which additionally asserts the Archer's `Range` *"had been 2100 in cards.csv"*, now also false (S-3).
⛔ But `TASK-997`'s `names:` fence lists **only** `SiegeFogStatics.h` and `Tests/SiegeFogTest.cpp`, and `SummonedUnit.h` is **`TASK-979`'s live diff**.
⇒ **As boarded, `TASK-997` cannot satisfy its own item (5b)**, and the Archer stale claim survives the wave unless the manager extends 997's fence or hands the site to `TASK-979`. **This is a row-collision for the manager, not a judgement call for me** — stopping and reporting per the dispatch's `TASK-995` fence instruction.

**(E) ✅ Closed, needs nothing:** the `−83.1%` documentation for 3600 stays correct (firing side is a zero diff), and I touched none of it. `git diff --stat` over `Source/` shows **zero** files attributable to this row.

---

## 7. SUITE COUNT (`TL-§5c` — **declared**, nothing executed this wave)

| | Count | Basis |
|---|---|---|
| Test files | **34** | `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp` |
| `IMPLEMENT_*_AUTOMATION_TEST` | **445** | all `IMPLEMENT_SIMPLE_AUTOMATION_TEST`; zero complex/custom |

⇒ **445 / 34 — unchanged.** This row adds **zero** tests and **zero** files, so it reconciles exactly against the other two lanes' **445 / 34**. Δ = **0 / 0**.

---

## 8. WHAT QA SHOULD SCRUTINISE

1. ⛔ **§6(C) first — the `DT_Cards` gap is a real hole in item (4), and it needs a ruling, not a re-review.**
2. ⛔ **§6(D) — `TASK-997`'s fence vs the `SummonedUnit.h` Archer site.** If this is not resolved, `J-F29` half-ships.
3. That the 33-line diff is **exactly** one new column + one authored cell + two moved `Range` cells — re-derive it cell-for-cell; a line diff cannot see it (§2).
4. §3 link 6 — the BP-CDO probe is a **byte scan**, deliberately declared as strong-but-not-conclusive. If you want it conclusive, it needs the engine.
5. §6(B) — whether blank or `0` is wanted in the 31 sparse cells. My reading of `TASK-995` 4(a)(ii) forced blank.
6. `SC-§65` §5 — I claim S-1/S-2/S-3 are **undelegatable by me** under `SC-§62`(ii). Check that reasoning; if you disagree, the fix is one edit each and I will take it.

---

## 9. ⛔⛔ THE `DT_Cards` WRITE (item 4) — ENGINE LICENSED 2026-09-04, AND **HALF OF IT IS COMPILE-BLOCKED**

**Scope used:** Unreal MCP `127.0.0.1:8000` → `DataTableTools` + `AssetTools` on `/Game/Data/DT_Cards` **only**. ⛔ No compile, no Git, no editor open/close, **no `L_Arena` interaction of any kind** (not even a dirty-check).

### 9.1 ⛔⛔ THE BLOCKER, PROVEN BY EXECUTION — NOT INFERRED FROM THE SCHEMA

`get_schema` on `DT_Cards` returns **31 properties ending at `chainFalloff`. `noticeRange` is ABSENT.** The running editor's `FCardRow` is the **pre-`TASK-979`** struct — its `NoticeRange` field exists in *source* but has never been compiled (`QUIET-MODULE`; `TASK-987` owns the wave's one compile). Corroborated independently: the schema's `spellDelivery` description still reads *"Column header not yet in cards.csv (frozen this wave — flagged)"*, i.e. the reflection data mirrors the old source exactly.

I did not stop at the schema. **I attempted the write and let it answer:**

```
set_rows  {"Longbowman": {"noticeRange": 3600}}
  ->  ERROR: Properties not found in schema: ['noticeRange']
```

⭐ **The best possible failure mode: it refused LOUDLY rather than accepting-and-dropping.** A tool that silently no-op'd would have been indistinguishable from success — the `set_rows` success return is `null`, which carries no information either way. Read-back confirms **zero** of the 32 rows gained the property, and the saved `.uasset` on disk still contains **zero** occurrences of `NoticeRange`.

⇒ ⛔⛔ **NO ONE CAN WRITE `NoticeRange` INTO `DT_Cards` UNTIL `TASK-987` COMPILES.** This is an **ordering dependency**, not a permissions problem, and no licence can lift it.

### 9.2 ✅ WHAT *DID* LAND — the `J-F29` half, which needs no new property

`range` **does** exist in the compiled schema, so the firing half was writable and is now written **and saved**:

| Row | `range` before | `range` after | |
|---|---|---|---|
| `Archer` | 2100 | **2000** | ✅ written |
| `Wizard` | 2100 | **2000** | ✅ written |
| `Longbowman` | 3600 | **3600** | ✅ **untouched — the zero diff holds in the asset too** |
| `Footman` (control) | 120 | **120** | ✅ unchanged |

**Persisted, verified by an instrument outside the engine:** `is_dirty` `true → false`; on-disk mtime `2026-09-02 22:53:54 → 2026-09-04 02:55:51`; sha256 `edffce06… → b8205df1…`. ⚠️ Size is **identical** at 42,722 bytes — two in-place float edits grow no name table — so **a size check would have reported "nothing happened"**. Saved with an explicit one-asset list, ⛔ never the empty list that flushes every dirty asset in the editor; `git status` over `Content/` confirms only `DT_Cards.uasset` moved.

### 9.3 ⭐ THE POSITIVE CONTROL, MADE TO **DISCRIMINATE** RATHER THAN DECORATE

In the **same** read-back call the instrument returned values that **changed** where I wrote (`Archer`/`Wizard` 2100→2000) **and** values that did **not** change where I did not (`Footman` 120, `Longbowman` 3600, plus `Knight` 120, `Cleric` 400, `ArrowTower` 900, `BallistaTower` 1400, `BombTower` 800, `CrystalTower` 800, `WatchTower` 0 — every one matching `cards.csv`). So it is neither echoing my input nor returning a constant: **it can tell written from unwritten.**
Collateral check — `set_rows`' *"only specified properties are updated"* is a doc claim, so I tested it: `Archer` `hP` 45 / `damage` 10 / `deckCount` 8 / `notes` / `cardArt` all survive bit-identically, `Wizard` `aoERadius` still 250, `sum(deckCount)` still **50**. **No wholesale row replacement.**

### 9.4 ✅ THE 33-LINE SHORT-ROW SHAPE **CANNOT** REPRODUCE IN THE ASSET — checked on all 32 rows, not argued

```
row_count: 32   distinct_keysets: 1   property_count_per_row: [30]
short_row_possible: FALSE      rows_carrying_noticeRange: []
```

Every row carries an **identical** property set, because a DataTable row is an `FCardRow` **struct instance** — there is no positional field list that can come up short the way a CSV line can. ⇒ When the compile lands, **all 32 rows gain `NoticeRange = 0.0f` simultaneously**, which *is* the default sentinel — so item (4)'s *"every other row at the default sentinel"* clause **self-satisfies at compile time** and needs no 32-row write.

### 9.5 ⛔⛔⭐⭐ CAN `TASK-995` ITEM (4) PASS? **PARTLY — AND THE REST IS A GATE-ORDERING DEFECT, NOT A WORK DEFECT.**

| Item (4) clause | Verdict |
|---|---|
| `Archer` + `Wizard` `Range == 2000` in `DT_Cards` (4a-i) | ✅ **CAN PASS NOW** |
| `Longbowman` `Range` unchanged at `3600` in `DT_Cards` | ✅ **CAN PASS NOW** |
| every other row at the **default sentinel** | ✅ **self-satisfies at compile** (§9.4) |
| **`Longbowman`'s NOTICE column `== 3600` in `DT_Cards`** | ⛔⛔ **CANNOT PASS — and cannot be made to pass by anyone before `TASK-987` compiles** |

⇒ ⛔ **`TASK-995` gates `TASK-993` but is scheduled BEFORE `TASK-987`. As written, its item (4) can never pass inside its own window.** The gate needs either to grade that clause as *deferred-to-ship-host* or to move behind the compile. **This is a scheduling fact, not something a re-review of my diff can fix.**

### 9.6 ⛔⛔⭐⭐ THE HAZARD THE ASSET WRITE SURFACED THAT THE CSV DID NOT — **AND IT CAN SILENTLY UNSHIP HIS RULING**

**The game reads `DT_Cards`. It does not read `cards.csv`.**

After `TASK-987` compiles, all 32 rows gain `NoticeRange` at the struct default **`0.0`** — including the Longbowman, whose `cards.csv` cell says **`3600`**. **The asset and the CSV will DISAGREE on exactly the one cell this row exists to ship.**

`0.0` is the sparse sentinel, so it resolves to the class default ⇒ ⛔ **the Longbowman would notice at the default, not at 3600 — and nothing would be wrong anywhere a reviewer looks.** `cards.csv` is correct. The tests are green (they read the CSV off disk, or call the resolver directly — neither reads `DT_Cards`). The compile is clean. The QA report passes. **And his ruling is simply not in the game.**

⚠️ `SiegeAssistantSelectionTest` already declares this class of gap in its own header — *"the CSV is the DataTable's SOURCE, not the DataTable… a row written into the asset without being mirrored into the CSV would be invisible here"*. **This is that gap running in the opposite direction**, and it is worse, because the direction that loses is the one the game actually reads.

⛔⛔⛔⭐⭐ **SUPERSEDED 2026-09-04 BY THE MANAGER — ⛔ THIS PARAGRAPH IS ⛔ NOT AN INSTRUCTION. ⛔ DO ⛔ NOT EXECUTE IT.** *(Inserted by the manager, ⛔ NOT by this handoff's author. ⛔ The author's text is ⛔ struck, ⛔ never deleted — `TL-§5c` cl. 4. ⛔ **Everything else in this document, including all of §9.6's reasoning above, is ⛔ UNTOUCHED and ⛔ still load-bearing.**)*

⛔⛔ **WHY: 🧑 JONATHAN'S `5000` RULING (`J-F28`, 2026-09-04) MADE THE LONGBOWMAN'S `NoticeRange` CELL ⛔ BLANK** (⭐ `TASK-1004`). ⛔ **At a `5000` class default, `3600` would make the Longbowman the ⛔ ONE UNIT IN THE GAME THAT NOTICES ⛔ LESS THAN EVERYONE ELSE — ⛔ INVERTING 🧑 HIS OWN `J-F20`, which excepted that card ⛔ UPWARD.**

✅ **THE CORRECT END STATE IS THE ⛔ DEFAULT STATE: ⛔ `noticeRange = 0.0` on ⛔ ALL 32 ROWS** — the ⛔ sparse sentinel meaning *"use the class default"* ⇒ ⛔ every unit resolves to `5000`. ⭐ **`TASK-1005` MEASURED it already in that state.** ⇒ ⛔⛔ **A WRITE WOULD ⛔ BREAK IT.** ⛔ **⭐ `TASK-1001` is ⛔ RE-SCOPED from a write to a ⛔ VERIFY.**

⚠️⛔ **AND THE GRADING THIS EARNED, ⛔ RECORDED BECAUSE IT IS THE POINT (`qa/TASK-995.md` W-1): ⛔ this was graded ⛔ WARN rather than ⛔ BLOCKER ⛔ ONLY because `TASK-1001` item (1) ⛔ independently forbids the write** ⇒ ⛔ **it was fenced by the ⛔ luck of another row, ⛔ NOT by anything in this document.** ⇒ ⚖️ ***⛔ A CANCELLED SWEEP IS ⛔ MORE DANGEROUS THAN AN OPEN ONE — ⛔ ITS TEXT IS ⛔ SPECIFIC, ⛔ CONFIDENT, ⛔ BOLDFACED AND ⛔ WRONG*** (⭐ `SC-§73`, ⛔ whose active-harm clause is ⛔ extended to ⛔ HANDOFFS by ⛔ this exhibit).

~~⇒ ⛔⛔ **REQUIRED, AND IT MUST BE BOARDED ON OR AFTER `TASK-987`: one post-compile `set_rows` — `{"Longbowman": {"noticeRange": 3600}}` — followed by a read-back and a save.** One cell. If it is skipped, everything looks correct and the feature is absent. **The board's own *"`set_rows`, NOT a CSV re-import"* rule means this will not happen by itself.**~~

⭐ **WHAT ⛔ SURVIVES THIS SUPERSEDE, ⛔ AND IT IS THE MAJORITY OF THE SECTION:** ⛔ the CSV↔`DT_Cards` ⛔ ASYMMETRY (*"the game reads `DT_Cards`, it does ⛔ not read `cards.csv`"*) is ⛔ PERMANENTLY TRUE and is ⛔ now law as ⭐⭐ **`FOG-§9.11a`** (⛔ **both halves land or ⛔ neither does**) · ⛔ the `SiegeAssistantSelectionTest` gap-direction observation ⛔ stands · ⛔ and it is ⛔ WHY ⭐ `TASK-987` clause (6b) ⛔ now stages `Content/Data/DT_Cards.uasset` ⛔ by name. ⇒ ⛔ **The ⛔ DIAGNOSIS was right. ⛔ Only the ⛔ PRESCRIBED VALUE was overtaken by a later ruling.**

---

**Files touched by this row: `Docs/Data/cards.csv` and — under the 2026-09-04 engine licence — `Content/Data/DT_Cards.uasset` (two `range` cells; saved).**
