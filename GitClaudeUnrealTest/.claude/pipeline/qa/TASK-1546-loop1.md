PASS
# QA Report: TASK-1546 re-gate, QA loop 1 (gate for TASK-1541: code names in the Controls help text replaced with plain player wording)

Verdict: **PASS**: 0 BLOCKER · 0 WARN · 0 NIT. Board: `TASK-1541` → `qa-passed` (loop 1 of 3 closes here).

Marker `TASK-1546-LOOP-1-PASSED`. Date 2026-09-27. Prior gate: `qa/TASK-1546.md` (FAIL: B1 at `SiegeControlsHelpWidget.cpp:514`; W1 at `:1475–1477` plus the out-of-fence test label; N1, N2 optional). The programmer's answer is `handoffs/TASK-1541-programmer.md` `## QA loop 1`. `TASK-1541` was at `ready-for-qa` (marker `TASK-1541-LOOP1-READY-FOR-QA-2026-09-27`) when I started. This is a separate file, following the `qa/TASK-1481-loop1.md` / `qa/TASK-1549-loop1.md` precedent, so `qa/TASK-1546.md` and its §10 stay byte-stable.

| file | sub-verdict | BLOCKER | WARN | NIT |
|---|---|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` | **B1 fixed, W1.1 fixed, N1 + N2 taken**; fence holds | 0 | 0 | 0 |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` | byte-identical to loop 0 | 0 | 0 | 0 |
| `handoffs/TASK-1541-programmer.md` | append-only, and every loop-1 claim holds | — | — | — |

## §0 — What I inspected, beyond reading text

`get_headless_status` → `editor_connected`. I ran four read-only `unreal_inspector` Python queries. Files were opened `rb`, using only `hashlib`, `re`, `os` and `difflib`. No git, no `subprocess`, no editor state or asset touched, no lifecycle call, nothing under `/Game/sA_ArcheryVfxPack` (`TASK-1557` is live in parallel).

1. **Bytes.** sha256, size, LF/CR counts and BOM for both touched files, three scratch copies, `TASK-1480`'s eight other anchors and `SiegeCardHandKeyLabelTest.cpp` (§4).
2. **Baseline.** I diffed against **my own loop-0 copy**, `scratchpad\SiegeControlsHelpWidget.after.cpp`. It hashes `ada3609f…ebcb`, which is `qa/TASK-1546.md` §8. The programmer's `SiegeControlsHelpWidget.loop0.cpp` has the same hash, so either copy gives the same diff.
3. **Lexer.** It tracks code, `//` and `/* */` comments, string literals, char literals and raw strings (there are 0 of the last two). On loop-0 vs now it produces:
   - the code skeleton;
   - a literal-by-literal diff;
   - a comment-sequence diff;
   - a line diff;
   - a row parse (27 rows: `AddRow` id / `DisplayName` / `OneLine`, plus the `Row.Detail` concatenation).
4. **Census.** Eleven classes over all 610 literals (unescaped), comparing the loop-0 and current multisets, plus a per-field pass over 27 × 3 row fields.
5. **Pins.** I checked the presence of all 609 literals in the three help-reading test files in the loop-0 prose vs the current prose, twice: case-sensitive over the composed row fields, and case-insensitive over all 610 widget literals.
6. **Other checks.** `TASK-1480`'s six hunk ranges, matched start copy → now. The handoff's append-only prefix hash. A trailing-whitespace grep.

**Source reads for truth (§1, §2):**

- `TeamId.h` :13–18, :25–40
- `SiegeCombatStatics.cpp` :30–36, :38–79, :292–295
- `HeroCharacter.cpp` :565–641
- The `ITeamAgent` implementer grep over all of `Source/` (`.h` and `.cpp`)
- The `ABuilding` / `ASummonedUnit` subclass grep
- `CommanderNpc.h` :86–113, `GoldNode.h` :74–83
- `Castle.cpp` :1106–1133, `Building.cpp` :525–543
- `Building.h` :209–219, `Building.cpp` :341–379
- Every `EnemyRevealCost` site in `SiegeControlsHelp*`

## §1 — B1: fixed, TRUE at source

`SiegeControlsHelpWidget.cpp:522` (`Hero.Attack` `Detail`, first literal) now reads: "One swing damages **every enemy unit, hero, building and castle** within your melee reach and inside a ". The next literal, "cone in front of you, …" (:523), is byte-identical.

**The set the sentence names equals the set the swing hits.**

