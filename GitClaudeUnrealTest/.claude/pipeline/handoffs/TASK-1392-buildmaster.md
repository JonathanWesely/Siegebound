# TASK-1392 — [PROBE-WAVE-COMMIT-HOST] — build-master handoff

**Marker:** `TASK-1392-PROBE-WAVE-COMMIT-HOST`
**Run:** 2026-09-22, build-master. **Zero code · zero compile · zero PIE · zero MCP · zero push.**
**Law:** `TL-§5e` cl. 1/7/7a/7b/7d · `SC-§102` · `SC-§118` cl. 1/8 · `SC-§133` · `SC-§134` cl. 7(a) · `SC-§138`

---

## 1. THE COMMIT

| | |
|---|---|
| **hash** | `c5e8d97386d76d7a292c1ad03343f7a3cbc3ad39` (`c5e8d97`) |
| **parent (`%P`)** | `478ce88ca00120b81c0473cbf655b891f10e1df0` ⇒ **BUILT ON, NOT AMENDED** |
| **shape** | **14 files / 890 insertions / 11 deletions** |
| **read from** | **THE COMMIT** (`git show --stat HEAD`, `git show --numstat HEAD`) — **never the index** |
| **ahead** | **12 → 13** |
| **`origin/main`** | **UNCHANGED at `c31692948dbe791d0dacf109aa509d83de76541e`** ⇒ **NOT PUSHED** |
| **trailer** | `Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>` — present, last line |

---

## 2. THE PATHSPEC — DERIVED AT MY OWN INSTANT (`SC-§133`)

**Anchored ONE LEVEL UP** at `C:/GitProjects/GitHub/GitClaudeUnrealTesting` (`SC-§102` — a mis-anchored
pathspec answers with SILENCE, which reads exactly like "nothing to commit"). Root confirmed by
`git rev-parse --show-toplevel`.

**Instrument:** `git status --porcelain --untracked-files=all --ignored=matching`.

**COUNT: 14 explicit paths. No `-A`, no `.`, no bare directory.** The dispatch's read named 8 paths
because it counted `playtest-evidence/2026-09-22/` as one entry; at `--untracked-files=all` that
directory expands to **7 PNGs**, giving 14. Otherwise the prediction was exact.

| # | path (under `GitClaudeUnrealTest/.claude/pipeline/`) | state | shape |
|---|---|---|---|
| 1 | `CONVENTIONS.md` | M | +33/−3 |
| 2 | `TASKBOARD.md` | M | +91/−7 |
| 3 | `qa/TASK-1274-verify.md` | M | +7/−1 |
| 4 | `handoffs/TASK-1388-buildmaster.md` | NEW | +121/−0 |
| 5 | `qa/TASK-1389-report.md` | NEW | +178/−0 |
| 6 | `qa/TASK-1390-verify.md` | NEW | +98/−0 |
| 7 | `qa/TASK-1391-verify.md` | NEW | +341/−0 |
| 8–12 | `playtest-evidence/2026-09-22/VER-TASK-1390-*.png` ×5 | NEW | +3/−0 each (LFS pointers) |
| 13–14 | `playtest-evidence/2026-09-22/VER-TASK-1391-*.png` ×2 | NEW | +3/−0 each (LFS pointers) |

### EVERY EXCLUSION, NAMED

- **`testvideo/`** — root-gitignored; `*.mp4` is an LFS pattern that would otherwise swallow it.
  **Verified ABSENT from the census entirely** — nothing to exclude, nothing staged.
- **raw `Saved/**`** — `!!` ignored. Only PROMOTED evidence under `playtest-evidence/` entered git.
- **`.uasset`** — **this wave produced NONE.** The only `.uasset` entries in the census are seven
  `!!`-ignored `*_BuiltData.uasset` files (Fire_Magic, IceAttack, Ice_Magic, Prickly_Knight,
  Tree_Pack_1, sA_ArcheryVfxPack, sA_StylizedWizardSet). **None is a dirty tracked file ⇒ nothing
  to hold and nothing to escalate** under the row's `.uasset` clause.
