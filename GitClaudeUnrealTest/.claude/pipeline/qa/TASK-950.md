# QA Report — TASK-950 — THE DECLARED-GAP CENSUS

**Verdict: PASS (census delivered)** — this row produces a **list for the manager**, not a gate over a diff.

---

## ⭐ THE ANSWER, FIRST

> ### **1 LIVE ORPHAN. 0 of the Witch's severity. And the reason there is only one is that the board caught up TODAY — 5 of the 6 known gaps were homed within the last few hours.**

**THE WORST INSTANCE IS STILL `BP_Unit_Witch` ITSELF** — and it is now **HOMED** (`TASK-946`/`947`/`948`/`949`). It remains **absent on disk**, measured by me at this instant. Nothing else in 799 files reaches its severity.

**THE ONE LIVE ORPHAN:** `Tools/ArtPipeline/cardart_render.py` — declared absent in **two** QA reports, named in `CONVENTIONS.md:83` as *"NEW, named here before the task is issued"*, **still absent**, **no row owns it**, and it carries a **🧑 Jonathan ruling from 2026-08-01** that has gone unexecuted for a month. ⛔ **It is NOT Witch-severity: nothing is unplayable.** All 32 card faces exist and render.

**THE ONE THAT WORRIES ME MORE THAN ITS BOARD STATUS SUGGESTS:** `SK_Witch` — *homed*, but sequenced behind **two open rows** (`886(8) → 899 → 863 → 907`). When `TASK-946` lands, she becomes **the only one of 14 spawnable units with no skeletal mesh** — she will spawn **static and unanimated**, and per `WITCH-§9.0` the cast animation is now the **ONLY** tell that a cast is running, because the cast bar was deliberately reversed out. ⚠️ **Homed is not shipped, and this one is three rows deep.**

---

## ⛔ SCOPE + COUNT, PUBLISHED TOGETHER (`SC-§49`, `TL-§5`)

**Predicate, quoted verbatim from board item (2) — NOT restated in tidier words:**

> *"a statement that a **DELIVERABLE THE FEATURE NEEDS IN ORDER TO FUNCTION DOES NOT EXIST** (shapes: "does not exist" · "not yet authored" · "missing" · "TODO in TASK-###" · "will be added by" · "degrades to")"*

**Scope:** `.claude/pipeline/qa/*.md` = **177 files** · `.claude/pipeline/handoffs/*.md` = **622 files** ⇒ **799 files total.** Date range: the **full history of both directories**, `TASK-001` (M1) through `TASK-934` / `TASK-925` (2026-09-03). No date filter was applied.

| Needle | `qa/` | `handoffs/` | Total |
|---|---|---|---|
| exist-family (`do(es)? ?n(o\|')t (yet )?exist`) | 64 occ / 41 files | — | — |
| `not yet authored` \| `never authored` \| `not authored` \| `will be added by` \| `degrades to` \| `TODO in TASK` | 21 occ / 19 files | — | — |
| **both of the above, combined** | **85 occ** | **203 occ / 146 files** | **288** |
| `missing` (alone — the widest shape) | 198 occ / 94 files | 412 occ / 233 files | **610** |
| **RAW HIT SET UNDER THE FULL PREDICATE** | | | **⛔ 898 occurrences across 799 files** |

⛔ **898 is the RAW shape-count, NOT the orphan count.** The `SC-§50` cl. 5 predicate is far narrower than its own trigger shapes — the overwhelming majority of the 898 are absences of *defects*, *laws*, *comments*, *test rows* and *fallback paths*, not of deliverables. **`SC-§50` cl. 5 explicitly excludes style and wording**, and a bloated list is worse than a short true one.

**⛔ SIZE VALVE INVOKED (item 7), AND DECLARED:** I **fully resolved the SPAWNABLE-ASSET species** (below — exhaustively, all three composed-path families, by measurement). I **did not** individually adjudicate the ~610 `missing` hits or the pre-`TASK-400` historical tail beyond the asset cut. **What I left unresolved is named in §6.**

---

## ⛔ CONTROLS, BOTH DIRECTIONS (`SC-§39`) — THE INSTRUMENT WAS WRONG ON ITS FIRST DRAFT

### ✅ POSITIVE CONTROL — **AND IT CAUGHT A BROKEN NEEDLE BEFORE I TRUSTED A SINGLE ZERO**

`qa/TASK-849.md:173`, verbatim:

