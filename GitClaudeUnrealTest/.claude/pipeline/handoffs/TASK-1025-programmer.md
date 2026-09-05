# TASK-1025 — the spell-VFX roster gate (gameplay-programmer)

**Status: ready-for-qa** · 2026-09-04 · HEAD `1a457df` · Evidence: `.claude/pipeline/footage/VID-006-fog-brightsun-paid-and-resolved-but-silent.md`

One new file, no shipped-code change, no compile, no engine, no MCP, no mutating Git.

---

## 0. ⚠️⚠️ READ THIS FIRST — MY DISPATCH PREMISE WENT STALE WHILE I WAS WRITING, AND I AM CORRECTING IT RATHER THAN INHERITING IT

My brief said, in bold: *"On today's tree this test FAILS on exactly two cards (`Fog`, `BrightSun`) … that is CORRECT."* **That sentence is now false.**

I re-measured rather than relayed (`SC-§40` cl. 9) and found it myself mid-task; the orchestrator confirmed it independently minutes later. Measured, on disk:

| asset | created | git | class strings in the `.uasset` |
|---|---|---|---|
| `Content/VFX/NS_Spell_Fog.uasset` | **2026-09-04 22:31:22**, 2,566,686 B | staged `A ` | `NiagaraSystem` ×4, `NiagaraEmitter` ×5 |
| `Content/VFX/NS_Spell_BrightSun.uasset` | **2026-09-04 22:31:27**, 2,453,143 B | staged `A ` | `NiagaraSystem` ×6, `NiagaraEmitter` ×6 |

⭐ `TASK-1024` landed both systems while this row was being authored, and its own handoff records the live editor handing back class `NiagaraSystem` for both composed paths (and `exists: false` for both *before* it ran — the measured defect).

⇒ **The roster is COMPLETE at 7/7 and this test is EXPECTED GREEN on today's tree, NOT red.**

⛔ **And the sentence that must be said plainly, because it is the weaker position: THIS GATE HAS NEVER BEEN OBSERVED RED.** Its free red — the strongest evidence a new gate can have — existed for a few hours and closed before the file compiled. **A gate never seen red is not yet evidence.** §5 is my answer to that, and it is the part of this handoff I most want QA to attack.

⛔ **Nothing was softened to reach green.** The assertion is unchanged: existence of the **named** asset, resolved through the shipped consumer's own expression. No exemption list, no disabled test, no bare "something was produced" check. The two cards that were red are green **because someone made the assets**, which is the only repair this gate ever accepts.

---

## 1. Files touched

| file | state | note |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeSpellVFXRosterTest.cpp` | ⛔ **NEW — UNTRACKED** | ⚠️ **the commit host must stage it BY EXPLICIT PATHSPEC.** An unstaged test commits as **silently absent**, and this project has paid for that twice. |
| `.claude/pipeline/TASKBOARD.md` | modified | **only `TASK-1025`'s own `status:` line**, re-located by heading immediately before the edit. Verified: exactly one `- status:` line under `#### TASK-1025` changed; line endings unchanged (LF, matching the file). |
| `.claude/pipeline/handoffs/TASK-1025-programmer.md` | new | this file |

⛔ **No `Source/` file other than the new test.** `SpellLibrary.cpp` was **read only** — the null-safe spawn is correct engineering and is explicitly out of scope.

**New test frame check (done FIRST, per the fence):** `Tests/` held no spell-VFX gate. The two sibling roster gates — `Siegebound.CardRoster.EverySpawnableCardRowResolvesItsComposedActorClassPath` (spawn half) and `…EveryCardRowResolvesItsCardArtTexture` (card-face half) — cover different assets, and both live in files other rows are actively rewriting. ⇒ new file, per the board's `names:` preference, so nothing serialises against `TASK-1021`/`1023`/`959`.