- **`.claude/settings.local.json`** ×2 (repo root + project) — `!!` ignored, grant surface, never staged.
- Other `!!` ignored, 0 staged: `.vs/` · `Binaries/` · `Build/` · `Intermediate/` ·
  `DerivedDataCache/` · `Models/` · `Content/Dev/` · `Plugins/SiegeLlama/{Binaries,Intermediate}/` ·
  `Tools/ArtPipeline/{.venv,Cache}/` · `Config/SiegeCloudDev.ini` · `Docs/GDD-Submission-v3.pdf` ·
  `GitClaudeUnrealTest.sln` · `Automation_GitClaudeUnrealTest.sln` · `.claude/tmp/__pycache__/`.

### `TL-§5e` cl. 7a ORPHAN SWEEP ⇒ **ZERO ADDITIONAL**

The whole-repo untracked census at my instant returned **exactly the 11 untracked files already
predicted** (1 handoff + 3 qa + 7 PNGs) plus 3 modified. **No orphan anywhere in the repo.**
`TASK-1388`'s declared tail — `handoffs/TASK-1388-buildmaster.md` and `TASKBOARD.md` — was found
present and is carried at #4 and #2.

---

## 3. `CLAUDE.md` AND `CONVENTIONS.md` — STATE STATED, WITHHOLDING DECLARED EITHER WAY

### `CLAUDE.md` — **CLEAN. NEVER AUTHORED · STAGE WITHHELD** (`SC-§139` cl. 6)

`git status --porcelain` and `git diff --numstat HEAD` against it both returned **empty** at my
instant. **I authored not one byte of it, and the stage was withheld either way** — the withholding
is unconditional and does not depend on it having been clean. **Verified ABSENT from `c5e8d97` by
name sweep over `git show --name-only HEAD`.** Nothing swept, nothing partially taken, nothing reverted.

### `CONVENTIONS.md` — **DIRTY, AND IT IS CARGO. COMMITTED.** (+33/−3, 12330 → 12360 lines)

🚨 **My row's original clause said this wave amended nothing in `CONVENTIONS.md` and that a dirty
file must be named, held and escalated. That clause was CORRECTED AT SOURCE by the manager
(struck-not-deleted) after the probes returned and the six amendments were written.** I acted on the
corrected clause, not on the struck one. **I authored none of its content** (`names:` line unchanged:
staging yes, content NEVER).

**THE SHAPE CHECK SURVIVED THE RESCOPE AND WAS RUN IN FULL: 7 hunks, ALL SIX mapping to sections
NAMED ON MY ROW. ZERO unnamed sections ⇒ nothing held, nothing escalated.**

| hunk (new line) | shape | section | named on row? |
|---|---|---|---|
| `@@ -12222,0 +12223,7 @@` | +7 | **`VER-§5` cl. 5** — `ui_perform` dead end re-affirmed under `1.0.6` | ✅ |
| `@@ -12235 +12242 @@` | ±1 | **`VER-§7` cl. 2** — prose-vs-enum data point | ✅ |
| `@@ -12258,0 +12266,5 @@` | +5 | **`VER-§8` cl. 5** — the trigger fired, the answer was negative | ✅ |
| `@@ -12284,0 +12297,2 @@` | +2 | **`VER-§8` cl. 7** — incl. the B4 `.sav` staleness | ✅ |
| `@@ -12286 +12300,14 @@` | −1/+14 | **`VER-§8` cl. 7** — the card-placement confirm composed | ✅ |
| `@@ -12308 +12335 @@` | ±1 | **`VER-§8` cl. 10(b)** — the round-trip figure | ✅ |
| `@@ -12309,0 +12337,3 @@` | +3 | **`VER-§8` cl. 10(b)** — ≈30 s corrected to a measured ≈70.11 s upper observation | ✅ |

Clause boundaries were read from the file itself (`VER-§8` cl. 5 @ 12265, cl. 7 @ 12272, cl. 8 @ 12315,
cl. 10 @ 12327, cl. 11 @ 12343), not assumed.

### 🚨 `VER-§8` cl. 11 — **VERIFIED UNAMENDED, TWO INDEPENDENT WAYS**

1. **No hunk reaches it.** The last hunk's new range ends at line **12339**; cl. 11 begins at
   **12343**. Every one of the 7 hunks lands strictly before it.
