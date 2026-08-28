# TASK-664 — [ROT-3] `Castle.{h,cpp}`: the F1 anchor dispositions + the in-file world-frame comment riders (gameplay-programmer)

**Status: COMPLETE → ready-for-qa (orchestrator flips the board — the no-board fence honoured).** Date: 2026-08-27.
Files touched: `Source/GitClaudeUnrealTest/Siegebound/Castle.h` + `Castle.cpp` — **ONLY these two** (the 664∥665 file fence). Pre-edit both files were clean vs HEAD; the entire working diff on them is this task's (git stat: cpp 59+/61−, h 46+/0−; the 61 deletions are exactly the two retired constructor blocks).
Fences honoured: ⛔ no compile (667 owns, QUIET-MODULE) · no editor/MCP/git/board · no seal/hull/manifest figure written (F1-R3) · no gameplay/HP/team logic · no new defaults of any kind (trailing-default law: nothing to audit — zero signatures changed) · compile-trap laws (no literal `*/` inside any comment; zero log literals touched, so no TEXT() surface moved).
Every figure below recomputed from `handoffs/TASK-663-buildmaster.md` (the RELAYED-DIAGNOSIS law), not from spec transcriptions.

---

## 1. THE DIFF — the four ruled dispositions

| family | ruling | executed |
|---|---|---|
| **GateBanners** | KEEP, zero-line | **ZERO-LINE — verified.** Constructor block (`Castle.cpp:470-479`) and the `GateBannerAnchors` property doc (`Castle.h:619-627`) byte-untouched; the CDO default stays 2 anchors at local (±1400, −3675, 0), yaw −90. |
| **TramplePath** | RETIRE (item 2) | Constructor population (13 anchors: 6-seg south leg + corner + 5-seg west leg + turn-in, old cpp :466-509) **REPLACED by a dated retirement comment** (`Castle.cpp:481-495`). `TramplePathAnchors` is now DEFAULT-CONSTRUCTED EMPTY. |
| **ToeRocks** | RETIRE, ⛔ no re-aim (item 3) | Constructor population (10 golden-angle rocks on the d1 rim, old cpp :511-526) **REPLACED the same way** (`Castle.cpp:497-511`). `ToeRockAnchors` DEFAULT-CONSTRUCTED EMPTY. Retirement note records the no-re-aim ground (663 §2 forward stations EMPTY ×3 both castles) and the mooted 659 §5.3 eye-call. |
| **Machinery** | STAYS | Untouched: `SpawnDiscoverabilityFurnishings` / `SpawnDiscoverabilityMesh` / `DestroyCastleFurnishings`, all four soft mesh refs (`GateBannerMeshAsset`, `TramplePathMeshAsset`, `ToeRockMeshAsset01/02` — still SET to the /Game/Meshes/SM_Castle_* paths), the one-shot missing-mesh log guards, the three `UPROPERTY` arrays themselves, the imported assets. Reversal = re-author anchors (this file pre-664 in git / TASK-661 handoff §2 tables) or a BP defaults edit — zero code changes. |

The F1 constants (`Castle.cpp:180-243` region: `HeroStopLaneX`, `SealRim*`, `ToeRock*`, `WrapLaneY`, `Trample*`) **stay as the recorded derivation for reversal** — namespace-scope `constexpr`, legal unused, no MSVC warning class (C4189 is locals-only). Declared for 667: if the toolchain ever surprises with an unused-const diagnostic on these, that is this task's diff to answer for, not a mystery.

## 2. THE EMPTY-FAMILY NO-OP PROOF, AT SOURCE (the ATorch law, verified not assumed)

Read at `ACastle::SpawnDiscoverabilityFurnishings` (Castle.cpp, the spawn lanes — untouched by this diff):

