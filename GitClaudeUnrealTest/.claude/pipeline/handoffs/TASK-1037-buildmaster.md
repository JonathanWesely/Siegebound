# TASK-1037 — [FOGAREA-TRACK] — build-master handoff

**Date:** 2026-09-05 · **Agent:** build-master · **Row:** `TASKBOARD.md` TASK-1037
**Outcome:** ✅ **27/27 STAGED. LFS 27/27 JOINED by oid. Registry entry REVERSED. NOT committed, NOT pushed — by instruction.**

🧑 **His ruling, verbatim:** *"yes turn on volumetric fog and add the 27 files to git"*
The first half (the volumetric flag) shipped under `TASK-1036`. **This row is the second half only.**

---

## 0. Fences honoured — declared up front

| fence | status |
|---|---|
| **NO COMMIT, NO PUSH** | ✅ Nothing committed. `HEAD` is still `7e3e883`. Staged-and-uncommitted is the delivered end state. |
| **No destructive Git** | ✅ Zero `checkout --` / `restore` / `stash` / `reset` / `clean`. The only mutating Git command in this entire row was **one `git add --pathspec-from-file`**. |
| **`L_Arena.umap` untouched** | ✅ Proven below (§5). |
| **No compile, no engine** | ✅ Zero builds, zero MCP calls. Editor **PID 17412 left alone and left up**. |
| **`BP_FogArea` never opened** | ✅ It was only ever hashed on disk and read as bytes. No save prompt was raised, therefore none was accepted. |
| **Board discipline** | ✅ Exactly **two** lines edited in `TASKBOARD.md`: the registry row (line 92) and TASK-1037's own `status:` (line 130). |
| **`CONVENTIONS.md`** | ✅ **Not touched by me.** It is dirty from another lane (123/1 vs HEAD); that dirt is not mine. |
| **`TASK-1008` (`SummonedUnit.*`)** | ✅ Different files; its row was not read for edit and not modified. |

---

## 1. The derived set — 27 files, and why the count is trustworthy

Derived with the **uncollapsed** form, as instructed:

```
git status --porcelain --untracked-files=all -- "GitClaudeUnrealTest/Content/FogArea"
```

⚠️ **The plain `--porcelain` form collapses the directory**: `Content/FogArea/` reads as **one** `??` entry while holding **27** files. Any count taken from the collapsed form is false. Every number in this report comes from the expanded form or from `git diff --cached`.

⚠️ **The repo root sits ABOVE the project directory.** Every pathspec is prefixed `GitClaudeUnrealTest/` — the doubled project name is correct, not a typo. A wrong pathspec here stages nothing and reports success.

**Before:** 27 files, **0 tracked**. No `.gitignore` rule applies to any of them (`git check-ignore` → 0 hits), so nothing was force-added.

| # | path (relative to `Content/FogArea/`) | bytes |
|---|---|---|
| 1 | `Blueprints/BP_FogArea.uasset` | 486,192 |
| 2 | `Data/E_FogAreaColorChannel.uasset` | 3,154 |
| 3 | `Data/E_FogAreaMaterialType.uasset` | 2,456 |
| 4 | `Data/E_FogAreaMode.uasset` | 3,072 |
| 5 | `Data/S_FogAreaField.uasset` | 7,283 |
| 6 | `Data/S_FogAreaGeneral.uasset` | 9,606 |
| 7 | `Data/S_FogAreaNoise.uasset` | 11,811 |
| 8 | `Data/S_FogAreaShadows.uasset` | 6,574 |
| 9 | `Data/S_FogAreaShafts.uasset` | 7,167 |
| 10 | `Data/S_FogAreaShape.uasset` | 8,596 |
| 11 | 🧑 **`Maps/Overview.umap`** — **VENDOR DEMO MAP, FLAGGED, see §4** | **150,655** |
| 12 | `Materials/Base/MI_FogArea_Box.uasset` | 8,336 |
| 13 | `Materials/Base/MI_FogArea_Box_DF.uasset` | 10,970 |
| 14 | `Materials/Base/MI_FogArea_Box_Shadows.uasset` | 9,837 |
| 15 | `Materials/Base/MI_FogArea_Box_Shafts.uasset` | 9,889 |
| 16 | `Materials/Base/MI_FogArea_DF.uasset` | 10,604 |
| 17 | `Materials/Base/MI_FogArea_Shadows.uasset` | 9,704 |
| 18 | `Materials/Base/MI_FogArea_Shafts.uasset` | 9,760 |
| 19 | `Materials/Functions/MF_Fog.uasset` | 87,536 |
| 20 | `Materials/Functions/MF_Shapes.uasset` | 18,644 |
| 21 | `Materials/M_FogArea.uasset` | 21,581 |
| 22 | `Textures/T_Noise_Shafts.uasset` | 155,363 |
| 23 | `Textures/T_Star.uasset` | 159,232 |
| 24 | `Textures/T_Volume_Curl_01.uasset` | 205,600 |
| 25 | ⚠️ `Textures/T_Volume_Noises_01.uasset` | **13,537,551** |
| 26 | `Textures/VT_Curl_Low.uasset` | 192,784 |
| 27 | ⚠️ `Textures/VT_Noises.uasset` | **13,591,130** |

