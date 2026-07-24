# TASK-268 handoff — `UDeckBuilderWidget`: generated card description + details-selection API

**Agent:** gameplay-programmer
**Branch:** `m7.6-arena10x`
**Status:** ready-for-qa
**Scope:** C++ FILE-ONLY. Not compiled, editor not touched, nothing staged/committed (TASK-269 owns the compile).

## Files touched (exactly two, both purely additive)

| File | Change |
|---|---|
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\DeckBuilderWidget.h` | +85 lines, **0 deletions** |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\DeckBuilderWidget.cpp` | +591 lines, **0 deletions** |

`git diff --stat` reports **676 insertions / 0 deletions** — machine proof that every pre-existing
signature and body (`AddCopy` / `RemoveCopy` / `GetCountOf` / `GetTotalCount` / `GetAverageCost` /
`IsCurrentDeckLegal` / `GetCollectionCardIDs` / `GetCardDisplayName` / `GetCardCost` /
`GetCardMaxCopies` / `GetCardArtTexture` / `LoadDefaultDeck` / `SaveDeckAs` / `LoadDeck` /
`GetSavedDeckNames` / `SetActiveDeck` / `OnDeckModelChanged` / `OnDeckSlotCountChanged`) is
byte-identical. No other file was opened for edit — `CardRow.h`, `cards.csv`, DT_Cards and every
WidgetBlueprint are untouched, and `Notes` is never read.

## Public API — EXACT signatures (TASK-270/271 bind to these)

```cpp
UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
FString GetCardDescription(FName CardID) const;

UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
void SelectCardForDetails(FName CardID);

UFUNCTION(BlueprintCallable, Category = "Siegebound|Deck")
void ClearCardDetails();

UFUNCTION(BlueprintPure, Category = "Siegebound|Deck")
FName GetSelectedDetailCardID() const;

UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Deck")
void OnCardDetailsRequested(const FString& CardID);
```

Private state: `UPROPERTY(Transient) FName SelectedDetailCardID;` (+ three private, non-UFUNCTION
composers: `AppendIdentityLines` / `AppendStatLines` / `AppendRuleLines`).

**Behavior contract for the UI:**
- `SelectCardForDetails(CardID)` stores the ID and fires `OnCardDetailsRequested(CardID.ToString())`.
  `NAME_None` or an unknown row **clears** the selection and still fires **with an empty string** —
  the panel shows `DetailsHintText`.
- `ClearCardDetails()` clears + fires with an empty string (bind `Btn_DetailsClose` to it).
- Neither fires `OnDeckModelChanged` — a details click is deck-neutral, so the x/50 counter, the
  average-cost guide and the `PlayBtn` gate never churn.
- Panel refresh recipe: on `OnCardDetailsRequested`, if the string is empty show the hint; else feed
  it to `GetCardDescription` / `GetCardDisplayName` / `GetCardCost` / `GetCardArtTexture`
  (`FName` from the string; the existing resolvers stay the art/name/cost source).
- `GetCardDescription` returns a **multi-line** `FString` separated by `\n` with a **blank line
  between blocks** → `DetailsBodyText` needs `AutoWrapText` ON and no fixed height.

## Composition order produced

1. **Identity line** — `"<Type> · Cost <n> gold · Max <n> per deck"` (Unit / Building / Economy /
   Spell / Hero Upgrade / Utility; the `Max` clause is dropped when `MaxCopies <= 0`).
2. blank line
3. **Stat block** (each line omitted when its field is 0 or does not apply): `Health:` ·
   `Damage: <n> per attack` (+ ` (splash: every enemy within <n> units of the hit)`) ·
   `Attacks every <n>s` · `Range: <n> units (<melee | homing shot | instant hit | must reach its
   target to detonate>)` · `Blind spot: ...` · `Move speed: <n> units per second`.
4. blank line
5. **Rules block** — one line per applicable clause, in this order: card role (miner / deep mine /
   masons / hero upgrade + stack tail / structure + tower) → Charge → Slayer → Suicide → Swarm →
   Chain → spawner (+ lifetime) → spell effect → spell delivery → targeting profile → castle
   damage scaling.

A block that produces no lines is skipped **with its blank line** — no leading, trailing or doubled
blank ever renders (a spell has no stat block; a plain melee unit has no rules block).
Longest card in the pool = 9 lines (Crystal Tower / Sapper), well inside the ~12-line target.

## Glossary (the ONLY authored copy — `DeckBuilderWidget.cpp`, one contiguous block, per keyword)

