# Handoff: TASK-1582 (gameplay-programmer) — marker `TASK-1582-VSBOT-RECIPE-UPPER-BOUND-WORDING`

Date 2026-09-28. Status → `ready-for-qa`. Gate: `TASK-1583`. Host: the first commit host to derive after `TASK-1583` PASS (expected `TASK-1580`, per the row). No compile and no runtime criterion.

Text only. No editor, no compile, no PIE, no mutating git. RUNS, declared (`SC-§71a`):
- `Read` / `Grep` / `Edit` / `Write` (this handoff);
- read-only `sha256sum`, `wc`, `sed -n` and `grep` over repo files;
- `git --no-optional-locks diff -U0` / `diff --stat`, plus one `git --no-optional-locks status --short -- Tools/Verify/recipes/` before the edit, which printed nothing. The row lists `diff` only; `status` is declared here because I ran it.
- One read-only Python reverse-derivation script, `revderive_1582.py`, in my session scratchpad (outside the repo). It opens the recipe `rb` and writes nothing.

Point of truth: `qa/TASK-1565.md` §2 W1 and N1. Every site was found by its text. `qa/TASK-1565.md`'s line numbers were still right at my instant because the file was byte-equal to (0). They are not right now: the two new `## Source` lines push everything below `:28` down by 2.

## Files touched

