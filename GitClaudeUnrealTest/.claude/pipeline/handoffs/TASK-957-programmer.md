# TASK-957 — GAMEPLAY-PROGRAMMER HANDOFF — the card-art roster gate, born green and synthesised red twice

**Gate:** `TASK-958` · **Ship host:** `TASK-961` · 2026-09-03
**Baseline:** `f050caf` · **Files touched: 1, NEW** — `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardArtRosterTest.cpp`

---

## 0. ⛔ WHAT I EXECUTED AND WHAT I DID NOT — ABOVE THE CLAIM (`TL-§5c` cl. 5(a), `SC-§54` cl. 4)

**I EXECUTED:** the four-instrument absence census of the synthetic (both substring directions, every zero positive-controlled) · a proper CSV parse of all 32 `CardArt` cells · a binary probe of `DT_Cards.uasset` · a filesystem bijection census · a **two-polarity mirror of the predicate whose two inverted runs went RED** · the declaration census at both endpoints · `git status` scope sweeps · sha256 digests of the three data deliverables · a brace/paren/`printf`-arity structural check of my own file, positive-controlled against a file that actually compiled.

⛔ **I DID NOT COMPILE AND I DID NOT RUN THE SUITE.** My fence forbids compiling, the editor, MCP and Git writes (the editor is wedged on a modal awaiting Jonathan). ⇒ **nobody has yet seen this file's assertions on a compiled run, green or red.** I say that before anything else. What I did instead — and why I believe it is a better discharge than a one-off transcript — is §3.

⛔ **I TOUCHED NO ASSET AND NO SHIPPED `Source/**` FILE.** Measured, not asserted — §1.

⚠️ **`SC-§55` — MY SESSION-START `gitStatus` SNAPSHOT WAS STALE AND I IGNORED IT.** It described castle-era work (`MI_Castle_Interior_Crumble*`, `SM_Castle*`, `A ` staged entries) from a session weeks old. **Nothing in this handoff derives from it.** Recording the disagreement rather than silently preferring the right one, per cl. 4.

---

## 1. ⛔⛔ THE FENCE, MEASURED — `TASK-958`'s TWO BLOCKER QUESTIONS ANSWERED WITH NUMBERS

### (2) WAS `DT_Cards.uasset` OR `cards.csv` WRITTEN ON DISK? ⛔ **NO.**

| sweep, at my instant | result |
|---|---|
| `git status --porcelain -uall -- Content/Data/ Docs/Data/ Content/UI/` | ⭐ **EMPTY** |
| the same instrument over `Source/` | prints 6 paths ⇒ **it is not blind** (`SC-§39`) |

**Digests, so QA re-verifies rather than trusts me:**

```
edffce064bff1a510c62e1f9b9720a05a16c4b1c5df51fe267a28eb10810252a  Content/Data/DT_Cards.uasset
bf91847aaa91ffac3b90ee11dd4f1e0f15ed79eb49ac3e14227624ea73b7b6be  Docs/Data/cards.csv
cfc6baea25e75f7ed56c107b07abc4faeed9945d90962b46cc55970520f8e761  Content/UI/CardArt/T_CardArt_Sorcerer.uasset
b4f375305aea3ea812e25fb0e30bfabd39f82d21a64fe52b02298c3223fec2ab  Content/Blueprints/Units/BP_Unit_Witch.uasset
```

⭐ The Witch digest is **character-for-character** the one `TASK-949` recorded at four points before the refusal and `TASK-964` re-read. **The refused move still has not happened, under this task ID or any other** (`SC-§54` cl. 1).

⚠️ **THE ONE SCOPED RATHER THAN ABSOLUTE CLAIM, DECLARED SO NOBODY DISCOVERS IT:** `LoadSynchronous()` warms `FSoftObjectPtr`'s **`mutable`** weak cache, so the walk does mutate that cache on the loaded table's rows in memory. It changes **no serialized value**, does **not** dirty the package and **never reaches disk** — and it is exactly what the shipped widget does on every hand refresh. **The claim about the ASSETS AND THE INDEX is absolute.** This is written into the file at the synthesis site too.

