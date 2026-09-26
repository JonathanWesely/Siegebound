# TASK-1437 — build-master handoff (WAVE-D2-COMMIT-HOST, and the milestone's last host)

**Commit `4620f710dff3ac7afd3a02ea0264d7c17b1201b5` (`4620f71`) · parent `%P` = `40c824bb26d6edb0f17d54f5819b12210bc50b74` · 91 files, +22,249 / −249 · LOCAL ONLY, NOT PUSHED.**
Marker `MENU-NAV-PARITY-COMMITTED-2026-09-25`. No code authored, no asset authored, no compile, no PIE, no editor lifecycle action.

## 1. Host reconciliation — ONE commit, not four

Four hosts were boarded (`TASK-1414` B · `TASK-1422` C · `TASK-1428` D1 · `TASK-1437` D2),
chained so each one's `blocked-by` asserts its predecessor is ALREADY COMMITTED. **None had run**,
so all three of those assertions were false at my instant.

**The split is impossible, and that is measured rather than argued.** The waves share source files:

| shared path | waves carried together |
|---|---|
| `SiegeMenuInputSubsystem.cpp` | **B + C + D1 + D2** (1406/1407/1408/1409/1410/1413 · 1418 · 1425/1426/1427 · 1429/1469/1471/1474) |
| `SiegeMenuInputSubsystem.h` | B + C + D1 + D2 |
| `DeckBuilderWidget.cpp/.h` | B + C + D1 + D2 |
| `SiegeControlsHelpWidget.cpp/.h` | B + C + D2 |
| `SiegeGraphicsMenuWidget.cpp/.h` | B + C |
| `Tests/SiegeMenuInputTest.cpp` | B + D2 |

No pathspec separates them. Any split attributes one wave's bytes to another wave's hash — the exact
stale-passenger shape `SC-§103` exists to prevent. `TASK-1414` / `1422` / `1428` are flipped
**SUBSUMED** (not done-by-own-commit), each row inviting the manager to re-scope or re-open.

## 2. Derived cargo vs the recorded figure

`TASK-1437`'s figure has read **8 → 13 → 17 → 21 → 23** and is advisory by standing rule.

- **Derived: 47 rows** with a named deliverable in this commit (from staged filenames + `TASK-1467` per M3).
- **Recorded: 41** across all four hosts' combined cargo.
- **Disagreement: +9 / −3.**
  - +9 recorded by no host: `1395` `1398` `1453` `1459` `1461` `1463` `1464` `1467` `1484`
  - −3 recorded but produced **no bytes**: `1473` `1480` `1481`

**34 rows flipped** in the same action as the commit; 16 further cargo rows were already `done`.
`TASK-1398` deliberately NOT flipped — per M3 its handoff owes `TASK-1463` + `TASK-1467`, not itself.

## 3. ESCALATION — amendment A1(ii)'s premise is wrong

A1(ii) told this host that `Content/UI/WBP_SessionMenu.uasset` (`TASK-1473`) ships this wave, and that an
identical digest means the flip never landed ⇒ HOLD + ESCALATE.

**Measured: the file is CLEAN, last touched at `9330a1b` (TASK-355), long before this milestone.
`TASK-1473` is `backlog` — never dispatched.** There is no failed flip to hold; the row simply never ran.
Escalated rather than absorbed. **`TASK-1473` is not cargo and was not flipped.**

A1(i) is confirmed correct: `WBP_VictoryScreen.uasset` clean at `1d433ca` — identical digest expected, not a hold.

## 4. Binary digest table — oid vs sha256, never size (`SC-§68`)

**25 binaries: 25 OK / 0 mismatch** (21 evidence PNGs + 4 `.uasset`; all LFS).

| asset | index oid (sha256) | worktree sha256 | match |
|---|---|---|---|
| `IA_MenuBack.uasset` | `ef78c0511285d7d3…` | `ef78c0511285d7d3…` | OK |
| `IA_MenuLeft.uasset` | `7ae96d2272ad16f6…` | `7ae96d2272ad16f6…` | OK |
| `IA_MenuRight.uasset` | `3fe8abbebb679713…` | `3fe8abbebb679713…` | OK |
| `IMC_MainMenu.uasset` | `6d09ac2541042612…` | `6d09ac2541042612…` | OK |

`IMC_MainMenu.uasset` at `HEAD~1` = `c0da00d3a5b5de47…` ⇒ **`c0da00d3…` → `6d09ac25…`, CHANGED** —
independently corroborating **both halves** of `TASK-1408`'s own declaration from git rather than from its say-so.

## 5. Deletions verified BY CONTENT

`--histogram`: **+7,287 / −69 over 18 source files**, confirmed *from the commit*.
(`--numstat` deletion counts are algorithm-dependent and were not relied on.)
All 69 deleted lines read: comment re-anchoring · one **replaced** log literal (`TASK-1463`'s gated subject) ·
one comment **re-tensed into history** · removal of the `ApplyButtonNotFocusable(...)` calls — which **is the
milestone**, those buttons being precisely the ones now reachable. No silent removals.

The `TASK-1483` hunk-3 byte-identity debt was **already discharged by the orchestrator** and was not redone.

## 6. Milestone ledger — resolved from git, not from the board

Of `TASK-1398` … `TASK-1437`: **33 have a deliverable tracked at `HEAD`.** Seven do not:

- `1403` — `done — unnecessary`, cargo absorbed by Jonathan's `40c824b`. Correct.
- `1404`, `1405` — `backlog`, never dispatched (`1405` conditional). **Genuinely open.**
- `1414`, `1422`, `1428` — subsumed hosts; no handoff by design.
- `1437` — this file, the declared tail.

## 7. Declared tail (`TL-§5e` cl. 7d(ii))

`.claude/pipeline/handoffs/TASK-1437-buildmaster.md` — this file, uncommitted, because it carries the hash
of the commit it describes. **No second commit was invented to swallow it.**

## 8. Gates

- Pathspec: **91 paths**, explicit, anchored one level up at `C:/GitProjects/GitHub/GitClaudeUnrealTesting`. No `-A`, no `.`, no bare directory, no `--allow-empty`, no amend.
- Exclusions: `testvideo/` root-gitignored (nothing present); `CLAUDE.md` / `CONVENTIONS.md` / `SLACK.md` all **clean**, nothing withheld; `TASK-1411`'s `FogVolume.cpp` already in `HEAD` at `40c824b` — clean file, commits nothing, and that silence is **not** a mis-anchored pathspec (`SC-§102`).
- Secret scan across every dirty text file: **clean**. One regex hit was the literal Python kwarg `hf_token=token` in a 2026-07-07 board line already in `HEAD` — a parameter name, not a value.
- **Push: NONE.** `origin/main` unchanged at `40c824b`. Ahead-count **0 → 1**.
- Editor censused by command line (`SC-§118`): **PID 24652**, GUI `UnrealEditor.exe` on this project, sole instance. **Left UP, untouched.**
- Verified from **the commit** (`git show --stat HEAD`), never the index — the UE Git plugin auto-stages on save.

## 9. Runtime honesty carried into the message

**Eight of ten** screens carry `VERIFIED`. Controls help is `VERIFIED` for navigation, `MEASURED` for scroll
(`TASK-1484`, gate at `.claude/pipeline/qa/TASK-1484-gate.md` — **no numbered `qa/TASK-14xx.md` row exists**).
The assistant console is `MEASURED` (`TASK-1434`). Both `MEASURED` verdicts are **instrument limits, not defects**,
and neither was laundered into a pass on any row.
