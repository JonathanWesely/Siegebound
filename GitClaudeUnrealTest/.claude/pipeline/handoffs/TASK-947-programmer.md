# TASK-947 — the spawnable-roster gate — gameplay-programmer handoff

**Author:** gameplay-programmer · **Date:** 2026-09-03 · **Gate:** `TASK-948` · **Commit host:** ⭐ **`TASK-949`** (`TL-§5d` cl. 1 — named, and it is the row that must take this file)

**Law honoured:** `SC-§50` cl. 4 · `SHIP-§9` · `SC-§39` · `SC-§49` · `SC-§41` · `SC-§38` · `SC-§40` cl. 10 · `TL-§5b`/`§5c`/`§5d`

---

## 0. ⛔ READ THIS FIRST — WHAT I COULD AND COULD NOT EXECUTE (`TL-§5c` cl. 5(a): the hole is NAMED, above the claim, never in a footnote)

⛔ **I DID NOT COMPILE AND I DID NOT RUN THE COMPILED TEST.** My fence bans compiling, the editor, MCP and Git; `TASK-946` was authoring a Blueprint in the live editor (PID 40528) throughout. A UE automation test cannot be executed without both a compile and an editor.

⇒ the evidence in §3 is split, explicitly:

| | what it is | who owes it |
|---|---|---|
| ⛔ **DECLARED** | the C++ file compiles / the test registers | ⭐ **`TASK-949`** — it compiles anyway |
| ✅ **EXECUTED (by me)** | the **predicate** run three times over the real tree by an **independent shell instrument** that mirrors it | me, transcripts pasted below |

⛔ **A shell mirror is not the compiled test and I do not claim it is.** What it IS: the *identical* predicate, over the *identical* tree, deriving its inputs by grepping the *same shipped source lines* the C++ reads at run time. `TL-§5c` cl. 5(c) — **the duty transfers by name: `TASK-949` runs `Automation RunTests Siegebound` and reports the `Result={}` pair.**

---

## 1. THE DELIVERABLE