> *"`BP_Unit_Witch` and `T_CardArt_Witch` **do not exist yet** (`TASK-835` / `TASK-834`; the card face **degrades to** text-only, never a crash)."*

⛔⛔ **THE KNOWN CASE READS *"do not exist yet"* — NOT *"does not exist"*.** A needle written to the shape the spec lists first (`does not exist`) **would have MISSED the one instance I was handed to prove the instrument works**, and would have returned a confident, wrong, low number. The needle was widened to `do(es)? ?n(o|')t (yet )?exist` **before** any count was taken. ✅ **Confirmed present in the hit set: 1 occurrence, both in the broad needle and in the narrow asset-species cut.**

### ✅ NEGATIVE CONTROL — a report I **READ**, whose hits I **adjudicated one at a time**

`qa/TASK-914.md` — **7 hits on the WIDEST needle** (incl. `missing`/`absent`), **ZERO `SC-§50` predicate hits.** Each read and ruled:

| Line | Text | Why it is NOT a declared gap |
|---|---|---|
| `:135` | *"fails loudly when **either** needle is absent"* | a test's failure mode |
| `:165` | *"`static` … **absent** in the definitions"* | correct C++ style |
| `:198` | *"It is **NOT a missing test**. Nobody should go hunting."* | an explicit **refutation** |
| `:222` | *"**missing** the real corner by 29%"* | a geometry error |
| `:244` | *"**ABSENT** FROM THE HANDOFF'S list"* | a documentation omission |
| `:306` ×3 | *"missing filter" · "missing good" · "absent good"* | abstract detection theory |

⭐ **This is the stronger form of a negative control: the needle FIRES SEVEN TIMES in a file with no true gap.** It proves the instrument is **live** (not dead-and-silent) *and* that the narrowing from 898 → the asset species is doing real work rather than hiding hits.

### ⚠️ A THIRD INSTRUMENT FAILURE, CAUGHT AND RECORDED

`Grep` rendered `SiegePlayerController.cpp:1801` and `:1804` as `\ QA-BINDING…` — **a leading backslash where the source has `//`**, which would be a compile error. I opened the file at source: **it is `//`, the code is fine, the tool's rendering lied.** ⛔ Recorded because it is this batch's own lesson — *a tool's output is a citation; the file is the measurement* (`SC-§40` cl. 1). **I did not report a phantom blocker.**

---

## 1. ⛔ THE `SC-§50` cl. 4 MECHANISM, RUN BY HAND — THREE COMPOSED-PATH CENSUSES

⛔ **I did not assume the composition rule — I read it.** `SiegePlayerController.cpp:4594-4658` (`ResolveCardActorClass` + `IsBuildingCard`) and the header default `BuildingEconomyCardIDs = { "DeepMine" }` (`:1663`).

- Building card **OR** Economy-typed building (`DeepMine` only) ⇒ `/Game/Blueprints/Buildings/BP_Building_<CardID>`, must be `ABuilding`
- Unit **OR** Economy ⇒ `/Game/Blueprints/Units/BP_Unit_<CardID>`, must be `ASummonedUnit`

### (a) ⭐ SPAWNABLE-ROSTER CENSUS — **22 rows / 21 resolve / ⛔ 1 does not**

Source: `Docs/Data/cards.csv`, 32 rows. Spawnable = `CardType` ∈ {Unit, Building, Economy} ⇒ **22**. Excluded and named with its count: **10** non-spawnable (5 Spell, 4 HeroUpgrade, 1 Utility).

| Family | Rows | Resolve | Miss |
|---|---|---|---|
| `BP_Building_*` (ArrowTower, Wall, BombTower, BallistaTower, Barracks, CrystalTower, WatchTower, **DeepMine**) | 8 | **8** | — |
| `BP_Unit_*` (Footman, Archer, Knight, MilitiaMob, Pikeman, Sapper, Cavalry, Longbowman, Cleric, Ogre, Wizard, Sorcerer, **Miner**) | 13 | **13** | — |
| `BP_Unit_Witch` | 1 | **0** | ⛔ **WITCH** |

⇒ ⛔⛔ **EXACTLY ONE UNRESOLVABLE SPAWNABLE ROW IN THE WHOLE GAME, AND IT IS THE ONE WE ALREADY KNEW.** ✅ **There is no second Witch.**
📌 This **independently reproduces** `TASK-946`'s own claim (*"The ONLY miss across 22 spawnable rows"*) **to the unit**, derived from the CSV and the filesystem rather than read off the board.

