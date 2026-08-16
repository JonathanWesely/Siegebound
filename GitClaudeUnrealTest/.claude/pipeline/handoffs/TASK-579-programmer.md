# TASK-579 handoff — [WR-25] ⚠️ THE WAR MAP'S FIRST OPEN — the status line that names the gap (gameplay-programmer, 2026-08-15)

- **Gate:** `.claude/pipeline/qa/TASK-565.md` — ⛔ **this task names TASK-565 as its gate** (build-master refuses to commit without this naming).
- **Compile:** TASK-566. **Commit:** TASK-570. ⛔ **This task opened NO compile of its own.**
- **Status:** `ready-for-qa`.
- **Discipline honoured:** ⛔ no compile · no build · no Git · **no editor (it IS up this session and I did not touch it)** · no MCP · no PIE · no `Content/` asset · no `.csv` · no `Build.cs` · no `Tests/` · no `L_Arena`. ⛔ **No token figure is quoted, derived or reasoned from anywhere in this handoff or in either file** (`AS-§12g`, batch-wide ban) — sweep in §6. Every size claim below is in **chars**, **lines** or **hit counts**.
- **Files touched — exactly the two in `names:`:**
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\WarMapWidget.h` (656 → 740 lines)
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\WarMapWidget.cpp` (858 → 986 lines)
- ⛔ **NOT touched, verified by `git status --porcelain` on the exact paths (no output):** `SiegeAssistantComponent.{h,cpp}` · `SiegeAssistantSnapshot.{h,cpp}` · `SiegeAssistantConsoleWidget.{h,cpp}` (TASK-561) · `SiegePlayerController.{h,cpp}` (TASK-563) · `Tests/` (TASK-564) · `/Game/UI/WBP_WarMap` (TASK-568) · any `Content/` asset · any `.csv` · `Build.cs`. ⛔ **And nothing TASK-578 owns** (`SiegeBotController` · `SummonedUnit` · `BattlefieldScatter` · `Projectile` · `Castle.h` · `UnitCommand.h` · `SorcererUnit.cpp`) — my two files are disjoint from its whole list.
- **Assets referenced:** none added. The file's two existing path references (`/Game/Data/DA_BattlefieldScatter`, `/Game/UI/WBP_WarMap`) are byte-unchanged.

---

## 1. ⭐ THE HEADLINE — WHAT SHIPPED, AND THE ONE SENTENCE THAT SAYS WHAT IT IS NOT

⛔⛔ **THE EMPTY STATE NOW EXPLAINS ITSELF. IT IS STILL EMPTY.** The snapshot is still null on a first open, there are still no place markers, and ⛔ **zero `Capture()` / `EnsureSnapshot()` invocations exist in the pair** (grep in §6). What changed is that the player is **told, in the status line the map already owned**, why there is nothing to click and what one action fixes it.

**The line, verbatim, and it is the whole deliverable:**

```
"No place markers yet - send your commander one order in the console and they appear. The console opens over this map, so you do not have to close it."
```

⚖️ **`WR-§9` row 12 is the one row on that list explicitly labelled a KNOWN GAP WE CHOSE NOT TO CLOSE — ⛔ not a designed outcome — and nothing in this task's code or comments dresses it up as one.** The real repair is **TASK-580**, it belongs in `USiegeAssistantComponent`, and it stays out of this commit for the manager's stated reason: touching that file would convert this batch's strongest proof — *"we did not touch the airlock"* — into a weaker one — *"we touched it and checked."*

---

## 2. ⭐⛔ FOUR DECISIONS INSIDE ~150 CHARACTERS — EACH ONE DELIBERATE, EACH ONE IN THE CODE

