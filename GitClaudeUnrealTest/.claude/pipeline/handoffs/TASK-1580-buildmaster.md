# TASK-1580: K6 code host (build-master handoff)

**Commit:** `b9db99cecaa1152dd02d0ce1f58b42f5b403af3b` (`b9db99c`), parent `9b82e8d`, 2026-09-29. This is a CODE commit: no `.claude/agents/*` and no `CLAUDE.md`. ⛔ Not pushed. Against `origin/main` (`f1091b8`) the count was 1 ahead / 0 behind before and **2 ahead / 0 behind after**.

marker `TASK-1580-K6-HOST-CODE` · row amendments up to `TASK-1580-AMENDED-4-2026-09-29` (flips list as updated by `TASK-1599`) · law: `CLAUDE.md` rule 5c · `TL-§5e` cl. 1/7 · `SC-§102` · `SC-§103` · `SC-§134` cl. 7(a) · `SHIP-§9c` · `VER-§4` cl. 1

Editor PID 15044 was not touched: no MCP call and no process action.

## 1. Gates read at my instant

| gate | read |
|---|---|
| `qa/TASK-1579-verify.md` | line 1 `Verdict: VERIFIED` |
| `qa/TASK-1545-verify.md` | line 1 `Verdict: MEASURED` (`TASK-1591` `done`) |
| `TASK-1578` | `built` (568/568, 6 arms red) |
| QA line 1 | `PASS` on `1568` / `1575` / `1577` / `1583` / `1586` / `1588`. No loop reports exist. |
| HEAD / index | `9b82e8d`, 1 ahead of `f1091b8`. The index was empty. |
| live agents | none (dispatch) |

## 2. Byte anchors: 14 of 14 worktree files equal before staging

| set | file | gated sha256 | HEAD blob |
|---|---|---|---|
| help (`qa/TASK-1577.md` §5) | `SiegeControlsHelpWidget.cpp` | `6a9ba044…fa6b` | = (via `git cat-file blob \| sha256sum`) |
| | `SiegeControlsHelpWidget.h` | `7ed5a066…1527` | = |
| | `Tests/SiegeControlsHelpTest.cpp` | `588a4180…3fa4` | = |
| | `SiegePlayerController.cpp` | `d2dfbf50…b1fd` | = |
| ship (`qa/TASK-1588.md` §4) | `Tools/Packaging/ship.ps1` | `3b366339…48d7` | = |
| | `Fixtures/check_cook_bp_verdict.ps1` | `d49f47b2…faeab` | = |
| | `Fixtures/cook-bp-green.log` | `b430b2f4…d444` | = |
| | `Fixtures/cook-bp-error-red.log` | `2b0fa374…1b40` | = |
| | `Fixtures/empty.log` | `e3b0c442…b855` | = |
| | `.claude/commands/ship.md` | `1cb5b1aa…c5eb` | = |
| | root `.gitattributes` (449 B, CRLF) | `a3a52ab3…984b` | LF blob, 439 B. Re-expanded LF→CRLF it equals the anchor. HEAD^ was LF too. |
| | control `Fixtures/b2_verdict_check.ps1` | `61bc217a…ace7` | clean, not in the diff |
| recipe (`qa/TASK-1583.md` §4) | `RCP-vsbot-capture-center-and-summon.md` | `53b0d17f…ab0d` | blob id `0ea91509` = §4 computed |
| | `handoffs/TASK-1582-programmer.md` | `fe2892b7…a8253` | blob id `e65dce30` = §4 computed |

## 3. Ship-gate host checklist (`qa/TASK-1588.md` §4), every step

1. I re-hashed all seven files and the control (above).
2. **Harnesses, re-run at my instant.**
   - `check_cook_bp_verdict.ps1 -RealLogDir …\packagedZIPofGame\.ship\20260910-063540`: empty-size, GREEN, RED, NULL, PAIR and REAL all `[OK]`. It printed `RESULT: ALL EXPECTATIONS MET on all three sides and the pair` and exited 0.
   - `b2_verdict_check.ps1`: `RESULT: ALL EXPECTATIONS MET on both sides`, exit 0.
   - The arms were not re-run, as QA said.
3. **Diff shapes.**
   - `ship.ps1` has four hunks: `-860,0 +861,126` · `-2354,0 +2481` · `-2435,0 +2563` · `-2563,0 +2692,7`. No `-` content line.
   - `ship.md` has one hunk, `@@ -102,0 +103 @@`, with no `-` line.
   - `.gitattributes` has one hunk, `@@ -9,0 +10 @@`, adding exactly `GitClaudeUnrealTest/Tools/Packaging/Fixtures/*.log -text`.
4. **Staged in the same `git add` as the three logs.**
   - `ls-files --eol` shows red `i/mixed w/mixed attr/-text`, green `i/crlf attr/-text` and empty `i/none attr/-text`.
   - `git add` printed no CRLF warning naming a `.log` file. The only warnings were on LF `.ps1` / `.md` / `.cpp` / `.h` files.
5. **After the commit.**
   - `git cat-file blob HEAD:…` piped to `sha256sum` in Git Bash gives red `2b0fa374…1b40`, green `b430b2f4…d444` and empty `e3b0c442…b855`.
   - `git check-attr text` reads `unset` on all three logs and `unspecified` on the control.
6. The ship set rode by explicit pathspec (§5).
7. No `/ship`, cook or package was run. The "fourth side" is still **owed**.

## 4. Frames promoted (`qa/TASK-1579-verify.md` Evidence; `VER-§4` cl. 1)

I copied these from `Saved/AuraVerify/T1579/` to `.claude/pipeline/playtest-evidence/2026-09-29/` with `cp -p`, not moved. For all 8, source sha256 = target sha256 = the LFS oid in HEAD, and size = the report's declared size.

