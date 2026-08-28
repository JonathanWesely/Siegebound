# TASK-665 — [ROT-4] THE WORLD-FRAME SWEEP outside `Castle.{h,cpp}` + THE SPAWN-FACING FIX (LIVE) + THE TRANSFORM-PINNING TEST (gameplay-programmer handoff)

Date: 2026-08-27. Inputs recomputed from **`handoffs/TASK-663-buildmaster.md`** (the RELAYED-DIAGNOSIS law — every figure below is re-derived from that record, cited by section), the ROT ACTIVATION RULING items 5–6, CONVENTIONS ROT-§1/§4.
Fences honoured: ⛔ `Castle.h`/`Castle.cpp` byte-untouched (TASK-664's — verified: zero edits in those files) · ⛔ zero compile (TASK-667 owns, QUIET-MODULE) · ⛔ no editor/MCP/git/board writes · zero network in the new test (asserted in-file) · trailing-default law (no signature changes anywhere) · compile-trap laws (double-only Atan2 chain; no mixed float/double template deduction).

---

## 1. THE SPAWN-FACING FIX (ruling item 5 — LIVE; 663 §1 measured delta 180°)

**The one authoring site:** `ASiegeGameMode::GetHeroStartTransform` **branch 3** — `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp` (the `OutRotation` line inside the `if (OwnCastle)` block, formerly `(TowardCenterline > 0) ? 0 : 180` = facing the ENEMY half).

**The fix:** yaw is now **derived at runtime from the resolved castle transform** — `atan2(CastleLocation.Y − OutLocation.Y, CastleLocation.X − OutLocation.X)` (663 §1's exact derivation), pitch/roll 0. ⛔ No hardcoded yaw anywhere: under the current layout this **evaluates** to yaw 180 (Blue) / 0 (Red) — the old facing negated, exactly the ruling's numbers — and a moved castle carries the facing with it the same way it already carries the location. Degenerate-input proof in-code: `SpawnDistance >= max(authored floor 1,500, derived)` > 0, so the atan2 arguments are never both ~0. Whole derivation in double (LWC/compile-trap law).

**The consuming lanes (found by grep, both covered by fixing the one resolver — no second authoring site exists):**
1. **Initial spawn / fresh-pawn respawn:** `RestartPlayer` → `GetHeroStartTransform` → `RestartPlayerAtTransform(FTransform(StartRotation, StartLocation))` → engine `FinishRestartPlayer` applies rotation to pawn + control rotation.
2. **Normal respawn:** `RestoreHeroAtStart` → same resolver → `SetActorLocationAndRotation(StartLocation, StartRotation, …)` + `PC->SetControlRotation(StartRotation)`.
`StartRotation` appears at NO other site in `Source/` (grep record: SiegeGameMode.cpp only).

**The log surface changed (declared for QA + 667):** the branch-3 Log line keeps its byte-identical prefix (`Castle-relative hero start for %s: castle X …`) and gains a tail: `…, facing yaw %.0f toward the own castle (TASK-665).` Prefix greps (663's) still match; full-line byte-comparisons against 663's quoted line will differ by exactly this declared tail. Expected live values (recomputed from 663 §1): Blue `-> (-21008, 0, 100), facing yaw 180`; Red mirror `facing yaw 0` (and `facing yaw -0` is impossible: Atan2(0, −3992) = +π exactly ⇒ +180; Atan2(0, +3992) = 0).

**STANDALONE-SAFETY PROOF (paths traced):** the fix lives INSIDE `if (OwnCastle)` (branch 3) and after the branch-2 `return`.
- Branch 2 accepted (PlayerStart on own side, outside castle — every no-castle or pre-9× test map): returns the PlayerStart's authored yaw **before** the fix line is reached — byte-identical.
- No castle in the level: `OwnCastle == nullptr` ⇒ the branch-2 side/inside tests degrade to accept (unchanged code) and branch 3's `if` is false ⇒ branch 4 arena origin, `ZeroRotator` — byte-identical. **"If no castle resolves, branch 3 never ran" holds by construction: branch 3 IS the `if (OwnCastle)` body.**
- Current L_Arena standalone DOES take branch 3 (the TASK-573 refusal fires, 663 §1) — so standalone behavior changes there **by the ruling's explicit design** (declared behavior change; Jonathan's acceptance is TASK-668), not by accident.
- The bot never routes through this: `SpawnBot` spawns a controller only, never a hero pawn, never calls RestartPlayer.

## 2. THE WORLD-FRAME SWEEP TABLE (everything outside `Castle.{h,cpp}`)

| # | Site | Verdict | Action |
|---|------|---------|--------|
| 1 | `SiegeGameMode.cpp` branch-3 `OutRotation` | **REAL FIX** (ruling item 5) | the §1 fix + in-code dated comment |
| 2 | `SiegeGameMode.cpp` `RestoreHeroAtStart` comment "PlayerStart yaw 0 looks across the arena" | world-frame falsehood (the live respawn facing is branch 3's) | ROT-§4 dated rewrite, old wording recorded in-line |
| 3 | `SiegeGameMode.cpp` branch-3 header "facing the enemy half" | falsified by the fix itself | dated rewrite (part of the fix) |
| 4 | `SiegeGameMode.h` resolver doc "(3) … facing the enemy half" | same | dated rewrite recording the pre-ROT wording |
| 5 | `SiegeGameMode.h` `HeroSpawnCastleOffset` doc (half-extent 3,656.85 ⇒ ≈3,957) | **self-corrected** — resolver reads live bounds; 663 §1 measured half 3,692.18 ⇒ 3,992 | ROT-§4 dated rider; pre-ROT figures left as authored |
| 6 | `SiegeGameMode.h` `HeroSpawnCastleClearance` doc (same figures) | self-corrected | ROT-§4 dated rider |
| 7 | `SiegeBotController.h` `BotCastleSpawnOffset` block (worked rows `9× … = 5,000.00`, "X ≈ 20,000", VERIFIED-INERT clamp note) | **self-corrected** — `ResolveCastleFaceDistance` measures live colliding bounds; 663 §6: 3,692.18 + 1,343.15 = 5,035.33 ⇒ X 19,964.67, measured live `castle-front (19965, 418, 20)` | ROT-§4 dated rider: new figures, clamp claims re-checked (19,964.67 ∈ [17,620, 32,380]; 5,035.33 < 7,340 — both hold), + the truth-change recorded: the fronted face is now Red's **GATE** face (serves M7.6 ruling #1 directly) |
| 8 | `SiegeBotController.h` `SwarmSpawnRadius` note "~20,000 from the centerline" | still TRUE as the approximation it states (19,965 ≈ ~20,000; "300 nowhere near" holds; stated as a relationship precisely so it cannot rot) | none |
| 9 | `SiegeBotController.cpp` `ComputeValidBotSpawnPoint` pass-through note ("~5,000 … X ~ 20,000") | self-corrected (same derivation) | ROT-§4 dated rider, both containment claims re-checked |
| 10 | `SiegeBotController.cpp` `ResolveCastleFaceDistance` / `GetCastleRedActor` | pure live measurement, no world-frame statement | none |
| 11 | `SummonedUnit.{h,cpp}` DEFEND band (`ResolveDefendEngagementRadius`) | **rotation-INVARIANT**: takes `max(ExtentX, ExtentY)` — the ROT X↔Y swap leaves the max unchanged (max(3,656.79, 3,692.18) both before and after, 663 §1 AABBs) | none (verify-only, as the spec expected) |
| 12 | `SummonedUnit.h:752` "every wall/gate face" 45° note | castle-local / direction-agnostic (ROT-§4: true statements get no rider) | none |
| 13 | `BattlefieldScatter.cpp` `ResolveCastleLocation` (±25,000 fallback) + `ResolveCastleQueryInset` (colliding extent.X toward centerline) | castle centres unmoved (fallback still true); inset derives live ⇒ self-corrected (now reads 3,692.18) | none |
| 14 | `BattlefieldScatter` reserved corridor (\|Y\| ≤ 1,000, whole X span) | no defect — the rotated gate mouths sit ON the corridor axis (mouths at y 0, x ∓21,300; 663 §2 walked spawn→mouth clean), so the corridor now runs gate-to-gate; the guarantee got stronger, not stale | none (read-only; the parked scatter:1313 rider untouched, stays parked) |
| 15 | `BattlefieldScatter` / `ScatterConfig.h` PlayerStart-disc references (−23,800) | the PlayerStart ACTOR is unmoved by the rotation — statements stay true; the live branch-3 spawn (−21,008, 0, 663 §1) is covered by the Blue castle keep-clear disc + the corridor (663 §2: pre-mouth stations EMPTY ×3) | none |
| 16 | `HeroCharacter.cpp:428-433` castle footprint dims | figures self-declared "HISTORY, not a dependency"; mechanism measures live collision (`ActorGetDistanceToCollision`) | none |
| 17 | War-map / place-symbol lane (`WarMapWidget`, `FSiegeWarMapProjection`, `CommanderNpc`) | pure world-XY→UV projection, **no gate-direction bake anywhere**; castle centres unmoved; dots/symbols are live world reads; commander's −90 is a MESH-local avatar offset riding the castle's rotated anchor | none |
| 18 | `Barracks` front point "toward the enemy half" | building-relative centerline law — independent of castle yaw | none |
| 19 | `SiegeSpawnConstants.h` | body-scale constants | none |
| 20 | "gates face world −Y … 617 C1" statements | grep-verified to exist ONLY inside `Castle.{h,cpp}` — TASK-664's riders, not mine | fence honoured |

## 3. THE TRANSFORM-PINNING TEST

**New file (declared "none fits" case):** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCastleTransformTest.cpp` — every existing Tests/ file is subject-scoped to pure statics/CDO reads, and `SiegeWarMapTest.cpp`'s charter states in bold that a test needing a world there is a FINDING. Same frame as the suite (same macro, `EditorContext | EngineFilter`, name `Siegebound.Castle.RotatedTransformPin`); the macro token appears exactly once in the file so grep censuses count one test.

**Mechanism (read-only, zero writes, zero network, no PIE):** `LoadObject<UWorld>` on `/Game/Maps/L_Arena.L_Arena` (live editor world when open, serialized package otherwise), enumerate `PersistentLevel->Actors`, census exactly 2 `ACastle` (one per serialized `Team`), then per castle read the ROOT component's `GetRelativeLocation()/GetRelativeRotation()` — serialized UPROPERTYs, valid registered or not, and world-equal for an unattached root (the no-attach-parent precondition is ASSERTED, not assumed). Pins: Blue (−25000, 0, 0) yaw **+90**, Red (+25000, 0, 0) yaw **−90** (ROT-§1), pitch/roll 0, yaw compared through `NormalizeAxis` (so 270 ≡ −90 passes, real un-rotation fails). Plus the constant-independent invariant: gate dir (sin θ, −cos θ) from the MEASURED yaw must point toward the centerline (dot > 0.99) — a sign-error tripwire that fires even if someone edits the pinned constants. Measured poses AddInfo'd into the automation log for 667's record.

**⭐ SUITE EXPECTATION FOR TASK-667: `127 → 128`** (one new test; macro census across `Tests/` verified at exactly 128 post-change).

## 4. M8 DECLARATIONS (per touched file)

- **`SiegeGameMode.{h,cpp}`:** the spawn rotation is **server-authored state** — `ASiegeGameMode` exists only on the server, and the changed VALUE travels over the spawn lane's own existing transports (initial: `RestartPlayerAtTransform` → engine `FinishRestartPlayer`/`ClientSetRotation`; respawn: replicated teleport + `SetControlRotation`). Zero new replication surface, zero new RPCs, no net-serialization change — replication posture inherited verbatim from the lane (TASK-356 doc §3.4.4 model).
- **`SiegeBotController.{h,cpp}`:** comment-only diffs; zero behavior, zero replication delta (server-side controller regardless).
- **`Tests/SiegeCastleTransformTest.cpp`:** dev-automation only (`WITH_DEV_AUTOMATION_TESTS`), editor context, zero network, never ships in a game target.

## 5. FILES TOUCHED (complete)

1. `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp` — the facing fix + branch-3 header + log tail + RestoreHeroAtStart comment
2. `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h` — resolver doc rewrite + 2 ROT-§4 riders
3. `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h` — 1 ROT-§4 rider (BotCastleSpawnOffset block)
4. `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp` — 1 ROT-§4 rider (pass-through note)
5. `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCastleTransformTest.cpp` — NEW (the pin test)
6. `.claude/pipeline/handoffs/TASK-665-programmer.md` — this record

⛔ NOT touched, per fence: `Castle.h`, `Castle.cpp`, TASKBOARD, CONVENTIONS, any Content/, any editor state. No behavioral edit outside item 1 of the sweep table.

## 6. DEVIATIONS DECLARED (SC-§15)

1. **The branch-3 log line grew a declared tail** (§1) — prefix byte-stable; 667's before/after grep must expect the new form. Chosen so the applied facing is readable straight out of a PIE log with no editor probe (the same observability standard 663 graded on).
2. **New test FILE rather than an extension** — the declared "none fits" case (§3 justification; the WarMap charter forbids worlds there).
3. **`SummonedUnit.cpp:2246` left as authored although its worked figure ("≈3,656.85 + 1,281 ≈ 4,937.9") quotes the X-half where the code takes `max(X, Y)`** — a PRE-existing, pre-ROT imprecision (the max was already 3,692.18 before the rotation, 663 §1's swap record), therefore NOT a rotation falsification and out of this task's write scope on a verify-only file. Recorded here for whoever owns that file next: the live log now prints 3,692.18 + 1,281 = 4,973.18.

## 7. QA SCRUTINY LIST (TASK-666)

1. The atan2 fix: sign convention (UE yaw = atan2(Y, X) in degrees), double-only chain, FRotator(0.0, yaw, 0.0) ordering (pitch, yaw, roll).
2. The standalone-safety trace in §1 — verify branch 2/4 are genuinely untouched in the diff (the only .cpp behavioral hunk is inside `if (OwnCastle)` after the branch-2 return).
3. The new test's two load states (open map vs serialized package) and the relative-vs-world reasoning + asserted precondition; `TestNull`/`TestEqual` overload usage; the file compiles under the unity build (fixture namespace is file-unique).
4. The log-format args (`OutRotation.Yaw` double through `%.0f` varargs — legal promotion) and that the line count/category (`LogGitClaudeUnrealTest`, Log) is unchanged.
5. Riders: confirm every figure against `handoffs/TASK-663-buildmaster.md` §1/§6 directly (RELAYED-DIAGNOSIS law), not against this handoff.
6. Fence check: `git diff --stat` shows zero delta in `Castle.{h,cpp}`.

**For TASK-667:** suite expectation **128/128**; the branch-3 grep form of §1; the facing re-verify is the pawn's actor yaw exactly as 663 §9.1 did it (expect Blue 180 at (−21008, 0, ~98)); `TASK-663-battery.py` re-run is unaffected by this diff (no probe surface touched).