- **TramplePath lane:** `TramplePathMeshAsset` is SET and resolvable (TASK-657's import landed; 663 measured 13/13 live pre-retirement), so `LoadSynchronous()` succeeds and the `for (const FTransform& PathAnchor : TramplePathAnchors)` loop executes **zero iterations** on the empty array → zero components, zero log lines. The `else if (!bLoggedTramplePathMeshMissing)` "family skipped" line CANNOT fire (the mesh resolved). `PathCount` stays 0.
- **ToeRock lane:** both rock meshes resolve → the indexed loop `RockAnchorIndex < ToeRockAnchors.Num()` runs zero times → zero components, zero log lines; the missing-mesh `else if` cannot fire. `RockCount` stays 0.
- **The only observable** is the pass-summary line (§3). No warning, no error, no per-family line — byte-zero log noise beyond the summary, on both castles, on every BeginPlay/Play-Again edge (the lane re-runs identically).
- **Declared residual (deliberate, per the ruling's machinery-stays clause):** `LoadSynchronous()` still loads `SM_Castle_TramplePath` / `SM_Castle_ToeRock01/02` into memory even though zero instances spawn. That IS the reversible posture the ruling ordered (soft refs stay SET); a designer opt-out (clearing the refs) remains available and was not exercised — clearing them is not in my scope.

## 3. THE DECLARED LOG FORM — 667's grep surface, verbatim

The existing format string (`Castle.cpp:1005-1010`, **byte-untouched**):
`ACastle '%s': F1 discoverability set — %d/%d banners, %d/%d path segments, %d/%d toe rocks spawned (TASK-661; every component forced NoCollision — GH-R9).`

With the post-664 CDO defaults (banners 2, path 0, rocks 0) it renders, per castle:

**`F1 discoverability set — 2/2 banners, 0/0 path segments, 0/0 toe rocks spawned`**

— exactly the ruling-item-4 shape as a verbatim substring (the `ACastle '<name>':` prefix and `(TASK-661; …)` suffix frame it, unchanged). **No format-string adjustment was needed — zero bytes changed on any log literal.** Both castles emit it (per-instance lane); the torch summary line (`furnished — %d of %d torch anchors spawned (cap %d), commander %s …`, Castle.cpp:857-861) is **byte-form intact** — untouched.

## 4. THE ROT-§4 COMMENT RIDERS — the site list at real post-edit lines

Dated riders (the TASK-623/637 model — new truth beside the old, history never rewritten). The five board-pinned 637/623 sites were re-located BY CONTENT and all five found at their expected content anchors:

| # | site (post-edit) | was | rider |
|---|---|---|---|
| 1 | `Castle.h:78-83` (class doc, furnishing reason 2) | 637 rider: "yaw 0 … gates face world −Y" | TASK-662 rotated: Blue +90 / Red −90, gates face centre; 663 read the furniture riding it byte-true. |
| 2 | `Castle.h:210-214` (`GetInteriorAnchorLocation` doc, after the 623/CR-R6 rider) | same claim | the level edit arrived; transform form absorbed it, ZeroVector point unchanged. |
| 3 | `Castle.h:493-495` (`InteriorAnchorRelativeLocation` doc, after the 637 rider) | same claim | yaws now live +90/−90; ZeroVector pose-proof, unchanged. |
| 4 | `Castle.cpp:755-761` (`SpawnCastleFurnishings` composition comment, after the 637 rider) | same claim | rotated per 662/663; "correct at ANY pose" carried its first real pose change. |
| 5 | `Castle.cpp:1478-1483` (`GetInteriorAnchorLocation` body, between the 637 rider and RESOLVED WORLD POINTS) | "BOTH yaw 0 — TASK-617 C1" | superseded; the resolved points (−25000,0,0)/(+25000,0,0) survive "under any yaw" exactly as their own note promises. |
| 6 | `Castle.cpp:166-179` (F1 constants block header) | "local +X is the face a straight run from the Blue spawn hits" — a WORLD-frame claim now false (663 §1: the spawn sits ON the gate axis) | rider: straight run now hits the MOUTH; compass words in the lane = 656's castle-LOCAL shorthand; trample/toe-rock present-tense premises = the RETIRED defaults; banners = the KEPT family. Covers the block's premise statements (`HeroStopLaneX` :186, `MouthStepLineY` :210 docs) in one place. |
| 7 | `Castle.h:633-648` (F1 header block above the anchor properties) | "nothing tells the player the door is south" + the three-family premise | rider: pre-rotation record; door now faces the centre, hero spawns 292 uu out ON the axis; ruling KEEPS banners / RETIRES path+rocks; compass = castle-LOCAL shorthand. |
| 8 | `Castle.h:668-676` (`TramplePathAnchors` doc) | retired-default description | RETIRED EMPTY rider (ruling item 2) — doc above kept as the record; property/lane/asset stay. |
| 9 | `Castle.h:688-695` (`ToeRockAnchors` doc) | retired-default description | RETIRED EMPTY, ⛔ NO RE-AIM rider (ruling item 3). |
| 10 | `Castle.cpp:481-495` / `:497-511` | the two constructor blocks | the retirement records themselves (§1). |

**Castle-LOCAL truths deliberately NOT ridden** (touching a true comment = a QA finding, ROT-§4): the local −Y gate/corridor statements, "+Y deeper into the keep", every torch anchor derivation (`Castle.h:536` "yaw 0 = the wall is to my…" is torch-relative), the commander anchor block (Castle.cpp :425-455 — all local), `GateBlockerVolume` relative transform, and the **GateBanner block including its "due SOUTH / west / east of the mouth" wording** — adjudicated as the 656 record's castle-local shorthand (the block-header frame declaration, now carrying rider #6, defines it; and ruling item 1 pins banners zero-line, which a rider inside their block would violate). Historical VID-001/656 measurement citations stay as history throughout.

## 5. M8 DECLARATION (verbatim, as ordered)

**Expected and TRUE: this diff is CDO defaults + comments only, on Tier-C cosmetic furnishing (not replicated, no gameplay truth), with the replication/authority posture unchanged.** The two emptied arrays feed the deliberately UN-authority-gated local-projection lane (both machines build identical — now identically smaller — local sets from the same `EditDefaultsOnly` defaults; the class-doc M8 statement stands as authored). No HasAuthority surface, no RPC, no replicated property, no gameplay/HP/team logic moved. Nothing false to declare — no STOP condition met.

## 6. GREPS FOR QA (TASK-666)

- `TASK-664` in `Castle.{h,cpp}` → exactly the 11 rider/retirement sites of §4 (6 in .h, 5 in .cpp).
- `TramplePathAnchors.Add|ToeRockAnchors.Add` → **zero matches** (the retirement); `GateBannerAnchors.Add` → exactly 2 (untouched).
- `TramplePathAnchors|ToeRockAnchors` outside `Castle.{h,cpp}` → zero matches anywhere in `Source/` (verified this task — no test pins the old 13/10 counts; the suite count is untouched by this diff).
- `F1 discoverability set` → one format string, byte-identical to pre-664 (`git diff` shows no hunk at it).
- The 664∥665 fence: `git status` on my side shows modifications to exactly the two owned files.

## 7. SC-§15 DECLARED DEVIATIONS / JUDGEMENT CALLS

1. **Rider #6 (cpp constants-block header) and #7 (h F1 block header) are sweep finds beyond the five board-pinned sites** — both state world-frame facts the rotation falsified ("straight run from the Blue spawn hits local +X", "the door is south"); the frames law demanded them.
2. **Banner-block compass words left untouched** despite citing directions — adjudication in §4; ruling item 1's zero-line pin outranks a cosmetic rider, and the region's frame is now declared once at rider #6.
3. **The F1 constants stay though two families' consumers are gone** — recovery data by design (§1); flagged for 667's compile as the one place an over-zealous toolchain could grumble.
4. **`SpawnDiscoverabilityFurnishings`'s own doc (`Castle.h`) and lane labels ("trample path (the guided route)") not ridden** — they describe MACHINERY (which stays and still handles all three families correctly, empty or not), not the map; the retirement is recorded at the properties and constructor those docs point to.

## 8. FOR 666/667

- 667's furnishing grep flips to the §3 declared form; torch line unchanged. Suite expectation from THIS diff: unchanged (665 declares its own additions).
- Live re-verify expectation: 2 banner components and ZERO path/rock components per castle in SIE, zero new log lines, Play-Again edge identical.
