# TASK-563 handoff — [WR-9] the WAR-MAP CONTROLLER SURFACE — input, proximity, and the 30-gold reveal (gameplay-programmer, 2026-08-15)

- **Gate:** `.claude/pipeline/qa/TASK-565.md` — ⛔ **this task names TASK-565 as its gate** (build-master refuses to commit without this naming).
- **Compile:** TASK-566. **Commit:** TASK-570. ⛔ **This task opened NO compile of its own.**
- **Status:** `ready-for-qa`.
- **Discipline honoured:** ⛔ no compile · no build · no Git · **no editor (it IS up this session and I did not touch it)** · no MCP · no PIE · no `Content/` asset · no `.csv` · no `Build.cs` · no `Tests/` · no `L_Arena`. ⛔ **No token figure is quoted, derived or reasoned from anywhere in this handoff or in either file** (`AS-§12g`, batch-wide ban) — sweep in §7.
- **Files touched — exactly the two in `names:`:**
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegePlayerController.h`
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegePlayerController.cpp`
- ⛔ **NOT touched (the spec's list, verified by `git diff --stat`):** `SiegeAssistantComponent.{h,cpp}` · `SiegeAssistantConsoleWidget.{h,cpp}` (TASK-561 owns it) · `CommanderNpc.{h,cpp}` · `Castle.{h,cpp}` · `SiegePlayerState.{h,cpp}` (⭐ **the shipped API is CALLED, never extended — no new gold entry point exists**) · `WarMapWidget.{h,cpp}` · `SiegeNetLimits.h` · `SiegeGameMode.{h,cpp}` · `SiegeBotController.{h,cpp}` · any Zone builder · any `.csv` · any `Content/` asset · `Build.cs` · `Tests/`.
- ⚠️ **`git diff` on `SiegePlayerController.h` ALSO shows `SpawnBoxHalfExtent (2460,2460) → (7380,7380)`. ⛔ THAT LINE IS TASK-557's, NOT MINE** — it is uncommitted in the shared working tree and the diff is against `HEAD`. My header edits are additive and disjoint from it.
- **Assets referenced by path only (none created, none required to exist today):** `/Game/Input/Actions/IA_WarMap` and `/Game/UI/WBP_WarMap` (**both TASK-568; both absent today, and the code is written to be fully functional without either**).

---

## 1. ⭐ THE HEADLINE — WHAT THE FEATURE NOW DOES END TO END

| step | what runs | law |
|---|---|---|
| press **M** | `IA_WarMap` → `OnWarMapPressed()` | `WR-§5` input row |
| already open? | `CloseWarMap()` — ⛔ **never gated** | the console key's contract |
| **proximity** | `IsHeroInCommanderRange()` → own-team `ACommanderNpc::IsPlayerInRange`. Refuse ⇒ **HUD line naming the reason**, ⛔ cursor untouched | spec (2) |
| **mode exclusion** | `SetWarMapOpen(true)` re-gates on `CanOpenWarMap()` | `WR-§6` |
| show | `GetOrCreateWarMapWidget()` → `OpenMap()`, then **ask the widget** whether it really opened | the console's post-hoc check |
| **click a marker** | `OnPlacePicked(FName)` → **open the console through the controller path FIRST** → `AppendToInput(symbol)` → **check the `bool`** | `WR-§6` + TASK-561's contract |
| **click reveal** | local affordability pre-check (message) → `HasAuthority()` ? `PerformEnemyReveal()` : `ServerRequestEnemyReveal()` | `WR-§7` |
| authority | read `EnemyRevealCost` off the NPC → `SpendGold` → survey enemy units+hero → `ClientReceiveEnemyReveal(WORLD XY)` | `WR-§7`, M8 RPC law |
| close / match end / **Play Again** | `CloseWarMap()` ⇒ the widget **discards the paid reveal, unconditionally** | `WR-§7`, `WR-§9` outcome 4 |

---

## 2. ⛔⛔ THE SIX INHERITED CONTRACTS — EACH DISCHARGED, WITH THE EVIDENCE

### (1) ✅ `ACommanderNpc` IS **NOT** AN `ITeamAgent`. I USED `GetCommanderTeam()`-ERA API ONLY AND NEVER CAST.

```
$ grep -nE "ITeamAgent|GetTeamId\(\)" <my added lines only>
(the only GetTeamId() calls I added are on ASummonedUnit and AHeroCharacter — both real ITeamAgents)
```
⛔ **`Cast<ITeamAgent>(Npc)` appears NOWHERE.** The NPC is reached only through `ACommanderNpc::FindCommanderNpcForTeam(World, Team)` → `IsPlayerInRange(...)` / `GetEnemyRevealCost()`, all named character-for-character off TASK-559's handoff §8. ⭐ **I did not need the NPC's team accessor at all** — the finder takes the team as a parameter, so `GetCommanderTeam()` never appears either and there is nothing to mis-spell. **The team I pass comes from `ASiegePlayerState::GetTeam()`, ⛔ never guessed** (no PS ⇒ no NPC ⇒ the gate refuses, rather than defaulting to Blue and gating the Red player on the ENEMY's commander).

### (2) ✅ TASK-561's REFUSAL-WHEN-CLOSED CONTRACT — CONSOLE OPENED FIRST, THROUGH THE POSTURE-OWNING PATH, AND THE `bool` IS CHECKED.

`HandleWarMapPlacePicked` runs, in this order: `EnsureAssistantConsoleOpen()` → **bail with a Warning if it returns false** → resolve the widget → `AppendToInput(PlaceSymbol.ToString())` → **bail with a Warning if it returns false**. ⛔ **Nothing is submitted** — the player still sends the sentence himself (`WR-§6`).

### (3) ✅ `OnPlacePicked` IS BOUND — BY THIS TASK, AND THIS IS THE FIRST BINDING IT HAS EVER HAD.
`GetOrCreateWarMapWidget()` takes all three: `OnMapOpenChanged` (posture) · **`OnPlacePicked`** (the symbol seam) · `OnRevealButtonClicked` (the purchase). Bound exactly once — that function is the only creation site and early-outs when the widget exists — and all three are `RemoveDynamic`'d in `EndPlay`.

### (4) ✅ `ClientReceiveEnemyReveal` CARRIES **WORLD XY**, NOT UV.
`PerformEnemyReveal` emplaces `Location.X, Location.Y` straight off `GetActorLocation()` — no projection, no `ArenaHalfExtent`, no panel size anywhere in this file. ⭐ **The projection therefore has exactly ONE owner (`FSiegeWarMapProjection`, inside the widget), shared byte-for-byte with the ally dots**, so red and blue can never disagree about where the arena is. Stated in the RPC's own doc comment as well as here.

### (5) ✅ **BOTH TRAILING DEFAULTS PASSED EXPLICITLY** at `UWarMapWidget::CreateAndAddToViewport`.
```cpp
WarMapWidget = UWarMapWidget::CreateAndAddToViewport(
    this,
    TSubclassOf<UWarMapWidget>(ResolvedMapClass),   // ← was default nullptr
    WarMapWidgetZOrder);                             // ← was default 0
```
`ResolvedMapClass` = `WarMapWidgetClass.LoadSynchronous()`, a new `TSoftClassPtr<UWarMapWidget>` defaulted to `/Game/UI/WBP_WarMap.WBP_WarMap_C`. **Null today (TASK-568 has not run) ⇒ the widget's own fallback to the C++ class, which paints and hit-tests everything — ⛔ not a degradation to "fix".** Full `SC-§33` ledger in §6.

### (6) ✅ THE CURSOR/POSTURE GAP TASK-444 FLAGGED IS CLOSED THE WAY 444 SAID TO — I DID NOT RE-DERIVE IT.
TASK-444 §5 flag (a) item (iii), quoted in the code at the site: *"one `|| bWarMapOpen` term in `ApplyCursorInputState()`'s `bWantCursor` composition — that last one is how the law says a new posture owner is added, and it composes with placement / targeting / group-pick / `IA_UICursor` rather than fighting them."* Done literally:
```cpp
const bool bWantCursor = bInPlacementMode || bInTargetingMode || (GroupPickStage != EGroupPickStage::None)
                       || bAssistantConsoleOpen || bWarMapOpen || bUICursorHeld;
```
⛔ **`SetInputMode` is called from NOWHERE new** — `ApplyCursorInputState` remains the ONLY owner of the match cursor posture (the level-travel law). Without this one term the map would look perfectly painted and be **entirely inert** (no visible cursor, no `GameAndUI`, no hit test).

### ✅ AND THE TWO STALE `2460` CROSS-NOTES — **VERIFIED AT THE CODE FIRST, THEN FIXED**
The claim was checked before the sentence was touched: the live value is `SiegePlayerController.h`'s `SpawnBoxHalfExtent = FVector2D(7380.f, 7380.f)`, so both prose claims were false.
- `SiegePlayerController.cpp` **`UpdatePlacementGhost`** (placement-validity v4 comment) — *"half-extent SpawnBoxHalfExtent (2460: covers the castle's walkable interior, TASK-349)"* → now names **7380 since TASK-557 (`WR-§2` row 1, the 9× castle)** and records 2460 as the retired 3× value.
- `SiegePlayerController.cpp` **`IsPointInOwnSpawnBox`** — *"2460 since TASK-349 — the box covers the 3× castle's walkable interior"* → now **7380 / 9× castle**, with the retired value kept and the ⛔ **3-way pairing** (`ACastle` ≡ this ≡ `ASiegeBotController`) spelled out at the site.
✅ **Comment-only, zero emitted bytes** — the `WR-§2b` row G rule ("fixed BY THE TASK ALREADY IN THAT FILE"), applied.

---

## 3. ⚠️ DECLARED DEPARTURES AND ADDITIONS (`SC-§15`) — FIVE, EVERY ONE NAMED

### D-1 ⭐⛔ **THE MAP AND THE CONSOLE ARE *NOT* MUTUALLY EXCLUSIVE — AND THAT IS A REFUSAL OF THE OBVIOUS READING OF SPEC ITEM (1), MADE ON A MECHANISM**

Spec (1): *"opening the map must cancel/refuse alongside targeting mode, placement mode and hold-target mode."* Those three I joined exactly. **The console is deliberately NOT in the set, in EITHER direction:** `CanOpenWarMap()` has no `bAssistantConsoleOpen` clause, and `CanOpenAssistantConsole()` was **not** given a war-map clause.

⚖️ **Why, and it is a deadlock argument rather than a taste one:** a marker click's entire purpose is to write a place symbol into the **console's input box** (`WR-§6`). ⇒ **The map must be able to open the console.** If they refused each other, the map could never deliver the one thing it exists to deliver, and the whole `WR-§6` seam would be dead code.
⚖️ **And the mechanical distinction is real:** placement / targeting / the group pick each own the **LMB**, and so does the map (its markers are hit-tested on mouse-down) — two of those live at once is a genuine conflict. The console owns the **KEYBOARD** and never hit-tests the world. `ApplyCursorInputState` ORs its owners, so the two compose with no posture fight.
🚩 **QA: this is the single decision most worth disagreeing with. If the gate rules them exclusive, the fix is one clause — but the marker click then has to close the map, which discards the paid reveal (`WR-§7`), i.e. clicking a place after paying 30 gold would delete the red dots. I judged that unacceptable and chose the pair.**

### D-2 📌 **`EnsureAssistantConsoleOpen()` — the open half of `OnAssistantConsolePressed` EXTRACTED VERBATIM** (an addition over the `names:` block)

The marker click needs the console's open **sequence**, which is delicate: guard → lazy create → **`AttachConsoleWidget` BEFORE `OpenConsole`** (attach after and the component misses the very broadcast it subscribes to — a silent FSM stall) → post-hoc `IsConsoleOpen()` → roll the posture back on failure. ⛔ **A second copy of that ordering is how the silent failure comes back.** So I extracted it, the shipped remedy for exactly this shape (`CreateUnitGroup` / `ApplyArmyWideStance`).

**Behaviour-preserving, and proven rather than asserted:** the body moved with **no line, no ordering and no log string changed**; the only edits are `return;` → `return false;` (×2), a trailing `return true;`, and a `return false;` on the fault-latch path. **The key handler ignores the return value exactly as it always did** — nothing in `OnAssistantConsolePressed` ever branched on it.

⛔ **`WR-§5` RULING 5 SURVIVES, AND HERE IS THE RUN, NOT THE CLAIM:**
```
$ awk '/^bool ASiegePlayerController::CanOpenAssistantConsole/,
       /^USiegeAssistantConsoleWidget\* ASiegePlayerController::GetOrCreateAssistantConsoleWidget/' \
      SiegePlayerController.cpp | grep -v '^\s*\(//\|\*\|/\*\)' \
  | grep -nE "CommanderNpc|IsHeroInCommanderRange|InteractRadius|IsPlayerInRange|bWarMapOpen|WarMapWidget|GetEnemyRevealCost"
(no output — exit 1)
```
**RAW COUNT: 0.** ⇒ ⛔ **No proximity test, no NPC reference and no map state exists anywhere in `CanOpenAssistantConsole` / `SetAssistantConsoleOpen` / `OnAssistantConsolePressed` / `EnsureAssistantConsoleOpen`.** The console still opens **anywhere**, with or without an NPC, with or without a castle. ✅ **And the other four console functions are byte-unchanged — only `OnAssistantConsolePressed` carries a diff hunk** (`git diff -U0 | grep '^@@'` shows hunks in that function and nowhere else in the console block).

### D-3 📌 **A POLLED RMB/Esc CLOSE IN `PlayerTick`** (an addition over the `names:` block)

⛔ **Not polish — it is the only guaranteed way out of the map, and the reason is a mechanism:** a marker click **opens the console, which takes Slate keyboard focus on its input box.** A focused `UEditableTextBox` consumes character keys ⇒ **pressing `M` again types "m" into the sentence instead of reaching `IA_WarMap`**, and until TASK-568 ships `WBP_WarMap`'s `CloseButton` there would be **no other way to dismiss a full-screen panel.** RMB/Esc polled here is the *identical* double-cover the three shipped cursor modes already carry (*"the player can always leave placement mode even if the asset is missing"*).
- ⛔ **`AS-§6` RULING A-2 (Escape, CLOSED) IS UNTOUCHED:** this poll is in the CONTROLLER's tick, ⛔ not in the console widget; no `FReply`, no preview handler, no console close route was added. Escape closes the **map**; the console's own Escape behaviour is byte-unchanged.
- ⚠️ **`bWarMapOpen` is false on every pre-existing path**, and the branch sits **after** the three shipped branches, so placement / targeting / the group pick evaluate exactly as before.

### D-4 📌 **THE AUTHORITY'S REFUSAL MESSAGE IS DELIVERED BY A *LOCAL PRE-CHECK*, NOT A THIRD RPC**

`WR-§8` fixes this batch at **exactly two RPCs**, and a refusal decided on the authority reaches a REMOTE client's screen through nothing (the server-side copy of that PC broadcasts `OnCardRefused` to no widget). ⇒ `HandleWarMapRevealButtonClicked` pre-checks `CanAfford(EnemyRevealCost)` **locally** and shows *"Not enough gold"* on the shipped `OnCardRefused` surface — **exactly the shipped `PlayHandSlot` / `DiscardHandSlot` pattern, for exactly its reason.** ⛔ **The pre-check is a MESSAGE, never the gate:** the authority re-reads the cost and re-runs `SpendGold`, and its refusal is the one that decides. `Gold` is `DOREPLIFETIME_CONDITION(..., COND_OwnerOnly)` (`SiegePlayerState.cpp:32`), so the value read locally is the owning client's live balance — **not a guess.**

### D-5 📌 **⛔ NO SERVER-SIDE PROXIMITY RE-TEST** (a decision, recorded so it does not read as an omission)

`WR-§7` and the manager's ruling both say this is an **ECONOMY/UI GATE, ⛔ NOT an anti-cheat boundary** — on a listen server the client already holds every enemy actor under Tier-B relevancy, so nothing is concealed and there is nothing to protect. A position re-test would only **add** a failure mode (a legitimately in-range player whose replicated pawn position lags a frame gets refused). ⭐ **And the economy is its own limit: every request costs the full price, so a spammed RPC drains the spammer's own gold and stops.** Written into the RPC implementation as a comment.

### 📌 ALSO DECLARED — SCOPE OF THE SURVEY
**Buildings, towers and the enemy CASTLE are deliberately NOT dotted.** Jonathan's sentence is *"reveal all enemy LOCATIONS"* about a map that *"updates with dots that show ally locations"*, and the ally side is **units + hero** — so the paid reveal is the SAME survey for the other team, with the team comparison inverted, same classes, same alive tests, same order. A castle is at a fixed already-known place (it **is** a place symbol). 🚩 **Flagged for Jonathan rather than assumed in either direction.**

---

## 4. ⛔⛔ THE GOLD RULE — SPEC ITEM (6)'s PASTED GREP, WITH ITS RAW HIT COUNT

**Command and raw result (TASK-565 re-runs this and expects ZERO):**
```
$ grep -nE "SpendGold|AddGold|EnemyRevealCost|EnemyReveal|RequestEnemyReveal|PerformEnemyReveal|ACommanderNpc|CommanderNpc|WarMap|SetWarMapOpen|CanOpenWarMap" \
      Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp \
      Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.h
(no output — exit 1)
```
### ⇒ **RAW COUNT: 0 hits across both files.**
⛔ **No assistant intent, `who`, `where`, executor branch or confirm path can reach the spend.** The executor does not name `SpendGold`, `EnemyRevealCost`, `PerformEnemyReveal`, `ServerRequestEnemyReveal`, `ACommanderNpc` or the war map at all — it cannot call what it cannot name, and it has no include reaching any of them.

⭐ **THE COMMENT THE SPEC ASKED FOR IS IN THE CODE, AT THE SPEND SITE**, so nobody later "fixes" it: *"The spend above was initiated by the PLAYER'S OWN CLICK on a UI button and is unreachable from every assistant path… ⚖️ The player spending gold at a map his AI happens to stand next to is not the AI spending gold, and the distinction is recorded because it will look like a violation to someone reading fast."* The same paragraph is repeated on `ServerRequestEnemyReveal`'s declaration, which is where a reviewer looks first.

### ⛔ NET-ZERO REFUSAL (spec item 4) — THE ORDER IS THE PROOF
`PerformEnemyReveal` refuses **before any gold moves** on: no authority · no world · no `ASiegePlayerState` · **no own-team commander (fail closed — ⛔ no fallback price is invented)** · `SpendGold` false. **After the spend there is deliberately no early-out at all** — every path reaches `ClientReceiveEnemyReveal`, because a refusal after the spend would be exactly the partial spend `WR-§7` forbids. An **empty** survey (the enemy army really is dead) is a legitimate paid-for answer and is sent as one.

---

## 5. 📌 M8 DECLARATION (`WR-§8` — ⛔ NOT the last three batches' boilerplate)

| M8 question | answer |
|---|---|
| new replicated property | ⛔ **none.** `bWarMapOpen`, `WarMapWidget`, `bWarnedNoCommanderNpc` are all local, non-replicated. |
| new replicated class | ⛔ **none.** |
| new relevancy tier | ⛔ **none.** The controller's Tier A declaration is unchanged; the map is a `UUserWidget` (client-local by construction) and the NPC is TASK-559's Tier C. |
| **new RPCs** | ⭐ **TWO, exactly as `WR-§8` declares — and they are the batch's only ones:** `ServerRequestEnemyReveal()` (**Reliable, WithValidation**) and `ClientReceiveEnemyReveal(const TArray<FVector2D>&)` (**Reliable**). Named `Server<Verb><Noun>` / `Client<Verb><Noun>`. |
| `HasAuthority()` on every mutation site | ✅ **`PerformEnemyReveal` carries its own guard AND `ServerRequestEnemyReveal_Implementation` carries a second one** — belt and braces, because the thing behind the guard is gold and TASK-357's FINDING-4 **measured** a Server RPC body executing on the caller. |
| `GetFirstPlayerController()` | ⛔ **zero uses** (sweep §7). |
| bandwidth | Reliable array payload bounded by `MaxEnemyRevealDots` (file-static, the `MaxHUDInitAttempts` class). Over-bound ⇒ truncate + Warning; ⛔ **no refund** (a partial refund is a partial spend). |

---

## 6. ⛔ `SC-§33` — THE TRAILING-DEFAULT LAW, DISCHARGED WITH THE PASTED, ENUMERATED, CLASSIFIED SWEEP

**(a) THIS TASK ADDS NO DEFAULTED PARAMETER AT ALL.** Every function it introduces takes required arguments or none:
`CanOpenWarMap()` · `SetWarMapOpen(bool)` · `IsHeroInCommanderRange()` · `GetWarMapWidget()` · `IsWarMapOpen()` · `ServerRequestEnemyReveal()` · `ClientReceiveEnemyReveal(const TArray<FVector2D>&)` · `OnWarMapPressed()` · `HandleWarMapOpenChanged(bool)` · `HandleWarMapPlacePicked(FName)` · `HandleWarMapRevealButtonClicked()` · `GetOrCreateWarMapWidget()` · `CloseWarMap()` · `PerformEnemyReveal()` · `FindOwnTeamCommanderNpc()` · `EnsureAssistantConsoleOpen()`. ⇒ **`SC-§33`'s trigger — "a DEFAULTED parameter added to a function that ALREADY HAS CALL SITES" — does not fire on anything I authored.**

**(b) THE SWEEP OVER EVERY SYMBOL I INTRODUCE, WHOLE `Source/` TREE, WITH THE RAW COUNT:**
```
$ grep -rn "ServerRequestEnemyReveal\|ClientReceiveEnemyReveal\|PerformEnemyReveal\|OnWarMapPressed\|CanOpenWarMap\
\|SetWarMapOpen\|IsWarMapOpen\|GetWarMapWidget\|GetOrCreateWarMapWidget\|CloseWarMap\|HandleWarMapOpenChanged\
\|HandleWarMapPlacePicked\|HandleWarMapRevealButtonClicked\|FindOwnTeamCommanderNpc\|IsHeroInCommanderRange\
\|EnsureAssistantConsoleOpen\|WarMapAction\|WarMapWidgetClass\|bWarMapOpen\|WarMapWidget\b" Source/ \
      --include=*.h --include=*.cpp --include=*.cs | wc -l
199

$ (same grep, -l)
Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.h
Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h
Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h
```
**RAW COUNT: 199 hits / 4 files.** Classification of the **4 hits outside my two files** — command and raw output:
```
$ grep -n <same pattern> CommanderNpc.h WarMapWidget.h
CommanderNpc.h:56    " *  (ServerRequestEnemyReveal / ClientReceiveEnemyReveal) live on"
WarMapWidget.h:324   " *  ⚠️ The TWO RPCs `WR-§8` declares for this batch (`ServerRequestEnemyReveal` /"
WarMapWidget.h:325   " *  `ClientReceiveEnemyReveal`) belong to TASK-563's `ASiegePlayerController`…"
WarMapWidget.h:419   " *  Accepts a paid reveal's dot list from TASK-563's `ClientReceiveEnemyReveal`."
```
**ALL FOUR ARE (ii) COMMENT PROSE — every one begins with ` * `.** ⇒ **ZERO code call sites outside my two files. Nothing to update, nothing left at a default, nothing to name onward.** ✅ *A sweep that finds every call site already correct is a RESULT and is reported as one* (`SC-§22`).

**(c) ⛔ THE TWO PRE-EXISTING `CreateAndAddToViewport` CALL SITES — ENUMERATED AND CLASSIFIED, BECAUSE THIS IS THE OBLIGATION HANDED TO ME BY NAME.**
```
$ grep -rn "CreateAndAddToViewport" Source/ --include=*.h --include=*.cpp
RAW HITS: 27 — of which exactly TWO are calls; the other 25 are declarations, definitions, log strings, includes and doc comments.
```
| call site | classification |
|---|---|
| `SiegePlayerController.cpp` `GetOrCreateWarMapWidget` — `UWarMapWidget::CreateAndAddToViewport(this, TSubclassOf<UWarMapWidget>(ResolvedMapClass), WarMapWidgetZOrder)` | **(i) FIXED BY THIS TASK.** Was the function's only future caller; **both** trailing defaults now passed explicitly. **This is TASK-560 handoff §4(d)'s obligation, discharged.** |
| `SiegePlayerController.cpp` `GetOrCreateAssistantConsoleWidget` — `USiegeAssistantConsoleWidget::CreateAndAddToViewport(this)` | ⚠️ **(ii) DELIBERATELY LEFT AT BOTH DEFAULTS, AND THIS IS A DECISION I AM DECLARING RATHER THAN AN ITEM I SKIPPED.** ⛔ **There is no `WBP_AssistantConsole` class to pass — `AS-§6` ruling A(c) RESERVES that asset and leaves it unauthored**, so `ConsoleClass = nullptr` is the *correct* value, not an inherited accident. And writing `(this, nullptr, 5)` would **hard-code `5` in a second place**, creating exactly the drift trap `SC-§33` exists to prevent: change the declaration's default and this site would silently keep the old number. ⇒ **byte-unchanged, on purpose.** 🚩 **If the gate disagrees, say so — it is a one-line change either way, and I would rather be corrected than have chosen silently.** |

**(d) ⚠️ ONE FUNCTION SIGNATURE OF ANOTHER TASK'S CHANGED SHAPE — NONE.** I added no parameter to any existing function. `OnAssistantConsolePressed` keeps its exact signature; `EnsureAssistantConsoleOpen` is brand-new with zero pre-existing call sites (`SC-§33`'s own scope exemption, the `FSiegeAssistantRegionStatics::IsPointInRegion` precedent) and exactly two callers, both created here.

---

## 7. ⛔ THE AIRLOCK + BAN SWEEPS — RUN, NOT ASSERTED

**Sweep A — forbidden symbols in ADDED CODE LINES ONLY (comment lines filtered out):**
```
$ git diff -U0 -- SiegePlayerController.{h,cpp} | grep '^+' | grep -v '^+++' \
    | grep -v '^+\s*\(\*\|//\|/\*\*\|\*/\|/\*\|//~\)' \
    | grep -nE "BuildZone|ComposeTurn|ZoneA|GBNF|Gbnf|Grammar|Capture\(|SetPause|SetGlobalTimeDilation\
|GetFirstPlayerController|PlaceVocabulary|own_castle|ancient_ground|nearest_mine|\"mid\"|\"hero\"|enemy_castle"
(no output — exit 1)
```
### ⇒ **RAW COUNT: 0.** ⛔ **No Zone builder, no turn composer, no grammar, no `Capture()`, no pause, no time dilation, no `GetFirstPlayerController`, and — critically — ⛔ NO PLACE-SYMBOL LITERAL ANYWHERE.**
⭐ **The symbol is moved as an opaque `FName` → `FString`.** This file knows nothing about `PlaceVocabulary` and cannot learn: which symbols exist is `USiegeAssistantVocabulary`'s, which are clickable is `UWarMapWidget`'s. ⇒ ✅ **ZERO prompt characters spent; `ZoneA` cannot have moved from its named/dated 2026-08-05 baseline of `5658` chars** (TASK-564 asserts it against the untouched `Tests/SiegeAssistantZoneATest.cpp`).

**Sweep B — the batch-wide token ban (`AS-§12g`):**
```
$ git diff -- SiegePlayerController.{h,cpp} | grep '^+' | grep -niE "token"
(no output — exit 1)
```
**RAW COUNT: 0.** ⛔ **No token figure is quoted, derived or reasoned from in the code or in this document.** Every size claim here is in chars.

**Sweep C — comment hygiene (an early-terminating `*/` inside prose is a real compile break and I introduced one, caught it, and fixed it):**
```
$ grep -nE "^\s+\*\s+.*\*/" SiegePlayerController.h SiegePlayerController.cpp   → 0
$ grep -nE "^\s*//.*/\*"    SiegePlayerController.h SiegePlayerController.cpp   → 0
$ brace balance: .h 17/17 · .cpp 506/506
```

---

## 8. ⚠️⚠️ THE ESCALATED SNAPSHOT GAP — ⛔ NOT MINE, AND HERE IS WHETHER I MADE IT BETTER OR WORSE

TASK-560 found that `GetTurnSnapshot()` **returns null until the player's first console sentence**, so a war map opened before any console use draws dots but **no place markers — nothing to click.** ⛔ **I did not reach into `SiegeAssistantComponent.{h,cpp}` and I did not force a `Capture()`** — sweep A above proves both (`Capture(` = 0 in added code; the component is absent from `git diff --stat`).

### ⚖️ MY HONEST ASSESSMENT, AS ASKED: **this task makes the gap MEASURABLY SMALLER, and it does so without crossing the boundary.**

⭐ **Because the marker click now OPENS THE CONSOLE, the console is on screen inside the map for the first time.** Before this task the player had to *know* to close the map, press the console key, type a sentence, then re-open the map. Now the console is one click away — except that with no markers there is nothing to click, which is the gap itself. **So the improvement is real but partial:** a player who sends **one** sentence through the console (which the map's own on-screen chrome line already tells him to do — *"Place markers appear once you have sent an order in the console"*) gets a fully working map for the rest of the match, and he no longer has to leave the map to do it, because the console key still works **anywhere**, including with the map up (D-1 is what makes that true).
⛔ **What I did NOT do and could not:** nothing here makes the FIRST open produce markers. **The owner question stands exactly as TASK-560 escalated it**, and my recommendation is unchanged from its candidate (2) — a *display-only* prime on the component, authored by whoever owns that file, explicitly not a turn.

---

## 9. 🚩 WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐⭐ **D-1 — the map/console non-exclusion.** This is the one place I refused the obvious reading of a spec line. Re-run the deadlock argument: if they refuse each other, can a marker click ever reach `AppendToInput`? I say no. **If you disagree, the whole click seam changes shape.**
2. ⭐ **D-2 — the `EnsureAssistantConsoleOpen` extraction.** Diff `OnAssistantConsolePressed` against `HEAD` and confirm the moved body is verbatim apart from the four return statements, and that the key still ignores the return value. **Then re-run my §3 D-2 grep and confirm 0 proximity symbols in the console path — `WR-§5` RULING 5 is the thing most able to break here and it is the one Jonathan stated in terms.**
3. ⛔ **§4's assistant grep.** Re-run it. Expect **0**. It is the batch's named QA criterion.
4. **The net-zero ordering in `PerformEnemyReveal`** — confirm every refusal is *before* `SpendGold` and that **nothing** returns early after it. A single `return` added below the spend would be a silent partial spend.
5. **World XY vs UV** at `ClientReceiveEnemyReveal` — the one seam a wrong value would pass every readback and still be wrong on screen.
6. **`SC-§33` §6(c)'s console row.** I left `CreateAndAddToViewport(this)` at both defaults with the `AS-§6` A(c) reason. **Rule on it** — I would rather be corrected than have chosen silently.
7. **D-3's polled Escape.** Confirm you agree it does not touch `AS-§6` RULING A-2 (it is in the controller tick, not the console widget; 0 added `FReply` / `EKeys::Escape` lines inside `SiegeAssistantConsoleWidget.*`, which I did not open at all).
8. **The five-way exclusion symmetry.** `CanOpenWarMap()` refuses against placement / targeting / pick / match-end; each of those three now refuses against `bWarMapOpen`. **Two of three would be the silent bug the mirror law exists for.**
9. ⚠️ **`WarMapWidgetZOrder = 4`.** It is a RELATIONSHIP (above the HUD's 0, below the console's 5) rather than a taste. If TASK-568 gives `WBP_WarMap` an opaque background, the console must still be legible on top of it. **Jonathan's pixel check, flagged.**

---

## 10. ⚠️ WHAT I DID **NOT** VERIFY, AND WHY (`SC-§32` posture — say it rather than imply green)

1. ⛔ **NOTHING HERE HAS BEEN COMPILED.** File-only by spec; TASK-566 owns the batch's only compile. Every API claim is a **code reading** matched against shipped usage in this same file, not a build result.
2. ⛔ **NO PIE, NO EDITOR, NO MCP** — the editor IS running this session and I did not touch it. **The proximity radius has not been walked, the map has never been rendered, and no marker has ever been clicked.** Marker feel, dot size, the letterbox and the Z-order stack are all **Jonathan's pixel pass** (`WR-§9`).
3. ⛔ **`IA_WarMap` and `WBP_WarMap` DO NOT EXIST**, so the null-safe paths are the ones that will run first at TASK-566. The *resolved* paths are untested by anything. ⚠️ **TASK-568 must confirm `M` is unbound in `IMC_Hero` before adding it and FLAG rather than stomp a conflict** (`WR-§5`, the E/R/T precedent) — `IMC_Hero` is a binary asset nobody in this wave can text-verify.
4. ⛔ **`ACommanderNpc` IS NOT SPAWNED YET** — TASK-562 furnishes the castle in parallel. Until it lands, `FindCommanderNpcForTeam` returns null ⇒ **the map refuses with a HUD line and one latched Warning, and the console is unaffected.** That is the designed pre-TASK-562 state, ⛔ not a defect to chase.
5. ⚠️ **`WR-§5`'s on-screen prompt row (*"Press M — War Map"*) IS NOT IMPLEMENTED**, and I am declaring its absence rather than letting it read as shipped: no TASK-563 spec item asks for it. ✅ **`IsHeroInCommanderRange()` is exposed `BlueprintPure` precisely so a HUD task can add it with zero change to this class.**
6. ⚠️ **D6 (frozen vs live dots)** ships **frozen**, per `WR-§7`. If Jonathan flips it, the change is a re-push on an interval — **no line of the widget and no signature here changes.**
