# QA Report — TASK-1192 (README-GATE over TASK-1191)
Verdict: PASS (LOOP 2, 2026-09-09 — see the § LOOP 2 section at the end; loop 1 below was FAIL and is preserved as written)

Subject: `Docs/Packaging/README-source.md` (944 lines, 308 `<!-- src:` markers counted by grep — the handoff's 333 counts multi-cite lines) at HEAD `8c444ca` as declared. Reviewer held Read/Grep/Glob only — no git, no engine, no shell (`SC-§71b`; every HEAD-vs-tree claim below is accepted as declared).

Bottom line: the document is very close. All 34 card rows match `Docs/Data/cards.csv` cell-for-cell, the archer worked example resolves at every stat, and ~300 C++/config/board citations were re-read at source and hold. It fails on the row's own cl. 1 rule — two C++-derived numbers carry citations that do not resolve to the number (both copied verbatim from the census table, which is exactly the trap the row names) — plus one mechanic sentence the code contradicts by omission. Three edits close it.

## Findings

### BLOCKER
1. [BLOCKER] `README-source.md:716-717` — Follow "**within 900 units**" cited `Siegebound/SiegePlayerController.h:2569-2583`. Those lines are the `CancelGroupPick()` / `OnCmdFollowPressed()` declarations and carry no number. The number lives at `SiegePlayerController.h:451` (`float FollowFormationRadius = 900.f;`). `SC-§110`: a citation naming the wrong line is a false literal. Fix: cite `:451` (keep the handler decl as a second landmark if desired). Source of the error: census §4a row `C` cites the same range.
2. [BLOCKER] `README-source.md:710-713` — Hold pick "**1200** / **700** / **1500**" cited `SiegePlayerController.h:1684; SiegeAssistantComponent.h:1726-1730`. `PC.h:1684` is the `OnCmdHoldPressed()` declaration, no number. `SiegeAssistantComponent.h:1726/1730` hold 700/1500 as self-declared *mirrors* of the controller (their own comment says so), and **1200 resolves nowhere in the citation**. The three numbers live at `SiegePlayerController.h:2131` (`GroupSelectRadiusDefault = 1200.f`), `:2135` (`GroupPositionRadiusDefault = 700.f`), `:2139` (`GroupAttackRadiusDefault = 1500.f`). Fix: cite those three lines. Source of the error: census §4a row `R` cites the same lines.
3. [BLOCKER] `README-source.md:471-472` — "The veil breaks permanently when the unit **attacks, heals or mines**" cited `SiegeInvisibilityStatics.h:97; :125; :137; :157`. The enum `ESiegeVeilBreakReason` (`:97-207`) has **six** enumerators: `Attack :125`, `Heal :137`, `Mine :157`, `Empower :176`, `Cast :195`, `Death :206`. Two omitted ones are wired live and player-reachable: `Empower` at `AncientGround.cpp:259` (a veiled Sorcerer counted on an ancient ground loses its veil) and `Cast` at `SummonedUnit.cpp:3607` (a veiled Witch completing her own cast loses hers). The README's sentence is exhaustive in form and the code disagrees with it. Fix: add both acts (and, optionally, that death clears it); cite `:176`, `:195`, `AncientGround.cpp:259`, `SummonedUnit.cpp:3607`.

### WARN
- [WARN] `README-source.md:470` — "skipped by enemy targeting" cited `SummonedUnit.cpp:1806`; that line is a comment about the team compare. The veil policy is `ESiegeVeilPolicy::SuppressVeiled` at `SummonedUnit.cpp:1850` (the war-map half at `SiegePlayerController.cpp:7218-7219` resolves). Fix the cite.
- [WARN] `README-source.md:733-737` and `:245-246` — bot rule `.cpp` ranges are offset ~50-60 lines (values in the table resolve at the `.h` lines, so the numbers are right): rule 3a Fireball is `SiegeBotController.cpp:747-809` (README cites `:701-733`, which is rule 2b Deep Mine); rule 3b Lightning is `:811-853` (cited `:735-766`); rule 4 Attack is `:856-907` (cited `:769-800`); rule 5 Cycle starts `:909` (cited `:804-810`). Rule 1 (`:446-` vs cited `:433-490`) and rule 2 (cited `:496-610`) overlap acceptably. Fix the four ranges.
- [WARN] `README-source.md:751` and `:926` — "There is no bot in a networked match" cited `SiegeGameMode.cpp:1719-1725` (the bot-class fallback). The gate is `SiegeGameMode.cpp:1678-1684` (`if (bNetworkedMatch) … return;`). Fix the cite.
- [WARN] `README-source.md:727` — "It has **no hero**" cited `SiegeBotController.h:217` (that line is `BotTeam = Red`). The fact is carried by `SiegeGameMode.cpp:1669-1765` (`SpawnBot` spawns a controller only — already co-cited, so the claim holds) and by `SpellLineSweep.h:124-125` / `SiegeBotController.cpp:772-773` ("the bot has no hero"). Drop or replace the `.h:217` half.
- [WARN] `README-source.md:50, :58, :76, :137` — the five `Tools/Packaging/ship.ps1` citations (`:836`, `:841`, `:1090-1091`, `:1174-1188`, `:2835-2839`) are pinned to `HEAD 8c444ca` of a file that is dirty from the live ship; the working copy at those lines holds other content (suite-result parse, click-abort message, capture comment). QA cannot run `git show` (`SC-§71b`) — **accepted as declared**. Build-master: confirm `git show HEAD:Tools/Packaging/ship.ps1` at those lines before render, or re-pin the comments to symbols (`Get-StageShimExeRel`, gate `C4-NO-MODELS`) per `SC-§110` cl. 3d. `D2-ZIP-READBACK` independently proves the click target at ship time.
- [WARN] `README-source.md:123, :132` — model size "about 2.5 GB" cited to a code comment (`Tools/fetch_llm_model.py:8` — verified: "The weights are ~2.5 GB"); the 2026-08-29 README said ~2.3 GB. **Ruling: acceptable as declared, not a `{{SHIP:NOT_MEASURED}}` case** — the figure is hedged ("about"), the citation holds the number, and it describes a file that is *excluded* from the package, so nothing in the zip depends on it. Build-master: if `Models\Qwen3-4B-Q4_K_M.gguf` is present on the cooking machine, `Get-Item` it and replace figure + citation in the same render; otherwise leave as is.
- [WARN] `README-source.md:856-874` — the 12-row *Notes for readers of the design document*. **Ruling: KEEP.** Every row verified at both ends (`Docs/GDD.md:47` 5 s · `:73` Max Copies · `:125` 2,460 · `:133` 1 gold · `:147` 600 · `:159` 900 · `:223` 2500 · `:350` 30 cards · `:359/:373/:405` 700/1200/700 · `:374` 1 s cadence · `:429` 800 · `:437` "no stat bonuses" · `:443` supersession; and the code lines opposite). `SHIP-§3a` and board cl. 2 require the difference *noted*; it is contained, skippable and leaks no IDs. Suggested reword (non-blocking): heading → "Where the shipped game differs from its design notes", plus one clause that the design document is not included in this package (line 121 already says `Docs/` was left out), so a player is not sent looking for a file that is not there.
- [WARN] `README-source.md:267, :920` — `{{SHIP:CLOUD_SYNC}}` is a ninth placeholder outside `SHIP-§3a`'s eight. `ship.ps1` has no placeholder renderer (`:95` — "The caller writes packagedZIPofGame/README.md"), and `.claude/commands/ship.md` mentions neither `README-source.md` nor any `{{SHIP:` token (grep = 0). The resolver **is** named (build-master, handoff §2, both texts supplied), so this is not a surviving-placeholder blocker — but the render is a by-hand agent step with no script check. Build-master must grep the rendered file for `{{SHIP:` = 0 before Phase D's zip (`SHIP-§3a` STOP). The missing `ship.md` step is a pipeline gap for the manager, not a README defect.
- [WARN] `README-source.md:898` — "*Custom* when the groups disagree" cited `SiegeGraphicsSettingsSubsystem.h:150-162`; `CustomQualityLevel = -1` is at `:163`. Extend the range by one line.

### NIT
- [NIT] `README-source.md:492` — Sapper "Attacks every: **once**"; the CSV cadence cell is `1.0`. "once" is a rendering of Suicide, not the cell. Consider "1.0 s (it detonates on the first)".
- [NIT] `README-source.md:440-441` — castles "about 25000 units" cited to code fallbacks (`BattlefieldScatter.cpp:1458-1462`, `SiegeBotController.h:477`); the level's actual transforms are NOT MEASURED (census §11). "about" covers it; consider saying the figure is the code's placement, which the level agrees with by comment.
- [NIT] `README-source.md:146` — "every key and every card works exactly as described" is a promise, not an observation (`SC-§109` taste, not a violation). "is described … below" would be neutral.
- [NIT] `README-source.md:812` — mid capture zone cited to `Content/Maps/L_Arena.umap (CaptureZone actor); CaptureZone.h` with no line; the numbers on the next lines are cited (`:190/:194/:204`), so nothing is uncited — cosmetic.

## Jonathan's spec — clause by clause

| Clause | README | Ruling |
|---|---|---|
| a section called "GameDetails" | `## GameDetails` line 282 | satisfied |
| subsection "gameplay" | `### Gameplay` line 284 | satisfied |
| all card descriptions, costs, effects | five tables (lines 483-542, 34 rows) + *Card by card* 1-34 (lines 550-662) quoting the shipped panel text (all 26 quoted sentences match `DeckBuilderWidget.cpp:71-268` verbatim) | satisfied |
| all the commands you can use | 28 bindings (lines 666-689) + wheel/Z/F11/Alt+Enter (691-698) + *The orders in detail* (704-720); every handler line verified in `PC.cpp`/`HC.cpp`/`SiegeGhostPawn.cpp`; the 28 count is accepted as declared (asset) | satisfied, save BLOCKERs 1-2 (citations, not values) |
| the AI commander and what it does | three readings (722-771): bot rules/decks/placement, commander + war map, assistant (inert) | satisfied |
| castle health | 2000 (`Castle.h:359`), crumble 75/50/25, damage scaling by type, Masons repair, torches, positions, HUD bars | satisfied |
| areas of interest: what spawns, effects, how many, how locations are decided | mines 6 (3/side) with the full draw rule; ancient grounds 2 with band/attempts; mid zone 1 fixed; castles 2; 7 scatter layers with counts (counts accepted as declared from the asset decode) | satisfied |
| very detailed — every detail about every card (archer: cost, health, damage, attack range, notice range, retention range, other details) | see the archer table below | satisfied |
| second subsection "other features" — multiplayer, settings, deckbuilder, etc., comprehensive | `### Other features` line 876: main menu (7 entries), Sandbox, Deck Builder, Settings, Graphics (every row enumerated), Login, Cloud (placeholder), Multiplayer (host plays / joiner observes), Quit, in-match list, present-but-unusable | satisfied |
| keep all the other stuff, update anything wrong | all ten original sections present in order; corrections: template-name apology deleted (`PKG-§8`), Development → `{{SHIP:CONFIG}}`, RAM figure dated, model size re-cited, "compiled `.exe`" → "program" | satisfied |

## The archer — every stat at source

| Stat | README (line 488) | Source | Holds |
|---|---|---|---|
| Cost | 12 | `cards.csv:3` Cost=12 | yes |
| Health | 45 | `cards.csv:3` HP=45 | yes |
| Damage | 10 | `cards.csv:3` Damage=10 | yes |
| Attack range | 2100, homing shot | `cards.csv:3` Range=2100, bRanged=true; `Projectile.h:150` 1500 u/s, `:159` 30 | yes |
| Attack range under fog | 609.6 | `SummonedUnit.cpp:1785/1972/2049` `ApplyFogVisionCeilingUU(AttackRange)`; ceiling `SiegeFogStatics.h:268` = 609.6; `SiegeFogStatics.cpp:122` `Min` | yes |
| Attacks every | 1.2 s | `cards.csv:3` Cadence=1.2 | yes |
| Speed | 350 | `cards.csv:3` Speed=350 | yes |
| Notice (paper) | 5000 | `cards.csv:3` NoticeRange blank → `SummonedUnit.cpp:4537-4571` `ResolveNoticeRadiusUU` returns class default; `SummonedUnit.h:914` `UnitEngagementRadiusUU = 5000.f` | yes |
| Notice (under fog) | 609.6 | acquisition funnel `SummonedUnit.cpp:1847-1850` + `SiegeCombatStatics.cpp:257`; ceiling as above | yes |
| Retention (paper) | 8000 | `SummonedUnit.h:1448` LeashRange 8000, `:1470` margin 1.5 → `SummonedUnit.cpp:4573-4589` `max(8000, 5000×1.5=7500)` = 8000 | yes |
| Retention (under fog) | 609.6 | `SummonedUnit.cpp:1752` and `:2024` `ApplyFogVisionCeilingUU(GetEffectiveLeashRangeUU())` | yes |
| Targeting | Standard | `cards.csv:3` Profile=Standard; tie-break 100 `SummonedUnit.h:1474` | yes |
| Default deck | 8 | `cards.csv:3` DeckCount=8 | yes |
| vs castle | half | `Castle.cpp:1157-1159` Projectile ×0.5; glossary `DeckBuilderWidget.cpp:264` | yes |
| Height bonus | +10 % per 152.4 | `SummonedUnit.h:1687/1700`; gate `bRangedAttack` `SummonedUnit.cpp:4771-4778` | yes |

## Card verification — 34/34

Every row of `Docs/Data/cards.csv:2-35` compared cell-for-cell against the five README tables (Cost, HP, Damage, Range, Cadence, Speed, Profile, DeckCount, bRanged/bCharge/bSlayer/bSuicide, SwarmCount, AoERadius, MinRange, SpawnCardID/SpawnInterval/Lifetime, SpellEffect/EffectDuration/MaxTargets/GoldSteal/ChainTargets/ChainFalloff/SpellDelivery, MaxCopies as the hero stack cap). **Mismatches: 0.** Also checked: default deck sums to 50 (`9+8+4+3+3+3+3+3+3+2+2+2+2+2+1`); the 19 `DeckCount 0` cards named at line 248-250 are exactly the 19 in the CSV; "no per-card copy limit" is true (`CardRow.h:204-212`, `DeckLibrary.cpp:17-19` — `MaxCopies` is the hero stack cap only; `HeroCharacter.cpp:1329-1340`); both bot decks sum to 50 and hold no Fireball, Witch, Fog or Bright Sun (`SiegeBotController.cpp:253-288`). The CSV ↔ `DT_Cards.uasset` sync is accepted as declared (census cl. 1).

## C++ / config / board numbers re-read at the cited line (held unless listed above)

`SummonedUnit.h` 914 · 1426 · 1448 · 1470 · 1474 · 1513 · 1517 · 1549 · 1574 · 1593 · 1603-1615 · 1619-1623 · 1632-1636 · 1649 · 1659 · 1687 · 1700 — `SummonedUnit.cpp` 924-927 · 1635-1638 · 1712-1720 · 1752 · 1785 · 1972 · 2024 · 2049 · 3043 · 3322-3335 · 4255-4277 · 4537-4571 · 4573-4589 · 4679-4695 · 4698-4781 · 4863 · 5526-5545 — `SiegeFogStatics.h` 268 · 325; `.cpp` 122 — `SiegeCombatStatics.cpp` 257 — `Castle.h` 359 · 394 · 563 · 724 · 757-765; `Castle.cpp` 152-153 · 1106-1160 — `ScatterConfig.h` 57-64 · 346 · 356 · 382 · 386 · 399 · 415 · 428 · 440 · 448 · 452 · 462 · 470-476 · 494 · 503 · 552 · 565 · 576 · 588 — `CommanderNpc.h` 295 · 311 — `HeroCharacter.h` 1103 · 1107 · 1118-1122 · 1126 · 1208-1220 · 1224-1236 · 1270 · 1274-1278 · 1285-1305 · 1328 · 1341; `HeroCharacter.cpp` 448-459 · 479 · 683 · 1329-1340 · 1506 — `SiegePlayerState.h` 312-340 — `SiegeGameState.h` 180 — `SiegeGameMode.h` 461 · 536 · 569; `.cpp` 74 · 523-572 · 1628-1646 · 1669-1684 — `SiegePlayerController.h` 451 · 1681-1690 · 1741 · 1745 · 1760-1767 · 1786 · 1866 · 2109 · 2119-2127 · 2131-2139 · 2153 · 2206 · 2235 · 2287-2322 · 2330; `.cpp` 147 · 598-609 · 618-620 · 627 · 636 · 640 · 644 · 652 · 661 · 673 · 690 · 709 · 727 · 974 · 1524 · 1970-1975 · 2765-2789 · 5021 · 5041-5071 · 6074-6079 · 6790-6792 · 7185-7219 — `FogVolume.h` 877-883 · 999 · 1041 · 1059 · 1072 · 1094 · 1139 — `SpellLineSweep.h` 103 · 112 · 121 · 133 — `SiegeInvisibilityStatics.h` 97-207 — `DeckTypes.h` 73 — `DeckComponent.h` 183 · 187 · 192-212 — `CardRow.h` 80-88 · 204-212 · 260 — `DeckLibrary.cpp` 8-25 — `MinerUnit.h` 484; `MinerUnit.cpp` 505-515 — `DeepMine.h` 66 — `Tower.h` 114; `Tower.cpp` 235-244 — `ClimbableTower.h` 464 · 670 · 770; `.cpp` 223 — `Building.h` 451 · 477; `Building.cpp` 412-417 — `Projectile.h` 150 · 159 — `AncientGround.h` 178 · 191; `AncientGround.cpp` 259 — `CaptureZone.h` 190 · 194 · 204 — `GoldNode.h` 236 · 253 · 265-269 — `BattlefieldScatter.h` 211; `.cpp` 293-307 · 1458-1462 · 1861 (half-field fallback) · 2066 (48 attempts) — `SiegeBotController.h` 217 · 221 · 227 · 231 · 235 · 269 · 273 · 376 · 443 · 477; `.cpp` 253-290 · 299-337 · 369-389 · 446-499 · 747-919 · 1622-1639 — `SiegeDeckSaveGame.h` 57 — `SiegeSettingsSubsystem.h` 293 · 296 — `SiegeSessionSubsystem.h` 62 — `SiegeAssistantComponent.h` 1704 · 1713 · 1726 · 1730 — `SiegeAssistantCommand.h` 66-76 — `SiegeGraphicsSettingsSubsystem.h` 150-175 · 642; `.cpp` 29-38 · 77-81 · 87-96 · 735-738 · 843-846 — `SiegeGraphicsMenuWidget.h` 374 · 410 · 418; `.cpp` 41-145 · 378 · 860-871 — `WarMapWidget.h` 1186 · 1292 · 1305 · 1317 — `SiegeAccountSubsystem.cpp` 43-50 — `DeckBuilderWidget.h` 93 · 370; `.cpp` 71-268 · 1240 · 1593 — `SiegeKeyboardLayoutSubsystem.cpp` 75 — `SettingsMenuWidget.cpp` 33 · 56 · 64 — `AccountMenuWidget.cpp` 30-50 — `SiegeControlsHelpWidget.cpp` 71-72 · 350-353 · 1168-1175 — `SiegeGhostPawn.cpp` 507 · 519 — `SpellLibrary.cpp` 344-383 · 522-571 · 686-697 · 740-745 — `SorcererUnit.cpp` 23 — `SiegeCloudClient.cpp` 108-115 — `Config/DefaultGame.ini` 19 · 26 · 29 — `Config/DefaultEngine.ini` 10-12 · 207-210 — `Config/DefaultInput.ini` 63-64 · 84-85 — `Tools/fetch_llm_model.py` 8 — `Plugins/SiegeLlama … SiegeLlamaSettings.h` 72 · 74; `SiegeLlamaSubsystem.cpp` 1994 — `TASKBOARD.md` 1600 (~181 s) · 1637 (0.3399) — `CONVENTIONS.md` 7043 · 7264 · 10026-10028 — UE 5.8 `Build.h` 214-215 (`ALLOW_CONSOLE_IN_SHIPPING 0`) · `CheatManagerDefines.h` 9 — `Docs/GDD.md` (the 15 lines in the divergence table).

The three census replacements (Addendum A12 `SpellLineSweep.h:103/112/121/133`, A13 `SiegeInvisibilityStatics.h:97/125/137/157`, A14 `TASKBOARD.md:1600/1637`) all resolve. Drifted citations found beyond those: the BLOCKER 1-2 pair and the WARN list above.

## Placeholder census (counted by grep, `-o`)

15 occurrences, 9 distinct: `{{SHIP:CONFIG}}` ×3 (16, 19, 162) · `{{SHIP:ZIP_NAME}}` ×1 (18) · `{{SHIP:DATE}}` ×1 (19) · `{{SHIP:HEAD}}` ×2 (20, 197) · `{{SHIP:DIFF_BASE}}` ×2 (20, 198) · `{{SHIP:SIZE}}` ×2 (21, 75) · `{{SHIP:VERIFIED}}` ×1 (188) · `{{SHIP:CHANGED_SINCE}}` ×1 (202) · `{{SHIP:CLOUD_SYNC}}` ×2 (267, 920). `{{SHIP:NOT_MEASURED:…}}` = 0. All eight `SHIP-§3a` placeholders present verbatim; every one has a named resolver in `handoffs/TASK-1191-programmer.md` §2. No per-ship value was hand-typed (the only dates/hashes in visible text are the previous package's, which are facts).

## `{{SHIP:CHANGED_SINCE}}` draft (lines 204-235)

Ruling: accurate and complete enough for a player; player-facing voice; **0 task IDs and 0 law citations in visible text** (grep `TASK-\d+|SC-§|SHIP-§|PKG-§|FOG-§|WITCH-§|J-W\d+|J-F\d+` hits only HTML comments at lines 2, 11-13, 666). Cross-checked against the reviewer's record of the period (TAB controls, height advantage, Watch Tower + contact climb, ranged ×3, order circles / Recall / ghost, tower stacking + wheel, card-bar buttons + the Play Again oval, Witch, Fog + Bright Sun + the render floor, the knight, the trees + grass, Settings → Graphics). The 56-commit `git log` completeness is accepted as declared (no git); the ship's own check line fills the placeholder above the list, as the handoff intends. One optional addition already covered elsewhere in the document: the war-map reveal now excludes veiled units.

## Rules of the document

- `SC-§109`: the assistant section (128-156) and *What is NOT in this build* (195-278) state what to observe; the one arguable sentence is NIT 3. The chat box's opening in Shipping is explicitly not claimed.
- Inert never listed as playable: Witch is listed as a card (she works when played) and labelled "never come from the opponent" with `DeckCount 0`; the bot's Fireball rule is absent from the rule table and named in *NOT in this build*; per-card discard is one line there, absent from *Other features*; console/cheats: one sentence, nothing listed. Multiplayer carries the census's exact label and never says two people fight.
- `PKG-§11`: HEAD/DIFF_BASE, the changed list, the fog cycle (181 s / 0.3399, observation phrasing), the keep-or-revert never watched, no console, the inert items, board-not-tree work — all present with reasons.
- `PKG-§8`: "Siegebound" as the displayed identity (`DefaultGame.ini:19/26`); the template-name apology is gone; the residue (exe name) stated honestly.
- Click target `Windows\GitClaudeUnrealTest.exe` named exactly once (line 50); the Shipping binary named once (line 57).
- "Development" appears only for the previous package or dated measurements (lines 20, 76, 119, 163, 167, 198); no Shipping fact is mis-labelled.
- All ten original sections present in the original order.

## Accepted as declared (`SC-§71b` — the reviewer holds no instrument for these)

1. `cards.csv` ↔ `DT_Cards.uasset` cell-for-cell sync (census cl. 1; `.uasset` + git history).
2. `IMC_Hero.uasset` = 28 mappings and the per-key IA names (asset decode; the handler lines were verified in code).
3. `DA_BattlefieldScatter.uasset` layer counts / spacings / blocking flags (asset decode).
4. `WBP_MainMenu` / `WBP_SessionMenu` / `WBP_DeckBuilder` / `WBP_HUD` label strings (asset name tables).
5. `Tools/Packaging/ship.ps1` line citations at HEAD (the working copy is dirty).
6. `git log 22728c8..8c444ca` = 56 commits; the previous package's Archer/Longbowman/Wizard ranges (700/1200/700).
7. `L_Arena` castle and player-start transforms (code fallbacks cited; level not decoded).

## Notes for build-master (apply once TASK-1191 clears the loop and this row reads PASS)

- The render is a by-hand step (no script does it): fill all 9 placeholders from handoff §2, then grep the rendered `packagedZIPofGame/README.md` for `{{SHIP:` and require 0 before Phase D.
- `{{SHIP:CLOUD_SYNC}}`: inspect the stage for `Config/SiegeCloudDev.ini` and pick the handoff's matching sentence.
- `{{SHIP:CHANGED_SINCE}}`: replace with the check line; keep the draft list beneath it.
- Model size: `Get-Item Models\Qwen3-4B-Q4_K_M.gguf` if present; otherwise leave "about 2.5 GB".
- Confirm `git show HEAD:Tools/Packaging/ship.ps1` at 836 / 841 / 1090-1091 / 1174-1188 / 2835-2839 still says what the README cites, or re-pin those comments to symbols.
- The README source, census, handoff and this report ride Phase F's commit (`SHIP-§3a` identity hazard) — never a commit between invocation 1 and the resume.

---

# LOOP 2 — diff-read of `TASK-1191` loop 1 (2026-09-09)
LOOP 2 Verdict: **PASS** — 0 BLOCKER / 1 WARN (outside the README — a board pathspec) / 3 NIT. The ship may render `Docs/Packaging/README-source.md` at Phase E.

Subject: `Docs/Packaging/README-source.md` (946 lines, 308 `<!-- src:` markers — unchanged from loop 1) against `handoffs/TASK-1191-programmer.md` § Loop 1. Reviewer held Read/Grep/Glob only (`SC-§71b`). Every replacement line below was re-read at the source file by me; the handoff was the claim, the file is the proof (`SC-§101`). README line numbers in this section are the **post-edit** numbers (the veil sentence grew 2 → 4 lines and the GDD paragraph +1, so everything after `:474` sits +2/+3 from loop 1's numbers).

## Loop-1 blockers — each verified at source

| # | README (now) | Cited | Source line reads | Holds |
|---|---|---|---|---|
| 1 | `:718-719` Follow "within **900**" | `SiegePlayerController.h:451; :2569-2583` | `:451` `float FollowFormationRadius = 900.f;` (doc `:444-449` "Radius of the ring the following squad spreads in around the hero"); `:2569` `CancelGroupPick()`, `:2583` `OnCmdFollowPressed()` | yes |
| 2 | `:712-715` Hold **1200 / 700 / 1500** | `SiegePlayerController.h:2131; :2135; :2139; :1684` | `:2131` `GroupSelectRadiusDefault = 1200.f;` · `:2135` `GroupPositionRadiusDefault = 700.f;` · `:2139` `GroupAttackRadiusDefault = 1500.f;` · `:1684` `void OnCmdHoldPressed();`. The `SiegeAssistantComponent.h:1726-1730` mirror is gone from the document (grep = 0). | yes |
| 3 | `:471-474` the veil sentence | 7 enum lines + 8 sites | `SiegeInvisibilityStatics.h:125` `Attack,` · `:137` `Heal,` · `:157` `Mine,` · `:176` `Empower,` · `:195` `Cast,` · `:206` `Death` (enum opens `:97`). Sites: `SummonedUnit.cpp:4180` (ranged, before `FireProjectileAt`) · `:4210` (melee, before `ApplyDamage`) · `:5584` (`ApplyDetonation`) all `Attack` · `:3039` `Heal` (in `PerformHeal`, after every early-out) · `MinerUnit.cpp:532` `Mine` (inside the claim's success branch — a refused/queued miner stays veiled, `:526-528`) · `AncientGround.cpp:259` `Empower` (the `IsAncientGroundEmpowerer()` arm at `:234`, which `continue`s at `:260` before the recipient loop — the **counted Sorcerer** breaks, recipients stay veiled, comment `:240-258`) · `SummonedUnit.cpp:3607` `Cast` (after `GrantInvisibility()` at `:3597`; the ineligible-subject path returns at `:3588` without reaching it, comment `:3603-3604` "an INTERRUPTED cast produces no veil … and therefore no break") · `:5621` `Death` (in `HandleDeath`, after `ApplyDetonation` at `:5604` so a veiled Sapper logs `Attack`, comment `:5618-5620`). | yes |

**Exhaustive-in-fact, measured:** grep `ESiegeVeilBreakReason::` across `Source/**` excluding `Tests/` = exactly the eight sites above plus the `ToString` switch (`SiegeInvisibilityStatics.cpp:95-100`) and header comments. Grep `bIsInvisible\s*=\s*false` = one hit, `SiegeInvisibilityStatics.cpp:73`, inside `ApplyBreak` (the one door, `:68-73`). Grep `GrantInvisibility(` = one caller, `SummonedUnit.cpp:3597` (the Witch's completed cast) — so "only a new Witch cast re-veils" is also exhaustive in fact. All six enumerators are live; there is no seventh way in or out.

### `SC-§109` ruling on the rewritten sentence (`:471-474`) — PASS
> "The veil breaks **permanently** when the veiled unit acts: it attacks, heals, mines, stands on an ancient ground as a Sorcerer (it is counted there every second), or — as a Witch — completes a veil cast of its own; death ends it as well. It never restores itself — only a new Witch cast re-veils."

- It describes what happens (six observable events and one non-event), and draws no conclusion for the player — no "so you should…", no "this means…".
- It is exhaustive in form ("when the veiled unit acts: …, or …") **and now exhaustive in fact**: six enumerators, eight call sites, one door, one grant — all measured above. Loop 1's blocker 3 was "exhaustive in form, three of six"; that gap is closed.
- Every qualifier is code-true: "as a Sorcerer" (the arm is `IsAncientGroundEmpowerer()`, `AncientGround.cpp:234`), "counted there every second" (`AncientGround.h:191` `BoostTickInterval = 1.0f`), "completes a veil cast" (`:3603-3604`), "death ends it as well" is stated as an ending rather than an act, matching J-W4's "cleared, not broken" (`SiegeInvisibilityStatics.h:198`).
- One deliberate residual the sentence carries correctly: a lone veiled Sorcerer on an ancient ground un-veils with nobody to boost (`AncientGround.cpp:251-258`, declared residual). "stands on an ancient ground as a Sorcerer" says exactly that — standing is the act — so the sentence does not over-promise.

## Loop-1 WARN citation moves — spot-checked by reading

| README (now) | New cite | Source line reads | Holds |
|---|---|---|---|
| `:245-246` bot Fireball never fires | `SiegeBotController.cpp:253-290; :747-809` | `:747` `// ---- Rule 3: SPELLS … 3a Fireball …`; `:805` `return; // rule 3 fired`; `:807-809` close 3a | yes |
| `:470` skipped by targeting / war-map reveal | `SummonedUnit.cpp:1850; SiegePlayerController.cpp:7218-7219` | `:1850` `GatherHostileAgents(World, Team, HostileAgents, ESiegeVeilPolicy::SuppressVeiled, &Vision)`; `:7218-7219` `… \|\| !FSiegeCombatStatics::IsAgentVisibleTo(OwnTeam, Unit)` | yes |
| `:737` rule 3 Lightning | `:811-853` | `:811` `// 3b) LIGHTNING at a player tower …`; `:853` closes the rule-3 block | yes |
| `:738` rule 4 Attack | `:856-907` | `:856` `// ---- Rule 4: ATTACK …`; `:907` closes | yes |
| `:739` rule 5 Discard | `:909-944` | `:909` `// ---- Rule 5: CYCLE …`; `:940` `return; // rule 5 fired`; `:942` closes; `:944` is the "No rule fired" comment | yes (range over-covers by 2 lines — NIT 1) |
| `:753` and `:929` no bot in a networked match | `SiegeGameMode.cpp:1678-1684` (both sites) | `:1678` `if (bNetworkedMatch)` … `:1683` `return;` `:1684` `}` | yes |
| `:729` "no hero" | `SiegeGameMode.cpp:1669-1765; SpellLineSweep.h:124-125; SiegeBotController.cpp:772-773` | `SpellLineSweep.h:124` "(the bot has no hero — its line spells fire from its own castle"; `Bot.cpp:773` "BOT'S CASTLE toward it (the bot has no hero — flagged design default)" | yes |
| `:901` *Custom* | `SiegeGraphicsSettingsSubsystem.h:150-163` | `:163` `static constexpr int32 CustomQualityLevel = -1;` | yes |

Old strings gone (grep = 0 each): `SummonedUnit.cpp:1806`, `SiegePlayerController.cpp:7185-7195`, `:701-733`, `:735-766`, `:769-800`, `:804-810`, `SiegeGameMode.cpp:1719`, `SiegeBotController.h:217`, `SiegeAssistantComponent.h:1726`, `SiegeGraphicsSettingsSubsystem.h:150-162`.

## The sweep's two additional finds — verified

- `:676` Shift "sprint at **750**" → `HeroCharacter.h:1107` `float SprintSpeed = 750.f;` (handler `HeroCharacter.cpp:448-459` `StartSprint`/`StopSprint` kept as landmark). Holds.
- `:688` H "**20** gold and draw **6**" → `SiegePlayerController.h:1786` `int32 DiscardAllCost = 20;` · `DeckComponent.h:183` `int32 HandSize = 6;` (handler `PC.cpp:727` `BindAction(DiscardAllAction …)` kept). Holds.

### Ruling on the sweep's sufficiency
The sweep (542 → 560 tokens re-resolved mechanically; 94 no-hit tokens triaged by hand; 153 single-line `.h/.ini/.py` cites read by eye) **closes the `SC-§110` false-literal class for this document at this HEAD** — a visible number whose *only* citation does not carry it. It cannot see two adjacent shapes, and both should be named so nobody reads "sweep clean" as "citations clean":

1. **Per-number coverage on a multi-number row.** The method scores a *token* as hit if *any* number on its row appears in its target lines. A row "1200 / 700 / 1500" with one token carrying 700+1500 and another carrying nothing scores one hit and one triaged behaviour cite — and 1200 resolving nowhere is invisible unless the triage asks the inverse question (is every number on this row carried by some token?). That is exactly loop 1's blocker 2. The handoff's triage description ("sitting beside a number that a sibling token carries") is per-token, not per-number.
2. **Behaviour-range drift beside a number-carrying sibling.** A `.cpp` range that points at the wrong function (loop 1's bot-rule WARN — the `.h` sibling carried every number) is classified "behaviour cite, sibling carries the number" and the triage stops. The sweep has no instrument for *which* behaviour the range lands on.

Both shapes were caught in loop 1 by the ~300-citation by-eye read, and their instances are fixed and verified here; the residual risk after loop 1 + this diff-read is a behaviour landmark inside an HTML comment, invisible to the player. **Sufficient for this ship.** If `sweep_cites.py` is ever promoted into `Tools/`, its spec must assert per-number coverage (every visible number on a row is carried by ≥1 token's target lines), and the bot-rule shape stays a by-eye check — recorded for the manager, not owed by this row.

## Nothing else moved — the diff surface
- Markers: 308 `<!-- src:` (loop 1: 308). Lines 946 (handoff: 943 → 946).
- Placeholders (grep `-o`): 15 occurrences / 9 distinct — `CONFIG` ×3 (16, 19, 162) · `ZIP_NAME` (18) · `DATE` (19) · `HEAD` ×2 (20, 197) · `DIFF_BASE` ×2 (20, 198) · `SIZE` ×2 (21, 75) · `VERIFIED` (188) · `CHANGED_SINCE` (202) · `CLOUD_SYNC` ×2 (267, 923). `{{SHIP:NOT_MEASURED` = 0. Identical to loop 1 modulo the +3 shift on line 920 → 923.
- Archer row `:490` byte-identical to loop 1's table (`12 | 45 | 10 | 2100 (609.6), homing shot | 1.2 s | 350 | 5000 (609.6) | 8000 (609.6) | Standard | 8 | ranged`, `cards.csv:3`). The card tables and *Card by card* lie outside every edited range (the 15 edits are at `:245-246`, `:470-474`, `:676`, `:688`, `:712-719`, `:729`, `:737-739`, `:753`, `:858-862`, `:901`, `:929`); the 34 rows are accepted as unchanged on that basis, not re-verified.
- Click target `Windows\GitClaudeUnrealTest.exe` still named once (`:50`). `:585` "healing breaks its own veil" and `:752` "never veils" unchanged and consistent with the new sentence. Loop 1's spec-clause table and the four NITs stand.

## GDD subsection (`:858-877`) — reads as a player needs
Heading *Where the shipped game differs from its design notes*; the opening now says "the design document itself is not included in this package (see *What was left out*)" — the referent is `:112` `## What was left out of the original project folder, and why`, whose `:121` row lists `Docs/`. A player is told the game follows the code, that the list below is what they would notice, and that the source of the difference is not something they should go looking for. The 12 table rows are untouched from loop 1. Satisfied; loop 1's WARN closes.

## Findings (loop 2)
- [WARN] **`TASKBOARD.md:2015` (TASK-1193 cl. 5 Phase F pathspec) and `:1999` (this row's `names:`) name `qa/TASK-1192.md`; the file on disk is `.claude/pipeline/qa/TASK-1192-report.md`** (loop 1 wrote it under that name and the board's `status:` line cites it correctly). `SC-§102`: a pathspec that matches nothing answers with silence — the gate's own report would ride nowhere while the commit reports success. Outside this row's edit scope (another row's spec); build-master must commit the on-disk name, or the manager corrects the two board references. Not a README defect.
- [NIT] `handoffs/TASK-1191-programmer.md:130` — the rule-5 description places the `return` at `:942` and the braces at `:944`; source has the `return` at `SiegeBotController.cpp:940`, braces closing `:942`, and `:944` is the "No rule fired" comment. The README's range `:909-944` still contains the whole rule — over-covers by two lines; no README change needed.
- [NIT] `handoffs/TASK-1191-programmer.md:110-134` — README line references are mixed pre-/post-edit (`:716-717` is now `:718-719`, `:710-713` → `:712-715`, `:674` → `:676`, `:686` → `:688`, `:733-737` → `:735-739`, `:751/:926` → `:753/:929`, `:898` → `:901`, `:856-859` → `:858-862`). Cosmetic; the README is the artifact.
- [NIT] `handoffs/TASK-1191-programmer.md:112` cites the counted-Sorcerer qualifier at `AncientGround.cpp:233-258`; `:233` is blank, the test is `:234`, the comment `:240-258`. The README cites only `:259`, which is exact.

## The item outside the author's write list — ruling on the census
`handoffs/TASK-1190-census.md:274` (row **R**, `IA_CmdHold`) still cites `PC.cpp:640, PC.h:1684` beside "opens 1200 … (700) … (1500)"; `:276` (row **C**, `IA_CmdFollow`) still cites `PC.cpp:661, PC.h:2569-2583` beside "formation within 900". Both are the false literals loop 1's blockers 1-2 traced upstream, and the census rides Phase F's commit (`TASK-1193` cl. 5 names `handoffs/TASK-1190-programmer.md`; the census file itself is `TASK-1190-census.md` — build-master should check whether it is on the pathspec at all, same `SC-§102` hazard as the WARN above).

**Recommendation: correct the two census rows, do not take the handoff note as the correction of record.** A census that ships in git with known-false citations is the same false-literal defect one document upstream, and the next reader of the census (the row itself says it is the input to the README) has no reason to open `TASK-1191`'s handoff § Loop 1 to learn that two rows are wrong. The edit is two cells: row R → `PC.h:2131 / :2135 / :2139 (+ :1684 the handler)`, row C → `PC.h:451 (+ :2569-2583 the handler)`, dated "corrected at TASK-1192 loop 2". Who: the census is `TASK-1190`'s deliverable and the gameplay-programmer's file; the manager re-issues it as a two-cell rider on the same loop (it is `done`, so a one-line re-open), or the manager makes the edit as the board's owner of record — not the reviewer (out of scope), and not build-master mid-ship. **This is not a blocker on `TASK-1192`** — the README is this row's subject and the README is correct — but it should land before Phase F so the census and README enter git agreeing.

## Notes for build-master (loop 2 — supersede nothing above; add these)
- The README may be rendered at Phase E from this source as it stands at the diff-read (946 lines, 308 markers, 15/9 placeholders).
- **Phase F pathspec: the QA report is `GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1192-report.md`**, not `TASK-1192.md`; and confirm `handoffs/TASK-1190-census.md` is on the pathspec if it is meant to ride (cl. 5 names only `TASK-1190-programmer.md`). Verify the COMMIT (`git show --stat HEAD`) lists both.
- Loop 1's build-master notes (placeholder grep = 0, `CLOUD_SYNC` by stage inspection, `CHANGED_SINCE` check line, model size `Get-Item`, `ship.ps1` HEAD lines) all stand.
