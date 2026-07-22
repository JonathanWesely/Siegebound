# Handoff — TASK-225 — Recorded-dark albedo fix: per-asset albedo_delight rebake

## Part A — headless Stage-2 rebakes (art-director, 2026-07-19) — DONE

**Status:** part-A DONE (all 5 targets rebaked, REAL outputs in Content/RawAssets). **Part B (editor same-path
reimport) NOT run — separate dispatch.** NO editor, NO reimport, NO Git by me. Ruling B honored: NO fleet
retexture; delight-tuning of the recorded-dark set only. Wall/DeepMine untouched (no cache — not targets).

### Targets & cache verification
All 5 recorded-dark assets had cached donors (`Tools/ArtPipeline/Cache/<CardID>/trellis_raw.glb`) — **nothing
skipped, no TRELLIS regeneration attempted**: Ogre (22.6 MB), Knight (23.4 MB), Cavalry (19.6 MB),
Pikeman (20.5 MB), MilitiaMob (18.9 MB).

### Manifest edit (main-lane M7.5, authorized by the spec)
`Tools/ArtPipeline/pipeline_manifest.json`: per-asset `albedo_delight` override objects added to EXACTLY
Ogre, Knight, Cavalry, Pikeman, MilitiaMob (verified no other asset carries the key) + the `_doc.albedo_delight`
note updated to record the TASK-225 pinning. No other manifest changes.

### Values chosen (per asset) — all 5 shipped at the TASK-195 tuned values
`{"ao_divide_strength": 1.0, "ao_floor": 0.25, "gamma": 0.55, "gain": 1.2}`

**Why no per-asset tempering:** the mandate allowed tempering only for OVER-lift. Eyeball gate on the workbench
texture-color previews found none: Pikeman/MilitiaMob's pale regions (hood, pauldrons, mail) were already pale
in the dark donors — the TASK-193 soft shoulder held them (≤1.0% of pixels soft-COMPRESSED, zero hard clipping
anywhere) while the near-black regions recovered. Cavalry is the opposite case: still the darkest after lift
(donor is genuinely near-black — dark-armored knight on dark horse); pushing STRONGER than the proven values was
outside the granted latitude, so it ships at tuned with the residual-darkness recorded below.

### Per-asset results (from Cache/<CardID>/refine_report.json — config applied=true on all 5)

| Asset | mean linear before→after | p99 after | shoulder frac | team slot0 | tris | run |
|---|---|---|---|---|---|---|
| Ogre | 0.0042 → 0.0586 (14.0×) | 0.426 | 0.0003 | 2.1% (cap 35%) | 15000 | 43.0s |
| Knight | 0.0059 → 0.0597 (10.1×) | 0.481 | 0.0021 | 3.5% | 15000 | 67.2s |
| Cavalry | 0.0009 → 0.0193 (21.4×) | 0.194 | 0.0 | 4.3% | 15000 | 16.7s |
| Pikeman | 0.0152 → 0.1490 (9.8×) | 0.660 | 0.0002 | 9.1% | 15000 | 39.5s |
| MilitiaMob | 0.0140 → 0.1284 (9.2×) | 0.809 | 0.0103 | 2.8% | 15000 | 14.0s |

- **WARN-parity:** each report's warnings list is BYTE-IDENTICAL to the shipped-dark report (only the known
  fit_mode=height silhouette-dims warns — expected-warns only, nothing new).
- **Slots:** `[TeamRegion, <CardID>PBR]` on all 5; team fractions identical to the shipped TASK-194-verified
  values — geometry pipeline fully deterministic.
- **Only the D changed:** `T_<CardID>_N.png` + `T_<CardID>_ORM.png` are byte-IDENTICAL to shipped (sha-verified)
  on all 5 — the delight step touched exactly the albedo, per the TASK-193 contract. FBX re-exported (hash
  differs by export metadata; same 15000 tris / slots / dims).

### Outputs (overwritten in place, ready for part-B reimport)
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\{Ogre,Knight,Cavalry,Pikeman,MilitiaMob}.fbx`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Textures\<CardID>\T_<CardID>_{D,N,ORM}.png`
  (N/ORM bit-identical to shipped; D is the lifted albedo)

### Backups (sha-recorded, pre-rebake shipped state)
Per asset: `Tools/ArtPipeline/Cache/<CardID>/TASK225_shipped_backup/` — contains the shipped FBX,
`Textures/T_<CardID>_{D,N,ORM}.png`, `shipped_hashes_before.txt` (sha256 of all 4 shipped files),
`refine_report_dark.json` (pre-run canonical report) and `previews_dark/` (pre-run canonical preview renders).
Rollback = copy these back over Content/RawAssets + restore the report/previews.

### Before/after evidence (durable, for the morning report)
`Tools/ArtPipeline/Cache/_TASK225_report/`:
- `AB_<CardID>_dark_vs_delight.png` × 5 (front + threequarter, labeled before|after)
- `AB_ALL5_threequarter_dark_vs_delight.png` (single contact sheet, all 5 stacked)
Eyeball verdicts: Ogre = the TASK-195-proven mossy olive-gray; Knight = recovered gray surcoat/plate separation;
Cavalry = brown horse + tack + red plume resurfaced (rider armor stays dark by donor design); Pikeman = near-black
lower body now reads blue-gray trousers / tan tabard / brown boots; MilitiaMob = readable olive levy garments +
skin tones + yellow trim. Note the TASK-195 caveat stands: workbench texture previews are the honest albedo;
UE's brighter lighting will read stronger.