### (6) ZERO EDITS TO ANY SHIPPED `Source/**` FILE — ⛔ **TRUE, AND THERE IS A TRAP HERE FOR YOU**

⛔⛔ **`SiegePlayerController.cpp` IS `M` IN THE TREE RIGHT NOW AND IT IS NOT MINE.** `TASK-958` item (6) calls a write to that file a **BLOCKER**, so read this before you run your sweep.

| when | `git status -- Source/` |
|---|---|
| my session start | `M …/Tests/SiegeCardRosterTest.cpp` — **one path** |
| mid-row | **+** `Building.{h,cpp}`, `ClimbableTower.h` |
| my finish | **+** `ClimbableTower.cpp`, **`SiegePlayerController.cpp`** |

⇒ **a concurrent lane (the stack lane) landed in `Source/` while I worked.** My deliverable is the **single `??` untracked path**. Corroboration (⛔ corroboration, **not** proof of a zero diff — `qa/TASK-948.md` §5 makes that distinction and I keep it):

```
grep -rn "TASK-957" Source/          -> 3 hits, ALL in Tests/SiegeCardArtRosterTest.cpp
grep -rl "SiegeCardArtRoster" Source/ .claude/  -> the new file + TASKBOARD.md, nothing else
grep -rl "TASK-964|TASK-947" Source/ -> non-empty  <- the instrument CAN find a marker
```

📌 **`TASK-961`: build your pathspec from your own `git status` and do NOT sweep those five `M` paths in on my account.**

---

## 2. THE DELIVERABLE

`IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiegeCardArtRosterTest, "Siegebound.CardRoster.EveryCardRowResolvesItsCardArtTexture", EditorContext | EngineFilter)` — **776 lines**, namespace `SiegeCardArtRosterTestFixture` (**censused unique** across all 38 fixture namespaces; class name and namespace each appear in exactly 1 file).

### ⭐⭐ THE SCOPE SENTENCE — QUOTED VERBATIM FROM `TASK-957` ITEM (1), NOT RESTATED (`SC-§49` cl. 4(a))

> "⛔ `SHIP-§9c` cl. 2 says ⛔ PREFER THE PREDICATE THE ⛔ CONSUMER ITSELF USES — ⛔ and here the consumer's ⛔ OWN predicate ⛔ IS `Row->CardArt.LoadSynchronous()`. ⇒ ⛔⛔ THERE IS ⛔ NO COMPOSER TO DRIFT (⛔ the path is a ⛔ DATA COLUMN in `DT_Cards`, ⛔ not a string this code builds) ⇒ ⛔ this row carries ⛔ NONE of `TASK-947`'s declared composer-drift residual."

It is in the **file header** and here, with the board's own emphasis marks reproduced — **not one word changed, added or reordered.** The item (2) predicate is quoted the same way.

**It is TRUE, and the invariant that keeps it true is in the file and is checkable:** no line may carry the string-macro-plus-open-paren-plus-quote followed by the game mount point. **Measured 0** in my file, **32** in `SiegePlayerController.cpp` (positive control). ⇒ **zero hand-composed asset paths, and therefore ⛔ no sixth copy for `TASK-959` to find.**

### ⛔ WHAT IT WALKS, AND HOW THE DENOMINATOR WAS DERIVED

**Every row of the table, every card type, no filter** — `CardTable->GetRowNames()`. Card art is a property of a Spell and a HeroUpgrade exactly as much as of a Unit; the sibling's 22-row spawnable filter would have left **ten** cards' faces ungated.

⭐ **THE TABLE COMES OFF `UCardHandWidget`'s CDO, NOT THE CONTROLLER'S — a deliberate strengthening of the dispatch, declared here rather than slipped in.** The dispatch said to read the controller's CDO. I read **both**: the widget's (it *is* the card-art consumer, `SHIP-§9c` cl. 2) and the controller's, and I **assert they are equal**. That buys a real, otherwise-unwatched tell: `/Game/Data/DT_Cards` is written **twice** in shipped C++ (`CardHandWidget.cpp:23` and `SiegePlayerController.cpp:212`), and if one moved, the card FACE would read art from a different table than the SPAWN path resolves against with every existing gate staying green. **If QA rules the widget CDO out of scope it is a two-line change and blocks nothing.**

