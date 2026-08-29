# QA Report — TASK-693 (the gate on TASK-692, the war-map mirror flip)
Verdict: **PASS**

Blockers: **0** · Warns: **1** · Nits: **2**

Inputs: `handoffs/TASK-692-programmer.md` · `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.{h,cpp}` · `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp` · Law `WM-§7` · `WR-§6` · `AS-§12g` · `SC-§33`.
⛔ No edits, no compile, no git, no board writes — per dispatch.

## 1. The truth table, re-derived from scratch (the double-flip check — every sign my own hand)

**The frame, verified at the ENGINE, not the handoff:** `FVector::RightVector = (0,1,0)` — `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Core\Public\Math\Vector.h:96-97` (`LeftVector = (0,-1,0)` at :99-100). At identity yaw the right vector IS world +Y. The player exits the Blue gate facing battlefield centre = **+X** — confirmed at `Castle.cpp:1478-1483` (TASK-662 rider: Blue yaw +90, **gate faces +X**). Facing +X ⇒ **his RIGHT = +Y**. Positive yaw carries +X onto +Y and reads CLOCKWISE from above, so on a top-down map drawing +X to the RIGHT, +Y lies 90° clockwise = the map's **BOTTOM**. His right must draw at the bottom.

**Reference actors verified at source/record (not memory):** castles `BattlefieldScatter.cpp:2632` (Blue −25000 / Red +25000, corroborated by the fallback literals at :1175/:1179); the mine verbatim at `Saved/Logs/GitClaudeUnrealTest_2.log:1919` — `MinesPass seed=704140289 … [0] P=(-17358,6103,0)`.

Recomputed under both V-lines (Half = 26000/12000):

| Actor | World XY | OLD V=(HalfY−Y)/2HalfY | NEW V=(Y+HalfY)/2HalfY | U (both) |
|---|---|---|---|---|
| Castle_Blue | (−25000, 0) | 0.5000 | 0.5000 | **0.01923** (left) ✅ |
| Castle_Red | (+25000, 0) | 0.5000 | 0.5000 | **0.98077** (right) ✅ |
| Mine pair[0] P | (−17358, **+6103**) | **0.24571 — TOP = his LEFT = the reported mirror** ⛔ | **0.75429 — BOTTOM = his RIGHT** ✅ | 0.16619 |

Old 0.2457 / new 0.7543 (complements sum to 1 — one clean mirror, not a rotation). **Concur with the handoff's diagnosis in full: the V-line was the liar, U was healthy, and the shipped fix draws +Y at the bottom — the correct side. No double flip: exactly ONE sign change in the forward line + its exact inverse, and the file-wide sweep (below) found no second flip site.**

## 2. The flip is confined to the pair, and the pair is an exact inverse

- `WorldToMapUV` (`WarMapWidget.cpp:361-385`): `MapUV.Y = Clamp((Y + HalfY)/(2·HalfY))` at :382; U-line untouched at :373.
- `MapUVToWorld` (:437-454): `Y = (2V − 1)·HalfY` at :453. Hand-derivation: `V=(Y+HalfY)/(2HalfY) ⇒ 2HalfY·V = Y+HalfY ⇒ Y=(2V−1)·HalfY` — **exact inverse**, same 1-uu floor applied to the same axes in both directions (:367-368, :441-442); round trip closes byte-exact on (0,1)², degenerate path recomputed by hand: UV(0.25,0.75) → (−0.5,+0.5) → (0.25,0.75) ✅.
- **No-other-sign-work sweep re-run** (grep `HalfY -`, `1.0 - `, `- WorldXY.Y`, `- MapUV.Y`, `1.f - `, `(1 - 2` across `WarMapWidget.cpp`): the ONLY hit is the historical comment at :378. Call sites all route through the pair with zero local sign work — markers :1399-1402, POI icons :1503/:1512/:1526/:1535, ally dots :1545, enemy dots :1558, elevation bake :992 (per-texel `MapUVToWorld`), click resolution via `BuildMarkerRects` (:1565-1566, same builder as the painter). `MapUVToLocal` / `ComputeMapRectLocal` / `FindMarkerIndexAtLocal` untouched and orientation-neutral (read in full).
- **The bake's "Row 0 = UV.Y 0 = the TOP of the drawn map" comment (:985-988) was CORRECTLY left alone** — verified reasoning: it relates texture row ⇔ UV.Y ⇔ Slate rect top, all UV/screen-space facts independent of which WORLD Y maps to UV.Y 0; the world enters that loop only through `MapUVToWorld` at :992. Convention-independent, still true.
- Stale-narration sweep (`grows UP`/`screen up`/`UPWARD` over all three fenced files): **zero matches.** The h :96-109 block and cpp :375-381/:444-450 comments state the new truth with the derivation.

