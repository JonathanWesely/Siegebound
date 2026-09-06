# TASK-1082 — [TREE-SURVEY] — art-director handoff

**Status:** ready-for-integration · **Date:** 2026-09-06 · **Marker:** `TASK-1082-TREE-SURVEY`
**Assets written: NONE.** Outputs are one evidence PNG + this note. No roster edit, no DataAsset write,
no commit, nothing staged.

---

## THE HEADLINE, IN ONE LINE

🧑 His ask — *"a large variety of light trees from `Megaplant_Library` and `Tree_Pack_1`"* — **cannot be
met as worded.** `Megaplant_Library` contributes **ZERO** rosterable trees. Every rosterable tree in both
folders comes from `Tree_Pack_1`, and there are **12 distinct designs** (shipped as 24 assets = 2 LOD
tiers of the same 12). They all share **one** foliage texture, so they are all **the same tone** —
and that tone is the **light yellow** one. **The good news he will care about: the light yellow trees he
already likes and the only trees we can roster are almost certainly the same trees.**

---

## ① THE TONE INSTRUMENT (and why the obvious one was wrong)

**Instrument — CTI (Canopy Tone Index).** Mean Rec.709 relative luminance of the sRGB-linearised
**foreground** pixels of an asset's thumbnail, ×100. Higher = lighter.

- **Source images:** UE 5.8's own asset thumbnail renderer (`CaptureAssetImage`), invoked **live, in one
  editor session** → one lighting rig, applied identically to every asset by the engine, not by me.
  256×256 RGB.
- **Foreground mask:** the rig's backdrop is a smooth **achromatic** dark checkerboard. I recovered it
  per-pixel as the median over *achromatic samples only* across all 51 captures, then masked
  `|image − plate| > 8`.
- **Reproducible without re-rendering:** scripts in scratch (`ua3.py`, `tone.py`, `sheet.py`); base64
  captures live in `Saved/TreeSurvey/` (gitignored).

**⚠ REJECTED INSTRUMENT — worth recording.** My first approach read the thumbnails **cached inside the
`.uasset` files**. Those are free and need no editor — but their backgrounds measured **179, 77 and 19
grey on three different assets**. They were baked by the *pack authors* under three different rigs, at
three different times. Ranking tone from them would have looked exactly like a real measurement and been
meaningless. Re-rendering live was the whole difference.

### Controls run (`SC-§88`)

| Control | Result | What it proves |
|---|---|---|
| **Repeatability** — same asset captured twice | max pixel Δ = **0.0**, CTI Δ = **0.0000** | renderer is deterministic; one capture ≠ one sample problem here |
| **Background plate** — is it really achromatic? | max channel spread **3.0** / 255; range 16–38 | plate model is valid |
| **Plate fit** — plate vs every image's border ring | mean abs error: min 0.10, **median 0.37**, max 5.35 | the plate is the actual backdrop |
| **Zero/adversarial subject** — default-material **grey** sphere | detected at **0.697 coverage**, CTI 18.93 | the mask does **not** discard neutral subjects — a chroma-only mask would have, and would have silently biased every dark tree |
| **Burst discipline** | 2 of 51 captures came back wrong on first try (`Tree_Japanese_Cypress_01_B` = 2 KB blank; `Tree_Norway_Spruce_01_A` = load timeout); **both correct on retry** | a single capture does lie — caught and re-taken |

### Cross-check on the census (two independent instruments)

I parsed all 236 `.uasset` headers **on disk** (export table → import table = UClass) *and* asked the
editor's asset registry. **They agree exactly**: `Tree_Pack_1` = 24 StaticMesh / 0 SkeletalMesh /
6 Texture2D; `Megaplant_Library` = 58 StaticMesh / 58 SkeletalMesh / 15 Texture2D.

---

## ② THE ANSWER HE WILL NOT LIKE — VERIFIED, NOT REPEATED

The manager's flag **holds, and is sharper than stated.**

`Megaplant_Library` is **199 assets, two species** — and its **11 whole trees are all `SkeletalMesh`**:

| Whole tree | Class | Bones | Rosterable? |
|---|---|---|---|
| `Tree_Norway_Spruce_01_A…D` (4) | **SkeletalMesh** | 509–1171 | ❌ |
| `Tree_Japanese_Cypress_01_A…G` (7) | **SkeletalMesh** | 44–959 | ❌ |

