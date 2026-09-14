# Handoff — TASK-1245 [AURA-PROJECT-MEMORY-2] — gameplay-programmer
marker `TASK-1245-AURA-PROJECT-MEMORY-2` · 2026-09-13 · answers ruling R6 (from `qa/TASK-1233-report.md` WARN) · gate: `TASK-1251` · host: `TASK-1253`

## 1. What changed — ONE line

File: `Docs/AuraProjectMemory.md`, line 49. Nothing else in the file, nothing under `Saved/`, no compile, no Git.

**Before (line 49):**
```
C++ (Source/GitClaudeUnrealTest/): `A` actors, `U` UObjects/components, `F` structs, `E` enums, `I` interfaces; one class per header/cpp pair.
```

**After (line 49):**
```
C++ (Source/GitClaudeUnrealTest/): `A` actors, `U` UObjects/components, `F` structs, `E` enums, `I` interfaces; one class per header/cpp pair; exception: pure data types (enums, structs, UInterfaces) may share a header-only file when they form one concept (e.g., `TeamId.h`).
```

The trailing period moved from after `pair` to the end of the appended clause, so the line stays one sentence — the spec says the clause is appended *after* `one class per header/cpp pair`, which puts it before the period.

## 2. Acceptance, measured

| # | Criterion | Measurement | Result |
|---|-----------|-------------|--------|
| 1 | Amended line quoted in full; its three clauses = `CONVENTIONS.md` § "C++ (Source/GitClaudeUnrealTest/)" three bullets at my instant (`SC-§38a`, `Read`) | Quoted above. `CONVENTIONS.md:59-62` read by `Read` (not Grep): `:60` `Classes: \`A\` actors, \`U\` UObjects/components, \`F\` structs, \`E\` enums, \`I\` interfaces` → clause 1 identical after the `Classes:` label. `:61` `One class per header/cpp pair; file name = class name without prefix` → clause 2 carries the first half (the pre-existing line never carried the file-name half; not in this row's spec, not added). `:62` `Exception: pure data types (enums, structs, UInterfaces) may share a header-only file when they form one concept (e.g., \`TeamId.h\`)` → clause 3 identical after lower-casing the leading `Exception:` to follow a semicolon, exactly as the row spec's append text writes it. | PASS |
| 2 | Line count ≤150 | `wc -l Docs/AuraProjectMemory.md` = **61** (unchanged from `qa/TASK-1233` §1 — a longer line, not a new one). | PASS |
| 3 | Hunk count = 1 | `git diff -U0 -- Docs/AuraProjectMemory.md \| grep -c '^@@'` = **1**; `git diff --stat` = `1 file changed, 1 insertion(+), 1 deletion(-)`; the hunk header is `@@ -49 +49 @@`. | PASS |
| 4 | `git status --porcelain -- Saved` empty | Output: `[]` (empty string, bracketed by the probe). | PASS |
| — | The three laws (`:59` / `:60` / `:61`) still character-exact | Re-read by `Read` after the edit: `Never compile via Live Coding — compilation belongs to build-master via Build.bat.` · `Never write generated meshes into Content/ — they enter through Tools/ArtPipeline Stage 2.` · `Never run Git.` — U+2014 em-dashes present, each a whole line, unchanged. | PASS |

## 3. Decisions QA should scrutinize

- **Backticks on `TeamId.h`.** The row spec quotes the append text inside a single code span, so whether `TeamId.h` itself carries backticks is not visible in the spec. I used `` `TeamId.h` `` because (a) the source bullet `CONVENTIONS.md:62` writes it that way and acceptance (1) binds the clause to that bullet, and (b) every other identifier on line 49 (`A`, `U`, `F`, `E`, `I`) is backticked. If the gate reads the spec as literal-no-backticks, the fix is two characters on the same line.
- **`Exception:` → `exception:`.** Lower-cased because the row spec's append text writes it lower-case after the semicolon; the source bullet capitalises it only because it opens a bullet.
- **⚠️ The dispatch's "three header-only files" claim is one-third wrong, and it does NOT change this edit.** The dispatch and ruling R6 name `TeamId.h`, `UnitCommand.h`, `SiegeAssistantCommand.h` as header-only. Measured under `Source/GitClaudeUnrealTest/Siegebound/`: `TeamId.h` and `UnitCommand.h` have no `.cpp` (header-only, correct); **`SiegeAssistantCommand.cpp` EXISTS** (JSON parsing, `DEFINE_LOG_CATEGORY(LogSiegeAssistant)`, ~13 type-keyword hits) — that header is a normal header/cpp pair whose header happens to hold several data types. The spec's append text names only `TeamId.h` as the example, so the file is correct as written; I am recording the discrepancy so the R6 prose is not copied forward as fact (`SC-§104`-style: the state is right, the tally in the ruling was not). No board edit — that would be a rescope (`SC-§100`) and is the manager's.
- **Files live in `Siegebound/`, not the module root.** All three are under `Source/GitClaudeUnrealTest/Siegebound/`; the memory line says `Source/GitClaudeUnrealTest/` as the module path, which is still the true module root. Not a defect, noted for anyone re-measuring.
- **Git's `LF will be replaced by CRLF` warning** on this file is autocrlf noise: the blob at `1ade367` is LF and the working copy after my edit is still LF (the Edit tool preserved line endings). Pre-existing, not introduced here, and the diff shows exactly one changed line, which it would not if endings had flipped.

## 4. Touched / not touched

- WRITES: `Docs/AuraProjectMemory.md` (line 49 only) · this handoff · `TASKBOARD.md` (`TASK-1245` `- status:` line only, via exact anchor).
- NOT touched: `Saved/**` (porcelain empty, quoted above) · `Tools/aura_sync.ps1` · `CONVENTIONS.md` · any `Source/` file · no compile, no Git commands beyond read-only `diff`/`status`.
- `Saved/.Aura/project_memory.txt` regeneration is `TASK-1253`'s (host) via `Tools/aura_sync.ps1`, not this row's.
