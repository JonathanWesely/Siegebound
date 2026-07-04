# M3 Kickoff — M2 safety branch + editor recovery (build-master)

**Date:** 2026-07-04
**Author:** build-master
**Context:** Jonathan authorized M3 work to proceed on `main` even if it breaks M2 testability, on the condition that the M2-testable state is preserved on a safety branch. UE5 crashed and was brought back up this session.

## Commits (on `main`, NOT pushed)

| Commit | Contents |
|--------|----------|
| `40b69ef` | **M2 art derived-data re-save** — 4 LFS assets (`L_Arena.umap`, `SM_GoldNode`, `SM_ArrowTower`, `SM_Wall`) re-saved by the crash-recovered editor on interactive load, baking derived data the headless MCP import (TASK-038) skipped. Verified intact before commit (see below). |
| `f903cf0` | **M2 wrap + M3 decomposition (docs)** — TASKBOARD statuses 021-040 done, CONVENTIONS shadow-var + M3 rules, `handoffs/TASK-040.md`. Docs/handoffs only. |

`main` HEAD = **`f903cf0`** (ahead of `origin/main` by 13, unpushed). Working tree clean.

## M2 safety branch

- **`m2-testable`** created at **`f903cf0`** (pinned; NOT checked out).
- This is the snapshot to `git checkout m2-testable` to playtest M2 if later M3 work disrupts `main`. It carries the enriched M2 game assets + the finished M2 board record.
- `main` remains active; M3 work proceeds there.

## Unexpected uncommitted art — resolved (halt + verify + preserve)

The pending changes were NOT docs-only as expected: 5 LFS content assets had been re-saved by the recovered editor at 2026-07-04 06:33 (single "Save All" pattern). I halted before branching and reported. Coordinator ruling: these are derived-data enrichment, not new intentional art (no agent was doing art work). Directed to VERIFY, then preserve the 4 M2 assets and restore the read-only donor.

**MCP verification (editor UP, read-only) — all PASS, matches TASK-038:**

| Asset | Tris | Bounds X×Y×Z | Material slot 0 |
|-------|------|--------------|-----------------|
| SM_GoldNode | 222 ✓ | 190.2 × 200.0 × 249.4 ✓ | M_GoldGlow → /Game/Materials/M_GoldGlow ✓ |
| SM_ArrowTower | 512 ✓ | 250.0 × 250.0 × 497.4 ✓ | — |
| SM_Wall | 264 ✓ | 400.0 × 100.0 × 250.0 ✓ | — |

**L_Arena actors (read-only):** GoldNode_Blue @ (-1200,0,0) ✓, GoldNode_Red @ (+1200,0,0) ✓; 4 boundary walls ArenaBoundary_East/West/North/South ✓; KillZ = -2000 ✓. Current loaded level = `/Game/Maps/L_Arena`.

LOD0 geometry/materials/level were byte-equivalent in every checked value; the 30KB→149KB size jump on SM_GoldNode is built derived data, not geometry change. Re-saves committed as `40b69ef`.

**Donor restored:** `Content/Input/Touch/UI_TouchSimple.uasset` — READ-ONLY donor; its boot-resave was churn, restored to HEAD via `git checkout --` (donor law + prior precedent). Not committed.

## Editor / MCP status

- **MCP: UP / reachable** — `list_toolsets` + full read-only asset/level queries succeeded. The recovered editor is running with `L_Arena` loaded.
- No editor boot/bounce/mutation was performed by build-master (verification was read-only). Ready for the later M3 editor wave.

## Notes for downstream

- M3 imminent work is file-only C++ (no editor needed). Editor is available when the M3 editor wave arrives.
- Do NOT push without Jonathan's explicit go-ahead (still true; 13 commits ahead of origin).
