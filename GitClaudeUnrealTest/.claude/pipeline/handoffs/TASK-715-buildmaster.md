# TASK-715 — build-master handoff — THE STAGE PRUNE + RE-MEASURE

**Verdict: ✅ PRUNE COMPLETE · `PKG-§10` INVARIANT ASSERTED AND HOLDING — exactly ONE runnable game exe survives, and it is the one the manifests name.**
Law: `PKG-§10` (authorizing) · `PKG-§9d` (sizes) · `PKG-§7a` (fence, re-proven). Date 2026-08-30.
⛔ No zip · ⛔ no commit, no git writes of any kind · ⛔ nothing under `Source/`/`Content/`/`Config/` touched · ⛔ no editor, no MCP call, no cook, no compile · ⛔ no arena verification attempted (TASK-716 owns it).

**Write surface this task actually touched: 2 file deletions inside the staged tree, and this handoff. Nothing else.**

---

## 1. ENUMERATION — DONE BEFORE ANYTHING WAS TOUCHED

**Method (`PKG-§10`: the manifests are the authority):** union the three UAT-emitted manifests, normalize separators, and set-difference against a full recursive listing of the stage. ⛔ **Matched on FULL RELATIVE PATH, never on basename** — §2b below is why that is not pedantry.

| Manifest | Entries |
|---|---|
| `Manifest_UFSFiles_Win64.txt` | 3,446 |
| `Manifest_NonUFSFiles_Win64.txt` | 40 |
| `Manifest_DebugFiles_Win64.txt` | 3 |
| **union (unique paths)** | **3,489** |

**Stage before the prune: 72 files, 2,480,649,570 B (2.3103 GiB)** — reproduces TASK-699's figure exactly.

### 1a. The full NON-MANIFEST set — all 28 files, before any deletion

⚠️ **28 files are in no manifest. Only 2 of them are orphans.** The other 26 legitimately belong to the artifact. This table is the whole reason step 1 exists as a separate step:

| Bytes | Path | Ruling |
|---:|---|---|
| 8,161,680 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_api_dump.dll` | KEEP — engine Vulkan layer |
| 13,848 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_api_dump.json` | KEEP |
| 2,890,128 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_crash_diagnostic.dll` | KEEP |
| 23,055 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_crash_diagnostic.json` | KEEP |
| 8,166,288 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_gfxreconstruct.dll` | KEEP |
| 28,099 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_gfxreconstruct.json` | KEEP |
| 1,485,200 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_khronos_profiles.dll` | KEEP |
| 66,696 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_khronos_profiles.json` | KEEP |
| 415,632 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_khronos_shader_object.dll` | KEEP |
| 4,455 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_khronos_shader_object.json` | KEEP |
| 397,192 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_khronos_synchronization2.dll` | KEEP |
| 1,910 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_khronos_synchronization2.json` | KEEP |
| 19,509,136 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_khronos_validation.dll` | KEEP |
| 95,917 | `Engine/Binaries/ThirdParty/Vulkan/Win64/VkLayer_khronos_validation.json` | KEEP |
| 69,056 | `Engine/Binaries/ThirdParty/Windows/WinPixEventRuntime/x64/WinPixEventRuntime.dll` | KEEP |
| 195,428 | `Engine/Extras/GPUDumpViewer/GPUDumpViewer.html` | KEEP — engine extras |
| 879 | `Engine/Extras/GPUDumpViewer/OpenGPUDumpViewer.bat` | KEEP |
| 2,165 | `Engine/Extras/GPUDumpViewer/OpenGPUDumpViewer.sh` | KEEP |
| 25 | `Engine/Saved/Config/Windows/Manifest.ini` | KEEP |
| **347,694,080** | **`GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest.exe`** | ⛔ **DELETE — orphan** |
| **401,526,784** | **`GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest.pdb`** | ⛔ **DELETE — orphan** |
| 1,077,741,392 | `GitClaudeUnrealTest/Content/Paks/GitClaudeUnrealTest-Windows.ucas` | ⛔⛔ **KEEP — THIS IS THE GAME** (trap §2a) |
| 664,292 | `GitClaudeUnrealTest/Content/Paks/GitClaudeUnrealTest-Windows.utoc` | ⛔⛔ **KEEP** |
| 3,307,440 | `GitClaudeUnrealTest/Content/Paks/global.ucas` | ⛔⛔ **KEEP** |
| 806 | `GitClaudeUnrealTest/Content/Paks/global.utoc` | ⛔⛔ **KEEP** |
| 246 | `Manifest_DebugFiles_Win64.txt` | KEEP — spec (5) names them |
| 3,231 | `Manifest_NonUFSFiles_Win64.txt` | KEEP |
| 318,308 | `Manifest_UFSFiles_Win64.txt` | KEEP |

### 1b. THE DELETE LIST — the complete set, and it is exactly two files

| # | Path (stage-relative) | Bytes | UFS | NonUFS | Debug |
|---|---|---:|---|---|---|
| 1 | `GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest.exe` | 347,694,080 | **0** | **0** | **0** |
| 2 | `GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest.pdb` | 401,526,784 | **0** | **0** | **0** |
| | **TOTAL** | **749,220,864 B (714.51 MiB)** | | | |

✅ **The 0-hit result was RE-VERIFIED here, per-manifest, from the manifest files themselves — ⛔ not taken from TASK-699's relayed list**, exactly as spec (1) requires. Both files: 0 hits in all three.

**And the identity check proves these are the stale *Development* binaries, ⛔ not a name coincidence:**

| Field | The deleted `GitClaudeUnrealTest.exe` |
|---|---|
| ProductName / FileDescription | `Third Person Game Template` |
| CompanyName | `Epic Games, Inc.` |
| LegalCopyright | `Fill out your copyright notice in the Description page of Project Settings.` |
| ProductVersion | `5.8.0` |
| LastWriteTime | **2026-08-29 16:56** (the Aug-29 Development cook) |
| SHA-256 | `9E4B468EF2EB4F91770223F2BFA404D4489A5C1307563657A46C1F56B7ED6A2E` |

⇒ This is precisely the `PKG-§10` hazard: a file named `GitClaudeUnrealTest.exe`, sitting in `Binaries\Win64\`, that a curious player double-clicks and gets a build whose own properties say *Third Person Game Template*.

### 1c. ✅ NOTHING UNIQUE WAS DESTROYED — verified before deleting

Both files exist **byte-size-identical, same mtime (Aug 29 16:56)** in the repo's own build directory `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Binaries\Win64\`. The stage copies were duplicates. ⇒ regenerable by re-cook *and* already present elsewhere; **TASK-699's "Development exe as an instrument" route survives at the repo path** if it is ever wanted again.

---

## 2. ⚠️⚠️ TWO TRAPS MEASURED HERE — BOTH WOULD HAVE BEEN CATASTROPHIC, IN OPPOSITE DIRECTIONS

> ### These are the finding of this task. The deletion itself was thirty seconds of work.

### 2a. THE DELETE-DIRECTION TRAP — "delete everything not in a manifest" DELETES THE ENTIRE GAME

**The `.ucas`/`.utoc` containers appear in NO manifest.** `GitClaudeUnrealTest-Windows.ucas` is **1,077,741,392 B — the whole cooked game — and it is a non-manifest file.**

**Why:** the UFS manifest lists the **3,446 logical assets that were packed INTO** the container (`Content/Maps/L_Arena.umap` etc.), ⛔ **not the container itself.** Confirmed by inspection: the UFS manifest contains **zero** `.exe`/`.pdb` entries and zero container entries. ⚠️ **The `.pak` IS manifest-listed while its sibling `.ucas`/`.utoc` are not** — an inconsistency that makes the naive rule look safe right up until it isn't.

⇒ ⛔ **`PKG-§10`'s scope word — "every non-manifest GAME BINARY" — is LOAD-BEARING and must never be generalized to "every non-manifest file."** A future agent (or a `ship.ps1` gate) that implements the shorter sentence ships an empty package. **Recorded as the reason the law is worded the way it is.**

### 2b. THE KEEP-DIRECTION TRAP — a basename grep says the orphan IS in the manifest

`grep -F "GitClaudeUnrealTest.exe" Manifest_NonUFSFiles_Win64.txt` returns **1 hit**, line 39:
```
GitClaudeUnrealTest.exe	2026-08-30T07:07:48.134Z
```
⚠️ **That hit is the ROOT SHIM** (`Windows\GitClaudeUnrealTest.exe`, 172,032 B, the `BootstrapPackagedGame` bootstrap) — **a completely different file from the 347 MB orphan in `Binaries\Win64\`.** They share a basename and nothing else.

⇒ An agent checking membership by filename concludes *"it's in the manifest, keep it"* and **the hazard survives the prune while the report claims compliance.** ⛔ **Manifest membership MUST be tested on the full relative path.** This is the same wrong-thing-measured failure family as TASK-699's §3(d) warning about reading the wrong exe's properties.

---

## 3. THE PRUNE — the record

A pre-delete guard re-`stat`ed each target and **refused to delete unless the byte size matched the enumerated value exactly** (a path-mixup interlock); both passed. No game process was running (verified: 0). Then:

```
PRE-DELETE GUARD OK : ...\Binaries\Win64\GitClaudeUnrealTest.exe  347,694,080 B
PRE-DELETE GUARD OK : ...\Binaries\Win64\GitClaudeUnrealTest.pdb  401,526,784 B

