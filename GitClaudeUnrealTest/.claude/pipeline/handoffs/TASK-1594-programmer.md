# TASK-1594 — [HELP-DERIVED-NUMBERS-2] — programmer handoff

Marker `TASK-1594-HELP-DERIVED-NUMBERS-2`. 2026-09-29. gameplay-programmer. Status → `ready-for-qa`. Gate: `TASK-1595`. 5a: `TASK-1596`. 5b: `TASK-1597`. Host: `TASK-1598`.

Help files only: code, prose, comments and tests. ⛔ No compile, no PIE, no editor call of any kind (no GUI-editor load, not even a read-only inspector call; PID 15044 untouched), no asset load, no asset save, no mutating git. Declared tooling (`SC-§71a`): `sha256sum`, `git --no-optional-locks diff -U0 b9db99c` / `rev-parse` / `hash-object` (read-only), `grep -a` and one Python name-table scan over `Content/Blueprints/Buildings/*.uasset` bytes (no load), and four read-only Python text scripts in the session scratchpad `t1594/`: `sim.py` (parses the 27 rows, composes the changed pages with today's values, runs the new tests' text checks and the old scans, measures page lengths), `arms.py` (anchor counts and byte deltas over all 294 `.h`/`.cpp` under `Source/`), `digits.py` (literal-digit census, header `UFUNCTION(` count) and `names.py` (namespace-name collision scan against the newly included headers).

## §0 Start state (spec (0)): measured, all equal

| file | expected (`qa/TASK-1577.md` §5, re-measured by `qa/TASK-1593.md` §4) | measured before any edit | blob = `b9db99c` |
|---|---|---|---|
| `SiegeControlsHelpWidget.cpp` | `6a9ba044…9fa6b` | `6a9ba04491fea8b58b4538fe8e9496a674db0c23fa4478bdf48fa3679a29fa6b` ✓ (317440 B · 4977 lines) | `fb0121d5` ✓ |
| `SiegeControlsHelpWidget.h` | `7ed5a066…d1527` | `7ed5a066b6c5f608a40b23a95ce858db786bdad0e5565eb60cf2583f8a4d1527` ✓ (81822 B · 1412) | `0501e697` ✓ |
| `Tests/SiegeControlsHelpTest.cpp` | `588a4180…de3fa4` | `588a4180553851f7c52da9e4907aac07d522b2c67db1a9e6121c1c6790de3fa4` ✓ (172078 B · 2981) | `4acd21fc` ✓ |

The accessor files I read and never edited, before and after (equal to `qa/TASK-1593.md` §4): `SiegePlayerController.h` `c0c558f3…fb2b7` · `HeroCharacter.h` `b4826384…d7ec` · `SiegeGameMode.h` `af7d9996…a13c2` · `Tests/SiegeHelpAccessorsTest.cpp` `ddfe3335…48ad` (also `SiegePlayerController.cpp` `32e582f0…a7d9`, `WarMapWidget.{cpp,h}` `4abd324d…` / `86ca2acd…`, unchanged). Byte copies of the three start files are in `t1594/start/` (`hash-object` equal to the `b9db99c` blobs). All three help files are LF-only and BOM-less before and after.

## §1 The numbers now shown (Acceptance: the per-number table)

Every number goes through **`TASK-1576`'s one splice**: a `{#Name}` token in the prose, one entry in `HelpDerivedNumbers[]`, replaced in `ComposeDetailForDisplay` → `SpliceDerivedNumbers` (⛔ no second splice). Each entry reads its owner when the page is composed, formats with `FText::AsNumber` and fixed `FNumberFormattingOptions`, splices with `FText::Format` (named arguments; the noun chosen by a `plural` argument), and carries a `// Conversion:` comment beside its format call. Values "at source" are the C++ defaults; no Blueprint stores any of them (see "object read").