| # | decision | why |
|---|---|---|
| **(1)** | ⛔ **IT NAMES NO KEY.** | The console's key lives in **`IMC_Hero`, a BINARY asset no file-only task can read** (TASK-563 §10.3 says so in terms), **and the KEYBOARD-LAYOUT batch remaps bindings POSITIONALLY** — so a transcribed letter would be a guess that is *additionally wrong for a Dvorak player*. ⚠️ **A UI string asserting an unverified fact is `SC-§34`'s stale-constant class wearing player-facing words.** ⛔ The shipped `TEXT("Press Z to accept…")` precedent in `SiegeAssistantConsoleWidget.cpp:71` exists and is **deliberately NOT followed**, for that reason. 🚩 **If the gate wants the key named, it is one literal — but somebody must first READ `IMC_Hero` in the editor and tell me what it is.** |
| **(2)** | ✅ **IT STATES THE REMEDY IS REACHABLE WITHOUT CLOSING THE MAP.** | TASK-563's **D-1** shipped exactly that (the map and console are deliberately NOT mutually exclusive), and it matters **MECHANICALLY, not for comfort: closing the map DISCARDS a paid 30-gold reveal (`WR-§7`)**, so *"close the map and come back"* would be advice that costs the player gold. |
| **(3)** | ⛔ **IT NEVER SAYS "SNAPSHOT", "CAPTURE", "ASSISTANT" OR "TURN".** | The player has no model of any of them. He has a commander he can talk to. |
| **(4)** | ⛔ **PURE ASCII.** | Non-ASCII inside `TEXT()` compiles fine here — **12 such literals ship in `SiegeAssistantComponent.cpp` alone and have built green** (I checked, rather than worrying) — but this string is **RENDERED by Slate's default font**, where a missing glyph is a *visible* defect rather than a build one. Every other player-facing string in this file is ASCII; this matches. |

---

## 3. ⭐ THE MECHANISM — A LATCH, ⛔ NOT A POLL, AND THE HINT PROVABLY DISAPPEARS

Spec item (5): *"the line must DISAPPEAR once the snapshot exists; a stale hint is its own defect."*

**`bShowingNoSnapshotHint` means EXACTLY ONE THING: the hint is the line ON SCREEN RIGHT NOW.** The invariant is enforced **by construction, not by remembering**:

- **ONE writer for `false`** — `SetStatusLine()` clears it on **every** line it sets.
- **ONE writer for `true`** — `ShowNoSnapshotHint()`, which re-arms **after** calling `SetStatusLine` (before would be silently undone).

⇒ ⛔ **The flag physically cannot claim the hint is showing when a picked symbol overwrote it.** The alternative — an assignment beside each of the four call sites — is the shape that rots the first time somebody adds a fifth.

**Retirement runs on the map's EXISTING refresh timer** (`SetAllyRefreshTimerEnabled`'s handle, interval and function name are all **byte-unchanged**; only the callback moved to a new `HandleMapRefreshTimer`, which does the ally sweep **then** the hint check).

### ⭐⭐ AND ONE-WAY IS **PROVABLY** SUFFICIENT — VERIFIED AT THE OWNING FILE, ⛔ NOT ASSUMED