DELETED + CONFIRMED ABSENT : ...\Binaries\Win64\GitClaudeUnrealTest.exe
DELETED + CONFIRMED ABSENT : ...\Binaries\Win64\GitClaudeUnrealTest.pdb

BYTES FREED : 749,220,864 B  (714.51 MiB)
```

⛔ **Nothing else was deleted** — redists, `NOTICES.txt`, the three manifests, the Vulkan layers, the GPUDumpViewer extras and all four Pak-folder containers are **untouched**. ✅ **Zero ambiguous items** — every one of the 28 non-manifest files resolved cleanly to KEEP or DELETE, so spec (2)'s "leave it and report it" branch was never needed.

---

## 4. ✅ THE `PKG-§10` INVARIANT — ASSERTED, HOLDING

**Every `.exe` remaining in the staged tree, exhaustively:**

| Bytes | Path | What it is |
|---:|---|---|
| 11,751,688 | `Engine/Extras/Redist/en-us/vc_redist.arm64.exe` | VC++ redist **installer** — in-manifest, not a game binary |
| 18,569,648 | `Engine/Extras/Redist/en-us/vc_redist.x64.exe` | VC++ redist **installer** — in-manifest, not a game binary |
| 172,032 | `GitClaudeUnrealTest.exe` | **the root shim** (`BootstrapPackagedGame`) — in-manifest, the click target |
| **177,716,736** | **`GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest-Win64-Shipping.exe`** | ⭐ **THE ONE RUNNABLE GAME EXE** |

> ### ✅ **ASSERTION PASSES: exactly ONE runnable game exe, plus the root shim — and it is the one the manifests name** (`Manifest_NonUFSFiles_Win64.txt` line 4).

**THE SURVIVOR, NAMED IN FULL:**
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Windows\GitClaudeUnrealTest\Binaries\Win64\GitClaudeUnrealTest-Win64-Shipping.exe`