- **The candidate set is exactly the enemy-team `ITeamAgent`s.**
  - `DoMeleeAttack` takes its candidates from `FSiegeCombatStatics::GatherHostileAgents` (`HeroCharacter.cpp:585`).
  - That function calls `GatherTeamAgentsFiltered(…, bWantHostile=true, …)` (`SiegeCombatStatics.cpp:295`).
  - The filter enumerates `GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), …)` (:54), then `IsValid` (:63), `Cast<ITeamAgent>` (:69) and `IsHostileTeam` (:75).
  - `IsHostileTeam` is `CandidateTeam != ViewerTeam` (:35). `ETeamId` has only `Blue` and `Red` (`TeamId.h:14–18`), so "hostile" means exactly "enemy": there is no neutral team for the word to miss.
  - After that come the range test (`HeroCharacter.cpp:621`), the cone test (:632) and `ApplyDamage` (:641).
- **There are exactly four implementers.** `public ITeamAgent` / `, ITeamAgent` across every `.h` and `.cpp` under `Source/` finds exactly four: `ACastle` (`Castle.h:103`), `ABuilding` (`Building.h:61`), `AHeroCharacter` (`HeroCharacter.h:398`) and `ASummonedUnit` (`SummonedUnit.h:204`). No `Variant_*` class and no test fixture implements it.
- **The set is closed.** `UTeamAgent` is `UINTERFACE(MinimalAPI, NotBlueprintable)` (`TeamId.h:25`), so no Blueprint can implement it. The gatherer's own comment relies on the same fact for its native cast (`SiegeCombatStatics.cpp:68`).
- **"building" is exact.** Every `ABuilding` subclass is a card-placed structure: `ABarracks` (`Barracks.h:46`), `ATower` (`Tower.h:73`), `AClimbableTower` (`ClimbableTower.h:219`) and `ADeepMine` (`DeepMine.h:44`).
- **"unit" covers miners** (and sorcerers): `AMinerUnit : ASummonedUnit` (`MinerUnit.h:287`) and `ASorcererUnit : ASummonedUnit` (`SorcererUnit.h:66`).
- **"damages" is honest for the named kinds.** The castle and building receivers refuse only same-team, destroyed or non-positive hits: `Castle.cpp:1121–1131` (plus the M8 authority belt at :1112) and `Building.cpp:529–541`. An enemy hero's swing therefore lands on a live enemy castle or building.
- **The exclusions are now honoured.**
  - `ACommanderNpc` "DELIBERATELY DOES **NOT** IMPLEMENT ITeamAgent" and lists "HeroCharacter.cpp:404 (melee cone)" among the eight target sites (`CommanderNpc.h:90–97`).
  - `AGoldNode` is "NOT a combatant: deliberately does NOT implement ITeamAgent" and is team-neutral (`GoldNode.h:78–82`).
  - Neither is a unit, hero, building or castle in the sentence's words. A gold node is not an *enemy* building, because it has no team.
- **Digits and code names.** The new literal has none (§3).
- **The 8-line R-05 comment (:513–520) is accurate, cites by text and sits inside the row block.** Checked claim by claim:
  - "exactly the team-agent kinds … GatherHostileAgents hands the swing": true, as above.
  - "UTeamAgent is NotBlueprintable, so no asset can join the set": true.
  - The commander "HAS a team but deliberately does not implement ITeamAgent and names the melee cone among the sites it stays out of (ACommanderNpc's class doc)": true, and the commander's team getter is `GetCommanderTeam()`.
  - "AGoldNode opts out the same way": true.
  - The loop-0 history sentence: true.
  - It has no `:NNN`.

The veil silence (`ESiegeVeilPolicy::SuppressVeiled`, `HeroCharacter.cpp:585`) stays unstated. `qa/TASK-1546.md` §2 ruled it not owed, because neither the old wording nor the new one says veiled units are hit or skipped. I apply the same ruling here.

## §2 — W1.1, N1, N2

- **W1.1: fixed.** The R-22 comment beside the one-liner is now :1485–1490.
  - The line diff shows exactly one replaced line: loop-0 :1477 became :1487–1490. So loop-0 :1475–1476 (now :1485–1486) are byte-identical, and the old `CommanderNpc.h:311` citation stays on an unchanged line.
  - The added clause is true. The one-liner (:1492) has no number and no property name. The `Detail` says "a fixed reveal fee" (:1509). "the amendment below" is the :1502–1507 block, which records the same change.
  - `EnemyRevealCost` survives in `SiegeControlsHelpWidget.cpp` only in comments (:1486, :1490, :1495, :1506), so the widget reads no reveal-cost property.
  - The `.h:163` field doc remains on the manager's list (`TASK-1560`).