⭐ **AND THE REASON IT HID:** the Witch has `SM_Witch` · `MI_Witch_PBR` · `T_Witch_{D,N,ORM}` · `T_CardArt_Witch` · `Witch.fbx` · `Witch.png` — **everything except the Blueprint.** Every eyeball check and every art-side readback passes.

### (b) CARD-ART CENSUS — **32 rows / 32 assets / 0 missing**

`Content/UI/CardArt/*.uasset` = **32**, one per CSV row **including `T_CardArt_Witch`**. ✅ The other half of the positive-control sentence **was delivered**. This is the species that *"degrades to text-only"*, and it is **clean**.

### (c) ⛔ SKELETAL CENSUS — **14 spawnable unit rows / 13 `SK_*` / ⛔ 1 missing** *(not requested; found on the way)*

`Content/Characters/`: Footman, Archer, Knight, Miner, Sapper, Longbowman, Pikeman, Cavalry, MilitiaMob, Ogre, Sorcerer, Wizard, Cleric = **13**. ⛔ **`SK_Witch` absent.** See §2 row 2 — this is the one I rank highest among the *homed*.

---

## 2. ⛔ THE RESOLVED LIST — report · verbatim sentence · deliverable · measured existence · board row

### 🔴 LIVE ORPHAN — declared gap · deliverable still absent · **NO ROW**

