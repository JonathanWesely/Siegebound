# TASK-1592 — [HELP-NUMBER-ACCESSORS] — programmer handoff

Marker `TASK-1592-HELP-NUMBER-ACCESSORS`. 2026-09-29. gameplay-programmer. Status → `ready-for-qa`. Gate: `TASK-1593`. Then `TASK-1594` builds on the reviewed bytes → `TASK-1595` → 5a `TASK-1596` → 5b `TASK-1597` → host `TASK-1598`.

Owner files only, source edits only. ⛔ No compile, no PIE, no editor call of any kind (not even read-only: every fact below came from source, config and asset BYTES on disk), no asset load, no mutating git. Declared tooling (`SC-§71a`): `git --no-optional-locks hash-object` / `rev-parse b9db99c:<path>` / `diff -U0`, `sha256sum`, `grep -a` byte scans of `Content/` packages, and one Python census over `Source/` (`t1592/arms.py` in the session scratchpad: anchor counts and byte deltas, read-only).

## §0 Start state (spec (0)): measured, all equal

Each file's working-tree blob (`hash-object`) equals its blob at `TASK-1580`'s commit `b9db99c` (`rev-parse b9db99c:./<path>`), and the index agreed. Scratch copies were taken in `t1592/start/` and re-hashed equal.

