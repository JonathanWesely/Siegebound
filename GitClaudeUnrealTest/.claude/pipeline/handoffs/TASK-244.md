# Handoff — TASK-244 — Consolidated evening-batch commit: Archer/Ogre SK batch + owed items (build-master)

**Date:** 2026-07-22 · **Status: DONE — commit `b90157e` on `main` (parent `59a994a`), 63 files, NOT pushed. Evening batch (TASK-242..244) CLOSED; 10/11 unit card types animated (Cavalry quadruped TASK-233 is the last static unit).**

## 1) Commit facts

- Hash: **`b90157e`** — "TASK-244: Archer+Ogre rigging batch + owed Sapper wire + MI_Castle_PBR (TASK-234/242/243) — 10/11 unit types animated"
- Worktree route (TASK-235/240-241 pattern): `git worktree add --no-checkout C:/GitProjects/wt244 main` (short path — the MAX_PATH lesson) + `read-tree main` + `cp --parents` of the explicit 63-path scope + explicit-pathspec `git -c core.longpaths=true add` + commit + `rm -rf` teardown + `git worktree prune`.
- **Primary tree byte-verified untouched:** `git status --porcelain | sha1sum` = `7474b97e…` identical before/after; HEAD stayed `42011de` on `m7.6-arena10x`; editor (live throughout) never touched.

## 2) File breakdown (63)

