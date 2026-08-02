# QA Report — TASK-419 (LLM-ASSISTANT game-lane Wave 0)

**Verdict: PASS** — 0 BLOCKER, 5 WARN (all carry-forward, none gating), 10 NIT.
**Loop 2 of 3** (re-run scoped to TASK-417's fix diff). **Date:** 2026-08-02 · **Reviewer:** qa-reviewer · **Mode:** pre-compile source review, no engine, no Git.

**TASK-420 is UNBLOCKED.**

## Per-task verdict

| Task | Verdict | Change since loop 1 |
|---|---|---|
| **TASK-416** — `USiegeAssistantSnapshot` | **PASS** | comment-only edit by TASK-417 in its header — **ruled ACCEPTABLE**, see the cross-owner ruling below. `.cpp` byte-untouched. |
| **TASK-417** — grammar + command + parser + vocabulary + tests | **PASS** | BLOCKER-1 and WARN-1 **closed**; WARN-5 closed with the arithmetic attached; two additive test guards. |
| **TASK-418** — `AAncientGround::FindNearestAncientGround` | **PASS** | unchanged. |
| **TASK-426** — the sealed corpus | **PASS** | unchanged, and **not edited to accommodate the fix** — verified. |

---

## Loop-1 blocker: CLOSED

**BLOCKER-1 — `militia_mob` → `militiamob`.** Verified at `SiegeAssistantVocabulary.cpp:126`. The old spelling survives **only in explanatory comments** (`:93`, `:103`, and `SiegeAssistantGrammarTest.cpp:945`) — never as a canonical. Grep across the module for `militia_mob` returns those three comment hits and nothing else.

**Option 2 correctly declined.** TASK-417 did not add a CardID→symbol exception to `CanonicalKind`. That was the right call and I would have pushed back if it had: `CanonicalKind` is currently a **pure total derivation** (`FName(*CardID.ToString().ToLower())`, `SiegeAssistantSnapshot.cpp:99-106`, verified unchanged), and trading that for a lookup table to satisfy a cosmetic preference is a real architectural cost that needs a ruling nobody had given. Declining to assume a ruling is the behaviour the batch's laws are trying to produce.

### The root cause it identified — VERIFIED, and it is correct and complete

I re-derived all three claims in the comment at `SiegeAssistantVocabulary.cpp:92-112` rather than accepting them:

- **"A unit symbol is the CardID lower-cased with no separator"** ✓ — matches `CanonicalKind` exactly.
- **"Place symbols ARE hand-authored with underscores, and that asymmetry is the trap"** ✓ — `SiegeAssistantSnapshot.cpp:52-61`'s `PlaceVocabulary` is a hand-written table of `enemy_castle` / `ancient_ground_near` / `nearest_mine`. Underscores are genuinely correct three lines below and genuinely wrong above. This explains why the defect read as reasonable, which is the part a root cause has to do.
- **"`MilitiaMob` is the only multi-word `CardType == Unit` row today"** ✓ — **independently confirmed against `cards.csv`.** The 12 Unit rows are `Footman`, `Archer`, `Knight`, `MilitiaMob`, `Pikeman`, `Sapper`, `Cavalry`, `Longbowman`, `Cleric`, `Ogre`, `Wizard`, `Sorcerer`; only `MilitiaMob` is multi-word. The forward-looking `ShieldMaiden -> shieldmaiden` example applies the rule correctly.

This is a real root cause, not a plausible-sounding one. It names the mechanism, the reason it survived review, and the condition under which it recurs.

### ⚠️ The `miner` claim — INDEPENDENTLY VERIFIED, and it is CORRECT, not a second defect

You were right to ask me to check this rather than accept it; it has exactly the shape of a second bug behind a good explanation. It is not one, and here is the evidence rather than the conclusion:

- **`cards.csv:5` is `Miner,Miner,Economy`** — the `CardType` really is `Economy`, not `Unit`. The premise of the claim is true.
- **The symbol is clean.** `Miner` → `miner`: single token, no separator, so `CanonicalKind` produces exactly what the vocabulary spells. It is structurally *not* the BLOCKER-1 class, which is specifically about multi-word CardIDs.
- **Including it is REQUIRED, not merely harmless.** `AMinerUnit` derives from `ASummonedUnit`, so the roster pass (`TActorIterator<ASummonedUnit>`, `SiegeAssistantSnapshot.cpp:373`) enumerates live miners, gives them a roster row, and feeds `miner` into the grammar's `kind` alternatives. TASK-416 records the miner as both follow- and group-eligible via its overrides. And the sealed corpus names them: **`DEV-08` — "everyone fall back except the miners"**. Omitting `miner` would have been the defect.
- **I also ran the check that was NOT asked for, which is the one that mattered: the REVERSE set-difference.** All **12** `CardType == Unit` rows are present in `UnitSynonyms`; the 13 canonicals are those 12 plus `miner`. **Nothing is missing.** No other Economy/Utility row spawns a commandable `ASummonedUnit` (`DeepMine` is a building actor, `Masons` is `Utility` and repairs), so there is no third case hiding behind the same reasoning.

**Conclusion: the set-difference is one-directional, deliberate, and correct.** Claim verified.

---

## Loop-1 WARNs: two closed

**WARN-1 — CLOSED.** `SiegeAssistantVocabulary.cpp:142` now reads `wizard <- { "wizards" }`; `"fire mage"` is gone and survives only in the comment at `:132-141`. I enumerated **every alias in all three sections** (13 unit rows, 7 place rows, 7 intent rows): **no alias anywhere contains `mage`, `caster` or `spellcaster`.** The three words appear in exactly one piece of prompt content — the `[notes]` line at `:200`, on the *prohibition* side, routing them to `which_unit`.

**DEV-06 and HOLD-09 are protected**, and I checked the mechanism rather than the intent: both rows use the token *"the mage"*; no alias in the shipped table pulls toward either caster card; the notes line explicitly declares `mage` ambiguous. The holdout row that scores bar #5 is clean.

**WARN-5 — CLOSED, and better than the finding asked for.** All three sites corrected **with the arithmetic attached**, so the number cannot be re-derived wrongly:
- `SiegeAssistantSnapshot.h:93` — zone table now `ZONE A ~600 tok`.
- `SiegeAssistantSnapshot.h:98-108` — carries `165/765 = 78% drop, bar PASSES` versus `165/515 = 68%, MARGINALLY FAILS` at the nominal 350, plus the bar-#2 warm-prefix reasoning and the `~4250 chars / ~1180 tok` figure once the vocabulary asset loads.
- `SiegeAssistantSnapshot.h:164-168` — `MaxSnapshotChars`'s comment now says in terms *"Do not read this constant as licence to shrink Zone A"*.
- `SiegeAssistantVocabulary.h:93-99` — the third site, reframed as *"dropping empty rows is a tidiness rule, not a budget rule"*, carrying the same 78%/68%.

"~350" now appears only ever as the thing **not** to trim toward. That closes the trap properly.

## WARNs carried forward — none gates TASK-420

- **[WARN-2 · open, action only]** Zone A's documented size is now correct in two headers (`~1180 tok` with the asset). The **action** remains: **TASK-413 must measure Zone A with the real asset loaded and a real tokenizer**, and report A / B+C / output against `ContextTokens 2048`. The headers now say so explicitly (`SiegeAssistantSnapshot.h:108`). ⚠️ Still not a licence to trim.
- **[WARN-3 · open · FOR BUILD-MASTER AND THE ORCHESTRATOR]** You report the Follow lane's compile **and link** are GREEN — good, and it removes the compile risk. **It does not close this WARN, because the hazard is COMMIT ORDERING, not compile health.** `SiegeAssistantSnapshot.cpp:234-235` calls `ACastle::FindNearestCastleForTeam`, which TASK-418's handoff records as absent from `HEAD` (uncommitted FOLLOW-batch work). If that is still true, **TASK-422 commit A cannot land before — or without — the FOLLOW lane's `Castle.{h,cpp}` commit**, or `main` is left non-compiling even though every working tree compiles. One command settles it: `git show HEAD:Source/GitClaudeUnrealTest/Siegebound/Castle.h | grep FindNearestCastleForTeam`. I still have no shell.
- **[WARN-4 · open → TASK-412]** `handoffs/TASK-410-programmer.md` still does not exist, so criterion (6)'s byte cross-check remains unperformable here. The 78% figure is a **3.6 chars/token estimate**, not spike measurement #3 — now correctly labelled as such in the header.
- **[WARN-6 · open → TASK-421]** Reactive-alias debt unchanged: `the nearest ancient ground`, `the nearest mine`, `the enemy base` are still absent, and `ancient_ground_near` still claims the bare alias `ancient ground` (`:158`) which quietly resolves the ambiguity `DEV-06` was built on. TASK-421's input, not a TASK-417 defect.
- **[WARN-7 · NEW → TASK-421 / the art-side gate] ⚠️ THE TWO NEW GUARDS NEVER SEE `DA_AssistantVocabulary`.** Test 11 reads `GetDefault<USiegeAssistantVocabulary>()` (`:893`) and `NewObject<USiegeAssistantVocabulary>()` (`:911`) — **both carry the C++ constructor defaults.** Nothing in the suite loads the authored asset. So the guards protect the **fallback** table and not the **shipped** one, and TASK-421 is precisely where `militia_mob` or a caster alias could return — into the table that actually ships in Zone A. This reads as "the bug can't come back" and what it means is "the bug can't come back *in the file we just fixed*."
  **Action:** both rules become **explicit acceptance criteria on TASK-421** — (a) no unit `Canonical` contains a separator; it is the CardID lower-cased, derived from `cards.csv`, never spelled by eye; (b) no alias on any row embeds `mage`/`caster`/`spellcaster` as a substring — and the art-side check verifies them against the authored asset, not against the CDO.

---

## The two new test guards — assessment as requested

**They close the MECHANISM, not the CLASS.** That is an acceptable and well-reasoned stopping point, but it should be recorded as such rather than left to read as complete.

**Guard A** (`:955-956`, no `_` in a unit canonical, deliberately **not** applied to places):
- **Closes** the observed mechanism and generalises to every future multi-word CardID. The comment explaining why places are exempt is exactly right and is the part that stops someone "fixing" the asymmetry later.
- **Misses**: a hyphen or space (`militia mob`, `militia-mob`), a plain typo (`footmen` as a canonical), or a canonical naming a card that does not exist (`catapult`). All pass the underscore check and are all equally unreachable.
- **A strictly better version costs the same and subsumes it:** assert the canonical matches `[a-z0-9]+` — a charset check rather than a single-character check. CardIDs are PascalCase alphanumeric, so lower-casing can only ever yield that set. Filed as NIT-8.
- **What would close the class outright** is set-membership against `DT_Cards` — and that would make a deliberately pure, asset-free suite depend on content. Declining that is the right trade; the class-closing check belongs to **TASK-420's PIE sanity**, which already fields units and pastes the roster (a live MilitiaMob prints `militiamob` in the roster line).

**Guard B** (`:968-969`, substring `mage`/`caster`/`spellcaster`, with the old whole-string check **retained** at `:975-976`, not replaced):
- **Closes** `fire mage`, `battle mage`, `war caster`, `spellcaster` — the whole embedding family that defeated the old whole-string check. Correctly stricter than §9b's token rule, and the reasoning is written at the site.
- **Cannot close the class, and nothing string-based could:** `warlock`, `magician`, `sorceress`, `conjurer` would all pass and would pull exactly the same way. "Words that read as a generic spellcaster" is a semantic property.
- **That residual is already covered by the right instrument** — the `[notes]` block plus the scored rows DEV-06 / HOLD-09. Assertion for string properties, measurement for semantics, is the correct division of labour. No further guard recommended.

**Test count: still exactly 11**, same 11 names, none removed, none renamed, none weakened — verified by grepping every `IMPLEMENT_SIMPLE_AUTOMATION_TEST` and every test path. Both guards are **additive** inside test 11, and the pre-existing whole-string caster check survives alongside the new substring one.

---

## ⚠️ RULING — the cross-owner comment edit in `SiegeAssistantSnapshot.h`

**ACCEPTED. I agree it was the right call, and I think it was substantively right, not merely permissible.** Recorded narrowly, because you are correct that this is the kind of precedent that gets cited later.

**I verified it is comment-only rather than accepting the declaration.** Every declaration in the header — the `USTRUCT` and its three `UPROPERTY` fields, the `UCLASS` line, all four `static constexpr` constants **including `MaxSnapshotChars = 1440` unchanged**, all eight pinned §9 members, both private enums, all three helpers, all 19 members and the three log latches — is byte-identical to loop 1; only line numbers moved. `SiegeAssistantSnapshot.cpp` is **entirely untouched** (`CanonicalKind` still at `:99`, the brace-init fix still at `:351` — the same line number as loop 1, which is itself evidence the file did not move).

**Why the law was not violated in substance:** M8's single-owner rule guards **concurrent clobber**. Here TASK-416 was already `qa-passed` — its owner had finished and handed off, so there was no live writer to clobber. The hazard was absent, and the law's purpose was satisfied vacuously, the same shape as CONVENTIONS §6 Ruling A.

**Why it was the right call and not just a permissible one:** the stale "~350" lived in **TASK-416's header**, which is the first thing a future reader opens. Fixing the arithmetic only in TASK-417's own files would have left the trap sitting in the file most likely to be read — the fix would have been in the wrong place for the hazard it addresses.

**The four conditions that make this citable, all of which held — a precedent, not a licence:**
1. The owning task is already **`qa-passed`** (no live writer). *"The module is quiet right now"* is **not** sufficient; the owner must be done.
2. **Comment-only, zero behaviour change** — and **the reviewer verifies this independently**, by diffing the declaration set. This is the condition that makes the precedent safe; a claim of "comment-only" is a relayed diagnosis like any other.
3. Made under an **explicit written QA finding that names the sites**. Not a drive-by tidy-up.
4. **Declared in the handoff**, so it never reads as an unexplained diff.

**If any condition fails, the correction goes back to the owner as its own task.** In particular this does **not** authorise a cross-owner `.cpp` edit, a cross-owner edit to close a NIT, or an edit to a file whose owner is mid-task.

**Applied immediately:** NIT-10 below identifies a stale comment in `SiegeAssistantSnapshot.cpp` — TASK-416's `.cpp`. **Do not make a second cross-owner edit for it.** It is a NIT, condition (2) would hold but (3) would not, and it gets swept when someone legitimately owns that file.

---

## New NITs from this loop

- **[NIT-8] Guard A could be a charset check for the same cost.** `SiegeAssistantGrammarTest.cpp:955-956` — assert the unit canonical matches `[a-z0-9]+` rather than merely lacking `_`. Subsumes the underscore rule and additionally catches spaces, hyphens and punctuation. Still pure, still no asset dependency.
- **[NIT-9] The `[notes]` line uses `caster` in two senses within one string.** `SiegeAssistantVocabulary.cpp:200` reads *"wizard = ranged fire **caster**"* and then *"mage / **caster** / spellcaster = ambiguous -> ask which_unit"* — same `=` operator, opposite meanings, one clause apart. **No scored row is at risk** (DEV-06 and HOLD-09 both use *"the mage"*, and `mage` appears only on the prohibition side), so this is cosmetic. Reword the gloss to *"ranged fire attacker"* and the line stops arguing with itself.
- **[NIT-10] Four stale `~500-token prefill` sites**, now inconsistent with the corrected Zone-A arithmetic three lines away: `SiegeAssistantSnapshot.h:111`, `SiegeAssistantVocabulary.h:52`, `SiegeAssistantGrammarTest.cpp:901`, and **`SiegeAssistantSnapshot.cpp:544`** (TASK-416's `.cpp` — see the ruling above). With Zone A at ~1180 tok the real turn-1 prefill is ~1345 → ~165 on turn two. The **ratio** is what bar #3 measures and it is documented correctly, so this is descriptive text only. **Sweep it when TASK-413 supplies real tokenizer counts** — that is the moment all four sites get correct absolutes at once, and it needs no cross-owner edit.

---

## Everything from loop 1 that this fix did NOT disturb — re-confirmed

Because the fix touched the file that feeds Zone A, I re-checked the things that could have regressed rather than assuming a small diff stays small:

- **No pinned §9 signature moved.** `GrammarCountMax = 30`, `SiegeAssistantMaxSelectionKinds = 3`, `USiegeAssistantGrammar::Build`, `ParseSiegeAssistantCommand`, `FSiegeAssistantCommand`, `USiegeAssistantVocabulary`'s three `UPROPERTY`s + `BuildSynonymTable`, and all eight `USiegeAssistantSnapshot` members — all byte-identical.
- **The Zone-A ↔ GBNF seam is untouched.** The fix is in the synonym block; the schema block and all three few-shots are unchanged, so the loop-1 proof (all three few-shots derived through the landed grammar **and** the parser, all accepted; all four sketch traps correct) still holds.
- **The sealed corpus was NOT edited** to accommodate the fix — `assistant_eval_{dev,holdout}.csv` are unchanged, and MilitiaMob still appears in neither, which is correct. The forbidden direction was not taken.
- **`BuildSynonymTable`'s determinism is unaffected** — the fix changes row content, not ordering; sorting is still by lower-cased string in both `AppendSection` and `NormaliseAliases`.
- **The three compile-error fixes are all still in place** — brace-init at `SiegeAssistantSnapshot.cpp:351`, no pointer-returning JSON helper, both key-type helpers intact.
- `Build.cs` unchanged: `Json` + `JsonUtilities` only.

---

## Notes for build-master (TASK-420) — you are cleared to run

1. **⛔ QUIESCE THE MODULE FIRST.** Confirm no programmer task in `Source/GitClaudeUnrealTest/` is in flight and no other compile gate is running. This lane is one of the two batches whose collision produced the QUIET-MODULE LAW.
2. **Attribute every diagnostic to the file that names it.** Anything from `SiegePlayerController.*` / `SummonedUnit.*` / `MinerUnit.*` / `Castle.*` / `GoldNode.*` belongs to the FOLLOW batch and is routed there, never recorded as this lane's finding.
3. **WARN-3 is a Git-ordering check, not a compile check** — see above. Report what `git show HEAD:...Castle.h` says even though your build is green.
4. **Re-confirm criterion (9) mechanically:** `git diff -U0 -- Source/GitClaudeUnrealTest/Siegebound/AncientGround.h Source/GitClaudeUnrealTest/Siegebound/AncientGround.cpp` must show zero deletions and zero modified lines.
5. **Run all 11 tests and paste the list.** The four that would mean a *law* broke rather than a bug appeared: `Grammar.CountRange` (the 1..30 range), `Grammar.SelectionCap` (bounded alternation, no `*`/`+`/`?`), and the two new guards inside `Vocabulary.SynonymTable`.
6. **PIE sanity §(3):** field a **MilitiaMob** specifically if `SummonTestUnit` allows it — its roster line printing `militiamob` is the live confirmation of BLOCKER-1's fix, and it costs nothing while you are already capturing a snapshot. Keep the `ancient_ground_near` ≠ `ancient_ground_far` assertion.
7. **`BuildZoneA` twice with the SAME vocabulary pointer.** `BuildZoneA(nullptr)` and `BuildZoneA(Asset)` legitimately differ (the caller contract) — pass the same pointer both times or the check is meaningless.
8. **Paste the snapshot in full with its character count**, and note Zone A's real length alongside it — that measurement feeds WARN-2 and TASK-413.

## Notes for the orchestrator

- **TASK-421 must be dispatched with WARN-6 and WARN-7 as explicit acceptance criteria.** WARN-7 is the one that will otherwise be missed: the new mechanical guards do not see the authored asset, so the art-side check has to carry those two rules by hand.
- **TASK-413 owns WARN-2 and WARN-4** — real tokenizer counts for Zone A *with the asset loaded*, reported against `ContextTokens 2048`, and the holdout number reported separately from the dev number.

---

# Appendix — LOOP 1 RECORD (2026-08-02, verdict FAIL)

Kept so the trail is auditable and so the still-open NITs are citable.

**Verdict was FAIL** — 1 BLOCKER (`militia_mob`, closed above), 6 WARN, 7 NIT. TASK-416 / 418 / 426 passed on the first loop and were not re-opened.

**The three relayed compile diagnostics, all re-derived from the artifacts — findings stand unchanged:**
1. **`C2228`/`C2737`** — most-vexing-parse **CONFIRMED**; the "complete-type include law" reading **REFUTED from the file** (`#include "Engine/DataTable.h"` is at `SiegeAssistantSnapshot.cpp:6` and always was, so both errors would have survived that "fix"). Brace-init present at `:351`. Lane-wide sweep for `Decl Name(TypeName(identifier));` found no second instance. Distinguishing test recorded: a genuinely missing complete type gives **`C2027`**, never `C2228`.
2. **`C4172`** — genuine UB, relayed cause **correct**, verified in the engine: `FJsonObject` publicly inherits the storage class whose `using FStringType = UE::FSharedString;` is at `JsonObject.h:99`, `Values` at `:237`, so `TPair<FString, TSharedPtr<FJsonValue>>&` genuinely converted rather than bound. Pointer-returning helper deleted; **no pointer/reference return, no `return &`, and no explicit `TPair` binding survives anywhere in the lane.**
3. **The four follow-on `C2039`/`C2664`** — the type-level abstraction **genuinely holds**: `UE::TSharedString` provides `Len()` (`SharedString.h:74`) and `operator*` (`:80`) publicly, `FString` provides both, `UE_JSONOBJECT_LEGACY_STRING_KEYS` is still live (`JsonObject.h:46`, legacy `FStringType = FString` at `:248`), and `FStringView::Equals(const TCHAR*, ESearchCase::Type)` exists exactly at `StringView.h:389`.

**Zone A ↔ GBNF seam (criterion 6a) — PASS, parsed not eyeballed.** All three few-shots derived through `root/command/who/selection/item/count/where/when` **and** through `ParseSiegeAssistantCommand`; all accepted. All four sketch-vs-landed traps correct: `who` not `select` · `n` not `count` · `"who":"none"` not `[]` · `{"ask":ASK}` taught with the five codes in `SiegeAssistantAskCodes`' exact order.

**Deliberate non-findings, confirmed and NOT filed:** `count` 1–30 not live-max, with its guarding 1-kind-roster test · selection cap as bounded alternation with **no `*`/`+`/`?`** anywhere, with its asserting test · `Kinds.Num() != Counts.Num()` hard-fails at `SiegeAssistantCommand.cpp:408` · **Zone A's size is correct and no trim was recommended anywhere.**

**Other criteria all PASS on both loops:** §9 registry character-for-character incl. access levels · §1 no multi-turn loop · §3 symbols only, no `FVector`/coordinate/timestamp/prose in any zone · §4 no registry/cache/dirty flag · Zone A byte-stability + fixed key order · Zone B+C cap with deterministic tail collapse logged once · shipping eligibility predicates never reimplemented (`ECardProfile`/`Profile ==`: zero hits) · `AncientGround` additive-only with **zero `HasAuthority()` in the `.cpp`** · `Build.cs` = `Json` + `JsonUtilities` only, no `HTTP`, no `Sockets` · 11 tests with a 37-case rejection battery, all traced by hand · no shadowing, complete-type includes, no literal `*/`, all format strings literal · M8 declaration duty verbatim in three handoffs and four headers, and true in code (zero `bReplicates`/`Replicated`/`Tick(`) · `GetFirstPlayerController()` zero calls.

**Corpus (TASK-426) verified from the files:** headers byte-identical and matching the §11 pin · 25/15 rows · `^intent=([a-z]+); ` present in **40/40** `Notes` cells · 8 commas per row, no in-cell comma · all six place symbols match `PlaceVocabulary` character-for-character with **`own_castle`** in DEV-16 and HOLD-06 and **`my_castle` nowhere on disk** · `nearest_mine` in DEV-23/HOLD-12 · every unit symbol a real lower-cased CardID · index-alignment, cap ≤ 3, counts 0..30, trigger pairs never half-filled, Refuse rows outcome-only · **few-shot disjointness checked here by literal string comparison — no corpus sentence appears as a few-shot.**

**Loop-1 NITs, still open and still non-gating:**
- **NIT-1** `SiegeAssistantCommand.cpp:236-238` — a non-numeric, non-string `n` reports `count_out_of_range:not_a_number` where `bad_type` reads better. Test 8 pins current behaviour.
- **NIT-2** No pure test asserts `BuildZoneA` byte-stability / state-independence; TASK-420 §(3) covers it at runtime. A 12th test would make it a compile-time guarantee.
- **NIT-3** `Zone B + Zone C <= MaxSnapshotChars` is guaranteed by the `MaxUtteranceChars` arithmetic, not by a final clamp. Unreachable today (`Head + Tail` ≈ 720 max); recorded for whoever raises that constant.
- **NIT-4** Few-shot #1 is a near-paraphrase of `DEV-01` (disjointness holds; both derive from the flagship sentence CONVENTIONS already carried). Consequence: DEV-01's dev-set result is not independent evidence — the holdout is.
- **NIT-5** Zone A's few-shots name `footman`/`sorcerer`/`archer` literally: shape-conformant under any roster, symbol-conformant only when those kinds are alive. ⛔ Do **not** "fix" by making Zone A state-dependent — that is the fatal §8 change.
- **NIT-6** `handoffs/TASK-418-programmer.md` cites `CONVENTIONS.md:699` / `AncientGround.h:119`; live lines are 779 / 145. Text matches; anchors drifted.
- **NIT-7** `ancient_ground_far` under the 3× castle law resolves as "nearest ground to the nearest enemy castle"; the identity guard (`SiegeAssistantSnapshot.cpp:275`) turns a degenerate result into a **missing** symbol rather than a wrong one — the safe direction. TASK-420 §(3) already asserts two different actors.
