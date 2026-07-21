# Handoff — TASK-235 — Consolidated M7.5 commit: anim batch + retired TASK-210 pile (build-master)

**Date:** 2026-07-19 · **Status: DONE — commit `f609887` on `main` (parent `cae0412`), 139 files, NOT pushed.**

## 1) Commit facts

- Hash: **`f609887`** — "TASK-235: consolidated M7.5 commit — Meshy anim batch (7/9 units live) + retired TASK-210 pile"
- Built via the proven TASK-228 **worktree route**: `git worktree add --no-checkout` on `main` + `read-tree main` + `cp --parents` of the explicit 139-path scope list + explicit-pathspec `git add` + commit + worktree teardown. The primary tree (checked out on `m7.6-arena10x` @ `42011de`, editor LIVE) was **never checked out, reset, or touched** — verified byte-identical before/after (`git status --porcelain` sha1 `1dfd44bd…`, 96 lines, both sides; branch ref + HEAD unchanged).
- Windows note for future runs: the scratchpad worktree path exceeds MAX_PATH for the deepest FBX paths — `git -c core.longpaths=true` was required on add/commit, and teardown needed `rm -rf` + `git worktree prune` (`git worktree remove` fails on the long paths).

## 2) File count by category (139 total)

| Category | Files |
|---|---|
| Live Meshy anims `A_<Unit>_{Idle,Walk,Attack,Death}` (Footman, Knight, Pikeman, Cleric, Longbowman, Miner, MilitiaMob) | 28 |
| `Backup_Procedural/` originals (same 7 units) | 28 |
| `MeshyRetargeted/<Unit>/A_*_meshy.fbx` raw evidence (9 units incl. held Cavalry/Sapper) | 36 |
| `Tools/ArtPipeline/retarget_meshy_to_siegebiped.py` (TASK-229, QA PASS) | 1 |
| `SK_<Unit>.uasset` re-rigs (TASK-213) | 8 |
| `RawAssets/Characters/<Unit>.fbx` renamed rig sources (TASK-212) | 8 |
| Tooling: `rig_character.py`, `fix_rig_root.py`, `meshy_generate.py`, `reimport_meshes.py` | 4 |
| `.claude/hooks/guard-secrets.sh` (msy_ pattern) | 1 |
| `IK_MeshyBiped` / `IK_SiegeBiped` / `RTG_MeshyBiped_to_SiegeBiped` | 3 |
| `Meshy/Footman/` TASK-203 FBXs | 5 |
| Handoffs (TASK-198/199/203/211/212/213/220/221-222 §7 delta/229/230/231/232) | 12 |
| QA reports (TASK-198-qa, TASK-212-qa, TASK-229-qa) | 3 |
| `TASKBOARD.md` + `CONVENTIONS.md` snapshot (mixed-lane text, incl. M7.6 sections — accepted per spec) | 2 |

## 3) Verifications performed

- **QA gates:** all four code items PASS — qa/TASK-229-qa.md, qa/TASK-198-qa.md, qa/TASK-212-qa.md (in this commit), qa/TASK-216-217-220-qa.md for reimport_meshes.py (TASK-220 PASS; that report file is branch-committed in `42011de`, noted).
- **LFS:** all 116 staged binaries (.uasset/.fbx) verified as LFS pointer blobs in the index before commit.
- **Secret scan:** staged text diff clean against the full token-format family (incl. the new `msy_` pattern); meshy_generate.py keeps keys env-only with redaction, no literals.
- **Leakage scan:** commit contains zero branch-owned paths (no L_Arena.umap, DA_BattlefieldScatter, TASK-216 Source files, DefaultEngine.ini), no AB_Test, no .fbm, no __pycache__, no double-committed cae0412 content (spot-verified byte-identical: overnight handoffs 223/225/226, Meshy 8-unit FBXs, glow materials, T_ uassets, pipeline_manifest.json).
- **Staged set == scope list exactly** (139 = 139, diff empty).

## 4) Index reconcile (primary tree)

The editor's SCC integration had auto-staged all 28 `Backup_Procedural/` uassets (`A ` in the index — the TASK-232 §7 git note; the older Src_Walk_LiveSpike/RTG `AM` strays from TASK-221-222 were already gone). Unstaged via `git restore --staged` BEFORE the commit window; they now show untracked in the primary tree, and their content is committed on main. Nothing rode implicitly.

## 5) Judgment call: AB_Test excluded

Dispatch left `Content/Characters/Anims/AB_Test/` to my call unless a handoff marks it keep. No handoff does — TASK-221-222 §5 calls it "evidence, scratch — delete with AB_Test at cleanup" and TASK-230 §6 only pruned it. **Excluded** (11 uassets stay untracked). The export-defect regression base is preserved in the commit anyway (Meshy source FBXs + IK_/RTG_ + the tool). A future cleanup task can delete the folder.

## 6) Remaining dirty tree (primary, vs branch `42011de`) — fully accounted

- **Branch-owned M7.6, committed on the branch (13):** L_Arena.umap, DA_BattlefieldScatter.uasset, 5 TASK-216 Source files, DefaultEngine.ini, handoffs TASK-214/216/217/218, qa/TASK-216-217-220-qa.md — differ from main by design until the Phase-6 merge gate.
- **cae0412/f609887 content showing dirty only because the branch base (`a33aba6`) predates it:** glow materials, retextured T_/PNG/root-FBX set, Meshy/MeshyRetargeted/Backup_Procedural dirs, anims, SKs, tooling, docs — all safe in main commits; resolves when the branch merges or rebases main.
- **Truly uncommitted anywhere (18, deliberate):** AB_Test 11 uassets (§5), 5 `Meshy/Footman/*.fbm/texture_0.png` embedded-texture extractions (cae0412 precedent: .fbm never committed), 2 `Tools/ArtPipeline/__pycache__/*.pyc` (junk; .gitignore doesn't cover __pycache__ — worth a hygiene line someday).
- **Post-commit doc deltas (this wrap-up):** TASKBOARD.md flips (235/232) + this handoff — ride the next docs window per convention.

## 7) Follow-ups for the manager

- AB_Test cleanup task (delete or archive the 11 evidence uassets) + optional `.gitignore` line for `__pycache__/` and `*.fbm/`.
- TASK-233 (Cavalry quadruped) / TASK-234 (Sapper re-rig) remain backlog; their raw provenance is now committed.
- Longbowman draw-overshoot stays on the M7.5 checkpoint WATCH list (TASK-231 ruling 3).
- SK_MilitiaMob/SK_Knight `bUsedWithSkeletalMesh` missing-usage-flag WATCH (TASK-232 §6) — cosmetic, fold into an M7.5 polish line.
