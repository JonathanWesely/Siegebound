# TASK-1158 — [FOGLOOK-SHIP] — build-master handoff

⚠️ **This document is born OUTSIDE its own commit.** `20c1bea` is already sealed and this file did
not exist when it was written. It is a genuine orphan the moment it is saved, and the **next commit
host takes it under `TL-§5e` cl. 7a** — exactly as I took `handoffs/TASK-1149-buildmaster.md` here.

Date 2026-09-08 · commit **`20c1bea`** · `main` **6 ahead of `origin/main`, 0 behind, NOT PUSHED**.
Editor **PID 13180** up throughout, MCP `http://127.0.0.1:8000/mcp` answering.

---

## 0. What shipped, and what did not

| | |
|---|---|
| ✅ **shipped** | ⭐ `TASK-1152` — ask **(B)**, fog uniformity. Three values on `BP_SiegeFog`. |
| ✅ **shipped, inert** | `MI_SiegeFog_Grey.uasset` — built, measured, **ZERO CALLERS**. Disclosed in the message (`SC-§36.1`). |
| ⛔ **NOT shipped** | ask **(C)**, the colour. ⭐ `TASK-1154` is `blocked-on-tooling`. **The fog 🧑 he sees today is still the same beige.** |

Ask (B) shipped **alone** on the manager's explicit ruling that `blocked-on-tooling` is a third
state my `blocked-by` clause never covered. I did not wait for colour, and I did not touch
`TASK-1154`, `1165`, `1166`, `1167` or `1168`.

## 1. ⛔ NO COMPILE AND NO SUITE WERE OWED — and that is SHOWN, not asserted

`§ spec cl. (1)` requires this to be explicit rather than silent. This lane is **art-only**: the
only changed engine artefacts are two `.uasset` files. The proof is the commit's **own file list**
in §5 below — an audit run against `20c1bea` itself, not against my intentions:

```
Source/            -> 0 files
Config/            -> 0 files
Tools/             -> 0 files
```

⇒ nothing UBT compiles and nothing the automation runner collects is in this commit. A compile
would have had nothing to build and a suite run would have been a measurement of an unrelated
tree, published as if it belonged to this row. Neither was run, and neither is claimed.

⚠️ **Consequence, stated so nobody mistakes it for a gap:** the suite figure standing for this
branch is still ⭐ `TASK-1149`'s **554 / 0**, unchanged by `20c1bea` because `20c1bea` cannot
change it. ⭐ `TASK-1167` is the row that owes an **executed** suite.

---

## 2. 🚨 THE INTEGRATION CHECK — READ FROM THE ASSET, NOT FROM THE HANDOFF

This row is the art lane's **only** gate; there was no compile to hide behind.

### 2.1 First, the thing that makes the read meaningful

An MCP property read of a **live editor** proves nothing about a **file** if the editor is holding
unsaved state. So I established the provenance before I trusted the value:

| | |
|---|---|
| editor **PID 13180** started | **2026-09-08 19:10:46** |
| `BP_SiegeFog.uasset` last written | **2026-09-08 18:32:54** |
| `MI_SiegeFog_Grey.uasset` last written | **2026-09-08 18:32:40** |

