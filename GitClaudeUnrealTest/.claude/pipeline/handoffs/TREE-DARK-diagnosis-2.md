# TREE-DARK — diagnosis 2 (art-director, READ-ONLY)

**Date:** 2026-09-06 · Editor PID 20564, MCP `127.0.0.1:8000` answered · **No PIE/Simulate started** (none was needed) ·
**Zero writes:** no `set_properties`, no node edit, no `save_assets`, no recompile, no import, no cvar set, no level save, no Git.

---

# ✅ VERDICT — **CAUSE FOUND. Missing material usage flag `bUsedWithInstancedStaticMeshes`.**

Both tree materials are missing the Instanced-Static-Mesh usage flag, so **Unreal substitutes the Default
Material (`WorldGridMaterial`) on every HISM instance** — opaque, unmasked, untextured. The asset is fine;
the material never reaches the instanced render path.

**The engine says it itself, verbatim, in this session's log** — and these are the **only two** such warnings
in the entire session (unlimited scan, `pattern: "missing usage flag"`, `maxEntries: 0`):

```
[2026.09.06-16.19.29:298][487]LogMaterial: Warning: Material
  /Game/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Trunk_Mobile.M_Pack1_Trunk_Mobile
  missing usage flag InstancedStaticMeshes! Default Material will be used in game.
[2026.09.06-16.19.29:298][487]LogMaterial: Warning: Material
  /Game/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Leaf_Mobile.M_Pack1_Leaf_Mobile
  missing usage flag InstancedStaticMeshes! Default Material will be used in game.
```

Frame 487 = level load, first ISM proxy build. UE's warn-once bit (`UsageFlagWarnings`) latches after that
first line, so the substitution then continues **silently** for the rest of the session — which is why three
prior investigations found nothing: **there is no second warning, no error, and every property read comes
back green on the asset itself.** This is `GHOST-§5`'s `bUsedWithSkeletalMesh` class of defect, exactly:
correct asset, green returns, wrong pixels.

Candidate 1 confirmed. Candidates 2 and 3 measured and **eliminated** (below).

---

## 1. Property table — the discriminator

Control = `M_Grass`, the material on the grass HISM scatter in this same level, which renders correctly.

| | `M_Pack1_Leaf_Mobile` | `M_Pack1_Trunk_Mobile` | **CONTROL `M_Grass`** (HISM, renders correctly) |
|---|---|---|---|
| **`bUsedWithInstancedStaticMeshes`** | **`false`** ⛔ | **`false`** ⛔ | **`true`** ✅ |
| `bUsedWithStaticLighting` | false | false | true |
| `bUsedWithNanite` | false | false | true |
| `bUsedWithSkeletalMesh` | false | false | false |
| `bUsedWithSplineMeshes` | false | false | — |
| `bAutomaticallySetUsageInEditor` | true | true | true |
| `BlendMode` | BLEND_Masked | **BLEND_Opaque** | BLEND_Masked |
| `OpacityMaskClipValue` | 0.3333 | 0.3333 | 0.3333 |
| `ShadingModel` | MSM_DefaultLit | MSM_DefaultLit | MSM_TwoSidedFoliage |
| `TwoSided` | false | false | true |
| `MaterialDomain` | MD_Surface | MD_Surface | MD_Surface |

One bit differs between the material that works on HISM and the two that do not, and it is the bit that
governs the HISM path. `bAutomaticallySetUsageInEditor` being `true` did **not** rescue it — the flag still
reads `false` in a live editor that has rendered these instances thousands of times.

Vendor Highpoly `M_Pack1_Leaf` also reads `false`, but that proves nothing — it is never placed on an HISM.

## 2. The 1083-vs-1099 OpacityMask conflict — **TASK-1083 was RIGHT. TASK-1099's relay was wrong.**

Read live, quoted from `MaterialTools.get_property_input`:

```
MP_OpacityMask ← MaterialExpressionTextureSample_0, output pin "A"
MP_BaseColor   ← MaterialExpressionTextureSample_0, output pin "RGB"
```

**The opacity mask IS connected**, to the alpha of the base-colour sampler — `TASK-1083` §8's node read.
`TASK-1099` §8 cl. 1 relayed "no `MP_OpacityMask` connection" from `TREE-DARK-diagnosis.md` §① and flagged
that it had not read the graph itself; that relayed list was incomplete. The vendor Highpoly `M_Pack1_Leaf`
wires it identically (`TextureSample_0` → `A`), so this is the pack's own shipped wiring, untouched by D1.

**And the alpha is real**, so a connected-but-stripped mask is out too:

| `T_Leaf_Pack1` | value |
|---|---|
| `HasAlphaChannel` | **True** |
| `Format` | **DXT5** (alpha-preserving) |
| `CompressionNoAlpha` | **false** |
| `CompressionSettings` / `SRGB` | TC_Default / true |
| `SourceFormat` / Dimensions | TSF BGRA8, 2048×2048 |

