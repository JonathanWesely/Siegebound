<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
## W1 Economy-balance tasks (decomposed 2026-07-24) — passive gold 1/s + all card costs ×3 (TASK-278..279)

**Directive (verbatim, Jonathan, 2026-07-24):** *"I want to make a couple changes to the spending balancing. Lets bring the passive gold per second from 1 gold every 2 seconds back to 1 gold every second, and then lets triple the cost of all cards."*

**W1-SCOPED, NOT held.** Develops on `m7.6-arena10x` and INTEGRATES into the W1 build (same lane as the TASK-273..277 Shield Wall changes — value edits with no file overlap with the parked M7.7 deck-details chain [DeckBuilderWidget.{h,cpp}] or M7.6's owned L_Arena.umap / DA_BattlefieldScatter / CaptureZone). Two value changes + one MANDATORY bot audit. **Reverts the INCOME half of the 2026-07-08 TASK-089 directive only; StartingGold stays 10 (FLAG 1).** No new asset/class identifiers → NO CONVENTIONS naming additions; the record of truth is the in-code UPROPERTY doc comments (updated by TASK-278) + the balance-ledger note (TASK-279) + this block, mirroring the TASK-089/090 pattern.

**Two changes (exact anchors, orchestrator-verified):**
- **Change 1 — passive gold 1/2s → 1/1s:** `SiegePlayerState.h:282` `BaseIncomeTickPeriod = 2` → **1**. Keep `GoldPerTick=1` (h:278) and `GoldTickInterval=1.0` (h:286) unchanged. Update the h:276 + h:280 doc comments (they cite the TASK-089 "1 gold per 2 s" directive this reverts). PRESERVE (do not change): `OvertimeIncomeMultiplier=2` still doubles base in overtime → 2 gold/s at 7:00 (consistent with "base accrual doubles"). Nuance: `GetGoldRate()` already displayed "+1/s" pre-change (`DivideAndRoundUp(1,2)=1` round-up), so the HUD number does not visibly move — but the ACCRUAL now truly matches it (the display becomes accurate). No literal in `GetGoldRate` — it derives from the properties (verified).
- **Change 2 — all 28 card costs ×3:** `Docs/Data/cards.csv`, `Cost` column ONLY (4th field, between `CardType` and `MaxCopies`). Exact new values: Footman **9** · Archer **12** · Knight **18** · Miner **24** · ArrowTower **15** · Wall **12** · MilitiaMob **15** · Pikeman **15** · Sapper **15** · Cavalry **21** · Longbowman **18** · Cleric **18** · Ogre **36** · BombTower **24** · BallistaTower **21** · Barracks **30** · DeepMine **45** · Masons **24** · SharpenedBlade **18** · PlateArmor **18** · SwiftBoots **15** · WarBanner **24** · Fireball **21** · FrostNova **18** · Lightning **24** · BattleCry **15** · Pickpocket **18** · CrystalTower **27**. Change ONLY `Cost` — `MaxCopies`/`HP`/`Damage`/`DeckCount`/`GoldSteal`/all other columns UNTOUCHED.

**Bot audit (MANDATORY — the thing that silently breaks):** the bot reads `Row->Cost` LIVE for all affordability (auto-scales), BUT `ASiegeBotController::AttackBankThreshold` (`SiegeBotController.h:230`) is a HARDCODED gold literal = **12** — the Rule-4 gate the bot banks to before committing an offensive unit ("this is what makes waves GROW as income scales"). 12 = the OLD Ogre cost (its priciest bankable unit). At ×3 the cheapest units are 9–21, so a 12-gold gate makes the bot dump gold on the cheapest affordable unit (Footman 9 / Archer 12) the instant it hits 12 and NEVER bank toward Knight 18 / Cavalry 21 / Ogre 36 — the wave-growth property BREAKS. FIX = scale `AttackBankThreshold` 12 → **36** (×3 = new Ogre cost; preserves the intent) + correct the stale `SiegeBotController.cpp:56` comment ("an Ogre needs 12 gold" → 36). LEAVE `BotDiscardCost=1` (h:234 — mirrors the §3.6 fixed 1-gold swap fee, NOT a card cost) and `TargetMinerCount=3` (h:226 — a count). TASK-278 must SWEEP the whole SiegeBotController.{h,cpp} for any OTHER hardcoded gold/cost literal in a spend/bank/affordability heuristic (scale-or-data-drive it), PRESERVE the M3 ordered-rules `LogSiegeBot` decision trace verbatim, and REPORT everything found.

**Auto-updating consumers (confirmed — NO action):** `GetGoldRate()` derives from the properties (Change-1 HUD auto-updates); the §8 deck-builder average-cost guide + the parked TASK-268 `GetCardDescription` generator read `Cost` live → both reflect the ×3 for free.

**FLAGS for Jonathan (implement exactly the two changes; these are for his playtest eyes, NOT blockers):**
1. **StartingGold=10 unchanged (DEFAULT — do NOT change; surface only):** cheapest unit is now Footman 9, so StartingGold 10 buys exactly ONE Footman at match start (was 3). Combined with passive 1/s this reshapes the opening — Jonathan may want to revisit at playtest.
2. **Bot AttackBankThreshold 12→36 (implemented to preserve intent):** the bot's bank gate moved WITH costs; watch that waves still grow (bot fields Knight/Ogre) and don't stall.
3. **Net economy shift:** passive income 2× (1/2s→1/s) but costs 3× → at passive-only rate a card takes **~1.5× longer** to afford than before; income investments (Miner +1/s costs 24, DeepMine +2/s costs 45) and the fixed-value Pickpocket steal (`GoldSteal=10`, UNCHANGED — not a card cost) are RELATIVELY weaker / slower-return vs the higher unit costs. Reads deliberate ("spending balancing"); surfaced for confirmation. The §3.6 fixed 1-gold card-swap fee and mine yields (Miner +1/s, DeepMine +2/s) are NOT card costs — unchanged.

**Dispatch shape:** TASK-278 (gameplay-programmer, file-only, dispatchable NOW) → QA (status-flow gate; shadow-scan + complete-type-include scan; verify the 28 Cost cells + AttackBankThreshold=36 against this block) → TASK-279 (build-master: compile + DT_Cards CSV-sync + PIE economy verify + branch commit NO push + balance-ledger note).

#### TASK-278 — Economy balance: base income 1 gold/s + all 28 card costs ×3 + bot bank-threshold audit (C++/CSV)
- assignee: gameplay-programmer
- status: done (INTEGRATED at TASK-279, build-master 2026-07-24, commit on m7.6-arena10x; compile GREEN, DT_Cards CSV-synced + saved, PIE economy verified. --- Prior QA 2026-07-24 — PASS, 0 blockers / 2 comment-hygiene NITs, report `qa/TASK-278.md`. INDEPENDENTLY verified: `BaseIncomeTickPeriod` 2→1 = exactly 1 gold/s (×2 overtime), all other gold props unchanged, no accrual-logic edit; all 28 cards.csv costs recomputed ×3 and matched, Cost column ONLY (28 data rows, header + MaxCopies/GoldSteal/etc. byte-identical); `AttackBankThreshold` 12→36 = new Ogre cost. Independent module sweep AGREES `AttackBankThreshold` was the ONLY stale hardcoded gold literal — all affordability reads are data-driven `Row->Cost`; `BotDiscardCost=1`/counts/time literals correctly left. 5 doc-comment edits comment-only. LogSiegeBot trace + TASK-265 spawn-clamp untouched. CLEAR for build-master TASK-279.)
- blocked-by: none — **dispatchable NOW** (file-only; no compile, no reimport, no Git)
- parallel-safe: yes (edits `SiegePlayerState.h` + `Docs/Data/cards.csv` + `SiegeBotController.{h,cpp}`; disjoint from the parked M7.7 `DeckBuilderWidget.{h,cpp}` and from `L_Arena.umap` / `DA_BattlefieldScatter` / `CaptureZone`)
- spec: >
    On `m7.6-arena10x`. FILE-ONLY — NO compile, NO DT_Cards reimport, NO Git (TASK-279 owns all of those).
    (1) **Change 1 — base income:** in `SiegePlayerState.h`, set `BaseIncomeTickPeriod` (line ~282) from `2` to **`1`**.
    Keep `GoldPerTick=1` and `GoldTickInterval=1.0f` UNCHANGED. Reword the doc comments at ~h:276 and ~h:280 to state
    "1 gold per 1 s (2026-07-24 balance directive — reverts the TASK-089 1-per-2s income change; was 2)"; keep the note that
    `OvertimeIncomeMultiplier` still doubles the base in overtime (→ 2/s at 7:00). Do NOT touch `StartingGold` (stays 10),
    `MaxGold`, miner/flat income, or `GetGoldRate()` logic.
    (2) **Change 2 — card costs ×3:** in `Docs/Data/cards.csv`, edit ONLY the `Cost` column (4th field, between `CardType`
    and `MaxCopies`) for all 28 rows to these EXACT values: Footman 9, Archer 12, Knight 18, Miner 24, ArrowTower 15,
    Wall 12, MilitiaMob 15, Pikeman 15, Sapper 15, Cavalry 21, Longbowman 18, Cleric 18, Ogre 36, BombTower 24,
    BallistaTower 21, Barracks 30, DeepMine 45, Masons 24, SharpenedBlade 18, PlateArmor 18, SwiftBoots 15, WarBanner 24,
    Fireball 21, FrostNova 18, Lightning 24, BattleCry 15, Pickpocket 18, CrystalTower 27. Change NOTHING else on any row
    (MaxCopies/HP/Damage/DeckCount/GoldSteal/etc. stay).
    (3) **Bot audit (MANDATORY):** in `SiegeBotController.h`, scale `AttackBankThreshold` `12` → **`36`** (comment: ×3 with
    the 2026-07-24 cost triple = new Ogre cost; preserves the wave-growth bank gate). Correct the stale `SiegeBotController.cpp`
    ~line 56 comment ("an Ogre needs 12 gold" → 36). LEAVE `BotDiscardCost=1` (fixed §3.6 swap fee, not a card cost) and
    `TargetMinerCount=3` (a count). Then SWEEP the ENTIRE `SiegeBotController.{h,cpp}` for ANY other hardcoded gold/cost
    numeric literal in a spend/bank/affordability heuristic; anything that assumed the OLD costs → scale ×3 or data-drive from
    `Row->Cost`, with a comment. All existing `Row->Cost` affordability reads stay as-is (they auto-scale). PRESERVE the M3
    ordered-rules `LogSiegeBot` decision trace verbatim (rule numbers + format). `handoffs/TASK-278.md` MUST list exactly what
    the sweep found + every literal changed (or "none beyond AttackBankThreshold").
    QA implied — shadow scan (no shadowing inherited reflected members), complete-type-include scan, confirm no other file
    hardcodes a cost/rate. NOT IN SCOPE: compiling, DT_Cards reimport, Git, `StartingGold`. Post completion in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h` (`BaseIncomeTickPeriod` 2→1),
    `Docs/Data/cards.csv` (`Cost` column ONLY, 28 rows, values above),
    `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.{h,cpp}` (`AttackBankThreshold` 12→36 + comment).
    Consumed as-is, do NOT alter: `GoldPerTick`, `GoldTickInterval`, `StartingGold`, `MaxGold`, `GetGoldRate`,
    `OvertimeIncomeMultiplier`, `BotDiscardCost`, `TargetMinerCount`, every `Row->Cost` read. No new identifiers → no
    CONVENTIONS naming change.

#### TASK-279 — Integration: compile + DT_Cards CSV-sync + PIE economy verify + branch commit + balance ledger (build)
- assignee: build-master
- status: done (BUILD-MASTER 2026-07-24 — economy balance integrated on m7.6-arena10x, commit `3c32e25` (no push). Step-0 PIE check idle (Jonathan NOT mid-playtest); editor closed clean + recompiled GREEN (`Result: Succeeded` ~16s). DT_Cards CSV-synced via `set_rows` (all 28 `Cost` cells) + saved (`is_dirty=false`) — readback Footman 9 / Ogre 36 / DeepMine 45 / CrystalTower 27, no other column drifted. PIE economy verify: (a) base income 1/s — CDO `BaseIncomeTickPeriod=1` + LogSiegeBot gold 10→36 in ~26 s / 18→36 in ~18 s; (b) tripled costs LIVE — DT readback + LogSiegeBot "Knight cost 18"; (c) bot banks to 36 → Knight ×2 (`AttackBankThreshold=36` CDO + trace). Clean PIE log (no errors/ensures/Accessed-None). Committed pathspecs: `SiegePlayerState.h`, `SiegeBotController.{h,cpp}`, `Docs/Data/cards.csv`, `Content/Data/DT_Cards.uasset` (LFS pointer verified) + board balance-ledger. DeckBuilderWidget (TASK-268) stayed parked/unstaged. No push. Feel = Jonathan W1 WATCH. Was: backlog.)
- blocked-by: TASK-278 (qa-passed)
- parallel-safe: no (single editor + compiler + Git)
- spec: >
    On `m7.6-arena10x`. (1) Compile TASK-278's C++ (editor bounce as usual — Jonathan's close/reopen grant covers this
    session). GREEN, report time + `Result: Succeeded`. Failure → append errors to `qa/TASK-278.md`, route back to
    gameplay-programmer (counts as a QA loop). (2) **DT_Cards CSV-sync:** re-sync `/Game/Data/DT_Cards` from
    `Docs/Data/cards.csv` via the reference-safe `set_rows`/reimport path (handoffs/TASK-031 — preserve the GUID + import-source
    linkage; do NOT recreate the asset). Verify the 28 rows' `Cost` match the CSV (spot-read Footman 9, Ogre 36, DeepMine 45,
    CrystalTower 27) and that NO other column drifted. (3) **VERIFY branch-owned files untouched:** `git diff --stat` should
    show ONLY `SiegePlayerState.h`, `Docs/Data/cards.csv`, `SiegeBotController.{h,cpp}`, `DT_Cards.uasset` (+ CONVENTIONS/board);
    if `L_Arena.umap` / `DA_BattlefieldScatter` / `SiegePlayerController` / `CaptureZone` / `DeckBuilderWidget` changed
    unexpectedly, STOP and report. (4) **PIE economy verify (machine-observable):** (a) base gold accrues 1/s — readback the
    Blue player's gold over ~5 s at match start, no miners → +5; (b) HUD "+N/s" reads the rate (still +1/s, now truthful);
    (c) card costs are tripled in-match (a play deducts the ×3 cost; cheapest = Footman 9); (d) bot — run a bot match, grep
    `LogSiegeBot`, confirm Rule 4 banks to 36 and eventually fields expensive units (Knight 18 / Cavalry 21 / Ogre 36) rather
    than perpetual Footman/Archer spam. (5) **BALANCE LEDGER note** (ties to the TASK-090 ledger + the Standing-backlog "Balance
    pass" item) in `handoffs/TASK-279.md`: the new economy math (passive 2× / costs 3× → ~1.5× longer to afford at passive
    rate), a best-effort re-measure of the undefended-Blue-castle kill time vs the bot (prior marks: 56.5 s / 71.3 s @ old
    economy, ~33 s pre-economy-change — the bot rushes autonomously, so this is machine-observable without player input), and
    whether the bot fielded heavy units under the new threshold. (6) **COMMIT on the branch** referencing TASK-278/279 + the
    directive: `SiegePlayerState.h`, `cards.csv`, `SiegeBotController.{h,cpp}`, `DT_Cards.uasset`, CONVENTIONS/board.
    **DO NOT PUSH.** Record the human WATCH (opening-economy feel — StartingGold 10 buys one Footman; bot wave-growth) for
    Jonathan's W1 look. Leave the editor running + saved. Post results + hash in 🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x` commit (NO push); `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h`,
    `Docs/Data/cards.csv`, `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.{h,cpp}`, `/Game/Data/DT_Cards`.
    Law: CLAUDE.md hard gates (PASS QA before commit, never push unasked), GDD §3.0 (cards.csv is the DT_Cards source of
    truth), M7.6 branch-ownership.

---

