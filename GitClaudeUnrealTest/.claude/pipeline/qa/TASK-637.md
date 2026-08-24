# QA Report — TASK-637 (reviewed under TASK-638)
Verdict: **PASS**

Reviewer: qa-reviewer · 2026-08-23 · Inputs: `handoffs/TASK-637-programmer.md`, `handoffs/TASK-629-artist.md` (as-built authority), `handoffs/TASK-617-buildmaster.md` (C1 yaw record), `qa/TASK-634.md` (the homed-site list + committed-baseline landmark table), `Castle.h:198-203` (the TASK-623 rider model), and the three source files read directly.

⚠️ **Gate scope: CODE-LEVEL, COMMENT-ONLY DIFF. No compile has run** (QUIET-MODULE — the diff rides TASK-649's slot). This PASS is not a build result.

## Findings

- [WARN] all three files — **instrument limit on criterion (a):** QA holds no git tool (by design; the TASK-621 lesson), so "comment-only at the byte level" was verified by the strongest available proxy, not a literal `git diff`: (1) landmark line-shift accounting over `Castle.h`/`Castle.cpp` against qa/TASK-634's committed-baseline positions — every shift is EXACTLY accounted for by the declared comment insertions (cpp: :426/:366/:99/:279 unshifted → +4 after b3 (`:763→:767`, `:930→:934`, `:1051→:1055`, `:1062→:1066`, `:1128→:1132`) → +9 after b4; h: :194→:200, :512→:523, :526→:537, :538→:549, :697→:708 = +6/+11 split exactly as declared), no unexplained shift anywhere; (2) every inserted region read in full — pure comment text; (3) the three named adjacent code lines byte-intact (below). — Residual: the literal byte-level diff confirmation is TASK-649's pre-flight (note 1 below). Not a blocker: no path found by which a non-comment byte could hide from the accounting.
- [NIT] `handoffs/TASK-637-programmer.md` §2 (b4) — site position given as "now :1133-1146"; the rider comment actually spans `Castle.cpp:1134-1149` inside the function at :1132-1151. Content located and verified regardless; handoff line-bookkeeping imprecision only. No action.

Blockers: **0** · Warns: **1** · Nits: **1**

## Independent verification (orchestrator criteria a–d, all re-computed — not trusted)

**(a) Comment-only, three files, adjacent code byte-intact.**
- `Torch.h:198` — `	float TorchAttenuationRadius = 1200.0f;` byte-exact; UPROPERTY line :197 untouched; the doc block :179-196 well-formed, closes once.
- `Castle.cpp:610` — `	const FTransform CastleMeshTransform = CastleMesh->GetComponentTransform();` byte-exact, directly under the b3 rider :601-609 (all `//`).
- `Castle.cpp:1150` — `	return GetActorTransform().TransformPosition(InteriorAnchorRelativeLocation);` byte-exact; the b4 block is all `//`.
- `Castle.h` b1 (:69-79) and b2 (:472-481) are ` * ` continuation lines inside pre-existing `/** */` blocks; `FVector GateBlockerExtent = FVector(900.f, 405.f, 678.f);` (:470) and every UPROPERTY line in the read ranges untouched.
- ⚠️ The working tree's `SiegeAccountSaveGame.h` / `SiegeAccountSubsystem.h` diffs are the **parallel ACCOUNTS client lane**, declared in 637's dispatch and handoff §1 — NOT part of this review, NOT covered by this PASS.

**(b) Five sites vs the homed list (GH-R13 + qa/TASK-634 notes item 4) — all five, none pre-corrected.**
| homed site | now | verdict |
|---|---|---|
| `Torch.h:180-189` hall dims | :182-195 | ✓ as-built 2910 × 1140 / 1986 clear / floor 174 / ceiling 2160 — **every number = 629 §2 verbatim** (x −1920…990 = 2910, y 240…1380 = 1140, clear measured 1986.0–2112.5 at flat ceiling 2160); WR-§1 1560 correctly restated as design MINIMUM (629: 1986 ≥ 1560); old "≈2910 × 720 / ≈1560 clear" QUOTED as the stale pre-redesign carve, not erased |
| `Castle.h:70` class-doc reason (2) | :69-79 | ✓ "any future re-pose" + rider quoting the stale Castle_Red-yaw-180 claim, cites 617 C1 |
| `Castle.h:470` InteriorAnchorRelativeLocation doc | :475-481 | ✓ "any castle pose" + rider, follows the TASK-623 model at :198-203 by name |
| `Castle.cpp:≈602` mesh-transform composition | :601-609 | ✓ "any castle pose comes along for free" + rider; real-reason framing correct (composition valid at ANY pose) |
| `Castle.cpp:1131-1137` GetInteriorAnchorLocation body | :1134-1149 | ✓ rotation rationale re-grounded on the hypothetical yaw; placement restated as MEASURED "Castle_Blue (−25000,0,0) and Castle_Red (+25000,0,0), BOTH yaw 0 — TASK-617 C1"; "TASK-218's yaw-180 plan is history, not the map" |
- **Yaw truth cross-checked at the C1 artifact** (`handoffs/TASK-617-buildmaster.md:58`): "Castle actor yaws: Castle_0 = 0, Castle_1 = 0", both gates world −Y. Corroborating arithmetic: C1's Red commander at x **24535** = 25000 + (−465) — only possible at yaw 0 (yaw 180 would give 25465). The comment's resolved points match.
- "Already-corrected declarations: NONE" verified: the only pre-existing corrected copy is the TASK-623 model itself (:198-203); the four other yaw sites all carry fresh TASK-637 riders.

**(c) Post-condition greps, re-run independently over ALL of `Source/`** (bare-literal patterns — the multi-byte `×` trap heeded).
- `yaw 180` (case-insensitive): **14 hits, zero normative** — 7 scatter TWIN-law sites (`BattlefieldScatter.cpp:1613/1630/1901`, `.h:134/398/419`, `ScatterConfig.h:408` — a different, still-true prop-mirroring mechanic) · 2 facing-semantics sites (`Castle.cpp:325` hypothetical 7th anchor, `Castle.h:523` yaw-convention table) · 5 STALE-marked rider quotes (`Castle.h:73/:200/:478`, `Castle.cpp:606/:1140`) — historical citations, excluded by the post-condition's own terms. Classification matches the handoff's exactly.
- `Castle_Red` within 60 chars of `180`: 3 hits, all inside STALE-marked rider quotes (the b1 quote splits across `Castle.h:72-73` and is caught by the yaw-180 sweep instead — coverage complete).
- Bare `720` / `1560` / `2910` / `clear height` / `1986`: every survivor classified — `Castle.cpp:62-63` `//~` BIRTH record (qa/634 nit (b) ruling: historical, standing) · `Castle.cpp:99` `HallClearHeightZ = 1560.f` CODE VALUE with its 634-authored design-minimum doc (:93-97) — touching it would be a behaviour byte · `Castle.cpp:127-128/:279` as-built text quoting old values AS history · `Castle.h:439/452/466` the gate's measured 1560-uu COLLISION-gap (a different 1560) · `Castle.h:422/431` gate clear heights (different quantity) · `SiegeAssistantSnapshot.{h:429,cpp:951}` line budgets, `SiegeNavAreas.h:131` engine line numbers — unrelated · `Torch.h:183/185/189` the new as-built text + rider quote. **Zero normative stale-dims claims remain.**

**(d) Compile-trap + hygiene.** No literal `*/` inside any new doc-comment text (all `.h` insertions are ` * ` continuations; both `.cpp` riders are `//`; every touched block closes exactly once). No new includes, symbols, or code paths in any changed region. QUIET-MODULE: no compile run, and a pure-comment diff can produce no artifact — accepted at declaration level per the qa/634 precedent; the wave's compile is TASK-649's.

## Notes for build-master (TASK-649 pre-flight)
1. **TASK-637's diff lives in EXACTLY three files:** `Source/GitClaudeUnrealTest/Siegebound/Torch.h` (+6), `Castle.h` (+11), `Castle.cpp` (+9) — all comment lines. Pre-flight `git diff -- Source/` and confirm: (i) those three files' ± lines all carry `//` or ` * ` prefixes (the byte-level confirmation this review's WARN delegates); (ii) the ONLY other `Source/` files in the tree are `SiegeAccountSaveGame.h` (+36) and `SiegeAccountSubsystem.h` (+6) — the parallel ACCOUNTS lane, **not covered by this PASS, needs its own QA before boarding**; (iii) any sixth Source file = unreviewed diff, stop and route back.
2. Suite expectation stays **118**; this diff moves no test surface.
3. GH-R13 ledger: this rider boards WITH TASK-649's commit, and the commit message/ledger should name TASK-637 + this report.
