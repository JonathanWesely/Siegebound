# QA Report — TASK-666 (the ROT wave's combined gate: TASK-664 + TASK-665)

**Verdict: PASS — both halves.**
- **TASK-664 (`Castle.{h,cpp}` dispositions + riders): PASS** — 0 blockers, 0 warns, 1 nit
- **TASK-665 (world-frame sweep + spawn-facing + transform pin test): PASS** — 0 blockers, 1 warn, 2 nits

Date: 2026-08-27 · Reviewer: qa-reviewer · Inputs: `handoffs/TASK-663-buildmaster.md` (the record of record, RELAYED-DIAGNOSIS law — every figure below recomputed from it, not trusted), the ROT ACTIVATION RULING (TASKBOARD CASTLE-ROTATION §), CONVENTIONS ROT-§0..§4, `handoffs/TASK-664-programmer.md`, `handoffs/TASK-665-programmer.md`, and raw reads of all seven Source files. Fences honoured: no edits, no compile, no git, no TASKBOARD, no editor.

---

## 1. CROSS-FENCES (spec item e)

- **Wave-marker enumeration:** grep `ROT-§|CASTLE-ROTATION|2026-08-27` across `Source/` hits EXACTLY seven files: `Castle.{h,cpp}` (all markers `TASK-664`) + `SiegeGameMode.{h,cpp}`, `SiegeBotController.{h,cpp}`, `Tests/SiegeCastleTransformTest.cpp` (all markers `TASK-665`). Zero `TASK-665` markers in `Castle.{h,cpp}`; zero `TASK-664` markers outside them. The two halves did not collide.
- **`BattlefieldScatter.cpp`:** zero wave markers; the parked F1-R4 site (:1311-1314, the buried-skirt hill-trace note) reads intact as pre-wave content; its lone resolver-model citation (:2671) is pre-existing. Sweep row 14's "no defect, corridor now gate-to-gate" verdict recomputed: mouths at world (∓21300, 0) sit ON the |Y| ≤ 1000 corridor axis (663 §2/§3) — verify-only was the correct action.
- **Verify-only files clean:** `SummonedUnit.{h,cpp}`, `HeroCharacter.cpp`, `ScatterConfig.h`, war-map lane — no wave markers, landmark reads match pre-wave content.
- **⚠️ DECLARED RESIDUAL (I have no git by design):** this enumeration is content-based (marker greps + landmark reads). An UNMARKED edit in an unreviewed file is not falsifiable from here. **TASK-667's git trio (status/diff --stat/log) is the mechanical confirmation and MUST refuse any Source line outside the §6 seven-file set** (DECK-wave lines excepted per the 667 dated rider — excluded, not refused).

## 2. TASK-664 — VERIFIED AGAINST THE RULING, ITEM BY ITEM