⇒ the editor was launched **38 minutes after** the last save of either asset, so the package it
holds was **loaded from the bytes now in `20c1bea`**. The CDO read below is a read of the shipped
file. (The artist's own session was PID **14120**, a different process, already gone.)

### 2.2 The read — `ObjectTools.get_properties` on `/Game/Blueprints/BP_SiegeFog.Default__BP_SiegeFog_C`

```
"general Data": { "density": 0.5, "wind Speed": 0.5, "wInd World Space": true,
                  "mask Margin": 3, "base Color": (1,1,1,1),
                  "emissive Color": (0.05,0.05,0.05,1) }
"noise Data":   { "sharpness": 0.1, "scale": 2.5, "channel": "R" }
"boxMaterials": { "Base": "/Game/FogArea/Materials/Base/MI_FogArea_Box" , ... }
"material":     "None"
"mode": "Base"   "material Mode": "Dynamic"
```

| claim | asset says | verdict |
|---|---|---|
| `general Data.density` 5 → **0.5** | **0.5** | ✅ |
| `noise Data.sharpness` 0.35 → **0.10** | **0.1** | ✅ |
| `general Data.wind Speed` 1 → **0.5** | **0.5** | ✅ |
| untouched: `noise Data.scale` 2.5 · `mask Margin` 3 · `emissive Color` 0.05 grey · `base Color` white | all as declared | ✅ |
| ⛔ ask (C) **not wired** | `boxMaterials["Base"]` is still the **vendor** `MI_FogArea_Box` | ✅ **zero-callers confirmed on the asset** |
| the artist's experimental `material` override was reverted | `material` = **`None`** | ✅ |

⭐ **The zero-callers disclosure in the commit message is therefore a MEASUREMENT, not a repetition
of the artist's sentence.** `MI_SiegeFog_Grey` ships referenced by nothing, and I confirmed that
from the Blueprint rather than from `TASK-1154`'s claim about it.

⛔ **`save_assets([])` was never called. I called no save at all** — the check is read-only.

### 2.3 Reader control (`SC-§96`)

The property reader was proven able to **fail** before its answers were believed: my first call
used the *class* path `…BP_SiegeFog_C` and the server answered
`the following properties could not be read: generalData, noiseData, boxMaterials, material,
zzNoSuchPropertyControl` — a **loud** refusal naming every property, including a deliberate
nonsense control. The successful CDO read then returned real, distinct, non-default values. An
instrument that says NO when asked wrongly is an instrument whose YES is worth something.

### 2.4 ⚠️ FINDING — the artist's §4 hash table is STALE against disk

`handoffs/TASK-1152-artist.md` §4 records the post-change `BP_SiegeFog.uasset` as
**`dbea1466…` (38,335 B)**. **Disk carries `6cf85fa9bb25…f342d5` (38,225 B).**

Not a defect, and not silently accepted: ⭐ `TASK-1154` ran **second on the same asset in the same
session**, set the CDO `material` slot during its experiment, **reverted it to `None` and re-saved**
— which is why the file is 110 bytes smaller and 14 s newer than the MI. My CDO read confirms the
end state is what `TASK-1152` intended **plus** that revert, and nothing else. **The bytes I
verified and shipped are the ones on disk; the handoff's hash is a snapshot of an instant that had
already passed** (`SC-§91`). Recorded because a hash mismatch that is *explained* still has to be
*said*.

### 2.5 Fences

| fence | before | after | verdict |
|---|---|---|---|
| `Content/Maps/L_Arena.umap` SHA-256 | `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` | **identical** | ✅ |
| `L_Arena` mtime | `2026-09-05 00:42:58.649187600 -0700` | **unmoved** | ✅ |
| `L_Arena` ever saved | **never** — `GFX-§11` | | ✅ |
| `Content/FogArea/**` (vendor) | `git status` **empty**, and **0 files** in `20c1bea` | | ✅ |
| promised evidence PNGs exist | **9/9 on disk**, all 9 in the commit | | ✅ |
| `testvideo/**` | 0 files in the commit | | ✅ |

---

## 3. `SC-§106` — EVERY PATHSPEC RESOLVED AGAINST DISK BEFORE STAGING

A pathspec commit against a missing path stages **nothing, silently**, and `git show --stat` reads
green for everything else. So each of the 18 candidate paths was tested with `[ -f ]` **before**
`git add`, and the checker was proven able to answer NO:

- **18/18 `EXISTS`** (the 17 committed + `CONVENTIONS.md`).
- **negative control:** `.claude/pipeline/handoffs/TASK-9999-nobody.md` → `MISSING`. The checker
  can say NO, so its 18 yeses are readings rather than a stuck return.

### The derived set vs the dispatch floor (`SC-§91` — the divergence is NAMED, which is mandatory)

Derived at **my own** instant from
`git status --short --untracked-files=all --ignored -- .claude/pipeline/`, not from the floor list.

| | file | why |
|---|---|---|
| **+1** | `handoffs/TASK-1149-buildmaster.md` | **not on the floor.** Genuine orphan under `TL-§5e` cl. 7a — its own host committed at `60dca54` **without it**, so cl. 7a-v's "scheduled, not abandoned" exception (which needs a host that has *not yet* committed) does not apply. Same reasoning `TASK-1149` itself used to sweep `TASK-1124-buildmaster.md`. |
| **−1** | `.claude/pipeline/CONVENTIONS.md` | **on the floor, but CLEAN at my instant** — zero modifications, nothing to stage. Staging it would have been a no-op that read as a change. **Disk wins; the list is what is stale.** |
| **+13** | the 13 `HELD-FOR: TASK-1158` documents | re-derived independently, **13/13 present**, matching `TASK-1149` §6 exactly. |

⇒ **17 files**, against a floor that named 7 categories. The count is a floor and a hint, never the set.

⛔ Every `??` was staged **by explicit path**. No `-A`, no `.`, no bare directory.
⛔ `handoffs/TASK-1158-buildmaster.md` — **this file** — was never staged.
⛔ `.git/index.lock` checked absent immediately before staging **and** immediately before committing.

---

## 4. LFS — oid vs sha256, NEVER size, with the reader's POSITIVE CONTROL

⭐ `TASK-1149`'s comparator hit `SC-§102`'s trap (**this repo's git root is
`C:/GitProjects/GitHub/GitClaudeUnrealTesting`, one level ABOVE the project dir**) and printed
`MISMATCH` on five PNGs from an **empty** oid. So I controlled my reader before believing a single
match.

**Negative control — the mis-anchored form, which is exactly how `TASK-1149` was fooled:**

```
$ git cat-file -p ":Content/Blueprints/BP_SiegeFog.uasset"
fatal: path 'Content/Blueprints/BP_SiegeFog.uasset' does not exist (neither on disk nor in the index)
   -> 101 bytes of REFUSAL
```

**Positive control — the same file, correctly anchored, byte count printed BEFORE any comparison:**

```
$ git cat-file -p ":GitClaudeUnrealTest/Content/Blueprints/BP_SiegeFog.uasset"     -> 129 bytes
version https://git-lfs.github.com/spec/v1
oid sha256:6cf85fa9bb25131d387e4335e6440e9743ae0612ba7962bda1ef39b4f7f342d5
size 38225
```

⇒ the reader returns a **known hit** as non-empty and a **known miss** as a loud fatal. Its verdicts
are measurements. `git check-attr` separately confirms `filter: lfs` on `.uasset` **and** `.png`, so
"LFS-tracked" is checked rather than assumed.

**All 11 binaries — every pointer non-empty (128–131 B) before comparison:**

| file | ptr bytes | index oid (12) | disk sha256 (12) | |
|---|---|---|---|---|
| `BP_SiegeFog.uasset` | 129 | `6cf85fa9bb25` | `6cf85fa9bb25` | ✅ |
| `MI_SiegeFog_Grey.uasset` | 128 | `0b4e19565249` | `0b4e19565249` | ✅ |
| `TASK-1151-editor-heightfog-only-castle-crisp-at-50000uu.png` | 131 | `dc819b8338ca` | `dc819b8338ca` | ✅ |
| `TASK-1152-AFTER-uniform-across-map-and-time.png` | 131 | `1257ed7cbde6` | `1257ed7cbde6` | ✅ |
| `TASK-1152-BEFORE-fixed-camera-swings-11x-in-time.png` | 131 | `af58439906fa` | `af58439906fa` | ✅ |
| `VID-007-t00m03s-control-no-fog-before-card.png` | 131 | `ec7dcacca879` | `ec7dcacca879` | ✅ |
| `VID-007-t00m20s-pole-a-total-whiteout.png` | 130 | `1a80e5d798cb` | `1a80e5d798cb` | ✅ |
| `VID-007-t00m27s-reference-standard.png` | 131 | `45d254626d5d` | `45d254626d5d` | ✅ |
| `VID-007-t00m43s-pole-b-near-clear.png` | 131 | `22813db01eea` | `22813db01eea` | ✅ |
| `VID-007-t00m48s-step-after-opaque.png` | 130 | `ac8121043f82` | `ac8121043f82` | ✅ |
| `VID-007-t00m48s-step-before-clear.png` | 131 | `017a22cfca9f` | `017a22cfca9f` | ✅ |

**11/11 MATCH by oid. Zero verified by size.** The comparator distinguishes *"the values differ"*
from *"I failed to read a value"* by branching on `ptr_bytes < 100 || oid == ""` → `READER-DEAD`,
a third verdict that is neither MATCH nor MISMATCH (`SC-§96`).

---

## 5. THE COMMIT — verified on the COMMIT, not the index

`git show --stat HEAD`:

```
 GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md  |  14 +-
 .../footage/VID-007-fog-visibility-swings-4x.md    | 178 ++++++++++
 .../pipeline/handoffs/TASK-1149-buildmaster.md     | 360 +++++++++++++++++++++
 .../.claude/pipeline/handoffs/TASK-1151-artist.md  | 214 ++++++++++++
 .../.claude/pipeline/handoffs/TASK-1152-artist.md  | 272 ++++++++++++++++
 .../.claude/pipeline/handoffs/TASK-1154-artist.md  | 202 ++++++++++++
 ...itor-heightfog-only-castle-crisp-at-50000uu.png |   3 +
 ...TASK-1152-AFTER-uniform-across-map-and-time.png |   3 +
 ...1152-BEFORE-fixed-camera-swings-11x-in-time.png |   3 +
 .../VID-007-t00m03s-control-no-fog-before-card.png |   3 +
 .../VID-007-t00m20s-pole-a-total-whiteout.png      |   3 +
 .../VID-007-t00m27s-reference-standard.png         |   3 +
 .../VID-007-t00m43s-pole-b-near-clear.png          |   3 +
 .../VID-007-t00m48s-step-after-opaque.png          |   3 +
 .../VID-007-t00m48s-step-before-clear.png          |   3 +
 .../Content/Blueprints/BP_SiegeFog.uasset          |   4 +-
 .../Materials/Instances/MI_SiegeFog_Grey.uasset    |   3 +
 17 files changed, 1265 insertions(+), 9 deletions(-)
```

**17 files against my derived list of 17 — no stray.** Audited on `20c1bea`'s own file list:

```
Source/  0   Config/  0   Tools/  0   L_Arena.umap  0
Content/FogArea/  0   testvideo/  0   TASK-1158-buildmaster  0
```

No soft-reset was needed. **`main` 6 ahead of `origin/main`, 0 behind. NOT PUSHED.**

---

## 6. WHAT STAYED DIRTY, AND WHOSE

| path | whose | why |
|---|---|---|
| `.claude/pipeline/TASKBOARD.md` | ⭐ **mine** | the **hash text only** — the two `- status:` lines now read `20c1bea`. The **flip** rode *inside* `20c1bea`; a row cannot carry its own commit's hash (`TL-§5e` cl. 7, the `60dca54`/`42734b7` precedent). **Bounded at one edit, never amended.** |
| `handoffs/TASK-1158-buildmaster.md` | ⭐ **mine, orphaned by construction** | this file. Next commit host takes it under cl. 7a. |

Everything else is clean: `git status --short --untracked-files=all` was **completely empty**
immediately after the commit. `Tools/ArtPipeline/pipeline_manifest.json` — modified at the session's
start per the opening snapshot — is **clean at my instant** and was never mine; recorded because a
silent disappearance is worth a sentence, and `TASK-1149` observed the same.

## 7. Board

`Edit` tool only. Never `Write`, never a shell rewrite.

- ⭐ `TASK-1158` `- status:` → `done 2026-09-08 (TASK-1158) — committed 20c1bea`, prior text kept as history.
- ⭐ `TASK-1152` `- status:` → `done 2026-09-08 — shipped 20c1bea by TASK-1158`, its whole measurement narrative kept as history.
- ⛔ `TASK-1154`, `1165`, `1166`, `1167`, `1168` — **not touched.**
- ⭐ `TASK-1151` was **already `DONE`**; its documents rode along under the cl. 7a-v hold, so no flip was owed or made.

## 8. 🙋 FOR THE ORCHESTRATOR / MANAGER

1. ⚖️ **`SC-§103` reading I made, and it should be checked.** The message *names* `TASK-1154` — but
   only to say ask (C) is **NOT** shipped. I read §103's "flip every id the message carries" as
   binding on ids the message claims as **shipped**, and the dispatch forbade touching that row, so
   I flipped **1158 + 1152 only**. If the manager reads it otherwise, `TASK-1154`'s row is the one
   to revisit — **not this commit**.
2. ⚠️ **The `TASK-1152` §4 hash discrepancy (§2.4)** — explained by `TASK-1154`'s revert-and-re-save,
   but it means **two art rows saved the same `.uasset` in one session** and only the second one's
   bytes exist. That worked here because the second row reverted cleanly; as a pattern it is one
   forgotten revert away from shipping an experiment.
3. 🧑 **`TASK-1159` cl. (2) is now unblocked and cl. (3) is NOT.** He can judge **uniformity** today.
   ⛔ **He must not judge COLOUR** — the fog is still the same beige until `TASK-1165` (or his own
   ~30-second hand edit: `BP_SiegeFog` → Class Defaults → `Box Materials["Base"]` → `MI_SiegeFog_Grey`
   → Save). Judging it now would record his verdict against the wrong picture.
4. ⛔ **Nothing was pushed.** `main` is **6 ahead**.