- **W1.2: correctly left alone.** `Tests/SiegeControlsHelpTest.cpp` is byte-identical (`525b5886…91d7`). The label is boarded into `TASK-1560` by the manager (row bullet `TASK-1541-FOLLOWUPS-RULED-2026-09-27`).
- **N1: taken, true.** :826 now says "multiplies the building's maximum health by a set factor, compounding".
  - `ABuilding::StackHealthMultiplier` is "`StackHealthStep ^ UpgradeCount`" (`Building.h:211`). It is computed by repeated multiplication with no cap (`Building.cpp:363–379`). So "factor" is exact, and "compounding … no ceiling" is unchanged.
  - The R-25 comment (:806–811) was amended to match (`SC-§53`), with no `:NNN`.
  - The other two "a set step" occurrences in the prose (`Cards.PlacementResize`, `PickMode.Resize`) are additive wheel steps. They are correct as they stand and were rightly not touched.
- **N2: taken.** :545–546 now say "…it still tells the HUD how much cooldown is left so the HUD can flash it; when the rally cooldown runs out…". "it" binds to the remaining time. The claim is unchanged from loop 0, which I verified at source then.
  - The programmer kept "the HUD can" instead of my "so it can", because "it" there could read as the press. **Accepted.** That is a better reading than my suggestion.

## §3 — The fence, the census and the pins, re-measured by me

