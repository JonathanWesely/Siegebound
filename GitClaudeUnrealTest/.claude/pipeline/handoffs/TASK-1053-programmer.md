# Handoff — TASK-1053 [FOGDOC-TRUTH-2] · gameplay-programmer · 2026-09-05

**Status ⇒ `ready-for-qa`.** Gate: **`TASK-1054`**. Host: **`TASK-1055`** (this row + `TASK-1050`, one commit).
⛔ **Nothing staged. No commit. No `checkout`/`restore`/`stash`/`reset`/`clean` was run.**

---

## 0. SUBJECT AND INSTANT (`SC-§64a` cl. 3)

| item | value | how I know |
|---|---|---|
| `HEAD` | **`c79bf5b`** | `git log --oneline -1`, my own read |
| index | **empty** | `git diff --cached --name-only` ⇒ 0 lines |
| dirty tracked `Source/` | **`FogVolume.cpp`**, **`FogVolume.h`** (mine) + `Tests/SiegeFogRetentionWiringTest.cpp` (**`TASK-1045`'s, not mine**) | `git diff --name-only -- Source/` |
| dirty elsewhere | `CONVENTIONS.md`, `TASKBOARD.md` (board traffic), `.claude/agents/qa-reviewer.md` (🧑 Jonathan's) | ⛔ **none of them touched by me** |
| my write set | **exactly the two `Source/` paths the row NAMED**, plus this handoff and TASK-1053's own `status:` line | below |

⚠️ **DECLARED AGAINST MYSELF — one instrument I do not hold:** I did **not** open the editor and did **not**
run MCP. §4's "zero Blueprint children" method is a **disk scan**, not a live engine query; `SC-§78` still
applies and I say so in the comment itself.

---

## 1. THE `.cpp` SITE (clauses 0–5) — the paragraph, replaced not deleted

`AFogVolume::Find`, the comment above its `return Volume;`. **The invariant was kept; only its ground was
replaced.** Old text (deleted):

> *"TActorIterator's order is stable for a fixed world, so a level that ALSO placed a BP_SiegeFog answers the
> same actor on every call rather than churning between two."*

New text, in full:

```
// Exactly one instance exists by construction — FindOrSpawn is the ONLY spawn site in the
// project and it LOOKS BEFORE IT CREATES, so the first valid hit is the whole answer.
//
// ⛔⛔ CORRECTED 2026-09-05 (TASK-1053) — THE INVARIANT ABOVE IS TRUE; THE GROUND THIS
// COMMENT USED TO GIVE FOR IT WAS FALSE. It said TActorIterator's order is stable "so a
// level that ALSO placed a BP_SiegeFog answers the same actor on every call rather than
// churning between two" — i.e. it TIE-BROKE BETWEEN TWO CANDIDATES IN A WORLD WHERE THE
// SECOND ONE CANNOT EXIST. This loop is TActorIterator<AFogVolume>, and BP_SiegeFog's
// parent is BP_FogArea_C (read back live under TASK-1043, committed ef2c901) ⇒ it is NOT
// an AFogVolume subclass and THIS ITERATOR NEVER SEES IT. ⛔ There is no tie to break —
// not a tie that happens to break stably.
//
// ⚠️⚠️ WHAT WOULD COMPETE, AND IT IS THE HALF WORTH READING: a Blueprint child of
// AFogVolume ITSELF. TActorIterator matches SUBCLASSES, and FindOrSpawn calls Find BEFORE
// it spawns ⇒ a level-placed child would be RETURNED FROM HERE AS THE ONE AUTHORITATIVE
// FOG-STATE ACTOR, with its own overrides of the five EditDefaultsOnly tunables IN FORCE,
// and the native spawn in FindOrSpawn would never run.
// ⛔ The AUTHORITATIVE statement of the parent fact and of the DORMANT-with-a-live-wire
// ruling over that hazard is the `CoreRedirects` paragraph in the FogVolume.h class doc —
// read it THERE rather than re-deriving it here, and ⛔ IF THIS COMMENT AND THAT PARAGRAPH
// EVER DISAGREE, THE HEADER WINS.
```

**Clause-by-clause:**

- **(2)(i)** *"FindOrSpawn is the ONLY spawn site … and it LOOKS BEFORE IT CREATES"* — ✅ present, first sentence.
- **(2)(ii)** ✅ *"it is NOT an AFogVolume subclass and THIS ITERATOR NEVER SEES IT"* — the invisibility is
  attributed to **class matching**, ⛔ explicitly **not** to stable ordering (*"not a tie that happens to break
  stably"*).
- **(2)(iii)** ✅ the live wire is named as the half worth reading: subclass matching + `Find`-before-spawn ⇒
  **returned as authoritative**, **five tunables in force**, **native spawn never runs**.
- **(2a)** ✅ **the word "safe" does not appear, and no variant of *"so placing one is safe"* was written.**
  The paragraph says nothing about whether placing anything is advisable; it says only what the iterator does
  and does not see, and what a subclass would do if one existed. **Why, not whether.**
- **(3)** ✅ the header is **cited in one clause** and its argument is **not copied** — no serialisation
  enumeration, no `Blueprintable`-inheritance chain, no DORMANT-vs-RETIRED reasoning is restated in the `.cpp`.
  ⚖️ **The precedence sentence is explicit and is repeated here for the record: IF THIS COMMENT AND THAT
  PARAGRAPH EVER DISAGREE, THE HEADER WINS.**

---

## 2. THE HEADER FINDINGS (clauses 6–11), one row each

All six are from `qa/TASK-1051.md`, all comment text, all located **by text**.

### (6) WARN-1 — the miscount that its own writing caused. ⛔ **NOT "change 1 to 4".**

Old: *"(The **single** textual `UFUNCTION` in this header is the comment on `BrightSunWindowSeconds` saying it
is NOT one.)"*

**My replacement states the PREDICATE, and contains no number:**

> *"(⛔ A grep for `UFUNCTION` in this header returns ⛔ PROSE ONLY — this paragraph and the comment on
> `BrightSunWindowSeconds` saying it is NOT one are among the hits — and ⛔ ZERO of them is a DECLARATION.
> ⛔ The zero is the whole finding. ⛔ Stated as a PREDICATE rather than as a COUNT on purpose: the wording
> that stood here before 2026-09-05 asserted a specific NUMBER of textual hits and was ⛔ FALSIFIED BY THE ACT
> OF WRITING IT — the same diff added further occurrences, one of them INSIDE the sentence itself
> (`qa/TASK-1051.md`; ⭐ `SC-§91`). ⛔ A count in a comment goes stale on the next edit, ⛔ including its own;
> a predicate does not.)"*

Three deliberate properties, so QA can check them cheaply:
1. **No number.** Not `1`, not `4`, not "several".
2. **"among the hits"**, ⛔ not "these are the hits" — the enumeration is a **sample**, so adding prose later
   cannot falsify it either. It is stale-proof in the same axis the old one failed on.
3. ⛔ **I did not re-litigate WARN vs BLOCKER.** `SC-§91` records QA's reasoning; I cite it and move on.
4. ✅ **Side-effect check, because this is the exact trap the sentence is about:** the header's textual
   `UFUNCTION` count is **unchanged by my diff** — same four lines before and after (now `:199 :201 :212
   :410`). ⇒ **no structural test that counts them can move.** *(I verified this; I did not write the number
   into the file.)*

### (7) WARN-2 — stale on a different axis (`FogVolume.h:18-21` at dispatch)

Old: *"The fog you can SEE is `TASK-841`'s and it is **still premise-blocked on `TASK-836`**."*
**Both halves re-derived at my own instant:** `TASK-836` = **DONE 2026-09-03** (board heading, my own grep);
`ef2c901` = *"TASK-1043: the fog gets a face - BP_SiegeFog ships, integration-checked (TASK-841)"*, commit
date **2026-09-05** (`git show -s --format=%cs`). New text says the visual **HAS SHIPPED**, dated, as
`/Game/Blueprints/BP_SiegeFog`, a child of the **VENDOR** `BP_FogArea`, **with no C++ spawner yet** — and
records **why the sweep passed over it** (claim-shaped predicate vs a sentence stale on a different axis),
pointing at `SC-§91`.

### (8) WARN-3 — *"harmless here"* ⇒ the narrow claim

Old: *"Placing one is indeed **harmless here**, but for the OPPOSITE reason…"*
New says the narrow thing and **refuses the reassuring reading**: it *"cannot reach fog **STATE** — ⛔ and that
is the NARROW claim, ⛔ about **ONE FUNCTION**, ⛔ **not a licence to place one**"*, then names the other face
(a level-placed visual with no state actor behind it is `TASK-998`'s *"renders and clamps nobody"* inverted).
⛔ **`TASK-1050`'s two clauses in the same sentence — *"not 'found too', it is never found at all"* and *"what
WOULD be found is a level-placed Blueprint child"* — were kept, unedited.**

### (9) NIT — the `ini` leg was under-stated

`config=Engine` is on the **`AActor` `UCLASS` line the ruling two paragraphs below already quotes**, and it is
**inherited** ⇒ the class **already has a config home**. New text: the `.ini` route is barred by
**EXACTLY ONE MISSING SPECIFIER, not by two** — the **per-property `Config`** — and this **STRENGTHENS**
DORMANT (a **second** live wire), rather than weakening it.

### (10) NIT — the heading

Old: *"⛔ SPAWNED AT RUNTIME, **NEVER LEVEL-PLACED**"* sitting directly above two paragraphs about somebody
level-placing a child. New: *"THE NATIVE ACTOR IS SPAWNED AT RUNTIME AND IS NEVER LEVEL-PLACED BY US — ⚠️ a
statement about how the ONE instance **ARRIVES**, ⛔ NOT a guarantee that nothing of this class can be placed
in a map; the two paragraphs below are about exactly that case."*

### (11) 🚨 THE UNMETHODED **"MEASURED"** — ✅ **METHOD RECORDED, ⛔ NOT DOWNGRADED**

The row permitted downgrading to *"asserted, method not recorded"*. **I did better: I reconstructed a real,
reproducible instrument and wrote it into the comment.** The method, verbatim in substance:

- **WHAT:** a **byte scan of every `.uasset` and `.umap` under `Content/`** for the FName **`FogVolume`** — the
  string a Blueprint child **must** carry, because its parent class is an **import**
  (`/Script/GitClaudeUnrealTest.FogVolume`) and an **uncooked** package stores import names in a **plain name
  table**. ⇒ **NOTHING matched.**
- 🚨 **WHY THE INSTRUMENT IS BELIEVED — validated against the FAILURE it detects, not merely against success
  (this is the part QA should press on, and the part that makes the null result admissible):**
  `BP_HeroCharacter` **does** carry `HeroCharacter` and `BP_CommanderNpc` **does** carry `CommanderNpc`
  (native parents); `BP_SiegeFog` **does** carry `BP_FogArea` (Blueprint parent). ⇒ **A null result from THIS
  scan is evidence.** ⚠️ `qa/TASK-1051.md` §7 was right that *"a text grep over binary/LFS `.uasset` files
  returning nothing is **not** evidence"* — **as a bare grep.** ⛔ What makes it evidence here is the
  **positive control**, plus the fact that these `.uasset` files are **real packages in the working tree, not
  LFS pointers** (I checked the header bytes: magic `C1 83 2A 9E`).
- **WHEN:** 2026-09-05, working tree at `c79bf5b`.
- **WHAT WOULD FALSIFY IT:** any asset under `Content/`, or under a **new content root** (a plugin — this
  project has none today), whose package bytes contain `FogVolume`.
- ⛔ **WHAT IT CANNOT SEE, written into the comment rather than left for a reader to discover:** the scan reads
  **disk only** — a child created in an open editor and **not yet saved** is invisible to it, as is anything
  outside `Content/`. It is **not** a live engine query; `SC-§78` still applies and an MCP `get_parent` sweep
  remains the stronger instrument.
- ⭐ **Free corroboration, and it points the other way too:** the scan matched **nothing on the `.umap` side
  either** ⇒ no **native** `AFogVolume` is level-placed in any map, which independently corroborates the
  heading clause 10 just narrowed.

---

## 3. THE CLAUSE-12 SWEEP — **SUBJECT-SHAPED**, and here is what I CHECKED, not only what I found

Per clause 7's promoted law: my predicate was ***"every sentence in `FogVolume.h`'s comment blocks, judged on
its own terms"***, ⛔ **not** *"the kinds of error QA listed"*. I read all 535 lines (now 600) and every member
doc block. **Checked and TRUE — no edit made** (recorded so nobody re-checks them):

| claim in the header | re-derived at my instant | verdict |
|---|---|---|
| `RaiseFog` **assigns** (`=`), never `+=` | `FogVolume.cpp` — assignment, no `+=` anywhere | ✅ true |
| `ApplyBrightSun` **zeroes the fog deadline in the same block** that stamps the shield | `FogActiveUntilTimeSeconds = 0.0;` immediately above the shield stamp | ✅ true |
| `IsFogActive` / `IsFogPrevented` use the **same strict `<`**, fail toward CLEAR | both `World->GetTimeSeconds() < …` | ✅ true |
| `GetBrightSunWindowSeconds` **degrades to `BrightSunBaseDurationSeconds`** | `return BrightSunBaseDurationSeconds;` on the no-world / no-hero paths | ✅ true |
| `ResetFog` **called from `ASiegeGameMode::PlayAgain`** | `SiegeGameMode.cpp` — the loop is inside `PlayAgain` | ✅ true |
| **87.8%** = `1 − 609.6 / 5000` | `FogVisionCeilingUU = 609.6f`; `UnitEngagementRadiusUU = 5000.f` | ✅ true |
| ×2 Watch Tower ⇒ `floor(2400 / 1524)` = **one** step | arithmetic | ✅ true |
| `FSiegeFogStatics` is **non-reflected** | `SiegeFogStatics.h` — plain `class GITCLAUDEUNREALTEST_API`; the nearby `USTRUCT(BlueprintType)` is **`FSiegeFogTuning`**, a **different type** | ✅ true |
| `Config/` holds **zero** `CoreRedirects` | my own grep ⇒ 0 | ✅ true |
| both deadlines `Transient`; five tunables `EditDefaultsOnly` | read off all seven `UPROPERTY` lines | ✅ true |
| the `EffectDuration`/`cards.csv` sentence is **stale and DELIBERATELY LEFT STANDING**, routed to `TASK-1016` | the comment says so itself and names its router | ✅ **correctly not swept — I left it** |
| `FogVolume.h:193`'s *"an auditor … would have found `BP_SiegeFog`, answered YES, and been wrong"* | `TASK-1050`'s, gated | ✅ **not re-opened** |

### 🚨 …and **FIVE more items the sweep DID turn up inside my two files. FIXED and DECLARED (clause 12).**

Every one is **stale on a different axis** from the parentage claim — i.e. the exact class the claim-shaped
sweep was structurally unable to see. ⛔ **None of them touches `TASK-1050`'s gated hunks** (that diff occupies
new lines `95-104` and `112-169`; these sit at old `:170`, `:201`, `:306`, `:442`, `:466`).

1. **`FindOrSpawn`'s doc named ONE caller; there are TWO.** It said *"Its caller is the `FogCover` arm of
   `USpellLibrary::ResolveSpell`."* — **true when written, false since `TASK-982`**: `SpellLibrary.cpp` calls
   `AFogVolume::FindOrSpawn` from **both** the `FogCover` arm **and** the `FogClear` (`BrightSun`) arm.
   ⚖️ **Fixed predicate-shaped, not count-shaped:** *"EVERY arm of `USpellLibrary::ResolveSpell` that touches
   fog state comes through this door — today the `FogCover` arm and the `FogClear` (`BrightSun`) arm"*, with
   the reason the second one uses the **write** door (`J-F17`: a pre-emptive `BrightSun` is legal with no fog
   up and may be the first cast of the match).
2. **`GetFogPreventionSecondsRemaining`'s doc said its callers were *"both boarded"* and that *"this row ships
   it with no caller of its own, on purpose."*** ⛔ **Both callers are LIVE now** —
   `SiegePlayerController.cpp` reads it twice, in `TASK-989`'s `Fog`-during-prevention refusal and
   `TASK-991`'s sun-on-sun refusal. ⇒ **the identical shape to clause 7's finding**: *true of the ROW that
   wrote it, false of the FILE a reader is holding.* Corrected and dated.
3. **`CONVENTIONS:131` no longer resolves.** The quoted `SM_ArenaTerrain` walk-surface sentence has moved to
   `CONVENTIONS.md:151`. ⛔ I did **not** write the new number (clause 6's own reasoning: a line number into a
   file that grows daily is a count). **Replaced with a text anchor**: *"`CONVENTIONS.md`'s **Terrain**
   bullet (⛔ LOCATE BY TEXT)"*, with a note that the old pointer had drifted.
4. **`SummonedUnit.h:895` no longer resolves either.** `UnitEngagementRadiusUU` is declared at `:914` today
   (and `SummonedUnit.h` is **currently dirty under another lane**, so any number I wrote would be wrong again
   within the hour). **Replaced with the symbol**: *"`ASummonedUnit::UnitEngagementRadiusUU` in
   `SummonedUnit.h` — ⛔ LOCATE BY TEXT"*.
5. **An unnamed cross-reference:** *"`FSiegeFogTuning` is deliberately NOT a member of this actor — see **the
   handoff** for why."* ⛔ Which handoff? **Named it**: `handoffs/TASK-998-programmer.md`, its *"STILL NOT a
   serialized member"* section (confirmed by reading it — that is where the reasoning lives).

⚖️ **The reusable half, offered because clause 7 asked for the lesson and not only the fix:** ⛔ **four of
these five are the same defect — a claim that was TRUE OF THE AUTHORING MOMENT, written in the present
tense, in a file that outlives the moment.** *"Its caller is X"*, *"both boarded"*, *"no caller of its own"*,
`file:131`. **A claim-shaped sweep cannot find them, because each one is stale about a different subject.**
Only walking the file sentence by sentence does.

### ⛔ OUTSIDE my two files — **DECLARED, NOT FIXED** (clause 5 / clause 12)

Three sentences carry the **same** staleness in `Tests/`, and I left every one of them alone:

- **`Tests/SiegeFogVolumeTest.cpp:31`** — *"`TASK-841`'s and is **still premise-blocked**"*. Same defect as
  clause 7, one file over.
- **`Tests/SiegeBrightSunTest.cpp:300`** — carries **`CONVENTIONS:131`** inside a `TEXT(...)` literal.
  ⚠️ **Note for whoever boards it: that one is an EXECUTABLE line** (a test message string), so it is not a
  comment-only fix and must not be batched with one.
- **`Tests/SiegeBrightSunTest.cpp:992`** — *"this row ships both with **no caller of its own**, on purpose"*.
  Same defect as sweep item 2.

⛔ I did not touch them. A one-line fix in an adjacent file is the cheapest possible scope breach.

---

## 4. THE FIFTH-FALSE-SITE CENSUS (clause 5) — ⛔ **NONE FOUND. And the census ends at a predicate.**

Re-derived at my own instant per `SC-§91`, by grepping the **symbol** tree-wide (`--include=*.h --include=*.cpp
--include=*.md --include=*.ini --include=*.csv --include=*.py`), **not** the sentence:

- **`Source/`** — every live mention of `BP_SiegeFog` is now inside **`FogVolume.h`/`FogVolume.cpp`** (corrected
  or true) **plus `SiegeFogStatics.h:73`**, which the row ruled **TRUE-AS-WRITTEN** and instructed me not to
  re-census. ⛔ **I did not re-census it and did not edit it.** *(It surfaced as a line in a symbol grep; I read
  no further and formed no opinion.)*
- **`Docs/`, `Config/`, `*.csv`, `*.py`** — **zero** mentions.
- ⚠️ **One thing worth naming so nobody counts it as a site:**
  `Intermediate/Build/.../UHT/FogVolume.gen.cpp` contains `BP_SiegeFog`. That is **UHT-generated metadata
  echoing the header's own class doc**, regenerated on every build, untracked. ⛔ **Not a site, not a source,
  not to be edited** — but it does mean the corrected header text will appear there after the next compile,
  which is expected.
- ⛔ **`CONVENTIONS.md` `FOG-§6`** — the manager's, corrected by the manager in the boarding action. ⛔ Not read
  for correction, not touched.

⇒ **The predicate, not the number: I found no fifth site of the parentage falsehood. I DID find five sites of
a DIFFERENT falsehood class (§3), all inside my fence, all fixed, plus three outside it, declared.** Per
`SC-§91` my own count is a **floor** too — the honest statement is *"no fifth parentage site survives a
symbol-grep of the source tree at `c79bf5b`"*, not *"there are four."*

---

## 5. SCOPE, PROOF, AND THE SUITE

### 5.1 ⛔ ZERO EXECUTABLE LINES — **PROVEN, not asserted**

`git diff -U0` line-shape filtering is the weak test (it only inspects the lines the diff happens to show), so
I ran the strong one: **both files were comment-stripped** (a real C++ tokenizer pass — `//`, `/* */`, and
string/char literal states, so a `//` inside a `TEXT("…")` cannot fool it) **at `HEAD` and in the working
tree, and the resulting code streams compared:**

```
FogVolume.cpp     HEAD code lines: 163   WORKING code lines: 163   IDENTICAL: True
FogVolume.h       HEAD code lines:  38   WORKING code lines:  38   IDENTICAL: True
⇒ ZERO EXECUTABLE LINES CHANGED IN BOTH FILES: True
```

⭐ **This proof covers the COMBINED diff — `TASK-1050`'s header hunks and mine — because it compares against
`HEAD`, not against the pre-edit working tree.** That is the gap `qa/TASK-1051.md` §1.2 declared open (it had
no Git and could not run the strip-and-filter). ✅ **It is now closed for the header as well as the `.cpp`.**
The corroborating line-shape filter agrees: **0 non-comment changed lines** out of 169 added / 26 removed.

Also verified: **LF endings preserved, no BOM, doc-block markers balanced** (`/**` × 18 = `*/` × 18), both
files still parse as one well-formed comment structure.

### 5.2 Suite

⛔ **Suite delta `0/0` — DECLARED-NOT-EXECUTED (`TL-§5c`). I did not compile and did not run the suite.**
Comment-only with a proven-identical code stream, so `0/0` is the right *expectation*; it is **declared, not
measured**, and an executed run still owes. ✅ **One real risk checked rather than assumed:** several fog
tests are **source-text structural probes** (`CountOccurrencesInCode`, `ExtractFunctionBody`). I grepped
`Tests/` for **every string I removed** — *"harmless here"*, *"single textual"*, *"NEVER LEVEL-PLACED"*,
*"churning between two"*, *"order is stable"*, *"FogCover` arm"*, *"see the handoff"*, *"both boarded"* — and
**no test asserts on any of them**. The header's `UFUNCTION` textual count is likewise **unchanged**.

⚠️ **Expected, not a finding:** a `.cpp` comment does **not** trigger the wide rebuild a header edit does. The
header in this row's diff **does**, but that rebuild was already owed by `TASK-1050` and is not new.

### 5.3 Write set

| path | what | note |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` | the `Find` comment block, **only** | cl. 0–5 |
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h` | **11** comment sites: cl. 6–11 (6) + sweep (5) | cl. 6–12 |
| `.claude/pipeline/handoffs/TASK-1053-programmer.md` | this file | new |
| `.claude/pipeline/TASKBOARD.md` | **TASK-1053's own `status:` line ONLY** | see 5.4 |

⛔ **NOT touched, by rule:** `.claude/pipeline/CONVENTIONS.md` (manager's, `SC-§82` WHO) · `.claude/agents/qa-reviewer.md`
(🧑 Jonathan's) · `Tests/SiegeFogRetentionWiringTest.cpp` (`TASK-1045`'s) · `/Game/Blueprints/BP_SiegeFog` ·
`Content/FogArea/**` · **any other `Source/` path**. ⛔ **Nothing was reparented; the editor was never opened.**

### 5.4 ⚠️ A NOTE FOR THE HOST ABOUT THE BOARD DIFF, SO IT IS NOT MISREAD

I located `TASK-1053` **by heading**, took the **first `- status:` beneath it**, asserted its pre-edit text
matched what I had read, replaced **that one index in place** (no insertion — the file's line count is
unchanged), and **grepped my marker `TASK-1053-STATUS-SET-BY-PROGRAMMER-2026-09-05` back out**: **1 hit,
line 302.** ⚠️ **But `git diff` cannot isolate my line**, because the manager boarded rows 1053–1055 in this
same dirty tree, so the whole block sits inside one added hunk (`@@ -294,0 +300,110 @@`) and my edit shows as
an addition with no matching deletion. **That is expected. The marker, not the diff, is the check.**

---

## 6. WHAT QA SHOULD SCRUTINISE (`TASK-1054`)

1. 🚨 **§2 (11)'s method — the load-bearing one.** Press on whether the **positive control** genuinely licenses
   the null result. My claim is narrow: *a Blueprint child must import its parent class, and an uncooked
   package stores import names in a plain name table*. If you doubt it, the falsifier is cheap and named in
   the comment. ⛔ **And note what I did NOT claim:** it is not a live engine read, and the comment says so.
2. 🚨 **Clause 2a compliance in the `.cpp`.** The prohibited sentence is *"so placing one is safe"*. Read my
   paragraph for **any** reassurance-shaped clause, not just that string. I believe there is none; the whole
   paragraph is mechanism.
3. ⚖️ **Clause 6: did I smuggle a number back in?** I wrote **no count** in the `UFUNCTION` sentence. **Two
   number-words appear elsewhere in my diff and I flag them against myself rather than let you find them:**
   (a) *"EXACTLY ONE MISSING SPECIFIER, not by two"* in clause 9 — **the row's own words**, and it is a
   structural fact about what is absent from each declaration, not a census that an edit can move;
   (b) *"the five ... tunables"* in the `.cpp` — pre-existing project vocabulary for that property set, and it
   matches the header. ⛔ **Neither is a stale-able count.** If you disagree on (a), it is one clause to cut.
4. ⚖️ **§3's five sweep fixes — are they in scope?** Clause 12 licensed *"a SEVENTH item inside these two
   files ⇒ FIX it and DECLARE it"*; I found five and read that licence as extending past the first. **All five
   are outside `TASK-1050`'s gated hunks** (ranges given in §3). If you rule any of them out of scope, they are
   independent single-block reverts.
5. ✅ **Confirm I did not re-open `TASK-1050`'s corrections.** The only lines I touched inside its hunks are the
   **six QA named** (`:101`, `:122`, `:126-127`, `:155-156`) — and at `:101` I preserved its *"not 'found too',
   never found at all"* and its *"what WOULD be found"* clauses intact.
6. ✅ **`SiegeFogStatics.h:73` was not re-censused, not read for correction, not edited.**

---

**⚖️ THE PRECEDENCE RULE, STATED IN THE HANDOFF AS CLAUSE 3 REQUIRES: the `CoreRedirects` paragraph in
`FogVolume.h` is the AUTHORITATIVE statement of the parent fact and of the DORMANT ruling. If it and the
`FogVolume.cpp` comment ever disagree, THE HEADER WINS — and the `.cpp` comment says so itself, so a reader
never has to find this file to learn it.**
