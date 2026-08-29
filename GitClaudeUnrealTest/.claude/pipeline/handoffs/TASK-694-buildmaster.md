# TASK-694 — build-master handoff — the MAP MIRROR's compile · suite · pixel proof · COMMIT A
2026-08-29/30 · gate input: `qa/TASK-693.md` (**PASS**, 0 blockers / 1 warn / 2 nits)

## 0. EDITOR AUTHORITY — the grant, verbatim

At dispatch the editor-close law was back in force (his 2026-08-28 grants expired). The graceful
`quit_editor()` over the TASK-667 remote-exec lane was **DENIED BY THE PERMISSION CLASSIFIER**;
per the dispatch law I did not force it and did not work around it — I STOPPED and reported.
**Jonathan then closed the editor himself (PID 24412) and granted, verbatim (2026-08-30):**

> **"I give you permission to close and reopen the editor whenever needed."**

Standing for this session; recorded here as the authority for every bounce in the 694→696→697
chain. Protocol still honored at each step: PIE-guard, node-identity probe before any
editor-shaped call (the 678 law), `L_Arena` never saved.

## 1. PRE-FLIGHT

- **Git trio:** HEAD `62df5f7`, **1 ahead** of origin, porcelain = exactly the 9 briefed rows
  (3 Source files · CONVENTIONS · TASKBOARD · `Config/DefaultEngine.ini` · the 692 handoff ·
  `qa/TASK-693.md` · `Docs/setupdirections.md`). **Zero unexplained Source lines.**
- **`L_Arena` law:** sha256 `9ccd54ef…0e58` == the ROT-§2 ledger at dispatch, after the `-game`
  boot, and after the client closed. **Never saved.** ✓
- **Node identity (the 678 law), before any editor-shaped call:** port-8000 owner probed =
  PID 24412 `UnrealEditor.exe` with our `.uproject`; process census 1 editor / **0 `-game`
  clients**; remote-exec enumerated **exactly 1 node**, `project=GitClaudeUnrealTest`.
  A guard in the runner refuses to act on anything but a single identity-matched node.
- **PIE-guard:** `is_in_play_in_editor()` = **False**, editor world `L_Arena`.
- ⚠️ `Config/DefaultEngine.ini` — the unclaimed `bAllowHighDPIInGameMode=True` is **still
  EXCLUDED** from this commit and left in the working tree (TASK-689's adjudication + P3's
  proceeding default, unchanged). It cooks into TASK-696's package as-is, which is the default.

## 2. COMPILE — `Result: Succeeded`

CLAUDE.md Build.bat line, **log-parsed per the exit-code-lie law** (the raw exit code was 0 and
was NOT trusted). 7 actions, **0 errors**, 10.47 s UBA / 12.88 s total. Both flip files rebuilt:
`[1/7] SiegeWarMapTest.cpp`, `[2/7] WarMapWidget.cpp`, then the module + link.
Log: session scratchpad `TASK-694/build-694.log`.

## 3. SUITE — 143/143, delta 0

Headless `UnrealEditor-Cmd -ExecCmds="Automation RunTests Siegebound;Quit" -unattended -NullRHI`.
- **`Test Completed` = 143 · `Result={Success}` = 143 · `Result={Fail}` = 0 · unique test paths = 143.**
- `: Error:` severity sweep over the run = **0**.
- **Independent macro census (my own, not transcribed): 143 `IMPLEMENT_SIMPLE_AUTOMATION_TEST`
  across 12 files, 26 in `SiegeWarMapTest.cpp`** — matches QA's count exactly. Delta 0 confirmed.
- All seven re-pinned orientation cases green by name, including the one carrying the new
  absolute-side block: `ProjectionOrientationIsPinned`, `ProjectionCentreAndCorners`,
  `ProjectionClampsOutOfBoundsInsteadOfDropping`, `ProjectionSurvivesADegenerateArenaExtent`,
  `FullChainMapsWorldCornersToRectCorners`, `MapUvToWorldInvertsTheProjection`,
  `PoiIconProjectionStaysInsideTheMapRect`.
Log: `TASK-694/suite-694.log`.

## 4. ⭐ THE PIXEL PROOF — the re-shot first-open capture vs THIS boot's own MinesPass line

**Capture:** `.claude/pipeline/handoffs/TASK-694-warmap-flipped-first-open.png` (1600×900,
standalone `-game` client on `L_Arena`, the TASK-689 machine route, **first open of the map in
that boot**, engine-side `Shot showui`).