- **Test name:** `Siegebound.CardRoster.EverySpellCardHasItsCastVFXSystem`
- **Class:** `FSiegeSpellVFXRosterTest` · **Flags:** `EditorContext | EngineFilter` (the sibling gates' flags)

---

## 2. ⛔ THE ROSTER IS WALKED FROM THE DATA — TWO SOURCES, NO CARD LIST

There is **no hand-written spell list anywhere in the file.** A list is a coordinate; the data is the key (`SC-§77`), and a list would miss the next spell exactly as this defect missed these two.

**(1) The runtime authority** — the `UDataTable` off `ASiegePlayerController`'s **own CDO** (`CardTableAsset`, read by reflection because it is not public; the lookup checks the **pointed-to type**, not just the field name, so a rename/retype is a named hard failure rather than an empty walk). The controller is the consumer that reads the row and calls `USpellLibrary::ResolveSpell`, so its table is the one that decides which VFX can ever be spawned.

Spellhood there is **`Row->SpellEffect != None`**, not `CardType == Spell` — join on the category the **code computes** (`SC-§37`). `ResolveSpell` switches on `SpellEffect`; its `None` arm refuses and returns **before** the shared VFX tail, so a `None` row can never reach the spawn and demanding an asset for it would be a false red.

**(2) The spreadsheet of record** — `Docs/Data/cards.csv`, filtered on `CardType` compared against `StaticEnum<ECardType>()`'s own entry name (never a typed `"Spell"` literal).

⚠️ **The empty first column header is handled and ASSERTED, not assumed** (`FOG-§9.8e`): `CardID` is field 0 precisely *because* the DataTable CSV convention leaves that header blank, so the file fails loudly if it ever grows a name. **Positive control, measured out-of-engine: 34/34 rows return a non-empty index-0 value** — a blank table would be caught there, not silently walked.

⚠️ **Only columns BEFORE `Notes` are read from the CSV, and that ordering is asserted by name.** `Notes` is free text and may carry an embedded comma that pushes every later column right. `SpellEffect` sits *after* `Notes`, so it is read from the **DataTable** (whose importer parses quoting correctly) and never from this file's own split. Same precondition, same reason, as `SiegeAssistantSelectionTest.cpp`. (Measured today: 34/34 rows are exactly 32 fields — no embedded commas at present, which is why the precondition passes rather than being load-bearing yet.)

**The walk is over the UNION of the two rosters**, so neither source can hide a card — and the two are separately **asserted to agree**, which catches a spell authored in the CSV and never imported (unplayable, nothing else in the tree looks) and a spell edited into the asset behind the source of record (vanishes on the next re-import). Both are green today.

**Derived roster, measured read-only:** `Fireball · FrostNova · Lightning · BattleCry · Pickpocket · Fog · BrightSun` (7).

---

## 3. ⛔ THE FAILURE MESSAGE IS THE DELIVERABLE — VERBATIM

Both strings below were **extracted from the shipped source and rendered by substitution**, not retyped (`SC-§37.1`). They are what the gate emits for a missing card. ⚠️ **They are the messages it WOULD produce; they were not captured from a run** — see §0 and §5.

**`Fog` — the assertion description:**

```
⭐ SPELL CAST VFX — card 'Fog' (CardType Spell, SpellEffect FogCover) has its Niagara system at '/Game/VFX/NS_Spell_Fog.NS_Spell_Fog'
```

**`Fog` — the work order (`AddError`):**

```
⛔ SPELL WITH NO CAST VFX — THE ASSET IS ABSENT: card 'Fog' (CardType Spell, SpellEffect FogCover) has NO Niagara system at '/Game/VFX/NS_Spell_Fog.NS_Spell_Fog'. ⛔ WHAT TO MAKE AND WHERE — no source file needs to be opened: author a Niagara system and save it as 'Content/VFX/NS_Spell_Fog.uasset' (asset name 'NS_Spell_Fog' — USpellLibrary COMPOSES that name from the CardID, so it must match character for character). ⛔ WHY IT IS URGENT: the spawn is NULL-SAFE and LOG-ONCE by design, so this card still charges its gold, still returns true and still logs 'resolved' while the player sees NOTHING — VID-006 measured exactly that on 'Fog' (985 → 935) and 'BrightSun' (893 → 834): charged, no fizzle, no refund, no pixels. A successful cast is byte-for-byte indistinguishable from a dead click. ⛔ THE REPAIR IS THE ASSET, never a weakened assertion here (CONVENTIONS SC-§50 cl. 4).
```

**`BrightSun` — the work order (`AddError`):**

```
⛔ SPELL WITH NO CAST VFX — THE ASSET IS ABSENT: card 'BrightSun' (CardType Spell, SpellEffect FogClear) has NO Niagara system at '/Game/VFX/NS_Spell_BrightSun.NS_Spell_BrightSun'. ⛔ WHAT TO MAKE AND WHERE — no source file needs to be opened: author a Niagara system and save it as 'Content/VFX/NS_Spell_BrightSun.uasset' (asset name 'NS_Spell_BrightSun' — USpellLibrary COMPOSES that name from the CardID, so it must match character for character). ⛔ WHY IT IS URGENT: the spawn is NULL-SAFE and LOG-ONCE by design, so this card still charges its gold, still returns true and still logs 'resolved' while the player sees NOTHING — VID-006 measured exactly that on 'Fog' (985 → 935) and 'BrightSun' (893 → 834): charged, no fizzle, no refund, no pixels. A successful cast is byte-for-byte indistinguishable from a dead click. ⛔ THE REPAIR IS THE ASSET, never a weakened assertion here (CONVENTIONS SC-§50 cl. 4).
```

⇒ **CardID + `/Game/…` object path + the `Content/VFX/…uasset` file to create.** An artist can act on that red without opening a single source file. That is what turns the test into a work order.

---

## 4. Two findings, two repairs — reported separately

The shipped consumer's typed soft pointer returns **null for both** of these, so it cannot tell them apart. This file can:

- **(a) ABSENT** — nothing loads at the composed path. Repair: **author the asset.**
- **(b) WRONG TYPE** — something *does* load there and it is not a `UNiagaraSystem`. Repair: **replace the impostor** (authoring a second asset at that path is impossible). Its message names the class it actually found.

Collapsing these into "no VFX" is how (b) gets "fixed" by an action that cannot work.

⛔ The probe runs **both** expressions: the consumer's own `TSoftObjectPtr<UNiagaraSystem>(…).LoadSynchronous()` for the verdict, and an untyped `FSoftObjectPath::TryLoad()` to classify — and a **CONSUMER AGREEMENT** assertion pins that the two never diverge, because the message is this gate's deliverable and it must not come apart from the behaviour the player is subject to.

---

## 5. ⛔⛔ MAKING THE RED REAL WHEN THE FREE RED HAS ALREADY EXPIRED

Three things, in descending order of durability.

### 5a. The permanent, in-test synthesised red — this is the real answer

A **measured-absent synthetic `CardID`, `XxNoSuchSpellXx`**, is pushed through the **same composer and the same probe** the walk just used, and the outcome is asserted to be `ABSENT`. The assertions are **inverted** — the block is green *because* the probe answered MISS — and nothing there calls `AddError` on an expected miss.

⛔ **It is the only assertion in the file that catches a loader gone BLIND and answering non-null for everything** — a mode in which every card passes, every count is healthy and every partition balances. Now that the roster is whole, it is the **only** thing on the page that proves the probe can still fail. `SC-§54` cl. 3: a synthetic input buys an assertion that re-proves on **every run, forever**; a disturbed asset buys one transcript that decays into a screenshot.

⛔ **Nothing on disk is touched.** The synthetic is a string on the function's stack. No asset is renamed, moved or opened for write, and **the tree is left unmutated.**

**The synthetic was MEASURED absent, not assumed** (`SC-§40` cl. 10), repo-wide with `.git`/`Binaries`/`Intermediate`/`DerivedDataCache`/`Saved` excluded, with live positive controls so a dead grep could not read as a clean zero:

| needle | files |
|---|---|
| `XxNoSuchSpellXx` | **0** ⇐ the synthetic |
| `NS_Spell_` | 38 (instrument control) |
| `Fireball` | 98 (instrument control) |

Zero hits in **both** substring directions — the trap that nearly caught `TASK-964`.

### 5b. The read-only decision table — the gate's logic, exercised outside the engine

I replicated the CSV half of the derivation and the composer exactly, and checked the filesystem. **No writes, no mutation, no engine:**

```
column-0 header repr: ''            <- the empty header (FOG-§9.8e)
positive control: 34/34 rows return a non-empty CardID
derived spell roster (7): Fireball, FrostNova, Lightning, BattleCry, Pickpocket, Fog, BrightSun

CardID           composed object path                                          on disk
Fireball         /Game/VFX/NS_Spell_Fireball.NS_Spell_Fireball                 PRESENT
FrostNova        /Game/VFX/NS_Spell_FrostNova.NS_Spell_FrostNova               PRESENT
Lightning        /Game/VFX/NS_Spell_Lightning.NS_Spell_Lightning               PRESENT
BattleCry        /Game/VFX/NS_Spell_BattleCry.NS_Spell_BattleCry               PRESENT
Pickpocket       /Game/VFX/NS_Spell_Pickpocket.NS_Spell_Pickpocket             PRESENT
Fog              /Game/VFX/NS_Spell_Fog.NS_Spell_Fog                           PRESENT
BrightSun        /Game/VFX/NS_Spell_BrightSun.NS_Spell_BrightSun               PRESENT
XxNoSuchSpellXx  /Game/VFX/NS_Spell_XxNoSuchSpellXx.NS_Spell_XxNoSuchSpellXx   ABSENT   <- the ABSENT branch
```

⚠️ **What this is and is not.** It is evidence that the roster derivation and the composer produce the right paths and that the present/absent split is real. It is **not** a run of the compiled test, and file presence is **not** loadability — `LoadSynchronous` returning a `UNiagaraSystem` is what the gate actually measures, at run time, and I ran no compile and no engine (fenced).

### 5c. The exact one-line mutations that turn it red — for whoever can run it

Named, **not performed**:

1. **Rename one asset:** `Content/VFX/NS_Spell_Fog.uasset` → any other name. The walk reds with the §3 `Fog` work order verbatim. (Restore by renaming back.)
2. **Add a spell row — the mutation that matters most, because it is the defect class this gate exists for:** append a `CardType Spell` row to `Docs/Data/cards.csv` with any new `CardID`. The gate reds **immediately**, naming the asset nobody has made yet. ⭐ **This is the whole design: a new spell is guarded by construction, with no edit to this test.**
3. **Break the mirror:** change either format literal in `SpellLibrary.cpp`. The **composer pin** reds and says the mirror is stale — rather than the walk silently asserting paths the game no longer uses.
4. **Blind the loader** (hypothetically): if `LoadSynchronous` ever returned non-null for everything, every card would pass, every count would balance — and **only §5a's inverted assertion** would fire.

---

## 6. Suite delta

⛔ **DELTA, not an absolute: `+1` test.** The last executed figure I was given is **475 / 0 at `1a457df`**; I did not execute the suite (fenced), so I state the delta only.

**Expected result on today's tree: GREEN** (7/7 present; §0). ⛔ If it comes back red on `Fog` or `BrightSun`, **the assets are present but do not load as `UNiagaraSystem`** — which is a real finding about `TASK-1024`'s output, not a reason to touch this file.

---

## 7. ⛔ What QA should scrutinise — my own weak points, named

1. **⛔⛔ The gate has never been seen red (§0/§5).** This is the weakest claim in the handoff. I have argued §5a is a stronger substitute than a one-off transcript; **push back if you disagree** — the alternative is asking the build-master to run it with one asset temporarily renamed, and I did not do that because it mutates the tree.
2. **⛔ THE ONE DECLARED RESIDUAL — THE COMPOSER MIRROR.** Unlike the card-art gate (which reads an authored `TSoftObjectPtr` cell and composes nothing), the spell VFX path is **composed in code**, inside a file-local anonymous-namespace helper (`SpawnSpellVFX`) that **no test can call**. This file therefore **mirrors** those two lines. Mitigation: the mirror is **pinned against the shipped source** — `SpellLibrary.cpp` is located **by filename** (a key, never a line number) and both format literals are asserted present, with a "scan is alive" self-check so a dead scan cannot read as a clean pin. ⚠️ A source-text pin is brittle (`SC-§75`(A) warns so); it earns its place only because the behaviour is unreachable by any other means. **Corroboration:** `TASK-1024`'s handoff independently derived the same format and got byte-identical paths back from `load_asset`.
3. **The `SpellEffect != None` vs `CardType == Spell` split.** I chose the code-computed category as the walk key and added a **separate COVERAGE assertion** (zero `Spell`-typed rows carry `None`) so the effect-keyed walk is provably total. Both predicates coincide today (7 = 7). ⛔ If you think the walk should key on `CardType` alone, say so — the union already includes those rows, so the behaviour is the same today and the difference is which message fires tomorrow.
4. **`AddExpectedMessagePlain` is declared for the SYNTHETIC ONLY.** A real card's failing load emits `LogUObjectGlobals: Failed to find object …` **undeclared, deliberately** — declaring a shipped card's warning would read as an exemption for exactly the card the gate exists to name. `Occurrences = -1` on the synthetic (documented as "silently ignored"), so it cannot itself redden the suite.
5. **Not compiled.** Fenced. API surface used: `FSoftObjectPath::TryLoad`, `TSoftObjectPtr<T>::LoadSynchronous`, `UDataTable::GetRowNames/FindRow/GetRowStruct`, `FindFProperty<FSoftObjectProperty>`, `StaticEnum<>`, `FFileHelper::LoadFileToString`, `IFileManager::FindFilesRecursive`, `AddExpectedMessagePlain`. **Every one already appears in the executed suite** (`SiegeCardArtRosterTest.cpp`, `SiegeAssistantSelectionTest.cpp`, `SiegeFogClampTest.cpp`), and `Niagara` is already a public module dependency (`SpellLibrary.cpp` uses it).
6. **Scope of the green bar** — declared in the file header, restated here so nobody over-reads it: this gate does **not** assert the VFX looks like anything (an empty system passes), does **not** assert it is spawned at run time, reads the **C++ CDO**, covers **only** `NS_Spell_*` (not `NS_ChainZap`/`NS_CastleDebris`/`NS_RecallChannel`), and says nothing about **where** the hero-line delivery spawns it.

---

## 8. ⛔ A gate row is OWED and is NOT boarded — flagged, not assumed

`TASK-1025`'s own `names:` block reads **"GATE: ⛔ OWED — ⛔ board it at dispatch."** I grepped the board for every reference to `1025`: the only hits are this row itself, its Slack line, its `names:` line, and `TASK-1024`'s out-of-scope clause. **No gate row exists.** Reporting it rather than assuming one is there, per the dispatch's instruction.

## 9. Fences honoured

⛔ No compile · ⛔ no engine · ⛔ no MCP · ⛔ no mutating Git (read-only `git status`/`log`/`diff` only, `SC-§71a`; no `checkout`/`restore`/`stash`/`reset`/`clean`) · ⛔ no asset created, renamed or moved · ⛔ no shipped `Source/` file edited · ⛔ no row counts or CSV line numbers written into source (`SC-§77` — every citation in the file is a `CardID`, a symbol, or a filename) · ⛔ board edit confined to `TASK-1025`'s own `status:` line, re-located by heading immediately before editing.