`namespace SiegeboundCardGlossary`, 29 strings: `MinerRole`, `DeepMineRole`, `MasonsRole`,
`UpgradeSharpenedBlade`, `UpgradePlateArmor`, `UpgradeSwiftBoots`, `UpgradeWarBanner`,
`UpgradeTailFmt`, `StructureRole`, `TowerRole`, `Charge`, `Slayer`, `SuicideFmt`, `SwarmFmt`,
`ChainFmt`, `SpawnerFmt`, `SpawnerLifetimeFmt`, `SpellAoEDamageCircleFmt`, `SpellAoEDamageLineFmt`,
`SpellFreezeCircleFmt`, `SpellFreezeLineFmt`, `SpellTopTargetsFmt`, `SpellAllyBuffFmt`,
`SpellGoldStealFmt`, `DeliveryHeroLine`, `DeliveryGroundCircle`, `ProfileSiege`,
`ProfileSupportFmt`, `ScalingSiege`, `ScalingRangedVsCastle`, `ScalingSpellVsCastle`.

Every `%s` / `%d` is filled from the **row**; no CSV-column magnitude is ever baked into a string.

## Glossary-mirror sites (each carries a `// mirrors <Class>::<Property>` comment in the .cpp)

| Glossary string | Baked magnitude | Mirrors |
|---|---|---|
| `MinerRole` | +1 gold/s, cap 6 | `ASiegePlayerState::MinerGoldPerTick` (1 / 1 s tick), `::MaxActiveMiners` (6) |
| `DeepMineRole` | +2 gold/s | `ADeepMine::DeepMineIncome` (2) |
| `MasonsRole` | 300 HP / 10 s | `ASiegePlayerController::MasonsHealAmount`, `::MasonsHealDuration` |
| `UpgradeSharpenedBlade` | +10 melee | `AHeroCharacter::MeleeDamageBonus` |
| `UpgradePlateArmor` | +100 max HP + heal 100 | `AHeroCharacter::MaxHPBonus` |
| `UpgradeSwiftBoots` | +25% move | `AHeroCharacter::MoveSpeedBonus` (0.25) |
| `UpgradeWarBanner` | 600 units, +20% | `AHeroCharacter::WarBannerAuraRadius`, `::WarBannerDamageBonus` |
| `Charge` | 2 s / double | `ASummonedUnit::ChargeMoveSeconds`, `::ChargeMultiplier` |
| `Slayer` | 150 HP / double | `ASummonedUnit::SlayerHPThreshold`, `::SlayerMultiplier` |
| `SwarmFmt` | 300-unit circle | `ASiegePlayerController::SwarmSpawnRadius` |
| `ChainFmt` | 350-unit bounce | `ATower::ChainBounceRadius` |
| `SpellAllyBuffFmt` | +50% attack / +25% move | `ASummonedUnit::BattleCryAttackSpeedBonus` (0.5), `::BattleCryMoveSpeedBonus` (0.25) |
| `DeliveryHeroLine` | ~900 long / 100 either side | `ASpellLineSweep::LineRange` (900), `::LineHalfWidth` (100) |
| `ScalingSiege` / `ScalingRangedVsCastle` / `ScalingSpellVsCastle` | ×2 / ×0.5 / ×0.5 | `ACastle::TakeDamage`, `ABuilding::TakeDamage` |

Plus 7 mirrored **CardID** constants (`GlossaryCardID_Miner` … `_WarBanner`), each commented with the
shipping site that keys on the same ID (`ASiegePlayerController::MinerCardID`,
`::BuildingEconomyCardIDs`, `::MasonsCardID`, `AHeroCharacter.cpp UpgradeCardID_*`).

> **Deliberate scope call:** the orchestrator's brief said "add a comment at both sites". The
> reciprocal comments in the OWNING classes were **NOT** written — TASK-268 is sole-owner of
> `DeckBuilderWidget.{h,cpp}` and touching `SummonedUnit.h` / `HeroCharacter.h` / `Castle.cpp` /
> `Building.cpp` / `DeepMine.h` / `Tower.h` / `SpellLineSweep.h` / `SiegePlayerController.h` /
> `SiegePlayerState.h` would blow the diff surface. CONVENTIONS and the board spec both require the
> comment only at the glossary site, which is satisfied. The 14 reciprocal sites are the table above
> — recommend folding them into the next task that opens those files (same shelf as the known-stale
> `ESpellDelivery` comment in `CardRow.h`, which was likewise kept out of this diff).

## Truth-law verification — clause by clause