**THE CLICK TARGET a player double-clicks** (for TASK-700's README, `PKG-§10` rider R2):
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Windows\GitClaudeUnrealTest.exe`

✅ **The hazard folder is now unambiguous.** `Binaries\Win64\` contains exactly `…-Win64-Shipping.exe` + `…-Win64-Shipping.pdb`. **There is no longer a wrong exe to click.**
✅ **Re-ran the full set-difference post-prune: NON-MANIFEST `.exe`/`.pdb` remaining = 0.**

---

## 5. RE-MEASURE (`PKG-§9d`) — AND IT RECONCILES TO THE BYTE

| | Files | Bytes | GiB |
|---|---:|---:|---:|
| TASK-699 staged tree (before) | 72 | 2,480,649,570 | 2.3103 |
| **Pruned stage (after)** | **70** | **1,731,428,706** | **1.6125** |
| **Delta** | −2 | **−749,220,864** | **−714.51 MiB (−30.2%)** |

✅ **`2,480,649,570 − 749,220,864 = 1,731,428,706` — EXACT MATCH.** ⛔ No unexplained residue; the delta is 100% accounted for by the two named files.
⭐ **And it lands exactly on TASK-699's predicted "staged tree minus orphans" figure of 1,731,428,706 B (1.61 GiB)** — 699's arithmetic is confirmed independently, from the pruned tree itself rather than by subtraction.
⭐ **The size story now reads correctly for the first time:** the Shipping artifact is **−15.7%** against the Development baseline (2,053,734,759 B), which is what a Shipping build should do. `PKG-§9d`'s smoke alarm is silent because the smoke is gone.

### The surviving exe's identity (`PKG-§10` step 4 — proving the prune did not remove the thing we ship)

| Field | Value — re-read from disk AFTER the prune | vs 699's S2 |
|---|---|---|
| ProductName | **`Siegebound`** | ✅ match |
| FileDescription | **`Siegebound`** | ✅ match |
| CompanyName | **`Jonathan Wesely`** | ✅ match |
| LegalCopyright | **`Copyright 2026 Jonathan Wesely. All Rights Reserved.`** | ✅ match |
| OriginalFilename | `GitClaudeUnrealTest-Win64-Shipping.exe` | ✅ expected S1 residue |
| InternalName | `GitClaudeUnrealTest` | ✅ expected S1 residue |
| ProductVersion | `++UE5+Release-5.8-CL-55116800` | ✅ match |
| Size | 177,716,736 B | ✅ match |

⭐ **Stronger than a properties re-read: SHA-256 before the prune == SHA-256 after.**
`A853A1E5369EDFDD31A3346DB8FDF3367E1F7FE97806C331EE05D49DC6AB8472` — **byte-identical.** The shim likewise unchanged (`7F2C8FD6…F4B5`). **The prune provably did not touch the artifact.**

---

## 6. LIVENESS SANITY — CHEAP, AND IT BOUGHT ONE EXTRA FACT

Launched **via the root shim, no args (the player's double-click path)**:

```
t+ 10s : PID  28688  GitClaudeUnrealTest                 WS     7 MB  Title=''
t+ 10s : PID   9104  GitClaudeUnrealTest-Win64-Shipping  WS 1,640 MB  Title='Siegebound'
t+ 20s : PID   9104  ...                                 WS 1,641 MB  Title='Siegebound'
t+ 30s : PID   9104  ...                                 WS 1,644 MB  Title='Siegebound'
```

✅ Process alive and stable **30 s**, ~1.64 GB working set, no crash, no crash dump. ✅ **Window title `Siegebound`.**
⭐ **THE EXTRA FACT — the shim's target resolution was proven, not assumed.** Resolving PID → path shows the shim (28688) launched
`…\GitClaudeUnrealTest\Binaries\Win64\GitClaudeUnrealTest-Win64-Shipping.exe` (9104). ⚠️ **Worth having: with two exes present, which one the shim picked was an open question; with one present it is now a closed one, and I measured the chain end-to-end rather than inferring it.**
📌 **Corroborates 699's `PKG-§9a` route-2 log-silence finding independently: the run created NO `Saved/` directory at all.** Nothing to purge; stage file count stayed 70.
✅ Both processes closed cleanly (`CloseMainWindow`); **remaining game processes: 0.**

⛔ **THIS IS NOT A BOOT-VERIFY AND I AM NOT REPORTING ONE.** Liveness + title only. ⛔ **No arena verification was attempted** — TASK-716 owns it, it needs an unlocked desktop, and `PKG-§6a` is explicit that a menu is not a pass. ⛔ No pixel claim, no gameplay claim.

---

## 7. FENCES — ALL HELD

- ⛔ **Zero git writes.** No `add`, no `commit`, no `push`, no branch, no stash. `PKG-§7a` fence property re-proven *after* my changes: toplevel `C:/GitProjects/GitHub/GitClaudeUnrealTesting`; **tracked under `packagedZIPofGame` = 0**; **`git status --porcelain` hits for the stage = 0**; `git check-ignore -v` → `.gitignore:19:packagedZIPofGame/`. ⇒ **My deletions are invisible to Git, as intended.**
- ⛔ **Nothing under `Source/`, `Content/`, or `Config/` touched.**
- ⛔ **No editor contact, no MCP call of any kind** — the node-identity law had no surface to apply to. `L_Arena` untouched (ledger `9ccd54ef…0e58`); I neither opened nor hashed anything in the project.
- ⛔ No zip written, no README touched (TASK-700 owns both). The Development zip `Siegebound-Win64-Development-2026-08-29.zip` (1,409,955,049 B) is **untouched** (`PKG-§7b` row S4).
- ⛔ No cook, no compile.
- **Disk:** 1.3 TB free — headroom is not a constraint for TASK-700's zip.

## 8. DEVIATIONS

**None.** Every step of the spec was executed as written, in order. The prune ran **before** any boot-verify (`PKG-§10` ordering, spec (6)); no item was ambiguous, so the report-don't-guess branch was never exercised.

---

## 9. FINDINGS FOR THE MANAGER

1. ⛔⛔ **THE RULED `ship.ps1` STAGE-HYGIENE GATE DOES NOT EXIST — MEASURED, NOT ASSUMED.** `PKG-§10` rules *"THIS BECOMES A `ship.ps1` GATE, ⛔ not a one-off TASK-700 cleanup."* Grepping `Tools/Packaging/ship.ps1` (1,504 lines): **`Manifest_` = 0 hits · `NonUFSFiles` = 0 · `DebugFiles` = 0 · `orphan` = 0.** The script's only prune is **`D3-PRUNE`, which is *zip retention* (`PKG-§7b`) — a different thing entirely.** ⚠️ **Consequence: TASK-715 cleans TODAY's stage; the NEXT cook re-creates the orphan and the next `/ship` re-ships the hazard.** ⇒ **Needs its own task against `ship.ps1`** (a phase-C gate, after stage, before boot-verify). ⛔ Not fixed here — `ship.ps1` is code, it is in the QA lane, and this task's fence is the staged tree.
2. ⛔⛔ **WRITE TRAP §2a INTO THE LAW BEFORE ANYONE IMPLEMENTS THAT GATE.** *"Delete every non-manifest file"* deletes the **1.078 GB `.ucas` — the entire game.** The containers are in no manifest because the UFS manifest lists their *contents*; and the `.pak` being listed while the `.ucas`/`.utoc` are not makes the naive rule look safe. **`PKG-§10`'s "game binary" scope is load-bearing.** A `ship.ps1` gate must whitelist by extension/role, ⛔ never by bare manifest absence.
3. ⛔ **AND TRAP §2b:** membership must be tested on the **full relative path** — a basename grep for `GitClaudeUnrealTest.exe` hits the **root shim** and silently exonerates the orphan. **Both traps belong in the gate's comments, in the same place TASK-701 was told to comment the `-COOKDIR` story.**
4. ✅ **TASK-700's rider R2 is now dischargeable:** there is one runnable game exe, the click target is unambiguous, and both absolute paths are in §4 above for the README.
5. ✅ **TASK-716 is unblocked** and will be verifying **the artifact we actually ship** — `PKG-§10`'s ordering requirement is satisfied.
6. 📌 **699's Development-exe instrument is not lost** (§1c) — identical copies remain in the repo build dir. Nothing that route needs was destroyed.
7. 🙋 **FOR-JONATHAN, unchanged from 699 and still true:** the file you double-click (`Windows\GitClaudeUnrealTest.exe`) is the engine's bootstrap shim, so *its* properties will always read `BootstrapPackagedGame` / `Epic Games, Inc.` **Your name is on the real game exe** — now the only one in the package, and this run proved the shim launches exactly it.

## 10. STATE AT HANDOFF

- Stage: **70 files, 1,731,428,706 B (1.6125 GiB)** at `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Windows\` — pruned, measured, one-exe invariant holding.
- 0 game processes running · no `Saved/` residue · nothing staged, committed or pushed · my only tree addition is this handoff.