### THE PREDICATE — the consumer's own two expressions, one function, two callers

```cpp
static ECardArtOutcome ResolveCardArt(const FCardRow& Row, UTexture2D*& OutTexture)
{
    OutTexture = nullptr;
    if (Row.CardArt.IsNull()) { return ECardArtOutcome::UnsetCell; }
    OutTexture = Row.CardArt.LoadSynchronous();
    return OutTexture ? ECardArtOutcome::Resolved : ECardArtOutcome::UnresolvablePath;
}
```

⛔ **THE TWO FINDINGS ARE SEPARATE, WITH SEPARATE REPAIRS**, per item (2): an **UNSET CELL** is a data-entry gap (repair the cell); an **UNRESOLVABLE PATH** is a missing/renamed/unimported texture (repair the asset). Collapsing them is how the second gets "fixed" by editing a cell that was already correct. **Both are asserted per row AND by count.**

⚠️ **DECLARED RESIDUAL (R1):** `UCardHandWidget::ResolveCardArtTexture` is **`private`** (`CardHandWidget.h:280`, inside the `private:` block at `:262`) and instance-scoped ⇒ a test cannot invoke it. What this gate evaluates are the **expressions it is built from**, on the same soft pointer from the same table. Its **third** degrade branch (`!Row`) is not exercised by the walk — it is reported per row as a hard `AddError` instead, which is the stronger treatment. **R2** (a texture that loads but looks wrong) and **R3** (a BP subclass overriding `CardTableAsset` — measured inert: `WBP_CardHand.uasset` carries **0** occurrences of `CardTableAsset`, instrument returns 1 for the header) are in the file header.

---

## 3. ⭐⭐ `SC-§51` cl. 6 — BORN GREEN, AND BOTH REDS DELIVERED

### 3.1 The measured born-green cost, re-measured rather than inherited (`SC-§40` cl. 3)

`TASK-950` said 32/32. **I re-measured at `f050caf` and did not take it:**

