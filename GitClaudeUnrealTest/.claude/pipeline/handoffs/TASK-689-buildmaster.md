# TASK-689 — build-master handoff — the WAR-MAP wave's compile · verification · THE COMMIT
2026-08-29 · gate input: `qa/TASK-688.md` (overall PASS, 0 blockers; per-task 684/685/580 PASS)

## 1. PRE-FLIGHT

- **Git trio at dispatch:** HEAD `8c5f8af` ("Update TASKBOARD.md" — Jonathan's self-commit atop
  `ee4aecd`, exactly the predicted +1), **0/0 vs origin** (he pushed it himself; push debt none).
- **Porcelain vs the 688 cargo enumeration:** exact match — the 5 Source files, the 3 icon
  uassets (arrived AUTO-STAGED `A ` rows per the TASK-568 §6 hostile-index precedent; handled by
  explicit-path restage), the 3 raw PNGs, `WBP_WarMap.uasset`, handoffs 683/684/685/687/580/691,
  `qa/TASK-688.md`, board + CONVENTIONS. Zero unexplained Source lines.
- **Adjudications (2 rows not in the enumeration):** `footage/VID-003-warmap-empty-blue-overlay.md`
  + `playtest-evidence/2026-08-29/` (VID-003 frames) = the wave's ORIGINATING footage diagnosis
  (Jonathan's clip of the empty blue map — the exact symptoms this wave repairs) — ruled
  wave-legitimate docs, INCLUDED.
- **⚠️ `Config/DefaultEngine.ini` — ADJUDICATED OUT (the one deviation from the expected shape):**
  the diff is NOT a no-op re-serialization; it is ONE semantic line, a new
  `[/Script/Engine.UserInterfaceSettings] bAllowHighDPIInGameMode=True` appended at EOF.
  The remote-exec law block IS intact at the top (verified by read). NO wave handoff declares the
  line; 683's artist logged the file as foreign dirt with mtime 12:34:04 — 25 min BEFORE the art
  lane's editor launch. Provenance unknown (plausibly Jonathan's own Project Settings touch).
  Per `qa/TASK-688.md`'s ruling ("exclude from the wave commit unless ruled wave-legitimate")
  it is **EXCLUDED from the commit and left in the working tree** for Jonathan/orchestrator to
  claim or revert. ⛔ Not reverted by me — it may be his deliberate setting.
- **W3 discharge (688 WARN-3):** `git diff` hunk census on `WarMapWidget.cpp` — exactly ONE hunk
  inside `NativeOnMouseButtonDown` (`@@ -868,18 +1636,27 @@`); ZERO hunks inside
  `BuildMarkerRects`/`FindMarkerIndexAtLocal` (the pair's only diff-text appearances: 1 context
  line + 3 comment mentions); `Tests/SiegeAssistantZoneATest.cpp` ABSENT from the diff. ✓
- **§25b sha ledger — ALL MATCH:** `T_WarMap_Icon_Mine` `4ac8fcdd…` (15,339 B) ·
  `T_WarMap_Icon_AncientGround` `6a11abc6…` (17,300 B) · `T_WarMap_Icon_Castle` `94859080…`
  (13,123 B) · `WBP_WarMap` `6827303b…` — sha256 == the 683/687 handoff ledger, byte sizes exact.
- **`L_Arena` law:** hash `9ccd54ef…0e58` == ledger at dispatch AND re-verified after the editor
  bounce. Never saved. ✓

## 2. BOUNCE + COMPILE

- Node-identity law honored: port-8000 owner probed = PID 20132 `UnrealEditor.exe` with our
  uproject (the art lane's boot, as expected); process census = 1 editor, 0 `-game` clients.
  PIE-guard `IsPIERunning` = false. Graceful close via the TASK-667 remote-exec lane (Epic's
  reference client, single node enumerated + identity-checked, `quit_editor()`); clean exit.
- **Compile: `Result: Succeeded`** (log-parsed per the exit-code lie law), 0 errors,
  14 actions, 16.8 s. Log: session scratchpad `TASK-689/build-689.log`.

## 3. SUITE — 143/143

Headless (the TASK-655/674 proven recipe, PowerShell). **`...Automation Test Queue Empty
143 tests performed.`** Census: 143 `Test Completed`, **143 `Result={Success}`, 0 fail**,
`: Error:` severity sweep = 0. The three new cases green by name:
`Siegebound.WarMap.HeightToBrightnessRampFloorCeilingAndClamp` ·
`Siegebound.WarMap.MapUvToWorldInvertsTheProjection` ·
`Siegebound.WarMap.PoiIconProjectionStaysInsideTheMapRect`.
Log: session scratchpad `TASK-689/suite-689.log`.

## 4. THE PIXEL LIST — every 688 pin, with captures

⚠️ **Lane deviation, declared:** Jonathan's desktop was LOCKED for the whole live phase (LogonUI
up, console idle) — OS-level input/capture cannot reach a locked session, so the M-key/mouse lane
was foreclosed. Verification ran through the game's OWN surfaces instead: a standalone `-game`
client on L_Arena, the shipped BlueprintCallable/reflection lane (faithful
`GetOrCreateWarMapWidget` replication incl. all three delegate binds), engine-side `Shot showui`
backbuffer captures (UI included), and the REAL bound delegate chain for the click seam. The
pointer-path hit-test itself is pinned by W3's 0-diff + suite case 26; the by-hand click remains
TASK-690's (as the wave always intended).