```
$ grep -nE "^\s*Snapshot\s*=|Snapshot\s*=\s*(nullptr|NewObject)" SiegeAssistantComponent.cpp
3222:		Snapshot = NewObject<USiegeAssistantSnapshot>(this);
```
**RAW COUNT: 1 write, ever.** It is inside `EnsureSnapshot()`'s `if (!Snapshot)` guard, and the component's own comment calls it ***"ONE object for the life of the component"*** (`Capture()` resets the object's **contents**, never the pointer). ⛔ **`Snapshot` is never assigned null anywhere.**

⇒ **The transition this latch watches is MONOTONIC — null → non-null, once, permanently.** There is no later state a one-way latch can miss and no case where the hint should re-arm mid-open. **That is a read of the owning file (permitted, `names:` says READ ONLY), not an inference.**

**⚠️ THE BOUND, STATED RATHER THAN IMPLIED:** retirement is bounded by the refresh interval (**0.25 s** default), ⛔ **not instantaneous**. The **markers themselves appear on the very next paint**; only the line of text lags, by at most one interval. ⭐ **And it costs nothing after it fires:** `UpdateNoSnapshotHint` early-outs **on the bool, before reading anything** — once retired it never touches the controller, the component or the snapshot again for the rest of the open.

---

## 4. ⚠️ ONE DECLARED DEPARTURE / EXTENSION (`SC-§15`) — THE CLICK PATH'S DISCRIMINATOR MOVED, AND IT IS A **CORRECTNESS** CHANGE

**Spec item (3) scopes the line to *"when the snapshot is null."* TASK-560's shipped click path did not test the snapshot — it tested `Markers.Num() > 0`:**

```cpp
// BEFORE (TASK-560)
SetStatusLine(Markers.Num() > 0 ? EmptyClickHintText : NoMarkersHintText);
```

⛔ **I changed the discriminator to the snapshot in BOTH deciding places, and I am declaring it rather than letting it read as a silent rewrite.**

⚖️ **Why it is not a liberty:** `Markers.Num() == 0` has **three** possible causes — no snapshot, **no place resolved this match**, and **a degenerate panel** — and only ONE of them is fixed by sending an order. ⇒ On the other two, the old line would have told the player to do something that **cannot work**. ⚠️ **A status line that names the wrong remedy is worse than no status line: he does the thing it says, nothing changes, and he stops believing the map.** The snapshot is the only condition the explanation is actually *about*, which is exactly what the spec says.

📌 **Consequence, stated:** the constant is renamed `NoMarkersHintText` → **`NoSnapshotStatusText`**, because its gate is now the snapshot rather than the marker count. **`grep -rn "NoMarkersHintText" Source/` → 0 hits** (§6), so the rename left nothing dangling. **`EmptyClickHintText` and `OpenStatusText` are byte-unchanged.**

---

## 5. ✅ SPEC ITEM (4) — ONE LINE EACH, ⛔ AND I FOUND NO BIGGER FINDING

The spec says: *if any of these is ALSO null-gated on the snapshot, ⛔ STOP — that is a bigger finding than this task and it changes `WR-§9` row 12.* **I checked all three at the code. ⛔ NONE of them is gated. There is no bigger finding, and that is reported as a result** (`SC-§22`).

| thing | verdict | the evidence |
|---|---|---|
| **ally dots** | ✅ **NOT GATED — works on a first open.** | `RefreshAllyDots()` reads `GetWorld()`, the owning `ASiegePlayerState` for the team, then two `TActorIterator`s. ⛔ It does not name the snapshot, the assistant or `GetReadOnlySnapshot` at all. |
| **enemy dots** | ✅ **NOT GATED — a paid reveal draws on a first open.** | `ReceiveEnemyReveal` stores the RPC payload into `EnemyDotsWorldXY`; `NativePaint` iterates it directly. ⛔ No snapshot on either path. |
| **the 30-gold reveal** | ✅ **NOT GATED — purchasable on a first open.** | Widget side: `HandleRevealButtonClicked` **broadcasts and nothing else**. Authority side (TASK-563): `grep -n "GetTurnSnapshot\|USiegeAssistantSnapshot" SiegePlayerController.cpp` → **the only `Snapshot` hit in the whole file is a COMMENT at :5140** naming `SiegeAssistantSnapshot.cpp` as an *idiom source*. ⇒ ⛔ zero snapshot reads on the spend path. |

⭐ **AND THE PAINT ORDER CONFIRMS IT VISUALLY, NOT JUST STRUCTURALLY:** `NativePaint` draws the ally loop, then the enemy loop, and only **then** calls `BuildMarkerRects` — ⛔ **with no early return between them.** A null snapshot empties the marker array and **nothing else on the map is affected.** ⇒ **`WR-§9` row 12's claim *"ally/enemy dots and the 30-gold reveal work from the first open"* is CORRECT as written. It needs no amendment.**

---

## 6. ⛔ THE SWEEPS — RUN AND PASTED, ⛔ NOT ASSERTED

**Sweep A — the airlock. `Capture()` / `EnsureSnapshot()` INVOCATIONS (strict — an actual call, not the word):**
```
$ grep -nE "(->|\.|::)\s*(Capture|EnsureSnapshot|CaptureTurnSnapshot)\s*\(" WarMapWidget.h WarMapWidget.cpp
(no output — exit 1)
```
### ⇒ **RAW COUNT: 0.** ⛔ **Opening the map, clicking it, and the new timer callback all survey NOTHING.** This is TASK-560's grep, re-run after my edits, with the same answer.

**Sweep B — forbidden symbols on NON-COMMENT lines:**
```
$ grep -vE "^\s*(\*|//|/\*)" WarMapWidget.h WarMapWidget.cpp | grep -nE "Capture\(|EnsureSnapshot|BuildZone|ComposeTurn|GBNF|Grammar|SpendGold|EnemyRevealCost|SetPause|TimeDilation|GetFirstPlayerController|26000|12000|own_castle|ancient_ground|nearest_mine"
536:WarMapWidget.cpp:  TEXT("Deliberately NOT fixed here: forcing a Capture() is the exact side effect the WAR-ROOM law forbids. …")
```
**RAW COUNT: 1. Classification: (ii) a LOG STRING LITERAL inside `TEXT(...)`, ⛔ not a call — and it is TASK-560's line, byte-unchanged by me.** ⇒ ⛔ **Zero code references to any Zone builder, the grammar, `SpendGold`, `EnemyRevealCost`, pause, time dilation, `GetFirstPlayerController`, any arena literal, or any place-symbol literal.**

**Sweep C — the token ban (`AS-§12g`):**
```
$ grep -niE "token" WarMapWidget.h WarMapWidget.cpp
WarMapWidget.h:207  " *  move. ⛔ ASSERT IN CHARS/BYTES, NEVER IN TOKENS: `AS-§12g` pins every token figure"
WarMapWidget.h:227  " *  nearest ancient ground"* resolving to `nearest_mine`. The symbol is the exact token the"
```
**RAW COUNT: 2. Classification: (ii) BOTH are TASK-560's pre-existing COMMENT PROSE (each begins ` * `), both byte-unchanged by me, and ⛔ NEITHER CONTAINS A NUMBER.** The ban is on the figure; there is no figure. ✅ **I added zero occurrences of the word and zero token figures.**

**Sweep D — Zone A's byte freeze, the batch headline:**
```
$ git status --porcelain -- Source/GitClaudeUnrealTest/Siegebound/Tests/
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp
```
⛔ **`Tests/SiegeAssistantZoneATest.cpp` DOES NOT APPEAR — it is byte-untouched, and the only entry under `Tests/` is TASK-564's own new file.** ⭐ **And the structural argument holds unchanged: I spent ZERO prompt characters, added no place symbol, no `who` shape, no grammar alternation and no schema field, and `BuildZoneA`'s entire input surface (`SiegeAssistantSnapshot.{h,cpp}` + `SiegeAssistantVocabulary.{h,cpp}`) is absent from my diff.** ⇒ **Zone A cannot have moved from its named, dated 2026-08-05 baseline of 5658 chars.**

**Sweep E — the two forbidden files, by exact path:**
```
$ git status --porcelain -- SiegeAssistantComponent.h SiegeAssistantComponent.cpp SiegeAssistantSnapshot.h SiegeAssistantSnapshot.cpp
(no output)
```
**RAW COUNT: 0.** ⛔ **The airlock is untouched. TASK-580's file is untouched.**

**Sweep F — comment hygiene + structural sanity** (an early-terminating `*/` inside prose is a real compile break):
```
$ grep -cE "^\s+\*\s+.*\*/"  WarMapWidget.h WarMapWidget.cpp   → 0 / 0
$ grep -cE "^\s*//.*/\*"     WarMapWidget.h WarMapWidget.cpp   → 0 / 0
brace balance: .h 7/7 · .cpp 77/77
declaration ↔ definition: all 5 touched functions match exactly (3 new + SetStatusLine + RefreshAllyDots)
```

---

## 7. ⛔ `SC-§33` — THE TRAILING-DEFAULT LAW, DISCHARGED WITH THE PASTED, ENUMERATED, CLASSIFIED SWEEP

**(a) ⛔ THIS TASK ADDS NO DEFAULTED PARAMETER AT ALL. All three new functions take ZERO arguments:** `void HandleMapRefreshTimer()` · `void ShowNoSnapshotHint()` · `void UpdateNoSnapshotHint()`. `SetStatusLine(const FString&)`'s signature is **byte-unchanged**.

```
$ grep -nE "= (nullptr|0)\)|= (nullptr|0)," WarMapWidget.h
533:		TSubclassOf<UWarMapWidget> MapClass = nullptr,
534:		int32 ZOrder = 0);
```
**RAW COUNT: 2 — and BOTH are TASK-560's, on `CreateAndAddToViewport`, byte-unchanged by me and already ratified by the gate** (TASK-565 item (18) ruled the war-map side gets the explicit spelling and TASK-563 supplies it). ⇒ **`SC-§33`'s trigger — *"a DEFAULTED parameter added to a function that ALREADY HAS CALL SITES"* — does not fire on anything I authored.**

**(b) THE SWEEP OVER EVERY SYMBOL TASK-579 INTRODUCES OR RENAMES, WHOLE `Source/` TREE, WITH THE RAW COUNT:**
```
$ grep -rn "HandleMapRefreshTimer\|ShowNoSnapshotHint\|UpdateNoSnapshotHint\|bShowingNoSnapshotHint\
\|NoSnapshotStatusText\|NoMarkersHintText" Source/ --include=*.h --include=*.cpp --include=*.cs | wc -l
24

$ (same grep, -l)
Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp
Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h
```
**RAW COUNT: 24 hits, exactly 2 files — both mine.**
**Classification: all 24 are (i) declarations, definitions, internal call sites and doc comments INSIDE my own pair. ⛔ ZERO pre-existing external call sites exist for any of them, so there is nothing to update, nothing left at a default and nothing to name to another task.**
⛔⭐ **AND THE RENAME IS PROVEN CLEAN: `NoMarkersHintText` contributes 0 of those 24 hits** — the retired name exists nowhere in `Source/`, so no dangling reference survives. ✅ *A sweep that finds every call site already correct is a RESULT and is reported as one* (`SC-§22`).

---

## 8. ⛔ TASK-564's 22 TESTS — WHY THEY STILL PASS, CHECKED RATHER THAN HOPED

⚠️ **This task is `blocked-by 564` precisely so the test author read settled code. I edited the subject afterwards, so the obligation is mine to show I did not invalidate it.**

1. **⛔ NO TEST OBSERVES THE STATUS LINE.** `grep -niE "status|hint|GetText|StatusTextBlock|OnWarMapStatusLine" Tests/SiegeWarMapTest.cpp` → **0 hits.** Every assertion is on `IsMapOpen()`, `GetEnemyRevealDotCount()`, `BuildMarkerRects` output, or the pure `FSiegeWarMapProjection` statics — **none of which I touched.**
2. **✅ THE HEADLESS `OpenMap()` PATH STAYS NULL-SAFE, AND I VERIFIED THE ENGINE CALL RATHER THAN TRUSTING IT.** The tests run `OpenMap()` on a bare `NewObject<UWarMapWidget>` with **no world and no owning controller**. My addition calls `GetReadOnlySnapshot()` there, which starts with `GetOwningPlayer()`:
   ```
   UserWidget.cpp:1487  APlayerController* UUserWidget::GetOwningPlayer() const
   UserWidget.cpp:1489  { return PlayerContext.IsValid() ? PlayerContext.GetPlayerController() : nullptr; }
   ```
   ⇒ **null-safe by the engine's own guard** ⇒ null controller ⇒ null snapshot ⇒ the test simply takes the `ShowNoSnapshotHint()` branch. ⭐ **And that exact route is ALREADY exercised by the shipped `BuildMarkerRectsYieldsNothingWithoutASnapshot` test**, so it is proven, not predicted.
3. **✅ NO NEW `Warning` OR `Error` IS EMITTED ON THE TEST PATH.** The only log the new code can reach is `GetReadOnlySnapshot`'s pre-existing **`Log`-level, one-shot-latched** line — spec item (5) satisfied (⛔ no per-frame log, ⛔ no `Warning`), and automation only fails on Warning/Error.
4. **✅ `SetAllyRefreshTimerEnabled` NEVER RUNS IN THE HARNESS** (`GetWorld()` is null ⇒ it returns before touching the timer), so the changed callback is unreachable from every test.
⇒ **SUITE DELTA: none. TASK-566 should still see the 110 TASK-564 reported.**

---

## 9. 📌 M8 DECLARATION (`WR-§8` — ⛔ NOT the last three batches' boilerplate)

⛔ **No replicated property, no new replicated class, no new relevancy tier, no RPC.** ⭐ **And this task is even further from the wire than TASK-560 was: it adds one non-replicated `bool` and three private, parameterless, client-local functions to a `UUserWidget`, which is client-local by construction.** ⛔ **Nothing here crosses the wire, nothing is authority-owned, and no `HasAuthority()` guard is owed** — the batch's two RPCs remain TASK-563's and are untouched. ⛔ **`GetFirstPlayerController` is not used** (Sweep B).

---

## 10. 🚩 WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐⭐ **§4's DEPARTURE — the click path's discriminator moving from `Markers.Num()` to the snapshot.** It is the one place I changed shipped TASK-560 behaviour rather than only adding. **Rule on it.** If you disagree, reverting is a two-line change and the constant rename can stand either way.
2. ⭐ **§2 decision (1) — NAMING NO KEY.** I refused to transcribe the console key because `IMC_Hero` is binary and the KBD batch remaps positionally. 🚩 **If the gate wants it named, somebody must READ the asset first — I will not guess it, and a wrong key in a help line is worse than a vague one.**
3. ⭐ **§3's MONOTONICITY ARGUMENT.** The one-way latch is only correct because `Snapshot` is written once and never nulled. **Re-run my grep on `SiegeAssistantComponent.cpp` and confirm the single write.** If a future task ever nulls that pointer, this latch is where it breaks — which is why the argument is written at the code, not only here.
4. **The `SetStatusLine`-clears / `ShowNoSnapshotHint`-re-arms ORDER.** ⛔ Arming *before* the call would be silently undone and the hint would never retire. It is commented at both sites; confirm both.
5. **§5 — re-run all three non-gating checks yourself.** The spec makes a bigger-finding STOP conditional on them, and I am reporting **no** bigger finding. That is the kind of negative claim worth a second pair of eyes.
6. **§8 — the TASK-564 non-invalidation argument.** I edited a file *after* its independent test author read it. Please confirm the four legs, especially that no test observes the status line.
7. ⚠️ **THE 0.25 s RETIREMENT LAG.** It is bounded, declared, and I judged it invisible (the markers appear on the next paint; only the sentence lags). **If you disagree, say so** — the alternative is a call from the console's own open/submit path, which is TASK-561/563's file and ⛔ not mine.

---

## 11. ⚠️ WHAT I DID **NOT** VERIFY, AND WHY (`SC-§32` posture — say it rather than imply green)

1. ⛔ **NOTHING HERE HAS BEEN COMPILED.** File-only by spec; TASK-566 owns the batch's only compile. Every API claim is a **code reading** matched against shipped usage in this same file, not a build result.
2. ⛔ **NO PIE, NO EDITOR, NO MCP** — the editor IS running this session and I did not touch it. ⛔ **The line has never been rendered.** Whether ~150 characters **fit and wrap legibly** in `WBP_WarMap`'s status line is ⚠️ **JONATHAN'S PIXEL CHECK and ⛔ MUST NOT be inferred from any property readback** (the UMG law). 🚩 **A layout note for TASK-568: `StatusTextBlock` should be wide enough for a wrapping sentence, ⛔ not sized for the two words `"War map"`** — this is the longest line the map can show.
3. ⛔ **`WBP_WarMap` STILL DOES NOT EXIST.** With no BP the line reaches only the `OnWarMapStatusLine` BIE and nothing renders it — ⚠️ **so at TASK-566 this feature is verifiable by code review and by the suite, ⛔ but NOT by looking at a screen until TASK-568 lands.** Stated so nobody records "no line appeared" as a defect before the chrome exists.
4. ⛔ **I DID NOT FIX THE GAP, AND THAT WAS THE INSTRUCTION.** The first open still has no markers. **TASK-580 holds the repair**, blocked on TASK-570 + Jonathan.
