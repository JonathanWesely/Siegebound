# QA Report — TASK-1233 (gate over TASK-1218)
subject: TASK-1218 · marker `TASK-1233-AURA-GATE-1218` · 2026-09-13 · qa-reviewer
Verdict: PASS

Instruments: `Read` + `Grep` only (no `Bash`, `SC-§71b`; no `unreal_inspector` — nothing in scope is engine-side, the subject is a Markdown digest). Subject `Docs/AuraProjectMemory.md` read in full; sources read at my instant: `.claude/pipeline/CONVENTIONS.md:5-32` + `:52-55`, `CLAUDE.md:9-15` + `:75`, board rows `TASK-1218` (:2392-2401) and `TASK-1233` (:2558-2566), handoff `handoffs/TASK-1218-programmer.md`.

## Acceptance, measured

| # | Criterion | Measurement | Result |
|---|-----------|-------------|--------|
| 1 | ≤150 lines | `Read` numbers the last content line **61** (`Never run Git.`); line 62 is the trailing newline. Matches the handoff's `wc -l` = 61. | PASS |
| 2 | Three sentences character-exact (`SC-§38a`, by `Read`) | Lines 59 / 60 / 61, each a whole line, U+2014 em-dashes present: `Never compile via Live Coding — compilation belongs to build-master via Build.bat.` · `Never write generated meshes into Content/ — they enter through Tools/ArtPipeline Stage 2.` · `Never run Git.` — compared character-for-character against the TASK-1218 row spec (:2398) and the dispatch. | PASS |
| 3 | Prefix / suffix rows = `CONVENTIONS.md` at my instant | Asset table: header + rule + **21 rows** (`BP_` … `IMC_`), `AuraProjectMemory.md:21-43` vs `CONVENTIONS.md:7-29` — every cell identical, including the padded prefix column and the `DA_` / `LS_` / `SKEL_` / `IK_` / `RTG_` parentheticals. Texture-suffix line `:47` vs `CONVENTIONS.md:32` — identical, including `(LINEAR — sRGB off; added 2026-07-07, TRELLIS pipeline)`. Both section names cited by title. Zero drift. | PASS |
| 4 | Canonical-copy header | Line 3: `CANONICAL COPY. \`Tools/aura_sync.ps1\` regenerates \`Saved/.Aura/project_memory.txt\` from this file; edit here, never under \`Saved/\`.` — both names present. | PASS |
| 5 | No secrets / keys / tokens / tokenised URLs | Full `Read` shows only local `C:/…` build paths and `Content/…` folders. Confirming `Grep` `(?i)https?://|token|secret|api[_-]?key|password|bearer|sk-|hf_|eyJ` → zero hits. | PASS |
| 6 | Seven-agent table = `CLAUDE.md` + `playtest-verifier` | `AuraProjectMemory.md:9-15` vs `CLAUDE.md:9-15` (which at my instant ALREADY carries the seventh row — `TASK-1225` has landed): all seven rows identical cell-for-cell; the `playtest-verifier` row equals the row-spec string exactly. | PASS |
| — | `Build.bat` command verbatim | `:54` vs `CLAUDE.md:75` — identical. | PASS |
| — | Nothing under `Saved/` | Not measurable from text by this reviewer; the handoff's `ls` states no `project_memory.txt` exists. Accepted as declared (`SC-§71b`); `TASK-1240` host re-measures. | declared |

## Findings

- [WARN] `Docs/AuraProjectMemory.md:49` — un-boarded line (not in the TASK-1218 row spec): `C++ (Source/GitClaudeUnrealTest/): \`A\` actors, \`U\` UObjects/components, \`F\` structs, \`E\` enums, \`I\` interfaces; one class per header/cpp pair.` Ruling, as asked: **harmless and accurate — NOT a blocker.** The prefix list is character-faithful to `CONVENTIONS.md:53` and "one class per header/cpp pair" is `CONVENTIONS.md:54`. What it drops is the `:55` exception (pure data types — enums, structs, UInterfaces — may share a header-only file when they form one concept, e.g. `TeamId.h`), so Aura could answer "one class per pair" about a file that legitimately holds three. Suggested fix (a later row, not this gate): append `(pure data types may share a header-only file, e.g. TeamId.h)` or delete the line — either is a one-line edit touching nothing else. Scope note: the programmer flagged the addition in the handoff §2.5 / §3 rather than sneaking it; that is the right posture.
- [NIT] `Docs/AuraProjectMemory.md:17` — `Pipeline files: …` orientation line, also outside the row's enumerated content. Accurate (`TASKBOARD.md`, `CONVENTIONS.md`, `handoffs/`, `qa/` are the real hub files). Keep or drop at the host's discretion; it costs one of the 150 lines.
- [NIT] `Docs/AuraProjectMemory.md:51` — section header adds `(build-master only; the editor must be CLOSED)`. Accurate to the house build law and to the team table two sections up; not in the spec, not harmful.

Blockers: 0 · Warns: 1 · Nits: 2.

## Notes for build-master / host (TASK-1240)
- File is 61 lines; the budget has 89 lines of headroom, so the WARN's suggested one-line amendment fits.
- `CLAUDE.md` already carries the `playtest-verifier` row at my instant, so the handoff §3 worry ("the two must agree when both land") is discharged — they agree now. Re-diff at host time anyway (`SC-§68`-style), since both files are live edit targets in this wave.
- `Saved/.Aura/project_memory.txt` is `TASK-1219`'s copy step; nothing here authorises staging anything under `Saved/` (standing exclusion registry).
- The handoff names a pre-existing ` M GitClaudeUnrealTest/.mcp.json` dirt that is NOT this row's — `TASK-1221` territory; do not attribute it to 1218.