`FScatterLayer::Meshes` is `TArray<TSoftObjectPtr<UStaticMesh>>` (`ScatterConfig.h:94`) — **it cannot hold
them.** They are PVE (Procedural Vegetation Engine) wind rigs; the bone counts are why they are skeletal.

Its **58 static meshes contain no whole trees at all** — every one is a part:

| Static mesh family | Count |
|---|---|
| `Branch_Norway_Spruce_*` (+ `_Dead`, `_Hanging`, `_Top`) | 27 |
| `Decoration_Norway_Spruce_*` (cones) | 10 |
| `Twig_Norway_Spruce_*` (+ `_Dead`) | 10 |
| `Tree_Japanese_Cypress_Branch_01…06` | 6 |
| `Japanese_Cypress_Decorations_01_A…E` | 5 |
| **Total** | **58 — 0 whole trees** |

**And they are the dark ones, by measurement.** Foliage **base-colour albedo**, measured full-frame:

| Foliage albedo texture | CTI | mean RGB | vs Tree_Pack_1 |
|---|---:|---|---|
| **`T_Leaf_Pack1`** (Tree_Pack_1) | **46.06** | **199, 176, 97** — light yellow | — |
| `T_Japanese_Cypress_Foliage_01_CA` | 11.10 | 104, 90, 60 | **4.1× darker** |
| `T_Norway_Spruce_Foliage_01_CA` | 8.68 | 87, 81, 47 | **5.3× darker** |

> **Honesty note on the Megaplant whole-tree CTI.** I also rendered all 11. They come back at CTI
> 10.5–20.0 — but at **1.3 %–3.7 % coverage**, because in their base pose they render as a **bare stem**
> (the PVE grower supplies the foliage). So *those* numbers measure a trunk, not a canopy, and I am **not**
> resting the "dark" claim on them. The albedo-texture row above is the trustworthy evidence.

**⇒ Verdict: "a large variety of light trees from those two folders" is not achievable.** The two folders
can deliver **light** (Tree_Pack_1) or **conifer variety** (Megaplant, as parts only) — never both, and
Megaplant can never deliver a rosterable tree at all.

---

## ③ THE NUMBERED CONTACT SHEET

**`.claude/pipeline/playtest-evidence/2026-09-06/RosterSheet_Trees.png`** (1490×1696)

**Index convention:** the printed number **is the 0-based index into
`DA_BattlefieldScatter → Layers[Trees].Meshes`**. 🧑 *"Drop 7 and 11"* ⇒ two array removals, zero
re-derivation. Order is **recommended roster first (0–11)**, alternates after (12–23), deliberately, so
that if `TASK-1083` adopts the recommendation **indices 0–11 stay valid unchanged**.

⚠ **PROVISIONAL until `TASK-1083` writes the array** — stated on the sheet itself. `TASK-1083` re-issues
it against the final array.

**Reproducing the sheet:** capture each mesh via `CaptureAssetImage`, write base64 through
`AssetTools.write_file` into `Saved/TreeSurvey/` (keeps images out of agent context), decode, mask against
the achromatic plate, tile 6-across in the array order. Scripts are in the session scratchpad.

---

## THE INDEX → PATH → TONE → MB TABLE (greppable)

All 24 live under `/Game/Tree_Pack_1/Meches/`. `Highpoly_Tree_1/` for 0–11, `Mobile_Tree_1/` for 12–23.

