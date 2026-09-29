# TASK-1576 — [HELP-DERIVED-NUMBERS] — programmer handoff

Marker `TASK-1576-HELP-DERIVED-NUMBERS`. 2026-09-28. gameplay-programmer. Status → `ready-for-qa`. Gate: `TASK-1577`. 5a: `TASK-1578`. 5b: `TASK-1579`. Host: `TASK-1580`.

Code and text, plus one new test and one narrowed assertion. ⛔ No compile, no PIE, no asset load, no asset save, no mutating git. Declared tooling (`SC-§71a`): `sha256sum`, `git --no-optional-locks diff --no-index`, a process census by command line, Python text simulations over scratch copies (`t1576/sim.py`, `digits.py`, `arms.py` in the session scratchpad), and five read-only inspector calls (`execute_unreal_python_readonly`) plus one `is_pie_active`.

## §0 Start state (amended (0)): measured, all equal

| file | expected (`qa/TASK-1586.md` §4; `.h` from `qa/TASK-1575.md` §9) | measured |
|---|---|---|
| `SiegeControlsHelpWidget.cpp` | `9b078d0e…4020d` | `9b078d0ec6a51f4439ce8d0bfa9ab1bb39fdcc24dfdd52a8c426de3ea8b4020d` ✓ (304435 B, 4771 lines) |
| `Tests/SiegeControlsHelpTest.cpp` | `c45412c9…b38b` | `c45412c9cd2597fdb2f836b749fc0753c5686b8833a7c5f9d7a43e914748b38b` ✓ (160530 B, 2790 lines) |
| `SiegeControlsHelpWidget.h` | `31c98693…bf4e` | `31c986935a49b859e20212c164d148f0865993417ed39c4be89d150adcd7bf4e` ✓ (81619 B, 1409 lines) |

Copied byte for byte to `t1576/W.start.cpp`, `T.start.cpp`, `W.start.h` (re-hashed equal). §11's diff is against those copies, which are the amended (0) anchors. All three files are LF-only and BOM-less, before and after (CR count 0).

## §1 The numbers now shown (Acceptance table)

Two numbers are derived and shown. Each is read when the page is composed, formatted with `FText::AsNumber` + `FNumberFormattingOptions`, spliced by `FText::Format` (named arguments), and put in place of a `{#Name}` token inside `FSiegeControlsHelpRegistry::ComposeDetailForDisplay`. That is the one function every rendering reads (the page body and every related block), so the value is the same wherever the page appears.

| page | property · owner (cited by text) | object read, and why | live value (read 2026-09-28, PID 12112) | conversion / format | rendered sentence |
|---|---|---|---|---|---|
| `Interface.MapMarks` (rank 1) | `USiegeMapMarkSubsystem::MaxMapMarks`, public `UPROPERTY(EditDefaultsOnly …) int32 MaxMapMarks = 9;` ("⛔ JONATHAN'S CAP (`M-5`)"); the `AddMark` refusal ("Returns false … at MaxMapMarks") compares against it | **The class default object** (`GetDefault<USiegeMapMarkSubsystem>()`). The game's store is the instance `ULocalPlayer`'s subsystem collection creates from this class, and its value cannot differ from the CDO's: `EditDefaultsOnly`, no `Config` specifier, no asset, no writer anywhere in `Source/` (grep: only reads), and **no Blueprint child** (asset-registry `ParentClass`/`NativeParentClass` scan = `[]`; loaded subclasses = `[]`). Reading the default keeps the registry world-free, so the suite can compose every page headlessly. | `9` (class default) | none; a whole count, 0 fractional digits; noun chosen by the count (`{Count} {PluralCount}\|plural(one=circle,other=circles)`) | "You can hold up to 9 circles at once. At the limit a further click refuses out loud and tells you how many you are already holding, rather than doing nothing and looking broken." |
| `Cards.StackUpgrade` (rank 4) | `ABuilding::StackHealthStep` (protected, no getter), read through the existing **public static** `ABuilding::StackHealthMultiplier(int32)` at one upgrade. `ABuilding::ApplyStackUpgrade` applies exactly this on every upgrade: `MaxHP = OldMaxHP * StackHealthMultiplier(1);` | **`ABuilding`'s class default**, which is what that function reads (`const ABuilding* const Defaults = GetDefault<ABuilding>();`) for EVERY building. The step is game-wide by ruling (`StackHealthStep`'s own comment: "a BP child that re-authored it would be IGNORED by the series"). So this is the value the game applies, including the function's guard (a step below 1 applies 1). The step therefore does not differ across placeable classes; one value, not a per-building list (the "if a number differs across the classes" clause). | `1.5` (`ABuilding` CDO; also 1.5 on `ATower`, `ABarracks`, `ADeepMine`, `AClimbableTower` native CDOs, which the game does not read) | none; a plain multiplier, not a bonus or a distance; at most 2 fractional digits, no trailing zeros | "Health: each upgrade multiplies the building's maximum health by 1.5, compounding, and that half has no ceiling at all — it keeps climbing after the height has stopped." |

The live values in the table were read with the read-only inspector (§3). They are not typed anywhere in player prose; the only place they appear in the three files is inside comments that say "at today's value".

## §2 Left out, and why (spec (3): "a number that would need a new accessor in its owner's file is listed and left out, not built")

