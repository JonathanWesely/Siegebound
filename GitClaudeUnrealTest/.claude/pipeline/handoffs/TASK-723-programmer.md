# TASK-723 — [HIGH-1] `cards.csv`: ×3 range on the three ranged UNITS + the `WatchTower` row

- **Assignee:** gameplay-programmer
- **Status:** ready-for-qa
- **Law:** `HIGH-§4` / `§6` / `§8` · `TOWER-§3` · `UNCAP-§2` · GDD §3.0 (data-driven card stats)
- **Files touched: EXACTLY ONE — `Docs/Data/cards.csv`.** 4 insertions, 3 deletions. No C++, no `.uasset`, no editor, no MCP, no compile, no Git.

---

## 1. THE ×3 — the ranged set RE-CONFIRMED AT THE DATA, then applied

⭐ **I did not trust the spec's table — I enumerated the selector `bRanged == true AND CardType == Unit` against the file itself before typing a digit.** The live data returns **EXACTLY THREE** rows and **agrees with the spec in every cell.** No disagreement, so no STOP.

| CardID | CardType | bRanged | **Range BEFORE** | **Range AFTER** | Factor |
|---|---|---|---|---|---|
| `Archer` | Unit | true | **700** | **2100** | ×3 exact |
| `Wizard` | Unit | true | **700** | **2100** | ×3 exact |
| `Longbowman` | Unit | true | **1200** | **3600** | ×3 exact |

**Nothing else on those three rows moved.** Verified post-edit, cell by cell:
- `Wizard.AoERadius` = **250, UNCHANGED** — splash is not reach.
- Cost / Damage / Cadence / Speed / HP / DeckCount / MinRange on all three: **untouched** (Archer 12/10/1.2, Wizard 24/15/1.6, Longbowman 18/18/1.5).

### Deliberately NOT touched — each re-verified at the data, not assumed

| Row | Range | Why it was excluded |
|---|---|---|
| `ArrowTower` | **900** | `bRanged=true` but **`CardType=Building`** ⇒ outside the selector (`HIGH-§4`; his lever is `HIGH-§7` row **R-2**) |
| `BombTower` | **800** | same — Building |
| `BallistaTower` | **1400** | same — Building |
| `CrystalTower` | **800** | `bRanged=false` (instant chain zap, not a projectile) **and** a Building |
| `Cleric` | **400** | ⛔ **a HEAL RADIUS on a unit that cannot attack** (`bRanged=false`, `Profile=Support`). Tripling it would triple a heal he never asked about |
| `Sorcerer` | 0 | cannot attack |
| all melee units | 120 | contact reach, `bRanged=false` |

---

## 2. THE `WatchTower` ROW — as written, verbatim

```
WatchTower,Watch Tower,Building,30,3,250,0,0,0,0,None,Climbable tower: no auto-fire; own units ascend the ramp to a raised platform for the high-ground damage bonus (TOWER-3),0,false,false,false,false,0,0,0,None,0,0,/Game/UI/CardArt/T_CardArt_WatchTower.T_CardArt_WatchTower,None,0,0,0,0,0,
```

**DELIMITER COUNT, as required: the row carries 30 commas ⇒ 31 fields, byte-identical in arity to the header (30 commas / 31 fields) and to all 30 pre-existing rows.** A machine audit of every row in the file reports **zero** delimiter mismatches. ⛔ **No comma appears anywhere inside `Notes`** (semicolons and parentheses only, matching the shipped `Miner` / `Wizard` note style). The file remains **unquoted, ASCII-only, zero `"` characters, CRLF throughout (32 CRLF, 0 bare LF), terminated with CRLF** — the appended line was written with an explicit `\r\n` so no mixed-ending line was introduced.

### Cell derivation — every value from `TOWER-§3`'s binding table, none re-derived