| idx | asset | CTI | tris | verts | LODs | mesh MB |
|---:|---|---:|---:|---:|---:|---:|
| 0 | `SM_Highpoly_Tree_1` | 29.04 | 7665 | 9102 | 1 | 0.183 |
| 1 | `SM_Highpoly_Tree_2` | 29.15 | 7325 | 9070 | 1 | 0.182 |
| 2 | `SM_Highpoly_Tree_3` | 29.55 | 7613 | 9273 | 1 | 0.187 |
| 3 | `SM_Highpoly_Tree_4` | 29.48 | 7685 | 9147 | 1 | 0.185 |
| 4 | `SM_Highpoly_Tree_5` | 30.28 | 6803 | 8558 | 1 | 0.180 |
| 5 | `SM_Highpoly_Tree_6` | 29.11 | 7619 | 9176 | 1 | 0.187 |
| 6 | `SM_Highpoly_Tree_7` | 30.26 | 6970 | 8627 | 1 | 0.184 |
| 7 | `SM_Highpoly_Tree_8` | 30.41 | 7090 | 8766 | 1 | 0.185 |
| 8 | `SM_Highpoly_Tree_9` | **30.81** | 7519 | 9472 | 1 | 0.187 |
| 9 | `SM_Highpoly_Tree_10` | 29.10 | 8522 | 12367 | 1 | 0.243 |
| 10 | `SM_Highpoly_Tree_11` | 28.77 | 8127 | 10593 | 1 | 0.220 |
| 11 | `SM_Highpoly_Tree_12` | 28.12 | 6757 | 9697 | 1 | 0.186 |
| 12 | `SM-Mobile_Tree_1` | 24.23 | 2458 | 3302 | 5 | 0.134 |
| 13 | `SM-Mobile_Tree_2` | **30.80** | 2025 | 3019 | 5 | 0.107 |
| 14 | `SM-Mobile_Tree_3` | 28.36 | 2115 | 3117 | 5 | 0.118 |
| 15 | `SM-Mobile_Tree_4` | 27.94 | 2137 | 3085 | 5 | 0.113 |
| 16 | `SM-Mobile_Tree_5` | 27.57 | 2283 | 3269 | 5 | 0.119 |
| 17 | `SM-Mobile_Tree_6` | 27.51 | 2422 | 3433 | 5 | 0.133 |
| 18 | `SM-Mobile_Tree_7` | 29.85 | 2249 | 3209 | 5 | 0.114 |
| 19 | `SM-Mobile_Tree_8` | 28.92 | 2213 | 3196 | 5 | 0.115 |
| 20 | `SM-Mobile_Tree_9` | 28.83 | 2360 | 3542 | 5 | 0.131 |
| 21 | `SM-Mobile_Tree_10` | 27.30 | 2202 | 3964 | 5 | 0.129 |
| 22 | `SM-Mobile_Tree_11` | 26.91 | 2291 | 3542 | 5 | 0.135 |
| 23 | `SM-Mobile_Tree_12` | 28.19 | 2393 | 3923 | 5 | 0.138 |

**⚠ Ranking "lightest first" is nearly meaningless inside this pool, and saying so is the honest result.**
The whole Highpoly family spans CTI **28.12–30.81** — a **9 % spread**, because all 24 meshes share the
*same* `T_Leaf_Pack1` albedo. What varies between these trees is **silhouette and canopy density, not
tone.** There is no "pick the light ones" move available here; they are all the light ones.

---

## ④ THE MB — WHY VARIETY HERE IS ALMOST FREE, AND MEGAPLANT IS NOT

**Formula (stated so it can be re-derived):** `W × H × bpp/8 × mipfactor`, with effective size capped by
`MaxTextureSize` where set; `bpp` = DXT1/BC1 4, DXT5/BC3 8, BC5 8, B8G8R8A8 32; `mipfactor` = 4/3 for
power-of-two (full mip chain), 1 for NPOT. Mesh MB = the engine's own `EstTotalCompressedSize`.

### ⭐ The highest-value finding in this task: **all 24 candidates share ONE texture set.**

| Texture set | Textures | MB | Shared by |
|---|---|---:|---|
| **Tree_Pack_1 (Highpoly)** | `T_Leaf_Pack1` 5.33 + `T_Leaf_Pack1_normal` 5.33 + `T_Trunk_Pack1` 7.63 + `T_Trunk_Pack1_normal` 7.63 | **25.92** | **all 12** |
| **Tree_Pack_1 (Mobile)** | `T_Leaf_Pack1` 5.33 + `T_Trunk_Pack1` 7.63 (normals **missing**, see defect) | **12.96** | all 12 |
| Megaplant **Norway Spruce** | 6 textures (bark 4K, foliage capped 1K, decorations 2K) | **41.33** | any spruce part |
| Megaplant **Japanese Cypress** | 6 textures, all 4096² | **96.00** | any cypress part |

**⇒ The bounded choice, in one sentence:** going from 1 tree to **all 12** costs **+2.31 MB of meshes and
0 MB of extra textures** — total **28.23 MB**. Adding a *single* Megaplant spruce part costs **+41.33 MB**;
a single cypress part **+96.00 MB**. Against a budget his own screenshot reports as **828.144 MB over**,
that is the whole argument.