| candidate (rank) | property · owner | why it is out | value read, for the record | object the game uses (read) |
|---|---|---|---|---|
| `Cards.Discard` fee (2) | `ASiegePlayerController::DiscardAllCost` | `protected:` (`SiegePlayerController.h`, the section opened by the second `protected:`; the property is `UPROPERTY(EditDefaultsOnly, …) int32 DiscardAllCost = 20;`), **no public getter** | 20 (native CDO) | `ASiegeGameMode` sets `PlayerControllerClass = ASiegePlayerController::StaticClass();`, native, so no Blueprint can override it |
| `Hero.Rally` radius / bonus / duration / cooldown (3) | `AHeroCharacter::RallyRadius` / `RallySpeedBonus` / `RallyDuration` / `RallyCooldown` | `protected:` (`HeroCharacter.h`, the second `protected:` section), `BlueprintReadOnly` but **no C++ getter** | 600 uu / 0.25 / 5 s / 20 s | `BP_HeroCharacter` (the game mode's `HeroPawnClassAsset`, resolved privately by `ResolveHeroPawnClass`). It was already resident, `BS_UP_TO_DATE`; its CDO reads the same four values, so **no Blueprint override** |
| `Hero.Attack` reach / cone half-angle / cooldown (5) | `AHeroCharacter::MeleeRange` / `MeleeHalfAngleDegrees` / `MeleeCooldown` | same: protected, no getter | 150 uu / 30° / 0.5 s | `BP_HeroCharacter`, same values, no override |
| `Cards.StackUpgrade` per-building height limit (4, amended (8), his answer **A**) | `ABuilding::MaxStackHeightMultiplier` via the public `GetMaxStackHeightMultiplier()` — **that read is public**, but (8) requires the set of buildings to come from "the same card → class resolution the game uses", and that resolution is **private** | `ASiegePlayerController::ResolveCardActorClass` and `IsBuildingCard` are declared under `private:` in `SiegePlayerController.h`; the list `IsBuildingCard` reads, `BuildingEconomyCardIDs`, and the card table soft pointer `CardTableAsset` are `protected`; `ResolveCardRow` is private. The bot's copy (`ASiegeBotController::ResolveBotCardActorClass`) is also a non-public member. `CONVENTIONS` names the public de-duplication `FSiegeCardPathStatics::ComposeCardActorClassPath` (`TASK-959`) and `CardDataTablePath()` (`TASK-970`), and **neither exists** in `Source/`. A third copy of the path rule inside the help file would be a second resolver that could drift (and would still lack `BuildingEconomyCardIDs`). ⇒ (8)'s own clause applies: "leave the cap off, list it, report it." | see the table below | see the table below |

**⛔ OWED on his answer A**, for the manager to board. The cheapest unblock I can see, each needing a row with an owner-file fence:
- (a) make the class resolution public (build `TASK-959`'s `FSiegeCardPathStatics::ComposeCardActorClassPath` + `TASK-970`'s `CardDataTablePath()`, plus a public read of the economy-building list, for example a public static `IsBuildingCard` equivalent), then a `TASK-1576`-shaped row adds (8) with its tests (a-cap) / (c-cap) / (d-cap) and arms (cap-i) / (cap-ii) exactly as the amendment specifies;
- (b) for Discard / Rally / Attack: one-line public `const` getters in `SiegePlayerController.h` and `HeroCharacter.h` (the `GetWalkSpeed()` / `GetRecallChannelSeconds()` pattern those headers already use), then a follow-up to show them. `.h` edits in both owners mean a wide recompile.

**The per-building cap table, as far as it could be read without loading an asset** (the data (8) will need; nothing here is rendered):

| player-facing name (`FCardRow::DisplayName`, `DT_Cards`, already resident) | CardID | CardType | class the game spawns (composed path, CONVENTIONS) | native parent (asset-registry `NativeParentClass`) | native CDO `MaxStackHeightMultiplier` (read) | Blueprint override |
|---|---|---|---|---|---|---|
| Arrow Tower | ArrowTower | Building | `BP_Building_ArrowTower` | `ATower` | 5 | not read |
| Ballista Tower | BallistaTower | Building | `BP_Building_BallistaTower` | `ATower` | 5 | not read |
| Bomb Tower | BombTower | Building | `BP_Building_BombTower` | `ATower` | 5 | not read |
| Crystal Tower | CrystalTower | Building | `BP_Building_CrystalTower` | `ATower` | 5 | not read |
| Barracks | Barracks | Building | `BP_Building_Barracks` | `ABarracks` | 5 | not read |
| Wall | Wall | Building | `BP_Building_Wall` | `ABuilding` | 5 | not read |
| Deep Mine | DeepMine | Economy (building via `BuildingEconomyCardIDs = [DeepMine]`, read) | `BP_Building_DeepMine` | `ADeepMine` | 5 | not read |
| Watch Tower | WatchTower | Building | `BP_Building_WatchTower` | `AClimbableTower` | **2** | not read |

- Miner is `Economy` but not in `BuildingEconomyCardIDs`, so the game places it as a unit; it has no `BP_Building_` asset.
- The only C++ writer of the cap is `AClimbableTower`'s constructor (`MaxStackHeightMultiplier = 2;`); `ATower`, `ABarracks` and `ADeepMine` inherit `ABuilding`'s 5 (grep of `Source/`, plus the CDO reads).
- ⛔ **The Blueprint-override column is not read.** None of the eight `BP_Building_*` assets was resident (`find_object` = none), and loading one to read it could make a `BS_ERROR` Blueprint resident (`VER-§12` cl. 7g: residency re-arms the PIE-start modal, and the status is not known before the load). With the cap owed anyway, I did not take that risk. The row that lands (8) must read each class's defaults with its own residency plan.

## §3 The editor reads for (2) (read-only; no load, no PIE, no save, no compile, no map load)

- **Census by command line:** one `UnrealEditor.exe`, PID **12112**, `"…\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\GitProjects\…\GitClaudeUnrealTest.uproject"`, created 16:58:12; no `UnrealEditor-Cmd.exe` (no compile, cook or commandlet live).
- **Liveness + state:** `is_pie_active` → `is_active: false` (a game-thread call returning a real object). Editor world `/Game/Maps/L_MainMenu`; dirty content `[]`, dirty maps `[]`.
- **`VER-§12` cl. 7g walk** (`ObjectIterator(Blueprint)`, `status`): 37 resident, histogram `BS_UNKNOWN` 17 / `BS_UP_TO_DATE` 20, `BS_ERROR` **0**. Repeated at the end of my reads: 37 resident, `BS_ERROR` 0. My reads loaded nothing.
- **Reads:** asset registry (`get_assets` under `/Game/Blueprints/Buildings`, and a Blueprint-class scan for a `SiegeMapMarkSubsystem` parent), `find_object` residency checks, native CDO `get_editor_property` reads (`Building`, `Tower`, `Barracks`, `DeepMine`, `ClimbableTower`, `SiegeMapMarkSubsystem`, `SiegePlayerController`, `HeroCharacter`), the resident `DT_Cards` (`get_data_table_row_names` / `get_data_table_column_as_string`), and the resident `BP_HeroCharacter_C` CDO. Every value in §1 and §2 comes from these.

## §4 (7) The "ONE stated number" sentence: old → new

Three sites, not two. The third was found by text and carries the same claim, so I made it true too (it is a `.cpp` comment, inside the fence).

| site | old | new |
|---|---|---|
| `.h`, `FSiegeControlsHelpAction::Detail` doc (UHT ToolTip; the one-sentence `.h` widening) | "The ONE stated number in the whole registry is the war map's 30 gold, because Jonathan's own words are the source and they are quoted at the property (CommanderNpc.h:297-311)." | "The ONE number typed as a quantity in the whole registry is the war map's 30 gold, because Jonathan's own words are the source and they are quoted at the property (CommanderNpc.h:297-311); every other number a page shows is derived at runtime from the property that owns it, through a `{#Name}` token that ComposeDetailForDisplay replaces (TASK-1576, 2026-09-28)." |
| `.cpp`, the TASK-707 transfer block ("⛔ NO TUNABLE'S VALUE IS RE-TYPED") | "The ONE number stated anywhere below is the war map's 30 gold, because Jonathan's own words are the source and 704 quoted them at the property (CommanderNpc.h:297-311)." | "The ONE number TYPED as a quantity anywhere below is the war map's 30 gold, because Jonathan's own words are the source and 704 quoted them at the property (CommanderNpc.h:297-311). Every other number a page shows is DERIVED at runtime … (TASK-1576 …). The other digits typed in the prose are names and list labels, not quantities: "Key 1", "stage-1", the "1." to "3." stage labels and the chat box's "(1)" to "(4)". (Until TASK-1576 this read …)" |
| `.cpp`, `Interface.WarMapReveal`'s comment "⭐⭐ THE ONE NUMBER STATED IN THE WHOLE REGISTRY …" (found by text) | unchanged, and a dated note is added under it | "⭐ TASK-1576 (2026-09-28): read "STATED" above as TYPED. Two pages now SHOW a number … none is typed; this quoted 30 gold remains the one quantity typed into the registry. …" |

**Why "as a quantity":** the digit census of the registry's literals is **9 lines before and 9 after** (`digits.py`, the same count `qa/TASK-1575.md` §3 made): "Key 1", "stage-1", "1." / "2." / "3.", "(1)"–"(4)" on three lines, and the quoted "30 gold". Only the 30 is a quantity. Saying "the ONE typed number" without that qualifier would be false about "Key 1".

## §5 The two stale comments (amended (8)), made true and dated

1. **"⛔ Whether the page shows each building's height limit as a number is 🧑 Jonathan's open question Q-STACK-CAP-2026-09-28, owned by TASK-1576 — ⛔ this row adds no per-building limit."** Now: he answered A, "show the numbers" (`TASK-1589`), so the page is to show each type's limit; it does not yet, because the card-to-class resolution it must read through is private (`ResolveCardActorClass` / `IsBuildingCard` / `BuildingEconomyCardIDs`), a new accessor is outside this fence, and a copied path rule would be a second resolver; the limits are OWED (this handoff). The old sentence is quoted at the end.
2. **"(the cap's number, if any, is TASK-1576's on 🧑 Q-STACK-CAP-2026-09-28)"**. Kept, with an appended dated clause: he answered A, and the per-building limits are OWED rather than shown, for the same reason; it points at note 1.

Neither is a `TASK-1585` pin. I also added dated `TASK-1576` notes (not rewrites) under two comments that the change made incomplete: `Interface.MapMarks`'s "⛔ NO NUMBER IS TYPED: the cap is "a limited number" … no coordinate, radius or count is described" and `Cards.StackUpgrade`'s "⛔ NO TUNABLE'S VALUE IS TYPED … "a set factor"".

## §6 Tests (spec (4)) and `N`

**Added: test 20, `Siegebound.ControlsHelp.ShownNumbersAreReadFromTheirOwners` (`FSiegeControlsHelpDerivedNumbersTest`)**, appended after test 19, with a 5-line index entry in the file header. Each owner is read in the test from the same object the composer reads (`GetDefault<USiegeMapMarkSubsystem>()->MaxMapMarks`, `ABuilding::StackHealthMultiplier(1)`) and formatted in the test with its own options (count: 0 fractional digits; factor: at most 2), so a composer that formats the wrong value cannot also supply the expectation.
- **(a)** per number, `ComposeDetailForDisplay(Row)` contains the value inside its own clause: `"hold up to " + <cap> + " circle"` (a prefix of both noun forms) and `"maximum health by " + <factor> + ", compounding"`. Not merely a digit somewhere on the page.
- **(a2)** per number, EVERY rendering carries it: the row's own `ComposeDetailContent` body and every related block that renders the row, found by walking all rows (null layout, no applied keys, test 13's lane). One `TestEqual(showing, renderings)` per number, plus a vacuity guard `renderings > 0`. Simulated: `Interface.MapMarks` 3 renderings (own, `Interface.WarMap`, `Cards.PlacementResize`), `Cards.StackUpgrade` 2 (own, `Cards.PlacementResize`).
- **(b)** a negative control per number: the same clause from a different value (`cap + 1`, `factor + 1`) is absent, with a fixture self-check that the two clauses differ. This is what makes (a) discriminate a wrong number rather than match any number.
- **(d)** no `{#` survives in any row's composed detail, plus a vacuity guard: at least one template carries a `{#` token (2 at these bytes).