1. **Banners KEEP, zero-line — CONFIRMED.** Constructor block `Castle.cpp:470-479` (Reserve(2) + 2 Adds at (±1400, −3675, 0) yaw −90) and the `GateBannerAnchors` property doc (`Castle.h:648-656`) carry no wave marker and match the 661-authored values 663 read live off the CDO (2 entries, byte-equal). No rider inside the banner block — the zero-line pin honoured; the compass-words frame is declared once at riders #6 (cpp:166-179) and #7 (h:633-646).
2. **Two retirements EXACTLY as ruled.** `TramplePathAnchors`/`ToeRockAnchors` have ZERO `.Add` sites anywhere in `Source/` (grep) — both default-constructed EMPTY; dated retirement records at cpp:481-495/:497-511 cite 663 §1/§2/§3 correctly (dead-flank origin; forward stations EMPTY ×3 both castles; no-re-aim ground; 659 §5.3 mooted). Machinery intact at source: `SpawnDiscoverabilityFurnishings` (cpp:879-988), all four soft refs still SET (:465-468), log guards, the three `UPROPERTY` arrays, `DestroyCastleFurnishings` teardown — untouched.
3. **Empty-array path traced at source — zero log noise PROVEN.** Both meshes are SET and resolvable (663 measured 13/13 + 10/10 live pre-retirement), so `LoadSynchronous()` succeeds, the ranged/indexed loops run zero iterations, and the `else if (!bLogged…Missing)` lines CANNOT fire (they require a resolve failure). Only observable = the pass-summary line.
4. **The pinned log form renders verbatim, zero log-literal bytes changed.** Format string cpp:982-987 (`F1 discoverability set — %d/%d banners, %d/%d path segments, %d/%d toe rocks spawned`) with post-664 CDO counts renders exactly **`F1 discoverability set — 2/2 banners, 0/0 path segments, 0/0 toe rocks spawned`** per castle. Torch line (cpp:834-838) byte-form intact. No `TEXT()` surface moved.
5. **Riders correct under ROT-§4.** All 11 sites (6 in .h: 78, 210, 493, 633-646, 668, 688 — the 633/643 grep lines are one block; 5 in .cpp: 166, 481, 497, 755, 1478) are dated, state the new truth (Blue +90 / Red −90, gates → centre, measured 662/663), and NEVER rewrite the 637/623 record. Castle-LOCAL truths verified un-ridden (torch/commander anchors, GateBlocker relative, "+Y deeper into the keep", banner-block compass shorthand) — no rider on a true comment.
6. **Kept consumerless constants — ACCEPTABLE.** Namespace-scope `constexpr` in a .cpp; MSVC C4189 is locals-only, and no unused-const warning class fires on the UE Win64 MSVC toolchain for internal-linkage namespace constants. 664 flagged it for 667 regardless — correct posture. Compile-trap scan of the diff: comments only (no literal `*/` inside any block comment; zero code lines added), no signature/default change, no seal/hull/manifest figure, no gameplay logic.
7. **M8 declaration TRUE:** CDO defaults + comments on the deliberately un-authority-gated Tier-C local-projection lane; both machines build identically smaller sets; no replication surface moved.

## 3. TASK-665 — THE FACING FIX, RECOMPUTED

- **The math (RULED CORRECT).** `SiegeGameMode.cpp:845-847`: `ToOwnCastle = CastleLocation − OutLocation`; Blue castle (−25000,0,0), spawn (−21008, 0, 100) ⇒ ToOwnCastle (−3992, 0, −100) ⇒ `FMath::Atan2(Y=0, X=−3992)` = +π ⇒ **+180** (atan2(+0, neg) = +π, so −180/"-0" cannot print); Red mirror ⇒ Atan2(0, +3992) = **0**. Matches 663 §1's derivation and the ruling's Blue 180 / Red 0. UE convention verified: forward = (cos yaw, sin yaw) ⇒ yaw = atan2(Y, X) — argument order correct; `FRotator(0.0, FacingYawDeg, 0.0)` = (pitch, yaw, roll) — correct; whole chain double (FVector components double, double literals, `%.0f` vararg promotion legal); degenerate-input proof holds (|ToOwnCastle.X| = SpawnDistance ≥ 1500 > 0). ⛔ No hardcoded yaw — derived from the resolved castle transform.
- **Single resolver CONFIRMED by grep:** `StartRotation` exists ONLY in `SiegeGameMode.cpp` (:281-284 RestartPlayer lane, :617-644 RestoreHeroAtStart lane); `SetControlRotation` only at :644. Both lanes flow through `GetHeroStartTransform`; the bot never routes through it (SpawnBot spawns a controller, no hero pawn).
- **Standalone-safety trace CONFIRMED against the code:** the only behavioral hunk sits at :845-847 inside `if (OwnCastle)` (branch 3), after branch 2's `return` at :765. Branch 2 (:742-783) unchanged — its warning literal still byte-matches 663 §1's quoted line; branch 4 (:857-861) unchanged (`ZeroRotator`). No-castle ⇒ branch 3 never runs ⇒ byte-identical, by construction. Current L_Arena standalone takes branch 3 and changes BY THE RULING'S DESIGN — declared, Jonathan's eye at 668.
- **The log tail (declared deviation 1):** prefix byte-stable through `(authored floor %.0f) -> (%.0f, %.0f, %.0f)`; tail `, facing yaw %.0f toward the own castle (TASK-665).` — 11 specifiers / 11 args, types legal, category/verbosity unchanged. 667's grep surface: Blue `facing yaw 180`, Red `facing yaw 0`.

