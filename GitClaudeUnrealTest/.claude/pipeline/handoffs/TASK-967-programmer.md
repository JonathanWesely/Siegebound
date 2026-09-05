# TASK-967 — [ROSTER-REVERSE] 14 unit Blueprints vs 13 `CardType Unit` rows

**Agent:** gameplay-programmer · **Date:** 2026-09-03 · **Tree:** `09b9b50` + working-tree dirt
**Deliverable:** this document. **Writes: this file only.** Zero `Source/**`, zero `Content/**`, no compile, no editor, no MCP, no Git writes.

---

## 0. VERDICT — `STRUCTURALLY COVERED`

**The odd Blueprint is `BP_Unit_Miner`. It is not an orphan. It has a `DT_Cards` row keyed `Miner` under `CardType Economy`, it is player-playable, and the `TASK-949` roster gate already probes it and passes it.**

This is item (3)(b) of the spec: *named by `DT_Cards` under a DIFFERENT `CardType`* ⇒ `STRUCTURALLY COVERED`, and the spec requires me to say which gate sees it — **`Siegebound.CardRoster.EverySpawnableCardRowResolvesItsComposedActorClassPath` sees it**, as one of the "2 Economy" rows in its own printed census.

**The `14`-vs-`13` finding is a category error in the comparison's denominator, not a defect.** The correct denominator is not "rows with `CardType Unit`" but "rows whose **spawn category** is `UnitActor`" — which is 13 `Unit` rows **+ 1 non-building `Economy` row (Miner) = 14**. Measured against that denominator the census is an **exact bijection, 14 ↔ 14**, with zero orphans in either direction.

⛔ **No change is warranted. Nothing to delete, nothing to add, no row to board for a repair.** Per `SC-§52` cl. 3 and the spec's item (0), this row closes with zero changes and that is the success case.

### `SC-§37` discharge — why this is "structurally covered", not "inert by coincidence of today's data"

The requirement is that the thing be safe **because of how the code is shaped**, not because current data happens to miss it. It is:

```cpp
// SiegePlayerController.cpp:4616  (ASiegePlayerController::ResolveCardActorClass)
else if (CardType == ECardType::Unit || CardType == ECardType::Economy)
{
    ClassPath = FString::Printf(TEXT("/Game/Blueprints/Units/BP_Unit_%s.BP_Unit_%s_C"), *CardName, *CardName);
    RequiredBase = ASummonedUnit::StaticClass();
}
```

The `|| CardType == ECardType::Economy` is a **code-shape fact**. *Any* Economy card not listed in `BuildingEconomyCardIDs` composes a `/Game/Blueprints/Units/BP_Unit_<CardID>` path — today, and for every Economy card anyone adds tomorrow. The gate's `ClassifyRow` mirrors that same branch. So `BP_Unit_Miner`'s coverage does not depend on Miner specifically existing; it follows from the branch. That is structural.

---

## 1. THE MEASUREMENT, RE-TAKEN (spec item 1, `SC-§40` cl. 1)

I re-measured both sides from the tree rather than inheriting the dated annotation. **Both figures in the dispatch reproduce exactly.**

| Side | Source | Count |
|---|---|---|
| Unit Blueprints on disk | `Content/Blueprints/Units/BP_Unit_*.uasset` | **14** |
| `CardType Unit` rows | `Docs/Data/cards.csv` | **13** |
| Total card rows | `Docs/Data/cards.csv` | **32** |

Full type breakdown, which **reproduces `TASK-949`'s compiled DataTable read to the unit on all six figures**:

| `CardType` | Rows | Spawnable? |
|---|---|---|
| Unit | 13 | ✅ |
| Economy | 2 | ✅ |
| Building | 7 | ✅ |
| Utility | 1 | ⛔ excluded |
| HeroUpgrade | 4 | ⛔ excluded |
| Spell | 5 | ⛔ excluded |
| **Total** | **32** | **22 spawnable / 10 excluded** |

`TASK-949`'s compiled run printed `32 row(s) read; 22 SPAWNABLE probed (13 Unit, 2 Economy, 7 Building); 10 EXCLUDED`. My CSV-derived figures are **identical in all four numbers and all six per-type counts**. That is two independent instruments (a compiled `DT_Cards` read vs. my CSV parse) agreeing — see §6 for the one thing this agreement does *not* prove.