2. **Byte-identical content.** cl. 11 → EOF, line-endings normalised (`tr -d '\r'` — the raw compare
   differed by exactly 18 bytes over 18 lines, a CRLF-vs-LF extraction artifact of `git show`, **not**
   content, and it was run down rather than assumed):
   - OLD (`HEAD~1`, from line 12313): `dd95ab0d9e40645649b519b77175a7a0aec1b6f1a3f393e6a442168ce7ed7b21`
   - NEW (working tree, from line 12343): `dd95ab0d9e40645649b519b77175a7a0aec1b6f1a3f393e6a442168ce7ed7b21`
   - **IDENTICAL.** The 12343 − 12313 = 30 line offset is exactly the file's net insertion (+33/−3).

⇒ **The `SetIgnoreInput` premise remains standing and unrelaxed, exactly as the dispatch required.**

---

## 4. THE (4) SHAPE CHECK — PER CARGO ROW

**All four AGREE. Nothing held. Nothing escalated.**

| row | declared deliverable | on disk? | row `status:` | verdict line 1 | agrees? |
|---|---|---|---|---|---|
| **`TASK-1389`** | `qa/TASK-1389-report.md` | ✅ +178/−0 | `measured` | `# Measurement Report — TASK-1389 …` — **no `Verdict:` line, and its row requires none**: the row states *"NO PASS/FAIL verdict (measurement report, not a gate)"* | ✅ |
| **`TASK-1390`** | `qa/TASK-1390-verify.md` + **evidence ×5** | ✅ +98/−0, **5 PNGs** | `measured` | `Verdict: MEASURED` byte-literal | ✅ |
| **`TASK-1391`** | `qa/TASK-1391-verify.md` + **evidence ×2** | ✅ +341/−0, **2 PNGs** | `measured` | `Verdict: MEASURED` byte-literal | ✅ |
| **`TASK-1388`** (tail) | `handoffs/TASK-1388-buildmaster.md` | ✅ +121/−0 NEW | `done`, commit `478ce88` | n/a | ✅ |

Evidence-directory census: **7 files, ALL `.png`**, 5 attributed to `TASK-1390` and 2 to `TASK-1391`
by filename — **exactly the declared counts, with no stray file of any kind** in the directory.

### 🔓 `qa/TASK-1274-verify.md` — THE FILE `TASK-1388` HELD, **RELEASED AND SHIPPED**

`TASK-1388` held it because `TASK-1385` had declared *"2 modified + 6 inserted"* (+8/−2) while git
measured **+7/−1**. **`TASK-1385` corrected its own declaration at source to `+7/−1`, 35 → 41 lines,
over hunks `@@ -29 +29,4 @@` and `@@ -30,0 +34,3 @@`.** I re-measured at my own instant and got
**+7/−1 over those two exact hunk headers — EXACT AGREEMENT** ⇒ the hold is discharged and the file
ships in `c5e8d97`. **The correction was the row's, not mine; I edited no content.**

---

## 5. THE LFS DETERMINATION — **`*.png` IS AN LFS PATTERN. VERIFIED BY oid-vs-`sha256`, NEVER BY SIZE.**

`.gitattributes:4` → `*.png filter=lfs diff=lfs merge=lfs -text`, and `git check-attr -a` on an
actual wave PNG returns `filter: lfs`. **These are this wave's first binaries.**

**Re-read from THE COMMIT (`git cat-file -p HEAD:<path>`), not the index.** Every one begins
`version https://git-lfs.github.com/spec/v1` — i.e. a genuine pointer, not a raw blob — and every
`oid sha256:` equals the working-tree file's `sha256sum`:

| PNG | oid == disk sha256 |
|---|---|
| `VER-TASK-1390-a2-live-arm-click-button-body-off-textblock.png` | `e529535f…de3619` **MATCH** |
| `VER-TASK-1390-baseline-menu-no-settings-panel.png` | `e72b5d93…6d3f73` **MATCH** |
| `VER-TASK-1390-dead-arm-click-over-no-widget.png` | `c5423c18…f203e9d` **MATCH** |
| `VER-TASK-1390-live-arm-click-settings-button.png` | `68c9c7e9…895dd63` **MATCH** |
| `VER-TASK-1390-reader-control-same-button-keyboard-route.png` | `24c98ff7…2a8faa` **MATCH** |
| `VER-TASK-1391-a1-composed-confirm-footman_t83.10s_f3124431.png` | `1cd8c783…6605e5` **MATCH** |
| `VER-TASK-1391-a2-composed-confirm-replication_t32.07s_f3132167.png` | `63d11b37…673f339` **MATCH** |