⇒ **Candidate 2 is dead.** The mask is wired and functional. That is precisely *why* the same material cuts a
clean leaf silhouette on a `StaticMeshActor` and in `CaptureAssetImage` — it is working everywhere it is
actually used.

## 3. Override materials on the HISM components — **eliminated, no Simulate needed**

The log warning **names these two materials by full path at the moment the ISM proxy was built**. The engine
could only reject `M_Pack1_Leaf_Mobile` on the InstancedStaticMeshes path if that material *is* the HISM
section material. ⇒ **The scatter's HISM components carry no override; they resolve to the same two
materials as the 15 hand-placed actors.** Candidate 3 answered from the log, without holding PIE.

## 4. ⭐ The asymmetry, answered explicitly

> *Why does the SAME mesh on the SAME material render correctly as a `StaticMeshActor` and wrongly as an HISM instance?*

**Because the usage-flag gate exists on exactly one of those two code paths.**

| path | gate | material actually rendered | result |
|---|---|---|---|
| `StaticMeshActor` → `FStaticMeshSceneProxy` | **no ISM usage check** | `M_Pack1_Leaf_Mobile` | leafy, masked, light-yellow ✅ |
| Scatter HISM → `FInstancedStaticMeshSceneProxy` | **`CheckMaterialUsage_Concurrent(MATUSAGE_InstancedStaticMeshes)`** → `false` | **`WorldGridMaterial`** (engine default) | opaque, **no opacity mask**, no leaf texture ⛔ |
| `CaptureAssetImage` thumbnail | plain `StaticMeshComponent`, no ISM check | `M_Pack1_Leaf_Mobile` | bright birch ✅ |

The instanced proxy replaces the section material with `UMaterial::GetDefaultMaterial(MD_Surface)` when the
flag is absent. `WorldGridMaterial` is **fully opaque with no opacity mask**, so every leaf *card* renders as
a complete solid quad instead of a cut leaf shape. That predicts, quantitatively, what `TASK-1099` §3 measured:

- **straight polygon edges against the sky** — the raw quad boundary, no mask ✔
- **~70 % dark coverage of the canopy bounding crop** — solid cards fill the volume a cut canopy leaves mostly empty ✔
- **near-black, flat** — mid-grey default material, unlit-side, under the arena's dusk sun ✔
- **invariant under `foliage.ForceLOD 0/1`** — every LOD's sections get the same substitution ✔
- **visibly collapses at `foliage.ForceLOD 4`** — geometry still changes; only the *material* is frozen wrong ✔

Every prior null result is explained, including why LOD forcing moved nothing.

---

## 5. THE FIX

Set **`bUsedWithInstancedStaticMeshes = true`** on both materials and **save** them:

- `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Leaf_Mobile`
- `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Trunk_Mobile`

Two property writes fix **all ~340 scatter instances across `SM-Mobile_Tree_1..12`** — they all share these
two materials. No mesh edit, no LOD work, no reimport, no code. `TASK-1100` stays VOID.

### ⚠️ Authorization: **this is a VENDOR-PACK EDIT and needs Jonathan's word.**

`/Game/Tree_Pack_1/` is a read-only vendor pack. `FIELD-§3`'s standing exception covers **only D1's two
normal samplers** and licenses nothing else — it does not cover a usage-flag write. **Not ours to make freely.**

Two further things the manager must board with it, both load-bearing:

1. **⛔ `TASK-239` risk — saving these materials forces a shader recompile**, and recompiling a material on
   this stack has wedged the editor for 100+ min twice. Schedule it deliberately, editor otherwise idle. It
   is only 2 small mobile materials (4 nodes each), but the risk is real and must not be stumbled into.
2. **The flag must be SAVED to the `.uasset` or it reverts every session.** `bAutomaticallySetUsageInEditor`
   is already `true` and did not rescue it (warn-once latched at frame 487), so there is no
   fix-itself-at-runtime path. A save is mandatory.

**Vendor-free alternative, if Jonathan declines the pack edit:** duplicate both materials into
`/Game/Materials/` with the flag set, and assign them as override materials on the scatter's HISM components
— but that is a scatter-config change (programmer/build-master), still needs the same shader compile, and
costs two duplicate assets. **The vendor edit is strictly cleaner; ask for it first.**

---

## 6. Fences — verified

- ✅ **Zero writes of any kind.** Reads only: `get_properties`, `get_property_input`, `get_asset_tags`,
  `find_assets`, `load_asset`, `GetLogEntries`. No `set_properties`, no graph edit, no `save_assets`,
  **no material recompile** (`TASK-239` respected), no import, no commit, no push.
- ✅ **No PIE/Simulate started** — `IsPIERunning` untouched; nobody else's session disturbed. The whole
  diagnosis came from asset reads plus the existing session log.
- ✅ **No console cvars set**, so nothing to restore. `L_Arena` never saved, never dirtied by me.
- ✅ Editor left up, PID 20564, MCP answering — **ready to be closed for a `TASK-1104` compile at any time.**
- ✅ TASKBOARD.md **not touched** (unboarded diagnostic; the manager boards the fix).
