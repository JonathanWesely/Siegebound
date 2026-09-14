# TASK-1244 — [AURA-INDEX-IGNORE-2] — programmer handoff (2026-09-13)

**Status:** ready-for-qa · gate `TASK-1250` · host `TASK-1253` · law: ruling R3 (a)–(d) · `SC-§38a` · `SC-§100`

## What was written
- `Docs/AuraIndexIgnore.txt` rev 2 — **146 → 148 lines · 102 → 104 patterns** (rev 1 = `11e9ea2`).
- This handoff.
- `.claude/pipeline/TASKBOARD.md` — ONLY the `- status:` line of this row (`backlog` → `ready-for-qa`), anchored on the row marker, asserted `backlog` before the write.
- ⛔ **Nothing under `Saved/` was written, read for input, or touched.** `git status --porcelain -- Saved` → empty (quoted in acceptance (6)). The host (`TASK-1253`) re-syncs via `Tools/aura_sync.ps1`.

## The four edits (R3) — hunk list, `git diff -U0 HEAD -- Docs/AuraIndexIgnore.txt`, 5 hunks, 8 insertions / 6 deletions
```
@@ -7 +7 @@
-# ---- Plan item 2 excludes (22) --------------------------------------------------------
+# ---- Plan item 2 excludes (22 plan + 4 house) ------------------------------------------
@@ -17,0 +18,5 @@ Content/Prickly_Knight/
+# house additions (TASK-1244): vendor packs not in plan item 2
+Content/Fire_Magic/
+Content/Ice_Magic/
+Content/IceAttack/
+Content/MedievalWeaponsSFX/
@@ -33,3 +37,0 @@ Tools/ArtPipeline/Cache/
-# packagedZIPofGame/ lives at the GIT root (one level above the project root), so this line is
-# project-root-relative and matches nothing here; kept verbatim for parity with plan item 2.
-packagedZIPofGame/
@@ -58 +60,2 @@ Content/Art/
-Config/
+# gitignored cloud ini (publishable key) — never indexed; the rest of Config/ (DefaultInput.ini etc.) IS indexed
+Config/SiegeCloudDev.ini
@@ -83 +85,0 @@ Plugins/node_modules/
-**/*.Target.cs
```
Mapping to the spec: hunk 1 = the header count comment · hunk 2 = (a) the four packs + the "house additions" count comment · hunk 3 = (d) · hunk 4 = (c) · hunk 5 = (b). Five hunks = the four edits + the two count comments (the second count comment rides inside hunk 2). Nothing else moved; the file's own `Source: TASK-1217` header line (line 5) was left as-is per "⛔ Nothing else moves".

Note on the dash: the (c) comment uses an em-dash `—` verbatim from the spec; the file already carried one (line 39 header "`.claude/ — NOT ignored wholesale`") in rev 1, so the encoding is not new to the file. File remains UTF-8, LF (git's CRLF warning is the repo's autocrlf notice, same as rev 1).

## Acceptance, item by item (all MEASURED on the edited file)
1. **Four new lines present verbatim, by Read** — read back lines 18–22 in full: `# house additions (TASK-1244): vendor packs not in plan item 2` / `Content/Fire_Magic/` / `Content/Ice_Magic/` / `Content/IceAttack/` / `Content/MedievalWeaponsSFX/`. Header line 7 reads `# ---- Plan item 2 excludes (22 plan + 4 house) ---…`. On disk today those four folders hold 255 + 226 + 51 + 1,368 = **1,900 files** (`find -type f | wc -l`; manager's glob said 1,897, QA said ~1,900 — same class, the 3-file delta is `find` counting non-asset files).
2. **`grep -c 'Target.cs' Docs/AuraIndexIgnore.txt` → `0`.** The two files it used to reach, `Source/GitClaudeUnrealTest.Target.cs` and `Source/GitClaudeUnrealTestEditor.Target.cs`, are now indexed (they are the ONLY `.Target.cs` on disk — measured).
3. **Bare `Config/` ABSENT** — `grep -cx 'Config/'` → `0` (exact-line match). **`Config/SiegeCloudDev.ini` PRESENT** at line 61. `Config/SiegeCloudDev.ini` IS gitignored: `git check-ignore -v` → `GitClaudeUnrealTest/.gitignore:63:Config/SiegeCloudDev.ini`. **`Config/DefaultInput.ini` is unmatched by every remaining pattern — reasoning below, and mechanically checked.**
4. **`grep -c 'packagedZIPofGame' Docs/AuraIndexIgnore.txt` → `0`.**
5. **Hunk list** — quoted above; `git diff -U0 … | grep -c '^@@'` → `5`.
6. **`git status --porcelain -- Saved`** → (empty output).

## Keep-set re-verification (acceptance (3) reasoning + the file's own no-negation law)
**Mechanical check first** (scratchpad `keepset_check.py`, Python 3.14): every one of the 104 patterns run against every REAL file under `Source/`, `Content/{Blueprints,UI,Data,Maps,Materials,VFX,Input,Meshes,Textures,Characters}`, `Docs/GDD.md`, `.claude/pipeline/CONVENTIONS.md`, plus all of `Config/` — **852 files** — under BOTH candidate semantics Aura's docs leave open (A: root-anchored, globs fnmatch'd on the full path with `*` crossing `/`, i.e. the permissive worst case; B: gitignore-style any-depth for slash-free names, `**` across segments, `*.ext` on the basename). Result:

```
patterns tested: 104   keep-set + Config files walked: 852
total (pattern,path) hits under either semantics: 1
  line 61: Config/SiegeCloudDev.ini -> Config/SiegeCloudDev.ini  [AB]
Config/ files and their fate:
  indexed   Config/DefaultEditor.ini
  indexed   Config/DefaultEditorPerProjectUserSettings.ini
  indexed   Config/DefaultEngine.ini
  indexed   Config/DefaultGame.ini
  indexed   Config/DefaultInput.ini
  EXCLUDED  Config/SiegeCloudDev.ini
  indexed   Config/SiegeCloudDev.ini.example
RESULT: PASS
```
Rev 1 under the same check had **2** keep-set hits (the `.Target.cs` pair, QA's WARN-1). Rev 2 has **0** keep-set hits; the one hit is the intended, non-keep-set exclusion.

**Reasoning per pattern class, for `Config/DefaultInput.ini` specifically** (why the mechanical result is not an accident of today's tree):
- *Patterns with an internal slash* (`Content/...`, `.claude/...`, `Tools/ArtPipeline/...`, `Plugins/...`): root-anchored under either semantics; none begins with `Config/` except line 61, and line 61 is a bare FILE path that matches only the exact string `Config/SiegeCloudDev.ini` — not `DefaultInput.ini`, and not the `.example` (a bare file line has no wildcard; `Config/SiegeCloudDev.ini` is not a prefix-match rule, and `.example` differs in the final segment). Precedent for a bare file line as an accepted shape: `.claude/pipeline/TASKBOARD.md` at line 41.
- *Bare folder names* (`Models/ Intermediate/ Saved/ Binaries/ DerivedDataCache/ Build/ Builds/ .vs/ .vscode/ .idea/ .tmp/ .agents/ .git/ .svn/ .plastic/ __pycache__/ env/ build/ lib/ lib64/ var/ dist/ parts/ sdist/ downloads/ eggs/ .eggs/ develop-eggs/ *.egg-info/`): under any-depth semantics they match a DIRECTORY of that name; `Config/DefaultInput.ini` has one directory segment, `Config`, which is none of them. The former `Config/` line was the sole pattern naming that segment and it is gone.
- *`**/dir/**` and `**token**` globs* (`**/Intermediate/**`, `**/Binaries/**`, `**/.tmp/**`, `**/.agents/**`, `**/.git/**`, `**__ExternalActors__**`, `**__ExternalObjects__**`): the path contains none of those tokens.
- *Extension globs* (`*.generated.h/.cpp`, `*.user`, `*.so`, `*.egg`, `*.sdf`, `*.suo`, `*.sln`, `*.slnx`, `*.VC.db`, `*.VC.opendb`, `*.py[cod]`, `*$py.class`, `*.userosscache`, `*.opensdf`, `*.sln.docstates`): no `*.ini` glob exists anywhere in the file; `.ini` matches none of these suffixes.
- *Bare file names* (`**/TASKBOARD.md`, `UpgradeLog.htm`, `Thumbs.db`, `.DS_Store`, `.gitignore`, `.gitattributes`, `ignore.conf`, `.installed.cfg`, `.Python`, `.vsconfig`): basename `DefaultInput.ini` equals none.
Hence `Config/DefaultInput.ini` (and `DefaultEngine.ini`, `DefaultGame.ini`, `DefaultEditor*.ini`) enters the index; `KBD-§`'s Enhanced-Input answer is now readable from the index, which was NIT-3's ask.

**One consequence QA should see and I checked:** un-ignoring `Config/` also indexes `Config/SiegeCloudDev.ini.example`. It is TRACKED (`git ls-files`), it is the ACC-§11 placeholder template, and it carries `ProjectUrl="https://<project-ref>.supabase.co"` / `AnonKey=<anon-or-publishable-key>` — placeholders only. Measured: `grep -cE 'eyJ[A-Za-z0-9_-]{20,}|sb_publishable_…|sb_secret_'` → `0`. No key enters the index. If a reviewer prefers belt-and-braces, a second bare line `Config/SiegeCloudDev.ini.example` would be a one-line follow-up; I did NOT add it because R3 says "by name" for the ONE gitignored file and "⛔ Nothing else moves".

## The four R3 edits, why (one line each, from the ruling)
- (a) The four packs are vendor content of the same class as the plan's `sA_*` excludes; Aura reasons about our call sites in `Source/` + `Content/Blueprints`, not a pack's emitters.
- (b) `**/*.Target.cs` reached the only two keep-set files any pattern touched; acceptance (2) of TASK-1217 says NONE.
- (c) `Config/` back in so `DefaultInput.ini` is visible to a verifier; the cloud ini stays out BY NAME because a key never enters an index, publishable or not (training-ON toggle may move an index off the machine).
- (d) A no-op line is one a future reader will "fix" by making it match something.

## For QA (`TASK-1250`) — what to scrutinize
- Re-run the greps in acceptance (2)/(3)/(4) yourself; they are one-liners.
- Read lines 18–22 and 60–61 in full (`SC-§38a`), not via grep.
- The `.example` indexing consequence above — agree it is placeholder-only, or ask the manager for the one-line follow-up.
- Semantics caveat unchanged from rev 1: Aura's docs still do not state whether a slash-free folder line is root-anchored or any-depth; the mechanical check passed under BOTH, so the question cannot bite today.

## For the host (`TASK-1253`)
- Commit `Docs/AuraIndexIgnore.txt` + this handoff + QA's report by pathspec (git root is ONE LEVEL UP — `SC-§102`). ⛔ Nothing under `Saved/` is staged or tracked; the live `Saved/.Aura/INDEX_IGNORE.txt` is now BEHIND the canonical by these 5 hunks until `Tools/aura_sync.ps1` is re-run — that re-sync is the host's step, not mine.

## Slack
One post in ⚙️ Dev & QA (`C0BF0QZP3CN`, thread `1783116269.740549`), ts `1789363371.545949`: the grep/absence lines, counts before/after, the matcher result.
