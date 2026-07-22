# QA Report — TASK-248 (SpellDelivery CSV column + stale-comment sweep) — QA-LIGHT

Verdict: **PASS**

Reviewer: qa-reviewer, 2026-07-22. QA-light per the manager pre-ruling (zero logic bytes; diff
confinement = acceptance). Verified by file-state inspection against handoffs/TASK-248.md, the
board spec, and the CONVENTIONS SpellDelivery registry entry (:207). Board untouched.

Blockers: 0 · Warnings: 1 (registry TEXT, manager-owned — not this change-set) · Nits: 0

## (1) Diff exactly as claimed — CONFIRMED (file-state inspection; git proper is the proxied half)

- `Docs/Data/cards.csv`: header col 31 = `SpellDelivery`; all 28 data rows carry the trailing
  cell — `HeroLine` on EXACTLY Fireball (:24) + FrostNova (:25), EMPTY on the other 26; uniform
  31 cells; trailing newline; Lightning Notes "in 400" deliberately untouched (the TASK-237
  frozen ruling — correct).
- The three comment swaps verified in place, number-only: SpellLibrary.cpp:331 → "700 radius"
  (BattleCry's 400 at :448 correctly untouched); SiegeBotController.cpp:628 → "(700 — GDD §4…)";
  SiegePlayerController.cpp:1796 → "Lightning 700 / BattleCry 400" (BattleCry correctly kept).
- Zero logic bytes anywhere in the four files' claimed deltas.

## (2) CSV semantics vs registry + CardRow.h:218 — SOUND, with ONE registry-text finding

- Header matches the UPROPERTY name 1:1 (`ESpellDelivery SpellDelivery = ESpellDelivery::Auto`,
  CardRow.h:218) — WARN-2 of qa/TASK-236-237-qa.md is CLOSED.
- Empty cell ⇒ C++ default `Auto` ⇒ per-effect inference — behavior identical before/after the
  DT_Cards reimport, and even a botched enum-cell parse degrades to `Auto` safely. HeroLine
  explicit on the two directive cards matches (and is redundant with) the Auto inference —
  correct belt-and-braces. The handoff's value-set precision note (strings pinned by the
  CardRow.h UENUM, semantics by SpellLibrary.h) is accurate.
- **[WARN — registry text, manager one-liner]** CONVENTIONS :207 says "unset/illegal cell ⇒ the
  legacy reticle ground-circle path". The CODE fallback is `Auto` ⇒ PER-EFFECT: AoEDamage/Freeze
  → **HeroLine**, others → GroundCircle. Zero divergence today (no unset AoEDamage/Freeze row
  exists; Fireball/FrostNova are explicit anyway), but the sentence promises the wrong fallback
  for a future unset/illegal AoEDamage/Freeze card — the same trap window WARN-2 described, now
  at the registry level. Suggested fix (manager-owned text, NOT a TASK-248 defect): "unset/illegal
  ⇒ Auto (per-effect: AoEDamage/Freeze → HeroLine, others → ground circle; never a crash)".

## (3) SiegeBotController.h SKIP — CORRECT lane call, divergence CONFIRMED

:247 still reads "Lightning 400" (untouched as ruled). The tip-vs-tip divergence is confirmed
from this QA's own TASK-216 review: the M7.6 edits (BotCastleSpawnOffset replacing
BotCenterlineSpawnX, ±25000/±24200 fallbacks, swarm doc) live in this file in the shared tree and
are branch-lane work — committing it from here to a main docs window would drag arena changes
across the merge gate. Deferring the one-number swap to the M7.6 merge-gate reconcile is right;
recorded destination is clear.

## (4) Leftovers accurately flagged — CONFIRMED on disk

- CardRow.h:203 "Lightning 400, BattleCry 400" — present; a genuine 5th stale site OUTSIDE the
  QA row-6 list; correctly recorded, correctly excluded from this diff.
- CardRow.h:71 ("header is NOT yet appended") + SpellLibrary.h:125 ("cards.csv carries no cells
  this wave") — both present and now stale-by-this-task; correctly flagged to the next wave
  owning those headers. Keeping them out preserves the exact review-surface contract.

## Notes for integration

Commit routes via the next MAIN docs/integration window per the handoff (not committed here);
DT_Cards reimport may log benign enum-cell notes for the 26 empty cells + nothing else — not a
failure. Merge-gate checklist item: SiegeBotController.h:247 400→700.