| promoted name | source | sha256 | bytes |
|---|---|---|---|
| `VER-TASK-1579-t06m38s-placement-resize-both-numbers-page-fit.png` | `p11_placementresize_t398.36s_f41459.png` | `148ab6cf…7d42` | 597725 |
| `VER-TASK-1579-t06m13s-stack-upgrade-health-factor-page-fit.png` | `p10_stackupgrade_t373.30s_f40061.png` | `29c679d7…f339` | 583198 |
| `VER-TASK-1579-t13m28s-map-marks-circle-cap-page-fit.png` | `p25_mapmarks_t808.46s_f64357.png` | `0e45c058…fe04` | 587398 |
| `VER-TASK-1579-t11m45s-war-map-circle-cap-block-page-fit.png` | `p22_warmap_t705.70s_f58601.png` | `50e57766…8f31` | 581498 |
| `VER-TASK-1579-t08m34s-orders-follow-page-fit.png` | `p16_follow_t514.67s_f47908.png` | `c03d4ff0…b527` | 576532 |
| `VER-TASK-1579-t07m42s-orders-hold-page-fit.png` | `p14_hold_t462.64s_f45064.png` | `6c223e28…e713` | 562831 |
| `VER-TASK-1579-t08m08s-orders-ambush-page-fit.png` | `p15_ambush_t488.17s_f46351.png` | `6dc85bd7…012e` | 468033 |
| `VER-TASK-1579-t09m53s-pick-resize-wheel-sentence.png` | `p18_pickresize_t593.79s_f52250.png` | `f33aef6d…735d` | 550107 |

- Not promoted: the two frames the report does not cite (`p18_…t567.25s…`, `p6_cardsplay_…`).
- `qa/TASK-1545-verify.md` owes no frame. Its Evidence section says "None … nothing is owed to a host".
- No film and nothing from `testvideo/` was staged.

## 5. Commit `b9db99c` (`git show --stat HEAD`, 39 files, +6975 / −244)

- Committed with `git commit -F <msg> --pathspec-from-file=<39 paths>`. The index held exactly the derived 39 before the commit. It was empty after, and the tree was clean.
- **The 39 files:**
  - root `.gitattributes`
  - `.claude/commands/ship.md`
  - `CONVENTIONS.md` (+12/−2)
  - `TASKBOARD.md`
  - 9 handoffs: `1562` `1567` `1569` `1574` `1576` `1578` `1582` `1585` `1587`
  - 8 qa reports: `1545-verify` `1568` `1575` `1577` `1579-verify` `1583` `1586` `1588`
  - 8 frames
  - 4 help sources
  - `ship.ps1`
  - `check_cook_bp_verdict.ps1`
  - 3 `.log` fixtures
  - the vsbot recipe
- **`CONVENTIONS.md` read-back: 2 hunks at default context, both named.**
  - `@@ -12410,9 +12410,15`: `VER-§8` cl. 11, marker `VER-8-11-ROUTE-I-NAMED-2026-09-29`, named on `TASK-1544`'s status line. It covers the three sites that line lists. At `-U0` it splits into two sub-hunks, both inside cl. 11.
  - `@@ -12563,6 +12569,10`: `PKG-§14` cl. 5 rider, marker `PKG-14-5-SHIPS-ANYWAY-IS-AN-INFERENCE`, named on `TASK-1584`. No third hunk.
- **`TASKBOARD.md` read-back: 48 `-U0` hunks at derive time.** Each sits in a named row or block:
  - `1544`, `1545`, `1552`, `1553`, `1555` (erratum)
  - `1560`–`1566`, including TASK-1562's 9 lag lines: `1552` `1553` `1560` `1561` `1562` `1563` `1564` `1565` `1566`
  - the K6 block
  - `1567`–`1580`
  - the `1581`–`1599` insertion
  - `973`–`976` (the `TASK-1584` supersede and blocked-by edits)
  - plus my in-progress line

## 6. Board flips (`SC-§103`), written after the commit: 24 status lines

- `done` + COMMITTED: `1574` `1576` `1585` `1567` `1587` `1582` (the old status is kept after "· was").
- Report COMMITTED: `1575` `1577` `1586` `1568` `1588` `1583`.
- `1578` → `done` · `1579` → `done` + COMMITTED, verdict `VERIFIED` kept.
- Appended COMMITTED:
  - `1569`
  - `1545` (its verdict wording untouched)
  - `1591`
  - the manager rows `1544` `1581` `1584` `1589` `1590` `1599`
- This row → `done` + hash.

## 7. One-cycle lag: the next host (`TASK-1598`) must carry these

- `TASKBOARD.md`: the 24 status lines in §6 (`git diff --stat` 24+/24−).
- `handoffs/TASK-1580-buildmaster.md` (this file, untracked).

## 8. Named and left; held

- **HELD:** nothing. **NAMED-AND-LEFT:** nothing. No dirty path went unclaimed, and no `.uasset` / `.umap` was dirty.
- Not staged, by law: `CLAUDE.md` and `.claude/agents/*` (both clean).

## Not examined / limitations

- **No runtime path, compile or suite run was re-done.** 5a (`TASK-1578`) and 5b (`TASK-1579`) are taken from their reports. Source identity is carried by the sha256 equality above.
- **`TASKBOARD.md` hunks were placed by row, not re-derived line by line** against every manager amendment's text.
- **The harness arms were not re-run.** QA §4 item 2 says they were seen red, and I accepted that.
- **Clone behaviour of `-text`** was checked at this commit only (`check-attr`, blob hash). No fresh clone was made.
