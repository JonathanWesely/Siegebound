# QA Report — TASK-623 (reviewed under TASK-624)
Verdict: PASS

Reviewer: qa-reviewer · Date: 2026-08-17 · Scope: pre-compile code review of the R-A1 diff (`BattlefieldScatter.cpp`) + the `Castle.h` comment rider. Blockers: 0 · Warns: 0 · Nits: 3.

**Instrument note:** this QA session had no shell/git tool, so `git diff` could not be run first-hand. Scope was verified by (1) line-for-line content match of both files against the full diff quoted in `handoffs/TASK-623-programmer.md` §3, and (2) filesystem mtime ordering (Glob) — the newest source files under `Source/` are exactly `BattlefieldScatter.cpp` and `Castle.h`; `BattlefieldScatter.h`, `ScatterConfig.h`, and every `Tests/*.cpp` sit deep in the older region. TASK-625 should still eyeball `git status` for source-file scope before compiling (its normal HEAD-verify covers this).

## Spec criteria (a)–(f)

| # | Criterion | Verdict | Evidence |
|---|---|---|---|
| (a) | Scope ~6 lines + rider, two files only | ✅ | 8 functional lines (2× `if`+braces+body) + comments in `BattlefieldScatter.cpp`; `Castle.h` comment-only. Mtime corroborates two-file scope. |
| (b) | Discs only — corridor NOT applied to non-blocking | ✅ | The corridor band test exists solely in `IsInKeepClear` (`.cpp:1214`); the non-blocking path calls `IsInKeepClearDiscs` (`:706`, `:799`), which contains no corridor branch (`:1221-1237`). The lane stays lush — the ruled outcome. |
| (c) | Blocking path byte-identical | ✅ | `:691` and `:791` are character-identical to the pre-change lines (handoff diff cross-checked). Only blocking-side text change: deletion of the now-falsified comment clause "grass (non-blocking) ignores keep-clear" at old `:690` — leaving it would itself have been a finding. |
| (d) | FootprintR inflation consistent with the `:1182` pattern | ✅ | Stronger than consistent — it is the SAME function, not a copy: `IsInKeepClearDiscs` inflates each disc by `sqrt(RadiusSq) + InstanceRadius` (`:1230-1231`), and `IsInKeepClear` itself delegates to it (`:1218`). Zero new geometry math introduced. |
| (e) | DA-serialized-4500 note present | ✅ | Primary-site comment `:700-703` states `CastleKeepClearRadius` 4,500 is DA-serialized (CR-R6 / W8-R3 correction) and a header re-derivation alone won't move the disc. |
| (f) | Rider comment-only, zero behaviour | ✅ | `Castle.h:193-197` — every changed line is inside the `/** ... */` doc block of `GetInteriorAnchorLocation`; the `UFUNCTION` at `:205-206` and all code bytes untouched. Yaw-180 claim corrected to measured yaw-0 / gates-face-−Y (617 C1); the transform-form rationale correctly preserved as future-proofing. |

## Determinism (checklist item 2)

✅ **Zero new FRandomStream draws.** Both new tests are pure rejections (`continue` / flag-clear). Call order audited at both sites: the per-attempt draw sequence is X (`:647`), Y (`:648`), mesh (`:660`), scale (`:666`); every rejection — field-edge `:683`, blocking gate `:691`, **new `:706`**, spacing `:717` — fires BEFORE the conditional Yaw draw (`:722`), matching the existing rejection shape. The twin block's law "ZERO FRandomStream DRAWS IN HERE" (`:772`) is preserved: the new twin re-test (`:799-802`) draws nothing, and a cleared `bPlaceTwin` skips traces (`GroundZAt`), never draws. Consequence: same seed ⇒ same layout intra-build; host==client holds; layouts differ from PRE-change builds per seed — explicitly allowed by the spec.

## The §6.1 adjudication (checklist item 3): ACCEPTED AS RULED — with a correction that makes it moot

The programmer declared that `IsInKeepClearDiscs` tests ALL `KeepClearZones` (2 castles + PlayerStart r=800), estimating ~20 grass + ~3 plants additionally cleared at the hero spawn. **Ruling: acceptable-as-ruled, no spec amendment required — and the declared side effect is in fact vacuous under the shipped level:**

1. **Containment.** The PlayerStart disc (center (−23800, 0), r=800) lies ENTIRELY inside the Blue castle disc (center (−25000, 0), r=4500): center distance 1200, and 1200 + 800 = 2000 ≤ 4500. Containment survives inflation (both discs inflate by the same FootprintR). Every point the PlayerStart disc rejects, the spec-mandated castle disc already rejects. The hero-spawn tufts are cleared by the CASTLE disc; the r=800 disc contributes nothing new today. `RebuildKeepClearZones` (`:1137-1206`) confirms the zone set: 2 castle discs (live sweep or ±25000 fallbacks) + PlayerStart discs, nothing else.
2. Therefore the spec's parenthetical "only the two castle pads lose decoration" remains **literally true** as shipped, and the CR ruling's load-bearing line — corridor NOT applied, lane stays lush — is honored (see (b)).
3. The spec itself names the vehicle (`IsInKeepClearDiscs`, the `:1182` pattern), which is all-discs by construction; a castle-only variant would need new zone-tagging plumbing, breaching the ~6-line contract for zero behavioral gain (per point 1).
4. The PlayerStart-disc membership only matters under a future level edit (a PlayerStart moved outside a castle disc) — the same defensive character as the existing twin guard, and directionally correct (flora cleared at a spawn pad is keep-clear intent).

