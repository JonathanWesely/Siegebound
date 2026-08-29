# TASK-692 — the war-map MIRROR fix (gameplay-programmer handoff)

Status: **ready-for-qa** (TASK-693). File-only per spec — ⛔ no compile (TASK-694 owns), no editor, no Git, no board writes beyond the 692 status flip.

## 1. THE DIAGNOSIS — the truth table (WM-§7: symptom over axis wording)

**Reference actors, world XY read from source / the measured record — ⛔ not from memory:**

- Castles: `BattlefieldScatter.cpp:2632` (`Blue −25000, Red +25000`, Y 0) + `Castle.cpp:1486-1487` (both yaw 0, TASK-617 C1).
- Mine: the recorded live MinesPass line, `Saved/Logs/GitClaudeUnrealTest_2.log` `[2026.08.29-21.43.54]`:
  `MinesPass seed=704140289 … pairsPlanned=3 minesSpawned=6 … [0] P=(-17358,6103,0) M=(17358,-6103,0) …`
  → **GoldNode pair[0] primary, world (−17358, +6103)**. (Mines are re-seeded per match; this is a *measured example* that pins the sign — the test's claims are half-plane claims, so they hold for every seed.)
- Arena half-extent: `USiegeScatterConfig::ArenaHalfExtent` CDO `(26000, 12000)` — always passed as a parameter, never transcribed into the projection.

**The player's frame, worked out explicitly (⛔ not guessed from "horizontal axis"):**

1. He spawns outside his own (Blue) gate facing battlefield centre ⇒ **facing world +X**.
2. UE's world frame is **left-handed** (X forward, Y right, Z up): at identity yaw `GetRightVector()` **is +Y** (equivalently: yaw is clockwise seen from above, and +90° yaw carries +X onto +Y). ⇒ facing +X, **his RIGHT = world +Y, his LEFT = world −Y**.
3. Viewed from above, a person's right lies 90° **clockwise** from their forward. The map draws his forward (+X) to the map's **RIGHT** (the U line — both castles confirm it, and the TASK-689 capture showed own castle left / enemy right, which Jonathan did not dispute). 90° clockwise from map-right is map-**BOTTOM**. ⇒ **on a correct map, world +Y (his right) draws toward the map's BOTTOM; world −Y (his left) toward the TOP.**

| Actor | World XY (source/record) | Pre-692 UV (shipped) | Required side from his seat | Verdict |
|---|---|---|---|---|
| Castle_Blue (own) | (−25000, 0) | (0.0192, 0.5000) — far LEFT, centre | map WEST/LEFT half (behind him), vertical centre | ✅ U axis healthy |
| Castle_Red (enemy) | (+25000, 0) | (0.9808, 0.5000) — far RIGHT, centre | map EAST/RIGHT half (ahead), vertical centre | ✅ U axis healthy |
| GoldNode seed 704140289 pair[0] P | (−17358, **+6103**) | (0.1662, **0.2457**) — TOP quarter | +Y = his **RIGHT** ⇒ map **BOTTOM** half (required V = 0.7543) | ⛔ **MIRRORED** — drawn on his LEFT |

**The mirrored axis: the world-Y → UV.V line of `WorldToMapUV`** (the map's vertical axis — i.e. the map was flipped about its horizontal axis, so his wording happens to agree, but the arithmetic above is the authority). The shipped `V = (HalfY − Y)/(2·HalfY)` drew +Y at the map TOP; every actor on his right rendered on his left and vice versa — Jonathan's exact symptom. U is proven healthy by both castles + the absence of any front/back complaint.

**Root cause of the original error:** the pre-692 comment reasoned "Slate's local Y grows DOWNWARD, so invert." In UE's left-handed frame, a top-down map with +X to the right must draw +Y downward — Slate's downward Y was already the correct direction; the "flip" itself created the mirror.

**Consistency proof against WM-§7's frame parenthetical** (declared, SC-§15): the law's aside "facing +X, his LEFT = world +Y" is arithmetically inconsistent with the symptom — if his left were +Y, the shipped map (which drew +Y on the top = his claimed left) would have looked CORRECT and there would have been nothing to report. The symptom + UE's right-vector fact force LEFT = −Y. The law itself names the symptom as the binding truth, so this is the law working as written, not a departure from it. Either reading indicts the SAME line (the V line), so the fix is unambiguous.

## 2. THE FLIP — one change, in the single owner, both directions together

`Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp`, `FSiegeWarMapProjection` (the pair at :361-456):

- `WorldToMapUV`: `MapUV.Y = Clamp((HalfY − WorldXY.Y)/(2·HalfY))` → **`Clamp((WorldXY.Y + HalfY)/(2·HalfY))`**. U line untouched.
- `MapUVToWorld`: `Y = (1 − 2V)·HalfY` → **`Y = (2V − 1)·HalfY`**. X line untouched.

**Inverse algebra, re-derived (stated in the code comment):** `V = (Y + HalfY)/(2·HalfY) ⇒ Y = (2V − 1)·HalfY`. Same 1-uu zero-divide floor on the same axes in both directions ⇒ the round trip stays byte-exact on (0,1)², including the degenerate-extent path (test 25(c) re-verified by hand: UV (0.25, 0.75) → world (−0.5, +0.5) → UV (0.25, 0.75)).

⛔ **No flip anywhere else** (WM-§7 single-owner law) — verified by grep after the edit: every call site routes through the pair with zero local sign work — markers (:1400), POI icons (:1504/:1513/:1527/:1536), ally dots (:1546), enemy dots (:1559), the elevation bake (:992, per-texel `MapUVToWorld`), click resolution (rects built from the same chain). All layers re-orient coherently from the one flip. The bake's "Row 0 = UV.Y 0 = the TOP of the drawn map = texture row 0" comment (cpp ~:985) is a UV-space claim and stays TRUE — untouched on purpose.

**Narration corrected in the same diff** (the WM-§7 comment-follows-code clause):
- `WarMapWidget.h` :96-109 — the pinned orientation block now reads `UV.Y = 0 at world Y = −HalfY ⇒ world +Y grows DOWN`, with the left-handed-frame rationale and the pre-692 mirror named.
- `WarMapWidget.cpp` :375-381 (forward) and :444-450 (inverse) — same truth, plus the derivation.
- No stale direction comment survives in the fenced files (post-edit grep for `grows UP` / `screen up` / `(HalfY -` / `1.0 - 2.0` is clean; remaining `+HalfY` mentions all state the NEW bottom-side convention or the pre-692 history).

## 3. TESTS — updated IN PLACE (`Tests/SiegeWarMapTest.cpp`, no second file)

| Test (unchanged names) | Change |
|---|---|
| 1 `ProjectionCentreAndCorners` | Corner table re-pinned: (−HX,−HY)=top-left … (+HX,+HY)=bottom-right; shipped-extent top-left check now (−SX,−SY); block comment states the new truth |
| 2 `ProjectionOrientationIsPinned` | Sweep now asserts UV.Y strictly **INCREASES** (world +Y grows DOWN); YOnly spot pin 0.25 → **0.75**; **+ the new absolute-side block (below)** |
| 3 `ProjectionClampsOutOfBounds…` | Corner-pin cases re-signed; `far past −Y` now pins V to the **TOP** (0.0) |
| 4 `ProjectionSurvivesADegenerateArena…` | Off-centre (2,−2) on the 1-uu floor now pins to the **top** edge (V 0.0) |
| 10 `FullChainMapsWorldCornersToRectCorners` | The four world-corner → pixel rows re-signed to match test 1 |
| 25 `MapUvToWorldInvertsTheProjection` | (a) UV(0,0) = world (−HX,**−HY**), UV(1,1) = (+HX,**+HY**), shipped ditto; (c) degenerate Y = (2·0.75−1)·1 = **+0.5**. (b) round-trip lattice byte-identical — untouched |
| 26 `PoiIconProjectionStaysInsideTheMapRect` | (b) corner-castle fixture moved to (−HX,−HY) for the top-left pin; far-out fixture re-signed to (+3HX,+4HY) for the bottom-right pin. (a) clamp lattice untouched |

**⭐ The new ABSOLUTE-SIDE case (the one that would have caught this)** — extends test 2 (`Siegebound.WarMap.ProjectionOrientationIsPinned`): every pre-existing assertion (and the round trip) stays green under ANY self-consistent sign convention; only tying REAL actors to an ABSOLUTE map side fails on a coherent mirror. Using the truth table's actors against the shipped CDO extent:
- Blue castle (−25000, 0) → **U < 0.5** (map's west/left half);
- Red castle (+25000, 0) → **U > 0.5**;
- the recorded +Y mine (−17358, 6103) → **V > 0.5** (map's bottom half — the pre-692 transform put it at V≈0.246, the exact reported mirror).
Half-plane claims only, so an arena resize cannot break them — only a re-mirrored axis can. The castle/mine coordinates are transcribed **deliberately** (the suite's `CanonicalPlaceSymbols` precedent, argued in the comment): an expectation read from the code under test asserts nothing.

**Suite delta: 143 → 143 (delta 0).** No `IMPLEMENT_SIMPLE_AUTOMATION_TEST` added or removed — the absolute-side pin lives inside the existing orientation case. TASK-694's gate expects **143/143**.

## 4. AIRLOCK + LAWS

- ⛔ No `Capture()` / `EnsureSnapshot()`, no console sentence, the 552 latch untouched and unspent, Zone A byte-untouched, ⛔ no token figure anywhere (AS-§12g).
- `SC-§33`: zero defaulted parameters added; no signature changed.
- **M8 declaration: unchanged posture — this diff adds NO replicated property, NO new class tier, NO RPC. The war map remains client-local display; the flip is two arithmetic signs and their tests.**
- Fences held: `WarMapWidget.h`, `WarMapWidget.cpp`, `Tests/SiegeWarMapTest.cpp` — nothing else touched.

## 5. DECLARED DEVIATIONS (SC-§15)

1. **WM-§7's frame parenthetical ("his LEFT = world +Y") is refuted by the arithmetic** (§1 above); the symptom — which the same law names as the binding truth — was honored instead. The diagnosed line is the same either way.
2. **Transcribed actor coordinates in the new test block** (castles ±25000, mine −17358/6103) — deliberate, precedent-cited (see §3); the arena extent itself stays untranscribed (read from the CDO).
3. The mine coordinate comes from a **seeded** per-match record (positions vary by seed); it is used as a measured sign-pinning example with half-plane assertions, valid for every seed.

## 6. FOR QA (TASK-693) — scrutiny list

- Verify the pair is an exact algebraic inverse (both axes, incl. the 1-uu floor path) and that **no other file gained a sign change** (the grep in §2 — re-run it).
- Verify every V expectation in tests 1/3/4/10/25/26 matches `V = (Y+HalfY)/(2·HalfY)` by hand — the failure mode of THIS task is a double flip, and screen-vs-world naming is exactly where it hides (WM-§7's own warning).
- Verify the new absolute-side block's numbers against §1's truth table, and that its claims are half-plane (resize-proof), not exact UVs.
- Verify no stale direction comment survives (`grows UP` grep) and that the bake's row-0 UV-space comment was correctly LEFT alone.
- Check I did not touch `MapUVToLocal` / `ComputeMapRectLocal` / `FindMarkerIndexAtLocal` (I did not — orientation-neutral by design).

## 7. FOR INTEGRATION (TASK-694) — the capture check against the truth table

After compile + 143/143: re-shoot the first-open capture (the 689 machine route), read that boot's own `MinesPass … [i] P=(x,y,0)` log line, pick any mine with **y > 0**, and confirm its gold pickaxe icon draws in the map's **BOTTOM half** (pre-692 it drew top). Castles must remain own-left / enemy-right. That is the pixel form of §1's table; then TASK-690 re-arms for Jonathan's retest.

Files touched:
- `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h` (narration block only)
- `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp` (the pair: two sign flips + comments)
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp` (in-place re-pins + the absolute-side block)