| Clause | Verified against |
|---|---|
| Castle scaling ×2 Siege / ×0.5 projectile / ×0.5 spell | `Castle.cpp:185-198` (`ACastle::TakeDamage`) |
| Structures take ×2 from Siege only (projectile/spell full) | `Building.cpp:311-323` (`ABuilding::TakeDamage`) |
| Charge = 2 s uninterrupted advance → ×2, lost on stall | `SummonedUnit.h:388-409`, `:724-732` (`TrackChargeMovement`, `ComputeOutputDamage`) |
| Slayer = ×2 vs MaxHP ≥ 150 | `SummonedUnit.h:411-416`, `:715-718` |
| Suicide = blast on contact **or** death, exactly once, then dies | `SummonedUnit.h:735-748` (`Detonate` / `ApplyDetonation`, `bDetonated` guard) |
| Swarm = N copies on a 300-unit circle for ONE cost | `SiegePlayerController.cpp:800`, `:1138-1148`; `SiegePlayerController.h:615-617` |
| Chain = instant zap, N targets, `Damage − n×Falloff` floored at 0, bounce measured from the PREVIOUS target within 350 | `Tower.cpp:380-470` (falloff at `:456`), `Tower.h:52-66`, `:113-114` |
| Tower targets units/hero ONLY (never castles/walls/towers) | `Tower.h:17-24`; `Tower.cpp:225-253` (positive class gate) |
| Blind spot = no target inside MinRange | `Tower.cpp:227`, `:249-253`; `Tower.h:44-48` |
| Buildings are static, BlockAll, nav-relevant | `Building.cpp:42-55` |
| Spawner: interval spawn of `SpawnCardID`, first spawn one full interval later, self-destruct at Lifetime, spawned units persist | `Barracks.h:12-45` |
| Siege profile ignores units/hero → nearest building → castle | `SummonedUnit.h:96-107` |
| Support profile never attacks; heals nearest damaged friendly in Range at row Damage HP/s | `SummonedUnit.h:96-107`, `:379-385` (`SupportHealInterval` keeps HP/s invariant) |
| Ranged unit fires a homing projectile tagged Projectile | `SummonedUnit.h:83-92` |
| AoEDamage (circle) = row Damage in row AoERadius, no friendly fire, castle 50% | `SpellLibrary.h:38-47`; `SpellLibrary.cpp:596-607` |
| AoEDamage / Freeze (**line**) — row AoERadius plays **no part**; corridor is `LineRange`/`LineHalfWidth`; hits units, hero, buildings AND castle (castle 50%); no LOS blocking; a line that hits nothing is still spent | `SpellLineSweep.h:13-68`, `:96-121` |
| Freeze = enemy units + buildings for EffectDuration; castle & hero immune | `SpellLibrary.h:48-53` |
| TopTargetsDamage = MaxTargets highest-**current**-HP enemies in AoERadius, castle EXCLUDED, buildings take full | `SpellLibrary.h:54-59` |
| AllyBuff = friendly units in AoERadius, +50% attack / +25% move for EffectDuration | `SpellLibrary.h:60-65`; `SummonedUnit.h:418-428` |
| GoldSteal = instant, no reticle, `min(GoldSteal, victim gold)` | `SpellLibrary.h:66-70`; `SiegePlayerController.cpp:1452-1456` |
| Delivery resolution (`Auto` → per effect) reuses the ONE brain | `USpellLibrary::GetEffectiveDelivery`, `SpellLibrary.cpp:658+`; called, never re-implemented |
| Only AoEDamage/Freeze actually BRANCH on delivery | `SpellLibrary.cpp:596-629` (`ResolveSpell` dispatch) — so a line reading is only taken for those two |
| Miner: +1/s only on registered arrival, killed/evicted ends it, cap 6 alive | `MinerUnit.h:13-70`; `SiegePlayerState.h:186-223`, `:294`, `:302` |
| Deep Mine: +2/s on placement, no miner-slot cost, income removed on death | `DeepMine.h:12-42`, `:66` |
| Masons: 300 HP over 10 s to the friendly castle; refused with no gold when there is none | `SiegePlayerController.cpp:2045-2079`; `Castle.h:88-100` |
| Hero upgrades: instant, cap = the row's own MaxCopies, over-cap refused with no spend, persists through death, resets only on match end | `HeroCharacter.h:195-212`, `:466-486`; `HeroCharacter.cpp:804-815`, `:568-571`; `SiegePlayerController.cpp:2003-2040` |