**7/7 MATCH.** Byte sizes were printed as description only and were **never the gate** — this project
has a measured scar where two different blobs weighed the same.

### The two filename deviations — **COMMITTED AS THEY ARE**

Already declared by the verifier in `qa/TASK-1390-verify.md`: attempt-1 files carry no `-a1`, and
`-t<MM>m<SS>s` is omitted. `TASK-1391`'s two files additionally carry a `_t83.10s_f3124431` style
stamp rather than `VER-§4`'s `-t<MM>m<SS>s`. **These are the verifier's declared record and are not
mine to rename** — no file was touched.

---

## 6. FENCE — WHAT THIS HOST DID NOT DO

**No code · no asset · no compile · no suite · no PIE · no MCP · no `--allow-empty` · no `--amend` ·
no `-A` / `.` / bare directory (14 explicit paths) · no push · no content edit of anything committed
— not a report, not a verdict line, not the amendment, not another row's line, not a grant surface.**

### EDITOR — CENSUSED **BY COMMAND LINE** AND **UNTOUCHED** (`SC-§118` cl. 1/8)

```
PID 26992  UnrealEditor.exe  "…/UE_5.8/…/UnrealEditor.exe" "C:\…\GitClaudeUnrealTest.uproject"
PID 13200  UnrealTraceServer.exe  daemon -d --sponsor 26992
```

A **GUI** instance, **🧑 Jonathan's** ⇒ **described, acted on in NO way.** No other editor and no
`-game` process present. Two boarded rows (`TASK-1393`, `TASK-1395`) need it up.

**Because its Git plugin auto-stages, the index was reset (`git reset -q`) before staging and every
verification in this document was read from THE COMMIT, never from the index.**

---

## 7. 🚨 MY DECLARED TAIL — FOR THE NEXT HOST (`TL-§5e` cl. 7d(ii))

**I did NOT invent a second commit to swallow my own hash** — `TASK-1347`'s refusal is law and the
regress does not terminate. Two paths are left dirty **on purpose**:

1. **`.claude/pipeline/handoffs/TASK-1392-buildmaster.md`** — NEW (this file).
2. **`.claude/pipeline/TASKBOARD.md`** — **THIS row's `status:` line ONLY**, flipped **AFTER** the
   commit so it could carry the **TRUE** hash `c5e8d97`. `Edit`, never `replace_all` (`SC-§120`).

**Nothing else is held.** `TASK-1396` (or whichever host follows) should expect exactly these two and
should re-derive at its own instant regardless (`SC-§133`).

---

## 8. FOLLOW-UPS — REPORTED, NOT ACTED ON (`SC-§50`)

Non-blocking; the manager turns these into rows if it wants them:

1. **`TASK-1391` recorded that the deck `.sav` changed `2026-09-22T11:41` local — 2 h 54 min BEFORE
   its run, not by it — so `TASK-1357` B4's deck census is now STALE.** The manager already dated this
   into `VER-§8` cl. 7; flagged here so a later row does not quote B4's census as current.
2. **`VER-§8` cl. 10(b)'s round trip is now recorded as a measured ≈70.11 s UPPER OBSERVATION, not a
   new constant.** A row budgeting PIE clock off `≈30 s` will under-budget.
3. **The `1.0.5` schema text is unrecoverable on this disk** — `handoffs/AURA-MCP-CENSUS.md` stores
   tool NAMES only, so a future re-measure cannot diff schemas either. Named as a debt by the
   manager in `VER-§8` cl. 5; it will cost again the next time a plugin version bumps.
4. **`TASK-1389`'s collapse leaves `VER-§8` cl. 11's mechanism still UNMEASURED.** Routes (i) Slate
   `SButton` Accept and (ii) `USiegeMenuInputSubsystem`'s `IA_Menu*` handlers both predict Jonathan's
   identical observable. `TASK-1393` is boarded to discriminate them and is blocked on this commit,
   which has now landed.

---

**Status flipped on this row only. Nothing pushed. `main` 13 ahead of `origin/main` at `c316929`.**