**THIS boot's own log line** (⛔ not a transcribed record — read from `TASK-694/game-694.log`):

```
MinesPass seed=44041537 mineStream=1340689156 pairsPlanned=3 minesSpawned=6 reserve=300:
 [0] P=(-17546,3836,0) M=(17546,-3836,0) hill=no fb=no culls=2
 [1] P=(-24615,-5336,0) M=(24615,5336,0) hill=no fb=no culls=10
 [2] P=(-8499,4932,0) M=(8499,-4932,0) hill=no fb=no culls=0
```

All three pairs straddle y=0 (M is P's 180° rotation), so this seed exercises **both** sides.

**Measured panel rect from the capture's own pixels: x=[40,1559], y=[99,800] (1520×702).**
Icon gold sampled at RGB(255,221,118); centroids clustered; UV = (px−rect0)/rectSize.
Predicted UV from the shipped half-extent (26000,12000) via the NEW line
`V=(Y+HalfY)/2·HalfY`, `U=(X+HalfX)/2·HalfX`.

| Mine | world (x,y) | required half | predicted UV | predicted px | **measured px** | **measured UV** | measured half | Δ | ✓ |
|---|---|---|---|---|---|---|---|---|---|
| pair0 P | (−17546, **+3836**) | **BOTTOM** | (0.1626, 0.6598) | (287.1, 562.2) | (290.2, 558.8) | (0.1646, **0.6549**) | **BOTTOM** | 4.7 px | ✅ |
| pair0 M | (+17546, **−3836**) | **TOP** | (0.8374, 0.3402) | (1312.9, 337.8) | (1314.8, 336.2) | (0.8387, **0.3379**) | **TOP** | 2.5 px | ✅ |
| pair1 P | (−24615, **−5336**) | **TOP** | (0.0266, 0.2777) | (80.5, 293.9) | (81.8, 292.2) | (0.0275, **0.2752**) | **TOP** | 2.2 px | ✅ |
| pair1 M | (+24615, **+5336**) | **BOTTOM** | (0.9734, 0.7223) | (1519.5, 606.1) | (1521.8, 604.2) | (0.9749, **0.7197**) | **BOTTOM** | 3.0 px | ✅ |
| pair2 P | (−8499, **+4932**) | **BOTTOM** | (0.3366, 0.7055) | (551.6, 594.3) | (553.8, 592.2) | (0.3380, **0.7026**) | **BOTTOM** | 3.0 px | ✅ |
| pair2 M | (+8499, **−4932**) | **TOP** | (0.6634, 0.2945) | (1048.4, 305.7) | (1049.8, 304.2) | (0.6643, **0.2923**) | **TOP** | 2.1 px | ✅ |

**ALL SIX MINES ON THE CORRECT SIDE — every y>0 mine draws in the map's BOTTOM half, every
y<0 mine in the TOP half.** Worst residual 4.7 px (icon centroid vs. geometric centre, well
inside one 24-px icon). Pre-692 each of these would have drawn in the mirrored half.

**Castles (team-tinted icons, colour-clustered):**

| Castle | world | required | measured px | measured U | verdict |
|---|---|---|---|---|---|
| Blue (own) | (−25000, 0) | **LEFT** | (51.8, 438.6) | **0.0078** | ✅ LEFT half |
| Red (enemy) | (+25000, 0) | **RIGHT** | (1530.5, 457.0) | **0.9806** (predicted 0.9808) | ✅ RIGHT half |

⚠️ The blue cluster (n=82) legitimately merges the own-castle icon with the adjacent **blue ally
dot**, pulling the centroid slightly left of the predicted 0.0192; the half-plane claim — the
only thing asserted — is unaffected. The red cluster (n=10) is the icon peeking from behind the
topmost `enemy_cas…` marker and matches prediction to 0.0002 U.

**Supporting log evidence, same boot:** `Snapshot seeded at rest (TASK-580): 7 places resolved
(3 region-bearing)` (markers on first open, no sentence) · `[WarMap] Elevation baked: 130x60
samples, 7800 hits, groundZ=0.0, maxZ=2430.0 (relief 2430.0 uu), ceiling=1000.0, 140 texel(s)
clamped full white.`

### ⚠️ LANE DECLARED HONESTLY (spec item 3 — no claims beyond what the route proved)

- **No OS input injection exists and none was claimed.** The map was opened by **faithful
  `GetOrCreateWarMapWidget` replication** over the remote-exec lane — the TASK-689 precedent —
  resolving the controller's own configured `WarMapWidgetClass` (`/Game/UI/WBP_WarMap.WBP_WarMap_C`,
  read from the live CDO, ⛔ not a guessed path), taking the posture through the shipped
  `SetWarMapOpen(true)` **first** (guard-first / UI-second, the shipped ordering), then creating,
  owning, adding to viewport and calling the shipped `OpenMap()`. `IsMapOpen()` = true,
  `IsInViewport()` = true.