---

## 2. THE SET DIFFERENCE, BOTH DIRECTIONS (spec item 2)

Published in full, sorted, by set difference and not by eye.

**`CardType Unit` rows (13):**
`Archer, Cavalry, Cleric, Footman, Knight, Longbowman, MilitiaMob, Ogre, Pikeman, Sapper, Sorcerer, Witch, Wizard`

**`BP_Unit_*` on disk (14):**
`Archer, Cavalry, Cleric, Footman, Knight, Longbowman, MilitiaMob, `**`Miner`**`, Ogre, Pikeman, Sapper, Sorcerer, Witch, Wizard`

| Direction | Result |
|---|---|
| **Blueprint with no `Unit` row** (the reverse orphan) | **`Miner`** — the single element |
| **`Unit` row with no Blueprint** (the forward gap) | **NONE** |

⭐ **The forward direction is empty, so there is no contradiction with the green gate** — the spec flagged a hit here as a stop-and-report, and there is no hit. `TASK-949`'s 13/13 Unit resolution is corroborated independently.

And `Miner` **is** in `DT_Cards`, under a different type:

```
Miner -> DT_Cards row keyed 'Miner': CardType = Economy
```

### The same apparent mismatch exists on the Buildings side, and resolves identically

I checked the neighbouring family while I had both sets in hand, because the same blind spot applies to it:

| | Rows | On disk |
|---|---|---|
| `CardType Building` | 7 | `BP_Building_*` = **8** |

The extra is **`BP_Building_DeepMine`** — `CardType Economy`, and listed in `BuildingEconomyCardIDs`, so it composes a *Buildings* path. **An 8-vs-7 mismatch of exactly the same shape and exactly the same cause.** Two independent instances of the same category error strongly confirm the diagnosis: the mismatch is in the comparison, not the assets.

### The corrected census — spawn category, not `CardType`

Classifying each row by the shipped `IsBuildingCard` + `ResolveCardActorClass` split (`BuildingEconomyCardIDs = { "DeepMine" }`, read from the CDO initialiser at `SiegePlayerController.h:1663`):

```
SPAWNABLE ROWS: UnitActor=14  BuildingActor=8   (NotSpawnable=10)   total=32
ON DISK       : BP_Unit_*=14  BP_Building_*=8

UNITS      row-not-on-disk = NONE    on-disk-not-a-row = NONE
BUILDINGS  row-not-on-disk = NONE    on-disk-not-a-row = NONE

BIJECTION: EXACT
```

**14 + 8 = 22 spawnable rows ↔ 22 spawnable Blueprints on disk. Zero orphans, zero gaps, both directions, both families.**

---

## 3. IS IT REACHABLE? — YES, FULLY, AS A NORMAL PLAYED CARD (spec item 3)

Not merely resolvable — **playable**. The whole chain is measured at source:

1. **It is a card in the table.** `Miner`, `CardType Economy`, cost/deck fields populated — it enters decks and hands like any other card.
2. **`PlayHandSlot` routes it into placement mode.** `SiegePlayerController.cpp:1016` — `case ECardType::Economy:` falls in with `Unit` and `Building` under the placement branch, calling `EnterPlacementMode(CardID)`. Economy is a first-class placement type, not an instant.
3. **`IsBuildingCard` returns false for it.** `SiegePlayerController.cpp:4657` — `CardType == ECardType::Economy && BuildingEconomyCardIDs.Contains(CardID)`; the array default is `{ "DeepMine" }` only, so Miner is **not** a building-economy card.
4. **`ResolveCardActorClass` therefore composes the Units path** (`:4616`) → `/Game/Blueprints/Units/BP_Unit_Miner.BP_Unit_Miner_C`, requiring base `ASummonedUnit`.
5. **The asset satisfies that base.** `BP_Unit_Miner.uasset` reparents to `/Script/GitClaudeUnrealTest.MinerUnit` (read from the asset's own import table), and `MinerUnit.h:287` declares `class GITCLAUDEUNREALTEST_API AMinerUnit : public ASummonedUnit`. ✅

The shipped code even documents this case explicitly, at `SiegePlayerController.cpp:4158-4160`:

> *"a Unit card **OR an Economy-typed UNIT card (Miner)** resolves `/Game/Blueprints/Units/BP_Unit_<CardID>` (`ASummonedUnit`)"*

**This is not an accident anyone needs to discover — it is a designed, commented, named special case with a dedicated C++ class, a mirrored bot path (`SiegeBotController.h:250` carries the same `BuildingEconomyCardIDs` default), and a cheat-manager path.** `Miner` appears across ~30 source files.

### Which gate sees it

`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardRosterTest.cpp`, `ClassifyRow`:

```cpp
case ECardType::Economy:
    // Deep Mine's row is Economy for the GDD §8 raidable-economy semantics,
    // but `ADeepMine` derives `ABuilding` and lives under /Blueprints/Buildings.
    Category = BuildingEconomyCardIDs.Contains(CardID) ? ESpawnCategory::BuildingActor : ESpawnCategory::UnitActor;
    break;
```

Miner is not in `BuildingEconomyCardIDs` ⇒ `UnitActor` ⇒ the gate composes `/Game/Blueprints/Units/BP_Unit_Miner.BP_Unit_Miner_C` and asserts it resolves to an `ASummonedUnit`. **It is one of the "2 Economy" rows in the gate's own census line, and that gate is green.** The reverse blind spot is real, but `BP_Unit_Miner` was never in its shadow.

---

## 4. LEGACY OR RECENT? — **LEGACY, AND THE PAIR HAS NEVER DIVERGED**

The dispatch asked me to settle this, because a Blueprint added last week is a different problem from one that shipped in July. It shipped in July, together with its card:

| Commit | Date | Event |
|---|---|---|
| `5bb9507` | **2026-07-04** | `BP_Unit_Miner.uasset` **created** (M2, TASK-031..038) — and `cards.csv` at that same commit already carries `Miner,…,Economy` |
| `150c3c3` | 2026-07-17 | Touched by M7 rigged-unit integration (`SK_Miner` on the shared SiegeBiped skeleton) |

`Miner`'s `CardType` read directly out of `cards.csv` at three points in history:

```
5bb9507  Miner CardType=Economy
150c3c3  Miner CardType=Economy
HEAD     Miner CardType=Economy
```

**The Blueprint and its `Economy` card row were born in the same commit, two months ago, and the row has never been anything but `Economy`.** There is no drift event to investigate — nothing ever moved. This has looked like `14`-vs-`13` since the day it shipped, and it has been correct the whole time.

---

## 5. THE REVERSE DETECTOR — NAMED, NOT BUILT (spec item 4)

⛔ **I wrote no code for this.** Description only, for the manager to board or not.

### The trap first: what a *naive* reverse gate would do

> *"Every `BP_Unit_<X>.uasset` must have a `DT_Cards` row keyed `<X>` with `CardType == Unit`."*

**That gate goes RED on `BP_Unit_Miner` — a correct, shipped, playable asset.** Its building twin goes RED on `BP_Building_DeepMine`, also correct. So the naive shape produces **two false reds and zero true findings** on today's tree, and invites an edit to two good files. This is precisely the `TASK-952` shape that would have gone red on nine correctly-shipping units. **The `DESIGN`/`Economy` population is exactly why the naive reverse gate is wrong.**

### What the detector must actually assert

> *For every `BP_Unit_<X>.uasset` under `Content/Blueprints/Units/`, there exists a `DT_Cards` row keyed `<X>` for which the **shipped classifier** yields `UnitActor`; and symmetrically for `BP_Building_<X>` / `BuildingActor`.*

**The load-bearing word is "shipped classifier."** The detector must call the same `ClassifyRow` + read `BuildingEconomyCardIDs` off the CDO that the forward gate uses — **never** re-derive a `CardType == Unit` test of its own. That single reuse is what keeps Miner and DeepMine green *without any hand-written exclusion list*, and what keeps the detector correct when someone adds a third Economy-typed card.

### Legitimate exclusions it would need

1. **Economy→`UnitActor` (Miner) and Economy→`BuildingActor` (DeepMine)** — handled *structurally* by the classifier reuse above, **not** by an allowlist. An allowlist here would be the wrong mechanism.
2. **Summon-only Blueprints** — a unit spawned by a Building/Spell rather than played (a `SpawnCardID` target, a spell's summon class, a `TSubclassOf` default). **Today this population is EMPTY** — the only `SpawnCardID` in the table is `Barracks → Footman`, and Footman is itself a `Unit` card, so nothing needs excluding. But the day someone adds a summon-only unit with no card row, the detector goes red on a correct asset. It needs a **declared allowlist with a written reason per entry**, empty on day one.
3. **Non-shipping paths** — redirectors, `Saved/Autosaves/**` (there is a `DT_Cards_Auto1.uasset` today), `Developers/**`, and any test/scratch folder.
4. **Deliberately retired** Blueprints kept alive for save-game or level references.

### The instrument trap specific to a reverse gate (`SC-§39`)

⚠️ **A reverse census that enumerates nothing is indistinguishable from a clean tree — both are green.** This is the *inverse* of the forward gate's failure mode and it is more dangerous, because the forward gate at least has a row list that would obviously be empty. A reverse gate whose directory scan silently misses (wrong mount point, casing, packaged-vs-editor path, AssetRegistry not yet scanned) reports "zero orphans" and **passes forever**. It therefore **must** carry a positive control asserting it finds a known-present Blueprint (`BP_Unit_Sorcerer`), and ideally a count floor.

### Is it worth a row? — **Marginal today. My honest recommendation: LOW priority, and I would not displace anything for it.**

I have to be straight about its value rather than oversell it: **I just measured the exact bijection, so this detector would find zero orphans today.** It protects future work only. Arguments each way:

- **For:** an orphaned Blueprint is exactly the thing the next person copies as a donor, and `TASK-946` measured that copying the wrong donor is a live hazard (the Cleric's law-violating rotation, the Sorcerer's parent). A reverse gate converts "dead weight nobody noticed" into a machine check. It is also cheap — it reuses the forward gate's fixtures entirely.
- **Against:** zero current population; it adds a gate whose *only* correct implementation is subtle (classifier reuse + positive control), and a botched implementation is actively harmful — two false reds pointing at good files, which is worse than no gate. `SC-§52` cl. 2 applies: *"I found redundancy" is not on its own a licence to edit.*

⚖️ **If it is boarded, the spec must mandate (a) reuse of the shipped `ClassifyRow`/`BuildingEconomyCardIDs` rather than a fresh `CardType` test, and (b) a positive control on the enumeration itself.** Without both, do not build it.

---

## 6. INSTRUMENT CONTROLS (spec item 5, `SC-§39`)

Every instrument was controlled in both directions. **No blind zero is reported anywhere in this document.**

**`DT_Cards.uasset` reader** (ASCII token extraction, 42,722 bytes, 267 tokens):

| Probe | Expected | Result |
|---|---|---|
| `Witch` | present (known-present at `09b9b50`) | ✅ found |
| `Miner`, `DeepMine`, `Footman`, `Sorcerer`, `Cleric`, `Barracks`, `Masons` | present | ✅ all found |
| `ZZZ_NoSuchCard` | absent | ✅ not found |
| `Xylophonist` | absent | ✅ not found |

⇒ The reader **discriminates**. It is not the empty-set reader that would have "proved" 14 orphans.

**Blueprint parent-class reader** — controlled by returning *different* values per file: Miner→`MinerUnit`, Sorcerer→`SorcererUnit`, Footman→`SummonedUnit`. A reader returning a constant would have been caught.

**Set-difference census** — inherently two-sided; it reported a non-empty difference in one direction (`Miner`) and empty in the other, so neither result is a silent zero.

### ⚠️ ONE INSTRUMENT FAILED AND I CAUGHT IT — recorded, because it is the exact trap the dispatch named

My first history check printed **three confident empty results** for the `Miner` row at three commits. That is the *"a failed `git show` printed three confident zeros"* failure verbatim. **I did not report it.** Diagnosis: **the git root is one level above the project directory** — the repo path is `GitClaudeUnrealTest/Docs/Data/cards.csv`, and `git show <c>:Docs/Data/cards.csv` resolves against the *repo root*, not the cwd:

```
fatal: path 'GitClaudeUnrealTest/Docs/Data/cards.csv' exists, but not 'Docs/Data/cards.csv'
```

Re-run as `git show <c>:./Docs/Data/cards.csv` it returned real data (§4). ⭐ **Note the asymmetry that makes this dangerous: `git log -- <relative path>` resolves against the cwd and worked fine, so the *same* relative path was simultaneously valid for one git subcommand and silently empty for another.** Anyone doing history archaeology in this repo from the project subdirectory should use `:./path` for `git show`/`git cat-file`.

---

## 7. WHAT I COULD NOT DETERMINE

Declared above my own claim, per the conduct this row's dispatch credits.

1. ⚠️ **I did not decode `CardType` per row from `DT_Cards.uasset` itself.** My ASCII extraction proves row-**key** presence/absence in the binary (that is all the control in §6 establishes), but it cannot read each row's `CardType` byte — those are serialized as tagged properties I did not parse. **My per-row `CardType` mapping comes from `Docs/Data/cards.csv`.** The mapping is corroborated by `TASK-949`'s *compiled* DataTable read agreeing on all six per-type counts, which is strong — but **a drift between `cards.csv` and `DT_Cards.uasset` that preserved all six counts (e.g. two rows swapping types) would be invisible to me.** I judge this remote and it does not affect the verdict (the `Miner` key is measured present in the binary, and the shipped resolver's Economy branch covers it either way), but it is not zero and I will not claim otherwise.
2. ⛔ **No runtime observation.** The editor is wedged on a modal dialog and fenced; I ran no compile, no PIE, no MCP. That `BP_Unit_Miner` *actually spawns in a match* is inferred from source + asset headers, not observed. (`TASK-949`'s green compiled gate does observe class resolution, which covers most of this.)
3. **I did not inspect `BP_Unit_Miner`'s internals** — components, its `SM_`/`SK_` assignment, or whether it carries any of the donor defects `TASK-946` found in the Cleric/Sorcerer. Out of this row's scope; flagged only because §5 argues orphan-hunting matters *because* of donor copying, and I have not audited this particular donor.
4. **I did not verify `BuildingEconomyCardIDs` is unmodified on the Blueprint/instance side.** I read the C++ CDO initialiser (`{ "DeepMine" }`, in both the controller and the bot). It is an `EditDefaultsOnly` `UPROPERTY`, so a Blueprint subclass or level instance could in principle override it. Checking that needs the editor.

---

## 8. FENCE COMPLIANCE

✅ Zero `Source/**` writes · ✅ zero `Content/**` writes · ✅ `DT_Cards`, `cards.csv`, CONVENTIONS untouched · ✅ no reverse detector built (§5 is prose) · ✅ orphan **not** deleted and its deletion **not** boarded — and there is no orphan to delete · ✅ no compile · ✅ no editor, no MCP (wedged, untouched) · ✅ no Git writes (`git log`/`show`/`cat-file` read-only only) · ✅ did not touch `TASK-964`'s `SiegeCardRosterTest.cpp` — read-only, and I introduced no edit that could collide.

⚠️ **ONE DECLARED DEVIATION — I did not update the task board, deliberately.** The dispatch allowed "my own status"; the board row itself is stricter, fencing "the board" in item (6) and stating in `names:` that this row **"Writes EXACTLY ONE file: `handoffs/TASK-967-programmer.md`"** with **"NO QA gate — zero code, zero assets."** I took the more specific and more restrictive instruction and wrote only this file. **`TASK-967`'s status is left for the orchestrator/manager to flip** — flagging rather than silently choosing either way. There is also no QA gate to route to, per the row's own `names:` clause.

---

## 9. ONE-LINE SUMMARY

**`BP_Unit_Miner` — `STRUCTURALLY COVERED`.** It has an `Economy` card row, `ResolveCardActorClass` routes `Unit || Economy` to the Units path by construction, the roster gate already probes it green, and it has been paired with its card since `5bb9507` on 2026-07-04. Counted by **spawn category** instead of `CardType`, the roster is an exact **22 ↔ 22** bijection with zero orphans in either direction. **No defect. No change. Zero diff, by design.**
