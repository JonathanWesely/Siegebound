# TASK-404 — [FC-8] `SM_Wizard`: `LargeProp` STATIC-mesh LOD chain — COMPLETE

**Agent:** art-director · **Date:** 2026-08-02 · **Status:** `ready-for-integration`
**Asset touched:** `/Game/Meshes/SM_Wizard` — **exactly one**, saved by explicit path.
**Law:** CONVENTIONS "Arena 10× scale-up & LOD/perf (M7.6)" → Classic LOD law, + the
`SM_Wizard` outlier clause (§9 FLEET-INCONSISTENCY LEDGER, **RULED 2026-08-02**).

> ⚠️ **THIS IS THE STATICMESH LANE.** It shares nothing with TASK-392's
> `Tools/regen_sk_lods.py` (SkeletalMesh, 50% @ 0.4 / 20% @ 0.15). `SK_Wizard` was **not
> touched** and was verified clean (`dirty: false`) at save time. Do not conflate the two in
> the commit or the verdict.

---

## 1. BEFORE → AFTER (`/Game/Meshes/SM_Wizard`)

| Property | BEFORE | AFTER |
|---|---|---|
| `LODGroup` | **`None`** | **`LargeProp`** |
| `lod_count` | **1** | **4** |
| **per-LOD triangles** | **[15000]** | **[15000, 7500, 3750, 1874]** |
| per-LOD vertices | [15688] | [15688, 9915, 5406, 2824] |
| screen sizes | [2] | [2, 0.2989, 0.1620, 0.0912] |
| reduction ratio vs LOD0 | — | **1.0 / 0.5 / 0.25 / 0.1249** |
| Nanite | false | false (unchanged) |
| material slots | `[TeamRegion, WizardPBR]` | `[TeamRegion, WizardPBR]` (unchanged) |
| bounds | — | min (−64.95, −49.71, 0.0015) / max (65.19, 50.16, 183.25) |

**LOD0 is bit-for-bit untouched** — 15000 tris / 15688 verts before *and* after. Only LODs
1–3 were added. Screen sizes are **auto-computed by the group from bounds** (I set no
threshold by hand); that is why they differ from each sibling's.

### The silent-no-op trap was explicitly tested for, and did NOT occur
The documented hazard is `lod_count` reporting 4 while the reduction did nothing
(`15000/15000/15000/15000`). I read **`get_triangle_count` per LOD index**, not just
`get_lod_count`, and computed the ratios: **1.0 / 0.5 / 0.25 / 0.1249**, strictly
decreasing, LOD3 at 12.49% of LOD0. **The reduction is real.**

---

## 2. Sibling comparison — "it matches the fleet" is a MEASUREMENT, not a claim

Live readback of every shipped static sibling, taken **before** I changed anything:

| Mesh | `LODGroup` | `lod_count` | per-LOD triangles | Nanite |
|---|---|---|---|---|
| **`SM_Wizard` (after)** | **`LargeProp`** | **4** | **15000 / 7500 / 3750 / 1874** | false |
| `SM_Cleric` | `LargeProp` | 4 | 14998 / 7498 / 3750 / 1874 | false |
| `SM_Longbowman` | `LargeProp` | 4 | 15000 / 7500 / 3750 / 1874 | false |
| `SM_Ogre` | `LargeProp` | 4 | 15000 / 7500 / 3750 / 1874 | false |
| `SM_MilitiaMob` | `LargeProp` | 4 | 15000 / 7500 / 3750 / 1874 | false |
| `SM_Sorcerer` | `LargeProp` | 4 | 15000 / 7500 / 3750 / 1874 | false |
| `SM_Footman` | `LargeProp` | 4 | 15000 / 7500 / 3750 / 1874 | false |

**`SM_Wizard` now sits exactly on the fleet ladder.** `SM_Cleric`'s 14998/7498 is its own
LOD0 being 2 tris short — not a discrepancy in the Wizard.

**Fleet vs. spec: NO disagreement this time.** All six siblings independently confirm the
spec's `LargeProp` instruction. I applied the group setting — **not** the castle explicit
50%/25% chain, **not** the SK 50%/20% chain, and **not** a hand-authored
`generate_lods` + `set_lod_thresholds` (that would have authored an explicit chain and
desynced the Wizard from the fleet in the opposite direction).

---

## 3. Visual confirmation — the mesh still renders correctly

`CaptureAssetImage` on `/Game/Meshes/SM_Wizard` **plus `/Game/Meshes/SM_Cleric` as a
control.** Both render at equivalent quality: full texturing, correct albedo, no black or
untextured surfaces, no missing//collapsed geometry, no silhouette damage. The Wizard shows
its robe, blue sash, staff with red orb and the flame in the off hand — all intact.