- **The proximity gate was satisfied, not bypassed:** the hero was walked to his own-team
  commander (`BP_CommanderNpc_C_0`, 400 uu 2D radius) and the shipped predicates then reported
  `IsHeroInCommanderRange()=True`, `CanOpenWarMap()=True`. ⛔ No gate was stubbed.
- **The three delegate binds were NOT taken** (they are C++ handlers with no reflection surface).
  They govern the **click seam only**, not the drawing — so ⛔ **no click/marker-pick claim is
  made here**; that remains TASK-690's by-hand acceptance, exactly as the wave intended.
- **The hero marker draws at his SPAWN position, not his walked-to position** — correct and
  expected: the TASK-580 snapshot is seeded at rest and moving anchors refresh only at each
  sentence (`W691-1`), and ⛔ **no sentence was sent** (the 552 latch is UNSPENT; Zone A
  untouched; no `Capture()`/`EnsureSnapshot()` call was made).
- **Boot error census: 22 `: Error:` lines, all benign and named** — 6 are my own Python
  API-discovery `AttributeError`s during lane probing; the remaining 16 are pre-existing
  `LogLiveCoding: Cannot enable module …ggml-cpu-*.dll` noise from SiegeLlama's third-party
  binaries. **Zero war-map errors.**

## 5. FINDINGS FOR THE MANAGER (report-only; none block the wave)

1. **⚖️ QA's WARN is still unclaimed — `CONVENTIONS.md:4423`.** `WM-§7`'s frame parenthetical
   *"facing +X, his LEFT = world +Y"* is arithmetically wrong; prescribed replacement verbatim:
   *"his RIGHT = world +Y (his LEFT = −Y)"*. This capture is now the third independent
   refutation (engine `RightVector`, the symptom, and these measured pixels). **Manager's edit.**
2. **📌 `PKG-§2`'s staging path is off by one directory level.** It names
   `…\GitClaudeUnrealTesting\GitClaudeUnrealTest\packagedZIPofGame\`, but the **git root is
   `C:\GitProjects\GitHub\GitClaudeUnrealTesting\`** (porcelain paths are `GitClaudeUnrealTest/`-
   prefixed) and Jonathan's real folder is `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\`.
   The law's stated INTENT ("at the git root") and his folder agree — only the parenthetical
   path is wrong. Rider needed so the wrong path does not replicate.
3. **📌 `Config/DefaultEngine.ini:1-7` comment is factually wrong about scope.** It says the
   remote-exec block is an *"Editor-only plugin setting — no effect on packaged/`-game` builds."*
   **Measured false:** the standalone `-game` client advertised a remote-exec node and executed
   Python over it (that is how this proof was taken). Harmless today, but it is a security-shaped
   claim in a comment — and `Docs/setupdirections.md` repeats the editor-only framing. Worth a
   one-line correction, and worth knowing that **a packaged build may also expose this lane**.
4. `enemy_castle`'s label still runs past the right edge of the panel at 1600×900 (TASK-689
   finding 2, unchanged and still cosmetic) — visible in this capture as `enemy_cas…`.

## 6. THE COMMIT

One commit, explicit `GitClaudeUnrealTest/`-prefixed paths: the 3 source files + the 692 handoff
+ `qa/TASK-693.md` + this handoff + the capture, with **TASKBOARD + CONVENTIONS staged LAST**.
⛔ `Config/DefaultEngine.ini` excluded. ⛔ Not pushed (the law). Hash in the return + Slack post.

## 7. POST STATE

`-game` client closed gracefully (console `quit`); `L_Arena` hash re-verified == ledger.
**TASK-690 RE-ARMS: Jonathan's retest of the FLIPPED map is the acceptance.** The chain continues
into TASK-696 (the cook) and TASK-697 (the zip) in this same session.