| Cell | Value | Source |
|---|---|---|
| RowName / CardID | `WatchTower` | `TOWER-§3`, row **T-1** |
| `DisplayName` | `Watch Tower` | `TOWER-§3` states it verbatim; matches the shipped spaced-display family (`Arrow Tower`, `Bomb Tower`, `Ballista Tower`, `Crystal Tower`) |
| `CardType` | `Building` | his word |
| `Cost` | **30** | ⭐ **Jonathan's number, verbatim, untuned** |
| `MaxCopies` | 3 | mirrors Barracks; inert for a Building (`UNCAP-§2`) |
| `HP` | **250** | derived — Barracks is the other 30-gold non-weapon structure |
| `Damage` / `Range` / `Cadence` | **0 / 0 / 0** | it does NOT auto-fire; `Cadence 0` also guarantees no timer arms (`ABuilding` / `qa/TASK-021` WARN-1) |
| `bRanged` | **false** | ⛔ **LOAD-BEARING** — `true` would pull it into the `bRanged && Unit` selector |
| `DeckCount` | **0** | `sum(DeckCount)` re-measured after the edit = **exactly 50** ✅ |
| `Speed`/`Profile`/`SwarmCount`/`AoERadius`/`MinRange`/`SpawnCardID`/`SpawnInterval`/`Lifetime`/all spell+keyword cells | 0 / None | inert, matching the Barracks shape |
| `CardArt` | `/Game/UI/CardArt/T_CardArt_WatchTower.T_CardArt_WatchTower` | pinned by spec + `TOWER-§3` |
| `SpellDelivery` | empty | trailing field, matching every non-spell row |

A cell-by-cell diff against `Barracks` shows differences in **only** the intended places: identity (RowName/DisplayName), `Notes`, `CardArt`, and the spawner triple (`SpawnCardID`/`SpawnInterval`/`Lifetime` zeroed — WatchTower is not a spawner). **Every other cell is byte-identical to Barracks.**

🧑 **On the name (row T-1):** I have **no better name to offer** — `WatchTower` sits correctly inside the shipped `<Name>Tower` family and the Blueprint path `BP_Building_WatchTower` is *forced* by `SiegePlayerController.cpp:3842` / `SiegeBotController.cpp:1570` (`BP_Building_<CardID>_C`). If Jonathan wants a different name it must change **here, in the C++/BP names, and in three art asset names together** — cheap this hour, expensive after TASK-727/728 land.

---

## 3. VERIFICATION DECLARED (spec item 3) — `ACastle` HAS NO ATTACK LOOP ✅

**Result: CONFIRMED NEGATIVE. There is no "ranged units now outrange castle defences" interaction, because the castle has no defences to outrange.**

- `Castle.h` declares **no** fire/scan/acquire member function at all (grep for `Fire|Scan|Attack|Acquire` on declaration lines returns **nothing**).
- `Castle.cpp` arms exactly **one** timer — `HealTimerHandle` → `ACastle::HandleHealTick` (`Castle.cpp:1608`). It heals; it does not shoot.
- Sweeping the whole `Siegebound/` module for a fire-handler timer returns **exactly two** auto-fire paths: `ATower::ScanAndFire` (`Tower.cpp:151`) and `ASummonedUnit::PerformAttack` (`SummonedUnit.cpp:1146`, `:2615`). **`ATower` is the only auto-firing *building*, and `ACastle` is not one.**

---

## 4. ⚠️ THE CONSEQUENCE — DECLARED, NOT SOFTENED (spec item 4)

**He asked for triple. He got triple. Here is what triple does, computed from the post-edit file, not asserted.**

### ⚠️ SEVERE: every defensive tower is now outranged by a unit it cannot answer

| Defender | Range | vs Archer **2100** | vs Longbowman **3600** |
|---|---|---|---|
| ArrowTower | 900 | outranged **2.3×** | outranged **4.0×** |
| BombTower | 800 | outranged **2.6×** | outranged **4.5×** |
| CrystalTower | 800 | outranged **2.6×** | outranged **4.5×** |
| BallistaTower | 1400 | outranged **1.5×** | outranged **2.6×** |

⇒ **A stationary Archer or Longbowman can demolish any tower card for free. Towers stop functioning as defence.** ⛔ **This is NOT softened and no number was pulled back to avoid it.** The fix he may want — **triple the towers too** — is `HIGH-§7` row **R-2**, a one-word overrule that would be four more cells in this same file. It rides to him via TASK-732.

### ✅ REASSURING COUNTERPART: the arena absorbs most of the ×3

Measured at source (`ScatterConfig.h:356` `ArenaHalfExtent = (26000, 12000)` ⇒ field **52,000 × 24,000 uu**; castles at **X = ±25,000** ⇒ **50,000 uu apart**, verified in `BattlefieldScatter.h:31/:555`, `SiegeBotController.h:477`, `SiegeGameMode.h:319`):

