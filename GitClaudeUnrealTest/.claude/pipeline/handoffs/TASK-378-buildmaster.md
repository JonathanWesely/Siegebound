# TASK-378 — [AG-B3] ANCIENT-GROUNDS integration commit — build-master handoff

**Date:** 2026-08-02 · **Agent:** build-master · **Status: COMPLETE**
**Gate:** Jonathan PASSED the TASK-377 PIE ship gate — verbatim: *"I can confirm all the features listed work as expected."*

---

## VERDICT

| | |
|---|---|
| **Commit A** (code + tooling + data + pipeline docs) | **`6e79a24`** — 49 files, +10,022 / −265 |
| **Commit B** (art assets + `DT_Cards`) | **`80c47e8`** — 28 files, +137 / −4 |
| **Branch** | `main`, **UNPUSHED — 3 ahead of `origin/main`**: A, B, and the hash-recording docs commit |
| **Compile** | **GREEN** — `Result: Succeeded`, target up to date, 0 actions |
| **`MI_Sorcerer_PBR`** | ❌ **NOT in either commit** — deliberately un-staged |
| **`L_Arena.umap`** | ❌ **NOT in either commit** — never saved, never staged |

**NO PUSH.** Jonathan's, by standing law. Baseline was `main == origin/main @ 10f14de`, 0 ahead — confirmed before starting.

---

## 1. THE FOUR COMMIT HAZARDS — each defeated and PROVEN

### 🔴 HAZARD 0 — the editor's Git provider auto-stages saved assets

Confirmed live: **13 files were staged with nobody having run `git add`** (`Provider=Git` in
`Saved/Config/WindowsEditor/SourceControlSettings.ini`). `git diff --cached --name-only` matched the
TASK-376 handoff's list exactly, `MI_Sorcerer_PBR` included.

**Action:** cleared the index with **`git restore --staged GitClaudeUnrealTest/Content/`** — this touches the
**index only** and does not write a single worktree byte (verified: all 13 files reverted to `??` with contents
intact). `git status` was then trustworthy, and every path was re-added explicitly.

**No bare `git commit`, no `-a`, no `add -A`, no `add .` was used anywhere in this task.** One git command per
shell call, no `&&`/`;` chaining, no output suppression. **`reset --hard` and `clean -fd` were never used.**

### 🔴 HAZARD 1 — `SK_Sorcerer.uasset` was `AM`; the LOD regen sat in the UNSTAGED half

Committing the inherited index entry would have shipped **pre-LOD bytes** and silently reverted TASK-372's last
acceptance criterion. **Three independent proofs it landed correctly:**

1. `git diff -- Content/Characters/SK_Sorcerer.uasset` → **EMPTY** after the explicit re-add (index == worktree).
2. Index blob **`46814f12dcb39619ec15641e70a7cba1fb147555`** == `git hash-object` of the working file — identical.
3. File mtime **2026-08-02 03:22**, i.e. after TASK-376's LOD regeneration (`lod_count` 1 → 3, verified there from
   a fresh editor process).

**Every other `AM` file was checked the same way.** Before each commit I re-read `git status --porcelain` and
judged the **two-column codes, not the names**: every staged entry read `A ` or `M ` with a **space in the second
column**, so no file went in with an unstaged remainder.

### 🔴 HAZARD 2 — `DT_Cards.uasset` was `M` and NOT staged

Explicitly added; verified present in `git diff --cached --name-only` before commit B. It carries the **entire
Sorcerer row and the Footman `DeckCount 11 → 9`** change. Without it the game would reference a card row absent
from the table. `Docs/Data/cards.csv` was diffed independently and contains **exactly the two intended deltas**.

### 🔴 HAZARD 3 — the two files that must NOT land

- **`Content/Materials/Instances/MI_Sorcerer_PBR.uasset`** — was sitting in the index from an earlier auto-stage.
  **Deliberately un-staged and never re-added.** It remains untracked on disk, byte-untouched.
- **`Content/Maps/L_Arena.umap`** — **never saved, never staged, never committed.** It is tracked at the HEAD blob
  and unmodified; it appeared in no `git status` output at any point. The one-time save exception stays spent.

