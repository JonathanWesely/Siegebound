# TASK-623 handoff — R-A1: castle-disc keep-clear for non-blocking layers + the Castle.h:194 comment rider (gameplay-programmer)

**Date:** 2026-08-17 · **Status:** ready-for-qa (TASK-624) · **Compile:** NONE in-task (G4 — TASK-625 owns the wave's one compile) · **Files touched: EXACTLY TWO** — `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp` + `Source/GitClaudeUnrealTest/Siegebound/Castle.h`.

⚠️ **Worktree scoping note for QA:** `git diff --stat` at write time also shows `Config/DefaultEngine.ini`, `Content/Blueprints/BP_Torch.uasset`, `TASKBOARD.md`, `CONVENTIONS.md` dirty — that is the PARALLEL lanes' work (TASK-620's D1–D3 dials; the manager's board/SIE-§1 writes), pre-existing before my first edit and never touched by me. Review only my two files.

---

## 1. The change (spec step 1) — two sites, blocking path byte-identical

**Mechanism recap (TASK-613 §2):** non-blocking layers (Grass 12,250 + Plants 2,000) skipped keep-clear entirely — the gate `if (Layer.bBlocking && IsInKeepClear(...))` never consulted any zone for them, producing the measured ~170 z<10 grass per castle footprint + ~104 grass / ~17 plants per apron (TASK-617 A7), every seed.

**The fix, exactly R-A1 as sketched (613 §7):** when `!Layer.bBlocking`, additionally reject candidates via **`IsInKeepClearDiscs(Point, FootprintR)`** — the TASK-255 disc-only entry point (NO corridor band; it is the same function the mines pass already uses at the now-`:1551` site). Each disc is inflated by the candidate's FootprintR inside `IsInKeepClearDiscs` itself (`InflatedRadius = sqrt(RadiusSq) + InstanceRadius`) — the identical inflation the blocking path gets, satisfying QA criterion (d) by reuse rather than reimplementation.

- **Primary site** (was `:691`): new test at **`:706-709`** — `if (!Layer.bBlocking && IsInKeepClearDiscs(Candidate, FootprintR)) { continue; }` immediately after the untouched blocking gate (still `:691-694`, byte-identical). The one comment amendment above it: the clause "grass (non-blocking) ignores keep-clear" (old `:690`) was deleted — this change falsifies it.
- **Twin site** (was `:776`, now `:791`): the existing `bool bPlaceTwin = !(Layer.bBlocking && IsInKeepClear(TwinPoint, FootprintR));` line is **byte-identical**; the new guarded re-test at **`:799-802`** sets `bPlaceTwin = false` when `!Layer.bBlocking && IsInKeepClearDiscs(TwinPoint, FootprintR)`. A rejection routes into the EXISTING `TwinSkipped` asymmetry-escape counter + its warning log — no new accounting.
- **Corridor:** NOT applied to non-blocking layers at either site (spec's ⛔). The lane stays lush.
- **DA-4500 note (QA criterion (e)):** present in the primary-site comment — `CastleKeepClearRadius` 4,500 is DA-SERIALIZED in `DA_BattlefieldScatter` (CR-R6's W8-R3 correction), so a header re-derivation alone will not move the disc.

**Functional line count:** 8 functional lines (2× `if` + braces + body) + comments — the "~6 lines" of the sketch, with the twin site costing the extra brace pair because the sketch's single-expression form would have rewritten the blocking line (byte-identity chosen over line count).

## 2. The rider (spec step 2) — Castle.h, comment-only

`Castle.h` `GetInteriorAnchorLocation` doc (was `:193-194`, now `:193-197`): the stale "Castle_Red is placed at yaw 180" claim corrected to the measured truth — **both castle actors sit at yaw 0, both gates face world −Y** (TASK-617 C1). The still-valid rationale (actor TRANSFORM, never ActorLocation + offset, so a future yawed castle stays correct) is preserved and now stated as the future-proofing it actually is. **Zero behaviour diff** — no code line in the file changed; the shipped `ZeroVector` default makes the function's return unchanged by construction anyway.

## 3. The full diff

```diff
--- a/Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp
+++ b/Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp
@@ -687,12 +687,27 @@ void ASiegeBattlefieldScatter::ScatterLayer(const FScatterLayer& Layer, FRandomS
 
 			// Blocking obstacles honor keep-clear + the reserved corridor, now inflated
 			// by the footprint radius so a wide instance centered off-lane no longer
-			// sprawls across the corridor; grass (non-blocking) ignores keep-clear.
+			// sprawls across the corridor.
 			if (Layer.bBlocking && IsInKeepClear(Candidate, FootprintR))
 			{
 				continue;
 			}
 
+			// R-A1 (TASK-623): NON-blocking layers (Grass/Plants) now honor the
+			// keep-clear DISCS ONLY, inflated by FootprintR through the SAME
+			// IsInKeepClearDiscs inflation the blocking path uses. ⛔ The corridor
+			// band is deliberately NOT applied here — the Blue→Red lane stays lush;
+			// only the keep-clear pads lose decoration. Radius authority: the live
+			// CastleKeepClearRadius 4,500 is DA-SERIALIZED in DA_BattlefieldScatter
+			// (CR-R6's W8-R3 correction), not merely the ScatterConfig.h C++ default —
+			// a future header re-derivation alone will NOT move this disc. Kills the
+			// measured in-footprint flora at the root (TASK-617 A7: ~170 z<10 grass
+			// per castle footprint + ~104 grass / ~17 plants per apron, every seed).
+			if (!Layer.bBlocking && IsInKeepClearDiscs(Candidate, FootprintR))
+			{
+				continue;
+			}
+
 			// Radius-aware MinSpacing (TASK-284 grid-hash accelerated): reject if any
@@ -774,6 +789,17 @@
 				// nothing and a future level edit (a moved castle, a second
 				// PlayerStart) would otherwise silently place into a keep-clear zone.
 				bool bPlaceTwin = !(Layer.bBlocking && IsInKeepClear(TwinPoint, FootprintR));
+				// R-A1 (TASK-623): the non-blocking DISC re-test, mirroring the primary
+				// site (discs only — the corridor is never applied to non-blocking
+				// layers). Same defensive character as the blocking guard above: the
+				// castle discs are an exact rotational pair and the PlayerStart disc is
+				// unreachable by a twin (X >= 0), so under the shipped level this can
+				// never reject — it guards the same future level edits, for free. A
+				// rejection lands in the existing TwinSkipped asymmetry-escape counter.
+				if (bPlaceTwin && !Layer.bBlocking && IsInKeepClearDiscs(TwinPoint, FootprintR))
+				{
+					bPlaceTwin = false;
+				}
 				float TwinGroundZ = 0.f;
--- a/Source/GitClaudeUnrealTest/Siegebound/Castle.h
+++ b/Source/GitClaudeUnrealTest/Siegebound/Castle.h
@@ -190,8 +190,11 @@ public:
 	 *  = the ACTOR TRANSFORM applied to InteriorAnchorRelativeLocation, never
-	 *  ActorLocation + offset: Castle_Red is placed at yaw 180, so a non-zero
-	 *  relative anchor must rotate with the castle or it lands outside the wrong wall.
+	 *  ActorLocation + offset. (TASK-623 comment rider, CR-R6: the claim here that
+	 *  "Castle_Red is placed at yaw 180" was STALE — BOTH castle actors sit at yaw
+	 *  0 and both gates face world −Y, measured live at TASK-617 C1. The transform
+	 *  form is kept regardless: a non-zero relative anchor must rotate with the
+	 *  castle if a level edit ever yaws one, or it lands outside the wrong wall.)
 	 *  At the shipped ZeroVector default the two are identical BY CONSTRUCTION, and
 	 *  this returns the actor's own location.
```

## 4. Call-site audit (the trailing-defaulted-parameter law: NOT TRIGGERED — no signature touched; enumerated anyway)

| Function | Declaration | Call sites AFTER this diff |
|---|---|---|
| `IsInKeepClear(Point2D, InstanceRadius = 0.f)` | `BattlefieldScatter.h:333` (defaulted trailing param, UNTOUCHED) | `.cpp:691` primary blocking gate (unchanged) · `.cpp:791` twin blocking gate (unchanged). Both pass the radius explicitly; the default is exercised by no caller — unchanged fact. |
| `IsInKeepClearDiscs(Point2D, InstanceRadius)` | `BattlefieldScatter.h:342` (no default, UNTOUCHED) | `.cpp:1207` (from `IsInKeepClear`, unchanged) · `.cpp:1551` mines pass, primary + twin (unchanged; was `:1521` pre-diff, shifted +30 by insertions only) · **NEW `.cpp:706`** (non-blocking primary) · **NEW `.cpp:799`** (non-blocking twin). |

No signature, parameter, default, or declaration changed anywhere. Grep confirms zero call sites outside `BattlefieldScatter.{h,cpp}`.

## 5. Tests (spec step 3) — statement

**No existing test asserts in-disc non-blocking placement — "expect none" CONFIRMED.** Evidence: grep of `Source/GitClaudeUnrealTest/Siegebound/Tests/` for `Scatter|KeepClear|bBlocking` — `SiegeWarMapTest.cpp` touches `USiegeScatterConfig` ONLY to read `ArenaHalfExtent` (`:183`, `:565-585` — the single-source-of-truth pattern, placement-agnostic); `SiegeStuckStaticsTest.cpp` names BattlefieldScatter only inside a doc comment (`:35`). **Therefore no test amendment was needed and none was made; no new test was added** — the placement path requires a live UWorld + HISM components + ground traces, which the headless suite does not stand up, and the spec's runtime verification lane is TASK-625's SIE census (≥3 seeds, expect 0 grass/plants inside both 4,500 discs, corridor flora present, blocking margins non-negative). Suite expectation stays **118**.

## 6. Declared adjudications — QA, scrutinize these

1. **The disc set includes the PlayerStart disc (r=800), not the castle discs alone.** The board spec's headline says "CASTLE keep-clear DISCS"; the prescribed vehicle (R-A1 as sketched, 613 §7 — `IsInKeepClearDiscs`, the `:1182`-pattern function the spec names) tests ALL discs in `KeepClearZones` = 2 castles + PlayerStart. A castle-only variant would need a tagged zone array or a second list — new plumbing well beyond the ~6-line contract, and the corridor-vs-disc distinction is the ruling's actual load-bearing line. Quantified side effect: expected ~20 grass + ~3 plants cleared from the r=800 hero-spawn disc at (−23800, 0) (area ≈ 2.01e6 uu² × the 4d densities) and, via the pair-generation asymmetry already documented at `:639-646`, from its harmless rotational image patch at (+23800, 0). Both are keep-clear pads by the ruling's own language ("only the keep-clear pads lose decoration"); flora at the hero's spawn feet being cleared is if anything the intent. If QA reads the spec stricter than the sketch, say so — castle-only is a different (larger) diff, not a tweak of this one.
2. **Blocking-layer path byte-identical** (QA criterion (c)): both blocking gate lines are character-identical; the only text change on the blocking side is the deletion of the now-false comment clause "grass (non-blocking) ignores keep-clear" — leaving a falsified comment standing would itself be a QA finding.
3. **Determinism + counts:** the new tests draw NOTHING from the FRandomStream (pure rejection via `continue`/flag), so intra-build determinism and host==client agreement hold; layouts change per seed — explicitly allowed by the spec. Count impact: the two castle discs remove ≈10.2% of the draw field for non-blocking candidates; with `MaxPlacementAttemptsPerInstance = 24` and grass MinSpacing 50, the per-instance all-attempts-rejected probability is ~1e-24 — placed counts still hit target (no `placed < target` regression for 625's log greps).
4. **`TwinSkipped` stays provably 0 under the shipped level** with the new twin test: the castle discs are an exact rotational pair (twin-vs-Red ≡ primary-vs-Blue, and a twin with X ≥ 0 is ≥ 25,000 uu from the Blue disc), and the PlayerStart disc is unreachable by a twin (X ≥ 0, per the existing `:789-796` proof). The existing "provably 0" claims on the counters remain true.
5. **Out-of-fence stale doc, NOT edited (manager, please board a rider):** `ScatterConfig.h:113-115` — the `bBlocking` property doc still reads "the non-blocking GRASS layer ignores keep-clear (lush everywhere)". Falsified by this change; the no-other-file fence forbade touching it here. A TASK-596-precedent comment rider on any future task owning `ScatterConfig.h` closes it. (Header doc lines `BattlefieldScatter.h:31-48/188/327-345` were checked: none contradicts the new behaviour.)

## 7. Fences honored

No compile · no editor/MCP session · no git write (one read-only `git diff` for this handoff) · no file beyond the two named · TASKBOARD.md not edited (orchestrator flips status per dispatch) · no console / no `M` / no DumpAssistantPrompt · L_Arena untouched (CF-R3/R4 · G4 · QUIET-MODULE).