| file | before sha256 | after sha256 | bytes | lines | EOL |
|---|---|---|---|---|---|
| `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md` | `0a63186421c3daf48af0d64b32e6ad17db6b45a239ca4864d041ee439c3ee2ea` (= the row's (0) anchor; measured before the first edit) | `53b0d17fbe4be6c526d5b540c8caf7cf8f87dcf64bdd83544fbd3ce08139ab0d` | 35521 → 37453 | 248 → 250 | LF, CR 0, no BOM, trailing LF |
| `.claude/pipeline/handoffs/TASK-1582-programmer.md` | new | (this file) | | | |
| `.claude/pipeline/TASKBOARD.md` | this row's `status:` line only | | | | |

- Git index line from `git diff`: `f2c11a5..0ea9150`. The before blob `f2c11a5` is the blob `qa/TASK-1565.md` §4 computed for the (0) bytes (`f2c11a5ed3135b8ceae56c0dcb9290e8f8aeafe1`). So HEAD (`9b82e8d`) held exactly (0) when I started.
- `git diff --stat`: 1 file, 4 insertions(+), 2 deletions(−).
- Git printed its usual `LF will be replaced by CRLF` notice. That is about checkout; the working file is LF.

**Reverse derivation (my own cut, measured).** Each cut string was asserted to occur exactly once.
1. At `:10`, replace "rows 1–4 ~~were measured once~~\*\* ~~(a2, 2026-09-26)~~ Corrected 2026-09-28 by \`TASK-1582\`: \*\*have at most one run, a2 (2026-09-26); what a2 did not measure there stays \`NOT MEASURED\`\*\*. Step 1" with "rows 1–4 were measured once\*\* (a2, 2026-09-26). Step 1".
2. At `:194`, replace "\*\*~~One run (a2, 2026-09-26) of~~ Corrected 2026-09-28 by \`TASK-1582\`: the walk," with "\*\*One run (a2, 2026-09-26) of the walk,".
3. Also at `:194`, replace "rows 1–4 have at most one run, a2 (2026-09-26); what a2 did not measure there stays \`NOT MEASURED\`; one summon.\*\*" with "rows 1–4; one summon.\*\*".
4. Drop the two whole lines that start "- Amended by \`TASK-1564\`, 2026-09-28," and "- Amended by \`TASK-1582\`, 2026-09-28,".

Result: **`0a63186421c3daf48af0d64b32e6ad17db6b45a239ca4864d041ee439c3ee2ea`, 35521 B, equal to (0).**

A line-level compare of the derived file against the current one names three spots and nothing else: `:10` (replaced), `:29`–`:30` (inserted) and derived `:192` = current `:194` (replaced). That fits the +4/−2 diffstat.

**Old-text form:**
- `:10`: the old words "were measured once" and "(a2, 2026-09-26)" are each struck with `~~…~~` and kept in full. They are two strike spans, not one: a single span would cross the old `**` bold closer that sits between them. The new words follow, marked "Corrected 2026-09-28 by \`TASK-1582\`:" (the `TASK-1564` precedent on the same line).
- `:194`: the old words "One run (a2, 2026-09-26) of" are struck and kept. The marker follows them directly. The old words "the walk, the capture, the summon and every step and row except Step 1 rows 1–4" and "; one summon." are kept byte-for-byte. The new predicate is inserted between them.
- Nothing is deleted and nothing is re-verdicted. `TASK-1564`'s struck text and its "Corrected 2026-09-28 by \`TASK-1564\`:" markers are untouched.

## Raw `git --no-optional-locks diff -U0` hunks (read-only, declared)

```
diff --git a/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md b/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md
index f2c11a5..0ea9150 100644
--- a/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md
+++ b/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md
@@ -10 +10 @@ Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are d
-> - ~~**Measured once:** one run, one summon.~~ Corrected 2026-09-28 by `TASK-1564`: **One summon. The walk, the capture, the summon and every step and row except Step 1 rows 1–4 were measured once** (a2, 2026-09-26). Step 1 rows 1–4 (the `Button_0` focus snapshot, `IA_MenuAccept`, `wait 3`, the in-batch `record_burst {"seconds":1}`) have a second run, `1493` (2026-09-27), which re-verified them in-run **y** and did no walk, capture or summon. `[M: 1493 Recipes used]`
+> - ~~**Measured once:** one run, one summon.~~ Corrected 2026-09-28 by `TASK-1564`: **One summon. The walk, the capture, the summon and every step and row except Step 1 rows 1–4 ~~were measured once~~** ~~(a2, 2026-09-26)~~ Corrected 2026-09-28 by `TASK-1582`: **have at most one run, a2 (2026-09-26); what a2 did not measure there stays `NOT MEASURED`**. Step 1 rows 1–4 (the `Button_0` focus snapshot, `IA_MenuAccept`, `wait 3`, the in-batch `record_burst {"seconds":1}`) have a second run, `1493` (2026-09-27), which re-verified them in-run **y** and did no walk, capture or summon. `[M: 1493 Recipes used]`
@@ -28,0 +29,2 @@ Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are d
+- Amended by `TASK-1564`, 2026-09-28, on `qa/TASK-1555.md` §5 items 1–3 and 5–7 (and §3 N1 for a date source). Named as `handoffs/TASK-1564-programmer.md` names them: the two corrections, "FALSE, corrected: struck and scoped to Step 1 rows 1–4" (the ⚠️ *Read this first* "Measured once" bullet and its *Fences* twin, "One run (a2), one summon."); "the short name" `1493`, added to the run short names at the top, with the run date taken from `TASK-1493`'s status flip; "`1493`'s values beside Step 1 rows 1–4" (the source cells of Step 1 rows 1–4); "The second H3 sighting", "at four sites, each saying it stays `HYPOTHESIS`" (Precondition 5, the *Fences* films bullet and the *Known hazards* films bullet here; the fourth is `README.md`'s H3 line); and the Step 0 run list, "Step 0 was also run by two later runs" (`qa/TASK-1524-verify.md` and `qa/TASK-1519-verify.md`, added to the *Known hazards* "A batch cannot branch" sub-bullet). Recorded by `TASK-1582`, 2026-09-28.
+- Amended by `TASK-1582`, 2026-09-28, on `qa/TASK-1565.md` W1 and N1. W1: the two clauses `TASK-1564` wrote (in the ⚠️ *Read this first* bullet and its *Fences* twin) read as one measurement per row. Each now states the upper bound it meant, "have at most one run, a2 (2026-09-26); what a2 did not measure there stays `NOT MEASURED`", with the words it replaces struck. The labels it must not contradict are unchanged: row 15a's position after the play (`NOT MEASURED`), and the Step 3 tail's order ("this recipe's composition, not a2's") and length (`NOT MEASURED`). "One summon" and the `1493` sentences are unchanged. N1: the `TASK-1564` entry above, and this one.
@@ -192 +194 @@ Pixels proved nothing here. In both a2 frames "The Archer is not identifiable by
-- ~~**One run (a2), one summon.** No second run exists, so other days, map states and bot behaviours are untested.~~ Corrected 2026-09-28 by `TASK-1564`: **One run (a2, 2026-09-26) of the walk, the capture, the summon and every step and row except Step 1 rows 1–4; one summon.** No second run of them exists, so for them other days, map states and bot behaviours are untested. Step 1 rows 1–4 have a second run on another day, `1493` (2026-09-27), re-verified in-run **y** for those rows only; it did no walk, capture or summon. `[M: 1493 Recipes used]`
+- ~~**One run (a2), one summon.** No second run exists, so other days, map states and bot behaviours are untested.~~ Corrected 2026-09-28 by `TASK-1564`: **~~One run (a2, 2026-09-26) of~~ Corrected 2026-09-28 by `TASK-1582`: the walk, the capture, the summon and every step and row except Step 1 rows 1–4 have at most one run, a2 (2026-09-26); what a2 did not measure there stays `NOT MEASURED`; one summon.** No second run of them exists, so for them other days, map states and bot behaviours are untested. Step 1 rows 1–4 have a second run on another day, `1493` (2026-09-27), re-verified in-run **y** for those rows only; it did no walk, capture or summon. `[M: 1493 Recipes used]`
```

## Each changed clause, old beside new

| site (found by text) | old | new (rendered words; struck words omitted) |
|---|---|---|
| ⚠️ *Read this first*, the `TASK-1564` bullet (`:10`) | "The walk, the capture, the summon and every step and row except Step 1 rows 1–4 **were measured once** (a2, 2026-09-26)." | "The walk, the capture, the summon and every step and row except Step 1 rows 1–4 … **have at most one run, a2 (2026-09-26); what a2 did not measure there stays `NOT MEASURED`**." |
| *Fences*, its twin (`:192` → `:194`) | "**One run (a2, 2026-09-26) of** the walk, the capture, the summon and every step and row except Step 1 rows 1–4; one summon." | "… the walk, the capture, the summon and every step and row except Step 1 rows 1–4 **have at most one run, a2 (2026-09-26); what a2 did not measure there stays `NOT MEASURED`**; one summon." |

The new words are QA's suggested wording, verbatim (`qa/TASK-1565.md` W1: "…have at most one run, a2 (2026-09-26); what a2 did not measure there stays `NOT MEASURED`").

**The `NOT MEASURED` labels the clauses must not contradict.** All are unchanged, quoted at their current addresses:
- `:74` (row 15a's cell): "**Its position here, after the play: `NOT MEASURED`.**"
- `:120` (the `NOT MEASURED` list under "What in these objects is quoted and what is not"): "**Row 15a's position.** The `DrawPile` read object is row 5's, measured in-batch at t=2.74 `[M: 1512 A1]`. No run has read it after the play inside a batch."
- `:144` (Step 3 tail, order): "**The tail's ORDER is this recipe's composition, not a2's.**"
- `:145` (Step 3 tail, length): "This tail writes no waits, so its own length is `NOT MEASURED`."

**Why the new clauses contradict none of them:** "at most one run" admits zero. The clause's own second half sends every item a2 did not measure back to `NOT MEASURED`. The old "were measured once" / "One run … of every step and row" asserted one measurement per row, which the labels above deny for row 15a's position and for the tail's order and length.

**What stays (spec (1)):** "One summon." (the start of `:10`'s bold sentence) and "; one summon." (`:194`) are unchanged. So are both `1493` sentences: `:10`'s "Step 1 rows 1–4 (…) have a second run, `1493` (2026-09-27), …" and `:194`'s "Step 1 rows 1–4 have a second run on another day, `1493` …". So is `:194`'s "No second run of them exists, …".

**The kept sentences, re-measured at the edit (`SC-§126`):**
- Grep of `.claude/pipeline/qa/` for `RCP-vsbot|vsbot-capture|capture-center-and-summon`: the only `*-verify.md` hits are `TASK-1512-verify.md` and `TASK-1493-verify.md`.
- Grep of `qa/*-verify.md` for `CaptureZone_Center`: `TASK-1512-verify.md` and `PLAYTEST-archer50-verify.md`. The latter's A3/A4 rows read "**NOT REACHED.**" (no match was started).
- `TASK-1493-verify.md:95` still reads "Step 1 rows 1–4 only" and "No walk, capture or summon was done". `:12` still reads "A second film, armed at `start_pie`, holds the menu only".
- ⇒ "One summon", "No second run of them exists" and "at most one run, a2" all hold at my instant.

## The two `## Source` entries (spec (2); `qa/TASK-1565.md` N1)

The form is the existing "Amended by \`TASK-###\`, <date>, on <report> …", as at `:27`–`:28`.
- **`TASK-1564`** (new `:29`), dated 2026-09-28 from the handoff's "Date 2026-09-28", on `qa/TASK-1555.md` §5 items 1–3 and 5–7. That is the handoff's own "Point of truth" line. §3 N1 is the date source, and §3 N1 is indeed the finding about `TASK-1493`'s status flip. Each amendment is named by the handoff's own words:
  - "FALSE, corrected: struck and scoped to Step 1 rows 1–4": its heading (1).
  - "the short name" and "`1493`'s values beside Step 1 rows 1–4": heading (2).
  - "The second H3 sighting" and "at four sites, each saying it stays `HYPOTHESIS`": heading (3).
  - "Step 0 was also run by two later runs": the text heading (4)'s addition begins with.
  - The entry ends "Recorded by \`TASK-1582\`, 2026-09-28.", after `:26`'s "Recorded by \`TASK-1554\`, 2026-09-27." precedent, because this row, not `TASK-1564`, wrote it.
- **`TASK-1582`** (new `:30`), on `qa/TASK-1565.md` W1 and N1: the upper-bound wording, the labels it keeps, and this log.

## For QA to scrutinise

1. **`:10` strikes "(a2, 2026-09-26)" as well as "were measured once".** QA's wording restates the parenthetical as "a2 (2026-09-26)", so those words change form. Keeping it and writing "have at most one run (a2, 2026-09-26); …" would also be an equivalent. But it would split the insertion around the old parenthetical and move away from QA's words. I took the verbatim wording, which leaves one insertion point.
2. **Two strike spans at `:10`**, "~~were measured once~~\*\* ~~(a2, 2026-09-26)~~": one span would wrap the old `**` closer, and GFM would then print the `~~` literally. By the flanking rules, `~~` closes before `**` and each span pairs cleanly. Rendering was not viewed.
3. **`:194`'s marker placement.** "Corrected 2026-09-28 by \`TASK-1582\`:" sits right after the struck "One run (a2, 2026-09-26) of" and introduces the corrected clause. Its first words, "the walk, … rows 1–4", are old bytes kept in place. Its new bytes are the marker and " have at most one run, a2 (2026-09-26); what a2 did not measure there stays \`NOT MEASURED\`". The exact cut is above. The marker sits inside `TASK-1564`'s bold span, whereas at `:10` it sits outside bold. Both follow from where the old bold markers already were.
4. **One phrase is no longer unique.** "this recipe's composition, not a2's" now occurs twice: at `:30`, my Source entry quoting it, and at `:144`, the Step 3 tail. Anyone anchoring on that phrase should take the Step 3 occurrence or a longer string (`SC-§127` cl. 4: an anchor must be both stable and unique). The count of `` `NOT MEASURED` `` also rose, by 2 at the clauses and 3 in the `:30` entry.
5. **`:29` names `README.md`'s H3 line as the fourth H3 site.** That records the handoff's "four sites" truthfully. `README.md` itself was not opened for write and not changed.

## Declined / not done

- Nothing in the spec was declined.
- `qa/TASK-1555.md:115`'s hash abbreviation slip is left as it is (spec (3): it is recorded on `TASK-1555`).
- No other sentence of the recipe was changed. `:10`'s and `:194`'s other sentences are byte-identical.
- `README.md` and the other four recipes are unchanged; see below.

## Neighbours (sha256 now, all equal to `qa/TASK-1565.md` §0.6 / §4)

- `README.md` `4f0eaaf634553b05e36536f9d1de858ef292abd3cdc9823a322162c4052d465b`
- `RCP-menu-to-deckbuilder.md` `54c7e067b17659727d9cc0550e6a41587d3449178ef8acef6fa01ede5837882d`
- `RCP-deckbuilder-set-active-by-keyboard.md` `c1965e7bcd4eaaa38abdd76fd209c3e4d56c90a0400d394dd3d9b23ee75567dc`
- `RCP-deckbuilder-slot-and-card-edit.md` `f72c9530c0633fb002ec7231cdda8e76cde20831addaedeee88cadde60b2e2d7`
- `RCP-play-unit-card-from-hand.md` `3eb2d019c372f1d786967e7ba8ad03b15929bae1d5982bf77d56aa8fe44f425f` (ends `…8fe44f425f`, as `qa/TASK-1549-loop1.md:90`)

## Not examined / limitations

- **Markdown rendering was not viewed**, including the nested `~~` inside `**` at `:10` and `:194` and inside a blockquote list item at `:10`. The flanking analysis above is from the GFM rules, not from a renderer.
- **"Nothing else moved"** rests on four things: the exact reverse derivation to (0), the three-hunk `git diff -U0`, the before-blob equal to `qa/TASK-1565.md` §4's computed blob, and the neighbour hashes. `TASKBOARD.md` was not diffed: the manager and the parallel `TASK-1576` programmer write it concurrently. My only write there is this row's `status:` line.
- **The kept sentences were re-measured by grep of the `qa/` reports as written.** I did not check them against films, frames or logs. A run that used this route without naming the recipe or `CaptureZone_Center` in its report would not be found by that grep.
- **`handoffs/TASK-1564-programmer.md` was read as written.** Its claims about the README's H3 line are its own and `qa/TASK-1565.md`'s, and I did not re-derive them.
- No recipe was run. No PIE, compile or editor call was made, and no mutating git was run.