**Final audit:** `git diff --name-only 10f14de..HEAD | grep -iE "MI_Sorcerer_PBR|L_Arena|umap"` → **zero hits**
(grep exit 1) across both commits.

---

## 2. COMPILE

```
Result: Succeeded
Total time in Unreal Build Accelerator local executor: 0.11 seconds
Target is up to date
```

**Exit-code law applied:** judged on the `Result: Succeeded` line, **not** the exit status — `Build.bat` returns 0
on failure (TASK-366 caught exactly that). "Target is up to date" is a *positive* result here, corroborated
structurally: the TASK-366 link (`UnrealEditor-GitClaudeUnrealTest.dll`, **2026-08-01 18:23:28**) post-dates
**every** source file in the tree (newest `BattlefieldScatter.cpp`, **16:14:36**), so the GREEN build covers these
exact bytes. **No Live Coding mutex was hit and the editor was never closed** (Jonathan active; a parallel agent
was working in it).

---

## 3. THE EXACT PATHSPEC LIST

### Commit A — `6e79a24` (49 files)

**C++ (17)** — all under `Source/GitClaudeUnrealTest/Siegebound/`:
`AncientGround.cpp`\* · `AncientGround.h`\* · `SorcererUnit.cpp`\* · `SorcererUnit.h`\* · `BattlefieldScatter.cpp` ·
`BattlefieldScatter.h` · `ScatterConfig.h` · `CaptureZone.h` (comment-only) · `SummonedUnit.cpp` · `SummonedUnit.h` ·
`CombatantHealthBarComponent.cpp` · `CombatantHealthBarComponent.h` · `CombatantHealthBarWidget.h` ·
`HealthBarProvider.h` · `SiegeCheatManager.cpp` · `SiegeCheatManager.h` · `DeckBuilderWidget.cpp`   (\* = new file)

**Data + QA-gated tooling (3):** `Docs/Data/cards.csv` · `Tools/ArtPipeline/rig_character.py` ·
`Tools/ArtPipeline/rig_manifest.json`

**Pipeline docs (29):** `.claude/pipeline/TASKBOARD.md` · `.claude/pipeline/CONVENTIONS.md` ·
`.claude/pipeline/qa/TASK-365-report.md` · `.claude/pipeline/qa/TASK-372-tooling-report.md` ·
`handoffs/TASK-{358,359,360,361,362,363,364}-programmer.md` · `handoffs/TASK-366-buildmaster.md` ·
`handoffs/TASK-{368,369,370,371,372,373,374,375}-artist.md` · `handoffs/TASK-375-facing-fix.md` ·
`handoffs/TASK-376-buildmaster.md` · `handoffs/TASK-374/` (3 files) · the four `TASK-375-*.png` evidence images

### Commit B — `80c47e8` (28 files)

**`/Game` uassets (14):** `Content/Blueprints/Units/BP_Unit_Sorcerer.uasset` ·
`Content/Characters/Anims/A_Sorcerer_{Attack,Death,Idle,Walk}.uasset` · `Content/Characters/SK_Sorcerer.uasset` ·
`Content/Data/DT_Cards.uasset` · `Content/Materials/M_AncientGround.uasset` · `Content/Meshes/SM_Sorcerer.uasset` ·
`Content/Textures/T_Sorcerer_{D,N,ORM}.uasset` · `Content/UI/CardArt/T_CardArt_Sorcerer.uasset` ·
`Content/UI/WBP_CombatantHealthBar.uasset`

**Raw sources (12):** `Content/RawAssets/Sorcerer.fbx` **and** `Content/RawAssets/Characters/Sorcerer.fbx`
(**both committed, NOT de-duplicated** — 611,068 B static lane vs 820,348 B rigged lane, distinct content) ·
`Content/RawAssets/Characters/Anims/Sorcerer_{Attack,Death,Idle,Walk}.fbx` ·
`Content/RawAssets/Characters/Sorcerer.lod.json` · `Content/RawAssets/Textures/Sorcerer/T_Sorcerer_{D,N,ORM}.png` ·
`Content/RawAssets/Concepts/Sorcerer.png` · `Content/RawAssets/CardArt/Sorcerer.png`