| Category | Files |
|---|---|
| SK_Archer + SK_Ogre first skeletal imports (TASK-243) | 2 |
| `A_{Archer,Ogre}_{Idle,Walk,Attack,Death}` first-authored clips (TASK-243) | 8 |
| `A_Sapper_*` live re-rig wire (TASK-234 §7c) | 4 |
| `Backup_Procedural/A_Sapper_*` originals (TASK-234) | 4 |
| `MI_Castle_PBR` (TASK-239 session-3 texture-streaming rebuild save) | 1 |
| Rigged SiegeBiped FBXs `Characters/{Archer,Ogre}.fbx` (TASK-242) | 2 |
| Procedural anim exports `Characters/Anims/{Archer,Ogre}_*.fbx` (fleet precedent — main tracks the fleet's) | 8 |
| `Meshy/{Archer,Ogre}/` rig+clip FBXs (TASK-242) | 10 |
| `MeshyRetargeted/{Archer,Ogre}/A_*_meshy.fbx` (TASK-242) | 8 |
| `Meshy/Sapper/` + `MeshyRetargeted/Sapper/` updated to the TASK-234 re-rig (content update superseding the f609887-era rig — hash-verified DIFF, not a double-commit) | 9 |
| `Tools/ArtPipeline/rig_manifest.json` (Archer+Ogre entries) | 1 |
| `TASKBOARD.md` snapshot (mojibake as-is per dispatch) + handoffs TASK-234/239/240-241/242/243 | 6 |

## 3) Gates + verifications

- **Structural readback over MCP pre-commit (all green):** SK_Archer/SK_Ogre exist, bound to shared `SK_Footman_Skeleton`, slots `[TeamRegion→MI_TeamColor_Blue, <Unit>PBR→MI_<Unit>_PBR]`, LOD0-only, saved not-dirty; all 8 A_{Archer,Ogre}_* + 4 live A_Sapper_* + 4 backups exist not-dirty; SM_Archer/SM_Ogre StaticMesh fallbacks intact; MI_Castle_PBR saved (not dirty in editor, DIFF on disk vs main = the rebuild save).
- **LFS:** 56/56 staged binaries (.uasset/.fbx) verified as LFS pointer blobs in the index.
- **Secret scan:** all 7 staged text files clean against the token-format family (hf_/msy_/sk-/ghp_/AKIA/xox/private-key/Bearer). No tokens anywhere; env-only law upheld.
- **Leakage scan:** zero branch-owned paths (no L_Arena.umap, DA_BattlefieldScatter, DefaultEngine.ini, Source/), no AB_Test, no .fbm, no __pycache__, no Cache/, no GDD-Submission pdf, no scratchpad refs.
- **Staged set == 63-path scope list exactly** (diff empty).
- Art-lane batch: no C++/tooling `.py` deltas (rig_manifest.json is a data manifest) — gate evidence is the art/PIE chain: handoffs/TASK-242.md (Blender rig + retarget gates), TASK-243.md (UE re-gates + PIE verdicts), TASK-234.md §7c (wire + mandatory PIE visual PASS).

## 4) MI adjudication (the dispatch's INVESTIGATE item)

`MI_Archer_PBR` + `MI_Ogre_PBR`: **EXCLUDED — no on-disk delta exists.** sha256 of both working files == the LFS oids in `main` (and they appear in neither status-vs-branch nor diff-vs-main). The in-session dirty flag TASK-243 reported is in-memory only (MCP `is_dirty` still true for both at this window; never saved to disk). So neither the "missed b9a756d" theory nor anything inexplicable — there is simply nothing to commit. Per TASK-243 §4's own rule: Jonathan's save toast decides; if a future save DOES produce a disk delta, diff it then.

## 5) Double-commit checks (all resolved)

- `cards.csv` — byte-identical to main (`c2b2f59` carried it). Skipped.
- `CONVENTIONS.md` — byte-identical to main. Nothing to commit (dispatch expected churn; there is none).
- `MeshyRetargeted/Sapper/` — in f609887 with the OLD (TASK-223) rig; TASK-234 overwrote on disk → hash DIFF → committed as a content update (plus `Meshy/Sapper/` same story, TASK-234 §6).
- qa/TASK-229-qa.md, qa/TASK-236-237-qa.md, TASK-201.md wave-2 append, TASK-221-222.md day-deltas — all verified already committed (SAME vs main).

## 6) Index reconcile (primary tree)

Editor SCC had auto-staged 14 new-file entries in the branch index (SK_Archer/Ogre, A_Archer/Ogre ×8, Backup_Procedural/A_Sapper ×4). Unstaged post-commit via `git restore --staged` (TASK-235 §4 precedent) — they now show untracked in the primary tree; content safe on main at `b90157e`. Nothing rode implicitly.

## 7) Remaining dirty tree (primary, vs branch `42011de`) — fully accounted

- **Zero** files are both modified-vs-branch AND different-from-main (verified by set intersection — every `M` entry is pure branch-base dirt, resolves at merge/rebase).
- **Branch-owned by design (differ from main until the Phase-6 merge gate):** L_Arena.umap, DA_BattlefieldScatter.uasset, DefaultEngine.ini, BattlefieldScatter.{h,cpp}, ScatterConfig.h, SiegeBotController.h, handoffs TASK-214/216/217/218, qa/TASK-216-217-220-qa.md (all committed on the branch at 42011de; clean in status).
- **Truly uncommitted anywhere (deliberate leftovers):**
  1. `Content/Characters/Anims/AB_Test/` — 11 evidence uassets (TASK-235 §5 ruling; cleanup task still pending with the manager).
  2. `Content/VFX/M_Spell_LightningStrike.uasset` + `NS_Spell_Lightning_NEW.uasset` — TASK-239 parked WIP (blocked-rework), excluded by every window since.
  3. `Docs/GDD-Submission-v3.pdf` — left untracked per dispatch (Jonathan hasn't decided).
  4. `Meshy/Footman/*.fbm/` embedded-texture extractions — never committed (cae0412 precedent).
  5. `Tools/ArtPipeline/__pycache__/` — junk (the .gitignore hygiene line is still an open manager follow-up).
- **Post-commit doc deltas (this wrap-up, ride the next docs window per convention):** TASKBOARD.md flips (243/244 + batch closure) and this handoff.

## 8) Follow-ups (for the manager)

- Cavalry quadruped (TASK-233) is now the ONLY unanimated unit card — the 11/11 closer.
- TASK-239 Lightning WIP remains parked blocked-rework (round-2 post-mortem in handoffs/TASK-239.md).
- TASK-234 §7c.5 gameplay observations (Sapper survives unit-kills unless its own suicide tick fires; spawn-overlap instant-annihilation) — programmer-lane candidates if the GDD intends always-die-on-attack.
- Standing hygiene items: AB_Test cleanup, `__pycache__`/`*.fbm` .gitignore lines, TASKBOARD/CONVENTIONS mojibake cleanup pass (separate flagged task).