| # | Pin | Verdict | Evidence |
|---|-----|---------|----------|
| 1 | First-open money shot, fresh match, NO sentence | **PASS** — elevation shading (near-black floor, gray hill/rock relief, BOTH castle shells full white BY DESIGN), gold pickaxe icons at the live mines, verdant-ring icons at both ancient grounds, castle icons at both castles, **all SEVEN markers + labels present on first open** (own_castle, hero, nearest_mine, mid, ancient_ground_near, ancient_ground_far, enemy_castle) | `playtest-evidence/2026-08-29/TASK-689-first-open-fresh-match.png` |
| 2 | Seed log | **PASS, verbatim ×3 (once per boot/reload):** `LogSiegeAssistant: Snapshot seeded at rest (TASK-580): 7 places resolved (3 region-bearing). …` — counted figures, L_Arena | `TASK-689/game-689.log` |
| 3 | Bake log | **PASS:** `[WarMap] Elevation baked: 130x60 samples, 7800 hits, groundZ=0.0, maxZ=2430.0 (relief 2430.0 uu), ceiling=1000.0, 71 texel(s) clamped full white.` — 130×60 exact, 7800/7800 hits, clamp census plausible (castle shells + rim) | same log |
| 4 | 687's three legibility fixes | **PASS on pixels:** status line "War map" white/outlined, fully CLEAR of the deck bar (VID-003 S3 healed); "Reveal Enemies (30 Gold)" complete INSIDE its button chrome (no "Rev" spill, no truncation); the Reveal/Close pair horizontally clear of the Rally HUD (Rally: Ready +1/s 0/6 unobstructed) | capture #1 |
| 5 | Castle icon/marker coincidence, BOTH castles | **PASS, pixel-sampled:** own castle — team-BLUE icon `RGB(63,149,255)` peeking from behind the topmost `own_castle` marker; enemy castle — team-RED icon `RGB(255,89,63)` behind the topmost `enemy_cas…` marker. Markers topmost confirmed | capture #1 + crops |
| 6 | Marker click → symbol in the console box, never submitted | **PASS (delegate-lane):** `OnPlacePicked.broadcast("own_castle")` through the REAL bound handler → console auto-opened (the TASK-561 contract) → InputBox read back **`own_castle `** → controller receipt logged: *"war-map marker click inserted one place symbol into the console input box (not submitted)."* Enter never pressed; console then closed, discarding the text; map closed; posture released | `TASK-689-after-click-console-insert.png` |
| 7 | Stood-down/degraded paths (not forced) | **Observed live, graceful:** a broadcast that landed 8 s AFTER a match-end was refused exactly per the ladder — *"assistant console open refused — … the match has ended (1)"* + *"the click is lost, not mis-delivered"* (one Warning, no crash). Blue ally dot verified isolated in the ended-match capture: solid team-blue `(63,149,255)` ~8×9 px square at the live pawn position | `TASK-689-endstate-destroyed-castle-iconless.png` |

Bonus observations (for 690's sheet context, all consistent with the rulings): the ended-match
capture shows the DESTROYED own castle correctly ICON-LESS while the live red castle keeps its
icon (the 685 deviation-1 behavior, live); a destroyed castle's elevation block also vanished on
the NEXT open's re-bake (freshly-baked post-destruction — WARN-2's cache concern only bites
mid-match with the map cached, which matches the gate's wording exactly).

## 5. FINDINGS FOR THE MANAGER (report-only, none block the wave)

1. **The RED AI razes an AFK Blue castle in ~3 min** (two consecutive unattended matches:
   3m11s and ~3m). Fine for testing; worth a pacing look if 690's playtest feels rushed.
2. **`enemy_castle`'s label** extends past the right edge of the map panel at 1600×900 (the
   marker sits at the map's edge; the label text runs right). Cosmetic; 690's eye should rule.
3. **`DefaultEngine.ini`** carries the unclaimed `bAllowHighDPIInGameMode=True` (see §1) —
   needs Jonathan to claim (then commit separately) or revert.
4. The seed log + bake log both print flawlessly as 690's paste-points.

## 6. THE COMMIT

One commit, explicit paths, the enumeration + this handoff + 3 captures + VID-003 docs;
TASKBOARD/CONVENTIONS staged last; `Config/DefaultEngine.ini` excluded (adjudicated, §1).
Hash recorded in the wave's Slack post and the return. ⛔ Not pushed (the law).

## 7. POST STATE

Editor relaunched and left UP (PID in the return), MCP live at `http://127.0.0.1:8000/mcp`,
`L_Arena` hash == ledger, tree clean except the adjudicated ini (+ Saved/ churn).
**TASK-690 (Jonathan's playtest) is the wave's acceptance** — the map is in; a clip can arrive
in 🎬 Footage Review.