**Narrowed (4)(b): test 15 (e)'s detail assertion** (`TowerAndMapMarkRowsAreAuthoredAndRawLaned`). It read `CarriesADigit(Detail)` with `Detail = ComposeDetailForDisplay(*Row)`, the composed page, which now legitimately carries derived digits. It now reads `CarriesADigit(Row->Detail.ToString())`, the typed template, and its label says so ("⛔ Row '%s' detail TEMPLATE types NO number either - …"). The one-liner assertion is unchanged. That is the only narrowed assertion: test 14 (e) scans `Cards.Discard`, which shows no number, and no other test scans for digits (grep of the test file for `IsDigit` / `CarriesADigit`: tests 14 and 15 only).

**(c) The `::` / `()` guard stays green.** Test 9 reads `ComposeDetailForDisplay`, so it now scans the composed text with the numbers in: 0 hits (simulated). The format patterns live outside every `Row.Detail` and no rendered text contains a pattern.

**Simulation, at these bytes** (`sim.py`: parses the 27 rows' `TEXT` runs, splices the numbers at the read values, resolves key tokens, runs tests 9, 10 (b), 15 (e), 19 (b) and 20's text checks): **0 failures**. On the start bytes the same run gives exactly test 20's 5 failures and nothing else, so test 20 is red on the text it replaces.

