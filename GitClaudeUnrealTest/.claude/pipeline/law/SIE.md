<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ SIE OCCUPANCY LAW (2026-08-17, bought by TASK-617) — namespace `SIE-§N`

### SIE-§1 ⛔ READ EARLY OR REGENERATE — A SIMULATE-IN-EDITOR WORLD RUNS THE FULL AI WAR UNATTENDED AND DECAYS

Simulate (`bSimulate=true`) does not pause the game: the AI armies fight from BeginPlay. Observed live 2026-08-17 (TASK-617, seed 690508353): the Blue castle hit crumble 75/50/25 and was **DESTROYED ~T+4min** ("match over, winner: Red"), tearing down its commander and furnishing with it (mechanism M8, by design). Any SIE measurement taken after world state has advanced past what the read assumes is **CONTAMINATED** — TASK-617 discarded and re-took one such read rather than ship it.

**Law:** every future SIE occupancy takes its measurements in the **first ~3 minutes** of a session, or **restarts the session** (regeneration is cheap; a contaminated number is not). Proof-of-freshness instrument: pristine-anchor cross-check — measured support/visual values must match known pristine-mesh anchors to ≤ ~0.5 uu before the batch is trusted. A read that post-dates a castle-destruction log line is discarded, never averaged.

