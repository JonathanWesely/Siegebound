# TASK-1210 — README-SAVED-PATH-HOST — build-master handoff (2026-09-10, 00:29–00:40 PDT)

marker `TASK-1210-README-SAVED-PATH-HOST` · subject `TASK-1208` (source fix) gated by `TASK-1209` (PASS) · law `PKG-§7b` · `SHIP-§4` · `SHIP-§8d` · `SHIP-§3a` · `PKG-§10` · `SC-§91` · `SC-§102` · `SC-§103` · `SC-§106` · `SC-§118` — cited, not restated.

⛔ **This file was born OUTSIDE its own commit.** The commit it records is **`8d05679`** (`8d056793ceb65e0d36dac0dfb8dfaf3ab42722a1`); this handoff is written after it and rides the NEXT commit (`TASK-1180`'s shape, bounded at one, never amended). The three status flips (`1208` · `1209` · `1210` → `done (commit 8d05679)`) were made after the commit for the same reason — a board line cannot carry the hash of the commit that carries it — so `TASKBOARD.md` is dirty by exactly those three lines afterwards, plus this untracked file. The one-line note on `TASK-1193`'s row needed only the ZIP hash, so it was appended BEFORE the commit and rides `8d05679` (the struck hash is therefore already in git).

Route taken for Phase E: **(a)** — re-render of the gated source. Scripts and logs are out of tree, gitignored (`.gitignore:19`), and kept (`TL-§6`): `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260910-073200-readme-r2\` — `render_readme_r2.py` · `render.log` · `values.json` · `README.r1-to-r2.diff` · `README-source.c6fce42.md` (the revision-1 source, `git show c6fce42:…`) · `README.r1.md` (backup of the revision-1 rendered README, sha256 `7D0D904E…F083D`, 86,729 B) · `zip_stage_r2.ps1` · `zip.log` · `commit-msg.txt`.

## 1. `SC-§91` at my instant + the `SC-§118` cl. 6 hand classification (nothing closed, nothing to close)

- `HEAD` = `c6fce42671320f3c5f1e13d0f46740915a1413a2` (the ship's docs commit — expected) · `origin/main...main` = `0 2` (2 ahead, 0 behind, not pushed) · porcelain: `M CONVENTIONS.md` (the manager's +11) · `M TASKBOARD.md` (+122/−6: this pass's boarding + 1208/1209's flip lines) · `M Docs/Packaging/README-source.md` (1208's hunk) · `?? handoffs/TASK-1208-programmer.md` · `?? qa/TASK-1209-report.md`. **Index empty** (`git diff --cached --stat` printed nothing — the editor auto-stage trap did not fire; no editor has run). `handoffs/TASK-1193-buildmaster.md` NOT dirty ⇒ no cl. 7a sweep owed.
- **Transferred QA check (`qa/TASK-1209-report.md`, `SC-§71b`) EXECUTED:** `git diff --stat -- GitClaudeUnrealTest/Docs/Packaging/README-source.md` → **`1 file changed, 6 insertions(+), 3 deletions(-)`** (one hunk `@@ -168,7 +168,10 @@`). Mis-anchor control (`SC-§102`): `git diff --stat -- Docs/Packaging/README-source.md` from the git root printed NOTHING (the silent zero, demonstrated). Encoding: first bytes `3c 21 2d` (no BOM), CR count 0, LF; the `c6fce42` blob likewise (81,034 B, 0 CR). git's standing `LF will be replaced by CRLF` warning is its autocrlf config, not a change in the file.
- `git check-ignore -v` on `packagedZIPofGame/README.md`, the zip and the run dir → `.gitignore:19:packagedZIPofGame/` (`PKG-§3`).

```
CENSUS 2026-09-10 00:2x PDT (Get-CimInstance Win32_Process; names UnrealEditor* / GitClaudeUnrealTest* / *Win64-Shipping* / RunUAT* / UnrealPak* / Siegebound*)
CENSUS: zero matching processes
POSITIVE CONTROL (the instrument sees processes at all): pid=20032 name=powershell.exe
```
Zero instances of any class ⇒ the stage is not locked by 🧑 Jonathan's session, no question owed in 🚨, nothing closed (`SC-§118` cl. 6).

## 2. `SHIP-§8d` — the stage is UNCHANGED, by measurement (`Get-FileHash -Algorithm SHA256`, 2026-09-10 00:29:31 -07:00)

| File (under `packagedZIPofGame\Windows\`) | sha256 NOW | PART 4 §3 AFTER | bytes | mtime | Verdict |
|---|---|---|---|---|---|
| `GitClaudeUnrealTest\Binaries\Win64\GitClaudeUnrealTest-Win64-Shipping.exe` | `9965A35AE1F349F604837B642D6A4FEBC2E8EBE624802DBFEC559835B4FEFF2C` | `9965A35A…4FEFF2C` | 178,076,160 | 2026-09-09 22:08:31 | **MATCH** |
| `GitClaudeUnrealTest\Content\Paks\GitClaudeUnrealTest-Windows.pak` | `E0CCFFDECCCD1EB6F5E837CB02ACEB8706DCCFD6D8C6DB864003C277CAEE6464` | `E0CCFFDE…CAEE6464` | 11,466,608 | 2026-09-09 23:37:49 | **MATCH** |
| `GitClaudeUnrealTest\Content\Paks\GitClaudeUnrealTest-Windows.ucas` | `7DB7A5DDA0160AEB40512CEB994B61464409E9BB44F42608B36F2FBF529B0EFA` | `7DB7A5DD…529B0EFA` | 1,103,375,200 | 2026-09-09 23:37:54 | **MATCH** |
| `GitClaudeUnrealTest\Content\Paks\GitClaudeUnrealTest-Windows.utoc` | `2DE83D7EBD77774210BF63297AC2E457D036A89EDC95D5B9931AF0E33CABDD7A` | `2DE83D7E…33CABDD7A` | 687,265 | 2026-09-09 23:37:54 | **MATCH** |

Reconciliation rows (not part of the rule): root shim `GitClaudeUnrealTest.exe` `7F2C8FD6…300F4B5` 172,032 B MATCH · `global.ucas` `3722B03E…BEBF266` 3,318,560 B MATCH · `global.utoc` `129E0607…D6BA9B` 806 B MATCH · `.pdb` `53EAF924…B856032E` 252,080,128 B MATCH. **ALL FOUR MATCH ⇒ no re-cook, the build identity of the zip is the one 🧑 Jonathan adjudicated by eye; the resume proceeds.**

Stage census: **70 files, 1,760,342,236 B** (both exactly PART 4 §7) · `Windows\GitClaudeUnrealTest\Saved\` **ABSENT** — his adjudication run(s) wrote nothing beside the executable, which is the very fact the README now states; nothing to purge · the only `Saved` directory in the stage is `Engine\Saved` holding the stray `Engine\Saved\Config\Windows\Manifest.ini` (25 B, 2026-08-29 17:12 — `PKG-§10` note; it rides, named, as in both previous zips) · `Engine\Config\StagedBuild_GitClaudeUnrealTest.ini` present, 3 B · **no staged file has an mtime later than 2026-09-09 23:40** (the resume's re-stage) — the stage has not been touched since.

## 3. Phase E by hand — route (a), the render, the diff quoted in full, the greps

**How the nine values were obtained without re-deriving any of them:** the `c6fce42` source (the one revision 1 was rendered from) was turned into a regex template — every fixed byte escaped, the first occurrence of each placeholder a named lazy group, every later occurrence a backreference (so the 3× `CONFIG`, 2× `SIZE`, 2× `HEAD`, 2× `DIFF_BASE`, 2× `CLOUD_SYNC` sites are forced to agree) — and matched against the whole revision-1 README. **Round-trip control: substituting the nine values back into the `c6fce42` source reproduced `README.r1.md` byte for byte (PASS).** Only then was the NEW source (`2092A842…` 83,081 B / 949 lines) rendered with the same values, `VERIFIED` extended by ONE paragraph. `values.json` holds all nine verbatim.

| Placeholder | × | Value (revision 1, verbatim — lengths measured) |
|---|---|---|
| `CONFIG` | 3 | `Shipping` |
| `ZIP_NAME` | 1 | `Siegebound-Win64-Shipping-2026-09-09.zip` |
| `DATE` | 1 | `2026-09-09` |
| `HEAD` | 2 | `31edc23` |
| `DIFF_BASE` | 2 | `22728c8` |
| `SIZE` | 2 | 405 chars, 1 line (unchanged from PART 4 §6) |
| `VERIFIED` | 1 | 2,981 chars / 16 lines (unchanged) **+ the r2 paragraph below** |
| `CHANGED_SINCE` | 1 | 1,425 chars / 7 lines — ⛔ NOT re-derived, verbatim |
| `CLOUD_SYNC` | 2 | 289 chars, 1 line (`PKG-§12` pinned text, verbatim) |

**The appended paragraph (the `VERIFIED` value is mine, `PKG-§7b` cl. 4), one sentence, player-facing, no task ids:** *"**README revision 2, re-rendered 2026-09-10:** the note above about where the game saves was wrong in revision 1 of this README (it named a folder beside the executable) and now states the measured location; the game files in this zip are unchanged — the same executable, `.pak`, `.ucas` and `.utoc`, with the same four SHA-256 hashes as the build verified above, re-measured before this archive was written."* Wording note: the manager's draft ended *"the same four file hashes as above"*; the rendered *What was verified* block says the four hashes MATCHED but does not print them, so *"as above"* would have pointed at nothing — it reads *"as the build verified above"* instead. Nothing else in `VERIFIED` was touched.

**Output:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\README.md` — **89,188 B, 972 lines, UTF-8, no BOM, LF, sha256 `FE9010F4FEC0B268ED4D6A5337056E5499EB575BACC7CA36838D2CC5B4DE846C`** (revision 1: 86,729 B / 967 lines / `7D0D904E…`). Rendered twice (see §6 (iv)); identical bytes both times.

**Greps (`render.log`):** `{{SHIP:` survivors = **0** — positive control: the source reads **15** · click target `Windows\GitClaudeUnrealTest.exe` on **exactly one line (50)** (`PKG-§10`) · zip name on exactly one line (18) · `<!-- src:` markers **333** (revision 1: 332; the +1 is 1208's citation — the render keeps the comments exactly as revision 1 did; ⛔ nothing is stripped, contrary to the QA report's *"single stripped comment"* wording — invisible when rendered either way) · title `# Siegebound — Win64 Shipping build` · U+FFFD = 0, em dashes 188 · `%LOCALAPPDATA%` on lines 171/172/175 · **the false location `Windows\GitClaudeUnrealTest\Saved` occurs on exactly two lines, both as the thing that was wrong, never as a claim:** line 175 (inside 1208's `<!-- src: … Windows\GitClaudeUnrealTest\Saved\ absent … -->` comment) and line 206 (revision 1's own *What was verified* sentence: *"the *Known notes* line above that names `Windows\GitClaudeUnrealTest\Saved\` is wrong for this build and will be corrected in the next package"* — see §6 (ii)).

**⭐ The proof — `diff` revision 1 → revision 2, quoted in full: 2 hunks, −3 / +8 = 1208's hunk (−3/+6) + one blank line + the one sentence. Nothing else moved.**

```
--- README.md (revision 1, c6fce42 render)
+++ README.md (revision 2)
@@ -168,7 +168,10 @@
   package for the same reasons; whether a debug-symbol (`.pdb`) file shipped beside the binary
   is recorded in the size line at the top.
-- The game writes its save data (accounts, decks, settings) into
-  `Windows\GitClaudeUnrealTest\Saved\` next to the executable, so extract somewhere you have
-  write permission (not `C:\Program Files`).
+- The game writes its save data (accounts, decks, settings) into your Windows user profile, not
+  next to the executable: `%LOCALAPPDATA%\GitClaudeUnrealTest\Saved\` — normally
+  `C:\Users\<you>\AppData\Local\GitClaudeUnrealTest\Saved\` (typing `%LOCALAPPDATA%` into the
+  File Explorer address bar opens that `Local` folder). Your decks are in
+  `SaveGames\SiegeDecks.sav` and your settings in `Config\Windows\GameUserSettings.ini`; nothing
+  is written beside the executable. <!-- src: measured — .claude/pipeline/handoffs/TASK-1193-buildmaster.md:400-402 (PART 4 §5: … [the 2,050-char citation of README-source.md:175, verbatim — see README.r1-to-r2.diff for the full line] … (grep 2026-09-10, 0 hits) -->
 - Carried forward from the 2026-08-29 package: on that machine's first runs the audio device
   occasionally failed to open (`OpenAudioStream failed`). It did not stop the game from booting
@@ -202,4 +205,6 @@
 
 **Also measured this ship:** the packaged game wrote its settings and decks to `C:\Users\<you>\AppData\Local\GitClaudeUnrealTest\Saved\`, **not** beside the executable — the *Known notes* line above that names `Windows\GitClaudeUnrealTest\Saved\` is wrong for this build and will be corrected in the next package. The folder next to the executable stayed empty after two runs.
+
+**README revision 2, re-rendered 2026-09-10:** the note above about where the game saves was wrong in revision 1 of this README (it named a folder beside the executable) and now states the measured location; the game files in this zip are unchanged — the same executable, `.pak`, `.ucas` and `.utoc`, with the same four SHA-256 hashes as the build verified above, re-measured before this archive was written.
 
 ⚠️ Not verified by machine: actually *playing* a match with mouse and keyboard. There is no
```
(The only elision above is the middle of line 175's citation comment; `README.r1-to-r2.diff` in the run dir holds every byte.)

## 4. Phase D by hand — `PKG-§7b` verbatim: `.partial` → `SHIP-§4` read-back → entry-stream hashes → rename

`zip_stage_r2.ps1`: .NET `ZipArchive`, **`ZipArchiveMode.Create`** on a `FileMode.CreateNew` stream (ZIP64-capable; ⛔ not `Compress-Archive`; ⛔ not `Update` mode on the existing zip — the existing zip was opened READ-only, for the entry comparison, and never written), `README.md` at the root first, then every staged file as `Windows/<rel>` in sorted order, `Optimal`. Written to **`Siegebound-Win64-Shipping-2026-09-09.zip.partial`** in 40 s (00:36:21 → 00:37:01 PDT). The script never renames; the rename was a separate step after the read-back was inspected.

**`SHIP-§4` read-back ON THE `.partial` (`zip.log`, verbatim line):**
`READ-BACK (.partial): entries=71 clickTarget=True binary=True README=True pak=1 ucas=2 utoc=2 models/gguf=0 exes=4 pdbs=1 rootEntries=README.md`
— exes = `Windows/GitClaudeUnrealTest.exe` (shim) · `…/GitClaudeUnrealTest-Win64-Shipping.exe` · `vc_redist.x64.exe` + `vc_redist.arm64.exe` (prereq installers — `PKG-§10`: GAME exes = 1 + shim) · pdb = the Shipping `.pdb` only · `README.md` entry **89,188 B** (= the file) / 30,767 B compressed · **sum of entry lengths 1,760,431,424 = stage 1,760,342,236 + README 89,188 EXACTLY** · entry count = 70 + 1.

**The entry-level same-build proof — the four game entries' streams hashed from INSIDE the `.partial` (`entry.Open()` → SHA-256, no extraction to disk):**

| Entry | sha256 from the archive stream | Verdict vs §2 |
|---|---|---|
| `Windows/…/GitClaudeUnrealTest-Win64-Shipping.exe` | `9965A35AE1F349F604837B642D6A4FEBC2E8EBE624802DBFEC559835B4FEFF2C` (178,076,160 B) | **MATCH** |
| `Windows/…/GitClaudeUnrealTest-Windows.pak` | `E0CCFFDECCCD1EB6F5E837CB02ACEB8706DCCFD6D8C6DB864003C277CAEE6464` (11,466,608 B) | **MATCH** |
| `Windows/…/GitClaudeUnrealTest-Windows.ucas` | `7DB7A5DDA0160AEB40512CEB994B61464409E9BB44F42608B36F2FBF529B0EFA` (1,103,375,200 B) | **MATCH** |
| `Windows/…/GitClaudeUnrealTest-Windows.utoc` | `2DE83D7EBD77774210BF63297AC2E457D036A89EDC95D5B9931AF0E33CABDD7A` (687,265 B) | **MATCH** |

**Entry-by-entry comparison with the revision-1 zip (central directories, no inflation):** 71 ↔ 71 entries, none missing either way; entries whose `Length` differs = **1** (`README.md` 86,729 → 89,188); entries whose `CompressedLength` differs = **1** (`README.md` 29,878 → 30,767) — **every one of the 70 game entries has the same length AND the same compressed length as in the old zip.** **`D2-SIZE-SANITY`: new 1,334,632,180 − old 1,334,631,291 = 889 B; README compressed delta 30,767 − 29,878 = 889 B; sum-of-compressed delta 889 B — equal, nothing unexplained.** `READ-BACK RESULT: PASS`.

**The rename (`zip.log` 00:37:43–44):** `.partial` hashed BEFORE the rename → `1CAF8DCD…3EA7F8`; `Move-Item -Force` over `Siegebound-Win64-Shipping-2026-09-09.zip`; the final file re-hashed → same. **Result: `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Siegebound-Win64-Shipping-2026-09-09.zip` — 1,334,632,180 B (1.33 GB), 71 entries, mtime 2026-09-10 00:37:01 PDT, sha256 `1CAF8DCD93FBFF07294A0F7650D360D8EF2B0163455858ECCA971E15833EA7F8`.**

- **Old sha256 ~~`9CA197CB43581459436B18C4382AE2B889795DFFBD03E20E102779B450D70F19`~~ (1,334,631,291 B, 2026-09-09 23:50:35) — STRUCK, not deleted from the record (`PKG-§7b` cl. 3): its README carried a save-location sentence that was false for a Shipping build; its 70 game entries are byte-identical to the new zip's.** Re-measured on the old file at 00:29 today before anything was written — it was the file PART 4 recorded.
- Retention: `.partial` remains = False · Shipping zips on disk = **1** (replaced in place, same name — the manager's ruling) · `Siegebound-Win64-Development-2026-08-29.zip` 1,409,955,049 B, 2026-08-29 17:16:25 — **PROTECTED, untouched** · `.ship\ship-state.json` 936 B, 2026-09-09 23:39:37 — **untouched** · nothing pruned.

## 5. Phase F — commit by explicit pathspec, anchored one level up (`SC-§102`), the controls, the flips

- **`SC-§106` existence, resolved against disk from the git root:** `GitClaudeUnrealTest/Docs/Packaging/README-source.md` EXISTS 83,081 B · `…/handoffs/TASK-1208-programmer.md` EXISTS 15,056 B · `…/qa/TASK-1209-report.md` EXISTS 11,823 B · `…/TASKBOARD.md` EXISTS 8,681,972 B · `…/CONVENTIONS.md` EXISTS 2,946,548 B. **Controls that went red:** `GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1209.md` (a wrong name) → `MISSING`; `Docs/Packaging/README-source.md` (the mis-anchored spelling) → `MISSING`.
- **Never-commit fences proven clean:** `git status --porcelain --untracked-files=all -- Tools Source Content Config Saved Plugins packagedZIPofGame CLAUDE.md SLACK.md .claude/commands` → empty. Index empty before the commit. `check-ignore` answers `.gitignore:19` for the staging dir.
- Untracked records `git add -- <one path>` each (2); then **`git commit -F commit-msg.txt -- <the 5 paths>`**. Subject = the board's cl. (4) first line verbatim; body names the four hashes, the r1→r2 diff shape, the old sha256 struck and the new one, the run dir; ends with the two trailer lines.
- **The COMMIT, verified (`git show --stat HEAD`), never the index:**

```
8d056793ceb65e0d36dac0dfb8dfaf3ab42722a1
TASK-1208: the shipped README said the game saves beside the executable — measured, it saves under %LOCALAPPDATA%\GitClaudeUnrealTest\Saved; the README entry in Siegebound-Win64-Shipping-2026-09-09.zip is replaced from the verified, unchanged stage — no re-cook, same build, same name (gated by TASK-1209; PKG-§7b, SHIP-§4)

 .../.claude/pipeline/CONVENTIONS.md                |  11 +
 GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md  | 123 +++++++++++-
 .../pipeline/handoffs/TASK-1208-programmer.md      | 222 +++++++++++++++++++++
 .../.claude/pipeline/qa/TASK-1209-report.md        |  53 +++++
 .../Docs/Packaging/README-source.md                |   9 +-
 5 files changed, 409 insertions(+), 9 deletions(-)
```
No stray; porcelain EMPTY immediately after; `origin/main...main` = **`0 3`** (3 ahead, 0 behind) — ⛔ **NOT pushed.** `TASKBOARD.md`'s 123 lines = the manager's boarding of this pass + 1208's/1209's own flip lines + my one-line `TASK-1193` note (the +1 over the pre-note stat of 122). No `Source/**` in the diff ⇒ no compile owed (resolved against git, not assumed).
- **`SC-§103`, three ids → three flips, `Edit` one `- status:` line each, AFTER the commit:** `TASK-1208` · `TASK-1209` · `TASK-1210` → `done (commit 8d05679)`; the board is dirty by exactly those three lines afterwards (a line cannot carry the hash of the commit that carries it), plus this untracked handoff. Both ride the next commit.
- **`L_Arena.umap` sha256 `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` — unmoved** (no editor ran; `Content/` porcelain empty).

## 6. What is NOT proven — unchanged from `handoffs/TASK-1193-buildmaster.md` PART 4 §9 (say so, do not re-derive) — plus this pass's own residuals

PART 4 §9 stands word for word: no input-injection lane ⇒ no gameplay-feel claim; the rig's capture proved nothing about pixels — the pixel evidence for this package is 🧑 his eye on the byte-identical stage; the Graphics 10-s revert, the fog cycle, the enemy-side shimmer remain unobserved; the list he is owed for his extract-and-click is unchanged. This pass adds:

- (i) 🙋 **His extract-and-click is now owed on the NEW archive** — the bytes he would extract changed (README only; the 70 game entries are hash-identical, proven from inside the archive), but the archive itself is a different file (`1CAF8DCD…`). His previous sitting was on the staged folder, not the zip, either way.
- (ii) 📌 **Lead for the manager (`SC-§101`), not acted on:** revision 1's *What was verified* sentence *"…is wrong for this build and will be corrected in the next package"* survives verbatim at line 206 (the one-sentence constraint of this row forbade touching it) and is superseded by the revision-2 paragraph immediately after it — the reader sees the correction, but the phrase *"in the next package"* is now stale beside a README that IS the correction. The value lives in `{{SHIP:VERIFIED}}`, i.e. the host's text, so the next ship simply does not repeat it; no source edit is owed.
- (iii) The appended sentence deviates from the manager's draft in one phrase (*"as above"* → *"as the build verified above"*), for the reason in §3.
- (iv) `render_readme_r2.py`'s FIRST run wrote the README and every check, then crashed printing the diff's `⚠️` context line to the cp1252 console (`UnicodeEncodeError`) before `render.log` was written; `sys.stdout.reconfigure(encoding="utf-8")` was added and the run repeated from the same inputs (`README.r1.md`, the source) — README bytes identical both times (`FE9010F4…`), `RESULT: PASS`, log on disk. Named because the first run's silence on `RESULT` was a crash, not a verdict.
- (v) The QA report's note *"reproduce line :175's comment as a single stripped comment"* was not followed literally: revision 1 kept every `<!-- src: -->` comment in the rendered file (332), and a render that strips them would have made the r1→r2 diff fail its own gate; revision 2 keeps them too (333). Invisible when rendered; no reader-facing difference.
- (vi) Not measured: whether 7-Zip / Explorer extract the new archive identically to the old — the format, method and .NET version are the same as PART 4 §7 and every non-README entry has the same compressed length, so the lead is weak, but it is his extract step, not mine.

## 7. Artefacts (absolute)

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Siegebound-Win64-Shipping-2026-09-09.zip` — 1,334,632,180 B, sha256 `1CAF8DCD93FBFF07294A0F7650D360D8EF2B0163455858ECCA971E15833EA7F8`, README revision 2, same build (`31edc23`) · `…\packagedZIPofGame\README.md` (89,188 B, `FE9010F4…`) · `…\packagedZIPofGame\Windows\` (the verified stage, 70 files, untouched)
- `…\packagedZIPofGame\.ship\20260910-073200-readme-r2\{render_readme_r2.py, render.log, values.json, README.r1-to-r2.diff, README-source.c6fce42.md, README.r1.md, zip_stage_r2.ps1, zip.log, commit-msg.txt}`
- Commit `8d05679` (5 files) · this handoff (rides the next commit) · Slack: 🔧 Build & Git `1783116286.945249`.