### For part B (build/art, editor dispatch)
- Same-path reimport via the proven commandlet (`Tools/reimport_meshes.py` + `reimport_apply_materials_mcp.py`
  precedent): `/Game/Meshes/SM_<CardID>` overwrite (NEVER delete+recreate) + `T_<CardID>_D` re-import (sRGB);
  `T_<CardID>_N/_ORM` unchanged on disk — reimporting them is harmless but unnecessary.
- MCP readback after: slots `[TeamRegion, <CardID>PBR]`, Nanite OFF, refs intact.
- Editor-bounce authorized overnight per the board block; editor RUNNING at end.

### Residual/recommendations
- **Cavalry remains the darkest** (p99 0.194 after lift). If it still reads too dark in-engine at part-B/morning
  review, the options are a stronger per-asset override (gamma ~0.45 — OUTSIDE the proven envelope, needs a
  ruling) or the Meshy retexture lane (TASK-200, stays open per Ruling B).
- TASK-170/171 (Wall/DeepMine): when their Stage-1 finally lands, judge darkness at THEIR gate — the pinned
  overrides here do NOT extend to them.

## Part B — editor same-path reimport (art-director, 2026-07-19 01:07–01:21) — DONE

**Status:** part-B DONE — all 5 lifted D textures live in-engine, hash-verified, editor clean. NO Git mutations by me.

### Lane
TASK-221's full-unreal-python remote-execution lane, still live in the same editor session (no re-enable needed).
Runner: scratchpad `ue_exec` client against the engine's bundled remote_execution module (multicast 239.0.0.1:6766,
bind 127.0.0.1). All ops scripted; NO save-all; single editor lane respected.

### What was done
- **Reimported in-place:** `/Game/Textures/T_{Ogre,Knight,Cavalry,Pikeman,MilitiaMob}_D` from
  `Content/RawAssets/Textures/<CardID>/T_<CardID>_D.png` via `unreal.AssetImportTask(replace_existing=True,
  automated=True)` (the proven Tools reimport pattern) — asset identity preserved (refs intact),
  **sRGB=ON + TC_DEFAULT preserved on all 5** (readback showed no post-fix needed).
- **N/ORM skipped** (bit-identical on disk per part A, sha-verified). **FBX skipped** (geometry unchanged; D-only
  per the TASK-193 contract).

### Verification (all PASS)
- **Hash readback:** post-save AssetRegistry `AssetImportData.FileMD5` == on-disk PNG MD5 on all 5:
  Ogre `44b447d399f96cb09da8988d790ed015`, Knight `1b6dc3af2ebea28f7445f38d7b75121e`,
  Cavalry `b8435596a462ba065b2699babe03a550`, Pikeman `bdacf2d4671312011731575021ce5831`,
  MilitiaMob `8bcf2bc65dffd9e7eefe242b24b34911`.
- **Visual:** transient 5-mesh preview row (Z=20000 in L_Arena) + SceneCapture2D→RenderTarget→PNG under an
  identical transient key light, identical camera/FOV/exposure before vs after. AFTER reads clearly brighter on
  all 5 (these MIs are shared by the SK unit versions — units + statics both lift). Cavalry rider still the
  darkest (donor design; part-A residual note stands).
- **No errors:** no material/shader/texture errors in the editor log for the reimport window; MapCheck after the
  level reload: 0 errors / 0 warnings.

### Evidence (durable)
`Tools/ArtPipeline/Cache/_TASK225_report/`:
- `TASK225B_inengine_BEFORE.png` / `TASK225B_inengine_AFTER.png` (1920x1080, identical setup)
- `AB_TASK225B_inengine_dark_vs_delight.png` — labeled composite strip for the morning report

### Editor / dirty state (for TASK-228)
- **Saved: ONLY the 5 texture assets.** The map was NEVER saved — transient evidence actors (5 previews +
  SceneCapture2D + key DirectionalLight) were deleted and L_Arena reloaded from disk; final dirty-state readback:
  **zero dirty maps, zero dirty content packages**. Editor RUNNING, L_Arena loaded clean.
- Two stale HighResShot requests (a 4K + a 1080p, from the abandoned viewport-screenshot attempts) may flush a
  stray PNG into `Saved/Screenshots/WindowsEditor/` whenever the editor next presents a frame — harmless, untracked.
- **Git/SCC disclosure (read-only `git status`, no mutations):** the 5 `Content/Textures/T_<CardID>_D.uasset`
  show ` M` modified-UNSTAGED (unlike TASK-221's auto-staged AM saves). Also pending from parts A / TASK-223:
  5 RawAssets D PNGs + 5 root FBX modified, 9 `Content/RawAssets/Characters/*.fbx` modified,
  `Content/RawAssets/Characters/Meshy/` untracked.

### Lane learnings (recorded for reuse)
- **Background-throttled editor never presents a frame**, so `HighResShot` requests queue forever; the
  EditorPerformanceSettings CDO exposes no python-visible throttle property in 5.8. Reliable capture lane:
  spawn SceneCapture2D → `capture_scene()` → `RenderingLibrary.export_render_target` (synchronous, focus-independent).
- **ExecPythonCommandEx quirk:** a remote-exec command string containing a `.py` token anywhere (even a comment)
  is treated as a file invocation and fails — keep the token out of script text sent over the wire.
