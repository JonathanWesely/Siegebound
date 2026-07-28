# TASK-343 — [FID-int] Archer + Pikeman chroma-fidelity re-bake integration — build-master handoff

**Date:** 2026-07-27 night · **Branch:** main · **Editor:** PID 6460 session (inherited from TASK-346, left RUNNING with MCP up) · **Verdict: PASS — both `_D` textures same-path imported (MD5-readback-proven), sampler sweep clean, Simulate-verified, TWO per-unit commits, NO push.**

Recipe executed from `handoffs/TASK-342-artist.md` (turnkey). **Delivery form honored: TEXTURE-SET-ONLY, and narrowed by the dispatch to the two `_D` diffuses ONLY** — N/ORM byte-identical on disk (no import, keeps their uassets out of the diff), FBX/SM/SK/MI/anims/LODs untouched. NO mesh work of any kind. `L_Arena` NEVER saved.

---

## 1. Imports (texture-skip trap law — explicit, with landed-readback)

Lane: MCP `TextureTools.import_file` refuses same-path overwrite → the proven TASK-297/225 remote-exec lane (`bRemoteExecution` flipped in-memory via MCP ObjectTools; engine `remote_execution.py` client; `unreal.AssetImportTask(replace_existing=True, automated=True)`), textures saved explicitly — the only two assets saved this session.

| Asset | Pre-import (old) | Post-import readback | Expected |
|---|---|---|---|
| `/Game/Textures/T_Archer_D` | FileMD5 `acef4286…`, stamp 1785110072 | **FileMD5 `ea2e4b0a5a9cf81f84152fc36314a111`, stamp 1785211529 (= new PNG mtime 2026-07-27 21:05:29), 1024², sRGB **True**, TC_Default, DXT1** | new PNG MD5 `EA2E4B0A…` (1,347,210 B) ✅ |
| `/Game/Textures/T_Pikeman_D` | FileMD5 `4aa9fcff…`, stamp 1785117683 | **FileMD5 `269c7354c405607985df0b3c8f0f0789`, stamp 1785211539 (= new PNG mtime 21:05:39), 1024², sRGB **True**, TC_Default, DXT1** | new PNG MD5 `269C7354…` (1,267,380 B) ✅ |

Byte-level proof the new bakes landed: the asset-registry import-source MD5 EQUALS the staged PNG's disk MD5, per unit. Simulate/PIE verified STOPPED before both imports (`IsPIERunning=false`).

## 2. SAMPLER-TYPE sweep — PASS

- Material referencers: `T_Archer_D` → **exactly `MI_Archer_PBR`**; `T_Pikeman_D` → **exactly `MI_Pikeman_PBR`**; both parent `M_AssetPBR`. No master node-defaults these textures (trap precondition absent; compression class unchanged `TC_Default`→`TC_Default`).
- **`Failed to compile Material` grep over the full session log: 0 hits** — also 0 for ensure / Fatal / `Accessed None` / `LogOutputDevice: Error` (one combined sweep at end of session, after imports + five Simulate rounds).

## 3. Meshes — VERIFY-ONLY spot checks (nothing imported)

Live readbacks on Simulate instances: `SkeletalVisualMesh` mesh = `SK_Archer` / `SK_Pikeman`; slots `[0 MI_TeamColor_<team>, 1 MI_<Unit>_PBR]` on every instance both teams; capsule half-heights **READ: Archer 90.0 / Pikeman 95.0** (law values, not assumed); skel rel-Z **−90.01 / −94.94** (capsule-half-height-relative grounding, TASK-307 systemic fix live); rel-yaw −90 both. Git delta contains NO SM/SK/MI/anim files — chains untouched by construction.

## 4. Simulate verify on `L_Arena` (Simulate, never PIE-in-viewport) — OBSERVATIONS

