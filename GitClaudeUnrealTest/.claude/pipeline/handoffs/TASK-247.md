# TASK-247 handoff — TASKBOARD.md mojibake repair (scripted, quiet-window)

- agent: build-master
- date: 2026-07-22
- verdict: REPAIRED + VERIFIED (TASKBOARD.md); CONVENTIONS.md scanned CLEAN, no repair needed
- commit: NONE by ruling — see "Pending commit" below

## What was wrong

A board write around the TASK-241 flip re-saved older sections through a cp1252 mis-decode:
UTF-8 bytes were read as windows-1252 and re-encoded as UTF-8 (classic double-encode). Em-dashes
became `â€`-class sequences, `×` became `Ã—`, `§` became `Â§`, arrows became `â†’`-class, emoji
became `ðŸ`-wrecks. The `â†` + U+0090 (C1 control) sequences found in the file prove the corrupting
reader used a *sloppy* cp1252 (the 5 undefined bytes 0x81/0x8D/0x8F/0x90/0x9D passed through as C1
controls) — the repair used the matching sloppy-cp1252 map.

## Method (scripted — no hand edits)

Tool: scratchpad `task247/mojibake_tool.py` (python 3.14), binary-safe line handling (split on
`\n` only, endings/BOM untouched). Two passes:

1. **Pass 1 — whole-line round-trip:** a line qualifies only if ALL its chars reverse-map through
   sloppy-cp1252 to bytes that decode as STRICT UTF-8 to different text. Clean lines are
   structurally immune (a lone real em-dash maps to 0x97, which is invalid UTF-8 alone → line
   untouched). **2,333 lines fixed.**
2. **Pass 2 — run-scoped (MIXED lines):** maximal non-ASCII runs (len ≥ 2) that individually
   round-trip. Needed because recent clean edits were threaded INTO corrupt lines. **2 lines
   fixed:** line 1 (header: BOM is clean/real, one mojibake em-dash) and line 1582 (TASK-170/171
   lineage status — the TASK-241-era clean edit sat inside an older corrupt line; 15 runs incl.
   `â†` + U+0090 → `←`).
3. **Exclusion (permanent, for any future scripted pass):** TASKBOARD lines 606–607 — the TASK-247
   spec itself intentionally QUOTES mojibake examples (`â€"`, `Ã—`, `ðŸ`). Line 607's `Ã—` was
   excluded from pass 2; line 606 never qualifies (its examples use ASCII quotes). Do NOT "fix"
   these.

## Detection counts / mapping

- TASKBOARD.md: **2,335 lines repaired** (2,333 whole-line + 2 mixed) across regions **7–485 and
  634–6126** (old numbering); the 2026-07-21/22 blocks (~486–633) were already clean.
- Mapping table = the full sloppy-cp1252 byte↔char map (256 entries), i.e. exact inverse of the
  corruption — not a hand-picked sequence list. Observed fixes: `â€"→—` `â€œ/â€�→“”` `â€“→–`
  `â†’→→` `â†�→←` `âˆ¥→∥` `â‡’→⇒` `Ã—→×` `Â§→§` `Â±→±` `Â²→²` `Ã©→é` `ðŸ”§/ðŸš¨/ðŸŽ¨→🔧🚨🎨` etc.
- CONVENTIONS.md: **0 candidates, 0 mixed lines — clean, untouched.**

## Verification evidence (all scripted assertions, zero failures)

- (a) Repaired file decodes as strict UTF-8; line count identical (6,131); BOM preserved.
- (b) Encoding-only diff: python per-line compare snapshot→output = **2,335 differing lines,
  set-equal to the script's changed-line log**; ASCII byte projection identical on every changed
  line (no content/whitespace drift); pass-1 inverse proof per line (re-corrupting the fix
  reproduces the original bytes exactly).
- (c) Spot-checks now correct: line 1 header em-dash; M7.6 heading tail "— Arena 10× scale-up +
  LOD/perf" (l.634); Authorization "10× MAP SCALE-UP … plan §1/§2/§3/§4/§6" (l.636); slack-quote
  emoji 🚨/🔧/🎨; TASK-241 block + line-1582 lineage status arrows/dashes.
- (d) Anchor-match proven LIVE: immediately after install the manager inserted the 59-line W1-PREP
  sub-block anchored under the repaired Authorization line (diff `636a637,695`) and QA flipped
  TASK-248 (l.618) — both matched anchors inside previously-corrupt territory, both writes clean
  UTF-8.
- Residual signature scan on the final live file: **zero** mojibake outside the intentional
  606–607 spec examples.

## Snapshots + hashes (scratchpad `…\764973cf-…\scratchpad\task247\`)

- `TASKBOARD.snapshot.md` (dispatch-time) sha256 `9a95157a64cc25a4bc11b013c41af23b939c5f83b2ac80d06df511157c6b5ca7`
- `TASKBOARD.snapshot2.md` (authoritative pre-repair input) sha256 `9177524ad464b89e87ae0757bf199d8b9bb279e7705b41d50b074f92bd4ae0ae`
- Repaired output as installed sha256 `5fbfd1ff0cebdc7bf9b6e2e42a5feb1c7f3567962f5b24f02b107a68cc6e0293`
- `CONVENTIONS.snapshot.md` sha256 `82afce5c97fd004af796f274c4f7f03584cf3456555fb4dab5051a364105a152` (unchanged on disk)
- Also there: `mojibake_tool.py`, detect/fix logs, `TASKBOARD.fixed.md` (byte-identical to install).

## Window violation note (no damage, orchestrator FYI)

The "no parallel board writers" window did NOT hold — two interleaved writes: (1) TASK-248 status
flips (pre-repair flip absorbed into the repair input; post-repair qa-passed flip landed on the
repaired file), (2) manager's W1-PREP/TASK-249..251 insertion post-install. Both were based on the
repaired content and are clean — verified, nothing clobbered either direction.

## Pending commit (IMPORTANT)

Per spec, **nothing was committed** — these are MAIN-lane docs but the tree is checked out on
`m7.6-arena10x` (at `ec7a271`) and both files carry legit uncommitted churn beyond the encoding fix
(TASK-248 qa-passed flip, W1-PREP block, TASK-246/247/248 handoffs, plus the cards.csv/C++ spell-lane
edits in the same tree). The encoding repair **rides the next MAIN docs window** together with that
board churn. Nothing pushed.
