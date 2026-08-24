# TASK-637 — [GH-12] THE COMMENT-RIDER SWEEP (gameplay-programmer handoff)

**Status: ready-for-qa (TASK-638 reviews; orchestrator flips the board — the no-TASKBOARD-edit fence was honoured).**
Comment-only, ZERO behaviour. No compile run (rides TASK-649's slot, QUIET-MODULE). No editor/MCP, no git writes, no board edit. Date: 2026-08-23.

Model: the TASK-623 `Castle.h` GetInteriorAnchorLocation rider (now at `Castle.h:199-203` post-shift). Truth sources: `handoffs/TASK-629-artist.md` (as-built hall: 2910 × 1140 uu, flat floor z 174 end-to-end, flat ceiling z 2160, 1986 uu clear — measured) · TASK-617 C1 (both castle actors yaw 0, both gates face world −Y, measured live).

---

## 1. FILES TOUCHED (exactly three, comments only)

- `Source/GitClaudeUnrealTest/Siegebound/Torch.h` — 1 site
- `Source/GitClaudeUnrealTest/Siegebound/Castle.h` — 2 sites
- `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp` — 2 sites

⚠️ **Tree note for QA/build-master:** `git diff --stat -- Source/` ALSO shows `SiegeAccountSaveGame.h` (+36) and `SiegeAccountSubsystem.h` (+6) — that is the **parallel ACCOUNTS client lane**, declared in my dispatch (zero overlap with my three files). Not TASK-637's diff; do not attribute it to me, do not treat my three-file claim as covering it.

## 2. THE FIVE SITES — before/after (all re-located by CONTENT per the spec's ⛔; the board's ≈:621 estimate was stale, the site sat at :602)

### (a) `Torch.h` — TorchAttenuationRadius doc (was :183-184, now :182-192)
- **BEFORE:** "the 9× castle's grand hall is ≈2910 × 720 uu with ≈1560 uu clear height (WR-§1)"
- **AFTER:** "the AS-BUILT grand hall is 2910 × 1140 uu with 1986 uu clear height — flat floor z 174 end-to-end, flat ceiling z 2160, MEASURED in handoffs/TASK-629-artist.md (WR-§1's 1560 is the design MINIMUM the build clears, not the ceiling)" + a TASK-637/GH-R13 rider naming the old figures as the pre-redesign carve. The pool-vs-floodlight argument the numbers served is kept (and noted stronger in the deeper hall). `TorchAttenuationRadius = 1200.0f;` and every UPROPERTY line byte-unchanged.

### (b1) `Castle.h` — the class-doc furnishing rationale, reason (2) (was :70, now :69-79)
- **BEFORE:** "Castle_Red's yaw 180 is handled for free because every anchor is CASTLE-MESH-RELATIVE."
- **AFTER:** "any future re-pose is handled for free because every anchor is CASTLE-MESH-RELATIVE" + rider: the yaw-180 claim was STALE — both actors yaw 0, gates face world −Y, measured at TASK-617 C1; the mesh-relative reasoning stands on its own.

### (b2) `Castle.h` — InteriorAnchorRelativeLocation doc (was :470, now :475-481)
- **BEFORE:** "which applies the actor transform (so Castle_Red's yaw 180 is handled for free)."
- **AFTER:** "(so any castle pose is handled for free — TASK-637 comment rider, GH-R13: the old parenthesis credited 'Castle_Red's yaw 180', STALE per TASK-617 C1; the transform form keeps the value pose-proof if a level edit ever yaws one — the TASK-623 rider reasoning)."

### (b3) `Castle.cpp` — SpawnCastleFurnishings mesh-transform composition (was :602, now :601-609)
- **BEFORE:** "never the actor's location: Castle_Red is placed at yaw 180 and the whole point of a relative anchor is that the rotation comes along for free"
- **AFTER:** "the whole point of a relative anchor is that any castle pose comes along for free" + rider: the yaw-180 justification was STALE (617 C1); the composition is correct at ANY pose, which is the real reason it is written this way. `const FTransform CastleMeshTransform = CastleMesh->GetComponentTransform();` byte-unchanged.

### (b4) `Castle.cpp` — GetInteriorAnchorLocation body (was :1131-1137, now :1133-1146) — TWO claims in one block
- **BEFORE:** ":1130-1131 'Castle_Red is placed at yaw 180, so a non-zero relative anchor has to ROTATE…'" and ":1136-1137 'RESOLVED WORLD POINTS at the shipped L_Arena placement (Castle_Blue (−25000, 0, 0) yaw 0, Castle_Red (+25000, 0, 0) yaw 180 — TASK-218)'"
- **AFTER:** rotation rationale re-grounded on the hypothetical ("if a level edit ever yaws one") + one rider covering both claims (617 C1; "TASK-218's yaw-180 plan is history, not the map") + the placement note restated as measured: "Castle_Blue (−25000, 0, 0) and Castle_Red (+25000, 0, 0), BOTH yaw 0 — TASK-617 C1", resolved points unchanged ("identical under any yaw while the anchor stays ZeroVector"). `return GetActorTransform().TransformPosition(InteriorAnchorRelativeLocation);` byte-unchanged.

**Already-corrected-by-intervening-task declarations: NONE.** All five sites were still stale at sweep time (content-grep proof below predates my edits in the session log); the GH-R11 lane carried none of them.

## 3. COMMENT-ONLY PROOF

`git diff -U2` over the three files: every `+`/`−` line is a `//` or ` * ` comment line. Zero expression/UPROPERTY/signature/value bytes; the three adjacent code lines quoted above verified byte-identical in the diff context. No literal `*/` introduced inside any doc-comment text (compile-trap law honoured). Line-shift ledger: Torch.h +6 · Castle.h +11 (+6 at site b1, +5 at site b2 — the TASK-623 rider moved :194→:199, TorchAnchors yaw table :512→:523) · Castle.cpp +9 (+4 at b3, +5 at b4).

## 4. POST-CONDITION GREP PROOF (spec (3))

**Sweep 1 — `grep -rn "yaw 180" Source/`** → 14 hits, classified, zero normative:
- `BattlefieldScatter.cpp:1613/1630/1901`, `BattlefieldScatter.h:134/398/419`, `ScatterConfig.h:408` — the scatter TWIN law (primary yaw 0 / twin yaw 180 about map centre, TASK-358) — a different, still-true mechanic, not a castle-actor claim.
- `Castle.cpp:325` (hypothetical 7th torch anchor facing) · `Castle.h:523` (torch yaw facing-convention table) — facing semantics, not placement claims.
- `Castle.h:73` · `Castle.h:200` (the TASK-623 model) · `Castle.h:478` · `Castle.cpp:606` · `Castle.cpp:1140` — riders QUOTING the falsified claim as STALE (historical citations, excluded by the post-condition by its own terms).

**Sweep 2 — `grep -rniE "Castle_Red.{0,40}yaw 180|yaw 180.{0,40}Castle_Red" Source/`** → 3 hits (`Castle.h:200`, `Castle.cpp:606`, `Castle.cpp:1140`), all inside STALE-marked rider quotes. **Zero normative Castle_Red-yaw-180 claims remain.**

**Sweep 3 — `grep -rn "720" / "1560" / "2910"` over `Siegebound/`** (byte-safe literals; note for TASK-638: a `2910 . 720` regex UNDER-matches — the `×` is multi-byte, grep `.` is one byte — enumerate the bare numbers instead) → every hit classified, zero normative stale dims:
- `Castle.cpp:62-63` — the `//~` BIRTH-record provenance block (qa/TASK-634.md nit (b): ruled correctly handled, historical, no action — left alone deliberately).
- `Castle.cpp:127-128` — 634's as-built correction ("1140 deep, was 720" — quotes the old value AS history) · `Castle.cpp:279` — "2910 × 1140 hall", correct.
- `Castle.cpp:99` — `constexpr float HallClearHeightZ = 1560.f;` — a CODE VALUE, the documented WR-§1 design minimum (634/635 HOLDS verdict: as-built 1986 ≥ 1560). Not a stale claim; touching it would be a behaviour byte.
- `Castle.h:439/452/466` — the GATE's measured 1560-uu COLLISION-gap span — a different 1560, not hall clear height.
- `SiegeAssistantSnapshot.{h,cpp}` line-budget 720s · `SiegeNavAreas.h:131` engine-source line numbers — unrelated.
- `Torch.h:183/185/189` — the new as-built text + the rider's STALE-marked quote.

## 5. WHAT QA (TASK-638) SHOULD SCRUTINIZE

- Verify comment-only AT THE DIFF (not this claim): the three files' `+/−` lines all carry comment prefixes; the two ACCOUNTS headers in the tree are the parallel lane, not mine.
- Cross-check my cited numbers against `handoffs/TASK-629-artist.md` §2/§4 (2910 × 1140 / 174 / 2160 / 1986) and the 617 C1 yaw-0 record.
- Reproduce the greps; use byte-safe literal patterns (the `×` multi-byte trap above).
- The five sites' new line positions: Torch.h:182-192 · Castle.h:69-79 · Castle.h:475-481 · Castle.cpp:601-609 · Castle.cpp:1133-1146.

## 6. NOT DONE, ON PURPOSE

⛔ No compile (QUIET-MODULE — the diff rides TASK-649's slot). ⛔ No edit to `Castle.cpp:62-63` (ruled historical), `HallClearHeightZ` (code value), the gate-gap 1560s, or any scatter/torch yaw text (not stale). ⛔ No TASKBOARD/git/editor/MCP touch.