- **Longbowman 3,600 uu = 7.2 % of the 50,000 uu castle-to-castle line** and **15.0 % of the field's 24,000 uu width**.
- **Archer 2,100 uu = 4.2 % of that line.**

⇒ ⭐ **"They will now outrange across most of the arena" is FALSE**, and worth telling him plainly. The M7.6 10× scale-up absorbs most of the change. **The pain is tower-vs-unit, not unit-vs-map.**

### ⚠️ Watch item, carried forward, NOT fixed here
AI engagement distances change — ranged units halt and open fire from 3× further out, and the bot's approach logic was tuned against the shipped ranges (`HIGH-§6`). **No observation exists yet; boarding a repair for an unobserved symptom is guessing.** It is a playtest watch item for TASK-732.

---

## 5. ⛔⛔ THIS EDIT IS INERT UNTIL `/Game/Data/DT_Cards` IS REIMPORTED

**Nothing in this task changes anything in-game yet.** `Docs/Data/cards.csv` is the *source* for the DataTable; the runtime reads `/Game/Data/DT_Cards.DT_Cards` (`Building.cpp:76`, `CardHandWidget.cpp:19`). **The reimport is an EDITOR step and it is TASK-731's — ⛔ it is NOT claimed done here.** ⚖️ *A CSV edit without a reimport is a no-op that looks done.* If TASK-731 skips it, every figure above is fiction and the game plays exactly as it did before.

---

## 6. What QA should scrutinise

1. **Delimiter arity on the appended row** — the whole risk of an unquoted CSV. Count the commas in the `WatchTower` line: it must be **30**. I count 30; a stray comma in `Notes` would shift every later column *silently* and the file would still parse.
2. **`bRanged=false` on `WatchTower`** — flip that one token to `true` and the card silently joins the `bRanged && Unit` selector for TASK-724's elevation bonus and any future ranged sweep. (It is a Building, so it would not match `CardType == Unit` today — but it is the token that makes that safety hold.)
3. **`sum(DeckCount)` is still exactly 50** — re-measure it; a non-zero DeckCount on the new row would break default-deck legality.
4. **The three range cells are field 8 (`Range`) and not a neighbour** — Archer's row has `10` (Damage) immediately before `2100`, and `1.2` (Cadence) immediately after. Confirm the edit landed on `Range`, not `Damage`.
5. **`Cleric` still reads 400 and the three tower Ranges still read 900 / 800 / 1400.**
6. **Line endings** — the file must stay pure CRLF; a bare-LF final line would be my error. Measured: 32 CRLF, 0 bare LF.

## 7. Declared deviations

- **NONE from the spec's numbers.** Every value is the spec's or `TOWER-§3`'s, unmodified.
- **One value the spec did not name: `DisplayName = "Watch Tower"`** (with the space). The task spec listed the row's cells but not `DisplayName`; I took it from `TOWER-§3` ("CardID `WatchTower`, DisplayName \"Watch Tower\"") and from the shipped building convention (`Arrow Tower`, `Bomb Tower`). **Flagged so it is a choice on the record, not an assumption.**
- **The `Notes` text is mine** (the spec did not dictate wording, only "no comma"): `Climbable tower: no auto-fire; own units ascend the ramp to a raised platform for the high-ground damage bonus (TOWER-3)`. It writes `TOWER-3` rather than `TOWER-§3` because **the file is ASCII-only and I would not be the one to introduce a non-ASCII byte into an unquoted CSV that a DataTable importer parses.**
- **Row placement: appended as the last line** (after `Sorcerer`), per "APPEND". DataTable row order is not semantic.

## 8. Not mine — declared so it is not misattributed to TASK-723

`git status` shows **`Docs/setupdirections.md`** modified (+97 lines, an "Appendix D — Claude permissions" section). **I did not touch that file.** It is pre-existing working-tree state from an earlier session. **My diff is `Docs/Data/cards.csv` and nothing else.**

## 9. Fences honoured

⛔ No C++ · ⛔ no `DT_Cards` reimport · ⛔ no compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ `Tools/Packaging/` untouched · ⛔ no other task's file touched. **Sole ownership of `cards.csv` this batch was respected: one file, one diff.**