**Clauses deliberately OMITTED as unverifiable/misleading:** a castle-scaling line on towers (they
can never hit a castle); the row `AoERadius` on line-delivered Fireball/FrostNova; a delivery line on
Pickpocket (instant, no aim); any effect line for a malformed row (the resolver would refuse it, so
the description promises nothing); any clause for an unknown Utility/HeroUpgrade CardID.

## Example rendered descriptions (hand-derived from `Docs/Data/cards.csv` — panel sizing reference)

**Footman** — plain melee (no rules block at all):
```
Unit · Cost 3 gold · Max 12 per deck

Health: 80
Damage: 12 per attack
Attacks every 1s
Range: 120 units (melee - it must close to contact)
Move speed: 400 units per second
```

**Archer** — ranged + castle scaling:
```
Unit · Cost 4 gold · Max 10 per deck

Health: 45
Damage: 10 per attack
Attacks every 1.2s
Range: 700 units (fires a homing shot)
Move speed: 350 units per second

Shots hit a castle for HALF the listed damage - units, heroes and structures take the full amount.
```

**Crystal Tower** — chain (widest card; the falloff sequence is computed, not authored):
```
Building · Cost 9 gold · Max 3 per deck

Health: 150
Damage: 15 per attack
Attacks every 1.5s
Range: 800 units (instant hit - there is no shot to dodge)

Stationary structure: it never moves, physically blocks the ground it stands on, and holds until it is destroyed.
Fires on its own at the nearest enemy unit or hero within range - it never shoots castles, walls or other structures.
Chain: every shot is an instant zap that arcs through up to 3 enemies, weakening as it goes (15 / 10 / 5 damage in turn). Each arc only reaches an enemy within 350 units of the previous one, so a lone target takes just the first hit.
```

**Barracks** — spawner (spawned card resolved to its DisplayName, never the raw CardID):
```
Building · Cost 10 gold · Max 3 per deck

Health: 250

Stationary structure: it never moves, physically blocks the ground it stands on, and holds until it is destroyed.
Summons a free Footman every 8 seconds, on your side and at no extra cost - the first one arrives a full interval after it is built.
It falls apart on its own after 60 seconds; everything it already summoned stays on the field.
```

**Fireball** — spell + HeroLine delivery (no stat block; radius deliberately absent):
```
Spell · Cost 7 gold · Max 3 per deck

Deals 100 damage to every enemy the bolt passes through - units, heroes, structures and castles alike. Your own side is never hit.
Aimed from your hero: it flies out as a bolt roughly 900 units long, catching anything within 100 units to either side, and passes straight through walls and bodies. A bolt that catches nothing is still spent.
A castle caught in it takes HALF damage; everything else takes the full amount.
```

**Ogre** — Siege profile:
```
Unit · Cost 12 gold · Max 2 per deck

Health: 500
Damage: 35 per attack
Attacks every 1.5s
Range: 120 units (melee - it must close to contact)
Move speed: 250 units per second

Siege: it ignores enemy units and the enemy hero completely, walking past them for the nearest enemy structure - and for the castle when none is left.
Its damage lands on castles and structures at DOUBLE the listed amount.
```

Bonus (the anti-drift proof) — **Lightning** renders `within 700 units`, the LIVE `AoERadius`, while
the `Notes` cell still says "in 400". Nothing in the pipeline can print the stale number.

## What QA should scrutinise

1. **`FString::Printf` format-string types.** The glossary is `const TCHAR Name[] = TEXT(...)`, NOT
   `const TCHAR* const` — UE's `Printf` static-asserts on a TCHAR **array**; a pointer would not
   compile. Any future glossary entry must keep the array form.
2. **The middle dot.** `IdentitySeparator()` composes U+00B7 from its code point
   (`FString::Printf(TEXT(" %c "), static_cast<TCHAR>(0x00B7))`) so no string literal in this file
   carries a non-ASCII byte — the compiler codepage cannot turn it into mojibake. Comments still
   carry raw UTF-8, exactly like the rest of the module.
3. **`SwarmCount > 1`, not `> 0`** (spec says "SwarmCount"): a `1` would render "one play puts 1 of
   them on the field", which is both silly and untrue. No row in the table authors 1, so the
   rendered output for the shipping 28 is identical either way. Flagging as a deliberate reading.
4. **Support/suicide/spell rows print NO `Damage: n per attack` line** — a Cleric's `Damage` is a
   heal rate, a Sapper's is its blast and a spell's is its effect. This is a truth-law call, not an
   omission; each number still appears, in its own rules line.
5. **`GetCardDescription` is `BlueprintPure` yet resolves the table** via `LoadSynchronous`
   (precedent: `GetCardCost` / `GetCardMaxCopies` are Pure and do exactly the same; the table is
   engine-cached after first load). Spec-mandated.