## Twin-site accounting + provably-0 claims (spot-proofs)

- A new-test rejection routes into the existing `else { ++TwinSkipped; }` (`:863-871`) — the `else` binds to the placement `if (bPlaceTwin)` at `:838`, so the flag-clear at `:801` lands there correctly; the `twinSkipped=` summary token (`:881`) and the >0 warning (`:886-891`, "keep-clear or slope rejected the twin point") remain accurate with no new accounting. Matches the handoff exactly.
- §6.4's "TwinSkipped stays provably 0" verified: `DrawMaxX = 0` under Rotational180 (`:578`), so twin X = −X ≥ 0 ⇒ the Blue disc and its contained PlayerStart disc are ≥ ~20,500 uu away (unreachable at grass-scale inflation), and twin-vs-Red ≡ primary-vs-Blue by the exact rotational pairing — a primary that survived `:706` cannot have its twin rejected at `:799`. The counter's provably-0 contract survives the change.
- Placed-count risk: negligible. In the blue-half draw field (26,000 × 24,000) only the clipped Blue disc (≈4.1e7 uu², ≈6.5% of the field) rejects primaries — even lower than the handoff's conservative full-field 10.2% figure — so with 24 attempts/instance the all-rejected probability is astronomically small; no `placed < target` regression expected in 625's log greps.

## Standard checks

- **Deprecated/removed UE 5.8 APIs:** none — the new code introduces zero new API calls (only the existing member function `IsInKeepClearDiscs`).
- **Null/validity safety:** no pointers in the new code; `FootprintR` is computed (`:670-679`) before both uses.
- **Reflection/GC:** no UPROPERTY/UFUNCTION surface touched; no signature, default, or declaration changed (`BattlefieldScatter.h:333`/`:342` untouched — confirmed by grep; call sites: `:691`, `:706` new, `:791`, `:799` new, `:1218`, `:1547` mines pass unchanged, zero call sites outside `BattlefieldScatter.{h,cpp}`).
- **Perf:** generate-time only, no per-tick work (see NIT-2).
- **Conventions:** no new names introduced; comment tags follow the TASK-### annotation convention.
- **Tests (checklist item 5, independently grepped):** `Tests/` grep for `Scatter|KeepClear|bBlocking` — `SiegeWarMapTest.cpp` touches `USiegeScatterConfig` only to read `ArenaHalfExtent` (`:183`, placement-agnostic); `SiegeStuckStaticsTest.cpp` names BattlefieldScatter only in a doc comment (`:35`). **No test asserts in-disc non-blocking placement — the handoff's "expect none" claim is confirmed.** No test file modified ⇒ suite expectation stays **118**.

## Findings

- [NIT] `BattlefieldScatter.cpp:1230` — `FMath::Sqrt(Zone.RadiusSq)` recomputed per zone per call; grass now makes up to ~294k `IsInKeepClearDiscs` calls per generate (12,250 × 24 worst case). Generate-time only, sub-millisecond-scale in aggregate; hoisting an inflated-radius cache would touch shared code beyond this task's fence. No action required.
- [NIT] handoff §6.1 attribution — the "~20 grass + ~3 plants cleared by the r=800 disc" figure is subsumed by the Blue castle disc (containment proof above); the adjudication's conclusion stands, its premise was over-cautious. Recorded for the wave record only.
- [NIT] handoff §6.3 — the "≈10.2% of the draw field" figure uses the full arena; the actual blue-half draw field sees ≈6.5%. Direction of error is conservative; conclusion unchanged.

## Riders for the manager (NOT findings against TASK-623)

1. **`ScatterConfig.h:113-115`** — the `bBlocking` property doc still reads "the non-blocking GRASS layer ignores keep-clear (lush everywhere)", falsified by this change (and it names only GRASS, though Plants is also non-blocking). Out-of-fence here by the no-other-file rule; board a TASK-596-precedent comment rider on the next task owning `ScatterConfig.h`.
2. (Optional) The CR ruling / spec wording "only the two castle pads lose decoration" is literally true as shipped (containment), but a future PlayerStart relocation outside a castle disc would make the PlayerStart pad a third cleared patch — a one-line wording note in the ruling record would keep it honest forever.

## Notes for build-master (TASK-625)

- **Runtime behavior is UNVERIFIED at this gate** — this review is code-level only. The verdict is conditional on your compile + the SIE census: ≥3 seeds, expect **0** grass/plants inside both 4,500 discs (617 baseline ~170 z<10/footprint + ~104/17 apron), corridor flora PRESENT (lane lush), blocking margins non-negative, tree/proxy parity.
- Census note: the r=800 hero-spawn patch at (−23800, 0) and its rotational image at (+23800, 0) will also be flora-free — that is the castle discs at work (containment), not a defect.
- Expect `twinSkipped=0` / `zMismatch=0` on every layer line as before; a non-zero value on a non-blocking layer is now also a real signal.
- Headless suite: expect 118, unchanged.