## 3. Tests — every re-pinned expectation hand-recomputed against the NEW line

All seven named sites verified; a file-wide grep confirmed **no other world-Y-sign-dependent expectation exists outside them** (the sole `Arena.Y` hit outside, :645, is the aspect ratio — neutral).

| Site | Hand-check |
|---|---|
| 1 (:347-374) | (−HX,−HY)→(0,0) top-left · (+HX,−HY)→(1,0) · (−HX,+HY)→(0,1) · (+HX,+HY)→(1,1); shipped top-left (−SX,−SY)→(0,0) ✅ |
| 2 (:404-435) | V strictly INCREASES on the sweep (V'=1/2HalfY>0) ✅; YOnly (0, HY/2) → V=0.75 ✅ |
| 3 (:491-497) | (−40HX,−40HY)→(0,0) · (+40HX,+40HY)→(1,1) · (0,−40HY)→(0.5, **0.0**=TOP) ✅ |
| 4 (:561-565) | (2,−2) on floored (1,1): U=1.5→1.0, V=−0.5→**0.0** top edge ✅ |
| 10 (:908-915) | four world corners → the matching rect pixels, + centre ✅ |
| 25 (:2061-2127) | (a) UV(0,0)→(−HX,−HY), UV(1,1)→(+HX,+HY), shipped ditto ✅; (b) lattice round trip convention-neutral, untouched ✅; (c) Y=(2·0.75−1)·1=**+0.5** ✅ |
| 26 (:2220-2239) | (b) (−HX,−HY) fixture → rect top-left; (+3HX,+4HY) → clamp (1,1) → bottom-right ✅; (a) clamp lattice untouched ✅ |

**The new absolute-side block is real and is the case that would have caught the mirror** (:451-460): Blue (−25000,0) U<0.5 · Red (+25000,0) U>0.5 · mine (−17358,+6103) V>0.5, all against the CDO-read shipped extent (not transcribed). **Resize-proof as claimed:** under the clamp, U<0.5 ⇔ world X<0 and V>0.5 ⇔ world Y>0 for ANY positive extent — only a re-mirrored axis can flip them.

**Suite census: 143 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros across 12 test files, 26 in `SiegeWarMapTest.cpp` — delta 0 confirmed by my own count.** One file (no second war-map test file). TASK-694's gate: **143/143**.

## 4. The three declared deviations — rulings

1. **⭐ WM-§7 parenthetical refutation — UPHELD; the CODE is correct.** The law's aside (CONVENTIONS.md:4423: "facing +X, his LEFT = world +Y") is refuted twice over, by my own hand: (a) the engine — `RightVector=(0,1,0)`, so RIGHT=+Y, LEFT=−Y; (b) the symptom itself — the pre-692 map drew +Y at the map top, and map-top IS the reader's left when forward is drawn map-right (rotate the map 90° CCW to align forward), so if his left truly were +Y the shipped map would have looked CORRECT and there would have been no report. The symptom — which the same law names as the binding truth — forces LEFT=−Y; honoring it over the aside is the law working as written. Either reading indicts the same V-line. **→ WARN below: the law line needs a manager rider — a wrong aside in law replicates (the ResolveHeroStart lesson).**
2. **Transcribed actor coordinates — RATIFIED.** The `CanonicalPlaceSymbols` precedent stands in this very file (:215-232: "reading the value under test from the code under test asserts nothing"); the ±25000 literals are independently shipped as engine-side fallbacks (`BattlefieldScatter.cpp:2632/:1175/:1179`); the claims are half-plane, so they survive an arena resize; the extent itself stays CDO-read.
3. **The seeded-mine argument — RATIFIED.** V>0.5 ⇔ Y>0 holds for every world point with positive Y under any seed and any positive extent; the specific record (verified verbatim in the log, :1919) is a measured sign-pin, exactly as argued.

## 5. Cross-cutting

- **Compile-trap scan:** all changed regions read line-by-line — declarations/definitions consistent (h :120/:167 vs cpp :361/:437, no signature change), struct-init tables well-formed, fixture helpers pre-existing, Unicode-in-`TEXT()` matches the file's long-standing shipped precedent. Nothing compiled here — 694 owns the proof.
- **SC-§33:** zero defaulted parameters in any `FSiegeWarMapProjection` declaration; no signature changed. ✅
- **M8:** declaration present (handoff §4) and true — the pair is a static plain struct; the diff adds no UPROPERTY, no replication, no RPC, no class tier. ✅
- **Airlock:** zero `Capture()`/`EnsureSnapshot()` CALL sites in the diff (every grep hit is a pre-existing comment or the TASK-691 log string); no token figure (AS-§12g — the file's own :47 ban stands); Zone A files outside the fence; no console sentence, no snapshot surface. ✅
- **Fence:** content-sweep of the three declared files is clean and no OTHER file shows war-map sign work; QA has no git lane, so the commit-scope confirmation (exactly these three source files + pipeline files) rides 694's cargo check as usual.

## Findings

- [WARN] `.claude/pipeline/CONVENTIONS.md:4423` — WM-§7's frame parenthetical "facing +X, his LEFT = world +Y" is arithmetically wrong (engine: `RightVector=(0,1,0)`; symptom: were LEFT=+Y, the pre-692 map would have been correct and unreported). The binding symptom clause and everything else in WM-§7 are sound. — **Fix: manager one-line rider correcting the aside to "his RIGHT = world +Y (his LEFT = −Y)"; do not let it stand — a wrong aside in law replicates.** Not a code defect; does not gate 694.
- [NIT] `handoffs/TASK-692-programmer.md` §1 — cites "Castle.cpp:1486-1487 (both yaw 0, TASK-617 C1)"; the file's own rider at :1478-1483 declares that parenthetical historical (TASK-662: Blue +90/Red −90, gates facing centre). The cited POSITIONS survive any yaw and the facing fact is strengthened, so nothing load-bearing changes. Record accuracy only.
- [NIT] Test 2's absolute-side strings embed `≈`/`↔` in `TEXT()` literals — consistent with the file's shipped Unicode precedent, flagged only so 694 knows any hypothetical C4819-class noise is cosmetic, not a defect.

## Notes for build-master (TASK-694) — the contract

1. **Compile** via the CLAUDE.md Build.bat line; ⛔ parse the log for `Result:` (the exit-code lie law), never `$LASTEXITCODE`.
2. **Suite: expect 143/143** (delta 0, my own macro census).
3. **The re-shot first-open capture (689 machine route), read against THAT boot's own `MinesPass … [i] P=(x,y,0)` line:** any mine with **y > 0** must draw its gold pickaxe in the map's **BOTTOM half** (pre-692 it drew top); any y < 0 mine in the TOP half; **Blue castle in the LEFT half, Red in the RIGHT** (own-left/enemy-right unchanged); any ally dot present judged the same way. Declare the input lane honestly per the spec.
4. **Commit cargo:** exactly `WarMapWidget.h` + `WarMapWidget.cpp` + `Tests/SiegeWarMapTest.cpp` + pipeline files (board/handoffs/qa), message naming TASK-692..694. ⛔ Never push. Then TASK-690 re-arms for Jonathan's retest of the FLIPPED map.
5. Carry the WARN upstream: the CONVENTIONS.md:4423 one-line rider is the MANAGER's edit, not yours.

— qa-reviewer, 2026-08-29