| page · token | property (owner) | public read (`TASK-1592`) | object read, and why | value at source | conversion / format | rendered |
|---|---|---|---|---|---|---|
| `Cards.StackUpgrade` · `{#StackUpgrade.HeightLimits}` | `ABuilding::MaxStackHeightMultiplier` (`int32`, `EditDefaultsOnly`, `ClampMin = "1"`) | `ABuilding::GetMaxStackHeightMultiplier()` (already public) on each class; the classes through `ASiegePlayerController::GetCardTableAsset()` + `IsBuildingCard` + `ResolveCardActorClass` | **each placeable building class's OWN class default** (`Cast<ABuilding>(Class->GetDefaultObject())`), the class the game spawns for that card: the enumeration is the controller's own resolution on its class default, the calls `EnterPlacementMode` (`IsBuildingCard`) and `TryConfirmPlacement` (`ResolveCardActorClass(PendingCardID, PendingCardType)`) make. ⛔ Never `GetDefault<ABuilding>()` for all. Per class because the ceiling is per class (`STACK-§10` cl. 2) and a Blueprint child may raise it | `ABuilding` 5 (inherited by `ATower` ×4, `ABarracks`, `ADeepMine`, the Wall's `ABuilding`); `AClimbableTower` 2 (its constructor). **No `BP_Building_*` package stores the name `MaxStackHeightMultiplier`** (byte scan below) ⇒ no Blueprint override expected; 5a's `AddInfo` lines are the reading | none: a whole multiple of the authored height; 0 / 0 fractional digits; `{Multiple} {PluralMultiple}\|plural(one=time,other=times)`; grouped by equal value in first-offered order; every type named | "…height, up to its height limit: **5 times that original height for the Arrow Tower, Wall, Bomb Tower, Ballista Tower, Barracks, Deep Mine and Crystal Tower, and 2 times for the Watch Tower**. The building's width and length are not touched." |
| `Cards.Discard` · `{#Discard.Fee}` | `ASiegePlayerController::DiscardAllCost` (`int32`) | `GetDiscardAllCost()` | **the controller's class default** (`GetDefault<ASiegePlayerController>()`): the game's controller is that native class (`PlayerControllerClass = ASiegePlayerController::StaticClass();` in `ASiegeGameMode`'s constructor; no Blueprint subclass, `qa/TASK-1593.md` ruling 2); `EditDefaultsOnly`, no `Config`, no package stores it (ruling 4), no writer in `Source/` | 20 | none; whole, 0 / 0; `{Gold} gold` | "The fee is **20 gold** and it is charged once for the whole hand, flat." |
| `Hero.Rally` · `{#Rally.Radius}` | `AHeroCharacter::RallyRadius` | `GetRallyRadius()` | **the class default of the hero class the game spawns**: `GetDefault<ASiegeGameMode>()->GetHeroPawnClassAsset().LoadSynchronous()` (the arena runs the native `ASiegeGameMode` through `GlobalDefaultGameMode`, `handoffs/TASK-1592-programmer.md` §2), which is `BP_HeroCharacter`; when it resolves nothing, the raw `AHeroCharacter` (below). The game spawns the hero and no C++ writes the seven, so the live hero holds these values | 600 uu | ÷ 100 → metres (uu are cm); 0 / 2 fractional digits; `metre`/`metres` | "within **6 metres** of your hero" |
| `Hero.Rally` · `{#Rally.SpeedBonus}` | `RallySpeedBonus` | `GetRallySpeedBonus()` | same | 0.25 | × 100 → percent (`SpeedMultiplier = 1.f + RallySpeedBonus`); 0 / 1; `{Percent}%` | "by **25%**" |
| `Hero.Rally` · `{#Rally.Duration}` | `RallyDuration` | `GetRallyDuration()` | same | 5 s | none (seconds); 0 / 2; `second`/`seconds` | "for **5 seconds**" |
| `Hero.Rally` · `{#Rally.Cooldown}` | `RallyCooldown` | `GetRallyCooldown()` | same | 20 s | none (seconds); 0 / 2; `second`/`seconds` | "After each rally there is a cooldown of **20 seconds**." |
| `Hero.Attack` · `{#Attack.Reach}` | `MeleeRange` | `GetMeleeRange()` | same | 150 uu | ÷ 100 → metres; 0 / 2; `metre`/`metres` | "within **1.5 metres** of you" |
| `Hero.Attack` · `{#Attack.ConeHalfAngle}` | `MeleeHalfAngleDegrees` | `GetMeleeHalfAngleDegrees()` | same | 30° | none (degrees, straight ahead to one edge); 0 / 1; `degree`/`degrees` | "inside a cone reaching **30 degrees** to either side of where you face" |
| `Hero.Attack` · `{#Attack.Cooldown}` | `MeleeCooldown` | `GetMeleeCooldown()` | same | 0.5 s | none (seconds); 0 / 2; `second`/`seconds` | "you can swing at most once every **0.5 seconds**" |

**The hero fallback, and how it matches `ResolveHeroPawnClass`.** `ResolveHelpHeroDefaults()` (anonymous namespace) does the game's two steps in the game's order: `GetHeroPawnClassAsset().LoadSynchronous()`, and when that is null, `AHeroCharacter::StaticClass()`. `ASiegeGameMode::ResolveHeroPawnClass` returns the loaded `HeroPawnClassAsset` (caching it), else warns once and returns `AHeroCharacter::StaticClass()`. The help does not cache (a page open is not a hot path) and does not log (the game mode already warns once on its own spawn path). `TSoftClassPtr<AHeroCharacter>::LoadSynchronous` returns null for a class that is not an `AHeroCharacter` (the game mode's own comment), and `Cast<AHeroCharacter>` is null-safe.

**Hero / fee override reading.** `qa/TASK-1593.md` ruling 4: no package stores any of the seven hero tunables or `DiscardAllCost` (positive control: `BP_HeroCharacter.uasset` carries the names it does override). Test 22 logs each of the seven on the spawned class's default beside the native `AHeroCharacter` default; those lines are 5a's reading.

**Where each is read, and when (qa/TASK-1593.md W1).** Only inside the token entries, i.e. only in `ComposeDetailForDisplay`, which the widget reaches through `ShowDetailForAction` → `ComposeDetailContent` once per page open (and per related block). ⛔ Never per frame, ⛔ never from the row list (`RefreshRows` composes no detail). `ResolveCardActorClass` is called **only** where `IsBuildingCard` is true, so its Error branch ("not a placement type") is unreachable from the help. A missing building class makes `ResolveCardActorClass` log its Warning and the page skips that type (the game cannot place it either). A class already in memory is found without a load.

## §2 The per-building cap (his answer A): the per-building table

**How the set is enumerated (no typed list, no CardID or class-name compare):** controller class default → `GetCardTableAsset().LoadSynchronous()` (row struct checked `FCardRow`) → every row name in table order → `FindRow<FCardRow>` → `IsBuildingCard(CardID, Row->CardType)` → `ResolveCardActorClass(CardID, Row->CardType)` → `Cast<ABuilding>(Class->GetDefaultObject())` → `GetMaxStackHeightMultiplier()`. Types are grouped by equal read value, groups in the order the table first offers each value, and **every type is named** (no "every other building": a dropped type would then vanish with no trace, and naming each one is what makes completeness testable). Each name is `FCardRow::DisplayName`, the name the deck shows.

| `DisplayName` (as rendered) | CardID | CardType | class the game spawns (`ResolveCardActorClass`) | native parent | expected read (`GetMaxStackHeightMultiplier()` on its own class default) | package stores `MaxStackHeightMultiplier`? | group |
|---|---|---|---|---|---|---|---|
| Arrow Tower | ArrowTower | Building | `BP_Building_ArrowTower_C` | `ATower` | 5 | no (0 hits; stores `StaticMesh`, `CardID`) | "5 times that original height for the …" |
| Wall | Wall | Building | `BP_Building_Wall_C` | `ABuilding` | 5 | no | 〃 |
| Bomb Tower | BombTower | Building | `BP_Building_BombTower_C` | `ATower` | 5 | no | 〃 |
| Ballista Tower | BallistaTower | Building | `BP_Building_BallistaTower_C` | `ATower` | 5 | no | 〃 |
| Barracks | Barracks | Building | `BP_Building_Barracks_C` | `ABarracks` | 5 | no (stores `StaticMesh`) | 〃 |
| Deep Mine | DeepMine | Economy (building through `BuildingEconomyCardIDs`) | `BP_Building_DeepMine_C` | `ADeepMine` | 5 | no (stores `StaticMesh`) | 〃 |
| Crystal Tower | CrystalTower | Building | `BP_Building_CrystalTower_C` | `ATower` | 5 | no | 〃 |
| Watch Tower | WatchTower | Building | `BP_Building_WatchTower_C` | `AClimbableTower` | 2 | no | "2 times for the Watch Tower" |

Table order (`Docs/Data/cards.csv`, the import order) is ArrowTower, Wall, BombTower, BallistaTower, Barracks, DeepMine, CrystalTower, WatchTower, which gives the rendered order. Miner is `Economy` but not in `BuildingEconomyCardIDs`, so `IsBuildingCard` is false and it is not listed. The byte scan: all eight packages are real (magic `c1832a9e`, 34975–38949 B); the name table scan finds `MaxStackHeightMultiplier` 0 times in each, while the same scan finds `StaticMesh` in all eight and `CardID` in six (positive control). ⚠️ That predicts "no Blueprint override"; the **reading** is test 21's per-class `AddInfo` line in 5a (value on the class's own default beside its native parent's, and the Blueprint status).

**Wording rules held:** each limit is stated positively ("up to its height limit: 5 times … and 2 times for the Watch Tower"); no "cannot be stacked", "refuses to be stacked" or "inert everywhere" on any page (test 19 green in `sim.py`); no reason for any limit; no "balance" / "design" in any changed literal; no hover colour added; "AT THE HEIGHT LIMIT the click still buys health, the outline stays blue…" is unchanged. One minimum grammatical repair, declared: "Its width and length are not touched." → "**The building's** width and length are not touched.", because after the new list "Its" would read as the Watch Tower's. Same claim.

## §3 `qa/TASK-1586.md` N2: the headline and the one-liner

| field | old | new |
|---|---|---|
| `Cards.StackUpgrade` `DisplayName` | "Stack a tower taller" | "**Upgrade one of your buildings**" |
| one-liner | "…the outline turns blue and the click makes that one taller instead of building a new one." | "…the outline turns blue and the click **upgrades that one** instead of building a new one." |

Why: every building type stacks, not only towers, and a click on a building at its height limit buys health only, so "taller" was false for that click. Test 16 needs only the three wheel rows' headlines distinct (`PickMode.Resize`, `Cards.PlacementResize`, `Interface.MapMarks`); this row is not one of them. All 27 headlines are still pairwise distinct (`sim.py`). The one-liner carries no digit.

## §4 `qa/TASK-1577.md` N1–N4, and every comment that called the numbers OWED or the resolution private (each dated `TASK-1594 (2026-09-29)`, old words quoted)

- **N1** (`.h`, `Detail` doc): "every other number a page shows" → "every other **quantity** a page shows"; and the example list's "once per melee cooldown" is gone (the Attack page now shows that cooldown), replaced by "or shows its value derived". Dated in the doc. **Line count unchanged (1412)**, so no `.generated.h` line macro moves.
- **N2** (`.cpp`, the `Cards.StackUpgrade` citation-block note that called the resolution and `BuildingEconomyCardIDs` "private to the controller"): rewritten true after `TASK-1592`: the resolution is read through the controller's public members; `BuildingEconomyCardIDs` and `CardTableAsset` are still protected and the page never reads either directly. The old sentence is quoted, with `qa/TASK-1577.md` N2's "protected, not private" correction noted.
- **N3** (test 20): new part (e): when the owner's cap is not one, the `Interface.MapMarks` page must contain `" circles at once"`. **Value-free on purpose**, so carried arm 1576-D1 ("250 circles") still reddens only (a) and (a2).
- **N4** (`.h`, `ComposeDetailForDisplay` doc): now says the prose's `{#Name}` tokens are replaced there by the number read from its owner. Kept to one line (no line shift). `FSiegeControlsHelpRegistry` is plain C++, so UHT does not see this doc.
- **Other comments made true and dated** (found by text):
  - `.cpp` derived-number block: "⛔ READS GO THROUGH EXISTING PUBLIC MEMBERS ONLY … are NOT here because reading them needs a new accessor … protected with no public getter … is private" (`qa/TASK-1593.md` table row 3's ~316–322 including "no public getter") → rewritten: they are here now, and why; old text quoted.
  - `.cpp` `Cards.StackUpgrade` note "The height half keeps "a set maximum multiple" (the per-building limits are owed, above)" → the height half is shown; old text quoted.
  - `.cpp` the TASK-1585 paragraph note "the per-building limits are OWED rather than shown, because the class resolution … is private to ASiegePlayerController" (`qa/TASK-1593.md` ~1123–1128) → appended dated clause: no longer owed or private, shown in "WHAT AN UPGRADE BUYS".
  - `.cpp` the TASK-707 transfer block ("Every other number a page shows is DERIVED … (TASK-1576 …: the map-circle cap and the stack health factor …)") → appended the TASK-1594 numbers.
  - `.cpp` `Interface.WarMapReveal`'s "⭐ TASK-1576 …: Two pages now SHOW a number" → appended: now five pages; the quoted 30 gold is still the one typed quantity.
  - `.cpp` `Cards.Discard`, `Hero.Rally`, `Hero.Attack`: a dated note beside each changed literal says which plain words became which token, what it renders at today's values, and which test pins it.
  - Test 20's docstring "the rows' other candidates … are ⛔ NOT shown yet, so nothing here asserts them" → they are shown and pinned by tests 21 and 22; old words quoted.
  - Test 14 (e)'s label "states the flat-fee rule in words instead of restating its value" → "states the flat-fee rule in words: the fee is charged once for the whole hand" (the value is now on the page). The pinned substring "charged once for the whole hand" is unchanged.

## §5 Tests (spec (4) as carried) and `N`

**Added: test 21, `Siegebound.ControlsHelp.EachBuildingTypeShowsItsOwnHeightLimit`** (`FSiegeControlsHelpHeightLimitsTest`). Reads the set HERE through the same public surface (controller class default → `GetCardTableAsset` → `IsBuildingCard` → `ResolveCardActorClass`, W1-safe) and each class's own `GetMaxStackHeightMultiplier()`; extracts the clause after "up to its height limit: " from the composed page.
- **(a-cap)** per type, the nearest `" <N> time"` group head before its (whole-word) name equals that class's read value, formatted HERE (0 fractional digits). Not "a digit somewhere on the page".
- **(a2-cap)** every rendering of the row (own page + each related block, found by walking every row: 2 today, own + `Cards.PlacementResize`) carries the same clause as the own page, with a `Renderings > 0` guard.
- **(c-cap)** every enumerated type has a non-empty `DisplayName` and is named exactly once in the clause.
- **(d-cap)** the reads hold ≥ 2 distinct values; the message says a retune or Blueprint override making them equal would blind (a-cap) and arm cap-i.
- Vacuity guard: ≥ 1 type enumerated. Logs per class: `[ControlsHelp] height limit: card '<id>' '<name>' -> class '<path>', GetMaxStackHeightMultiplier() = <v> on its own class default; native parent '<name>' = <v> (no Blueprint override | the Blueprint OVERRIDES the native value); status <BS_…> (Blueprint '<path>')`, plus the composed clause.

**Added: test 22, `Siegebound.ControlsHelp.DiscardRallyAndAttackNumbersAreReadFromTheirOwners`** (`FSiegeControlsHelpFeeAndHeroNumbersTest`). Owners read HERE through the same getters on the same objects (controller class default; the spawned hero class's default with the same fallback), converted and formatted HERE with the same options.
- **(a)** per number (8), the own page contains the clause: `fee is 20 gold` · `within 6 metre` · `of your hero by 25%` · ` for 5 second` · `a cooldown of 20 second` · `within 1.5 metre` · `cone reaching 30 degree` · `at most once every 0.5 second` (values shown at today's reads; the test builds them from the reads).
- **(a2)** per number, every rendering carries it (1 each today), with a vacuity guard; **(b)** per number, the clause from value + 1 is absent, with a fixture self-check that it differs.
- **(e)** the typed-digit half: `Hero.Rally`'s and `Hero.Attack`'s TEMPLATES (`Row->Detail`) carry no digit, with a scanner self-check and a guard that each template carries `{#` (not vacuous).
- Logs: the hero class (and whether the fallback was taken) with its Blueprint status; each of the seven on the spawned default beside the native `AHeroCharacter` default (the override reading; ⚠️ the native default is a baseline only, no expectation comes from it); the fee.

**Narrowed (spec (4)(b)): test 14 (e)**, `DiscardAllLetterMovesWhileCardDigitsHold`: `CarriesADigit(DiscardDetail)` (the composed page, which now legitimately carries "20") → `CarriesADigit(DiscardRow->Detail.ToString())` (the typed template), label "⛔ ...and neither does its detail TEMPLATE - the fee the page shows is read from DiscardAllCost when the page is composed, never typed". The only narrowed assertion: test 15 (e) already reads templates (TASK-1576), and no other test scans these pages for digits.

**(c) The `::` / `()` guard (test 9) stays green:** no composed page carries any of its ten fragments (`sim.py`; the tokens and names carry none). **Test 19's phrases** absent on every row. **Test 10 (b)** / **test 20 (d)**: no unresolved `{` / `{#` on any composed page. **Test 15 (e)** / **16** / **20 (a)(b)** green. `sim.py`: 0 failures.

**`N` = 571 + 2 = 573.** Measured: `IMPLEMENT_SIMPLE_AUTOMATION_TEST(` declarations across `Source/` = 573; in the help test file 20 → 22. No existing test changed shape (test 14 changed one operand and two labels; test 20 gained part (e)).

## §6 The arms `TASK-1596` owes (`SHIP-§9`)

Byte replaces on BOM-less, LF-only UTF-8, once each (⛔ no `Get-Content` / `Set-Content`, ⛔ no `-replace`). Assert `count(anchor) == 1` across `Source/` first (`arms.py`: **1 each at my bytes**, and no replacement already present except cap-i's, whose replacement is the common expression `GetDefault<ABuilding>()`, 20 hits elsewhere, harmless). Restore by byte copy and re-hash to §10's value. All are C++ mutants needing a recompile. Base size of `SiegeControlsHelpWidget.cpp`: **346171 B**. Error formats: `AutomationTest.cpp:2065` `Expected '%s' to be %d, but it was %d.`, `:2666` `… to be false.`, `:2676` `… to be true.`

### New arms (TASK-1594)

| arm | anchor in `SiegeControlsHelpWidget.cpp` (1 in `Source/`) | replace once with | Δ → size | red test · errors |
|---|---|---|---|---|
| **cap-i** (base-class read) — REQUIRED | `Cast<ABuilding>(HelpBuildingClass->GetDefaultObject())` | `GetDefault<ABuilding>()` | −31 → 346140 | test 21 · **1** |
| **cap-ii** (a dropped class: the one Economy building) — REQUIRED | `if (HelpCardRow == nullptr \|\| !CardRulesDefaults->IsBuildingCard(HelpCardID, HelpCardRow->CardType))` | `if (HelpCardRow == nullptr \|\| !CardRulesDefaults->IsBuildingCard(HelpCardID, HelpCardRow->CardType) \|\| HelpCardRow->CardType == ECardType::Economy)` | +47 → 346218 | test 21 · **1** |
| **TD-Rally** (typed digit) — REQUIRED | `TEXT("After each rally there is a cooldown of {#Rally.Cooldown}. On cooldown the press does nothing, but it ")` | `TEXT("After each rally there is a cooldown of 20 seconds. On cooldown the press does nothing, but it ")` | −7 → 346164 | test 22 · **1** |
| **DF-Discard** (data-follow, `Cards.Discard`) — REQUIRED | `const int32 HelpDiscardFee = FeeDefaults->GetDiscardAllCost();` | `const int32 HelpDiscardFee = FeeDefaults->GetDiscardAllCost() * 2;` | +4 → 346175 | test 22 · **2** |
| **DF-Rally** (data-follow, `Hero.Rally`: the wrong property) — REQUIRED | `const float HelpRallyRadiusMetres = RallyHeroDefaults->GetRallyRadius() / 100.f;` | `const float HelpRallyRadiusMetres = RallyHeroDefaults->GetRallyDuration() / 100.f;` | +2 → 346173 | test 22 · **2** |
| **DF-Attack** (data-follow, `Hero.Attack`: the wrong property) — REQUIRED | `const float HelpAttackCooldownSeconds = AttackHeroDefaults->GetMeleeCooldown();` | `const float HelpAttackCooldownSeconds = AttackHeroDefaults->GetRallyCooldown();` | 0 → 346171 (sha moves) | test 22 · **2** |
| **N3** (plural forms swapped) — REQUIRED | `TEXT("{Count} {PluralCount}\|plural(one=circle,other=circles)")` | `TEXT("{Count} {PluralCount}\|plural(one=circles,other=circle)")` | 0 → 346171 (sha moves) | test 20 · **1** |
| TD-Discard (typed digit on `Cards.Discard`) — optional | `TEXT("The fee is {#Discard.Fee} and it is charged once for the whole hand, flat. Dumping a ")` | `TEXT("The fee is 20 gold and it is charged once for the whole hand, flat. Dumping a ")` | −7 → 346164 | test 14 · **1** |

The data-follow arm per page: `Cards.StackUpgrade` = cap-i (the health factor keeps carried 1576-D2) · `Cards.Discard` = DF-Discard · `Hero.Rally` = DF-Rally · `Hero.Attack` = DF-Attack.

**Expected error lines** (`en` culture; the numbers assume today's reads):
- **cap-i:** `Expected '(a-cap) Building type 'Watch Tower' (card 'WatchTower') is listed under the height limit its OWN class default holds (the nearest '<N> time' before its name)' to be 2, but it was 5.` (The page renders one group, "5 times that original height for the Arrow Tower, … Crystal Tower and Watch Tower". (c-cap), (d-cap), (a2-cap) stay green.)
- **cap-ii:** `Expected '(c-cap) Building type 'Deep Mine' (card 'DeepMine') is named exactly once in Cards.StackUpgrade's height-limit clause' to be 1, but it was 0.` ((a-cap) skips a type that is not named once; the grouping is otherwise unchanged.)
- **TD-Rally:** `Expected '⛔ Row 'Hero.Rally' detail TEMPLATE types NO number - every number the page shows is read from its owner when the page is composed' to be false.` (The typed "20 seconds" equals the owner, so (a) and (a2) stay green by design; the template still carries three tokens, so the vacuity guard stays green.)
- **DF-Discard** (renders "40 gold"): `Expected 'Row 'Cards.Discard' shows the discard fee (GetDiscardAllCost, controller class default, gold) as its owner holds it: 'fee is 20 gold'' to be true.` and `Expected 'Every rendering of row 'Cards.Discard' shows the discard fee (GetDiscardAllCost, controller class default, gold) (its own page and each related block that renders it)' to be 1, but it was 0.`
- **DF-Rally** (renders "0.05 metres"): `Expected 'Row 'Hero.Rally' shows the rally radius (GetRallyRadius, spawned hero class default, uu / 100 = metres) as its owner holds it: 'within 6 metre'' to be true.` and `Expected 'Every rendering of row 'Hero.Rally' shows the rally radius (GetRallyRadius, spawned hero class default, uu / 100 = metres) (its own page and each related block that renders it)' to be 1, but it was 0.`
- **DF-Attack** (renders "20 seconds"): `Expected 'Row 'Hero.Attack' shows the melee cooldown (GetMeleeCooldown, spawned hero class default, seconds) as its owner holds it: 'at most once every 0.5 second'' to be true.` and `Expected 'Every rendering of row 'Hero.Attack' shows the melee cooldown (GetMeleeCooldown, spawned hero class default, seconds) (its own page and each related block that renders it)' to be 1, but it was 0.`
- **N3** (renders "9 circle at once"): `Expected 'Row 'Interface.MapMarks' names the circles in the plural when the owner's cap is not one: ' circles at once'' to be true.` (test 20 (a) "hold up to 9 circle" is a prefix of both forms and stays green.)
- **TD-Discard:** `Expected '⛔ ...and neither does its detail TEMPLATE - the fee the page shows is read from DiscardAllCost when the page is composed, never typed' to be false.`

### Carried arms (`qa/TASK-1577.md` §6), re-counted at my bytes: anchors unmoved, 1 each, same deltas, same red sets

| arm | Δ → mutant size at my bytes | red test · errors (unchanged) | why my tests stay green |
|---|---|---|---|
| 1576-T | −6 → 346165 | test 15 · 1 | typed "9 circles at once" satisfies test 20 (e) |
| 1576-D1 | +24 → 346195 | test 20 · 2 | (e) is value-free: "250 circles at once" still plural |
| 1576-D2 | 0 → 346171 | test 20 · 2 | the health clause is outside test 21's extracted height clause |
| 1585-A | +77 → 346248 | test 19 · 2 | inserted in the last paragraph; test 21's clause ends at the first ". " after its lead |
| 1585-B | −109 → 346062 | test 19 · 3 | `PickMode.Resize` is not read by tests 21/22 |
| 1574-guard | +33 → 346204 | test 9 · 2 | `Hero.Jump` is not read by tests 21/22 |

**Moved pins: none.** `TASK-1585`'s anchors A and B and the 1574 guard anchor are byte-identical and count 1.

### `TASK-1592`'s arms (`qa/TASK-1593.md` §3): anchors unchanged (1 each), ⚠️ **arm-R's red set GROWS** — this is a moved expectation, not a leak

- **arm-G** (`GetRallyCooldown` returns `RallyDuration`): still **HelpAccessors test 1 · 1** only. Test 22 reads the owner through the same getter, so the page and the expectation move together; a getter swap is test 1's claim, not test 22's.
- **arm-H** (optional, empty hero soft class): still **HelpAccessors test 3 · 1** only. The help falls back to the raw `AHeroCharacter`, whose seven values equal `BP_HeroCharacter`'s today, so every page is identical; test 22 logs the fallback.
- **arm-R** (`IsBuildingCard` false for every card): the help's height-limit entry now depends on `IsBuildingCard`, finds no building, returns false, and leaves `{#StackUpgrade.HeightLimits}` **visible** (`HELP-§2` mechanism 2, by design). Expected red set at these bytes: **4 tests, 6 errors**:
  - `Siegebound.HelpAccessors.BuildingCardsResolveThroughTheControllersOwnResolution` · 1 (as in `qa/TASK-1593.md` §3);
  - `Siegebound.ControlsHelp.DetailKeysDeriveThroughTheAccessor` (test 10 (b)) · 3: `Expected 'Row 'Cards.StackUpgrade' leaves no unresolved token in its body on QWERTY' to be false.`, `… on Dvorak' to be false.`, `Expected 'Row 'Cards.PlacementResize' related block 'Cards.StackUpgrade' leaves no unresolved token' to be false.`;
  - `Siegebound.ControlsHelp.ShownNumbersAreReadFromTheirOwners` (test 20 (d)) · 1: `Expected 'Row 'Cards.StackUpgrade' leaves no unresolved number token in its composed detail' to be false.`;
  - `Siegebound.ControlsHelp.EachBuildingTypeShowsItsOwnHeightLimit` (test 21) · 1: `Expected 'At least one placeable building type was enumerated through the game's own resolution, so (a-cap) and (c-cap) are measurements' to be true.`
  - `ResolveCardActorClass` is never reached under arm-R, so no Warning or Error is logged.

**Total arms owed by `TASK-1596`:** TASK-1592's arm-G, arm-R (with the red set above), optional arm-H; TASK-1594's cap-i, cap-ii, TD-Rally, DF-Discard, DF-Rally, DF-Attack, N3 (required) and TD-Discard (optional); and the six carried help arms if 5a re-runs them (they are not new to this wave; the row names them for completeness).

## §7 The 5a shape (`TASK-1596`)

- **`.h`:** two doc comments changed, **line count unchanged (1412)**, so `.generated.h` line macros are unmoved: `USTRUCT` `GENERATED_BODY` 127; `UCLASS` / `GENERATED_BODY` 450/453, 633/636, 933/936. In `SiegeControlsHelpWidget.gen.cpp`, only `FSiegeControlsHelpAction::Detail`'s `Comment` / `ToolTip` metadata (and CRCs) change. The `ComposeDetailForDisplay` doc is on plain C++ `FSiegeControlsHelpRegistry`, invisible to UHT. **Exec-symbol set 9 = 9** (`UFUNCTION(` on code lines: 9 before and after).
- **`.cpp`:** gains `Engine/DataTable.h`, `Siegebound/CardRow.h`, `Siegebound/HeroCharacter.h`, `Siegebound/SiegeGameMode.h`, `Siegebound/SiegePlayerController.h` (all project/engine headers already compiled elsewhere). New anonymous-namespace names (`ResolveHelpHeroDefaults`, `ComposeHelpDiscardFee`, `ComposeHelpRally{Radius,SpeedBonus,Duration,Cooldown}`, `ComposeHelpAttack{Reach,ConeHalfAngle,Cooldown}`, `FSiegeHelpHeightLimitGroup`, `JoinHelpBuildingNames`, `ComposeHelpStackHeightLimits`) and every new local occur nowhere else in `Source/` (grep, whole word; unity blobs merge anonymous namespaces). `names.py`: none of the widget's 29 parsed namespace-scope names, nor its 24 `SiegeControlsHelpText` constants, occurs in the newly included headers (transitively, 19 headers).
- **Test file:** gains the same five includes plus `Engine/Blueprint.h` under `WITH_EDITOR`; a new namespace `SiegeControlsHelpNumbersTestUtils`; test class and test names unique in `Source/`. No local shadows an `FAutomationTestBase` member.
- **Build:** the three help files and the four `TASK-1592` owner headers compile together in one wide, editor-closed build (`TASK-1592`'s new UFUNCTIONs already rule out Live Coding). Then a relaunch (the widget is on the PIE path).
- **Literal-digit census** of code string literals in the widget `.cpp`: **17 = 17** (no digit added to any literal; the new format patterns and tokens carry none).

## §8 Residency (`VER-§12` cl. 7g)

No GUI-editor load and no editor call at all. The building classes, `DT_Cards` and `BP_HeroCharacter` load in the **suite's process** (tests 21 and 22, and every help test that composes the four pages), and first in the GUI editor at 5b's PIE, when the help page opens. The `[ControlsHelp]` `AddInfo` lines of tests 21 and 22 carry each class, its value on its own default beside its native parent's, and (under `WITH_EDITOR`) its generating Blueprint's status: 5a's Blueprint-override reading (`TASK-1576` (2)). ⚠️ Apply `qa/TASK-1593.md` W2's reading rule to these lines too: a status other than `BS_UpToDate` / `BS_UpToDateWithWarnings` is not a reading.

## §9 Page lengths, for `TASK-1597`'s page-fit check

Composed detail, characters, today's values, key tokens (`{Cards.Cancel}`, `{Cards.Discard}`) left unresolved in both columns (`sim.py`):

| page | before | after | Δ |
|---|---|---|---|
| `Cards.StackUpgrade` (own detail) | 2015 | 2149 | **+134** (grew) |
| `Cards.StackUpgrade` headline · one-liner | 20 · 167 | 29 · 163 | +9 · −4 |
| `Cards.PlacementResize` page (renders `Cards.StackUpgrade` as a related block: its headline and body) | 6470 (own + related bodies + headlines) | 6613 | **+143** (grew) |
| `Hero.Attack` | 519 | 556 | **+37** (grew) |
| `Hero.Rally` | 449 | 473 | **+24** (grew) |
| `Cards.Discard` | 978 | 966 | −12 |

The four pages that grew (`Cards.StackUpgrade`, `Cards.PlacementResize`, `Hero.Attack`, `Hero.Rally`) are 5b's page-fit legs. The rendered text of the four changed pages, for 5b's wording leg, is in §12.

## §12 The rendered text of the four changed pages (today's values; `{Cards.Cancel}` / `{Cards.Discard}` are key chips)

**`Cards.StackUpgrade`** — headline "Upgrade one of your buildings" · one-liner "While you are placing a building, hover one you already own of the same card: the outline turns blue and the click upgrades that one instead of building a new one." · the changed paragraph (the rest of the page is unchanged):

> WHAT AN UPGRADE BUYS. Height: each upgrade adds one more copy of the building's ORIGINAL height, up to its height limit: 5 times that original height for the Arrow Tower, Wall, Bomb Tower, Ballista Tower, Barracks, Deep Mine and Crystal Tower, and 2 times for the Watch Tower. The building's width and length are not touched. Health: each upgrade multiplies the building's maximum health by 1.5, compounding, and that half has no ceiling at all — it keeps climbing after the height has stopped. The health is GRANTED rather than repaired: a damaged tower stays exactly as damaged, it is simply damaged out of a bigger pool.

**`Cards.Discard`** — headline and one-liner unchanged · the changed paragraph:

> The fee is 20 gold and it is charged once for the whole hand, flat. Dumping a single dead card costs exactly what dumping a full hand costs, because this prices a hand RESET rather than a per-card cycle — there is no longer any way to bin one card on its own at any price.

**`Hero.Rally`** — headline and one-liner unchanged · the whole page:

> Speeds up every friendly summoned unit within 6 metres of your hero by 25% for 5 seconds — units only, never the hero, never enemy units. Friendly miners are included, since a miner is a kind of summoned unit.
>
> After each rally there is a cooldown of 20 seconds. On cooldown the press does nothing, but it still tells the HUD how much cooldown is left so the HUD can flash it; when the cooldown runs out, the HUD is told that Rally is ready again. A dead hero cannot rally.

**`Hero.Attack`** — headline and one-liner unchanged · the changed first paragraph (the second is unchanged):

> One swing damages every enemy unit, hero, building and castle within 1.5 metres of you and inside a cone reaching 30 degrees to either side of where you face, and you can swing at most once every 0.5 seconds. Damage per swing is worked out fresh each time — the base damage plus the Sharpened Blade stacks. No friendly fire.

`Cards.PlacementResize` renders the `Cards.StackUpgrade` block with the new headline and the same body.

## §10 Size valve (carried spec (5))

**Not tripped.** All four pages were done whole, in the row's order (`Cards.StackUpgrade`, `Cards.Discard`, `Hero.Rally`, `Hero.Attack`), plus N2 and N1–N4. Nothing is OWED.

## §11 Files touched, sha256 before → after

| file | before | after | size · lines |
|---|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` | `6a9ba04491fea8b58b4538fe8e9496a674db0c23fa4478bdf48fa3679a29fa6b` | `6641802920079d5c70c7c1f5182246f46bc3262d4c0536102cbe5d6f5fc3fa0d` | 317440 → 346171 B · 4977 → 5496 |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h` | `7ed5a066b6c5f608a40b23a95ce858db786bdad0e5565eb60cf2583f8a4d1527` | `25e80996c543dbd816253aecfbbd6222594fb769115cba95a3782b467daf924c` | 81822 → 82041 B · 1412 → 1412 |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` | `588a4180553851f7c52da9e4907aac07d522b2c67db1a9e6121c1c6790de3fa4` | `ee68a048a8a37ad23d9adb6d1d5339289934a4ff9cad21e35dff335426cdf0e8` | 172078 → 210420 B · 2981 → 3649 |

All LF-only (CR = 0), no BOM, before and after. Also written: this handoff, and `TASKBOARD.md` (this row's `status:` line only). `git diff --numstat b9db99c`: `.cpp` +555 / −36, `.h` +4 / −4, test +675 / −7. The full `git --no-optional-locks diff -U0 b9db99c` of the three files is the appendix (saved too as `t1594/diff.txt`, sha256 `f196db5c…baa71f`).

## What QA (`TASK-1595`) should scrutinize

1. **Every read is public and on the right object** (§1): the controller's class default for the fee and the resolution; each building class's own default for its limit (⛔ no `GetDefault<ABuilding>()` in the new code; cap-i is exactly that mutation); the spawned hero class's default through `GetHeroPawnClassAsset()` with the game's fallback. `GetDefault<AHeroCharacter>()` appears only in test 22, as the override reading's logged baseline.
2. **W1**: `ResolveCardActorClass` behind `IsBuildingCard` in both the composer and test 21, and only at compose time.
3. **The per-building wording**: every type named, grouping computed from reads, positive phrasing, no reason, no "balance"/"design", and the "Its" → "The building's" repair (§2).
4. **Each test can fail** (read the code paths): (a-cap)'s nearest-head rule, (c-cap)'s whole-word count, (d-cap), test 22's (a)/(b)/(a2)/(e), test 14's narrowed operand, test 20 (e)'s value-free plural pin.
5. **arm-R's red set grows** (§6) because the help page now depends on `IsBuildingCard`; `TASK-1596` needs that reading rule, not "exactly one red test".
6. **Compile shapes, since nothing was compiled**: `FText::AsNumber(float|int32, const FNumberFormattingOptions*)` (`Text.h:428/432`, `[[nodiscard]]`, used), `FFormatArgumentValue(const float)` (non-explicit, `Text.h:1002`) for the float plural arguments (the plural modifier handles `Float`, `TextFormatArgumentModifier.cpp`), `TSoftClassPtr::LoadSynchronous() const` returning `UClass*` (`SoftObjectPtr.h:1008`), `UClass::GetDefaultObject() const` (`Class.h:4519`), `FString::Find(const TCHAR*, ESearchCase, ESearchDir, int32)` with its FromStart clamp (`String.cpp.inl:464`), the conditional operator between two `TEXT` literals of different lengths (both decay to `const TCHAR*`, the `ClimbableTower.cpp` precedent), local structs as `TArray` element types, and pointer-to-member calls on const objects (the `TASK-1592` test's pattern).

## Not examined / limitations

- ⛔ **No compile, no suite, no PIE, no pixels.** `N = 573`, every arm's red test and error count, the rendered sentences and the page lengths are text-level predictions from `sim.py` / `arms.py`, assuming today's C++ defaults, no Blueprint override, the `en` culture ("1.5", "25%") and the table's import order. `TASK-1596` / `TASK-1597` measure them.
- **The Blueprint-override reading for the eight building classes is a prediction here** (the name-table byte scan: no package stores `MaxStackHeightMultiplier`); test 21's `AddInfo` lines in 5a are the reading. Same for the hero: `qa/TASK-1593.md` ruling 4's scan, and test 22's lines.
- **Load-time log warnings.** Tests 21/22 and every help test that composes these pages load `DT_Cards`, the eight building Blueprints and `BP_HeroCharacter` in the suite's process. If loading one of them logs a Warning and the automation controller elevates warnings (`bElevateLogWarningsToErrors`), the first help test to compose that page would carry it. The same classes already load in `SiegeCardRosterTest` (buildings) and in `TASK-1592`'s test 3 (hero), so this is the exposure those tests already have; not a code defect if it happens.
- **First open in a match** of `Cards.StackUpgrade` (or `Cards.PlacementResize`) may load up to eight building Blueprints synchronously, and of `Hero.Rally` / `Hero.Attack` the hero Blueprint (usually already resident in a match): a possible one-time hitch at the click, never per frame. 5b's cl. 7g walk covers the GUI editor's first load.
- **Plural on floats:** the noun is chosen from the unrounded float, so a value like 1.004 s would show "1 seconds". No shipped value is near that. In a decimal-comma culture "1.5" reads "1,5" (the tests format the same way).
- **A cap below 1** (hand-edited past `ClampMin = "1"`) would show as read (e.g. "0 times") while `StackHeightMultiplier` applies `FMath::Max(1, …)`. The spec binds the read to `GetMaxStackHeightMultiplier()`, so I show the raw value.
- **"within 1.5 metres of you"** simplifies the melee range test, which measures to the nearest point of the target's collision (so a castle wall counts). Not false, and the old prose ("within your melee reach") said no more.
- **Enumeration scope:** every row `IsBuildingCard` accepts and `ResolveCardActorClass` resolves is listed, whether or not it is in the default deck (all eight have `MaxCopies` > 0 in `cards.csv`). Two rows with the same `DisplayName` would be named twice and turn (c-cap) red, by design.
- **cap-ii is data-dependent:** it drops the Economy building (Deep Mine today). With no Economy building in the table it would not discriminate; 5a should record that as not discriminating, not as a pass.
- **Left as they are (not false, noted):** `Cards.StackUpgrade`'s health sentence says "a damaged tower stays exactly as damaged" (an example, true of towers); the `Cards.Discard` citation block's older sentence "the prose says "a single set amount"" stays with the dated TASK-1594 correction directly beneath it (the file's convention); test 14's section banner "⛔ THE FEE IS PUT IN WORDS" stays, with the dated note under it.
- `names.py`'s namespace-name parse is heuristic (29 names found in the anonymous namespace; the 24 `SiegeControlsHelpText` constants checked separately). The compile is the final arbiter.
- Line numbers in this handoff are hints at these bytes; every site is cited by text.

## Appendix: `git --no-optional-locks diff -U0 b9db99c` (the three help files)

```diff
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
index fb0121d..3bd8a14 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
@@ -31,0 +32,10 @@
+// TASK-1594 (2026-09-29) — the owners of the numbers 🧑 his answer A and his "all of it" added,
+// read and never typed (`HELP-§2`), each through a PUBLIC member TASK-1592 added or made public:
+// the controller for the card table, the building-card rule, the card → class resolution and the
+// discard fee; the game mode for the hero class it spawns; the hero for the Rally and Attack
+// numbers. All READ-ONLY to this file; see the TASK-1594 half of the derived-number block below.
+#include "Engine/DataTable.h"
+#include "Siegebound/CardRow.h"
+#include "Siegebound/HeroCharacter.h"
+#include "Siegebound/SiegeGameMode.h"
+#include "Siegebound/SiegePlayerController.h"
@@ -316,7 +326,11 @@ namespace
-	//  ⛔ READS GO THROUGH EXISTING PUBLIC MEMBERS ONLY (TASK-1576 spec (3)). The row's other
-	//  candidates (the discard fee, the four Rally values, the melee reach / cone / cooldown, and
-	//  each building's own height limit) are NOT here because reading them needs a new accessor in
-	//  an owner file: DiscardAllCost and the hero's Melee* / Rally* tunables are protected with no
-	//  public getter, and the card-to-building-class resolution the game uses
-	//  (ASiegePlayerController::ResolveCardActorClass / IsBuildingCard) is private. They are listed
-	//  in handoffs/TASK-1576-programmer.md for the manager to board; ⛔ none is typed instead.
+	//  ⛔ READS GO THROUGH PUBLIC MEMBERS ONLY (TASK-1576 spec (3)). ⭐ TASK-1594 (2026-09-29): the
+	//  row's other candidates (the discard fee, the four Rally values, the melee reach / cone /
+	//  cooldown, and each building's own height limit) are now HERE TOO, in the TASK-1594 half of
+	//  this block below. TASK-1592 made them readable: GetDiscardAllCost() and the seven
+	//  GetMelee* / GetRally* getters are public, ResolveCardActorClass and IsBuildingCard moved to
+	//  `public:` unchanged, and GetCardTableAsset() / ASiegeGameMode::GetHeroPawnClassAsset() are
+	//  new public reads. (Until TASK-1594 this paragraph said those candidates were NOT here
+	//  "because reading them needs a new accessor in an owner file: DiscardAllCost and the hero's
+	//  Melee* / Rally* tunables are protected with no public getter, and the card-to-building-class
+	//  resolution the game uses (ASiegePlayerController::ResolveCardActorClass / IsBuildingCard) is
+	//  private", which was true until TASK-1592.) ⛔ Still none is typed.
@@ -403,0 +418,427 @@ namespace
+	// ════════════════════════════════════════════════════════════════════════════════════
+	//  ⭐⭐ TASK-1594 (2026-09-29) — THE REST OF THE ROW'S NUMBERS: 🧑 HIS ANSWER A TO
+	//  Q-STACK-CAP-2026-09-28 ("show the numbers": each building type's own height limit) AND HIS
+	//  "YES, ALL OF IT" (the discard fee, Rally's four values, Attack's three). The same `{#Name}`
+	//  tokens and the SAME splice as the two entries above (⛔ no second splice), and every read
+	//  goes through a PUBLIC member (TASK-1592) on the object the game uses.
+	//
+	//  WHICH OBJECT EACH ENTRY READS, AND WHY (TASK-1576 spec (2), carried by TASK-1594):
+	//    • the discard fee: ASiegePlayerController::GetDiscardAllCost() on the controller's CLASS
+	//      DEFAULT. The game's controller IS that native class: ASiegeGameMode's constructor sets
+	//      `PlayerControllerClass = ASiegePlayerController::StaticClass();` and no Blueprint
+	//      subclass of the controller exists (qa/TASK-1593.md ruling 2). DiscardAllCost is
+	//      EditDefaultsOnly with no Config specifier, no package stores a value for it
+	//      (qa/TASK-1593.md ruling 4), and nothing in Source/ writes it. ⇒ the live controller
+	//      cannot charge a different fee from the one read here.
+	//    • Rally's and Attack's numbers: the seven AHeroCharacter getters, on the class default of
+	//      the hero class the game SPAWNS, which ResolveHelpHeroDefaults finds through
+	//      ASiegeGameMode::GetHeroPawnClassAsset() on the game mode's class default (the arena
+	//      runs the native ASiegeGameMode through GlobalDefaultGameMode, and no Blueprint subclass
+	//      of it exists; handoffs/TASK-1592-programmer.md §2). That is BP_HeroCharacter today.
+	//      ⛔ NOT GetDefault<AHeroCharacter>(): a Blueprint child may override a native default,
+	//      and the class the game spawns is the one this page describes. (No package stores a
+	//      value for any of the seven, so BP_HeroCharacter inherits the native defaults today,
+	//      qa/TASK-1593.md ruling 4; reading ITS default is what keeps a later Blueprint retune
+	//      on the page.) The spawned hero holds its class default's values: the game spawns the
+	//      hero rather than placing one, and no C++ writes any of the seven
+	//      (handoffs/TASK-1592-programmer.md §1).
+	//    • each building type's height limit: ABuilding::GetMaxStackHeightMultiplier() on the
+	//      class default of EACH placeable building class, found by the game's OWN card → class
+	//      resolution on the controller's class default: the table GetCardTableAsset() names,
+	//      then IsBuildingCard, then ResolveCardActorClass. EnterPlacementMode asks IsBuildingCard
+	//      and TryConfirmPlacement spawns `ResolveCardActorClass(PendingCardID, PendingCardType)`,
+	//      so this set is the set the game places and it cannot drift from it. ⛔ NEVER one
+	//      base-class read for all of them (GetDefault<ABuilding>(), the pre-2026-09-03 bug shape
+	//      Building.cpp's StackHeightMultiplier note records): the ceiling is PER CLASS
+	//      (STACK-§10 cl. 2) and a Blueprint child may raise it. ⛔ No CardID or class-name compare
+	//      singles a building out (STACK-§2): the climbable tower lands in its own group only
+	//      because its read value differs. Each type is named by its card's DisplayName (FCardRow),
+	//      the name the deck shows the player.
+	//
+	//  ⚠️ LOADS, AND WHEN (qa/TASK-1593.md W1). ResolveCardActorClass loads a class on every call,
+	//  logs a Warning for a missing class and an ERROR for a card type that is not a placement
+	//  type; GetHeroPawnClassAsset().LoadSynchronous() loads the hero Blueprint. ⇒ both run ONLY
+	//  inside these entries, i.e. only when a page is COMPOSED (ComposeDetailForDisplay, which the
+	//  widget reaches once per page open through ShowDetailForAction → ComposeDetailContent: ⛔ never
+	//  per frame and ⛔ never from the row list), and ResolveCardActorClass is called ONLY for a card
+	//  IsBuildingCard accepts, so its Error branch cannot be reached from this file. Once a class is
+	//  in memory a later call finds it there.
+	// ════════════════════════════════════════════════════════════════════════════════════
+
+	/**
+	 *  The class default of the hero class the game spawns (the reason is in the block comment
+	 *  above), or null. ⭐ THE FALLBACK IS THE GAME'S OWN: ASiegeGameMode::ResolveHeroPawnClass
+	 *  returns the loaded HeroPawnClassAsset, and when `HeroPawnClassAsset.LoadSynchronous()` is
+	 *  null it returns `AHeroCharacter::StaticClass()`, the raw native hero. This does the same two
+	 *  steps in the same order, through the public GetHeroPawnClassAsset(). It does not cache and
+	 *  does not log: the game mode warns once about a missing Blueprint on its own spawn path.
+	 */
+	const AHeroCharacter* ResolveHelpHeroDefaults()
+	{
+		const ASiegeGameMode* const HeroModeDefaults = GetDefault<ASiegeGameMode>();
+		if (HeroModeDefaults == nullptr)
+		{
+			return nullptr;
+		}
+
+		UClass* HelpHeroClass = HeroModeDefaults->GetHeroPawnClassAsset().LoadSynchronous();
+		if (HelpHeroClass == nullptr)
+		{
+			HelpHeroClass = AHeroCharacter::StaticClass();
+		}
+
+		return Cast<AHeroCharacter>(HelpHeroClass->GetDefaultObject());
+	}
+
+	/**
+	 *  `{#Discard.Fee}` — the discard-all fee, e.g. "20 gold". Owner: ASiegePlayerController::
+	 *  DiscardAllCost, read through the public GetDiscardAllCost() on the controller's class
+	 *  default (TASK-1592); the reason is in the block comment above.
+	 */
+	bool ComposeHelpDiscardFee(FText& OutNumberText)
+	{
+		const ASiegePlayerController* const FeeDefaults = GetDefault<ASiegePlayerController>();
+		if (FeeDefaults == nullptr)
+		{
+			return false;
+		}
+
+		const int32 HelpDiscardFee = FeeDefaults->GetDiscardAllCost();
+
+		// Conversion: none. It is an amount of gold and DiscardAllCost is a whole number (int32),
+		// so it is shown with no fractional digits and followed by the word "gold".
+		FNumberFormattingOptions GoldOptions;
+		GoldOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(0);
+
+		FFormatNamedArguments NumberArgs;
+		NumberArgs.Add(TEXT("Gold"), FText::AsNumber(HelpDiscardFee, &GoldOptions));
+		OutNumberText = FText::Format(FText::FromString(FString(TEXT("{Gold} gold"))), NumberArgs);
+		return true;
+	}
+
+	/**
+	 *  `{#Rally.Radius}` — how far Rally reaches from the hero, e.g. "6 metres". Owner:
+	 *  AHeroCharacter::RallyRadius, read through GetRallyRadius() (TASK-1592) on the spawned hero
+	 *  class's default. AHeroCharacter::Rally buffs a unit when `FVector::DistSquared(MyLocation,
+	 *  FriendlyUnit->GetActorLocation()) > RallyRadiusSquared` is false.
+	 */
+	bool ComposeHelpRallyRadius(FText& OutNumberText)
+	{
+		const AHeroCharacter* const RallyHeroDefaults = ResolveHelpHeroDefaults();
+		if (RallyHeroDefaults == nullptr)
+		{
+			return false;
+		}
+
+		const float HelpRallyRadiusMetres = RallyHeroDefaults->GetRallyRadius() / 100.f;
+
+		// Conversion: RallyRadius is in Unreal units (centimetres), so ÷ 100 gives metres. At most
+		// two fractional digits, no trailing zeros. The noun is chosen by the converted value.
+		FNumberFormattingOptions MetreOptions;
+		MetreOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(2);
+
+		FFormatNamedArguments NumberArgs;
+		NumberArgs.Add(TEXT("Metres"), FText::AsNumber(HelpRallyRadiusMetres, &MetreOptions));
+		NumberArgs.Add(TEXT("PluralMetres"), HelpRallyRadiusMetres);
+		OutNumberText = FText::Format(
+			FText::FromString(FString(TEXT("{Metres} {PluralMetres}|plural(one=metre,other=metres)"))), NumberArgs);
+		return true;
+	}
+
+	/**
+	 *  `{#Rally.SpeedBonus}` — how much faster a rallied unit moves, e.g. "25%". Owner:
+	 *  AHeroCharacter::RallySpeedBonus, read through GetRallySpeedBonus() (TASK-1592) on the
+	 *  spawned hero class's default.
+	 */
+	bool ComposeHelpRallySpeedBonus(FText& OutNumberText)
+	{
+		const AHeroCharacter* const RallyHeroDefaults = ResolveHelpHeroDefaults();
+		if (RallyHeroDefaults == nullptr)
+		{
+			return false;
+		}
+
+		const float HelpRallyBonusPercent = RallyHeroDefaults->GetRallySpeedBonus() * 100.f;
+
+		// Conversion: RallySpeedBonus is the fraction AHeroCharacter::Rally adds to a unit's move
+		// speed (`const float SpeedMultiplier = 1.f + RallySpeedBonus;`), so × 100 gives the percent
+		// the speed rises by. At most one fractional digit, followed by a percent sign.
+		FNumberFormattingOptions PercentOptions;
+		PercentOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(1);
+
+		FFormatNamedArguments NumberArgs;
+		NumberArgs.Add(TEXT("Percent"), FText::AsNumber(HelpRallyBonusPercent, &PercentOptions));
+		OutNumberText = FText::Format(FText::FromString(FString(TEXT("{Percent}%"))), NumberArgs);
+		return true;
+	}
+
+	/**
+	 *  `{#Rally.Duration}` — how long a rallied unit keeps the boost, e.g. "5 seconds". Owner:
+	 *  AHeroCharacter::RallyDuration (handed to ApplyMoveSpeedBuff), read through
+	 *  GetRallyDuration() (TASK-1592) on the spawned hero class's default.
+	 */
+	bool ComposeHelpRallyDuration(FText& OutNumberText)
+	{
+		const AHeroCharacter* const RallyHeroDefaults = ResolveHelpHeroDefaults();
+		if (RallyHeroDefaults == nullptr)
+		{
+			return false;
+		}
+
+		const float HelpRallyDurationSeconds = RallyHeroDefaults->GetRallyDuration();
+
+		// Conversion: none. RallyDuration is already in seconds. At most two fractional digits, no
+		// trailing zeros. The noun is chosen by the value.
+		FNumberFormattingOptions SecondOptions;
+		SecondOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(2);
+
+		FFormatNamedArguments NumberArgs;
+		NumberArgs.Add(TEXT("Seconds"), FText::AsNumber(HelpRallyDurationSeconds, &SecondOptions));
+		NumberArgs.Add(TEXT("PluralSeconds"), HelpRallyDurationSeconds);
+		OutNumberText = FText::Format(
+			FText::FromString(FString(TEXT("{Seconds} {PluralSeconds}|plural(one=second,other=seconds)"))), NumberArgs);
+		return true;
+	}
+
+	/**
+	 *  `{#Rally.Cooldown}` — how long after a rally the next one waits, e.g. "20 seconds". Owner:
+	 *  AHeroCharacter::RallyCooldown (the `SinceLastRally < RallyCooldown` gate and the
+	 *  OnRallyReady timer), read through GetRallyCooldown() (TASK-1592) on the spawned hero
+	 *  class's default.
+	 */
+	bool ComposeHelpRallyCooldown(FText& OutNumberText)
+	{
+		const AHeroCharacter* const RallyHeroDefaults = ResolveHelpHeroDefaults();
+		if (RallyHeroDefaults == nullptr)
+		{
+			return false;
+		}
+
+		const float HelpRallyCooldownSeconds = RallyHeroDefaults->GetRallyCooldown();
+
+		// Conversion: none. RallyCooldown is already in seconds. At most two fractional digits, no
+		// trailing zeros. The noun is chosen by the value.
+		FNumberFormattingOptions SecondOptions;
+		SecondOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(2);
+
+		FFormatNamedArguments NumberArgs;
+		NumberArgs.Add(TEXT("Seconds"), FText::AsNumber(HelpRallyCooldownSeconds, &SecondOptions));
+		NumberArgs.Add(TEXT("PluralSeconds"), HelpRallyCooldownSeconds);
+		OutNumberText = FText::Format(
+			FText::FromString(FString(TEXT("{Seconds} {PluralSeconds}|plural(one=second,other=seconds)"))), NumberArgs);
+		return true;
+	}
+
+	/**
+	 *  `{#Attack.Reach}` — how far one swing reaches, e.g. "1.5 metres". Owner:
+	 *  AHeroCharacter::MeleeRange (DoMeleeAttack skips a target when `Distance > MeleeRange`),
+	 *  read through GetMeleeRange() (TASK-1592) on the spawned hero class's default.
+	 */
+	bool ComposeHelpAttackReach(FText& OutNumberText)
+	{
+		const AHeroCharacter* const AttackHeroDefaults = ResolveHelpHeroDefaults();
+		if (AttackHeroDefaults == nullptr)
+		{
+			return false;
+		}
+
+		const float HelpAttackReachMetres = AttackHeroDefaults->GetMeleeRange() / 100.f;
+
+		// Conversion: MeleeRange is in Unreal units (centimetres), so ÷ 100 gives metres. At most
+		// two fractional digits, no trailing zeros. The noun is chosen by the converted value.
+		FNumberFormattingOptions MetreOptions;
+		MetreOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(2);
+
+		FFormatNamedArguments NumberArgs;
+		NumberArgs.Add(TEXT("Metres"), FText::AsNumber(HelpAttackReachMetres, &MetreOptions));
+		NumberArgs.Add(TEXT("PluralMetres"), HelpAttackReachMetres);
+		OutNumberText = FText::Format(
+			FText::FromString(FString(TEXT("{Metres} {PluralMetres}|plural(one=metre,other=metres)"))), NumberArgs);
+		return true;
+	}
+
+	/**
+	 *  `{#Attack.ConeHalfAngle}` — how far to each side of the hero's facing the swing reaches,
+	 *  e.g. "30 degrees". Owner: AHeroCharacter::MeleeHalfAngleDegrees (DoMeleeAttack's
+	 *  `FMath::Cos(FMath::DegreesToRadians(MeleeHalfAngleDegrees))` cone test on the horizontal
+	 *  plane), read through GetMeleeHalfAngleDegrees() (TASK-1592) on the spawned hero class's
+	 *  default.
+	 */
+	bool ComposeHelpAttackConeHalfAngle(FText& OutNumberText)
+	{
+		const AHeroCharacter* const AttackHeroDefaults = ResolveHelpHeroDefaults();
+		if (AttackHeroDefaults == nullptr)
+		{
+			return false;
+		}
+
+		const float HelpAttackHalfAngleDegrees = AttackHeroDefaults->GetMeleeHalfAngleDegrees();
+
+		// Conversion: none. MeleeHalfAngleDegrees is already in degrees, measured from straight
+		// ahead to one edge of the cone. At most one fractional digit, no trailing zeros. The noun
+		// is chosen by the value.
+		FNumberFormattingOptions DegreeOptions;
+		DegreeOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(1);
+
+		FFormatNamedArguments NumberArgs;
+		NumberArgs.Add(TEXT("Degrees"), FText::AsNumber(HelpAttackHalfAngleDegrees, &DegreeOptions));
+		NumberArgs.Add(TEXT("PluralDegrees"), HelpAttackHalfAngleDegrees);
+		OutNumberText = FText::Format(
+			FText::FromString(FString(TEXT("{Degrees} {PluralDegrees}|plural(one=degree,other=degrees)"))), NumberArgs);
+		return true;
+	}
+
+	/**
+	 *  `{#Attack.Cooldown}` — the shortest time between two swings, e.g. "0.5 seconds". Owner:
+	 *  AHeroCharacter::MeleeCooldown (DoMeleeAttack returns when `(Now - LastMeleeTime) <
+	 *  MeleeCooldown`), read through GetMeleeCooldown() (TASK-1592) on the spawned hero class's
+	 *  default.
+	 */
+	bool ComposeHelpAttackCooldown(FText& OutNumberText)
+	{
+		const AHeroCharacter* const AttackHeroDefaults = ResolveHelpHeroDefaults();
+		if (AttackHeroDefaults == nullptr)
+		{
+			return false;
+		}
+
+		const float HelpAttackCooldownSeconds = AttackHeroDefaults->GetMeleeCooldown();
+
+		// Conversion: none. MeleeCooldown is already in seconds. At most two fractional digits, no
+		// trailing zeros. The noun is chosen by the value.
+		FNumberFormattingOptions SecondOptions;
+		SecondOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(2);
+
+		FFormatNamedArguments NumberArgs;
+		NumberArgs.Add(TEXT("Seconds"), FText::AsNumber(HelpAttackCooldownSeconds, &SecondOptions));
+		NumberArgs.Add(TEXT("PluralSeconds"), HelpAttackCooldownSeconds);
+		OutNumberText = FText::Format(
+			FText::FromString(FString(TEXT("{Seconds} {PluralSeconds}|plural(one=second,other=seconds)"))), NumberArgs);
+		return true;
+	}
+
+	/**
+	 *  The placeable building types that read the SAME height limit, in the order the card table
+	 *  first offers each value. ⭐ Built from the reads, ⛔ never from a typed list: a retune, a
+	 *  Blueprint override or a new building card moves a type between groups with no text edit.
+	 */
+	struct FSiegeHelpHeightLimitGroup
+	{
+		int32 HeightLimit = 0;
+		TArray<FString> BuildingNames;
+	};
+
+	/** "the A", "the A and B", "the A, B and C": one group's names as one list with one article. */
+	FString JoinHelpBuildingNames(const TArray<FString>& BuildingNames)
+	{
+		FString JoinedNames(TEXT("the "));
+		for (int32 NameIndex = 0; NameIndex < BuildingNames.Num(); ++NameIndex)
+		{
+			if (NameIndex > 0)
+			{
+				JoinedNames += (NameIndex == BuildingNames.Num() - 1) ? TEXT(" and ") : TEXT(", ");
+			}
+			JoinedNames += BuildingNames[NameIndex];
+		}
+		return JoinedNames;
+	}
+
+	/**
+	 *  `{#StackUpgrade.HeightLimits}` — 🧑 his answer A: every placeable building type's own
+	 *  height limit, e.g. "5 times that original height for the Arrow Tower, Wall … and Crystal
+	 *  Tower, and 2 times for the Watch Tower" at today's values. Owner: ABuilding::
+	 *  MaxStackHeightMultiplier, read through the public GetMaxStackHeightMultiplier() on EACH
+	 *  building class's OWN class default; the set, the object and the loads are in the block
+	 *  comment above.
+	 *
+	 *  ⭐ Every type is NAMED, grouped by equal value. ⛔ There is no "every other building": a type
+	 *  the enumeration dropped would then vanish from the page with no trace, and naming each one is
+	 *  what lets the suite prove every placeable type is covered exactly once (test 21's (c-cap)).
+	 *  ⛔ It gives no reason for any limit and never says a building cannot be stacked: each limit
+	 *  is stated positively, as how far it stacks (STACK-§10 cl. 2; qa/TASK-1586.md ruling 8).
+	 */
+	bool ComposeHelpStackHeightLimits(FText& OutNumberText)
+	{
+		const ASiegePlayerController* const CardRulesDefaults = GetDefault<ASiegePlayerController>();
+		if (CardRulesDefaults == nullptr)
+		{
+			return false;
+		}
+
+		const UDataTable* const HelpCardTable = CardRulesDefaults->GetCardTableAsset().LoadSynchronous();
+		if (HelpCardTable == nullptr || HelpCardTable->GetRowStruct() != FCardRow::StaticStruct())
+		{
+			return false;
+		}
+
+		TArray<FSiegeHelpHeightLimitGroup> HeightLimitGroups;
+		for (const FName& HelpCardID : HelpCardTable->GetRowNames())
+		{
+			const FCardRow* const HelpCardRow =
+				HelpCardTable->FindRow<FCardRow>(HelpCardID, TEXT("SiegeControlsHelp height limits"), /*bWarnIfRowMissing=*/ false);
+
+			// ⛔ qa/TASK-1593.md W1: ResolveCardActorClass is reached ONLY for a card the game's own
+			// building rule accepts, so its "not a placement type" Error can never fire from here.
+			if (HelpCardRow == nullptr || !CardRulesDefaults->IsBuildingCard(HelpCardID, HelpCardRow->CardType))
+			{
+				continue;
+			}
+
+			const UClass* const HelpBuildingClass = CardRulesDefaults->ResolveCardActorClass(HelpCardID, HelpCardRow->CardType);
+			const ABuilding* const HelpBuildingDefaults =
+				HelpBuildingClass != nullptr ? Cast<ABuilding>(HelpBuildingClass->GetDefaultObject()) : nullptr;
+			if (HelpBuildingDefaults == nullptr)
+			{
+				// The game refuses to play a card whose class does not resolve, and spends no gold,
+				// so that card places nothing and has no limit to show. ResolveCardActorClass has
+				// already logged the missing class.
+				continue;
+			}
+
+			// ⭐ THE READ: this building class's OWN class default, through the public accessor.
+			const int32 HelpHeightLimit = HelpBuildingDefaults->GetMaxStackHeightMultiplier();
+
+			FSiegeHelpHeightLimitGroup* MatchingGroup = HeightLimitGroups.FindByPredicate(
+				[HelpHeightLimit](const FSiegeHelpHeightLimitGroup& ExistingLimitGroup) { return ExistingLimitGroup.HeightLimit == HelpHeightLimit; });
+			if (MatchingGroup == nullptr)
+			{
+				MatchingGroup = &HeightLimitGroups.AddDefaulted_GetRef();
+				MatchingGroup->HeightLimit = HelpHeightLimit;
+			}
+			MatchingGroup->BuildingNames.Add(HelpCardRow->DisplayName);
+		}
+
+		if (HeightLimitGroups.Num() == 0)
+		{
+			return false;
+		}
+
+		// Conversion: none. MaxStackHeightMultiplier is already a whole multiple of the building's
+		// authored height (int32; StackHeightMultiplier caps the height at exactly that multiple),
+		// so it is shown with no fractional digits and followed by "time" or "times", chosen by
+		// the value.
+		FNumberFormattingOptions MultipleOptions;
+		MultipleOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(0);
+
+		FString HeightLimitsClause;
+		for (int32 GroupIndex = 0; GroupIndex < HeightLimitGroups.Num(); ++GroupIndex)
+		{
+			const FSiegeHelpHeightLimitGroup& LimitGroup = HeightLimitGroups[GroupIndex];
+			if (GroupIndex > 0)
+			{
+				HeightLimitsClause += (GroupIndex == HeightLimitGroups.Num() - 1) ? TEXT(", and ") : TEXT(", ");
+			}
+
+			FFormatNamedArguments NumberArgs;
+			NumberArgs.Add(TEXT("Multiple"), FText::AsNumber(LimitGroup.HeightLimit, &MultipleOptions));
+			NumberArgs.Add(TEXT("PluralMultiple"), LimitGroup.HeightLimit);
+			HeightLimitsClause += FText::Format(FText::FromString(FString(GroupIndex == 0
+				? TEXT("{Multiple} {PluralMultiple}|plural(one=time,other=times) that original height for ")
+				: TEXT("{Multiple} {PluralMultiple}|plural(one=time,other=times) for "))), NumberArgs).ToString();
+			HeightLimitsClause += JoinHelpBuildingNames(LimitGroup.BuildingNames);
+		}
+
+		OutNumberText = FText::FromString(HeightLimitsClause);
+		return true;
+	}
+
@@ -408 +849,11 @@ namespace
-		{ TEXT("{#StackUpgrade.HealthFactor}"), &ComposeHelpStackHealthFactor }
+		{ TEXT("{#StackUpgrade.HealthFactor}"), &ComposeHelpStackHealthFactor },
+		// ⭐ TASK-1594 (2026-09-29):
+		{ TEXT("{#StackUpgrade.HeightLimits}"), &ComposeHelpStackHeightLimits },
+		{ TEXT("{#Discard.Fee}"),               &ComposeHelpDiscardFee },
+		{ TEXT("{#Rally.Radius}"),              &ComposeHelpRallyRadius },
+		{ TEXT("{#Rally.SpeedBonus}"),          &ComposeHelpRallySpeedBonus },
+		{ TEXT("{#Rally.Duration}"),            &ComposeHelpRallyDuration },
+		{ TEXT("{#Rally.Cooldown}"),            &ComposeHelpRallyCooldown },
+		{ TEXT("{#Attack.Reach}"),              &ComposeHelpAttackReach },
+		{ TEXT("{#Attack.ConeHalfAngle}"),      &ComposeHelpAttackConeHalfAngle },
+		{ TEXT("{#Attack.Cooldown}"),           &ComposeHelpAttackCooldown }
@@ -516 +967,4 @@ namespace
-//     the moment a page showed a derived number.)
+//     the moment a page showed a derived number.) ⭐ TASK-1594 (2026-09-29) added the rest the
+//     same way, still typing no digit: each building type's height limit on Cards.StackUpgrade,
+//     the discard fee on Cards.Discard, Rally's radius, bonus, duration and cooldown on
+//     Hero.Rally, and the melee reach, cone and cooldown on Hero.Attack.
@@ -716,0 +1171,11 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// ⭐ TASK-1594 (2026-09-29, 🧑 his "all of it"): the three numbers are now SHOWN, and still
+			// not typed. "within your melee reach", "inside a cone in front of you" and "once per melee
+			// cooldown" became the `{#Attack.Reach}`, `{#Attack.ConeHalfAngle}` and `{#Attack.Cooldown}`
+			// tokens, which ComposeDetailForDisplay replaces with MeleeRange in metres,
+			// MeleeHalfAngleDegrees in degrees and MeleeCooldown in seconds, each read through its
+			// TASK-1592 getter on the class default of the hero class the game spawns (the
+			// derived-number block near the top of this file says why that is the object). "to either
+			// side of where you face" is the cone test itself: the angle is measured on the horizontal
+			// plane from the hero's facing (DoMeleeAttack's `Facing` and `MinCosAngle`). At today's
+			// values it renders "within 1.5 metres of you", "30 degrees" and "0.5 seconds". Pinned by
+			// test 22 (DiscardRallyAndAttackNumbersAreReadFromTheirOwners).
@@ -718,2 +1183,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("One swing damages every enemy unit, hero, building and castle within your melee reach and inside a ")
-				TEXT("cone in front of you, and you can swing at most once per melee cooldown. ")
+				TEXT("One swing damages every enemy unit, hero, building and castle within {#Attack.Reach} of you and inside a ")
+				TEXT("cone reaching {#Attack.ConeHalfAngle} to either side of where you face, and you can swing at most once ")
+				TEXT("every {#Attack.Cooldown}. ")
@@ -738,2 +1204,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Speeds up every friendly summoned unit within the rally radius by the rally speed bonus for ")
-				TEXT("the rally duration — units only, never the hero, never enemy units. Friendly miners are ")
+				TEXT("Speeds up every friendly summoned unit within {#Rally.Radius} of your hero by {#Rally.SpeedBonus} for ")
+				TEXT("{#Rally.Duration} — units only, never the hero, never enemy units. Friendly miners are ")
@@ -741,3 +1207,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("On cooldown the press does nothing, but it still tells the HUD how much cooldown is left ")
-				TEXT("so the HUD can flash it; when the rally cooldown runs out, the HUD is told ")
-				TEXT("that Rally is ready again. A dead hero cannot rally.")));
+				TEXT("After each rally there is a cooldown of {#Rally.Cooldown}. On cooldown the press does nothing, but it ")
+				TEXT("still tells the HUD how much cooldown is left so the HUD can flash it; when the cooldown runs out, ")
+				TEXT("the HUD is told that Rally is ready again. A dead hero cannot rally.")));
@@ -747,0 +1214,13 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// ⭐ TASK-1594 (2026-09-29, 🧑 his "all of it"): the four numbers are now SHOWN, and still not
+			// typed. "within the rally radius", "by the rally speed bonus", "for the rally duration" and
+			// "when the rally cooldown runs out" became the `{#Rally.Radius}`, `{#Rally.SpeedBonus}`,
+			// `{#Rally.Duration}` and `{#Rally.Cooldown}` tokens, which ComposeDetailForDisplay
+			// replaces with RallyRadius in metres, RallySpeedBonus as a percent, and RallyDuration and
+			// RallyCooldown in seconds, each read through its TASK-1592 getter on the class default of
+			// the hero class the game spawns. "of your hero" is the range test itself: AHeroCharacter::
+			// Rally measures from the hero's own location (`MyLocation`) to each unit's. "After each
+			// rally there is a cooldown of" is the same cooldown the old sentence named: the rally sets
+			// `LastRallyTime = Now` and a press is refused while `SinceLastRally < RallyCooldown`; the
+			// sentence that followed now says "when the cooldown runs out" so the word is not repeated.
+			// At today's values it renders "within 6 metres of your hero by 25% for 5 seconds" and "a
+			// cooldown of 20 seconds". Pinned by test 22 (DiscardRallyAndAttackNumbersAreReadFromTheirOwners).
@@ -907,0 +1387,10 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// ⭐ TASK-1594 (2026-09-29, 🧑 his "all of it"): the fee is now SHOWN, and still not typed.
+			// "a single set amount" became the `{#Discard.Fee}` token, which ComposeDetailForDisplay
+			// replaces with GetDiscardAllCost() (TASK-1592) read off the controller's class default,
+			// the fee DiscardEntireHand charges (the derived-number block near the top of this file
+			// says why that is the object). That is DiscardAllCost's own header rule for this row: "IF
+			// IT IS EVER SHOWN TO THE PLAYER IT IS READ FROM HERE, NEVER TYPED". ⇒ the strings below still
+			// carry ⛔ not one digit character, and test 14 (e)'s digit check now reads that TYPED
+			// template (it read the composed page until TASK-1594); test 22 pins the shown fee against
+			// its owner. The pinned "charged once for the whole hand" sentence is untouched. At today's
+			// value it renders "The fee is 20 gold".
@@ -912 +1401 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The fee is a single set amount and it is charged once for the whole hand, flat. Dumping a ")
+				TEXT("The fee is {#Discard.Fee} and it is charged once for the whole hand, flat. Dumping a ")
@@ -1054,13 +1543,19 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// fail (STACK-§2), and ⛔ no building is named here. ⭐ TASK-1576 (2026-09-28): 🧑 Jonathan
-			// answered Q-STACK-CAP-2026-09-28 with A, "show the numbers" (TASK-1589), so the page is
-			// to show each building type's own height limit. ⛔ It does NOT show them yet, and the
-			// reason is access, not choice: each value must be read from the class the game really
-			// places for that card, found by the game's own card-to-class resolution, and that
-			// resolution (ASiegePlayerController::ResolveCardActorClass with IsBuildingCard, plus the
-			// BuildingEconomyCardIDs list it reads) is private to the controller. Reading it from here
-			// needs a new accessor in an owner file, which TASK-1576's fence forbids (its spec (3) and
-			// (8)), and a copy of the path rule here would be a second resolver that could drift. ⇒ the
-			// per-building limits are OWED on his answer A (handoffs/TASK-1576-programmer.md), and
-			// ⛔ this row still states no per-building limit. (Until TASK-1576 this sentence read
-			// "Whether the page shows each building's height limit as a number is Jonathan's open
-			// question Q-STACK-CAP-2026-09-28, owned by TASK-1576".)
+			// fail (STACK-§2), and ⛔ no building is named HERE, in code: the page names each type
+			// from its card's DisplayName, read at compose time (below). ⭐ TASK-1576 (2026-09-28): 🧑
+			// Jonathan answered Q-STACK-CAP-2026-09-28 with A, "show the numbers" (TASK-1589), so the
+			// page shows each building type's own height limit. ⭐ TASK-1594 (2026-09-29): IT DOES NOW.
+			// The `{#StackUpgrade.HeightLimits}` token is replaced by one clause that names every
+			// placeable building type under the value read off ITS OWN class default through
+			// GetMaxStackHeightMultiplier(), types with equal values grouped (the derived-number block
+			// near the top of this file). The set comes from the game's own card-to-class resolution,
+			// read through the controller's public members: GetCardTableAsset(), IsBuildingCard and
+			// ResolveCardActorClass (public since TASK-1592). BuildingEconomyCardIDs and CardTableAsset
+			// themselves are still protected; the page never reads either directly (IsBuildingCard
+			// reads the list, GetCardTableAsset() returns the pointer). No path rule is copied here.
+			// (Until TASK-1594 this note said the page did NOT show them yet because "that resolution
+			// (ASiegePlayerController::ResolveCardActorClass with IsBuildingCard, plus the
+			// BuildingEconomyCardIDs list it reads) is private to the controller", and that the limits
+			// were OWED on his answer A. That was true until TASK-1592, except that the list and the
+			// table pointer were protected rather than private, qa/TASK-1577.md N2. Before TASK-1576
+			// the sentence read "Whether the page shows each building's height limit as a number is
+			// Jonathan's open question Q-STACK-CAP-2026-09-28, owned by TASK-1576".)
@@ -1081,4 +1576,20 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// today's value. The height half keeps "a set maximum multiple" (the per-building limits
-			// are owed, above). Pinned by test 20 (ShownNumbersAreReadFromTheirOwners).
-			FSiegeControlsHelpAction& Row = AddRow(TEXT("Cards.StackUpgrade"), CategoryCards, TEXT("Stack a tower taller"),
-				TEXT("While you are placing a building, hover one you already own of the same card: the outline turns blue and the click makes that one taller instead of building a new one."), ESiegeInputLane::RawNonLetter);
+			// today's value. Pinned by test 20 (ShownNumbersAreReadFromTheirOwners).
+			// ⭐ TASK-1594 (2026-09-29): the height half is now SHOWN per building type too. "and it
+			// stops at a set maximum multiple of that original" became "up to its height limit:" + the
+			// `{#StackUpgrade.HeightLimits}` token, which renders "5 times that original height for the
+			// Arrow Tower, Wall, Bomb Tower, Ballista Tower, Barracks, Deep Mine and Crystal Tower, and 2
+			// times for the Watch Tower" at today's reads. Each limit is stated positively, as how far
+			// the building stacks: ⛔ no "cannot be stacked" (test 19; qa/TASK-1586.md ruling 8), ⛔ no
+			// reason for any limit and ⛔ no "balance" / "design" wording (STACK-§10 cl. 2). "Its width
+			// and length" became "The building's width and length", because after the new list "Its"
+			// would read as the last building named; the claim is unchanged. (Until TASK-1594 this note
+			// said "The height half keeps "a set maximum multiple" (the per-building limits are owed,
+			// above)".) Pinned by test 21 (EachBuildingTypeShowsItsOwnHeightLimit).
+			// ⭐ TASK-1594 (2026-09-29, qa/TASK-1586.md N2): the headline read "Stack a tower taller"
+			// and the one-liner "the click makes that one taller". Every building type stacks (not
+			// only towers), and a click on a building already at its height limit buys health only, so
+			// "taller" was false for that click. Both are building-neutral now: "Upgrade one of your
+			// buildings" and "the click upgrades that one". Test 16 needs only the three WHEEL rows'
+			// headlines distinct, and this row is not one of them.
+			FSiegeControlsHelpAction& Row = AddRow(TEXT("Cards.StackUpgrade"), CategoryCards, TEXT("Upgrade one of your buildings"),
+				TEXT("While you are placing a building, hover one you already own of the same card: the outline turns blue and the click upgrades that one instead of building a new one."), ESiegeInputLane::RawNonLetter);
@@ -1096 +1607 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("height, and it stops at a set maximum multiple of that original. Its width and length are ")
+				TEXT("height, up to its height limit: {#StackUpgrade.HeightLimits}. The building's width and length are ")
@@ -1128 +1639,5 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// ASiegePlayerController; see the TASK-1576 note in the citation block above). The sentence after
+			// ASiegePlayerController; see the TASK-1576 note in the citation block above; ⭐ TASK-1594,
+			// 2026-09-29: no longer owed and no longer private, the limits are SHOWN, each type named
+			// under its own read value, in "WHAT AN UPGRADE BUYS" above, not in this paragraph's place,
+			// and ⛔ still no building is singled out: see the TASK-1594 notes in the citation block
+			// above). The sentence after
@@ -1949,0 +2465,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// ⭐ TASK-1594 (2026-09-29): "Two pages" is now five (Cards.StackUpgrade, Cards.Discard,
+			// Hero.Rally, Hero.Attack and Interface.MapMarks), each still read from its owner through
+			// a `{#Name}` token; this quoted 30 gold is still the one quantity typed, and the reveal fee
+			// was not a TASK-1594 candidate either.
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h
index 0501e69..a630d69 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h
@@ -175 +175 @@ struct FSiegeControlsHelpAction
-	 *  step", "once per melee cooldown", "a fixed reveal fee") and the code name sits in the
+	 *  step", "a fixed reveal fee") or shows its value derived, and the code name sits in the
@@ -179 +179 @@ struct FSiegeControlsHelpAction
-	 *  (CommanderNpc.h:297-311); every other number a page shows is derived at runtime from the
+	 *  (CommanderNpc.h:297-311); every other quantity a page shows is derived at runtime from the
@@ -181 +181 @@ struct FSiegeControlsHelpAction
-	 *  (TASK-1576, 2026-09-28).
+	 *  (TASK-1576, 2026-09-28; TASK-1594, 2026-09-29: "number" became "quantity" here).
@@ -366 +366 @@ struct GITCLAUDEUNREALTEST_API FSiegeControlsHelpRegistry
-	/** The detail prose, or the same pinned TODO string. The detail renderer reads THIS, ⛔ never Row.Detail directly. */
+	/** The detail prose with every `{#Name}` number token replaced by the number read from its owner at this call (the .cpp's derived-number block; TASK-1576, named here by TASK-1594), or the same pinned TODO string. The detail renderer reads THIS, ⛔ never Row.Detail directly. */
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
index 4acd21f..1f23900 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
@@ -4,0 +5,4 @@
+// TASK-1594 tests 21 and 22: the owners of the numbers his answer A and "all of it" added, read here
+// through the SAME public members the composer reads (TASK-1592's accessors on the controller, the
+// game mode and the hero), and the card table they walk.
+#include "Engine/DataTable.h"
@@ -6,0 +11,4 @@
+#include "Siegebound/CardRow.h"
+#include "Siegebound/HeroCharacter.h"
+#include "Siegebound/SiegeGameMode.h"
+#include "Siegebound/SiegePlayerController.h"
@@ -23,0 +32,7 @@
+#if WITH_EDITOR
+// TASK-1594 tests 21 and 22: each resolved Blueprint class's generating Blueprint and its status,
+// for the AddInfo lines TASK-1596 reads (UBlueprint::GetBlueprintFromClass / UBlueprint::Status,
+// both declared under WITH_EDITORONLY_DATA, which an editor target always builds with).
+#include "Engine/Blueprint.h"
+#endif
+
@@ -32,0 +48,9 @@
+ *  ⭐⭐ TESTS 21 AND 22 = TASK-1594 (2026-09-29), 🧑 his answer A and his "all of it": test 21
+ *  checks that Cards.StackUpgrade names every placeable building type exactly once, under the
+ *  height limit read from THAT class's own default; test 22 checks that the discard fee and the
+ *  Rally and Attack numbers are read from their owners, and that the Rally and Attack templates
+ *  type no digit. Both read their owners through the same public members the composer uses, and
+ *  both load the building and hero classes in the SUITE's process (never the GUI editor), logging
+ *  each class's value and Blueprint status. TASK-1594 also narrowed test 14 (e)'s detail digit
+ *  check to the TYPED template and added test 20's plural pin (qa/TASK-1577.md N3).
+ *
@@ -1839 +1863,5 @@ bool FSiegeControlsHelpDiscardAllLayoutTest::RunTest(const FString& Parameters)
-	TestTrue(TEXT("⭐ The page states the flat-fee rule in words instead of restating its value"),
+	// ⭐ TASK-1594 (2026-09-29): the page now SHOWS the fee beside this rule, read from its owner
+	// when the page is composed ("The fee is <GetDiscardAllCost()> gold and it is charged once for
+	// the whole hand"). The rule's pinned words are unchanged; only this label moved, because it
+	// said "instead of restating its value", and the value is now on the page (test 22 pins it).
+	TestTrue(TEXT("⭐ The page states the flat-fee rule in words: the fee is charged once for the whole hand"),
@@ -1870,2 +1898,6 @@ bool FSiegeControlsHelpDiscardAllLayoutTest::RunTest(const FString& Parameters)
-	TestFalse(TEXT("⛔ ...and neither does its detail page - the fee is read from DiscardAllCost, never typed"),
-		CarriesADigit(DiscardDetail));
+	// ⭐ NARROWED BY TASK-1594 (2026-09-29) to the TYPED template, as TASK-1576 narrowed test 15 (e):
+	// it read `CarriesADigit(DiscardDetail)`, the COMPOSED page, which now legitimately carries the
+	// fee read from GetDiscardAllCost(). A typed digit in the template still fails it; that the shown
+	// fee is the owner's is test 22's claim, not this one's.
+	TestFalse(TEXT("⛔ ...and neither does its detail TEMPLATE - the fee the page shows is read from DiscardAllCost when the page is composed, never typed"),
+		CarriesADigit(DiscardRow->Detail.ToString()));
@@ -2840,0 +2873,5 @@ bool FSiegeControlsHelpRefutedRulesTest::RunTest(const FString& Parameters)
+ *  (e)  ⭐ TASK-1594 (2026-09-29, qa/TASK-1577.md N3): the map-circle noun's PLURAL. (a) stops at
+ *       " circle" so it also holds at a cap of one, which left a swapped plural ("9 circle") green.
+ *       When the owner's cap is not one, the page must say "circles at once". The pin is
+ *       value-free on purpose, so arm 1576-D1 (a wrong VALUE) still reddens (a) and (a2) only.
+ *
@@ -2842,4 +2879,6 @@ bool FSiegeControlsHelpRefutedRulesTest::RunTest(const FString& Parameters)
- *  that the pages still fit the panel is 5b's to measure and Jonathan's to judge. ⛔ And the rows'
- *  other candidates (the discard fee, the Rally values, the melee numbers and each building's own
- *  height limit) are ⛔ NOT shown yet, so nothing here asserts them; handoffs/TASK-1576-programmer.md
- *  lists why (each needs a new accessor in its owner's file).
+ *  that the pages still fit the panel is 5b's to measure and Jonathan's to judge. ⭐ The rows'
+ *  other candidates are shown since TASK-1594 (2026-09-29) and are pinned by their own tests:
+ *  each building type's height limit by test 21, and the discard fee, the Rally values and the
+ *  melee numbers by test 22. (Until TASK-1594 this paragraph said they were "NOT shown yet, so
+ *  nothing here asserts them", each needing a new accessor in its owner's file, which TASK-1592
+ *  then added.)
@@ -2977,0 +3017,629 @@ bool FSiegeControlsHelpDerivedNumbersTest::RunTest(const FString& Parameters)
+	// ── (e) ⭐ TASK-1594 (qa/TASK-1577.md N3): THE MAP-CIRCLE NOUN IS PLURAL WHEN THE CAP IS NOT ONE ──
+	// (a) stops at " circle" on purpose (a prefix of both forms), so a plural pattern with its two
+	// forms swapped rendered "9 circle at once" and passed. ⛔ VALUE-FREE: it checks the noun's form,
+	// not the number before it, so arm 1576-D1 (a wrong value, "250 circles") does not reach it and
+	// the new arm N3 (the forms swapped) reaches nothing else.
+	if (OwnerMapMarkCap != 1)
+	{
+		const FSiegeControlsHelpAction* const PluralMarksRow = FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Interface.MapMarks")));
+		if (TestNotNull(TEXT("Row 'Interface.MapMarks' is in the registry, so the plural pin can be read"), PluralMarksRow))
+		{
+			TestTrue(TEXT("Row 'Interface.MapMarks' names the circles in the plural when the owner's cap is not one: ' circles at once'"),
+				FSiegeControlsHelpRegistry::ComposeDetailForDisplay(*PluralMarksRow).ToString().Contains(TEXT(" circles at once"), ESearchCase::CaseSensitive));
+		}
+	}
+	else
+	{
+		AddInfo(TEXT("The owner's map-circle cap is one today, so the plural pin (e) is not exercised; (a) still pins the value."));
+	}
+
+	return true;
+}
+
+// ════════════════════════════════════════════════════════════════════════════════════════
+//  TESTS 21-22 — THE NUMBERS TASK-1594 ADDED (🧑 his answer A and his "all of it")
+//
+//  ⛔ `SC-§37`, as in test 20: every claim is made against the OWNER'S VALUE, read HERE through
+//  the same public member the composer reads and formatted HERE with the composer's fixed
+//  options, ⛔ never against a typed digit and ⛔ never against a string the composer supplies.
+//  ⚠️ `VER-§12` cl. 7g: both tests load classes (the card table, the eight building Blueprints,
+//  BP_HeroCharacter) in the SUITE's process, which is where this wave loads them before 5b;
+//  ⛔ never in the GUI editor. Each class, its value and its Blueprint status are logged with
+//  AddInfo under the prefix "[ControlsHelp]", and those lines are TASK-1596's Blueprint-override
+//  reading (TASK-1576 (2)).
+// ════════════════════════════════════════════════════════════════════════════════════════
+
+namespace SiegeControlsHelpNumbersTestUtils
+{
+	/** Every start index of Needle in Haystack, left to right, case-sensitive. */
+	static TArray<int32> FindEveryOccurrence(const FString& Haystack, const FString& Needle)
+	{
+		TArray<int32> Positions;
+		if (Needle.IsEmpty())
+		{
+			return Positions;
+		}
+
+		int32 SearchFrom = 0;
+		while (SearchFrom < Haystack.Len())
+		{
+			const int32 FoundAt = Haystack.Find(*Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
+			if (FoundAt == INDEX_NONE)
+			{
+				break;
+			}
+			Positions.Add(FoundAt);
+			SearchFrom = FoundAt + 1;
+		}
+		return Positions;
+	}
+
+	/**
+	 *  Every place Name stands WHOLE in a list: after a space, and followed by a comma, a space or
+	 *  the end. The left boundary is what stops a name matching inside a longer one that ends
+	 *  with it.
+	 */
+	static TArray<int32> FindWholeNameOccurrences(const FString& Haystack, const FString& Name)
+	{
+		TArray<int32> WholeAt;
+		for (const int32 FoundAt : FindEveryOccurrence(Haystack, Name))
+		{
+			const int32 AfterAt = FoundAt + Name.Len();
+			const bool bStartsWhole = FoundAt > 0 && Haystack[FoundAt - 1] == TEXT(' ');
+			const bool bEndsWhole = AfterAt == Haystack.Len() || Haystack[AfterAt] == TEXT(',') || Haystack[AfterAt] == TEXT(' ');
+			if (bStartsWhole && bEndsWhole)
+			{
+				WholeAt.Add(FoundAt);
+			}
+		}
+		return WholeAt;
+	}
+
+	/**
+	 *  The height-limit clause of a composed Cards.StackUpgrade page: the text after "up to its
+	 *  height limit: " and before the next ". ". Empty when the lead is absent.
+	 */
+	static FString ExtractHeightLimitClause(const FString& Page)
+	{
+		const FString Lead(TEXT("up to its height limit: "));
+		const int32 LeadAt = Page.Find(*Lead, ESearchCase::CaseSensitive);
+		if (LeadAt == INDEX_NONE)
+		{
+			return FString();
+		}
+
+		const int32 ClauseStart = LeadAt + Lead.Len();
+		const int32 ClauseEnd = Page.Find(TEXT(". "), ESearchCase::CaseSensitive, ESearchDir::FromStart, ClauseStart);
+		return ClauseEnd == INDEX_NONE ? Page.Mid(ClauseStart) : Page.Mid(ClauseStart, ClauseEnd - ClauseStart);
+	}
+
+	/** The nearest native class at or above Class: a Blueprint class's native parent, or the class itself. */
+	static const UClass* FindNativeAncestor(const UClass* Class)
+	{
+		const UClass* Walk = Class;
+		while (Walk != nullptr && !Walk->HasAnyClassFlags(CLASS_Native))
+		{
+			Walk = Walk->GetSuperClass();
+		}
+		return Walk;
+	}
+
+#if WITH_EDITOR
+	/**
+	 *  "<status> (Blueprint '<path>')" for a Blueprint-generated class, or "native (no generating
+	 *  Blueprint)". The engine calls: UBlueprint::GetBlueprintFromClass, then the transient
+	 *  UBlueprint::Status (Engine/Blueprint.h), spelled from the engine's own enumerators. A read
+	 *  only: ⛔ nothing here asserts on it (TASK-1592's test 2 owns the BS_Error assertion).
+	 */
+	static FString DescribeGeneratingBlueprint(const UClass* Class)
+	{
+		const UBlueprint* const Generator = UBlueprint::GetBlueprintFromClass(Class);
+		if (Generator == nullptr)
+		{
+			return FString(TEXT("native (no generating Blueprint)"));
+		}
+
+		const TCHAR* StatusName = TEXT("BS_(unlisted)");
+		switch (Generator->Status.GetValue())
+		{
+		case BS_Unknown:              StatusName = TEXT("BS_Unknown");              break;
+		case BS_Dirty:                StatusName = TEXT("BS_Dirty");                break;
+		case BS_Error:                StatusName = TEXT("BS_Error");                break;
+		case BS_UpToDate:             StatusName = TEXT("BS_UpToDate");             break;
+		case BS_BeingCreated:         StatusName = TEXT("BS_BeingCreated");         break;
+		case BS_UpToDateWithWarnings: StatusName = TEXT("BS_UpToDateWithWarnings"); break;
+		default:                                                                   break;
+		}
+		return FString::Printf(TEXT("%s (Blueprint '%s')"), StatusName, *Generator->GetPathName());
+	}
+#endif
+}
+
+// ════════════════════════════════════════════════════════════════════════════════════════
+//  TEST 21 — Siegebound.ControlsHelp.EachBuildingTypeShowsItsOwnHeightLimit   ⭐⭐
+// ════════════════════════════════════════════════════════════════════════════════════════
+
+/**
+ *  ⭐⭐ TASK-1594 (2026-09-29): 🧑 his answer A to Q-STACK-CAP-2026-09-28, "show the numbers".
+ *  Cards.StackUpgrade names every building type the player can place, each under the height limit
+ *  read from THAT class's own default (ABuilding::GetMaxStackHeightMultiplier), types with equal
+ *  values grouped. The set is enumerated HERE through the game's own card → class resolution on
+ *  the controller's class default (GetCardTableAsset(), IsBuildingCard, then ResolveCardActorClass,
+ *  public since TASK-1592), the same calls the composer makes and the game places through.
+ *
+ *  (a-cap)  per building type, the limit its name sits under is its OWN class default's value: the
+ *           nearest "<N> time" group head before its name, in the clause after "up to its height
+ *           limit: ". ⛔ Not merely its digit somewhere on the page.
+ *  (a2-cap) every rendering of the row (its own page and each related block that renders it,
+ *           found by walking every row) carries the same clause as its own page.
+ *  (c-cap)  completeness: every enumerated type has a player-facing name (FCardRow::DisplayName)
+ *           and is named EXACTLY ONCE in the clause. A type the composer dropped is named zero
+ *           times, which is why the composer names every type and never writes "every other".
+ *  (d-cap)  discrimination control: the reads hold at least two distinct values. ⛔ If they do not
+ *           (a retune, or a Blueprint override that makes every limit equal), a composer reading
+ *           one class for all would look right, (a-cap) could not tell, and arm cap-i could not go
+ *           red. The failure message says so.
+ *
+ *  ⚠️ WHAT IT CANNOT PROVE (`SC-§32`): nothing here paints a page; whether the list reads well
+ *  and still fits the panel is 5b's to measure and 🧑 his to judge.
+ */
+IMPLEMENT_SIMPLE_AUTOMATION_TEST(
+	FSiegeControlsHelpHeightLimitsTest,
+	"Siegebound.ControlsHelp.EachBuildingTypeShowsItsOwnHeightLimit",
+	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
+
+bool FSiegeControlsHelpHeightLimitsTest::RunTest(const FString& Parameters)
+{
+	using namespace SiegeControlsHelpNumbersTestUtils;
+
+	// ── THE OWNERS, READ HERE THROUGH THE SAME PUBLIC SURFACE THE COMPOSER USES ────────────
+	const ASiegePlayerController* const PlacementRulesDefaults = GetDefault<ASiegePlayerController>();
+	if (!TestNotNull(TEXT("SELF-CHECK: the ASiegePlayerController class default resolves, so the game's card → class resolution can be asked"), PlacementRulesDefaults))
+	{
+		return false;
+	}
+
+	const TSoftObjectPtr<UDataTable>& PlacementCardTableAsset = PlacementRulesDefaults->GetCardTableAsset();
+	const UDataTable* const PlacementCardTable = PlacementCardTableAsset.LoadSynchronous();
+	if (PlacementCardTable == nullptr || PlacementCardTable->GetRowStruct() != FCardRow::StaticStruct())
+	{
+		AddError(FString::Printf(TEXT("⛔ The card table GetCardTableAsset() names ('%s') did not load as an FCardRow table, so no building type can be enumerated and every claim below would be vacuous."),
+			*PlacementCardTableAsset.ToString()));
+		return false;
+	}
+
+	struct FReadBuildingType
+	{
+		FString CardId;
+		FString DisplayName;
+		int32   HeightLimit = 0;
+	};
+
+	TArray<FReadBuildingType> ReadBuildingTypes;
+	TArray<int32> DistinctHeightLimits;
+
+	for (const FName& PlacementCardID : PlacementCardTable->GetRowNames())
+	{
+		const FCardRow* const PlacementCardRow =
+			PlacementCardTable->FindRow<FCardRow>(PlacementCardID, TEXT("SiegeControlsHelpTest height limits"), /*bWarnIfRowMissing=*/ false);
+
+		// ⛔ qa/TASK-1593.md W1: ResolveCardActorClass only for a card IsBuildingCard accepts, as the
+		// composer does, so its "not a placement type" Error can never fire from this test.
+		if (PlacementCardRow == nullptr || !PlacementRulesDefaults->IsBuildingCard(PlacementCardID, PlacementCardRow->CardType))
+		{
+			continue;
+		}
+
+		UClass* const PlacedClass = PlacementRulesDefaults->ResolveCardActorClass(PlacementCardID, PlacementCardRow->CardType);
+		const ABuilding* const PlacedClassDefaults = PlacedClass != nullptr ? Cast<ABuilding>(PlacedClass->GetDefaultObject()) : nullptr;
+		if (PlacedClassDefaults == nullptr)
+		{
+			// The game places nothing for a card whose class does not resolve (the play is refused
+			// with no gold spent), and the composer skips it for the same reason. Whether every
+			// building card resolves is TASK-1592's test 2's claim and the card roster's.
+			AddInfo(FString::Printf(TEXT("[ControlsHelp] height limit: building card '%s' did not resolve to an ABuilding class, so the game cannot place it and the page does not list it."),
+				*PlacementCardID.ToString()));
+			continue;
+		}
+
+		FReadBuildingType& ReadType = ReadBuildingTypes.AddDefaulted_GetRef();
+		ReadType.CardId      = PlacementCardID.ToString();
+		ReadType.DisplayName = PlacementCardRow->DisplayName;
+		ReadType.HeightLimit = PlacedClassDefaults->GetMaxStackHeightMultiplier();
+		DistinctHeightLimits.AddUnique(ReadType.HeightLimit);
+
+		// ⭐ THE BLUEPRINT-OVERRIDE READING (TASK-1576 (2), carried by TASK-1594 (5)): the value on
+		// this class's own default beside the value on its nearest NATIVE ancestor's default. A
+		// difference is a Blueprint override the page must follow; equal means none.
+		const UClass* const NativeAncestor = FindNativeAncestor(PlacedClass);
+		const ABuilding* const NativeAncestorDefaults =
+			NativeAncestor != nullptr ? Cast<ABuilding>(NativeAncestor->GetDefaultObject()) : nullptr;
+		const int32 NativeHeightLimit = NativeAncestorDefaults != nullptr ? NativeAncestorDefaults->GetMaxStackHeightMultiplier() : INDEX_NONE;
+#if WITH_EDITOR
+		const FString GeneratorText = DescribeGeneratingBlueprint(PlacedClass);
+#else
+		const FString GeneratorText(TEXT("(status not read: not an editor build)"));
+#endif
+		AddInfo(FString::Printf(TEXT("[ControlsHelp] height limit: card '%s' '%s' -> class '%s', GetMaxStackHeightMultiplier() = %d on its own class default; native parent '%s' = %d (%s); status %s"),
+			*ReadType.CardId, *ReadType.DisplayName, *PlacedClass->GetPathName(), ReadType.HeightLimit,
+			*GetNameSafe(NativeAncestor), NativeHeightLimit,
+			NativeHeightLimit == ReadType.HeightLimit ? TEXT("no Blueprint override") : TEXT("the Blueprint OVERRIDES the native value"),
+			*GeneratorText));
+	}
+
+	// ⛔ THE VACUITY GUARD: an enumeration that found nothing would make every claim below true.
+	if (!TestTrue(TEXT("At least one placeable building type was enumerated through the game's own resolution, so (a-cap) and (c-cap) are measurements"),
+		ReadBuildingTypes.Num() > 0))
+	{
+		return false;
+	}
+
+	// ── (d-cap) THE DISCRIMINATION CONTROL ───────────────────────────────────────────────
+	TestTrue(*FString::Printf(TEXT("(d-cap) The height limits read hold at least two distinct values (%d distinct among %d building types). If this is red, a retune or a Blueprint override has made every limit equal: a composer that read ONE class for all would then look right, (a-cap) could not tell, and arm cap-i could not go red."),
+		DistinctHeightLimits.Num(), ReadBuildingTypes.Num()), DistinctHeightLimits.Num() >= 2);
+
+	// ── THE PAGE ─────────────────────────────────────────────────────────────────────────
+	const FSiegeControlsHelpAction* const StackRow = FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Cards.StackUpgrade")));
+	if (!TestNotNull(TEXT("Row 'Cards.StackUpgrade' is in the registry"), StackRow))
+	{
+		return false;
+	}
+
+	const FString OwnPage = FSiegeControlsHelpRegistry::ComposeDetailForDisplay(*StackRow).ToString();
+	const FString OwnClause = ExtractHeightLimitClause(OwnPage);
+	if (!TestFalse(TEXT("Row 'Cards.StackUpgrade' carries a height-limit clause after 'up to its height limit: '"), OwnClause.IsEmpty()))
+	{
+		return false;
+	}
+
+	// A leading space, so every name and every group head in the clause is preceded by one.
+	const FString SpacedClause = FString(TEXT(" ")) + OwnClause;
+
+	// FORMATTED HERE, TO THE COMPOSER'S FIXED FORMAT: a whole multiple with no fractional digits.
+	// ⛔ Written out here rather than asked of the composer. " <N> time" is a prefix of both "time"
+	// and "times", with the space that separates it from the list before it.
+	FNumberFormattingOptions MultipleFormat;
+	MultipleFormat.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(0);
+	auto GroupHeadFor = [&MultipleFormat](int32 HeightLimit) -> FString
+	{
+		return FString(TEXT(" ")) + FText::AsNumber(HeightLimit, &MultipleFormat).ToString() + FString(TEXT(" time"));
+	};
+
+	for (const FReadBuildingType& ReadType : ReadBuildingTypes)
+	{
+		// ── (c-cap) THE NAME IS PLAYER-FACING DATA, AND THE TYPE IS NAMED EXACTLY ONCE ──────
+		if (!TestFalse(*FString::Printf(TEXT("(c-cap) Building card '%s' has a player-facing name (FCardRow::DisplayName), so the page can name it"), *ReadType.CardId),
+			ReadType.DisplayName.IsEmpty()))
+		{
+			continue;
+		}
+
+		const TArray<int32> NamedAt = FindWholeNameOccurrences(SpacedClause, ReadType.DisplayName);
+		TestEqual(*FString::Printf(TEXT("(c-cap) Building type '%s' (card '%s') is named exactly once in Cards.StackUpgrade's height-limit clause"), *ReadType.DisplayName, *ReadType.CardId),
+			NamedAt.Num(), 1);
+
+		// (a-cap) needs exactly one place to look. A missing or repeated name is (c-cap)'s failure,
+		// ⛔ not a second one here.
+		if (NamedAt.Num() != 1)
+		{
+			continue;
+		}
+
+		// ── (a-cap) ITS OWN LIMIT: THE NEAREST GROUP HEAD BEFORE ITS NAME ────────────────────
+		int32 ShownHeightLimit = INDEX_NONE;
+		int32 NearestHeadAt = INDEX_NONE;
+		for (const int32 CandidateHeightLimit : DistinctHeightLimits)
+		{
+			for (const int32 HeadAt : FindEveryOccurrence(SpacedClause, GroupHeadFor(CandidateHeightLimit)))
+			{
+				if (HeadAt < NamedAt[0] && HeadAt > NearestHeadAt)
+				{
+					NearestHeadAt = HeadAt;
+					ShownHeightLimit = CandidateHeightLimit;
+				}
+			}
+		}
+
+		TestEqual(*FString::Printf(TEXT("(a-cap) Building type '%s' (card '%s') is listed under the height limit its OWN class default holds (the nearest '<N> time' before its name)"), *ReadType.DisplayName, *ReadType.CardId),
+			ShownHeightLimit, ReadType.HeightLimit);
+	}
+
+	// ── (a2-cap) EVERY RENDERING CARRIES THE SAME CLAUSE ─────────────────────────────────
+	// ⛔ Found by walking every row's composed page, never from a typed list of pages.
+	auto NoAppliedKeys = [](const FSiegeControlsHelpAction&) -> TArray<FKey> { return TArray<FKey>(); };
+	int32 Renderings = 0;
+	int32 RenderingsWithOwnClause = 0;
+	for (const FSiegeControlsHelpAction& PageRow : FSiegeControlsHelpRegistry::GetActions())
+	{
+		const FSiegeControlsDetailContent Page =
+			FSiegeControlsHelpRegistry::ComposeDetailContent(PageRow, nullptr, NoAppliedKeys);
+
+		if (PageRow.ActionId == StackRow->ActionId)
+		{
+			++Renderings;
+			RenderingsWithOwnClause += ExtractHeightLimitClause(Page.Body.ToString()) == OwnClause ? 1 : 0;
+		}
+
+		for (const FSiegeControlsDetailEntry& Entry : Page.Related)
+		{
+			if (Entry.ActionId == StackRow->ActionId)
+			{
+				++Renderings;
+				RenderingsWithOwnClause += ExtractHeightLimitClause(Entry.Body.ToString()) == OwnClause ? 1 : 0;
+			}
+		}
+	}
+
+	TestTrue(TEXT("Row 'Cards.StackUpgrade' is rendered somewhere, so (a2-cap) is a measurement"), Renderings > 0);
+	TestEqual(TEXT("(a2-cap) Every rendering of row 'Cards.StackUpgrade' carries the same height-limit clause as its own page"),
+		RenderingsWithOwnClause, Renderings);
+
+	AddInfo(FString::Printf(TEXT("[ControlsHelp] Cards.StackUpgrade height-limit clause as composed: '%s' (%d building type(s), %d distinct limit(s), %d of %d rendering(s) carry it)"),
+		*OwnClause, ReadBuildingTypes.Num(), DistinctHeightLimits.Num(), RenderingsWithOwnClause, Renderings));
+
+	return true;
+}
+
+// ════════════════════════════════════════════════════════════════════════════════════════
+//  TEST 22 — Siegebound.ControlsHelp.DiscardRallyAndAttackNumbersAreReadFromTheirOwners   ⭐⭐
+// ════════════════════════════════════════════════════════════════════════════════════════
+
+/**
+ *  ⭐⭐ TASK-1594 (2026-09-29): 🧑 his "yes, all of it". Three pages now SHOW numbers, and this
+ *  test asserts each one IS its owner's value, read HERE through the same public getter
+ *  (TASK-1592) on the same object the composer reads:
+ *    • Cards.Discard, the fee: GetDiscardAllCost() on ASiegePlayerController's class default (the
+ *      game's controller is that native class), in gold;
+ *    • Hero.Rally, radius / bonus / duration / cooldown, and Hero.Attack, reach / cone half-angle /
+ *      cooldown: the seven AHeroCharacter getters on the class default of the hero class the game
+ *      spawns (GetHeroPawnClassAsset() on ASiegeGameMode's class default, or the raw AHeroCharacter
+ *      when it resolves nothing, which is ResolveHeroPawnClass's own fallback), converted to player
+ *      units exactly as TASK-1576 (3) fixes them: uu ÷ 100 = metres, the bonus × 100 = percent,
+ *      seconds and degrees as they are.
+ *
+ *  (a)  per number, the row's own composed detail carries the owner's value INSIDE its own clause
+ *       ("fee is <N> gold", "within <N> metre", "of your hero by <N>%", " for <N> second",
+ *       "a cooldown of <N> second", "cone reaching <N> degree", "at most once every <N> second");
+ *  (a2) per number, every rendering of that row carries it (its own page and each related block,
+ *       found by walking every row);
+ *  (b)  per number, a NEGATIVE CONTROL: the same clause from a different value is absent;
+ *  (e)  the Rally and Attack TEMPLATES type no digit (Cards.Discard's is test 14 (e)'s, narrowed by
+ *       TASK-1594; Cards.StackUpgrade's is test 15 (e)'s). A typed number that happens to equal the
+ *       owner passes (a) by design, and only (e) sees it.
+ *
+ *  The hero class, its seven values beside the native AHeroCharacter's (the override reading), its
+ *  Blueprint status and the fee are logged with AddInfo. ⚠️ `SC-§32`: nothing here paints a page.
+ */
+IMPLEMENT_SIMPLE_AUTOMATION_TEST(
+	FSiegeControlsHelpFeeAndHeroNumbersTest,
+	"Siegebound.ControlsHelp.DiscardRallyAndAttackNumbersAreReadFromTheirOwners",
+	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
+
+bool FSiegeControlsHelpFeeAndHeroNumbersTest::RunTest(const FString& Parameters)
+{
+	using namespace SiegeControlsHelpNumbersTestUtils;
+
+	// ── THE OWNERS, READ FROM THE SAME OBJECTS THE COMPOSER READS ───────────────────────────
+	const ASiegePlayerController* const FeeOwnerDefaults = GetDefault<ASiegePlayerController>();
+	const ASiegeGameMode* const SpawningModeDefaults = GetDefault<ASiegeGameMode>();
+	if (!TestNotNull(TEXT("SELF-CHECK: the ASiegePlayerController class default resolves, so the discard fee can be read"), FeeOwnerDefaults)
+		|| !TestNotNull(TEXT("SELF-CHECK: the ASiegeGameMode class default resolves, so the hero class it spawns can be read"), SpawningModeDefaults))
+	{
+		return false;
+	}
+
+	// The hero class the game spawns: the loaded GetHeroPawnClassAsset(), or, when that resolves
+	// nothing, the raw AHeroCharacter (the two steps of ASiegeGameMode::ResolveHeroPawnClass).
+	const TSoftClassPtr<AHeroCharacter>& SpawnedHeroAsset = SpawningModeDefaults->GetHeroPawnClassAsset();
+	UClass* SpawnedHeroClass = SpawnedHeroAsset.LoadSynchronous();
+	const bool bSpawnedHeroFallback = SpawnedHeroClass == nullptr;
+	if (bSpawnedHeroFallback)
+	{
+		SpawnedHeroClass = AHeroCharacter::StaticClass();
+	}
+
+	const AHeroCharacter* const SpawnedHeroDefaults = Cast<AHeroCharacter>(SpawnedHeroClass->GetDefaultObject());
+	// ⚠️ The native default is the override reading's BASELINE only: ⛔ no expectation below is
+	// taken from it.
+	const AHeroCharacter* const NativeHeroDefaults = GetDefault<AHeroCharacter>();
+	if (!TestNotNull(TEXT("SELF-CHECK: the spawned hero class has an AHeroCharacter class default"), SpawnedHeroDefaults)
+		|| !TestNotNull(TEXT("SELF-CHECK: the native AHeroCharacter class default resolves (the override reading's baseline)"), NativeHeroDefaults))
+	{
+		return false;
+	}
+
+#if WITH_EDITOR
+	const FString SpawnedHeroGeneratorText = DescribeGeneratingBlueprint(SpawnedHeroClass);
+#else
+	const FString SpawnedHeroGeneratorText(TEXT("(status not read: not an editor build)"));
+#endif
+	AddInfo(FString::Printf(TEXT("[ControlsHelp] hero class '%s' -> '%s'%s, status %s"),
+		*SpawnedHeroAsset.ToString(), *SpawnedHeroClass->GetPathName(),
+		bSpawnedHeroFallback ? TEXT(" (the game's fallback: the soft class resolved nothing)") : TEXT(""),
+		*SpawnedHeroGeneratorText));
+
+	// ⭐ THE BLUEPRINT-OVERRIDE READING for the seven hero numbers (TASK-1576 (2)).
+	struct FHeroNumberRead
+	{
+		const TCHAR* GetterName;
+		float (AHeroCharacter::*Getter)() const;
+	};
+	const FHeroNumberRead HeroNumberReads[] =
+	{
+		{ TEXT("GetMeleeRange"),            &AHeroCharacter::GetMeleeRange },
+		{ TEXT("GetMeleeHalfAngleDegrees"), &AHeroCharacter::GetMeleeHalfAngleDegrees },
+		{ TEXT("GetMeleeCooldown"),         &AHeroCharacter::GetMeleeCooldown },
+		{ TEXT("GetRallyRadius"),           &AHeroCharacter::GetRallyRadius },
+		{ TEXT("GetRallySpeedBonus"),       &AHeroCharacter::GetRallySpeedBonus },
+		{ TEXT("GetRallyDuration"),         &AHeroCharacter::GetRallyDuration },
+		{ TEXT("GetRallyCooldown"),         &AHeroCharacter::GetRallyCooldown }
+	};
+	for (const FHeroNumberRead& HeroRead : HeroNumberReads)
+	{
+		const float SpawnedValue = (SpawnedHeroDefaults->*HeroRead.Getter)();
+		const float NativeValue  = (NativeHeroDefaults->*HeroRead.Getter)();
+		AddInfo(FString::Printf(TEXT("[ControlsHelp] hero %s() = %.6g on the spawned class's default; native AHeroCharacter = %.6g (%s)"),
+			HeroRead.GetterName, static_cast<double>(SpawnedValue), static_cast<double>(NativeValue),
+			SpawnedValue == NativeValue ? TEXT("no Blueprint override") : TEXT("the Blueprint OVERRIDES the native value")));
+	}
+
+	const int32 OwnerDiscardFee = FeeOwnerDefaults->GetDiscardAllCost();
+	AddInfo(FString::Printf(TEXT("[ControlsHelp] ASiegePlayerController::GetDiscardAllCost() = %d on the controller's class default (native; no Blueprint subclass)"),
+		OwnerDiscardFee));
+
+	// ── CONVERTED AND FORMATTED HERE, TO THE UNITS AND OPTIONS TASK-1576 (3) FIXES ─────────
+	// ⛔ Written out here rather than asked of the composer, so a composer that formats the WRONG
+	// value cannot also supply the expectation. Gold: whole. Metres and seconds: at most two
+	// fractional digits. Percent and degrees: at most one.
+	FNumberFormattingOptions WholeFormat;
+	WholeFormat.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(0);
+	FNumberFormattingOptions TwoPlaceFormat;
+	TwoPlaceFormat.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(2);
+	FNumberFormattingOptions OnePlaceFormat;
+	OnePlaceFormat.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(1);
+
+	auto FormatInPlayerUnits = [](float Value, const FNumberFormattingOptions& Format) -> FString
+	{
+		return FText::AsNumber(Value, &Format).ToString();
+	};
+
+	const float OwnerRallyRadiusMetres     = SpawnedHeroDefaults->GetRallyRadius() / 100.f;      // uu ÷ 100 = metres
+	const float OwnerRallyBonusPercent     = SpawnedHeroDefaults->GetRallySpeedBonus() * 100.f;  // fraction × 100 = percent
+	const float OwnerRallyDurationSeconds  = SpawnedHeroDefaults->GetRallyDuration();
+	const float OwnerRallyCooldownSeconds  = SpawnedHeroDefaults->GetRallyCooldown();
+	const float OwnerAttackReachMetres     = SpawnedHeroDefaults->GetMeleeRange() / 100.f;       // uu ÷ 100 = metres
+	const float OwnerAttackHalfAngle       = SpawnedHeroDefaults->GetMeleeHalfAngleDegrees();
+	const float OwnerAttackCooldownSeconds = SpawnedHeroDefaults->GetMeleeCooldown();
+
+	struct FShownHelpNumber
+	{
+		const TCHAR* ActionId;
+		const TCHAR* What;
+		FString      Clause;        // (a): the owner's value, inside its own clause
+		FString      WrongClause;   // (b): the same clause from a DIFFERENT value
+	};
+
+	const FShownHelpNumber ShownHelpNumbers[] =
+	{
+		{ TEXT("Cards.Discard"), TEXT("the discard fee (GetDiscardAllCost, controller class default, gold)"),
+			FString(TEXT("fee is ")) + FText::AsNumber(OwnerDiscardFee, &WholeFormat).ToString() + FString(TEXT(" gold")),
+			FString(TEXT("fee is ")) + FText::AsNumber(OwnerDiscardFee + 1, &WholeFormat).ToString() + FString(TEXT(" gold")) },
+		{ TEXT("Hero.Rally"), TEXT("the rally radius (GetRallyRadius, spawned hero class default, uu / 100 = metres)"),
+			FString(TEXT("within ")) + FormatInPlayerUnits(OwnerRallyRadiusMetres, TwoPlaceFormat) + FString(TEXT(" metre")),
+			FString(TEXT("within ")) + FormatInPlayerUnits(OwnerRallyRadiusMetres + 1.f, TwoPlaceFormat) + FString(TEXT(" metre")) },
+		{ TEXT("Hero.Rally"), TEXT("the rally speed bonus (GetRallySpeedBonus, spawned hero class default, x 100 = percent)"),
+			FString(TEXT("of your hero by ")) + FormatInPlayerUnits(OwnerRallyBonusPercent, OnePlaceFormat) + FString(TEXT("%")),
+			FString(TEXT("of your hero by ")) + FormatInPlayerUnits(OwnerRallyBonusPercent + 1.f, OnePlaceFormat) + FString(TEXT("%")) },
+		{ TEXT("Hero.Rally"), TEXT("the rally duration (GetRallyDuration, spawned hero class default, seconds)"),
+			FString(TEXT(" for ")) + FormatInPlayerUnits(OwnerRallyDurationSeconds, TwoPlaceFormat) + FString(TEXT(" second")),
+			FString(TEXT(" for ")) + FormatInPlayerUnits(OwnerRallyDurationSeconds + 1.f, TwoPlaceFormat) + FString(TEXT(" second")) },
+		{ TEXT("Hero.Rally"), TEXT("the rally cooldown (GetRallyCooldown, spawned hero class default, seconds)"),
+			FString(TEXT("a cooldown of ")) + FormatInPlayerUnits(OwnerRallyCooldownSeconds, TwoPlaceFormat) + FString(TEXT(" second")),
+			FString(TEXT("a cooldown of ")) + FormatInPlayerUnits(OwnerRallyCooldownSeconds + 1.f, TwoPlaceFormat) + FString(TEXT(" second")) },
+		{ TEXT("Hero.Attack"), TEXT("the melee reach (GetMeleeRange, spawned hero class default, uu / 100 = metres)"),
+			FString(TEXT("within ")) + FormatInPlayerUnits(OwnerAttackReachMetres, TwoPlaceFormat) + FString(TEXT(" metre")),
+			FString(TEXT("within ")) + FormatInPlayerUnits(OwnerAttackReachMetres + 1.f, TwoPlaceFormat) + FString(TEXT(" metre")) },
+		{ TEXT("Hero.Attack"), TEXT("the melee cone's half-angle (GetMeleeHalfAngleDegrees, spawned hero class default, degrees)"),
+			FString(TEXT("cone reaching ")) + FormatInPlayerUnits(OwnerAttackHalfAngle, OnePlaceFormat) + FString(TEXT(" degree")),
+			FString(TEXT("cone reaching ")) + FormatInPlayerUnits(OwnerAttackHalfAngle + 1.f, OnePlaceFormat) + FString(TEXT(" degree")) },
+		{ TEXT("Hero.Attack"), TEXT("the melee cooldown (GetMeleeCooldown, spawned hero class default, seconds)"),
+			FString(TEXT("at most once every ")) + FormatInPlayerUnits(OwnerAttackCooldownSeconds, TwoPlaceFormat) + FString(TEXT(" second")),
+			FString(TEXT("at most once every ")) + FormatInPlayerUnits(OwnerAttackCooldownSeconds + 1.f, TwoPlaceFormat) + FString(TEXT(" second")) }
+	};
+
+	// The pure composer's own fallback lane, as in tests 13 and 20: no layout subsystem, no applied keys.
+	auto NoAppliedKeys = [](const FSiegeControlsHelpAction&) -> TArray<FKey> { return TArray<FKey>(); };
+
+	for (const FShownHelpNumber& Shown : ShownHelpNumbers)
+	{
+		// FIXTURE SELF-CHECK: the negative control really is a different clause. (A claim about
+		// the FIXTURE, ⛔ not about a page.)
+		TestNotEqual(*FString::Printf(TEXT("FIXTURE SELF-CHECK: the negative control for %s is a different clause, so (b) means something"), Shown.What),
+			Shown.Clause, Shown.WrongClause);
+
+		const FSiegeControlsHelpAction* const Row = FSiegeControlsHelpRegistry::FindAction(FName(Shown.ActionId));
+		if (!TestNotNull(*FString::Printf(TEXT("Row '%s' is in the registry"), Shown.ActionId), Row))
+		{
+			continue;
+		}
+
+		// ── (a) THE ROW'S OWN COMPOSED DETAIL SHOWS THE OWNER'S VALUE, IN ITS CLAUSE ─────────
+		const FString OwnPage = FSiegeControlsHelpRegistry::ComposeDetailForDisplay(*Row).ToString();
+		TestTrue(*FString::Printf(TEXT("Row '%s' shows %s as its owner holds it: '%s'"), Shown.ActionId, Shown.What, *Shown.Clause),
+			OwnPage.Contains(Shown.Clause, ESearchCase::CaseSensitive));
+
+		// ── (b) NEGATIVE CONTROL: A DIFFERENT VALUE IS NOT WHAT THE PAGE SHOWS ───────────────
+		TestFalse(*FString::Printf(TEXT("NEGATIVE CONTROL: row '%s' does not show a different value ('%s'), so (a) tells numbers apart"), Shown.ActionId, *Shown.WrongClause),
+			OwnPage.Contains(Shown.WrongClause, ESearchCase::CaseSensitive));
+
+		// ── (a2) EVERY RENDERING OF THE ROW SHOWS IT ─────────────────────────────────────────
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
+	// ── (e) ⛔ THE RALLY AND ATTACK TEMPLATES TYPE NO DIGIT ─────────────────────────────────
+	// ⭐ The typed-digit half: the template (Row->Detail, before ComposeDetailForDisplay replaces its
+	// `{#Name}` tokens) carries no digit at all. A number typed in place of a token that happens to
+	// equal its owner passes (a) and (a2); only this sees it.
+	auto TemplateCarriesADigit = [](const FString& Prose) -> bool
+	{
+		for (const TCHAR Character : Prose)
+		{
+			if (FChar::IsDigit(Character))
+			{
+				return true;
+			}
+		}
+		return false;
+	};
+
+	// FIXTURE SELF-CHECK: a scanner that can never answer true would make (e) vacuous. (A claim
+	// about the SCANNER, ⛔ not about the prose.)
+	TestTrue(TEXT("FIXTURE SELF-CHECK: the digit scanner finds a digit when one is present"),
+		TemplateCarriesADigit(FString(TEXT("a cooldown of 20 seconds"))));
+
+	const TCHAR* const TemplateRowIds[] = { TEXT("Hero.Rally"), TEXT("Hero.Attack") };
+	for (const TCHAR* const TemplateRowId : TemplateRowIds)
+	{
+		const FSiegeControlsHelpAction* const TemplateRow = FSiegeControlsHelpRegistry::FindAction(FName(TemplateRowId));
+		if (!TestNotNull(*FString::Printf(TEXT("Row '%s' is in the registry"), TemplateRowId), TemplateRow))
+		{
+			continue;
+		}
+
+		const FString TemplateProse = TemplateRow->Detail.ToString();
+		TestFalse(*FString::Printf(TEXT("⛔ Row '%s' detail TEMPLATE types NO number - every number the page shows is read from its owner when the page is composed"), TemplateRowId),
+			TemplateCarriesADigit(TemplateProse));
+		TestTrue(*FString::Printf(TEXT("Row '%s' detail TEMPLATE carries number tokens, so the check above is not vacuous"), TemplateRowId),
+			TemplateProse.Contains(TEXT("{#"), ESearchCase::CaseSensitive));
+	}
+
```