**Re the recent `M_AssetPBR` / `M_TeamColor` `bUsedWithSkeletalMesh` fix:** that is why the
sibling control was captured. The Wizard's appearance is **consistent with the Cleric**, so
there is no visible material anomaly to attribute to either the flag change or this LOD
work. Both materials read `dirty: false` — I did not touch them.

Captures (session scratchpad, not checked in):
`…\scratchpad\SM_Wizard_after.png` · `…\scratchpad\SM_Cleric_control.png`

---

## 4. ⚠️ REQUIRED ANSWER — would a same-path reimport re-apply `LargeProp` for free?

**YES it would — and the gap was never in the code. This is a ONE-OFF CATCH-UP, not a
recurring gap. No code change is warranted, and none was made.**

Traced in source:
- `Tools/reimport_meshes.py:474` — `_reimport_one()` calls `_apply_lods()` for every card.
- `_apply_lods()` (`:352-398`) sets `lod_group = DEFAULT_LOD_GROUP` (`"LargeProp"`, `:114`)
  for **everything** not in `CASTLE_CLASS_CARD_IDS` (`:115-117` = `Castle`,
  `Castle_Crumble01..03`). **The branch is CardID-agnostic** — `Wizard` would take the
  default automatically.

**So why was the Wizard missed?** Because it was never *enrolled* in a reimport wave:
- not in `DEFAULT_CARD_IDS` (`:91-95`), **and**
- **not in the `Tools/reimport_cards.txt` sidecar** (19 cards — the TASK-201/202 fleet
  retexture wave, which is precisely the wave that back-filled LODs for free across the
  whole fleet, per the SEQUENCING LAW).

`SM_Wizard` predates the LOD line (TASK-220) and no reimport has run over it since. The
durable lane is already correct: **if `Wizard` is ever added to a reimport wave, it picks up
`LargeProp` with no code edit.** I deliberately did **not** edit `reimport_meshes.py` or
`reimport_cards.txt` — adding `Wizard` to the sidecar would enroll it in a full
texture+collision+material reimport, which is far outside this task's scope.

---

## 5. Discipline / safety record

- **PIE:** stopped throughout. **Editor left OPEN** (Jonathan is awake).
- **`L_Arena` NEVER saved.** It *is* dirty in memory (verified `true` before **and** after
  my save — I did not clear it). On-disk proof it never reached disk:

  | File | Before | After |
  |---|---|---|
  | `Content\Maps\L_Arena.umap` | **7/29/2026 3:53:38 AM**, 535,522 B | **7/29/2026 3:53:38 AM**, 535,522 B — ✅ **UNCHANGED** |
  | `Content\Meshes\SM_Wizard.uasset` | 7/26/2026 1:06:17 PM, 803,448 B | 8/2/2026 1:24:12 PM, **810,449 B** (+7,001 B = the 3 new LODs) |

- **Saved via `save_assets(["/Game/Meshes/SM_Wizard"])` — explicit path.**
  `save_assets([])` (save-all) was **never** called; with `L_Arena` dirty it would have
  written the map.
- Dirty-state audit at save time: `SM_Wizard` true (mine); `SM_Cleric`, `SM_Sorcerer`,
  `M_AssetPBR`, `M_TeamColor`, `SK_Wizard` all **false** — nothing foreign was swept in.
  Post-save `SM_Wizard` is `dirty: false`.
- **No Git commands run.** No C++, no gameplay code, no Blueprint, no `SK_Wizard`,
  no other asset.

---

## 6. For build-master (TASK-403 **commit C**)

- **Commit exactly one path:** `Content/Meshes/SM_Wizard.uasset`
- **Must NOT share a commit** with the FOLLOW feature (commit A) or the input assets
  (commit B) — the board is explicit that this is a wholly unrelated deliverable.
- ⚠️ The editor's Git provider auto-stages saved assets. **Stage by explicit pathspec and
  re-check `git status --porcelain`** (the TASK-378 hazard). **`Content/Maps/L_Arena.umap`
  must NOT appear** — if it does, something else saved it after me; my on-disk mtime check
  above proves it was still Jul 29 when I finished. **Stop and report rather than commit
  it.**
- Integration notes: no pivot, scale, bounds, material-slot or collision change. LOD0 is
  identical, so nothing referencing `SM_Wizard` (placement ghost / card preview /
  `BP_Unit_Wizard`'s `VisualMesh`) changes appearance. Risk surface per the ruling is the
  **ghost and card preview only** — not combat, since the in-play actor is `SK_Wizard`.
