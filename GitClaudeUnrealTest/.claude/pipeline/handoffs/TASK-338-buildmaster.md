# TASK-338 — [CASTLE-crumble-tune-int] DONE: independent band re-measure PASS + commit of the TASK-339 master fix and TASK-337 retuned MIs

**Status: done.** Date: 2026-07-27 · build-master. Verified in **Simulate** on `L_Arena` (never PIE-in-viewport), exclusive editor, own commit on main (hash in the board line / Slack), **no push**. `L_Arena` never saved; no editor-world mutation of any kind (all drives ran on PIE-world actors; both worlds probe-verified untouched).

## SAMPLER-TYPE TRAP sweep (the new integration law) — PASS

- **`Failed to compile Material` grep = 0 hits post-fix.** Same editor session as TASK-337/339: last failure line in the session log is **22.33.20** (the pre-fix `MI_Castle_Crumble01` block from TASK-337's first run). Nothing after TASK-339's 22.45 enum fix — through THREE full Simulate loads today (two TASK-333 gold sessions + this castle session, all loading `M_CastleCrumble` + all three MIs) and every stage render below. Master AND MIs compile; zero `Failed to compile Material Instance with Base M_CastleCrumble` lines post-fix.
- **Forced deterministic recompile:** `MaterialTools.recompile` on `/Game/Materials/M_CastleCrumble` (the tool RAISES on shader failure) — returned success, logged zero new LogMaterial lines (the cached-clean path).
- **Terminal live proof:** the three stages measure DISTINCT wall values (113.8 / 75.1 / 57.4 — see below). A Default-Material fallback renders all stages bit-identical (~0.52×P flat, the TASK-337 first-run signature); this is structurally impossible in the table below.

## Independent band re-measure (TASK-331/337 protocol, NOT taken on trust)

Protocol reproduced: frozen camera pose loc (−23200, −1300, 550) / rot (−4.4, 144.2, 0) — set + readback-verified exact; bot neutralized via `StopDecisionTimer()` **before any summon** (0 units existed all session — no interference, no win-state risk); Blue castle found **by class + Team enum** (`Castle_0`, never by label); driven 2000 → 1480 → 980 → 480 HP by instigator-less `apply_damage` (520/500/500); each stage readback-verified to fire ONCE in order with the correct mesh AND `MI_Castle_Crumble0N` on BOTH slots; HP re-verified stable between damage and shot; 1920×1080 HighResShot; wall region (800,980,615,695), grass control (300,650,850,1000); mean Rec.709 luma on 8-bit sRGB.

MI values readback-verified live before measuring: MI01 `Darken 0.38 / Scorch 0.18` · MI02 `0.24 / 0.52` · MI03 `0.28 / 0.82` (untouched) — exactly TASK-337's final table.

| State | Wall | Grass | ×P | Gate | Verdict | TASK-337 re-run |
|---|---|---|---|---|---|---|
| P pristine (`SM_Castle`, Blue accent + PBR) | **171.22** | 151.16 | 1.000 | — | — | 173.57 |
| S1 (`SM_Castle_Crumble01`, MI01 both slots) | **113.83** | 151.02 | **0.665** | 0.55–0.80 | **PASS** | 0.664 |
| S2 (`…02`, MI02 both slots) | **75.12** | 150.96 | **0.439** | 0.35–0.55 | **PASS** | 0.439 |
| S3 (`…03`, MI03 both slots) | **57.38** | 150.95 | **0.335** | ≤ 0.40 | **PASS** | 0.336 |
| gaps | | | S1−S2 = **0.226** · S2−S3 = **0.104** | ≥ 0.10 / ≥ 0.05 | **PASS / PASS** | 0.225 / 0.103 |

Exposure-consistent: grass 150.95–151.16 (spread 0.21). **All five band criteria PASS**, and the independent ratios reproduce TASK-337's to **±0.001** — two sessions, two implementations, same answer. Visual reads match the intent table: S1 = dimmed dusty tan with scorch patches, unmistakably "battle-worn but standing"; S2 = blackened charcoal, clearly worse; S3 = near-dead dark charred silhouette. Captures: `TASK-338-P-pristine.png` / `TASK-338-S1.png` / `TASK-338-S2.png` / `TASK-338-S3.png`.

## ResetCastle + isolation — PASS

- `ResetCastle()` on the driven castle: hp 2000, mesh back to pristine `SM_Castle`, **per-team accent restored via `ApplyTeamVisuals`** (slot 0 `MI_TeamColor_Blue`, slot 1 `MI_Castle_PBR`). Render proof: reset wall luma **171.50** vs P 171.22 (0.2 %) — `TASK-338-reset-restored.png`.
- **Other castle unaffected:** `Castle_1` (RED) read hp 2000 / `SM_Castle` / `MI_TeamColor_Red` + `MI_Castle_PBR` at baseline AND after the full drive+reset.
- Message Log clean across the session: ensure / Accessed None / Fatal / `LogOutputDevice: Error` = **0**.

## ⚠️ WATCH (recorded, non-blocking — the two-tier ruling)

**Jonathan's playtest eye is the FINAL authority on the damage read.** The STAGE-LEGIBILITY band is the shipping gate and it passes twice-measured; his next playtest verdict either closes this chain for good or reopens it as a new task. The numbers to judge against: stage 1 should read "battle-worn but standing", stage 3 "near-dead charred", three glance-distinct states.

## Non-gating note (recorded honestly)

`MI_Castle_Crumble01.RoughBoost` reads 0.35 on disk vs the TASK-157 table's 0.30 — pre-existing (TASK-337 verified it did not touch RoughBoost anywhere), and RoughBoost is explicitly NON-GATING per the retune spec. Recorded only so nobody "discovers" it later.

## Commit scope (this task's commit)

`Content/Materials/M_CastleCrumble.uasset` (TASK-339 ORM sampler `LinearColor → Masks` + master resave — the ALWAYS-commit item) + `MI_Castle_Crumble01.uasset` + `MI_Castle_Crumble02.uasset` (TASK-337 conditional retune — it RAN) + the TASK-337/338/339 handoffs and captures + TASKBOARD status flips. **`MI_Castle_Crumble03` NOT touched, NOT in the commit.** Diff-guard verified nothing foreign.
