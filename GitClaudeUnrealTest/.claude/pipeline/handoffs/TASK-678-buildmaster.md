# TASK-678 — [UNCAP-3 + VID2S1-3] THE SHARED GATE — build-master handoff (2026-08-28)

Status: **done**. One compile, one suite run, one standalone client carried BOTH lanes' live
checks. COMMIT A (CARD-UNCAP) = `90db1e1` (11 files). COMMIT B (SESSION-BACK + records) = the
commit that carries this file — see the git log for the authoritative hash. Main 6 ahead after
both. ⛔ Not pushed (the never-push law).

## 0. Pre-flight (all gates held)

- HEAD `d8bfd23` verified before staging and again before COMMIT A (Jonathan self-commits law —
  no duplicate/amend; no race appeared).
- Porcelain reconciled EXACTLY against the two lanes' ledgers: the 9 UNCAP-§7 owned Source files
  + `SessionMenuWidget.{h,cpp}` + `Tests/SiegeDeckSlotsTest.cpp`(in the 9) + handoffs 676/680 +
  qa 677/681 + the VID-002 report + the 5 evidence PNGs + TASKBOARD/CONVENTIONS. Zero unexplained
  lines at every checkpoint.
- ⛔ `SiegeBotController.h`: **zero diff** (WARN-1 honored — never staged).
- `L_Arena.umap` == the ROT-§2 ledger `9ccd54ef…0e58` at pre-flight, after the game session, and
  at commit time. Never opened for edit, never saved.
- Gates: `qa/TASK-677.md` PASS (0/2/1) + `qa/TASK-681.md` PASS (0/0/2), both read in full.

## 1. ⚠️ Deviation of record — the 17:28 process, and the crash I caused

The dispatch expected editor PID 34664 UP. Measured instead: **no editor existed**. PID 9308
(booted 17:28) was a **standalone game client — Jonathan's own**: its log shows
`Bringing World /Game/Maps/L_MainMenu` and THREE `[SessionMenu] Back pressed - no session
active; WBP handles panel dismissal.` lines at 17:29 — him reproducing VID-002 live on the OLD
binary, then the client idling 36 minutes. My editor-shaped preflight probe
(`EditorLevelLibrary.get_editor_world` — an editor API — sent over remote-exec into a `-game`
process) hit an EXCEPTION_ACCESS_VIOLATION inside the Python plugin and **crashed his client**
(18:07). Cost assessment, measured: no editor up ⇒ no dirty packages existed; last client
activity was 36 min before the crash ⇒ no save write in flight; saves verified against my
pre-task sha ledger afterward; no crash dialog left on screen; L_Arena untouched. Reported to
the Build & Git thread immediately (mid-task post). **Lesson banked: a remote-exec node is not
necessarily the editor — `-game` clients answer as their own nodes (the TASK-674 §3 discovery,
now with teeth). Probe `os.getpid()`/world-for-play BEFORE any editor API.** All subsequent
drive scripts (`t678_*.py`) are game-safe (zero Editor* libraries).

## 2. Compile + suite (the one shared slot, QUIET-MODULE discharged)

- Build.bat per the CLAUDE.md line, log-parse law honored: **`Result: Succeeded`** — a REAL
  rebuild, 18 actions, every touched TU compiled by name (`DeckLibrary.cpp`, `DeckComponent.cpp`,
  `DeckBuilderWidget.cpp`, `SessionMenuWidget.cpp`, `SiegeBotController.cpp`,
  `SiegeDeckSlotsTest.cpp` + unity modules). Zero errors, zero warnings. 18.09 s.
- Suite headless (the TASK-674 invocation): **expected 140/140, actual 140/140**
  `Result={Success}`, 0 fails — 136 baseline + the 4 `Siegebound.Deck.Uncap*` cases, each
  visibly green by name. Logs: session scratchpad `TASK-678/build-678.log` +
  `automation-678.log` (+ `report/`).

## 3. Back lane — live verdict GREEN (qa/681's pins, machine lane)

One fresh standalone client (`-game`, 1280×720, abslog `TASK-678/game-678.log`):

| Pin | Verdict |
|---|---|
| Machine-driven `BackPressed()` in the real standalone menu | **PASS ×2** — two full round trips. Each press: panel LEFT the viewport, a **FRESH `WBP_MainMenu_C` instance** entered it (`_1` after R1, `_2` after R2 — instance names prove re-creation, not un-hiding) |
| Exact NEW log line present | **PASS** — `[SessionMenu] Back pressed - no session active; returning to main menu.` exactly **2×** in the runtime log (one per press); exactly 1 hit in `Source/` |
| OLD line absent | **PASS** — `WBP handles panel dismissal`: **0** in the runtime log, **0** in `Source/` |
| Menu-returns pixel | **PASS** — `TASK-678-back-menu-returns-1280x720.png` (beside this file): the re-created menu renders ALL 7 entries (Play vs Bot · Sandbox · Deck Builder · Multiplayer · Settings · Login · Quit), panel gone |
| Second round trip re-wires | **PASS at the instrument's reach** — R2 ran off the R1-created instance and produced `_2`; the panel-open leg was the machine mimic of the TASK-355 transition (the menu's buttons are anonymously named `Button_0..6`, so the real OnClicked broadcast lane could not target Multiplayer). The physical click stays TASK-682's (qa/681 note 5 rates this optional, not gating). |