| instrument | result |
|---|---|
| `Docs/Data/cards.csv`, proper CSV parse | **32** rows; **0** with an empty `CardArt` cell |
| conformance to `T_CardArt_<CardID>` under the card-art folder | **32/32**; **0** non-conforming |
| distinct `CardArt` paths | **32 of 32** |
| `Content/UI/CardArt/*.uasset` | **32** |
| bijection asset ↔ row | ⭐ **exact, 0 orphans in BOTH directions** (`SC-§57` cl. 3's shape) |
| `DT_Cards.uasset` binary — the source the COMPILED gate reads | `T_CardArt_` **64** · the mount+folder **32** · **0** of the 32 CardIDs missing a `T_CardArt_<CardID>` string |

⇒ ⛔ **32/32 PRESENT. NO FREE RED EXISTS.**

### 3.2 ⛔ BOTH SYNTHESISED REDS — THE EXECUTED MIRROR, EACH NAMING A CardID

⛔ **DECLARED AS A SUBSTITUTE FOR THE COMPILED RUN, NOT AS IT** (`SC-§54` cl. 4). Different program, different language; it reads `cards.csv` + the filesystem because it cannot open a `.uasset` row. Script lives in the scratchpad, **not committed**.

⭐ **Mapping validated against a compiled instrument** the way `TASK-964`'s was: `TASK-949`'s compiled run printed **`32 row(s) read`**; my mirror reads **32**; the filesystem holds **32**; the asset carries all 32 by name. **Four sides agree.**

```
INPUTS
  rows              32
  positive control  Sorcerer -> /Game/UI/CardArt/T_CardArt_Sorcerer.T_CardArt_Sorcerer
  red subject       Witch    -> /Game/UI/CardArt/T_CardArt_Witch.T_CardArt_Witch
  synthetic absent  /Game/UI/CardArt/T_CardArt_SorcererQqSyntheticAbsentQq   (DERIVED from the
                    positive control's authored path, NOT composed)
  DoesPackageExist(positive control) = True
  DoesPackageExist(synthetic absent) = False

RUN 1 - THE SHIPPED POLARITY (what the gate actually asserts)
  32 row(s) read; 32 probe(s) executed; 32 RESOLVED; 0 UNSET cell(s); 0 UNRESOLVABLE path(s); 32 distinct art path(s)
  => RUN 1 VERDICT: GREEN

RUN 2 - SYNTHESISED FAILURE (a): an UNSET CardArt cell
        in-memory copy of the 'Witch' row only; cards.csv and DT_Cards.uasset are NOT touched
  32 row(s) read; 32 probe(s) executed; 31 RESOLVED; 1 UNSET cell(s); 0 UNRESOLVABLE path(s); 31 distinct art path(s)
  FAIL  card 'Witch' - its DT_Cards CardArt cell is UNSET; the card face degrades to TEXT-ONLY
  => RUN 2 VERDICT: RED

RUN 3 - SYNTHESISED FAILURE (b): a BOGUS but WELL-FORMED CardArt path
        in-memory copy of the 'Witch' row only; cards.csv and DT_Cards.uasset are NOT touched
  32 row(s) read; 32 probe(s) executed; 31 RESOLVED; 0 UNSET cell(s); 1 UNRESOLVABLE path(s); 31 distinct art path(s)
  FAIL  card 'Witch' - CardArt '/Game/UI/CardArt/T_CardArt_WitchQqSyntheticAbsentQq' does NOT resolve to a loadable UTexture2D; the card face degrades to TEXT-ONLY
  => RUN 3 VERDICT: RED

RUN 4 - SELF-CONTROL ON THE HARNESS (SC-39: prove this reporter can print BOTH words)
  PASS  SYNTHETIC - a check wired to pass on purpose
  FAIL  SYNTHETIC - a check wired to fail on purpose

NEGATIVE CONTROL (the load-bearing assertion), MIRRORED
  resolve('/Game/UI/CardArt/T_CardArt_SorcererQqSyntheticAbsentQq') = ABSENT   [synthetic, measured absent]
  resolve('/Game/UI/CardArt/T_CardArt_Sorcerer') = PRESENT   [positive control]
  DISCRIMINATION - one probe, two different answers in one run: True

SUMMARY  run1=GREEN  run2=RED  run3=RED
  RUN 2 named 1 CardID(s): Witch
  RUN 3 named 1 CardID(s): Witch
```

⭐ **RUN 4 matters:** without it, RUN 2/3's `FAIL` could be a reporter that cannot print `PASS`. The harness is positive-controlled in both directions.

**And the tree afterwards** — `git status --porcelain -- Content/Data/ Docs/Data/ Content/UI/` → **empty**, digests above unchanged. The syntheses ran on **`dict` copies** in the mirror and on **`FCardRow` stack copies** in the compiled test.

### 3.3 ⭐⭐ THE HALF I BELIEVE IS STRONGER THAN THE TRANSCRIPT: BOTH REDS ARE ALSO **PERMANENT AND COMPILED**

⛔ A transcript decays into a screenshot (`SC-§54` cl. 3(c)). So both failure modes are rebuilt **inside the test**, driving the **same `ResolveCardArt`** the walk just used, on **stack copies of a real row** (the donor is the **positive control** — the one row the test has already proven resolves, making the three states a single-variable A/B/C on one known-good input):

| state | input | asserted outcome |
|---|---|---|
| C (control) | the donor **untouched** | `RESOLVED`, texture non-null |
| A | `CardArt` **cleared** | `UNSET CELL`, texture null |
| B ⭐⭐ | `CardArt` → **measured-absent, well-formed** path | `UNRESOLVABLE PATH`, texture null |

⛔ **THE ASSERTIONS ARE INVERTED, NOT RE-RUN.** They assert the **miss**, so the row is **GREEN, and green BECAUSE the predicate answered MISS**. **No `AddError` fires on an expected miss** — every `AddError` in the file is a self-check failure or a real roster defect. *A control that turns the suite red is a bug wearing a control's clothes.*

⛔ **The bogus path is DERIVED, never composed:** the donor's own `GetLongPackageName()` + `GetAssetName()` plus the suffix ⇒ same folder, same prefix, guaranteed well-formed, and **no path literal enters the file**. If the card-art folder ever moves, the control moves with it.

### 3.4 The synthetic name — measured absent in BOTH substring directions

⚠️ **The trap that nearly caught `TASK-964`: a name that is a SUBSTRING of something already in the tree cannot be proven absent by the grep used to prove it.** I chose `QqSyntheticAbsentQq` and proved it both ways.

| needle | files / hits, repo-wide (`.git`, `Binaries`, `Intermediate`, `DerivedDataCache`, `Saved` excluded) |
|---|---|
| ⭐ `QqSyntheticAbsentQq` | **0 / 0** |
| `ZzNoSuchCardZz` (the sibling's constant) | 6 / 30 |
| `Sorcerer` (positive control) | 157 / 2189 |
| `T_CardArt_` (positive control) | 86 / 798 |

plus `DT_Cards.uasset` as a binary: synthetic **0**, `Sorcerer` **4**.
**Substring relation, both directions:** `ZzNoSuchCardZz` ⊄ mine and mine ⊄ `ZzNoSuchCardZz`; same for `Sorcerer`, `T_CardArt_`, `CardArt`. ⭐ **The checker itself is positive-controlled** — it correctly reports `ZzNoSuchCard` ⊂ `ZzNoSuchCardZz`, i.e. it *would* have caught the `TASK-964` case.

### 3.5 ⛔ WHAT REMAINS OWED, PLAINLY

⛔ **Nobody has seen this file compiled.** Every API is hand-verified against the installed UE 5.8 headers (§6); **hand-verification is not a compile.** ⛔ It has never run under its own `EditorContext | EngineFilter` runner. ⛔ **`TASK-961` owes both.**

---

## 4. ⛔⛔ THE TELL AUDIT — `SC-§51` cl. 1–4, RUN STRICTLY ON MY OWN FILE

⛔ **I applied cl. 2's strict test — *is there a world in which it ALONE fires?* — and it gave SMALLER numbers than my first draft claimed. The smaller numbers are the true ones, and correcting my own file was cheaper than being corrected.** ⛔ Nothing subsumed was deleted (cl. 2 forbids it); it is labelled **"subsumed, kept for its message"** at its site and **not counted**.

### ⛔ LEDGER (a) — VACUITY: **ONE tell, wearing five messages**

This file has **no type filter**, so the count and positive-control assertions form a single implication chain — each false forces the next false:

`TotalRows > 0` ⟹ `ProbesExecuted > 0` ⟹ `CONTROL WALKED` ⟹ `CONTROL SET` ⟹ `CONTROL RESOLVED`

| assertion | world in which it ALONE fires | counted? |
|---|---|---|
| `TotalRows > 0` | none — it cannot fire while any later member passes | ⛔ subsumed |
| `ProbesExecuted > 0` | none | ⛔ subsumed |
| `POSITIVE CONTROL WALKED` | none — cannot fire while `SET` passes | ⛔ subsumed |
| `POSITIVE CONTROL SET` | none — cannot fire while `RESOLVED` passes | ⛔ subsumed |
| ⭐ **`POSITIVE CONTROL RESOLVED`** | **control walked, cell set, texture will not load** | ✅ **THE LIVE ONE** |

⚠️ **This differs from the sibling's answer (two) and the difference is structural, not stylistic:** `TASK-947` had a spawnable filter, so `SpawnableRows > 0` could fire where `TotalRows > 0` could not. **I have no filter, so that separation does not exist here.** Publishing "two" would have been copying the sibling's homework.

### ⛔ LEDGER (b) — INSTRUMENT CONTROLS (`SC-§39`): **THREE tells**

| control | world in which it ALONE fires | note |
|---|---|---|
| POSITIVE (`CONTROL RESOLVED`) | as above | ⚠️ the **same assertion** as ledger (a)'s live member — counted once in each ledger and **the two ledgers touch at exactly this point**, which is stated rather than hidden |
| SYNTHESISED (a) — a cleared cell classifies `UNSET CELL` | the `IsNull()` branch is "simplified" away while the loader half still works and every row still passes | ✅ independent |
| ⭐⭐ **SYNTHESISED (b) / NEGATIVE** — a measured-absent path must NOT load | **`LoadSynchronous` gone BLIND, answering non-null for everything** — every row green, every count healthy, every partition balanced, ledger (a) silent | ✅ **THE LOAD-BEARING ONE** |

⚖️ **`SC-§51` cl. 3, live on this file: A FULL ROW SET AND A BLIND PROBE PRODUCE THE SAME GREEN. Only the negative control distinguishes them.**
⚠️ **DISCRIMINATION is NOT a fourth tell** — it cannot fire while the three outcome assertions pass. **Subsumed, kept, not counted.**

### ⛔ LEDGER (c) — OTHER PROPERTIES: six, **each passes at `0 == 0`, none may be counted as a vacuity tell** (cl. 1)

probe-per-row · outcome partition · row-name coverage · art-path distinctness · **card-table PARITY between the two shipped CDOs** · ⭐ **NON-DESTRUCTIVENESS** (fires alone if the synthesis ever starts mutating the loaded DataTable instead of a stack copy — the only tell for `SC-§54` cl. 3 compliance at run time).

### ⛔ WHY IT CANNOT PASS VACUOUSLY

Four self-checks return **`false`** rather than continuing (CDO unreachable · `CardTableAsset` renamed/retyped · unset · table won't load · wrong row struct). Past them, a green requires: a live table, a reached positive control whose art **resolves**, a spawn-side/face-side table **agreement**, three **different** outcomes from one predicate in one run, and the donor still intact afterwards. ⛔ An unreadable instrument and a healthy roster do **not** produce the same bar.

---

## 5. ⚠️ FOR THE MANAGER — A NEW `SC-§56` VARIANT, WORSE THAN THE DOCUMENTED ONE (⛔ I report, I do not edit law)

⛔ **`git ls-tree -r <commit> --full-name -- <root-relative-path>` RETURNS EMPTY WITH EXIT 0 AND AN EMPTY `stderr`.** `--full-name` changes only the **OUTPUT** format; **`--full-tree`** is what makes the **pathspec** root-relative.

| variant | `.cpp` found at `09b9b50` |
|---|---|
| A — cwd-relative pathspec, no `--full-tree` | **32** ✅ (but the output is cwd-relative, so the follow-on `git show` then fails — the *documented* trap) |
| B — root-relative pathspec **+ `--full-name`** (what I ran first) | ⛔ **0** |
| C — root-relative pathspec **+ `--full-tree`** | **32** ✅ (and the output feeds `git show` correctly) |

⛔⛔ **THIS DEFEATS `SC-§56` cl. 3's REMEDY.** The documented trap emits `fatal:` on stderr, so *"capture stderr or check the exit status"* catches it. **Here git is not failing** — a pathspec that matches nothing is a legitimate empty result. **Exit status 0, stderr empty, answer confidently wrong.** ⛔ **Only cl. 5's positive control catches this one.** It handed me a clean-looking `433 → 0` before I controlled it.

⚠️ **And the sting:** `--full-name` is precisely what `TASK-964` added to fix the `git show` half of the same trap. **The fix for one half re-arms the other.** The correct combination is **`--full-tree` for the pathspec**, which also yields root-relative output for `git show`.

📌 Also for the manager, non-blocking: **`SC-§57` cl. 3's corroboration shape held again** — the card-art side is an exact 32↔32 bijection with zero orphans in both directions, which is the *absence* of the discrepancy that section was bought by.

---

## 6. ⛔ ENGINE-API SURFACE — EVERY SIGNATURE VERIFIED AGAINST THE INSTALLED UE 5.8 HEADERS (⛔ not against memory)

| call | verified at | verdict |
|---|---|---|
| `TSoftObjectPtr<T>::LoadSynchronous() const` → `T*` | `SoftObjectPtr.h:547` | ✅ **takes no `LoadFlags`** — this is the consumer's exact call |
| `TSoftObjectPtr<T>::IsNull() const` · `ToSoftObjectPath() const` | `:592` · `:96` | ✅ |
| `TSoftObjectPtr& operator=(const TSoftObjectPtr&)` · `operator=(FSoftObjectPath)` | `:185` · `:417` | ✅ both used |
| `FSoftObjectPath(const FString&)` — **non-explicit** | `SoftObjectPath.h:68` | ✅ house-proven in the sibling |
| `GetLongPackageName()` · `GetAssetName()` · `IsNull()` · `operator==`/`!=` | `:252` · `:265` · `:342` · `:384`/`:387` | ✅ |
| `FindFProperty<FSoftObjectProperty>` + `PropertyClass.Get()` + `ContainerPtrToValuePtr<T>(const UObject*) const` | `UnrealType.h` | ✅ byte-identical to the sibling, which compiled at `09b9b50` |
| `TestTrue/TestFalse(const FString&, bool)` · `TestNotNull(const TCHAR*/FString&, const ValueType*)` · `TestNull(const FString&, const void*)` · `TestEqual(const TCHAR*, int32, int32)` · `TestEqual(const TCHAR*, const FString&, const FString&)` | `AutomationTest.h:2605`/`2369` · `2449`/`2459` · `2532` · `1985` · `1997` | ✅ all present, no ambiguity |
| `AddExpectedMessagePlain(FString, ELogVerbosity::Type, MatchType, int32)` | `:1796` | ✅ **already in the executed suite** — `SiegeAssistantSelectionTest.cpp:1478` uses the identical 4-arg shape with `Occurrences -1` |
| `StaticEnum<ECardType>()` · `GetNameStringByValue(int64)` | shipped precedent, `SiegeAssistantSelectionTest.cpp` | ✅ |

⛔ **Zero deprecated or removed APIs.** ⚠️ **Hand-verification is not a compile.**

### ⭐ The one non-obvious call, and why it cannot itself turn the suite red

The synthesis's deliberate failing load emits an engine warning. **Traced at source:** `TSoftObjectPtr::LoadSynchronous` → `FSoftObjectPath::TryLoad(nullptr, LOAD_None)` → `StaticLoadObject` → `SafeLoadError`, which logs at **`ELogVerbosity::Warning`**, *never* Error, unless `-TREATLOADWARNINGSASERRORS` is on the command line. The framework maps a captured Warning to `EAutomationEventType::Warning`, and `bElevateLogWarningsToErrors` **defaults to `false`** with **no project override** — so it would not fail the test even undeclared.

It is nevertheless declared with `AddExpectedMessagePlain`, and **two deliberate choices make that line safe**:
1. ⛔ the pattern is **my own derived suffix**, not the engine's prose (`SC-§38`) ⇒ an engine message reword cannot break this file;
2. ⛔ `Occurrences = -1`, which `AutomationTest.h:1780` documents as *"occurrences of this message will be silently ignored"* and which `HasMetExpectedMessages` (`:1808-1850`) handles by doing **nothing** ⇒ **if the message never appears, nothing fails.**

⚠️ **Also verified `InOuter == nullptr` on that path**, so `FLinkerLoad::AddKnownMissingPackage` is **not** reached and the failing load leaves **no global state behind**.

---

## 7. ⛔ THE DELTA — `+1` DECLARATION, `+1` FILE (`TL-§5b`/`§5c` cl. 5(a))

Scope `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`, needle `^IMPLEMENT_SIMPLE_AUTOMATION_TEST`.

| | declarations | files |
|---|---|---|
| `09b9b50` (last **EXECUTED**), re-derived by me — **32 joined, 0 `show` failures, 0 empty blobs** | 433 | 32 |
| worktree **before** me (`TASK-964`'s `+1` already in) | 434 | 32 |
| worktree **now** | **435** | **33** |
| ⭐ **MY DELTA** | **+1** | **+1** |

⛔⛔ **`435` IS A DECLARATION CENSUS, NOT A PASS COUNT. I DID NOT RUN THE SUITE.** The last **EXECUTED** figures remain `TASK-949`'s **`433 Result={Success}` / `0 Result={Fail}`** at `09b9b50` — ⛔ **stale by construction; re-census, do NOT reconcile to it.**

⛔ **`TL-§5d`: the file is UNTRACKED. The commit host is `TASK-961`, and its item (3) pathspec must take `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardArtRosterTest.cpp` or ⛔ the gate does not exist and nothing will ever go red to say so.** ⚠️ Derive that spelling from your own `git status` — **the git root is one level above the project dir** (`qa/TASK-948.md` `W-3`).

---

## 8. ⚠️ SELF-REPORTED — THREE THINGS I GOT WRONG AND FIXED BEFORE SUBMITTING (`SC-§29`)

1. ⛔ **I published a tell count that was too high** and corrected it (§4). The first draft claimed one vacuity tell *and* separate positive-control tells; strict cl. 2 shows all five collapse into one chain. **Scrutinise my arithmetic; it is the item `TASK-958` (3) will audit.**
2. ⛔ **A self-referential grep gate.** I wrote the no-path-literal invariant as a runnable one-liner in the file header, and **the paragraph then matched itself** — the check reported 1 and the only hit was the sentence asserting it was 0. Restated in words, with the reason recorded in the file, because *a grep gate whose pattern appears in its own documentation can never read clean.*
3. ⛔ **Two imprecise claims a reader would have grepped and found false:** *"there is no `/Game` literal anywhere in this file — grep it"* (a bare grep returns 7, all comments) and *"there is no `32` in this file"* (a `TEXT()` message said "the 32-row walk"). Both restated as checkable invariants, and the message no longer carries a roster size.

---

## 9. FOR QA (`TASK-958`) — SCRUTINISE THESE, IN THIS ORDER

1. ⛔⛔ **The `SiegePlayerController.cpp` `M` in the tree is NOT MINE** (§1). Your item (6) calls it a blocker; the evidence that it is a concurrent lane's is the status timeline plus a marker census. **Please rule on it explicitly so `TASK-961` is not left guessing.**
2. ⛔ **The widget-CDO decision** (§2). It is **wider than my dispatch asked for** and I declare it as a strengthening, not a slip. If you rule it out of scope it is a two-line revert.
3. ⛔ **My tell arithmetic** (§4) — specifically the claim that `CONTROL WALKED` and `CONTROL SET` are **subsumed by** `CONTROL RESOLVED`. If that is wrong, my published ledger is wrong in the safe direction (I under-claim), but it should still be right.
4. ⛔ **The `LoadSynchronous` cache caveat** (§1). *"No asset was written"* is blocker-grade and I have scoped it rather than stated it absolutely. Rule on whether the scoping is honest or an escape hatch.
5. **The `AddExpectedMessagePlain` line** (§6). It is the only call in the file without a byte-identical shipped precedent for its **argument values** (the shape is precedented; `Occurrences -1` is too, at `SiegeAssistantSelectionTest.cpp:1478`).
6. **Vacuity:** the test can only pass with a live table, an agreeing pair of CDOs, a reached-and-resolved positive control, three *different* predicate outcomes in one run, and an unmutated donor afterwards. **I claim no more tells than §4 lists.**

**⭐ NOT A FINDING — ruled at boarding, please do not flag it:** `CardTypeSymbol` and `FindSoftObjectField` are **deliberate duplicates** of helpers in `SiegeCardRosterTest.cpp`. Extracting a shared header would put this row inside a file **`TASK-959` is rewriting**; the identical ruling was made for `TASK-962` item (7), and `TASK-963` item (7) is instructed not to flag it. The **namespaces** are what keep the two sets apart in a unity build — ⛔ do not flatten them.

**⭐ FOR `TASK-959` in one line:** I added **ZERO** copies of any composed asset path. The no-path-literal invariant measures **0** in my file against **32** in `SiegePlayerController.cpp`. My file is not a sixth site and does not need re-pointing.