**ONE NEW FILE, SOLE:**
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\Tests\SiegeCardRosterTest.cpp`

**Test name (exactly as specified):** `Siegebound.CardRoster.EverySpawnableCardRowResolvesItsComposedActorClassPath`
**Class:** `FSiegeCardRosterSpawnableActorClassPathTest` · **Flags:** `EditorContext | EngineFilter` (the house pattern)

### ⛔ ZERO PRODUCTION EDITS — MEASURED, NOT ASSERTED

```
$ git -C <repo-root> status --porcelain -- Source/
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardRosterTest.cpp
```
**One line, and it is `??`.** ⛔ No `SiegePlayerController.{h,cpp}` write ⇒ **no collision with `TASK-942`.** ⛔ No `Content/**`, no `DT_Cards`, no `cards.csv` (proven again in §3 RUN 3, where the synthetic row went into a **scratchpad copy** and the repo file measured `0` modifications).

⚠️ **NOTE FOR BUILD-MASTER (`SC-§40` cl. 1):** the git root is **`C:/GitProjects/GitHub/GitClaudeUnrealTesting`**, one level ABOVE the project dir — repo-relative pathspecs carry a `GitClaudeUnrealTest/` prefix. `TASK-949`'s `names:` block writes them without it. **Confirm in your own `git status`; do not adopt either spelling from prose.**

---

## 2. ⭐⭐ EVERYTHING IS DERIVED. NOTHING IS TRANSCRIBED.

⛔ **A hand-written list of 22 buys one release of correctness and re-arms the identical trap for card #23** — the mistake `TASK-874` was boarded to **undo** (it *deleted* a hardcoded 13-name roster rather than bumping it to 14). Every input is read at run time:

| input | derived from | why not typed |
|---|---|---|
| the **roster** | `ASiegePlayerController::CardTableAsset` read off the **CDO by reflection** (`protected`), loaded, walked by `GetRowNames()` | ⭐ this is the **same `UDataTable` object `ResolveCardRow` loads** — a row present in the asset but absent from `cards.csv` is **still walked** (the neighbouring `SiegeAssistantSelectionTest` reads the CSV and declares that reimport gap as a residual; **this file does not inherit it**) |
| the **Economy→Building exception** | `BuildingEconomyCardIDs` off the same CDO by reflection | typing `DeepMine` would put a **second copy** in the tree, free to drift |
| the **spawnable/excluded split** | a `switch` over `ECardType` with **every enumerator named and NO `default:` label** | a 7th card type is a **compile-time `-Wswitch` failure**; an out-of-range byte from the asset falls through to `Unclassified` and goes **red at run time**. ⛔ No silent third outcome. |
| the **required base classes** | `ABuilding::StaticClass()` / `ASummonedUnit::StaticClass()` — the **symbols** (`SC-§38`) | a string would not follow a rename |

⚠️ **Every reflection lookup checks the TYPE, not just the name**, and a miss is a **hard `AddError` + `return false`**, never a skip. A renamed property that merely made the file walk zero rows would otherwise produce **the same green bar as a clean roster** — the exact vacuous pass this gate exists to prevent.

---

## 3. ⛔⛔ `SHIP-§9` — THE GATE WAS SEEN **RED** BEFORE IT WAS TRUSTED

**Instrument:** a shell mirror of the predicate (see §0). It **derives** its inputs by grepping the shipped source — the two `Printf` format strings out of `ResolveCardActorClass`, the `BuildingEconomyCardIDs` initialiser out of `SiegePlayerController.h`, the `CardType` column index out of the CSV header — so the roster and the contract are **read, not typed**, exactly as the C++ does.

### ⭐⭐ RUN 1 — THE FREE, REAL, **UNSYNTHESISED** RED (19:00:57, `BP_Unit_Witch` genuinely absent)

⛔ **`TASK-946` had NOT yet landed. This is the real thing, not a synthesis.**

```
── THE WALK — every DT_Cards/cards.csv row, spawnable rows resolved ──
  ok       Footman          (Unit)    /Game/Blueprints/Units/BP_Unit_Footman
  ...
  ok       Sorcerer         (Unit)    /Game/Blueprints/Units/BP_Unit_Sorcerer
  ok       WatchTower       (Building) /Game/Blueprints/Buildings/BP_Building_WatchTower
  ⛔ FAIL   Witch            (Unit)    /Game/Blueprints/Units/BP_Unit_Witch   <- DOES NOT RESOLVE (ASummonedUnit)

── ASSERTED COUNTS (relationships, ⛔ no literal totals — SC-§40 cl.10) ──
  total rows walked ............ 32
  spawnable (Unit|Economy|Building) 22
  excluded  (Spell|HeroUpgrade|Utility) 10
  partition: 22 + 10 = 32  vs total 32   -> BALANCES
  non-vacuous: spawnable > 0     -> YES
  distinct composed paths ...... 22  vs spawnable 22 -> NO COLLISION

── EXCLUSIONS NAMED WITH THEIR COUNTS (spec item 1) ──
  Building     7        Economy      2        HeroUpgrade  4
  Spell        5        Unit        13        Utility      1

── CONTROLS (SC-§39) ──
  POSITIVE control  Sorcerer -> .../BP_Unit_Sorcerer : RESOLVES (instrument can return PRESENT)
  NEGATIVE control  ZzNoSuchCardZz -> .../BP_Unit_ZzNoSuchCardZz : DOES NOT RESOLVE (instrument can return ABSENT)
  NEGATIVE control is not a real row: cards.csv hits = 0 (must be 0)

 RESULT: ⛔ RED — 1 spawnable row(s) do not resolve: Witch          EXIT=1
```

⇒ ⭐ **THIS GATE WOULD HAVE GONE RED AT `1aa0fee`.** That is `SC-§50` cl. 4's claim, executed rather than argued.

### ⭐⭐ RUN 2 — AND THEN `TASK-946` LANDED **MID-TASK**, SO I CAUGHT THE **RED → GREEN TRANSITION LIVE**

`BP_Unit_Witch.uasset` appeared at **19:02**. Same instrument, same tree, **the only change being the asset**:

```
  total rows walked 32 · spawnable 22 · excluded 10 · partition BALANCES · 22 distinct paths, NO COLLISION
  POSITIVE control Sorcerer : RESOLVES     NEGATIVE control ZzNoSuchCardZz : DOES NOT RESOLVE
 RESULT: GREEN — every spawnable row resolved.                                   EXIT=0
```

⭐⭐ **`TASK-949` item (1) asks for exactly this transition as its integration check — it is already observed, on the predicate, and I did not have to arrange it.** ⛔ It still owes the **compiled** version (see §0).

### ⭐ RUN 3 — THE DISCRIMINATION PROOF, ON A NAME **MEASURED** ABSENT (`SC-§40` cl. 10)

Because 946 landed, the dispatch's fallback binds: *point it at a name you measure absent.* One synthetic spawnable `Unit` row for `ZzNoSuchCardZz` appended to a **scratchpad copy** of `cards.csv`:

```
  ⛔ FAIL   ZzNoSuchCardZz   (Unit)    /Game/Blueprints/Units/BP_Unit_ZzNoSuchCardZz  <- DOES NOT RESOLVE
  total rows walked 33 · spawnable 23 · partition 23 + 10 = 33 BALANCES · 23 distinct paths
  NEGATIVE control is not a real row: cards.csv hits = 1 (must be 0)   <- this control CORRECTLY fired too
 RESULT: ⛔ RED — 1 spawnable row(s) do not resolve: ZzNoSuchCardZz    EXIT=1

  repo cards.csv untouched? -> 0 modification(s)
```

⇒ **the instrument discriminates in both directions on the same tree, and it is not the Witch row that makes it red — it is ANY unresolvable spawnable row.**

### ⛔ THE SYNTHETIC VALUE WAS **MEASURED** ABSENT, NOT ASSUMED (`SC-§40` cl. 10)

| probe | `ZzNoSuchCardZz` | positive control `Sorcerer` |
|---|---|---|
| `Docs/Data/cards.csv` | **0** | 1 |
| `Content/Data/DT_Cards.uasset` (`grep -a`) | **0** | 3 |
| `Content/Blueprints/**` filenames | **0** | 1 |
| files under `Source/` | **0** | 27 |

⛔ *"I picked a token nothing should match"* is not evidence. Four instruments, each shown able to return non-zero. Recorded in the test's own header comment so nobody re-derives it.

---

## 4. ⛔⛔ THE VACUOUS-GREEN DEFECT — THE COUNT IS **ASSERTED**, NOT MERELY LOGGED

⭐ **This is `TASK-948` item (2)'s blocker and I built the file around it.** A walk over zero rows passes vacuously and reports the **identical green** as one that checked the whole roster. Five assertions are about the count itself:

| assertion | goes red when |
|---|---|
| `TotalRows > 0` | the table yielded nothing to walk |
| `SpawnableRows > 0` | the type filter matched nothing ⇒ **the gate checked NOTHING** |
| `SpawnableRows + ExcludedRows == TotalRows` | a row was **silently dropped** from the gate (partition is total) |
| `ProbesExecuted == SpawnableRows` | rows were **skipped rather than passed** |
| `DistinctPaths == SpawnableRows` | two rows compose one path ⇒ a card silently borrowing another's actor |

⭐⭐ **AND THEY PIN RELATIONSHIPS, NOT LITERALS (`SC-§40` cl. 10).** ⛔ **I deliberately did NOT write `TestEqual(SpawnableRows, 22)`.** That would rebuild the `TASK-874` trap *inside the very gate written to close it*: card #23 would turn the file red **for the wrong reason**, and the obvious "fix" would be to bump the number. Every assertion above holds however the roster grows and **still goes red on an empty or truncated walk** — plus the positive control (§5) independently proves the walk reached real data. **Three independent tells for one vacuous-green.**

⚠️ **HONEST SCOPE ON ONE OF THEM (`SC-§49`), declared rather than dressed up:** as the loop is written **today**, `ProbesExecuted` and `SpawnableRows` **cannot diverge**, so that row discriminates **nothing on this diff**. It is a **tripwire for the next edit** — the cheap-looking `continue` between "count it as spawnable" and "probe it" is exactly how a roster gate quietly stops probing. Said so in the code comment too.

---

## 5. `SC-§39` — BOTH CONTROLS, AND THE NEGATIVE ONE RUNS ON **EVERY** PASS

- **POSITIVE — `Sorcerer`:** asserted (a) **reached by the walk** and (b) **RESOLVED**. Without it, *"nothing is missing"* and *"the probe is blind"* are the same answer.
- **NEGATIVE — `ZzNoSuchCardZz`:** asserted (a) **not a real table row** and (b) its composed path **does NOT resolve**.

⭐⭐ **The negative control is the load-bearing half and it is worth stating plainly for the gate: it executes on every single green run.** ⇒ **every green this file ever reports carries, inside itself, a live demonstration that the existence probe is still capable of returning ABSENT.** If the probe ever silently starts answering "present" for everything, **that assertion goes red.** ⛔ A gate that has only ever been seen passing is indistinguishable from no gate — this one cannot enter that state without failing.

⚠️ Declared cost: if `Sorcerer` is ever retired from the game the positive control goes red and a new one must be chosen. That is what a control costs and it is written in the code comment.

---

## 6. ⛔⛔⭐ SCOPE — PUBLISHED UNDER THE PREDICATE I MEASURED (`SC-§49`)

**The predicate, quoted verbatim from `TASK-947` item (1) and pinned in the test's own header comment:**

> *"WALK EVERY `DT_Cards` ROW OF A SPAWNABLE `CardType` (Unit · Economy · Building) AND ASSERT ITS COMPOSED CONVENTIONS PATH RESOLVES."*

⭐ **THIS GATE PINS THE ASSET SIDE AND ONLY THE ASSET SIDE:** *"the asset the law names exists at the path the law names."*

⛔⛔ **IT DOES NOT DETECT A COMPOSER DRIFT.** If the shipped `ResolveCardActorClass` changes **how** it builds the string — a renamed folder, a dropped `BP_Unit_` prefix, a lost `_C` — this file keeps composing the CONVENTIONS path independently, keeps finding the assets, and **stays green while every card in the game fails to spawn.**

⚖️ **THE MITIGATING MEASUREMENT, which is why that residual is acceptable rather than merely admitted:** **a composer drift breaks EVERY card at once**, which is instantly visible to the first person who plays anything. **One absent asset breaks exactly one card**, is invisible to the 21 working ones, and survived a QA declaration, a commit and a playtest. **The silent case is the one this row closes.**

⛔ Both sentences are in the test's own header comment, not only here.

---

## 7. ⛔ THE REFUSED SHAPE — AND WHY I STILL THINK IT SHOULD HAPPEN (item (4)'s escape hatch, used as written)

⛔ **I did NOT extract a shared path-composer out of `SiegePlayerController.cpp`.** It was considered and refused at boarding: it would put me in `TASK-942`'s file and **serialise two independent lanes for no gain today**. Respected in full — see the `git status` in §1.

⭐ **AND I DO THINK IT SHOULD HAPPEN, so I am saying so here rather than doing it:**

> **PROPOSED MANAGER ROW — extract the composition into one shared, headless, pure static** (shape: `static FString ASiegePlayerController::ComposeCardActorClassPath(FName CardID, bool bBuildingActor)`, consumed by both `ResolveCardActorClass` **and** `SiegeCardRosterTest.cpp`). ⭐ **That single change converts this gate from asset-side-only into asset-side **and** composer-side**, because the test would then fail when the composer drifts instead of following it. It is a `SiegePlayerController.cpp` write and must be boarded **after `TASK-942` lands** to keep the lanes disjoint. ⚠️ There is a **third** consumer to fold in at the same time: `SiegeCheatManager.cpp:255` already carries a comment saying it composes *"the SAME unit BP path"* by hand — i.e. **the drift surface is already three copies wide, not two.** ⛔ Not boarded by me; naming it is the whole of my duty here.

---

## 8. `TL-§5c` / `TL-§5b` — THE SUITE, DECLARED

**Fresh census, taken at run time, with its scope and its file count (`TL-§5b` part 2):**

| | declared | files | scope |
|---|---|---|---|
| before this row | **432** | 31 | `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`, needle `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` |
| after this row | **433** | 32 | same |

⭐ **MY DECLARED DELTA: `+1` (one new test in one new file).**

⛔ **`declared`. NOT a pass count. I did not compile and I did not run the suite** (`TL-§5c` cl. 1). ⛔ **No suite ABSOLUTE is carried anywhere in this handoff as an expectation** (`TL-§5b`) — the two figures above are **this tree at this instant**, and under four parallel lanes they are **stale by construction**. ⛔ **Do not reconcile *to* them; re-census.**

⚠️ **Instrument controlled (`TL-§5b` 2c):** at the pinned scope the **bare** `^IMPLEMENT_` needle and the **scoped** one return the **same** number (`432` before / both). The trap is a **SCOPE** problem, not a needle problem; my scope excludes `GitClaudeUnrealTest.cpp`'s `IMPLEMENT_PRIMARY_GAME_MODULE` by construction.

⚠️ **`SC-§41` / census hygiene:** the macro token appears **exactly once at line-start** in the new file (measured: `1`), so a `^`-anchored census counts one test. The token is **not** written in prose anywhere in the file. Test-name and class-name collision census across the whole `Tests/` scope: **`Siegebound.CardRoster`** and **`FSiegeCardRosterSpawnableActorClassPathTest`** appear in **this file only**.

### ⚠️ `TL-§5d` — THE COMMIT HOST, NAMED

⛔ **This file is UNTRACKED and therefore INVISIBLE TO `HEAD`. A green suite would measure the working tree and nothing would ever go red to say so.**
⭐ **HOST: `TASK-949`.** Its `names:`/pathspec block already lists `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardRosterTest.cpp`. ⛔ **If this file does not go in that commit, the gate does not exist.**

---

## 9. ⛔ WHAT QA SHOULD SCRUTINISE (I would start here)

1. ⛔⛔ **§0 first — I did not compile and did not run the compiled test.** Judge the shell mirror on whether it truly mirrors the predicate (§3's derivation table), and **route the compiled execution to `TASK-949` by name**, not to me.
2. ⛔ **The vacuous-green question (§4).** Five count assertions + the positive control. **Check I did not sneak a literal `22` in anywhere** — I claim I did not; verify it rather than take it.
3. ⛔ **The `switch` in `ClassifyRow` has NO `default:` label.** That is deliberate (compile-time `-Wswitch` on a new enumerator + run-time `Unclassified` on an out-of-range byte). ⚠️ **If you think it should have one, say so — but note that adding `default:` DELETES the compile-time tripwire.**
4. ⚠️ **`FPackageName::DoesPackageExist` vs. a load.** I check package existence first and only load the `_C` when the package exists — deliberately, so a known-absent asset costs no failed-load log noise that could perturb the run. Verify that is the right predicate for *"the asset exists at the path the law names."*
5. ⚠️ **Engine-API surface, verified against the UE 5.8 headers, but worth a second eye since I could not compile:** `FPackageName::DoesPackageExist(const FString&)` (PackageName.h:450, no deprecation) · `FSoftObjectPath(const FString&)` **non-explicit** (SoftObjectPath.h:68) · `FObjectPropertyBase::PropertyClass` is a `TObjectPtr<UClass>` so I compare through **`.Get()`** · `TestNotNull(const FString&, const ValueType*)` (AutomationTest.h:2459) · `TestEqual(const TCHAR*, int32, int32)` (:1985) · `FString::Join` (UnrealString.h.inl:2071).
6. ⭐ **`SC-§49`:** the scope sentence is in **both** the handoff (§6) and the test's own header comment. **Confirm it is present AND true.**
7. ⛔ **Zero shipped-`Source/**` edits (§1)** — one `??` line. And note the **git-root prefix caveat** in §1 before writing any pathspec.

---

## 10. ⛔ MEASURED FOLLOW-UPS — **NAMED, NOT BOARDED, NOT BUILT** (item (7))

Out of scope for this row by explicit instruction. Recorded so they have an owner if the manager wants them, and **measured** so nobody re-derives them:

- **`SM_<CardID>` census.** The same walk could assert each spawnable row's mesh. ⚠️ **Do not assume the naming is uniform** — the shipped meshes are `SM_Witch`, `SM_WatchTower` (no `SM_Unit_`/`SM_Building_` prefixes), and buildings and units share one folder. The composition rule is **not** the `BP_` rule and would need deriving before it could be gated.
- **`T_CardArt_<CardID>` census.** ⭐ **Cheaper and stronger than a composed-path guess, because `cards.csv` carries the art path as a DATA COLUMN (`CardArt`)** — e.g. `/Game/UI/CardArt/T_CardArt_Witch.T_CardArt_Witch`. That column can be walked directly, so the gate would assert the **authored** reference rather than a reconstructed one. ⚠️ Note `UCardHandWidget` already **degrades to text-only** on a missing art asset — i.e. **`SC-§50` cl. 2's graceful-degrade hiding pattern is live on that path too.** This is the highest-value follow-up in this list.
- **The composer-drift extraction** — §7 above.

---

## 11. FILES

**Written (1):**
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardRosterTest.cpp` — **NEW**

**Read only, never written:**
`Docs/Data/cards.csv` · `Content/Data/DT_Cards.uasset` (binary probe) · `Content/Blueprints/{Units,Buildings}/` · `SiegePlayerController.{h,cpp}` (`ResolveCardActorClass`, `IsBuildingCard`, `ResolveCardRow` — **by symbol**, `SC-§38`) · `CardRow.h` · `Building.h` · `SummonedUnit.h` · `Tests/SiegeCastleTransformTest.cpp`, `SiegeAssistantSelectionTest.cpp`, `SiegeRespawnLifecycleTest.cpp` (house patterns) · `handoffs/STACK-BUGS-diagnosis.md` §2.2 · `qa/TASK-849.md` `N-5` · `CONVENTIONS.md` · `TASKBOARD.md` · the UE 5.8 engine headers named in §9.5

**Scratchpad only (⛔ outside the repo, never staged):** the shell mirror + the synthetic-row CSV copy.

⛔ **No compile · no editor · no MCP · no Git · no `L_Arena` · no `Content/**` write.**

---

## 12. ⚠️ ONE ROUTING NOTE, NOT A FINDING

Mid-task I received a course correction addressed to the **art-director** (the `Restore Packages` modal, `WITCH-§5` slot layout, the never-save law for `L_Arena`/`BP_FogArea`). **It was not for this row** — I hold no editor or MCP access on `TASK-947`. I acted on none of it and am reporting the mis-route rather than silently absorbing it. ⭐ Its subject matter did land, though: `BP_Unit_Witch.uasset` appeared on disk at 19:02, which is what produced §3's RUN 2.