Honest notes: (a) three `[SessionMenu] Session system unavailable.` Warnings in the log are my
two world-less warm-up attempts — the subsystem-null guard (qa/681 NIT-1) holding, not a defect;
(b) plain `Shot`/`HighResShot` exclude UMG and OS-side window capture dies at the lock screen
(TASK-669 §6 — the desktop was locked mid-gate) — `Shot showui` is the UI-inclusive instrument,
now banked.

## 4. Uncap lane — live verdict GREEN (qa/677's pins, machine lane)

Scratch-profile discipline: all 5 saves sha-ledgered + byte-copied BEFORE the client ran
(`TASK-678/savegames-backup/original-shas.txt`); the drive scratched ONLY the empty slot deck10
on the auto-logged-in profile; after the client closed, **all 5 restored byte-identical**
(diff of sha ledgers: empty).

| Pin | Verdict |
|---|---|
| `AddCopy('Footman')` climbs past 12 (the old cap) | **PASS** — 12 → 13 with no refusal, on to **50**; `GetTotalCount()` 50 |
| The shim's 50 reaches the WBP | **PASS** — `GetCardMaxCopies('Footman')` returned **50** live (the identical BlueprintPure the `WBP_DeckCardTile` graph calls); with `GetCountOf`==50 the tile's grey condition fires exactly at 50-of-one |
| Badge shows the TRUE count (U3) | **PASS on pixels** — `TASK-678-uncap-builder-50of50-1280x720.png`: Footman badge **50**, footer **Deck: 50/50**, Avg cost 9.0 (=50×9/50), orange active outline on deck10 |
| 50-of-one deck legal end-to-end | **PASS** — deck10 set active → `open L_Arena` → `ASiegePlayerController 'SiegePlayerController_0': active saved deck 'deck10' is legal (50 cards) — using it this match` AND `UDeckComponent on 'SiegePlayerController_0': built a 50-card draw pile from the pending OVERRIDE deck 'deck10' (1 entries)` (cpp:56-58). **ZERO** deck fallback Warnings (:66-69 / controller :286-288 never fired; the 4 grep hits on "falling back" are my own script prints ×3 + the SiegeAssistant designed-default line). Bonus: the bot's own deck also built via the override lane. |

## 5. Grant/permission record (coordinator instruction)

- Jonathan's standing session close/reopen grant (relayed from last night, standing this
  morning): honored in spirit — no editor existed to close; the bounce degenerated to
  compile-then-relaunch. The classifier blocked nothing this gate. His client's crash (§1) was
  NOT a grant matter — it was my instrument error, reported immediately.
- The desktop was locked during the gate (his choice, his machine) — OS-side capture lanes died
  at the lock screen exactly as TASK-669 §6 predicts; in-engine `Shot showui` carried the pixel
  duties. No workaround against the lock was attempted.

## 6. Manager-owed riders (record, per the dispatch)

1. **WARN-1 (677):** `SiegeBotController.h:203` still reads as a cap-inclusive legality
   definition — one dated rider line owed to the NEXT wave that owns that header.
2. **WARN-2 (677):** the pre-existing `FDeckList::TotalCount()` int32 accumulation can wrap and
   the uncap made the wrap REACHABLE by save/payload tampering — the proposed one-line fix
   (refuse any entry `Count > SiegeLegalDeckSize` in `IsDeckLegal`) is a future-wave rider.
3. New instrument law candidate: remote-exec node ≠ editor (§1 lesson) — worth a CONVENTIONS
   line if the manager agrees.

## 7. Environment state at finish

Editor relaunched and left **UP** (PID in the completion report; fresh 140/140 binary), MCP +
remote-exec lanes live, PIE false, dirty packages 0, `L_Arena` == ledger. Game clients: all
mine, all closed (his crashed one is §1). Saves: byte-identical to the pre-task ledger. The
571+552 latch UNSPENT (no assistant console, no `M`, no sentence — the only console commands
issued were `Shot`/`open L_Arena`/`quit` in my own client). TASK-679 + TASK-682 are now
unblocked for Jonathan; TASK-682's strongest instrument is a VID-003 clip of the real click.

## 8. The two commits

- **COMMIT A `90db1e1`** — the 9 UNCAP-§7 owned Source files + `handoffs/TASK-676-programmer.md`
  + `qa/TASK-677.md` (11 files, +450/−73). ⛔ `SiegeBotController.h` verified zero-diff and
  never staged.
- **COMMIT B (this one)** — `SessionMenuWidget.{h,cpp}` + `handoffs/TASK-680-programmer.md` +
  `qa/TASK-681.md` + `footage/VID-002-multiplayer-back-inert.md` + the 5 promoted
  `playtest-evidence/2026-08-28/VID-002-*.png` + this handoff + the 2 capture PNGs +
  TASKBOARD/CONVENTIONS **LAST** (the UNCAP-§/SESSION-BACK law + all flips, already carried in
  the working tree by the manager's board pass; my own 678 flip added).