**Tooling data, no QA gate required (2):** `Tools/ArtPipeline/pipeline_manifest.json` ·
`Tools/ArtPipeline/concept_prompts.json`

**Git LFS** confirmed active on `.uasset` / `.fbx` / `.png` (`git check-attr filter` → `lfs`).

---

## 4. 🔴 QA WARN-4 RULED — `rig_character.py` line endings (TASK-383 MUST READ THIS)

`qa/TASK-372-tooling-report.md` WARN-4 deferred this to build-master at TASK-378, with the test spelled out.
**Measured: `git diff --numstat` = 65 added / 3 removed.** That is decisively the *small-diff* branch, **not** the
~1191/1191 whole-file-rewrite branch.

**Root cause established rather than inferred:** **`core.autocrlf=true`** in this repo. Git's object store holds
**LF**, and `git show HEAD:…/rig_character.py | file -` confirms **HEAD already stores this file as LF**. The CRLF
QA observed exists **only in the working copy** and is normalised away on every `add`.

**RULING: there is ZERO EOL churn in commit A, and nothing is buried under it. TASK-383 must NOT renormalise the
file — it is already LF in Git, and a "renormalisation" commit would be a no-op.** TASK-383's WARN-1/2/3 + NIT-1/2
work is unaffected and still stands.

---

## 5. ⚠️ FOLLOW-UPS FOR THE MANAGER (reported, not acted on)

1. **`handoffs/SORCERER-DARKNESS-diagnosis.md` is UNCOMMITTED, on purpose.** It is the parallel art-director
   diagnosis agent's output (written 11:30, still in flight while I committed) and belongs to a **different lane**,
   not the ANCIENT-GROUNDS batch. Committing another agent's live file risked capturing a partial write.
   Pipeline-docs commits are always permitted, so that lane's own task should carry it. **It is the only batch-
   adjacent file left untracked besides `MI_Sorcerer_PBR`.**
2. **TASK-367 and TASK-369 were marked `done` from ARTIFACT EVIDENCE, not from a recorded completion — please
   confirm my reading.**
   - **TASK-367** (Jonathan's `BarStack` UMG wrap): the board still said `backlog`, but
     `WBP_CombatantHealthBar.uasset` is modified on disk and Jonathan's ship gate — whose entire boost-bar
     checklist depends on that wrap — PASSED. Jonathan never posted a completion for the task itself.
   - **TASK-369** (Sorcerer concept, `DECISION-NEEDED` on GATE A): the ruling appears to have been **option (1),
     accept the overshoot + a per-asset `proportions` override** — proven by `rig_manifest.json` now carrying the
     `Sorcerer.proportions` block (QA-reviewed) and by `Content/RawAssets/Concepts/Sorcerer.png` **existing** on
     disk, though the status text still claims it was "deliberately ABSENT" (that clause is **stale**). **The
     ruling itself was never written to the board.** Please record it properly.
3. **The board's `main 9 ahead UNPUSHED` claim was already corrected; the measured value after this task is
   `3 ahead`** (commits A, B, and this hash-recording docs commit). Any memory note still saying 9 is stale.
4. TASK-372's other open flags (24 fps clips, `A_Wizard_*` `bForceRootLock=false`, Sorcerer capsule `88/34` vs
   mesh-matched ≈`91/40`) and TASK-376's four follow-ups (durable SK-LOD tooling, the commandlet
   `get_vertex_count` gap, the `__ExternalObjects__` sweep filter) are **untouched by this pass and still open**.

---

## 6. UNBLOCKED BY THIS COMMIT

The ANCIENT-GROUNDS follow-up chains (TASK-379..394) and the entire FOLLOW-COMMAND batch code lane were
`blocked-by TASK-378` on **file ownership** (ruling 12). That block is now **released**. Per the board's own
serialization note, **TASK-379 → 380 → 389** should run promptly to unblock TASK-396, and TASK-379 must never run
concurrently with TASK-396.

**Machine state left as found:** editor open and untouched, MCP up, `L_Arena` not saved, no asset saved or created,
PIE not started. Working tree clean apart from the two deliberate exclusions above.