Method: VERIFYCAP `BP_Unit_{Archer,Pikeman}` editor placements (Blue + Red each), Team set per-instance, Simulate via MCP; live match ran alongside. Five rounds were needed — units auto-march ~350 uu/s and verify pairs kept dying to the live match (VERIFYCAP kill log: Blue pair died 3×, once to each other when staged 400 uu apart). Working method for the record: **freeze via runtime `MaxWalkSpeed=0`** on the PIE instances (transient, anims keep ticking), stage enemy pairs FAR apart. A slomo-0.02 freeze attempt made every capture read dark — **eye adaptation runs on game time; slomo stalls exposure** (recorded so the next verify doesn't repeat it).

- **(a) Colour/concept:** the Archer's grey-teal chalk is GONE — tunic reads dark muted forest-green with warm brown leather (`TASK-343-verify-BluePair-corridor.png`, `-BluePair-tight.png` vs the committed shipped-state `TASK-312-verify-Archer-tight.png`: the difference is stark; the new bake is deliberately darker — luma retention came down from 1.56× to 1.01×, so expect a MOODIER unit than the old bleached one). Pikeman reads olive-grey tabard + slate trousers + warm wood pike at gameplay distance; its BACK is dominated by the pale cape (`TASK-343-verify-RedPair-litback.png`) — **recorded shipped property (artist's back-view luma 1.776 note), not a defect**. Flat-lit truth = the committed galleries `TASK-342-BEFORE_AFTER_{Archer,Pikeman}.png` (concept | shipped | final); the byte proof is §1. Sun on L_Arena is azimuth −35°/elev 38° (READ from `DirectionalLight_0`): Red units' fronts are geometrically backlit everywhere on the map (`TASK-343-verify-PikemanRed-front.png`, wide-pike stance) — true for the shipped state too, noted for Jonathan's angle-of-view at playtest.
- **(b) Team recolour:** slot-0 readback `MI_TeamColor_Blue`/`MI_TeamColor_Red` on all instances both teams (BeginPlay recolor live). Blue bands read clearly on both units (Archer 3.29% neck/shoulder; Pikeman's **10.31% wrap — DELIBERATE per ruling 5, not flagged**, `TASK-343-verify-PikemanBlue-closeup.png`). Red bands are subtle at gameplay angles (deep red on dark cloth under warm sun; the Ogre TASK-341 residue generalizes to Archer/Pikeman).
- **(c) Anims TICK:** two-sample `hand_r` bone delta with a forced render between samples — **Archer 20.8 uu, Pikeman 31.6 uu** pose advance at unchanged actor location; with the viewport unrendered the pose freezes (URO + `OnlyTickPoseWhenRendered` from C++, working as designed, TASK-341 parity). Walk/attack/death cycles all observed across rounds (the VERIFYCAP casualties died with death anims; a Blue-vs-Red archer duel ran bow-draw cycles, `TASK-343-verify-Archer-duel.png`).
- **(d) Facing/grounding:** OBSERVED only, nothing authored — Blue marches yaw≈0 (+X), Red yaw≈180 (−X), feet on grass with attached shadows in all captures.
- **(e) Logs:** ensure 0 · Fatal 0 · Accessed None 0 · `LogOutputDevice: Error` 0 · `Failed to compile Material` 0 (whole session).

## 5. Commits (per-unit revert model, explicit pathspecs, NO push)

1. **`3770bf6`** — Archer: `T_Archer_D.png` + `T_Archer_D.uasset` + `pipeline_manifest.json` (**carries BOTH units' pins** — an Archer-only revert also reverts Pikeman's γ1.0/g2.3 pin; noted per the recipe) + `handoffs/TASK-342-artist.md` + board.
2. **(this commit)** — Pikeman: `T_Pikeman_D.png` + `T_Pikeman_D.uasset` + this handoff + board + galleries (`TASK-342-BEFORE_AFTER_*.png`, copied from gitignored Cache into handoffs/ — the TASK-310 precedent; Cache itself stays ignored) + 6 verify PNGs (all LFS).

`git diff --stat` verified clean of anything foreign before each commit (N/ORM/FBX byte-identical = absent from the delta, as required). `git reset --hard`/`git clean -fd` never used; one git command per call.

## WATCH (binding, say-it-here duty)

**Jonathan's playtest eye is FINAL.** Both units pass the chroma gate, but: (1) the Archer is now deliberately darker than the old bleached bake — if it reads "too dark" in play, the numbers go to the manager (headroom exists: the gate band allows brighter within luma ≤1.25×); (2) **Pikeman chroma is DONOR-CAPPED at ≈0.8** — if the tabard still reads washed-out to Jonathan, the remaining headroom needs a fresh Meshy donor = manager go + Jonathan's pose-reroll answer (re-gen re-rolls the accepted TASK-318 wide-pike stance) + ~30 cr (balance 2446, untouched this task).

## Residue / notes

- Red-band legibility at gameplay angles now observed on THREE units (Ogre TASK-341, Archer, Pikeman) — pattern for the manager's existing residue item, not new work here.
- Editor left RUNNING (PID 6460) with MCP up; `L_Arena` dirty-unsaved (VERIFYCAP placements added and deleted, never saved); `bRemoteExecution` flip is in-memory only (reverts on restart).
- Slomo/eye-adaptation interaction + the `MaxWalkSpeed=0` freeze method recorded in §4 for future Simulate verifies.
