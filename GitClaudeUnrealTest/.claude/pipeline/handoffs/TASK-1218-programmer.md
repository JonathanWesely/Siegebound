# TASK-1218 — AURA-PROJECT-MEMORY — programmer handoff (2026-09-13)

marker `TASK-1218-AURA-PROJECT-MEMORY` · gate `TASK-1233` · host `TASK-1240` · law: plan item 3 · `SC-§38a`
Files written: `Docs/AuraProjectMemory.md` (NEW) · this handoff · the one `status:` line on my board row. Nothing else. No editor, no compile, no Git, nothing under `Saved/**`, `Source/**`, `Content/**`, `Config/**`, `Tools/**`.

## 1. Acceptance, measured

| # | Criterion | Instrument | Result |
|---|-----------|-----------|--------|
| 1 | ≤150 lines | `wc -l` and `grep -c ""` (agree; file ends in a newline) | **61 lines** |
| 2 | Three sentences character-exact | `Read` of lines 56–62 (not `Grep`, `SC-§38a`) | lines **59, 60, 61** — each on its own line, verbatim as dispatched (see §2) |
| 3 | Prefix rows = `CONVENTIONS.md` at my instant | `diff` of the 23-line block (`\| Prefix \| Type \| Folder \|` header + rule + 21 rows), grep-anchored: source starts `CONVENTIONS.md:7`, copy starts `AuraProjectMemory.md:21` | **IDENTICAL, zero diff**. Texture-suffix line diffed the same way: **IDENTICAL** |
| 4 | Canonical-copy header naming `Tools/aura_sync.ps1` → `Saved/.Aura/project_memory.txt` | `Read` line 3 | present |
| 5 | No secrets / keys / tokens / URLs-with-tokens | `grep -i -E "token\|secret\|api[_-]?key\|password\|https?://"` | **zero hits** (exit 1); the file's only path-like strings are local `C:/…` build paths and `Content/…` folders |
| 6 | Nothing under `Saved/` | `ls -la Saved/.Aura/` after the write | `CrashReportState.json`, `INDEX_IGNORE.txt`, `PythonRequirementsChecksum.txt`, `Skills/`, `project.json` — all timestamped 20:45–20:46 (Aura's own install, `TASK-1214`), before my session; **no `project_memory.txt` exists** — that is `TASK-1219`'s copy step |

Also diffed the `Build.bat` command line against `CLAUDE.md` (`grep -F` both, `diff`): **IDENTICAL**.

## 2. Content order (row spec, in order)

1. line 1 title · line 3 the canonical-copy header sentence
2. seven-agent table — six rows copied from `CLAUDE.md` + `playtest-verifier | Runs Aura PIE verification, writes runtime evidence | Editing code/art, compiling, Git`
3. `## Asset prefixes (Content Browser)` — cites the CONVENTIONS section by name, table copied byte-exact
4. `## Texture suffixes` — cites the section by name, line copied byte-exact
5. one line of C++ class-prefix convention (`A`/`U`/`F`/`E`/`I`, one class per pair) — from CONVENTIONS "C++" section, added because Aura's chat answers about classes; not in the row spec, ⛔ flag if QA reads it as scope creep — it is one line and deletable without touching anything else
6. the `Build.bat` command in a fenced block, verbatim
7. the three laws, one per line, character-exact:
   `Never compile via Live Coding — compilation belongs to build-master via Build.bat.`
   `Never write generated meshes into Content/ — they enter through Tools/ArtPipeline Stage 2.`
   `Never run Git.`

Note for QA on the em-dashes: the two long sentences carry U+2014 `—` exactly as in the dispatch, plan item 3, and doc §6 item 4. `Read` them; a `Grep` with a hyphen would not match and would file a false blocker (that is the `SC-§38a` case).

## 3. What QA should scrutinize

- Item 5 above (the one-line C++ prefix note) is the only content not enumerated by the row. Everything else is a copy.
- The team table: I copied `CLAUDE.md`'s six rows as they read at my instant; `CLAUDE.md` itself is a `TASK-1225` (plan item 9) edit target and will gain the same seventh row later — the two must agree when both land (`TASK-1240` host).
- `git status` at my instant showed a pre-existing ` M GitClaudeUnrealTest/.mcp.json` that is NOT mine (I ran no Git command beyond the read-only status; the dirt predates my write). Named so the host does not attribute it to this row.

## 4. Slack

⚙️ Dev & QA `1783116269.740549`, one post, `⚙️ PROGRAMMER: 🟡 TASK-1218` — line count 61, three laws verbatim: yes.