| check | result |
|---|---|
| Code skeleton (literals masked, comments removed, whitespace collapsed) | **identical**, 53207 = 53207 chars. My loop-1 lexer uses a different mask token than loop 0's, so this length is not comparable with §1's 54381. Equality within each run is the claim. |
| Literals | **610 = 610.** Char literals 0 = 0, raw strings 0 = 0. |
| Changed literals | **3**, all `Detail`: #93 `Hero.Attack` (:522), #108 `Hero.Rally` (:546), #187 `Cards.StackUpgrade` (:826). The row parse (27 = 27 rows, ids equal) shows **0 `DisplayName` / `OneLine` bytes changed**, and the only changed `Detail`s are those three (506→521, 462→451, 2578→2580 source chars). |
| Comments | 2049 → 2062 (+13) in **3 ops**: insert 8 at :513 (R-05), replace 1→3 at :809 (R-25), replace 1→4 at :1487 (R-22). Each sits inside its own row block. |
| Line diff vs `ada3609f…` | **6 hunks**. They equal the handoff's six `@@` headers exactly (`-512,0 +513,8` · `-514 +522` · `-538 +546` · `-801 +809,3` · `-816 +826` · `-1477 +1487,4`). |
| `:NNN` / "line NNN" in the 18 new or changed lines | 0 |
| Trailing whitespace, whole file | 0 lines |
| Census, code names | **0.** The unescaped 11-class flag multiset over all 610 literals is identical, loop 0 → now. The 3 changed literals carry no flag, apart from "HUD" ×2 (ALL-CAPS). That word was in the same literal at loop 0, and it is an ordinary game-UI word. The per-field residuals are the same non-code set as loop 0: `(2D)`, ENDS, EMPTY, and the `{Interface.WarMap}` / `{Interface.ControlsHelp}` key-chip tokens. |
| Digits | The composed-prose digit string is **identical**, `1011223123430`, loop 0 = now. The changed literals have `[]` old and new. |
| `ForbiddenInPlayerProse` fragments, plus `::` and `()`, in the registry prose | 0 |
| Detail longer than one-liner | 27 / 27 |
| Pins, case-sensitive over the composed row fields | **0 presence changes** across 609 literals: 406 `SiegeControlsHelpTest.cpp` · 63 `SiegeCardHandKeyLabelTest.cpp` · 140 `SiegeMenuInputTest.cpp`. |
| Pins, case-insensitive over all 610 widget literals | **0 presence changes** |
| Test literals containing any old or new loop-1 phrase | 0 |
| The moved pin's phrase "charged once for the whole hand" | Once in the composed prose, loop 0 = now = 1. The `TEXT` at :696 is its only occurrence as a literal. |
| The mutation-arm literal "The fee is a single set amount and it is charged once for the whole hand, flat. Dumping a " | **Byte-identical.** Count 1 = 1. It is now at :696 (loop-0 :688, +8 from the R-05 insert). |
| `TASK-1480`'s hunks | Start-copy (`f407d0b4…`) ranges 178–193, 209–222, 340–348, 413–434, 584–587 and 4128–4141: **every line equal-matched** start → now. |
| `TASK-1480`'s other files | Byte-identical to `qa/TASK-1481-loop1.md` §4 / `qa/TASK-1546.md` §6: `.h` `cc16d2ca…` · `SiegeMenuInputSubsystem.cpp` `53050739…` · `.h` `5bba1a03…` · `Tests/SiegeMenuInputTest.cpp` `5470c886…` · `DeckBuilderWidget.h` `950dc811…` · `.cpp` `de406a0c…` · `SiegePlayerController.cpp` `8f33a5de…` · `.h` `840416b6…` (8/8). |
| Handoff append-only | The first 57387 B hash `ce4ed4588fa27b7ad54c386ece8a057c0c48a2b23a177ea618987323ade4df65` (= the dispatch's `ce4ed458…`). Byte 57387 starts `\n## QA loop 1`. The file is 68860 B. |

The handoff's "composed prose 30401 → 30407" differs from my 30153 → 30159 in absolute terms, because the composition differs (separators and escapes). The **delta is +6 in both** (+15 for B1, −11 for N2, +2 for N1).

## §4 — After-sha256, re-measured (`rb` + `hashlib`): the anchors for 5a `TASK-1538` and host `TASK-1540`

| file | sha256 | bytes | EOL | = handoff |
|---|---|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` | **`a6bf281fd83d4db65461fe8a831e93b632faa1aca9af80ef6785b6cfbc235d8b`** | 278984 | LF 4497, CR 0, no BOM | yes |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` | **`525b5886779d1e346162f3d1e04e226ef69cfffab6ed6714705341385fc591d7`** | 150481 | LF 2624, CR 0, no BOM | yes (unchanged since loop 0) |
| scratch loop-0 copy (mine, `.after.cpp`, and the programmer's `.loop0.cpp`) | `ada3609ff7115fa6c3a202f2998e1a42d381e8d7bbe60cfb1f0de14def1debcb` | 277821 | LF 4484, CR 0 | = `qa/TASK-1546.md` §8 |

`ada3609f…` is now superseded. **`TASK-1538` compiles `a6bf281f…` and `TASK-1540` commits `a6bf281f…`, never `ada3609f…`.**

## §5 — 5a (`TASK-1538`): `qa/TASK-1546.md` §10 still applies UNCHANGED

- **Shape:** unchanged. The change is `.cpp`-only: no header moved, so no `.generated.h` / `.gen.cpp` change is expected. `N` = **566** (no test added). **Mutation arms owed: 1.**
- **The arm's edit is valid as written.** The `Cards.Discard` literal that begins "The fee is a single set amount and it is charged once for the whole hand, flat. Dumping a " is byte-identical, now at `:696`. The edit (`whole` → `entire`, that literal only), the expected RED assertion ("⭐ The page states the flat-fee rule in words instead of restating its value", test `Siegebound.ControlsHelp.DiscardAllLetterMovesWhileCardDigitsHold`, pin at `Tests/SiegeControlsHelpTest.cpp:1819`) and the stop-if-green rule all stand.
- **The hashes in steps 1 and 4 resolve to this re-gate** (§10 already said "the loop-1 re-gate's after-sha256"): widget **`a6bf281f…5d8b`**, test **`525b5886…91d7`**. Step 4's restore must return the widget to `a6bf281f…5d8b` byte for byte. Step 5 is 566 started / 0 failed.

**For 5b (`TASK-1547`):** the pages that changed since loop 0 are `Hero.Attack`, `Hero.Rally` and `Cards.StackUpgrade`. `Hero.Attack`'s first sentence grew by 15 characters, so its wrap may shift.

## §6 — Observations (no action on this row)

- The dispatch called the diff "5-hunk". The handoff's `-U0` has **6** `@@` headers, and my line diff agrees. The handoff never states a count, so this is a miscount in the relay only.
- The R-22 amendment says "the page leaves no name either: the one-liner types no number and names no property, …". The one-liner was already name-free before `TASK-1541`; the clause describes the page's state, and the "where it printed EnemyRevealCost" half pins the change on the `Detail`. That is accurate, so I raise no finding.

## Not examined / limitations

- Nothing was compiled or run. Wrapping and legibility on the real `UTextBlock` belong to 5b (`TASK-1547`).
- `SC-§71b` scope: this verdict is text- and byte-level. The only inspector use was read-only file hashing and lexing in the editor's Python. No asset, graph, Blueprint or PIE was involved, and I made no read under `/Game/`.
- "No Blueprint joins the set" rests on `NotBlueprintable` at source. I did not enumerate Blueprint subclasses of the four native classes. Any such subclass is still a castle, building, hero or unit, so the sentence holds either way.
- Whether an enemy hero exists in a given mode (for example vs-bot) was not checked. The sentence is true either way ("every … hero within your melee reach").
- The truth of sentences this loop did not change was not re-examined beyond `qa/TASK-1546.md` §4. The mechanism-3 citation blocks' line numbers remain unverified, per `TASK-1480` (a).
- The census is regex classes plus the loop-0 manual sweep. The three changed literals were also read in full by hand.
