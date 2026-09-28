# Handoff: TASK-1540 (build-master), commit G

- **Commit:** `133012a96454a164d5d52f198a3f50d39dc3cfb9` (`133012a`), parent `ab57522` (`TASK-1559`), 2026-09-28.
- **Kind:** CODE commit (C++ rode). No `.claude/agents/*`, no `CLAUDE.md`, no `Saved/**`, no `testvideo/**`, no `.claude/settings.local.json`.
- **Not pushed.** `origin/main` = `a35ba29` before and after. Ahead/behind: 0/7 before, **0/8 after**.
- **Size:** 34 files, +4532 / −133, verified on `git show --stat HEAD`, never the index.

## 1. Gates read at my instant

| Gate | Read |
|---|---|
| `TASK-1480` QA | `qa/TASK-1481-loop1.md` line 1 `PASS` (0/0/0). `qa/TASK-1481.md` line 1 `FAIL` is loop 0, superseded. |
| `TASK-1541` QA | `qa/TASK-1546-loop1.md` line 1 `PASS` (0/0/0). `qa/TASK-1546.md` line 1 `FAIL` is loop 0, superseded. |
| `TASK-1541` 5b | `qa/TASK-1547-verify.md` line 1 `Verdict: VERIFIED`. |
| 5a | `TASK-1538` `done`: `Result: Succeeded`, mutation arm red then green, 566/566 (`handoffs/TASK-1538-buildmaster.md`). |
| Reports | `qa/TASK-1499-verify.md` (VERIFIED), `qa/TASK-1404-verify.md` (MEASURED), `qa/TASK-1493-verify.md` (VERIFIED, H1 HOLDS) all on disk. |
| `TASK-1405` | `done — not yet`. |
| HEAD | `ab57522` (the delete's own commit is in). Index empty before staging. |

## 2. Byte identity (3), re-hashed before staging and checked again on the HEAD blobs

All under `Source/GitClaudeUnrealTest/Siegebound/`. `core.autocrlf=true` (system gitconfig), no `.gitattributes` rule for these files. LF files: HEAD blob sha256 = anchor. The two CRLF files: HEAD blob is LF, and blob re-expanded LF→CRLF = anchor (the worktree bytes).

| File | sha256 (worktree = anchor) | EOL | HEAD blob check |
|---|---|---|---|
| `SiegeControlsHelpWidget.cpp` | `a6bf281fd83d4db65461fe8a831e93b632faa1aca9af80ef6785b6cfbc235d8b` | LF | equal |
| `Tests/SiegeControlsHelpTest.cpp` | `525b5886779d1e346162f3d1e04e226ef69cfffab6ed6714705341385fc591d7` | LF | equal |
| `SiegeControlsHelpWidget.h` | `cc16d2caade4dab2a4676724ba9a2596b1f455d115560afe35f259941012d174` | LF | equal |
| `SiegeMenuInputSubsystem.cpp` | `53050739542a4eb8df72223839089c66bdb907f356ab15ea0dbe1aa17eb6aa63` | LF | equal |
| `SiegeMenuInputSubsystem.h` | `5bba1a035c40a39574223fe98f88130bf8fca9ded0385e27ecb4f81d1b68f2dc` | LF | equal |
| `Tests/SiegeMenuInputTest.cpp` | `5470c88605442b63bdbe60126d37dc33fd30b2a1e6fada7cb1661a451d85797d` | LF | equal |
| `DeckBuilderWidget.h` | `950dc811bbebc4b345eaa50cebd5f814f47262da990215d1a581ca8cfb6b1385` | CRLF | equal after LF→CRLF |
| `DeckBuilderWidget.cpp` | `de406a0ccd1a0bc9b665372a339fd85e9a6ebf9180adf0e09a17d31cd938934e` | CRLF | equal after LF→CRLF |
| `SiegePlayerController.cpp` | `8f33a5de3c8e06fe39ef3a484756d93f40facd8c0cadaea81116d40078db7fb4` | LF | equal |

- mtimes: latest source is `SiegeControlsHelpWidget.cpp` at 2026-09-27 23:44:52 (the arm's restore). All 9 precede the `TASK-1538` DLL, 23:45:07.
- `TASK-1480` and `TASK-1541` share the widget file and shipped together. The widget is `TASK-1541`'s loop-1 bytes `a6bf281f…`, never loop 0's `ada3609f…`.
- `Tests/SiegeControlsHelpTest.cpp` rode because a pin moved (`handoffs/TASK-1541-programmer.md` "MOVED PIN (1)", `:1818-1819`).

## 3. Read-backs (4)

**`CONVENTIONS.md`: 2 hunks, both named. No foreign hunk.**
- `@@ -7202 +7203`: `DECK-§4(c)` scope note, marker `DECK-4C-SCOPED-TO-PLAY-2026-09-27`. Named on `TASK-1551`'s status line.
- `@@ -12519 +12521,7`: `VER-§12` cl. 7f, marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS` (named on `TASK-1539`'s status line), with `TASK-1551`'s in-sequence label `VER-12-7F-CALL-FORM-SIGHTING-2026-09-27` inside it (named on `TASK-1551`'s status line).
- `TASK-1405` wrote no byte ("No `CONVENTIONS.md` byte moved"), consistent with the diff.

**`TASKBOARD.md`: 50 hunks at derive time, every one inside a row or block the spec or an amendment names.**
- Spec (4): rows `1402` / `1404` / `1405` / `1413` / `1421` / `1427` / `1480` / `1481` / `1493` / `1494` / `1498` / `1499` / `1500`.
- Amendment 1: `1436` / `1489` / `1501` (append lines) · `1439` / `1443` / `1445`.
- Amendment 2: the `BP-PREMISE-SETTLED-2026-09-24` block's new paragraph (line 6098, marker `ONKEYDOWN-CONTROL-FIRED-APPENDS-2026-09-27`) · `1399` · `1442` · `1444` · `1468`.
- Amendment 3: `1440`.
- Commit F's lag: `1519` / `1532` / `1533` / `1534` / `1535` / `1536` status flips.
- One 629-line insertion (new lines 9345–9973): the K5 block (with `K5-ORDER-AMENDED`, `-2`, `-3`) and rows `1537`–`1565`, all new since F. That includes `TASK-1559`'s own post-commit flip (`done — COMMITTED ab57522`), which is the delete's one-cycle lag.
- No hunk outside a row or the named blocks, and no assignee-line hunk.

## 4. Evidence promotion owed by TASK-1547 (VER-§4 cl. 1, SC-§68)

Copied with `cp -p`, never moved. Sources remain in `Saved/AuraVerify/T1547/` (7 files, untouched; `Saved/**` not staged). The names are the ones `qa/TASK-1547-verify.md` § Evidence proposes. Check: source sha256 = target sha256 = LFS oid in `HEAD`, 7/7. `git lfs ls-files` lists all 7.

| Target `playtest-evidence/2026-09-28/` | Source `Saved/AuraVerify/T1547/` | sha256 = oid |
|---|---|---|
| `VER-TASK-1547-t00m39s-move-page-plain-words.png` | `p0_move_t39.26s_f44095.png` | `cc4e7275ff1eaddd122f00848e9d193e51aa94e5a4e6ddc5cd1f355f2ceed754` |
| `VER-TASK-1547-t00m42s-sprint-page-plain-words.png` | `p3_sprint_t42.97s_f44313.png` | `f1bd8daa454b8584afd808daebce2569df56199e7258be5c2ddfaeacb61b5346` |
| `VER-TASK-1547-t01m19s-attack-page-plain-words.png` | `p4_attack_t79.74s_f46452.png` | `ac74b5c6a2f95be0dbd0571ba8674459b5363aec5a879aefc87283fb489eb5bd` |
| `VER-TASK-1547-t01m24s-rally-page-plain-words.png` | `p5_rally_t84.12s_f46657.png` | `3b1a7f884cd35675cd84e378fca192d795566e480a87e9213d09dae05911859d` |
| `VER-TASK-1547-t02m03s-discard-page-whole-hand.png` | `p8_discard_t123.44s_f48922.png` | `c8fe679c876c98f35042ec4b3bf01db5549cc08b42d34a34d0f2cec6cae3f0b6` |
| `VER-TASK-1547-t02m26s-stack-upgrade-page-plain-words.png` | `p10_stackupgrade_t146.15s_f50135.png` | `2703956e83254bb83dc3786ed441aad1515793dc6016744e4b70898fce87eb7e` |
| `VER-TASK-1547-t03m00s-map-marks-page-plain-words.png` | `p25_mapmarks_t180.94s_f52038.png` | `52f5f12758711813a8057be0e12ba5a576beed050d293b43d28dafff6e435991` |

No other promotion was owed. `qa/TASK-1499-verify.md` promotes none. `qa/TASK-1404-verify.md` captured none. `qa/TASK-1493-verify.md` says "Nothing promoted" (its pixel-null pair stays in `Saved/`).

## 5. What shipped (`git show --stat HEAD`, 34 paths, under `GitClaudeUnrealTest/`)

- `Source/GitClaudeUnrealTest/Siegebound/`: `DeckBuilderWidget.cpp` · `DeckBuilderWidget.h` · `SiegeControlsHelpWidget.cpp` · `SiegeControlsHelpWidget.h` · `SiegeMenuInputSubsystem.cpp` · `SiegeMenuInputSubsystem.h` · `SiegePlayerController.cpp` · `Tests/SiegeControlsHelpTest.cpp` · `Tests/SiegeMenuInputTest.cpp`
- `.claude/pipeline/`: `CONVENTIONS.md` · `TASKBOARD.md`
- `.claude/pipeline/handoffs/`: `TASK-1439-programmer.md` · `TASK-1443-programmer.md` · `TASK-1445-programmer.md` · `TASK-1480-programmer.md` · `TASK-1536-buildmaster.md` · `TASK-1538-buildmaster.md` · `TASK-1541-programmer.md` · `TASK-1559-buildmaster.md`
- `.claude/pipeline/qa/`: `TASK-1404-verify.md` · `TASK-1481.md` · `TASK-1481-loop1.md` · `TASK-1493-verify.md` · `TASK-1499-verify.md` · `TASK-1546.md` · `TASK-1546-loop1.md` · `TASK-1547-verify.md`
- `.claude/pipeline/playtest-evidence/2026-09-28/`: the 7 `VER-TASK-1547-*.png` in §4

The index held exactly these 34 paths right before `git commit -F <msg> -- <34 paths>`. Nothing else was staged, and the index is empty after the commit.

## 6. Named and left (not G's)

For commit H (`TASK-1550`), per the dispatch and `TASK-1540-AMENDED-2`: `.claude/agents/playtest-verifier.md` · `Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md` · `RCP-deckbuilder-slot-and-card-edit.md` · `RCP-menu-to-deckbuilder.md` · `RCP-vsbot-capture-center-and-summon.md` · `README.md` · `handoffs/TASK-1548-programmer.md` · `handoffs/TASK-1554-programmer.md` · `qa/TASK-1549.md` · `qa/TASK-1549-loop1.md` · `qa/TASK-1555.md`.

- **HELD:** nothing. No dirty path went unclaimed, and no `.uasset` / `.umap` was dirty.
- `qa/TASK-1545-verify.md` has not landed, so `TASK-1545` stays a reference only.

## 7. Board flips (SC-§103), written after the commit

- `1480` `built` → `done` + COMMITTED
- `1481` and `1546`: reports COMMITTED (loop 0 + loop 1 each)
- `1538` → COMMITTED
- `1541` `verified` → `done` + COMMITTED, verdict text kept
- `1547` → `done` + COMMITTED, verdict `VERIFIED` kept, promotion marked DISCHARGED
- `1404` / `1493` / `1499` → `done` + COMMITTED, verdict text kept
- `1405` / `1537` / `1539` / `1542` / `1543` / `1551` / `1440` / `1556` / `1439` / `1443` / `1445`: `— COMMITTED 133012a (host TASK-1540)` appended
- `1540` → `done` + hash

References, not flipped: `1536` (its lag rode) · `1402` / `1413` / `1421` / `1427` / `1494` / `1498` / `1500` · `1544` · `1545` · `1548`–`1550` · `1552`–`1555` · `1557`–`1565`.

## 8. One-cycle lag: the next host must carry these

- `TASKBOARD.md`: the 21 status-line flips in §7. They sit on 21 lines and nothing else moved; `git diff --stat` shows 21+/21−.
- `handoffs/TASK-1540-buildmaster.md` (this file, untracked).

## 9. Still dirty after G (at hand-off)

- ` M .claude/agents/playtest-verifier.md` (H)
- ` M .claude/pipeline/TASKBOARD.md` (this row's flips; the lag)
- ` M Tools/Verify/recipes/` ×5 (H)
- `?? .claude/pipeline/handoffs/TASK-1540-buildmaster.md` (the lag)
- `?? .claude/pipeline/handoffs/TASK-1548-programmer.md` · `TASK-1554-programmer.md` (H)
- `?? .claude/pipeline/qa/TASK-1549.md` · `TASK-1549-loop1.md` · `TASK-1555.md` (H)

## 10. Findings for the manager (reported here, not boarded)

1. **`qa/TASK-1547-verify.md` § Evidence now reads stale.** It still says "Nothing below exists under `playtest-evidence/` yet". The frames exist now. The discharge is recorded on `TASK-1547`'s status line, and I did not edit the verifier's report. If the report should carry a dated append, that is the manager's call.
2. **CRLF files and the byte-identity law.** `(3)` says the staged blob equals the after-sha256. Under `core.autocrlf=true` that holds literally only for LF files. For the two CRLF `DeckBuilderWidget` files, the blob is the LF-normalized form, and it equals the anchor only after LF→CRLF re-expansion. I verified both ways. A clause that says "staged blob, or its CRLF re-expansion for a CRLF worktree file" would stop a later host from reading this as a mismatch.
3. **`TASK-1480` → `done`** closes K5's code lane. `TASK-1560` (help-prose follow-through) is now unblocked by its "after G" condition, per `K5-ORDER-AMENDED-3` step 8.
