# QA Report — FOOTAGE-PIPELINE-SETUP (Jonathan-directed infrastructure, no TASK-###)
Verdict: PASS

Scope: the FOOTAGE-REVIEW lane — `Tools/VideoReview/extract_frames.py`, `.claude/agents/footage-analyst.md`, CONVENTIONS FR-§0..§5, CLAUDE.md deltas, SLACK.md deltas, root `.gitignore`, dry-run artifacts (VID-001 report + 5 promoted PNGs). Gate: safe and correct to commit. Blockers: 0 · Warns: 6 · Nits: 8.

## Verified clean (the load-bearing checks)

- **`shell=True`: zero code occurrences** — grep hits only the docstring/comment stating the ban. Every subprocess call is list-argv (`run()` at extract_frames.py:85-87; call sites 145, 195, 235, 282, 329, 334, 407).
- **`parents[3]` arithmetic CORRECT**: file at `<root>/GitClaudeUnrealTest/Tools/VideoReview/extract_frames.py` → parents[0]=VideoReview, [1]=Tools, [2]=GitClaudeUnrealTest, [3]=`C:\GitProjects\GitHub\GitClaudeUnrealTesting` = git root. Confirmed against on-disk reality: root `.gitignore`, root `.gitattributes`, and `testvideo/` all live there. `EVIDENCE_ROOT` re-descends into the project dir correctly (:49).
- **Promote write-confinement HOLDS** (:373-395): `--id` gated by `re.fullmatch(r"VID-\d{3}")`, symptom slug sanitized to `[a-z0-9-]` with empty-slug rejection, `-t` token digits-only from `parse_ts`, date from `date.today().isoformat()`. No component can carry a path separator or `..` — dest cannot escape `playtest-evidence/<date>/`. Name shape matches FR-§1 exactly.
- **Exit-code table matches the docstring**, including the argparse 2→64 remap (:464-467): argparse's native exit 2 (usage) is remapped to 64 so it cannot collide with exit 2 = no-ffmpeg; `--help`'s exit 0 passes through as 0. Exits 2/3/4/5/64 all reachable at the documented sites.
- **`-c` crop-state suffix (frames) is unambiguous** (:326): `f00005_00s.png` vs `f00005_00s-c.png`; the `--run` glob `{tag}-r*.png` cannot cross-match (`-c-r01` fails the `-r*` position for the uncropped tag, and vice versa). **Confirmed live in the dry-run cache**: the stale pre-autocrop `f00005_00s.png` (the analyst's reported "tool quirk" at VID-001 line 63) coexists with the post-fix `f00005_00s-c.png` — the quirk was a stale-cache read, and the suffix is precisely its fix. Sheets record crop in `manifest.json` only — sufficient, conditional on WARN-3 below.
- **Malformed-filename hardening real**: filenames verbatim, list argv, slug/hash cache key (:113-114); the dry run itself exercised a Game Bar name with double spaces + parentheses end to end.
- **Root `.gitignore:12` `testvideo/`** — unanchored dir pattern; everything beneath an ignored directory is ignored, so `testvideo/.frames/**` (the whole cache) is covered. The comment's LFS claim verified: `*.mp4` at root `.gitattributes:7`.
- **Agent frontmatter conforms to house convention** (qa-reviewer/manager precedent): single-line comma `tools:` allowlist — `Bash, Read, Grep, Glob, Write` + the 4 Slack tools, no Edit, no engine/Blender MCP (FR-§0.3 satisfied); description carries the trigger ("Use when Jonathan provides a gameplay video…") and the hard negative.
- **FR-§ batch internally consistent, no namespace collision**: all 8 `FR-` occurrences in CONVENTIONS.md sit inside the batch (lines 4141-4173); no clash with SIE-§/ACC-§/SC-§/GH-. Born-with-prefix honored ("cite as FR-§N, never bare §N"). FR-§2's exit table matches the tool; FR-§1's cache-contents list matches actual behavior; FR-§0.2's `.gitignore:12` citation is character-accurate.
- **CLAUDE.md deltas present + consistent**: 6-agent count (:3), team row (:14), footage/ communication bullet (:24), routing-rule-1 exception citing FR-§0.4 (:40), hard-gates video line (:63).
- **SLACK.md deltas present**: 🎬 registry row ts `1787798959.639009` (:35), routing row (:50), `🎬 FOOTAGE-ANALYST:` in the prefix list (:63).
- **VID-001 dry-run report is FR-§4/template-conformant**: all sections present (Symptoms → Timeline table → promoted evidence → mechanism → routing → not-examined), header carries probe stats + the VFR ±0.25 s qualifier + autocrop note, every mechanism line labeled "Hypothesis, not verdict", observations stated as pixels not conclusions, budget honesty line present. All 5 promoted PNGs exist on disk under `playtest-evidence/2026-08-26/` with names character-identical to the report bullets and law-conformant to FR-§1.

## Findings

- [WARN] extract_frames.py:122-133 / :321 / :387-389 — `parse_ts` has no lower bound: a negative timestamp (`--at -5`, `--t -5`) passes `do_frames`' upper-bound-only check (ffmpeg tolerates it; frame tag becomes `f-0005_00s`), and in `do_promote`, `divmod(int(round(-5)), 60)` → `(-1, 55)` → evidence name token `-t-1m55s`, violating the FR-§1 `t<MM>m<SS>s` pattern — the one hole in the otherwise-tight promote name gate. Fix: in `parse_ts`, exit 5 when `t < 0`.
- [WARN] footage-analyst.md:17 — the usage hint `promote … [--t mmss]` contradicts what `parse_ts` accepts: an analyst following it literally and passing `--t 0132` for 1m32s gets 132 s → token `-t02m12s` — a wrong-timestamp evidence name minted by the doc itself. The tool's own help says `e.g. 1:32` (correct). Fix: change the hint to `[--t m:ss]`.
- [WARN] extract_frames.py:228-231 vs :277/:284 — `do_sheets` purges `thumbs/` but never deletes old `sheet_*.png` in the cache root: a re-run with a longer interval/bigger grid producing fewer sheets leaves a stale tail (e.g. old sheet_03/04) beside a fresh manifest listing only 2 — the exact stale-cache hazard class the `-c` suffix just fixed for frames, and it also makes manifest-only crop recording unsafe for the leftover sheets. Fix: `for f in cache.glob("sheet_*.png"): f.unlink()` before regeneration. (With this fix, sheets embedding crop state in `manifest.json` only is sufficient — a sheets run rewrites the whole set + manifest together.)
- [WARN] extract_frames.py:366 — `do_crop` writes its output beside the SOURCE frame wherever that is: pointed at a promoted PNG, it would write a `…-crop-xNyM.png` into `playtest-evidence/`, violating FR-§1's "written ONLY via the tool's `promote`" law for that directory. Doctrine fences the analyst to cache frames, but the law lives in code everywhere else in this tool. Fix: refuse (or redirect to the cache) when `src` resolves under `EVIDENCE_ROOT`.
- [WARN] CLAUDE.md:34 + SLACK.md:55 — both still say "All five agents" hold/post direct Slack grants; the team is now six and SLACK.md's own identity-prefix list already carries `🎬 FOOTAGE-ANALYST:`. These counts govern proxy-vs-direct decisions. Fix: update both to six, and note footage-analyst direct posting as unproven-until-first-success (the manager/qa precedent, SLACK.md:39-40).
- [WARN] footage-analyst.md:11 vs CONVENTIONS.md FR-§0.3 (4148) — the agent body relaxes the binding ruling "never runs git" to "never run a git command that writes" (its own frontmatter description at :3 says "never runs Git", agreeing with the ruling). A fence text weaker than a ⛔ non-reopenable ruling is a drift seed. Fix: align the body to "never runs git".

### Nits
- [NIT] extract_frames.py:136-138 — `fmt_ts` rounding edge: s ∈ [59.95, 60) renders `00:60.0` instead of `01:00.0` (sheet labels only; stamps are interval multiples so it rarely triggers).
- [NIT] extract_frames.py:85-87 — `run()` uses `text=True` without `encoding`: ffmpeg stderr (UTF-8) decoded via locale cp1252 — mojibake normally, and a hard `UnicodeDecodeError` is possible on the 5 undefined cp1252 bytes. Add `encoding="utf-8", errors="replace"`; a generous `timeout=` would also guard pathological decode hangs (local-only tool, no network — the FR lane has no secrets and no remote calls).
- [NIT] extract_frames.py:221-225 — `--grid 0x0` passes the regex → `ZeroDivisionError` at :248 (uncaught traceback, exit 1, off the documented table). Reject cols/rows < 1 with exit 64.
- [NIT] extract_frames.py:163 — a container reporting `format.duration` as `"N/A"` (rare, some MKV) makes `float()` throw an uncaught ValueError. Wrap → exit 3.
- [NIT] extract_frames.py:113-114 — cache key = sha1(filename) only, no mtime/size: overwriting a video with the same name silently serves stale cached frames. Game Bar's timestamped names make collisions unlikely; folding mtime into the hash would close it.
- [NIT] extract_frames.py:188-206 — `detect_crop` has no minimum-area sanity clamp: an all-dark scene at the 3 sample points could vote in a sliver box. The 3-point vote + near-full-frame rejection + the live 992×576-in-2496×1440 verification mitigate; consider rejecting winning boxes under ~5% of frame area.
- [NIT] extract_frames.py:354-356 / :374-376 — `crop`/`promote` frame-not-found exits 3, but the docstring defines 3 as "video missing or undecodable". Widen the docstring wording ("input missing/undecodable").
- [NIT] .gitignore:12 — `testvideo/` is unanchored, so a nested `Anything/testvideo/` would also be ignored anywhere in the tree. Harmless today; `/testvideo/` would anchor it to the root if ever desired.

## Notes for build-master

**Stage EXPLICIT paths only — the working tree carries unrelated dirt (Castle art/code wave, TASKBOARD).** No `git add -A`, no `git add .` at any level. From git root `C:\GitProjects\GitHub\GitClaudeUnrealTesting`:

1. `.gitignore`  (root — the testvideo/ ignore)
2. `GitClaudeUnrealTest/Tools/VideoReview/extract_frames.py`
3. `GitClaudeUnrealTest/.claude/agents/footage-analyst.md`
4. `GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md`  ⚠ this file also carries pre-existing modifications from earlier waves — eyeball `git diff` and confirm every hunk you stage is intended (FR-§ batch = lines ~4141-4174)
5. `GitClaudeUnrealTest/CLAUDE.md`
6. `GitClaudeUnrealTest/.claude/pipeline/SLACK.md`
7. `GitClaudeUnrealTest/.claude/pipeline/footage/VID-001-castle-entrance-blocked.md`
8. `GitClaudeUnrealTest/.claude/pipeline/playtest-evidence/2026-08-26/` — exactly 5 PNGs (berm-lip, entry-jump-blocked, floating-hp-bar, click-ring-no-response, card-label-collision); they will stage as LFS pointers (`*.png` at root `.gitattributes:4`) — expected, matches the 2026-08-17 evidence precedent
9. `GitClaudeUnrealTest/.claude/pipeline/qa/FOOTAGE-PIPELINE-SETUP.md`  (this report)

**⛔ The testvideo gate — run all three, from git root, BEFORE commit:**
```powershell
git check-ignore -v testvideo/.frames        # MUST print the .gitignore:12  testvideo/  rule
git ls-files testvideo                       # MUST print nothing (nothing ever tracked)
git diff --cached --name-only | Select-String -Pattern "testvideo"   # MUST print nothing after staging
```
If any of the three misbehaves, STOP and escalate — a video entering LFS is the failure FR-§0.2 exists to prevent.

The 6 WARNs are commit-safe (doc/robustness hardening, none affect the committed tree's correctness); recommend boarding them as a small follow-up polish task rather than blocking this commit.

---

## ADDENDUM — WARNS-FIX VERDICT (2026-08-26, post-PASS re-review)

Verdict: **PASS** — all 6 WARNs closed at source; scope confirmed to exactly the six claimed sites; the staging list and testvideo gate above stand unchanged. (Line numbers below refer to the post-fix files; extract_frames.py grew 490→501 lines, +11 = exactly the three insertions.)

1. **W1 negative timestamps — FIXED** (extract_frames.py:133-135): `if t < 0 or any(n < 0 for n in nums)` → exit 5 "negative timestamp" — closes both the `do_frames` lower-bound hole and the malformed `-t-1m55s` promote token; the `any()` clause even catches mixed-sign inputs like `1:-30` whose total is positive. Exit 5 fits the docstring's "bad timestamp" row.
2. **W2 promote hint — FIXED** (footage-analyst.md:17): now `[--t <m:ss or seconds, e.g. 1:32>]` — matches `parse_ts`' accepted forms and the tool's own `--t` help; the mmss wrong-timestamp trap is gone.
3. **W3 stale sheets — FIXED** (extract_frames.py:236-237): `cache.glob("sheet_*.png")` purge before regeneration, upstream of both the PIL and ffmpeg-fallback branches — no stale tail can survive a re-run, so manifest-only crop recording for sheets is now sufficient as predicted. Residual (nit-level, acceptable): if extraction fails AFTER the purge (exit 3), the old `manifest.json` briefly outlives its deleted sheets — but that fails loud (Read on a missing file), never serves wrong pixels, and the cache is disposable by law.
4. **W4 crop-into-evidence — FIXED** (extract_frames.py:371-377): `out.resolve().relative_to(EVIDENCE_ROOT.resolve())` succeeding ⇒ inside the evidence tree ⇒ refuse with exit 64 ("use promote") before `region.save` is reached; `ValueError` = outside = normal path. Guard direction correct, `resolve()` normalizes `..`/case, and promote remains the only code path that writes into `playtest-evidence/`.
5. **W5 stale agent counts — FIXED** (CLAUDE.md:34, SLACK.md:55): both now "All six agents". Note: SLACK.md:40's "all five agents verified direct" is a dated 2026-07-03 changelog entry — historically accurate at five agents, correctly left frozen; operative law (:55) is what governs and is now right.
6. **W6 git fence — FIXED** (footage-analyst.md:11): now "never run any git command (FR-§0.3)" — aligned to the ruling's letter with the citation added; frontmatter description (:3, "never runs Git") already agreed.

**Scope check — diff since the original review is exactly the six sites:** full re-reads confirm extract_frames.py changed at the three insertions only (+3/+2/+6 lines; every other line character-identical, argparse/docstring/paths untouched), footage-analyst.md at lines 11 + 17 only (62 lines both passes), CLAUDE.md at :34 only (70 lines), SLACK.md at :55 only (92 lines). Root `.gitignore` and the CONVENTIONS FR-§ batch (4141-4174) re-read character-identical. Honest limit: QA has no git access — this is file-level comparison against the reviewed versions, covering the whole review set except CONVENTIONS' pre-§FR bulk and the binary PNGs; the committer's mandated `git diff` hunk eyeball (staging note 4) remains the final authority there.