| Roster option | Meshes | Textures | **Total** |
|---|---:|---:|---:|
| 12 Highpoly (indices 0–11) | 2.31 | 25.92 | **28.23 MB** |
| 12 Mobile (indices 12–23) | 1.49 | 12.96 | **14.45 MB** |
| all 24 (both families) | 3.80 | 25.92 | **29.72 MB** |

**Caveats, stated:** Tree_Pack_1 textures are **not** virtual-textured, so 25.92 MB is a real resident
figure at full mip. **Every Megaplant texture is `VirtualTextureStreaming = True`**, so its residency is
bounded by the VT physical pool rather than by full size — **41.33 / 96.00 MB are upper bounds**, not
measured residency. `TASK-1085` re-measures for real.

---

## RECOMMENDATION (recommend, not decide — `TASK-1083` selects)

**Roster the 12 `SM_Highpoly_Tree_1…12` (indices 0–11).** Rationale, with the trade-off exposed rather
than buried:

- **Do not roster both families.** `SM-Mobile_Tree_N` is the *same design* as `SM_Highpoly_Tree_N`,
  decimated — measured, not assumed: silhouette IoU on the diagonal is the row maximum for **all 12**
  pairs (diagonal mean **0.81**, off-diagonal mean 0.64). Rostering both puts visually duplicate trees in
  the array at double the mesh cost and buys no variety.
- **Highpoly is recommended because its materials are complete.** ⚠ **Defect found in the Mobile family:**
  `M_Pack1_Leaf_Mobile` and `M_Pack1_Trunk_Mobile` reference **`/Game/Tree_Pack_16/Textures/dgd` and
  `/fg` — which do not exist** (confirmed by the editor: *"Asset does not exist"*, and there is no
  `Tree_Pack_16` folder under `Content/`). They render (UE substitutes a default) but ship two dangling
  references and no normal maps.
- **The counter-argument, honestly:** Mobile is better on **both** axes under pressure — **2.0–2.5 k tris
  vs 6.8–8.5 k**, **5 LODs vs 1**, and **12.96 MB vs 25.92 MB**. The Highpoly family has **`LODs = 1`**,
  i.e. *no LODs at all*, which on a scattered field is a real draw cost. The Mobile defect is also
  **repairable** — the correct normals (`T_Leaf_Pack1_normal`, `T_Trunk_Pack1_normal`) exist in the same
  pack; the materials just point at the wrong path. **That repair is a vendor-pack edit, which my fences
  forbid, so I am flagging it rather than doing it.**
- ⇒ **If tri/VRAM pressure dominates, or if someone repoints those two material refs, switch to indices
  12–23.** That is `TASK-1083`'s call against `TASK-1081`'s measured budget, and 🧑 his if it is a
  look-vs-framerate question (`FIELD-§3`).

**No "fits in N MB" cut line is needed:** the full 12 fit in 28.23 MB and there is no cheaper subset worth
having, because the texture set — the dominant cost — is paid in full the moment the *first* tree is
rostered.

## For `TASK-1083` specifically

- `ZOffset`: **re-derive.** These are a new mesh set. `ApproxSize` Z ranges **1382–2094 uu**; pivots
  looked base-centred in every thumbnail, but that is an eyeball, not a measurement — verify against
  bounds `MinZ` before trusting `0`.
- `FootprintRadius`: prefer **0** (auto-derive). Canopy XY spans **795–1684 uu** across the 12 — no single
  override is right for all of them.
- Keep the `Cylinder` `CollisionProxyMesh` + scale + Z offset (the `TASK-137` nav-blob defect).
- `LODGroup` is already `Foliage` on all 24.

---

## Notes for whoever reads next

- **`TASK-1081` should confirm one thing I could not:** these trees measure **light yellow**
  (`T_Leaf_Pack1` albedo RGB **199, 176, 97**) and are visibly gold in the sheet. That is a strong
  hypothesis that 🧑 his praised *"light yellow ones in the background"* **already are** `Tree_Pack_1` —
  in which case ask 1 is partly "more of what is already there", and `TASK-1083` cl. 1 (**do not empty
  the array**) is live. I did not read the DataAsset, by design.
- **A Fab request is the honest route to real tree variety.** Neither folder can supply a second *light*
  species. If 🧑 he wants genuine variety rather than 12 variations of one yellow tree, that needs a new
  pack — I can author a `FAB-###` request on his word.

## Fences — all held

