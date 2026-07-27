# TASK-332 — [GOLD-glow] GoldNode emissive dialled back on `M_GoldGlow` (art-director)

**Status: ready-for-integration** (hand off to TASK-333). Date: 2026-07-27.
Saved: **ONLY `/Game/Materials/M_GoldGlow`** (`Content/Materials/M_GoldGlow.uasset`). No Git commit (TASK-333 owns it).

## The lever (exactly one, per manager ruling 9 / CONVENTIONS "the emissive brightness lever")

| What | Node | Old | New |
|---|---|---|---|
| Base emissive strength | `MaterialExpressionConstant_0.R` (the Constant on `Multiply_0.B`, i.e. the value the whole chain — and therefore the `GlowIntensity` param — multiplies) | **6.0** | **0.45** |

Nothing else in the material was touched: no node added/deleted/rewired, no parameter renamed, no default changed,
no texture touched (`T_GoldNode_*` remain orphaned, untouched). The trap was avoided: the `GlowIntensity`
ScalarParameter default is still 1.0 and the C++ MID contract is byte-identical — `GlowIntensityFull = 1.0` still
means "the authored look", which is now simply 13.3x dimmer at the source.

## Acceptance numbers (measured the audit's way — `CaptureAssetImage` on `SM_GoldNode`, 256x256)

**Method validation first (TASK-329 discipline):** mask = per-row background model from the L/R edge strips,
subject = |ΔRGB| > 36 (~29% coverage = the mesh silhouette). On the BEFORE capture this mask reproduces the
TASK-309 audit to 0.3 points — **86.0% measured vs 86.3% recorded** — and puts the CrystalTower control at
9.1–9.9% vs the audit's 6.5% (same single-digit band). Numbers below are therefore audit-comparable.

| Measure | BEFORE (6.0) | AFTER (0.45) | Target |
|---|---|---|---|
| Blown past luma 0.85, **mean-RGB luma** | **86.0%** | **6.6%** | ≤15%, band 5–10% ✅ (CrystalTower ref = 6.5%) |
| Blown past luma 0.85, Rec.709 luma | 86.0% | 49.5% raw | see floor note |
| Rec.709 **glow-attributable** (raw − basecolor floor 37.5%) | ~48.5 pts | **12.0 pts** | ≤15 ✅ |

**The floor note (important for anyone re-measuring):** the audit never recorded its luma formula; both mean-RGB
and Rec.709 reproduce the 86.3% BEFORE anchor identically, so the ambiguity is real and only diverges after the fix.
A control capture with the emissive at 0.05 (glow effectively OFF) still measures **37.5% blown under Rec.709** —
that residual is the **LIT gold BaseColor** under the thumbnail key light (Rec.709's 0.715 green weight rates any lit
warm-yellow surface ~blown), not glow. Rec.709-raw therefore cannot measure glow blow-out on this asset; mean-RGB
(6.6%) and Rec.709-minus-floor (12.0) are the meaningful readings, and both are inside the band.

Determinism: thumbnail captures render at a fixed Time — three spaced captures at one value were bit-identical, so
the TASK-226 sine pulse does not add noise to these numbers (and did not to the audit's).

## Before/after captures

- `handoffs/TASK-332-GoldNode-before.png` — the featureless cream blob (86.0% blown)
- `handoffs/TASK-332-GoldNode-after.png` — crystal facets, chunk boundaries, dark crevices and base rubble all
  legible; still reads self-lit warm yellow, not a dull rock

Iteration ladder (captured, for the record): 6.0→86.0% · 2.0→(mean)49.0% · 1.0→26.6% · 0.6→11.7% ·
**0.5→8.7% · 0.45→6.6%** · 0.4→5.9% · 0.05 floor→1.7%. 0.45 was chosen over 0.4 to stay near the top of the
band — the failure mode on the far side is a mine nobody notices, and every step down also starves the depleted ember.

## Regression set — verified by post-save readback (all PASS)

- (a) `GlowIntensity` ScalarParameter: **exact name preserved**, DefaultValue 1.0, still on `Multiply_3.B` → the
  `AGoldNode::UpdateGlowGauge()` MID write still lands. Zero C++ change.
- (b) TASK-226 pulse intact: `Time_0 → Sine_0 (Period 10 s = 0.1 Hz) → Multiply_1 (x0.12) → Add_0 (+1.0)` → ±12%.
- (c) TASK-296c keep-both superset intact: `EmissiveColor ← Multiply_3 (A: Multiply_2 [base x pulse], B: GlowIntensity)`.
- (d) Stock nodes only — the ONLY edit was one float on an existing Constant; expression count 10 before and after,
  same set. No Custom HLSL anywhere near it; recompiles were seconds each.
- (e) Team contract: material has no team logic — gold nodes glow regardless of team, modulation semantics unchanged.
- BaseColor untouched: still `Constant3Vector_0` (1.0 / 0.66 / 0.12) driving both BaseColor and the emissive tint.

## Notes for TASK-333 (the two-point gauge verify)

1. **Full reserve (MID writes 1.0):** expect the after-capture look under the arena sun. If re-measuring, use the
   floor note above — quote mean-RGB or Rec.709-minus-floor, not Rec.709-raw, or the number will look like a fail
   that isn't one.
2. **Depleted (MID writes 0.05): WATCH THIS — it is the one real risk of the dial-back.** The ember floor scaled
   proportionally (spec accepted this): effective depleted emissive is now 0.45 x 0.05 = **0.0225** vs the old 0.30.
   Under the daylit arena that may read as no ember at all. If (b) fails, the sanctioned remedy is NOT the material —
   it is raising `AGoldNode::GlowIntensityDepleted` (0.05 → ~0.2–0.3) in C++, a one-line gameplay-programmer change
   that restores the depleted ember to roughly its old absolute energy while keeping the full-reserve fix. Flag it
   back to the manager; do not re-touch `M_GoldGlow` for it.
3. Continuity check (c) is pure code-path proof and is unaffected by this change.
4. Commit scope for TASK-333: `Content/Materials/M_GoldGlow.uasset` + this handoff + the two PNGs. Pre-existing
   board/CONVENTIONS dirt (+97/+1 lines, the manager's TASK-330..333 decomposition) was already in the tree before
   this task ran — not mine, manager/build-master call whether it rides along.

## Compliance

Simulate/PIE confirmed NOT running before any edit (readback false) and never started. `L_Arena` untouched — no
level operation of any kind, no save prompt appeared. No C++, no Blueprint, no texture, no Git write, no
TASKBOARD.md edit, nothing published externally. Captures/analysis ran through the local MCP HTTP endpoint;
scratch scripts stayed in the session scratchpad.