6. **Null-safety paths:** `NAME_None` → empty string, silent (matches `GetCardArtTexture`);
   unknown row / missing table → empty string, logged once through the EXISTING
   `bWarnedMissingTable` / `WarnedMissingRowIDs` guards. No new logging channel, no ensure, no
   crash. `SelectCardForDetails` on a bad ID clears rather than parks.
7. **`Notes` is not referenced anywhere in the diff** (grep-checkable), and no `Description` column,
   `FCardRow` change or DT_Cards reimport exists.

## Downstream notes for TASK-270/271 (art-director)

- Panel must hold **9 lines + 2 blank separators** worst case at the widest card; body text
  `AutoWrapText` ON, on an opaque dark plate (the legibility law).
- Bind `Btn_CardFace` → `SelectCardForDetails(CardID)` only. It must never call `AddCopy`/`RemoveCopy`,
  and it goes bottom-most in the tile Overlay so the `+`/`−` plate keeps hit-test priority.
- `Btn_DetailsClose` → `ClearCardDetails()`. Empty string on `OnCardDetailsRequested` ⇒ show
  `DetailsHintText`.

---

## DELTA — 2026-07-24 compile fix (C7595 non-literal format string)

Build-master's TASK-277 editor-target build failed to compile this file at 14 sites (line numbers 873, 901, 909, 929, 937, 942, 969, 974, 982, 987, 995, 1003, 1011, 1037 — the report says "15", the authoritative line list is 14; details appended to `qa/TASK-268.md`). **This is a compile-correctness fix only; the rendered card descriptions are byte-identical to the QA-verified composition, and TASK-268 STAYS PARKED/uncommitted pending its held integration (TASK-269+).**

- **Root cause:** UE 5.8's `FString::Printf` takes a `TCheckedFormatString`. The macro `UE_CHECK_FORMAT_STRING` (`Engine/.../String/FormatStringSan.h:18`) evaluates the format inside a `constexpr` initializer: `constexpr UCFS::FResult UCFS_Result = UCFS_FChecker::Check(false, 0, Fmt);`. For that to be a constant expression, the format array must be **usable in a constant expression**. A namespace-scope `const TCHAR Foo[]` is NOT (arrays aren't const-integral, and it isn't `constexpr`), so binding it into the `constexpr` evaluation fails → **C7595** ("call to immediate function is not a constant expression"). The prior QA note assumed a `const TCHAR[]` array satisfied the checker; it does not — it must be `constexpr`.
- **Fix (pattern, applied to all 14):** promoted each glossary FORMAT constant from `const TCHAR …Fmt[] = TEXT("…")` to `constexpr TCHAR …Fmt[] = TEXT("…")`. Constants changed: `UpgradeTailFmt`, `SuicideFmt`, `SwarmFmt`, `ChainFmt`, `SpawnerFmt`, `SpawnerLifetimeFmt`, `SpellAoEDamageCircleFmt`, `SpellAoEDamageLineFmt`, `SpellFreezeCircleFmt`, `SpellFreezeLineFmt`, `SpellTopTargetsFmt`, `SpellAllyBuffFmt`, `SpellGoldStealFmt`, `ProfileSupportFmt`.
- **Why this over inlining literals at the call sites:** it is provably byte-identical (the format strings and all 14 `FString::Printf(...)` call sites are untouched — a `constexpr TCHAR[]` decays to `const TCHAR*` identically to `const TCHAR[]` at every use site) and it **preserves the single-source-of-truth glossary** QA verified (all 20 mirrored magnitudes + anti-drift composition). Inlining would have duplicated/orphaned those constants and risked a transcription-drift that QA's exact-composition review specifically guards against.
- **Not changed:** the non-format glossary constants (`StructureRole`, `Charge`, `Slayer`, `DeliveryHeroLine`, `ProfileSiege`, `ScalingSiege`, etc.) stay `const TCHAR[]` — they are passed to `OutLines.Add(...)` (plain FString construction, no consteval check), so they never triggered C7595 and need no change.

**QA to scrutinize:** confirm output byte-identical (strings + call sites unchanged — grep shows all 14 `…Fmt[]` now `constexpr`, TEXT() bodies identical) and that the consteval requirement is actually satisfied by `constexpr` (it is: a `constexpr` array is usable in the `constexpr UCFS_Result` initializer where a plain `const` array is not). Files touched: `DeckBuilderWidget.cpp` only. No `.h` change, no logic, no compile, no Git. Integration remains held.