## 4. TASK-665 — THE SWEEP TABLE, SPOT-CHECKED AT SOURCE

- **The 6 riders:** figures all recompute from 663 directly — h:304-311 & h:334-337 (X half 3,692.18 ⇒ derived 3,992, matches 663 §1's resolver log); `SiegeBotController.h:355-369` & `.cpp:1397-1403` (3,692.18 + 1,343.15 = **5,035.33** ⇒ X = 25,000 − 5,035.33 = **19,964.67**, 663 §6 measured 19,965; clamp re-checks hold: 19,964.67 ∈ [17,620, 32,380], 5,035.33 < 7,340); resolver-doc rewrite h:408-437 and RestoreHeroAtStart comment cpp:635-641 both record the pre-ROT wording. Old worked rows left as authored — record never rewritten.
- **Bot derive-not-hardcode CONFIRMED at source:** `ResolveCastleFaceDistance` (cpp:1204-1268) is a live colliding-AABB ray-exit (exact per-direction, not max(Ex,Ey)); rider-only was the correct action — nothing needed editing to move the anchor.
- **DEFEND invariance RECOMPUTED:** `SummonedUnit.cpp:2224` takes `FMath::Max(CastleBoxExtent.X, CastleBoxExtent.Y)` — X↔Y swap-invariant (max(3,656.79, 3,692.18) both before and after, 663 §1). Verify-only correct.
- **War-map zero-bake REPRODUCED:** no yaw/gate-direction bake in the WarMap lane (grep); commander −90 is the castle-LOCAL avatar offset (`CommanderNpcAnchor`, Castle.cpp:456) riding the rotated transform. Verify-only correct.
- **Full-tree post-condition REPRODUCED:** every "gates face world −Y / yaw 0" claim in `Source/` survives ONLY inside `Castle.{h,cpp}` — each inside a 637/623-era rider now carrying a 664 counter-rider. (The `SiegeStuckStatics` "facing world +X" hits are UNIT-fallback statements, rotation-independent — correctly untouched.) Zero residual `looks across the arena` / `facing the enemy half` phrases.

## 5. THE NEW TEST — `Tests/SiegeCastleTransformTest.cpp`

- **Pins what it claims:** Blue (−25000,0,0) yaw +90 / Red (+25000,0,0) yaw −90 (ROT-§1), pitch/roll 0, yaw via `FRotator::NormalizeAxis` delta (270 ≡ −90 passes, a real un-rotation fails); PLUS the constant-independent invariant — gate dir (sin θ, −cos θ) from the MEASURED yaw, `GateWorldX × TowardCenterline > 0.99` — recomputed: yaw +90 at X<0 ⇒ (+1)(+1); yaw 0 ⇒ 0 ⇒ FAILS. The tripwire fires even if the pinned constants are edited. Local (0,−1) under yaw θ → (sin θ, −cos θ) — rotation arithmetic verified.
- **Both load states handled honestly:** `LoadObject<UWorld>` returns the live editor world when open, the serialized package otherwise; reads ROOT-component `GetRelativeLocation/Rotation` (serialized UPROPERTYs, valid unregistered) with the relative==world precondition ASSERTED (`TestNull` on `GetAttachParent`), never `GetActorLocation`. Census: exactly 2 castles, one per team, null-entry-safe iteration.
- **Read-only / zero network / no PIE:** no save path, no mutation, no dirty-flag surface anywhere in the file. Compile posture: same macro + `EditorContext | EngineFilter` flags as all 127 existing tests; `#include "Siegebound/…"` matches suite style; `TeamId.h` exists; fixture namespace `SiegeCastleTransformTestFixture` file-unique (unity-safe); `TestEqual` double-with-tolerance and FString-What overloads all exist in UE5; namespace-scope constexpr needs no lambda capture.
- **Macro census CONFIRMED: exactly 128** occurrences across `Tests/` (127 pre-wave + this 1). The macro token appears once in the new file.

## 6. FINDINGS

- **[WARN] TASK-667 suite-count co-flight** — 128 is correct for THIS wave alone. The DECK wave (TASK-669..675) authors `Tests/SiegeDeckSlotsTest.cpp` in parallel; if it lands before 667's run the live census will exceed 128. 667 must reconcile **128 + deck-declared** explicitly, not fail the ROT wave on the delta (the 667 dated rider already anticipates the porcelain side; this extends it to the suite count).
- **[NIT] `handoffs/TASK-664-programmer.md`** — stale line citations for untouched surfaces (~23 lines off): summary format string cited :1005-1010, actual :982-987; torch line cited :857-861, actual :834-838; banner property doc cited h:619-627, actual h:648-656. Content anchors all resolve; grep surfaces are content-based; no code impact.
- **[NIT] `handoffs/TASK-665-programmer.md` sweep table** — no explicit row for the spec-named `HeroCharacter.cpp` ≈:105 site (team-gating comment). Verified here independently: the claim ("enemy castle's GateBlockerVolume stops him at the gate, his own ignores him") is castle-relative and direction-agnostic — still true post-rotation, correctly zero-diff. Completeness gap only; closed by this review.
- **[NIT] `SummonedUnit.cpp:2246`** — the worked figure "≈3,656.85 + 1,281 ≈ 4,937.9" stays imprecise (live prints 3,692.18 + 1,281 = 4,973.18). Correctly adjudicated as PRE-existing/pre-ROT (the max was already 3,692.18 before the swap) and out of a verify-only file's write scope; recorded in the 665 handoff for the file's next owner.

## 7. DEVIATION RULINGS (SC-§15)

| # | deviation | ruling |
|---|---|---|
| 664-1 | Riders #6/#7 beyond the five pinned sites | **ACCEPTED** — genuine world-frame falsehoods; ROT-§4 demanded them |
| 664-2 | Banner-block compass words untouched | **ACCEPTED** — ruling item 1's zero-line pin outranks a cosmetic rider; frame declared at the block headers |
| 664-3 | Consumerless F1 constants kept | **ACCEPTED** — reversal data per the ruling's machinery-stays clause; no MSVC warning class; flagged for 667 |
| 664-4 | Machinery docs/lane labels not ridden | **ACCEPTED** — they describe machinery (still true, empty or not), not the map |
| 665-1 | Branch-3 log tail grown | **ACCEPTED** — declared, prefix byte-stable, verified at source; buys log-only facing observability |
| 665-2 | New test FILE not an extension | **ACCEPTED** — genuine "none fits": `SiegeWarMapTest.cpp:59` charter verbatim makes a world-needing test there a FINDING |
| 665-3 | `SummonedUnit.cpp:2246` recorded-not-touched | **ACCEPTED** — pre-existing imprecision, not a rotation falsification (see NIT above) |

## 8. NOTES FOR BUILD-MASTER (TASK-667 PRE-FLIGHT)

**The reviewed set — EXACTLY these seven Source files (+ the two handoffs + this report). Any other Source line = UNREVIEWED, stop** (DECK-wave lines per the 667 dated rider: EXPLAINED + EXCLUDED from this commit, not refused):
1. `Source/GitClaudeUnrealTest/Siegebound/Castle.h` (664)
2. `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp` (664)
3. `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h` (665)
4. `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp` (665 — the ONE behavioral hunk lives here, branch 3)
5. `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h` (665, comment-only)
6. `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp` (665, comment-only)
7. `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCastleTransformTest.cpp` (665, NEW)

- **Suite expectation: 128** (`Siegebound.Castle.RotatedTransformPin` is the +1) — reconcile against the DECK co-flight per the WARN.
- Grep surfaces: `F1 discoverability set — 2/2 banners, 0/0 path segments, 0/0 toe rocks spawned` (both castles, ×2 SIE incl. Play-Again); branch-3 tail `facing yaw 180 toward the own castle (TASK-665)` (Blue; Red `facing yaw 0`); torch line byte-form intact.
- Live re-verify: pawn actor yaw (663 §9.1's instrument) — expect Blue 180 at (−21008, 0, ~98); 2 banner components + ZERO path/rock components per castle; `TASK-663-battery.py` byte-for-byte vs `TASK-663-battery-result.json`; nav sessions held ≥ 10 s.
- 664 flagged the kept constants as the one spot a toolchain could grumble — if any unused-const diagnostic appears, it is 664's diff to answer for, declared in advance.