| # | Source | Verbatim | Deliverable | Exists today? | Board row |
|---|---|---|---|---|---|
| **L-1** | `qa/TASK-865.md:153` + `qa/TASK-878.md:44` | *"`cardart_render.py` **does not exist** yet (`CONVENTIONS.md:83`) and should be BORN with this law rather than retrofitted"* / *"glob `Tools/ArtPipeline/*.py` → **14 files**, none of them it"* | `Tools/ArtPipeline/cardart_render.py` | ⛔ **NO** — I re-globbed: **14 `.py`, none of them it** (reproduces `878`'s count exactly) | ⛔ **NONE.** 28 board mentions, **all citations or fences** (`TASK-878`(e): *"`TASK-876` … did **not** create `cardart_render.py`"*). No row assigns authorship. |

⛔ **`CONVENTIONS.md:83` convicts the pipeline in its own words:** *"**Tool (NEW, named here before the task is issued)**"* — the law recorded that the row was never issued, and then nobody issued it.
🧑 **It sits on a Jonathan ruling** (`CONVENTIONS.md:82`, 2026-08-01, `TASK-373`): *"the higher-fidelity face is the new standard and **the other 29 are re-rendered to match**"*. **~1 month unexecuted.**
⚖️ **SEVERITY: MEDIUM, and I will not inflate it.** All 32 faces exist and render; nothing is unplayable and no gold is lost. It is a **fidelity + tooling** debt with a user ruling behind it — **worth a row, not worth a night.**

### 🟠 HOMED, BUT DEEP IN A CHAIN — verify these are actually *moving*, not just *listed*

| # | Source | Deliverable | Exists? | Row | State |
|---|---|---|---|---|---|
| **H-1** | `qa/TASK-849.md:173` ⭐**POSITIVE CONTROL** | `BP_Unit_Witch` | ⛔ **NO** | ✅ **`TASK-946`** (author) · `947` (roster gate) · `948` · `949` (ship) | ▶ **dispatchable now** |
| **H-2** | `handoffs/TASK-833-artist.md:130` · `TASK-923-programmer.md:104` · `TASK-925-buildmaster.md:188` · `TASK-832-artist.md:228` · `TASK-835-buildmaster.md:224` | `SK_Witch` + `A_Witch_Cast` montage | ⛔ **NO** | ✅ `TASK-863` → integration `TASK-907` | ⚠️ **3 rows deep:** `886(8) → 899 → 863 → 907`; `886` is `backlog`, `899` awaits 🧑 `J-W15` |
| **H-3** | `qa/TASK-856.md` `F-1` | the `:492` *"13 `RelatedActionIds`"* sentence (measured **16**) | n/a (prose) | ✅ **`TASK-938` SITE C** — *"**THIRD SITTING UNHOMED, AND IT ENDS HERE**"* | ▶ **dispatchable now.** ⭐ **The launcher's "unhomed across three sittings" is NO LONGER TRUE — it was homed today.** |
| **H-4** | `TASK-835-buildmaster.md:224` · `TASK-832-artist.md:228` | veil **proven on a skeletal mesh in PIE** | n/a (observation) | ✅ `TASK-907` | boarded, blocked-by `TASK-863` — **correctly, since it needs `SK_Witch`** |
| **H-5** | `handoffs/TASK-834-artist.md` §1(b) | the **95.2%-metallic** `SM_Witch` re-bake | asset exists, bake disputed | ✅ `TASK-899` (`J-W15`) | 🧑 **awaiting Jonathan.** Default = ship as-is. |
| **H-6** | `qa/TASK-878.md` §F1 | validate her **Stage-2** `Witch.fbx` + 3 PNGs (through the writer with **zero post-write validation**) | files exist, **unvalidated** | ✅ `TASK-886` **item (8)** | `backlog`, blocked-by `TASK-878` |
| **H-7** | `handoffs/TASK-868-programmer.md` §8 | `SiegeAcquisitionFunnelTest` test 1 compile hazard | n/a | ✅ `TASK-882` | ▶ dispatchable |

### ✅ CLOSED — declared gap, **deliverable measured PRESENT today** (`SC-§40` cl. 1: measured, not believed)

`T_CardArt_Witch` (the *other* half of the control sentence) · `MI_Unit_Invisible` · `M_HeroSpirit` · `BP_Building_CrystalTower` + `SM_CrystalTower` (`qa/TASK-097-report.md:27` — ⭐ **the exact same *"degrades to a temporarily dead card"* shape as the Witch, from M1, and it was closed**) · `M_SpellReticle` · `M_BattlefieldGround` · `SM_Torch` · `SM_WarTable` · `WBP_WarMap` · `IA_WarMap` · `SM_WatchTower` · `MI_WatchTower_PBR` · `BP_Building_WatchTower` · `MI_Castle_Interior_PBR` · `IA_AssistantConsole` · `BP_SiegeGhostPawn` · building meshes/BPs (`qa/TASK-030-report.md:37`).
Also closed: **`SC-§41`** (`qa/TASK-869.md:177`: *"`grep "SC-§41"` … returns ZERO matches — the law **does not exist**"*) ⇒ **now a real section at `CONVENTIONS.md:3807`**, which even records that it *"was CITED AS SETTLED LAW THREE TIMES BEFORE IT EXISTED."* · **Escape on the war map** ⇒ **struck 2026-09-03 on Jonathan's own words** (`TASKBOARD.md:15181`) — ⛔ **not an orphan; do not re-raise it.**

### ⚖️ RULED NOT-NEEDED **IN WRITING** — `SC-§50` cl. 3 satisfied, ⛔ NOT orphans

- **`WBP_AssistantConsole`** — `handoffs/TASK-444-programmer.md:30`: *"🔒 **RESERVED, NOT AUTHORED** (ruling A(c))"*; `TASK-561-programmer.md:88` confirms `ConsoleClass = nullptr` **is** the shipped v1 state. ⭐ **This is exactly the compliant shape the law asks for.**
- **`SK_Ogre_Skeleton`** — `handoffs/TASK-316-artist.md:10`: *"That asset **does not exist and never did**"* ⇒ corrected to shared `SK_Footman_Skeleton`; `TASK-340-artist.md:137` re-states it as a hard rule.

---

## 3. ⛔ FINDINGS RAISED BY THIS CENSUS

- **[WARN] `TASKBOARD.md:15823` (`TASK-899` `blocked-by`) — WRONG CLAUSE CITED, AND THE WRONG ONE READS AS THE OPPOSITE INSTRUCTION.** It cites *"`TASK-886` (**its item (6)** validates her Stage-2 FBX + PNGs)"*. ⛔ **It is item (8).** `TASK-886:15573` says so in its own words: *"the order is **`886(8)` → 899 → 863**"*. **Item (6) is the FENCE** — *"`refine_trellis_glb.py` ONLY … **ZERO `Content/` edits**"* — so a reader who follows the citation lands on a clause **forbidding** the very thing the blocker depends on. **Fix:** manager corrects `(6)` → `(8)` on `TASK-899`'s `blocked-by`. *(`SC-§38`: a rider attached to a **position** does not travel.)*
- **[NIT] `qa/TASK-908.md:203` already found this species once** — *"the 'PRE-EXISTING RED ROW' it routes to `TASK-849`/`TASK-850` **DOES NOT EXIST** on today's tree"*. ⛔ **That is `SC-§50` cl. 3's trap firing in a live report** — a cited `TASK-###` that resolved to nothing. Recorded as corroboration that cl. 3 is load-bearing, not theoretical.
- **[NIT] The `SC-§50` predicate's own listed shapes are incomplete.** `does not exist` misses `do not exist`; the known case uses the latter. **Recommend the law's shape list be amended to the inflected form** `do(es)? ?n(o|')t (yet )?exist`, since the next agent to run this census will copy the shapes verbatim from cl. 3 and get a false low count. *(Raised, not authored — `SC-§27`.)*

---

## 4. ⛔ `SC-§29` COVERAGE LEDGER

**THIS REPORT COVERS:** `TASK-950` **alone** — a read-only census over `.claude/pipeline/qa/*.md` (177) + `.claude/pipeline/handoffs/*.md` (622) + `TASKBOARD.md` + `CONVENTIONS.md` + filesystem existence checks under `Content/**`, `Tools/ArtPipeline/*.py`, `Docs/Data/cards.csv`, and **read-only** reads of `SiegePlayerController.{h,cpp}` to derive the path-composition contract.

**THIS REPORT DOES ⛔ NOT COVER:** any diff · any compile · any suite · `TASK-941`/`942`/`946`/`947`'s work (concurrent) · the Witch asset itself · the stack lane · **and it is ⛔ NOT a gate over any code task.** ⛔ **No file was edited. No row was boarded. No Git operation was performed. The editor (PID 40528) was not touched, no MCP call was made.**

**`TL-§5c` DECLARED FIRST:** ⛔ **no shell, no compile, no editor, no MCP, no Git, no suite run.** ⛔ **Every number here is STATIC and was derived on disk by me at this instant.** ⛔ **No pass count appears in this report because none was measured.**

---

## 5. ⛔ FOR THE MANAGER — ROWS ARE YOURS, NOT MINE (item 6)

1. ⛔ **BOARD `cardart_render.py`** — the one live orphan. Medium severity; carries a 🧑 Jonathan ruling (29 faces owed a re-render). ⚖️ **Or rule it NOT-NEEDED in writing** — `SC-§50` cl. 3 accepts either, and *"we ship token faces"* is a legitimate answer. ⛔ **What it may not do is stay silent, which is what it has done for a month.**
2. ⛔ **CORRECT `TASK-899`'s `blocked-by`: item (6) → item (8).**
3. ⚠️ **WATCH THE `SK_Witch` CHAIN.** `886(8) → 899 → 863 → 907`, and `899` is blocked on a **human ruling**. When `946` ships, she spawns **static and unanimated** with **no cast tell at all** (`WITCH-§9.0` removed the bar). ⭐ **That is a playtest report waiting to be written** — worth telling Jonathan *before* he clicks the card, so the second Witch surprise is one we announced rather than one he found.
4. ✅ **`TASK-947`'s roster gate closes the SPAWNABLE species permanently.** My hand census is a **snapshot**; `947` is the **mechanism**, and `SC-§50` cl. 4 is right that only a mechanism closes this class. ⭐ **It has a free, real, unsynthesised red available right now** (`Witch`) — that red must be **captured before `946` lands**, or the gate ships unvalidated against the one failure it exists to detect (`SHIP-§9`).

---

## 6. ⛔ DECLARED UNRESOLVED (item 7 — a declared partial beats a silent truncation)

- **~610 `missing` hits** (198 `qa/` + 412 `handoffs/`) were **counted, not individually adjudicated.** Sampling across both directories found them dominated by *"missing null check"*, *"a missing bind degrades to…"*, *"missing-file fixture"* and *"NOT a missing test"* — **absences of defects and fallback paths, not of deliverables.** ⛔ **I did not read all 610 and I do not claim they are clean.**
- **The pre-`TASK-400` historical tail** was swept with the exist-family needles (results in §2) but **not** cross-resolved against the board row-by-row; those gaps were instead settled the cheaper and stronger way — **by measuring whether the asset exists today** (all did).
- **Not measured:** whether each existing `BP_*` actually **derives the required base class** (`ABuilding` / `ASummonedUnit`). `ResolveCardActorClass` requires `IsChildOf(RequiredBase)` and returns `nullptr` otherwise — ⛔ **so a BP that exists at the right path but has the wrong parent fails identically to an absent one, and my filesystem census CANNOT see it.** ⭐ **`TASK-947`'s test can and should assert the parent, not merely the path** — flagged for that row.

---

*qa-reviewer · 2026-09-03 · read-only · positive control `qa/TASK-849.md` `N-5` ✅ confirmed in hit set · negative control `qa/TASK-914.md` ✅ 7 wide hits / 0 predicate hits*
