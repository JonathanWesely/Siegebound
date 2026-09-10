# TASK-1191 — handoff (gameplay-programmer, 2026-09-09)

**Deliverable:** `Docs/Packaging/README-source.md` (new, tracked — `SHIP-§3a`). Gate: `TASK-1192`.
**Also written:** `handoffs/TASK-1190-census.md` § Addendum (TASK-1191) — 18 measured entries (A1–A18); this note; my `- status:` line on the board.
**Not touched:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\README.md` (read only — the ship renders it at Phase E), `Source/**`, `Content/**`, `Config/**`, `Tools/**`, no editor, no MCP, no compile, no Git write.
**Git instant:** `HEAD = 8c444ca`, `origin/main...main = 0 0`; last `Source/Content/Config` commit `ea7b4d2` (before HEAD) ⇒ every `Siegebound/…`, `Config/…`, `Docs/Data/…` citation is HEAD content. `Tools/Packaging/ship.ps1` is dirty from the live ship; its citations are to `git show HEAD:…` line numbers and say so.

## 1. Section map (the ten originals kept in order, corrected; the new ones after)

| # | Section | Status |
|---|---|---|
| 1 | Title block (`# Siegebound — Win64 {{SHIP:CONFIG}} build`, zip / built / built-from / size) | kept; *Development* → `{{SHIP:CONFIG}}`; the diff base is now recorded **in** the README (`PKG-§11`) |
| 2 | ⚠️ First, an honest correction | kept verbatim except "a compiled game `.exe`" → "a compiled game program" (two exes now exist) |
| 3 | How to run it | kept; **the click target is named exactly once** (`Windows\GitClaudeUnrealTest.exe`); the Shipping binary name added; the *"Third Person Game Template"* apology **deleted** (`PKG-§8`: `ProjectName=Siegebound`) |
| 4 | Windows will probably warn you | kept verbatim |
| 5 | Minimum requirements | GPU row cited to `DefaultEngine.ini:207-210`; Disk → `{{SHIP:SIZE}}`; RAM = *last measured 2026-08-29* + the 1,640 MB Shipping working set from `ship.ps1:841` |
| 6 | What was packaged | kept; hard sizes removed (→ the size line); Shipping binary wording; plugin ships / model does not |
| 7 | What was left out … and why | kept; the ~8 GB / ~10 GB figures (uncited) replaced by words; model size 2.5 GB cited to `fetch_llm_model.py:8` |
| 8 | About the in-game AI assistant | **re-verified against census 5c / 7:** EXISTS-INERT in the zip (C4-NO-MODELS); says so; `SC-§109` — states what to observe (Enter gives no working assistant; whether the box opens in Shipping is *not claimed*); re-enable path labelled untested |
| 9 | Known notes for this build | rewritten for Shipping: no console, no cheats, log-silent (`CONV:7043`), pdb → size line; save-data path kept; audio note dated to 2026-08-29; positional keys |
| 10 | What was verified before shipping | `{{SHIP:VERIFIED}}` + the standing "not verified by machine" sentence |
| 11 | **What is NOT in this build** (`PKG-§11`) | new: HEAD / DIFF_BASE; `{{SHIP:CHANGED_SINCE}}` + my draft list; the inert/unobserved list with reasons, each as *what to observe*; board-not-tree work by feature |
| 12 | **GameDetails → Gameplay** | new: match paragraph · Gold · Deck and hand · Playing a card · Your hero · How units fight (notice/retention paper **and** fog values) · Keywords and damage rules · The castle · Fog and Bright Sun · Invisibility · The cards (5 tables, all 34) · Card by card (34 entries, shipped glossary text quoted) · Commands (28 bindings + raw inputs + the no-console sentence) · Orders in detail · The AI commander (bot / commander NPC + war map / assistant) · Areas of interest (6 mines, 2 grounds, mid zone, castles, 7 scatter layers) · Winning/losing · Notes for readers of the design document (12 divergence rows) |
| 13 | **GameDetails → Other features** | new: exactly census §7's REACHABLE items; Multiplayer under the census's own label *"EXISTS (host plays; joiner observes)"*; assistant under *Present in the build but not yet usable*; ABSENT items (key rebinding, replay) never mentioned |

## 2. Placeholders — who resolves each

| Placeholder | Occurrences | Resolved by | With |
|---|---|---|---|
| `{{SHIP:ZIP_NAME}}` | 1 | build-master, Phase E | `Siegebound-Win64-Shipping-<date>.zip` |
| `{{SHIP:DATE}}` | 1 | build-master | the ship date |
| `{{SHIP:CONFIG}}` | 3 (title, "Built" line, Known notes) | build-master | `Shipping` — ⚠️ the *Known notes* paragraph describes Shipping facts (no console, log-silent); a future Development ship must edit that paragraph, not just the placeholder |
| `{{SHIP:SIZE}}` | 2 (title block, Disk row) | build-master | extracted size + file count, exe size, pdb size **or its absence**, pak/ucas/utoc, archive size (`PKG-§9d`, `ship.md` step 3) — the Known-notes text points the reader at this line for the pdb |
| `{{SHIP:HEAD}}` | 2 | build-master | measured HEAD at the ship's instant |
| `{{SHIP:DIFF_BASE}}` | 2 | build-master | `22728c8` confirmed (bracket by the 2026-08-29 date) |
| `{{SHIP:VERIFIED}}` | 1 | build-master | what was measured this ship, by which instrument, + the `SHIP-§8b(6)` adjudication observations and who adjudicated |
| `{{SHIP:CHANGED_SINCE}}` | 1 | build-master | ⭐ **the placeholder sits directly above my visible draft list.** Intended render: replace the placeholder with one line stating the check (*"Checked against `git log 22728c8..<HEAD>`, N commits, on <date>; additions: …"* or *"nothing missing"*) and keep the draft list beneath it, appending any player-facing change the log has that the draft lacks. Nothing in the draft should be deleted unless the log does not support it. |
| `{{SHIP:CLOUD_SYNC}}` | 2 (What is NOT in this build; Other features → Login) | build-master, after inspecting the stage | if `Config/SiegeCloudDev.ini` is staged: *"available — Login → Link to Cloud (email + password), then Sync Now; your decks and settings are copied to the cloud account"*; if not: *"not available in this package — the cloud configuration file is not included; accounts are local only"* (census §7: UAT stages every `Config/*.ini` unless a `[Staging]` deny list exists; `DefaultGame.ini` has none; the ini exists gitignored on the cooking machine; **the package was not inspected**) |
| `{{SHIP:NOT_MEASURED:…}}` | **0** | — | none needed: everything the README states as a number is in the census or the addendum |

Total: 9 distinct placeholders, 15 occurrences. All eight `SHIP-§3a` placeholders are present verbatim; `CLOUD_SYNC` is the dispatch's addition.

## 3. Cited numbers

- `<!-- src: … -->` comments in the source: **333** (measured: `grep -o '<!-- src:' | wc -l`; 310 lines carry one or more; opener/closer counts balance 310/310; no `|` inside any comment; the click-target path appears exactly once, line 50; the file is 80,293 bytes / 943 lines); every table row in the five card tables carries one citation to its `cards.csv` line, which covers every cell in that row (the gate re-reads all 34 rows anyway). C++-derived numbers each carry their own `file:line`.
- The **notice / retention columns** in the Units table repeat the same derived values on every row (5000 / 609.6 and 8000 / 609.6); they are cited **once**, in the paragraph above the table and in the *How units fight* section, rather than 26 times — the derivation is one formula, not 26 facts.
- **Spot-check performed before writing (SC-§110):** every C++ line I cite was re-read at HEAD and held its number — `SummonedUnit.h` 914/1426/1448/1470/1474/1513/1517/1549/1574/1593/1603-1615/1619-1623/1632-1636/1649/1659/1687/1700; `SiegeFogStatics.h` 268/325; `Castle.h` 359/394/563/724/757-765; `HeroCharacter.h` 1103/1107/1118-1122/1126/1208-1220/1224-1236/1270/1274-1278/1285-1305/1328/1341; `SiegeGameMode.h` 461/536/558/569; `SiegeGameState.h` 180; `SiegePlayerState.h` 312-340; `SiegePlayerController.h` 1741/1745/1767/1786/1866/2109/2119-2127/2153/2206/2235/2287-2322/2330; `SiegePlayerController.cpp` 147; `FogVolume.h` 877-883/999/1041/1059/1072/1094/1139; `DeckTypes.h` 73; `DeckComponent.h` 183/187; `CommanderNpc.h` 295/311; `ScatterConfig.h` 57-64/346/356/382/386/399/415/428/440/448/452/462/470-476/494/503/552/565/576/588; `AncientGround.h` 178/191; `CaptureZone.h` 190/194/204; `GoldNode.h` 236/253/265-269; `Tower.h` 114; `Tower.cpp` 235-238/244; `MinerUnit.h` 484; `DeepMine.h` 66; `Building.h` 451/477; `ClimbableTower.h` 670/770; `ClimbableTower.cpp` 223; `Projectile.h` 150/159; `BattlefieldScatter.h` 211; `SiegeBotController.h` 217/221/227/231/235/239/269/273/372/376/443/477; `SiegeBotController.cpp` 253-290; `SiegeDeckSaveGame.h` 57; `SiegeSettingsSubsystem.h` 293/296; `SiegeSessionSubsystem.h` 62; `SiegeAssistantComponent.h` 1704/1713/1726-1730; `SiegeGraphicsSettingsSubsystem.h` 150-175/642; `.cpp` 29-38/87-96/735-738/843-846; `SiegeGraphicsMenuWidget.h` 374/410/418/1311; `.cpp` 378/860-871; `WarMapWidget.h` 1186/1292-1317; `SpellLineSweep.h` 103/112/121/133; `SiegeInvisibilityStatics.h` 97/125/137/157; `SiegeAccountSubsystem.cpp` 46/50; `DeckBuilderWidget.h` 93/370; `DeckBuilderWidget.cpp` 71-268 (every glossary string quoted) + 1593; `SummonedUnit.cpp` 4573-4583/4681-4690/3043; `SiegeFogStatics.cpp` 122; `SiegeCombatStatics.cpp` 257; `SiegePlayerController.cpp` 6074-6079; `Docs/Data/cards.csv` 1-35 (all rows read). Lines I cite only on the census's word (not re-read): the `.cpp` behaviour ranges (e.g. `SiegeGameMode.cpp:523-572`, `GoldNode.cpp:114-180`, `SpellLibrary.cpp:344-445`) and the asset name-table citations.
- **Three census citations did not resolve as written and were replaced** (addendum A12–A14): `SpellLineSweep.h:284-314` (file is 190 lines), `SiegeInvisibilityStatics.h:29/41/61` (doc lines, not the enum), and the fog-cycle board lines (`TB:1515`/`1801` drifted; `:1600` and `:1637` hold the numbers now).

## 4. Addendum numbers I had to measure (18)

A1 DX12/SM6 · A2 identity fields · A3 the Shipping click target + binary name · A4 the 1,640 MB working set · A5 the 2026-08-29 RAM line · A6 model size 2.5 GB · A7 log-silence ruling line · A8 Watch Tower stack ×2 · A9 account limits · A10 deck-builder right-click/auto-save · A11 the Support glossary gate · A12 line-spell geometry · A13 veil-break enum · A14 fog-cycle board lines · A15 fullscreen keys · A16 hero-upgrade lines split · A17 the ×1.787 derivation · A18 the 56-commit count. Full citations in the census § Addendum.

⚠️ **A6 discrepancy:** the 2026-08-29 README said the weights are *~2.3 GB*; the only in-tree figure is `fetch_llm_model.py:8` (*~2.5 GB*, a comment). I used 2.5 GB with that citation. Neither is a measurement of the file; the build-master can replace it with `Get-Item` on `Models\Qwen3-4B-Q4_K_M.gguf` if the file is present on the cooking machine.

## 5. EXISTS-INERT / omitted, with reasons

| Item | Treatment | Reason (census) |
|---|---|---|
| Witch, Fog, Bright Sun vs the bot | **listed as playable cards** (they work when you play them) **+ labelled** in *What is NOT in this build*: the bot never plays them | `DeckCount = 0`, absent from both bot decks (`CSV:33-35`, `Bot.cpp:253-290`) |
| Bot Fireball rule 3a | **omitted from the bot's rule table** (a rule that cannot fire is not behaviour); named in *What is NOT in this build* | neither curated deck holds Fireball |
| 19 `DeckCount 0` cards | listed by name as *reachable only through the Deck Builder* | `CSV:2-35` |
| Fog visibility cycle | *What is NOT in this build*, `SC-§109` phrasing ("stand still … see it thin and thicken"), 181 s / 0.3399 cited to `TB:1600` / `TB:1637` | `TASK-1177`/`1184` |
| Graphics auto-revert | described as the feature it is meant to be in *Other features*, **and** listed as never observed, with the observation to make | `TASK-1125` |
| Veil shimmer (J-W18) | listed as what the enemy will see; "a ruling is open" | `CONV:10028` |
| Assistant console | *Present in the build but not yet usable*; section 8 rewritten; Enter row says "inert in this package" | `C4-NO-MODELS` |
| Console / cheats (`§4d`) | one sentence: do not exist in Shipping; **nothing listed**, no heading | `ALLOW_CONSOLE`, `UE_WITH_CHEAT_MANAGER` |
| Per-card discard | omitted from *Other features*; one line in *What is NOT in this build* | `PC.h:1760-1767`, retired |
| Multiplayer | **present, under the census's exact label** *EXISTS (host plays; joiner observes)*; never called playable for two | `PC.cpp:974`, `Building.cpp:415` |
| Cloud sync | `{{SHIP:CLOUD_SYNC}}` ×2 | staging conditional |
| Ghost card placement | listed as unmeasured, with the observation | no code gate found |
| Gamepad | one line: not checked | `IMC_Default` activation untraced |
| Key rebinding UI, footage/replay | **never mentioned** | ABSENT |
| Sandbox (No Bot) | listed as reachable (it is in the menu) | `GM.cpp:1628-1646` |

## 6. Judgement calls QA should scrutinise

1. **The GDD divergences are noted** in one contained subsection (*Notes for readers of the design document*, 12 rows) rather than sprinkled or omitted. The board row (cl. 2) and `SHIP-§3a` require the difference to be *noted*; the dispatch said the README should *not mention the GDD*. I followed the law and the board and confined the note to one place a player can skip; every GDD number in it is cited to its `Docs/GDD.md` line and every shipped number to code. If the gate rules the subsection out, deleting it removes no other claim.
2. **The `{{SHIP:CHANGED_SINCE}}` draft is visible text** beneath the placeholder (not hidden in a comment) so the rendered README can never lose it by a lazy render — the placeholder's intended value is the *check line*, not the list (see §2).
3. **The click target is a path, not a number,** and the census §11 lists exe names as NOT MEASURED *for the package*. I named it from the recipe (`ship.ps1` HEAD `:1090-1091`, `:1174-1188`) and the measured layout law (`CONV:7264`); the ship's `D2-ZIP-READBACK` asserts the click-target exe exists, so the claim is verified by the same run that renders the README.
4. **"18 rows" is not stated** for the Graphics panel; the rows are enumerated (`SC-§104`, and the census declined to count them). "Ten" groups and "five" levels are stated and cited.
5. **The 28-binding count** is stated once and cited to the asset via the census's decode (the gate's `SC-§71b` accepted-as-declared class); the per-row handlers are cited to `PC.cpp` / `HC.cpp`.
6. **Section 2's wording** changed "a compiled game `.exe`" to "a compiled game program" because the Shipping stage holds a launcher and a binary; everything else in that section is verbatim.
7. **RAM row:** two figures, both labelled by date and build; neither is a Shipping measurement of this package.
8. The Units table gives the Cleric's "Damage" as *heals 8 HP/s* and its "range" as the heal radius — the row's cells are 8 / 400 (`CSV:13`), and the code uses them as heal rate / heal radius (`SU.cpp:3043`).

## 7. What the census could not supply (and the README therefore does not state)

- Any measured size of this package (→ `{{SHIP:SIZE}}`); the pdb's presence.
- Whether the packaged exe's Windows file properties read *Siegebound* (the ini chain is cited; the measurement is the ship's — `PKG-§8`; belongs in `{{SHIP:VERIFIED}}` if the build-master opens the Properties dialog).
- The screen-resolution list (engine-enumerated at runtime — described as such, no numbers).
- Foliage cull distances per graphics level (described qualitatively; the authored asset bands were left out because the per-level scale is NOT MEASURED).
- Whether the assistant chat box opens at all in Shipping (`PKG-§9b`) — explicitly not claimed.
- The stuck-unit watchdog thresholds; scatter mesh lists; quality-level display names — not mentioned.

## 8. Slack

⚙️ Dev & QA thread `1783116269.740549` — posted after this note (section list, cited-number count, addendum count, the EXISTS-INERT list).

---

## Loop 1 — after `qa/TASK-1192-report.md` (FAIL, 3 BLOCKER / 9 WARN / 4 NIT) — 2026-09-09

**Scope of this pass:** `Docs/Packaging/README-source.md` only (14 exact-string edits, 15 replacements, every one asserted to occur exactly once before writing — the patch aborts otherwise). No `Source/**`, `Content/**`, `Tools/**`, `Config/**`, no editor, no compile, no Git. Every replacement line below was **re-read at source by me** before citing it (`SC-§101` — QA's lines were the claim, the file is the proof). Placeholders after: 15 occurrences / 9 distinct, unchanged. `<!-- src:` markers unchanged (every edit is inside an existing comment; none added, none removed). Lines 943 → 946.

### Blockers — before / after, with the verified source line

| # | README | Before | After | Verified at source |
|---|---|---|---|---|
| 1 | Follow "within **900** units" (`:716-717`) | `SiegePlayerController.h:2569-2583` (the `CancelGroupPick` / `OnCmdFollowPressed` declarations — no number) | `SiegePlayerController.h:451 (FollowFormationRadius); :2569-2583 (the C handler)` | `:451` = `float FollowFormationRadius = 900.f;` — its doc (`:444-449`) is "Radius of the ring the following squad spreads in around the hero" |
| 2 | Hold "**1200** / **700** / **1500**" (`:710-713`) | `SiegePlayerController.h:1684; SiegeAssistantComponent.h:1726-1730` (`:1684` = `void OnCmdHoldPressed();` — no number; the assistant lines are self-declared *mirrors* and 1200 resolved nowhere) | `SiegePlayerController.h:2131 (1200); :2135 (700); :2139 (1500); :1684 (the R handler)` — the assistant mirror dropped | `:2131` `GroupSelectRadiusDefault = 1200.f;` · `:2135` `GroupPositionRadiusDefault = 700.f;` · `:2139` `GroupAttackRadiusDefault = 1500.f;` |
| 3 | veil-break sentence (`:471-472`) | *"The veil breaks **permanently** when the unit attacks, heals or mines; it never restores itself — only a new Witch cast re-veils"* cited `SiegeInvisibilityStatics.h:97; :125; :137; :157` — exhaustive in form, three of six | see *The veil sentence as rewritten* below | `ESiegeVeilBreakReason` (`SiegeInvisibilityStatics.h:97-207`) = `Attack :125`, `Heal :137`, `Mine :157`, `Empower :176`, `Cast :195`, `Death :206`. **All six are wired in shipping source** (grep `ESiegeVeilBreakReason::` outside `Tests/`): Attack `SummonedUnit.cpp:4180`, `:4210`, `:5584` · Heal `SummonedUnit.cpp:3039` · Mine `MinerUnit.cpp:532` · Empower `AncientGround.cpp:259` (fires on the **counted Sorcerer** standing in the ground — the recipients stay veiled, per the comment at `:233-258`) · Cast `SummonedUnit.cpp:3607` (placed after the cast **completes**; an interrupted cast does not break, `:3603-3604`) · Death `SummonedUnit.cpp:5621` (after the Sapper's detonation so a veiled Sapper logs `Attack`, `:5618-5620`). So the sixth reason QA left optional **is** live, and is stated as "death ends it as well" rather than listed as an act. |

Source of blockers 1-2: census `handoffs/TASK-1190-census.md` §4a rows **C** and **R** carry these same wrong citations. ⚠️ The census is **not** in this loop's write list, so I did not edit it — the manager / build-master should correct rows C and R (to `PC.h:451` and `PC.h:2131/:2135/:2139`) before the census rides Phase F's commit, or accept this note as the correction of record.

### The veil sentence as rewritten (`:471-474`)

> - The veil breaks **permanently** when the veiled unit acts: it attacks, heals, mines, stands on an ancient ground as a Sorcerer (it is counted there every second), or — as a Witch — completes a veil cast of its own; death ends it as well. It never restores itself — only a new Witch cast re-veils `<!-- src: Siegebound/SiegeInvisibilityStatics.h:97; :125; :137; :157; :176; :195; :206 (all six reasons); Siegebound/SummonedUnit.cpp:4180; :4210; :5584 (Attack); :3039 (Heal); Siegebound/MinerUnit.cpp:532 (Mine); Siegebound/AncientGround.cpp:259 (Empower); Siegebound/SummonedUnit.cpp:3607 (Cast); :5621 (Death) -->`.

"counted there every second" = `AncientGround.h:191` `BoostTickInterval = 1.0f`, already cited at `:801-805`. Line `:583` ("healing breaks its own veil") and `:750` ("never veils") are unchanged and consistent.

### WARNs fixed in the same pass (each target re-read)

| README | Before | After | What the new line carries |
|---|---|---|---|
| `:470` | `SummonedUnit.cpp:1806` (a comment about the team compare) · `SiegePlayerController.cpp:7185-7195` (comment) | `SummonedUnit.cpp:1850` · `SiegePlayerController.cpp:7218-7219` | `:1850` `GatherHostileAgents(…, ESiegeVeilPolicy::SuppressVeiled, &Vision)` · `:7218-7219` the reveal loop's `!FSiegeCombatStatics::IsAgentVisibleTo(OwnTeam, Unit)` guard |
| `:245-246` (bot Fireball never fires) | `SiegeBotController.cpp:701-733` (rule 2b Deep Mine) | `:747-809` | `:747` `// ---- Rule 3: SPELLS … 3a Fireball …`, 3a returns at `:805`, the 3b comment opens `:811` |
| `:735` rule 3b Lightning | `:735-766` | `:811-853` | `:811` `// 3b) LIGHTNING at a player tower …` → `:853` closes the rule-3 block |
| `:736` rule 4 Attack | `:769-800` | `:856-907` | `:856` `// ---- Rule 4: ATTACK …` → `:907` |
| `:737` rule 5 Cycle | `:804-810` | `:909-944` | `:909` `// ---- Rule 5: CYCLE …` → `:942` `return; // rule 5 fired`, braces close `:944` |
| `:751` and `:926` "no bot in a networked match" | `SiegeGameMode.cpp:1719-1725` (the bot-class fallback) | `:1678-1684` (both sites) | `:1678` `if (bNetworkedMatch)` … `:1684` `return;` |
| `:727` "no hero" | `SiegeBotController.h:217` (`BotTeam = Red`) + `SiegeGameMode.cpp:1669-1765` | `SiegeGameMode.cpp:1669-1765 (SpawnBot spawns a controller only); SpellLineSweep.h:124-125; SiegeBotController.cpp:772-773` | both added lines read "the bot has no hero" |
| `:898` Custom quality | `SiegeGraphicsSettingsSubsystem.h:150-162` | `:150-163` | `:163` `static constexpr int32 CustomQualityLevel = -1;` |
| `:856-859` GDD subsection (QA: KEEP, optional reword) | heading *Notes for readers of the design document* | heading *Where the shipped game differs from its design notes* + the clause "the design document itself is not included in this package (see *What was left out*)" | `:121` already lists `Docs/` as left out |

Rule 1 (`:433-490`) and rule 2 (`:496-610`) ranges were left as they are — QA ruled the overlap acceptable, and `:446` / `:556` are the rule headers inside those ranges.

### The sweep (the `SC-§110` shape: a citation beside a number whose target line does not carry the number)

- **Mechanical pass** (`sweep_cites.py`, in the session scratchpad, not the tree): every `<!-- src: -->` marker parsed into `file:line[-line]` tokens — **542 tokens before the patch, 560 after** (the +18 are the tokens this loop added); 0 unresolved paths. For each token, the visible numbers on its row/sentence were searched in the target lines; 94 tokens had no hit. **All 94 were triaged by hand:** every one is a *behaviour* citation (a handler, a refusal path, a format string with `%s`, a derivation such as 2 × 840 = 1680 or 0.25 s = "4 times a second") sitting beside a number that a **sibling token on the same row** carries — e.g. `PC.cpp:1970-1975` (the Miner refusal) beside `SiegePlayerState.h:332` (`MaxActiveMiners = 6`). None is the shape.
- **By-eye pass:** every **single-line** citation into a `.h` / `.ini` / `.py` file — **153 distinct `file:line`** — printed with its source text and read. Every literal is on its line. The only misses were QA's blocker 2 (`PC.h:1684`) and WARN (`Bot.h:217`), both fixed above.
- **Found beyond QA's two and fixed (co-cited, nothing reworded):** `:674` Shift row "sprint at **750**" cited only the handler `HeroCharacter.cpp:448-459` → added `HeroCharacter.h:1107` (`SprintSpeed = 750.f`); `:686` H row "**20** gold and draw **6**" cited only the handler `PC.cpp:727` → added `SiegePlayerController.h:1786` (`DiscardAllCost = 20`) and `DeckComponent.h:183` (`HandSize = 6`).
- **Noted, not changed (not the shape — uncited restatements of numbers cited upstream):** `:553-554` Archer entry "1500 units/s … 2100 … 609.6" (cited at `:403-404` / `:488` / `:384`).
- **Count moved:** 14 `src` comments edited (E1–E12 ×2, E14) + 1 heading/paragraph (E13). Citation tokens re-pointed: 11; tokens added: 18; tokens removed: 2 (`SiegeAssistantComponent.h:1726-1730` mirror, `SiegeBotController.h:217`).

### Untouched by instruction (QA passed them; a diff-read should see no hunk)

The archer example, all 34 card rows and the 34 *Card by card* entries, the other rows of the 28-binding table, the placeholders, `SC-§109` phrasing, the four NITs (`:492` "once", `:440-441` "about 25000", `:146`, `:812`), the `ship.ps1` HEAD-pinned lines (build-master confirms `git show HEAD:` or re-pins to symbols), the 2.5 GB model figure (build-master `Get-Item` if present), `{{SHIP:CLOUD_SYNC}}` (build-master after inspecting the stage).

Slack: ⚙️ Dev & QA thread, one line posted for loop 1.