✅ Survey only; **no roster edit, no DataAsset write, no asset created or modified.**
✅ `Content/Tree_Pack_1`, `Content/Megaplant_Library`, `Content/Data/DA_BattlefieldScatter.uasset`,
`Content/Maps/L_Arena.umap` — all verified **clean** in `git status` after the run.
✅ Nothing staged, no commit, no push, no `checkout`/`restore`/`stash`/`reset`/`clean`.
✅ Editor writes went only to `Saved/TreeSurvey/` (gitignored, `.gitignore:111`).

⚠ **One unrelated dirty path to report, not mine:** `M Content/UI/WBP_CardHand.uasset`. That is the card
hotkey widget — `TASK-1079`'s subject, running in parallel. **Not staged, not reverted, not touched.**

**Editor:** it was already running (PID 17008) with MCP up on `127.0.0.1:8000`. I used it for
**read-only** queries and thumbnail renders only, in short batches, to stay out of `TASK-1079`/`TASK-1081`'s
way. No level loaded, no asset saved, no PIE.

---

## 🚨 INCIDENT — I TRUNCATED `TASKBOARD.md` TO 0 BYTES, AND RECOVERED IT. READ THIS.

**What happened.** Updating my own status row, I edited the board with a Python one-liner instead of the
Edit tool. The string contained `'🚨'` — a **lone surrogate pair**, which Python accepts in a
literal but **cannot encode to UTF-8**. `open(path,'w')` had already **truncated the file** before the
encode raised. Result: **`TASKBOARD.md` = 0 bytes.**

**Why it was nearly unrecoverable.** The board was `M` (modified, **unstaged**) and **HEAD did not contain
the new lane at all** — `git show HEAD:...TASKBOARD.md` has 31944 lines and **zero** occurrences of
`TASK-1082`. The entire `TASK-1079…1094` lane the manager boarded this morning existed **only** in that
uncommitted working copy. A reflexive `git checkout --` would have "restored" the file and **silently
destroyed the whole lane** — which is exactly why the fence forbids it. I did not run it.

**How it was recovered — reconstruct, don't restore.**
1. Established the true baseline: the board's last commit is `09b89cd` (`2026-09-06T09:17:21Z`) and the
   **index matches HEAD**, so the working copy = **HEAD blob + every `Edit` applied after that timestamp**.
2. Scanned the 615 session transcripts under `~/.claude/projects/…/` for `Edit` calls whose `file_path`
   is `TASKBOARD.md` and whose timestamp is after the cutoff. **Exactly 4**, all successful: the manager's
   single **65,946-char** lane insertion at 10:02:15Z, plus three status updates (`TASK-1087` 10:18Z,
   `TASK-1079` 10:22Z, `TASK-1089` 10:30Z).
3. Replayed those 4 onto the HEAD blob in timestamp order, requiring each `old_string` to match **exactly
   once**. All 4 applied, none skipped.
4. **Verified before writing** — this is the part that makes it a recovery rather than a hope:
   - all **16** `#### TASK-1079…1094` headings present;
   - **14/14 spot-checks** against text I had read **verbatim** from the live file at task start —
     including the original status line, `ScatterConfig.h:94`, the Slack thread id, and the sheet path —
     matched exactly;
   - `git diff --numstat` = **+349 / −0** vs HEAD (pure addition, nothing lost);
   - line endings **LF**, identical to the HEAD blob, so other agents' exact-match `Edit` calls still work.
5. Only then wrote the file, and re-applied my own status row **with the Edit tool**. Final state:
   **+354 / −0**, 16/16 lane rows, nothing staged.

**The board is whole.** But an earlier trap deserves flagging: a first replay pass that did **not** filter
by commit timestamp replayed edits from *all* sessions and produced a 38,837-line file that **looked
plausible** — ~6,900 lines of duplicated rows. Line count alone would not have caught it; the `−0`
deletion count and the verbatim spot-checks did.

**Two lessons worth keeping.**
- **`open(...,'w')` truncates before it encodes.** Any encoding error in the payload destroys the file and
  writes nothing. Never hand-roll a write to a shared pipeline file — use the Edit tool, which round-trips
  UTF-8 and fails *before* touching the file. (Related: this box's Python defaults stdout and
  `open()` to **cp1252**, not UTF-8 — it silently mangled two other reads during this task.)
- **The fence against `checkout --`/`restore`/`reset` earned its keep today.** The one moment it was
  tempting was the one moment it would have caused the real, permanent loss.