| file | blob at `b9db99c` = working tree | sha256 before |
|---|---|---|
| `SiegePlayerController.h` | `5fc4f74e…` | `840416b6f03bb436d90772caa5821c1455495917b8b8ed277c9119e543536d10` |
| `SiegePlayerController.cpp` | `cc420f58…` | `d2dfbf5021b1594162d8e0a50d2b6cced2e7327c7833b912dd324d121995b1fd` (= the row's `d2dfbf50…b1fd`) |
| `HeroCharacter.h` | `8918ba55…` | `a0859bf1dda88c56cdaa08984be09d0ac4e8131706b034a5685e6d3d1315136a` |
| `SiegeGameMode.h` | `6d7db266…` | `ea2c22b5f634bc91f779dc7f39d284485fc1142d675d4ca8f5b22a45f6726818` |
| `WarMapWidget.h` | `7c5c075a…` | `75918d5f8a5d19d1687b15104b6a94e9a8a9b66ee61bf9b36d115beed31ddaee` |
| `WarMapWidget.cpp` | `7c6694da…` | `28b19e45dfdfc1a812976460a90da6ca5908918dc4b302134ca005c8c46fb97f` |

Line endings preserved: the three `.h` files that were CRLF (`SiegePlayerController.h`, `HeroCharacter.h`, `SiegeGameMode.h`) are still CR count = line count; the other three were LF-only and still are; no BOM anywhere, before or after. The new test file is LF, no BOM.

## §1 Declarations moved or added (spec (1)–(3)), old beside new

All paths `Source/GitClaudeUnrealTest/Siegebound/`. Line numbers are hints at these bytes; cite by text.

| member | was | now | returns / default |
|---|---|---|---|
| `UClass* ResolveCardActorClass(FName CardID, ECardType CardType) const;` | `SiegePlayerController.h:2948`, inside `private:` (opens 2359) | `SiegePlayerController.h:1677`, inside the first `public:` (208–1699) | unchanged: the composed-path class, or nullptr. Doc kept with it, plus one dated line. Body (`.cpp`) not touched |
| `bool IsBuildingCard(FName CardID, ECardType CardType) const;` | `SiegePlayerController.h:2957`, `private:` | `SiegePlayerController.h:1687`, first `public:` | unchanged. Doc kept, plus one dated line. Body not touched |
| `const TSoftObjectPtr<UDataTable>& GetCardTableAsset() const { return CardTableAsset; }` | (new) | `SiegePlayerController.h:1694`, first `public:`; plain C++ | `CardTableAsset` (protected `UPROPERTY(EditDefaultsOnly)`); default `/Game/Data/DT_Cards.DT_Cards`, set in the constructor (`SiegePlayerController.cpp`, `CardTableAsset = TSoftObjectPtr<UDataTable>(…DT_Cards…)`) |
| `UFUNCTION(BlueprintPure, Category = "Siegebound|Cards") int32 GetDiscardAllCost() const { return DiscardAllCost; }` | (new) | `SiegePlayerController.h:1698`, first `public:` | `DiscardAllCost`, `int32`, default **20** |
| `const TSoftClassPtr<AHeroCharacter>& GetHeroPawnClassAsset() const { return HeroPawnClassAsset; }` | (new) | `SiegeGameMode.h:288`, first `public:` (144–289); plain C++ | `HeroPawnClassAsset`; default `/Game/Blueprints/BP_HeroCharacter.BP_HeroCharacter_C` (constructor, `SiegeGameMode.cpp`). `ResolveHeroPawnClass` stays private and unchanged |

At the old `private:` site one 3-line `//~` pointer says where the two declarations went. `ResolveCardRow` stays private. No caller changed (the three call sites, `EnterPlacementMode` ×2 and `TryConfirmPlacement`, are untouched). ⛔ `FSiegeCardPathStatics` not built; `TASK-959` / `TASK-970` untouched.

**Per-instance state (spec (1)'s question): neither member reads per-instance state that can differ from the class default at runtime.**
- `IsBuildingCard` reads its two arguments and `BuildingEconomyCardIDs`: `UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Cards")`, no Blueprint specifier (no Blueprint graph can write it), no `Config` specifier. Its only reader is `IsBuildingCard`, and nothing in `Source/` writes it (grep: declaration + that one read). The game's controller is the native class: `ASiegeGameMode`'s constructor sets `PlayerControllerClass = ASiegePlayerController::StaticClass()`, and the arena runs that native mode (§2), so no Blueprint CDO can re-author the list.
- `ResolveCardActorClass` reads its two arguments, `IsBuildingCard`'s answer, and `this` only for the name in its warning log; the class comes from a composed asset path, the same for any instance.
- For `TASK-1594`: the hero's seven properties are `EditAnywhere` (instance-editable), so a PLACED hero could differ from its class. The game spawns the hero instead: `L_Arena.umap`'s bytes carry no `HeroCharacter` name at all, and no C++ writes any of the seven (grep: reads only, in `HeroCharacter.cpp`).

## §2 The game-mode object the arena runs (spec (2)): **the native `ASiegeGameMode`, via `GlobalDefaultGameMode`**

How I know, read without loading an asset and without an editor call:
1. `Config/DefaultEngine.ini`, `[/Script/EngineSettings.GameMapsSettings]`: `GlobalDefaultGameMode=/Script/GitClaudeUnrealTest.SiegeGameMode`. No other `Config/` line names `SiegeGameMode`, `SiegePlayerController` or `PlayerControllerClass`.
2. `Content/Maps/L_Arena.umap` (a real 605,098-byte package; the file opens with magic `c1832a9e`, not an LFS pointer) contains **no** name matching `*GameMode*` at all (byte scan). A World Settings game-mode override is serialized as the `DefaultGameMode` property plus the class path, and the file does carry `WorldSettings` names (an in-file positive control).
3. Positive control of the method on a map that DOES override: `L_MainMenu.umap` carries `DefaultGameMode`, `/Game/Blueprints/BP_MenuGameMode` and `BP_MenuGameMode_C`.
4. No Blueprint subclass of `ASiegeGameMode` exists: the only four packages in `Content/` whose bytes contain `SiegeGameMode` are `WBP_DeckBuilder`, `WBP_DeckCardTile`, `WBP_MainMenu` (widget blueprints, `/Script/UMG.WidgetBlueprintGeneratedClass`) and `Dev/BP_T19_PlayAgain` (parent `/Script/Engine.Actor`).
5. `ASiegeGameMode::StartMatch` opens the arena with `OpenLevelBySoftObjectPtr` and no `?game=` option; `StartSandboxMatch` adds only `Sandbox=1`.

⇒ `GetDefault<ASiegeGameMode>()` is the object whose `HeroPawnClassAsset` the game spawns from, and test 3 reads it there.

⚠️ For `TASK-1594`: `GetHeroPawnClassAsset()` is the authored soft pointer, not the resolution. If the Blueprint were missing, the game would fall back to the raw `AHeroCharacter` inside the private `ResolveHeroPawnClass`, and a reader of the soft pointer would get null instead. Today it resolves (test 3 pins it).

## §3 The 8 getters (spec (3))

`GetRecallChannelSeconds()` pattern: `UFUNCTION(BlueprintPure, Category = <the property's own Category>)`, a one-line doc saying the Controls help derives the number, body `return <Property>;`. Every property's type is the one the row wrote; nothing was retyped.

| getter (file:line) | returns | property type · Category | C++ default |
|---|---|---|---|
| `ASiegePlayerController::GetDiscardAllCost` (`SiegePlayerController.h:1698`) | `DiscardAllCost` | `int32` · `Siegebound|Cards` | 20 |
| `AHeroCharacter::GetMeleeRange` (`HeroCharacter.h:593`) | `MeleeRange` | `float` · `Siegebound|Combat` | 150 |
| `AHeroCharacter::GetMeleeHalfAngleDegrees` (`:597`) | `MeleeHalfAngleDegrees` | `float` · `Siegebound|Combat` | 30 |
| `AHeroCharacter::GetMeleeCooldown` (`:601`) | `MeleeCooldown` | `float` · `Siegebound|Combat` | 0.5 |
| `AHeroCharacter::GetRallyRadius` (`:605`) | `RallyRadius` | `float` · `Siegebound|Combat` | 600 |
| `AHeroCharacter::GetRallySpeedBonus` (`:609`) | `RallySpeedBonus` | `float` · `Siegebound|Combat` | 0.25 |
| `AHeroCharacter::GetRallyDuration` (`:613`) | `RallyDuration` | `float` · `Siegebound|Combat` | 5 |
| `AHeroCharacter::GetRallyCooldown` (`:617`) | `RallyCooldown` | `float` · `Siegebound|Combat` | 20 |

The hero block sits in the first `public:` (402–839), after `IsDead()`, under a `//~` banner. The properties stay `protected` and unchanged.

**Name collisions checked, so the new names break nothing:** none of the 10 new or public names (`GetMeleeRange` … `GetRallyCooldown`, `GetDiscardAllCost`, `GetCardTableAsset`, `GetHeroPawnClassAsset`) occurs anywhere in `Source/`, in the engine's `GameFramework/` and `Engine/` class headers, or in the bytes of any `.uasset` / `.umap` under `Content/` (0 hits each). That last scan is what rules out a Blueprint function or variable of the same name in `BP_HeroCharacter` (which would stop that Blueprint compiling). `BP_HeroCharacter.uasset` is a real package whose name table is readable (it carries `HeroCharacter` names), so the zero means something.

## §4 Tests (spec (4)) and `N`

New file `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHelpAccessorsTest.cpp`: `#if WITH_DEV_AUTOMATION_TESTS`, 3 × `IMPLEMENT_SIMPLE_AUTOMATION_TEST`, `EditorContext | EngineFilter`, fixture namespace `SiegeHelpAccessorsTestFixture`. All new symbols are unique in `Source/` (grep).

1. **`Siegebound.HelpAccessors.EachGetterReturnsItsOwnProperty`** (`FSiegeHelpAccessorsGetterPinTest`), spec (4)(a). Each of the 7 hero getters is called on `GetDefault<AHeroCharacter>()` and compared with `FindFProperty<FFloatProperty>(AHeroCharacter::StaticClass(), <property name>)` read on the same object, tolerance 0. `GetDiscardAllCost()` is compared with `FindFProperty<FIntProperty>(…, "DiscardAllCost")` on `GetDefault<ASiegePlayerController>()`. A missing or retyped property is an `AddError`, not a skip. Sibling guard: a self-check that all 8 were pinned, then `TestTrue` that **at least two of the 8 values differ**; then every equal pair inside `AHeroCharacter` is named by `AddInfo` (0 at today's values: 150 / 30 / 0.5 / 600 / 0.25 / 5 / 20 are pairwise distinct, so any sibling swap inside the hero is visible). The one equal pair across classes (`RallyCooldown` 20 = `DiscardAllCost` 20) cannot be swapped, because a getter can only return its own class's field.
2. **`Siegebound.HelpAccessors.BuildingCardsResolveThroughTheControllersOwnResolution`** (`FSiegeHelpAccessorsBuildingResolutionTest`), spec (4)(b), public surface only, on `GetDefault<ASiegePlayerController>()`: `GetCardTableAsset().LoadSynchronous()` (self-checks: not null, loads, row struct is `FCardRow`), then for every row name, `FindRow<FCardRow>`. Where `IsBuildingCard(CardID, Row->CardType)` is true, it asserts `ResolveCardActorClass(…)` `IsChildOf(ABuilding)`. Then the **non-empty guard**: `BuildingCards > 0`. Expected at today's data (`handoffs/TASK-1576-programmer.md` §2): 8 building cards (7 `Building` plus `DeepMine` through `BuildingEconomyCardIDs`); Miner stays a unit.
3. **`Siegebound.HelpAccessors.HeroClassResolvesThroughTheGameModesOwnAsset`** (`FSiegeHelpAccessorsHeroClassTest`), spec (4)(b): `GetDefault<ASiegeGameMode>()->GetHeroPawnClassAsset().LoadSynchronous()` `IsChildOf(AHeroCharacter)`.

**`BS_Error` check (tests 2 and 3, under `#if WITH_EDITOR`).** The engine call, read at engine source: `UBlueprint::GetBlueprintFromClass(const UClass*)` (`Engine/Classes/Engine/Blueprint.h`, declared inside `#if WITH_EDITORONLY_DATA`; body `Cast<UBlueprint>(InClass->ClassGeneratedBy)` in `Engine/Private/Blueprint.cpp`), then the transient `UBlueprint::Status` (`TEnumAsByte<EBlueprintStatus>`), compared with `BS_Error`. A native class has no generating Blueprint and passes. `Engine/Blueprint.h` is included only under `WITH_EDITOR`. An editor target always builds with editor-only data, so the `WITH_EDITORONLY_DATA` declaration is present wherever `WITH_EDITOR` is 1.

**`AddInfo` lines for `TASK-1596`'s status reading (`VER-§12` cl. 7g)**, all prefixed `[HelpAccessors]`: each getter's value beside its property's; the table path; one line per building card, `card '<ID>' (<CardType>) -> class '<path>', status BS_… (Blueprint '<path>')`; a summary of rows read, building cards and classes resolved; and `hero class '<soft path>' -> '<class path>', status …`.

**`N` = 568 + 3 = 571.** Measured: `IMPLEMENT_SIMPLE_AUTOMATION_TEST(` declarations in `Source/` are 571 at these bytes (568 at `b9db99c`, matching `TASK-1580`'s 568/568). No existing test changed.

## §5 The mutation arms `TASK-1596` owes (`SHIP-§9`)

Run one at a time as byte replaces (⛔ no `Get-Content` / `Set-Content`, ⛔ no `-replace`). Before each, assert the anchor occurs **exactly once in `Source/`** (at these bytes: 1 each, `arms.py`). Restore by byte copy and re-hash equal. Every arm is C++ and needs a recompile of the mutant. `\n` / `\t` below are literal LF / TAB bytes; `SiegePlayerController.cpp` is LF-only.

| arm | file | anchor (1 in `Source/`) | replace once with | bytes | expect |
|---|---|---|---|---|---|
| **arm-G** (getter returns a sibling) — REQUIRED | `HeroCharacter.h` | `float GetRallyCooldown() const { return RallyCooldown; }` | `float GetRallyCooldown() const { return RallyDuration; }` | **0** (same length; the sha moves) | exactly one red test, **test 1**, **1 error** |
| **arm-R** (`IsBuildingCard` false for every card) — REQUIRED | `SiegePlayerController.cpp` | `\tif (CardType == ECardType::Building)\n\t{\n\t\treturn true;\n\t}\n\treturn CardType == ECardType::Economy && BuildingEconomyCardIDs.Contains(CardID);` | `\tif (CardType == ECardType::Building)\n\t{\n\t\treturn CardType != ECardType::Building;\n\t}\n\treturn CardType == ECardType::Economy && BuildingEconomyCardIDs.Contains(CardID) && CardType == ECardType::Building;` | **+62** | exactly one red test, **test 2**, **1 error** (the non-empty guard) |
| arm-H (hero soft class emptied) — optional, not required by the row | `SiegeGameMode.h` | `const TSoftClassPtr<AHeroCharacter>& GetHeroPawnClassAsset() const { return HeroPawnClassAsset; }` | `const TSoftClassPtr<AHeroCharacter>& GetHeroPawnClassAsset() const { static const TSoftClassPtr<AHeroCharacter> ArmHNone; return ArmHNone; }` | **+43** | exactly one red test, **test 3**, **1 error** |

arm-R's replacement keeps both parameters and the list in use and returns false on every path, with no constant condition. That is deliberate: there is no unreachable code for C4702 to flag (the engine only disables C4702 locally, through the `PRAGMA_DISABLE_UNREACHABLE_CODE_WARNINGS` macro in `MSVCPlatformCompilerPreSetup.h`, not globally), and there is no unused parameter.

**Expected error lines** (UE 5.8 formats, `AutomationTest.cpp`: float `TestEqual` "Expected '%s' to be %f, but it was %f and outside tolerance %f."; `TestTrue` "Expected '%s' to be true."):
- **arm-G:** `Expected 'GetRallyCooldown() returns RallyCooldown (AHeroCharacter class default; the property read by reflection by its name)' to be 20.000000, but it was 5.000000 and outside tolerance 0.000000.` The sibling guard stays green: the values are still 8 with 7 distinct, and the arm changes no property.
- **arm-R:** `Expected 'At least one card in the table is a building card by IsBuildingCard (the set the Controls help lists is non-empty)' to be true.` The row self-check stays green (rows are still read), and the summary `AddInfo` reads `0 building card(s)`. No other test can see it: `IsBuildingCard` was private until this row, so no test calls it, and its only callers are the instance play paths, which no headless test drives.
- **arm-H:** `Expected 'GetHeroPawnClassAsset() ('') resolves to a class that IsChildOf(AHeroCharacter)' to be true.` (a null soft pointer's `ToString()` is empty).

**Reading rule:** exactly one red test with exactly one error per arm. Any other red test means the arm leaked ⇒ STOP.

## §6 `qa/TASK-1586.md` W1 (spec (5)): comments only, old beside new

Each correction is dated `TASK-1592 (2026-09-29)`, cites `MARK-§4` by its current text (markers dropped), and quotes the old words. None of them contains a brace, and every added line is a comment line by the suite's own definition (it starts with `//` or `* `), so the code-line census tests (`SiegePlacementTest`, `SiegeBuildingStackTest`, …) count the same as before. Checked: every source-scanning test that reads these files counts code lines only, or counts tokens these comments do not contain.

| site | old | new |
|---|---|---|
| (i) `SiegePlayerController.cpp`, `ApplyPlacementFootprintWheel`'s leading comment | "…fire on every notch of an idle scroll. The WatchTower's refusal already has a voice — the RED ghost plus "That building cannot be stacked" on the click (TASK-813) — and this is the same exclusion speaking once." | The claim is **dropped** from the live text, which now ends "…every notch of an idle scroll." A dated note quotes the dropped sentence whole and says why it is false: since `STACK-§8` / `STACK-§10` the height gate is `CanStackHeight`, which `AClimbableTower` answers true (its limit is a per-class ceiling instead), so no shipped class produces `NotStackable` and no click says that line (quoting the state's doc: "NO SHIPPED CLASS PRODUCES THIS STATE"). The wheel exclusion is `CanScaleFootprint`, a different rule, and the note records that this sentence is what `Cards.PlacementResize`'s false clause was copied from. (Verified at source: `CanStackHeight` returns true in `ABuilding` and in `AClimbableTower`'s override, the only override. It is plain C++, so no Blueprint can override it, and both `StackNotStackableRefusalText()` call sites are gated on it being false.) |
| (iii-a) `SiegePlayerController.cpp`, `ApplyGroupPickWheel`'s comment | "the wheel is verified globally unbound and must stay INERT outside the pick; this is only ever called from the pick branch" | "the wheel is verified globally unbound, and THIS POLL is INERT outside the pick; it is only ever called from the pick branch", plus a dated note: true of this poll, not of the wheel, with `MARK-§4`'s three consumers quoted and the old parenthesis quoted |
| (iii-b) `SiegePlayerController.h`, `ApplyGroupPickWheel`'s doc | "the wheel is globally unbound and must stay INERT outside the pick" | "the wheel is globally unbound, and THIS POLL is INERT outside the pick", plus the same dated note |
| (ii) `WarMapWidget.cpp`, the block above `NativeOnMouseWheel` | "⛔ THE WHEEL NOW HAS EXACTLY TWO CONSUMERS AND ⛔ NO THIRD MAY BE ADDED WITHOUT AMENDING MARK-§4 BY NAME." | "⛔ MARK-§4 NAMES EVERY CONSUMER OF THE WHEEL, AND ⛔ A NEW ONE IS ADDED ONLY BY AMENDING IT BY NAME.", plus a dated note quoting the old sentence and `MARK-§4`'s current three-consumer sentence in full (it was amended by name, to three, by `STACK-§4`). The note also says consumers 1 and 3 are sibling controller polls, so the block's "inert outside the pick flow" is true of consumer 1's poll only |
| (iv) `WarMapWidget.h`, class comment section (e) | "✅ **THE TWO CONSUMERS ARE STRUCTURALLY DIFFERENT MECHANISMS IN DIFFERENT LANES, VERIFIED AT THE SOURCE…**" | "✅ **THE CONTROLLER'S POLL AND THIS WIDGET'S EVENT ARE STRUCTURALLY DIFFERENT MECHANISMS IN DIFFERENT LANES, VERIFIED AT THE SOURCE…**", plus a dated note: the old opening quoted, the count went stale when `STACK-§4` amended `MARK-§4` to three, `MARK-§4`'s sentence quoted, and the contrast holds for both controller polls against the widget's Slate event |

⚖️ **For QA: in (ii) and (iv) the note also says "inert outside the pick flow" is true of consumer 1's poll only.** That clause sits in the same paragraph as the named sentence, it states the same fact as (iii)'s correction, and leaving it unqualified next to a note that says "three consumers" would make the block contradict itself. It is a qualification added in the note. The old words are left in place, not rewritten. Rule on it if you read it as outside "No other comment moves".

## §7 The 5a shape (`TASK-1596`)

- **What the compiler and UHT see (`SC-§93`).** The two `.cpp` edits and the `ApplyGroupPickWheel` doc (a private, non-reflected function) are comment-only. The `.h` changes are not all comment-only: 8 new UFUNCTIONs, 4 non-reflected public members (the two moved declarations, and the plain C++ getters `GetCardTableAsset` and `GetHeroPawnClassAsset`), and a class-doc comment edit in `WarMapWidget.h` that UHT captures as class metadata.
- **`.generated.h` line macros** (anchors measured before → after):
  - `SiegePlayerController.h`: delegates 40 / 50 / 60 / 72, `UCLASS` 203, `GENERATED_BODY` 206 → **unmoved** (every edit is below 206).
  - `HeroCharacter.h`: 44 / 54 / 65 / 74 / 114 / 145 / 148 / 260 / 272 / 313 / 397 / 400 → **unmoved** (edits below 400; nothing reflected after it).
  - `SiegeGameMode.h`: `UCLASS` 139, `GENERATED_BODY` 142 → **unmoved**.
  - **`WarMapWidget.h`: `UCLASS` 775 → 785, `GENERATED_BODY` 778 → 788** (+10, section (e) sits in `UWarMapWidget`'s class comment, 431–784), so `_h_775_PROLOG` → **`_h_785_PROLOG`** and `_h_778_{CALLBACK_WRAPPERS, ENHANCED_CONSTRUCTORS, GENERATED_BODY, INCLASS_NO_PURE_DECLS, RPC_WRAPPERS_NO_PURE_DECLS}` → **`_h_788_…`**. Delegates 55 / 58 / 68 are unmoved.
- **`.gen.cpp` content:** `HeroCharacter.gen.cpp` gains 7 functions (thunk, `Z_Construct_UFunction_…`, metadata `Category` / `Comment` / `ToolTip` / `ModuleRelativePath`, function-table entries). `SiegePlayerController.gen.cpp` gains 1. `WarMapWidget.gen.cpp`: `UWarMapWidget`'s class `Comment` / `ToolTip` metadata changes, and no function changes. `SiegeGameMode.gen.cpp`: no UHT-visible change, so expected byte-identical.
- **Exec-symbol set (`DEFINE_FUNCTION(<Class>::exec…)`)**, baseline counted in the on-disk `Intermediate/…/UHT/*.gen.cpp`; each equals the header's `UFUNCTION(` declaration count at `b9db99c`:
  - `AHeroCharacter` **29 → 36**: `+ execGetMeleeRange, execGetMeleeHalfAngleDegrees, execGetMeleeCooldown, execGetRallyRadius, execGetRallySpeedBonus, execGetRallyDuration, execGetRallyCooldown` (the other 29 unchanged: `execAbortLadderClimb … execSetMeleeSuppressed`).
  - `ASiegePlayerController` **44 → 45**: `+ execGetDiscardAllCost`.
  - `ASiegeGameMode` **6 → 6**. `UWarMapWidget` **11 → 11**. Nothing else in the module changes.
- **Build impact:** four widely included headers change, so expect a wide recompile. New UFUNCTIONs mean Live Coding cannot apply the change: a full compile with the editor closed, then a relaunch (the graceful-quit lane), as usual. No header in the fence gained an `#include`. The new test includes `Engine/DataTable.h`, `Siegebound/{Building,CardRow,HeroCharacter,SiegeGameMode,SiegePlayerController}.h` and, under `WITH_EDITOR`, `Engine/Blueprint.h`.
- **Shadowing (`C4458`), checked against the engine base:** `FAutomationTestBase`'s data members (`bComplexTask`, `bRunOnSeparateThread`, `bSuppressLogs`, `TestName`, `TestParameterContext`, `ExecutionInfo`, `ExpectedMessages`, `ActionCS`, plus the static log-suppression flags) collide with no local in the test. The new getters are `const` members that take no parameters.
- **Residency (`VER-§12` cl. 7g):** nothing was loaded in the GUI editor (PID 15044): I made no editor call at all. Tests 2 and 3 load `DT_Cards`, the eight `BP_Building_*` classes and `BP_HeroCharacter` **in the suite's own process**. Their `[HelpAccessors] … status BS_…` lines are the status reading.

## §8 Found and reported, not fixed ("A further false comment is reported, not fixed")

1. `WarMapWidget.cpp`, inside `NativeOnMouseWheel`: "not a map zoom (there is none, and adding one would be the third consumer MARK-§4 forbids)". That would now be a **fourth** consumer.
2. `WarMapWidget.cpp`, inside `NativeOnMouseWheel`, and `WarMapWidget.h` section (e)'s second paragraph: "it adds ⛔ NO third consumer". This is still true that the absorb adds no consumer, but the count reads stale. Optional wording.
3. **Made false by this row, in `TASK-1594`'s files (forbidden to me):** `SiegeControlsHelpWidget.cpp`'s comments saying "DiscardAllCost and the hero's Melee* / Rally* tunables are protected with no [getter]" and "(ASiegePlayerController::ResolveCardActorClass / IsBuildingCard) is private" (the owed-numbers block near the top, and the `Cards.StackUpgrade` note "that resolution … is private to the controller"). `TASK-1594` should correct them when it lands the numbers.
4. `Tests/SiegeSpellVFXRosterTest.cpp` file header: "read by reflection (`CardTableAsset` is not public)". This is still literally true (the property stays protected), but a public `GetCardTableAsset()` now exists. Optional.
5. `HeroCharacter.h`, `GetRecallChannelSeconds()`'s doc: "the value the `HELP-§` row DERIVES rather than restating in prose". Nothing in `Source/` calls `GetRecallChannelSeconds` (grep: its declaration only), and `SiegeControlsHelpWidget.cpp` does not mention `RecallChannelSeconds`. Either the doc is false or the Recall page's number is typed or absent. Not examined further; outside this row.
6. The dated line citations inside the corrected comments (`SiegePlayerController.cpp:647-657`, `:2843-2876` in `WarMapWidget.{h,cpp}`) are stale annotations (`SC-§126` cl. 12). Left as they are.

## What QA (`TASK-1593`) should scrutinize

1. Only access sections moved and members were added: both moved declarations are byte-identical to their old lines (diff below); no `.cpp` body changed; no tunable's value changed.
2. Each getter returns exactly the property its name says (the table in §3). arm-G's mutant is 0 bytes long by design, so only the sha shows it.
3. The (4)(a) pin compares against reflection BY NAME, so it cannot take its expectation from the getter. The "two differ" guard is the literal ask; the within-class equal-pair `AddInfo` is my addition, and it is not an assertion because it is tunable-dependent.
4. The (4)(b) walk uses only the public surface (`GetCardTableAsset`, `IsBuildingCard`, `ResolveCardActorClass`, `GetHeroPawnClassAsset`), plus `FindRow`. The `BS_Error` call and its guard are in §4.
5. §6's comments are true at source, and ⚖️ the qualification flagged there.
6. The 5a shape in §7, especially the `WarMapWidget.h` line-macro shift and the exec delta +7 / +1 / 0.

## Not examined / limitations

- ⛔ **No compile, no suite run, no PIE, no editor call.** `N = 571`, "8 building cards", each arm's single error line and the exec-symbol set are predictions from the source, from engine source formats and from `arms.py`. `TASK-1596` measures them.
- The `.gen.cpp` baseline counts come from `Intermediate/` files with older mtimes (UHT rewrites only on change). They match the headers' `UFUNCTION(` declaration counts at `b9db99c` (29 / 44 / 6), but they are not a fresh UHT run.
- **The Blueprint status of the eight building classes and of `BP_HeroCharacter` is not known before the suite loads them.** The `BS_Error` assertions exist to find out. If one is `BS_Error`, test 2 or 3 goes red with the class named. That would be a finding, not an arm leak.
- `ResolveCardActorClass` logs a Warning for a missing class (not an Error), so a missing `BP_Building_*` shows as a failed `IsChildOf(ABuilding)` row, not as a log failure.
- The game-mode answer (§2) rests on byte scans of the packages' name tables. That is sound for a serialized override, but a map can also be launched with a URL `?game=` option; the shipped entry points pass none (§2 item 5).
- The float and int values in the expected error lines assume today's C++ defaults on the native class defaults (native CDOs, no Blueprint in the read). The hero's `BP_HeroCharacter` defaults are not read by test 1. `handoffs/TASK-1576-programmer.md` §2 recorded them equal on 2026-09-28.

## Files touched, sha256 before → after

| file | before | after | size / lines |
|---|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` | `840416b6f03bb436d90772caa5821c1455495917b8b8ed277c9119e543536d10` | `c0c558f34571d96e7ea375192d20cf14cf5e7b43d4c50585d48546806c3fb2b7` | 209276 → 211687 B · 3593 → 3626 |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` | `d2dfbf5021b1594162d8e0a50d2b6cced2e7327c7833b912dd324d121995b1fd` | `32e582f064ec00dada335e01e13deae00c55bc406fd192896cace3f10a95a7d9` | 388165 → 389435 B · 7775 → 7789 |
| `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h` | `a0859bf1dda88c56cdaa08984be09d0ac4e8131706b034a5685e6d3d1315136a` | `b48263843e7ecb4ff9544670e7a0d14093b816de04c92eaae1b14a76e42fd7ec` | 93604 → 95800 B · 1486 → 1519 |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h` | `ea2c22b5f634bc91f779dc7f39d284485fc1142d675d4ca8f5b22a45f6726818` | `af7d99966eb05b0179a3c728d2280436a4198d7b9acef991ff0aa77ca0aa13c2` | 47730 → 48345 B · 800 → 810 |
| `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h` | `75918d5f8a5d19d1687b15104b6a94e9a8a9b66ee61bf9b36d115beed31ddaee` | `86ca2acd3b19f2b22c3cd449467f3b3f90dbe0272753a5860de53241455d63d1` | 108237 → 109055 B · 1728 → 1738 |
| `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp` | `28b19e45dfdfc1a812976460a90da6ca5908918dc4b302134ca005c8c46fb97f` | `4abd324dec233729535422ed2c635596bc2fe5c99e6b0c764e132f3c95728206` | 145272 → 146170 B · 2953 → 2964 |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHelpAccessorsTest.cpp` | (new) | `ddfe333565ad047dd16ab0c84d2022286ad3abd76a8119527fb70a6d3df548ad` | 19745 B · 394 |

Also written: this handoff, and `TASKBOARD.md` (this row's `status:` line only). `git diff --stat b9db99c -- Source/`: 6 files, +139 / −28 (the new test file is untracked, so it is not in the diff: the whole file is new).

## `git --no-optional-locks diff -U0 b9db99c` (the six tracked files; CR stripped for display)

```diff
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h
index 8918ba5..88efdb1 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h
@@ -585,0 +586,33 @@ public:
+	//~ ─── THE CONTROLS HELP'S READS (TASK-1592, 2026-09-29) — one-line public getters, the
+	//~     GetRecallChannelSeconds() pattern. The Attack and Rally pages DERIVE these numbers
+	//~     from the hero class the game spawns rather than typing them. Each returns exactly
+	//~     the property its name says; the properties stay protected and unchanged. ───
+
+	/** Melee reach in uu (MeleeRange). The Controls help's Attack page derives its number from this (TASK-1592). */
+	UFUNCTION(BlueprintPure, Category = "Siegebound|Combat")
+	float GetMeleeRange() const { return MeleeRange; }
+
+	/** Half-angle of the forward melee cone in degrees (MeleeHalfAngleDegrees). The Controls help's Attack page derives its number from this (TASK-1592). */
+	UFUNCTION(BlueprintPure, Category = "Siegebound|Combat")
+	float GetMeleeHalfAngleDegrees() const { return MeleeHalfAngleDegrees; }
+
+	/** Minimum seconds between melee swings (MeleeCooldown). The Controls help's Attack page derives its number from this (TASK-1592). */
+	UFUNCTION(BlueprintPure, Category = "Siegebound|Combat")
+	float GetMeleeCooldown() const { return MeleeCooldown; }
+
+	/** Rally's buff radius in uu (RallyRadius). The Controls help's Rally page derives its number from this (TASK-1592). */
+	UFUNCTION(BlueprintPure, Category = "Siegebound|Combat")
+	float GetRallyRadius() const { return RallyRadius; }
+
+	/** Rally's fractional move-speed bonus per buffed unit (RallySpeedBonus). The Controls help's Rally page derives its number from this (TASK-1592). */
+	UFUNCTION(BlueprintPure, Category = "Siegebound|Combat")
+	float GetRallySpeedBonus() const { return RallySpeedBonus; }
+
+	/** Seconds each friendly unit keeps Rally's buff (RallyDuration). The Controls help's Rally page derives its number from this (TASK-1592). */
+	UFUNCTION(BlueprintPure, Category = "Siegebound|Combat")
+	float GetRallyDuration() const { return RallyDuration; }
+
+	/** Seconds before Rally can be used again (RallyCooldown). The Controls help's Rally page derives its number from this (TASK-1592). */
+	UFUNCTION(BlueprintPure, Category = "Siegebound|Combat")
+	float GetRallyCooldown() const { return RallyCooldown; }
+
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h
index 6d7db26..90ec1d7 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h
@@ -279,0 +280,10 @@ public:
+	/**
+	 *  The hero class this mode spawns, as AUTHORED (HeroPawnClassAsset, the soft class of
+	 *  /Game/Blueprints/BP_HeroCharacter). Plain C++, not a UFUNCTION (TASK-1592, 2026-09-29):
+	 *  the Controls help reads the Attack and Rally numbers off the class the game spawns.
+	 *  ⚠️ This is the soft pointer, ⛔ not the resolution: ResolveHeroPawnClass stays private and
+	 *  unchanged, and it alone caches the class and falls back to the raw AHeroCharacter when
+	 *  the Blueprint is missing.
+	 */
+	const TSoftClassPtr<AHeroCharacter>& GetHeroPawnClassAsset() const { return HeroPawnClassAsset; }
+
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
index cc420f5..958e111 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
@@ -3504,3 +3504,11 @@ void ASiegePlayerController::ApplyPlacementFootprintWheel()
-	// not a click, so a HUD line here would fire on every notch of an idle scroll. The
-	// WatchTower's refusal already has a voice — the RED ghost plus "That building cannot
-	// be stacked" on the click (TASK-813) — and this is the same exclusion speaking once.
+	// not a click, so a HUD line here would fire on every notch of an idle scroll.
+	// ⭐ TASK-1592 (2026-09-29), comment only: the sentence that ended this paragraph is
+	// DROPPED because it is false at source. It read: "The WatchTower's refusal already has a
+	// voice — the RED ghost plus "That building cannot be stacked" on the click (TASK-813) —
+	// and this is the same exclusion speaking once." Since STACK-§8 / STACK-§10 the height
+	// question has its own gate, CanStackHeight, which AClimbableTower answers true (its limit
+	// is a per-class height ceiling instead), so no shipped class produces the NotStackable
+	// state and no click says that line (the state's doc in SiegePlayerController.h: "NO
+	// SHIPPED CLASS PRODUCES THIS STATE"). The exclusion above is the wheel's own X/Y gate,
+	// CanScaleFootprint, a different rule from the stack. qa/TASK-1586.md W1: this sentence is
+	// the text Cards.PlacementResize's false clause was copied from.
@@ -4279 +4287 @@ void ASiegePlayerController::ApplyGroupPickWheel()
-	// is verified globally unbound and must stay INERT outside the pick; this is
+	// is verified globally unbound, and THIS POLL is INERT outside the pick; it is
@@ -4282,0 +4291,6 @@ void ASiegePlayerController::ApplyGroupPickWheel()
+	// ⭐ TASK-1592 (2026-09-29), comment only: "INERT outside the pick" is true of this poll,
+	// not of the wheel. The wheel has MARK-§4's three named consumers, in its words: "(1) the
+	// controller's group-pick poll · (2) UWarMapWidget while the map is open and the cursor is
+	// over it · (3) the controller's PLACEMENT-mode footprint poll (ApplyPlacementFootprintWheel,
+	// STACK-§4)". Until TASK-1592 the parenthesis read "the wheel is verified globally unbound
+	// and must stay INERT outside the pick", the wheel law's wording before MARK-§4 amended it.
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h
index 5fc4f74..4a814f8 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h
@@ -1656,0 +1657,43 @@ public:
+	//~ ─── THE CARD → CLASS RESOLUTION AND THE DISCARD FEE, PUBLIC FOR THE CONTROLS HELP
+	//~     (TASK-1592, 2026-09-29; qa/TASK-1577.md Ruling 1) ───
+	//~
+	//~ ⭐ The Controls help shows each building type's height limit and the discard fee as live
+	//~ numbers. It reads them through THESE members, so it resolves a card exactly the way the
+	//~ game does and cannot drift from it: there is no copy of the path rule outside this class.
+	//~ ResolveCardActorClass and IsBuildingCard moved here from `private:` with their signatures
+	//~ and bodies unchanged, and no caller changed; ResolveCardRow stays private.
+	//~ ⛔ Not FSiegeCardPathStatics: TASK-959 / TASK-970 keep their own scope.
+
+	/**
+	 *  Resolves the BP class to spawn for a card by its CardType (CONVENTIONS
+	 *  composed soft-class paths, TASK-030): Unit/Economy →
+	 *  /Game/Blueprints/Units/BP_Unit_<CardID> (must be an ASummonedUnit;
+	 *  TASK-010/034); Building → /Game/Blueprints/Buildings/
+	 *  BP_Building_<CardID> (must be an ABuilding; TASK-035). Missing or
+	 *  incompatible = nullptr — the caller refuses the play with NO gold spent
+	 *  (the M1 meshless-ASummonedUnit fallback is retired per the spec).
+	 *  ⭐ PUBLIC SINCE TASK-1592 (2026-09-29): the Controls help reads the game's own card → class resolution here.
+	 */
+	UClass* ResolveCardActorClass(FName CardID, ECardType CardType) const;
+
+	/**
+	 *  True when the card spawns an ABuilding: every Building card, plus Economy
+	 *  cards whose actor is a building (Deep Mine — CardType Economy, but ADeepMine
+	 *  under /Blueprints/Buildings/, TASK-057). The single source of truth for the
+	 *  confirm spawn branch, the §3.5 building-clearance rule, and the BP-class path
+	 *  (BuildingEconomyCardIDs drives the Economy exception — TASK-059).
+	 *  ⭐ PUBLIC SINCE TASK-1592 (2026-09-29): the Controls help reads the game's own building-card rule here.
+	 */
+	bool IsBuildingCard(FName CardID, ECardType CardType) const;
+
+	/**
+	 *  The card stat table every play resolves against (CardTableAsset, the soft pointer
+	 *  ResolveCardRow loads). Plain C++, not a UFUNCTION (TASK-1592, 2026-09-29): the Controls
+	 *  help walks the SAME table the game plays from, never a typed path.
+	 */
+	const TSoftObjectPtr<UDataTable>& GetCardTableAsset() const { return CardTableAsset; }
+
+	/** The discard-all fee (DiscardAllCost). The Controls help's Discard page derives its number from this rather than typing it (TASK-1592). */
+	UFUNCTION(BlueprintPure, Category = "Siegebound|Cards")
+	int32 GetDiscardAllCost() const { return DiscardAllCost; }
+
@@ -2544 +2587 @@ private:
-	 *  wheel is globally unbound and must stay INERT outside the pick). Runs
+	 *  wheel is globally unbound, and THIS POLL is INERT outside the pick). Runs
@@ -2547,0 +2591,6 @@ private:
+	 *  ⭐ TASK-1592 (2026-09-29), comment only: "INERT outside the pick" is true of this poll,
+	 *  not of the wheel. The wheel has MARK-§4's three named consumers, in its words: "(1) the
+	 *  controller's group-pick poll · (2) UWarMapWidget while the map is open and the cursor is
+	 *  over it · (3) the controller's PLACEMENT-mode footprint poll (ApplyPlacementFootprintWheel,
+	 *  STACK-§4)". Until TASK-1592 the parenthesis read "the wheel is globally unbound and must
+	 *  stay INERT outside the pick", the wheel law's wording before MARK-§4 amended it.
@@ -2939,19 +2988,3 @@ private:
-	/**
-	 *  Resolves the BP class to spawn for a card by its CardType (CONVENTIONS
-	 *  composed soft-class paths, TASK-030): Unit/Economy →
-	 *  /Game/Blueprints/Units/BP_Unit_<CardID> (must be an ASummonedUnit;
-	 *  TASK-010/034); Building → /Game/Blueprints/Buildings/
-	 *  BP_Building_<CardID> (must be an ABuilding; TASK-035). Missing or
-	 *  incompatible = nullptr — the caller refuses the play with NO gold spent
-	 *  (the M1 meshless-ASummonedUnit fallback is retired per the spec).
-	 */
-	UClass* ResolveCardActorClass(FName CardID, ECardType CardType) const;
-
-	/**
-	 *  True when the card spawns an ABuilding: every Building card, plus Economy
-	 *  cards whose actor is a building (Deep Mine — CardType Economy, but ADeepMine
-	 *  under /Blueprints/Buildings/, TASK-057). The single source of truth for the
-	 *  confirm spawn branch, the §3.5 building-clearance rule, and the BP-class path
-	 *  (BuildingEconomyCardIDs drives the Economy exception — TASK-059).
-	 */
-	bool IsBuildingCard(FName CardID, ECardType CardType) const;
+	//~ ResolveCardActorClass and IsBuildingCard were declared here until TASK-1592 (2026-09-29).
+	//~ They moved to `public:` with their docs, and their signatures and bodies are unchanged: see
+	//~ the card → class block at the end of the first `public:` section.
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp
index 7c6694d..6ffb31c 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp
@@ -2786,2 +2786,13 @@ FReply UWarMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const
-// unchanged. ⛔ THE WHEEL NOW HAS EXACTLY TWO CONSUMERS AND ⛔ NO THIRD MAY BE
-// ADDED WITHOUT AMENDING MARK-§4 BY NAME.
+// unchanged. ⛔ MARK-§4 NAMES EVERY CONSUMER OF THE WHEEL, AND ⛔ A NEW ONE IS
+// ADDED ONLY BY AMENDING IT BY NAME.
+//
+// ⭐ TASK-1592 (2026-09-29), comment only: this block used to end "THE WHEEL NOW HAS
+// EXACTLY TWO CONSUMERS AND NO THIRD MAY BE ADDED WITHOUT AMENDING MARK-§4 BY NAME".
+// That clause did its job: STACK-§4 amended MARK-§4 by name, to three. MARK-§4 now reads
+// "the wheel has exactly THREE consumers — (1) the controller's group-pick poll · (2)
+// UWarMapWidget while the map is open and the cursor is over it · (3) the controller's
+// PLACEMENT-mode footprint poll (ApplyPlacementFootprintWheel, STACK-§4). It stays inert
+// everywhere else, and no FOURTH consumer may be added without amending this line again."
+// Consumers 1 and 3 are both controller polls, in sibling branches of PlayerTick that never
+// run in the same frame, so "inert outside the pick flow" above is true of consumer 1's poll
+// only. This widget's Slate event is still the one consumer outside the controller.
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h
index 7c5c075..fe4a2dc 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h
@@ -742,2 +742,2 @@ struct FSiegeWarMapMarkCircle
- *  ✅ **THE TWO CONSUMERS ARE STRUCTURALLY DIFFERENT MECHANISMS IN DIFFERENT LANES, VERIFIED
- *  AT THE SOURCE RATHER THAN ASSUMED:** the controller's is a **POLL** —
+ *  ✅ **THE CONTROLLER'S POLL AND THIS WIDGET'S EVENT ARE STRUCTURALLY DIFFERENT MECHANISMS IN
+ *  DIFFERENT LANES, VERIFIED AT THE SOURCE RATHER THAN ASSUMED:** the controller's is a **POLL** —
@@ -749,0 +750,10 @@ struct FSiegeWarMapMarkCircle
+ *  ⭐ TASK-1592 (2026-09-29), comment only: this paragraph used to open "THE TWO CONSUMERS ARE
+ *  STRUCTURALLY DIFFERENT MECHANISMS", a count that went stale when `STACK-§4` amended
+ *  `MARK-§4` by name, to three. `MARK-§4` now reads "the wheel has exactly THREE consumers —
+ *  (1) the controller's group-pick poll · (2) `UWarMapWidget` while the map is open and the
+ *  cursor is over it · (3) the controller's PLACEMENT-mode footprint poll
+ *  (`ApplyPlacementFootprintWheel`, `STACK-§4`)". Consumers 1 and 3 are both controller polls,
+ *  in sibling branches of `PlayerTick` that never run in the same frame, so the contrast drawn
+ *  here holds for both of them against this widget's Slate event, and "inert outside the pick
+ *  flow" is true of consumer 1's poll only.
+ *
```
