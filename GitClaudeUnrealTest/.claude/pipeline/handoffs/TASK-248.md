# TASK-248 — Spell-lane follow-ups: SpellDelivery header cell + stale-comment sweep — programmer handoff

Status: ready-for-qa (QA-LIGHT per spec — orchestrator-proxied diff review) · 2026-07-22 · gameplay-programmer
Law: CONVENTIONS "Spell delivery overhaul" → SpellDelivery column entry (2026-07-22) + "Data-driven card stats".
Closes: qa/TASK-236-237-qa.md WARN-2 (CSV column gap) + the row-6 stale-comment adjudication (ex-TASK-240 checklist item).
Working tree: m7.6-arena10x checkout; ALL edited files verified byte-identical between main tip and branch tip BEFORE editing (true main-lane files — `git diff main m7.6-arena10x` showed only BattlefieldScatter.{h,cpp}, ScatterConfig.h, SiegeBotController.h diverging). NOT committed (per dispatch) — the commit routes via the next MAIN docs/integration window.

## 1. cards.csv — SpellDelivery column appended (WARN-2 closure)

`Docs/Data/cards.csv`: appended `,SpellDelivery` to the header (column 31) and a trailing cell to all 28 data rows:

- `HeroLine` — Fireball (row 24), FrostNova (row 25) — the two directive cards, explicit per the registry.
- EMPTY (unset) — all 26 other rows, per the CONVENTIONS entry exactly ("unset/illegal cell ⇒ the legacy reticle ground-circle path"). No default token is documented by the registry, so no token was invented.

**Exact value set (recorded per spec):** `Auto` / `GroundCircle` / `HeroLine` — the `ESpellDelivery` UENUM value names (CardRow.h:79-84). Registry precision note: CONVENTIONS says the strings are "pinned by SpellLibrary.h" — the string SET is literally the UENUM in **CardRow.h**; SpellLibrary.h pins the SEMANTICS (`GetEffectiveDelivery` / `IsLineDeliverySpell`, SpellLibrary.h:124-136). Not a conflict; recorded for accuracy.

**Import-contract verification (spec item 1 check):**
- Header cell `SpellDelivery` matches the FCardRow UPROPERTY name 1:1 (`ESpellDelivery SpellDelivery = ESpellDelivery::Auto;`, CardRow.h:218) — the CONVENTIONS "CSV header must match UPROPERTY names 1:1" law is now satisfied for this column (previously C++-only).
- The DataTable CSV importer parses enum cells by UENUM value name — `HeroLine` cells bind directly; EMPTY cells leave the C++ default `Auto` (at worst a benign per-row parse note in the reimport log — same class as the missing-column notice TASK-236 pre-warned; build-master must not treat either as a failure).
- **DT reimport is NOT done here** — DT_Cards reimports at the next integration window. Until then (and even on a botched cell import), the runtime `Auto` inference remains the safety net per TASK-236's design: AoEDamage/Freeze → HeroLine (exactly Fireball + FrostNova today), everything else → GroundCircle. Behavior is identical before/after reimport; the explicit cells exist as the data override lever going forward.
- CSV hygiene: file stays LF-EOL + trailing newline (matched the index `i/lf w/lf` state); uniform 31 cells per row (empty trailing field), so no short-row import problems.
- The Lightning **Notes** cell still reads "in 400" — DELIBERATELY untouched: QA's TASK-237 review ruled it frozen/display-only (no code reads Notes). Not part of this sweep.

## 2. Stale "400" comment sweep — 3 of 4 lines edited (comment-only, zero logic bytes)

| # | Site (QA row-6 list) | Found at | Edit |
|---|---|---|---|
| 1 | SpellLibrary.cpp "in a 400 radius" (~193 in the QA cite) | SpellLibrary.cpp:331 | `enemies in a 400 radius` → `enemies in a 700 radius` |
| 2 | SiegeBotController.cpp rule-3b doc (~620) | SiegeBotController.cpp:628 | `AoERadius (400 — GDD §4; data-driven law)` → `(700 — …)` |
| 3 | SiegePlayerController.cpp reticle doc (~1731/1796) | SiegePlayerController.cpp:1796 | `Lightning 400 / BattleCry 400` → `Lightning 700 / BattleCry 400` (BattleCry's 400 is still CORRECT — only the Lightning number was stale) |
| 4 | SiegeBotController.h:247 | present, both tips | **SKIPPED — lane ruling below** |

Exact diffs (acceptance evidence — the full `git diff` on Source/ + Docs/Data is ONLY these three 1-line hunks + the cards.csv tail-append; zero logic bytes):

```
SpellLibrary.cpp:331        -	 *  enemies in a 400 radius; "the tower-killer"). Ruling 4 selection: the
                            +	 *  enemies in a 700 radius; "the tower-killer"). Ruling 4 selection: the
SiegeBotController.cpp:628  -		//     within the Lightning ROW's own AoERadius (400 — GDD §4; data-driven law):
                            +		//     within the Lightning ROW's own AoERadius (700 — GDD §4; data-driven law):
SiegePlayerController.cpp:1796 -		// Fireball 300 / FrostNova 350 / Lightning 400 / BattleCry 400) so the
                               +		// Fireball 300 / FrostNova 350 / Lightning 700 / BattleCry 400) so the
```

## 3. SiegeBotController.h ruling (the dispatch's explicit question): SKIPPED — cross-lane

- Verified: the stale line (`…(Fireball 300, Lightning 400 — GDD §4 Set III), read`, line 247 branch / ~246 main) is byte-IDENTICAL on both tips.
- But the FILE is branch-owned: tip-vs-tip it diverges in 4 hunks (BotCastleSpawnOffset replacing BotCenterlineSpawnX, swarm-fan doc, ±25k/±24.2k fallback locations — all M7.6 arena work).
- Editing it in this (branch) working tree therefore CANNOT ride the main docs window (committing the file to main from here would drag arena changes onto main before the merge gate) and would instead put a spell-lane edit into the arena lane. Per dispatch: not worth a one-line comment — **SKIPPED + recorded. Reconcile at the M7.6 merge gate** (or the first post-merge wave touching SiegeBotController.h on main): same 400→700 number-swap on that one line.

## 4. Flagged residue for the manager (recorded, NOT edited — spec confinement to the 4 QA-adjudicated lines + header)

1. `CardRow.h:203` — "(… Lightning 400, BattleCry 400)" in the AoERadius column doc: a 5th stale-"400" site the QA list did not include (main-lane, tips identical). Same one-number swap whenever a wave owns CardRow.h.
2. `CardRow.h:69-76` (ESpellDelivery enum doc "CSV note") + `CardRow.h:210-215` (SpellDelivery UPROPERTY doc) + `SpellLibrary.h:125` ("cards.csv carries no cells this wave"): these say the header is NOT yet appended — TRUE until this task's csv lands, FALSE after. Comment-only refresh belongs to the next wave owning those headers (kept out of this diff so the review surface stays exactly the spec's 4-line + header contract).

## QA / integration notes

- No compile needed for correctness of this change-set (comments + CSV); the CSV takes effect only at the next DT_Cards reimport (integration window). Expect at most benign enum-cell parse notes for the 26 empty cells.
- Git-diff confinement check: `git status` also shows pre-existing modifications to TASKBOARD.md/CONVENTIONS.md (manager's TASK-248 registry entry + board block — not mine, except the TASK-248 status flip) and untracked handoffs/TASK-246.md (not mine).