**`N` = 567 + 1 = 568.** One test added; no existing test changed shape (test 15 changed one assertion's operand and label). `IMPLEMENT_SIMPLE_AUTOMATION_TEST` in this file: 19 → 20.

## §7 The mutation arms `TASK-1578` owes (`SHIP-§9`): 6

Run one at a time, as byte replaces on BOM-less UTF-8 (⛔ no `Get-Content` / `Set-Content`, ⛔ no `-replace`; `qa/TASK-1575.md` §5's method). Before each injection assert the anchor count == 1 in `Source/` **at `qa/TASK-1577.md`'s bytes** and check the byte delta; restore by byte copy and re-hash equal. Every anchor below was counted at MY bytes by `arms.py` (bytes, all of `Source/`): **1 each**. All six need a recompile of the mutant (they are C++).

| arm | file | anchor (occurs once in `Source/`) | replace once with | bytes | expect |
|---|---|---|---|---|---|
| **1576-T** (typed digit) | `SiegeControlsHelpWidget.cpp` | `TEXT("You can hold up to {#MapMarks.Cap} at once. At the limit a further click refuses out loud and ")` | `TEXT("You can hold up to 9 circles at once. At the limit a further click refuses out loud and ")` | **−6** (317440 → 317434) | exactly one red test, **test 15**, **1 error** |
| **1576-D1** (data-follow, wrong property) | `SiegeControlsHelpWidget.cpp` | `const int32 HelpMapMarkCap = MarkStoreDefaults->MaxMapMarks;` | `const int32 HelpMapMarkCap = static_cast<int32>(MarkStoreDefaults->MinMarkRadiusUU);` | **+24** | exactly one red test, **test 20**, **2 errors** |
| **1576-D2** (data-follow, wrong term of the series) | `SiegeControlsHelpWidget.cpp` | `const float HelpStackHealthFactor = ABuilding::StackHealthMultiplier(1);` | `const float HelpStackHealthFactor = ABuilding::StackHealthMultiplier(2);` | **0** (same length; the sha moves) | exactly one red test, **test 20**, **2 errors** |
| **1585-A** (unchanged, re-counted) | `SiegeControlsHelpWidget.cpp` | `The size you dial in with the wheel applies to what you PLACE, not to what you GROW: an ` | (as `handoffs/TASK-1585-programmer.md` §2) | **+77** | test 19, 2 errors (as ruled) |
| **1585-B** (unchanged, re-counted) | `SiegeControlsHelpWidget.cpp` | `It has two other jobs elsewhere: while you are placing a building it can resize that building, and on the war map it resizes one of your own map circles.` | `It is inert everywhere except inside a pick.` | **−109** | test 19, 3 errors (as ruled) |
| **1574-guard** (unchanged, re-counted) | `SiegeControlsHelpWidget.cpp` | `Falling out of the world is a death, not a despawn` (ASCII form `TEXT("Falling out of the world is a death, not a despawn ` also 1) | (as `qa/TASK-1575.md` §5) | **+33** | test 9, 2 errors (as ruled) |

**Moved pins: none.** `TASK-1585`'s three anchors are byte-identical and still 1 each; my edits are in other literals of the same rows (the `Cards.StackUpgrade` splice is in "WHAT AN UPGRADE BUYS", not the last paragraph).

**Expected error lines** (UE formats: `TestFalse` "Expected '%s' to be false.", `TestTrue` "… to be true.", `TestEqual(int32)` "Expected '%s' to be %d, but it was %d."; the digits shown assume the editor's `en` culture):
- **1576-T:** `Expected '⛔ Row 'Interface.MapMarks' detail TEMPLATE types NO number either - a number the page shows is read from its owner when the page is composed' to be false.`
  - ⭐ **The point of this arm:** the typed `9` equals the owner today, so **test 20 stays green** (its (a), (a2), (b), (d) and the vacuity guard all hold: `Cards.StackUpgrade` still carries a token). A typed number that happens to be right is invisible to a data-follow test; the narrowed template assertion is the only thing that sees it. Two red tests would mean the arm leaked.
- **1576-D1** (renders "hold up to 250 circles"):
  - `Expected 'Row 'Interface.MapMarks' shows the map-circle cap (the map-mark store's MaxMapMarks, class default) as its owner holds it: 'hold up to 9 circle'' to be true.`
  - `Expected 'Every rendering of row 'Interface.MapMarks' shows the map-circle cap (the map-mark store's MaxMapMarks, class default) (its own page and each related block that renders it)' to be 3, but it was 0.`
  - (b)'s control ("hold up to 10 circle") stays absent; test 15 (e) reads the template, which the arm does not touch.
- **1576-D2** (renders "by 2.25, compounding"):
  - `Expected 'Row 'Cards.StackUpgrade' shows the stack health factor (StackHealthMultiplier at one upgrade) as its owner holds it: 'maximum health by 1.5, compounding'' to be true.`
  - `Expected 'Every rendering of row 'Cards.StackUpgrade' shows the stack health factor (StackHealthMultiplier at one upgrade) (its own page and each related block that renders it)' to be 2, but it was 0.`

**Reading rule.** 1576-T: exactly 1 error in test 15. 1576-D1 / D2: exactly 2 errors in test 20; 1 error means (a) or (a2) is dead. Any other red test means the arm leaked ⇒ STOP.

**No leak into test 20 from `TASK-1585`'s arms** (the check `qa/TASK-1586.md` ruling 5 (ii) asked for): simulated, 1585-A and 1585-B fail only test 19's text checks, and test 20's (a) / (a2) / (d) stay green on both mutants; the guard arm fails only test 9's two fragments.

## §8 The 5a shape (`TASK-1578`)

- **The `.h` changed in one doc comment** (`FSiegeControlsHelpAction::Detail`), +3 lines. UHT's `Comment` / `ToolTip` metadata for `Detail` changes in `SiegeControlsHelpWidget.gen.cpp`, with its CRCs. Measured on the file:
  - `UCLASS()` 447 / 630 / 930 → **450 / 633 / 933**;
  - `GENERATED_BODY()` 450 / 633 / 933 → **453 / 636 / 936** (the struct's at 127 is unmoved), so the `.generated.h` line macros `_h_450/_h_633/_h_933` → `_h_453/_h_636/_h_936`;
  - **exec-symbol set 9 = 9**: the `.h` still declares exactly 9 `UFUNCTION`s (grep finds 10 `UFUNCTION(` hits; the 10th is the `TASK-1496` comment at line 684, unchanged).
- The widget `.cpp` gains two includes (`Siegebound/Building.h`, `Siegebound/SiegeMapMarkSubsystem.h`), both headers of this module. The test file gains the same two.
- New anonymous-namespace symbols (`FSiegeHelpDerivedNumber`, `ComposeHelpMapMarkCap`, `ComposeHelpStackHealthFactor`, `HelpDerivedNumbers`, `SpliceDerivedNumbers`) and the test class name are unique in `Source/` (grep), so a unity blob cannot collide on them.
- Relaunch: the widget is on the PIE path, so a C++ relaunch is owed as usual.

## §9 Size valve (spec (5))

**Not tripped.** All five pages were worked in rank order in one session. Ranks 1 and 4 (health factor) were built; ranks 2, 3 and 5, and rank 4's per-building cap, are left out by spec (3) / (8) for lack of a public accessor (§2), not by the valve. The cap is listed as **OWED on his answer A**, as (8)'s valve clause asks, though the cause is access rather than session length.

## §10 `qa/TASK-1586.md` NIT N2 (the "Stack a tower taller" headline and "makes that one taller"): ruled OUT

QA's trigger was "once `TASK-1576` shows the tower's cap, the tower's second click (health only) contradicts 'taller' on the same page". This row does not show any cap, so the trigger does not fire here, and a headline change is outside this row's subject (numbers). The contradiction QA describes will land with the owed cap; the row that shows the per-building limits should take N2 in the same edit (for example a building-neutral headline and "the click upgrades that one"). Test 16 only needs the three wheel rows' headlines distinct, so either wording is safe for it.

## §11 Files touched, sha256 before → after

| file | before | after | size / lines |
|---|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` | `9b078d0ec6a51f4439ce8d0bfa9ab1bb39fdcc24dfdd52a8c426de3ea8b4020d` | `6a9ba04491fea8b58b4538fe8e9496a674db0c23fa4478bdf48fa3679a29fa6b` | 304435 → 317440 B · 4771 → 4977 |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h` | `31c986935a49b859e20212c164d148f0865993417ed39c4be89d150adcd7bf4e` | `7ed5a066b6c5f608a40b23a95ce858db786bdad0e5565eb60cf2583f8a4d1527` | 81619 → 81822 B · 1409 → 1412 |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` | `c45412c9cd2597fdb2f836b749fc0753c5686b8833a7c5f9d7a43e914748b38b` | `588a4180553851f7c52da9e4907aac07d522b2c67db1a9e6121c1c6790de3fa4` | 160530 → 172078 B · 2790 → 2981 |

`SiegePlayerController.cpp` is untouched (it should reach 5a at `qa/TASK-1586.md`'s `d2dfbf50…b1fd`).

**Page length, for `TASK-1579` (5b page fit):** every affected page **shrinks**. "a limited number of circles" (27 chars) → "up to 9 circles" (15): `Interface.MapMarks` −12, and the pages that render it as a block (`Interface.WarMap`, `Cards.PlacementResize`) −12 each. "a set factor" (12) → "1.5" (3): `Cards.StackUpgrade` −9, and `Cards.PlacementResize` −9 more (−21 total). The two new sentences 5b should see painted with the live values: "You can hold up to 9 circles at once." and "…maximum health by 1.5, compounding…".

## §12 Found and reported, not edited

- **`.h` `ComposeDetailForDisplay` doc:** "The detail prose, or the same pinned TODO string." It is still true (it returns the prose), but it no longer says the prose's number tokens are replaced there. The `.cpp` definition carries the full comment. The `.h` fence here was one sentence of the `Detail` doc, so I left it; a one-clause amendment could ride the next `.h` edit.
- **N2** (§10), carried to the row that shows the caps.
- **The 30 gold is quoted, not derived.** `ACommanderNpc::EnemyRevealCost` is its owner, and deriving it would change a quotation of Jonathan's words; it was not a candidate here (the handoff `TASK-1541` §8 item 6 list is OUT). Recorded so nobody reads (7)'s "one typed quantity" as an oversight.

## What QA (`TASK-1577`) should scrutinize

1. **Is `ABuilding::StackHealthMultiplier(1)` an "existing public member or getter" for `StackHealthStep`?** It is a public static that already exists, and it is literally the per-upgrade factor the game applies (`ApplyStackUpgrade`). It is not a getter of the raw property: a step below 1 reads back as 1, which is what the game applies. I think that is the right thing to show; rule on it.
2. **The object choice for the cap** (the CDO, not the live `ULocalPlayer` subsystem): §1's reasons. The live instance is created from the class and nothing writes the property, so they cannot differ; reading the live one would need a world threaded into the pure registry, which the `.h` fence does not allow.
3. **The accessor rulings in §2:** that each left-out number is really behind `protected` / `private` with no public getter, and that a help-side copy of the composed-path rule is correctly declined for (8).
4. **The `FText` shapes, since nothing was compiled:** `FFormatNamedArguments::Add(TEXT("…"), FText)` / `Add(TEXT("…"), int32)` and `FText::Format(FText, FFormatNamedArguments)` (engine precedents: `Args.Add(TEXT("AssetCount"), FText::AsNumber(…))` in `SPrivateAssetsDialog.cpp`, `Args.Add(TEXT("Count"), X.Num())` in `FindInBlueprintManager.cpp`); the plural modifier on a named int argument (`FileHelpers.cpp`'s `{0}|plural(one=error,other=errors)` is the ordered form); `FText::AsNumber(int32 / float, const FNumberFormattingOptions*)`; the constant-initialised table of `{const TCHAR*, function pointer}`.
5. **Arm reading rules** (§7), especially 1576-T's "test 20 stays green by design".
6. **No digit in any new player literal** (the two changed literals carry tokens, not digits; the literal-digit census is 9 = 9) and no `::` / `()` in any composed page (simulated 0).

## Not examined / limitations

- ⛔ **No compile, no suite run, no PIE, no pixels.** `N = 568`, "0 failures at these bytes", every arm's error count and the rendered sentences are text-level predictions from `sim.py` / `arms.py`, which splice the numbers at the values the inspector read and assume the editor's `en` number formatting ("1.5", "9"). `TASK-1578` measures them.
- **The Blueprint-override question for the eight building classes is not answered** (§2): none was resident and I did not load them (`VER-§12` cl. 7g). It only matters for the owed cap; the shown health factor is read from `ABuilding`'s CDO by the game itself, so no override can change it.
- **The plural form** is chosen by the current culture's plural rules. At today's cap of 9 it reads "circles" in every culture I know of; the "1 circle" branch was not exercised.
- **Culture:** in a culture with a decimal comma the factor renders "1,5". The test formats the same way, so it holds; whether "by 1,5, compounding" reads well there is a localisation question this English-only screen has not had before.
- The live values were read from a running binary built before this row; the C++ defaults they come from are unchanged by it.
- Line numbers in this handoff are hints at these bytes; every site is cited by text.

## `git --no-optional-locks diff --no-index -U0` against the amended (0) anchors (scratch start copies, sha256 equal to §0)

```diff
diff --git a/start/W.start.h b/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h
index 016de72..0501e69 100644
--- a/start/W.start.h
+++ b/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h
@@ -177,2 +177,5 @@ struct FSiegeControlsHelpAction
-	 *  stated number in the whole registry is the war map's 30 gold, because Jonathan's own
-	 *  words are the source and they are quoted at the property (CommanderNpc.h:297-311).
+	 *  number typed as a quantity in the whole registry is the war map's 30 gold, because
+	 *  Jonathan's own words are the source and they are quoted at the property
+	 *  (CommanderNpc.h:297-311); every other number a page shows is derived at runtime from the
+	 *  property that owns it, through a `{#Name}` token that ComposeDetailForDisplay replaces
+	 *  (TASK-1576, 2026-09-28).
diff --git a/start/W.start.cpp b/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
index 6a25ca9..fb0121d 100644
--- a/start/W.start.cpp
+++ b/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
@@ -24,0 +25,5 @@
+// TASK-1576 (2026-09-28) — the owners of the two numbers the pages now SHOW, read and never typed
+// (`HELP-§2`): ABuilding for the stack health factor (its public static StackHealthMultiplier) and
+// USiegeMapMarkSubsystem for the map-circle cap (its public MaxMapMarks). Both are READ-ONLY to this
+// file; see the derived-number block in the anonymous namespace below.
+#include "Siegebound/Building.h"
@@ -25,0 +31 @@
+#include "Siegebound/SiegeMapMarkSubsystem.h"
@@ -105,0 +112,11 @@ namespace SiegeControlsHelpText
+	/**
+	 *  ⭐ TASK-1576 (2026-09-28) — the opening of a DERIVED-NUMBER token, e.g. `{#MapMarks.Cap}`.
+	 *  A second token kind in the same prose, told apart from a `{ActionId}` key token by the `#`.
+	 *  ComposeDetailForDisplay replaces each one with a number READ from the property that owns it
+	 *  (the derived-number block in the anonymous namespace below), BEFORE ResolveDetailTokens ever
+	 *  sees the text, so no key-token code path changes. ⛔ A number token naming nothing in that
+	 *  block is LEFT VISIBLE, exactly like a misspelled key token (`HELP-§2` mechanism 2), and the
+	 *  suite's "no unresolved token" scans read the composed text, so they catch it.
+	 */
+	static const TCHAR* NumberTokenOpen = TEXT("{#");
+
@@ -286,0 +304,140 @@ namespace
+
+	// ════════════════════════════════════════════════════════════════════════════════════
+	//  ⭐⭐ TASK-1576 (2026-09-28) — THE NUMBERS A PAGE SHOWS, EACH READ FROM ITS OWNER.
+	//
+	//  `HELP-§2`: "NO NUMBER IS RESTATED IN PROSE IF IT CAN BE READ FROM DATA ... Prefer deriving".
+	//  The prose carries a `{#Name}` token (SiegeControlsHelpText::NumberTokenOpen) where a number
+	//  goes; ComposeDetailForDisplay hands the text to SpliceDerivedNumbers, which asks the entry
+	//  below for that token to READ the owning property at that moment, format it in player units
+	//  (FText::AsNumber with fixed fractional digits, spliced by FText::Format), and put the result
+	//  in place of the token. ⇒ a retune of the owner changes the page with ⛔ no text edit, and
+	//  ⛔ no digit of either value is typed anywhere in this file.
+	//
+	//  ⛔ READS GO THROUGH EXISTING PUBLIC MEMBERS ONLY (TASK-1576 spec (3)). The row's other
+	//  candidates (the discard fee, the four Rally values, the melee reach / cone / cooldown, and
+	//  each building's own height limit) are NOT here because reading them needs a new accessor in
+	//  an owner file: DiscardAllCost and the hero's Melee* / Rally* tunables are protected with no
+	//  public getter, and the card-to-building-class resolution the game uses
+	//  (ASiegePlayerController::ResolveCardActorClass / IsBuildingCard) is private. They are listed
+	//  in handoffs/TASK-1576-programmer.md for the manager to board; ⛔ none is typed instead.
+	//
+	//  WHICH OBJECT EACH ENTRY READS, AND WHY (spec (2): the object the game uses, ⛔ never simply
+	//  the easiest one to reach):
+	//    • the map-circle cap: USiegeMapMarkSubsystem's class default object. The game's store is
+	//      the instance ULocalPlayer's subsystem collection creates FROM THIS CLASS, and its
+	//      MaxMapMarks is the CDO's value: the property is EditDefaultsOnly on a class with no
+	//      Config specifier and no asset, nothing in Source/ assigns it, and no Blueprint child of
+	//      the class exists (asset registry and loaded classes both read empty, TASK-1576). The
+	//      live instance therefore cannot differ, and reading the default keeps the registry free
+	//      of any world, which is what lets the suite compose every page headlessly.
+	//    • the stack health factor: ABuilding::StackHealthMultiplier(1), the existing public
+	//      static. ABuilding::ApplyStackUpgrade multiplies MaxHP by exactly this value on every
+	//      upgrade (`MaxHP = OldMaxHP * StackHealthMultiplier(1);`), and the function reads
+	//      ABuilding's own class default for EVERY building (StackHealthStep is game-wide by
+	//      ruling; its own comment says a Blueprint child's value would be IGNORED). ⇒ this is the
+	//      value the game applies, including the function's guard (a step below 1 applies 1).
+	// ════════════════════════════════════════════════════════════════════════════════════
+
+	/**
+	 *  One number a page may show: the token the prose writes, and the function that reads the
+	 *  owner and formats the value. Compose returns false when the owner cannot be read, and the
+	 *  token is then LEFT VISIBLE (`HELP-§2` mechanism 2), ⛔ never replaced by a guessed value.
+	 *  ⚠️ A plain aggregate of two constant pointers, so the table below is constant-initialised
+	 *  and adds no dynamic initialiser to this translation unit (the MakeListPanelMargin note).
+	 */
+	struct FSiegeHelpDerivedNumber
+	{
+		const TCHAR* Token;
+		bool (*Compose)(FText& OutNumberText);
+	};
+
+	/**
+	 *  `{#MapMarks.Cap}` — how many circles the war map holds at once, e.g. "9 circles".
+	 *  Owner: USiegeMapMarkSubsystem::MaxMapMarks (public UPROPERTY; the `AddMark` refusal compares
+	 *  against it). Read from the class default object; the reason is in the block comment above.
+	 */
+	bool ComposeHelpMapMarkCap(FText& OutNumberText)
+	{
+		const USiegeMapMarkSubsystem* const MarkStoreDefaults = GetDefault<USiegeMapMarkSubsystem>();
+		if (MarkStoreDefaults == nullptr)
+		{
+			return false;
+		}
+
+		const int32 HelpMapMarkCap = MarkStoreDefaults->MaxMapMarks;
+
+		// Conversion: none. It is a count of circles, so it is shown as a whole number with no
+		// fractional digits. The noun is chosen by the count itself (the plural argument), so a
+		// retune to one reads "1 circle".
+		FNumberFormattingOptions CountOptions;
+		CountOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(0);
+
+		FFormatNamedArguments NumberArgs;
+		NumberArgs.Add(TEXT("Count"), FText::AsNumber(HelpMapMarkCap, &CountOptions));
+		NumberArgs.Add(TEXT("PluralCount"), HelpMapMarkCap);
+		OutNumberText = FText::Format(
+			FText::FromString(FString(TEXT("{Count} {PluralCount}|plural(one=circle,other=circles)"))), NumberArgs);
+		return true;
+	}
+
+	/**
+	 *  `{#StackUpgrade.HealthFactor}` — what each stack upgrade multiplies a building's maximum
+	 *  health by, e.g. "1.5". Owner: ABuilding::StackHealthStep (protected, no getter), read
+	 *  through the existing PUBLIC static ABuilding::StackHealthMultiplier at one upgrade, which
+	 *  is the factor ApplyStackUpgrade applies; the reason is in the block comment above.
+	 */
+	bool ComposeHelpStackHealthFactor(FText& OutNumberText)
+	{
+		const float HelpStackHealthFactor = ABuilding::StackHealthMultiplier(1);
+
+		// Conversion: none. It is a multiplier, not a bonus or a distance, so it is shown as the
+		// factor itself ("by 1.5"), with at most two fractional digits and no trailing zeros.
+		FNumberFormattingOptions FactorOptions;
+		FactorOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(2);
+
+		FFormatNamedArguments NumberArgs;
+		NumberArgs.Add(TEXT("Factor"), FText::AsNumber(HelpStackHealthFactor, &FactorOptions));
+		OutNumberText = FText::Format(FText::FromString(FString(TEXT("{Factor}"))), NumberArgs);
+		return true;
+	}
+
+	/** Every number a page may show. ⛔ One entry per token; the prose names the token, this names the owner. */
+	const FSiegeHelpDerivedNumber HelpDerivedNumbers[] =
+	{
+		{ TEXT("{#MapMarks.Cap}"),              &ComposeHelpMapMarkCap },
+		{ TEXT("{#StackUpgrade.HealthFactor}"), &ComposeHelpStackHealthFactor }
+	};
+
+	/**
+	 *  Replaces every known `{#Name}` token in DetailText with its freshly read, formatted number.
+	 *  Text with no `{#` is returned untouched (the cheap gate ResolveDetailTokens also uses), and
+	 *  an unknown or unreadable token stays in the text, visibly.
+	 */
+	FText SpliceDerivedNumbers(const FText& DetailText)
+	{
+		FString Working = DetailText.ToString();
+		if (!Working.Contains(SiegeControlsHelpText::NumberTokenOpen, ESearchCase::CaseSensitive))
+		{
+			return DetailText;
+		}
+
+		bool bReplacedAny = false;
+		for (const FSiegeHelpDerivedNumber& Number : HelpDerivedNumbers)
+		{
+			if (!Working.Contains(Number.Token, ESearchCase::CaseSensitive))
+			{
+				continue;
+			}
+
+			FText NumberText;
+			if (!Number.Compose(NumberText))
+			{
+				continue;
+			}
+
+			Working.ReplaceInline(Number.Token, *NumberText.ToString(), ESearchCase::CaseSensitive);
+			bReplacedAny = true;
+		}
+
+		return bReplacedAny ? FText::FromString(Working) : DetailText;
+	}
@@ -350,2 +507,10 @@ namespace
-//     The ONE number stated anywhere below is the war map's 30 gold, because Jonathan's own
-//     words are the source and 704 quoted them at the property (CommanderNpc.h:297-311).
+//     The ONE number TYPED as a quantity anywhere below is the war map's 30 gold, because
+//     Jonathan's own words are the source and 704 quoted them at the property
+//     (CommanderNpc.h:297-311). Every other number a page shows is DERIVED at runtime: the
+//     prose carries a `{#Name}` token and ComposeDetailForDisplay replaces it with the value read
+//     from the property that owns it (TASK-1576, 2026-09-28: the map-circle cap and the stack
+//     health factor; the derived-number block near the top of this file). The other digits
+//     typed in the prose are names and list labels, not quantities: "Key 1", "stage-1", the
+//     "1." to "3." stage labels and the chat box's "(1)" to "(4)". (Until TASK-1576 this read
+//     "The ONE number stated anywhere below is the war map's 30 gold", which stopped being true
+//     the moment a page showed a derived number.)
@@ -889,3 +1054,13 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// fail (STACK-§2), and ⛔ no building is named here. ⛔ Whether the page shows each
-			// building's height limit as a number is 🧑 Jonathan's open question
-			// Q-STACK-CAP-2026-09-28, owned by TASK-1576 — ⛔ this row adds no per-building limit.
+			// fail (STACK-§2), and ⛔ no building is named here. ⭐ TASK-1576 (2026-09-28): 🧑 Jonathan
+			// answered Q-STACK-CAP-2026-09-28 with A, "show the numbers" (TASK-1589), so the page is
+			// to show each building type's own height limit. ⛔ It does NOT show them yet, and the
+			// reason is access, not choice: each value must be read from the class the game really
+			// places for that card, found by the game's own card-to-class resolution, and that
+			// resolution (ASiegePlayerController::ResolveCardActorClass with IsBuildingCard, plus the
+			// BuildingEconomyCardIDs list it reads) is private to the controller. Reading it from here
+			// needs a new accessor in an owner file, which TASK-1576's fence forbids (its spec (3) and
+			// (8)), and a copy of the path rule here would be a second resolver that could drift. ⇒ the
+			// per-building limits are OWED on his answer A (handoffs/TASK-1576-programmer.md), and
+			// ⛔ this row still states no per-building limit. (Until TASK-1576 this sentence read
+			// "Whether the page shows each building's height limit as a number is Jonathan's open
+			// question Q-STACK-CAP-2026-09-28, owned by TASK-1576".)
@@ -898,0 +1074,9 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// ⭐ TASK-1576 (2026-09-28): the health factor is now SHOWN, and it is still not typed. The
+			// prose carries the `{#StackUpgrade.HealthFactor}` number token where it said "a set
+			// factor", and ComposeDetailForDisplay replaces it with ABuilding::StackHealthMultiplier at
+			// one upgrade, read when the page is composed: the factor ApplyStackUpgrade applies on
+			// every upgrade, read off ABuilding's own class default for every building (the
+			// derived-number block near the top of this file says why that is the object the game
+			// uses). It renders "multiplies the building's maximum health by 1.5, compounding" at
+			// today's value. The height half keeps "a set maximum multiple" (the per-building limits
+			// are owed, above). Pinned by test 20 (ShownNumbersAreReadFromTheirOwners).
@@ -913 +1097 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("not touched. Health: each upgrade multiplies the building's maximum health by a set factor, ")
+				TEXT("not touched. Health: each upgrade multiplies the building's maximum health by {#StackUpgrade.HealthFactor}, ")
@@ -941 +1125,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// cap's number, if any, is TASK-1576's on 🧑 Q-STACK-CAP-2026-09-28). The sentence after
+			// cap's number, if any, is TASK-1576's on 🧑 Q-STACK-CAP-2026-09-28; ⭐ TASK-1576,
+			// 2026-09-28: he answered A, "show the numbers", and the per-building limits are OWED
+			// rather than shown, because the class resolution they must be read through is private to
+			// ASiegePlayerController; see the TASK-1576 note in the citation block above). The sentence after
@@ -1757,0 +1945,5 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// ⭐ TASK-1576 (2026-09-28): read "STATED" above as TYPED. Two pages now SHOW a number
+			// (the map-circle cap and the stack health factor), but each is read from its owner when
+			// the page is composed, through a `{#Name}` token, and none is typed; this quoted 30 gold
+			// remains the one quantity typed into the registry. (It is quoted, not derived, and the
+			// reveal fee's owner, ACommanderNpc::EnemyRevealCost, was not a TASK-1576 candidate.)
@@ -1871,0 +2064,8 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// ⭐ TASK-1576 (2026-09-28): the cap is now SHOWN, and it is still not typed. "You can hold a
+			// limited number of circles at once" became "You can hold up to" + the `{#MapMarks.Cap}`
+			// number token + "at once", and ComposeDetailForDisplay replaces the token with
+			// USiegeMapMarkSubsystem::MaxMapMarks, read off the class default when the page is composed
+			// and formatted as a whole count with its noun ("9 circles" at today's value; "1 circle" if
+			// it were ever one). The count that is described is therefore the store's own cap, the one
+			// the AddMark refusal compares against; ⛔ no coordinate or radius is described, as before.
+			// Pinned by test 20 (ShownNumbersAreReadFromTheirOwners).
@@ -1901 +2101 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("You can hold a limited number of circles at once. At the limit a further click refuses out loud and ")
+				TEXT("You can hold up to {#MapMarks.Cap} at once. At the limit a further click refuses out loud and ")
@@ -2125 +2325,7 @@ FText FSiegeControlsHelpRegistry::ComposeDetailForDisplay(const FSiegeControlsHe
-	return Row.Detail;
+
+	// ⭐ TASK-1576 (2026-09-28): every `{#Name}` number token is replaced here with the number READ
+	// from its owner at this call (the derived-number block in the anonymous namespace above). This
+	// is the one function every rendering of a page reads (the page's own body and each related
+	// block both come through it), so a number shows the same value wherever the page appears.
+	// Row.Detail itself stays the TYPED template, which is what the suite scans for typed digits.
+	return SpliceDerivedNumbers(Row.Detail);
diff --git a/start/T.start.cpp b/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
index c265a03..4acd21f 100644
--- a/start/T.start.cpp
+++ b/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
@@ -6,0 +7,3 @@
+// TASK-1576 test 20: the two owners whose numbers the pages now show, read here from the SAME
+// objects the composer reads (ABuilding::StackHealthMultiplier, USiegeMapMarkSubsystem's defaults).
+#include "Siegebound/Building.h"
@@ -14,0 +18 @@
+#include "Siegebound/SiegeMapMarkSubsystem.h"
@@ -28,0 +33,6 @@
+ *  ⭐⭐ TEST 20 = TASK-1576: the numbers the pages SHOW (the map-circle cap and the stack health
+ *  factor) are read from the properties that own them, in the same objects the composer reads,
+ *  on every page that renders them; and no number token is left unresolved. Test 15 (e)'s
+ *  "types NO number" detail check was narrowed by the same task to the TYPED template, so a
+ *  derived number passes it and a typed digit still fails it.
+ *
@@ -1963,0 +1974,7 @@ bool FSiegeControlsHelpTowerRowsTest::RunTest(const FString& Parameters)
+	// ⭐ TASK-1576 (2026-09-28): two of these pages now SHOW a number (the map-circle cap on
+	// Interface.MapMarks, the health factor on Cards.StackUpgrade), each READ from its owner when
+	// the page is composed. ⇒ (e)'s DETAIL check is narrowed to what is TYPED: the row's source
+	// template (Row->Detail, before ComposeDetailForDisplay replaces its `{#Name}` tokens). A
+	// derived number passes it and a typed digit still fails it; that the shown number is the
+	// owner's is test 20's claim, not this one's. The one-liner check is unchanged (no one-liner
+	// shows a number).
@@ -2071,2 +2088,4 @@ bool FSiegeControlsHelpTowerRowsTest::RunTest(const FString& Parameters)
-		TestFalse(*FString::Printf(TEXT("⛔ Row '%s' detail page types NO number either"), Expected.ActionId),
-			CarriesADigit(Detail));
+		// ⭐ NARROWED BY TASK-1576 to the typed template (see the scanner's note above): it read
+		// `CarriesADigit(Detail)`, the COMPOSED page, which now legitimately carries derived digits.
+		TestFalse(*FString::Printf(TEXT("⛔ Row '%s' detail TEMPLATE types NO number either - a number the page shows is read from its owner when the page is composed"), Expected.ActionId),
+			CarriesADigit(Row->Detail.ToString()));
@@ -2789,0 +2809,172 @@ bool FSiegeControlsHelpRefutedRulesTest::RunTest(const FString& Parameters)
+// ════════════════════════════════════════════════════════════════════════════════════════
+//  TEST 20 — Siegebound.ControlsHelp.ShownNumbersAreReadFromTheirOwners   ⭐⭐
+// ════════════════════════════════════════════════════════════════════════════════════════
+
+/**
+ *  ⭐⭐ TASK-1576 (2026-09-28). `HELP-§2`: "NO NUMBER IS RESTATED IN PROSE IF IT CAN BE READ FROM
+ *  DATA ... Prefer deriving." Two pages now SHOW a number, and this test asserts that each shown
+ *  number IS its owner's value, read here from the same object the composer reads:
+ *    • Interface.MapMarks, the map-circle cap: USiegeMapMarkSubsystem's MaxMapMarks on the class
+ *      default object (the game's store is built from that class, and the property has no config,
+ *      no asset, no writer and no Blueprint child; the composer's block comment says so in full);
+ *    • Cards.StackUpgrade, the health factor: ABuilding::StackHealthMultiplier at ONE upgrade, the
+ *      factor ApplyStackUpgrade multiplies MaxHP by, read off ABuilding's class default for every
+ *      building.
+ *
+ *  ⛔ `SC-§37`: every claim is made against the OWNER'S VALUE, formatted here, ⛔ never against a
+ *  typed digit. A `Contains(TEXT("9"))` would pass on a page that typed the 9, which is the exact
+ *  defect the derivation removes. (A typed digit that happens to equal the owner today passes this
+ *  test by design; test 15 (e) catches it, on the typed template.)
+ *
+ *  (a)  per number, the row's own composed detail carries the owner's value INSIDE its own clause
+ *       ("hold up to <cap> circle", "maximum health by <factor>, compounding"), not merely as a
+ *       digit somewhere on the page;
+ *  (a2) per number, EVERY rendering of that row carries it: its own page body and each related
+ *       block that renders it on another page, found by walking every row's composed page (never
+ *       a typed list of pages);
+ *  (b)  per number, a NEGATIVE CONTROL: the same clause built from a DIFFERENT value is absent, so
+ *       (a) tells a wrong number from the right one rather than matching any number;
+ *  (d)  no `{#…}` number token survives on any composed page, with a vacuity guard that at least
+ *       one template carries one (without it (d) would also hold on a registry that typed every
+ *       number).
+ *
+ *  ⚠️ WHAT IT CANNOT PROVE (`SC-§32`): nothing here paints a page. That the numbers read well and
+ *  that the pages still fit the panel is 5b's to measure and Jonathan's to judge. ⛔ And the rows'
+ *  other candidates (the discard fee, the Rally values, the melee numbers and each building's own
+ *  height limit) are ⛔ NOT shown yet, so nothing here asserts them; handoffs/TASK-1576-programmer.md
+ *  lists why (each needs a new accessor in its owner's file).
+ */
+IMPLEMENT_SIMPLE_AUTOMATION_TEST(
+	FSiegeControlsHelpDerivedNumbersTest,
+	"Siegebound.ControlsHelp.ShownNumbersAreReadFromTheirOwners",
+	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
+
+bool FSiegeControlsHelpDerivedNumbersTest::RunTest(const FString& Parameters)
+{
+	// ── THE OWNERS, READ FROM THE SAME OBJECTS THE COMPOSER READS ────────────────────────
+	const USiegeMapMarkSubsystem* const MarkStoreDefaults = GetDefault<USiegeMapMarkSubsystem>();
+	if (!TestNotNull(TEXT("The map-mark store's class default object resolves, so the cap can be read"), MarkStoreDefaults))
+	{
+		return false;
+	}
+
+	const int32 OwnerMapMarkCap        = MarkStoreDefaults->MaxMapMarks;
+	const float OwnerStackHealthFactor = ABuilding::StackHealthMultiplier(1);
+
+	// ── FORMATTED HERE, TO THE FORMAT TASK-1576 FIXES ────────────────────────────────────
+	// The cap is a count of circles: a whole number, no fractional digits. The health factor is a
+	// plain multiplier: at most two fractional digits. ⛔ Written out here rather than asked of the
+	// composer, so a composer that formatted the WRONG value cannot also supply the expectation.
+	FNumberFormattingOptions CountOptions;
+	CountOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(0);
+	FNumberFormattingOptions FactorOptions;
+	FactorOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(2);
+
+	auto CapClause = [&CountOptions](int32 Cap) -> FString
+	{
+		// "circle" is a prefix of both noun forms, so the clause also holds for a cap of one.
+		return FString(TEXT("hold up to ")) + FText::AsNumber(Cap, &CountOptions).ToString() + FString(TEXT(" circle"));
+	};
+	auto FactorClause = [&FactorOptions](float Factor) -> FString
+	{
+		return FString(TEXT("maximum health by ")) + FText::AsNumber(Factor, &FactorOptions).ToString() + FString(TEXT(", compounding"));
+	};
+
+	struct FShownNumber
+	{
+		const TCHAR* ActionId;
+		const TCHAR* What;
+		FString      Clause;        // (a): the owner's value, inside its own clause
+		FString      WrongClause;   // (b): the same clause from a DIFFERENT value
+	};
+
+	const FShownNumber ShownNumbers[] =
+	{
+		{ TEXT("Interface.MapMarks"), TEXT("the map-circle cap (the map-mark store's MaxMapMarks, class default)"),
+			CapClause(OwnerMapMarkCap), CapClause(OwnerMapMarkCap + 1) },
+		{ TEXT("Cards.StackUpgrade"), TEXT("the stack health factor (StackHealthMultiplier at one upgrade)"),
+			FactorClause(OwnerStackHealthFactor), FactorClause(OwnerStackHealthFactor + 1.f) }
+	};
+
+	// The pure composer's own fallback lane, as in test 13: no layout subsystem, no applied keys.
+	auto NoAppliedKeys = [](const FSiegeControlsHelpAction&) -> TArray<FKey> { return TArray<FKey>(); };
+
+	for (const FShownNumber& Shown : ShownNumbers)
+	{
+		// FIXTURE SELF-CHECK: the negative control really is a different clause. (A claim about the
+		// FIXTURE, ⛔ not about a page.)
+		TestNotEqual(*FString::Printf(TEXT("FIXTURE SELF-CHECK: the negative control for %s is a different clause, so (b) means something"), Shown.What),
+			Shown.Clause, Shown.WrongClause);
+
+		const FSiegeControlsHelpAction* const Row = FSiegeControlsHelpRegistry::FindAction(FName(Shown.ActionId));
+		if (!TestNotNull(*FString::Printf(TEXT("Row '%s' is in the registry"), Shown.ActionId), Row))
+		{
+			continue;
+		}
+
+		// ── (a) THE ROW'S OWN COMPOSED DETAIL SHOWS THE OWNER'S VALUE, IN ITS CLAUSE ─────
+		const FString OwnPage = FSiegeControlsHelpRegistry::ComposeDetailForDisplay(*Row).ToString();
+		TestTrue(*FString::Printf(TEXT("Row '%s' shows %s as its owner holds it: '%s'"), Shown.ActionId, Shown.What, *Shown.Clause),
+			OwnPage.Contains(Shown.Clause, ESearchCase::CaseSensitive));
+
+		// ── (b) NEGATIVE CONTROL: A DIFFERENT VALUE IS NOT WHAT THE PAGE SHOWS ───────────
+		TestFalse(*FString::Printf(TEXT("NEGATIVE CONTROL: row '%s' does not show a different value ('%s'), so (a) tells numbers apart"), Shown.ActionId, *Shown.WrongClause),
+			OwnPage.Contains(Shown.WrongClause, ESearchCase::CaseSensitive));
+
+		// ── (a2) EVERY RENDERING OF THE ROW SHOWS IT ─────────────────────────────────────
+		// ⛔ Found by walking every row's composed page, never from a typed list: the row's own page
+		// body, plus every related block on any page that renders it.
+		int32 Renderings = 0;
+		int32 RenderingsShowingIt = 0;
+		for (const FSiegeControlsHelpAction& PageRow : FSiegeControlsHelpRegistry::GetActions())
+		{
+			const FSiegeControlsDetailContent Page =
+				FSiegeControlsHelpRegistry::ComposeDetailContent(PageRow, nullptr, NoAppliedKeys);
+
+			if (PageRow.ActionId == Row->ActionId)
+			{
+				++Renderings;
+				RenderingsShowingIt += Page.Body.ToString().Contains(Shown.Clause, ESearchCase::CaseSensitive) ? 1 : 0;
+			}
+
+			for (const FSiegeControlsDetailEntry& Entry : Page.Related)
+			{
+				if (Entry.ActionId == Row->ActionId)
+				{
+					++Renderings;
+					RenderingsShowingIt += Entry.Body.ToString().Contains(Shown.Clause, ESearchCase::CaseSensitive) ? 1 : 0;
+				}
+			}
+		}
+
+		// ⛔ THE VACUITY GUARD: a walk that found no rendering at all would make the next line 0 == 0.
+		TestTrue(*FString::Printf(TEXT("Row '%s' is rendered somewhere, so (a2) is a measurement"), Shown.ActionId),
+			Renderings > 0);
+		TestEqual(*FString::Printf(TEXT("Every rendering of row '%s' shows %s (its own page and each related block that renders it)"), Shown.ActionId, Shown.What),
+			RenderingsShowingIt, Renderings);
+
+		AddInfo(FString::Printf(TEXT("Row '%s': '%s' shown on %d of %d rendering(s)."),
+			Shown.ActionId, *Shown.Clause, RenderingsShowingIt, Renderings));
+	}
+
+	// ── (d) NO NUMBER TOKEN SURVIVES ON ANY COMPOSED PAGE ──────────────────────────────────
+	// ⚠️ A token naming nothing is LEFT VISIBLE by the composer (`HELP-§2` mechanism 2), so this
+	// is the check that stops a misspelled `{#…}` reaching the screen.
+	int32 TemplatesWithNumberTokens = 0;
+	for (const FSiegeControlsHelpAction& PageRow : FSiegeControlsHelpRegistry::GetActions())
+	{
+		if (PageRow.Detail.ToString().Contains(TEXT("{#"), ESearchCase::CaseSensitive))
+		{
+			++TemplatesWithNumberTokens;
+		}
+
+		TestFalse(*FString::Printf(TEXT("Row '%s' leaves no unresolved number token in its composed detail"), *PageRow.ActionId.ToString()),
+			FSiegeControlsHelpRegistry::ComposeDetailForDisplay(PageRow).ToString().Contains(TEXT("{#"), ESearchCase::CaseSensitive));
+	}
+
+	TestTrue(TEXT("At least one shipped template carries a number token, so (d) is a measurement and the mechanism is in use"),
+		TemplatesWithNumberTokens > 0);
+
+	return true;
+}
+
```