**Pack total: 28,735,087 B = 27.40 MB.** ⚠️ **94% of that is two volume textures** (#25 + #27 = 27.13 MB). Everything else in the pack together is ~1.5 MB. Not a problem — they are LFS pointers in the tree — but it is the number to know before anyone clones.

Staging used the **derived explicit list**, not a directory sweep:

```
git add --pathspec-from-file=<the 27 derived paths>
```

## 2. Read-back — 27, from the index, not from a clean status

```
git diff --cached --name-only -- "GitClaudeUnrealTest/Content/FogArea"  →  27
```

⛔ A clean `git status` cannot distinguish *staged* from *never-modified*, so it was never used as the instrument.

✅ **The staged set is EXACTLY those 27 and nothing else.** `git diff --cached --name-only` with no pathspec returns 27 paths, **0 of them outside `Content/FogArea/`.**

📌 **Incidental, worth recording:** the session-start snapshot showed `Content/UI/CardArt/T_CardArt_{BrightSun,Fog}.uasset` as already-staged (`A `). They are **no longer** in the index because **two commit hosts landed in between** — `1a457df` (TASK-987) and `7e3e883` (TASK-1034) — and swept them in. Verified by reflog and `git ls-tree HEAD`. Not my doing, and it means my `git add` inherited a **clean index**, which is why the full staged set is exactly my 27.

## 3. ⛔ The LFS check — by OID, never by size, never by `git show | sha256sum`

Both `.uasset` and `.umap` are LFS patterns in the **root** `.gitattributes`. `git check-attr filter` returns **`lfs` for all 27** (27/27, not a default).

**Method** — for each staged path: read the staged blob with `git cat-file -p :<path>`, require its first line to be the pointer header `version https://git-lfs.github.com/spec/v1`, parse `oid sha256:<hex>` and `size`, and compare that oid against **`sha256sum` of the working file on disk**.

- ⛔ **Not by size** — size is corroboration only; the join is the oid.
- ⛔ **Not by `git show | sha256sum`** — for a `.uasset` that returns **pointer text**, and it has already reported correct files as DIVERGED in this project.

### Verdict: **JOINED 27 · MISMATCH 0 · RAW BLOBS 0 · MISSING OIDS 0**

⭐ **The `TASK-928` near-miss is excluded by construction:** a sweep that joins *nothing* and a sweep that matches *everything* both report `0` mismatches. **This sweep reports `JOINED=27`, not `JOINED=0`** — 27 real oid pairs were compared. Every path in the list below is a positive join.

| oid (sha256) | bytes | file |
|---|---|---|
| `cbe615d2ded6eae79041016ec2d4a7ef12a304f0590d53aa3a66d66252584d90` | 486192 | `Blueprints/BP_FogArea.uasset` |
| `9161ee55f4d2c735f71ab3c287e8ea3f884b720787d1e5062a507532c16b2729` | 3154 | `Data/E_FogAreaColorChannel.uasset` |
| `59b460f564f1807904d9bcab7b86a14b355dd174c5d1931b181cda621ac4b158` | 2456 | `Data/E_FogAreaMaterialType.uasset` |
| `ea7b981125e56a81e15d75dc72a1843c7d8ad5b7bd2086ebb9ceb3c537a3a703` | 3072 | `Data/E_FogAreaMode.uasset` |
| `54ef343a71a35d1a73b0b25c96ead02184f6ea5856fa7de741115bb5d6bacec9` | 7283 | `Data/S_FogAreaField.uasset` |
| `b4218b12705a533dfab20da9646f408c389d4bc9c169b7675e22316a3e1bdb7d` | 9606 | `Data/S_FogAreaGeneral.uasset` |
| `677a8cae11ed4f36e14892ff40d86c8cee13b545dcdd1a81795e83753b4ce1fb` | 11811 | `Data/S_FogAreaNoise.uasset` |
| `d58680a3cfb54cd1677a118aa68fb791e5938d49bc8da501b01daf4759213268` | 6574 | `Data/S_FogAreaShadows.uasset` |
| `1cdcf7121ed6409cd1a02627955a982c6980a08adc8356facb4e2e65e46b93cb` | 7167 | `Data/S_FogAreaShafts.uasset` |
| `f33f534c61af3683a701e10f0f1d572dd6f906a6261a6d8b8535f30f1ee4b953` | 8596 | `Data/S_FogAreaShape.uasset` |
| `bcb8b8c74d6b47c351bb8cd83bf798ba59982dd8f492a3144bb8a5121a4e3dfc` | 150655 | 🧑 `Maps/Overview.umap` |
| `edb5c24ccb723edd61d0302350e4a084d40e5d4f6c092e75d63d35d1b94bee45` | 8336 | `Materials/Base/MI_FogArea_Box.uasset` |
| `eb19704898f062dfb87014ff3c6835e2fd6b922117ab1965fc3680f50bfaa331` | 10970 | `Materials/Base/MI_FogArea_Box_DF.uasset` |
| `1df19ff09fe80752a9235f9e64ed5490ad9ee741eeb444387148f50988fc531e` | 9837 | `Materials/Base/MI_FogArea_Box_Shadows.uasset` |
| `eae5c97c6ebb7f7d9630cb6377b8740f03cfdc6fa8b6b18275d5d8a714b7e9c2` | 9889 | `Materials/Base/MI_FogArea_Box_Shafts.uasset` |
| `b70ca390897c21785796924709e3331cb2464a45acaa0869d8ad679b0977c6e3` | 10604 | `Materials/Base/MI_FogArea_DF.uasset` |
| `f3153dd733ac9f80ee855c3320b2a634cdeac216266fd1a220e00748b1719f7c` | 9704 | `Materials/Base/MI_FogArea_Shadows.uasset` |
| `72d6a9177b9cff83619f9c16cbd05afbc90dfe0a9a0feb2a766517581e538dd1` | 9760 | `Materials/Base/MI_FogArea_Shafts.uasset` |
| `b52cddd174c820025bdd70075bc4485abc7e9d86f474046d35370e78ffbe407b` | 87536 | `Materials/Functions/MF_Fog.uasset` |
| `90a1da0b2c059f5820f0d2eae8a4da237d4b1fef1973966beaab5b5059a38535` | 18644 | `Materials/Functions/MF_Shapes.uasset` |
| `e61242c28f2de9571729182af2a6723c71489911444b128709cf361fcba0d23c` | 21581 | `Materials/M_FogArea.uasset` |
| `e49fd69d92469bd33107601b8e0dbeb19815bddd830d175b235659067b806d65` | 155363 | `Textures/T_Noise_Shafts.uasset` |
| `e16bc6eaf90a952c0acf75fff26b61198c2a33298d519ae1cc95c8cbeabdfb9c` | 159232 | `Textures/T_Star.uasset` |
| `ceea4805e7827a4d226fedb32afacef3f866a04e85b33bab09a29d7b2ea02066` | 205600 | `Textures/T_Volume_Curl_01.uasset` |
| `a4eacc5aa8214899c042e3be40b0e850866d47772ceca0213bea9e9e7fcf5323` | 13537551 | `Textures/T_Volume_Noises_01.uasset` |
| `77c7a6c2e3c9ab724b3ad9b8259445c49ef604336eb9ec7c9c9d3132dc78198b` | 192784 | `Textures/VT_Curl_Low.uasset` |
| `0adebce60c577ee42f39c19b5b96274bb658f6503c12d5fa2708780f65870aec` | 13591130 | `Textures/VT_Noises.uasset` |

**Corroboration (a success return is not evidence):** the LFS objects are physically present in the local store — spot-checked `.git/lfs/objects/<aa>/<bb>/<oid>` for the two 13 MB textures and `BP_FogArea`, all **PRESENT at the exact expected byte length**. So the bytes exist to back the pointers, not just the pointers.

**No raw blob entered the index.** Nothing to report and stop on.

## 4. 🧑 THE ONE QUESTION — reported, not decided

**`Content/FogArea/Maps/Overview.umap` (150,655 B) is a vendor demo map.** It is not used by the game, it enlarges the repo, and a stray `.umap` sitting in `Content/` is the sort of thing that gets opened by accident.

⛔ **It is STAGED, deliberately.** 🧑 His ruling said *"the 27 files"* ⇒ **all 27 is the default, and no agent narrows his instruction.** I did not exclude it on my own judgement.

🧑 **If you want it out, one word does it** — it is a single `git restore --staged` on that one path before the commit host runs, and the row becomes 26. Nothing else in the pack depends on it. (Also worth knowing: it is the file where the vendor's *real look* lives — `FOG-§9` records `/Game/FogArea/Maps/Overview` as never opened and *"worth its own granted task"*. Deleting it costs that reference.)

## 5. ✅ `L_Arena.umap` was NOT touched

`Content/Maps/L_Arena.umap` carries `TASK-1036`'s one-flag change and is gate-pinned. Measured **before** staging and again **after**:

| | before | after |
|---|---|---|
| git state | ` M` (modified, **unstaged**) | ` M` (modified, **unstaged**) |
| sha256 | `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` | **identical** |
| in the index | no | **no** — `git diff --cached --name-only` contains no `L_Arena` |

It was not staged, not modified, and no pathspec of mine could have reached it: the add was a fixed 27-path list, and `Content/Maps/` is not under `Content/FogArea/`. The hash still matches TASK-1036's recorded NEW value, so that row's evidence is intact and its `CONVENTIONS.md` re-pin remains outstanding for the manager (still owed — not my row).

## 6. ⛔ The registry reversal — done in the SAME action

`TASKBOARD.md` line **92**, the `Content/FogArea/**` row of the **Standing Exclusion Registry**, previously read `DO NOT STAGE — AWAITING 🧑 J-F11` with a **CONTESTED 2026-09-05** marker. It now reads **RETIRED — THE PROHIBITION IS LIFTED**, and records: his verbatim ruling as the authorisation, the host row (`TASK-1037`), the 27 read-back, the LFS 27/27 join, and the fact that a clean clone no longer breaks `TASK-841`.

**Why I was entitled to remove it.** The registry's own rule reads: *"REMOVING ONE REQUIRES THE NAMED AUTHORISATION IN THAT ROW'S OWN CELL."* That cell named 🧑 **`J-F11`** — and **he answered `J-F11`**. The authorisation is delivered, so this is a retirement **by its own terms**, not a softening.

**What I deliberately did NOT retire.** The staging prohibition is the only thing lifted. The vendor-pack clauses are **kept verbatim in the new row**: nothing inside `Content/FogArea/**` is edited, moved or deleted; `BP_FogArea` is **not opened** (it dirties its own package on load); every save prompt is declined. Tracking the pack does not make it ours.

⚠️ **MANAGER — please RATIFY.** The board spec's item (1) says the manager owns the registry and build-master should merely *report*; the dispatch that launched me made the reversal **this row's deliverable** and told me to do it in the same action as the staging. I followed the dispatch, because leaving a `DO NOT STAGE` standing over files that are now tracked would make the registry **a liar in its first week** and every future commit host would keep skipping them. **The edit is yours to ratify or reword** — the substance that must survive is: *these files are tracked; do not re-exclude them; the vendor no-edit rule still stands.*

## 7. ⛔ Commit status — and a noted tension

**Nothing was committed. Nothing was pushed.** `HEAD` remains `7e3e883`.

⚠️ **Noted for the record:** the board spec's item (7) says *"COMMIT: fold into the NEXT commit host, or take one of your own if none is open"*, while my dispatch says **"NO COMMIT. NO PUSH. Staged-and-uncommitted is the correct end state."** I obeyed the **dispatch** — it is the later and more specific instruction, and the conservative reading (leaving bytes staged) is trivially reversible while an unwanted commit is not.

⇒ **The next commit host inherits 27 staged paths.** It must **not** re-derive its pathspec from `git status` and assume these are untracked — they are already in the index. When it commits, the message should say plainly that these are **VENDOR files entering git on 🧑 his ruling**.

---

## Files

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\FogArea\**` — 27 files, staged
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\TASKBOARD.md` — line 92 (registry row, reversed), line 130 (TASK-1037 status). Not staged.
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\handoffs\TASK-1037-buildmaster.md` — this file
